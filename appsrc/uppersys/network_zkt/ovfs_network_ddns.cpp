#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string>
#include "ovfs_network_ddns.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_network_upnp.h"
#include "ovfs_network_resource.h"

using namespace ovfs_soft;
using namespace ovfs_network;

static OVFS_BOOL is_custom_lib_exist(const char *lib_name)
{
	if (NULL == lib_name)
	{
		return OVFS_FALSE;
	}
	
	char custom_lib[512];
	OVFS_CLR_ARG(custom_lib);
	
	snprintf(custom_lib, sizeof(custom_lib), "%s/%s", OVFS_DDNS_LIB_CUSTOM_PATH, lib_name);
	LOGD("check custom lib %s exist ? \n", custom_lib);
	if (access(custom_lib, F_OK) == 0)
	{
		LOGD("exist\n");
		return OVFS_TRUE;
	}
	
	LOGD("not exist\n");
	return OVFS_FALSE; 
}

static S32 upgrade(Common_Thread_T hThreadHandle,void *param)
{
	OVFS_ASSERT(param != NULL);
	ovfs_network_ddns *dns = (ovfs_network_ddns *)param;
	dns->upgrade_fun();
	
	return 0;
}

OVFS_VOID ovfs_network_ddns::upgrade_fun()
{
	//每隔一段时间注册ddns的域名服务，或者被m_sleep_ops->wakeup()唤醒
	while(1)
	{
		if(m_ddns_enable != OVFS_FALSE)
		{
			ovfs_ddns_info_node *node = m_ddns_lib_info.head; 
			while(node != NULL)
			{			
				Common_Lock(m_list_lock);
				ovfs_soft::ovfs_ddns_cfg ddns_cfg = node->param; 
				Common_UnLock(m_list_lock); 
				if(OVFS_TRUE == proc_is_need_update_ddns(ddns_cfg.ddns_name))
				{
					ddns_upgrade(ddns_cfg.ddns_name, ddns_cfg.server_host, ddns_cfg.server_port, ddns_cfg.usr_name, ddns_cfg.passwd, ddns_cfg.domain_host);
					LOGD("====ddns_upgrade====\n");
				}
				node = node->next;
			}
		}
		Common_InterSleep_Sleep(m_sleep_ops,60,0);//一分钟检查一次
	}
}

ovfs_network_ddns::ovfs_network_ddns()
{	
	OVFS_CLR_ARG(m_ddns_lib_info);
	open_ddns_lib(OVFS_DDNS_LIB_CUSTOM, OVFS_DDNS_LIB_CUSTOM_PATH);
	open_ddns_lib(OVFS_DDNS_LIB_COMMON, OVFS_DDNS_LIB_PATH);
	
	m_ddns_enable = OVFS_FALSE;
	//m_last_update_time = 0;

	OVFS_CLR_ARG(m_sleep_ops);
	OVFS_CLR_ARG(m_list_lock);
	Common_InterSleep_Create(&m_sleep_ops);
	Common_Lock_Create(&m_list_lock,NULL); 
	
	OVFS_CLR_ARG(m_external_ip);
	m_extern_port = 0;
	load_cfg();
		
    m_upgrade_thr = NULL;
	Common_Thread_Create(&m_upgrade_thr,"DDNS",1024*64,
		                 COMMON_THREAD_CREATEFLAG_NORMAL,upgrade,(void *)this);
}

ovfs_network_ddns::~ovfs_network_ddns()
{
	OVFS_ASSERT(0);//暂不支持析构
}


OVFS_ERR ovfs_network_ddns::wakeup_ddns()  //唤醒DDNS线程,在
{
	return proc_wakeup_ddns();	
}


OVFS_ERR ovfs_network_ddns::proc_wakeup_ddns()  //唤醒DDNS线程,在
{
    Common_InterSleep_WakeUp(m_sleep_ops);
	
	return OVFS_SUCCESS; 
}

