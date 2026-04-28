#ifndef __LIBMODULE_STRUCT_H__
#define __LIBMODULE_STRUCT_H__


#ifndef WIN32
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/un.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdarg.h>  
#include <sys/stat.h>


#define GetLastError() errno
#define strnicmp strncasecmp 
#define stricmp strcasecmp

#define closeSocket close
#define ioctlsocket ioctl



#else
#include <stdio.h>
#include <io.h>
#include <winsock2.h>
#include <Windows.h>
#include <Mstcpip.h>
#include <ws2ipdef.h>
#include <direct.h>
#include <Iphlpapi.h>


typedef int socklen_t;



#define snprintf _snprintf

#define MSG_NOSIGNAL 0


#endif




#ifndef EWOULDBLOCK
#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE
#define EAGAIN                  WSATRY_AGAIN
#define EINTR                   WSAEINTR

#define closeSocket closesocket
#endif
#include "libcommon_api.h"
#include "libmodule_api.h"


#define LIBMODULE_MAX_NUM  8
#define LIBMODULE_MAX_CLIENT_NUM  128
#define LIBMODULE_MAX_RESOURCE_NUM  128
#define LIBMODULE_MAX_SUBSCRIBE_NUM  128
#define LIBMODULE_MAKEFOURCC(ch0, ch1, ch2, ch3) ((U32)(U8)(ch0) | ((U32)(U8)(ch1) << 8) | ((U32)(U8)(ch2) << 16) | ((U32)(U8)(ch3) << 24 ))
#define LIBMODULE_PACKET_STARTCODE LIBMODULE_MAKEFOURCC('o','n','v','s')
#define LIBMODULE_HEART_INV     10
typedef struct _tagModulePacketHeader
{
	unsigned int uStartCode;// 数据同步码 
	int nDataLen;// 数据长度.
	unsigned int uDataFormat:8;// 0- JSON
	unsigned int uExternHeaderLen:8;// 扩展头长度
	unsigned int uSeq:15;
	unsigned int bResp:1; // 0-请求 ,1-应答
	unsigned int  uMsgId:16;// 整包消息编号，便于识别对应包
	unsigned int  uRes:16;
}ModulePacketHeader_T;

typedef struct _tagModuleUDPPacketExtHeader
{
	unsigned int uType:8;// 1- udp
	unsigned int uRes:24;
	unsigned int uTotal;// 总长度
	unsigned int uTotalPkt:16;// 总包个数
	unsigned int uPktIdx:16; // 当前包索引 0- 开始
}ModuleUDPPacketExtHeader_T;

typedef struct _tagLibModuleClientInfo
{
	unsigned int uModuleID;//模块分配的ID
	char *pszModuleName;
	int nSocket; // 与该模块对应的socket;
	int bUnixSocket;
	char *pRecvBuffer; // 未完成消息时的临时缓冲。
	int nRecvLen; // 已使用的大小
	int nNeedLen; // 总需要接收的大小
	int nWriteFlag; // 写标志
    ModulePacketHeader_T tHeader;
	int nHeaderLen;
	unsigned short uSendSeq;
	char *pSendBuffer;
	int nSendLen;
	int nNeedSendLen;

	// http
	S32 bHttp;// 1-是HTTP,0-私有协议
	S32 bHeaderChecked; // 0- 需要检查是不是HTTP协议,
    CommonHttpContext_T tHttpContext;


	//////////////////////////////////////////////////////////////////////////
	Common_Thread_T hClientThread;
	int bClientThreadExit;
	int bNeedDelete;
	void *pModuleMgr;

	//////////////////////////////////////////////////////////////////////////
	struct _tagLibModuleClientInfo *pNext;
	struct _tagLibModuleClientInfo *pPrev;
}LibModuleClientInfo_T;
#ifdef WIN32
#define LibModuleDefaultMgrServer_Name "core"
#define LibModuleDefaultMgrServer_Domain "127.0.0.1"
#define LibModuleDefaultMgrServer_Port 600
#else
#define LibModuleDefaultMgrServer_Name "core"
#define LibModuleDefaultMgrServer_Domain "/tmp/core.domain"
#define LibModuleDefaultMgrServer_Port 0
#endif

