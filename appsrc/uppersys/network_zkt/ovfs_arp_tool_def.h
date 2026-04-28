
#ifndef _OVFS_ARP_TOOL_DEF_H_
#define _OVFS_ARP_TOOL_DEF_H_

namespace ovfs_soft
{

//搜索相关(新搜索)
// IPV6/域名支持,可用于搜索结果或者通道设备配置
typedef struct _tagAntsARP_IPCSearchDeviceInfo
{
	unsigned int		dwSize;
	unsigned int		dwProtocolType;//协议类型(即协议ID)
	char        szProtocolName[16];// 协议名(希望仅做显示用,对内部来说，它不是重要的)
	unsigned char        byDeviceType;//0-未定义的 (可默认为IPC),  1-dvr,2-nvr,3-ipc,4-dec,5-DES,6-DEM
	unsigned char	    byEnableQuickAdd;// 支持自动配置,IP可修改的
	unsigned char        byMac[6];// MAC ,6 bytes HEX
	unsigned int		dwChanNum; // 设备的通道数0-不定，可认为是一个通道;
	
	unsigned char		byTransMode;   //可用于指定默认模式：0-主码流模式，1-子码流模式
	unsigned char		byLinkProtocol;	//可用于指定默认模式: 0-TCP;1-UDP;2-多播
	unsigned char		byEXMode;		//可用于指定默认模式:是否启用流模式 1:开启
	unsigned char		byRTSPMode; // 0: 一般模式;1:该协议同RTSP协议,szDomainAux有效,配置或PTZ都走RTSP流方式。
	unsigned short		wChannel; // 使用的设备通道,默认是第一通道
	
	unsigned short		wLinkAblility; // 连接支持能力,支持或操作:(0-TCP + UDP ,)1-TCP,2-UDP,...
	unsigned int		dwVideoPort;//!协议端口
	char	    szDomain[128];// 域名/IPv4/IPv6，登陆访问用,当为IP时应跟szIP同
	char	    szDomainAux[128];// 域名/IPv4/IPv6 当RTSP时有效
	char        szIP[40]; // 设备IP,可用于修改IP
	unsigned char        byNetMask;// 24->255.255.255.0;16->255.255.0.0
	unsigned char        byPhyIdx; // 0- eth0,1-eth1
	unsigned char        byProp;// bit0:=1表示设备名支持修改,=0不支持修改;
	unsigned char        byRes[1];
	char        szGateway[40];
	char        szDns1[40];
	char        szDns2[40];
	char		szDeviceName[64 + 1];
	char        szUserName[64 + 1];
	char        szPassword[64 + 1];
	char		szDescription[64 + 1];
	unsigned char		byRes2[128];
}AntsARP_IPCSearchDeviceInfo_T;

typedef struct
{
	AntsARP_IPCSearchDeviceInfo_T device;
	int enable;
}ovfs_arp_channel_device;
}
#endif