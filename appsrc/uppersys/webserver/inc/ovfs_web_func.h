#ifndef __OVFS_WEB_FUNC_H__
#define __OVFS_WEB_FUNC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "goahead.h"
#include "AntsWebCommon.h"
#include "ovfs_web_rest.h"
#include "web_inter_api.h"
#include "ovfs_web_alist.h"
#include "libmodule_api.h"
#include "mxml.h"
#include "third_protocol_api.h"
#include <common_media_struct.h>


#ifndef OVFS_WEB_API_VERSION
//v1.7.2 同步代码, 独立出君正平台专用代码
//v1.7.3 frmDevicePara扩展字段DeviceTypeString, FunctionInfo, frmGetQRCodePictureV2接口bug修改
//v1.8.0 新增获取设备能力集frmDeviceAbility接口, 支持推送免打扰, 支持nvr升级ipc
//v2.0.0 合并去掉SIMPLIFIED和WITH_PTZ宏
#define OVFS_WEB_API_VERSION "V3.0.0"


#endif

#define  ovfs_print_json(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		printf("[%s.%d]%s\n",__FUNCTION__,__LINE__,ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)


// webserver中裁掉搜索服务.
#define OVFS_WEB_DISCOVERY 1

#ifndef MAX_DVRINFO_NUM
#define MAX_DVRINFO_NUM 20
#endif

#ifndef MAX_WIFI_NUM
#define MAX_WIFI_NUM 32
#endif

#define LIB_PATH "/root/lib"
#define LIB_DEBUG_PATH "/dev"


/**********************云台控制命令 begin*************************/
#if 0//ndef SIMPLIFIED
#define OVFS_PTZ_LIGHT_PWRON		2	/* 接通灯光电源 */
#define OVFS_PTZ_WIPER_PWRON		3	/* 接通雨刷开关 */
#define OVFS_PTZ_FAN_PWRON		4	/* 接通风扇开关 */
#define OVFS_PTZ_HEATER_PWRON	5	/* 接通加热器开关 */
#define OVFS_PTZ_AUX_PWRON1		6	/* 接通辅助设备开关 */
#define OVFS_PTZ_AUX_PWRON2		7	/* 接通辅助设备开关 */
#define OVFS_PTZ_SET_PRESET		8	/* 设置预置点 */
#define OVFS_PTZ_CLE_PRESET		9	/* 清除预置点 */
#endif

#define OVFS_PTZ_ZOOM_IN			11	/* 焦距以速度SS变大(倍率变大) */
#define OVFS_PTZ_ZOOM_OUT		12	/* 焦距以速度SS变小(倍率变小) */
#define OVFS_PTZ_FOCUS_NEAR      13  /* 焦点以速度SS前调 */
#define OVFS_PTZ_FOCUS_FAR       14  /* 焦点以速度SS后调 */
#define OVFS_PTZ_IRIS_OPEN       15  /* 光圈以速度SS扩大 */
#define OVFS_PTZ_IRIS_CLOSE      16  /* 光圈以速度SS缩小 */

#if 0//ndef SIMPLIFIED
#define OVFS_PTZ_TILT_UP			21	/* 云台以SS的速度上仰 */
#define OVFS_PTZ_TILT_DOWN		22	/* 云台以SS的速度下俯 */
#define OVFS_PTZ_PAN_LEFT		23	/* 云台以SS的速度左转 */
#define OVFS_PTZ_PAN_RIGHT		24	/* 云台以SS的速度右转 */
#define OVFS_PTZ_UP_LEFT			25	/* 云台以SS的速度上仰和左转 */
#define OVFS_PTZ_UP_RIGHT		26	/* 云台以SS的速度上仰和右转 */
#define OVFS_PTZ_DOWN_LEFT		27	/* 云台以SS的速度下俯和左转 */
#define OVFS_PTZ_DOWN_RIGHT		28	/* 云台以SS的速度下俯和右转 */
#define OVFS_PTZ_PAN_AUTO		29	/* 云台以SS的速度左右自动扫描 */

