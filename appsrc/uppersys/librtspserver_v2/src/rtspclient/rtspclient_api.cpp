#include "rtspclient_api.h"
#include "rtspclient.h"
#include <time.h>
#ifndef RTSP_NO_CLIENT
#define ANTS_RTSP_MAX_CLIENT_NUM  512
#define ANTS_RTSPCLIENT_CHAN_MAX    512
typedef struct _tagRtspClientMgr
{
    int nHandle;
    CRtspClient *pClient;
    char *pUrl;
    char *pUserName;
    char *pPassword;
    void *pUserData;
    int mode;
    int bSpecial_Ex;
    unsigned int dwProp;
    ANTS_RTSPCLIENTCALLBACK pCallback;
    ANTS_RTSPREALDATACALLBACKFXN pCallbackEx;
    ANTS_RTSPREALDATACALLBACKFXN_V2 pCallbackV2;
    int  nMask;

    unsigned int uUseCnt;
    int bNeedClose;
    int bNeedDelete;
	int nCombChanList[ANTS_RTSPCLIENT_CHAN_MAX];
	int nCombChanCount;
	int nCombChanMaxCount;
	int nRcvBuffSize;
	

} RtspClientMgr_T;

typedef struct _tagRtspDealRecvMgr
{
    int nHandle;
    CH264RtpSession *m_pVideoSess;
    int  nMask;

    unsigned int uUseCnt;
    int bNeedClose;
    int bNeedDelete;
} RtspDealRecvMgr_T;

static RtspClientMgr_T m_rtspclients[ANTS_RTSP_MAX_CLIENT_NUM];
static int g_nMaskStatic = 0;
static int m_bInit = 0;
static CClientPacketRecvThread m_rtspclinetRecvThread;
static int m_brtspclientRecvThread_exit = 0;
static JMutex m_rtsp_api_lock;

static RtspDealRecvMgr_T m_rtspDealRecv[ANTS_RTSP_MAX_CLIENT_NUM];
static int g_nMaskRecvStatic = 0;

