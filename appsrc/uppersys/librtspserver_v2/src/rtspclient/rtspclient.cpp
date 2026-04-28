#include "rtspclient.h"
#include "rtsp_common.h"

#include <time.h>
#include "digcalc.h"
#ifndef RTSP_NO_CLIENT
#ifdef WIN32
#include<Mstcpip.h>
#include <ws2tcpip.h>
#include <netioapi.h>

#else


#endif

static int Ants_rtpsession_client_callbakcFxn (void *hClient,int nCallbackType,int SubType,int nProp,int nDataType, void *pData,int nDataLen,void *pUser)
{
    CRtspClient *pClient = (CRtspClient* )hClient;
    if (pClient == NULL)
    {
        if(nProp & 1)
        {
            if (pData != NULL)
            {
                free(pData);
                pData = NULL;
            }
        }
        return 0;
    }

    return pClient->Client_callbakcFxn(nCallbackType,SubType,nProp,nDataType,pData,nDataLen);


}

static int rtprtcp_datapacketcallback(void *pClient,unsigned int uiIPv4[4],int nPort,int nCH,int bRtcp,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,int nlen,void *pUser)
{
    CRtspClient *pClient1 = (CRtspClient *)pUser;
    if (pClient == NULL)
    {
        return 0;
    }
    pClient1->SendRTP_RTCPData(nCH,pData,nlen);
    return 0;
}
CRtspClient::CRtspClient()
{
    int i;
    m_bOpen = 0;
    m_nFd = -1;
    m_nSocket = -1;
    m_nMode = 0;
    m_pUrl = NULL;
	memset(m_nSrvIPv4,0,sizeof(m_nSrvIPv4));
	m_bSrvIPv6 = 0;
    m_nSrvPort = 0;
    m_nSeq = 0;
    m_bRtspCap = 0;
    m_nRecvBuffPos = 0;
    m_nSendBuffPos = 0;
    m_nSeq = 0;
    m_nCurrOP = 0;
    m_bRtspCap = 0;
    m_bNeedClose = 0;
    m_bDollar = 0;
    m_nDollarPos = 0;
    m_nDollarLen = 0;
    m_pDollarBuffer = NULL;
    m_nDollarPos0 = 0;
    m_pMsg = NULL;
    m_nWaitOPRes = 0;
    m_pVideo_control = NULL;
    m_pAudio_control = NULL;
    m_pBaseUrl = NULL;
    m_bVideoExist = 0;
    m_bAudioExist = 0;
    m_strSessionID = NULL;
    m_pVideo_rtpmap = NULL;
    m_pAudio_rtpmap = NULL;
    m_psprop_parameter_sets_sps = NULL;
    m_psprop_parameter_sets_pps = NULL;
    m_nVideoTransPort = 1234;
    m_nAudioTransPort = 3456;
    m_nVideoSrvTransPort = 0;
    m_nAudioSrvTransPort = 0;
    m_nVideoPayloadClockRate = 90000;
    m_nAudioPayloadClockRate = 8000;
    m_pAudioSess = NULL;
    m_pVideoSess = NULL;
	m_pSecondVideoSess = NULL;

    m_pUserName = NULL;
    m_pPassword = NULL;
    m_pBase64Enc = NULL;
    m_bDigest = 0;
	m_bNeedAuth = 0;
	m_nAuthCount = 0;
    m_pRealm = NULL;
    m_pNonce = NULL;
    m_nTimeOut = 60;
    m_tLastSetParameter = 0;
    m_bSet_Parameter = 0;
    m_bGet_Parameter = 0;
    m_bPlaying = 0;
	memset(m_nLocalIPv4,0,sizeof(m_nLocalIPv4));
    m_bLocalIPv6 = 0;
    m_nLocalPort = 0;
    m_bConnected = 0;
    m_bCallbackStatus = 0;
    m_nReason = 0;
#ifdef NO_BLOCK_OP
    m_pRecvRawBuffer = NULL;
    m_bHasContext = 0;
    m_nRecvRawUsePos= 0;
    m_nRecvRawBufferPos = 0;
#endif
    m_fCallback = NULL;
    m_fCallback_Ex = NULL;
    m_pCallbackUserData = NULL;
    m_fCallback_V2 = NULL;
    m_bReady = 0;
    m_bSpecial_Ex = 0;
    m_dwProp = 0;

    m_bIFrameNeed = 1;
    m_nLastFrameNum = 0;
    m_bConnecting = 0;
    m_pTstConfig = NULL;
    m_nTstWidth = 0;
    m_nTstHeight = 0;
    m_nTstFrameRate = 0;

    m_nLiveCnt = 0;
		m_nIFrameLiveCnt = 0;
        m_bReadyForData = 0;


    for (i = 0; i < RTSPCLIENT_MAX_SUPPORT_CMD_NUM;i++)
    {
            m_nOPOrders[i] = 0;
    }
    m_nOPOrders[0] = 'o'; // option
    m_nOPOrders[1] = 'd'; // describe
    m_nOPOrders[2] = 'v'; // video
    m_nOPOrders[3] = 'a'; // audio
    m_nOPOrders[4] = 'A';
    m_nOPOrders[5] = 't'; //
    m_nOPOrders[6] = 'p'; // play
    m_nSendLock.Init();
    m_nRecvLock.Init();
    m_nOPLock.Init();
    m_hConnectStatusLock.Init();

    m_pMulticastAddr[0] = NULL;
    m_pMulticastAddr[1] = NULL;
    m_pMulticastAddr[2] = NULL;
    m_nMulticastPort[0] = 0;
    m_nMulticastPort[1] = 0;
    m_nMulticastTTL[0] = 0;
    m_nMulticastTTL[1] = 0;
    m_bAudioOrder = 0;

    m_dwRunTimeMSecond = 0;
    m_dwRunTimeSecond = 0;
    m_bFirstSetup = 0;

    m_bReplay = 0;
    m_nScale = 0;
    m_nScale_Req = 0;
    m_bRateControl = 0;
    m_bRateControl_Req = 0;
    m_bPause = 0;
    m_bPauseReq = 0;
    m_dwApplyID = 0;
    m_bSeek = 0;
    m_bStart = 0;
    m_bSeek_Req = 0;
    m_bPlay_Req = 0;
    m_bPlay = 0;
    m_bOnlyIFrame = 0;
    m_bOnlyIFrame_Req = 0;

    m_bOnlyIFrame_Req = 0;
    m_bOnlyIFrame = 0;
    memset(&m_tPlayStart,0,sizeof(m_tPlayStart));
    memset(&m_tPlayStart_Req,0,sizeof(m_tPlayStart_Req));

    m_bSetupVideo_Req = 1;
    m_bSetupAudio_Req = 1;
    m_bSetupTalk_Req = 0;
    m_bSetupApp_Req = 0;

    m_bTalkExist = 0;
    m_bAppExist = 0;
    m_pTalk_control = NULL;
    m_pTalk_rtpmap = NULL;
    m_pApp_control = NULL;
    m_pApp_rtpmap = NULL;
    m_pAppSess = NULL;
    m_pTalkSess = NULL;
    m_nTalkPayloadType = -1;
    m_nTalkPayloadClockRate = 0;
    m_nTalkStreamType = -1;
    m_nAppPayloadType = -1;
    m_nAppPayloadClockRate = 0;
    m_nAppStreamType = -1;

    m_nVideoPayloadType = -1;
    m_nAudioPayloadType = -1;
    m_nVideoPayloadClockRate = 0;
    m_nAudioPayloadClockRate = 0;
    m_nVideoStreamType = ANTS_RTSP_STREAM_H264;
    m_nAudioStreamType = ANTS_RTSP_STREAM_G711U;

    m_nAutoConnect = 1;
    m_nAutoReConnect = 1;
    m_bStep = 0;
    m_dwStepCnt = 0;
    m_dwStepCnt_Req = 0;
    m_bStep_Req = 0;

    m_bIntraPause = 0;
    m_nRcvBuffSize = 65535;
    m_bPausing = 0;
    m_pBuffMgrHead = NULL;
    m_pBuffMgrTail = NULL;
    m_nBufferMgrCnt = 0;
    m_nBufferMgrSize = 0;
    m_hBuffMgrLock.Init();
    m_bBufferMgrMethod = 0;
    m_bReportBufferMgr = 0;
    m_bReportFull = 0;
    m_pReqList = NULL;
    m_pReqList_tail = NULL;
    m_nReqListCnt = 0;
    m_hReqListLock.Init();

    m_hAddDecChanLock.Init();
    memset(m_nCombChanCurr,0x80,sizeof(m_nCombChanCurr));
    memset(m_nCombChanReq,0x80,sizeof(m_nCombChanReq));
    m_nCombChanCurrCnt = 0;
    m_nCombChanReqCnt = 0;
    m_nCombChanAddReqCnt = 0;
    m_nCombChanAddDoReqCnt = 0;
    m_nCombChanDecReqCnt = 0;
    m_nCombChanDecDoReqCnt = 0;


    m_nUrlType = 0;
	memset(&m_tUrl, 0, sizeof(RTSP_COMMON_URL_T));

    //RTP
    m_pRtp_ServerIP = NULL;
    m_nRtp_ServerPort = -1;
    m_pRtp_LocalIP = NULL;
    m_nRtp_LocalPort = -1;

    m_nRtp_pType = 0;
    m_bRtp_RTCP = 0;
    m_nRtp_PayloadNum = 0;
    m_nRtp_Dir = 0;
    m_bRtp_PassiveMode = 0;
    m_bRtp_Multicast = 0;
    for (i = 0; i < MAX_RTP_PAYLOAD_NUM; i++)
    {
        m_nRtp_rPort[i][0] = -1;
        m_nRtp_rPort[i][1] = -1;
        m_nRtp_lPort[i][0] = -1;
        m_nRtp_lPort[i][1] = -1;
        m_nRtp_StreamType[i] = -1;
        m_nRtp_PayloadType[i] = -1;
        m_pRtp_Session[i] = NULL;
        m_nRtp_SockFD[i][0] = -1;
        m_nRtp_SockFD[i][1] = -1;
        m_dwRtp_SSRC[i] = 0;
    }
    m_bRtp_extSockFD = 0;
    m_nRtp_TcpSockFD = -1;
	m_bRtp_DollarFlag = 0;

}

CRtspClient::~CRtspClient()
{
    Close();

}


void CRtspClient::NeedClose(int nReason)
{

    if (0 != nReason && m_fCallback_V2 != NULL)
    {
        ANTS_RTSP_STATUS_T tStatus;
        memset(&tStatus,0,sizeof(tStatus));
        tStatus.nStatusType = ANTS_RTSP_STATUS_Error;
        tStatus.nStatusCode = nReason;

        m_fCallback_V2(m_nClientHandle,ANTS_RTSP_DATATYPE_STATUS,0,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
        if (tStatus.byOPResult == 1)
        {
			if (ANTS_RTSP_ERROR_Authenticate == nReason && m_nAuthCount >= 3)
			{
			}
			else
			{
				return;
			}
        }
        if (nReason == ANTS_RTSP_ERROR_NoAnyData)
        {
            if((m_bReplay && (m_bPausing  || m_bIntraPause)) || (!m_nAutoReConnect))
            {
                return;
            }
        }
    }
    m_nReason = nReason;
    m_bNeedClose = 1;

}

void CRtspClient::StatusHandle(int nReason)
{

    if (0 != nReason && m_fCallback_V2 != NULL)
    {
        ANTS_RTSP_STATUS_T tStatus;
        memset(&tStatus,0,sizeof(tStatus));
        tStatus.nStatusType = ANTS_RTSP_STATUS_Error;
        tStatus.nStatusCode = nReason;

        m_fCallback_V2(m_nClientHandle,ANTS_RTSP_DATATYPE_STATUS,0,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
        if (tStatus.byOPResult == 1)
        {
            return;
        }
    }
}

int CRtspClient::Client_callbakcFxn(int nCallbackType,int nSubType,int nProp,int nDataType, void *pData,int nDataLen)
{
    ANTS_RTPClientCallbackType tCallbackType;
     AntsFrameHeader *pheader;
     int bCallbackProp = 0;
     ANTS_RTSP_STATUS_T tStatus;
	 uint32_t currTime = 0;
     memset(&tStatus,0,sizeof(tStatus));



     pheader = (AntsFrameHeader *)pData;
     if (nCallbackType == ANTS_RTPClientCallbackType_Error)
     {
         RTSP_DEBUG("[%s.%d] nCallbackType = %d nSubType = %d,nProp = %d,nDataType = %d\n",__FUNCTION__,__LINE__,nCallbackType,nSubType,nProp,nDataType);
         if (nSubType == ANTS_RTSP_ERROR_DisMatchPlayloadType)
         {
             NeedClose(ANTS_RTSP_ERROR_DisMatchPlayloadType);
             if(nProp & 1)
             {
                 if (pData != NULL)
                 {
                      free(pData);
                      pData = NULL;
                 }
             }
             return 0;
         }
     }
     if(nCallbackType == ANTS_RTPClientCallbackType_Stream)
     {
          m_nLiveCnt = 0;
         if(!m_bHasVideo)
         {
             Ants_rtsp_GetSysRunTime(&m_tLastLiveTime,NULL);

             m_nIFrameLiveCnt = 0;

         }
         else if(ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB != nSubType && (nDataType == AntsPktIFrames || nDataType == AntsPktPFrames))
         {
             if (pheader == NULL)
             {
                 return 0;
             }
            //RTSP_DEBUG("[%s.%d] m_nLastFrameNum %d uiFrameNo=%d nDataType=%d uiFrameType=%d nSubType=%d m_bIFrameNeed=%d m_tLastLiveTime=%u m_nIFrameLiveCnt=%d m_nLastIframeTime=%u\n",__FUNCTION__,__LINE__,m_nLastFrameNum,pheader->uiFrameNo,nDataType,pheader->uiFrameType,nSubType,m_bIFrameNeed,m_tLastLiveTime,m_nIFrameLiveCnt,m_nLastIframeTime);
             if (m_nLastFrameNum == 0)
             {
                 m_nLastFrameNum = pheader->uiFrameNo;
             }
             else
             {
                 if (pheader->uiFrameNo != m_nLastFrameNum + 1)
                 {//丢帧了
					 printf("[RTSP] FrameNo.%u -- last %u \n",pheader->uiFrameNo, m_nLastFrameNum);
                     m_bIFrameNeed = 1;
                 }
             }
             m_nLastFrameNum = pheader->uiFrameNo;

             if (m_bIFrameNeed)
             {
                 if (nDataType == AntsPktPFrames)
                 {
                     if(nProp & 1)
                     {
                         if (pData != NULL)
                         {
                             free(pData);
                             pData = NULL;
                         }
                     }
                     return 0;
                 }
                 else
                 {
                     m_bIFrameNeed = 0;
                 }
             }

             // time(&m_tLastLiveTime);
             Ants_rtsp_GetSysRunTime(&currTime,NULL);
			 if (currTime > m_tLastLiveTime + 1)
			 {
				 printf("[RTSP] curr = %u last = %u \n",currTime,m_tLastLiveTime);
			 }
			 m_tLastLiveTime = currTime;
             m_nLiveCnt = 0;
             if (nDataType == AntsPktIFrames)
             {
                 m_nLastIframeTime = m_tLastLiveTime;
                 m_nIFrameLiveCnt = 0;
             }
         }
         else
         {
             if(ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB == nSubType)
             {
				 Ants_rtsp_GetSysRunTime(&currTime,NULL);
				 if (currTime > m_tLastLiveTime + 1)
				 {
					 printf("[RTSP] comb curr = %u last = %u \n",currTime,m_tLastLiveTime);
				 }
				 m_tLastLiveTime = currTime;
                // Ants_rtsp_GetSysRunTime(&m_tLastLiveTime,NULL);
                 m_nLiveCnt = 0;
                 //RTSP_DEBUG("[%s.%d] AntsComb m_nLastFrameNum %d uiFrameNo=%d nDataType=%d uiFrameType=%d nSubType=%d m_bIFrameNeed=%d m_tLastLiveTime=%u m_nIFrameLiveCnt=%d m_nLastIframeTime=%u\n",__FUNCTION__,__LINE__,m_nLastFrameNum,pheader->uiFrameNo,nDataType,pheader->uiFrameType,nSubType,m_bIFrameNeed,m_tLastLiveTime,m_nIFrameLiveCnt,m_nLastIframeTime);

                 m_nLastIframeTime = m_tLastLiveTime;
                 m_nIFrameLiveCnt = 0;
             }
         }
     }
     else
     {
         //RTSP_DEBUG("[%s.%d] nCallbackType = %d nSubType = %d,nProp = %d,nDataType = %d\n",__FUNCTION__,__LINE__,nCallbackType,nSubType,nProp,nDataType);
     }


      // if((nDataType == AntsPktIFrames /*|| nDataType == AntsPktPFrames || nDataType == AntsPktAudioFrames*/) && (pheader != NULL))
      //   printf("[%x.%d.%d]nCallbackType = %d,subtype = %d,type = %d,datalen = %d,no = %d  %d\n",m_nClientHandle,m_fCallback_Ex != NULL,m_fCallback!=NULL,nCallbackType,nSubType,nDataType,nDataLen,pheader->uiFrameNo,pheader->uiFrameTime * 1000 + pheader->uiFrameTickCount /1000);
     if (m_fCallback_V2 != NULL)
     {
         int nCallbackDateType;

         tCallbackType = (ANTS_RTPClientCallbackType)nCallbackType;

         m_bCallbackStatus = 1;
         if (tCallbackType == ANTS_RTPClientCallbackType_Stream)
         {
              if (nDataType == AntsPktAntsCombFrames)
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_STREAMDATA_ANTSCOMB;
             }
             else if (nDataType == AntsPktAudioFrames)
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_AUDIOSTREAMDATA;
             }
             else if (nDataType == AntsPktSysHeader)
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_SYSHEAD;
             }
			 else if (nDataType == AntsPktDTMFFrames)
			 {
				nCallbackDateType =  ANTS_RTSP_DATATYPE_RTP_DTMF;
			 }
             else
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_STREAMDATA;
             }
             if (m_bBufferMgrMethod)
             {
                 AntsRtspClient_BufferMgr_T *pMgr = new AntsRtspClient_BufferMgr_T;
                 if (pMgr != NULL)
                 {
                     int bReport = 0,bReportFull = 0,nMgrSize = 0;
                     unsigned char *pNewData = NULL;
                    memset(pMgr,0,sizeof(AntsRtspClient_BufferMgr_T));
                    if (nProp & 1)
                    {
                        pNewData = (unsigned char *)pData;
                    }
                    else
                    {
                        pNewData = (unsigned char *)malloc(nDataLen + 4);
                        if(pNewData != NULL)
                        {
                            memcpy(pNewData,pData,nDataLen);
                        }
                    }
                    pMgr->nBufferSize = nDataLen;
                    pMgr->pBuffer = (unsigned char *)pNewData;
                    if (nCallbackDateType == ANTS_RTSP_DATATYPE_STREAMDATA_ANTSCOMB)
                    {
                        pMgr->nCh = (nProp >> 16 ) & 0xFFFF;
                        pMgr->nStream = (nProp >> 8) & 0xFF;
                    }
                    m_hBuffMgrLock.Lock();

                    if (m_pBuffMgrTail == NULL)
                    {
                        m_pBuffMgrTail = m_pBuffMgrHead = pMgr;
                        bReport = 1;
                    }
                    else
                    {
                        m_pBuffMgrTail->pNext = pMgr;
                        m_pBuffMgrTail = pMgr;
                    }
                    m_nBufferMgrCnt++;
                    m_nBufferMgrSize+=nDataLen;
                    if (m_nBufferMgrSize >= m_nRcvBuffSize)
                    {
                        bReportFull = 1;
                        nMgrSize = m_nBufferMgrSize;
                        if (m_bReportFull)
                        {
                            bReportFull = 0;
                        }
                        else
                        {
                            m_bReportFull = 1;
                        }
                    }

                    m_hBuffMgrLock.Unlock();
                    if(bReport || m_bReportBufferMgr)
                    {
                        tStatus.nStatusType = ANTS_RTSP_STATUS_BUFFERFILL;
                        m_bReportBufferMgr = 0;
                        m_bReportFull = 0;
                        m_fCallback_V2(m_nClientHandle,ANTS_RTSP_DATATYPE_STATUS,0,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
                    }
                    if(bReportFull)
                    {
                        tStatus.nStatusType = ANTS_RTSP_STATUS_BUFFERFULL;
                        tStatus.nStatusCode = nMgrSize;
                        m_fCallback_V2(m_nClientHandle,ANTS_RTSP_DATATYPE_STATUS,0,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
                    }

                    bCallbackProp = 1;
                 }

             }
             else
             {
                m_fCallback_V2(m_nClientHandle,nCallbackDateType,nProp,(unsigned char *)pData,nDataLen,m_pCallbackUserData);

                 bCallbackProp = 1;
             }

         }
         else if(tCallbackType == ANTS_RTPClientCallbackType_Status ||
                 tCallbackType == ANTS_RTPClientCallbackType_Error)
         {

                nCallbackDateType = ANTS_RTSP_DATATYPE_STATUS;
                 tStatus.nStatusType = nSubType;
                 if (tCallbackType == ANTS_RTPClientCallbackType_Error)
                 {
                     tStatus.nStatusType = ANTS_RTSP_STATUS_Error;
                 }
                 tStatus.nStatusCode = nDataType;
                 m_fCallback_V2(m_nClientHandle,nCallbackDateType,0,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
                 bCallbackProp = 1;

         }



         m_bCallbackStatus = 2;


     }
     else if (m_fCallback_Ex != NULL)
     {
         int nCallbackDateType;

         tCallbackType = (ANTS_RTPClientCallbackType)nCallbackType;

         m_bCallbackStatus = 1;
         if (tCallbackType == ANTS_RTPClientCallbackType_Stream)
         {
             if (nDataType == AntsPktAudioFrames)
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_AUDIOSTREAMDATA;
             }
             else if (nDataType == AntsPktSysHeader)
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_SYSHEAD;
             }
             else
             {
                 nCallbackDateType = ANTS_RTSP_DATATYPE_STREAMDATA;
             }
             m_fCallback_Ex(m_nClientHandle,nCallbackDateType,(unsigned char *)pData,nDataLen,m_pCallbackUserData);
         }
         else if(tCallbackType == ANTS_RTPClientCallbackType_Status ||
             tCallbackType == ANTS_RTPClientCallbackType_Error)
         {

             nCallbackDateType = ANTS_RTSP_DATATYPE_STATUS;
             tStatus.nStatusType = nSubType;
             if (tCallbackType == ANTS_RTPClientCallbackType_Error)
             {
                 tStatus.nStatusType = ANTS_RTSP_STATUS_Error;
             }
             tStatus.nStatusCode = nDataType;
             m_fCallback_Ex(m_nClientHandle,nCallbackDateType,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
             bCallbackProp = 1;

         }


         m_bCallbackStatus = 2;


     }
    else if (m_fCallback != NULL)
    {
        int tDateType;

        tCallbackType = (ANTS_RTPClientCallbackType)nCallbackType;

       m_bCallbackStatus = 1;
       if (tCallbackType == ANTS_RTPClientCallbackType_Stream)
       {
           m_fCallback(m_nClientHandle,tCallbackType,nSubType,nDataType,pData,nDataLen,m_pCallbackUserData);
       }
       else if(tCallbackType == ANTS_RTPClientCallbackType_Status ||
           tCallbackType == ANTS_RTPClientCallbackType_Error)
       {

           m_fCallback(m_nClientHandle,tCallbackType,nSubType,nDataType,pData,nDataLen,m_pCallbackUserData);

       }


       m_bCallbackStatus = 2;



    }
     if (tCallbackType == ANTS_RTPClientCallbackType_Status)
     {
         //RTSP_DEBUG("[%08X]nCallbackType = %d,subtype = %d,type = %d,datalen = %d\n",m_nClientHandle,nCallbackType,nSubType,nDataType,nDataLen);
     }
    // if(nDataType == AntsPktIFrames)
      //     RTSP_DEBUG("[%d]nCallbackType = %d,subtype = %d,type = %d,datalen = %d,no = %d  time = %d\n",m_nClientHandle,nCallbackType,nSubType,nDataType,nDataLen,pheader == NULL?0:pheader->uiFrameNo,pheader->uiFrameTime * 1000 + pheader->uiFrameTickCount /1000);
      if (!bCallbackProp)
      {
          if(nProp & 1)
          {
              if (pData != NULL)
              {
                  free(pData);
                  pData = NULL;
              }
          }
      }


    return 0;
}
void CRtspClient::SetCallBack(ANTS_RTSPCLIENTCALLBACK pCallback,void *pUserData)
{
    if (m_pCallbackUserData != pUserData)
    {
        m_pCallbackUserData = pUserData;
    }
    if (m_fCallback != pCallback)
    {
        m_fCallback = pCallback;
    }



}

ANTS_RTSPCLIENTCALLBACK CRtspClient::GetCallBack()
{
    return m_fCallback;
}
void CRtspClient::SetCallBackEx(ANTS_RTSPREALDATACALLBACKFXN pCallback,void *pUserData)
{
    if (m_pCallbackUserData != pUserData)
    {
        m_pCallbackUserData = pUserData;
    }
    if (m_fCallback_Ex != pCallback)
    {
        m_fCallback_Ex = pCallback;
    }


}

void CRtspClient::SetCallBackV2(ANTS_RTSPREALDATACALLBACKFXN_V2 pCallback,void *pUserData)
{
    if (m_pCallbackUserData != pUserData)
    {
        m_pCallbackUserData = pUserData;
    }
    if (m_fCallback_V2 != pCallback)
    {
        m_fCallback_V2 = pCallback;
    }



}
ANTS_RTSPREALDATACALLBACKFXN CRtspClient::GetCallBackEx()
{
    return m_fCallback_Ex;
}
void *CRtspClient::GetCallbackUserData()
{
    return m_pCallbackUserData;
}
char *CRtspClient::GetUrl()
{
    return m_pUrl;
}
int CRtspClient::GetMode()
{
    return m_nMode;
}
char *CRtspClient::GetUserName()
{
    return m_pUserName;
}
char *CRtspClient::GetPassword()
{
    return m_pPassword;
}

