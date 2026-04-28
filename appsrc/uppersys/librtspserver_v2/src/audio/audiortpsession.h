#ifndef _AUDIORTPSESSION_H_
#define _AUDIORTPSESSION_H_
#ifndef WIN32
#else
#include "hi_voice_api.h"
#endif
#include "rtpsession.h"
#include "rtsp_common.h"
using namespace jrtplib ;

#define PAYLAODTYPE_G711U                    0
#define PAYLAODTYPE_G711A                    8
#define PAYLAODTYPE_G726_16K                    97

#define AUDIO_DEC_MAX_BUFFERSIZE           1024 * 5
#define AUDIO_ENC_MAX_BUFFERSIZE           1024 * 5


#define AUDIO_PACK_NUM     1 // 8 //8÷°“ªµ˜

#define AUDIORTPSESSION_RECV_BUFFSIZE      (64 * 1024)//((36 + 320) * AUDIO_PACK_NUM + 16)

#define AUDIO_SIZE_PER_FRAME    (36 + 320)

class CAudioRtpSession:public RTPSession
{
public:
    CAudioRtpSession();
    ~CAudioRtpSession();
	int SetDefaultPayloadType(uint8_t pt);
	int GetDefaultPayloadType(){return m_nDefaultPayloadType;}

	/** Sets the default marker for RTP packets to \c m. */
	int SetDefaultMark(bool m);
    int SendAudioPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//∫¡√Î
    int Send_Adpcm2G711U_Packet(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//∫¡√Î
    void SetClientCallback(void *hClient,ANTS_RTPSESSION_CLIENT_CALLBACK sessCallback,void *pUser);
    int DealRecvPacket(unsigned char *pRecvData,int len);
    int DealRecvPacket_G711(unsigned char *pRecvData,int len);
    time_t GetAliveTime(){return m_tAliveTime;}
protected:


private:

    int m_bFirstPacket;
	bool m_bDefaultMark;
	int m_nDefaultPayloadType;
#ifdef WIN32
    hiVOICE_ADPCM_STATE_S m_adpcm_dec;
    hiVOICE_G711_STATE_S m_g711U_enc;

    char m_dec_buffer[AUDIO_DEC_MAX_BUFFERSIZE];
    char m_enc_buffer[AUDIO_ENC_MAX_BUFFERSIZE];
#endif	

    unsigned char *m_pRecvDataBuff;
    int m_nRecvDataPos;
    int m_nRecvDataStart;
    int m_nRecvDataBuffSize;
    int m_bFrameDealing;
    int m_nFrameCnt; // 

    unsigned int m_uiFrameNo;
    ANTS_RTPSESSION_CLIENT_CALLBACK m_pRtpSessionClientFxn;
    void *m_pRtpSessionClientHandle;
    void *m_pRtpSessionClientUser;
    uint16_t m_nLastSeq;
    uint32_t m_nCurrStamp;
    time_t m_tAliveTime;
    int64_t m_nLastStamp;
};


#endif
