#ifndef _RTSP_H
#define _RTSP_H
#include <jthread/jthread.h>
#include <rtpsession.h>

#include "rtspserver_v2.h"
#include "h264rtpsession.h"
#include "audiortpsession.h"
#include "apprtpsession.h"

#include "rtsp_common.h"
#include "rtsp_msg.h"
#include <openssl/ssl.h>
#include <openssl/err.h>



#ifdef WIN32
#pragma warning(disable : 4996)


#else


#endif



using namespace jthread;
using namespace jrtplib;


class RTPTCPSender;
class CRtspServer;
class CClientSocket;
class CRtpSessionMgr;

#define GETBYTE(a,n) (((a) >> ((n) << 3)) & 0xFF)
#define GETWORD(a,n) (((a)[n>>1] >> ((n&1) << 4)) & 0xFFFF)
typedef int (*ANTS_RTSPSTATUSINTERCALLBACK) (CRtspServer *pRtspServer,int hStreamHandle,int nType, void *pInParam,void *pOutParam);


#define RTSP_BUFFER_RECV_SIZE  (10 * 1024)
#define RTSP_BUFFER_SEND_SIZE  (512 * 1024)//512000

#define RTSP_APP_SUPPORT_MAX_NUM  16







typedef struct
{
    //[rtsp,urtsp]://[ip,dnsname][:port]/file?ctype=[video;audio]&stype=[unicast,multicast]&ptype=[tcp,http,udp]&ipv=[224.x.x.x]&portv=x&ipa=[224.x.x.x]&porta=x&ttl=x
    char nProtoType;//0-rtsp,1-urtsp
    char ctype;//0-auto,1-video,2-audio,3-videoback,4-audioback,5-app
    char stype;//0-auto,1-unicast,2-multicast;
    char ptype;//0-auto,1-udp,2-tcp,3-http
    short bIP;//0-dnsname,1-IP
    short bIPv6;//0-IPv4,1-IPv6
    char pAddress[ANTS_RTSP_MAX_DNSNAME_LEN];//ip or name
    char pStreamFileName[ANTS_RTSP_MAX_FILENAME_LEN];
    int bHasParam;
    int nPort;//0-[rtsp/urtsp:554]
    int destPortV;//0-auto
    int destPortA;//0-auto
    unsigned int destIPV[4];//0-auto
    unsigned int destIPA[4];//0-auto
    int nTTL;//0-255,[1-255]
    unsigned char byAppPayloadType;
    int bRecording;
    int nCustom;    
    int bAntsComb;    
    int nCh; // 0-第一通道
    int nStream;// 0-主码流，1-子码流
    int bTimeValid;// bit0 = 1 开始时间有效，bit1=1结束时间有效
    Ants_RtspDayTime tStart;
    Ants_RtspDayTime tStop;


} RTSP_URL_T;

class CRtspTmpObject
{
public:
    CRtspTmpObject(void *pt = NULL){m_pt = pt;};
    void SetPt(void *pt){m_pt = pt;};
    ~CRtspTmpObject(){if(m_pt)free(m_pt);m_pt=NULL;}
protected:
private:
    void *m_pt;
};
int Ants_Rtsp_ParseUrl(char *pUrl,RTSP_URL_T *pResult);

typedef struct _tagRTSP_APP_MGR
{
    unsigned char byAppPayloadType;
    unsigned int dwClockRate;
    char *szAppName;
    CAppRtpSession *pSess;
    int nAppRtpPort;
} RTSP_APP_MGR_T;

typedef struct _tagRTSP_SEND_MSG
{
    char *pData;
    int   len;
    int   bType;
    struct _tagRTSP_SEND_MSG *pNext;
} RTSP_SEND_MSG_T;

