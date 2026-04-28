#include <wsserver_api.h>
#include <dlfcn.h>
#include "ovfs_media.h"

typedef int (*WSServer_Init)();
typedef int (*WSServer_UnInit)();
typedef int (*WSServer_Start)();
typedef int (*WSServer_Stop)();
typedef int (*WSServer_SetStreamControlCallBack)(int hWsHandle, WSServer_StreamControl *pStreamControl, void *pUser);
typedef int (*WSServer_SendAlarm)(void *pEventInfo);

typedef struct
{
	WSServer_Init                        fWSServerInit;
	WSServer_UnInit                      fWSServerUnInit;
	WSServer_Start                       fWSServerStart;
	WSServer_Stop                        fWSServerStop;
	WSServer_SetStreamControlCallBack    fWSServerSetStreamControlCallBack;
	WSServer_SendAlarm                   fWSServerSendAlarm;
}OVFS_WSSERVER_INTERFACE_T;

typedef struct
{
    int state;
    MQ_HANDLE_H mqHandle;

    pthread_pool_t *tp;
    COMMON_DLIST_T conList;
    int streamCount;
    cJSON_Struct *auth;
    OVFS_WSSERVER_INTERFACE_T tWsInterface;
} WS_MGR_T;

typedef struct
{
    int iDev;
    int iCh;
    int iType;
	int iStreamId;

	int iCmd; // 0-start, 1-stop，2-pause, 3-speed, 4-frame
	int bAudio;     // 是否播放音频
	int bSmart;     // 是否播放视频
	int bVideo;
	int bPause;      //pause flag
	int iSpeed;     // [-4 , 4] 播放速度
	int iKeyFrame;  // I帧 个数
	int nRecordStart; //sec, if record
	int nRecordStop;  //sec, if record
	int requestOneFrame;
 }Request_Param_T;


static WS_MGR_T s_wsMgr;

int Ws_Get_Interface(void *pWsInterface, int size)
{
    OVFS_WSSERVER_INTERFACE_T tWsInterface;
	void *pWSLib = NULL;

    if (NULL == pWsInterface || size < sizeof(OVFS_WSSERVER_INTERFACE_T))
    {
        return -1;
	}

    memset(&tWsInterface, 0, sizeof(tWsInterface));

	if(pWSLib == NULL)
	{
        pWSLib = dlopen("/root/libwsserver.so", RTLD_NOW|RTLD_LOCAL);// dlclose(g_pOnvifLib);
		if(pWSLib == NULL)
		{
		    pWSLib = dlopen("/root/lib/libwsserver.so", RTLD_NOW|RTLD_LOCAL);// dlclose(g_pOnvifLib);
		}

		if(pWSLib == NULL)
		{
	        LOGE("[MID]Load ws server failed\n");
			return -1;
		}

		if(pWSLib != NULL)
		{
			if(tWsInterface.fWSServerInit == NULL)
			{
		   		tWsInterface.fWSServerInit = (WSServer_Init)dlsym(pWSLib, "WSServerInit");
			}

			if(tWsInterface.fWSServerUnInit == NULL)
			{
		        tWsInterface.fWSServerUnInit = (WSServer_UnInit)dlsym(pWSLib, "WSServerUnInit");
			}

			if(tWsInterface.fWSServerStart == NULL)
			{
		   		tWsInterface.fWSServerStart = (WSServer_Start)dlsym(pWSLib, "WSServerStart");
			}

			if(tWsInterface.fWSServerStop == NULL)
			{
		   		tWsInterface.fWSServerStop = (WSServer_Stop)dlsym(pWSLib, "WSServerStop");
			}

			if(tWsInterface.fWSServerSetStreamControlCallBack == NULL)
			{
		   		tWsInterface.fWSServerSetStreamControlCallBack = (WSServer_SetStreamControlCallBack)dlsym(pWSLib, "WSServerSetStreamControlCallBack");
			}

			if(tWsInterface.fWSServerSendAlarm == NULL)
			{
		        tWsInterface.fWSServerSendAlarm = (WSServer_SendAlarm)dlsym(pWSLib, "WSServerSendAlarm");
			}
		}

	}

	if (pWSLib != NULL)
	{
        memcpy(pWsInterface, &tWsInterface, sizeof(OVFS_WSSERVER_INTERFACE_T));
	}

    return 0;
}



