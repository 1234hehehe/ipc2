#ifndef _OVFS_NETWORK_WIFI_H_
#define _OVFS_NETWORK_WIFI_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"


int NetWork_Wifi_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault);
int NetWork_Config_STA_Json(cJSON_Struct *parentItem);
int NetWork_Get_WifiWorkStatus_Json(cJSON_Struct *parentItem);
int NetWork_Get_WifiWorkMode_Json(cJSON_Struct *parentItem);
int NetWork_Put_WifiWorkStatus_Json(cJSON_Struct *parentItem);
int NetWork_Get_WifiSTAList_Json(cJSON_Struct *parentItem);
int NetWork_Get_WifiAPList_Json(cJSON_Struct *parentItem);
int NetWork_Get_WifiScanList_Json(cJSON_Struct *parentItem);
int NetWork_Put_AddAP_Json(cJSON_Struct *parentItem);
int NetWork_Put_DelAP_Json(cJSON_Struct *parentItem);


#ifdef __cplusplus
}
#endif

#endif

