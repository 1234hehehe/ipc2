/*
 * ovfs_rtmp_mgr.c
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

#include <rtmpserver_api.h>
#include "ovfs_media.h"
#ifdef RTMP_AAC
#include "g711.h"
#include <aac/faac.h>
#endif

typedef struct
{
    RTMPPUSH_HANDLE hRtmpPush[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_CHANNEL_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM];
    RTMPPUSH_CONFIG_T Pushcfg;
	int pushState[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_CHANNEL_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM];
} RTMPPUSH_MGR_T;


typedef struct
{
    int state;
    int rtmpHandle;
    MQ_HANDLE_H mqHandle;
    RTMP_CONFIG_T cfg;
    COMMON_DLIST_T conList;
    pthread_pool_t *tp;
	RTMPPUSH_MGR_T tRtmpPushMgr;
} RTMP_MGR_T;

typedef struct
{
    unsigned int token;
    int rtmpHandle;
    void *streamHandle;
    int loginSessionId;
    MEDIA_REQ_STREAM_INFO_T reqInfo;

    int streamType;
    unsigned int connectIp;
    long long int connectTime;
    long long int lastReadTime;

    char username[64];
    char password[64];
    int loginHandle;
    int streamQueueHandle;
    RTMP_MGR_T *rtmpMgr;
    long long unsigned int totalReadSize;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int isExit;
    long long int timeStamp;

    int pause;
    int forceI;
    int forceS;
    int reqIFrame;
    int reqChangeCodec;
} RTMP_CONNECT_INFO_NODE_T;

static RTMP_MGR_T s_rtmpMgr;

static int RtmpParserParam(int rtmpHandle, int streamType, char *url, RTMP_CONNECT_INFO_NODE_T *node)
{
    char *str = NULL, *devStr = NULL;
    char tmp1[32] = { 0 };
    if (node == NULL || url == NULL)
        return -1;

    if ((str = strstr(url,"ch")) == NULL)
    {
        LOGE("ch not find return \n");
        return -1;
    }

    devStr = strstr(str,"dev=");
    if (devStr)
    {
        sscanf(devStr,"dev=%[0-9]",tmp1);
        node->reqInfo.dev = atoi(tmp1) - 1;
        if (node->reqInfo.dev < 0)
            node->reqInfo.dev = 0;
    }
    else
        node->reqInfo.dev = 0;

    node->reqInfo.chan = MEDIA_CHANNEL_ID_DEFAULT;
    node->reqInfo.streamid = MEDIA_STREAM_ID_VIDEO_DEFAULT;

    if (sscanf(url, "ch%02d", &node->reqInfo.chan) == 1)
    {
        node->reqInfo.chan--;
    }

    str = strstr(url, "_sub.264");
    if (str != NULL)
    {
        node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_SUB;
    }

    str = strstr(url, "_third.264");
    if (str != NULL)
    {
        node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD;
    }

    node->streamType  = streamType;
    node->connectTime = Utils_GetMs();

    if (node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
    {
        ///file/ch01.264?rectype=&start=&stop=20140227180000
        // ch01.264?start=20140220105000&stop=20140220105100
        char *pStr = NULL;

        pStr = strstr(url, "rectype=");
        if (pStr == NULL)
        {
            LOGE("not find rectype=\n");
            return -1;
        }

        pStr = strstr(url, "start=");
        if (pStr != NULL)
        {
            sscanf(pStr + strlen("start="), "%04d%02d%02d%02d%02d%02d", &node->reqInfo.recordPlayStartYear, &node->reqInfo.recordPlayStartMon,
                            &node->reqInfo.recordPlayStartDay, &node->reqInfo.recordPlayStartHour, &node->reqInfo.recordPlayStartMinute,
                            &node->reqInfo.recordPlayStartSecond);
        }
        else
        {
            LOGE("cound not find start=\n");
            return -1;
        }

        pStr = strstr(url, "stop=");
        if (pStr != NULL)
        {
            sscanf(pStr + strlen("stop="), "%04d%02d%02d%02d%02d%02d", &node->reqInfo.recordPlayStopYear, &node->reqInfo.recordPlayStopMon,
                            &node->reqInfo.recordPlayStopDay, &node->reqInfo.recordPlayStopHour, &node->reqInfo.recordPlayStopMinute,
                            &node->reqInfo.recordPlayStopSecond);
        }
    }

    if (node->reqInfo.chan < MEDIA_CHANNEL_ID_MIN || node->reqInfo.chan > MEDIA_CHANNEL_ID_MAX || node->reqInfo.streamid < MEDIA_STREAM_ID_VIDEO_MIN || node->reqInfo.streamid > MEDIA_STREAM_ID_VIDEO_MAX)
    {
        LOGE("channelId=%d streamId=%d\n",node->reqInfo.chan ,node->reqInfo.streamid);
        return -1;
    }

    return 0;
}

static void RtmpConnectNodeFree(void *ptr)
{
    MEDIA_FREE(ptr);
}

static int RtmpConnectNodeCompare(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;

    if (((RTMP_CONNECT_INFO_NODE_T *) a)->streamHandle == b)
        return 0;

    return -1;
}

static void RtmpInputRecordDataThread(void *thParam)
{
    int ret = 0;
    if (thParam == NULL)
        return;

    prctl(PR_SET_NAME, __func__);

    RTMP_CONNECT_INFO_NODE_T *node = (RTMP_CONNECT_INFO_NODE_T*)thParam;

    while (1)
    {
        void *recvData= NULL;
        int recvSize = 0;

        if (node == NULL || node->isExit || node->rtmpMgr == NULL || node->rtmpMgr->state != MEDIA_RTMP_STATE_START)
        {
            break;
        }

        ret= RestMedia_RequestStreamRead(node->streamQueueHandle,-1,NULL,&recvData,&recvSize,NULL,1000);
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
            ret = Ants_RTMPServer_InputAntsData(node->rtmpHandle, node->streamHandle, recvData, recvSize);
            RestMedia_RequestStreamRelease(node->streamQueueHandle);
            if (ret <= 0)
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

static int RtmpStreamCallback(int hRtmpHandle, char *pToken, int nDataSizeint, int ntype,int nStreamType,void *pRtmpInfo, void *pUser)
{
    LOGD("\n");
    if (pUser == NULL || pToken == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return 0;
    }

    if (nStreamType == ANTS_RTMPSERVER_STREAMTYPE_FILE)
    {
        LOGW("type %d token is %s\n",ntype, pToken);
        RTMP_MGR_T *rtmpMgr = pUser;
        RTMP_CONNECT_INFO_NODE_T *node = NULL;
        node = Common_DList_Search(rtmpMgr->conList,pRtmpInfo,RtmpConnectNodeCompare);
        if (node == NULL)
        {
            LOGE("not find this connection\n");
            return 0;
        }

        switch (ntype)
        {
            case ANTS_RTMPSERVER_STREAM_START:
            {
                RestMedia_RequestRecordControl(node->streamQueueHandle, MEDIA_STREAM_CONTROL_PLAY, NULL);
                break;
            }
            case ANTS_RTMPSERVER_STREAM_STOP:
            {
                RestMedia_RequestRecordControl(node->streamQueueHandle, MEDIA_STREAM_CONTROL_STOP, NULL);
                break;
            }
            case ANTS_RTMPSERVER_STREAM_PAUSE:
            {
                char *valueStr = strstr(pToken,"pause=");
                int value = 0;
                if (valueStr == NULL)
                {
                    LOGE("could not find string 'pause'\n");
                    return 0;
                }
                value = atoi(valueStr+6);
                if (value == 1)
                    RestMedia_RequestRecordControl(node->streamQueueHandle, MEDIA_STREAM_CONTROL_STOP, NULL);
                else
                    RestMedia_RequestRecordControl(node->streamQueueHandle, MEDIA_STREAM_CONTROL_PLAY, NULL);
                break;
            }
            case ANTS_RTMPSERVER_STREAM_SPEED:
            {
                char *valueStr = strstr(pToken,"speed=");
                int value = 0;
                if (valueStr == NULL)
                {
                    LOGE("could not find string 'speed'\n");
                    return 0;
                }
                value = atoi(valueStr+6);
                if (value)
                    value = 1;
                else
                    value = 0;
                RestMedia_RequestRecordControl(node->streamQueueHandle,MEDIA_STREAM_CONTROL_FORCE_IFRAME,&value);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    LOGD("\n");
    return 1;
}

//用户可以通过pStreamUser返回一个句柄hId，后面的接口都会把这个句柄回传，pRtmpStreamHandle是每个连接的唯一标志
static int RtmpOpenCallback(int rtmpHandle, void *streamHandle, char *url, int size, int streamType, void **streamUser, void *user)
{
    LOGD("\n");
    RTMP_MGR_T * rtmpMgr = (RTMP_MGR_T *) user;
    MEDIA_AUTH_INFO_T authInfo = { 0 };
    if (url == NULL || streamHandle == NULL|| rtmpMgr == NULL || streamUser == NULL || rtmpHandle <= 0)
    {
        LOGE("%s \n",strerror(EINVAL));
        return 0;
    }

    if (streamType != ANTS_RTMPSERVER_STREAMTYPE_LIVE && streamType != ANTS_RTMPSERVER_STREAMTYPE_FILE)
    {
        LOGW("streamType error %d\n",streamType);
        return 0;
    }

    if (rtmpMgr->state != MEDIA_RTMP_STATE_START)
    {
        LOGE("state error\n");
        return 0;
    }

    prctl(PR_SET_NAME,__func__);

    RTMP_CONNECT_INFO_NODE_T *node = NULL;
    *streamUser = MEDIA_MALLOC(sizeof(RTMP_CONNECT_INFO_NODE_T));
    if (*streamUser == NULL)
    {
        LOGE("calloc mem failed\n");
        return 0;
    }
    memset(*streamUser,0,sizeof(RTMP_CONNECT_INFO_NODE_T));

    node = *streamUser;
    node->token = Common_Rand32();
    node->streamHandle = streamHandle;
    node->rtmpHandle = rtmpHandle;
    node->rtmpMgr = rtmpMgr;

    if (streamType == ANTS_RTMPSERVER_STREAMTYPE_LIVE)
        node->reqInfo.type = MEDIA_STREAM_TYPE_REAL;
    else
        node->reqInfo.type = MEDIA_STREAM_TYPE_RECORD;

    LOGW("req uri is %s type is %d \n",url,streamType);//req uri is ch01.264?rectype=0&start=20170502045230&stop=20170502235959&sessionid=0429768135
    if (RtmpParserParam(rtmpHandle,streamType,url,node) < 0)
    {
        LOGE("parser uri failed\n");
        MEDIA_FREE(*streamUser);
        *streamUser = NULL;
        return 0;
    }

	char strP[16] = {0};
	snprintf(strP, sizeof(strP), "ovfsZSJQZLHL");

    authInfo.authMethod = MEDIA_AUTH_TYPE_TEXT;
    authInfo.userName = "(null)";
    authInfo.password = strP;
    authInfo.sessionId = (int)node->streamHandle;
    snprintf(authInfo.ipStr, sizeof(authInfo.ipStr), "%s", "127.0.0.1");

    node->loginSessionId = RestMedia_RegistAuthNode(&authInfo);
    LOGW("session id %d\n",node->loginSessionId);

    MEDIA_AUTH_NODE_T *authNode = NULL;
    authNode = RestMedia_SearchAuthNode((int)node->streamHandle);
    if (authNode == NULL)
    {
        LOGE("could not find this authorized info\n");
        return 0;
    }

    node->reqInfo.auth = authNode->auth;

    node->streamQueueHandle = RestMedia_RequestStreamOpen(&node->reqInfo);

    if (node->streamQueueHandle < 0)
    {
        LOGE("open stream dev %d channel %d stream %d failed\n",node->reqInfo.dev,node->reqInfo.chan - 1, node->reqInfo.streamid - 1);
        MEDIA_FREE(*streamUser);
        *streamUser = NULL;
        return 0;
    }

    if (node->reqInfo.type == MEDIA_STREAM_TYPE_RECORD)
    {
        pthread_condattr_t condattr;
        pthread_condattr_init(&condattr);
        pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
        pthread_cond_init(&node->cond, &condattr);
        pthread_condattr_destroy(&condattr);
        pthread_mutex_init(&node->mutex,NULL);
        pthread_pool_add(node->rtmpMgr->tp,RtmpInputRecordDataThread,(void *)node);
    }

    Common_DList_InsertTail(node->rtmpMgr->conList,node,sizeof(RTMP_CONNECT_INFO_NODE_T));
    Utils_Sleep(10);
    LOGD("\n");
    return 1;
}

static int RtmpReadDataCallback(int hRtmpHandle, void *streamHandle, void *streamUser, int streamType, char **data, unsigned int *size, void *pStreamInfo, int *nReadType)
{
    RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) streamUser;
    int ret = -1;
    if (node == NULL || data == NULL || size == NULL
        || streamType != ANTS_RTMPSERVER_STREAMTYPE_LIVE)
    {
        return 0;
    }

    if (node->rtmpMgr->state != MEDIA_RTMP_STATE_START)
    {
        LOGE("state error\n");
        return 0;
    }

    if (nReadType != NULL)
        *nReadType = 1;

    int sidx = -1;
    do
    {
        ret = RestMedia_RequestStreamRead(node->streamQueueHandle,-1,
                                          &sidx,(void **)data,(int *)size,NULL,400);
        if (ret != 0)
        {
            break;
        }

        Ovfs_FrameHeader_T *h = NULL;
        h = (Ovfs_FrameHeader_T *) (*data);
        if (sidx > MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD)/*other data except video data*/
        {
            if (node->totalReadSize == 0)
            {
                RestMedia_RequestStreamRelease(node->streamQueueHandle);
                continue;
            }
            break;
        }

        /*video data*/
        if (h->uMedia.struVideoHeader.cCodecId != Ovfsmid_VideoCodecID_H264_hisi_RTP)
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

        if (node->totalReadSize == 0
            && h->uiFrameType != Ovfs_FrameType_IFrames
            && h->uiFrameType != Ovfs_FrameType_SubIFrames
            && h->uiFrameType != Ovfs_FrameType_ThirdIFrames)
        {
            /*request i frame*/
            if (node->reqIFrame == 0)
            {
                RestMedia_RequestVideoIFrame(&node->reqInfo);
                node->reqIFrame = 1;
            }
            RestMedia_RequestStreamRelease(node->streamQueueHandle);
            continue;
        }

        break;
    }while(1);

    if (ret == 0)
        node->totalReadSize += (long long unsigned int )(*size);
    return (ret != 0) ? 0 : 1;
}

