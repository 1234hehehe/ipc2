#ifndef _OVFS_WIFI_COMMON_H_
#define _OVFS_WIFI_COMMON_H_

#include <string>
#include <vector>
#include <mutex>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/un.h>
#include <pthread.h>

#include "ovfs_comm_tool.h"
#include "libcommon_api.h"
#include <common_json_str_ops.h>

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define YELLOW "\033[1;33m"

#define PRINT_ERR(x...) printf(RED "[ERR][%s:%d]", __FUNCTION__, __LINE__); printf(x)
#define PRINT_WARN(x...) printf(YELLOW "[DBG][%s:%d]", __FUNCTION__, __LINE__); printf(x)
#define PRINT_DBG(x...) printf(GREEN "[DBG][%s:%d]", __FUNCTION__, __LINE__); printf(x)
#define PRINT_INF(x...) printf(NONE "[INF][%s:%d]", __FUNCTION__, __LINE__); printf(x)


#define PRINT_TIME(x...) \
do \
{ \
    struct timeval tv;\
    gettimeofday(&tv, NULL);\
    PRINT_INF("[%ld.%06ld]", tv.tv_sec, tv.tv_usec); printf(x);\
}\
while (0)

#define MALLOC(x)              Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define REALLOC(x,s)           Common_Realloc(x,s,__func__,__LINE__)
#define FREE(x)                Common_Free(x,__func__,__LINE__)
#define STRDUP(x)              Common_StrDup(x,__func__,__LINE__)


#define AUTH_MODE_WEP   0x01
#define AUTH_MODE_WPA   0x02
#define AUTH_MODE_WPA2   0x04
#define AUTH_MODE_ESS   0x08

typedef enum
{
	CLOSE = 0,
	AP,
	STA,
}WIFI_WORK_MODE;

typedef enum
{
	IDEL = 0,
	SWITCH_MODE,
	CONFIG_STA,
	STA_ADDAP,
	STA_DELAP,
	STA_SCAN,
	STA_SCAN_END,
}WIFI_WORK_STATUS;

typedef enum
{
    STAUTS = 0,
    AP_STATUS,
    STA_STATUS,
    STA_STORE_LIST,
}WIFI_STU_TYPE;

typedef struct 
{
	char ipv4[16];
	char mac[32];
	char reserved[10];
} WIFI_APMODE_STALIST_S;

typedef struct 
{
    char ssid[64];
    char psk[80];
    char ipv4[32];
    char mac[32];
    int auth_type;
}WIFI_AP_STATUS;//AP (Readonly for IPC)
 

typedef struct 
{
	char ssid[64]; //Readonly
	char mac[32]; //Readonly
	int auth_type;  //Readonly
	int signal; //Readonly
}WIFI_SCANAPITEM_S;

typedef enum
{
	FREE = 0,
	ADD,
	DEL,
}WIFI_STA_OPT;

typedef enum
{
	WIFI_WORK_STA_STATUS_DISCONNECTED,//断开

	WIFI_WORK_STA_STATUS_INTERFACE_DISABLED,//接口禁用

	WIFI_WORK_STA_STATUS_INACTIVE,//所有配置项未激活

	WIFI_WORK_STA_STATUS_SCANNING,//扫描

	WIFI_WORK_STA_STATUS_AUTHENTICATING,

	WIFI_WORK_STA_STATUS_ASSOCIATING,//尝试连接中

	WIFI_WORK_STA_STATUS_ASSOCIATED,

	WIFI_WORK_STA_STATUS_4WAY_HANDSHAKE,//认证状态

	WIFI_WORK_STA_STATUS_GROUP_HANDSHAKE,

	WIFI_WORK_STA_STATUS_COMPLETED,//连接完成

    WIFI_WORK_STA_STATUS_FAILED=-1,//获取状态失败
}WIFI_WORK_STA_STATUS_E;

typedef struct 
{
	char ssid[64];
	char psk[80]; 
	char mac[32]; //Readonly
	int auth_type; //Readonly
} WIFI_STA_APAUTHCFG_S;

typedef struct 
{
	int bDhcp; //使用动态或是静态IP
	char ipv4[16];
	char netmask[16];
	char gateway[16];
} WIFI_STA_IPADDRCFG_S;

typedef struct 
{
    int priority;
	WIFI_STA_APAUTHCFG_S authCfg;
	WIFI_STA_IPADDRCFG_S ipAddrCfg;
} WIFI_STA_CONNECTIONCFG_S;

typedef struct{
    char ssid[64];
    WIFI_WORK_STA_STATUS_E Status; //wpa_cli status
    char ipv4addr[16];
    char signal; //0-100
}WIFI_STA_STATUS;

#endif

