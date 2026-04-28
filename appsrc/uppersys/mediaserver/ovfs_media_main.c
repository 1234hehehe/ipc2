/*
 * ovfs_media_main.c
 *
 *  Created on: 2016年7月21日
 *      Author: eric
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>

#include "ovfs_media.h"
#include"librecord_sdk.h"

typedef struct
{
    MQ_HANDLE_H mqHandle;
    pthread_pool_t *tpoolHandle;
    Common_cJSON_T *cfgFileJson;
    MEDIA_ALARM_STATUS_T alarmStatusInfo;
    MEDIA_BOARD_STATE_T boardState;
    int disconnectEventState;
} MEDIA_CONTEXT_T;

enum
{
    CFG_CONTROL_RTSP_GET_CFG,
    CFG_CONTROL_RTSP_GET_CFG_JSON,
    CFG_CONTROL_RTSP_SET_CFG_JSON,

    CFG_CONTROL_RTMP_GET_CFG,
    CFG_CONTROL_RTMP_GET_CFG_JSON,
    CFG_CONTROL_RTMP_SET_CFG_JSON,

    CFG_CONTROL_DISCON_GET_CFG,
    CFG_CONTROL_DISCON_GET_CFG_JSON,
    CFG_CONTROL_DISCON_SET_CFG_JSON,
#ifdef RTMP
    CFG_CONTROL_RTMPPUSH_GET_CFG,
    CFG_CONTROL_RTMPPUSH_GET_CFG_JSON,
    CFG_CONTROL_RTMPPUSH_SET_CFG_JSON,
    CFG_CONTROL_RTMPPUSH_NOTDISTURB_GET_CFG,
    CFG_CONTROL_RTMPPUSH_NOTDISTURB_GET_CFG_JSON,
    CFG_CONTROL_RTMPPUSH_NOTDISTURB_SET_CFG_JSON,
#endif
#ifdef WITH_CURL
	CFG_CONTROL_SMARTPROTOCOL_GET_CFG,
	CFG_CONTROL_SMARTPROTOCOL_GET_CFG_JSON,
	CFG_CONTROL_SMARTPROTOCOL_SET_CFG_JSON,
	CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_GET_CFG,
	CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_GET_CFG_JSON,
	CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_SET_CFG_JSON,
#endif
};

typedef int (*EXTER_RECORDSDK_INIT_F)();
typedef int (*EXTER_RECORDSDK_UNINIT_F)();
typedef struct
{
    EXTER_RECORDSDK_INIT_F   	init;
    EXTER_RECORDSDK_UNINIT_F    uninit;
}RECORDSDK_EXTERNAL_LIBS_T;

RECORDSDK_EXTERNAL_LIBS_T       s_recordfuncCb;

MEDIA_CONTEXT_T s_media_ct;

static void DumpJson(Common_cJSON_T *object)
{
    if (object)
    {
        char * p = Common_cJSON_Print(object,NULL);
        LOGD("@%s@\n",p);
        if (p)
            Common_cJSON_free(p);
    }
}

#ifdef WITH_CURL
static int SmartProtocolJsonToCfg(Common_cJSON_T *json, HTTP_PUSH_CFG_T* cfg)
{
    if (json == NULL || cfg == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    Common_cJSON_T *tmpJson = NULL;
    tmpJson = Common_cJSON_GetObjectItem(json,"Enable");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number)
    {
        cfg->enable = tmpJson->valueint;
    }
    else
    {
        LOGE("check HttpPush Enable failed\n");
        return -1;
    }

	tmpJson = Common_cJSON_GetObjectItem(json,"ServerAddr");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring)
    {
        snprintf(cfg->serverAddr,sizeof(cfg->serverAddr)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check SmartProtocolJsonToCfg serverAddr failed\n");
        return -1;
    }

	tmpJson = Common_cJSON_GetObjectItem(json,"HeartBeatInterval");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number)
    {
        cfg->heartInterval= tmpJson->valueint;
    }
    else
    {
        LOGE("check SmartProtocolJsonToCfg heartInterval failed\n");
        return -1;
    }

	tmpJson = Common_cJSON_GetObjectItem(json,"EventListMaxLen");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number)
    {
        cfg->eventListMaxLen = tmpJson->valueint;
    }
    else
    {
        LOGE("check SmartProtocolJsonToCfg EventListMaxLen failed\n");
        return -1;
    }

    return 0;
}

static int SmartProtocolNotDisturbJsonToCfg(cJSON_Struct *cfgJson, SMART_PROTO_NOTDISTURB_CFG_T *cfgStruct)
{
    SMART_PROTO_NOTDISTURB_CFG_T *structData = (SMART_PROTO_NOTDISTURB_CFG_T *) cfgStruct;
    char jsonPath[128] = { 0 };
    int retInt = 0;
    char *retP = NULL;
    int i = 0, j = 0;
    char pathname[128] = {0};

    if(Common_Json_GetAttrValueInt(cfgJson, "EnableAlarm", &retInt))
    {
        structData->enableAlarm = retInt;
    }

    if(Common_Json_GetAttrValueInt(cfgJson, "EnableSmartResult", &retInt))
    {
        structData->enableSmartResult = retInt;
    }

    if(Common_Json_GetAttrValueInt(cfgJson, "EnableBkgPic", &retInt))
    {
        structData->enableBkgPic = retInt;
    }

    if(Common_Json_GetAttrValueInt(cfgJson, "EnableConfig", &retInt))
    {
        structData->enableConfig = retInt;
    }

    cJSON_Struct *alarmlist = Common_Json_GetAttrValueArr(cfgJson, "AlarmList");
    if(alarmlist)
    {
        j = Common_Json_ArraySize(alarmlist);
        for(i=0; i<j; i++)
        {
            if(Common_Json_GetAttrValue(alarmlist, i, NULL, NULL, &retP, NULL, NULL))
            {
                snprintf(structData->alarmList[i],
                 sizeof(structData->alarmList[i]), "%s", retP);
            }
        }
    }

    for (i = 0; i < MAX_DAYS; i++)
    {
        for (j = 0; j < MAX_TIMESEGMENT; j++)
        {
            snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Start", i, j);
            if(Common_Json_GetAttrValueInt(cfgJson, pathname, &retInt))
            {
                structData->NotDisturbTime[i][j].startTime = retInt;
            }

			snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Stop", i, j);
            if(Common_Json_GetAttrValueInt(cfgJson, pathname, &retInt))
            {
                structData->NotDisturbTime[i][j].stopTime = retInt;
            }
        }
    }

    return 0;
}

#endif

static int RtmpJsonToCfg(Common_cJSON_T *json, RTMP_CONFIG_T* cfg)
{
    if (json == NULL || cfg == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    Common_cJSON_T *enableJson = NULL, *rtmpPortJson = NULL;
    enableJson = Common_cJSON_GetObjectItem(json,"Enable");
    rtmpPortJson = Common_cJSON_GetObjectItem(json,"RtmpPort");
    if (enableJson == NULL || rtmpPortJson == NULL)
    {
        LOGE("not find Enable or RtmpPort in json\n");
        return -1;
    }

    if (enableJson->type != Common_cJSON_Number || (enableJson->valueint != 0 && enableJson->valueint != 1))
    {
        LOGE("enable type or value error\n");
        return -1;
    }

    if (rtmpPortJson->type != Common_cJSON_Number || rtmpPortJson->valueint <=0)
    {
        LOGE("rtmpPort type or value error\n");
        return -1;
    }

    cfg->enable = enableJson->valueint;
    cfg->rtmpPort = rtmpPortJson->valueint;
    return 0;
}
#ifdef RTMP
static int RtmpPushJsonToCfg(Common_cJSON_T *json, RTMPPUSH_CONFIG_T* cfg)
{
	S8 *szUrl = NULL;
	int nEnable = 0,nEnableAudio = 0;
	int nClient = 0,nChannel = 0,nStream = 0 ;
	S8 szTmp[64];
	cJSON_Struct *pJson = (cJSON_Struct *)json;
    if (json == NULL || cfg == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

	for(nClient = 0; nClient < RTMPPUSH_SESSION_MAX_NUM; nClient++)
	{
		for(nChannel = 0; nChannel < RTMPPUSH_CHANNEL_MAX_NUM; nChannel++)
		{
			for(nStream = 0; nStream < RTMPPUSH_STREAM_MAX_NUM; nStream++)
			{
				nEnableAudio = 0;
				nEnable = 0;
				szUrl = NULL;
				sprintf(szTmp, "Client%d/device0/Channel%d/Stream%d/Enable",nClient,nChannel,nStream);
			    Common_Json_GetAttrValue(pJson, -1, szTmp, NULL, NULL,&nEnable,NULL);
				sprintf(szTmp, "Client%d/device0/Channel%d/Stream%d/EnableAudio",nClient,nChannel,nStream);
				Common_Json_GetAttrValue(pJson, -1, szTmp, NULL, NULL,&nEnableAudio,NULL);
				sprintf(szTmp, "Client%d/device0/Channel%d/Stream%d/Url",nClient,nChannel,nStream);
				Common_Json_GetAttrValue(pJson, -1, szTmp, NULL, &szUrl,NULL,NULL);
				cfg->tSession[nClient][nChannel][nStream].bEnable = nEnable != 0;
				cfg->tSession[nClient][nChannel][nStream].bEnableAudio = nEnableAudio != 0;
				cfg->tSession[nClient][nChannel][nStream].szUrl[0] = 0;
				if(szUrl != NULL)
				{
					strncpy(cfg->tSession[nClient][nChannel][nStream].szUrl,szUrl,511);
				}
			}

		}
	}



    return 0;
}

static int RtmpPushNotDisturbJsonToCfg(Common_cJSON_T *json, RTMPPUSH_NOTDISTURB_CONFIG_T* cfg)
{
    int i = 0;
    int j = 0;
    int i_num = 0;
	int nClient = 0;
	S8 *strTmp = NULL;
    S8 pathname[128] = {0};
	cJSON_Struct *pJson = (cJSON_Struct *)json;
    if (json == NULL || cfg == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    cJSON_Struct* ClientList = Common_Json_GetAttrValueArr(pJson, "ClientList");

	for(i = 0; i < RTMPPUSH_SESSION_MAX_NUM; i++)
	{
        if(Common_Json_GetAttrValue(ClientList, i, NULL, NULL, &strTmp,NULL,NULL))
        {
            if(sscanf(strTmp, "Client%d",&nClient))
            {
                cfg->Client[nClient] = 1;
            }
        }
	}

    cJSON_Struct* notdisturbtime = Common_Json_GetAttrValueObj(pJson, "NotDisturbTime");
    if(!notdisturbtime)
    {
        LOGE("get NotDisturbTime is NULL!\n");
        return -1;
    }

    for(i=0; i<MAX_DAYS; i++)
    {
        for(j=0; j<MAX_TIMESEGMENT; j++)
        {
            snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Start", i, j);
            if(Common_Json_GetAttrValueInt(notdisturbtime, pathname, &i_num))
            {
                cfg->NotDisturbTime[i][j].startTime = i_num;
            }

            snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Stop", i, j);
            if(Common_Json_GetAttrValueInt(notdisturbtime, pathname, &i_num))
            {
                cfg->NotDisturbTime[i][j].stopTime = i_num;
            }

        }
    }

    return 0;
}

#endif
static int RtspJsonToCfg(Common_cJSON_T *json, RTSP_CONFIG_T* cfg)
{
    if (json == NULL || cfg == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    RTSP_CONFIG_T tmpCfg = { 0 };
    Common_cJSON_T *tmpJson = NULL;
    tmpJson = Common_cJSON_GetObjectItem(json,"Enable");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && (tmpJson->valueint == 0 || tmpJson->valueint == 1))
    {
        tmpCfg.enable = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp Enable failed\n");
        return -1;
    }

    tmpJson = Common_cJSON_GetObjectItem(json,"StreamMaxNum");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && (tmpJson->valueint > 0))
    {
        tmpCfg.streamMaxNum = tmpJson->valueint;
    }
    else
    {
        LOGW("check rtsp streamMaxNum failed\n");
#if (defined PLATFORM_HI3516EV200 || defined PLATFORM_HI3516E)
        tmpCfg.streamMaxNum = 4;
#endif
    }

    LOGW("rtsp stream max num is %d\n", tmpCfg.streamMaxNum);

    tmpJson = Common_cJSON_GetObjectItem(json,"RtspPort");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint > 0)
    {
        tmpCfg.rtspPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp RtspPort failed\n");
        return -1;
    }

    tmpJson = Common_cJSON_GetObjectItem(json,"HttpPort");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint > 0)
    {
        tmpCfg.httpPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp HttpPort failed\n");
        return -1;
    }

    tmpJson = Common_cJSON_GetObjectItem(json,"Auth");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number)
    {
        if (tmpJson->valueint < MEDIA_AUTH_TYPE_AUTO || tmpJson->valueint > (MEDIA_AUTH_TYPE_TEXT | MEDIA_AUTH_TYPE_DIGEST))
        {
            LOGW("auth type error , use the default type digest authorize\n");
            tmpCfg.authMask = MEDIA_AUTH_TYPE_DIGEST;
        }
        else
            tmpCfg.authMask = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp Auth failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.Enable");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && (tmpJson->valueint == 0 || tmpJson->valueint == 1))
    {
        tmpCfg.multiEnable = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.Enable failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.MainVideo.IP");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring && strlen(tmpJson->valuestring) < sizeof(tmpCfg.multiMainVideoIp))
    {
        snprintf(tmpCfg.multiMainVideoIp,sizeof(tmpCfg.multiMainVideoIp)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check rtsp multiEnable failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.MainVideo.Port");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiMainVideoPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.MainVideo.Port failed\n");
        return -1;
    }


    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.MainVideo.TTL");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiMainVideoTtl = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.MainVideo.TTL failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.SubVideo.IP");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring && strlen(tmpJson->valuestring) < sizeof(tmpCfg.multiSubVideoIp))
    {
        snprintf(tmpCfg.multiSubVideoIp,sizeof(tmpCfg.multiSubVideoIp)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check rtsp MultiCast.SubVideo.IP failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.SubVideo.Port");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiSubVideoPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.SubVideo.Port failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.SubVideo.TTL");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiSubVideoTtl = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.SubVideo.TTL failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.ThirdVideo.IP");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring && strlen(tmpJson->valuestring) < sizeof(tmpCfg.multiSubVideoIp))
    {
        snprintf(tmpCfg.multiThirdVideoIp,sizeof(tmpCfg.multiThirdVideoIp)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check rtsp MultiCast.ThirdVideo.IP failed\n");
        return -1;
    }
    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.ThirdVideo.Port");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiThirdVideoPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.ThirdVideo.Port failed\n");
        return -1;
    }
    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.ThirdVideo.TTL");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiThirdVideoTtl = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.ThirdVideo.TTL failed\n");
        return -1;
    }
    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.MainAudio.IP");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring && strlen(tmpJson->valuestring) < sizeof(tmpCfg.multiMainAudioIp))
    {
        snprintf(tmpCfg.multiMainAudioIp,sizeof(tmpCfg.multiMainAudioIp)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check rtsp MultiCast.MainAudio.IP failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.MainAudio.Port");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiMainAudioPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.MainAudio.Port failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.MainAudio.TTL");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiMainAudioTtl = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.MainAudio.TTL failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.SubAudio.IP");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring && strlen(tmpJson->valuestring) < sizeof(tmpCfg.multiSubAudioIp))
    {
        snprintf(tmpCfg.multiSubAudioIp,sizeof(tmpCfg.multiSubAudioIp)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check rtsp MultiCast.SubAudio.IP failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.SubAudio.Port");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiSubAudioPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.SubAudio.Port failed\n");
        return -1;
    }

    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.SubAudio.TTL");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiSubAudioTtl = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.SubAudio.TTL failed\n");
        return -1;
    }
    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.ThirdAudio.IP");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_String && tmpJson->valuestring && strlen(tmpJson->valuestring) < sizeof(tmpCfg.multiMainAudioIp))
    {
        snprintf(tmpCfg.multiThirdAudioIp,sizeof(tmpCfg.multiThirdAudioIp)-1,"%s",tmpJson->valuestring);
    }
    else
    {
        LOGE("check rtsp MultiCast.ThirdAudio.IP failed\n");
        return -1;
    }
    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.ThirdAudio.Port");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiThirdAudioPort = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.ThirdAudio.Port failed\n");
        return -1;
    }
    tmpJson = JsonOper_GetObjectItemByPath(json,"MultiCast.ThirdAudio.TTL");
    if (tmpJson != NULL && tmpJson->type == Common_cJSON_Number && tmpJson->valueint >= 0)
    {
        tmpCfg.multiThirdAudioTtl = tmpJson->valueint;
    }
    else
    {
        LOGE("check rtsp MultiCast.ThirdAudio.TTL failed\n");
        return -1;
    }

    memcpy(cfg,&tmpCfg,sizeof(RTSP_CONFIG_T));
    return 0;
}

static int DisconnectJsonToCfg(Common_cJSON_T *json, MEDIA_DISCONNECT_INFO_T* cfg)
{
    if (json == NULL || cfg == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -1;
    }

    Common_cJSON_T *tmpJson = NULL;
    tmpJson = Common_cJSON_GetObjectItem(json, "DisconnectIP");
    if (tmpJson == NULL)
    {
        LOGE("not find DisconnectIP in json\n");
        return -1;
    }

    if (tmpJson->type != Common_cJSON_String || tmpJson->valuestring == NULL)
    {
        LOGE("IpJson type or value error\n");
        return -1;
    }

    snprintf(cfg->disconnectIp, sizeof(cfg->disconnectIp), "%s", tmpJson->valuestring);

    tmpJson = Common_cJSON_GetObjectItem(json, "DeviceId");
    if (tmpJson == NULL)
    {
        LOGE("not find DeviceId in json\n");
        return -1;
    }

    if (tmpJson->type != Common_cJSON_Number)
    {
        LOGE("DeviceId type error\n");
        return -1;
    }
    cfg->deviceId = tmpJson->valueint;

    tmpJson = Common_cJSON_GetObjectItem(json, "ChannelId");
    if (tmpJson == NULL)
    {
        LOGE("not find ChannelId in json\n");
        return -1;
    }

    if (tmpJson->type != Common_cJSON_Number)
    {
        LOGE("ChannelId type error\n");
        return -1;
    }
    cfg->channelId = tmpJson->valueint;

    tmpJson = Common_cJSON_GetObjectItem(json, "StreamId");
    if (tmpJson == NULL)
    {
        LOGE("not find StreamId in json\n");
        return -1;
    }

    if (tmpJson->type != Common_cJSON_Number)
    {
        LOGE("StreamId type error\n");
        return -1;
    }
    cfg->streamId = tmpJson->valueint;
    return 0;
}

static void AsyncTaskDoThread(void *data)
{
    int ret = 0, taskId = 0, delay = 0;
    if (data == NULL)
    {
        LOGE("task data is null\n");
        return;
    }

    memcpy(&taskId, data, sizeof(taskId));
    memcpy(&delay, data + sizeof(taskId), sizeof(delay));
    MEDIA_FREE(data);

    if (delay > 0)
        Utils_Sleep(delay);

    switch (taskId)
    {
        case MEDIA_ASYNC_TASK_RTSP_INIT:
        {
            prctl(PR_SET_NAME,"RtspMgr_Init");
            if ((ret = RtspMgr_Init(s_media_ct.mqHandle)) < 0)
            {
                LOGE("rtsp  server init failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTSP_SERVER_INIT_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTSP_SERVER_INIT_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_RTSP_START:
        {
            prctl(PR_SET_NAME,"RtspMgr_Start");
            if ((ret = RtspMgr_Start()) < 0)
            {
                LOGE("rtsp  server start failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTSP_SERVER_START_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTSP_SERVER_START_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_RTSP_STOP:
        {
            prctl(PR_SET_NAME,"RtspMgr_Stop");
            if ((ret = RtspMgr_Stop()) < 0)
            {
                LOGE("rtsp  server stop failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTSP_SERVER_STOP_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTSP_SERVER_STOP_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_RTSP_RESTART:
        {
            LOGI("MEDIA_ASYNC_TASK_RTSP_RESTART\n");
            prctl(PR_SET_NAME,"RtspMgr_Restart");
            if ((ret = RtspMgr_Restart()) < 0)
            {
                LOGE("rtsp  server restart failed %d\n",ret);
            }
            break;
        }


        case MEDIA_ASYNC_TASK_RTMP_INIT:
        {
            prctl(PR_SET_NAME,"RtmpMgr_Init");
            if ((ret = RtmpMgr_Init(s_media_ct.mqHandle)) < 0)
            {
                LOGE("rtmp  server init failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTMP_SERVER_INIT_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTMP_SERVER_INIT_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_RTMP_START:
        {
            prctl(PR_SET_NAME,"RtmpMgr_Start");
            if ((ret = RtmpMgr_Start()) < 0)
            {
                LOGE("rtmp  server start failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTMP_SERVER_START_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTMP_SERVER_START_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_RTMP_STOP:
        {
            prctl(PR_SET_NAME,"RtmpMgr_Stop");
            if ((ret = RtmpMgr_Stop()) < 0)
            {
                LOGE("rtmp  server stop failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTMP_SERVER_STOP_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_RTMP_SERVER_STOP_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_RTMP_RESTART:
        {
            LOGI("MEDIA_ASYNC_TASK_RTMP_RESTART\n");
            prctl(PR_SET_NAME,"RtmpMgr_Restart");
            if ((ret = RtmpMgr_Restart()) < 0)
            {
                LOGE("rtmp server restart failed %d\n",ret);
            }
            break;
        }
#ifdef RTMP
		 case MEDIA_ASYNC_TASK_RTMPPUSH_INIT:
        {
            prctl(PR_SET_NAME,"RtmpMgr_Init");
            ret = RtmpPushMgr_Init(s_media_ct.mqHandle);


            break;
        }
		 case MEDIA_ASYNC_TASK_RTMPPUSH_RESTART:
        {
            LOGI("MEDIA_ASYNC_TASK_RTMPPUSH_RESTART\n");
            prctl(PR_SET_NAME,"RtmpPushMgr_Restart");
            if ((ret = RtmpPushMgr_Restart()) < 0)
            {
                LOGE("rtmp server restart failed %d\n",ret);
            }
            break;
        }
#endif

        case MEDIA_ASYNC_TASK_REST_INIT:
        {
            prctl(PR_SET_NAME,"RestMedia_Init");
            if ((ret = RestMedia_Init(s_media_ct.mqHandle)) < 0)
            {
                LOGE("rest  init failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_REST_INIT_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_REST_INIT_DONE,NULL,0);
            }
            break;
        }
#ifdef	WITH_CURL
		//HttpPush
		case MEDIA_ASYNC_TASK_SMARTPROTOCOL_INIT:
        {
            prctl(PR_SET_NAME,"SmartProto_Init");
            if ((ret = SmartProto_Init(s_media_ct.mqHandle)) < 0)
            {
                LOGE("SmartProto_Init failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_SMARTPROTOCOL_INIT_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_SMARTPROTOCOL_INIT_DONE,NULL,0);
            }
            break;
        }
		case MEDIA_ASYNC_TASK_SMARTPROTOCOL_START:
        {
            prctl(PR_SET_NAME,"SmartProto_Start");
            if ((ret = SmartProto_Start()) < 0)
            {
                LOGE("SmartProto_Start failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_SMARTPROTOCOL_START_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_SMARTPROTOCOL_START_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_SMARTPROTOCOL_STOP:
        {
            prctl(PR_SET_NAME,"SmartProto_Stop");
            if ((ret = SmartProto_Stop()) < 0)
            {
                LOGE("SmartProto_Stop failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_SMARTPROTOCOL_STOP_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_SMARTPROTOCOL_STOP_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_SMARTPROTOCOL_RESTART:
        {
            prctl(PR_SET_NAME,"SmartProto_Restart");
            if ((ret = SmartProto_Restart()) < 0)
            {
                LOGE("SmartProto_Restart failed %d\n",ret);
            }
            break;
        }
#endif
        /*case MEDIA_ASYNC_TASK_WS_INIT:
        {
            prctl(PR_SET_NAME,"WsMgr_Init");
            if ((ret = WsMgr_Init(s_media_ct.mqHandle)) < 0)
            {
                LOGE("WsMgr_Init failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_WS_INIT_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_WS_INIT_DONE,NULL,0);
            }
            break;
        }
		case MEDIA_ASYNC_TASK_WS_START:
        {
            prctl(PR_SET_NAME,"SmartProto_Start");
            if ((ret = WsMgr_Start()) < 0)
            {
                LOGE("SmartProto_Start failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_WS_START_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_WS_START_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_WS_STOP:
        {
            prctl(PR_SET_NAME,"SmartProto_Stop");
            if ((ret = WsMgr_Stop()) < 0)
            {
                LOGE("SmartProto_Stop failed %d\n",ret);
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_WS_STOP_FAIL,NULL,0);
            }
            else
            {
                Mq_PostMsg(s_media_ct.mqHandle,MEDIA_MSG_WS_STOP_DONE,NULL,0);
            }
            break;
        }
        case MEDIA_ASYNC_TASK_WS_RESTART:
        {
            prctl(PR_SET_NAME,"SmartProto_Restart");
            if ((ret = WsMgr_Restart()) < 0)
            {
                LOGE("SmartProto_Restart failed %d\n",ret);
            }
            break;
        }*/
		case MEDIA_ASYNC_TASK_LIBRECORD_INIT:
		{
			prctl(PR_SET_NAME,"librecordsdk_init");
			if(s_recordfuncCb.init)
			{
				LOGD("call librecord_sdk_init\n");
				s_recordfuncCb.init();
			}
			break;
		}
    }
}

