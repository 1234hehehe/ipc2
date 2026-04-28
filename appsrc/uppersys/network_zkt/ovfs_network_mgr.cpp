#include "ovfs_comm_def.h"
#include "ovfs_comm_errno.h"
#include "ovfs_network_base_def.h"
#include "ovfs_comm_tool.h"
#include "ovfs_network_app.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_network_utility_api.h"
#include "ovfs_network_mgr.h"
#include "ovfs_wifi.h"
#include "ovfs_arp_tool_api.h"
#include "ovfs_network_rest.h"
#include "ovfs_network_4g.h"

using namespace ovfs_network;
using namespace wifi;

extern char str_RNDISifname[8];
char g_gunUsb[8] = {0};
extern WiFi g_Wifi;
extern pthread_mutex_t g_ip_set_lock;
extern pthread_mutex_t g_dns_set_lock;
extern int Call_SpeakInfo(char *source);

int g_metric = 0;
int s_nWiredExtranet = 0;
Common_InterSleep_T m_sleep_ops;
Common_Lock_T m_lock;

S8 acSerialNumber[128] = {0};
NETWORK_MGR_CONTEXT_T s_network_mgr_ct;
static int s_reset_netcard_cur_time = 0;
static int s_reset_netcard_lst_time = 0;
static int s_check_dhcp_failed_cnt = 0;
static int s_ip_conflict_delay_cnt = 0;
static char str_NetworkCustom[64] = "/update/res/custom/Network.json";
static int g_network_has_init = 0;
static int g_need_send_status = 0;

//static char strLastifname[8] = "";
#ifndef AWSIOT
typedef int	(*ALIWIFISDK_GETSET_FUNC_F)(int nType, void *pData, Common_cJSON_T* in, Common_cJSON_T* out);
typedef int	(*EXTER_ALIWIFISDK_SET_CALLBACK_F)(ALIWIFISDK_GETSET_FUNC_F get, ALIWIFISDK_GETSET_FUNC_F set);
#endif

typedef int (*STREAM_CALLBACK_OPEN) (char *pUrl);
typedef int (*STREAM_CALLBACK_CLOSE) (int hStreamHandle);
//typedef int (*STREAM_CALLBACK_CONTROL) (int hStreamHandle,void *pInParam);
typedef int (*STREAM_CALLBACK_READ) (int hStreamHandle,char **pData,int *pDataSize);
typedef int (*STREAM_CALLBACK_RELEASE) (int hStreamHandle);

typedef struct
{
    STREAM_CALLBACK_OPEN          fxnOpen;
    STREAM_CALLBACK_CLOSE         fxnClose;
    STREAM_CALLBACK_READ          fxnRead;
    STREAM_CALLBACK_RELEASE       fxnRelease;
    //STREAM_CALLBACK_CONTROL       fxnControl;
} ALIWIFISDK_STREAMCONTROL;

typedef int (*EXTER_ALIWIFISDK_CALL_FUNCTION_F)(int type,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);

typedef int (*EXTER_ALIWIFISDK_INIT_F)();
typedef int (*EXTER_ALIWIFISDK_UNINIT_F)();
typedef int (*EXTER_ALIWIFISDK_CONFIG_F)(const char* uriString, void *indata, void *outdata, int type);
typedef int	(*EXTER_ALIWIFISDK_CALLBACK_F)(EXTER_ALIWIFISDK_CALL_FUNCTION_F callfun);
typedef int (*EXTER_ALIWIFISDK_STREAMCALLBACK_F)(ALIWIFISDK_STREAMCONTROL *sc);

typedef struct
{
    EXTER_ALIWIFISDK_INIT_F      init;
    EXTER_ALIWIFISDK_UNINIT_F    uninit;
	EXTER_ALIWIFISDK_CONFIG_F    config;
    EXTER_ALIWIFISDK_CALLBACK_F  callback;
    EXTER_ALIWIFISDK_STREAMCALLBACK_F  streamCallback;
#ifndef AWSIOT
	EXTER_ALIWIFISDK_SET_CALLBACK_F    setCallback;
#endif
}ALIWIFISDK_EXTERNAL_LIBS_T;

static ALIWIFISDK_EXTERNAL_LIBS_T       s_aliwififuncCb;

static int LoadExternalAliwifiLibs()
{
	void * libHdl  = NULL;
	char acFileName[128] = {0};

	memset(acFileName, 0, sizeof(acFileName));
	sprintf(acFileName, "%s", "/tmp/libaliwifi_sdk.so");
	libHdl	= dlopen(acFileName, RTLD_LAZY);
	if(libHdl == NULL)
	{
		char *error_message = dlerror();
		LOGE("dlopen file[%s] failed. error[%s]\n",acFileName,error_message);

		memset(acFileName, 0, sizeof(acFileName));
		sprintf(acFileName, "%s", "/root/lib/libaliwifi_sdk.so");
		libHdl	= dlopen(acFileName, RTLD_LAZY);
	}

	if(libHdl)
	{
		LOGD("dlopen file[%s] succed.\n",acFileName);

		s_aliwififuncCb.init  = (EXTER_ALIWIFISDK_INIT_F )dlsym(libHdl, "libaliwifi_sdk_init");
		if(s_aliwififuncCb.init == NULL)
		{
			LOGE("Not found libaliwifi_sdk_init\n");
			return -1;
		}

		s_aliwififuncCb.uninit	= (EXTER_ALIWIFISDK_UNINIT_F )dlsym(libHdl, "libaliwifi_sdk_uninit");
		if(s_aliwififuncCb.uninit == NULL)
		{
			LOGE("Not found libaliwifi_sdk_uninit\n");
			return -1;
		}
#ifdef AWSIOT
		s_aliwififuncCb.config = (EXTER_ALIWIFISDK_CONFIG_F )dlsym(libHdl, "libaliwifi_sdk_config");
		if(s_aliwififuncCb.config == NULL)
		{
			LOGE("Not found libaliwifi_sdk_config\n");
			return -1;
		}

		s_aliwififuncCb.callback = (EXTER_ALIWIFISDK_CALLBACK_F)dlsym(libHdl, "libaliwifi_sdk_callback");
        if(s_aliwififuncCb.callback == NULL)
		{
			LOGE("Not found libaliwifi_sdk_callback\n");
			return -1;
		}

        s_aliwififuncCb.streamCallback = (EXTER_ALIWIFISDK_STREAMCALLBACK_F)dlsym(libHdl, "libaliwifi_sdk_streamcallback");
        if(s_aliwififuncCb.streamCallback == NULL)
		{
			LOGE("Not found libaliwifi_sdk_streamcallback\n");
			return -1;
		}
#else
        s_aliwififuncCb.setCallback = (EXTER_ALIWIFISDK_SET_CALLBACK_F)dlsym(libHdl, "libaliwifi_sdk_set_callback");
#endif

	}
	else
	{
		char *error_message = dlerror();
		LOGE("dlopen file[%s] failed. error[%s]\n",acFileName,error_message);
		return -1;
	}

	return 0;
}

#ifndef AWSIOT
static int AliwifiSetCallBack(int nType, void *pData, Common_cJSON_T* in, Common_cJSON_T* out)
{
	int ret = -1;

	switch (nType)
    {
        case 1:
			LOGE("AliwifiSetCallBack RebootSystem\n");
            ret = RebootSystem();
            break;
        default:
            ret = -1;
			break;
    }

	return ret;
}

static int AliwifiGetCallBack(int nType, void *pData, Common_cJSON_T* in, Common_cJSON_T* out)
{
	int ret = -1;

	switch (nType)
    {
        default:
            ret = -1;
			break;
    }

	return ret;
}
#endif

int NetWork_WifiConfig(int type, const char* uriString,Common_cJSON_T* in,Common_cJSON_T* out)
{
    if(s_aliwififuncCb.config)
    {
        LOGW("uriString[%s] type[%d]in:[%p]\n",uriString,type, in);
	    return s_aliwififuncCb.config(uriString, in, out, type);
    }
    return 0;
}

int sendInfo(cJSON_Struct *data)
{
    cJSON_Struct *pInData = NULL,*pOutData = NULL;

    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValueObj(pInData, "Header");
    Common_Json_SetAttrValueStr(pInData, "Header/Uri", "/WebServer/Wifi");
    Common_Json_SetAttrValueStr(pInData, "Header/Method", "put");
    Common_Json_AddItem(pInData, -1, "Data", data);

    Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);

    Common_cJSON_DetachItemFromObject((Common_cJSON_T *)pInData, "Data");

    Common_Json_Delete(pInData);
    Common_Json_Delete(pOutData);

    return 0;
}

int sendDeviceStatus(cJSON_Struct *data)
{
    int ret = -1;
    ovfs_print_json(data);

    cJSON_Struct *pInData = NULL,*pOutData = NULL;

    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValueObj(pInData, "Header");
    Common_Json_SetAttrValueStr(pInData, "Header/Uri", "/WebServer/DeviceStatus");
    Common_Json_SetAttrValueStr(pInData, "Header/Method", "put");
    cJSON_Struct *pDataTmp = Common_Json_Duplicate(data, 1);
    Common_Json_AddItem(pInData, -1, "Data", pDataTmp);

    Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
    Common_Json_GetAttrValueInt(pOutData, "Header/Code", &ret);
LOGD("ret:[%d]\n",ret);
    Common_Json_Delete(pInData);
    Common_Json_Delete(pOutData);

    return ret;
}

static int AliwifiCallBack(int type,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	int ret = 0, nSatus = -1;

    if(access("/tmp/network_debug",F_OK) == 0 || type != 0)
    {
        char* outStr = Common_Json_Print(pInParams,NULL);
        LOGW("type:[%d] in:[%s]\n", type, outStr);
        if(outStr)
            Common_Free(outStr,__FUNCTION__,__LINE__);
    }

	if(type == 0)
	{
        Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInParams,pOutParams,3000);
    }
    else if(type == 1)
    {
        g_need_send_status = 1;
        g_metric = 1;
        sendInfo(pInParams);
    }
    else  if(type == 2)
    {
		Common_Json_GetAttrValueInt(pInParams, "Status", &nSatus);
		if(nSatus == 1)
		{
			g_need_send_status = 1;
			g_metric = 1;
		}
    }

	return ret;
}

static int StreamOpen(char *pUrl)
{
    int ret = 0;
    CoOpen_Param_T *param = (CoOpen_Param_T *)calloc(1, sizeof(CoOpen_Param_T) * 1 * 1);

    param[0].nIndex = 0;
    param[0].szUri = pUrl;
    param[0].nMode = 0;

    ret = Module_StreamQueue_CoOpen(NetWork_RestComm_GetModuleHdl(), -1, param, 1, 1000);

    free(param);
    param = NULL;

    return ret;
}


static int StreamClose(int hStreamHandle)
{
    return Module_StreamQueue_Close(NetWork_RestComm_GetModuleHdl(), hStreamHandle);
}


static int StreamRead(int hStreamHandle,char **pData,int *pDataSize)
{
    return Module_StreamQueue_ReadData(NetWork_RestComm_GetModuleHdl(), hStreamHandle, -1, NULL, NULL, (void **)pData, pDataSize, 100);
}

static int StreamRelease(int hStreamHandle)
{
    return Module_StreamQueue_ReleaseData(NetWork_RestComm_GetModuleHdl(), hStreamHandle);
}


void String2Upper(char *pStr)
{
	if (NULL == pStr)
	{
		return;
	}

	size_t i = 0;
	size_t j = strlen(pStr);

	for (i = 0; i < j; i++)
	{
		pStr[i] = toupper(pStr[i]);
	}
}


//1- 失败0-v3版本1-v4版本
static int GetP2pVersion()
{
	S32 iRet = -1;
	S8 *szStringVal = NULL;
	S8 acVersion[64] = {0};
	cJSON_Struct *pInData = NULL,*pOutData = NULL;
	cJSON_Struct *pItem = NULL;

	pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pInData == NULL)
	{
		LOGE("Common_Json_New failed\n");
		return -1;
	}

	Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/AliIoT4ovfs/iotcfg",0,0);
	Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"get",0,0);

	iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
	if(0 != iRet)
	{
		LOGE("Module_CallFunctions failed. ret=[%d]\n",iRet);
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);
		return -1;
	}

	pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/Version", NULL, &szStringVal, NULL, NULL);
	if(pItem == NULL)
	{
		LOGE("Common_Json_GetAttrValue failed\n");
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);
		return -1;
	}

	strcpy(acVersion, szStringVal);

	Common_Json_Delete(pInData);
	Common_Json_Delete(pOutData);

	String2Upper(acVersion);

	if((strncmp(acVersion, "V03", 3) == 0) || (strncmp(acVersion, "V3", 2) == 0))
	{
		return 0;
	}
	else if((strncmp(acVersion, "V04", 3) == 0) || (strncmp(acVersion, "V4", 2) == 0))
	{
		return 1;
	}

	return -1;
}


#if 0

/**
 * 解析RTA,并存入tb
 */
static void ParseRtattr(struct rtattr **tb, int max, struct rtattr *attr, int len)
{
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len))
    {
        if (attr->rta_type <= max)
        {
            tb[attr->rta_type] = attr;
        }
    }
}

/**
 * 显示连接信息
 * 当网卡变动的时候触发这个信息,例如插/拔网线,增/减网卡设备,启用/禁用接口等.
 */
static void PrintIfInfoMsg(struct nlmsghdr *nh)
{
    int len;
    struct rtattr *tb[IFLA_MAX + 1];
    struct ifinfomsg *ifinfo;
    bzero(tb, sizeof(tb));
    ifinfo = (struct ifinfomsg *) NLMSG_DATA(nh);
    len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
    ParseRtattr(tb, IFLA_MAX, IFLA_RTA(ifinfo), len);
    LOGW("%s: %s flag=%x change=%x\n", (nh->nlmsg_type == RTM_NEWLINK) ? "NEWLINK" : "DELLINK",
         (ifinfo->ifi_flags & IFF_RUNNING) ? "up" : "down", ifinfo->ifi_flags, ifinfo->ifi_change);
    if (tb[IFLA_IFNAME])
    {
        LOGW("%s\n", RTA_DATA(tb[IFLA_IFNAME]));
        char *rel = strstr((char *)RTA_DATA(tb[IFLA_IFNAME]), "eth");
        if (rel != NULL)
        {
            s_network_mgr_ct.isNetChange = 1;
			if (ifinfo->ifi_flags & IFF_RUNNING)
			{
				s_network_mgr_ct.event[0] = 0;
			}
			else
			{
				s_network_mgr_ct.event[0] = 1;
			}
        }
    }
    //Mq_PostMsg(s_network_mgr_ct.mqHandle, NETWORK_MSG_MGR_NET_OUT, NULL, 0);
}

/**
 * 显示地址信息
 * 当地址变动的时候触发这个信息,例如通过DHCP获取到地址后
 */
static void PrintIfAddrMsg(struct nlmsghdr *nh)
{
    int len;
    struct rtattr *tb[IFA_MAX + 1];
    struct ifaddrmsg *ifaddr;
    char tmp[256];
    bzero(tb, sizeof(tb));
    ifaddr = (struct ifaddrmsg *) NLMSG_DATA(nh);
    len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*ifaddr));
    ParseRtattr(tb, IFA_MAX, IFA_RTA(ifaddr), len);

    LOGW("%s \n", (nh->nlmsg_type == RTM_NEWADDR) ? "NEWADDR" : "DELADDR");
    if (tb[IFA_LABEL] != NULL)
    {
        LOGW("%s \n", RTA_DATA(tb[IFA_LABEL]));
    }
    if (tb[IFA_ADDRESS] != NULL)
    {
        inet_ntop(ifaddr->ifa_family, RTA_DATA(tb[IFA_ADDRESS]), tmp, sizeof(tmp));
        LOGW("%s \n", tmp);
    }
    LOGW("\n");
}

/**
 * 显示路由信息
 * 当路由变动的时候触发这个信息
 */
static void PrintRtMsg(struct nlmsghdr *nh)
{
    int len;
    struct rtattr *tb[RTA_MAX + 1];
    struct rtmsg *rt;
    char tmp[256];
    bzero(tb, sizeof(tb));
    rt = (struct rtmsg *) NLMSG_DATA(nh);
    len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*rt));
    ParseRtattr(tb, RTA_MAX, RTM_RTA(rt), len);
    LOGW("%s: \n", (nh->nlmsg_type == RTM_NEWROUTE) ? "NEWROUT" : "DELROUT");
    if (tb[RTA_DST] != NULL)
    {
        inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_DST]), tmp, sizeof(tmp));
        LOGW("RTA_DST %s \n", tmp);
    }
    if (tb[RTA_SRC] != NULL)
    {
        inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_SRC]), tmp, sizeof(tmp));
        LOGW("RTA_SRC %s \n", tmp);
    }
    if (tb[RTA_GATEWAY] != NULL)
    {
        inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), tmp, sizeof(tmp));
        LOGW("RTA_GATEWAY %s \n", tmp);
    }

}
#endif

