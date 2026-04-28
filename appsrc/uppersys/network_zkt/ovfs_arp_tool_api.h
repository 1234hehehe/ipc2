 
/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 主要是进行IP冲突检测，网络断线检测，ipc连接次数计数
 *	源 文 件 : ovfs_arp_tool_api.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2015/12/21
 *****************************************************************************/


#ifndef _OVFS_ARP_TOOL_API_H_
#define _OVFS_ARP_TOOL_API_H_

//#include "ovfs_comm_type.h"
#include "ovfs_comm_errno.h"

//#include "ovfs_remote_agent_def.h"
//#include "ovfs_access_manage_def.h"
#include "ovfs_arp_tool_def.h"

namespace ovfs_soft {


int ovfs_arptool_init(int max_video_ch_num);
//获取指定mac地址的ipc被连接的次数
int ovfs_arptool_get_ipc_connected_times(unsigned char *byMac, char* ip);

//开启arp和rarp服务
int ovfs_arptool_start_arp_rarp_server();
//关闭arp和rarp服务
int ovfs_arptool_stop_arp_rarp_server();

//开启arp搜索本地设备的功能
int ovfs_arptool_start_arp_search(int interval_time_by_second);
//关闭arp搜索本地设备的功能
int ovfs_arptool_stop_arp_search();
	
//设置本地设备类型和通道的连接参数，以便arp服务通知所有在线设备自己连接了哪些通道
//0-默认为IPC,1-dvr,2-nvr,3-ipc,4-dec,5-DES,6-DEM
int ovfs_arptool_set_arp_search_local_data(int device_type, ovfs_arp_channel_device *chan_info, int chan_count);

// byMac_out 为使用此IP或者冲突的MAC
// dwTimeoutMs 为毫秒，最小为3000
int ovfs_arptool_check_IP_conflict(unsigned char *mac/*[6]*/, char *ip, unsigned char *mac_out/*[6]*/, unsigned int timeout_ms);
//本地网卡是否和别的设备产生冲突
int ovfs_arptool_check_local_IP_conflict(const char *if_name, const char *ip = NULL);
//本地网线是否断开连接
int ovfs_arptool_check_net_out(const char *if_name);


}//namespace ovfs_soft 
#endif	//#ifndef _OVFS_ARP_TOOL_API_H_
