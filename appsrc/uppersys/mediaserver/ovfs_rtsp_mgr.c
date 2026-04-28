/*
 * ovfs_rtsp_mgr.c
 *
 *  Created on: 2016年10月11日
 *      Author: eric
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rtspserver_v2.h>

#include "ovfs_media.h"

#define Ovfs_FrameType_SpeakFrames 0x80
/*
 * UtcTime是事件发生时间onvif标准定义，UtcStart UtcStop是我们扩展的，不是onvif标准定义的，这里我们用来存储事件开始和结束时间
 */
#define RTSP_APP_DATA_STR         \
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
        "<tt:MetadataStream xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\">\n" \
        "<tt:Event>\n" \
        "<wsnt:NotificationMessage>\n" \
        "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">tns1:%s</wsnt:Topic>\n" \
        "<wsnt:Message>\n" \
        "<tt:Message UtcTime=\"%s\" UtcStart=\"%s\" UtcStop=\"%s\">\n" \
        "<tt:Source>\n" \
        "<tt:SimpleItem Name=\"%s\" Value=\"%s%02d\" />\n" \
        "</tt:Source>\n" \
        "<tt:Data>\n" \
        "<tt:SimpleItem Name=\"State\" Value=\"%s\" %s/>\n" \
        "</tt:Data>\n" \
        "</tt:Message>\n" \
        "</wsnt:Message>\n" \
        "</wsnt:NotificationMessage>\n" \
        "</tt:Event>\n" \
        "</tt:MetadataStream>\n"


#define RTSP_APP_DATA_STR_SMART         \
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
                "<tt:MetadataStream xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\">\n" \
                "<tt:Event>\n" \
                "<wsnt:NotificationMessage>\n" \
                "<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">tns1:%s</wsnt:Topic>\n" \
                "<wsnt:Message>\n" \
                "<tt:Message UtcTime=\"%s\" UtcStart=\"%s\" UtcStop=\"%s\">\n" \
                "<tt:Source>\n" \
                "<tt:SimpleItem Name=\"%s\" Value=\"%s%02d\" />\n" \
                "</tt:Source>\n" \
                "<tt:Data>\n" \
                "<tt:SimpleItem Name=\"State\" Value=\"%s\" ImgBase64=\"%s\"/>\n" \
                "</tt:Data>\n" \
                "</tt:Message>\n" \
                "</wsnt:Message>\n" \
                "</wsnt:NotificationMessage>\n" \
                "</tt:Event>\n" \
                "</tt:MetadataStream>\n"
#define RTSP_APP_DATA_MAX   1024
#define RTSP_APP_DATA_SMART_MAX   1024 * 300
#define RTSP_APP_DATA_DELAY 5

/*ovfs_hostmgr_type.h*/
#define MAJOR_OPERATION                 0x3
#define MINOR_START_VT                  0x7c    /* START Voice*/
#define MINOR_STOP_VT                   0x7d    /* STOP Voice*/
#define MINOR_REMOTE_PLAYBYTIME         0x80    /* 远程按时间回放 */
typedef struct
{
    int state;
    int rtspHandle;
    MQ_HANDLE_H mqHandle;
    RTSP_CONFIG_T cfg;
    pthread_pool_t *tp;
    COMMON_DLIST_T connStateList;
    COMMON_DLIST_T conList;
    MEDIA_BOARD_STATE_T boardState;
    int debugPrint;
    int streamCount;
} RTSP_MGR_T;

typedef struct
{
    int liveCombo;
    unsigned int sessionId;
    int sessionHandle;
    int isPlay;
    long long int connTime;
    MEDIA_REQ_STREAM_INFO_T reqInfo;
} RTSP_SESSION_TABLE_T;

//typedef struct
//{
//    unsigned int tsCnt;
//    unsigned int tsBase;
//    long long unsigned int ctsBase;
//    long long unsigned int ctsCnt;
//    long long unsigned int lastDiffTime;
//} RTSP_SMOOTHING_T;

typedef struct
{
    long long unsigned int lastIfSendCost;
    int restCnt;
    struct timeval timeP0;
    struct timeval timeP1;
    unsigned int lastDataSize;
    unsigned int lastTimeStamp;
    unsigned int spendTime;
    long long unsigned int difftime;
} RTSP_SMOOTHING_T;

typedef struct
{
    FILE * fp;
    int isPrint;
    int delayCnt;
    unsigned int lastVts;
    unsigned int lastSts;
    unsigned int lastTs;
}RTSP_DIPINXIAN_T;

typedef struct
{
    unsigned int token;
    char username[64];
    char password[64];
    int rtspHandle;
    int streamHandle;
    int sessionHandle;
    unsigned int sessionId;

    MEDIA_REQ_STREAM_INFO_T reqInfo;

    int liveCombo;
    int streamType;
    int videoType;
    int audioType;
    int protoType;
    int multiCast;
    int preferh264;
    int reqChangeCodec;
    int ctype;
    int videoCodecId;

    unsigned short videoW;
    unsigned short videoH;
    char *customType;

    unsigned int connectIp;
    long long int connectTime;
    int reqIFrame;

    RTSP_MGR_T *rtspMgr;
    int streamQueueHandle;
    long long unsigned int totalReadSize;
    int isPlaying;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int isExit;
    long long int timeStamp;

    int pause;
    int forceI;
    int forceS;

    RTSP_SMOOTHING_T smooth;
    Ovfs_FrameHeader_T *frameHeader;
    RTSP_DIPINXIAN_T dpxCt;
    int mediaType;
} RTSP_CONNECT_INFO_NODE_T;

typedef struct
{
    unsigned int alarmType;
    char *alarmTns;
    char *alarmName;
    char *alarmValue;
    char *alarmKeyName;
} OVFS_RTSPSERVER_APP_T;

typedef enum
{
    OVFS_RTSPSERVER_ALARMTYPE_ALARMIN = 0, //信号量报警
    OVFS_RTSPSERVER_ALARMTYPE_DISKFULL, //硬盘满
    OVFS_RTSPSERVER_ALARMTYPE_VIDEOLOSS, //信号丢失
    OVFS_RTSPSERVER_ALARMTYPE_MOTION, //移动侦测
    OVFS_RTSPSERVER_ALARMTYPE_DISKNOFORMAT, //硬盘未格式化
    OVFS_RTSPSERVER_ALARMTYPE_DISKIOERROR, // 读写硬盘出错
    OVFS_RTSPSERVER_ALARMTYPE_MASK, // 遮挡报警
    OVFS_RTSPSERVER_ALARMTYPE_VIDEOSTANDARDEXCEPTION, //制式不匹配
    OVFS_RTSPSERVER_ALARMTYPE_ILLEGEACCESS, // 非法访问
    OVFS_RTSPSERVER_ALARMTYPE_VIDEOEXCEPTION, // 视频信号异常
    OVFS_RTSPSERVER_ALARMTYPE_RECEXCEPTION, // 录像异常
    OVFS_RTSPSERVER_ALARMTYPE_RETICLEDISCONNECT = 20, // 网线断
    OVFS_RTSPSERVER_ALARMTYPE_IPCONELICT = 21, // IP冲突
    OVFS_RTSPSERVER_ALARMTYPE_IVS_COUNTWIRE = 22, //计数检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTWIRE = 23, //过线检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTREGION = 24, //区域检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_OBJECTREGION = 25, //物品检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_SOUNDALARM = 26, //异常声音报警
    OVFS_RTSPSERVER_ALARMTYPE_IVS_PLATEDETECT_ALARM = 27, //车牌识别
    OVFS_RTSPSERVER_ALARMTYPE_IVS_FACEDETECT_ALARM = 28, //人脸检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_FIREDETECT_ALARM = 29, //火焰检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_COLOR = 30, //视频诊断_偏色
    OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_BRIGHT = 31, //视频诊断_偏亮
    OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_BLUR = 32, //视频诊断_模糊
    OVFS_RTSPSERVER_ALARMTYPE_IVS_MOTION_DETECT = 33, //智能移动侦测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_DARK = 34, //视频诊断_偏暗
    OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_VI_LOST = 35, //视频诊断_视频丢失
    OVFS_RTSPSERVER_ALARMTYPE_IVS_TEMPHIGH = 36, //温度报警高温
    OVFS_RTSPSERVER_ALARMTYPE_IVS_TEMPLOW = 37, //温度报警低温
    OVFS_RTSPSERVER_ALARMTYPE_MEDIA_DISCONNECT = 38, //meida disconnect 平台掉线
    OVFS_RTSPSERVER_ALARMTYPE_IVS_RETROGRADE = 39, // 逆行
    OVFS_RTSPSERVER_ALARMTYPE_IVS_HIGHDENSITY = 40, // 密度
    OVFS_RTSPSERVER_ALARMTYPE_IVS_HUMANOID = 41, // 人形
    OVFS_RTSPSERVER_ALARMTYPE_IVS_MAXHEIGHT = 42, // 目标限高
    OVFS_RTSPSERVER_ALARMTYPE_IVS_RUN = 43, // 快速移动(奔跑)检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_VIOLENTMOTION = 44, // 剧烈运动检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_SCENCECHAGE = 45, // 剧烈运动检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTPERSON = 46, // 人形检测
    OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTABSENT = 47, //离岗
    OVFS_RTSPSERVER_ALARMTYPE_IVS_REGIONALINVASIN = 48, //区域入侵
    OVFS_RTSPSERVER_ALARMTYPE_IVS_PERSIONSTAYING = 49, //人员逗留
    OVFS_RTSPSERVER_ALARMTYPE_IVS_PARKINGVIOLATION = 50, //车辆违停
    OVFS_RTSPSERVER_ALARMTYPE_SENSORALARM = 51, //传感器报警
    OVFS_RTSPSERVER_ALARMTYPE_MAX,
} OVFS_RTSPSERVER_ALARMTYPE_E;

typedef struct
{
    int vdType;
    int vdAlarmIdx;
    int vdAlarmType;
} OVFS_RTSPSERVER_VIDEODIAGNOSE_TABLE_T;

static OVFS_RTSPSERVER_VIDEODIAGNOSE_TABLE_T s_rtspVdTable[] =
{
    {0,21,OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_COLOR},
    {1,25,OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_DARK},
    {2,22,OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_BRIGHT},
    {7,26,OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_VI_LOST},
    {8,23,OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_BLUR},
};

static RTSP_MGR_T s_rtspMgr;

static OVFS_RTSPSERVER_APP_T ovfs_rtspserver_apps[] =
{
    {OVFS_RTSPSERVER_ALARMTYPE_ALARMIN, "Device/Trigger/DigitalInput", "InputToken", "AlarmIn" , "AlarmIn"},
    {OVFS_RTSPSERVER_ALARMTYPE_DISKFULL, "Device/DiskFull", "DiskNum", "Disk","DiskFull"},
    {OVFS_RTSPSERVER_ALARMTYPE_VIDEOLOSS, "VideoSource/SignalLoss", "VideoSourceToken", "VideoSource"},
    {OVFS_RTSPSERVER_ALARMTYPE_MOTION, "VideoSource/MotionAlarm", "VideoSourceToken", "VideoSource", "Motion"},
    {OVFS_RTSPSERVER_ALARMTYPE_DISKNOFORMAT, "Device/DiskNOFormat", "DiskNum", "Disk","UnmatchFormat"},
    {OVFS_RTSPSERVER_ALARMTYPE_DISKIOERROR, "Device/DiskIOError", "DiskNum", "Disk","DiskErr"},
    {OVFS_RTSPSERVER_ALARMTYPE_MASK, "VideoSource/ImageTooDark", "VideoSourceToken", "VideoSource","Vhide"},
    {OVFS_RTSPSERVER_ALARMTYPE_VIDEOSTANDARDEXCEPTION, "Device/VideoStandardException", "StandardType", "Standard"},
    {OVFS_RTSPSERVER_ALARMTYPE_ILLEGEACCESS, "Device/IllegeAccess", "UserNum", "User","IllegallyAcc"},
    {OVFS_RTSPSERVER_ALARMTYPE_VIDEOEXCEPTION, "VideoSource/VideoException", "VideoSourceToken", "VideoSource"},
    {OVFS_RTSPSERVER_ALARMTYPE_RECEXCEPTION, "Device/RecException", "DiskNum", "Disk","VideoRecordErr"},
    {OVFS_RTSPSERVER_ALARMTYPE_RETICLEDISCONNECT, "Device/ReticleDisconnect", "EthNum", "Eth","NetCableBreak"},
    {OVFS_RTSPSERVER_ALARMTYPE_IPCONELICT, "Device/IpConelict", "EthNum", "Eth","IpConflict"},

    /*13*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_COUNTWIRE, "Device/IvsCountWire", "Count", "Wire", "CounterWire"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTWIRE, "Device/IvsDetectWire", "Detect", "Wire", "DetectWire"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTREGION, "Device/IvsDetectRegion", "Detect", "Region","DetectRegion"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_OBJECTREGION, "Device/IvsObjectRegion", "Object", "Region", "ObjectRegion"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_SOUNDALARM, "Device/IvsSoundAlarm", "Sound", "Exception","SoundDetect"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_PLATEDETECT_ALARM, "Device/PlateDetect","Detect", "Alarm","DetectPlate"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_FACEDETECT_ALARM, "Device/FaceDetect","Detect", "Alarm", "DetectFace"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_FIREDETECT_ALARM, "Device/FireDetect","Detect", "Alarm", "DetectFire"},
    /*21*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_COLOR, "Device/Color","VideoDiagnose", "Alarm","Vdiagnose"},/*21*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_BRIGHT, "Device/Bright","VideoDiagnose", "Alarm","Vdiagnose"},/*22*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_BLUR, "Device/Blur","VideoDiagnose", "Alarm","Vdiagnose"},/*23*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_MOTION_DETECT, "Device/IvsMotionDetect","Detect", "Motion", "SmartMotion"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_DARK, "Device/Dark","VideoDiagnose", "Alarm","Vdiagnose"},/*25*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_VIDEODIAGNOSE_VI_LOST, "Device/ViLost","VideoDiagnose", "Alarm","Vdiagnose"},/*26*/
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_TEMPHIGH, "Device/TempHigh","TempHigh", "Alarm","Temperature"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_TEMPLOW, "Device/TempLow","TempLow", "Alarm","Temperature"},
    {OVFS_RTSPSERVER_ALARMTYPE_MEDIA_DISCONNECT, "Device/MediaDisconnect","MediaDisconnect", "Alarm","MediaDisconnect"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_HUMANOID, "Device/IvsHumanoid","Humanoid", "Region","Humanoid"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_RUN, "Device/IvsRun","Run", "Region","Run"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_VIOLENTMOTION, "Device/IvsViolentMotion","ViolentMotion", "Region","ViolentMotion"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_MAXHEIGHT, "Device/IvsMaxHeight","MaxHeight", "Wire","MaxHeight"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_HIGHDENSITY, "Device/IvsHighDensity","HighDensity", "Region","HighDensity"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_RETROGRADE, "Device/IvsRetrograde","Retrograde", "Wire","Retrograde"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_SCENCECHAGE, "Device/IvsScenceChange","ScenceChange", "Wire","ScenceChange"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTPERSON, "Device/PersonDetect","Detect", "Alarm","DetectPerson"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_DETECTABSENT, "Device/IvsDetectAbsent","Detect", "Region","DetectAbsent"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_REGIONALINVASIN, "Device/IvsRegionalInvasion","Detect", "Region","RegionalInvasion"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_PERSIONSTAYING, "Device/IvsPersonStaying","Detect", "Region","PersonStaying"},
    {OVFS_RTSPSERVER_ALARMTYPE_IVS_PARKINGVIOLATION, "Device/IvsParkingViolation","Detect", "Region","ParkingViolation"},
    {OVFS_RTSPSERVER_ALARMTYPE_SENSORALARM, "Device/SensorAlarm","SensorAlarm", "Alarm","SensorAlarm"},
};