int SendArpCheck ( char * if_name,char * mac,char * ip,char * broad_mac,char * dest )
{
    Ether_pkg pkg;
    struct hostent *host =NULL;
    struct sockaddr sa;
    int sockfd=0,len=0;
    char buffer[255] = {0};
    memset(( char * )&pkg,'\0',sizeof(pkg) );

    /* 填充ethernet包文 */
    memcpy ( ( char * ) pkg.ether_shost, ( char * ) mac,6 );
    memcpy ( ( char * ) pkg.ether_dhost, ( char * ) broad_mac,6 );
    pkg.ether_type = htons ( ETHERTYPE_ARP );


    /* 下面填充arp包文 */
    pkg.ar_hrd = htons ( ARPHRD_ETHER );
    pkg.ar_pro = htons ( ETHERTYPE_IP );
    pkg.ar_hln = 6;
    pkg.ar_pln = 4;
    pkg.ar_op = htons ( ARPOP_REQUEST );
    memcpy ( ( char * ) pkg.arp_sha, ( char * ) mac,6 );
    memcpy ( ( char * ) pkg.arp_spa, ( char * ) ip,4 );
    memcpy ( ( char * ) pkg.arp_tha, ( char * ) broad_mac,6 );

    LOGI ( "Resolve [%s],Please Waiting...\n",dest );

    /*以欺骗方式发包，会造成IP冲突错误 */
    //fflush( stdout );
    //memset( ip,0,sizeof(ip) );
    if ( inet_aton ( dest, ( struct in_addr * ) ip ) ==0 )
    {
        if ( ( host = gethostbyname ( dest ) ) ==NULL )
        {
            fprintf ( stderr,"Fail! %s\n\a",hstrerror ( h_errno ) );
            return ( -1 );
        }
        memcpy ( ( char * ) ip,host->h_addr,4 );
    }

    memcpy ( ( char * ) pkg.arp_tpa, ( char * ) ip,4 );
	/*unsigned char tip[5];
    memset(tip,0,sizeof(tip));
	inet_aton(dest,(struct in_addr *)tip);
	memcpy((char *)pkg.arp_tpa,(char *)tip,4);*/
     /* 实际应该使用 PF_PACKET */
    if ( ( sockfd = socket ( PF_INET,SOCK_PACKET,htons ( ETH_P_ALL ) ) ) ==-1 )
    {
        fprintf ( stderr,"Socket Error:%s\n\a",strerror ( errno ) );
        return ( 0 );
    }

    memset ( &sa,0,sizeof ( sa ) );
    strcpy ( sa.sa_data,if_name );

    len = sendto ( sockfd,&pkg,sizeof ( pkg ),0,&sa,sizeof ( sa ) );
    if ( len != sizeof ( pkg ) )
    {
        fprintf ( stderr,"Sendto Error:%s\n\a",strerror ( errno ) );
		close(sockfd);
        return ( 0 );
    }
    Ether_pkg *parse;
    parse = ( Ether_pkg * ) buffer;

	int times = 5;

	while( times > 0 )
	{
	    fd_set readfds;
	    struct timeval tv;
	    tv.tv_sec = 2;
	    tv.tv_usec = 500000; /*500毫秒*/
	    FD_ZERO ( &readfds );
	    FD_SET ( sockfd, &readfds );
	    len = select ( sockfd+1, &readfds, 0, 0, &tv );
	    if ( len > -1 )
	    {
	        if ( FD_ISSET ( sockfd, &readfds ) )
	        {
		            memset ( buffer, 0, sizeof ( buffer ) );
		            len=recvfrom ( sockfd, buffer, sizeof ( buffer ), 0, NULL, (socklen_t *)&len );
					LOGD("parse->ether_type = %.04X, parse->ar_op is %.04X\n", ntohs ( parse->ether_type ), ntohs( parse->ar_op ) );
					if ( ntohs ( parse->ether_type ) == ETHERTYPE_ARP )
					{
		                   if ( ntohs ( parse->ar_op ) == ARPOP_REPLY )
							{
								struct in_addr *src = ( struct in_addr * )(parse->arp_spa);
								struct in_addr *dst = ( struct in_addr * )(parse->arp_tpa);
								printf ( "\nsrc IP = [%s] MAC = [%s]\n", inet_ntoa(*src), ether_ntoa( (ether_addr*)parse->arp_sha ));
								printf ( "dst IP = [%s] MAC = [%s]\n", inet_ntoa(*dst), ether_ntoa( (ether_addr*)parse->arp_tha ));
								close(sockfd);
								return 1;
							}
							else
							{
								LOGI("here recv packet is not arp respose!!! parse->ar_op is %.04X\n", ntohs( parse->ar_op ));
								times--;
								continue;
							}
					}
					else if ( ntohs( parse->ether_type) == ETHERTYPE_IP )
					{
						LOGI("here recv packet is IP !!!\n");
						times--;
						continue;
					}
					else
					{
						LOGI("here recv packet is not arp !!!\n");
						pHx((unsigned char *)buffer, sizeof(buffer));
						break;
					}

	        }
	        else
	        {
	            LOGW("time is coming here !!!\n");
	        }
	    }
	    else
	    {
	        LOGW("select is return < 0 here !!!\n");
	    }
	}
	close(sockfd);
    return 0;
}

int GetDNS(ovfs_soft::ovfs_local_net_dns *dns,char *path)
{
	if(path == NULL ) return -1;
	memset(dns, 0, sizeof(ovfs_local_net_dns));
	FILE *fp = fopen(path, "r");
	if(fp == NULL)
		return -1;

	S8 sz_param[256];
	U32 i = 0;
	while(fgets(sz_param, sizeof(sz_param), fp) != NULL && i < 3)
	{
		S8 *pstr = strstr(sz_param, "nameserver ");
		S8 *pstr1 = strstr(sz_param, "\n");

		if(NULL != pstr)
		{
			i++;
		}

		if(i==3 || NULL == pstr ||NULL == pstr1)
		{
			continue;
		}

		ovfs_ipaddr_struct *ip_addr = i==1?&dns->dns1:&dns->dns2;

		//LOGD("%s\n", sz_param);
		ip_addr->is_ipv4 = OVFS_TRUE;

		if(strstr(sz_param, ".") != NULL)
		{
			ip_addr->is_ipv4 = OVFS_TRUE;
			pstr += strlen("nameserver ");
			*pstr1 = '\0';
			snprintf(ip_addr->ipv4, sizeof(ip_addr->ipv4), "%s", pstr);

		}
		else if(strstr(sz_param, ":") != NULL)
		{
			ip_addr->is_ipv4 = OVFS_FALSE;
			pstr += strlen("nameserver ");
			*pstr1 = '\0';
			snprintf(ip_addr->ipv6, sizeof(ip_addr->ipv6), "%s", pstr);
		}
		else
		{
			continue;
		}
	}

	fclose(fp);
	return 0;
}

