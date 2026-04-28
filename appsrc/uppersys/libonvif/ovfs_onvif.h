/**
 * @file ovfs_onvif.h
 * @date create on:
 * @author eric
 * @brief
 *
 * @defgroup
 * @{
 *  @note
 *
 */
#ifndef OVFS_ONVIF_H_
#define OVFS_ONVIF_H_

typedef     unsigned int    DWORD;
typedef     unsigned short  WORD;

#include <libcommon_api.h>
#include <libstreamqueue_api.h>
#include <libaccess_api.h>
#include <cjson.h>
#include <common_json_str_ops.h>


#ifndef NULL
#define NULL             (void *)0
#endif

#define    ONVIF_MALLOC(x)              Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define    ONVIF_REALLOC(addr,x)        Common_Realloc(addr,x,__func__,__LINE__)
#define    ONVIF_FREE(x)                Common_Free(x,__func__,__LINE__)
#define    ONVIF_STRDUP(x)              Common_StrDup(x,__func__,__LINE__)

#define    ONVIF_MSG_HEAD_LEN           1024
#define    ONVIF_MSG_BUF_LEN            1024*8
#define    ONVIF_MAX_MSG_BUF_LEN        1024*30

#define    ONVIF_SUBSCRIBE_MAX_NUM      16
#define    ONVIF_URI_MAX_LEN            256

#define	   ONVIF_NAMESTR_MAX_LEN		80

enum
{
    ONVIF_ASYNC_TASK_REST_INIT = 0,
    ONVIF_ASYNC_TASK_MGR_INIT,
    ONVIF_ASYNC_TASK_MGR_START,
    ONVIF_ASYNC_TASK_MGR_RESTART,
    ONVIF_ASYNC_TASK_EVENT_NOTIFY,
    ONVIF_ASYNC_TASK_EVENT_SEND,
    ONVIF_ASYNC_TASK_MGR_GETCFG,
    ONVIF_ASYNC_TASK_MGR_FIXEDIP,
    ONVIF_ASYNC_TASK_MAX,
};

enum
{
    ONVIF_TIMER_REGULAR_CHECK = 0,
    ONVIF_TIMER_EVENT_NOTIFY,
    ONVIF_TIMER_EVENT_CHECK,
    ONVIF_TIMER_FIXEDIP_CHECK,
    ONVIF_TIMER_MAX,
};

enum
{
    ONVIF_MSG_REST_INIT_FAIL = 0,
    ONVIF_MSG_REST_INIT_DONE,
    ONVIF_MSG_MGR_INIT_DONE,
    ONVIF_MSG_MGR_INIT_FAIL,
    ONVIF_MSG_MGR_START_DONE,
    ONVIF_MSG_MGR_START_FAIL,

    ONVIF_MSG_MGR_RESTART,
    ONVIF_MSG_MGR_RESTART_DONE,
};

enum
{
    ONVIF_REQ_CFG_LOAD = 0,
    ONVIF_REQ_GET_CFG,
    ONVIF_REQ_GET_CFG_STRUCT,
    ONVIF_REQ_SET_CFG,
    ONVIF_REQ_SET_CFG_STRUCT,
    ONVIF_REQ_RTSP_GET_APP_DATA,
    ONVIF_REQ_RTSP_SET_APP_DATA,
};


#define ONVIF_XML_ACTION_NOAUTH_REQ 0
#define ONVIF_XML_ACTION_AUTH_REQ 1

enum
{
#define    ONVIF_XML_ACTION_GET_DEVICE_INFO_STR   "GetDeviceInformation"
    ONVIF_XML_ACTION_GET_DEVICE_INFO_ID = 0,

#define    ONVIF_XML_ACTION_GET_DNS_STR           "GetDNS"
    ONVIF_XML_ACTION_GET_DNS_ID,

#define    ONVIF_XML_ACTION_GET_CAPBILITY_STR     "GetCapabilities"
    ONVIF_XML_ACTION_GET_CAPBILITY_ID,

#define    ONVIF_XML_ACTION_GET_SCOPE_STR         "GetScopes"
    ONVIF_XML_ACTION_GET_SCOPE_ID,

#define    ONVIF_XML_ACTION_SET_SCOPE_STR         "SetScopes"
	ONVIF_XML_ACTION_SET_SCOPE_ID,

#define    ONVIF_XML_ACTION_GET_DATE_TIME_STR     "GetSystemDateAndTime"
    ONVIF_XML_ACTION_GET_DATE_TIME_ID,

#define    ONVIF_XML_ACTION_GET_NETWORK_IF_STR    "GetNetworkInterfaces"
    ONVIF_XML_ACTION_GET_NETWORK_IF_ID,