typedef struct _tagLibModuleResourceInfo
{
	char *pResUri; // 记录资源
	char *pOwerModule; // 源模块名
	char *pUsedModule; // 目的模块名
	struct _tagLibModuleResourceInfo *pPrev;
	struct _tagLibModuleResourceInfo *pNext;
}LibModuleResourceInfo_T;

typedef struct _tagLibModuleProxyInfo
{
	char *pResUri; // 记录资源
	char *pProxyUri;// 代理uri
	char *pProxyModule; // 代理模块名
	U32 dwFlag;
	struct _tagLibModuleProxyInfo *pPrev;
	struct _tagLibModuleProxyInfo *pNext;
}LibModuleProxyInfo_T;

typedef struct _tagLibModuleRegInfo
{
	unsigned int uModuleID;//模块分配的ID
	char *pszModuleName; // 模块 名
	char *pszDomain;// 如/tmp/core.domain
	char *pszIpv4;// 到达管理者的IP,core从peer获取
	int nPort;// 端口
	char *pszMac;// 00:00:00:00:00:00
	char *pszSerialNumber; 
	int nLastAliveTime;// 最后一次心跳时间(开机时间)
	int nLastRegTime; // 最新注册时间
	int nModuleListRefreshFlag;
	int bOnline; // 离线
	S32 nModuleMark; // 模块标识，判断是否同一实例
	struct _tagLibModuleRegInfo *pNext;
	struct _tagLibModuleRegInfo *pPrev;
}LibModuleRegInfo_T;

#define MODULE_SUBSCRIBE_RENEW_INTERVAL  60
typedef struct _tagLibModuleSubscribeUser
{
	S32 nSubscribeId; // 分配的ID
	S8 *szSubscribeUri;// 要订阅资源
	S8 *szRessUri;// 分配 的资源
	S8 *szResponceUri; // 接受事件的URI
	S8 *szModuleName;// 模块名
	Module_Events_Def fxn;
	void *pUserData;
	S32 bNeedDelete;// 需要删除，删除前需要反订阅
	S32 nLastRenewTime; // 
	void *pPrivateInfo;
	S32 nPrivateInfoSize;
	// 使用计数
	S32 nUseCount;
	struct _tagLibModuleSubscribeUser *pPrev;
	struct _tagLibModuleSubscribeUser *pNext;
}LibModuleSubscribeUser_T;


typedef struct _tagLibModuleSubscribeOwner
{
	S32 nSubscribeId;
	S8 *szSubscribeUri;// 订阅资源
	S32 bInvalid;// 如果此资源有访问者，则直接失效处理,下次注册时再用上,避免通知对端重订阅,否则删除节点。
	Module_Subscribe_Def fxn;
	void *pUserData;
	S32 nUsedCnt;
	LibModuleSubscribeUser_T *pUserInfoList[LIBMODULE_MAX_SUBSCRIBE_NUM];
	LibModuleSubscribeUser_T *pUserInfoHead;
	S32 nUserCount;
	struct _tagLibModuleSubscribeOwner *pPrev;
	struct _tagLibModuleSubscribeOwner *pNext;
}LibModuleSubscribeOwner_T;
typedef struct _tagLibModuleSubscribeInfo
{
	LibModuleSubscribeOwner_T *pOwnerInfoList[LIBMODULE_MAX_SUBSCRIBE_NUM];// 本地资源
	LibModuleSubscribeOwner_T *pOwnerInfoHead;
	LibModuleSubscribeUser_T *pUserInfoList[LIBMODULE_MAX_SUBSCRIBE_NUM]; // 记录本模块使用的资源
	LibModuleSubscribeUser_T *pUserInfoHead; // 需要重试订阅的列表,记录pUserInfoList中未订阅成功的节点，不用重分配内存
	int nOwerCount;
	int nUserCount;
	U32 dwHandleCount;

	LibModuleSubscribeUser_T *pUserInfoDeleteHead;
	Common_Lock_T hLock;
}LibModuleSubscribeInfo_T;
typedef struct _tagLibModuleStreamQueueStreamMultiRess
{
	S8 *szStreamRess;
	S32 nShmId;
	 
}LibModuleStreamQueueStreamMultiRess_T;

