#ifndef __LIBCOMMON_API_H__
#define __LIBCOMMON_API_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <dlfcn.h>

#ifdef WIN32




#define LIBCOMMON_API __declspec(dllexport)
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

typedef  char S8;
typedef  short S16;
typedef  int S32;
typedef unsigned __int64 U64;
typedef __int64 S64;
typedef double F64;
typedef float F32;
//typedef U32 SIZE_T;


// 文件锁
#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2

// 共享内存
typedef S32 key_t;
#define IPC_CREAT  01000
#define IPC_EXCL   02000
#define IPC_NOWAIT 04000
//
#define IPC_SET  1
#define IPC_RMID 0
#define IPC_STAT 2

#define IPC_PRIVATE   0




#define snprintf _snprintf
#define MSG_NOSIGNAL 0
#define popen _popen
#define pclose _pclose

#else
#include <stddef.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
//#include <net/if.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/prctl.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <time.h>
#include<sys/prctl.h>
#include <semaphore.h>
#include <stdarg.h>  
#include <sys/stat.h>
#include <limits.h>
#include <dirent.h>

#define GetLastError() errno
#define strnicmp strncasecmp 
#define stricmp strcasecmp

#define closeSocket close
#define ioctlsocket ioctl

#define LIBCOMMON_API 
#include <sys/time.h>
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

typedef  char S8;
typedef  short S16;
typedef  int S32;
typedef unsigned long long U64;
typedef long long S64;
typedef double F64;
typedef float F32;
typedef U32 SIZE_T;






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




#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL             (void *)0
#endif



#define COMMON_ERROR_TYPE_SUCC               0
#define COMMON_ERROR_TYPE_INVALIDPARAM      -1 // 无效参数
#define COMMON_ERROR_TYPE_UNINITED          -2 // 未初始化的
#define COMMON_ERROR_TYPE_TIMEOUT           -3 // 超时
#define COMMON_ERROR_TYPE_BUSY              -4 // 忙
#define COMMON_ERROR_TYPE_AGAIN             -5 // 需要重试
#define COMMON_ERROR_TYPE_UNKWON            -6 // 无法识别，或者不支持的
#define COMMON_ERROR_TYPE_LIMITED            -7 // 资源限制
#define COMMON_ERROR_TYPE_MISMATCH            -8 // 不匹配
#define COMMON_ERROR_TYPE_NOTFOUND            -9 // 不存在
#define COMMON_ERROR_TYPE_UNREGISTER        -10 // 失联
#define COMMON_ERROR_TYPE_RUNNING           -11 // 多实例
#define COMMON_ERROR_TYPE_RESUBMIT           -12 // 重复资源提交
#define COMMON_ERROR_TYPE_REFUSED            -13 // 拒绝接入
#define COMMON_ERROR_TYPE_INTERNALERROR            -14 // 内部错误
#define COMMON_ERROR_TYPE_USER_AUTH                    -15 // 密码验证失败
#define COMMON_ERROR_TYPE_USER_RIGHT                    -16 //没有操作权限
#define COMMON_ERROR_TYPE_PARTIAL_SUCCESS                    -17 //部分成功
#define COMMON_ERROR_TYPE_METHOD_UNSUPPORT                  -18 //不支持的方法

