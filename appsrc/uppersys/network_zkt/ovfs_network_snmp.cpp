/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : ovfs
 *	描    述 : snmp
 *	源 文 件 : ovfs_network_snmp.cpp
 *	作    者 : 舒适
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2017/6/24
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ovfs_network_snmp.h"

#define SNMP_NAME_LEN			        32      //用户名长度
#define SNMP_DESC_LEN_64				64
#define MAX_CFG_CHANNUM_IPC             1
#define MAX_CFG_ALARMNUM_IPC            2

typedef struct
{
    S8  chDescription[32];
	U32 dwAlarmStatus;
	U32 dwAlarmTime;
	U32 dwAlarmDuration;
}NETWORK_SNMP_PARAM;

typedef struct
{
	U32		dwSize;							//!结构长度
	U8		byEnable;						//!0-禁用SNMP，1-表示启用SNMP
	U8		byRes1[3];						//!保留
	U16		wVersion;						//!snmp 版本  v1 = 1, v2 =2, v3 =3，设备目前不支持 v3
	U16		wServerPort;					//!snmp消息接收端口，默认 161
	U8		byReadCommunity[SNMP_NAME_LEN];		//!读共同体，最多31,默认"public"
	U8		byWriteCommunity[SNMP_NAME_LEN];		//!写共同体,最多31 字节,默认 "private"
	U8		byTrapHostIP [SNMP_DESC_LEN_64];		//!自陷主机ip地址描述，支持IPV4 IPV6和域名描述    
	U16		wTrapHostPort;					//!trap主机端口
	U8		bySendCount;					//!发送次数
	U8		bySendInterval;					//!发送时间间隔
	U8		byRes2[100];					//!保留
}NETWORK_SNMP_T;

static NETWORK_SNMP_T s_network_snmp_info;
static U32 s_alarmStartTime[MAX_CFG_ALARMNUM_IPC][MAX_CFG_CHANNUM_IPC] = {0};


static U32 SNMPTime(S8 *btime)
{
   Common_Time_T c_time;
   time_t c_time_t;
   S32 nYear,nMonth,nDay,nHour,nMin,nSec;
   	
   if (NULL == btime)
   {
       return 0;
   }

   memset(&c_time,0,sizeof(c_time));

   sscanf(btime, "%04d%02d%02d%02d%02d%02d", &nYear, &nMonth, &nDay,
											   &nHour, &nMin, &nSec);
   c_time.year = nYear;
   c_time.month = nMonth;
   c_time.day = nDay;
   c_time.hour = nHour;
   c_time.min = nMin;
   c_time.sec = nSec;

 
   Common_Common2LinuxTime(&c_time, &c_time_t);
   
   return (U32)c_time_t;
}

static S32 SNMPSendThread(Common_Thread_T hThreadHandle,void *data)
{
    S32 i = 0;
	S8 szCommand[128];
    NETWORK_SNMP_PARAM *pSnmpParam = (NETWORK_SNMP_PARAM*)data;

	if (NULL == pSnmpParam)
	{
        return -1;
	}
	
	snprintf(szCommand, sizeof(szCommand), "snmpsend.sh %s %d %d %s %s %d %d",
		     (char*)s_network_snmp_info.byTrapHostIP, s_network_snmp_info.wTrapHostPort, pSnmpParam->dwAlarmTime, pSnmpParam->chDescription, pSnmpParam->dwAlarmStatus==1?"ON":"OFF", pSnmpParam->dwAlarmTime, pSnmpParam->dwAlarmDuration);

	if(s_network_snmp_info.bySendInterval < 2)
	{
		s_network_snmp_info.bySendInterval = 2;
	}

	if(s_network_snmp_info.bySendCount < 1)
	{
		s_network_snmp_info.bySendCount = 1;
	}

	for(i = 0; i < s_network_snmp_info.bySendCount; i++)
	{
		Common_System(szCommand);
		Common_Sleep(s_network_snmp_info.bySendInterval, 0);
	}

	Common_Free(pSnmpParam,__FUNCTION__,__LINE__);
	
	return 0;
}
	