#if 0
static S32 NetMonitorThread(Common_Thread_T hThreadHandle,OVFS_VOID *param)
{
    prctl(PR_SET_NAME, __func__);
	if(NULL == param)
	{
		return -1;
	}

	ovfs_network_config_mgr *net_mgr = (ovfs_network_config_mgr *)param;

    int socketFd = 0, retSelect = 0, retRead = 0, recvLen = 0;
    fd_set rfds;
    struct timeval timeout = { 0, 0 };
    struct sockaddr_nl sa;
    struct nlmsghdr *nh = NULL;
    char *buf = (char *) MALLOC(1024);
    recvLen = 1024;

    if (buf == NULL)
    {
        LOGE("malloc failed\n");
        return -1;
    }

    /*打开NetLink Socket*/
    socketFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (socketFd < 0)
    {
        LOGE("create socket failed\n");
        FREE(buf);
        return -1;
    }

    if (setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF, &recvLen, sizeof(recvLen)) < 0)
    {
        LOGE("set socket recv buf failed\n");
        close(socketFd);
        FREE(buf);
        return -1;
    }

    /*设定接收类型并绑定Socket*/
    bzero(&sa, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE;
    if (bind(socketFd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
        LOGE("bind socket failed\n");
        close(socketFd);
        FREE(buf);
        return -1;
    }

	char if_name[6] = "eth0";
    char mac[OVFS_MAC_STRING_LEN + 1]={0};
	char broad_mac[7]={0};
	char ip[OVFS_IPV6_STRING_LEN + 1]={0},gateway[OVFS_IPV6_STRING_LEN + 1]={0};

	int iFailedCount = 0;

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(socketFd, &rfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        retSelect = select(socketFd + 1, &rfds, NULL, NULL, &timeout);

        if (retSelect > 0 && FD_ISSET(socketFd, &rfds))
        {
            retRead = read(socketFd, buf, recvLen);
            if (retRead < (int) sizeof(struct nlmsghdr))
                continue;

            nh = (struct nlmsghdr *) buf;
            PRINT_DBG("len %d type %d\n", nh->nlmsg_len, nh->nlmsg_type);
            for (nh = (struct nlmsghdr *) buf; NLMSG_OK(nh, (unsigned int )retRead); nh = NLMSG_NEXT(nh, retRead))
            {
                switch (nh->nlmsg_type)
                {
                case NLMSG_DONE:
                case NLMSG_ERROR:
                    break;
                case RTM_NEWLINK:/*16--链路相关的*/
                case RTM_DELLINK:
                    PrintIfInfoMsg(nh);
                    break;
                case RTM_NEWADDR:/*20--ip 地址*/
                case RTM_DELADDR:
                    PrintIfAddrMsg(nh);
                    break;
                case RTM_NEWROUTE:/*24--路由信息*/
                case RTM_DELROUTE:
                    PrintRtMsg(nh);
                    break;
                default:
                    /*收到些奇怪的信息*/
                    break;
                }
            }
        }

	#if 0
		if(NetWorkTool_CheckConnect(NULL,TEST_DNS1,DNS_PORT) || NetWorkTool_CheckConnect(NULL,TEST_DNS2,DNS_PORT))
		{
			snprintf(if_name,sizeof(if_name),"eth0");
			if(NetWorkTool_GetIFFlags(if_name) == 0)
			{
				if(NetWorkTool_CheckConnect(if_name,TEST_DNS1,DNS_PORT) || NetWorkTool_CheckConnect(if_name,TEST_DNS2,DNS_PORT))
				{
					NetWorkTool_UpdateDefaultRoute(if_name);
				}
			}

			Common_Sleep(10, 0);

			iFailedCount = 0;//可通外网清除计数（有可能是网络不好引起）
		}
		else//获取公网IP失败后，检查网关IP
		{
			snprintf(if_name,sizeof(if_name),"wlan0");
			if(NetWorkTool_GetIFFlags(if_name) == 0)
			{
				if(NetWorkTool_CheckConnect(if_name,TEST_DNS1,DNS_PORT) || NetWorkTool_CheckConnect(if_name,TEST_DNS2,DNS_PORT))
				{
					NetWorkTool_UpdateDefaultRoute(if_name);
				}
			}

			snprintf(if_name,sizeof(if_name),"eth0");
			NetWorkTool_MacIP((char *)if_name,mac,ip);
			NetWorkTool_GetGateway((char *)if_name,gateway);

			struct in_addr *localip = (struct in_addr *) ip;
			LOGD ( "local Mac=[%s] Ip=[%s] gateway={%s}\n", ether_ntoa ((ether_addr*)mac),inet_ntoa (*localip),gateway);

			ovfs_netcard_config eth_cfg;
			OVFS_CLR_ARG(eth_cfg);
			if(OVFS_SUCCESS == ovfs_cfgm_get_local_eth_cfg(if_name, &eth_cfg))
			{
				LOGD("dhcp=%d\n",eth_cfg.dhcp);
				if(eth_cfg.dhcp == OVFS_TRUE)//动态IP条件下才去检查网关IP是否连通
				{
					int ret = SendArpCheck((char *)if_name,mac,ip,broad_mac,gateway);
					LOGD("ret=%d,iFailedCount=%d\n",ret,iFailedCount);

					if(1 == ret)
					{
						Common_Sleep(15, 0);

						iFailedCount = 0;
					}
					else
					{
						if(ovfs_utility_is_net_dev_out("eth0") == 0)//eth0网线连接条件下才记失败次数的。
							iFailedCount ++;

						if(iFailedCount >= 2)
						{
							iFailedCount = 0;
							LOGE("Need Change DHCP!\n");
							net_mgr->update_netcard_dhcp_cfg(&eth_cfg);
						}
						Common_Sleep(10, 0);
					}

				}
			}
		}

		#endif

    }
    if(buf) FREE(buf);
    close(socketFd);
    return 0;
}

static S32 WiFiMonitorThread(Common_Thread_T hThreadHandle,OVFS_VOID *param)
{
    prctl(PR_SET_NAME, __func__);
	if(NULL == param)
	{
		return -1;
	}

	Common_Sleep(20, 0);

	struct timespec tpstart;
	struct timespec tpend;
	long timedif;
	WIFI_WORK_MODE wifi_mode,wifi_lastmode = CLOSE;
	WIFI_WORK_STA_STATUS_E wifi_stastatus,wifi_laststatus = WIFI_WORK_STA_STATUS_DISCONNECTED;

	int isCheckTime = 0;
	clock_gettime(CLOCK_MONOTONIC, &tpstart);
	clock_gettime(CLOCK_MONOTONIC, &tpend);

	while(1)
	{
		Common_Lock(m_lock);
		wifi_mode = g_Wifi.getCurrentMode();
		wifi_stastatus = g_Wifi.GetConnStatus();
		Common_UnLock(m_lock);

		if(wifi_mode != wifi_lastmode)
		{
			LOGD("Wifi Change Mode:%d\n",wifi_mode);
			if(wifi_mode == AP)
			{
				if(g_Wifi.isReady())
					Call_SpeakInfo((char *)"wifi_ap_ok");
			}
			else if(wifi_mode == STA)
			{
				isCheckTime = 1;
				clock_gettime(CLOCK_MONOTONIC, &tpstart);
			}
			wifi_lastmode = wifi_mode;
		}

		if(wifi_mode == STA)
		{
			if(wifi_stastatus != wifi_laststatus)
			{
				LOGD("Wifi Change[%d] Status:%d\n",wifi_laststatus,wifi_stastatus);
				if(wifi_stastatus == WIFI_WORK_STA_STATUS_COMPLETED)
				{
					if(wifi_laststatus != WIFI_WORK_STA_STATUS_FAILED)
						Call_SpeakInfo((char *)"wifi_conn_ok");
					if(g_Wifi.isConfigSTA)
					{
						NetWork_SaveCfg();
						g_Wifi.isConfigSTA = false;
					}
					if(g_Wifi.update())
					{
						if(ovfs_utility_is_net_dev_out("eth0"))
						{
							//NetWorkTool_UpdateDefaultRoute((char *)"wlan0");
						}
						else
						{
							//NetWorkTool_UpdateDefaultRoute((char *)"eth0");
						}
					}
				}
				else if(wifi_stastatus == WIFI_WORK_STA_STATUS_4WAY_HANDSHAKE)
				{
					if(wifi_laststatus == WIFI_WORK_STA_STATUS_DISCONNECTED)
					{
						isCheckTime = 1;
						LOGD("Start Timer!\n");
						clock_gettime(CLOCK_MONOTONIC, &tpstart);
					}
				}
				wifi_laststatus = wifi_stastatus;
			}

			if(wifi_stastatus != WIFI_WORK_STA_STATUS_COMPLETED)
			{
				clock_gettime(CLOCK_MONOTONIC, &tpend);
				timedif = tpend.tv_sec-tpstart.tv_sec;
				//LOGD("it took %ld seconds\n", timedif);
				if(timedif > 40)
				{
					if(isCheckTime)
					{
						if((g_Wifi.isConfigSTA) || (ovfs_utility_is_net_dev_out("eth0")))
						{
							Call_SpeakInfo((char *)"wifi_conn_fail");
							g_Wifi.setParam(SWITCH_MODE,AP);
							g_Wifi.isConfigSTA = 0;
						}
						isCheckTime = 0;
					}
				}
			}
			else
			{
				if(s_network_mgr_ct.isNetChange)
				{
					if(s_network_mgr_ct.event[0] == 0)
					{
						//NetWorkTool_UpdateDefaultRoute((char *)"eth0");
					}
					else
					{
						//NetWorkTool_UpdateDefaultRoute((char *)"wlan0");
					}
					s_network_mgr_ct.isNetChange = 0;
				}
			}
		}

	}
}
#endif

static S32 NetcardHasValidAddr(const char *if_name)
{
    if(if_name == NULL)
    {
        return 0;
    }

    char acIpAddr[32];

    memset(acIpAddr, 0, sizeof(acIpAddr));

    NetWorkTool_GetIPAddr((char *)if_name, acIpAddr);

    if(strlen(acIpAddr) == 0 || stricmp(acIpAddr, "0.0.0.0") == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

static int g_metric_old = 0;
static int g_metric_subsribe = 0;

int is4gDevice()
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

	pItem = Common_Json_GetAttrValue(pOutData, -1, "Data/SerialNumber", NULL, &szStringVal, NULL, NULL);
	if(pItem == NULL || szStringVal == NULL)
	{
		LOGE("Common_Json_GetAttrValue failed\n");
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);
		return -1;
	}

	strcpy(acSerialNumber, szStringVal);

	Common_Json_Delete(pInData);
	Common_Json_Delete(pOutData);

	if(strstr(acDeviceString, "4g") != NULL)
	{
		return 1;
	}

	if((strncmp(acSerialNumber, "0347", 4) == 0) ||
	   (strncmp(acSerialNumber, "8347", 4) == 0) ||
	   (strncmp(acSerialNumber, "0342", 4) == 0) ||
	   (strncmp(acSerialNumber, "034f", 4) == 0) ||
	   (strncmp(acSerialNumber, "0345", 4) == 0) ||
	   (strncmp(acSerialNumber, "034d", 4) == 0) ||
	   (strncmp(acSerialNumber, "0382", 4) == 0) ||
	   (strncmp(acSerialNumber, "0346", 4) == 0))
	{
		return 1;
	}

	return 0;
}

int Read_NetworkCustom(int *pWiredExtranet)
{
	U32 nStrLen = 0;
	FILE *fp = NULL;
	int nWiredExtranet = 0;
	char *pConfigString = NULL;
	cJSON_Struct *pConfigJson = NULL;

	fp = fopen(str_NetworkCustom,"rb");
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

	Common_Json_GetAttrValue(pConfigJson,-1,"WiredExtranet",NULL,NULL,&nWiredExtranet,NULL);

	*pWiredExtranet = nWiredExtranet;

	if (pConfigJson != NULL)
	{
		Common_Json_Delete(pConfigJson);
		pConfigJson = NULL;
	}

	return 0;
}

#if 0
static S32 LteMonitorThread(Common_Thread_T hThreadHandle,OVFS_VOID *param)
{
    prctl(PR_SET_NAME, __func__);
	if(NULL == param)
	{
		return -1;
	}
	int cnt = 0;
	int card_num = 0;
	int b_ethConnected = -1,b_wlanConnected = -1,b_pppConnected = -1;
	S32 n4gThreadLstTime = 0;
	S8 s8DeviceType[32]= {0};
	OVFS_BOOL bEthDhcp = OVFS_FALSE;
	ovfs_soft::ovfs_netcard_config eth_conf;
	while(1)
	{
		card_num = 0;
#ifdef AUTOROUTESWITCH
		if(access("/sys/class/net/eth0",F_OK) == 0)
		{
			ovfs_soft::ovfs_local_net_dns eth_dns;
			GetDNS(&eth_dns,(char *)"/tmp/resolv.conf");

			if(NetWorkTool_CheckConnect((char *)"eth0",eth_dns.dns1.ipv4) || NetWorkTool_CheckConnect((char *)"eth0",eth_dns.dns2.ipv4))
			{
				b_ethConnected = 1;
			}
			else
			{
				b_ethConnected = 0;
			}
			card_num++;
		}
		if(access("/sys/class/net/wlan0",F_OK) == 0)
		{
			ovfs_soft::ovfs_local_net_dns wlan_dns;
			GetDNS(&wlan_dns,(char *)"/tmp/udhcpc_resolv_wlan0.conf");

			if(NetWorkTool_CheckConnect((char *)"wlan0",wlan_dns.dns1.ipv4) || NetWorkTool_CheckConnect((char *)"wlan0",wlan_dns.dns2.ipv4))
			{
				b_wlanConnected = 1;
			}
			else
			{
				b_wlanConnected = 0;
			}
			card_num++;
		}
		if(access("/sys/class/net/ppp0",F_OK) == 0)
		{
			ovfs_soft::ovfs_local_net_dns ppp_dns;
			GetDNS(&ppp_dns,(char *)"/etc/ppp/resolv.conf");
			if(b_pppConnected <= 0 || (cnt & 0x1F) == 0)
			{
				cnt = 0;
				if(NetWorkTool_CheckConnect((char *)"ppp0",ppp_dns.dns1.ipv4) || NetWorkTool_CheckConnect((char *)"ppp0",ppp_dns.dns2.ipv4))
				{
					b_pppConnected = 1;
				}
				else
				{
					b_pppConnected = 0;
				}
			}
			card_num++;
		}
		else
			b_pppConnected = 0;
#else
		if(access("/sys/class/net/wlan0",F_OK) == 0)
		{
			b_wlanConnected = 1;
			b_pppConnected = 0;
			b_ethConnected = 0;
			card_num = 2;
		}
		else if(access("/sys/class/net/ppp0",F_OK) == 0)
		{
			b_wlanConnected = 0;
			b_pppConnected = 1;
			b_ethConnected = 0;
			card_num = 2;
		}
		else
		{
			b_wlanConnected = 0;
			b_pppConnected = 0;
			b_ethConnected = 1;
			card_num = 2;
		}
#endif

		char system_cmd[128] = {0};

		{
			//获取设备类型
			if(strlen(s8DeviceType) == 0)
			{
				GetDeviceType(s8DeviceType);
			}

			//如果是4 G 小黄人
			if(strcmp(s8DeviceType, "ipc_iot4g_dome") == 0)
			{
				char eth_gateway[32] = {0};
				NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
				if(strlen(eth_gateway) && strcmp(eth_gateway,"0.0.0.0"))
				{
					snprintf(system_cmd, sizeof(system_cmd), "route del default dev eth0");
					Common_System(system_cmd);
				}
			}
		}

		if(ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf) == OVFS_SUCCESS)
		{
			bEthDhcp = eth_conf.dhcp;
		}

		if(b_ethConnected == 1 || b_wlanConnected == 1 || b_pppConnected == 1)
		{
			LOGW("TotalNum=%d : eth0 Status:%d  wlan0 Status:%d  ppp0 Status:%d\n",card_num,b_ethConnected,b_wlanConnected,b_pppConnected);

			if(card_num > 1)
			{
				if(b_ethConnected == 1)
				{
					g_metric = 0;
					if(bEthDhcp == OVFS_FALSE)
					{
						LOGW("ETH is frist!!!\n");
						if(strcmp(strLastifname,"eth0") != 0)
						{
							char eth_gateway[32] = {0};
							NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
							if(0 == strcmp(eth_gateway,"0.0.0.0"))
							{
								ovfs_soft::ovfs_netcard_config eth_conf;
								 if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf))
								 {
									snprintf(eth_gateway,sizeof(eth_gateway),"%s",eth_conf.gate_way.ipv4);
								 }
							}

							snprintf(system_cmd, sizeof(system_cmd), "route del default dev ppp0; rm /tmp/ppp0_ok");
						    Common_System(system_cmd);
						    snprintf(system_cmd, sizeof(system_cmd), "route add default dev ppp0 metric 1 && touch /tmp/ppp0_ok");
						    Common_System(system_cmd);
							snprintf(system_cmd, sizeof(system_cmd), "route del default dev eth0");
						    Common_System(system_cmd);
						    snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s dev eth0 metric 0",eth_gateway);
						    Common_System(system_cmd);
							Common_System("cp /tmp/resolv.conf /etc/resolv.conf");
							//NetWorkTool_UpdateDefaultRoute(s_netcardListHdl,"eth0");
						}
					}
					snprintf(strLastifname,sizeof(strLastifname),"eth0");
				}
				else
				{
					if(b_wlanConnected == 1)
					{
						LOGW("WLAN is frist!!!\n");
						g_metric = 1;
						if(strcmp(strLastifname,"wlan0") != 0)
						{
							char wlan_gateway[32] = {0};
							NetWorkTool_GetGateway((char *)"wlan0",wlan_gateway);
							if(0 == strcmp(wlan_gateway,"0.0.0.0") || strlen(wlan_gateway) == 0)
							{
								ovfs_soft::ovfs_netcard_config eth_conf;
								 if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("wlan0",&eth_conf))
								 {
									snprintf(wlan_gateway,sizeof(wlan_gateway),"%s",eth_conf.gate_way.ipv4);
								 }
							}

							Common_System("route del default dev wlan0");
							snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s dev wlan0 metric 0",wlan_gateway);
							Common_System(system_cmd);
							Common_System("cp /tmp/udpcpc_resolv_wlan0.conf /etc/resolv.conf");
							//NetWorkTool_UpdateDefaultRoute(s_netcardListHdl,"wlan0");
							snprintf(strLastifname,sizeof(strLastifname),"wlan0");
						}
					}
					else if(b_pppConnected == 1)
					{
						LOGW("PPP is frist!!!\n");
						g_metric = 2;

						if(strcmp(strLastifname,"ppp0") != 0)
						{
							Common_System("route add default dev ppp0 metric 0 && touch /tmp/ppp0_ok");
							Common_System("cp /etc/ppp/resolv.conf /etc/resolv.conf");
							snprintf(strLastifname,sizeof(strLastifname),"ppp0");
							//NetWorkTool_UpdateDefaultRoute(s_netcardListHdl,"ppp0");
						}
						if(access("/tmp/ppp0_ok",F_OK) != 0)
						{
							snprintf(system_cmd, sizeof(system_cmd), "route del default dev ppp0");
							Common_System(system_cmd);
							snprintf(system_cmd, sizeof(system_cmd), "route add default dev ppp0 metric 0 && touch /tmp/ppp0_ok");
							Common_System(system_cmd);
						}

					}
				}
			}
			else if(access("/sys/class/net/eth0",F_OK) == 0)
			{
				LOGW("Only eth0 !!!\n");
				g_metric = 0;
				if(strcmp(strLastifname,"eth0") != 0)
					snprintf(strLastifname,sizeof(strLastifname),"eth0");
			}
		}
		else
		{
			g_metric = 0;
			if(bEthDhcp == OVFS_FALSE)
			{
				LOGW("all no connect ! ETH is frist!!!\n");
				if(strcmp(strLastifname,"eth0") != 0)
				{
					char eth_gateway[32] = {0};
					NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
					if(0 == strcmp(eth_gateway,"0.0.0.0"))
					{
						ovfs_soft::ovfs_netcard_config eth_conf;
						 if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf))
						 {
							snprintf(eth_gateway,sizeof(eth_gateway),"%s",eth_conf.gate_way.ipv4);
						 }
					}

					snprintf(system_cmd, sizeof(system_cmd), "route del default dev ppp0; rm /tmp/ppp0_ok");
					Common_System(system_cmd);
					snprintf(system_cmd, sizeof(system_cmd), "route add default dev ppp0 metric 1 && touch /tmp/ppp0_ok");
					Common_System(system_cmd);
					snprintf(system_cmd, sizeof(system_cmd), "route del default dev eth0");
					Common_System(system_cmd);
					snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s dev eth0 metric 0",eth_gateway);
					Common_System(system_cmd);
					Common_System("cp /tmp/resolv.conf /etc/resolv.conf");
				}
			}
			snprintf(strLastifname,sizeof(strLastifname),"eth0");
		}
		Common_Sleep(2, 0);
		cnt ++;
	}
	return 0;
}
#endif

static S32 NetTaskThread(Common_Thread_T hThreadHandle,OVFS_VOID *param)
{
	prctl(PR_SET_NAME, __func__);
	if(NULL == param)
	{
		return -1;
	}

	ovfs_network_config_mgr *net_mgr = (ovfs_network_config_mgr *)param;

	while(1)
	{
		net_mgr->sync_network();
	}
}

static int s_route_eventSubId[16] = {0};

