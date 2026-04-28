/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 提供配置文件管理
 *	源 文 件 : ovfs_cfg_manage_resource.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/

#ifndef _OVFS_CFG_MANAGE_API_H_
#define _OVFS_CFG_MANAGE_API_H_

//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
//#include "ovfs_comm_sys.h"
#include "ovfs_network_def.h"

namespace ovfs_soft {

typedef struct _tagovfs_ddns_info_T
{ 
	OVFS_BOOL enable; 
	S32  update_min;
	S32  check_min;	
	ovfs_ddns_cfg param;
}ovfs_ddns_info;

typedef enum
{
    OVFS_AUTO_LOCAL_ETH = 0,
    OVFS_AUTO_DNS, 
    OVFS_AUTO_PPPOE,
    OVFS_AUTO_DDNS,
}ovfs_cfg_auto_update_type;

typedef S32 (*ovfs_cfg_auto_update_callback)(void *user_param, S32 type, OVFS_VOID *cfg_data, U32 cfg_size);

//------------------------------------ 系统功能控制 -----------------------------------
OVFS_ERR ovfs_cfgm_init();

OVFS_ERR ovfs_cfgm_set_serialno(const S8* serial_no);
OVFS_ERR ovfs_cfgm_get_serialno(S8* serial_no, U32 size);
const ovfs_network_capability* ovfs_cfgm_get_network_cap(); //获取网络能力参数
OVFS_ERR ovfs_cfgm_set_network_cap(const ovfs_network_capability* network_ability); //获取网络能力参数


//----------------------------------------------- 网络配置 -------------------------------------------------------
OVFS_ERR ovfs_cfgm_get_local_eth_cfg(const S8* if_name, ovfs_soft::ovfs_netcard_config *eth);
OVFS_ERR ovfs_cfgm_add_local_eth_cfg(const ovfs_netcard_config *eth);
OVFS_ERR ovfs_cfgm_set_local_eth_cfg(const ovfs_netcard_config *eth, S32 type);


OVFS_ERR ovfs_cfgm_get_dns_cfg(ovfs_local_net_dns *dns);
OVFS_ERR ovfs_cfgm_set_dns_cfg(const ovfs_local_net_dns *dns);
OVFS_ERR ovfs_cfgm_update_auto_dns_cfg(const ovfs_local_net_dns *dns);

OVFS_ERR ovfs_cfgm_get_default_if( S8* if_name, U32 size);
OVFS_ERR ovfs_cfgm_set_default_if(const  S8* if_index);

OVFS_ERR ovfs_cfgm_get_pppoe_cfg(ovfs_pppoe_config *cfg);	//拨号
OVFS_ERR ovfs_cfgm_set_pppoe_cfg(const ovfs_pppoe_config *cfg);
OVFS_ERR ovfs_cfgm_set_pppoe_status(const ovfs_pppoe_status *cfg);

OVFS_ERR ovfs_cfgm_set_upnp_cfg_ex(OVFS_BOOL enable);
OVFS_ERR ovfs_cfgm_get_upnp_cfg_ex(OVFS_BOOL *enable);
OVFS_ERR ovfs_cfgm_get_upnp_port_cfg(S32 port_idx, S8 *eth_name, size_t eth_name_len, U16 *internal_port, U16 *external_port, OVFS_BOOL *enable, ovfs_soft::ovfs_upnp_port_protocol *protocol);
OVFS_ERR ovfs_cfgm_set_upnp_port_cfg(U16 internal_port, S8 *eth_name, ovfs_soft::ovfs_upnp_port_protocol protocol, U16 external_port, OVFS_BOOL enable);

//------------------------------------------------ wifi，先空着 ---------------------------------------


//------------------------------------------------ DST、时区配置 ------------------------------
OVFS_ERR ovfs_cfgm_get_dst_cfg(ovfs_dst_cfg *cfg);	//夏令时配置
OVFS_ERR ovfs_cfgm_set_dst_cfg(const ovfs_dst_cfg *cfg);

OVFS_ERR ovfs_cfgm_get_time_zone_cfg(ovfs_time_zone_cfg *zone_cfg);		//时区
OVFS_ERR ovfs_cfgm_set_time_zone_cfg(const ovfs_time_zone_cfg *zone_cfg);		//设置时区

OVFS_ERR ovfs_cfgm_get_time_zoneTime(ovfs_soft::ovfs_time_zone zone, int *zoneHour, int *zoneMin);

OVFS_ERR ovfs_cfgm_get_ntp_cfg(ovfs_ntp_config *ntp_cfg);	//自动对时
OVFS_ERR ovfs_cfgm_set_ntp_cfg(const ovfs_ntp_config *ntp_cfg);

OVFS_ERR ovfs_cfgm_get_time_zone_dst_cap_num(U32 *node_num);
OVFS_ERR ovfs_cfgm_get_time_zone_dst_cap(ovfs_soft::ovfs_dst_cap_node *dst_cap, U32 node_num, U32 *ret_num);
OVFS_ERR ovfs_cfgm_set_time_zone_dst_cap(ovfs_soft::ovfs_dst_cap_node *dst_cap, U32 node_num);


//----------------------------------------------- 邮件功能配置 ---------------------------------
OVFS_ERR ovfs_cfgm_get_email_sender(ovfs_email_sender_cfg *cfg);	//发送者配置
OVFS_ERR ovfs_cfgm_set_email_sender(const ovfs_email_sender_cfg *cfg);

OVFS_ERR ovfs_cfgm_get_email_receiver_num(U32 *num);	//获取接收者个数
OVFS_ERR ovfs_cfgm_get_email_receiver(U32 idx, ovfs_email_receiver_cfg *cfg);	//获取接收者地址
OVFS_ERR ovfs_cfgm_set_email_receiver(const ovfs_email_receiver_cfg *cfg, U32 receiver_num);	//设置接收者

OVFS_ERR ovfs_cfgm_set_email_attachment_status(OVFS_BOOL enable_attach);
OVFS_ERR ovfs_cfgm_get_email_attachment_status(OVFS_BOOL * enable_attach);

//----------------------------------------------ddns配置-------------------------------------
OVFS_ERR ovfs_cfgm_set_ddns_enable(const S8 *ddns_name, OVFS_BOOL enable);
OVFS_ERR ovfs_cfgm_get_ddns_enable(const S8 *ddns_name, OVFS_BOOL *enable);
OVFS_ERR ovfs_cfgm_set_ddns_start(OVFS_BOOL enable);
OVFS_ERR ovfs_cfgm_get_ddns_start(OVFS_BOOL *enable);	

OVFS_ERR ovfs_cfgm_set_ddns_cap(const ovfs_soft::ovfs_ddns_ability *ddns_cap);
OVFS_ERR ovfs_cfgm_set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *ddns_cfg);
OVFS_ERR ovfs_cfgm_get_ddns_cfg(S8 *ddns_name, ovfs_soft::ovfs_ddns_cfg *ddns_cfg);

OVFS_ERR ovfs_cfgm_set_ddns_update_interval(const S8 *ddns_name, S32 update_min);
OVFS_ERR ovfs_cfgm_get_ddns_update_interval(const S8 *ddns_name, S32* update_min);

OVFS_ERR ovfs_cfgm_set_ddns_check_interval(const S8 *ddns_name, S32 check_min);
OVFS_ERR ovfs_cfgm_get_ddns_check_interval(const S8 *ddns_name, S32* check_min);

OVFS_ERR ovfs_cfgm_set_ddns_info(const ovfs_ddns_info *ddns_info);
OVFS_ERR ovfs_cfgm_get_ddns_info(S8 *ddns_name, ovfs_ddns_info *ddns_info);

OVFS_ERR ovfs_cfgm_get_idx_by_eth_name(const S8 *eth_name, U32 *idx);
OVFS_ERR ovfs_cfgm_get_eth_description_by_idx(U32 idx, S8 *description, U32 name_size);

OVFS_ERR ovfs_cfgm_set_cfg_auto_update_callback(OVFS_VOID *user_param, ovfs_cfg_auto_update_callback callback);


}//namespace ovfs_soft {
#endif	//#ifndef _OVFS_CFG_MANAGE_API_H_