static void AsyncTaskAdd(int taskId, int delay)
{
    if (taskId < 0 || taskId > MEDIA_ASYNC_TASK_MAX)
        return;

    void *task = MEDIA_MALLOC(sizeof(int) * 2);
    memcpy(task, &taskId, sizeof(taskId));
    memcpy(task + sizeof(taskId), &delay, sizeof(delay));
    pthread_pool_add(s_media_ct.tpoolHandle, AsyncTaskDoThread, (void *) task);
}

static int MediaCfgControl(void *cfg, int type)
{
    Common_cJSON_T *cfgFileJson = s_media_ct.cfgFileJson;
    int ret = 0;

    if (cfg == NULL || type < 0)
    {
        LOGE("%s type %d\n",strerror(EINVAL),type);
        return -1;
    }

    switch (type)
    {
        /*RTSP*/
        case CFG_CONTROL_RTSP_GET_CFG:
        {
            Common_cJSON_T *rtspJson = NULL;

            rtspJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.Rtsp");
            if (rtspJson == NULL)
            {
                LOGE("load rtsp cfg failed\n");
                ret = -1;
                break;
            }
            if (RtspJsonToCfg(rtspJson,(RTSP_CONFIG_T *)cfg) < 0)
            {
                ret = -1;
                break;
            }
            break;
        }
        case CFG_CONTROL_RTSP_GET_CFG_JSON:
        {
            Common_cJSON_T *rtspJson = NULL, *tpJson = NULL;

            rtspJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.Rtsp");
            if (rtspJson == NULL)
            {
                LOGE("load rtsp cfg failed\n");
                if (cfgFileJson == NULL)
                    LOGE("cfgFileJson is NULL\n");
                DumpJson(cfgFileJson);
                ret = -1;
                break;
            }

            tpJson = Common_cJSON_Duplicate(rtspJson,1);
            Common_cJSON_AddItemToObject((Common_cJSON_T *)cfg,"Rtsp",tpJson);
            break;
        }
        case CFG_CONTROL_RTSP_SET_CFG_JSON:
        {
            Common_cJSON_T *rtspJson = NULL, *tpJson = NULL;
            rtspJson = Common_cJSON_GetObjectItem((Common_cJSON_T *)cfg,"Rtsp");
            if (rtspJson == NULL)
            {
                LOGE("load rtsp cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.Rtsp");
            if (tpJson == NULL)
            {
                LOGE("load rtsp cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,rtspJson,0);
            if (ret < 0)
            {
                LOGE("merge cfg failed\n");
                ret = -1;
                break;
            }
            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
//            Timer_Start(MEDIA_TIMER_SAVE_CONFIG,1000);
            Timer_Start(MEDIA_TIMER_RESTART_RTSP,100);
            break;
        }
        /*RTMP*/
        case CFG_CONTROL_RTMP_GET_CFG:
        {
            Common_cJSON_T *rtmpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.Rtmp");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }
            if (RtmpJsonToCfg(rtmpJson,(RTMP_CONFIG_T *)cfg) < 0)
            {
                ret = -1;
                break;
            }
            break;
        }
        case CFG_CONTROL_RTMP_GET_CFG_JSON:
        {
            Common_cJSON_T *rtmpJson = NULL, *tpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.Rtmp");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = Common_cJSON_Duplicate(rtmpJson,1);
            Common_cJSON_AddItemToObject((Common_cJSON_T *)cfg,"Rtmp",tpJson);
            break;
        }
        case CFG_CONTROL_RTMP_SET_CFG_JSON:
        {
            Common_cJSON_T *rtmpJson = NULL, *tpJson = NULL;
            rtmpJson = Common_cJSON_GetObjectItem((Common_cJSON_T *)cfg,"Rtmp");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.Rtmp");
            if (tpJson == NULL)
            {
                LOGE("load rtsp cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,rtmpJson,0);
            if (ret < 0)
            {
                LOGE("merge cfg failed\n");
                ret = -1;
                break;
            }
            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
//            Timer_Start(MEDIA_TIMER_SAVE_CONFIG,1000);
            Timer_Start(MEDIA_TIMER_RESTART_RTMP,100);
            break;
        }
#ifdef RTMP
         /*RTMPPUSH*/
        case CFG_CONTROL_RTMPPUSH_GET_CFG:
        {
            Common_cJSON_T *rtmpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.RtmpPush");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }
            if (RtmpPushJsonToCfg(rtmpJson,(RTMPPUSH_CONFIG_T *)cfg) < 0)
            {
                ret = -1;
                break;
            }
            break;
        }
        case CFG_CONTROL_RTMPPUSH_GET_CFG_JSON:
        {
            Common_cJSON_T *rtmpJson = NULL, *tpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.RtmpPush");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmppush cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = Common_cJSON_Duplicate(rtmpJson,1);
            Common_cJSON_AddItemToObject((Common_cJSON_T *)cfg,"RtmpPush",tpJson);
            break;
        }
        case CFG_CONTROL_RTMPPUSH_SET_CFG_JSON:
        {
            Common_cJSON_T *rtmpJson = NULL, *tpJson = NULL;
            rtmpJson = Common_cJSON_GetObjectItem((Common_cJSON_T *)cfg,"RtmpPush");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmppush cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.RtmpPush");
            if (tpJson == NULL)
            {
                LOGE("load rtmppush cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,rtmpJson,0);
            if (ret < 0)
            {
                LOGE("merge rtmppush cfg failed\n");
                ret = -1;
                break;
            }
            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
//            Timer_Start(MEDIA_TIMER_SAVE_CONFIG,1000);
            Timer_Start(MEDIA_TIMER_RESTART_RTMPPUSH,100);
            break;
        }
         /*RTMPPUSH NotDisturb*/
        case CFG_CONTROL_RTMPPUSH_NOTDISTURB_GET_CFG:
        {
            Common_cJSON_T *rtmpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.RtmpPushNotDisturb");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }
            if (RtmpPushNotDisturbJsonToCfg(rtmpJson,(RTMPPUSH_NOTDISTURB_CONFIG_T *)cfg) < 0)
            {
                ret = -1;
                break;
            }
            break;
        }
        case CFG_CONTROL_RTMPPUSH_NOTDISTURB_GET_CFG_JSON:
        {
            Common_cJSON_T *rtmpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.RtmpPushNotDisturb");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmppush notdisturb cfg failed\n");
                ret = -1;
                break;
            }

            JsonOper_MergeObj((Common_cJSON_T *)cfg,rtmpJson,0);
            break;
        }
        case CFG_CONTROL_RTMPPUSH_NOTDISTURB_SET_CFG_JSON:
        {
            Common_cJSON_T *tpJson = NULL;

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.RtmpPushNotDisturb");
            if (tpJson == NULL)
            {
                LOGE("load rtmppush notdisturb cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,(Common_cJSON_T *)cfg,0);
            if (ret < 0)
            {
                LOGE("merge rtmppush cfg failed\n");
                ret = -1;
                break;
            }

            int i=0, j=0, i_num=0;
            char pathname[128] = {0};
            for(i=0; i<MAX_DAYS; i++)
            {
                for(j=0; j<MAX_TIMESEGMENT; j++)
                {
                    snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Start", i, j);
                    if(Common_Json_GetAttrValueInt(cfg, pathname, &i_num))
                    {
                        Common_Json_SetAttrValueInt(tpJson, pathname, i_num);
                    }
                    snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Stop", i, j);
                    if(Common_Json_GetAttrValueInt(cfg, pathname, &i_num))
                    {
                        Common_Json_SetAttrValueInt(tpJson, pathname, i_num);
                    }
                }
            }

            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
//            Timer_Start(MEDIA_TIMER_SAVE_CONFIG,1000);
//            Timer_Start(MEDIA_TIMER_RESTART_RTMPPUSH,100);
            break;
        }
#endif
        /*MEDIA DISCONNECT */
        case CFG_CONTROL_DISCON_GET_CFG:
        {
            Common_cJSON_T *disconJson = NULL;

            disconJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.MediaDisconnect");
            if (disconJson == NULL)
            {
                LOGE("load media disconnect cfg failed\n");
                ret = -1;
                break;
            }
            if (DisconnectJsonToCfg(disconJson,(MEDIA_DISCONNECT_INFO_T *)cfg) < 0)
            {
                ret = -1;
                break;
            }

            break;
        }
        case CFG_CONTROL_DISCON_GET_CFG_JSON:
        {
            Common_cJSON_T *disconJson = NULL, *tpJson = NULL;

            disconJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.MediaDisconnect");
            if (disconJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = Common_cJSON_Duplicate(disconJson,1);
            Common_cJSON_AddItemToObject((Common_cJSON_T *)cfg,"MediaDisconnect",tpJson);
            break;
        }
        case CFG_CONTROL_DISCON_SET_CFG_JSON:
        {
            Common_cJSON_T *disconJson = NULL, *tpJson = NULL;
            disconJson = Common_cJSON_GetObjectItem((Common_cJSON_T *)cfg,"MediaDisconnect");
            if (disconJson == NULL)
            {
                LOGE("load disconnect cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.MediaDisconnect");
            if (tpJson == NULL)
            {
                LOGE("load mediadisconnect cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,disconJson,0);
            if (ret < 0)
            {
                LOGE("merge cfg failed\n");
                ret = -1;
                break;
            }
            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
            break;
        }
#ifdef WITH_CURL
		/*HttpPush*/
		case CFG_CONTROL_SMARTPROTOCOL_GET_CFG:
        {
            Common_cJSON_T *HttpPushJson = NULL;

            HttpPushJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.SmartProtocol");
            if (HttpPushJson == NULL)
            {
                LOGE("load HttpPush cfg failed\n");
                ret = -1;
                break;
            }
            if (SmartProtocolJsonToCfg(HttpPushJson,(HTTP_PUSH_CFG_T*)cfg) < 0)
            {
                ret = -1;
                break;
            }
            break;
        }
		case CFG_CONTROL_SMARTPROTOCOL_GET_CFG_JSON:
        {
            Common_cJSON_T *HttpPushJson = NULL, *tpJson = NULL;

            HttpPushJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.SmartProtocol");
            if (HttpPushJson == NULL)
            {
                LOGE("load HttpPush cfg failed\n");
                if (cfgFileJson == NULL)
                    LOGE("cfgFileJson is NULL\n");
                DumpJson(cfgFileJson);
                ret = -1;
                break;
            }

            tpJson = Common_cJSON_Duplicate(HttpPushJson,1);
            Common_cJSON_AddItemToObject((Common_cJSON_T *)cfg,"SmartProtocol",tpJson);
            break;
        }
        case CFG_CONTROL_SMARTPROTOCOL_SET_CFG_JSON:
        {
            Common_cJSON_T *HttpPushJson = NULL, *tpJson = NULL;
            HttpPushJson = Common_cJSON_GetObjectItem((Common_cJSON_T *)cfg,"SmartProtocol");
            if (HttpPushJson == NULL)
            {
                LOGE("load HttpPush cfg failed\n");
                ret = -1;
                break;
            }

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.SmartProtocol");
            if (tpJson == NULL)
            {
                LOGE("load HttpPush cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,HttpPushJson,0);
            if (ret < 0)
            {
                LOGE("merge cfg failed\n");
                ret = -1;
                break;
            }
            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
            Timer_Start(MEDIA_TIMER_RESTART_SMARTPROTOCOL,100);
            break;
        }
         /*RTMPPUSH NotDisturb*/
        case CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_GET_CFG:
        {
            Common_cJSON_T *rtmpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.SmartProtocolNotDisturb");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmp cfg failed\n");
                ret = -1;
                break;
            }
            if (SmartProtocolNotDisturbJsonToCfg(rtmpJson,(SMART_PROTO_NOTDISTURB_CFG_T*)cfg) < 0)
            {
                ret = -1;
                break;
            }
            break;
        }
        case CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_GET_CFG_JSON:
        {
            Common_cJSON_T *rtmpJson = NULL;

            rtmpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.SmartProtocolNotDisturb");
            if (rtmpJson == NULL)
            {
                LOGE("load rtmppush notdisturb cfg failed\n");
                ret = -1;
                break;
            }

            JsonOper_MergeObj((Common_cJSON_T *)cfg,rtmpJson,0);
            break;
        }
        case CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_SET_CFG_JSON:
        {
            Common_cJSON_T *tpJson = NULL;

            tpJson = JsonOper_GetObjectItemByPath(cfgFileJson,"MediaServer.SmartProtocolNotDisturb");
            if (tpJson == NULL)
            {
                LOGE("load rtmppush notdisturb cfg failed\n");
                ret = -1;
                break;
            }

            ret = JsonOper_MergeObj(tpJson,(Common_cJSON_T *)cfg,0);
            if (ret < 0)
            {
                LOGE("merge rtmppush cfg failed\n");
                ret = -1;
                break;
            }

            int i=0, j=0, i_num=0;
            char pathname[128] = {0};
            for(i=0; i<MAX_DAYS; i++)
            {
                for(j=0; j<MAX_TIMESEGMENT; j++)
                {
                    snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Start", i, j);
                    if(Common_Json_GetAttrValueInt(cfg, pathname, &i_num))
                    {
                        Common_Json_SetAttrValueInt(tpJson, pathname, i_num);
                    }
                    snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Stop", i, j);
                    if(Common_Json_GetAttrValueInt(cfg, pathname, &i_num))
                    {
                        Common_Json_SetAttrValueInt(tpJson, pathname, i_num);
                    }
                }
            }

            RestMedia_SaveCfg(s_media_ct.cfgFileJson);
            Timer_Start(MEDIA_TIMER_RESTART_SMARTPROTOCOL,100);
            break;
        }
#endif
        default:
        {
            LOGE("not found this type\n");
            break;
        }
    }

    return ret;
}

#ifdef WITH_CURL
static void HandleMsgDataHttpPush(unsigned int msgId, void *data, unsigned int size)
{
    switch (msgId)
    {
        case MEDIA_MSG_SMARTPROTOCOL_INIT_DONE:
        {
            LOGI("MEDIA_MSG_SMARTPROTOCOL_INIT_DONE\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_SMARTPROTOCOL_START,0);
            break;
        }
        case MEDIA_MSG_SMARTPROTOCOL_INIT_FAIL:
        {
            LOGI("MEDIA_MSG_SMARTPROTOCOL_INIT_FAIL\n");
            break;
        }
        case MEDIA_MSG_SMARTPROTOCOL_START_DONE:
        {
            LOGI("MEDIA_MSG_SMARTPROTOCOL_START_DONE\n");
            break;
        }
        case MEDIA_MSG_SMARTPROTOCOL_START_FAIL:
        {
            LOGI("MEDIA_MSG_SMARTPROTOCOL_START_FAIL\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_SMARTPROTOCOL_START,1000);
            break;
        }
        case MEDIA_MSG_SMARTPROTOCOL_STOP_DONE:
        {
            LOGI("MEDIA_MSG_SMARTPROTOCOL_STOP_DONE\n");
            break;
        }
        case MEDIA_MSG_SMARTPROTOCOL_STOP_FAIL:
        {
            LOGI("MEDIA_MSG_SMARTPROTOCOL_STOP_FAIL\n");
            break;
        }
    }
}
#endif
/*static void HandleMsgDataWs(unsigned int msgId, void *data, unsigned int size)
{
    switch (msgId)
    {
        case MEDIA_MSG_WS_INIT_DONE:
        {
            LOGI("MEDIA_MSG_WS_INIT_DONE\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_WS_START,0);
            break;
        }
        case MEDIA_MSG_WS_INIT_FAIL:
        {
            LOGI("MEDIA_MSG_WS_INIT_FAIL\n");
            break;
        }
        case MEDIA_MSG_WS_START_DONE:
        {
            LOGI("MEDIA_MSG_WS_START_DONE\n");
            break;
        }
        case MEDIA_MSG_WS_START_FAIL:
        {
            LOGI("MEDIA_MSG_WS_START_FAIL\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_WS_START,1000);
            break;
        }
        case MEDIA_MSG_WS_STOP_DONE:
        {
            LOGI("MEDIA_MSG_WS_STOP_DONE\n");
            break;
        }
        case MEDIA_MSG_WS_STOP_FAIL:
        {
            LOGI("MEDIA_MSG_WS_STOP_FAIL\n");
            break;
        }
    }
}*/

static void HandleMsgDataRtsp(unsigned int msgId, void *data, unsigned int size)
{
    switch (msgId)
    {
        case MEDIA_MSG_RTSP_SERVER_INIT_DONE:
        {
            LOGI("MEDIA_MSG_RTSP_SERVER_INIT_DONE\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTSP_START,0);
            break;
        }
        case MEDIA_MSG_RTSP_SERVER_INIT_FAIL:
        {
            LOGI("MEDIA_MSG_RTSP_SERVER_INIT_FAIL\n");
            break;
        }
        case MEDIA_MSG_RTSP_SERVER_START_DONE:
        {
            LOGI("MEDIA_MSG_RTSP_SERVER_START_DONE\n");
            break;
        }
        case MEDIA_MSG_RTSP_SERVER_START_FAIL:
        {
            LOGI("MEDIA_MSG_RTSP_SERVER_START_FAIL\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTSP_START,1000);
            break;
        }
        case MEDIA_MSG_RTSP_SERVER_STOP_DONE:
        {
            LOGI("MEDIA_MSG_RTSP_SERVER_STOP_DONE\n");
            break;
        }
        case MEDIA_MSG_RTSP_SERVER_STOP_FAIL:
        {
            LOGI("MEDIA_MSG_RTSP_SERVER_STOP_FAIL\n");
            break;
        }
        case MEDIA_MSG_RTSP_STREAM_OPEN:
        {
            LOGI("MEDIA_MSG_RTSP_STREAM_OPEN\n");
            break;
        }
        case MEDIA_MSG_RTSP_STREAM_CLOSE:
        {
            LOGI("MEDIA_MSG_RTSP_STREAM_CLOSE\n");
            break;
        }
    }
}

static void HandleMsgDataRtmp(unsigned int msgId, void *data, unsigned int size)
{
    switch (msgId)
    {
        case MEDIA_MSG_RTMP_SERVER_INIT_DONE:
        {
            LOGI("MEDIA_MSG_RTMP_SERVER_INIT_DONE\n");
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTMP_START,0);
#ifdef RTMP
			AsyncTaskAdd(MEDIA_ASYNC_TASK_RTMPPUSH_INIT, 0);
#endif
            break;
        }
        case MEDIA_MSG_RTMP_SERVER_INIT_FAIL:
        {
            LOGI("MEDIA_MSG_RTMP_SERVER_INIT_FAIL\n");
            break;
        }
        case MEDIA_MSG_RTMP_SERVER_START_DONE:
        {
            LOGI("MEDIA_MSG_RTMP_SERVER_START_DONE\n");
            break;
        }
        case MEDIA_MSG_RTMP_SERVER_START_FAIL:
        {
            LOGI("MEDIA_MSG_RTMP_SERVER_START_FAIL\n");
            break;
        }
        case MEDIA_MSG_RTMP_SERVER_STOP_DONE:
        {
            LOGI("MEDIA_MSG_RTMP_SERVER_STOP_DONE\n");
            break;
        }
        case MEDIA_MSG_RTMP_SERVER_STOP_FAIL:
        {
            LOGI("MEDIA_MSG_RTMP_SERVER_STOP_FAIL\n");
            break;
        }

        case MEDIA_MSG_RTMP_STREAM_OPEN:
        {
            LOGI("MEDIA_MSG_RTMP_STREAM_OPEN\n");
            break;
        }
        case MEDIA_MSG_RTMP_STREAM_CLOSE:
        {
            LOGI("MEDIA_MSG_RTMP_STREAM_CLOSE\n");
            break;
        }
    }
}

static void HandleMsgData(unsigned int msgId, void *data, unsigned int size)
{
    switch (msgId)
    {
        case MEDIA_MSG_REST_INIT_DONE:
        {
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTSP_INIT, 0);
#ifdef RTMP
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTMP_INIT, 0);
#endif
#ifdef WITH_CURL
			AsyncTaskAdd(MEDIA_ASYNC_TASK_SMARTPROTOCOL_INIT, 0);
#endif
            //AsyncTaskAdd(MEDIA_ASYNC_TASK_WS_INIT, 0);

            break;
        }
        case MEDIA_MSG_REST_INIT_FAIL:
        {
            AsyncTaskAdd(MEDIA_ASYNC_TASK_REST_INIT, 1000);
            break;
        }
        case MEDIA_MSG_REST_SUBSCRIBE_CHECK:
        {
            break;
        }
        default:
            HandleMsgDataRtsp(msgId, data, size);
#ifdef RTMP
            HandleMsgDataRtmp(msgId, data, size);
#endif
#ifdef WITH_CURL
			HandleMsgDataHttpPush(msgId, data, size);
#endif
            //HandleMsgDataWs(msgId, data, size);

            break;
    }
}

static void HandleTimerData(unsigned int timerId)
{
    switch(timerId)
    {
        case MEDIA_TIMER_SYSTEM_INFO:
            LOGI("MEDIA_TIMER_SYSTEM_INFO %s %s\n",__DATE__,__TIME__);
            break;
		case MEDIA_TIMER_LIBRECORD_INIT:
		{
			AsyncTaskAdd(MEDIA_ASYNC_TASK_LIBRECORD_INIT,0);
			Timer_Stop(MEDIA_TIMER_LIBRECORD_INIT);
			break;
		}
        case MEDIA_TIMER_SAVE_CONFIG:
        {
            LOGI("MEDIA_TIMER_SAVE_CONFIG\n");
            if (RestMedia_SaveCfg(s_media_ct.cfgFileJson) == 0)
            {
                Timer_Stop(MEDIA_TIMER_SAVE_CONFIG);
            }
            break;
        }
        case MEDIA_TIMER_RESTART_RTSP:
        {
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTSP_RESTART,0);
            Timer_Stop(MEDIA_TIMER_RESTART_RTSP);
            break;
        }
        case MEDIA_TIMER_RESTART_RTMP:
        {
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTMP_RESTART,0);
            Timer_Stop(MEDIA_TIMER_RESTART_RTMP);
            break;
        }
#ifdef RTMP
		case MEDIA_TIMER_RESTART_RTMPPUSH:
        {
            AsyncTaskAdd(MEDIA_ASYNC_TASK_RTMPPUSH_RESTART,0);
            Timer_Stop(MEDIA_TIMER_RESTART_RTMPPUSH);
            break;
        }
#endif
#ifdef WITH_CURL
		case MEDIA_TIMER_RESTART_SMARTPROTOCOL:
        {
            AsyncTaskAdd(MEDIA_ASYNC_TASK_SMARTPROTOCOL_RESTART,0);
            Timer_Stop(MEDIA_TIMER_RESTART_SMARTPROTOCOL);
            break;
        }
#endif
    }
}

static int HandleReqAppData(unsigned int id, void *reqInData, unsigned int reqInSize, int *reqRet, void *reqOutData, unsigned int reqOutSize, int setOrGet)
{
    if (setOrGet == 0)//get app data
    {
        MEDIA_ALARM_STATUS_T *out = reqOutData;
        if(reqOutSize != sizeof(MEDIA_ALARM_STATUS_T))
        {
            LOGE("request size error");
            *reqRet = -1;
            return -1;
        }

        out->alarmStatusCnt = s_media_ct.alarmStatusInfo.alarmStatusCnt;

        /*TODO 此处分配内存，请求函数得到结果后需要释放内存*/
        if (out->alarmStatusCnt > 0)
            out->alarmStatus = MEDIA_MALLOC(sizeof(MEDIA_ALARM_STATUS_NODE_T) * out->alarmStatusCnt);
        else
            out->alarmStatus = NULL;

        if (out->alarmStatus)
        {
            memcpy(out->alarmStatus,s_media_ct.alarmStatusInfo.alarmStatus,sizeof(MEDIA_ALARM_STATUS_NODE_T) * out->alarmStatusCnt);
        }
        *reqRet = 0;
        return 0;
    }
    else // set app data
    {
        MEDIA_ALARM_STATUS_T *in = reqInData;
         int i = 0;
         if(reqInSize != sizeof(MEDIA_ALARM_STATUS_T) || in->alarmStatusCnt <= 0)
         {
             LOGE("param error\n");
             *reqRet = -1;
             return -1;
         }

         if (s_media_ct.alarmStatusInfo.alarmStatusCnt < in->alarmStatusCnt)
         {
             if (s_media_ct.alarmStatusInfo.alarmStatus)
             {
                 MEDIA_FREE(s_media_ct.alarmStatusInfo.alarmStatus);
                 s_media_ct.alarmStatusInfo.alarmStatus = NULL;
             }
             s_media_ct.alarmStatusInfo.alarmStatus = MEDIA_MALLOC(sizeof(MEDIA_ALARM_STATUS_NODE_T) * in->alarmStatusCnt);
             s_media_ct.alarmStatusInfo.alarmStatusCnt = in->alarmStatusCnt;
             memcpy(s_media_ct.alarmStatusInfo.alarmStatus,in->alarmStatus,sizeof(MEDIA_ALARM_STATUS_NODE_T) * in->alarmStatusCnt);
         }
         else if (in->alarmStatusCnt == s_media_ct.alarmStatusInfo.alarmStatusCnt) //every 60 sec
         {
             memcpy(s_media_ct.alarmStatusInfo.alarmStatus,in->alarmStatus,sizeof(MEDIA_ALARM_STATUS_NODE_T) * in->alarmStatusCnt);
         }
         else if (in->alarmStatusCnt == 1 && s_media_ct.alarmStatusInfo.alarmStatusCnt > 1)// one event report
         {
             for (i = 0;i<s_media_ct.alarmStatusInfo.alarmStatusCnt;i++)
             {
                 if (strcmp(s_media_ct.alarmStatusInfo.alarmStatus[i].alarmName,in->alarmStatus->alarmName) == 0
                                 && s_media_ct.alarmStatusInfo.alarmStatus[i].channel == in->alarmStatus->channel
                                 && s_media_ct.alarmStatusInfo.alarmStatus[i].regionId == in->alarmStatus->regionId)
                 {
                     memcpy(&s_media_ct.alarmStatusInfo.alarmStatus[i],in->alarmStatus,sizeof(MEDIA_ALARM_STATUS_NODE_T));
                 }
             }
         }
         else
         {
             LOGE("unknow case , incoming status count %d , cache status count %d\n",in->alarmStatusCnt,s_media_ct.alarmStatusInfo.alarmStatusCnt);
         }
         *reqRet = 0;
         return 0;
    }

    return 0;
}

static void HandleReqData(unsigned int id, void *reqInData, unsigned int reqInSize, int *reqRet, void *reqOutData, unsigned int reqOutSize)
{
    *reqRet = -1;
    switch(id)
    {
        case MEDIA_REQ_MEDIA_CFG_LOAD:
        {
            *reqRet = RestMedia_LoadCfg(&s_media_ct.cfgFileJson);
            break;
        }
        case MEDIA_REQ_RTSP_GET_CFG:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTSP_GET_CFG);
            break;
        }
        case MEDIA_REQ_RTSP_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTSP_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_RTSP_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_RTSP_SET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_RTMP_GET_CFG:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTMP_GET_CFG);
            break;
        }
        case MEDIA_REQ_RTMP_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTMP_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_RTMP_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_RTMP_SET_CFG_JSON);
            break;
        }
#ifdef RTMP
        case MEDIA_REQ_RTMPPUSH_GET_CFG:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTMPPUSH_GET_CFG);
            break;
        }
        case MEDIA_REQ_RTMPPUSH_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTMPPUSH_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_RTMPPUSH_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_RTMPPUSH_SET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTMPPUSH_NOTDISTURB_GET_CFG);
            break;
        }
        case MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_RTMPPUSH_NOTDISTURB_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_RTMPPUSH_NOTDISTURB_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_RTMPPUSH_NOTDISTURB_SET_CFG_JSON);
            break;
        }
