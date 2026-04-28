/*
 * ants_web.h
 *
 *  Created on: 2016-8-1
 *      Author: eric
 */
#ifndef OVFS_ALARM_COMMON_H_
#define OVFS_ALARM_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

//#define MEM_WATCH

#define LOCAL_PORT				15
#define MAX_TIMESEGMENT		    4       //设备最大时间段数
#define MAX_DAYS				7       //每周天数
#define MAX_ALARMOUT            4       //最大报警输出个数

#define MAX_CHANNEL_SUPPORT     1
#define MAX_VI_DEV_SUPPORT      1
#define MAX_STREAM_SUPPORT      2
#define MAX_REGION_SUPPORT      16

#define MAX_NAMELEN 32
#define MAX_USRNAME_LEN			64
#define MAX_PWD_LEN				64
#define MAX_DOMAIN_LEN				128
#define MAX_ADDRESS_LEN			64

#define MODULE_ROOT_NAME			"Alarm"

#define METHOD_GET				0
#define METHOD_PUT				1
#define METHOD_POST				2
#define METHOD_DELETE			3

#define MAX_SENSORALARM         8

#define ALARM_STR_AI 			"AlarmIn"
#define ALARM_STR_MOTION  		"Motion"
#define ALARM_STR_VHIDE			"Vhide"
//#define ALARM_STR_VDIAGNOSE		"Vdiagnose"
//#define ALARM_STR_CNTWIRE		"CounterWire"
#define ALARM_STR_DETWIRE		"DetectWire"
//#define ALARM_STR_DETREGION		"DetectRegion"
#define ALARM_STR_DETABSENT		"DetectAbsent"
//#define ALARM_STR_OBJREGION		"ObjectRegion"
//#define ALARM_STR_DETSOUND 		"SoundDetect"
//#define ALARM_STR_SMOTION		"SmartMotion"
//#define ALARM_STR_DETFIRE		"DetectFire"
//#define ALARM_STR_DETFACE		"DetectFace"
//#define ALARM_STR_DETPLATE		"DetectPlate"
//#define ALARM_STR_RECFACE		"RecognitionFace"
//#define ALARM_STR_TEMPERATURE	"Temperature"
#define ALARM_STR_DISKFULL		"DiskFull"
#define ALARM_STR_DISKERR		"DiskErr"
#define ALARM_STR_NETBREAK		"NetCableBreak"
#define ALARM_STR_IPCONFLICT	"IpConflict"
#define ALARM_STR_ILLACCESS		"IllegallyAcc"
#define ALARM_STR_UNFORMAT		"UnmatchFormat"
#define ALARM_STR_RECERR		"VideoRecordErr"
#define ALARM_STR_CLIENTBREAK	"MediaDisconnect"
//#define ALARM_STR_DETHELMET		"DetectHelmet"
//#define ALARM_STR_HUMAN_TEMP    "HumanTemperature"
//#define ALARM_STR_FACE_MASK     "FaceMask"

#define ALARM_STR_SENSORALARM		"SensorAlarm"
#define ALARM_STR_LOWBATTERYALARM	"LowBatteryAlarm"

//#define ALARM_STR_HUMANOID		"Humanoid"
//#define ALARM_STR_RUN				"Run"
//#define ALARM_STR_VIOLENTMD		"ViolentMotion"
//#define ALARM_STR_MAXHEIGHT		"MaxHeight"
//#define ALARM_STR_HIGHDENSITY		"HighDensity"
#define ALARM_STR_RETROGRADE		"Retrograde"
//#define ALARM_STR_SCENCECHANGE		"ScenceChange"
#define ALARM_STR_DETPERSON		"DetectPerson"
#define ALARM_STR_REGIONALINVASION		"RegionalInvasion"
#define ALARM_STR_PERSONSTAYING		"PersonStaying"
#define ALARM_STR_PARKINGVIOLATION		"ParkingViolation"

//#define ALARM_STR_MOTOR_DETECT  "EBike"

//#define ALARM_SUBSTR_WHITELIST		"RecFaceWhite"
//#define ALARM_SUBSTR_BLACKLIST		"RecFaceBlack"
//#define ALARM_SUBSTR_OTHERLIST		"RecFaceOther"
//#define ALARM_TYPE_BEGIN 0

