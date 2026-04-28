#ifndef _APPRTPSESSION_H_
#define _APPRTPSESSION_H_


#include "rtpsession.h"

#include "rtsp_common.h"

using namespace jrtplib ;

#pragma pack(push,1)





#define APPRTPSESSION_RECV_BUFFSIZE     (4 * 1024)
#define APPRTPSESSION_RECV_BUFFSIZE_INC  (4 * 1024)  //每次增加数,最大不超过 H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X = 1M
#define APPRTPSESSION_RECV_BUFFSIZE_X     (1024 * 1024 /APPRTPSESSION_RECV_BUFFSIZE)

#define DATAPACKNODE_NUM     512



#pragma pack(pop)



class CAppRtpSession:public RTPSession
{
public:
    CAppRtpSession();
    ~CAppRtpSession();
	int SetDefaultPayloadType(uint8_t pt);
	int GetDefaultPayloadType(){return m_nDefaultPayloadType;}

	/** Sets the default marker for RTP packets to \c m. */
	int SetDefaultMark(bool m);
     int SendAppPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
    time_t GetAliveTime(){return m_tAliveTime;}
    
    void SetClientCallback(void *hClient,ANTS_RTPSESSION_CLIENT_CALLBACK sessCallback,void *pUser);
    void SetSpecialFlag(int bFlag){m_bSpecial_Ex = bFlag;}
    void SetRecvDataBuffer(void *pDataBuff,size_t nDataBuffSize);

    int DealRecvPacket(unsigned char *pRecvData,int nDataLen);

  

    void SetProp(unsigned int dwProp)
    {
        if (dwProp & 2)
        {
            m_bSpecial_Ex = 1;
        }
        m_dwProp = dwProp;

    }
    int IsReady()
    {
        return m_bVideoReady;
    }

 
    
protected:
    

private:
    int SplitPacket(const void *data,size_t len,uint32_t timestamp);
 
	bool m_bDefaultMark;
	int m_nDefaultPayloadType;
 
 
   


    uint8_t m_pPackBuf[1500 + 2];
    uint8_t *m_pRecvDataBuff;
    int m_nRecvDataPos;
    int m_nRecvDataStart;
    int m_nRecvDataBuffSize;
    int m_bFrameDealing;

    uint64_t m_nLastTimestamp;

    JMutex m_hMemLock;
    uint16_t m_nLastSeq;
    uint32_t m_nframe_num;
    uint32_t m_nLastframe_num;
    int m_bLastMark;

    uint32_t m_uiFrameNo;
    uint32_t m_uiLastFrameNo;
 
    int m_bVideoReady; // 服务器 端INPUT一I帧数据，

    ANTS_RTPSESSION_CLIENT_CALLBACK m_pRtpSessionClientFxn;
    void *m_pRtpSessionClientHandle;
    void *m_pRtpSessionClientUser;
    time_t m_tAliveTime;
    
    int64_t m_nLastStamp;
    uint32_t m_nCurrStamp;

    unsigned int test_seq ;


    int m_bSpecial_Ex;
    int m_bLastRecvError;

  

};


#endif
