/*
 * ovfs_utility_api.c
 *
 *  Created on: 2016年12月13日
 *      Author: eric
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <ifaddrs.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "ovfs_comm_def.h"
//#include "ovfs_comm_debug.h"

#include "ovfs_network_utility_api.h"

using namespace ovfs_soft;


static OVFS_BOOL is_net_dev_exist(const char *target)
{
	if(target == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}
	
	FILE *fh;
	char buf[512];

	if(target == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}

	fh = fopen("/proc/net/dev", "r");
	if(fh == NULL)
	{
		LOGE("open /proc/ned/dev fail\n");
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);
	
	int err = -1;
	while (fgets(buf, sizeof buf, fh)) 
	{
		if(NULL != strstr(buf, target))
		{
			err = 0;
			break;
		}
	}
	fclose(fh);
	return err == 0?OVFS_TRUE:OVFS_FALSE;
}

static OVFS_BOOL is_net_dev_up(const char *if_name)
{
	if(if_name == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}
	
	struct ifreq ifr;
	int skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}
	//printf("if_name = %s\n", if_name);
	memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
	int ret = ioctl(skfd, SIOCGIFFLAGS, &ifr);
	close(skfd);
	if (ret < 0)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}

	if(ifr.ifr_flags & IFF_UP)
	{
		return OVFS_TRUE;
	}
	else
	{
		return OVFS_FALSE;
	}
}

static OVFS_BOOL is_slave_net_dev(const char *if_name)
{
	if(if_name == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}
	
	struct ifreq ifr;
	int skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}
	//printf("if_name = %s\n", if_name);
	memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
	int ret = ioctl(skfd, SIOCGIFFLAGS, &ifr);
	close(skfd);
	if (ret < 0)
	{
		OVFS_ASSERT(0);
		return OVFS_FALSE;
	}

	if(ifr.ifr_flags & IFF_SLAVE)
	{
		return OVFS_TRUE;
	}
	else
	{
		return OVFS_FALSE;
	}
}

static int is_net_dev_out(const char *if_name)
{
    int m_net_out = 0;
	
	if(if_name == NULL)
	{
		OVFS_ASSERT(0);
		return 0;
	}
	
	struct ifreq ifr;
	int skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
	{
		OVFS_ASSERT(0);
		return 0;
	}
	
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
	//printf("[%d]check ....SIOCGIFFLAGS\n",phy_idx);
	// SIOCGIFFLAGS
	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) == 0)
	{
		if (ifr.ifr_ifru.ifru_flags & IFF_RUNNING)
		{
			m_net_out = 0;
		}
		else
		{
			m_net_out = 1;
		}
	}
	else 
	{// SIOCETHTOOL
		struct ethtool_value edata;
		memset(&ifr, 0, sizeof(ifr));
		memset(&edata, 0,sizeof(edata));
		edata.cmd = ETHTOOL_GLINK;
		//printf("[%d]check ....SIOCETHTOOL\n",i);

		snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
		ifr.ifr_data = (char *)&edata;

		 if (ioctl(skfd, SIOCETHTOOL, &ifr) == 0)
		 {
		     m_net_out = !edata.data;
		 }
		 else
		 { // SIOCGMIIREG
			unsigned short *data, mii_val;
			//printf("[%d]check ....SIOCGMIIREG\n",i);

			snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
			if (ioctl(skfd, SIOCGMIIPHY, &ifr) == 0)
			{
			 	data = (unsigned short *)(&ifr.ifr_data);
				data[1] = 1;
				if (ioctl(skfd, SIOCGMIIREG, &ifr) == 0)
				{
					 // mii_val |= data[3];
					 mii_val = data[3];
					 m_net_out = !(((mii_val & 0x0016) == 0x0004) ? 0 : 1);
				}
			}
		 }
	}

	close(skfd);

	return m_net_out;
}

static int get_ipv6_prefixlen(const S8 *netmask)
{
	int value[8] = {0};
	int prefixlen = 0;
	if(netmask == NULL)
	{
		OVFS_ASSERT(0);
		return -1;
	}
	sscanf(netmask, "%x:%x:%x:%x:%x:%x:%x:%x", &value[0], &value[1], &value[2], &value[3],
												&value[4], &value[5], &value[6], &value[7]);

	for(U32 i = 0; i < OVFS_ARRAY_DIM(value); ++i)
	{
		if(value[i] == 0xffff)
		{
			prefixlen += 16;
			continue;
		}
		for(U32 j = 0; j < 16; ++j)
		{
			if(value[i] & (1 << (15 -j)))
				prefixlen ++;
			else
				break;
		}
		break;
	}
	return prefixlen;
}

static OVFS_ERR get_local_mac(const char *ifname, U8 *mac , U32 size)
{
	if(size < 6 || mac == NULL)
	{
      return OVFS_ERR_UTL_BASE;
	}
    struct ifreq ifr;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0)
    {
      return OVFS_ERR_UTL_BASE;
    }

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
   	int ret = ioctl(sock,SIOCGIFHWADDR,&ifr,sizeof(ifr));
    if(ret == 0)
    {
      /*snprintf(mac, size, "%02X:%02X:%02X:%02X:%02X:%02X",
	  	ifr.ifr_hwaddr.sa_data[0],
	  	ifr.ifr_hwaddr.sa_data[1],
	  	ifr.ifr_hwaddr.sa_data[2],
	  	ifr.ifr_hwaddr.sa_data[3],
	  	ifr.ifr_hwaddr.sa_data[4],
	  	ifr.ifr_hwaddr.sa_data[5]);*/
	  	for(U32 i = 0; i < size && OVFS_ARRAY_DIM(ifr.ifr_hwaddr.sa_data); ++i)
	  	{
			mac[i] = ifr.ifr_hwaddr.sa_data[i];
	  	}
    }
    else
    {
      LOGE("get mac ioctl error!\n");
    }
	
	close(sock);
    return 0 == ret?OVFS_SUCCESS:OVFS_ERR_UTL_BASE;
}