static S32 CheckDhcpThread(Common_Thread_T hThreadHandle,OVFS_VOID *param)
{
	prctl(PR_SET_NAME, __func__);
	if(NULL == param)
	{
		return -1;
	}

	ovfs_network_config_mgr *net_mgr = (ovfs_network_config_mgr *)param;

	int n4GInited = 0, nWifiInited = 0;
	int nAliwifiSdkInit = 0;
	int nMetricSleepCount = 0;
	int nAliwifiSdkSleepCount = 0;
	int nP2PVersion = 0;

	while(access("/tmp/updating",F_OK))
	{
		net_mgr->check_dhcp_status();

		if(access("/tmp/lte",F_OK) == 0)
		{
			if(n4GInited == 0)
			{
				net_mgr->check_4G_status();

				n4GInited = 1;
			}
		}

		if(ovfs_utility_is_net_dev_exist("wlan0") == OVFS_TRUE)
		{
			if((nWifiInited == 0))
			{
				net_mgr->check_wifi_status();

				nWifiInited = 1;
			}

			if(nAliwifiSdkInit == 0)
			{
#ifdef PLATFORM_JZT30
				nAliwifiSdkSleepCount++;

				nP2PVersion = GetP2pVersion();

				if(nAliwifiSdkSleepCount <= 30)
				{
					if(nP2PVersion == 1)
					{
						LoadExternalAliwifiLibs();

						if(s_aliwififuncCb.init)
						{
							LOGD("call libaliwifi_sdk_init\n");
							s_aliwififuncCb.init();
						}
						if(s_aliwififuncCb.setCallback)
						{
							LOGD("call libaliwifi_sdk_set_callback\n");
							s_aliwififuncCb.setCallback(AliwifiGetCallBack,AliwifiSetCallBack);
						}

						nAliwifiSdkSleepCount = 0;
						nAliwifiSdkInit = 1;
					}
				}
				else
				{
					if(nP2PVersion == -1 || nP2PVersion == 1)
					{
						LoadExternalAliwifiLibs();

						if(s_aliwififuncCb.init)
						{
							LOGD("call libaliwifi_sdk_init\n");
							s_aliwififuncCb.init();
						}
						if(s_aliwififuncCb.setCallback)
						{
							LOGD("call libaliwifi_sdk_set_callback\n");
							s_aliwififuncCb.setCallback(AliwifiGetCallBack,AliwifiSetCallBack);
						}

						nAliwifiSdkSleepCount = 0;
						nAliwifiSdkInit = 1;
					}
				}
#else
				LoadExternalAliwifiLibs();

#ifdef AWSIOT

                if(s_aliwififuncCb.callback)
				{
					LOGD("call libaliwifi_sdk_set_callback\n");
					s_aliwififuncCb.callback(AliwifiCallBack);
				}

                if(s_aliwififuncCb.streamCallback)
				{
					LOGD("call libaliwifi_sdk_set_callback\n");
                    ALIWIFISDK_STREAMCONTROL sc;
                    sc.fxnOpen = StreamOpen;
                    sc.fxnClose = StreamClose;
                    sc.fxnRead = StreamRead;
                    sc.fxnRelease = StreamRelease;
					s_aliwififuncCb.streamCallback(&sc);
				}
#else

				if(s_aliwififuncCb.setCallback)
				{
					LOGD("call libaliwifi_sdk_set_callback\n");
					s_aliwififuncCb.setCallback(AliwifiGetCallBack,AliwifiSetCallBack);
				}
#endif
                if(s_aliwififuncCb.init)
                {
                    LOGD("call libaliwifi_sdk_init\n");
                    s_aliwififuncCb.init();
                }

				nAliwifiSdkInit = 1;
#endif
			}
		}

		if((g_network_has_init == 1) && (nAliwifiSdkInit == 1 || n4GInited == 1))
		{
			net_mgr->wifi_4g_route_monitor(nAliwifiSdkInit, n4GInited);
		}

		if(g_metric != g_metric_subsribe)
		{
            LOGW("send route\n");
            g_need_send_status = 1;
			g_metric_subsribe = g_metric;
			/*Common_cJSON_T* outparam = Common_cJSON_CreateObject();
			if(outparam)
			{
				Common_cJSON_AddNumberToObject(outparam,"route_metric",g_metric);

				int i = 0;
				for(i = 0; i < 16; i ++)
				{
					if(s_route_eventSubId[i] <= 0)
					{
						continue;
					}

					Common_cJSON_T* retParam = NULL;
					Module_SendEvent(NetWork_RestComm_GetModuleHdl(),s_route_eventSubId[i],(cJSON_Struct*)outparam,(cJSON_Struct**)(&retParam),2000);
					if(retParam)
					{
						Common_cJSON_Delete(retParam);
						retParam = NULL;
					}

					char* out = NULL;
					LOGI("send route Event. g_metric=%d result=%s\n",g_metric, out = Common_cJSON_Print(outparam,NULL));
					if(out)
						Common_Free(out,__FUNCTION__,__LINE__);

					g_metric_old = g_metric;
				}

				Common_cJSON_Delete(outparam);
				outparam = NULL;
			}*/
		}

		//if(g_metric != g_metric_old)
		{
			nMetricSleepCount++;
			if(nMetricSleepCount >= 2)
			{
				Common_cJSON_T *inParam = Common_cJSON_CreateObject();
				Common_cJSON_T* outparam = NULL;
				if(inParam)
				{
					Common_cJSON_T *header = Common_cJSON_CreateObject();
					Common_cJSON_AddStringToObject(header, "Method", "put");
					Common_cJSON_AddStringToObject(header, "Uri", "/Boardsys/Sys/SetRoute");
					Common_cJSON_AddItemToObject(inParam, "Header", header);
					Common_cJSON_T *inData = Common_cJSON_CreateObject();
					Common_cJSON_AddNumberToObject(inData,"route_metric",g_metric);
					Common_cJSON_AddItemToObject(inParam, "Data", inData);
					int ret = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(), (cJSON_Struct *)inParam, (cJSON_Struct **)(&outparam), 3000);
					if(ret == 0)
					{
						g_metric_old = g_metric;
						nMetricSleepCount = 0;
					}
					if(outparam)
					{
						Common_cJSON_Delete(outparam);
						outparam = NULL;
					}

					if(strlen(g_gunUsb) > 0)
					{
						Common_Json_SetAttrValueObj(header, "RemoteServerInfo");
						Common_Json_SetAttrValueStr(header, "RemoteServerInfo/IP", "10.0.0.2");
						Common_Json_SetAttrValueInt(header, "RemoteServerInfo/Port", 10009);
						Common_Json_SetAttrValueStr(header, "RemoteServerInfo/BindIF", g_gunUsb);
						ret = Module_CallFunctions_Remote((char *)"10.0.0.2", 10009, (cJSON_Struct *)inParam, (cJSON_Struct **)(&outparam), 3000);
						if(outparam)
						{
							Common_cJSON_Delete(outparam);
							outparam = NULL;
						}
					}

					Common_cJSON_Delete(inParam);
					inParam = NULL;
				}
			}
		}

        if(g_need_send_status)
        {
            char if_ipaddr[32] = {0};
            char if_netmask[32] = {0};
            char if_gateway[32] = {0};
            char if_name[32] = {0};
            net_mgr->get_default_router(if_name,sizeof(if_name));
            NetWorkTool_GetIPAddr((char *)if_name,if_ipaddr);
            NetWorkTool_GetMaskAddr((char *)if_name,if_netmask);
            NetWorkTool_GetGateway((char *)if_name,if_gateway);
            cJSON_Struct *data = Common_cJSON_CreateObject();
            Common_Json_SetAttrValueInt(data, "NetType", g_metric+1);
            Common_Json_SetAttrValueStr(data, "DefaultRoute", if_name);
            Common_Json_SetAttrValueStr(data, "Ip", if_ipaddr);
            Common_Json_SetAttrValueStr(data, "NetMask", if_netmask);
            Common_Json_SetAttrValueStr(data, "Gateway", if_gateway);
            if(g_metric != 2)
            {
                if(g_metric == 1)
                {
                    //Common_Json_SetAttrValueInt(data, "Dhcp", 1);
                    cJSON_Struct *wifidata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                    NetWork_WifiConfig(0, "/network/netattr/wificonfig", NULL, (Common_cJSON_T*)wifidata);
                    char *ssid = NULL;
                    char *psk = NULL;
                    Common_Json_GetAttrValueStr(wifidata, "Ssid", &ssid);
                    Common_Json_SetAttrValueStr(data, "SSID", ssid);
                    Common_Json_GetAttrValueStr(wifidata, "Psk", &psk);
                    Common_Json_SetAttrValueStr(data, "Password", psk);
                    Common_Json_Delete(wifidata);
				    wifidata = NULL;
                }
                else
                {
					/*
					ovfs_soft::ovfs_netcard_config eth_conf;
					if(ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf) == OVFS_SUCCESS)
					{
						Common_Json_SetAttrValueInt(data, "Dhcp", eth_conf.dhcp);
					}
					*/
                }
            }
            else
            {
                cJSON_Struct *ltedata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                NetWork_Get_LTECfg_Json(ltedata);
                char *ccid = NULL;
                Common_Json_GetAttrValueStr(ltedata, "Info/CCID", &ccid);
                Common_Json_SetAttrValueStr(data, "CCID", ccid);
                Common_Json_Delete(ltedata);
				ltedata = NULL;
            }

            int nEthNetIn = !ovfs_utility_is_net_dev_out("eth0");
            Common_Json_SetAttrValueInt(data, "EthConnectStatus", nEthNetIn);

            if(sendDeviceStatus(data) == 0)
            {
                g_need_send_status = 0;
            }

            Common_Json_Delete(data);
            data = NULL;
        }
		Common_InterSleep_Sleep(m_sleep_ops, 1, 0);
	}

	return 0;
}

//dhcp模式下启动,第一次需要设置成0.0.0.0,否则某些路由器会改变ip
static int g_iSwitchCount = 0;
#if 0
S32 route_subscribeList_fxn(ModuleHandle_T hModuleHandle,S32 nType /* 0-subscribe,1-unsubscribe,2-QueryEvent*/,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData)
{
	int i = 0, ret = -1;
    LOGW("NetWork_EventSubscribe URI:%s nType:%d\n",szSubscribeUri,nType);

    if (0 == nType)
    {
		for(i = 0; i < 16; i ++)
		{
	        if(s_route_eventSubId[i] <= 0)
	        {
				s_route_eventSubId[i] = nRecvID;
				break;
	        }
		}

        if(pQueryEventInfo && i < 16)
        {
			Common_cJSON_T* outParam = Common_cJSON_CreateObject();
			if(outParam)
			{
				Common_cJSON_AddNumberToObject(outParam,"route_metric",g_metric);
	            *pQueryEventInfo = (cJSON_Struct *)outParam;
				ret = 0;
            }
        }
    }
    else if (1 == nType)
    {
		ret = 0;
		for(i = 0; i < 16; i ++)
		{
	        if(s_route_eventSubId[i] == nRecvID)
	        {
				s_route_eventSubId[i] = 0;
	        }
		}
    }
    else if (2 == nType)
    {
		int nIdx = -1;
		for(i = 0; i < 16; i ++)
		{
	        if(s_route_eventSubId[i] == nRecvID)
	        {
				nIdx = i;
				break;;
	        }
		}
        if(nIdx >= 0)
        {
        	if(pQueryEventInfo)
        	{
        		Common_cJSON_T* outParam = Common_cJSON_CreateObject();
				if(outParam)
				{
					Common_cJSON_AddNumberToObject(outParam,"route_metric",g_metric);
	        		*pQueryEventInfo = outParam;
					ret = 0;
				}
        	}
        }
        else
        {
        	LOGE("Can't found id. Uri:%s\n",szSubscribeUri);
        }
    }
	return ret;
}
#endif
static int GunUsbStatusReport(ModuleHandle_T hAccessHandle, int nSubscribeID,
                                 cJSON_Struct *pEventInfo,
                                 cJSON_Struct **pOutParams, void *pUserData)
{
    char *str = NULL;
    if(Common_Json_GetAttrValueStr(pEventInfo, "GunUsb", &str))
    {
        LOGW("GunUsb:%s\n",str);
        snprintf(g_gunUsb,sizeof(g_gunUsb),"%s",str);
    }

    return 0;
}

ovfs_network_config_mgr::ovfs_network_config_mgr()
{
	OVFS_CLR_ARG(m_eth_cfg);
	m_eth_num = 0;
	OVFS_CLR_ARG(m_dns_cfg);
	OVFS_CLR_ARG(m_setting_task_list);
    OVFS_CLR_ARG(m_net_speedInfo);
    OVFS_CLR_ARG(m_netcard_speedInfo);

	m_network_ability = ovfs_cfgm_get_network_cap();
	OVFS_ASSERT(NULL != m_network_ability);

    OVFS_CLR_ARG(m_sleep_ops);
    Common_InterSleep_Create(&m_sleep_ops);

	OVFS_CLR_ARG(m_lock);
	OVFS_CLR_ARG(m_lock_netspeed);
    Common_Lock_Create(&m_lock,NULL);
    Common_Lock_Create(&m_lock_netspeed,NULL);

	//创建网卡设置任务线程
	m_pSetCFGthread = NULL;
	Common_Thread_Create(&m_pSetCFGthread,"NetTaskThread",1024*128,COMMON_THREAD_CREATEFLAG_NORMAL,NetTaskThread,(void *)this);
	//创建检测dhcp 线程
	m_pCheckDhcpthread = NULL;
	Common_Thread_Create(&m_pCheckDhcpthread,"CheckDhcpThread",1024*128,COMMON_THREAD_CREATEFLAG_NORMAL,CheckDhcpThread,(void *)this);
	//创建网络变化监测线程
	m_pMonitorthread = NULL;
	//Common_Thread_Create(&m_pMonitorthread,"NetMonitorThread",1024*128,COMMON_THREAD_CREATEFLAG_NORMAL,NetMonitorThread,(void *)this);
    init_net_config();
	LOGD("ovfs_network_config_mgr construct\n");

    //Module_RegisterSubscribe(NetWork_RestComm_GetModuleHdl(),(S8*)"/Network/SubscribRoute",route_subscribeList_fxn,NULL);
	//Module_SubscribeEvent(NetWork_RestComm_GetModuleHdl(), (S8*)"/Core/Subscribe/GunUsbStatus", GunUsbStatusReport, NULL);
}


ovfs_network_config_mgr::~ovfs_network_config_mgr()
{

}

OVFS_VOID ovfs_network_config_mgr::init_net_config()
{
	g_network_has_init = 0;
#if 0
	for(U32 i = 0; i < m_network_ability->bond_num; ++i)
	{
		ovfs_bounding_eth_config bond_cfg;
		OVFS_CLR_ARG(bond_cfg);
		if(OVFS_SUCCESS == ovfs_cfgm_get_bond_eth_cfg(m_network_ability->bond_status[i].bound_name, &bond_cfg))
			set_bond_eth_cfg(&bond_cfg);
	}
#endif

	if(is4gDevice() == 1)
	{
		Common_System("touch /tmp/is4gDevice");
	}

	if(access("/tmp/is4gDevice",F_OK) == 0)
	{
		Common_System("route del default dev eth0");
	}

	Read_NetworkCustom(&s_nWiredExtranet);

	ovfs_soft::ovfs_local_net_dns dns;
	OVFS_CLR_ARG(dns);
	if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_dns_cfg(&dns))
			set_dns_cfg(&dns);

	for(U32 i = 0; i < m_network_ability->total_eth_num; ++i)
	{
		ovfs_soft::ovfs_netcard_config eth_cfg;
		OVFS_CLR_ARG(eth_cfg);
		if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg(m_network_ability->eth_status[i].if_name, &eth_cfg))
			set_local_netcard_cfg(&eth_cfg);
	}

	S8 router[64];
	OVFS_CLR_ARG(router);
	if(OVFS_SUCCESS == get_default_if(router, sizeof(router)))
	{
		//LOGD("\n\n\n\n\n\n\n\n\n\ndefault router %s \n\n\n\n\n", router);
		set_default_if(router);
	}
}

