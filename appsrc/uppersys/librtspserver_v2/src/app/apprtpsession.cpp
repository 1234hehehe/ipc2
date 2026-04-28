#include "apprtpsession.h"
#include "rtsp_common.h"
#include "rtsp.h"








class CRtspServer;

CAppRtpSession::CAppRtpSession(){
    m_nLastStamp = 0;m_bDefaultMark = 0; m_nDefaultPayloadType = PAYLAODTYPE_H264;m_tAliveTime = 0;
    m_uiFrameNo = 0;
    m_uiLastFrameNo = 0;
    m_bFrameDealing = 0;
    
    m_pRtpSessionClientFxn = NULL;
    m_pRtpSessionClientHandle = NULL;
    m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
    if (m_pRecvDataBuff != NULL)
    {
        m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
    }
    else
    {
        m_nRecvDataBuffSize = 0;
    }
   
    m_nRecvDataPos = 0;
    m_nRecvDataStart = 0;
    m_nRecvDataStart = sizeof(AntsFrameHeader);
   
    m_nLastSeq= 0;
  
 
    m_bSpecial_Ex = 0;
 
   
  
 
    m_nLastTimestamp = 0;
    m_nLastframe_num = 0;
 
   
    m_nCurrStamp = 0;
    m_bLastRecvError = 0;
  
    SetPayloadClockRate(90000);
    m_hMemLock.Init();
}
CAppRtpSession::~CAppRtpSession()
{
    m_hMemLock.Lock();
    if (m_pRecvDataBuff != NULL)
    {
        free(m_pRecvDataBuff);
        m_pRecvDataBuff = NULL;
    }
    m_nRecvDataBuffSize = 0;
   
    m_hMemLock.Unlock();
    
}

int CAppRtpSession::SetDefaultPayloadType(uint8_t pt)
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
int CAppRtpSession::SetDefaultMark(bool m)
{
	int status;
	status = RTPSession::SetDefaultMark(m);
	if (!status)
	{
		m_bDefaultMark = m;
	}
	return status;

}

 int CAppRtpSession::SplitPacket(const void *data,size_t len,uint32_t timestamp)
 {//输入完整NAL数据,不包括起始码
     int PackSize = GetPacketMaxDataSize();
     int nPos;
     int bMark;
     int nSendSize;
     int nRet = -1;
     uint8_t *pPackBuf = NULL;

     
     pPackBuf = GetPacketBuffer();
     if (pPackBuf == NULL)
     {
         RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
         return -1;
     }
     
     // printf("[%s] len = %d PackSize = %d type = %d nri = %d\n",__FUNCTION__,len,PackSize,pNALU->TYPE,pNALU->NRI);
     
     if (len <= PackSize)
     {
        return SendPacket(data,len,1,timestamp);
     }
     //开始分片
     nPos = 0;
     bMark = 0;
     nSendSize = 0;

     while(1)
     {
         
         bMark = 0;
         
         
        if (nPos == 0)
        {
            nSendSize = PackSize;
            memcpy(pPackBuf,(char *)data + nPos,nSendSize);
            
            nPos += nSendSize;

          
        }
        else
        {
            if (PackSize>= len - nPos)
            {//尾
                bMark = 1;
                nSendSize = len - nPos;
                memcpy(pPackBuf,(char *)data + nPos,nSendSize);
                nPos += nSendSize;

       
     
            }
            else
            {
                nSendSize = PackSize;
                memcpy(pPackBuf,(char *)data + nPos,nSendSize);
                nPos += nSendSize;

            }
            
        }

        nRet = SendPacket(pPackBuf,nSendSize,bMark,timestamp);
        if (nRet)
        {
            break;
        }
        if (bMark)
        {
            break;
        }
     }

     return nRet;

     

 }




 int CAppRtpSession::SendAppPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
 {
     int nRet = -1;
     unsigned int timestampinc;

     uint32_t uStartCode = 0;
     uint32_t *p32;


     int nmul ;
     if (data == NULL)
     {
         return nRet;
     }
     if(0 == GetDestinationCnt())
     {
         return -1;
     }
    
  
     nmul = 90;//CRtspServer::GetPayloadClockRate(m_nDefaultPayloadType) / 1000;
     nmul = GetPayloadClockRate() / 1000;

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
  }
  else
  {
    int64_t timestamp = AbstimestampSec * 1000;
            timestamp += AbstimestampUSec / 1000;
     if (m_nLastStamp == 0 || timestamp < m_nLastStamp)
     {

         timestampinc = 40 * nmul;
     }
     else
     {
         timestampinc = (timestamp - m_nLastStamp) * nmul;
     }

  //   if (timestampinc != 40 * 90)
    // {
      //   printf("timestampinc = %d,%lld,%lld\n",timestampinc,timestamp,m_nLastStamp);
  //   }
     m_nLastStamp = timestamp;

     m_nCurrStamp += timestampinc;
  }
  
  //分片..
  nRet = SplitPacket(data,len,m_nCurrStamp);
  
     return nRet;
 }




