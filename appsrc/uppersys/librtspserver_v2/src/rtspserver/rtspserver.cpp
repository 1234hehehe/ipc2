
#include "rtsp_common.h"
#include "rtspserver_v2.h"
#include "rtspclient.h"
#ifdef WIN32
#pragma comment(lib,"Ws2_32.lib")
#include <ws2tcpip.h>
#endif
#include "rtsp.h"
static CRtspServer *m_hRtspServers[MAX_RTSPSERVER_NUM] = {0};
//static int m_hRtspStreams[MAX_RTSPSERVER_NUM][MAX_RTSPSERVER_STREAM_NUM];
//static int m_hRtspRecordStreams[MAX_RTSPSERVER_NUM][MAX_RTSPSERVER_STREAM_NUM];
static JMutex m_hRtspServerLock;
static int m_bRtspServerInit = 0;
static ANTS_RTSPSTATUSCALLBACK m_fRtspStatusCallback = NULL;
static void *m_pRtspStatusUser = NULL;

static ANTS_RTSPSERVER_STREAMDATA m_fRtspStreamDataCallback = NULL;
static void *m_pRtspStreamDataUser = NULL;
static unsigned long m_dwProp = 0;

static int ants_RtspStausCallBack (CRtspServer *pRtspServer,int hSessionHandle,int nType, void *pInParam,void *pOutParam)
{
    int hRtspHandle = 0;
    int nRet = -1;
    ANTS_RTSPSTATUSCALLBACK fRtspStatusCallback;
    void *pRtspStatusUser = NULL;
    if (pRtspServer == NULL)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();
    if (m_fRtspStatusCallback == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }

    hRtspHandle = pRtspServer->GetCurrHandle();
    if (hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }

    fRtspStatusCallback = m_fRtspStatusCallback;
    pRtspStatusUser = m_pRtspStatusUser;
    m_hRtspServerLock.Unlock();
    Ants_RTSPV2_InParam_T *pin = (Ants_RTSPV2_InParam_T *)pInParam;
    //RTSP_DEBUG("[RTSP] status callback [%x]start nType = %d %s\n",hStreamHandleExt,nType,pin?(pin->pUrl?pin->pUrl:"nul"):"nul");
    nRet = (*fRtspStatusCallback)(hRtspHandle,hSessionHandle,nType,pInParam,pOutParam,pRtspStatusUser);
    //RTSP_DEBUG("[RTSP] status callback stop nType = %d %s\n",nType,pin?(pin->pUrl?pin->pUrl:"nul"):"nul");
    return nRet;

}


static int Tcp_RtpRtcp_DataCallback(void *pClient,unsigned int uiIPv4[4],int nPort,int nCh,int bRtcp,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,int nlen,void *pUser)
{
    int i;
    int bOwe = -1;
    CRtspServer *pServer = (CRtspServer *)pUser;
    if (pServer == NULL)
    {
        return -1;
    }
    //printf("ip = %x:%d- %d,brtcp = %d,len = %d\n",uiIPv4,nPort,nCh,bRtcp,nlen);

    return pServer->SendRTP_RTCPData((CClientSocket *)pClient,nCh,pRTPHeader,nRTPHeadLen,pAdd,nAddLen,pData,nlen);
}

int Ants_RTSPServerV2_Init()
{
    int i,j;
    //RTSP_COMMON_URL_T Result;
    // CRtspClient test;
    //  test.Open("rtsp://www.126.com:90/a/b/c/?f=b&c=d#test",1);

    m_hRtspServerLock.Init();
    m_hRtspServerLock.Lock();
    if (m_bRtspServerInit)
    {
        m_hRtspServerLock.Unlock();
        return 0;
    }
    for (i = 0; i < MAX_RTSPSERVER_NUM; i++)
    {
        m_hRtspServers[i] = NULL;
#if 0
        for (j = 0; j < MAX_RTSPSERVER_STREAM_NUM; j++)
        {
            m_hRtspStreams[i][j] = -1;
            m_hRtspRecordStreams[i][j] = -1;
        }
#endif
    }

    m_bRtspServerInit = 1;
    m_hRtspServerLock.Unlock();
    return 0;
}
//创建RTSP服务器
int Ants_RTSPServerV2_Start(int nPort)//返回handle
{
    int hRtspHandle = -1;
    CRtspServer *pServer = NULL;
    int idx;
    int nRet = -1;
    if (!m_bRtspServerInit)
    {
        RTSP_ERROR("server start not inited\n");
        return -1;
    }
    m_hRtspServerLock.Lock();
    for (idx = 0; idx < MAX_RTSPSERVER_NUM; idx++)
    {
        if (m_hRtspServers[idx] == NULL)
        {
            break;
        }
    }
    if (idx == MAX_RTSPSERVER_NUM)
    {
        m_hRtspServerLock.Unlock();
        RTSP_ERROR("server idx = %d\n",idx);
        return -1;
    }

    pServer = new CRtspServer;
    if(pServer == NULL)
    {
        m_hRtspServerLock.Unlock();
        RTSP_ERROR("server new mem failed\n");
        return -1;
    }
    nRet = pServer->Create(nPort);
    if (nRet)
    {
        m_hRtspServerLock.Unlock();
        delete pServer;
        RTSP_ERROR("server create failed\n");
        return -1;
    }
    m_hRtspServers[idx] = pServer;
    hRtspHandle = idx + 1;
    pServer->SetCurrHandle(hRtspHandle);
    pServer->SetStatusCallBack(ants_RtspStausCallBack);
    pServer->SetSendTCPDataCallBack(Tcp_RtpRtcp_DataCallback,m_hRtspServers[idx]);
    m_hRtspServerLock.Unlock();
    return hRtspHandle;

}


int Ants_RTSPServerV2_SetStatusCallBack(ANTS_RTSPSTATUSCALLBACK fxn,void *pUser)
{
    if (!m_bRtspServerInit)
    {
        return 0;
    }
    m_hRtspServerLock.Lock();
    m_fRtspStatusCallback = fxn;
    m_pRtspStatusUser = pUser;
    m_hRtspServerLock.Unlock();

    return 0;
}

int Ants_RTSPServerV2_SetStreamCallBack(int hRtspHandle,unsigned int dwSessionID,unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return 0;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();//
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//

    nRet = m_hRtspServers[hRtspHandle - 1]->SetStreamCallBack(dwSessionID,dwProp,fxn,pUser);

    m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//

    return nRet;
}

int Ants_RTSPServerV2_SetTunnelingOverHTTP(int hRtspHandle,int nHttpPort)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();

    nRet = m_hRtspServers[hRtspHandle - 1]->SetTunnelingOverHTTP(nHttpPort);

    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();
    return nRet;

}

#if 0
//创建流
int Ants_RTSPServer_CreateStream(int hRtspHandle,char *pToken,int bStreamType,int nVideoPayloadType,int nAudioPayloadType,int bFixed)//返回流handle
{
    int idx,StreamIdx;
    int nRet = -1;
    int hStreamHandle = -1,handle;
    CRtspServer *pServer = NULL;
    char *pNewToken = NULL;

    if (!m_bRtspServerInit)
    {
        return -1;
    }
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM || pToken == NULL)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    for (StreamIdx = 0; StreamIdx < MAX_RTSPSERVER_STREAM_NUM; StreamIdx++)
    {
        if (m_hRtspStreams[hRtspHandle - 1][StreamIdx] == -1)
        {
            break;
        }
    }
    if (StreamIdx == MAX_RTSPSERVER_STREAM_NUM)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }


    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();
    pNewToken = strdup(pToken);
    if (pNewToken == NULL)
    {
        m_hRtspServerLock.Lock();
        m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
        m_hRtspServerLock.Unlock();
        return -1;
    }
    int i = 0;
    while(1)
    {
        if (pNewToken[i] == 0)
        {
            break;
        }
        if(!((pNewToken[i] >= 'a' && pNewToken[i] <= 'z') ||
             (pNewToken[i] >= 'A' && pNewToken[i] <= 'Z') ||
             (pNewToken[i] >= '0' && pNewToken[i] <= '9') ||
             pNewToken[i] == '.'
            ))
        {

            pNewToken[i] = '_';
        }
        i++;
    }


    handle = m_hRtspServers[hRtspHandle - 1]->CreateStream(pNewToken,bStreamType,nVideoPayloadType,nAudioPayloadType,bFixed);
    free(pNewToken);

    if (handle < 0)
    {
        m_hRtspServerLock.Lock();
        m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
        m_hRtspServerLock.Unlock();
        return -1;
    }

    m_hRtspStreams[hRtspHandle - 1][StreamIdx] = handle;
    hStreamHandle = ((StreamIdx + 1) << 16) | hRtspHandle;

    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();
    return hStreamHandle;

}
#endif


