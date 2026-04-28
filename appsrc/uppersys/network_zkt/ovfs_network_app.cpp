/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 处理模块中配置相关的操作
 *	源 文 件 : ovfs_network_app.cpp
 *	作    者 : 舒适
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
//#include <sys/syscall.h>

#include "ovfs_comm_tool.h"
#include "ovfs_comm_def.h"
//#include "ovfs_comm_debug.h"
#include "ovfs_comm_errno.h"

#include "ovfs_network_base_def.h"
#include "ovfs_network_app.h"
#include "ovfs_network_api.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_arp_tool_api.h"
#include "ovfs_network_rest_common.h"
#include "ovfs_network_event.h"
#include "ovfs_network_telnetd.h"
#include "ovfs_network_snmp.h"
#include "ovfs_network_ftp.h"
#include "ovfs_network_http.h"
#include "ovfs_network_wifi.h"
#include "ovfs_network_utility_api.h"
#include "ovfs_network_4g.h"
#include "ovfs_network_transdata.h"


#define CFG_NO_MATCH		    -100
#define MAC_PATH                "/usr/etc/macaddr"
#define NETWORK_CFG_PATH        "/usr/etc/cfgfiles/NetWork.json"

using namespace ovfs_soft;
//using namespace ovfs_cfgm;

typedef struct {
	char szSubject[64];
	char szContent[128];
	char szAttachFile[64];
}EMAILMESSAGE,*LPEMAILMESSAGE;

#if 0
typedef struct
{
	Common_Thread_T s_alarm_thread;
	int m_net_out[2];
	int m_ip_conflict[2];
	int s_alarm_exit;
} NETWORK_APP_T;
#endif

static OVFS_BOOL s_api_init = OVFS_FALSE;
static pthread_mutex_t s_api_init_lock = PTHREAD_MUTEX_INITIALIZER;
//static OVFS_BOOL s_wifi_enable = OVFS_FALSE;
//static NETWORK_APP_T  s_network_app;

pthread_mutex_t g_ip_set_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_dns_set_lock = PTHREAD_MUTEX_INITIALIZER;

extern int NetWork_WifiConfig(int type, const char* uriString,Common_cJSON_T* in,Common_cJSON_T* out);

static int NetWork_SetCfg_Ability()
{
    int ret = 0;
    ovfs_network_capability m_network_capability;
	U32 i = 0;
	ovfs_netcard_config eth;

    memset(&m_network_capability, 0 , sizeof(m_network_capability));
	m_network_capability.total_eth_num = 1;
	m_network_capability.eth_status[0].is_exist = OVFS_TRUE;
	m_network_capability.eth_status[0].ability_mask = OVFS_ETH_EANBLE|OVFS_ETH_CAN_BE_ROUTER|OVFS_ETH_CAN_BE_SEARCHED;
	snprintf(m_network_capability.eth_status[0].if_name, sizeof(m_network_capability.eth_status[0].if_name), "%s", "eth0");
	snprintf(m_network_capability.eth_status[0].description, sizeof(m_network_capability.eth_status[0].description), "%s", "LAN1");

    ret = ovfs_cfgm_set_network_cap(&m_network_capability);

    for(i = 0; i < m_network_capability.total_eth_num; i++)
    {
        memset(&eth,0,sizeof(eth));
	    eth.ip.is_ipv4 = OVFS_TRUE;
	    eth.netmask.is_ipv4 = OVFS_TRUE;
	    eth.gate_way.is_ipv4 = OVFS_TRUE;
		eth.dhcp = OVFS_FALSE;

		snprintf(eth.if_name,sizeof(eth.if_name),"%s",m_network_capability.eth_status[i].if_name);
		snprintf(eth.ip.ipv4,sizeof(eth.ip.ipv4),"192.168.%d.188",1+i);
		snprintf(eth.netmask.ipv4,sizeof(eth.netmask.ipv4),"%s","255.255.255.0");
		snprintf(eth.gate_way.ipv4,sizeof(eth.gate_way.ipv4),"192.168.%d.1",1+i);

		ret = ovfs_cfgm_add_local_eth_cfg(&eth);
		if (0 != ret)
		{
		   LOGE("[%s:%d]:ovfs_cfgm_add_local_eth_cfg fail\n",__FUNCTION__, __LINE__);
           return -1;
		}
    }


	return ret;
}

/*

static int FillDevInfo(void* cfgData,cJSON_Struct* parentItem)
{
	char *pStringValue;

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"SerialNumber",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		ovfs_cfgm_set_serialno(pStringValue);
	}

    return 0;
}


static int NetWork_SetCfg_DevInfo()
{
	FillDevInfo(NULL,NULL);

    return 0;
}
*/

static int FillNetLocalInfo(void* cfgData,cJSON_Struct* parentItem, cJSON_Struct *pJsonDefault, S32 nIdx)
{
	char *pStringValue;
	S32 nIntValue;
	ovfs_netcard_config *cfgReal;

    if(cfgData == NULL || (parentItem == NULL && pJsonDefault == NULL))
    {
        return -1;
    }

    cfgReal = (ovfs_netcard_config *)cfgData;
	memset(cfgReal,0,sizeof(ovfs_netcard_config));
    cfgReal->ip.is_ipv4 = OVFS_TRUE;
    cfgReal->netmask.is_ipv4 = OVFS_TRUE;
    cfgReal->gate_way.is_ipv4 = OVFS_TRUE;
	snprintf(cfgReal->if_name,sizeof(cfgReal->if_name),"eth%d",nIdx);

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"EthName",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->if_name,pStringValue,sizeof(cfgReal->if_name));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"EthName",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->if_name,pStringValue,sizeof(cfgReal->if_name));
		}
	}

	ovfs_cfgm_get_local_eth_cfg(cfgReal->if_name, cfgReal);

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"EnableDhcp",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		cfgReal->dhcp = (OVFS_BOOL)nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"EnableDhcp",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			cfgReal->dhcp = (OVFS_BOOL)nIntValue;
		}
	}

    if(Common_File_IsExist("/proc/net/if_inet6"))
    {
        cfgReal->ipv6_support = OVFS_TRUE;
    }
    else
    {
        cfgReal->ipv6_support = OVFS_FALSE;
    }

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"EnableDhcpV6",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		cfgReal->dhcpv6 = (OVFS_BOOL)nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"EnableDhcpV6",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			cfgReal->dhcpv6 = (OVFS_BOOL)nIntValue;
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"IpAddrV4",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->ip.ipv4,pStringValue,sizeof(cfgReal->ip.ipv4));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"IpAddrV4",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->ip.ipv4,pStringValue,sizeof(cfgReal->ip.ipv4));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"IpAddrV6",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->ip.ipv6,pStringValue,sizeof(cfgReal->ip.ipv6));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"IpAddrV6",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->ip.ipv6,pStringValue,sizeof(cfgReal->ip.ipv6));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"IpMaskV4",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->netmask.ipv4,pStringValue,sizeof(cfgReal->netmask.ipv4));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"IpMaskV4",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->netmask.ipv4,pStringValue,sizeof(cfgReal->netmask.ipv4));
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"IpV6PrefixLen",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        cfgReal->ipv6_prefixlen = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"IpV6PrefixLen",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
            cfgReal->ipv6_prefixlen = nIntValue;
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"MacAddr",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->mac,pStringValue,sizeof(cfgReal->mac));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"MacAddr",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->mac,pStringValue,sizeof(cfgReal->mac));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"GatewayV4",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->gate_way.ipv4,pStringValue,sizeof(cfgReal->gate_way.ipv4));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"GatewayV4",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->gate_way.ipv4,pStringValue,sizeof(cfgReal->gate_way.ipv4));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"GatewayV6",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(cfgReal->gate_way.ipv6,pStringValue,sizeof(cfgReal->gate_way.ipv6));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"GatewayV6",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->gate_way.ipv6,pStringValue,sizeof(cfgReal->gate_way.ipv6));
		}
	}

    return 0;
}

static int FillDefaultIFInfo(void* cfgData,cJSON_Struct* parentItem, cJSON_Struct *pJsonDefault)
{
	char *pStringValue;

	ovfs_cfgm_set_default_if("eth0");

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"DefaultRoute",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
        if (NULL != strstr(pStringValue, "eth"))
        {
            ovfs_cfgm_set_default_if(pStringValue);
			return 0;
        }
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"DefaultRoute",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
	        if (NULL != strstr(pStringValue, "eth"))
	        {
	            ovfs_cfgm_set_default_if(pStringValue);
				return 0;
	        }
		}
	}

    return 0;
}

static int FillNetDNSInfo(void* cfgData,cJSON_Struct* parentItem,int bInit)
{
	char *pStringValue;
	S32 nIntValue;

    if(NULL == cfgData)
    {
        return -1;
    }

    ovfs_local_net_dns *cfgReal = (ovfs_local_net_dns *)cfgData;
	if(bInit)
	{
		memset(cfgReal,0,sizeof(ovfs_local_net_dns));
		cfgReal->dns1.is_ipv4 = OVFS_TRUE;
		cfgReal->dns2.is_ipv4 = OVFS_TRUE;
		cfgReal->auto_dns = OVFS_FALSE;
		snprintf(cfgReal->dns1.ipv4,sizeof(cfgReal->dns1.ipv4),"8.8.8.8");
		snprintf(cfgReal->dns2.ipv4,sizeof(cfgReal->dns2.ipv4),"8.8.4.4");
	}

    if (parentItem != NULL)
    {
	    nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"AutoDNS",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			cfgReal->auto_dns = (OVFS_BOOL)nIntValue;
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Dns1V4",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->dns1.ipv4,pStringValue,sizeof(cfgReal->dns1.ipv4));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Dns1V6",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->dns1.ipv6,pStringValue,sizeof(cfgReal->dns1.ipv6));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Dns2V4",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->dns2.ipv4,pStringValue,sizeof(cfgReal->dns2.ipv4));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Dns2V6",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->dns2.ipv6,pStringValue,sizeof(cfgReal->dns2.ipv6));
		}
    }
	else
	{
	    LOGE("%s: Can't find %s\n",__FUNCTION__,"DNS");
	}

    return 0;
}

static int FillNetPppoeInfo(void* cfgData,cJSON_Struct* parentItem,int bInit)
{
	char *pStringValue;
	S32 nIntValue;

    if(cfgData == NULL)
    {
        return -1;
    }

    ovfs_pppoe_config* cfgReal = (ovfs_pppoe_config*)cfgData;
	if(bInit)
	{
		memset(cfgReal, 0, sizeof(ovfs_pppoe_config));
	}

	if (parentItem != NULL)
	{
	    nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			cfgReal->enable = (OVFS_BOOL)nIntValue;
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"User",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->user_name,pStringValue,sizeof(cfgReal->user_name));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Password",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(cfgReal->password,pStringValue,sizeof(cfgReal->password));
		}
	}
	else
	{
        LOGE("%s: Can't find %s\n",__FUNCTION__,"DNS");
	}

    return 0;
}


static int FillNetDDNSInfo(void* cfgData,cJSON_Struct* parentItem)
{
	U32 i = 0, num = 0;
	char *pStringValue;
	S32 nIntValue;
	ovfs_ddns_info ddns_info;
	cJSON_Struct *pJsonTmp = NULL;
	int ret = 0;

    if(cfgData == NULL)
    {
        return -1;
    }

	ovfs_ddns_ability* cfgReal = (ovfs_ddns_ability*)cfgData;
	LOGI("%s: ovfs_ddns_ability node_num = %d\n",__FUNCTION__,cfgReal->node_num);

	if (NULL != parentItem)
	{
	    nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			ovfs_cfgm_set_ddns_start((OVFS_BOOL)nIntValue);
		}

		pJsonTmp = Common_Json_GetItem(parentItem,-1,"DDNS");
		if (NULL != pJsonTmp)
		{
	        num = Common_Json_Size(pJsonTmp);
			if (num > 0)
			{
				for (i = 0; i<cfgReal->node_num && i<num; i++)
				{
				    memset(&ddns_info,0,sizeof(ddns_info));

					pStringValue = NULL;
					Common_Json_GetAttrValue(pJsonTmp,i,"DdnsProtoName",NULL,&pStringValue,NULL,NULL);
					if (NULL != pStringValue)
					{
						Common_Strncpy(ddns_info.param.ddns_name,pStringValue,sizeof(ddns_info.param.ddns_name));
					}

				    nIntValue = -1;
					Common_Json_GetAttrValue(pJsonTmp,i,"Enable",NULL,NULL,&nIntValue,NULL);
					if (-1 != nIntValue)
					{
						ddns_info.enable = (OVFS_BOOL)nIntValue;
					}

				    nIntValue = -1;
					Common_Json_GetAttrValue(pJsonTmp,i,"UpdateIpIntervalTime",NULL,NULL,&nIntValue,NULL);
					if (-1 != nIntValue)
					{
						ddns_info.update_min = (OVFS_BOOL)nIntValue;
					}

				    nIntValue = -1;
					Common_Json_GetAttrValue(pJsonTmp,i,"CheckIpIntervalTime",NULL,NULL,&nIntValue,NULL);
					if (-1 != nIntValue)
					{
						ddns_info.check_min = (OVFS_BOOL)nIntValue;
					}

					pStringValue = NULL;
					Common_Json_GetAttrValue(pJsonTmp,i,"User",NULL,&pStringValue,NULL,NULL);
					if (NULL != pStringValue)
					{
						Common_Strncpy(ddns_info.param.usr_name,pStringValue,sizeof(ddns_info.param.usr_name));
					}

					pStringValue = NULL;
					Common_Json_GetAttrValue(pJsonTmp,i,"Password",NULL,&pStringValue,NULL,NULL);
					if (NULL != pStringValue)
					{
						Common_Strncpy(ddns_info.param.passwd,pStringValue,sizeof(ddns_info.param.passwd));
					}

					pStringValue = NULL;
					Common_Json_GetAttrValue(pJsonTmp,i,"DomainName",NULL,&pStringValue,NULL,NULL);
					if (NULL != pStringValue)
					{
						Common_Strncpy(ddns_info.param.domain_host,pStringValue,sizeof(ddns_info.param.domain_host));
					}

					pStringValue = NULL;
					Common_Json_GetAttrValue(pJsonTmp,i,"ServerName",NULL,&pStringValue,NULL,NULL);
					if (NULL != pStringValue)
					{
						Common_Strncpy(ddns_info.param.server_host,pStringValue,sizeof(ddns_info.param.server_host));
					}

				    nIntValue = -1;
					Common_Json_GetAttrValue(pJsonTmp,i,"DdnsPort",NULL,NULL,&nIntValue,NULL);
					if (-1 != nIntValue)
					{
						ddns_info.param.server_port = (OVFS_BOOL)nIntValue;
					}

					ret = ovfs_cfgm_set_ddns_info(&ddns_info);
					if (0 != ret)
					{
                        LOGE("%s: Can't find %s\n",__FUNCTION__,"DdnsProtoName");
					}
				}
			}
		}
	}
	else
	{
        LOGE("%s: Can't find %s\n",__FUNCTION__,"DDNS");
	}

    return 0;
}

