/*
 * access_rest.h
 *
 *  Created on: 2017年4月17日
 *      Author: qinjx
 */
#ifndef ACCESS_REST_H_
#define ACCESS_REST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define MATHOD_GET		0
#define MATHOD_PUT		1
#define MATHOD_POST	2
#define MATHOD_DELETE	3

#define ERROR_INVALID_PARAM	-1
#define ERROR_INVALID_PWD	-2


#define OVFS_MAX_LOGIN_USER   256

#define OVFS_LOGINHANDLE_BITSIZE  12
#define OVFS_LOGINHANDLE_BITMASK  0x0FFF

#define OVFS_SUBHANDLE_BITSIZE  8
#define OVFS_SUBHANDLE_BITMASK  0x0FF

#define OVFS_MODULE_BITSIZE  4
#define OVFS_MODULE_BITMASK  0x0F

#define OVFS_RAND_BITSIZE  7
#define OVFS_RAND_BITMASK  0x7F

#define OVFS_CFG_FLAG		0x1000
#define OVFS_ONLINE_FLAG	0x2000
#define OVFS_IPTABLE_FLAG	0x4000

#define ARRAYSIZE(ARRAY)	(int)(sizeof(ARRAY)/sizeof(ARRAY[0]))

#define ovfs_make_result(CODE, DES, DATA, OUTDATA) MakeResult(__FILE__,__FUNCTION__,__LINE__,CODE, DES, DATA, OUTDATA)

typedef int  (*OVFS_GET_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_PUT_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_POST_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_DELETE_METHOD)(void *,void *,void *,void**);

typedef struct
{
	int streamNum;
}OVFS_CHANNEL_ABILITY;

typedef struct
{
	int chanNum;
	OVFS_CHANNEL_ABILITY **pChanAbility;
}OVFS_DEV_ABILITY;

typedef struct
{
	int devNum;
	int totalChanNum;
	int totalStreamNum;
	OVFS_DEV_ABILITY **pDevAbility;
}OVFS_ABILITY,*POVFS_ABILITY;

typedef struct
{
	OVFS_GET_METHOD ovfs_get_method;
	OVFS_PUT_METHOD ovfs_put_method;
	OVFS_POST_METHOD ovfs_post_method;
	OVFS_DELETE_METHOD ovfs_delete_method;
}OVFS_REST_METHOD,*POVFS_REST_METHOD;

S32 access_loadCfg(ModuleHandle_T hModuleHandle);
S32 access_saveCfg(ModuleHandle_T hModuleHandle);
S32 access_CreateDefCfg(ModuleHandle_T hModuleHandle);
OVFS_ABILITY * get_channel_ability();
int request_channel_ability(ModuleHandle_T hModuleHandle);
int init_access_res(ModuleHandle_T hModuleHandle);
int get_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
int put_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
int post_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
int delete_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
S32 access_get_debug();

#ifdef __cplusplus
}
#endif

#endif /* ACCESS_REST_H_ */