// Resource Responce Status Code and describe
typedef enum
{
	// 1xx 临时响应并需要请求者继续执行操作的状态代码
	Common_ResResponceStatusCode_Continue          = 100, //"Continue" 继续，请求者应当继续提出请求，服务器返回此代码表示已经收到请求的一部分，正在等待其余部分
	Common_ResResponceStatusCode_SwithingProto          = 101, //"Switching Protocols" 切换协议，请求者已经要求服务器切换协议，服务器已经确认并准备切换
	Common_ResResponceStatusCode_Processing          = 102, //"Processing "This code indicates that the server has received and is processing the request, but no response is available yet" 处理中，服务器授受请求并处理中，暂时无有效应答。阻止请求者超时失败
	// 2xx 成功
	Common_ResResponceStatusCode_Ok                = 200, //"OK"} //  服务器成功处理了请求
	Common_ResResponceStatusCode_Created          =201, //"Created"} // 请求成功并且服务器创建了新的资源 
	Common_ResResponceStatusCode_Accepted          =202, //"Accepted"} // 服务器授受了请求，但尚未处理
	Common_ResResponceStatusCode_NonAuthInfo          =203, //"NonAuthoritative Information"} //非授权信息 ,服务器已经成功处理了请求，但返回的信息可能来自另一来源
	Common_ResResponceStatusCode_NoConntent          =204, //"No Conntent"} // 无内容 ，服务器成功处理了请求，但没有返回任何内容
	Common_ResResponceStatusCode_ResetContent         =205, //"Reset Content"} // 重置内容，服务器成功处理了请求，但没有返回任何内容
	Common_ResResponceStatusCode_PartialContent          =206, //"Partial Content"} // 部分内容，服务器成功处理了部分GET请求
	Common_ResResponceStatusCode_MultiStaus          =207, //"Multi-Stauts"} // 
	Common_ResResponceStatusCode_AlreadyReported          =208, //"Already Reported"} // 
	Common_ResResponceStatusCode_IMUsed          =226, //"IM Used"} // 
	Common_ResResponceStatusCode_LowSpace         =250, //"Low on Storage Space"} //
	//3xx 重定向,要未完成请求，需要进一步操作，通常这些状态代码用来重定向
	Common_ResResponceStatusCode_MultiChoices      =300, //"Multiple Choices"} // 多种选择，针对请求的服务器可执行多种操作，服务器可根据请求者选择一项操作，或者提供操作列表供请求者选择。
	Common_ResResponceStatusCode_MovedPermanently      =301, //"Moved Permanently"} //永久移动，请求的网页或资源永久移动到新的位置。服务器返回此响应(对GET或HEAD请求的响应)时，会自己将请求者转到新的位置。
	Common_ResResponceStatusCode_MovedTemp        =302, //"Moved Temporarily"} // 临时移动，服务器目前从不同的位置的网页响应请求，但请求者应继续使用原有位置来进行以后的请求。
	Common_ResResponceStatusCode_SeeOther         =303, //"See Other"} // 看其他位置，请求者应当对不同的位置使用单独的GET请求来检索响应时，服务器返回此代码
	Common_ResResponceStatusCode_NotModified      =304, //"Not Modified"} // 未修改，自从上次请求后，请求的网页未修改过，服务器返回此响应时，不会返回网页内容
	Common_ResResponceStatusCode_UseProxy         =305, //"Use Proxy"} // 使用代理，请求者只能使用代理访问请求的网页或资源，如果服务器返回此响应，还表示请求者应使用代理。
	Common_ResResponceStatusCode_TempRedirect          =307, //"Temporary Redirect"} // 临时重定向，服务器目前从不同的位置的网页响应请求，但请求者就继续使用原有位置来进行以后的请求。
	Common_ResResponceStatusCode_PermanentRedirect          =308, //"Permanent Redirect"} // 永久重定向，服务器目前从不同的位置的网页响应请求。
	// 4xx 请求错误，表示请求可能出错，妨碍服务器的处理
	Common_ResResponceStatusCode_BadRequest       =400, //"Bad Request"} // 错误请求，服务器不理解请求的语法
	Common_ResResponceStatusCode_Unauthorized     =401, //"Unauthorized"} // 未授权，请求要求 身份验证。对于需要登录的网页，服务器可能返回此响应
	Common_ResResponceStatusCode_PaymentRequired       =402, //"Payment Required"} // 
	Common_ResResponceStatusCode_Forbidden        =403, //"Forbidden"} //  禁止，服务器拒绝请求
	Common_ResResponceStatusCode_NotFound         =404, //"Not Found"} // 未找到，服务器找不到请求的网页或资源
	Common_ResResponceStatusCode_MethodNotAllowed =405, //"Method Not Allowed"} // 方法禁用，禁用请求中的指定方法
	Common_ResResponceStatusCode_NotAcceptable    =406, //"Not Acceptable"} // 不授受，无法使用请求的内容特性响应请求的网页或资源 
	Common_ResResponceStatusCode_ProxyAuthRequired     =407, //"Proxy Authentication Required"} // 需要代理授权，此状态代码与401类似 ，但指定请求者应当授权使用代理
	Common_ResResponceStatusCode_RequestTimeOut       =408, //"Request Time-out"} // 请求超时，服务器等候请求时发生超时
	Common_ResResponceStatusCode_Conflict          =409, //"Conflict"} // 冲突，服务器在完成请求时发生冲突，服务器必须在响应中包含有关冲突 的信息
	Common_ResResponceStatusCode_Gone             =410, //"Gone"} //  已删除，如果请求的资源已永久删除，服务器会返回此响应
	Common_ResResponceStatusCode_LengthRequired        =411, //"Length Required"} // 需要有效长度，服务器不接受不含有效内容的长度标头字段 的请求
	Common_ResResponceStatusCode_PreconditionFailed    =412, //"Precondition Failed"} //未满足前提条件，服务器未满足请求者在请求中设置的其中一个前提条件
	Common_ResResponceStatusCode_PayloadTooLarge      =413, //"Request Entity Too Large"} //请求的实体过大，服务器无法处理请求，因为请求实体过大，超出服务器的处理能力。
	Common_ResResponceStatusCode_UriTooLarge =414, //"Request-URI Too Large"} // 请求的URI过大，请求的URI过长，服务器无法处理
	Common_ResResponceStatusCode_UnsupportedMediaType =415, //"Unsupported Media Type"} // 不支持的媒体类型，请求的格式不受请求页面的支持
	Common_ResResponceStatusCode_RangeNotSatisfiable          =416, //"Range Not Satisfiable"} // 请求的范围不符合要求 ，如果页面无法提供请求的范围，则服务器会返回此状态代码
	Common_ResResponceStatusCode_ExpectationFailed          =417, //"Expectation Failed"} // 未满足的期望值，服务器未满足期望请求标头字段的要求
	Common_ResResponceStatusCode_Teapot          =418, //"I'm a teapot"} // 
	Common_ResResponceStatusCode_MisdirectedRequest          =421, //"Misdirected Request"} // 
	Common_ResResponceStatusCode_UnprocessableEntity          =422, //"Unprocessable Entity"} // 
	Common_ResResponceStatusCode_Locked          =423, //"Locked"} // 
	Common_ResResponceStatusCode_UpgradeRequired          =426, //"Upgrade Required"} // 
	Common_ResResponceStatusCode_PreconditionRequired          =428, //"Preconditain Required"} // 
	Common_ResResponceStatusCode_TooManyRequests          =429, //"Too Many Requests"} // 
	Common_ResResponceStatusCode_RequestHeaderFieldsTooLarge          =431, //"Request Header Fields Too Large"} // 
	Common_ResResponceStatusCode_ParameterNotUnderStood =451, //"Parameter Not Understood"} // 
	Common_ResResponceStatusCode_ConferenceNotFound =452, //"Conference Not Found"} //
	Common_ResResponceStatusCode_NotEnoughBandWidth =453, //"Not Enough Bandwidth"} //
	Common_ResResponceStatusCode_SessionNotFound =454, //"Session Not Found"} //
	Common_ResResponceStatusCode_MethodNotValid =455, //"Method Not Valid in This State"} //
	Common_ResResponceStatusCode_HeaderFieldNotValid =456, //"Header Field Not Valid for Resource"} //
	Common_ResResponceStatusCode_InvalidRange =457, //"Invalid Range"} //
	Common_ResResponceStatusCode_ParameterIsReadOnly =458, //"Parameter Is Read-Only"} //
	Common_ResResponceStatusCode_AggrOperationNotAllowed =459, //"Aggregate operation not allowed"} //
	Common_ResResponceStatusCode_OnlyAggrOperationAllowed =460, //"Only aggregate operation allowed"} //
	Common_ResResponceStatusCode_UnsupportedTransport =461, //"Unsupported transport"} //
	Common_ResResponceStatusCode_DestinationUnreachable =462, //"Destination unreachable"} //
	// 5xx 服务器错误，表示 服务器在尝试处理请求时发生内部错误，这些错误可能是服务器本身的错误，而不是请求出错
	Common_ResResponceStatusCode_InternalServerError =500, //"Internal Server Error"} //服务器内部错误，服务器遇到错误，无法完成请求。
	Common_ResResponceStatusCode_NotImplemented =501, //"Not Implemented"} // 尚未实施，服务器不具备完成请求的功能，例如，服务器无法识别请求方法时，可能返回此代码
	Common_ResResponceStatusCode_BadGateway =502, //"Bad Gateway"} // 错误网关，服务器作为网关或者代理，从上游服务器收到无效响应
	Common_ResResponceStatusCode_ServerUnavailable =503, //"Service Unavailable"} // 服务不可用，服务器目前无法使用(由于超载或者停机维护)。通常，这只是暂时状态
	Common_ResResponceStatusCode_GatewayTimeOut =504, //"Gateway Time-out"} //网关超时，服务器作为网关或者代理，但没有及时从上游服务器收到请求
	Common_ResResponceStatusCode_VersionNotSupported =505, //"Version not supported"} // 服务器不支持请求中的所用协议版本
	Common_ResResponceStatusCode_VariantAlsoNegotiates =506, //"Variant Also Negotiates"} // 
	Common_ResResponceStatusCode_InsufficientStorage =507, //"Insufficient Storage"} // 
	Common_ResResponceStatusCode_LoopDetected =508, //"Loop Detected"} // 
	Common_ResResponceStatusCode_NotExtended =510, //"Not Extended"} // 
	Common_ResResponceStatusCode_NetworkAuthRequired =511, //"Network Authentication Required"} // 客户端需要得到网络访问权限
	Common_ResResponceStatusCode_OptionNotSupported =551, //"Option not supported"}
}Common_ResResponceStatusCode_E;



#define LIBCOMMON_MAKEFOURCC(ch0, ch1, ch2, ch3) ((U32)(U8)(ch0) | ((U32)(U8)(ch1) << 8) | ((U32)(U8)(ch2) << 16) | ((U32)(U8)(ch3) << 24 ))

typedef struct _tagCommon_Time
{
	S16    year;   //年，是多少就多少，不需要1900做处理
	S8    month;  //月，是多少就多少，不需要加1减1做处理
	S8    day;    //日，是多少就多少

	S8    hour;
	S8    min;
	S8    sec;
	S8    isdst; // 是否为DST

	S8    wday; // 星期
	S8    mday;
	S16   yday;

	S16   dst_offset; // 单位分钟
	S16   zone; // 时区 ，单位分钟
}Common_Time_T;

