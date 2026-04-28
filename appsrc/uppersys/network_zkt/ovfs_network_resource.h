#ifndef _OVFS_NETWORK_RESOURCE_H_
#define _OVFS_NETWORK_RESOURCE_H_

//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_network_def.h"
#include "ovfs_network_mgr.h"
#include "ovfs_network_smtp.h"
#include "ovfs_network_ddns.h"
#include "ovfs_network_pppoe.h"
#include "ovfs_network_upnp.h"

//管理utilitiy模块使用的所有对象资源
namespace ovfs_network
{

class ovfs_network_res_manage
{
public:
	OVFS_VOID construct_res();	//只允许初始化函数调用一次
	ovfs_network_config_mgr *get_eth_cfg_mgr();
	ovfs_network_smtp *get_smtp();
	ovfs_network_ddns *get_ddns();
        ovfs_network_pppoe *get_pppoe();
        ovfs_network_upnp *get_upnp();

private:


private:
	ovfs_network_config_mgr *m_eth_cfg_mgr;
	ovfs_network_smtp *m_smtp;
	ovfs_network_ddns *m_ddns;
        ovfs_network_pppoe *m_pppoe;
        ovfs_network_upnp *m_upnp;
	
public:
	ovfs_network_res_manage();
	~ovfs_network_res_manage();
	const char* class_name() {return "ovfs_network_res_manage";};
private:
	ovfs_network_res_manage(const ovfs_network_res_manage &other);
	ovfs_network_res_manage &operator=(const ovfs_network_res_manage &other);
};

ovfs_network_res_manage* get_res_manage();

} //namespace ovfs_network{
#endif //#ifndef _OVFS_NETWORK_RESOURCE_H_