int CRtspClient::Open(char  *pUrl,int nMode,char *pUserName,char *pPassword)
{
    int nRet = -1;

    if (NULL != strstr(pUrl, "rtsp://") || NULL != strstr(pUrl, "RTSP://"))
    {
	    if (m_bOpen == 1 || m_bOpen == 2)
	    {
	        return -1;
	    }

#if 0
	    pUrl = "rtsp://192.168.0.153/ch01_third.264";
	    nMode = ANTS_RTPTransProto_UDP;

#endif
	   // nMode = ANTS_RTPTransProto_UDP;
	    RTSP_DEBUG("[%08X]Open url = %s [%s:%s] use %s  Ex = %d\n",m_nClientHandle,pUrl,pUserName == NULL?"nul":pUserName,pPassword == NULL?"nul":pPassword,nMode == ANTS_RTPTransProto_AUTO?"auto":(nMode == ANTS_RTPTransProto_UDP?"udp":(nMode==ANTS_RTPTransProto_TCP?"tcp":(nMode == ANTS_RTPTransProto_HTTP?"http":(nMode == ANTS_RTPTransProto_MULTICAST?"multicast":"butt")))),m_bSpecial_Ex);


	    m_bOpen = 1;
	    Ants_rtsp_GetSysRunTime(&m_tLastLiveTime,NULL);
	    //time(&m_tLastLiveTime);
	    m_nLiveCnt = 0;
			m_nIFrameLiveCnt = 0;
	    //解析IP，端口
	    nRet = Ants_Rtsp_Com_ParseUrl(pUrl,&m_tUrl);
	    if (nRet < 0)
	    {
	        Close();
	        return -1;
	    }

	    m_bHasVideo = 0;

	    m_bReplay = (m_dwProp >> 2) & 1;

	    if(m_dwProp & (7 << 3))
	    {

	        m_bSetupVideo_Req = ((m_dwProp >> 3)&1);
	        m_bSetupAudio_Req = ((m_dwProp >> 4)&1);
	        m_bSetupApp_Req = ((m_dwProp >> 5)&1);

	    }
	    else
	    {
	        m_bSetupVideo_Req = 1;
	        m_bSetupAudio_Req = 1;
	        m_bSetupApp_Req = 0;
	    }
	    m_bSetupTalk_Req = ((m_dwProp >> 6)&1);
	    if (m_bSetupTalk_Req)
	    {
	        m_bSetupAudio_Req = 1;
	    }

	    if (m_dwProp & (1 << 7))
	    {
	        m_nAutoConnect = 0;
	    }
	    if (m_dwProp & (1 << 2))
	    {
	        m_nAutoConnect = 0;
	    }
	    m_bBufferMgrMethod = 0;
	    if (m_dwProp & (1 << 8))
	    {
	        m_bBufferMgrMethod = 1;
	        m_dwProp |= (1 << 0);
	    }

	    if (m_dwProp & (1 << 9))
	    {
	        m_nAutoReConnect = 0;
	    }
	    if (m_bReplay)
	    {
	        m_nAutoReConnect = 0;
	        m_nAutoConnect = 0;
	    }
	    if (m_bSetupTalk_Req && ((m_dwProp & (7 << 3))) == 0)
	    {
	        m_nAutoReConnect = 0;
	    }


	    if (m_tUrl.pProtoType == NULL || (strcmp(m_tUrl.pProtoType,"rtsp") &&
	        strcmp(m_tUrl.pProtoType,"RTSP")) )
	    {
	        Close();
	        return -1;
	    }
	    if (m_tUrl.pAddress == NULL)
	    {
	        Close();
	        return -1;
	    }
#if 0
	    {
	        struct hostent *h;
	        h=gethostbyname(m_tUrl.pAddress);
	        if (h == NULL)
	        {
	            Close();
	            return -1;
	        }
	        int i;
	        char ipstr[17];
	        for (i = 0; (h->h_addr_list)[i] != NULL; i++)
	        {

	            RTSP_DEBUG("%s\n", inet_ntoa(*(struct in_addr  *)(h->h_addr_list)[i]));
	            RTSP_DEBUG("officail name : %s\n", h->h_name);
	            break;
	        }
	        m_nSrvIPv4 = ((struct in_addr  *)h->h_addr)->s_addr;

	        if (m_nSrvIPv4 == 0 || m_nSrvIPv4 == -1)
	        {
	            Close();
	            return -1;
	        }
	    }
#endif
	    {
	        struct addrinfo *pResult = NULL,*pNext = NULL;
	        if(getaddrinfo(m_tUrl.pAddress,NULL, NULL, &pResult ))
	        {
	            Close();
	            return -1;
	        }
	        pNext = pResult;
	        while (pNext != NULL)
	        {
	            if (pNext->ai_family == AF_INET)
	            {
	                //RTSP_DEBUG("officail name : %s\n", inet_ntoa((struct in_addr)((struct sockaddr_in *)pNext->ai_addr)->sin_addr));
	                //RTSP_DEBUG("officail name : %s\n", pNext->ai_canonname );
	                m_nSrvIPv4[0] = ((struct in_addr)((struct sockaddr_in *)pNext->ai_addr)->sin_addr).s_addr;
	                 RTSP_DEBUG("officail name : %d.%d.%d.%d\n", m_nSrvIPv4[0]&0xFF,(m_nSrvIPv4[0]>>8)&0xFF,(m_nSrvIPv4[0]>>16)&0xFF,(m_nSrvIPv4[0]>>24)&0xFF);
					 m_bSrvIPv6 = 0;
	                 break;

	            }
				else if (pNext->ai_family == AF_INET6)
				{
					memcpy(m_nSrvIPv4,&((struct sockaddr_in6 *)pNext->ai_addr)->sin6_addr,sizeof(m_nSrvIPv4));
					char szStringIPv6[128];
					inet_ntop(AF_INET6,m_nSrvIPv4,szStringIPv6,128);
					RTSP_DEBUG("officail name : %s\n", szStringIPv6);
					m_bSrvIPv6 = 1;
					break;
				}
	            pNext = pNext->ai_next;
	        }

	        freeaddrinfo(pResult);
			if(m_bSrvIPv6)
			{
				if (m_nSrvIPv4[0] == 0 && m_nSrvIPv4[1] == 0 && m_nSrvIPv4[2] == 0 && m_nSrvIPv4[3] == 0)
				{
					Close();
					return -1;
				}
			}
			else
			{
				if (m_nSrvIPv4[0] == 0 || m_nSrvIPv4[0] == -1)
				{
					Close();
					return -1;
				}
			}


	    }
	    m_nSrvPort = atoi(m_tUrl.pPort);
	    if (m_nSrvPort == 0)
	    {
	        m_nSrvPort = 554;
	    }


	    m_pUrl = Ants_strndup(pUrl,-1);
	    if (m_pUrl == NULL)
	    {
	        Close();
	        return -1;
	    }

	    {
	        int len;
	        int i;
	        len = strlen(m_pUrl);
	        for (i = len-1;i >= 0;i--)
	        {
	            if (m_pUrl[i] == '/' ||
	                m_pUrl[i] == ' ' ||
	                m_pUrl[i] == '\\'||
	                m_pUrl[i] == 0)
	            {
	                m_pUrl[i] = 0;
	            }
	            else
	            {
	                break;
	            }
	        }

	    }
	    if (m_pUrl[0] == 0)
	    {
	        Close();
	        return -1;
	    }
	    m_pRecvRawBuffer = new char[RTSPCLIENT_RECV_RAW_BUFFSIZE];
	    if (m_pRecvRawBuffer == NULL)
	    {
	        Close();
	        return -1;
	    }
	    m_nMode = nMode;
	    //分配端口资源
	    if(m_bSetupVideo_Req && nMode != RTSPCLIENT_OPEN_MODE_MULTI)
	    {
	        nRet =  CreateVideo(nMode);
	        if (nRet < 0)
	        {
	            Close();
	            return -1;
	        }
	    }
	    if (m_bSetupAudio_Req && nMode != RTSPCLIENT_OPEN_MODE_MULTI)
	    {
	        nRet =  CreateAudio(nMode);
	        if (nRet < 0)
	        {
	            Close();
	            return -1;
	        }
	    }
	    if (m_bSetupApp_Req && nMode != RTSPCLIENT_OPEN_MODE_MULTI)
	    {
	        nRet =  CreateApp(nMode);
	        if (nRet < 0)
	        {
	            Close();
	            return -1;
	        }
	    }
	    if (m_bSetupTalk_Req && nMode != RTSPCLIENT_OPEN_MODE_MULTI)
	    {
	        nRet =  CreateTalk(nMode);
	        if (nRet < 0)
	        {
	            Close();
	            return -1;
	        }
	    }


	        m_pUserName = Ants_strndup(pUserName,-1);
	        m_pPassword = Ants_strndup(pPassword,-1);

	    if (m_pUserName != NULL)
	    {
	        char strenc[128];
	        int len;
	        if (m_pPassword == NULL)
	        {
	             len = snprintf(strenc,128,"%s:",m_pUserName);
	        }
	        else
	        {
	             len = snprintf(strenc,128,"%s:%s",m_pUserName,m_pPassword);
	        }
	        m_pBase64Enc = Ants_Rtsp_Base64Encode(strenc,len);

	    }

	    m_bAudioOrder = 0;
	    m_bOpen = 2;
		m_nUrlType = 0;
    }
	else if (NULL != strstr(pUrl, "rtp://") || NULL != strstr(pUrl, "RTP://"))
	{// rtp://[serverip][:port]@[multicast_address]:port
	    char *pstart,*pstop,*pat,*pColon,testC,*pParam,*pUrlStop;
		int bPort = 0;
        m_nUrlType = 1;

		pstart = pUrl + 6;
        pstop = strchr(pstart,'?');
        pat = strchr(pstart,'@');
        pUrlStop = pUrl + strlen(pUrl);
        if (pat == NULL)
        {// 只有服务器端信息
            pColon = strchr(pstart,':');
            if (pColon == NULL)
            {
                 Close();
                return -1;
            }
           // @前
             testC = *(pColon + 1);
                if (testC >= '0' && testC <= '9' )
                {// 有port
                    m_nRtp_ServerPort = atoi(pColon + 1);
                }
                if (pstart + 1 != pColon)
                {// 有地址
                    int nLen = pColon - pstart + 1;
                    m_pRtp_ServerIP = new char [nLen];
                    if (m_pRtp_ServerIP != NULL)
                    {
                        strncpy(m_pRtp_ServerIP,pstart,nLen - 1);
                        m_pRtp_ServerIP[nLen - 1] = 0;
                    }
                }


        }
        else
        {
            if (pstop != NULL)
            {
                if (pat > pstop)
                {
                    Close();
                    return -1;
                }
            }
            // 取@前
            pColon = strchr(pstart,':');
            if (pColon < pat)
            { // @前
                if (pColon + 1 != pat)
                {// 有port
                    m_nRtp_ServerPort = atoi(pColon + 1);
                }
                if (pstart + 1 != pColon)
                {// 有地址
                    int nLen = pColon - pstart + 1;
                    m_pRtp_ServerIP = new char [nLen];
                    if (m_pRtp_ServerIP != NULL)
                    {
                        strncpy(m_pRtp_ServerIP,pstart,nLen - 1);
                        m_pRtp_ServerIP[nLen - 1] = 0;
                    }
                }
                // @ 后
                pColon = strchr(pat,':');
            }
            if(pColon != NULL)
            {
                 testC = *(pColon + 1);
                // @ 后
                if (testC >= '0' && testC <= '9' )
                {// 有port
                    m_nRtp_LocalPort = atoi(pColon + 1);
                }
                if (pat + 1 != pColon)
                {// 有地址
                    int nLen = pColon - pat;
                    m_pRtp_LocalIP = new char [nLen];
                    if (m_pRtp_LocalIP != NULL)
                    {
                        strncpy(m_pRtp_LocalIP,pat + 1,nLen - 1);
                        m_pRtp_LocalIP[nLen - 1] = 0;
                    }
                }
            }
        }
        m_nRtp_PayloadNum = 0;
        // 获取参数部分
        if (pstop != NULL)
        {

             pParam = strstr(pstop,"passivemode=");
             if (pParam!=NULL)
             {
                 testC = pParam[strlen("passivemode=")];
                 if ( testC >= '0' && testC <= '9')
                 {
                     sscanf(pParam,"passivemode=%d",&m_bRtp_PassiveMode);
                 }
             }
             pParam = strstr(pstop,"dir=");
             if (pParam!=NULL)
             {
                 testC = pParam[strlen("dir=")];
                 if ( testC >= '0' && testC <= '9')
                 {
                     sscanf(pParam,"dir=%d",&m_nRtp_Dir);
                 }
             }
             pParam = strstr(pstop,"ptype=");
             if (pParam!=NULL)
             {
                 testC = pParam[strlen("ptype=")];
                 if (testC >= '0' && testC <= '9')
                 {
                     sscanf(pParam,"ptype=%d",&m_nRtp_pType);
                 }
             }
             pParam = strstr(pstop,"rtcp=");
             if (pParam!=NULL)
             {
                 testC = pParam[strlen("rtcp=")];
                 if (testC >= '0' && testC <= '9')
                 {
                     sscanf(pParam,"rtcp=%d",&m_bRtp_RTCP);
                 }
             }
             m_nRtp_PayloadNum = 0;
             {
                 int nValue0 = -1,nValue1= -1,nValueNum = 0;
                 char *pParamStart = NULL,*pParamStop = NULL,*pParamPos = NULL;
                 pParam = strstr(pstop,"streamtype=");
                 if (pParam!=NULL)
                 {
                     pParamStart= pParam +strlen("streamtype=");
                     pParamStop = strchr(pParamStart,'&');
                     if (pParamStop == NULL)
                     {
                         pParamStop = pUrlStop;
                     }
                     pParamPos = pParamStart;
                     for (int nPayloadIdx = 0;nPayloadIdx < MAX_RTP_PAYLOAD_NUM;nPayloadIdx++)
                     {
                         testC = pParamPos[0];
                         if ((testC >= '0' && testC <= '9'))
                         {
                            //有效
                             m_nRtp_StreamType[m_nRtp_PayloadNum] = atoi(pParamPos);
                             m_nRtp_PayloadNum++;
                         }
                         else
                         {
                             break;
                         }
                         pParamPos = strchr(pParamPos,',');
                         if (pParamPos == NULL)
                         {
                             break;
                         }
                         pParamPos++;
                         if (pParamPos >= pParamStop)
                         {
                             break;
                         }

                     }

                 }
                 pParam = strstr(pstop,"payloadtype=");
                 if (pParam!=NULL)
                 {
                     pParamStart= pParam +strlen("payloadtype=");
                     pParamStop = strchr(pParamStart,'&');
                     if (pParamStop == NULL)
                     {
                         pParamStop = pUrlStop;
                     }
                     pParamPos = pParamStart;
                     for (int nPayloadIdx = 0;nPayloadIdx < m_nRtp_PayloadNum;nPayloadIdx++)
                     {
                         testC = pParamPos[0];
                         if ((testC >= '0' && testC <= '9'))
                         {
                             //有效
                             m_nRtp_PayloadType[nPayloadIdx] = atoi(pParamPos);
                         }
                         else
                         {
                             break;
                         }
                         pParamPos = strchr(pParamPos,',');
                         if (pParamPos == NULL)
                         {
                             break;
                         }
                         pParamPos++;
                         if (pParamPos >= pParamStop)
                         {
                             break;
                         }

                     }

                 }
                 pParam = strstr(pstop,"ssrc=");
                 if (pParam!=NULL)
                 {
                     pParamStart= pParam +strlen("ssrc=");
                     pParamStop = strchr(pParamStart,'&');
                     if (pParamStop == NULL)
                     {
                         pParamStop = pUrlStop;
                     }
                     pParamPos = pParamStart;
                     for (int nPayloadIdx = 0;nPayloadIdx < m_nRtp_PayloadNum;nPayloadIdx++)
                     {
                         testC = pParamPos[0];
                         if ((testC >= '0' && testC <= '9') ||
                             (testC >= 'a' && testC <= 'f'))
                         {

                             sscanf(pParamPos,"%x",&m_dwRtp_SSRC[nPayloadIdx]);

                         }
                         else
                         {
                             //break;
                         }

                         pParamPos = strchr(pParamPos,',');
                         if (pParamPos == NULL)
                         {
                             break;
                         }
                         pParamPos++;
                         if (pParamPos >= pParamStop)
                         {
                             break;
                         }

                     }

                 }
                 pParam = strstr(pstop,"lport=");
                 if (pParam!=NULL)
                 {
                     pParamStart= pParam +strlen("lport=");
                     pParamStop = strchr(pParamStart,'&');
                     if (pParamStop == NULL)
                     {
                         pParamStop = pUrlStop;
                     }
                     pParamPos = pParamStart;
                     for (int nPayloadIdx = 0;nPayloadIdx < m_nRtp_PayloadNum;nPayloadIdx++)
                     {

                         testC = pParamPos[0];
                         if ((testC >= '0' && testC <= '9'))
                         {
                             //有效
                             nValue0 = -1;
                             nValue1 = -1;
                             nValueNum = sscanf(pParamPos,"%d-%d",&nValue0,&nValue1);

                             m_nRtp_lPort[nPayloadIdx][0] = nValue0;
                             m_nRtp_lPort[nPayloadIdx][1] = nValue1;
                         }
                         else
                         {
                             //break;
                         }
                         pParamPos = strchr(pParamPos,',');
                         if (pParamPos == NULL)
                         {
                             break;
                         }
                         pParamPos++;
                         if (pParamPos >= pParamStop)
                         {
                             break;
                         }

                     }

                 }
                 pParam = strstr(pstop,"rport=");
                 if (pParam!=NULL)
                 {
                     pParamStart= pParam +strlen("rport=");
                     pParamStop = strchr(pParamStart,'&');
                     if (pParamStop == NULL)
                     {
                         pParamStop = pUrlStop;
                     }
                     pParamPos = pParamStart;
                     for (int nPayloadIdx = 0;nPayloadIdx < m_nRtp_PayloadNum;nPayloadIdx++)
                     {

                         testC = pParamPos[0];
                         if ((testC >= '0' && testC <= '9'))
                         {
                             //有效
                             nValue0 = -1;
                             nValue1 = -1;
                             nValueNum = sscanf(pParamPos,"%d-%d",&nValue0,&nValue1);

                             m_nRtp_rPort[nPayloadIdx][0] = nValue0;
                             m_nRtp_rPort[nPayloadIdx][1] = nValue1;
                         }
                         else
                         {
                             //break;
                         }
                         pParamPos = strchr(pParamPos,',');
                         if (pParamPos == NULL)
                         {
                             break;
                         }
                         pParamPos++;
                         if (pParamPos >= pParamStop)
                         {
                             break;
                         }

                     }

                 }
                 pParam = strstr(pstop,"sock_fds=");
                 if (pParam!=NULL)
                 {
                     pParamStart= pParam +strlen("sock_fds=");
                     pParamStop = strchr(pParamStart,'&');
                     if (pParamStop == NULL)
                     {
                         pParamStop = pUrlStop;
                     }
                     pParamPos = pParamStart;
                     for (int nPayloadIdx = 0;nPayloadIdx < m_nRtp_PayloadNum;nPayloadIdx++)
                     {

                         testC = pParamPos[0];
                         if ((testC >= '0' && testC <= '9'))
                         {
                             //有效
                             nValue0 = -1;
                             nValue1 = -1;
                             nValueNum = sscanf(pParamPos,"%d-%d",&nValue0,&nValue1);

                             m_nRtp_SockFD[nPayloadIdx][0] = nValue0;
                             m_nRtp_SockFD[nPayloadIdx][1] = nValue1;
                             m_bRtp_extSockFD = 1;
                         }
                         else
                         {
                             //break;
                         }
                         pParamPos = strchr(pParamPos,',');
                         if (pParamPos == NULL)
                         {
                             break;
                         }
                         pParamPos++;
                         if (pParamPos >= pParamStop)
                         {
                             break;
                         }

                     }

                 }

             }

        }
        // 检查参数合法性
        if (m_nRtp_PayloadNum == 0 &&
            m_pRtp_LocalIP == NULL &&
            m_nRtp_LocalPort != -1)
        {
			if (m_nRtp_pType == 1)
			{
		        m_pRecvRawBuffer = new char[RTSPCLIENT_RECV_RAW_BUFFSIZE];
		        if (m_pRecvRawBuffer == NULL)
		        {
		            Close();
		            return -1;
		        }
			}

            nRet = CreateRtpVideo(m_nRtp_pType, m_nRtp_LocalPort);
            if (nRet < 0)
            {
                Close();
                return -1;
            }
			if(m_nRtp_LocalPort == 0 && m_nRtp_pType != 1)
			{
				nRet = CreateRtpSecondVideo(m_nRtp_pType, m_nRtp_LocalPort);
				if (nRet < 0)
				{
					Close();
					return -1;
				}
			}


			m_bRtp_DollarFlag = 1;

        }
        else if (m_bRtp_PassiveMode)
        {// 被动，需要本地监听,TCP
            if (m_nRtp_pType != 1)
            {// not tcp
                Close();
                return -1;
            }
            if (!m_bRtp_extSockFD)
            {
                Close();
                return -1;
            }
            if (m_nRtp_SockFD[0][0] < 0 || m_nRtp_PayloadNum == 0)
            {
                Close();
                return -1;
            }
            // 开始监听
           for (int nPIdx = 0;nPIdx < m_nRtp_PayloadNum;nPIdx++)
           {
               char *pPlayloadName = NULL;
               int nClockRate;
               m_pRtp_Session[nPIdx]=new CH264RtpSession;
               if (m_pRtp_Session[nPIdx] != NULL)
               {
                   m_pRtp_Session[nPIdx]->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,m_pRtp_Session[nPIdx]);
				   m_pRtp_Session[nPIdx]->SetProp(m_dwProp);
                   if(m_pRtp_Session[nPIdx]->CreateBySocket(m_nRtp_SockFD[0][0],m_nRtp_lPort[nPIdx][0],m_nRtp_lPort[nPIdx][1],1,m_bRtp_RTCP,rtprtcp_datapacketcallback,this))
                   {
                       Close();
                       return -1;
                   }
                   m_pRtp_Session[nPIdx]->SetSSRC(m_dwRtp_SSRC[nPIdx]);
                   if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_H264)
                   {
                       pPlayloadName="H264";
                       nClockRate = 90000;
                   }
                   else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_H265)
                   {
                       pPlayloadName="H265";
                       nClockRate = 90000;
                   }
                   else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_JPEG)
                   {
                       pPlayloadName="JPEG";
                       nClockRate = 90000;
                   }
                   else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_PS)
                   {
                       pPlayloadName="PS";
                       nClockRate = 90000;
                   }
                   else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_APP)
                   {
                       pPlayloadName="application";
                       nClockRate = 90000;
                   }
                   else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_G711A)
                   {
                       pPlayloadName="PCMA";
                       nClockRate = 8000;
                   }
                   else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_G711U)
                   {
                       pPlayloadName="PCMU";
                       nClockRate = 8000;
                   }
                   m_pRtp_Session[nPIdx]->SetPayloadStreamType(m_nRtp_StreamType[nPIdx],m_nRtp_PayloadType[nPIdx],nClockRate,pPlayloadName);
				   m_pRtp_Session[nPIdx]->SetDefaultPayloadType(m_nRtp_PayloadType[nPIdx]);
               }
           }


        }
        else if(m_bRtp_PassiveMode == 0)
        {
            if (m_nRtp_SockFD[0][0] > 0 && m_nRtp_PayloadNum > 0)
            {
                for (int nPIdx = 0;nPIdx < m_nRtp_PayloadNum;nPIdx++)
                {
                    char *pPlayloadName = NULL;
                    int nClockRate;
                    m_pRtp_Session[nPIdx]=new CH264RtpSession;
                    if (m_pRtp_Session[nPIdx] != NULL)
                    {
                        m_pRtp_Session[nPIdx]->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,m_pRtp_Session[nPIdx]);
						m_pRtp_Session[nPIdx]->SetProp(m_dwProp);
                        if(m_pRtp_Session[nPIdx]->CreateBySocket(m_nRtp_SockFD[0][0],m_nRtp_pType == 1?m_nRtp_lPort[nPIdx][0]:m_nRtp_SockFD[nPIdx][0],m_nRtp_pType == 1?m_nRtp_lPort[nPIdx][1]:m_nRtp_SockFD[nPIdx][1],m_nRtp_pType == 1,m_bRtp_RTCP,rtprtcp_datapacketcallback,this))
                        {
                            Close();
                            return -1;
                        }
                        m_pRtp_Session[nPIdx]->SetSSRC(m_dwRtp_SSRC[nPIdx]);
                        if (m_pRtp_ServerIP != NULL)
                        {
                            m_pRtp_Session[nPIdx]->AddDestination(m_pRtp_ServerIP,m_nRtp_rPort[nPIdx][0],m_nRtp_rPort[nPIdx][1],0,0,0,this);
                        }
                        if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_H264)
                        {
                            pPlayloadName="H264";
                            nClockRate = 90000;
                        }
                        else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_H265)
                        {
                            pPlayloadName="H265";
                            nClockRate = 90000;
                        }
                        else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_JPEG)
                        {
                            pPlayloadName="JPEG";
                            nClockRate = 90000;
                        }
                        else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_PS)
                        {
                            pPlayloadName="PS";
                            nClockRate = 90000;
                        }
                        else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_APP)
                        {
                            pPlayloadName="application";
                            nClockRate = 90000;
                        }
                        else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_G711A)
                        {
                            pPlayloadName="PCMA";
                            nClockRate = 8000;
                        }
                        else if (m_nRtp_StreamType[nPIdx] == ANTS_RTSP_STREAM_G711U)
                        {
                            pPlayloadName="PCMU";
                            nClockRate = 8000;
                        }
                        m_pRtp_Session[nPIdx]->SetPayloadStreamType(m_nRtp_StreamType[nPIdx],m_nRtp_PayloadType[nPIdx],nClockRate,pPlayloadName);
						m_pRtp_Session[nPIdx]->SetDefaultPayloadType(m_nRtp_PayloadType[nPIdx]);
                    }
                }
            }
            // 主动方式
            else if (m_nRtp_pType == 0)
            {// udp
                Close();
                return -1;
                if (m_nRtp_Dir == 0 || m_nRtp_Dir == 2)
                {// 接收
                }
            }
            else if (m_nRtp_pType == 1)
            {// tcp
                Close();
                return -1;
            }
            else
            {
                Close();
                return -1;
            }
        }
        else
        {
            Close();
            return -1;
        }
	}
    else
    {
        return -1;
    }

    RTSP_DEBUG("[%08X]Open Succ\n",m_nClientHandle);

    return 0;
}

int CRtspClient::CreateRTP()
{
    int i;
    for (i = 0; i < m_nRtp_PayloadNum;i++)
    {
      //  m_pRtp_Session[i] = new CH264RtpSession;
//        m_pRtp_Session[i]->Create();
    }
    return 0;
}

void CRtspClient::StartThread()
{
    if (m_bRtp_extSockFD)
    {
        return;
    }
    Start();
}
void CRtspClient::Connect()
{
    int nRet;
    int TmpSocket = -1;
    struct timeval tv_out;
    m_nOPLock.Lock();
    if (m_nSocket != -1)
    {
        m_nOPLock.Unlock();
        return;
    }
    if (m_bSrvIPv6)
    {
		m_nSocket = socket(AF_INET6, SOCK_STREAM, 0);
    }
	else
	{
		m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
	}
    if (m_nSocket == -1)
    {
        m_nOPLock.Unlock();
        return ;
    }
#if 0
    int size = 1024 * 1024;
    if (setsockopt(m_nSocket,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
    {

    }
#endif
#ifdef USE_ONE_THREAD

    if(m_nMode != ANTS_RTPTransProto_TCP && m_nMode != ANTS_RTPTransProto_HTTP)
    {

#if defined(__WIN32__) || defined(_WIN32)
        unsigned long arg = 1;
        nRet =  ioctlsocket(m_nSocket, FIONBIO, &arg);
        if (nRet)
        {
            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return ;
        }
        tcp_keepalive tkeepalive;
        DWORD dwBytesReturned;

        tkeepalive.onoff = 1;
        tkeepalive.keepalivetime = 60000;
        tkeepalive.keepaliveinterval = 15;
        if(WSAIoctl(m_nSocket,SIO_KEEPALIVE_VALS,&tkeepalive,sizeof(tkeepalive),NULL,0,&dwBytesReturned,NULL,NULL))
        {
            RTSP_DEBUG("WSAIoctl failed\n");
        }

#else


        int curFlags = fcntl(m_nSocket, F_GETFL, 0);
        nRet =  fcntl(m_nSocket, F_SETFL, curFlags|O_NONBLOCK);
        if (nRet < 0 )
        {
            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return ;
        }

        int keepalive = 1; // 开启keepalive属性
        int keepidle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
        int keepinterval = 15; // 探测时发包的时间间隔为5 秒
        int keepcount = 4; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
        setsockopt(m_nSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));


#endif

    }
    else
    {
#ifdef NO_BLOCK_OP
#if defined(__WIN32__) || defined(_WIN32)
        unsigned long arg = 1;
        nRet =  ioctlsocket(m_nSocket, FIONBIO, &arg);
        if (nRet)
        {
            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return ;
        }
        tcp_keepalive tkeepalive;
        DWORD dwBytesReturned;

        tkeepalive.onoff = 1;
        tkeepalive.keepalivetime = 5000;
        tkeepalive.keepaliveinterval = 2;
        if(WSAIoctl(m_nSocket,SIO_KEEPALIVE_VALS,&tkeepalive,sizeof(tkeepalive),NULL,0,&dwBytesReturned,NULL,NULL))
        {
            RTSP_DEBUG("WSAIoctl failed\n");
        }

#else


        int curFlags = fcntl(m_nSocket, F_GETFL, 0);
        nRet =  fcntl(m_nSocket, F_SETFL, curFlags|O_NONBLOCK);
        if (nRet < 0 )
        {
            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return ;
        }
        int keepalive = 1; // 开启keepalive属性
        int keepidle = 5; // 如该连接在60秒内没有任何数据往来,则进行探测
        int keepinterval = 2; // 探测时发包的时间间隔为5 秒
        int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
        setsockopt(m_nSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));


#endif
#endif
    }

    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
    nRet = setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv_out, sizeof(tv_out));
    if (nRet < 0 )
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return ;
    }

    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
    nRet = setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv_out, sizeof(tv_out));
    if (nRet < 0 )
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return ;
    }

    int size = 65535;//m_nRcvBuffSize;
    if (!m_bBufferMgrMethod)
    {
        size = m_nRcvBuffSize;
    }

    if (setsockopt(m_nSocket,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return ;
    }
#if 0
    int nLen = sizeof(int );
    size = 0;
    if (getsockopt(m_nSocket,SOL_SOCKET,SO_RCVBUF,( char *)&size,&nLen) != 0)
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return ;
    }
    m_nRcvBuffSize = size;
#endif


#endif
#ifndef WIN32
    int nPhyIdx = (m_dwProp >> 24)&0xFF;
    if(nPhyIdx > 0)
    {
        // 绑定网卡
        struct ifreq  ifr;
        sprintf(ifr.ifr_ifrn.ifrn_name,"eth%d", nPhyIdx - 1);
		RTSP_DEBUG("[%08x]bind phy  phyidx = %d! \n",m_nClientHandle,nPhyIdx);

        if (setsockopt(m_nSocket, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
        {
            RTSP_DEBUG("SO_BINDTODEVICE failed phyidx = %d! [%s]\n",nPhyIdx,strerror(errno));

            /* Deal with error... */
        }

    }
#endif

    TmpSocket = m_nSocket;


   m_bConnected = 1;
   m_hConnectStatusLock.Lock();
   m_bConnecting = 1;
   m_hConnectStatusLock.Unlock();
   struct sockaddr *pAddr = NULL;
   struct sockaddr_in addr;
   struct sockaddr_in6 addrV6;
   int addrLen = sizeof(addr);
   if(m_bSrvIPv6)
   {
	   memset(&addrV6,0,sizeof(addrV6));
	   addrV6.sin6_family = AF_INET6;
	   memcpy(&addrV6.sin6_addr, m_nSrvIPv4,sizeof(m_nSrvIPv4));
	   addrV6.sin6_port = htons(m_nSrvPort);
	   #ifndef WIN32
	   char szEthName[256];
	   int nPhyIdx = (m_dwProp >> 24)&0xFF;
	    if(nPhyIdx > 0)
		{
			sprintf(szEthName,"eth%d",nPhyIdx - 1);

		}
		else
		{
		 sprintf(szEthName,"eth%d",0);
		}
		addrV6.sin6_scope_id = if_nametoindex(szEthName);//2;
		#endif

	   pAddr = (sockaddr *)&addrV6;
	   addrLen = sizeof(addrV6);
   }
   else
   {
		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = m_nSrvIPv4[0];
		addr.sin_port = htons(m_nSrvPort);
		pAddr = (sockaddr *)&addr;
   }
    RTSP_DEBUG("[%08x]Start connect,waiting...\n",m_nClientHandle);
    if (connect(TmpSocket,(struct sockaddr *)pAddr,addrLen)!= 0)
    {
        int err = GetLastError();
        if(err == EINPROGRESS || err == EWOULDBLOCK)
        {

        }
        else
        {
            m_hConnectStatusLock.Lock();
            m_bConnecting = 0;
            m_hConnectStatusLock.Unlock();
            m_bConnected = 0;
            RTSP_ERROR("[%x]connect error %d,%s\n",m_nClientHandle,err,strerror(err));

            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return ;
        }


    }
    else
    {
        m_hConnectStatusLock.Lock();
        m_bConnecting = 0;
        m_hConnectStatusLock.Unlock();
        RTSP_DEBUG("[%08x]Connected %d\n",m_nClientHandle,m_nSocket);

        Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Connected,0,0,NULL,0);


        m_bConnected = 2;
    }

     m_nOPLock.Unlock();

}
int CRtspClient::Close()
{
    CMessage *p;
    AntsRtspClient_Req_List_T *pReq;
    int timeCnt = 10;
    RTSP_DEBUG("[%08X]Closing url = %s\n",m_nClientHandle,m_pUrl == NULL?"nul":m_pUrl);

    NeedClose(ANTS_RTSP_ERROR_NoError);
    RTSP_DEBUG("[%08X]stop thread\n",m_nClientHandle);
    while(IsRunning())
    {
        Ants_WaitTime(0,100000);
        //RTPTime::Wait(RTPTime(0,100000));
        timeCnt--;
        if (timeCnt <= 0)
        {

          //  break;
        }
            RTSP_DEBUG("[%08X]wait callback = %d return, cnt = %d\n",m_nClientHandle,m_bCallbackStatus,timeCnt);
    }
    RTSP_DEBUG("[%08X]stop thread ok\n",m_nClientHandle);
  //  Kill();
    timeCnt = 1000;
    m_nOPLock.Lock();
    RTSP_DEBUG("[%08X]close socket[%d]\n",m_nClientHandle,m_nSocket);
	if (m_nSocket != m_nRtp_TcpSockFD)
	{
		struct linger stl;
		stl.l_onoff=1;
		stl.l_linger=0;
		setsockopt(m_nRtp_TcpSockFD,SOL_SOCKET,SO_LINGER,(char *)&stl,sizeof(stl));

		closeSocket(m_nRtp_TcpSockFD);
	}
	m_nRtp_TcpSockFD = -1;
    if (m_nSocket != -1)
    {
        struct linger stl;
        stl.l_onoff=1;
        stl.l_linger=0;
        setsockopt(m_nSocket,SOL_SOCKET,SO_LINGER,(char *)&stl,sizeof(stl));

        closeSocket(m_nSocket);
        m_nSocket = -1;
    }
    RTSP_DEBUG("[%08X]close socket[%d] ok\n",m_nClientHandle,m_nSocket);
    RTSP_DEBUG("[%08X]destroy video[%x]\n",m_nClientHandle,m_pVideoSess);
    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->Destroy();
        delete m_pVideoSess;
        m_pVideoSess =NULL;
    }
    RTSP_DEBUG("[%08X]destroy video[%x] ok\n",m_nClientHandle,m_pVideoSess);
	if (m_pSecondVideoSess != NULL)
	{
		m_pSecondVideoSess->Destroy();
		delete m_pSecondVideoSess;
		m_pSecondVideoSess =NULL;
	}
    RTSP_DEBUG("[%08X]destroy audio[%x]\n",m_nClientHandle,m_pAudioSess);
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->Destroy();
        delete m_pAudioSess;
        m_pAudioSess = NULL;
    }
    RTSP_DEBUG("[%08X]destroy audio[%x] ok\n",m_nClientHandle,m_pAudioSess);
    if (m_pAppSess != NULL)
    {
        m_pAppSess->Destroy();
        delete m_pAppSess;
        m_pAppSess = NULL;
    }
    if (m_pTalkSess != NULL)
    {
        m_pTalkSess->Destroy();
        delete m_pTalkSess;
        m_pTalkSess = NULL;
    }
    Ants_Rtsp_Com_ReleaseUrl(&m_tUrl);

    Ants_strFree(m_pUrl);
    m_pUrl = NULL;
    if (m_pDollarBuffer != NULL)
    {
        delete [] m_pDollarBuffer;
        m_pDollarBuffer = NULL;
    }
    while(m_pMsg != NULL)
    {
        p = m_pMsg;
        m_pMsg = m_pMsg->m_pNext;
        delete p;
    }
    if (m_pRecvRawBuffer != NULL)
    {
        delete []m_pRecvRawBuffer;
        m_pRecvRawBuffer = NULL;
    }
    m_pMsg = NULL;
    m_bVideoExist = 0;
    m_bAudioExist = 0;
    m_nTimeOut = 0;
    m_bDollar = 0;
        Ants_strFree(m_pVideo_control);
        m_pVideo_control = NULL;
        Ants_strFree(m_pAudio_control);
        m_pAudio_control = NULL;
        Ants_strFree(m_pVideo_rtpmap);
        m_pVideo_rtpmap = NULL;
        Ants_strFree(m_pAudio_rtpmap);
        m_pAudio_rtpmap = NULL;
        Ants_strFree(m_pUserName);
        m_pUserName = NULL;
        Ants_strFree(m_pPassword);
        m_pPassword = NULL;
        Ants_strFree(m_pBase64Enc);
        m_pBase64Enc = NULL;

        Ants_strFree(m_pApp_control);
        m_pApp_control = NULL;
        Ants_strFree(m_pTalk_control);
        m_pTalk_control = NULL;
        Ants_strFree(m_pApp_rtpmap);
        m_pApp_rtpmap = NULL;
        Ants_strFree(m_pTalk_rtpmap);
        m_pTalk_rtpmap = NULL;

        m_bDigest = 0;
        Ants_strFree(m_pRealm);
        m_pRealm = NULL;
        Ants_strFree(m_pNonce);
        m_pNonce = NULL;
        Ants_strFree(m_strSessionID);
        m_strSessionID = NULL;
        Ants_strFree(m_pBaseUrl);
        m_pBaseUrl = NULL;

        Ants_strFree(m_psprop_parameter_sets_sps);
        m_psprop_parameter_sets_sps = NULL;
        Ants_strFree(m_psprop_parameter_sets_pps);
        m_psprop_parameter_sets_pps = NULL;

        Ants_strFree(m_pTstConfig);
        m_pTstConfig = NULL;

        Ants_strFree(m_pMulticastAddr[0]);
        m_pMulticastAddr[0] = NULL;
        Ants_strFree(m_pMulticastAddr[1]);
        m_pMulticastAddr[1] = NULL;
        Ants_strFree(m_pMulticastAddr[2]);
        m_pMulticastAddr[2] = NULL;

		m_nMulticastPort[0] = 0;
		m_nMulticastPort[1] = 0;
		m_nMulticastPort[2] = 0;


    m_nMode = 0;
	memset(&m_nSrvIPv4,0,sizeof(m_nSrvIPv4));
    m_nSrvPort = 0;
    m_nSeq = 0;
    m_bRtspCap = 0;
    m_nRecvBuffPos = 0;
    m_nSendBuffPos = 0;
    m_nSeq = 0;
    m_nCurrOP = 0;
    m_bRtspCap = 0;
    m_bNeedClose = 0;
    m_bDollar = 0;
    m_nDollarPos = 0;
    m_nDollarLen = 0;
    m_nDollarPos0 = 0;
    m_nWaitOPRes = 0;
    m_bVideoExist = 0;
    m_bAudioExist = 0;
    m_nVideoTransPort = 1234;
    m_nAudioTransPort = 3456;
    m_nVideoSrvTransPort = 0;
    m_nAudioSrvTransPort = 0;
    m_nVideoPayloadClockRate = 90000;
    m_nAudioPayloadClockRate = 8000;

    m_nTimeOut = 60;
    m_tLastSetParameter = 0;
    m_bSet_Parameter = 0;
    m_bGet_Parameter = 0;
    m_bPlaying = 0;
	memset(m_nLocalIPv4,0,sizeof(m_nLocalIPv4));
    m_nLocalPort = 0;
    m_bConnected = 0;
    m_bCallbackStatus = 0;
    m_nReason = 0;
#ifdef NO_BLOCK_OP
    m_pRecvRawBuffer = NULL;
    m_bHasContext = 0;
    m_nRecvRawUsePos= 0;
    m_nRecvRawBufferPos = 0;