static void *ANTS_RTSPClientPacketRecvThread()
{
    int i;
    int bHave = 0;
    CRtspClient *pClient = NULL;
     unsigned int dwSec = 0 ,dwUSec = 0;
    while(1)
    {
        if (m_brtspclientRecvThread_exit)
        {
            break;
        }
        bHave = 0;

       
        Ants_rtsp_GetSysRunTime(&dwSec,&dwUSec);
        
        for(i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
        {
            m_rtsp_api_lock.Lock();
            if(m_rtspclients[i].bNeedDelete)
            {
                m_rtsp_api_lock.Unlock();
                continue;
            }
            m_rtspclients[i].uUseCnt++;
            m_rtsp_api_lock.Unlock();
            pClient = m_rtspclients[i].pClient;
            if(pClient != NULL)
            {
                pClient->SetSysTime(dwSec,dwUSec);
                //bHave |= m_rtspclients[i]->DataPacketRecvThread();
                //重连
                if (pClient->IsNeedReConnect())
                {
                    //
                    m_rtsp_api_lock.Lock();
                    if (m_rtspclients[i].uUseCnt > 1)
                    {
                        m_rtspclients[i].bNeedClose = 1;
                        m_rtspclients[i].uUseCnt--;
                        m_rtsp_api_lock.Unlock();
                        continue;
                    }
                    m_rtsp_api_lock.Unlock();


                    RTSP_DEBUG("[%08X][ReConnect] Closing %s\n",m_rtspclients[i].nHandle,m_rtspclients[i].pUrl?m_rtspclients[i].pUrl:"nul");
                    pClient->Close();
                    RTSP_DEBUG("[%08X][ReConnect] Reopen %s\n",m_rtspclients[i].nHandle,m_rtspclients[i].pUrl?m_rtspclients[i].pUrl:"nul");
                    //开始
                    pClient->SetHandle(m_rtspclients[i].nHandle);
                    pClient->SetCallBack(m_rtspclients[i].pCallback,m_rtspclients[i].pUserData);
                    pClient->SetCallBackEx(m_rtspclients[i].pCallbackEx,m_rtspclients[i].pUserData);
                    pClient->SetCallBackV2(m_rtspclients[i].pCallbackV2,m_rtspclients[i].pUserData);
                    pClient->SetSpecialFlag(m_rtspclients[i].bSpecial_Ex);
                    pClient->SetProp(m_rtspclients[i].dwProp);
                    pClient->SetSysTime(dwSec,dwUSec);
					if(m_rtspclients[i].nRcvBuffSize > 0 )
					{
						pClient->Control(ANTS_RTSP_CONTROL_CMD_RCVBUFFSIZE,0,&m_rtspclients[i].nRcvBuffSize,sizeof(int),NULL,0);
					}

                    if(pClient->Open(m_rtspclients[i].pUrl,m_rtspclients[i].mode,m_rtspclients[i].pUserName,m_rtspclients[i].pPassword) >= 0)
                    {
                        pClient->StartThread();
                    }
					if (m_rtspclients[i].nCombChanCount > 0)
					{
						pClient->Control(ANTS_RTSP_CONTROL_CMD_ANTSCOMB_ADDCH,0,m_rtspclients[i].nCombChanList,m_rtspclients[i].nCombChanMaxCount,NULL,0);
					}
                    m_rtsp_api_lock.Lock();
                    m_rtspclients[i].bNeedClose = 0;
                    m_rtsp_api_lock.Unlock();

                    RTSP_DEBUG("[%08X][ReConnect] OK \n",m_rtspclients[i].nHandle);

                }
            }
            m_rtsp_api_lock.Lock();
            m_rtspclients[i].uUseCnt--;
            m_rtsp_api_lock.Unlock();
        }

        // if (!bHave)
        {
            Ants_WaitTime(1,0);
            //RTPTime::Wait(RTPTime(5,0000));
        }
    }
    return NULL;
}
static void _ants_freeHandle(RtspClientMgr_T *p)
{
    if (p == NULL)
    {
        return;
    }
    if(p->pClient != NULL)
    {
        p->pClient->Close();
        delete p->pClient;
        p->pClient = NULL;

    }
    p->nMask = 0;
    p->nHandle = 0;
    Ants_strFree(p->pUrl);
    p->pUrl = NULL;
    Ants_strFree(p->pUserName);
    p->pUserName = NULL;
    Ants_strFree(p->pPassword);
    p->pPassword = NULL;
    p->pUserData = NULL;
    p->mode = 0;
    p->bSpecial_Ex = 0;
    p->pCallback = NULL;
    p->pCallbackEx = NULL;
    memset(p,0,sizeof(RtspClientMgr_T));
}

static void _ants_freeRecvHandle(RtspDealRecvMgr_T *p)
{
    if (p == NULL)
    {
        return;
    }

	if(p->m_pVideoSess != NULL)
    {
        delete p->m_pVideoSess;
        p->m_pVideoSess = NULL;

    }
	
    p->nMask = 0;
    p->nHandle = 0;
    memset(p,0,sizeof(RtspDealRecvMgr_T));
}

int Ants_RTSPClient_Init(int bUnInit)
{
    int i;
    if (!bUnInit)
    {
        if (!m_bInit)
        {
            m_rtsp_api_lock.Init();
            m_rtsp_api_lock.Lock();
            if (m_bInit)
            {
                m_rtsp_api_lock.Unlock();
                return 0;
            }

            memset(m_rtspDealRecv,0,ANTS_RTSP_MAX_CLIENT_NUM * sizeof(RtspDealRecvMgr_T));          
			
            memset(m_rtspclients,0,ANTS_RTSP_MAX_CLIENT_NUM * sizeof(RtspClientMgr_T));
            m_brtspclientRecvThread_exit = 0;
//#ifndef USE_ONE_THREAD
            m_rtspclinetRecvThread.SetCallback(ANTS_RTSPClientPacketRecvThread);
            m_rtspclinetRecvThread.Start();
//#endif
            m_bInit = 1;
            m_rtsp_api_lock.Unlock();

        }
    }
    else
    {
        if (m_bInit)
        {
            m_rtsp_api_lock.Lock();

            for(i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
            {
                _ants_freeHandle(&m_rtspclients[i]);
                _ants_freeRecvHandle(&m_rtspDealRecv[i]);
            }
            m_brtspclientRecvThread_exit = 1;
            while(m_rtspclinetRecvThread.IsRunning())
            {
                //RTPTime::Wait(RTPTime(0,100000));
                Ants_WaitTime(0,100000);
            }
            m_bInit = 0;
            m_rtsp_api_lock.Unlock();
        }

    }
    return 0;
}

int Ants_RTSPClient_Open(char *pUrl,ANTS_RTPTransProto nProto,ANTS_RTSPCLIENTCALLBACK pCallback,char *pUserName,char *pPassword,void *pUserData)
{
    CRtspClient *pclient;
    int nhandle = -1;
    int nIdx = -1;
    int i;
    //time_t test_time;
    uint32_t test_time;
    int nMask;
    char *pUrl_save = NULL,*pUserName_save = NULL,*pPassword_save = NULL;
    RTSP_DEBUG("[RTSP]Open url = %s [%s:%s] use %s  \n",pUrl == NULL?"nul":pUrl,pUserName == NULL?"nul":pUserName,pPassword == NULL?"nul":pPassword,nProto == ANTS_RTPTransProto_AUTO?"auto":(nProto == ANTS_RTPTransProto_UDP?"udp":(nProto==ANTS_RTPTransProto_TCP?"tcp":(nProto == ANTS_RTPTransProto_HTTP?"http":(nProto == ANTS_RTPTransProto_MULTICAST?"multicast":"butt")))));
    if (!m_bInit)
    {
        Ants_RTSPClient_Init(0);
    }
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {

        m_rtsp_api_lock.Unlock();
        return 0;
    }
    m_rtsp_api_lock.Unlock();

    pUrl_save = Ants_strndup(pUrl,-1);
    if (pUrl_save == NULL)
    {
        return 0;
    }
    if (pUserName != NULL && strlen(pUserName))
    {
        pUserName_save = Ants_strndup(pUserName,-1);
        if (pUserName_save == NULL)
        {
            Ants_strFree(pUrl_save);
            return 0;
        }
    }
    if (pPassword != NULL && strlen(pPassword))
    {
        pPassword_save = Ants_strndup(pPassword,-1);
        if (pPassword_save == NULL)
        {
            Ants_strFree(pUrl_save);
            Ants_strFree(pUserName_save);
            return 0;
        }
    }
    pclient = new CRtspClient;
    if (pclient == NULL)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        return 0;
    }

    pclient->SetCallBack(pCallback,pUserData);

    m_rtsp_api_lock.Lock();
    for (i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
    {
        if (m_rtspclients[i].pClient == NULL)
        {
            nIdx = i;

            break;
        }
    }
    if (nIdx == -1)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        m_rtsp_api_lock.Unlock();
        return 0;
    }
    //time(&test_time);
    Ants_rtsp_GetSysRunTime(&test_time,NULL);
    g_nMaskStatic++;
    if (g_nMaskStatic > 255)
    {
        g_nMaskStatic = 1;
    }
    nMask = (((test_time << 8) | (g_nMaskStatic)) << 8) & 0x7FFFFFFF;
    nhandle =nIdx | nMask;
    pclient->SetHandle(nhandle);



    if(pclient->Open(pUrl,(int)nProto,pUserName,pPassword) < 0)
    {
        m_rtspclients[nIdx].pClient = NULL;
        m_rtspclients[nIdx].nMask = 0;
        _ants_freeHandle(&m_rtspclients[nIdx]);
        m_rtsp_api_lock.Unlock();
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        return 0;
    }
    m_rtspclients[nIdx].pClient = pclient;
    m_rtspclients[nIdx].nMask = nMask;
    m_rtspclients[nIdx].nHandle = nhandle;
    m_rtspclients[nIdx].pUrl = pUrl_save;
    m_rtspclients[nIdx].pUserName = pUserName_save;
    m_rtspclients[nIdx].pPassword = pPassword_save;
    m_rtspclients[nIdx].pUserData = pUserData;
    m_rtspclients[nIdx].mode = nProto;
    m_rtspclients[nIdx].bSpecial_Ex = 0;
    m_rtspclients[nIdx].pCallback = pCallback;
    m_rtspclients[nIdx].pCallbackEx = NULL;
	memset(m_rtspclients[nIdx].nCombChanList,0x80,sizeof(m_rtspclients[nIdx].nCombChanList));
	m_rtspclients[nIdx].nCombChanCount = 0;
	m_rtspclients[nIdx].nCombChanMaxCount = 0;

    pclient->StartThread();
    m_rtsp_api_lock.Unlock();
    return nhandle;
}

int Ants_RTSPClient_Open_Ex(char *pUrl,ANTS_RTPTransProto nProto,ANTS_RTSPREALDATACALLBACKFXN pCallback,char *pUserName,char *pPassword,void *pUserData)
{
    CRtspClient *pclient;
    int nhandle = -1;
    int nIdx = -1;
    int i;
    //time_t test_time;
    uint32_t test_time;
    int nMask;
    char *pUrl_save = NULL,*pUserName_save = NULL,*pPassword_save = NULL;
    RTSP_DEBUG("[RTSP]OpenEx url = %s [%s:%s] use %s  \n",pUrl == NULL?"nul":pUrl,pUserName == NULL?"nul":pUserName,pPassword == NULL?"nul":pPassword,nProto == ANTS_RTPTransProto_AUTO?"auto":(nProto == ANTS_RTPTransProto_UDP?"udp":(nProto==ANTS_RTPTransProto_TCP?"tcp":(nProto == ANTS_RTPTransProto_HTTP?"http":"butt"))));
    if (!m_bInit)
    {
        Ants_RTSPClient_Init(0);
    }
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {

        m_rtsp_api_lock.Unlock();
        return 0;
    }
    m_rtsp_api_lock.Unlock();

    pUrl_save = Ants_strndup(pUrl,-1);
    if (pUrl_save == NULL)
    {
        return 0;
    }
    if (pUserName != NULL && strlen(pUserName))
    {
        pUserName_save = Ants_strndup(pUserName,-1);
        if (pUserName_save == NULL)
        {
            Ants_strFree(pUrl_save);
            return 0;
        }
    }
    if (pPassword != NULL && strlen(pPassword))
    {
        pPassword_save = Ants_strndup(pPassword,-1);
        if (pPassword_save == NULL)
        {
            Ants_strFree(pUrl_save);
            Ants_strFree(pUserName_save);
            return 0;
        }
    }


    pclient = new CRtspClient;
    if (pclient == NULL)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);

        return 0;
    }

    pclient->SetCallBackEx(pCallback,pUserData);



    m_rtsp_api_lock.Lock();
    for (i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
    {
        if (m_rtspclients[i].pClient == NULL)
        {
            nIdx = i;

            break;
        }
    }
    if (nIdx == -1)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        m_rtsp_api_lock.Unlock();
        return 0;
    }
    //time(&test_time);
    Ants_rtsp_GetSysRunTime(&test_time,NULL);
    g_nMaskStatic++;
    if (g_nMaskStatic > 255)
    {
        g_nMaskStatic = 1;
    }
    nMask = (((test_time << 8) | (g_nMaskStatic)) << 8) & 0x7FFFFFFF;
    nhandle =nIdx | nMask;
    pclient->SetHandle(nhandle);


    if(pclient->Open(pUrl,(int)nProto,pUserName,pPassword) < 0)
    {
        m_rtspclients[nIdx].pClient = NULL;
        m_rtspclients[nIdx].nMask = 0;
        _ants_freeHandle(&m_rtspclients[nIdx]);
        m_rtsp_api_lock.Unlock();
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        return 0;
    }
    m_rtspclients[nIdx].pClient = pclient;
    m_rtspclients[nIdx].nMask = nMask;
    m_rtspclients[nIdx].nHandle = nhandle;
    m_rtspclients[nIdx].pUrl = pUrl_save;
    m_rtspclients[nIdx].pUserName = pUserName_save;
    m_rtspclients[nIdx].pPassword = pPassword_save;
    m_rtspclients[nIdx].pUserData = pUserData;
    m_rtspclients[nIdx].mode = nProto;
    m_rtspclients[nIdx].bSpecial_Ex = 0;
    m_rtspclients[nIdx].dwProp = 0;
    m_rtspclients[nIdx].pCallbackV2 = NULL;
    m_rtspclients[nIdx].pCallback = NULL;
    m_rtspclients[nIdx].pCallbackEx = pCallback;
	memset(m_rtspclients[nIdx].nCombChanList,0x80,sizeof(m_rtspclients[nIdx].nCombChanList));
	m_rtspclients[nIdx].nCombChanCount = 0;
	m_rtspclients[nIdx].nCombChanMaxCount = 0;

    pclient->StartThread();
    m_rtsp_api_lock.Unlock();
    return nhandle;
}
int Ants_RTSPClient_Open_Ex2(char *pUrl,ANTS_RTPTransProto nProto,ANTS_RTSPREALDATACALLBACKFXN pCallback,char *pUserName,char *pPassword,void *pUserData)
{
    CRtspClient *pclient;
    int nhandle = -1;
    int nIdx = -1;
    int i;
    //time_t test_time;
    uint32_t test_time;
    int nMask;
    char *pUrl_save = NULL,*pUserName_save = NULL,*pPassword_save = NULL;
    RTSP_DEBUG("[RTSP]OpenEx2 url = %s [%s:%s] use %s  \n",pUrl == NULL?"nul":pUrl,pUserName == NULL?"nul":pUserName,pPassword == NULL?"nul":pPassword,nProto == ANTS_RTPTransProto_AUTO?"auto":(nProto == ANTS_RTPTransProto_UDP?"udp":(nProto==ANTS_RTPTransProto_TCP?"tcp":(nProto == ANTS_RTPTransProto_HTTP?"http":"butt"))));
    if (!m_bInit)
    {
        Ants_RTSPClient_Init(0);
    }
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {

        m_rtsp_api_lock.Unlock();
        return 0;
    }
    m_rtsp_api_lock.Unlock();

    pUrl_save = Ants_strndup(pUrl,-1);
    if (pUrl_save == NULL)
    {
        return 0;
    }
    if (pUserName != NULL && strlen(pUserName))
    {
        pUserName_save = Ants_strndup(pUserName,-1);
        if (pUserName_save == NULL)
        {
            Ants_strFree(pUrl_save);
            return 0;
        }
    }
    if (pPassword != NULL && strlen(pPassword))
    {
        pPassword_save = Ants_strndup(pPassword,-1);
        if (pPassword_save == NULL)
        {
            Ants_strFree(pUrl_save);
            Ants_strFree(pUserName_save);
            return 0;
        }
    }
    pclient = new CRtspClient;
    if (pclient == NULL)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        return 0;
    }

    pclient->SetCallBackEx(pCallback,pUserData);
    pclient->SetSpecialFlag(1);
    pclient->SetProp(2);


    m_rtsp_api_lock.Lock();
    for (i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
    {
        if (m_rtspclients[i].pClient == NULL)
        {
            nIdx = i;

            break;
        }
    }
    if (nIdx == -1)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        m_rtsp_api_lock.Unlock();
        return 0;
    }
    //time(&test_time);
    Ants_rtsp_GetSysRunTime(&test_time,NULL);
    g_nMaskStatic++;
    if (g_nMaskStatic > 255)
    {
        g_nMaskStatic = 1;
    }
    nMask = (((test_time << 8) | (g_nMaskStatic)) << 8) & 0x7FFFFFFF;
    nhandle =nIdx | nMask;
    pclient->SetHandle(nhandle);


    if(pclient->Open(pUrl,(int)nProto,pUserName,pPassword) < 0)
    {
        m_rtspclients[nIdx].pClient = NULL;
        m_rtspclients[nIdx].nMask = 0;
        _ants_freeHandle(&m_rtspclients[nIdx]);
        m_rtsp_api_lock.Unlock();
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        return 0;
    }
    m_rtspclients[nIdx].pClient = pclient;
    m_rtspclients[nIdx].nMask = nMask;
    m_rtspclients[nIdx].nHandle = nhandle;
    m_rtspclients[nIdx].pUrl = pUrl_save;
    m_rtspclients[nIdx].pUserName = pUserName_save;
    m_rtspclients[nIdx].pPassword = pPassword_save;
    m_rtspclients[nIdx].pUserData = pUserData;
    m_rtspclients[nIdx].mode = nProto;
    m_rtspclients[nIdx].bSpecial_Ex = 1;
    m_rtspclients[nIdx].pCallback = NULL;
    m_rtspclients[nIdx].dwProp = 2;
    m_rtspclients[nIdx].pCallbackV2 = NULL;
    m_rtspclients[nIdx].pCallbackEx = pCallback;
	memset(m_rtspclients[nIdx].nCombChanList,0x80,sizeof(m_rtspclients[nIdx].nCombChanList));
	m_rtspclients[nIdx].nCombChanCount = 0;
	m_rtspclients[nIdx].nCombChanMaxCount = 0;

    pclient->StartThread();
    m_rtsp_api_lock.Unlock();
    return nhandle;
}

