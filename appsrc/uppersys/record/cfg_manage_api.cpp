#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include "libcommon_api.h"
#include "cjson.h"
#include "ipcstorage.h"
#include "cfg_manage_api.h"

using namespace cv_soft;

static ModuleHandle_T sStreamModeHandle = NULL;

static S32 s_RecordConfigInit = -1;
static RECORD_FUNCTION s_RecordFunc = {1};
static RECORD_CUSTOM s_RecordCustom = {0};
static S32 s_RecycleRecord = 0;
static PRESERVERECORDRULE	s_PreserverRecord = {0};
static S32 s_RecordMode[CV_MAX_LOCAL_CH_NUM] = {0};
static S32 s_ManualStreamMask[CV_MAX_LOCAL_CH_NUM] = {0};
static PRERECORDRULE		s_PreRecord[CV_MAX_LOCAL_CH_NUM] = {0};
static cv_record_arming_day	s_RecordPlan[CV_MAX_LOCAL_CH_NUM][7] = {0};

static S32 s_RecordCapInit = -1;

static cv_device_capability s_SysCap = {0};

static S32 s_HistoryStreamMaxNum = 0;

static int cv_cfgm_FillRecordPlan(cJSON_Struct* opsNode)
{
	LOGD("cv_cfgm_FillRecordPlan\n");

	if(opsNode)
	{
		Common_Json_GetAttrValue(opsNode, -1, "RecycleRecord", NULL, NULL, &s_RecycleRecord, NULL);

		Common_Json_GetAttrValue(opsNode, -1, "PreserveMode", NULL, NULL, (S32*)&s_PreserverRecord.PreserveMode, NULL);
		if(s_PreserverRecord.PreserveMode !=1 && s_PreserverRecord.PreserveMode != 2 && s_PreserverRecord.PreserveMode != 4)
			s_PreserverRecord.PreserveMode = 0;

		Common_Json_GetAttrValue(opsNode, -1, "PreserveTime", NULL, NULL, (S32*)&s_PreserverRecord.PreserveTime, NULL);
		s_PreserverRecord.PreserveTime = s_PreserverRecord.PreserveTime < 1 ? 1 : s_PreserverRecord.PreserveTime;

		Common_Json_GetAttrValue(opsNode, -1, "PreserveVolume", NULL, NULL, (S32*)&s_PreserverRecord.PreserveVolume, NULL);
		s_PreserverRecord.PreserveVolume = s_PreserverRecord.PreserveVolume < 1 ? 1 : s_PreserverRecord.PreserveVolume;

		Common_Json_GetAttrValue(opsNode, -1, "PreservePercent", NULL, NULL, (S32*)&s_PreserverRecord.PreservePercent, NULL);
		s_PreserverRecord.PreservePercent = s_PreserverRecord.PreservePercent > 100 ? 100 : (s_PreserverRecord.PreservePercent < 1 ? 1 : s_PreserverRecord.PreservePercent);

		Common_Json_GetAttrValue(opsNode, -1, "HistoryStreamMaxNum", NULL, NULL, (S32*)&s_HistoryStreamMaxNum, NULL);
		s_HistoryStreamMaxNum = s_HistoryStreamMaxNum < 0 ? 0 : s_HistoryStreamMaxNum;

		for(U32 i = 0; i < s_SysCap.video_ch_num; i ++)
		{
			S8 str[128];

			sprintf(str, "Channel%d/RecordMode", i);
			Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, &s_RecordMode[i], NULL);
			s_RecordMode[i] = s_RecordMode[i] < 0 ? 0 : (s_RecordMode[i] > 2 ? 2 : s_RecordMode[i]);

			sprintf(str, "Channel%d/ManualStreamMask", i);
			Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, &s_ManualStreamMask[i], NULL);
			s_ManualStreamMask[i] = s_ManualStreamMask[i] < 0 ? 0 : (s_ManualStreamMask[i] > 0x0f ? 0x0f : s_ManualStreamMask[i]);

			sprintf(str, "Channel%d/PreRecordMode", i);
			Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&s_PreRecord[i].PreRecordMode, NULL);
			s_PreRecord[i].PreRecordMode = (s_PreRecord[i].PreRecordMode < 0||s_PreRecord[i].PreRecordMode > 1) ? 0 : s_PreRecord[i].PreRecordMode;

			sprintf(str, "Channel%d/PreRecordTime", i);
			Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&s_PreRecord[i].PreRecordTime, NULL);
			s_PreRecord[i].PreRecordTime = s_PreRecord[i].PreRecordTime < 0 ? 0 : s_PreRecord[i].PreRecordTime;

			sprintf(str, "Channel%d/PreRecordSpace", i);
			Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&s_PreRecord[i].PreRecordMem, NULL);
			s_PreRecord[i].PreRecordMem = s_PreRecord[i].PreRecordMem < 0 ? 0 : s_PreRecord[i].PreRecordMem;
			if((s_PreRecord[i].PreRecordMem <= 0) ||(s_PreRecord[i].PreRecordMem >= (5<<20)))
				s_PreRecord[i].PreRecordMem = (5<<20);

			sprintf(str, "Channel%d/PreRecordDelayTime", i);
			Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&s_PreRecord[i].DelayRecordTime, NULL);
			s_PreRecord[i].DelayRecordTime = s_PreRecord[i].DelayRecordTime < 0 ? 0 : s_PreRecord[i].DelayRecordTime;