#ifdef __cplusplus
extern "C"{
#endif

#ifndef CJSON_STRUCT
#define CJSON_STRUCT
	typedef void cJSON_Struct; // Common_cJSON_T
#endif

	// 工具
	LIBCOMMON_API void Common_InitHooks(void *(*malloc_fn)(U32 sz),void (*free_fn)(void *ptr));
	LIBCOMMON_API void*Common_Malloc(S32 nSize,S32 nAlignBytes,const S8 *pDes,S32 nLine);
	LIBCOMMON_API void * Common_Calloc(size_t n, size_t size,const S8 *pDes,S32 nLine);
	LIBCOMMON_API void *Common_Realloc(void *mem_address, size_t newsize,const S8 *pDes,S32 nLine);
	LIBCOMMON_API void Common_Free(void *pMem,const S8 *pDes,S32 nLine);
	LIBCOMMON_API S8  *Common_StrDup(S8 *pString,const S8 *pDes,S32 nLine);
	LIBCOMMON_API S8  *Common_StrnDup(S8 *pString,S32 nMaxLen,const S8 *pDes,S32 nLine);
	LIBCOMMON_API void *Common_Copy(void *pDst,void *pSrc,S32 nByteSize);
	LIBCOMMON_API S8* Common_Strcpy(S8 *pDst,S8 *pSrc);
	LIBCOMMON_API void Common_Strncpy(S8 *pDst,S8 *pSrc,S32 nCount);//字符串复制
	LIBCOMMON_API S32 Common_StrCmp(S8 *pDst,S8 *pSrc);
	LIBCOMMON_API S32 Common_StriCmp(S8 *pDst,S8 *pSrc);// 忽略大小写比较
	LIBCOMMON_API S32 Common_StrnCmp(S8 *pDst,S8 *pSrc,S32 nCount);
	LIBCOMMON_API S32 Common_StrniCmp(S8 *pDst,S8 *pSrc,S32 nCount);
	LIBCOMMON_API void Common_Memory_StartDebug(S32 bAutoPrint,S32 nIntervalSec,S8 *pFilePath);
	LIBCOMMON_API void Common_Memory_StopDebug();
	LIBCOMMON_API void Common_Memory_Print(S8 *pFilePath);
	LIBCOMMON_API S32 Common_Memory_IsDebugging();
	// TRACE 
#ifndef COMMON_TRACE
#define COMMON_TRACE
	typedef void* Common_Trace_T;
#endif
//#ifdef WIN32
#if 1
	void Common_Trace_Return(Common_Trace_T *lpTrace,S32 nRetIdx,S8 **szFName,S8 **szSName,U32 *lpSAddr);
	void Common_Trace_RetFree(Common_Trace_T *lpTrace);
#else
	// void Common_Trace_Return(Common_Trace_T *lpTrace,S32 nRetIdx,S8 **szFName,S8 **szSName,U32 *lpSAddr);
#define Common_Trace_Return(lpTrace,nRetIdx,szFName,szSName,lpSAddr) \
	do{	\
	Dl_info *tdlip = NULL;\
	void *address = NULL;\
	if(!Common_Memory_IsDebugging())break;\
	tdlip = (Dl_info *)Common_Malloc(sizeof(Dl_info),0,__FUNCTION__,__LINE__);\
	if(tdlip == NULL)break;\
	memset(tdlip,0,sizeof(Dl_info));\
	address = __builtin_return_address(nRetIdx);\
	if(address == NULL){Common_Free(tdlip,__FUNCTION__,__LINE__);tdlip=NULL;break;}\
	if(0 == dladdr(address, tdlip))\
	{\
	Common_Free(tdlip,__FUNCTION__,__LINE__);tdlip=NULL;printf("[%d][%p] errno = <%s>\n",nRetIdx,address,dlerror());\
	break;	\
	}\
	if(*szFName != NULL){*szFName = (S8 *)tdlip->dli_fname;}\
	if(*szSName != NULL){*szSName = (S8 *)tdlip->dli_sname;}\
	if((void *)lpSAddr != NULL){*(U32 *)lpSAddr = (long)tdlip->dli_saddr;}\
	*lpTrace = (Common_Trace_T)tdlip;\
	}while(0)

	// void Common_Trace_RetFree(Common_Trace_T *lpTrace);
#define Common_Trace_RetFree(lpTrace) \
      do{/*if(lpTrace == NULL)break;*/if(*lpTrace == NULL)break;Common_Free(*lpTrace,__FUNCTION__,__LINE__);*lpTrace = NULL;}while(0)

#endif
	// Signal Catcher
	LIBCOMMON_API S32 Common_RegistSigHandle(S32 sigNum);

	// 时间处理
	// MicroSec : 微秒
	LIBCOMMON_API void Common_Sleep(S32 nSec,S32 MicroSec);
	// pMSec :毫秒
	LIBCOMMON_API S32  Common_GetSystemCount(S32 *pSec,S32 *pMSec);
	LIBCOMMON_API U64  Common_GetSystemCount64();
	LIBCOMMON_API S32  Common_GetCurrentTime(S32 *pSec,S32 *pMSec);
	LIBCOMMON_API U64  Common_GetLocalTime(Common_Time_T *pCommon_Time);
	LIBCOMMON_API struct tm * Common_LocalTime_r(time_t *t,struct tm *pTm);
	LIBCOMMON_API S32 Common_Linux2CommonTime(time_t linux_time, Common_Time_T *pCommon_time);
	LIBCOMMON_API S32 Common_Common2LinuxTime(Common_Time_T *pCommon_time, time_t *plinux_time);
	LIBCOMMON_API U32 Common_cmp_time(struct timeval *time1, struct timeval *time2);
	LIBCOMMON_API S64 Common_cnt_delta_ms(struct timeval *time1, struct timeval *time2);
	LIBCOMMON_API S64 Common_cnt_interval_ms(struct timeval *time1, struct timeval *time2);
#ifndef COMMON_INTERSLEEP
#define COMMON_INTERSLEEP
	typedef void* Common_InterSleep_T ; 
	LIBCOMMON_API S32 Common_InterSleep_Create(Common_InterSleep_T *pInterSleepHandle);
	// nMSec 毫秒
	LIBCOMMON_API S32 Common_InterSleep_Sleep(Common_InterSleep_T hInterSleepHandle,S32 nSec,S32 nMSec);
	LIBCOMMON_API S32 Common_InterSleep_WakeUp(Common_InterSleep_T hInterSleepHandle);
	LIBCOMMON_API S32 Common_InterSleep_Destroy(Common_InterSleep_T *pInterSleepHandle);
#endif
	// 随机
	LIBCOMMON_API U32 Common_Rand32();
	LIBCOMMON_API U16 Common_Rand16();
	
	// 线程
#ifndef COMMON_THREAD
#define COMMON_THREAD
	typedef void* Common_Thread_T ; 
#endif
	typedef S32 (*Common_Thread_def)(Common_Thread_T hThreadHandle,void *pUserData);
	/*
	uStackSize:0- 系统默认stack大小,否则指定大小(单位:KBytes)
	*/
#define COMMON_THREAD_CREATEFLAG_NORMAL     0 // 普通线程
#define COMMON_THREAD_CREATEFLAG_REALTIME   (1 << 0) // 实时线程
#define COMMON_THREAD_CREATEFLAG_DETACH     (1 << 1) // 支持detach,不返回 pThreadHandle
#define COMMON_THREAD_CREATEFLAG_PRI_NORMAL  (0 << 16) // 
#define COMMON_THREAD_CREATEFLAG_PRI_LOW  (1 << 16) // 
#define COMMON_THREAD_CREATEFLAG_PRI_LOW1  (2 << 16) // 
#define COMMON_THREAD_CREATEFLAG_PRI_HIGH  (3 << 16) // 
#define COMMON_THREAD_CREATEFLAG_PRI_HIGH1  (4 << 16) // 
	LIBCOMMON_API S32 Common_Thread_Create(Common_Thread_T *pThreadHandle,const S8 *szThreadName,U32 uStackSize,S32 CreateFLAG,Common_Thread_def fxn,void *pUserData);
//	LIBCOMMON_API S32 Common_Thread_Join(Common_Thread_T hThreadHandle,S32 nRetValue);
//	LIBCOMMON_API S32 Common_Thread_Cancel(Common_Thread_T hThreadHandle,S32 nRetValue);
//	LIBCOMMON_API S32 Common_Thread_Exit(Common_Thread_T hThreadHandle,S32 nRetValue);
	LIBCOMMON_API S32 Common_Thread_Detach(Common_Thread_T hThreadHandle);
	LIBCOMMON_API S32 Common_Thread_Yield(Common_Thread_T hThreadHandle);
	LIBCOMMON_API S32 Common_Thread_Destroy(Common_Thread_T *pThreadHandle);
	LIBCOMMON_API Common_Thread_T Common_Thread_Self();

	// 锁
#ifndef COMMON_LOCK
#define COMMON_LOCK
	typedef void* Common_Lock_T;
#endif
	LIBCOMMON_API S32 Common_Lock_Create(Common_Lock_T *pLock,const S8 *szLockName);
	LIBCOMMON_API S32 Common_TryLock(Common_Lock_T hLock);
	LIBCOMMON_API S32 Common_Lock(Common_Lock_T hLock);
	LIBCOMMON_API S32 Common_UnLock(Common_Lock_T hLock);
	LIBCOMMON_API S32 Common_Lock_Destroy(Common_Lock_T *pLock);

#ifndef COMMON_COND
#define COMMON_COND
	typedef void* Common_Cond_T;
#endif

	LIBCOMMON_API S32 Common_Cond_Create(Common_Cond_T *pCond,const S8 *szCondName);
	LIBCOMMON_API S32 Common_Cond_TryWait(Common_Cond_T hCond,Common_Lock_T hLock,U32 nMSec);
	LIBCOMMON_API S32 Common_Cond_Wait(Common_Cond_T hCond,Common_Lock_T hLock);
	LIBCOMMON_API S32 Common_Cond_Broadcast(Common_Cond_T hCond);
	LIBCOMMON_API S32 Common_Cond_Signal(Common_Cond_T hCond);
	LIBCOMMON_API S32 Common_Cond_Destroy(Common_Cond_T *pCond);

#ifndef COMMON_RWLOCK
#define COMMON_RWLOCK
	typedef void* Common_RWLock_T;
#endif
	LIBCOMMON_API S32 Common_RWLock_Create(Common_RWLock_T *pLock,const S8 *szLockName);
	LIBCOMMON_API S32 Common_RWLock_TryWLock(Common_RWLock_T hLock);
	LIBCOMMON_API S32 Common_RWLock_TryRLock(Common_RWLock_T hLock);
	LIBCOMMON_API S32 Common_RWLock_WLock(Common_RWLock_T hLock);
	LIBCOMMON_API S32 Common_RWLock_RLock(Common_RWLock_T hLock);
	LIBCOMMON_API S32 Common_RWLock_UnLock(Common_RWLock_T hLock);
	LIBCOMMON_API S32 Common_RWLock_Destroy(Common_RWLock_T *pLock);

#ifndef COMMON_SEM
#define COMMON_SEM
	typedef void* Common_Sem_T;
#endif
	LIBCOMMON_API S32 Common_Sem_Create(Common_Sem_T *phSem,S32 nInitCount,S32 nMaxCount,const S8 *szSemName);
	LIBCOMMON_API S32 Common_Sem_Post(Common_Sem_T hSem);
	LIBCOMMON_API S32 Common_Sem_Pend(Common_Sem_T hSem);
	LIBCOMMON_API S32 Common_Sem_TryPend(Common_Sem_T hSem,U32 nMSec);
	LIBCOMMON_API S32 Common_Sem_Destroy(Common_Sem_T *phSem);

	// system
	LIBCOMMON_API S32  Common_System(const S8 *cmd);
	LIBCOMMON_API S32  Common_Exe_Cmd(const S8 *cmd, U32 timeout_ms, S8 *buf, U32 buf_size);
	LIBCOMMON_API S32  Common_SystemServerStart();
	LIBCOMMON_API S32  Common_SystemServerStop();

	LIBCOMMON_API void *Common_Exe_Popen(const S8 *cmd);
	LIBCOMMON_API S32  Common_Exe_Pread(void *popen_stream, void *buf, U32 nbyte, U32 timeout_ms);
	LIBCOMMON_API S8 * Common_Exe_Pgets(S8 *s, U32 size, U32 timeout_ms, void *popen_stream);
	LIBCOMMON_API S32  Common_Exe_Pclose(void *popen_stream);
	LIBCOMMON_API S32  Common_Exe_Pcmd(const S8 *cmd, U32 timeout_ms, S8 *buf, U32 buf_size);
	//JSON
	
#define Common_Json_Type_False 0
#define Common_Json_Type_True 1
#define Common_Json_Type_NULL 2
#define Common_Json_Type_Number 3
#define Common_Json_Type_Double 4
#define Common_Json_Type_String 5
#define Common_Json_Type_Array 6
#define Common_Json_Type_Object 7

	LIBCOMMON_API cJSON_Struct * Common_Json_Parse(const S8 *szJsonString,const S8 **pEndString,const S8 **pszErrorString);
	LIBCOMMON_API S8 * Common_Json_Print(cJSON_Struct *pJson,S32 *lpStrLen);
	LIBCOMMON_API S8 * Common_Json_PrintUnformatted(cJSON_Struct *pJson,S32 *lpStrLen);
	LIBCOMMON_API cJSON_Struct *Common_Json_New(const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat);
	LIBCOMMON_API cJSON_Struct *Common_Json_New_ex(const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat,const S8 *pDes,S32 nLine);
	typedef void (*Common_Json_Print_def)(const S8 *fmt,...);
	LIBCOMMON_API void Common_Json_StandardPrint(cJSON_Struct *pJson,S8 *pPrefix,S8 *pSuffix,Common_Json_Print_def fMyPrintf);

	// nWhich:pJson为数组时有效,-1为加到尾，否则为指定位置
	// szWhichName:pJson为对象时有效,如果已存在则替换
	LIBCOMMON_API S32 Common_Json_SetItemExtData(cJSON_Struct *pJson,void *pData,S32 nDataSize);
	LIBCOMMON_API void* Common_Json_GetItemExtData(cJSON_Struct *pJson,S32 *lpDataSize);
	LIBCOMMON_API S32 Common_Json_AddItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName,cJSON_Struct *pItem);
	LIBCOMMON_API S32 Common_Json_RemoveItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName);
	LIBCOMMON_API cJSON_Struct *Common_Json_DetachItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName);
	LIBCOMMON_API cJSON_Struct *Common_Json_GetItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName);
	LIBCOMMON_API cJSON_Struct *Common_Json_GetFirstChild(cJSON_Struct *pJson);
	LIBCOMMON_API cJSON_Struct *Common_Json_GetLastChild(cJSON_Struct *pJson);
	LIBCOMMON_API cJSON_Struct *Common_Json_GetParent(cJSON_Struct *pJson);
	LIBCOMMON_API cJSON_Struct *Common_Json_GetNext(cJSON_Struct *pJson);
	LIBCOMMON_API cJSON_Struct *Common_Json_GetPrev(cJSON_Struct *pJson);
	LIBCOMMON_API S32 Common_Json_Size(cJSON_Struct *pJson);
	LIBCOMMON_API S32 Common_Json_ArraySize(cJSON_Struct *pJson);
	LIBCOMMON_API S32 Common_Json_ChildSize(cJSON_Struct *pJson);
	LIBCOMMON_API S32 Common_Json_GetAttr(cJSON_Struct *pJson,S32 *nWhich,S8 **szObjectName,S32 *nType,S8 **pStringValue,S32 *nIntValue,double *fFloat);
	LIBCOMMON_API S32 Common_Json_SetAttrName(cJSON_Struct *pJson,S8 *szOldPathName,S8 *szObjectName);
	LIBCOMMON_API cJSON_Struct * Common_Json_GetAttrValue(cJSON_Struct *pJson,S32 nWhich,const S8 *szObjectName,S32 *nType,S8 **pStringValue,S32 *nIntValue,double *fFloat);
	LIBCOMMON_API cJSON_Struct * Common_Json_SetAttrValue(cJSON_Struct *pJson,S32 nWhich,const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat);

