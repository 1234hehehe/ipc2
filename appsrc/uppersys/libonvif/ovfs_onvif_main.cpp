/*
 * ovfs_network_main.c
 *
 *  Created on: 2016骞?7鏈?21鏃?
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

#include <libcommon_api.h>

#include <common_json_str_ops.h>

#include "ovfs_onvif.h"
#include "ovfs_com_utils.h"
#include "third_protocol_api.h"

typedef struct
{
    void *pipeHandle;
    MQ_HANDLE_H mqHandle;
    pthread_pool_t *tpoolHandle;
    //cJSON_Struct *jsonCfg;
    ONVIF_MGR_CFG_T cfg;
    ONVIF_ALARM_STATUS_T alarmStatusInfo;
    void *asyncHandle;
    fCallParamFun fCallFxn;
} ONVIF_CONTEXT_T;

ONVIF_CONTEXT_T s_onvif_ct;

static void SignalHandle(int param)
{
    LOGW("recv signal %d\n", param);
    if (param == SIGINT)
    {
        LOGW("RECV SIGINT! now exit\n");
        Mq_QuitLoop(s_onvif_ct.mqHandle);
    }

    if (param == SIGPIPE)
    {
        LOGW("RECV SIGPIPE! \n");
    }
}

static void AsyncTaskHandle(int id, void *data, int size)
{
    int ret = 0;

    switch (id)
    {
    case ONVIF_ASYNC_TASK_REST_INIT:
    {
        prctl(PR_SET_NAME, "RestOnvif_Init");
        ret = RestOnvif_Init(s_onvif_ct.mqHandle);
        ret = (ret != 0) ? ONVIF_MSG_REST_INIT_FAIL : ONVIF_MSG_REST_INIT_DONE;

        break;
    }
    case ONVIF_ASYNC_TASK_MGR_INIT:
    {
        prctl(PR_SET_NAME, "OnvifMgr_Init");
        ret = OnvifMgr_Init(s_onvif_ct.mqHandle);
        ret = (ret != 0) ? ONVIF_MSG_MGR_INIT_FAIL : ONVIF_MSG_MGR_INIT_DONE;
        break;
    }
    case ONVIF_ASYNC_TASK_MGR_START:
    {
        prctl(PR_SET_NAME, "OnvifMgr_Start");
        ret = OnvifMgr_Start();
        ret = (ret != 0) ? ONVIF_MSG_MGR_START_FAIL : ONVIF_MSG_MGR_START_DONE;
        break;
    }
    case ONVIF_ASYNC_TASK_MGR_RESTART:
    {
        prctl(PR_SET_NAME, "OnvifMgr_Restart");
        ret = ONVIF_MSG_MGR_RESTART_DONE;
        OnvifMgr_Restart();
        break;
    }
    case ONVIF_ASYNC_TASK_EVENT_NOTIFY:
    {
        OnvifMgr_EventNotify();
        return;
    }
    case ONVIF_ASYNC_TASK_EVENT_SEND:
    {
        OnvifEvent_SendEvent(*(int *)data);
        return;
    }
    case ONVIF_ASYNC_TASK_MGR_GETCFG:
    {
        OnvifMgr_GetCfg();
        return;
    }
    case ONVIF_ASYNC_TASK_MGR_FIXEDIP:
    {
        OnvifMgr_FixedIp();
        return;
    }
    default:
    {
        LOGE("unknown task id %u\n", id);
        return;
    }
    }
    Mq_PostMsg(s_onvif_ct.mqHandle, (unsigned int)ret, NULL, 0);
}

static void HandleTimerData(unsigned int timerId)
{
    switch(timerId)
    {
        case ONVIF_TIMER_REGULAR_CHECK:
        {
            break;
        }
        case ONVIF_TIMER_EVENT_NOTIFY:
        {
            Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_EVENT_NOTIFY, 50, NULL, 0);
            break;
        }
        case ONVIF_TIMER_EVENT_CHECK:
        {
            OnvifMgr_EventCheck();
            break;
        }
        case ONVIF_TIMER_FIXEDIP_CHECK:
        {
            Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_MGR_FIXEDIP, 0, NULL, 0);
            break;
        }
    }
}
static int HandleReqAppData(unsigned int id, void *reqInData, unsigned int reqInSize, int *reqRet,
                            void *reqOutData, unsigned int reqOutSize, int setOrGet)
{
    if (setOrGet == 0)//get app data
    {
        ONVIF_ALARM_STATUS_T *out = (ONVIF_ALARM_STATUS_T *)reqOutData;
        if(reqOutSize != sizeof(ONVIF_ALARM_STATUS_T))
        {
            LOGE("request size error");
            *reqRet = -1;
            return -1;
        }

        out->alarmStatusCnt = s_onvif_ct.alarmStatusInfo.alarmStatusCnt;

        /*TODO 此处分配内存，请求函数得到结果后需要释放内存*/
        if (out->alarmStatusCnt > 0)
            out->alarmStatus = (ONVIF_ALARM_STATUS_NODE_T *)ONVIF_MALLOC(sizeof(ONVIF_ALARM_STATUS_NODE_T) *
                               out->alarmStatusCnt);
        else
            out->alarmStatus = NULL;

        if (out->alarmStatus)
        {
            memcpy(out->alarmStatus, s_onvif_ct.alarmStatusInfo.alarmStatus,
                   sizeof(ONVIF_ALARM_STATUS_NODE_T) * out->alarmStatusCnt);
        }
        *reqRet = 0;
        return 0;
    }
    else // set app data
    {
        ONVIF_ALARM_STATUS_T *in = (ONVIF_ALARM_STATUS_T *)reqInData;
        int i = 0;
        if (reqInSize != sizeof(ONVIF_ALARM_STATUS_T) || in->alarmStatusCnt <= 0)
        {
            LOGE("param error\n");
            *reqRet = -1;
            return -1;
        }

        if (s_onvif_ct.alarmStatusInfo.alarmStatusCnt < in->alarmStatusCnt)
        {
            if (s_onvif_ct.alarmStatusInfo.alarmStatus)
            {
                ONVIF_FREE(s_onvif_ct.alarmStatusInfo.alarmStatus);
                s_onvif_ct.alarmStatusInfo.alarmStatus = NULL;
            }
            s_onvif_ct.alarmStatusInfo.alarmStatus = (ONVIF_ALARM_STATUS_NODE_T *)ONVIF_MALLOC(
                        sizeof(ONVIF_ALARM_STATUS_NODE_T) * in->alarmStatusCnt);
            s_onvif_ct.alarmStatusInfo.alarmStatusCnt = in->alarmStatusCnt;
            memcpy(s_onvif_ct.alarmStatusInfo.alarmStatus, in->alarmStatus,
                   sizeof(ONVIF_ALARM_STATUS_NODE_T) * in->alarmStatusCnt);
        }
        else if (in->alarmStatusCnt == s_onvif_ct.alarmStatusInfo.alarmStatusCnt) //every 60 sec
        {
            memcpy(s_onvif_ct.alarmStatusInfo.alarmStatus, in->alarmStatus,
                   sizeof(ONVIF_ALARM_STATUS_NODE_T) * in->alarmStatusCnt);
        }
        else if (in->alarmStatusCnt == 1
                 && s_onvif_ct.alarmStatusInfo.alarmStatusCnt > 1)// one event report
        {
            for (i = 0; i < s_onvif_ct.alarmStatusInfo.alarmStatusCnt; i++)
            {
                if (strcmp(s_onvif_ct.alarmStatusInfo.alarmStatus[i].alarmName, in->alarmStatus->alarmName) == 0
                        && s_onvif_ct.alarmStatusInfo.alarmStatus[i].channel == in->alarmStatus->channel
                        && s_onvif_ct.alarmStatusInfo.alarmStatus[i].regionId == in->alarmStatus->regionId)
                {
                    memcpy(&s_onvif_ct.alarmStatusInfo.alarmStatus[i], in->alarmStatus,
                           sizeof(ONVIF_ALARM_STATUS_NODE_T));
                }
            }

            if (strcmp(in->alarmStatus->alarmName, "Motion") == 0)
            {
                Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_EVENT_SEND, 0,
                                   &in->alarmStatus->status,
                                   sizeof(in->alarmStatus->status));
                // OnvifEvent_SendEvent(in->alarmStatus->status);
            }
        }
        else
        {
            LOGE("unknow case , incoming status count %d , cache status count %d\n", in->alarmStatusCnt,
                 s_onvif_ct.alarmStatusInfo.alarmStatusCnt);
        }
        *reqRet = 0;
        return 0;
    }

    return 0;
}

