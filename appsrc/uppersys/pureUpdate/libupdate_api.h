#ifndef __LIBUPDATE_API_H__
#define __LIBUPDATE_API_H__
#ifdef WIN32
#define LIBUPDATE_API __declspec(dllexport)
#else
#define LIBUPDATE_API
#endif
#include "libcommon_api.h"
#ifdef __cplusplus
extern "C"{
#endif
#define UPDATE_BROADCAST_HANDLE void *
LIBUPDATE_API S32 Update_udp_SendPkt(S32 nSocket,char *pToIpv4,S32 nToPort,U32 bResp,U16 uMsgId,U32 *pSeq,void *pData,S32 nLen);
LIBUPDATE_API S32 Update_Tcp_Require(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut);
LIBUPDATE_API S32 Update_Udp_Require(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut);
typedef S32 (*Update_Broadcast_Callback_Def)(UPDATE_BROADCAST_HANDLE handle,S8 *szFromIP,S32 nFromPort,cJSON_Struct *pResult,void *pUserData);
LIBUPDATE_API S32 Update_Broadcast_Start(UPDATE_BROADCAST_HANDLE *pHandle,S32 nPort,cJSON_Struct *pInParams,S8 *pBuffer,S32 nBufferSize,S32 nTimes,S32 nIntervalSec,Update_Broadcast_Callback_Def fxn,void *pUserData);


/*
	nTimes:次数,0-无限次，> 0有限次数
	nIntervalSec:> 0 间隔时间(秒)
*/

LIBUPDATE_API S32 Update_Broadcast_Stop(UPDATE_BROADCAST_HANDLE *pHandle);

LIBUPDATE_API S32 Update_Udp_Start(UPDATE_BROADCAST_HANDLE *pHandle,char *szDomain,S32 nPort,cJSON_Struct *pInParams,S8 *pBuffer,S32 nBufferSize,S32 nTimes,S32 nIntervalSec,Update_Broadcast_Callback_Def fxn,void *pUserData);
LIBUPDATE_API S32 Update_Udp_Send(UPDATE_BROADCAST_HANDLE *pHandle,cJSON_Struct *pInParams,S8 *pBuffer,S32 nBufferSize);
LIBUPDATE_API S32 Update_Udp_Stop(UPDATE_BROADCAST_HANDLE *pHandle);
#ifdef __cplusplus
}
#endif

#endif