static int FillNetUpnpInfo(void* cfgData,cJSON_Struct* parentItem,int bInit)
{
	S32 nIntValue;
	S8 *pStringValue;
	S32 i = 0, num = 0;
	cJSON_Struct *pJsonTmp = NULL;
	ovfs_upnp_port_detail_info m_port_detail_info;

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		ovfs_cfgm_set_upnp_cfg_ex((OVFS_BOOL)nIntValue);
	}

	pJsonTmp = Common_Json_GetItem(parentItem,-1,"Map");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i < num; i++)
			{
			    COMMON_CLR_ARG(m_port_detail_info);

				nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"Enable",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.enable_upnp = (OVFS_BOOL)nIntValue;
				}

				nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"InPort",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.internal_port = nIntValue;
				}

				nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"OutPort",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.external_port = nIntValue;
				}

				nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"Protocol",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.port_protocol = (ovfs_upnp_port_protocol)nIntValue;
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Eth",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(m_port_detail_info.eth_name,pStringValue,sizeof(m_port_detail_info.eth_name));
				}

				ovfs_cfgm_set_upnp_port_cfg(m_port_detail_info.internal_port,m_port_detail_info.eth_name,m_port_detail_info.port_protocol,
					                        m_port_detail_info.external_port,m_port_detail_info.enable_upnp);
			}
		}
	}

    return 0;
}

static int FillNetMailInfo(void* cfgData,cJSON_Struct* parentItem, cJSON_Struct *pJsonDefault)
{
	unsigned int i = 0,num = 0;
	U32 receiver_num = 0;
	OVFS_BOOL enable_attach = OVFS_FALSE;
	ovfs_email_sender_cfg sender_cfg;
	ovfs_email_receiver_cfg receiver_cfg[OVFS_MAX_EMAIL_RECIEVER_NUM];
	char *pStringValue;
	S32 nIntValue = -1;
	cJSON_Struct *pJsonTmp = NULL;
	//char receiver_name[32];

	memset(&sender_cfg,0,sizeof(sender_cfg));
	memset(receiver_cfg,0,sizeof(receiver_cfg));

	Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		sender_cfg.enable = (OVFS_BOOL)nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Enable",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.enable = (OVFS_BOOL)nIntValue;
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Attachment",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		enable_attach = (OVFS_BOOL)nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Attachment",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			enable_attach = (OVFS_BOOL)nIntValue;
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Sender/User",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(sender_cfg.user_name,pStringValue,sizeof(sender_cfg.user_name));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Sender/User",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.user_name,pStringValue,sizeof(sender_cfg.user_name));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Sender/Password",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(sender_cfg.password,pStringValue,sizeof(sender_cfg.password));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Sender/Password",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.password,pStringValue,sizeof(sender_cfg.password));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Sender/SmtpServer",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(sender_cfg.server_addr,pStringValue,sizeof(sender_cfg.server_addr));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Sender/SmtpServer",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.server_addr,pStringValue,sizeof(sender_cfg.server_addr));
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Sender/SmtpPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		sender_cfg.server_port = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Sender/SmtpPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.server_port = nIntValue;
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Sender/EnableSsl",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		sender_cfg.ssl_enable = (OVFS_BOOL)nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"Sender/EnableSsl",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.ssl_enable = (OVFS_BOOL)nIntValue;
		}
	}

    receiver_num = 0;
	pJsonTmp = Common_Json_GetItem(pJsonDefault,-1,"Receiver");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<OVFS_ARRAY_DIM(receiver_cfg) && i<num; i++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Name",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].name,pStringValue,sizeof(receiver_cfg[i].name));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Addr",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].addr,pStringValue,sizeof(receiver_cfg[i].addr));
				}

				receiver_num++;
			}
		}
	}

    receiver_num = 0;
	pJsonTmp = Common_Json_GetItem(parentItem,-1,"Receiver");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<OVFS_ARRAY_DIM(receiver_cfg) && i<num; i++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Name",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].name,pStringValue,sizeof(receiver_cfg[i].name));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Addr",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].addr,pStringValue,sizeof(receiver_cfg[i].addr));
				}

				receiver_num++;
			}
		}
	}

    LOGI("%s: receiver_num = %d\n",__FUNCTION__, receiver_num);
	ovfs_cfgm_set_email_attachment_status(enable_attach);
	ovfs_cfgm_set_email_sender(&sender_cfg);
	ovfs_cfgm_set_email_receiver(receiver_cfg, receiver_num);

    return 0;
}

static int UpdateAutoEthCfg(OVFS_VOID *cfg_data, U32 cfg_size)
{
	return 0;
}

static int UpdateAutoDNSCfg(OVFS_VOID *cfg_data, U32 cfg_size)
{
	return 0;
}

static int UpdateAutoPPPOECfg(OVFS_VOID *cfg_data, U32 cfg_size)
{
	return 0;
}

static int UpdateAutoDDNSCfg(OVFS_VOID *cfg_data, U32 cfg_size)
{
    int ret = -1;
	cJSON_Struct *pJson = NULL;
	cJSON_Struct *pJsonTmp = NULL;

	if (cfg_size < sizeof(ovfs_ddns_ability))
	{
	    return -1;
	}

	Module_LoadConfig(NetWork_RestComm_GetModuleHdl(),&pJson);

	pJsonTmp = Common_Json_GetItem(pJson,-1,"NetApp/DDNS");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get DDNS info fail\n",__FUNCTION__, __LINE__);
	}

	ret = FillNetDDNSInfo(cfg_data,pJsonTmp);

	Common_Json_Delete(pJson);

	return ret;
}

static int NetCfgAutoUpdateCallback(void *user_param, S32 type, OVFS_VOID *cfg_data, U32 cfg_size)
{
    int ret = -1;
    if (NULL == cfg_data)
    {
        return -1;
	}

	if (OVFS_AUTO_LOCAL_ETH == type)
	{
	    ret = UpdateAutoEthCfg(cfg_data, cfg_size);
	}
    else if (OVFS_AUTO_DNS == type)
    {
        ret = UpdateAutoDNSCfg(cfg_data, cfg_size);
	}
	else if (OVFS_AUTO_PPPOE == type)
	{
        ret = UpdateAutoPPPOECfg(cfg_data, cfg_size);
	}
	else if (OVFS_AUTO_DDNS == type)
	{
        ret = UpdateAutoDDNSCfg(cfg_data, cfg_size);
	}

	return ret;
}

static int NetWork_SetCfg_Eth(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
    char mul_name[32];
	int ret = 0;
	U32 i = 0;
	ovfs_netcard_config eth;
	const ovfs_network_capability *m_network_capability = NULL;
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}

    for(i = 0; i < m_network_capability->total_eth_num; i++)
    {
        memset(mul_name,0,sizeof(mul_name));
        snprintf(mul_name,sizeof(mul_name),"NetAttr/Eth/%d",i);

		pJsonTmp = Common_Json_GetItem(pJson,-1,mul_name);
		if (NULL == pJsonTmp)
		{
		   LOGE("[%s:%d]:Common_Json_GetItem fail\n",__FUNCTION__, __LINE__);
		   pJsonTmpDefault = Common_Json_GetItem(pJsonDefault,-1,mul_name);
		   if (NULL == pJsonTmpDefault)
		   {
			   LOGE("[%s:%d]:Default Common_Json_GetItem fail\n",__FUNCTION__, __LINE__);
	           continue;
		   }
		}

		FillNetLocalInfo((void*)(&eth),pJsonTmp,pJsonTmpDefault,i);

		ret = ovfs_cfgm_set_local_eth_cfg(&eth,0);
		if (0 != ret)
		{
		   LOGE("[%s:%d]:ovfs_cfgm_set_local_eth_cfg fail\n",__FUNCTION__, __LINE__);
		}
    }

    return 0;
}

static int NetWork_SetCfg_DNS(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	int ret = -1;
	ovfs_local_net_dns dns;
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

	pJsonTmp = Common_Json_GetItem(pJson,-1,"NetAttr/DNS");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get DNS info fail\n",__FUNCTION__, __LINE__);
	}

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault,-1,"NetAttr/DNS");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:get DNS info fail\n",__FUNCTION__, __LINE__);
	}

    FillNetDNSInfo((void*)(&dns), pJsonTmpDefault,1);
	FillNetDNSInfo((void*)(&dns), pJsonTmp,0);

	ret = ovfs_cfgm_set_dns_cfg(&dns);

    return ret;
}

static int NetWork_SetCfg_DefaultIF(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

	pJsonTmp = Common_Json_GetItem(pJson,-1,"NetAttr/DefaultRoute");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get DefaultIF info fail\n",__FUNCTION__, __LINE__);
	}

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault,-1,"NetAttr/DefaultRoute");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get DefaultIF info fail\n",__FUNCTION__, __LINE__);
	}

    FillDefaultIFInfo(NULL,pJsonTmp,pJsonDefault);

    return 0;
}

static int NetWork_SetCfg_PPPOE(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	int ret = -1;
	ovfs_pppoe_config cfgData;
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

	pJsonTmp = Common_Json_GetItem(pJson,-1,"NetApp/PPPoE");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get pppoe info fail\n",__FUNCTION__, __LINE__);
	}

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault,-1,"NetApp/PPPoE");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get pppoe info fail\n",__FUNCTION__, __LINE__);
	}

	FillNetPppoeInfo((void*)(&cfgData),pJsonTmpDefault,1);
    FillNetPppoeInfo((void*)(&cfgData),pJsonTmp,0);

	ret = ovfs_cfgm_set_pppoe_cfg(&cfgData);

    return ret;
}

static int NetWork_SetCfg_UPNP(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

	pJsonTmp = Common_Json_GetItem(pJson,-1,"NetApp/UPnP");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get upnp info fail\n",__FUNCTION__, __LINE__);
	}

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault,-1,"NetApp/UPnP");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:Default get upnp info fail\n",__FUNCTION__, __LINE__);
	}

	FillNetUpnpInfo(NULL,pJsonTmpDefault,1);
	FillNetUpnpInfo(NULL,pJsonTmp,0);

    return 0;
}

static int NetWork_SetCfg_Mail(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

	pJsonTmp = Common_Json_GetItem(pJson,-1,"NetApp/Mail");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get mail info fail\n",__FUNCTION__, __LINE__);
	}

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault,-1,"NetApp/Mail");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get mail info fail\n",__FUNCTION__, __LINE__);
	}

	FillNetMailInfo(NULL,pJsonTmp,pJsonTmpDefault);

    return 0;
}

static int Modify_Mac(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *parentItem = NULL;
	char *mac, *eth;
	U32 num = 0, i = 0;
    const ovfs_network_capability *m_network_capability = NULL;
	S32 ret = 0;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -2;
	}

	parentItem = (cJSON_Struct*)in;

    pJsonTmp = Common_Json_GetItem(parentItem,-1,"MacInfo");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<m_network_capability->total_eth_num && i<num; i++)
			{
				mac = NULL;
				eth = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"EthName",NULL,&eth,NULL,NULL);
				Common_Json_GetAttrValue(pJsonTmp,i,"MacAddr",NULL,&mac,NULL,NULL);
				ret = NetWorkTool_SetMac(eth, mac);
			}
		}
	}

	if(0 != ret)
	{
		LOGE("Failed! ret=%d\n",ret);
		return -4;
	}
	ret = NetWork_RestComm_SaveConfig((char *)MAC_PATH, parentItem);

	return ret;
}