#define Common_Json_GetAttrValueStr(pJson, szObjectName, pStringValue) \
    ({ \
    int _valType; \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, -1, szObjectName, &_valType, pStringValue, NULL, NULL); \
    _json && _valType == Common_Json_Type_String ? _json : NULL; \
    })
#define Common_Json_GetAttrValueInt(pJson, szObjectName, nIntValue) \
    ({ \
    int _valType; \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, -1, szObjectName, &_valType, NULL, nIntValue, NULL); \
    _json && _valType == Common_Json_Type_Number ? _json : NULL; \
    })
#define Common_Json_GetAttrValueBol(pJson, szObjectName, nIntValue) \
    ({ \
    int _valType; \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, -1, szObjectName, &_valType, NULL, nIntValue, NULL); \
    *nIntValue = _valType;\
    _json && (_valType == Common_Json_Type_False || _valType == Common_Json_Type_True) ? _json : NULL; \
    })
#define Common_Json_GetAttrValueFlt(pJson, szObjectName, fFloat) \
    ({ \
    int _valType; \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, -1, szObjectName, &_valType, NULL, NULL, fFloat); \
    _json && (_valType == Common_Json_Type_Double || _valType == Common_Json_Type_Number) ? _json : NULL; \
    })
#define Common_Json_GetAttrValueObj(pJson, szObjectName) \
    ({ \
    int _valType; \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, -1, szObjectName, &_valType, NULL, NULL, NULL); \
    _json && _valType == Common_Json_Type_Object ? _json : NULL; \
    })