#ifdef JZT31N
			//报警录像&&延迟时间小于120秒,设置为120秒
			if(s_PreRecord[i].DelayRecordTime < 120)
			{
				s_PreRecord[i].DelayRecordTime = 120;
			}
#endif

			for(U32 j = 0; j < 7; j ++)
			{
				sprintf(str, "Channel%d/Weekday%d/AlldayEnable", i, j);
				Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&s_RecordPlan[i][j].bAlldayRec, NULL);
				for(U32 k = 0; k < CV_MAX_ARMING_TIME_QUANTUM; k ++)
				{
					U32	timeval;
					sprintf(str, "Channel%d/Weekday%d/Sched%d/StartTime", i, j, k);
					timeval = s_RecordPlan[i][j].arming[k].time.start_hour_min;
					Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&timeval, NULL);
					if((timeval / 100) > 23)
					{
						timeval = (timeval % 100) + 2300;
					}
					if((timeval % 100) > 59)
					{
						timeval = (timeval / 100) * 100 + 59;
					}
					s_RecordPlan[i][j].arming[k].time.start_hour_min = timeval;

					sprintf(str, "Channel%d/Weekday%d/Sched%d/StopTime", i, j, k);
					timeval = s_RecordPlan[i][j].arming[k].time.stop_hour_min;
					Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, (S32*)&timeval, NULL);
					if((timeval / 100) > 23)
					{
						timeval = (timeval % 100) + 2300;
					}
					if((timeval % 100) > 59)
					{
						timeval = (timeval / 100) * 100 + 59;
					}
					s_RecordPlan[i][j].arming[k].time.stop_hour_min = timeval;
					#if 0
					if(s_RecordPlan[i][j].arming[k].time.stop_hour_min <= s_RecordPlan[i][j].arming[k].time.start_hour_min)
					{
						s_RecordPlan[i][j].arming[k].time.stop_hour_min = s_RecordPlan[i][j].arming[k].time.start_hour_min;
						if(s_RecordPlan[i][j].arming[k].time.start_hour_min == 2359)
							s_RecordPlan[i][j].arming[k].time.start_hour_min -= 1;
						else
							s_RecordPlan[i][j].arming[k].time.stop_hour_min += 1;
					}
					#endif

					sprintf(str, "Channel%d/Weekday%d/Sched%d/SchedMode", i, j, k);
					Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, &s_RecordPlan[i][j].arming[k].arming_type, NULL);
					s_RecordPlan[i][j].arming[k].arming_type = s_RecordPlan[i][j].arming[k].arming_type < 1 ? 1 : (s_RecordPlan[i][j].arming[k].arming_type > 3 ? 3 : s_RecordPlan[i][j].arming[k].arming_type);

					sprintf(str, "Channel%d/Weekday%d/Sched%d/StreamMask", i, j, k);
					Common_Json_GetAttrValue(opsNode, -1, str, NULL, NULL, &s_RecordPlan[i][j].arming[k].recordmask, NULL);
					s_RecordPlan[i][j].arming[k].recordmask = s_RecordPlan[i][j].arming[k].recordmask < 1 ? 1 : s_RecordPlan[i][j].arming[k].recordmask;
				}
			}
		}
	}
	return 0;
}

