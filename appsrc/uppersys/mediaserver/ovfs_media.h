/**
 * @file ovfs_media.h
 * @date create on: 2016年10月13日
 * @author eric
 * @brief
 *
 * @defgroup
 * @{
 *  @note
 *
 */
#ifndef OVFS_MEDIA_H_
#define OVFS_MEDIA_H_

typedef     unsigned int    DWORD;
typedef     unsigned short  WORD;

#include <libcommon_api.h>
#include <libstreamqueue_api.h>
#include <libaccess_api.h>
#include <cjson.h>
#include <common_json_str_ops.h>
#include <common_media_struct.h>

#ifndef NULL
#define NULL             (void *)0
#endif

#define    MEDIA_MALLOC(x)              Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define    MEDIA_CALLOC(n, x)           Common_Calloc(n,x,__func__,__LINE__)
#define    MEDIA_FREE(x)                Common_Free(x,__func__,__LINE__)
#define    MEDIA_STRDUP(x)              Common_StrDup(x,__func__,__LINE__)

#define    MEDIA_AUTH_TMP_FILE          "/tmp/.media"
#define    MEDIA_CFG_FILENAME           "/usr/etc/cfgfiles/MediaServer.json"
#define    MEDIA_CFG_DEFAULT            "{\"MediaServer\":{\"Rtsp\":{\"Enable\":1,\"StreamMaxNum\":10,\"RtspPort\":554,\"HttpPort\":8002,\"Auth\":2,\"MultiCast\":{\"Enable\":0,\"MainVideo\":{\"IP\":\"238.255.0.2\",\"Port\":28080,\"TTL\":255},\"SubVideo\":{\"IP\":\"238.255.0.3\",\"Port\":28084,\"TTL\":255},\"ThirdVideo\":{\"IP\":\"238.255.0.4\",\"Port\":28088,\"TTL\":255},\"MainAudio\":{\"IP\":\"238.255.0.5\",\"Port\":28080,\"TTL\":255},\"SubAudio\":{\"IP\":\"238.255.0.6\",\"Port\":28084,\"TTL\":255},\"ThirdAudio\":{\"IP\":\"238.255.0.7\",\"Port\":28088,\"TTL\":255}}},\"Rtmp\":{\"Enable\":1,\"RtmpPort\":1935},\"MediaDisconnect\":{\"DisconnectIP\":\"0.0.0.0\",\"DeviceId\":0,\"ChannelId\":0,\"StreamId\":0},"\
										"\"RtmpPush\":{\"Client0\":{\"Device0\":{\"Channel0\":{\"Stream0\":{"\
	                                    "\"Enable\":0,\"EnableAudio\":0,\"Url\":\"\""\
										"}}}}},"\
										"\"RtmpPushNotDisturb\":{\"NotDisturbTime\":{},\"ClientList\":[]},"\
	                  					"\"SmartProtocol\":{\"Enable\":0,\"ServerAddr\":\"http://0.0.0.0\",\"HeartBeatInterval\":3,\"EventListMaxLen\":8},"\
	                  					"\"SmartProtocolNotDisturb\":{\"EnableAlarm\":0,\"EnableSmartResult\":0,\"EnableBkgPic\":0,\"EnableConfig\":0,\"NotDisturbTime\":{},\"AlarmList\":[\"All\"]}"\
										"}}"

#define    MEDIA_SUBSCRIBE_MAX_NUM      16
#define    MEDIA_ASYNC_TASK_MAX         32
#define    MEDIA_URI_MAX_LEN            256