class CClientSocket:JThread
{
public:
    CClientSocket(CRtspServer *pServer,int clientSocket,int bIPv6 = 0, SSL *pSsl = NULL);
    int Create();
    int Destroy();    
    void SetNext(CClientSocket *Next)
    {
        m_pNext = Next;
    }
    void SetPrev(CClientSocket *Prev)
    {
        m_pPrev = Prev;
    }
    CClientSocket *GetNext()
    {
        return m_pNext;
    }
    CClientSocket *GetPrev()
    {
        return m_pPrev;
    }
    int GetSocket();
    unsigned char *GetReadBuffer()
    {
        return m_pRequestBuffer;
    }
    CMessage *GetMessage();
    int ReadAndParse();
    int Parse(char *pData,int nDataSize);
    int SetMulticastIPv4(unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio = 0);
    int GetMulticastIPv4(unsigned int *uiIPv4,int *nPort,int *nTTL,int bAudio = 0,int *bIPv6=NULL);   
    int SetSPS(char *pSPSData,int nSPSLen);
    int SetPPS(char *pPPSData,int nPPSLen);
    int SetVPS(char *pVPSData,int nVPSLen); 
    int GetProfileLevelID();    
    char * GetPPS_Base64();
    char * GetSPS_Base64();
    char * GetVPS_Base64();   
	int H265_GetProfileId();
	int H265_GetProfileSpace();
	int H265_GetTierFlag();
	int H265_GetLevelId();
	char* H265_GetInteropConstraints();
    int SetConfig(int nCmdType,void *pData,int nDataSize);    
    int SetStreamInfo(int bStreamType,int nVideoPayloadType,int nAudioPayloadType);
    int SetSupportMulticast(int bSupportMulticast)
    {
        int bLast = m_bSupportMulticast;
        m_bSupportMulticast = bSupportMulticast;
        return bLast;
    }
    int GetSupportMulticast()
    {
        return m_bSupportMulticast;
    }    
    int AddAppStream(int nAppPayloadType,unsigned int dwClockRate,char *szAppName);
    int DeleteAppStream();    
    int GetAppIndexByPayloadType(int nAppPayloadType);    
    int SendResponse();
	void PushRTPorRTCP(int ch,void *pData,int len);
    int SendData(const void *pData,size_t len);
    int SendDataV2(const void *pData,size_t len,int bType);   
    int SendMsg();     
    int SendRTP_RTCPData(int nCh,const void *pData,size_t len);
    int SendRTP_RTCPDataV2(int nCh,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,size_t len);   
    int MessageHandle();
    int MessageHandleWithoutSend();    
    int Options_handle(CMessage *pMsg);
    int Describe_handle(CMessage *pMsg);
    int Setup_handle(CMessage *pMsg);
    int Play_handle(CMessage *pMsg);
    int Pause_handle(CMessage *pMsg);
    int Teardown_handle(CMessage *pMsg);
    int Dollar_handle(CMessage *pMsg);
    int Httpget_handle(CMessage *pMsg);
    int Httppost_handle(CMessage *pMsg);
    int set_parameter_handle(CMessage *pMsg);
    int get_parameter_handle(CMessage *pMsg);
    int ExtraData_handle(CMessage *pMsg);
    int AntsComb_AddChan_handle(CMessage *pMsg);
    int AntsComb_DecChan_handle(CMessage *pMsg);    
    void *Thread();    
    unsigned int GetErrorCode()
    {
        return m_nErrorCode;
    }
    int CheckAlive(uint32_t Currtime = 0);
    void RefreshTime(uint32_t newTime){m_tLastRefreshTime = newTime;}
    void OutPutSocketRefreshTime(uint32_t newTime);
    int IsOwe(unsigned int uiIPv4[4],int nPort);
    int GetPlayReady()
    {
        return m_bPlayReady;
    }
    int GetRtpSessHandle()
    {
        return m_hRtpSess;
    }
    CRtpSessionMgr *GetRtpSessMgr()
    {
        return m_pRtpSess;
    }	
    unsigned int GetIP(char *szString = NULL,int size = 0);
	int GetIPv6(unsigned char *byIPv6,char *szString = NULL,int size = 0);
	unsigned int GetLocalIP(char *szString = NULL,int size = 0);
	int GetLocalIPv6(unsigned char *byIPv6,char *szString = NULL,int size = 0);
    unsigned int GetPort();
    int IsMulticast()
    {
        return m_bTransType == 2;
    }
    ~CClientSocket();
    int CreateVideo();
    int CreateAudio();
    int SetStreamCallBack(unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser);
    int Client_callbakcFxn(int nCallbackType,int nSubType,int nProp,int nDataType, void *pData,int nDataLen,void *pUser);
    void Poll();
    RTPSession *GetBackSession(int bAudio)
    {
        if(bAudio)
        {
            return m_pAudioSession;
        }
        else
        {
            return m_pVideoSession;
        }
    }
    int GetStreamCallBack(unsigned long *dwProp,ANTS_RTSPSERVER_STREAMDATA *fxn,void **pUser)
    {
        if (dwProp)
        {
            *dwProp = m_dwStreamDataProp;
        }
        if (pUser)
        {
            *pUser = m_pStreamDataUser;
        }
        if (fxn)
        {
            *fxn = m_fStreamDataCallback;
        }
        return 0;
    }
    int OnSendFailAsk_callback();
    // 对象操作。
    void SetUsed(int bUse)
    {
        if(bUse)m_bUsing++;
        else m_bUsing--;
    }
    int IsUsed()
    {
        return m_bUsing;
    }
    void SetDeleteFlag()
    {
        m_bWaitDelete = 1;
    }
    int IsWaitDelete()
    {
        return m_bWaitDelete;
    }
    int GetSessionID()
    {
        return m_nSessionID;
    } 
    void SetThreadExit(int bExit)
    {
        m_bThreadExit = bExit;
    }      
    int GetThreadFlag()
    {
        return m_bThreadFlag;
    } 
    void SetSendUDPFlag(int bSend)
    {
        m_bCheckAndSendUDP = bSend;
    }
    int IsSendUDP()
    {
        return m_bCheckAndSendUDP;
    }
	int SetNeedClose(int nReason = 0);
	int IsIPv6()
	{
		return m_bIPv6;
	}
	

private:
    int incomingRequestHandler();
    int CheckRecvHandler();
    void selectHandler();
    void Fd_Set();