#define ALARM_TYPE_ALARMIN                     0
#define ALARM_TYPE_MOTION                       1
#define ALARM_TYPE_VHIDE                          2
//#define ALARM_TYPE_VDIAGNOSE                 3
//#define ALARM_TYPE_COUNTER_WIRE           4
#define ALARM_TYPE_DETECT_WIRE              5
//#define ALARM_TYPE_DETECT_REGION           6
//#define ALARM_TYPE_OBJECT_REGION            7
//#define ALARM_TYPE_SOUND_DETECT             8
//#define ALARM_TYPE_SMART_MOTION             9
//#define ALARM_TYPE_DETECT_FIRE                  10
//#define ALARM_TYPE_DETECT_FACE                  11
//#define ALARM_TYPE_DETECT_PLATE                 12
//#define ALARM_TYPE_RECOGNITION_FACE         13
//#define ALARM_TYPE_TEMPERATURE                  14

#define ALARM_TYPE_END          14
#define EXCEPT_TYPE_BEGIN	ALARM_TYPE_END+1

#define ALARM_TYPE_DISKFULL                     15
#define ALARM_TYPE_DISKERR	                       16
#define ALARM_TYPE_CABLE_BREAK              17
#define ALARM_TYPE_IP_CONFLIT                   18
#define ALARM_TYPE_ILLEG_ACCESS             19
#define ALARM_TYPE_UNMATCH_FORMAT       20
#define ALARM_TYPE_RECORD_ERR                 21
#define ALARM_TYPE_CLIENT_BREAK             22

#define EXCEPT_TYPE_END                             22
#define ACTION2_TYPE_START	EXCEPT_TYPE_END +1

//#define ALARM_TYPE_HUMANOID                 23
//#define ALARM_TYPE_RUN                              24
//#define ALARM_TYPE_VIOLENTMD                    25
//#define ALARM_TYPE_MAXHEIGHT                    26
//#define ALARM_TYPE_HIGHDENSITY                  27
#define ALARM_TYPE_RETROGRADE                   28
//#define ALARM_TYPE_SCENCECHANGE                 29
#define ALARM_TYPE_DETECT_PERSON                30
#define ALARM_TYPE_DETECT_ABSENT                31

//#define ALARM_TYPE_DETECT_HELMET                32
//#define ALARM_TYPE_HUMAN_TEMP                      33
//#define ALARM_TYPE_FACE_MASK                        34
//#define ALARM_TYPE_DETECT_MOTOR                 35
#define ALARM_TYPE_REGIONAL_INVASION		36
#define ALARM_TYPE_PERSON_STAYING		37
#define ALARM_TYPE_PARKING_VIOLATION		38
#define ALARM_TYPE_SENSOR_ALARM		39
#define ALARM_TYPE_LOW_BATTERY_ALARM		40

#define ACTION2_TYPE_END            ALARM_TYPE_LOW_BATTERY_ALARM

#define ALARM_TYPE_MAX					ACTION2_TYPE_END+1

//#define AlARM_SUBTYPE_START				ALARM_TYPE_MAX
//#define AlARM_SUBTYPE_RECFACE_START		AlARM_SUBTYPE_START
//#define AlARM_SUBTYPE_RECFACE_WLIST		AlARM_SUBTYPE_RECFACE_START
//#define AlARM_SUBTYPE_RECFACE_BLIST		AlARM_SUBTYPE_RECFACE_WLIST + 1
//#define AlARM_SUBTYPE_RECFACE_OLIST		AlARM_SUBTYPE_RECFACE_BLIST + 1
//#define AlARM_SUBTYPE_RECFACE_END		AlARM_SUBTYPE_RECFACE_OLIST
//#define AlARM_SUBTYPE_END					AlARM_SUBTYPE_RECFACE_END + 1

#define LINKAGE_CFG_TYPE					0
#define ALARM_CFG_TYPE					1
#define MAX_METHOD_LEN					64
#define MAX_CMD_LEN						64

#define LINK_TYPE_MAIL				2
#define LINK_TYPE_RECORD			3
#define LINK_TYPE_SNAP			5
#define LINK_TYPE_AOUT			6
#define LINK_TYPE_PTZ			7
#define LINK_TYPE_SOUND 		8
#define LINK_TYPE_LIGHTALARM    9
#define LINK_TYPE_REDBLUELIGHT    10
#define LINK_TYPE_FTP           11