static int cv_cfgm_FillcJson(cJSON_Struct* opsNode)
{
	if(opsNode)
	{
		Common_Json_SetAttrValue(opsNode, -1, "RecycleRecord", Common_Json_Type_Number, NULL, s_RecycleRecord, 0);
		Common_Json_SetAttrValue(opsNode, -1, "PreserveMode", Common_Json_Type_Number, NULL, s_PreserverRecord.PreserveMode, 0);
		Common_Json_SetAttrValue(opsNode, -1, "PreserveTime", Common_Json_Type_Number, NULL, s_PreserverRecord.PreserveTime, 0);
		Common_Json_SetAttrValue(opsNode, -1, "PreserveVolume", Common_Json_Type_Number, NULL, s_PreserverRecord.PreserveVolume, 0);
		Common_Json_SetAttrValue(opsNode, -1, "PreservePercent", Common_Json_Type_Number, NULL, s_PreserverRecord.PreservePercent, 0);
		Common_Json_SetAttrValue(opsNode, -1, "HistoryStreamMaxNum", Common_Json_Type_Number, NULL, s_HistoryStreamMaxNum, 0);
		for(U32 i = 0; i < s_SysCap.video_ch_num; i ++)
		{
			S8 str[128];

			sprintf(str, "Channel%d", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Object, NULL, 0, 0);

			sprintf(str, "Channel%d/RecordMode", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_RecordMode[i], 0);
			sprintf(str, "Channel%d/ManualStreamMask", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_ManualStreamMask[i], 0);
			sprintf(str, "Channel%d/PreRecordMode", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_PreRecord[i].PreRecordMode, 0);
			sprintf(str, "Channel%d/PreRecordTime", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_PreRecord[i].PreRecordTime, 0);
			sprintf(str, "Channel%d/PreRecordSpace", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_PreRecord[i].PreRecordMem, 0);
#ifdef JZT31N
			//报警录像&&延迟时间小于120秒,设置为120秒
			if(s_PreRecord[i].DelayRecordTime < 120)
			{
				s_PreRecord[i].DelayRecordTime = 120;
			}
#endif
			sprintf(str, "Channel%d/PreRecordDelayTime", i);
			Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_PreRecord[i].DelayRecordTime, 0);

			for(U32 j = 0; j < 7; j ++)
			{
				sprintf(str, "Channel%d/Weekday%d", i, j);
				Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Object, NULL, 0, 0);

				sprintf(str, "Channel%d/Weekday%d/AlldayEnable", i, j);
				Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_RecordPlan[i][j].bAlldayRec, 0);
				for(U32 k = 0; k < CV_MAX_ARMING_TIME_QUANTUM; k ++)
				{
					sprintf(str, "Channel%d/Weekday%d/Sched%d", i, j, k);
					Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Object, NULL, 0, 0);

					sprintf(str, "Channel%d/Weekday%d/Sched%d/StartTime", i, j, k);
					Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_RecordPlan[i][j].arming[k].time.start_hour_min, 0);
					sprintf(str, "Channel%d/Weekday%d/Sched%d/StopTime", i, j, k);
					Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_RecordPlan[i][j].arming[k].time.stop_hour_min, 0);
					sprintf(str, "Channel%d/Weekday%d/Sched%d/SchedMode", i, j, k);
					Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_RecordPlan[i][j].arming[k].arming_type, 0);
					sprintf(str, "Channel%d/Weekday%d/Sched%d/StreamMask", i, j, k);
					Common_Json_SetAttrValue(opsNode, -1, str, Common_Json_Type_Number, NULL, s_RecordPlan[i][j].arming[k].recordmask, 0);
				}
			}
		}
	}
	return 0;
}

void cv_cfgm_default()
{
	for(int i = 0; i < CV_MAX_LOCAL_CH_NUM; i ++)
	{
		s_RecordMode[i] = 0;
		s_ManualStreamMask[i] = 0x0d;
		s_PreRecord[i].DelayRecordTime = 60;
		s_PreRecord[i].PreRecordMode = 0;
		s_PreRecord[i].PreRecordTime = 0;
		s_PreRecord[i].PreRecordMem = (5<<20);
		for(int j = 0; j < 7; j ++)
		{
			s_RecordPlan[i][j].bAlldayRec = 1;
			for(int k = 0; k < CV_MAX_ARMING_TIME_QUANTUM; k ++)
			{
				if(k == 0)
				{
					s_RecordPlan[i][j].arming[k].time.start_hour_min = 0000;
					s_RecordPlan[i][j].arming[k].time.stop_hour_min = 2359;
					s_RecordPlan[i][j].arming[k].arming_type = 0;
					s_RecordPlan[i][j].arming[k].recordmask = 0x0d;
				}
				else
				{
					s_RecordPlan[i][j].arming[k].time.start_hour_min = 0000;
					s_RecordPlan[i][j].arming[k].time.stop_hour_min = 0;
					s_RecordPlan[i][j].arming[k].arming_type = 0;
					s_RecordPlan[i][j].arming[k].recordmask = 0x0d;
				}
			}
		}
	}
}

CV_ERR cv_cfgm_saveconfig()
{
	S32 ret = -1;
	cJSON_Struct	*pSetNode = NULL;
	pSetNode = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	if(pSetNode)
	{
		cv_cfgm_FillcJson(pSetNode);
		ret = Module_SaveConfig(sStreamModeHandle, pSetNode);
		Common_Json_Delete(pSetNode);
		if(ret != MODULE_ERROR_TYPE_SUCC)
		{
			LOGE("Module_SaveConfig err : ret = %d\n", ret);
		}
		else
		{
			return CV_SUCCESS;
		}
	}
	return CV_ERR_REC_MALLOC_FAIL;
}