S32 NetWork_Send_SNMP(cJSON_Struct *parentItem)
{
    Common_Thread_T hThread_SNMP;
	 
    if (1 == s_network_snmp_info.byEnable && strlen((S8 *)s_network_snmp_info.byTrapHostIP) > 0)
    {
        cJSON_Struct *pArray;

		pArray = Common_Json_GetItem(parentItem,-1,"/Data/ResList");
		if (NULL != pArray)
		{
		    S32 nArrayNum = 0,nWhich = 0,nIntValue = -1;
			S8 *pStringValue;
			nArrayNum = Common_Json_ArraySize(pArray);
			for (nWhich = 0; nWhich < nArrayNum; nWhich++)
			{
		        NETWORK_SNMP_PARAM *pSnmpParam = NULL;
		        pSnmpParam = (NETWORK_SNMP_PARAM *)Common_Malloc(sizeof(NETWORK_SNMP_PARAM),0,__FUNCTION__,__LINE__);
				if (NULL != pSnmpParam)
				{
				   memset(pSnmpParam, 0, sizeof(NETWORK_SNMP_PARAM));
				   Common_Json_GetAttrValue(pArray,nWhich,"AlarmName",NULL,&pStringValue,NULL,NULL);
				   if (NULL != pStringValue)
				   {
                       Common_Strncpy(pSnmpParam->chDescription,pStringValue,sizeof(pSnmpParam->chDescription));
				   }

				   Common_Json_GetAttrValue(pArray,nWhich,"AlarmType",NULL,NULL,&nIntValue,NULL);
				   if (2 == nIntValue || 3 == nIntValue)
				   {
				       U32 *pStartTime = &s_alarmStartTime[nIntValue - 2][0];
					   Common_Json_GetAttrValue(pArray,nWhich,"Status",NULL,NULL,&nIntValue,NULL);
					   if (-1 != nIntValue)
					   {
                           pSnmpParam->dwAlarmStatus = nIntValue;
					   }
					   
					   if (1 == pSnmpParam->dwAlarmStatus)
					   {
					       Common_Json_GetAttrValue(pArray,nWhich,"StartTime",NULL,&pStringValue,NULL,NULL);
						   *pStartTime = SNMPTime(pStringValue);
						   pSnmpParam->dwAlarmTime = *pStartTime;
						   pSnmpParam->dwAlarmDuration = 0;
					   }
					   else if (0 == pSnmpParam->dwAlarmStatus)
					   {
					       U32 tmp_time;
					       Common_Json_GetAttrValue(pArray,nWhich,"StopTime",NULL,&pStringValue,NULL,NULL);
						   tmp_time = SNMPTime(pStringValue);
						   pSnmpParam->dwAlarmTime = tmp_time;
						   pSnmpParam->dwAlarmDuration = tmp_time - *pStartTime > 0 ? tmp_time - *pStartTime : 0;
					   }
					   
				       Common_Thread_Create(&hThread_SNMP,__FUNCTION__,0,COMMON_THREAD_CREATEFLAG_DETACH,SNMPSendThread,pSnmpParam);
				   }
			    }
			}
		}
	}

	return 0;
}

S32 NetWork_Get_SNMP_Json(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
        return -1;
	}
	
	Common_Json_SetAttrValue(parentItem,-1,"SNMPEnable",Common_Json_Type_Number,NULL,s_network_snmp_info.byEnable,0);
	Common_Json_SetAttrValue(parentItem,-1,"SNMPVersion",Common_Json_Type_Number,NULL,s_network_snmp_info.wVersion,0);
	Common_Json_SetAttrValue(parentItem,-1,"ServerPort",Common_Json_Type_Number,NULL,s_network_snmp_info.wServerPort,0);	
	Common_Json_SetAttrValue(parentItem,-1,"ReadCommunity",Common_Json_Type_String,(S8 *)s_network_snmp_info.byReadCommunity,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"WriteCommunity",Common_Json_Type_String,(S8 *)s_network_snmp_info.byWriteCommunity,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"TrapHostIP",Common_Json_Type_String,(S8 *)s_network_snmp_info.byTrapHostIP,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"TrapHostPort",Common_Json_Type_Number,NULL,s_network_snmp_info.wTrapHostPort,0);
	Common_Json_SetAttrValue(parentItem,-1,"SendCount",Common_Json_Type_Number,NULL,s_network_snmp_info.bySendCount,0);
	Common_Json_SetAttrValue(parentItem,-1,"SendInterval",Common_Json_Type_Number,NULL,s_network_snmp_info.bySendInterval,0);	

	return 0;
}

S32 NetWork_Put_SNMP_Json(cJSON_Struct *parentItem)
{
	S8 *pStringValue;
	S32 nIntValue;
	
	if (NULL == parentItem)
	{
	    LOGE("%s: Can't find %s\n",__FUNCTION__,"SNMP");
		return -1;
	}	
	
    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"SNMPEnable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        s_network_snmp_info.byEnable = nIntValue;
	}

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"SNMPVersion",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        s_network_snmp_info.wVersion = nIntValue;
	}

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"ServerPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        s_network_snmp_info.wServerPort = nIntValue;
	}	

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"ReadCommunity",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
        Common_Strncpy((S8 *)s_network_snmp_info.byReadCommunity,pStringValue,sizeof(s_network_snmp_info.byReadCommunity));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"WriteCommunity",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
        Common_Strncpy((S8 *)s_network_snmp_info.byWriteCommunity,pStringValue,sizeof(s_network_snmp_info.byWriteCommunity));
	}
	
	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"TrapHostIP",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
        Common_Strncpy((S8 *)s_network_snmp_info.byTrapHostIP,pStringValue,sizeof(s_network_snmp_info.byTrapHostIP));
	}	
	
    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"TrapHostPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        s_network_snmp_info.wTrapHostPort = nIntValue;
	}	
	
    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"SendCount",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        s_network_snmp_info.bySendCount = nIntValue;
	}	

    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"SendInterval",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
        s_network_snmp_info.bySendInterval = nIntValue;
	}		
	
	return 0;
}

S32 NetWork_SNMP_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{	
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;
	
    COMMON_CLR_ARG(s_network_snmp_info);
	s_network_snmp_info.dwSize = sizeof(s_network_snmp_info);
	s_network_snmp_info.byEnable = 0;
	s_network_snmp_info.wServerPort = 161;
	s_network_snmp_info.bySendCount = 3;
	s_network_snmp_info.bySendInterval = 60;
	s_network_snmp_info.wTrapHostPort = 162;
	
	pJsonTmp = Common_Json_GetItem(pJson, -1, "NetApp/SNMP");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get SNMP info fail\n",__FUNCTION__, __LINE__);
	}	

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault, -1, "NetApp/SNMP");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get SNMP info fail\n",__FUNCTION__, __LINE__);
	} 
	
	NetWork_Put_SNMP_Json(pJsonTmpDefault);
	NetWork_Put_SNMP_Json(pJsonTmp);
	
	return 0;
}

S32 NetWork_SNMP_Destroy()
{		
	return 0;
}