OVFS_ERR ovfs_network_config_mgr::wifi_4g_route_monitor(int nAliwifiSdkInit, int n4GInited)
{
    int nEthNetIn = 0;
    static int detectCount = 0;
    static int nEthNetIn_last = -1;
    char system_cmd[256] = {0};
	char eth_ipaddr[32] = {0};
    char eth_netmask[32] = {0};
    char eth_gateway[32] = {0};
    char eth_subnet[32] = {0};

    nEthNetIn = !ovfs_utility_is_net_dev_out("eth0");
    LOGW("nEthNetIn:[%d] last[%d] detectCount[%d]\n",nEthNetIn,nEthNetIn_last,detectCount);
	if(nEthNetIn == nEthNetIn_last)
	{
		detectCount = 0;
		return OVFS_ERR_NETWORK_BASE;
	}
    else if(nEthNetIn != nEthNetIn_last && detectCount <= 1)
    {
        detectCount++;
        return OVFS_ERR_NETWORK_BASE;
    }

	nEthNetIn_last = nEthNetIn;
	detectCount = 0;

    if(nEthNetIn == 1)//eth0插上
    {
    	if(nAliwifiSdkInit == 1)
		{
	        {
#if 0
				ovfs_soft::ovfs_netcard_config eth_cfg;
				OVFS_CLR_ARG(eth_cfg);
				if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0", &eth_cfg))
					set_local_netcard_cfg(&eth_cfg);
#endif
				memset(eth_ipaddr, 0, sizeof(eth_ipaddr));
				memset(eth_netmask, 0, sizeof(eth_netmask));
				memset(eth_gateway, 0, sizeof(eth_gateway));
				memset(eth_subnet, 0, sizeof(eth_subnet));

	            NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
	            NetWorkTool_GetMaskAddr((char *)"eth0",eth_netmask);
	            NetWorkTool_GetGateway((char *)"eth0",eth_gateway);

	            ovfs_soft::ovfs_netcard_config eth_conf;
	            if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf))
	            {
	            	if(!strlen(eth_ipaddr) || 0 == strcmp(eth_ipaddr,"0.0.0.0"))
	            	{
	            		snprintf(eth_ipaddr,sizeof(eth_ipaddr),"%s",eth_conf.ip.ipv4);
	            	}
	            	if(!strlen(eth_netmask) || 0 == strcmp(eth_netmask,"0.0.0.0"))
	            	{
	            		snprintf(eth_netmask,sizeof(eth_netmask),"%s",eth_conf.netmask.ipv4);
	            	}
	            	if(!strlen(eth_gateway) || 0 == strcmp(eth_gateway,"0.0.0.0"))
	            	{
	            		snprintf(eth_gateway,sizeof(eth_gateway),"%s",eth_conf.gate_way.ipv4);
	            	}
	            }

	            //当eth0 还没有配置地址时
	            if(!strlen(eth_ipaddr) || 0 == strcmp(eth_ipaddr,"0.0.0.0") || !strlen(eth_netmask) || 0 == strcmp(eth_netmask,"0.0.0.0") || !strlen(eth_gateway) || 0 == strcmp(eth_gateway,"0.0.0.0"))
	            {
	            	strcpy(eth_ipaddr, "192.168.1.188");
	                strcpy(eth_netmask, "255.255.0.0");
	                strcpy(eth_gateway, "192.168.1.1");
	            }

	            NetWorkTool_GetSubNet(eth_subnet, eth_ipaddr, eth_netmask);

	            snprintf(system_cmd, sizeof(system_cmd), "ifconfig eth0 %s netmask %s", eth_ipaddr, eth_netmask);
	            Common_System(system_cmd);

	            snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev eth0",eth_subnet,eth_netmask);
	            Common_System(system_cmd);
	            snprintf(system_cmd, sizeof(system_cmd), "route add -net %s netmask %s dev eth0 metric %d",eth_subnet,eth_netmask,g_metric);
	            Common_System(system_cmd);
	            snprintf(system_cmd, sizeof(system_cmd), "route del default dev eth0");
	            Common_System(system_cmd);
	            snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s dev eth0 metric %d",eth_gateway,g_metric);
	            Common_System(system_cmd);
			}

			{
				cJSON_Struct *data = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	            Common_Json_SetAttrValueInt(data, "NetType", 1);
	            Common_Json_SetAttrValueStr(data, "DefaultRoute", "eth0");
	            LOGW("s_aliwififuncCb SetRoute=[1]\n");
	            NetWork_WifiConfig(1, "/network/netattr/SetRoute", (Common_cJSON_T*)data, NULL);
	            Common_Json_Delete(data);
	            data = NULL;
			}
    	}

		if(n4GInited == 1)
		{
			memset(eth_ipaddr, 0, sizeof(eth_ipaddr));
			memset(eth_netmask, 0, sizeof(eth_netmask));
			memset(eth_gateway, 0, sizeof(eth_gateway));
			memset(eth_subnet, 0, sizeof(eth_subnet));

			int nRet = NetWorkTool_GetDefaultRouteV2((char *)"eth0", eth_gateway);
			if((nRet == -1) || (strlen(eth_gateway) == 0))
			{
				NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
				NetWorkTool_GetMaskAddr((char *)"eth0",eth_netmask);
				NetWorkTool_GetGateway((char *)"eth0",eth_gateway);

				ovfs_soft::ovfs_netcard_config eth_conf;
	            if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf))
	            {
					if(!strlen(eth_ipaddr) || 0 == strcmp(eth_ipaddr,"0.0.0.0"))
	            	{
	            		snprintf(eth_ipaddr,sizeof(eth_ipaddr),"%s",eth_conf.ip.ipv4);
	            	}
	            	if(!strlen(eth_netmask) || 0 == strcmp(eth_netmask,"0.0.0.0"))
	            	{
	            		snprintf(eth_netmask,sizeof(eth_netmask),"%s",eth_conf.netmask.ipv4);
	            	}
					if(!strlen(eth_gateway) || 0 == strcmp(eth_gateway,"0.0.0.0"))
	            	{
	            		snprintf(eth_gateway,sizeof(eth_gateway),"%s",eth_conf.gate_way.ipv4);
	            	}
	            }
				if(!strlen(eth_ipaddr) || 0 == strcmp(eth_ipaddr,"0.0.0.0") || !strlen(eth_netmask) || 0 == strcmp(eth_netmask,"0.0.0.0") || !strlen(eth_gateway) || 0 == strcmp(eth_gateway,"0.0.0.0"))
	            {
	            	strcpy(eth_ipaddr, "192.168.1.188");
	                strcpy(eth_netmask, "255.255.0.0");
					strcpy(eth_gateway, "192.168.1.1");
	            }

				NetWorkTool_GetSubNet(eth_subnet, eth_ipaddr, eth_netmask);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev eth0",eth_subnet,eth_netmask);
				Common_System(system_cmd);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route add -net %s netmask %s dev eth0",eth_subnet,eth_netmask);
				Common_System(system_cmd);

				if(access("/tmp/is4gDevice",F_OK) != 0 || s_nWiredExtranet == 1)
				{
					memset(system_cmd, 0, sizeof(system_cmd));
					snprintf(system_cmd, sizeof(system_cmd), "%s", "route del default dev eth0");
					Common_System(system_cmd);

					memset(system_cmd, 0, sizeof(system_cmd));
					snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s dev eth0 metric 1",eth_gateway);
					Common_System(system_cmd);
				}

				ovfs_soft::ovfs_local_net_dns dns_tmp;
				memset(&dns_tmp, 0, sizeof(dns_tmp));
				ovfs_soft::ovfs_cfgm_get_dns_cfg(&dns_tmp);
				set_dns_cfg(&dns_tmp);
			}

			{
				set_4g_route_change(1);
				LOGW("set_4g_route_change SetRoute=[1]\n");
			}
		}

		g_metric = 0;
    }
    else
    {
    	if(nAliwifiSdkInit == 1)
    	{
        	{
				snprintf(system_cmd, sizeof(system_cmd), "%s", "ifconfig eth0 0.0.0.0");
	        	Common_System(system_cmd);
			}

			{
				cJSON_Struct *data = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	            Common_Json_SetAttrValueInt(data, "NetType", 2);
	            Common_Json_SetAttrValueStr(data, "DefaultRoute", "wlan0");
				LOGW("s_aliwififuncCb SetRoute=[2]\n");
	            NetWork_WifiConfig(1, "/network/netattr/SetRoute", (Common_cJSON_T*)data, NULL);
	            Common_Json_Delete(data);
	            data = NULL;
			}
		}

		if(n4GInited == 1)
		{
			memset(eth_ipaddr, 0, sizeof(eth_ipaddr));
			memset(eth_netmask, 0, sizeof(eth_netmask));
			memset(eth_gateway, 0, sizeof(eth_gateway));
			memset(eth_subnet, 0, sizeof(eth_subnet));

			int nRet = NetWorkTool_GetDefaultRouteV2((char *)"eth0", eth_gateway);
			if((nRet == 0) || (strlen(eth_gateway)))
			{
				NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
				NetWorkTool_GetMaskAddr((char *)"eth0",eth_netmask);

				ovfs_soft::ovfs_netcard_config eth_conf;
	            if(OVFS_SUCCESS == ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf))
	            {
					if(!strlen(eth_ipaddr) || 0 == strcmp(eth_ipaddr,"0.0.0.0"))
	            	{
	            		snprintf(eth_ipaddr,sizeof(eth_ipaddr),"%s",eth_conf.ip.ipv4);
	            	}
	            	if(!strlen(eth_netmask) || 0 == strcmp(eth_netmask,"0.0.0.0"))
	            	{
	            		snprintf(eth_netmask,sizeof(eth_netmask),"%s",eth_conf.netmask.ipv4);
	            	}
	            }
				if(!strlen(eth_ipaddr) || 0 == strcmp(eth_ipaddr,"0.0.0.0") || !strlen(eth_netmask) || 0 == strcmp(eth_netmask,"0.0.0.0"))
	            {
	            	strcpy(eth_ipaddr, "192.168.1.188");
	                strcpy(eth_netmask, "255.255.0.0");
	            }

				NetWorkTool_GetSubNet(eth_subnet, eth_ipaddr, eth_netmask);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "route del -net %s netmask %s dev eth0",eth_subnet,eth_netmask);
				Common_System(system_cmd);

				memset(system_cmd, 0, sizeof(system_cmd));
				snprintf(system_cmd, sizeof(system_cmd), "%s", "route del default dev eth0");
				Common_System(system_cmd);
			}

			{
				set_4g_route_change(2);
				LOGW("set_4g_route_change SetRoute=[1]\n");
			}
		}
    }

	return OVFS_SUCCESS;
}

OVFS_VOID ovfs_network_config_mgr::check_4G_status()
{
	ovfs_network_4g_init();

	//创建4G状态监测线程
	//m_p4Gthread = NULL;
	//Common_Thread_Create(&m_p4Gthread,"LteMonitorThread",1024*128,COMMON_THREAD_CREATEFLAG_NORMAL,WiFiMonitorThread,(void *)this);

	return ;
}

OVFS_VOID ovfs_network_config_mgr::check_wifi_status()
{
	Common_cJSON_T* networkRoot = NULL;
	NetWork_Rest_Get_NetworkItem_OfRootUri(&networkRoot);

	NetWork_Rest_NetAttr_initwifi(networkRoot);

	//创建WiFi状态监测线程
	//m_pWiFithread = NULL;
	//Common_Thread_Create(&m_pWiFithread,"WiFiMonitorThread",1024*128,COMMON_THREAD_CREATEFLAG_NORMAL,WiFiMonitorThread,(void *)this);

	return ;
}

OVFS_VOID ovfs_network_config_mgr::check_dhcp_status()
{
	int ip_conflict = 0,network_blocked = 0;
	int failed_max_num = 5;
	int nEthNetOut = 0;
	ovfs_soft::ovfs_netcard_config eth_conf;

	nEthNetOut = ovfs_utility_is_net_dev_out("eth0");

	if(nEthNetOut)
	{
		s_check_dhcp_failed_cnt = 0;
		s_ip_conflict_delay_cnt = 0;
		return;
	}

	if(ovfs_soft::ovfs_cfgm_get_local_eth_cfg("eth0",&eth_conf) != OVFS_SUCCESS)
	{
		s_check_dhcp_failed_cnt = 0;
		s_ip_conflict_delay_cnt = 0;
		return;
	}

	if(!eth_conf.dhcp || strlen(eth_conf.gate_way.ipv4) == 0 || stricmp(eth_conf.gate_way.ipv4, "0.0.0.0") == 0)
	{
		s_check_dhcp_failed_cnt = 0;
		s_ip_conflict_delay_cnt = 0;
		return;
	}

	if(NetWorkTool_CheckConnect((char *)"eth0",eth_conf.gate_way.ipv4))
	{
		s_check_dhcp_failed_cnt = 0;
	}
	else
	{
		s_check_dhcp_failed_cnt++;
	}

	if(s_check_dhcp_failed_cnt >= failed_max_num)
	{
		network_blocked = 1;

		s_check_dhcp_failed_cnt = 0;
	}

	s_ip_conflict_delay_cnt++;

	if(s_ip_conflict_delay_cnt >= 10)
	{
		ip_conflict = ovfs_soft::ovfs_arptool_check_local_IP_conflict((char *)"eth0");

		s_ip_conflict_delay_cnt = 0;
	}

	LOGD("network_blocked=[%d],ip_conflict=[%d]\n",network_blocked,ip_conflict);
#if 0
	if(network_blocked)
	{
		Common_GetSystemCount(&s_reset_netcard_cur_time, NULL);
		if((s_reset_netcard_lst_time == 0) || (s_reset_netcard_cur_time >= s_reset_netcard_lst_time + 3600))
		{
			s_reset_netcard_lst_time = s_reset_netcard_cur_time;

			Common_Lock(m_lock);

			network_setting_task_node *node = m_setting_task_list.head;
			//dhcp节点因为是异步添加的 需要看一下链表里面是否有相同的节点 并且不是dhcp的
			//避免覆盖掉用户非dhcp的节点
			OVFS_BOOL needAdd = OVFS_TRUE;
			while(NULL != node)
			{
				if(node->task_type == NETWORK_RESET_NETCARD && strcmp(node->net_cfg.card_cfg.if_name, eth_conf.if_name)== 0)
				{
					needAdd = OVFS_FALSE;
					break;
				}

				node = node->next;
			}

			if(needAdd)
			{
				network_setting_task_node *new_node = malloc_net_set_task(NETWORK_RESET_NETCARD, (const S8 *)&eth_conf, sizeof(ovfs_soft::ovfs_netcard_config));
				if(new_node)
				{
					if(OVFS_SUCCESS != add_new_task(&m_setting_task_list, new_node))
					{
						Common_Free(new_node,__FUNCTION__,__LINE__);
						new_node = NULL;
					}
				}
			}

			Common_UnLock(m_lock);
		}
	}
#endif
	if(network_blocked || ip_conflict)
	{
		Common_Lock(m_lock);

		network_setting_task_node *node = m_setting_task_list.head;
		//dhcp节点因为是异步添加的 需要看一下链表里面是否有相同的节点 并且不是dhcp的
		//避免覆盖掉用户非dhcp的节点
		OVFS_BOOL needAdd = OVFS_TRUE;
		while(NULL != node)
		{
			if(node->task_type == NETWORK_SET_NET_CAED_CFG && strcmp(node->net_cfg.card_cfg.if_name, eth_conf.if_name)== 0)
			{
				if(!node->net_cfg.card_cfg.dhcp)
				{
					needAdd = OVFS_FALSE;
					break;
				}
			}

			node = node->next;
		}

		if(needAdd)
		{
			network_setting_task_node *new_node = malloc_net_set_task(NETWORK_SET_NET_CAED_CFG, (const S8 *)&eth_conf, sizeof(ovfs_soft::ovfs_netcard_config));
			if(new_node != NULL)
			{
				if(OVFS_SUCCESS != add_new_task(&m_setting_task_list, new_node))
				{
					Common_Free(new_node,__FUNCTION__,__LINE__);
					new_node = NULL;
				}
			}
		}

		Common_UnLock(m_lock);

	}

	return ;
}

OVFS_VOID ovfs_network_config_mgr::sync_network()
{
    Common_Lock(m_lock);
    //取出网络任务中m_setting_task_list循环链表中的头结点 并且在链表中删除它
    network_setting_task_node *node = get_one_net_setting_task();
    Common_UnLock(m_lock);
    if(node == NULL)
    {
        //如果m_setting_task_list任务为空 就休眠1秒或者被唤醒
        Common_InterSleep_Sleep(m_sleep_ops, 1, 0);

        return ;
    }

    //设置网络任务信息到系统
    OVFS_ERR ret = issue_setting_task(node);

    Common_Lock(m_lock);
    //判断设置网络信息是否成功，
    //成功:释放该节点
    //失败:如果当前节点在链表里面是否有相同的类型和网卡名称 有就释放 没有就再添加进去
    release_one_net_setting_task(node, ret==OVFS_SUCCESS?OVFS_TRUE:OVFS_FALSE);
    Common_UnLock(m_lock);
    if(ret != OVFS_SUCCESS)
    {
        Common_InterSleep_Sleep(m_sleep_ops, 1, 0);
    }
    return ;
}

OVFS_ERR ovfs_network_config_mgr::issue_setting_task(network_setting_task_node *node)
{
	if(node == NULL)
		return OVFS_ERR_NETWORK_BASE;

	switch(node->task_type)
	{
		case NETWORK_SET_NET_CAED_CFG:
			LOGW("set netcard cfg\n");
			return update_netcard_cfg(&node->net_cfg.card_cfg);

		case NETWORK_SET_DNS_CFG:
			LOGW("set DNS\n");
			return update_dns_cfg(&node->net_cfg.dns_cfg);
		case NETWORK_RESET_NETCARD:
			LOGW("reset netcard\n");
			return reset_netcard(&node->net_cfg.card_cfg);
		default:
			OVFS_ASSERT(0);
			break;
	}

	return OVFS_ERR_NETWORK_BASE;

}

OVFS_ERR ovfs_network_config_mgr::get_local_netcard_cfg(const S8* if_name, ovfs_soft::ovfs_netcard_config *netCfg)
{
    int ret = 0,family = -1;
    char ip[OVFS_IPV6_STRING_LEN + 1]={0},netmask[OVFS_IPV6_STRING_LEN + 1]={0},gateway[OVFS_IPV6_STRING_LEN + 1]={0};
    char mac[OVFS_MAC_STRING_LEN]={0};
    LOGD("if_name=%s,dhcp=%d\n",if_name,netCfg->dhcp);

	if (strstr(netCfg->if_name, "eth") != NULL)
	{
	    if(OVFS_SUCCESS != ovfs_cfgm_get_local_eth_cfg(if_name, netCfg))
	        return OVFS_ERR_NETWORK_INVALID_PARA;
	}

    LOGD("if_name=%s,dhcp=%d\n",if_name,netCfg->dhcp);
	NetWorkTool_GetMacAddr((char *)if_name,mac);
	snprintf(netCfg->mac,sizeof(netCfg->mac),"%s",mac);
    if(netCfg->dhcp)
    {
		ret = network_get_ip_bydhcp(if_name, &netCfg->ip, &netCfg->netmask, &netCfg->gate_way);
    }
    if(OVFS_SUCCESS != ret)
    {
        ret = NetWorkTool_GetIfAddr((char *)if_name, ip, netmask,&family);
        ret |= NetWorkTool_GetGateway((char *)if_name,gateway);
        LOGD("ret=%d,family=%d,ip={%s},netmask={%s},gateway={%s}\n",ret,family,ip,netmask,gateway);
        if(ret != 0)
            return OVFS_ERR_NETWORK_BASE;
        if(family == OVFS_TRUE)
        {
            netCfg->ip.is_ipv4 = OVFS_TRUE;
			snprintf(netCfg->ip.ipv4,sizeof(netCfg->ip.ipv4),"%s",ip);
            netCfg->netmask.is_ipv4 = OVFS_TRUE;
			snprintf(netCfg->netmask.ipv4,sizeof(netCfg->netmask.ipv4),"%s",netmask);
            netCfg->gate_way.is_ipv4 = OVFS_TRUE;
			snprintf(netCfg->gate_way.ipv4,sizeof(netCfg->gate_way.ipv4),"%s",gateway);
        }
        if(family == OVFS_FALSE)
        {
            netCfg->ip.is_ipv4 = OVFS_FALSE;
			snprintf(netCfg->ip.ipv6,sizeof(netCfg->ip.ipv6),"%s",ip);
            netCfg->netmask.is_ipv4 = OVFS_FALSE;
			snprintf(netCfg->netmask.ipv6,sizeof(netCfg->netmask.ipv6),"%s",netmask);
            netCfg->gate_way.is_ipv4 = OVFS_FALSE;
			snprintf(netCfg->gate_way.ipv6,sizeof(netCfg->gate_way.ipv6),"%s",gateway);
        }
    }
    LOGD("ret=%d,mac=%s,ip={%s},netmask={%s},gateway={%s}\n",ret,mac,netCfg->ip.ipv4,netCfg->netmask.ipv4,netCfg->gate_way.ipv4);
    return OVFS_SUCCESS;
}

