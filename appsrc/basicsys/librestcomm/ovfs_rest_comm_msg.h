#ifndef _REST_COMM_MSG_
#define _REST_COMM_MSG_

#ifdef __cplusplus
extern "C"{
#endif

#include"cjson.h"

int RestComm_PraseInputJson(Common_cJSON_T* inputData,char** method,char** uri,Common_cJSON_T** inData);

int RestComm_GetUriAndQue(const char* srcUri,char** uriStr, char** conditionStr);

Common_cJSON_T* RestComm_GenerateOutParam(int retCode,Common_cJSON_T* outData,const char* errStr);

Common_cJSON_T* RestComm_GenerateInParam(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inData);

#ifdef __cplusplus
};
#endif
#endif
                                                  
