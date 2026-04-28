
/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 :实现email发送的功能
 *	源 文 件 :ovfs_network_smpt.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2012/12/23
 *****************************************************************************/


#ifndef _OVFS_NETWORK_SMTP_H_
#define _OVFS_NETWORK_SMTP_H_
//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
//#include "ovfs_comm_debug.h"
//#include "ovfs_utility_def.h"
#include "ovfs_network_base_def.h"

using ovfs_soft::OVFS_ERR;

namespace ovfs_network
{
S32 email_send_thread(Common_Thread_T hThreadHandle,void *para); //邮件发送线程

class ovfs_network_smtp
{
public:
	OVFS_ERR set_email_sender(const ovfs_soft::ovfs_email_sender_cfg *cfg); //设置邮件发送配置
	OVFS_ERR set_email_receiver(const ovfs_soft::ovfs_email_receiver_cfg *cfg, U32 receiver_num);	//设置接收者
 	OVFS_ERR send_email(const S8 *title, 
						const S8 *content, 
						const ovfs_soft::ovfs_email_attachment_fname *attachment,
					 	U32 attachment_num,
					 	OVFS_BOOL is_sync,
					 	OVFS_BOOL is_need_rm_attach); //发送邮件
 	OVFS_ERR send_email(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg,
		                const ovfs_soft::ovfs_email_receiver_cfg *recv_cfg,
		                U32 receiver_num,
		                const S8 *title, 
						const S8 *content, 
						const ovfs_soft::ovfs_email_attachment_fname *attachment,
					 	U32 attachment_num,
					 	OVFS_BOOL is_sync,
					 	OVFS_BOOL is_need_rm_attach); //发送邮件					 	
								
	OVFS_ERR send_email_tst();	//发送测试邮件，测试邮箱配置是否正常

	OVFS_ERR send_email_tst(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg
		,const ovfs_soft::ovfs_email_receiver_cfg *cfg
		, U32 receiver_num);

	OVFS_VOID send_fun();

private:
	ovfs_soft::ovfs_email_sender_cfg m_email_cfg; //邮件的发送配置
	ovfs_email_reciver_list m_recvier_list;		//邮件的接收者
	ovfs_email_task_list m_email_task_list;		//邮件任务链表

	Common_Lock_T m_task_lock;
	Common_Lock_T m_cfg_lock;
	U32 m_email_file_no;

	Common_Thread_T m_send_handle;

private:
	ovfs_email_task_node *malloc_email(
					const S8 *title, 
					const S8 *content, 
					const ovfs_soft::ovfs_email_attachment_fname *attachment,
				 	U32 attachment_num,
				 	OVFS_BOOL is_need_rm_attach);						//创建发送任务

	ovfs_email_task_node *malloc_email(
					const ovfs_soft::ovfs_email_sender_cfg *sender_cfg
					,const ovfs_soft::ovfs_email_receiver_cfg *cfg
					,U32 receiver_num
					,const S8 *title 
					,const S8 *content 
					,const ovfs_soft::ovfs_email_attachment_fname *attachment
				 	,U32 attachment_num
				 	,OVFS_BOOL is_need_rm_attach);						//创建发送任务
				 	
	OVFS_VOID free_email(ovfs_email_task_node *task_node);			//销毁发送任务
	OVFS_ERR push_email(ovfs_email_task_node *task_node);			//塞入发送任务
	ovfs_email_task_node * pop_email();							//弹出发送任务

	OVFS_ERR send_email_sync(ovfs_email_task_node *task_node);
	OVFS_ERR send_non_ssl_email(ovfs_email_task_node *task_node);	//普通方式直接发送
	OVFS_ERR send_ssl_email(ovfs_email_task_node *task_node);		//SSL方式发送
	OVFS_ERR send_email_curl(ovfs_email_task_node *task_node);		//SSL方式发送	

private:
	OVFS_VOID killsocket(int s);
	OVFS_ERR GetResponse(int s);
	int CreateSocket();
	OVFS_ERR CreateMessage(ovfs_email_task_node *node);
	OVFS_ERR ConnectHost(int s, const char *hostname, int port);
	OVFS_ERR LogIn(int s, char *username,char *password);
	OVFS_ERR SendMail(int s, const char *from, ovfs_soft::ovfs_email_receiver_cfg *receiver, U32 receiver_num, const char *date, const char *subject,const char *data);
	OVFS_ERR SendAffix(int s, const char *filename );
	OVFS_ERR Quit(int s);
	OVFS_ERR End(int s);
	OVFS_ERR use_msmtp();
	OVFS_ERR update_mail_cfg_file(ovfs_email_task_node *task_node);	
	OVFS_ERR create_email_file(const char* content, unsigned int size, char* filename, unsigned int len);
	OVFS_ERR deliver_mail(char *to_address,
					   const char* mail_title,				
					   const char* mail_content,		
					   ovfs_soft::ovfs_email_attachment_fname* attach_file_name,
					   int file_num,
					   int istest);
	OVFS_ERR check_mail_result(const S8 * filename);

	OVFS_ERR SendAttachment(int s, const ovfs_soft::ovfs_email_attachment_fname *attachment, U32 attachment_num);
	
public:
	ovfs_network_smtp();
	~ovfs_network_smtp();
	const char* class_name() {return "ovfs_network_smtp";};
	
private:
	ovfs_network_smtp(const ovfs_network_smtp &other);
	ovfs_network_smtp&operator=(const ovfs_network_smtp &other);
};

}; //namespace ovfs_network

#endif