	int m_bIPv6;
    
    int m_nClientSocket;
    int m_nSessionID;
    int m_bHasError;
    unsigned int m_nErrorCode;
    int m_bNeedClose;
	int m_nReason;


    CClientSocket *m_pNext;
    CClientSocket *m_pPrev;
    friend class CRtspServer;    
    int m_bCommandValid;
    int m_nReadLen;
	int m_nReadPos;
    int m_nWriteLen;
    int m_nWritePos;
    int m_nSendPos;
    CMessage *m_pMsg;
    CMessage *m_pMsg_tail;
    static int m_nSessionCnt;

    int m_bAntsComb;
    CMessage m_tMsg;

    char *m_pCurrUrl;
    char *m_pRequire;
    int m_nRequireType;//0- 不处理/无效;1-视频,2-音频;3-音视频
    char *m_pUserName;
    char *m_pPassword;
    int m_bAuthOK; // 验证OK
    char *m_pNonce;

    int m_nRequireVideoType;
    int m_nRequireAudioType;

    int m_bRecording;// 是否回放 
    Ants_RtspDayTime m_tRecStart;
    Ants_RtspDayTime m_tRecStop;
    int m_nLastPlaySeq;
    int m_bRateControl;
    int m_bCreateRecordStream; // 是否内部创建的流。
    int m_nScale;
    static unsigned int m_dwRecordCreateCnt;

    CH264RtpSession *m_pVideoSession;
    CAudioRtpSession *m_pAudioSession;
    int m_nVideoBasePort;
    int m_nAudioBasePort;

    unsigned long m_dwStreamDataProp;
    ANTS_RTSPSERVER_STREAMDATA m_fStreamDataCallback;
    void *m_pStreamDataUser;

    int m_bStreamOpen;// 1-打开流了，关闭SOCKET的时候要调用关掉流。
    int m_bPlayReady;// 已经发送describe，配置改变时需要关掉客户端。


    int m_bTransType;// 0 - UDP,1-广播,2- 多播,3-TCP,4-http

    char m_stype;//0-auto,1-unicast,2-multicast;
    char m_ptype;//0-auto,1-udp,2-tcp,3-http


    unsigned int m_dwIPAdress_VA[4 + RTSP_APP_SUPPORT_MAX_NUM][4];//
    int m_nPort_VA[4 + RTSP_APP_SUPPORT_MAX_NUM];//??
    int m_nTTL_VA[4 + RTSP_APP_SUPPORT_MAX_NUM];


    int m_bSetup;//bit0-video,bit1-audio
    int m_bPlay;

    unsigned int  m_dwIPAddress[4];
    int m_uPort;

	int m_bPlaying_status;


    int m_bDollar;
    int m_nDollarLen;
    int m_nDollarPos;
    char *m_pDollarBuffer;
    char m_pDollarBuffer0[4];



    int m_bOverHTTP;
    char *m_pOverHttpSessionCookie;
    CClientSocket *m_pOutPutSocket;
	CClientSocket *m_pInPutSocket;

    int m_hRtpSess;//?
    CRtpSessionMgr *m_pRtpSess;

    CRtspServer *m_pRtspServer;


    JMutex m_hWriteLock;
    JMutex m_hMsgLock;


    //time_t m_tLastRefreshTime;
    uint32_t m_tLastRefreshTime;