#define    MEDIA_REST_REQ_REAL_VIDEO_ADDR         "/BoardSys/Video/LiveStream/Device%d/Channel%d/Stream%d/Address"
#define    MEDIA_REST_REQ_REAL_VIDEO_NALU         "/BoardSys/Video/LiveStream/Device%d/Channel%d/Stream%d/NaluState"
#define    MEDIA_REST_REQ_REAL_AUDIO_ADDR         "/BoardSys/Audio/Aenc/Channel0"
#define    MEDIA_REST_REQ_TALK_ENC_AUDIO_ADDR     "/BoardSys/Audio/Aenc/Channel0"
#define    MEDIA_REST_REQ_TALK_DEC_AUDIO_ADDR     "/BoardSys/Speak/Adec/LiveStream/Channel0/Address"
#define    MEDIA_REST_REQ_RECORD_ADDR             "/Record/Replay/HistoryStream"
#define    MEDIA_REST_REQ_SMART_ADDR              "/SmartServer/Stream/Address"
#define    MEDIA_REST_REQ_VIDEO_ENC_TYPE_ADDR     "/BoardSys/Video/Attribute/Device%d/Channel%d/Stream%d?REAL"
#define    MEDIA_REST_REQ_AUDIO_ATTR_ADDR         "/BoardSys/Audio/Attribute/All"
#define    MEDIA_REST_REQ_ALARM_STATUS_ADDR       "/Alarm/Status"
#define    MEDIA_REST_SUB_ALARM_STATUS_ADDR       "/Alarm/Subscribe/Status"
#define    MEDIA_REST_SUB_AUDIO_STATUS_ADDR       "/BoardSys/Subscribe/Audio/AudioEnable"
#define    MEDIA_REST_REQ_LOGIN                   "/Access/OnlineUser"
#define    MEDIA_REST_REQ_LOGOUT                  "/Access/OnlineUser?SessionId=%d"
#define    MEDIA_REST_REQ_CORE_RESTORE            "/Core/Restore/Update"
#define    MEDIA_REST_REQ_VIDEO_IFRAME            "/BoardSys/Video/ReqIFrame?Device=%d&Channel=%d&Stream=%d"
#define    MEDIA_REST_REQ_BOARD_SYSTEM_ABILITY_ADDR "/Boardsys/Video/Ability/Number"

#define    MEDIA_STREAM_TYPE_REAL            0
#define    MEDIA_STREAM_TYPE_TALKING         1
#define    MEDIA_STREAM_TYPE_RECORD          2

enum
{
    MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN = 0,
    MEDIA_STREAM_QUEUE_ID_VIDEO_SUB,
    MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD,
    MEDIA_STREAM_QUEUE_ID_AUDIO_REAL,
    MEDIA_STREAM_QUEUE_ID_AUDIO_TALK,
    MEDIA_STREAM_QUEUE_ID_SMART,

    MEDIA_STREAM_QUEUE_ID_RECORD,
    MEDIA_STREAM_QUEUE_ID_MAX,
};
#define    MEDIA_STREAM_ID_VIDEO_DEFAULT           MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN
#define    MEDIA_STREAM_ID_VIDEO_MIN               MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN
#define    MEDIA_STREAM_ID_VIDEO_MAX               MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD

#define    MEDIA_CHANNEL_ID_DEFAULT          0
#define    MEDIA_CHANNEL_ID_MIN              0
#define    MEDIA_CHANNEL_ID_MAX              8

#define    MEDIA_STREAM_VIDEO_ENC_TYPE_H264  0
#define    MEDIA_STREAM_VIDEO_ENC_TYPE_H265  1
#define    MEDIA_STREAM_VIDEO_ENC_TYPE_JPEG  3
#define    MEDIA_STREAM_VIDEO_ENC_TYPE_H264_PLUS 7
#define    MEDIA_STREAM_VIDEO_ENC_TYPE_H265_PLUS 8

enum
{
    MEDIA_TIMER_SYSTEM_INFO = 0,
    MEDIA_TIMER_RESTART_RTSP,
    MEDIA_TIMER_RESTART_RTMP,
    MEDIA_TIMER_SAVE_CONFIG,
    MEDIA_TIMER_RESTART_RTMPPUSH,
    MEDIA_TIMER_RESTART_SMARTPROTOCOL,
    MEDIA_TIMER_LIBRECORD_INIT,
    MEDIA_TIMER_MAX
};