static int RtmpCloseCallback(int hRtmpHandle, void *pRtmpStreamHandle, void *pStreamUser, int nStreamType)
{
    int ret = 0;
    COMMON_DLIST_T conList = NULL;
    LOGD("do\n");

    prctl(PR_SET_NAME,__func__);
    if (pStreamUser == NULL || (nStreamType != ANTS_RTMPSERVER_STREAMTYPE_FILE && nStreamType != ANTS_RTMPSERVER_STREAMTYPE_LIVE))
    {
        return 0;
    }

    RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) pStreamUser;
    if (node == NULL)
    {
        LOGE("stream handle is null\n");
        return 0;
    }

    if (nStreamType == ANTS_RTMPSERVER_STREAMTYPE_FILE)
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
    }
    else
    {
        RestMedia_RequestStreamClose(node->streamQueueHandle,&node->reqInfo);
    }

    if (node->reqChangeCodec > 0)
    {
        /*request video codec back */
        RestMedia_RequestBoardCodecTempChange(0,&node->reqInfo);
    }

    RestMedia_UnregistAuthNode((int)node->streamHandle);

    conList = node->rtmpMgr->conList;
    Common_DList_Delete(conList,pRtmpStreamHandle,RtmpConnectNodeCompare);

    LOGD("done\n");
    return 1;
}

