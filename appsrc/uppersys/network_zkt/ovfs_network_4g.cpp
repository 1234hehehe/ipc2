#include "ovfs_network_4g.h"
#include "curl/curl.h"
#include  <linux/sockios.h>
#include  <linux/ethtool.h>

#include <net/if.h>                 /*ifreq 结构*/
#include <termios.h>
/*
常见错误码

CME ERROR's (GSM Equipment related codes)

Error Description

CME ERROR: 0 Phone failure

CME ERROR: 1 No connection to phone

CME ERROR: 2 Phone adapter link reserved

CME ERROR: 3 Operation not allowed

CME ERROR: 4 Operation not supported

CME ERROR: 5 PH_SIM PIN required

CME ERROR: 6 PH_FSIM PIN required

CME ERROR: 7 PH_FSIM PUK required

CME ERROR: 10 SIM not inserted

CME ERROR: 11 SIM PIN required

CME ERROR: 12 SIM PUK required

CME ERROR: 13 SIM failure

CME ERROR: 14 SIM busy

CME ERROR: 15 SIM wrong

CME ERROR: 16 Incorrect password

CME ERROR: 17 SIM PIN2 required

CME ERROR: 18 SIM PUK2 required

CME ERROR: 20 Memory full

CME ERROR: 21 Invalid index

CME ERROR: 22 Not found

CME ERROR: 23 Memory failure

CME ERROR: 24 Text string too long

CME ERROR: 25 Invalid characters in text string

CME ERROR: 26 Dial string too long

CME ERROR: 27 Invalid characters in dial string

CME ERROR: 30 No network service

CME ERROR: 31 Network timeout

CME ERROR: 32 Network not allowed, emergency calls only

CME ERROR: 40 Network personalization PIN required

CME ERROR: 41 Network personalization PUK required

CME ERROR: 42 Network subset personalization PIN required

CME ERROR: 43 Network subset personalization PUK required

CME ERROR: 44 Service provider personalization PIN required

CME ERROR: 45 Service provider personalization PUK required

CME ERROR: 46 Corporate personalization PIN required

CME ERROR: 47 Corporate personalization PUK required

CME ERROR: 48 PH-SIM PUK required

CME ERROR: 100 Unknown error

CME ERROR: 103 Illegal MS

CME ERROR: 106 Illegal ME

CME ERROR: 107 GPRS services not allowed

CME ERROR: 111 PLMN not allowed

CME ERROR: 112 Location area not allowed

CME ERROR: 113 Roaming not allowed in this location area

CME ERROR: 126 Operation temporary not allowed

CME ERROR: 132 Service operation not supported

CME ERROR: 133 Requested service option not subscribed

CME ERROR: 134 Service option temporary out of order

CME ERROR: 148 Unspecified GPRS error

CME ERROR: 149 PDP authentication failure

CME ERROR: 150 Invalid mobile class

CME ERROR: 256 Operation temporarily not allowed

CME ERROR: 257 Call barred

CME ERROR: 258 Phone is busy

CME ERROR: 259 User abort

CME ERROR: 260 Invalid dial string

CME ERROR: 261 SS not executed

CME ERROR: 262 SIM Blocked

CME ERROR: 263 Invalid block

CME ERROR: 772 SIM powered down


 CMS ERROR's (GSM Network related codes)

Error Description

CMS ERROR: 1 Unassigned number

CMS ERROR: 8 Operator determined barring

CMS ERROR: 10 Call bared

CMS ERROR: 21 Short message transfer rejected

CMS ERROR: 27 Destination out of service

CMS ERROR: 28 Unindentified subscriber

CMS ERROR: 29 Facility rejected

CMS ERROR: 30 Unknown subscriber

CMS ERROR: 38 Network out of order

CMS ERROR: 41 Temporary failure

CMS ERROR: 42 Congestion

CMS ERROR: 47 Recources unavailable

CMS ERROR: 50 Requested facility not subscribed

CMS ERROR: 69 Requested facility not implemented

CMS ERROR: 81 Invalid short message transfer reference value

CMS ERROR: 95 Invalid message unspecified

CMS ERROR: 96 Invalid mandatory information

CMS ERROR: 97 Message type non existent or not implemented

CMS ERROR: 98 Message not compatible with short message protocol

CMS ERROR: 99 Information element non-existent or not implemente

CMS ERROR: 111 Protocol error, unspecified

CMS ERROR: 127 Internetworking , unspecified

CMS ERROR: 128 Telematic internetworking not supported

CMS ERROR: 129 Short message type 0 not supported

CMS ERROR: 130 Cannot replace short message

CMS ERROR: 143 Unspecified TP-PID error

CMS ERROR: 144 Data code scheme not supported

CMS ERROR: 145 Message class not supported

CMS ERROR: 159 Unspecified TP-DCS error

CMS ERROR: 160 Command cannot be actioned

CMS ERROR: 161 Command unsupported

CMS ERROR: 175 Unspecified TP-Command error

CMS ERROR: 176 TPDU not supported

CMS ERROR: 192 SC busy

CMS ERROR: 193 No SC subscription

CMS ERROR: 194 SC System failure

CMS ERROR: 195 Invalid SME address

CMS ERROR: 196 Destination SME barred

CMS ERROR: 197 SM Rejected-Duplicate SM

CMS ERROR: 198 TP-VPF not supported

CMS ERROR: 199 TP-VP not supported

CMS ERROR: 208 D0 SIM SMS Storage full

CMS ERROR: 209 No SMS Storage capability in SIM

CMS ERROR: 210 Error in MS

CMS ERROR: 211 Memory capacity exceeded

CMS ERROR: 212 Sim application toolkit busy

CMS ERROR: 213 SIM data download error

CMS ERROR: 255 Unspecified error cause

CMS ERROR: 300 ME Failure

CMS ERROR: 301 SMS service of ME reserved

CMS ERROR: 302 Operation not allowed

CMS ERROR: 303 Operation not supported

CMS ERROR: 304 Invalid PDU mode parameter

CMS ERROR: 305 Invalid Text mode parameter

CMS ERROR: 310 SIM not inserted

CMS ERROR: 311 SIM PIN required

CMS ERROR: 312 PH-SIM PIN required

CMS ERROR: 313 SIM failure

CMS ERROR: 314 SIM busy

CMS ERROR: 315 SIM wrong

CMS ERROR: 316 SIM PUK required

CMS ERROR: 317 SIM PIN2 required

CMS ERROR: 318 SIM PUK2 required

CMS ERROR: 320 Memory failure

CMS ERROR: 321 Invalid memory index

CMS ERROR: 322 Memory full

CMS ERROR: 330 SMSC address unknown

CMS ERROR: 331 No network service

CMS ERROR: 332 Network timeout

CMS ERROR: 340 No +CNMA expected

CMS ERROR: 500 Unknown error

CMS ERROR: 512 User abort

CMS ERROR: 513 Unable to store

CMS ERROR: 514 Invalid Status

CMS ERROR: 515 Device busy or Invalid Character in string

CMS ERROR: 516 Invalid length

CMS ERROR: 517 Invalid character in PDU

CMS ERROR: 518 Invalid parameter

CMS ERROR: 519 Invalid length or character

CMS ERROR: 520 Invalid character in text

CMS ERROR: 521 Timer expired

CMS ERROR: 522 Operation temporary not allowed

CMS ERROR: 532 SIM not ready

CMS ERROR: 534 Cell Broadcast error unknown

CMS ERROR: 535 Protocol stack busy

*/
/*
/dev/ttyUSB2
模块标识:AT+GMM -> EC20F
模块型号 : AT+CGMM -> EC20F

模块版本信息 :AT+CGMR -> EC20CEHCR06A03M1G
修订号码:AT+GMR:EC20CEHCR06A03M1G

制造商ID:AT+GMI -> Quectel
制造商名:AT+CGMI -> Quectel
序列号 :AT+CGSN -> 869756040198324
查询字符集:AT+CSCS=? -> +CSCS: ("IRA","GSM","UCS2")
国际移动用户标识码(SIM卡标识):AT+CIMI -> 460110351423526
卡拔出时上报:+CPIN: NOT READY


*/
#include "ovfs_network_def.h"
#include "ovfs_network_app.h"
#include "ovfs_network_api.h"
#include "ovfs_network_rest.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_network_utility_api.h"
#include "mxml.h"
using namespace ovfs_soft;

typedef struct
{
    int errCode;
    char errTips[32];
    char atType[32];
    char errMsg[32];
} OVFS_LTE_EVENT,*POVFS_LTE_EVENT;

typedef struct _tagNetworkLTECfg
{
    int bInit;
    int bEnable;
    int bAleadyReboot;
    int bRNDIS;
    int n4GFactory;			//0--以前旧版本, 1--域格clm920_rv3, 2--移远ec800/ec801
    OVFS_LTE_EVENT stStatus;
    int bAvailable;
    int bSIMCardStatus;
    int b4GRegStatus;
	int nStateOfCharge;
    char strIMEI[32];
    char strCCID[32];
    char strNetInfo[32];
    char strServiceInfo[40];
    int uSignalStrength;
    int uGPRS_Attach;
    int nQNetDevCtl;
    char strIPv4[32];
    char strIPv6[32];
    char strDNS1v4[32];
    char strDNS1v6[32];
    char strDNS2v4[32];
    char strDNS2v6[32];
    int subscribeID;
	int nApnType;
	char acApnStr[128];
    Common_Lock_T hLock;
} NetworkLTECfg_T;

typedef struct
{
	char acDataBuf[4 * 1024];
	U32 dwFull;
	U32 dwOffset;
	U32 dwTotoalSize;
} HttpBody;

typedef struct
{
	int nHttpCode;
} HttpHeader;

typedef struct
{
	int nDnsNum;    
	char ipv4[8][OVFS_IPV4_STRING_LEN + 1];
} ovfs_all_dns;

static int g_LteIndex = 0; //0--处于检测4G  模组状态中,1--处于检测4G  卡插拔中,2--处于检测4G  卡信息状态中,3-处于非法插入4 G  卡
static int g_LteCurStatus = 0;
static int g_LteModulePluged = 0;//4G模组是否插上过
static NetworkLTECfg_T g_LteCfg;
char str_OldRNDISifname[8] = "eth1";
char str_RNDISifname[8] = "eth1";
static int g_n4GNotPlugCount = 0;       //-1 --4G 卡插拔初始化状态 0--未插卡  1--已插卡
static int g_n4GConnectStatus = -1;   //-1 --4G 连接初始化状态  0--4G 联网中 1--4G 联网成功2--4G 联网失败
static int g_n4GConnectSuccCount = 0;
static int g_n4GConnectFailCount = 0;
static int g_n4GNotSpecifiedCount = 0;  //每检测不是同张卡12 次，播报语音
static int g_n4GDomainFailCount = 0;
//static int g_n4GUseless = 0;
static int g_n4gRouteChange = -1;
static int g_n4GCardSpecified = -1;
static char str_4gCustomerCfg[64] = "/update/res/custom/4GCustomerCfg.json";
static char str_4gCcidCfg[64] = "/usr/etc/4GCcid.json";
static char str_4gLogCfg[64] = "/tmp/devOffline.log";
static char str_4gDevFile[64] = "/tmp/ltedev";
static char UPLOAD_AUTH_DATA[] = "wuhanants:p2p-ants";
static char UPLOAD_LOGFILE_URL[] = "http://%s:%d/upload";
S32 g_n4gThreadCurTime = 0;
static int g_nNeedRedial = 0;
static int g_4g_thread_exit = 0,g_status_thread_exit = 0,g_nCheckCnt = 0;
static Common_Thread_T     m_thread_4g_handle = NULL,m_thread_status_handle = NULL;

extern int g_metric;
extern int s_nWiredExtranet;
extern S8 acSerialNumber[128];

static int Get_LteStatus(S8 *moudleType);
static S32  Lte_Exe_Cmd(const S8 *cmd,U32 timeout_ms, S8 *buf, U32 buf_size);
static S32 Lte_subscribeList_fxn(ModuleHandle_T hModuleHandle,S32 nType,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData);

extern int GetDNS(ovfs_soft::ovfs_local_net_dns *dns,char *path);

static int add_4g_route(char *ifname)
{
	char system_cmd[128] = {0};
	char lte_ipaddr[32] = {0}, lte_netmask[32] = {0}, lte_subnet[32] = {0}, lte_gateway[32] = {0};
	
	//注意两种情况的处理:
	//1, 已存在eth0 路由,pppd拨号会报错:  not replacing existing default route via 192.168.168.1,
	//2, 4g网卡被down-->up 后, 默认路由没啦
	int nRet = NetWorkTool_GetDefaultRouteV2(ifname, lte_gateway);
	if((nRet == -1) || (strlen(lte_gateway) == 0))
	{
		if(g_LteCfg.bRNDIS)
		{
			if(NetWorkTool_Get_Dhcpc_Result(ifname, lte_ipaddr, lte_netmask, lte_gateway) == 0)
			{
				NetWorkTool_GetSubNet(lte_subnet, lte_ipaddr, lte_netmask);
				
				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev %s", lte_subnet, lte_netmask, ifname);
				Common_System(system_cmd);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route add -net %s netmask %s dev %s", lte_subnet, lte_netmask, ifname);
				Common_System(system_cmd);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del default dev %s", ifname);
				Common_System(system_cmd);
				
				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s dev %s metric 0", lte_gateway, ifname);//添加默认路由器前,必须保证局域网路由正常
				Common_System(system_cmd);
			}
		}
		else
		{
			if((NetWorkTool_GetIPAddr(ifname,lte_ipaddr) == 0) && (NetWorkTool_GetMaskAddr(ifname,lte_netmask) == 0))
			{
				NetWorkTool_GetSubNet(lte_subnet, lte_ipaddr, lte_netmask);
				
				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev %s", lte_subnet, lte_netmask, ifname);
				Common_System(system_cmd);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route add -net %s netmask %s dev %s", lte_subnet, lte_netmask, ifname);
				Common_System(system_cmd);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del default dev %s", ifname);
				Common_System(system_cmd);
				
				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route add default dev %s metric 0", ifname);//添加默认路由器前,必须保证局域网路由正常
				Common_System(system_cmd);
			}
		}
	}
	
	g_metric = 2;
	
	return 0;
}

static int del_4g_route(char *ifname)
{
	char system_cmd[128] = {0};
	char lte_ipaddr[32] = {0}, lte_netmask[32] = {0}, lte_subnet[32] = {0}, lte_gateway[32] = {0};
	
	//插有线eth0 上网, 所以去掉4g 的默认路由
	int nRet = NetWorkTool_GetDefaultRouteV2(ifname, lte_gateway);
	if((nRet == 0) && (strlen(lte_gateway)))
	{
		if(g_LteCfg.bRNDIS)
		{
			if(NetWorkTool_Get_Dhcpc_Result(ifname, lte_ipaddr, lte_netmask, lte_gateway) == 0)
			{
				NetWorkTool_GetSubNet(lte_subnet, lte_ipaddr, lte_netmask);
				
				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev %s", lte_subnet, lte_netmask, ifname);
				Common_System(system_cmd);
			}
		}
		else
		{
			if((NetWorkTool_GetIPAddr(ifname,lte_ipaddr) == 0) && (NetWorkTool_GetMaskAddr(ifname,lte_netmask) == 0))
			{
				NetWorkTool_GetSubNet(lte_subnet, lte_ipaddr, lte_netmask);
				
				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev %s", lte_subnet, lte_netmask, ifname);
				Common_System(system_cmd);	
			}
		}
		
		memset(system_cmd, 0, sizeof(system_cmd));
		snprintf(system_cmd, sizeof(system_cmd), "route del default dev %s", ifname);
		Common_System(system_cmd);
	}
	
	return 0;
}

