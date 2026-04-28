#ifndef _RTSP_CLIENT_H_
#define _RTSP_CLIENT_H_
#include <jthread/jthread.h>
#include <rtpsession.h>

#ifndef RTSP_NO_CLIENT

#include "h264rtpsession.h"
#include "audiortpsession.h"
#include "apprtpsession.h"


#include "rtsp_common.h"
#include "rtsp_msg.h"
#include "rtspclient_api.h"
#include "rtsp_data_thread.h"

using namespace jthread;
using namespace jrtplib;
#define USE_ONE_THREAD

#define NO_RTCP_SUPPORT
#define NO_BLOCK_OP

#define MAX_RTP_PAYLOAD_NUM  4 //RTP 地址，最大支持的负载数

#define RTSPCLIENT_OPEN_MODE_AUTO 0
#define RTSPCLIENT_OPEN_MODE_UDP 1
#define RTSPCLIENT_OPEN_MODE_MULTI 2
#define RTSPCLIENT_OPEN_MODE_TCP 3
#define RTSPCLIENT_OPEN_MODE_HTTP 4

#define RTSPCLIENT_RECV_BUFFSIZE  (2048)
#define RTSPCLIENT_SEND_BUFFSIZE  1024

#define RTSPCLIENT_RECV_RAW_BUFFSIZE  (64 * 1024)

#define RTSPCLIENT_MAX_SUPPORT_CMD_NUM  8

#define RTSPCLIENT_NODATA_TIMEOUT    30
#define RTSPCLIENT_NOIFRAME_TIMEOUT    200

typedef struct _tagAntsRtspClient_BufferMgr
{
    unsigned char *pBuffer;
    int nBufferSize;
    int nCh;
    int nStream;
    int nStreamType;
    struct _tagAntsRtspClient_BufferMgr *pNext;
}AntsRtspClient_BufferMgr_T;


#define RTSPCLIENT_REQ_LIST_MAX    20

typedef struct _tagAntsRtspClient_Req_List
{
    int nCmd; // 请求命令
    int nSeq; // 执行序号
    int bRes;// 是否要求应答
    struct _tagAntsRtspClient_Req_List *pNext;
}AntsRtspClient_Req_List_T;


#define RTSPCLIENT_CHAN_MAX    512
class CRtspClient:public JThread
{
public:
    CRtspClient();
    ~CRtspClient();
    void SetHandle(int handle){m_nClientHandle = handle;}
    void SetCallBack(ANTS_RTSPCLIENTCALLBACK pCallback,void *pUserData);
    ANTS_RTSPCLIENTCALLBACK GetCallBack();
    void SetCallBackEx(ANTS_RTSPREALDATACALLBACKFXN pCallback,void *pUserData);
    void SetCallBackV2(ANTS_RTSPREALDATACALLBACKFXN_V2 pCallback,void *pUserData);
    ANTS_RTSPREALDATACALLBACKFXN GetCallBackEx();
    void *GetCallbackUserData();
    void SetSysTime(unsigned int Sec,unsigned int uSec)
    {
        m_dwRunTimeSecond = Sec;
        m_dwRunTimeMSecond = uSec / 1000;
    }

    ANTS_RTSPREALDATACALLBACKFXN_V2 GetCallBackV2();
    void SetProp(unsigned int dwProp)
    {
        m_dwProp = dwProp;
        if(dwProp & 2)m_bSpecial_Ex = 1;
    }
    unsigned int GetProp(){return m_dwProp;}
    void SetSpecialFlag(int bFlag){m_bSpecial_Ex = bFlag;}
    int GetSpecialFlag(){return m_bSpecial_Ex;}
    int Open(char *pUrl,int nMode,char *pUserName,char *pPassword);
    void StartThread();
    const char *GetTstConfig(int *pVideoType,int *pAudioType,int *pWidth,int *pHeight,int *pFrameFrate);
    int Control(int nCmd,unsigned long dwApplyID,void *pInBuffer,int nInSize,void *pOutBuffer,int nOutSize);
    int InputAntsData(void *pData,int nDataSize);
    int InputData(void *pData,int nDataSize);	
    int Close();
    int Client_callbakcFxn(int nCallbackType,int nSubType,int nProp,int nDataType, void *pData,int nDataLen);