static int RtmpReleaseDataCallback(int hRtmpHandle, void *pRtmpStreamHandle, void *streamUser, int nStreamType)
{
    if (streamUser == NULL || nStreamType != ANTS_RTMPSERVER_STREAMTYPE_LIVE)
    {
        return 0;
    }
    RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) streamUser;
    RestMedia_RequestStreamRelease(node->streamQueueHandle);
//    RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) streamUser;
//        node->rtmpMgr->ReleaseCallback(node->shmNodeData,node->shmNodeHandle);
    return 1;
}

int RtmpMgr_Init(MQ_HANDLE_H mqHandle)
{
    LOGI("do\n");
    int ret = 0;
#ifdef RTMP

    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(errno));
        return -EINVAL;
    }

    memset(&s_rtmpMgr,0,sizeof(RTMP_MGR_T));

    s_rtmpMgr.mqHandle = mqHandle;

    ret = Ants_RTMPServer_Init();
    if (ret > 0)
        s_rtmpMgr.state = MEDIA_RTMP_STATE_STOP;
#endif
    LOGI("done\n");
    return (ret > 0) ? 0 : -1;
}

int RtmpMgr_Start()
{
    int ret = 0;
#ifdef RTMP
    RTMP_CONFIG_T cfg;
    memset(&cfg,0,sizeof(RTMP_CONFIG_T));

    LOGD("\n");
    if (s_rtmpMgr.state == MEDIA_RTMP_STATE_UNINIT)
    {
        LOGE("rtmp uninit\n");
        return -EINVAL;
    }

    if (s_rtmpMgr.state == MEDIA_RTMP_STATE_START)
    {
        LOGE("rtmp mgr already start\n");
        return -1;
    }

    if (Mq_Request(s_rtmpMgr.mqHandle, MEDIA_REQ_RTMP_GET_CFG, NULL, 0, &ret, &cfg, sizeof(RTMP_CONFIG_T)) < 0 || ret < 0)
    {
        LOGE("load rtmp config failed\n");
        return -1;
    }

    if (cfg.enable == 0)
    {
        LOGW("rtmp server was disabled\n");
        return 0;
    }

    /*TODO check configure file*/

    ret  = Ants_RTMPServer_Start(cfg.rtmpPort);
    if (ret > 0)
    {
        s_rtmpMgr.rtmpHandle = ret;
        ret = 0;
    }
    else
    {
        LOGE("rtmp server start  failed , ret %d\n", ret);
        ret = -1;
    }

    if (ret == 0)
    {
        ret = Ants_RTMPServer_SetStreamCallBack(s_rtmpMgr.rtmpHandle, RtmpStreamCallback, (void *)&s_rtmpMgr);
        if (ret <= 0)
        {
            LOGE("RTMPServer_SetStreamCallBack failed %d\n",ret);
            Ants_RTMPServer_Stop(s_rtmpMgr.rtmpHandle);
            ret = -1;
        }
        else
            ret = 0;

    }

    if (ret == 0)
    {
        RTMPServer_StreamControl sc;
        sc.fxnOpen = RtmpOpenCallback;
        sc.fxnRead = RtmpReadDataCallback;
        sc.fxnControl = NULL;
        sc.fxnClose = RtmpCloseCallback;
        sc.fxnRelease = RtmpReleaseDataCallback;
        ret = Ants_RTMPServer_SetStreamCallBackV2(s_rtmpMgr.rtmpHandle, &sc, (void *)&s_rtmpMgr);
        if (ret <= 0)
        {
            LOGE("RTMPServer_SetStreamCallBackV2 failed %d\n",ret);
            Ants_RTMPServer_Stop(s_rtmpMgr.rtmpHandle);
            ret = -1;
        }
        else
        {
            ret = 0;
            s_rtmpMgr.cfg = cfg;
            s_rtmpMgr.state = MEDIA_RTMP_STATE_START;
            s_rtmpMgr.tp = thread_pool_create(24, 0, PTHREAD_STACK_MIN * 64);
            Common_DList_Init(&s_rtmpMgr.conList, RtmpConnectNodeFree);
        }


    }
#endif
    LOGI("done\n");
    return ret;
}

