/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 处理订阅相关
 *	源 文 件 : ovfs_network_event.cpp
 *	作    者 : 舒适
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
//#include <sys/syscall.h>


#include "ovfs_comm_def.h"
//#include "ovfs_comm_debug.h"
#include "ovfs_comm_errno.h"

#include "ovfs_network_base_def.h"
#include "ovfs_network_api.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_arp_tool_api.h"
#include "ovfs_network_rest_common.h"
#include "ovfs_network_event.h"
#include "ovfs_network_snmp.h"
#include "ovfs_network_utility_api.h"


using namespace ovfs_soft;
//using namespace ovfs_cfgm;

#define MAX_NETWORK_EVNET_NUM (2)
#define MAX_NETWORK_GETEVNET_NUM (4)

typedef struct
{
	Common_Thread_T s_alarm_thread;
	int m_net_out[OVFS_MAX_ETH_CARD_NUM];
	int m_ip_conflict[OVFS_MAX_ETH_CARD_NUM];
	int s_alarm_exit;
	int nSubscribeId[MAX_NETWORK_GETEVNET_NUM];
	int m_upnp_port[OVFS_PORT_MAX];
} NETWORK_EVENT_T;

typedef struct
{
	char* subUri;
	int   id[16];
}NETWORK_REST_EVENT_SUB_INFO_T;


static pthread_mutex_t s_network_subLock;
static NETWORK_EVENT_T  s_network_event;
static NETWORK_REST_EVENT_SUB_INFO_T s_network_eventSubInfo[MAX_NETWORK_EVNET_NUM];



static int NetWork_Event_FindMatchSubUri(const char* uri)
{
	int ret = -1;
	unsigned int i = 0;
	
	pthread_mutex_lock(&s_network_subLock);
	
	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo);i++)
	{
		if(NULL != s_network_eventSubInfo[i].subUri && 
		   NULL != uri && 
		  (0 == strcmp(s_network_eventSubInfo[i].subUri,uri)))
		{
			ret = i;
			break;
		}
	}
	
	pthread_mutex_unlock(&s_network_subLock);
	
	return ret;
}

static int NetWork_Event_SetSubId(int idx,int id)
{
	int ret = -1;
	unsigned int i = 0;
	
	pthread_mutex_lock(&s_network_subLock);
	
	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo[idx].id);i++)
	{
		if(s_network_eventSubInfo[idx].id[i] <= 0)
		{
			s_network_eventSubInfo[idx].id[i] = id;
			ret = 0;
			break;
		}
	}
	
	pthread_mutex_unlock(&s_network_subLock);
	
	return ret;
}

static int NetWork_Event_UnSetSubId(int idx,int id)
{
	int ret = -1;
	unsigned int i = 0;
	
	pthread_mutex_lock(&s_network_subLock);

	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo[idx].id);i++)
	{
		if(s_network_eventSubInfo[idx].id[i] == id)
		{
			s_network_eventSubInfo[idx].id[i] = 0;
			ret = 0;
			break;
		}
	}
	
	pthread_mutex_unlock(&s_network_subLock);
	
	return ret;
}

static int NetWork_Event_CheckSubId(int idx,int id)
{
	int ret = -1;
	unsigned int i = 0;

	pthread_mutex_lock(&s_network_subLock);
	
	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo[idx].id);i++)
	{
		if(s_network_eventSubInfo[idx].id[i] == id)
		{
			ret =  id;
			break;
		}
	}
	
	pthread_mutex_unlock(&s_network_subLock);

	return ret;
}

#if 0
static int FillRTSPInfo(cJSON_Struct* parentItem)
{
	S32 nIntValue;		

    if (parentItem != NULL)
    {
	    nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Data/Rtsp/RtspPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
		    LOGI("%s: RtspPort %d %d\n",__FUNCTION__,s_network_event.m_upnp_port[OVFS_PORT_RTSP],nIntValue);
		    if (s_network_event.m_upnp_port[OVFS_PORT_RTSP] != nIntValue)
		    {
                ovfs_network_set_upnp_port_info(OVFS_PORT_RTSP,OVFS_TRUE,nIntValue,nIntValue,OVFS_PROTOCOL_TCP,(char *)"eth0");
				s_network_event.m_upnp_port[OVFS_PORT_RTSP] = nIntValue;
		    }	
		}	
    }
	else
	{
	    LOGE("%s: Can't find %s\n",__FUNCTION__,"RtspPort");
	}
	
    return 0;
}