//添加或者改变
int Ants_RTSPServerV2_AddAppStream(int hRtspHandle,int hSessionHandle,int nAppPayloadType,unsigned int dwClockRate,char *szAppName)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();//
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
    nRet = m_hRtspServers[hRtspHandle - 1]->AddAppStream(hSessionHandle,nAppPayloadType,dwClockRate,szAppName);
    m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}

#if 0
//删除
int Ants_RTSPServer_DeleteAppStream(int hStreamHandle,int nAppPayloadType)
{
    int idx,StreamIdx;
    int nRet = -1;
    int hRtspHandle;
    if (!m_bRtspServerInit)
    {
        return -1;
    }
    hRtspHandle = hStreamHandle & 0xFFFF;
    hStreamHandle >>= 16;
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
    if (hStreamHandle < 1 || hStreamHandle > MAX_RTSPSERVER_STREAM_NUM)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();//
    if (m_hRtspServers[hRtspHandle - 1] == NULL ||
        m_hRtspStreams[hRtspHandle - 1][hStreamHandle - 1] < 0)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
    nRet = m_hRtspServers[hRtspHandle - 1]->DeleteAppStream(m_hRtspStreams[hRtspHandle - 1][hStreamHandle - 1],nAppPayloadType);
    m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;

}
#endif

//nCmdType:
//          1- 设置流类型 ,ANTS_RTSPSERVER_STREAMTYPE_ALL/
//          2- 设置视频负载类型自动 0-自动,1-非自动
//          3- 设置音频负载类型自动 0-自动,1-非自动
//          4- 设置视频负载类型
//          5- 设置音频负载类型

int Ants_RTSPServerV2_SetConfig(int hRtspHandle,int hStreamHandle,int nCmdType,void *pData,int nDataSize)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }
    if (nCmdType == 9)
    {
        if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
        {
            return -1;
        }
        m_hRtspServerLock.Lock();//
        nRet = m_hRtspServers[hRtspHandle - 1]->SetConfig(-1,nCmdType,pData,nDataSize);
        m_hRtspServerLock.Unlock();//
        return nRet;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();//
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//

    nRet = m_hRtspServers[hRtspHandle - 1]->SetConfig(hStreamHandle,nCmdType,pData,nDataSize);

    m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}


int Ants_RTSPServerV2_SetStreamInfo(int hRtspHandle, int hSessionHandle,int bStreamType,int nVideoPayloadType,int nAudioPayloadType)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//

    nRet = m_hRtspServers[hRtspHandle - 1]->SetStreamInfo(hSessionHandle,bStreamType,nVideoPayloadType,nAudioPayloadType);

	m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}


#if 1
int Ants_RTSPServerV2_InputAntsCombData(int hRtspHandle,int hStreamHandle,int nType,int nChan,int nStreamIdx,void *pData,int nDataSize)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
	
    m_hRtspServerLock.Lock();//

    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//

	nRet = m_hRtspServers[hRtspHandle - 1]->InputAntsCombData(hStreamHandle,nType,nChan,nStreamIdx,pData,nDataSize);

	m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}


int Ants_RTSPServerV2_InputDataV2(int hRtspHandle,int hStreamHandle,int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();//
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
    nRet = m_hRtspServers[hRtspHandle - 1]->InputDataEx(hStreamHandle,nPayloadType,nFrameType,pData,nDataSize,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}


int Ants_RTSPServerV2_InputDataEx(int hRtspHandle,int hStreamHandle,int nPayloadType,void *pData,int nDataSize,int64_t timestamp,int bRelTimeStamp)
{
    return Ants_RTSPServerV2_InputDataV2(hRtspHandle,hStreamHandle,nPayloadType,0,pData,nDataSize,bRelTimeStamp?timestamp:0,(bRelTimeStamp)?0:(timestamp/1000000),(bRelTimeStamp)?0:(timestamp%1000000),bRelTimeStamp?1:2);
}


//输入流
int Ants_RTSPServerV2_InputData(int hRtspHandle,int hStreamHandle,void *pData,int nDataSize,int64_t timestamp,int bAudio,int bAdpcm2G711u)
{
    int nRet = -1;
    uint32_t Reltimestamp;
    uint32_t AbstimestampSec,AbstimestampUSec;
    int bValid;
    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();//
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();//
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
    nRet = m_hRtspServers[hRtspHandle - 1]->InputData(hStreamHandle,-1,bAudio?ANTS_RTSPSERVER_FRAMETYPE_AUDIO:0,pData,nDataSize,timestamp,0,0,1);
    m_hRtspServerLock.Lock();//
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}


int Ants_RTSPServerV2_InputAntsData(int hRtspHandle,int hStreamHandle,void *pData,int nDataSize,int bAdpcm2G711u)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
    nRet = m_hRtspServers[hRtspHandle - 1]->InputAntsData(hStreamHandle,pData,nDataSize,bAdpcm2G711u);
    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}
#endif

int Ants_RTSPServerV2_SetMulticastAddressIPv4(int hRtspHandle, int hSessionHandle,int bAudio,const char *pIP,int nPort,int nTTL)
{
    int nRet = -1;
	uint32_t ip[4]={0,0,0,0};
	int bIPv6 = 0;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM || pIP == NULL)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
	if (strstr(pIP,":"))
	{
		inet_pton(AF_INET6,pIP,&ip);
		bIPv6 = 1;
	}
	else
	{
		ip[0] = inet_addr(pIP);
	}
	

    nRet = m_hRtspServers[hRtspHandle - 1]->SetMulticastAddressIPv4(hSessionHandle,ip,nPort,nTTL,bAudio,bIPv6);
    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}


int Ants_RTSPServerV2_CloseStream(int hRtspHandle, int hSessionHandle)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return 0;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//

    nRet = m_hRtspServers[hRtspHandle - 1]->CloseStream(hSessionHandle);
	
    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    
    return nRet;
}


#if 0
int Ants_RTSPServer_SetMulticastAddressIPv4NoApply(int hRtspHandle, int hStreamHandle,int bAudio,const char *pIP,int nPort,int nTTL)
{
    int idx,StreamIdx;
    int nRet = -1;
    int hRtspHandle;
    uint32_t ip;
    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }

    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }

    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//
    ip = inet_addr(pIP);

    nRet = m_hRtspServers[hRtspHandle - 1]->SetMulticastAddressIPv4NoApply(hStreamHandle,ip,nPort,nTTL,bAudio);
    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}
#endif


int Ants_RTSPServerV2_EnableMulticast(int hRtspHandle,int hSessionHandle,int bEnable)
{
    int nRet = -1;

    if (!m_bRtspServerInit)
    {
        return -1;
    }

    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }

    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }

    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();//

    nRet = m_hRtspServers[hRtspHandle - 1]->SetSupportMulticast(hSessionHandle,bEnable);
    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}