int Ants_RTSPClient_Open_V2(char *pUrl,ANTS_RTPTransProto nProto,unsigned long dwProp,ANTS_RTSPREALDATACALLBACKFXN_V2 pCallback,char *pUserName,char *pPassword,void *pUserData)
{
    CRtspClient *pclient;
    int nhandle = -1;
    int nIdx = -1;
    int i;
    //time_t test_time;
    uint32_t test_time;
    int nMask;
    char *pUrl_save = NULL,*pUserName_save = NULL,*pPassword_save = NULL;
    RTSP_DEBUG("[RTSP]OpenV2 Prop = %x url = %s [%s:%s] use %s  \n",dwProp,pUrl == NULL?"nul":pUrl,pUserName == NULL?"nul":pUserName,pPassword == NULL?"nul":pPassword,nProto == ANTS_RTPTransProto_AUTO?"auto":(nProto == ANTS_RTPTransProto_UDP?"udp":(nProto==ANTS_RTPTransProto_TCP?"tcp":(nProto == ANTS_RTPTransProto_HTTP?"http":"butt"))));
    if (!m_bInit)
    {
        Ants_RTSPClient_Init(0);
    }
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {

        m_rtsp_api_lock.Unlock();
        return 0;
    }
    m_rtsp_api_lock.Unlock();

    pUrl_save = Ants_strndup(pUrl,-1);
    if (pUrl_save == NULL)
    {
        return 0;
    }
    if (pUserName != NULL && strlen(pUserName))
    {
        pUserName_save = Ants_strndup(pUserName,-1);
        if (pUserName_save == NULL)
        {
            Ants_strFree(pUrl_save);
            return 0;
        }
    }
    if (pPassword != NULL && strlen(pPassword))
    {
        pPassword_save = Ants_strndup(pPassword,-1);
        if (pPassword_save == NULL)
        {
            Ants_strFree(pUrl_save);
            Ants_strFree(pUserName_save);
            return 0;
        }
    }
    pclient = new CRtspClient;
    if (pclient == NULL)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        return 0;
    }


    pclient->SetCallBackV2(pCallback,pUserData);


    pclient->SetProp(dwProp);



    m_rtsp_api_lock.Lock();
    for (i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
    {
        if (m_rtspclients[i].pClient == NULL)
        {
            nIdx = i;

            break;
        }
    }
    if (nIdx == -1)
    {
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        m_rtsp_api_lock.Unlock();
        return 0;
    }
    //time(&test_time);
    Ants_rtsp_GetSysRunTime(&test_time,NULL);
    g_nMaskStatic++;
    if (g_nMaskStatic > 255)
    {
        g_nMaskStatic = 1;
    }
    nMask = (((test_time << 8) | (g_nMaskStatic)) << 8) & 0x7FFFFFFF;
    nhandle =nIdx | nMask;
    pclient->SetHandle(nhandle);

	RTSP_DEBUG("jrtp [%08d:%s]  pclient->Open begin \r\n", __LINE__, __FUNCTION__) ;

    if(pclient->Open(pUrl,(int)nProto,pUserName,pPassword) < 0)
    {
        m_rtspclients[nIdx].pClient = NULL;
        m_rtspclients[nIdx].nMask = 0;
        _ants_freeHandle(&m_rtspclients[nIdx]);
        m_rtsp_api_lock.Unlock();
        Ants_strFree(pUrl_save);
        Ants_strFree(pUserName_save);
        Ants_strFree(pPassword_save);
        delete pclient;
        return 0;
    }

	RTSP_DEBUG("jrtp [%08d:%s]  pclient->Open end \r\n", __LINE__, __FUNCTION__) ;

    m_rtspclients[nIdx].pClient = pclient;
    m_rtspclients[nIdx].nMask = nMask;
    m_rtspclients[nIdx].nHandle = nhandle;
    m_rtspclients[nIdx].pUrl = pUrl_save;
    m_rtspclients[nIdx].pUserName = pUserName_save;
    m_rtspclients[nIdx].pPassword = pPassword_save;
    m_rtspclients[nIdx].pUserData = pUserData;
    m_rtspclients[nIdx].mode = nProto;
    m_rtspclients[nIdx].dwProp = dwProp;
    m_rtspclients[nIdx].pCallback = NULL;
    m_rtspclients[nIdx].pCallbackEx = NULL;
    m_rtspclients[nIdx].pCallbackV2 = pCallback;
	memset(m_rtspclients[nIdx].nCombChanList,0x80,sizeof(m_rtspclients[nIdx].nCombChanList));
	m_rtspclients[nIdx].nCombChanCount = 0;
	m_rtspclients[nIdx].nCombChanMaxCount = 0;

    pclient->StartThread();
    m_rtsp_api_lock.Unlock();

	RTSP_DEBUG("jrtp [%08d:%s]  handle[%d] end \r\n", __LINE__, __FUNCTION__, nhandle) ;

    return nhandle;
}