    unsigned char m_pRequestBuffer[RTSP_BUFFER_RECV_SIZE + 1];
    unsigned char m_pResponseBuffer[RTSP_BUFFER_SEND_SIZE + 1];

    int m_bWaitDelete; //等待删除的，不再应该使用这个对象
    int m_bUsing;// 是否在使用中。在使用中是不能删除的。
    int m_nMaxNumSocket;
    int m_bThreadExit;
    int m_bThreadFlag;    
    fd_set m_readSet;
    fd_set m_writeSet;
    fd_set m_exceptionSet;

    void  *pStreamUser;  
    int    m_bModeOpen;

    uint32_t m_dwIPv4Multicast[2][4];
    uint16_t m_nMulticastPort[2];
    uint32_t m_nMulticastCnt[2]; 
    int      m_nTTL[2];    

    int      m_bSupportMulticast;
    int      m_bPlayloadType[2];
    int      m_nPlayloadStreamType[2];
    int      m_nPlayloadClockRate[2];
    char      m_szPlayloadName[2][32];
    int      m_StreamType;

    RTSP_APP_MGR_T *m_pAppMgr[RTSP_APP_SUPPORT_MAX_NUM];

    int      m_nStream;
    int      m_nCh;

    int      m_bTCP;//0-udp 1-TCP 2-对讲udp  3-对讲tcp
    int      m_bUsed;   

    RTSP_SEND_MSG_T *m_pSendMsg; 
    RTSP_SEND_MSG_T *m_pSendMsg_tail;  

    char *m_pSPS;
    char *m_pPPS;
    int m_nSPSLen;
    int m_nPPSLen;
    char *m_pVPS;
    int m_nVPSLen; 
	char *m_pVPS_un;
	int m_nVPS_un_Len;

    int m_nInterleaved;
    int m_bCheckAndSendUDP; // 判断是否当前会话来主导UDP数据的发送（利用本会话的线程）
	char m_sInteropConstraints[100];
    SSL *m_pSsl;
};
class CRtpSessionMgr
{
public:
    CRtpSessionMgr(char *pToken,int bFixed = 0,int bTCP = 0,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn = NULL,void *pUser=NULL);
    ~CRtpSessionMgr();

