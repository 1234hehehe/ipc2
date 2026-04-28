#ifndef __LIBMODULE_API_H__
#define __LIBMODULE_API_H__
#include "libcommon_api.h"

#ifdef WIN32
#define LIBMODULE_API __declspec(dllexport)
#else
#define LIBMODULE_API
#endif
#define MODULE_ERROR_TYPE_SUCC               0
#define MODULE_ERROR_TYPE_INVALIDPARAM      -1 // 无效参数
#define MODULE_ERROR_TYPE_UNINITED          -2 // 未初始化的

#define MODULE_ERROR_TYPE_TIMEOUT           -3 // 超时
#define MODULE_ERROR_TYPE_BUSY              -4 // 忙
#define MODULE_ERROR_TYPE_AGAIN             -5 // 需要重试
#define MODULE_ERROR_TYPE_UNKNOW            -6 // 无法识别，或者不支持的
#define MODULE_ERROR_TYPE_LIMITED            -7 // 资源限制
#define MODULE_ERROR_TYPE_MISMATCH            -8 // 不匹配
#define MODULE_ERROR_TYPE_NOTFOUND            -9 // 不存在
#define MODULE_ERROR_TYPE_UNREGISTER        -10 // 失联
#define MODULE_ERROR_TYPE_RUNNING           -11 // 多实例
#define MODULE_ERROR_TYPE_RESUBMIT           -12 // 重复资源提交
#define MODULE_ERROR_TYPE_REFUSED            -13 // 拒绝接入
#define MODULE_ERROR_TYPE_INTERNALERROR            -14 // 内部错误
#define MODULE_ERROR_TYPE_USER_AUTH                    -15 // 密码验证失败
#define MODULE_ERROR_TYPE_USER_RIGHT                    -16 //没有操作权限
#define MODULE_ERROR_TYPE_PARTIAL_SUCCESS                    -17 //部分成功
#define MODULE_ERROR_TYPE_METHOD_UNSUPPORT                  -18 //不支持的方法


#define MODULE_ERROR_TYPE_STREAM_NODATA            -50 // 没有数据
#define MODULE_ERROR_TYPE_STREAM_NOEXIST            -51 // 流不存在
#define MODULE_ERROR_TYPE_STREAM_NORWRIGHT            -52 // 读/写权限
#define MODULE_ERROR_TYPE_STREAM_NEEDRELEASE            -53 // 上一次数据未释放
#define MODULE_ERROR_TYPE_STREAM_WRITEFULL            -54 // 缓冲满
#define MODULE_ERROR_TYPE_STREAM_FORCECLOSE            -55 // 被强行关掉
#define MODULE_ERROR_TYPE_STREAM_INVALID            -56 // 无效的流处理
#define MODULE_ERROR_TYPE_STREAM_OPEN            -57 // 打开失败
#define MODULE_ERROR_TYPE_STREAM_INVALIDRESS            -58 // 内部错误资源不存在
#define MODULE_ERROR_TYPE_STREAM_CLOSED            -59 // 打开失败


