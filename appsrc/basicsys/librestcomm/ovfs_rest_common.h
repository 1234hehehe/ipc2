#ifndef _REST_COMMON_H_
#define _REST_COMMON_H_

#ifdef __cplusplus
extern "C"{
#endif

#include"libcommon_api.h"
#include"libmodule_api.h"
#include"cjson.h"
#include"ovfs_rest_comm_cfg.h"
#include"ovfs_rest_comm_list.h"
#include"ovfs_rest_comm_sub.h"

typedef enum
{
	OPS_TYPE_GET = 0,
	OPS_TYPE_POST,
	OPS_TYPE_PUT,
	OPS_TYPE_DELETE
}REST_OPS_TYPE_E;

#define BOARDSYS_NAME	"BoardSys"
#define FUNCLIST_NAME	"_FUNC_LIST_"

#define DEVICE_IDX_NAME 	"Device"
#define CHANNEL_IDX_NAME  	"Channel"
#define STREAM_IDX_NAME		"Stream"



typedef int (*REST_FUNC_F)(const char* uriString,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out);
typedef int (*REST_FUNC_CALL)(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T** outParam);
typedef int (*REST_PUT_F)(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);

typedef struct
{
	ModuleHandle_T*		moduleHdl;
	REST_FUNC_CALL		callFunc;
	REST_PUT_F			put;
	REST_PUT_F			get;
}REST_CB_TALBE_T;

#define NODE_FLAGS_IS_CFG_DIR	(0x1<<0)
#define NODE_FLAGS_IS_CFG_NODE  (0x1<<1)

typedef struct
{
	const char* 	describtion;
	const char*     label;

	REST_FUNC_F 	get;
	REST_FUNC_F 	post;
	REST_FUNC_F 	put;
	REST_FUNC_F 	delet;
	//REST_FUNC_F     cfgChanged;
	unsigned int	flags[4];
}REST_NODE_ATTR_T;

typedef struct
{
	int			errorCode;
	const char* errorString;
}REST_ER_T;

typedef struct
{
    char* resoStr;
    int               w;
    int               h;
}REST_RESO_STR_T;

#define EC_REST_BASE		-9000

#define EC_REST_METHOD_NOT_FOUND                            (EC_REST_BASE-16)
#define EC_REST_METHOD_NOT_FOUND_STR						"method not supported on this Uri."
#define EC_REST_URI_PATH_NOT_EXIST                          (EC_REST_BASE-17)
#define EC_REST_URI_PATH_NOT_EXIST_STR						"URI path is not existed."

#define EC_REST_SNAP_BUFF_RUN_OUT							(EC_REST_BASE-18)
#define EC_REST_SNAP_BUFF_RUN_OUT_STR						"Snap buff run out."

#define EC_REST_NO_READLY	                                   (EC_REST_BASE-19)
#define EC_REST_NO_READLY_STR			           "Rest is not readly.Try layer."

#define EC_REST_OPERATION_FAIL	                           (-1)
#define EC_REST_OPERATION_FAIL_STR					   		"Operation fail."



int RestComm_GetUriList(const char* uri,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out);
int RestComm_GetIdxFormString(const char* string,int* device,int* channel,int* stream);
char* RestComm_GetUriStrFromJson(const Common_cJSON_T* jsonObj);

const char* RestComm_GetErrString(REST_ER_T* errlist,int errorCode);

int RestComm_Init(REST_CB_TALBE_T* cbTable);
int RestComm_Destroy();

int RestComm_FuncCall(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T** outParam);

int RestComm_PutMyself(const char* uri,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int RestComm_GetMyself(const char* uri,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);

int RestComm_MargeJsonWithUri(Common_cJSON_T* saveObj,const char* uri,Common_cJSON_T* param);

ModuleHandle_T RestComm_GetModuleHdl(void);

Common_cJSON_T* RestComm_GetItemWithUri(Common_cJSON_T* obj,const char* uri);

int RestComm_StartCfgChangedTimer(void);

Common_cJSON_T* RestComm_AddPathToTree(const char* uri,Common_cJSON_T* tree,const char* desc,const char* label,REST_FUNC_F get,REST_FUNC_F put,REST_FUNC_F post,REST_FUNC_F delet);

int RestComm_LowerStr(const char* inString,char* outString,int size);

int RestComm_SaveCfgFilesImmediately();

int RestComm_RegisterRestore(Common_cJSON_T* obj,const char* label,int needRestart);

int RestComm_AddCfgNodeWithStr(const char* path,int reduceCnt);
int RestComm_AddCfgNode(Common_cJSON_T* obj,int reduceCnt);

int RestComm_RestoreCfg(const char* uri,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out);

int RestComm_CallRestFunc(const char* uriString,const char* uriContion,Common_cJSON_T* jsonObj,Common_cJSON_T* in,Common_cJSON_T* out,int type); //get-0 put-1 post-2 delet-3

int RestComm_ParseUriString(Common_cJSON_T* rootTree,const char* uriString,const char* uriCondition,Common_cJSON_T* in,Common_cJSON_T* out,int type); //get-0 put-1 post-2 delet-3


int RestComm_Get(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int RestComm_Put(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int RestComm_Post(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);
int RestComm_Delete(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out);

char* RestComm_GetResoStr(int w, int h);  //need free
#ifdef __cplusplus
};
#endif
#endif
                                                  