//销毁流
#if 0
int Ants_RTSPServer_DestroyStreamByToken(char *pToken)
{

    int nRet = -1;
    int hRtspHandle;
    if (pToken == NULL)
    {
        return -1;
    }

    for (hRtspHandle = 0; hRtspHandle < MAX_RTSPSERVER_NUM; hRtspHandle++)
    {
        m_hRtspServerLock.Lock();
        if (m_hRtspServers[hRtspHandle] == NULL)
        {
            m_hRtspServerLock.Unlock();
            continue;
        }
        if(m_hRtspServers[hRtspHandle]->IsWaitDelete())
        {
            m_hRtspServerLock.Unlock();
            continue;
        }
        m_hRtspServers[hRtspHandle]->SetUsed(1);
        m_hRtspServerLock.Unlock();

        nRet = Ants_RTSPServer_DestroyStreamByHandleToken(hRtspHandle + 1,pToken);
        if (!nRet)
        {
            m_hRtspServerLock.Lock();
            m_hRtspServers[hRtspHandle]->SetUsed(0);
            m_hRtspServerLock.Unlock();
            break;
        }
        m_hRtspServerLock.Lock();
        m_hRtspServers[hRtspHandle]->SetUsed(0);
        m_hRtspServerLock.Unlock();
    }

    return nRet;

}
int Ants_RTSPServer_DestroyStreamByHandleToken(int hRtspHandle,char *pToken)
{
    int idx;
    int nRet = -1;
    int hStreamhandle = -1;
    char *pStreamToken = NULL;
    if (!m_bRtspServerInit)
    {
        return -1;
    }
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
    if (pToken == NULL)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();
    for (idx = 0; idx < MAX_RTSPSERVER_STREAM_NUM; idx++)
    {
        if (m_hRtspStreams[hRtspHandle - 1][idx] >= 0)
        {
            pStreamToken = m_hRtspServers[hRtspHandle - 1]->GetStreamToken(m_hRtspStreams[hRtspHandle - 1][idx]);

            if(pStreamToken != NULL && !strcmp(pToken,pStreamToken))
            {
                nRet = m_hRtspServers[hRtspHandle - 1]->DestroyStream(m_hRtspStreams[hRtspHandle - 1][idx]);
                m_hRtspStreams[hRtspHandle - 1][idx] = -1;
                break;
            }

        }
    }
    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();
    return nRet;
}

int Ants_RTSPServer_DestroyStream(int hStreamHandle)
{
    int idx,StreamIdx;
    int nRet = -1;
    int hRtspHandle;
    int hStreamHandle_Intra;
    if (!m_bRtspServerInit)
    {
        return -1;
    }
    hRtspHandle = hStreamHandle & 0xFFFF;
    hStreamHandle >>= 16;
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
    if (hStreamHandle < 1 || hStreamHandle > MAX_RTSPSERVER_STREAM_NUM)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL ||
        m_hRtspStreams[hRtspHandle - 1][hStreamHandle - 1] < 0)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    hStreamHandle_Intra = m_hRtspStreams[hRtspHandle - 1][hStreamHandle - 1] ;
    m_hRtspStreams[hRtspHandle - 1][hStreamHandle - 1] = -1;
    m_hRtspServerLock.Unlock();//
    nRet = m_hRtspServers[hRtspHandle - 1]->DestroyStream(hStreamHandle_Intra);

    m_hRtspServerLock.Lock();

    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();//
    return nRet;
}
#endif

//销毁RTSP服务器
int Ants_RTSPServerV2_Stop(int hRtspHandle)
{
    int nRet = -1;
    CRtspServer  *pServer = NULL;
    if (!m_bRtspServerInit)
    {
        return -1;
    }
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();

    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return 0;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return 0;
    }
    m_hRtspServers[hRtspHandle - 1]->SetDeleteFlag();
    pServer = m_hRtspServers[hRtspHandle - 1];

    m_hRtspServerLock.Unlock();
    while(1)
    {
        m_hRtspServerLock.Lock();
        if(!pServer->IsUsed())
        {
            m_hRtspServerLock.Unlock();
            break;
        }
        m_hRtspServerLock.Unlock();
        Ants_WaitTime(0,100000);
    }

#if 0
    for (idx = 0; idx < MAX_RTSPSERVER_STREAM_NUM; idx++)
    {
        m_hRtspStreams[hRtspHandle - 1][idx] = -1;
    }
#endif

    m_hRtspServers[hRtspHandle - 1] = NULL;
    pServer->Destroy();
    delete pServer;

    nRet = 0;
    return nRet;
}

int64_t Ants_RTSPServer_GetCurrentTime()
{
    int64_t tim = 0;
    struct timeval tv;
#ifdef WIN32
    SYSTEMTIME systemtime;
    FILETIME filetime;
    GetSystemTime(&systemtime);
    SystemTimeToFileTime(&systemtime,&filetime);
    tim = ( ((unsigned __int64)(filetime.dwHighDateTime) << 32) + (unsigned __int64)(filetime.dwLowDateTime) ) / 10000;
    // tim-= 11644473600000000ui64; // EPOCH
#else
    gettimeofday(&tv,0);
    tim = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

    return tim;
}
void Ants_RTSPServer_WaitTime(int nSec,int MicroSec)
{
    Ants_WaitTime(nSec,MicroSec);

}


int Ants_RTSPServerV2_SetStreamControlCallBack(int hRtspHandle, RTSPServer_StreamControl *pStreamControl, void *pUser)
{
    if (!m_bRtspServerInit)
    {
        return -1;
    }
    if (hRtspHandle < 1 || hRtspHandle > MAX_RTSPSERVER_NUM)
    {
        return -1;
    }
    m_hRtspServerLock.Lock();
    if (m_hRtspServers[hRtspHandle - 1] == NULL)
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    if(m_hRtspServers[hRtspHandle - 1]->IsWaitDelete())
    {
        m_hRtspServerLock.Unlock();
        return -1;
    }
    m_hRtspServers[hRtspHandle - 1]->SetUsed(1);
    m_hRtspServerLock.Unlock();

    m_hRtspServers[hRtspHandle - 1]->SetStreamControlCallBack(pStreamControl, pUser);

    m_hRtspServerLock.Lock();
    m_hRtspServers[hRtspHandle - 1]->SetUsed(0);
    m_hRtspServerLock.Unlock();

    return 0;
}