OVFS_ERR ovfs_network_ddns::set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *cfg)
{ 
	Common_Lock(m_list_lock);
	OVFS_ERR ret = proc_set_ddns_cfg(cfg); 	
	ovfs_cfgm_set_ddns_cfg(cfg);
	Common_UnLock(m_list_lock);
	
	return ret; 
}


OVFS_ERR ovfs_network_ddns::proc_set_ddns_cfg(const ovfs_ddns_cfg *cfg)
{	
	ovfs_ddns_info_node *node = get_ddns_func(cfg->ddns_name); 
	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE; 
	}
	node->param = *cfg; 
	node->m_last_update_time = 0;
	
	return proc_wakeup_ddns(); 
}

OVFS_ERR ovfs_network_ddns::open_ddns_lib(ovfs_soft::ovfs_ddns_lib_type type,const S8 *lib_path)
{
	if(lib_path == NULL)
	{
		LOGE("file lib_path is NULL !\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}

	DIR	*sp_dp = opendir(lib_path);
	if(NULL == sp_dp)
	{	
		LOGE("open dir %s fail \n", lib_path);
		return OVFS_ERR_NETWORK_BASE;
	}

	struct dirent *ptr;
	//逐个去读取文件夹里面的内容
	while((ptr=readdir(sp_dp))!=NULL)
	{
		if (NULL == strstr(ptr->d_name, ".so") ||  NULL != strstr(ptr->d_name, ".so."))
			continue;

		//非定制库时 查看客户定制区是否存在库 存在则不加载
		if (type == OVFS_DDNS_LIB_COMMON && is_custom_lib_exist(ptr->d_name))
		{
			continue;
		}
		
		S8 spath[512];
		OVFS_CLR_ARG(spath);
		snprintf(spath, sizeof(spath), "%s/%s",lib_path,ptr->d_name);
		LOGD("%s\n",spath);
		//打开指定的动态链接库文件
		void *pLib = dlopen(spath,RTLD_NOW|RTLD_LOCAL);
		if(NULL == pLib)
		{
			LOGE("open %s fail err:%s\n", spath, dlerror()); 
			printf("open %s fail err:%s\n", spath, dlerror());
			continue;
		}

		// 打开成功，再检查功能
		I8_DDNS_GETSUPPORTFUNCTIONS fGetSupportFunctions;
		//得到I8_DDNS_GetSupportFunctions 的函数指针
		fGetSupportFunctions = (I8_DDNS_GETSUPPORTFUNCTIONS)dlsym(pLib,"I8_DDNS_GetSupportFunctions");
		if(fGetSupportFunctions == NULL)
		{
			LOGE("%s func error %s\n", spath, dlerror());
			dlclose(pLib);
			continue;
		}
		
		I8_DDNS_FUNCTION_T ddns_func;
		OVFS_CLR_ARG(ddns_func);
		//获取协议支持的功能和函数
		if(fGetSupportFunctions(&ddns_func,sizeof(ddns_func)))
		{
			// 失败
			LOGE("GetFunctions call failed %s  \n",spath);
			dlclose(pLib);
			continue;
		}

		//判断协议获取的参数是否正确
		if(ddns_func.u32Flag[0] != I8_DDNS_FLAG0 ||
			ddns_func.u32Flag[1] != I8_DDNS_FLAG1 ||
			ddns_func.u32Size > sizeof(I8_DDNS_FUNCTION_T) ||
			ddns_func.szName[0] == 0||
			ddns_func.szServerHost[0] == 0||
			ddns_func.fpInit == NULL||
			ddns_func.fpUnInit == NULL)
		{
			LOGE("fGetSupportFunctions call failed %s  \n",spath);
			dlclose(pLib);
			continue;
		}

		//判断协议是否在链表里面是否重复
		if (is_lib_confict(&ddns_func))
		{
			LOGE("Exsit ipc proto [%s] %s\n",ddns_func.szName, spath);
			dlclose(pLib);
			continue;
		}
			
		LOGD("load DDNS %s\n",spath);
	//	if(ddns_func.fpInit() != 0)
	//	{
	//		LOGE("%s init fail\n",ddns_func.szName);
	//		dlclose(pLib);
	//		continue;
	//	}
		
		ovfs_ddns_info_node *node = (ovfs_ddns_info_node *)Common_Malloc(sizeof(ovfs_ddns_info_node),0,__FUNCTION__,__LINE__);
		if(node == NULL)
		{
			dlclose(pLib);
			LOGE("load ddns proto new failed %s\n",spath);
			continue;
		}
		
		memset(node, 0, sizeof(ovfs_ddns_info_node));
		node->ddns_func = ddns_func;
		node->enable = OVFS_TRUE;
		node->lib = pLib;
		//添加协议节点到链表
		if(OVFS_SUCCESS != add_one_node(node))
		{
			dlclose(pLib);
			Common_Free(node, __FUNCTION__, __LINE__);
			LOGE("ADD NODE FAIL %s\n",spath);
			continue;
		}
	}
	
	closedir(sp_dp);
	return OVFS_SUCCESS;
}


OVFS_BOOL ovfs_network_ddns::is_lib_confict(I8_DDNS_FUNCTION_T *ddns_func)
{
	if(ddns_func == NULL)
		return OVFS_FALSE;
	
	ovfs_ddns_info_node *node = m_ddns_lib_info.head;
	while(node != NULL)
	{
		if(strcmp(node->ddns_func.szName , ddns_func->szName) == 0)
			return OVFS_TRUE;
		
		node = node->next;
	}
	
	return OVFS_FALSE;
}

OVFS_ERR ovfs_network_ddns::add_one_node(ovfs_ddns_info_node *node)
{
	if(NULL == node )
		return OVFS_ERR_NETWORK_BASE;

	LOGW("load ddns lib <%d> -> %s\n", node->ddns_func.u32Index, node->ddns_func.szName);
	if(m_ddns_lib_info.head == NULL)
	{
		m_ddns_lib_info.head = node;
		m_ddns_lib_info.tail = node;
	}
	else
	{
		node->prev = m_ddns_lib_info.tail;
		m_ddns_lib_info.tail->next = node;
		m_ddns_lib_info.tail = node;
	}
	m_ddns_lib_info.node_num++;

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_ddns::del_one_node(ovfs_ddns_info_node *node)
{
	if(NULL == node )
		return OVFS_ERR_NETWORK_BASE;

	
	ovfs_ddns_info_node *prev = node->prev;
	ovfs_ddns_info_node *next = node->next;

	if(prev != NULL)
	{
		prev->next = next;
	}
	else
	{
		m_ddns_lib_info.head = next;
	}

	if(next != NULL)
	{
	  next->prev = prev;
	}
	else
	{
		m_ddns_lib_info.tail = prev;
	}
	if(node->lib != NULL)
		dlclose(node->lib);
	
	Common_Free(node, __FUNCTION__, __LINE__);

	m_ddns_lib_info.node_num--;

	return OVFS_SUCCESS;
}

ovfs_ddns_info_node *ovfs_network_ddns::get_ddns_func(const S8 *ddns_name)
{
	if(NULL == ddns_name)
		return NULL;
	
	ovfs_ddns_info_node *node = m_ddns_lib_info.head;
	while(node != NULL)
	{
		if(strcmp(node->ddns_func.szName , ddns_name) == 0
			&& node->enable == OVFS_TRUE)
			return node;
		
		node = node->next;
	}

	return NULL;
}

ovfs_ddns_info_node *ovfs_network_ddns::get_ddns_node(const S8 *ddns_name)
{
	if(NULL == ddns_name)
		return NULL;
	
	ovfs_ddns_info_node *node = m_ddns_lib_info.head;
	while(node != NULL)
	{
		if(strcmp(node->ddns_func.szName , ddns_name) == 0)
			return node;
		
		node = node->next;
	}

	return NULL;
}

OVFS_ERR ovfs_network_ddns::set_ddns_param(S8 *ddns_name, U16 ctrl_port, U16 media_port, U16 http_port)
{
	ovfs_ddns_info_node *node = get_ddns_func(ddns_name);
		
	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE;
	}

	if(node->ddns_func.fpSetParam == NULL)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	
	return node->ddns_func.fpSetParam(ctrl_port, media_port, http_port) == 0? OVFS_SUCCESS:OVFS_ERR_NETWORK_BASE;
}

OVFS_ERR ovfs_network_ddns::ddns_upgrade(S8 *ddns_name, S8 *server_host, U16 server_port, S8 *usr_name, S8 *passwd, S8 *domain_host)
{	
	ovfs_ddns_info_node *node = get_ddns_func(ddns_name); 
	
	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	
	if(node->ddns_func.fpUpgrade == NULL)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	if(strlen(domain_host) == 0
		|| server_port == 0
		|| strlen(server_host) == 0)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	return node->ddns_func.fpUpgrade(server_host, server_port, usr_name, passwd, domain_host) == 0? OVFS_SUCCESS:OVFS_ERR_NETWORK_BASE;
}

OVFS_ERR ovfs_network_ddns::get_inter_net_ip(S8 *ddns_name, S8 *usr_name,S8 *passwd)
{
	ovfs_ddns_info_node *node = get_ddns_func(ddns_name);
	
	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE;
	}

	if(node->ddns_func.fpGetExternalIp == NULL)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	
	return node->ddns_func.fpGetExternalIp(usr_name, passwd) == 0? OVFS_SUCCESS:OVFS_ERR_NETWORK_BASE;
	
}

OVFS_ERR ovfs_network_ddns::get_default_inter_ip(S8 *ddns_name, S8 *usr_name,S8 *passwd)
{
	ovfs_ddns_info_node *node = get_ddns_func(ddns_name);
		
	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE;
	}

	if(node->ddns_func.fpGetDefaultUser == NULL)
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	
	return node->ddns_func.fpGetDefaultUser(usr_name, passwd) == 0? OVFS_SUCCESS:OVFS_ERR_NETWORK_BASE;
	
}