static void HandleReqData(unsigned int id, void *reqInData,
                          unsigned int reqInSize, int *reqRet, void *reqOutData,
                          unsigned int reqOutSize)
{
    int ret = 0;
    *reqRet = -1;

    switch (id)
    {
    case ONVIF_REQ_CFG_LOAD:
    {
        ret = RestOnvif_LoadCfg(&s_onvif_ct.cfg);
        *reqRet = ret;
        break;
    }
    case ONVIF_REQ_GET_CFG:
    {/*
        cJSON_Struct *cfg = NULL;
        if (reqInData == NULL)
        {
            cfg = Common_Json_Duplicate(s_onvif_ct.jsonCfg, 1);

            Common_Json_AddItem(reqOutData, -1, "Data", cfg);
            *reqRet = 0;
        }
        else
        {
            // DumpJson((Common_cJSON_T *)s_onvif_ct.jsonCfg);
            cfg = Common_Json_GetAttrValue(s_onvif_ct.jsonCfg, -1, (char *)reqInData, NULL,
                                           NULL, NULL, NULL);
            if (cfg != NULL)
            {
                Common_Json_AddItem(reqOutData, -1, NULL, Common_Json_Duplicate(cfg, 1));
                // DumpJson((Common_cJSON_T *)cfg);
                // cJSON_Struct *tmp = Common_Json_GetFirstChild(cfg);
                // do
                // {
                //     if (tmp == NULL)
                //         break;
                //     DumpJson((Common_cJSON_T *)tmp);
                //     Common_Json_AddItem(reqOutData, -1, NULL, Common_Json_Duplicate(tmp,1));
                //     tmp = Common_Json_GetNext(tmp);
                // } while (1);
                *reqRet = 0;
            }
            else
            {
                LOGW("%s cfg error\n", (char *)reqInData);
            }
        }*/
        break;
    }
    case ONVIF_REQ_SET_CFG:
    {/*
        ret = JsonOper_MergeObj((Common_cJSON_T *)s_onvif_ct.jsonCfg,
                                (Common_cJSON_T *)reqInData, 0);
        if (ret < 0)
        {
            LOGE("merge cfg failed\n");
            *reqRet = -1;
            break;
        }

        if (RestOnvif_SaveCfg((cJSON_Struct *)s_onvif_ct.jsonCfg) == 0)
        {
            RestOnvif_LoadCfg(&s_onvif_ct.jsonCfg);
            *reqRet = 0;
            LOGI("set config done\n");
            // DumpJson((Common_cJSON_T *)s_onvif_ct.jsonCfg);
            // Mq_PostMsg(s_onvif_ct.mqHandle, ONVIF_MSG_MGR_RESTART, NULL, 0);
        }
        else
        {
            LOGW("cfg error\n");
        }*/
        break;
    }
    case ONVIF_REQ_GET_CFG_STRUCT:
    {
        memcpy(reqOutData,&s_onvif_ct.cfg,sizeof(s_onvif_ct.cfg));
        //RestOnvif_CfgJsonToCfgStruct(s_onvif_ct.jsonCfg, reqOutData);
        *reqRet = 0;
        break;
    }
    case ONVIF_REQ_SET_CFG_STRUCT:
    {
        memcpy(&s_onvif_ct.cfg,reqInData,sizeof(s_onvif_ct.cfg));
        if (RestOnvif_SaveCfg(s_onvif_ct.cfg) == 0)
        {
            RestOnvif_LoadCfg(&s_onvif_ct.cfg);
            *reqRet = 0;
            LOGI("set config done\n");
        }
        else
        {
            LOGW("cfg error\n");
            *reqRet = -1;
        }
        break;
    }
    case ONVIF_REQ_RTSP_SET_APP_DATA:
    {
        HandleReqAppData(id, reqInData, reqInSize, reqRet, reqOutData, reqOutSize, 1);
        break;
    }
    case ONVIF_REQ_RTSP_GET_APP_DATA:
    {
        HandleReqAppData(id, reqInData, reqInSize, reqRet, reqOutData, reqOutSize, 0);
        break;
    }
    }
}