typedef struct _tagLibModuleStreamQueueStreamRess
{
	// Co-Open 资源
	S32 nIndex;
	S32 nMode;//0-read,1-write
	S32 bKicked;
	S8 *szUri; // 请求URI
	S8 *szResUri;
	S8 *szReceiveUri; // 接收资源URI
	S32 bNeedDelete; // 是否要删除
	S32 nUseCount;

	cJSON_Struct *pOpenParams;
	void *pOwner;
	// 共享缓冲资源
	
	int nLastReadIndex; // 上一次读的位置
	LibModuleStreamQueueStreamMultiRess_T *pStreamRessArray;
	S32 nStreamRessArrayCount;
	struct _tagLibModuleStreamQueueStreamRess *pPrev;
	struct _tagLibModuleStreamQueueStreamRess *pNext;
	struct _tagLibModuleStreamQueueStreamRess *pPrev_Re;
	struct _tagLibModuleStreamQueueStreamRess *pNext_Re;
	struct _tagLibModuleStreamQueueStreamRess *pPrev_De;
	struct _tagLibModuleStreamQueueStreamRess *pNext_De;
}LibModuleStreamQueueStreamRess_T;

#define LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM  128
typedef struct _tagLibModuleStreamQueueUser
{
	S32 nStreamQueueId; // 本地对应 ID
	// 资源提供端
	S8 *szUri; // 请求URI
	S32 nOwnerStreamQueueId;
	S8 *szResUri;// 记录资源URI
	S8 *szReceiveUri; // 接收资源URI
	
	S32 nModuleId;// 模块ID
	void *pUserPrivateData;
	int nLastReadIndex; // 上一次读的位置
	S32 bDataNeedRelease; // 标志缓冲是否释放
	S32 nNeedReleaseIndex;
	S32 nNeedReleaseIndexMulti;


	// 资源使用端
	S32 nEpollReadHandle;
	S32 nEpollWriteHandle;
	S32 bNeedDelete; // 是否要删除
	S32 nUseCount;
	S32 nStreamRessCount;
	LibModuleStreamQueueStreamRess_T *pStreamRessList[LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM];
	LibModuleStreamQueueStreamRess_T *pStreamRessHead; // 流信息,对应 CoOwner/CoOpen组合流,读
	LibModuleStreamQueueStreamRess_T *pStreamRessWriteHead;// 写
	S32 nStreamRessWriteCount;
	struct _tagLibModuleStreamQueueUser *pPrev;
	struct _tagLibModuleStreamQueueUser *pNext;
}LibModuleStreamQueueUser_T;
#define LIBMODULE_STREAMQUEUE_MAX_POOL  1024
#define LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM  128
#define LIBMODULE_STREAMQUEUE_ADDNUM  16;
#define LIBMODULE_STREAMQUEUE_MAX_USER_NUM  128


typedef struct _tagLibModuleStreamQueueOwner
{
	S32 nStreamQueueId; // 本服务器ID
	S8 *szUri;// 资源名,为NULL时，表示匿名，不匹配
	/*
	 bit[0,13]:index,bit[14,15]:0-owner,1-Co-owner,2-user,3-Co-User
	 bit[16,31]:StaticCount
	*/
	S32 nStreamType;
	S32 nMode;// 0- 读 1-写
	S32 bDataNeedRelease; // 标志缓冲是否释放

	S8 *szStreamRess; // 流资源
	S32 nShmId;
    S32 nUserCount;
	S32 pUserIdList[LIBMODULE_STREAMQUEUE_MAX_USER_NUM]; // 流下挂的用户列表
	S32 nCoOwnerCount;
	S32 pCoOwnerIdList[LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM];// 被哪些综合流使用

	Module_StreamQueue_Callbacks tFxn;
	void *pUserData;
	struct _tagLibModuleStreamQueueOwner *pPrev;
	struct _tagLibModuleStreamQueueOwner *pNext;
}LibModuleStreamQueueOwner_T;

