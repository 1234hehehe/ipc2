#ifndef _OVFS_NETWORK_FTP_H_
#define _OVFS_NETWORK_FTP_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"

S32 NetWork_Ftp_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault);
S32 NetWork_Ftp_Destroy();
S32 NetWork_Get_Ftp_Json(cJSON_Struct *parentItem);
S32 NetWork_Put_Ftp_Json(cJSON_Struct *parentItem);
S32 NetWork_Put_DoFtp_Json(cJSON_Struct *parentItem);
S32 NetWork_Get_Ftp_Status_Json(cJSON_Struct *parentItem);

#ifdef __cplusplus
}
#endif

#endif