static int FillRTMPInfo(cJSON_Struct* parentItem)
{
	S32 nIntValue;		

    if (parentItem != NULL)
    {
	    nIntValue = -1;
		Common_Json_GetAttrValue(parentItem,-1,"Data/Rtmp/RtmpPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
		    LOGI("%s: RtmpPort %d %d\n",__FUNCTION__,s_network_event.m_upnp_port[OVFS_PORT_RTMP],nIntValue);
		    if (s_network_event.m_upnp_port[OVFS_PORT_RTMP] != nIntValue)
			{
                ovfs_network_set_upnp_port_info(OVFS_PORT_RTMP,OVFS_TRUE,nIntValue,nIntValue,OVFS_PROTOCOL_TCP,(char *)"eth0");
				s_network_event.m_upnp_port[OVFS_PORT_RTMP] = nIntValue;
		    }
		}	
    }
	else
	{
	    LOGE("%s: Can't find %s\n",__FUNCTION__,"Port");
	}
	
    return 0;
}
#endif

static int NetWork_Event_FindMatchID(S32 nSubscribeID)
{
	int ret = -1;
	unsigned int i = 0;
	
	pthread_mutex_lock(&s_network_subLock);
	
	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_event.nSubscribeId);i++)
	{
		if(nSubscribeID == s_network_event.nSubscribeId[i])
		{
			ret = i;
			break;
		}
	}
	
	pthread_mutex_unlock(&s_network_subLock);
	
	return ret;
}

static S32 NetWork_GetEvent(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
#if 0
	char* out = NULL;
	LOGI("recv call input:%s \n",out = Common_cJSON_PrintUnformatted((Common_cJSON_T*)pEventInfo,NULL));
	if(out)
	{
		Common_Free(out,__FUNCTION__,__LINE__);
	}
#endif
    int nIdx = 0;

    nIdx = NetWork_Event_FindMatchID(nSubscribeID);
	if(nIdx < 0)
	{
		LOGE("nSubscribeID uri=%d not supported!\n",nSubscribeID);
		return -1;
	}
	
    if (0 == nIdx)
    {
        //FillRTSPInfo(pEventInfo);
    }
    else if (1 == nIdx)
    {
        //FillRTMPInfo(pEventInfo);
    }
	else if (2 == nIdx || 3 == nIdx)
	{
        NetWork_Send_SNMP(pEventInfo);
	}

    return 0;
}

static int NetWork_Check_NetOut(Common_cJSON_T* out, int type)
{
    U32 i = 0;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
    const ovfs_network_capability *m_network_capability = NULL;
	int net_out = 0;
	
	if (NULL == out)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}
	
    tmp_array = Common_cJSON_CreateArray();
	if (NULL != tmp_array)
	{
	#if 0
	    if (0 == type)
	    {
		    Common_cJSON_AddItemToObject(out,"NetOut",tmp_array);
		    for (i = 0;i < m_network_capability->total_eth_num;i++)
		    {
				net_out = ovfs_arptool_check_net_out(m_network_capability->eth_status[i].if_name);

				tmp = Common_cJSON_CreateObject();
				Common_cJSON_AddStringToObject(tmp,"EthName",m_network_capability->eth_status[i].if_name);
				Common_cJSON_AddNumberToObject(tmp,"NetOut",net_out);
				Common_cJSON_AddItemToArray(tmp_array,tmp);
			}
	    }
		else if (1 == type)
	#endif
		{
		    Common_cJSON_AddItemToObject(out,"ResList",tmp_array);
		    for (i = 0;i < m_network_capability->total_eth_num;i++)
		    {
				//net_out = ovfs_arptool_check_net_out(m_network_capability->eth_status[i].if_name);
				net_out = ovfs_utility_is_net_dev_out(m_network_capability->eth_status[i].if_name);

				tmp = Common_cJSON_CreateObject();
				Common_cJSON_AddStringToObject(tmp,"AlarmName","NetCableBreak");
				Common_cJSON_AddNumberToObject(tmp,"IsHappent",net_out);
				Common_cJSON_AddStringToObject(tmp,"DevName",m_network_capability->eth_status[i].if_name);				
				Common_cJSON_AddItemToArray(tmp_array,tmp);
			}
		}
	}
	
	return 0;
}