OVFS_BOOL ovfs_utility_is_net_dev_exist(const char *target)
{
	return is_net_dev_exist(target);
}

OVFS_BOOL ovfs_utility_is_net_dev_up(const char *if_name)
{
	return is_net_dev_up(if_name);
}

OVFS_BOOL ovfs_utility_is_slave_net_dev(const char *if_name)
{
	return is_slave_net_dev(if_name);
}

int ovfs_utility_is_net_dev_out(const char *if_name)
{
	return is_net_dev_out(if_name);
}

int ovfs_utility_get_ipv6_prefixlen(const S8 *netmask)
{
	return get_ipv6_prefixlen(netmask);
}

OVFS_ERR ovfs_utility_get_local_mac(const char *ifname, U8 *mac , U32 size)
{
	return get_local_mac(ifname, mac, size);
}

//---------------------------------------------------------------------------
//  Base64 code table
//  0-63 : A-Z(25) a-z(51), 0-9(61), +(62), /(63)
static char Base2Chr(unsigned char n)
{
    n &= 0x3F;
    if (n < 26)
        return (char) (n + 'A');
    else if (n < 52)
        return (char) (n - 26 + 'a');
    else if (n < 62)
        return (char) (n - 52 + '0');
    else if (n == 62)
        return '+';
    else
        return '/';
}

int ovfs_utility_base64_code(unsigned char *s,unsigned char *d)
{
	char CharSet[64]=
	{
		'A','B','C','D','E','F','G','H',
		'I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X',
		'Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n',
		'o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3',
		'4','5','6','7','8','9','+','/'
	};
	unsigned char In[3];
	unsigned char Out[4];
	int cnt=0;
	if(!s||!d) return 0;
	for(;*s!=0;)
	{
		if(cnt+4>76)
		{
			*d++='\n';
			cnt=0;
		}
		if(strlen((char*)s)>=3)
		{
			In[0]=*s;
			In[1]=*(s+1);
			In[2]=*(s+2);
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|(In[1]&0xf0)>>4;
			Out[2]=(In[1]&0x0f)<<2|(In[2]&0xc0)>>6;
			Out[3]=In[2]&0x3f;
			*d=CharSet[Out[0]];
			*(d+1)=CharSet[Out[1]];
			*(d+2)=CharSet[Out[2]];
			*(d+3)=CharSet[Out[3]];
			s+=3;
			d+=4;
		}
		else if(strlen((char*)s)==1)
		{
			In[0]=*s;
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|0;
			*d=CharSet[Out[0]];
			*(d+1)=CharSet[Out[1]];
			*(d+2)='=';
			*(d+3)='=';
			s+=1;
			d+=4;
		}
		else if(strlen((char*)s)==2)
		{
			In[0]=*s;
			In[1]=*(s+1);
			Out[0]=In[0]>>2;
			Out[1]=(In[0]&0x03)<<4|(In[1]&0xf0)>>4;
			Out[2]=(In[1]&0x0f)<<2|0;
			*d=CharSet[Out[0]];
			*(d+1)=CharSet[Out[1]];
			*(d+2)=CharSet[Out[2]];
			*(d+3)='=';
			s+=2;
			d+=4;
		}
		cnt+=4;
	}
	*d='\0';
	return 1;
}

int ovfs_utility_base64encode22(char * const aDest, const unsigned char * aSrc, int aLen)
{
    char * p = aDest;
    int i;
    unsigned char t = '0';

    for (i = 0; i < aLen; i++)
    {
        switch (i % 3)
        {
            case 0:
                *p++ = Base2Chr(*aSrc >> 2);
                t = (*aSrc++ << 4) & 0x3F;
                break;
            case 1:
                *p++ = Base2Chr(t | (*aSrc >> 4));
                t = (*aSrc++ << 2) & 0x3F;
                break;
            case 2:
                *p++ = Base2Chr(t | (*aSrc >> 6));
                *p++ = Base2Chr(*aSrc++);
                break;
        }
    }
    if (aLen % 3 != 0)
    {
        *p++ = Base2Chr(t);
        if (aLen % 3 == 1)
            *p++ = '=';
        *p++ = '=';
    }
    *p = 0;  //  aDest is an ASCIIZ string
    return (p - aDest);  //  exclude the end of zero
}

OVFS_BOOL ovfs_utility_ipaddr_valid(ovfs_ipaddr_struct *ip)
{
    if (ip == NULL)
    {
        LOGE("param error\n");
        return OVFS_FALSE;
    }

    if (ip->is_ipv4 == OVFS_TRUE)
    {
        struct in_addr addr;
        if (0 == inet_pton(AF_INET, ip->ipv4, &addr))
        {
            LOGE("%s\n", ip->ipv4);
            return OVFS_FALSE;
        }
        else
        {
            //UTL_PD("%s\n", ip->ipv4);
            if((addr.s_addr & 0xFF) > 0)
            	return OVFS_TRUE;
        }
    }
    else
    {
        struct in6_addr addr;
        if (0 == inet_pton(AF_INET6, ip->ipv6, &addr))
        {
            LOGE("%s\n", ip->ipv6);
            return OVFS_FALSE;
        }
        else
        {
            //UTL_PD("%s\n", ip->ipv6);
            if((addr.__in6_u.__u6_addr8[0] & 0xFF) > 0)
            	return OVFS_TRUE;
        }
    }

    return OVFS_FALSE;
}