#endif

    m_nTalkPayloadType = -1;
    m_nTalkPayloadClockRate = 0;
    m_nTalkStreamType = -1;
    m_nAppPayloadType = -1;
    m_nAppPayloadClockRate = 0;
    m_nAppStreamType = -1;

    m_nVideoPayloadType = -1;
    m_nAudioPayloadType = -1;
    m_nVideoPayloadClockRate = 0;
    m_nAudioPayloadClockRate = 0;
    m_nVideoStreamType = ANTS_RTSP_STREAM_H264;
    m_nAudioStreamType = ANTS_RTSP_STREAM_G711U;

    m_bOpen = 3;
    m_bReady = 2;
    m_bDigest = 0;
	m_bNeedAuth = 0;
    m_bSpecial_Ex = 0;
    m_bIFrameNeed = 1;
    m_nLastFrameNum = 0;
    m_bFirstSetup = 0;
    m_bStep = 0;
    m_dwStepCnt = 0;
    m_dwStepCnt_Req = 0;
    m_bStep_Req = 0;
    m_bIntraPause = 0;
    m_hReqListLock.Lock();
    while(m_pReqList!= NULL)
    {
         pReq = m_pReqList;
        m_pReqList = m_pReqList->pNext;
        delete pReq;
    }

    m_pReqList = NULL;
    m_pReqList_tail = NULL;
    m_nReqListCnt = 0;
    m_hReqListLock.Unlock();
    memset(m_nCombChanCurr,0x80,sizeof(m_nCombChanCurr));
    m_nCombChanCurrCnt = 0;
    //m_nCombChanAddReqCnt = 0;
    m_nCombChanDecReqCnt = 0;
	m_nCombChanDecDoReqCnt = 0;
	m_nCombChanAddDoReqCnt = 0;

    if (m_pRtp_ServerIP != NULL)
    {
        delete []m_pRtp_ServerIP;
        m_pRtp_ServerIP = NULL;
    }
    if (m_pRtp_LocalIP != NULL)
    {
        delete []m_pRtp_LocalIP;
        m_pRtp_LocalIP = NULL;
    }
    for (int i = 0; i < MAX_RTP_PAYLOAD_NUM;i++)
    {
        if (m_pRtp_Session[i] != NULL)
        {
            delete m_pRtp_Session[i] ;
            m_pRtp_Session[i] = NULL;
        }
        m_nRtp_rPort[i][0] = -1;
        m_nRtp_rPort[i][1] = -1;
        m_nRtp_lPort[i][0] = -1;
        m_nRtp_lPort[i][1] = -1;
        m_nRtp_StreamType[i] = -1;
        m_nRtp_PayloadType[i] = -1;

    }
    m_bRtp_PassiveMode = 0;
    m_nRtp_PayloadNum = 0;
    m_nRtp_Dir = 0;
    m_nOPLock.Unlock();
    RTSP_DEBUG("[%08X]Closed Stop OK\n",m_nClientHandle);
    return 0;
}

int CRtspClient::CreateRtpSocketTcp(int nPort)
{
    int nRet;
    int TmpSocket = -1;
    struct timeval tv_out;
    struct sockaddr_in addr;

    m_nOPLock.Lock();
    if (m_nSocket != -1)
    {
        m_nOPLock.Unlock();
        return -1;
    }

    m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nSocket == -1)
    {
        m_nOPLock.Unlock();
        return -1;
    }
#if 0
    int size = 1024 * 1024;
    if (setsockopt(m_nSocket,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
    {

    }
#endif
#ifdef USE_ONE_THREAD

    {
#ifdef NO_BLOCK_OP
#if defined(__WIN32__) || defined(_WIN32)
        unsigned long arg = 1;
        nRet =  ioctlsocket(m_nSocket, FIONBIO, &arg);
        if (nRet)
        {
            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return -1;
        }
        tcp_keepalive tkeepalive;
        DWORD dwBytesReturned;

        tkeepalive.onoff = 1;
        tkeepalive.keepalivetime = 5000;
        tkeepalive.keepaliveinterval = 2;
        if(WSAIoctl(m_nSocket,SIO_KEEPALIVE_VALS,&tkeepalive,sizeof(tkeepalive),NULL,0,&dwBytesReturned,NULL,NULL))
        {
            RTSP_DEBUG("WSAIoctl failed\n");
        }

#else


        int curFlags = fcntl(m_nSocket, F_GETFL, 0);
        nRet =  fcntl(m_nSocket, F_SETFL, curFlags|O_NONBLOCK);
        if (nRet < 0 )
        {
            closeSocket(m_nSocket);
            m_nSocket = -1;
            m_nOPLock.Unlock();
            return -1;
        }
        int keepalive = 1; // 开启keepalive属性
        int keepidle = 5; // 如该连接在60秒内没有任何数据往来,则进行探测
        int keepinterval = 2; // 探测时发包的时间间隔为5 秒
        int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
        setsockopt(m_nSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
        setsockopt(m_nSocket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));


#endif
#endif
    }

    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
    nRet = setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv_out, sizeof(tv_out));
    if (nRet < 0 )
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return -1;
    }

    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
    nRet = setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv_out, sizeof(tv_out));
    if (nRet < 0 )
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return -1;
    }

    int size = 65535;//m_nRcvBuffSize;
    if (!m_bBufferMgrMethod)
    {
        size = m_nRcvBuffSize;
    }

    if (setsockopt(m_nSocket,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return -1;
    }
#if 0
    int nLen = sizeof(int );
    size = 0;
    if (getsockopt(m_nSocket,SOL_SOCKET,SO_RCVBUF,( char *)&size,&nLen) != 0)
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return ;
    }
    m_nRcvBuffSize = size;
#endif


#endif
#ifndef WIN32
    int nPhyIdx = (m_dwProp >> 24)&0xFF;
    if(nPhyIdx > 0)
    {
        // 绑定网卡
        struct ifreq  ifr;
        sprintf(ifr.ifr_ifrn.ifrn_name,"eth%d", nPhyIdx - 1);
        if (setsockopt(m_nSocket, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
        {
            RTSP_DEBUG("SO_BINDTODEVICE failed phyidx = %d! [%s]\n",nPhyIdx,strerror(errno));

            /* Deal with error... */
        }

    }
#endif

    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nPort);
    addr.sin_addr.s_addr = htonl(0);
    if (bind(m_nSocket,(struct sockaddr *)&addr,sizeof(struct sockaddr_in)) != 0)
    {
        closeSocket(m_nSocket);
        m_nSocket = -1;
        m_nOPLock.Unlock();
        return -1;
    }

    m_nOPLock.Unlock();

	return 0;
}

int CRtspClient::CreateRtpSecondVideo(int nmode, int nPort)
{
	uint16_t portbase = 0,destport;
	int status = -1,i,num;
	CH264RtpSession *sess;


	if (m_pSecondVideoSess != NULL)
	{
		m_pSecondVideoSess->Destroy();
		delete m_pSecondVideoSess;
		m_pSecondVideoSess =NULL;
	}

	sess = new CH264RtpSession;
	if (sess == NULL)
	{
		return -1;
	}
	sess->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,sess);
	sess->SetSpecialFlag(m_bSpecial_Ex);
	sess->SetProp(m_dwProp);

	if (nmode == 1)
	{
		if (portbase == 0)
		{
			for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
			{
				portbase -= portbase & 1;
				if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
				{
					break;
				}
				if (portbase == 0)
				{
					continue;
				}


				sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
				status = CreateRtpSocketTcp(portbase);
				if (!status)
				{
					m_nSecondVideoTransPort = portbase;
					break;
				}
				else
				{
					// RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
				}

			}
		}
		else
		{
			sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
			status = CreateRtpSocketTcp(portbase);
			if (!status)
			{
				m_nSecondVideoTransPort = portbase;
			}
			else
			{
				//  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
			}
		}

		m_nSecondVideoTransPort = portbase;
	}
	else
	{
		portbase = nPort;

		if (portbase == 0)
		{
			for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
			{
				portbase -= portbase & 1;
				if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
				{
					break;
				}
				if (portbase == 0)
				{
					continue;
				}


				status = sess->CreateClient(portbase,0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
				if (!status)
				{
					m_nSecondVideoTransPort = portbase;
					break;
				}
				else
				{
					// RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
				}

			}
		}
		else
		{
			status = sess->CreateClient(portbase,nmode == RTSPCLIENT_OPEN_MODE_MULTI?2:0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
			if (!status)
			{
				m_nSecondVideoTransPort = portbase;
			}
			else
			{
				//  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
			}
		}
	}

	if (status)
	{
		delete sess;
		return -1;
	}

	//sess->SetDefaultPayloadType(m_nVideoPayloadType);
	//sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(nPayloadType)/1000);
	//sess->SetDefaultMark(false);
	m_pSecondVideoSess = sess;
	return 0;
}

int CRtspClient::CreateRtpVideo(int nmode, int nPort)
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CH264RtpSession *sess;


    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->Destroy();
        delete m_pVideoSess;
        m_pVideoSess =NULL;
    }

    sess = new CH264RtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,sess);
    sess->SetSpecialFlag(m_bSpecial_Ex);
    sess->SetProp(m_dwProp);

    if (nmode == 1)
    {
        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
				status = CreateRtpSocketTcp(portbase);
                if (!status)
                {
                    m_nVideoTransPort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
            sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
			status = CreateRtpSocketTcp(portbase);
            if (!status)
            {
                m_nVideoTransPort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }

        m_nVideoTransPort = portbase;
    }
    else
    {
        portbase = nPort;

        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->CreateClient(portbase,0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
                if (!status)
                {
                    m_nVideoTransPort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
            status = sess->CreateClient(portbase,nmode == RTSPCLIENT_OPEN_MODE_MULTI?2:0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
            if (!status)
            {
                m_nVideoTransPort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }
    }

    if (status)
    {
        delete sess;
        return -1;
    }

    //sess->SetDefaultPayloadType(m_nVideoPayloadType);
    //sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(nPayloadType)/1000);
    //sess->SetDefaultMark(false);
    m_pVideoSess = sess;
    return 0;
}

int CRtspClient::CreateVideo(int nmode, int nPort)
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CH264RtpSession *sess;


    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->Destroy();
        delete m_pVideoSess;
        m_pVideoSess =NULL;
    }

    sess = new CH264RtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,sess);
    sess->SetSpecialFlag(m_bSpecial_Ex);
    sess->SetProp(m_dwProp);
    if (nmode == RTSPCLIENT_OPEN_MODE_TCP ||
        nmode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

         status = sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);

        m_nVideoTransPort = 0;
    }
    else
    {
        if (nmode == RTSPCLIENT_OPEN_MODE_MULTI)
        {
            portbase = m_nMulticastPort[0];
        }
		else
		{
            portbase = nPort;
		}

        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;;portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->CreateClient(portbase,0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
                if (!status)
                {
                    m_nVideoTransPort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
			unsigned int dwipv4[4]={0,0,0,0};
			if (m_bSrvIPv6)
			{
				inet_pton(AF_INET6,m_pMulticastAddr[1],dwipv4);
			}
			else
			{
				inet_pton(AF_INET,m_pMulticastAddr[1],dwipv4);
			}
            status = sess->CreateClient(portbase,nmode == RTSPCLIENT_OPEN_MODE_MULTI?2:0,rtprtcp_datapacketcallback,this,nmode == RTSPCLIENT_OPEN_MODE_MULTI?dwipv4:NULL,m_bSrvIPv6);
            if (!status)
            {
                m_nVideoTransPort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }
    }

    if (status)
    {
        delete sess;
        return -1;
    }

    //sess->SetDefaultPayloadType(m_nVideoPayloadType);
    //sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(nPayloadType)/1000);
    //sess->SetDefaultMark(false);
    m_pVideoSess = sess;
    return 0;
}
int CRtspClient::CreateAudio(int nmode)
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CAudioRtpSession *sess;

    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->Destroy();
        delete m_pAudioSess;
        m_pAudioSess = NULL;
    }
    sess = new CAudioRtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,sess);
    sess->SetProp(m_dwProp);
    if (nmode == RTSPCLIENT_OPEN_MODE_TCP ||
        nmode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

        status = sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
        m_nAudioTransPort = 2;
    }
    else
    {
        if (nmode == RTSPCLIENT_OPEN_MODE_MULTI)
        {
            portbase = m_nMulticastPort[1];
        }
        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;;portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->CreateClient(portbase,0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
                if (!status)
                {
                    m_nAudioTransPort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
			unsigned int dwipv4[4]={0,0,0,0};
			if (m_bSrvIPv6)
			{
				inet_pton(AF_INET6,m_pMulticastAddr[0],dwipv4);
			}
			else
			{
				 inet_pton(AF_INET,m_pMulticastAddr[0],dwipv4);
			}
            status = sess->CreateClient(portbase,nmode == RTSPCLIENT_OPEN_MODE_MULTI?2:0,rtprtcp_datapacketcallback,this,nmode == RTSPCLIENT_OPEN_MODE_MULTI?dwipv4:NULL,m_bSrvIPv6);
            if (!status)
            {
                m_nAudioTransPort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }





    }

    if (status)
    {
        delete sess;
        return -1;
    }

  //  sess->SetDefaultPayloadType(m_nAudioPayloadType);
   // sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(nPayloadType)/1000);
   // sess->SetDefaultMark(false);
    m_pAudioSess = sess;
    return 0;
}
int CRtspClient::CreateApp(int nmode)
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CAppRtpSession *sess;

    if (m_pAppSess != NULL)
    {
        m_pAppSess->Destroy();
        delete m_pAppSess;
        m_pAppSess = NULL;
    }
    sess = new CAppRtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,sess);
    sess->SetProp(m_dwProp);
    if (nmode == RTSPCLIENT_OPEN_MODE_TCP ||
        nmode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

        status = sess->CreateClient(0,1,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
        m_nAppTransPort = 4;
    }
    else
    {
        if (nmode == RTSPCLIENT_OPEN_MODE_MULTI)
        {
            portbase = m_nMulticastPort[2];
        }
        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;;portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->CreateClient(portbase,0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
                if (!status)
                {
                    m_nAppTransPort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
            status = sess->CreateClient(portbase,nmode == RTSPCLIENT_OPEN_MODE_MULTI?2:0,rtprtcp_datapacketcallback,this,NULL,m_bSrvIPv6);
            if (!status)
            {
                m_nAppTransPort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }





    }

    if (status)
    {
        delete sess;
        return -1;
    }

    //  sess->SetDefaultPayloadType(m_nAudioPayloadType);
    // sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(nPayloadType)/1000);
    // sess->SetDefaultMark(false);
    m_pAppSess = sess;
    return 0;
}
int CRtspClient::CreateTalk(int nmode)
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CAudioRtpSession *sess;

    if (m_pTalkSess != NULL)
    {
        m_pTalkSess->Destroy();
        delete m_pTalkSess;
        m_pTalkSess = NULL;
    }
    sess = new CAudioRtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_client_callbakcFxn,sess);
    sess->SetProp(m_dwProp);
    if (nmode == RTSPCLIENT_OPEN_MODE_TCP ||
        nmode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

        status = sess->Create(0,1,rtprtcp_datapacketcallback,this);
        m_nTalkTransPort = 6;
    }
    else
    {
        if (nmode == RTSPCLIENT_OPEN_MODE_MULTI)
        {
            portbase = m_nMulticastPort[1];
        }
        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;;portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->Create(portbase,0,rtprtcp_datapacketcallback,this);
                if (!status)
                {
                    m_nTalkTransPort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
            status = sess->Create(portbase,0,rtprtcp_datapacketcallback,this);
            if (!status)
            {
                m_nTalkTransPort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }





    }

    if (status)
    {
        delete sess;
        return -1;
    }

    //  sess->SetDefaultPayloadType(m_nAudioPayloadType);
    // sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(nPayloadType)/1000);
    // sess->SetDefaultMark(false);
    m_pTalkSess = sess;
    return 0;
}
int CRtspClient::DestoryVideo()
{
    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->Destroy();
        delete m_pVideoSess;
        m_pVideoSess = NULL;
    }
return 0;
}
int CRtspClient::DestoryAudio()
{
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->Destroy();
        delete m_pAudioSess;
        m_pAudioSess = NULL;
    }
return 0;
}
int CRtspClient::DestoryApp()
{
    if (m_pAppSess != NULL)
    {
        m_pAppSess->Destroy();
        delete m_pAppSess;
        m_pAppSess = NULL;
    }
    return 0;
}
int CRtspClient::DestoryTalk()
{
    if (m_pTalkSess != NULL)
    {
        m_pTalkSess->Destroy();
        delete m_pTalkSess;
        m_pTalkSess = NULL;
    }
    return 0;
}
int CRtspClient::GenerateSeq()
{
    int nSeq;
    m_nSendLock.Lock();
    ++m_nSeq;
    nSeq = m_nSeq;
    m_nSendLock.Unlock();
    nSeq &= 0xFFFF;
    return nSeq;
}
int CRtspClient::DataPacketRecvThread()
{
    int nRet;
    m_nRecvLock.Lock();
   nRet = PollData();
   m_nRecvLock.Unlock();

    return nRet;
}
int CRtspClient::PollData_RTP()
{
    unsigned int dwSec = 0,dwUSec = 0;
    if(0 == Ants_rtsp_GetSysRunTime(&dwSec,&dwUSec))
    {
       dwUSec /= 1000;
    }
    for (int nRTPIdx = 0;nRTPIdx < m_nRtp_PayloadNum;nRTPIdx++)
    {
        if (m_pRtp_Session[nRTPIdx] == NULL)
        {
            break;
        }
        m_pRtp_Session[nRTPIdx]->SetTime(dwSec,dwUSec);
        // 发送
        m_pRtp_Session[nRTPIdx]->DoCheckSend();
        // UDP
        m_pRtp_Session[nRTPIdx]->RecvRTPPacket();
        m_pRtp_Session[nRTPIdx]->RecvRTCPPacket();
        // TCP
        if (m_nRtp_pType == 1)
        {
            DealRecv();
            DealCmd();
        }
        m_pRtp_Session[nRTPIdx]->PollRTPPacket();
        m_pRtp_Session[nRTPIdx]->DoCheckSend();
        // 检查状态

    }
	if (m_pVideoSess != NULL)
    {
		m_pVideoSess->SetTime(dwSec,dwUSec);
		m_pVideoSess->RecvRTPPacket();

		m_pVideoSess->RecvRTCPPacket();
        m_pVideoSess->PollRTPPacket();
		m_pVideoSess->DoCheckSend();
    }
	if (m_pSecondVideoSess != NULL)
    {
		m_pSecondVideoSess->SetTime(dwSec,dwUSec);
		m_pSecondVideoSess->RecvRTPPacket();

		m_pSecondVideoSess->RecvRTCPPacket();
        m_pSecondVideoSess->PollRTPPacket();
		m_pSecondVideoSess->DoCheckSend();
    }
    if (m_pAudioSess != NULL)
    {
		m_pAudioSess->SetTime(dwSec,dwUSec);
		m_pAudioSess->RecvRTPPacket();

		m_pAudioSess->RecvRTCPPacket();

        m_pAudioSess->PollRTPPacket();
		m_pAudioSess->DoCheckSend();
    }
    return 0;
}
int  CRtspClient::PollData()
{

    int bFrame = 0;
    bool bav = 0;
    int bhave = 0;
#ifdef NO_RTCP_SUPPORT
    if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP || m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {
     //   return 0;
    }
#endif
#if 1
    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->PollRTPPacket();
    }
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->PollRTPPacket();
    }

#else
#ifndef RTP_SUPPORT_THREAD
    if (m_pVideoSess != NULL)
    {


            bav = 0;
            m_pVideoSess->WaitForIncomingData(RTPTime(0,0),&bav);
            m_pVideoSess->Poll();
            if (bav)
            {
                m_pVideoSess->BeginDataAccess();
                if (m_pVideoSess->GotoFirstSource())
                {
                    do
                    {
                        while ((packet = m_pVideoSess->GetNextPacket()) != 0)
                        {
                            bhave |= 1;
                            bFrame = m_pVideoSess->DealRecvPacket(packet);
                            m_pVideoSess->DeletePacket(packet);
                            if (bFrame == 1)
                            {// Frame
                                break;
                            }
                        }
                        if (bFrame)
                        {
                           break;
                        }

                    } while (m_pVideoSess->GotoNextSource());
                }

                m_pVideoSess->EndDataAccess();
            }




    }
    //audio
    bFrame = 0;
    if (m_pAudioSess != NULL)
    {


            bav = 0;
            m_pAudioSess->WaitForIncomingData(RTPTime(0,0),&bav);
            m_pAudioSess->Poll();
            if (bav)
            {
                m_pAudioSess->BeginDataAccess();
                if (m_pAudioSess->GotoFirstSource())
                {
                    do
                    {
                        while ((packet = m_pAudioSess->GetNextPacket()) != 0)
                        {
                             bhave |= 1;
                            bFrame = m_pAudioSess->DealRecvPacket(packet);
                            m_pAudioSess->DeletePacket(packet);
                            if (bFrame == 1)
                            {// Frame
                                break;
                            }
                        }
                        if (bFrame)
                        {
                            break;
                        }
                    } while (m_pAudioSess->GotoNextSource());
                }

                m_pAudioSess->EndDataAccess();
            }

    }
#else
#if 0
    if (m_pVideoSess != NULL)
    {


        bav = 1;
        if (bav)
        {
            m_pVideoSess->BeginDataAccess();
            if (m_pVideoSess->GotoFirstSourceWithData())
            {
                do
                {
                    while ((packet = m_pVideoSess->GetNextPacket()) != 0)
                    {
                        bhave |= 1;
                        bFrame = m_pVideoSess->DealRecvPacket(packet);
                        m_pVideoSess->DeletePacket(packet);
                        if (bFrame == 1)
                        {// Frame
                            break;
                        }
                    }
                    if (bFrame)
                    {
                        break;
                    }

                } while (m_pVideoSess->GotoNextSourceWithData());
            }

            m_pVideoSess->EndDataAccess();
        }




    }
    //audio
    bFrame = 0;
    if (m_pAudioSess != NULL)
    {


        bav = 0;

        if (bav)
        {
            m_pAudioSess->BeginDataAccess();
            if (m_pAudioSess->GotoFirstSource())
            {
                do
                {
                    while ((packet = m_pAudioSess->GetNextPacket()) != 0)
                    {
                        bhave |= 1;
                        bFrame = m_pAudioSess->DealRecvPacket(packet);
                        m_pAudioSess->DeletePacket(packet);
                        if (bFrame == 1)
                        {// Frame
                            break;
                        }
                    }
                    if (bFrame)
                    {
                        break;
                    }
                } while (m_pAudioSess->GotoNextSource());
            }

            m_pAudioSess->EndDataAccess();
        }

    }
#endif
#endif
#endif
   return bhave;
}
int CRtspClient::IsNeedReConnect()
{

    time_t currTime;
    int bNeedReConnect = 0;
    if (m_nUrlType == 1)
    {
        return 0;
    }
    if (!m_bOpen)
    {
        return 0;
    }
    if (!m_bReady)
    {
        return 0;
    }
    // keepalive
    if (m_bPlaying && (!m_nWaitOPRes) && IsRunning())
    {
        // printf("m_nTimeOut = %d m_dwRunTimeSecond = %d m_tLastSetParameter=%d %d\n",m_nTimeOut,m_dwRunTimeSecond,m_tLastSetParameter,time(NULL));
        if (m_nTimeOut > 5)
        {

            if (m_dwRunTimeSecond > m_tLastSetParameter + m_nTimeOut - 5 || m_dwRunTimeSecond < m_tLastSetParameter || m_tLastSetParameter == 0)
            {
                m_tLastSetParameter = m_dwRunTimeSecond;
                if (m_bGet_Parameter)
                {
                    RTSP_DEBUG("Get_Parameter %d - %d\n",m_tLastSetParameter,m_dwRunTimeSecond);
                    Get_Parameter();
                }
                else if (m_bSet_Parameter)
                {
                    RTSP_DEBUG("Set_Parameter %d - %d\n",m_tLastSetParameter,m_dwRunTimeSecond);
                    Set_Parameter();
                }
                else
                {
                    RTSP_DEBUG("Option %d - %d\n",m_tLastSetParameter,m_dwRunTimeSecond);
                    Option();
                }
                //or send rtcp to keepalive

                DealSend();
            }


        }
        else
        {
            m_nTimeOut = 60;
        }

    }
    //////////////////////////////////////////////////////////////////////////
    if (!m_nAutoReConnect)
    {
        return 0;
    }
    if (m_bReplay)
    { // 回放不重连
        return 0;
    }
    if (!IsRunning())
    {
        bNeedReConnect = 1;
    }

    if (!bNeedReConnect)
    {
        return 0;
    }


    if(m_fCallback_V2 != NULL)
    {
        ANTS_RTSP_STATUS_T tStatus;
        memset(&tStatus,0,sizeof(tStatus));
        tStatus.nStatusType = ANTS_RTSP_STATUS_RECONNECT;
        tStatus.nStatusCode = m_nReason;
        m_fCallback_V2(m_nClientHandle,ANTS_RTSP_DATATYPE_STATUS,0,(unsigned char *)&tStatus,sizeof(tStatus),m_pCallbackUserData);
        if (tStatus.byOPResult == 1)
        {

            return 0;
        }
    }

    return bNeedReConnect;
}

void *CRtspClient::RTPThread()
{
    struct timeval tv_timeToDelay;
    int selectResult;
    int nSocket_rtp = -1,nSocket_rtcp = -1,nTmpSocket;
	 int nSocket_rtp_sec = -1,nSocket_rtcp_sec = -1;
    fd_set readSet;
    int nfds = 0,nRtpIdx;

    while (1)
    {
		if (m_bNeedClose)
		{
			RTSP_DEBUG("[%08x]Close....%d,reason = %d\n",m_nClientHandle,nSocket_rtp,m_nReason);
			if (m_nReason == ANTS_RTSP_ERROR_SocketError)
			{
			   int nError,nRet;
			   socklen_t nsize = sizeof(int);

			   //nRet = getsockopt(m_nSocket, SOL_SOCKET, SO_ERROR, (char *)&nError, &nsize);
			   nError = errno;
			   RTSP_DEBUG("[%08x][%d] %s ip = %x,\n",m_nClientHandle,nError,strerror(nError),m_nSrvIPv4);
			}

            break;
        }

		if (m_nRtp_ServerPort > 0 && m_nSocket > 0 && m_pRtp_ServerIP != NULL && m_bConnected == 0)
		{
		    m_bConnected = 1;
		    m_hConnectStatusLock.Lock();
		    m_bConnecting = 1;
		    m_hConnectStatusLock.Unlock();

			if (m_bRtp_PassiveMode == 0)
			{
				struct sockaddr_in addr;
				memset(&addr,0,sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = inet_addr(m_pRtp_ServerIP);
				addr.sin_port = htons(m_nRtp_ServerPort);
				RTSP_DEBUG("[%08x]Start connect,waiting[%s:%d]...\n",m_nClientHandle,m_pRtp_ServerIP,m_nRtp_ServerPort);
				if (connect(m_nSocket,(struct sockaddr *)&addr,sizeof(addr))!= 0)
				{
				    int err = GetLastError();
				    if(err == EINPROGRESS || err == EWOULDBLOCK)
				    {

				    }
				    else
				    {
		                m_bConnected = 0;
		                m_bConnecting = 0;
		                RTSP_DEBUG("[%08x]Exception %d %s\n",m_nClientHandle,errno,strerror(errno));
		                //NeedClose(ANTS_RTSP_ERROR_SocketError);
				    }


				}
				else
				{
				    m_hConnectStatusLock.Lock();
				    m_bConnecting = 0;
				    m_hConnectStatusLock.Unlock();
				    RTSP_DEBUG("[%08x]Connected %d\n",m_nClientHandle,m_nSocket);

				    Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Connected,0,0,NULL,0);


				    m_bConnected = 2;
					m_nRtp_TcpSockFD = m_nSocket;
				}
			}
			else
			{
			    RTSP_DEBUG("[%08x]Start listen...\n",m_nClientHandle);
			    if (listen(m_nSocket, 10) < 0)
			    {
	                m_bConnected = 0;
	                m_bConnecting = 0;
	                RTSP_DEBUG("[%08x]Exception %d %s\n",m_nClientHandle,errno,strerror(errno));
					closeSocket(m_nSocket);
	                NeedClose(ANTS_RTSP_ERROR_SocketError);
			    }
			}
		}

		if (m_bConnected == 1)
        {
            if (m_bRtp_PassiveMode == 0)
            {
	            fd_set writeSet,exceptSet;
	            // 正在连接中
	            FD_ZERO(&writeSet);
	            FD_ZERO(&exceptSet);


	            nfds = m_nSocket;
	            FD_SET(m_nSocket,&writeSet);
	            FD_SET(m_nSocket,&exceptSet);
	            tv_timeToDelay.tv_sec = 0;
	            tv_timeToDelay.tv_usec = 100000;
	            selectResult = select(nfds + 1, NULL, &writeSet, &exceptSet, &tv_timeToDelay);
	            if (selectResult < 0)
	            {
	                //错误

	                RTSP_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,m_nSocket,GetLastError(),strerror(GetLastError()));
	                if( GetLastError() != EINTR)
	                {
	                    NeedClose();
	                }

	                continue;

	            }
	            if (selectResult == 0)
	            {
	                //超时

	                continue;
	            }

	            if (FD_ISSET(m_nSocket,&writeSet))
	            {
	                int nError,nRet;
	                socklen_t nsize = sizeof(int);
	                m_bConnected = 2;
	                m_bConnecting = 0;
	                nRet = getsockopt(m_nSocket, SOL_SOCKET, SO_ERROR, (char *)&nError, &nsize);
	                if(nRet == 0 && nError == 0)
	                {
	                    m_bConnected = 2;
	                    m_bConnecting = 0;
						m_nRtp_TcpSockFD = m_nSocket;
	                    Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Connected,0,0,NULL,0);
	                    m_nBeforePlayLiveCnt = 0;
	                }
	                else
	                {
#if 0
						if (m_bRtp_PassiveMode == 0)
						{
							struct sockaddr_in addr;
							memset(&addr,0,sizeof(addr));
							addr.sin_family = AF_INET;
							addr.sin_addr.s_addr = inet_addr(m_pRtp_ServerIP);
							addr.sin_port = htons(m_nRtp_ServerPort);
							RTSP_DEBUG("[%08x]Start connect,waiting[%s:%d]...\n",m_nClientHandle,m_pRtp_ServerIP,m_nRtp_ServerPort);
							if (connect(m_nSocket,(struct sockaddr *)&addr,sizeof(addr))!= 0)
							{
								int err = GetLastError();
								if(err == EINPROGRESS || err == EWOULDBLOCK)
								{

								}
								else
								{
									m_bConnected = 0;
									m_bConnecting = 0;
									RTSP_DEBUG("[%08x]Exception %d %s\n",m_nClientHandle,errno,strerror(errno));
									//NeedClose(ANTS_RTSP_ERROR_SocketError);
								}


							}
							else
							{
								m_hConnectStatusLock.Lock();
								m_bConnecting = 0;
								m_hConnectStatusLock.Unlock();
								RTSP_DEBUG("[%08x]Connected %d\n",m_nClientHandle,m_nSocket);

								Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Connected,0,0,NULL,0);


								m_bConnected = 2;
								m_nRtp_TcpSockFD = m_nSocket;
							}
						}
#endif
						m_bConnected = 0;
						m_bConnecting = 0;
						RTSP_DEBUG("[%08x]writeSet %d %s\n",m_nClientHandle,errno,strerror(errno));

	                }
	                RTSP_DEBUG("[%08x]nRet = %d,err = %d,%s\n",m_nClientHandle,nRet,nError,strerror(nError));


	            }

	            if (FD_ISSET(m_nSocket,&exceptSet))
	            {
	                m_bConnected = 0;
	                m_bConnecting = 0;
	                RTSP_DEBUG("[%08x]Exception %d %s\n",m_nClientHandle,errno,strerror(errno));
	                NeedClose(ANTS_RTSP_ERROR_SocketError);

	            }
	            continue;
            }
			else
			{
	            fd_set readSet;
	            // 正在连接中
	            FD_ZERO(&readSet);

	            nfds = m_nSocket;
	            FD_SET(m_nSocket,&readSet);
	            tv_timeToDelay.tv_sec = 0;
	            tv_timeToDelay.tv_usec = 100000;
	            selectResult = select(nfds + 1, &readSet, NULL, NULL, &tv_timeToDelay);
	            if (selectResult < 0)
	            {
	                //错误

	                RTSP_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,m_nSocket,GetLastError(),strerror(GetLastError()));
	                if( GetLastError() != EINTR)
	                {
	                    NeedClose();
	                }

	                continue;

	            }
	            if (selectResult == 0)
	            {
	                //超时

	                continue;
	            }

	            if (FD_ISSET(m_nSocket,&readSet))
	            {
				    struct sockaddr_in clientAddr;
	                socklen_t clientAddrLen = sizeof(clientAddr);
					int clientSocket;

					clientSocket = accept(m_nSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
					if (clientSocket < 0)
					{
					    RTSP_DEBUG("accept fail!!!!!!!!!!!\n");
					}
					else
					{
					    RTSP_DEBUG("accept sucess [%d:%d][%d:%d]\n", clientAddr.sin_addr.s_addr, inet_addr(m_pRtp_ServerIP), htons(clientAddr.sin_port), m_nRtp_ServerPort);
					    if (clientAddr.sin_addr.s_addr == inet_addr(m_pRtp_ServerIP))
					    {
		                    m_bConnected = 2;
		                    m_bConnecting = 0;
							m_nRtp_TcpSockFD = clientSocket;
							//closeSocket(m_nSocket);
							//m_nSocket = clientSocket;
							RTSP_DEBUG("m_nSocket = %d clientSocket = %d!!!!!!!!!!!\n",m_nSocket, clientSocket);
		                    Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Connected,0,0,NULL,0);
					    }
					}
				}

				continue;
			}
        }

        FD_ZERO(&readSet);
        if (m_nRtp_TcpSockFD != -1)
        {
            FD_SET(m_nRtp_TcpSockFD, &readSet);
            if (m_nRtp_TcpSockFD > nfds)
            {
                nfds = m_nRtp_TcpSockFD;
            }
        }
        else
        {
		    if (m_pVideoSess != NULL)
		    {
		       nSocket_rtp = m_pVideoSess->GetRTPSocket();
		       if (nSocket_rtp != -1)
		       {
		           FD_SET(nSocket_rtp, &readSet);
		           if (nSocket_rtp > nfds)
		           {
		               nfds = nSocket_rtp;
		           }
		       }
               if(m_bRtp_RTCP)
               {
                   nSocket_rtcp = m_pVideoSess->GetRTPSocket();
                   if (nSocket_rtcp != -1)
                   {
                       FD_SET(nSocket_rtcp, &readSet);
                       if (nSocket_rtcp > nfds)
                       {
                           nfds = nSocket_rtcp;
                       }
                   }
               }

		    }
			if (m_pSecondVideoSess != NULL)
			{
				nSocket_rtp_sec = m_pSecondVideoSess->GetRTPSocket();
				if (nSocket_rtp_sec != -1)
				{
					FD_SET(nSocket_rtp_sec, &readSet);
					if (nSocket_rtp_sec > nfds)
					{
						nfds = nSocket_rtp_sec;
					}
				}
				if(m_bRtp_RTCP)
				{
					nSocket_rtcp_sec = m_pSecondVideoSess->GetRTPSocket();
					if (nSocket_rtcp_sec != -1)
					{
						FD_SET(nSocket_rtcp_sec, &readSet);
						if (nSocket_rtcp_sec > nfds)
						{
							nfds = nSocket_rtcp_sec;
						}
					}
				}

			}
            for (nRtpIdx = 0; nRtpIdx < m_nRtp_PayloadNum;nRtpIdx++)
            {
                if (m_pRtp_Session[nRtpIdx] != NULL)
                {
                    nTmpSocket = m_pRtp_Session[nRtpIdx]->GetRTPSocket();
                    if (nTmpSocket != -1)
                    {
                        FD_SET(nTmpSocket,&readSet);
                        if (nTmpSocket > nfds)
                        {
                            nfds = nTmpSocket;
                        }
                    }
                    nTmpSocket = m_pRtp_Session[nRtpIdx]->GetRTCPSocket();
                    if (nTmpSocket != -1)
                    {
                        FD_SET(nTmpSocket,&readSet);
                        if (nTmpSocket > nfds)
                        {
                            nfds = nTmpSocket;
                        }
                    }
                }
            }
        }