static void HandleMsgData(unsigned int msgId, void *data, unsigned int size)
{
    switch (msgId)
    {
    case ONVIF_MSG_REST_INIT_FAIL:
    {
        LOGW("ONVIF_MSG_REST_INIT_FAIL\n");
        Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_REST_INIT, 1000, NULL, 0);
        break;
    }
    case ONVIF_MSG_REST_INIT_DONE:
    {
        LOGW("ONVIF_MSG_REST_INIT_DONE\n");
        Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_MGR_INIT, 0, NULL, 0);
        break;
    }
    case ONVIF_MSG_MGR_INIT_FAIL:
    {
        LOGW("ONVIF_MSG_INIT_FAIL\n");
        Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_MGR_INIT, 1000, NULL, 0);
        break;
    }
    case ONVIF_MSG_MGR_INIT_DONE:
    {
        LOGW("ONVIF_MSG_MGR_INIT_DONE\n");
        Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_MGR_START, 0, NULL, 0);
        break;
    }
    case ONVIF_MSG_MGR_START_FAIL:
    {
        LOGW("ONVIF_MSG_MGR_START_FAIL\n");
        Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_MGR_START, 1000, NULL, 0);
        break;
    }
    case ONVIF_MSG_MGR_START_DONE:
    {
        LOGW("ONVIF_MSG_MGR_START_DONE\n");
        break;
    }
    case ONVIF_MSG_MGR_RESTART:
    {
        LOGW("ONVIF_MSG_MGR_RESTART\n");
        Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_MGR_RESTART, 0, NULL, 0);
        break;
    }
    }
}