#define OVFS_RES_TRIGGER		"TriggerCfg"
#define OVFS_RES_LINKAGE		"LinkageCfg"
#define OVFS_RES_COLLECT		"CollectData"
#define OVFS_RES_STATUS		"Status"
#define OVFS_RES_SUBSCRIBE		"Subscribe"

#define OVFS_LABEL_TRIGGER		"TriggerCfg"
#define OVFS_LABEL_LINKAGE		"LinkageCfg"
#define OVFS_LABEL_DEVICE		"Device"
#define OVFS_LABEL_CHANNEL		"Channel"
#define OVFS_LABEL_STREAM		"Stream"
#define OVFS_LABEL_REGION		"Region"

/* 报警 */
//主类型
#define MAJOR_ALARM						0x1
//次类型
#define MINOR_ALARM_IN					0x1		/* 报警输入 */
#define MINOR_ALARM_OUT					0x2		/* 报警输出 */
#define MINOR_MOTDET_START				0x3		/* 移动侦测报警开始 */
#define MINOR_MOTDET_STOP				0x4		/* 移动侦测报警结束 */
#define MINOR_HIDE_ALARM_START			0x5		/* 遮挡报警开始 */
#define MINOR_HIDE_ALARM_STOP			0x6		/* 遮挡报警结束 */
//#define MINOR_COUNTWIRE                 0x7     /* 目标计数 */
//#define MINOR_DETECTWIRE                0x8     /* 过线检测 */
//#define MINOR_DETECTREGION              0x9     /* 区域检测 */
//#define MINOR_OBJECTREGION              0xa     /* 物品检测 */
//#define MINOR_SOUNDALARM                0xb     /* 异常声音 */
//#define MINOR_PLATEDETECT				0xc     /* 车牌检测 */
//#define MINOR_FACEDETECT				0xd     /* 人脸检测 */
//#define MINOR_FIREDETECT				0xe     /* 火焰检测 */
//#define MINOR_VIDEODIAGNOSE_COLOR		0xf     /* 视频诊断_偏色 */
//#define MINOR_VIDEODIAGNOSE_BRIGHT		0x10    /* 视频诊断_偏亮 */
//#define MINOR_VIDEODIAGNOSE_BLUR		0x11    /* 视频诊断_模糊 */
//#define MINOR_MOTION_DETECT				0x12    /* 智能移动侦测 */
//#define MINOR_VIDEODIAGNOSE_DARK		0x13    /* 视频诊断_偏暗 */
//#define MINOR_VIDEODIAGNOSE_VI_LOST		0x14    /* 视频诊断_视频丢失 */
//#define MINOR_TEMPERATURE				0x15    /* 温度报警 */

//#define MINOR_HUMANOID				0x17    /* 人形报警 */
//#define MINOR_RUN						0x18    /* 快跑报警 */
//#define MINOR_VIOLENTMD				0x19    /* 剧烈运动报警 */
//#define MINOR_MAXHEIGHT				0x1A    /* 限高报警*/
//#define MINOR_HIGHDENSITY				0x1B    /* 高密度报警 */
//#define MINOR_RETROGRADE				0x1C    /* 逆行报警 */

//#define MINOR_FACERECOGNITION           0x1d     /* 人脸识别检测 */
//#define MINOR_SCENCECHANGE           0x1e     /* 场景变换 */
#define MINOR_PERSONDETECT           0x1f     /* 人形检测 */
//#define MINOR_DETECTABSENT           0x20     /* 离岗检测 */
//#define MINOR_DETECTEWALL           0x21     /* 区域检测   重命名成    电子围栏 */
//#define MINOR_MOTORDETECT        0x32    /*电动车检测*/

/* 异常 */
//主类型
#define MAJOR_EXCEPTION					0x2
//次类型
//#define MINOR_VI_LOST					0x21	/* 视频信号丢失 */
#define MINOR_ILLEGAL_ACCESS			0x22	/* 非法访问 */
#define MINOR_HD_FULL					0x23	/* 硬盘满 */
#define MINOR_HD_ERROR					0x24	/* 硬盘错误 */
#define MINOR_IP_CONFLICT				0x26	/* IP地址冲突 */
#define MINOR_NET_BROKEN				0x27	/* 网络断开*/
#define MINOR_REC_ERROR                 0x28    /* 录像出错 */
#define MINOR_VI_EXCEPTION              0x2a    /* 视频输入异常(只针对模拟通道) */
#define MINOR_NET_CONNECTED             0x2c    // 网络恢复连接
#define MINOR_CLIENT_DISCONNECT         0x2d    // 客户端断开连接
#define MINOR_CLIENT_CONNECTED          0x2e    // 客户端恢复连接

