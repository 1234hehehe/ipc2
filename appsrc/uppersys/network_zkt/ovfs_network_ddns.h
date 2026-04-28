
/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 :实现email发送的功能
 *	源 文 件 :ovfs_network_ddns.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2012/12/23
 *****************************************************************************/


#ifndef _OVFS_NETWORK_DDNS_H_
#define _OVFS_NETWORK_DDNS_H_
//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
//#include "ovfs_comm_debug.h"
//#include "ovfs_utility_def.h"
#include "libcommon_api.h"
#include "ovfs_network_base_def.h"
#include "ovfs_network_def.h"
#include "i8_ddns_api.h"

using ovfs_soft::OVFS_ERR;
using ovfs_soft::ovfs_ddns_lib_type;

namespace ovfs_network
{
//S32 email_send_thread(Common_Thread_T hThreadHandle,void *para); //邮件发送线程

class ovfs_network_ddns
{
public:
	OVFS_ERR get_ddns_cap(ovfs_soft::ovfs_ddns_ability *ddns_ability);
	OVFS_ERR set_ddns_enable(const S8 *ddns_name, OVFS_BOOL enable);
	OVFS_VOID upgrade_fun();
	OVFS_ERR set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *cfg);
	OVFS_ERR start_ddns(OVFS_BOOL enable);
	OVFS_ERR wakeup_ddns();  //唤醒DDNS线程
	
private:
	OVFS_ERR set_ddns_param(S8 *ddns_name, U16 ctrl_port, U16 media_port, U16 http_port);
	OVFS_ERR ddns_upgrade(S8 *ddns_name, S8 *server_host, U16 server_port, S8 *usr_name, S8 *passwd, S8 *domain_host);
	OVFS_ERR get_inter_net_ip(S8 *ddns_name, S8 *usr_name,S8 *passwd); 
	OVFS_ERR get_default_inter_ip(S8 *ddns_name, S8 *usr_name,S8 *passwd); 
	OVFS_ERR proc_set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *cfg);
	OVFS_ERR proc_get_ddns_cap(ovfs_soft::ovfs_ddns_ability *ddns_ability);
	OVFS_ERR proc_wakeup_ddns(); 
	
	
private:
	ovfs_ddns_info_list m_ddns_lib_info;
	OVFS_BOOL m_ddns_enable;
	Common_Thread_T m_upgrade_thr;
	Common_InterSleep_T m_sleep_ops;
	Common_Lock_T m_list_lock;
	S8 m_external_ip[64];
	U16  m_extern_port;
	//time_t m_last_update_time;
	
private:
	OVFS_ERR open_ddns_lib(ovfs_soft::ovfs_ddns_lib_type type,const S8 *lib_path);
	OVFS_BOOL is_lib_confict(I8_DDNS_FUNCTION_T *ddns_func);
	OVFS_ERR del_one_node(ovfs_ddns_info_node *node);
	OVFS_ERR add_one_node(ovfs_ddns_info_node *node);
	ovfs_ddns_info_node *get_ddns_func(const S8 *ddns_name) ;
	ovfs_ddns_info_node *get_ddns_node(const S8 *ddns_name) ;
	OVFS_ERR load_cfg();
	OVFS_BOOL proc_is_need_update_ddns(const S8 *ddns_name);
	
public:
	ovfs_network_ddns();
	~ovfs_network_ddns();
	const char* class_name() {return "ovfs_network_ddns";};
	
private:
	ovfs_network_ddns(const ovfs_network_ddns &other);
	ovfs_network_ddns&operator=(const ovfs_network_ddns &other);
};

}; //namespace ovfs_network

#endif