static void RtspConnectNodeFree(void *ptr)
{
    if (ptr)
        MEDIA_FREE(ptr);
}

static void RtspConnectStateNodeFree(void *ptr)
{
    if (ptr)
        MEDIA_FREE(ptr);
}
static int ConnListAppCompare(void * a, void * b)
{
    if (a == b)
        return 0;
    return -1;
}

static int ConnStateListCompare(void * a, void * b)
{
    if (((RTSP_SESSION_TABLE_T *)a)->sessionId == *(int *)b)
        return 0;

    return -1;
}

static int ConnStateListCheckAndDelete(void * a, void * b)
{
//    LOGW("%d %d %d\n",((RTSP_SESSION_TABLE_T *)a)->isPlay ,  *(time_t *)b , ((RTSP_SESSION_TABLE_T *)a)->connTime);
    if (((RTSP_SESSION_TABLE_T *)a)->isPlay == 0 && *(long long int *)b - ((RTSP_SESSION_TABLE_T *)a)->connTime > 60000LL)
    {
        LOGW("overtime curtime %lld conntime %lld isPlay %d\n",
                        *(long long int *)b,
                        ((RTSP_SESSION_TABLE_T *)a)->connTime,
                        ((RTSP_SESSION_TABLE_T *)a)->isPlay);
        return 0;
    }

    return -1;
}
/*
 *  node == NULL, this function will send app data to all app106 connected node
 *  node != NULL, this function will send app data to this node
 */
static int RtspSendApp106(COMMON_DLIST_T appList, MEDIA_ALARM_STATUS_NODE_T *node, int idx)
{
    int alarmIdx = -1, i = 0, appDataLen = 0, listCnt = 0, ret = 0;
    int sendPic = 0;
    char * appData = NULL;
    struct timeval tmval;
    time_t updateTime_t;
    struct tm updateTm;
    long int timestamp = 0;
    char startTime[32] = { 0 };
    char stopTime[32] = { 0 };
    char utcTime[32] = { 0 };

    if (appList == NULL || node == NULL)
    {
        LOGE("param error \n");
        return -1;
    }

    listCnt = Common_DList_GetCount(appList);
    LOGW("listCnt:[%d] \n",listCnt);
    if (listCnt <= 0)
    {
        node->sendPic = 0;
        return 0;
    }

    for (i = 0;i < COMMON_ARRAY_ELEMENT_COUNT(ovfs_rtspserver_apps); i++)
    {
        if (ovfs_rtspserver_apps[i].alarmKeyName == NULL)
            continue;

        if (strcmp(ovfs_rtspserver_apps[i].alarmKeyName,node->alarmName) == 0)
        {
            alarmIdx = i;
            break;
        }
    }

    if (idx >= 0)
        alarmIdx = idx;

    if (alarmIdx < 0 || (node->startTime.tm_year == 0 && node->stopTime.tm_year == 0))
    {
        return 0;
    }

//    /*filter ivs */
//    if (strcasecmp(node->alarmName, "CounterWire") == 0
//                    || strcasecmp(node->alarmName, "DetectWire") == 0
//                    || strcasecmp(node->alarmName, "ObjectRegion") == 0
//                    || strcasecmp(node->alarmName, "DetectRegion") == 0)
//    {
//        if (node->regionId != 0)
//            return 0;
//    }

    gettimeofday(&tmval,NULL);
    timestamp = tmval.tv_sec*1000 + tmval.tv_usec/1000;
/*
    appData = MEDIA_MALLOC(RTSP_APP_DATA_MAX);
    if (appData == NULL)
    {
        LOGE("malloc failed\n");
        return -1;
    }

    memset(appData,0,RTSP_APP_DATA_MAX);
*/
    time(&updateTime_t);
    localtime_r(&updateTime_t, &updateTm);

    if (node->status == 1)
    {
        /* 开启状态时需要持续更新时间并推送*/
        strftime(startTime, sizeof(startTime) - 1, "%Y-%m-%d %H:%M:%S", &node->startTime);
        strftime(stopTime, sizeof(stopTime) - 1, "%Y-%m-%d %H:%M:%S", &updateTm);
        strftime(utcTime, sizeof(utcTime) - 1, "%Y-%m-%d %H:%M:%S", &node->startTime);
    }
    else
    {
        /* 关闭状态时，需要发送一次时间*/
        strftime(startTime, sizeof(startTime) - 1, "%Y-%m-%d %H:%M:%S", &node->startTime);
        strftime(stopTime, sizeof(stopTime) - 1, "%Y-%m-%d %H:%M:%S", &node->stopTime);
        strftime(utcTime, sizeof(utcTime) - 1, "%Y-%m-%d %H:%M:%S", &node->stopTime);
    }

    if (strcasecmp(node->alarmName, "RegionalInvasion") == 0
        || strcasecmp(node->alarmName, "DetectWire") == 0
        || strcasecmp(node->alarmName, "PersonStaying") == 0
        || strcasecmp(node->alarmName, "DetectAbsent") == 0
        || strcasecmp(node->alarmName, "Retrograde") == 0
        || strcasecmp(node->alarmName, "ParkingViolation") == 0
        )
    {
        /* LOGE("Smart type Alarm srctype %d region id %d\n",node->alarmSrcType,node->regionId); */
        LOGD("node->sendPic:[%d]\n",node->sendPic);
        if(node->status && node->sendPic)
        {
            while(1)
            {
                if(Common_File_IsExist("/tmp/smartsnap/start.jpg"))
                {
                    char *imgData = NULL;
                    int imgDataLen = 0;

                    FILE *fp = Common_File_fOpen("/tmp/smartsnap/start.jpg", "rb");
                    if(fp)
                    {
                        Common_File_fSeek(fp, 0, SEEK_END);
                		int size = Common_File_fTell(fp);
                		Common_File_fSeek(fp,0,SEEK_SET);
                        char *buf = MEDIA_CALLOC(1, size);
                        Common_File_fRead(buf, size, 1, fp);
                        Common_File_fClose(fp);

                        imgData = Common_Base64_Encode(buf, size, &imgDataLen);

                        Common_Free(buf, __FUNCTION__, __LINE__);
                        buf = NULL;

                    }
                    else
                    {
                        LOGE("fOpen start.jpg failed!\n");
                    }

                    sendPic = 1;
                    LOGD("len:[%d]\n",imgDataLen);

                    appData = MEDIA_CALLOC(1, imgDataLen+RTSP_APP_DATA_MAX);

                    appDataLen = snprintf(appData, imgDataLen+RTSP_APP_DATA_MAX - 1, RTSP_APP_DATA_STR_SMART, ovfs_rtspserver_apps[alarmIdx].alarmTns,
                                    utcTime, startTime, stopTime, ovfs_rtspserver_apps[alarmIdx].alarmName,
                                    ovfs_rtspserver_apps[alarmIdx].alarmValue, (node->alarmSrcType == 0) ? 1 : node->channel + 1,
                                    node->status ? "true" : "false", imgData?imgData:"");

                    if(imgData)
                    {
                        Common_Free(imgData, __FUNCTION__, __LINE__);
                        imgData = NULL;
                    }
                    break;

                }
                else
                {
                    LOGE("start.jpg is not exist!\n");
                    Common_Sleep(0, 100 * 1000);
                }
            }

        }
        else
        {
            appData = MEDIA_CALLOC(1, RTSP_APP_DATA_MAX);

            appDataLen = snprintf(appData, RTSP_APP_DATA_MAX - 1, RTSP_APP_DATA_STR, ovfs_rtspserver_apps[alarmIdx].alarmTns,
                            utcTime, startTime, stopTime, ovfs_rtspserver_apps[alarmIdx].alarmName,
                            ovfs_rtspserver_apps[alarmIdx].alarmValue, (node->alarmSrcType == 0) ? 1 : node->channel + 1,
                            node->status ? "true" : "false", "");
        }


    }
    else
    {
//        LOGW("Normal type\n");
        appData = MEDIA_CALLOC(1, RTSP_APP_DATA_MAX);

        appDataLen = snprintf(appData, RTSP_APP_DATA_MAX - 1, RTSP_APP_DATA_STR, ovfs_rtspserver_apps[alarmIdx].alarmTns,
                        utcTime, startTime, stopTime, ovfs_rtspserver_apps[alarmIdx].alarmName,
                        ovfs_rtspserver_apps[alarmIdx].alarmValue, (node->alarmSrcType == 0) ? 1 : node->channel + 1,
                        node->status ? "true" : "false", "");
    }

    if (appDataLen <= 0/* || appDataLen >= RTSP_APP_DATA_MAX - 1*/)
    {
        LOGE("string len error %d\n",appDataLen);
        MEDIA_FREE(appData);
        return -1;
    }
    else
    {
        appData[appDataLen] = '\0';
        appDataLen++;
    }

    //printf("[%s]\n",appData);

    for (i = 0;i < listCnt;i++)
    {
        RTSP_CONNECT_INFO_NODE_T * appNode = Common_DList_GetNode(appList,i);
        if (appNode && appNode->ctype == 106)
        {
            int rtspHandle = appNode->rtspHandle;
            int streamHandle = appNode->streamHandle;
            LOGD("try to send app data to %d channel %d id %d name %s %s status %d start time %s stop time %s\n",
                            appNode->reqInfo.chan,node->channel,node->regionId,ovfs_rtspserver_apps[alarmIdx].alarmKeyName,
                            ovfs_rtspserver_apps[alarmIdx].alarmTns,
                            node->status,startTime,stopTime);
            ret= Ants_RTSPServerV2_InputDataEx(rtspHandle, streamHandle,
                            ANTS_RTSPSERVER_PAYLOADTYPE_APP, appData, appDataLen, timestamp, 0);
        }

        if (ret < 0)
        {
            LOGE("input app data failed %d\n",ret);
        }
    }

    if(sendPic)
    {
        if(node->sendPic)
        {
            node->sendPic = 0;
        }
    }

    MEDIA_FREE(appData);
    return 0;
}

//rtsp://[username]:[password]@[ip]:[port]/[codec]/[channel]/[subtype]/av_stream?ctype=video&transtype=tcp
static int RtspParseHikvisionUrl(char *pUrl, int *pDev, int *pCh, int *pStream, int *ptype, int*stype, int *preferh264)
{
    char *pStr = NULL, *startStr = NULL, tmp1[16] = { 0 }, tmp2[128] = { 0 },url[128] = {0};
    int nRet = 0;
    int nCh = -1, nStream = -1;

	snprintf(url,sizeof(url),"%s",pUrl);

    startStr = strstr(url,"/ch");
    if (startStr == NULL)
        return -1;

	//rtsp://192.168.0.18/ch1/main/av_stream/?ctype=video&transtype=tcp
	LOGD("startStr=%s\n",startStr);
    nRet = sscanf(startStr,"/ch%d/%s",&nCh,tmp2);
    LOGD("ret=%d %d %s\n",nRet,nCh,tmp2);
    if (nRet != 2 )
    {
        return -1;
    }

    nCh = nCh - 1;
    if (strncmp(tmp2,"main",4) == 0)
    {
        nStream = 0;
    }
    else if (strncmp(tmp2,"sub",3) == 0)
    {
        nStream = 1;
    }
    else
        return -1;

    if (nCh < MEDIA_CHANNEL_ID_MIN || nCh > MEDIA_CHANNEL_ID_MAX || nStream < MEDIA_STREAM_ID_VIDEO_MIN || nStream > MEDIA_STREAM_ID_VIDEO_MAX)
    {
        return -1;
    }

	if (pDev)
	{
		*pDev = 0;
	}

    if (pCh)
    {
        *pCh = nCh;
    }

    if (pStream)
    {
        *pStream = nStream;
    }

	int nptype = 0;
    int nstype = 0;
	pStr = strstr(startStr, "transtype=udp");
    if (pStr)
    {
        nptype = 1;
    }

    if (ptype)
    {
        *ptype = nptype;
    }

    if (stype)
    {
        *stype = nstype;
    }

	if (preferh264)
	{
		*preferh264 = 0;
	}

	LOGD("ptype=%d\n",*ptype);
    return 0;
}