#define OVFS_PTZ_FILL_PRE_SEQ	30	/* 将预置点加入巡航序列 */
#define OVFS_PTZ_SET_SEQ_DWELL	31	/* 设置巡航点停顿时间 */
#define OVFS_PTZ_SET_SEQ_SPEED	32	/* 设置巡航速度 */
#define OVFS_PTZ_CLE_PRE_SEQ		33	/* 将预置点从巡航序列中删除 */
#define OVFS_PTZ_STA_MEM_CRUISE	34	/* 开始记录轨迹 */
#define OVFS_PTZ_STO_MEM_CRUISE	35	/* 停止记录轨迹 */
#define OVFS_PTZ_RUN_CRUISE		36	/* 开始轨迹 */
#define OVFS_PTZ_RUN_SEQ			37	/* 开始巡航 */
#define OVFS_PTZ_STOP_SEQ		38	/* 停止巡航 */
#define OVFS_PTZ_GOTO_PRESET		39	/* 快球转到预置点 */
#define OVFS_PTZ_FILL_SEQ_CRUISE	40	/* 将巡航序列设置到云台中 */
#define OVFS_PTZ_DEL_SEQ	41	/* 删除巡航组 */

#define OVFS_PTZ_SET_3D		42	/* 设置3d 定位 */
#define OVFS_PTZ_SET_OPERATION		43	/* 扩展的操作 */

#define OVFS_PTZ_TRANS_DATA		0xffff	/* 透传数据 */
#endif

/**********************云台控制命令 end*************************/

//校时结构参数
typedef enum
{
    //后面添加的时区在最后面添加 已经废弃的宏 不要删除
    //枚举值不要随意修改 IE会按枚举值索引语言
    OVFS_TIME_ZONE_WEST_LINE = 0,				   //国际日期变更线西
    OVFS_TIME_ZONE_SAMOA = 1,						//中途岛,萨摩亚群岛
    OVFS_TIME_ZONE_HAWAII = 2,						//夏威夷
    OVFS_TIME_ZONE_ALASKA = 3,						//阿拉斯加
    OVFS_TIME_ZONE_PACIFIC_OCEAN = 4,				//太平洋时间(美国和加拿大)
    OVFS_TIME_ZONE_MOUNTAIN = 5,					//山地时间(美国和加拿大)
    OVFS_TIME_ZONE_CENTRAL_CANADA = 6,          	//中部时间(美国和加拿大)
    OVFS_TIME_ZONE_EASTERN_TIME_CANADA = 7,     	  //东部时间(美国和加拿大)
    OVFS_TIME_ZONE_CARACAS = 8,                 	  //加拉加斯
    OVFS_TIME_ZONE_ATLANTIC_CANADA = 9,         	  //大西洋时间(加拿大)
    OVFS_TIME_ZONE_NEWFOUNDLAND = 10,            	  //纽芬兰
    OVFS_TIME_ZONE_GEORGETOWN = 11,              	  //乔治敦, 巴西利亚
    OVFS_TIME_ZONE_ATLANTIC_OCEAN = 12,          	  //中大西洋
    OVFS_TIME_ZONE_ANGLE_ISLANDS = 13,           	  //佛得角群岛,亚速尔群岛
    OVFS_TIME_ZONE_GREENWICH = 14,               	  //格林威治标准时间:都柏林, 爱丁堡, 伦敦, 里斯本
    OVFS_TIME_ZONE_AMSTERDAM = 15,               	  //阿姆斯特丹, 柏林, 罗马, 巴黎
    OVFS_TIME_ZONE_ATHENS = 16,                  	  //雅典, 耶路撒冷 ,伊斯坦布尔
    OVFS_TIME_ZONE_BAGHDAD = 17,                 	  //巴格达 ,科威特
    OVFS_TIME_ZONE_TEHERAN = 18,                 	  //德黑兰
    OVFS_TIME_ZONE_MOSCOW = 19,                  	  //莫斯科, 圣彼得堡, 伏尔加格勒
    OVFS_TIME_ZONE_KABUL = 20,                   	  //喀布尔
    OVFS_TIME_ZONE_ISB = 21,                     	  //伊斯兰堡, 卡拉奇 ,塔什干
    OVFS_TIME_ZONE_MADRAS = 22,                  	  //马德拉斯,钦奈, 加尔各答, 孟买, 新德里
    OVFS_TIME_ZONE_KATHMANDU = 23,               	  //加德满都
    OVFS_TIME_ZONE_NOVOSIBIRSK = 24,             	  //阿拉木图,达卡,新西伯利亚
    OVFS_TIME_ZONE_RANGOON = 25,                 	  //仰光
    OVFS_TIME_ZONE_BANGKOK = 26,                 	  //曼谷, 河内, 雅加达
    OVFS_TIME_ZONE_BEIJING = 27,                 	  //北京,香港特别行政区,乌鲁木齐,新加坡
    OVFS_TIME_ZONE_OSAKA = 28,                   	  //首尔,大阪, 札幌, 东京
    OVFS_TIME_ZONE_ADELAIDE = 29,                	//阿德莱德 ,达尔文
    OVFS_TIME_ZONE_CANBERRA = 30,                	  //堪培拉, 墨尔本, 悉尼
    OVFS_TIME_ZONE_SOLOMON_ISLANDS = 31,         	  //所罗门群岛, 新喀里多尼亚
    OVFS_TIME_ZONE_OSKLAND = 32,                 	  //奥克兰, 惠林顿 ,斐济 ,马加丹
    OVFS_TIME_ZONE_NUKUALOFA = 33,               	  //努库阿洛法  OVFS_TZ_E;
    OVFS_TIME_ZONE_MART = 34,                          //椹厠钀ㄦ柉缇ゅ矝 -9:30
}OVFS_TZ_E;