CV_ERR cv_cfgm_initconfig()
{
	cJSON_Struct	*pGetNode = NULL, *pSetNode = NULL, *pDefaultNode = NULL;
    S8 *pStrTmp = NULL;

	cv_cfgm_default();

	S32 ret = Module_LoadConfigByType(sStreamModeHandle,Module_ConfigType_Custom,&pGetNode);
	if(ret == MODULE_ERROR_TYPE_SUCC)
	{
		Common_Json_GetAttrValue(pGetNode, -1, "PreRecord", NULL, NULL, (S32*)&s_RecordFunc.PreRecord, NULL);

        Common_Json_GetAttrValueInt(pGetNode, "SDCapacity", &s_RecordCustom.SDCapacity);
        Common_Json_GetAttrValueStr(pGetNode, "SDRecordableDays", &pStrTmp);
        if(pStrTmp)
        {
            snprintf(s_RecordCustom.SDRecordableDays, sizeof(s_RecordCustom.SDRecordableDays),"%s",pStrTmp);
        }
	}
	if(pGetNode)
	{
		Common_Json_Delete(pGetNode);
		pGetNode = NULL;
	}

    Module_LoadConfigByType(sStreamModeHandle,Module_ConfigType_Default,&pDefaultNode);
    Common_Json_GetAttrValue(pDefaultNode, -1, "HistoryStreamMaxNum", NULL, NULL, (S32*)&s_HistoryStreamMaxNum, NULL);

    ret = Module_LoadConfig(sStreamModeHandle, &pGetNode);
	if(ret != MODULE_ERROR_TYPE_SUCC)
	{
		LOGD("Load default config!\n");
		if(pDefaultNode)
		{
            pGetNode = Common_Json_Duplicate(pDefaultNode, 1);
        }
        else
		{
			LOGW("LoadConfig failed,use default cfg!\n");
			pSetNode = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
			if(pSetNode)
			{
				cv_cfgm_FillcJson(pSetNode);
				ret = Module_SaveConfig(sStreamModeHandle, pSetNode);
				if(ret != MODULE_ERROR_TYPE_SUCC)
				{
					LOGE("Module_SaveConfig err : ret = %d\n", ret);
				}
				Common_Json_Delete(pSetNode);
			}
		}
	}
	if(pGetNode)
	{
		cv_cfgm_FillRecordPlan(pGetNode);
		Common_Json_Delete(pGetNode);
	}

    if(pDefaultNode)
	{
		Common_Json_Delete(pDefaultNode);
		pDefaultNode = NULL;
	}
    LOGD("s_HistoryStreamMaxNum:[%d]\n",s_HistoryStreamMaxNum);
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_initdevcap()
{
#if 1
	cJSON_Struct *pConfig,*pOutParams = NULL;
	S32 iRetval, ErrCode;
	U32	vi_dev = 0, vi_chn = 0, totalchannelnum = 0,ai_chn = 0;

	pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig, -1, "Header", Common_Json_Type_Object, "ovfs", 0, 0);
		Common_Json_SetAttrValue(pConfig, -1, "Header/Uri", Common_Json_Type_String, "/BoardSys/Video/Ability/Number", 0, 0);
		Common_Json_SetAttrValue(pConfig, -1, "Header/Method", Common_Json_Type_String, "get", 0, 0);

		iRetval = Module_CallFunctions(sStreamModeHandle, pConfig, &pOutParams, 3000);
		Common_Json_Delete(pConfig);
		if(iRetval == MODULE_ERROR_TYPE_SUCC && pOutParams != NULL)
		{
			Common_Json_GetAttrValue(pOutParams, -1, "Header/Code", NULL, NULL, &ErrCode, NULL);
			if(ErrCode == MODULE_ERROR_TYPE_SUCC)
			{
				Common_Json_GetAttrValue(pOutParams, -1, "Data/ChanTotalNum", NULL, NULL, (S32*)&s_SysCap.video_ch_num, NULL);
				Common_Json_GetAttrValue(pOutParams, -1, "Data/DevTotalNum", NULL, NULL, (S32*)&vi_dev, NULL);
				s_SysCap.video_vi_num = vi_dev;
				for(U32 i = 0; i < vi_dev; i ++)
				{
					S8 str[128];
					sprintf(str, "Data/viDev%d/viChanNum", i);
					Common_Json_GetAttrValue(pOutParams, -1, str, NULL, NULL, (S32*)&vi_chn, NULL);
					s_SysCap.video_vi_chn[i] = vi_chn;
					for(U32 j = 0; j < vi_chn; j ++)
					{
						s_SysCap.video_ch_videv[totalchannelnum + j] = i;
						s_SysCap.video_ch_vichn[totalchannelnum + j] = j;
						sprintf(str, "Data/viDev%d/viChan%d", i, j);
						Common_Json_GetAttrValue(pOutParams, -1, str, NULL, NULL, (S32*)&s_SysCap.video_stream_num[totalchannelnum + j], NULL);
					}
					totalchannelnum += vi_chn;
					vi_chn = 0;
				}
			}
			Common_Json_Delete(pOutParams);
			pOutParams = NULL;
		}

		if(s_SysCap.video_ch_num != totalchannelnum)
			s_SysCap.video_ch_num = totalchannelnum;
	}

	pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig, -1, "Header", Common_Json_Type_Object, "ovfs", 0, 0);
		Common_Json_SetAttrValue(pConfig, -1, "Header/Uri", Common_Json_Type_String, "/BoardSys/Audio/Ability", 0, 0);
		Common_Json_SetAttrValue(pConfig, -1, "Header/Method", Common_Json_Type_String, "get", 0, 0);
		iRetval = Module_CallFunctions(sStreamModeHandle, pConfig, &pOutParams, 3000);
		Common_Json_Delete(pConfig);
		if(iRetval == MODULE_ERROR_TYPE_SUCC)
		{
			Common_Json_GetAttrValue(pOutParams, -1, "Header/Code", NULL, NULL, &ErrCode, NULL);
			if(ErrCode == MODULE_ERROR_TYPE_SUCC)
			{
				Common_Json_GetAttrValue(pOutParams, -1, "Data/AencChanNum", NULL, NULL, (S32*)&s_SysCap.audio_ch_num, NULL);
			}
			Common_Json_Delete(pOutParams);
			pOutParams = NULL;
		}
	}
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/boardsys/event/ability",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
		iRetval = Module_CallFunctions(sStreamModeHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		if(pOutParams &&(iRetval == MODULE_ERROR_TYPE_SUCC))
		{
			Common_Json_GetAttrValue(pOutParams,-1,"Data/AlarmInNum",NULL,NULL,(S32*)&ai_chn,NULL);
			s_SysCap.alarm_in_num = s_SysCap.video_ch_num+ai_chn;
			LOGW("alarm_in_num:%d\n",s_SysCap.alarm_in_num);
		}
		Common_Json_Delete(pOutParams);
		pOutParams = NULL;
	}
	if(s_SysCap.video_ch_num > 0)
		s_RecordCapInit = 0;