//rtsp://[username]:[password]@[ip]:[port]/[codec]/[channel]/[subtype]/av_stream?ctype=video&transtype=tcp
static int RtspParseTianShiTongUrl(char *pUrl, int *pDev, int *pCh, int *pStream, int *ptype, int*stype, int *preferh264)
{
    char *startStr = NULL;
    startStr = strstr(pUrl,"/mpeg4");
    if (startStr == NULL)
    {
        return -1;
    }

	//rtsp://192.168.0.18/mpeg4
	//rtsp://192.168.0.18/mpeg4mpeg4cif
	LOGD("startStr=%s\n",startStr);

    if (strstr(startStr,"mpeg4cif"))
    {
        *pStream = 1;
    }
    else
    {
        *pStream = 0;
    }
    return 0;
}
static int RtspParseRealUrl(char *pUrl, int *pDev, int *pCh, int *pStream, int *ptype, int*stype, int *preferh264)
{
    char *pStr = NULL, *startStr = NULL, *devStr = NULL, tmp1[16] = { 0 }, tmp2[16] = { 0 }, tmp3[16] = { 0 },url[64] = {0};
    int nRet = 0;
    int nCh = -1, nStream = -1;

	snprintf(url,sizeof(url),"%s",pUrl);
	if (NULL != strstr(url,"/av_stream"))
	{
		nRet = RtspParseHikvisionUrl(pUrl, pDev, pCh, pStream, ptype, stype, preferh264);
		return nRet;
	}

    if (NULL != strstr(url,"/mpeg4"))
	{
		nRet = RtspParseTianShiTongUrl(pUrl, pDev, pCh, pStream, ptype, stype, preferh264);
		return nRet;
	}

    startStr = strstr(url,"/ch");
    if (startStr == NULL)
        return -1;

    devStr = strstr(startStr,"dev=");
    if (devStr && pDev)
    {
        sscanf(devStr,"dev=%[0-9]",tmp1);
        *pDev = atoi(tmp1) - 1;
        if (*pDev < 0)
            *pDev = 0;
    }
    else
        *pDev = 0;

    LOGD("device num %d\n",*pDev);

    char tmp4[16] = {0};
    char tmp5[16] = {0};
    nRet = sscanf(startStr,"%*[^ch]ch%[0-9]%[^0-9]%[0-9]%[^0-9]%[0-9]",tmp1,tmp2,tmp3,tmp4,tmp5);
    LOGW("ret %d %s %s %s [%s][%s]\n",nRet,tmp1,tmp2,tmp3,tmp4,tmp5);
    if (nRet < 3 || (strncmp(tmp3,"264",3) != 0 && strncmp(tmp5,"264",3) != 0))
    {
        LOGW("return -1\n");
        return -1;
    }

    nCh = atoi(tmp1) - 1;
    if (strncmp(tmp2,".",1) == 0)
    {
        nStream = 0;
    }

    else if (strncmp(tmp2,"_sub.",5) == 0)
    {
        nStream = 1;
    }
    else if (strncmp(tmp2,"_third.",7) == 0)
    {
        nStream = 2;
    }
    else if (strncmp(tmp2,"_",1) == 0)
    {
        nCh += 1;
        nStream = atoi(tmp3);
    }
    else
        return -1;

    int nptype = 0;
    int nstype = 0;
    pStr = strstr(startStr, "ptype=udp");
    if (pStr)
    {
        nptype = 1;
        pStr = strstr(startStr, "stype=multicast");
        if (pStr)
        {
            nstype = 1;
        }
    }

    pStr = strstr(startStr, "preferh264");
    if (pStr)
    {
        *preferh264 = 1;
    }

    if (nCh < MEDIA_CHANNEL_ID_MIN || nCh > MEDIA_CHANNEL_ID_MAX || nStream < MEDIA_STREAM_ID_VIDEO_MIN || nStream > MEDIA_STREAM_ID_VIDEO_MAX)
    {
        return -1;
    }

    if (pCh != NULL)
    {
        *pCh = nCh;
    }

    if (pStream != NULL)
    {
        *pStream = nStream;
    }

    if (ptype)
    {
        *ptype = nptype;
    }

    if (stype)
    {
        *stype = nstype;
    }

    return 0;
}


static int RtspParseRealDongLiUrl(char *pUrl, int *pDev, int *pCh, int *pStream, int *ptype, int*stype)
{
    char *pStr = NULL, *startStr = NULL, *devStr = NULL, tmp1[16] = { 0 };
    int nRet = 0;
    int nCh = -1, nStream = -1;
    int nptype = 0;
    int nstype = 0;

    startStr = strstr(pUrl,"/stream");
    if (startStr == NULL)
        return -1;

    nCh = 0;

    devStr = strstr(startStr,"dev=");
    if (devStr && pDev)
    {
        sscanf(devStr,"dev=%[0-9]",tmp1);
        *pDev = atoi(tmp1) - 1;
        if (*pDev < 0)
            *pDev = 0;
    }
    else
        *pDev = 0;

    nRet = sscanf(startStr,"%*[^stream]stream%[0-9]",tmp1);
    LOGW("%s ret %d %s\n",pUrl,nRet,tmp1);

    nStream = atoi(tmp1);

    pStr = strstr(startStr, "ptype=udp");
    if (pStr)
    {
        nptype = 1;
    }

    snprintf(tmp1,sizeof(tmp1),"stream%dm",nStream);
    pStr = strstr(startStr, tmp1);
    if (pStr)
    {
        nstype = 2;//for DongLi
    }

    if (nCh < MEDIA_CHANNEL_ID_MIN || nCh > MEDIA_CHANNEL_ID_MAX || nStream < MEDIA_STREAM_ID_VIDEO_MIN || nStream > MEDIA_STREAM_ID_VIDEO_MAX)
    {
        return -1;
    }

    if (pCh != NULL)
    {
        *pCh = nCh;
    }

    if (pStream != NULL)
    {
        *pStream = nStream;
    }

    if (ptype)
    {
        *ptype = nptype;
    }

    if (stype)
    {
        *stype = nstype;
    }

    return 0;
}

static int RtspParseComboUrl(char *pUrl, int *pDev, int *pCh, int *pStream, int *ptype, int*stype, int *preferh264)
{
    char *pStr = NULL, *startStr = NULL, *devStr = NULL, tmp1[16] = { 0 }, tmp2[16] = { 0 }, tmp3[16] = { 0 };
    int nRet = 0;
    int nCh = -1, nStream = -1;

    startStr = strstr(pUrl,"/living_comb");
    if (startStr == NULL)
        return -1;

    devStr = strstr(startStr,"dev=");
    if (devStr && pDev)
    {
        sscanf(devStr,"dev=%[0-9]",tmp1);
        *pDev = atoi(tmp1) - 1;
        if (*pDev < 0)
            *pDev = 0;
    }
    else
        *pDev = 0;

    LOGD("device num %d\n",*pDev);
    nRet = sscanf(startStr,"%*[^living_comb]living_comb%[0-9]%[^0-9]%[0-9]",tmp1,tmp2,tmp3);
    if (nRet != 3 || strncmp(tmp3,"264",3) != 0)
    {
        return -1;
    }

    nCh = atoi(tmp1) - 1;
    if (strncmp(tmp2,".",1) == 0)
    {
        nStream = 0;
    }

    else if (strncmp(tmp2,"_sub.",5) == 0)
    {
        nStream = 1;
    }
    else if (strncmp(tmp2,"_third.",7) == 0)
    {
        nStream = 2;
    }
    else
        return -1;

    int nptype = 0;
    int nstype = 0;
    pStr = strstr(startStr, "ptype=udp");
    if (pStr)
    {
        nptype = 1;
        pStr = strstr(startStr, "stype=multicast");
        if (pStr)
        {
            nstype = 1;
        }
    }

    pStr = strstr(startStr, "preferh264");
    if (pStr)
    {
        *preferh264 = 1;
    }

    if (nCh < MEDIA_CHANNEL_ID_MIN || nCh > MEDIA_CHANNEL_ID_MAX || nStream < MEDIA_STREAM_ID_VIDEO_MIN || nStream > MEDIA_STREAM_ID_VIDEO_MAX)
    {
        return -1;
    }

    if (pCh != NULL)
    {
        *pCh = nCh;
    }

    if (pStream != NULL)
    {
        *pStream = nStream;
    }

    if (ptype)
    {
        *ptype = nptype;
    }

    if (stype)
    {
        *stype = nstype;
    }

    return 0;
}

static int RtspParseTalkingUrl(char *pUrl, int *pCh, int *pType)
{
    char *pStr;
    int nValue = -1;
    int nCh = -1, nType = -1;

    pStr = strstr(pUrl, "audioback");
    if (pStr == NULL)
    {
//        return -1;
    }

    pStr = strstr(pUrl, "ch_");
    if (pStr != NULL)
    {
        if (1 == sscanf(pStr, "ch_%d", &nValue))
        {
            if (nValue >= 0)
            {
                nCh = nValue;
            }
        }
    }

    pStr = strstr(pUrl, "type_g711a");
    if (pStr)
    {
        nType = ANTS_RTSPSERVER_PAYLOADTYPE_G711A;
    }

    pStr = strstr(pUrl, "type_g711u");
    if (pStr)
    {
        nType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
    }

    if (nType < 0)
        nType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;

    if (nCh < 0)
        nCh = MEDIA_CHANNEL_ID_MIN;

    if (nCh < MEDIA_CHANNEL_ID_MIN || nCh > MEDIA_CHANNEL_ID_MAX)
    {
        return -1;
    }

    if (pCh != NULL)
    {
        *pCh = nCh;/*对讲的通道号为0 ?*/
    }

    if (pType != NULL)
    {
        *pType = nType;
    }

    return 0;
}

static int RtspParseMeidaTypeUrl(char *pUrl)
{
    char *tmpStr = NULL, tmp1[32] = { 0 };
    int mediaType = 0;
    if ((tmpStr = strstr(pUrl,"MediaType=")) != NULL)
    {
        sscanf(tmpStr,"MediaType=%[0-9]",tmp1);
        mediaType = atoi(tmp1);
        if (mediaType < 1 || mediaType > 15)
            mediaType = 0;
        LOGD("parser MediaType %d\n",mediaType);
    }
    return mediaType;
}

static void RtspInputTalkingDataThread(void *thParam)
{
    int ret = 0;
    if (thParam == NULL)
        return;

    prctl(PR_SET_NAME, __func__);

    RTSP_CONNECT_INFO_NODE_T *node = (RTSP_CONNECT_INFO_NODE_T*)thParam;
    while (1)
    {
        void *recvData= NULL;
        int recvSize = 0;
        if (node == NULL || node->isExit || node->rtspMgr == NULL || node->rtspMgr->state != MEDIA_RTSP_STATE_START)
        {
            break;
        }

        ret= RestMedia_RequestStreamRead(node->streamQueueHandle,MEDIA_STREAM_QUEUE_ID_AUDIO_REAL,NULL,&recvData,&recvSize,NULL,400);
        if (node == NULL || node->isExit)
        {
            if (ret >= 0 && node != NULL)
            {
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
            }
            break;
        }

        if (ret < 0)
        {
//            LOGE("Get audio data failed! %d, sleep 10ms\n",ret);
            Utils_Sleep(40);
        }
        else
        {
            void * sendData = recvData;
            int sendSize = 0;

            do
            {
                Ovfs_FrameHeader_T *h = NULL;
                h = sendData;
                sendData = sendData + sizeof(Ovfs_FrameHeader_T);
                sendSize = h->uiFrameLen;
                if (sendSize <= 0)
                {
                    LOGE("size error\n");
                    break;
                }
                node->timeStamp += sendSize;
                Ants_RTSPServerV2_InputData(node->rtspHandle, node->streamHandle, sendData, sendSize, node->timeStamp, 1, 0);
                sendData = sendData + sendSize;
            } while (sendData < recvData + recvSize);

            RestMedia_RequestStreamRelease(node->streamQueueHandle);
//            node->lastReadTime = Utils_GetMs();
            node->totalReadSize += (long long unsigned int) recvSize + sizeof(Ovfs_FrameHeader_T);
        }
    }
    pthread_mutex_lock(&node->mutex);
    pthread_cond_signal(&node->cond);
    pthread_mutex_unlock(&node->mutex);
    LOGI("input talk data thread eixt\n");
    return;
}

static void RtspReceiveTalkingDataCallback(int hStreamHandle,unsigned int dwSessionID,unsigned long dwDataType,unsigned long dwProp,unsigned char *lpBuffer,unsigned long dwBufSize,void* lpUser)
{
    if(lpBuffer == NULL || lpUser == NULL)
    {
        return;
    }

    int nPos = 0;
    Ovfs_FrameHeader_T *pHeader;
    RTSP_CONNECT_INFO_NODE_T * node = lpUser;

    do
    {
        if (nPos + sizeof(Ovfs_FrameHeader_T) > dwBufSize)
        {
            break;
        }

        pHeader = (Ovfs_FrameHeader_T *) (lpBuffer + nPos);
        if (pHeader->uiFrameLen + nPos > dwBufSize || pHeader->uiStartId != OVFS_FRAME_STARTCODE || pHeader->uiFrameType != Ovfs_FrameType_AudioFrames)
        {
            LOGE("check head failed\n");
            break;
        }

        int ret = RestMedia_RequestStreamWrite(node->streamQueueHandle, MEDIA_STREAM_QUEUE_ID_AUDIO_TALK,(void *)pHeader,pHeader->uiFrameLen + sizeof(Ovfs_FrameHeader_T));
        if (ret != 0)
        {
            LOGE("wirte audio data failed %d\n",ret);
            break;
        }

        nPos += pHeader->uiFrameLen + sizeof(Ovfs_FrameHeader_T);
    }
    while(1);

    return;
}
//
//static void RtspSmoothingDataHandle(RTSP_SMOOTHING_T *smooth, Ovfs_FrameHeader_T *header, RTSP_CONNECT_INFO_NODE_T *node)
//{
//
//    long long unsigned int llt = Common_GetSystemCount64();
//    if (header->uiFrameType == Ovfs_FrameType_PFrames || header->uiFrameType == Ovfs_FrameType_SubPFrames
//                    || header->uiFrameType == Ovfs_FrameType_ThirdPFrames)
//    {
//        if (smooth->tsBase == 0)
//            smooth->tsBase = header->dwTimeStamp/90;
//
//        if (smooth->ctsBase == 0)
//        {
//            smooth->ctsBase = llt;
//        }
//
//        smooth->tsCnt = (header->dwTimeStamp) / 90  - smooth->tsBase;
//        smooth->ctsCnt = llt - smooth->ctsBase;
//
//        if(llt - smooth->lastDiffTime < 10)
//            usleep(5000);
//        else if (llt - smooth->lastDiffTime < 20)
//            usleep(15000);
////        if ((unsigned int) smooth->ctsCnt < smooth->tsCnt && smooth->tsCnt - (unsigned int) smooth->ctsCnt > 10)
////        {
////            unsigned int diff = smooth->tsCnt - (unsigned int) smooth->ctsCnt;
////            if (diff > 20)
////                usleep(10000);
////            else
////                usleep(5000);
////
////            printf("sleep time %llu diff %u\n",Common_GetSystemCount64() - llt,diff);
//////            if (diff > 40)
//////                LOGE("diff %u\n",smooth->tsCnt - (unsigned int) smooth->ctsCnt);
//////            usleep(10000);
////        }
//
//        llt =  Common_GetSystemCount64();
//
//        printf("pframe %llu ReadRelease %llu ctsCnt %llu tsCnt %u frame no %u frame size %u\n",
//                        llt - smooth->lastDiffTime,
//                        llt - node->lastReadTime,
//                        smooth->ctsCnt,smooth->tsCnt,header->uiFrameNo,header->uiFrameLen);
//
//        smooth->lastDiffTime = Common_GetSystemCount64();
//    }
//    else if (header->uiFrameType == Ovfs_FrameType_IFrames || header->uiFrameType == Ovfs_FrameType_SubIFrames
//                    || header->uiFrameType == Ovfs_FrameType_ThirdIFrames)
//    {
//        smooth->tsCnt = (header->dwTimeStamp) / 90  - smooth->tsBase;
//        smooth->ctsCnt = llt - smooth->ctsBase;
//        printf("Iframe %llu ReadRelease %llu ctsCnt %llu tsCnt %u frame no %u frame size %u\n",
//                        llt - smooth->lastDiffTime,
//                        llt - node->lastReadTime,
//                        smooth->ctsCnt,smooth->tsCnt,header->uiFrameNo,header->uiFrameLen);
//
////        smooth->ctsCnt += header->dwTimeStamp / 90 - smooth->lastDiffTime;
////        smooth->tsCnt = (header->dwTimeStamp) / 90  - smooth->tsBase;
////        smooth->ctsCnt = Common_GetSystemCount64() - smooth->ctsBase;
////        ctsB = Common_GetSystemCount64();
////        smooth->tsBase = header->dwTimeStamp;
////        smooth->ctsCnt += ctsB - ctsA;
////        if (type == 0) //real
////            smooth->ctsCnt += 10;
//        smooth->lastDiffTime = llt;
//    }
//}