#define Common_Json_GetAttrValueArr(pJson, szObjectName) \
    ({ \
    int _valType; \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, -1, szObjectName, &_valType, NULL, NULL, NULL); \
    _json && _valType == Common_Json_Type_Array ? _json : NULL; \
    })
#define Common_Json_GetAttrValueArrItem(pJson, nWhich) \
    ({ \
    cJSON_Struct *_json; \
    _json = Common_Json_GetAttrValue(pJson, nWhich, NULL, NULL, NULL, NULL, NULL);\
    _json;\
    })

#define Common_Json_SetAttrValueStr(pJson, szObjectName, pStringValue) \
    Common_Json_SetAttrValue(pJson, -1, szObjectName, Common_Json_Type_String, pStringValue, 0, 0)

#define Common_Json_SetAttrValueInt(pJson, szObjectName, nIntValue) \
    Common_Json_SetAttrValue(pJson, -1, szObjectName, Common_Json_Type_Number, NULL, nIntValue, 0)

#define Common_Json_SetAttrValueBol(pJson, szObjectName, nIntValue) \
    Common_Json_SetAttrValue(pJson, -1, szObjectName, nIntValue == 0 ? Common_Json_Type_False : Common_Json_Type_True, NULL, 0, 0)

#define Common_Json_SetAttrValueFlt(pJson, szObjectName, fFloat) \
    Common_Json_SetAttrValue(pJson, -1, szObjectName, Common_Json_Type_Double, NULL, 0, fFloat)

#define Common_Json_SetAttrValueObj(pJson, szObjectName) \
    Common_Json_SetAttrValue(pJson, -1, szObjectName, Common_Json_Type_Object, NULL, 0, 0)

#define Common_Json_SetAttrValueArr(pJson, szObjectName) \
    Common_Json_SetAttrValue(pJson, -1, szObjectName, Common_Json_Type_Array, NULL, 0, 0)

#define Common_Json_SetAttrValueArrStr(pJson, nWhich, pStringValue) \
    Common_Json_SetAttrValue(pJson, nWhich, NULL, Common_Json_Type_String, pStringValue, 0, 0)

#define Common_Json_SetAttrValueArrInt(pJson, nWhich, nIntValue) \
    Common_Json_SetAttrValue(pJson, nWhich, NULL, Common_Json_Type_Number, NULL, nIntValue, 0)

#define Common_Json_SetAttrValueArrObj(pJson, nWhich) \
    Common_Json_SetAttrValue(pJson, nWhich, NULL, Common_Json_Type_Object, NULL, 0, 0)

	LIBCOMMON_API cJSON_Struct * Common_Json_SetAttrValue_ex(cJSON_Struct *pJson,S32 nWhich,const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat,const S8 *pDes,S32 nLine);
	LIBCOMMON_API void Common_Json_Delete(cJSON_Struct *pJson);
	LIBCOMMON_API void Common_Json_Delete_ex(cJSON_Struct *pJson,const S8 *pDes,S32 nLine);
	LIBCOMMON_API cJSON_Struct *Common_Json_Duplicate(cJSON_Struct *item,S32 recurse);
	/*
		pszPath:以./\分隔的路径
		pArrayIndex:如果路径包含数组，则为下标值。
		NextString:下一个有效起始,将其作为下次的pszPath来查询下一个字段。
		返回，当前分隔字段,用完需要free
		如 pszPath="a[3]/b/c" ,*pArrayIndex=3,*NextString="b/c",返回 "a"  
	*/
	LIBCOMMON_API S32 Common_UriOneParse(S8 *pszUri,S32 *pArrayIndex,S8 **CurrString,S8 **NextString);
#if 0
	typedef struct _tagCommon_UriSeg
	{
		S8 *pSeg;
		S32 nIndex;
	}Common_UriSeg_T;
	LIBCOMMON_API S8 *Common_UriParses(S8 *pszUri,,Common_PathSeg_T *pArray,S32 *nArrayNum);
#endif

	//net
	LIBCOMMON_API S32 Common_GetLocalNetInfo(S8 *ifname,S32 bIpv6,S8 **pIpAddress,S8 **pNetmask,S8 **pGateway,S8 **pMacString);
	LIBCOMMON_API S32 Common_GetRemoteIP(S32 nSocket,S8 **pIPString,S32 *nPort,S32 *bIPv6);
	LIBCOMMON_API S32 Common_GetLocalIP(S32 nSocket,S8 **pIPString,S32 *nPort,S32 *bIPv6);
	LIBCOMMON_API U32 Common_IpAddr_IsValid(S8 *pIpv4_v6,S32 bIpv6);


	//---------------------------------------------- 监控句柄的使用 ---------------------------------------------
	LIBCOMMON_API S32 Common_Socket_Open(S32 domain, S32 type, S32 protocol);
	LIBCOMMON_API S32 Common_Socket_Close(S32 socket);
	LIBCOMMON_API U32 Common_Socket_IsValid(S32 socket_fd);  //检查句柄是否合法

	LIBCOMMON_API S32 Common_File_Open(S8 *file_path, S32 flags, U32 mode);
	LIBCOMMON_API S32 Common_File_Close(S32 fd);
	LIBCOMMON_API S32 Common_File_IsValid(S32 fd);    //检查句柄是否合法
	LIBCOMMON_API S32 Common_File_Read(S32 fd,void *Buff,U32 nBytes); 
	LIBCOMMON_API S32 Common_File_Write(S32 fd,void *Buff,U32 nBytes); 
	LIBCOMMON_API S64 Common_File_Seek(S32 fd,S64 offset,S32 origin);

	LIBCOMMON_API FILE* Common_File_fOpen(S8 *file_path, S8 *mode);
	LIBCOMMON_API S32 Common_File_fClose(FILE* fd);
	LIBCOMMON_API U32 Common_File_fIsValid(FILE* fd); //检查句柄是否合法
	LIBCOMMON_API S32 Common_File_fRead(void *Buff,U32 SizePer,U32 nCount,FILE *fd); 
	LIBCOMMON_API S32 Common_File_fWrite(void *Buff,U32 SizePer,U32 nCount,FILE *fd); 
	LIBCOMMON_API S64 Common_File_fSeek(FILE * fd,S64 offset,S32 origin); 
	LIBCOMMON_API S64 Common_File_fTell(FILE * fd); 
	LIBCOMMON_API S32 Common_File_fEof(FILE * fd); 
	LIBCOMMON_API S32 Common_File_fFlush(FILE * fd); 
#if 0
	typedef struct _tagCommonFile_EPoll_Event			
	{			
		S32 fd;		
		FILE* pFd;
		S32 event;// 1- 读，2-写，3-读写		
	}CommonFile_EPoll_Event_T;			
#define COMMON_FILE_EPOLL_CTL_ADD 0
#define COMMON_FILE_EPOLL_CTL_MOD 1
#define COMMON_FILE_EPOLL_CTL_DEL 2
	typedef void* CommonFile_EPollHandle_T;
	LIBCOMMON_API S32 Common_File_EPoll_Create(CommonFile_EPollHandle_T *pHandle,S32 nSize);
	LIBCOMMON_API S32 Common_File_EPoll_Ctl(CommonFile_EPollHandle_T hHandle,S32 nOp,S32 nFd,CommonFile_EPoll_Event_T *pEvent);
	LIBCOMMON_API S32 Common_File_EPoll_Wait(CommonFile_EPollHandle_T hHandle,CommonFile_EPoll_Event_T *pResults,S32 nMaxResultsNum,S32 nMSecTimeout);
	LIBCOMMON_API S32 Common_File_EPoll_Destroy(CommonFile_EPollHandle_T *hHandle);
#endif

	LIBCOMMON_API S8 * Common_GetSelfExeName(S8* exeName, S32 len);
	LIBCOMMON_API S32 Common_InstanceIsRunning();

	LIBCOMMON_API S8* Common_FindErrString(S32 errCode);