OVFS_BOOL ovfs_utility_netmask_valid(ovfs_ipaddr_struct *ip)
{
    if (OVFS_FALSE == ovfs_utility_ipaddr_valid(ip))
    {
        LOGE("param error\n");
        return OVFS_FALSE;
    }

    if (ip->is_ipv4 == OVFS_TRUE)
    {
        struct in_addr addr;
        if (0 == inet_pton(AF_INET, ip->ipv4, &addr))
        {
            LOGE("%s\n", ip->ipv4);
            return OVFS_FALSE;
        }
        else
        {
            U32 tmp;
			tmp = ntohl(addr.s_addr);
			tmp = ~tmp + 1;
			//LOGI("tmp = %x %x %d\n", tmp, tmp - 1, tmp & (tmp - 1));
			if ((tmp & (tmp - 1)) == 0)
			{
			    return OVFS_TRUE;
			}
			else
			{
                return OVFS_FALSE;
			}
        }
    }
    else
    {

    }

    return OVFS_FALSE;
}

OVFS_BOOL ovfs_utility_same_subnet(ovfs_ipaddr_struct *ip1, ovfs_ipaddr_struct *ip2, ovfs_ipaddr_struct *mask)
{
    if (OVFS_FALSE == ovfs_utility_ipaddr_valid(ip1) || OVFS_FALSE == ovfs_utility_ipaddr_valid(ip2) || OVFS_FALSE == ovfs_utility_netmask_valid(mask))
    {
        LOGE("param error\n");
        return OVFS_FALSE;
    }

    if (ip1->is_ipv4 == OVFS_TRUE && ip2->is_ipv4 == OVFS_TRUE && mask->is_ipv4 == OVFS_TRUE)
    {
        struct in_addr addr1;
		struct in_addr addr2;
		struct in_addr addr_mask;
        if (0 == inet_pton(AF_INET, ip1->ipv4, &addr1) || 
			0 == inet_pton(AF_INET, ip2->ipv4, &addr2) ||
			0 == inet_pton(AF_INET, mask->ipv4, &addr_mask))
        {
            LOGE("%s %s\n", ip1->ipv4,  ip2->ipv4);
            return OVFS_FALSE;
        }
        else
        {
            U32 tmp1 ,tmp2, tmp_mask;
			tmp1 = ntohl(addr1.s_addr);
			tmp2 = ntohl(addr2.s_addr);
			tmp_mask = ntohl(addr_mask.s_addr);
			if ((tmp1 & tmp_mask) == (tmp2 & tmp_mask))
			{
			    return OVFS_TRUE;
			}
			else
			{
                return OVFS_FALSE;
			}
        }
    }
    else
    {

    }

    return OVFS_FALSE;
}

OVFS_BOOL ovfs_utility_gateway_valid(ovfs_ipaddr_struct *ip, ovfs_ipaddr_struct *gateway, ovfs_ipaddr_struct *mask)
{
    if (OVFS_FALSE == ovfs_utility_ipaddr_valid(ip) || OVFS_FALSE == ovfs_utility_ipaddr_valid(gateway) || OVFS_FALSE == ovfs_utility_netmask_valid(mask))
    {
        LOGE("param error\n");
        return OVFS_FALSE;
    }
		
	if (ip->is_ipv4 == OVFS_TRUE && gateway->is_ipv4 == OVFS_TRUE && mask->is_ipv4 == OVFS_TRUE)
	{
		struct in_addr addr;
		struct in_addr addr_gateway;
		struct in_addr addr_mask;
		U32 tmp1 ,tmp2, tmp_mask;
		if (0 == inet_pton(AF_INET, ip->ipv4, &addr) || 
			0 == inet_pton(AF_INET, gateway->ipv4, &addr_gateway) ||
			0 == inet_pton(AF_INET, mask->ipv4, &addr_mask))
		{
			LOGE("%s %s\n", ip->ipv4, gateway->ipv4);
			return OVFS_FALSE;
		}
		else
		{
			if(addr.s_addr & (0xFF == 0))  return OVFS_FALSE;
			if(addr_mask.s_addr & (0xFF == 0))  return OVFS_FALSE;
			if(addr_gateway.s_addr & (0xFF == 0))  return OVFS_FALSE;
			tmp1 = ntohl(addr.s_addr);
			tmp2 = ntohl(addr_gateway.s_addr);
			tmp_mask = ntohl(addr_mask.s_addr);
			if ((tmp1 & tmp_mask) == (tmp2 & tmp_mask))
			{
				return OVFS_TRUE;
			}
			else
			{
			    struct in_addr addr_tmp_gateway;
				addr_tmp_gateway.s_addr = htonl((tmp1 & tmp_mask) + 1);
			    inet_ntop(AF_INET, (void *)&addr_tmp_gateway, gateway->ipv4, OVFS_IPV4_STRING_LEN + 1);
				return OVFS_TRUE;
			}
		}
	}
	else
	{
	
	}


    return OVFS_FALSE;
}