#else
	s_SysCap.video_ch_num = 1;
	s_SysCap.video_ch_videv[0] = 0;
	s_SysCap.video_ch_vichn[0] = 0;
	s_SysCap.video_stream_num[0] = 2;
	s_SysCap.video_vi_num = 1;
	s_SysCap.video_vi_chn[0] = 1;
	s_SysCap.audio_ch_num =1;
	s_RecordCapInit = 0;
#endif
	return CV_SUCCESS;
}

void cv_cfgm_init(void* Handle)
{
	sStreamModeHandle = (ModuleHandle_T)Handle;
	while(s_RecordCapInit != 0)
	{
		cv_cfgm_initdevcap();
		if(s_RecordCapInit != 0)
			Common_Sleep(1, 0);
	}
	cv_cfgm_initconfig();
	s_RecordConfigInit = 0;
}

CV_ERR cv_cfgm_get_prerecord(U32 ch, PRERECORDRULE *pPrerecord)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;
	if(pPrerecord == NULL)
		return CV_ERR_REC_INVALID_PARA;
	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	pPrerecord->PreRecordMode = s_PreRecord[ch].PreRecordMode;
	pPrerecord->PreRecordTime = s_PreRecord[ch].PreRecordTime;
	pPrerecord->PreRecordMem = s_PreRecord[ch].PreRecordMem;
	pPrerecord->DelayRecordTime = s_PreRecord[ch].DelayRecordTime;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_set_prerecord(U32 ch, PRERECORDRULE Prerecord)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;
	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	s_PreRecord[ch].PreRecordMode = Prerecord.PreRecordMode;
	s_PreRecord[ch].PreRecordTime = Prerecord.PreRecordTime;
	s_PreRecord[ch].PreRecordMem = Prerecord.PreRecordMem;
	s_PreRecord[ch].DelayRecordTime = Prerecord.DelayRecordTime;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_plan_day(U32 ch, U32 day, cv_record_arming_day *pPlan)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;
	if(pPlan == NULL)
		return CV_ERR_REC_INVALID_PARA;
	if(ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;
	if(ch >= 7)
		return CV_ERR_REC_INVALID_PARA;

	pPlan->bAlldayRec = s_RecordPlan[ch][day].bAlldayRec;
	for(U32 i = 0; i < CV_MAX_ARMING_TIME_QUANTUM; i ++)
	{
		pPlan->arming[i] = s_RecordPlan[ch][day].arming[i];
	}
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_set_plan_day(U32 ch, U32 day, cv_record_arming_day Plan)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;
	if(ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;
	if(day >= 7)
		return CV_ERR_REC_INVALID_PARA;

	s_RecordPlan[ch][day].bAlldayRec = Plan.bAlldayRec;
	for(U32 i = 0; i < CV_MAX_ARMING_TIME_QUANTUM; i ++)
	{
		s_RecordPlan[ch][day].arming[i] = Plan.arming[i];
	}
    return CV_SUCCESS;
}

unsigned char respBuffer[1024*3] = {0};
cv_device_capability* cv_cfgm_get_dev_cap()
{
	return &s_SysCap;
}

CV_ERR cv_cfgm_get_record_mode(U32 ch, U32 *mode)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	*mode = s_RecordMode[ch];
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_set_record_mode(U32 ch, U32 mode)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	s_RecordMode[ch] = mode;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_manual_mask(U32 ch, U32 *mask)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	*mask = s_ManualStreamMask[ch];
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_set_manual_mask(U32 ch, U32 mask)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	s_ManualStreamMask[ch] = mask;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_local_rec_plan(U32 ch, cv_local_rec_plan *plan)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	plan->monday = s_RecordPlan[ch][1];
	plan->tuesday = s_RecordPlan[ch][2];
	plan->wednesday = s_RecordPlan[ch][3];
	plan->thursday = s_RecordPlan[ch][4];
	plan->friday = s_RecordPlan[ch][5];
	plan->saturday = s_RecordPlan[ch][6];
	plan->sunday = s_RecordPlan[ch][0];
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_pre_rec_mode(U32 ch, U32 *pMode)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	*pMode = s_PreRecord[ch].PreRecordMode;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_pre_rec_sec(U32 ch, U32 *pSec)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	*pSec = s_PreRecord[ch].PreRecordTime;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_pre_rec_mem(U32 ch, U32 *pMem)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	*pMem = s_PreRecord[ch].PreRecordMem;
    return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_delay_rec_sec(U32 ch, U32 *pSec)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;

	#if 0
	switch(s_PreRecord[ch].DelayRecordTime)
	{
	case 0:
		*pSec = 5;
		break;
	case 1:
		*pSec = 20;
		break;
	case 2:
		*pSec = 30;
		break;
	case 3:
		*pSec = 60;
		break;
	case 4:
		*pSec = 120;
		break;
	case 5:
		*pSec = 300;
		break;
	case 6:
		*pSec = 600;
		break;
	default:
		*pSec = 0;
		break;
	}
	#else
	*pSec= s_PreRecord[ch].DelayRecordTime;
	#endif
    return CV_SUCCESS;
}