int Ants_RTSPClient_GetTstConfig(int hRtspHandle,int *pVideoType,int *pAudioType,char *pConfig,int nConfigBufsize,int *pWidth,int *pHeight,int *pFrameFrate)
{
    int nhandle = -1;
    int nMask;
    const char *pCfg;


    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("not initted\n");
        return -1;
    }

    if (m_rtspclients[nhandle].pClient == NULL)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("handle ,no exist\n");
        return -1;
    }
    if (nMask != m_rtspclients[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("invalid handle\n");
        return -1;
    }
    pCfg = m_rtspclients[nhandle].pClient->GetTstConfig(pVideoType,pAudioType,pWidth,pHeight,pFrameFrate);
    if (pCfg)
    {
        int len;
        len = strlen(pCfg);
        if (len >= nConfigBufsize)
        {
            RTSP_DEBUG("[RTSP.%d]len >= nConfigBufsize [%d >= %d] \n",__LINE__,len, nConfigBufsize);
            pCfg = NULL;
        }
        else if(pConfig != NULL)
        {
            strcpy(pConfig,pCfg);
        }
    }
    else
    {
        RTSP_DEBUG("[RTSP.%d]GetTstConfig return NULL\n",__LINE__);
    }

    m_rtsp_api_lock.Unlock();
    if (pCfg == NULL)
    {

        return -1;
    }
    return 0;
}