const ovfs_inet_ipv4_ip_info_list *ovfs_utility_get_ipv4_ipconfig(const S8* if_name)
{
	//UTL_PD("ovfs_get_ipv4_ipconfig %s\n", if_name);
	struct ifaddrs *ifaddr, *ifa;
    int family;
	int s = 0;
	ovfs_inet_ipv4_ip_info_list *ipv4_config = NULL;
	if(if_name == NULL )
	{
		OVFS_ASSERT(0);
		return NULL;
	}
	
	if(!is_net_dev_exist(if_name) || !is_net_dev_up(if_name))
	{
		LOGE("getifaddrs if_name = %s is note exist\n", if_name);
		return NULL;
	}

	U8 mac[6];
	if(OVFS_SUCCESS != get_local_mac(if_name, mac, sizeof(mac)))
	{
		LOGE("%s get_local_mac fail \n", if_name);
		return NULL;
	}
	
	
    if (getifaddrs(&ifaddr) == -1) 
	{
		LOGE("getifaddrs if_name = %s fail\n", if_name);
		return NULL;
	}
	
	for(ifa = ifaddr; NULL != ifa; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL
			|| ifa->ifa_name == NULL)
		{
			continue;
		}
		if(strncmp(if_name, ifa->ifa_name, strlen(if_name)) != 0)
		{
			continue;
		}
		family = ifa->ifa_addr->sa_family;
		if(family == AF_INET)
		{
			
			if(ipv4_config == NULL)
			{
				ipv4_config = (ovfs_inet_ipv4_ip_info_list *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv4_ip_info_list), 0,__FUNCTION__,__LINE__);
				if(ipv4_config != NULL)
				{
					memset(ipv4_config, 0, sizeof(ovfs_inet_ipv4_ip_info_list) );
				}
			}
			if(ipv4_config == NULL)
			{
				break; 

			}
			
			ovfs_inet_ipv4_ip_info_node *ipv4_node = (ovfs_inet_ipv4_ip_info_node *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv4_ip_info_node), 0,__FUNCTION__,__LINE__);
			if(ipv4_node == NULL)
			{
				break;
			}
			memset(ipv4_node, 0, sizeof(ovfs_inet_ipv4_ip_info_node));
			//S8 *pstr = strstr(ifa->ifa_name, ":");
			//if(pstr != NULL)
			//{
			snprintf(ipv4_node->label, sizeof(ipv4_node->label), "%s", ifa->ifa_name);
			
			//}
			s += getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),
							ipv4_node->info.ip, sizeof(ipv4_node->info.ip), 
							NULL, 0, NI_NUMERICHOST);
			
			s += getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in),
							ipv4_node->info.netmask, sizeof(ipv4_node->info.netmask), 
							NULL, 0, NI_NUMERICHOST);

			if(ipv4_config->head == NULL)
			{
				ipv4_config->head = ipv4_node;
				ipv4_config->tail = ipv4_node;
			}
			else
			{
				ipv4_node->prev = ipv4_config->tail;
				ipv4_config->tail->next = ipv4_node;
				ipv4_config->tail = ipv4_node;
			}
			ipv4_config->addr_num++;
			
		}
	}
	freeifaddrs(ifaddr);
	return ipv4_config;
}

OVFS_ERR ovfs_utility_relase_ipv4_ipconfig(const ovfs_soft::ovfs_inet_ipv4_ip_info_list *ipv4_config)
{
	if(ipv4_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_SUCCESS;
	}

	ovfs_soft::ovfs_inet_ipv4_ip_info_list *list = NULL;
	memcpy(&list, &ipv4_config, sizeof(ovfs_inet_ipv4_ip_info_list *));
	
	ovfs_inet_ipv4_ip_info_node *ipv4_node = list->head;
	while(ipv4_node != NULL)
	{
		ovfs_inet_ipv4_ip_info_node *del = ipv4_node;
		ipv4_node = ipv4_node->next;
		Common_Free(del,__FUNCTION__,__LINE__);
	}
	Common_Free(list,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
}

const ovfs_inet_ipv6_ip_info_list *ovfs_utility_get_ipv6_ipconfig(const S8* if_name)
{
	struct ifaddrs *ifaddr, *ifa;
    int family;
	int s = 0;
	S8 param[256]={0};
	ovfs_inet_ipv6_ip_info_list * ipv6_config = NULL;
	if(if_name == NULL )
	{
		OVFS_ASSERT(0);
		return NULL;
	}
	
	if(!is_net_dev_exist(if_name) || !is_net_dev_up(if_name))
	{
		LOGE("getifaddrs if_name = %s is note exist\n", if_name);
		return NULL;
	}

	U8 mac[6];
	if(OVFS_SUCCESS != get_local_mac(if_name, mac, sizeof(mac)) )
	{
		LOGE("%s get_local_mac fail \n", if_name);
		return NULL;
	}
	
	
    if (getifaddrs(&ifaddr) == -1) 
	{
		LOGE("getifaddrs if_name = %s fail\n", if_name);
		return NULL;
	}
	
	for(ifa = ifaddr; NULL != ifa; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL
			|| ifa->ifa_name == NULL)
		{
			continue;
		}
		if(strncmp(if_name, ifa->ifa_name, strlen(if_name)) != 0)
		{
			continue;
		}
		family = ifa->ifa_addr->sa_family;
		if(family == AF_INET6)
		{
			
			if(ipv6_config == NULL)
			{
				ipv6_config = (ovfs_inet_ipv6_ip_info_list *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv6_ip_info_list), 0,__FUNCTION__,__LINE__);
				if(ipv6_config != NULL)
				{
					memset(ipv6_config, 0, sizeof(ovfs_inet_ipv6_ip_info_list) );
				}
			}
			if(ipv6_config == NULL)
			{
				break; 

			}
			
			ovfs_inet_ipv6_ip_info_node *ipv6_node = (ovfs_inet_ipv6_ip_info_node *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv6_ip_info_node), 0,__FUNCTION__,__LINE__);
			if(ipv6_node == NULL)
			{
				continue;
			}
			memset(ipv6_node, 0, sizeof(ovfs_inet_ipv6_ip_info_node));
			s += getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
							ipv6_node->info.ip, sizeof(ipv6_node->info.ip), 
							NULL, 0, NI_NUMERICHOST);
			s += getnameinfo(ifa->ifa_netmask,sizeof(struct sockaddr_in6),
							param, sizeof(param), 
							NULL, 0, NI_NUMERICHOST);
			
			ipv6_node->info.prefixlen = get_ipv6_prefixlen(param);
			if(ipv6_config->head == NULL)
			{
				ipv6_config->head = ipv6_node;
				ipv6_config->tail = ipv6_node;
			}
			else
			{
				ipv6_node->prev = ipv6_config->tail;
				ipv6_config->tail->next = ipv6_node;
				ipv6_config->tail = ipv6_node;
			}
			ipv6_config->addr_num++;
		}
	}
	freeifaddrs(ifaddr);
	return ipv6_config;
}


