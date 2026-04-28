#include "audiortpsession.h"
#include "rtsp.h"
#include "rtspclient_api.h"
class CRtspServer;

#define ANTS_RTSP_G711A      1
#define ANTS_RTSP_G711U      2

CAudioRtpSession::CAudioRtpSession()
{
    m_nLastStamp = 0;
    m_bDefaultMark=0;
    m_nDefaultPayloadType=-1;//PAYLAODTYPE_G711U;
    m_tAliveTime = 0;
    m_uiFrameNo = 0;
    m_bFrameDealing = 0;
    m_pRtpSessionClientFxn = NULL;
    m_pRtpSessionClientHandle = NULL;
    m_nRecvDataBuffSize = AUDIORTPSESSION_RECV_BUFFSIZE;
    m_nRecvDataPos = 0;
    m_nRecvDataStart = 0;
    m_nCurrStamp = 0;
    m_nLastSeq = 0;
    m_nFrameCnt = 0;
    SetPayloadClockRate(8000);
    m_nRecvDataStart = sizeof(AntsFrameHeader);
    m_pRecvDataBuff = NULL;

}

CAudioRtpSession::~CAudioRtpSession()
{
    if(m_pRecvDataBuff != NULL)
    {
        free(m_pRecvDataBuff);
        m_pRecvDataBuff = NULL;
    }
}
int CAudioRtpSession::SetDefaultPayloadType(uint8_t pt)
{
	int status;
	status = RTPSession::SetDefaultPayloadType(pt);
	if (!status)
	{
		m_nDefaultPayloadType = pt;
	}

	return status;


}

/** Sets the default marker for RTP packets to \c m. */
int CAudioRtpSession::SetDefaultMark(bool m)
{
	int status;
	status = RTPSession::SetDefaultMark(m);
	if (!status)
	{
		m_bDefaultMark = m;
	}
	return status;

}

int CAudioRtpSession::Send_Adpcm2G711U_Packet(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)//毫秒
{
    int nRet = -1;
    short outlen = 0;


   return SendAudioPacket(data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);



}

int CAudioRtpSession::SendAudioPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{
    uint32_t *p32;
    char *pNalStart,*pNalEnd,*pCurr;
    int NalLen;
    int nRet = 0;
	int nmul ;
    uint32_t timestampinc;
     uint32_t msw = 0,lsw = 0;
     int nPktNum = 0;
     int nPos,i;
     int PackSize = GetPacketMaxDataSize();
     int bMark = 0;

    if(0 == GetDestinationCnt())
    {
        return -1;
    }
    if (data == NULL)
    {
        return -1;
    }

    if(m_uiDefaultPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_AAC)
    {
        m_nCurrStamp += 1024;
        //printf("Reltimestamp = %u[%u]\n",Reltimestamp,m_nCurrStamp);
    }
    else
    {
        nmul = GetPayloadClockRate()  / 1000;
       int nFps = GetFrameRate();
        if (nFps > 0)
        {
            m_nCurrStamp += nmul * 1000 / nFps;
        }
        else
        {
            if (bTimeStampValid & 1)
            {
#if 0
                if (m_nLastStamp == 0 || timestamp < m_nLastStamp)
                {
                    if (timestamp < m_nLastStamp)
                    {
                        timestampinc = (timestamp + 0xFFFFFFFF - m_nLastStamp)/90;
                        if (timestampinc > 100)
                        {
                            timestampinc = 40;
                        }
                        timestampinc *= nmul;
                    }
                    else
                    {
                        timestampinc = 40 * nmul ;
                    }
                }
                else
                {
                    timestampinc = (timestamp - m_nLastStamp)/90 * nmul;
                }
#endif
                m_nCurrStamp = Reltimestamp;
                //printf("timestamp = %lld\n",m_nCurrStamp);
            }
            else
            {
        	    int64_t timestamp = AbstimestampSec * 1000;
                timestamp += AbstimestampUSec / 1000;


                if (m_nLastStamp == 0 || timestamp < m_nLastStamp)
                {
        		    timestampinc = 40 * nmul * 2 ;
                }
                else
                {
                    timestampinc = (timestamp - m_nLastStamp) * nmul*2 ;
                }
                timestampinc = len;
                //printf("timestampinc = %d\n",timestampinc);
                m_nLastStamp = timestamp;
                m_nCurrStamp += timestampinc;
             }
        }
    }
  // printf("m_bDefaultMark = %d %d\n",m_bDefaultMark,timestampinc);
    //Ants_rtsp_GetNTPTime(&msw,&lsw);
    if ((bTimeStampValid & 2) && m_bRecording)
    {
        Ants_rtsp_GetRTP2NTPTime(AbstimestampSec,AbstimestampUSec,&msw,&lsw);
        SetRecordNTP(msw,lsw);
    }
    nPktNum = (len + PackSize - 1) / PackSize;
    nPos = 0;
 // printf("audio bRelTimeStam = %d stamp = %u\n",bRelTimeStam,m_nCurrStamp);
    for (i = 0; i < nPktNum;i++)
    {
        if (PackSize > len - nPos)
        {
            PackSize = len - nPos;
        }
        if (i == nPktNum - 1)
        {
            bMark = 1;
        }
        nRet = SendPacket((uint8_t *)data + nPos,PackSize,bMark,m_nCurrStamp + nPos);
        nPos += PackSize;
    }




    return nRet;
}
void CAudioRtpSession::SetClientCallback(void *hClient,ANTS_RTPSESSION_CLIENT_CALLBACK sessCallback,void *pUser)
{

    m_pRtpSessionClientHandle = hClient;
    m_pRtpSessionClientUser = pUser;
    m_pRtpSessionClientFxn = sessCallback;

}