int Ants_RTSPClient_Control(int hRtspHandle,int nCmd,unsigned long dwApplyID,void *pInBuffer,int nInSize,void *pOutBuffer,int nOutSize)
{
    int nhandle = -1;
    int nMask;
    int nRet = -1;
	RtspClientMgr_T *pClientMgr = NULL;

	//RTSP_DEBUG("jrtp [%08d:%s] handle[%x]  nCmd[%d]  begin\r\n", __LINE__, __FUNCTION__, hRtspHandle, nCmd) ;

    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("not initted\n");
		RTSP_ERROR("jrtp [%08d:%s] not initted nCmd[%d]\r\n", __LINE__, __FUNCTION__, nCmd) ;
        return -1;
    }

    if (m_rtspclients[nhandle].pClient == NULL)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("handle ,no exist\n");
		RTSP_ERROR("jrtp [%08d:%s] handle ,no exist nCmd[%d]\r\n", __LINE__, __FUNCTION__, nCmd) ;
        return -1;
    }
    if (nMask != m_rtspclients[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("invalid handle\n");
		RTSP_ERROR("jrtp [%08d:%s] invalid handle[%x] mask[%d] ori mask[%d] nCmd[%d]\r\n", __LINE__, __FUNCTION__, hRtspHandle, nMask, m_rtspclients[nhandle].nMask, nCmd) ;
        return -1;
    }
#if 1  
    pClientMgr = &m_rtspclients[nhandle];
	switch(nCmd)
	{
	case ANTS_RTSP_CONTROL_CMD_RCVBUFFSIZE:
		{
			int nBufSize = 0;
			if (pInBuffer != NULL && nInSize >= sizeof(int))
			{
				nBufSize = *(int *)pInBuffer;
			}
			if (nBufSize < 65536)
			{
				nBufSize = 65536;
			}
			if (nBufSize > 10 * 1024 * 1024)
			{
				nBufSize = 10 * 1024 * 1024;
			}
			if (pClientMgr->nRcvBuffSize != nBufSize)
			{
				pClientMgr->nRcvBuffSize = nBufSize;
			}


			break;
		}
	case ANTS_RTSP_CONTROL_CMD_ANTSCOMB_ADDCH:
		{
			int nReq;
			int n,nNewIdx = -1,bExist = 0,bChg = 0,nUseCnt = 0;
			int *pInChan=(int *)pInBuffer;
			for (nReq = 0; nReq < nInSize && pInChan != NULL;nReq++)
			{
				if (pInChan[nReq] != 0x80808080)
				{
					nNewIdx = -1;
					bExist = 0;
					nUseCnt = 0;
					for (n = 0; n < ANTS_RTSPCLIENT_CHAN_MAX; n++)
					{
						if (nUseCnt >= pClientMgr->nCombChanCount && nNewIdx != -1)
						{
							break;
						}
						if (pClientMgr->nCombChanList[n] != 0x80808080 )
						{
							nUseCnt++;
							if (pClientMgr->nCombChanList[n] == pInChan[nReq])
							{
								// 存在
								bExist = 1;
								if (nNewIdx != -1)
								{
									break;
								}
								nNewIdx = -1;
							}
						}
						else if(nNewIdx == -1)
						{
							nNewIdx = n;
							if (bExist)
							{
								break;
							}
						}
					}
					if ((!bExist) && nNewIdx != -1)
					{
						pClientMgr->nCombChanList[nNewIdx] = pInChan[nReq];
						pClientMgr->nCombChanCount++;
						if(pClientMgr->nCombChanMaxCount <= nNewIdx)
						{
							pClientMgr->nCombChanMaxCount = nNewIdx + 1;
						}
						bChg = 1;
					}
				}

			}

			break;
		}
	case ANTS_RTSP_CONTROL_CMD_ANTSCOMB_DECCH:
		{
			int nReq;
			int n,bChg = 0,nUseCnt = 0,nLastN = -1;
			int *pInChan=(int *)pInBuffer;
			for (nReq = 0; nReq < nInSize && pInChan != NULL;nReq++)
			{
				if (pInChan[nReq] != 0x80808080)
				{
					nUseCnt = 0;
					for (n = 0; n < ANTS_RTSPCLIENT_CHAN_MAX; n++)
					{
						if (nUseCnt >= pClientMgr->nCombChanCount)
						{
							break;
						}
						if (pClientMgr->nCombChanList[n] != 0x80808080 )
						{
							nUseCnt++;
							if (pClientMgr->nCombChanList[n] == pInChan[nReq])
							{
								// 存在
								pClientMgr->nCombChanList[n] = 0x80808080;
								pClientMgr->nCombChanCount--;
								if(pClientMgr->nCombChanMaxCount == n + 1)
								{
									pClientMgr->nCombChanMaxCount = nLastN + 1;
								}
								bChg = 1;
								break;
							}
							nLastN = n;
						}

					}

				}

			}
		
			break;
		}
	default:
		break;
	}
#endif
	if (m_rtspclients[nhandle].bNeedClose ||
		m_rtspclients[nhandle].bNeedDelete)
	{
		m_rtsp_api_lock.Unlock();
		RTSP_ERROR("jrtp [%08d:%s] Need close handle[%x] mask[%d] ori mask[%d] nCmd[%d]\r\n", __LINE__, __FUNCTION__, hRtspHandle, nMask, m_rtspclients[nhandle].nMask, nCmd) ;
		return 0;
	}
	m_rtspclients[nhandle].uUseCnt++;
	m_rtsp_api_lock.Unlock();
	//RTSP_DEBUG("jrtp [%08d:%s] handle[%x] mask[%d] ori mask[%d] nCmd[%d] nRet[%d] pClient->Control begin\r\n", __LINE__, __FUNCTION__, hRtspHandle, nMask, m_rtspclients[nhandle].nMask, nCmd, nRet) ;
    nRet = m_rtspclients[nhandle].pClient->Control(nCmd,dwApplyID,pInBuffer,nInSize,pOutBuffer,nOutSize);
	//RTSP_DEBUG("jrtp [%08d:%s] handle[%x] mask[%d] ori mask[%d] nCmd[%d] nRet[%d] pClient->Control end\r\n", __LINE__, __FUNCTION__, hRtspHandle, nMask, m_rtspclients[nhandle].nMask, nCmd, nRet) ;
  
	//RTSP_DEBUG("jrtp [%08d:%s] handle[%x] mask[%d] ori mask[%d] nCmd[%d] nRet[%d] end\r\n", __LINE__, __FUNCTION__, hRtspHandle, nMask, m_rtspclients[nhandle].nMask, nCmd, nRet) ;
    m_rtsp_api_lock.Lock();
    m_rtspclients[nhandle].uUseCnt--;
    m_rtsp_api_lock.Unlock();
   
    return nRet;
}