static int NetWork_Check_NetIpConflict(Common_cJSON_T* out, int type)
{
    U32 i = 0;
	Common_cJSON_T* tmp = NULL;
	Common_cJSON_T* tmp_array = NULL;
    const ovfs_network_capability *m_network_capability = NULL;
	int ip_conflict = 0;
	
	if (NULL == out)
	{
		LOGE("param is NULL.\n");
		return -1;
	}

	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}
	
    tmp_array = Common_cJSON_CreateArray();
	if (NULL != tmp_array)
	{
	#if 0
	    if (0 == type)
	    {
		    Common_cJSON_AddItemToObject(out,"NetIpConflict",tmp_array);
	        for (i = 0;i < m_network_capability->total_eth_num;i++)
	        {
				ip_conflict = ovfs_arptool_check_local_IP_conflict(m_network_capability->eth_status[i].if_name);

				tmp = Common_cJSON_CreateObject();
				Common_cJSON_AddStringToObject(tmp,"EthName",m_network_capability->eth_status[i].if_name);
				Common_cJSON_AddNumberToObject(tmp,"NetIpConflict",ip_conflict);
				Common_cJSON_AddItemToArray(tmp_array,tmp);
			}
	    }
		else if (1 == type)
	#endif
		{
		    Common_cJSON_AddItemToObject(out,"ResList",tmp_array);
	        for (i = 0;i < m_network_capability->total_eth_num;i++)
	        {
				ip_conflict = ovfs_arptool_check_local_IP_conflict(m_network_capability->eth_status[i].if_name);

				tmp = Common_cJSON_CreateObject();
				Common_cJSON_AddStringToObject(tmp,"AlarmName","IpConflict");
				Common_cJSON_AddNumberToObject(tmp,"IsHappent",ip_conflict);
				Common_cJSON_AddStringToObject(tmp,"DevName",m_network_capability->eth_status[i].if_name);	
				Common_cJSON_AddItemToArray(tmp_array,tmp);
			}
		}
	}
	
	return 0;
}

static int NetWork_Check_SendFlag(int idx)
{
    U32 i = 0, ret = -1;
    const ovfs_network_capability *m_network_capability = NULL;
	int net_out = 0, ip_conflict = 0;
	
	m_network_capability = ovfs_cfgm_get_network_cap();
    if (NULL == m_network_capability)
    {
	    LOGE("[%s:%d]:ovfs_cfgm_get_network_cap fail\n",__FUNCTION__, __LINE__);
	    return -1;
	}
	
    for (i = 0;i < m_network_capability->total_eth_num && i < OVFS_MAX_ETH_CARD_NUM;i++)
    {
        if (0 == idx)
        {
		    //net_out = ovfs_arptool_check_net_out(m_network_capability->eth_status[i].if_name);
		    net_out = ovfs_utility_is_net_dev_out(m_network_capability->eth_status[i].if_name);
			//LOGW("[%s:%d]:net_out = %d %d\n",__FUNCTION__, __LINE__,net_out,s_network_event.m_net_out[i]);
			if (s_network_event.m_net_out[i] != net_out)
			{
                s_network_event.m_net_out[i] = net_out;
				ret = 1;
			}
        }
		else if (1 == idx)
		{
			ip_conflict = ovfs_arptool_check_local_IP_conflict(m_network_capability->eth_status[i].if_name);
			//LOGW("[%s:%d]:ip_conflict = %d %d\n",__FUNCTION__, __LINE__,ip_conflict,s_network_event.m_ip_conflict[i]);
			if (s_network_event.m_ip_conflict[i] != ip_conflict)
			{
                s_network_event.m_ip_conflict[i] = ip_conflict;
				ret = 1;
			}			
		}
	}

	
	return ret;
}