int RtmpMgr_Stop()
{
    int ret = 0;
    LOGI("do\n");
#ifdef RTMP
    if (s_rtmpMgr.state == MEDIA_RTMP_STATE_UNINIT)
    {
        LOGE("rtmp uninit\n");
        return -EINVAL;
    }

    if (s_rtmpMgr.state == MEDIA_RTMP_STATE_STOP)
    {
        LOGE("rtmp mgr alread stop\n");
        return -1;
    }

    s_rtmpMgr.state = MEDIA_RTMP_STATE_STOP;
    ret = Ants_RTMPServer_Stop(s_rtmpMgr.rtmpHandle);
    if (ret > 0)
    {
        ret = 0;
        s_rtmpMgr.state = MEDIA_RTMP_STATE_STOP;
        pthread_pool_destroy(s_rtmpMgr.tp);
        s_rtmpMgr.tp = NULL;
        s_rtmpMgr.rtmpHandle = 0;
        Common_DList_Uninit(&s_rtmpMgr.conList);
    }
    else
    {
        LOGE("rtmp server stop failed %d\n",ret);
        s_rtmpMgr.state = MEDIA_RTMP_STATE_START;
        ret = -1;
    }
#endif
    LOGD("\n");
    return ret;
}

int RtmpMgr_Restart()
{
    LOGD("\n");
    int ret = -1;
    RTMP_CONFIG_T cfg;
    memset(&cfg, 0, sizeof(RTMP_CONFIG_T));

    if (s_rtmpMgr.state == MEDIA_RTMP_STATE_UNINIT)
    {
        LOGE("rtmp server not init\n");
        return -1;
    }

    if (Mq_Request(s_rtmpMgr.mqHandle, MEDIA_REQ_RTMP_GET_CFG, NULL, 0, &ret, &cfg, sizeof(RTMP_CONFIG_T)) < 0 || ret < 0)
    {
        LOGE("load rtmp config failed\n");
        return -1;
    }

    if (cfg.enable == 0 && s_rtmpMgr.state == MEDIA_RTMP_STATE_START)
    {
        RtmpMgr_Stop();
    }

    if (cfg.enable == 1 && s_rtmpMgr.state == MEDIA_RTMP_STATE_STOP)
    {
        RtmpMgr_Start();
    }

    if (cfg.enable == 1 && s_rtmpMgr.state == MEDIA_RTMP_STATE_START)
    {
        if (memcmp(&cfg, &s_rtmpMgr.cfg, sizeof(RTMP_CONFIG_T)) != 0)
        {
            RtmpMgr_Stop();
            RtmpMgr_Start();
        }
    }

    LOGD("\n");

    return 0;
}

#ifdef RTMP_AAC
//FILE *fp = NULL;
faacEncHandle hEncoder[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM] = {NULL};
unsigned long nInputSamples = 0;           // AAC编码器PCM输入样本点数，
unsigned long nMaxOutPutBytes = 0;         //AAC编码后AAC数据输出字节数
//faacEncConfigurationPtr pConfig = {0};
char *pcmBuffer[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM] = {NULL};
int pcmBufferSize[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM];
int curSize[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM];
FILE *fp_aac = NULL;
char *aacBuffer[RTMPPUSH_SESSION_MAX_NUM][RTMPPUSH_STREAM_MAX_NUM] = {NULL};
#endif