#endif
        case MEDIA_REQ_RESTORE_CFG:
        {
            LOGW("restore cfg\n");
            if (RestMedia_RestoreCfg(&s_media_ct.cfgFileJson) == 0)
                *reqRet = 0;
            else
                *reqRet = -1;
            break;
        }
        case MEDIA_REQ_RTSP_GET_APP_DATA:
        {
            HandleReqAppData(id, reqInData, reqInSize, reqRet, reqOutData, reqOutSize, 0);
            break;
        }
        case MEDIA_REQ_RTSP_SET_APP_DATA:
        {
            HandleReqAppData(id, reqInData, reqInSize, reqRet, reqOutData, reqOutSize, 1);
            break;
        }
        case MEDIA_REQ_GET_BOARD_STATE:
        {
            memcpy(reqOutData, &s_media_ct.boardState, sizeof(s_media_ct.boardState));
            *reqRet = 0;
            break;
        }
        case MEDIA_REQ_SET_BOARD_STATE:
        {
            memcpy(&s_media_ct.boardState, reqInData,sizeof(s_media_ct.boardState));
            *reqRet = 0;
            break;
        }
        case MEDIA_REQ_DISCON_GET_CFG:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_DISCON_GET_CFG);
            break;
        }
        case MEDIA_REQ_DISCON_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_DISCON_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_DISCON_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_DISCON_SET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_DISCON_GET_EVENT:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_DISCON_GET_CFG_JSON);
            if (*reqRet == 0)
                Common_Json_SetAttrValue((cJSON_Struct *)reqOutData,-1, "MediaDisconnect/State", Common_Json_Type_Number,NULL, s_media_ct.disconnectEventState, 0);
            break;
        }
        case MEDIA_REQ_DISCON_SET_EVENT:
        {
            if (reqInData != NULL && (*(int *)reqInData == 0 || *(int *)reqInData == 1))
            {
                if (*(int *)reqInData != s_media_ct.disconnectEventState)
                {
                    s_media_ct.disconnectEventState = *(int *)reqInData;
                    *reqRet = 0;
                }
                else
                {
                    LOGW("same disconnect event state %d\n",s_media_ct.disconnectEventState);
                    *reqRet = -1;
                }
            }
            else
            {
                LOGE("request set event state value error\n");
                *reqRet = -1;
            }
            break;
        }