typedef struct TZ_NODE
{
    OVFS_TZ_E tz_name;
    int time_offset;
}TZ_NODE_T;

//Utils
typedef struct
{
    int TZ;
    int bias_enable;
    int bias;
}LOCAL_TZ_T;

typedef struct
{
    int offsetHour;
    int offsetMinute;
}NET_TZ_T;

static const TZ_NODE_T arry_zone[] =
{
    {OVFS_TIME_ZONE_WEST_LINE, -1200},
    {OVFS_TIME_ZONE_SAMOA, -1100},
    {OVFS_TIME_ZONE_HAWAII, -1000},
    {OVFS_TIME_ZONE_ALASKA, -900},
    {OVFS_TIME_ZONE_PACIFIC_OCEAN, -800},
    {OVFS_TIME_ZONE_MOUNTAIN, -700},
    {OVFS_TIME_ZONE_CENTRAL_CANADA, -600},
    {OVFS_TIME_ZONE_EASTERN_TIME_CANADA, -500},
    {OVFS_TIME_ZONE_CARACAS, -430},
    {OVFS_TIME_ZONE_ATLANTIC_CANADA, -400},
    {OVFS_TIME_ZONE_NEWFOUNDLAND, -330},
    {OVFS_TIME_ZONE_GEORGETOWN, -300},
    {OVFS_TIME_ZONE_ATLANTIC_OCEAN, -200},
    {OVFS_TIME_ZONE_ANGLE_ISLANDS, -100},
    {OVFS_TIME_ZONE_GREENWICH, 0},
    {OVFS_TIME_ZONE_AMSTERDAM, 100},
    {OVFS_TIME_ZONE_ATHENS, 200},
    {OVFS_TIME_ZONE_BAGHDAD, 300},
    {OVFS_TIME_ZONE_TEHERAN, 330},
    {OVFS_TIME_ZONE_MOSCOW, 400},
    {OVFS_TIME_ZONE_KABUL, 430},
    {OVFS_TIME_ZONE_ISB, 500},
    {OVFS_TIME_ZONE_MADRAS, 530},
    {OVFS_TIME_ZONE_KATHMANDU, 545},
    {OVFS_TIME_ZONE_NOVOSIBIRSK, 600},
    {OVFS_TIME_ZONE_RANGOON, 630},
    {OVFS_TIME_ZONE_BANGKOK, 700},
    {OVFS_TIME_ZONE_BEIJING, 800},
    {OVFS_TIME_ZONE_OSAKA, 900},
    {OVFS_TIME_ZONE_ADELAIDE, 930},
    {OVFS_TIME_ZONE_CANBERRA, 1000},
    {OVFS_TIME_ZONE_SOLOMON_ISLANDS, 1100},
    {OVFS_TIME_ZONE_OSKLAND, 1200},
    {OVFS_TIME_ZONE_NUKUALOFA, 1300},
    {OVFS_TIME_ZONE_MART, -930}
};