static void RtspInputRecordDataThread(void *thParam)
{
    int ret = 0;
    if (thParam == NULL)
        return;

    prctl(PR_SET_NAME, __func__);

    RTSP_CONNECT_INFO_NODE_T *node = (RTSP_CONNECT_INFO_NODE_T*)thParam;

    while (1)
    {
        void *recvData= NULL;
        int recvSize = 0;

        if (node == NULL || node->isExit || node->rtspMgr == NULL || node->rtspMgr->state != MEDIA_RTSP_STATE_START)
        {
            break;
        }

        /*if(node->pause)
        {
            LOGW("record pause\n");
            Utils_Sleep(20);
            continue;
        }*/

        ret= RestMedia_RequestStreamRead(node->streamQueueHandle,-1,NULL,&recvData,&recvSize,NULL,400);

        if (node == NULL || node->isExit)
        {
            if (ret == 0)
                RestMedia_RequestStreamRelease(node->streamQueueHandle);

            break;
        }

        if (ret < 0)
        {
//            LOGD("Get record video data failed! %d, sleep 10ms\n",ret);
            Utils_Sleep(20);
        }
        else
        {
            if (node->liveCombo == 1)
            {
                ret = Ants_RTSPServerV2_InputAntsCombData(node->rtspHandle, node->streamHandle, 0, node->reqInfo.chan + 1, 0, recvData,recvSize);
            }
            else
            {
                Ovfs_FrameHeader_T *h = (Ovfs_FrameHeader_T *)recvData;
                if (h->uiFrameType == Ovfs_FrameType_IFrames || h->uiFrameType == Ovfs_FrameType_PFrames || h->uiFrameType == Ovfs_FrameType_AudioFrames)
                {
                    ret = Ants_RTSPServerV2_InputAntsData(node->rtspHandle, node->streamHandle, recvData,
                                recvSize, 0);
                    node->totalReadSize += (long long unsigned int) recvSize;
                }
                else
                    ret = 0;
            }
            Ovfs_FrameHeader_T *h = (Ovfs_FrameHeader_T *)recvData;
            /* LOGE("reord input data size %d ts %u\n",recvSize, h->uiFrameTime); */
            RestMedia_RequestStreamRelease(node->streamQueueHandle);

            if (ret != 0)
            {
                LOGE("input data failed %d\n",ret);
            }
        }
    }
    pthread_mutex_lock(&node->mutex);
    pthread_cond_signal(&node->cond);
    pthread_mutex_unlock(&node->mutex);
    LOGI("input record data thread exit\n");
    return;
}

static int RtspStatusHandleRealAndTalkingStream(int rtspHandle, int streamHandle, int type,
                Ants_RTSPV2_InParam_T *in, Ants_RTSPV2_OutParam_T *out, RTSP_MGR_T *rtspMgr)
{
    char name[] = "vnd.onvif.metadata";
    int ret = 0;
    MEDIA_AUTH_NODE_T *authNode = NULL;


    authNode = RestMedia_SearchAuthNode(in->dwSessionID);
    if (authNode == NULL)
    {
        LOGE("could not find this authorized info\n");
        return -1;
    }

    /*TODO check configure file*/

    RTSP_CONNECT_INFO_NODE_T* node = MEDIA_MALLOC(sizeof(RTSP_CONNECT_INFO_NODE_T));
    if (node == NULL)
    {
        LOGE("calloc mem failed\n");
        RestMedia_UnregistAuthNode(in->dwSessionID);
        return -ENOMEM;
    }
    memset(node,0,sizeof(RTSP_CONNECT_INFO_NODE_T));
    node->token = Common_Rand32();
    node->rtspMgr = rtspMgr;
    node->rtspHandle = rtspHandle;
    node->streamHandle = streamHandle;
    node->sessionId = in->dwSessionID;
    node->reqInfo.auth = authNode->auth;

    /*talking*/
    if (strstr(in->pUrl,"audioback") != NULL)
    {
        node->reqInfo.type = MEDIA_STREAM_TYPE_TALKING;
        ret = RtspParseTalkingUrl(in->pUrl,&node->reqInfo.chan,&node->audioType);
    }
    else if (in->pRequire == NULL)
    {
        node->reqInfo.type = MEDIA_STREAM_TYPE_REAL;
        node->liveCombo = in->nAntsComb;
        if (in->nAntsComb)
        {
            ret = RtspParseComboUrl(in->pUrl, &node->reqInfo.dev, &node->reqInfo.chan, &node->reqInfo.streamid, &node->protoType, &node->multiCast, &node->preferh264);
            node->mediaType = RtspParseMeidaTypeUrl(in->pUrl);
        }
        else
        {
            ret = RtspParseRealUrl(in->pUrl, &node->reqInfo.dev, &node->reqInfo.chan, &node->reqInfo.streamid, &node->protoType, &node->multiCast, &node->preferh264);
            if (ret < 0)
                ret = RtspParseRealDongLiUrl(in->pUrl, &node->reqInfo.dev, &node->reqInfo.chan, &node->reqInfo.streamid, &node->protoType, &node->multiCast);
        }
        if (ret == 0)
        {
            BOARD_ABILITY_T ability;
            memset(&ability,0,sizeof(BOARD_ABILITY_T));
            RestMedia_RequestBoardAbility(&ability, &node->reqInfo);
            if (ability.videoAbility.devNum > node->reqInfo.dev &&
                            ability.videoAbility.dev[node->reqInfo.dev].chanNum > node->reqInfo.chan &&
                            ability.videoAbility.dev[node->reqInfo.dev].chan[node->reqInfo.chan].streamNum > node->reqInfo.streamid)
            {
                ret = 0;
            }
            else
            {
                LOGW("could not find this stream %d %d %d\n",
                                node->reqInfo.dev,node->reqInfo.chan,node->reqInfo.streamid);
                ret = -1;
            }
        }
    }
    else if (in->pRequire != NULL)
    {
        node->reqInfo.type = MEDIA_STREAM_TYPE_TALKING;
        RtspParseTalkingUrl(in->pUrl,&node->reqInfo.chan,&node->audioType);
        ret = 0;
    }

    if (ret != 0)
    {
        LOGE("RtspParseParam failed!\n");
        RestMedia_UnregistAuthNode(in->dwSessionID);
        MEDIA_FREE(node);
        return -1;
    }

    if (node->reqInfo.type == MEDIA_STREAM_TYPE_REAL)
    {
        if (node->liveCombo)
        {
            node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB;
            node->audioType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
            node->streamType = ANTS_RTSPSERVER_STREAMTYPE_VIDEO;
        }
        else
        {
            /*request venc type from board system */
            node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB;
            node->audioType = RestMedia_RequestAudioType(&node->reqInfo);
            node->streamType = ANTS_RTSPSERVER_STREAMTYPE_ALL;

            node->videoType = RestMedia_RequestVencType(&node->reqInfo);
            void *sps = NULL, *pps = NULL;
            int spsSize = 0, ppsSize = 0;
            RestMedia_RequestNaluType(&node->reqInfo,&sps,&spsSize,&pps,&ppsSize);
            if(spsSize>0)
            {
                Ants_RTSPServerV2_SetConfig(rtspHandle,streamHandle,12,sps,spsSize);
            }
            if(ppsSize>0)
            {
                Ants_RTSPServerV2_SetConfig(rtspHandle,streamHandle,13,pps,ppsSize);
            }

            if (sps)
                MEDIA_FREE(sps);
            if (pps)
                MEDIA_FREE(pps);
            sps = pps = NULL;

            void *vps =NULL;
            int vpsSize = 0;
            if(node->videoType == MEDIA_STREAM_VIDEO_ENC_TYPE_H265)
            {
                RestMedia_RequestVPSType(&node->reqInfo,&vps,&vpsSize);
                if(vpsSize>0)
                {
                    Ants_RTSPServerV2_SetConfig(rtspHandle,streamHandle,14,vps,vpsSize);
                }
            }
            if (vps)
                MEDIA_FREE(vps);
            vps = NULL;

            if (node->videoType == MEDIA_STREAM_VIDEO_ENC_TYPE_H264)
                node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
            else if (node->videoType == MEDIA_STREAM_VIDEO_ENC_TYPE_H265)
                node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_H265;
            else if (node->videoType == MEDIA_STREAM_VIDEO_ENC_TYPE_JPEG)
                node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG;
            else if (node->videoType == MEDIA_STREAM_VIDEO_ENC_TYPE_H264_PLUS)
                node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
            else if (node->videoType == MEDIA_STREAM_VIDEO_ENC_TYPE_H265_PLUS)
                node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_H265;
            else
            {
                LOGE("video type error %d\n",node->videoType);
                ret = -1;
            }

            if (ret == 0 && node->rtspMgr->boardState.audioState != 1)
            {
                /*如果请求音频失败，或者音频没有开启，则只启用视频流*/
                    node->audioType = -1;
                    node->streamType = ANTS_RTSPSERVER_STREAMTYPE_VIDEO;
            }
        }
        out->nCh = node->reqInfo.chan + 1;
        out->nStream = node->reqInfo.streamid + 1;
    }
    else /*talking*/
    {
        node->videoType = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
        node->audioType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
        node->streamType = ANTS_RTSPSERVER_STREAMTYPE_ALL;
        out->nCh = node->reqInfo.chan;
        out->nRequireType = 2;
        out->nRequireAudioType = node->audioType;
    }

    if (ret != 0)
    {
        MEDIA_FREE(node);
        RestMedia_UnregistAuthNode(in->dwSessionID);
        return -1;
    }

    Ants_RTSPServerV2_SetStreamInfo(rtspHandle, streamHandle, node->streamType, node->videoType, node->audioType);

    if (node->reqInfo.streamid == MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN)
    {
        Ants_RTSPServerV2_SetMulticastAddressIPv4(rtspHandle, streamHandle, 0, rtspMgr->cfg.multiMainVideoIp, rtspMgr->cfg.multiMainVideoPort,
                        rtspMgr->cfg.multiMainVideoTtl);
        Ants_RTSPServerV2_SetMulticastAddressIPv4(rtspHandle, streamHandle, 1, rtspMgr->cfg.multiMainVideoIp, rtspMgr->cfg.multiMainVideoPort,
                        rtspMgr->cfg.multiMainVideoTtl);
    }
    else if (node->reqInfo.streamid == MEDIA_STREAM_QUEUE_ID_VIDEO_SUB)
    {
        Ants_RTSPServerV2_SetMulticastAddressIPv4(rtspHandle, streamHandle, 0, rtspMgr->cfg.multiSubVideoIp, rtspMgr->cfg.multiSubVideoPort,
                        rtspMgr->cfg.multiSubVideoTtl);
        Ants_RTSPServerV2_SetMulticastAddressIPv4(rtspHandle, streamHandle, 1, rtspMgr->cfg.multiSubVideoIp, rtspMgr->cfg.multiSubVideoPort,
                        rtspMgr->cfg.multiSubVideoTtl);
    }
    else if (node->reqInfo.streamid == MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD)
    {
        Ants_RTSPServerV2_SetMulticastAddressIPv4(rtspHandle, streamHandle, 0, rtspMgr->cfg.multiThirdVideoIp, rtspMgr->cfg.multiThirdVideoPort,
                        rtspMgr->cfg.multiThirdVideoTtl);
        Ants_RTSPServerV2_SetMulticastAddressIPv4(rtspHandle, streamHandle, 1, rtspMgr->cfg.multiThirdVideoIp, rtspMgr->cfg.multiThirdVideoPort,
                        rtspMgr->cfg.multiThirdVideoTtl);
    }

    Ants_RTSPServerV2_EnableMulticast(rtspHandle, streamHandle, rtspMgr->cfg.multiEnable);

    if (node->multiCast == 2)//DongLi
        Ants_RTSPServerV2_EnableMulticast(rtspHandle, streamHandle, 1);

    Ants_RTSPServerV2_AddAppStream(rtspHandle, streamHandle, ANTS_RTSPSERVER_PAYLOADTYPE_APP, 90000, name);

    LOGI("ch %d stream %d audio %d streamtype %d session id %u client ip %x uri %s\n",node->reqInfo.chan,node->reqInfo.streamid,node->audioType,node->streamType,in->dwSessionID,in->dwClientIP,in->pUrl);
    MEDIA_FREE(node);
    return 0;
}

static int RtspStatusGetRecordFileType()
{
    return 0;
}

static int RtspStatusHandleRecordStream(int rtspHandle, int sessionHandle, int type, Ants_RTSPV2_InParam_T *in, Ants_RTSPV2_OutParam_T *out, RTSP_MGR_T *rtspMgr)
{
    Ants_RTSPServerV2_EnableMulticast(rtspHandle, sessionHandle, 0);
    int nVideoType = ANTS_RTSPSERVER_PAYLOADTYPE_H264, nAudioType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U, nStreamType = ANTS_RTSPSERVER_STREAMTYPE_ALL;

    LOGW("record living combo %d %s session handle %d channel %d streamid %d\n",in->nAntsComb,in->pUrl,sessionHandle,in->nChan,in->nStream);
    LOGW("start time %d stop time %d\n",in->tRangeStart.byHour,in->tRangeStop.byHour);
    if (in->nAntsComb == 1)
    {
        nVideoType = ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB;
        nStreamType = ANTS_RTSPSERVER_STREAMTYPE_VIDEO;
        Ants_RTSPServerV2_SetStreamInfo(rtspHandle, sessionHandle, nStreamType, nVideoType, nAudioType);
    }
    else
    {
        RtspStatusGetRecordFileType();
        Ants_RTSPServerV2_SetStreamInfo(rtspHandle, sessionHandle, nStreamType, nVideoType, nAudioType);
    }

    return 0;
}