void CAppRtpSession::SetClientCallback(void *hClient,ANTS_RTPSESSION_CLIENT_CALLBACK sessCallback,void *pUser)
{
    m_pRtpSessionClientFxn = sessCallback;
    m_pRtpSessionClientHandle = hClient;
    m_pRtpSessionClientUser = pUser;

}
void CAppRtpSession::SetRecvDataBuffer(void *pDataBuff,size_t nDataBuffSize)
{
  
    return;
//    m_pRecvDataBuff = (uint8_t *)pDataBuff;
 //   m_nRecvDataBuffSize = nDataBuffSize;
 //   m_nRecvDataStart = sizeof(AntsFrameHeader) + 4;
 //   m_nRecvDataPos = 0;
}




int CAppRtpSession::DealRecvPacket(unsigned char *pRecvData,int nDataLen)
{
    unsigned char *pdata = NULL,*pDstData = NULL;
    int npktlen;
    AntsFrameHeader *pFrame;
    int Slicetype;
    unsigned short nSeq,nReqSeq;
    unsigned int nTimestamp;
    int bSeqError = 0;
    int CurrLen;
    int Profile_idc;
    int Constraint_sets;
    int Level_idc;
    int cc;
    int nRet;
    int bFrame = 0;
    int type;
    int b00_00_01 = 0,b00_00_00_01 = 0;

    int version;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    int bMark = 0;
    //printf("len = %d\n",nDataLen);
    version = (pRecvData[0] >> 6)&0x3;
    if(version != 2)
    {
        return 0;
    }
    type = pRecvData[1] & 0x7F;
    bMark = (pRecvData[1] >> 7) & 1;
    if(type != m_nDefaultPayloadType)
    {
        m_nRecvDataPos = 0;
        m_bFrameDealing = 0;
        free(m_pRecvDataBuff);
        m_pRecvDataBuff = NULL;
        m_nRecvDataBuffSize = 0;
        // RTSP_ERROR("invalid Video Payloadtype = %d\n",type);
        if (m_pRtpSessionClientFxn)
        {
            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_ERROR,ANTS_RTSP_CALLBACKBYPE_ERROR_PAYLAODTYPE,0,type,NULL,0,m_pRtpSessionClientUser);
        }
        return 0;
    } 
    m_hMemLock.Lock();
    if (m_bLastRecvError)
    {
        m_bLastRecvError = 0;
        m_bFrameDealing = 0;
        m_nRecvDataPos = 0;
        if (m_pRecvDataBuff)
        {
           free(m_pRecvDataBuff);
        }
        m_pRecvDataBuff = NULL;
        m_nRecvDataBuffSize = 0;
    }
     m_hMemLock.Unlock();
    OnRtpRtcpPacket(pRecvData,nDataLen);
    if (nDataLen >= 2)
    {
        uint8_t checkRTCP;
        checkRTCP = pRecvData[1];
        if (checkRTCP >= 200 && checkRTCP <= 204)
        {// rtcp
            //printf("rtcp\n");
            return 0;
        }
    }


    m_hMemLock.Lock();
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = (uint8_t *)malloc(APPRTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = APPRTPSESSION_RECV_BUFFSIZE;
        }
    }
    if (m_pRecvDataBuff == NULL)
    {
         m_bLastRecvError=1;
        m_hMemLock.Unlock();
        RTSP_ERROR("Memory malloc failed\n");
        return -1;
    }
    m_hMemLock.Unlock();
      // printf("%u\n",packet->GetTimestamp());
    //nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
    nSeq = (pRecvData[2] << 8) | pRecvData[3];
   // nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
    nTimestamp = (pRecvData[4] << 24) | (pRecvData[5] << 16) | (pRecvData[6] << 8) | pRecvData[7];
   //printf("time = %d \n",nTimestamp);