typedef struct _tagLibModuleStreamQueueCoOwner
{
	S32 nStreamQueueId;
	S8 *szUri; // 
	int nLastReadIndex; // 上一次读的位置
	S32 nOwnerCount;
	S32 pOwerIdList[LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM]; //  绑定的流列表
	S32 nUserCount;
	S32 pUserIdList[LIBMODULE_STREAMQUEUE_MAX_USER_NUM]; // 流下挂的用户列表
	Module_StreamQueue_Callbacks tFxn;
	void *pUserData;
	struct _tagLibModuleStreamQueueCoOwner *pPrev;
	struct _tagLibModuleStreamQueueCoOwner *pNext;
}LibModuleStreamQueueCoOwner_T;


typedef struct _tagLibModuleStreamQueueInfo
{
	LibModuleStreamQueueOwner_T *pOwnerListHead;// 流的创建者
	S32 nOwnerCount;
	LibModuleStreamQueueCoOwner_T *pCoOwnerListHead;// 流的创建者
	S32 nCoOwnerCount;
	LibModuleStreamQueueUser_T *pUserListHead; // 流的使用者
	S32 nUserCount;
	LibModuleStreamQueueStreamRess_T *pStreamRessReConnectList;
	LibModuleStreamQueueStreamRess_T *pStreamRessDeleteList;
	Common_InterSleep_T hInterSleep;
	void *pInfoPool[LIBMODULE_STREAMQUEUE_MAX_POOL];
	S32 nInfoPoolCount;
	S32 nInfoPoolFreeIdx;
	Common_Lock_T hLock;
	Module_StreamQueue_Callbacks tGlobalFxn;
	void *pUserData;
	S32 nStaticCount;
	Common_Thread_T hThread_ReConnect;
	S32 bReConnectThreadExit;
}LibModuleStreamQueueInfo_T;

