/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 主要是对arp_tool层所要用到的类对象的资源统一管理
 *	源 文 件 : ovfs_arp_tool_resource.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/

#ifndef _OVFS_ARP_TOOL_RESOURCE_H_
#define _OVFS_ARP_TOOL_RESOURCE_H_

//#include "ovfs_comm_type.h"
#include "ovfs_comm_def.h"
//#include "ovfs_comm_sys.h"
//#include "ovfs_comm_debug.h"
#include "ovfs_arp_rarp_service.h"

//管理utilitiy模块使用的所有对象资源
namespace ovfs_arptool
{

class ovfs_arptool_res_manage: public ovfs_soft::ovfs_diagnose_client
{
public:
	OVFS_VOID construct_res(int max_video_ch_num);	//只允许初始化函数调用一次
	ovfs_arptool_arp *get_arptool();
	OVFS_VOID diagnose_dump(U32 para, ovfs_soft::ovfs_diag_dump dump_fuc);
	
private:
	ovfs_arptool_arp *m_arp_tool;

public:
	ovfs_arptool_res_manage();
	~ovfs_arptool_res_manage();
	const char* class_name() {return "ovfs_arptool_res_manage";};
	
private:
	ovfs_arptool_res_manage(const ovfs_arptool_res_manage &other);
	ovfs_arptool_res_manage &operator=(const ovfs_arptool_res_manage &other);
};

ovfs_arptool_res_manage* get_res_manage();

} //namespace ovfs_arptool{
#endif //#ifndef _OVFS_ARP_TOOL_RESOURCE_H_