static int NetWork_Check_Event(int idx,Common_cJSON_T* out,int type)
{
    int ret = -1;
	
    if (0 == idx)
    {
        ret = NetWork_Check_NetOut(out,type);
	}
	else if (1 == idx)
	{
        ret= NetWork_Check_NetIpConflict(out,type);
	}
	
	return ret;
}

static int NetWork_EventSend(int type)
{
	unsigned int i = 0;
	Common_cJSON_T* param = Common_cJSON_CreateObject();
	Common_cJSON_T* retParam = NULL;
	NetWork_Check_Event(type,param,1);

	pthread_mutex_lock(&s_network_subLock);

	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo[type].id);i++)
	{
	    if(s_network_eventSubInfo[type].id[i] <= 0)
	    {
			continue;
	    }	

        retParam = NULL;
		Module_SendEvent(NetWork_RestComm_GetModuleHdl(),s_network_eventSubInfo[type].id[i],(cJSON_Struct*)param,(cJSON_Struct**)(&retParam),2000);
		if(retParam)
		{
			Common_cJSON_Delete(retParam);
		}	

        {
			char* out = NULL;
			LOGI("send Event. type=%d result=%s\n",type,out = Common_cJSON_Print(param,NULL));
			if(out)
			{
			    Common_Free(out,__FUNCTION__,__LINE__);
			}	
        }
	}
	
	pthread_mutex_unlock(&s_network_subLock);

	Common_cJSON_Delete(param);
	
	return 0;
}

/* 0-subscribe,1-unsubscribe,2-QueryEvent*/
static S32 NetWork_EventSubscribe(ModuleHandle_T hModuleHandle,S32 nType,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData)
{
    int nIdx = 0;
		
    nIdx = NetWork_Event_FindMatchSubUri(szSubscribeUri);
	if(nIdx < 0)
	{
		LOGE("subscribe uri=%s not supported!\n",szSubscribeUri);
		return -1;
	}

	if(access("/tmp/network_debug",F_OK) == 0)
	{
		LOGI("NetWork_EventSubscribe URI:%s nType:%d\n",szSubscribeUri,nType);	
	}

	if (0 == nType)
    {
        NetWork_Event_SetSubId(nIdx,nRecvID);

		if(pQueryEventInfo)
		{
			Common_cJSON_T* outParam = Common_cJSON_CreateObject();
			*pQueryEventInfo = outParam;
			NetWork_Check_Event(nIdx,outParam,0);
		}		
	}
	else if (1 == nType)
	{
        NetWork_Event_UnSetSubId(nIdx,nRecvID);
	}
	else if (2 == nType)
	{
		if(NetWork_Event_CheckSubId(nIdx,nRecvID))
		{	
			if(pQueryEventInfo)
			{
				Common_cJSON_T* outParam = Common_cJSON_CreateObject();
				*pQueryEventInfo = outParam;
				NetWork_Check_Event(nIdx,outParam,0);
			}
		}
		else
		{
			LOGE("Can't found id. Uri:%s\n",szSubscribeUri);
		}	
	}
	
    return 0;
}

static S32 AlarmDoThread(Common_Thread_T hThreadHandle,void *data)
{
	unsigned int i = 0;
	
    while(1)
    {
        if (s_network_event.s_alarm_exit)
        {
            break;
		}
		
        Common_Sleep(1, 0);

		for(i = 0;i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo);i++)
		{
           if (NetWork_Check_SendFlag(i) > 0)
           {
               NetWork_EventSend(i);
		   }
		}	
	}

	return 0;
}

