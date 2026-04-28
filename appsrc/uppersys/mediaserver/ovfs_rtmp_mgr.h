/**
 * @file ovfs_rtmp_mgr.h
 * @date create on: 2016年10月11日
 * @author eric
 * @brief
 *
 * @defgroup ovfs_rtmp_mgr ovfs_rtmp_mgr
 * @{
 *  @note
 *
 */
#ifndef OVFS_RTMP_MGR_H_
#define OVFS_RTMP_MGR_H_

#include <libcommon_api.h>
#include <cjson.h>

typedef struct
{
    int enable;
    int rtmpPort;
} RTMP_CONFIG_T;

#define RTMPPUSH_SESSION_MAX_NUM 4
#define RTMPPUSH_CHANNEL_MAX_NUM 8
#define RTMPPUSH_STREAM_MAX_NUM 8

#define MAX_TIMESEGMENT		    8       //设备最大时间段数
#define MAX_DAYS				7       //每?

typedef struct
{
    int bEnable;
    char szUrl[512];
	int bEnableAudio;
} RTMPPUSH_SESSION_T;

typedef struct
{
    RTMPPUSH_SESSION_T tSession[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_CHANNEL_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM];
} RTMPPUSH_CONFIG_T;
/*
typedef struct
{
    unsigned int	startTime;  //min
    unsigned int	stopTime;   //min
}OVFS_TIME_RANGED_T;
*/
typedef struct
{
    OVFS_TIME_RANGED_T NotDisturbTime[MAX_DAYS][MAX_TIMESEGMENT];
    int Client[RTMPPUSH_SESSION_MAX_NUM];
} RTMPPUSH_NOTDISTURB_CONFIG_T;


int RtmpMgr_Init(MQ_HANDLE_H mqHandle);
int RtmpMgr_Start();
int RtmpMgr_Stop();
int RtmpMgr_Restart();
int RtmpPushMgr_Init(MQ_HANDLE_H mqHandle);

int RtmpPushMgr_Restart();


#endif /* OVFS_RTMP_MGR_H_ */
/**
 * @}
 */