OVFS_ERR ovfs_utility_relase_ipv6_ipconfig(const ovfs_inet_ipv6_ip_info_list *ipv6_config)
{
	if(ipv6_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_SUCCESS;
	}

	ovfs_soft::ovfs_inet_ipv6_ip_info_list *list = NULL;
	memcpy(&list, &ipv6_config, sizeof(ovfs_inet_ipv6_ip_info_list *));
	
	ovfs_inet_ipv6_ip_info_node *ipv6_node = list->head;
	while(ipv6_node != NULL)
	{
		ovfs_inet_ipv6_ip_info_node *del = ipv6_node;
		ipv6_node = ipv6_node->next;
		Common_Free(del,__FUNCTION__,__LINE__);
	}
	Common_Free(list,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
}

const ovfs_inet_ipv4_route_info_list *ovfs_utility_get_ipv4_route_config(const S8* if_name)
{
	char buf[1024];
	char devname[64];
	unsigned long d, g, m;
	int flgs, ref, use, metric, mtu, win, ir;
	ovfs_soft::ovfs_inet_ipv4_route_info_list *ipv4_route = NULL;
	FILE *fp = fopen("/proc/net/route", "r");
	if(fp == NULL)
	{
		OVFS_ASSERT(0);
		return NULL;
	}
	fgets(buf, sizeof buf, fp);	/* eat line */

	while (fgets(buf, sizeof buf, fp)) 
	{
	//	UTL_PD("%s\n", buf);
		int r = sscanf(buf, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
				   devname, &d, &g, &flgs, &ref, &use, &metric, &m,
				   &mtu, &win, &ir);
		if(r != 11)
		{
		//	LOGE("%d\n", r);
			break;
		}

		if(strncmp(devname, if_name, strlen(if_name)) != 0)
		{
		//	LOGE("%s:%d %s:%d\n", devname, strlen(devname), if_name, strlen(if_name));
			continue;
		}
	//	UTL_PW("%s, %lx, %lx, %X, %d, %d, %d, %lx, %d, %d, %d\n",
	//			   devname, d, g, flgs, ref, use, metric, m,
	//			   mtu, win, ir);
		if(g != 0)
		{
			if(ipv4_route == NULL)
			{
				ipv4_route = (ovfs_inet_ipv4_route_info_list *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv4_route_info_list), 0,__FUNCTION__,__LINE__);
				if(ipv4_route != NULL)
				{
					memset(ipv4_route, 0, sizeof(ovfs_inet_ipv4_route_info_list) );
				}
			}
			if(ipv4_route == NULL)
			{
				break; 

			}
			
			ovfs_inet_ipv4_route_info_node *ipv4_node = (ovfs_inet_ipv4_route_info_node *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv4_route_info_node), 0,__FUNCTION__,__LINE__);
			if(ipv4_node == NULL)
			{
				break;
			}
			memset(ipv4_node, 0, sizeof(ovfs_inet_ipv4_route_info_node));
			snprintf(ipv4_node->addr, sizeof(ipv4_node->addr), "%d.%d.%d.%d", (int)g&0xff, (int)(g>>8)&0xff, (int)(g>>16)&0xff, (int)(g>>24)&0xff);
			ipv4_node->metric = metric;
			
		//	UTL_PW("%s: %d\n", ipv4_node->addr, ipv4_node->metric);
			if(ipv4_route->head == NULL)
			{
				ipv4_route->head = ipv4_node;
				ipv4_route->tail = ipv4_node;
			}
			else
			{
				ipv4_node->prev = ipv4_route->tail;
				ipv4_route->tail->next = ipv4_node;
				ipv4_route->tail = ipv4_node;
			}
			ipv4_route->addr_num++;
		}
	}
	fclose(fp);
	return ipv4_route;
}


OVFS_ERR ovfs_utility_relase_ipv4_route_config(const ovfs_inet_ipv4_route_info_list *route_ipv4_config)
{
	if(route_ipv4_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_SUCCESS;
	}

	
	ovfs_soft::ovfs_inet_ipv4_route_info_list *list = NULL;
	memcpy(&list, &route_ipv4_config, sizeof(ovfs_inet_ipv4_route_info_list *));
	
	ovfs_inet_ipv4_route_info_node *ipv4_node = list->head;
	while(ipv4_node != NULL)
	{
		ovfs_inet_ipv4_route_info_node *del = ipv4_node;
		ipv4_node = ipv4_node->next;
		Common_Free(del,__FUNCTION__,__LINE__);
	}
	Common_Free(list,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
}

const ovfs_inet_ipv6_route_info_list *ovfs_utility_get_ipv6_route_config(const S8* if_name)
{
	char buf[1024];
	char addr6x[64];
	char nexthop[64];
	char iface[16];
	int iflags, metric, refcnt, use, prefix_len, slen;
	ovfs_soft::ovfs_inet_ipv6_route_info_list *ipv6_route = NULL;

	FILE *fp = fopen("/proc/net/ipv6_route", "r");
	if(fp == NULL)
	{
		OVFS_ASSERT(0);
		return NULL;
	}

	while (fgets(buf, sizeof buf, fp)) 
	{
		int r = sscanf(buf, "%63s%x%*s%x%63s%x%x%x%x%15s\n",
				addr6x, &prefix_len, &slen, nexthop, &metric, &use, &refcnt, &iflags, iface);
		if(r != 9)
		{
			break;
		}

		if(strcmp(iface, if_name) != 0)
		{
			continue;
		}

		if(strcmp(addr6x, "00000000000000000000000000000000") != 0)
		{
			if(ipv6_route == NULL)
			{
				ipv6_route = (ovfs_inet_ipv6_route_info_list *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv6_route_info_list), 0,__FUNCTION__,__LINE__);
				if(ipv6_route != NULL)
				{
					memset(ipv6_route, 0, sizeof(ovfs_inet_ipv6_route_info_list) );
				}
			}
			if(ipv6_route == NULL)
			{
				break; 

			}
			
			ovfs_inet_ipv6_route_info_node *ipv6_node = (ovfs_inet_ipv6_route_info_node *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv6_route_info_node), 0,__FUNCTION__,__LINE__);
			if(ipv6_node == NULL)
			{
				break;
			}
			memset(ipv6_node, 0, sizeof(ovfs_inet_ipv6_route_info_node));
			for(U32 i = 0; i < strlen(addr6x); i += 4)
			{
				if(i != 0)
					snprintf(ipv6_node->addr + strlen(ipv6_node->addr), sizeof(ipv6_node->addr) - strlen(ipv6_node->addr), "%s", ":");
				snprintf(ipv6_node->addr + strlen(ipv6_node->addr), sizeof(ipv6_node->addr) - strlen(ipv6_node->addr) , "%s", addr6x + i);
			}
			ipv6_node->metric = metric;
			if(ipv6_route->head == NULL)
			{
				ipv6_route->head = ipv6_node;
				ipv6_route->tail = ipv6_node;
			}
			else
			{
				ipv6_node->prev = ipv6_route->tail;
				ipv6_route->tail->next = ipv6_node;
				ipv6_route->tail = ipv6_node;
			}
			ipv6_route->addr_num++;
		}	
	}
	fclose(fp);

	return ipv6_route;
}