static int RtmpPush_Open_Callback(RTMPPUSH_HANDLE hRtmpPushHandle, void **pStreamUser, void *pUser)
{
	int nClient = 0,nChannel = 0,nStream = 0;
	nClient = (int )pUser;
	nStream = nClient & 0xFF;
	nChannel = (nClient >> 8 ) &0xFF;
	nClient = (nClient >> 16) & 0xFFFF;
	if(nClient >= RTMPPUSH_SESSION_MAX_NUM || nStream >= RTMPPUSH_STREAM_MAX_NUM || nChannel >= RTMPPUSH_CHANNEL_MAX_NUM)
	{
		return -1;
	}
	 LOGD("\n");
    RTMP_MGR_T * rtmpMgr = (RTMP_MGR_T *) pUser;
    MEDIA_AUTH_INFO_T authInfo = { 0 };

    prctl(PR_SET_NAME,__func__);

    RTMP_CONNECT_INFO_NODE_T *node = NULL;
    *pStreamUser = MEDIA_MALLOC(sizeof(RTMP_CONNECT_INFO_NODE_T));
    if (*pStreamUser == NULL)
    {
        LOGE("calloc mem failed\n");
        return 0;
    }
    memset(*pStreamUser,0,sizeof(RTMP_CONNECT_INFO_NODE_T));

    node = *pStreamUser;
    node->token = Common_Rand32();
    node->streamHandle = hRtmpPushHandle;
    node->rtmpHandle = NULL;
    node->rtmpMgr = rtmpMgr;


    node->reqInfo.type = MEDIA_STREAM_TYPE_REAL;
	node->reqInfo.dev = 0;
	node->reqInfo.chan = nChannel;
	if(nStream == 0)
	{
		node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN;
	}
	else if(nStream == 1)
	{
		node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_SUB;
	}
	else if(nStream == 2)
	{
		node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD;
	}
    /*
	else if(nStream == 3)
	{
		node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_FOURTH;
	}
	else if(nStream == 4)
	{
		node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_FIFTH;
	}
	else if(nStream == 5)
	{
		node->reqInfo.streamid = MEDIA_STREAM_QUEUE_ID_VIDEO_SIXTH;
	}
    */
	node->streamType = ANTS_RTMPSERVER_STREAMTYPE_LIVE;


    LOGW("req uri is %s \n",s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl);//req uri is ch01.264?rectype=0&start=20170502045230&stop=20170502235959&sessionid=0429768135

	char strP[16] = {0};
	snprintf(strP, sizeof(strP), "ovfsZSJQZLHL");

    authInfo.authMethod = MEDIA_AUTH_TYPE_TEXT;
    authInfo.userName = "(null)";
    authInfo.password = strP;
    authInfo.sessionId = (int)node->streamHandle;
    snprintf(authInfo.ipStr, sizeof(authInfo.ipStr), "%s", "127.0.0.1");

    node->loginSessionId = RestMedia_RegistAuthNode(&authInfo);
    LOGW("session id %d\n",node->loginSessionId);

    MEDIA_AUTH_NODE_T *authNode = NULL;
    authNode = RestMedia_SearchAuthNode((int)node->streamHandle);
    if (authNode == NULL)
    {
        LOGE("could not find this authorized info\n");
		 MEDIA_FREE(*pStreamUser);
        *pStreamUser = NULL;
        return 0;
    }

    node->reqInfo.auth = authNode->auth;

    node->streamQueueHandle = RestMedia_RequestStreamOpen(&node->reqInfo);

    if (node->streamQueueHandle < 0)
    {
        LOGE("open stream dev %d channel %d stream %d failed\n",node->reqInfo.dev,node->reqInfo.chan - 1, node->reqInfo.streamid - 1);
		RestMedia_UnregistAuthNode((int)node->streamHandle);
        MEDIA_FREE(*pStreamUser);
        *pStreamUser = NULL;
        return 0;
    }

    RestMedia_RequestVideoIFrame(&node->reqInfo);


    LOGD("\n");
    return 1;
}
static int RtmpPush_Close_Callback(RTMPPUSH_HANDLE hRtmpPushHandle,int nType, void **pStreamUser,void *pUser)
{
	   int ret = 0;
	   COMMON_DLIST_T conList = NULL;

       int nClient = 0,nChannel = 0,nStream = 0;
       nClient = (int )pUser;
       nStream = nClient & 0xFF;
       nChannel = (nClient >> 8 ) &0xFF;
       nClient = (nClient >> 16) & 0xFFFF;
       if(nClient >= RTMPPUSH_SESSION_MAX_NUM || nStream >= RTMPPUSH_STREAM_MAX_NUM || nChannel >= RTMPPUSH_CHANNEL_MAX_NUM)
       {
           return -1;
       }

	   LOGD("do nType = %d \n",nType);
	   if(ANTS_RTMPPUSH_CLOSE_TYPE_RECONNECT == nType)
	   {
	   	 return 1;
	   }

	   prctl(PR_SET_NAME,__func__);
	   if (pStreamUser == NULL)
	   {
		   return 0;
	   }

	   RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) (*pStreamUser);
	   if (node == NULL)
	   {
		   LOGE("stream handle is null\n");
		   return 0;
	   }


	   RestMedia_RequestStreamClose(node->streamQueueHandle,&node->reqInfo);


	   RestMedia_UnregistAuthNode((int)node->streamHandle);
	   MEDIA_FREE(node);
	   *pStreamUser = NULL;