static int RtspStatusHandlePlayingCallback(int rtspHandle, int sessionHandle, int type, void *inParam, void *outParam, void *user)
{
    RTSP_MGR_T * rtspMgr = (RTSP_MGR_T *)user;
    Ants_RTSPV2_InParam_T *in = (Ants_RTSPV2_InParam_T *) inParam;
    int ret = 0;

    RTSP_SESSION_TABLE_T *sessionHandleNode = Common_DList_Search(rtspMgr->connStateList,&in->dwSessionID,ConnStateListCompare);
    if (sessionHandleNode == NULL)
    {
        LOGW("could not find this session node\n");
        return 0;
    }
    sessionHandleNode->isPlay = 1;

    if (sessionHandleNode->reqInfo.type == MEDIA_STREAM_TYPE_REAL)
    {

        //LOGW("request i frame\n");
        //RestMedia_RequestVideoIFrame(&sessionHandleNode->reqInfo);

        MEDIA_DISCONNECT_INFO_T disconInfo;
        Mq_Request(rtspMgr->mqHandle, MEDIA_REQ_DISCON_GET_CFG, NULL, 0, &ret, (void *) (&disconInfo),
                        sizeof(MEDIA_DISCONNECT_INFO_T));
        if (ret == 0)
        {
            struct in_addr inaddr;
            inaddr.s_addr = in->dwClientIP;
            char disconIp[32] = { 0 };
            snprintf(disconIp, sizeof(disconIp), "%s", inet_ntoa(inaddr));
            if (strcmp(disconIp, disconInfo.disconnectIp) == 0
                            && sessionHandleNode->reqInfo.dev == disconInfo.deviceId
                            && sessionHandleNode->reqInfo.chan == disconInfo.channelId
                            && sessionHandleNode->reqInfo.streamid == disconInfo.streamId)
            {
                int state = 0;
                Mq_Request(rtspMgr->mqHandle, MEDIA_REQ_DISCON_SET_EVENT, &state, sizeof(int), &ret,NULL,0);
                if (ret == 0)
                    RestMedia_SendMediaDisconnectEvent();
                LOGW("match ip %s send event 0 \n", disconIp);
            }
        }
    }
    return 0;
}

static int RtspStatusHandleStopingCallback(int rtspHandle, int sessionHandle, int type, void *inParam, void *outParam, void *user)
{
    RTSP_MGR_T * rtspMgr = (RTSP_MGR_T *) user;
    Ants_RTSPV2_InParam_T *in = (Ants_RTSPV2_InParam_T *) inParam;
    int ret = 0;

    LOGW("conn state list count %d remove dwSessionId %d\n", Common_DList_GetCount(rtspMgr->connStateList), in->dwSessionID);

    if (in->nReason != ANTS_RTSPSERVER_ERROR_Close_Normal && in->nReason != ANTS_RTSPSERVER_ERROR_NoError)
    {
        LOGW("remote client close abnormal, reason %d\n", in->nReason);

        RTSP_SESSION_TABLE_T *sessionHandleNode = Common_DList_Search(rtspMgr->connStateList, &in->dwSessionID,
                        ConnStateListCompare);
        if (sessionHandleNode != NULL && sessionHandleNode->reqInfo.type == MEDIA_STREAM_TYPE_REAL)
        {
            MEDIA_DISCONNECT_INFO_T disconInfo;

            Mq_Request(rtspMgr->mqHandle, MEDIA_REQ_DISCON_GET_CFG, NULL, 0, &ret, (void *) (&disconInfo),
                            sizeof(MEDIA_DISCONNECT_INFO_T));
            if (ret == 0)
            {
                struct in_addr inaddr;
                inaddr.s_addr = in->dwClientIP;
                char disconIp[32] = { 0 };
                snprintf(disconIp, sizeof(disconIp), "%s", inet_ntoa(inaddr));
                if (strcmp(disconIp, disconInfo.disconnectIp) == 0
                                && sessionHandleNode->reqInfo.dev == disconInfo.deviceId
                                && sessionHandleNode->reqInfo.chan == disconInfo.channelId
                                && sessionHandleNode->reqInfo.streamid == disconInfo.streamId)
                {
                    int state = 1;
                    Mq_Request(rtspMgr->mqHandle, MEDIA_REQ_DISCON_SET_EVENT, &state, sizeof(int), &ret,NULL,0);
                    if (ret == 0)
                        RestMedia_SendMediaDisconnectEvent();
                    LOGW("match ip %s send event 1 \n", disconIp);
                }
            }
        }
    }

    RestMedia_UnregistAuthNode(in->dwSessionID);
    Common_DList_Delete(rtspMgr->connStateList, &in->dwSessionID, ConnStateListCompare);
    return 0;
}

static int RtspStatusCallback(int rtspHandle, int sessionHandle, int type, void *inParam, void *outParam, void *user)
{
    /*TODO parser url get types*/
    int ret = -1;

    RTSP_MGR_T * rtspMgr = (RTSP_MGR_T *)user;
    Ants_RTSPV2_InParam_T *in = (Ants_RTSPV2_InParam_T *) inParam;
    Ants_RTSPV2_OutParam_T *out = (Ants_RTSPV2_OutParam_T *) outParam;
    LOGD("type:[%d]\n",type);
    if (rtspMgr == NULL || in == NULL
                    || in->pUrl == NULL)//ANTS_RTSPSERVER_CALLBACK_TYPE_SET_PARAM
    {
        LOGD("param error\n");
        return -1;
    }
    if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_STOPPING)
    {
        ret = RtspStatusHandleStopingCallback(rtspHandle, sessionHandle, type, inParam, outParam, user);
        return ret;
    }
    else if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING)
    {
        ret = RtspStatusHandlePlayingCallback(rtspHandle, sessionHandle, type, inParam, outParam, user);
        return ret;
    }
    else if (type != ANTS_RTSPSERVER_CALLBACK_TYPE_SET_PARAM)
    {
        LOGD("rtsp type error %d\n",type);
        return -1;
    } else if (out == NULL)
    {
        LOGD("param error\n");
        return -1;
    }

    LOGI("type %d living combo %d recording %d sessionId %u sessionHandle %d\n",type,in->nAntsComb,in->bRecording,in->dwSessionID,sessionHandle);

    if (rtspMgr->state != MEDIA_RTSP_STATE_START)
    {
        LOGD("state error\n");
        return -1;
    }

    if (in->bRecording)
        ret = RtspStatusHandleRecordStream(rtspHandle,sessionHandle,type,in,out,rtspMgr);
    else
        ret = RtspStatusHandleRealAndTalkingStream(rtspHandle,sessionHandle,type,in,out,rtspMgr);

    if (ret == 0)
    {
        RTSP_SESSION_TABLE_T *sessionHandleNode = MEDIA_MALLOC(sizeof(RTSP_SESSION_TABLE_T));
        memset(sessionHandleNode,0,sizeof(RTSP_SESSION_TABLE_T));
        sessionHandleNode->sessionHandle = sessionHandle;
        sessionHandleNode->sessionId = in->dwSessionID;
        sessionHandleNode->liveCombo = in->nAntsComb;
        sessionHandleNode->connTime = Utils_GetMs();
        if (Common_DList_Delete(rtspMgr->connStateList,&in->dwSessionID,ConnStateListCompare) == 0)
        {
            RestMedia_UnregistAuthNode(in->dwSessionID);
        }
        Common_DList_InsertTail(rtspMgr->connStateList, sessionHandleNode, sizeof(RTSP_SESSION_TABLE_T));
    }
    LOGD("\n");
    return ret;
}

static int RtspOpenRealAndTalking(int rtspHandle, int streamHandle, RTSP_CONNECT_INFO_NODE_T * node, Ants_RTSPV2_InParam_T *in)
{
    int ret = 0;
    /*talking*/
    if (strstr(in->pUrl,"audioback") != NULL)
    {
        node->reqInfo.type = MEDIA_STREAM_TYPE_TALKING;
        ret = RtspParseTalkingUrl(in->pUrl,&node->reqInfo.chan,&node->audioType);
    }
    else if (in->pRequire == NULL)
    {
        node->reqInfo.type = MEDIA_STREAM_TYPE_REAL;
        node->liveCombo = in->nAntsComb;
        if (in->nAntsComb)
        {
            ret = RtspParseComboUrl(in->pUrl, &node->reqInfo.dev, &node->reqInfo.chan, &node->reqInfo.streamid, &node->protoType, &node->multiCast, &node->preferh264);
            node->mediaType = RtspParseMeidaTypeUrl(in->pUrl);
        }
        else
        {
            ret = RtspParseRealUrl(in->pUrl, &node->reqInfo.dev, &node->reqInfo.chan, &node->reqInfo.streamid, &node->protoType, &node->multiCast, &node->preferh264);
            if (ret < 0)
                ret = RtspParseRealDongLiUrl(in->pUrl, &node->reqInfo.dev, &node->reqInfo.chan, &node->reqInfo.streamid, &node->protoType, &node->multiCast);
        }
    }
    else if (in->pRequire != NULL)
    {
        node->reqInfo.type = MEDIA_STREAM_TYPE_TALKING;
        ret = 0;
    }
    else
        ret = -1;

    if (ret != 0)
    {
        LOGE("RtspParseParam failed!\n");
        return -1;
    }

    if (strstr(in->pUrl,"ctype=app106") != NULL)
        node->ctype = 106;

    if (node->ctype == 0)
    {
        node->streamQueueHandle = RestMedia_RequestStreamOpen(&node->reqInfo);
        if (node->streamQueueHandle < 0)
        {
            LOGE("open stream dev %d channel %d stream %d failed\n",node->reqInfo.dev,node->reqInfo.chan, node->reqInfo.streamid);
            return -1;
        }

        if (node->reqInfo.type == MEDIA_STREAM_TYPE_TALKING)
        {
            pthread_condattr_t condattr;
            pthread_condattr_init(&condattr);
            pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
            pthread_cond_init(&node->cond, &condattr);
            pthread_condattr_destroy(&condattr);
            pthread_mutex_init(&node->mutex,NULL);

            pthread_pool_add(node->rtspMgr->tp,RtspInputTalkingDataThread,(void *)node);
            Ants_RTSPServerV2_SetStreamCallBack(rtspHandle,in->dwSessionID,0,RtspReceiveTalkingDataCallback,node);
            struct in_addr in;
            in.s_addr = node->connectIp;
            //RestMedia_WriteEventLog(MAJOR_OPERATION,MINOR_START_VT,node->reqInfo.chan + 1,inet_ntoa(in));
        }

//        Utils_Sleep(10);
    }

    Common_DList_InsertTail(node->rtspMgr->conList,node,sizeof(RTSP_CONNECT_INFO_NODE_T));
    return 0;
}

static int RtspOpenRecord(int rtspHandle, int streamHandle, RTSP_CONNECT_INFO_NODE_T * node, Ants_RTSPV2_InParam_T *in)
{
    LOGW("record stream rtsphandle %d streamHandle %d session id %d\n",rtspHandle,streamHandle,in->dwSessionID);
    Ants_RtspDayTime startTime, stopTime;
    char *tmpStr = NULL, tmp1[32] = { 0 };
    node->liveCombo = in->nAntsComb;
    node->reqInfo.type = MEDIA_STREAM_TYPE_RECORD;
    memcpy(&startTime,&in->tRangeStart,sizeof(Ants_RtspDayTime));
    memcpy(&stopTime,&in->tRangeStop,sizeof(Ants_RtspDayTime));

    tmpStr = strstr(in->pUrl,"dev=");
    if (tmpStr)
    {
        sscanf(tmpStr,"dev=%[0-9]",tmp1);
        node->reqInfo.dev = atoi(tmp1) - 1;
        if (node->reqInfo.dev < 0)
            node->reqInfo.dev = 0;
    }

    tmpStr = strstr(in->pUrl,"ch=");
    if (tmpStr)
    {
        sscanf(tmpStr,"ch=%[0-9]",tmp1);
        node->reqInfo.chan = atoi(tmp1) - 1;
        if (node->reqInfo.chan < 0)
            node->reqInfo.chan = 0;
    }

    if (node->liveCombo == 0)
    {
        char *pTmp;
        pTmp = strstr(in->pUrl,"startime=");
        if(pTmp != NULL)
        {
            pTmp += strlen("startime=");
            startTime.wYear = (pTmp[0]-'0')*1000 + (pTmp[1]-'0')*100 + (pTmp[2]-'0')*10 + (pTmp[3]-'0');
            startTime.byMon = (pTmp[4]-'0')*10 + (pTmp[5]-'0');
            startTime.byDay = (pTmp[6]-'0')*10 + (pTmp[7]-'0');

            startTime.byHour = (pTmp[9]-'0')*10 + (pTmp[10]-'0');
            startTime.byMin = (pTmp[11]-'0')*10 + (pTmp[12]-'0');
            startTime.bySec = (pTmp[13]-'0')*10 + (pTmp[14]-'0');
        }

        pTmp = strstr(in->pUrl,"endtime=");
        if(pTmp != NULL)
        {
            pTmp += strlen("endtime=");
            stopTime.wYear = (pTmp[0]-'0')*1000 + (pTmp[1]-'0')*100 + (pTmp[2]-'0')*10 + (pTmp[3]-'0');
            stopTime.byMon = (pTmp[4]-'0')*10 + (pTmp[5]-'0');
            stopTime.byDay = (pTmp[6]-'0')*10 + (pTmp[7]-'0');

            stopTime.byHour = (pTmp[9]-'0')*10 + (pTmp[10]-'0');
            stopTime.byMin = (pTmp[11]-'0')*10 + (pTmp[12]-'0');
            stopTime.bySec = (pTmp[13]-'0')*10 + (pTmp[14]-'0');
        }
    }

    /* RestMedia_CheckPlayBackTime(&startTime,&stopTime); */

    node->reqInfo.recordPlayStartYear = startTime.wYear;
    node->reqInfo.recordPlayStartMon = startTime.byMon;
    node->reqInfo.recordPlayStartDay = startTime.byDay;
    node->reqInfo.recordPlayStartHour = startTime.byHour;
    node->reqInfo.recordPlayStartMinute = startTime.byMin;
    node->reqInfo.recordPlayStartSecond = startTime.bySec;
    if (in->bRangeValid & 2)
    {
        node->reqInfo.recordPlayStopYear = stopTime.wYear;
        node->reqInfo.recordPlayStopMon = stopTime.byMon;
        node->reqInfo.recordPlayStopDay = stopTime.byDay;
        node->reqInfo.recordPlayStopHour = stopTime.byHour;
        node->reqInfo.recordPlayStopMinute = stopTime.byMin;
        node->reqInfo.recordPlayStopSecond = stopTime.bySec;
    }

    node->streamQueueHandle = RestMedia_RequestStreamOpen(&node->reqInfo);
    if (node->streamQueueHandle < 0)
    {
        LOGE("open record stream dev %d channel %d stream %d failed\n",node->reqInfo.dev,node->reqInfo.chan, node->reqInfo.streamid);
        return -1;
    }

    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&node->cond, &condattr);
    pthread_condattr_destroy(&condattr);
    pthread_mutex_init(&node->mutex,NULL);

    pthread_pool_add(node->rtspMgr->tp,RtspInputRecordDataThread,(void *)node);

    struct in_addr inAdd;
    inAdd.s_addr = node->connectIp;
    //RestMedia_WriteEventLog(MAJOR_OPERATION,MINOR_REMOTE_PLAYBYTIME,node->reqInfo.chan + 1,inet_ntoa(inAdd));
    LOGW("open record stream dev %d channel %d stream %d\n",node->reqInfo.dev,node->reqInfo.chan, node->reqInfo.streamid);
    Utils_Sleep(10);
    return 0;
}