#ifdef USE_ONE_THREAD
        PollData();
#endif
		tv_timeToDelay.tv_sec = 0;
        tv_timeToDelay.tv_usec = 10000;

		selectResult = select(nfds + 1, &readSet, NULL/*&writeSet*/, NULL, &tv_timeToDelay);
        if (selectResult < 0)
        {//错误

            RTSP_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,nSocket_rtp,GetLastError(),strerror(GetLastError()));
            if( GetLastError() != EINTR)
            {
                NeedClose(ANTS_RTSP_ERROR_SocketError);
            }

            continue;
         }
         else if (selectResult == 0)
         {//超时
             continue;
         }
         else if(m_nRtp_TcpSockFD != -1)
         {
             if (FD_ISSET(m_nRtp_TcpSockFD,&readSet))
             {
                //DealRecv();
                //DealCmd();
                RecvAndParse_noblock_rtp();
             }
         }
		 else
		 {
		     if (nSocket_rtp != -1 && FD_ISSET(nSocket_rtp, &readSet))
		     {
		         if (m_pVideoSess != NULL && m_pVideoSess->GetDefaultPayloadType() >= 0)
		         {
		             //RTSP_DEBUG("[%s.%d]\n", __FUNCTION__, __LINE__);
		             m_pVideoSess->RecvRTPPacket(1);
		         }
		     }

			 if (nSocket_rtp_sec != -1 && FD_ISSET(nSocket_rtp_sec, &readSet))
			 {
				 if (m_pSecondVideoSess != NULL && m_pSecondVideoSess->GetDefaultPayloadType() >= 0)
				 {
					 //RTSP_DEBUG("[%s.%d]\n", __FUNCTION__, __LINE__);
					 m_pSecondVideoSess->RecvRTPPacket(1);
				 }
			 }
             for (nRtpIdx = 0; nRtpIdx < m_nRtp_PayloadNum;nRtpIdx++)
             {
                 if (m_pRtp_Session[nRtpIdx] != NULL)
                 {
                     nTmpSocket = m_pRtp_Session[nRtpIdx]->GetRTPSocket();
                     if (nTmpSocket != -1 && FD_ISSET(nTmpSocket,&readSet))
                     {
                        m_pRtp_Session[nRtpIdx]->RecvRTPPacket(1);
                     }
                     nTmpSocket = m_pRtp_Session[nRtpIdx]->GetRTCPSocket();
                     if (nTmpSocket != -1)
                     {
                        m_pRtp_Session[nRtpIdx]->RecvRTCPPacket();
                     }
                 }
             }
		 }
    }

    Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Disconnected,0,m_nReason,NULL,0);

    return NULL;
}

void *CRtspClient::Thread()
{
    struct timeval tv_timeToDelay;
    fd_set readSet,writeSet,exceptSet;
    int selectResult;
  //  time_t currTime;
    int nfds = 0;
    int nSocket_rtp,nSocket_rtcp,nSocket_rtpA,nSocket_rtcpA;
    int bLocalIPSet = 0;
    unsigned int bTimeout = 0,dwTimeout = 0;
    m_dwRunTimeLastSecond = 0;
    m_dwRunTimeSecond = 0;
    m_dwRunTimeMSecond = 0;
    ThreadStarted();

	if (m_nUrlType == 1)
	{
        return RTPThread();
	}

#ifndef WIN32
     RTSP_DEBUG("[%08x]select id = %d\n",m_nClientHandle,getpid());
#endif
    m_nNextOPIndex = 0;
    m_nWaitOPRes = 0;
    //time(&m_tLastLiveTime);
    Ants_rtsp_GetSysRunTime(&m_tLastLiveTime,NULL);
    m_nLiveCnt = 0;
    m_nLastIframeTime = m_tLastLiveTime;
    m_nIFrameLiveCnt = 0;
    while ((!m_nAutoConnect) && (!m_bStart))
    {
        if (m_bNeedClose)
        {
            break;
        }
        Ants_WaitTime(0,100000);
    }
    Connect();
    m_bReady = 1;
    //time(&m_tLastLiveTime);
    Ants_rtsp_GetSysRunTime(&m_tLastLiveTime,NULL);
    m_nLiveCnt = 0;
    m_nLastIframeTime = m_tLastLiveTime;
    m_nIFrameLiveCnt = 0;
    m_nBeforePlayLiveCnt = 0;

    nSocket_rtp = -1;
    nSocket_rtcp = -1;
    nSocket_rtpA = -1;
    nSocket_rtcpA = -1;
   while(1)
   {
       if (m_bNeedClose)
       {
           RTSP_DEBUG("[%08x]Close....%d,reason = %d\n",m_nClientHandle,m_nSocket,m_nReason);
           if (m_nReason == ANTS_RTSP_ERROR_SocketError)
           {
               int nError,nRet;
               socklen_t nsize = sizeof(int);

               //nRet = getsockopt(m_nSocket, SOL_SOCKET, SO_ERROR, (char *)&nError, &nsize);
               nError = errno;
               RTSP_DEBUG("[%08x][%d] %s ip = %x,\n",m_nClientHandle,nError,strerror(nError),m_nSrvIPv4);
           }
           if(m_nReason != ANTS_RTSP_ERROR_SocketClose &&
               m_nReason != ANTS_RTSP_ERROR_SocketError &&
               m_bConnected == 2)
           {
               //RTSP_DEBUG("TearDown Start Reason = %d\n",m_nReason);
                TearDown();
                // RTSP_DEBUG("TearDown end\n");
           }

          // tv_timeToDelay.tv_sec = 2;
          // tv_timeToDelay.tv_usec = 0;
          // select(0, NULL, NULL, NULL, &tv_timeToDelay);
           break;

       }
       m_nOPLock.Lock();
       if (m_nSocket < 0)
       {
           m_bConnected = 0;
           m_nOPLock.Unlock();
           break;
       }
       m_nOPLock.Unlock();

       if (m_bConnected == 1)
       {// 正在连接中
             FD_ZERO(&writeSet);
             FD_ZERO(&exceptSet);


           nfds = m_nSocket;
           FD_SET(m_nSocket,&writeSet);
           FD_SET(m_nSocket,&exceptSet);
           tv_timeToDelay.tv_sec = 0;
           tv_timeToDelay.tv_usec = 100000;
           selectResult = select(nfds + 1, NULL, &writeSet, &exceptSet, &tv_timeToDelay);
           if (selectResult < 0)
           {//错误

               RTSP_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,m_nSocket,GetLastError(),strerror(GetLastError()));
               if( GetLastError() != EINTR)
               {
                   NeedClose();
               }

               continue;

           }
           if (selectResult == 0)
           {//超时

               continue;
           }

           if (FD_ISSET(m_nSocket,&writeSet))
           {
                int nError,nRet;
                socklen_t nsize = sizeof(int);
               m_bConnected = 2;
               m_bConnecting = 0;
               nRet = getsockopt(m_nSocket, SOL_SOCKET, SO_ERROR, (char *)&nError, &nsize);
               if(nRet == 0 && nError == 0)
               {
                   m_bConnected = 2;
                   m_bConnecting = 0;
                   Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Connected,0,0,NULL,0);
                   m_nBeforePlayLiveCnt = 0;
               }
               else
               {
                   m_bConnected = 0;
                   m_bConnecting = 0;
                   NeedClose(ANTS_RTSP_ERROR_SocketError);
               }
                  RTSP_DEBUG("[%08x]nRet = %d,err = %d,%s\n",m_nClientHandle,nRet,nError,strerror(nError));


           }

           if (FD_ISSET(m_nSocket,&exceptSet))
           {
               m_bConnected = 0;
               m_bConnecting = 0;
               RTSP_DEBUG("[%08x]Exception %d %s\n",m_nClientHandle,errno,strerror(errno));
               NeedClose(ANTS_RTSP_ERROR_SocketError);

           }
           continue;
      }
           if(!bLocalIPSet)
           {
               struct sockaddr_in addr;
			    struct sockaddr_in6 addr6;
				struct sockaddr *pAddr = NULL;
               int nRet;
               socklen_t addrlen;
               addrlen = sizeof(addr);
			    pAddr = (struct sockaddr *)&addr;
				m_bLocalIPv6 = m_bSrvIPv6;
			   if (m_bSrvIPv6)
			   {
				   addrlen = sizeof(addr6);
				   pAddr = (struct sockaddr *)&addr6;
			   }

               nRet = getsockname(m_nSocket,(struct sockaddr *)pAddr,&addrlen);
               if (!nRet)
               {
				   if (m_bSrvIPv6)
				   {
						memcpy(m_nLocalIPv4,&addr6.sin6_addr,16);
				   }
				   else
				   {
						m_nLocalIPv4[0] = ntohl(addr.sin_addr.s_addr);
				   }
                   m_nLocalPort = ntohs(addr.sin_port);
                   bLocalIPSet = 1;
               }

           }


           //printf("m_nWaitOPRes = %d m_nNextOPIndex = %d\n",m_nWaitOPRes,m_nNextOPIndex);
           if(!m_nWaitOPRes)
           {
               if (m_nNextOPIndex < RTSPCLIENT_MAX_SUPPORT_CMD_NUM)
               {
                   switch ( m_nOPOrders[m_nNextOPIndex])
                   {
                   case 'o':
                       {

                           Option();
                           break;
                       }
                   case 'd':
                       {
                           Describe();
                           break;
                       }
                   case 'v':
                       {
                           SetupV();
                           break;
                       }
                   case 'a':
                       {
                           SetupA();
                           break;
                       }
                   case 'A':
                       {
                           SetupApp();
                           break;
                       }
                   case 't':
                       {
                           SetupTalk();
                           break;
                       }
                   case 'p':
                       {
                           Play();
                           break;
                       }
                   default:
                       {

                       }

                   }
                   m_nNextOPIndex++;
               }

           }

           unsigned int dwSec ,dwUSec;
           if(0 == Ants_rtsp_GetSysRunTime(&dwSec,&dwUSec))
           {
               m_dwRunTimeSecond = dwSec;
               m_dwRunTimeMSecond = dwUSec / 1000;
           }
          // printf("m_dwRunTimeSecond = %d dwSec = %d\n",m_dwRunTimeSecond,dwSec);
           if(m_dwRunTimeLastSecond != m_dwRunTimeSecond)
           {
              long lSec,luSec;

               //Ants_rtsp_CurrentTime(&lSec,&luSec);
               // printf("sec = %d msec = %d  ............................ dwRunTimeLastSecond = %d\n",m_dwRunTimeSecond,m_dwRunTimeMSecond,m_dwRunTimeLastSecond);
               m_dwRunTimeLastSecond = m_dwRunTimeSecond;
               if (m_pVideoSess != NULL)
               {
                   m_pVideoSess->SetTime(m_dwRunTimeSecond,m_dwRunTimeMSecond);
               }
               if (m_pAudioSess != NULL)
               {
                   m_pAudioSess->SetTime(m_dwRunTimeSecond,m_dwRunTimeMSecond);
               }
               if (m_bPlaying)
               {
                    m_nBeforePlayLiveCnt = 0;
               }
               else
               {
                    m_nLiveCnt = 0;
                    m_nIFrameLiveCnt = 0;
                    m_nBeforePlayLiveCnt++;
               }
               if (m_nBeforePlayLiveCnt > 60)
               {
                   m_nLiveCnt = 60;
                   m_nBeforePlayLiveCnt = 0;
               }

               m_nLiveCnt++;
               m_nIFrameLiveCnt++;
               //RTSP_DEBUG("m_nLiveCnt = %d m_nIFrameLiveCnt = %d m_nBeforePlayLiveCnt =%d\n",m_nLiveCnt,m_nIFrameLiveCnt,m_nBeforePlayLiveCnt);

               if(m_nLiveCnt > 60)
               {
                   m_nLiveCnt = 0;
                   m_nIFrameLiveCnt = 0;
                   NeedClose(ANTS_RTSP_ERROR_NoAnyData);
               }
               else if(m_nIFrameLiveCnt > RTSPCLIENT_NOIFRAME_TIMEOUT / 5)
               {
                   NeedClose(ANTS_RTSP_ERROR_NoAnyData);
                   m_nLiveCnt = 0;
                   m_nIFrameLiveCnt = 0;
               }
               if (m_bReplay)
               {
                   if(m_bPausing  || m_bIntraPause)
                   {
                       m_nLiveCnt = 0;
                       m_nIFrameLiveCnt = 0;
                   }
               }

           }
           if (m_bPlaying)
           {
               int n,m,bExist = 0,nUseCnt,nUseCntReq;

               m_hAddDecChanLock.Lock();
               if (m_nCombChanDecReqCnt != m_nCombChanDecDoReqCnt)
               {
                   int nCombChan[RTSPCLIENT_CHAN_MAX];
                   int nCombChanCnt = 0;
                   int nCurrCnt = m_nCombChanCurrCnt;

                   nUseCnt = 0;
                   for (n = 0 ; n < RTSPCLIENT_CHAN_MAX; n++)
                   {
                       if (nUseCnt >= nCurrCnt)
                       {
                           break;
                       }
                       if(m_nCombChanCurr[n] != 0x80808080)
                       {

                           nUseCnt++;
                           bExist = 0;
                           nUseCntReq = 0;
                           for (m = 0 ; m < RTSPCLIENT_CHAN_MAX; m++)
                           {
                               if (nUseCntReq >= m_nCombChanReqCnt)
                               {
                                   break;
                               }
                               if (m_nCombChanReq[m] != 0x80808080)
                               {
                                   nUseCntReq++;
                                   if (m_nCombChanCurr[n] == m_nCombChanReq[m])
                                   {
                                       bExist = 1;
                                       break;
                                   }
                               }
                           }
                           if (!bExist)
                           {

                               nCombChan[nCombChanCnt] = m_nCombChanCurr[n];
                               nCombChanCnt++;
                               m_nCombChanCurr[n] = 0x80808080;
                               m_nCombChanCurrCnt -- ;
                           }
                       }
                   }
                   if (nCombChanCnt > 0)
                   {
                       AntsComb_DecCh(nCombChan,nCombChanCnt);
                   }
                   m_nCombChanDecDoReqCnt = m_nCombChanDecReqCnt;
               }
               if (m_nCombChanAddReqCnt != m_nCombChanAddDoReqCnt)
               {
                   int nCombChan[RTSPCLIENT_CHAN_MAX];
                   int nCombChanCnt = 0;
                   int nNewIdx = -1;

                   nUseCntReq = 0;
                   for (n = 0 ; n < RTSPCLIENT_CHAN_MAX; n++)
                   {
                       if (nUseCntReq >= m_nCombChanReqCnt)
                       {
                           break;
                       }
                       if(m_nCombChanReq[n] != 0x80808080)
                       {
                           nUseCntReq++;
                           bExist = 0;
                           nUseCnt = 0;
                           nNewIdx = -1;
                            for (m = 0 ; m < RTSPCLIENT_CHAN_MAX; m++)
                            {
                                if (nUseCnt >= m_nCombChanCurrCnt && nNewIdx != -1)
                                {
                                    break;
                                }
                                if (m_nCombChanCurr[m] != 0x80808080)
                                {
                                    nUseCnt++;
                                    if (m_nCombChanReq[n] == m_nCombChanCurr[m])
                                    {
                                        bExist = 1;
                                        break;
                                    }
                                }
                                else if (nNewIdx == -1)
                                {
                                    nNewIdx = m;
                                }
                            }
                            if ((!bExist) && nNewIdx != -1)
                            {

                                m_nCombChanCurr[nNewIdx] = m_nCombChanReq[n];
                                m_nCombChanCurrCnt++;
                                nCombChan[nCombChanCnt] = m_nCombChanReq[n];
                                nCombChanCnt++;
                            }
                       }
                   }
                   if (nCombChanCnt > 0)
                   {
                       AntsComb_AddCh(nCombChan,nCombChanCnt);
                   }
                   m_nCombChanAddDoReqCnt = m_nCombChanAddReqCnt;
               }

               m_hAddDecChanLock.Unlock();
           }
           //printf("play = %d replay = %d waitres = %d\n",m_bPlaying,m_bReplay,m_nWaitOPRes);
            if (m_bPlaying && m_bReplay )
            {
                int bDoPlay = 0;
                if (m_bBufferMgrMethod)
                {
                    // 检查缓冲大小
                    unsigned long len = 0;
                    int nLevel = 0;
                    m_hBuffMgrLock.Lock();
                    nLevel = m_nBufferMgrSize * 100 / m_nRcvBuffSize;
                    m_hBuffMgrLock.Unlock();
                  //  printf("[%d] %d-%d\n",nLevel,m_nBufferMgrSize,m_nRcvBuffSize);
                    if (m_bIntraPause)
                    {
                        if (nLevel <= 20)
                        { //
                            // 需要恢复
                            if(!m_bPausing)
                            {
                                Play();
                            }

                            m_bIntraPause = 0;
                            RTSP_DEBUG("Buffer is empty,play! len = %d\n",len);
                        }
                    }
                    else if(nLevel >= 80 && (!m_bPausing))
                    {
                         Pause();
                         m_bIntraPause = 1;
                         RTSP_DEBUG("Buffer is full,Pause! len = %d\n",len);
                    }
                }
                if(!m_bIntraPause)
                {
                    if (m_bPause != m_bPauseReq)
                    {
                        m_bPause = m_bPauseReq;
                        if (m_bPause)
                        {
                            Pause();
                            m_bPausing = 1;
                        }
                        else
                        {
                            bDoPlay = 1;
                        }

                    }


                    if (m_bSeek != m_bSeek_Req)
                    {

                        bDoPlay = 1;
                    }
                    if (m_nScale != m_nScale_Req)
                    {
                        bDoPlay = 1;
                    }
                    if (m_bRateControl != m_bRateControl_Req)
                    {
                        bDoPlay = 1;
                    }

                    if (m_bPlay != m_bPlay_Req)
                    {
                        m_bPlay = m_bPlay_Req;
                        bDoPlay = 1;
                    }
                    if (m_bOnlyIFrame != m_bOnlyIFrame_Req)
                    {
                        bDoPlay = 1;
                    }
                    if (bDoPlay)
                    {
                        Play();
                        m_bPausing = 0;
                    }
                }


            }

           //time(&currTime);









          DealSend();




       FD_ZERO(&readSet);
   //    FD_ZERO(&writeSet);

       FD_SET(m_nSocket,&readSet);

       nfds = m_nSocket;
  //     FD_SET(m_nSocket,&writeSet);
       if (m_pVideoSess != NULL)
       {

           nSocket_rtp = m_pVideoSess->GetRTPSocket();
           nSocket_rtcp = m_pVideoSess->GetRTCPSocket();
           if ( nSocket_rtp != -1)
           {
               FD_SET(nSocket_rtp,&readSet);
               if (nSocket_rtp > nfds)
               {
                   nfds = nSocket_rtp;
               }
           }
           if ( nSocket_rtcp != -1)
           {
               FD_SET(nSocket_rtcp,&readSet);
               if (nSocket_rtcp > nfds)
               {
                   nfds = nSocket_rtcp;
               }
           }
       }
       if (m_pAudioSess != NULL)
       {

           nSocket_rtpA = m_pAudioSess->GetRTPSocket();
           nSocket_rtcpA = m_pAudioSess->GetRTCPSocket();
           if ( nSocket_rtpA != -1)
           {
               FD_SET(nSocket_rtpA,&readSet);
               if (nSocket_rtpA > nfds)
               {
                   nfds = nSocket_rtpA;
               }
           }
           if ( nSocket_rtcpA != -1)
           {
               FD_SET(nSocket_rtcpA,&readSet);
               if (nSocket_rtcpA > nfds)
               {
                   nfds = nSocket_rtcpA;
               }
           }
       }


       if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP || m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
       {
           tv_timeToDelay.tv_sec = 0;
           tv_timeToDelay.tv_usec = 10000;
       }
       else
       {
           tv_timeToDelay.tv_sec = 0;
           tv_timeToDelay.tv_usec = 10000;
       }
        bTimeout = 0;
#ifdef USE_ONE_THREAD
       PollData();
#endif
        selectResult = select(nfds + 1, &readSet, NULL/*&writeSet*/, NULL, &tv_timeToDelay);
       if (selectResult < 0)
       {//错误

           RTSP_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,m_nSocket,GetLastError(),strerror(GetLastError()));
           if( GetLastError() != EINTR)
           {
                NeedClose(ANTS_RTSP_ERROR_SocketError);
           }

           continue;

       }
       if (selectResult == 0)
       {//超时
           bTimeout = 1;
           dwTimeout = 0;
           continue;
       }
        dwTimeout = 10000 - tv_timeToDelay.tv_usec;

#if 0
       if (FD_ISSET(m_nSocket,&writeSet))
       {
           DealSend();

       }
#endif
       if (FD_ISSET(m_nSocket,&readSet))
       {
           DealRecv();
           DealCmd();

       }
        if (nSocket_rtp != -1 && FD_ISSET(nSocket_rtp,&readSet))
        {
            if (m_pVideoSess != NULL)
            {
                m_pVideoSess->RecvRTPPacket();
            }

        }
        if (nSocket_rtcp != -1 && FD_ISSET(nSocket_rtcp,&readSet))
        {
            if (m_pVideoSess != NULL)
            {
                m_pVideoSess->RecvRTCPPacket();
            }

        }
        if (nSocket_rtpA != -1 && FD_ISSET(nSocket_rtpA,&readSet))
        {
            if (m_pAudioSess != NULL)
            {
                m_pAudioSess->RecvRTPPacket();
            }

        }
        if (nSocket_rtcpA != -1 && FD_ISSET(nSocket_rtcpA,&readSet))
        {
            if (m_pAudioSess != NULL)
            {
                m_pAudioSess->RecvRTCPPacket();
            }

        }


   }

   Client_callbakcFxn(ANTS_RTPClientCallbackType_Status,ANTS_RTSP_STATUS_Disconnected,0,m_nReason,NULL,0);

    return NULL;
}

int CRtspClient::Option()
{
    int nlen,npos;
    int nSeq;
    char *buf = NULL;
    buf = new char [ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"OPTIONS %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "OPTIONS";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    if(m_bPlaying)
    {
        AddReqList(RTSP_CMD_TYPE_OPTION,nSeq,0);

    }
    else
    {
        AddReqList(RTSP_CMD_TYPE_OPTION,nSeq,1);
        m_nWaitOPRes = 1;
        m_nWaitSeq = nSeq;
    }

    delete []buf;
    return 0;
}
int CRtspClient::Describe()
{
    int nlen,npos;
    int nSeq;
    char *buf;
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if(buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"DESCRIBE %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");


    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Accept: application/sdp\r\n");
	if (m_bNeedAuth)
	{

		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "DESCRIBE";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}
    if (m_bSetupTalk_Req)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: www.onvif.org/ver20/backchannel\r\n");

    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    AddReqList(RTSP_CMD_TYPE_DESCRIBE,nSeq,1);
    delete []buf;
    return 0;
}


int CRtspClient::SetupV()
{

    int nlen,npos;
    int nSeq;
    char *buf=NULL;
    if (!m_bVideoExist)
    {
        return 0;
    }
    if (!m_bSetupVideo_Req)
    {
        return 0;
    }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if(buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    if (m_pVideo_control != NULL  && m_pVideo_control[0] != '*' && m_pVideo_control[0] != 0)
    {
        if (m_pBaseUrl != NULL)
        {
            if (NULL != strstri(m_pVideo_control,":/"))
            {// abs-url
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pVideo_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pBaseUrl,m_pVideo_control);
            }

        }
        else
        {
            if (NULL != strstri(m_pVideo_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pVideo_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pUrl,m_pVideo_control);
            }

        }

    }
    else
    {

        if (m_pBaseUrl != NULL)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pBaseUrl);
        }
        else
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pUrl);
        }

    }

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n",m_nVideoTransPort,m_nVideoTransPort+1);
    }
    else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
    {
        int nPort;
        nPort = m_nMulticastPort[0];
        if (nPort == 0)
        {
            nPort = m_nVideoTransPort;
        }
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;multicast;port=%d-%d\r\n",nPort,nPort+1);
    }
    else
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",m_nVideoTransPort,m_nVideoTransPort+1);
    }

    if(m_strSessionID != NULL)
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "SETUP";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    if (m_bReplay)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: onvif-replay\r\n");
    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    AddReqList(RTSP_CMD_TYPE_SETUP_V,nSeq,1);
    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    delete []buf;
    m_bHasVideo = 1;
    return 0;
}
int CRtspClient::SetupA()
{

    int nlen,npos;
    int nSeq;
    char *buf=NULL;
    if (!m_bAudioExist)
    {
        return 0;
    }
    if (!m_bSetupAudio_Req)
    {
        return 0;
    }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    if (m_pAudio_control != NULL && m_pAudio_control[0] != '*' && m_pAudio_control[0] != 0)
    {
        if (m_pBaseUrl != NULL)
        {
            if (NULL != strstri(m_pAudio_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pAudio_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pBaseUrl,m_pAudio_control);
            }

        }
        else
        {
            if (NULL != strstri(m_pAudio_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pAudio_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pUrl,m_pAudio_control);
            }

        }
    }
    else
    {
        if (m_pBaseUrl != NULL)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pBaseUrl);
        }
        else
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pUrl);
        }

    }

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n",m_nAudioTransPort,m_nAudioTransPort+1);
    }
    else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
    {
        int nPort;
        nPort = m_nMulticastPort[1];
        if (nPort == 0)
        {
            nPort = m_nAudioTransPort;
        }
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;multicast;port=%d-%d\r\n",nPort,nPort+1);
    }
    else
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",m_nAudioTransPort,m_nAudioTransPort+1);
    }
    if(m_strSessionID != NULL)
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "SETUP";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    if (m_bReplay)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: onvif-replay\r\n");
    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    AddReqList(RTSP_CMD_TYPE_SETUP_A,nSeq,1);
    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    delete []buf;
    return 0;
}

int CRtspClient::SetupApp()
{

    int nlen,npos;
    int nSeq;
    char *buf=NULL;
    if (!m_bAppExist)
    {
        return 0;
    }
    if (!m_bSetupApp_Req)
    {
        return 0;
    }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    if (m_pApp_control != NULL && m_pApp_control[0] != '*' && m_pApp_control[0] != 0)
    {
        if (m_pBaseUrl != NULL)
        {
            if (NULL != strstri(m_pApp_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pApp_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pBaseUrl,m_pApp_control);
            }

        }
        else
        {
            if (NULL != strstri(m_pApp_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pApp_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pUrl,m_pApp_control);
            }

        }
    }
    else
    {
        if (m_pBaseUrl != NULL)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pBaseUrl);
        }
        else
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pUrl);
        }

    }

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n",m_nAppTransPort,m_nAppTransPort+1);
    }
    else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
    {
        int nPort;
        nPort = m_nMulticastPort[1];
        if (nPort == 0)
        {
            nPort = m_nAppTransPort;
        }
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;multicast;port=%d-%d\r\n",nPort,nPort+1);
    }
    else
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",m_nAppTransPort,m_nAppTransPort+1);
    }
    if(m_strSessionID != NULL)
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "SETUP";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    if (m_bReplay)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: onvif-replay\r\n");
    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    AddReqList(RTSP_CMD_TYPE_SETUP_APP,nSeq,1);
    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    delete []buf;
    return 0;
}

int CRtspClient::SetupApp_handle(CMessage *pMsg)
{

    char *pSessionID = NULL;
    int line,seg;
    char *pstr;
    int bTimeout = 0;
    int nTransPort0 = -1,nTransPort1 = -1;
    int nSrvTransPort0 = -1,nSrvTransPort1 = -1;
    for (line = 0; line < pMsg->m_nLineCnt;line++)
    {

        if (!pMsg->StrCmp("Session:",line,0,strlen("Session:")))
        {
            pSessionID = pMsg->GetStr(line,1);
            if (pSessionID != NULL)
            {
                pstr = strchr(pSessionID,';');
                if (pstr != NULL)
                {
                    pstr[0] = 0;
                    pstr = (char *)strstri(pstr + 1,"timeout=");
                    if (pstr != NULL )
                    {
                        sscanf(pstr+8,"%d",&m_nTimeOut);
                        bTimeout = 1;
                    }
                }

            }
			else
			{
				pSessionID = pMsg->GetStr(line,0);
				pSessionID += strlen("Session:");
				pstr = strchr(pSessionID,';');
				if (pstr != NULL)
				{
					pstr[0] = 0;
					pstr = (char *)strstri(pstr + 1,"timeout=");
					if (pstr != NULL )
					{
						sscanf(pstr+8,"%d",&m_nTimeOut);
						bTimeout = 1;
					}
				}
			}
            if(!bTimeout)
            {
                for (seg = 2; seg < pMsg->m_nSegCnt[line];seg++)
                {
                    pstr = pMsg->StrStr("timeout=",line,seg);
                    if (pstr != NULL)
                    {
                        sscanf(pstr+8,"%d",&m_nTimeOut);
                    }
                }
            }

        }
        if (!pMsg->StrCmp("Transport:",line,0,strlen("Transport:")))
        {
            for (seg = 0; seg < pMsg->m_nSegCnt[line];seg++)
            {

                if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
                    m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
                {
                    pstr = pMsg->StrStr("interleaved=",line,seg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                        if (n != 2)
                        {
                            n = sscanf(pstr+12,"%d",&nTransPort0);
                            if (n == 1)
                            {
                                nTransPort1 = nTransPort0 + 1;
                            }
                        }
                    }

                }
                else
                {
                    pstr = pMsg->StrStr("client_port=",line,seg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                        if (n != 2)
                        {
                            n = sscanf(pstr+12,"%d",&nTransPort0);
                            if (n == 1)
                            {
                                nTransPort1 = nTransPort0 + 1;
                            }
                        }
                    }
                    pstr = pMsg->StrStr("server_port=",line,seg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr+12,"%d-%d",&nSrvTransPort0,&nSrvTransPort1);
                        if (n != 2)
                        {
                            n = sscanf(pstr+12,"%d",&nSrvTransPort0);
                            if (n == 1)
                            {
                                nSrvTransPort1 = nSrvTransPort0 + 1;
                            }
                        }
                    }
                }
            }
        }

    }
    if(!m_bFirstSetup)
    {
        m_bFirstSetup |= 1;
        if (pSessionID == NULL || (m_strSessionID != NULL && strcmp(m_strSessionID ,pSessionID)))
        {
            NeedClose(ANTS_RTSP_ERROR_InvSessionID);
        }
        else if (m_strSessionID == NULL)
        {
            m_strSessionID = Ants_strndup(pSessionID,-1);
            if (m_strSessionID == NULL)
            {
                NeedClose(ANTS_RTSP_ERROR_InvSessionID);
            }
        }
    }

    if ((m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)&& m_nAppTransPort != nTransPort0)
    {
        m_nAppTransPort = nTransPort0;
    }
    if (nTransPort0 == -1 ||
        nTransPort1 == -1 ||
        (nTransPort0 & 1) ||
        (nTransPort1 != nTransPort0 + 1) ||
        (m_nMode != RTSPCLIENT_OPEN_MODE_MULTI && m_nAppTransPort != nTransPort0))
    {
        NeedClose(ANTS_RTSP_ERROR_InvTransPort);
    }

    if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

    }
    else
    {
#if 0
        if (nSrvTransPort0 == -1 ||
            nSrvTransPort1 == -1 ||
            (nSrvTransPort0 & 1) ||
            (nSrvTransPort1 != nSrvTransPort0 + 1) )
        {
            NeedClose(ANTS_RTSP_ERROR_InvTransPort);
        }
        else
        {
            m_nAppSrvTransPort = nSrvTransPort0;
        }
#else
        m_nAppSrvTransPort = nSrvTransPort0;
#endif

    }





    return 0;
}
int CRtspClient::SetupTalk()
{

    int nlen,npos;
    int nSeq;
    char *buf=NULL;
    if (!m_bTalkExist)
    {
        return 0;
    }
    if (!m_bSetupTalk_Req)
    {
        return 0;
    }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    if (m_pTalk_control != NULL && m_pTalk_control[0] != '*' && m_pTalk_control[0] != 0)
    {
        if (m_pBaseUrl != NULL)
        {
            if (NULL != strstri(m_pTalk_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pTalk_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pBaseUrl,m_pTalk_control);
            }

        }
        else
        {
            if (NULL != strstri(m_pTalk_control,":/"))
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pTalk_control);
            }
            else
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s/%s RTSP/1.0\r\n",m_pUrl,m_pTalk_control);
            }

        }
    }
    else
    {
        if (m_pBaseUrl != NULL)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pBaseUrl);
        }
        else
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SETUP %s RTSP/1.0\r\n",m_pUrl);
        }

    }

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n",m_nTalkTransPort,m_nTalkTransPort+1);
    }
    else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
    {
        int nPort;
        nPort = m_nMulticastPort[1];
        if (nPort == 0)
        {
            nPort = m_nTalkTransPort;
        }
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;multicast;port=%d-%d\r\n",nPort,nPort+1);
    }
    else
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",m_nTalkTransPort,m_nTalkTransPort+1);
    }
    if(m_strSessionID != NULL)
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "SETUP";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    if (m_bReplay)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: onvif-replay\r\n");
    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    AddReqList(RTSP_CMD_TYPE_SETUP_TALK,nSeq,1);
    delete []buf;
    return 0;
}