#ifdef RTMP_AAC

       if(hEncoder[nClient][nStream])
       {
            faacEncClose(hEncoder[nClient][nStream]);
            hEncoder[nClient][nStream] = NULL;
       }

       if(pcmBuffer[nClient][nStream])
       {
            free(pcmBuffer[nClient][nStream]);
            pcmBuffer[nClient][nStream] = NULL;
       }

       if(aacBuffer[nClient][nStream])
       {
            free(aacBuffer[nClient][nStream]);
            aacBuffer[nClient][nStream] = NULL;
       }
#endif

	   LOGD("done\n");
	   return 1;

}

//nReadType为1时，表示传的ppBuffer是带ants头信息的，nReadType为2时，表示传的ppBuffer是不带ants头信息的，此时需要用户填充pStreamInfo字段，结构为rtmp_stream_info
static int RtmpPush_Read_Callback(RTMPPUSH_HANDLE hRtmpPushHandle,char **ppBuffer, int *dwBuffsize, void *pStreamInfo, int *nReadType, void *pStreamUser, void *pUser)
{

   int nClient = 0,nChannel = 0,nStream = 0;
   nClient = (int )pUser;
   nStream = nClient & 0xFF;
   nChannel = (nClient >> 8 ) &0xFF;
   nClient = (nClient >> 16) & 0xFFFF;
   if(nClient >= RTMPPUSH_SESSION_MAX_NUM || nStream >= RTMPPUSH_STREAM_MAX_NUM || nChannel >= RTMPPUSH_CHANNEL_MAX_NUM)
   {
       return -1;
   }

   RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) pStreamUser;
   int ret = -1;
   if (node == NULL || ppBuffer == NULL || dwBuffsize == NULL)
   {
	   return 0;
   }

   if (nReadType != NULL)
	   *nReadType = 1;

   if (node->totalReadSize == 0)
   {
	   RestMedia_RequestVideoIFrame(&node->reqInfo);
   }

   void *data = NULL;
   int size = 0;
   //ret = RestMedia_RequestStreamRead(node->streamQueueHandle,-1,NULL,(void **)ppBuffer,(int *)dwBuffsize,NULL,100);
   ret = RestMedia_RequestStreamRead(node->streamQueueHandle,-1,NULL,&data,&size,NULL,100);

   if(ret<0)return 0;

   Ovfs_FrameHeader_T* header = ( Ovfs_FrameHeader_T* ) data;
/*   if (node->totalReadSize == 0 && header->uiFrameType != Ovfs_FrameType_IFrames && header->uiFrameType != Ovfs_FrameType_SubIFrames
                                        && header->uiFrameType != Ovfs_FrameType_ThirdIFrames)
   {
       LOGW("give up[%d][%d]\n",node->totalReadSize,header->uiFrameType);
       return 0;
   }*/
#ifdef RTMP_AAC

   if(header->uiFrameType == Ovfs_FrameType_AudioFrames && s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio)
   {
        int out_len = 2*(size-36);
        char *out_buf = NULL;
        out_buf = calloc(1,out_len);

        int r = 0;
        void *in_buf = data+36;
        r = g711_decode((void *)out_buf, &out_len, in_buf, size-36, TP_ULAW);
#if 0
        LOGD("r:[%d] out_len:[%d]\n",r,out_len);
        if(fp == NULL)
        {
            LOGW("fopen out_buf\n");
            fp = fopen("/tmp/audio.pcm","wb");
        }
        LOGD("fp ok\n");

        fwrite(out_buf,1,out_len,fp);
        LOGD("fwrite out_buf\n");
#endif

        if(hEncoder[nClient][nStream] == NULL)
        {
            hEncoder[nClient][nStream] = faacEncOpen(8000,1,&nInputSamples,&nMaxOutPutBytes);
            LOGW("nInputSamples:[%lu] nMaxOutPutBytes:[%lu]\n",nInputSamples,nMaxOutPutBytes);

            faacEncConfigurationPtr pConfig = faacEncGetCurrentConfiguration(hEncoder[nClient][nStream]);   /*获取AAC编码器参数配置结构体指针*/
            pConfig->outputFormat = 0;// 1
        	pConfig->allowMidside = 0;
        	pConfig->useLfe = 1;
        	pConfig->bitRate = 0;
        	pConfig->bandWidth = 32000;
        	pConfig->inputFormat = FAAC_INPUT_16BIT;
        	pConfig->aacObjectType = LOW;

        	faacEncSetConfiguration(hEncoder[nClient][nStream],pConfig);    //设置配置参数到AAC编码器

            pcmBufferSize[nClient][nStream] = nInputSamples * 16 / 8;

        }

        if(pcmBuffer[nClient][nStream] == NULL)
        {
            pcmBuffer[nClient][nStream] = calloc(1,pcmBufferSize[nClient][nStream]);
            aacBuffer[nClient][nStream] = calloc(1,nMaxOutPutBytes+36);
        }

        if(hEncoder[nClient][nStream])
        {
            //LOGD("curSize:[%d] pcmBufferSize:[%d]\n",curSize,pcmBufferSize);
            int offset = 0;
            if(curSize[nClient][nStream] < pcmBufferSize[nClient][nStream])
            {
                if(pcmBufferSize[nClient][nStream] - curSize[nClient][nStream] > out_len)//out_len
                {
                    memcpy(pcmBuffer[nClient][nStream]+curSize[nClient][nStream],out_buf,out_len);//out_buf,out_len
                    curSize[nClient][nStream] += out_len;//out_len
                }
                else
                {
                    offset = pcmBufferSize[nClient][nStream]-curSize[nClient][nStream];
                    memcpy(pcmBuffer[nClient][nStream]+curSize[nClient][nStream],out_buf,pcmBufferSize[nClient][nStream]-curSize[nClient][nStream]);//out_buf
                    curSize[nClient][nStream] = pcmBufferSize[nClient][nStream];
                }
            }
            //LOGD("curSize:[%d] offset:[%d]\n",curSize,offset);

            if(curSize[nClient][nStream] == pcmBufferSize[nClient][nStream])
            {
                memset(aacBuffer[nClient][nStream],0,nMaxOutPutBytes+36);
                r = faacEncEncode(hEncoder[nClient][nStream], pcmBuffer[nClient][nStream], nInputSamples, aacBuffer[nClient][nStream]+36, nMaxOutPutBytes);
                //printf("------------[%x][%x][%x][%x][%x][%x][%x]\n",aacBuffer+36,aacBuffer+37,aacBuffer+38,aacBuffer+39
                //    ,aacBuffer+40,aacBuffer+41,aacBuffer+42);
#if 0
                LOGW("r:[%d]\n",r);

                if(fp_aac == NULL)
                {
                    LOGW("fopen fp_aac\n");
                    fp_aac = fopen("/tmp/audio.aac","wb");
                }
#endif
                if(r > 0)
                {
#if 0
                    fwrite(aacBuffer+36,1,r,fp_aac);
                    LOGD("fwrite aacBuffer\n");
#endif
                    memcpy(aacBuffer[nClient][nStream],data,36);
                    Ovfs_FrameHeader_T* aacHeader = ( Ovfs_FrameHeader_T* ) aacBuffer[nClient][nStream];
                    aacHeader->uMedia.struAudioHeader.cCodecId = Ovfs_VoiceCodecID_AAC;
                    aacHeader->uiFrameLen = r;
                    *ppBuffer = aacBuffer[nClient][nStream];
                    *dwBuffsize = r+36;
                }

                curSize[nClient][nStream] = 0;
                memset(pcmBuffer[nClient][nStream],0,pcmBufferSize[nClient][nStream]);
                if(offset)
                {
                    curSize[nClient][nStream] = out_len-offset;//out_len
                    memcpy(pcmBuffer[nClient][nStream],out_buf+offset,out_len-offset);//out_buf,out_len
                }

            }

        }

        free(out_buf);
        //free(out_buf_16);
        //LOGD("free out_buf\n");

   }
   else
