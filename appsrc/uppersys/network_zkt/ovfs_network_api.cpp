/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 已完成功能:
 				1.获取普通网卡参数的功能
 				2.获取bonding网卡参数的功能
 				3.获取dns参数的功能
 *	源 文 件 : ovfs_network_api.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/

#include <pthread.h>

#include "ovfs_comm_def.h"
//#include "ovfs_comm_debug.h"
#include "ovfs_comm_errno.h"

#include "libcommon_api.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_network_api.h"
#include "ovfs_network_resource.h"

namespace ovfs_network{
static OVFS_BOOL s_api_init = OVFS_FALSE;
static pthread_mutex_t	s_api_init_lock = PTHREAD_MUTEX_INITIALIZER;
}//namespace ovfs_network{

using namespace ovfs_soft;
using namespace ovfs_network;

/*****************************************************************************************************
 * oˉ êy ?? : ovfs_network_init
 * ?è    ê? : 3?ê??ˉnetwork?￡?é,?ò??2éó?μ￥?t???¨11?¨ò???ovfs_network_res_manageμ????ó￡?è?oóμ÷ó??üμ?ò???3?ê??ˉoˉêy
 * ?μ??		: OVFS_SUCCESS---3é1|
 			  ??OVFS_SUCCESS---ê§°ü
 * ê?è?2?êy : none
 * ê?3?2?êy : 
 * ×￠òaê??? :
 * °?±?DT?? :
 *           vision  /    date      /   author   /  decription
 *           1.0     / 2012/12/12   /    xxxx    /   xxxx
 ****************************************************************************************************/

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_init()
{
	if (s_api_init == OVFS_FALSE)
	{
		pthread_mutex_lock(&s_api_init_lock);
		if (s_api_init == OVFS_FALSE)
		{			
			//初始化依赖的模块
			//ovfs_utility_init(OVFS_UTILITY_FOR_MODEL);

			ovfs_cfgm_init();

			get_res_manage()->construct_res();

			s_api_init = OVFS_TRUE;
		}
		pthread_mutex_unlock(&s_api_init_lock);
	}
	return OVFS_SUCCESS;
}


/*****************************************************************************************************
 * 函 数 名 : ovfs_network_get_local_get_eth_num
 * 描    述 : 获取系统实际的普通网卡个数
 * 返回		: OVFS_SUCCESS---成功
 			  非OVFS_SUCCESS---失败
 * 输入参数 : none
 * 输出参数 : eth_num --- 网卡个数
 * 注意事项 : 
 * 版本修订 :
 *           vision  /    date      /   author   /  decription
 *           1.0     / 2012/12/12   /    xxxx    /   xxxx
 ****************************************************************************************************/
FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_netcard_cfg(const S8 *if_name, ovfs_netcard_config *netcfg)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(if_name)||OVFS_IS_NULL_PTR(netcfg))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}

	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->get_local_netcard_cfg(if_name, netcfg);
}

/*****************************************************************************************************
 * 函 数 名 : ovfs_network_get_local_eth_cfg
 * 描    述 : 获取一个普通网卡的网络参数。
 * 返回		: OVFS_SUCCESS---成功
 			  非OVFS_SUCCESS---失败
 * 输入参数 : index --- 网卡索引，表示第几个网卡
 * 输出参数 : eth --- 网卡参数
 * 注意事项 : 
 * 版本修订 :
 *           vision  /    date      /   author   /  decription
 *           1.0     / 2012/12/12   /    xxxx    /   xxxx
 ****************************************************************************************************/
FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_netcard_cfg(const ovfs_netcard_config *netcfg)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(netcfg))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}
	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->set_local_netcard_cfg(netcfg);
}


/*****************************************************************************************************
 * 函 数 名 : ovfs_network_get_dns_cfg
 * 描    述 : 获取dns参数
 * 返回		: OVFS_SUCCESS---成功
 			  非OVFS_SUCCESS---失败
 * 输入参数 : none
 * 输出参数 : eth --- dns参数
 * 注意事项 : 
 * 版本修订 :
 *           vision  /    date      /   author   /  decription
 *           1.0     / 2012/12/12   /    xxxx    /   xxxx
 ****************************************************************************************************/
FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_dns_cfg(const ovfs_local_net_dns *eth)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(eth))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}
	
	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->set_dns_cfg(eth);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_dns_cfgv2(const ovfs_local_net_dns *eth)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(eth))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}
	
	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->set_dns_cfgv2(eth);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_dns_cfg(ovfs_local_net_dns *dns)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(dns))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}

	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->get_dns_cfg(dns);
}

/*****************************************************************************************************
 * 函 数 名 : ovfs_network_get_default_if
 * 描    述 : 获取dns参数
 * 返回		: OVFS_SUCCESS---成功
 			  非OVFS_SUCCESS---失败
 * 输入参数 :size --- 字符串大小
 			if_name --- 网卡名称
 * 输出参数 : if_name --- 网卡名称
 				
 * 注意事项 : 
 * 版本修订 :
 *           vision  /    date      /   author   /  decription
 *           1.0     / 2012/12/12   /    xxxx    /   xxxx
 ****************************************************************************************************/
FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_default_if(const S8* if_name)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(if_name))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}

	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->set_default_if(if_name);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_default_if( S8* if_name, U32 size)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(if_name))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}

	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}
		
	return net_mgr->get_default_router(if_name, size);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_special_traffic(const S8* if_name, U32 *upload, U32 *download)
{
	if (!s_api_init)
	{
        LOGE("Network Manage Not Init\n");
        return OVFS_ERR_NETWORK_NOT_INIT;
    }
	
	if(OVFS_IS_NULL_PTR(upload)||OVFS_IS_NULL_PTR(download))
	{
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
	}
	
	ovfs_network_config_mgr *net_mgr = get_res_manage()->get_eth_cfg_mgr();
	if(NULL == net_mgr)
	{
		LOGE("net_mgr is NULL\n"); 
		return OVFS_ERR_NETWORK_BASE; 
	}
	
	return net_mgr->get_special_netcard_traffic(if_name, upload, download); 
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_email_sender(const ovfs_email_sender_cfg *cfg)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(cfg))
    {
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
    }

	ovfs_network_smtp *smtp = get_res_manage()->get_smtp();
	if(NULL == smtp)
	{
		LOGE("smtp is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return smtp->set_email_sender(cfg);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_email_receiver(const ovfs_email_receiver_cfg *cfg, U32 receiver_num)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(cfg))
    {
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
    }

	ovfs_network_smtp *smtp = get_res_manage()->get_smtp();
	if(NULL == smtp)
	{
		LOGE("smtp is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return smtp->set_email_receiver(cfg, receiver_num);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_send_email(const S8 *title,
								 const S8 *content,
								 const ovfs_email_attachment_fname *attachment, 
								 U32 attachment_num,
								 OVFS_BOOL is_sync,
								 OVFS_BOOL is_need_rm_attach)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}


	ovfs_network_smtp *smtp = get_res_manage()->get_smtp();
	if(NULL == smtp)
	{
		LOGE("smtp is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return smtp->send_email(title, content, attachment, attachment_num, is_sync, is_need_rm_attach);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_send_email(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg,
		                         const ovfs_soft::ovfs_email_receiver_cfg *recv_cfg,
		                         U32 receiver_num,
		                         const S8 *title,
								 const S8 *content,
								 const ovfs_email_attachment_fname *attachment, 
								 U32 attachment_num,
								 OVFS_BOOL is_sync,
								 OVFS_BOOL is_need_rm_attach)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}


	ovfs_network_smtp *smtp = get_res_manage()->get_smtp();
	if(NULL == smtp)
	{
		LOGE("smtp is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return smtp->send_email(sender_cfg, recv_cfg, receiver_num, title, content, attachment, attachment_num, is_sync, is_need_rm_attach);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_send_email_tst()
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}


	ovfs_network_smtp *smtp = get_res_manage()->get_smtp();
	if(NULL == smtp)
	{
		LOGE("smtp is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return smtp->send_email_tst();
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_send_email_tst(const ovfs_soft::ovfs_email_sender_cfg *sender_cfg
		,const ovfs_soft::ovfs_email_receiver_cfg *receiver_cfg
		, U32 receiver_num)
{
	if(NULL == sender_cfg || NULL == receiver_cfg || receiver_num == 0) 
	{
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}


	ovfs_network_smtp *smtp = get_res_manage()->get_smtp();
	if(NULL == smtp)
	{
		LOGE("smtp is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return smtp->send_email_tst(sender_cfg, receiver_cfg, receiver_num);
}


FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_ddns_cap(ovfs_soft::ovfs_ddns_ability *ddns_ability)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(ddns_ability))
    {
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
    }
	
	ovfs_network_ddns *ddns = get_res_manage()->get_ddns();
	if(NULL == ddns)
	{
		LOGE("ddns is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return ddns->get_ddns_cap(ddns_ability);
}



FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_ddns_enable(const S8 *ddns_name, OVFS_BOOL enable)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(ddns_name))
    {
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
    }

	ovfs_network_ddns *ddns = get_res_manage()->get_ddns();
	if(NULL == ddns)
	{
		LOGE("ddns is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return ddns->set_ddns_enable(ddns_name, enable);
}



FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_ddns_cfg(const ovfs_soft::ovfs_ddns_cfg *cfg)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(OVFS_IS_NULL_PTR(cfg))
    {
		LOGE("Invalid Para\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;	
    }

	ovfs_network_ddns *ddns = get_res_manage()->get_ddns();
	if(NULL == ddns)
	{
		LOGE("ddns is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return ddns->set_ddns_cfg(cfg);
}



FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_start_ddns(OVFS_BOOL enable)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}


	ovfs_network_ddns *ddns = get_res_manage()->get_ddns();
	if(NULL == ddns)
	{
		LOGE("ddns is NULL\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	return ddns->start_ddns(enable);
}



FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_pppoe_config(const ovfs_pppoe_config *pppoe_config)
{
	if (!s_api_init)
	{
		LOGE("network module not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(pppoe_config == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}

	if(get_res_manage()->get_pppoe() == NULL)
	{
		return OVFS_ERR_NETWORK_OPERATE_FAIL;
	}

	return get_res_manage()->get_pppoe()->set_config(pppoe_config);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_pppoe_status(ovfs_pppoe_status *pppoe_status)
{
	if (!s_api_init)
	{
		LOGE("network module not init\n");
		return OVFS_ERR_NETWORK_NOT_INIT;
	}

	if(pppoe_status == NULL)
	{
		LOGE("NULL Pointer\n");
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}

	if(get_res_manage()->get_pppoe() == NULL)
	{
		return OVFS_ERR_NETWORK_OPERATE_FAIL;
	}

	return get_res_manage()->get_pppoe()->get_status(pppoe_status);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_upnp_state(OVFS_BOOL enable)
{
    if (!s_api_init)
    {
        LOGE("network module not init\n");
        return OVFS_ERR_NETWORK_NOT_INIT;
    }

    if(get_res_manage()->get_upnp() == NULL)
    {
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    if(enable)
    {
        return get_res_manage()->get_upnp()->enable_upnp();
    }
    else
    {
        return get_res_manage()->get_upnp()->disable_upnp();
    }
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_upnp_state(OVFS_BOOL *enable)
{
    if (!s_api_init)
    {
        LOGE("network module not init\n");
        return OVFS_ERR_NETWORK_NOT_INIT;
    }

    if(enable == NULL)
    {
        LOGE("NULL Pointer\n");
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

    if(get_res_manage()->get_upnp() == NULL)
    {
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    return get_res_manage()->get_upnp()->upnp_is_start(enable);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_set_upnp_port_info(OVFS_BOOL enable, U16 internal_port,  U16 external_port, ovfs_upnp_port_protocol port_protocol, const S8 *eth_name)
{
    if (!s_api_init)
    {
        LOGE("network module not init\n");
        return OVFS_ERR_NETWORK_NOT_INIT;
    }

    if(get_res_manage()->get_upnp() == NULL)
    {
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    return get_res_manage()->get_upnp()->set_port_info(enable, internal_port, external_port, port_protocol, eth_name);
}

FOX_HELPER_DLL_EXPORT OVFS_ERR ovfs_soft::ovfs_network_get_upnp_port_info(S32 index, ovfs_upnp_port_detail_info *p_port_detail_info)
{
    if (!s_api_init)
    {
        LOGE("network module not init\n");
        return OVFS_ERR_NETWORK_NOT_INIT;
    }

    if(p_port_detail_info == NULL)
    {
        LOGE("NULL Pointer\n");
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }
	
    if(get_res_manage()->get_upnp() == NULL)
    {
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    return get_res_manage()->get_upnp()->get_port_info(index, p_port_detail_info);
}