// core 本地socket和net socket同时存在
typedef struct _tagLibModuleInfo
{
	ModuleHandle_T hModuleHandle;
	//本模块的信息
	unsigned int uModuleID;//模块分配的ID
	char *pszModuleName;
	char *pszSystemName;
	int bUseUnixDomain;
	char *pszLocalDomain;
	char *pszMac;// 00:00:00:00:00:00
	char *pszSerialNumber; 
	int bManager;// core
	int nServerType;// 0- p2p方式,1-core转发方式,2-模块管理者core
	int nClientSocket;// 连接管理服务器(core)的socket;
	int nServerSocket;//本模块做为服务器的socket;
	int nServerPort;
	int nUnixSvrSocket; // linux本地socket
	unsigned int uCoreID;//Core ID
	char *pszCoreMac;
	char *pszCoreIp;
	char *pszCoreDomain;
	char *pszCoreSerialNumber;
	int nCorePort;
	S32 nModuleMark; // 模块标识，判断是否同一实例
	// 本模块作为服务器时，client,记录其他模块接入的信息
	LibModuleClientInfo_T *pClientModuleInfo[LIBMODULE_MAX_CLIENT_NUM];
	LibModuleClientInfo_T *pClientInfoHead;
	
	int nClientModuleNum;
	unsigned short uStaticCount;

	Module_CallFunctions_Def fCallFunction;
	void *pCallUserData;
	// 监听线程句柄
	Common_Thread_T hListenThread;
	int bListenThreadExit;
	// 心跳线程句柄
	Common_Thread_T hHeartThread;
	int bHeartThreadExit;

	int bRegister;
	int bHeart;
	int nLastHeartTime;
	int nHeartFailCnt;

	//////////////////////////////////////////////////////////////////////////
	Common_Lock_T   hRegModuleLock;
	LibModuleRegInfo_T *pModuleMgrServer; //管理 服务器
	Common_Lock_T hProxyLock;
	U32 dwProxyRefreshFlag;
	S32 bNeedReportProxy;
	LibModuleProxyInfo_T *pResProxyListHead; //资源代理 服务器
	LibModuleRegInfo_T *pModuleRegInfoList[LIBMODULE_MAX_CLIENT_NUM];
	LibModuleRegInfo_T *pModuleRegInfoHead;
	LibModuleRegInfo_T *pModuleRegInfoTail;
	int nRegModuleNum;
	U32 dwRegRefreshFlag;
	int bNeedReport;
	//////////////////////////////////////////////////////////////////////////
	LibModuleResourceInfo_T *pOwnResourceList[LIBMODULE_MAX_RESOURCE_NUM]; // 当前模块的资源
	LibModuleResourceInfo_T *pUsedResourceList[LIBMODULE_MAX_RESOURCE_NUM]; // 当前模块使用的外部资源
	LibModuleResourceInfo_T *pOwnRessHead;
	LibModuleResourceInfo_T *pUsedRessHead;
	int nOwnRessNum;
	int nUsedRessNum;
	Common_Lock_T   hModuleRessLock;
	//////////////////////////////////////////////////////////////////////////
	// 订阅资源
	LibModuleSubscribeInfo_T tSubscribeInfo;
	

	//////////////////////////////////////////////////////////////////////////
	LibModuleStreamQueueInfo_T tStreamQueueInfo;

	//////////////////////////////////////////////////////////////////////////
	struct _tagLibModuleInfo *pNext;
}LibModuleInfo_T;


Common_Log_T Module_Log_GetDefaultHandle();
 void Module_Log_SetDefaultHandle(Common_Log_T hLog);
#if 0
#define MODULE_LOG_INIT(tag,defaultLevel)  do{Common_Log_T hTmp = NULL;Common_Log_Create(&hTmp,tag,defaultLevel);Module_Log_SetDefaultHandle(hTmp);}while(0)
#define MODULE_LOGE(fmt,...)    Common_Log_Out(Module_Log_GetDefaultHandle() ,COMMON_LOG_LV_BASE,  __LINE__,__FUNCTION__,"[ERROR]", fmt, ##__VA_ARGS__)  //ouput error info
#define MODULE_LOGI(fmt,...)    Common_Log_Out(Module_Log_GetDefaultHandle() ,COMMON_LOG_LV_MID,   __LINE__,__FUNCTION__,  "[INFO ]",  fmt,##__VA_ARGS__)   // output basic info
#define MODULE_LOGW(fmt,...)    Common_Log_Out(Module_Log_GetDefaultHandle() ,COMMON_LOG_LV_LOW,  __LINE__,__FUNCTION__, "[WARN ]", fmt,##__VA_ARGS__)  // output warning info
#define MODULE_LOGD(fmt,...)    Common_Log_Out(Module_Log_GetDefaultHandle() ,COMMON_LOG_LV_HIGH,__LINE__,__FUNCTION__, "[DEBUG]",fmt,##__VA_ARGS__)// output debug info
#define MODULE_LOG_UNINIT()     do{Common_Log_T hTmp = Module_Log_GetDefaultHandle();Common_Log_Destroy(&hTmp);}while(0);
#else
#define MODULE_LOG_INIT(tag,defaultLevel)
#define MODULE_LOGE(fmt,...)    
#define MODULE_LOGI(fmt,...)   
#define MODULE_LOGW(fmt,...)    
#define MODULE_LOGD(fmt,...)   
#define MODULE_LOG_UNINIT()     







#endif



#define MODULE_DEBUG printf
#define MODULE_INFO printf
#define MODULE_ERROR printf




#endif

