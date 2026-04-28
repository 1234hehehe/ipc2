/*
 * ants_web.h
 *
 *  Created on: 2016年8月1日
 *      Author: eric
 */
#ifndef OVFS_LOG_H_
#define OVFS_LOG_H_

#include "sqlite3mgr.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MATHOD_GET		0
#define MATHOD_PUT		1
#define MATHOD_POST	2
#define MATHOD_DELETE	3

#define OVFS_MAX_LOGIN_USER   64

#define OVFS_LOGINHANDLE_BITSIZE  12
#define OVFS_LOGINHANDLE_BITMASK  0x0FFF

#define OVFS_SUBHANDLE_BITSIZE  12
#define OVFS_SUBHANDLE_BITMASK  0x0FFF

#define OVFS_RAND_BITSIZE  7
#define OVFS_RAND_BITMASK  0x07F

/* 操作 */
//主类型

#define MAJOR_OPERATION					0x1
#define MAJOR_SETCONFIG                 0x2

//次类型
#define MINOR_FACEDETECT                0xd     /* 人脸检测 */
#define MINOR_FACERECOGNITION           0x1d     /* 人脸识别检测 */
#define MINOR_PERSONDETECT              0x1f     /* 人形检测 */



#define MINOR_START_DVR					0x41	/* 开机 */
#define MINOR_STOP_DVR					0x42	/* 关机 */
#define MINOR_STOP_ABNORMAL				0x43	/* 异常关机 ,如段错误*/
#define MINOR_REBOOT_DVR                0x44    /* 本地重启设备 */
#define MINOR_STOP_DVR_ILLEGAL          0x45    /* 非法关机,如关掉电源*/

#define MINOR_REMOTE_REBOOT				0x7b	/* 远程重启 */
#define MINOR_REMOTE_STOP               0x83    /* 远程关机 */

#define ARRAYSIZE(ARRAY)	(int)(sizeof(ARRAY)/sizeof(ARRAY[0]))

#define ovfs_make_result(CODE, DES, DATA, OUTDATA) MakeResult(__FILE__,__FUNCTION__,__LINE__,CODE, DES, DATA, OUTDATA)

#define  ovfs_print_json(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		LOGI("%s\n",ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)

typedef int  (*OVFS_GET_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_PUT_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_POST_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_DELETE_METHOD)(void *,void *,void *,void**);

typedef struct
{
	OVFS_GET_METHOD ovfs_get_method;
	OVFS_PUT_METHOD ovfs_put_method;
	OVFS_POST_METHOD ovfs_post_method;
	OVFS_DELETE_METHOD ovfs_delete_method;
}OVFS_REST_METHOD,*POVFS_REST_METHOD;

typedef struct
{
	S32 dwCreateTime;			//句柄创建时模块运行的时间(单位:s)
	S32 dwHandle;
	S32 dwLogHandle;
    S32 dwLogType;
	S32 nLogCnt;
}OVFS_USER_INFO,*POVFS_USER_INFO;

typedef struct
{
	int nUserCount;
	OVFS_USER_INFO *pUserInfo[OVFS_MAX_LOGIN_USER];
	SQLITE3MGR	*pSqlMgr;
	Common_Lock_T tLock;
	Common_Thread_T hThread;
}OVFS_LOG_MGR,*POVFS_LOG_MGR;

int ovfs_init_log(ModuleHandle_T hModuleHandle);
int ovfs_deal_log_res(int iMethod, char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);

#ifdef __cplusplus
}
#endif

#endif /* OVFS_LOG_H_ */