static S32 onvif_main(Common_Thread_T hThreadHandle,void *pUserData)//int argc, char **argv
{

    prctl(PR_SET_NAME, "ovfs_onvifserver");

    Utils_AsyncTaskInit(&s_onvif_ct.asyncHandle, AsyncTaskHandle);
    if (Mq_Init(&s_onvif_ct.mqHandle, 0) < 0)
    {
        LOGE("mq init failed\n");
        goto exit_log;
    }

    if (Timer_Init(ONVIF_TIMER_MAX, s_onvif_ct.mqHandle) < 0)
    {
        LOGE("timer init failed\n");
        goto exit_mq;
    }

    /*create threads pool*/
    s_onvif_ct.tpoolHandle = thread_pool_create(ONVIF_ASYNC_TASK_MAX, 0,
                             PTHREAD_STACK_MIN * 4);

    Utils_AsyncTaskAdd(s_onvif_ct.asyncHandle, ONVIF_ASYNC_TASK_REST_INIT, 0, NULL , 0);

    //Timer_Start(ONVIF_TIMER_REGULAR_CHECK, 30000);
    Mq_MsgLoop(s_onvif_ct.mqHandle, NULL, HandleTimerData, HandleMsgData,
               HandleReqData);
LOGW("---------------onvif exit!\n");
    OnvifMgr_Uninit();
    RestOnvif_Uninit();
    pthread_pool_destroy(s_onvif_ct.tpoolHandle);
    Timer_Uninit();

exit_mq:
    Mq_Uninit(&s_onvif_ct.mqHandle);

exit_log:

    printf("exit normally\n");

    return 0;
}

Common_Thread_T pMainThreadHandle = NULL;

int THIRD_PROTOCOL_Init(fCallParamFun pCallParamFun)
{
    LOGW("ONVIF init\n");
    memset(&s_onvif_ct, 0, sizeof(s_onvif_ct));
    s_onvif_ct.fCallFxn = pCallParamFun;
    Common_Thread_Create(&pMainThreadHandle, "Onvif", 0, 0, onvif_main, NULL);

    return 0;
}

int THIRD_PROTOCOL_Uninit()
{
    return Common_Thread_Destroy(&pMainThreadHandle);
}

