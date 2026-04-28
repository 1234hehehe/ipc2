#ifndef _OVFS_NETWORK_PPPOE_H_
#define _OVFS_NETWORK_PPPOE_H_

//PPPOE Connect Config File,Include Some Important Options, Such as:ETH='ethn', USER='xxxxxx', DNSTYPE=SERVER Or DNSTYPE=NOCHANGE
//If DNSTYPE=SERVER We Will Get DNS Server Address From ISP && The DNS Server Address Store In /etc/ppp/resolv.conf, if DNSTYPE=NOCHAGE We Won't Change Current DNS Server Address 
#define OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH	"/root/pppoe/pppoe.conf"

//PAP File Verification UserName && Password
#define OVFS_PPPOE_PAP_FILE_ABSOLUTE_PATH		"/root/pppoe/pap-secrets"

//Chap File Verification UserName && Password
#define OVFS_PPPOE_CHAP_FILE_ABSOLUTE_PATH		"/root/pppoe/chap-secrets"

//DNSTYPE=SERVER && This File Will Be Store.
#define OVFS_PPPOE_DDNS_ADDRESS_FILE_ABSOLUTE_PATH	"/root/pppoe/resolv.conf"

//When Someone Re-config pppoe param, The Above File Will Change, So We Save a copy of each document.
#define OVFS_PPPOE_FILE_BAK_SUFFIX				"-bak"

//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
#include "ovfs_comm_def.h"
#include "ovfs_network_def.h"
//#include "ovfs_comm_sys.h"
//#include "ovfs_utility_def.h"

using ovfs_soft::OVFS_ERR;
using ovfs_soft::ovfs_pppoe_config;
using ovfs_soft::ovfs_pppoe_status;
//using ovfs_soft::ovfs_rw_lock;

namespace ovfs_network
{

class ovfs_network_pppoe
{
public:
        //At Current Version, We Just Support Use Default Normal Network Card For PPPOE Connect;
        OVFS_ERR set_config(const ovfs_pppoe_config *pppoe_config);

        //At Current Version, We Just Get Default Normal Network Card PPPOE Connect Status.
	OVFS_ERR get_status(ovfs_pppoe_status *pppoe_status);

public:
	OVFS_VOID refresh();

private:
        OVFS_ERR load_pppoe_cfg();
        
    	OVFS_ERR connect();        
	OVFS_ERR	 disconnect();

        OVFS_ERR refresh_config();
		OVFS_ERR create_config_file();
        OVFS_ERR refresh_config_file();
        OVFS_ERR refresh_secrets_file(const S8 *absolute_file_name);
        OVFS_ERR refresh_pppoe_status();

private:
        S8 m_eth_name[OVFS_MAX_ETH_IFNAME_SIZE];
        OVFS_BOOL m_config_changed;
        ovfs_pppoe_config m_pppoe_config;
        ovfs_pppoe_status m_pppoe_status;


        Common_RWLock_T m_config_rw_lock;
        Common_RWLock_T m_status_rw_lock;
        Common_Thread_T m_thread_handle;
	
public:
	ovfs_network_pppoe();
	~ovfs_network_pppoe();
	const char* class_name() {return "ovfs_network_pppoe";};
	
private:
	ovfs_network_pppoe(const ovfs_network_pppoe &other);
	ovfs_network_pppoe&operator=(const ovfs_network_pppoe &other);
};//END CLASS ovfs_network_pppoe

}//END NAMESPACE ovfs_network

#endif // END #ifndef _OVFS_NETWORK_PPPOE_H_