    int IsNeedReConnect();

    int SendRTP_RTCPData(int nCh,const void *pData,size_t len);
    int BufferMgrRead(int *nChan,int *nStream,int *nStreamType,unsigned char **lpBuffer,unsigned long *dwBufSize);
    int BufferMgrReadRelease();


    int DataPacketRecvThread();
    char *GetUrl();
    int GetMode();
    char *GetUserName();
    char *GetPassword();
	void NeedClose(int nReason = 0);


private:
    int Option();
    int Describe();
    int SetupV();
    int SetupA();
    int SetupApp();
    int SetupTalk();
    int Play();
    int Pause();
    int Set_Parameter();
    int Get_Parameter();
    int AntsComb_AddCh(int *pChan,int nCnt);
    int AntsComb_DecCh(int *pChan,int nCnt);
    
    int TearDown();


    int PollData();
    int PollData_RTP();
    void Connect();



    int Option_handle(CMessage *pMsg);
    int Dollar_handle(CMessage *pMsg);
    int Describe_handle(CMessage *pMsg);
    int SetupV_handle(CMessage *pMsg);
    int SetupA_handle(CMessage *pMsg);
    int SetupApp_handle(CMessage *pMsg);
    int SetupTalk_handle(CMessage *pMsg);
    int Play_handle(CMessage *pMsg);
    int Error_handle(CMessage *pMsg,AntsRtspClient_Req_List_T *pReq = NULL);

    int AddReqList(int nCmd,int nSeq,int bRes);
    
    
    int SendData(const void *pData,size_t len);
    int DealSend();
    int DealRecv();
    int DealCmd();
    int GenerateSeq();
    CMessage *Parse(char *pData,int nDataSize,int bExt);
    int RecvAndParse();
    int RecvAndParse_noblock();
	int RecvAndParse_noblock_rtp();
    void PushRTPorRTCP(int ch,void *pData,int len);
    CMessage *GetMessage();
    void *RTPThread();
    void *Thread();

    int CreateRtpSocketTcp(int nPort);
    int CreateRtpVideo(int nmode, int nPort = 0);	
	int CreateRtpSecondVideo(int nmode, int nPort = 0);
    int CreateVideo(int nmode, int nPort = 0);
    int CreateAudio(int nmode);
    int CreateApp(int nmode);
    int CreateTalk(int nmode);
    int DestoryVideo();
    int DestoryAudio();
    int DestoryApp();
    int DestoryTalk();
    void StatusHandle(int nReason = 0);
    int CreateRTP();
private:

    int m_bConnected;//0-un,1-ing,2-Connected
    int m_nFd;
    int m_bOpen;
    int m_nSocket;
    int m_nMode;
    int m_bReady;
    int m_bConnecting;
    JMutex m_hConnectStatusLock;

    int m_nAutoConnect;// 是否自动连接，默认自动
    int m_nAutoReConnect;

    int m_bOption; // 1-req,2-ok
    int m_bDescribe;// 1-req,2-ok
    int m_bSetup;// 1-req,2-ok



    int m_bFirstSetup;
    int m_bStart; // 历史流，开始
    int m_bReplay;
    int m_nScale;
    int m_nScale_Req;
    int m_bRateControl;
    int m_bRateControl_Req;
    int m_bPause;
    int m_bPauseReq;
    int m_bPausing;
    unsigned int m_dwApplyID;
    int m_bSeek;
    int m_bSeek_Req;
    Ants_RtspDayTime m_tPlayStart;
    Ants_RtspDayTime m_tPlayStart_Req;

    int m_bPlay;
    int m_bPlay_Req;

    int m_bOnlyIFrame;
    int m_bOnlyIFrame_Req;

    int m_bStep;
    int m_bStep_Req;
    unsigned int m_dwStepCnt;
    unsigned int m_dwStepCnt_Req;

    int m_bIntraPause;
    int m_nRcvBuffSize;
    

 