OVFS_ERR ovfs_network_ddns::get_ddns_cap(ovfs_soft::ovfs_ddns_ability *ddns_ability)
{
	return proc_get_ddns_cap(ddns_ability);
}

OVFS_ERR ovfs_network_ddns::proc_get_ddns_cap(ovfs_soft::ovfs_ddns_ability *ddns_ability)
{
	if(NULL == ddns_ability)
		return OVFS_ERR_NETWORK_INVALID_PARA;
	
	memset(ddns_ability, 0, sizeof(ovfs_ddns_ability));
	
	ovfs_ddns_info_node *node = m_ddns_lib_info.head;
	
	while(node != NULL && ddns_ability->node_num < OVFS_ARRAY_DIM(ddns_ability->ddns_cap))
	{
		ddns_ability->ddns_cap[ddns_ability->node_num].enable = node->enable;
		ddns_ability->ddns_cap[ddns_ability->node_num].version = node->ddns_func.u32Version;
		ddns_ability->ddns_cap[ddns_ability->node_num].ddns_port = node->ddns_func.u32ServerPort;
		snprintf(ddns_ability->ddns_cap[ddns_ability->node_num].ddns_name, 
			sizeof(ddns_ability->ddns_cap[ddns_ability->node_num].ddns_name),
			"%s",
			node->ddns_func.szName);

		snprintf(ddns_ability->ddns_cap[ddns_ability->node_num].ddns_host, 
			sizeof(ddns_ability->ddns_cap[ddns_ability->node_num].ddns_host),
			"%s",
			node->ddns_func.szServerHost);
		
		ddns_ability->node_num++;
		node = node->next;
	}
	
	return OVFS_SUCCESS; 
}