enum
{
    MEDIA_REQ_MEDIA_CFG_LOAD = 0,

    MEDIA_REQ_RTSP_GET_CFG,
    MEDIA_REQ_RTSP_GET_CFG_JSON,
    MEDIA_REQ_RTSP_SET_CFG_JSON,

    MEDIA_REQ_RTMP_GET_CFG,
    MEDIA_REQ_RTMP_GET_CFG_JSON,
    MEDIA_REQ_RTMP_SET_CFG_JSON,

    MEDIA_REQ_RTSP_GET_APP_DATA,
    MEDIA_REQ_RTSP_SET_APP_DATA,
    MEDIA_REQ_RESTORE_CFG,
    MEDIA_REQ_GET_BOARD_STATE,
    MEDIA_REQ_SET_BOARD_STATE,

    MEDIA_REQ_DISCON_GET_CFG,
    MEDIA_REQ_DISCON_GET_CFG_JSON,
    MEDIA_REQ_DISCON_SET_CFG_JSON,
    MEDIA_REQ_DISCON_GET_EVENT,
    MEDIA_REQ_DISCON_SET_EVENT,

    MEDIA_REQ_RTMPPUSH_GET_CFG,
    MEDIA_REQ_RTMPPUSH_GET_CFG_JSON,
    MEDIA_REQ_RTMPPUSH_SET_CFG_JSON,

    MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG,
    MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG_JSON,
    MEDIA_REQ_RTMPPUSH_NOTDISTURB_SET_CFG_JSON,

	MEDIA_REQ_SMARTPROTOCOL_GET_CFG,
	MEDIA_REQ_SMARTPROTOCOL_GET_CFG_JSON,
	MEDIA_REQ_SMARTPROTOCOL_SET_CFG_JSON,

    MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_GET_CFG,
    MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_GET_CFG_JSON,
    MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_SET_CFG_JSON,
};

enum
{
    MEDIA_STREAM_CONTROL_PLAY,
    MEDIA_STREAM_CONTROL_STOP,
    MEDIA_STREAM_CONTROL_FORCE_IFRAME,
};

enum
{
    MEDIA_MSG_RTSP_SERVER_INIT_DONE,
    MEDIA_MSG_RTSP_SERVER_INIT_FAIL,
    MEDIA_MSG_RTSP_SERVER_START_DONE,
    MEDIA_MSG_RTSP_SERVER_START_FAIL,
    MEDIA_MSG_RTSP_SERVER_STOP_DONE,
    MEDIA_MSG_RTSP_SERVER_STOP_FAIL,

    MEDIA_MSG_RTMP_SERVER_INIT_DONE,
    MEDIA_MSG_RTMP_SERVER_INIT_FAIL,
    MEDIA_MSG_RTMP_SERVER_START_DONE,
    MEDIA_MSG_RTMP_SERVER_START_FAIL,
    MEDIA_MSG_RTMP_SERVER_STOP_DONE,
    MEDIA_MSG_RTMP_SERVER_STOP_FAIL,

    MEDIA_MSG_REST_INIT_DONE,
    MEDIA_MSG_REST_INIT_FAIL,
    MEDIA_MSG_REST_SUBSCRIBE_CHECK,

    MEDIA_MSG_RTSP_STREAM_OPEN,
    MEDIA_MSG_RTSP_STREAM_CLOSE,

    MEDIA_MSG_RTMP_STREAM_OPEN,
    MEDIA_MSG_RTMP_STREAM_CLOSE,

	MEDIA_MSG_SMARTPROTOCOL_INIT_DONE,
    MEDIA_MSG_SMARTPROTOCOL_INIT_FAIL,
    MEDIA_MSG_SMARTPROTOCOL_START_DONE,
    MEDIA_MSG_SMARTPROTOCOL_START_FAIL,
    MEDIA_MSG_SMARTPROTOCOL_STOP_DONE,
    MEDIA_MSG_SMARTPROTOCOL_STOP_FAIL,