static int RtspDiPinXian_Open(RTSP_DIPINXIAN_T *dpxCt)
{
    if (access("/root/video_delay",F_OK) != 0)
        return -1;

    char tmpStr[8] = { 0 };
    dpxCt->fp = fopen("/root/video_delay","rw");
    if (dpxCt->fp == NULL)
        return -1;

    fseek(dpxCt->fp,0,SEEK_SET);
    fgets(tmpStr,sizeof(tmpStr),dpxCt->fp);
    dpxCt->delayCnt = atoi(tmpStr);
    fgets(tmpStr,sizeof(tmpStr),dpxCt->fp);
    dpxCt->isPrint = atoi(tmpStr);

    LOGW("video frame delay %d debug print %d\n",dpxCt->delayCnt,dpxCt->isPrint);
    return 0;
}

static int RtspDiPinXian_Close(RTSP_DIPINXIAN_T *dpxCt)
{
    if (dpxCt->fp != NULL)
    {
        fclose(dpxCt->fp);
        dpxCt->fp = NULL;
        memset(dpxCt, 0, sizeof(RTSP_DIPINXIAN_T));
    }
    return 0;
}

static int RtspFrameFilter(RTSP_CONNECT_INFO_NODE_T *node)
{
    int mediaType = 0;

    if (node->mediaType == 0)
        return 0;

    /*Horizon Frame*/
    if (node->frameHeader->uiFrameType >= Ovfs_FrameType_HorizonSmart_Frame
                    && node->frameHeader->uiFrameType <= Ovfs_FrameType_HorizonSmart_Snap)
    {
        mediaType = (1 << (node->frameHeader->uiFrameType & 0xF));
    }
    else if (node->frameHeader->uiFrameType == Ovfs_FrameType_SmartIFrames || node->frameHeader->uiFrameType == Ovfs_FrameType_SmartPFrames)
    {
        mediaType = (1 << 1);
    }
    else /*Other Frame*/
    {
        mediaType = (1 << 0);
    }

    if (node->mediaType & mediaType)
    {
        return 0;
    }

    return -1;
}

static int RtspOpenCallback(int rtspHandle, int streamHandle, void *inParam, void *outParam, void **streamUser, void *user)
{
    LOGD("\n");
    int ret = 0;
    Ants_RTSPV2_InParam_T *in = (Ants_RTSPV2_InParam_T *) inParam;
    RTSP_CONNECT_INFO_NODE_T *node = NULL;
    prctl(PR_SET_NAME,__func__);
    RTSP_MGR_T * rtspMgr = (RTSP_MGR_T *) user;
    MEDIA_AUTH_NODE_T *authNode = NULL;
    RTSP_SESSION_TABLE_T *sessionTableNode = NULL;

    if (rtspMgr == NULL || in == NULL || in->pUrl == NULL)
    {
        LOGE("RtspOpenCallback param failed!\n");
        return -1;
    }

    if (rtspMgr->state != MEDIA_RTSP_STATE_START)
    {
        LOGD("state error\n");
        return -1;
    }
/* #ifdef STREAM_CNT_LIMIT */
    if (strstr(in->pUrl,"audioback") == NULL && strstr(in->pUrl,"ctype=app106") == NULL
        && rtspMgr->streamCount >= rtspMgr->cfg.streamMaxNum)
    {
        LOGW("reach the max number of connection %d max %d\n",rtspMgr->streamCount,
             rtspMgr->cfg.streamMaxNum);
        return -1;
    }
/* #endif */
    authNode = RestMedia_SearchAuthNode(in->dwSessionID);
    if (authNode == NULL)
    {
        LOGE("could not find auth info\n");
        Common_DList_Delete(rtspMgr->connStateList, &in->dwSessionID, ConnStateListCompare);
        return -1;
    }

    sessionTableNode = Common_DList_Search(rtspMgr->connStateList,&in->dwSessionID,ConnStateListCompare);
    if (sessionTableNode == NULL)
    {
        LOGE("could not find session node info\n");
        RestMedia_UnregistAuthNode(in->dwSessionID);
        return -1;
    }

    *streamUser = MEDIA_MALLOC(sizeof(RTSP_CONNECT_INFO_NODE_T));
    if (*streamUser == NULL)
    {
        LOGE("calloc mem failed\n");
        RestMedia_UnregistAuthNode(in->dwSessionID);
        Common_DList_Delete(rtspMgr->connStateList, &in->dwSessionID, ConnStateListCompare);
        return -ENOMEM;
    }

    memset(*streamUser,0,sizeof(RTSP_CONNECT_INFO_NODE_T));
    node = *streamUser;
    node->sessionHandle = sessionTableNode->sessionHandle;
    node->token = Common_Rand32();
    node->rtspMgr = rtspMgr;
    node->rtspHandle = rtspHandle;
    node->streamHandle = streamHandle;
    node->reqInfo.auth = authNode->auth;
    node->sessionId = in->dwSessionID;
    node->connectIp = in->dwClientIP;
    node->customType = RestMedia_GetCustomType();

    LOGI("rtsp handle %d sessionHandle %d stream handle %d session id %u url %s ip 0x%x port %u \n",
                    rtspHandle,node->sessionHandle ,streamHandle,in->dwSessionID,in->pUrl,in->dwClientIP,in->wRemotePort);

    RtspDiPinXian_Open(&node->dpxCt);

    if (in->bRecording || strstr(in->pUrl,"record"))
        ret = RtspOpenRecord(rtspHandle,streamHandle,node,in);
    else
        ret = RtspOpenRealAndTalking(rtspHandle,streamHandle,node,in);

    LOGW("conn list count %d session handle %u sessionId %u\n",Common_DList_GetCount(node->rtspMgr->conList),node->sessionHandle,node->sessionId);

    if (ret < 0)
    {
        RestMedia_UnregistAuthNode(in->dwSessionID);
        Common_DList_Delete(rtspMgr->connStateList, &in->dwSessionID, ConnStateListCompare);
        MEDIA_FREE(node);
        return ret;
    }

/* #ifdef STREAM_CNT_LIMIT */
    if ((node->reqInfo.type == MEDIA_STREAM_TYPE_REAL && node->ctype != 106) ||
        node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
    {
        rtspMgr->streamCount++;
        LOGE("current stream count %d max %d\n",rtspMgr->streamCount, rtspMgr->cfg.streamMaxNum);
    }

/* #endif */
    memcpy(&sessionTableNode->reqInfo,&node->reqInfo,sizeof(MEDIA_REQ_STREAM_INFO_T));

#if (defined PLATFORM_HI3516EV200)
    if (node->preferh264 == 1)
    {
        RestMedia_RequestBoardCodecTempChange(1, &node->reqInfo);
    }
#endif
    LOGD("\n");
    return ret;
}

static int RtspControlRealRecord(int rtspHandle, int streamHandle, int type, RTSP_CONNECT_INFO_NODE_T *node,Ants_RTSPV2_InParam_T *in)
{
    if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING)
    {
        int force = 0;

        if (in->bPause != node->pause)
        {
            if (in->bPause)
                RestMedia_RequestRecordControl(node->streamQueueHandle, MEDIA_STREAM_CONTROL_STOP, NULL);
            else
                RestMedia_RequestRecordControl(node->streamQueueHandle, MEDIA_STREAM_CONTROL_PLAY, NULL);
            node->pause = in->bPause;
        }

        if (in->nFrame != node->forceI)
        {
            if (in->nFrame)
            {
                force = 1;
                RestMedia_RequestRecordControl(node->streamQueueHandle,MEDIA_STREAM_CONTROL_FORCE_IFRAME,&force);
            }
            else
            {
                force = 0;
                RestMedia_RequestRecordControl(node->streamQueueHandle,MEDIA_STREAM_CONTROL_FORCE_IFRAME,&force);
            }
            node->forceI = in->nFrame;
        }
    }
    return 0;
}

static int RtspControlCallback(int rtspHandle, int streamHandle, int type, void *inParam, void *outParam, void *streamUser)
{
    Ants_RTSPV2_InParam_T *in = (Ants_RTSPV2_InParam_T *) inParam;
    RTSP_CONNECT_INFO_NODE_T * node = (RTSP_CONNECT_INFO_NODE_T *)streamUser;
    MEDIA_AUTH_INFO_T authInfo = { 0 };
    int ret = -1;
	char strP[16] = {0};
	snprintf(strP, sizeof(strP), "ovfsZSJQZLHL");

    LOGI("type %d url %s\n",type,in->pUrl);

    if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING)
    {
        if (node->reqInfo.type == MEDIA_STREAM_TYPE_REAL)
        {
            node->isPlaying = 1;
            LOGW("isPlaying!\n");
        }
    }
    /*Authorize handle*/
    if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_AUTH)
    {
        Ants_RTSPV2_AuthInParam_T *pIn = (Ants_RTSPV2_AuthInParam_T *)inParam;
        Ants_RTSPV2_AuthOutParam_T *pOut = (Ants_RTSPV2_AuthOutParam_T *)outParam;
        if (pIn == NULL || pOut == NULL)
        {
            LOGE("%s\n",strerror(EINVAL));
            return 0;
        }

        if (pIn->dwSessionID == 0)
        {
            LOGE("dwSessionId was 0\n");
            return 0;
        }

        if (s_rtspMgr.cfg.authMask != 0 && (pIn->nAuthType < MEDIA_AUTH_TYPE_TEXT || pIn->nAuthType > MEDIA_AUTH_TYPE_DIGEST || (pIn->nAuthType & s_rtspMgr.cfg.authMask) == 0))
        {
            LOGW("auth type error or unsuport %d cfg auth mask %d\n",pIn->nAuthType,s_rtspMgr.cfg.authMask);
            pOut->bAuthOk = 0;
            if (s_rtspMgr.cfg.authMask >= MEDIA_AUTH_TYPE_DIGEST)
            pOut->nAuthType = 4; // extern digest authorize
            else
                pOut->nAuthType = 3; // extern base64 authorize

            LOGD("\n");
            return 0;
        }

        authInfo.sessionId = in->dwSessionID;

        if (s_rtspMgr.cfg.authMask == 0)
        {
            authInfo.authMethod = MEDIA_AUTH_TYPE_TEXT;
            authInfo.userName = "(null)";
            authInfo.password = strP;
        }
        else
        {
            if (pIn->nAuthType == 1)
            {
                authInfo.authMethod = MEDIA_AUTH_TYPE_TEXT;
                authInfo.userName = pIn->pUserName;
                authInfo.password = pIn->pPassword;
            }
            else if (pIn->nAuthType == 2)
            {
                /*TEST*/
                authInfo.authMethod = MEDIA_AUTH_TYPE_DIGEST;
                authInfo.userName = pIn->pUserName;
                authInfo.digest.cnonce = "";
                authInfo.digest.nc = "";
                authInfo.digest.opaque = "";
                authInfo.digest.qop = "";
                authInfo.digest.realm = pIn->pRealm;
                authInfo.digest.nonce = pIn->pNonce;
                authInfo.digest.uri = pIn->pUri;
                authInfo.digest.response = pIn->pResponse;
                authInfo.digest.method = pIn->pCmd;

            }
        }

        struct in_addr inaddr;
        inaddr.s_addr = in->dwClientIP;
        snprintf(authInfo.ipStr, sizeof(authInfo.ipStr), "%s", inet_ntoa(inaddr));

        ret = RestMedia_RegistAuthNode(&authInfo);

        if (ret > 0)
        {
            pOut->bAuthOk = 1;
            if (pIn->nAuthType == 1)
                pOut->nAuthType = 3;
            else
                pOut->nAuthType = 4;
        }
        else
        {
            pOut->bAuthOk = 0;
            if (s_rtspMgr.cfg.authMask >= MEDIA_AUTH_TYPE_DIGEST)
                pOut->nAuthType = 4; // extern digest authorize
            else
                pOut->nAuthType = 3; // extern base64 authorize
        }
        LOGD("\n");
        return 0;
    }

    if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_ADDCH)
    {
        if (Common_DList_Search(node->rtspMgr->connStateList,&in->dwSessionID,ConnStateListCompare) == NULL)
        {
            LOGE("could not find session node info\n");
            Ants_RTSPServerV2_CloseStream(node->rtspHandle,node->sessionHandle);
            return -1;
        }

        if (RestMedia_SearchAuthNode(in->dwSessionID) == NULL)
        {
            LOGE("could not find auth info\n");
            Ants_RTSPServerV2_CloseStream(node->rtspHandle,node->sessionHandle);
            return -1;
        }

        char *tmp = in->pChChange;
        char delims[] = ";";
        char *token = NULL;
        int channel = 0, streamId = 0;
        LOGW(" %s\n",in->pChChange);
        while ((token = strsep(&tmp, delims)) != NULL)
        {
            channel = atoi(token);
            if (channel == 0)
            {
                LOGW("channel number %d\n", channel);
                break;
            }
            if (channel < 0)
            {
                channel = -channel;
                streamId = 1;
            }
            else
                streamId = channel >> 16;

            channel = (channel & 0xffff) - 1;

            if (streamId < MEDIA_STREAM_ID_VIDEO_MIN || streamId > MEDIA_STREAM_ID_VIDEO_MAX
                            || channel < MEDIA_CHANNEL_ID_MIN || channel > MEDIA_CHANNEL_ID_MAX)
            {
                LOGE("channel or stream id out of range %d %d\n",channel,streamId);
                return 0;
            }
            ret = RestMedia_RequestStreamAdd(&node->streamQueueHandle,&node->reqInfo,channel,streamId);
            if (ret != 0)
                break;
        }

        if (ret == 0)
            RestMedia_RequestStreamAddOrDecHandle(&node->streamQueueHandle,&node->reqInfo);
    }

    if (type == ANTS_RTSPSERVER_CALLBACK_TYPE_DECCH)
    {
        if (Common_DList_Search(node->rtspMgr->connStateList,&in->dwSessionID,ConnStateListCompare) == NULL)
        {
            LOGE("could not find session node info\n");
            Ants_RTSPServerV2_CloseStream(node->rtspHandle,node->sessionHandle);
            return -1;
        }

        if (RestMedia_SearchAuthNode(in->dwSessionID) == NULL)
        {
            LOGE("could not find auth info\n");
            Ants_RTSPServerV2_CloseStream(node->rtspHandle,node->sessionHandle);
            return -1;
        }

        char *tmp = in->pChChange;
        char delims[] = ";";
        char *token = NULL;
        int channel = 0, streamId = 0;
        LOGW("%s\n",in->pChChange);
        while ((token = strsep(&tmp, delims)) != NULL)
        {
            channel = atoi(token);
            if (channel == 0)
            {
                LOGW("channel number %d\n", channel);
                break;
            }
            if (channel < 0)
            {
                channel = -channel;
                streamId = 1;
            }
            else
                streamId = channel >> 16;

            channel = (channel & 0xffff) - 1;
            if (streamId < MEDIA_STREAM_ID_VIDEO_MIN || streamId > MEDIA_STREAM_ID_VIDEO_MAX
                            || channel < MEDIA_CHANNEL_ID_MIN || channel > MEDIA_CHANNEL_ID_MAX)
            {
                LOGE("channel or stream id out of range %d %d\n",channel,streamId);
                return 0;
            }

            ret = RestMedia_RequestStreamDec(&node->streamQueueHandle,&node->reqInfo,channel,streamId);
            if (ret != 0)
                break;
        }

        if (ret == 0)
            RestMedia_RequestStreamAddOrDecHandle(&node->streamQueueHandle,&node->reqInfo);
    }
    if (node == NULL || node->rtspMgr == NULL || node->reqInfo.auth == NULL)
    {
        LOGD("\n");
        return 0;
    }

    if (node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
        ret = RtspControlRealRecord(rtspHandle, streamHandle, type, node, in);
    else
        ret = 0;

    LOGD("\n");
    return 0;
}

