#ifndef _NETWORK_REST_H_
#define _NETWORK_REST_H_

#include"libcommon_api.h"
#include"libmodule_api.h"
#include"cjson.h"


#ifdef __cplusplus
extern "C"{
#endif

typedef struct
{
	void*			funcCall;
	ModuleHandle_T*	moduleHdl;
}NETWORK_REST_INIT_T;

int NetWork_Rest_Init(NETWORK_REST_INIT_T* initInfo);
int NetWork_Rest_Destroy();
int NetWork_Rest_Get_NetworkItem_OfRootUri(Common_cJSON_T** networkRoot);
int NetWork_Rest_Get(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int NetWork_Rest_Put(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int NetWork_Rest_Post(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int NetWork_Rest_Delete(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);

const char* NetWork_Rest_GetErrString(int errorCode);

#ifdef __cplusplus
};
#endif

#endif
                                                  