    int m_bIFrameNeed;
    unsigned int m_nLastFrameNum;
    time_t m_nLastIframeTime;
    
    char *m_pUrl;
	int          m_bSrvIPv6;
    unsigned int m_nSrvIPv4[4];
    short   m_nSrvPort;
	int          m_bLocalIPv6;
    unsigned int m_nLocalIPv4[4];
    short   m_nLocalPort;
    char *m_pUserName;
    char *m_pPassword;
    char *m_pBase64Enc;

    char *m_pMulticastAddr[3];
    int  m_nMulticastPort[3];
    int m_nMulticastTTL[3];

    //Digest
	int m_bNeedAuth;// 0- 不验证,1- 需要验证
    int m_bDigest;
	int m_nAuthCount; // 已验证次数 ,超过3 次关闭
    char *m_pRealm;
    char *m_pNonce;

    //time_t m_tLastLiveTime;
    uint32_t m_tLastLiveTime;
    int m_nLiveCnt;
    int m_nIFrameLiveCnt;
    int m_bNoCheckNoData;
    int m_nBeforePlayLiveCnt;
    
    RTSP_COMMON_URL_T m_tUrl;
#ifdef NO_BLOCK_OP
    char *m_pRecvRawBuffer;
    int m_nRecvRawBufferPos;
    int m_nRecvRawUsePos;
#endif
    char m_pRecvBuff[RTSPCLIENT_RECV_BUFFSIZE + 4];
    int m_nRecvBuffPos;
    char m_pSendBuff[RTSPCLIENT_SEND_BUFFSIZE + 4];
    int m_nSendBuffPos;

    unsigned short m_nSeq;
    char m_nCurrOP;// 0-no op,bit0-option
    int m_nWaitOPRes;
    int m_nWaitSeq;
    int m_nNextOPIndex;
    int m_bRtspCap;// bit0-option,1-describe,2-setup,3-play,4-set_parameter
    char m_nOPOrders[RTSPCLIENT_MAX_SUPPORT_CMD_NUM];

    int m_bAudioOrder;// 音频优先连接
    int m_bSetupVideo_Req;
    int m_bSetupAudio_Req;
    int m_bSetupTalk_Req;
    int m_bSetupApp_Req;

    int m_bNeedClose;
    int m_nReason;
    int m_bSet_Parameter;
    int m_bGet_Parameter;
    int m_bPlaying;
    int m_bReadyForData;
    unsigned int m_tLastSetParameter;

    
    int m_bDollar;
    int m_nDollarLen;
    int m_nDollarPos;
    int m_nDollarPos0;
    char *m_pDollarBuffer;
    char m_pDollarBuffer0[5];
    CMessage *m_pMsg;

    int m_bHasContext;
    int m_nContextLen;
    int m_nContextPos;

    char *m_strSessionID;


    int m_bSpecial_Ex;
    int m_bVideoExist;
    int m_bAudioExist;
    CH264RtpSession *m_pVideoSess;
    CAudioRtpSession *m_pAudioSess;
	CH264RtpSession *m_pSecondVideoSess;

    int m_bAppExist;
    char *m_pApp_control;
    char *m_pApp_rtpmap;
    int m_nAppTransPort;
    int m_nAppSrvTransPort;
    CAppRtpSession *m_pAppSess;
    int m_nAppPayloadType;
    int m_nAppPayloadClockRate;
    int m_nAppStreamType;

    int m_bTalkExist;
    char *m_pTalk_control;
    char *m_pTalk_rtpmap;
    int m_nTalkTransPort;
    int m_nTalkSrvTransPort;
    CAudioRtpSession *m_pTalkSess;
    int m_nTalkPayloadType;
    int m_nTalkPayloadClockRate;
    int m_nTalkStreamType;
   

    JMutex m_nSendLock;
    JMutex m_nRecvLock;
    JMutex m_nOPLock;

    int m_bHasVideo;

    int m_nVideoPayloadType;
    int m_nVideoPayloadClockRate;
    int m_nAudioPayloadClockRate;
    int m_nAudioPayloadType;
    ANTS_RTSP_STREAM_TYPE m_nVideoStreamType;
    ANTS_RTSP_STREAM_TYPE m_nAudioStreamType;
    int m_nVideoTransPort;
    int m_nAudioTransPort;
	int m_nSecondVideoTransPort;
    