void web_TZ_translate_lton(LOCAL_TZ_T *local_TZ, NET_TZ_T *net_TZ);
void web_TZ_translate_ntol(NET_TZ_T *net_TZ, LOCAL_TZ_T *local_TZ);

typedef struct
{
    int Year;         //年
    int Month;        //月
    int Day;          //日
    int Hour;         //时
    int Minute;       //分
    int Second;       //秒
    int Zone;         //时区
}WEB_TIME_T;

//联动中涉及的结构体
typedef struct
{
    char mailAccount[32];
    char password[32];
    char smtpServer[32];
    int smtpPort;
    int bEnableSSL;
    int serverVerify;
}OVFS_WEB_MAIL_SENDER_T;

typedef struct
{
    char name[32];
    char mailAccount[32];
}OVFS_WEB_MAIL_RECEIVER_T;

typedef struct
{
    int type;
    OVFS_WEB_MAIL_SENDER_T sender;
    OVFS_WEB_MAIL_RECEIVER_T receiver;
    int attachment;
    int mailinterval;
}OVFS_WEB_ALARMLINK_MAIL_T;

typedef struct
{
    int mask0;
}OVFS_WEB_ALARMLINK_RECORD_T;

typedef struct
{
    int benable;
	int snap_channle;//抓拍通道
    int count;
    int interval;
}OVFS_WEB_ALARMLINK_SNAP_T;

typedef struct
{
	int enable;    //  0-不用 1-用
    int mask0;
    int mask1;
}OVFS_WEB_ALARMLINK_ALARMOUT_T;

typedef struct
{
    int type;    //类型  0-不调用 1-调用预置点 2-调用巡航 3-调用轨迹 4-调用看守卫
    int index;   //序号
}OVFS_WEB_ALARMLINK_PTZ_T;

typedef struct
{
    int enable;    //  0-不用 1-用
    int index;   //序号
}OVFS_WEB_ALARMLINK_AUDIO_T;


typedef struct
{
    int enable;    //  韦根通道 0 不联动 1(公共伟根号)	2（用户设置为根号）3（自动伟根号，针对人脸）
	int wiegandId; //韦根号
}OVFS_WEB_ALARMLINK_WIEGAND_T;


typedef struct
{
    int bmail_para_set;
    OVFS_WEB_ALARMLINK_MAIL_T mail_para;
    int brecord_para_set;
    OVFS_WEB_ALARMLINK_RECORD_T record_para;
    int bsnap_para_set;
    OVFS_WEB_ALARMLINK_SNAP_T snap_para;
    int balarmout_para_set; //改用为报警输出的数目
    OVFS_WEB_ALARMLINK_ALARMOUT_T alarmout_para;
	int bptz_para_set;
	OVFS_WEB_ALARMLINK_PTZ_T ptz_para;
    int baudio_para_set;
    OVFS_WEB_ALARMLINK_AUDIO_T audio_para;
	int bwiegand_para_set;
    int bwiegand_para_count;
	OVFS_WEB_ALARMLINK_WIEGAND_T wiegand_para[32];
}OVFS_WEB_ALARMLINK_T;

typedef struct
{
	unsigned int u32StxCode;//!0xAC010000
	unsigned int u32LoadLen;
	unsigned char u8Content[1024 + 256];
	unsigned int u32EtxCode;//!0xC7010000
	unsigned int u32CheckSum;
}OVFS_WEB_DISCOVERY_PACKET_T;