//#define MINOR_DETHELMET                 0x2f     /* helmet detect */
//#define MINOR_HUMAN_TEMP                0x30     /* human temperature */
//#define MINOR_FACE_MASK                 0x31     /* face mask detect */


#define ARRAYSIZE(ARRAY)	(int)(sizeof(ARRAY)/sizeof(ARRAY[0]))

#define MATHOD_GET		0
#define MATHOD_PUT		1
#define MATHOD_POST	2
#define MATHOD_DELETE	3

#define ovfs_make_result(CODE, DES, DATA, OUTDATA) MakeResult(__FILE__,__FUNCTION__,__LINE__,CODE, DES, DATA, OUTDATA)

#define  ovfs_print_json(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		printf("%s\n",ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)
#define DWORD U32
#define BYTE U8
typedef struct tag_Ovfs_Log
{
    DWORD						dwLogTime;
    DWORD						dwMajorType;					//!主类型 1-报警; 2-异常; 3-操作; 0-全部
    DWORD						dwMinorType;					//!次类型 0-全部;
    BYTE						sUser[MAX_NAMELEN];				//!用户名
    BYTE						sRemoteHostAddr[MAX_NAMELEN];	//!远程主机地址
    DWORD						dwChannel;						//!通道号
    DWORD						dwDiskNum;						//!硬盘号
    DWORD						dwAlarmInPort;					//!报警输入端口
    DWORD						dwAlarmOutPort;					//!报警输出端口
    DWORD						bStatus;						//!报警状态
} OVFS_LOG, *POVFS_LOG;

typedef int  (*OVFS_GET_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_PUT_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_POST_METHOD)(void *,void *,void *,void**);
typedef int  (*OVFS_DELETE_METHOD)(void *,void *,void *,void**);

typedef struct
{
    OVFS_GET_METHOD ovfs_get_method;
    OVFS_PUT_METHOD ovfs_put_method;
    OVFS_POST_METHOD ovfs_post_method;
    OVFS_DELETE_METHOD ovfs_delete_method;
    void *pUserData;
} OVFS_REST_METHOD,*POVFS_REST_METHOD;

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int min;
    int second;

    int weekDay;
    int yearDay;
} OVFS_TIME_T;

typedef struct
{
    char  pName[MAX_USRNAME_LEN];
    char  pPwd[MAX_PWD_LEN];
    char  pSmtpSvr[MAX_DOMAIN_LEN];
    int     port;
    int     bEnSsl;
    int     SvrVerify;
} OVFS_SENDER_INFO;

typedef struct
{
    char  pName[MAX_USRNAME_LEN];
    char  pAddr[MAX_ADDRESS_LEN];
} OVFS_RECEIVE_INFO;

typedef struct
{
    int count;
    OVFS_RECEIVE_INFO  *recver;
} OVFS_RECEIVE_LIST;

typedef struct
{
    OVFS_SENDER_INFO sender;
    OVFS_RECEIVE_LIST recvList;
    int     bAttach;
    int     interval;
} OVFS_MAIL_CFG;


#ifndef SIMPLIFIED
typedef struct
{
    int dataLen;
    int mode; // 0 -不拓展 commonId, 1-拓展 commonId
    int hiddenId;
    int commonId;
    int faceId;
} OVFS_WIEGAND_CFG;
#endif

typedef struct
{
    int detPersonComp; /*human > detectPerson*/
    int regionRename;// 1: ewall
    int relinkAudio;// 0:disable >0 sec
} OVFS_CUSTOM_CFG;

//联动配置结构定义
typedef struct
{
    int     isValid;                ///该标志置起时才配置参数
    int     enable;                 ///使能联动FTP
    int     iCount;
} OVFS_LINK_FTP_CFG;

typedef struct
{
    int     isValid;                ///该标志置起时才配置参数
    int     enable;                 ///使能联动上报中心
    int     iCount;
} OVFS_LINK_CENTER_CFG;