    int GetHandle()
    {
        return m_nStreamHandle;
    }
    int CreateVideo(int nPayloadType,int bIPv6 = 0);
    int CreateAudio(int nPayloadType,int bIPv6 = 0);
    int SetConfig(int nCmdType,void *pData,int nDataSize);
    int GetSourceCnt()
    {
        return (m_pVideoSess != NULL) + (m_pAudioSess != NULL);
    }
    int HasVideo()
    {
        return (m_pVideoSess != NULL);
    }
    int HasAudio()
    {
        return (m_pAudioSess != NULL);
    }
    CH264RtpSession *GetVideo()
    {
        return m_pVideoSess;
    }
    CAudioRtpSession *GetAudio()
    {
        return m_pAudioSess;
    }    
    int GetStreamType();
    char *GetToken()
    {
        return m_pToken;
    }
    int GetPayloadType(int bAudio);
    int SetPayloadType(int nPlayloadType,int bAudio);
    int GetPayloadStreamType(int *nStreamType,int *nPlayloadType,int *nClockRate,char *pPlayloadName,int bAudio);
    int SetPayloadStreamType(int nStreamType,int nPlayloadType,int nClockRate,char *pPlayloadName,int bAudio);
    int GetUseCnt(int bTCP = 0)
    {
        return bTCP?m_nUseCntTCP:m_nUseCnt;
    }
    int InputData(int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);
    int AddDestination(unsigned int uiIPv4[4],int nPort,int nCh = 0,int bAudio = 0,int bTCP = 0,void *pClient=NULL,int bIPv6 = 0);
    int DeleteDestination(unsigned int uiIPv4[4],int nPort,int nCh = 0,int bAudio = 0,int bTCP = 0,void *pClient=NULL);
    int StartMulticastIPv4(unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio = 0,int bIPv6 = 0);
    int StartMulticastIPv4NoApply(unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio = 0,int bIPv6 = 0);
    int StopMulticastIPv4(int bAll = 0,int bAudio = 0,int bIPv6 = 0);
    int GetMulticastIPv4(unsigned int *uiIPv4,int *nPort,int *nTTL,int bAudio = 0,int bIPv6 = 0);
    uint32_t GetTimeOut()
    {
        return m_nTimeOut;
    }
    void Poll();
    int GetSPS(char *pSPSData,int nBuffSize);
    int GetPPS(char *pPPSData,int nBuffSize);
    char * GetPPS_Base64();
    char * GetSPS_Base64();
    char * GetVPS_Base64();
    int GetProfileLevelID();
    int IsVideoReady();
    int SetSupportMulticast(int bSupportMulticast)
    {
        int bLast = m_bSupportMulticast;
        m_bSupportMulticast = bSupportMulticast;
        return bLast;
    }
    int GetSupportMulticast()
    {
        return m_bSupportMulticast;
    }
    void SetTime(unsigned int uSec,unsigned int uMSec);
    int SetStreamCallBack(unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser)
    {
        m_dwStreamDataProp = dwProp;
        m_fStreamDataCallback=fxn;
        m_pStreamDataUser=pUser;
        return 0;
    }
    int GetStreamCallBack(unsigned long *dwProp,ANTS_RTSPSERVER_STREAMDATA *fxn,void **pUser)
    {
        if (dwProp)
        {
            *dwProp = m_dwStreamDataProp;
        }
        if (pUser)
        {
            *pUser = m_pStreamDataUser;
        }
        if (fxn)
        {
            *fxn = m_fStreamDataCallback;
        }
        return 0;
    }
    int SetAutoPayloadType(int bAuto,int bAudio);
    int IsAutoPayloadType(int bAudio);
    int AddAppStream(int nAppPayloadType,unsigned int dwClockRate,char *szAppName);
    int DeleteAppStream(int nAppPayloadType);
    int InputDataEx(int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);
    int InputAntsCombData(int nType,int nChan,int nStreamIdx,void *pData,int nDataSize);
    int GetAppStreamInfo(int nIdx,int *nAppPayloadType,unsigned int *dwClockRate,char **szAppName);
    int GetAppIndexByPayloadType(int nAppPayloadType);
    CAppRtpSession *GetAppSession(int nIdx)
    {
        return m_pAppMgr[nIdx]?m_pAppMgr[nIdx]->pSess:NULL;
    }
    CAppRtpSession *GetAppSessionByType(int nAppPayloadType,unsigned int dwClockRate,char *szAppName)
    {
        int i;
        
        if (szAppName == NULL)
        {
            return NULL;
        }
        
        for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
        {
            if(m_pAppMgr[i] != NULL)
            {
                if (m_pAppMgr[i]->byAppPayloadType == nAppPayloadType ||
                    0 == strcmp(m_pAppMgr[i]->szAppName,szAppName ))
                {
                    return m_pAppMgr[i]?m_pAppMgr[i]->pSess:NULL;
                }
            }
        }   

        return NULL;
    }  
    int GetAppCnt()
    {
        int i = 0, cnt = 0;
              
        for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
        {
            if(m_pAppMgr[i] != NULL)
            {
                if (m_pAppMgr[i]->pSess != NULL)
                {
                    cnt += m_pAppMgr[i]->pSess->GetDestinationCnt();
                }
            }
        }   

        return cnt;
    }       
    void SetRecordExHeader_C(uint8_t c,int bAudio = 0);
    void SetRecordExHeader_E(uint8_t e,int bAudio = 0);
    void SetRecordExHeader_D(uint8_t d,int bAudio = 0);
    void SetRecordPlaySeq(int nSeq);
    void SetRecordAbsTimeStamp(uint32_t Sec,uint32_t uSec,int bAudio = 0);

    // 对象操作。
    void SetUsed(int bUse)
    {
        if(bUse)m_bUsing++;
        else m_bUsing--;
    }
    int IsUsed()
    {
        return m_bUsing;
    }
    void SetDeleteFlag()
    {
        m_bWaitDelete = 1;
    }
    int IsWaitDelete()
    {
        return m_bWaitDelete;
    }
    int SetStreamUser(void *pUser)
    {
        pStreamUser = pUser;
        return 0;
    }
    void *GetStreamUser()
    {
        return pStreamUser;
    }  
    int GetIsTCP()
    {
        return m_bTCP;
    }   
    int IsRecording()
    {
        return m_bRecording;
    }   
    void SetSetup(int bSetup)
    {
        if(bSetup) m_bSetup++;
        else m_bSetup--;
    }
    int IsSetup()
    {
        return m_bSetup;
    }   
    int SetCurSendSessionID(int nSessionID)
    {
        m_nSessionID = nSessionID;
        return 0;
    }
    int GetCurSendSessionID()
    {
        return m_nSessionID;
    }
	char *GetPoken()
	{
        return m_pToken;
	}
    int  IsAntsComb();
	void SetCombFlag(int bAntsComb);
private:
    CH264RtpSession *m_pVideoSess;//?
    CAudioRtpSession *m_pAudioSess;//?
    int m_nVideoRtpPort;
    int m_nAudioRtpPort;
    int m_nVideoPayloadType;
    int m_nAudioPayloadType;
    int m_nUseCnt;
    int m_nUseCntTCP;
    int m_bFixed;//????
    char *m_pToken;
    int m_nStreamHandle;
    static unsigned char m_byStreamHandleCnt;
    int m_bTransType;// 0 - UDP,1-?,2-?,3-TCP
    uint32_t m_nTimeOut;//0-,
    int m_bSupportMulticast;