static int H265RTP_SplitPacket(ANTS_RTSP_H264_RTP_PARAM_T *pParam)
{
    //输入完整NAL数据,不包括起始码
    int nPos;
    int bMark;
    int nSendSize;
    int nRet = -1;
    unsigned char LayerID,TID,nalType,forbidden_bit;

    uint8_t FU[2],FUheader0,FUheader1,FUheader2;
    uint8_t *pPackBuf = NULL;
    int nHeadSize;
    int nPackSize;
    uint8_t pheaderBuffer[12 + 4 + 12 + 4];

    uint8_t *data;
    size_t len;
    int nTotalSize;
    int nBuf;
    uint16_t *pTCPPackSize;

    uint8_t uNAL_org[2];

    RTPHeader *pHeader;
    RTPExtensionOnvifHeader *pExtOnvifHeader;
    RTPExtensionHeader *pExtHeader;
    //int nSendLen;
    nHeadSize = 4 + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) + sizeof(RTPExtensionOnvifHeader);
    nPackSize = pParam->nMTUSize - nHeadSize - 60;
    memset(pheaderBuffer,0,sizeof(pheaderBuffer));

    pheaderBuffer[0] = '$';
    pheaderBuffer[1] = 0;
    pTCPPackSize = (uint16_t *)(pheaderBuffer + 2);
    pHeader = (RTPHeader *)(pheaderBuffer + 4);
    pExtHeader = (RTPExtensionHeader *)(pHeader + 1);
    pExtOnvifHeader = (RTPExtensionOnvifHeader *)(pExtHeader + 1);

    nTotalSize = 0;
    if (pParam->nStartIdx == pParam->nStopIdx)
    {
        nTotalSize = pParam->nStopPos - pParam->nStartPos;
    }
    else
    {
        for (nBuf = pParam->nStartIdx; nBuf <= pParam->nStopIdx; nBuf++)
        {
            if (nBuf == pParam->nStartIdx)
            {
                nTotalSize += pParam->nInDataSize[nBuf] - pParam->nStartPos;

            }
            else if (nBuf == pParam->nStopIdx)
            {
                nTotalSize += pParam->nStopPos;
            }
            else
            {
                nTotalSize += pParam->nInDataSize[nBuf];
            }

        }
    }






    // header

    pHeader->version = 2;
    pHeader->marker = 1;
    pHeader->payloadtype = ANTS_RTSPSERVER_PAYLOADTYPE_H265;
    pHeader->ssrc = pParam->dwSSRC;

    pHeader->timestamp = htonl(pParam->Reltimestamp);
    //nSendLen = len+sizeof(RTPHeader);

    pHeader->extension = 1;
    // nSendLen += sizeof(RTPExtensionOnvifHeader);
    // nSendLen += sizeof(RTPExtensionHeader);
    pExtHeader->extid = htons(0xABAC);
    pExtHeader->length = htons(3);
    pExtOnvifHeader->padding = 0;
    pExtOnvifHeader->seq = 0; // 动态填写，实时流时忽略
    pExtOnvifHeader->mbz = 0;
    pExtOnvifHeader->ntpTimeStamp0 = htonl(pParam->ntpTimeStamp0);
    pExtOnvifHeader->ntpTimeStamp1 = htonl(pParam->ntpTimeStamp1);
    pExtOnvifHeader->discontinuity = pParam->discontinuity;
    pExtOnvifHeader->eSectionEnd = pParam->eSectionEnd;
    pExtOnvifHeader->cIFrameStart =pParam->cIFrameStart;




    // printf("[%s] len = %d PackSize = %d type = %d nri = %d\n",__FUNCTION__,len,PackSize,pNALU->TYPE,pNALU->NRI);

    if (nTotalSize <= nPackSize)
    {

        pTCPPackSize[0] = htons(nTotalSize + nHeadSize - 4);
        pParam->wSeq++;
        //printf("wseq = %u\n",pParam->wSeq);
        pHeader->sequencenumber = htons(pParam->wSeq);
        uNAL_org[0] = *((uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos);
        nalType = HEVC_NALU_GET_TYPE(uNAL_org[0]);
        if(nalType > HEVC_NAL_SLICE_RSV_IRAP_VCL23)
        {
            pHeader->marker = 0;
        }

        memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,pheaderBuffer,nHeadSize);
        pParam->nWritePos += nHeadSize;
        if (pParam->nStartIdx == pParam->nStopIdx)
        {

            memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos,nTotalSize);
            pParam->nWritePos += nTotalSize;
        }
        else
        {
            for (nBuf = pParam->nStartIdx; nBuf <= pParam->nStopIdx; nBuf++)
            {
                if (nBuf == pParam->nStartIdx)
                {
                    memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[nBuf] + pParam->nStartPos,pParam->nInDataSize[nBuf] - pParam->nStartPos);
                    pParam->nWritePos += pParam->nInDataSize[nBuf] - pParam->nStartPos;
                }
                else if (nBuf == pParam->nStopIdx)
                {
                    memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[nBuf],pParam->nStopPos);
                    pParam->nWritePos += pParam->nStopPos;
                }
                else
                {
                    memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[nBuf],pParam->nInDataSize[nBuf]);
                    pParam->nWritePos += pParam->nInDataSize[nBuf];
                }

            }
        }

        return 0;//nTotalSize;

    }
    //开始分片
    nPos = 0;
    bMark = 0;
    nSendSize = 0;


    // pFU = (FU_INDICATOR *)&FU;
    // pFUHeader = (FU_HEADER *)&FUheader;




    uNAL_org[0] = *((uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos);
    uNAL_org[1] = *((uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos+1);


    nalType = HEVC_NALU_GET_TYPE(uNAL_org[0]);
    forbidden_bit = HEVC_NALU_GET_F(uNAL_org[0]);
    LayerID = HEVC_NALU_GET_LAYERID(uNAL_org[0],uNAL_org[1]);
    TID = HEVC_NALU_GET_TID(uNAL_org[1]);


    HEVC_NALU_RESET(FU[0],FU[1]);
    HEVC_NALU_SET_F(FU[0],forbidden_bit);
    HEVC_NALU_SET_LAYERID(FU[0],FU[1],LayerID);
    HEVC_NALU_SET_TID(FU[1],TID);
    HEVC_NALU_SET_TYPE(FU[0],HEVC_FU_TYPE);

    //start
    HEVC_FU_RESET(FUheader0);
    HEVC_FU_SET_TYPE(FUheader0,nalType);
    HEVC_FU_SET_E(FUheader0,0);
    HEVC_FU_SET_S(FUheader0,1);

    //mid
    HEVC_FU_RESET(FUheader1);
    HEVC_FU_SET_TYPE(FUheader1,nalType);
    HEVC_FU_SET_E(FUheader1,0);
    HEVC_FU_SET_S(FUheader1,0);

    //end
    HEVC_FU_RESET(FUheader2);
    HEVC_FU_SET_TYPE(FUheader2,nalType);
    HEVC_FU_SET_E(FUheader2,1);
    HEVC_FU_SET_S(FUheader2,0);

    int nPackNum,nLeftSize = 0,nPktIdx = 0,nwPos;
    int nCurrIdx,nCurrPos,nPackDataSize;
    nPackDataSize = nPackSize - 3;// FU头3个字节,264占2个字节
    nPackNum = (nTotalSize - 2/* NAL头2个字节，264占1个字节 */ + (nPackDataSize) - 1) / (nPackDataSize);
    nLeftSize = nPackSize;
    nCurrIdx = pParam->nStartIdx;
    nCurrPos = pParam->nStartPos + 2; // 偏移|NAL头字节
    for (nPktIdx = 0; nPktIdx < nPackNum; nPktIdx++)
    {
        bMark = 0;
        pPackBuf = (uint8_t *)(((uint8_t *)(pParam->pOutData)) + pParam->nWritePos + nHeadSize);
        nPos = 0;
        nwPos = 0;
        if (nPktIdx == 0)
        {
            pPackBuf[0] = FU[0];
            pPackBuf[1] = FU[1];
            pPackBuf[2] = FUheader0;
            nSendSize = nPackDataSize;
            nPos +=2;// NAL头字节
        }
        else if (nPktIdx == nPackNum - 1)
        {
            bMark = 1;
            pPackBuf[0] = FU[0];
            pPackBuf[1] = FU[1];
            pPackBuf[2] = FUheader2;
            nSendSize = (nTotalSize - 2/* NAL 头字节*/) - (nPackDataSize) * nPktIdx;
        }
        else
        {
            pPackBuf[0] = FU[0];
            pPackBuf[1] = FU[1];
            pPackBuf[2] = FUheader1;
            nSendSize = nPackDataSize;
        }
        nwPos += 3; // FU 头字节
        nLeftSize = nSendSize;
        //nLeftSize -= 2;
        pHeader->marker = bMark;
        pTCPPackSize[0] = htons(nSendSize + 3/* FU 头字节*/ + nHeadSize - 4 /*TCP 头长度 $*/);
        pParam->wSeq++;
        // printf("wseq = %u\n",pParam->wSeq);
        pHeader->sequencenumber = htons(pParam->wSeq);
        // write header
        memcpy((uint8_t *)pParam->pOutData+pParam->nWritePos,pheaderBuffer,nHeadSize);
        pParam->nWritePos += nHeadSize;
        // write data


        for (; nCurrIdx <= pParam->nStopIdx;)
        {

            len = pParam->nInDataSize[nCurrIdx] - nCurrPos;
            data = (uint8_t *)pParam->pInData[nCurrIdx] + nCurrPos;
            if (pParam->nStopIdx == nCurrIdx)
            {
                len = pParam->nStopPos - nCurrPos;
            }

            if (len <= nLeftSize)
            {
                // 最后一包
                memcpy(pPackBuf + nwPos,data,len);
                nwPos += len;
                nLeftSize -= len;
                nCurrPos += len;

                nCurrIdx++;
                nCurrPos = 0;

            }
            else
            {
                memcpy(pPackBuf + nwPos,data,nLeftSize);
                nwPos += nLeftSize;
                nCurrPos += nLeftSize;
                nLeftSize -= nLeftSize;


                break;

            }

        }
        pParam->nWritePos += nSendSize + 3; /* FU 头长度*/

    }







    return 0;//pParam->nWritePos;



}
int Ants_RTSPServer_H265toRTP_InputData(ANTS_RTSP_H264_RTP_PARAM_T *pParam)
{
    char *pNalStart,*pNalEnd,*pCurr,*data;
    int nBuf,len;
    int NalLen;
    uint32_t uStartCode = 0;
    int nOutPos = 0,nPackSize;
    int nRet = 0;
    uint32_t msw = 0,lsw = 0;
    AntsFrameHeader tFrameHeader;
    if (pParam == NULL)
    {
        return -1;
    }
    if (pParam->pOutData == NULL || pParam->nOutSize == 0)
    {
        return -1;
    }
    Ants_rtsp_GetRTP2NTPTime(pParam->AbstimestampSec,pParam->AbstimestampUSec,&pParam->ntpTimeStamp0,&pParam->ntpTimeStamp1);
    if(pParam->bIFrame)
    {
        pParam->cIFrameStart = 1;
        pParam->discontinuity = 1;
    }
    else
    {
        pParam->cIFrameStart = 0;
        pParam->discontinuity = 0;
    }
    if (pParam->nMTUSize < 500)
    {
        pParam->nMTUSize = 1500;
    }
    pParam->nStartIdx = -1;
    pParam->nStartPos = 0;
    pParam->nWritePos = 0;
    pParam->nWritePos += sizeof(AntsFrameHeader);
    memset(&tFrameHeader,0,sizeof(tFrameHeader));
    nPackSize = pParam->nMTUSize - 60 - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionOnvifHeader);
    for (nBuf = 0; nBuf < pParam->nInCount;nBuf++)
    {
        pNalStart = NULL;
        pNalEnd = NULL;
        data = (char *)pParam->pInData[nBuf];
        pCurr = data;
        len = pParam->nInDataSize[nBuf];
        // printf("[%d]size = %d\n",nBuf,len);
        while(1)
        {
            if (pCurr - (char *)data >= len)
            {
                if (nBuf == pParam->nInCount -1)
                {
                    if (pParam->nStartIdx != -1)
                    {
                        pParam->nStopIdx = nBuf;
                        pParam->nStopPos = pCurr - data;
                        nRet = H265RTP_SplitPacket(pParam);
                        //printf("[%d,%d]->[%d,%d] %02x\n",pParam->nStartIdx,pParam->nStartPos,pParam->nStopIdx,pParam->nStopPos,pParam->pInData[pParam->nStartIdx][pParam->nStartPos]);
                        pParam->nStartIdx = -1;
                        pParam->nStartPos = 0;
                        pParam->nStopIdx = -1;
                        pParam->nStopPos = 0;
                        if (nRet)
                        {
                            break;
                        }
                    }
                }

                break;
            }
            uStartCode >>= 8;
            uStartCode |= (*pCurr) << 24;
            if (uStartCode == 0x01000000)
            {
                if (pParam->nStartIdx == -1)
                {
                    pParam->nStartIdx = nBuf;
                    pParam->nStartPos = pCurr - data + 1;
                    if (pParam->nStartPos == pParam->nInDataSize[nBuf])
                    {
                        pParam->nStartIdx++;
                        pParam->nStartPos = 0;
                    }
                }
                else
                {
                    pParam->nStopIdx = nBuf;
                    pParam->nStopPos = pCurr - data - 3;
                    nRet = H265RTP_SplitPacket(pParam);
                    // printf("[%d,%d]->[%d,%d] %02x\n",pParam->nStartIdx,pParam->nStartPos,pParam->nStopIdx,pParam->nStopPos,pParam->pInData[pParam->nStartIdx][pParam->nStartPos]);
                    pParam->nStartIdx = nBuf;
                    pParam->nStartPos = pParam->nStopPos + 4;
                    if (pParam->nStartPos == pParam->nInDataSize[nBuf])
                    {
                        pParam->nStartIdx++;
                        pParam->nStartPos = 0;
                    }
                    pParam->nStopIdx = -1;
                    pParam->nStopPos = 0;
                    if (nRet)
                    {
                        break;
                    }

                }

            }

            pCurr++;

        }

    }
    tFrameHeader.uiStartId = ANTS_FRAME_STARTCODE;
    tFrameHeader.uiFrameLen = pParam->nWritePos - sizeof(AntsFrameHeader);
    tFrameHeader.uiFrameNo = pParam->dwFrameNo++;
    tFrameHeader.uiFrameTime = pParam->AbstimestampSec;
    tFrameHeader.uiFrameTickCount = pParam->AbstimestampUSec;
    tFrameHeader.uiFrameType = pParam->bIFrame?AntsPktIFrames:AntsPktPFrames;
    tFrameHeader.uiTimeStamp = pParam->Reltimestamp;
    tFrameHeader.uMedia.struVideoHeader.cCodecId = AntsH265_RTP_PACK;
    tFrameHeader.uMedia.struVideoHeader.usWidth = pParam->nWidth;
    tFrameHeader.uMedia.struVideoHeader.usHeight = pParam->nHeight;
    memcpy(pParam->pOutData,&tFrameHeader,sizeof(tFrameHeader));
    // printf("====================== ok size = %d\n",pParam->nWritePos);
    pParam->nOutSize = pParam->nWritePos;
    return 0;
}
int Ants_RTSPServer_RTPtoH265_InputData(ANTS_RTSP_RTP_H264_PARAM_T *pParam)
{
    int bHasAntsHeader = 0;
    AntsFrameHeader tFrameHeader;
    int nReadPos = 0,nWritePos = 0,nPktLen;
    unsigned char *pPack,*pPktData;
    unsigned char LayerID,TID,nalType,forbidden_bit;
    unsigned char fuType;
    uint8_t uNAL_org[2];
    int bMark,bExtHeader,cc,nExtLen,nRtpLen;
    int nPktDataSize,nWriteDataLen = 0;
    int s,e,r;
    if (pParam == NULL)
    {
        return -1;
    }
    if (pParam->pInData == NULL || pParam->nInDataSize == 0 ||
        pParam->pOutData == NULL || pParam->nOutSize < pParam->nInDataSize)
    {
        return -1;
    }
    if (pParam->pInData[0] != '$')
    {
        memcpy(&tFrameHeader,pParam->pInData,sizeof(AntsFrameHeader));
        if (tFrameHeader.uiStartId == ANTS_FRAME_STARTCODE)
        {
            bHasAntsHeader = 1;
            nWritePos += sizeof(AntsFrameHeader);
            nReadPos += sizeof(AntsFrameHeader);
        }
    }
    if (pParam->pInData[nReadPos] != '$')
    {
        return -1;
    }
    pPack = pParam->pInData + nReadPos;
    nWriteDataLen = 0;
    while(1)
    {
        if (nReadPos >= pParam->nInDataSize)
        {
            break;
        }
        if (pPack[0] != '$')
        {
            break;
        }
        nPktLen = pPack[2];
        nPktLen <<= 8;
        nPktLen += pPack[3];
        if (nReadPos + nPktLen + 4> pParam->nInDataSize)
        {
            break;
        }
        // do
        bMark = (pPack[4 + 1] >> 7) & 1;
        cc = (pPack[4 + 0] & 0x0F);
        bExtHeader = (pPack[4 + 0] >> 4) & 1;
        nExtLen = 0;
        if (bExtHeader)
        {
            nExtLen = (pPack[4 + sizeof(struct RTPHeader) + 2] << 8) | (pPack[4 + sizeof(struct RTPHeader) + 3]) * 4 + 4;
        }

        nRtpLen =sizeof(struct RTPHeader) + cc * 4 + nExtLen;

        pPktData = pPack + nRtpLen + 4;
        nPktDataSize = nPktLen - nRtpLen;
        fuType = HEVC_NALU_GET_TYPE(pPktData[0]);
        forbidden_bit = HEVC_NALU_GET_F(pPktData[0]);
        LayerID = HEVC_NALU_GET_LAYERID(pPktData[0],pPktData[1]);
        TID = HEVC_NALU_GET_TID(pPktData[1]);
        if (fuType == HEVC_FU_TYPE)
        {
            // 分片的
            s = HEVC_FU_GET_S(pPktData[2]);
            e = HEVC_FU_GET_E(pPktData[2]);
            nalType = HEVC_FU_GET_TYPE(pPktData[2]);
            if(s)
            {


                HEVC_NALU_RESET(uNAL_org[0],uNAL_org[1]);
                HEVC_NALU_SET_F(uNAL_org[0],forbidden_bit);
                HEVC_NALU_SET_TYPE(uNAL_org[0],nalType);
                HEVC_NALU_SET_LAYERID(uNAL_org[0],uNAL_org[1],LayerID);
                HEVC_NALU_SET_TID(uNAL_org[1],TID);
                pParam->pOutData[nWritePos] = 0x00;
                pParam->pOutData[nWritePos + 1] = 0x00;
                pParam->pOutData[nWritePos + 2] = 0x00;
                pParam->pOutData[nWritePos + 3] = 0x01;
                pParam->pOutData[nWritePos + 4] = uNAL_org[0];
                pParam->pOutData[nWritePos + 5] = uNAL_org[1];
                nWritePos += 6;
                nWriteDataLen += 6;
            }

            memcpy(pParam->pOutData + nWritePos,pPktData + 3 ,nPktDataSize - 3 /* FU 头*/);
            nWritePos += nPktDataSize - 3;

            nWriteDataLen += nPktDataSize - 3;
        }
        else
        {
            // 不分片
            pParam->pOutData[nWritePos] = 0x00;
            pParam->pOutData[nWritePos + 1] = 0x00;
            pParam->pOutData[nWritePos + 2] = 0x00;
            pParam->pOutData[nWritePos + 3] = 0x01;
            nWritePos += 4;
            memcpy(pParam->pOutData + nWritePos,pPktData,nPktDataSize);
            nWritePos += nPktDataSize;
            nWriteDataLen += nPktDataSize + 4;
        }


        pPack += 4 + nPktLen;
        nReadPos += 4 + nPktLen;
    }

    if (bHasAntsHeader)
    {
        tFrameHeader.uiFrameLen = nWriteDataLen;
        tFrameHeader.uMedia.struVideoHeader.cCodecId = AntsH265;
        memcpy(pParam->pOutData,&tFrameHeader,sizeof(tFrameHeader));
    }

    pParam->nOutSize = nWritePos;

    return 0;
}