static S32 NetWork_SetCfg_MAC()
{
    unsigned int ret = 0;
    cJSON_Struct* pConfig = NULL;
	int bModifyMac;
	S8 szMac[32];
	S8 CurMac[32] = {0};
	S8 *pSerial = NULL;
	S32 szTmp[6];
	unsigned int i = 0;
	const ovfs_network_capability *m_network_capability = NULL;

	m_network_capability = ovfs_cfgm_get_network_cap();
	if (NULL == m_network_capability)
	{
		LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
		return -1;
	}

    ret = NetWork_RestComm_LoadConfig((char *)MAC_PATH, &pConfig);
	if (0 == ret)
	{
		cJSON_Struct *pJsonTmp = NULL;
		cJSON_Struct *parentItem = NULL;

		parentItem = (cJSON_Struct*)pConfig;
		pJsonTmp = Common_Json_GetItem(parentItem,-1,"MacInfo");
		if (NULL != pJsonTmp)
		{
			int num = Common_Json_Size(pJsonTmp);
			if (num > 0)
			{
				for (i = 0; i<m_network_capability->total_eth_num && i<(unsigned int)num; i++)
				{
					char *mac = NULL, *eth = NULL;
					Common_Json_GetAttrValue(pJsonTmp,i,"EthName",NULL,&eth,NULL,NULL);
					Common_Json_GetAttrValue(pJsonTmp,i,"MacAddr",NULL,&mac,NULL,NULL);

					bModifyMac = 1;
					ret = NetWorkTool_GetMacAddr(eth, CurMac);
					if(ret == 0)
					{
						LOGW("curmac = %s, setmac = %s\n", CurMac, mac);
						if(stricmp(CurMac, mac) == 0)
						{
							bModifyMac = 0;
						}
					}
					if(bModifyMac)
					ret = NetWorkTool_SetMac((S8 *)eth, mac);
				}
			}
		}
	}

	Common_Json_Delete(pConfig);

    if (0 != ret)
    {
		pSerial = Module_GetSerialNumber(0);
		if (NULL == pSerial)
		{
		    LOGE("[%s:%d]:Module_GetSerialNumber fail\n",__FUNCTION__, __LINE__);
		    return -2;
		}

		if (5 == sscanf(pSerial+10,"%02x%02x%02x%02x%02x",&szTmp[0],&szTmp[1],&szTmp[2],&szTmp[3],&szTmp[4]))
		{
			for (i = 0; i < m_network_capability->total_eth_num; i++)
			{
				bModifyMac = 1;
                sprintf(szMac,"%02x:%02x:%02x:%02x:%02x:%02x",i*2,szTmp[0],szTmp[1],szTmp[2],szTmp[3],szTmp[4]);
				ret = NetWorkTool_GetMacAddr((S8 *)m_network_capability->eth_status[i].if_name, CurMac);
				if(ret == 0)
				{
					if(stricmp(CurMac, szMac) == 0)
					{
						LOGW("curmac = %s, setmac = %s\n", CurMac, szMac);
						bModifyMac = 0;
					}
				}
				if(bModifyMac)
				ret = NetWorkTool_SetMac((S8 *)m_network_capability->eth_status[i].if_name, szMac);
			}
#if 0
			int n9001Exist = ovfs_utility_get_wifi_cardtype();
 			LOGW("n9001Exist=[%d]\n",n9001Exist);
			if(n9001Exist)
			{
				S8 acNewWlan1Mac[32] = {0};
				S8 acOldWlan1Mac[32] = {0};
				char acCmdBuf[128];
				snprintf(acCmdBuf, sizeof(acCmdBuf) - 1, "ifconfig wlan1 up");
				Common_System(acCmdBuf);

				bModifyMac = 1;
				ret = NetWorkTool_GetMacAddr((S8 *)"wlan1", acOldWlan1Mac);
				if(ret == 0)
				{
					S32 szTmpWlan1[6];
					if (6 == sscanf(acOldWlan1Mac,"%02x:%02x:%02x:%02x:%02x:%02x",&szTmpWlan1[0],&szTmpWlan1[1],&szTmpWlan1[2],&szTmpWlan1[3],&szTmpWlan1[4],&szTmpWlan1[5]))
					{
						sprintf(acNewWlan1Mac,"%02x:%02x:%02x:%02x:%02x:%02x",szTmpWlan1[0],szTmpWlan1[1],szTmpWlan1[2],szTmp[2],szTmp[3],szTmp[4]);

						if(stricmp(acOldWlan1Mac, acNewWlan1Mac) == 0)
						{
							LOGW("acOldWlan1Mac = %s, acNewWlan1Mac = %s\n", acOldWlan1Mac, acNewWlan1Mac);
							bModifyMac = 0;
						}

						if(bModifyMac)
						ret = NetWorkTool_SetMac((S8 *)"wlan1", acNewWlan1Mac);
					}
				}
			}
#endif
		}
	}
	return ret;
}

static int Send_Test_Mail(const char* uriString, const char* condition, Common_cJSON_T* curObj, Common_cJSON_T* in, Common_cJSON_T* out)
{
	cJSON_Struct *pInData = NULL, *pOutData = NULL;
	ovfs_email_sender_cfg sender_cfg;
	ovfs_email_receiver_cfg receiver_cfg[OVFS_MAX_EMAIL_RECIEVER_NUM];
	char *pStringValue = NULL;
	S32 nIntValue, nIndex = 0;
	S32 iResult, receiver_num = 0;
	cJSON_Struct *pArray = NULL;
	cJSON_Struct *pArrayReceiver = NULL;

	 memset(&sender_cfg, 0, sizeof(sender_cfg));
	 memset(&receiver_cfg, 0, sizeof(receiver_cfg));

	pInData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	if (pInData != NULL)
	{
		Common_Json_SetAttrValue(pInData, -1, "Header", Common_Json_Type_Object, NULL, 0, 0);
		Common_Json_SetAttrValue(pInData, -1, "Header/Uri", Common_Json_Type_String, "/Alarm/EmailCfg", 0, 0);
		Common_Json_SetAttrValue(pInData, -1, "Header/Method", Common_Json_Type_String, "get", 0, 0);
		Common_Json_SetAttrValue(pInData, -1, "Data", Common_Json_Type_Object, NULL, 0, 0);

		iResult = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(), pInData, &pOutData, 3000);
		if(iResult != 0)
		{
			pOutData = NULL;
			Common_Free(pInData, __FUNCTION__, __LINE__);
			return -1;
		}
		Common_Free(pInData, __FUNCTION__, __LINE__);
	}
	else
	{
		return -1;
	}

	sender_cfg.enable = OVFS_TRUE;
	pArray = Common_Json_GetItem(pOutData, 0, "Data/ResList");

	if(NULL == pArray)
	{
		Common_Free(pOutData, __FUNCTION__, __LINE__);
		return -1;
	}
	else
	{
		LOGI("[Status] tree:= %s \n", Common_Json_Print(pArray, NULL));
		pStringValue = NULL;
		Common_Json_GetAttrValue(pArray, -1, "Sender/User", NULL, &pStringValue, NULL, NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.user_name, pStringValue, sizeof(sender_cfg.user_name));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(pArray, -1, "Sender/Password", NULL, &pStringValue, NULL, NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.password,pStringValue,sizeof(sender_cfg.password));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(pArray,-1,"Sender/SmtpServer",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.server_addr,pStringValue,sizeof(sender_cfg.server_addr));
		}
		pStringValue = NULL;

		nIntValue = -1;
			Common_Json_GetAttrValue(pArray,-1,"Sender/SmtpPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.server_port = nIntValue;
		}

		nIntValue = -1;
		Common_Json_GetAttrValue(pArray,-1,"Sender/EnableSsl",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.ssl_enable = (OVFS_BOOL)nIntValue;
		}
		nIntValue = -1;

		pArrayReceiver = Common_Json_GetItem(pArray, -1, "Receiver");
		receiver_num = Common_Json_Size(pArrayReceiver);
		pArray = NULL;

		if(receiver_num == 0 || NULL == pArrayReceiver)
		{
			receiver_num = 0;
			pArrayReceiver = NULL;
			Common_Free(pOutData, __FUNCTION__, __LINE__);
			return -1;
		}
		else
		{
			for(nIndex = 0; nIndex < receiver_num; nIndex++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pArrayReceiver,nIndex,"Name",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[nIndex].name,pStringValue,sizeof(receiver_cfg[nIndex].name));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pArrayReceiver,nIndex,"Addr",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[nIndex].addr,pStringValue,sizeof(receiver_cfg[nIndex].addr));
				}
				pStringValue = NULL;
			}
		}
	}
	ovfs_network_send_email_tst(&sender_cfg,receiver_cfg,receiver_num);

	extern char g_array_buf[1024];
	Common_cJSON_AddStringToObject(out, "TestResult", g_array_buf);

	Common_Free(pOutData, __FUNCTION__, __LINE__);
	return 0;
}

static int Link_Send_Mail(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	U32 i = 0;
	U32 receiver_num = 0, num = 0, attachment_num = 0;
	//OVFS_BOOL enable_attach;
	int ret = 0;
	ovfs_email_sender_cfg sender_cfg;
	ovfs_email_receiver_cfg receiver_cfg[OVFS_MAX_EMAIL_RECIEVER_NUM];
	ovfs_email_attachment_fname attachment[OVFS_MAX_EMAIL_ATTACHMENT];
	char *pStringValue;
	S32 nIntValue, bSync = 0;
	cJSON_Struct *parentItem = NULL;
	cJSON_Struct *pJsonTmp = NULL;
	S8 *title = NULL;
	S8 *content = NULL;

	LOGI("[%s:%d]:Link_Send_Mail!!!!!!\n",__FUNCTION__, __LINE__);

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

    memset(&sender_cfg,0,sizeof(sender_cfg));
	{
		sender_cfg.enable = OVFS_TRUE;

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/User",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.user_name,pStringValue,sizeof(sender_cfg.user_name));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/Password",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.password,pStringValue,sizeof(sender_cfg.password));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/SmtpServer",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.server_addr,pStringValue,sizeof(sender_cfg.server_addr));
		}

		nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/SmtpPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.server_port = nIntValue;
		}

		nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/EnableSsl",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.ssl_enable = (OVFS_BOOL)nIntValue;
			//sender_cfg.ssl_enable = OVFS_TRUE;
		}
	}

	memset(attachment,0,sizeof(attachment));
	pJsonTmp = Common_Json_GetItem(parentItem,-1,"Attachment");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<OVFS_ARRAY_DIM(attachment) && i<num; i++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,NULL,NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(attachment[i].fname,pStringValue,sizeof(attachment[i].fname));
					LOGI("[%s:%d]:attachment[%d].fname = %s\n",__FUNCTION__, __LINE__,i,attachment[i].fname);
				}

				attachment_num++;
			}
		}
	}

    num = 0;
	pJsonTmp = NULL;
	memset(receiver_cfg,0,sizeof(receiver_cfg));
	pJsonTmp = Common_Json_GetItem(parentItem,-1,"Receiver");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<OVFS_ARRAY_DIM(receiver_cfg) && i<num; i++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Name",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].name,pStringValue,sizeof(receiver_cfg[i].name));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Address",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].addr,pStringValue,sizeof(receiver_cfg[i].addr));
				}

				receiver_num++;
			}
		}
	}


	title = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Title",NULL,&title,NULL,NULL);

	content = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Content",NULL,&content,NULL,NULL);

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Sync",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		bSync = nIntValue;
	}

	if (NULL != title && NULL != content && receiver_num > 0)
	{
	    LOGI("[%s:%d]:Title = %s Content = %s\n",__FUNCTION__, __LINE__,title,content);
        ret = ovfs_network_send_email(&sender_cfg,receiver_cfg,receiver_num,title,content,
			                          attachment,attachment_num,bSync == 1 ? OVFS_TRUE : OVFS_FALSE,OVFS_FALSE);
		//ovfs_network_send_email(&sender_cfg,receiver_cfg,receiver_num,title,content,
		//	                    NULL,0,OVFS_TRUE,OVFS_FALSE);
	}

	return ret == 0 ? 1 : -1;
}

