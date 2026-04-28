#ifndef ANTS_WEB_COMMON_H
#define ANTS_WEB_COMMON_H

#include "webs.h"

typedef char char_t;

#define RESULT "Result"
#define ERROR_STRING "ErrorString"
#define DATA "Data"
#define DEV "Dev"
#define CH "Ch"
#define TYPE "Type"
#define STATUS_CODE "StatusCode"
#define SAVE_OK "Operation Ok"
#define COPY_CHANNELS  "CopyChannels"
//#define SECURE_WEB_CODE_OK 0
#define SECURE_WEB_CODE_FAILED -1
#define SECURE_WEB_CODE_INVALID_JSON_FORMAT -2
#define SECURE_WEB_CODE_RESOURCE_LIMITED -3
#define SECURE_WEB_CODE_NO_AUTHORIZE -4
#define SECURE_WEB_CODE_LOGIN_LOCKED -5

#ifndef PRINT_DBG
#define PRINT_DBG(x...)  fprintf(stderr, "[WEBS][DBG][%s:%d] ", __FUNCTION__, __LINE__); fprintf(stderr, x)
#endif

enum
{
    WEB_CODE_OK = 0,
    // 以下错误编码,必须保持连续的整数递减. 修改后相应的增加字符串. 参考,s_errorStr 和 Ovfs_Web_StrError().
    WEB_CODE_BASE = -0xA0000,
    WEB_CODE_GeneralMistake = WEB_CODE_BASE-1, // 通常性错误
    WEB_CODE_InvalidArg = WEB_CODE_BASE-2, // 错误的参数
    WEB_CODE_LackingMem = WEB_CODE_BASE-3, // 内存不足
    WEB_CODE_Unauthorized = WEB_CODE_BASE-4, // 没有取得认证
    WEB_CODE_PermissionDenied = WEB_CODE_BASE-5, // 权限不足
    WEB_CODE_BlockingOperation = WEB_CODE_BASE-6, // 操作被阻塞
    WEB_CODE_InvalidJson = WEB_CODE_BASE-7, // json内容错误
    WEB_CODE_InternalMistake = WEB_CODE_BASE-8, // 内部错误
    WEB_CODE_Unsupported = WEB_CODE_BASE-9, // 不支持的功能
    WEB_CODE_LackingThread = WEB_CODE_BASE-10, // 无法创建线程
    WEB_CODE_TaskExist = WEB_CODE_BASE-11, // 同样任务已经存在
    WEB_CODE_FileNotAccess = WEB_CODE_BASE-12, // 文件无法访问
    WEB_CODE_IamBusy = WEB_CODE_BASE-13, // 正忙(通常是无法取得锁)
    WEB_CODE_SingleAccountLogin = WEB_CODE_BASE-14, // 单用户登录
    WEB_CODE_SessionNeedLoginFirst = WEB_CODE_BASE-15, // 单用户登录
    WEB_CODE_InvalidIpMaskGateway = WEB_CODE_BASE-16, // ip 子网掩码 网关 不匹配
    WEB_CODE_PortOccupied = WEB_CODE_BASE-17, // 端口被占用
    WEB_CODE_SessionCountMax = WEB_CODE_BASE-18, // 在线用户已达最大数量
    WEB_CODE_Enabled_DetectWire = WEB_CODE_BASE-21,//-655381,越界检测已启用
    WEB_CODE_Enabled_DetectRegion = WEB_CODE_BASE-22,//-655382,区域入侵已启用
    WEB_CODE_Enabled_StayPerson = WEB_CODE_BASE-23,//-655383,人员逗留已启用
    WEB_CODE_Enabled_IllegalPark = WEB_CODE_BASE-24,//-655384,车辆违停已启用
    WEB_CODE_Enabled_DetectAsent = WEB_CODE_BASE-25,//-655385,人员离岗已启用
    WEB_CODE_Enabled_Retrograde = WEB_CODE_BASE-26,//-655386,车辆逆行已启用
    WEB_CODE_HandleFreed = WEB_CODE_BASE-27,//-655387,句柄超时已被释放
};

//extern pthread_mutex_t mutex_session;
extern pthread_rwlock_t rw_session;