    MEDIA_MSG_WS_INIT_DONE,
    MEDIA_MSG_WS_INIT_FAIL,
    MEDIA_MSG_WS_START_DONE,
    MEDIA_MSG_WS_START_FAIL,
    MEDIA_MSG_WS_STOP_DONE,
    MEDIA_MSG_WS_STOP_FAIL,
};

enum
{
    MEDIA_ASYNC_TASK_RTSP_INIT,
    MEDIA_ASYNC_TASK_RTSP_START,
    MEDIA_ASYNC_TASK_RTSP_STOP,
    MEDIA_ASYNC_TASK_RTSP_RESTART,

    MEDIA_ASYNC_TASK_RTMP_INIT,
    MEDIA_ASYNC_TASK_RTMP_START,
    MEDIA_ASYNC_TASK_RTMP_STOP,
    MEDIA_ASYNC_TASK_RTMP_RESTART,

    MEDIA_ASYNC_TASK_REST_INIT,
    MEDIA_ASYNC_TASK_FILE_MONITOR,

	MEDIA_ASYNC_TASK_RTMPPUSH_INIT,
    MEDIA_ASYNC_TASK_RTMPPUSH_RESTART,

	MEDIA_ASYNC_TASK_SMARTPROTOCOL_INIT,
	MEDIA_ASYNC_TASK_SMARTPROTOCOL_START,
    MEDIA_ASYNC_TASK_SMARTPROTOCOL_STOP,
    MEDIA_ASYNC_TASK_SMARTPROTOCOL_RESTART,

    /*MEDIA_ASYNC_TASK_WS_INIT,
	MEDIA_ASYNC_TASK_WS_START,
    MEDIA_ASYNC_TASK_WS_STOP,
    MEDIA_ASYNC_TASK_WS_RESTART,*/

	MEDIA_ASYNC_TASK_LIBRECORD_INIT,
};

enum
{
    MEDIA_RTSP_STATE_UNINIT = 0,
    MEDIA_RTSP_STATE_STOP,
    MEDIA_RTSP_STATE_START,
};

enum
{
    MEDIA_RTMP_STATE_UNINIT = 0,
    MEDIA_RTMP_STATE_STOP,
    MEDIA_RTMP_STATE_START,
};

enum
{
    MEDIA_RTMPPUSH_STATE_UNINIT = 0,
    MEDIA_RTMPPUSH_STATE_STOP,
    MEDIA_RTMPPUSH_STATE_START,
};


enum
{
    MEDIA_AUTH_TYPE_AUTO = 0, //自动
    MEDIA_AUTH_TYPE_TEXT,     //明文
    MEDIA_AUTH_TYPE_DIGEST,   //digest
    MEDIA_AUTH_TYPE_WS,       //WS-UsernameToken
    MEDIA_AUTH_TYPE_SESSIONID,//session id
};

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
}MEDIA_DIGEST_INFO_T;

typedef struct
{
    int sessionId; // rtspStreamhandle or rtmpStreamHandle

    int authMethod;
    char *userName;
    char *password;
    char ipStr[64];
    MEDIA_DIGEST_INFO_T digest;

} MEDIA_AUTH_INFO_T;

typedef struct
{
    int sessionId;
    int loginHandle;
    void *auth;
} MEDIA_AUTH_NODE_T;

