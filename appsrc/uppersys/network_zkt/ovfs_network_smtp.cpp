/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 :
 	1.email发送方法有两种，ssl和非ssl，非ssl直接建立tcp套接字发送smtp的数据包来发邮件，格式需要严格按照smtp协议来进行
	 	ssl方式使用的是第三方工具mutt+msmtp组合来进行发送
	2.邮件的处理分为两种，一种是立即发送，另外一种是异步线程发送，异步发送时将任务添加进任务链表中，后台线程不断的从任务链表中取出任务然后发送。
 *	源 文 件 :ovfs_network_smpt.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2012/12/23
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <errno.h>
#include <time.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "curl/curl.h"
#include "ovfs_network_smtp.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_utility_file_operate.h"
#include "ovfs_network_utility_api.h"

using namespace ovfs_soft;
using namespace ovfs_network;

static const char* muttrc_content[] = 
{
	"set send_charset=\"utf-8\"\n",
	"set charset=UTF-8\n",
	"set locale=zh_CN.UTF-8\n",
	"set use_from=yes\n",
	"set envelope_from=yes\n",
	"set sendmail_wait=0\n",
	"set copy=no\n"
};

OVFS_ERR ovfs_network_smtp::set_email_sender(const ovfs_soft::ovfs_email_sender_cfg *cfg)
{
	//更新发送邮件人的信息
	Common_Lock(m_cfg_lock);
	m_email_cfg = *cfg;
	Common_UnLock(m_cfg_lock);

	ovfs_cfgm_set_email_sender(cfg);
	
	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_smtp::set_email_receiver(const ovfs_soft::ovfs_email_receiver_cfg *cfg, U32 receiver_num)
{
	if(cfg == NULL)
		return OVFS_ERR_NETWORK_BASE;

	//更新接收邮件人的信息
	Common_Lock(m_cfg_lock);
	m_recvier_list.node_num = 0;
	for(U32 i = 0; i < receiver_num && i<OVFS_ARRAY_DIM(m_recvier_list.receiver); ++i)
	{
		m_recvier_list.receiver[i] = cfg[i];
		m_recvier_list.node_num++;
	}
	Common_UnLock(m_cfg_lock);

	ovfs_cfgm_set_email_receiver(cfg, receiver_num);
	
	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_smtp::send_email(const S8 *title, 
					const S8 *content, 
					const ovfs_soft::ovfs_email_attachment_fname *attachment,
				 	U32 attachment_num,
				 	OVFS_BOOL is_sync,
				 	OVFS_BOOL is_need_rm_attach)
{
	Common_Lock(m_cfg_lock);
	if(m_email_cfg.enable != OVFS_TRUE)
	{
		if(attachment != NULL && is_need_rm_attach == OVFS_TRUE)
		{
			for(U32 i = 0; i < attachment_num; ++i)
			{
				S8 cmd[256];
				snprintf(cmd, sizeof(cmd), "rm %s -rf", attachment[i].fname);
				Common_System(cmd);
			}
		}
		Common_UnLock(m_cfg_lock);
		return OVFS_ERR_NETWORK_BASE;
	}
	ovfs_email_task_node *node = malloc_email(title, content, attachment, attachment_num,is_need_rm_attach);
	Common_UnLock(m_cfg_lock);
	
	if(NULL == node)
		return OVFS_ERR_NETWORK_BASE;
	
	if(is_sync == OVFS_TRUE)
	{
		OVFS_ERR ret = send_email_sync(node);
		free_email(node);
		
		if(OVFS_SUCCESS != ret)
			return OVFS_ERR_NETWORK_BASE;
	}
	else
	{
		Common_Lock(m_task_lock);
		if(OVFS_SUCCESS != push_email(node))
		{
			Common_UnLock(m_task_lock);
			free_email(node);
			return OVFS_ERR_NETWORK_BASE;
		}
		Common_UnLock(m_task_lock);
	}

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_smtp::send_email(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg,
		            const ovfs_soft::ovfs_email_receiver_cfg *recv_cfg,
		            U32 receiver_num,
	                const S8 *title, 
					const S8 *content, 
					const ovfs_soft::ovfs_email_attachment_fname *attachment,
				 	U32 attachment_num,
				 	OVFS_BOOL is_sync,
				 	OVFS_BOOL is_need_rm_attach)
{
	Common_Lock(m_cfg_lock);
	if (sender_cfg != NULL)
	{
		if(sender_cfg->enable != OVFS_TRUE)
		{
			if(attachment != NULL && is_need_rm_attach == OVFS_TRUE)
			{
				for(U32 i = 0; i < attachment_num; ++i)
				{
					S8 cmd[256];
					snprintf(cmd, sizeof(cmd), "rm %s -rf", attachment[i].fname);
					Common_System(cmd);
				}
			}
			Common_UnLock(m_cfg_lock);
			return OVFS_ERR_NETWORK_BASE;
		}
	}
	ovfs_email_task_node *node = malloc_email(sender_cfg, recv_cfg, receiver_num, title, content, attachment, attachment_num,is_need_rm_attach);
	Common_UnLock(m_cfg_lock);
	LOGD("*********************\n");	
	if(NULL == node)
		return OVFS_ERR_NETWORK_BASE;
	LOGD("*********************\n");	
	if(is_sync == OVFS_TRUE)
	{
		LOGD("*********************\n");
		OVFS_ERR ret = send_email_sync(node);
		free_email(node);
		
		if(OVFS_SUCCESS != ret)
			return OVFS_ERR_NETWORK_BASE;
	}
	else
	{
		LOGD("*********************\n");
		Common_Lock(m_task_lock);
		if(OVFS_SUCCESS != push_email(node))
		{
			Common_UnLock(m_task_lock);
			free_email(node);
			return OVFS_ERR_NETWORK_BASE;
		}
		Common_UnLock(m_task_lock);
	}

	return OVFS_SUCCESS;
}
							
OVFS_ERR ovfs_network_smtp::send_email_tst()
{
	const S8 *title = "Test email for NVR";
	S8 content[64];

#if 0
	const ovfs_device_capability *dev_cap = ovfs_cfgm_get_dev_cap();
	snprintf(content, sizeof(content), "This email is form NVR, SN is %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			dev_cap->serial_no[0],
			dev_cap->serial_no[1],
			dev_cap->serial_no[2],
			dev_cap->serial_no[3],
			dev_cap->serial_no[4],
			dev_cap->serial_no[5],
			dev_cap->serial_no[6],
			dev_cap->serial_no[7],
			dev_cap->serial_no[8],
			dev_cap->serial_no[9]);
#endif
    S8 serial_no[32];
    OVFS_CLR_ARG(serial_no);
    ovfs_cfgm_get_serialno(serial_no, sizeof(serial_no));
	snprintf(content, sizeof(content), "This email is form NVR, SN is %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			serial_no[0],
			serial_no[1],
			serial_no[2],
			serial_no[3],
			serial_no[4],
			serial_no[5],
			serial_no[6],
			serial_no[7],
			serial_no[8],
			serial_no[9]);
	
	Common_Lock(m_cfg_lock);
	ovfs_email_task_node *node = malloc_email(title, content, NULL, 0, OVFS_FALSE);
	Common_UnLock(m_cfg_lock);

	if(NULL == node)
		return OVFS_ERR_NETWORK_BASE;

	node->is_test = OVFS_TRUE;

	OVFS_ERR ret = send_email_sync(node);

	free_email(node);

	return ret;
}

OVFS_ERR ovfs_network_smtp::send_email_tst(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg
		,const ovfs_soft::ovfs_email_receiver_cfg *recv_cfg
		, U32 receiver_num)
{
	if(NULL == sender_cfg || receiver_num == 0 || NULL == recv_cfg)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	const S8 *title = "Test email for Test";
	S8 content[64];
	OVFS_CLR_ARG(content);
	snprintf(content, sizeof(content), "This email is test email");
	
	Common_Lock(m_cfg_lock);
	ovfs_email_task_node *node = malloc_email(sender_cfg, recv_cfg, receiver_num, title, content, NULL, 0, OVFS_FALSE);
	Common_UnLock(m_cfg_lock);

	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	node->is_test = OVFS_TRUE;

	OVFS_ERR ret = send_email_sync(node);

	free_email(node);

	return ret;
}

S32 ovfs_network::email_send_thread(Common_Thread_T hThreadHandle,void *para)
{
	OVFS_ASSERT(NULL != para);

	ovfs_network_smtp *smtp = (ovfs_network_smtp *)para;
	
	while(1)
	{
		smtp->send_fun();
		Common_Sleep(1, 0);
	}

	return 0;
}

OVFS_VOID ovfs_network_smtp::send_fun()
{
	Common_Lock(m_task_lock);
	ovfs_email_task_node *node = pop_email();
	Common_UnLock(m_task_lock);
	while(node != NULL)
	{
		if(node->send_times > OVFS_MAX_EMAIL_SEND_TIMES)
		{
			free_email(node);
		}
		else
		{
			OVFS_ERR ret = send_email_sync(node);
			if(ret == OVFS_SUCCESS)
			{
				free_email(node);
			}
			else
			{
				node->send_times++;
				Common_Lock(m_task_lock);

				if(push_email(node) != OVFS_SUCCESS)
					free_email(node);

				Common_UnLock(m_task_lock);
			}
		}
		Common_Lock(m_task_lock);
		node = pop_email();
		Common_UnLock(m_task_lock);
	}
}

ovfs_email_task_node *ovfs_network_smtp::malloc_email(
				const ovfs_soft::ovfs_email_sender_cfg *sender_cfg
				,const ovfs_soft::ovfs_email_receiver_cfg *recv_cfg
				,U32 receiver_num
				,const S8 *title 
				,const S8 *content 
				,const ovfs_soft::ovfs_email_attachment_fname *attachment
			 	,U32 attachment_num
			 	,OVFS_BOOL is_need_rm_attach)
{
	LOGD("*********************\n");
	//ovfs_email_task_node *task_node = (ovfs_email_task_node *)Common_Malloc(sizeof(ovfs_email_task_node), 0,__FUNCTION__,__LINE__);
	ovfs_email_task_node *task_node = new ovfs_email_task_node;
	if(NULL == task_node)
		return NULL;
	LOGD("*********************\n");
	//memset(task_node, 0, sizeof(ovfs_email_task_node));
	task_node->send_times = 0;
	task_node->is_need_rm_attachment = is_need_rm_attach;
	task_node->attachment = NULL;
	task_node->content = NULL;
	task_node->title = NULL;
	task_node->receiver = NULL;
	task_node->receiver_num = 0;
	task_node->attach_num = 0;	
	//分配邮件附件空间
	{
		if(attachment != NULL)
		{
			if(attachment_num != 0)
			{
				U32 size = sizeof(ovfs_email_attachment_fname)*attachment_num;
				
				task_node->attachment =(ovfs_email_attachment_fname*)Common_Malloc(size, 0,__FUNCTION__,__LINE__);
				if(	task_node->attachment == NULL)
				{
					LOGE("malloc attchment fail! need size = %d\n",size);
					goto leave_fun_err;
				}
				else
				{
					for(U32  i = 0; i < attachment_num; i++)
						snprintf(task_node->attachment[i].fname,sizeof(task_node->attachment[i].fname),"%s",attachment[i].fname);
				}
				task_node->attach_num = attachment_num;
			}
		}
	}
	//分配邮件内容空间
	{
		U32 size = 1;
		if(content != NULL)	
			size = strlen(content)+1;
		task_node->content = (S8 *)Common_Malloc(size, 0,__FUNCTION__,__LINE__);
		
		if(task_node->content == NULL)
		{
			LOGE("malloc content file! need size = %d\n",size);
			goto leave_fun_err;
		}
		
		task_node->content[0] = '\0';
		if(content != NULL)
			snprintf(task_node->content,size,"%s",content);
		
	}

	//分配邮件标题空间
	{
		U32 size = 1;
		if(title != NULL)	
			size = strlen(title)+1;
		
		
		task_node->title = (char *)Common_Malloc(size, 0,__FUNCTION__,__LINE__);
		if(task_node->title == NULL)
		{
			LOGE("malloc buf file! need size = %d\n",size);
			goto leave_fun_err;
		}
		
		task_node->title[0] = '\0';
		if(title != NULL)
			snprintf(task_node->title,size,"%s",title);
	}

	//分配接受者空间

	{
		U32 size = sizeof(ovfs_email_receiver_cfg)*receiver_num;

		if(0 != size)
		{
			task_node->receiver = (ovfs_email_receiver_cfg * )Common_Malloc(size, 0,__FUNCTION__,__LINE__);
			if(NULL == task_node->receiver)
			{
				LOGE("malloc receiver! need size = %d\n",size);
				goto leave_fun_err;
			}
			for(U32 i = 0; i < receiver_num; ++i)
			{
				task_node->receiver[i] = recv_cfg[i];
			}
			task_node->receiver_num = receiver_num;
		}
		else
		{
			LOGE("no receiver\n");
			goto leave_fun_err;
		}
	}

	{
		task_node->ssl_enable = sender_cfg->ssl_enable;
		task_node->server_port = sender_cfg->server_port;
		snprintf(task_node->server_addr, sizeof(task_node->server_addr), "%s", sender_cfg->server_addr);
		snprintf(task_node->user_name, sizeof(task_node->user_name), "%s", sender_cfg->user_name);
		snprintf(task_node->password, sizeof(task_node->password), "%s", sender_cfg->password);
	}
	LOGD("*********************\n");	
	return task_node;
	
leave_fun_err:
	if(is_need_rm_attach && attachment !=NULL)
	{
		for(U32 i = 0; i < attachment_num; ++i)
		{
			S8 cmd[256];
			snprintf(cmd, sizeof(cmd), "rm %s -rf", attachment[i].fname);
			Common_System(cmd);
		}
	}
	
	if (task_node->receiver != NULL)
	{
		Common_Free(task_node->receiver,__FUNCTION__,__LINE__);
		task_node->receiver = NULL;
	}
	
	if (task_node->title != NULL)
	{
		Common_Free(task_node->title,__FUNCTION__,__LINE__);
		task_node->title = NULL;
	}
		

	if (task_node->content != NULL)
	{
		Common_Free(task_node->content,__FUNCTION__,__LINE__);
		task_node->content = NULL;
	}
		

	if(task_node->attachment != NULL)
	{
		Common_Free(task_node->attachment,__FUNCTION__,__LINE__);
		task_node->attachment = NULL;
	}

#if 0
	if(task_node != NULL)
		Common_Free(task_node,__FUNCTION__,__LINE__);
	task_node = NULL;
#endif

	if(task_node != NULL)
	{
		delete task_node;
	    task_node = NULL;		
	}
	
	LOGE("malloc email date file!\n");
	
	return NULL;
}


ovfs_email_task_node *ovfs_network_smtp::malloc_email(
					const S8 *title, 
					const S8 *content, 
					const ovfs_soft::ovfs_email_attachment_fname *attachment,
				 	U32 attachment_num,
				 	OVFS_BOOL is_need_rm_attach)
{
	//ovfs_email_task_node *task_node = (ovfs_email_task_node *)Common_Malloc(sizeof(ovfs_email_task_node), 0,__FUNCTION__,__LINE__);
	ovfs_email_task_node *task_node = new ovfs_email_task_node;
	if(NULL == task_node)
		return NULL;

	//memset(task_node, 0, sizeof(ovfs_email_task_node));
    task_node->send_times = 0;
	task_node->is_need_rm_attachment = is_need_rm_attach;
	task_node->attachment = NULL;
	task_node->content = NULL;
	task_node->title = NULL;
	task_node->receiver = NULL;
	task_node->receiver_num = 0;
	task_node->attach_num = 0;
	//分配邮件附件空间
	{
		if(attachment != NULL)
		{
			if(attachment_num != 0)
			{
				U32 size = sizeof(ovfs_email_attachment_fname)*attachment_num;
				
				task_node->attachment =(ovfs_email_attachment_fname*)Common_Malloc(size, 0,__FUNCTION__,__LINE__);
				if(	task_node->attachment == NULL)
				{
					LOGE("malloc attchment fail! need size = %d\n",size);
					goto leave_fun_err;
				}
				else
				{
					for(U32  i = 0; i < attachment_num; i++)
						snprintf(task_node->attachment[i].fname,sizeof(task_node->attachment[i].fname),"%s",attachment[i].fname);
				}
				task_node->attach_num = attachment_num;
			}
		}
	}
	//分配邮件内容空间
	{
		U32 size = 1;
		if(content != NULL)	
			size = strlen(content)+1;
		task_node->content = (S8 *)Common_Malloc(size, 0,__FUNCTION__,__LINE__);
		
		if(task_node->content == NULL)
		{
			LOGE("malloc content file! need size = %d\n",size);
			goto leave_fun_err;
		}
		
		task_node->content[0] = '\0';
		if(content != NULL)
			snprintf(task_node->content,size,"%s",content);
		
	}

	//分配邮件标题空间
	{
		U32 size = 1;
		if(title != NULL)	
			size = strlen(title)+1;
		
		
		task_node->title = (char *)Common_Malloc(size, 0,__FUNCTION__,__LINE__);
		if(task_node->title == NULL)
		{
			LOGE("malloc buf file! need size = %d\n",size);
			goto leave_fun_err;
		}
		
		task_node->title[0] = '\0';
		if(title != NULL)
			snprintf(task_node->title,size,"%s",title);
	}

	//分配接受者空间

	{
		U32 size = sizeof(ovfs_email_receiver_cfg)*m_recvier_list.node_num;

		if(0 != size)
		{
			task_node->receiver = (ovfs_email_receiver_cfg * )Common_Malloc(size, 0,__FUNCTION__,__LINE__);
			if(NULL == task_node->receiver)
			{
				LOGE("malloc receiver! need size = %d\n",size);
				goto leave_fun_err;
			}
			for(U32 i = 0; i < m_recvier_list.node_num; ++i)
			{
				task_node->receiver[i] = m_recvier_list.receiver[i];
			}
			task_node->receiver_num = m_recvier_list.node_num;
		}
		else
		{
			LOGE("no receiver\n");
			goto leave_fun_err;
		}
	}

	{
		task_node->ssl_enable = m_email_cfg.ssl_enable;
		task_node->server_port = m_email_cfg.server_port;
		snprintf(task_node->server_addr, sizeof(task_node->server_addr), "%s", m_email_cfg.server_addr);
		snprintf(task_node->user_name, sizeof(task_node->user_name), "%s", m_email_cfg.user_name);
		snprintf(task_node->password, sizeof(task_node->password), "%s", m_email_cfg.password);
	}
	
	return task_node;
	
leave_fun_err:
	if(is_need_rm_attach && attachment !=NULL)
	{
		for(U32 i = 0; i < attachment_num; ++i)
		{
			S8 cmd[256];
			snprintf(cmd, sizeof(cmd), "rm %s -rf", attachment[i].fname);
			Common_System(cmd);
		}
	}
	
	if (task_node->receiver != NULL)
	{
		Common_Free(task_node->receiver,__FUNCTION__,__LINE__);
		task_node->receiver = NULL;
	}
	
	if (task_node->title != NULL)
	{
		Common_Free(task_node->title,__FUNCTION__,__LINE__);
		task_node->title = NULL;
	}
		

	if (task_node->content != NULL)
	{
		Common_Free(task_node->content,__FUNCTION__,__LINE__);
		task_node->content = NULL;
	}
		

	if(task_node->attachment != NULL)
	{
		Common_Free(task_node->attachment,__FUNCTION__,__LINE__);
		task_node->attachment = NULL;
	}
	
#if 0	
	if(task_node != NULL)
		Common_Free(task_node,__FUNCTION__,__LINE__);
	task_node = NULL;
#endif

	if(task_node != NULL)
	{
		delete task_node;
	    task_node = NULL;		
	}	

	LOGE("malloc email date file!\n");
	
	return NULL;
}

OVFS_VOID ovfs_network_smtp::free_email(ovfs_email_task_node *task_node)
{
	if (task_node->receiver != NULL)
	{
		Common_Free(task_node->receiver,__FUNCTION__,__LINE__);
		task_node->receiver = NULL;
	}
	
	if (task_node->title != NULL)
	{
		Common_Free(task_node->title,__FUNCTION__,__LINE__);
		task_node->title = NULL;
	}

	if (task_node->content != NULL)
	{
		Common_Free(task_node->content,__FUNCTION__,__LINE__);
		task_node->content = NULL;
	}
	
	if(task_node->attachment != NULL)
	{
		if(task_node->is_need_rm_attachment)
		{
			for(U32 i = 0; i < task_node->attach_num; ++i)
			{
				S8 cmd[256];
				snprintf(cmd, sizeof(cmd), "rm %s -rf", task_node->attachment[i].fname);
				Common_System(cmd);
			}
		}
		Common_Free(task_node->attachment,__FUNCTION__,__LINE__);
		task_node->attachment = NULL;
	}

#if 0	
	if(task_node != NULL)
		Common_Free(task_node,__FUNCTION__,__LINE__);
	task_node = NULL;
#endif

	if(task_node != NULL)
	{
		delete task_node;
	    task_node = NULL;		
	}
	
	return ;
}

OVFS_ERR ovfs_network_smtp::push_email(ovfs_email_task_node *task_node)
{
	if(NULL == task_node)
		return OVFS_ERR_NETWORK_BASE;
	
	if(m_email_task_list.task_num >= OVFS_MAX_EMAIL_TASK_NUM)
		return OVFS_ERR_NETWORK_BASE;

	task_node->next = NULL;
	task_node->prev = NULL;
	
	if(m_email_task_list.head == NULL)
	{
		m_email_task_list.head = task_node;
		m_email_task_list.tail = task_node;
	}
	else
	{
		task_node->prev = m_email_task_list.tail;
		m_email_task_list.tail->next = task_node;
		m_email_task_list.tail = task_node;
	}
	
	m_email_task_list.task_num++;
	return OVFS_SUCCESS;
}

ovfs_email_task_node * ovfs_network_smtp::pop_email()
{
	if(m_email_task_list.head == NULL)
		return NULL;

	ovfs_email_task_node *node = m_email_task_list.head;

	m_email_task_list.head = node->next;

	if(m_email_task_list.head == NULL)
	{
		m_email_task_list.tail = NULL;
	}
	else
	{
		m_email_task_list.head->prev = NULL;
	}

	m_email_task_list.task_num--;
	return node;
}

OVFS_ERR ovfs_network_smtp::send_email_sync(ovfs_email_task_node *task_node)
{
	if(NULL == task_node)
	{
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}
	if(NULL == task_node->title
		|| NULL == task_node->content
		||NULL ==  task_node->receiver)
	{
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}

#if 0		
	if(task_node->ssl_enable == OVFS_TRUE)
		return send_ssl_email(task_node);
	else
		return send_non_ssl_email(task_node);

	return OVFS_ERR_NETWORK_INVALID_PARA;
#endif

    return send_email_curl(task_node);
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
    size_t num_bytes = size * nmemb;    
    char* data = (char*)ptr;    
    std::stringstream* strstream = (std::stringstream*)userp; 
	
    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) 
	{
        return 0;
    } 
	
    strstream->read(data, num_bytes);    
  
    return strstream->gcount();  
}

char g_array_buf[1024] = "";
static int  g_array_len = 0;
static int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  	(void)handle; /* prevent compiler warning */
  	(void)userp;
	char tmpStr[][16] = {"Fail", "limit", "invalid","unexpect","disable","denied"};
	unsigned int tmpIndex = 0;
	
 	if(type == CURLINFO_HEADER_IN)
	{
		while(tmpIndex < sizeof(tmpStr)/sizeof(tmpStr[0]))
		{
			if(strcasestr(data, tmpStr[tmpIndex])  != NULL)
			{
				if(g_array_len + size + 1 < sizeof(g_array_buf))
				{
					memcpy(g_array_buf+g_array_len, data, size);
					g_array_len += size;
					g_array_buf[g_array_len] = '\0';
					g_array_len += 1;
				}
				return 0;
			}
			tmpIndex++;
		}
	}
  	return 0;
}

OVFS_ERR ovfs_network_smtp::send_email_curl(ovfs_email_task_node *node)
{
	S8 url[128];
	CURL *curl;
	CURLcode res = CURLE_OK;
   	struct curl_slist *recipients = NULL;

	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}
	 
	LOGD("Para:\n");
	LOGD("smtp_server:	%s\n", node->server_addr);
	LOGD("smtp_port:	%d\n", node->server_port);
	LOGD("username:	%s\n", node->user_name);
	LOGD("password:	%s\n", node->password);
	LOGD("from_address:	%s\n", node->user_name);
	LOGD("mail_title:	%s\n", node->title);
	LOGD("mail_content:	%s\n", node->content);
	if(node->attachment != NULL)
	{
		for(U32 i = 0; i < node->attach_num;i++)
		{
			LOGD("attach_file_list:	%s\n", node->attachment[i].fname);
		}
	}
	LOGD("attach_file_num:	%d\n", node->attach_num);

	int total_num = (node->receiver_num > OVFS_MAX_EMAIL_RECV_MAS_NUM)?OVFS_MAX_EMAIL_RECV_MAS_NUM:node->receiver_num;
	LOGD("*********************total_num=%d\n",total_num);
	if(node->receiver != NULL)
	{
		for(U32 i = 0; i < node->receiver_num; i++)
		{
			LOGD("%s	\n",node->receiver[i].addr);
		}
	}
	LOGD("*********************\n");
	
    curl = curl_easy_init();
    if (curl) 
	{
	    curl_easy_setopt(curl, CURLOPT_USERNAME, node->user_name);
	    curl_easy_setopt(curl, CURLOPT_PASSWORD, node->password);
		if (node->ssl_enable == OVFS_TRUE && node->server_port != 25)
		{
		    snprintf(url, sizeof(url), "smtps://%s:%d", node->server_addr, node->server_port);
		}
		else
		{
		    snprintf(url, sizeof(url), "smtp://%s:%d", node->server_addr, node->server_port);
		}
		LOGD("url = %s\n", url);
	    curl_easy_setopt(curl, CURLOPT_URL, url);

		if (node->ssl_enable == OVFS_TRUE)
		{
			if (node->server_port == 25)
			{
			    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
			}
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

	    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, node->user_name);
        for (U32 i = 0; i < node->receiver_num; i++) 
		{    
	  		if(strlen(node->receiver[i].addr) == 0)
	  		{
				continue;
	  		}
			
            recipients = curl_slist_append(recipients, node->receiver[i].addr);    
        }
		
		memset(g_array_buf, 0, sizeof(g_array_buf));
		g_array_len = 0;

		CreateMessage(node);		
        node->stream.str(node->m_strMessage.c_str());    
        node->stream.flush(); 
	
	    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
	    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&(node->stream));	
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L); 
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
	
		res = curl_easy_perform(curl);
		if(res == CURLE_OK)
		{
			snprintf(g_array_buf, sizeof(g_array_buf), "%s", "Send Test Mail OK");
		}
	    else
	    {
	        	LOGE("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			LOGE("curl code is %d\n",res);
			switch(res)
			{
				case CURLE_OPERATION_TIMEOUTED:
					snprintf(g_array_buf, sizeof(g_array_buf), "%s", "NetWork TimeOut");
					break;
				case CURLE_COULDNT_RESOLVE_HOST:
					snprintf(g_array_buf, sizeof(g_array_buf), "%s", "Couldn't Resolve Host Name");
					break;
				case CURLE_COULDNT_CONNECT:
					snprintf(g_array_buf, sizeof(g_array_buf), "%s", "Couldn't Connect Server");
					break;
				default:
					break;
			}
			
	    }
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
	    return res == CURLE_OK ? OVFS_SUCCESS : OVFS_ERR_NETWORK_BASE;
	  
	}

	return OVFS_ERR_NETWORK_BASE;
}

