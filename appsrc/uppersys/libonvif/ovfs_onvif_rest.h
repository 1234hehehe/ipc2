/*
 * ovfs_media_rest.h
 *
 *  Created on: 2017骞?2鏈?28鏃?
 *      Author: eric
 */

#ifndef OVFS_ONVIF_REST_H_
#define OVFS_ONVIF_REST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <cjson.h>
#include <libcommon_api.h>

#include "ovfs_onvif.h"

/* ONVIF REST ERROR CODE*/
#define EC_ONVIF_REST_BASE                               -99000
#define EC_ONVIF_REST_UNKNOWN                            (EC_ONVIF_REST_BASE-1)
#define EC_ONVIF_REST_UNKNOWN_STR                        "Input parameter Invalid"
#define EC_ONVIF_REST_PARAM_INVALID                      (EC_ONVIF_REST_BASE-2)
#define EC_ONVIF_REST_PARAM_INVALID_STR                  "Input parameter Invalid"
#define EC_ONVIF_REST_NO_METHOD                          (EC_ONVIF_REST_BASE-3)
#define EC_ONVIF_REST_NO_METHOD_STR                      "No such method"
#define EC_ONVIF_REST_NO_URI                             (EC_ONVIF_REST_BASE-4)
#define EC_ONVIF_REST_NO_URI_STR                         "No such Uri"
#define EC_ONVIF_REST_OP_FAILED                          (EC_ONVIF_REST_BASE-5)
#define EC_ONVIF_REST_OP_FAILED_STR                      "Operation failed"

typedef enum
{
    REST_GET,
    REST_PUT,
    REST_POST,
    REST_DELETE,
} OVFS_RESTMETHOD_E;

int RestOnvif_Init(MQ_HANDLE_H mqHandle);
int RestOnvif_Uninit();
int RestOnvif_LoadCfg(ONVIF_MGR_CFG_T *cfg);
int RestOnvif_SaveCfg(ONVIF_MGR_CFG_T cfg);

int RestOnvif_PasswordCheck(ONVIF_AUTH_INFO_T *authResult);
int RestOnvif_RequestVersion(ONVIF_AUTH_INFO_T *auth, char *serialNumber,
                             char *firmwareVersion, char *hardwareId);
int RestOnvif_RequestCoreVersion(ONVIF_CORE_VERSION_T *coreVersion);
int RestOnvif_RequestGetTimeAll(ONVIF_TIME_ALL_T *timeAll);
int RestOnvif_RequestNetworkAttr(ONVIF_NETWORK_ATTR_T *networkAttr);
int RestOnvif_GetChanNameOsd(cJSON_Struct **outJsonChannel);
int RestOnvif_GetMultiNameOsd(cJSON_Struct **outJsonChannel);
int RestOnvif_GetDateTimeOsd(cJSON_Struct **outJsonTime);
int RestOnvif_RequestGetOsd(ONVIF_OSD_ATTR_T *osdAttr);
int RestOnvif_RequestSetOsd(ONVIF_OSD_ATTR_T *osdAttr);
int RestOnvif_RequestSetCustomOsd(ONVIF_OSD_ATTR_T *osdAttr);
int RestOnvif_RequestDeleteOsd(ONVIF_OSD_ATTR_T *osdAttr);
int RestOnvif_RequestSetMultiOsd(ONVIF_OSD_ATTR_T *osdAttr);
int RestOnvif_RequestImageAttr(ONVIF_IMAGE_SET_T *image);
int RestOnvif_SetImageAttr(int iChannel, ONVIF_IMAGE_SET_T *image);
int RestOnvif_RequestVideoEncoderCapability(int devNo, int channelNo,
        int streamNo, ONVIF_VideoEncoderCapability_T *video);

int RestOnvif_GetVideoEncoderCfg(int devNo, int channelNo, int streamNo,
                                 ONVIF_VideoEncoderCfg_T *video);
int RestOnvif_SetVideoEncoderCfg(int devNo, int channelNo, int streamNo,
                                 ONVIF_VideoEncoderCfg_T *video);
int RestOnvif_GetMultiCastCfg(int devNo, int channelNo, int streamNo,
                              ONVIF_MultiCast_T *pMultiCast);
int RestOnvif_SetMultiCastCfg(int devNo, int channelNo, int streamNo,
                              ONVIF_MultiCast_T *pMultiCast);

int RestOnvif_RequesNetInfo(ONVIF_NET_INFO_T *pNetInfo);
int RestOnvif_SetNetInfo(ONVIF_NET_INFO_T *pNetInfo);

const ONVIF_OSD_MAP_T *RestOnvif_ParserDate(char *formartStr, int formartId);
const ONVIF_OSD_MAP_T *RestOnvif_ParserHour(char *formartStr, int formartId);
int RestOnvif_RequestWebServer(ONVIF_WEB_ATTR_T *webAttr);
int RestOnvif_RequestViAttr(ONVIF_VI_ATTR_T *viAttr);
int RestOnvif_RequestMotionAttr(ONVIF_MOTION_ATTR_T *pMotionAttr);
int RestOnvif_SetMotionAttr(ONVIF_MOTION_ATTR_T *pMotionAttr);

int RestOnvif_RequestSetTimeAll(ONVIF_TIME_ALL_T *timeAll);
int RestOnvif_GetCfgTimeZone(char *tzName, int nameLen, int *tzOffset,
                             char *tzDstName, int dstLen);
int RestOnvif_SetCfgTimeZone(char *tzName, int tzOffset, char *tzDstName);
int RestOnvif_IsDefaultPassword();
int RestOnvif_IsDefaultIp();
int RestOnvif_RequestGetMultiOsd(ONVIF_OSD_ATTR_T *osdAttr);
int RestOnvif_RequestGetCustomOsd(ONVIF_OSD_ATTR_T *osdAttr);

int RestOnvif_RequestUri(char *uri,OVFS_RESTMETHOD_E method);

int RestOnvif_RequestGetPresets(ONVIF_PTZ_PRESET_INFO_T **presetsAttr,int *presetsNum);
int RestOnvif_RequestSetPreset(int *presetsNum);
int RestOnvif_RequestGotoPreset(int presetsNum);
int RestOnvif_RequestDelPreset(int presetsNum);
int RestOnvif_SetScope(char *nameP);
int RestOnvif_RequestBoardLensZoom(int type,int speed);

int RestOnvif_ReqIFrame(int devNo, int channelNo, int streamNo);
int RestOnvif_RequestReboot();
int RestOnvif_RequestSnap(int devNo, int channelNo, int streamNo,char *PicName);
int RestOnvif_RequestGetZoomCfg(float *zoom, int *max);
int RestOnvif_RequestSetZoomCfg(float zoom);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* OVFS_ONVIF_REST_H_ */