int CRtspClient::SetupTalk_handle(CMessage *pMsg)
{

    char *pSessionID = NULL;
    int line,seg;
    char *pstr;
    int bTimeout = 0;
    int nTransPort0 = -1,nTransPort1 = -1;
    int nSrvTransPort0 = -1,nSrvTransPort1 = -1;
    for (line = 0; line < pMsg->m_nLineCnt;line++)
    {

        if (!pMsg->StrCmp("Session:",line,0,strlen("Session:")))
        {
            pSessionID = pMsg->GetStr(line,1);
            if (pSessionID != NULL)
            {
                pstr = strchr(pSessionID,';');
                if (pstr != NULL)
                {
                    pstr[0] = 0;
                    pstr = (char *)strstri(pstr + 1,"timeout=");
                    if (pstr != NULL )
                    {
                        sscanf(pstr+8,"%d",&m_nTimeOut);
                        bTimeout = 1;
                    }
                }

            }
			else
			{
				pSessionID = pMsg->GetStr(line,0);
				pSessionID += strlen("Session:");
				pstr = strchr(pSessionID,';');
				if (pstr != NULL)
				{
					pstr[0] = 0;
					pstr = (char *)strstri(pstr + 1,"timeout=");
					if (pstr != NULL )
					{
						sscanf(pstr+8,"%d",&m_nTimeOut);
						bTimeout = 1;
					}
				}
			}
            if(!bTimeout)
            {
                for (seg = 2; seg < pMsg->m_nSegCnt[line];seg++)
                {
                    pstr = pMsg->StrStr("timeout=",line,seg);
                    if (pstr != NULL)
                    {
                        sscanf(pstr+8,"%d",&m_nTimeOut);
                    }
                }
            }

        }
        if (!pMsg->StrCmp("Transport:",line,0,strlen("Transport:")))
        {
            for (seg = 0; seg < pMsg->m_nSegCnt[line];seg++)
            {

                if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
                    m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
                {
                    pstr = pMsg->StrStr("interleaved=",line,seg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                        if (n != 2)
                        {
                            n = sscanf(pstr+12,"%d",&nTransPort0);
                            if (n == 1)
                            {
                                nTransPort1 = nTransPort0 + 1;
                            }
                        }
                    }

                }
                else
                {
                    pstr = pMsg->StrStr("client_port=",line,seg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                        if (n != 2)
                        {
                            n = sscanf(pstr+12,"%d",&nTransPort0);
                            if (n == 1)
                            {
                                nTransPort1 = nTransPort0 + 1;
                            }
                        }
                    }
                    pstr = pMsg->StrStr("server_port=",line,seg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr+12,"%d-%d",&nSrvTransPort0,&nSrvTransPort1);
                        if (n != 2)
                        {
                            n = sscanf(pstr+12,"%d",&nSrvTransPort0);
                            if (n == 1)
                            {
                                nSrvTransPort1 = nSrvTransPort0 + 1;
                            }
                        }
                    }
                }
            }
        }

    }
    if(!m_bFirstSetup)
    {
        m_bFirstSetup |= 1;
        if (pSessionID == NULL || (m_strSessionID != NULL && strcmp(m_strSessionID ,pSessionID)))
        {
            NeedClose(ANTS_RTSP_ERROR_InvSessionID);
        }
        else if (m_strSessionID == NULL)
        {
            m_strSessionID = Ants_strndup(pSessionID,-1);
            if (m_strSessionID == NULL)
            {
                NeedClose(ANTS_RTSP_ERROR_InvSessionID);
            }
        }
    }

    if ((m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)&& m_nTalkTransPort != nTransPort0)
    {
        m_nTalkTransPort = nTransPort0;
    }
    if (nTransPort0 == -1 ||
        nTransPort1 == -1 ||
        (nTransPort0 & 1) ||
        (nTransPort1 != nTransPort0 + 1) ||
        (m_nMode != RTSPCLIENT_OPEN_MODE_MULTI && m_nTalkTransPort != nTransPort0))
    {
        NeedClose(ANTS_RTSP_ERROR_InvTransPort);
    }

    if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

    }
    else
    {
#if 0
        if (nSrvTransPort0 == -1 ||
            nSrvTransPort1 == -1 ||
            (nSrvTransPort0 & 1) ||
            (nSrvTransPort1 != nSrvTransPort0 + 1) )
        {
            NeedClose(ANTS_RTSP_ERROR_InvTransPort);
        }
        else
        {
            m_nTalkSrvTransPort = nSrvTransPort0;
        }
#else
        m_nTalkSrvTransPort = nSrvTransPort0;
#endif

    }





    return 0;
}

int CRtspClient::Play()
{
    int nlen,npos;
    int nSeq,nRet;
    char *buf=NULL;

    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    if(!m_bPlaying)
    {


        if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
        {

            //分配端口资源
            if (m_bVideoExist)
            {
	            nRet =  CreateVideo(m_nMode);
	            if (nRet < 0)
	            {
	                NeedClose(ANTS_RTSP_ERROR_ResError);
	                delete [] buf;
	                return -1;
	            }
            }

			if (m_bAudioExist)
			{
	            nRet =  CreateAudio(m_nMode);
	            if (nRet < 0)
	            {
	                //NeedClose(ANTS_RTSP_ERROR_ResError);
	                delete [] buf;
	                return -1;
	            }
			}
        }
        if (m_bVideoExist && m_pVideoSess != NULL)
        {
            m_pVideoSess->SetPayloadClockRate(m_nVideoPayloadClockRate);
            m_pVideoSess->SetRTSPStreamType(m_nVideoStreamType);

        }
        if (m_bAudioExist && m_pAudioSess != NULL)
        {
            m_pAudioSess->SetPayloadClockRate(m_nAudioPayloadClockRate);
            m_pAudioSess->SetRTSPStreamType(m_nAudioStreamType);

        }
        if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
            m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
        {

        }
        else
        {
            if (m_bVideoExist && m_pVideoSess != NULL)
            {
                m_pVideoSess->SetPayloadClockRate(m_nVideoPayloadClockRate);
                if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
                {
                    m_pVideoSess->SetDefaultPayloadType(m_nVideoPayloadType);
					unsigned int dwip[4] = {0,0,0,0};
					if (m_bSrvIPv6)
					{
						inet_pton(AF_INET6,m_pMulticastAddr[1],dwip);
					}
					else
					{
						inet_pton(AF_INET,m_pMulticastAddr[1],dwip);
					}

                    m_pVideoSess->JoinMulticastGroup(dwip,m_nMulticastPort[0]);
					m_pVideoSess->AddDestination(m_nSrvIPv4,0,m_nVideoSrvTransPort,0,0,this,m_bSrvIPv6);
                }
				else
				{
                     m_pVideoSess->AddDestination(m_nSrvIPv4,m_nVideoSrvTransPort,m_nVideoSrvTransPort + 1,0,0,this,m_bSrvIPv6);
				}
            }
            if (m_bAudioExist && m_pAudioSess != NULL)
            {
                m_pAudioSess->SetPayloadClockRate(m_nAudioPayloadClockRate);
                if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
                {
					unsigned int dwip[4] = {0,0,0,0};
					if (m_bSrvIPv6)
					{
						inet_pton(AF_INET6,m_pMulticastAddr[0],dwip);
					}
					else
					{
						inet_pton(AF_INET,m_pMulticastAddr[0],dwip);
					}
                    m_pAudioSess->SetDefaultPayloadType(m_nAudioPayloadType);
                    m_pAudioSess->JoinMulticastGroup(dwip,m_nMulticastPort[1]);
					m_pAudioSess->AddDestination(m_nSrvIPv4,0,m_nAudioSrvTransPort,0,0,this,m_bSrvIPv6);
                }
				else
				{
                    m_pAudioSess->AddDestination(m_nSrvIPv4,m_nAudioSrvTransPort,m_nAudioSrvTransPort+1,0,0,this,m_bSrvIPv6);
				}
            }
        }
    }
    else
    {
        m_nCurrOP = 'p';
    }

    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"PLAY %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "PLAY";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    if (m_bReplay)
    {
        if (m_bPlay != m_bPlay_Req)
        {
            m_bPlay = m_bPlay_Req;
        }
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: onvif-replay\r\n");
#if 1  //add by HG_Panda 2022-01-08 解决海康回放
    if( m_bSeek == m_bSeek_Req)
    {
         npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Range: npt=0.000-\r\n");
    }

#endif
        //if(memcmp(&m_tPlayStart,&m_tPlayStart_Req,sizeof(m_tPlayStart)) != 0)
        if( m_bSeek != m_bSeek_Req)
        {
             m_bSeek = m_bSeek_Req;
            m_tPlayStart = m_tPlayStart_Req;
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Range: clock=%04d%02d%02dT%02d%02d%02d.%03dZ-\r\n",
                m_tPlayStart.wYear,m_tPlayStart.byMon,m_tPlayStart.byDay,
                m_tPlayStart.byHour,m_tPlayStart.byMin,m_tPlayStart.bySec,m_tPlayStart.wMsec);
        }
        if (m_bRateControl != m_bRateControl_Req)
        {
            m_bRateControl = m_bRateControl_Req;
        }

        if(!m_bRateControl)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Rate-Control: no\r\n");
        }
        else
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Rate-Control: yes\r\n");
        }
        if (m_bOnlyIFrame_Req != m_bOnlyIFrame)
        {
            m_bOnlyIFrame = m_bOnlyIFrame_Req;

        }
        if (m_bOnlyIFrame)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Frames: intra\r\n");
        }
        else
        {
            //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Frames: all\r\n");
        }

        if(m_nScale_Req != m_nScale)
        {
            m_nScale = m_nScale_Req;

        }
        if (m_nScale)
        {
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Scale: %.01f\r\n",(float)m_nScale);
        }


    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    if (!m_bReadyForData)
    {
        m_bReadyForData = 1;
    }
    SendData(buf,npos);

    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    AddReqList(RTSP_CMD_TYPE_PLAY,nSeq,1);
    delete [] buf;
    return 0;
}

int CRtspClient::Play_handle(CMessage *pMsg)
{
    char *pSessionID = NULL;
    int line,seg;
    char *pstr;
    int bTimeout = 0;
    int nTransPort0 = -1,nTransPort1 = -1;
    int nSrvTransPort0 = -1,nSrvTransPort1 = -1;
    if(!m_bPlaying)
    {
        if (m_bTalkExist && m_pTalkSess != NULL)
        {
            m_pTalkSess->AddDestination(m_nSrvIPv4,0,0,1,m_nTalkTransPort,this,m_bSrvIPv6);
        }
        m_bPlaying = 1;
    }

    return 0;
}
int CRtspClient::Pause()
{
    int nlen,npos;
    int nSeq;
    char *buf = NULL;

    if (!m_bGet_Parameter)
    {
        return -1;
    }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"PAUSE %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Type: text/parameters\r\n");
    //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Length: 0\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "PAUSE";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    if (m_bReplay)
    {
        npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Require: onvif-replay\r\n");
        if(memcmp(&m_tPlayStart,&m_tPlayStart_Req,sizeof(m_tPlayStart)) != 0)
        {
            m_tPlayStart = m_tPlayStart_Req;
            npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Range: clock=%04d%02d%02dT%02d%02d%02d.%03dZ-\r\n",
                m_tPlayStart.wYear,m_tPlayStart.byMon,m_tPlayStart.byDay,
                m_tPlayStart.byHour,m_tPlayStart.byMin,m_tPlayStart.bySec,m_tPlayStart.wMsec);
        }

    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    m_nCurrOP = 'P';
    m_nWaitOPRes = 1;
    m_nWaitSeq = nSeq;
    AddReqList(RTSP_CMD_TYPE_SETUP,nSeq,1);
    delete [] buf;
    return 0;
}