#ifdef __cplusplus
extern "C"{
#endif

#ifndef CJSON_STRUCT
#define CJSON_STRUCT
	typedef void cJSON_Struct; // Common_cJSON_T
#endif
#ifndef MODULE_HANDLE
#define MODULE_HANDLE
	typedef void* ModuleHandle_T ;
	typedef void* ModuleResponceHandle_T ;
#endif

	// 获取产品序列号
	LIBMODULE_API S8* Module_GetSerialNumber(S32 nIndex /*= 0 */);
	LIBMODULE_API S32 Module_VersionAuth_burn(S8 *szAuthInfo);
	LIBMODULE_API S32 Module_VersionAuth_Erase();
	LIBMODULE_API S8* Module_VersionAuth_GetMethod();



	// 返回值 :0 -已处理,-1 未处理
	typedef S32 (*Module_CallFunctions_Def)(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData);

	/*
	{
		SystemName:,
		ModuleName:,
		RemoteDomain:,
		RemotePort:,
		LocalSvrPort:,// tcp 监听
		LocalUdpPort:,// udp 监听
		WaitRegister:,// 是否等待注册成功才返回 ,默认是等待;0-不等待,1-等待
		SvrType:
	}
	*/

	LIBMODULE_API S32 Module_Init(ModuleHandle_T *pModuleHandle,cJSON_Struct *pInitConfig,cJSON_Struct **pOutConfig,Module_CallFunctions_Def fxn,void *pUserData);
	LIBMODULE_API S32 Module_Init_Ex(ModuleHandle_T *pModuleHandle,cJSON_Struct *pInitConfig,cJSON_Struct **pOutConfig,Module_CallFunctions_Def fxn,void *pUserData);
	LIBMODULE_API S32 Module_Unint(ModuleHandle_T *pModuleHandle);
	LIBMODULE_API S32 Module_CallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nMSecTimeOut);
    LIBMODULE_API S32 Module_CallFunctions_Remote(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut);
	LIBMODULE_API S32 Module_LoadConfig(ModuleHandle_T hModuleHandle,cJSON_Struct **pConfig);
	LIBMODULE_API S32 Module_SaveConfig(ModuleHandle_T hModuleHandle,cJSON_Struct *pConfig);
    /*****************************************************************************
     函 数 名  : Module_LoadConfigByPath
     功能描述  : 加载指定路径的配置文件
     输入参数  : ModuleHandle_T hModuleHandle 模块名
                 const S8 *pcPath             配置文件路径及名称
     输出参数  : cJSON_Struct **ppConfigJson  配置文件
     返 回 值  : 0 加载成功; -1 加载失败
     调用函数  : static_LoadConfigByPath
     被调函数  :

     修改历史      :
      1.日    期   : 2019年6月28日
        作    者   : wangconglin
        修改内容   : 新生成函数

    *****************************************************************************/
    LIBMODULE_API S32 Module_LoadConfigByPath(ModuleHandle_T hModuleHandle,
                                                       const S8 *pcPath,
                                                       cJSON_Struct **ppConfigJson);

    /*****************************************************************************
     函 数 名  : Module_SaveConfigByPath
     功能描述  : 保存配置到指定文件中
     输入参数  : ModuleHandle_T hModuleHandle 模块名
                 const S8 *pcPath             配置文件路径及名称
                 const cJSON_Struct *pConfigJson 配置文件
     输出参数  : 无
     返 回 值  : 0 保存成功; -1 保存失败
     调用函数  : static_SaveConfigByPath
     被调函数  :

     修改历史      :
      1.日    期   : 2019年6月28日
        作    者   : wangconglin
        修改内容   : 新生成函数

    *****************************************************************************/
    LIBMODULE_API S32 Module_SaveConfigByPath(ModuleHandle_T hModuleHandle,
                                                       const S8 *pcPath,
                                                       cJSON_Struct *pConfigJson);

	LIBMODULE_API S32 Module_LoadTempData(ModuleHandle_T hModuleHandle,cJSON_Struct **pTempJson);
	LIBMODULE_API S32 Module_SaveTempData(ModuleHandle_T hModuleHandle,cJSON_Struct *pTempJson);
	LIBMODULE_API S32 Module_CheckRight(S32 nCheckRightType,S32 nDevice,S32 nChannel,cJSON_Struct *pInParam,cJSON_Struct **pOutParams);

	typedef enum _tagModule_ConfigType
	{
		Module_ConfigType_Normal  = 0,// 一般配置 等同于Module_LoadConfig/Module_SaveConfig操作
		Module_ConfigType_Default = 1, // load only
		Module_ConfigType_Custom  = 2, // load only
		Module_ConfigType_Temp    = 3, // 临时配置，应用程序重启有效，设备重启失效 ，等同于 Module_LoadTempData/Module_SaveTempData
		Module_ConfigType_FactoryInfo    = 4, // 厂商信息
		Module_ConfigType_Debug    = 5, // 调试信息
	}Module_ConfigType_E;

	LIBMODULE_API S32 Module_LoadConfigByType(ModuleHandle_T hModuleHandle,Module_ConfigType_E nType,cJSON_Struct **pConfigJson);
	LIBMODULE_API S32 Module_SaveConfigByType(ModuleHandle_T hModuleHandle,Module_ConfigType_E nType,cJSON_Struct *pConfigJson);

	// 订阅操作接口
	typedef S32 (*Module_Events_Def)(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData);
	LIBMODULE_API S32 Module_SubscribeEvent(ModuleHandle_T hModuleHandle,S8 *szSubscribeUri,Module_Events_Def fxn,void *pUserData);
	LIBMODULE_API S32 Module_UnSubscribeEvent(ModuleHandle_T hModuleHandle,S32 nSubscribeID);
	LIBMODULE_API S32 Module_QueryEvent(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct **pOutEventInfo,int nMSecTimeOut);
	// 被订阅操作接口
	typedef S32 (*Module_Subscribe_Def)(ModuleHandle_T hModuleHandle,S32 nType /* 0-subscribe,1-unsubscribe,2-QueryEvent*/,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData);
	LIBMODULE_API S32 Module_RegisterSubscribe(ModuleHandle_T hModuleHandle,S8 *szSubscribeUri,Module_Subscribe_Def fxn,void *pUserData);
	LIBMODULE_API S32 Module_UnRegisterSubscribe(ModuleHandle_T hModuleHandle,S8 *szSubscribeUri);
	LIBMODULE_API S32 Module_SendEvent(ModuleHandle_T hModuleHandle,S32 nRecvID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,S32 nMSecTimeOut);
	//订阅绑定私有数据,用于保存一些私有数据，避免遍历
	LIBMODULE_API S32 Module_Subscribe_SetPrivateInfo(ModuleHandle_T hModuleHandle,S32 nRecvID/*or nSubscribeID*/,void *pBuff,S32 nSize);
	LIBMODULE_API S32 Module_Subscribe_GetPrivateInfo(ModuleHandle_T hModuleHandle,S32 nRecvID,void **pBuff,S32 *lpSize);

	// URI 代理(路由)
	LIBMODULE_API S32 Module_UriProxy_Register(ModuleHandle_T hModuleHandle,S8 *szUri,S8 *szProxyUri);
	LIBMODULE_API S32 Module_UriProxy_UnRegister(ModuleHandle_T hModuleHandle,S8 *szUri);


	// 流操作
