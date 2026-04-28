#ifndef _CFG_MANAGE_API_H_
#define _CFG_MANAGE_API_H_

#include <sys/time.h>
#include <libcommon_api.h>
#include <libmodule_api.h>
#include "record_common.h"

using namespace cv_soft;

#define MAX_VI_DEV 2

typedef struct
{
    S32		PreRecord;
}RECORD_FUNCTION, *LPRECORD_FUNCTION;

typedef struct
{
    S32     SDCapacity;
    S8      SDRecordableDays[16];
}RECORD_CUSTOM, *LPRECORD_CUSTOM;

typedef struct
{
    U32		PreserveMode;
    U32		PreserveTime;
	U32		PreservePercent;
    U32		PreserveVolume;
}PRESERVERECORDRULE, *LPRESERVERECORDRULE;

typedef struct
{
    U32		PreRecordMode;			//0:不启用时间限制	1:启用时间限制;注内存限制始终启用
    U32		PreRecordTime;
    U32		PreRecordMem;
    U32		DelayRecordTime;
}PRERECORDRULE, *LPRERECORDRULE;

//设备能力描述
typedef struct
{
    U32	dev_type;				//设备类型

    U32	video_ch_num;			//支持的视频通道数目
    U32	video_stream_num[CV_MAX_LOCAL_CH_NUM];		//每个通道支持的码流个数
    U32	video_ch_videv[CV_MAX_LOCAL_CH_NUM];		//每个通道所在的videv
    U32	video_ch_vichn[CV_MAX_LOCAL_CH_NUM];		//每个通道所在的vichn
    U32	audio_ch_num;			//支持的音频通道个数
    U32 	local_playback_window;	//支持本地回放最大窗口数目
    U32	pb_ch_num;				//支持的回放通道数目

    U32	video_vi_num;
    U32	video_vi_chn[MAX_VI_DEV];

    U32	alarm_in_num;			//报警输入数目(video_ch_num + 本地AI数目)
    //U32	alarm_out_num;			//报警输出数目

    S8	product_disk_locationmap_path[64];	//磁盘位置图路径
    //U32 	is_support_local_snap;	//是否支持本地抓图
}cv_device_capability;

void cv_cfgm_init(void* Handle);
CV_ERR cv_cfgm_saveconfig();
CV_ERR cv_cfgm_initconfig();
CV_ERR cv_cfgm_initdevcap();
CV_ERR cv_cfgm_get_prerecord(U32 ch, PRERECORDRULE *pPrerecord);
CV_ERR cv_cfgm_set_prerecord(U32 ch, PRERECORDRULE Prerecord);
CV_ERR cv_cfgm_get_plan_day(U32 ch, U32 day, cv_record_arming_day *pPlan);
CV_ERR cv_cfgm_set_plan_day(U32 ch, U32 day, cv_record_arming_day Plan);
cv_device_capability* cv_cfgm_get_dev_cap();
CV_ERR cv_cfgm_get_record_mode(U32 ch, U32 *mode);
CV_ERR cv_cfgm_set_record_mode(U32 ch, U32 mode);
CV_ERR cv_cfgm_get_manual_mask(U32 ch, U32 *mask);
CV_ERR cv_cfgm_set_manual_mask(U32 ch, U32 mask);
CV_ERR cv_cfgm_get_local_rec_plan(U32 ch, cv_local_rec_plan *plan);
CV_ERR cv_cfgm_get_pre_rec_mode(U32 ch, U32 *pMode);
CV_ERR cv_cfgm_get_pre_rec_sec(U32 ch, U32 *pSec);
CV_ERR cv_cfgm_get_pre_rec_mem(U32 ch, U32 *pMem);
CV_ERR cv_cfgm_get_delay_rec_sec(U32 ch, U32 *pSec);
U32 cv_cfgm_get_streamhandle(U32 ch, U32 streamtype);
S32 cv_cfgm_ReadData(S32 fd, cJSON_Struct **pPrivInfo, void **pData, S32 *lpSize, S32 nMSecTimeout);
S32 cv_cfgm_ReleaseData(S32 fd);
void cv_cfgm_RequestVideoIFrame(int streamtype);
S32 cv_cfgm_get_channel(U32 videv, U32 vichn);
CV_ERR cv_cfgm_change_event(cv_event_id *event, U32 alarmtype, U32 alarmsrc);
CV_ERR cv_cfgm_get_recplan(U32 ch, U32 *pRecordMode);
CV_ERR cv_cfgm_get_diskconfig(S32 *pRecycle, PRESERVERECORDRULE *pPreserve);
CV_ERR cv_cfgm_set_diskconfig(S32 Recycle, PRESERVERECORDRULE Preserve);
CV_ERR cv_cfgm_get_record_func(RECORD_FUNCTION *sRecFunc);
S32 cv_cfgm_get_historystream_maxnum();
CV_ERR cv_cfgm_get_record_custom(RECORD_CUSTOM *sRecCustom);
CV_ERR cv_cfgm_change_record_encode(int nRecordStatue);
CV_ERR cv_cfgm_play_audio(char *pFileName);

#endif	//#ifndef _COMM_SYS_H_