#endif
   {
        *ppBuffer = data;
        *dwBuffsize = size;
   }

   node->totalReadSize += *dwBuffsize;
   return (ret < 0) ? 0 : 1;

}
static int RtmpPush_Release_Callback(RTMPPUSH_HANDLE hRtmpPushHandle, void *pStreamUser, void *pUser)
{

	   RTMP_CONNECT_INFO_NODE_T * node = (RTMP_CONNECT_INFO_NODE_T *) pStreamUser;
	   RestMedia_RequestStreamRelease(node->streamQueueHandle);
	   return 1;

}
//此接口只对历史流有用，实时流的时候可以不设此回调
static int RtmpPush_Control_Callback(RTMPPUSH_HANDLE hRtmpPushHandle, int ntype, char *pData, int nDataSize,void *pStreamUser,void *pUser)
{
   return 1;
}

int RtmpPushMgr_CheckTime(RTMPPUSH_NOTDISTURB_CONFIG_T *cfg)
{
    int ret = 0;
    time_t timeA = time(NULL);
    Common_Time_T t_happent;

    Common_Linux2CommonTime(timeA,&t_happent);
    int t_start = t_happent.hour*100+t_happent.min;
	int i = t_happent.wday;
    int j = 0;
	for(j = 0;j < MAX_TIMESEGMENT;j++)
	{
        if((0 == cfg->NotDisturbTime[i][j].startTime) && (0 == cfg->NotDisturbTime[i][j].stopTime))
        {
            continue;
        }
		if((t_start >= cfg->NotDisturbTime[i][j].startTime) && (t_start <= cfg->NotDisturbTime[i][j].stopTime))
		{
            ret = 1;
			break;
		}
	}

    return ret;
}

void *checkTimeThread()
{
    int ret = 0;
    int nClient = 0,nChannel = 0,nStream = 0;
    int needClose = 0;

    RTMPPush_StreamControl tStreamControl;
    tStreamControl.fxnOpen = RtmpPush_Open_Callback;
    tStreamControl.fxnClose = RtmpPush_Close_Callback;
    tStreamControl.fxnRead = RtmpPush_Read_Callback;
    tStreamControl.fxnRelease = RtmpPush_Release_Callback;
    tStreamControl.fxnControl = RtmpPush_Control_Callback;

    while(1)
    {
        RTMPPUSH_NOTDISTURB_CONFIG_T cfg = {0};

        if (Mq_Request(s_rtmpMgr.mqHandle, MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG, NULL, 0, &ret, &cfg, sizeof(RTMPPUSH_NOTDISTURB_CONFIG_T)) < 0 || ret < 0)
        {
            LOGE("load rtmppush notdisturb config failed\n");
            Common_Sleep(1, 0);
            continue;
        }
        needClose = RtmpPushMgr_CheckTime(&cfg);

        for(nClient = 0 ; nClient < RTMPPUSH_SESSION_MAX_NUM ; nClient++)
        {
            for(nChannel = 0 ; nChannel < RTMPPUSH_CHANNEL_MAX_NUM ; nChannel++)
            {
                for(nStream = 0 ; nStream < RTMPPUSH_STREAM_MAX_NUM ; nStream++)
                {
                    if(needClose && cfg.Client[nClient] == 0)
                    {
                        if(s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] != NULL)
                        {
                            Ants_RTMPPush_Close(&s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream]);
                        }
                    }
                    else
                    {
                        if(s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] == NULL &&
                           s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnable &&
					       s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl[0] != 0)
                        {
                            RTMPPUSH_HANDLE hRtmpPush = NULL;

                            Ants_RTMPPush_Open(&hRtmpPush,s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl,
                                NULL,NULL,&tStreamControl,(void *)((nClient << 16) |(nChannel << 8)| nStream));
                            s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] = hRtmpPush;
                            if(hRtmpPush != NULL)
                            {
                                Ants_RTMPPush_Control(hRtmpPush,ANTS_RTMPPUSH_CONTROL_TYPE_ENABLE_AUDIO,&s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio,sizeof(int));
                                Ants_RTMPPush_Start(hRtmpPush);
                            }
                        }

                    }
                }
            }
        }

        Common_Sleep(1, 0);
    }
}