#define DIFF_TIMEVAL(ta, tb) \
({\
    int tmpDiffMs = 0; \
    struct timeval *pa = &(ta), *pb = &(tb); \
    if (ta.tv_sec < tb.tv_sec || (ta.tv_sec == tb.tv_sec && ta.tv_usec < tb.tv_usec)) \
    { \
        pa = &(tb); pb = &(ta); \
    }\
    if (pa->tv_usec >= pb->tv_usec) \
    {\
        tmpDiffMs = (pa->tv_usec - pb->tv_usec) / 1000 + (pa->tv_sec - pb->tv_sec) * 1000;\
    }\
    else \
    {\
        tmpDiffMs = (1000000 + pa->tv_usec - pb->tv_usec) / 1000 + (pa->tv_sec - pb->tv_sec - 1) * 1000;\
    };\
    tmpDiffMs;\
})

static void RtspSmoothingDataHandle(RTSP_SMOOTHING_T *smooth, Ovfs_FrameHeader_T *header, int debugPrint)
{
    if (header->uiFrameType == Ovfs_FrameType_IFrames || header->uiFrameType == Ovfs_FrameType_PFrames)
    {
        int sleepPlan = 0, intervalTimeMs = 0;
        unsigned int avlFrame = 0;
        long long unsigned int llt = 0;

        if (smooth->lastTimeStamp == 0)
            smooth->lastTimeStamp = header->dwTimeStamp;

        if (smooth->difftime == 0)
            smooth->difftime = Common_GetSystemCount64();

        intervalTimeMs = (header->dwTimeStamp - smooth->lastTimeStamp)/90;
        if (intervalTimeMs <= 10)
            intervalTimeMs = 40;

        if (debugPrint > 0)
            gettimeofday(&smooth->timeP0, NULL);
//        if (header->uiFrameType == Ovfs_FrameType_IFrames)
//        {
//            smooth->lastIfSendCost = DIFF_TIMEVAL(smooth->timeP0,smooth->timeP1);
//        }

        sleepPlan = intervalTimeMs - 10;

        if (smooth->restCnt > 0)
            avlFrame = smooth->restCnt;

        if (avlFrame < 3 && sleepPlan > (int) avlFrame * 2)
        {
            sleepPlan -= avlFrame * 2;
        }
        else if ((avlFrame >= 3 && avlFrame < 5) && sleepPlan > (int) avlFrame * 3)
        {
            sleepPlan -= avlFrame * 3;
        }
        else if (avlFrame >= 5 && sleepPlan > (int) avlFrame * 4)
        {
            sleepPlan -= avlFrame * 4;
        }
        else
        {
            sleepPlan = 0;
        }

        llt = Common_GetSystemCount64();
        if (llt - smooth->difftime + sleepPlan> intervalTimeMs - 10)
        {
            sleepPlan = sleepPlan - (llt - smooth->difftime + sleepPlan - intervalTimeMs + 10);
            if (sleepPlan < 0)
                sleepPlan = 0;
        }

        if (sleepPlan > 0 && sleepPlan < 1000)
        {
            struct timeval tmp = { 0, sleepPlan * 1000 };
            select(0, NULL, NULL, NULL, &tmp);
            llt = Common_GetSystemCount64();
        }

        if (debugPrint > 0)
        {
            gettimeofday(&smooth->timeP1, NULL);
//        if (llt - smooth->difftime > 50 || llt - smooth->difftime < 30)
            printf("pframe %u diff %llu sleep time %d/%d restCnt %d frame type %02x frame no %u frame size %u\n",
                            (header->dwTimeStamp - smooth->lastTimeStamp) / 90,
                            llt - smooth->difftime, sleepPlan,
                            DIFF_TIMEVAL(smooth->timeP0, smooth->timeP1),
                            smooth->restCnt, header->uiFrameType,
                            header->uiFrameNo, header->uiFrameLen);
        }
        smooth->lastTimeStamp = header->dwTimeStamp;
        smooth->difftime = llt;
        smooth->lastDataSize = header->uiFrameLen;
    }
}

static int RtspReadDataCallback(int rtspHandle, int streamHandle, void *streamUser, int nStreamType,
                char **data, int *size, void *streamInfo, int *readType, int *adpcm2G711u)
{

    int ret = -1;
    RTSP_CONNECT_INFO_NODE_T * node = (RTSP_CONNECT_INFO_NODE_T *)streamUser;
    rtsp_stream_info * rsi = streamInfo;
    if (node == NULL ||data == NULL || size == NULL
        || nStreamType !=  ANTS_RTSPSERVER_SOURCETYPE_LIVE || rsi == NULL)
    {
//        LOGD("param error streamtype %d\n",nStreamType);
        return -1;
    }

    if (node->rtspMgr->state != MEDIA_RTSP_STATE_START)
    {
        LOGE("state error\n");
        return -1;
    }

    if (readType)
    {
        *readType = 1;
    }

    if (adpcm2G711u)
    {
        *adpcm2G711u = 0;
    }

    if (node->ctype == 106)
    {
        Utils_Sleep(10);
        return -1;
    }

    if (node->reqInfo.type != MEDIA_STREAM_TYPE_REAL)
        return -1;

    Ovfs_FrameHeader_T *h = NULL;
        do
        {
            int sidx = -1;
        ret = RestMedia_RequestStreamRead(node->streamQueueHandle, -1, &sidx, (void **) data, (int *) size,&node->smooth.restCnt, 400);
            if (ret != 0)
            {
                h = NULL;
                break;
            }

        h = (Ovfs_FrameHeader_T *) (*data);
            node->frameHeader = h;
            if (sidx % MEDIA_STREAM_QUEUE_ID_MAX >= MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN && sidx % MEDIA_STREAM_QUEUE_ID_MAX<= MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD)
                rsi->byStream = (unsigned char) sidx % MEDIA_STREAM_QUEUE_ID_MAX;
            else
                rsi->byStream = 0;

            rsi->wCh = sidx/MEDIA_STREAM_QUEUE_ID_MAX;
        //        LOGE("rsi ch %d stream %d\n",rsi->wCh,rsi->byStream);

            if (node->liveCombo == 1 &&
                (h->uiFrameType == Ovfs_FrameType_AudioFrames || h->uiFrameType == Ovfs_FrameType_SpeakFrames) &&
                node->rtspMgr->boardState.audioState == 0)
            {
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                continue;
            }

            if (node->liveCombo == 0 &&
                (h->uiFrameType == Ovfs_FrameType_SmartIFrames ||
                 h->uiFrameType == Ovfs_FrameType_SmartPFrames ||
                 h->uiFrameType == Ovfs_FrameType_SpeakFrames))
            {
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                continue;
            }
        //        if (h->uiFrameType == 8)
        //            LOGE("Get a FRAME type %u No. %u size %u Queue RestCnt %d\n",
        //                            h->uiFrameType,h->uiFrameNo,h->uiFrameLen,node->smooth.restCnt);

        //        RtspDiPinXian_PrintTs(&node->dpxCt,node);

            if (RtspFrameFilter(node) < 0)
            {
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                continue;
            }

            if (node->totalReadSize != 0)
                break;

            if (node->mediaType != 0 && (node->mediaType & 0x1) == 0)
                break;

            if (h->uiFrameType == Ovfs_FrameType_SmartPFrames ||
                h->uiFrameType == Ovfs_FrameType_SmartIFrames ||
                h->uiFrameType == Ovfs_FrameType_AudioFrames ||
                h->uiFrameType == Ovfs_FrameType_SpeakFrames)
            {
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                continue;
            }

            if (h->uiFrameType != Ovfs_FrameType_IFrames && h->uiFrameType != Ovfs_FrameType_SubIFrames
                                            && h->uiFrameType != Ovfs_FrameType_ThirdIFrames)
            {
                LOGD("skip one frame %d no %u, wait for I frame! isPlaying[%d] reqIFrame[%d]\n",
                    h->uiFrameType,h->uiFrameNo,node->isPlaying, node->reqIFrame);
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                if (node->isPlaying && node->reqIFrame == 0)
                {
                    LOGW("request i frame\n");
                    RestMedia_RequestVideoIFrame(&node->reqInfo);
                    node->reqIFrame = 1;
                }
                continue;

            }
            else
            {
                LOGI("video codec id is %d [%d]\n",h->uMedia.struVideoHeader.cCodecId, h->uiFrameType);
                node->videoCodecId = h->uMedia.struVideoHeader.cCodecId;
                node->videoW = h->uMedia.struVideoHeader.usWidth;
                node->videoH = h->uMedia.struVideoHeader.usHeight;
                break;
            }

#if (defined PLATFORM_HI3516EV200)
            if (node->preferh264 == 1 && h->uMedia.struVideoHeader.cCodecId != Ovfsmid_VideoCodecID_H264_hisi_RTP)
            {
                /*request boardsys change video codec type to h264*/
                if (node->reqChangeCodec % 10 == 0)
                {
                    RestMedia_RequestBoardCodecTempChange(1,&node->reqInfo);
                }
                node->reqChangeCodec++;
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                continue;
            }
#endif

        } while (1);

        if (ret == 0)
        {
            node->totalReadSize += (long long unsigned int) (*size);
            node->connectTime++;

            if (node->liveCombo == 0 && (h->uiFrameType == Ovfs_FrameType_IFrames
                            || h->uiFrameType == Ovfs_FrameType_SubIFrames
                            || h->uiFrameType == Ovfs_FrameType_ThirdIFrames))
            {
                if (node->videoCodecId != 0 && node->videoCodecId != h->uMedia.struVideoHeader.cCodecId)
                {
                    LOGW("codec id changed, %d %d\n",node->videoCodecId,h->uMedia.struVideoHeader.cCodecId);
                    RestMedia_RequestStreamRelease(node->streamQueueHandle);
                    ret = -1;
                    ret = Ants_RTSPServerV2_CloseStream(node->rtspHandle,node->sessionHandle);
                    LOGW("close stream ret %d\n",ret);
                    return -1;
                }
            }

            if (node->liveCombo == 0)
            {
                RtspSmoothingDataHandle(&node->smooth, node->frameHeader, node->rtspMgr->debugPrint);
            }
            }

        if (ret < 0 && node->liveCombo != 0)
        {
            Utils_Sleep(10);
        }
    return (ret < 0)?-1:0;
}

static int RtspReleaseDataCallback(int rtspHandle, int streamHandle, void *streamUser, int streamType)
{
    RTSP_CONNECT_INFO_NODE_T * node = (RTSP_CONNECT_INFO_NODE_T *) streamUser;

    if (streamType != ANTS_RTSPSERVER_SOURCETYPE_LIVE)
        return -1;

    if (node == NULL)
    {
        LOGE("streamUser was NULL\n");
        return -1;
    }
    if (node && (node->ctype == 106 || node->reqInfo.type == MEDIA_STREAM_TYPE_TALKING))
        return 0;

    if (node->liveCombo == 0 && (node->frameHeader->uiFrameType == Ovfs_FrameType_IFrames
                  || node->frameHeader->uiFrameType == Ovfs_FrameType_PFrames ))
    {
        node->smooth.spendTime = Common_GetSystemCount64() - node->smooth.difftime;
        if (node->smooth.spendTime > 20)
            node->smooth.spendTime = 0;
    }

    RestMedia_RequestStreamRelease(node->streamQueueHandle);
    node->connectTime--;
//    LOGW("release done %d handle %d\n",ret,node->streamQueueHandle);
    return 0;
}