int NetWork_Event_Start()
{
    int ret = 0;

	ret = Module_RegisterSubscribe(NetWork_RestComm_GetModuleHdl(),(S8 *)"/Network/Subscribe",NetWork_EventSubscribe,NULL);
	if(ret != 0)
	{	
		LOGE("resgister subscribe fail!\n");
	}

#if 0	
	ret = Module_SubscribeEvent(NetWork_RestComm_GetModuleHdl(),(S8 *)"/MediaServer/Subscribe/Rtsp/Attribute",NetWork_GetEvent,NULL);
	if(ret <= 0)
	{	
		LOGE("Module_SubscribeEvent /MediaServer/Subscribe/Rtsp/Attribute  fail!\n");
	}
	else
	{
        s_network_event.nSubscribeId[0] = ret;
		LOGI("Module_SubscribeEvent /MediaServer/Subscribe/Rtsp/Attribute  %d!\n",s_network_event.nSubscribeId[0]);
	}
	
	ret = Module_SubscribeEvent(NetWork_RestComm_GetModuleHdl(),(S8 *)"/MediaServer/Subscribe/Rtmp/Attribute",NetWork_GetEvent,NULL);
	if(ret <= 0)
	{	
		LOGE("Module_SubscribeEvent /MediaServer/Subscribe/Rtmp/Attribute fail!\n");
	}
	else
	{
        s_network_event.nSubscribeId[1] = ret;
		LOGI("Module_SubscribeEvent /MediaServer/Subscribe/Rtmp/Attribute %d!\n",s_network_event.nSubscribeId[1]);
	}
#endif	

	ret = Module_SubscribeEvent(NetWork_RestComm_GetModuleHdl(),(S8 *)"/Alarm/Subscribe/Status?AlarmName=Vhide",NetWork_GetEvent,NULL);
	if(ret <= 0)
	{	
		LOGE("Module_SubscribeEvent /Alarm/Subscribe/Status?AlarmName=Vhide fail!\n");
	}
	else
	{
        s_network_event.nSubscribeId[2] = ret;
		LOGI("Module_SubscribeEvent /Alarm/Subscribe/Status?AlarmName=Vhide %d!\n",s_network_event.nSubscribeId[2]);
	}

	ret = Module_SubscribeEvent(NetWork_RestComm_GetModuleHdl(),(S8 *)"/Alarm/Subscribe/Status?AlarmName=Vdiagnose",NetWork_GetEvent,NULL);
	if(ret <= 0)
	{	
		LOGE("Module_SubscribeEvent /Alarm/Subscribe/Status?AlarmName=Vdiagnose fail!\n");
	}
	else
	{
        s_network_event.nSubscribeId[3] = ret;
		LOGI("Module_SubscribeEvent /Alarm/Subscribe/Status?AlarmName=Vdiagnose %d!\n",s_network_event.nSubscribeId[3]);
	}	
	
    ret = Common_Thread_Create(&s_network_event.s_alarm_thread,"AlarmDoThread",0,0,AlarmDoThread,NULL);
	if(ret != 0)
	{
		LOGE("create arp thread fail! ret=%d\n",ret);
		return -1;
	}
	
	return 0;
}

int NetWork_Event_Stop()
{
    int ret = 0;
	unsigned int i = 0;
	
	s_network_event.s_alarm_exit = 1;
	Common_Thread_Destroy(&s_network_event.s_alarm_thread);
	
	ret = Module_UnRegisterSubscribe(NetWork_RestComm_GetModuleHdl(),(S8 *)"/Network/Subscribe");
	if(ret != 0)
	{	
		LOGE("resgister subscribe fail!\n");
		return -1;
	}
	
	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_event.nSubscribeId);i++)
	{
		if(s_network_event.nSubscribeId[i] > 0)
		{
			Module_UnSubscribeEvent(NetWork_RestComm_GetModuleHdl(),s_network_event.nSubscribeId[i]);
			s_network_event.nSubscribeId[i] = 0;
		}	
	}
	
	return 0;
}

int NetWork_Event_Init()
{			
    OVFS_CLR_ARG(s_network_event);
	OVFS_CLR_ARG(s_network_eventSubInfo);
	pthread_mutex_init(&s_network_subLock,NULL);

	s_network_eventSubInfo[0].subUri = Common_StrDup((S8 *)"/Network/Subscribe/NetOut",__FUNCTION__,__LINE__);
	s_network_eventSubInfo[1].subUri = Common_StrDup((S8 *)"/Network/Subscribe/NetIpConflict",__FUNCTION__,__LINE__);	

	return 0;
}

int NetWork_Event_Destroy()
{	
	unsigned int i = 0;
	
	for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_eventSubInfo);i++)
	{
		if(NULL != s_network_eventSubInfo[i].subUri)
		{
			Common_Free(s_network_eventSubInfo[i].subUri,__FUNCTION__,__LINE__);
		}	
	}
	
	pthread_mutex_destroy(&s_network_subLock);
	
	return 0;
}