typedef struct
{
    int port_http;
    int port_https;
    int port_onvif;
    int port_rtsp;
    int port_rtmp;
}OVFS_PORT_ALL_T;

typedef struct
{
    char ifname[32];
    char ipv4[IPV4_ADDR_STR_LEN];
    char hwaddr[HW_ADDR_STR_LEN];
    char netmask[IPV4_ADDR_STR_LEN];
    char gateway[IPV4_ADDR_STR_LEN];
    char dns1[IPV4_ADDR_STR_LEN];
    char dns2[IPV4_ADDR_STR_LEN];
}OVFS_IF_CONF_T;

// LOG
typedef struct
{
    time_t logtime;
    int major_type;
    int minor_type;
    char username[32];
    char ip[32];
    int channel;
    int alarm_in_port;
    int alarm_out_port;
    int status;
}OVFS_WEB_LOG_T;
// Alarm
typedef struct
{
    char alarm_name[32];
    char username[32];
    int ishappen;
    char ip[32];
}OVFS_WEB_ALARM_T;

typedef struct
{
	char remoteURL[PATHNAME_LEN];
	char userName[NAME_LEN];
	char password[PASSWD_LEN];
	char fileRename[NAME_LEN];
	int  streamNo;
}OVFS_WEB_CAPTURE_FTP_T;


typedef struct
{
	int id;
	char *name;
	int sex;
	char *ageRange;
}OVFS_WEB_FACEINFO_FILTER;

const char* QueryBuildString();
int ovfs_web_discovery_init(COMMON_DLIST_T *sockList, int port);
int Fxn_Web_Discovery(Common_Thread_T hThreadHandle,void *pUserData);