// H.264
#define NALU_GET_F(a) (((a)>>7)&1)
#define NALU_GET_NRI(a) (((a)>>5)&3)
#define NALU_GET_TYPE(a) (((a)>>0)&31)

#define NALU_RESET(a)  (a)=0
#define NALU_SET_F(a,f) (a) |= (((f)&1) << 7)
#define NALU_SET_NRI(a,nri) (a) |= (((nri)&3) << 5)
#define NALU_SET_TYPE(a,type) (a) |= (((type)&31) << 0)

#define FU_RESET(a)  (a)=0
#define FU_SET_F(a,f) (a) |= (((f)&1) << 7)
#define FU_SET_NRI(a,nri) (a) |= (((nri)&3) << 5)
#define FU_SET_TYPE(a,type) (a) |= (((type)&31) << 0)

#define FU_GET_F(f)  (((f) >> 7)&1)
#define FU_GET_NRI(nri)  (((nri) >> 5)&3)
#define FU_GET_TYPE(type)  (((type) >> 0)&31)

#define FUHEADER_RESET(a) (a)=0
#define FUHEADER_SET_TYPE(a,type) (a) |= (((type)&31) << 0)
#define FUHEADER_SET_R(a,r) (a) |= (((r)&1) << 5)
#define FUHEADER_SET_E(a,e) (a) |= (((e)&1) << 6)
#define FUHEADER_SET_S(a,s) (a) |= (((s)&1) << 7)