U32 cv_cfgm_get_streamhandle(U32 ch, U32 streamtype)
{
	cJSON_Struct *pConfig,*pOutParams = NULL;
	S32 iRetval, iStreamHandle = 0;
	S8 *str, struri[128];

	if(s_RecordConfigInit < 0)
		return 0;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return 0;
	if(streamtype < 0 || (streamtype >= s_SysCap.video_stream_num[ch] && streamtype != CV_ENC_STR_AUDIO&&streamtype != CV_ENC_STR_SMART))
		return 0;

	pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig, -1, "Header", Common_Json_Type_Object, "ovfs", 0, 0);
		if(streamtype < CV_ENC_STR_AUDIO)
			sprintf(struri, "/BoardSys/Video/LiveStream/Device%d/Channel%d/Stream%d/Address", s_SysCap.video_ch_videv[ch], s_SysCap.video_ch_vichn[ch], streamtype);
		else if(streamtype == CV_ENC_STR_AUDIO)
			sprintf(struri, "/BoardSys/Audio/Aenc/LiveStream/Channel%d/Address", ch);
		else if(streamtype == CV_ENC_STR_SMART)
			sprintf(struri, "/SmartServer/Stream/Address");
		Common_Json_SetAttrValue(pConfig, -1, "Header/Uri", Common_Json_Type_String, struri, 0, 0);
		Common_Json_SetAttrValue(pConfig, -1, "Header/Method", Common_Json_Type_String, "get", 0, 0);
		iRetval = Module_CallFunctions(sStreamModeHandle, pConfig, &pOutParams, 3000);
		if(iRetval == MODULE_ERROR_TYPE_SUCC && pOutParams != NULL)
		{
			S32 ErrCode;
			Common_Json_GetAttrValue(pOutParams, -1, "Header/Code", NULL, NULL, &ErrCode, NULL);
			if(ErrCode == MODULE_ERROR_TYPE_SUCC)
			{
				Common_Json_GetAttrValue(pOutParams, -1, "Data/AddressString", NULL, &str, NULL, NULL);
				if(str[0] != 0)
				{
					CoOpen_Param_T param[2];
					memset(param, 0, sizeof(param));
					param[0].nIndex = 0;
					param[0].szUri = str;
					iStreamHandle = Module_StreamQueue_CoOpen(sStreamModeHandle, -1, param, 1, 1000);
					if(param[0].nErrorCode)
						iStreamHandle = 0;
//					iStreamHandle = Module_StreamQueue_Open(sStreamModeHandle, str, NULL, NULL, 100);
				}
			}
		}
		Common_Json_Delete(pConfig);
		Common_Json_Delete(pOutParams);
		pOutParams = NULL;
	}

	return iStreamHandle;
}

S32 cv_cfgm_ReadData(S32 fd, cJSON_Struct **pPrivInfo, void **pData, S32 *lpSize, S32 nMSecTimeout)
{
	return Module_StreamQueue_ReadData(sStreamModeHandle, fd, 0, NULL, pPrivInfo, pData, lpSize, nMSecTimeout);
}

S32 cv_cfgm_ReleaseData(S32 fd)
{
	return Module_StreamQueue_ReleaseData(sStreamModeHandle, fd);
}

