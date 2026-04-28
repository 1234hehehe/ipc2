#ifndef _REST_COMM_SUB_
#define _REST_COMM_SUB_

#ifdef __cplusplus
extern "C"{
#endif

#include"libcommon_api.h"
#include"libmodule_api.h"
#include"cjson.h"

typedef struct
{
	char* subUri;

	int id[16];
}REST_SUB_INFO_T;

int RestComm_SetSubId(REST_SUB_INFO_T* subInfo, int offset,int id);
int RestComm_UnSetSubId(REST_SUB_INFO_T* subInfo,int offset,int id);
int RestComm_CheckSubId(REST_SUB_INFO_T* subInfo,int offset,int id);
int RestComm_FindMatchEventSubUri(REST_SUB_INFO_T* subInfo,int offset,const char* uri);
int RestComm_SendSubInfo(ModuleHandle_T* mdHdl,REST_SUB_INFO_T* subInfo,int offset,Common_cJSON_T* info);
#ifdef __cplusplus
};
#endif
#endif
