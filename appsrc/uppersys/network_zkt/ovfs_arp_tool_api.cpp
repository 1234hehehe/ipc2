#include "ovfs_comm_def.h"
//#include "ovfs_comm_debug.h"

#include "ovfs_cfg_manage_api.h"
//#include "ovfs_remote_agent_api.h"

#include "ovfs_arp_tool_api.h"
#include "ovfs_arp_tool_resource.h"

namespace ovfs_arptool{
static OVFS_BOOL s_api_init = OVFS_FALSE;
static pthread_mutex_t	s_api_init_lock = PTHREAD_MUTEX_INITIALIZER;
}

using namespace ovfs_soft;
using namespace ovfs_arptool;





FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_init(int max_video_ch_num)
{
	if (s_api_init == OVFS_FALSE)
	{
		pthread_mutex_lock(&s_api_init_lock);
		if (s_api_init == OVFS_FALSE)
		{			
			//初始化依赖的模块
			get_res_manage()->construct_res(max_video_ch_num);
			s_api_init = OVFS_TRUE;
		}
		pthread_mutex_unlock(&s_api_init_lock);
	}
	return 0;
}


FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_get_ipc_connected_times(unsigned char *byMac, char* ip)
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->get_ipc_connected_times(byMac, ip);
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_start_arp_rarp_server()
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->start_arp_rarp_server();
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_stop_arp_rarp_server()
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->stop_arp_rarp_server();
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_start_arp_search(int interval_time_by_second)
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->start_arp_search(interval_time_by_second);
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_stop_arp_search()
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->stop_arp_search();
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_set_arp_search_local_data(int device_type, ovfs_arp_channel_device *chan_info, int chan_count)
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->set_arp_search_local_data(device_type, chan_info, chan_count);
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_check_IP_conflict(unsigned char *mac/*[6]*/, char *ip, unsigned char *mac_out/*[6]*/, unsigned int timeout_ms)
{
	
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->check_IP_conflict(mac, ip, mac_out, timeout_ms);
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_check_local_IP_conflict(const char *if_name, const char *ip)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->check_local_IP_conflict(if_name, ip);
}

FOX_HELPER_DLL_EXPORT int ovfs_soft::ovfs_arptool_check_net_out(const char *if_name)
{
	if (!s_api_init)
	{
		LOGE("not init\n");
		return 0;
	}
	
	ovfs_arptool_arp *arp_tool = get_res_manage()->get_arptool();
	if (arp_tool == NULL)
	{
		LOGE("no arp_tool object!\n");
		return 0;
	}
	return arp_tool->check_net_out(if_name);
}