static int NetWork_Cfg_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
    int ret = 0;

	ret = NetWork_SetCfg_Eth(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_Eth fail!.ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_SetCfg_DNS(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_DNS fail!.ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_SetCfg_DefaultIF(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_DefaultIF fail!.ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_SetCfg_PPPOE(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_PPPOE fail!.ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_SetCfg_UPNP(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_UPNP fail!.ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_SetCfg_Mail(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_Mail fail!.ret=%d\n",ret);
        return ret;
	}

	if(access("/tmp/telnetd", 0) != 0)
	{
		ret = NetWork_Telnetd_Init(pJson, pJsonDefault);
		if (0 != ret)
		{
		    LOGE("NetWork_Telnetd_Init fail!.ret=%d\n",ret);
	        return ret;
		}
	}

	ret = NetWork_SNMP_Init(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_SNMP_Init fail!.ret=%d\n",ret);
        return ret;
	}
#if 0
    if (OVFS_TRUE == s_wifi_enable)
    {
		ret = NetWork_Wifi_Init(pJson, pJsonDefault);
		if (0 != ret)
		{
		    LOGE("NetWork_Wifi_Init fail!.ret=%d\n",ret);
	        return ret;
		}
    }
#endif
	ret = NetWork_Ftp_Init(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_Ftp_Init fail!.ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_HTTP_Init(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_HTTP_Init fail!.ret=%d\n",ret);
        return ret;
	}

    ret = NetWork_TransData_Init(pJson, pJsonDefault);
	if (0 != ret)
	{
	    LOGE("NetWork_HTTP_Init fail!.ret=%d\n",ret);
        return ret;
	}

	return ret;
}

static int NetWork_Arp_Start()
{
    int ret = 0;

    ret = ovfs_arptool_init(1);
	if (0 != ret)
	{
        LOGE("[%s:%d]:ovfs_arptool_init fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

    ret = ovfs_arptool_start_arp_rarp_server();
	if (0 != ret)
	{
        LOGE("[%s:%d]:ovfs_arptool_start_arp_rarp_server fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	ret = NetWork_Event_Init();
	if (0 != ret)
	{
        LOGE("[%s:%d]:NetWork_Event_Init fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	ret = NetWork_Event_Start();
	if (0 != ret)
	{
        LOGE("[%s:%d]:NetWork_Event_Start fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

#if 0
    ret = Common_Thread_Create(&s_network_app.s_alarm_thread,"AlarmDoThread",0,0,AlarmDoThread,NULL);
	if(ret != 0)
	{
		LOGE("create arp thread fail! ret=%d\n",ret);
		return -1;
	}
#endif

	return 0;
}

/*
static int NetWork_Arp_Stop()
{
    int ret = 0;

#if 0
	s_network_app.s_alarm_exit = 1;
	Common_Thread_Destroy(&s_network_app.s_alarm_thread);
#endif

    ret = NetWork_Event_Stop();
	if (0 != ret)
	{
        LOGE("[%s:%d]:NetWork_Event_Stop fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	ret = NetWork_Event_Destroy();
	if (0 != ret)
	{
        LOGE("[%s:%d]:NetWork_Event_Destroy fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

    ret = ovfs_arptool_stop_arp_rarp_server();
	if (0 != ret)
	{
        LOGE("[%s:%d]:ovfs_arptool_stop_arp_rarp_server fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	return 0;
}
*/

static int NetWork_Get_WlanDns(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    char dns1[32] = {0};
    char dns2[32] = {0};

    ovfs_utility_get_wlan_dns(dns1, dns2);

    Common_cJSON_AddNumberToObject(out,"AutoDNS",1);
	Common_cJSON_AddStringToObject(out,"Dns1V4",dns1);
	Common_cJSON_AddStringToObject(out,"Dns1V6","");
	Common_cJSON_AddStringToObject(out,"Dns2V4",dns2);
	Common_cJSON_AddStringToObject(out,"Dns2V6","");

    return 0;
}

static int NetWork_Get_Wlan(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    char wlan_ipaddr[32] = {0};
    char wlan_netmask[32] = {0};
    char wlan_gateway[32] = {0};
    NetWorkTool_GetIPAddr((char *)"wlan0",wlan_ipaddr);
    NetWorkTool_GetMaskAddr((char *)"wlan0",wlan_netmask);
    NetWorkTool_GetGateway((char *)"wlan0",wlan_gateway);

    if(!strlen(wlan_ipaddr) || 0 == strcmp(wlan_ipaddr,"0.0.0.0") || !strlen(wlan_netmask) || 0 == strcmp(wlan_netmask,"0.0.0.0") || !strlen(wlan_gateway) || 0 == strcmp(wlan_gateway,"0.0.0.0"))
    {
        ovfs_utility_get_wlan_addr(wlan_ipaddr, wlan_netmask);
        ovfs_utility_get_wlan_gateway(wlan_gateway);
    }

    //???°wifi ¨¨???2?????¨|?-??????????€??a?
    if(!strlen(wlan_ipaddr) || 0 == strcmp(wlan_ipaddr,"0.0.0.0") || !strlen(wlan_netmask) || 0 == strcmp(wlan_netmask,"0.0.0.0") || !strlen(wlan_gateway) || 0 == strcmp(wlan_gateway,"0.0.0.0"))
    {
        strcpy(wlan_ipaddr, "192.168.10.1");
        strcpy(wlan_netmask, "255.255.255.0");
        strcpy(wlan_gateway, "192.168.10.1");
    }

    char mac[OVFS_MAC_STRING_LEN]={0};
	NetWorkTool_GetMacAddr((char *)"wlan0",mac);
    Common_cJSON_AddStringToObject(out,"EthName","wlan0");
	Common_cJSON_AddNumberToObject(out,"EnableDhcp",1);
	Common_cJSON_AddStringToObject(out,"IpAddrV4",wlan_ipaddr);
	Common_cJSON_AddStringToObject(out,"IpAddrV6","");
	Common_cJSON_AddStringToObject(out,"IpMaskV4",wlan_netmask);
	Common_cJSON_AddStringToObject(out,"IpMaskV6","");
	Common_cJSON_AddStringToObject(out,"GatewayV4",wlan_gateway);
	Common_cJSON_AddStringToObject(out,"GatewayV6","");
	Common_cJSON_AddStringToObject(out,"MacAddr",mac);

    return 0;
}

static int NetWork_Get_WifiSignal(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    int wifi_mode = 0;
    char wifi_ssid[128] = {0};

    ovfs_utility_get_wifi_mode(&wifi_mode);
    Common_cJSON_AddNumberToObject(out,"Mode",wifi_mode);
    if(wifi_mode == 0)
    {
        ovfs_utility_get_wifi_ssid(wifi_ssid);
        Common_cJSON_AddStringToObject(out,"Ssid",wifi_ssid);
    }

    return 0;
}

static int NetWork_Get_Eth(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;
	unsigned int idx = 0;
    const ovfs_network_capability *m_network_capability = NULL;
	ovfs_netcard_config ethcfg;
	//ovfs_netcard_config real_ethcfg;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	//LOGI("[%s:%d]:%s\n",__FUNCTION__, __LINE__, uriString);

	idx = atoi(uriString+strlen("/Network/NetAttr/Eth/"));

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}

    if (idx < 0 || idx > m_network_capability->total_eth_num - 1)
    {
        return -1;
	}
	else
	{
	    memset(&ethcfg,0,sizeof(ethcfg));
		//memset(&real_ethcfg,0,sizeof(real_ethcfg));
		/*取网卡的实际配置信息*/
        //ret = ovfs_network_get_local_eth_cfg(m_network_capability->eth_status[idx].if_name,&real_ethcfg);
		/*取保存的配置信息*/
		pthread_mutex_lock(&g_ip_set_lock);
		ret = ovfs_cfgm_get_local_eth_cfg(m_network_capability->eth_status[idx].if_name,&ethcfg);
		if (0 != ret)
		{
			pthread_mutex_unlock(&g_ip_set_lock);
            return -1;
		}
		pthread_mutex_unlock(&g_ip_set_lock);

		char mac[OVFS_MAC_STRING_LEN]={0};
		NetWorkTool_GetMacAddr((char *)ethcfg.if_name,mac);
		Common_cJSON_AddStringToObject(out,"EthName",ethcfg.if_name);
		Common_cJSON_AddNumberToObject(out,"EnableDhcp",ethcfg.dhcp);
		Common_cJSON_AddNumberToObject(out,"SupportIpV6",ethcfg.ipv6_support);
		Common_cJSON_AddNumberToObject(out,"EnableDhcpV6",ethcfg.dhcpv6);
		Common_cJSON_AddStringToObject(out,"IpAddrV4",ethcfg.ip.ipv4);
		Common_cJSON_AddStringToObject(out,"IpAddrV6",ethcfg.ip.ipv6);
		Common_cJSON_AddStringToObject(out,"IpMaskV4",ethcfg.netmask.ipv4);
		Common_cJSON_AddStringToObject(out,"IpMaskV6",ethcfg.netmask.ipv6);
		Common_cJSON_AddNumberToObject(out,"IpV6PrefixLen",ethcfg.ipv6_prefixlen);
		Common_cJSON_AddStringToObject(out,"GatewayV4",ethcfg.gate_way.ipv4);
		Common_cJSON_AddStringToObject(out,"GatewayV6",ethcfg.gate_way.ipv6);
		//Common_cJSON_AddStringToObject(out,"MacAddr",real_ethcfg.mac);//获取的MAC地址返回实际的
		Common_cJSON_AddStringToObject(out,"MacAddr",mac);
	}

	return 0;
}

static int NetWork_Get_WifiConfig(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    return NetWork_WifiConfig(0, uriString, in, out);
}

static int NetWork_Put_WifiConfig(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    return NetWork_WifiConfig(1, uriString, in, out);
}

static int NetWork_Del_WifiConfig(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    return NetWork_WifiConfig(3, uriString, in, out);
}


#if 0
static int NetWork_Get_Eth_Cfg(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;
	unsigned int idx = 0;
    const ovfs_network_capability *m_network_capability = NULL;
	ovfs_netcard_config ethcfg;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	LOGI("[%s:%d]:%s\n",__FUNCTION__, __LINE__, uriString);

	idx = atoi(uriString+strlen("/Network/NetAttr/Eth/"));

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}

    if (idx < 0 || idx > m_network_capability->total_eth_num - 1)
    {
        return -1;
	}
	else
	{
	    memset(&ethcfg,0,sizeof(ethcfg));
        ret = ovfs_network_get_local_eth_cfg(m_network_capability->eth_status[idx].if_name,&ethcfg);
		if (0 != ret)
		{
            return -1;
		}

		Common_cJSON_AddStringToObject(out,"EthName",ethcfg.if_name);
		Common_cJSON_AddNumberToObject(out,"EnableDhcp",ethcfg.dhcp);
		Common_cJSON_AddStringToObject(out,"MacAddr",ethcfg.mac);

        if (OVFS_TRUE == ethcfg.dhcp)
        {
		    memset(&ethcfg,0,sizeof(ethcfg));
	        ret = ovfs_cfgm_get_local_eth_cfg(m_network_capability->eth_status[idx].if_name,&ethcfg);
			if (0 != ret)
			{
	            return -1;
			}

			Common_cJSON_AddStringToObject(out,"IpAddrV4",ethcfg.ip.ipv4);
			Common_cJSON_AddStringToObject(out,"IpAddrV6",ethcfg.ip.ipv6);
			Common_cJSON_AddStringToObject(out,"IpMaskV4",ethcfg.netmask.ipv4);
			Common_cJSON_AddStringToObject(out,"IpMaskV6",ethcfg.netmask.ipv6);
			Common_cJSON_AddStringToObject(out,"GatewayV4",ethcfg.gate_way.ipv4);
			Common_cJSON_AddStringToObject(out,"GatewayV6",ethcfg.gate_way.ipv6);
        }
		else
		{
			Common_cJSON_AddStringToObject(out,"IpAddrV4",ethcfg.ip.ipv4);
			Common_cJSON_AddStringToObject(out,"IpAddrV6",ethcfg.ip.ipv6);
			Common_cJSON_AddStringToObject(out,"IpMaskV4",ethcfg.netmask.ipv4);
			Common_cJSON_AddStringToObject(out,"IpMaskV6",ethcfg.netmask.ipv6);
			Common_cJSON_AddStringToObject(out,"GatewayV4",ethcfg.gate_way.ipv4);
			Common_cJSON_AddStringToObject(out,"GatewayV6",ethcfg.gate_way.ipv6);
		}
	}

	return 0;
}
#endif
static int NetWork_Put_Eth(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;
	unsigned int idx = 0;
    const ovfs_network_capability *m_network_capability = NULL;
	ovfs_netcard_config ethcfg;
	ovfs_netcard_config ethcfgSetting;
	char *pStringValue;
	S32 nIntValue;
	cJSON_Struct* parentItem = NULL;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	LOGI("[%s:%d]:%s\n",__FUNCTION__, __LINE__, uriString);

	idx = atoi(uriString+strlen("/Network/NetAttr/Eth/"));

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}

    if (idx < 0 || idx > m_network_capability->total_eth_num - 1)
    {
        return -1;
	}
	else
	{
		pthread_mutex_lock(&g_ip_set_lock);

	    memset(&ethcfg,0,sizeof(ovfs_netcard_config));
        //ret = ovfs_network_get_local_eth_cfg(m_network_capability->eth_status[idx].if_name,&ethcfg);
		ret = ovfs_cfgm_get_local_eth_cfg(m_network_capability->eth_status[idx].if_name,&ethcfg);
		if (0 == ret)
		{
			memcpy(&ethcfgSetting,&ethcfg,sizeof(ovfs_netcard_config));

		    parentItem = (cJSON_Struct*)in;

			pStringValue = NULL;
			Common_Json_GetAttrValue(parentItem,-1,"EthName",NULL,&pStringValue,NULL,NULL);
			if (NULL != pStringValue && 0 == Common_StrCmp(pStringValue,ethcfg.if_name))
			{
				nIntValue = -1;
				Common_Json_GetAttrValue(parentItem,-1,"EnableDhcp",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
						ethcfgSetting.dhcp = (OVFS_BOOL)nIntValue;
				}

                nIntValue = -1;
				Common_Json_GetAttrValue(parentItem,-1,"EnableDhcpV6",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
						ethcfgSetting.dhcpv6 = nIntValue;
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(parentItem,-1,"IpAddrV4",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
						Common_Strncpy(ethcfgSetting.ip.ipv4,pStringValue,sizeof(ethcfgSetting.ip.ipv4));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(parentItem,-1,"IpAddrV6",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
						Common_Strncpy(ethcfgSetting.ip.ipv6,pStringValue,sizeof(ethcfgSetting.ip.ipv6));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(parentItem,-1,"IpMaskV4",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
						Common_Strncpy(ethcfgSetting.netmask.ipv4,pStringValue,sizeof(ethcfgSetting.netmask.ipv4));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(parentItem,-1,"IpMaskV6",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
						Common_Strncpy(ethcfgSetting.netmask.ipv6,pStringValue,sizeof(ethcfgSetting.netmask.ipv6));
				}

                nIntValue = -1;
				Common_Json_GetAttrValue(parentItem,-1,"IpV6PrefixLen",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
						ethcfgSetting.ipv6_prefixlen = (OVFS_BOOL)nIntValue;
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(parentItem,-1,"GatewayV4",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
						Common_Strncpy(ethcfgSetting.gate_way.ipv4,pStringValue,sizeof(ethcfgSetting.gate_way.ipv4));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(parentItem,-1,"GatewayV6",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
						Common_Strncpy(ethcfgSetting.gate_way.ipv6,pStringValue,sizeof(ethcfgSetting.gate_way.ipv6));
				}

				if(0 != memcmp(&ethcfgSetting,&ethcfg,sizeof(ovfs_netcard_config)))
				{
			        ret = ovfs_network_set_netcard_cfg(&ethcfgSetting);
					if (0 != ret)
					{
						pthread_mutex_unlock(&g_ip_set_lock);
			            return -1;
					}

					if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_set_local_eth_cfg(&ethcfgSetting, 0))
					{
						pthread_mutex_unlock(&g_ip_set_lock);
						return OVFS_ERR_NETWORK_BASE;
					}

					NetWork_SaveCfg();
				}
			}
		}

		pthread_mutex_unlock(&g_ip_set_lock);

	}

	return 0;
}

static int NetWork_Get_DefaultIFInfo(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    char mul_name[32];
	int net_type = 0;
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&mul_name,0,sizeof(mul_name));
	ret = ovfs_network_get_default_if(mul_name,sizeof(mul_name));
	if (0 != ret)
	{
		LOGE("ovfs_network_get_default_if fail.\n");
		return -1;
	}

	Common_cJSON_AddStringToObject(out,"DefaultRoute",mul_name);

	if(Common_StrnCmp(mul_name, (char *)"eth", 3) == 0)
	{
		net_type = 1;
	}
	else if(Common_StrnCmp(mul_name, (char *)"wlan", 4) == 0)
	{
		net_type = 2;
	}
	else if((Common_StrnCmp(mul_name, (char *)"ppp", 3) == 0) || (Common_StrnCmp(mul_name, (char *)"usb", 3) == 0))
	{
		if(access("/tmp/lte", F_OK) == 0)
		{
			net_type = 3;
		}
		else
		{
			net_type = 1;
		}
	}

	Common_cJSON_AddNumberToObject(out,"NetType",net_type);

	return 0;
}

static int NetWork_Put_DefaultIFInfo(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    char mul_name[32];
	char *pStringValue;
	int ret = -1;
	cJSON_Struct* parentItem = NULL;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&mul_name,0,sizeof(mul_name));
	ret = ovfs_network_get_default_if(mul_name,sizeof(mul_name));
	if (0 != ret)
	{
		LOGE("ovfs_network_get_default_if fail.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"DefaultRoute",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(mul_name,pStringValue,sizeof(mul_name));
	}

	ret = ovfs_network_set_default_if(mul_name);
	if (0 != ret)
	{
		LOGE("ovfs_network_set_default_if fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_DNS(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    ovfs_local_net_dns dns;
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&dns,0,sizeof(dns));
	ret = ovfs_cfgm_get_dns_cfg(&dns);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_dns_cfg fail.\n");
		return -1;
	}

	Common_cJSON_AddNumberToObject(out,"AutoDNS",dns.auto_dns);
	Common_cJSON_AddStringToObject(out,"Dns1V4",dns.dns1.ipv4);
	Common_cJSON_AddStringToObject(out,"Dns1V6",dns.dns1.ipv6);
	Common_cJSON_AddStringToObject(out,"Dns2V4",dns.dns2.ipv4);
	Common_cJSON_AddStringToObject(out,"Dns2V6",dns.dns2.ipv6);

	return 0;
}
#if 0
static int NetWork_Get_DNS_Cfg(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    ovfs_local_net_dns dns;
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&dns,0,sizeof(dns));
	ret = ovfs_cfgm_get_dns_cfg(&dns);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_dns_cfg fail.\n");
		return -1;
	}

	Common_cJSON_AddNumberToObject(out,"AutoDNS",dns.auto_dns);
	Common_cJSON_AddStringToObject(out,"Dns1V4",dns.dns1.ipv4);
	Common_cJSON_AddStringToObject(out,"Dns1V6",dns.dns1.ipv6);
	Common_cJSON_AddStringToObject(out,"Dns2V4",dns.dns2.ipv4);
	Common_cJSON_AddStringToObject(out,"Dns2V6",dns.dns2.ipv6);

	return 0;
}
#endif
static int NetWork_Put_DNS(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    ovfs_local_net_dns dns;
	char *pStringValue;
	S32 nIntValue;
	int ret = -1;
	cJSON_Struct* parentItem = NULL;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&dns,0,sizeof(dns));
	ret = ovfs_cfgm_get_dns_cfg(&dns);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_dns_cfg fail.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"AutoDNS",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		dns.auto_dns = (OVFS_BOOL)nIntValue;
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Dns1V4",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(dns.dns1.ipv4,pStringValue,sizeof(dns.dns1.ipv4));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Dns1V6",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(dns.dns1.ipv6,pStringValue,sizeof(dns.dns1.ipv6));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Dns2V4",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(dns.dns2.ipv4,pStringValue,sizeof(dns.dns2.ipv4));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Dns2V6",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(dns.dns2.ipv6,pStringValue,sizeof(dns.dns2.ipv6));
	}

	pthread_mutex_lock(&g_dns_set_lock);
	ret = ovfs_network_set_dns_cfg(&dns);
	if (0 != ret)
	{
		LOGE("ovfs_network_set_dns_cfg fail.\n");
		pthread_mutex_unlock(&g_dns_set_lock);
		return -1;
	}
	if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_set_dns_cfg(&dns))
	{
		pthread_mutex_unlock(&g_dns_set_lock);
		return -2;
	}

	NetWork_SaveCfg();

	pthread_mutex_unlock(&g_dns_set_lock);

	return 0;
}

static int NetWork_Get_PPPOE(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    ovfs_pppoe_config pppoe;
	ovfs_pppoe_status status;
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&pppoe,0,sizeof(pppoe));
	ret = ovfs_cfgm_get_pppoe_cfg(&pppoe);
	if (0 != ret)
	{
		LOGE("ovfs_cfgm_get_pppoe_cfg fail.\n");
		return -1;
	}

    memset(&status,0,sizeof(status));
	ret = ovfs_network_get_pppoe_status(&status);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_pppoe_status fail.\n");
		return -1;
	}
    LOGI("NetWork_Get_PPPOE %d %s!!!!!!!!!!!!!! \n",status.pppoe_ok,status.ip.ipv4);
	Common_cJSON_AddNumberToObject(out,"Enable",pppoe.enable);
	Common_cJSON_AddStringToObject(out,"User",pppoe.user_name);
	Common_cJSON_AddStringToObject(out,"Password",pppoe.password);
	Common_cJSON_AddStringToObject(out,"IpV4",status.ip.ipv4);
	Common_cJSON_AddStringToObject(out,"IpV6",status.ip.ipv6);

	return 0;
}

static int NetWork_Put_PPPOE(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    ovfs_pppoe_config pppoe;
	char *pStringValue;
	S32 nIntValue;
	int ret = -1;
	cJSON_Struct* parentItem = NULL;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&pppoe,0,sizeof(pppoe));
	ret = ovfs_cfgm_get_pppoe_cfg(&pppoe);
	if (0 != ret)
	{
		LOGE("ovfs_cfgm_get_pppoe_cfg fail.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		pppoe.enable = (OVFS_BOOL)nIntValue;
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"User",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(pppoe.user_name,pStringValue,sizeof(pppoe.user_name));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"Password",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		Common_Strncpy(pppoe.password,pStringValue,sizeof(pppoe.password));
	}

	ret = ovfs_network_set_pppoe_config(&pppoe);
	if (0 != ret)
	{
		LOGE("ovfs_network_set_pppoe_cfg fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_UPNP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    OVFS_BOOL enable;
	int ret = -1;
	S32 i = 0, index = 0;
	ovfs_upnp_port_detail_info m_port_detail_info;
    cJSON_Struct *pNode = NULL;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = ovfs_network_get_upnp_state(&enable);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_upnp_state fail.\n");
		return -1;
	}

	Common_cJSON_AddNumberToObject(out,"Enable",enable);
    pNode = Common_Json_SetAttrValue(out,-1,"Map",Common_Json_Type_Array,NULL,0,0);

    for (i = 0; i < OVFS_PORT_MAX; i++)
    {
    	COMMON_CLR_ARG(m_port_detail_info);
		ovfs_network_get_upnp_port_info(i, &m_port_detail_info);

		if (m_port_detail_info.internal_port != 0)
		{
			Common_Json_SetAttrValue(pNode,index,"Enable",Common_Json_Type_Number,NULL,m_port_detail_info.enable_upnp,0);
			Common_Json_SetAttrValue(pNode,index,"InPort",Common_Json_Type_Number,NULL,m_port_detail_info.internal_port,0);
			Common_Json_SetAttrValue(pNode,index,"OutPort",Common_Json_Type_Number,NULL,m_port_detail_info.external_port,0);
            Common_Json_SetAttrValue(pNode,index,"TcpOrUdp",Common_Json_Type_Number,NULL,m_port_detail_info.port_protocol,0);
			Common_Json_SetAttrValue(pNode,index,"Eth",Common_Json_Type_String,m_port_detail_info.eth_name,0,0);
			Common_Json_SetAttrValue(pNode,index,"Status",Common_Json_Type_Number,NULL,m_port_detail_info.mapping_success,0);
			Common_Json_SetAttrValue(pNode,index,"OutIP",Common_Json_Type_String,m_port_detail_info.externalIP,0,0);
            index++;
		}
    }

	return 0;
}

static int NetWork_Get_UPNP_Cfg(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    OVFS_BOOL enable;
	int ret = -1;
	S32 i = 0, index = 0;
	ovfs_upnp_port_detail_info m_port_detail_info;
    cJSON_Struct *pNode = NULL;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = ovfs_network_get_upnp_state(&enable);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_upnp_state fail.\n");
		return -1;
	}

	Common_cJSON_AddNumberToObject(out,"Enable",enable);
    pNode = Common_Json_SetAttrValue(out,-1,"Map",Common_Json_Type_Array,NULL,0,0);

    for (i = 0; i < OVFS_PORT_MAX; i++)
    {
        COMMON_CLR_ARG(m_port_detail_info);
	    ovfs_network_get_upnp_port_info(i, &m_port_detail_info);

		if (m_port_detail_info.internal_port != 0)
		{
			Common_Json_SetAttrValue(pNode,index,"Enable",Common_Json_Type_Number,NULL,m_port_detail_info.enable_upnp,0);
			Common_Json_SetAttrValue(pNode,index,"InPort",Common_Json_Type_Number,NULL,m_port_detail_info.internal_port,0);
			Common_Json_SetAttrValue(pNode,index,"OutPort",Common_Json_Type_Number,NULL,m_port_detail_info.external_port,0);
            Common_Json_SetAttrValue(pNode,index,"TcpOrUdp",Common_Json_Type_Number,NULL,m_port_detail_info.port_protocol,0);
			Common_Json_SetAttrValue(pNode,index,"Eth",Common_Json_Type_String,m_port_detail_info.eth_name,0,0);
            index++;
		}
    }

	return 0;
}

static int NetWork_Put_UPNP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    OVFS_BOOL enable;
	S32 nIntValue;
	S8 *pStringValue;
	int ret = -1, num = 0, i = 0;
	cJSON_Struct* parentItem = NULL;
	cJSON_Struct *pJsonTmp = NULL;
	ovfs_upnp_port_detail_info m_port_detail_info;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = ovfs_network_get_upnp_state(&enable);
	if (0 != ret)
	{
		LOGE("ovfs_network_get_upnp_state fail.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		enable = (OVFS_BOOL)nIntValue;
	}

	ret = ovfs_network_set_upnp_state(enable);
	if (0 != ret)
	{
		LOGE("ovfs_network_set_upnp_state fail.\n");
		return -1;
	}

	pJsonTmp = Common_Json_GetItem(parentItem,-1,"Map");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i < num; i++)
			{
			   	COMMON_CLR_ARG(m_port_detail_info);

			    nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"Enable",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.enable_upnp = (OVFS_BOOL)nIntValue;
				}

			    nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"InPort",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.internal_port = nIntValue;
				}

			    nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"OutPort",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.external_port = nIntValue;
				}

			    nIntValue = -1;
				Common_Json_GetAttrValue(pJsonTmp,i,"TcpOrUdp",NULL,NULL,&nIntValue,NULL);
				if (-1 != nIntValue)
				{
					m_port_detail_info.port_protocol = (ovfs_upnp_port_protocol)nIntValue;
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Eth",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(m_port_detail_info.eth_name,pStringValue,sizeof(m_port_detail_info.eth_name));
				}

				if (m_port_detail_info.internal_port != 0)
				{
                    ovfs_network_set_upnp_port_info(m_port_detail_info.enable_upnp,m_port_detail_info.internal_port,m_port_detail_info.external_port,
						                            m_port_detail_info.port_protocol,m_port_detail_info.eth_name);
				}
			}
		}
	}

	return 0;
}

static int NetWork_Get_HTTP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_HTTP_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_HTTP_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_HTTP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_HTTP_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_HTTP_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_DoHTTP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_DoHTTP_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_DoHTTP_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_Ftp(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_Ftp_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_Ftp_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_Ftp(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_Ftp_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_Ftp_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_DoFtp(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_DoFtp_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_DoFtp_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_TransData(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_TransData_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_Ftp_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_TransData(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_TransData_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_Ftp_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_TransDataStart(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;
    int status = 0;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    Common_Json_GetAttrValueInt(in, "Start", &status);

	ret = NetWork_TransData_Start(status);
	if (0 != ret)
	{
		LOGE("NetWork_Put_TransDataStart fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_DoTransData(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_DoTransData_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_DoTransData_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_Telnetd(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_Telnetd_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_Telnetd_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_Telnetd(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_Telnetd_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_Telnetd_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_SNMP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_SNMP_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_SNMP_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_SNMP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_SNMP_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_SNMP_Json fail.\n");
		return -1;
	}

	return 0;
}

#if 0
static int NetWork_Config_STAConnect(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = 0;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Config_STA_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Config_STA_Json fail.\n");
		return -2;
	}

	return ret;
}

static int NetWork_Get_WifiWorkStatus(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_WifiWorkStatus_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_WifiWorkStatus_Json fail. ret=%d\n",ret);
		return -1;
	}

	return 0;
}

static int NetWork_Put_WifiWorkStatus(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_WifiWorkStatus_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_WifiWorkStatus_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_WifiSTAList(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_WifiSTAList_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_WifiSTAList fail.ret=%d\n",ret);
		return -1;
	}

	return 0;
}

static int NetWork_Get_WifiAPList(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_WifiAPList_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_WifiAPList_Json fail.ret=%d\n",ret);
		return -1;
	}

	return 0;
}

static int NetWork_Get_WifiScanList(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_WifiScanList_Json((cJSON_Struct*)out);
	if (ret < 0)
	{
		LOGE("NetWork_Get_WifiScanList_Json fail.ret=%d\n",ret);
		return -1;
	}
	else if(ret > 0)
	{
		LOGW("WifiScanList is NULL.ret=%d\n",ret);
	}

	return 0;
}

static int NetWork_Put_AddAP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_AddAP_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_AddAP_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_DelAP(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_DelAP_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_DelAP fail.\n");
		return -1;
	}

	return 0;
}
#endif

static int NetWork_Get_LTECfg(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_LTECfg_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_LTECfg_Json fail.ret=%d\n",ret);
		return ret;
	}

	return 0;
}

static int NetWork_Put_LTECfg(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Put_LTECfg_Json((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Put_LTECfg_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Get_4GCustomer(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_4GCustomer_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_4GCustomer_Json fail.ret=%d\n",ret);
		return ret;
	}

	return 0;
}

static int NetWork_Put_4GSpecified(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = Network_Put_4GSpecified((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("Network_Put_4GSpecified fail.ret=%d\n",ret);
		return ret;
	}

	return 0;
}

static int NetWork_Put_SpecifiedCard(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = Network_Put_SpecifiedCard((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("Network_Put_SpecifiedCard fail.ret=%d\n",ret);
		return ret;
	}

	return 0;
}

static int NetWork_Put_4GBind(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Bind_Ccid((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Bind_Ccid fail.ret=%d\n",ret);
		return ret;
	}

	return 0;
}

static int NetWork_Put_4GUnbind(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Unbind_Ccid((cJSON_Struct*)in);
	if (0 != ret)
	{
		LOGE("NetWork_Unbind_Ccid fail.ret=%d\n",ret);
		return ret;
	}

	return 0;
}

static int NetWork_Get_Mail(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	OVFS_BOOL enable_attach = OVFS_FALSE;
    U32 receiver_num = 0, i = 0;
	ovfs_email_sender_cfg sender_cfg;
	ovfs_email_receiver_cfg receiver_cfg;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

    memset(&sender_cfg,0,sizeof(sender_cfg));
	ret = ovfs_cfgm_get_email_sender(&sender_cfg);
	if (0 != ret)
	{
		LOGE("ovfs_cfgm_get_email_sender fail.\n");
	}
	else
	{
		Common_cJSON_AddNumberToObject(out,"Enable",sender_cfg.enable);

		tmp = Common_cJSON_CreateObject();
		Common_cJSON_AddStringToObject(tmp,"User",sender_cfg.user_name);
		Common_cJSON_AddStringToObject(tmp,"Password",sender_cfg.password);
		Common_cJSON_AddStringToObject(tmp,"SmtpServer",sender_cfg.server_addr);
		Common_cJSON_AddNumberToObject(tmp,"SmtpPort",sender_cfg.server_port);
		Common_cJSON_AddNumberToObject(tmp,"EnableSsl",sender_cfg.ssl_enable);
		Common_cJSON_AddItemToObject(out,"Sender",tmp);
	}

	ret = ovfs_cfgm_get_email_attachment_status(&enable_attach);
	if (0 != ret)
	{
		LOGE("ovfs_cfgm_get_email_attachment_status fail.\n");
	}
	else
	{
		Common_cJSON_AddNumberToObject(out,"Attachment",enable_attach);
	}

    ovfs_cfgm_get_email_receiver_num(&receiver_num);
	if (receiver_num > 0)
	{
	    tmp_array = Common_cJSON_CreateArray();
		if (NULL != tmp_array)
		{
		    Common_cJSON_AddItemToObject(out,"Receiver",tmp_array);
			for (i = 0; i < receiver_num; i++)
			{
			    memset(&receiver_cfg,0,sizeof(receiver_cfg));
				ovfs_cfgm_get_email_receiver(i,&receiver_cfg);

				tmp = Common_cJSON_CreateObject();
				Common_cJSON_AddStringToObject(tmp,"Name",receiver_cfg.name);
				Common_cJSON_AddStringToObject(tmp,"Addr",receiver_cfg.addr);
				Common_cJSON_AddItemToArray(tmp_array,tmp);
			}
		}
	}

	return 0;
}

static int NetWork_Put_Mail(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	U32 i = 0;
	U32 receiver_num = 0, num = 0;
	OVFS_BOOL enable_attach;
	int ret = 0;
	ovfs_email_sender_cfg sender_cfg;
	ovfs_email_receiver_cfg receiver_cfg[OVFS_MAX_EMAIL_RECIEVER_NUM];
	char *pStringValue;
	S32 nIntValue;
	cJSON_Struct *parentItem = NULL;
	cJSON_Struct *pJsonTmp = NULL;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

    memset(&sender_cfg,0,sizeof(sender_cfg));
	ret = ovfs_cfgm_get_email_sender(&sender_cfg);
	if (0 != ret)
	{
		LOGE("ovfs_cfgm_get_email_sender fail.\n");
	}
	else
	{
	    nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.enable = (OVFS_BOOL)nIntValue;
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/User",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.user_name,pStringValue,sizeof(sender_cfg.user_name));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/Password",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.password,pStringValue,sizeof(sender_cfg.password));
		}

		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/SmtpServer",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
		{
			Common_Strncpy(sender_cfg.server_addr,pStringValue,sizeof(sender_cfg.server_addr));
		}

		nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/SmtpPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.server_port = nIntValue;
		}

		nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Sender/EnableSsl",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			sender_cfg.ssl_enable = (OVFS_BOOL)nIntValue;
		}

		ret = ovfs_network_set_email_sender(&sender_cfg);
		if (0 != ret)
		{
	        LOGE("%s: ovfs_network_set_email_sender fail!\n",__FUNCTION__);
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Attachment",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		enable_attach = (OVFS_BOOL)nIntValue;
		ovfs_cfgm_set_email_attachment_status(enable_attach);
	}

	memset(receiver_cfg,0,sizeof(receiver_cfg));
	pJsonTmp = Common_Json_GetItem(parentItem,-1,"Receiver");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<OVFS_ARRAY_DIM(receiver_cfg) && i<num; i++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Name",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].name,pStringValue,sizeof(receiver_cfg[i].name));
				}

				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"Addr",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					Common_Strncpy(receiver_cfg[i].addr,pStringValue,sizeof(receiver_cfg[i].addr));
				}

				receiver_num++;
			}

			ret = ovfs_network_set_email_receiver(receiver_cfg, receiver_num);
			if (0 != ret)
			{
		        LOGE("%s: ovfs_network_set_email_receiver fail!\n",__FUNCTION__);
			}
		}
	}

	return 0;
}