#define COMMON_FILE_LOCK_READ     0
#define COMMON_FILE_LOCK_WRITE     1
#define COMMON_FILE_UNLOCK         2
#define COMMON_FILE_LOCK_WHENCE_SET         0
#define COMMON_FILE_LOCK_WHENCE_CUR         1
#define COMMON_FILE_LOCK_WHENCE_END         2
	LIBCOMMON_API S32 Common_File_Lock(S32 lockFd, S32 type, S32 whence, S32 start, S32 len, S32 timeout);


	

	LIBCOMMON_API U32 Common_File_IsExist(S8 *pathname); //文件是否存在
	LIBCOMMON_API S32 Common_File_MkDir(S8 *pathname); 

	// tar
#ifndef COMMON_TAR
#define COMMON_TAR
	typedef void* Common_Tar_T;
#endif
	// 获取tar 源文件大小
	LIBCOMMON_API S64 Common_Tar_GetSize(S8 *szTarPathname); 
	// 解压
	LIBCOMMON_API S32 Common_Tar_Decompress(S8 *szTarPathname,S8 *szToPath); 
	// 异步解压
	LIBCOMMON_API S32 Common_Tar_De_Create(Common_Tar_T *phTar,S8 *szTarPathname,S8 *szToPath);
	//获取当前解压大小
	LIBCOMMON_API S64 Common_Tar_GetProgressSize(Common_Tar_T hTar,S64 *pSize,S64 *pTotalSize);
	// 销毁解压
	LIBCOMMON_API S32 Common_Tar_De_Destroy(Common_Tar_T *phTar); 

	// base64
	// decode /encode 返回值需要Common_Free
	LIBCOMMON_API S8* Common_Base64_Decode(S8 *pBase64code, U32 Base64length,U32 *ResultSize);
	LIBCOMMON_API S8* Common_Base64_Encode(S8 *pBase64code, U32 base64length,U32 *pResultSize);
	LIBCOMMON_API S32 Common_Base64_IsValid(S8 *base64code, U32 base64length);
	// md5
#ifndef COMMON_MD5
#define COMMON_MD5
	typedef void* Common_Md5_T;
#endif
	LIBCOMMON_API S32 Common_Md5_Create(Common_Md5_T *phMd5);
	LIBCOMMON_API S32 Common_Md5_Append(Common_Md5_T hMd5,U8 *pData,S32 nDataLen);
	LIBCOMMON_API S32 Common_Md5_Finish(Common_Md5_T hMd5,U8 *pHexOut/*[16]*/,U8 *pStringOut/*[33]*/);
	LIBCOMMON_API S32 Common_Md5_Destroy(Common_Md5_T *pMd5);
	LIBCOMMON_API S32 Common_Md5_Simply(U8 *pData,S32 nDataLen,U8 *pHexOut/*[16]*/,U8 *pStringOut/*[33]*/);
	LIBCOMMON_API S32 Common_Md5_File(U8 *pFileName,U8 *pHexOut/*[16]*/,U8 *pStringOut/*[33]*/);
	// SHA1
#ifndef COMMON_SHA1
#define COMMON_SHA1
	typedef void* Common_Sha1_T;
#endif
	LIBCOMMON_API S32 Common_Sha1_Create(Common_Sha1_T *phSha1);
	LIBCOMMON_API S32 Common_Sha1_Append(Common_Sha1_T hSha1,U8 *pData,S32 nDataLen);
	LIBCOMMON_API S32 Common_Sha1_Finish(Common_Sha1_T hSha1,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/);
	LIBCOMMON_API S32 Common_Sha1_Destroy(Common_Sha1_T *pSha1);
	LIBCOMMON_API S32 Common_Sha1_Simply(U8 *pData,S32 nDataLen,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/);

	// SHA256
#ifndef COMMON_SHA256
#define COMMON_SHA256
	typedef void* Common_Sha256_T;
#endif
	LIBCOMMON_API S32 Common_Sha256_Create(Common_Sha256_T *phSha256);
	LIBCOMMON_API S32 Common_Sha256_Append(Common_Sha256_T hSha256,U8 *pData,S32 nDataLen);
	LIBCOMMON_API S32 Common_Sha256_Finish(Common_Sha256_T hSha256,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/);
	LIBCOMMON_API S32 Common_Sha256_Destroy(Common_Sha256_T *pSha256);
	LIBCOMMON_API S32 Common_Sha256_Simply(U8 *pData,S32 nDataLen,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/);

	// HMACSHA1
#ifndef COMMON_HMAC_SHA1
#define COMMON_HMAC_SHA1
	typedef void* Common_HMacSha1_T;
#endif
	LIBCOMMON_API S32 Common_HMacSha1_Create(Common_HMacSha1_T *phHMacSha1,U8 *szKey,S32 nKeyLen);
	LIBCOMMON_API S32 Common_HMacSha1_Append(Common_HMacSha1_T hHMacSha1,U8 *pData,S32 nDataLen);
	LIBCOMMON_API S32 Common_HMacSha1_Finish(Common_HMacSha1_T hHMacSha1,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/);
	LIBCOMMON_API S32 Common_HMacSha1_Destroy(Common_HMacSha1_T *phHMacSha1);
	LIBCOMMON_API S32 Common_HMacSha1_Simply(U8 *szKey,S32 nKeyLen,U8 *pData,S32 nDataLen,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/);

	// HMAC SHA256
#ifndef COMMON_HMAC_SHA256
#define COMMON_HMAC_SHA256
	typedef void* Common_HMacSha256_T;
#endif
	LIBCOMMON_API S32 Common_HMacSha256_Create(Common_HMacSha256_T *phHMacSha256,U8 *szKey,S32 nKeyLen);
	LIBCOMMON_API S32 Common_HMacSha256_Append(Common_HMacSha256_T hHMacSha256,U8 *pData,S32 nDataLen);
	LIBCOMMON_API S32 Common_HMacSha256_Finish(Common_HMacSha256_T hHMacSha256,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/);
	LIBCOMMON_API S32 Common_HMacSha256_Destroy(Common_HMacSha256_T *phHMacSha256);
	LIBCOMMON_API S32 Common_HMacSha256_Simply(U8 *szKey,S32 nKeyLen,U8 *pData,S32 nDataLen,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/);


	// digest
	LIBCOMMON_API void Common_Digest_CalcNonce(S8 nonce[20 + 1]);
	LIBCOMMON_API void Common_Digest_CalcOpaque(S8 opaque[8 + 1]);
	LIBCOMMON_API S32 Common_Digest_CalcHA1(S8 *szAlg,S8 *szUserName,S8 *szRealm,S8 *szPassword,S8 *szNonce,S8 *szCNonce,S8 szOutHex32Key_HA1[32 + 1] /*HashHex[32 + 1]*/);
	LIBCOMMON_API S32 Common_Digest_CalcResponse(S8 szHA1[32 + 1],S8 *szNonce,S8 *szNonceCount,S8 *szCNonce,S8 *szQop,S8 *szMethod,S8 *szDigestUri,S8 szHEntity[32 + 1],S8 szOutHex32Key_Response[32 + 1] /*HashHex[32 + 1]*/);
   /* CalcResponse:
      szHA1:Common_Digest_CalcHA1 计算结果
	  szNonce:nonce from server
	  szNonceCount:8 hex digits
	  szQop:qop-value: "", "auth", "auth-int"
	  szMethod:method from the request
	  szDigestUri:requested URL
	  szHEntity:H(entity body) if qop="auth-int"
	  szOutHex32Key_Response:request-digest or response-digest

	  // The "response" field is computed as:
	  //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
	  // or, if "fPasswordIsMD5" is True:
	  //    md5(<password>:<nonce>:md5(<cmd>:<url>))
   */
	// WS-UsernameToken
	LIBCOMMON_API S32 Common_UsernameToken_CalcNonce(S8 HexNonce[20]);
	LIBCOMMON_API S32 Common_UsernameToken_CalcDigest(S8 *szCreated, S8  *HexNonce/*[20]*/, S32 nNonceLen, S8 *szPassword, S8 HexHash[20]);
	/*
		szCreated: 时间
		HexNonce:
		szPassword:密码明文
		HexHash:结果 Hex,需要与提供的Hash比较，一致则验证通过
	*/

	// http 处理