static int add_4g_dns(char *ifname)
{
	S8 filename[128] = {0};
	ovfs_soft::ovfs_local_net_dns lte_dns;
	ovfs_soft::ovfs_local_net_dns cfg_dns;
	
	if(g_LteCfg.bRNDIS)
	{
		snprintf(filename, sizeof(filename), "/tmp/udhcpc_resolv_%s.conf", str_RNDISifname);
		memset(&lte_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
		GetDNS(&lte_dns,(char *)filename);
	}
	else
	{
		snprintf(filename, sizeof(filename), "%s", "/etc/ppp/resolv.conf");
		memset(&lte_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
		GetDNS(&lte_dns,(char *)filename);
	}
	
	snprintf(filename, sizeof(filename), "%s", "/etc/resolv.conf");
	memset(&cfg_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
	GetDNS(&cfg_dns,(char *)filename);

	if(memcmp(&lte_dns, &cfg_dns, sizeof(ovfs_soft::ovfs_local_net_dns)) != 0)
	{
		ovfs_soft::ovfs_network_set_dns_cfgv2(&lte_dns);
	}
	
	return 0;
}

static int del_4g_dns(char *ifname)
{
	S8 filename[128] = {0};
	ovfs_soft::ovfs_local_net_dns lte_dns;
	ovfs_soft::ovfs_local_net_dns cfg_dns;
	
	if(g_LteCfg.bRNDIS)
	{
		snprintf(filename, sizeof(filename), "/tmp/udhcpc_resolv_%s.conf", str_RNDISifname);
		memset(&lte_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
		GetDNS(&lte_dns,(char *)filename);
	}
	else
	{
		snprintf(filename, sizeof(filename), "%s", "/etc/ppp/resolv.conf");
		memset(&lte_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
		GetDNS(&lte_dns,(char *)filename);
	}
	
	snprintf(filename, sizeof(filename), "%s", "/etc/resolv.conf");
	memset(&cfg_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
	GetDNS(&cfg_dns,(char *)filename);
	
	if(memcmp(&lte_dns, &cfg_dns, sizeof(ovfs_soft::ovfs_local_net_dns)) == 0)
	{
		ovfs_soft::ovfs_local_net_dns dns_tmp;
		memset(&dns_tmp, 0, sizeof(dns_tmp));
		ovfs_soft::ovfs_cfgm_get_dns_cfg(&dns_tmp);
		ovfs_soft::ovfs_network_set_dns_cfg(&dns_tmp);
	}
	
	return 0;
}

int set_4g_route_change(int nNetType)
{
	g_n4gRouteChange = nNetType;
	
	return 0;
}

int check_RNDIS_IsOK(char *ifname)
{
    int bOk = 0;
	
	//AT 命令不能多线程同时调用, 所以每10秒钟取拨号结果就行
	if(0 == g_LteCfg.nQNetDevCtl)
	{
		LOGE("DialResult=[%d]\n",g_LteCfg.nQNetDevCtl);
		return bOk;
	}
	
	int bNetExist = ovfs_utility_is_net_dev_exist(ifname);
	int nNetOut = ovfs_utility_is_net_dev_out(ifname);
	if((bNetExist == OVFS_FALSE) || (nNetOut == 1))
	{
		LOGE("bNetExist=[%d],nNetOut=[%d]\n",bNetExist,nNetOut);
		return bOk;
	}
	
    S32 socket_fd = Common_Socket_Open(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socket_fd == -1)
    {
    	LOGE("open socket failed\n");
        return bOk;
    }
	
    struct ifreq  ifr;						/*网络接口结构*/
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE,(char *)&ifr, sizeof(ifr)) < 0)
    {
    	LOGE("bind to device[%s] failed\n",ifname);
        Common_Socket_Close(socket_fd);
        return bOk;
    }
	
	Common_Socket_Close(socket_fd);
	
    char lte_ipaddr[32] = {0};
	if(NetWorkTool_GetIPAddr(ifname,lte_ipaddr) != 0 || strlen(lte_ipaddr) == 0)
	{
		LOGE("lte_ipaddr=[%s] invalid\n",lte_ipaddr);
		return bOk;
	}
	
	char lte_netmask[32] = {0};
	if(NetWorkTool_GetMaskAddr(ifname,lte_netmask) != 0 || strlen(lte_netmask) == 0)
	{
		LOGE("lte_netmask=[%s] invalid\n",lte_netmask);
		return bOk;
	}
	/*
	char lte_gateway[32] = {0};
	if(NetWorkTool_GetDefaultRouteV2(ifname, lte_gateway) != 0 || strlen(lte_gateway) == 0)
	{
		LOGE("lte_gateway=[%s] invalid\n",lte_gateway);
		return bOk;
	}
	*/
	snprintf(g_LteCfg.strIPv4,sizeof(g_LteCfg.strIPv4),"%s",lte_ipaddr);
	
	bOk = 1;
	
    return bOk;
}

int check_PPP_IsOK(char *ifname)
{
    int bOk = 0;
	
	int bNetExist = ovfs_utility_is_net_dev_exist(ifname);
	int nNetOut = ovfs_utility_is_net_dev_out(ifname);
	if((bNetExist == OVFS_FALSE) || (nNetOut == 1))
	{
		LOGE("bNetExist=[%d],nNetOut=[%d]\n",bNetExist,nNetOut);
		return bOk;
	}
	
    S32 socket_fd = Common_Socket_Open(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socket_fd == -1)
    {
    	LOGE("open socket failed\n");
        return bOk;
    }
	
    struct ifreq  ifr;						/*网络接口结构*/
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE,(char *)&ifr, sizeof(ifr)) < 0)
    {
    	LOGE("bind to device[%s] failed\n",ifname);
        Common_Socket_Close(socket_fd);
        return bOk;
    }

	Common_Socket_Close(socket_fd);
	
    char lte_ipaddr[32] = {0};
	if(NetWorkTool_GetIPAddr(ifname,lte_ipaddr) != 0 || strlen(lte_ipaddr) == 0)
	{
		LOGE("lte_ipaddr=[%s] invalid\n",lte_ipaddr);
		return bOk;
	}
	
	char lte_netmask[32] = {0};
	if(NetWorkTool_GetMaskAddr(ifname,lte_netmask) != 0 || strlen(lte_netmask) == 0)
	{
		LOGE("lte_netmask=[%s] invalid\n",lte_netmask);
		return bOk;
	}
	/*
	char lte_gateway[32] = {0};
	if(NetWorkTool_GetDefaultRouteV2(ifname, lte_gateway) != 0 || strlen(lte_gateway) == 0)
	{
		LOGE("lte_gateway=[%s] invalid\n",lte_gateway);
		return bOk;
	}
	*/
	snprintf(g_LteCfg.strIPv4,sizeof(g_LteCfg.strIPv4),"%s",lte_ipaddr);
	
	bOk = 1;
	
    return bOk;
}

static void Set4GModelLight()
{
	LOGW("set 4g model light begin\n");

	if(g_LteCfg.bInit == 1 && g_LteCfg.n4GFactory == 3)
	{
		int nCount = 10,nSetOk = 0;
		char buf[1024] = {0};
		while((nSetOk == 0) && (nCount-- > 0))
		{
			memset(buf, 0, sizeof(buf));
			Lte_Exe_Cmd("AT+MGPIOCFG=12,24,1,0", 1000, buf, sizeof(buf) - 1);
			char *pstrOk = strstr(buf,"OK");
    		if(pstrOk)
    		{
				nSetOk = 1;
			}
			else
			{
				Common_Sleep(1,0);
			}
		}
	}
	
	LOGW("set 4g model light succ\n");
}

static int Guard4GNetcard()
{
	LOGW("guard 4g net card begin\n");

	int nRet = 0;
	static int nLstGuard4GNetcardTime = 0;
	static int nCurGuard4GNetcardTime = 0;
	
	if(g_LteCfg.bInit == 1 && g_LteCfg.bRNDIS == 1)
	{
		if(ovfs_utility_is_net_dev_exist(str_RNDISifname) || ovfs_utility_is_net_dev_exist(str_OldRNDISifname))
		{
			Common_GetSystemCount(&nLstGuard4GNetcardTime, NULL);
			Common_GetSystemCount(&nCurGuard4GNetcardTime, NULL);
			nRet = 0;
		}
		else
		{
			Common_GetSystemCount(&nCurGuard4GNetcardTime, NULL);
			nRet = -1;
		}
		
		if(nCurGuard4GNetcardTime >= nLstGuard4GNetcardTime + 120)
		{
			LOGE("Guard4GNetcard failed. reboot system\n");
			Common_System("lsusb; ls -lh /dev/ttyUSB*; ifconfig -a; route -n");
			RebootSystem();
			Common_Sleep(10, 0);//设备重启有3秒延迟,不加睡眠会导致3秒延迟一直被设置
			return -1;
		}
	}
	
	LOGW("guard 4g net card succ\n");
	
	return nRet;
}

static int Guard4GDriverFile()
{
	LOGW("guard 4g driver file begin\n");

	int nRet = 0;
	static int nLstGuard4GDriverTime = 0;
	static int nCurGuard4GDriverTime = 0;
	
	if(g_LteCfg.bInit == 1)
	{
		int nExist1 = 0, nExist2 = 0;
		char acAtCmdFile[] = "/dev/at_cmd";
		char acModemCmdFile[] = "/dev/modem_cmd";

		nExist1 = !access(acAtCmdFile, F_OK);
		nExist2 = !access(acModemCmdFile, F_OK);
		
		struct stat st1,st2;
		stat(acAtCmdFile, &st1);
		stat(acModemCmdFile, &st2);
		
		if(((nExist1 == 1) && ((st1.st_mode & S_IFMT) == S_IFCHR)) && ((nExist2 == 1) && ((st2.st_mode & S_IFMT) == S_IFCHR)))
		{
			Common_GetSystemCount(&nLstGuard4GDriverTime, NULL);
			Common_GetSystemCount(&nCurGuard4GDriverTime, NULL);
			nRet = 0;
		}
		else
		{
			Common_GetSystemCount(&nCurGuard4GDriverTime, NULL);
			nRet = -1;
		}
		
		if(nCurGuard4GDriverTime >= nLstGuard4GDriverTime + 120)
		{
			LOGE("Guard4GDriverFile failed. reboot system\n");
			Common_System("lsusb; ls -lh /dev/ttyUSB*; ifconfig -a; route -n");
			RebootSystem();
			Common_Sleep(10, 0);//设备重启有3秒延迟,不加睡眠会导致3秒延迟一直被设置
			nRet = -1;
		}
	}
	
	LOGW("guard 4g driver file end\n");
	
	return nRet;
}

static void RebootLteModel()
{
	LOGW("Reboot 4G begin\n");
	
	Common_Sleep(3,0);
	
    char cmd[64];
	
	if(g_LteCfg.n4GFactory == 3)
	{
		memset(cmd, 0, sizeof(cmd));
    	sprintf(cmd, "echo -e \"AT+RESET\r\n\" > /dev/at_cmd");
    	Common_System(cmd);
	}
	else
	{
		memset(cmd, 0, sizeof(cmd));
    	sprintf(cmd, "echo -e \"AT+CFUN=1,1\r\n\" > /dev/at_cmd");
    	Common_System(cmd);
	}
	
	Common_Sleep(10,0);
	
	while(1)
	{
		if((Guard4GDriverFile() != 0) || (Guard4GNetcard() != 0))
		{
			Common_Sleep(1,0);
            continue;
		}
		
		break;
	}
	
	//g_n4GConnectStatus = -1;
    g_n4GNotPlugCount = 0;

	Set4GModelLight();
	
	LOGW("Reboot 4G succ\n");
}

static void ResetLteModel()
{
#if 0
	LOGW("Reset 4G begin\n");

	char cmd[64];
	
#ifdef PLATFORM_JZT30 
    sprintf(cmd, "echo 0 > /sys/class/gpio/gpio14/value");
    Common_System(cmd);
	
	Common_Sleep(1,0);

	sprintf(cmd, "echo 1 > /sys/class/gpio/gpio14/value");
    Common_System(cmd);
#elif PLATFORM_JZT41
	sprintf(cmd, "echo 0 > /sys/class/gpio/gpio62/value");
    Common_System(cmd);

	Common_Sleep(1,0);

	sprintf(cmd, "echo 1 > /sys/class/gpio/gpio62/value");
    Common_System(cmd);
#endif

	Common_Sleep(10,0);

	while(1)
	{
		if(access("/dev/at_cmd", F_OK) == 0)
			break;
		
		Common_Sleep(1,0);
	}
	
	Guard4GNetcard();

	//g_n4GConnectStatus = -1;
    g_n4GNotPlugCount = 0;
	
	Set4GModelLight();
	
	LOGW("Reset 4G succ\n");
#endif
}

int RebootSystem()
{
    cJSON_Struct *pInData = NULL,*pOutData = NULL;

    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/Core/Power/Reboot",0,0);
    Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
    Common_Json_SetAttrValue(pInData,-1,"Data",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pInData,-1,"Data/Delay",Common_Json_Type_Number,NULL,3,0);

	LOGE("No find 4G netcard!!!\n");
    Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);

    Common_Json_Delete(pInData);
    Common_Json_Delete(pOutData);
	
    return 0;
}

// 0--"4G设备联网成功，祝您使用愉快"
// 1--"4G设备联网失败"
// 2--"4G设备联网中，请您稍等"
// 3--"复位成功"
// 4--"请插入4G卡"
// 5--"非法4G卡插入，请检查"
// 6--"域名解析失败"
int Play_Sound(int soundIndex,int times,int isStart)
{
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    char acSoundPath[128] = {0};

	int nEthNetIn = !ovfs_utility_is_net_dev_out("eth0");
	if(nEthNetIn && (s_nWiredExtranet == 1))
	{
		return 0;
	}

    LOGI("play sound soundIndex=[%d],times=[%d],isStart=[%d]\n",soundIndex,times,isStart);

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);

        if(isStart)
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/BoardSys/Audio/Adec/PlayFile",0,0);
        }
        else
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/BoardSys/Audio/Adec/StopPlayFile",0,0);
        }

        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        
        sprintf(acSoundPath, "/update/soundFile/sound_4g_%d", soundIndex);
        Common_Json_SetAttrValue(pRoot,-1,"Data/Path",Common_Json_Type_String,acSoundPath,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/Times",Common_Json_Type_Number,NULL,times,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/Priority",Common_Json_Type_Number,NULL,1,0);
        iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(), pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
    }

    return 0;
}

int is4gGunDomeDevice()
{
	S32 iRet = -1;
	S8 *szStringVal = NULL;
	S8 acDeviceString[128] = {0};
	cJSON_Struct *pInData = NULL,*pOutData = NULL;
	cJSON_Struct *pItem = NULL;
	
	pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pInData == NULL)
	{
		LOGE("Common_Json_New failed\n");
		return -1;
	}
	
	Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/Core/Version",0,0);
	Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"get",0,0);

	iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
	if(0 != iRet)
	{
		LOGE("Module_CallFunctions failed. ret=[%d]\n",iRet);
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);
		return -1;
	}

	pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/DeviceTypeString", NULL, &szStringVal, NULL, NULL);
	if(pItem == NULL || szStringVal == NULL)
	{
		LOGE("Common_Json_GetAttrValue failed\n");
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);	
		return -1;
	}
	
	strcpy(acDeviceString, szStringVal);

	Common_Json_Delete(pInData);
	Common_Json_Delete(pOutData);

	if(strstr(acDeviceString, "ipc_gundome_4g") != NULL)
	{
		return 1;
	}
	
	return 0;
}

U64 str2num(char *pString,char *pNumStr)
{
    U64 res = 0;
    U8 i=0,idx=0;
    if((NULL == pString) || (NULL == pNumStr)) return 0;
    char *p = pString;
    for(i=0; i<strlen(pString); i++)
    {
        if(*p >= '0' && *p <= '9')
        {
            pNumStr[idx++] = *p;
            p++;
        }
        else
        {
            if(idx > 0)
            {
                pNumStr[idx] = 0;
                break;
            }
            p++;
        }
    }
    res = atoll(pNumStr);
    return res;
}

static int ovfs_get_apn(char *pMCC, char *pMNC, char *pAPN)
{
	int ret = 0;
    if(pMCC == NULL || pMNC == NULL || pAPN == NULL)
    {
        printf("invalid param\n");
        return ret;;
    }

    FILE *fp = NULL;
	mxml_node_t *root = NULL;
	mxml_node_t *apns = NULL;
	mxml_node_t *apn = NULL;
	
	fp = fopen("/root/ppp/apns-conf.xml","r");
	if(fp == NULL)
	{
		printf("open file failed\n");
		return ret;
	}
	
	root = mxmlLoadFile(NULL,NULL,fp);
	if(root == NULL)
	{
		printf("mxmlLoadFile failed\n");
		fclose(fp);
		return ret;
	}
	
	apns = mxmlFindElement(root,root,"apns",NULL,NULL,MXML_DESCEND_ALL);
	if(apns == NULL)
	{
		printf("mxmlFindElement failed\n");
		mxmlDelete(root);
		fclose(fp);
		return ret;
	}
	
	apn = mxmlFindElement(apns,root,"apn",NULL,NULL,MXML_DESCEND_ALL);
	
	while(apn)
	{
		if(strcmp(mxmlElementGetAttr(apn,"mcc"),pMCC) == 0 && strcmp(mxmlElementGetAttr(apn,"mnc"),pMNC) == 0)
		{
			sprintf(pAPN,"%s",mxmlElementGetAttr(apn,"apn"));

			ret = 1;
		
			break;
		}
		
		apn = mxmlFindElement(apn,apns,"apn",NULL,NULL,MXML_DESCEND_ALL);
	}
	
	
	mxmlDelete(root);
	fclose(fp);
    return ret;
}