OVFS_ERR ovfs_network_smtp::send_non_ssl_email(ovfs_email_task_node *node)
{
	int socket = 0;
	struct tm* timeinfo;
	char month[][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	time_t stime;
	time(&stime);
	timeinfo = localtime(&stime);
	S8 date[128];

	snprintf(date, sizeof(date), "%d %s %d %d:%d:%d", timeinfo->tm_mday,month[timeinfo->tm_mon],timeinfo->tm_year+1900,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec); 
	

	LOGD("Para:\n");
	LOGD("smtp_server:	%s\n", node->server_addr);
	LOGD("smtp_port:	%d\n", node->server_port);
	LOGD("username:	%s\n", node->user_name);
	LOGD("password:	%s\n", node->password);
	LOGD("from_address:	%s\n", node->user_name);
	LOGD("mail_title:	%s\n", node->title);
	LOGD("mail_content:	%s\n", node->content);
	if(node->attachment != NULL)
	{
		for(U32 i = 0; i < node->attach_num;i++)
		{
			LOGD("attach_file_list:	%s\n", node->attachment[i].fname);
		}
	}
	LOGD("attach_file_num:	%d\n", node->attach_num);

	int total_num = (node->receiver_num > OVFS_MAX_EMAIL_RECV_MAS_NUM)?OVFS_MAX_EMAIL_RECV_MAS_NUM:node->receiver_num;
	LOGD("*********************\n");
	if(node->receiver != NULL)
	{
		for(U32 i = 0; i < node->receiver_num; i++)
		{
			LOGD("%s	\n",node->receiver[i].addr);
		}
	}
	LOGD("*********************\n");
	
	
	{
		socket=CreateSocket();
		if(-1 == socket)
		{
			LOGE("socket create error\n");
			return OVFS_ERR_NETWORK_BASE;
		}
		LOGD("create the socket ok\n");

		OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(ConnectHost(socket,node->server_addr, node->server_port), "ConnectHost");

		LOGD("connect the smtp host ok\n");
		
		LOGD("%s %s\n", node->user_name,node->password);
		
		OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(LogIn(socket, node->user_name,node->password), "LogIn");
	}
	OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(SendMail(socket,node->user_name,node->receiver,total_num, date, node->title, node->content), "SendMail");

	OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(SendAttachment(socket, node->attachment, node->attach_num), "SendAttachment");
	

	OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(End(socket), "End");
	OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(Quit(socket), "Quit");

	killsocket(socket);
	return OVFS_SUCCESS;

leave_fun:
	killsocket(socket);
	return OVFS_ERR_NETWORK_BASE;
	
}

OVFS_ERR ovfs_network_smtp::send_ssl_email(ovfs_email_task_node *node)
{
	int total_num = (node->receiver_num > OVFS_MAX_EMAIL_RECV_MAS_NUM)?OVFS_MAX_EMAIL_RECV_MAS_NUM:node->receiver_num;
	char to_addr_buf[64*total_num+1];
	
	LOGW("total_num = %d\n",total_num);
	U32 index = 0;
	to_addr_buf[0] = '\0';
	for(int i = 0; i < total_num; i ++)
	{
		LOGD("%d->%s\n",i, node->receiver[i].addr);
		if(strlen(node->receiver[i].addr) != 0)
		{
			if(index == 0)
			{
				snprintf(to_addr_buf,sizeof(to_addr_buf),"%s", node->receiver[i].addr);
			}		
			else
			{
				snprintf(to_addr_buf + strlen(to_addr_buf),sizeof(to_addr_buf) - strlen(to_addr_buf)," %s",node->receiver[i].addr);
			}
			index++;
		}
		LOGW("receiver[%d]:%s\n ",i, node->receiver[i].addr);
			
	}
	LOGW("%s\n", to_addr_buf);	
		//sprintf(to_addr_buf, "%s %s", to_addr_buf, to_addr->addr[i].addr);

	update_mail_cfg_file(node);
	if (use_msmtp() == OVFS_SUCCESS)
	{

		return deliver_mail(to_addr_buf, node->title, node->content, node->attachment, node->attach_num, node->is_test);

	}
	else
	{
		LOGE("lack of documents\n");
//		OVFS_ASSERT(0);
		return OVFS_ERR_NETWORK_BASE;
	}
	return OVFS_SUCCESS;
}

ovfs_network_smtp::ovfs_network_smtp()
{
    OVFS_CLR_ARG(m_task_lock);
	Common_Lock_Create(&m_task_lock, NULL);
	
    OVFS_CLR_ARG(m_cfg_lock);
	Common_Lock_Create(&m_cfg_lock, NULL);
	

	OVFS_CLR_ARG(m_email_task_list);
	OVFS_CLR_ARG(m_recvier_list);
	OVFS_CLR_ARG(m_email_cfg);
	m_email_file_no = 0;

	ovfs_cfgm_get_email_sender(&m_email_cfg);
	U32 reciever_cont=0;
	if(OVFS_SUCCESS == ovfs_cfgm_get_email_receiver_num(&reciever_cont))
	{
		for(U32 i = 0; i < reciever_cont; ++i)
		{
			if(OVFS_SUCCESS != ovfs_cfgm_get_email_receiver(i, &m_recvier_list.receiver[i]))
				break;
			m_recvier_list.node_num++;
		}
	}

	
	m_send_handle = NULL;
	Common_Thread_Create(&m_send_handle,"email_send_thread",1024*128,
		                 COMMON_THREAD_CREATEFLAG_NORMAL,email_send_thread,(void *)this);	
}

ovfs_network_smtp::~ovfs_network_smtp()
{
	Common_Lock(m_task_lock);
	ovfs_email_task_node *node = pop_email();
	
	while(NULL != node)
	{
		free_email(node);
		node = pop_email();
	}
	Common_UnLock(m_task_lock);
	
	Common_Lock_Destroy(&m_task_lock);
	Common_Lock_Destroy(&m_cfg_lock);
}

OVFS_ERR ovfs_network_smtp::CreateMessage(ovfs_email_task_node *node)
{
    struct tm* timeinfo;
	char month[][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	time_t stime;
	time(&stime);
	timeinfo = localtime(&stime);
	S8 date[128];
	U32 i = 0;
	
	if(node == NULL)
	{
		return OVFS_ERR_NETWORK_BASE;
	}

    snprintf(date, sizeof(date), "%d %s %d %d:%d:%d", timeinfo->tm_mday,month[timeinfo->tm_mon],timeinfo->tm_year+1900,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	LOGD("*********************\n");
	
	node->m_strMessage = "Date: ";
	node->m_strMessage += date;
	node->m_strMessage += "\r\nFrom: \"";
	node->m_strMessage += node->user_name;
	node->m_strMessage += "\"<";
	node->m_strMessage += node->user_name;
	node->m_strMessage += ">\r\n";
	node->m_strMessage += "X-Mailer: %s";
	node->m_strMessage += "The Bat! (v3.02) Professional\r\n";
	node->m_strMessage += "X-Priority: 3\r\n";
	node->m_strMessage += "To: ";

	for (i = 0; i < node->receiver_num; i++)
	{
	    if (strlen(node->receiver[i].addr) > 0)
	    {
			if (i > 0)
			{
			    node->m_strMessage += ",";
			}
			node->m_strMessage += "<";
			node->m_strMessage += node->receiver[i].addr;
			node->m_strMessage += ">";
	    }
		
	}

	node->m_strMessage += "\r\nSubject: ";
	node->m_strMessage += node->title;
	node->m_strMessage += "\r\nMIME-Version: 1.0\r\n";
	node->m_strMessage += "Content-Type: multipart/mixed;boundary=\"";
	node->m_strMessage += OVFS_SMTP_BONDERY_TEXT;
	node->m_strMessage += "\"\r\n\r\n--";
	node->m_strMessage += OVFS_SMTP_BONDERY_TEXT;
	node->m_strMessage += "\r\nContent-Type: multipart/alternative;boundary=\"";
	node->m_strMessage += OVFS_SMTP_BONDERY_TEXT_EX;
	node->m_strMessage += "\"\r\n\r\n--";
	node->m_strMessage += OVFS_SMTP_BONDERY_TEXT_EX;
	node->m_strMessage += "\r\nContent-Type: text/plain;charset=UTF-8\r\nContent-Transfer-Encoding: 7bit\r\n\r\n";
	node->m_strMessage += node->content;
	node->m_strMessage += "\"\r\n\r\n--";
	node->m_strMessage += OVFS_SMTP_BONDERY_TEXT_EX;
	node->m_strMessage += "--\r\n";

	for (i = 0; i < node->attach_num; i++)
	{
		FILE *fp=NULL;        
		
		fp = fopen(node->attachment[i].fname, "rb");
		if(fp == NULL)
		{
			LOGE("can not open file %s\n", node->attachment[i].fname);
			continue;
		}
	
		const S8 *pstr0 = node->attachment[i].fname;
		const S8 *pstr1 = node->attachment[i].fname;

		do
		{
			pstr1 = strstr(pstr0, "/");
			if(pstr1 != NULL)
			{
				pstr0 = pstr1 + 1;
			}
		}while(pstr1 != NULL);

		node->m_strMessage += "--";
		node->m_strMessage += OVFS_SMTP_BONDERY_TEXT;
		node->m_strMessage += "\r\nContent-Type: image/jpeg; name=";
		node->m_strMessage += pstr0;
		node->m_strMessage += "\r\nContent-Transfer-Encoding: base64\r\n";
		node->m_strMessage += "Content-Disposition: attachment;filename=";
		node->m_strMessage += pstr0;
		node->m_strMessage += "\r\n\r\n";

		S8 datasrc[64];
		S8 datades[128];
		S32 ret_len = 0;
		OVFS_CLR_ARG(datasrc);
		OVFS_CLR_ARG(datades);
		datades[0] = '\0';	
		
		do {
		    ret_len = fread(datasrc, 1, 63, fp);
			
			if(ret_len <= 0)
			{
				break;
			}	
			
			datasrc[ret_len] = '\0';

			OVFS_CLR_ARG(datades);
			ovfs_utility_base64encode22((char *)datades, (unsigned char *)datasrc, ret_len);

			node->m_strMessage += datades;
			node->m_strMessage += "\r\n";
		} while (ret_len == 63);
		
		fclose(fp);
		fp = NULL;		
	}
	LOGD("*********************\n");
	node->m_strMessage += "\"\r\n\r\n--";
	node->m_strMessage += OVFS_SMTP_BONDERY_TEXT;
	node->m_strMessage += "--\r\n";	
	
	return OVFS_SUCCESS;
}

OVFS_VOID ovfs_network_smtp::killsocket(int s)
{
	shutdown(s, SHUT_RDWR);
	close(s);
	return;
}

OVFS_ERR ovfs_network_smtp::GetResponse(int s)
{
	S8 recv_data[1024];
	int rt = recv(s,recv_data,1024,0);
	if(rt == OVFS_SOCKET_ERROR)
	{
		LOGE("receive nothing\n");
		return OVFS_ERR_NETWORK_BASE;
	}
	recv_data[rt]='\0';
	LOGD("----%s\n",recv_data);
	if(*recv_data == '5')
	{
		return OVFS_ERR_NETWORK_BASE;;
	}
	return OVFS_SUCCESS;
}

int ovfs_network_smtp::CreateSocket()
{
	int s = socket(AF_INET,SOCK_STREAM,0);
	if(s == OVFS_SOCKET_ERROR)
	{
		LOGE("socket init error\n");
		killsocket(s);
		return -1;
	}
	//设置发送超时时间为6秒
	struct timeval TimeOut;
	TimeOut.tv_sec = 6;
	TimeOut.tv_usec = 0;
	if(setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,&TimeOut,sizeof(TimeOut))==OVFS_SOCKET_ERROR)
	{
		perror("Error:");
		killsocket(s);
		return -1;
	}
	TimeOut.tv_sec = 6;
	TimeOut.tv_usec = 0;
	//设置接收超时时间为6秒
	if(setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&TimeOut,sizeof(TimeOut))==OVFS_SOCKET_ERROR)
	{
		perror("Error:");
		killsocket(s);
		return -1;
	}

	return s;
}

OVFS_ERR ovfs_network_smtp::ConnectHost(int s, const char *hostname, int port)
{
//	LOGD("socket id :%d\n",s);
	if(hostname == NULL)
		return OVFS_ERR_NETWORK_INVALID_PARA;
	
	struct sockaddr_in remote;
	memset(&remote,0,sizeof(struct sockaddr));
	remote.sin_family = AF_INET;
	remote.sin_port   = htons(port);

	struct hostent * phostinfo = 0;
	
	phostinfo = gethostbyname(hostname);
	if (0 == phostinfo)
	{
		LOGE("get host by name error, user's hostname is [%s]\n", hostname);
		return OVFS_ERR_NETWORK_BASE;
	}
	
	memcpy(&remote.sin_addr,*phostinfo->h_addr_list, sizeof(remote.sin_addr));
	int rt = connect(s,(struct sockaddr *)&remote,sizeof(struct sockaddr));
	if(rt == OVFS_SOCKET_ERROR)
	{
		LOGE("connect error\n");
		return OVFS_ERR_NETWORK_BASE;
	}
	
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");
	
	return OVFS_SUCCESS;
}


OVFS_ERR ovfs_network_smtp::LogIn(int s, char *username,char *password)
{
	char ch[256];
	if(username == NULL || password == NULL)
		return OVFS_ERR_NETWORK_BASE;
	
	char userdes[128];
	userdes[0] = '\0';
	
	char passdes[128];
	passdes[0] = '\0';

//	LOGD("%s:%s\n", username,password);
    ovfs_utility_base64_code((unsigned char*)username, (unsigned char*)userdes);
	ovfs_utility_base64_code((unsigned char*)password, (unsigned char*)passdes);
//	LOGD("%s:%s\n", userdes,passdes);
	
	const char* send_data = "EHLO Localhost\r\n";
//	LOGD("say hello\n");
	
	OVFS_NETWORK_CHECK_SEND_RESULT(s, send_data, (int)strlen(send_data), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");

	const char* send_data_bak = "AUTH LOGIN\r\n";
	OVFS_NETWORK_CHECK_SEND_RESULT(s, send_data_bak, (int)strlen(send_data_bak), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");

	snprintf(ch, sizeof(ch),"%s\r\n",userdes);
	OVFS_NETWORK_CHECK_SEND_RESULT(s, ch, (int)strlen(ch), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");
	
	snprintf(ch, sizeof(ch),"%s\r\n",passdes);
	OVFS_NETWORK_CHECK_SEND_RESULT(s, ch, (int)strlen(ch), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");

	return OVFS_SUCCESS;
}


OVFS_ERR ovfs_network_smtp::SendMail(int s, const char *from, ovfs_email_receiver_cfg *receiver, U32 receiver_num, const char *date, const char *subject,const char *data)
{
	if(from == NULL /*|| to == NULL*/ || date == NULL || subject == NULL)
	{
		return OVFS_ERR_NETWORK_BASE;
	}

	char From[64];
	snprintf(From, sizeof(From), "MAIL FROM: <%s>\r\n", from);

	//LOGD("%s\n", From);
	OVFS_NETWORK_CHECK_SEND_RESULT(s, From, (int)strlen(From), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");


	char tmpbuf[1024] = { 0 };
	for (U32 i=0; i<receiver_num; i++)
	{
		if(strlen(receiver[i].addr) == 0)
			continue;
		snprintf(tmpbuf,sizeof(tmpbuf),"RCPT TO:<%s>\r\n", receiver[i].addr);

		OVFS_NETWORK_CHECK_SEND_RESULT(s, tmpbuf, (int)strlen(tmpbuf), 0);	
		OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");
	}
	const char* send_data = "DATA\r\n";
	OVFS_NETWORK_CHECK_SEND_RESULT(s, send_data, (int)strlen(send_data), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");
	



	tmpbuf[0] = '\0';
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "Date: %s\r\n", date);
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "From: \"%s\"<%s>\r\n", from, from);
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "X-Mailer: %s", "The Bat! (v3.02) Professional\r\n");
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "%s", "X-Priority: 3\r\n");
	
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "%s", "To: ");
	for (U32 i=0; i<receiver_num; i++)
	{
		if(i > 0)
		snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "%s", ",");
		snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "<%s>", receiver[i].addr);
		
	}
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "%s", "\r\n");
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf),  "Subject: %s\r\n", subject);
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "%s", "MIME-Version: 1.0\r\n");

	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "Content-Type: multipart/mixed;boundary=\"%s\"\r\n", OVFS_SMTP_BONDERY_TEXT);
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf),  "\r\n--%s\r\n", OVFS_SMTP_BONDERY_TEXT);
	
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "Content-Type: multipart/alternative;boundary=\"%s\"\r\n", OVFS_SMTP_BONDERY_TEXT_EX);
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf),  "\r\n--%s\r\n", OVFS_SMTP_BONDERY_TEXT_EX);
	
		
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "%s", "Content-Type: text/plain;charset=\"US-ASCII\"\r\nContent-Transfer-Encoding: 7bit\r\n\r\n");

	OVFS_NETWORK_CHECK_SEND_RESULT(s, tmpbuf, (int)strlen(tmpbuf), 0);

	{
		S8 *conten_send = (S8 *)Common_Malloc(strlen(data )+ 3, 0,__FUNCTION__,__LINE__);
		if(NULL == conten_send)
			return OVFS_ERR_NETWORK_BASE;

		snprintf(conten_send, strlen(data)+ 3, "%s\r\n", data);
		
		S32 data_size = strlen(conten_send);
		S32 Send_Len = 0;
		S32 send_times = 3;
		do{
			send_times--;
			int Ret_Len = send(s, conten_send + Send_Len, data_size - Send_Len, 0);
			if(Ret_Len == -1)
			{
				LOGE("send err ret_code = %d\n", Send_Len);
				Common_Free(conten_send,__FUNCTION__,__LINE__);
				return OVFS_ERR_NETWORK_BASE;
			}
			Send_Len += Ret_Len;
		}while(Send_Len != data_size && send_times != 0);
		
		if(Send_Len != data_size)
		{
			LOGE("send err ret_code = %d\n", Send_Len);
			Common_Free(conten_send,__FUNCTION__,__LINE__);
			return OVFS_ERR_NETWORK_BASE;
		}
		
		Common_Free(conten_send,__FUNCTION__,__LINE__);
		
	}
	
	tmpbuf[0] = '\0';
	snprintf(tmpbuf + strlen(tmpbuf), sizeof(tmpbuf) - strlen(tmpbuf), "\r\n--%s--\r\n", OVFS_SMTP_BONDERY_TEXT_EX);
	
	OVFS_NETWORK_CHECK_SEND_RESULT(s, tmpbuf, (int)strlen(tmpbuf), 0);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_smtp::SendAttachment(int s, const ovfs_soft::ovfs_email_attachment_fname *attachment, U32 attachment_num)
{
	OVFS_ERR ret = OVFS_SUCCESS;
	for (U32 i=0; i<attachment_num; i++)
	{
		if(SendAffix(s, attachment[i].fname) != OVFS_SUCCESS)
		{
			LOGE("send affix error\n");
			ret = OVFS_ERR_NETWORK_BASE;
			break;
		}
	}
	char send_buf[1024];
	snprintf(send_buf, sizeof(send_buf), "\r\n--%s--\r\n", OVFS_SMTP_BONDERY_TEXT);
	OVFS_NETWORK_CHECK_SEND_RESULT(s,send_buf,(int)strlen(send_buf),0); 
	
	return ret;
}