static int NetWork_Get_DDNS(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	OVFS_BOOL enable = OVFS_FALSE;
    U32 i = 0;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
	int ret = -1;
	ovfs_ddns_info ddns_info;
	ovfs_ddns_ability ddns_ability;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ovfs_cfgm_get_ddns_start(&enable);
	Common_cJSON_AddNumberToObject(out,"Enable",enable);

    memset(&ddns_ability,0,sizeof(ddns_ability));
	ovfs_network_get_ddns_cap(&ddns_ability);
	LOGD("ddns_ability.node_num=%d\n",ddns_ability.node_num);
	if (ddns_ability.node_num > 0)
	{
	    tmp_array = Common_cJSON_CreateArray();
		if (NULL != tmp_array)
		{
		    Common_cJSON_AddItemToObject(out,"DDNS",tmp_array);
	        for (i = 0;i < ddns_ability.node_num;i++)
	        {
			    memset(&ddns_info,0,sizeof(ddns_info));
				ret = ovfs_cfgm_get_ddns_info(ddns_ability.ddns_cap[i].ddns_name,&ddns_info);
				if (0 != ret)
				{
					LOGE("ovfs_cfgm_get_email_sender fail.\n");
				}
				else
				{
					tmp = Common_cJSON_CreateObject();
					Common_cJSON_AddNumberToObject(tmp,"Enable",ddns_info.enable);
					Common_cJSON_AddStringToObject(tmp,"DdnsProtoName",ddns_info.param.ddns_name);
					Common_cJSON_AddStringToObject(tmp,"User",ddns_info.param.usr_name);
					Common_cJSON_AddStringToObject(tmp,"Password",ddns_info.param.passwd);
					Common_cJSON_AddStringToObject(tmp,"DomainName",ddns_info.param.domain_host);
					Common_cJSON_AddStringToObject(tmp,"ServerName",ddns_info.param.server_host);
					Common_cJSON_AddNumberToObject(tmp,"DdnsPort",ddns_info.param.server_port);
					Common_cJSON_AddNumberToObject(tmp,"CheckIpIntervalTime",ddns_info.update_min);
					Common_cJSON_AddNumberToObject(tmp,"UpdateIpIntervalTime",ddns_info.check_min);
					Common_cJSON_AddItemToArray(tmp_array,tmp);
				}
			}
		}
	}

	return 0;
}