int Read_4GCustomer(char *pCustomerStr, char *pCustomIDStr)
{
	U32 nStrLen = 0;
	FILE *fp = NULL;
	char *pCustomer = NULL;
	char *pCustomID = NULL;
	char *pConfigString = NULL;
	cJSON_Struct *pConfigJson = NULL;

	fp = fopen(str_4gCustomerCfg,"rb");
	if (fp == NULL)
	{
		//LOGE("fopen file %s failed\n", str_4gCustomerCfg);
		return -1;
	}
	
	fseek(fp,0,SEEK_END);
	nStrLen = ftell(fp);
	fseek(fp,0,SEEK_SET);
	if (nStrLen <= 0)
	{
		//LOGE("file %s len is %d\n", str_4gCustomerCfg, nStrLen);
		fclose(fp);
		return -1;
	}
	
	pConfigString = (char *)Common_Malloc(nStrLen+1,0,__FUNCTION__,__LINE__);
	if (pConfigString != NULL)
	{
		if(nStrLen == fread(pConfigString,1,nStrLen,fp))
		{				
			pConfigJson = Common_Json_Parse(pConfigString,NULL,NULL);
		}
	}
	
	fclose(fp);
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}

	Common_Json_GetAttrValue(pConfigJson,-1,"Customer",NULL,&pCustomer,NULL,NULL);
    if(pCustomer == NULL)
    {
    	Common_Strncpy(pCustomerStr, (char*)"", strlen((char*)""));
	}
	else
    {
    	Common_Strncpy(pCustomerStr, pCustomer, strlen(pCustomer));
    }

	Common_Json_GetAttrValue(pConfigJson,-1,"CustomerID",NULL,&pCustomID,NULL,NULL);
    if(pCustomID == NULL)
    {
    	Common_Strncpy(pCustomIDStr, (char*)"000", strlen((char*)"000"));
	}
	else
    {
    	Common_Strncpy(pCustomIDStr, pCustomID, strlen(pCustomID));
    }

	if (pConfigJson != NULL)
	{
		Common_Json_Delete(pConfigJson);
		pConfigJson = NULL;
	}

	return 0;
}

int Read_4GBindInfo(char *pCustomerStr, char *pCustomIDStr, char *pCcidStr, char *pNetInfo)
{
	U32 nStrLen = 0;
	FILE *fp = NULL;
	char *pTempStr = NULL;
	char *pConfigString = NULL;
	cJSON_Struct *pConfigJson = NULL;

	if(pCustomerStr == NULL || pCustomIDStr == NULL || pCcidStr == NULL || pNetInfo == NULL)
	{
		//LOGE("invalid param\n");
		return -1;
	}

	fp = fopen(str_4gCcidCfg,"rb");
	if (fp == NULL)
	{
		//LOGE("fopen file %s failed\n", str_4gCcidCfg);
		return -1;
	}
	
	fseek(fp,0,SEEK_END);
	nStrLen = ftell(fp);
	fseek(fp,0,SEEK_SET);
	if (nStrLen <= 0)
	{
		LOGE("file %s len is %d\n", str_4gCcidCfg, nStrLen);
		fclose(fp);
		return -1;
	}
	
	pConfigString = (char *)Common_Malloc(nStrLen+1,0,__FUNCTION__,__LINE__);
	if (pConfigString != NULL)
	{
		if(nStrLen == fread(pConfigString,1,nStrLen,fp))
		{				
			pConfigJson = Common_Json_Parse(pConfigString,NULL,NULL);
		}
	}
	
	fclose(fp);
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}

	pTempStr = NULL;
	Common_Json_GetAttrValue(pConfigJson,-1,"Customer",NULL,&pTempStr,NULL,NULL);
	if(pTempStr == NULL)
    {
    	Common_Strncpy(pCustomerStr, (char*)"", strlen((char*)""));
	}
	else
    {
    	Common_Strncpy(pCustomerStr, pTempStr, strlen(pTempStr));
    }

	pTempStr = NULL;
	Common_Json_GetAttrValue(pConfigJson,-1,"CustomerID",NULL,&pTempStr,NULL,NULL);
	if(pTempStr == NULL)
    {
    	Common_Strncpy(pCustomIDStr, (char*)"", strlen((char*)""));
	}
	else
    {
    	Common_Strncpy(pCustomIDStr, pTempStr, strlen(pTempStr));
    }

	pTempStr = NULL;
	Common_Json_GetAttrValue(pConfigJson,-1,"Ccid",NULL,&pTempStr,NULL,NULL);
	if(pTempStr == NULL)
    {
    	Common_Strncpy(pCcidStr, (char*)"", strlen((char*)""));
	}
	else
    {
    	Common_Strncpy(pCcidStr, pTempStr, strlen(pTempStr));
    }

	pTempStr = NULL;
	Common_Json_GetAttrValue(pConfigJson,-1,"NetInfo",NULL,&pTempStr,NULL,NULL);
	if(pTempStr == NULL)
    {
    	Common_Strncpy(pNetInfo, (char*)"", strlen((char*)""));
	}
	else
    {
    	Common_Strncpy(pNetInfo, pTempStr, strlen(pTempStr));
    }

	if (pConfigJson != NULL)
	{
		Common_Json_Delete(pConfigJson);
		pConfigJson = NULL;
	}

	return 0;
}

int Read_LteDevInfo(char *pUsbNameStr)
{
	FILE *fp = NULL;
	char acLteDevName[8] = {0};
	
	if(pUsbNameStr == NULL)
	{
		//LOGE("invalid param\n");
		return -1;
	}
	
	if(access(str_4gDevFile,F_OK) != 0)
	{
		strcpy(pUsbNameStr,"usb0");
		return 0;
	}
	
	fp = fopen(str_4gDevFile,"rb");
	if (fp == NULL)
	{
		strcpy(pUsbNameStr,"usb0");
		return 0;
	}
	
	fread(acLteDevName,1,sizeof(acLteDevName),fp);
	
	fclose(fp);

	if(strlen(acLteDevName) <= 0)
	{
		strcpy(pUsbNameStr,"usb0");
		return 0;
	}
	
	if((acLteDevName[strlen(acLteDevName) - 1] == '\n'))
	{
		strncpy(pUsbNameStr, acLteDevName, strlen(acLteDevName) - 1);
	}
	
	return 0;
}

int Write_4GBindInfo(char *pCustomerStr, char *pCustomIDStr, char *pCcidStr, char *pNetInfo)
{
	FILE *fp = NULL;
	char *pConfigString = NULL;
	cJSON_Struct *pConfigJson = NULL;

	if(pCustomerStr == NULL || pCustomIDStr == NULL || pCcidStr == NULL || pNetInfo == NULL)
	{
		LOGE("invalid param\n");
		return -1;
	}

	pConfigJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	Common_Json_SetAttrValue(pConfigJson,-1,"Customer",Common_Json_Type_String,pCustomerStr,0,0);
	Common_Json_SetAttrValue(pConfigJson,-1,"CustomerID",Common_Json_Type_String,pCustomIDStr,0,0);
	Common_Json_SetAttrValue(pConfigJson,-1,"Ccid",Common_Json_Type_String,pCcidStr,0,0);
	Common_Json_SetAttrValue(pConfigJson,-1,"NetInfo",Common_Json_Type_String,pNetInfo,0,0);
	pConfigString = Common_Json_Print(pConfigJson,NULL);
	if (pConfigJson != NULL)
	{
		Common_Json_Delete(pConfigJson);
		pConfigJson = NULL;
	}

	fp = fopen(str_4gCcidCfg,"w+");

	fwrite(pConfigString, strlen(pConfigString), 1, fp);
	fclose(fp);

	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}

	return 0;
}

int Network_Put_4GSpecified(cJSON_Struct* in)
{
	//g_n4GCardSpecified = 0;
	return 0;
}

int Network_Put_SpecifiedCard(cJSON_Struct* in)
{
	int nStatus = -1;
    Common_Json_GetAttrValue(in,-1,"Status",NULL,NULL,&nStatus,NULL);
	g_n4GCardSpecified = nStatus;
	g_n4GNotSpecifiedCount = 0;
	g_n4GDomainFailCount = 0;
	g_n4GConnectSuccCount = 0;
	return 0;
}

