
/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 主要是提供获取和设置网络参数的功能
 *	源 文 件 : ovfs_network_api.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/

#ifndef _OVFS_NETWORK_API_H_
#define _OVFS_NETWORK_API_H_

#include "ovfs_comm_def.h"
//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
//#include "ovfs_comm_sys.h"
#include "ovfs_network_def.h"


namespace ovfs_soft {

//------------------------------------ 系统功能控制 -----------------------------------
OVFS_ERR ovfs_network_init();

//----------------------------------------------- 网络配置 -------------------------------------------------------
OVFS_ERR ovfs_network_get_netcard_cfg(const S8* if_name, ovfs_soft::ovfs_netcard_config *netcfg);
OVFS_ERR ovfs_network_set_netcard_cfg(const ovfs_netcard_config *netcfg);


OVFS_ERR ovfs_network_get_dns_cfg(ovfs_local_net_dns *dns);
OVFS_ERR ovfs_network_set_dns_cfg(const ovfs_local_net_dns *dns);
OVFS_ERR ovfs_network_set_dns_cfgv2(const ovfs_local_net_dns *eth);

OVFS_ERR ovfs_network_get_default_if( S8* if_name, U32 size);
OVFS_ERR ovfs_network_set_default_if(const  S8* if_index);

OVFS_ERR ovfs_netowrk_get_link_status(OVFS_BOOL *network_link_ok);

OVFS_ERR ovfs_network_set_pppoe_cfg(const ovfs_pppoe_config *cfg);
OVFS_ERR ovfs_network_get_pppoe_status(ovfs_pppoe_status *pppoe_status);

OVFS_ERR ovfs_network_get_traffic(U32 *upload, U32 *download); 
OVFS_ERR ovfs_network_get_special_traffic(const S8* if_name, U32 *upload, U32 *download); 

//------------------------------------------------ DDNS ---------------------------------------
OVFS_ERR ovfs_network_get_ddns_cap(ovfs_soft::ovfs_ddns_ability *ddns_ability);
OVFS_ERR ovfs_network_set_ddns_enable(const S8 *ddns_name, OVFS_BOOL enable);
OVFS_ERR ovfs_network_set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *cfg);
OVFS_ERR ovfs_network_start_ddns(OVFS_BOOL enable);



//----------------------------------------------- 邮件功能配置 ---------------------------------
OVFS_ERR ovfs_network_set_email_sender(const ovfs_email_sender_cfg *cfg);
OVFS_ERR ovfs_network_set_email_receiver(const ovfs_email_receiver_cfg *cfg, U32 receiver_num);	//设置接收者

OVFS_ERR ovfs_network_send_email(const S8 *title,	//标题
								 const S8 *content,	//正文
								 const ovfs_email_attachment_fname *attachment, 	//附件
								 U32 attachment_num,	//附件数目
								 OVFS_BOOL is_sync,
								 OVFS_BOOL is_need_rm_attach = OVFS_FALSE);

OVFS_ERR ovfs_network_send_email(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg,
		                         const ovfs_soft::ovfs_email_receiver_cfg *recv_cfg,
		                         U32 receiver_num,
		                         const S8 *title,	//标题
								 const S8 *content,	//正文
								 const ovfs_email_attachment_fname *attachment, 	//附件
								 U32 attachment_num,	//附件数目
								 OVFS_BOOL is_sync,
								 OVFS_BOOL is_need_rm_attach = OVFS_FALSE);

OVFS_ERR ovfs_network_send_email_tst();	//发送测试邮件，测试邮箱配置是否正常

OVFS_ERR ovfs_network_send_email_tst(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg
		,const ovfs_soft::ovfs_email_receiver_cfg *cfg
		, U32 receiver_num);//发送测试邮件，测试邮箱配置是否正常

//---------------------------------------------PPPOE-----------------------------------------
OVFS_ERR ovfs_network_set_pppoe_config(const ovfs_pppoe_config *pppoe_config);
OVFS_ERR ovfs_network_get_pppoe_status(ovfs_pppoe_status *pppoe_status);

//--------------------------------------------UPNP--------------------------------------------
OVFS_ERR ovfs_network_set_upnp_state(OVFS_BOOL enable);
OVFS_ERR ovfs_network_get_upnp_state(OVFS_BOOL *enable);
OVFS_ERR ovfs_network_set_upnp_port_info(OVFS_BOOL enable_upnp, U16 internal_port,  U16 external_port = 0, ovfs_upnp_port_protocol port_protocol  = ovfs_soft::OVFS_PROTOCOL_TCP, const S8 *eth_name = NULL);
OVFS_ERR ovfs_network_get_upnp_port_info(S32 index, ovfs_upnp_port_detail_info *p_port_detail_info);

//网络测试
OVFS_ERR ovfs_network_test_dest_ip(const S8 *local_ip, const S8 *remote_ip, U32 *drop_rate, U32 *delay_ms);  

//抓包备份
OVFS_ERR ovfs_network_wireshark_backup(const S8* if_name, const S8* save_path); 

OVFS_ERR ovfs_network_ethtool_gset(const char* if_name, ovfs_soft::ovfs_eth_speed* speed, ovfs_soft::ovfs_eth_duplex* duplex, OVFS_BOOL* autoneg, OVFS_BOOL* link)  ;
	

}//namespace ovfs_soft {
#endif	//#ifndef _OVFS_NETWORK_API_H_

