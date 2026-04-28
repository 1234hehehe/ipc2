#ifndef _OVFS_ARPTOOL_SERVICE_H_
#define _OVFS_ARPTOOL_SERVICE_H_



#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

//#include <net/if.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
 #include <time.h>
#include <sys/time.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <errno.h>
#include <ifaddrs.h>

//#include <sys/ioctl.h>              /*ioctl 命令*/  
#include <linux/if_ether.h>         /*ethhdr 结构*/  
#include <net/if.h>                 /*ifreq 结构*/ 
#include <netpacket/packet.h>

#include <pthread.h>

//#include "ovfs_comm_type.h"
#include "ovfs_comm_def.h"
//#include "ovfs_comm_sys.h"
//#include "ovfs_comm_debug.h"
//#include "ovfs_utility_def.h"
#include "libcommon_api.h"
#include "ovfs_network_def.h"
#include "ovfs_arp_tool_def.h"

#define OVFS_ARPTOOL_ARP_FTYPE 0x0806  //  通过IP 查找MAC
#define OVFS_ARPTOOL_RARP_FTYPE 0x8035 //通过MAC查找IP



#define OVFS_ARPTOOL_OP_ARP_REQ     1	//ARP请求
#define OVFS_ARPTOOL_OP_ARP_RES     2	//ARP响应
#define OVFS_ARPTOOL_OP_RARP_REQ    3	//RARP请求
#define OVFS_ARPTOOL_OP_RARP_RES    4	//RARP响应

#define OVFS_ARPTOOL_ARP_SEARCH_MARK    0x517a5461 //ARP搜索请求码

#define OVFS_ARPTOOL_ARP_SEARCH_MAX_DEV    128	//
#define OVFS_ARPTOOL_ARP_SEARCH_MAX_PKT    256

#define OVFS_ARPTOOL_ARP_SEARCH_MAX_USE_DEV    1024
#define OVFS_ARPTOOL_ARP_MAX_BUFFSIZE 1600 //ARP包的接收缓冲大小
#define OVFS_ARPTOOL_ARP_PING_PKT_SIZE	128
#define OVFS_ARPTOOL_ETH_NUM 16


namespace ovfs_arptool
{
#pragma pack(push,1)

 typedef struct
{
	unsigned char byDstMac[6];
	unsigned char bySrcMac[6];
	unsigned short wEtherType;
}OVFS_ARPTOOL_ARP_ETHER_HEADER_T;


typedef struct _tagOVFS_ARPTOOL_ARPPACKET
{
	unsigned short wHardType; // 硬件类型.1-以太网地址，
	unsigned short wProtoType;// 协议类型;0x0800 为IP协议
	unsigned char byHrdLen;// 硬件地址长度;  以太网为6
	unsigned char byProtoLen;// 协议长度 IP协议,4
	unsigned short wOP;// 操作类型,1-ARP请求,2-ARP应答,3-RARP请求,4-RARP应答,5-SEARCH REQ ,6-SEARCH RES,7-SEARCH HELLO
	unsigned char bySrcMac[6];
	unsigned int dwSrcIP;
	unsigned char byDstMac[6];
	unsigned int dwDstIP;
	
}OVFS_ARPTOOL_ARPPACKET_T;

 

typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_PACKET
{
	unsigned int dwMark; // OVFS_ARPTOOL_ARP_SEARCH_MARK
	unsigned char byDevType; // 设备类型0, 1- DVR,2-NVR,3-IPC,4_DEC
	unsigned char byPhyNum;// 网卡个数
	unsigned char byPhyIdx[2]; // 0- eth0 ,1-eth1
	unsigned char byMac[2][6]; // 第一网卡
	unsigned int  dwIP[2]; // 
	unsigned short wTotalChanNum; // 总通道个数
	unsigned short wChanInfoSize; // 每个通道原始数据大小
	unsigned short wChanNum; // 当前包，容纳的通道信息个数
	unsigned short wChanStartIdx; // 本包开始通道索引,0开始
	unsigned int   dwRefreshCnt; // 更新计数，标识设备信息是否有更新。
	unsigned int dwRes[6];
}OVFS_ARPTOOL_ARP_SEARCH_PACKET_T;

typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO
{
	unsigned short wSize:14;
	unsigned short bCompressed:1;
	unsigned short bEnable:1; // 使能
}OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T;






#pragma pack(pop)


typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEVINFO
{
	unsigned char byDevType; // 设备类型0, 1- DVR,2-NVR,3-IPC,4_DEC
	unsigned char byPhyNum;// 网卡个数
	unsigned char byPhyIdx[2]; // 0- eth0 ,1-eth1
	unsigned char byMac[2][6]; // 第一网卡
	unsigned int  dwIP[2]; // 
	unsigned short wTotalChanNum; // 总通道个数
	unsigned short wChanInfoSize; // 总通道个数
}OVFS_ARPTOOL_ARP_SEARCH_DEVINFO_T;


typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV
{
	OVFS_ARPTOOL_ARP_SEARCH_DEVINFO_T tDevInfo;
	unsigned short wChanInfoSize; // 每个通道原始数据大小
	unsigned char  *byEnable; // -1未设，1-使能,0-不使能
	unsigned char  *byChanInfo;
	unsigned int    dwBuffSize;
	unsigned int   dwRefreshCnt;// 更新计数
	unsigned int   dwRunTimeSecond; // 60秒未更新，则删除
}OVFS_ARPTOOL_ARP_SEARCH_DEV_T;

typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_MGR
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_T tDev;
	struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_MGR *pPrev;
	struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_MGR *pNext;
}OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T;



typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR
{
	unsigned char byMac[6];
	char szIP[40];

	char bLocal;// 是否为本地设备
	struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR *pWhoUse;//这个ipc被哪些设备连接
	int  nUseCount;
	//
	struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR *pPrev;
	struct _tagOVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR *pNext;
}OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T;



typedef struct _tagOVFS_ARPTOOL_ARP_REQUIRE
{
	sem_t hSem_Res;
	int nStatus; // 0- 无效请求，1有效请求，2-冲突，3-超时
	int bReqMacSet;
	unsigned char byReqMac[6];
	unsigned int dwReqIP;
	unsigned char byConflictMac[6];
	int nTimeoutCnt;
	char *pArpPacket;
	int nArpPacketSize;
}OVFS_ARPTOOL_ARP_REQUIRE_T;


typedef struct _tagOVFS_ARPTOOL_ARP_SEARCH_SNED_PKT
{
	int nDataSize;
	char pBuffer[1500];
}OVFS_ARPTOOL_ARP_SEARCH_SNED_PKT_T;


typedef struct _tagOVFS_ARPTOOL_ARP_SOCKET_T
{
	int socket;		//套接字
	int is_bond_if;//是否是bonding的虚拟网卡
	char eth_name[16];	//网卡名
	unsigned int phy_idx; //网卡索引
	int fr_ifindex; //网卡号
}ovfs_arptool_arp_socket_t;






typedef int (*OVFS_ARPTOOL_ARP_CALLBACK)(unsigned int dwOP, unsigned char *bySrcMac/*[6]*/, char *pSrcIP, unsigned char *byDstMac/*[6]*/, char *pDstIP);

typedef int (*OVFS_ARPTOOL_ADDITIONALDATA_CALLBACK)(char *pSrcInfo, int nSrcInfoSize, char *pData, int nSize);