    int m_nVideoSrvTransPort;
    int m_nAudioSrvTransPort;
    int m_nTimeOut;

    int m_nClientHandle;
    unsigned int m_dwProp;// bit0:0- 调用者释放，1-被调用者释放
    ANTS_RTSPCLIENTCALLBACK m_fCallback;
    ANTS_RTSPREALDATACALLBACKFXN m_fCallback_Ex;
    ANTS_RTSPREALDATACALLBACKFXN_V2 m_fCallback_V2;
    void *m_pCallbackUserData;

    int m_bCallbackStatus;//0-no,1-start,2-end
    char *m_pBaseUrl;
    
    char *m_pVideo_control;
    char *m_pAudio_control;
    
    char *m_pVideo_rtpmap;
    char *m_pAudio_rtpmap;
    char *m_psprop_parameter_sets_sps;
    char *m_psprop_parameter_sets_pps;
    char *m_pTstConfig;
    int m_nTstWidth;
    int m_nTstHeight;
    int m_nTstFrameRate;

    unsigned int m_dwRunTimeLastSecond;
    unsigned int m_dwRunTimeSecond;
    unsigned int m_dwRunTimeMSecond;


    int m_bBufferMgrMethod;
    AntsRtspClient_BufferMgr_T *m_pBuffMgrHead;
    AntsRtspClient_BufferMgr_T *m_pBuffMgrTail;
    int m_nBufferMgrCnt;
    int m_nBufferMgrSize;
    int m_bReportBufferMgr;
    int m_bReportFull;
    JMutex m_hBuffMgrLock;

    AntsRtspClient_Req_List_T *m_pReqList;
    AntsRtspClient_Req_List_T *m_pReqList_tail;
    int m_nReqListCnt;
    JMutex m_hReqListLock;


    int m_nCombChanCurr[RTSPCLIENT_CHAN_MAX];
    int m_nCombChanCurrCnt;
    int m_nCombChanReq[RTSPCLIENT_CHAN_MAX];
    int m_nCombChanReqCnt;
    int m_nCombChanAddReqCnt;
    int m_nCombChanAddDoReqCnt;
    int m_nCombChanDecReqCnt;
    int m_nCombChanDecDoReqCnt;
    JMutex m_hAddDecChanLock;
   
    int m_nUrlType;//0 - rtsp  1 - rtp  
    char *m_pRtp_ServerIP;// NULL 无效,发送地址
    int m_nRtp_ServerPort; // -1无效 
	 int m_nRtp_ServerSecondPort; // -1无效 
    char *m_pRtp_LocalIP;// NULL 无效   ，接收地址(多播)
    int m_nRtp_LocalPort; // -1无效
    unsigned int m_dwRtp_SSRC[MAX_RTP_PAYLOAD_NUM]; // 
    unsigned int m_nRtp_pType;// 0- udp,1-tcp
    unsigned int m_bRtp_RTCP;// 0-off,1-on
    int m_bRtp_Multicast; // 是否为多播
    int m_bRtp_PassiveMode; // 0-主动方式，1-被动方式
    int m_nRtp_Dir;// 0- recv,1-send,2-both

    int m_nRtp_PayloadNum;
    int m_nRtp_TcpSockFD;
    int m_bRtp_extSockFD; // 是否为外部socket
    int m_nRtp_rPort[MAX_RTP_PAYLOAD_NUM][2]; // -1
    int m_nRtp_lPort[MAX_RTP_PAYLOAD_NUM][2];
    int m_nRtp_SockFD[MAX_RTP_PAYLOAD_NUM][2];
    int m_nRtp_StreamType[MAX_RTP_PAYLOAD_NUM];
    int m_nRtp_PayloadType[MAX_RTP_PAYLOAD_NUM];
    CH264RtpSession *m_pRtp_Session[MAX_RTP_PAYLOAD_NUM];

	int m_bRtp_DollarFlag;


};

#endif

#endif 