    int m_bRecording;
	int m_bAntsComb;

    int m_bVideoType_Auto;
    int m_bAudioType_Auto;

    uint32_t m_dwIPv4Multicast[2][4];
	uint32_t m_dwIPv4MulticastIPv6[2][4];
    uint16_t m_nMulticastPort[2];
    uint32_t m_nMulticastCnt[2];
	uint32_t m_nMulticastCntIPv6[2];

    int m_nTTL[2];
    ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK m_fTCPDataCallBack;
    void   *m_pRtpRtcpCallbackUser;

    unsigned long m_dwStreamDataProp;
    ANTS_RTSPSERVER_STREAMDATA m_fStreamDataCallback;
    void *m_pStreamDataUser;


    CRtpSessionMgr *m_pNext;
    friend class CRtspServer;
    unsigned int m_dwRunTimeSecond;
    unsigned int m_dwRunTimeMSecond;
    RTSP_APP_MGR_T *m_pAppMgr[RTSP_APP_SUPPORT_MAX_NUM];

    int m_bWaitDelete; //等待删除的，不再应该使用这个对象
    int m_bUsing;// 是否在使用中。在使用中是不能删除的。
    int m_bSetup;

    int m_bTCP;
    void  *pStreamUser;   

	int m_nSessionID;
};



class CRTSPOverHttpSessionCookie
{
public:
    CRTSPOverHttpSessionCookie(CClientSocket *pinput,CClientSocket *pOutput,char *pCookie);
    ~CRTSPOverHttpSessionCookie();
    void AddInputClient(CClientSocket *pPost);
    void AddOutputClient(CClientSocket *pGet);
    char *m_pXsessioncookie;
    CClientSocket *m_pGet;//output server --> client
    CClientSocket *m_pPost;//input server <--  client
    CRTSPOverHttpSessionCookie *m_pNext;

};

class CRtspServer:JThread
{
public:
    CRtspServer();
    int Create(int nPort = 554);
	int CreateIPv4(int nPort = 554);
	int CreateIPv6(int nPort = 554);
    int Destroy();
    ~CRtspServer();


    //void SetRtspStatus(int nStatus);

    int SetTunnelingOverHTTP(int nHttpPort);
	int SetTunnelingOverHTTPIPv4(int nHttpPort);
	 int SetTunnelingOverHTTPIPv6(int nHttpPort);

    int CreateStream(char *pToken,int *bExist,int bTCP,int bStreamType = 0/*0-av,1-v,2-a*/,int nVideoPayloadType = PAYLAODTYPE_H264,int nAudioPayloadType = PAYLAODTYPE_G711U,int bFixed = 0,int bIPv6 = 0);//?
    char *GetStreamToken(int handle);
    int DestroyStream(char *pToken);
    int DestroyStream(int handle);
    int CloseStream(int handle);
    CRtpSessionMgr *GetSessionMgrByHandle(int handle);
    int InputData(int handle,int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);
    int InputAntsData(int handle,void *pData,int nDataSize,int bAdpcm2G711u = 0);
    int SetConfig(int handle,int nCmdType,void *pData,int nDataSize);
    int UDPData(CRtpSessionMgr *pSess = NULL, int nSessionID = 0);    
    int AddAppStream(int handle,int nAppPayloadType,unsigned int dwClockRate,char *szAppName);
    int DeleteAppStream(int handle,int nAppPayloadType);
    int InputDataEx(int handle,int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);
    int InputAntsCombData(int handle,int nType,int nChan,int nStreamIdx,void *pData,int nDataSize);
    int GetAppStreamInfo(int handle,int nIdx,int *nAppPayloadType,unsigned int *dwClockRate,char **szAppName);



    int IsAutoPayloadType(int handle,int bAudio);
    uint32_t GetStreamTimeOut(int handle);
    int AddClientSocket(int clientSocket,int bIPv6,SSL *pSsl);