OVFS_ERR ovfs_network_smtp::SendAffix(int s, const char *filename )
{
	//没有用到的变量 暂时先屏蔽
	//int len = 0;
	//int length = 0;
	S8* attach_buffer = NULL;

	FILE *fp=NULL;        
	
	fp = fopen(filename,"rb");
	if(fp==NULL)
	{
		LOGE("can not open file %s\n", filename);
		return OVFS_ERR_NETWORK_BASE;
	}

	fseek(fp,0,SEEK_END);
	//length = ftell(fp);
	fseek(fp,0,SEEK_SET);

	char datasrc[64];
	OVFS_CLR_ARG(datasrc);

	char send_buf[2048];
	OVFS_CLR_ARG(send_buf);
	send_buf[0] = '\0';
	

#if 1
	const S8 *pstr0 = filename;
	const S8 *pstr1 = filename;

	do
	{
		pstr1 = strstr(pstr0, "/");
		if(pstr1 != NULL)
		{
			pstr0 = pstr1 + 1;
		}
	}while(pstr1 != NULL);
	
	snprintf(send_buf + strlen(send_buf), sizeof(send_buf) - strlen(send_buf), "--%s\r\n", OVFS_SMTP_BONDERY_TEXT);
	snprintf(send_buf + strlen(send_buf), sizeof(send_buf) - strlen(send_buf), "Content-Type: image/jpeg; name=%s\r\n", pstr0);
	snprintf(send_buf + strlen(send_buf), sizeof(send_buf) - strlen(send_buf), "%s", "Content-Transfer-Encoding: base64\r\n");
	snprintf(send_buf + strlen(send_buf), sizeof(send_buf) - strlen(send_buf), "Content-Disposition: attachment;filename=%s\r\n\r\n", pstr0);
	 
	OVFS_NETWORK_CHECK_SEND_RESULT_GOTO(s,send_buf,(int)strlen(send_buf),0); 
	{
	//没有用到的变量
	//	len = length;

		int ret_len = 0;

		attach_buffer = (S8*)Common_Malloc(OVFS_SMTP_BUFFER_SIZE, 0,__FUNCTION__,__LINE__);
		if(attach_buffer == NULL)
			goto leave_fun;

		attach_buffer[0] = '\0';

		do{
			ret_len = fread(datasrc, 1, 63, fp);
			
			if(ret_len <= 0)
				break;
			
			datasrc[ret_len] = '\0';

		//		LOGW("ret_len = %d %s\n",ret_len , (char *)datasrc);

			OVFS_CLR_ARG(send_buf);
			ovfs_utility_base64encode22((char *)send_buf,(unsigned char *)datasrc,ret_len);

			//snprintf(send_buf, sizeof(send_buf), "%s", datasrc);
			//OVFS_NETWORK_CHECK_SEND_RESULT_GOTO(s, (char *)send_buf,(int)strlen(send_buf),0);  

			snprintf(send_buf + strlen(send_buf), sizeof(send_buf) - strlen(send_buf), "%s", "\r\n");
			snprintf(attach_buffer + strlen(attach_buffer), OVFS_SMTP_BUFFER_SIZE - strlen(attach_buffer), "%s", send_buf);
			if(strlen(attach_buffer) >= OVFS_SMTP_BUFFER_SIZE/2)
			{
				OVFS_NETWORK_CHECK_SEND_RESULT_GOTO(s,attach_buffer,(int)strlen(attach_buffer),0); 
				attach_buffer[0] = '\0';
			}
			
		}while(ret_len == 63);

		if(strlen(attach_buffer) > 0)
		{
			OVFS_NETWORK_CHECK_SEND_RESULT_GOTO(s,attach_buffer,(int)strlen(attach_buffer),0); 
		}

		fclose(fp);
		fp = NULL;
	}
	
	return OVFS_SUCCESS;
	
leave_fun:
	if(fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	if(attach_buffer != NULL)
		Common_Free(attach_buffer,__FUNCTION__,__LINE__);
	LOGE("send attach fail\n");
#endif
	return OVFS_SUCCESS;
}

// í?3?ò???óê??μ?μ???ê±ê1ó?
OVFS_ERR ovfs_network_smtp::Quit(int s)
{
	const char *send_data = "QUIT\r\n";
	OVFS_NETWORK_CHECK_SEND_RESULT(s, send_data, (int)strlen(send_data), 0);
	//OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");
	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_smtp::End(int s)
{
	//const char *send_data = "--__MESSAGE__ID__54yg6f6h6y456345--\r\n";
	//OVFS_NETWORK_CHECK_SEND_RESULT(s, send_data, (int)strlen(send_data), 0);
	const char *send_data_bak = "\r\n.\r\n";
	OVFS_NETWORK_CHECK_SEND_RESULT(s, send_data_bak, (int)strlen(send_data_bak), 0);
	OVFS_NETWORK_CHECK_FUNC_RESULT(GetResponse(s), "GetResponse");
	return OVFS_SUCCESS;
}





OVFS_ERR ovfs_network_smtp::use_msmtp()
{
	int ret = 0;
	struct stat test_stat ;
	memset(&test_stat, 0, sizeof(test_stat));
	ret = stat(OVFS_APP_MSMTPPATH, &test_stat);
	if ((0 == ret) && S_ISREG(test_stat.st_mode))
	{
		
		return OVFS_SUCCESS;
	}

	LOGE("ret = %d %x\n", ret , test_stat.st_mode);
	return OVFS_ERR_NETWORK_BASE;
}

//修改MSMTP的配置文件以供mutt调用
OVFS_ERR ovfs_network_smtp::update_mail_cfg_file(ovfs_email_task_node *task_node)
{
	unsigned int i = 0;
	char sendmail[128] = {0};
	char fromaddress[256] = {0};
	snprintf(sendmail, 
		sizeof(sendmail),
		"set sendmail=\"%s -C %s\"\n", 
		OVFS_APP_MSMTPPATH, 
		task_node->is_test == OVFS_TRUE?OVFS_RC_MSMTPPATH_TEST:OVFS_RC_MSMTPPATH);
	
	snprintf(fromaddress, 
		sizeof(fromaddress),
		"set from=\"%s<%s>\"\n", 
		task_node->user_name, task_node->user_name);

	{
		FILE* pfmuttrc = fopen(task_node->is_test == OVFS_TRUE?OVFS_RC_MUTTPATH_TEST:OVFS_RC_MUTTPATH, "w");
		if (NULL == pfmuttrc)
		{
			LOGE("Sorry, create MUTT configuration file failure\n");
			return OVFS_ERR_NETWORK_BASE;
		}

		for (i=0; i<sizeof(muttrc_content)/sizeof(muttrc_content[0]); i++)
		{
			fwrite(muttrc_content[i], 1, strlen(muttrc_content[i]), pfmuttrc);
		}

		LOGW("%s,\n %s\n", sendmail, fromaddress);
		fwrite(sendmail, 1, strlen(sendmail), pfmuttrc);
		fwrite(fromaddress, 1, strlen(fromaddress), pfmuttrc);
		
		fclose(pfmuttrc);
	}

	char opt_string[2048];
	const char* prcfilename = task_node->is_test == OVFS_TRUE ? OVFS_RC_MSMTPPATH_TEST : OVFS_RC_MSMTPPATH;
	FILE* pfmsmtprc = fopen(prcfilename, "w");
	if (NULL == pfmsmtprc)
	{
		LOGE("Sorry, create MSMTP configuration file failure\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	{

		snprintf(opt_string, sizeof(opt_string), "%s", "account yourmail\n");
		if (task_node->ssl_enable == OVFS_TRUE)
		{
		
			if(task_node->server_port != 25)
			{
				snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "tls_starttls off\n");
			}
			
			snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "tls on\n");
			snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "auth on\n");
			snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "tls_certcheck off\n");
		}
		else
		{
			snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "tls off\n");
			snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "auth login\n");
		}
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "timeout 10\n");
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "host %s\n", task_node->server_addr);
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "port %d\n", task_node->server_port);
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "from %s\n", task_node->user_name);
		
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "user %s\n", task_node->user_name);
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "password %s\n", task_node->password);
		snprintf(opt_string + strlen(opt_string), sizeof(opt_string) - strlen(opt_string), "%s", "account default:yourmail\n");
		LOGD("%s\n", opt_string);
		fwrite(opt_string, 1, strlen(opt_string), pfmsmtprc);
		
		fclose(pfmsmtprc);
		chmod(prcfilename, 0x600);

	}

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_smtp::create_email_file(const char* content, unsigned int size, char* filename, unsigned int len)
{
#if 0
	ovfs_binary_file_operate *email_file = ovfs_utility_new_binary_operate(OVFS_MODULE_USER_ID_NETWORK, class_name());
	if(email_file == NULL)
		return OVFS_ERR_NETWORK_BASE;
	
	snprintf(filename, len,"/var/%p%x.dat", content, m_email_file_no++);
	
	if (email_file->open(filename, OVFS_FILE_OPEN_RW_TRUNC, OVFS_FILE_IO_DEFAULT) == OVFS_SUCCESS)
	{
		email_file->write(content, size);
		email_file->close();
		ovfs_utility_delete_binary_operate(email_file, __FILE__, __FUNCTION__);
		return OVFS_SUCCESS;
	}

	ovfs_utility_delete_binary_operate(email_file, __FILE__, __FUNCTION__);
#endif	
	binary_file_ops *email_file = new binary_file_ops();
	if(email_file == NULL)
		return OVFS_ERR_NETWORK_BASE;
	
	snprintf(filename, len,"/var/%p%x.dat", content, m_email_file_no++);
	
	if (email_file->open(filename, OVFS_FILE_OPEN_RW_TRUNC, OVFS_FILE_IO_DEFAULT) == OVFS_SUCCESS)
	{
		email_file->write(content, size);
		email_file->close();
		delete email_file;
		return OVFS_SUCCESS;
	}

	delete email_file;
	LOGE("create mail content file failure, so sorry\n");
	return OVFS_ERR_NETWORK_BASE;
}