    /*media 1 and media 2*/
#define    ONVIF_XML_ACTION_GET_PROFILES_STR      "GetProfiles"
    ONVIF_XML_ACTION_GET_PROFILES_ID,

#define    ONVIF_XML_ACTION_GET_SERVICE_STR       "GetServices"
    ONVIF_XML_ACTION_GET_SERVICE_ID,

#define    ONVIF_XML_ACTION_GET_VIDEO_SOURCE_STR  "GetVideoSources"
    ONVIF_XML_ACTION_GET_VIDEO_SOURCE_ID,

#define    ONVIF_XML_ACTION_GET_PROFILE_STR       "GetProfile"
    ONVIF_XML_ACTION_GET_PROFILE_ID, //9

#define    ONVIF_XML_ACTION_GET_STREAM_URI_STR    "GetStreamUri"
    ONVIF_XML_ACTION_GET_STREAM_URI_ID,

#define    ONVIF_XML_ACTION_GET_VIDEO_SOURCE_CFG_STR    "GetVideoSourceConfiguration"
    ONVIF_XML_ACTION_GET_VIDEO_SOURCE_CFG_ID,

#define    ONVIF_XML_ACTION_GET_OSDS_STR           "GetOSDs"
    ONVIF_XML_ACTION_GET_OSDS_ID,
#define    ONVIF_XML_ACTION_SET_OSD_STR           "SetOSD"
    ONVIF_XML_ACTION_SET_OSD_ID,

#define ONVIF_XML_ACTION_GET_IMAGE_SET_STR    "GetImagingSettings"
    ONVIF_XML_ACTION_GET_IMAGE_SET_ID,

#define ONVIF_XML_ACTION_GET_OPTIONS_STR    "GetOptions"
    ONVIF_XML_ACTION_GET_OPTIONS_ID,

#define ONVIF_XML_ACTION_SET_IMAGING_SETTING_STR    "SetImagingSettings"
    ONVIF_XML_ACTION_SET_IMAGING_SETTING_ID,

#define ONVIF_XML_ACTION_GetVideoEncoderConfigurationOptions_STR    "GetVideoEncoderConfigurationOptions"
    ONVIF_XML_ACTION_GetVideoEncoderConfigurationOptions_ID,

#define ONVIF_XML_ACTION_SetVideoEncoderConfiguration_STR    "SetVideoEncoderConfiguration"
    ONVIF_XML_ACTION_SetVideoEncoderConfiguration_ID,

#define ONVIF_XML_ACTION_SetNetworkInterfaces_STR    "SetNetworkInterfaces"
    ONVIF_XML_ACTION_SetNetworkInterfaces_ID,

#define ONVIF_XML_ACTION_SetNetworkDefaultGateway_STR    "SetNetworkDefaultGateway"
    ONVIF_XML_ACTION_SetNetworkDefaultGateway_ID,


#define ONVIF_XML_ACTION_GetOSDOptions_STR    "GetOSDOptions"
    ONVIF_XML_ACTION_GetOSDOptions_ID, // 20

#define ONVIF_XML_ACTION_GetMoveOptions_STR    "GetMoveOptions"
    ONVIF_XML_ACTION_GetMoveOptions_ID,

#define ONVIF_XML_ACTION_SetSystemDateAndTime_STR    "SetSystemDateAndTime"
    ONVIF_XML_ACTION_SetSystemDateAndTime_ID,

#define ONVIF_XML_ACTION_GetVideoAnalyticsConfigurations_STR    "GetVideoAnalyticsConfigurations"
    ONVIF_XML_ACTION_GetVideoAnalyticsConfigurations_ID,

#define ONVIF_XML_ACTION_GetAnalyticsModules_STR    "GetAnalyticsModules"
    ONVIF_XML_ACTION_GetAnalyticsModules_ID,

#define ONVIF_XML_ACTION_ModifyAnalyticsModules_STR    "ModifyAnalyticsModules"
    ONVIF_XML_ACTION_ModifyAnalyticsModules_ID,

#define ONVIF_XML_ACTION_GetVideoEncoderConfiguration_STR    "GetVideoEncoderConfiguration"
    ONVIF_XML_ACTION_GetVideoEncoderConfiguration_ID, //26

#define ONVIF_XML_ACTION_GetAudioEncoderConfigurationOptions_STR    "GetAudioEncoderConfigurationOptions"
    ONVIF_XML_ACTION_GetAudioEncoderConfigurationOptions_ID,

#define ONVIF_XML_ACTION_GetAudioEncoderConfiguration_STR    "GetAudioEncoderConfiguration"
    ONVIF_XML_ACTION_GetAudioEncoderConfiguration_ID, // 28

#define ONVIF_XML_ACTION_GetRules_STR    "GetRules"
    ONVIF_XML_ACTION_GetRules_ID,

#define ONVIF_XML_ACTION_ModifyRules_STR    "ModifyRules"
    ONVIF_XML_ACTION_ModifyRules_ID,

#define ONVIF_XML_ACTION_SetVideoAnalyticsConfiguration_STR    "SetVideoAnalyticsConfiguration"
    ONVIF_XML_ACTION_SetVideoAnalyticsConfiguration_ID,

#define ONVIF_XML_ACTION_Subscribe_STR    "Subscribe"
    ONVIF_XML_ACTION_Subscribe_ID,

#define ONVIF_XML_ACTION_Renew_STR    "Renew"
    ONVIF_XML_ACTION_Renew_ID,

#define ONVIF_XML_ACTION_Unsubscribe_STR    "Unsubscribe"
    ONVIF_XML_ACTION_Unsubscribe_ID,

#define ONVIF_XML_ACTION_GetServiceCapabilities_STR "GetServiceCapabilities"
    ONVIF_XML_ACTION_GetServiceCapabilities_ID,

#define ONVIF_XML_ACTION_CreateOSD_STR "CreateOSD"
    ONVIF_XML_ACTION_CreateOSD_ID,

#define ONVIF_XML_ACTION_DeleteOSD_STR "DeleteOSD"
    ONVIF_XML_ACTION_DeleteOSD_ID,

#define ONVIF_XML_ACTION_GetEventProperties_STR "GetEventProperties"
    ONVIF_XML_ACTION_GetEventProperties_ID,

#define ONVIF_XML_ACTION_CreatePullPointSubscription_STR "CreatePullPointSubscription"
    ONVIF_XML_ACTION_CreatePullPointSubscription_ID,

#define ONVIF_XML_ACTION_PullMessages_STR "PullMessages"
    ONVIF_XML_ACTION_PullMessages_ID,


#define ONVIF_XML_ACTION_GetAudioSources_STR "GetAudioSources"
    ONVIF_XML_ACTION_GetAudioSources_ID,

#define ONVIF_XML_ACTION_GetAudioSourceConfigurations_STR "GetAudioSourceConfigurations"
    ONVIF_XML_ACTION_GetAudioSourceConfigurations_ID,

#define ONVIF_XML_ACTION_GetSnapshotUri_STR "GetSnapshotUri"
    ONVIF_XML_ACTION_GetSnapshotUri_ID,

#define ONVIF_XML_ACTION_GetOSD_STR "GetOSD"
    ONVIF_XML_ACTION_GetOSD_ID,