#ifdef WITH_CURL
		case MEDIA_REQ_SMARTPROTOCOL_GET_CFG:
		{
			*reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_SMARTPROTOCOL_GET_CFG);
			break;
		}
		case MEDIA_REQ_SMARTPROTOCOL_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_SMARTPROTOCOL_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_SMARTPROTOCOL_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_SMARTPROTOCOL_SET_CFG_JSON);
            break;
        }

        case MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_GET_CFG:
		{
			*reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_GET_CFG);
			break;
		}
		case MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_GET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqOutData,CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_GET_CFG_JSON);
            break;
        }
        case MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_SET_CFG_JSON:
        {
            *reqRet = MediaCfgControl(reqInData,CFG_CONTROL_SMARTPROTOCOL_NOTDISTURB_SET_CFG_JSON);
            break;
        }
#endif
    }
}

static void SignalHandle(int param)
{
    if (param == SIGINT)
    {
        LOGW("RECV SIGINT! now exit\n");
        Mq_QuitLoop(s_media_ct.mqHandle);
    }

    if (param == SIGPIPE)
    {
        LOGW("RECV SIGPIPE! \n");
    }
}

static int LoadExternalRecordLibs()
{
    void * libHdl  = NULL;

    libHdl  = dlopen("/root/lib/librecord_sdk.so", RTLD_LAZY);

    if(libHdl)
    {
        s_recordfuncCb.init  = (EXTER_RECORDSDK_INIT_F )dlsym(libHdl, "librecord_sdk_init");
        if(s_recordfuncCb.init == NULL)
        {
            LOGE("Not found librecord_sdk_init\n");
            return -1;
        }

        s_recordfuncCb.uninit  = (EXTER_RECORDSDK_UNINIT_F )dlsym(libHdl, "librecord_sdk_uninit");
        if(s_recordfuncCb.uninit == NULL)
        {
            LOGE("Not found librecord_sdk_uninit\n");
            return -1;
        }
    }
    else
    {
        LOGW("Not found librecord_sdk.so.\n");
        return -1;
    }

	LOGD("dlopen /root/lib/librecord_sdk.so succ\n");

    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, SignalHandle);
    signal(SIGPIPE, SignalHandle);
    memset(&s_media_ct, 0, sizeof(s_media_ct));

    prctl(PR_SET_NAME, "ovfs_mediaserver");
    Timer_SignalMask();

    LOG_INIT("MediaServer", COMMON_LOG_LV_HIGH);

    Common_RegistSigHandle(SIGSEGV);
    Common_RegistSigHandle(SIGILL);
    Common_RegistSigHandle(SIGABRT);

    if (Mq_Init(&s_media_ct.mqHandle, 0) < 0)
    {
        LOGE("mq init failed\n");
        goto exit_log;
    }

    if (Timer_Init(MEDIA_TIMER_MAX, s_media_ct.mqHandle) < 0)
    {
        LOGE("timer init failed\n");
        goto exit_mq;
    }

	LoadExternalRecordLibs();

    /*create threads pool*/
    s_media_ct.tpoolHandle = thread_pool_create(MEDIA_ASYNC_TASK_MAX, 0, PTHREAD_STACK_MIN*64);

    AsyncTaskAdd(MEDIA_ASYNC_TASK_REST_INIT, 0);

	Timer_Start(MEDIA_TIMER_LIBRECORD_INIT,3000);

    Timer_Start(MEDIA_TIMER_SYSTEM_INFO,20000);

    Mq_MsgLoop(s_media_ct.mqHandle, NULL, HandleTimerData, HandleMsgData, HandleReqData);

	if(s_recordfuncCb.uninit)
	{
		LOGD("call librecord_sdk_uninit\n");
		s_recordfuncCb.uninit();
	}

    RtspMgr_Stop();

#ifdef RTMP
    RtmpMgr_Stop();
#endif
    RestMedia_Uninit();
    pthread_pool_destroy(s_media_ct.tpoolHandle);
    Timer_Uninit();

    if (s_media_ct.cfgFileJson)
        Common_cJSON_Delete(s_media_ct.cfgFileJson);

    exit_mq:
    Mq_Uninit(&s_media_ct.mqHandle);
    exit_log:
    LOG_UNINIT();
    printf("exit normally\n");
    return 0;
}
