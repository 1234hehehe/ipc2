
/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 
 *	源 文 件 : ovfs_arp_tool_resource.cpp
 *	作    者 : 刘建文
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/
#include <pthread.h>

#include <string>
#include <arpa/inet.h>

//#include "ovfs_comm_debug.h"
#include "ovfs_comm_errno.h"

#include "ovfs_arp_tool_resource.h"

namespace ovfs_arptool{
static ovfs_arptool_res_manage *s_resource_manage = NULL;
static pthread_mutex_t	s_resource_lock = PTHREAD_MUTEX_INITIALIZER;
}

using namespace ovfs_soft;
using namespace ovfs_arptool;


ovfs_arptool_res_manage* ovfs_arptool::get_res_manage()
{
	if (s_resource_manage == NULL)
	{
		pthread_mutex_lock(&s_resource_lock);
		if (s_resource_manage == NULL)
		{
			s_resource_manage = new(ovfs_arptool_res_manage);
			OVFS_ASSERT(s_resource_manage != NULL);
		}
		pthread_mutex_unlock(&s_resource_lock);
	}
	return s_resource_manage;
}

//---------------------------------------------- ovfs_arptool_res_manage --------------------------------------
ovfs_arptool_res_manage::ovfs_arptool_res_manage()
{
	m_arp_tool = NULL;
}

ovfs_arptool_res_manage::~ovfs_arptool_res_manage()
{
	//暂时不支持析构
	OVFS_ASSERT(0);
}

//只允许初始化函数调用一次
OVFS_VOID ovfs_arptool_res_manage::construct_res(int max_video_ch_num)
{
	m_arp_tool = new ovfs_arptool_arp(max_video_ch_num);
	OVFS_ASSERT(m_arp_tool != NULL);
	//m_arp_tool->start_arp_rarp_server();
    //m_arp_tool->start_arp_search(30);

	//注册诊断
	{
		std::string help;
		help += "1-开启arp服务\n";
		help += "2-停止arp服务/\n";
		help += "3-设置通道连接参数\n";
		help += "4-查看本地ip是否冲突\n";
		help += "5-查看本地网线是否断开\n";
		help += "6-查看本地网卡信息\n";
		help += "7-查看本地服务套接字信息\n";
		help += "8-查看本地通道连接状态\n";
		help += "9-查看本地ipc连接状态\n";
		help += "10-查看本地设备链表\n";
		help += "11-查看ip是否冲突\n";
		help += "12-开启arp搜索服务\n";
		help += "13-停止arp搜索服务/\n";
		//ovfs_utility_regist_diag(this, "诊断 ARPTOOL 资源", help.c_str());
	}
	return;
}

ovfs_arptool_arp *ovfs_arptool_res_manage::get_arptool()
{
	return m_arp_tool;
}

OVFS_VOID ovfs_arptool_res_manage::diagnose_dump(U32 para, ovfs_soft::ovfs_diag_dump dump_fuc)
{
	switch(para)
	{
		case 1:
			dump_fuc("开启arp和rarp服务\n");
			m_arp_tool->start_arp_rarp_server();
			break;
		case 2:
			dump_fuc("停止arp和rarp服务\n");
			m_arp_tool->stop_arp_rarp_server();
			break;
		case 3:
			//m_arp_tool->set_arp_search_local_data();
			break;
		case 4:
			m_arp_tool->diagnose_dump(6, dump_fuc);
			break;
		
		case 5:
			m_arp_tool->diagnose_dump(7, dump_fuc);
			break;
			
		case 6:
			m_arp_tool->diagnose_dump(1, dump_fuc);
			break;
			
		case 7:
			m_arp_tool->diagnose_dump(2, dump_fuc);
			break;
			
		case 8:
			m_arp_tool->diagnose_dump(3, dump_fuc);
			break;
			
		case 9:
			m_arp_tool->diagnose_dump(4, dump_fuc);
			break;
			
		case 10:
			m_arp_tool->diagnose_dump(5, dump_fuc);
			break;
		case 11:
			{
				char ip[128];
				snprintf(ip, sizeof(ip), "%s", "192.168.0.1");
				unsigned int dwip = inet_addr(ip);
				for(int i = 0; i < 65535; ++i)
				{
					unsigned char byMacIn[6] = {0xCA, 0x25, 0x8C, 0xF7, 0x52, 0x46};
					unsigned char byMacOut[6];
					snprintf(ip, sizeof(ip), "%d.%d.%d.%d"
						, dwip&0xff
						, ( dwip>>8 ) & 0xff
						, ( dwip>>16 ) & 0xff
						, ( dwip>>24 ) & 0xff);
					dump_fuc("检查 %s 是否有人在用\n", ip);
					int ret = m_arp_tool->check_IP_conflict(byMacIn, ip, byMacOut, 3000);
					if(ret)
					{
						dump_fuc("%s有人在用 使用者[%02x:%02x:%02x:%02x:%02x:%02x]\n"
							, ip
							, byMacOut[0]
							, byMacOut[1]
							, byMacOut[2]
							, byMacOut[3]
							, byMacOut[4]
							, byMacOut[5]);
					}
					else
					{
						dump_fuc("%s没有人在用 \n",ip);
					}
					dwip = htonl(dwip);
					dwip++;
					dwip = htonl(dwip);
				}
			}
			break;
		case 12:
			dump_fuc("开启arp和rarp搜索服务\n");
			m_arp_tool->start_arp_search(30);
			break;
		case 13:
			dump_fuc("停止arp和rarp搜索服务\n");
			m_arp_tool->stop_arp_search();
			break;
			
		default:
			break;
	}
	
}