    int SendRTP_RTCPData(CClientSocket *pClient,int nCh,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,size_t len);
    

    void RemoveClientFromDelList();
    void RemoveSessionMgrFromDelList();
    void RemoveAllClientSocket();
    void RemoveAllNoReadyClientSocket();
    void RemoveClientSocket(int clientSocket);
    void RemoveClientSocket(CClientSocket *pClient);
    int LookupParam(char *pParam,CRtpSessionMgr **pRtpSess);//handle
    
    int GetServerBasePortByHandle(int handle);
    int GetServerBasePortAByHandle(int handle);
    unsigned int GetSSRCByHandle(int handle,int bAudio);
	void SetSSRCByHandle(int handle,int bAudio,unsigned int uSSRC);
    int GetServerBaseAppPortByHandle(int handle,int nIdx);
    unsigned int GetAppSSRCByHandle(int handle,int nIdx);

    int AddRtpDataClient(int handle,unsigned int uiIPv4[4],int nPort,int nCh,int bAudio = 0,int bTCP = 0,void *pClient = NULL,int bIPv6 = 0);
    int RemoveRtpDataClient(int handle,unsigned int uiIPv4[4],int nPort,int nCh,int bAudio = 0,int bTCP = 0,void *pClient = NULL);

    int GetMulticastAddressIPv4(int handle,unsigned int *uiIPv4,int *nPort,int *nTTL,int bAudio = 0,int bIPv6 = 0);
    int SetMulticastAddressIPv4(int handle,unsigned int uiIPv4[4],int nPort,int nTTL = 255,int bAudio = 0,int bIPv6 = 0);
    int SetStreamInfo(int handle,int bStreamType,int nVideoPayloadType,int nAudioPayloadType);   
    int SetMulticastAddressIPv4NoApply(int handle,unsigned int uiIPv4[4],int nPort,int nTTL = 255,int bAudio = 0,int bIPv6 = 0);

    int AddHttpSessionCookie(CClientSocket *pGet,char *pCookie);
    int AddPostHttpSessionCookie(CClientSocket *pPost,char *pCookie);
    CClientSocket *GetOutPutClientByHttpSessionCookie(char *pCookie);
    CClientSocket *GetInPutClientByHttpSessionCookie(char *pCookie);
    CClientSocket *GetOtherClientInHttpSessionByClient(CClientSocket *pClient);
    int RemoveHttpSessionCookie(char *pCookie);
    int RemoveHttpSessionCookieByClient(CClientSocket *pClient);
    int RemoveAllHttpSessionCookie();

    int GetSourceCnt(int handle);
    int HasVideo(int handle);
    int HasAudio(int handle);
    int GetStreamType(int handle);
    int GetPayloadType(int handle,int bAudio);
    int SetPayloadType(int handle,int nPlayloadType,int bAudio);
    int GetPayloadStreamType(int handle,int *nStreamType,int *nPlayloadType,int *nClockRate,char *pPlayloadName,int bAudio);
    int SetPayloadStreamType(int handle,int nStreamType,int nPlayloadType,int nClockRate,char *pPlayloadName,int bAudio);

    int ChangeTimeStampType(int handle);

    int GetSPS(int handle,char *pSPSData,int nBuffSize);
    int GetPPS(int handle,char *pPPSData,int nBuffSize);
    int GetProfileLevelID(int handle);

    char * GetPPS_Base64(int handle);
    char * GetSPS_Base64(int handle);
    char * GetVPS_Base64(int handle);
    int IsVideoReady(int handle);

    void SetStreamControlCallBack(RTSPServer_StreamControl *pStreamControl, void *pUser);
    int fOpen(int hStreamHandle,void *pInParam,void *pOutParam,void **pStreamUser);
    int fClose(int hStreamHandle,void *pStreamUser,int nStreamType,char *pUrl);
    int fRead(int hStreamHandle,void *pStreamUser,int nStreamType, char **pData,int *pDataSize,void *pStreamInfo, int *nReadType, int *bAdpcm2G711u);
    int fRelease(int hStreamHandle,void *pStreamUser,int nStreamType);
    int fControl(int hStreamHandle,int nType,void *pInParam,void *pOutParam,void *pStreamUser);
      
    void SetStatusCallBack(ANTS_RTSPSTATUSINTERCALLBACK fxn);
    void SetSendTCPDataCallBack(ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser=NULL);
    int fStatusCallBack(int hStreamHandle,int nType, void *pInParam,void *pOutParam);
    int fStatusCallBack(int nType, void *pInParam,void *pOutParam);