//设置本地网络(先写配置文件，再调用ifconfig命令配置到linux网络配置文件)
OVFS_ERR ovfs_network_config_mgr::set_local_netcard_cfg(const ovfs_soft::ovfs_netcard_config *netCfg)
{
	if (netCfg->dhcp != OVFS_TRUE)
	{
	    if (ovfs_utility_ipaddr_valid((ovfs_ipaddr_struct *)&netCfg->ip) == OVFS_FALSE ||
			ovfs_utility_netmask_valid((ovfs_ipaddr_struct *)&netCfg->netmask) == OVFS_FALSE)
	    {
		    return OVFS_ERR_NETWORK_INVALID_PARA;
		}
	    ovfs_utility_gateway_valid((ovfs_ipaddr_struct *)&netCfg->ip, (ovfs_ipaddr_struct *)&netCfg->gate_way, (ovfs_ipaddr_struct *)&netCfg->netmask);
	}

	//if(OVFS_TRUE != is_if_name_valid(netCfg->if_name))
	//	return OVFS_ERR_NETWORK_INVALID_PARA;

	//if(ovfs_cfgm_set_local_eth_cfg(netCfg, 0) != OVFS_SUCCESS)
	//	return OVFS_ERR_NETWORK_INVALID_PARA;

	//if(OVFS_TRUE == is_eth_in_bond(netCfg->if_name))
	//////	return OVFS_ERR_NETWORK_INVALID_PARA;

	S8 cmd_string[256];
	snprintf(cmd_string, sizeof(cmd_string), "ifconfig %s up ",  netCfg->if_name);
	Common_System(cmd_string);
	LOGW("cmd:%s\n",cmd_string);
	network_setting_task_node *node = malloc_net_set_task(NETWORK_SET_NET_CAED_CFG, (const S8*)netCfg, sizeof(ovfs_soft::ovfs_netcard_config));
	if(node == NULL)
	{
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}

	Common_Lock(m_lock);
	if(OVFS_SUCCESS != add_new_task(&m_setting_task_list, node))
	{
		Common_UnLock(m_lock);
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}
	Common_UnLock(m_lock);

	Common_InterSleep_WakeUp(m_sleep_ops);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::update_netcard_dhcp_cfg(ovfs_soft::ovfs_netcard_config *netCfg)
{
	LOGD("if_name=%s  netCfg->dhcp=%d\n",netCfg->if_name, netCfg->dhcp);
	if(netCfg->dhcp == OVFS_TRUE)
		set_local_netcard_cfg(netCfg);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::update_netcard_cfg(const ovfs_soft::ovfs_netcard_config *netCfg)
{
    LOGD("if_name=%s  netCfg->dhcp=%d\n",netCfg->if_name, netCfg->dhcp);
    if(netCfg->dhcp == OVFS_TRUE)
    {
        g_network_has_init = 1;
        if(ovfs_utility_is_net_dev_out("eth0"))
        {
            //没插网线，不进行dhcp, 反正成功不了
            LOGW("eth0 out! no dhcp!\n");
            return OVFS_SUCCESS;
        }

		ovfs_ipaddr_struct	ip;			//ip地址
		ovfs_ipaddr_struct	netmask;	//子网掩码
		ovfs_ipaddr_struct	gate_way;	//网关地址
		OVFS_CLR_ARG(ip);
		OVFS_CLR_ARG(netmask);
		OVFS_CLR_ARG(gate_way);

		ip.is_ipv4 = netCfg->ip.is_ipv4;
		netmask.is_ipv4 = netCfg->netmask.is_ipv4;
		gate_way.is_ipv4 = netCfg->gate_way.is_ipv4;
		memcpy(&ip, &(netCfg->ip), sizeof(ip));
		memcpy(&netmask, &(netCfg->netmask), sizeof(netmask));
		memcpy(&gate_way, &(netCfg->gate_way), sizeof(gate_way));

		//LOGE("netmask=%s gate_way=%s\n",netmask.ipv4, gate_way.ipv4);

		if(OVFS_SUCCESS != network_get_ip_bydhcp(netCfg->if_name, &ip, &netmask, &gate_way))
		{
			return OVFS_ERR_NETWORK_BASE;
		}

		if (strstr(netCfg->if_name, "eth") != NULL)
		{
			pthread_mutex_lock(&g_ip_set_lock);

	        ovfs_netcard_config local_eth;
			OVFS_CLR_ARG(local_eth);
			if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_get_local_eth_cfg(netCfg->if_name, &local_eth))
			{
				pthread_mutex_unlock(&g_ip_set_lock);
				return OVFS_ERR_NETWORK_BASE;
			}
			LOGE("local_eth.dhcp=%d   eth->dhcp=%d\n", local_eth.dhcp, netCfg->dhcp);
	        if(local_eth.dhcp == OVFS_TRUE && (0 != memcmp(&local_eth.ip,&ip,sizeof(ip)) || 0 != memcmp(&local_eth.netmask,&netmask,sizeof(netmask)) || 0 != memcmp(&local_eth.gate_way,&gate_way,sizeof(gate_way))))
	        {
				local_eth.ip = ip;
				local_eth.netmask = netmask;
				local_eth.gate_way = gate_way;
				if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_set_local_eth_cfg(&local_eth, 1))  //写配置文件，主要是要将网关地址写进去.
				{
					pthread_mutex_unlock(&g_ip_set_lock);
					return OVFS_ERR_NETWORK_BASE;
				}
		        NetWork_SaveCfg();
                g_need_send_status = 1;
	        }
			pthread_mutex_unlock(&g_ip_set_lock);
		}
    }
    else
    {
    	g_iSwitchCount = 1;

        char udhcpc_pid_file[64]={0};
        snprintf(udhcpc_pid_file, sizeof(udhcpc_pid_file), "/var/run/udhcpc_%s.pid",(char *)netCfg->if_name);
        network_check_udhcpc_pid(udhcpc_pid_file);

        char cur_gateway[OVFS_IPV6_STRING_LEN + 1]={0},sub_net[OVFS_IPV6_STRING_LEN + 1]={0},cmd[128]={0};
        LOGD("ip={%s},netmask={%s},gateway={%s}\n",netCfg->ip.ipv4,netCfg->netmask.ipv4,netCfg->gate_way.ipv4);
        int ret = NetWorkTool_SetDevInfo((char *)netCfg->if_name,
                    (char *)netCfg->mac,
            netCfg->ip.is_ipv4 == OVFS_TRUE?(char *)netCfg->ip.ipv4:(char *)netCfg->ip.ipv6,
            netCfg->netmask.is_ipv4 == OVFS_TRUE?(char *)netCfg->netmask.ipv4:(char *)netCfg->netmask.ipv6
            );
        ret = NetWorkTool_GetGateway((char *)netCfg->if_name,cur_gateway);
        if((ret == 0) && (0 == strcmp(cur_gateway,"0.0.0.0")))
        {
            NetWorkTool_GetSubNet(sub_net,
                netCfg->ip.is_ipv4 == OVFS_TRUE?(char *)netCfg->ip.ipv4:(char *)netCfg->ip.ipv6,
                netCfg->netmask.is_ipv4 == OVFS_TRUE?(char *)netCfg->netmask.ipv4:(char *)netCfg->netmask.ipv6);

            snprintf(cmd,sizeof(cmd),"route del -net %s netmask %s dev %s",sub_net,
				netCfg->netmask.is_ipv4 == OVFS_TRUE?(char *)netCfg->netmask.ipv4:(char *)netCfg->netmask.ipv6,
				netCfg->if_name);
            Common_System(cmd);

			snprintf(cmd,sizeof(cmd),"route add -net %s netmask %s dev %s metric %d",sub_net,
				netCfg->netmask.is_ipv4 == OVFS_TRUE?(char *)netCfg->netmask.ipv4:(char *)netCfg->netmask.ipv6,
				netCfg->if_name,1);

            LOGD("%s\n",cmd);
            Common_System(cmd);
        }

		if(access("/tmp/is4gDevice",F_OK) != 0 || s_nWiredExtranet == 1)
		{
			snprintf(cmd, sizeof(cmd), "route del default dev %s",netCfg->if_name);
			Common_System(cmd);

	        snprintf(cmd,sizeof(cmd),"route add default gw %s dev %s metric %d",
	        netCfg->gate_way.is_ipv4 == OVFS_TRUE?(char *)netCfg->gate_way.ipv4:(char *)netCfg->gate_way.ipv6,netCfg->if_name,1);
	        Common_System(cmd);
	        LOGD("%s\n",cmd);

			//Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
		}

		if (strstr(netCfg->if_name, "eth") != NULL)
		{
			pthread_mutex_lock(&g_ip_set_lock);
	        if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_set_local_eth_cfg(netCfg,0))
	        {
				pthread_mutex_unlock(&g_ip_set_lock);
	            return OVFS_ERR_NETWORK_INVALID_PARA;
	        }
			pthread_mutex_unlock(&g_ip_set_lock);
		}
		//NetWork_SaveCfg();
		g_need_send_status = 1;
    }

    LOGW("ipv6_support:[%d] dhcpv6:[%d]\n",netCfg->ipv6_support, netCfg->dhcpv6);
    if(netCfg->ipv6_support == OVFS_FALSE)
    {
    	g_network_has_init = 1;
        return OVFS_SUCCESS;
    }

    if(netCfg->dhcpv6 == 0)
    {
        char cmd[128]={0};
        snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.eth0.autoconf=0");
        Common_System(cmd);
        NetWorkTool_SetDevInfoV6((char *)netCfg->if_name,(char *)netCfg->ip.ipv6,netCfg->ipv6_prefixlen);
        NetWorkTool_SetDefaultRouteV6((char *)netCfg->if_name, (char *)netCfg->gate_way.ipv6);
    }
    else if(netCfg->dhcpv6 == 1)
    {
        char cmd[128]={0};
        snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.eth0.autoconf=0");
        Common_System(cmd);

        ovfs_ipaddr_struct	ip;			//ip地址
		int prefixlen = netCfg->ipv6_prefixlen;
		ovfs_ipaddr_struct	gate_way;	//网关地址
		OVFS_CLR_ARG(ip);
		OVFS_CLR_ARG(gate_way);

		memcpy(&ip, &(netCfg->ip), sizeof(ip));
		memcpy(&gate_way, &(netCfg->gate_way), sizeof(gate_way));

		//LOGE("netmask=%s gate_way=%s\n",netmask.ipv4, gate_way.ipv4);

		if(OVFS_SUCCESS != network_get_ipv6_bydhcp6(netCfg->if_name, &ip, &prefixlen, &gate_way))
		{
			return OVFS_ERR_NETWORK_BASE;
		}

        if (strstr(netCfg->if_name, "eth") != NULL)
		{
			pthread_mutex_lock(&g_ip_set_lock);

	        ovfs_netcard_config local_eth;
			OVFS_CLR_ARG(local_eth);
			if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_get_local_eth_cfg(netCfg->if_name, &local_eth))
			{
				pthread_mutex_unlock(&g_ip_set_lock);
				return OVFS_ERR_NETWORK_BASE;
			}
			LOGE("local_eth.dhcp=%d   eth->dhcp=%d\n", local_eth.dhcpv6, netCfg->dhcpv6);
	        if(local_eth.dhcpv6 == OVFS_TRUE && (0 != memcmp(&local_eth.ip,&ip,sizeof(ip)) || local_eth.ipv6_prefixlen != prefixlen || 0 != memcmp(&local_eth.gate_way,&gate_way,sizeof(gate_way))))
	        {
				local_eth.ip = ip;
				local_eth.ipv6_prefixlen = prefixlen;
				local_eth.gate_way = gate_way;
				if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_set_local_eth_cfg(&local_eth, 1))  //写配置文件，主要是要将网关地址写进去.
				{
					pthread_mutex_unlock(&g_ip_set_lock);
					return OVFS_ERR_NETWORK_BASE;
				}
		        NetWork_SaveCfg();
	        }
			pthread_mutex_unlock(&g_ip_set_lock);
		}

    }
    else if(netCfg->dhcpv6 == 2)
    {
		if (strstr(netCfg->if_name, "eth") != NULL)
		{
            char cmd[128]={0};
            ovfs_netcard_config local_eth;
            OVFS_CLR_ARG(local_eth);

			snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.eth0.disable_ipv6=1");
			Common_System(cmd);

			snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.eth0.autoconf=1");
			Common_System(cmd);

			snprintf(cmd, sizeof(cmd), "sysctl -w net.ipv6.conf.eth0.disable_ipv6=0");
			Common_System(cmd);

            char ipv6[64] = {0};         //ip地址
            int prefixlen = 64;
            char gatewayv6[64] = {0};   //网关地址

            LOGW("ifname:[%s] ipv6:[%s] prefixlen[%d]\n",netCfg->if_name,netCfg->ip.ipv6,netCfg->ipv6_prefixlen,netCfg->gate_way.ipv6);

            int trycount = 5;//出现异常: dhcp 很多次都失败..
    		while(--trycount)
    		{
    			if(OVFS_SUCCESS == NetWorkTool_GetDevInfoV6((char *)netCfg->if_name,(char *)ipv6,&prefixlen))
    				break;
    			Common_Sleep(1, 0);
    		}
            LOGW("trycount[%d] ifname:[%s] ipv6:[%s] prefixlen[%d]\n",trycount, netCfg->if_name,netCfg->ip.ipv6,netCfg->ipv6_prefixlen,netCfg->gate_way.ipv6);

			pthread_mutex_lock(&g_ip_set_lock);
            //NetWorkTool_GetDevInfoV6((char *)netCfg->if_name,(char *)netCfg->ip.ipv6,(int *)&netCfg->ipv6_prefixlen);
			if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_get_local_eth_cfg(netCfg->if_name, &local_eth))
			{
				pthread_mutex_unlock(&g_ip_set_lock);
				return OVFS_ERR_NETWORK_BASE;
			}

	        if((0 != memcmp(&local_eth.ip.ipv6,&ipv6,sizeof(ipv6)) ||
                 local_eth.ipv6_prefixlen != prefixlen))
            {
                snprintf(local_eth.ip.ipv6, sizeof(netCfg->ip.ipv6),"%s",ipv6);
                local_eth.ipv6_prefixlen = prefixlen;
                NetWorkTool_GetDefaultRouteV6((char *)netCfg->if_name,(char *)local_eth.gate_way.ipv6);
                LOGW("GetInfo ifname:[%s] ipv6:[%s] prefixlen[%d] gateway:[%s]\n",netCfg->if_name,local_eth.ip.ipv6,local_eth.ipv6_prefixlen,local_eth.gate_way.ipv6);
                if(OVFS_SUCCESS != ovfs_soft::ovfs_cfgm_set_local_eth_cfg(&local_eth, 1))  //写配置文件，主要是要将网关地址写进去.
        		{
        			pthread_mutex_unlock(&g_ip_set_lock);
        			return OVFS_ERR_NETWORK_BASE;
        		}
                NetWork_SaveCfg();
            }
            pthread_mutex_unlock(&g_ip_set_lock);
		}
    }

	g_network_has_init = 1;
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::reset_netcard(const ovfs_soft::ovfs_netcard_config *netCfg)
{
	char cmd[128] = {0};

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "ifconfig eth0 down");
	Common_System(cmd);

	Common_Sleep(1, 0);

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
	Common_System(cmd);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::network_check_udhcpc_pid(const S8 *udhcpc_pid_file)
{
    if(0 == access(udhcpc_pid_file, 0))
    {
        FILE* fp = fopen(udhcpc_pid_file, "r");
        if (NULL == fp)
        {
            // 不处理,很可能没有启动udhcpc.
            LOGW("Can not open udhcpc_pid\n");
            return OVFS_ERR_NETWORK_UNKNOWN;
        }
        else
        {
            char cmd[64]={0};
            char buf[32]={0};
            fseek(fp, 0, SEEK_SET);
            fgets(buf, sizeof(buf), fp);
            fclose(fp);
            int pid = atoi(buf);
            snprintf(cmd, sizeof(cmd), "kill -SIGUSR2 %d;kill -9 %d;", pid, pid);
            LOGW("kill udhcpc cmd: %s \n", cmd);
            Common_System(cmd);
            snprintf(cmd, sizeof(cmd), "rm %s", udhcpc_pid_file);
            LOGW("cmd: %s \n", cmd);
			remove(udhcpc_pid_file);
            //Common_System(cmd);
        }
    }
	else
	{
		return OVFS_ERR_NETWORK_BASE;
	}
	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::check_dhcpc_result(const S8 *if_name)
{
	char conflie[32];
	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc_ip_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return OVFS_ERR_NETWORK_OPERATE_FAIL;
	}

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc_resolv_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return OVFS_ERR_NETWORK_OPERATE_FAIL;
	}

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::check_dhcpc6_result(const S8 *if_name)
{
	char conflie[32];
	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc6_ip_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return OVFS_ERR_NETWORK_OPERATE_FAIL;
	}

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc6_resolv_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return OVFS_ERR_NETWORK_OPERATE_FAIL;
	}

	return OVFS_SUCCESS;
}