static int NetWork_Put_DDNS(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	U32 i = 0, num = 0;
	int ret = 0;
	char *pStringValue;
	S32 nIntValue;
	cJSON_Struct *parentItem = NULL;
	cJSON_Struct *pJsonTmp = NULL;
	ovfs_ddns_info ddns_info;

	if (NULL == in || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	parentItem = (cJSON_Struct*)in;

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"Enable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		ovfs_network_start_ddns((OVFS_BOOL)nIntValue);
	}

	pJsonTmp = Common_Json_GetItem(parentItem,-1,"DDNS");
	if (NULL != pJsonTmp)
	{
        num = Common_Json_Size(pJsonTmp);
		if (num > 0)
		{
			for (i = 0; i<num; i++)
			{
				pStringValue = NULL;
				Common_Json_GetAttrValue(pJsonTmp,i,"DdnsProtoName",NULL,&pStringValue,NULL,NULL);
				if (NULL != pStringValue)
				{
					ret = ovfs_cfgm_get_ddns_info(pStringValue, &ddns_info);
				    if (0 == ret)
					{
						nIntValue = -1;
						Common_Json_GetAttrValue(pJsonTmp,i,"Enable",NULL,NULL,&nIntValue,NULL);
						if (-1 != nIntValue)
						{
							ovfs_network_set_ddns_enable(ddns_info.param.ddns_name, (OVFS_BOOL)nIntValue);
						}

						nIntValue = -1;
						Common_Json_GetAttrValue(pJsonTmp,i,"UpdateIpIntervalTime",NULL,NULL,&nIntValue,NULL);
						if (-1 != nIntValue)
						{
							ovfs_cfgm_set_ddns_update_interval(ddns_info.param.ddns_name, nIntValue);
						}

						nIntValue = -1;
						Common_Json_GetAttrValue(pJsonTmp,i,"CheckIpIntervalTime",NULL,NULL,&nIntValue,NULL);
						if (-1 != nIntValue)
						{
							ovfs_cfgm_set_ddns_check_interval(ddns_info.param.ddns_name, nIntValue);
						}

						nIntValue = -1;
						Common_Json_GetAttrValue(pJsonTmp,i,"DdnsPort",NULL,NULL,&nIntValue,NULL);
						if (-1 != nIntValue)
						{
							ddns_info.param.server_port = nIntValue;
						}

						pStringValue = NULL;
						Common_Json_GetAttrValue(pJsonTmp,i,"User",NULL,&pStringValue,NULL,NULL);
						if (NULL != pStringValue)
						{
							Common_Strncpy(ddns_info.param.usr_name,pStringValue,sizeof(ddns_info.param.usr_name));
						}

						pStringValue = NULL;
						Common_Json_GetAttrValue(pJsonTmp,i,"Password",NULL,&pStringValue,NULL,NULL);
						if (NULL != pStringValue)
						{
							Common_Strncpy(ddns_info.param.passwd,pStringValue,sizeof(ddns_info.param.passwd));
						}

						pStringValue = NULL;
						Common_Json_GetAttrValue(pJsonTmp,i,"DomainName",NULL,&pStringValue,NULL,NULL);
						if (NULL != pStringValue)
						{
							Common_Strncpy(ddns_info.param.domain_host,pStringValue,sizeof(ddns_info.param.domain_host));
						}

						pStringValue = NULL;
						Common_Json_GetAttrValue(pJsonTmp,i,"ServerName",NULL,&pStringValue,NULL,NULL);
						if (NULL != pStringValue)
						{
							Common_Strncpy(ddns_info.param.server_host,pStringValue,sizeof(ddns_info.param.server_host));
						}

						ret = ovfs_network_set_ddns_cfg(&(ddns_info.param));
					}
				}
			}
		}
	}

	return 0;
}