#define MODULE_STREAM_OPEN_FLAG_CREATE    (1 << 0)
#define MODULE_STREAM_OPEN_FLAG_READ      (1 << 1)
#define MODULE_STREAM_OPEN_FLAG_WRITE     (1 << 2)
#define MODULE_STREAM_OPEN_FLAG_VIDEO     (1 << 3)
#define MODULE_STREAM_OPEN_FLAG_PROCCESS  (1 << 4)
	// 流提供者
	typedef S32 (*Module_StreamQueue_Open_def)(ModuleHandle_T hModuleHandle,S32 *pSourceFd,S32 nRemoteFd,S8 *szUri,cJSON_Struct *pOpenParams,cJSON_Struct **pStreamInfo,void **pUserDataForRemoteFd,void *pUserData);
	/*
	 部分参数说明:
	    pSourceFd : 流句柄 由 Module_StreamQueue_Create返回;当 *pSourceFd == -1时，需要指定一个有效的fd，否则会拒绝远端连接
		nRemoteFd : 请求流的远端操作句柄，可以用它关闭客户端连接，可以用control接口传递给远端一些信息
		szUri : 对应Module_StreamQueue_Open的参数 szUri.
		pOpenParams : 对应 Module_StreamQueue_Open 参数 pOpenParams
		pStreamInfo : 对应 Module_StreamQueue_Open 参数 ppStreamInfo，返回一些流信息
		pUserDataForRemoteFd : 流提供者在Module_StreamQueue_Open_def回调中传入,用于保存一些与nRemoteFd关联的一些私有数据，在其他回调中都会直接透过来，便于对 nRemoteFd 查找，减少遍历
		pUserData :设置回调时同时设置的用户数据，一般对应于 nSourceFd 信息
	*/
	typedef S32 (*Module_StreamQueue_Close_def)(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,cJSON_Struct *pCloseParams,cJSON_Struct **pOutParams,void *pUserDataForRemoteFd,void *pUserData);
	typedef S32 (*Module_StreamQueue_Control_def)(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,cJSON_Struct *pControlParams,cJSON_Struct **pOutParams,void *pUserDataForRemoteFd,void *pUserData);
	typedef S32 (*Module_StreamQueue_RecvData_def)(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,cJSON_Struct *pMode,cJSON_Struct *pPrivInfo,void **pData,S32 nSize,void *pUserData);
	typedef S32 (*Module_StreamQueue_SendData_def)(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,cJSON_Struct **pPrivInfo,void **pData,S32 *nSize,void *pUserData);
	typedef S32 (*Module_StreamQueue_SendDataRelease_def)(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,void *pUserData);
	typedef S32 (*Module_StreamQueue_Status_def)(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,S32 nType,cJSON_Struct *pStatusParams,cJSON_Struct **pOutParams,void *pUserData);
	typedef struct _tagModule_StreamQueue_Callbacks
	{
		Module_StreamQueue_Open_def fOpen;
		Module_StreamQueue_Close_def fClose;
		Module_StreamQueue_Control_def fControl;
#if 0
		Module_StreamQueue_RecvData_def fRecvData;
		Module_StreamQueue_SendData_def fOutData;
		Module_StreamQueue_SendDataRelease_def fOutDataRelease;
#else
		void *pResFuns[5];
#endif
	}Module_StreamQueue_Callbacks;
	LIBMODULE_API S32 Module_StreamQueue_SetGlobalCallback(ModuleHandle_T hModuleHandle,Module_StreamQueue_Callbacks *pFxns,void *pUserData);
	LIBMODULE_API S32 Module_StreamQueue_Create(ModuleHandle_T hModuleHandle,char *szUri,cJSON_Struct *pCreateParams,cJSON_Struct **pOutParams,Module_StreamQueue_Callbacks *pFxns,void *pUserData);
	/*
		pCreateParams = {QueueType=,MaxMemSize=,MaxMemNum=,Mode=,}
		QueueType:0-循环队列,1-先进先出线性队列(FIFO) ,默认为 0
		Mode:0- 读 1-写 ,默认写
		MaxMemSize:bytes 默认为 1024 * 1024,
		MaxMemNum: 节点个数 默认为 25
		HasEliminated:0- 无淘汰机制,1-淘汰机制
		pOutParams={code=,Describe=}
	*/
	LIBMODULE_API S32 Module_StreamQueue_CoCreate(ModuleHandle_T hModuleHandle,char *szCoUri,S32 *FdSets,U32 dwFdCount,Module_StreamQueue_Callbacks *pFxns,void *pUserData);
	// 联合创建，可以将几个流合成一个新流，定义一个新名字，产生一个新的句柄。

	LIBMODULE_API S32 Module_StreamQueue_Destroy(ModuleHandle_T hModuleHandle,S32 fd);

	// 流请求者
	LIBMODULE_API S32 Module_StreamQueue_Open(ModuleHandle_T hModuleHandle,S8 *szUri,cJSON_Struct *pOpenParams,cJSON_Struct **pStreamInfo,S32 nMSecTimeout);
	/*
		pOpenParams = {Mode=}
		Mode:0- 读 1-写
	*/
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

	LIBMODULE_API S32 Module_StreamQueue_CoOpen(ModuleHandle_T hModuleHandle,S32 fd,CoOpen_Param_T *pSets,S32 nSetsNum,S32 nMSecTimeout);
	/* Module_StreamQueue_CoOpen: 一个句柄管理多个流，被管理的流，将尽量均匀提取
	    fd :有效时,在现有的基础上再加减流,Module_StreamQueue_CoOpen 返回 fd;当无效时，Module_StreamQueue_CoOpen将创建一个新的fd;
		pSets :需要添加的 流数组
		nSetsNum: 数组个数
	*/







	LIBMODULE_API S32 Module_StreamQueue_Close(ModuleHandle_T hModuleHandle,S32 fd);
	LIBMODULE_API S32 Module_StreamQueue_WriteData(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,cJSON_Struct *pPrivInfo, void *pData1, S32 nSize1, void *pData2, S32 nSize2);
	LIBMODULE_API S32 Module_StreamQueue_ReadData(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,S32 *lpIndex,cJSON_Struct **pPrivInfo,void **pData, S32 *lpSize, S32 nMSecTimeout);
	LIBMODULE_API S32 Module_StreamQueue_ReleaseData(ModuleHandle_T hModuleHandle,S32 fd);
	LIBMODULE_API S32 Module_StreamQueue_ClearData(ModuleHandle_T hModuleHandle,S32 fd);
	LIBMODULE_API S32 Module_StreamQueue_GetRestCnt(ModuleHandle_T hModuleHandle,S32 fd);
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
	LIBMODULE_API S32 Module_StreamQueue_EPoll_Create(StreamQueue_EPollHandle_T *pHandle,S32 nSize);
	LIBMODULE_API S32 Module_StreamQueue_EPoll_Ctl(StreamQueue_EPollHandle_T hHandle,S32 nOp,S32 nFd,StreamQueue_EPoll_Event_T *pEvent);
	LIBMODULE_API S32 Module_StreamQueue_EPoll_Wait(StreamQueue_EPollHandle_T hHandle,StreamQueue_EPoll_Event_T *pResults,S32 nMaxResultsNum,S32 nMSecTimeout);
	LIBMODULE_API S32 Module_StreamQueue_EPoll_Destroy(StreamQueue_EPollHandle_T *hHandle);

	LIBMODULE_API S32 Module_StreamQueue_Control(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,cJSON_Struct *pControlInfo,cJSON_Struct **pResults,S32 nMSecTimeOut);


#ifdef __cplusplus
};
#endif
#endif