    /*GPS 定制*/
#if (defined ONVIF_EXT_GPS)
#define ONVIF_XML_ACTION_PushAnalogGpsInfo_STR "pushAnalogGpsInfo"
    ONVIF_XML_ACTION_PushAnalogGpsInfo_ID,

#define ONVIF_XML_ACTION_PushStationInfo_STR "pushStationInfo"
    ONVIF_XML_ACTION_PushStationInfo_ID,
#endif


#define ONVIF_XML_ACTION_GetNetworkProtocols_STR "GetNetworkProtocols"
    ONVIF_XML_ACTION_GetNetworkProtocols_ID,

#define ONVIF_XML_ACTION_GetVideoEncoderConfigurations_STR    "GetVideoEncoderConfigurations"
    ONVIF_XML_ACTION_GetVideoEncoderConfigurations_ID,

#define ONVIF_XML_ACTION_GetAudioEncoderConfigurations_STR    "GetAudioEncoderConfigurations"
    ONVIF_XML_ACTION_GetAudioEncoderConfigurations_ID,

#define ONVIF_XML_ACTION_GetConfigurations_STR "GetConfigurations"
	ONVIF_XML_ACTION_GetConfigurations_ID,

//GetConfiguration
#define ONVIF_XML_ACTION_GetConfiguration_STR "GetConfiguration"
		ONVIF_XML_ACTION_GetConfiguration_ID,

//GetConfigurationOptions
#define ONVIF_XML_ACTION_GetConfigurationOptions_STR "GetConfigurationOptions"
		ONVIF_XML_ACTION_GetConfigurationOptions_ID,

//GetPresetTours
#define ONVIF_XML_ACTION_GetPresetTours_STR "GetPresetTours"
		ONVIF_XML_ACTION_GetPresetTours_ID,

//GetPresetTour
#define ONVIF_XML_ACTION_GetPresetTour_STR "GetPresetTour"
		ONVIF_XML_ACTION_GetPresetTour_ID,

//GetPresetTourOptions

//OperatePresetTour

//RemovePresetTour

//GetNode
#define ONVIF_XML_ACTION_GetNode_STR "GetNode"
	ONVIF_XML_ACTION_GetNode_ID,

#define ONVIF_XML_ACTION_GetNodes_STR "GetNodes"
	ONVIF_XML_ACTION_GetNodes_ID,

#define ONVIF_XML_ACTION_GetPresets_STR "GetPresets"
    ONVIF_XML_ACTION_GetPresets_ID,

#define ONVIF_XML_ACTION_SetPreset_STR "SetPreset"
    ONVIF_XML_ACTION_SetPreset_ID,

#define ONVIF_XML_ACTION_GotoPreset_STR "GotoPreset"
    ONVIF_XML_ACTION_GotoPreset_ID,

#define ONVIF_XML_ACTION_DelPreset_STR "RemovePreset"
    ONVIF_XML_ACTION_DelPreset_ID,

#define ONVIF_XML_ACTION_GetStatus_STR "GetStatus"
    ONVIF_XML_ACTION_GetStatus_ID,

#define ONVIF_XML_ACTION_AbsoluteMove_STR "AbsoluteMove"
    ONVIF_XML_ACTION_AbsoluteMove_ID,

#define ONVIF_XML_ACTION_ContinuousMove_STR "ContinuousMove"
    ONVIF_XML_ACTION_ContinuousMove_ID,

#define ONVIF_XML_ACTION_Move_STR "Move"
    ONVIF_XML_ACTION_Move_ID,

#define ONVIF_XML_ACTION_Stop_STR "Stop"
    ONVIF_XML_ACTION_Stop_ID,

#define ONVIF_XML_ACTION_HK_MaskOptions_STR "GetPrivacyMaskOptions"
	ONVIF_XML_ACTION_HK_MaskOptions_ID,

#define ONVIF_XML_ACTION_HK_PrivacyMask_STR "GetPrivacyMasks"
	ONVIF_XML_ACTION_HK_PrivacyMask_ID,

//#define ONVIF_XML_ACTION_HK_GetPatterns_STR "GetPatterns"
//	ONVIF_XML_ACTION_HK_GetPatterns_ID,

//#define ONVIF_XML_ACTION_HK_GetIOInputs_STR "GetIOInputs"
//	ONVIF_XML_ACTION_HK_GetIOInputs_ID,


#define ONVIF_XML_ACTION_SetSynchronizationPoint_STR "SetSynchronizationPoint"
    ONVIF_XML_ACTION_SetSynchronizationPoint,

#define ONVIF_XML_ACTION_GetVideoSourceConfigurations_STR "GetVideoSourceConfigurations"
    ONVIF_XML_ACTION_GetVideoSourceConfigurations,

#define ONVIF_XML_ACTION_GetNTP_STR "GetNTP"
    ONVIF_XML_ACTION_GetNTP,

#define ONVIF_XML_ACTION_GetDiscoveryMode_STR "GetDiscoveryMode"
    ONVIF_XML_ACTION_GetDiscoveryMode,

#define ONVIF_XML_ACTION_GetNetworkDefaultGateway_STR "GetNetworkDefaultGateway"
    ONVIF_XML_ACTION_GetNetworkDefaultGateway,

#define ONVIF_XML_ACTION_GetHostname_STR "GetHostname"
    ONVIF_XML_ACTION_GetHostname,

#define ONVIF_XML_ACTION_SystemReboot_STR "SystemReboot"
    ONVIF_XML_ACTION_SystemReboot,

#define ONVIF_XML_ACTION_GetSupportedAnalyticsModules_STR "GetSupportedAnalyticsModules"
    ONVIF_XML_ACTION_GetSupportedAnalyticsModules,

#define ONVIF_XML_ACTION_GetSupportedRules_STR "GetSupportedRules"
    ONVIF_XML_ACTION_GetSupportedRules,

#define ONVIF_XML_ACTION_AddVideoEncoderConfiguration_STR "AddVideoEncoderConfiguration"
        ONVIF_XML_ACTION_AddVideoEncoderConfiguration,

#define ONVIF_XML_ACTION_GetGuaranteedNumberOfVideoEncoderInstances_STR "GetGuaranteedNumberOfVideoEncoderInstances"
        ONVIF_XML_ACTION_GetGuaranteedNumberOfVideoEncoderInstances,

#define ONVIF_XML_ACTION_GetVideoEncoderInstances_STR "GetVideoEncoderInstances"
            ONVIF_XML_ACTION_GetVideoEncoderInstances,

#define ONVIF_XML_ACTION_GetAudioSourceConfigurationOptions_STR "GetAudioSourceConfigurationOptions"
                ONVIF_XML_ACTION_GetAudioSourceConfigurationOptions,
#define ONVIF_XML_ACTION_GetMetadataConfigurations_STR "GetMetadataConfigurations"
                    ONVIF_XML_ACTION_GetMetadataConfigurations,
#define ONVIF_XML_ACTION_GetAudioOutputConfigurationOptions_STR "GetAudioOutputConfigurationOptions"
                        ONVIF_XML_ACTION_GetAudioOutputConfigurationOptions,
#define ONVIF_XML_ACTION_GetAudioOutputConfigurations_STR "GetAudioOutputConfigurations"
                            ONVIF_XML_ACTION_GetAudioOutputConfigurations,
#define ONVIF_XML_ACTION_GetAudioDecoderConfigurations_STR "GetAudioDecoderConfigurations"
                                ONVIF_XML_ACTION_GetAudioDecoderConfigurations,
#define ONVIF_XML_ACTION_GetAudioDecoderConfigurationOptions_STR "GetAudioDecoderConfigurationOptions"
                            ONVIF_XML_ACTION_GetAudioDecoderConfigurationOptions,

};