int NetWork_Get_4GCustomer_Json(cJSON_Struct* out)
{
	int nRet = 0;
	char acCustomer[32] = {0};
	char acCustomID[32] = {0};
	
	if(!g_LteCfg.bInit)
	{
		Common_Json_SetAttrValue(out,-1,"Customer",Common_Json_Type_String,"",0,0);
    	Common_Json_SetAttrValue(out,-1,"CustomerID",Common_Json_Type_String,"000",0,0);
		Common_Json_SetAttrValue(out,-1,"UseCCID",Common_Json_Type_String,"",0,0);
		
		Common_Json_SetAttrValue(out,-1,"IsBind",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(out,-1,"BindCCID",Common_Json_Type_String,"",0,0);
		Common_Json_SetAttrValue(out,-1,"BindNetInfo",Common_Json_Type_String,"",0,0);
		Common_Json_SetAttrValue(out,-1,"UseNetInfo",Common_Json_Type_String,"",0,0);
	}
	else
	{
		nRet = Read_4GCustomer(acCustomer, acCustomID);
		if(nRet == 0 && strlen(acCustomer) && strlen(acCustomID) && strcmp(acCustomID, "000"))
		{
			Common_Json_SetAttrValue(out,-1,"Customer",Common_Json_Type_String,acCustomer,0,0);
	    	Common_Json_SetAttrValue(out,-1,"CustomerID",Common_Json_Type_String,acCustomID,0,0);
			Common_Json_SetAttrValue(out,-1,"UseCCID",Common_Json_Type_String,g_LteCfg.strCCID,0,0);
			
			Common_Json_SetAttrValue(out,-1,"IsBind",Common_Json_Type_Number,NULL,0,0);
			Common_Json_SetAttrValue(out,-1,"BindCCID",Common_Json_Type_String,"",0,0);
			Common_Json_SetAttrValue(out,-1,"BindNetInfo",Common_Json_Type_String,"",0,0);
			Common_Json_SetAttrValue(out,-1,"UseNetInfo",Common_Json_Type_String,g_LteCfg.strNetInfo,0,0);
		}
		else
		{
			Common_Json_SetAttrValue(out,-1,"Customer",Common_Json_Type_String,"",0,0);
	    	Common_Json_SetAttrValue(out,-1,"CustomerID",Common_Json_Type_String,"000",0,0);
			Common_Json_SetAttrValue(out,-1,"UseCCID",Common_Json_Type_String,g_LteCfg.strCCID,0,0);
			
			Common_Json_SetAttrValue(out,-1,"IsBind",Common_Json_Type_Number,NULL,0,0);
			Common_Json_SetAttrValue(out,-1,"BindCCID",Common_Json_Type_String,"",0,0);
			Common_Json_SetAttrValue(out,-1,"BindNetInfo",Common_Json_Type_String,"",0,0);
			Common_Json_SetAttrValue(out,-1,"UseNetInfo",Common_Json_Type_String,g_LteCfg.strNetInfo,0,0);
		}
	}
	
	return 0;
}

int NetWork_Bind_LteCfg()
{
#if 0
	int nRet = 0;
	char acCustomer[32] = {0};
	char acCustomID[32] = {0};
	char acCcidStr[32] = {0};
	char acNetInfo[32] = {0};

	nRet = strlen(g_LteCfg.strCCID);
	if(nRet == 0)
	{
		LOGE("can not get ccid\n");
		return -1;
	}
	
	nRet = Read_4GBindInfo(acCustomer, acCustomID, acCcidStr, acNetInfo);
	if(nRet == -1 || strlen(acCcidStr) == 0)
	{
		nRet = Read_4GCustomer(acCustomer, acCustomID);
		if(nRet == 0 && strlen(acCustomer) && strlen(acCustomID) && strcmp(acCustomID, "000"))
		{
			Write_4GBindInfo(acCustomer, acCustomID, g_LteCfg.strCCID, g_LteCfg.strNetInfo);
			g_n4GUseless = 0;
			g_n4GNotSpecifiedCount = 0;
		}
		return 0;
	}	
	
	if(!strcmp(acCcidStr, g_LteCfg.strCCID))
	{
		g_n4GNotSpecifiedCount = 0;
		return 0;
	}
	else
	{
		LOGE("ccid change. old ccid[%s]--new ccid[%s]\n",acCcidStr,g_LteCfg.strCCID);
		if(g_n4GNotSpecifiedCount % 12 == 0)
		{
			Play_Sound(5,1,1);
		}
		g_n4GNotSpecifiedCount++;
		return -1;
	}
#endif	
	return 0;
}

int NetWork_Bind_Ccid(cJSON_Struct* in)
{
#if 0
	int nRet= 0;
	char *pCustomer = NULL;
	char *pCustomerID = NULL;
	char acCustomer[32] = {0};
	char acCustomID[32] = {0};
	char acCcidStr[32] = {0};
	char acNetInfo[32] = {0};

	Common_Json_GetAttrValue(in,-1,"Customer",NULL,&pCustomer,NULL,NULL);

	Common_Json_GetAttrValue(in,-1,"CustomerID",NULL,&pCustomerID,NULL,NULL);

	if(pCustomer == NULL || pCustomerID == NULL)
	{
		LOGE("invalid param\n");
		return -1;
	}

	nRet = strlen(g_LteCfg.strCCID);
	if(nRet == 0)
	{
		LOGE("can not get ccid\n");
		return -1;
	}

	nRet = Read_4GBindInfo(acCustomer, acCustomID, acCcidStr, acNetInfo);
	if(nRet == 0 && strlen(acCcidStr))
	{
		return 0;
	}

	nRet = Read_4GCustomer(acCustomer, acCustomID);
	if(nRet == 0 && strlen(acCustomer) && strlen(acCustomID) && strcmp(acCustomID, "000"))
	{
		LOGE("invalid custom. acCustomer=[%s],acCustomID=[%s]\n",acCustomer,acCustomID);
		return -1;
	}

	Write_4GBindInfo(pCustomer, pCustomerID, g_LteCfg.strCCID, g_LteCfg.strNetInfo);
	g_n4GUseless = 0;
	g_n4GNotSpecifiedCount = 0;
#endif
	return 0;
}

int NetWork_Unbind_Ccid(cJSON_Struct* in)
{
#if 0
	int nRet= 0;
	int nUnbind = 0;
	char *pCustomer = NULL;
	char *pCustomerID = NULL;
	char acCustomer[32] = {0};
	char acCustomID[32] = {0};
	char acCcidStr[32] = {0};
	char acNetInfo[32] = {0};

	nRet = Read_4GCustomer(acCustomer, acCustomID);
	if(nRet != 0 || !strlen(acCustomer) || !strlen(acCustomID) || !strcmp(acCustomID, "000"))
	{
		nRet = Read_4GBindInfo(acCustomer, acCustomID, acCcidStr, acNetInfo);
		if(nRet != 0 || !strlen(acCustomer) || !strlen(acCustomID) || !strcmp(acCustomID, "000"))
		{
			return 0;
		}
	}

	Common_Json_GetAttrValue(in,-1,"Customer",NULL,&pCustomer,NULL,NULL);

	Common_Json_GetAttrValue(in,-1,"CustomerID",NULL,&pCustomerID,NULL,NULL);

	if(pCustomer == NULL && pCustomerID != NULL && strcmp(acCustomID, pCustomerID) == 0)
	{
		nUnbind = 1;
	}

	if(pCustomer != NULL && strcmp(acCustomer, pCustomer) == 0 && pCustomerID == NULL)
	{
		nUnbind = 1;
	}

	if(pCustomer != NULL && strcmp(acCustomer, pCustomer) == 0 && pCustomerID != NULL && strcmp(acCustomID, pCustomerID) == 0)
	{
		nUnbind = 1;
	}

	if(nUnbind == 0)
	{
		return -1;	
	}

	remove(str_4gCcidCfg);

	g_n4GUseless = 1;
#endif	
	return 0;
}

static void ovfs_change_apn()
{
	int nApnSucc = 0;
    char buf[1024] = {0}, buf2[256] = {0}, cimi[32] = {0}, apn[128] = {0},mcc[6] = {0}, mnc[6] = {0}, *pstrOk = NULL;

	if(g_LteCfg.nApnType == 1)
	{
		if(strlen(g_LteCfg.acApnStr))
		{
			strncpy(apn, g_LteCfg.acApnStr, strlen(g_LteCfg.acApnStr));
			nApnSucc = 1;
		}
	}
	else
	{
	    if (0 == Lte_Exe_Cmd("AT+CIMI", 1000, buf, sizeof(buf) - 1))
	    {
	        pstrOk = strstr(buf,"OK");
	    }
		
	    if(!pstrOk)
	    {
	        if (0 == Lte_Exe_Cmd("AT+CIMI?", 1000, buf, sizeof(buf) - 1))
	        {
	            pstrOk = strstr(buf,"OK");
	        }   
	    }
	        
	    if(pstrOk && (pstrOk - buf) >= 4)
	    {
	        str2num(buf + 2, cimi);

	        strncpy(mcc, cimi, 3);
	        strncpy(mnc, cimi+3, 2);

	        if(ovfs_get_apn(mcc,mnc,apn))
	        {
	        	memset(g_LteCfg.acApnStr, 0, sizeof(g_LteCfg.acApnStr));
	        	strncpy(g_LteCfg.acApnStr, apn, strlen(apn));
				nApnSucc = 1;
	        }
	    }
	}

	if(nApnSucc == 1)
	{
		if(g_LteCfg.n4GFactory == 1)
		{
			pstrOk = NULL;
			if (0 == Lte_Exe_Cmd("AT+CGDCONT?", 1000, buf, sizeof(buf) - 1))
			{
				pstrOk = strstr(buf,"OK");
			}
			
			if(!pstrOk || !strstr(buf, g_LteCfg.acApnStr))
			{
				sprintf(buf2, "AT*CGDFLT=1,\"IP\",\"%s\",,,,,,,,,,,,,,,,,,1", apn);
				Lte_Exe_Cmd(buf2,1000,buf,sizeof(buf) - 1);

				//拔插usb 线需要重启后才能拨号
				RebootLteModel();
			}
		}
		else if(g_LteCfg.n4GFactory == 2 || g_LteCfg.n4GFactory == 3)
		{
			pstrOk = NULL;
			if (0 == Lte_Exe_Cmd("AT+QICSGP=1", 1000, buf, sizeof(buf) - 1))
			{
				pstrOk = strstr(buf,"OK");
			}
			
			if(!pstrOk || !strstr(buf, g_LteCfg.acApnStr))
			{
				sprintf(buf2, "AT+QICSGP=1,1,\"%s\",\"\",\"\",0", apn);
				Lte_Exe_Cmd(buf2,1000,buf,sizeof(buf) - 1);

				RebootLteModel();
			}
		}
	}
	
    return;
}

void ovfs_reset_4g_info()
{
	memset(g_LteCfg.strCCID,0,sizeof(g_LteCfg.strCCID));
    memset(g_LteCfg.strNetInfo,0,sizeof(g_LteCfg.strNetInfo));
    memset(g_LteCfg.strServiceInfo,0,sizeof(g_LteCfg.strServiceInfo));
	memset(g_LteCfg.strIPv4,0,sizeof(g_LteCfg.strIPv4));
    memset(g_LteCfg.strIPv6,0,sizeof(g_LteCfg.strIPv6));
    memset(g_LteCfg.strDNS1v4,0,sizeof(g_LteCfg.strDNS1v4));
    memset(g_LteCfg.strDNS1v6,0,sizeof(g_LteCfg.strDNS1v6));
    memset(g_LteCfg.strDNS2v4,0,sizeof(g_LteCfg.strDNS2v4));
    memset(g_LteCfg.strDNS2v6,0,sizeof(g_LteCfg.strDNS2v6));

    snprintf(g_LteCfg.strIPv4,sizeof(g_LteCfg.strIPv4),"NA");
    snprintf(g_LteCfg.strIPv6,sizeof(g_LteCfg.strIPv6),"NA");
	
	g_LteCfg.uSignalStrength = 0;
	g_LteCfg.uGPRS_Attach = 0;

	return;
}

int GetAllDns(char *path, ovfs_all_dns * pAllDns)
{
	if(path == NULL || pAllDns == NULL)
		return -1;
	
	FILE *fp = fopen(path, "r");
	if(fp == NULL)
		return -1;

	char sz_param[256];
	while(fgets(sz_param, sizeof(sz_param), fp) != NULL)
	{
		char *pstr = strstr(sz_param, "nameserver ");
		char *pstr1 = strstr(sz_param, "\n");
		if(NULL == pstr ||NULL == pstr1)
		{
			continue;
		}
		
		if(strstr(sz_param, ".") != NULL && (pAllDns->nDnsNum < (int)(sizeof(pAllDns->ipv4)/sizeof(pAllDns->ipv4[0]))))
		{
			pstr += strlen("nameserver ");
			*pstr1 = '\0';
			snprintf(pAllDns->ipv4[pAllDns->nDnsNum], sizeof(pAllDns->ipv4[pAllDns->nDnsNum]), "%s", pstr);
			pAllDns->nDnsNum++;
		}
	}
	
	fclose(fp);
	return 0;
}

int Is_Custom_fac()
{
	int nRet = 0,nIsCustomFac = 0;
	char acCustomer[32] = {0};
	char acCustomID[32] = {0};

	nRet = Read_4GCustomer(acCustomer, acCustomID);
	if(nRet == 0 && strlen(acCustomer) && strlen(acCustomID) && strcmp(acCustomID, "000"))
	{
		nIsCustomFac = 1;
	}

	return nIsCustomFac;
}

int killProgress(char *pProgressName)
{
	int nPid = -1;
	char acReqBuf[128];
	char acResBuf[128];

	if(NULL == pProgressName)
	{
		return -1;
	}

	memset(acReqBuf, 0, sizeof(acReqBuf));
	memset(acResBuf, 0, sizeof(acResBuf));
	sprintf(acReqBuf, "ps | grep \"%s\" | grep -v \"grep\" | awk \'{print $1}\' ", pProgressName);
	
	Common_Exe_Cmd(acReqBuf, 3000, acResBuf, sizeof(acResBuf));

	if (strlen(acResBuf) > 0)
	{
		nPid = atoi(acResBuf);
	}

	if (0 < nPid)
	{
		memset(acReqBuf, 0, sizeof(acReqBuf));
		snprintf(acReqBuf, sizeof(acReqBuf) - 1, "kill -SIGUSR2 %d ; kill -9 %d ;", nPid, nPid);
		Common_System(acReqBuf);
		
		if(strstr(pProgressName, "udhcpc"))
		{
			snprintf(acReqBuf, sizeof(acReqBuf), "rm /var/run/udhcpc_%s.pid",str_RNDISifname);
			Common_System(acReqBuf);
		}
	}

	return 0;
}

int CalcFileSize(char *pFileName)
{
	FILE *fp = NULL;
	fp = fopen(pFileName,"rb");
	if (fp == NULL)
	{
		return 0;
	}
	
	fseek(fp,0,SEEK_END);
	int nStrLen = ftell(fp);
	fseek(fp,0,SEEK_SET);
	if (nStrLen <= 0)
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return nStrLen;
}

size_t HttpHeadCallBack(void* buffer, size_t size, size_t nmemb, void* userp)
{
	HttpHeader* pHttpHeader = (HttpHeader*)(userp);

	if (NULL == pHttpHeader)
	{
		return size * nmemb;
	}

	sscanf((char*)buffer, "HTTP/1.%*c %3d", &(pHttpHeader->nHttpCode));

	return size * nmemb;
}

size_t HttpBodyCallBack(void* buffer, size_t size, size_t nmemb, void* userp)
{
	HttpBody* pHttpBody = (HttpBody*)(userp);

	if (NULL == pHttpBody)
	{
		return size * nmemb;
	}

	if (pHttpBody->dwOffset + size * nmemb > pHttpBody->dwTotoalSize)
	{
		pHttpBody->dwFull = 1;
	}
	else
	{
		memcpy(pHttpBody->acDataBuf + pHttpBody->dwOffset, buffer, size * nmemb);

		pHttpBody->dwOffset += (size * nmemb);
	}

	return size * nmemb;
}

int ReportedLogFile(const char* pSuperHost, const U32 wHttpPort, const char* pSerialNo, const char* pLogFilePath, const char* pUploadFileName, const U32 dwTimeout)
{
	time_t t = 0;
	char tmp[32];
	char acURL[256];
	char acUploadFileName[64];
	CURL* pCurl = NULL;
	struct curl_httppost* formpost = NULL;
	struct curl_httppost* lastptr = NULL;
	CURLcode curlCode = CURLE_OK;
	HttpBody* pHttpBody = NULL;
	HttpHeader* pHttpHeader = NULL;

	if (NULL == pSuperHost || 0 == wHttpPort || NULL == pSerialNo || !strlen(pSerialNo) || NULL == pLogFilePath || NULL == pUploadFileName)
	{
		LOGE("input parameter invalid\n");

		return 0;
	}

	//! 初始化HTTP-POST请求
	pHttpBody = new HttpBody;
	pHttpHeader = new HttpHeader;
	
	if (NULL == pHttpBody || NULL == pHttpHeader)
	{
		LOGE("create HttpBody(%p)/HttpHeader(%p) invalid\n", pHttpBody, pHttpHeader);

		if(pHttpBody)
			delete(pHttpBody);

		if(pHttpHeader)
			delete(pHttpHeader);

		return 0;
	}

	memset(pHttpHeader, 0, sizeof(HttpHeader));
	memset(pHttpBody, 0, sizeof(HttpBody));

	// 修改日志文件名,以序列号_yyyymmddhhmmss.log格式的文件名上传至文件服务器
	memset(tmp, 0, sizeof(tmp));
	memset(acURL, 0, sizeof(acURL));
	memset(acUploadFileName, 0, sizeof(acUploadFileName));

	pHttpBody->dwFull = 0;
	pHttpBody->dwOffset = 0;
	pHttpBody->dwTotoalSize = sizeof(pHttpBody->acDataBuf);

	t = time(0);
	strftime(tmp, sizeof(tmp), "%Y%m%d%H%M%S", localtime(&t));

	sprintf(acUploadFileName, "%s-%s-%s.log", pSerialNo, tmp, pUploadFileName);
	sprintf(acURL, UPLOAD_LOGFILE_URL, pSuperHost, wHttpPort);

	pCurl = curl_easy_init();

	if (NULL == pCurl)
	{
		LOGE("call curl_easy_init() error\n");

		if(pHttpBody)
			delete(pHttpBody);

		if(pHttpHeader)
			delete(pHttpHeader);

		return 0;
	}

	curl_easy_setopt(pCurl, CURLOPT_URL, acURL);
	curl_easy_setopt(pCurl, CURLOPT_DNS_SERVERS, "8.8.8.8,8.8.4.4,223.5.5.5,223.6.6.6");
	curl_easy_setopt(pCurl, CURLOPT_USERAGENT, pSerialNo);
	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, (dwTimeout >> 16) & 0xFFFF);
	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, dwTimeout & 0xFFFF);
	curl_easy_setopt(pCurl, CURLOPT_USERPWD, UPLOAD_AUTH_DATA);
	curl_easy_setopt(pCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(pCurl, CURLOPT_WRITEHEADER, (void*)pHttpHeader);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void*)pHttpBody);
	curl_easy_setopt(pCurl, CURLOPT_HEADERFUNCTION, HttpHeadCallBack);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, HttpBodyCallBack);

	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, pLogFilePath, CURLFORM_FILENAME, acUploadFileName, CURLFORM_END);

	curl_easy_setopt(pCurl, CURLOPT_HTTPPOST, formpost);
	
	curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1L);

	//! 设置解析域名为ipv4地址
	curl_easy_setopt(pCurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);

	curlCode = curl_easy_perform(pCurl);

	if (CURLE_OK != curlCode)
	{
		LOGE("call curl_easy_perform(%s) error(%d:%s)\n", acURL, curlCode, curl_easy_strerror(curlCode));

		curl_formfree(formpost);

		curl_easy_cleanup(pCurl);

		if(pHttpBody)
			delete(pHttpBody);

		if(pHttpHeader)
			delete(pHttpHeader);

		return 0;
	}

	curl_formfree(formpost);

	curl_easy_cleanup(pCurl);

	if (200 != pHttpHeader->nHttpCode)
	{
		LOGE("call curl_easy_perform(%s) error(%d)\n", acURL, pHttpHeader->nHttpCode);

		if(pHttpBody)
			delete(pHttpBody);

		if(pHttpHeader)
			delete(pHttpHeader);

		return 0;
	}
	
	{
		//! 检查HTTP响应数据
		cJSON_Struct *pRoot = Common_Json_Parse(pHttpBody->acDataBuf, NULL, NULL);

		if (NULL == pRoot)
		{
			LOGE("http response data format error\n");

			if(pHttpBody)
				delete(pHttpBody);

			if(pHttpHeader)
				delete(pHttpHeader);

			return 0;
		}

		int nResultCode = 0;
		Common_Json_GetAttrValueInt(pRoot, "resultCode", &nResultCode);
		
		if (0 != nResultCode)
		{
			LOGE("http response result-code(%d) error\n", nResultCode);

			Common_Json_Delete(pRoot);

			if(pHttpBody)
				delete(pHttpBody);

			if(pHttpHeader)
				delete(pHttpHeader);

			return 0;
		}

		Common_Json_Delete(pRoot);
	}

	if(pHttpBody)
		delete(pHttpBody);

	if(pHttpHeader)
		delete(pHttpHeader);

	return 1;
}


static S32 status_4g_thread(Common_Thread_T hThreadHandle,void* para)
{
	ovfs_all_dns struAllDns;
	int failed_max_num = 6;
	int npppNetOut = 0;
	int npppNetUp = 0;
	int ping_count = 0;
	int ping_4g_failed_cnt = 0;
	int n4gThreadLstTime = 0;
	int nSleepCount = 0;
	int nWriteCount = 0;
	int nMaxSleepCount = 30;
	int nMaxWriteCount = 10;
	char acLogBuf[128] = {0};
	char acBuildDate[128] = {0};
	char acProductDate[128] = {0};
	char acSerialNumber[128] = {0};
	char acDeviceName[128] = {0};
//	S8 system_cmd[128] = {0};

	while(!g_status_thread_exit)
	{
		//4g 的AT 和拨号线程卡主, 重启设备
		{
			Common_GetSystemCount(&n4gThreadLstTime, NULL);
			if((n4gThreadLstTime != 0) && (g_n4gThreadCurTime != 0) && (n4gThreadLstTime > g_n4gThreadCurTime + 120))
			{
				LOGE("at commond block (%d > %d + 120). reboot system\n",n4gThreadLstTime,g_n4gThreadCurTime);
				RebootSystem();
				Common_Sleep(10, 0);//设备重启有3秒延迟,不加睡眠会导致3秒延迟一直被设置
			}
		}
		
		if(!ovfs_utility_is_net_dev_out("eth0"))
		{
			Common_Sleep(10, 0);
			continue;
		}
		
		if(g_LteCfg.bEnable == 0 || g_LteCfg.bSIMCardStatus == 0)
		{
			ping_4g_failed_cnt = 0;
			ping_count = 0;
			nSleepCount = 0;
			Common_Sleep(10,0);
			continue;
		}
		
		if(strlen(acBuildDate) == 0 || strlen(acProductDate) == 0 || strlen(acSerialNumber) == 0)
		{
			cJSON_Struct *pInData = NULL,*pOutData = NULL;
			cJSON_Struct *pItem = NULL;
			char *szStringVal = NULL;
			
			pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pInData != NULL)
			{
				Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/Core/Version",0,0);
				Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
				
				int iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
				if(0 == iRet)
				{
					pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/BuildDate", NULL, &szStringVal, NULL, NULL);
					if(pItem != NULL && szStringVal != NULL)
					{
						strcpy(acBuildDate, szStringVal);
					}
					
					pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/ProductDate", NULL, &szStringVal, NULL, NULL);
					if(pItem != NULL && szStringVal != NULL)
					{
						strcpy(acProductDate, szStringVal);
					}
					
					pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/SerialNumber", NULL, &szStringVal, NULL, NULL);
					if(pItem != NULL && szStringVal != NULL)
					{
						strcpy(acSerialNumber, szStringVal);
					}
					
					Common_Json_Delete(pOutData);
				}
				
				Common_Json_Delete(pInData);
			}
		}

		if(strlen(acDeviceName) == 0)
		{
			cJSON_Struct *pInData = NULL,*pOutData = NULL;
			cJSON_Struct *pItem = NULL;
			char *szStringVal = NULL;
			
			pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pInData != NULL)
			{
				Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/AliIoT4ovfs/iotcfg",0,0);
				Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
				
				int iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
				if(0 == iRet)
				{
					pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/QRCode", NULL, &szStringVal, NULL, NULL);
					if(pItem != NULL && szStringVal != NULL)
					{
						char *pTmp = strstr(szStringVal, "dn=");
						if(pTmp)
						{
							strcpy(acDeviceName, pTmp+strlen("dn="));
						}
					}
					
					Common_Json_Delete(pOutData);
				}
				
				Common_Json_Delete(pInData);
			}
		}

		//30 秒ping 一次，失败6 次就重新拨号
		{
			memset(&struAllDns, 0, sizeof(struAllDns));
			GetAllDns((char *)"/etc/resolv.conf", &struAllDns);
		}
		
		int bCurrOk = 0;
		if(g_LteCfg.bRNDIS)
		{
			bCurrOk = check_RNDIS_IsOK(str_RNDISifname);
			
			npppNetUp = ovfs_utility_is_net_dev_up(str_RNDISifname);
			
			npppNetOut = ovfs_utility_is_net_dev_out(str_RNDISifname);
		}
		else
		{
			bCurrOk = check_PPP_IsOK(str_RNDISifname);
			
			npppNetUp = ovfs_utility_is_net_dev_up(str_RNDISifname);
	
			npppNetOut = ovfs_utility_is_net_dev_out(str_RNDISifname);
		}
		
		if(!bCurrOk || !npppNetUp || npppNetOut)
		{
			ping_4g_failed_cnt = 0;
			ping_count = 0;
			nSleepCount++;
		}
		else
		{
			ping_count++;
			if(ping_count > 3)
			{
				ping_count = 0;
				int b_pppConnected = 0;
				
				for(int index = 0; index < struAllDns.nDnsNum; index++)
				{
					if(NetWorkTool_CheckConnect((char *)str_RNDISifname,struAllDns.ipv4[index]))
					{
						b_pppConnected = 1;
						break;
					}
				}
				
				if(b_pppConnected)
				{
					ping_4g_failed_cnt = 0;
					nSleepCount = 0;
					nWriteCount = 0;

					if(access(str_4gLogCfg, F_OK) == 0)
					{
						memset(acLogBuf, 0, sizeof(acLogBuf));
						sprintf(acLogBuf, "rm -rf %s", str_4gLogCfg);
						Common_System(acLogBuf);
					}
				}
				else
				{
					ping_4g_failed_cnt++;
					nSleepCount++;
				}
			}
		}
		
		if(nSleepCount >= nMaxSleepCount && ++nWriteCount <= nMaxWriteCount/* && CalcFileSize(str_4gLogCfg) < nMaxFileSize*/)
		{
			memset(acLogBuf, 0, sizeof(acLogBuf));
			sprintf(acLogBuf, "echo --------------------nWriteCount=[%d]-------------------- >> %s", nWriteCount, str_4gLogCfg);
			Common_System(acLogBuf);
			
			memset(acLogBuf, 0, sizeof(acLogBuf));
			sprintf(acLogBuf, "echo SimInsert[%d] 4gSignal[%d] 4gRegStatus[%d] >> %s", g_LteCfg.bSIMCardStatus, g_LteCfg.uSignalStrength, g_LteCfg.b4GRegStatus, str_4gLogCfg);
			Common_System(acLogBuf);
			
			memset(acLogBuf, 0, sizeof(acLogBuf));
			sprintf(acLogBuf, "echo dialRes[%d] pppNetUp[%d] pppNetOut[%d] >> %s", bCurrOk, npppNetUp, npppNetOut, str_4gLogCfg);
			Common_System(acLogBuf);
			
			memset(acLogBuf, 0, sizeof(acLogBuf));
			sprintf(acLogBuf, "ifconfig >> %s", str_4gLogCfg);
			Common_System(acLogBuf);
			
			memset(acLogBuf, 0, sizeof(acLogBuf));
			sprintf(acLogBuf, "cat /etc/resolv.conf >> %s", str_4gLogCfg);
			Common_System(acLogBuf);
			
			memset(acLogBuf, 0, sizeof(acLogBuf));
			sprintf(acLogBuf, "route -n >> %s", str_4gLogCfg);
			Common_System(acLogBuf);
			
			for(int index = 0; index < struAllDns.nDnsNum; index++)
			{
				memset(acLogBuf, 0, sizeof(acLogBuf));
				sprintf(acLogBuf, "echo [%s] ping [%s] fail >> %s", str_RNDISifname, struAllDns.ipv4[index], str_4gLogCfg);
				Common_System(acLogBuf);
			}
			
			if(nWriteCount == nMaxWriteCount)
			{
				memset(acLogBuf, 0, sizeof(acLogBuf));
				sprintf(acLogBuf, "echo BuildDate[%s] >> %s", acBuildDate, str_4gLogCfg);
				Common_System(acLogBuf);
				
				memset(acLogBuf, 0, sizeof(acLogBuf));
				sprintf(acLogBuf, "echo ProductDate[%s] >> %s", acProductDate, str_4gLogCfg);
				Common_System(acLogBuf);
				
				memset(acLogBuf, 0, sizeof(acLogBuf));
				sprintf(acLogBuf, "echo SerialNumber[%s] >> %s", acSerialNumber, str_4gLogCfg);
				Common_System(acLogBuf);
				
				memset(acLogBuf, 0, sizeof(acLogBuf));
				sprintf(acLogBuf, "echo DeviceName[%s] >> %s", acDeviceName, str_4gLogCfg);
				Common_System(acLogBuf);
				
				memset(acLogBuf, 0, sizeof(acLogBuf));
				sprintf(acLogBuf, "dmesg >> %s", str_4gLogCfg);
				Common_System(acLogBuf);
			}
		}
		
		if(ping_4g_failed_cnt >= failed_max_num)
		{
			g_nNeedRedial = 1;
			
			ping_4g_failed_cnt = 0;
		}
		
		Common_Sleep(10,0);
	}
	return 0;
}

