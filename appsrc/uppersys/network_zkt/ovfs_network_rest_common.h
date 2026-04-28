#ifndef _NETWORK_REST_COMMON_H_
#define _NETWORK_REST_COMMON_H_

#include"libcommon_api.h"
#include"libmodule_api.h"
#include"cjson.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
	OPS_TYPE_GET = 0,
	OPS_TYPE_POST,
	OPS_TYPE_PUT,
	OPS_TYPE_DELETE
}NETWORK_REST_OPS_TYPE_E;

#define NETWORK_NAME	"NetWork"
#define FUNCLIST_NAME	"_FUNC_LIST_"

#define ETH_IDX_NAME 	"ETH"



typedef int (*NETWORK_REST_FUNC_F)(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out);
typedef int (*NETWORK_REST_FUNC_CALL)(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T* outParam);
typedef int (*NETWORK_REST_PUT_F)(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);

typedef struct
{
	ModuleHandle_T*		        moduleHdl;
	NETWORK_REST_FUNC_CALL		callFunc;
	NETWORK_REST_PUT_F			put;
	NETWORK_REST_PUT_F			get;
}NETWORK_REST_CB_TALBE_T;

#define NETWORK_NODE_FLAGS_IS_CFG_DIR	(0x1<<0)
#define NETWORK_NODE_FLAGS_IS_CFG_NODE  (0x1<<1)

typedef struct
{
	const char* 	        describtion;
	const char*             label;

	NETWORK_REST_FUNC_F 	get;
	NETWORK_REST_FUNC_F 	post;
	NETWORK_REST_FUNC_F 	put;
	NETWORK_REST_FUNC_F 	fdelete;
	//NETWORK_REST_FUNC_F     cfgChanged;
	unsigned int	        flags[4];
}NETWORK_REST_NODE_ATTR_T;

typedef struct
{
	int			errorCode;
	const char* errorString;
}NETWORK_REST_ER_T;

#define EC_NETWORK_REST_BASE                                        -8000
#define EC_NETWORK_REST_METHOD_NOT_FOUND                            (EC_NETWORK_REST_BASE-1)
#define EC_NETWORK_REST_METHOD_NOT_FOUND_STR						"method not supported on this Uri."
#define EC_NETWORK_REST_URI_PATH_NOT_EXIST                          (EC_NETWORK_REST_BASE-2)
#define EC_NETWORK_REST_URI_PATH_NOT_EXIST_STR						"URI path is not existed."

#define EC_NETWORK_REST_OPERATION_FAIL	                            (-1)
#define EC_NETWORK_REST_OPERATION_FAIL_STR					   		"Operation fail."


int NetWork_RestComm_GetUriList(const char* uri,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out);
int NetWork_RestComm_GetIdxFormString(const char* string,int* eth);
char* NetWork_RestComm_GetUriStrFromJson(const Common_cJSON_T* jsonObj);

const char* NetWork_RestComm_GetErrString(int errorCode);

int NetWork_RestComm_Init(NETWORK_REST_CB_TALBE_T* cbTable);
int NetWork_RestComm_Destroy();

int NetWork_RestComm_FuncCall(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T* outParam);

int NetWork_RestComm_MargeJsonWithUri(Common_cJSON_T* saveObj,const char* uri,Common_cJSON_T* param);

ModuleHandle_T NetWork_RestComm_GetModuleHdl();

Common_cJSON_T* NetWork_RestComm_GetItemWithUri(Common_cJSON_T* obj,const char* uri);

int NetWork_RestComm_StartCfgChangedTimer(void);

Common_cJSON_T* NetWork_RestComm_AddPathToTree(const char* uri,Common_cJSON_T* tree,const char* desc,const char* label,NETWORK_REST_FUNC_F get,NETWORK_REST_FUNC_F put,NETWORK_REST_FUNC_F post,NETWORK_REST_FUNC_F fdelete);

S32 NetWork_RestComm_LoadConfig(char *szPath,cJSON_Struct **pConfig);
S32 NetWork_RestComm_SaveConfig(char *szPath,cJSON_Struct *pConfig);

#ifdef __cplusplus
};
#endif

#endif