static int NetWork_Get_NetOut(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    U32 i = 0;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
    const ovfs_network_capability *m_network_capability = NULL;
	int net_out = 0;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -2;
	}

    tmp_array = Common_cJSON_CreateArray();
	if (NULL != tmp_array)
	{
	    Common_cJSON_AddItemToObject(out,"NetOut",tmp_array);
        for (i = 0;i < m_network_capability->total_eth_num;i++)
        {
			net_out = ovfs_arptool_check_net_out(m_network_capability->eth_status[i].if_name);

			tmp = Common_cJSON_CreateObject();
			Common_cJSON_AddStringToObject(tmp,"EthName",m_network_capability->eth_status[i].if_name);
			Common_cJSON_AddNumberToObject(tmp,"NetOut",net_out);
			Common_cJSON_AddItemToArray(tmp_array,tmp);
		}
	}


	return 0;
}

static int NetWork_Get_NetIpConflict(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    U32 i = 0;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
    const ovfs_network_capability *m_network_capability = NULL;
	int ip_conflict = 0;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}

    tmp_array = Common_cJSON_CreateArray();
	if (NULL != tmp_array)
	{
	    Common_cJSON_AddItemToObject(out,"NetIpConflict",tmp_array);
        for (i = 0;i < m_network_capability->total_eth_num;i++)
        {
			ip_conflict = ovfs_arptool_check_local_IP_conflict(m_network_capability->eth_status[i].if_name);

			tmp = Common_cJSON_CreateObject();
			Common_cJSON_AddStringToObject(tmp,"EthName",m_network_capability->eth_status[i].if_name);
			Common_cJSON_AddNumberToObject(tmp,"NetIpConflict",ip_conflict);
			Common_cJSON_AddItemToArray(tmp_array,tmp);
		}

		S8 if_name[16] = "wlan0";
		ip_conflict = ovfs_arptool_check_local_IP_conflict(if_name);
		tmp = Common_cJSON_CreateObject();
		Common_cJSON_AddStringToObject(tmp,"EthName",if_name);
		Common_cJSON_AddNumberToObject(tmp,"NetIpConflict",ip_conflict);
		Common_cJSON_AddItemToArray(tmp_array,tmp);
	}

	return 0;
}

static int NetWork_Get_NetTraffic(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    U32 i = 0;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
    const ovfs_network_capability *m_network_capability = NULL;
	U32 upload = 0, download = 0;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}

    tmp_array = Common_cJSON_CreateArray();
	if (NULL != tmp_array)
	{
	    Common_cJSON_AddItemToObject(out,"NetTraffic",tmp_array);
        for (i = 0;i < m_network_capability->total_eth_num;i++)
        {
            upload = 0;
			download = 0;
			ovfs_network_get_special_traffic(m_network_capability->eth_status[i].if_name,&upload,&download);

			tmp = Common_cJSON_CreateObject();
			Common_cJSON_AddStringToObject(tmp,"EthName",m_network_capability->eth_status[i].if_name);
			Common_cJSON_AddNumberToObject(tmp,"UpLoad",upload);
			Common_cJSON_AddNumberToObject(tmp,"DownLoad",download);
			Common_cJSON_AddItemToArray(tmp_array,tmp);
		}
	}

	return 0;
}

static int NetWork_Get_Ftp_Status(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	int ret = -1;

	if (NULL == out || NULL == uriString)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	ret = NetWork_Get_Ftp_Status_Json((cJSON_Struct*)out);
	if (0 != ret)
	{
		LOGE("NetWork_Get_Ftp_Json fail.\n");
		return -1;
	}

	return 0;
}

static int NetWork_Put_Restore(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    if (NULL == uriString)
    {
		LOGE("uriString is NULL.\n");
		return -1;
	}

	remove(NETWORK_CFG_PATH);

	return 1;
}

static int NetWork_SaveCfg_NetAttr(Common_cJSON_T* tmpNetAttr)
{
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp1 = NULL;
    const ovfs_network_capability *m_network_capability = NULL;
	char mul_name[32];
	U32 i = 0;

    if (NULL == tmpNetAttr)
    {
        return -1;
	}

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetAttr,"DefaultRoute",tmp);
	NetWork_Get_DefaultIFInfo("NetWork/NetAttr/DefaultRoute",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetAttr,"DNS",tmp);
	NetWork_Get_DNS("NetWork/NetAttr/DNS",NULL,NULL,NULL,tmp);
	//NetWork_Get_DNS_Cfg("NetWork/NetAttr/DNS",NULL,NULL,NULL,tmp);

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL != m_network_capability)
    {
	    for(i = 0; i < m_network_capability->total_eth_num; i++)
	    {
	        memset(mul_name,0,sizeof(mul_name));

			tmp = Common_cJSON_CreateObject();
		    Common_cJSON_AddItemToObject(tmpNetAttr,"Eth",tmp);

		    tmp1 = Common_cJSON_CreateObject();
			snprintf(mul_name,sizeof(mul_name),"%d",i);
		    Common_cJSON_AddItemToObject(tmp,mul_name,tmp1);

			snprintf(mul_name,sizeof(mul_name),"NetWork/NetAttr/Eth/%d",i);
			//NetWork_Get_Eth_Cfg(mul_name,NULL,NULL,NULL,tmp1);
			ovfs_netcard_config ethcfg;
		    memset(&ethcfg,0,sizeof(ethcfg));

	        int ret = ovfs_cfgm_get_local_eth_cfg(m_network_capability->eth_status[i].if_name,&ethcfg);
			if (0 == ret)
			{
				NetWorkTool_GetMacAddr((char *)m_network_capability->eth_status[i].if_name,ethcfg.mac);

				Common_cJSON_AddStringToObject(tmp1,"EthName",ethcfg.if_name);
				Common_cJSON_AddNumberToObject(tmp1,"EnableDhcp",ethcfg.dhcp);
				Common_cJSON_AddNumberToObject(tmp1,"EnableDhcpV6",ethcfg.dhcpv6);
				Common_cJSON_AddStringToObject(tmp1,"MacAddr",ethcfg.mac);

				Common_cJSON_AddStringToObject(tmp1,"IpAddrV4",ethcfg.ip.ipv4);
				Common_cJSON_AddStringToObject(tmp1,"IpAddrV6",ethcfg.ip.ipv6);
				Common_cJSON_AddStringToObject(tmp1,"IpMaskV4",ethcfg.netmask.ipv4);
				Common_cJSON_AddStringToObject(tmp1,"IpMaskV6",ethcfg.netmask.ipv6);
				Common_cJSON_AddNumberToObject(tmp1,"IpV6PrefixLen",ethcfg.ipv6_prefixlen);
				Common_cJSON_AddStringToObject(tmp1,"GatewayV4",ethcfg.gate_way.ipv4);
				Common_cJSON_AddStringToObject(tmp1,"GatewayV6",ethcfg.gate_way.ipv6);
			}
	    }
	}
#if 0
	if (OVFS_TRUE == s_wifi_enable)
	{
	    tmp = Common_cJSON_CreateObject();
	    Common_cJSON_AddItemToObject(tmpNetAttr,"Wifi",tmp);
		NetWork_Get_WifiWorkMode_Json((cJSON_Struct *)tmp);
	}
#endif
	if(access("/tmp/lte",F_OK) == 0)
	{
		tmp = Common_cJSON_CreateObject();
		Common_cJSON_AddItemToObject(tmpNetAttr,"LteCfg",tmp);
		ovfs_network_4g_Save((cJSON_Struct *)tmp);
	}

    return 0;
}

static int NetWork_SaveCfg_NetApp(Common_cJSON_T* tmpNetApp)
{
	Common_cJSON_T* tmp = NULL;

    if (NULL == tmpNetApp)
    {
        return -1;
	}

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"PPPoE",tmp);
	NetWork_Get_PPPOE("NetWork/NetApp/PPPoE",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"DDNS",tmp);
	NetWork_Get_DDNS("NetWork/NetApp/DDNS",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"UPnP",tmp);
	NetWork_Get_UPNP_Cfg("NetWork/NetApp/UPnP",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"Mail",tmp);
	NetWork_Get_Mail("NetWork/NetApp/Mail",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"Telnetd",tmp);
	NetWork_Get_Telnetd("NetWork/NetApp/Telnetd",NULL,NULL,NULL,tmp);

	tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"SNMP",tmp);
	NetWork_Get_SNMP("NetWork/NetApp/SNMP",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"Ftp",tmp);
	NetWork_Get_Ftp("NetWork/NetApp/Ftp",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"HTTP",tmp);
	NetWork_Get_HTTP("NetWork/NetApp/HTTP",NULL,NULL,NULL,tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(tmpNetApp,"TransData",tmp);
	NetWork_Get_TransData("NetWork/NetApp/TransData",NULL,NULL,NULL,tmp);

	return 0;
}

