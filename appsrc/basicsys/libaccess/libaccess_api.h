#ifndef __LIBACCESS_API_H__
#define __LIBACCESS_API_H__
#include "libcommon_api.h"

#ifdef WIN32
#define LIBACCESS_API __declspec(dllexport)
#else
#define LIBACCESS_API
#endif
#define ACCESS_ERROR_TYPE_SUCC               0
#define ACCESS_ERROR_TYPE_INVALIDPARAM      -1 // 无效参数
#define ACCESS_ERROR_TYPE_UNINITED          -2 // 未初始化的
#define ACCESS_ERROR_TYPE_UNREGISTER        -3 // 失联
#define ACCESS_ERROR_TYPE_RUNNING           -4 // 多实例
#define ACCESS_ERROR_TYPE_TIMEOUT           -5 // 超时
#define ACCESS_ERROR_TYPE_BUSY              -6 // 忙
#define ACCESS_ERROR_TYPE_AGAIN             -7 // 需要重试
#define ACCESS_ERROR_TYPE_AUTH             -8 // 验证错误
#define ACCESS_ERROR_TYPE_NORIGHT             -9 // 没有权限
#define ACCESS_ERROR_TYPE_GETCFG             -10 // 获取用户配置失败


#ifdef __cplusplus
extern "C"{
#endif


#ifndef CJSON_STRUCT
#define CJSON_STRUCT
	typedef void cJSON_Struct; // Common_cJSON_T
#endif

#ifndef ACCESS_HANDLE
#define ACCESS_HANDLE
	typedef void* AccessHandle_T ;
	typedef void* AccessResponceHandle_T ;
#endif
	typedef S32 (*Access_CallFunctions_Def)(AccessHandle_T hAccessHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData);

	/*
	{
	SystemName:,
	AccessName:,
	RemoteSvrDomain:,
	RemoteSvrPort:,
	LocalSvrPort:,
	SvrType:
	}
	*/

	LIBACCESS_API S32 Access_Init(AccessHandle_T *pAccessHandle,cJSON_Struct *pInitConfig,cJSON_Struct **pOutConfig,Access_CallFunctions_Def fxn,void *pUserData);
	LIBACCESS_API S32 Access_Unint(AccessHandle_T *pAccessHandle);
	LIBACCESS_API S32 Access_CallFunctions(AccessHandle_T hAccessHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut);
	LIBACCESS_API S32 Access_LoadConfig(AccessHandle_T hAccessHandle,cJSON_Struct **pConfig);
	LIBACCESS_API S32 Access_SaveConfig(AccessHandle_T hAccessHandle,cJSON_Struct *pConfig);
	LIBACCESS_API S32 Access_LoadTempData(AccessHandle_T hAccessHandle,cJSON_Struct **pTempJson);
	LIBACCESS_API S32 Access_SaveTempData(AccessHandle_T hAccessHandle,cJSON_Struct *pTempJson);
    LIBACCESS_API S32 Access_UserAuth(AccessHandle_T hAccessHandle,cJSON_Struct *pInParams);

	typedef enum _tagAccess_ConfigType
	{
		Access_ConfigType_Normal  = 0,// 一般配置 等同于Access_LoadConfig/Access_SaveConfig操作
		Access_ConfigType_Default = 1, // load only
		Access_ConfigType_Custom  = 2, // load only
		Access_ConfigType_Temp    = 3, // 临时配置，应用程序重启有效，设备重启失效 ，等同于 Module_LoadTempData/Module_SaveTempData
		Access_ConfigType_FactoryInfo    = 4, // 厂商信息
		Access_ConfigType_Debug    = 5, // 调试信息
	}Access_ConfigType_E;

	LIBACCESS_API S32 Access_LoadConfigByType(AccessHandle_T hAccessHandle,Access_ConfigType_E nType,cJSON_Struct **pConfigJson);
	LIBACCESS_API S32 Access_SaveConfigByType(AccessHandle_T hAccessHandle,Access_ConfigType_E nType,cJSON_Struct *pConfigJson);


	// 订阅操作接口
	typedef S32 (*Access_Events_Def)(AccessHandle_T hAccessHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData);
	LIBACCESS_API S32 Access_SubscribeEvent(AccessHandle_T hAccessHandle,S8 *szSubscribeUri,cJSON_Struct *pInParams,cJSON_Struct *pOutParams,Access_Events_Def fxn,void *pUserData);
	LIBACCESS_API S32 Access_UnSubscribeEvent(AccessHandle_T hAccessHandle,S32 nSubscribeID);
	LIBACCESS_API S32 Access_QueryEvent(AccessHandle_T hAccessHandle,S32 nSubscribeID,cJSON_Struct **pOutEventInfo,int nMSecTimeOut);
	// 被订阅操作接口
	typedef S32 (*Access_Subscribe_Def)(AccessHandle_T hAccessHandle,S32 nType /* 0-subscribe,1-unsubscribe,2-QueryEvent*/,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData);
	LIBACCESS_API S32 Access_RegisterSubscribe(AccessHandle_T hAccessHandle,S8 *szSubscribeUri,Access_Subscribe_Def fxn,void *pUserData);
	LIBACCESS_API S32 Access_UnRegisterSubscribe(AccessHandle_T hAccessHandle,S8 *szSubscribeUri);
	LIBACCESS_API S32 Access_SendEvent(AccessHandle_T hAccessHandle,S32 nRecvID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,S32 nMSecTimeOut);
	//订阅绑定私有数据,用于保存一些私有数据，避免遍历
	LIBACCESS_API S32 Access_Subscribe_SetPrivateInfo(AccessHandle_T hAccessHandle,S32 nRecvID/*or nSubscribeID*/,void *pBuff,S32 nSize);
	LIBACCESS_API S32 Access_Subscribe_GetPrivateInfo(AccessHandle_T hAccessHandle,S32 nRecvID,void **pBuff,S32 *lpSize);


	// 流操作
#define ACCESS_STREAM_OPEN_FLAG_CREATE    (1 << 0)
#define ACCESS_STREAM_OPEN_FLAG_READ      (1 << 1)
#define ACCESS_STREAM_OPEN_FLAG_WRITE     (1 << 2)
#define ACCESS_STREAM_OPEN_FLAG_VIDEO     (1 << 3)
#define ACCESS_STREAM_OPEN_FLAG_PROCCESS  (1 << 4)
	LIBACCESS_API S32 Access_StreamQueue_Open(AccessHandle_T hAccessHandle,char *szUri,cJSON_Struct *pOpenParams,cJSON_Struct **pStreamInfo,S32 nMSecTimeout);
#ifndef _COOPEN_PARAM_
#define _COOPEN_PARAM_
	typedef struct _tagCoOpen_Param
	{
		S32 nIndex; // 当前流本地索引号,当索引号存在时，将被替换.后面读写流时有效;打开的非Co流时，nIndex 无效
		S32 nMode;// 0- 读 1-写
		S8 *szUri; // 需要打开的Uri ; 为NULL时 表示从列表中删除
		cJSON_Struct *pOpenParams; //打开时的参数
		cJSON_Struct *pOutStreamInfo; // 返回的流信息
		S32 nErrorCode; //错误码 0- 成功
	}CoOpen_Param_T;
#endif
	LIBACCESS_API S32 Access_StreamQueue_CoOpen(AccessHandle_T hAccessHandle,S32 fd,CoOpen_Param_T *pSets,S32 nSetsNum,S32 nMSecTimeout);
	/* Module_StreamQueue_CoOpen: 一个句柄管理多个流，被管理的流，将尽量均匀提取
	fd :有效时,在现有的基础上再加减流,Module_StreamQueue_CoOpen 返回 fd;当无效时，Module_StreamQueue_CoOpen将创建一个新的fd;
	pSets :需要添加的 流数组
	nSetsNum: 数组个数
	*/
	LIBACCESS_API S32 Access_StreamQueue_Close(AccessHandle_T hAccessHandle,S32 fd);
	LIBACCESS_API S32 Access_StreamQueue_WriteData(AccessHandle_T hAccessHandle,S32 fd,S32 nIndex,cJSON_Struct *pPrivInfo, void *pData1, S32 nSize1, void *pData2, S32 nSize2);
	LIBACCESS_API S32 Access_StreamQueue_ReadData(AccessHandle_T hAccessHandle,S32 fd,S32 nIndex,S32 *lpIndex,cJSON_Struct **pPrivInfo,void **pData, S32 *lpSize, S32 nTimeout);
	LIBACCESS_API S32 Access_StreamQueue_ReleaseData(AccessHandle_T hAccessHandle,S32 fd);

#ifndef STREAMQUEUE_EPOLL_EVENT
#define STREAMQUEUE_EPOLL_EVENT
	typedef struct _tagStreamQueue_EPoll_Event
	{
		S32 fd;
		S32 event;// 1- 读，2-写，3-读写
	}StreamQueue_EPoll_Event_T;
#endif
#define STEAMQUEUE_EPOLL_CTL_ADD 0
#define STEAMQUEUE_EPOLL_CTL_MOD 1
#define STEAMQUEUE_EPOLL_CTL_DEL 2
	typedef void* StreamQueue_EPollHandle_T;
	LIBACCESS_API S32 Access_StreamQueue_EPoll_Create(StreamQueue_EPollHandle_T *pHandle,S32 nSize);
	LIBACCESS_API S32 Access_StreamQueue_EPoll_Ctl(StreamQueue_EPollHandle_T hHandle,S32 nOp,S32 nFd,StreamQueue_EPoll_Event_T *pEvent);
	LIBACCESS_API S32 Access_StreamQueue_EPoll_Wait(StreamQueue_EPollHandle_T hHandle,StreamQueue_EPoll_Event_T *pResults,S32 nMaxResultsNum,S32 nMSecTimeout);
	LIBACCESS_API S32 Access_StreamQueue_EPoll_Destroy(StreamQueue_EPollHandle_T *hHandle);

	LIBACCESS_API S32 Access_StreamQueue_Control(AccessHandle_T hAccessHandle,S32 fd,S32 nIndex,cJSON_Struct *pControlInfo,cJSON_Struct **pResults,S32 nMSecTimeOut);



#ifdef __cplusplus
};
#endif
#endif


