#ifndef __OVFS_WEB_REST_H__
#define __OVFS_WEB_REST_H__

#include "libcommon_api.h"
#include "libaccess_api.h"
#include "ovfs_web_define.h"
#include "AntsWebCommon.h"
#include "ovfs_web_ngx.h"
#include "libnetwork_sdk.h"

extern AccessHandle_T g_AccessHandle;

/* WEB FUN_DEBUGLOG END */
#ifdef __cplusplus
extern "C"{
#endif

//User Manage
extern cJSON_Struct *g_ovfs_config;

typedef struct OVFS_WEB_DEVICEINFO
{
    int bPTZ;
    COMMON_DLIST_T DiskList;
    COMMON_DLIST_T DiskFormatList;
}OVFS_WEB_DEVICEINFO_T;

typedef struct
{
    int type;
    char protocolName[32];
    int bEnable;
}WEB_PLATFORM_T;

typedef struct
{
    enum web__DiscoveryMode discovery_mode;
    COMMON_DLIST_T sockList;
    /* int sockfd_dis; */
	int port;
    int support_new_upgrade;
}WEB_DISCOVERY_T;

typedef enum {
	FULL=0,
	W4_H3,
	W16_H9,
	ORIGIN_IMAGE
}WND_MODE_T;

typedef enum {
	I8=0,
	AVI
}REC_FILE_FORMAT_T;

typedef enum {
	REALTIME=0,
	SMOOTH=30
}PREVIEW_BUFFER_RANGE_T;



typedef struct OVFS_WEB_PLUGIN_PARAMS{
	WND_MODE_T wm;
	char* prev_capture_path;
	char* pb_capture_path;
	char* file_capture_path;
	char* backup_path;
	char* rec_path;

	char* plate_pics;
	char* face_pics;

	REC_FILE_FORMAT_T rec_file_format;
	int prev_buf_val;
}OVFS_WEB_PLUGIN_PARAMS_T;

typedef struct{
    int status;
    char ip[16];
    int port;
    char sn[20];
}OVFS_AUDIO_BROADCAST_T;



typedef struct OVFS_WEB_CONTEXT
{
    int version;
#if defined(WEB_PRINT_DEBUG)
    const char *logfile[WEB_MAXLOGS];
    FILE *fdebug[WEB_MAXLOGS];
#endif
    char updatePath[256];

	//whf new add config params
	int httpport;
	int enable_http;

    int httpsport;
	int enable_https;
    int https_support;
	char* custom_support_protocols;
	char* custom_ciphers;
	int enbale_http_redirect_to_https;
    int password_tips;

	OVFS_WEB_PLUGIN_PARAMS_T plugin_params;

	pthread_t param_check;
    COMMON_DLIST_T pNonceList;
    COMMON_DLIST_T subscribeList;
    COMMON_DLIST_T loginFailedList;
    COMMON_DLIST_T userLoginList;
    OVFS_WEB_DEVICEINFO_T devInfo;
    WEB_DISCOVERY_T discovery;

	int enable_T8S;
    int enable_TST;
    int enable_HK;
    int support_HK;

    MUTEX_TYPE hLoginFailedLock;
	MUTEX_TYPE hReqSessionLock;
	MUTEX_TYPE hReqPtzCruiseCallLock;
	MUTEX_TYPE hReqPtzTackCallLock;
	int enable_single_account_login_mode;
	int enable_session;
    int session_count;
    int session_timeout;
    int debugPrint;
    char* face_picture_format;//ÊâπÈáè‰∫∫ËÑ∏ÂØºÂÖ•Êó∂ Ëß£ÊûêÊñá‰ª∂ÂêçÁöÑËßÑÂàô
	unsigned char maxTryCount;//
	unsigned int maxCoolTime;
	/*
	"ipc_normal","ipc_dome","ipc_netdome","ipc_fisheye","ipc_temperature" "ipc_guard"
	*/

	char DeviceTypeString[32];
	cJSON_Struct* fnInfo;

    OVFS_AUDIO_BROADCAST_T broadcast;

    int support_ipv6;
    int need_send_devicestatus;
    cJSON_Struct* pDeviceStatus;

}OVFS_WEB_CONTEXT_T;

/*typedef int (*EXTER_NETWORKSDK_INIT_F)();
typedef int (*EXTER_NETWORKSDK_UNINIT_F)();
typedef int (*EXTER_NETWORKSDK_CONFIG_F)(void *indata, void **outdata);
typedef struct
{
    EXTER_NETWORKSDK_INIT_F   init;
    EXTER_NETWORKSDK_UNINIT_F   uninit;
    EXTER_NETWORKSDK_CONFIG_F   config;
}NETWORKSDK_EXTERNAL_LIBS_T;*/

extern OVFS_WEB_CONTEXT_T *g_ovfs_web;
extern pthread_attr_t g_thread_attr;
extern NETWORKSDK_EXTERNAL_LIBS_T s_networkfuncCb;

OVFS_WEB_CONTEXT_T *ovfs_web_new();

#if defined(WEB_PRINT_DEBUG)
void web_open_logfile(OVFS_WEB_CONTEXT_T *web, int i);
void web_close_logfiles(OVFS_WEB_CONTEXT_T *web);
void web_set_logfile(OVFS_WEB_CONTEXT_T *web, int i, const char *logfile);
void web_set_recv_logfile(OVFS_WEB_CONTEXT_T *web, const char *logfile);
void web_set_sent_logfile(OVFS_WEB_CONTEXT_T *web, const char *logfile);
void web_set_test_logfile(OVFS_WEB_CONTEXT_T *web, const char *logfile);
#endif

/*********************************************************** DList Struct *******************************************************************/

// web string List
typedef struct
{
    char *str;
}WEB_DLIST_STRING_T;

// NonceList Node Data
typedef struct
{
    char *nonce;
    int ncount;
    time_t time_gen;
}WEB_NONCE_NODE_T;

// NonceList Disk Data
typedef struct
{
    int diskNo;
    char *disk_uri;
    char *path;
    int use_type;
    int format;
}WEB_DISK_NODE_T;

// NonceList DiskFormat Data
typedef struct
{
    int diskNo;
    int progress;
}WEB_DISKFORMAT_NODE_T;

// NonceList Subscribe Data
typedef struct
{
    char *uri;
    int bFirst;
    int toId;
}WEB_SUBSCRIBE_NODE_T;

// NonceList LoginFailedList Data
typedef struct
{
    char *ip;
    char *username;
    int count;
    int blocked;
    int remain_time;
}WEB_LOGINFAILED_NODE_T;

// NonceList LoginSucessList Data
typedef struct
{
	long session_id;
    long start_time;
    long current_time;
}WEB_LOGINSUCCESS_NODE_T;


/********************************************************** DList Struct End************************************************************************/
//User Custome_UI
typedef struct
{
    int b_enable;

    u_int bDevice_info:2;
    u_int bDevice_ptz:2;
    u_int bDevice_qrcode:2;
    u_int bRes:26;
}WEB_CUSTOM_PARAM_LEFT_DEVICE_T;

typedef struct
{
    int b_enable;

    u_int bChannel_Display:2;
    u_int bChannel_VideoParam:2;
    u_int bChannel_Sensor:2;
    u_int bChannel_Motion:2;
    u_int bChannel_VideoLost:2;
    u_int bChannel_TamperAlarm:2;
    u_int bChannel_Mask:2;
    u_int bChannel_LanSearch:2;
    u_int bChannel_CountWire:2;
    u_int bChannel_Detectwire:2;
    u_int bChannel_DetectRegion:2;
    u_int bChannel_ObjectRegion:2;
    u_int bChannel_FaceDetect:2;
    u_int bChannel_FireDetect:2;
    u_int bChannel_VideoDiagnose:2;
    u_int bChannel_PlateDetect:2;

    u_int bChannel_Sound:2;
    u_int bChannel_MotionDetect:2;
    u_int bRes:28;
}WEB_CUSTOM_PARAM_LEFT_CHANNEL_T;

typedef struct
{
    int b_enable;

    u_int bNet_Display:2;
    u_int bNet_DDNS:2;
    u_int bNet_NTP:2;
    u_int bNet_Email:2;
    u_int bNet_Wifi:2;
    u_int bNet_Platform:2;
    u_int bNet_Multicast:2;
    u_int bNet_Ftp:2;
    u_int bNet_Others:2;
    u_int bRes:14;
}WEB_CUSTOM_PARAM_LEFT_NET_T;

typedef struct
{
    int b_enable;

    u_int bAlarm_AlarmInput:2;
    u_int bAlarm_AlarmOut:2;
    u_int bAlarm_Exception:2;
    u_int bAlarm_Others:2;
    u_int bRes:24;
}WEB_CUSTOM_PARAM_LEFT_ALARM_T;

typedef struct
{
    int b_enable;

    u_int bUsermgn_UserSet:2;
    u_int bUsermgn_Online:2;
    u_int bUsermgn_Others:2;
    u_int bRes:26;
}WEB_CUSTOM_PARAM_LEFT_USERMANAGE_T;

typedef struct
{
    int b_enable;

    u_int bSystem_Update:2;
    u_int bSystem_AutoReboot:2;
    u_int bSystem_Info:2;
    u_int bSystem_HDD:2;
    u_int bSystem_Recovery:2;
    u_int bSystem_LocalConfig:2;
    u_int bRes:20;
}WEB_CUSTOM_PARAM_LEFT_SYSTEM_T;

typedef struct
{
    WEB_CUSTOM_PARAM_LEFT_DEVICE_T device;
    WEB_CUSTOM_PARAM_LEFT_CHANNEL_T channel;
    WEB_CUSTOM_PARAM_LEFT_NET_T net;
    WEB_CUSTOM_PARAM_LEFT_ALARM_T alarm;
    WEB_CUSTOM_PARAM_LEFT_USERMANAGE_T usermanage;
    WEB_CUSTOM_PARAM_LEFT_SYSTEM_T system;
}WEB_CUSTOM_PARAM_LEFT_T;

typedef struct
{
    WEB_CUSTOM_PARAM_LEFT_T left_frame;
}WEB_CUSTOM_UI_T;

//extern WEB_CUSTOM_UI_T g_ovfs_ui_custom;
extern cJSON_Struct *g_ovfs_uiconfig;

//AlarmReprot ONVIF_METADATA∑Ω Ω
typedef struct
{
    unsigned int u32DataLen;
    char *dataBuffer;
}WEB_METASTREAMDATA_T;

typedef enum
{
    REST_GET,
    REST_PUT,
    REST_POST,
    REST_DELETE,
}WEB_RESTMETHOD_E;

typedef struct
{
    char *Realm;
    char *Qop;
    char *Nonce;
    char *Opaque;
    char *Cnonce;
    char *Uri;
    char *Response;
    char *Nc;
    char *Method;
}WEB_DIGEST_AUTH_T;

typedef struct
{
    char *nonce;
    char *created;
    char *passwdDigest;
}WEB_USERNAMETOKEN_AUTH_T;

typedef struct
{
    int Type;
    char *UserName;
    char *Password;
    WEB_DIGEST_AUTH_T digest;
    WEB_USERNAMETOKEN_AUTH_T userToken;
}WEB_AUTH_T;

typedef struct
{
    char Ipv4[16];
    char Ipv6[16];
    char Mac[32];

}WEB_IPADDR_T;

typedef struct
{
    WEB_RESTMETHOD_E Method;
    char *ToUri;
    WEB_AUTH_T Auth;
    int IsRemote;
    WEB_IPADDR_T IpAddr;
    cJSON_Struct *Data;
}WEB_REST_INPARAM_T;

typedef struct
{
    char *url;
    char *method;
    cJSON_Struct *Data;
}WEB_REST_FUNC_CALL_IN_T;

typedef struct
{
    char *url;
    int codeNum;
    cJSON_Struct *Data;
}WEB_REST_FUNC_CALL_OUT_T;

#define WEEK_DAY_NUM 7
#define DAY_SEGMENT_NUM 8

typedef struct
{
    int	StartHour;
    int	StartMin;
    int	StopHour;
    int	StopMin;
}WEB_ALARM_TIME, *LPWEB_ALARM_TIME;

int Ovfs_Fastcgi_Start(ANTS_WEBSITESDK_CONFIG *cfg);
void Ants_Fastcgi_Stop(ANTS_WEBSITESDK_CONFIG *cfg);
int Ovfs_Web_RestMethod(AccessHandle_T ModuleHandle, WEB_REST_INPARAM_T *InParam, cJSON_Struct **OutParam,int TimeOut);

const char *Ovfs_Web_StrError(int error);
// Ovfs_Web_MakeHeader,…˙≥…rest«Î«ÛµƒheaderΩ·ππ.
// ¥À ±inparam÷–ø…“‘√ª∂®“Âmethod∫Õuri,‘Ú…˙≥…header÷–“≤√ª”–.ø…“‘—”≥ŸµΩ∫Û√Êµ˜”√Ovfs_Web_UpdateHeaderMethodUri ±»∑∂®method∫Õuri.
int Ovfs_Web_MakeHeader(WEB_REST_INPARAM_T *InParam, cJSON_Struct **header);
int Ovfs_Web_UpdateHeader(cJSON_Struct *header, int method, const char *uri);
// Ovfs_Web_RestMethodA,µ˜”√rest∑Ω∑®.
// ∆‰÷–,header∫Õdata∂º «µ˜”√’ﬂ…Í«Î∫Õ Õ∑≈µƒ.outParam «‘⁄¥ÀΩ”ø⁄÷–…Í«Î∂¯µ˜”√’ﬂ Õ∑≈µƒ.
int Ovfs_Web_RestMethodA(cJSON_Struct *header, cJSON_Struct *data, cJSON_Struct **outParam, int TimeOut);

int ovfs_web_parse_ui_custom(const char *path, cJSON_Struct **pResult);

// AlarmTime Translate (7x8)
int ovfs_web_alarmtime_trans2web(WEB_REST_INPARAM_T inparam, const char *uri_path, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt);
int ovfs_web_alarmtime_trans2webA(cJSON_Struct *header, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt,cJSON_Struct *inParam);
int ovfs_web_alarmtime_trans2webA_extB(cJSON_Struct *header, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt);

int ovfs_web_alarmtime_trans2local(WEB_REST_INPARAM_T inparam, const char *uri_path, int device_index, int chanel_index, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt);
int ovfs_web_alarmtime_trans2localA(cJSON_Struct *header, int device_index, int chanel_index, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt,cJSON_Struct *data);

// AlarmTime Translate (7x1)
int ovfs_web_alarmtime_trans2web_ext(WEB_REST_INPARAM_T inparam, const char *uri_path, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt);
int ovfs_web_alarmtime_trans2web_extA(cJSON_Struct *header, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt,cJSON_Struct *inParam);
int ovfs_web_alarmtime_trans2local_ext(WEB_REST_INPARAM_T inparam, const char *uri_path, int device_index, int chanel_index, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt);
int ovfs_web_alarmtime_trans2local_extA(cJSON_Struct *header, int device_index, int chanel_index, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt,cJSON_Struct *data);
int ovfs_web_alarmtime_trans2local_extB(cJSON_Struct *header, int device_index, int chanel_index, cJSON_Struct *p_json,OVFS_WEB_OPTION_S *opt);

// value translate to index
int ovfs_web_value_to_index(int *arry, int arry_size, int value);

void ovfs_web_rest_dispatch(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response);
// Web REST API
void ovfs_web_rest_get_help(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response);
void ovfs_web_rest_get_port(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response);
void ovfs_web_rest_put_port(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response);

void web_list_string_nodefree(void *data);
int web_list_string_nodecompare(void *a, void *b);

//Nonce List Operator
void web_nonce_nodefree(void* data);
int web_nonce_nodecompare(void *a, void *b);
void nonceList_check();

//Subscripe List Operator
void web_subscribe_nodefree(void* data);
int web_subscribe_nodecompare(void *a, void *b);
void subscribeList_done();

//loginfailed Operator
void web_loginfailed_nodefree(void* data);
int web_loginfailed_nodecompare(void *a, void *b);
void loginFailedList_done();


//loginfailed Operator
void web_loginsuccess_nodefree(void* data);
int web_loginsuccess_nodecompare(void *a, void *b);
//int find_item_by_username(void *a, void *b);




//Platform List Operator
void web_platform_nodefree(void* data);
int web_platform_nodecompare(void *a, void *b);
void web_restore_config_platform();

//Disk List Operator
void web_disk_nodefree(void* data);
int web_disk_nodecompare(void *a, void *b);

//DiskFormat List Operator
void web_diskformat_nodefree(void* data);
int web_diskformat_nodecompare(void *a, void *b);

#ifdef __cplusplus
};
#endif
#endif