int Ants_RTSPClient_Read(long hRtspHandle,int *nChan,int *nStream,int *nStreamType,unsigned char **lpBuffer,unsigned long *dwBufSize)
{
    int nhandle = -1;
    int nMask;
    int nRet = -1;


    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("not initted\n");
        return -1;
    }

    if (m_rtspclients[nhandle].pClient == NULL)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("handle ,no exist\n");
        return -1;
    }
    if (nMask != m_rtspclients[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("invalid handle\n");
        return -1;
    }
    if (m_rtspclients[nhandle].bNeedClose ||
        m_rtspclients[nhandle].bNeedDelete)
    {
        m_rtsp_api_lock.Unlock();
        return -1;
    }
    m_rtspclients[nhandle].uUseCnt++;
    m_rtsp_api_lock.Unlock();
    nRet = m_rtspclients[nhandle].pClient->BufferMgrRead(nChan,nStream,nStreamType,lpBuffer,dwBufSize);

    m_rtsp_api_lock.Lock();
    m_rtspclients[nhandle].uUseCnt--;
    m_rtsp_api_lock.Unlock();

    return nRet;
}
int Ants_RTSPClient_ReadRelease(long hRtspHandle)
{
    int nhandle = -1;
    int nMask;
    int nRet = -1;


    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("not initted\n");
        return -1;
    }

    if (m_rtspclients[nhandle].pClient == NULL)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("handle ,no exist\n");
        return -1;
    }
    if (nMask != m_rtspclients[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("invalid handle\n");
        return -1;
    }
    if (m_rtspclients[nhandle].bNeedClose ||
        m_rtspclients[nhandle].bNeedDelete)
    {
        m_rtsp_api_lock.Unlock();
        return -1;
    }
    m_rtspclients[nhandle].uUseCnt++;
    m_rtsp_api_lock.Unlock();
    nRet = m_rtspclients[nhandle].pClient->BufferMgrReadRelease();

    m_rtsp_api_lock.Lock();
    m_rtspclients[nhandle].uUseCnt--;
    m_rtsp_api_lock.Unlock();

    return nRet;
}
int Ants_RTSPClient_Close(int hRtspHandle)
{
    CRtspClient *pclient = NULL;
    int nhandle = -1;
    int nMask;
	RTSP_DEBUG("jrtp [%08d:%s] handle[%x]  begin\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;

    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;
    m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
		RTSP_ERROR("jrtp [%08d:%s] handle[%x]  not initted\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;
        RTSP_ERROR("not initted\n");
        return -1;
    }

    if (m_rtspclients[nhandle].pClient == NULL)
    {
        m_rtsp_api_lock.Unlock();
		RTSP_ERROR("jrtp [%08d:%s] handle[%x]  handle ,no exist\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;
        RTSP_ERROR("handle ,no exist\n");
        return 0;
    }
    pclient = m_rtspclients[nhandle].pClient;
    if (nMask != m_rtspclients[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
		RTSP_ERROR("jrtp [%08d:%s] handle[%x]  invalid handle\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;
        RTSP_ERROR("invalid handle\n");
        return -1;
    }
    m_rtspclients[nhandle].bNeedDelete = 1;
    m_rtspclients[nhandle].bNeedClose = 1;
	if (pclient != NULL)
	{
		pclient->NeedClose();
	}
    m_rtsp_api_lock.Unlock();
    while(1)
    {
        m_rtsp_api_lock.Lock();
        if (m_rtspclients[nhandle].uUseCnt <= 0)
        {
            m_rtspclients[nhandle].pClient = NULL;
            m_rtspclients[nhandle].nMask = 0;
            m_rtspclients[nhandle].bNeedDelete = 0;
            m_rtspclients[nhandle].bNeedClose = 0;
            _ants_freeHandle(&m_rtspclients[nhandle]);
            m_rtsp_api_lock.Unlock();
            break;
        }
        m_rtsp_api_lock.Unlock();
        Ants_WaitTime(0,100000);
    }




    if (pclient != NULL)
    {
		RTSP_DEBUG("jrtp [%08d:%s] handle[%x]  pclient->Close() begin\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;
        pclient->Close();
		RTSP_DEBUG("jrtp [%08d:%s] handle[%x]  pclient->Close() end\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;
        delete pclient;
    }
    
	RTSP_DEBUG("jrtp [%08d:%s] handle[%x]  end\r\n", __LINE__, __FUNCTION__, hRtspHandle) ;
    return 0;
}

void * Ants_RTSPClient_Malloc(unsigned int uSize,int nAlign)
{
	return malloc(uSize);
}
void Ants_RTSPClient_Free(void *pMem)
{
	free(pMem);
}
#if 0
int Ants_RTSPClient_OpenDealRecv(int mPayloadType, int mStreamType, int mPayloadClockRate, ANTS_RTSPCLIENT_DEALRECV_CALLBACK pCallback, void *pUserData)
{
    printf("mPayloadType = %d %d %d!!!!!!!!!!!\n", mPayloadType, mStreamType, mPayloadClockRate);
    CH264RtpSession *sess;
    int nhandle = -1;
    int nIdx = -1;
    int i;
    //time_t test_time;
    uint32_t test_time;
    int nMask;
	
	if (!m_bInit)
    {
        Ants_RTSPClient_Init(0);
    }

	m_rtsp_api_lock.Lock();

	if (!m_bInit)
    {

        m_rtsp_api_lock.Unlock();
        return 0;
    }

	m_rtsp_api_lock.Unlock();    

	sess = new CH264RtpSession;
    if (sess == NULL)
    {
        return -1;
    }

	sess->SetDefaultPayloadType(mPayloadType);
	sess->SetRTSPStreamType(mStreamType);
	sess->SetPayloadClockRate(mPayloadClockRate);
		
	m_rtsp_api_lock.Lock();

	for (i = 0; i < ANTS_RTSP_MAX_CLIENT_NUM; i++)
    {
        if (m_rtspDealRecv[i].m_pVideoSess == NULL)
        {
            nIdx = i;

            break;
        }
    }
    if (nIdx == -1)
    {
        delete sess;
        m_rtsp_api_lock.Unlock();
        return 0;
    }


    Ants_rtsp_GetSysRunTime(&test_time,NULL);
    g_nMaskRecvStatic++;
    if (g_nMaskRecvStatic > 255)
    {
        g_nMaskRecvStatic = 1;
    }
    nMask = (((test_time << 8) | (g_nMaskRecvStatic)) << 8) & 0x7FFFFFFF;
    nhandle = nIdx | nMask;

    m_rtspDealRecv[nIdx].m_pVideoSess = sess;
    m_rtspDealRecv[nIdx].nMask = nMask;
    m_rtspDealRecv[nIdx].nHandle = nhandle;

	sess->SetClientCallback((void *)nhandle, pCallback, pUserData);	

	m_rtsp_api_lock.Unlock(); 

	printf("Ants_RTSPClient_OpenDealRecv  nhandle = %d!!!!!!!!!!!\n", nhandle);	

	return nhandle;
}

int Ants_RTSPClient_DealRecv(int hRtspHandle, unsigned char *pRecvData, int nDataLen)
{
	//printf("Ants_RTSPClient_DealRecv start hRtspHandle = %d nDataLen = %d!!!!!!!!!!!\n", hRtspHandle, nDataLen);	
    CH264RtpSession *sess = NULL;
    int nhandle = -1;
    int nMask;
    int nRet = -1;
	
    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;

	//printf("Ants_RTSPClient_DealRecv1!!!!!!!!!!!\n");
	m_rtsp_api_lock.Lock();
	//printf("Ants_RTSPClient_DealRecv2!!!!!!!!!!!\n");	
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("not initted\n");
        return -1;
    }

    if (m_rtspDealRecv[nhandle].m_pVideoSess == NULL)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("handle ,no exist\n");
        return 0;
    }

	sess = m_rtspDealRecv[nhandle].m_pVideoSess;

	if (nMask != m_rtspDealRecv[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("invalid handle\n");
        return -1;
    }

    if (m_rtspDealRecv[nhandle].bNeedClose ||
        m_rtspDealRecv[nhandle].bNeedDelete)
    {
        m_rtsp_api_lock.Unlock();
        return -1;
    }
    m_rtspDealRecv[nhandle].uUseCnt++;
    m_rtsp_api_lock.Unlock();
	//printf("Ants_RTSPClient_DealRecv3!!!!!!!!!!!\n");	
	nRet = sess->DealRecvPacket(pRecvData, nDataLen);
	//printf("Ants_RTSPClient_DealRecv4!!!!!!!!!!!\n");	
	m_rtsp_api_lock.Lock();
	//printf("Ants_RTSPClient_DealRecv5!!!!!!!!!!!\n");		
    m_rtspDealRecv[nhandle].uUseCnt--;
    m_rtsp_api_lock.Unlock(); 

	//printf("Ants_RTSPClient_DealRecv end!!!!!!!!!!!\n");		

	return nRet;
}

int Ants_RTSPClient_CloseDealRecv(int hRtspHandle)
{
printf("Ants_RTSPClient_CloseDealRecv start hRtspHandle = %d!!!!!!!!!!!\n", hRtspHandle);
    CH264RtpSession *sess = NULL;
    int nhandle = -1;
    int nMask;

    nhandle = hRtspHandle & 0xFF;
    nMask = hRtspHandle & 0xFFFFFF00;

	m_rtsp_api_lock.Lock();
    if (!m_bInit)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("not initted\n");
        return -1;
    }

    if (m_rtspDealRecv[nhandle].m_pVideoSess == NULL)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("handle ,no exist\n");
        return 0;
    }

	sess = m_rtspDealRecv[nhandle].m_pVideoSess;

	if (nMask != m_rtspDealRecv[nhandle].nMask)
    {
        m_rtsp_api_lock.Unlock();
        RTSP_ERROR("invalid handle\n");
        return -1;
    }

	m_rtspDealRecv[nhandle].bNeedDelete = 1;
    m_rtspDealRecv[nhandle].bNeedClose = 1;

	m_rtsp_api_lock.Unlock();

    while(1)
    {
        m_rtsp_api_lock.Lock();
        if (m_rtspDealRecv[nhandle].uUseCnt <= 0)
        {
            m_rtspDealRecv[nhandle].m_pVideoSess = NULL;
            m_rtspDealRecv[nhandle].nMask = 0;
            m_rtspDealRecv[nhandle].bNeedDelete = 0;
            m_rtspDealRecv[nhandle].bNeedClose = 0;
            _ants_freeRecvHandle(&m_rtspDealRecv[nhandle]);
            m_rtsp_api_lock.Unlock();
            break;
        }
        m_rtsp_api_lock.Unlock();
        Ants_WaitTime(0,100000);
    }

	if (sess != NULL)
	{
        delete sess;
	}
printf("Ants_RTSPClient_CloseDealRecv end!!!!!!!!!!!\n");
    return 0;	
}
#endif

#endif