/*
	arp工具主要是提供ip冲突检测， 网线断开， 以及 ipc被连接次数的统计3种功能，都是在每个nvr设备上开启一个arp和rarp的服务，
	这个服务既要向局域网内广播自己的设备信息，也要收集其它设备发出来的信息，并且做出对应的arp或rarp应答。这样就将arp和rarp服务集于一体
	每个设备运行这个工具就运行了标准的arp和rarp的服务和客户端。
	具体实现思路:
	1.ip冲突检测，向局域网内发送标准的arp ip检测包 如过发现有相同ip的arp响应了，并且mac地址不是需要检测的mac，那么久证明这个ip已经被别人用了，有ip冲突
	2.检测断线就比较简单，直接查看网卡的状态就可以
	3.检测ipc连接数比较麻烦，会有第三种操作:
		第一:nvr定时向外发送arp请求，局域网内其他的设备收到请求后，通过arp服务向外广播自己的通道连接状态，本地nvr收到设备信息时将参数存到设备链表中m_dev_mgr;m_dev_mgr_tail;
		第二种:当通道发生变化时nvr向外广播自己的设备信息。
		本地nvr在接受到arp包时就去更新设备链表m_dev_mgr;m_dev_mgr_tail;同时设备链表发生变化，这些设备使用的ipc的信息存在m_use_mgr链表中也需要更新,这样我们就能知道局网内ipc被连接的状态。
		当长时间没有收到设备的响应时，就任务设备已经掉线，
*/

class ovfs_arptool_arp
{
public:
	//网络套接字监听线程函数，负责接收网络数据和处理响应包
	int on_message();

	//开启arp和rarp服务
	int start_arp_rarp_server();
	
	//关闭arp和rarp服务
	int stop_arp_rarp_server();

	//下面四个接口暂时还没有实现，等待以后实现用来回调arp数据包，可以做arp数据传输
	int set_callback(unsigned int option, OVFS_ARPTOOL_ARP_CALLBACK fxn, void *pUser);
	int send_packet(unsigned int option, unsigned char *src_mac/*[6]*/, char *src_ip, unsigned char *dst_mac/*[6]*/, char *dst_ip);
	int send_packet(unsigned int option, unsigned char *src_mac/*[6]*/, unsigned int src_ip, unsigned char *dst_mac/*[6]*/, unsigned int dst_ip);
	int require(int bRARP, unsigned char *bySrcMac/*[6]*/,char *pSrcIP,unsigned char *byDstMac/*[6]*/,char *pDstIP,unsigned int dwTimeoutMs);
	
   	//设置本地设备类型和通道的连接参数，以便arp服务通知所有在线设备自己连接了哪些通道
   	//0-默认为IPC,1-dvr,2-nvr,3-ipc,4-dec,5-DES,6-DEM
	int set_arp_search_local_data(int device_type, ovfs_soft::ovfs_arp_channel_device *chan_info, int chan_count);

	//开启arp搜索本地设备的功能
	int start_arp_search(int interval_time_by_second);
	//关闭arp搜索本地设备的功能
	int stop_arp_search();

	//这3个接口暂时不需要
	//int get_arp_search_result(OVFS_ARPTOOL_ARP_SEARCH_DEV_T **result);
	//int release_arp_search_result();
	//int get_arp_search_use_count(unsigned char *mac, char *ip);

	// byMac == NULL && pIP != NULL,此IP是否被使用
	// byMac != NULL && pIP != NULL ,检测本IP是否与指定MAC冲突。
	// byMac_out 为使用此IP或者冲突的MAC
	// dwTimeoutMs 为毫秒，最小为3000
	int check_IP_conflict(unsigned char *mac/*[6]*/, char *ip, unsigned char *mac_out/*[6]*/, unsigned int timeout_ms);
	//本地网卡是否和别的设备产生冲突
	int check_local_IP_conflict(const char *if_name, const char *ip );
	//本地网线是否断开连接
	int check_net_out(const char *if_name);
	//获取指定mac的ipc被连接的次数
	int get_ipc_connected_times(unsigned char *byMac, char *ip);


	void diagnose_dump(unsigned int para, ovfs_soft::ovfs_diag_dump dump_fuc);

private:
	int init_socket();
	
	//将每个可用网卡开启arp和rarp的套接字
	int open_all_support_socket();
	//关闭所有套接字
	int close_all_socket();
	
	//是否需要打开新的套接字
	int is_need_open_socket(const char *if_name, int is_rarp, unsigned int *socket_index);
	//字符串型的mac地址转换成无符号型
	void str_mac_2_ucmac(char *src_mac, unsigned char *dst, unsigned int dst_size);
	//打开指定网卡的套接字
	int open_socket(const char *if_name, int is_rarp, int *fr_ifindex);