#define ONVIF_CFG_DEFAULT                      "{\"AuthEnable\":1,\"OnvifPort\":80,\"AdaptiveIp\":0,\"DiscoveryMode\":0,\"TZ\":{\"TimeZone\":\"UTC\",\"TimeOffset\":800,\"Name\":\"\"},\"DST\":{\"Name\":\"\"},\"BuildTime\":\"2018-1-19 18:16Z\"}"
#define ONVIF_REST_REQ_LOGIN                   "/Access/OnlineUser"
#define ONVIF_REST_REQ_LOGOUT                  "/Access/OnlineUser?SessionId=%d"

enum
{
    ONVIF_AUTH_TYPE_AUTO = 0, //自动
    ONVIF_AUTH_TYPE_TEXT,     //明文
    ONVIF_AUTH_TYPE_DIGEST,   //digest
    ONVIF_AUTH_TYPE_WS,       //WS-UsernameToken
    ONVIF_AUTH_TYPE_SESSIONID,//session id
};

typedef struct
{
    int authEnable;
    int onvifPort;
    int adaptiveIp;
    int timeout;
    int fixedIp;
    char fixedIpAddr[16];
    int discoveryMode;
} ONVIF_MGR_CFG_T;

typedef struct
{
    int sessionId;
    int loginHandle;
    void *auth;
} ONVIF_AUTH_NODE_T;

