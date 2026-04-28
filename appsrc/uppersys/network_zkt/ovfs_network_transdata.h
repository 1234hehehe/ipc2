#ifndef _OVFS_NETWORK_TRANSDATA_H_
#define _OVFS_NETWORK_TRANSDATA_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"

S32 NetWork_TransData_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault);
S32 NetWork_TransData_Destroy();
S32 NetWork_TransData_CheckStatus();
S32 NetWork_Get_TransData_Json(cJSON_Struct *parentItem);
S32 NetWork_Put_TransData_Json(cJSON_Struct *parentItem);
S32 NetWork_TransData_Start(int status);
S32 NetWork_Put_DoTransData_Json(cJSON_Struct *parentItem);

#ifdef __cplusplus
}
#endif

#endif
