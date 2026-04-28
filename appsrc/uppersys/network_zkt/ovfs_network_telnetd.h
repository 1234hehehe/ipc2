#ifndef _OVFS_NETWORK_TELNETD_H_
#define _OVFS_NETWORK_TELNETD_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"

S32 NetWork_Telnetd_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault);
S32 NetWork_Telnetd_Destroy();
S32 NetWork_Get_Telnetd_Json(cJSON_Struct *parentItem);
S32 NetWork_Put_Telnetd_Json(cJSON_Struct *parentItem);

#ifdef __cplusplus
}
#endif

#endif
