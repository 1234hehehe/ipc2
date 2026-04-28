#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>       			/* for ifconf */  
#include <linux/sockios.h>   	 	/* for net status mask */  
#include <netinet/in.h>       		/* for sockaddr_in */  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <sys/ioctl.h> 


//#include "ovfs_comm_debug.h"
#include "ovfs_comm_errno.h"
#include "libcommon_api.h"
#include "ovfs_network_resource.h"



namespace ovfs_network{
static ovfs_network_res_manage *s_resource_manage = NULL;
static pthread_mutex_t	s_resource_lock = PTHREAD_MUTEX_INITIALIZER;
}

using namespace ovfs_soft;
using namespace ovfs_network;


ovfs_network_res_manage* ovfs_network::get_res_manage()
{
	if (s_resource_manage == NULL)
	{
		pthread_mutex_lock(&s_resource_lock);
		if (s_resource_manage == NULL)
		{
			s_resource_manage = new(ovfs_network_res_manage);
			OVFS_ASSERT(s_resource_manage != NULL);
		}
		pthread_mutex_unlock(&s_resource_lock);
	}
	return s_resource_manage;
}

//---------------------------------------------- ovfs_network_res_manage --------------------------------------
ovfs_network_res_manage::ovfs_network_res_manage()
{
	m_eth_cfg_mgr = NULL;
	m_smtp = NULL;
	m_ddns = NULL;
    m_pppoe = NULL;
    m_upnp = NULL;	//如果不初始化，调用get_.... 就会得到一个错误的值。
}

ovfs_network_res_manage::~ovfs_network_res_manage()
{
	//暂时不支持析构
	OVFS_ASSERT(0);
}

//只允许初始化函数调用一次
OVFS_VOID ovfs_network_res_manage::construct_res()
{
	
	m_eth_cfg_mgr = new ovfs_network_config_mgr();
	OVFS_ASSERT(NULL != m_eth_cfg_mgr);

	m_smtp = new ovfs_network_smtp();
	OVFS_ASSERT(NULL != m_smtp);

	m_ddns = new ovfs_network_ddns();
	OVFS_ASSERT(NULL != m_ddns);

	m_pppoe = new ovfs_network_pppoe();
	OVFS_ASSERT(NULL != m_pppoe);

	m_upnp = new ovfs_network_upnp();
	OVFS_ASSERT(NULL != m_upnp);
	
	return;
}


ovfs_network_config_mgr *ovfs_network_res_manage::get_eth_cfg_mgr()
{
	return m_eth_cfg_mgr;
}

ovfs_network_smtp *ovfs_network_res_manage::get_smtp()
{
	return m_smtp;
}

ovfs_network_ddns *ovfs_network_res_manage::get_ddns()
{
	return m_ddns;
}

ovfs_network_pppoe *ovfs_network_res_manage::get_pppoe()
{
	return m_pppoe;
}

ovfs_network_upnp *ovfs_network_res_manage::get_upnp()
{
	return m_upnp;
}
