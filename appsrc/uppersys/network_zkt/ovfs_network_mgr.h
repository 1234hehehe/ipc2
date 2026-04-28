#ifndef OVFS_NETWORK_MGR_H_
#define OVFS_NETWORK_MGR_H_

#include <sys/types.h>
#include <ifaddrs.h>
#include "ovfs_network_base_def.h"
#include "ovfs_network_rest_common.h"


using namespace ovfs_soft;
namespace ovfs_network
{

typedef struct
{
    int state;
    int isNetChange;
    int event[4]; // 0 netOut 1 netConflict
} NETWORK_MGR_CONTEXT_T;

//网络带宽统计结构体
typedef struct
{
	U64 m_rx_speed;
	U64 m_tx_speed;
	U64 m_last_rx_size;
	U64 m_last_tx_size;
	struct timeval m_last_cnt_time;
}network_speed_Info;
class ovfs_network_config_mgr
{
private:
	ovfs_network_config_mgr (const ovfs_network_config_mgr &other);
	ovfs_network_config_mgr&operator=(const ovfs_network_config_mgr &other);
	//获取指定网卡的网络流量，由于存在多个地方会使用这个功能。因此封装到线程里面调用.结果存入到m_netcard_speedInfo中
	OVFS_ERR get_special_netcard_traffic(const S8* if_name);

	OVFS_ERR push_net_setting_task(network_setting_task_list *list, network_setting_task_node *node);
	OVFS_BOOL is_task_confict(network_setting_task_node *node, network_setting_task_node *new_node);
	OVFS_ERR add_new_task(network_setting_task_list *list, network_setting_task_node *new_node);
	OVFS_ERR pop_net_setting_task(network_setting_task_list *list, network_setting_task_node *node);
	OVFS_ERR debug_connect();
	network_setting_task_node* get_one_net_setting_task();
	OVFS_VOID release_one_net_setting_task(network_setting_task_node *node_busy, OVFS_BOOL is_success);
	network_setting_task_node *malloc_net_set_task(network_task_type task_type, const S8 * para, U32 size);

	OVFS_ERR issue_setting_task(network_setting_task_node *node);
    OVFS_ERR set_dns_auto();
	OVFS_ERR reset_netcard(const ovfs_soft::ovfs_netcard_config *netCfg);
    OVFS_ERR update_netcard_cfg(const ovfs_soft::ovfs_netcard_config *netCfg);
    OVFS_ERR update_dns_cfg(const ovfs_soft::ovfs_local_net_dns *dns);
    OVFS_ERR update_new_dns(const ovfs_soft::ovfs_local_net_dns *dns);
    OVFS_ERR network_check_udhcpc_pid(const S8 *udhcpc_pid_file);
	OVFS_ERR check_dhcpc_result(const S8 *if_name);
    OVFS_ERR check_dhcpc6_result(const S8 *if_name);
    OVFS_ERR network_get_ip_bydhcp(const S8 *if_name, ovfs_ipaddr_struct *ip, ovfs_ipaddr_struct *netmask, ovfs_ipaddr_struct *gateway);
    OVFS_ERR network_get_ipv6_bydhcp6(const S8 *if_name, ovfs_ipaddr_struct *ip, int *prefixlen, ovfs_ipaddr_struct *gateway);
    OVFS_ERR network_get_dns_bydhcp(S8 *if_name, ovfs_local_net_dns *net_dns);
public:
	ovfs_network_config_mgr ();
	~ovfs_network_config_mgr ();
	OVFS_VOID init_net_config();
    OVFS_VOID sync_network();
    //??è?oíéè??í??¨2?êy????
    OVFS_ERR wifi_4g_route_monitor(int nAliwifiSdkInit, int n4GInited);
	OVFS_VOID check_dhcp_status();
	OVFS_VOID check_4G_status();
	OVFS_VOID check_wifi_status();
	OVFS_ERR get_local_netcard_cfg(const S8* if_name, ovfs_soft::ovfs_netcard_config *eth);
	OVFS_ERR set_local_netcard_cfg(const ovfs_soft::ovfs_netcard_config *eth);
	OVFS_ERR update_netcard_dhcp_cfg(ovfs_soft::ovfs_netcard_config *eth);

	//??è?oíéè??DNS????
	OVFS_ERR get_dns_cfg(ovfs_soft::ovfs_local_net_dns *dns);
	OVFS_ERR set_dns_cfg(const ovfs_soft::ovfs_local_net_dns *dns);
	OVFS_ERR set_dns_cfgv2(const ovfs_soft::ovfs_local_net_dns *dns);
	//??è?oíéè????è?í??¨
	OVFS_ERR get_default_if(S8 * if_name, U32 size);
	OVFS_ERR set_default_if(const  S8* if_index);
	OVFS_ERR get_default_router( S8* if_name, U32 size);
    OVFS_BOOL is_if_can_be_router(const S8* if_name);

    OVFS_ERR get_special_netcard_traffic(const S8* if_name, U32 *upload, U32 *download);

public:

private:
	U32 m_eth_num;
	ovfs_soft::ovfs_netcard_config m_eth_cfg[OVFS_MAX_ETH_CARD_NUM];
	ovfs_soft::ovfs_local_net_dns m_dns_cfg;
	const ovfs_soft::ovfs_network_capability *m_network_ability;
	network_setting_task_list m_setting_task_list;

	Common_Thread_T m_pSetCFGthread;//网卡设置任务线程
	Common_Thread_T m_pCheckDhcpthread;//网卡设置任务线程
	Common_Thread_T m_pMonitorthread;//网络变化监测线程
	Common_Thread_T m_pWiFithread;//WiFi状态监测线程
	Common_Thread_T m_p4Gthread;//4G状态监测线程

	Common_Lock_T m_lock_netspeed;  //针对网络速度的互斥锁

	//获取整体的网络速度
	network_speed_Info m_net_speedInfo;

	//获取指定网卡的网络速度
	network_speed_Info  m_netcard_speedInfo[OVFS_MAX_ETH_CARD_NUM];
};
}

#endif /* OVFS_NETWORK_MGR_H_ */