#ifndef COMMON_HTTP_CONTEXT
#define COMMON_HTTP_CONTEXT
#define HTTP_RECV_BUFFER_SIZE 1024
#define HTTP_KEYVALUE_MAX_NUM   64
	typedef struct _tagHttpContextKeyValue
	{
		S8 *pKey;
		S8 *pValue;

	}HttpContextKeyValue_T;
	typedef struct _tagCommonHttpContext
	{

		// 处理步骤
		S32 nHttpStep;// 0 - 请求行,1-请求头,2-正文,3-完成
		S32 bKeepAlive;// 0-短连接,1-长连接
		S32 bResponce;// 0- 请求; 1-应答
		// 请求行
		S32 nHttpMethod;// 0- Get,1-put,2-post,3-delete
		S8 *pHttpMethod;// 
		S8 *pHttpUri; // http URI
		//应答
		S32 nHttpCode; // 应答码
		S8 *pHttpCode;
		S8 *pCodeDescribe;// 应答描述
		S8 *pHttpVer; // Http version
		// 请求头,键值对
		HttpContextKeyValue_T tKeyValue[HTTP_KEYVALUE_MAX_NUM];
		S32 nKeyValueCount;
		// 请求内容-正文
		S8 *pContext;// 正文缓冲,NULL 为无正文
		S32 nContextLen;// 实际内容长度


		//--------- 内部使用
		// 缓冲
		S8 *pHttpLine; //
		S32 nHttpLineLen; // 缓冲长度
		S32 nHttpLinePos; //数据大小
		S32 nDealLinePos; // 处理位置
		S32 bContextMalloc;// 0- pContext 非空时也不用释放，指向pHttpLine，1-需要释放 
		S32 nContextNeedLen; // 要求的长度

		S8 *pSendBuffer; // 发送缓冲
		S32 nSendBuffSize;
		S32 nSendSize; // 需要发送的大小
		S32 nSendPos; // 已发送多少



	}CommonHttpContext_T;
#endif
	LIBCOMMON_API S32 Common_Http_Init(CommonHttpContext_T *pHttpContext);
	LIBCOMMON_API S32 Common_Http_Free(CommonHttpContext_T *pHttpContext);
	LIBCOMMON_API S32 Common_Http_Recv(S32 nSocket,CommonHttpContext_T *pHttpContext);
	LIBCOMMON_API S32 Common_Http_Send(S32 nSocket,CommonHttpContext_T *pHttpContext);
	LIBCOMMON_API S32 Common_Http_Http2Json(CommonHttpContext_T *pHttpContext,cJSON_Struct **pJson);
	LIBCOMMON_API S32 Common_Http_Json2Http(cJSON_Struct *pJson,CommonHttpContext_T *pHttpContext);

	LIBCOMMON_API S32 Common_Http_Parse(S8 *pRecvBuffer,S32 nDataLen,CommonHttpContext_T *pHttpContext);
	LIBCOMMON_API S8 *Common_Http_Http2String(CommonHttpContext_T *pHttpContext,S32 *lpSize); // 内存不需释放
	// 定时器
#ifndef COMMON_TIMER
#define COMMON_TIMER
	typedef void *Common_Timer_T;
#endif
	typedef S32 (*Common_Timer_Callback_def)(Common_Timer_T hTimer,void *pUserData);
	LIBCOMMON_API S32 Common_Timer_Create(Common_Timer_T *phTimer,U32 uInterval_ms,Common_Timer_Callback_def fxn,void *pUserData);
	LIBCOMMON_API S32 Common_Timer_Destroy(Common_Timer_T *phTimer);

	// 日志
	// 日志级别
	
	enum
	{
		COMMON_LOG_LV_BASE = 0, // 输出 LOGE
		COMMON_LOG_LV_LOW,        // 输出 LOGW LOGE
		COMMON_LOG_LV_MID,        // 输出 LOGI LOGW LOGE
		COMMON_LOG_LV_HIGH,      // 输出 LOGI LOGW LOGE LOGD
	};
#ifndef COMMON_LOG
#define COMMON_LOG
	typedef void *Common_Log_T;
#endif
	LIBCOMMON_API S32 Common_Log_Create(Common_Log_T *phLog,S8 *tag, U32 defaultLevel);
	LIBCOMMON_API void Common_Log_Out(Common_Log_T hLog,U32 level, S32 lineNum, const S8 *funcStr, const S8 *prefix,const S8 *fmt,...);
	LIBCOMMON_API void Common_Log_SetLevel(Common_Log_T hLog,U32 level);
	LIBCOMMON_API S32 Common_Log_Destroy(Common_Log_T *phLog);
	// 可以使用以下宏

	LIBCOMMON_API Common_Log_T Common_Log_GetDefaultHandle();
	LIBCOMMON_API void Common_Log_SetDefaultHandle(Common_Log_T hLog);
#define LOG_INIT(tag,defaultLevel)  do{Common_Log_T hTmp = NULL;Common_Log_Create(&hTmp,tag,defaultLevel);Common_Log_SetDefaultHandle(hTmp);}while(0)
#define LOGE(fmt,...)    Common_Log_Out(Common_Log_GetDefaultHandle() ,COMMON_LOG_LV_BASE,  __LINE__,__FUNCTION__,"[ERROR]", fmt, ##__VA_ARGS__)  //ouput error info
#define LOGI(fmt,...)    Common_Log_Out(Common_Log_GetDefaultHandle() ,COMMON_LOG_LV_MID,   __LINE__,__FUNCTION__,  "[INFO ]",  fmt,##__VA_ARGS__)   // output basic info
#define LOGW(fmt,...)    Common_Log_Out(Common_Log_GetDefaultHandle() ,COMMON_LOG_LV_LOW,  __LINE__,__FUNCTION__, "[WARN ]", fmt,##__VA_ARGS__)  // output warning info
#define LOGD(fmt,...)    Common_Log_Out(Common_Log_GetDefaultHandle() ,COMMON_LOG_LV_HIGH,__LINE__,__FUNCTION__, "[DEBUG]",fmt,##__VA_ARGS__)// output debug info
#define LOG_SET_LEVEL(level)     Common_Log_SetLevel(Common_Log_GetDefaultHandle(),level)
#define LOG_UNINIT()     do{Common_Log_T hTmp = Common_Log_GetDefaultHandle();Common_Log_Destroy(&hTmp);}while(0)

	// 字符编码
#if 0
	LIBCOMMON_API S32 Common_Unicode_to_Utf8(unsigned short unicode,U8 *pUtf8);
	LIBCOMMON_API U16 Common_Utf8_to_Unicode(U8 *pUtf8,S32 *pnRetBytes);
	LIBCOMMON_API U16 Common_Unicode_to_Gb2312(unsigned short uniCode);
	LIBCOMMON_API U16 Common_Gb2312_to_Unicode(unsigned short gb2312);
	LIBCOMMON_API S32 Common_Utf8_to_Gb2312_string(U8 *pUTF8,U8 *pGB2312,S32 nBufsize);
	LIBCOMMON_API S32 Common_Gb2312_to_Utf8_String(U8 *pGB2312,U8 *pUTF8,S32 nBufsize);
#endif

	// 二维码
#ifndef COMMON_QRCODE
#define COMMON_QRCODE
	typedef struct _tagCommon_QRCode
	{ // 整个点阵尺寸为(wWidth + byMargin * 2) x (wWidth + byMargin * 2);
		// wStride = ((wWidth + byMargin * 2 + 7) >>3)<<3;
		U16 wWidth; // 二维码宽度，不包括边框，像素[输出]
		U16 wStride; // 宽度stride,包括边框，像素，字节对齐[输出]
		U8 byPixelWide;// 每个点占几个像素,0-1x1个,1-1x1个，2-2x2个，3-3x3个，相当于放大[输入/输出]
		U8 byMargin; // 二维码边框宽度,像素,0-无边框[输入/输出]
		U8 byBitsOrder;// 0-字节从高到低，1-字节从低到高. [输入/输出]
		U8 byPicType; // 0- 点阵;1-BMP-bit1;2-BMP-bit24
		U8 *pPixelData; // 需要用free 释放[输出]  ，使用完后，一定要调用Common_Free释放内存，否则内存泄露.
		U32 dwDataSize; // 数据长度[输出]
		U32 dwRes[4];
	}Common_QRCode_T;