int CAudioRtpSession::DealRecvPacket(unsigned char *pRecvData,int len)
{
    unsigned char *pData,*pDstData;
    int nDatalen;
    int nType;
    int cc;
    AntsFrameHeader *pheader = NULL;
    uint64_t nTimestamp;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    int version = 0;
    struct RTPExtensionHeader *pExtHeader = NULL;
    uint32_t dwSec = 0,dwUSec = 0,bAbs = 0;
    version = (pRecvData[0] >> 6)&0x3;
    if(version != 2)
    {
        return 0;
    }
    if (len >= 2)
    {
        uint8_t checkRTCP;
        checkRTCP = pRecvData[1];
        if (checkRTCP >= 200 && checkRTCP <= 204)
        {// rtcp
            return 0;
        }
    }
    if (len < 12)
    {
        return -1;
    }

    //  struct timeval currtime;
    cc = pRecvData[0] &0x0F;

    bExtHeader = (pRecvData[0] >> 4) & 1;


    pData = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    nDatalen = len - 12 - cc * 4;//packet->GetPayloadLength();

    if(bExtHeader)
    {
        uint16_t length;
        RTPExtensionHeader tExtHeader;
        RTPExtensionOnvifHeader tExtOnvifHeader;
        memcpy(&tExtHeader,pData,4);
        if (tExtHeader.extid == htons(0xABAC))
        {// onvif
            if (htons(tExtHeader.length) >= 3)
            {
                memcpy(&tExtOnvifHeader,pData + sizeof(RTPExtensionHeader),htons(tExtHeader.length) * 4);
                bAbs = 1;
                Ants_rtsp_GetNTP2RTPTime(htonl(tExtOnvifHeader.ntpTimeStamp0),htonl(tExtOnvifHeader.ntpTimeStamp1),&dwSec,&dwUSec);
            }

        }
        //pExtHeader = (struct RTPExtensionHeader *)(pData);
        //pData += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        //nDatalen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pData[2] << 8) | (pData[3]);
        pData += 4 + length * 4;//(char *)packet->GetPayloadData();
        nDatalen -= 4 + length * 4;//packet->GetPayloadLength();
    }
    nType = pRecvData[1] & 0x7F;//packet->GetPayloadType();
    //nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
  //  nSeq = (pRecvData[2] << 8) | pRecvData[3];
    // nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
    nTimestamp = (pRecvData[4] << 24) | (pRecvData[5] << 16) | (pRecvData[6] << 8) | pRecvData[7];

    if (nType != m_nDefaultPayloadType)
    {
        return -1;
    }

   // if (nType == 0 || nType == 8)
    if (GetRTSPStreamType() == ANTS_RTSP_STREAM_G711A ||
        GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U)
    {
        return DealRecvPacket_G711(pRecvData,len);
    }

    if (nDatalen > AUDIORTPSESSION_RECV_BUFFSIZE)
    {
        RTSP_DEBUG("audio error : %d\n",__LINE__);
        return -1;
    }
    // gettimeofday(&currtime,NULL);
    // printf("%d-%d\n",packet->GetTimestamp(),RTPTime::CurrentTime().GetMicroSeconds()/1000);
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = ( unsigned char*)malloc(AUDIORTPSESSION_RECV_BUFFSIZE);
    }
    if (m_pRecvDataBuff == NULL)
    {
        return -1;
    }
    m_nRecvDataStart = sizeof(AntsFrameHeader);
    m_nRecvDataPos = 0;
    pDstData =(unsigned char *) m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos;
    pheader = (AntsFrameHeader *)m_pRecvDataBuff;
    memset(pheader,0,sizeof(AntsFrameHeader));
    pheader->uiStartId = ANTS_FRAME_STARTCODE;
    pheader->uiFrameType = AntsPktAudioFrames;
    pheader->uiFrameNo = m_uiFrameNo++;
    if (bAbs)
    {
        pheader->uiFrameTime = dwSec;
        pheader->uiFrameTickCount = dwUSec;
    }
    else
    {
        Tmptimestamp = nTimestamp / GetPayloadClockRate() ;
        pheader->uiFrameTime = Tmptimestamp;
        Tmptimestamp = (nTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pheader->uiFrameTime * 1000* 1000);
        pheader->uiFrameTickCount = Tmptimestamp;
    }
    pheader->uiFrameLen = nDatalen;
    Tmptimestamp = nTimestamp * 8000.0/GetPayloadClockRate();
    pheader->uiTimeStamp = Tmptimestamp;
    pheader->uMedia.struAudioHeader.cCodecId = GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U? ANTS_RTSP_G711U:ANTS_RTSP_G711A;
    pheader->uMedia.struAudioHeader.cChannels = 1;
    pheader->uMedia.struAudioHeader.cSampleRate = 8;
    pheader->uMedia.struAudioHeader.cBitRate = 16;
    pheader->uMedia.struAudioHeader.cResolution = 0;

    memcpy(pDstData,pData,nDatalen);
    m_nRecvDataPos += nDatalen;
    if (m_pRtpSessionClientFxn != NULL)
    {
        int prop = 0;
        if(m_dwProp & 1)
            prop = 1;
        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,
            ANTS_RTSP_CALLBACKBYPE_STREAM,
            //nType == 0?ANTS_RTSP_CALLBACKBYPE_STREAM_G711U:ANTS_RTSP_CALLBACKBYPE_STREAM_G711A,
            GetRTSPStreamType(),prop,
            pheader->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
        if (prop)
        {
             m_pRecvDataBuff = NULL;
        }
    }
    return 1;

    return 0;


}