static int WsOpenCallback(void *pInParam)
{
    int ret = 0;
    Request_Param_T *in = (Request_Param_T *) pInParam;

    MEDIA_REQ_STREAM_INFO_T reqInfo = {0};

    reqInfo.type = in->iType;
    reqInfo.dev = in->iDev;
    reqInfo.chan = in->iCh;
    reqInfo.streamid = in->iStreamId;
    reqInfo.auth = s_wsMgr.auth;

    if(in->iType == WEBSOCKET_REQUEST_TYPE_RECORD)
    {
        reqInfo.type = MEDIA_STREAM_TYPE_RECORD;
        struct tm startTime = {0};
		struct tm stopTime = {0};
		time_t t1 = in->nRecordStart;
		time_t t2 = in->nRecordStop;
		gmtime_r ( &t1,&startTime );
		gmtime_r ( &t2,&stopTime );

        reqInfo.recordPlayStartYear = startTime.tm_year + 1900;
        reqInfo.recordPlayStartMon = startTime.tm_mon + 1;
        reqInfo.recordPlayStartDay = startTime.tm_mday;
        reqInfo.recordPlayStartHour = startTime.tm_hour;
        reqInfo.recordPlayStartMinute = startTime.tm_min;
        reqInfo.recordPlayStartSecond = startTime.tm_sec;

        reqInfo.recordPlayStopYear = stopTime.tm_year + 1900;
        reqInfo.recordPlayStopMon = stopTime.tm_mon + 1;
        reqInfo.recordPlayStopDay = stopTime.tm_mday;
        reqInfo.recordPlayStopHour = stopTime.tm_hour;
        reqInfo.recordPlayStopMinute = stopTime.tm_min;
        reqInfo.recordPlayStopSecond = stopTime.tm_sec;

    }
    else if(in->iType == WEBSOCKET_REQUEST_TYPE_TALKING)
    {
        reqInfo.type = MEDIA_STREAM_TYPE_TALKING;
    }

    ret = RestMedia_RequestStreamOpen(&reqInfo);

    int i = 0;
    if (reqInfo.param)
    {
        for (i = 0;i < MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX;i++)
        {
            if (reqInfo.param[i].szUri)
            {
                MEDIA_FREE(reqInfo.param[i].szUri);
                reqInfo.param[i].szUri = NULL;
            }
			if(reqInfo.param[i].pOpenParams != NULL)
	        {
	        	Common_Json_Delete(reqInfo.param[i].pOpenParams);
	        	reqInfo.param[i].pOpenParams = NULL;
	        }
			if(reqInfo.param[i].pOutStreamInfo != NULL)
	        {
	        	Common_Json_Delete(reqInfo.param[i].pOutStreamInfo);
	        	reqInfo.param[i].pOutStreamInfo = NULL;
	        }
        }
        MEDIA_FREE(reqInfo.param);
        reqInfo.param = NULL;
    }

    return ret;
}

static int WsCloseCallback(int hStreamHandle)
{
    int ret = 0;
    MEDIA_REQ_STREAM_INFO_T reqInfo = {0};

    ret = RestMedia_RequestStreamClose(hStreamHandle, &reqInfo);
    return ret;
}

static int WsReadDataCallback(int hStreamHandle, void *pInParam,int *nReqIFrame,void **pData,int *pDataSize)
{
    int ret = 0;
    int reqIFrame = *nReqIFrame;
    Request_Param_T *in = (Request_Param_T *) pInParam;
    if (pInParam == NULL || pData == NULL || pDataSize == NULL)
    {
        return 0;
    }

    int sidx = -1;
    do
    {
        ret = RestMedia_RequestStreamRead(hStreamHandle,-1,&sidx,(void **)pData,(int *)pDataSize,NULL,400);

        if (ret != 0)
        {
            break;
        }

        Ovfs_FrameHeader_T *h = NULL;
        h = (Ovfs_FrameHeader_T *) (*pData);

        if(in->iType == WEBSOCKET_REQUEST_TYPE_REAL)
        {
            if (h->uiFrameType != Ovfs_FrameType_IFrames
                && h->uiFrameType != Ovfs_FrameType_SubIFrames
                && h->uiFrameType != Ovfs_FrameType_ThirdIFrames)
            {
                if(reqIFrame == 1)
                {
                    MEDIA_REQ_STREAM_INFO_T reqInfo = {0};

                    reqInfo.type = in->iType;
                    reqInfo.dev = in->iDev;
                    reqInfo.chan = in->iCh;
                    reqInfo.streamid = in->iStreamId;

                    reqInfo.auth = s_wsMgr.auth;

                    RestMedia_RequestVideoIFrame(&reqInfo);
                    reqIFrame = 0;

                    RestMedia_RequestStreamRelease(hStreamHandle);
                    continue;
                }

                if(*nReqIFrame == 1)
                {
                    RestMedia_RequestStreamRelease(hStreamHandle);
                    continue;
                }

            }

        }

        break;
    }while(1);

    if(ret == 0)
    {
        if(*nReqIFrame == 1)
        {
            *nReqIFrame = 0;
        }
    }

    return ret;
}