OVFS_ERR ovfs_utility_relase_ipv6_route_config(const ovfs_soft::ovfs_inet_ipv6_route_info_list *route_ipv6_config)
{
	if(route_ipv6_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_SUCCESS;
	}

	
	ovfs_soft::ovfs_inet_ipv6_route_info_list *list = NULL;
	memcpy(&list, &route_ipv6_config, sizeof(ovfs_inet_ipv6_route_info_list *));
	
	ovfs_inet_ipv6_route_info_node *ipv6_node = list->head;
	while(ipv6_node != NULL)
	{
		ovfs_inet_ipv6_route_info_node *del = ipv6_node;
		ipv6_node = ipv6_node->next;
		Common_Free(del,__FUNCTION__,__LINE__);
	}
	Common_Free(list,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
}

const ovfs_inet_ipv4_dns_info_list *ovfs_utility_get_ipv4_dns_config(const S8* if_name)
{
	S8 cfg_path[256];
	ovfs_inet_ipv4_dns_info_list *ipv4_dns = NULL;
	snprintf(cfg_path, sizeof(cfg_path), "/reserve/resolv_%s.conf\n", if_name);
	FILE *fp = fopen(cfg_path, "r");
	if(fp == NULL)
		return NULL;

	S8 sz_param[256];
	while(fgets(sz_param, sizeof(sz_param), fp) != NULL )
	{
		S8 *pstr = strstr(sz_param, "nameserver ");
		S8 *pstr1 = strstr(sz_param, "\n");
		if(NULL == pstr ||NULL == pstr1)
		{
			continue;
		}
		
		if(strstr(sz_param, ".") != NULL)
		{
			if(ipv4_dns == NULL)
			{
				ipv4_dns = (ovfs_inet_ipv4_dns_info_list *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv4_dns_info_list), 0,__FUNCTION__,__LINE__);
				if(ipv4_dns != NULL)
				{
					memset(ipv4_dns, 0, sizeof(ovfs_inet_ipv4_dns_info_list) );
				}
			}
			if(ipv4_dns == NULL)
			{
				break; 

			}
			
			ovfs_inet_ipv4_dns_info_node *ipv4_node = (ovfs_inet_ipv4_dns_info_node *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv4_dns_info_node), 0,__FUNCTION__,__LINE__);
			if(ipv4_node == NULL)
			{
				break;
			}
			memset(ipv4_node, 0, sizeof(ovfs_inet_ipv4_dns_info_node));
			pstr += strlen("nameserver ");
			*pstr1 = '\0';
			snprintf(ipv4_node->addr, sizeof(ipv4_node->addr) , "%s", pstr);
			
			if(ipv4_dns->head == NULL)
			{
				ipv4_dns->head = ipv4_node;
				ipv4_dns->tail = ipv4_node;
			}
			else
			{
				ipv4_node->prev = ipv4_dns->tail;
				ipv4_dns->tail->next = ipv4_node;
				ipv4_dns->tail = ipv4_node;
			}
			ipv4_dns->addr_num++;
		}
		else
		{
			continue;

		}
	}
	
	fclose(fp);
	return ipv4_dns;
}