#if WIN32

#define DEBUGV3(format,...) \
	printf("[WEBSDK]File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)

#elif LINUX
#define DEBUGV3(x...) \
	do{printf("[WEBSDK][%s:%s:%d]",__FILE__, __FUNCTION__, __LINE__);printf(x);}while(0)

#define WEB_TMP(x...)  \
	do{printf("[%s:%d]", __FUNCTION__, __LINE__);printf(x);fflush(stdout);}while(0)

#ifdef WEB_DEBUG_PRINT
#define WEB_DEBUG \
    do{printf("[WEB_DEBUG][%s:%s:%d]",__FILE__, __FUNCTION__, __LINE__);printf(x);}while(0)
#else
#define WEB_DEBUG
#endif
#include <sys/time.h>
#endif

extern void PrintBuffer(void *buffer, int len);
extern void PrintBufferWrap(void *buffer, int len, int wrapbyte);

typedef struct
{
    int type; // 请求中的"Type"字段的值.
    int dev; // 请求中的"Dev"字段的值.
    int ch; // 请求中的"Ch"字段的值.
} OVFS_WEB_OPTION_S;

typedef int (*WebsProc)(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

#if LINUX
#include <ants_hostmgr_type.h>

#define DeviceType_DVR                      0x01        /* DVR */
#define DeviceType_NVR                      0x02        /* NVR */
#define DeviceType_IPC                      0x03        /* IPC */
#define NO_AUTHORIZE                        0x01        //错误类型没有权限
#define HOST_MGR_NAME "WebServer"
#define HOST_MGR_ID 20

#define FUNCTION_START do{
#define FUNCTION_END }while(0);

//webserver用到的基础信息
typedef struct {
	int						g_nLoginHandle;		//!成功登录句柄
	int						g_isAlarming;
	unsigned char			g_ChanNum;			//通道数量
	unsigned char			g_byIPChanNum;		//最大数字通道数
	unsigned int            g_byStartChan;		//起始通道
	unsigned char			g_DVRType;          //设备类型
	unsigned char			g_AlarmOutPortNum;  //报警输出端口数量
	unsigned char			g_AlarmInPortNum;   //报警端口数量
	unsigned int            g_Error;            //全局错误提示
	BYTE					sSerialNumber[48];	//序列号
	WORD					wDVRPort;			//端口号
	WORD					wHttpPort;			//HttpPort
	WORD                    wHttpsPort;         /* HTTPS 端口*/
	WORD                    wRTMPPort;          /* RTMP 端口*/
	WORD                    wRTSPPort;          /* RTSP 端口*/
	ANTS_DVR_USER_INFO_EX   g_struUserInfo;     //用户信息结构体
	int                     g_IsStart;
	char g_UpdatePath[256];
    BYTE g_byDishNum;/*硬盘个数*/
	char szRes[19];
    BYTE DVRName[NAME_LEN];/*设备名称*/
}ANTS_WEBSITESDK_CONFIG,*LPANTS_WEBSITESDK_CONFIG;

extern ANTS_WEBSITESDK_CONFIG g_struWebSiteSDKInfo;

#endif


#if WIN32
/*
宏定义声明中 不能使用条件编译 windwos下直接声明为匿名结构体 使得编译通过
*/
typedef struct _XXXX{char a:1;} ANTS_DVR_USER_EX;
typedef struct _XXXX2{char a:1;} ANTS_DVR_USER_INFO_EX;
#endif

//check string is not utf-8 format or not
#include <stdio.h>
#include <stdint.h>
/*#ifndef FALSE
typedef int8_t BOOL;
#define TRUE 1
#define FALSE 0
#endif
*/
typedef struct _UTF8_HEAD{
    uint8_t countOf1;
    uint8_t head;
}UTF8_HEAD;
const static UTF8_HEAD utf8Head[] = {
    {0, 0x0},
    {1, 0x80},
    {2, 0xC0},
    {3, 0xE0},
    {4, 0xF0},
    {5, 0xF8},
    {6, 0xFC},
    {7, 0xFE},
};

uint32_t check_reparse_utf8( char* data, int32_t len,char* buf);

#endif