typedef struct
{
    int     isValid;                ///该标志置起时才配置参数
    int     enable;                 ///使能联动邮件
    int     bDefault;                 ///使用默认邮件配置
    OVFS_SENDER_INFO sender;
    OVFS_RECEIVE_LIST recvList;
    int     bAttach;
    int     interval;
    int     iCount;
} OVFS_LINK_MAIL_CFG;

typedef struct
{
    int             isValid;                ///该标志置起时才配置参数
    int             enable;                 ///使能联动录像
    unsigned int    mask[MAX_VI_DEV_SUPPORT];                   ///联动录像的通道掩码
    int     iCount;
} OVFS_LINK_RECORD_CFG;

typedef struct
{
    int             isValid;                        ///该标志置起时才配置参数
    int             enable;                 ///使能联动HTTP
    int     iCount;
} OVFS_LINK_HTTP_CFG;

typedef struct
{
    int             isValid;                        ///该标志置起时才配置参数
    int             enable;                 ///使能联动当前通道
    int             count;                  ///抓图次数
    int             interval;               ///抓图间隔
} OVFS_LINK_SNAP_CFG;

typedef struct
{
    int                isValid;                        ///该标志置起时才配置参数
    int                enable;                           ///使能联动报警输出
    unsigned int       mask[MAX_ALARMOUT];                             ///联动报警输出的通道掩码
    int     iCount;
} OVFS_LINK_AOUT_CFG;

typedef struct
{
    int                isValid;                        ///该标志置起时才配置参数
    int                type;                           ///联动ptz类型0-不使能.1-调用预置点.2-调用巡航.3-调用轨迹.4-调用看守位….
    int                index;                          ///联动ptz的编号
    int     iCount;
} OVFS_LINK_PTZ_CFG;

typedef struct
{
    int isValid;
    int enable;
    int index;
    int index_last;
} OVFS_LINK_SOUND_CFG;

typedef struct
{
    int     isValid;                ///该标志置起时才配置参数
    int     enable;                 ///使能联动灯光报警
} OVFS_LINK_LIGHTALARM_CFG;

typedef struct
{
    int dev;
    int ch;
} OVFS_EXPAND_AOUT_CH_LIST;

typedef struct
{
    int isValid;
    int enable;
    OVFS_EXPAND_AOUT_CH_LIST list[32];
    int iCount;
} OVFS_LINK_EXPAND_AOUT_CFG;

typedef struct
{
    unsigned int	startTime;  //min
    unsigned int	stopTime;   //min
} OVFS_TIME_RANGED_T;

typedef struct
{
    int mode;                       //1-ALL, 2-NIGHT, 3-CUSTOM
    U32 weeklist[MAX_DAYS];
    OVFS_TIME_RANGED_T depSchedTime[MAX_TIMESEGMENT]; //布放时间
} OVFS_SCHEDTIME_TIME_T;

typedef struct
{
    OVFS_LINK_FTP_CFG       linkFtp;
    OVFS_LINK_MAIL_CFG      linkMail;
    OVFS_LINK_RECORD_CFG    linkRecord;
    OVFS_LINK_SNAP_CFG      linkSnap;
    OVFS_LINK_AOUT_CFG      linkAlarmOut;
    OVFS_LINK_EXPAND_AOUT_CFG      linkExpandAlarmOut;
    OVFS_LINK_PTZ_CFG       linkPtz;
    OVFS_SCHEDTIME_TIME_T   shedule;
} OVFS_LINKAGE_CFG,*POVFS_LINKAGE_CFG;

//布防配置结构定义
typedef struct
{
    char *name;
    int count;
    char **plinkCmd;
} OVFS_LINK_METHOD;

typedef struct
{
    OVFS_LINK_METHOD **method;
    int count;
} OVFS_LINK_MAP_LIST;

typedef struct
{
    int streamNum;
} OVFS_CHANNEL_ABILITY;

typedef struct
{
    int chanNum;
    OVFS_CHANNEL_ABILITY **pChanAbility;
} OVFS_DEV_ABILITY;

typedef struct
{
    int alarmInNum;
    int alarmOutNum;
} OVFS_ABILITY,*POVFS_ABILITY;