static int RtspCloseCallback(int rtspHandle, int streamHandle, void *streamUser, int streamType, char *url)
{
    RTSP_CONNECT_INFO_NODE_T * node = (RTSP_CONNECT_INFO_NODE_T *) streamUser;
    prctl(PR_SET_NAME,__func__);
    int ret = 0;
    COMMON_DLIST_T *connList = NULL;
    LOGI("stream handle %d url %s streamtype %d streamHandle %d\n", streamHandle,url,streamType,node->streamHandle);

    if (node->reqInfo.type == MEDIA_STREAM_TYPE_TALKING || node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
    {
        struct timespec tv;
        node->timeStamp = 0;

        clock_gettime(CLOCK_MONOTONIC, &tv);
        tv.tv_sec = tv.tv_sec + 2;
        pthread_mutex_lock(&node->mutex);
        node->isExit = 1;
        ret = pthread_cond_timedwait(&node->cond,&node->mutex,&tv);
        pthread_mutex_unlock(&node->mutex);
        if (ret != 0)
        {
            LOGW("input thread function could not exit normally, %d\n",ret);
        }

        RestMedia_RequestStreamClose(node->streamQueueHandle,&node->reqInfo);
        if (node->reqInfo.type == MEDIA_STREAM_TYPE_TALKING)
        {
            struct in_addr in;
            in.s_addr = node->connectIp;
            //RestMedia_WriteEventLog(MAJOR_OPERATION,MINOR_STOP_VT,node->reqInfo.chan + 1,inet_ntoa(in));
        }
        if (node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
        {
            node->rtspMgr->streamCount--;
            LOGE("current stream count %d max %d\n",node->rtspMgr->streamCount,
                 node->rtspMgr->cfg.streamMaxNum);
        }
		pthread_cond_destroy(&node->cond);
		pthread_mutex_destroy(&node->mutex);

    }
    else if (node->reqInfo.type == MEDIA_STREAM_TYPE_REAL)
    {
/* #ifdef STREAM_CNT_LIMIT */
        if (node->ctype != 106)
            node->rtspMgr->streamCount--;
        LOGE("current stream count %d max %d\n",node->rtspMgr->streamCount,
             node->rtspMgr->cfg.streamMaxNum);
/* #endif */
        RestMedia_RequestStreamClose(node->streamQueueHandle,&node->reqInfo);
        }

#if (defined PLATFORM_HI3516EV200)
    if (node->preferh264 == 1)
    {
        RestMedia_RequestBoardCodecTempChange(0, &node->reqInfo);
    }
#endif

    RestMedia_UnregistAuthNode(node->sessionId);

    LOGW("conn list count %d read_release num %lld\n",Common_DList_GetCount(node->rtspMgr->conList),node->connectTime);

    connList = node->rtspMgr->conList;
    RtspDiPinXian_Close(&node->dpxCt);
	if(node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
	{
		if(node)
		{
			MEDIA_FREE(node);
		}
	}
	else
	{
		Common_DList_Delete(connList,node,ConnListAppCompare);
	}
    LOGD("\n");

    return 0;
}

static void RtspTimerCheckThread(void *data)
{
    RTSP_MGR_T *ct = data;
    int ret = -1;
    int alarm_send_count = 0;
    while (ct != NULL && ct->state == MEDIA_RTSP_STATE_START)
    {
        MEDIA_ALARM_STATUS_T alarmStatusInfo = { 0 };
        long long int curTime = Utils_GetMs();
        unsigned int sessionId = 0;
        int tryCnt = 5;

        if(alarm_send_count >= 3) //send interval for 3s
        {
            alarm_send_count = 0;
            Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_GET_APP_DATA, NULL,0,&ret,(void *)(&alarmStatusInfo), sizeof(MEDIA_ALARM_STATUS_T));

            if (alarmStatusInfo.alarmStatusCnt > 0)
                RtspMgr_SendAppData(&alarmStatusInfo,0);

            if(alarmStatusInfo.alarmStatus)
                MEDIA_FREE(alarmStatusInfo.alarmStatus);
        }
        else
        {
            alarm_send_count++;
        }

        do
        {
            RTSP_SESSION_TABLE_T * sessionHandleNode = Common_DList_Search(ct->connStateList, &curTime,
                            ConnStateListCheckAndDelete);
            if (sessionHandleNode)
            {
                sessionId = sessionHandleNode->sessionId;
                RestMedia_UnregistAuthNode(sessionId);
                Common_DList_Delete(ct->connStateList, &sessionId, ConnStateListCompare);
                LOGW("remove session id %d\n",sessionId);
            }
            else
            {
                sessionId = 0;
            }
            tryCnt--;
        } while (sessionId != 0 && tryCnt > 0);
        Utils_Sleep(1000);
    }
}

int RtspMgr_Init(MQ_HANDLE_H mqHandle)
{
    int ret = -1;
    LOGD("\n");
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(errno));
        return -EINVAL;
    }

    memset(&s_rtspMgr,0,sizeof(RTSP_MGR_T));
    s_rtspMgr.mqHandle = mqHandle;

    if ((ret = Ants_RTSPServerV2_Init()) == 0)
    {
        if ((ret = Ants_RTSPServerV2_SetStatusCallBack(RtspStatusCallback, (void *)&s_rtspMgr)) == 0)
        {
            s_rtspMgr.state = MEDIA_RTSP_STATE_STOP;
        }
    }

    if (ret < 0)
    {
        LOGE("rtsp server init failed , ret %d\n", ret);
    }
    LOGD("\n");
    return ret;
}

int RtspMgr_Start()
{
    int ret = 0;

    LOGD("\n");

    if (s_rtspMgr.state == MEDIA_RTSP_STATE_UNINIT)
    {
        LOGE("rtsp uninit\n");
        return -EINVAL;
    }

    if (s_rtspMgr.state == MEDIA_RTSP_STATE_START)
    {
        LOGE("rtsp mgr status error %d\n",s_rtspMgr.state);
        return -1;
    }
    memset(&s_rtspMgr.cfg,0,sizeof(s_rtspMgr.cfg));
    if (Mq_Request(s_rtspMgr.mqHandle,MEDIA_REQ_RTSP_GET_CFG,NULL,0,&ret,&s_rtspMgr.cfg,sizeof(RTSP_CONFIG_T)) || ret < 0)
    {
        LOGE("get cfg failed\n");
        return -1;
    }

    if (Mq_Request(s_rtspMgr.mqHandle,MEDIA_REQ_GET_BOARD_STATE,NULL,0, &ret, &s_rtspMgr.boardState,sizeof(s_rtspMgr.boardState)) || ret < 0)
    {
        LOGE("get board state failed\n");
        return -1;
    }
    if (s_rtspMgr.cfg.enable == 0)
    {
        LOGW("rtsp server was disabled\n");
        return 0;
    }

    /*TODO check configure file*/

    ret = Ants_RTSPServerV2_Start(s_rtspMgr.cfg.rtspPort);
    if (ret < 0)
    {
        LOGE("Ants_RTSPServerV2_Start failed %d\n",ret);
        return ret;
    }

    s_rtspMgr.rtspHandle = ret;
    RTSPServer_StreamControl sc;
    sc.fxnOpen = RtspOpenCallback;
    sc.fxnRead = RtspReadDataCallback;
    sc.fxnControl = RtspControlCallback;
    sc.fxnClose = RtspCloseCallback;
    sc.fxnRelease = RtspReleaseDataCallback;

    ret = Ants_RTSPServerV2_SetStreamControlCallBack(s_rtspMgr.rtspHandle, &sc, (void *)&s_rtspMgr);
    if (ret < 0)
    {
        LOGE("rtsp server RTSPServerV2_SetStreamControlCallBack failed %d\n",ret);
        Ants_RTSPServerV2_Stop(s_rtspMgr.rtspHandle);
        return ret;
    }

    /*ret = Ants_RTSPServerV2_SetTunnelingOverHTTP(s_rtspMgr.rtspHandle, s_rtspMgr.cfg.httpPort);
    if (ret < 0)
    {
        LOGE("rtsp server RTSPServerV2_SetTunnelingOverHTTP failed %d rtspHandle %d\n",ret,s_rtspMgr.rtspHandle);
        Ants_RTSPServerV2_Stop(s_rtspMgr.rtspHandle);
        return ret;
    }*/

    s_rtspMgr.tp = thread_pool_create(24, 0, PTHREAD_STACK_MIN * 64);
    Common_DList_Init(&s_rtspMgr.conList, RtspConnectNodeFree);
    Common_DList_Init(&s_rtspMgr.connStateList, RtspConnectStateNodeFree);
    s_rtspMgr.state = MEDIA_RTSP_STATE_START;
    pthread_pool_add(s_rtspMgr.tp, RtspTimerCheckThread, &s_rtspMgr);

    LOGD("\n");
    return 0;
}

int RtspMgr_Stop()
{
    int ret = 0;
//    RTSP_CONNECT_INFO_NODE_T *node = NULL;

    LOGD("\n");

    if (s_rtspMgr.state == MEDIA_RTSP_STATE_UNINIT)
    {
        LOGE("rtsp uninit\n");
        return -EINVAL;
    }

    if (s_rtspMgr.state == MEDIA_RTSP_STATE_STOP)
    {
        LOGE("rtsp mgr already stop\n");
        return -1;
    }

    s_rtspMgr.state = MEDIA_RTSP_STATE_STOP;
    ret = Ants_RTSPServerV2_Stop(s_rtspMgr.rtspHandle);

    if (ret < 0)
    {
        LOGE("stop rtsp failed %d\n",ret);
        s_rtspMgr.state = MEDIA_RTSP_STATE_START;
        return ret;
    }

    /* s_rtspMgr.state = MEDIA_RTSP_STATE_STOP; */
    pthread_pool_destroy(s_rtspMgr.tp);
//    cnt = Common_DList_GetCount(s_rtspMgr.conList);
//    for (i = 0;i < cnt;i++)
//    {
//        node = Common_DList_GetNode(s_rtspMgr.conList,i);
//        if (node)
//        {
//            RestMedia_RequestStreamClose(node->streamQueueHandle);
//            RestMedia_UnregistAuthNode(node->sessionId);
//        }
//    }
    Common_DList_Uninit(&s_rtspMgr.conList);
    Common_DList_Uninit(&s_rtspMgr.connStateList);
    s_rtspMgr.tp = NULL;
    s_rtspMgr.conList = NULL;
    s_rtspMgr.connStateList = NULL;
    s_rtspMgr.streamCount = 0;

    memset(&s_rtspMgr.cfg,0,sizeof(s_rtspMgr.cfg));
    LOGD("\n");
    return 0;

}

int RtspMgr_Restart()
{
    LOGD("\n");
    int ret = -1;
    RTSP_CONFIG_T cfg;
    memset(&cfg, 0, sizeof(RTSP_CONFIG_T));

    if (s_rtspMgr.state == MEDIA_RTSP_STATE_UNINIT)
    {
        LOGE("rtsp server not init\n");
        return -1;
    }

    if (Mq_Request(s_rtspMgr.mqHandle, MEDIA_REQ_RTSP_GET_CFG, NULL, 0, &ret, &cfg, sizeof(RTSP_CONFIG_T)) < 0 || ret < 0)
    {
        LOGE("load rtsp config failed\n");
        return -1;
    }

    if(cfg.enable == s_rtspMgr.cfg.enable && cfg.httpPort == s_rtspMgr.cfg.httpPort && cfg.rtspPort == s_rtspMgr.cfg.rtspPort)
    {
        LOGD("no need restart rtsp server , only update cfg file\n");
        memcpy(&s_rtspMgr.cfg,&cfg,sizeof(RTSP_CONFIG_T));
        return 0;
    }
    if (cfg.enable == 0 && s_rtspMgr.state == MEDIA_RTSP_STATE_START)
    {
        RtspMgr_Stop();
    }

    if (cfg.enable == 1 && s_rtspMgr.state == MEDIA_RTSP_STATE_STOP)
    {
        RtspMgr_Start();
    }

    if (cfg.enable == 1 && s_rtspMgr.state == MEDIA_RTSP_STATE_START)
    {
        if (memcmp(&cfg, &s_rtspMgr.cfg, sizeof(RTSP_CONFIG_T)) != 0)
        {
            RtspMgr_Stop();
            RtspMgr_Start();
        }
    }

    LOGD("\n");
    return 0;
}

int RtspMgr_SendAppData(MEDIA_ALARM_STATUS_T * alarmStatusInfo, int sendAll)
{
    int i = 0, j = 0;
    MEDIA_ALARM_STATUS_NODE_T *node = NULL;
    if (alarmStatusInfo == NULL || alarmStatusInfo->alarmStatusCnt <= 0)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    if (s_rtspMgr.state != MEDIA_RTSP_STATE_START)
    {
        LOGE("rtsp server not start\n");
        return -1;
    }

    for (i = 0; i < alarmStatusInfo->alarmStatusCnt; i++)
    {
        node = alarmStatusInfo->alarmStatus + i;
        if (node == NULL)
        {
            LOGE("node offset failed\n");
            break;
        }

        if (node->status == 0)
        {
            if (node->stopTime.tm_year == 0)
                continue;

            if (mktime(&node->stopTime) > time(NULL) || time(NULL) - mktime(&node->stopTime) - Common_GetTimeDiff() >= RTSP_APP_DATA_DELAY)
            {
                continue;
            }
        }
        else
        {
            if(sendAll == 0)
            {
                //int cur_time = time(NULL);
                //int start_time = mktime(&node->startTime);
                //int time_diff = Common_GetTimeDiff();
                //int inter = cur_time - start_time - time_diff;
                //LOGW("[%d][%d][%d][%d]\n",cur_time,start_time,time_diff,inter);
                if (abs(time(NULL) - mktime(&node->startTime) - Common_GetTimeDiff()) < 3)
                {
                    continue;
                }

            }
        }

        if (strcmp(node->alarmName,"Vdiagnose") == 0)
        {
            for (j = 0;j< COMMON_ARRAY_DIM(s_rtspVdTable);j++)
            {
                if (s_rtspVdTable[j].vdType == node->regionId)
                {
//                    LOGW("match %s region id %d vdalarmidx %d vd %d %d\n",
//                                    node->alarmName,node->regionId,
//                                    s_rtspVdTable[j].vdAlarmIdx,s_rtspVdTable[j].vdAlarmType,j);
                    RtspSendApp106(s_rtspMgr.conList, node , s_rtspVdTable[j].vdAlarmIdx);
                }
            }
        }
        else
            RtspSendApp106(s_rtspMgr.conList, node, -1);
    }

    return alarmStatusInfo->alarmStatusCnt;
}

static int ConnListCloseStream(void * a, void * b)
{
    RTSP_CONNECT_INFO_NODE_T *node = a;
    if (node->liveCombo == 0 && node->reqInfo.type == MEDIA_STREAM_TYPE_REAL)
    {
        LOGW("close stream session handle %d\n",node->sessionHandle);
        Ants_RTSPServerV2_CloseStream(node->rtspHandle,node->sessionHandle);
    }
    return -1;
}

int RtspMgr_UpdateBoardState(MEDIA_BOARD_STATE_T boardState)
{
    if (s_rtspMgr.state != MEDIA_RTSP_STATE_START)
    {
        return -1;
    }

    if (memcmp(&s_rtspMgr.boardState,&boardState,sizeof(MEDIA_BOARD_STATE_T)) != 0)
    {
        memcpy(&s_rtspMgr.boardState,&boardState,sizeof(MEDIA_BOARD_STATE_T));
        Common_DList_Search(s_rtspMgr.conList,&boardState,ConnListCloseStream);
    }
    return 0;
}

int RtspMgr_GetDebugPrint()
{
    return s_rtspMgr.debugPrint;
}

int RtspMgr_SetDebugPrint(int enable)
{
    s_rtspMgr.debugPrint = enable;
    return 0;
}