int CAudioRtpSession::DealRecvPacket_G711(unsigned char *pRecvData,int len)
{
    unsigned char *pData,*pDstData;
    int nDatalen;
    int nType;
    int cc;
     unsigned short nSeq,nReqSeq;
    AntsFrameHeader *pheader = NULL;
    uint64_t nTimestamp;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    uint32_t dwSec = 0,dwUSec = 0,bAbs = 0;
   // unsigned char *pUData = (unsigned char *)pRecvData;
    if (len >= 2)
    {
        uint8_t checkRTCP;
        checkRTCP = pRecvData[1];
        if (checkRTCP >= 200 && checkRTCP <= 204)
        {// rtcp
            return 0;
        }
    }
    if (len < 12)
    {
        return -1;
    }

    cc = pRecvData[0] &0x0F;

    bExtHeader = (pRecvData[0] >> 4) & 1;


    pData = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    nDatalen = len - 12 - cc * 4;//packet->GetPayloadLength();

    if(bExtHeader)
    {
        uint16_t length;
        RTPExtensionHeader tExtHeader;
        RTPExtensionOnvifHeader tExtOnvifHeader;
        memcpy(&tExtHeader,pData,4);
        if (tExtHeader.extid == htons(0xABAC))
        {// onvif
            if (htons(tExtHeader.length) >= 3)
            {
                memcpy(&tExtOnvifHeader,pData + sizeof(RTPExtensionHeader),htons(tExtHeader.length) * 4);
                bAbs = 1;
                Ants_rtsp_GetNTP2RTPTime(htonl(tExtOnvifHeader.ntpTimeStamp0),htonl(tExtOnvifHeader.ntpTimeStamp1),&dwSec,&dwUSec);
            }

        }
        //pExtHeader = (struct RTPExtensionHeader *)(pData);
        //pData += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        //nDatalen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pData[2] << 8) | (pData[3]);
        pData += 4 + length * 4;//(char *)packet->GetPayloadData();
        nDatalen -= 4 + length * 4;//packet->GetPayloadLength();

    }





    nType = pRecvData[1] & 0x7F;//packet->GetPayloadType();
    //nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
    nSeq = (pRecvData[2] << 8) | pRecvData[3];
    // nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
    nTimestamp = (pRecvData[4] << 24) | (pRecvData[5] << 16) | (pRecvData[6] << 8) | pRecvData[7];
    //if (nType != 0 && nType != 8)
    if(GetRTSPStreamType() != ANTS_RTSP_STREAM_G711A && GetRTSPStreamType() != ANTS_RTSP_STREAM_G711U)
    {
        return -1;
    }
    if (nDatalen > AUDIORTPSESSION_RECV_BUFFSIZE)
    {
        return -1;
    }

    if (m_nLastSeq != 0)
    {
        nReqSeq = m_nLastSeq + 1;
        if (nSeq != nReqSeq)
        {
            RTSP_ERROR("[RTSP]nType = %d audio lost seq no. %d<= %d \n",nType,nSeq,m_nLastSeq);
           // printf("{%02x %02x %02x %02x,%02x %02x %02x %02x,%02x %02x %02x %02x,%02x %02x %02x %02x}\n",pUData[0],pUData[1],pUData[2],pUData[3],
           //     pUData[4],pUData[5],pUData[6],pUData[7],pUData[8],pUData[9],pUData[10],pUData[11],pUData[12],pUData[13],pUData[14],pUData[15]);
        }
    }
    m_nLastSeq = nSeq;

#if 0
#ifdef WIN32
    {


    SYSTEMTIME SystemTime;

    GetSystemTime(&SystemTime);
    printf("%d -- %d\n",SystemTime.wSecond * 1000 + SystemTime.wMilliseconds, nTimestamp / 8);
    }
#else
    {


    struct timeval currtime;
     gettimeofday(&currtime,NULL);
     printf("%d -- %d\n",currtime.tv_sec * 1000 + currtime.tv_usec /1000, nTimestamp / 8);
    }
#endif
#endif


    // printf("%d-%d\n",packet->GetTimestamp(),RTPTime::CurrentTime().GetMicroSeconds()/1000);
    m_nRecvDataStart = sizeof(AntsFrameHeader);
    //m_nRecvDataPos = 0;
    pDstData =(unsigned char *) m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos;

  // printf("start %d [%u] ->\n",nDatalen,nTimestamp);
    if (m_nRecvDataPos > 0)
    {
        //nTimestamp -= 1.0* m_nRecvDataPos/320 * 40 * 8;
        if (m_nRecvDataPos > nTimestamp)
        {
            nTimestamp = 0;
        }
        else
        {
            nTimestamp -= m_nRecvDataPos;
        }
        //printf(" d[%d] ->",nTimestamp);
    }

    //memcpy(pDstData,pData,nDatalen);
    //m_nRecvDataPos += nDatalen;
    if (m_pRtpSessionClientFxn != NULL)
    {
        int nReadPos = 0,nWriteCnt;

        do
        {
                 if (nReadPos >= nDatalen)
                 {
                      break;
                 }
                 if (m_pRecvDataBuff == NULL)
                 {
                     m_pRecvDataBuff = (unsigned char*)malloc(AUDIORTPSESSION_RECV_BUFFSIZE);
                 }
                 if (m_pRecvDataBuff == NULL)
                 {
                     return -1;
                 }
                 if(m_nRecvDataPos == AUDIO_SIZE_PER_FRAME * m_nFrameCnt)
                 {
                     // 开始

                     pheader = (AntsFrameHeader *)(m_pRecvDataBuff + m_nRecvDataPos);
                     memset(pheader,0,sizeof(AntsFrameHeader));
                     pheader->uiStartId = ANTS_FRAME_STARTCODE;
                     pheader->uiFrameType = AntsPktAudioFrames;
                     pheader->uiFrameNo = m_uiFrameNo++;
                     if (bAbs)
                     {
                         pheader->uiFrameTime = dwSec;
                         pheader->uiFrameTickCount = dwUSec;
                     }
                     else
                     {
                         Tmptimestamp = m_nCurrStamp / GetPayloadClockRate() ;
                         pheader->uiFrameTime = Tmptimestamp;
                         Tmptimestamp = (m_nCurrStamp * 1000.0 * 1000/ GetPayloadClockRate()  - pheader->uiFrameTime * 1000* 1000);
                         pheader->uiFrameTickCount = Tmptimestamp;
                     }

                     Tmptimestamp = m_nCurrStamp * 8000.0/GetPayloadClockRate();
                     m_nCurrStamp += 320;
                     pheader->uiTimeStamp = Tmptimestamp;
                     pheader->uiFrameLen = 320;
                     pheader->uMedia.struAudioHeader.cCodecId = GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U? ANTS_RTSP_G711U:ANTS_RTSP_G711A;
                     pheader->uMedia.struAudioHeader.cChannels = 1;
                     pheader->uMedia.struAudioHeader.cSampleRate = 8;
                     pheader->uMedia.struAudioHeader.cBitRate = 16;
                     pheader->uMedia.struAudioHeader.cResolution = 0;
                     m_nRecvDataPos += sizeof(AntsFrameHeader);
                 }
                 else
                 {
                     nWriteCnt = (m_nFrameCnt + 1) * AUDIO_SIZE_PER_FRAME - m_nRecvDataPos;
                     if (nWriteCnt <= nDatalen - nReadPos)
                     {
                         memcpy(m_pRecvDataBuff + m_nRecvDataPos,pData + nReadPos,nWriteCnt);
                         m_nRecvDataPos += nWriteCnt;
                         nReadPos += nWriteCnt;
                         //m_nCurrStamp += 320;
                         m_nFrameCnt++;
                         if(m_nFrameCnt == AUDIO_PACK_NUM)
                         {
                            // 回调
                             int prop = 0;
                             if(m_dwProp & 1)
                                 prop = 1;
                             m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,
                                 ANTS_RTSP_CALLBACKBYPE_STREAM,
                                 // nType == 0?ANTS_RTSP_CALLBACKBYPE_STREAM_G711U:ANTS_RTSP_CALLBACKBYPE_STREAM_G711A,
                                 GetRTSPStreamType(),prop,
                                 AntsPktAudioFrames,m_pRecvDataBuff,m_nFrameCnt * AUDIO_SIZE_PER_FRAME,m_pRtpSessionClientUser);
                             m_nRecvDataPos = 0;
                             m_nFrameCnt = 0;
                             if (prop)
                             {
                                  m_pRecvDataBuff = NULL;
                             }
                         }

                     }
                     else
                     {
                        nWriteCnt = nDatalen - nReadPos;
                        memcpy(m_pRecvDataBuff + m_nRecvDataPos,pData + nReadPos,nWriteCnt);
                        m_nRecvDataPos += nWriteCnt;
                        nReadPos += nWriteCnt;
                     }
                 }


        }while(1);

    }
    //printf(" last [%d]\n",m_nRecvDataPos);
    return 1;
}