OVFS_ERR ovfs_utility_relase_ipv4_dns_config(const ovfs_inet_ipv4_dns_info_list *dns_ipv4_config)
{
	if(dns_ipv4_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_SUCCESS;
	}

	
	ovfs_soft::ovfs_inet_ipv4_dns_info_list *list = NULL;
	memcpy(&list, &dns_ipv4_config, sizeof(ovfs_inet_ipv4_dns_info_list *));
	
	ovfs_inet_ipv4_dns_info_node *ipv4_node = list->head;
	while(ipv4_node != NULL)
	{
		ovfs_inet_ipv4_dns_info_node *del = ipv4_node;
		ipv4_node = ipv4_node->next;
		Common_Free(del,__FUNCTION__,__LINE__);
	}
	Common_Free(list,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
}


const ovfs_inet_ipv6_dns_info_list *ovfs_utility_get_ipv6_dns_config(const S8* if_name)
{
	S8 cfg_path[256];
	ovfs_inet_ipv6_dns_info_list *ipv6_dns = NULL;
	snprintf(cfg_path, sizeof(cfg_path), "/etc/resolv_%s.conf\n", if_name);
	FILE *fp = fopen(cfg_path, "r");
	if(fp == NULL)
		return NULL;

	S8 sz_param[256];
	while(fgets(sz_param, sizeof(sz_param), fp) != NULL )
	{
		S8 *pstr = strstr(sz_param, "nameserver ");
		S8 *pstr1 = strstr(sz_param, "\n");
		if(NULL == pstr ||NULL == pstr1)
		{
			continue;
		}
		
		if(strstr(sz_param, ":") != NULL)
		{
			if(ipv6_dns == NULL)
			{
				ipv6_dns = (ovfs_inet_ipv6_dns_info_list *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv6_dns_info_list), 0,__FUNCTION__,__LINE__);
				if(ipv6_dns != NULL)
				{
					memset(ipv6_dns, 0, sizeof(ovfs_inet_ipv6_dns_info_list) );
				}
			}
			if(ipv6_dns == NULL)
			{
				break; 

			}
			
			ovfs_inet_ipv6_dns_info_node *ipv6_node = (ovfs_inet_ipv6_dns_info_node *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_inet_ipv6_dns_info_node), 0,__FUNCTION__,__LINE__);
			if(ipv6_node == NULL)
			{
				break;
			}
			memset(ipv6_node, 0, sizeof(ovfs_inet_ipv6_dns_info_node));
			pstr += strlen("nameserver ");
			*pstr1 = '\0';
			snprintf(ipv6_node->addr, sizeof(ipv6_node->addr) , "%s", pstr);
			
			if(ipv6_dns->head == NULL)
			{
				ipv6_dns->head = ipv6_node;
				ipv6_dns->tail = ipv6_node;
			}
			else
			{
				ipv6_node->prev = ipv6_dns->tail;
				ipv6_dns->tail->next = ipv6_node;
				ipv6_dns->tail = ipv6_node;
			}
			ipv6_dns->addr_num++;
		}
		else
		{
			continue;

		}
	}
	
	fclose(fp);
	return ipv6_dns;
}



