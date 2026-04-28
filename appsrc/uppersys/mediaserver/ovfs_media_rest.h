/*
 * ovfs_media_rest.h
 *
 *  Created on: 2017年2月28日
 *      Author: eric
 */

#ifndef OVFS_MEDIA_REST_H_
#define OVFS_MEDIA_REST_H_

#include <cjson.h>
#include <libcommon_api.h>
#include <rtspserver_v2.h>
#include "ovfs_media.h"

/* MEDIA REST ERROR CODE*/
#define EC_MEDIA_REST_BASE                               -93000
#define EC_MEDIA_REST_UNKNOWN                            (EC_MEDIA_REST_BASE-1)
#define EC_MEDIA_REST_UNKNOWN_STR                        "Input parameter Invalid"
#define EC_MEDIA_REST_PARAM_INVALID                      (EC_MEDIA_REST_BASE-2)
#define EC_MEDIA_REST_PARAM_INVALID_STR                  "Input parameter Invalid"
#define EC_MEDIA_REST_NO_METHOD                          (EC_MEDIA_REST_BASE-3)
#define EC_MEDIA_REST_NO_METHOD_STR                      "No such method"
#define EC_MEDIA_REST_NO_URI                             (EC_MEDIA_REST_BASE-4)
#define EC_MEDIA_REST_NO_URI_STR                         "No such Uri"
#define EC_MEDIA_REST_OP_FAILED                          (EC_MEDIA_REST_BASE-5)
#define EC_MEDIA_REST_OP_FAILED_STR                      "Operation failed"

int RestMedia_Init(MQ_HANDLE_H mqHandle);
int RestMedia_Uninit();
int RestMedia_LoadCfg(Common_cJSON_T **cfg);
int RestMedia_RestoreCfg(Common_cJSON_T **cfg);
int RestMedia_SaveCfg(Common_cJSON_T *cfg);
int RestMedia_RequestStreamOpen(MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestStreamClose(int streamHandle,MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestStreamRead(int streamHandle, int index, int *rindex, void **data, int *dataSize, int *restCnt, int timeout);
int RestMedia_RequestStreamRelease(int streamHandle);
int RestMedia_RequestStreamWrite(int streamHandle,int index, void *data, int size);
int RestMedia_RequestVencType(MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestAudioType(MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestStreamAdd(int *streamQueueId,MEDIA_REQ_STREAM_INFO_T *reqInfo,int channel,int streamId);
int RestMedia_RequestStreamDec(int *streamQueueId,MEDIA_REQ_STREAM_INFO_T *reqInfo,int channel,int streamId);
int RestMedia_RequestStreamAddOrDecHandle(int *streamQueueId,MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestRecordControl(int streamHandle, int cmd, void *cmdData);
int RestMedia_RegistAuthNode(MEDIA_AUTH_INFO_T *authNode);
int RestMedia_UnregistAuthNode(int sessionId);
void *RestMedia_SearchAuthNode(int sessionId);
int RestMedia_WriteEventLog(int majorType, int minorType, int channel, char *ipAddr);
int RestMedia_CheckPlayBackTime(Ants_RtspDayTime *startTime, Ants_RtspDayTime *stopTime);
int RestMedia_RequestNaluType(MEDIA_REQ_STREAM_INFO_T *reqInfo, void **sps, int *spsSize, void **pps, int *ppsSize);
char *RestMedia_GetCustomType();
int RestMedia_SendMediaDisconnectEvent();
int RestMedia_RequestVideoIFrame(MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestBoardAbility(BOARD_ABILITY_T *ability, MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMedia_RequestBoardCodecTempChange(int changeOrBack, MEDIA_REQ_STREAM_INFO_T *reqInfo);
int RestMeida_RequestCoreVersion(HTTP_PUSH_CORE_VERSION_T *coreVersion);
int RestMedia_Request(cJSON_Struct *pRequire, cJSON_Struct **pResponce);
#endif /* OVFS_MEDIA_REST_H_ */