	//从ipc的链表中找到指定的mac和ip地址的IPC
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *proc_find_ipc_from_usr_mgr(ovfs_soft::AntsARP_IPCSearchDeviceInfo_T *chan_dev);
	//将所有ipc中用户是本机节点删除
	void delete_ipc_self_usr_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs);
	//将每人用的ipc从ipc链表中删除
	void delete_use_ipc_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs);
	//找到每个ipc中用户是本机的节点
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *proc_find_self_from_ipc_user(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *who_use);

	//创建新的ipc节点
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *malloc_ipc_node(ovfs_soft::AntsARP_IPCSearchDeviceInfo_T *dev);
	//创建新的ipc用户节点
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *malloc_usr_node(unsigned char *mac, char *ip, char is_local);
	//将ipc节点加入到ipc链表之中
	void add_ipc_node_2_use(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs);
	//将ipc使用者节点加入到ipc的使用者链表之中
	void add_use_node_2_ipc(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs, OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUser);
	//更新本地的通道连接信息
	void update_local_chan_info(ovfs_soft::ovfs_arp_channel_device *chan_info, int chan_count);
	//更新本地通道信息的广播包内容
	void update_local_send_pkt(ovfs_soft::ovfs_arp_channel_device *chan_info, int chan_count);
	//是否跟本机ip冲突
	int is_conflict_with_self(unsigned char *byMac_in/*[6]*/, char *pIP, unsigned char *byMac_out);
	//更新本地设备的mac地址和ip， 需要更新所有ipc的本地使用者的mac和ip
	void update_local_usr_mac(const unsigned char *mac, const char *ip);
	//将所有套接字加入select
	void fd_set_all(fd_set *readSet, fd_set *execptSet, int *max_socket);
	//将所有需要发送的ip冲突请求从每个网卡发送一次
	void send_all_ip_conflict_pkt();
	//检查ip或者mac地址有没有更新
	void check_mac_and_ip_change(const char *if_name, int socket, unsigned int phy_idx, int fr_ifindex);
	//检查网线是否断开
	void check_net_out_update(const char *if_name, int socket, unsigned int phy_idx, int fr_ifindex);
	//间查ip冲突
	void check_ip_conflict_update(const char *if_name, int socket, unsigned int phy_idx, int fr_ifindex, char *buffer, int size);
	//发送ip冲突检测包
	int send_ip_conflict_pkt(int socket,  unsigned int phy_idx, int fr_ifindex, char *buffer, int buffer_size);
	//发送设备搜索包
	int send_dev_search_pkt(int socket,  unsigned int phy_idx, int fr_ifindex, char *buffer, int buffer_size);
	//向外广播自己通道设备的广播包
	void broadcast_self_dev_pkt(int socket, int phy_idx, int fr_ifindex);
	//更新设备链表
	void update_dev_mgr();

	//将长时间没有更新的设备从设备链表中弹出来
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *proc_pop_dev_node();
	//删除设备节点，并释放里面的数据
	void proc_del_dev_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pDelete);
	//找到ipc中指定的使用者
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T * proc_find_ipc_usr_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T* who_use, OVFS_ARPTOOL_ARP_SEARCH_DEVINFO_T *dev_info);
	//删除ipc中指定的使用者
	void proc_delete_ipc_usr(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T* pIPCs, OVFS_ARPTOOL_ARP_SEARCH_DEVINFO_T *dev_info);

	//接收所有套接字的数据
	//void recv_all_socket_data(int &need_open_socket,  char *buffer, int buffer_size);
	//接收指定套接字的数据
	int recv_data(int socket, int phy_idx, int fr_ifindex, char* buffer, int buffer_size);

	//响应rarp消息
	void respon_rarp_require(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len);
	//响应arp消息
	void respon_arp_require(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len);
	//响应各种arp消息 1-ARP请求,2-ARP应答,5-设备搜索请求 ,6-设备搜索响应,7-SEARCH HELLO
	void respon_arp_op_1(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len);
	void respon_arp_op_2(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len);
	void respon_arp_op_5(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len);
	void respon_arp_op_6_and_7(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len);

	//从设备链表中找到消息包的发送者
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *proc_find_dev_node(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt);
	//创建新的设备节点
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *malloc_search_dev_node(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt);
	//将新的设备节点加入到设备链表
	void push_search_dev_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev);
	//将新的设备节点弹出设备列表
	void pop_search_dev_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev);
	//由于通道数量发生变化，更新设备节点里面的通道参数
	int update_dev_node_by_chan_change(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev, OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt);
	//由于刷新码发生变化，更新设备节点里面的通道参数
	void update_dev_node_by_chan_freshcnt(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev, OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt);
	//清楚所有的设备链表
	void delete_all_dev();
	//删除所有的ipc
	void delete_all_ipc();