int alarmStatus = 0;
int THIRD_PROTOCOL_AlarmCallBack(THIRD_ALARM_INFO *pAlarmStatus, int nAlarmStatusCnt)
{
    //LOGD("THIRD_PROTOCOL_AlarmCallBack!\n");
    //LOGD("nAlarmStatusCnt:[%d]\n",nAlarmStatusCnt);
    LOGD("AlarmName:[%s][%d][%s][%s]\n",pAlarmStatus->AlarmName,pAlarmStatus->Status,pAlarmStatus->StartTime,pAlarmStatus->StopTime);

    if(strcmp(pAlarmStatus->AlarmName, "Motion") != 0)return 0;

    if(alarmStatus != pAlarmStatus->Status)
    {
        alarmStatus = pAlarmStatus->Status;
        OnvifEvent_SendEvent(alarmStatus);
    }

    int i = 0;
    int ret = 0;
    ONVIF_ALARM_STATUS_T alarmStatusInfo = { 0 };
    ONVIF_ALARM_STATUS_NODE_T *node = NULL;
    alarmStatusInfo.alarmStatusCnt = nAlarmStatusCnt;
    alarmStatusInfo.alarmStatus = (ONVIF_ALARM_STATUS_NODE_T *)ONVIF_MALLOC(sizeof(
                                      ONVIF_ALARM_STATUS_NODE_T) * nAlarmStatusCnt);

    memset(alarmStatusInfo.alarmStatus, 0, sizeof(ONVIF_ALARM_STATUS_NODE_T)*nAlarmStatusCnt);
    for (i = 0; i < nAlarmStatusCnt; i ++)
    {
        node = alarmStatusInfo.alarmStatus + i;
        Common_Strncpy(node->alarmName, pAlarmStatus->AlarmName, sizeof(node->alarmName));
        node->alarmType = pAlarmStatus->AlarmType;
        node->alarmSrcType = pAlarmStatus->AlarmSrcType;
        Common_Strncpy(node->devName, pAlarmStatus->DevName, sizeof(node->devName));
        node->device = pAlarmStatus->Device;
        node->channel = pAlarmStatus->Channel;
        node->stream = pAlarmStatus->Stream;
        node->regionId = pAlarmStatus->RegionId;
        node->status = pAlarmStatus->Status;
        sscanf(pAlarmStatus->StartTime, "%04d%02d%02d%02d%02d%02d",
                       &node->startTime.tm_year,
                       &node->startTime.tm_mon,
                       &node->startTime.tm_mday,
                       &node->startTime.tm_hour,
                       &node->startTime.tm_min,
                       &node->startTime.tm_sec);
        node->startTime.tm_year -= 1900;
        node->startTime.tm_mon -= 1;
        node->startTime.tm_isdst = -1;

        sscanf(pAlarmStatus->StopTime, "%04d%02d%02d%02d%02d%02d",
                       &node->stopTime.tm_year,
                       &node->stopTime.tm_mon,
                       &node->stopTime.tm_mday,
                       &node->stopTime.tm_hour,
                       &node->stopTime.tm_min,
                       &node->stopTime.tm_sec);
        node->stopTime.tm_year -= 1900;
        node->stopTime.tm_mon -= 1;
        node->stopTime.tm_isdst = -1;
    }

    Mq_Request(s_onvif_ct.mqHandle, ONVIF_REQ_RTSP_SET_APP_DATA, (void *)(&alarmStatusInfo),
               sizeof(ONVIF_ALARM_STATUS_T), &ret, NULL, 0);
    ONVIF_FREE(alarmStatusInfo.alarmStatus);

    return 0;
}

int THIRD_PROTOCOL_SetConfig(THIRD_ONVIF_CFG *cfg)
{
    int ret = 0;
    //LOGD("[%d][%d][%d][%d][%d][%s]\n",cfg->UseMask,cfg->AuthEnable,cfg->AdaptiveIp,cfg->Timeout,
    //    cfg->FixedIp,cfg->FixedIpAddr);

    if(cfg->UseMask)
    {
        if(cfg->UseMask & 0x1)
        {
            s_onvif_ct.cfg.authEnable = cfg->AuthEnable;
        }

        if(cfg->UseMask & 0x2)
        {
            s_onvif_ct.cfg.adaptiveIp = cfg->AdaptiveIp;
        }

        if(cfg->UseMask & 0x4)
        {
            s_onvif_ct.cfg.timeout = cfg->Timeout;
        }

        if(cfg->UseMask & 0x8)
        {
            s_onvif_ct.cfg.fixedIp = cfg->FixedIp;
        }

        if(cfg->UseMask & 0x10)
        {
            //Common_Strncpy(s_onvif_ct.cfg.fixedIpAddr, cfg->FixedIpAddr, strlen(cfg->FixedIpAddr));
            snprintf(s_onvif_ct.cfg.fixedIpAddr,sizeof(s_onvif_ct.cfg.fixedIpAddr),"%s",cfg->FixedIpAddr);
        }

        OnvifMgr_GetCfg();
    }

    return 0;
}

int THIRD_PROTOCOL_DealUdpPkg(void *UserData, char *InBuffer,int InSize,char **OutBuffer,int *OutSize)
{
    int ret = 0;

    THIRD_UDP_USERDATA *userdata = (THIRD_UDP_USERDATA *)UserData;
    //LOGD("[%d][%s][%s][%d]\n",userdata->UseMask,userdata->DevName,userdata->RemoteIp,InSize);
    ret = UdpMultiServerCallBack(userdata->DevName, userdata->RemoteIp,InBuffer, InSize,OutBuffer,OutSize);

    return ret;
}