void cv_cfgm_RequestVideoIFrame(int streamtype)
{
	cJSON_Struct *pInData = NULL,*pOutData = NULL;
	char uri[256] = {0};
	int device = 0, channel = 0;

	snprintf(uri, sizeof(uri) - 1, "/BoardSys/Video/ReqIFrame?Device=%d&Channel=%d&Stream=%d", device, channel, streamtype);
    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,uri,0,0);
    Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"put",0,0);

    LOGE("cv_cfgm_RequestVideoIFrame [%s]\n",uri);
    Module_CallFunctions(sStreamModeHandle,pInData,&pOutData,3000);

    Common_Json_Delete(pInData);
    Common_Json_Delete(pOutData);
}

S32 cv_cfgm_get_channel(U32 videv, U32 vichn)
{
	if(s_RecordConfigInit < 0)
		return -1;
	if(s_RecordCapInit < 0)
		return -1;

	for(U32 i = 0; i < s_SysCap.video_ch_num; i ++)
	{
		if(videv == s_SysCap.video_ch_videv[i] && vichn == s_SysCap.video_ch_vichn[i])
			return i;
	}

	return -1;
}

CV_ERR cv_cfgm_change_event(cv_event_id *event, U32 alarmtype, U32 alarmsrc)
{
	if(event == NULL)
		return CV_ERR_REC_INVALID_PARA;

	switch(alarmtype)
	{
	case ALARM_TYPE_ALARMIN:
	{
		U32 aiNum = s_SysCap.alarm_in_num - s_SysCap.video_ch_num;
		if(alarmsrc >= aiNum)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_ALARM_EVENT;
		event->event_type.sub_type = CV_EVENT_LOCAL_ALRAM_IN;
		event->event_id.alarm_id.is_local = TRUE;
		event->event_id.alarm_id.src.ch_idx = alarmsrc;
		break;
	}
	case ALARM_TYPE_MOTION:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_MD;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_VHIDE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		//event->event_type.sub_type = CV_EVENT_OD;
		event->event_type.sub_type = CV_EVENT_MD;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_VDIAGNOSE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_VIDEO_DIAGNOSE;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_COUNTER_WIRE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_OBJECT_COUNT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_DETECT_WIRE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_CROSS_LINE;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_DETECT_REGION:
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_REGION_DETECT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_OBJECT_REGION:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_GOODS_DETECT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_SOUND_DETECT:
		if(alarmsrc >= s_SysCap.audio_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_SOUND_DETECT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_SMART_MOTION:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_MOTION;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_DETECT_FIRE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_FIRE_DETECT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_DETECT_FACE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_FACE_DETECT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_DETECT_PLATE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_CAR_PLATE_DETECT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_RECOGNITION_FACE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_RECOGNITION_FACE;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_HUMANOID:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_HUMANOID;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_DETECT_PERSON:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_DETECT_PERSON;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_RUN:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_RUN;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_VIOLENTMD:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_VIOLENTMD;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_MAXHEIGHT:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_MAXHEIGHT;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_HIGHDENSITY:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_HIGHDENSITY;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_RETROGRADE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_RETROGRADE;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_SCENCECHANGE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_SCENCECHANGE;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_TEMPERATURE:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_TEMPERATURE;
		event->event_id.ch_idx = alarmsrc;
		break;
	case ALARM_TYPE_CABLE_BREAK:
		event->event_type.main_type = CV_EVENT_NET_EVENT;
		event->event_type.sub_type = CV_EVENT_NET_DISCONNECT;
		event->event_id.ch_idx = 0;
		break;
	case ALARM_TYPE_IP_CONFLIT:
		event->event_type.main_type = CV_EVENT_NET_EVENT;
		event->event_type.sub_type = CV_EVENT_NET_IP_CONFLICT;
		event->event_id.ch_idx = 0;
		break;
	case ALARM_TYPE_ILLEG_ACCESS:
		event->event_type.main_type = CV_EVENT_NET_EVENT;
		event->event_type.sub_type = CV_EVENT_NET_ILLEG_ACCESS;
		event->event_id.ch_idx = 0;
		break;
	case ALARM_TYPE_CLIENT_BREAK:
		event->event_type.main_type = CV_EVENT_NET_EVENT;
		event->event_type.sub_type = CV_EVENT_NET_CLIENT_DISCONNECT;
		event->event_id.ch_idx = 0;
		break;
    case ALARM_TYPE_DETECT_MOTOR:
		if(alarmsrc >= s_SysCap.video_ch_num)
			return CV_ERR_REC_INVALID_PARA;
		event->event_type.main_type = CV_EVENT_CH_EVENT;
		event->event_type.sub_type = CV_EVENT_SMART_DETECT_MOTOR;
		event->event_id.ch_idx = alarmsrc;
		break;
	default:
		return CV_ERR_REC_INVALID_PARA;
		break;
	}
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_recplan(U32 ch, U32 *pRecordMode)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	if(ch < 0 || ch >= CV_MAX_LOCAL_CH_NUM)
		return CV_ERR_REC_INVALID_PARA;
	if(pRecordMode == NULL)
		return CV_ERR_REC_INVALID_PARA;

	*pRecordMode = s_RecordMode[ch];
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_diskconfig(S32 *pRecycle, PRESERVERECORDRULE *pPreserve)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	*pRecycle = s_RecycleRecord;
	pPreserve->PreserveMode = s_PreserverRecord.PreserveMode;
	pPreserve->PreserveTime = s_PreserverRecord.PreserveTime;
	pPreserve->PreservePercent = s_PreserverRecord.PreservePercent;
	pPreserve->PreserveVolume = s_PreserverRecord.PreserveVolume;
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_set_diskconfig(S32 Recycle, PRESERVERECORDRULE Preserve)
{
	if(s_RecordConfigInit < 0)
		return CV_ERR_REC_NOT_INIT;

	s_RecycleRecord = Recycle;
	s_PreserverRecord.PreserveMode = Preserve.PreserveMode;
	s_PreserverRecord.PreserveTime= Preserve.PreserveTime;
	s_PreserverRecord.PreserveVolume = Preserve.PreserveVolume;
	s_PreserverRecord.PreservePercent = Preserve.PreservePercent;
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_get_record_func(RECORD_FUNCTION *sRecFunc)
{
	*sRecFunc = s_RecordFunc;
	return CV_SUCCESS;
}

S32 cv_cfgm_get_historystream_maxnum()
{
    return s_HistoryStreamMaxNum;
}

CV_ERR cv_cfgm_get_record_custom(RECORD_CUSTOM *sRecCustom)
{
	*sRecCustom = s_RecordCustom;
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_change_record_encode(int nRecordStatue)
{
	Common_cJSON_T *inParam = Common_cJSON_CreateObject();
	Common_cJSON_T* outparam = NULL;
	if(inParam)
	{
		Common_cJSON_T *header = Common_cJSON_CreateObject();
		Common_cJSON_AddStringToObject(header, "Method", "put");
		Common_cJSON_AddStringToObject(header, "Uri", "/Boardsys/Sys/SetAlarmRecord");
		Common_cJSON_AddItemToObject(inParam, "Header", header);
		Common_cJSON_T *inData = Common_cJSON_CreateObject();
		Common_cJSON_AddNumberToObject(inData,"StartAlarmRecord",nRecordStatue);//1--720P;0--还原
		Common_cJSON_AddItemToObject(inParam, "Data", inData);
		Module_CallFunctions(sStreamModeHandle, (cJSON_Struct *)inParam, (cJSON_Struct **)(&outparam), 3000);
		if(outparam)
		{
			Common_cJSON_Delete(outparam);
			outparam = NULL;
		}
		/*
		Common_Json_SetAttrValueObj(header, "RemoteServerInfo");
		Common_Json_SetAttrValueStr(header, "RemoteServerInfo/IP", "10.0.0.2");
		Common_Json_SetAttrValueInt(header, "RemoteServerInfo/Port", 10009);
		Common_Json_SetAttrValueStr(header, "RemoteServerInfo/BindIF", "usb0");
		Module_CallFunctions_Remote("10.0.0.2", 10009, (cJSON_Struct *)inParam, (cJSON_Struct **)(&outparam), 3000);
		if(outparam)
		{
			Common_cJSON_Delete(outparam);
			outparam = NULL;
		}
		*/
		Common_cJSON_Delete(inParam);
		inParam = NULL;
	}
	return CV_SUCCESS;
}

CV_ERR cv_cfgm_play_audio(char *pFileName)
{
	if(pFileName == NULL)
		return CV_ERR_REC_INVALID_PARA;

	Common_cJSON_T *inParam = Common_cJSON_CreateObject();
	Common_cJSON_T* outparam = NULL;
	if(inParam)
	{
		Common_cJSON_T *header = Common_cJSON_CreateObject();
		Common_cJSON_AddStringToObject(header, "Method", "put");
		Common_cJSON_AddStringToObject(header, "Uri", "/BoardSys/Audio/Adec/PlayFile");
		Common_cJSON_AddItemToObject(inParam, "Header", header);
		Common_cJSON_T *inData = Common_cJSON_CreateObject();
		Common_cJSON_AddStringToObject(inData, "Path", pFileName);
		Common_cJSON_AddNumberToObject(inData, "Times", 1);
		Common_cJSON_AddNumberToObject(inData,"Priority",1);
		Common_cJSON_AddItemToObject(inParam, "Data", inData);
		
		Module_CallFunctions(sStreamModeHandle, (cJSON_Struct *)inParam, (cJSON_Struct **)(&outparam), 3000);
		
		if(outparam)
		{
			Common_cJSON_Delete(outparam);
			outparam = NULL;
		}
		if(inParam)
		{
			Common_cJSON_Delete(inParam);
			inParam = NULL;
		}
	}
	return CV_SUCCESS;
}