private:
	//最大本地通道数
	const unsigned int m_max_channel_num;
	// arp 套接字
	ovfs_arptool_arp_socket_t m_arp_socket[OVFS_ARPTOOL_ETH_NUM]; 
	// rarp 套接字
	ovfs_arptool_arp_socket_t m_rarp_socket[OVFS_ARPTOOL_ETH_NUM]; 
	//mac地址
	unsigned char m_local_mac[OVFS_ARPTOOL_ETH_NUM][6];
	//ipv4
	char m_local_ipv4[OVFS_ARPTOOL_ETH_NUM][OVFS_MAX_IP_ADDR_NUM + 1][64];
	U32 m_local_ipv4_num[OVFS_ARPTOOL_ETH_NUM];
	
	int m_net_out[OVFS_ARPTOOL_ETH_NUM];
	int m_ip_conflict[OVFS_ARPTOOL_ETH_NUM][OVFS_MAX_IP_ADDR_NUM + 1];
	int m_ip_conflict_cnt[OVFS_ARPTOOL_ETH_NUM][OVFS_MAX_IP_ADDR_NUM + 1];
	unsigned char m_conflict_mac[OVFS_ARPTOOL_ETH_NUM][OVFS_MAX_IP_ADDR_NUM + 1][6];
	
	//网卡名称
	char m_local_if_name[OVFS_ARPTOOL_ETH_NUM][64];

	//网卡个数
	unsigned int m_local_eth_cnt;
	ovfs_soft::ovfs_if_config_struct *m_if_config[OVFS_ARPTOOL_ETH_NUM];
	
	Common_Lock_T m_socket_lock;
	Common_Lock_T m_serch_dev_lock;

	ovfs_soft::ovfs_arp_channel_device *m_local_chanInfo;
	int m_local_chan_count;

	//本地设备信息的广播包
	OVFS_ARPTOOL_ARP_SEARCH_SNED_PKT_T *m_send_pkt[OVFS_ARPTOOL_ARP_SEARCH_MAX_PKT];
	int m_send_pkt_num;
	unsigned int m_send_refresh_cnt;//刷新码
	unsigned int m_send_refreshed_cnt; // 刷新码,已刷新的
	int    m_send_cnt; // 每次发送次数计数
	
	int m_exit_thread;
	Common_Thread_T m_thread; 
	
	int m_local_device_type;

	//ipc链表
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *m_use_mgr; 
	int m_dev_use_mgr_count;
	
	int m_interval_time_by_second;
	int m_last_search_time_by_second;
	int m_start_search;
	
	unsigned int m_run_time_s;
    unsigned int m_run_time_ms;
    unsigned int m_run_time_tast_second;
	OVFS_ARPTOOL_ARP_REQUIRE_T *m_conflict_require[8];


    //局域网内设备信息
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *m_dev_mgr;
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *m_dev_mgr_tail;
	int m_dev_search_count;

	
	
public:
	const char* class_name(){return "ovfs_arptool_arp";}
	ovfs_arptool_arp( unsigned int max_channel_num);
	~ovfs_arptool_arp();
	
};
}//namespace ovfs_arptool
#endif
