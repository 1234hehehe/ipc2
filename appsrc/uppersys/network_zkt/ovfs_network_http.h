#ifndef _OVFS_NETWORK_HTTP_H_
#define _OVFS_NETWORK_HTTP_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"

S32 NetWork_HTTP_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault);
S32 NetWork_HTTP_Destroy();
S32 NetWork_Get_HTTP_Json(cJSON_Struct *parentItem);
S32 NetWork_Put_HTTP_Json(cJSON_Struct *parentItem);
S32 NetWork_Put_DoHTTP_Json(cJSON_Struct *parentItem);

#ifdef __cplusplus
}
#endif

#endif