OVFS_ERR ovfs_utility_relase_ipv6_dns_config(const ovfs_soft::ovfs_inet_ipv6_dns_info_list *dns_ipv6_config)
{
	if(dns_ipv6_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_SUCCESS;
	}

	
	ovfs_soft::ovfs_inet_ipv6_dns_info_list *list = NULL;
	memcpy(&list, &dns_ipv6_config, sizeof(ovfs_inet_ipv6_dns_info_list *));
	
	ovfs_inet_ipv6_dns_info_node *ipv6_node = list->head;
	while(ipv6_node != NULL)
	{
		ovfs_inet_ipv6_dns_info_node *del = ipv6_node;
		ipv6_node = ipv6_node->next;
		Common_Free(del,__FUNCTION__,__LINE__);
	}
	Common_Free(list,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
}

const ovfs_if_config_struct *ovfs_utility_get_if_config(const S8* if_name)
{
	ovfs_if_config_struct * if_config = NULL;
	if(if_name == NULL)
	{
		OVFS_ASSERT(0);
		return NULL;
	}

	if(!is_net_dev_exist(if_name))
	{
		LOGE("getifaddrs if_name = %s is note exist\n", if_name);
		return NULL;
	}

	U8 mac[6];
	if(OVFS_SUCCESS != get_local_mac(if_name, mac, sizeof(mac)))
	{
		LOGE("%s get_local_mac fail \n", if_name);
		return NULL;
	}
	
	
	if_config = (ovfs_if_config_struct *)(OVFS_VOID*)Common_Malloc(sizeof(ovfs_if_config_struct), 0,__FUNCTION__,__LINE__);
	if(if_config == NULL)
	{
		return NULL;
	}
	
	memset(if_config, 0, sizeof(ovfs_if_config_struct));
	
	if_config->is_up = is_net_dev_up(if_name);
	if_config->is_slave = is_slave_net_dev(if_name);

	
	const ovfs_inet_ipv4_ip_info_list *ip_ipv4_config = ovfs_utility_get_ipv4_ipconfig(if_name);
	const ovfs_inet_ipv4_route_info_list *route_ipv4_config = ovfs_utility_get_ipv4_route_config(if_name);
	const ovfs_inet_ipv4_dns_info_list *dns_ipv4_config = ovfs_utility_get_ipv4_dns_config(if_name);

	memcpy(&if_config->ipv4_config.ip_config, &ip_ipv4_config, sizeof(ovfs_inet_ipv4_ip_info_list *));
	memcpy(&if_config->ipv4_config.route_config, &route_ipv4_config, sizeof(ovfs_inet_ipv4_route_info_list *));
	memcpy(&if_config->ipv4_config.dns_config, &dns_ipv4_config, sizeof(ovfs_inet_ipv4_dns_info_list *));

	
	const ovfs_inet_ipv6_ip_info_list *ip_ipv6_config = ovfs_utility_get_ipv6_ipconfig(if_name);
	const ovfs_inet_ipv6_route_info_list *route_ipv6_config = ovfs_utility_get_ipv6_route_config(if_name);
	const ovfs_inet_ipv6_dns_info_list *dns_ipv6_config = ovfs_utility_get_ipv6_dns_config(if_name);

	memcpy(&if_config->ipv6_config.ip_config, &ip_ipv6_config, sizeof(ovfs_inet_ipv6_ip_info_list *));
	memcpy(&if_config->ipv6_config.route_config, &route_ipv6_config, sizeof(ovfs_inet_ipv6_route_info_list *));
	memcpy(&if_config->ipv6_config.dns_config, &dns_ipv6_config, sizeof(ovfs_inet_ipv6_dns_info_list *));

	snprintf(if_config->ifname, sizeof(if_config->ifname), "%s", if_name);
	return if_config;
}


OVFS_ERR ovfs_utility_relase_if_config(const ovfs_soft::ovfs_if_config_struct *if_config)
{
	if( if_config == NULL)
	{
		OVFS_ASSERT(0);
		return OVFS_ERR_UTL_INVALID_PARA;
	}
	if(if_config->ipv4_config.ip_config != NULL)
	{
		ovfs_utility_relase_ipv4_ipconfig(if_config->ipv4_config.ip_config);
	}
	
	if(if_config->ipv4_config.route_config != NULL)
	{
		ovfs_utility_relase_ipv4_route_config(if_config->ipv4_config.route_config);
	}
	
	if(if_config->ipv4_config.dns_config != NULL)
	{
		ovfs_utility_relase_ipv4_dns_config(if_config->ipv4_config.dns_config);
	}
	
	if(if_config->ipv6_config.ip_config != NULL)
	{
		ovfs_utility_relase_ipv6_ipconfig(if_config->ipv6_config.ip_config);
	}
	
	if(if_config->ipv6_config.route_config != NULL)
	{
		ovfs_utility_relase_ipv6_route_config(if_config->ipv6_config.route_config);
	}
	
	if(if_config->ipv6_config.dns_config != NULL)
	{
		ovfs_utility_relase_ipv6_dns_config(if_config->ipv6_config.dns_config);
	}

	ovfs_soft::ovfs_if_config_struct *cfg = NULL;
	memcpy(&cfg, &if_config, sizeof(ovfs_soft::ovfs_if_config_struct *));
	Common_Free(cfg,__FUNCTION__,__LINE__);
	return OVFS_SUCCESS;
	
}

OVFS_ERR ovfs_utility_get_wlan_addr(char *ipaddr, char *netmask)
{
    if(NULL == ipaddr || NULL == netmask)
    {
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

	char *filename = (char *)"/tmp/udhcpc_ip_wlan0.conf";

	if(0 != access(filename, 0))
    {
		return OVFS_ERR_UTL_FILE_IO_ERR;
	}
    else
    {
        FILE* fp = fopen(filename, "r");
        if (NULL == fp)
        {
            return OVFS_ERR_UTL_FILE_IO_ERR;
        }
        else
        {
			fscanf(fp, "wlan0 %s netmask %s", ipaddr, netmask);
			fclose(fp);
			return OVFS_SUCCESS; 
        }
    }
	
	return OVFS_SUCCESS; 
}

OVFS_ERR ovfs_utility_get_wlan_gateway(char *gateway)
{
    if(NULL == gateway)
    {
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

	char *filename = (char *)"/tmp/udhcpc_route_wlan0.conf";

	if(0 != access(filename, 0))
    {
		return OVFS_ERR_UTL_FILE_IO_ERR;
	}
    else
    {
        FILE* fp = fopen(filename, "r");
        if (NULL == fp)
        {
            return OVFS_ERR_UTL_FILE_IO_ERR;
        }
        else
        {
			fscanf(fp, "gw %s", gateway);
			fclose(fp);
			return OVFS_SUCCESS; 
        }
    }
	
	return OVFS_SUCCESS; 
}

OVFS_ERR ovfs_utility_get_wlan_dns(char *dns1, char *dns2)
{
    if(NULL == dns1 || NULL == dns2)
    {
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

    char acBuf[128] = {0};
	char *filename = (char *)"/tmp/udhcpc_resolv_wlan0.conf";

	if(0 != access(filename, 0))
    {
		return OVFS_ERR_UTL_FILE_IO_ERR;
	}
    else
    {
        FILE* fp = fopen(filename, "r");
        if (NULL == fp)
        {
            return OVFS_ERR_UTL_FILE_IO_ERR;
        }
        else
        {
            memset(acBuf, 0, sizeof(acBuf));
            fgets(acBuf,sizeof(acBuf),fp);
            sscanf(acBuf, "nameserver %s", dns1);
            
            memset(acBuf, 0, sizeof(acBuf));
            fgets(acBuf,sizeof(acBuf),fp);
            sscanf(acBuf, "nameserver %s", dns2);
            
			fclose(fp);
			return OVFS_SUCCESS; 
        }
    }
	
	return OVFS_SUCCESS; 
}

OVFS_ERR ovfs_utility_get_wifi_mode(int *mode)
{
    char system_cmd[128] = {0};

    if(NULL == mode)
    {
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

    memset(system_cmd, 0, sizeof(system_cmd));
    Common_Exe_Cmd("ps | grep hostapd", 10000, system_cmd, sizeof(system_cmd));
    //hostapd -B /tmp/hostapd.conf应该匹配这个，但是linux 终端中一行字符串太长被截断了
    if(strstr(system_cmd, "hostapd -B"))
    {
        *mode = 0;
    }
    else
    {
        *mode = 1;
    }
	
	return OVFS_SUCCESS; 
}

OVFS_ERR ovfs_utility_get_wifi_ssid(char *ssid)
{
    char system_cmd[128] = {0};

    if(NULL == ssid)
    {
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

    memset(system_cmd, 0, sizeof(system_cmd));
    Common_Exe_Cmd("cat /tmp/hostapd.conf | grep ssid", 10000, system_cmd, sizeof(system_cmd));

	if(strstr(system_cmd, "ssid="))
    {
    	sscanf(system_cmd, "ssid=%s", ssid);
    	return OVFS_SUCCESS; 
    }
	
	return OVFS_ERR_UTL_UNKNOWN; 
}

int ovfs_utility_get_wifi_cardtype()
{
	char acCmdBuf[128];
	char acResBuf[128];
	
	memset(acCmdBuf, 0, sizeof(acCmdBuf));
	memset(acResBuf, 0, sizeof(acResBuf));
	
	sprintf(acCmdBuf, "%s", "lsmod | grep hawk_usb");
	Common_Exe_Cmd(acCmdBuf, 3000, acResBuf, sizeof(acResBuf));

	return (strlen(acResBuf) ? 1 : 0);
}