typedef struct
{
    int				enable;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_TRIGGER_CFG,*POVFS_TRIGGER_CFG;

typedef struct
{
    OVFS_TRIGGER_CFG trigger[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
    OVFS_LINKAGE_CFG linkage[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
} OVFS_ALARM_COMM_CFG,*POVFS_ALARM_COMM_CFG;

typedef struct
{
    int enable;
    int playTimes;
    int audioSelected;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_SOUND_CFG;

typedef struct
{
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_LIGHT_CFG;


//alarmIn cfg
typedef struct
{
    int				enable;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_ALARMIN_TRI_CFG,*POVFS_ALARMIN_TRI_CFG;

typedef struct
{
    OVFS_ALARMIN_TRI_CFG cfgTri[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
    OVFS_LINKAGE_CFG linkage[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
} OVFS_ALARMIN_CFG,*POVFS_ALARMIN_CFG;

//motion cfg
typedef struct
{
    int				enable;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_MOTION_TRI_CFG,*POVFS_MOTION_TRI_CFG;

typedef struct
{
    OVFS_MOTION_TRI_CFG cfgTri[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
    OVFS_LINKAGE_CFG linkage[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
} OVFS_MOTION_CFG,*POVFS_MOTION_CFG;

//vhide cfg
typedef struct
{
    int				enable;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_VHIDE_TRI_CFG,*POVFS_VHIDE_TRI_CFG;

typedef struct
{
    OVFS_VHIDE_TRI_CFG cfgTri[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
    OVFS_LINKAGE_CFG linkage[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
} OVFS_VHIDE_CFG,*POVFS_VHIDE_CFG;

//vdialognose cfg
typedef struct
{
    int				enable;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_VDIAGNOSE_TRI_CFG,*POVFS_VDIAGNOSE_TRI_CFG;

typedef struct
{
    OVFS_VDIAGNOSE_TRI_CFG cfgTri[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
    OVFS_LINKAGE_CFG linkage[MAX_VI_DEV_SUPPORT][MAX_CHANNEL_SUPPORT];
} OVFS_VDIAGNOSE_CFG,*POVFS_VDIAGNOSE_CFG;

//common alarm cfg
typedef struct
{
    int				enable;
    OVFS_SCHEDTIME_TIME_T shedule;
} OVFS_COMMON_TRI_CFG,*POVFS_COMMON_TRI_CFG;

typedef struct
{
    int enable;
    int duration;
    OVFS_SCHEDTIME_TIME_T schTime;
} OVFS_LIGHT_ALARM_CFG;

typedef struct
{
    OVFS_LINKAGE_CFG linkage;
    OVFS_LIGHT_ALARM_CFG lightCfg;
    OVFS_SOUND_CFG soundCfg;
} OVFS_COMMON_CFG,*POVFS_COMMON_CFG;

#ifndef SIMPLIFIED
typedef struct
{
    int enableSwitch;
    int arming[3];
} OVFS_RECFACE_STATE;

typedef struct
{
    OVFS_RECFACE_STATE recFaceState;
} OVFS_ADD_STATE;
typedef struct
{
    OVFS_COMMON_CFG whiteList;
    OVFS_COMMON_CFG blackList;
    OVFS_COMMON_CFG otherList;
} OVFS_RECFACE_CFG;
#endif

typedef struct
{
    int snapInterval;// 0: stop >0: seconds
    int snapStreamIndex;
    OVFS_SCHEDTIME_TIME_T schTime;
} OVFS_FTP_SNAP_CFG;

typedef struct
{
    int Duration;
    int EnableAudio;
    int EnableLight;
} OVFS_ONEKEY_DRIVE_CFG;

typedef struct
{
    int enable;
    char url[1024];
    int interval;
    int withImage;
    int updateImage;
}OVFS_PUSH_CFG;

#if 0//def SIMPLIFIED
typedef struct
{
    int enable;//布防
    OVFS_COMMON_CFG	alarmIncfg;
    OVFS_COMMON_CFG motioncfg;
    OVFS_COMMON_CFG vhidecfg;
    OVFS_COMMON_CFG PersonDetcfg;
    OVFS_COMMON_CFG netBreakcfg;
    OVFS_COMMON_CFG ipConflitcfg;
    OVFS_COMMON_CFG illAccesscfg;
} OVFS_ALARM_CFG,*POVFS_ALARM_CFG;
#else
typedef struct
{
    int enable;
    OVFS_MAIL_CFG	mailcfg;
    OVFS_FTP_SNAP_CFG ftpsnapCfg;
    OVFS_COMMON_CFG	alarmIncfg;
    OVFS_COMMON_CFG motioncfg;
    OVFS_COMMON_CFG vhidecfg;

    OVFS_COMMON_CFG regionalInvasioncfg;
    OVFS_COMMON_CFG traverseCfg;
    OVFS_COMMON_CFG personStayCfg;
    OVFS_COMMON_CFG absentCfg;
    OVFS_COMMON_CFG parkViolationCfg;
    OVFS_COMMON_CFG vehicleRetrogradeCfg;

    OVFS_COMMON_CFG netBreakcfg;
    OVFS_COMMON_CFG ipConflitcfg;
    OVFS_COMMON_CFG illAccesscfg;

    OVFS_COMMON_CFG sensorAlarm[MAX_SENSORALARM];

    OVFS_ONEKEY_DRIVE_CFG oneKeyDriveCfg;
    OVFS_PUSH_CFG pushCfg;
} OVFS_ALARM_CFG,*POVFS_ALARM_CFG;
#endif
typedef struct
{
    S8 *sName;					//报警名称
    S8 sUser[MAX_NAMELEN];
    S8 sRemoteIP[MAX_NAMELEN];	//远程IP
    S32 iType;					//报警类型
    S32 iSrcType;					//报警源类型
    S8 *sDevName;				//报警设备名
    S32 iIndexChan;
    S32 iDev;						//报警设备id       -1:表示无效,0-MAX_DEVICE_COUNT
    S32 iChan;					//报警通道id       -1:表示无效,0-MAX_CHANNEL_COUNT
    S32 iStream;					//报警流id            -1:表示无效,0-MAX_STREAM_COUNT
    S32 iRegion;					//报警区域id            -1:表示无效,0-MAX_STREAM_COUNT
    S32 iState;					//报警状态
    S32 iLstState;					//上次的状态
    S32 iSysState;				//底层上来的报警状态
    S8 bArming;					//布防状态
    Common_Time_T t_start;			//报警开始时间 //-1:表示没开始
    Common_Time_T t_stop;			//报警结束时间 //-1:表示没结束
    Common_Time_T t_startEx[MAX_REGION_SUPPORT];			//报警开始时间 //-1:表示没开始
    Common_Time_T t_stopEx[MAX_REGION_SUPPORT];			//报警结束时间 //-1:表示没结束
    S32 iAudioState;            //联动音频状态 -1:无声音 0-播放默认 1-播放自定义
    S32 iAudioTimes;            //音频播放次数
    S32 iLightState;            //light state
    S32 iLogId;                 //报警日志ID
    S8 sExpandAlarmOut[32][8];  //触发的扩展报警输出
} OVFS_ALARM_EVENT,*POVFS_ALARM_EVENT;


typedef struct
{
    int     curState; // -1: enable out of schedule 0: stop 1: enable
    long    lastTime;
    int     linkCnt;
} ACTION_COMM_STATE_T;

typedef struct
{
    int     curState; // -1: enable out of schedule 0: stop 1: enable
    long    lastTime;

    int     linkIdx;
    char    path[64];
    int     times;
    int     linkCnt;
} ACTION_SOUND_STATE_T;

typedef struct
{
    ACTION_SOUND_STATE_T soundAlarm;
    ACTION_COMM_STATE_T lightAlarm;
	ACTION_COMM_STATE_T redbluelight;
    ACTION_COMM_STATE_T ftpAlarm;
} OVFS_ACTION_STATE_T;


typedef struct
{
    OVFS_ALARM_EVENT **pAlarmEvent;
    S32 alarmCount;
    Common_Lock_T alarmLock;

    Common_Lock_T       actionLock;
    OVFS_ACTION_STATE_T actionState;
} OVFS_ALARM_BKBD,*POVFS_ALARM_BKBD;

typedef struct
{
    S32 nRecvID;
    S8 *szSubscribeUri;			//原始的订阅URI
    S8 *szValidUri;				//去除条件后的URI
    void *pCondition;
} OVFS_SUBSCRIBE_NODE,*POVFS_SUBSCRIBE_NODE;

typedef struct
{
    COMMON_DLIST_T listdl;
    ModuleHandle_T hModuleHandle;
} OVFS_COMMON_LIST_T,*POVFS_COMMON_LIST_T;

#ifdef __cplusplus
}
#endif

#endif /* OVFS_ALARM_COMMON_H_ */