int CRtspClient::Control(int nCmd,unsigned long dwApplyID,void *pInBuffer,int nInSize,void *pOutBuffer,int nOutSize)
{
    int nRet = 0;
    switch (nCmd)
    {
        case ANTS_RTSP_CONTROL_CMD_TALKDATA:
            {
                if (!m_bSetupTalk_Req)
                {
                    return -1;
                }
                if (m_pTalkSess == NULL)
                {
                    return -1;
                }
                nRet = m_pTalkSess->SendAudioPacket(pInBuffer,nInSize,0,0,0,0);
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_START:
            {
                m_bStart = 1;
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_PAUSE:
            {
                int bPause = 1;
                if (pInBuffer != NULL && nInSize >= sizeof(int))
                {
                    bPause = *(int *)pInBuffer;
                }
                m_bPauseReq = (bPause != 0);
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_PLAY:
            {
                int nSpeed = 0;
                if (pInBuffer != NULL && nInSize >= sizeof(int))
                {
                   nSpeed = *(int *)pInBuffer;
                }
#if 0
                if (nSpeed < -32)
                {
                    nSpeed = -32;
                }
                if (nSpeed > 32)
                {
                    nSpeed = 32;
                }
#endif
                m_nScale_Req = nSpeed;
                m_bPlay_Req = 1;
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_ONLYIFRAME:
            {
                int nIFrame = 0;
                if (pInBuffer != NULL && nInSize >= sizeof(int))
                {
                    nIFrame = *(int *)pInBuffer;
                }
                m_bOnlyIFrame_Req = nIFrame;
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_SEEK:
            {
                if (pInBuffer == NULL || nInSize < sizeof(Ants_RtspDayTime))
                {
                    nRet = -1;
                    break;
                }
                m_tPlayStart_Req = *(Ants_RtspDayTime *)pInBuffer;
                m_bSeek_Req ++;

                break;
            }
        case ANTS_RTSP_CONTROL_CMD_RATECONTROL:
            {
                int nRc = 0;
                if (pInBuffer != NULL && nInSize >= sizeof(int))
                {
                    nRc = *(int *)pInBuffer;
                }
                m_bRateControl_Req = (nRc != 0);
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_STEP:
            {
                m_bStep = 1;
                m_dwStepCnt_Req++;
                break;
            }
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
                if (m_nRcvBuffSize != nBufSize)
                {
                    m_nRcvBuffSize = nBufSize;
                }


                break;
            }
        case ANTS_RTSP_CONTROL_CMD_ANTSCOMB_ADDCH:
            {
                int nReq;
                int n,nNewIdx = -1,bExist = 0,bChg = 0,nUseCnt = 0;
                int *pInChan=(int *)pInBuffer;
                m_hAddDecChanLock.Lock();
                for (nReq = 0; nReq < nInSize && pInChan != NULL;nReq++)
                {
                    if (pInChan[nReq] != 0x80808080)
                    {
                        nNewIdx = -1;
                        bExist = 0;
                        nUseCnt = 0;
                        for (n = 0; n < RTSPCLIENT_CHAN_MAX; n++)
                        {
                            if (nUseCnt >= m_nCombChanReqCnt && nNewIdx != -1)
                            {
                                break;
                            }
                            if (m_nCombChanReq[n] != 0x80808080 )
                            {
                                nUseCnt++;
                                if (m_nCombChanReq[n] == pInChan[nReq])
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
                            m_nCombChanReq[nNewIdx] = pInChan[nReq];
                            m_nCombChanReqCnt++;
                            bChg = 1;
                        }
                    }

                }
                if (bChg)
                {
                    m_nCombChanAddReqCnt++;
                }
                m_hAddDecChanLock.Unlock();

                break;
            }
        case ANTS_RTSP_CONTROL_CMD_ANTSCOMB_DECCH:
            {
                int nReq;
                int n,bChg = 0,nUseCnt = 0;
                int *pInChan=(int *)pInBuffer;
                m_hAddDecChanLock.Lock();
                for (nReq = 0; nReq < nInSize && pInChan != NULL;nReq++)
                {
                    if (pInChan[nReq] != 0x80808080)
                    {
                        nUseCnt = 0;
                         for (n = 0; n < RTSPCLIENT_CHAN_MAX; n++)
                        {
                            if (nUseCnt >= m_nCombChanReqCnt)
                            {
                                break;
                            }
                            if (m_nCombChanReq[n] != 0x80808080 )
                            {
                                nUseCnt++;
                                if (m_nCombChanReq[n] == pInChan[nReq])
                                {
                                    // 存在
                                    m_nCombChanReq[n] = 0x80808080;
                                    m_nCombChanReqCnt--;
                                    bChg = 1;
                                    break;
                                }
                            }

                        }

                    }

                }
                if (bChg)
                {
                    m_nCombChanDecReqCnt++;
                }
                m_hAddDecChanLock.Unlock();
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_GETRTPPORT:
            {
                if (pOutBuffer == NULL || nOutSize < sizeof(int))
                {
                    nRet = -1;
                    break;
                }
                *(int *)pOutBuffer = m_nVideoTransPort;
				if(nOutSize >= sizeof(int) * 2)
				{
				 ((int *)pOutBuffer)[1] = m_nSecondVideoTransPort;
				}
                break;
            }

        case ANTS_RTSP_CONTROL_CMD_SETRTP:
            {
                if (pInBuffer != NULL && nInSize >= sizeof(ANTS_RTSP_RTPPARAM_T))
                {
                    ANTS_RTSP_RTPPARAM_T *pRtpParm = (ANTS_RTSP_RTPPARAM_T *)pInBuffer;
					//printf("[%s.%d]here   \n",__FUNCTION__,__LINE__);
					if(pRtpParm->dwServerIP != 0 && pRtpParm->dwServerIP != 0xFFFFFFFF)
					{
						char szIP[32],*pIp = NULL;
						sprintf(szIP,"%d.%d.%d.%d",(pRtpParm->dwServerIP >> 0) & 0xFF,(pRtpParm->dwServerIP >> 8) & 0xFF,(pRtpParm->dwServerIP >> 16) & 0xFF,(pRtpParm->dwServerIP >> 24) & 0xFF);
					//	printf("[%s.%d]ip......%s \n",__FUNCTION__,__LINE__,szIP);
						pIp = strdup(szIP);
						if (pIp != NULL)
						{
							char *szTmp = m_pRtp_ServerIP;
							m_pRtp_ServerIP = pIp;
							if(szTmp)
							{
								free(szTmp);
							}
						}

					}
					if(m_pVideoSess != NULL)
					{
					    if (pRtpParm->mPayloadType >= 0)
					    {
					        m_pVideoSess->SetDefaultPayloadType(pRtpParm->mPayloadType);
					    }
						if (pRtpParm->mSubPayLoadType > 0)
						{
							m_pVideoSess->SetSecondPayloadType(pRtpParm->mSubPayLoadType);
						}

						if (pRtpParm->mStreamType >= 0)
						{
					         m_pVideoSess->SetRTSPStreamType(pRtpParm->mStreamType);
						}
						if (pRtpParm->mSubStreamType >= 0)
						{
							m_pVideoSess->SetSecondStreamType(pRtpParm->mSubStreamType);
						}

						if (pRtpParm->mPayloadClockRate >= 0)
						{
					        m_pVideoSess->SetPayloadClockRate(pRtpParm->mPayloadClockRate);
						}

						if (pRtpParm->mSSRC != (unsigned int)-1)
						{
					        m_pVideoSess->SetSSRC(pRtpParm->mSSRC);
						}
						if (pRtpParm->mPort > 0)
						{
                            m_nRtp_ServerPort = pRtpParm->mPort;
							if (m_pRtp_ServerIP != NULL)
							{
                                m_pVideoSess->AddDestination(m_pRtp_ServerIP,m_nRtp_ServerPort,m_nRtp_ServerPort+1,0,m_nRtp_pType,0,this);
							}
						}

					}
					if (nInSize >= sizeof(ANTS_RTSP_RTPPARAM_T) * 2)
					{
						pRtpParm++;
						if(m_pSecondVideoSess != NULL)
						{
							if (pRtpParm->mPayloadType >= 0)
							{
								m_pSecondVideoSess->SetDefaultPayloadType(pRtpParm->mPayloadType);
							}
							if (pRtpParm->mSubPayLoadType > 0)
							{
								m_pSecondVideoSess->SetSecondPayloadType(pRtpParm->mSubPayLoadType);
							}

							if (pRtpParm->mStreamType >= 0)
							{
								m_pSecondVideoSess->SetRTSPStreamType(pRtpParm->mStreamType);
							}
							if (pRtpParm->mSubStreamType >= 0)
							{
								m_pSecondVideoSess->SetSecondStreamType(pRtpParm->mSubStreamType);
							}

							if (pRtpParm->mPayloadClockRate >= 0)
							{
								m_pSecondVideoSess->SetPayloadClockRate(pRtpParm->mPayloadClockRate);
							}

							if (pRtpParm->mSSRC != (unsigned int)-1)
							{
								m_pSecondVideoSess->SetSSRC(pRtpParm->mSSRC);
							}
							if (pRtpParm->mPort > 0)
							{
								m_nRtp_ServerSecondPort = pRtpParm->mPort;
								if (m_pRtp_ServerIP != NULL)
								{
									m_pSecondVideoSess->AddDestination(m_pRtp_ServerIP,m_nRtp_ServerSecondPort,m_nRtp_ServerSecondPort+1,0,m_nRtp_pType,0,this);
								}
							}
						}
						if(pRtpParm->dwServerIP != 0 && pRtpParm->dwServerIP != 0xFFFFFFFF)
						{
							char szIP[32],*pIp = NULL;
							sprintf(szIP,"%d.%d.%d.%d",(pRtpParm->dwServerIP >> 0) & 0xFF,(pRtpParm->dwServerIP >> 8) & 0xFF,(pRtpParm->dwServerIP >> 16) & 0xFF,(pRtpParm->dwServerIP >> 24) & 0xFF);
							pIp = strdup(szIP);
							if (pIp != NULL)
							{
								char *szTmp = m_pRtp_ServerIP;
								m_pRtp_ServerIP = pIp;
								if(szTmp)
								{
									free(szTmp);
								}

							}

						}
					}
                }
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_RTP_SEND_ANTS:
            {
                AntsFrameHeader *pFrame;
                nRet = -1;
                if (pInBuffer == NULL || nInSize < sizeof(AntsFrameHeader))
                {
                    break;
                }
                nRet = InputAntsData(pInBuffer,nInSize);
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_RTP_SEND:
            {
                nRet = -1;
                if (pInBuffer == NULL)
                {
                    break;
                }
                nRet = InputData(pInBuffer,nInSize);
                break;
            }
        case ANTS_RTSP_CONTROL_CMD_RTP_POLL:
            {
                if (!m_nUrlType)
                {
                    nRet = -1;

                    break;
                }

                PollData_RTP();
                if (pOutBuffer != NULL && nOutSize == sizeof(ANTS_RTSP_STATUS_T))
                {
                    ANTS_RTSP_STATUS_T *pStatus = (ANTS_RTSP_STATUS_T *)pOutBuffer;
                    memset(pStatus,0,sizeof(ANTS_RTSP_STATUS_T));
                    if (m_bRtp_extSockFD)
                    {
                        if (m_bNeedClose)
                        {
                            pStatus->nStatusType = ANTS_RTSP_STATUS_Disconnected;
                            pStatus->nStatusCode = m_nReason;
                        }
                    }
                }
                break;
            }
        default:
            {
                nRet = -1;
                break;
            }
    }
    return nRet;
}


int CRtspClient::InputAntsData(void *pData,int nDataSize)
{
    int nRet = -1;
    int i = 0;
    AntsFrameHeader framehead;
    int64_t timeStamp = 0;
    int nPlayloadType,bAudio = 0,nStreamType;
    int bRelTimeStamp = 0;

    if (pData == NULL)
    {
        return nRet;
    }
    //RTSP_DEBUG("[%s.%d]nDataSize = %d\n",__FUNCTION__,__LINE__,nDataSize);
    do
    {
        memcpy(&framehead,pData,sizeof(AntsFrameHeader));
        //framehead= (AntsFrameHeader *)pData;

        if (framehead.uiStartId != ANTS_FRAME_STARTCODE)
        {
            //RTSP_ERROR("[%s.%d]start id = 0x%x\n",__FUNCTION__,__LINE__,framehead.uiStartId);
            return nRet;
        }
        if (framehead.uiFrameLen > nDataSize - sizeof(AntsFrameHeader))
        {
            RTSP_ERROR("[%s.%d]uiFrameLen = %d nDataSize = %d ,%d\n",__FUNCTION__,__LINE__,framehead.uiFrameLen , nDataSize , sizeof(AntsFrameHeader));
            return nRet;
        }
        bRelTimeStamp |= 2;
        if (framehead.uiFrameType == AntsPktIFrames ||
            framehead.uiFrameType == AntsPktPFrames ||
            framehead.uiFrameType == AntsPktSubIFrames ||
            framehead.uiFrameType == AntsPktSubPFrames ||
            framehead.uiFrameType == AntsPktAudioFrames)
        {


            timeStamp = framehead.uiFrameTime * 1000 + framehead.uiFrameTickCount/1000;
            if (framehead.uiFrameType == AntsPktAudioFrames)
            {
                if (framehead.uMedia.struAudioHeader.cCodecId == 1)
                {
                    nStreamType = ANTS_RTSP_STREAM_G711A;
                }
                else if (framehead.uMedia.struAudioHeader.cCodecId == AntsADPCM)
                {

                }
                else
                {
                    nStreamType = ANTS_RTSP_STREAM_G711U;
                }
                bAudio = 1;
            }
            else
            {

                if (framehead.uMedia.struVideoHeader.cCodecId == AntsMJPEG_hisi)
                {
                    nStreamType = ANTS_RTSP_STREAM_JPEG;
                    bRelTimeStamp |= 1;
                }
                else if (framehead.uMedia.struVideoHeader.cCodecId == AntsH265)
                {
                    nStreamType = ANTS_RTSP_STREAM_H265;
                    bRelTimeStamp |= 1;
                }
                else if (framehead.uMedia.struVideoHeader.cCodecId == AntsH265_RTP_PACK)
                {
                    nStreamType = ANTS_RTSP_STREAM_H265;
                    bRelTimeStamp |= 1;
                    bRelTimeStamp |= (1 << 31);
                }
                else
                {
                    nStreamType = ANTS_RTSP_STREAM_H264;
                    if (framehead.uMedia.struVideoHeader.cCodecId == AntsH264_hisi_RTP)
                    {
                        bRelTimeStamp |= 1;
                    }
                    if (framehead.uMedia.struVideoHeader.cCodecId == AntsH264_RTP_PACK)
                    {
                        bRelTimeStamp |= 1;
                        bRelTimeStamp |= (1 << 31);
                    }

                }



            }

	        if (m_pVideoSess != NULL && m_pVideoSess->GetRTSPStreamType() == nStreamType)
	        {
	            nRet = m_pVideoSess->SendVideoPacket(framehead.uiFrameType,
	                (void *)((char *)pData + sizeof(AntsFrameHeader)) ,
	                framehead.uiFrameLen,
	                framehead.uiTimeStamp,
	                framehead.uiFrameTime,
	                framehead.uiFrameTickCount,bRelTimeStamp);
			}
			else if (m_pSecondVideoSess != NULL && m_pSecondVideoSess->GetRTSPStreamType() == nStreamType)
			{
				nRet = m_pSecondVideoSess->SendVideoPacket(framehead.uiFrameType,
					(void *)((char *)pData + sizeof(AntsFrameHeader)) ,
					framehead.uiFrameLen,
					framehead.uiTimeStamp,
					framehead.uiFrameTime,
					framehead.uiFrameTickCount,bRelTimeStamp);
			}
			else if (m_nRtp_PayloadNum > 0)
            {
                int nPayl;
                for (nPayl = 0; nPayl < m_nRtp_PayloadNum;nPayl++)
                {
                    if(m_nRtp_StreamType[nPayl] == nStreamType)
                    {
                        if (m_pRtp_Session[nPayl] != NULL)
                        {
                            nRet =m_pRtp_Session[nPayl]->SendVideoPacket(framehead.uiFrameType,
                                (void *)((char *)pData + sizeof(AntsFrameHeader)) ,
                                framehead.uiFrameLen,
                                framehead.uiTimeStamp,
                                framehead.uiFrameTime,
                                framehead.uiFrameTickCount,bRelTimeStamp);
                        }
                        break;
                    }
                }
            }
            else
            {
                break;
            }

            // checkerror(nRet);
        }
        pData = (char *)pData + framehead.uiFrameLen + sizeof(AntsFrameHeader);
        nDataSize -= framehead.uiFrameLen + sizeof(AntsFrameHeader);
        if (nDataSize <= 0)
        {
            break;
        }

    }
    while(1);

    return nRet;
}

int CRtspClient::InputData(void *pData,int nDataSize)
{
    int len = 0;
    int nDataLen = 0;
	unsigned short wLen;
	short nLen;
    char *pHead = NULL;

    m_nSendLock.Lock();

    if (m_nRtp_TcpSockFD < 0)
    {
        m_nSendLock.Unlock();
        return 0;
    }
    //RTSP_DEBUG("send = %d\n",nDataSize);
    wLen = htons(nDataSize);
    nDataLen = 2;
	pHead = (char *)&nLen;
    pHead[0] = wLen & 0xFF;
    pHead[1] = (wLen >> 8)&0xFF;
	len = htons(*((short *)pHead));
    //RTSP_DEBUG("len = %d\n",len);
    while (nDataLen > 0)
    {
		if (m_bNeedClose)
		{
			break;
		}
	    len = send(m_nRtp_TcpSockFD,pHead,nDataLen,0);
		//("send1 len = %d\n",len);
	    if (len > 0)
	    {
			nDataLen -= len;
			pHead += len;
		}
		else
	    {
	        int nError = GetLastError();
	        //RTSP_DEBUG("[%d]Exception %d %s\n",m_nRtp_TcpSockFD,errno,strerror(errno));
	        if(nError == EINTR || nError == EAGAIN || nError == EWOULDBLOCK || nError == EINPROGRESS)
	        {
	            Ants_WaitTime(0,10000);
				continue;
	        }
	        else
	        {
	            m_nSendLock.Unlock();
	            NeedClose(ANTS_RTSP_ERROR_SocketError);
	            return -1;
	        }
	    }
    }

    nDataLen = nDataSize;
    while (nDataLen > 0)
    {
		if (m_bNeedClose)
		{
			break;
		}
	    len = send(m_nRtp_TcpSockFD,(const char *)pData+nDataSize-nDataLen,nDataLen,0);
		//RTSP_DEBUG("send2 len = %d\n",len);
	    if (len > 0)
	    {
			nDataLen -= len;
			//pData += len;
		}
		else
	    {
	        int nError = GetLastError();

	        if(nError == EINTR || nError == EAGAIN || nError == EWOULDBLOCK || nError == EINPROGRESS)
	        {
	            Ants_WaitTime(0,10000);
				continue;
	        }
	        else
	        {
	            m_nSendLock.Unlock();
	            NeedClose(ANTS_RTSP_ERROR_SocketError);
	            return -1;
	        }
	    }
    }

    m_nSendLock.Unlock();
    return 0;
}


int CRtspClient::BufferMgrRead(int *nChan,int *nStream,int *nStreamType,unsigned char **lpBuffer,unsigned long *dwBufSize)
{
    int nCnt = 0;
    AntsRtspClient_BufferMgr_T *pMgr;
    if (lpBuffer == NULL || dwBufSize == NULL)
    {
        return -1;
    }
    m_hBuffMgrLock.Lock();
    if (m_pBuffMgrHead == NULL)
    {
        m_bReportBufferMgr = 1;
        m_hBuffMgrLock.Unlock();
        return 0;
    }
    pMgr = m_pBuffMgrHead;


    nCnt = m_nBufferMgrCnt;
    *lpBuffer = pMgr->pBuffer;
    *dwBufSize = pMgr->nBufferSize;
    if (nChan)
    {
        *nChan = pMgr->nCh;
    }
    if (nStream)
    {
        *nStream = pMgr->nStream;
    }
    m_hBuffMgrLock.Unlock();
    return nCnt;
}

int CRtspClient::BufferMgrReadRelease()
{
    int nCnt = 0;
    AntsRtspClient_BufferMgr_T *pMgr;

    m_hBuffMgrLock.Lock();
    if (m_pBuffMgrHead == NULL)
    {
        m_hBuffMgrLock.Unlock();
        return 0;
    }
    pMgr = m_pBuffMgrHead;
    m_pBuffMgrHead = m_pBuffMgrHead->pNext;
    if (m_pBuffMgrHead == NULL)
    {
        m_pBuffMgrTail = NULL;

    }
    m_nBufferMgrCnt--;
    m_nBufferMgrSize-=pMgr->nBufferSize;
    free(pMgr->pBuffer);
    delete pMgr;
    m_hBuffMgrLock.Unlock();
    return 0;
}
 int CRtspClient::Get_Parameter()
 {
     int nlen,npos;
     int nSeq;
     char *buf = NULL;

     if (!m_bGet_Parameter)
     {
         return -1;
     }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
     nSeq = GenerateSeq();
     npos = 0;
     npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"GET_PARAMETER %s RTSP/1.0\r\n",m_pUrl);
     npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
     npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
     //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Type: text/parameters\r\n");
     //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Length: 0\r\n");
     npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	 if (m_bNeedAuth)
	 {
		 if (m_bDigest)
		 {
			 HASHHEX Response,HA1;
			 char *cmd = "GET_PARAMETER";

			 // The "response" field is computed as:
			 //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			 // or, if "fPasswordIsMD5" is True:
			 //    md5(<password>:<nonce>:md5(<cmd>:<url>))
			 DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			 DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			 npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				 m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		 }
		 else if (m_pBase64Enc != NULL)
		 {
			 npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		 }
	 }

     npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
     if (npos > ANTS_RTSP_TMP_BUFSIZE)
     {
         RTSP_ERROR("size = %d\n",npos);
     }
     SendData(buf,npos);
       m_nCurrOP = 'g';
       AddReqList(RTSP_CMD_TYPE_GET_PARAMETER,nSeq,0);
     // m_nWaitOPRes = 1;
       delete [] buf;
     return 0;
 }
int CRtspClient::Set_Parameter()
{
    int nlen,npos;
    int nSeq;
    char *buf=NULL;

   if (!m_bSet_Parameter)
   {
       return -1;
   }
    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"SET_PARAMETER %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Type: text/parameters\r\n");
    //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Length: 0\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "SET_PARAMETER";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
     m_nCurrOP = 's';
     AddReqList(RTSP_CMD_TYPE_SET_PARAMETER,nSeq,0);
   // m_nWaitOPRes = 1;
     delete [] buf;
    return 0;
}

int CRtspClient::AntsComb_AddCh(int *pChan,int nCnt)
{
    int nlen,npos;
    int nSeq;
    char *buf=NULL;
    int i;
    if (pChan == NULL || nCnt < 1 || nCnt > RTSPCLIENT_CHAN_MAX)
    {
        return -1;
    }

    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"ANTSCOMB_ADDCH %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Channel-Number: ");
    if (nCnt > 0)
    {
        for (i = 0;i < nCnt;i++)
        {
            if (pChan[i])
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"%d;",pChan[i]);
            }
        }

    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    //npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Content-Length: 0\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "ANTSCOMB_ADDCH";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    m_nCurrOP = 'C';
    AddReqList(RTSP_CMD_TYPE_ANTSCOMB_ADDCH,nSeq,1);
    // m_nWaitOPRes = 1;
    delete [] buf;
    return 0;
}

int CRtspClient::AntsComb_DecCh(int *pChan,int nCnt)
{
    int nlen,npos;
    int nSeq;
    char *buf=NULL;
    int i;
    if (pChan == NULL || nCnt < 1 || nCnt > RTSPCLIENT_CHAN_MAX)
    {
        return -1;
    }

    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"ANTSCOMB_DECCH %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Channel-Number: ");
    if (nCnt > 0)
    {
        for (i = 0;i < nCnt;i++)
        {
            if (pChan[i])
            {
                npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"%d;",pChan[i]);
            }
        }

    }
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "ANTSCOMB_DECCH";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
    m_nCurrOP = 'D';
    AddReqList(RTSP_CMD_TYPE_ANTSCOMB_DECCH,nSeq,1);
    // m_nWaitOPRes = 1;
    delete [] buf;
    return 0;
}

int CRtspClient::TearDown()
{
    int nlen,npos;
    int nSeq;
    char *buf=NULL;

    buf = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buf == NULL)
    {
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return 0;
    }
    if (m_strSessionID == NULL)
    {
        delete [] buf;
        return 0;
    }
    nSeq = GenerateSeq();
    npos = 0;
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"TEARDOWN %s RTSP/1.0\r\n",m_pUrl);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"CSeq: %d\r\n",nSeq);
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"User-Agent: LibVLC/2.0.3 (LIVE555 Streaming Media v2011.12.23)\r\n");
    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Session: %s\r\n",m_strSessionID);
	if (m_bNeedAuth)
	{
		if (m_bDigest)
		{
			HASHHEX Response,HA1;
			char *cmd = "TEARDOWN";

			// The "response" field is computed as:
			//    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
			// or, if "fPasswordIsMD5" is True:
			//    md5(<password>:<nonce>:md5(<cmd>:<url>))
			DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
			DigestCalcResponse(HA1,m_pNonce,"","","",cmd,m_pUrl,"",Response);
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				m_pUserName,m_pRealm,m_pNonce,m_pUrl,Response);
		}
		else if (m_pBase64Enc != NULL)
		{
			npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"Authorization: Basic %s\r\n",m_pBase64Enc);
		}
	}

    npos += snprintf(buf + npos,ANTS_RTSP_TMP_BUFSIZE-npos,"\r\n");
    if (npos > ANTS_RTSP_TMP_BUFSIZE)
    {
        RTSP_ERROR("size = %d\n",npos);
    }
    SendData(buf,npos);
     m_nCurrOP = 't';
     AddReqList(RTSP_CMD_TYPE_TEARDOWN,nSeq,0);
    // m_nWaitOPRes = 1;
     delete []buf;
    return 0;
}
int CRtspClient::Option_handle(CMessage *pMsg)
{
    int line,seg;
    char *pstr = NULL;
    for (line = 0; line < pMsg->m_nLineCnt;line++)
    {
        if (!pMsg->StrCmp("Public:",line,0))
        {
            for (seg = 0; seg < pMsg->m_nSegCnt[line];seg++)
            {
                pstr = pMsg->StrStr("SET_PARAMETER",line,seg);
                if (pstr != NULL)
                {
                    m_bSet_Parameter = 1;
                    continue;
                }

                pstr = pMsg->StrStr("GET_PARAMETER",line,seg);
                if (pstr != NULL)
                {
                    m_bGet_Parameter = 1;
                    continue;
                }

            }

        }
    }
    return 0;
}
int CRtspClient::Dollar_handle(CMessage *pMsg)
{
    int nCh,len;

    char *pstr = pMsg->GetStr(0,0);
    if (pstr == NULL || !pMsg->m_bDollar)
    {
        return 0;
    }
    m_nRecvLock.Lock();
    nCh = pMsg->m_nDollar_ch;//pstr[1];
    len = pMsg->m_nMsgLen;//*((short *)(pstr + 2));
     PushRTPorRTCP(nCh,pstr,len);
     m_nRecvLock.Unlock();

    return 1;
}
void CRtspClient::PushRTPorRTCP(int ch,void *pData,int len)
{
    if (pData == NULL)
    {
        return ;
    }
  //  printf("len = %d\n",len);
    if (m_nMode == RTSPCLIENT_OPEN_MODE_TCP || m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
    {

        if (ch == m_nVideoTransPort)
        {
            if (m_pVideoSess != NULL)
            {

                if(-1 == m_pVideoSess->DealRecvPacket((unsigned char *)pData,len))
                {
                  //  printf("client = %d ip = %x/%x,port = %d/%d\n",(uint32_t)this,m_nLocalIPv4,m_nSrvIPv4,m_nLocalPort,m_nSrvPort);
                }
            }



        }
        else if (ch == m_nAudioTransPort)
        {
            if(m_pAudioSess != NULL)
            {
                m_pAudioSess->DealRecvPacket((unsigned char *)pData,len);
            }


        }
        else if (ch == m_nAppTransPort)
        {
            if (m_pAppSess != NULL)
            {
                 m_pAppSess->DealRecvPacket((unsigned char *)pData,len);
            }


        }
        else if (ch == m_nTalkTransPort)
        {
            if (m_pTalkSess != NULL)
            {
                m_pTalkSess->DealRecvPacket((unsigned char *)pData,len);
            }


        }


    }




}
int CRtspClient::SendData(const void *pData,size_t len)
{
    m_nSendLock.Lock();
   if (m_nSendBuffPos + len >= RTSPCLIENT_SEND_BUFFSIZE)
   {
       m_nSendLock.Unlock();
       return -1;
   }
   //if(((char *)pData)[0] != '$'){((char*)pData)[len] = 0;RTSP_DEBUG("S:[%s]\n",pData);}
   memcpy(m_pSendBuff + m_nSendBuffPos,pData,len);
   m_nSendBuffPos += len;
   DealSend();
   m_nSendLock.Unlock();
   return 0;
}
int CRtspClient::SendRTP_RTCPData(int nCh,const void *pData,size_t len)
{
    DOLLAR_HEAER_T dollar;
    dollar.cDollar = '$';
    dollar.nCH = nCh & 0xFF;
    dollar.nLen = htons(len);
    m_nSendLock.Lock();
	if (m_bRtp_DollarFlag == 0)
	{
       SendData(&dollar,4);
	}
	else
	{
	   short nLen;
	   nLen = htons(len);
	   SendData(&nLen,2);
	}
       SendData(pData,len);

    DealSend();
     m_nSendLock.Unlock();

      RTSP_DEBUG("[%s.%d] [%08X] rtcp send\n",__FUNCTION__,__LINE__,m_nClientHandle);
       return 0;
}
int CRtspClient::DealSend()
{
    int len;
    m_nSendLock.Lock();
    if (m_nSendBuffPos <= 0)
    {
        m_nSendLock.Unlock();
        return 0;
    }
    if (m_nSocket < 0)
    {
        m_nSendLock.Unlock();
        return 0;
    }
   // printf("send = %d\n",m_nSendBuffPos);
   //add by HG_Panda  异常时不发送信号 避免崩溃
    len = send(m_nSocket,m_pSendBuff,m_nSendBuffPos,MSG_NOSIGNAL);
    if (len < 0)
    {
        int nError = GetLastError();

        if(nError == EINTR || nError == EAGAIN || nError == EWOULDBLOCK || nError == EINPROGRESS)
        {
			len = 0;
        }
        else
        {
            m_nSendLock.Unlock();
            NeedClose(ANTS_RTSP_ERROR_SocketError);
            return -1;
        }

    }
    if (len == m_nSendBuffPos)
    {
        m_nSendBuffPos = 0;
    }
    else if(len > 0)
    {
        m_nSendBuffPos -= len;
        memmove(m_pSendBuff,m_pSendBuff + len,m_nSendBuffPos);
    }
    m_nSendLock.Unlock();
    return 0;
}
int CRtspClient::DealRecv()
{
    int bNeedMore = 0;
    CMessage *pmsg = NULL,*p = NULL;


#ifdef NO_BLOCK_OP
    bNeedMore = RecvAndParse_noblock();
#else
    bNeedMore = RecvAndParse();
#endif
    if (bNeedMore)
    {
        return 0;
    }
    if (m_nRecvBuffPos > 0 && m_nRecvBuffPos < RTSPCLIENT_RECV_BUFFSIZE)
    {
        m_pRecvBuff[m_nRecvBuffPos] = 0;
    }

    pmsg = Parse(m_pRecvBuff,m_nRecvBuffPos,0);
    if (pmsg != NULL)
    {
        if (m_pMsg != NULL)
        {
            p = m_pMsg;
            while(1)
            {
                if (p->m_pNext == NULL)
                {
                    break;
                }
                p = p->m_pNext;
            }
            p->m_pNext = pmsg;
        }
        else
        {
            m_pMsg = pmsg;
        }
    }
    m_nRecvBuffPos = 0;

    return 0;
}
int CRtspClient::DealCmd()
{
    CMessage *pMsg = NULL,*pExtMsg = NULL;
    int nRet = 0,nSeq,nResNum;
    int line = 0;
    int bAudio = 0;
    unsigned short wSeq;
    char *pstr = NULL;
    AntsRtspClient_Req_List_T *pReq = NULL,*pDel = NULL;
    int bIgnoring = 0;
    while((pMsg = GetMessage()) != NULL)
    {
        if (pMsg->IsDollar())
        {
            Dollar_handle(pMsg);
        }
        else if (m_nUrlType == 1)
        {// RTP 地址，可以在此将消息回调出去。一般TCP时有消息
            m_fCallback_V2(m_nClientHandle,ANTS_RTSP_DATATYPE_RTP_CONTROL,0,(unsigned char *)pMsg->m_pMessage,pMsg->m_nMsgLen,m_pCallbackUserData);
        }
        else
        {


                nSeq = pMsg->GetSeqNum();
                m_hReqListLock.Lock();
                pReq = m_pReqList;
                bIgnoring = 1;

                while (pReq != NULL)
                {
                    if (pReq->nSeq == nSeq)
                    {
                        // 摘除
                        m_pReqList = pReq->pNext;
                        if (m_pReqList == NULL)
                        {
                            m_pReqList_tail = NULL;
                        }
                         m_nReqListCnt--;
                        break;
                    }
                    else
                    {
                        if(pReq->bRes)
                        {
                            //
                            pReq = NULL;
                            bIgnoring = 0;
                            break;
                        }
                        pDel = pReq;
                        pReq = pReq->pNext;
                        m_pReqList = pReq;
                        if (m_pReqList == NULL)
                        {
                            m_pReqList_tail = NULL;
                        }
                        delete pDel;
                        m_nReqListCnt--;
                        continue;


                    }
                    pReq = pReq->pNext;
                }
                m_hReqListLock.Unlock();

                if (pReq == NULL)
                {
                    if (!bIgnoring)
                    {
                        NeedClose(ANTS_RTSP_ERROR_InvSeqNo);
                        Error_handle(pMsg);
                    }

                    delete pMsg;
                    continue;
                }
                if (pReq->bRes &&
                    m_nWaitOPRes &&
                    m_nWaitSeq == pReq->nSeq)
                {
                    m_nWaitOPRes = 0;
                }

                nResNum = pMsg->GetResNum();
                if (nResNum != 200)
                {
                    Error_handle(pMsg,pReq);
                }
                else
                {
                    if (pReq->nCmd == RTSP_CMD_TYPE_PLAY)
                    {
                         Play_handle(pMsg);

                    }
                    else if (pReq->nCmd == RTSP_CMD_TYPE_OPTION)
                    {
                        Option_handle(pMsg);
                    }
                    else if (pReq->nCmd == RTSP_CMD_TYPE_DESCRIBE)
                    {
                        Describe_handle(pMsg);
                    }

                    else if (pReq->nCmd == RTSP_CMD_TYPE_SETUP_V)
                    {
                        SetupV_handle(pMsg);
                    }
                    else if (pReq->nCmd == RTSP_CMD_TYPE_SETUP_A)
                    {
                        SetupA_handle(pMsg);
                    }
                    else if (pReq->nCmd == RTSP_CMD_TYPE_SETUP_APP)
                    {
                        SetupApp_handle(pMsg);
                    }
                    else if (pReq->nCmd == RTSP_CMD_TYPE_SETUP_TALK)
                    {
                        SetupTalk_handle(pMsg);
                    }
                    else
                    {

                    }


                }
                delete pReq;
                pReq = NULL;
        }

        delete pMsg;
    }
    return 0;
}
const char *CRtspClient::GetTstConfig(int *pVideoType,int *pAudioType,int *pWidth,int *pHeight,int *pFrameFrate)
{
    if (pVideoType)
    {
        *pVideoType = m_nVideoStreamType;
    }
    if (pAudioType)
    {
        *pAudioType = m_nAudioStreamType;
    }
    if (pWidth)
    {
        *pWidth = m_nTstWidth;
    }
    if (pHeight)
    {
        *pHeight = m_nTstHeight;
    }
    if (pFrameFrate)
    {
        *pFrameFrate = m_nTstFrameRate;
    }
    return m_pTstConfig;
}
int CRtspClient::Describe_handle(CMessage *pMsg)
{

    CMessage *psdp = NULL,*ptmp = NULL,*pExtMsg = NULL;
    int line;
    char *pstr;
    int bMediaType = 0;// 1 -video ,2 - audio,
    int bVideoRtpmap = 0,bAudioRtpmap = 0,bAppRtpmap = 0;
    int bMediaVideo = 0;

    int nAudioPayloadType = -1;
    int bSendOnlyA = 0;
    int nMulticastPort_A=0;

    int nVideoPayloadType = -1;
    int bSendOnlyV = 0;
    int nMulticastPort_V=0;
    int nVideoPayloadClockRate = 0,nAudioPayloadClockRate = 0;
    int nVideoStreamType = -1,nAudioStreamType = -1;
    char *psprop_parameter_sets_sps = NULL ,*psprop_parameter_sets_pps = NULL,*psprop_parameter_sets_vps = NULL;
    char *pAudio_control = NULL,*pVideo_control = NULL;
    char *pMulticastAddr = NULL;
    int nTTL = 0;
    //describe
        if (!pMsg->IsExtraData())
        {
            NeedClose(ANTS_RTSP_ERROR_InvSdpParam);
        }
        else
        {
            for (line = 0; line < pMsg->m_nLineCnt;line++)
            {
                pstr = pMsg->StrStr("Content-Base:",line,0);
                if (pstr != NULL)
                {
                    Ants_strFree(m_pBaseUrl);

                   m_pBaseUrl = Ants_strndup(pstr+14,-1);
                   if (m_pBaseUrl != NULL)
                   {
                       int len;
                       int i;
                       len = strlen(m_pBaseUrl);
                       for (i = len-1;i >= 0;i--)
                       {
                           if (m_pBaseUrl[i] == '/' ||
                               m_pBaseUrl[i] == ' ' ||
                               m_pBaseUrl[i] == '\\'||
                               m_pBaseUrl[i] == 0)
                           {
                               m_pBaseUrl[i] = 0;
                           }
                           else
                           {
                               break;
                           }
                       }
                       if (i  <= 0)
                       {
                           Ants_strFree(m_pBaseUrl);
                           m_pBaseUrl = NULL;
                       }
                   }
                }
            }
            pExtMsg = Parse((char *)pMsg->m_pContext,pMsg->m_nContextLength,1);
            if (pExtMsg == NULL)
            {
                NeedClose(ANTS_RTSP_ERROR_ParseError);
            }
            else
            {
                psdp = pExtMsg;
                pExtMsg = pExtMsg->m_pNext;
                while(pExtMsg != NULL)
                {
                    ptmp = pExtMsg;
                    pExtMsg = pExtMsg->m_pNext;
                    delete ptmp;
                }
                if (psdp->StrCmp("v=0",0,0))
                {
                    NeedClose(ANTS_RTSP_ERROR_InvSdpParam);
                }
                else
                {

                    for (line = 0; line < psdp->m_nLineCnt;line++)
                    {
                        pstr = psdp->GetStr(line,0);
                        if (pstr != NULL)
                        {
                            if (!strnicmp("a=sendonly",pstr,strlen("a=sendonly")))
                            {
                                if (bMediaType == 2)
                                {
                                    bSendOnlyA = 1;
                                }
                                else if (bMediaType == 1)
                                {
                                    bSendOnlyV = 1;
                                }
                            }
                            else if (!strnicmp("a=recvonly",pstr,strlen("a=recvonly")))
                            {
                            }
                            else if (!strnicmp("m=",pstr,2))
                            {
                                if (nAudioPayloadType != -1)
                                {

                                    if (bSendOnlyA)
                                    {
                                        m_nTalkPayloadType = nAudioPayloadType;
                                        if(m_pTalk_control != NULL)
                                        {
                                            Ants_strFree(m_pTalk_control);
                                            m_pTalk_control = NULL;
                                        }
                                        m_pTalk_control = pAudio_control;
                                        pAudio_control = NULL;
                                        if (m_pTalkSess != NULL)
                                        {
                                            m_pTalkSess->SetDefaultPayloadType(m_nTalkPayloadType);
                                        }

										if (m_nTalkPayloadType != 0 &&
                                            m_nTalkPayloadType != 8)
                                        {
                                            //NeedClose();
                                            m_bTalkExist = 0;//不支持
                                        }
                                        else
                                        {
                                            if (m_nTalkPayloadType == 0)
                                            {
                                                m_nTalkStreamType = ANTS_RTSP_STREAM_G711U;
                                            }
                                            else
                                            {
                                                m_nTalkStreamType = ANTS_RTSP_STREAM_G711A;
                                            }
                                            m_nTalkPayloadClockRate = nAudioPayloadClockRate;
                                            if(m_nTalkPayloadClockRate == 0)
                                            {
                                                m_nTalkPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nTalkPayloadType);
                                            }
                                            m_bTalkExist = 1;
                                            if (m_nMode == 2)
                                            {// 多播

                                                //m_nMulticastPort[1] = nMulticastPort_A;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if ( bMediaVideo == 0)
                                        {
                                            m_bAudioOrder = 1;
                                        }
                                        if(m_pAudio_control != NULL)
                                        {
                                            Ants_strFree(m_pAudio_control);
                                            m_pAudio_control = NULL;
                                        }
                                        m_pAudio_control = pAudio_control;
                                        pAudio_control = NULL;

                                        if (m_pMulticastAddr[0] != NULL)
                                        {
                                            Ants_strFree(m_pMulticastAddr[0]);
                                            m_pMulticastAddr[0] = NULL;
                                        }
                                        m_pMulticastAddr[0] = pMulticastAddr;
                                        m_nMulticastTTL[0] = nTTL;
                                        pMulticastAddr = NULL;


                                        m_nAudioPayloadType = nAudioPayloadType;
                                        m_nAudioPayloadClockRate = nAudioPayloadClockRate;
                                        if(m_nAudioPayloadClockRate == 0)
                                        {
                                            m_nAudioPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nAudioPayloadType);
                                        }

                                        if (m_pAudioSess != NULL)
                                        {
                                            m_pAudioSess->SetDefaultPayloadType(m_nAudioPayloadType);
                                        }

										if (m_nAudioPayloadType != 0 &&
                                            m_nAudioPayloadType != 8)
                                        {
                                            //NeedClose();
                                            m_bAudioExist = 0;//不支持
                                        }
                                        else
                                        {
                                            if (m_nAudioPayloadType == 0)
                                            {
                                                m_nAudioStreamType = ANTS_RTSP_STREAM_G711U;
                                            }
                                            else
                                            {
                                                m_nAudioStreamType = ANTS_RTSP_STREAM_G711A;
                                            }
                                            m_bAudioExist = 1;
                                            if (m_nMode == 2)
                                            {// 多播

                                                m_nMulticastPort[1] = nMulticastPort_A;
                                            }
                                        }
                                    }
                                    nAudioPayloadType = -1;
                                    nMulticastPort_A = 0;
                                    bSendOnlyA = 0;
                                }
                                if (nVideoPayloadType != -1)
                                {
                                    if(m_pVideo_control != NULL)
                                    {
                                        Ants_strFree(m_pVideo_control);
                                        m_pVideo_control = NULL;
                                    }
                                    m_pVideo_control = pVideo_control;
                                    pVideo_control = NULL;

                                    if (m_pMulticastAddr[1] != NULL)
                                    {
                                        Ants_strFree(m_pMulticastAddr[1]);
                                        m_pMulticastAddr[1] = NULL;
                                    }
                                    m_pMulticastAddr[1] = pMulticastAddr;
                                    m_nMulticastTTL[1] = nTTL;
                                    pMulticastAddr = NULL;

                                    m_nVideoPayloadType = nVideoPayloadType;
                                    m_nVideoPayloadClockRate = nVideoPayloadClockRate;
                                    if(m_nVideoPayloadClockRate == 0)
                                    {
                                        m_nVideoPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nVideoPayloadType);
                                    }
                                    bMediaVideo = 1;
                                    if (nVideoStreamType == -1)
                                    {
                                        if (nVideoPayloadType == 26)
                                        {
                                             nVideoStreamType = ANTS_RTSP_STREAM_JPEG;
                                        }
                                    }
                                    m_nVideoStreamType = (ANTS_RTSP_STREAM_TYPE)nVideoStreamType;
                                    if (m_pVideoSess != NULL)
                                    {
                                        char *pdec;
                                        unsigned int declen = 0;
                                        m_pVideoSess->SetDefaultPayloadType(m_nVideoPayloadType);
                                        if (psprop_parameter_sets_sps != NULL)
                                        {
                                            pdec = Ants_Rtsp_Base64Decode(psprop_parameter_sets_sps,strlen(psprop_parameter_sets_sps),&declen);
                                            if (pdec != NULL)
                                            {
                                                if (m_pVideoSess != NULL)
                                                {
                                                    m_pVideoSess->SetSPS(pdec,declen);
                                                }
                                                delete []pdec;
                                            }
                                            Ants_strFree(psprop_parameter_sets_sps);
                                            psprop_parameter_sets_sps = NULL;
                                        }
                                        if (psprop_parameter_sets_pps != NULL)
                                        {
                                            declen = 0;
                                            pdec = Ants_Rtsp_Base64Decode(psprop_parameter_sets_pps,strlen(psprop_parameter_sets_pps),&declen);
                                            if (pdec != NULL)
                                            {
                                                if (m_pVideoSess != NULL)
                                                {
                                                    m_pVideoSess->SetPPS(pdec,declen);
                                                }
                                                delete []pdec;
                                            }
                                            Ants_strFree(psprop_parameter_sets_pps);
                                            psprop_parameter_sets_pps = NULL;
                                        }
                                        if (psprop_parameter_sets_vps != NULL)
                                        {
                                            declen = 0;
                                            pdec = Ants_Rtsp_Base64Decode(psprop_parameter_sets_vps,strlen(psprop_parameter_sets_vps),&declen);
                                            if (pdec != NULL)
                                            {
                                                if (m_pVideoSess != NULL)
                                                {
                                                    m_pVideoSess->SetVPS(pdec,declen);
                                                }
                                                delete []pdec;
                                            }
                                            Ants_strFree(psprop_parameter_sets_vps);
                                            psprop_parameter_sets_vps = NULL;
                                        }
                                    }



                                    m_bVideoExist = 1;
                                    if (m_nMode == 2)
                                    {// 多播

                                        m_nMulticastPort[0] = nMulticastPort_V;
                                    }

                                    nVideoPayloadType = -1;
                                    nMulticastPort_V = 0;
                                    bSendOnlyV = 0;
                                }
                                if (!strnicmp("m=video",pstr,7))
                                {
                                    bMediaType = 1;
                                    pstr = psdp->GetStr(line,3);
                                    if (pstr != NULL)
                                    {
                                        nVideoPayloadType = atoi(pstr);



                                        if (m_nMode == 2)
                                        {// 多播
                                            pstr = psdp->GetStr(line,1);
                                            if(pstr != NULL)
                                            {
                                                nMulticastPort_V = atoi(pstr) ;
                                            }
                                        }
                                    }



                                }
                                else if (!strnicmp("m=audio",pstr,7))
                                {


                                    bMediaType = 2;
                                    pstr = psdp->GetStr(line,3);
                                    if (pstr != NULL)
                                    {
                                        nAudioPayloadType = atoi(pstr);


                                         if (m_nMode == 2)
                                         {// 多播
                                             pstr = psdp->GetStr(line,1);
                                             if(pstr != NULL)
                                             {
                                                 nMulticastPort_A = atoi(pstr) ;
                                             }
                                         }


                                    }

                                }
                                else if (!strnicmp("m=application",pstr,13))
                                {

                                    bMediaType = 3;
                                    pstr = psdp->GetStr(line,3);
                                    if (pstr != NULL)
                                    {
                                        m_nAppPayloadType = atoi(pstr);
                                        if (m_pAppSess != NULL)
                                        {
                                            m_pAppSess->SetDefaultPayloadType(m_nAppPayloadType);
                                        }



                                            m_bAppExist = 1;
                                            if (m_nMode == 2)
                                            {// 多播
                                                pstr = psdp->GetStr(line,1);
                                                if(pstr != NULL)
                                                {
                                                    m_nMulticastPort[2] = atoi(pstr) ;
                                                }
                                            }


                                    }
                                    else
                                    {
                                        NeedClose(ANTS_RTSP_ERROR_SDP_PlayloadType);
                                    }
                                }
                                else
                                {
                                    bMediaType = 5;//other
                                }
                            }
                            else if (!strnicmp("c=IN",pstr,4))
                            {

                                if (m_nMode == 2)
                                {// 多播
                                    pstr = psdp->GetStr(line,1);
                                    if (pstr != NULL && (!strnicmp("IP4",pstr,3)))
                                    {
                                        pstr = psdp->GetStr(line,2);
                                        if(pstr)
                                        {
                                            char *pstr1;
                                            pstr1 = strchr(pstr,'/');


                                                if(pstr1)
                                                {
                                                    char *pstr2;
                                                    pstr2 = strchr(pstr1+1,'/');
                                                    pMulticastAddr = Ants_strndup(pstr,pstr1-pstr);
                                                    if (pstr2)
                                                    {
                                                       nTTL= atoi(pstr1+1);
                                                    }
                                                    else
                                                    {
                                                        nTTL = 255;
                                                    }
                                                }
                                                else
                                                {
                                                    pMulticastAddr = Ants_strndup(pstr,-1);
                                                    nTTL = 255;
                                                }




                                        }

                                    }
                                }


                            }
                            else if (!strnicmp("a=control",pstr,9))
                            {

                                if (bMediaType == 2)
                                {
                                    pAudio_control = Ants_strndup(pstr+10,-1);
                                }
                                else if(bMediaType == 1)
                                {
                                   pVideo_control = Ants_strndup(pstr+10,-1);
                                }
                                else if(bMediaType == 3)
                                {
                                    if (m_pApp_control != NULL)
                                    {
                                        Ants_strFree(m_pApp_control);
                                        m_pApp_control = NULL;
                                    }
                                    m_pApp_control = Ants_strndup(pstr+10,-1);
                                }
                            }
                            else if (!strnicmp("a=fmtp",pstr,6))
                            {// "a=fmtp:96 packetization-mode=1; profile-level-id=420029; sprop-parameter-sets=Z0IAKeKQCgDLYC3AQEBpB4kRUA==,aM48gA=="
                                int rtptype;
                                rtptype = atoi(pstr+7);

                                    if (rtptype == nVideoPayloadType)
                                    {
                                        int seg;
                                        for (seg = 1; seg < psdp->m_nSegCnt[line]; seg++)
                                        {
                                            pstr = psdp->StrStr("sprop-parameter-sets",line,seg);
                                            if(NULL != pstr)
                                            {
                                                int pos ;
                                                char *pstr1,*pstr2;
                                                char *pdec;
                                                unsigned int declen = 0;
                                                pstr1 = strchr(pstr+21,',');

                                                if (pstr1 != NULL)
                                                {
                                                    if (Ants_Rtsp_IsBase64(pstr+21,pstr1 - pstr - 21))
                                                    {
                                                        psprop_parameter_sets_sps = Ants_strndup(pstr+21,pstr1 - pstr - 21);
                                                    }
                                                    pstr2 = strchr(pstr1 + 1,';');
                                                    if (Ants_Rtsp_IsBase64(pstr1 + 1,pstr2 - pstr1 - 1))
                                                    {
                                                        psprop_parameter_sets_pps = Ants_strndup(pstr1 + 1,pstr2 - pstr1 - 1);
                                                    }
                                                    if (pstr2 != NULL)
                                                    {
                                                        pstr2 = (char *)strstri(pstr2,";config=");
                                                        if (pstr2 != NULL)
                                                        {
                                                            pstr1 = strchr(pstr2 + 1,';');
                                                            if (m_pTstConfig != NULL)
                                                            {
                                                                Ants_strFree(m_pTstConfig);
                                                                m_pTstConfig = NULL;
                                                            }
                                                            m_pTstConfig = Ants_strndup(pstr2 + 8,pstr1 - pstr2 - 8);
                                                        }
                                                    }

                                                }
                                                else
                                                {
                                                    if (Ants_Rtsp_IsBase64(pstr+21,-1))
                                                    {

                                                        psprop_parameter_sets_sps = Ants_strndup(pstr+21,-1);

                                                    }

                                                    RTSP_DEBUG("[%08X]pps == NULL [%s]\n",m_nClientHandle,pstr == NULL?"nul":pstr);
                                                }





                                                break;

                                            }
                                            else
                                            {
                                                pstr = psdp->StrStr("sprop-vps",line,seg);
                                                if(NULL != pstr)
                                                {
                                                    int pos ;
                                                    char *pstr1,*pstr2;
                                                    char *pdec;
                                                    unsigned int declen = 0;
                                                    pstr1 = strchr(pstr+10,';');

                                                    if (pstr1 != NULL)
                                                    {
                                                        if (Ants_Rtsp_IsBase64(pstr+10,pstr1 - pstr - 10))
                                                        {
                                                            psprop_parameter_sets_vps = Ants_strndup(pstr+10,pstr1 - pstr - 10);
                                                        }

                                                    }
                                                    else if (Ants_Rtsp_IsBase64(pstr+10,-1))
                                                    {

                                                        psprop_parameter_sets_vps = Ants_strndup(pstr+10,-1);

                                                    }

                                                    RTSP_DEBUG("[%08X]pps == NULL [%s]\n",m_nClientHandle,pstr == NULL?"nul":pstr);


                                                }
                                                pstr = psdp->StrStr("sprop-pps",line,seg);
                                                if(NULL != pstr)
                                                {
                                                    int pos ;
                                                    char *pstr1,*pstr2;
                                                    char *pdec;
                                                    unsigned int declen = 0;
                                                    pstr1 = strchr(pstr+10,';');


                                                    if (pstr1 != NULL)
                                                    {
                                                        if (Ants_Rtsp_IsBase64(pstr+10,pstr1 - pstr - 10))
                                                        {
                                                            psprop_parameter_sets_pps = Ants_strndup(pstr+10,pstr1 - pstr - 10);
                                                        }

                                                    }
                                                    else if (Ants_Rtsp_IsBase64(pstr+10,-1))
                                                    {

                                                        psprop_parameter_sets_pps = Ants_strndup(pstr+10,-1);

                                                    }

                                                    RTSP_DEBUG("[%08X]pps == NULL [%s]\n",m_nClientHandle,pstr == NULL?"nul":pstr);


                                                }
                                                pstr = psdp->StrStr("sprop-sps",line,seg);
                                                if(NULL != pstr)
                                                {
                                                    int pos ;
                                                    char *pstr1,*pstr2;
                                                    char *pdec;
                                                    unsigned int declen = 0;
                                                    pstr1 = strchr(pstr+10,';');


                                                    if (pstr1 != NULL)
                                                    {
                                                        if (Ants_Rtsp_IsBase64(pstr+10,pstr1 - pstr - 10))
                                                        {
                                                            psprop_parameter_sets_sps = Ants_strndup(pstr+10,pstr1 - pstr - 10);
                                                        }

                                                    }
                                                    else if (Ants_Rtsp_IsBase64(pstr+10,-1))
                                                    {

                                                        psprop_parameter_sets_sps = Ants_strndup(pstr+10,-1);

                                                    }

                                                    RTSP_DEBUG("[%08X]spps == NULL [%s]\n",m_nClientHandle,pstr == NULL?"nul":pstr);


                                                }
                                            }

                                        }


                                    }

                            }
                            else if(!strnicmp("a=x-dimensions:",pstr,sizeof("a=x-dimensions:")))
                            {
                                char *pstr1,*pstr2;
                                int len;
                                len = strlen(pstr);
                                pstr1 = pstr + len + 1;
                                m_nTstWidth = atoi(pstr1);
                                len = strlen(pstr1);
                                pstr2 = pstr1 + len + 1;
                                m_nTstHeight = atoi(pstr2);
                            }
                            else if(!strnicmp("a=x-framerate:",pstr,sizeof("a=x-framerate:")))
                            {
                                char *pstr1;
                                int len;
                                len = strlen(pstr);
                                pstr1 = pstr + len + 1;
                                m_nTstFrameRate = atoi(pstr1);


                            }
                            else if (!strnicmp("a=rtpmap",pstr,8))
                            {
                                int rtptype;
                                int n;

                                rtptype = atoi(pstr+9);

                                    if (rtptype == nVideoPayloadType)
                                    {
                                        if(NULL != (pstr = psdp->StrStr("H264",line,1)))
                                        {
                                            nVideoStreamType = ANTS_RTSP_STREAM_H264;

                                            n = sscanf(pstr+4,"/%d",&nVideoPayloadClockRate);



                                        }
                                        else if(NULL != (pstr = psdp->StrStr("H265",line,1)))
                                        {
                                            nVideoStreamType = ANTS_RTSP_STREAM_H265;

                                            n = sscanf(pstr+4,"/%d",&nVideoPayloadClockRate);



                                        }
                                        else if(NULL != (pstr = psdp->StrStr("JPEG",line,1)))
                                        {
                                            nVideoStreamType = ANTS_RTSP_STREAM_JPEG;
                                            n = sscanf(pstr+4,"/%d",&nVideoPayloadClockRate);



                                        }
                                        else if(NULL != (pstr = psdp->StrStr("AntsComb",line,1)))
                                        {
                                            nVideoStreamType = ANTS_RTSP_STREAM_AntsComb;
                                            n = sscanf(pstr+8,"/%d",&nVideoPayloadClockRate);



                                        }

                                    }
                                    else if (rtptype == m_nAppPayloadType)
                                    {
                                        if(NULL != (pstr = psdp->StrStr("vnd.onvif.metadata",line,1)))
                                        {
                                            m_nAppStreamType = ANTS_RTSP_STREAM_APP;

                                            n = sscanf(pstr+strlen("vnd.onvif.metadata"),"/%d",&m_nAppPayloadClockRate);
                                            if (n != 1)
                                            {
                                                //NeedClose(ANTS_RTSP_ERROR_SDP_Rtpmap);
                                                //Error_handle(pMsg);
                                                m_bAppExist = 0;
                                            }


                                        }

                                    }
                                    else if(rtptype == nAudioPayloadType)
                                    {
                                      if (NULL != (pstr = psdp->StrStr("PCMU",line,1)))
                                      {
                                          nAudioStreamType = ANTS_RTSP_STREAM_G711U;
                                          n = sscanf(pstr + 4,"/%d",&nAudioPayloadClockRate);

                                      }
                                      else if (NULL != (pstr = psdp->StrStr("PCMA",line,1)))
                                      {
                                          nAudioStreamType = ANTS_RTSP_STREAM_G711A;

                                          n = sscanf(pstr+4,"/%d",&nAudioPayloadClockRate);

                                      }


                                    }


                            }
                        }
                    }

                }
                delete psdp;
            }

        }
        if (nAudioPayloadType != -1)
        {

            if (bSendOnlyA)
            {
                m_nTalkPayloadType = nAudioPayloadType;
                if(m_pTalk_control != NULL)
                {
                    Ants_strFree(m_pTalk_control);
                    m_pTalk_control = NULL;
                }
                m_nTalkStreamType = nAudioStreamType;
                m_pTalk_control = pAudio_control;
                pAudio_control = NULL;
                if (m_pTalkSess != NULL)
                {
                    m_pTalkSess->SetDefaultPayloadType(m_nTalkPayloadType);
                }

				if (m_nTalkStreamType == ANTS_RTSP_STREAM_G711U ||
					m_nTalkStreamType == ANTS_RTSP_STREAM_G711A)
				{
					m_bTalkExist = 1;
					m_nTalkPayloadClockRate = nAudioPayloadClockRate;
					if(m_nTalkPayloadClockRate == 0)
					{
						m_nTalkPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nTalkPayloadType);
					}

				}
				else if (m_nTalkPayloadType != 0 &&
                    m_nTalkPayloadType != 8)
                {
                    //NeedClose();
                    m_bTalkExist = 0;//不支持
                }
                else
                {
                    if (m_nTalkPayloadType == 0)
                    {
                        m_nTalkStreamType = ANTS_RTSP_STREAM_G711U;
                    }
                    else
                    {
                        m_nTalkStreamType = ANTS_RTSP_STREAM_G711A;
                    }
                    m_nTalkPayloadClockRate = nAudioPayloadClockRate;
                    if(m_nTalkPayloadClockRate == 0)
                    {
                        m_nTalkPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nTalkPayloadType);
                    }

                    m_bTalkExist = 1;
                    if (m_nMode == 2)
                    {// 多播

                        //m_nMulticastPort[1] = nMulticastPort_A;
                    }
                }
            }
            else
            {
                if ( bMediaVideo == 0)
                {
                    m_bAudioOrder = 1;
                }
                if(m_pAudio_control != NULL)
                {
                    Ants_strFree(m_pAudio_control);
                    m_pAudio_control = NULL;
                }
                m_pAudio_control = pAudio_control;
                pAudio_control = NULL;

                if (m_pMulticastAddr[0] != NULL)
                {
                    Ants_strFree(m_pMulticastAddr[0]);
                    m_pMulticastAddr[0] = NULL;
                }
                m_pMulticastAddr[0] = pMulticastAddr;
                m_nMulticastTTL[0] = nTTL;
                pMulticastAddr = NULL;


                m_nAudioPayloadType = nAudioPayloadType;
                m_nAudioPayloadClockRate = nAudioPayloadClockRate;
                if(m_nAudioPayloadClockRate == 0)
                {
                    m_nAudioPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nAudioPayloadType);
                }
                m_nAudioStreamType = (ANTS_RTSP_STREAM_TYPE)nAudioStreamType;

                if (m_pAudioSess != NULL)
                {
                    m_pAudioSess->SetDefaultPayloadType(m_nAudioPayloadType);
                }

				if (m_nAudioStreamType == ANTS_RTSP_STREAM_G711U ||
					m_nAudioStreamType == ANTS_RTSP_STREAM_G711A)
				{
					m_bAudioExist = 1;
					if (m_nMode == 2)
					{// 多播

						m_nMulticastPort[1] = nMulticastPort_A;
					}
				}
                else if (m_nAudioPayloadType != 0 &&
                    m_nAudioPayloadType != 8)
                {
                    //NeedClose();
                    m_bAudioExist = 0;//不支持
                }
                else
                {
                    if (m_nAudioPayloadType == 0)
                    {
                        m_nAudioStreamType = ANTS_RTSP_STREAM_G711U;
                    }
                    else
                    {
                        m_nAudioStreamType = ANTS_RTSP_STREAM_G711A;
                    }
                    m_bAudioExist = 1;
                    if (m_nMode == 2)
                    {// 多播

                        m_nMulticastPort[1] = nMulticastPort_A;
                    }
                }
            }
            nAudioPayloadType = -1;
            nMulticastPort_A = 0;
            bSendOnlyA = 0;
        }
        if (nVideoPayloadType != -1)
        {
            if(m_pVideo_control != NULL)
            {
                Ants_strFree(m_pVideo_control);
                m_pVideo_control = NULL;
            }
            m_pVideo_control = pVideo_control;
            pVideo_control = NULL;

            if (m_pMulticastAddr[1] != NULL)
            {
                Ants_strFree(m_pMulticastAddr[1]);
                m_pMulticastAddr[1] = NULL;
            }
            m_pMulticastAddr[1] = pMulticastAddr;
            m_nMulticastTTL[1] = nTTL;
            pMulticastAddr = NULL;

            m_nVideoPayloadType = nVideoPayloadType;
            m_nVideoPayloadClockRate = nVideoPayloadClockRate;
            if(m_nVideoPayloadClockRate == 0)
            {
                m_nVideoPayloadClockRate = Ants_Rtsp_GetPayloadClockRate(m_nVideoPayloadType);
            }
            if (nVideoStreamType == -1)
            {
                if (nVideoPayloadType == 26)
                {
                    nVideoStreamType = ANTS_RTSP_STREAM_JPEG;
                }
            }

                m_nVideoStreamType = (ANTS_RTSP_STREAM_TYPE)nVideoStreamType;

            bMediaVideo = 1;
            if (m_pVideoSess != NULL)
            {
                char *pdec;
                unsigned int declen = 0;
                m_pVideoSess->SetDefaultPayloadType(m_nVideoPayloadType);
                if (psprop_parameter_sets_sps != NULL)
                {
                    pdec = Ants_Rtsp_Base64Decode(psprop_parameter_sets_sps,strlen(psprop_parameter_sets_sps),&declen);
                    if (pdec != NULL)
                    {
                        if (m_pVideoSess != NULL)
                        {
                            m_pVideoSess->SetSPS(pdec,declen);
                        }
                        delete []pdec;
                    }
                    Ants_strFree(psprop_parameter_sets_sps);
                    psprop_parameter_sets_sps = NULL;
                }
                if (psprop_parameter_sets_pps != NULL)
                {
                    declen = 0;
                    pdec = Ants_Rtsp_Base64Decode(psprop_parameter_sets_pps,strlen(psprop_parameter_sets_pps),&declen);
                    if (pdec != NULL)
                    {
                        if (m_pVideoSess != NULL)
                        {
                            m_pVideoSess->SetPPS(pdec,declen);
                        }
                        delete []pdec;
                    }
                    Ants_strFree(psprop_parameter_sets_pps);
                    psprop_parameter_sets_pps = NULL;
                }
                if (psprop_parameter_sets_vps != NULL)
                {
                    declen = 0;
                    pdec = Ants_Rtsp_Base64Decode(psprop_parameter_sets_vps,strlen(psprop_parameter_sets_vps),&declen);
                    if (pdec != NULL)
                    {
                        if (m_pVideoSess != NULL)
                        {
                            m_pVideoSess->SetVPS(pdec,declen);
                        }
                        delete []pdec;
                    }
                    Ants_strFree(psprop_parameter_sets_vps);
                    psprop_parameter_sets_vps = NULL;
                }
            }



            m_bVideoExist = 1;
            if (m_nMode == 2)
            {// 多播

                m_nMulticastPort[0] = nMulticastPort_V;
            }

            nVideoPayloadType = -1;
            nMulticastPort_V = 0;
            bSendOnlyV = 0;
        }

        Ants_strFree(pMulticastAddr);
        pMulticastAddr = NULL;
        Ants_strFree(pAudio_control);
        pAudio_control = NULL;
        Ants_strFree(pVideo_control);
        pVideo_control = NULL;
        Ants_strFree(psprop_parameter_sets_sps);
        psprop_parameter_sets_sps = NULL;
        Ants_strFree(psprop_parameter_sets_pps);
        psprop_parameter_sets_pps = NULL;
		Ants_strFree(psprop_parameter_sets_vps);
		psprop_parameter_sets_vps = NULL;


        if (!m_bAudioExist)
        {
            m_bAudioOrder = 0;
        }
        if(m_bAudioOrder)
        {
            m_nOPOrders[2] = 'a';
            m_nOPOrders[3] = 'v';
        }
        else
        {
            m_nOPOrders[2] = 'v';
            m_nOPOrders[3] = 'a';
        }
        if (m_nMode == 2)
        {// 多播,检查合法性
            if (m_bAudioExist)
            {
	            if (m_pMulticastAddr[0] != NULL)
	            {
	                unsigned int ipv4;
	                ipv4 = inet_addr(m_pMulticastAddr[0]);
	                if(htonl(ipv4) < 0xE0000000 || htonl(ipv4) > 0xEFFFFFFF)
	                {
	                    NeedClose(ANTS_RTSP_ERROR_InvalidMulticast);
	                }
	            }
	            else
	            {
	                if (m_pMulticastAddr[1] != NULL)
	                {
                        m_pMulticastAddr[0] = Ants_strndup(m_pMulticastAddr[1], strlen(m_pMulticastAddr[1]));
	                }
					else
					{
	                    NeedClose(ANTS_RTSP_ERROR_InvalidMulticast);
					}
	            }
            }

			if (m_bVideoExist)
			{
	            if (m_pMulticastAddr[1] != NULL)
	            {
	                unsigned int ipv4;
	                ipv4 = inet_addr(m_pMulticastAddr[1]);
	                if(htonl(ipv4) < 0xE0000000 || htonl(ipv4) > 0xEFFFFFFF)
	                {
	                    NeedClose(ANTS_RTSP_ERROR_InvalidMulticast);
	                }
	            }
	            else
	            {
	                NeedClose(ANTS_RTSP_ERROR_InvalidMulticast);
	            }
			}
        }

    return 0;
}
int CRtspClient::SetupV_handle(CMessage *pMsg)
{
        char *pSessionID = NULL;
        int line,seg;
        char *pstr;
        int bTimeout = 0;
        int nTransPort0 = -1,nTransPort1 = -1;
        int nSrvTransPort0 = -1,nSrvTransPort1 = -1;
        for (line = 0; line < pMsg->m_nLineCnt;line++)
        {
            if (!pMsg->StrCmp("Session:",line,0,strlen("Session:")))
            {
                pSessionID = pMsg->GetStr(line,1);
                if (pSessionID != NULL)
                {
                    pstr = strchr(pSessionID,';');
                    if (pstr != NULL)
                    {
                          pstr[0] = 0;
                          pstr = (char *)strstri(pstr + 1,"timeout=");
                          if (pstr != NULL )
                          {
                               sscanf(pstr+8,"%d",&m_nTimeOut);
                               bTimeout = 1;
                          }
                    }

                }
				else
				{
						pSessionID = pMsg->GetStr(line,0);
						pSessionID += strlen("Session:");
						pstr = strchr(pSessionID,';');
						if (pstr != NULL)
						{
							pstr[0] = 0;
							pstr = (char *)strstri(pstr + 1,"timeout=");
							if (pstr != NULL )
							{
								sscanf(pstr+8,"%d",&m_nTimeOut);
								bTimeout = 1;
							}
						}
				}
                if(!bTimeout)
                {
                    for (seg = 2; seg < pMsg->m_nSegCnt[line];seg++)
                    {
                        pstr = pMsg->StrStr("timeout=",line,seg);
                        if (pstr != NULL)
                        {
                            sscanf(pstr+8,"%d",&m_nTimeOut);
                        }
                    }
                }


            }
            if (!pMsg->StrCmp("Transport:",line,0,strlen("Transport:")))
            {
                for (seg = 0; seg < pMsg->m_nSegCnt[line];seg++)
                {

                    if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
                        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
                    {
                        pstr = pMsg->StrStr("interleaved=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+12,"%d",&nTransPort0);
                                if (n == 1)
                                {
                                    nTransPort1 = nTransPort0 + 1;
                                }
                            }
                        }

                    }
					else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
					{
                        pstr = pMsg->StrStr("port=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr,"port=%d-%d",&nTransPort0,&nTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+5,"%d",&nTransPort0);
                                if (n == 1)
                                {
                                    nTransPort1 = nTransPort0 + 1;
                                }
                            }
                        }
					}
                    else
                    {
                        pstr = pMsg->StrStr("client_port=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr,"client_port=%d-%d",&nTransPort0,&nTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+12,"%d",&nTransPort0);
                                if (n == 1)
                                {
                                    nTransPort1 = nTransPort0 + 1;
                                }
                            }
                        }
                        pstr = pMsg->StrStr("server_port=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr+12,"%d-%d",&nSrvTransPort0,&nSrvTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+12,"%d",&nSrvTransPort0);
                                if (n == 1)
                                {
                                    nSrvTransPort1 = nSrvTransPort0 + 1;
                                }
                            }
                        }
                    }
                 }

            }
        }
        if(!m_bFirstSetup)
        {
            m_bFirstSetup |= 1;
            if (pSessionID == NULL || (m_strSessionID != NULL && strcmp(m_strSessionID,pSessionID)))
            {
                NeedClose(ANTS_RTSP_ERROR_InvSessionID);
            }
            else if (m_strSessionID == NULL)
            {
                m_strSessionID = Ants_strndup(pSessionID,-1);
                if (m_strSessionID == NULL)
                {
                    NeedClose(ANTS_RTSP_ERROR_InvSessionID);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////
        // 解决一些服务器，对端口应答不一致的情况。
        if ((m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
            m_nMode == RTSPCLIENT_OPEN_MODE_HTTP) && m_nVideoTransPort != nTransPort0)
        {
            m_nVideoTransPort = nTransPort0;
        }
        //////////////////////////////////////////////////////////////////////////
        if (nTransPort0 == -1 ||
            nTransPort1 == -1 ||
            (nTransPort0 & 1) ||
            (nTransPort1 != nTransPort0 + 1) ||
            (m_nMode != RTSPCLIENT_OPEN_MODE_MULTI && m_nVideoTransPort != nTransPort0))
        {
            NeedClose(ANTS_RTSP_ERROR_InvTransPort);
        }
        if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
            m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
        {

        }
		else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
		{
		    if (nTransPort0 != m_nMulticastPort[0] && nTransPort0 != -1)
		    {
                m_nMulticastPort[0] = nTransPort0;
			}
            m_nVideoSrvTransPort = nTransPort1;
		}
        else
        {
#if 0 //
            if (nSrvTransPort0 == -1 ||
                nSrvTransPort1 == -1 ||
                (nSrvTransPort0 & 1) ||
                (nSrvTransPort1 != nSrvTransPort0 + 1) )
            {
                NeedClose(ANTS_RTSP_ERROR_InvTransPort);
            }
            else
            {
                m_nVideoSrvTransPort = nSrvTransPort0;
            }
#else
            m_nVideoSrvTransPort = nSrvTransPort0;
#endif

        }




    return 0;
}
int CRtspClient::SetupA_handle(CMessage *pMsg)
{

        char *pSessionID = NULL;
        int line,seg;
        char *pstr;
        int bTimeout = 0;
        int nTransPort0 = -1,nTransPort1 = -1;
        int nSrvTransPort0 = -1,nSrvTransPort1 = -1;
        for (line = 0; line < pMsg->m_nLineCnt;line++)
        {

            if (!pMsg->StrCmp("Session:",line,0,strlen("Session:")))
            {
                pSessionID = pMsg->GetStr(line,1);
                if (pSessionID != NULL)
                {
                    pstr = strchr(pSessionID,';');
                    if (pstr != NULL)
                    {
                        pstr[0] = 0;
                        pstr = (char *)strstri(pstr + 1,"timeout=");
                        if (pstr != NULL )
                        {
                            sscanf(pstr+8,"%d",&m_nTimeOut);
                            bTimeout = 1;
                        }
                    }

                }
				else
				{
					pSessionID = pMsg->GetStr(line,0);
					pSessionID += strlen("Session:");
					pstr = strchr(pSessionID,';');
					if (pstr != NULL)
					{
						pstr[0] = 0;
						pstr = (char *)strstri(pstr + 1,"timeout=");
						if (pstr != NULL )
						{
							sscanf(pstr+8,"%d",&m_nTimeOut);
							bTimeout = 1;
						}
					}
				}
                if(!bTimeout)
                {
                    for (seg = 2; seg < pMsg->m_nSegCnt[line];seg++)
                    {
                        pstr = pMsg->StrStr("timeout=",line,seg);
                        if (pstr != NULL)
                        {
                            sscanf(pstr+8,"%d",&m_nTimeOut);
                        }
                    }
                }

            }
            if (!pMsg->StrCmp("Transport:",line,0,strlen("Transport:")))
            {
                for (seg = 0; seg < pMsg->m_nSegCnt[line];seg++)
                {

                    if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
                        m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
                    {
                        pstr = pMsg->StrStr("interleaved=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+12,"%d",&nTransPort0);
                                if (n == 1)
                                {
                                    nTransPort1 = nTransPort0 + 1;
                                }
                            }
                        }

                    }
					else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
					{
                        pstr = pMsg->StrStr("port=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr,"port=%d-%d",&nTransPort0,&nTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+5,"%d",&nTransPort0);
                                if (n == 1)
                                {
                                    nTransPort1 = nTransPort0 + 1;
                                }
                            }
                        }
					}
                    else
                    {
                        pstr = pMsg->StrStr("client_port=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr+12,"%d-%d",&nTransPort0,&nTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+12,"%d",&nTransPort0);
                                if (n == 1)
                                {
                                    nTransPort1 = nTransPort0 + 1;
                                }
                            }
                        }
                        pstr = pMsg->StrStr("server_port=",line,seg);
                        if (pstr != NULL)
                        {
                            int n;
                            n = sscanf(pstr+12,"%d-%d",&nSrvTransPort0,&nSrvTransPort1);
                            if (n != 2)
                            {
                                n = sscanf(pstr+12,"%d",&nSrvTransPort0);
                                if (n == 1)
                                {
                                    nSrvTransPort1 = nSrvTransPort0 + 1;
                                }
                            }
                        }
                    }
                }
            }

        }
        if(!m_bFirstSetup)
        {
            m_bFirstSetup |= 1;
            if (pSessionID == NULL || (m_strSessionID != NULL && strcmp(m_strSessionID ,pSessionID)))
            {
                NeedClose(ANTS_RTSP_ERROR_InvSessionID);
            }
            else if (m_strSessionID == NULL)
            {
                m_strSessionID = Ants_strndup(pSessionID,-1);
                if (m_strSessionID == NULL)
                {
                    NeedClose(ANTS_RTSP_ERROR_InvSessionID);
                }
            }
        }

        if ((m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
            m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)&& m_nAudioTransPort != nTransPort0)
        {
            m_nAudioTransPort = nTransPort0;
        }
        if (nTransPort0 == -1 ||
            nTransPort1 == -1 ||
            (nTransPort0 & 1) ||
            (nTransPort1 != nTransPort0 + 1) ||
            (m_nMode != RTSPCLIENT_OPEN_MODE_MULTI && m_nAudioTransPort != nTransPort0))
        {
            NeedClose(ANTS_RTSP_ERROR_InvTransPort);
        }

        if(m_nMode == RTSPCLIENT_OPEN_MODE_TCP ||
            m_nMode == RTSPCLIENT_OPEN_MODE_HTTP)
        {

        }
		else if (m_nMode == RTSPCLIENT_OPEN_MODE_MULTI)
		{
		    if (nTransPort0 != m_nMulticastPort[1] && nTransPort0 != -1)
		    {
                m_nMulticastPort[1] = nTransPort0;
			}
            m_nAudioSrvTransPort = nTransPort1;
		}
        else
        {
#if 0
            if (nSrvTransPort0 == -1 ||
                nSrvTransPort1 == -1 ||
                (nSrvTransPort0 & 1) ||
                (nSrvTransPort1 != nSrvTransPort0 + 1) )
            {
                NeedClose(ANTS_RTSP_ERROR_InvTransPort);
            }
            else
            {
                m_nAudioSrvTransPort = nSrvTransPort0;
            }
#else
            m_nAudioSrvTransPort = nSrvTransPort0;
#endif

        }





    return 0;
}