#define FUHEADER_GET_TYPE(a) (((a) >> 0)&31)
#define FUHEADER_GET_R(a)   (((a) >> 5)&1)
#define FUHEADER_GET_E(a)  (((a) >> 6)&1)
#define FUHEADER_GET_S(a)  (((a) >> 7)&1)
static int H264RTP_SplitPacket(ANTS_RTSP_H264_RTP_PARAM_T *pParam)
{
    //输入完整NAL数据,不包括起始码
    int nPos;
    int bMark;
    int nSendSize;
    int nRet = -1;
    unsigned char NRI,nalType,forbidden_bit;
    FU_INDICATOR *pFU;
    NALU_HEADER *pNALU;
    FU_HEADER *pFUHeader;
    uint8_t FU,FUheader0,FUheader1,FUheader2;
    uint8_t *pPackBuf = NULL;
    int nHeadSize;
    int nPackSize;
    uint8_t pheaderBuffer[12 + 4 + 12 + 4];

    uint8_t *data;
    size_t len;
    int nTotalSize;
    int nBuf;
    uint16_t *pTCPPackSize;

    uint8_t uNAL_org;

    RTPHeader *pHeader;
    RTPExtensionOnvifHeader *pExtOnvifHeader;
    RTPExtensionHeader *pExtHeader;
    //int nSendLen;
    nHeadSize = 4 + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) + sizeof(RTPExtensionOnvifHeader);
    nPackSize = pParam->nMTUSize - 60 - nHeadSize;
     memset(pheaderBuffer,0,sizeof(pheaderBuffer));

    pheaderBuffer[0] = '$';
    pheaderBuffer[1] = 0;
    pTCPPackSize = (uint16_t *)(pheaderBuffer + 2);
    pHeader = (RTPHeader *)(pheaderBuffer + 4);
    pExtHeader = (RTPExtensionHeader *)(pHeader + 1);
    pExtOnvifHeader = (RTPExtensionOnvifHeader *)(pExtHeader + 1);

    nTotalSize = 0;
    if (pParam->nStartIdx == pParam->nStopIdx)
    {
        nTotalSize = pParam->nStopPos - pParam->nStartPos;
    }
    else
    {
        for (nBuf = pParam->nStartIdx;nBuf <= pParam->nStopIdx;nBuf++)
        {
            if (nBuf == pParam->nStartIdx)
            {
                nTotalSize += pParam->nInDataSize[nBuf] - pParam->nStartPos;
                
            }
            else if (nBuf == pParam->nStopIdx)
            {
                nTotalSize += pParam->nStopPos;
            }
            else
            {
                nTotalSize += pParam->nInDataSize[nBuf];
            }
           
        }
    }
   

    

   
   
   // header
   
    pHeader->version = 2;
    pHeader->marker = 1;
    pHeader->payloadtype = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
    pHeader->ssrc = pParam->dwSSRC;
   
    pHeader->timestamp = htonl(pParam->Reltimestamp);
    //nSendLen = len+sizeof(RTPHeader);

    pHeader->extension = 1;
   // nSendLen += sizeof(RTPExtensionOnvifHeader);
   // nSendLen += sizeof(RTPExtensionHeader);
    pExtHeader->extid = htons(0xABAC);
    pExtHeader->length = htons(3);
    pExtOnvifHeader->padding = 0;
    pExtOnvifHeader->seq = 0; // 动态填写，实时流时忽略
    pExtOnvifHeader->mbz = 0;
    pExtOnvifHeader->ntpTimeStamp0 = htonl(pParam->ntpTimeStamp0);
    pExtOnvifHeader->ntpTimeStamp1 = htonl(pParam->ntpTimeStamp1);
    pExtOnvifHeader->discontinuity = pParam->discontinuity;
    pExtOnvifHeader->eSectionEnd = pParam->eSectionEnd;
    pExtOnvifHeader->cIFrameStart =pParam->cIFrameStart;


    

    // printf("[%s] len = %d PackSize = %d type = %d nri = %d\n",__FUNCTION__,len,PackSize,pNALU->TYPE,pNALU->NRI);
    
    if (nTotalSize <= nPackSize)
    {
        pTCPPackSize[0] = htons(nTotalSize + nHeadSize - 4);
        pParam->wSeq++;
        //printf("wseq = %u\n",pParam->wSeq);
        pHeader->sequencenumber = htons(pParam->wSeq);
        uNAL_org = *((uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos);
        nalType = NALU_GET_TYPE(uNAL_org);
        if(nalType != NAL_IDR_SLICE && nalType != NAL_SLICE)
        {
            pHeader->marker = 0;
        }
        
        memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,pheaderBuffer,nHeadSize);
        pParam->nWritePos += nHeadSize;
        if (pParam->nStartIdx == pParam->nStopIdx)
        {
         
            memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos,nTotalSize);
            pParam->nWritePos += nTotalSize;
        }
        else
        {
            for (nBuf = pParam->nStartIdx;nBuf <= pParam->nStopIdx;nBuf++)
            {
                if (nBuf == pParam->nStartIdx)
                {
                    memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[nBuf] + pParam->nStartPos,pParam->nInDataSize[nBuf] - pParam->nStartPos);
                    pParam->nWritePos += pParam->nInDataSize[nBuf] - pParam->nStartPos;
                }
                else if (nBuf == pParam->nStopIdx)
                {
                    memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[nBuf],pParam->nStopPos);
                    pParam->nWritePos += pParam->nStopPos;
                }
                else
                {
                    memcpy((uint8_t *)pParam->pOutData + pParam->nWritePos,(uint8_t *)pParam->pInData[nBuf],pParam->nInDataSize[nBuf]);
                    pParam->nWritePos += pParam->nInDataSize[nBuf];
                }

            }
        }
        
        return 0;//nTotalSize;
        
    }
    //开始分片
    nPos = 0;
    bMark = 0;
    nSendSize = 0;

    
    // pFU = (FU_INDICATOR *)&FU;
    // pFUHeader = (FU_HEADER *)&FUheader;




    uNAL_org = *((uint8_t *)pParam->pInData[pParam->nStartIdx] + pParam->nStartPos);


    NRI = NALU_GET_NRI(uNAL_org);
    nalType = NALU_GET_TYPE(uNAL_org);
    forbidden_bit = NALU_GET_F(uNAL_org);


    FU_RESET(FU);
    FU_SET_F(FU,forbidden_bit);
    FU_SET_NRI(FU,NRI);
    FU_SET_TYPE(FU,FU_TYPE_H264);

    //start
    FUHEADER_RESET(FUheader0);
    FUHEADER_SET_TYPE(FUheader0,nalType);
    FUHEADER_SET_R(FUheader0,0);
    FUHEADER_SET_E(FUheader0,0);
    FUHEADER_SET_S(FUheader0,1);

    //mid
    FUHEADER_RESET(FUheader1);
    FUHEADER_SET_TYPE(FUheader1,nalType);
    FUHEADER_SET_R(FUheader1,0);
    FUHEADER_SET_E(FUheader1,0);
    FUHEADER_SET_S(FUheader1,0);

    //end
    FUHEADER_RESET(FUheader2);
    FUHEADER_SET_TYPE(FUheader2,nalType);
    FUHEADER_SET_R(FUheader2,0);
    FUHEADER_SET_E(FUheader2,1);
    FUHEADER_SET_S(FUheader2,0);
   
        int nPackNum,nLeftSize = 0,nPktIdx = 0,nwPos;
        int nCurrIdx,nCurrPos,nPackDataSize;
        nPackDataSize = nPackSize - 2;
        nPackNum = (nTotalSize - 1 + (nPackDataSize) - 1) / (nPackDataSize);
        nLeftSize = nPackSize;
        nCurrIdx = pParam->nStartIdx;
        nCurrPos = pParam->nStartPos + 1; // 偏移一字节
        for (nPktIdx = 0;nPktIdx < nPackNum;nPktIdx++)
        {
            bMark = 0;
            pPackBuf = (uint8_t *)(((uint8_t *)(pParam->pOutData)) + pParam->nWritePos + nHeadSize);
             nPos = 0;
             nwPos = 0;
            if (nPktIdx == 0)
            {
                pPackBuf[0] = FU;
                pPackBuf[1] = FUheader0;
                nSendSize = nPackDataSize;
                nPos ++;
            }
            else if (nPktIdx == nPackNum - 1)
            {
                bMark = 1;
                pPackBuf[0] = FU;
                pPackBuf[1] = FUheader2;
                nSendSize = (nTotalSize - 1) - (nPackDataSize) * nPktIdx;
            }
            else
            {
                pPackBuf[0] = FU;
                pPackBuf[1] = FUheader1;
                nSendSize = nPackDataSize;
            }
            nwPos += 2;
            nLeftSize = nSendSize;
            //nLeftSize -= 2;
            pHeader->marker = bMark;
            pTCPPackSize[0] = htons(nSendSize + 2 + nHeadSize - 4);
            pParam->wSeq++;
           // printf("wseq = %u\n",pParam->wSeq);
            pHeader->sequencenumber = htons(pParam->wSeq);
            // write header
            memcpy((uint8_t *)pParam->pOutData+pParam->nWritePos,pheaderBuffer,nHeadSize);
            pParam->nWritePos += nHeadSize;
            // write data
           
           
            for (;nCurrIdx <= pParam->nStopIdx;)
            {
                
                len = pParam->nInDataSize[nCurrIdx] - nCurrPos;
                data = (uint8_t *)pParam->pInData[nCurrIdx] + nCurrPos;
                if (pParam->nStopIdx == nCurrIdx)
                {
                    len = pParam->nStopPos - nCurrPos;
                }
             
                    if (len <= nLeftSize)
                    {// 最后一包
                        memcpy(pPackBuf + nwPos,data,len);
                        nwPos += len;
                        nLeftSize -= len;
                        nCurrPos += len;

                        nCurrIdx++;
                        nCurrPos = 0;

                    }
                    else
                    {
                        memcpy(pPackBuf + nwPos,data,nLeftSize);
                        nwPos += nLeftSize;
                        nCurrPos += nLeftSize;
                        nLeftSize -= nLeftSize;
                        
                        
                        break;

                    }
                   
            }
            pParam->nWritePos += nSendSize + 2;
            
        }
        
  
    


    

    return 0;//pParam->nWritePos;



}
int Ants_RTSPServer_H264toRTP_InputData(ANTS_RTSP_H264_RTP_PARAM_T *pParam)
{
     char *pNalStart,*pNalEnd,*pCurr,*data;
    int nBuf,len;
    int NalLen;
    uint32_t uStartCode = 0;
    int nOutPos = 0,nPackSize;
    int nRet = 0;
    uint32_t msw = 0,lsw = 0;
    AntsFrameHeader tFrameHeader;
    int nType = 0;// 0- 264,1-265
    if (pParam == NULL)
    {
        return -1;
    }
    if (pParam->pOutData == NULL || pParam->nOutSize == 0)
    {
        return -1;
    }
    nType = pParam->dwCodecType;
    if (nType < 0 || nType > 4)
    {
        return -1;
    }
    Ants_rtsp_GetRTP2NTPTime(pParam->AbstimestampSec,pParam->AbstimestampUSec,&pParam->ntpTimeStamp0,&pParam->ntpTimeStamp1);
    if(pParam->bIFrame)
    {
        pParam->cIFrameStart = 1;
        pParam->discontinuity = 1;
    }
    else
    {
        pParam->cIFrameStart = 0;
        pParam->discontinuity = 0;
    }
    if (pParam->nMTUSize < 500)
    {
        pParam->nMTUSize = 1500;
    }
    pParam->nStartIdx = -1;
    pParam->nStartPos = 0;
    pParam->nWritePos = 0;
    pParam->nWritePos += sizeof(AntsFrameHeader);
    memset(&tFrameHeader,0,sizeof(tFrameHeader));
    nPackSize = pParam->nMTUSize - 60 - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionOnvifHeader);
    if(nType == 0 || nType == 1)
    {

   
        for (nBuf = 0; nBuf < pParam->nInCount;nBuf++)
        {
            pNalStart = NULL;
            pNalEnd = NULL;
            data = (char *)pParam->pInData[nBuf];
            pCurr = data;
            len = pParam->nInDataSize[nBuf];
           // printf("[%d]size = %d\n",nBuf,len);
            while(1)
            {
                if (pCurr - (char *)data >= len)
                {
                    if (nBuf == pParam->nInCount -1)
                    {
                        if (pParam->nStartIdx != -1)
                        {
                            pParam->nStopIdx = nBuf;
                            pParam->nStopPos = pCurr - data;
                            if (nType == 1)
                            {
                                nRet = H265RTP_SplitPacket(pParam);
                            }
                            else if(nType == 0)
                            {
                                nRet = H264RTP_SplitPacket(pParam);
                            }
                            
                            
                            //printf("[%d,%d]->[%d,%d] %02x\n",pParam->nStartIdx,pParam->nStartPos,pParam->nStopIdx,pParam->nStopPos,pParam->pInData[pParam->nStartIdx][pParam->nStartPos]);
                            pParam->nStartIdx = -1;
                            pParam->nStartPos = 0;
                            pParam->nStopIdx = -1;
                            pParam->nStopPos = 0;
                            if (nRet)
                            {
                                break;
                            }
                        }
                    }
                   
                    break;
                }
                uStartCode >>= 8;
                uStartCode |= (*pCurr) << 24;
                if (uStartCode == 0x01000000)
                {
                    if (pParam->nStartIdx == -1)
                    {
                        pParam->nStartIdx = nBuf;
                        pParam->nStartPos = pCurr - data + 1;
                        if (pParam->nStartPos == pParam->nInDataSize[nBuf])
                        {
                            pParam->nStartIdx++;
                            pParam->nStartPos = 0;
                        }
                    }
                    else
                    {
                        pParam->nStopIdx = nBuf;
                        pParam->nStopPos = pCurr - data - 3;
                        if (nType == 1)
                        {
                            nRet = H265RTP_SplitPacket(pParam);
                        }
                        else
                        {
                            nRet = H264RTP_SplitPacket(pParam);
                        }
                       // printf("[%d,%d]->[%d,%d] %02x\n",pParam->nStartIdx,pParam->nStartPos,pParam->nStopIdx,pParam->nStopPos,pParam->pInData[pParam->nStartIdx][pParam->nStartPos]);
                         pParam->nStartIdx = nBuf;
                         pParam->nStartPos = pParam->nStopPos + 4;
                         if (pParam->nStartPos == pParam->nInDataSize[nBuf])
                         {
                             pParam->nStartIdx++;
                             pParam->nStartPos = 0;
                         }
                         pParam->nStopIdx = -1;
                         pParam->nStopPos = 0;
                         if (nRet)
                         {
                             break;
                         }

                    }
                   
                }

                pCurr++;

            }
            
        }
        tFrameHeader.uiFrameLen = pParam->nWritePos - sizeof(AntsFrameHeader);
        tFrameHeader.uiFrameType = pParam->bIFrame?AntsPktIFrames:AntsPktPFrames;
        tFrameHeader.uMedia.struVideoHeader.cCodecId = nType?AntsH265_RTP_PACK:AntsH264_RTP_PACK;
        tFrameHeader.uMedia.struVideoHeader.usWidth = pParam->nWidth;
        tFrameHeader.uMedia.struVideoHeader.usHeight = pParam->nHeight;
        pParam->nOutSize = pParam->nWritePos;
    }
    else 
    {
        memcpy(pParam->pOutData + sizeof(tFrameHeader),pParam->pInData[0],pParam->nInDataSize[0]);
        tFrameHeader.uiFrameLen = pParam->nInDataSize[0];
        if (nType == 4)
        {
            tFrameHeader.uiFrameType = AntsPktIFrames;
            tFrameHeader.uMedia.struVideoHeader.cCodecId = AntsMJPEG_hisi;
        }
        else
        {
             tFrameHeader.uiFrameType = AntsPktAudioFrames;
             tFrameHeader.uMedia.struAudioHeader.cCodecId = nType == 2?AntsG711U:AntsG711A;
        }
        
        
         pParam->nOutSize = sizeof(tFrameHeader) + pParam->nInDataSize[0];
    }
    tFrameHeader.uiStartId = ANTS_FRAME_STARTCODE;
    tFrameHeader.uiFrameNo = pParam->dwFrameNo++;
    tFrameHeader.uiFrameTime = pParam->AbstimestampSec;
    tFrameHeader.uiFrameTickCount = pParam->AbstimestampUSec;
    tFrameHeader.uiTimeStamp = pParam->Reltimestamp;
    
    memcpy(pParam->pOutData,&tFrameHeader,sizeof(tFrameHeader));
   // printf("====================== ok size = %d\n",pParam->nWritePos);
   
    return 0;
}
int Ants_RTSPServer_RTPtoH264_InputData(ANTS_RTSP_RTP_H264_PARAM_T *pParam)
{
    int bHasAntsHeader = 0;
    AntsFrameHeader tFrameHeader;
    int nReadPos = 0,nWritePos = 0,nPktLen;
    unsigned char *pPack,*pPktData;
    unsigned char NRI,nalType,forbidden_bit;
    unsigned char fuType,type;
    uint8_t uNAL_org;
    int bMark,bExtHeader,cc,nExtLen,nRtpLen;
    int nPktDataSize,nWriteDataLen = 0;
    int s,e,r;
    if (pParam == NULL)
    {
        return -1;
    }
    if (pParam->pInData == NULL || pParam->nInDataSize == 0 ||
        pParam->pOutData == NULL || pParam->nOutSize < pParam->nInDataSize)
    {
        return -1;
    }
    if (pParam->pInData[0] != '$')
    {
        memcpy(&tFrameHeader,pParam->pInData,sizeof(AntsFrameHeader));
        if (tFrameHeader.uiStartId == ANTS_FRAME_STARTCODE)
        {
            bHasAntsHeader = 1;
            nWritePos += sizeof(AntsFrameHeader);
            nReadPos += sizeof(AntsFrameHeader);
        }
    }
    if (tFrameHeader.uiFrameType == AntsPktAudioFrames)
    {
        memcpy(pParam->pOutData,pParam->pInData,pParam->nInDataSize);
        pParam->nOutSize = pParam->nInDataSize;
        return 0;
    }
    else if (tFrameHeader.uMedia.struVideoHeader.cCodecId != AntsH264_RTP_PACK && tFrameHeader.uMedia.struVideoHeader.cCodecId != AntsH265_RTP_PACK)
    {
        return -1;
    }
    if (pParam->pInData[nReadPos] != '$')
    {
        return -1;
    }
    pPack = pParam->pInData + nReadPos;
    nWriteDataLen = 0;
    while(1)
    {
        if (nReadPos >= pParam->nInDataSize)
        {
            break;
        }
        if (pPack[0] != '$')
        {
            break;
        }
        nPktLen = pPack[2];
        nPktLen <<= 8;
        nPktLen += pPack[3];
        if (nReadPos + nPktLen + 4> pParam->nInDataSize)
        {
            break;
        }
        // do 
        bMark = (pPack[4 + 1] >> 7) & 1;
        type = pPack[4 + 1] & 0x7F;
        if (type == PAYLAODTYPE_H265)
        {
            return Ants_RTSPServer_RTPtoH265_InputData(pParam);
        }
        cc = (pPack[4 + 0] & 0x0F);
        bExtHeader = (pPack[4 + 0] >> 4) & 1;
        nExtLen = 0;
        if (bExtHeader)
        {
            nExtLen = (pPack[4 + sizeof(struct RTPHeader) + 2] << 8) | (pPack[4 + sizeof(struct RTPHeader) + 3]) * 4 + 4;
        }

        nRtpLen =sizeof(struct RTPHeader) + cc * 4 + nExtLen;

        pPktData = pPack + nRtpLen + 4;
        nPktDataSize = nPktLen - nRtpLen;
        fuType = FU_GET_TYPE(pPktData[0]);
        forbidden_bit = FU_GET_F(pPktData[0]);
        NRI = FU_GET_NRI(pPktData[0]);
        if (fuType == FU_TYPE_H264)
        { // 分片的
            s = FUHEADER_GET_S(pPktData[1]);
            e = FUHEADER_GET_E(pPktData[1]);
            r = FUHEADER_GET_R(pPktData[1]);
            nalType = FUHEADER_GET_TYPE(pPktData[1]);
            if(s)
            {

           
                NALU_RESET(uNAL_org);
                NALU_SET_NRI(uNAL_org,NRI);
                NALU_SET_F(uNAL_org,forbidden_bit);
                NALU_SET_TYPE(uNAL_org,nalType);
                pParam->pOutData[nWritePos] = 0x00;
                pParam->pOutData[nWritePos + 1] = 0x00;
                pParam->pOutData[nWritePos + 2] = 0x00;
                pParam->pOutData[nWritePos + 3] = 0x01;
                pParam->pOutData[nWritePos + 4] = uNAL_org;
                nWritePos += 5;
                nWriteDataLen += 5;
             }
            
            memcpy(pParam->pOutData + nWritePos,pPktData + 2,nPktDataSize - 2);
            nWritePos += nPktDataSize - 2;

            nWriteDataLen += nPktDataSize - 2;
        }
        else
        {
            // 不分片
            pParam->pOutData[nWritePos] = 0x00;
            pParam->pOutData[nWritePos + 1] = 0x00;
            pParam->pOutData[nWritePos + 2] = 0x00;
            pParam->pOutData[nWritePos + 3] = 0x01;
            nWritePos += 4;
            memcpy(pParam->pOutData + nWritePos,pPktData,nPktDataSize);
            nWritePos += nPktDataSize;
            nWriteDataLen += nPktDataSize + 4;
        }


        pPack += 4 + nPktLen;
        nReadPos += 4 + nPktLen;
    }
    
    if (bHasAntsHeader)
    {
        tFrameHeader.uiFrameLen = nWriteDataLen;
        tFrameHeader.uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;
        memcpy(pParam->pOutData,&tFrameHeader,sizeof(tFrameHeader));
    }
    
    pParam->nOutSize = nWritePos;
    
    return 0;
}