typedef struct
{
    int type;
    int dev;
    int chan;
    int streamid;

    void *auth;
    CoOpen_Param_T *param;
    /*record play info*/
    unsigned int recordPlayStartYear;
    unsigned int recordPlayStartMon;
    unsigned int recordPlayStartDay;
    unsigned int recordPlayStartHour;
    unsigned int recordPlayStartMinute;
    unsigned int recordPlayStartSecond;

    unsigned int recordPlayStopYear;
    unsigned int recordPlayStopMon;
    unsigned int recordPlayStopDay;
    unsigned int recordPlayStopHour;
    unsigned int recordPlayStopMinute;
    unsigned int recordPlayStopSecond;
} MEDIA_REQ_STREAM_INFO_T;

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
    int sendPic;
    struct tm startTime;
    struct tm stopTime;
} MEDIA_ALARM_STATUS_NODE_T;

typedef struct
{
    int alarmStatusCnt;
    MEDIA_ALARM_STATUS_NODE_T *alarmStatus;
} MEDIA_ALARM_STATUS_T;

/*the state of BoardSystem notify*/
typedef struct
{
    int audioState;
    int encState;
} MEDIA_BOARD_STATE_T;

//时区_编号，后面实现的时候再去添加
typedef enum
{
    //后面添加的时区在最后面添加 已经废弃的宏 不要删除
    //枚举值不要随意修改 IE会按枚举值索引语言
    OVFS_TIME_ZONE_WEST_LINE = 0,                  //国际日期变更线西
    OVFS_TIME_ZONE_SAMOA = 1,                       //中途岛,萨摩亚群岛
    OVFS_TIME_ZONE_HAWAII = 2,                      //夏威夷
    OVFS_TIME_ZONE_ALASKA = 3,                      //阿拉斯加
    OVFS_TIME_ZONE_PACIFIC_OCEAN = 4,               //太平洋时间(美国和加拿大)
    OVFS_TIME_ZONE_MOUNTAIN = 5,                    //山地时间(美国和加拿大)
    OVFS_TIME_ZONE_CENTRAL_CANADA = 6,              //中部时间(美国和加拿大)
    OVFS_TIME_ZONE_EASTERN_TIME_CANADA = 7,           //东部时间(美国和加拿大)
    OVFS_TIME_ZONE_CARACAS = 8,                       //加拉加斯
    OVFS_TIME_ZONE_ATLANTIC_CANADA = 9,               //大西洋时间(加拿大)
    OVFS_TIME_ZONE_NEWFOUNDLAND = 10,                 //纽芬兰
    OVFS_TIME_ZONE_GEORGETOWN = 11,                   //乔治敦, 巴西利亚
    OVFS_TIME_ZONE_ATLANTIC_OCEAN = 12,               //中大西洋
    OVFS_TIME_ZONE_ANGLE_ISLANDS = 13,                //佛得角群岛,亚速尔群岛
    OVFS_TIME_ZONE_GREENWICH = 14,                    //格林威治标准时间:都柏林, 爱丁堡, 伦敦, 里斯本
    OVFS_TIME_ZONE_AMSTERDAM = 15,                    //阿姆斯特丹, 柏林, 罗马, 巴黎
    OVFS_TIME_ZONE_ATHENS = 16,                       //雅典, 耶路撒冷 ,伊斯坦布尔
    OVFS_TIME_ZONE_BAGHDAD = 17,                      //巴格达 ,科威特
    OVFS_TIME_ZONE_TEHERAN = 18,                      //德黑兰
    OVFS_TIME_ZONE_MOSCOW = 19,                       //莫斯科, 圣彼得堡, 伏尔加格勒
    OVFS_TIME_ZONE_KABUL = 20,                        //喀布尔
    OVFS_TIME_ZONE_ISB = 21,                          //伊斯兰堡, 卡拉奇 ,塔什干
    OVFS_TIME_ZONE_MADRAS = 22,                       //马德拉斯,钦奈, 加尔各答, 孟买, 新德里
    OVFS_TIME_ZONE_KATHMANDU = 23,                    //加德满都
    OVFS_TIME_ZONE_NOVOSIBIRSK = 24,                  //阿拉木图,达卡,新西伯利亚
    OVFS_TIME_ZONE_RANGOON = 25,                      //仰光
    OVFS_TIME_ZONE_BANGKOK = 26,                      //曼谷, 河内, 雅加达
    OVFS_TIME_ZONE_BEIJING = 27,                      //北京,香港特别行政区,乌鲁木齐,新加坡
    OVFS_TIME_ZONE_OSAKA = 28,                        //首尔,大阪, 札幌, 东京
    OVFS_TIME_ZONE_ADELAIDE = 29,                   //阿德莱德 ,达尔文
    OVFS_TIME_ZONE_CANBERRA = 30,                     //堪培拉, 墨尔本, 悉尼
    OVFS_TIME_ZONE_SOLOMON_ISLANDS = 31,              //所罗门群岛, 新喀里多尼亚
    OVFS_TIME_ZONE_OSKLAND = 32,                      //奥克兰, 惠林顿 ,斐济 ,马加丹
    OVFS_TIME_ZONE_NUKUALOFA = 33,                    //努库阿洛法

    OVFS_TIME_ZONE_UNKNOW = 0x13149527,     //未定义值

}ovfs_time_zone;

