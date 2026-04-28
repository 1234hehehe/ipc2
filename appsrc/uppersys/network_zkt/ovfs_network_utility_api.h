/**
 * @file libcommon_api.h
 * @date create on: 2016年12月13日
 * @author eric
 * @brief 
 * 
 * @defgroup ovfs_utility_api ovfs_utility_api
 * @{
 *  @note
 *
 */
#ifndef OVFS_NETWORK_UTILITY_API_H_
#define OVFS_NETWORK_UTILITY_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>

#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
#include "ovfs_network_def.h"


using namespace ovfs_soft;

int ovfs_utility_base64_code(unsigned char *s,unsigned char *d);
int ovfs_utility_base64encode22(char * const aDest, const unsigned char * aSrc, int aLen);

OVFS_BOOL ovfs_utility_is_net_dev_exist(const char *target);
OVFS_BOOL ovfs_utility_is_net_dev_up(const char *if_name);
OVFS_BOOL ovfs_utility_is_slave_net_dev(const char *if_name);
int       ovfs_utility_is_net_dev_out(const char *if_name);
int       ovfs_utility_get_ipv6_prefixlen(const S8 *netmask);
OVFS_BOOL ovfs_utility_ipaddr_valid(ovfs_ipaddr_struct *ip);
OVFS_BOOL ovfs_utility_netmask_valid(ovfs_ipaddr_struct *ip);
OVFS_BOOL ovfs_utility_same_subnet(ovfs_ipaddr_struct *ip1, ovfs_ipaddr_struct *ip2, ovfs_ipaddr_struct *mask);
OVFS_BOOL ovfs_utility_gateway_valid(ovfs_ipaddr_struct *ip, ovfs_ipaddr_struct *gateway, ovfs_ipaddr_struct *mask);
OVFS_ERR  ovfs_utility_get_local_mac(const char *ifname, U8 *mac , U32 size);

const ovfs_if_config_struct *ovfs_utility_get_if_config(const S8* if_name);
OVFS_ERR ovfs_utility_relase_if_config(const ovfs_if_config_struct *if_config);


const ovfs_inet_ipv4_ip_info_list *ovfs_utility_get_ipv4_ipconfig(const S8* if_name);
OVFS_ERR ovfs_utility_relase_ipv4_ipconfig(const ovfs_inet_ipv4_ip_info_list *ipv4_config);


const ovfs_inet_ipv6_ip_info_list *ovfs_utility_get_ipv6_ipconfig(const S8* if_name);
OVFS_ERR ovfs_utility_relase_ipv6_ipconfig(const ovfs_inet_ipv6_ip_info_list *ipv6_config);


const ovfs_inet_ipv4_route_info_list *ovfs_utility_get_ipv4_route_config(const S8* if_name);
OVFS_ERR ovfs_utility_relase_ipv4_route_config(const ovfs_inet_ipv4_route_info_list *route_ipv4_config);


const ovfs_inet_ipv6_route_info_list *ovfs_utility_get_ipv6_route_config(const S8* if_name);
OVFS_ERR ovfs_utility_relase_ipv6_route_config(const ovfs_inet_ipv6_route_info_list *route_ipv6_config);


const ovfs_inet_ipv4_dns_info_list *ovfs_utility_get_ipv4_dns_config(const S8* if_name);
OVFS_ERR ovfs_utility_relase_ipv4_dns_config(const ovfs_inet_ipv4_dns_info_list *dns_ipv4_config);


const ovfs_inet_ipv6_dns_info_list *ovfs_utility_get_ipv6_dns_config(const S8* if_name);
OVFS_ERR ovfs_utility_relase_ipv6_dns_config(const ovfs_inet_ipv6_dns_info_list *dns_ipv6_config);

S32 ovfs_utility_linux_to_ovfs_time(time_t linux_time, ovfs_time_struct *ovfs_time);
S32 ovfs_utility_ovfs_to_linux_time(const ovfs_time_struct *ovfs_time, time_t *linux_time);
OVFS_ERR ovfs_utility_get_wlan_gateway(char *gateway);
OVFS_ERR ovfs_utility_get_wlan_addr(char *ipaddr, char *netmask);
OVFS_ERR ovfs_utility_get_wlan_dns(char *dns1, char *dns2);
OVFS_ERR ovfs_utility_get_wifi_mode(int *mode);
OVFS_ERR ovfs_utility_get_wifi_ssid(char *ssid);
int ovfs_utility_get_wifi_cardtype();

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* OVFS_UTILITY_API_H_ */
/**
 * @}
 */