OVFS_ERR ovfs_network_ddns::set_ddns_enable(const S8 *ddns_name, OVFS_BOOL enable)
{
	ovfs_ddns_info_node *node = get_ddns_node(ddns_name);
	if(NULL == node)
	{
		return OVFS_ERR_NETWORK_BASE; 
	}
	
	Common_Lock(m_list_lock); 
	
	node->enable = enable; 
	node->m_last_update_time = 0;
	
	ovfs_cfgm_set_ddns_enable(ddns_name,enable);
	
	Common_UnLock(m_list_lock); 
	
	proc_wakeup_ddns();
	
	return OVFS_SUCCESS;
}


OVFS_ERR ovfs_network_ddns::start_ddns(OVFS_BOOL enable)
{
	m_ddns_enable = enable;
	
	proc_wakeup_ddns();

	ovfs_cfgm_set_ddns_start(enable);
	
	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_ddns::load_cfg()
{	
	ovfs_ddns_ability ddns_ability; 
	OVFS_CLR_ARG(ddns_ability); 
	if(proc_get_ddns_cap(&ddns_ability)!= OVFS_SUCCESS)
	{
		return OVFS_ERR_NETWORK_BASE; 
	}

	ovfs_cfgm_set_ddns_cap(&ddns_ability);
	
	//配置到链表中
	for(U32 i=0; i<ddns_ability.node_num && i < OVFS_ARRAY_DIM(ddns_ability.ddns_cap); i++)
	{	
		ovfs_ddns_cfg ddns_cfg; 
		OVFS_CLR_ARG(ddns_cfg); 
		if(ovfs_cfgm_get_ddns_cfg(ddns_ability.ddns_cap[i].ddns_name, &ddns_cfg) == OVFS_SUCCESS)
		{
			proc_set_ddns_cfg(&ddns_cfg); 
		}
	}

	ovfs_cfgm_get_ddns_start(&m_ddns_enable); 
	
	//下面这段代码暂时没有用，没有针对单个的DDNS服务器的启用或禁用操作.
	ovfs_ddns_info_node *node = m_ddns_lib_info.head; 
	while(node != NULL)
	{
		OVFS_BOOL enable = OVFS_TRUE;
		if(ovfs_cfgm_get_ddns_enable(node->ddns_func.szName, &enable) == OVFS_SUCCESS
			&& enable == OVFS_FALSE)
		{
			enable = OVFS_FALSE;
		}
		
		node->enable = enable; 
		node = node->next;
	}
	
	return OVFS_SUCCESS; 
}



OVFS_BOOL ovfs_network_ddns::proc_is_need_update_ddns(const S8 *ddns_name)
{
	//OVFS_BOOL  mapping_success;
	//U16 internal_port;
	U16 extern_port_tmp = 0;
	//ovfs_upnp_port_protocol port_protocol;
	S8 external_ip_tmp[64]={0};
	ovfs_network_upnp *upnp = get_res_manage()->get_upnp();
	OVFS_ERR ret = OVFS_ERR_NETWORK_BASE;

	time_t time_now = 0;
	time_now = time(NULL);

	S32 update_min = 720;
	ovfs_cfgm_get_ddns_update_interval(ddns_name, &update_min);

	Common_Lock(m_list_lock); 
	
	ovfs_ddns_info_node *node = get_ddns_func(ddns_name);
	if(NULL == node)
	{
		Common_UnLock(m_list_lock); 
		return OVFS_FALSE; 
	}
	
	if( time_now - node->m_last_update_time >= update_min * 60 )
	{
		node->m_last_update_time = time_now;
		Common_UnLock(m_list_lock); 
		return OVFS_TRUE;
	}
	
	Common_UnLock(m_list_lock); 
	
	if(NULL != upnp)
	{
		//ret = upnp->get_port_info(OVFS_PORT_HTTP,&mapping_success,&internal_port,&extern_port_tmp,&port_protocol,external_ip_tmp,sizeof(external_ip_tmp));
	}

	if(OVFS_SUCCESS == ret )
	{
		if(0 != strcmp(m_external_ip,external_ip_tmp) ||  extern_port_tmp!=m_extern_port)
		{
			snprintf(m_external_ip,sizeof(m_external_ip),"%s",external_ip_tmp);
			m_extern_port = extern_port_tmp;
			return OVFS_TRUE;
		}
	}
	
	return OVFS_FALSE;
	
}