/***Preview Modules***/
int frmHelp(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmWebApiVersion(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmUserLogin(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmUserLogout(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmDeviceAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmReqIFrame(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetFormatAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetImageModeAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmExtendOSD(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmUartConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmExpandAlarmOut(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSensorAlarm(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


//云镜操作
int frmPTZControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPTZPreset(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#if 0
int frmPTZCruise(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPTZTrack(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//#if 0//def WITH_PTZ
//增强接口
//两点扫描
int frmPTZExtend_SetScan(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//空闲操作
int frmPTZExtend_IdleOperation(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
// 3D定位
int frmPTZExtend_3DPosition(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//红外补光
int frmPTZExtend_IrlightCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//隐私遮蔽
int frmPTZExtend_SetCover(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//雨刷 开关
int frmPTZExtend_Wiper(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//喷淋位置
int frmPTZExtend_SprayPos(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//喷淋模式
int frmPTZExtend_SprayMode(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#endif

//获取告警信息
int frmGetAlarmInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmQueryAlarmInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmOneKeyDriveCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmOneKeyDriveControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

//日志查询
//int frmLogCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

/***Alarm Modules***/
int frmAlarmInPara_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAlarmOut(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmAlarmException(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

/***Chanpara Modules***/
int GetBit32(int iBit32, int iIndex);
int frmVideoShowParaCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#ifndef WIFIDOME
int frmMultiLineOSD(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSingleLineOSD(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmActionAlarmTimePara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmMotionDetPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAudioAlarmCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmVideoPlanPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#endif
int frmSpeechAlarmCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmVideoEffect(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoIPCSetPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoCompressAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmImageCapability(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmVideoBoardAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmVideoTestHardWare(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmOnvifPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmImageType(Webs * wp, OVFS_WEB_OPTION_S * opt, cJSON_Struct * header, cJSON_Struct * indata, cJSON_Struct * outdata);
int frmVideoParaEx(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAutoLensCorrection(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmDCIrisCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmIspConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmAutoApertureCorrection(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmBadPixelTest(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmVideoRegionOfInterest(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPTZLinkCFG(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

//int  frmVideoLostPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoHidePara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoShelterPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmDetectLinkPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmHttpLinkPara(Webs * wp, OVFS_WEB_OPTION_S * opt, cJSON_Struct * header, cJSON_Struct * indata, cJSON_Struct * outdata);

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)
int frmVideoQueryByMonth(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmFlashVideoQuery(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoRecordsQuery(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSearchSDCardPics(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmRtmpPushCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPushSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#endif

int CaptureV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int base64_decodev2( const char * base64,  char * bindata );

int frmVideoPersonPara(Webs * wp, OVFS_WEB_OPTION_S * opt, cJSON_Struct * header, cJSON_Struct * indata, cJSON_Struct * outdata);

//#endif
int frmGetSmartAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


int frmGetRtspUrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmOptimalVideoEncode(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


/***Devicepara Modules***/
int frmDevicePara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoFormatPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoFormatPara_v2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoImageMode(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmDeviceTimeCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmDstPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetFactoryInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmDecoderPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetPTZProtocal(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmGetQRCodePictureV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmParaPlatform28181(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAudioPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAudioParaAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
long int Net_GetTimeDiff();

/***Netpara Modules***/
typedef struct
{
    int enable;
    char *protolName;
    char *user;
    char *password;
    char *domainName;
    char *serverName;
    int port;
    int checkip_interval;
    int update_interval;
}WEB_DDNS_T;
int frmNetworkSettings(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmMulticast(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmNetTelnetPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetNtpPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmCurTimeOffset(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


int frmGetManagerHostsPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetSipPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmPasswordLost(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetDefaultRoute(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmLteCustomCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);

#if 1//ndef SIMPLIFIED
int frmFTPSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmEmailSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetSnmp(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetUPNPPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetDDNSServiceAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetDDNSPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmNet3G(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetLtepara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetLteCardInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

//int frmNasSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmHttpEventSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmP2PCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmP2PState(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmDTMFTrigger(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmTurnServerCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmTurnServerState(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmLensCustomCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#endif

int frmAliIoTCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAliIoTState(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAliIoTReboot(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAliIoTUnbinding(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

//int frmCustomPlatformCfg(Webs * wp, OVFS_WEB_OPTION_S * opt, cJSON_Struct * header, cJSON_Struct * indata, cJSON_Struct * outdata);
int frmRtspCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmThirdPartyProtocols(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmAudioBroadcast(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmUsageLog(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAlarmLog(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


/***Systemmng Modules***/
//硬盘相关信息结构体

// 最大的存储设备数
#ifndef MAX_DISK_NUM
#define MAX_DISK_NUM    (10)
#endif

// 每个存储设备的分区个数
#ifndef MAX_DISK_PARTITION
#define MAX_DISK_PARTITION    (10)
#endif

#ifndef WEB_TEXT_LENGTH
#define WEB_TEXT_LENGTH    (256)
#endif

typedef struct
{
    int partitionNum;
    char path[WEB_TEXT_LENGTH];
}OVFS_WEB_DISK;

//码流信息
//int frmBitRate(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//自动维护
int frmAutoReboot(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)
int frmHDFormat(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//硬盘管理
int frmGetHDInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetHDStatus(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmGetHDFormatProgress(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmGetHDFSInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmRecordConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
#endif

int frmDeviceReboot(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmParaSysRestore(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmFactoryRestore(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmDeviceRestore(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmSysUpdate(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSysUpdateFree(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int upload(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int GetProgress(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSetUuid(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmMediaDisconnect(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmUploadInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


#define FILE_CONFIG_EXPORT "/tmp/config_export.tgz"
#define FILE_CONFIG_IMPORT "/tmp/config_import.tgz"
//获取配置文件压缩包
int frmGetConfigFileV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmExportConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//设置获取文件压缩包
int frmSetConfigFileV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//修改升级文件的写入名称
int frmChangeFileName(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmHwCheck(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmWaterMark(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmBlackWhiteList(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmHttpHttpsConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmHttpPushCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmHttpAddrTest(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmLocalSettings(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmKeepSessionAlive(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);


/***Usermng Modules***/
#define BIT_PREVIEW			(0x1<<0)
#define BIT_PLAYBACK 		(0x1<<1)
#define BIT_SETTING 			(0x1<<2)
#define BIT_VIEWSETTING 	(0x1<<3)
#define BIT_RECORD 			(0x1<<4)
#define BIT_PTZ			 	(0x1<<5)
#define BIT_BACKUP		 	(0x1<<6)
#define BIT_LOG			 	(0x1<<7)
#define BIT_VIEWINFO		(0x1<<8)
#define BIT_UPGRADE		 	(0x1<<9)
#define BIT_POWER			(0x1<<10)
#define BIT_FORMAT		 	(0x1<<11)
#define BIT_IPCHANNEL		(0x1<<12)
#define BIT_CORRECTTIME		(0x1<<13)
#define BIT_DELETEUSER		(0x1<<14)
#define BIT_MODIFYUSER 		(0x1<<15)
#define BIT_RESETPWD 		(0x1<<16)
#define BIT_RESETPWDSELF	(0x1<<17)
#define BIT_ONLINEAUTH 		(0x1<<18)
#define BIT_REMOTETALK 		(0x1<<19)

typedef struct
{
    u_int remote_ptz:1;
    u_int remote_record:1;
    u_int remote_setparam:1;
    u_int remote_log:1;
    u_int remote_update_format:1;
    u_int remote_talk:1;
    u_int remote_preview:1;
    u_int remote_reboot:1;
    u_int res:24;
}USER_WEB_RIGHTS_T;
//在线用户
int frmUserOnline(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmUserOnlineCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmUserManage(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmUserRights_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
//int frmUserRights(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmValidateTokens(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSetLogoPic(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);

int frmProductTest(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);

int frmIotRecordCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmIotSDStatus(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmIotSDFormat(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmIotNetworkStatus(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmIotPresetControl(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmIotHomePositionCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmLaserLight(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmIotLightCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmExposureLight(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAovSleep(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmSwitchSensor(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmBatteryConfig(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmIndicatorLightConfig(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);
int frmSimCardSwitch(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata);

int frmMotionDetect_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVideoHide_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmSetCustomAudio(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAudioSpeech(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmRegionalInvasion(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmTraverseDetect(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPersonStaying(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPersonAbsent(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmParkingViolation(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmVehicleRetrograde(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmDisableAllSmart(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmCruiseControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmPTZStepControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int frmAlarmPushConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmNetworkStatus(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmLightConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmAllOSDConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
int frmWLANConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

//Common Interface
int ovfs_web_get_alarmlink(cJSON_Struct *header, int ch, OVFS_WEB_ALARMLINK_T *palarm_link, int *handle_type,OVFS_WEB_OPTION_S *opt,cJSON_Struct *inParam);
int ovfs_web_set_alarmlink(cJSON_Struct *header, int ch, OVFS_WEB_ALARMLINK_T *alarm_link, int handle_type,OVFS_WEB_OPTION_S *opt,cJSON_Struct *lowerData);
// LOG
int web_set_log(OVFS_WEB_LOG_T *log);
// ALARM
int ovfs_web_send_alarm(OVFS_WEB_ALARM_T *alarm);
int web_semantic_get_userright_remote(cJSON_Struct *header,int right_id);

typedef struct
{
    int corridor;
    int twdr;
    int exposureLight;
    int ispScene;
    char lightList[8][16];
    int switchsensor;
}OVFS_IMAGE_CAP_T;

int get_image_capability(cJSON_Struct *header, OVFS_WEB_OPTION_S *opt, OVFS_IMAGE_CAP_T *imageCap);

extern int JsonOper_MergeObj(Common_cJSON_T* dst,Common_cJSON_T* src,int type);

//talk
int talk_stream_open();
int talk_stream_close(int handle);
int talk_stream_release(int handle);
int talk_stream_read(int handle,void **data, int *size);
int talk_stream_write(int handle,void *data, int size);


typedef int (*fTHIRD_PROTOCOL_Init)(fCallParamFun pCallParamFun);
typedef int (*fTHIRD_PROTOCOL_Uninit)();
typedef int (*fTHIRD_PROTOCOL_AlarmCallBack)(THIRD_ALARM_INFO *pAlarmStatus, int nAlarmStatusCnt);
typedef int (*fTHIRD_PROTOCOL_DealUdpPkg)(void *UserData, char *InBuffer,int InSize,char **OutBuffe,int *OutSize);
typedef int (*fTHIRD_PROTOCOL_SetConfig)(THIRD_ONVIF_CFG *cfg);
typedef int (*fTHIRD_PROTOCOL_SetTalkCallBack) (TalkCallBack *pfxn);

typedef struct
{
    char libName[32];
    fTHIRD_PROTOCOL_Init 		fInit;
    fTHIRD_PROTOCOL_Uninit  	fUninit;
    fTHIRD_PROTOCOL_AlarmCallBack  	fAlarmCallBack;
    fTHIRD_PROTOCOL_DealUdpPkg fDealUdpPkg;
    fTHIRD_PROTOCOL_SetConfig fSetConfig;
    fTHIRD_PROTOCOL_SetTalkCallBack fSetTalkCallBack;
} THIRD_PROTOCOL_OPS_INFO_T;

typedef struct
{
    char addr[32];
    int port;
    char *buf;
    int bufLen;
    pthread_t serverPth;
    fTHIRD_PROTOCOL_DealUdpPkg cb;
    COMMON_DLIST_T sockDevList;
}ONVIF_UDP_CONTEXT_T;

typedef struct
{
	char* device;
	int ifindex;
    int wlanidx;
	int fd;
	u_int8_t* buffer;
	u_int32_t buf_len;
} HK_Ether_Cap;

typedef struct
{
	int code;
	BYTE version;
    char sn[20];
	BYTE frameCount;
	char res[64];
}__attribute__((packed)) Broadcast_header;
#endif

#define MAJOR_OPERATION     1
#define MINOR_SHUTDOWN          66
#define MINOR_REBOOT            68
#define MINOR_LOGIN	            1	/* 登录 */
#define MINOR_LOGOUT	        2	/* 注销登录 */
#define MINOR_CONFIG_IMPORT     9   /* 导入配置文件 */
#define MINOR_FORMAT_SD         10
#define MINOR_RESTORE_SIMPE     11
#define MINOR_RESTORE_ALL       12
#define MINOR_EXPAND_ALARMOUT_TRIGGER      13
#define MINOR_EXPAND_ALARMOUT_STOP         14

#define MAJOR_CONFIG_SET    2
#define MINOR_SET_DEVICECFG		101
#define MINOR_SET_TIMECFG		102
#define MINOR_SET_AUDIOCFG		103
#define MINOR_SET_OSDCFG		104
#define MINOR_SET_IMAGECFG		105
#define MINOR_SET_VIDEOENCODECFG		106
#define MINOR_SET_LIGHTCFG		107
#define MINOR_SET_PRVACYAREACFG		108
#define MINOR_SET_MOTIONCFG		109
#define MINOR_SET_HIDECFG		110
#define MINOR_SET_REGIONALINVASIONCFG		111
#define MINOR_SET_TRAVERSEDETECTCFG		112
#define MINOR_SET_PERSONSTAYINGCFG		113
#define MINOR_SET_PERSONABSENTCFG		114
#define MINOR_SET_PAKINGVIOLATIONCFG		115
#define MINOR_SET_VEHICLERETOGRADECFG		116
#define MINOR_SET_ALARMINCFG		117
#define MINOR_SET_ALARMOUTCFG		118
#define MINOR_SET_RECORDCFG		119
#define MINOR_SET_NETWORKCFG		120
#define MINOR_SET_GB28181CFG		121
#define MINOR_SET_EMAILCFG		122
#define MINOR_SET_FTPCFG		123
#define MINOR_SET_RTSPCFG		124
#define MINOR_SET_RTMPCFG		125
#define MINOR_SET_USERCFG		126
#define MINOR_SET_AUTOREBOOTCFG		127
#define MINOR_SET_UARTCFG		128
#define MINOR_SET_SENSORALASRMCFG		129
#define MINOR_SET_EXPANDALARMOUTCFG		130