static S32 ovfs_4g_thread(Common_Thread_T hThreadHandle,void* para)
{
    int bLastOk = -1,bCurrOk = 0;
    int reboot1 = 0,reboot2 = 0,sim_getinfofail_reset = 0,pppd_fail_reset = 0,sim_uninserted_reset = 0;
    char buf[1024];
    S8 system_cmd[128], s8Progress[128];

    memset(g_LteCfg.strCCID,0,sizeof(g_LteCfg.strCCID));
    memset(g_LteCfg.strNetInfo,0,sizeof(g_LteCfg.strNetInfo));
    memset(g_LteCfg.strServiceInfo,0,sizeof(g_LteCfg.strServiceInfo));

    snprintf(g_LteCfg.strIPv4,sizeof(g_LteCfg.strIPv4),"NA");
    snprintf(g_LteCfg.strIPv6,sizeof(g_LteCfg.strIPv6),"NA");
	
    while(!g_4g_thread_exit)
    {	
		Common_GetSystemCount(&g_n4gThreadCurTime, NULL);

		if(g_n4gRouteChange == 1)
		{
			del_4g_route(str_RNDISifname);
			
			del_4g_dns(str_RNDISifname);
		}
		
		if(!g_LteCfg.bEnable)
        {
            LOGD("g_LteCfg.bEnable=[%d]\n",g_LteCfg.bEnable);
			
			{
	            if(g_LteCfg.bRNDIS)
	            {
	                bCurrOk = check_RNDIS_IsOK(str_RNDISifname);
					LOGD("check_RNDIS_IsOK. bCurrOk=[%d]\n",bCurrOk);
	                if(bCurrOk)
	                {
						snprintf(s8Progress, sizeof(s8Progress), "udhcpc -i %s",str_RNDISifname);
						killProgress(s8Progress);
						
						snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s down",str_RNDISifname);
	                    Common_System(system_cmd);
						
						bLastOk = bCurrOk = 0;
	                }
	            }
	            else
	            {
	                bCurrOk = check_PPP_IsOK(str_RNDISifname);
					LOGD("check_PPP_IsOK. bCurrOk=[%d]\n",bCurrOk);
	                if(bCurrOk)
	                {
	                    Common_System("killall pppd");
						bLastOk = bCurrOk = 0;
	                }
	            }
	        }
			
            reboot1 = 0;
            reboot2 = 0;
            sim_getinfofail_reset = 0;
            pppd_fail_reset = 0;
            sim_uninserted_reset = 0;
			
			g_n4GConnectStatus = -1;
			g_n4GConnectFailCount = 0;
			g_n4GConnectSuccCount = 0;
		    g_n4GNotPlugCount = 0;
			g_n4GNotSpecifiedCount = 0;
			g_n4GDomainFailCount = 0;
			g_nCheckCnt = 0;
			
			g_LteCfg.uGPRS_Attach = 0;
			memset(g_LteCfg.strIPv4,0,sizeof(g_LteCfg.strIPv4));
		    memset(g_LteCfg.strIPv6,0,sizeof(g_LteCfg.strIPv6));
		    memset(g_LteCfg.strDNS1v4,0,sizeof(g_LteCfg.strDNS1v4));
		    memset(g_LteCfg.strDNS1v6,0,sizeof(g_LteCfg.strDNS1v6));
		    memset(g_LteCfg.strDNS2v4,0,sizeof(g_LteCfg.strDNS2v4));
		    memset(g_LteCfg.strDNS2v6,0,sizeof(g_LteCfg.strDNS2v6));
            Common_Sleep(1,0);
            continue;
        }
		
		Common_System("lsusb; ls -lh /dev/ttyUSB*; ifconfig -a; route -n");
		
		if((Guard4GDriverFile() != 0) || (Guard4GNetcard() != 0))
		{
			Common_Sleep(1,0);
            continue;
		}
		
		if (0 == Get_LteStatus(buf))
        {
        
        }

        if(g_LteIndex == 0)
        {
            if(g_LteCurStatus == 0)
            {
                if(g_LteModulePluged == 1)
                {
                    reboot1++;
                }
                else
                {
                    reboot2++;
                }
            }
            else
            {
                reboot1 = 0;
                reboot2 = 0;
				g_LteModulePluged = 1;
            }
        }
        else if(g_LteIndex == 1)
        {
            if(g_LteCurStatus == 0)
            {
                sim_uninserted_reset++;
            }
            else
            {
                sim_uninserted_reset = 0;
            }

            reboot1 = 0;
            reboot2 = 0;
			g_LteModulePluged = 1;
        }
        else if(g_LteIndex == 2)
        {
            if(g_LteCurStatus == 0)
            {
                sim_getinfofail_reset++;
            }
            else
            {
                sim_getinfofail_reset = 0;
            }

            reboot1 = 0;
            reboot2 = 0;
			g_LteModulePluged = 1;
            sim_uninserted_reset = 0;
        }
		else if(g_LteIndex == 3)
		{
		}
		
		if(g_LteCfg.bSIMCardStatus == 1)
		{
			if(g_n4GConnectStatus == -1)
	        {
	            Play_Sound(2,1,1); 
	            g_n4GConnectStatus = 0;
	        }
		}
		
		if(g_LteCurStatus == 0)
		{
			if(/*reboot1 >= 3 || reboot2 >= 3 || */sim_uninserted_reset >= 3 || sim_getinfofail_reset >= 3)
			{
				{
		            if(g_LteCfg.bRNDIS)
		            {
		                bCurrOk = check_RNDIS_IsOK(str_RNDISifname);
						LOGD("check_RNDIS_IsOK. bCurrOk=[%d]\n",bCurrOk);
		                if(bCurrOk)
		                {
							snprintf(s8Progress, sizeof(s8Progress), "udhcpc -i %s",str_RNDISifname);
							killProgress(s8Progress);
							
							snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s down",str_RNDISifname);
		                    Common_System(system_cmd);
							
							bLastOk = bCurrOk = 0;
		                }
		            }
		            else
		            {
		                bCurrOk = check_PPP_IsOK(str_RNDISifname);
						LOGD("check_PPP_IsOK. bCurrOk=[%d]\n",bCurrOk);
		                if(bCurrOk)
		                {
		                    Common_System("killall pppd");
							bLastOk = bCurrOk = 0;
		                }
		            }
		        }
				
				g_nCheckCnt = 0;
				g_LteCfg.uGPRS_Attach = 0;
				memset(g_LteCfg.strIPv4,0,sizeof(g_LteCfg.strIPv4));
			    memset(g_LteCfg.strIPv6,0,sizeof(g_LteCfg.strIPv6));
			    memset(g_LteCfg.strDNS1v4,0,sizeof(g_LteCfg.strDNS1v4));
			    memset(g_LteCfg.strDNS1v6,0,sizeof(g_LteCfg.strDNS1v6));
			    memset(g_LteCfg.strDNS2v4,0,sizeof(g_LteCfg.strDNS2v4));
			    memset(g_LteCfg.strDNS2v6,0,sizeof(g_LteCfg.strDNS2v6));

				g_n4GConnectSuccCount = 0;

				if(g_LteIndex == 1)
				{
					//未插4G 卡
					if(g_n4GNotPlugCount % 12 == 0)
			        {
			            Play_Sound(4,1,1);
			        }
					g_n4GNotPlugCount++;
				}
				else if(g_LteIndex == 2)
				{
					//4G 卡异常,  播报联网失败
					g_n4GConnectStatus = 2;
					if(g_n4GConnectStatus == 2)
	                {
	                	if(g_n4GConnectFailCount % 12 == 3)//拨号后不是立马就在线
	                	{
	                    	Play_Sound(1,1,1);
	                	}
						g_n4GConnectFailCount++;
	                }
				}
			}
		}
		else if(g_LteCurStatus == 1)
		{
	        LOGW("b4GEnable=%d,bRNDIS=%d,str_RNDISifname=%s\n",g_LteCfg.bEnable,g_LteCfg.bRNDIS,str_RNDISifname);
	        Common_Lock(g_LteCfg.hLock);
	        if(g_LteCfg.bEnable)
	        {
	            if(g_LteCfg.bRNDIS)//RNDIS方式
	            {
					if(g_nNeedRedial)
					{
						snprintf(s8Progress, sizeof(s8Progress), "udhcpc -i %s",str_RNDISifname);
						killProgress(s8Progress);
						
						snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s down",str_RNDISifname);
	                    Common_System(system_cmd);

						g_nNeedRedial = 0;
					}
					
	                bCurrOk = check_RNDIS_IsOK(str_RNDISifname);
	                LOGW("check_RNDIS_IsOK. bCurrOk=%d,bLastOk=%d,g_nCheckCnt=%d\n",bCurrOk,bLastOk,g_nCheckCnt);
	                if(bLastOk != bCurrOk)
	                {
	                    g_nCheckCnt = 0;
	                    if(!bCurrOk)
	                    {
							if(g_LteCfg.n4GFactory == 0)
							{
		                        Lte_Exe_Cmd("AT^NDISDUP=1,1,\"3gnet\"",1000,buf,sizeof(buf) - 1);
							}
							else if(g_LteCfg.n4GFactory == 1)
							{
								ovfs_change_apn();
							}
							else if(g_LteCfg.n4GFactory == 2 || g_LteCfg.n4GFactory == 3)
							{
								Lte_Exe_Cmd("AT+QNETDEVCTL=0,1,0",1000,buf,sizeof(buf) - 1);
								
								ovfs_change_apn();
								
								Lte_Exe_Cmd("AT+QNETDEVCTL=3,1,0",1000,buf,sizeof(buf) - 1);
							}
							
							snprintf(s8Progress, sizeof(s8Progress), "udhcpc -i %s",str_RNDISifname);
							killProgress(s8Progress);
							
							if(ovfs_utility_is_net_dev_exist(str_OldRNDISifname))
							{
								memset(system_cmd, 0, sizeof(system_cmd));
								sprintf(system_cmd, "ifconfig %s down;ip link set %s name %s;ifconfig %s up", str_OldRNDISifname, str_OldRNDISifname, str_RNDISifname, str_RNDISifname);
							    Common_System(system_cmd);
							}
							else if(ovfs_utility_is_net_dev_exist(str_RNDISifname))
							{
								memset(system_cmd, 0, sizeof(system_cmd));
								snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s up",str_RNDISifname);
	                    		Common_System(system_cmd);
							}
							
							snprintf(system_cmd, sizeof(system_cmd), "udhcpc -i %s -p /var/run/udhcpc_%s.pid -s udhcpc.script &",str_RNDISifname,str_RNDISifname);
		                    Common_System(system_cmd);
							
							for(int ndex = 0; ndex < 10; ndex++)
							{
								char lte_ipaddr[32] = {0}, lte_netmask[32] = {0}, lte_gateway[32] = {0};
								if(NetWorkTool_Get_Dhcpc_Result(str_RNDISifname, lte_ipaddr, lte_netmask, lte_gateway) == 0)
								{
									break;
								}
								
								Common_Sleep(1,0);
							}
	                    }
	                }
	                else if(bCurrOk == 0)
	                {
	                    if(g_nCheckCnt == 0)
	                    {
	                        if(g_LteCfg.n4GFactory == 0)
							{
		                        Lte_Exe_Cmd("AT^NDISDUP=1,1,\"3gnet\"",1000,buf,sizeof(buf) - 1);
							}
							else if(g_LteCfg.n4GFactory == 1)
							{
								ovfs_change_apn();
							}
							else if(g_LteCfg.n4GFactory == 2 || g_LteCfg.n4GFactory == 3)
							{
								Lte_Exe_Cmd("AT+QNETDEVCTL=0,1,0",1000,buf,sizeof(buf) - 1);
								
								ovfs_change_apn();

								Lte_Exe_Cmd("AT+QNETDEVCTL=3,1,0",1000,buf,sizeof(buf) - 1);
							}
							
							snprintf(s8Progress, sizeof(s8Progress), "udhcpc -i %s",str_RNDISifname);
							killProgress(s8Progress);
							
							if(ovfs_utility_is_net_dev_exist(str_OldRNDISifname))
							{
								memset(system_cmd, 0, sizeof(system_cmd));
								sprintf(system_cmd, "ifconfig %s down;ip link set %s name %s;ifconfig %s up", str_OldRNDISifname, str_OldRNDISifname, str_RNDISifname, str_RNDISifname);
							    Common_System(system_cmd);
							}
							else if(ovfs_utility_is_net_dev_exist(str_RNDISifname))
							{
								memset(system_cmd, 0, sizeof(system_cmd));
								snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s up",str_RNDISifname);
	                    		Common_System(system_cmd);
							}
							
							snprintf(system_cmd, sizeof(system_cmd), "udhcpc -i %s -p /var/run/udhcpc_%s.pid -s udhcpc.script &",str_RNDISifname,str_RNDISifname);
		                    Common_System(system_cmd);
							
							for(int ndex = 0; ndex < 10; ndex++)
							{
								char lte_ipaddr[32] = {0}, lte_netmask[32] = {0}, lte_gateway[32] = {0};
								if(NetWorkTool_Get_Dhcpc_Result(str_RNDISifname, lte_ipaddr, lte_netmask, lte_gateway) == 0)
								{
									break;
								}
								
								Common_Sleep(1,0);
							}
	                    }
	                    g_nCheckCnt++;
	                    if(g_nCheckCnt >= 6)
	                    {
							snprintf(s8Progress, sizeof(s8Progress), "udhcpc -i %s",str_RNDISifname);
							killProgress(s8Progress);
							
							snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s down",str_RNDISifname);
		                    Common_System(system_cmd);
							
	                        g_nCheckCnt = 0;
	                    }
	                }

					//if(bCurrOk != bLastOk)
					{
				        if(bCurrOk == 0)
				        {
				            memset(g_LteCfg.strIPv4,0,sizeof(g_LteCfg.strIPv4));
				            memset(g_LteCfg.strIPv6,0,sizeof(g_LteCfg.strIPv6));
				            memset(g_LteCfg.strDNS1v4,0,sizeof(g_LteCfg.strDNS1v4));
				            memset(g_LteCfg.strDNS1v6,0,sizeof(g_LteCfg.strDNS1v6));
				            memset(g_LteCfg.strDNS2v4,0,sizeof(g_LteCfg.strDNS2v4));
				            memset(g_LteCfg.strDNS2v6,0,sizeof(g_LteCfg.strDNS2v6));
				        }
						else
						{
							S8 filename[128] = {0};
							snprintf(filename, sizeof(filename), "/tmp/udhcpc_resolv_%s.conf", str_RNDISifname);
							ovfs_soft::ovfs_local_net_dns lte_dns;
							memset(&lte_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
							GetDNS(&lte_dns,(char *)filename);
							
							if(strcmp(lte_dns.dns1.ipv4, g_LteCfg.strDNS1v4) || strcmp(lte_dns.dns2.ipv4, g_LteCfg.strDNS2v4))
					        {
					        	snprintf(g_LteCfg.strDNS1v4,sizeof(g_LteCfg.strDNS1v4),"%s",lte_dns.dns1.ipv4);
								snprintf(g_LteCfg.strDNS2v4,sizeof(g_LteCfg.strDNS2v4),"%s",lte_dns.dns2.ipv4);
						 	}

							if(g_n4gRouteChange == 2)
							{
								add_4g_route(str_RNDISifname);
								
								add_4g_dns(str_RNDISifname);
							}
						}
					}
					
	                bLastOk = bCurrOk;
	            }
	            else
	            {
					if(g_nNeedRedial)
					{
						Common_System("killall pppd");
						g_nNeedRedial = 0;
					}
					
	                bCurrOk = check_PPP_IsOK(str_RNDISifname);
	                LOGW("check_PPP_IsOK. bCurrOk=%d,bLastOk=%d,g_nCheckCnt=%d\n",bCurrOk,bLastOk,g_nCheckCnt);
	                if(bLastOk != bCurrOk)
	                {
	                    g_nCheckCnt = 0;
	                    if(!bCurrOk)
	                    {
	                        Common_System("killall pppd");
	                        ovfs_change_apn();
							if(g_LteCfg.n4GFactory == 2 || g_LteCfg.n4GFactory == 3)
							{
								Lte_Exe_Cmd("AT+QNETDEVCTL=0,1,0",1000,buf,sizeof(buf) - 1);
							}
	                        Common_System("pppd call lte-dial &");

							for(int ndex = 0; ndex < 10; ndex++)
							{
								char lte_ipaddr[32] = {0}, lte_netmask[32] = {0};
								if((NetWorkTool_GetIPAddr(str_RNDISifname,lte_ipaddr) == 0) && (NetWorkTool_GetMaskAddr(str_RNDISifname,lte_netmask) == 0))
								{
									break;
								}
								
								Common_Sleep(1,0);
							}
	                    }
	                }
	                else if(bCurrOk == 0)
	                {
	                    if(g_nCheckCnt == 0)
	                    {
	                        //Common_System("killall pppd");
	                        ovfs_change_apn();
							if(g_LteCfg.n4GFactory == 2 || g_LteCfg.n4GFactory == 3)
							{
								Lte_Exe_Cmd("AT+QNETDEVCTL=0,1,0",1000,buf,sizeof(buf) - 1);
							}
	                        Common_System("pppd call lte-dial &");

							for(int ndex = 0; ndex < 10; ndex++)
							{
								char lte_ipaddr[32] = {0}, lte_netmask[32] = {0};
								if((NetWorkTool_GetIPAddr(str_RNDISifname,lte_ipaddr) == 0) && (NetWorkTool_GetMaskAddr(str_RNDISifname,lte_netmask) == 0))
								{
									break;
								}
								
								Common_Sleep(1,0);
							}
	                    }
	                    g_nCheckCnt++;
	                    if(g_nCheckCnt >= 6)
	                    {
	                        Common_System("killall pppd");
	                        g_nCheckCnt = 0;
	                    }
	                }
					
					//if(bCurrOk != bLastOk)
					{
				        if(bCurrOk == 0)
				        {
				            memset(g_LteCfg.strIPv4,0,sizeof(g_LteCfg.strIPv4));
				            memset(g_LteCfg.strIPv6,0,sizeof(g_LteCfg.strIPv6));
				            memset(g_LteCfg.strDNS1v4,0,sizeof(g_LteCfg.strDNS1v4));
				            memset(g_LteCfg.strDNS1v6,0,sizeof(g_LteCfg.strDNS1v6));
				            memset(g_LteCfg.strDNS2v4,0,sizeof(g_LteCfg.strDNS2v4));
				            memset(g_LteCfg.strDNS2v6,0,sizeof(g_LteCfg.strDNS2v6));
				        }
						else
						{
							S8 filename[128] = {0};
							snprintf(filename, sizeof(filename), "%s", "/etc/ppp/resolv.conf");
							ovfs_soft::ovfs_local_net_dns lte_dns;
							memset(&lte_dns, 0, sizeof(ovfs_soft::ovfs_local_net_dns));
							GetDNS(&lte_dns,(char *)filename);
							if(strcmp(lte_dns.dns1.ipv4, g_LteCfg.strDNS1v4) || strcmp(lte_dns.dns2.ipv4, g_LteCfg.strDNS2v4))
					        {
					        	snprintf(g_LteCfg.strDNS1v4,sizeof(g_LteCfg.strDNS1v4),"%s",lte_dns.dns1.ipv4);
								snprintf(g_LteCfg.strDNS2v4,sizeof(g_LteCfg.strDNS2v4),"%s",lte_dns.dns2.ipv4);
						 	}
							
							if(g_n4gRouteChange == 2)
							{
								add_4g_route(str_RNDISifname);
								
								add_4g_dns(str_RNDISifname);
							}
						}
					}
					
	                bLastOk = bCurrOk;
	            }
				
				if(!bCurrOk)
                {
                	g_n4GConnectSuccCount = 0;
                	g_LteCfg.uGPRS_Attach = 0;
                    pppd_fail_reset ++;

					//4G 卡拨号失败，播报联网失败
					g_n4GConnectStatus = 2;
					if(g_n4GConnectStatus == 2)
                    {
                    	if(g_n4GConnectFailCount % 12 == 3)//拨号后不是立马就在线
                    	{
                        	Play_Sound(1,1,1);
                    	}
						g_n4GConnectFailCount++;
                    }
                }
                else
                {
					g_n4GConnectFailCount = 0;
                	g_LteCfg.uGPRS_Attach = 1;
                    pppd_fail_reset = 0;

					g_n4GConnectStatus = 1;
					if(g_n4GConnectStatus == 1)
					{
						if(!Is_Custom_fac())
						{
							if(g_n4GConnectSuccCount == 0)
		                	{
		                    	Play_Sound(0,1,1);
		                	}
							g_n4GConnectSuccCount++;
						}
						else
						{
							if(g_n4GCardSpecified == 0)
							{
								if(g_n4GNotSpecifiedCount % 12 == 0)
								{
									Play_Sound(5,1,1);
								}
								g_n4GNotSpecifiedCount++;
							}
							else if(g_n4GCardSpecified == 1)
							{
			                	if(g_n4GConnectSuccCount == 0)
			                	{
			                    	Play_Sound(0,1,1);
			                	}
								g_n4GConnectSuccCount++;
							}
							else if(g_n4GCardSpecified == 2)
							{
								if(g_n4GDomainFailCount % 12 == 0)
								{
									Play_Sound(6,1,1);
								}
								g_n4GDomainFailCount++;
							}
						}
					}
                }
	        }
			Common_UnLock(g_LteCfg.hLock);
		}

		//异常处理
		{
			LOGW("g_LteIndex=[%d],g_LteCurStatus=[%d]\n",g_LteIndex,g_LteCurStatus);
            LOGW("reboot1=[%d],reboot2=[%d],sim_uninserted_reset=[%d],sim_getinfofail_reset=[%d],pppd_fail_reset=[%d]\n",reboot1,reboot2,sim_uninserted_reset,sim_getinfofail_reset,pppd_fail_reset);

			if(reboot1 > 36)
	        {
	        	LOGE(" reboot1=[%d]. reboot system\n",reboot1);
	            RebootSystem();
	        }
	        if(reboot2 > 360)
	        {
	            if(g_LteCfg.bAleadyReboot == 0)
	            {
	                g_LteCfg.bAleadyReboot = 1;
	                NetWork_SaveCfg();
					LOGE(" reboot2=[%d]. reboot system\n",reboot2);
	                RebootSystem();
	            }
	        }
			
			if(/*sim_uninserted_reset >= 12 ||*/ sim_getinfofail_reset >= 60 || pppd_fail_reset >= 60)
	    	{
	    		//sim_uninserted_reset = 0;
	    		sim_getinfofail_reset = 0;
	            pppd_fail_reset = 0;
				RebootLteModel();
				continue;
			}
		}
		
        Common_Sleep(10,0);
    }
    return 0;
}

S32 ovfs_network_4g_init()
{
	int ret = 0;
	cJSON_Struct* pConfig = NULL;
	cJSON_Struct* pConfigDefault = NULL;

	ret = Module_LoadConfig(NetWork_RestComm_GetModuleHdl(),&pConfig);
	if (0 != ret)
	{
        LOGE("[%s:%d]:Module_LoadConfig fail\n",__FUNCTION__, __LINE__);
	}

	ret = Module_LoadConfigByType(NetWork_RestComm_GetModuleHdl(),Module_ConfigType_Default,&pConfigDefault);
	if (0 != ret)
	{
        LOGE("[%s:%d]:Module_LoadConfigByType fail\n",__FUNCTION__, __LINE__);
	}

    int bEnable = 1,bAleadyReboot = -1;
    memset(&g_LteCfg,0,sizeof(g_LteCfg));
    Common_Lock_Create(&g_LteCfg.hLock,"LteLock");

	Common_cJSON_T* networkRoot = NULL;
	NetWork_Rest_Get_NetworkItem_OfRootUri(&networkRoot);
	
	NetWork_Rest_NetAttr_init4g(networkRoot);
	NetWork_Rest_Subscribe_init4g(networkRoot);

    Module_RegisterSubscribe(NetWork_RestComm_GetModuleHdl(),(S8*)"/Network/Subscribe/LteNetError",Lte_subscribeList_fxn,NULL);

    Common_Json_GetAttrValue(pConfig,-1,"NetAttr/LteCfg/AleadyReboot",NULL,NULL,&bAleadyReboot,NULL);
    if(bAleadyReboot != -1)
    {
        g_LteCfg.bAleadyReboot = bAleadyReboot;
    }
    Common_Json_GetAttrValue(pConfig,-1,"NetAttr/LteCfg/Enable",NULL,NULL,&bEnable,NULL);
    if(bEnable == -1)
    {
        Common_Json_GetAttrValue(pConfigDefault,-1,"NetAttr/LteCfg/Enable",NULL,NULL,&bEnable,NULL);
    }
    bEnable = (bEnable > 0);
    g_LteCfg.bAvailable = 1;

#if defined PLATFORM_HI3516CV500 || defined PLATFORM_JZT40
    g_LteCfg.bEnable = 1;//bEnable;
#else//ifdef PLATFORM_JZT30
    g_LteCfg.bEnable = bEnable;
#endif//ifdef PLATFORM_JZT30

	int nValueInt = -1;
	Common_Json_GetAttrValue(pConfig,-1,"NetAttr/LteCfg/ApnType",NULL,NULL,&nValueInt,NULL);
    if(nValueInt == -1)
    {
        Common_Json_GetAttrValue(pConfigDefault,-1,"NetAttr/LteCfg/ApnType",NULL,NULL,&nValueInt,NULL);
    }
	if(nValueInt == -1)
	{
		g_LteCfg.nApnType = 0;
	}
	else
	{
		g_LteCfg.nApnType = nValueInt;
	}

	char *pValueStr = NULL;
	Common_Json_GetAttrValue(pConfig,-1,"NetAttr/LteCfg/ApnStr",NULL,&pValueStr,NULL,NULL);
    if(pValueStr == NULL)
    {
        Common_Json_GetAttrValue(pConfigDefault,-1,"NetAttr/LteCfg/ApnStr",NULL,&pValueStr,NULL,NULL);
    }
	if(pValueStr == NULL)
	{
		memset(g_LteCfg.acApnStr, 0, sizeof(g_LteCfg.acApnStr));
	}
	else
	{
		memset(g_LteCfg.acApnStr, 0, sizeof(g_LteCfg.acApnStr));
		strncpy(g_LteCfg.acApnStr, pValueStr, strlen(pValueStr));
	}

	Common_Json_Delete(pConfig);
	Common_Json_Delete(pConfigDefault);
	
	if(access("/tmp/lte/yuga_clm920_nc3_rndis",F_OK) == 0)
    {
        g_LteCfg.bRNDIS = 1;
        sprintf(str_RNDISifname,"usb0");
		g_LteCfg.n4GFactory = 0;
    }
    else
    {
        g_LteCfg.bRNDIS = 0;
	    sprintf(str_RNDISifname,"ppp0");
		g_LteCfg.n4GFactory = 0;
    }
	
	if(access("/tmp/yuga_clm920_rv3_1",F_OK) == 0)
	{
		g_LteCfg.bRNDIS = 1;
        if(access(str_4gDevFile,F_OK) == 0)
		{
			Read_LteDevInfo(str_RNDISifname);
		}
		else
		{
        	sprintf(str_RNDISifname,"usb0");
		}
		g_LteCfg.n4GFactory = 1;
	}
	else if(access("/tmp/yuga_clm920_rv3_0",F_OK) == 0)
	{
		g_LteCfg.bRNDIS = 0;
	    sprintf(str_RNDISifname,"ppp0");
		g_LteCfg.n4GFactory = 1;
	}
	
	if(access("/tmp/quectel_ec800e_1",F_OK) == 0)
	{
		g_LteCfg.bRNDIS = 1;
		if(access(str_4gDevFile,F_OK) == 0)
		{
			Read_LteDevInfo(str_RNDISifname);
		}
		else
		{
        	sprintf(str_RNDISifname,"usb0");
		}
		g_LteCfg.n4GFactory = 2;
	}
	else if(access("/tmp/quectel_ec800e_0",F_OK) == 0)
	{
		g_LteCfg.bRNDIS = 0;
	    sprintf(str_RNDISifname,"ppp0");
		g_LteCfg.n4GFactory = 2;
	}

	if(access("/tmp/lynq_1",F_OK) == 0)
	{
		g_LteCfg.bRNDIS = 1;
		if(access(str_4gDevFile,F_OK) == 0)
		{
			Read_LteDevInfo(str_RNDISifname);
		}
		else
		{
        	sprintf(str_RNDISifname,"usb0");
		}
		g_LteCfg.n4GFactory = 3;
	}
	else if(access("/tmp/lynq_0",F_OK) == 0)
	{
		g_LteCfg.bRNDIS = 0;
	    sprintf(str_RNDISifname,"ppp0");
		g_LteCfg.n4GFactory = 3;
	}

	LOGE("bRNDIS=[%d],n4GFactory=[%d],str_RNDISifname=[%s]\n",g_LteCfg.bRNDIS,g_LteCfg.n4GFactory,str_RNDISifname);
    g_n4GConnectStatus = -1;
    g_n4GNotPlugCount = 0;
    g_nCheckCnt = 0;
    Common_Thread_Create(&m_thread_status_handle,"net_status",1024*256,COMMON_THREAD_CREATEFLAG_NORMAL,status_4g_thread,NULL);
	Common_Thread_Create(&m_thread_4g_handle,"net_4G",1024*256,COMMON_THREAD_CREATEFLAG_NORMAL,ovfs_4g_thread,NULL);
    g_LteCfg.bInit = 1;

	Set4GModelLight();
	
    return 0;
}
S32 ovfs_network_4g_Save(cJSON_Struct *pJson)
{
	if(g_LteCfg.bInit == 0)
	{
		LOGE("4G not init\n");
		return -1;
	}
	
    Common_Json_SetAttrValue((cJSON_Struct *)pJson, -1,"Enable",Common_Json_Type_Number,NULL,g_LteCfg.bEnable,0);
    Common_Json_SetAttrValue((cJSON_Struct *)pJson, -1,"AleadyReboot",Common_Json_Type_Number,NULL,g_LteCfg.bAleadyReboot,0);
	Common_Json_SetAttrValue((cJSON_Struct *)pJson, -1,"ApnType",Common_Json_Type_Number,NULL,g_LteCfg.nApnType,0);
    Common_Json_SetAttrValue((cJSON_Struct *)pJson, -1,"ApnStr",Common_Json_Type_String,g_LteCfg.acApnStr,0,0);
	
    return 0;
}

int NetWork_Get_LTECfg_Json(cJSON_Struct* out)
{
	if(g_LteCfg.bInit == 0)
	{
		LOGE("4G not init\n");
		return -1;
	}
	
    if(g_LteCfg.bAvailable)
        Common_Json_SetAttrValue(out,-1,"IsSupported",Common_Json_Type_Number,NULL,1,0);
    else
        Common_Json_SetAttrValue(out,-1,"IsSupported",Common_Json_Type_Number,NULL,0,0);
    Common_Json_SetAttrValue(out,-1,"Enable",Common_Json_Type_Number,NULL,g_LteCfg.bEnable,0);
    Common_Json_SetAttrValue(out,-1,"Info",Common_Json_Type_Object,NULL,0,0);

    Common_Json_SetAttrValue(out,-1,"Info/Is5GNet",Common_Json_Type_Number,NULL,g_LteCfg.bRNDIS,0);

    Common_Json_SetAttrValue(out,-1,"Info/IMEI",Common_Json_Type_String,g_LteCfg.strIMEI,0,0);
	Common_Json_SetAttrValue(out,-1,"Info/StateOfCharge",Common_Json_Type_Number,NULL,g_LteCfg.nStateOfCharge,0);
    Common_Json_SetAttrValue(out,-1,"Info/InsertedStatus",Common_Json_Type_Number,NULL,g_LteCfg.bSIMCardStatus,0);
    Common_Json_SetAttrValue(out,-1,"Info/CCID",Common_Json_Type_String,g_LteCfg.strCCID,0,0);

    Common_Json_SetAttrValue(out,-1,"Info/NetInfo",Common_Json_Type_String,g_LteCfg.strNetInfo,0,0);
    Common_Json_SetAttrValue(out,-1,"Info/ServiceInfo",Common_Json_Type_String,g_LteCfg.strServiceInfo,0,0);

    Common_Json_SetAttrValue(out,-1,"Info/SignalStrength",Common_Json_Type_Number,NULL,g_LteCfg.uSignalStrength,0);

	Common_Json_SetAttrValue(out,-1,"Info/ApnType",Common_Json_Type_Number,NULL,g_LteCfg.nApnType,0);
    Common_Json_SetAttrValue(out,-1,"Info/ApnStr",Common_Json_Type_String,g_LteCfg.acApnStr,0,0);

    Common_Json_SetAttrValue(out,-1,"Info/LinkStatus",Common_Json_Type_Number,NULL,g_LteCfg.uGPRS_Attach,0);

    Common_Json_SetAttrValue(out,-1,"Info/IpAddrV4",Common_Json_Type_String,g_LteCfg.strIPv4,0,0);
    Common_Json_SetAttrValue(out,-1,"Info/IpAddrV6",Common_Json_Type_String,g_LteCfg.strIPv6,0,0);

    Common_Json_SetAttrValue(out,-1,"Info/DNS1V4",Common_Json_Type_String,g_LteCfg.strDNS1v4,0,0);
    Common_Json_SetAttrValue(out,-1,"Info/DNS1V6",Common_Json_Type_String,g_LteCfg.strDNS1v6,0,0);

    Common_Json_SetAttrValue(out,-1,"Info/DNS2V4",Common_Json_Type_String,g_LteCfg.strDNS2v4,0,0);
    Common_Json_SetAttrValue(out,-1,"Info/DNS2V6",Common_Json_Type_String,g_LteCfg.strDNS2v6,0,0);

    Common_Json_SetAttrValue(out,-1,"ErrorCode",Common_Json_Type_Number,NULL,g_LteCfg.stStatus.errCode,0);
    Common_Json_SetAttrValue(out,-1,"ErrorMsg",Common_Json_Type_String,g_LteCfg.stStatus.errTips,0,0);

    //Common_Json_StandardPrint(out,"out <",">\n",NULL);
    return 0;
}

int NetWork_Put_LTECfg_Json(cJSON_Struct* in)
{
	if(g_LteCfg.bInit == 0)
	{
		LOGE("4G not init\n");
		return -1;
	}
	
    int bEnable = -1;
    Common_Json_GetAttrValue(in,-1,"Enable",NULL,NULL,&bEnable,NULL);
	
	int nApnType = -1;
    Common_Json_GetAttrValue(in,-1,"Info/ApnType",NULL,NULL,&nApnType,NULL);
	
	char *pStrValue = NULL;
	Common_Json_GetAttrValue(in,-1,"Info/ApnStr",NULL,&pStrValue,NULL,NULL);
	
    Common_Lock(g_LteCfg.hLock);
    if(bEnable != -1 && g_LteCfg.bEnable != bEnable)
    {
        g_LteCfg.bEnable = bEnable;
        g_nCheckCnt = 0;
    }
	
	if((nApnType != -1 && nApnType != g_LteCfg.nApnType) || (pStrValue != NULL && strcmp(g_LteCfg.acApnStr, pStrValue)))
	{
		g_LteCfg.nApnType = nApnType;
		memset(g_LteCfg.acApnStr, 0, sizeof(g_LteCfg.acApnStr));
		strncpy(g_LteCfg.acApnStr, pStrValue, strlen(pStrValue));

		g_LteCfg.uGPRS_Attach = 0;
		g_nNeedRedial = 1;
	}
    Common_UnLock(g_LteCfg.hLock);
    return 0;
}

static S32 Lte_subscribeList_fxn(ModuleHandle_T hModuleHandle,S32 nType /* 0-subscribe,1-unsubscribe,2-QueryEvent*/,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData)
{
    int nIdx = 0;

    //nIdx = NetWork_Event_FindMatchSubUri(szSubscribeUri);
    if(nIdx < 0)
    {
        LOGE("subscribe uri=%s not supported!\n",szSubscribeUri);
        return -1;
    }

    LOGI("NetWork_EventSubscribe URI:%s nType:%d\n",szSubscribeUri,nType);

    if (0 == nType)
    {
        g_LteCfg.subscribeID = nRecvID;
        //NetWork_Event_SetSubId(nIdx,nRecvID);

        if(pQueryEventInfo)
        {
            //Common_cJSON_T* outParam = Common_cJSON_CreateObject();
            cJSON_Struct *outParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            *pQueryEventInfo = outParam;
            //NetWork_Check_Event(nIdx,outParam,0);
        }
    }
    else if (1 == nType)
    {
        //NetWork_Event_UnSetSubId(nIdx,nRecvID);
    }
    else if (2 == nType)
    {
        /*
        if(NetWork_Event_CheckSubId(nIdx,nRecvID))
        {
        	if(pQueryEventInfo)
        	{
        		//Common_cJSON_T* outParam = Common_cJSON_CreateObject();
        		cJSON_Struct *outParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        		*pQueryEventInfo = outParam;
        		//NetWork_Check_Event(nIdx,outParam,0);
        	}
        }
        else
        {
        	LOGE("Can't found id. Uri:%s\n",szSubscribeUri);
        }
        */
    }
    return 0;
}

static int Lte_report_error(OVFS_LTE_EVENT *pErrorEvent)
{
    cJSON_Struct *param = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    if(strlen(g_LteCfg.stStatus.errTips) == 0)
    {
        switch(pErrorEvent->errCode)
        {
        case -1:
            sprintf(g_LteCfg.stStatus.errTips,"NIC not found.");
            break;
        case -2:
            sprintf(g_LteCfg.stStatus.errTips,"SIM Card not ready.");
            break;
        case -3:
            sprintf(g_LteCfg.stStatus.errTips,"Network service not ready.");
            break;
        }
    }

    Common_Json_SetAttrValue(param,-1,"ErrorCode",Common_Json_Type_Number,NULL,pErrorEvent->errCode,0);
    Common_Json_SetAttrValue(param,-1,"Opinion",Common_Json_Type_String,pErrorEvent->errTips,0,0);
    Common_Json_SetAttrValue(param,-1,"AtReq",Common_Json_Type_String,pErrorEvent->atType,0,0);
    Common_Json_SetAttrValue(param,-1,"AtRsp",Common_Json_Type_String,pErrorEvent->errMsg,0,0);

    cJSON_Struct *retParam = NULL;
    Module_SendEvent(NetWork_RestComm_GetModuleHdl(),g_LteCfg.subscribeID,(cJSON_Struct*)param,(cJSON_Struct**)(&retParam),2000);
    if(retParam)
    {
        Common_Json_Delete(retParam);
    }

    char* out = NULL;
    LOGI("send Event. result=%s\n",out = Common_Json_Print(param,NULL));
    if(out)
    {
        Common_Free(out,__FUNCTION__,__LINE__);
    }
    if(param)
    {
        Common_Json_Delete(param);
    }

    return 0;
}

int strReline(char *pString,char *pNewStr)
{
    U8 i=0;
    if((NULL == pString) || (NULL == pNewStr)) return 0;
    char *p = pString;
    for(i=0; i<strlen(pString); i++)
    {
        if(*p == '\r' || *p == '\n')
        {
            pNewStr[i] = ' ';
        }
        else
        {
            pNewStr[i] = *p;
        }
        p++;
    }
    pNewStr[i] = 0;
    return 0;
}


static S32  Lte_Exe_Cmd(const S8 *cmd,U32 timeout_ms, S8 *buf, U32 buf_size)
{
    int nPos = 0;
    U32 execute_ok = 0;
    struct timeval timeout;
    fd_set rd_set;
    S32 at_fd = -1;
    char szCmd[256];
    int nTryCnt = 0;

    if ((cmd == NULL) || (buf == NULL) || (buf_size == 0))
    {
        LOGE("Invalid Param\n");
        return -EINVAL;
    }

	int ret = access("/dev/at_cmd", F_OK);
	if(ret != 0)
	{
		LOGE("file[/dev/at_cmd] not exist\n");
		return -2;
	}
	else
	{
		struct stat st;
		int nRet = stat("/dev/at_cmd", &st);
		if((nRet != 0) || ((st.st_mode & S_IFMT) != S_IFCHR))
		{
			LOGE("file[/dev/at_cmd] not char device file\n");
			return -2;
		}
	}

    memset(buf, 0, buf_size);
    memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));


    at_fd = open("/dev/at_cmd",O_RDWR | O_NOCTTY |O_NONBLOCK | O_NDELAY);

    LOGD("/dev/at_cmd at_fd=%d\n",at_fd);
    if(at_fd <= 0)
    {
        LOGE("Not Find dev!\n");
        return -2;
    }

    fcntl(at_fd,F_SETFL,FNDELAY);
	tcflush(at_fd,TCIOFLUSH);	//4g 重置后没有接收返回命令，下次ATE0 可能会收到重置后返回命令
	memset(szCmd, 0, sizeof(szCmd));
	sprintf(szCmd,"%s\r\n",cmd);
    sprintf(g_LteCfg.stStatus.atType,"%s",cmd);
    write(at_fd,szCmd,strlen(szCmd));

	Common_Sleep(0, 100000);	//ATE0 命令发送完立即读取，可能会读到发送出去的ATE0

    timeout_ms = 500;
    do
    {
        FD_ZERO(&rd_set);
        FD_SET(at_fd, &rd_set);
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(at_fd + 1, &rd_set, NULL, NULL, &timeout);
        if (ret < 0)
        {
            LOGW("select Function Error\n");
            break;
        }
        else if (ret == 0)
        {
            LOGW("select Function Timeout\n");

            nTryCnt++;
            // timeout_ms+=30 * nTryCnt;
            if(nTryCnt >= 3)
            {
                break;
            }
			write(at_fd,szCmd,strlen(szCmd));
			Common_Sleep(0, 100000);
            continue;
        }
        else
        {
            if (FD_ISSET(at_fd, &rd_set))
            {
                S32 real_read_size = read(at_fd, buf + nPos, buf_size - nPos - 1);
                //printf("[%s.%d] here buf_size = %d real_read_size = %d nPos = %d\n",__FUNCTION__,__LINE__,buf_size,real_read_size,nPos);

                nPos += real_read_size;
                execute_ok = 1;
                if (nPos == (S32) buf_size - 1)
                {
                    LOGW("CMD[%s] Output Info Size May Be Overflow, Store Buffer Size = %u\n", cmd, buf_size);
                    break;
                }
                if(nPos > 0)
                {
                    char *pTmp = strstr(buf,"ERROR");
                    if(strstr(buf,"OK"))
                    {
                        break;
                    }
                    else if(pTmp)
                    {
                        //strReline(pTmp,g_LteCfg.stStatus.errMsg);
                        break;
                    }
                    else
                    {
                        if(real_read_size <= 0)
						{
						    LOGW("buf=<%s>,real_read_size=%d,nPos=%d\n",buf,real_read_size,nPos);
                            break;
						}
                    }
                }
                else
                {
                    LOGE("real_read_size=%d\n",real_read_size);
                    break;
                }
            }
        }
    }
    while(1);

    buf[nPos] = '\0';

    LOGD("execute_ok = %d \n<%s> -> \n<%s> \n",execute_ok,cmd,buf);