typedef struct{
    ovfs_time_zone zone_type;
    S32 zone_hour;
    S32 zone_min;
}ovfs_dst_cap_node;

typedef struct
{
    char disconnectIp[32];
    int deviceId;
    int channelId;
    int streamId;
} MEDIA_DISCONNECT_INFO_T;

#define BOARD_VIDEO_STREAM_MAX  4
#define BOARD_VIDEO_CHAN_MAX 8
#define BOARD_VIDEO_DEV_MAX 4
typedef struct
{
    int restore;
}BOARD_VIDEO_STREAM_ABILITY_T;
typedef struct
{
    int streamNum;
    BOARD_VIDEO_STREAM_ABILITY_T stream[BOARD_VIDEO_STREAM_MAX];
}BOARD_VIDEO_CHANNEL_ABILITY_T;
typedef struct
{
    int chanNum;
    BOARD_VIDEO_CHANNEL_ABILITY_T chan[BOARD_VIDEO_CHAN_MAX];
}BOARD_VIDEO_DEV_ABILITY_T;
typedef struct
{
    int devNum;
    BOARD_VIDEO_DEV_ABILITY_T dev[BOARD_VIDEO_DEV_MAX];
}BOARD_VIDEO_ABILITY_T;
typedef struct
{
    BOARD_VIDEO_ABILITY_T videoAbility;
}BOARD_ABILITY_T;

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
} HTTP_PUSH_CORE_VERSION_T;

typedef struct
{
    int  enable;          /**/
    char serverAddr[128]; /*管理端地址 IP 或者 域名*/
    int  heartInterval;   /*心跳间隔秒数*/
    int  eventListMaxLen;  /*事件发送队列最大节点数*/
} HTTP_PUSH_CFG_T;

#define MAX_TIMESEGMENT		    8       //设备最大时间段数
#define MAX_DAYS				7       //每?

typedef struct
{
    unsigned int	startTime;  //min
    unsigned int	stopTime;   //min
}OVFS_TIME_RANGED_T;

typedef struct
{
    int  enableAlarm;          /**/
    int  enableSmartResult;   /**/
    int  enableBkgPic;   /**/
    int  enableConfig;  /**/
	char  alarmList[16][16];
    OVFS_TIME_RANGED_T NotDisturbTime[MAX_DAYS][MAX_TIMESEGMENT];
} SMART_PROTO_NOTDISTURB_CFG_T;


#include "ovfs_com_mq.h"
#include "ovfs_com_pthread_pool.h"
#include "ovfs_com_timer.h"
#include "ovfs_com_utils.h"
#include "ovfs_media_rest.h"
#include "ovfs_rtsp_mgr.h"
#include "ovfs_rtmp_mgr.h"
#include "ovfs_smart_proto.h"
//#include "ovfs_ws_mgr.h"

#endif /* OVFS_MEDIA_H_ */
/**
 * @}
 */
