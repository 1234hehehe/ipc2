/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 已完成功能:
 		1.停止文件写入。
 		2. 导入，导出， 还原配置功能
 		3. 录像计划配置
 		4.	事件管理配置
 		5.网络参数配置
 *	源 文 件 : ovfs_cfg_manage_api.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/


#include <pthread.h>

#include "ovfs_comm_def.h"
//#include "ovfs_comm_debug.h"

//#include "libcommon_api.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_comm_errno.h"
#include "ovfs_network_base_def.h"

using namespace ovfs_soft;
using namespace ovfs_network;

typedef struct
{
    //ovfs_device_capability  m_device_ability;
    S8 serial_no[32];
	ovfs_network_capability m_network_ability;
	ovfs_ddns_ability ddns_ability;
	U32 m_eth_num;
	ovfs_netcard_config m_eth_cfg[OVFS_MAX_ETH_CARD_NUM]; 
	U32 m_bond_num; 
	ovfs_bounding_eth_config m_bond_cfg[OVFS_MAX_ETH_CARD_NUM]; 
	ovfs_local_net_dns m_dns_cfg;	
	S8 m_default_if[OVFS_MAX_ETH_IFNAME_SIZE];
    ovfs_pppoe_config m_pppoe_config;
    ovfs_pppoe_status m_pppoe_status;
    OVFS_BOOL m_ddns_enable;
    ovfs_ddns_info m_ddns_info[OVFS_MAX_ABILITY_DDNS_NUM];
    OVFS_BOOL m_upnp_enable;//是否使能UPNP(总开关)
    ovfs_upnp_port_detail_info m_port_detail_info[OVFS_PORT_MAX];//UPNP端口信息	
    OVFS_BOOL enable_attach;
    ovfs_email_sender_cfg m_email_cfg;
	ovfs_email_reciver_list m_recvier_list;
    OVFS_VOID *user_param;
    ovfs_cfg_auto_update_callback funcallback;    
}ovfs_network_cfg;


namespace ovfs_cfgm{
static OVFS_BOOL s_api_init = OVFS_FALSE;
static pthread_mutex_t s_api_init_lock = PTHREAD_MUTEX_INITIALIZER;
static ovfs_network_cfg m_network_cfg;
}//namespace ovfs_cfgm{

using namespace ovfs_cfgm;