    void IncWriteLen(int len);
    void SendData();



    void DecWriteLen(int len);
    void WriteWait();



    unsigned int GetLocalIP();



    static char *GetPayloadTypeName(int nPayloadType);
    static int GetPayloadClockRate(int nPayloadType);

    int CheckAlive();
    void *Thread();
    int GetCurrHandle()
    {
        return m_hRtspHandle;
    }
    void SetCurrHandle(int hRtspHandle)
    {
        m_hRtspHandle = hRtspHandle;
    }

    int SetSupportMulticast(int hStreamHandle,int bSupportMulticast);
    int GetSupportMulticast(int hStreamHandle);
    int SetStreamCallBack(unsigned int dwSessionID,unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser);
    int GetStreamCallBack(unsigned int dwSessionID,unsigned long *dwProp,ANTS_RTSPSERVER_STREAMDATA *fxn,void **pUser);
    int IsOldRecording()
    {
        return m_bOldRecording;
    }

    // 对象操作。
    void SetUsed(int bUse)
    {
        if(bUse)m_bUsing++;
        else m_bUsing--;
    }
    int IsUsed()
    {
        return m_bUsing;
    }
    void SetDeleteFlag()
    {
        m_bWaitDelete = 1;
    }
    int IsWaitDelete()
    {
        return m_bWaitDelete;
    }
    int GetFxnMode()
    {
        return fxnMode;
    } 
    void GLock(){m_lock.Lock();}
    void GUnlock(){m_lock.Unlock();}

private:
    int AddRtpSessionMgr(CRtpSessionMgr *pMgr);
    int RemoveRtpSessionMgr(char *pParam);
    int RemoveAllRtpSessionMgr();
    int RemoveRtpSessionMgr(CRtpSessionMgr *pMgr);
    int incomingConnectionHandler(int bOverHttp = 0,int nClientSocket = -1,int bIPv6 = 0);
    int incomingRequestHandler(CClientSocket *pClient);
    void selectHandler();
    void Fd_SetAll();
    void Poll();
    void SetTime(unsigned int uSec,unsigned int uMSec);

    int m_bInit;
    int m_hRtspHandle;
    int m_nSocket;
	int m_nSocketIPv6;
    int m_nServerPort;
    int m_nOverHttpSocket;
	int m_nOverHttpSocketIPv6;
    int m_nOverHttpPort;
    int m_nMaxNumSocket;
    int m_bThreadExit;
    int m_bThreadFlag;
    fd_set m_readSet;
    fd_set m_writeSet;
    fd_set m_exceptionSet;
    char m_test[24];
    CClientSocket *m_pClientSocketList;
    CClientSocket *m_pClientSocketDelList;
    CRtpSessionMgr *m_pRtpSessionMgrList;
    CRtpSessionMgr *m_pRtpSessionMgrDelList;
    JMutex m_lock;


    CRTSPOverHttpSessionCookie *m_pHttpSessionCookieList;


    CAntsSemaphore m_hWriteSem;
    int m_bWriteWait;

    int m_nTotalWriteSize;//

    int m_bRelTimeStamp;
    int m_bSupportMulticast;

    unsigned long m_dwStreamDataProp;
    ANTS_RTSPSERVER_STREAMDATA m_fStreamDataCallback;
    void *m_pStreamDataUser;



    ANTS_RTSPSTATUSINTERCALLBACK m_fStatusCallBack;
    ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK m_fTCPDataCallBack;
    void *m_pRtpRtcpCallbackUser;

    unsigned int m_dwRunTimeLastSecond;
    unsigned int m_dwRunTimeSecond;
    unsigned int m_dwRunTimeMSecond;

    unsigned int m_uiLastFrameNo;				//!帧号

    int m_bOldRecording;// 是否为老的回放方式 recordstream

    int m_bWaitDelete; //等待删除的，不再应该使用这个对象
    int m_bUsing;// 是否在使用中。在使用中是不能删除的。

    ANTS_RTSP_CALLBACK_OPEN          fxnOpen;
	ANTS_RTSP_CALLBACK_CLOSE         fxnClose;
	ANTS_RTSP_CALLBACK_READ          fxnRead;
	ANTS_RTSP_CALLBACK_READ_RELEASE  fxnRelease;
	ANTS_RTSP_CALLBACK_CONTROL       fxnControl;
	void                            *fxnUser;

	int  fxnMode;

    SSL_CTX *m_sslCtx;

};

















#endif