int CRtspClient::AddReqList(int nCmd,int nSeq,int bRes)
{
    int n;
    int bSet = 0;
    int nMaxIdx = -1;
    int nMaxCnt = -1;
    AntsRtspClient_Req_List_T *pReq;
    m_hReqListLock.Lock();
    if (m_nReqListCnt == RTSPCLIENT_REQ_LIST_MAX)
    {
        m_hReqListLock.Unlock();
        NeedClose(ANTS_RTSP_ERROR_BufferTooSmall);
       return -1;
    }
    pReq = new AntsRtspClient_Req_List_T;
    if (pReq == NULL)
    {
        m_hReqListLock.Unlock();
        NeedClose(ANTS_RTSP_ERROR_MallocError);
        return -1;
    }
    memset(pReq,0,sizeof(AntsRtspClient_Req_List_T));
    pReq->bRes = bRes;
    pReq->nSeq = nSeq;
    pReq->nCmd = nCmd;
    if(m_pReqList_tail == NULL)
    {
        m_pReqList_tail = m_pReqList = pReq;
    }
    else
    {
        m_pReqList_tail->pNext = pReq;
        m_pReqList_tail = pReq;
    }
    m_nReqListCnt++;
    m_hReqListLock.Unlock();
    return 0;

}
int CRtspClient::Error_handle(CMessage *pMsg,AntsRtspClient_Req_List_T *pReq)
{
    int line,seg;
    char ops[][16]={"Options","Describe","Setup","SetupV","SetupA","app","talk","Play","PAUSE","GetParameter","SetParameter","AddCh","DecCh","TearDown"};
    char *tmpstr=NULL;
    int pos = 0;
    int bWWW_au = 0;
    int bNeedAuth = 0;
    char *pstr;
    tmpstr = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (tmpstr == NULL)
    {

        return -1;
    }

    if (pMsg->GetResNum() == 401)
    {//需要认证，或密码错误
        bNeedAuth = 1;
		m_bNeedAuth = 1;
		if (m_bDigest == 1)
		{
		    StatusHandle(ANTS_RTSP_ERROR_Authenticate);
		}
		m_nAuthCount++;
		if (m_nAuthCount >= 3)
		{
			m_nAuthCount = 0;
			NeedClose(ANTS_RTSP_ERROR_Authenticate);
		}
    }
    tmpstr[ANTS_RTSP_TMP_BUFSIZE] = 0;
    pos += snprintf(tmpstr+pos,ANTS_RTSP_TMP_BUFSIZE - pos,"\n=======ERROR[%s]==========={{\n",pReq == NULL?"":((pReq->nCmd >= 1 && pReq->nCmd <= (sizeof(ops)/sizeof(ops[0])))?ops[pReq->nCmd-1]:""));
    for (line = 0; line < pMsg->m_nLineCnt;line++)
    {
        bWWW_au = 0;
        for (seg = 0; seg < pMsg->m_nSegCnt[line];seg++)
        {
            if (bNeedAuth)
            {
                if (!pMsg->StrCmp("WWW-Authenticate:",line,seg))
                {
                    bWWW_au = 1;
                }
            }
            if (bWWW_au)
            {
                if (pMsg->StrStr("Digest",line,seg))
                {
                    m_bDigest = 1;
                }
                else if (pMsg->StrStr("Basic",line,seg))
                {
                    m_bDigest = 0;
                }

                if (pstr= pMsg->StrStr("realm=",line,seg))
                {//取引号里面的内容
                    char tmpstr1[128];
                    int i,bq = 0,j = 0;
                    for (i = 0; i < 127; i++)
                    {
                        if (pstr[i] == 0)
                        {
                            break;
                        }
                        if (bq)
                        {
                            if (pstr[i] == '\"')
                            {
                                tmpstr1[j++] = 0;
                                break;
                            }
                            tmpstr1[j++] = pstr[i];
                        }
                        else if(pstr[i] == '\"')
                        {
                            bq = 1;
                        }

                    }
                    tmpstr1[127] = 0;
                    Ants_strFree(m_pRealm);
                    m_pRealm = Ants_strndup(tmpstr1,-1);
                }
                if (pstr = pMsg->StrStr("nonce=",line,seg))
                {
                    char tmpstr1[128];
                    int i,bq = 0,j = 0;
                    for (i = 0; i < 127; i++)
                    {
                        if (pstr[i] == 0)
                        {
                            break;
                        }
                        if (bq)
                        {
                            if (pstr[i] == '\"')
                            {
                                tmpstr1[j++] = 0;
                                break;
                            }
                            tmpstr1[j++] = pstr[i];
                        }
                        else if(pstr[i] == '\"')
                        {
                            bq = 1;
                        }

                    }
                    tmpstr1[127] = 0;
                    Ants_strFree(m_pNonce);
                    m_pNonce = Ants_strndup(tmpstr1,-1);
                }
            }
            pos += snprintf(tmpstr+pos,ANTS_RTSP_TMP_BUFSIZE - pos,"%s ",pMsg->GetStr(line,seg));
        }

        bWWW_au = 0;
        pos += snprintf(tmpstr+pos,ANTS_RTSP_TMP_BUFSIZE - pos,"%s","\n");
    }
    pos += snprintf(tmpstr+pos,ANTS_RTSP_TMP_BUFSIZE - pos,"%s","}}=======ERROR===========\n");
    RTSP_ERROR(tmpstr);
    if(!m_bNeedClose)
    {


    if (bNeedAuth)
    {
#if 0
        HASHHEX Response,HA1;

        // The "response" field is computed as:
        //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
        // or, if "fPasswordIsMD5" is True:
        //    md5(<password>:<nonce>:md5(<cmd>:<url>))
        DigestCalcHA1("",m_pUserName,m_pRealm,m_pPassword,"","",HA1);
        DigestCalcResponse(HA1,m_pNonce,"","","","Option",url,"",Response);
#endif

        m_nNextOPIndex = 0;
    //    Ants_WaitTime(1,0);
    //    RTPTime::Wait(RTPTime(0,10000));
    }
    else if(pReq != NULL)
    {
        if(pReq->nCmd == RTSP_CMD_TYPE_SETUP_A)
        {
            // 音频错误，则关掉音频，仍然放视频
            RTSP_ERROR("can\'t open audio,go on\n");
        }
        else if (!pReq->bRes)
        {
        }
        else
        {
            NeedClose(ANTS_RTSP_ERROR_ResError);
        }

    }
    }
    delete []tmpstr;

   return 0;
}
CMessage *CRtspClient::GetMessage()
{
    CMessage *p;
    if (m_pMsg == NULL)
    {
        return NULL;
    }
    p = m_pMsg;
    m_pMsg = m_pMsg->m_pNext;

    p->m_pNext = NULL;
    return p;

}
#define  TEST 0
#if TEST
static char testbuf[1024 * 1024 * 5];
static int testpos = 0;
static char *ptestlast = NULL;
static int testlast = 0;
#endif
int CRtspClient::RecvAndParse()
{
    char c;
    int nRet;
    int bNeedMore = 0;
    int nCrLf = 0;
    int bLine = 0;
    int bHasContext = 0;
    int nContextLen = 0;
    int errorNo;
    char *pConText = NULL;
    unsigned long len;
    int bRtspRes = 0;
	int nSocket = 0;

	if (m_nUrlType == 1)
	{
        //nSocket = m_pRtp_Session[0]->GetRTPSocket();
        nSocket = m_nRtp_SockFD[0][0];
	}
	else
	{
        nSocket = m_nSocket;
	}

    do
    {
        if (m_bNeedClose)
        {
            break;
        }
        if (m_bDollar)
        {
            if (m_nDollarPos0 < 4)
            {
                nRet = recv(nSocket,m_pDollarBuffer0 + m_nDollarPos0,4 - m_nDollarPos0,0);
                if (nRet == 0)
                {
                    NeedClose(ANTS_RTSP_ERROR_SocketClose);
                    break ;
                }
                if (nRet < 0)
                {
                    errorNo = GetLastError();
                    if (errorNo == EWOULDBLOCK ||
                        errorNo == EINTR||
                        errorNo == EAGAIN ||
                        errorNo == ETIMEDOUT)
                    {

                        bNeedMore = 1;
                    }
                    else
                    {
                        NeedClose(ANTS_RTSP_ERROR_SocketError);
                    }

                    break ;
                }
#if TEST
              //  memcpy(testbuf + testpos,m_pDollarBuffer0 + m_nDollarPos0,nRet);
              //  testpos += nRet;
#endif
                if (nRet != 4 - m_nDollarPos0)
                {
                    m_nDollarPos0 += nRet;
                    bNeedMore = 1;
                    break;
                }

                m_nDollarPos0 += 4- m_nDollarPos0;
            }
            if (m_pDollarBuffer == NULL)
            {
                //继续
                m_nDollarLen = htons(*((short *)&m_pDollarBuffer0[2]));
                if (m_nDollarLen <= 0 || m_nDollarLen > 1500)
                {
                   // NeedClose(ANTS_RTSP_ERROR_InvStreamDataLen);
                    //break;
                    //丢掉
                    m_nDollarLen = 0;
                    m_nDollarPos0 = 0;
                    m_bDollar = 0;
                    m_nDollarPos = 0;
                    continue;

                }
                m_pDollarBuffer = new char[m_nDollarLen];
                m_nDollarPos = 0;
                if (m_pDollarBuffer == NULL)
                {//内在分配失败，关掉
                    NeedClose(ANTS_RTSP_ERROR_MallocError);
                    break;
                }


            }
                nRet = recv(nSocket,m_pDollarBuffer + m_nDollarPos,m_nDollarLen - m_nDollarPos,0);
                if (nRet == 0)
                {
                    NeedClose(ANTS_RTSP_ERROR_SocketClose);
                    break ;
                }
                if (nRet < 0)
                {
                    errorNo = GetLastError();
                    if (errorNo == EWOULDBLOCK ||
                        errorNo == EINTR||
                        errorNo == EAGAIN ||
                        errorNo == ETIMEDOUT)
                    {

                        bNeedMore = 1;
                    }
                    else
                    {
                        NeedClose(ANTS_RTSP_ERROR_SocketError);
                    }

                    break ;
                }
#if TEST
             //   memcpy(testbuf + testpos,m_pDollarBuffer + m_nDollarPos,nRet);
              //  testpos += nRet;
#endif
                if (nRet != m_nDollarLen - m_nDollarPos)
                {
                    m_nDollarPos += nRet;
                    bNeedMore = 1;
                    break;
                }
                 m_nDollarPos += m_nDollarLen - m_nDollarPos;
                //完整一包
#if TEST
          //       ptestlast = testbuf + testpos - nRet;
          //       testlast = m_nDollarLen;
#endif
                PushRTPorRTCP(m_pDollarBuffer0[1],m_pDollarBuffer,m_nDollarLen);
                m_pDollarBuffer = NULL;
                m_nDollarLen = 0;
                m_nDollarPos = 0;
                m_nDollarPos0 = 0;
                m_bDollar = 0;
                break;

        }
        nRet = recv(nSocket,&c,1,0);
        if (nRet == 0)
        {
            NeedClose(ANTS_RTSP_ERROR_SocketClose);
            break;
        }
        if (nRet < 0)
        {
            errorNo = GetLastError();
            if (errorNo == EWOULDBLOCK ||
                errorNo == EINTR||
                errorNo == EAGAIN ||
                errorNo == ETIMEDOUT)
            {

                bNeedMore = 1;
            }
            else
            {
                NeedClose(ANTS_RTSP_ERROR_SocketError);
            }

            break;
        }
#if TEST
        testbuf[testpos++] = c;
#endif
        if (c == '$')
        {
            //处理命令消息?
#if 0
            if (m_nRecvBuffPos > 0)
            {
                CMessage *pmsg,*p;
                pmsg = Parse(m_pRecvBuff,m_nRecvBuffPos,0);
                if (pmsg != NULL)
                {
                    if (m_pMsg != NULL)
                    {
                        p = m_pMsg;
                        while(1)
                        {
                            if (p->m_pNext == NULL)
                            {
                                break;
                            }
                            p = p->m_pNext;
                        }
                        p->m_pNext = pmsg;
                    }
                    else
                    {
                        m_pMsg = pmsg;
                    }
                }
                m_nRecvBuffPos = 0;
            }
#endif
            //////////////////////////////////////////////////////////////////////////
            m_bDollar = 1;
            m_nDollarPos0 = 1;
            m_pDollarBuffer0[0] = '$';
            m_nDollarPos = 0;
            m_nDollarLen = 0;
            nRet = recv(nSocket,m_pDollarBuffer0+1,3,0);
            if (nRet == 0)
            {
                NeedClose(ANTS_RTSP_ERROR_SocketClose);
                break;
            }
            if (nRet < 0)
            {
                errorNo = GetLastError();
                if (errorNo == EWOULDBLOCK ||
                    errorNo == EINTR||
                    errorNo == EAGAIN ||
                    errorNo == ETIMEDOUT)
                {

                    bNeedMore = 1;
                }
                else
                {
                    NeedClose(ANTS_RTSP_ERROR_SocketError);
                }


                break;
            }
#if TEST
            memcpy(testbuf + testpos,m_pDollarBuffer0 + 1,nRet);
            testpos += nRet;
#endif
            if (nRet != 3)
            {
                m_nDollarPos0 = 1 + nRet;
                bNeedMore = 1;
                break;
            }

            m_nDollarPos0 = 1 + nRet;
        }
        else
        {


            m_pRecvBuff[m_nRecvBuffPos] = c;
            m_nRecvBuffPos ++;
            if ((m_nRecvBuffPos == 1 && (c != 'R' && c != 'r')) ||
                (m_nRecvBuffPos == 2 && (c != 'T' && c != 't')) ||
                (m_nRecvBuffPos == 3 && (c != 'S' && c != 's')) ||
                (m_nRecvBuffPos == 4 && (c != 'P' && c != 'p')))
            {
                m_nRecvBuffPos = 0;
                bRtspRes = 0;
                nCrLf = 0;
                continue;
            }


            if (m_nRecvBuffPos >= 2)
            {
                if (c == LF && m_pRecvBuff[m_nRecvBuffPos - 2] == CR)
                {
                    nCrLf++;
                    bLine = 1;
                }
                else if(c != CR)
                {
                    nCrLf = 0;
                }
                if(c == ':')
                {
                    if (m_nRecvBuffPos>=15 && !strnicmp("Content-Length:",&m_pRecvBuff[m_nRecvBuffPos - 15],15))
                    {
                        bHasContext = 1;
                        pConText = &m_pRecvBuff[m_nRecvBuffPos];
                    }
                }
                if (nCrLf == 1 && bHasContext)
                {
                    if (pConText != NULL)
                    {
                        nContextLen = atoi(pConText);
                        pConText = NULL;
                        if (nContextLen <= 0)
                        {
                            bHasContext = 0;
                        }
                    }


                }
                if (nCrLf == 2)
                {//完成
                    if (bHasContext)
                    {//要继续接收
                        if (m_nRecvBuffPos + nContextLen > RTSPCLIENT_RECV_BUFFSIZE)
                        {
                            NeedClose(ANTS_RTSP_ERROR_BufferTooSmall);
                            break;
                        }
                        nRet = recv(nSocket,m_pRecvBuff+ m_nRecvBuffPos,nContextLen,0);
                        if (nRet == 0)
                        {
                            NeedClose(ANTS_RTSP_ERROR_SocketClose);
                            break;
                        }
                        if (nRet < 0)
                        {
                            errorNo = GetLastError();
                            if (errorNo == EWOULDBLOCK ||
                                errorNo == EINTR||
                                errorNo == EAGAIN ||
                                errorNo == ETIMEDOUT)
                            {

                                bNeedMore = 1;
                            }
                            else
                            {
                                NeedClose(ANTS_RTSP_ERROR_SocketError);
                            }


                            break;
                        }
#if TEST
                        memcpy(testbuf + testpos,m_pRecvBuff+ m_nRecvBuffPos,nRet);
                        testpos += nRet;
#endif
                        if (nRet != nContextLen)
                        {
                            m_nRecvBuffPos += nRet;
                            bNeedMore = 1;
                            break;
                        }
                        m_nRecvBuffPos += nContextLen;

                        //完成
                        break;
                    }
                    else
                    {
                        //完成
                        break;
                    }
                }

            }

            if (m_nRecvBuffPos > RTSPCLIENT_RECV_BUFFSIZE)
            {
                NeedClose(ANTS_RTSP_ERROR_BufferTooSmall);
                break;
            }
            len = 0;
#if 0
#ifdef WIN32
            ioctlsocket(m_nSocket,FIONREAD,&len);
#else
            ioctl(m_nSocket,FIONREAD,&len);
#endif
            m_bTestDebug = 2013;
            if (len <= 0)
                break ;
#endif
        }
    }while(1);
    return bNeedMore;

}
int CRtspClient::RecvAndParse_noblock()
{
    char c;
    int nRet;
    int bNeedMore = 0;
    int nCrLf = 0;
    int bLine = 0;
    int bHasContext = 0;
    int nContextLen = 0;
    int errorNo;
    char *pConText = NULL;
    unsigned long len;
    int bRtspRes = 0;
	int nSocket = 0;

	if (m_nUrlType == 1)
	{
        //nSocket = m_pRtp_Session[0]->GetRTPSocket();
        nSocket = m_nRtp_SockFD[0][0];
	}
	else
	{
        nSocket = m_nSocket;
	}

    if (m_nRecvRawBufferPos - m_nRecvRawUsePos > 0)
    {

            memmove(m_pRecvRawBuffer,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
            m_nRecvRawBufferPos -= m_nRecvRawUsePos;
            m_nRecvRawUsePos = 0;

    }
    else
    {
        m_nRecvRawBufferPos = 0;
        m_nRecvRawUsePos = 0;
    }
    nRet = recv(nSocket,m_pRecvRawBuffer + m_nRecvRawBufferPos,RTSPCLIENT_RECV_RAW_BUFFSIZE - m_nRecvRawBufferPos,0);
    if (nRet == 0)
    {
        NeedClose(ANTS_RTSP_ERROR_SocketClose);
        return bNeedMore;
    }
    if (nRet < 0)
    {
        errorNo = GetLastError();
        if (errorNo == EWOULDBLOCK ||
            errorNo == EINTR||
            errorNo == EAGAIN ||
            errorNo == ETIMEDOUT)
        {

            bNeedMore = 1;
        }
        else
        {
            RTSP_DEBUG("%s \n",strerror(errorNo));
            NeedClose(ANTS_RTSP_ERROR_SocketError);
        }

        return bNeedMore ;
    }
    m_nRecvRawBufferPos += nRet;
    do
    {
        if (m_bNeedClose)
        {
            break;
        }
        if (m_nRecvRawBufferPos - m_nRecvRawUsePos <= 0)
        {
            bNeedMore = 1;
            break;
        }
        if (m_bDollar)
        {
            if (m_nDollarPos0 < 4)
            {
                if (m_nRecvRawBufferPos - m_nRecvRawUsePos < 4 - m_nDollarPos0)
                {
                    memcpy(m_pDollarBuffer0 + m_nDollarPos0,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
                    m_nDollarPos0 += m_nRecvRawBufferPos - m_nRecvRawUsePos;
                    m_nRecvRawUsePos = 0;
                    m_nRecvRawBufferPos = 0;
                    bNeedMore = 1;
                    break;
                }
                memcpy(m_pDollarBuffer0 + m_nDollarPos0,m_pRecvRawBuffer + m_nRecvRawUsePos,4 - m_nDollarPos0);
                m_nRecvRawUsePos += 4 - m_nDollarPos0;
                m_nDollarPos0 = 4;
            }
            m_nDollarLen = htons(*((short *)&m_pDollarBuffer0[2]));

            if (m_nDollarLen <= 0 || m_nDollarLen > RTSPCLIENT_RECV_RAW_BUFFSIZE)
            {
                RTSP_ERROR("!!too long %d\n",m_nDollarLen);
                // NeedClose(ANTS_RTSP_ERROR_InvStreamDataLen);
                //break;
                //丢掉
                m_nDollarLen = 0;
                m_nDollarPos0 = 0;
                m_bDollar = 0;
                m_nDollarPos = 0;
                continue;

            }
            if (m_nRecvRawBufferPos - m_nRecvRawUsePos < m_nDollarLen)
            {
                bNeedMore = 1;
                break;
            }


                m_pDollarBuffer = m_pRecvRawBuffer + m_nRecvRawUsePos;
                m_nDollarPos = 0;


            m_nDollarPos = m_nDollarLen;
            m_nRecvRawUsePos += m_nDollarLen;

            PushRTPorRTCP(m_pDollarBuffer0[1],m_pDollarBuffer,m_nDollarLen);
            m_pDollarBuffer = NULL;
            m_nDollarLen = 0;
            m_nDollarPos = 0;
            m_nDollarPos0 = 0;
            m_bDollar = 0;
            continue;

        }
        else if (m_bHasContext)
        {
            if (m_nRecvRawBufferPos - m_nRecvRawUsePos < m_nContextLen - m_nContextPos)
            {
                memcpy(m_pRecvBuff+ m_nRecvBuffPos,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
                m_nRecvBuffPos += m_nRecvRawBufferPos - m_nRecvRawUsePos;
                m_nContextPos += m_nRecvRawBufferPos - m_nRecvRawUsePos;
                m_nRecvRawUsePos = 0;
                m_nRecvRawBufferPos = 0;

                bNeedMore = 1;
                break;
            }
            memcpy(m_pRecvBuff+ m_nRecvBuffPos,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nContextLen - m_nContextPos);
            m_nRecvBuffPos += m_nContextLen - m_nContextPos;
            m_nRecvRawUsePos += m_nContextLen - m_nContextPos;
            m_bHasContext = 0;
            m_nContextLen = 0;
            m_nContextPos = 0;
            break;

        }
        if (m_nRecvRawBufferPos - m_nRecvRawUsePos < 1)
        {
            bNeedMore = 1;
            break;
        }
        c = m_pRecvRawBuffer[m_nRecvRawUsePos];
        m_nRecvRawUsePos++;

        if (c == '$')
        {
            //处理命令消息?

            //////////////////////////////////////////////////////////////////////////
            m_bDollar = 1;
            m_nDollarPos0 = 1;
            m_pDollarBuffer0[0] = '$';
            m_nDollarPos = 0;
            m_nDollarLen = 0;
            if (m_nRecvRawBufferPos - m_nRecvRawUsePos < 3)
            {
                memcpy(m_pDollarBuffer0+1,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
                m_nDollarPos0 += m_nRecvRawBufferPos - m_nRecvRawUsePos;
                m_nRecvRawUsePos = 0;
                m_nRecvRawBufferPos = 0;
                bNeedMore = 1;
                break;
            }
            memcpy(m_pDollarBuffer0+1,m_pRecvRawBuffer + m_nRecvRawUsePos,3);
            m_nRecvRawUsePos += 3;

            m_nDollarPos0 = 4;
        }
        else
        {


            m_pRecvBuff[m_nRecvBuffPos] = c;
            m_nRecvBuffPos ++;
            if ((m_nRecvBuffPos == 1 && (c != 'R' && c != 'r')) ||
                (m_nRecvBuffPos == 2 && (c != 'T' && c != 't')) ||
                (m_nRecvBuffPos == 3 && (c != 'S' && c != 's')) ||
                (m_nRecvBuffPos == 4 && (c != 'P' && c != 'p')))
            {
                m_nRecvBuffPos = 0;
                bRtspRes = 0;
                nCrLf = 0;
                continue;
            }


            if (m_nRecvBuffPos >= 2)
            {
                if (c == LF && m_pRecvBuff[m_nRecvBuffPos - 2] == CR)
                {
                    nCrLf++;
                    bLine = 1;
                }
                else if(c != CR)
                {
                    nCrLf = 0;
                }
                if(c == ':')
                {
                    if (m_nRecvBuffPos>=15 && !strnicmp("Content-Length:",&m_pRecvBuff[m_nRecvBuffPos - 15],15))
                    {
                        bHasContext = 1;
                        pConText = &m_pRecvBuff[m_nRecvBuffPos];
                    }
                }
                if (nCrLf == 1 && bHasContext)
                {
                    if (pConText != NULL)
                    {
                        nContextLen = atoi(pConText);
                        pConText = NULL;
                        if (nContextLen <= 0)
                        {
                            bHasContext = 0;
                        }

                    }


                }
                if (nCrLf == 2)
                {//完成
                    if (bHasContext)
                    {//要继续接收
                        if (m_nRecvBuffPos + nContextLen > RTSPCLIENT_RECV_BUFFSIZE)
                        {
                            NeedClose(ANTS_RTSP_ERROR_BufferTooSmall);
                            break;
                        }
                        if (m_nRecvRawBufferPos - m_nRecvRawUsePos < nContextLen)
                        {
                            memcpy(m_pRecvBuff+ m_nRecvBuffPos,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
                            m_nRecvBuffPos += m_nRecvRawBufferPos - m_nRecvRawUsePos;
                            m_nContextPos = m_nRecvRawBufferPos - m_nRecvRawUsePos;
                            m_nRecvRawUsePos = 0;
                            m_nRecvRawBufferPos = 0;
                            bNeedMore = 1;
                            m_bHasContext = 1;
                            m_nContextLen = nContextLen;

                            break;
                        }
                        memcpy(m_pRecvBuff+ m_nRecvBuffPos,m_pRecvRawBuffer + m_nRecvRawUsePos,nContextLen);
                        m_nRecvBuffPos += nContextLen;
                        m_nRecvRawUsePos += nContextLen;
                        m_bHasContext = 0;
                        m_nContextLen = 0;
                        m_nContextPos = 0;

                        //完成
                        break;
                    }
                    else
                    {
                        //完成
                        break;
                    }
                }

            }

            if (m_nRecvBuffPos > RTSPCLIENT_RECV_BUFFSIZE)
            {
                NeedClose(ANTS_RTSP_ERROR_BufferTooSmall);
                break;
            }

        }
    }while(1);
    return bNeedMore;

}
int CRtspClient::RecvAndParse_noblock_rtp()
{
    char c;
    int nRet;
    int bNeedMore = 0;
    int nCrLf = 0;
    int bLine = 0;
    int bHasContext = 0;
    int nContextLen = 0;
    int errorNo;
    char *pConText = NULL;
    unsigned long len;
    int bRtspRes = 0;
    int nSocket = 0;

    nSocket = m_nRtp_TcpSockFD;

    if (m_nRecvRawBufferPos - m_nRecvRawUsePos > 0)
    {

        memmove(m_pRecvRawBuffer,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
        m_nRecvRawBufferPos -= m_nRecvRawUsePos;
        m_nRecvRawUsePos = 0;

    }
    else
    {
        m_nRecvRawBufferPos = 0;
        m_nRecvRawUsePos = 0;
    }
    nRet = recv(nSocket,m_pRecvRawBuffer + m_nRecvRawBufferPos,RTSPCLIENT_RECV_RAW_BUFFSIZE - m_nRecvRawBufferPos,0);
    if (nRet == 0)
    {
        NeedClose(ANTS_RTSP_ERROR_SocketClose);
        return bNeedMore;
    }
    if (nRet < 0)
    {
        errorNo = GetLastError();
        if (errorNo == EWOULDBLOCK ||
            errorNo == EINTR||
            errorNo == EAGAIN ||
            errorNo == ETIMEDOUT)
        {

            bNeedMore = 1;
        }
        else
        {
            RTSP_DEBUG("%s \n",strerror(errorNo));
            NeedClose(ANTS_RTSP_ERROR_SocketError);
        }

        return bNeedMore ;
    }
    m_nRecvRawBufferPos += nRet;
    do
    {
        if (m_bNeedClose)
        {
            break;
        }
        if (m_nRecvRawBufferPos - m_nRecvRawUsePos <= 0)
        {
            bNeedMore = 1;
            break;
        }
        if (m_bDollar)
        {
            if (m_nDollarPos0 < 2)
            {
                if (m_nRecvRawBufferPos - m_nRecvRawUsePos < 2 - m_nDollarPos0)
                {
                    memcpy(m_pDollarBuffer0 + m_nDollarPos0,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
                    m_nDollarPos0 += m_nRecvRawBufferPos - m_nRecvRawUsePos;
                    m_nRecvRawUsePos = 0;
                    m_nRecvRawBufferPos = 0;
                    bNeedMore = 1;
                    break;
                }
                memcpy(m_pDollarBuffer0 + m_nDollarPos0,m_pRecvRawBuffer + m_nRecvRawUsePos,2 - m_nDollarPos0);
                m_nRecvRawUsePos += 2 - m_nDollarPos0;
                m_nDollarPos0 = 2;
            }
            m_nDollarLen = htons(*((short *)&m_pDollarBuffer0[0]));

            if (m_nDollarLen <= 0 || m_nDollarLen > RTSPCLIENT_RECV_RAW_BUFFSIZE)
            {
                RTSP_ERROR("!!too long %d\n",m_nDollarLen);
                // NeedClose(ANTS_RTSP_ERROR_InvStreamDataLen);
                //break;
                //丢掉
                m_nDollarLen = 0;
                m_nDollarPos0 = 0;
                m_bDollar = 0;
                m_nDollarPos = 0;
                continue;

            }
            if (m_nRecvRawBufferPos - m_nRecvRawUsePos < m_nDollarLen)
            {
                bNeedMore = 1;
                break;
            }


            m_pDollarBuffer = m_pRecvRawBuffer + m_nRecvRawUsePos;
            m_nDollarPos = 0;


            m_nDollarPos = m_nDollarLen;
            m_nRecvRawUsePos += m_nDollarLen;

            //PushRTPorRTCP(m_pDollarBuffer0[1],m_pDollarBuffer,m_nDollarLen);
            m_pVideoSess->DealRecvPacket((unsigned char *)m_pDollarBuffer,m_nDollarLen);
            m_pDollarBuffer = NULL;
            m_nDollarLen = 0;
            m_nDollarPos = 0;
            m_nDollarPos0 = 0;
            m_bDollar = 0;
            continue;

        }

        if (m_nRecvRawBufferPos - m_nRecvRawUsePos < 2)
        {
            memcpy(m_pDollarBuffer0,m_pRecvRawBuffer + m_nRecvRawUsePos,m_nRecvRawBufferPos - m_nRecvRawUsePos);
            m_nDollarPos0 += m_nRecvRawBufferPos - m_nRecvRawUsePos;
            m_nRecvRawUsePos = 0;
            m_nRecvRawBufferPos = 0;
            bNeedMore = 1;
            break;
        }

        m_bDollar = 1;
        m_nDollarPos = 0;
        m_nDollarLen = 0;
        memcpy(m_pDollarBuffer0,m_pRecvRawBuffer + m_nRecvRawUsePos,2);
        m_nRecvRawUsePos += 2;
        m_nDollarPos0 = 2;
    }
    while(1);
    return bNeedMore;

}
CMessage * CRtspClient::Parse(char *pData,int nDataSize,int bExt)
{//假定 N个完整消息
    int i,npos;
    char c,c1;
    CMessage tmpMsg,*pMsg = NULL,*pHead = NULL,*pTail = NULL;
    int crlf_cnt = 0;
    int Pos,Len,MsgPos,MsgLen;
    int bq = 0;

    if (nDataSize <= 0)
    {
        return pHead;
    }

 //RTSP_DEBUG("[%d]C->:%d\n[%s]\n",m_nSocket,nDataSize,pData);
if(!bExt)
{
    if (m_bDollar)
    {
        if (m_pDollarBuffer != NULL)
        {
            if (m_nDollarLen > m_nDollarPos + nDataSize)
            {
                memcpy(m_pDollarBuffer + m_nDollarPos - 4,pData,nDataSize);
                m_nDollarPos += nDataSize;
                nDataSize = 0;
            }
            else
            {
                memcpy(m_pDollarBuffer + m_nDollarPos - 4,pData,m_nDollarLen - m_nDollarPos);
                pData += m_nDollarLen - m_nDollarPos;
                nDataSize -= m_nDollarLen - m_nDollarPos;
                pMsg = new CMessage;
                if (pMsg != NULL)
                {
                    pMsg->m_nMsgLen = m_nDollarLen - 4;
                    pMsg->m_pMessage = m_pDollarBuffer;
                    pMsg->m_bAllocFlag = 1;
                    pMsg->m_nDollar_ch = m_pDollarBuffer0[1];


                    pMsg->m_nLineCnt = 1;
                    pMsg->m_nSegCnt[0] = 1;
                    pMsg->m_nPos[0][0] = 0;
                    pMsg->m_nLen[0][0] =  m_nDollarLen - 4;
                    pMsg->m_bDollar = 1;



                    if (pTail == NULL)
                    {
                        pHead = pTail = pMsg;
                    }
                    else
                    {
                        pTail->m_pNext = pMsg;
                        pTail = pMsg;
                    }




                }
                m_nDollarPos = 0;
                m_nDollarLen = 0;
                m_bDollar = 0;
                m_pDollarBuffer = NULL;

            }
        }
        else
        {
            //
            if (m_nDollarPos >= 4)
            {//说明内存分配失败
                if (m_nDollarLen > m_nDollarPos + nDataSize)
                {
                    //memcpy(m_pDollarBuffer + m_nDollarPos,pData,nDataSize);
                    m_nDollarPos += nDataSize;
                    nDataSize = 0;
                }
                else
                {
                    //memcpy(m_pDollarBuffer + m_nDollarPos,pData,m_nDollarLen - m_nDollarPos);
                    pData += m_nDollarLen - m_nDollarPos;
                    nDataSize -= m_nDollarLen - m_nDollarPos;
                    m_nDollarPos = m_nDollarLen;

                    m_nDollarPos = 0;
                    m_nDollarLen = 0;
                    m_bDollar = 0;
                    m_pDollarBuffer = NULL;

                }
            }
            else
            {
                //说明长度尚未获取
                if (m_nDollarPos + nDataSize >= 4)
                {//可以获取长度
                    int Len;
                    memcpy(m_pDollarBuffer0 + m_nDollarPos,pData,4 - m_nDollarPos);
                    pData += 4 - m_nDollarPos;
                    nDataSize -= 4 - m_nDollarPos;
                    m_nDollarPos = 4;
                    Len = htons(*((short *)&m_pDollarBuffer0[2]));
                    m_nDollarLen = Len + 4;
                    m_pDollarBuffer = new char [m_nDollarLen];
                    if (m_pDollarBuffer != NULL)
                    {
                        //memcpy(m_pDollarBuffer,m_pDollarBuffer0,4);
                    }

                    if (m_nDollarLen > m_nDollarPos + nDataSize)
                    {
                        if (m_pDollarBuffer != NULL)
                        {
                            memcpy(m_pDollarBuffer + m_nDollarPos-4,pData,nDataSize);
                        }
                        m_nDollarPos += nDataSize;
                        nDataSize = 0;
                    }
                    else
                    {
                        if (m_pDollarBuffer != NULL)
                        {
                            memcpy(m_pDollarBuffer + m_nDollarPos - 4,pData,m_nDollarLen - m_nDollarPos);
                        }

                        pData += m_nDollarLen - m_nDollarPos;
                        nDataSize -= m_nDollarLen - m_nDollarPos;
                        m_nDollarPos = m_nDollarLen;
                        if (m_pDollarBuffer != NULL)
                        {
                            pMsg = new CMessage;
                            if (pMsg != NULL)
                            {
                                pMsg->m_nMsgLen = m_nDollarLen - 4;
                                pMsg->m_pMessage = m_pDollarBuffer;
                                pMsg->m_bAllocFlag = 1;
                                pMsg->m_nDollar_ch = m_pDollarBuffer0[1];

                                pMsg->m_nLineCnt = 1;
                                pMsg->m_nSegCnt[0] = 1;
                                pMsg->m_nPos[0][0] = 0;
                                pMsg->m_nLen[0][0] =  m_nDollarLen - 4;
                                pMsg->m_bDollar = 1;


                                if (pTail == NULL)
                                {
                                    pHead = pTail = pMsg;
                                }
                                else
                                {
                                    pTail->m_pNext = pMsg;
                                    pTail = pMsg;
                                }

                            }

                        }

                        m_nDollarPos = 0;
                        m_nDollarLen = 0;
                        m_bDollar = 0;
                        m_pDollarBuffer = NULL;

                    }
                }
                else
                {
                    //仍然不能获取


                    memcpy(m_pDollarBuffer0 + m_nDollarPos,pData,nDataSize);
                    m_nDollarPos += nDataSize;
                    nDataSize = 0;

                }
            }
        }
    }
    else
    {
        if (m_pDollarBuffer != NULL)
        {
            delete []m_pDollarBuffer;
            m_pDollarBuffer = NULL;
        }

    }
}
if (nDataSize <= 0)
{
    return pHead;
}

    //请求命令

    crlf_cnt = 0;
    c = pData[0];
    Pos = 0;
    Len = 0;
    MsgPos = 0;


    //////////////////////////////////////////////////////////////////////////
    //	memcpy(pData+nDataSize+1,pData,nDataSize/2);
    //	pData[nDataSize]=LF;
    //	nDataSize = nDataSize *3/2+1;
    //////////////////////////////////////////////////////////////////////////
    for (i = 1; i < nDataSize; i++)
    {

        c1 = pData[i];

        if ((!bExt) && c == '$')
        {//$[1Byte ChID][2bytes Length][data][without CRLF]
            int Len,CurrLen,ch;
            if (nDataSize - i < 3)
            {//遇到错误
                m_bDollar = 1;
                m_nDollarLen = 0;
                m_nDollarPos = nDataSize - i + 1;
                m_pDollarBuffer = NULL;
                memcpy(m_pDollarBuffer0,&pData[i - 1],nDataSize - i + 1);

                return pHead;
            }
            ch = c1;
            Len = htons(*((short *)&pData[i + 1]));
       //     RTSP_DEBUG("$ch = %d,len = %d ,sock = %d\n",c1,Len,m_nSocket);



            CurrLen = Len;
            if (CurrLen > nDataSize - i - 3)
            {
                CurrLen = nDataSize - i - 3;
            }


            if (Len > nDataSize - i - 3)
            {//未完,遇到错误

                m_bDollar = 1;
                m_nDollarLen = Len + 4;
                m_nDollarPos = nDataSize - i + 1;
                m_pDollarBuffer = new char [m_nDollarLen];
                if (m_pDollarBuffer != NULL)
                {
                    memcpy(m_pDollarBuffer,&pData[i - 1] + 4,nDataSize - i + 1 - 4);
                    memcpy(m_pDollarBuffer0,&pData[i - 1],4);
                }


                return pHead;
            }


            //
            pMsg = new CMessage;
            if (pMsg != NULL)
            {
                pMsg->m_nMsgLen = Len;
                pMsg->m_pMessage = pData + i -1 + 4;


                pMsg->m_nLineCnt = 1;
                pMsg->m_nSegCnt[0] = 1;
                pMsg->m_nPos[0][0] = 0;
                pMsg->m_nLen[0][0] =  Len;
                pMsg->m_bDollar = 1;
                pMsg->m_nDollar_ch = c1;



                if (pTail == NULL)
                {
                    pHead = pTail = pMsg;
                }
                else
                {
                    pTail->m_pNext = pMsg;
                    pTail = pMsg;
                }




            }



            tmpMsg.Reset();



            i += Len + 4 - 1;
            c = pData[i];

            MsgPos = i;
            Pos = i;
            crlf_cnt = 0;

            continue;

        }
        else if (c == '\"')
        {
            bq = !bq;
        }
        else if ((bq) && (c == ' '))
        {
        }
        else if ((!bq) && (c == ' '/* || c == ';'*/))
        {//当前消息,当前行,某字段结束
            crlf_cnt = 0;
            if (MsgPos == i - 1)
            {
                MsgPos = i;
            }
            if (Pos == i - 1)
            {//去掉多个空格


            }
            else
            {

                tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,i-1-Pos);




            }
            Pos = i;//从非空格开始
        }
        else if (c == CR || c1 == LF)
        {
            if(c1 == LF)
            {
                bq = 0;
                crlf_cnt++;
                if (crlf_cnt > 1)
                {//消息结束,开始检查下一条消息

                    tmpMsg.m_nMsgLen = i - MsgPos + 1;
                    tmpMsg.m_pMessage = (char *)pData + MsgPos;
                    if (tmpMsg.m_bHasContext)
                    {//

                        tmpMsg.m_pContext = (uint8_t *)pData + i + 1;
                        tmpMsg.m_nContextLength = nDataSize - i - 1;
                        i = nDataSize;

                    }


                    pMsg = new CMessage;
                    if (pMsg != NULL)
                    {

                        if (!pMsg->CopyMessage(tmpMsg))
                        {

                            if (pTail == NULL)
                            {
                                pHead = pTail = pMsg;
                            }
                            else
                            {
                                pTail->m_pNext = pMsg;
                                pTail = pMsg;
                            }


                        }
                        else
                        {
                            delete pMsg;
                        }
                    }
                    tmpMsg.Reset();

                    MsgPos = i;

                }
                else
                {
                    //当前消息,某一行结束,开始检查下一行
                    if (tmpMsg.m_nLineCnt < MSG_MAX_LINENUMS)
                    {
                        tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,i-1-Pos + (c != CR));
                        tmpMsg.IncLine();

                    }



                }

                Pos = i;


            }
            else
            {
                //格式不对,忽略
                if (c == CR || c == LF || c == ' ' || c == 0)
                {
                    if (MsgPos == i - 1)
                    {
                        MsgPos = i;
                    }
                    if (Pos == i - 1)
                    {
                        Pos = i;
                    }
                }
            }

        }
        else
        {
            if (crlf_cnt && c == LF)
            {
            }
            else
            {
                crlf_cnt = 0;
            }
            if (c == CR || c == LF || c == ' ' || c == 0)
            {
                if (MsgPos == i - 1)
                {
                    MsgPos = i;
                }
                if (Pos == i - 1)
                {
                    Pos = i;
                }
            }

        }

        c = c1;


    }

    if (MsgPos == 0)
    {
        if (bExt || crlf_cnt == 1)
        {
            tmpMsg.m_pMessage = pData;
            tmpMsg.m_nMsgLen = nDataSize;
        }
        else
        {
            tmpMsg.m_bHasContext = 1;
            tmpMsg.m_pContext = (uint8_t *)pData;
            tmpMsg.m_nContextLength = nDataSize;
        }


        pMsg = new CMessage;
        if (pMsg != NULL)
        {

            if (!pMsg->CopyMessage(tmpMsg))
            {

                if (pTail == NULL)
                {
                    pHead = pTail = pMsg;
                }
                else
                {
                    pTail->m_pNext = pMsg;
                    pTail = pMsg;
                }


            }
            else
            {
                delete pMsg;
            }
        }
    }

    return pHead;

}


#endif