static void cfgm_init()
{
    if (!s_api_init)
    {
        pthread_mutex_lock(&s_api_init_lock);
        if (s_api_init == OVFS_FALSE)
        {			
            OVFS_CLR_ARG(m_network_cfg);
            s_api_init = OVFS_TRUE;
        }
        pthread_mutex_unlock(&s_api_init_lock);
    }
    
    return;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_serialno(const S8* serial_no)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == serial_no)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

    snprintf(m_network_cfg.serial_no, sizeof(m_network_cfg.serial_no), "%s", serial_no);
	
    return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_serialno(S8* serial_no, U32 size)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == serial_no)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

    snprintf(serial_no, size, "%s", m_network_cfg.serial_no);

    return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT const ovfs_network_capability* ovfs_soft::ovfs_cfgm_get_network_cap()
{
    const ovfs_network_capability* network_ability = NULL;

	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return NULL;
	}

	network_ability = &(m_network_cfg.m_network_ability);

	return network_ability;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_network_cap(const ovfs_network_capability* network_ability)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}


	if (NULL == network_ability)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
    
    memcpy(&(m_network_cfg.m_network_ability), network_ability, sizeof(ovfs_network_capability));

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_init()
{
    cfgm_init();
	
    return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_pppoe_cfg(ovfs_pppoe_config *cfg)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(cfg == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

    memcpy(cfg, &(m_network_cfg.m_pppoe_config), sizeof(ovfs_pppoe_config));
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_pppoe_cfg(const ovfs_pppoe_config *cfg)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(cfg == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	memcpy(&(m_network_cfg.m_pppoe_config), cfg, sizeof(ovfs_pppoe_config));	

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_pppoe_status(const ovfs_pppoe_status *cfg)
{	
    int bChanged = 0;
	
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(cfg == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

    if (0 == memcmp(&(m_network_cfg.m_pppoe_status), cfg, sizeof(ovfs_pppoe_status)))
    {
        bChanged = 1;
	}
	
	memcpy(&(m_network_cfg.m_pppoe_status), cfg, sizeof(ovfs_pppoe_status));	

    if (NULL != m_network_cfg.funcallback && 1 == bChanged)
	{
        m_network_cfg.funcallback(m_network_cfg.user_param, OVFS_AUTO_PPPOE, (OVFS_VOID *)cfg, sizeof(ovfs_pppoe_status));
	}

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_upnp_cfg_ex(OVFS_BOOL enable)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	m_network_cfg.m_upnp_enable = enable;
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_upnp_cfg_ex(OVFS_BOOL *enable)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(enable == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

    *enable = m_network_cfg.m_upnp_enable;
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_upnp_port_cfg(S32 port_idx, S8 *eth_name, size_t eth_name_len, U16 *internal_port, U16 *external_port, OVFS_BOOL *enable, ovfs_upnp_port_protocol *protocol)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if((eth_name == NULL) || (internal_port == NULL) || (external_port == NULL) || (protocol == NULL) || (enable == NULL))
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	if(eth_name_len <= 0)
	{
		LOGE("Invalid Param\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	if (port_idx <0 || port_idx >= OVFS_PORT_MAX)
	{
		LOGE("Invalid Param\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	snprintf(eth_name, eth_name_len, "%s", m_network_cfg.m_port_detail_info[port_idx].eth_name);
	*internal_port = m_network_cfg.m_port_detail_info[port_idx].internal_port;
	*external_port = m_network_cfg.m_port_detail_info[port_idx].external_port;
	*enable = m_network_cfg.m_port_detail_info[port_idx].enable_upnp;
	*protocol = m_network_cfg.m_port_detail_info[port_idx].port_protocol;
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_upnp_port_cfg(U16 internal_port, S8 *eth_name, ovfs_upnp_port_protocol protocol, U16 external_port, OVFS_BOOL enable)
{
    S32 i = 0, select = -1;
	
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(eth_name == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	for (i = 0; i < OVFS_PORT_MAX; i++)
	{
	    if (m_network_cfg.m_port_detail_info[i].internal_port == 0)
	    {
	        if (select == -1)
	        {
                select = i;
			}
            continue;
		}
		
        if (m_network_cfg.m_port_detail_info[i].internal_port == internal_port && internal_port != 0)
        {
            select = i;
            break;
		}
	}

    if (select != -1 && select < OVFS_PORT_MAX)
    {
		snprintf(m_network_cfg.m_port_detail_info[select].eth_name, sizeof(m_network_cfg.m_port_detail_info[select].eth_name), "%s", eth_name);
	    m_network_cfg.m_port_detail_info[select].internal_port = internal_port;
		m_network_cfg.m_port_detail_info[select].external_port = external_port;
		m_network_cfg.m_port_detail_info[select].enable_upnp = enable;
		m_network_cfg.m_port_detail_info[select].port_protocol = protocol;

		return OVFS_SUCCESS;
    }
	
	return OVFS_ERR_CFGM_INVALID_PARA;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_local_eth_cfg(const S8* if_name, ovfs_soft::ovfs_netcard_config *eth)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == if_name || NULL == eth)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	for (i = 0; i < m_network_cfg.m_network_ability.total_eth_num; i++)
	{
        if (0 == strcmp(m_network_cfg.m_eth_cfg[i].if_name, if_name))
        {
            memcpy(eth, &(m_network_cfg.m_eth_cfg[i]), sizeof(ovfs_netcard_config));
            break;
		}
	}

	if (i >= m_network_cfg.m_network_ability.total_eth_num)
	{
		LOGE("if_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_add_local_eth_cfg(const ovfs_netcard_config *eth)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	if (NULL == eth)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	for (i = 0; i < m_network_cfg.m_network_ability.total_eth_num; i++)
	{
        if (NULL == strstr(m_network_cfg.m_eth_cfg[i].if_name, "eth"))
        {
            memcpy(&(m_network_cfg.m_eth_cfg[i]), eth, sizeof(ovfs_netcard_config));
            break;
		}
	}

	if (i >= m_network_cfg.m_network_ability.total_eth_num)
	{
		LOGE("if_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	return OVFS_SUCCESS;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_local_eth_cfg(const ovfs_netcard_config *eth, S32 type)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	if (NULL == eth)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	for (i = 0; i < m_network_cfg.m_network_ability.total_eth_num; i++)
	{
        if (0 == strcmp(m_network_cfg.m_eth_cfg[i].if_name, eth->if_name))
        {
            memcpy(&(m_network_cfg.m_eth_cfg[i]), eth, sizeof(ovfs_netcard_config));
            break;
		}
	}

	if (i >= m_network_cfg.m_network_ability.total_eth_num)
	{
		LOGE("if_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
/*	eth0 静态和dhcp 都采用(默认/用户)设置的dns , 有线dhcp 不设置dns
	if ((0 != type) && 
		(NULL != m_network_cfg.funcallback) &&
		(OVFS_TRUE == eth->dhcp))
	{
		m_network_cfg.funcallback(m_network_cfg.user_param, OVFS_AUTO_DNS, (OVFS_VOID *)eth, sizeof(ovfs_netcard_config));
	}
*/
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_dns_cfg(ovfs_local_net_dns *dns)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == dns)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

    memcpy(dns, &(m_network_cfg.m_dns_cfg), sizeof(ovfs_local_net_dns));

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_dns_cfg(const ovfs_local_net_dns *dns)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == dns)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

    memcpy(&(m_network_cfg.m_dns_cfg), dns, sizeof(ovfs_local_net_dns));
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_update_auto_dns_cfg(const ovfs_local_net_dns *dns)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == dns)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

    if (OVFS_TRUE != dns->auto_dns)
    {
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	memcpy(&(m_network_cfg.m_dns_cfg), dns, sizeof(ovfs_local_net_dns));
	
	if (NULL != m_network_cfg.funcallback)
	{
        m_network_cfg.funcallback(m_network_cfg.user_param, OVFS_AUTO_DNS, (OVFS_VOID *)dns, sizeof(ovfs_local_net_dns));
	}
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_default_if( S8* if_name, U32 size)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	if (NULL == if_name)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

    if (size < OVFS_MAX_ETH_IFNAME_SIZE)
    {
		LOGE("Invalid Param\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	snprintf(if_name, size, "%s", m_network_cfg.m_default_if);
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_default_if(const  S8* if_name)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == if_name)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
    snprintf(m_network_cfg.m_default_if, sizeof(m_network_cfg.m_default_if), "%s", if_name);

	return OVFS_SUCCESS;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_email_sender(ovfs_email_sender_cfg *cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == cfg)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	memcpy(cfg, &(m_network_cfg.m_email_cfg), sizeof(ovfs_email_sender_cfg));
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_email_sender(const ovfs_email_sender_cfg *cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	if (NULL == cfg)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	memcpy(&(m_network_cfg.m_email_cfg), cfg, sizeof(ovfs_email_sender_cfg));
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_email_receiver_num(U32 *num)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == num)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}	

    *num = m_network_cfg.m_recvier_list.node_num;
		
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_email_receiver(U32 idx, ovfs_email_receiver_cfg *cfg)
{  
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == cfg)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	if (idx >= m_network_cfg.m_recvier_list.node_num || idx >= OVFS_ARRAY_DIM(m_network_cfg.m_recvier_list.receiver))
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	memcpy(cfg, &(m_network_cfg.m_recvier_list.receiver[idx]), sizeof(ovfs_email_receiver_cfg));
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_email_receiver(const ovfs_email_receiver_cfg *cfg, U32 receiver_num)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == cfg || 0 == receiver_num)
	{
		LOGE("NULL Pointer cfg=%p,receiver_num=%d\n",cfg,receiver_num);
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for(i = 0; i < receiver_num && i<OVFS_ARRAY_DIM(m_network_cfg.m_recvier_list.receiver); ++i)
	{
		m_network_cfg.m_recvier_list.receiver[i] = cfg[i];
		m_network_cfg.m_recvier_list.node_num++;
	}
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_email_attachment_status(OVFS_BOOL enable_attach)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

    m_network_cfg.enable_attach = enable_attach;
		
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_email_attachment_status(OVFS_BOOL * enable_attach)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	if (NULL == enable_attach)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	*enable_attach = m_network_cfg.enable_attach;

	return OVFS_SUCCESS;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_dst_cfg(ovfs_dst_cfg *cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(cfg == NULL)
	{	
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	return OVFS_ERR_CFGM_NOT_SUPPORT;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_dst_cfg(const ovfs_dst_cfg *cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(cfg == NULL)
	{	
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	return OVFS_ERR_CFGM_NOT_SUPPORT;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_time_zone_cfg(ovfs_time_zone_cfg *zone_cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(zone_cfg == NULL)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	return OVFS_ERR_CFGM_NOT_SUPPORT;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_time_zone_cfg(const ovfs_time_zone_cfg *zone_cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if(zone_cfg == NULL)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	return OVFS_ERR_CFGM_NOT_SUPPORT;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_time_zoneTime(ovfs_soft::ovfs_time_zone zone, int *zoneHour, int *zoneMin)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	return OVFS_ERR_CFGM_NOT_SUPPORT;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ntp_cfg(ovfs_ntp_config *ntp_cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	return OVFS_ERR_CFGM_NOT_SUPPORT;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ntp_cfg(const ovfs_ntp_config *ntp_cfg)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	return OVFS_ERR_CFGM_NOT_SUPPORT;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_time_zone_dst_cap_num(U32 *node_num)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	return OVFS_ERR_CFGM_NOT_SUPPORT;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_time_zone_dst_cap(ovfs_soft::ovfs_dst_cap_node *dst_cap, U32 node_num, U32 *ret_num)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	return OVFS_ERR_CFGM_NOT_SUPPORT;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_time_zone_dst_cap(ovfs_soft::ovfs_dst_cap_node *dst_cap, U32 node_num)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	return OVFS_ERR_CFGM_NOT_SUPPORT;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_enable(const S8 *ddns_name, OVFS_BOOL enable)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	if (NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        m_network_cfg.m_ddns_info[i].enable = enable;
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ddns_enable(const S8 *ddns_name, OVFS_BOOL *enable)
{
    U32 i = 0;
		
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == enable || NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        *enable = m_network_cfg.m_ddns_info[i].enable;
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_start(OVFS_BOOL enable)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	m_network_cfg.m_ddns_enable = enable;

	return OVFS_SUCCESS;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ddns_start(OVFS_BOOL *enable)
{
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == enable)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	*enable = m_network_cfg.m_ddns_enable;

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_update_interval(const S8 *ddns_name, S32 update_min)
{
    U32 i = 0;
		
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        m_network_cfg.m_ddns_info[i].update_min = update_min;
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ddns_update_interval(const S8 *ddns_name, S32* update_min)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n"); 
		return OVFS_ERR_CFGM_NOT_INIT; 
	}
		
	if (NULL == update_min || NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        *update_min = m_network_cfg.m_ddns_info[i].update_min;
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_check_interval(const S8 *ddns_name, S32 check_min)
{
    U32 i = 0;
		
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        m_network_cfg.m_ddns_info[i].check_min = check_min;
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}

	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ddns_check_interval(const S8 *ddns_name, S32* check_min)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n"); 
		return OVFS_ERR_CFGM_NOT_INIT; 
	}
		
	if (NULL == check_min || NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}
	
	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        *check_min = m_network_cfg.m_ddns_info[i].check_min;
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_cap(const ovfs_soft::ovfs_ddns_ability *ddns_cap)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == ddns_cap)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

    memcpy(&(m_network_cfg.ddns_ability), ddns_cap, sizeof(ovfs_ddns_ability));

	for (i = 0; i < ddns_cap->node_num && i < OVFS_ARRAY_DIM(ddns_cap->ddns_cap); i++)
	{
		snprintf(m_network_cfg.m_ddns_info[i].param.ddns_name, 
			     sizeof(m_network_cfg.m_ddns_info[i].param.ddns_name),
			     "%s",
			     ddns_cap->ddns_cap[i].ddns_name);

		snprintf(m_network_cfg.m_ddns_info[i].param.server_host, 
			     sizeof(m_network_cfg.m_ddns_info[i].param.server_host),
			     "%s",
			     ddns_cap->ddns_cap[i].ddns_host);	

		m_network_cfg.m_ddns_info[i].param.server_port = ddns_cap->ddns_cap[i].ddns_port;
	}

    if (NULL != m_network_cfg.funcallback)
    {
        m_network_cfg.funcallback(m_network_cfg.user_param, OVFS_AUTO_DDNS, (OVFS_VOID *)ddns_cap, sizeof(ovfs_ddns_ability));
	}
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *ddns_cfg)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == ddns_cfg)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_cfg->ddns_name))
	    {
	        memcpy(&(m_network_cfg.m_ddns_info[i].param), ddns_cfg, sizeof(ovfs_ddns_cfg));
            break;
		}
	}
	
	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}	
	
	return OVFS_SUCCESS;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ddns_cfg(S8 *ddns_name, ovfs_soft::ovfs_ddns_cfg *ddns_cfg)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n"); 
		return OVFS_ERR_CFGM_NOT_INIT; 
	}

	if (NULL == ddns_cfg || NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        memcpy(ddns_cfg, &(m_network_cfg.m_ddns_info[i].param), sizeof(ovfs_ddns_cfg));
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	return OVFS_SUCCESS;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_ddns_info(const ovfs_ddns_info *ddns_info)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	if (NULL == ddns_info)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_info->param.ddns_name))
	    {
	        memcpy(&(m_network_cfg.m_ddns_info[i]), ddns_info, sizeof(ovfs_ddns_info));
            break;
		}
	}
	
	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}	
	
	return OVFS_SUCCESS;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_ddns_info(S8 *ddns_name, ovfs_ddns_info *ddns_info)
{
    U32 i = 0;
	
	if (s_api_init == OVFS_FALSE)
	{
		LOGE("cfg_manage_init first!\n"); 
		return OVFS_ERR_CFGM_NOT_INIT; 
	}

	if (NULL == ddns_info || NULL == ddns_name)
	{
		LOGE("invalid para!\n");
		return OVFS_ERR_CFGM_INVALID_PARA;
	}

	for (i = 0; i < m_network_cfg.ddns_ability.node_num && i < OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap); i++)
	{
	    if (0 == strcmp(m_network_cfg.m_ddns_info[i].param.ddns_name, ddns_name))
	    {
	        memcpy(ddns_info, &(m_network_cfg.m_ddns_info[i]), sizeof(ovfs_ddns_info));
            break;
		}	
	}

	if (i >= m_network_cfg.ddns_ability.node_num && i >= OVFS_ARRAY_DIM(m_network_cfg.ddns_ability.ddns_cap))
	{
		LOGE("ddns_name ERROR\n");
		return OVFS_ERR_CFGM_INVALID_PARA;	
	}
	
	return OVFS_SUCCESS;
}


//根据名称找到它的索引
FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_idx_by_eth_name(const S8 *eth_name, U32 *idx)
{
    U32 i = 0;
	
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	const ovfs_network_capability *net_cap = &(m_network_cfg.m_network_ability);

	if(net_cap == NULL)
	{
		return OVFS_ERR_CFGM_BASE;
	}
	
	for(i = 0; i < net_cap->total_eth_num; ++i)
	{
		if(0 == strcmp(eth_name, net_cap->eth_status[i].if_name))
		{
			*idx = i;
			return OVFS_SUCCESS;
		}
	}
	
	for(i = 0; i < net_cap->bond_num; ++i)
	{
		if(0 == strcmp(eth_name, net_cap->bond_status[i].bound_name))
		{
			*idx = (i + net_cap->total_eth_num);
			return OVFS_SUCCESS;
		}
	}

	return OVFS_ERR_CFGM_BASE;
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_get_eth_description_by_idx(U32 idx, S8 *description, U32 name_size)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}
	
	const ovfs_network_capability *net_cap = &(m_network_cfg.m_network_ability);
	if(net_cap == NULL)
	{
		return OVFS_ERR_CFGM_BASE;
	}

	if(idx < net_cap->total_eth_num)
	{
		snprintf(description, name_size, "%s", net_cap->eth_status[idx].description);
		return OVFS_SUCCESS;
	}
	else if (idx < (net_cap->total_eth_num + net_cap->bond_num))
	{
		snprintf(description, name_size, "%s", net_cap->bond_status[idx - net_cap->total_eth_num].description);
		return OVFS_SUCCESS;
	}
	else
	{
		return OVFS_ERR_CFGM_BASE;
	}
	
	return OVFS_ERR_CFGM_BASE;
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_cfgm_set_cfg_auto_update_callback(OVFS_VOID *user_param, ovfs_cfg_auto_update_callback callback)
{
	if(!s_api_init)
	{
		LOGE("cfg_manage_init first!\n");
		return OVFS_ERR_CFGM_NOT_INIT;
	}

	m_network_cfg.funcallback = callback;
	m_network_cfg.user_param = user_param;
	
    return OVFS_SUCCESS;
}