OVFS_ERR   ovfs_network_config_mgr::network_get_ip_bydhcp(const S8 *if_name, ovfs_ipaddr_struct *ip, ovfs_ipaddr_struct *netmask, ovfs_ipaddr_struct *gateway)
{
	S8 oldIp[32];
    S8 cmd[256];
    S8 udhcpc_pid[64];
    OVFS_CLR_ARG(cmd);
    OVFS_CLR_ARG(udhcpc_pid);
    snprintf(udhcpc_pid, sizeof(udhcpc_pid), "/var/run/udhcpc_%s.pid",if_name);
    snprintf(oldIp, sizeof(oldIp), "%s",ip->ipv4);

	S8 strip[64];
    S8 strnetmask[64];
    S8 strgateway[64];
    OVFS_CLR_ARG(strip);
    OVFS_CLR_ARG(strnetmask);
    OVFS_CLR_ARG(strgateway);
    if(OVFS_ERR_NETWORK_UNKNOWN != network_check_udhcpc_pid(udhcpc_pid))
    {
		if(0 == g_iSwitchCount)
		{
			snprintf(cmd, sizeof(cmd), "ifconfig %s 0.0.0.0",if_name);
	    	LOGW("cmd = %s \n", cmd);
			Common_System(cmd);
			g_iSwitchCount = 1;
		}
		else
		{
			g_iSwitchCount = 2;
		}
	    //snprintf(cmd, sizeof(cmd), "udhcpc -i %s -R -b -r %s -p %s -s udhcpc.script > /dev/null;",if_name,ip->ipv4,udhcpc_pid);

		if (strstr(if_name, "eth") != NULL)
		{
			if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
			{
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
            	Common_System(cmd);
			}

			snprintf(cmd, sizeof(cmd), "uip=%s usubnet=%s urouter=%s udhcpc -i %s -r %s -a -R -b -t 4 -T 1 -s /root/bin/udhcpc.script -p %s > /dev/null;",
				ip->ipv4,netmask->ipv4,gateway->ipv4,if_name,ip->ipv4,udhcpc_pid);
			LOGW("cmd = %s \n", cmd);
			if(OVFS_SUCCESS != Common_System(cmd))
		    {
		        LOGE("%s fail\n", cmd);

				if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
				{
					snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
	            	Common_System(cmd);
				}

				if(!ovfs_utility_is_net_dev_out("eth0"))
				{
					char eth_ipaddr[32] = {0};
	                char eth_gateway[32] = {0};
	                NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
	                NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
					if(strlen(eth_ipaddr) == 0 || stricmp(eth_ipaddr, "0.0.0.0") == 0)
					{
						snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s",if_name,oldIp,netmask->ipv4);
				    	LOGW("cmd = %s \n", cmd);
						Common_System(cmd);
					}

					if(strlen(eth_gateway) == 0 || stricmp(eth_gateway, "0.0.0.0") == 0)
					{
						if(access("/tmp/is4gDevice",F_OK) != 0 || s_nWiredExtranet == 1)
						{
							snprintf(cmd, sizeof(cmd), "route del default dev %s",if_name);
							LOGW("cmd = %s \n", cmd);
							Common_System(cmd);

							snprintf(cmd, sizeof(cmd), "route add default gw %s dev %s metric 1",gateway->ipv4,if_name);
					    	LOGW("cmd = %s \n", cmd);
							Common_System(cmd);

							//Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
						}
					}
				}
		        return OVFS_ERR_NETWORK_BASE;
		    }
		}
		else if (strstr(if_name, "wlan") != NULL)
		{
			snprintf(cmd, sizeof(cmd), "udhcpc -i %s -p %s -s udhcpc.script &",if_name,udhcpc_pid);
			LOGW("cmd = %s \n", cmd);
			if(Common_System(cmd) ==  0)
			{

			}
		}

		int trycount = 200;//出现异常: dhcp 很多次都失败..
		while(--trycount)
		{
			if(OVFS_SUCCESS == check_dhcpc_result(if_name))
				break;
			Common_Sleep(0, 100 * 1000);
		}
		LOGW("if_name=%s,trycount=%d\n", if_name,trycount);
		if(trycount == 0)
		{
			if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
			{
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
            	Common_System(cmd);
			}

			if(!ovfs_utility_is_net_dev_out("eth0"))
			{
				char eth_ipaddr[32] = {0};
                char eth_gateway[32] = {0};
                NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
                NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
				if(strlen(eth_ipaddr) == 0 || stricmp(eth_ipaddr, "0.0.0.0") == 0)
				{
					snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s",if_name,oldIp,netmask->ipv4);
			    	LOGW("cmd = %s \n", cmd);
					Common_System(cmd);
				}

				if(strlen(eth_gateway) == 0 || stricmp(eth_gateway, "0.0.0.0") == 0)
				{
					if(access("/tmp/is4gDevice",F_OK) != 0 || s_nWiredExtranet == 1)
					{
						snprintf(cmd, sizeof(cmd), "route del default dev %s",if_name);
						LOGW("cmd = %s \n", cmd);
						Common_System(cmd);

						snprintf(cmd, sizeof(cmd), "route add default gw %s dev %s metric 1",gateway->ipv4,if_name);
				    	LOGW("cmd = %s \n", cmd);
						Common_System(cmd);

						//Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
					}
				}
			}
			return OVFS_ERR_NETWORK_BASE;
		}

    }

	if(0 != NetWorkTool_Get_Dhcpc_Result(if_name,strip,strnetmask,strgateway))
    {
        LOGE("fail\n");
		if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
		{
			snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
        	Common_System(cmd);
		}

		if(!ovfs_utility_is_net_dev_out("eth0"))
		{
			char eth_ipaddr[32] = {0};
            char eth_gateway[32] = {0};
            NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
            NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
			if(strlen(eth_ipaddr) == 0 || stricmp(eth_ipaddr, "0.0.0.0") == 0)
			{
				snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s",if_name,oldIp,netmask->ipv4);
		    	LOGW("cmd = %s \n", cmd);
				Common_System(cmd);
			}

			if(strlen(eth_gateway) == 0 || stricmp(eth_gateway, "0.0.0.0") == 0)
			{
				if(access("/tmp/is4gDevice",F_OK) != 0 || s_nWiredExtranet == 1)
				{
					snprintf(cmd, sizeof(cmd), "route del default dev %s",if_name);
					LOGW("cmd = %s \n", cmd);
					Common_System(cmd);

					snprintf(cmd, sizeof(cmd), "route add default gw %s dev %s metric 1",gateway->ipv4,if_name);
			    	LOGW("cmd = %s \n", cmd);
					Common_System(cmd);

					//Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
				}
			}
		}
        return OVFS_ERR_NETWORK_BASE;
    }

    //返回获取到的结果
    snprintf((ip->is_ipv4==OVFS_TRUE)?ip->ipv4:ip->ipv6, (ip->is_ipv4==OVFS_TRUE)?sizeof(ip->ipv4):sizeof(ip->ipv6), "%s", strip);
    snprintf((netmask->is_ipv4==OVFS_TRUE)?netmask->ipv4:netmask->ipv6, (netmask->is_ipv4==OVFS_TRUE)?sizeof(netmask->ipv4):sizeof(netmask->ipv6), "%s", strnetmask);
    snprintf((gateway->is_ipv4==OVFS_TRUE)?gateway->ipv4:gateway->ipv6, (gateway->is_ipv4==OVFS_TRUE)?sizeof(gateway->ipv4):sizeof(gateway->ipv6), "%s", strgateway);
    if(!ovfs_utility_ipaddr_valid(ip) ||
        !ovfs_utility_ipaddr_valid(netmask) ||
        (!ovfs_utility_ipaddr_valid(gateway)) )
    {
        LOGE("ip:%s netmask:%s gateway : %s\n", ip->ipv4, netmask->ipv4, gateway->ipv4);
		if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
		{
			snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
        	Common_System(cmd);
		}

		if(!ovfs_utility_is_net_dev_out("eth0"))
		{
			char eth_ipaddr[32] = {0};
            char eth_gateway[32] = {0};
            NetWorkTool_GetIPAddr((char *)"eth0",eth_ipaddr);
            NetWorkTool_GetGateway((char *)"eth0",eth_gateway);
			if(strlen(eth_ipaddr) == 0 || stricmp(eth_ipaddr, "0.0.0.0") == 0)
			{
				snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s",if_name,oldIp,netmask->ipv4);
		    	LOGW("cmd = %s \n", cmd);
				Common_System(cmd);
			}

			if(strlen(eth_gateway) == 0 || stricmp(eth_gateway, "0.0.0.0") == 0)
			{
				if(access("/tmp/is4gDevice",F_OK) != 0 || s_nWiredExtranet == 1)
				{
					snprintf(cmd, sizeof(cmd), "route del default dev %s",if_name);
					LOGW("cmd = %s \n", cmd);
					Common_System(cmd);

					snprintf(cmd, sizeof(cmd), "route add default gw %s dev %s metric 1",gateway->ipv4,if_name);
			    	LOGW("cmd = %s \n", cmd);
					Common_System(cmd);

					//Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
				}
			}
		}
        return OVFS_ERR_NETWORK_BASE;
    }
//	Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
    LOGD("ip:%s netmask:%s gateway : %s\n", ip->ipv4, netmask->ipv4, gateway->ipv4);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::network_get_ipv6_bydhcp6(const S8 *if_name, ovfs_ipaddr_struct *ip, int *prefixlen, ovfs_ipaddr_struct *gateway)
{
	S8 oldIp[64];
    S8 cmd[256];
    S8 udhcpc_pid[64];
    OVFS_CLR_ARG(cmd);
    OVFS_CLR_ARG(udhcpc_pid);
    snprintf(udhcpc_pid, sizeof(udhcpc_pid), "/var/run/udhcpc6_%s.pid",if_name);
    snprintf(oldIp, sizeof(oldIp), "%s",ip->ipv6);

	S8 strip[64];
    S8 strnetmask[64];
    S8 strgateway[64];
    OVFS_CLR_ARG(strip);
    OVFS_CLR_ARG(strnetmask);
    OVFS_CLR_ARG(strgateway);
    int iprefixlen = 0;
    if(OVFS_ERR_NETWORK_UNKNOWN != network_check_udhcpc_pid(udhcpc_pid))
    {
		if (strstr(if_name, "eth") != NULL)
		{
			if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
			{
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
            	Common_System(cmd);
			}

			snprintf(cmd, sizeof(cmd), "udhcpc6 -i %s -R -b -t 4 -T 1 -s /root/bin/udhcpc6.script -p %s",
				if_name,udhcpc_pid);
			LOGW("cmd = %s \n", cmd);
			if(OVFS_SUCCESS != Common_System(cmd))
		    {
		        LOGE("%s fail\n", cmd);

				if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
				{
					snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
	            	Common_System(cmd);
				}

				if(!ovfs_utility_is_net_dev_out("eth0"))
				{
                    char eth_ipaddr[64] = {0};
                    int eth_prefix = 0;
                    char eth_gateway[64] = {0};
                    NetWorkTool_GetDevInfoV6((char *)"eth0",eth_ipaddr,&eth_prefix);
                    NetWorkTool_GetDefaultRouteV6((char *)"eth0",eth_gateway);
                    if(strlen(eth_ipaddr) == 0)
                    {
                        snprintf(cmd, sizeof(cmd), "ifconfig %s %s/%d",if_name,oldIp,*prefixlen);
                        LOGW("cmd = %s \n", cmd);
                        Common_System(cmd);
                    }

				}
		        return OVFS_ERR_NETWORK_BASE;
		    }
		}
		else if (strstr(if_name, "wlan") != NULL)
		{
			snprintf(cmd, sizeof(cmd), "udhcpc6 -i %s -p %s -s udhcpc6.script &",if_name,udhcpc_pid);
			LOGW("cmd = %s \n", cmd);
			if(Common_System(cmd) ==  0)
			{

			}
		}

		int trycount = 200;//出现异常: dhcp 很多次都失败..
		while(--trycount)
		{
			if(OVFS_SUCCESS == check_dhcpc6_result(if_name))
				break;
			Common_Sleep(0, 100 * 1000);
		}
		LOGW("if_name=%s,trycount=%d\n", if_name,trycount);
		if(trycount == 0)
		{
			if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
			{
				snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
            	Common_System(cmd);
			}

			if(!ovfs_utility_is_net_dev_out("eth0"))
			{
				char eth_ipaddr[64] = {0};
                int eth_prefix = 0;
                char eth_gateway[64] = {0};
                NetWorkTool_GetDevInfoV6((char *)"eth0",eth_ipaddr,&eth_prefix);
                NetWorkTool_GetDefaultRouteV6((char *)"eth0",eth_gateway);
                if(strlen(eth_ipaddr) == 0)
                {
                    snprintf(cmd, sizeof(cmd), "ifconfig %s %s/%d",if_name,oldIp,*prefixlen);
                    LOGW("cmd = %s \n", cmd);
                    Common_System(cmd);
                }

				if(strlen(eth_gateway) == 0)
				{
					if(access("/tmp/is4gDevice",F_OK) != 0)
					{
						NetWorkTool_SetDefaultRouteV6((char *)if_name,eth_gateway);
					}
				}
			}
			return OVFS_ERR_NETWORK_BASE;
		}

    }

	if(0 != NetWorkTool_Get_Dhcpc6_Result(if_name,strip,&iprefixlen,strgateway))
    {
        LOGE("fail\n");
		if(ovfs_utility_is_net_dev_up("eth0") == OVFS_FALSE)
		{
			snprintf(cmd, sizeof(cmd), "ifconfig eth0 up");
        	Common_System(cmd);
		}

		if(!ovfs_utility_is_net_dev_out("eth0"))
		{
			char eth_ipaddr[64] = {0};
            int eth_prefix = 0;
            char eth_gateway[64] = {0};
            NetWorkTool_GetDevInfoV6((char *)"eth0",eth_ipaddr,&eth_prefix);
            NetWorkTool_GetDefaultRouteV6((char *)"eth0",eth_gateway);
            if(strlen(eth_ipaddr) == 0)
            {
                snprintf(cmd, sizeof(cmd), "ifconfig %s %s/%d",if_name,oldIp,*prefixlen);
                LOGW("cmd = %s \n", cmd);
                Common_System(cmd);
            }

			if(strlen(eth_gateway) == 0)
			{
				if(access("/tmp/is4gDevice",F_OK) != 0)
				{
					NetWorkTool_SetDefaultRouteV6((char *)if_name,eth_gateway);
				}
			}
		}
        return OVFS_ERR_NETWORK_BASE;
    }

    //返回获取到的结果
    snprintf(ip->ipv6, sizeof(ip->ipv6), "%s", strip);
    *prefixlen = iprefixlen;
    snprintf(gateway->ipv6, sizeof(gateway->ipv6), "");

    //NetWorkTool_SetDefaultRouteV6((char *)if_name, gateway->ipv6);

//	Common_System("rm /tmp/ppp0_ok");//ppp0的路由优先
    LOGD("ip:%s netmask:%d gateway : %s\n", ip->ipv6, *prefixlen, gateway->ipv6);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::get_dns_cfg(ovfs_soft::ovfs_local_net_dns *dns)
{
    if(dns->auto_dns == OVFS_TRUE)
    {
        if(OVFS_SUCCESS != GetDNS(dns,(char *)"/etc/resolv.conf"))
            return OVFS_ERR_NETWORK_BASE;
        else
        {
            dns->auto_dns = OVFS_TRUE;
        }
    }
    else
    {
        if(OVFS_SUCCESS != ovfs_cfgm_get_dns_cfg(dns))
            return OVFS_ERR_NETWORK_INVALID_PARA;
    }
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::update_new_dns(const ovfs_soft::ovfs_local_net_dns *dns)
{
	FILE *fp = fopen("/etc/resolv.conf", "w+");
	if(fp == NULL)
	{
		LOGE("open file fail\n");
		return OVFS_ERR_NETWORK_BASE;
	}

	S8 sz_param[256] = {0};

	if(!strstr(sz_param, dns->dns1.ipv4))
	{
		snprintf(sz_param + strlen(sz_param), sizeof(sz_param) - strlen(sz_param), "nameserver %s\n",dns->dns1.ipv4);
	}

	if(!strstr(sz_param, dns->dns2.ipv4))
	{
		snprintf(sz_param + strlen(sz_param), sizeof(sz_param) - strlen(sz_param), "nameserver %s\n",dns->dns2.ipv4);
	}

	if(!strstr(sz_param, "223.5.5.5"))
	{
		snprintf(sz_param + strlen(sz_param), sizeof(sz_param) - strlen(sz_param), "%s", "nameserver 223.5.5.5\n");
	}

	if(!strstr(sz_param, "8.8.8.8"))
	{
		snprintf(sz_param + strlen(sz_param), sizeof(sz_param) - strlen(sz_param), "%s", "nameserver 8.8.8.8\n");
	}

	fwrite(sz_param, 1, strlen(sz_param), fp);
	fclose(fp);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::network_get_dns_bydhcp(S8 *if_name, ovfs_local_net_dns *net_dns)
{
	S8 cmd[1024];
	S8 dns1[64];
	S8 dns2[64];
	S8 buf[64];
    OVFS_CLR_ARG(cmd);
	OVFS_CLR_ARG(dns1);
	OVFS_CLR_ARG(dns2);

    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc_resolv_%s.conf | sed -n \'1p\' | awk \'{print $2}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(dns1, strlen(dns1), "%s", buf);
	}

    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc_resolv_%s.conf | sed -n \'2p\' | awk \'{print $2}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(dns2, strlen(dns2), "%s", buf);
	}

    LOGW("[DHCP]if_name=%s,dns :%s , %s\n", if_name,dns1, dns2);

	if(strlen(dns1) == 0
		&& strlen(dns2) == 0)
	return OVFS_ERR_NETWORK_BASE;

	net_dns->dns1.is_ipv4 = strstr(dns1, ":")==NULL?OVFS_TRUE:OVFS_FALSE;
	net_dns->dns2.is_ipv4 = strstr(dns2, ":")==NULL?OVFS_TRUE:OVFS_FALSE;

	if(net_dns->dns1.is_ipv4 == OVFS_TRUE)
	{
		snprintf(net_dns->dns1.ipv4, sizeof(net_dns->dns1.ipv4), "%s", dns1);
	}
	else
	{
		snprintf(net_dns->dns1.ipv6, sizeof(net_dns->dns1.ipv6), "%s", dns1);
	}

	if(net_dns->dns2.is_ipv4 == OVFS_TRUE)
	{
		snprintf(net_dns->dns2.ipv4, sizeof(net_dns->dns2.ipv4), "%s", dns2);
	}
	else
	{
		snprintf(net_dns->dns2.ipv6, sizeof(net_dns->dns2.ipv6), "%s", dns2);
	}


	net_dns->auto_dns = OVFS_TRUE;

	if(ovfs_cfgm_update_auto_dns_cfg(net_dns) != OVFS_SUCCESS)
		return OVFS_ERR_NETWORK_INVALID_PARA;

	return OVFS_SUCCESS;
}


OVFS_ERR ovfs_network_config_mgr::set_dns_auto()
{
	ovfs_local_net_dns net_dns;
	OVFS_CLR_ARG(net_dns);

	S8 router[64];
	if(get_default_if(router, sizeof(router)) != OVFS_SUCCESS)
		return OVFS_ERR_NETWORK_BASE;

	if(OVFS_SUCCESS != network_get_dns_bydhcp(router, &net_dns))
		return OVFS_ERR_NETWORK_BASE;

	return update_new_dns(&net_dns);

}

OVFS_ERR ovfs_network_config_mgr::set_dns_cfg(const ovfs_local_net_dns *dns)
{
	if( ((ovfs_utility_ipaddr_valid((ovfs_ipaddr_struct *)&dns->dns1) == OVFS_FALSE && strlen(dns->dns1.is_ipv4?dns->dns1.ipv4:dns->dns1.ipv6) != 0)
			|| (ovfs_utility_ipaddr_valid((ovfs_ipaddr_struct *)&dns->dns2) == OVFS_FALSE && strlen(dns->dns2.is_ipv4?dns->dns2.ipv4:dns->dns2.ipv6) != 0))
		&& dns->auto_dns != OVFS_TRUE )
		return OVFS_ERR_NETWORK_INVALID_PARA;


	if(ovfs_cfgm_set_dns_cfg(dns) != OVFS_SUCCESS)
		return OVFS_ERR_NETWORK_INVALID_PARA;

	network_setting_task_node *node = malloc_net_set_task(NETWORK_SET_DNS_CFG, (const S8 *)dns, sizeof(ovfs_local_net_dns));
	if(node == NULL)
		return OVFS_ERR_NETWORK_INVALID_PARA;

	Common_Lock(m_lock);
	if(OVFS_SUCCESS != add_new_task(&m_setting_task_list, node))
	{
		Common_UnLock(m_lock);
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}
	Common_UnLock(m_lock);

	Common_InterSleep_WakeUp(m_sleep_ops);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::set_dns_cfgv2(const ovfs_local_net_dns *dns)
{
	if( ((ovfs_utility_ipaddr_valid((ovfs_ipaddr_struct *)&dns->dns1) == OVFS_FALSE && strlen(dns->dns1.is_ipv4?dns->dns1.ipv4:dns->dns1.ipv6) != 0)
			|| (ovfs_utility_ipaddr_valid((ovfs_ipaddr_struct *)&dns->dns2) == OVFS_FALSE && strlen(dns->dns2.is_ipv4?dns->dns2.ipv4:dns->dns2.ipv6) != 0))
		&& dns->auto_dns != OVFS_TRUE )
		return OVFS_ERR_NETWORK_INVALID_PARA;

#if 0	//ovfs_cfgm_set_dns_cfg中存放的是有线的dns, 本接口是4g设置dns接口
	if(ovfs_cfgm_set_dns_cfg(dns) != OVFS_SUCCESS)
		return OVFS_ERR_NETWORK_INVALID_PARA;
#endif
	network_setting_task_node *node = malloc_net_set_task(NETWORK_SET_DNS_CFG, (const S8 *)dns, sizeof(ovfs_local_net_dns));
	if(node == NULL)
		return OVFS_ERR_NETWORK_INVALID_PARA;

	Common_Lock(m_lock);
	if(OVFS_SUCCESS != add_new_task(&m_setting_task_list, node))
	{
		Common_UnLock(m_lock);
		return OVFS_ERR_NETWORK_INVALID_PARA;
	}
	Common_UnLock(m_lock);

	Common_InterSleep_WakeUp(m_sleep_ops);

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::update_dns_cfg(const ovfs_soft::ovfs_local_net_dns *dns)
{
    LOGD("dns->auto_dns=%d\n",dns->auto_dns);
	if(dns->auto_dns == OVFS_TRUE)
		return set_dns_auto();
	else
		return update_new_dns(dns);
}

OVFS_BOOL ovfs_network_config_mgr::is_if_can_be_router(const S8* if_name)
{
	U32 i = 0;
	for(i = 0; i < m_network_ability->total_eth_num; ++i)
	{
		if(0 == strcmp(m_network_ability->eth_status[i].if_name, if_name)
		&& m_network_ability->eth_status[i].is_exist == OVFS_TRUE
		&& m_network_ability->eth_status[i].ability_mask&OVFS_ETH_EANBLE
		&& m_network_ability->eth_status[i].ability_mask&OVFS_ETH_CAN_BE_ROUTER)
		{
			break;
		}
	}

	if(i == m_network_ability->total_eth_num)
		return OVFS_FALSE;

	return OVFS_TRUE;
}


OVFS_ERR ovfs_network_config_mgr::get_default_if(S8 * if_name, U32 size)
{
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::set_default_if(const  S8* if_index)
{
    LOGD("if_index=%s\n",if_index);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::get_default_router( S8* if_name, U32 size)
{
    //LOGD("get_default_router enter\n");

	if(0 == g_metric)
		snprintf(if_name, size, "eth0");
	else if(1 == g_metric)
		snprintf(if_name, size, "wlan0");
	else if(2 == g_metric)
		snprintf(if_name, size, str_RNDISifname);
#if 0
    OVFS_ERR ret = OVFS_ERR_NETWORK_BASE;

	ret = ovfs_cfgm_get_default_if(if_name, size);
	if(ret == OVFS_SUCCESS )
	{
		if(is_if_can_be_router(if_name) != OVFS_TRUE)
			ret = OVFS_ERR_NETWORK_BASE;
	}

	U32 i = 0;

	if(ret != OVFS_SUCCESS)
	{

		for(i = 0; i < m_network_ability->total_eth_num; ++i)
		{
			//NET_CLASS_PW(" %s %x \n", m_network_ability->eth_status[i].if_name, m_network_ability->eth_status[i].ability_mask);
			if(m_network_ability->eth_status[i].is_exist == OVFS_TRUE
			&& m_network_ability->eth_status[i].ability_mask&OVFS_ETH_EANBLE
			&& m_network_ability->eth_status[i].ability_mask&OVFS_ETH_CAN_BE_ROUTER)
			{
				break;
			}
		}

		if(i == m_network_ability->total_eth_num)
			return OVFS_ERR_NETWORK_BASE;


		snprintf(if_name, size, "%s", m_network_ability->eth_status[i].if_name);
	}
#endif
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::push_net_setting_task(network_setting_task_list *list, network_setting_task_node *node)
{
	node->next = NULL;
	node->prev = NULL;
	if(list->head == NULL)
	{
		list->head = node;
		list->tail = node;
	}
	else
	{
		node->prev = list->tail;
		list->tail->next = node;
		list->tail = node;
	}
	list->task_num++;

	return OVFS_SUCCESS;
}

OVFS_BOOL ovfs_network_config_mgr::is_task_confict(network_setting_task_node *node, network_setting_task_node *new_node)
{
	if(node->task_type == new_node->task_type)
	{
		if((node->task_type == NETWORK_SET_NET_CAED_CFG && strcmp(node->net_cfg.card_cfg.if_name, new_node->net_cfg.card_cfg.if_name)== 0)
			|| (node->task_type == NETWORK_RESET_NETCARD && strcmp(node->net_cfg.card_cfg.if_name, new_node->net_cfg.card_cfg.if_name)== 0)
			|| (node->task_type == NETWORK_SET_BOND_CFG && strcmp(node->net_cfg.bond_cfg.if_name, new_node->net_cfg.bond_cfg.if_name)== 0)
			|| node->task_type == NETWORK_SET_DEFAULT_ROUTE_CFG
			|| node->task_type == NETWORK_SET_DNS_CFG
			)
		{
			return OVFS_TRUE;
		}
	}

	return OVFS_FALSE;
}

OVFS_ERR ovfs_network_config_mgr::add_new_task(network_setting_task_list *list, network_setting_task_node *new_node)
{
	network_setting_task_node *node = list->head;

	//判断新的节点是否在链表里面存在相同的类型和网卡名称 如果存在 就更新任务节点 如果不存在就添加
	while(NULL != node)
	{
		if(is_task_confict(node, new_node) == OVFS_TRUE)
		{
			break;
		}

		node = node->next;
	}


	if(node == NULL)
	{
		push_net_setting_task(list, new_node);
	}
	else
	{
		new_node->next = node->next;
		new_node->prev = node->prev;

		memcpy(node, new_node, sizeof(network_setting_task_node));
		Common_Free(new_node,__FUNCTION__,__LINE__);
	}
	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_config_mgr::pop_net_setting_task(network_setting_task_list *list, network_setting_task_node *node)
{
	network_setting_task_node *next = node->next;
	network_setting_task_node *prev = node->prev;

	if(prev != NULL)
	{
		prev->next = next;
	}
	else
	{
		list->head = next;
	}

	if(next != NULL)
	{
	  next->prev = prev;
	}
	else
	{
		list->tail = prev;
	}

	node->next = NULL;
	node->prev = NULL;
	list->task_num--;
	return OVFS_SUCCESS;
}


network_setting_task_node* ovfs_network_config_mgr::get_one_net_setting_task()
{
	network_setting_task_node *node = m_setting_task_list.head;

	if(node == NULL)
	return NULL;

	pop_net_setting_task(&m_setting_task_list, node);

	return node;

}

OVFS_VOID ovfs_network_config_mgr::release_one_net_setting_task(network_setting_task_node *node_busy, OVFS_BOOL is_success)
{
	if(is_success == OVFS_TRUE)
	{
		Common_Free(node_busy,__FUNCTION__,__LINE__);
		return;
	}

	network_setting_task_node *node = m_setting_task_list.head;
	while(NULL != node)
	{
		//查找当前节点是否在m_setting_task_list 链表里面有相同的
		if(is_task_confict(node, node_busy))
				break;

		node = node->next;
	}


	if(node != NULL)
	{
		//如果有相同的 就释放掉该节点
		Common_Free(node_busy,__FUNCTION__,__LINE__);
		return;
	}
	else
	{
		//如果没有就把改节点再添加进去
		push_net_setting_task(&m_setting_task_list, node_busy);
	}
}

network_setting_task_node *ovfs_network_config_mgr::malloc_net_set_task(network_task_type task_type, const S8 * para, U32 size)
{
	if(para == NULL)
		return NULL;

	network_setting_task_node *node = (network_setting_task_node *)Common_Malloc(sizeof(network_setting_task_node), 0,__FUNCTION__,__LINE__);
	if(node == NULL)
		return NULL;

	memset(node, 0, sizeof(network_setting_task_node));
	node->task_type = task_type;

	switch(task_type)
	{
		case NETWORK_SET_NET_CAED_CFG:
			if(sizeof(node->net_cfg.card_cfg) != size)
				goto leave_fail;
			break;

		case NETWORK_SET_DNS_CFG:
			if(sizeof(node->net_cfg.dns_cfg) != size)
				goto leave_fail;
			break;

		case NETWORK_RESET_NETCARD:
			if(sizeof(node->net_cfg.card_cfg) != size)
				goto leave_fail;
			break;

		default:
			goto leave_fail;
			break;
	}

	memcpy(&node->net_cfg, para, size);

	return node;

leave_fail:
	Common_Free(node,__FUNCTION__,__LINE__);
	OVFS_ASSERT(0);
	return NULL;
}

OVFS_ERR ovfs_network_config_mgr::get_special_netcard_traffic(const S8* if_name, U32 *upload, U32 *download)
{
	U32 eth_num = 0;
	if(OVFS_SUCCESS != ovfs_cfgm_get_idx_by_eth_name(if_name, &eth_num))
	{
		return OVFS_ERR_NETWORK_BASE;
	}

	if(eth_num>=OVFS_ARRAY_DIM(m_netcard_speedInfo))
	{
		return OVFS_ERR_NETWORK_BASE;
	}

	U64 cur_rx_size = 0;
	U64 cur_tx_size = 0;
	FILE *fp;
    char buf[4096];
    char cmd[128];

	snprintf(cmd, sizeof(cmd), "%s %s", "ifconfig", if_name);

    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
		S8* pstr = strstr(buf, "lo");
		if(pstr != NULL)
		{
          	break;
		}
		else
		{
			S64 data_size = 0;
			pstr = strstr(buf, "RX bytes:");
			if(pstr != NULL)
			{
				data_size = strtoull(pstr + strlen("RX bytes:"), NULL, 10);
				cur_rx_size += data_size;
			}

			pstr = strstr(buf, "TX bytes:");
			if(pstr != NULL)
			{
				data_size = strtoull(pstr + strlen("TX bytes:"), NULL, 10);
				cur_tx_size += data_size;
			}
		}
    }
	pclose(fp);

	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);

	Common_Lock(m_lock_netspeed);

	//如果是第一次网络流量只取参照标本
	if(m_netcard_speedInfo[eth_num].m_last_tx_size == 0)
	{
		m_netcard_speedInfo[eth_num].m_last_cnt_time = cur_time;
		m_netcard_speedInfo[eth_num].m_last_rx_size = cur_rx_size;
		m_netcard_speedInfo[eth_num].m_last_tx_size = cur_tx_size;

		Common_UnLock(m_lock_netspeed);

		return OVFS_SUCCESS;
	}

	if(Common_cnt_interval_ms(&cur_time, &m_netcard_speedInfo[eth_num].m_last_cnt_time) != 0)
	{
		m_netcard_speedInfo[eth_num].m_rx_speed = (cur_rx_size - m_netcard_speedInfo[eth_num].m_last_rx_size) * 1000 / Common_cnt_interval_ms(&cur_time, &m_netcard_speedInfo[eth_num].m_last_cnt_time);
		m_netcard_speedInfo[eth_num].m_tx_speed = (cur_tx_size - m_netcard_speedInfo[eth_num].m_last_tx_size) * 1000 / Common_cnt_interval_ms(&cur_time, &m_netcard_speedInfo[eth_num].m_last_cnt_time);

		m_netcard_speedInfo[eth_num].m_last_cnt_time = cur_time;
		m_netcard_speedInfo[eth_num].m_last_rx_size = cur_rx_size;
		m_netcard_speedInfo[eth_num].m_last_tx_size = cur_tx_size;
	}

	Common_UnLock(m_lock_netspeed);

	return OVFS_SUCCESS;
}