typedef struct
{
    char *realm;
    char *qop;
    char *nonce;
    char *opaque;
    char *cnonce;
    char *uri;
    char *response;
    char *nc;
    char *method;
} ONVIF_DIGEST_INFO_T;

typedef struct
{
    int hasAuth;
    int authMethod;
    char *userName;
    char *password;
    char ipStr[64];
    ONVIF_DIGEST_INFO_T digest;

    char *nonce;
    char *created;
    char *passwordDigest;
} ONVIF_AUTH_INFO_T;

/*uritool /core/version | tr -d ',' | awk -F ' ' '{if(substr($2,1,1) == "\""){print "char "substr($1,2,length($1)-3)"[64];"}else{print "int "substr($1,2,length($1)-3)";"}}'*/
typedef struct
{
    char DeviceName[64];
    char ProductName[64];
    char DeviceType[64];
    char DeviceModel[64];
    char Country[64];
    char City[64];
    char Web[64];
    char Tel[64];
    char Copyright[64];
    char Manufacturer[64];
    char Brand[64];
    char Customer[64];
    char SensorModel[64];
    int IsOfDome;
    int IsOfIr;
    int LensSupport;
    char LensDrvType[64];
    char LensType[64];
    int IrisSupport;
    char IrisType[64];
    char Version[64];
    int SvnNumber;
    char HardVersion[64];
    char BuildDate[64];
    char ProductDate[64];
    char Hardware[64];
    char UUID[64];
    char AuthMethod[64];
    char SerialNumber[64];
    char Status[64];
    char DateTime[64];

    char SoftwareModel[64];
} ONVIF_CORE_VERSION_T;