#endif
	LIBCOMMON_API S32 Common_QuickResponseCode(const S8 *szString,Common_QRCode_T *pPixelInfo);

    // 位操作宏
#define COMMON_ARRAY_ELEMENT_COUNT(x)        (sizeof(x)/sizeof(x[0]))
#define COMMON_ARRAY_DIM(arg)                 (sizeof(arg) / sizeof(arg[0]))
#define COMMON_CLR_ARG(arg)                      memset(&arg, 0, sizeof(arg))
#define COMMON_ALIGN_ARG(arg, align)      (((arg) + ((align)-1))/(align)) * (align)

#define COMMON_TST_BIT(arg, bit)                 (((arg) & (1 << (bit))) != 0)
#define COMMON_SET_BIT(arg, bit)                 ((arg) |= (1 << (bit)))
#define COMMON_CLR_BIT(arg, bit)                ((arg) &= ~(1 << (bit)))

	// array 
#define COMMON_TST_BIT_EX(pArray, bit)                 ((((pArray)[(bit)>>5]) & (1 << ((bit) & 31))) != 0)
#define COMMON_SET_BIT_EX(pArray, bit)                 (((pArray)[(bit)>>5]) |= (1 << ((bit) & 31)))
#define COMMON_CLR_BIT_EX(pArray, bit)                (((pArray)[(bit)>>5]) &= ~(1 << ((bit) & 31)))
	/*位数组操作宏(a代表数组指针，b代表数组中的第多少位，c代表a中每个元素的bits数目)*/
#define COMMON_BITMASK(b,c)                      (1<<((b)%(c)))
#define COMMON_BITSLOT(b,c)                       ((b)/(c))                                                                                         //将数据映射到所属字节内
#define COMMON_BITSET(a,b,c)                       ((a)[COMMON_BITSLOT(b,c)] |= COMMON_BITMASK(b,c))                  //置位
#define COMMON_BITCLEAR(a,b,c)                  ((a)[COMMON_BITSLOT(b,c)] &= ~COMMON_BITMASK(b,c))             //清位
#define COMMON_BITTEST(a,b,c)                     ((a)[COMMON_BITSLOT(b,c)] & COMMON_BITMASK(b,c))   

	
	 //双向链表操作
	/** 双链表 handle*/
	typedef void * COMMON_DLIST_T;

	/**==========================================================
	* @brief LIST_COMPARE_F 回调接口
	* @note 链表节点匹配判断回调函数 , 一般用来节点的查找
	* @remarks
	* @details
	* @param a 链表节点,数据指针
	* @param b 匹配参数, 用来与链表进行匹配的参数
	* @return
	* - 0 匹配成功
	* - <0 匹配失败
	*
	* =========================================================*/
	typedef S32 (*COMMON_LIST_COMPARE_F)(void * a, void *b);

	/**==========================================================
	* @brief LIST_FOR_EACH_F
	* @note 链表节点循环执行回调函数,一般用来对所有节点共同操作
	* @remarks
	* @details
	* @param data 链表节点,数据指针
	* @return
	* - 0 继续后续循环
	* - != 0 退出循环
	*
	* =========================================================*/
	typedef S32 (*COMMON_LIST_FOR_EACH_F)(void * data);

	typedef void (*COMMON_LIST_NODE_FREE_F)(void * data);
	/**==========================================================
	* @brief Common_DList_Create
	* @note 初始化一个双链表
	* @remarks
	* @details
	* @param dlistHandle 链表handle,此参数将在函数中初始化并分配内存
	* @return
	* - 0 成功
	* - <0 失败
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_Init(COMMON_DLIST_T*dlistHandle,COMMON_LIST_NODE_FREE_F nodeFree);

	/**==========================================================
	* @brief Common_DList_Destroy
	* @note 销毁一个双链表中的所有节点及节点分配的内存,并且销毁双链表handle及内存
	* @remarks
	* @details
	* @param dlistHandle 链表handle
	* @return
	* - 0 成功
	* - <0 失败
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_Uninit(COMMON_DLIST_T* dlistHandle);

	/**==========================================================
	* @brief Common_DList_InsertTail
	* @note 往双链表中的尾部插入一个新的节点
	* @remarks
	* @details
	* @param dlistHandle 链表handle
	* @param data 数据指针 ,  此数据指针需要提前分配内存并赋值
	* @param size 数据大小
	* @return
	* - 0 成功
	* - <0 失败
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_InsertTail(COMMON_DLIST_T dlistHandle, void *data, U32 size);

	/**==========================================================
	* @brief Common_DList_Delete
	* @note 双链表中的单个节点删除,并且释放节点指针分配的内存
	* @remarks
	* @details
	* @param dlistHandle 双链表handle
	* @param data 匹配数据 ,  用来和链表节点中数据进行匹配
	* @param cb 匹配函数
	* @return
	* -             =0     成功删除一个节点
	* -             =-1    没有匹配成功
	* -             <0     参数错误,或者请查看错误列表
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_Delete(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb);

	/**==========================================================
	* @brief Common_DList_DeleteAll
	* @note 双链表中所有节点的删除
	* @remarks
	* @details
	* @param dlistHandle 双链表handle
	* @return
	* - 0 成功
	* - <0 失败
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_DeleteAll(COMMON_DLIST_T dlistHandle);

	/**==========================================================
	* @brief Common_DList_Search
	* @note 查找双链表中的节点
	* @remarks
	* @details
	* @param dlistHandle 双链表handle
	* @param data 匹配参数
	* @param cb 匹配回调函数
	* @return
	* -             !=NULL    -   成功查找到节点,返回节点数据指针
	* -             =NULL     -   没有匹配到节点,或者参数错误
	*
	* =========================================================*/
	LIBCOMMON_API void * Common_DList_Search(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb);

	/**==========================================================
	* @brief Common_DList_GetCount
	* @note 获得双链表中节点个数
	* @remarks
	* @details
	* @param dlistHandle 双链表handle
	* @return
	* -             >= 0    当前链表节点个数
	* -             < 0      参数错误
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_GetCount(COMMON_DLIST_T dlistHandle);

	/**==========================================================
	* @brief Common_DList_GetFirst
	* @note 获得双链表第一个节点数据
	* @remarks
	* @details
	* @param dlistHandle 双链表handle
	* @return
	* -             !=NULL    返回节点数据指针
	* -             =NULL     没有节点,或者参数错误
	*
	* =========================================================*/
	LIBCOMMON_API void* Common_DList_GetFirst(COMMON_DLIST_T dlistHandle);

	/**==========================================================
	* @brief Common_DList_DeleteWithCk
	* @note 删除双链表第一个没有引用的节点及释放节点数据指针
	* @remarks
	* @details
	* @param dlistHandle
	* @return
	*  -            =0   成功 , 链表中没有节点,也返回成功
	*  -            <0   参数错误
	*
	* =========================================================*/
	LIBCOMMON_API S32 Common_DList_DeleteWithCk(COMMON_DLIST_T dlistHandle);

	/**==========================================================
	* @brief Common_DList_GetNode
	* @note
	* @remarks
	* @details
	* @param dlistHandle
	* @param idx
	* @return
	* -             !=NULL    返回节点数据指针
	* -             =NULL     没有节点,序号超过链表节点总数,或者参数错误
	*
	* =========================================================*/
	LIBCOMMON_API void * Common_DList_GetNode(COMMON_DLIST_T dlistHandle, S32 idx);

	LIBCOMMON_API void * Common_DList_LockNode(COMMON_DLIST_T dlistHandle, void **nodeHandle);
	LIBCOMMON_API S32 Common_DList_UnlockNode(COMMON_DLIST_T dlistHandle,void *data);
	LIBCOMMON_API void * Common_DList_Detach(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb);
    LIBCOMMON_API S32 Common_DList_Separate(COMMON_DLIST_T dlistHandleSrc, COMMON_DLIST_T dlistHandleDst, void *data,COMMON_LIST_COMPARE_F cb);
    LIBCOMMON_API S32 Common_DList_DeleteMulti(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb);
#ifdef __cplusplus
};
#endif
#endif