int NetWork_SaveCfg()
{
    Common_cJSON_T* saveObj = NULL;
	Common_cJSON_T* tmpNetAttr = NULL;
	Common_cJSON_T* tmpNetApp = NULL;

	saveObj = Common_cJSON_CreateObject();

	tmpNetAttr = Common_cJSON_CreateObject();
	Common_cJSON_AddItemToObject(saveObj,"NetAttr",tmpNetAttr);
	NetWork_SaveCfg_NetAttr(tmpNetAttr);

	tmpNetApp = Common_cJSON_CreateObject();
	Common_cJSON_AddItemToObject(saveObj,"NetApp",tmpNetApp);
	NetWork_SaveCfg_NetApp(tmpNetApp);

    Module_SaveConfig((NetWork_RestComm_GetModuleHdl()),(cJSON_Struct*)saveObj);
	{
#if 0
		char* out = NULL;
		printf("[NetWork] tree:=%s\n",out = Common_cJSON_Print(saveObj,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
#endif
	}

    Common_cJSON_Delete(saveObj);

	return 0;
}

int NetWork_Rest_NetAttr_init(Common_cJSON_T* jsonTree)
{
    const ovfs_network_capability *m_network_capability = NULL;
	char mul_name[32];
	U32 i = 0;

	NetWork_RestComm_AddPathToTree("/NetAttr",jsonTree,"NetAttr root URI. Get URI list on it.",
		                           "NetAttr",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/DefaultRoute",jsonTree,"DefaultRouteroot Info. Get or Put DefaultRouteroot Info on it.",
		                           "DefaultRoute",NetWork_Get_DefaultIFInfo,NetWork_Put_DefaultIFInfo,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/DNS",jsonTree,"DNS Info. Get or Put DNS Info on it.",
		                           "DNS",NetWork_Get_DNS,NetWork_Put_DNS,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/Eth",jsonTree,"Eth Eth URI. Get URI list on it.",
		                           "Eth",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/Lte4GCustomCfg",jsonTree,"LTE 4G URI. Get URI list on it.",
			                           "LTE",NetWork_Get_4GCustomer,NULL,NULL,NULL);

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL != m_network_capability)
    {
	    for(i = 0; i < m_network_capability->total_eth_num; i++)
	    {
	        memset(mul_name,0,sizeof(mul_name));

			snprintf(mul_name,sizeof(mul_name),"/NetAttr/Eth/%d",i);
			NetWork_RestComm_AddPathToTree(mul_name,jsonTree,"Eth Info. Get or Put Eth Info on it.",
				                           "Eth",NetWork_Get_Eth,NetWork_Put_Eth,NULL,NULL);
	    }
	}

#if 0
    if (OVFS_TRUE == s_wifi_enable)
    {
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi",jsonTree,"Wifi URI. Get URI list on it.",
			                           "Eth",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/ConfigSTA",jsonTree,"Config Wifi URI. Get URI list on it.",
			                           "ConfigSTA",NULL,NetWork_Config_STAConnect,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/WorkStatus",jsonTree,"WorkStatus URI. Get or Put WorkStatus on it.",
			                           "WorkStatus",NetWork_Get_WifiWorkStatus,NetWork_Put_WifiWorkStatus,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/STAList",jsonTree,"APList URI. Get STA list on it.",
			                           "STAList",NetWork_Get_WifiSTAList,NULL,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/APList",jsonTree,"APList URI. Get AP list on it.",
			                           "APList",NetWork_Get_WifiAPList,NULL,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/ScanList",jsonTree,"ScanList. Get Scan list on it.",
			                           "ScanList",NetWork_Get_WifiScanList,NULL,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/AddAP",jsonTree,"Add AP. Put AP Info on it.",
			                           "AddAP",NULL,NetWork_Put_AddAP,NULL,NULL);
		NetWork_RestComm_AddPathToTree("/NetAttr/Wifi/DelAP",jsonTree,"Del AP. Put AP Info on it.",
			                           "DelAP",NULL,NetWork_Put_DelAP,NULL,NULL);
    }
#endif

    {
		char* out = NULL;
		Common_cJSON_T* tmpTree = Common_cJSON_GetObjectItem(jsonTree,"NetAttr");
		LOGI("[NetAttr] tree:=%s\n",out = Common_cJSON_Print(tmpTree,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
	}

	return 0;
}

int NetWork_Rest_NetAttr_initwifi(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/NetAttr/Wlan",jsonTree,"Wlan URI. Get URI list on it.",
	                                   "Wlan",NetWork_RestComm_GetUriList,NULL,NULL,NULL);

    NetWork_RestComm_AddPathToTree("/NetAttr/Wlan/0",jsonTree,"Wlan Info. Get Wlan Info on it.",
				                           "Wlan",NetWork_Get_Wlan,NULL,NULL,NULL);

    NetWork_RestComm_AddPathToTree("/NetAttr/WlanDns",jsonTree,"Wlan Info. Get Wlan Dns Info on it.",
				                           "Wlan",NetWork_Get_WlanDns,NULL,NULL,NULL);

    NetWork_RestComm_AddPathToTree("/NetAttr/WifiSignal",jsonTree,"Wlan Info. Get Wifi Signal Info on it.",
				                           "WifiSignal",NetWork_Get_WifiSignal,NULL,NULL,NULL);

    NetWork_RestComm_AddPathToTree("/NetAttr/WifiConfig",jsonTree,"Wlan Info. Get Wifi Signal Info on it.",
				                           "WifiConfig",NetWork_Get_WifiConfig,NetWork_Put_WifiConfig,NULL,NetWork_Del_WifiConfig);

	NetWork_RestComm_AddPathToTree("/NetAttr/WifiScanSsidResult",jsonTree,"Wlan Info. Get Wifi Signal Info on it.",
				                           "WifiConfig",NetWork_Get_WifiConfig,NULL,NULL,NULL);
	return 0;
}

int NetWork_Rest_NetAttr_init4g(Common_cJSON_T* jsonTree)
{
	// 4g
	NetWork_RestComm_AddPathToTree("/NetAttr/LteCfg",jsonTree,"LTE 4G URI. Get URI list on it.",
			                           "LTE",NetWork_Get_LTECfg,NetWork_Put_LTECfg,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/Lte4GBind",jsonTree,"LTE 4G URI. Put URI list on it.",
			                           "LTE",NULL,NetWork_Put_4GBind,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/Lte4GUnbind",jsonTree,"LTE 4G URI. Put URI list on it.",
			                           "LTE",NULL,NetWork_Put_4GUnbind,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/Lte4GSpecified",jsonTree,"LTE 4G URI. Put URI list on it.",
			                           "LTE",NULL,NetWork_Put_4GSpecified,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetAttr/LteSpecifiedCard",jsonTree,"LTE 4G URI. Put URI list on it.",
			                           "LTE",NULL,NetWork_Put_SpecifiedCard,NULL,NULL);
	return 0;
}

int NetWork_Rest_NetApp_init(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/NetApp",jsonTree,"NetApp root URI. Get URI list on it.",
		                           "NetApp",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/PPPoE",jsonTree,"PPPoE Info. Get or Put PPPoE Info on it.",
		                           "PPPoE",NetWork_Get_PPPOE,NetWork_Put_PPPOE,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/DDNS",jsonTree,"DDNS Info. Get or Put DDNS Info on it.",
		                           "DDNS",NetWork_Get_DDNS,NetWork_Put_DDNS,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/UPnP",jsonTree,"UPnP Info. Get or Put UPnP Info on it.",
		                           "UPnP",NetWork_Get_UPNP,NetWork_Put_UPNP,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/Mail",jsonTree,"Mail Info. Get or Put Mail Info on it.",
		                           "Mail",NetWork_Get_Mail,NetWork_Put_Mail,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/Telnetd",jsonTree,"Telnetd Info. Get or Put Telnetd Info on it.",
		                           "Telnetd",NetWork_Get_Telnetd,NetWork_Put_Telnetd,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/SNMP",jsonTree,"SNMP Info. Get or Put Telnetd Info on it.",
		                           "SNMP",NetWork_Get_SNMP,NetWork_Put_SNMP,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/Ftp",jsonTree,"Ftp Info. Get or Put Ftp Info on it.",
		                           "Ftp",NetWork_Get_Ftp,NetWork_Put_Ftp,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/HTTP",jsonTree,"HTTP Info. Get or Put HTTP Info on it.",
		                           "HTTP",NetWork_Get_HTTP,NetWork_Put_HTTP,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/NetApp/TransData",jsonTree,"Two Way Trans Data on udp.",
		                           "TransData",NetWork_Get_TransData, NetWork_Put_TransData,NULL,NULL);

    {
		char* out = NULL;
		Common_cJSON_T* tmpTree = Common_cJSON_GetObjectItem(jsonTree,"NetApp");
		LOGI("[NetApp] tree:=%s\n",out = Common_cJSON_Print(tmpTree,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
	}

	return 0;
}

int NetWork_Rest_Functions_init(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/Functions",jsonTree,"Functions root URI. Get URI list on it.",
		                           "Functions",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Functions/SendMail",jsonTree,"SendMail Info. Put SendMail Info on it.",
		                           "SendMail",NULL,Link_Send_Mail,NULL,NULL);

	NetWork_RestComm_AddPathToTree("/Functions/ModifyMac",jsonTree,"Modify Mac. Put Mac Info on it.",
		                           "ModifyMac",NULL,Modify_Mac,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Functions/DoFtp",jsonTree,"DoFtp. Put DoFtp Info on it.",
		                           "DoFtp",NULL,NetWork_Put_DoFtp,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Functions/DoHTTP",jsonTree,"DoHTTP. Put DoHTTP Info on it.",
		                           "DoHTTP",NULL,NetWork_Put_DoHTTP,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Functions/TestMail",jsonTree,"Test Mail",
		                           "TestMail",NULL,Send_Test_Mail,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Functions/TransDataStart",jsonTree,"Test Mail",
		                           "TestMail",NULL,NetWork_Put_TransDataStart,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Functions/DoTransData",jsonTree,"Test Mail",
		                           "TestMail",NULL,NetWork_Put_DoTransData,NULL,NULL);

    {
		char* out = NULL;
		Common_cJSON_T* tmpTree = Common_cJSON_GetObjectItem(jsonTree,"Functions");
		LOGI("[Functions] tree:=%s\n",out = Common_cJSON_Print(tmpTree,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
	}

	return 0;
}

int NetWork_Rest_Status_init(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/Status",jsonTree,"Status root URI. Get URI list on it.",
		                           "Status",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Status/NetOut",jsonTree,"NetOut Info. Get NetOut Info on it.",
		                           "NetOut",NetWork_Get_NetOut,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Status/NetIpConflict",jsonTree,"NetIpConflict Info. Get NetIpConflict Info on it.",
		                           "NetIpConflict",NetWork_Get_NetIpConflict,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Status/NetTraffic",jsonTree,"NetTraffic Info. Get NetTraffic Info on it.",
		                           "NetTraffic",NetWork_Get_NetTraffic,NULL,NULL,NULL);
    NetWork_RestComm_AddPathToTree("/Status/Ftp",jsonTree,"Get do Ftp status on it.",
		                           "Ftp",NetWork_Get_Ftp_Status,NULL,NULL,NULL);
    {
		char* out = NULL;
		Common_cJSON_T* tmpTree = Common_cJSON_GetObjectItem(jsonTree,"Status");
		LOGI("[Status] tree:=%s\n",out = Common_cJSON_Print(tmpTree,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
	}

	return 0;
}

int NetWork_Rest_Subscribe_init(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/Subscribe",jsonTree,"Subscribe root URI. Get URI list on it.",
		                           "Status",NetWork_RestComm_GetUriList,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Subscribe/NetOut",jsonTree,"NetOut Subscribe URI.",
		                           "AlarmNetOut",NULL,NULL,NULL,NULL);
	NetWork_RestComm_AddPathToTree("/Subscribe/NetIpConflict",jsonTree,"NetIpConflict Subscribe URI.",
		                           "AlarmNetIpConflict",NULL,NULL,NULL,NULL);

	char* out = NULL;
	Common_cJSON_T* tmpTree = Common_cJSON_GetObjectItem(jsonTree,"Subscribe");
	LOGI("[Status] tree:=%s\n",out = Common_cJSON_Print(tmpTree,NULL));
	if(out)
		Common_Free(out,__FUNCTION__,__LINE__);

    return 0;
}

int NetWork_Rest_Subscribe_init4g(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/Subscribe/LteNetError",jsonTree,"LteNetError Subscribe URI.","LteNetError",NULL,NULL,NULL,NULL);

	return 0;
}

static int NetWork_Call_Restore()
{
	S32 iRet = -1;
	cJSON_Struct *pInData = NULL,*pOutData = NULL;
	cJSON_Struct *pNode = NULL;
	S32 nArrarIdx = 0;

	pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pInData != NULL)
	{
		Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/Core/Restore/Update",0,0);
		Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
		Common_Json_SetAttrValue(pInData,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pNode = Common_Json_SetAttrValue(pInData,-1,"Data/ResList",Common_Json_Type_Array,NULL,0,0);
		nArrarIdx = 0;
		Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String, "/NetWork/Restore", 0, 0);
		Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String, "NetWork", 0, 0);
		Common_Json_SetAttrValue(pNode, nArrarIdx, "/NeedReboot", Common_Json_Type_Number, NULL, 1, 0);

		iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
		if(0 != iRet)
		{
			char* out = NULL;
			LOGI("[Status] tree:=%s\n",out = Common_Json_Print(pOutData,NULL));
			if(out)
				Common_Free(out,__FUNCTION__,__LINE__);
		}
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);
	}

	return 0;
}

int NetWork_Rest_Restore_init(Common_cJSON_T* jsonTree)
{
	NetWork_RestComm_AddPathToTree("/Restore",jsonTree,"Restore root URI. Get URI list on it.",
		                           "Restore",NetWork_RestComm_GetUriList,NetWork_Put_Restore,NULL,NULL);

	NetWork_Call_Restore();

    {
		char* out = NULL;
		Common_cJSON_T* tmpTree = Common_cJSON_GetObjectItem(jsonTree,"Restore");
		LOGI("[Status] tree:=%s\n",out = Common_cJSON_Print(tmpTree,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
	}

    return 0;
}

int NetWork_Cfgm_init()
{
    int ret = 0;

	ret = ovfs_cfgm_init();
	if (0 != ret)
	{
	    LOGE("[%s:%d]:ovfs_cfgm_init fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	ret = ovfs_cfgm_set_cfg_auto_update_callback(NULL, NetCfgAutoUpdateCallback);
	if (0 != ret)
	{
	    LOGE("[%s:%d]:ovfs_cfgm_set_cfg_auto_update_callback fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	ret = NetWork_SetCfg_Ability();
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_Ability fail! ret=%d\n",ret);
        return ret;
	}

	ret = NetWork_SetCfg_MAC();
	if (0 != ret)
	{
	    LOGE("NetWork_SetCfg_MAC fail! ret=%d\n",ret);
	}
#if 0
	s_wifi_enable = ovfs_utility_is_net_dev_exist("wlan0");
	if (OVFS_FALSE == s_wifi_enable)
	{
        s_wifi_enable = ovfs_utility_is_net_dev_exist("ra0");
	}
#endif
	return 0;
}


int NetWork_LoadCfgAndStart()
{
    int ret = 0;
	cJSON_Struct* pConfig = NULL;
	cJSON_Struct* pConfigDefault = NULL;

	ret = Module_LoadConfig(NetWork_RestComm_GetModuleHdl(),&pConfig);
	if (0 != ret)
	{
        LOGE("[%s:%d]:Module_LoadConfig fail\n",__FUNCTION__, __LINE__);
	}

	ret = Module_LoadConfigByType(NetWork_RestComm_GetModuleHdl(),Module_ConfigType_Default,&pConfigDefault);
	if (0 != ret)
	{
        LOGE("[%s:%d]:Module_LoadConfigByType fail\n",__FUNCTION__, __LINE__);
	}

    ret = NetWork_Cfg_Init(pConfig,pConfigDefault);
	if (0 != ret)
	{
        LOGE("[%s:%d]:NetWork_Cfg_init fail\n",__FUNCTION__, __LINE__);
		Common_Json_Delete(pConfig);
		Common_Json_Delete(pConfigDefault);
        return ret;
	}

	Common_Json_Delete(pConfig);
	Common_Json_Delete(pConfigDefault);

    ret = ovfs_network_init();
	if (0 != ret)
	{
        LOGE("[%s:%d]:ovfs_network_init fail\n",__FUNCTION__, __LINE__);
        return ret;
	}

	ret = NetWork_Arp_Start();
	if (0 != ret)
	{
        LOGE("[%s:%d]:NetWok_Arp_Init fail\n",__FUNCTION__, __LINE__);
        return ret;
	}
#if 0
	ret = NetWork_SaveCfg();
	if (0 != ret)
	{
	    LOGE("[%s:%d]:NetWork_SaveCfg fail\n",__FUNCTION__, __LINE__);
	}
#endif
	return 0;
}

int NetWork_App_Init()
{
    if (!s_api_init)
    {
        pthread_mutex_lock(&s_api_init_lock);
        if (s_api_init == OVFS_FALSE)
        {
            //OVFS_CLR_ARG(s_network_app);
            s_api_init = OVFS_TRUE;
        }
        pthread_mutex_unlock(&s_api_init_lock);
    }

	return 0;
}