int RtmpPushMgr_Init(MQ_HANDLE_H mqHandle)
{
   LOGD("\n");
   RtmpPushMgr_Restart();

   Common_Thread_T tCheckRunTime = NULL;
   Common_Thread_Create(&tCheckRunTime, __FUNCTION__, 0, 0, checkTimeThread, NULL);
   return 0;
}


int RtmpPushMgr_Restart()
{
    LOGD("\n");
    int ret = -1;
    int needClose = 0;
    RTMPPUSH_CONFIG_T cfg;
	int nClient = 0,nChannel = 0,nStream = 0;
    memset(&cfg, 0, sizeof(RTMPPUSH_CONFIG_T));
	RTMPPush_StreamControl tStreamControl;
    RTMPPUSH_NOTDISTURB_CONFIG_T notdisturb_cfg = {0};

    if (s_rtmpMgr.state == MEDIA_RTMP_STATE_UNINIT)
    {
        LOGE("rtmp server not init\n");
        return -1;
    }

    if (Mq_Request(s_rtmpMgr.mqHandle, MEDIA_REQ_RTMPPUSH_GET_CFG, NULL, 0, &ret, &cfg, sizeof(RTMPPUSH_CONFIG_T)) < 0 || ret < 0)
    {
        LOGE("load rtmp config failed\n");
        return -1;
    }
	tStreamControl.fxnOpen = RtmpPush_Open_Callback;
	tStreamControl.fxnClose = RtmpPush_Close_Callback;
	tStreamControl.fxnRead = RtmpPush_Read_Callback;
	tStreamControl.fxnRelease = RtmpPush_Release_Callback;
	tStreamControl.fxnControl = RtmpPush_Control_Callback;

    if (Mq_Request(s_rtmpMgr.mqHandle, MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG, NULL, 0, &ret, &notdisturb_cfg, sizeof(RTMPPUSH_NOTDISTURB_CONFIG_T)) < 0 || ret < 0)
    {
        LOGE("load rtmppush notdisturb config failed\n");
        return -1;
    }

    needClose = RtmpPushMgr_CheckTime(&notdisturb_cfg);

	for(nClient = 0 ; nClient < RTMPPUSH_SESSION_MAX_NUM ; nClient++)
	{
		for(nChannel = 0 ; nChannel < RTMPPUSH_CHANNEL_MAX_NUM ; nChannel++)
		{
			for(nStream = 0 ; nStream < RTMPPUSH_STREAM_MAX_NUM ; nStream++)
			{
                if(needClose && notdisturb_cfg.Client[nClient] == 0)
                {
					s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio = cfg.tSession[nClient][nChannel][nStream].bEnableAudio;
				    s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnable = cfg.tSession[nClient][nChannel][nStream].bEnable;
					strcpy(s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl,cfg.tSession[nClient][nChannel][nStream].szUrl);
                    if(s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] != NULL)
                    {
                        Ants_RTMPPush_Close(&s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream]);
                        continue;
                    }
                }

				if(s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio != cfg.tSession[nClient][nChannel][nStream].bEnableAudio)
				{
					s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio = cfg.tSession[nClient][nChannel][nStream].bEnableAudio;
				}
				if(cfg.tSession[nClient][nChannel][nStream].bEnable != s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnable ||
				   0 != stricmp(cfg.tSession[nClient][nChannel][nStream].szUrl,s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl))
				{
				// 需要重启
					if(s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] != NULL /*&& s_rtmpMgr.tRtmpInterface.fRtmpPush_Close != NULL*/)
					{
						LOGD("[%d][%d][%d]hRtmpPush = %p\n",nClient,nChannel,nStream,s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream]);
						//s_rtmpMgr.tRtmpInterface.fRtmpPush_Close(&s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream]);
						Ants_RTMPPush_Close(&s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream]);
					}
				    s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnable = cfg.tSession[nClient][nChannel][nStream].bEnable;
					strcpy(s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl,cfg.tSession[nClient][nChannel][nStream].szUrl);
					if(s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnable &&
					   s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl[0] != 0 /*&&
					   s_rtmpMgr.tRtmpInterface.fRtmpPush_Open != NULL*/)
					{
						RTMPPUSH_HANDLE hRtmpPush = NULL;
						/*s_rtmpMgr.tRtmpInterface.fRtmpPush_Open*/
                        Ants_RTMPPush_Open(&hRtmpPush,s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].szUrl,
							NULL,NULL,&tStreamControl,(void *)((nClient << 16) |(nChannel << 8)| nStream));
						s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] = hRtmpPush;
						if(hRtmpPush != NULL)
						{
							/*s_rtmpMgr.tRtmpInterface.fRtmpPush_Control*/
                            Ants_RTMPPush_Control(hRtmpPush,ANTS_RTMPPUSH_CONTROL_TYPE_ENABLE_AUDIO,&s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio,sizeof(int));
							//s_rtmpMgr.tRtmpInterface.fRtmpPush_Start(hRtmpPush);
							Ants_RTMPPush_Start(hRtmpPush);

						}
					}
				}
                else if(s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream] != NULL)
                {
                    /*s_rtmpMgr.tRtmpInterface.fRtmpPush_Control*/
                    Ants_RTMPPush_Control(s_rtmpMgr.tRtmpPushMgr.hRtmpPush[nClient][nChannel][nStream],ANTS_RTMPPUSH_CONTROL_TYPE_ENABLE_AUDIO,&s_rtmpMgr.tRtmpPushMgr.Pushcfg.tSession[nClient][nChannel][nStream].bEnableAudio,sizeof(int));
                }
			}
		}
	}



    LOGD("\n");

    return 0;
}


