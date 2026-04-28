#ifndef OVFS_COMM_TOOL_H_
#define OVFS_COMM_TOOL_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>

#include <libcommon_api.h>

#include <net/route.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>

#include <netdb.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
//#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>

#include <netinet/ip_icmp.h> //struct icmp
#include <netinet/in.h> //sockaddr_in
#include <netinet/ip.h>
#include <netinet/ether.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#define UP    1
#define DOWN    0

struct route_info{
int dstAddr;
int srcAddr;
int gateWay;
char ifName[16];
};

/* 根据 RFC 0826 修改*/
typedef struct _Ether_pkg Ether_pkg;
struct _Ether_pkg
{
    /* 前面是ethernet头 */
    unsigned char ether_dhost[6]; /* 目地硬件地址 */
    unsigned char ether_shost[6]; /* 源硬件地址 */
    unsigned short int ether_type; /* 网络类型 */

    /* 下面是arp协议 */
    unsigned short int ar_hrd; /* 硬件地址格式 */
    unsigned short int ar_pro; /* 协议地址格式 */
    unsigned char ar_hln; /* 硬件地址长度(字节) */
    unsigned char ar_pln; /* 协议地址长度(字节) */
    unsigned short int ar_op; /* 操作代码 */
    unsigned char arp_sha[6]; /* 源硬件地址 */
    unsigned char arp_spa[4]; /* 源协议地址 */
    unsigned char arp_tha[6]; /* 目地硬件地址 */
    unsigned char arp_tpa[4]; /* 目地协议地址 */
};

struct in6_ifreq
{
	struct in6_addr ifr6_addr;
	uint32_t ifr6_prefixlen;
	unsigned int ifr6_ifindex;
};

#ifdef __cplusplus
extern "C"
{
#endif
void pHx(unsigned char *p,int len);

int NetWorkTool_GetIFFlags( char *ifname );
int NetWorkTool_GetGateway(char *devName,char *gateway);
int NetWorkTool_GetMacAddr(char *ifname, char *mac);
int NetWorkTool_SetMacAddr(char *ifname, char *mac);
int NetWorkTool_SetMac(char *if_name, char *mac);
int NetWorkTool_GetIPAddr(char *ifname, char *ipv4);
int NetWorkTool_SetIPAddr(char *ifname, char *ipv4);
int NetWorkTool_GetMaskAddr(char *ifname, char *netmask);
int NetWorkTool_SetMaskAddr(char *ifname, char *netmask);
int NetWorkTool_MacIP(const char *device,char *mac,char *ip);
int NetWorkTool_IfUpDown(char *ifname, int flag/*1 up ; 0 down*/);
int NetWorkTool_Get_Dhcpc_Result(const char *if_name,char *ip,char *netmask,char *gateway);
int NetWorkTool_Get_Dhcpc6_Result(const char *if_name,char *ip,int *prefixlen,char *gateway);
int NetWorkTool_SetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask);
int NetWorkTool_GetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask);
int NetWorkTool_SetDevInfoV6(char *ifname, char *ipv6, int prefixlen);
int NetWorkTool_GetDevInfoV6(char *ifname, char *ipv6, int *prefixlen);
int NetWorkTool_GetDefaultRoute(char *gateWay);
int NetWorkTool_GetDefaultRouteV2(char *ethName, char *gateWay);
int NetWorkTool_AddDefaultRoute(char *ethName, char *gateWay);
int NetWorkTool_DelDefaultRoute(char *ethName, char *gateWay);
int NetWorkTool_UpdateDefaultRoute(char *ifname);
int NetWorkTool_GetDefaultRouteV6(char *ethName, char *gateWay);
int NetWorkTool_SetDefaultRouteV6(char *ethName, char *gateWay);
int NetWorkTool_GetIpaddrByMacFromUdhcpdLease(char *leaseFile, char *mac, char *ip);
int NetWorkTool_GetIfAddr(char *ifname, char *ip, char *netmask,int *family);
int NetWorkTool_GetSubNet(char *subnet, char *ipv4, char *netmask);
bool NetWorkTool_GetPublicIP(char *url);
int NetWorkTool_CheckConnect(char *if_name,char *ip);


#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif
