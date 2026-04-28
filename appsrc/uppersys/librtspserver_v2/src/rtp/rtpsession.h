/*

  This file is a part of JRTPLIB
  Copyright (c) 1999-2011 Jori Liesenborgs

  Contact: jori.liesenborgs@gmail.com

  This library was developed at the Expertise Centre for Digital Media
  (http://www.edm.uhasselt.be), a research center of the Hasselt University
  (http://www.uhasselt.be). The library is based upon work done for 
  my thesis at the School for Knowledge Technology (Belgium/The Netherlands).

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

/**
 * \file rtpsession.h
 */

#ifndef RTPSESSION_H

#define RTPSESSION_H


#include "rtsp_common.h"


#ifdef RTP_SUPPORT_THREAD
	#include <jthread/jmutex.h>	
#endif // RTP_SUPPORT_THREAD

#include <jthread/jmutex.h>
using namespace jthread;
namespace jrtplib
{



typedef struct _tagRTP_PACKET_NODE 
{
    unsigned int nSeqNum;
    unsigned char *pData;
    int nDataLen;
    struct _tagRTP_PACKET_NODE *pPrev;
    struct _tagRTP_PACKET_NODE *pNext;
}RTP_PACKET_NODE_T;

typedef int (*ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK)(void *pClient,unsigned int uiIPv4[4],int nPort,int nCH,int bRtcp,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,int nlen,void *pUser);

#define ANTS_RTP_MAX_NODE_CLIENT_NUM  32
typedef struct _tagIPADDRESS_NODE
{
    int bIPV4;
    int bTCP;
    //tcp
    //udp
    unsigned int uiIPV4[4];
    unsigned int uiPort;
    unsigned int uiPort_rtcp;
    unsigned int uiCH;
    int nUseCnt;
    void *pClient[ANTS_RTP_MAX_NODE_CLIENT_NUM];
    int bNeedIFrame;
    unsigned int dwErrFrameCnt;// 发送失败时的帧号
    struct _tagIPADDRESS_NODE *pPrev;
    struct _tagIPADDRESS_NODE *pNext;
}IPADDRESS_NODE_T;

typedef struct _tagANTS_RTP_DATA_INFO
{
    char pBaseMem[32];
    char *pHeader; // offset 4
    int nHeadLen;
    char *pData;
    int nDataSize;
    char pAdd[8];
    int nAddSize;
}ANTS_RTP_DATA_INFO_T;


//#define ANTS_RTPPACKET_MAXSIZE     (1500)
#define ANTS_RTPPACKET_MAXSIZE     (1440)
#define ANTS_RTPPACKET_NUM     (30)
#define ANTS_RTPPACKET_ANTSCOMB_MAXSIZE     (32768)

/** High level class for using RTP.
 *  For most RTP based applications, the RTPSession class will probably be the one to use. It handles 
 *  the RTCP part completely internally, so the user can focus on sending and receiving the actual data.
 *  \note The RTPSession class is not meant to be thread safe. The user should use some kind of locking 
 *        mechanism to prevent different threads from using the same RTPSession instance.
 */
class  RTPSession
{
public:
	/** Constructs an RTPSession instance, optionally using a specific instance of a random
	 *  number generator, and optionally installing a memory manager. 
	 *  Constructs an RTPSession instance, optionally using a specific instance of a random
	 *  number generator, and optionally installing a memory manager. If no random number generator
	 *  is specified, the RTPSession object will try to use either a RTPRandomURandom or 
	 *  RTPRandomRandS instance. If neither is available on the current platform, a RTPRandomRand48
	 *  instance will be used instead. By specifying a random number generator yourself, it is
	 *  possible to use the same generator in several RTPSession instances.
	 */
	RTPSession();
	virtual ~RTPSession();
	
	
    int Create(int nPort,int bOnlyTCP = 0,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn = NULL,void *pUser=NULL,uint32_t *uiIPv4/*[4]*/ = NULL,int bIPv6=0);
	int CreateClient(int nPort,int bTCP = 0,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn = NULL,void *pUser=NULL,uint32_t *uiIPv4/*[4]*/ = NULL,int bIPv6 = 0);
    int CreateBySocket(int nTCP_fd,int nRTP_fd,int nRTCP_fd,int bTCP = 0,int bRTCP = 0,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn = NULL,void *pUser=NULL);
    int GetRTPSocket(){return m_nSocket_rtp;}
    int GetRTCPSocket(){return m_nSocket_rtcp;}
    int RecvRTPPacket(int bType = 0);
    int RecvRTCPPacket();
    int AddRTPPacket(char *pData,int len);
    int PollRTPPacket();
    int PollRTCPPacket();
    virtual int DealRecvPacket(unsigned char *pRecvData,int nDataLen)=0;

    uint32_t GetRandom32();
    uint32_t GetRandom16();

    int SendH264RtpPacked(const void *data,size_t len);
    int SendRTPData(const void *data,size_t len);
    int SendRTPData(char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen, void *data,size_t len);
    int SendRTPDataNoCopy();
    int SendRTCPData(const void *data,size_t len);
    int SetPayloadClockRate(unsigned int uiPayloadClockRate){m_uiPayloadClockRate = uiPayloadClockRate;return 0;}
    int SetRTSPStreamType(uint8_t uiRTSPStreamType)
    {
        m_uiRTSPStreamType = uiRTSPStreamType;
        return 0;
    }
    unsigned int GetRTSPStreamType()
    {
        return m_uiRTSPStreamType;
    }
	int SetSecondStreamType(uint8_t uiRTSPStreamType)
	{
		m_uiSecondStreamType = uiRTSPStreamType;
		return 0;
	}
	unsigned int GetSecondStreamType()
	{
		return m_uiSecondStreamType;
	}
    int GetPayloadStreamType(int *nStreamType,int *nPlayloadType,int *nClockRate,char *pPlayloadName){if(nStreamType)*nStreamType=m_uiRTSPStreamType;if(nClockRate)*nClockRate=m_uiPayloadClockRate;if(nPlayloadType)*nPlayloadType=m_uiDefaultPayloadType;if(pPlayloadName)strcpy(pPlayloadName,m_szPlayloadName);return 0;}
    int SetPayloadStreamType(int nStreamType,int nPlayloadType,int nClockRate,char *pPlayloadName){m_uiRTSPStreamType=nStreamType;m_uiPayloadClockRate = nClockRate;m_uiDefaultPayloadType=nPlayloadType;if(pPlayloadName)strcpy(m_szPlayloadName,pPlayloadName);return 0;}
	int GetSecondPayloadStreamType(int *nStreamType,int *nPlayloadType,char *pPlayloadName){if(nStreamType)*nStreamType=m_uiSecondStreamType;if(nPlayloadType)*nPlayloadType=m_uiSecondPayloadType;if(pPlayloadName)strcpy(pPlayloadName,m_szSecondPlayloadName);return 0;}

	int SetSecondPayloadStreamType(int nStreamType,int nPlayloadType,char *pPlayloadName){m_uiSecondStreamType=nStreamType;m_uiSecondPayloadType=nPlayloadType;if(pPlayloadName)strcpy(m_szSecondPlayloadName,pPlayloadName);return 0;}

    unsigned int GetPayloadClockRate(){return m_uiPayloadClockRate;}

    void SetProp(unsigned int dwProp)
    {
        m_dwProp = dwProp;
       
    }
    unsigned int GetProp(){return m_dwProp;}

    void SetSSRC(uint32_t uiSSRC){m_uiSSRC = uiSSRC;}
    unsigned int GetSSRC(){return m_uiSSRC;}
    
    void SetTCPDataCallBack(ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser=NULL);
    int GetDestinationCnt();
    int AddDestination(char *pIP,int nPort,int nPort_rtcp = 0,int bIPv6 = 0,int bTCP = 0,int nCh = 0,void *pClient = NULL);
    int AddDestination(unsigned int uiIP[4],int nPort,int nPort_rtcp = 0,int bTCP = 0,int nCH = 0,void *pClient = NULL,int bIPv6 = 0);

    int DeleteDestination(char *pIP,int nPort,int bIPv6 = 0,void *pClient = NULL);
    int DeleteDestination(unsigned int uiIP[4],int nPort,int bTCP = 0,int nCH = 0,void *pClient = NULL);
    int IsMyDestination(unsigned int uiIP[4],int nPort);
    /** Clears the list of destinations. */
	void ClearDestinations();

	/** Leaves the session without sending a BYE packet. */
	void Destroy();

	/** Sends a BYE packet and leaves the session. 
	 *  Sends a BYE packet and leaves the session. At most a time \c maxwaittime will be waited to 
	 *  send the BYE packet. If this time expires, the session will be left without sending a BYE packet. 
	 *  The BYE packet will contain as reason for leaving \c reason with length \c reasonlength.
	 */
	void BYEDestroy(const unsigned int maxwaittime,const void *reason,size_t reasonlength);

	/** Returns whether the session has been created or not. */
	bool IsActive();
	
	/** Returns our own SSRC. */
	uint32_t GetLocalSSRC();
	void SetLocalSSRC(uint32_t uSSRC);
	

	/** Returns \c true if multicasting is supported. */
	bool SupportsMulticasting();

	/** Joins the multicast group specified by \c addr. */
	int JoinMulticastGroup(uint32_t uiIPv4[4],int nPort);

	/** Leaves the multicast group specified by \c addr. */
	int LeaveMulticastGroup(uint32_t uiIPv4[4],int nPort);

	/** Leaves all multicast groups. */
	void LeaveAllMulticastGroups();

    void SetMulticastTTL(unsigned char byTTL);

	/** Sends the RTP packet with payload \c data which has length \c len.
	 *  Sends the RTP packet with payload \c data which has length \c len.
	 *  The used payload type, marker and timestamp increment will be those that have been set 
	 *  using the \c SetDefault member functions.
	 */
	int SendPacket(const void *data,size_t len);
    unsigned char *GetPacketBuffer();
    int GetPacketMaxDataSize();
    unsigned char *GetAntsCombPacketBuffer();
    int GetAntsCombPacketMaxDataSize();
    int DoCheckSend();
    int SendPacket(const void *data,size_t len,int bMark,uint32_t timestamp);
    int SendPacketByBuffer(int nCh,const void *data,size_t len,char *pAdd,int nAddLen,int bMark,uint32_t timestamp,int bRTCP = 0);
    int SendPacketNoCopy(const void *data,size_t len,char *pAdd,int nAddLen,int bMark,uint32_t timestamp);
    int SendAntsCombPacketNoCopy(int nType,int nChan,int nStreamIdx, void *data,size_t len,int bMark = 0,int bStart = 0);
    int SendPacket(int len,int bMark,uint32_t timestamp);
    int OnRtpRtcpPacket(const void *data,size_t len);

	/** Sends the RTP packet with payload \c data which has length \c len.
	 *  It will use payload type \c pt, marker \c mark and after the packet has been built, the 
	 *  timestamp will be incremented by \c timestampinc.
	 */
	
#ifdef RTP_SUPPORT_SENDAPP
	/** If sending of RTCP APP packets was enabled at compile time, this function creates a compound packet 
	 *  containing an RTCP APP packet and sends it immediately. 
	 *  If sending of RTCP APP packets was enabled at compile time, this function creates a compound packet 
	 *  containing an RTCP APP packet and sends it immediately. If successful, the function returns the number 
	 *  of bytes in the RTCP compound packet. Note that this immediate sending is not compliant with the RTP 
	 *  specification, so use with care. 
	 */
	int SendRTCPAPPPacket(uint8_t subtype, const uint8_t name[4], const void *appdata, size_t appdatalen);
#endif // RTP_SUPPORT_SENDAPP

#ifdef RTP_SUPPORT_RTCPUNKNOWN
	/** Tries to send an Unknown packet immediately. 
	 *  Tries to send an Unknown packet immediately. If successful, the function returns the number 
	 *  of bytes in the RTCP compound packet. Note that this immediate sending is not compliant with the RTP 
	 *  specification, so use with care.  Can send message along with a receiver report or a sender report
	 */
	int SendUnknownPacket(bool sr, uint8_t payload_type, uint8_t subtype, const void *data, size_t len);
#endif // RTP_SUPPORT_RTCPUNKNOWN 
	/** Sets the default payload type for RTP packets to \c pt. */
	int SetDefaultPayloadType(uint8_t pt);
	int SetSecondPayloadType(uint8_t pt);

	/** Sets the default marker for RTP packets to \c m. */
	int SetDefaultMark(bool m);

	/** Sets the default value to increment the timestamp with to \c timestampinc. */                         
	int SetDefaultTimestampIncrement(uint32_t timestampinc);

	/** This function increments the timestamp with the amount given by \c inc.
	 *  This function increments the timestamp with the amount given by \c inc. This can be useful 
	 *  if, for example, a packet was not sent because it contained only silence. Then, this function 
	 *  should be called to increment the timestamp with the appropriate amount so that the next packets 
	 *  will still be played at the correct time at other hosts.
	 */
	




	
	/** Sets the receive mode to \c m.
	 *  Sets the receive mode to \c m. Note that when the receive mode is changed, the list of
	 *  addresses to be ignored ot accepted will be cleared.
	 */
	int SetReceiveMode(int m);

	/** Adds \c addr to the list of addresses to ignore. */
	int AddToIgnoreList(uint32_t uiIPv4,int nPort);

	/** Deletes \c addr from the list of addresses to ignore. */
	int DeleteFromIgnoreList(uint32_t uiIPv4,int nPort);

	/** Clears the list of addresses to ignore. */
	void ClearIgnoreList();

	/** Adds \c addr to the list of addresses to accept. */
	int AddToAcceptList(uint32_t uiIPv4,int nPort);

	/** Deletes \c addr from the list of addresses to accept. */
	int DeleteFromAcceptList(uint32_t uiIPv4,int nPort);

	/** Clears the list of addresses to accept. */
	void ClearAcceptList();
	
	/** Sets the maximum allowed packet size to \c s. */
	int SetMaximumPacketSize(size_t s);
    size_t GetMaximumPacketSize(){return maxpacksize;}

	
	/** Sets the timestamp unit for our own data.
	 *  Sets the timestamp unit for our own data. The timestamp unit is defined as a time interval in 
	 *  seconds divided by the corresponding timestamp interval. For example, for 8000 Hz audio, the 
	 *  timestamp unit would typically be 1/8000. Since this value is initially set to an illegal value, 
	 *  the user must set this to an allowed value to be able to create a session.
	 */
	int SetTimestampUnit(double u);
	
	/** Sets the RTCP interval for the SDES name item.
	 *  After all possible sources in the source table have been processed, the class will check if other 
	 *  SDES items need to be sent. If \c count is zero or negative, nothing will happen. If \c count 
	 *  is positive, an SDES name item will be added after the sources in the source table have been 
	 *  processed \c count times.
	 */
	void SetNameInterval(int count);

	/** Sets the RTCP interval for the SDES e-mail item.
	 *  After all possible sources in the source table have been processed, the class will check if other 
	 *  SDES items need to be sent. If \c count is zero or negative, nothing will happen. If \c count 
	 *  is positive, an SDES e-mail item will be added after the sources in the source table have been 
	 *  processed \c count times.
	 */	
	void SetEMailInterval(int count);
	
	/** Sets the RTCP interval for the SDES location item.
	 *  After all possible sources in the source table have been processed, the class will check if other 
	 *  SDES items need to be sent. If \c count is zero or negative, nothing will happen. If \c count 
	 *  is positive, an SDES location item will be added after the sources in the source table have been 
	 *  processed \c count times.
	 */		
	void SetLocationInterval(int count);

	/** Sets the RTCP interval for the SDES phone item.
	 *  After all possible sources in the source table have been processed, the class will check if other 
	 *  SDES items need to be sent. If \c count is zero or negative, nothing will happen. If \c count 
	 *  is positive, an SDES phone item will be added after the sources in the source table have been 
	 *  processed \c count times.
	 */	
	void SetPhoneInterval(int count);

	/** Sets the RTCP interval for the SDES tool item.
	 *  After all possible sources in the source table have been processed, the class will check if other 
	 *  SDES items need to be sent. If \c count is zero or negative, nothing will happen. If \c count 
	 *  is positive, an SDES tool item will be added after the sources in the source table have been 
	 *  processed \c count times.
	 */	
	void SetToolInterval(int count);

	/** Sets the RTCP interval for the SDES note item.
	 *  After all possible sources in the source table have been processed, the class will check if other 
	 *  SDES items need to be sent. If \c count is zero or negative, nothing will happen. If \c count 
	 *  is positive, an SDES note item will be added after the sources in the source table have been 
	 *  processed \c count times.
	 */	
	void SetNoteInterval(int count);
	
	/** Sets the SDES name item for the local participant to the value \c s with length \c len. */
	int SetLocalName(const void *s,size_t len);
	
	/** Sets the SDES e-mail item for the local participant to the value \c s with length \c len. */
	int SetLocalEMail(const void *s,size_t len);

	/** Sets the SDES location item for the local participant to the value \c s with length \c len. */
	int SetLocalLocation(const void *s,size_t len);

	/** Sets the SDES phone item for the local participant to the value \c s with length \c len. */
	int SetLocalPhone(const void *s,size_t len);

	/** Sets the SDES tool item for the local participant to the value \c s with length \c len. */
	int SetLocalTool(const void *s,size_t len);

	/** Sets the SDES note item for the local participant to the value \c s with length \c len. */
	int SetLocalNote(const void *s,size_t len);
    void SetTime(unsigned int uSec,unsigned int uMSec){m_dwRunTimeSecond = uSec,m_dwRunTimeMSecond = uMSec;}
    void GetTime(unsigned int *uSec,unsigned int *uMSec){if(uSec)*uSec=m_dwRunTimeSecond;if(uMSec)*uMSec=m_dwRunTimeMSecond;}

    void SetRecording(int bRecording){m_bRecording = bRecording;}
    int IsRecording(){return m_bRecording;}

    void SetRecordPlaySeq(int nPlaySeq){m_nLastPlaySeq = nPlaySeq;}
    int GetRecordPlaySeq(){return m_nLastPlaySeq;}
    void SetRecordNTP(uint32_t msw,uint32_t lsw)
    {
        m_ntpTimeStamp0 = msw;m_ntpTimeStamp1 = lsw;
    }
    void SetRecordExHeaderCED(uint8_t d,uint8_t e,uint8_t c)
    {
        m_discontinuity = d;m_eSectionEnd = e;m_cIFrameStart = c;
    }
    void SetRecordExHeader_C(uint8_t c)
    {
        m_cIFrameStart = c;
    }
    void SetRecordExHeader_E(uint8_t e)
    {
       m_eSectionEnd = e;
    }
    void SetRecordExHeader_D(uint8_t d)
    {
        m_discontinuity = d;
    }
    void SetRecordAbsTimeStamp(uint32_t Sec,uint32_t uSec)
    {
        m_dwAbsTimeSec = Sec;
        m_dwAbsTimeUSec = uSec;
    }
   
    int SetFrameRate(int nFps){m_nFrameRate = nFps;return 0;}
    int GetFrameRate(){return m_nFrameRate;}

#ifdef RTPDEBUG
	void DumpSources();
	void DumpTransmitter();
#endif // RTPDEBUG
protected:
	
	
	
    unsigned int m_dwProp; // bit0:0-调用者释放帧内存，1-被调用者释放帧内存

//private:
public:
	
	int CreateCNAME(uint8_t *buffer,size_t *bufferlength,bool resolve);


	

	bool created;
	bool deletetransmitter;
	bool usingpollthread;
	bool acceptownpackets;
	bool useSR_BYEifpossible;
	size_t maxpacksize;
	double sessionbandwidth;
	double controlfragment;
	double sendermultiplier;
	double byemultiplier;
	double membermultiplier;
	double collisionmultiplier;
	double notemultiplier;
	bool sentpackets;

    //JMutex m_tLock;

    int    m_bClient;
    time_t m_tLastRtcpTime;
    uint32_t m_uiSSRC_id;
    //RTCP
    uint32_t m_Fractioinlost;// (8bit)表示包的丢失率是多少，即丢失的包的数量除以期望的包的数量，丢失的包的数量可以通过每次检查RTP包头中的Sequence Nmuber统计出来；
    uint32_t m_numberOfPacketsLost;//从会话开始至今丢失的包的总数
    uint32_t m_uiExtHighseqnr;
    uint32_t m_uiExtHighseqnr_low16;//最后一次的序列号
    uint32_t m_uiExtHighseqnr_high16;// 循环次数
    uint32_t m_uiInterarrival_jitter;//对RTP包到达不一致性的估计
    uint32_t m_lsr;//Last SR Timestamp(LSR) 表示所接收到的此RR块对应的SSRC发送的最后一个SR中64-bit NTP时间戳的中间32位，以告知它所发送的SR是否已经被收到；
    uint32_t  m_Dlsr;   //Delay Since Last SR (DLSR)从收到最后一个SR到此RR块被发出经历的时间，精确到1/65536秒
    struct timeval m_LsrTime;

    //RTP 
    char m_szPlayloadName[32];
	char m_szSecondPlayloadName[32];
    uint32_t m_uiPayloadClockRate;
    uint8_t m_uiDefaultPayloadType;
	uint8_t m_uiSecondPayloadType;
	uint8_t m_uiSecondStreamType;
    uint8_t m_uiRTSPStreamType;
    uint8_t m_uiDefaultMark;
    uint32_t m_uiSSRC;
    uint16_t m_uiSeqNumber;
    uint32_t m_uiTimeStamp;
    uint32_t m_uiTimeStampInc;
    uint8_t m_pRTPPacketBuffer[ANTS_RTPPACKET_MAXSIZE + 60];
    uint8_t m_pRTCPPacketBuffer[ANTS_RTPPACKET_MAXSIZE + 60];

    // for udp 
	int m_bIPv6;
    int m_nTranType; // 0- UDP,1-TCP,2-Multicast
    int m_nPortBase;
    int m_bExternSocket; // 是否外部SOCKET
    int m_bRTP_Rtcp;
    int m_nSocket_rtp;
    int m_nSocket_rtcp;
    int m_nSocket_tcp;
    int m_nInterleaved[2]; // tcp,
    RTP_PACKET_NODE_T *m_pRTPPackNode_Head;
    RTP_PACKET_NODE_T *m_pRTPPackNode_Tail;
    int m_nRTPPacketCnt;
    JMutex m_tPacketLock;
    char *m_pRecvBuff;
    uint16_t m_wUDPSeqNumLast;
    int m_bFirstUDPSeqFlag;
    unsigned int m_nUDP_RTCP_cnt;
    char *m_pCurrFrame;// 当前帧起始
    int m_nCurrPos; // 当前帧发送位置
    int m_nTotalSize; // 当前帧的总长度
    int m_nCurrFrameBufSize; // 当前帧的缓冲总长度
    int m_nRTP_Error;// 1- 内存分配失败

    IPADDRESS_NODE_T *m_pIPAddressNode_Head;
    IPADDRESS_NODE_T *m_pIPAddressNode_Tail;
    int m_nIPAddressNodeCnt;
    JMutex m_tIPAddrLock;

    int m_bRTPMulticast;
    int m_bRTCPMulticast;
    unsigned int m_dwMulticastIPv4[4];
    unsigned char m_byMulticastTTL;
    int m_nFrameRate;
    ANTS_RTP_DATA_INFO_T *m_pDataInfo[ANTS_RTPPACKET_NUM];
    int m_nPktCount;

    


   

    ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK m_pRtpRtcp_DataPacketCallback_fxn;
    void *m_pRtpRtcpCallbackUser;
    unsigned int m_dwRunTimeSecond;
    unsigned int m_dwRunTimeMSecond;
  protected:  
      int m_nFrameType;
      unsigned int m_dwFrameCnt;
    // 回放
    int m_bRecording;
    int m_nLastPlaySeq;
    uint32_t m_ntpTimeStamp0; // ntp 时间戳
    uint32_t m_ntpTimeStamp1;
    uint8_t m_discontinuity;// 标识与上一帧不连续,一般用于倒放。每个GOP第一包设置为1,因为它与前一包无关。
    uint8_t m_eSectionEnd; // 录像片段结束
    uint8_t m_cIFrameStart;// I帧开始 clean point

    uint32_t m_dwAbsTimeSec;
    uint32_t m_dwAbsTimeUSec;

	uint32_t m_totalPacketCount;  // 当前数据包总数
	uint32_t m_totalPacketSize;   // 当前数据包总大小
    

};

} // end namespace

#endif // RTPSESSION_H