#if 0
    for(int i= 0; i < strlen(buf); i++)
    {
        if(i%16 == 0)printf("\n");
        printf("%02X ",buf[i]);
    }
    printf("\n");

#endif
    tcflush(at_fd,TCIOFLUSH);

    if(close(at_fd) < 0)
        perror("close error");
    if(execute_ok > 0)
        return 0;
    return at_fd;
}

int Get_LteStatus(S8 *moudleType)
{
    int ret = 0,bGetOk = 0,nReturnOk = 0;
    char buf[1024],*pstrOk = NULL,*pContext = NULL;;

    ret = Lte_Exe_Cmd("ATE0", 1000, buf, sizeof(buf) - 1);
    if(ret == 0)
    {
		pstrOk = strstr(buf,"0\r\n");
        if(pstrOk)
        {
			Lte_Exe_Cmd("ATV1", 1000, buf, sizeof(buf) - 1);
		}
        g_LteIndex = 0;
        g_LteCurStatus = 1;
		memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
    }
    else
    {
        bGetOk = 0;
        g_LteIndex = 0;
        g_LteCurStatus = 0;
		
		if(g_LteCfg.bRNDIS)
		{
			g_LteCfg.nQNetDevCtl = 0;
		}
		
		sprintf(g_LteCfg.stStatus.errMsg,"ATE0 Command Error");
        return -1;
    }
	
    ret = Lte_Exe_Cmd("ATI", 500, buf, sizeof(buf) - 1);
    if (0 == ret)
    {
//    	char moudleType[32];
        pstrOk = strstr(buf,"OK");
        if(pstrOk)
        {
            sscanf(buf, "\n%s\"", moudleType);
            printf("moudleType=%s\n",moudleType);

            pstrOk = strstr(buf,"MH5000");

            if(pstrOk)
            {
                g_LteCfg.bRNDIS = 1;
            }
            g_LteIndex = 0;
            g_LteCurStatus = 1;
			memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
        }
        else
        {
            sprintf(g_LteCfg.stStatus.errMsg,"ATI Command Error");
            g_LteIndex = 0;
            g_LteCurStatus = 0;
        }
    }
    else
    {
        if(0 == bGetOk)
        {
            g_LteIndex = 0;
            g_LteCurStatus = 0;
            g_LteCfg.stStatus.errCode = -1;
            sprintf(g_LteCfg.stStatus.errMsg,"ATI Command Error");
			
			if(g_LteCfg.bRNDIS)
			{
				g_LteCfg.nQNetDevCtl = 0;
			}
			
            Lte_report_error(&g_LteCfg.stStatus);
            return g_LteCfg.stStatus.errCode;
        }
    }

    snprintf(g_LteCfg.strIMEI,sizeof(g_LteCfg.strIMEI),"NA");
    if (0 == Lte_Exe_Cmd("AT+GSN", 4000, buf, sizeof(buf) - 1))
    {
        pstrOk = strstr(buf,"OK");
        if(pstrOk && (pstrOk - buf) >= 4)
        {
            g_LteIndex = 0;
            g_LteCurStatus = 1;
            U64 id = str2num(buf + 2,g_LteCfg.strIMEI);
            //sscanf(buf + 2, "+GSN: \"%llu\"", &id);
            //snprintf(g_LteCfg.strIMEI,sizeof(g_LteCfg.strIMEI),"%lld",id);
            LOGD("id=%llu,strIMEI=%s\n",id,g_LteCfg.strIMEI);
			memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
        }
        else
        {
            g_LteIndex = 0;
            g_LteCurStatus = 0;
            sprintf(g_LteCfg.stStatus.errMsg,"AT+GSN Command Error");
        }
    }

	{
		int nReturnOk = 0, nAdcValue = 0;
		
		if(g_LteCfg.n4GFactory == 0)
		{
			nReturnOk = 0;
		}
		else if(g_LteCfg.n4GFactory == 1)
		{
			nReturnOk = 0;
		}
		else if(g_LteCfg.n4GFactory == 2)
		{
			if (0 == Lte_Exe_Cmd("AT+QADC=0", 1000, buf, sizeof(buf) - 1))
			{
				pstrOk = strstr(buf,"OK");
				if(pstrOk && (pstrOk - buf) >= 4)
				{
					pContext = strstr(buf,"+QADC: 0,");
		            if(pContext != NULL)
		            {
		                nAdcValue = atoi(pContext+strlen("+QADC: 0,"));
						nReturnOk = 1;
		            }
				}
			}
		}
		else if(g_LteCfg.n4GFactory == 3)
		{
			if (0 == Lte_Exe_Cmd("AT+MADC=1", 1000, buf, sizeof(buf) - 1))
			{
				pstrOk = strstr(buf,"OK");
				if(pstrOk && (pstrOk - buf) >= 4)
				{
					pContext = strstr(buf,"+MADC: 1,");
		            if(pContext != NULL)
		            {
		                nAdcValue = atoi(pContext+strlen("+MADC: 1,"));
						nReturnOk = 1;
		            }
				}
			}
		}

		if(nReturnOk == 0)
		{
			g_LteCfg.nStateOfCharge = -1;
			LOGD("nStateOfCharge=[%d]\n",g_LteCfg.nStateOfCharge);
		}
		else if(nAdcValue < 400)
		{
			g_LteCfg.nStateOfCharge = -1;
			LOGD("nStateOfCharge=[%d]\n",g_LteCfg.nStateOfCharge);
		}
		else
		{
			int nChargeValue[] = {920, 895, 850, 815, 620};
			if(nAdcValue >= nChargeValue[0])
			{
				g_LteCfg.nStateOfCharge = 100;
			}
			else if(nAdcValue >= nChargeValue[1] && nAdcValue < nChargeValue[0])
			{
				g_LteCfg.nStateOfCharge = 25 * (nAdcValue - nChargeValue[1]) / (nChargeValue[0] - nChargeValue[1]) + 75;
			}
			else if(nAdcValue >= nChargeValue[2] && nAdcValue < nChargeValue[1])
			{
				g_LteCfg.nStateOfCharge = 25 * (nAdcValue - nChargeValue[2]) / (nChargeValue[1] - nChargeValue[2]) + 50;
			}
			else if(nAdcValue >= nChargeValue[3] && nAdcValue < nChargeValue[2])
			{
				g_LteCfg.nStateOfCharge = 25 * (nAdcValue - nChargeValue[3]) / (nChargeValue[2] - nChargeValue[3]) + 25;
			}
			else if(nAdcValue >= nChargeValue[4] && nAdcValue < nChargeValue[3])
			{
				g_LteCfg.nStateOfCharge = 25 * (nAdcValue - nChargeValue[4]) / (nChargeValue[3] - nChargeValue[4]) + 0;
			}
			else if(nAdcValue < nChargeValue[4])
			{
				g_LteCfg.nStateOfCharge = 0;
			}
			LOGD("nAdcValue=[%d],nStateOfCharge=[%d]\n",nAdcValue,g_LteCfg.nStateOfCharge);
		}
	}

    // InsertedStatus
    if (0 == Lte_Exe_Cmd("AT+CPIN?", 1000, buf, sizeof(buf) - 1))
    {
    	nReturnOk = 0;
        pstrOk = strstr(buf,"\nOK");
        if(pstrOk && (pstrOk - buf) >= 4)
        {
        	if(strstr(buf,"+CPIN: READY"))
            {
                nReturnOk = 1;
            }
        }
		
		if(1 == nReturnOk)
		{
			//拔插4 G 卡
			if(g_LteCfg.bSIMCardStatus != 1)
			{
				g_n4GCardSpecified = -1;
				g_n4GNotSpecifiedCount = 0;
				g_n4GDomainFailCount = 0;
				g_n4GConnectStatus = -1;
				g_n4GConnectFailCount = 0;
				g_n4GConnectSuccCount = 0;
			}
		
			g_LteIndex = 1;
            g_LteCurStatus = 1;
            g_LteCfg.bSIMCardStatus = 1;
			memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
		}
		else
		{
		    g_LteIndex = 1;
		    g_LteCurStatus = 0;
			g_LteCfg.bSIMCardStatus = 0;
			g_LteCfg.b4GRegStatus = 0;
			sprintf(g_LteCfg.stStatus.errMsg,"AT+CPIN? Command Error");
		    g_LteCfg.stStatus.errCode = -2;

			if(g_LteCfg.bRNDIS)
			{
				g_LteCfg.nQNetDevCtl = 0;
			}
			
		    Lte_report_error(&g_LteCfg.stStatus);
		    return g_LteCfg.stStatus.errCode;
		}
    }
	
    g_n4GNotPlugCount = 0;
    
    /*
	ICCID为IC卡的唯一识别号码，共有20位数字组成，其编码格式为：XXXXXX 0MFSS YYGXX XXXXX。分别介绍如下：
	前六位运营商代码：中国移动的为：898600/898602/898604/898607；中国联通的为：898601/898606/898609，中国电信的为：898603 。
	*/
    int bNotSupportServ = 0;
    {
        pstrOk = NULL;
		if (0 == Lte_Exe_Cmd("AT+CCID", 1000, buf, sizeof(buf) - 1))
        {
            pstrOk = strstr(buf,"OK");
        }
		if(!pstrOk)
		{
			if (0 == Lte_Exe_Cmd("AT+ICCID", 1000, buf, sizeof(buf) - 1))
	        {
	            pstrOk = strstr(buf,"OK");
	        }
		}
		if(!pstrOk)
        {
			if (0 == Lte_Exe_Cmd("AT+CICCID", 1000, buf, sizeof(buf) - 1))
		    {
		        pstrOk = strstr(buf,"OK");
		    } 
        }
		if(!pstrOk)
        {
			if (0 == Lte_Exe_Cmd("AT+ECICCID", 1000, buf, sizeof(buf) - 1))
		    {
		        pstrOk = strstr(buf,"OK");
		    }
        }
		
        if(pstrOk)
        {
            char CCID[32] = {0};
            U64 id = str2num(buf + 2,CCID);
            LOGD("id=%llu,CCID=%s\n",id,CCID);
			
			char *pTmp = NULL;
			if((pTmp = strstr(buf+2,": ")) != NULL)
			{
				snprintf(CCID,21,"%s",pTmp+2);
	            LOGD("buf=%s,CCID=%s\n",buf,CCID);
			}
			else if((pTmp = strstr(buf+2,":")) != NULL)
			{
				snprintf(CCID,21,"%s",pTmp+1);
	            LOGD("buf=%s,CCID=%s\n",buf,CCID);
			}
			
			if(strlen(CCID) > 0)
			{
				snprintf(g_LteCfg.strCCID,sizeof(g_LteCfg.strCCID),"%s",CCID);
	            if((0 == strncmp(CCID,"898600",6)) || (0 == strncmp(CCID,"898602",6)) || (0 == strncmp(CCID,"898604",6)) || (0 == strncmp(CCID,"898607",6)))
	            {
	                snprintf(g_LteCfg.strServiceInfo,sizeof(g_LteCfg.strServiceInfo),"中国移动");
	            }
	            else if((0 == strncmp(CCID,"898601",6)) || (0 == strncmp(CCID,"898606",6)) || (0 == strncmp(CCID,"898609",6)))
	            {
	                snprintf(g_LteCfg.strServiceInfo,sizeof(g_LteCfg.strServiceInfo),"中国联通");
	            }
	            else if(0 == strncmp(CCID,"898603",6) || (0 == strncmp(CCID,"898611",6)))
	            {
	                snprintf(g_LteCfg.strServiceInfo,sizeof(g_LteCfg.strServiceInfo),"中国电信");
	            }
	            else
	            {
	                bNotSupportServ = 1;
	            }
			}
            g_LteIndex = 2;
            g_LteCurStatus = 1;
        }
    }
#if 0
	if((g_n4GUseless == 1) || NetWork_Bind_LteCfg() != 0)
	{
		if(check_PPP_IsOK(str_RNDISifname))
		{
			Common_System("killall pppd");
		}

		g_LteIndex = 3;
        g_LteCurStatus = 0;
        g_LteCfg.stStatus.errCode = -3;
        Lte_report_error(&g_LteCfg.stStatus);
        return g_LteCfg.stStatus.errCode;
	}
#endif
    if (0 == Lte_Exe_Cmd("AT+COPS?", 1000, buf, sizeof(buf) - 1))
    {
        pstrOk = strstr(buf,"OK");
        if(pstrOk && (pstrOk - buf) >= 4)
        {
            g_LteIndex = 2;
            g_LteCurStatus = 1;
			memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
            bGetOk = 1;
        }
        else
        {
            g_LteIndex = 2;
            g_LteCurStatus = 0;
            sprintf(g_LteCfg.stStatus.errMsg,"AT+COPS? Command Error");
        }

        if(bGetOk)
        {
            pContext = strstr(buf,",\"");
            if(pContext != NULL)
            {
                char *p;
                pContext = pContext + strlen(",\"");
                LOGD("before:%s\n",pContext);
                p = strstr(pContext,"\",");
                if(p != NULL)
                {
                    p[0] = 0;
                    LOGD("%s\n",pContext);
                }
                if(0 == strcmp(pContext,"46011") || 0 == strcmp(pContext,"CHN-CT"))
                {
                    snprintf(g_LteCfg.strNetInfo,sizeof(g_LteCfg.strNetInfo),"CHN-CT");
                    if(bNotSupportServ)
                        snprintf(g_LteCfg.strServiceInfo,sizeof(g_LteCfg.strServiceInfo),"中国电信");
                }
                else if(0 == strcmp(pContext,"46001") || 0 == strcmp(pContext,"CHN-UNICOM"))
                {
                    snprintf(g_LteCfg.strNetInfo,sizeof(g_LteCfg.strNetInfo),"CHN-UNICOM");
                    if(bNotSupportServ)
                        snprintf(g_LteCfg.strServiceInfo,sizeof(g_LteCfg.strServiceInfo),"中国联通");
                }
                else if(0 == strcmp(pContext,"46000") || 0 == strcmp(pContext,"CHINA MOBILE"))
                {
                    snprintf(g_LteCfg.strNetInfo,sizeof(g_LteCfg.strNetInfo),"CHINA MOBILE");
                    if(bNotSupportServ)
                        snprintf(g_LteCfg.strServiceInfo,sizeof(g_LteCfg.strServiceInfo),"中国移动");
                }
                else
                {
                    snprintf(g_LteCfg.strNetInfo,sizeof(g_LteCfg.strNetInfo),"%s",pContext);
                }
            }
        }
    }
	
    if (0 == Lte_Exe_Cmd("AT+CSQ", 1000, buf, sizeof(buf) - 1))
    {
    	nReturnOk = 0;
    	int nintValue = 0;
        pstrOk = strstr(buf,"OK");
        if(pstrOk && (pstrOk - buf) >= 4)
        {
            pContext = strstr(buf,"+CSQ: ");
            if(pContext != NULL)
            {
                nintValue = atoi(pContext+strlen("+CSQ: "));
				nintValue = nintValue * 100/31;
				if((nintValue >= 5) && (nintValue <= 100))
				{
					nReturnOk = 1;
				}
            }
        }
		
		if(0 == nReturnOk)
		{
			if (0 == Lte_Exe_Cmd("AT+CESQ", 1000, buf, sizeof(buf) - 1))
            {
                pstrOk = strstr(buf,"OK");
                if(pstrOk && (pstrOk - buf) >= 4)
                {
                    pContext = strstr(buf,"+CESQ: ");
                    if(pContext != NULL)
                    {
                        nintValue = atoi(pContext+strlen("+CESQ: "));
                        if(nintValue == 99)
                            nintValue = 0;
                        else
                            nintValue = nintValue * 100 / 63;
						if((nintValue >= 5) && (nintValue <= 100))
						{
							nReturnOk = 1;
						}
                    }
                }
            }
		}
			
		if(1 == nReturnOk)
		{
			g_LteIndex = 2;
        	g_LteCurStatus = 1;
			LOGD("uSignalStrength=%d\n",nintValue);
			g_LteCfg.uSignalStrength = nintValue;
			memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
		}
		else
		{
			g_LteIndex = 2;
        	g_LteCurStatus = 0;
			g_LteCfg.uSignalStrength = 0;
            g_LteCfg.stStatus.errCode = -3;
            sprintf(g_LteCfg.stStatus.errMsg,"AT+CSQ Command Error");

			if(g_LteCfg.bRNDIS)
			{
				g_LteCfg.nQNetDevCtl = 0;
			}
			
            Lte_report_error(&g_LteCfg.stStatus);
            return g_LteCfg.stStatus.errCode;
		}
    }
	
    if (0 == Lte_Exe_Cmd("AT+CREG?", 1000, buf, sizeof(buf) - 1))
    {
    	nReturnOk = 0;
    	int n=0,stat=0;
        pstrOk = strstr(buf,"OK");
        if(pstrOk && (pstrOk - buf) >= 4)
        {
            pContext = strstr(buf,"+CREG: ");
            if(pContext != NULL)
            {
                sscanf(pContext, "+CREG: %d,%d", &n,&stat);
                LOGD("n=%d,stat=%d\n",n,stat);
				if((1 == stat) || (5 == stat))
				{
					nReturnOk = 1;
				}
            }
        }
		
		if(0 == nReturnOk)
		{
			if (0 == Lte_Exe_Cmd("AT+CGREG?", 1000, buf, sizeof(buf) - 1))
            {
                pstrOk = strstr(buf,"OK");
                if(pstrOk && (pstrOk - buf) >= 4)
                {
                    pContext = strstr(buf,"+CGREG: ");
                    if(pContext != NULL)
                    {
                        sscanf(pContext, "+CGREG: %d,%d", &n,&stat);
                        LOGD("n=%d,stat=%d\n",n,stat);
						if((1 == stat) || (5 == stat))
						{
							nReturnOk = 1;
						}
                    }
                }
			}
		}

		if(0 == nReturnOk)
		{
			if (0 == Lte_Exe_Cmd("AT+CEREG?", 1000, buf, sizeof(buf) - 1))
            {
                pstrOk = strstr(buf,"OK");
                if(pstrOk && (pstrOk - buf) >= 4)
                {
                    pContext = strstr(buf,"+CEREG: ");
                    if(pContext != NULL)
                    {
                        sscanf(pContext, "+CEREG: %d,%d", &n,&stat);
                        LOGD("n=%d,stat=%d\n",n,stat);
						if((1 == stat) || (5 == stat))
						{
							nReturnOk = 1;
						}
                    }
                }
			}
		}

		if(1 == nReturnOk)
		{
            g_LteIndex = 2;
            g_LteCurStatus = 1;
			g_LteCfg.b4GRegStatus = 1;
			memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
		}
		else
        {
            g_LteIndex = 2;
            g_LteCurStatus = 0;
			g_LteCfg.b4GRegStatus = 0;
            g_LteCfg.stStatus.errCode = -3;
            sprintf(g_LteCfg.stStatus.errMsg,"AT+CREG? Command Error");

			if(g_LteCfg.bRNDIS)
			{
				g_LteCfg.nQNetDevCtl = 0;
			}
			
            Lte_report_error(&g_LteCfg.stStatus);
            return g_LteCfg.stStatus.errCode;
        }
    }

	if(g_LteCfg.n4GFactory == 1)
	{
	    if (0 == Lte_Exe_Cmd("AT^SYSINFO", 1000, buf, sizeof(buf) - 1))
	    {
	        pContext = strstr(buf,"^SYSINFO:");
	        if(pContext != NULL)
	        {
	            int srv_status=0,srv_domain=0,roam_status=0,sys_mode=0,sim_state=0;
	            sscanf(pContext, "^SYSINFO: %d,%d,%d,%d,%d", &srv_status,&srv_domain,&roam_status,&sys_mode,&sim_state);
	            LOGD("srv_status=%d,srv_domain=%d,roam_status=%d,sys_mode=%d,sim_state=%d\n",srv_status,srv_domain,roam_status,sys_mode,sim_state);
	            if((2 == srv_status) && ((2 == srv_domain) || (3 == srv_domain)))
	            {
	                g_LteIndex = 2;
	                g_LteCurStatus = 1;
					memset(&g_LteCfg.stStatus,0,sizeof(g_LteCfg.stStatus.errTips));
	                LOGI("PS domain is OK!\n");
	            }
	            else
	            {
	                g_LteIndex = 2;
	                g_LteCurStatus = 0;
	                g_LteCfg.stStatus.errCode = -3;
	                sprintf(g_LteCfg.stStatus.errMsg,"AT^SYSINFO Command Error");
	                Lte_report_error(&g_LteCfg.stStatus);
	                return g_LteCfg.stStatus.errCode;
	            }
	        }
	    }
	}
	
    if(g_LteCfg.bRNDIS)
    {
		if (0 == Lte_Exe_Cmd("AT+QNETDEVCTL?", 1000, buf, sizeof(buf) - 1))
		{
			nReturnOk = 0;
			if(strstr(buf,"OK") && strstr(buf,"+QNETDEVCTL"))
			{
				pContext = strstr(buf,"3,1,0,");
	            if(pContext != NULL)
	            {
	            	nReturnOk = atoi(pContext+strlen("3,1,0,"));
	            }
			}
			
			g_LteCfg.nQNetDevCtl = nReturnOk;
		}
    }
	
    return g_LteCfg.stStatus.errCode;
}