#if 0
    {
        int xx;
        for (xx = 0; xx < 12; xx++)
        {
            printf("%02x ",((uint8_t *)pRecvData)[xx]);
        }
        printf("\n");
    }
#endif
    cc = pRecvData[0] &0x0F;

    bExtHeader = (pRecvData[0] >> 4) & 1;
    

    pdata = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    npktlen = nDataLen - 12 - cc * 4;//packet->GetPayloadLength();

    if(bExtHeader)
    {
        uint16_t extid;
        uint16_t length;
       // pExtHeader = (struct RTPExtensionHeader *)(pdata);
       // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
       // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();
        
    }

    if (npktlen <= 0 || npktlen > APPRTPSESSION_RECV_BUFFSIZE_INC * APPRTPSESSION_RECV_BUFFSIZE_X)
    {
        m_bLastRecvError = 1;
        RTSP_ERROR("packet length err %d\n",npktlen);
        return -1;
    }
    //printf("curr = %d,last = %d\n",nSeq,m_nLastSeq);
    bSeqError = 0;
    if (m_nLastSeq != 0)
    {
        nReqSeq = m_nLastSeq + 1;
        if (nSeq != nReqSeq)
        {
            RTSP_ERROR("[RTSP]lost seq no. %d <= %d\n",nSeq,m_nLastSeq);
            bSeqError = 1;
        }
    }
    m_nLastSeq = nSeq;
   

    if (bSeqError)
    {

        m_bLastRecvError = 1;
        m_bLastMark = bMark;
        //RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
        return -1;
    }
     if(m_bLastMark)
     {
         // 新一包起始
         m_nRecvDataPos = 0;
     }
     m_bLastMark = bMark;
     

    


    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > APPRTPSESSION_RECV_BUFFSIZE * APPRTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bLastRecvError = 1;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * APPRTPSESSION_RECV_BUFFSIZE_INC > APPRTPSESSION_RECV_BUFFSIZE * APPRTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bLastRecvError = 1;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }  
            if (CurrLen <= m_nRecvDataBuffSize + x * APPRTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * APPRTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bLastRecvError = 1;
                    m_hMemLock.Unlock();
                    return -1;
                }
                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
               // pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * APPRTPSESSION_RECV_BUFFSIZE_INC;
                //free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();



    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;

    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
    
    memcpy(pDstData,pdata,npktlen);
    m_nRecvDataPos += npktlen;
    if (bMark)
    {
        memset(pFrame,0,sizeof(AntsFrameHeader));
        m_uiFrameNo++;
        pFrame->uiStartId = ANTS_APP_STARTCODE;
        pFrame->uiFrameType = AntsPktAppFrames;
        pFrame->uiFrameNo = m_uiFrameNo;
        Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate();
        pFrame->uiFrameTime = Tmptimestamp;
        Tmptimestamp = (m_nLastTimestamp * 1000 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
        pFrame->uiFrameTickCount = Tmptimestamp;
        pFrame->uiFrameLen = m_nRecvDataPos;
        Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
        pFrame->uiTimeStamp = Tmptimestamp;
        pFrame->uMedia.struAppHeader.byAppPayloadType  = 0;
        
        if (m_pRtpSessionClientFxn != NULL)
        {
            int prop = 0;
            if(m_dwProp & 1)
                prop = 1;
            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_APP,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
            if (prop)
            {
                m_pRecvDataBuff = NULL;
            }
        }
        m_nRecvDataPos = 0;
        bFrame = 1;
        return bFrame;
    }
    return 0;
 
   

}