typedef struct
{
    int Enable;
    char Server[64];
    int Interval;
} ONVIF_TIME_NTP_T;

typedef struct
{
    int Zone;
    int EnableBias;
    int ZoneBias;
} ONVIF_TIME_ZONE_T;

typedef struct
{
    int Month;
    int WeekIdx;
    int WeekDay;
    int Hour;
    int Min;
} ONVIF_TIME_NODE_T;

typedef struct
{
    int Enable;
    int Mode;
    int Bias;
    ONVIF_TIME_NODE_T StartTime;
    ONVIF_TIME_NODE_T StopTime;
} ONVIF_TIME_DST_T;

typedef struct
{
    int Year;
    int Month;
    int Day;
    int Hour;
    int Min;
    int Sec;
} ONVIF_TIME_T;

typedef struct
{
    ONVIF_TIME_NTP_T ntp;
    ONVIF_TIME_ZONE_T timezone;
    ONVIF_TIME_DST_T dst;
    ONVIF_TIME_T systime;
    ONVIF_TIME_T utc;
} ONVIF_TIME_ALL_T;

typedef enum
{
    SENSOR_SUPPORT_DAY_NIGHT = 0,
    SENSOR_SUPPORT_EXPUSORE,
    SENSOR_SUPPORT_WB,
    SENSOR_SUPPORT_FOUCE,
    SENSOR_SUPPORT_SHARPEN,
    SENSOR_SUPPORT_3DNOICE,
    SENSOR_SUPPORT_WDR,
    SENSOR_SUPPORT_DEFOG,
    SENSOR_SUPPORT_GAMMA,
    SENSOR_SUPPORT_COLOR_STYLE,
    SENSOR_SUPPORT_MIRROR,
    SENSOR_SUPPORT_FLICKER,
    SENSOR_SUPPORT_IR_LIGHT,
    SENSOR_SUPPORT_AUTO_LEN,
    SENSOR_SUPPORT_AUTO_IRIS,
    SENSOR_SUPPORT_COL_ADJ,   // 15
    SENSOR_SUPPORT_SLOW_FREAM,
    SENSOR_SUPPORT_MAX
} OVF_HAL_SENSOR_ABI_E;


typedef struct
{
    unsigned int brightness;
    unsigned int contrast;
    unsigned int saturation;
    unsigned int sharpness;
} ONVIF_IMAGE_SET_T;

typedef struct
{
    int resolution[10][3]; //width high fps
    char encType[6][8];
    int fpsRange[2];
    int bitrateRange[2];
    int Iinterval[2];
    int encQuality[2];
    char profiles[6][8];
} ONVIF_VideoEncoderCapability_T;

typedef struct
{
    int isValid;
    int resolution[3]; //width high fps
    int encQuality;
    int Iinterval;
    int bitrateCtrlMode;
    int encodeFormat;
    int profiles;
    int bitrate;
} ONVIF_VideoEncoderCfg_T;

typedef struct
{
    int rtspPort;
    int httpPort;
    int auth;
    char ipV4[16];
    int port;
    int ttl;
    int bEnable;
} ONVIF_MultiCast_T;


typedef struct
{
    char EthName[32];
    int EnableDhcp;
    char IpAddrV4[32];
    char IpAddrV6[32];
    char IpMaskV4[32];
    char IpMaskV6[32];
    char GatewayV4[32];
    char GatewayV6[32];
    char MacAddr[32];
    int AuthDNS;
    char Dns1V4[32];
    char Dns2V4[32];

} ONVIF_NETWORK_ATTR_T;

typedef struct
{
    int formatId;
    const char *formatStr;
} ONVIF_OSD_MAP_T;

