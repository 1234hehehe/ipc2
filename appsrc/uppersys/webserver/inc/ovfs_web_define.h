#ifndef OVFS_WEB_DEFINE_H
#define OVFS_WEB_DEFINE_H

#define OVFS_WEB_DIE(mesg,...) {Common_Log_Out(Common_Log_GetDefaultHandle() ,COMMON_LOG_LV_BASE,  __LINE__,__FUNCTION__,"[ERROR]", mesg, ##__VA_ARGS__); exit(1);}

#ifndef IPC_THREAD_STACK_MIN
#define IPC_THREAD_STACK_MIN                 (1024*1024*1)
#endif

#ifndef U_INT_T
#define U_INT_T
typedef unsigned int u_int;
#endif

#ifndef closesocket
#define closesocket 		close
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET 		-1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR 		-1
#endif

#ifndef Utils_SetThreadName
#define Utils_SetThreadName() prctl(PR_SET_NAME,__func__)
#endif

/* WEB FUN_DEBUGLOG  */

#ifndef SOAP_MESSAGE
# define SOAP_MESSAGE fprintf
#endif

#ifdef WEB_PRINT_DEBUG
#define WEB_DBGLOG(DBGFILE, CMD)  \
{ if (g_ovfs_web)\
  { if (!g_ovfs_web->fdebug[WEB_INDEX_##DBGFILE]) \
  		web_open_logfile((OVFS_WEB_CONTEXT_T *)g_ovfs_web, WEB_INDEX_##DBGFILE);\
  	if (g_ovfs_web->fdebug[WEB_INDEX_##DBGFILE]) \
  	{ FILE *fdebug = g_ovfs_web->fdebug[WEB_INDEX_##DBGFILE];\
  	  CMD;\
  	  fflush(fdebug);\
  	}\
  }\
}
#else
#define WEB_DBGLOG(DBGFILE, CMD)
#endif

#ifndef WEB_DBGFUN
# define WEB_DBGFUN(FNAME) WEB_DBGLOG(TEST, SOAP_MESSAGE(fdebug, "%s(%d): %s()\n", __FILE__, __LINE__, FNAME))
#endif

#ifndef WEB_MAXLOGS
# define WEB_MAXLOGS    (3)
# define WEB_INDEX_RECV  (0)
# define WEB_INDEX_SENT  (1)
# define WEB_INDEX_TEST  (2)
#endif

#ifndef WEB_DiscoveryMode
#define WEB_DiscoveryMode
enum web__DiscoveryMode {web__DiscoveryMode__Discoverable = 0, web__DiscoveryMode__NonDiscoverable = 1};
#endif

#ifndef MIN2
#define MIN2(x,y)       ( (x)<(y) ? (x):(y) )
#endif

#ifndef MAX2
#define MAX2(x,y)       ( (x)>(y) ? (x):(y) )
#endif

typedef enum
{
	StxCode_V1 	= 0x000002e8, 
	StxCode_V2 	= 0xCA010000,
	StxCode_V3  	= 0xCB010000,
}DISCOVERY_STXCODE;

typedef enum
{
	EtxCode_V1 	= 0xC7010000, 	
}DISCOVERY_ETXCODE;

typedef enum
{
	Command_Discovery 		= 0x1015,
	Command_ModifyIPAddr 	= 0x1016,
	ANTS_WRITEUMEYEUUID_V2 	=0x101C,
	Command_ModifyMac 	= 0x7001,

	Command_TestTool_GetTestResult = 0xf001,
}DISCOVERY_COMMOND;

enum
{
    REQUEST_DISCOVERY_PACKET_LEN_MIN = 40, 
    REQUEST_DISCOVERY_PACKET_LEN_MAX = 200, 
    REQUEST_MODIFYIP_PACKET_LEN_MIN = 100,
    REQUEST_MODIFYIP_PACKET_LEN_MAX = 1024,
    REQUEST_CUSTOM_PACKET_LEN_MIN = 100,
    REQUEST_CUSTOM_PACKET_LEN_MAX = 1024,
};

enum {IPV4_ADDR_STR_LEN = 16, HW_ADDR_STR_LEN = 18, IPV6_ADDR_STR_LEN = 40};

#define MUTEX_TYPE		pthread_mutex_t
#define MUTEX_INITIALIZER	PTHREAD_MUTEX_INITIALIZER
#define MUTEX_SETUP(x)		pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x)	pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)		pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x) 	pthread_mutex_unlock(&(x))

#endif