OVFS_ERR ovfs_network_smtp::deliver_mail(char *to_address,
					   const char* mail_title,				
					   const char* mail_content,		
					   ovfs_email_attachment_fname* attach_file_name,
					   int file_num,
					   int istest)
{
	OVFS_ERR ret = OVFS_SUCCESS;
	char cmd_string[2048] = {0};
	char cmd_substring[256] = {0};
	char content_file_name[256];

	OVFS_NETWORK_CHECK_FUNC_RESULT(create_email_file(mail_content, strlen(mail_content), content_file_name, sizeof(content_file_name)), "create_email_file");
	
	snprintf(cmd_string, sizeof(cmd_string), "%s -F %s ", OVFS_APP_MUTTPATH, istest ? OVFS_RC_MUTTPATH_TEST: OVFS_RC_MUTTPATH);

	snprintf(cmd_string + strlen(cmd_string), sizeof(cmd_string) - strlen(cmd_string), "%s", to_address);
	

	LOGD("cmd_string = %s\n",cmd_string);
	for(int i = 0; i < file_num; i++)
	{
		if (attach_file_name !=NULL && 0!=strlen(attach_file_name[i].fname))
		{
			cmd_substring[0] = '\0';
			snprintf(cmd_substring, sizeof(cmd_substring), "%s", " -a");
			{
				snprintf(cmd_substring + strlen(cmd_substring), sizeof(cmd_substring) - strlen(cmd_substring), "%s", " ");
				snprintf(cmd_substring + strlen(cmd_substring), sizeof(cmd_substring) - strlen(cmd_substring), "%s", attach_file_name[i].fname);
			}
			snprintf(cmd_string + strlen(cmd_string), sizeof(cmd_string) - strlen(cmd_string), "%s", cmd_substring);
		}
	}
	
	snprintf(cmd_substring, sizeof(cmd_substring), " -s \"%s\" < %s >/var/mail_result", mail_title, content_file_name);
	snprintf(cmd_string + strlen(cmd_string), sizeof(cmd_string) - strlen(cmd_string), "%s", cmd_substring);
	
	LOGD("cmd_string = %s\n",cmd_string);
	Common_System(cmd_string);

	snprintf(cmd_string,sizeof(cmd_string), "%s", content_file_name);
	ret = check_mail_result("/var/mail_result");
	
	remove(content_file_name);
	return ret;
}

OVFS_ERR ovfs_network_smtp::check_mail_result(const S8 * filename)
{
	char szLine[1024];
	FILE* fp = fopen(filename,"r");
	if(!fp)
	{   
		LOGE("open tmp file for mail result file failure\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	memset(szLine, 0, sizeof(szLine));
	fgets(szLine, sizeof(szLine), fp);
	fclose(fp);
	
//	LOGD("read mail result content was [%s]\n", szLine);
	if (0 == strlen(szLine))
	{
		return OVFS_SUCCESS;
	}
	return OVFS_ERR_NETWORK_BASE;
}