static int WsWriteDataCallback(int hStreamHandle,void *pData,int nDataSize)
{
    int ret = 0;
    ret = RestMedia_RequestStreamWrite(hStreamHandle, MEDIA_STREAM_QUEUE_ID_AUDIO_TALK, pData, nDataSize);
    return ret;
}

static int WsReleaseDataCallback(int hStreamHandle)
{
    int ret = 0;
    ret = RestMedia_RequestStreamRelease(hStreamHandle);
    return ret;
}

static int WsControlCallback(int hStreamHandle,int nType,void *pInParam,void *pOutParam)
{
    int ret = 0;
    return ret;
}

int WsMgr_Init(MQ_HANDLE_H mqHandle)
{
    int ret = -1;
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(errno));
        return -EINVAL;
    }

    memset(&s_wsMgr,0,sizeof(WS_MGR_T));
    s_wsMgr.mqHandle = mqHandle;

    s_wsMgr.auth = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if (s_wsMgr.auth)
    {
        Common_Json_SetAttrValue(s_wsMgr.auth, -1, "Method", Common_Json_Type_Number, NULL, MEDIA_AUTH_TYPE_TEXT, 0);
        Common_Json_SetAttrValue(s_wsMgr.auth, -1, "UserName", Common_Json_Type_String, "(null)", 0, 0);
        Common_Json_SetAttrValue(s_wsMgr.auth, -1, "Password", Common_Json_Type_String, "ovfsZSJQZLHL",0, 0);
    }

    ret = Ws_Get_Interface(&s_wsMgr.tWsInterface, sizeof(OVFS_WSSERVER_INTERFACE_T));
    if (ret < 0)
    {
        LOGE("%s\n", strerror(errno));
        return -EINVAL;
    }

    if(s_wsMgr.tWsInterface.fWSServerInit)
    {
        ret = s_wsMgr.tWsInterface.fWSServerInit();
    }
    else
    {
        ret = -1;
    }

    if (ret < 0)
    {
        LOGE("WS server init failed , ret %d\n", ret);
    }

    LOGD("\n");
    return ret;

}

int WsMgr_Start()
{
    int ret = 0;
    if(s_wsMgr.tWsInterface.fWSServerStart)
    {
        ret = s_wsMgr.tWsInterface.fWSServerStart();
    }
    else
    {
        ret = -1;
    }

    if (ret < 0)
    {
        LOGE("WSServerStart failed %d\n",ret);
        return ret;
    }

    //s_wsMgr.rtspHandle = ret;
    WSServer_StreamControl sc;
    sc.fxnOpen = WsOpenCallback;
    sc.fxnRead = WsReadDataCallback;
    sc.fxnWrite = WsWriteDataCallback;
    sc.fxnControl = WsControlCallback;
    sc.fxnClose = WsCloseCallback;
    sc.fxnRelease = WsReleaseDataCallback;

    if(s_wsMgr.tWsInterface.fWSServerSetStreamControlCallBack)
    {
        ret = s_wsMgr.tWsInterface.fWSServerSetStreamControlCallBack(1, &sc, (void *)&s_wsMgr);
    }
    else
    {
        ret = -1;
    }

    if (ret < 0)
    {
        LOGE("ws server RTSPServerV2_SetStreamControlCallBack failed %d\n",ret);
        WsMgr_Stop();
        return ret;
    }
}

int WsMgr_Stop()
{
    int ret = 0;

    if(s_wsMgr.tWsInterface.fWSServerStop)
    {
        ret = s_wsMgr.tWsInterface.fWSServerStop();
    }

    if(s_wsMgr.auth)
    {
        Common_Json_Delete(s_wsMgr.auth);
        s_wsMgr.auth = NULL;
    }

    return ret;
}

int WsMgr_Restart()
{
    int ret = 0;

    WsMgr_Stop();
    WsMgr_Start();

    return ret;
}

int WsMgr_SendAlarm(void *data)
{
    if(s_wsMgr.tWsInterface.fWSServerSendAlarm)
    {
        s_wsMgr.tWsInterface.fWSServerSendAlarm(data);
    }

    return 0;
}