typedef struct
{
    int osdEnable;
    char osdString[128];
    int osdSize;
    float osdX;
    float osdY;
    const ONVIF_OSD_MAP_T *osdDataFormat;
    const ONVIF_OSD_MAP_T *osdTimeFormat;
} ONVIF_OSD_SUB_ATTR_T;

typedef struct
{
    ONVIF_OSD_SUB_ATTR_T channelOsdAttr;
    ONVIF_OSD_SUB_ATTR_T datetimeOsdAttr;
    ONVIF_OSD_SUB_ATTR_T multiOsdAttr;
#if (defined ONVIF_EXT_CUSTOM_OSD)
    ONVIF_OSD_SUB_ATTR_T custOsdAttr[3];
#endif
} ONVIF_OSD_ATTR_T;

typedef struct
{
    int httpPort;
    int httpsPort;
} ONVIF_WEB_ATTR_T;

typedef struct
{
    int Width;
    int Height;
    int Fps;
    int Format;
} ONVIF_VI_ATTR_T;

typedef struct
{
    char ethName[16];
    int bDhcp;
    char ipV4[16];
    char ipMaskV4[16];
    char gateWayV4[16];
    char mac[32];
} ONVIF_NET_INFO_T;

typedef struct
{
    int enable;
    int Sensitivity;
    int blockW;
    int blockH;
    char rect[32 * 32 + 32];
} ONVIF_MOTION_ATTR_T;

typedef struct
{
    ONVIF_VideoEncoderCfg_T stream[3];
    ONVIF_VI_ATTR_T viAttr;
    ONVIF_MOTION_ATTR_T motion;
} ONVIF_PROFILES_T;

typedef struct
{
    char ip[32];
    char uri[32];
    int port;
    unsigned long long int updateTime;
    unsigned int token;
    int isSend;
    int isPullPoint;
} ONVIF_SUBSCRIBE_NODE_T;

typedef struct
{
    char alarmName[64];
    int alarmType;
    int alarmSrcType;
    char devName[64];
    int device;
    int channel;
    int regionId;
    int stream;
    int status;
    struct tm startTime;
    struct tm stopTime;
} ONVIF_ALARM_STATUS_NODE_T;

typedef struct
{
    int alarmStatusCnt;
    ONVIF_ALARM_STATUS_NODE_T *alarmStatus;
} ONVIF_ALARM_STATUS_T;

typedef struct
{
    char availability[4];
    char nsindicator[4];
    char ewindicator[4];
    int satellitenum;
    double latitude;
    double longitude;
    double speed;
    double direction;
    double altitude;
    char ns[32];
    char ew[32];
    char spd[32];
    char station[128];
} ONVIF_GPS_EXT_INFO_T;

typedef struct
{
	char token[32];
	char name[32];
}ONVIF_PTZ_PRESET_INFO_T;

typedef struct
{
	int IsOfDome;
	int LensSupport;
	int presetsNum;
	ONVIF_PTZ_PRESET_INFO_T *presetsAttr;
}ONVIF_PTZ_CAPABILITY_T;

typedef struct
{
	ONVIF_PTZ_CAPABILITY_T ptzInfo;
}
ONVIF_CAPABILITY_SET_T;


/*send print*/
#define DBGS(x,...) do{if (access("/tmp/onvif_debug", F_OK)==0) printf("\e[0;32m" x,##__VA_ARGS__);printf("\e[0m");}while(0)
/*recv print*/
#define DBGR(x,...) do{if (access("/tmp/onvif_debug", F_OK)==0) printf("\e[0;36m" x,##__VA_ARGS__);printf("\e[0m");}while(0)
/*recv unknown print*/
#define DBGW(x,...) do{if (access("/tmp/onvif_debug", F_OK)==0) printf("\e[0;33m" x,##__VA_ARGS__);printf("\e[0m");}while(0)

#include "ovfs_com_timer.h"
#include "ovfs_com_mq.h"
#include "ovfs_com_pthread_pool.h"
#include "ovfs_onvif_rest.h"
#include "ovfs_onvif_mgr.h"

#define ONVIF_MULTIADDR "239.255.255.250"
#define ONVIF_MULTIPORT 3702
#define RTSP_CFG_PATH   "/usr/etc/cfgfiles/MediaServer.json"

#endif /* OVFS_ONVIF_H_ */
/**
 * @}
 */
