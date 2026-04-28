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

#include "rtpsession.h"
#include "rtsp_common.h"


#ifndef WIN32
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#else
#include <winbase.h>
#include<Ws2ipdef.h >
#include<Ws2tcpip.h >
#endif // WIN32

#ifdef RTPDEBUG
#include <iostream>
#endif // RTPDEBUG


#include "rtsp_common.h"
#include <time.h>

#ifdef RTP_SUPPORT_THREAD
#define SOURCES_LOCK					{ if (usingpollthread) sourcesmutex.Lock(); }
#define SOURCES_UNLOCK					{ if (usingpollthread) sourcesmutex.Unlock(); }
#define BUILDER_LOCK					{ if (usingpollthread) buildermutex.Lock(); }
#define BUILDER_UNLOCK					{ if (usingpollthread) buildermutex.Unlock(); }
#define SCHED_LOCK					{ if (usingpollthread) schedmutex.Lock(); }
#define SCHED_UNLOCK					{ if (usingpollthread) schedmutex.Unlock(); }
#define PACKSENT_LOCK					{ if (usingpollthread) packsentmutex.Lock(); }
#define PACKSENT_UNLOCK					{ if (usingpollthread) packsentmutex.Unlock(); }
#else
#define SOURCES_LOCK
#define SOURCES_UNLOCK
#define BUILDER_LOCK
#define BUILDER_UNLOCK
#define SCHED_LOCK
#define SCHED_UNLOCK
#define PACKSENT_LOCK
#define PACKSENT_UNLOCK
#endif // RTP_SUPPORT_THREAD

namespace jrtplib
{

RTPSession::RTPSession()
{
    int i;
	time_t tim;
	int rand_num;
    created = false;

    m_bClient = 0;
    m_tLastRtcpTime = 0;
    m_uiSSRC_id = 0;
    m_uiExtHighseqnr_high16 = 0;
    m_uiExtHighseqnr_low16 = 0;
    m_Fractioinlost = 0;
    m_numberOfPacketsLost = 0;
    m_uiInterarrival_jitter = 0;
    m_lsr = 0;
    m_Dlsr = 0;
    m_uiExtHighseqnr =0;
    m_LsrTime.tv_sec = 0;
    m_LsrTime.tv_usec = 0;

    m_bRTPMulticast = 0;
    m_bRTCPMulticast = 0;
	memset(m_dwMulticastIPv4,0,sizeof(m_dwMulticastIPv4));
    m_byMulticastTTL = 255;


    m_uiPayloadClockRate = 90000;

    m_nSocket_rtp = -1;
    m_nSocket_rtcp = -1;
    m_nTranType = 0;
    m_nPortBase = RTSP_BASE_PORT_NUM;
    m_pRTPPackNode_Head = NULL;
    m_pRTPPackNode_Tail = NULL;
    m_pRecvBuff = NULL;
    m_nRTPPacketCnt = 0;
    m_tPacketLock.Init();
    m_pIPAddressNode_Head = NULL;
    m_pIPAddressNode_Tail = NULL;
    m_nIPAddressNodeCnt = 0;
    m_tIPAddrLock.Init();
    m_pRtpRtcp_DataPacketCallback_fxn = NULL;
    m_pRtpRtcpCallbackUser = NULL;
	{
		static int SSRC_Cnt = 0;
		time(&tim);
		srand(tim + SSRC_Cnt);
		rand_num = (int)(255.0*rand()/(RAND_MAX+1.0));
		rand_num &= 0xFF;
		SSRC_Cnt++;
		SSRC_Cnt &= 0xFF;

		//产生ID
		m_uiSSRC = ((tim & 0x7FFF) << 16) | SSRC_Cnt |(rand_num << 8) ;
	}

   // m_uiSSRC = (uint32_t)(uint64_t)this;
    m_uiSeqNumber = 0;
    m_uiRTSPStreamType = -1;
	m_uiSecondStreamType = -1;
//    m_tLock.Init();
    m_dwProp = 0;
    m_wUDPSeqNumLast = 0;
    m_bFirstUDPSeqFlag = 0;
    m_nUDP_RTCP_cnt = 0;
    m_dwRunTimeMSecond = 0;
    m_dwRunTimeSecond = 0;
    m_uiDefaultPayloadType = 0xFF;
	m_uiSecondPayloadType = 0xFF;
    memset(m_pRTPPacketBuffer,0,sizeof(RTPHeader));
    m_bRecording = 0;
    m_nLastPlaySeq = 0;
    m_discontinuity = 0;
    m_cIFrameStart = 0;
    m_eSectionEnd = 0;
    m_dwAbsTimeSec = 0;
    m_dwAbsTimeUSec = 0;
    m_dwFrameCnt = 0;
    m_nFrameType = 0;
    m_nFrameRate = 0;
    m_nPktCount = 0;
	m_totalPacketCount = 0;
	m_totalPacketSize = 0;

    for (i = 0; i < ANTS_RTPPACKET_NUM; i++)
    {
        m_pDataInfo[i] = NULL;
    }
    m_bExternSocket = 0;
    memset(m_szPlayloadName,0,sizeof(m_szPlayloadName));
	 memset(m_szSecondPlayloadName,0,sizeof(m_szSecondPlayloadName));
    m_bRTP_Rtcp = 0;
    m_nInterleaved[0] = -1;
    m_nInterleaved[1] = -1;
    m_nSocket_tcp = -1;
    m_pCurrFrame = NULL;
    m_nCurrPos = 0;
    m_nTotalSize = 0;
    m_nCurrFrameBufSize = 0;
    m_nRTP_Error = 0;
    //std::cout << (void *)(rtprnd) << std::endl;
}

RTPSession::~RTPSession()
{
    Destroy();


}
void RTPSession::SetMulticastTTL(unsigned char byTTL)
{
    unsigned char ttl = 255;
    m_byMulticastTTL = byTTL;
    ttl = byTTL;
    if (m_nSocket_rtp >= 0)
    {
        setsockopt(m_nSocket_rtp,IPPROTO_IP,IP_MULTICAST_TTL,(const char *)&ttl,sizeof(ttl));
    }
    if (m_nSocket_rtcp >= 0)
    {
        setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_MULTICAST_TTL,(const char *)&ttl,sizeof(ttl));
    }

}
int RTPSession::Create(int nPort,int bOnlyTCP,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser,uint32_t *uiIPv4,int bIPv6)
{
    struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	struct sockaddr *pAddr = NULL;
	int addrLen = 0;
    int size;
    unsigned char ttl = 255;
    unsigned char loop = 0;
    //m_tLock.Lock();
    if (created)
    {
        //m_tLock.Unlock();
        return -1;
    }
	m_bIPv6 = bIPv6;

    m_pRtpRtcp_DataPacketCallback_fxn = fxn;
    m_pRtpRtcpCallbackUser = pUser;

    if (nPort & 1)
    {
        //m_tLock.Unlock();
        return -1;
    }
    if(!bOnlyTCP)
    {
		if (bIPv6)
		{
			m_nSocket_rtp = socket(PF_INET6,SOCK_DGRAM,0);
			if (m_nSocket_rtp < 0)
			{
				//m_tLock.Unlock();
				return -1;
			}


			m_nSocket_rtcp = socket(PF_INET6,SOCK_DGRAM,0);
			if (m_nSocket_rtcp < 0)
			{
				closeSocket(m_nSocket_rtp);
				m_nSocket_rtp = -1;
				//m_tLock.Unlock();
				return -1;
			}
		}
		else
		{
			m_nSocket_rtp = socket(PF_INET,SOCK_DGRAM,0);
			if (m_nSocket_rtp < 0)
			{
				//m_tLock.Unlock();
				return -1;
			}


			m_nSocket_rtcp = socket(PF_INET,SOCK_DGRAM,0);
			if (m_nSocket_rtcp < 0)
			{
				closeSocket(m_nSocket_rtp);
				m_nSocket_rtp = -1;
				//m_tLock.Unlock();
				return -1;
			}
		}




        size = 65535;
        if (setsockopt(m_nSocket_rtp,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
        {
            if(m_nSocket_rtp > 0)
            {
                closeSocket(m_nSocket_rtp);
            }
            if(m_nSocket_rtcp > 0)
            {
                closeSocket(m_nSocket_rtcp);
            }
            m_nSocket_rtp = -1;
            m_nSocket_rtcp = -1;
            // m_tLock.Unlock();
            return -1;
        }
        ttl = m_byMulticastTTL;
        setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_MULTICAST_TTL,(const char *)&ttl,sizeof(ttl));
        loop = 0;
        setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_MULTICAST_LOOP,(const char *)&loop,sizeof(loop));
        size = 65535;
        if (setsockopt(m_nSocket_rtcp,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
        {
            if(m_nSocket_rtp > 0)
            {
                closeSocket(m_nSocket_rtp);
            }
            if(m_nSocket_rtcp > 0)
            {
                closeSocket(m_nSocket_rtcp);
            }
            m_nSocket_rtp = -1;
            m_nSocket_rtcp = -1;
            //m_tLock.Unlock();
            return -1;
        }
#ifndef WIN32
        int nPhyIdx = (m_dwProp >> 24)&0xFF;
        if(nPhyIdx > 0)
        {
            // 绑定网卡
            struct ifreq  ifr;
            sprintf(ifr.ifr_ifrn.ifrn_name,"eth%d", nPhyIdx - 1);
            if (setsockopt(m_nSocket_rtcp, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
            {
                RTSP_DEBUG("SO_BINDTODEVICE failed phyidx = %d! [%s]\n",nPhyIdx,strerror(errno));

                /* Deal with error... */
            }
            if (setsockopt(m_nSocket_rtp, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
            {
                RTSP_DEBUG("SO_BINDTODEVICE failed phyidx = %d! [%s]\n",nPhyIdx,strerror(errno));

                /* Deal with error... */
            }
        }
#endif

		if (bIPv6)
		{
			memset(&addr6,0,sizeof(struct sockaddr_in6));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(nPort);
			addr6.sin6_addr = in6addr_any;
			if (uiIPv4 != NULL)
			{
				memcpy(&addr6.sin6_addr ,uiIPv4,16);
			}


			pAddr = (struct sockaddr *)&addr6;
			addrLen = sizeof(struct sockaddr_in6);
		}
		else
		{
			memset(&addr,0,sizeof(struct sockaddr_in));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(nPort);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			if (uiIPv4 != NULL)
			{
				addr.sin_addr.s_addr  = uiIPv4[0];
			}
			pAddr = (struct sockaddr *)&addr;
			addrLen = sizeof(struct sockaddr_in);
		}


        if (bind(m_nSocket_rtp,(struct sockaddr *)pAddr,addrLen) != 0)
        {
            if(m_nSocket_rtp > 0)
            {
                closeSocket(m_nSocket_rtp);
            }
            if(m_nSocket_rtcp > 0)
            {
                closeSocket(m_nSocket_rtcp);
            }
            m_nSocket_rtp = -1;
            m_nSocket_rtcp = -1;
            //m_tLock.Unlock();
            return -1;
        }



		if (bIPv6)
		{
			memset(&addr6,0,sizeof(struct sockaddr_in6));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(nPort + 1);
			addr6.sin6_addr = in6addr_any;
			if (uiIPv4 != NULL)
			{
				memcpy(&addr6.sin6_addr ,uiIPv4,16);
			}
			pAddr = (struct sockaddr *)&addr6;
			addrLen = sizeof(struct sockaddr_in6);
		}
		else
		{
			memset(&addr,0,sizeof(struct sockaddr_in));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(nPort + 1);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			if (uiIPv4 != NULL)
			{
				addr.sin_addr.s_addr  = uiIPv4[0];
			}
			pAddr = (struct sockaddr *)&addr;
			addrLen = sizeof(struct sockaddr_in);
		}
        if (bind(m_nSocket_rtcp,(struct sockaddr *)pAddr,addrLen) != 0)
        {
            if(m_nSocket_rtp > 0)
            {
                closeSocket(m_nSocket_rtp);
            }
            if(m_nSocket_rtcp > 0)
            {
                closeSocket(m_nSocket_rtcp);
            }
            m_nSocket_rtp = -1;
            m_nSocket_rtcp = -1;
            //m_tLock.Unlock();
            return -1;
        }
    }
    m_nPortBase = nPort;
    m_tPacketLock.Lock();
    if (m_pRecvBuff == NULL)
    {
        m_pRecvBuff = new char[65535];
    }
    m_tPacketLock.Unlock();
    if (m_pRecvBuff == NULL)
    {
        if(m_nSocket_rtp > 0)
        {
            closeSocket(m_nSocket_rtp);
        }
        if(m_nSocket_rtcp > 0)
        {
            closeSocket(m_nSocket_rtcp);
        }

        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        //m_tLock.Unlock();
        return -1;
    }
	m_nTranType = bOnlyTCP;
    created = true;

    //m_tLock.Unlock();
    return 0;




}

int RTPSession::CreateClient(int nPort,int nTranType,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser,uint32_t *uiIPv4/*[4]*/,int bIPv6)
{
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	struct sockaddr *pAddr = NULL;
	int addrLen = 0;
    int size;
    int nPhyIdx;
    //m_tLock.Lock();
    if (created)
    {
        //m_tLock.Unlock();
        return -1;
    }
	m_bIPv6 = bIPv6;
    m_pRtpRtcp_DataPacketCallback_fxn = fxn;
    m_pRtpRtcpCallbackUser = pUser;
    if (nTranType == 1)
    {
        m_nTranType = 1;
        m_bClient = 1;
        created = true;
        //m_tLock.Unlock();
        return 0;
    }
    if (nPort & 1)
    {
        //m_tLock.Unlock();
        return -1;
    }
	if (bIPv6)
	{
		m_nSocket_rtp = socket(PF_INET6,SOCK_DGRAM,0);
		if (m_nSocket_rtp < 0)
		{
			//m_tLock.Unlock();
			return -1;
		}



		m_nSocket_rtcp = socket(PF_INET6,SOCK_DGRAM,0);
		if (m_nSocket_rtcp < 0)
		{
			closeSocket(m_nSocket_rtp);
			m_nSocket_rtp = -1;
			//m_tLock.Unlock();
			return -1;
		}
	}
	else
	{
		m_nSocket_rtp = socket(PF_INET,SOCK_DGRAM,0);
		if (m_nSocket_rtp < 0)
		{
			//m_tLock.Unlock();
			return -1;
		}



		m_nSocket_rtcp = socket(PF_INET,SOCK_DGRAM,0);
		if (m_nSocket_rtcp < 0)
		{
			closeSocket(m_nSocket_rtp);
			m_nSocket_rtp = -1;
			//m_tLock.Unlock();
			return -1;
		}
	}



    size = 65535;
    if (setsockopt(m_nSocket_rtp,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
    {
        closeSocket(m_nSocket_rtp);
        closeSocket(m_nSocket_rtcp);
        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        //m_tLock.Unlock();
        return -1;
    }
    size = 65535;
    if (setsockopt(m_nSocket_rtcp,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int)) != 0)
    {
        closeSocket(m_nSocket_rtp);
        closeSocket(m_nSocket_rtcp);
        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        // m_tLock.Unlock();
        return -1;
    }
#ifndef WIN32
    nPhyIdx = (m_dwProp >> 24)&0xFF;
    if(nPhyIdx > 0)
    {
        // 绑定网卡
        struct ifreq  ifr;
        sprintf(ifr.ifr_ifrn.ifrn_name,"eth%d", nPhyIdx - 1);
        if (setsockopt(m_nSocket_rtcp, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
        {
            RTSP_DEBUG("SO_BINDTODEVICE failed phyidx = %d! [%s]\n",nPhyIdx,strerror(errno));

            /* Deal with error... */
        }
        if (setsockopt(m_nSocket_rtp, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
        {
            RTSP_DEBUG("SO_BINDTODEVICE failed phyidx = %d! [%s]\n",nPhyIdx,strerror(errno));

            /* Deal with error... */
        }
    }
#endif

    int reuse = 1;
    if(reuse && nTranType == 2)
    {
        if(setsockopt(m_nSocket_rtp, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
        {
            RTSP_DEBUG("socket Setting SO_REUSEADDR failed socket [%s].", strerror(errno));
        }

        if(setsockopt(m_nSocket_rtcp, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
        {
            RTSP_DEBUG("socket Setting SO_REUSEADDR failed socket [%s].", strerror(errno));
        }
    }
	if (bIPv6)
	{
		memset(&addr6,0,sizeof(struct sockaddr_in6));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(nPort);
		addr6.sin6_addr = in6addr_any;
		if (nTranType == 2&& uiIPv4 != NULL)
		{
			memcpy(&addr6.sin6_addr ,uiIPv4,16);
		}
		pAddr = (struct sockaddr *)&addr6;
		addrLen = sizeof(struct sockaddr_in6);
	}
	else
	{
		memset(&addr,0,sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(nPort);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (nTranType == 2 && uiIPv4 != NULL)
		{
			addr.sin_addr.s_addr = uiIPv4[0];
		}

		pAddr = (struct sockaddr *)&addr;
		addrLen = sizeof(struct sockaddr_in);
	}

    if (bind(m_nSocket_rtp,(struct sockaddr *)pAddr,addrLen) != 0)
    {
        closeSocket(m_nSocket_rtp);
        closeSocket(m_nSocket_rtcp);
        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        // m_tLock.Unlock();
        return -1;
    }
	if (bIPv6)
	{
		memset(&addr6,0,sizeof(struct sockaddr_in6));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(nPort+1);
		addr6.sin6_addr = in6addr_any;
		if (nTranType == 2 && uiIPv4 != NULL)
		{
			memcpy(&addr6.sin6_addr ,uiIPv4,16);
		}
		pAddr = (struct sockaddr *)&addr6;
		addrLen = sizeof(struct sockaddr_in6);
	}
	else
	{
		memset(&addr,0,sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(nPort+1);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (nTranType == 2 && uiIPv4 != NULL)
		{
			addr.sin_addr.s_addr = uiIPv4[0];
		}

		pAddr = (struct sockaddr *)&addr;
		addrLen = sizeof(struct sockaddr_in);
	}

    if (bind(m_nSocket_rtcp,(struct sockaddr *)pAddr,addrLen) != 0)
    {
        closeSocket(m_nSocket_rtp);
        closeSocket(m_nSocket_rtcp);
        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        //  m_tLock.Unlock();
        return -1;
    }

    m_nTranType = nTranType;
    m_nPortBase = nPort;
    m_tPacketLock.Lock();
    if (m_pRecvBuff == NULL)
    {
        m_pRecvBuff = new char[65535];
    }
    m_tPacketLock.Unlock();
    if (m_pRecvBuff == NULL)
    {
        closeSocket(m_nSocket_rtp);
        closeSocket(m_nSocket_rtcp);
        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        // m_tLock.Unlock();
        return -1;
    }
    m_bClient = 1;
    created = true;
    // m_tLock.Unlock();

    return 0;




}

int RTPSession::CreateBySocket(int nTCP_fd,int nRTP_fd,int nRTCP_fd,int bTCP,int bRTCP,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser)
{
    // TCP 时,nRTP_fd/nRTCP_fd为interleaved 编号
    // UDP 时,nTCP_fd无效
    struct sockaddr_in addr;
    int size;
    unsigned char ttl = 255;
    unsigned char loop = 0;
    //m_tLock.Lock();
    if (created)
    {
        //m_tLock.Unlock();
        return -1;
    }


    m_pRtpRtcp_DataPacketCallback_fxn = fxn;
    m_pRtpRtcpCallbackUser = pUser;


    if(bTCP)
    {
        m_nInterleaved[0] = nRTP_fd;
        m_nInterleaved[1] = nRTCP_fd;
        m_nSocket_tcp = nTCP_fd;
        if (nTCP_fd == -1)
        {
            //m_tLock.Unlock();
            return -1;
        }
    }
    else
    {


        m_nSocket_rtp = nRTP_fd;
        m_nSocket_rtcp = nRTCP_fd;
        if (nRTP_fd == -1)
        {
            //m_tLock.Unlock();
            return -1;
        }
    }
    m_bExternSocket = 1;
    m_bRTP_Rtcp = bRTCP;
    m_nTranType = bTCP;
    m_tPacketLock.Lock();
    if (m_pRecvBuff == NULL)
    {
        m_pRecvBuff = new char[65535];
    }
    m_tPacketLock.Unlock();
    if (m_pRecvBuff == NULL)
    {
        // NEEDCLOSE

        m_nSocket_rtp = -1;
        m_nSocket_rtcp = -1;
        //m_tLock.Unlock();
        return -1;
    }
    created = true;

    //m_tLock.Unlock();
    return 0;




}

int RTPSession::OnRtpRtcpPacket(const void *data,size_t len)
{

    unsigned char *p8 = (unsigned char *)data;
    if (p8[1] >= 200 && p8[1] <= 204)
    {
        // RTCP
        RTCPCommonHeader *pHeader = (RTCPCommonHeader *)data;
        RTPSourceIdentifier *psid;
        RTCPSenderReport *pSR;
        if(pHeader->packettype == 200)
        {


            Ants_rtsp_CurrentTime(&m_LsrTime.tv_sec,&m_LsrTime.tv_usec);
            // SR
            psid = (RTPSourceIdentifier *)(pHeader + 1);
            pSR = (RTCPSenderReport *)(psid + 1);

            m_lsr = (pSR->ntptime_lsw >> 16) | (pSR->ntptime_msw) & 0xFF;
        }

    }
    else
    {
        // RTP
        RTPHeader *pHeader = (RTPHeader *)data;
        m_uiSSRC_id = pHeader->ssrc;
        if(m_uiExtHighseqnr_low16 >= pHeader->sequencenumber)
        {
            m_uiExtHighseqnr_high16++;
        }
        m_uiExtHighseqnr_low16 = pHeader->sequencenumber;
        m_uiExtHighseqnr = (m_uiExtHighseqnr_high16 << 16) | m_uiExtHighseqnr_low16;

    }
    return 0;

}

int RTPSession::RecvRTPPacket(int bType)
{
    int nRet;
    int RecvLen = 0;
    struct sockaddr_in srcaddr;
	struct sockaddr_in6 srcaddr6;
	struct sockaddr *pAddr;

#if (defined(WIN32) || defined(_WIN32_WCE))
    int fromlen;
    unsigned long len;
#else
    socklen_t fromlen;
    size_t len;
#endif // WIN32
    //m_tLock.Lock();
    if (m_nSocket_rtp != -1)
    {


        if (m_pRecvBuff == NULL)
        {
            // m_tLock.Unlock();
            return -1;
        }

        len = 0;
        ioctlsocket(m_nSocket_rtp,FIONREAD,&len);

        while(len > 0)
        {
			if (m_bIPv6)
			{
				fromlen = sizeof(struct sockaddr_in6);
				pAddr = (struct sockaddr *)&srcaddr6;
			}
			else
			{
				fromlen = sizeof(struct sockaddr_in);
				pAddr = (struct sockaddr *)&srcaddr;
			}



            nRet = recvfrom(m_nSocket_rtp,m_pRecvBuff,65535,0,(struct sockaddr *)pAddr,&fromlen);
            if (nRet < 0)
            {
                RTSP_ERROR("recvfrom err %d\n",errno);
                if(!m_bExternSocket)
                closeSocket(m_nSocket_rtp);
                m_nSocket_rtp = -1;
                //m_tLock.Unlock();
                return -1;
            }
            else if (nRet == 0)
            {
                RTSP_ERROR("error\n");
                if(!m_bExternSocket)
                closeSocket(m_nSocket_rtp);
                m_nSocket_rtp = -1;
                //m_tLock.Unlock();
                return -1;
            }

			if (bType == 0)
			{
				unsigned int dwip[4] = {0,0,0,0};
				unsigned short dstPort = 0;
				if (m_bIPv6)
				{
					memcpy(dwip,&srcaddr6.sin6_addr,16);
					dstPort = htons(srcaddr6.sin6_port);
				}
				else
				{
					dwip[0] = srcaddr.sin_addr.s_addr;
					dstPort = htons(srcaddr.sin_port);
				}


	            if(IsMyDestination(dwip,dstPort))
	            {
	                AddRTPPacket(m_pRecvBuff,nRet);
	            }
			}
			else
			{
	            AddRTPPacket(m_pRecvBuff,nRet);
			}

            len = 0;
            ioctlsocket(m_nSocket_rtp,FIONREAD,&len);
        }

    }
    //m_tLock.Unlock();
    return 0;


}
int RTPSession::RecvRTCPPacket()
{
    int nRet;
    int RecvLen = 0;
	struct sockaddr_in srcaddr;
	struct sockaddr_in6 srcaddr6;
	struct sockaddr *pAddr;
#if (defined(WIN32) || defined(_WIN32_WCE))
    int fromlen;
    unsigned long len;
#else
    socklen_t fromlen;
    size_t len;
#endif // WIN32
    //m_tLock.Lock();
    if (m_nSocket_rtcp != -1)
    {
        if (m_pRecvBuff == NULL)
        {
            //m_tLock.Unlock();
            return -1;
        }

        len = 0;
        ioctlsocket(m_nSocket_rtcp,FIONREAD,&len);
        while(len > 0)
        {
			if (m_bIPv6)
			{
				fromlen = sizeof(struct sockaddr_in6);
				pAddr = (struct sockaddr *)&srcaddr6;
			}
			else
			{
				fromlen = sizeof(struct sockaddr_in);
				pAddr = (struct sockaddr *)&srcaddr;
			}
            nRet = recvfrom(m_nSocket_rtcp,m_pRecvBuff,65535,0,(struct sockaddr *)pAddr,&fromlen);
            if (nRet < 0)
            {
                //m_tLock.Unlock();
                return -1;
            }
            else if (nRet == 0)
            {
                if(!m_bExternSocket)
                closeSocket(m_nSocket_rtcp);
                m_nSocket_rtcp = -1;
                //m_tLock.Unlock();
                return -1;
            }
			// printf("[%s.%d] len = %d  ................port = %d \n",__FUNCTION__,__LINE__,len,htons(srcaddr.sin_port));
            len = 0;
            ioctlsocket(m_nSocket_rtcp,FIONREAD,&len);
        }
    }
    // m_tLock.Unlock();
    return 0;

}
int RTPSession::AddRTPPacket(char *pData,int len)
{
    RTPHeader *prtpheader;
    RTP_PACKET_NODE_T *pNew,*p;
    unsigned char *prtpdata;
    int bDrop = 0;
    unsigned short wSeqNum;
    prtpheader = (RTPHeader *)pData;

    if (len < sizeof(RTPHeader))
    {
        return -1;
    }
    if(prtpheader->version != 2)
    {
        return -1;
    }
    wSeqNum = htons(prtpheader->sequencenumber);
    if (!m_bFirstUDPSeqFlag)
    {
        m_bFirstUDPSeqFlag = 1;
        m_wUDPSeqNumLast = wSeqNum;
        if (m_wUDPSeqNumLast == 0)
        {
            m_wUDPSeqNumLast = -1;
        }
        else
        {
            m_wUDPSeqNumLast--;
        }

    }

    m_tPacketLock.Lock();
    if (m_pRTPPackNode_Head == NULL)
    {
        if (wSeqNum == m_wUDPSeqNumLast + 1)
        {
            m_wUDPSeqNumLast = wSeqNum;
            DealRecvPacket((unsigned char *)pData,len);
            m_tPacketLock.Unlock();
            return 0;
        }

    }

    pNew = new RTP_PACKET_NODE_T;
    if (pNew == NULL)
    {
        m_tPacketLock.Unlock();
        return -1;
    }
    prtpdata = new unsigned char[len + 16];
    if (prtpdata == NULL)
    {
        m_tPacketLock.Unlock();
        delete pNew;
        return -1;
    }
//printf("1*seq :%d len = %d\n",htons(prtpheader->sequencenumber),len);

    pNew->nDataLen = len;
    pNew->pData = prtpdata;
    pNew->nSeqNum = wSeqNum;
    pNew->pNext = NULL;
    pNew->pPrev = NULL;



    if (m_pRTPPackNode_Head == NULL)
    {
        m_pRTPPackNode_Head = pNew;
        m_pRTPPackNode_Tail = pNew;
        memcpy(prtpdata,pData,len);
        m_nRTPPacketCnt++;
    }
    else
    {
        p = m_pRTPPackNode_Tail;
        while(p != NULL)
        {
            // if (p->nSeqNum < pNew->nSeqNum)
            if (pNew->nSeqNum - p->nSeqNum > 0)
            {
                //insert pNew after p
                break;
            }
            else if (p->nSeqNum == pNew->nSeqNum)
            {
                //drop pnew
                bDrop = 1;

                break;
            }
            p = p->pPrev;

        }
        if (bDrop)
        {
            m_tPacketLock.Unlock();
            //printf("drop : %d ,%d\n",p->nSeqNum, pNew->nSeqNum);
            delete pNew;
            delete []prtpdata;
            return 0;
        }
        memcpy(prtpdata,pData,len);
        if (p == NULL)
        {
            // 到头了
            pNew->pNext = m_pRTPPackNode_Head;
            m_pRTPPackNode_Head->pPrev = pNew;
            m_pRTPPackNode_Head = pNew;
        }
        else
        {
            pNew->pPrev = p;
            pNew->pNext = p->pNext;
            if (p->pNext != NULL)
            {
                p->pNext->pPrev = pNew;
            }
            p->pNext = pNew;


            if (p == m_pRTPPackNode_Tail)
            {
                m_pRTPPackNode_Tail = pNew;
            }

        }
        m_nRTPPacketCnt++;
        //printf("add %d\n",m_nRTPPacketCnt);


    }
    m_tPacketLock.Unlock();
    return 0;

}
int RTPSession::PollRTCPPacket()
{
    RTCPCommonHeader *pRtcpHeader,*pRtcpHeader_des;
    RTPSourceIdentifier *pSid,*pSid_des;
    RTCPReceiverReport *pRR;
	RTCPSenderReport *pSR;
    RTCPSDESHeader *pSde;
    char *pCName;
    int PktLen = 0,nItemLen,pktlen1,pktlen2;
    time_t CurrTime;
    struct timeval tv_curr,last_lsr;
	uint32_t msw = 0, lsw = 0;
	uint64_t tempTime = 0;



    if (m_bClient || m_bRTP_Rtcp)
    {

		if (m_uiSSRC_id == 0)
		{
			return 0;
		}
        PktLen = 0;
        last_lsr = m_LsrTime;
        Ants_rtsp_CurrentTime(&tv_curr.tv_sec,&tv_curr.tv_usec);
        CurrTime = tv_curr.tv_sec;
        if (CurrTime < m_tLastRtcpTime + 5 ||
            CurrTime < m_tLastRtcpTime)
        {
            return 0;
        }
        if(last_lsr.tv_sec != 0 && last_lsr.tv_usec != 0)
            m_Dlsr = ((tv_curr.tv_sec * 1000000 + tv_curr.tv_usec) - (last_lsr.tv_sec * 1000000 + last_lsr.tv_usec)) * 65536 /1000000;
        m_tLastRtcpTime = CurrTime;
        pRtcpHeader = (RTCPCommonHeader *)m_pRTCPPacketBuffer;
        pSid = (RTPSourceIdentifier *)(pRtcpHeader + 1);
        pRR = (RTCPReceiverReport *)(pSid + 1);
        pRtcpHeader->version = 2;
        pRtcpHeader->count = 1;
        pRtcpHeader->padding = 0;
        pRtcpHeader->packettype = 201;
        pktlen1 = (sizeof(RTCPReceiverReport) + sizeof(RTPSourceIdentifier) + 3)/4;
        pRtcpHeader->length = htons(pktlen1);
        pSid->ssrc = htonl(m_uiSSRC);
        pRR->ssrc = htonl(m_uiSSRC_id);
        pRR->fractionlost = m_Fractioinlost;
        pRR->packetslost[0] = (m_numberOfPacketsLost >> 16) &0xFF;
        pRR->packetslost[1] = (m_numberOfPacketsLost >> 8) &0xFF;
        pRR->packetslost[2] = m_numberOfPacketsLost & 0xFF;
        pRR->exthighseqnr = htonl(m_uiExtHighseqnr);
        pRR->jitter = htonl(m_uiInterarrival_jitter);
        pRR->lsr = htonl(m_lsr);
        pRR->dlsr = htonl(m_Dlsr);


        pRtcpHeader_des = (RTCPCommonHeader *)(pRR + 1);
        pSid_des = (RTPSourceIdentifier *)(pRtcpHeader_des + 1);
        pSde = (RTCPSDESHeader *)(pSid_des + 1);
        pCName = (char *)(pSde + 1);
        pRtcpHeader_des->version = 2;
        pRtcpHeader_des->count = 1;
        pRtcpHeader_des->padding = 0;
        pRtcpHeader_des->packettype = 202;
        nItemLen = sprintf(pCName,"root");
        pktlen2 = (sizeof(RTCPSDESHeader) + sizeof(RTPSourceIdentifier) + (nItemLen + 1)+ 3)/4;
        pRtcpHeader_des->length = htons(pktlen2);
        pSid_des->ssrc=htonl(m_uiSSRC);
        pSde->sdesid = 1;
        pSde->length = nItemLen;

        PktLen = sizeof(RTCPCommonHeader) * 2 + (pktlen1 + pktlen2) * 4;
        if (m_nTranType == 1 && m_bExternSocket)
        {
            SendPacketByBuffer(m_nInterleaved[1],(char *)m_pRTCPPacketBuffer,PktLen,NULL,0,0,0,1);
        }
        else
        {
            SendRTCPData(m_pRTCPPacketBuffer,PktLen);
        }

    }
	else
	{
		Ants_rtsp_CurrentTime(&tv_curr.tv_sec,&tv_curr.tv_usec);
		CurrTime = tv_curr.tv_sec;
		if (CurrTime < m_tLastRtcpTime + 5 ||
			CurrTime < m_tLastRtcpTime)
		{
			return 0;
		}
			#if 1
		        pRtcpHeader = (RTCPCommonHeader *)m_pRTCPPacketBuffer;
				pSid = (RTPSourceIdentifier *)(pRtcpHeader + 1);
		        pSR = (RTCPSenderReport *)(pSid + 1);
				pRtcpHeader->version = 2;
		        pRtcpHeader->count = 0;
		        pRtcpHeader->padding = 0;
		        pRtcpHeader->packettype = 200;
				pktlen1 = (sizeof(RTCPSenderReport)+ sizeof(RTPSourceIdentifier) +  3)/4;
				pRtcpHeader->length = htons(pktlen1);

				pSid->ssrc = htonl(m_uiSSRC);


				Ants_rtsp_GetRTP2NTPTime(tv_curr.tv_sec, tv_curr.tv_usec, &msw, &lsw);
				pSR->ntptime_msw = htonl(msw);
				pSR->ntptime_lsw = htonl(lsw);

				tempTime = (uint64_t)tv_curr.tv_sec * 1000000;
				tempTime += tv_curr.tv_usec;
				tempTime = (tempTime / 1000) * (m_uiPayloadClockRate/1000);

				//pSR->rtptimestamp = htonl(tempTime);
				pSR->rtptimestamp = htonl(m_uiTimeStamp);

				pSR->packetcount = htonl(m_totalPacketCount);
				pSR->octetcount = htonl(m_totalPacketSize);

				pRtcpHeader_des = (RTCPCommonHeader *)(pSR + 1);
		        pSid_des = (RTPSourceIdentifier *)(pRtcpHeader_des + 1);
		        pSde = (RTCPSDESHeader *)(pSid_des + 1);
		        pCName = (char *)(pSde + 1);
		        pRtcpHeader_des->version = 2;
		        pRtcpHeader_des->count = 1;
		        pRtcpHeader_des->padding = 0;
		        pRtcpHeader_des->packettype = 202;
		        nItemLen = sprintf(pCName,"root");
		        pktlen2 = (sizeof(RTCPSDESHeader) + sizeof(RTPSourceIdentifier) + (nItemLen + 1)+ 3)/4;
		        pRtcpHeader_des->length = htons(pktlen2);
		        pSid_des->ssrc=htonl(m_uiSSRC);
		        pSde->sdesid = 1;
		        pSde->length = nItemLen;

		        PktLen = sizeof(RTCPCommonHeader) * 2 + (pktlen1 + pktlen2) * 4;
			#endif
				if(PktLen <=0 )
				{
					return 0;
				}
			if (m_nTranType == 1 && m_bExternSocket)
	        {
	            SendPacketByBuffer(m_nInterleaved[1],(char *)m_pRTCPPacketBuffer,PktLen,NULL,0,0,0,1);
	        }
	        else
	        {
	            SendRTCPData(m_pRTCPPacketBuffer,PktLen);
	        }

	}

    return 0;
}
int RTPSession::PollRTPPacket()
{

    RTP_PACKET_NODE_T *pPacket;
    RTPHeader *prtpheader;

    while(1)
    {

#if 0
        if (m_nRTPPacketCnt < 5)
        {
            //y  return 0;
            break;
        }
#endif
        m_tPacketLock.Lock();
        pPacket = m_pRTPPackNode_Head;
        if (pPacket == NULL)
        {
            m_tPacketLock.Unlock();

            break;
        }
        if(m_wUDPSeqNumLast + 1 != pPacket->nSeqNum && m_nRTPPacketCnt < 5)
        {
            m_tPacketLock.Unlock();
            break;
        }

        m_pRTPPackNode_Head = m_pRTPPackNode_Head->pNext;
        if (m_pRTPPackNode_Head == NULL)
        {
            m_pRTPPackNode_Tail = NULL;
        }
        else
        {
            m_pRTPPackNode_Head->pPrev = NULL;
        }
        m_nRTPPacketCnt--;
        if (m_nRTPPacketCnt < 0)
        {
            m_nRTPPacketCnt = 0;
        }
        m_wUDPSeqNumLast = pPacket->nSeqNum;
        // printf("dec %d\n",m_nRTPPacketCnt);
        m_tPacketLock.Unlock();
        pPacket->pNext = NULL;
        pPacket->pPrev = NULL;
        prtpheader = (RTPHeader *)pPacket->pData;

        //printf("2*seq :%d\n",htons(prtpheader->sequencenumber));
        DealRecvPacket(pPacket->pData,pPacket->nDataLen);

        delete [] pPacket->pData;
        delete pPacket;



    }
    if (m_nUDP_RTCP_cnt + 5 < m_dwRunTimeSecond)
    {
        m_nUDP_RTCP_cnt = m_dwRunTimeSecond;
        PollRTCPPacket();
        //     printf("RTCP    m_nUDP_RTCP_cnt = %d - %d\n",m_nUDP_RTCP_cnt,m_dwRunTimeSecond);
    }







    return 0;
}




void RTPSession::Destroy()
{
    int i;
    // m_tLock.Lock();
    if (!created)
    {
        //m_tLock.Unlock();
        return;
    }
    LeaveAllMulticastGroups();
    if(m_bExternSocket)
    {
        m_nSocket_rtcp = -1;
        m_nSocket_rtp = -1;
    }
    else
    {
        if (m_nSocket_rtcp != -1)
        {
            closeSocket(m_nSocket_rtcp);
            m_nSocket_rtcp = -1;
        }
        if (m_nSocket_rtp != -1)
        {
            closeSocket(m_nSocket_rtp);
            m_nSocket_rtp = -1;
        }
    }

    m_tPacketLock.Lock();
    if (m_pRecvBuff != NULL)
    {
        delete []m_pRecvBuff;
        m_pRecvBuff = NULL;
    }

    while(1)
    {
        RTP_PACKET_NODE_T *p = m_pRTPPackNode_Head;
        if (p == NULL)
        {
            break;
        }
        m_pRTPPackNode_Head = m_pRTPPackNode_Head->pNext;
        delete [] p->pData;
        delete p;


    }
    m_pRTPPackNode_Head = NULL;
    m_pRTPPackNode_Tail = NULL;
    m_nRTPPacketCnt = 0;


    m_tPacketLock.Unlock();

    m_tIPAddrLock.Lock();
    while(1)
    {
        IPADDRESS_NODE_T  *p = m_pIPAddressNode_Head;
        if (p == NULL)
        {
            break;
        }
        m_pIPAddressNode_Head = m_pIPAddressNode_Head->pNext;
        delete p;


    }
    m_pIPAddressNode_Head = NULL;
    m_pIPAddressNode_Tail = NULL;
    m_nIPAddressNodeCnt = 0;
    for (i = 0; i < ANTS_RTPPACKET_NUM; i++)
    {
        if (m_pDataInfo[i] != NULL)
        {
            free(m_pDataInfo[i]);
            m_pDataInfo[i] = NULL;
        }


    }
    m_nPktCount = 0;
    m_tIPAddrLock.Unlock();



    m_pRtpRtcp_DataPacketCallback_fxn = NULL;
    m_pRtpRtcpCallbackUser = NULL;


    created = false;
    //m_tLock.Unlock();
}

void RTPSession::BYEDestroy(const unsigned int maxwaittime,const void *reason,size_t reasonlength)
{
    if (!created)
        return;

    Destroy();
    created = false;
}

bool RTPSession::IsActive()
{
    return created;
}

uint32_t RTPSession::GetLocalSSRC()
{
    if (!created)
        return 0;


    return htonl(m_uiSSRC);
}

void RTPSession::SetLocalSSRC(uint32_t uSSRC)
{
	if (!created)
		return;
	m_uiSSRC = ntohl(uSSRC);

}

void RTPSession::ClearDestinations()
{
    IPADDRESS_NODE_T *pNode,*pDel;
    if (!created)
        return;

    m_tIPAddrLock.Lock();
    pNode = m_pIPAddressNode_Head;
    while(pNode != NULL)
    {
        pDel = pNode;
        pNode = pNode->pNext;
        delete pDel;
    }
    m_pIPAddressNode_Head = NULL;
    m_pIPAddressNode_Tail = NULL;
    m_tIPAddrLock.Unlock();
}

bool RTPSession::SupportsMulticasting()
{
    if (!created)
        return false;
    return 0;
}

int RTPSession::JoinMulticastGroup(uint32_t uiIPv4[4],int nPort)
{
    if (!created)
        return -1;
    if(m_bClient)
    {
		if (m_bIPv6)
		{
		}
		else
		{
			if (htonl(uiIPv4[0]) < 0xE0000000 || htonl(uiIPv4[0]) > 0xEFFFFFFF)
			{
				return -1;
			}
		}


            // 多播
            struct ip_mreq mcast;
			 struct ipv6_mreq mcast6;
			 int nMcastLen = 0;

            if(m_nSocket_rtp >= 0)
            {
                if (m_bRTPMulticast)
                {
					if (m_bIPv6)
					{
						memcpy(&mcast6.ipv6mr_multiaddr, m_dwMulticastIPv4,16);
						mcast6.ipv6mr_interface = htonl(INADDR_ANY);
						if (setsockopt(m_nSocket_rtp,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
						{
							RTSP_DEBUG("ipv6 setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

						}
					}
					else
					{
						mcast.imr_multiaddr.s_addr = m_dwMulticastIPv4[0];
						mcast.imr_interface.s_addr = htonl(INADDR_ANY);
						if (setsockopt(m_nSocket_rtp,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
						{
							RTSP_DEBUG("setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

						}
					}



                    m_bRTPMulticast = 0;
                }
				if (m_bIPv6)
				{
					memcpy(&mcast6.ipv6mr_multiaddr, uiIPv4,16);
					mcast6.ipv6mr_interface = htonl(INADDR_ANY);
					if (setsockopt(m_nSocket_rtp,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
					{
						RTSP_DEBUG("ipv6 setsockopt rtp error! add membership [%d]-<%s>\n",errno,strerror(errno));

					}
					else
					{
						memcpy(m_dwMulticastIPv4,uiIPv4,16);
						m_bRTPMulticast = 1;
					}
				}
				else
				{
					mcast.imr_multiaddr.s_addr = uiIPv4[0];
					mcast.imr_interface.s_addr = htonl(INADDR_ANY);
					if (setsockopt(m_nSocket_rtp,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
					{
						RTSP_DEBUG("setsockopt rtp error! add membership [%d]-<%s>\n",errno,strerror(errno));

					}
					else
					{
						memcpy(m_dwMulticastIPv4,uiIPv4,16);
						m_bRTPMulticast = 1;
					}
				}

            }

            if(m_nSocket_rtcp >= 0)
            {
                if (m_bRTCPMulticast)
                {
					if (m_bIPv6)
					{
						memcpy(&mcast6.ipv6mr_multiaddr, m_dwMulticastIPv4,16);
						mcast6.ipv6mr_interface = htonl(INADDR_ANY);
						if (setsockopt(m_nSocket_rtcp,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
						{
							RTSP_DEBUG("ipv6 setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

						}
					}
					else
					{
						mcast.imr_multiaddr.s_addr = m_dwMulticastIPv4[0];
						mcast.imr_interface.s_addr = htonl(INADDR_ANY);
						if (setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
						{
							RTSP_DEBUG("setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

						}
					}

                  m_bRTCPMulticast = 0;
                }
				if (m_bIPv6)
				{
					memcpy(&mcast6.ipv6mr_multiaddr, uiIPv4,16);
					mcast6.ipv6mr_interface = htonl(INADDR_ANY);
					if (setsockopt(m_nSocket_rtcp,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
					{
						RTSP_DEBUG("ipv6 setsockopt rtp error! add membership [%d]-<%s>\n",errno,strerror(errno));

					}
					else
					{
						memcpy(m_dwMulticastIPv4,uiIPv4,16);
						m_bRTCPMulticast = 1;
					}
				}
				else
				{
					mcast.imr_multiaddr.s_addr = uiIPv4[0];
					mcast.imr_interface.s_addr = htonl(INADDR_ANY);
					if (setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
					{
						RTSP_DEBUG("setsockopt rtp error! add membership [%d]-<%s>\n",errno,strerror(errno));

					}
					else
					{
						memcpy(m_dwMulticastIPv4,uiIPv4,16);
						m_bRTCPMulticast = 1;
					}
				}

            }
    }
    return 0;
}

int RTPSession::LeaveMulticastGroup(uint32_t uiIPv4[4],int nPort)
{
    if (!created)
        return -1;
    if(m_bClient)
    {
		if (m_bIPv6)
		{
		}
		else
		{
			if (htonl(uiIPv4[0]) < 0xE0000000 || htonl(uiIPv4[0]) > 0xEFFFFFFF)
			{
				return -1;
			}
		}


            // 多播
			struct ip_mreq mcast;
			struct ipv6_mreq mcast6;
			if (m_bRTPMulticast)
			{
				if (m_bIPv6)
				{
					memcpy(&mcast6.ipv6mr_multiaddr, m_dwMulticastIPv4,16);
					mcast6.ipv6mr_interface = htonl(INADDR_ANY);
					if (m_nSocket_rtp >= 0 && setsockopt(m_nSocket_rtp,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
					{
						RTSP_DEBUG("ipv6 setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

					}


				}
				else
				{
					mcast.imr_multiaddr.s_addr = m_dwMulticastIPv4[0];
					mcast.imr_interface.s_addr = htonl(INADDR_ANY);
					if (m_nSocket_rtp >= 0 && setsockopt(m_nSocket_rtp,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
					{
						RTSP_DEBUG("setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

					}
				}
				m_bRTPMulticast = 0;
			}
			if (m_bRTCPMulticast)
			{
				if (m_bIPv6)
				{
					memcpy(&mcast6.ipv6mr_multiaddr, m_dwMulticastIPv4,16);
					mcast6.ipv6mr_interface = htonl(INADDR_ANY);
					if (m_nSocket_rtcp >= 0 && setsockopt(m_nSocket_rtcp,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
					{
						RTSP_DEBUG("ipv6 setsockopt rtcp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

					}


				}
				else
				{
					mcast.imr_multiaddr.s_addr = m_dwMulticastIPv4[0];
					mcast.imr_interface.s_addr = htonl(INADDR_ANY);
					if (m_nSocket_rtcp >= 0 && setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
					{
						RTSP_DEBUG("setsockopt rtcp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

					}
				}
				m_bRTCPMulticast = 0;
			}


    }
    return 0;
}

void RTPSession::LeaveAllMulticastGroups()
{
    if (!created)
        return;
    if(m_bClient)
    {

		struct ip_mreq mcast;
		struct ipv6_mreq mcast6;
		if (m_bRTPMulticast)
		{
			if (m_bIPv6)
			{
				memcpy(&mcast6.ipv6mr_multiaddr, m_dwMulticastIPv4,16);
				mcast6.ipv6mr_interface = htonl(INADDR_ANY);
				if (m_nSocket_rtp >= 0 && setsockopt(m_nSocket_rtp,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
				{
					RTSP_DEBUG("ipv6 setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

				}


			}
			else
			{
				mcast.imr_multiaddr.s_addr = m_dwMulticastIPv4[0];
				mcast.imr_interface.s_addr = htonl(INADDR_ANY);
				if (m_nSocket_rtp >= 0 && setsockopt(m_nSocket_rtp,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
				{
					RTSP_DEBUG("setsockopt rtp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

				}
			}
			m_bRTPMulticast = 0;
		}
		if (m_bRTCPMulticast)
		{
			if (m_bIPv6)
			{
				memcpy(&mcast6.ipv6mr_multiaddr, m_dwMulticastIPv4,16);
				mcast6.ipv6mr_interface = htonl(INADDR_ANY);
				if (m_nSocket_rtcp >= 0 && setsockopt(m_nSocket_rtcp,IPPROTO_IPV6,IPV6_DROP_MEMBERSHIP,(char *)&mcast6,sizeof(mcast6)) < 0)
				{
					RTSP_DEBUG("ipv6 setsockopt rtcp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

				}


			}
			else
			{
				mcast.imr_multiaddr.s_addr = m_dwMulticastIPv4[0];
				mcast.imr_interface.s_addr = htonl(INADDR_ANY);
				if (m_nSocket_rtcp >= 0 && setsockopt(m_nSocket_rtcp,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mcast,sizeof(mcast)) < 0)
				{
					RTSP_DEBUG("setsockopt rtcp error! drop membership [%d]-<%s>\n",errno,strerror(errno));

				}
			}
			m_bRTCPMulticast = 0;
		}

    }
    return ;
}




int RTPSession::AddDestination(char *pIP,int nPort,int nPort_rtcp,int bIPv6 ,int bTCP,int nCh,void *pClient)
{
	uint32_t uiIP[4]={0,0,0,0};

    if (bIPv6)
    {
        return -1;
    }
    if (!bTCP)
    {
		if (bIPv6)
		{

			inet_pton(AF_INET6,pIP,uiIP);

		}
		else
		{
			uiIP[0] = inet_addr(pIP);
			if (uiIP[0] == 0 || uiIP[0] == -1)
			{
				return -1;
			}
		}


    }



    return AddDestination(uiIP,nPort,nPort_rtcp,bTCP,nCh,pClient,bIPv6);
}
int RTPSession::AddDestination(unsigned int uiIP[4],int nPort,int nPort_rtcp,int bTCP,int nCH,void *pClient,int bIPv6)
{
    IPADDRESS_NODE_T *pNew,*pNode;
    int nFoundIdx = -1,i,nUseCnt;
    // check exist
    m_tIPAddrLock.Lock();
    pNode = m_pIPAddressNode_Head;
    while (pNode != NULL)
    {
        if (pNode->uiIPV4[0] == uiIP[0] &&
			pNode->uiIPV4[1] == uiIP[1] &&
			pNode->uiIPV4[2] == uiIP[2] &&
			pNode->uiIPV4[3] == uiIP[3] &&
            pNode->uiPort == nPort &&
            pNode->bTCP == bTCP &&
            pNode->uiCH == nCH)
        {
            printf("[%s.%d] found\n",__FUNCTION__,__LINE__);
            break;

        }

        pNode = pNode->pNext;
    }
    if (pNode != NULL)
    {
        //已存在
        int nNewIdx = -1;
        nFoundIdx = -1;
        pNode->nUseCnt++;
        nFoundIdx = -1;
        nUseCnt = 0;
        for (i = 0; i < ANTS_RTP_MAX_NODE_CLIENT_NUM; i++)
        {
            if(pNode->pClient[i] != NULL)
            {
                nUseCnt ++;
                if (pNode->pClient[i] == pClient)
                {
                    nFoundIdx = i;
                }
            }
            else if(nNewIdx == -1)
            {
                nNewIdx = i;
            }
        }
        if (nFoundIdx == -1 && nNewIdx != -1)
        {
            pNode->pClient[nNewIdx] = pClient;
            nUseCnt ++;
        }
        pNode->nUseCnt = nUseCnt;
#if 0
        printf("[%s.%d]exist(btcp=%d ,uip=%x,port=%u,ch=%d,cnt = %d pclient=%x)\n",__FUNCTION__,__LINE__,pNode->bTCP,pNode->uiIPV4,pNode->uiPort,pNode->uiCH,pNode->nUseCnt,pClient);
        {
            int nTotal = 0;
            printf("Clients:\n");
            for (i = 0; i < ANTS_RTP_MAX_NODE_CLIENT_NUM; i++)
            {
                if(pNode->pClient[i] != NULL)
                {
                    nTotal++;
                    printf("%x ",pNode->pClient[i]);
                }

            }
            printf("Clients end %d\n",nTotal);
        }
#endif


        m_tIPAddrLock.Unlock();

        return 0;
    }



    pNew = new IPADDRESS_NODE_T;
    if (pNew == NULL)
    {
        m_tIPAddrLock.Unlock();
        printf("[%s.%d]new failed\n",__FUNCTION__,__LINE__);

        return -1;
    }
    memset(pNew,0,sizeof(IPADDRESS_NODE_T));


    pNew->bTCP = bTCP;
    pNew->bIPV4 = !bIPv6;
    pNew->uiIPV4[0] = uiIP[0];
	pNew->uiIPV4[1] = uiIP[1];
	pNew->uiIPV4[2] = uiIP[2];
	pNew->uiIPV4[3] = uiIP[3];
    pNew->uiPort = nPort;
    pNew->uiPort_rtcp = nPort_rtcp;
    pNew->uiCH = nCH;
    pNew->pNext = NULL;
    pNew->pPrev = NULL;
    pNew->nUseCnt = 1;
    pNew->bNeedIFrame = 1;
    pNew->pClient[0] = pClient;
    if (m_pIPAddressNode_Tail == NULL)
    {
        m_pIPAddressNode_Head =
            m_pIPAddressNode_Tail = pNew;
    }
    else
    {
        m_pIPAddressNode_Tail->pNext = pNew;
        pNew->pPrev = m_pIPAddressNode_Tail;
        m_pIPAddressNode_Tail = pNew;
    }
    m_nIPAddressNodeCnt++;
#if 0
    printf("[%s.%d]new(btcp=%d ,uip=%x,port=%u,ch=%d,cnt = %d NodeCnt=%d)\n",__FUNCTION__,__LINE__,pNew->bTCP,pNew->uiIPV4,pNew->uiPort,pNew->uiCH,pNew->nUseCnt,m_nIPAddressNodeCnt);
    {
        int nTotal = 0;
        printf("Clients:\n");
        for (i = 0; i < ANTS_RTP_MAX_NODE_CLIENT_NUM; i++)
        {
            if(pNew->pClient[i] != NULL)
            {
                nTotal++;
                printf("%x ",pNew->pClient[i]);
            }

        }
        printf("Clients end %d\n",nTotal);
    }
#endif
    m_tIPAddrLock.Unlock();
    return 0;

}

int RTPSession::GetDestinationCnt()
{
    int nCnt ;
    if (m_bExternSocket)
    {
        if (m_nTranType == 1)
        {
            return 1;
        }


    }

    m_tIPAddrLock.Lock();
    nCnt = m_nIPAddressNodeCnt;
    m_tIPAddrLock.Unlock();
    return nCnt;
}
int RTPSession::IsMyDestination(unsigned int uiIP[4],int nPort)
{
    IPADDRESS_NODE_T *pNew,*pNode;
    // check exist
    m_tIPAddrLock.Lock();
    pNode = m_pIPAddressNode_Head;
    while (pNode != NULL)
    {
        if (pNode->uiIPV4[0] == uiIP[0] && pNode->uiIPV4[1] == uiIP[1] && pNode->uiIPV4[2] == uiIP[2] && pNode->uiIPV4[3] == uiIP[3] && (pNode->uiPort == nPort || pNode->uiPort == 0) && pNode->bTCP == 0)
        {
            m_tIPAddrLock.Unlock();
            return 1;
        }

        pNode = pNode->pNext;
    }
    m_tIPAddrLock.Unlock();
    return 0;
}
int RTPSession::DeleteDestination(char *pIP,int nPort,int bIPv6,void *pClient)
{
    uint32_t uiIP[4];

    if (bIPv6)
    {
        return -1;
    }
	if(bIPv6)
	{
		inet_pton(AF_INET6,pIP,uiIP);
	}
	else
	{
		uiIP[0] = inet_addr(pIP);
		if (uiIP[0] == 0 || uiIP[0] == -1)
		{
			return -1;
		}
	}


    return -1;// DeleteDestination(uiIP,nPort,pClient);
}
int RTPSession::DeleteDestination(unsigned int uiIP[4],int nPort,int bTCP,int nCH ,void *pClient)
{
    IPADDRESS_NODE_T *pDel,*pNode;
    int bFoundIdx = -1,i,cnt = 0;
    // check exist
    m_tIPAddrLock.Lock();
    pNode = m_pIPAddressNode_Head;
    while (pNode != NULL)
    {
        RTSP_DEBUG("[%s.%d]node(btcp=%d ,uip=%x/%x/%x/%x,port=%u,ch=%d,cnt = %d NodeCnt=%d)\n",__FUNCTION__,__LINE__,pNode->bTCP,pNode->uiIPV4[0],pNode->uiIPV4[1],pNode->uiIPV4[2],pNode->uiIPV4[3],pNode->uiPort,pNode->uiCH,pNode->nUseCnt,m_nIPAddressNodeCnt);
        //printf("[%s.%d]node(btcp=%d ,uip=%x,port=%u,ch=%d,cnt = %d NodeCnt=%d)\n",__FUNCTION__,__LINE__,pNode->bTCP,pNode->uiIPV4,pNode->uiPort,pNode->uiCH,pNode->nUseCnt,m_nIPAddressNodeCnt);

        if (pNode->bTCP == bTCP &&
            pNode->uiIPV4[0] == uiIP[0] &&
			pNode->uiIPV4[1] == uiIP[1] &&
			pNode->uiIPV4[2] == uiIP[2] &&
			pNode->uiIPV4[3] == uiIP[3] &&
            pNode->uiPort == nPort &&
            pNode->uiCH == nCH)
        {
            break;

        }

        pNode = pNode->pNext;
    }
    if (pNode == NULL)
    {
        m_tIPAddrLock.Unlock();
        RTSP_DEBUG("[%s.%d](btcp=%d ,uip=%x/%x/%x/%x,port=%u,ch=%d)\n",__FUNCTION__,__LINE__,bTCP,pNode->uiIPV4[0],pNode->uiIPV4[1],pNode->uiIPV4[2],pNode->uiIPV4[3],nPort,nCH);
		//printf("[%s.%d](btcp=%d ,uip=%x,port=%u,ch=%d)\n",__FUNCTION__,__LINE__,bTCP,uiIP,nPort,nCH);
        return 0;
    }
        //已存在
        pNode->nUseCnt--;
        cnt = 0;
        for (i = 0; i < ANTS_RTP_MAX_NODE_CLIENT_NUM;i++)
        {
            if (pNode->pClient[i] != NULL)
            {
                if (pNode->pClient[i] == pClient)
                {
                    pNode->pClient[i] = NULL;
                }
                else
                {
                    cnt++;
                }
            }




    }
#if 0
    printf("[%s.%d](btcp=%d ,uip=%x,port=%u,ch=%d,cnt = %d/%d NodeCnt=%d)\n",__FUNCTION__,__LINE__,pNode->bTCP,pNode->uiIPV4,pNode->uiPort,pNode->uiCH,pNode->nUseCnt,cnt,m_nIPAddressNodeCnt);
    {
        int nTotal = 0;
        printf("Clients:\n");
        for (i = 0; i < ANTS_RTP_MAX_NODE_CLIENT_NUM; i++)
        {
            if(pNode->pClient[i] != NULL)
            {
                nTotal++;
                printf("%x ",pNode->pClient[i]);
            }

        }
        printf("Clients end %d\n",nTotal);
    }
#endif
    if (cnt > 0)
    {
        m_tIPAddrLock.Unlock();
        return 0;
    }

    pDel = pNode;
    if (pDel->pNext == NULL && pDel->pPrev == NULL)
    {
        // 最后一个
        m_pIPAddressNode_Head =
            m_pIPAddressNode_Tail = NULL;
    }
    else if (pDel->pNext == NULL)
    {
        //尾
        m_pIPAddressNode_Tail = pDel->pPrev;
        m_pIPAddressNode_Tail->pNext = NULL;
    }
    else if (pDel->pPrev == NULL)
    {
        // 头
        m_pIPAddressNode_Head = m_pIPAddressNode_Head->pNext;
        m_pIPAddressNode_Head->pPrev = NULL;
    }
    else
    {
        pNode = pDel->pPrev;
        pNode->pNext = pDel->pNext;
        pNode = pDel->pNext;
        pNode->pPrev = pDel->pPrev;

    }
    m_nIPAddressNodeCnt--;
    if (m_nIPAddressNodeCnt < 0 )
    {
        m_nIPAddressNodeCnt = 0;
    }

    delete pDel;

    RTSP_DEBUG("NodeCnt = %d\n",m_nIPAddressNodeCnt);
	//printf("NodeCnt = %d\n",m_nIPAddressNodeCnt);
    m_tIPAddrLock.Unlock();

    return 0;
}
unsigned char *RTPSession::GetPacketBuffer()
{
    if (m_bRecording)
    {
        return m_pRTPPacketBuffer + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) + sizeof(RTPExtensionOnvifHeader);
    }

    return m_pRTPPacketBuffer + sizeof(RTPHeader);
}
int RTPSession::GetPacketMaxDataSize()
{
    if (m_bRecording)
    {
        return ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionOnvifHeader);
    }

    return ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader);
}

unsigned char *RTPSession::GetAntsCombPacketBuffer()
{

    return m_pRTPPacketBuffer + sizeof(RTPHeader) + sizeof(RTPExtensionHeader) + sizeof(RTPExtensionAntsCombHeader_T);

}
int RTPSession::GetAntsCombPacketMaxDataSize()
{

    if (m_nSocket_rtp > 0)
    {
        return ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionAntsCombHeader_T);
    }
	else
	{
        return ANTS_RTPPACKET_ANTSCOMB_MAXSIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionAntsCombHeader_T);
	}
}

void RTPSession::SetTCPDataCallBack(ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser)
{
    m_pRtpRtcp_DataPacketCallback_fxn = fxn;
    m_pRtpRtcpCallbackUser = pUser;

}
int RTPSession::SendPacket(int len,int bMark,uint32_t timestamp)
{
    RTPHeader *pHeader;
    RTPExtensionOnvifHeader *pExtOnvifHeader;
    RTPExtensionHeader *pExtHeader;
    int nSendLen;
    if (m_bRecording)
    {
        if (len > ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionOnvifHeader))
        {
            RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
            return -1;
        }

    }
    else if (len > ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader))
    {
        RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
        return -1;
    }
    pHeader = (RTPHeader *)(m_pRTPPacketBuffer);
    // memset(pHeader,0,sizeof(RTPHeader));
    pHeader->version = 2;
    pHeader->marker = (bMark != 0);
    pHeader->payloadtype = m_uiDefaultPayloadType;
    pHeader->ssrc = m_uiSSRC;
    m_uiSeqNumber++;
    pHeader->sequencenumber = htons(m_uiSeqNumber);
    pHeader->timestamp = htonl(timestamp);
	m_uiTimeStamp = timestamp;
    nSendLen = len+sizeof(RTPHeader);
    if (m_bRecording)
    {
        pHeader->extension = 1;

        pExtHeader = (RTPExtensionHeader *)(pHeader + 1);
        pExtOnvifHeader = (RTPExtensionOnvifHeader *)(pExtHeader + 1);
        nSendLen += sizeof(RTPExtensionOnvifHeader);
        nSendLen += sizeof(RTPExtensionHeader);
        pExtHeader->extid = htons(0xABAC);
        pExtHeader->length = htons(3);
        pExtOnvifHeader->padding = 0;
        pExtOnvifHeader->seq = m_nLastPlaySeq & 0xFF;
        pExtOnvifHeader->mbz = 0;
        pExtOnvifHeader->ntpTimeStamp0 = htonl(m_ntpTimeStamp0);
        pExtOnvifHeader->ntpTimeStamp1 = htonl(m_ntpTimeStamp1);
        pExtOnvifHeader->discontinuity = m_discontinuity;
        pExtOnvifHeader->eSectionEnd = m_eSectionEnd;
        pExtOnvifHeader->cIFrameStart = m_cIFrameStart;
    }

    // printf("Send Seq:%d, time = %u\n",m_uiSeqNumber,timestamp);
    return SendRTPData(m_pRTPPacketBuffer,nSendLen);


}
int RTPSession::SendPacket(const void *data,size_t len,int bMark,uint32_t timestamp)
{
    RTPHeader *pHeader;
    RTPExtensionOnvifHeader *pExtOnvifHeader;
    RTPExtensionHeader *pExtHeader;
    int nSendLen;
    if (m_bRecording)
    {
        if (len > ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionOnvifHeader))
        {
            RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
            return -1;
        }

    }
    else if (len > ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader))
    {
        RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
        return -1;
    }

    pHeader = (RTPHeader *)(m_pRTPPacketBuffer);
    //memset(pHeader,0,sizeof(RTPHeader));
    pHeader->version = 2;
    pHeader->marker = (bMark != 0);
    pHeader->payloadtype = m_uiDefaultPayloadType;
    pHeader->ssrc = m_uiSSRC;
    m_uiSeqNumber++;
    pHeader->sequencenumber = htons(m_uiSeqNumber);
    pHeader->timestamp = htonl(timestamp);
	m_uiTimeStamp = timestamp;
    nSendLen = len+sizeof(RTPHeader);
    if (m_bRecording)
    {
        pHeader->extension = 1;

        pExtHeader = (RTPExtensionHeader *)(pHeader + 1);
        pExtOnvifHeader = (RTPExtensionOnvifHeader *)(pExtHeader + 1);
        nSendLen += sizeof(RTPExtensionOnvifHeader);
        nSendLen += sizeof(RTPExtensionHeader);
        pExtHeader->extid = htons(0xABAC);
        pExtHeader->length = htons(3);
        pExtOnvifHeader->padding = 0;
        pExtOnvifHeader->seq = m_nLastPlaySeq & 0xFF;
        pExtOnvifHeader->mbz = 0;
        pExtOnvifHeader->ntpTimeStamp0 = htonl(m_ntpTimeStamp0);
        pExtOnvifHeader->ntpTimeStamp1 = htonl(m_ntpTimeStamp1);
        pExtOnvifHeader->discontinuity = m_discontinuity;
        pExtOnvifHeader->eSectionEnd = m_eSectionEnd;
        pExtOnvifHeader->cIFrameStart = m_cIFrameStart;

        if (data != m_pRTPPacketBuffer+sizeof(RTPHeader) + sizeof(RTPExtensionOnvifHeader) + sizeof(RTPExtensionHeader))
        {
            memcpy(m_pRTPPacketBuffer+sizeof(RTPHeader) + sizeof(RTPExtensionOnvifHeader) + sizeof(RTPExtensionHeader),data,len);
        }
    }
    else if (data != m_pRTPPacketBuffer+sizeof(RTPHeader) )
    {
        if(m_uiDefaultPayloadType == 99)
        {
            int frameSize = len - 7; //remove adts header
            char payload[4] = {0};
            payload[0] = 0x00;
            payload[1] = 0x10;
            payload[2] = (frameSize & 0x1FE0) >> 5;
            payload[3] = (frameSize & 0x1F) << 3;

            //printf("aac len:[%d] [%d] [%d]\n",len, frameSize,sizeof(payload));

             pHeader->marker = 1;

            memcpy(m_pRTPPacketBuffer+sizeof(RTPHeader),payload,sizeof(payload));

            memcpy(m_pRTPPacketBuffer+sizeof(RTPHeader) + sizeof(payload),data+7,frameSize);

            nSendLen = frameSize + sizeof(payload) + sizeof(RTPHeader);
        }
        else
        {
            memcpy(m_pRTPPacketBuffer+sizeof(RTPHeader),data,len);
        }
    }
    //printf("seq = %d \n",pHeader->sequencenumber);

    //send
    SendRTPData(m_pRTPPacketBuffer,nSendLen);
    return 0;

}
int RTPSession::SendAntsCombPacketNoCopy(int nType,int nChan,int nStreamIdx, void *data,size_t len,int bMark,int bStart)
{
    RTPHeader *pHeader;
    RTPExtensionAntsCombHeader_T *pExtAntsCombHeader;
    RTPExtensionHeader *pExtHeader;
    int nSendLen;
    if (m_bRecording)
    {
        if (len > GetAntsCombPacketMaxDataSize())
        {
            return -1;
        }

    }
    else if (len > GetAntsCombPacketMaxDataSize() + sizeof(RTPExtensionHeader) + sizeof(RTPExtensionAntsCombHeader_T))
    {
        return -1;
    }

    pHeader = (RTPHeader *)(m_pRTPPacketBuffer);
    //memset(pHeader,0,sizeof(RTPHeader));
    pHeader->version = 2;
    pHeader->marker = (bMark != 0);
    pHeader->payloadtype = m_uiDefaultPayloadType;
    pHeader->ssrc = m_uiSSRC;
    m_uiSeqNumber++;
    pHeader->sequencenumber = htons(m_uiSeqNumber);
    pHeader->timestamp = htonl(0);
    nSendLen = len+sizeof(RTPHeader);

    pHeader->extension = 1;

    pExtHeader = (RTPExtensionHeader *)(pHeader + 1);
    pExtAntsCombHeader = (RTPExtensionAntsCombHeader_T *)(pExtHeader + 1);
    nSendLen += sizeof(RTPExtensionAntsCombHeader_T);
    nSendLen += sizeof(RTPExtensionHeader);
    pExtHeader->extid = htons(ANTS_RTSP_COMB_HEADER_MARK);
    pExtHeader->length = htons(sizeof(RTPExtensionAntsCombHeader_T) / sizeof(uint32_t));
    memset(pExtAntsCombHeader,0,sizeof(RTPExtensionAntsCombHeader_T));
    pExtAntsCombHeader->byType = nType;
    pExtAntsCombHeader->byStreamIdx = nStreamIdx;
    pExtAntsCombHeader->wChan = nChan;
    pExtAntsCombHeader->byStart = bStart;
    pExtAntsCombHeader->byEnd = bMark;
    // printf("seq = %d \n",pHeader->sequencenumber);

    //send
    //printf("m_uiSeqNumber = %d---------------------------------------------------------------------\n", m_uiSeqNumber);
    SendRTPData((char *)m_pRTPPacketBuffer,sizeof(RTPHeader) + sizeof(RTPExtensionHeader) + sizeof(RTPExtensionAntsCombHeader_T),NULL,0,data,len);
    return 0;
}

int RTPSession::DoCheckSend()
{// 返回还剩余的数据大小
    int nSend,nSendLen = 0;
    int nRet = 0;
    if (m_nSocket_tcp < 0)
    {
        return -1;
    }

    do
    {
        if (m_nCurrPos >= m_nTotalSize || m_pCurrFrame == NULL)
        {
            break;
        }
        // 非阻塞方式
        nSend = send(m_nSocket_tcp,m_pCurrFrame + m_nCurrPos,m_nTotalSize - m_nCurrPos,0);
        if (nSend > 0)
        {
            m_nCurrPos += nSend;
            nSendLen += nSend;
        }
        else
        {
            int errorNo = GetLastError();

            if(errorNo == EWOULDBLOCK ||
                errorNo == EINTR||
                errorNo == EAGAIN||
                errorNo == EINPROGRESS)
            {// 失败，下次再试
                //RTSP_DEBUG("[%s.%d]errno = %d %s[%d] m_bNeedClose = %d m_bUsing = %d\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket, m_bNeedClose, m_bUsing);
               break;
            }
            //失败，socket需要关掉
            nRet = -1;
            break;
        }


    } while (m_nCurrPos < m_nTotalSize);
    if (m_pCurrFrame != NULL)
    {
        if (m_nCurrPos >= m_nTotalSize)
        { // 数据发完
            free(m_pCurrFrame);
            m_pCurrFrame = NULL;
            m_nTotalSize = 0;
            m_nCurrPos = 0;
            m_nCurrFrameBufSize = 0;
            return 0;
        }
        else
        {
            return m_nTotalSize - m_nCurrPos;
        }

    }
    return nRet;
}
int RTPSession::SendPacketByBuffer(int nCh,const void *data,size_t len,char *pAdd,int nAddLen,int bMark,uint32_t timestamp,int bRTCP)
{
// tcp
    RTPHeader tHeader;
    int pktlen,nNum;
    short wLen;

#define H264RTPSESSION_RTP_BUFFSIZE_INC  (8 * 1024)

    if (m_pCurrFrame != NULL && (m_nCurrPos >= m_nTotalSize || m_nRTP_Error == 1))
    {
        free(m_pCurrFrame);
        m_pCurrFrame = NULL;
        m_nCurrPos = 0;
        m_nTotalSize = 0;
        m_nCurrFrameBufSize = 0;


    }
    if (m_nRTP_Error)
    {
        return -1;
    }
    if (m_pCurrFrame == NULL || (bRTCP == 0 && len + 4 +sizeof(RTPHeader) + nAddLen > m_nCurrFrameBufSize - m_nTotalSize) ||
        (bRTCP && len + 4 + nAddLen > m_nCurrFrameBufSize - m_nTotalSize))
    {
        //
        char *pTmp;
        int nNewSize;
        nNewSize = m_nCurrFrameBufSize + H264RTPSESSION_RTP_BUFFSIZE_INC;
        pTmp = (char *)realloc(m_pCurrFrame,nNewSize);
        if (pTmp == NULL)
        {
            m_nRTP_Error = 1;//
            return -1;
        }
        m_pCurrFrame = pTmp;
        m_nCurrFrameBufSize = nNewSize;
    }

    // $
    m_pCurrFrame[m_nTotalSize] = '$';
    m_nTotalSize++;
    m_pCurrFrame[m_nTotalSize] = nCh;
    m_nTotalSize++;
    if (bRTCP)
    {
        wLen = len + nAddLen;
    }
    else
    {
        wLen = len + sizeof(RTPHeader) + nAddLen;
    }

    m_pCurrFrame[m_nTotalSize] = (wLen >> 8) & 0xFF;
    m_nTotalSize++;
    m_pCurrFrame[m_nTotalSize] = wLen & 0xFF;
    m_nTotalSize++;
    if (!bRTCP)
    {

        // rtp header
        memset(&tHeader,0,sizeof(RTPHeader));
        tHeader.version = 2;
        tHeader.marker = (bMark != 0);
        tHeader.payloadtype = m_uiDefaultPayloadType;
        tHeader.ssrc = m_uiSSRC;
        m_uiSeqNumber++;
        tHeader.sequencenumber = htons(m_uiSeqNumber);
        tHeader.timestamp = htonl(timestamp);
		m_uiTimeStamp = timestamp;
        memcpy(m_pCurrFrame + m_nTotalSize,&tHeader,sizeof(RTPHeader));
        m_nTotalSize += sizeof(RTPHeader);
    }
    if (pAdd != NULL)
    {
        memcpy(m_pCurrFrame + m_nTotalSize,pAdd,nAddLen);
        m_nTotalSize += nAddLen;
    }

    memcpy(m_pCurrFrame + m_nTotalSize,data,len);
    m_nTotalSize += len;
    return 0;

}

int RTPSession::SendPacketNoCopy(const void *data,size_t len,char *pAdd,int nAddLen,int bMark,uint32_t timestamp)
{
    int nHeadLen = 0;
    ANTS_RTP_DATA_INFO_T *pInfo;
    RTPHeader *pHeader;
    RTPExtensionOnvifHeader *pExtOnvifHeader;
    RTPExtensionHeader *pExtHeader;
    int nSendLen;
    if (m_bRecording)
    {
        if (len > ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader) - sizeof(RTPExtensionHeader) - sizeof(RTPExtensionOnvifHeader))
        {
            RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
            return -1;
        }

    }
    else if (len > ANTS_RTPPACKET_MAXSIZE - sizeof(RTPHeader))
    {
        RTSP_DEBUG("[%s.%d]\n",__FUNCTION__,__LINE__);
        return -1;
    }

    pInfo = (ANTS_RTP_DATA_INFO_T *)malloc(sizeof(ANTS_RTP_DATA_INFO_T));
    if (pInfo == NULL)
    {
        return -1;
    }

    memset(pInfo,0,sizeof(ANTS_RTP_DATA_INFO_T));
    pInfo->pHeader = pInfo->pBaseMem + 4;

    pHeader = (RTPHeader *)(pInfo->pHeader);
    //memset(pHeader,0,sizeof(RTPHeader));
    pHeader->version = 2;
    pHeader->marker = (bMark != 0);
    pHeader->payloadtype = m_uiDefaultPayloadType;
    pHeader->ssrc = m_uiSSRC;
    m_uiSeqNumber++;
    pHeader->sequencenumber = htons(m_uiSeqNumber);
    pHeader->timestamp = htonl(timestamp);
	m_uiTimeStamp = timestamp;
    nSendLen = len+sizeof(RTPHeader);
    nHeadLen = sizeof(RTPHeader);
    if (m_bRecording)
    {
        pHeader->extension = 1;

        pExtHeader = (RTPExtensionHeader *)(pHeader + 1);
        pExtOnvifHeader = (RTPExtensionOnvifHeader *)(pExtHeader + 1);
        nSendLen += sizeof(RTPExtensionOnvifHeader);
        nSendLen += sizeof(RTPExtensionHeader);
        nHeadLen += sizeof(RTPExtensionOnvifHeader);
        nHeadLen += sizeof(RTPExtensionHeader);
        pExtHeader->extid = htons(0xABAC);
        pExtHeader->length = htons(3);
        pExtOnvifHeader->padding = 0;
        pExtOnvifHeader->seq = m_nLastPlaySeq & 0xFF;
        pExtOnvifHeader->mbz = 0;
        pExtOnvifHeader->ntpTimeStamp0 = htonl(m_ntpTimeStamp0);
        pExtOnvifHeader->ntpTimeStamp1 = htonl(m_ntpTimeStamp1);
        pExtOnvifHeader->discontinuity = m_discontinuity;
        pExtOnvifHeader->eSectionEnd = m_eSectionEnd;
        pExtOnvifHeader->cIFrameStart = m_cIFrameStart;

        if (data != m_pRTPPacketBuffer+sizeof(RTPHeader) + sizeof(RTPExtensionOnvifHeader) + sizeof(RTPExtensionHeader))
        {
            // memcpy(m_pRTPPacketBuffer+sizeof(RTPHeader) + sizeof(RTPExtensionOnvifHeader) + sizeof(RTPExtensionHeader),data,len);
        }
    }
    else if (data != m_pRTPPacketBuffer+sizeof(RTPHeader) )
    {
        //memcpy(m_pRTPPacketBuffer+sizeof(RTPHeader),data,len);
    }
    pInfo->nHeadLen = nHeadLen;
    pInfo->pData = (char *)data;
    pInfo->nDataSize = len;
    if (pAdd != NULL)
    {
        memcpy(pInfo->pAdd ,pAdd,nAddLen);
        pInfo->nAddSize = nAddLen;
    }
    else
    {
        pInfo->nAddSize = 0;
    }

    m_tIPAddrLock.Lock();

    m_pDataInfo[m_nPktCount] = pInfo;
    m_nPktCount++;
    m_tIPAddrLock.Unlock();
    if (bMark || m_nPktCount >= ANTS_RTPPACKET_NUM)
    {
        int i;

        SendRTPDataNoCopy();
        m_tIPAddrLock.Lock();
        for (i = 0; i < m_nPktCount; i++)
        {
            if(m_pDataInfo[i] != NULL)
            {
                free(m_pDataInfo[i]);
                m_pDataInfo[i] = NULL;
            }

        }

        m_nPktCount = 0;
        m_tIPAddrLock.Unlock();
    }



    return 0;
}
int RTPSession::SendRTPDataNoCopy()
{
    IPADDRESS_NODE_T *pNode,*pNodeClients = NULL;
    int nRet;
    RTCPCommonHeader *pRtcpHeader;
    struct sockaddr_in rtpaddr;
	struct sockaddr_in6 rtpaddrIPv6;
	struct sockaddr *pAddr = NULL;
	int AddrLen = 0;
    int nPos,nTotalSize;
    int i,nNode;
    int nNodeClientsNum = 0;
    int nOffset[ANTS_RTPPACKET_NUM],nLen[ANTS_RTPPACKET_NUM],nTotalPkt = 0;
    char *pBaseData = NULL,*pCurr;
#if 0
    if (m_bClient)
    {
        return -1;
    }
#endif
    pBaseData = (char *)malloc((1500 + 4) * m_nPktCount);
    if (pBaseData == NULL)
    {
        return -1;
    }

    nPos = 0;

    m_tIPAddrLock.Lock();
    if (m_nIPAddressNodeCnt > 0)
    {
        pNodeClients = new IPADDRESS_NODE_T[m_nIPAddressNodeCnt];
        if (pNodeClients == NULL)
        {
            free(pBaseData);
            m_tIPAddrLock.Unlock();
            return -1;
        }
        nNodeClientsNum = 0;
        pNode = m_pIPAddressNode_Head;
        while (pNode != NULL)
        {
            if (nNodeClientsNum >= m_nIPAddressNodeCnt)
            {
                break;
            }

            pNodeClients[nNodeClientsNum] = *pNode;
            nNodeClientsNum++;
            pNode = pNode->pNext;
        }


    }

    for (i = 0; i < m_nPktCount; i++)
    {
        if (m_pDataInfo[i] != NULL)
        {
            nPos += 4;
            nOffset[i] = nPos;
            memcpy(pBaseData + nPos,m_pDataInfo[i]->pHeader,m_pDataInfo[i]->nHeadLen);
            nPos += m_pDataInfo[i]->nHeadLen;
            if (m_pDataInfo[i]->nAddSize > 0)
            {
                memcpy(pBaseData + nPos,m_pDataInfo[i]->pAdd,m_pDataInfo[i]->nAddSize);
                nPos+=m_pDataInfo[i]->nAddSize;
            }
            memcpy(pBaseData + nPos,m_pDataInfo[i]->pData,m_pDataInfo[i]->nDataSize);
            nPos += m_pDataInfo[i]->nDataSize;
            nLen[i] = m_pDataInfo[i]->nHeadLen + m_pDataInfo[i]->nDataSize + m_pDataInfo[i]->nAddSize;
            free(m_pDataInfo[i]);
            m_pDataInfo[i] = NULL;
            nTotalPkt++;
        }
    }
    m_nPktCount = 0;

    m_tIPAddrLock.Unlock();



    nTotalSize = nPos;




    for(nNode = 0; nNode < nNodeClientsNum; nNode++)
    {
        pNode = &pNodeClients[nNode];



        if (pNode->bTCP == 1)
        {
            if (m_pRtpRtcp_DataPacketCallback_fxn != NULL)
            {

                // 填充TCP前置头 $+CH+len
                for (i = 0; i < nTotalPkt; i++)
                {
                    pCurr = pBaseData+nOffset[i] - 4;
                    pCurr[0] = '$';
                    pCurr[1] = pNode->uiCH;
                    pCurr[3] = nLen[i] &0xFF;
                    pCurr[2] = (nLen[i] >> 8) &0xFF;
                }
                nRet = m_pRtpRtcp_DataPacketCallback_fxn(pNode->pClient[0],pNode->uiIPV4,pNode->uiPort,pNode->uiCH,0,NULL,0,NULL,0,(char *)pBaseData ,nTotalSize,m_pRtpRtcpCallbackUser);
            }
        }
        else if(m_nSocket_rtp > 0)
        {
			if (pNode->bIPV4)
			{
				memset(&rtpaddr,0,sizeof(struct sockaddr_in));
				rtpaddr.sin_family = AF_INET;
				rtpaddr.sin_port = htons(pNode->uiPort);
				rtpaddr.sin_addr.s_addr = pNode->uiIPV4[0];
				pAddr = (struct sockaddr *)&rtpaddr;
				AddrLen = sizeof(struct sockaddr_in);
			}
			else
			{
				memset(&rtpaddrIPv6,0,sizeof(struct sockaddr_in6));
				rtpaddrIPv6.sin6_family = AF_INET6;
				rtpaddrIPv6.sin6_port = htons(pNode->uiPort);
				memcpy(&rtpaddrIPv6.sin6_addr,pNode->uiIPV4,16);
				pAddr = (struct sockaddr *)&rtpaddrIPv6;
				AddrLen = sizeof(struct sockaddr_in6);
			}


            for (i = 0; i < nTotalPkt; i++)
            {
                nRet = sendto(m_nSocket_rtp,(const char *)pBaseData+nOffset[i],nLen[i],0,(const struct sockaddr *)pAddr,AddrLen);
				//printf("SendRTPDataNoCopy sendto nRet = %d!!!!!!!!!!!!!!!!!!!!!1111\n", nRet);
            }

        }


    }


    free(pBaseData);
    pBaseData = NULL;
    if (pNodeClients != NULL)
    {
        delete [] pNodeClients;
    }


    return 0;

}

int RTPSession::SendH264RtpPacked(const void *data,size_t len)
{
    IPADDRESS_NODE_T *pNode,*pNodeClients = NULL;
    int nRet;
    RTCPCommonHeader *pRtcpHeader;
    struct sockaddr_in rtpaddr;
	struct sockaddr_in6 rtpaddrV6;
	struct sockaddr *pAddr = NULL;
	int addrLen = 0;
    int nNodeClientsNum = 0,nNode = 0;
#if 0
    if (m_bClient)
    {
        return -1;
    }

    if (m_nSocket_rtp == -1)
    {
        return -1;
    }
#endif

    m_tIPAddrLock.Lock();
    // 客户端拷贝
    if (m_nIPAddressNodeCnt > 0)
    {
        pNodeClients = new IPADDRESS_NODE_T[m_nIPAddressNodeCnt];
        if (pNodeClients == NULL)
        {
            m_tIPAddrLock.Unlock();
            return -1;
        }
        nNodeClientsNum = 0;
        pNode = m_pIPAddressNode_Head;
        while (pNode != NULL)
        {
            if (nNodeClientsNum >= m_nIPAddressNodeCnt)
            {
                break;
            }

            pNodeClients[nNodeClientsNum] = *pNode;
            nNodeClientsNum++;
            pNode = pNode->pNext;
        }


    }


    m_tIPAddrLock.Unlock();






    for(nNode = 0; nNode < nNodeClientsNum; nNode++)
    {

        pNode = &pNodeClients[nNode];

        if (pNode->bTCP == 1)
        {
            if (m_pRtpRtcp_DataPacketCallback_fxn != NULL)
            {

                nRet = m_pRtpRtcp_DataPacketCallback_fxn(pNode->pClient[0],pNode->uiIPV4,pNode->uiPort,pNode->uiCH,0,NULL,0,NULL,0,(char *)data,len,m_pRtpRtcpCallbackUser);
            }

        }
        else if(m_nSocket_rtp > 0)
        {
            unsigned char *pPack;
            int nPktLen,nPktPos;
			if(pNode->bIPV4)
			{
				memset(&rtpaddr,0,sizeof(struct sockaddr_in));
				rtpaddr.sin_family = AF_INET;
				rtpaddr.sin_port = htons(pNode->uiPort);
				rtpaddr.sin_addr.s_addr = pNode->uiIPV4[0];
				addrLen = sizeof(struct sockaddr_in);
				pAddr = (struct sockaddr *)&rtpaddr;
			}
			else
			{
				memset(&rtpaddrV6,0,sizeof(struct sockaddr_in6));
				rtpaddrV6.sin6_family = AF_INET6;
				rtpaddrV6.sin6_port = htons(pNode->uiPort);
				memcpy(&rtpaddrV6.sin6_addr,pNode->uiIPV4,16);
				addrLen = sizeof(struct sockaddr_in6);
				pAddr = (struct sockaddr *)&rtpaddrV6;

			}
            nPktPos = 0;
            pPack = (unsigned char *)data;
            while(1)
            {
                if (nPktPos >= len)
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
                if (nPktPos + nPktLen + 4> len)
                {
                    break;
                }

                nRet = sendto(m_nSocket_rtp,(const char *)pPack + 4,nPktLen,0,(const struct sockaddr *)pAddr,addrLen);
                pPack += 4 + nPktLen;
                nPktPos += 4 + nPktLen;

            }
            // nRet = sendto(m_nSocket_rtp,(const char *)data + nPos,nSendLen,0,(const struct sockaddr *)&rtpaddr,sizeof(struct sockaddr_in));

        }



    }

    if (pNodeClients != NULL)
    {
        delete [] pNodeClients;
    }

    return 0;

}

int RTPSession::SendRTPData(const void *data,size_t len)
{
    IPADDRESS_NODE_T *pNode,*pNodeClients = NULL;
    int nRet;
    RTCPCommonHeader *pRtcpHeader;

    int bTry = 0,nSendLen,nPos;
    int nNodeClientsNum = 0,nNode = 0;
    int bSend = 0;
#if 0
    if (m_bClient)
    {
        return -1;
    }

    if (m_nSocket_rtp == -1)
    {
        return -1;
    }
#endif
    pRtcpHeader = (RTCPCommonHeader *)data;
    if (pRtcpHeader->packettype >= 200 &&
        pRtcpHeader->packettype <= 204)
    {
        return -1;
    }

    m_tIPAddrLock.Lock();
    // 客户端拷贝
    if (m_nIPAddressNodeCnt > 0)
    {
        pNodeClients = new IPADDRESS_NODE_T[m_nIPAddressNodeCnt];
        if (pNodeClients == NULL)
        {
            m_tIPAddrLock.Unlock();
            return -1;
        }
        nNodeClientsNum = 0;
        pNode = m_pIPAddressNode_Head;
        while (pNode != NULL)
        {
            if (nNodeClientsNum >= m_nIPAddressNodeCnt)
            {
                break;
            }

            pNodeClients[nNodeClientsNum] = *pNode;
            nNodeClientsNum++;
            pNode = pNode->pNext;
        }


    }


    m_tIPAddrLock.Unlock();
    nPos = 0;
    nSendLen = len;
    for(nNode = 0; nNode < nNodeClientsNum; nNode++)
    {
        pNode = &pNodeClients[nNode];


        if (pNode->bTCP == 1)
        {
            if (m_pRtpRtcp_DataPacketCallback_fxn != NULL)
            {
                if (pNode->nUseCnt > 1)
                {
                    RTSP_DEBUG("RTSP-TCP -USECNT = %d\n",pNode->nUseCnt);
                }

                nRet = m_pRtpRtcp_DataPacketCallback_fxn(pNode->pClient[0],pNode->uiIPV4,pNode->uiPort,pNode->uiCH,0,NULL,0,NULL,0,(char *)data,len,m_pRtpRtcpCallbackUser);


            }


        }
        else if(m_nSocket_rtp > 0)
        {
			 struct sockaddr_in rtpaddr;
			  struct sockaddr_in6 rtpaddr6;
			  struct sockaddr *pAddr;
			  int AddrLen = 0;
			if (pNode->bIPV4)
			{
				memset(&rtpaddr,0,sizeof(struct sockaddr_in));
				rtpaddr.sin_family = AF_INET;
				rtpaddr.sin_port = htons(pNode->uiPort);
				rtpaddr.sin_addr.s_addr = pNode->uiIPV4[0];
				AddrLen = sizeof(struct sockaddr_in);
				pAddr = (struct sockaddr *)&rtpaddr;
			}
			else
			{
				memset(&rtpaddr6,0,sizeof(struct sockaddr_in6));
				rtpaddr6.sin6_family = AF_INET6;
				rtpaddr6.sin6_port = htons(pNode->uiPort);
				memcpy(&rtpaddr6.sin6_addr,pNode->uiIPV4,16);
				AddrLen = sizeof(struct sockaddr_in6);
				pAddr = (struct sockaddr *)&rtpaddr6;
			}


            nRet = sendto(m_nSocket_rtp,(const char *)data,len,0,(const struct sockaddr *)pAddr,AddrLen);
		    //printf("SendRTPData sendto nRet = %d!!!!!!!!!!!!!!!!!!!!!\n", nRet);
        }




    }

    if (pNodeClients != NULL)
    {
        delete [] pNodeClients;
    }

    return 0;

}

int RTPSession::SendRTPData(char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen, void *data,size_t len)
{
    IPADDRESS_NODE_T *pNode,*pNodeClients = NULL;
    int nRet;
    RTCPCommonHeader *pRtcpHeader;
    struct sockaddr_in rtpaddr;
	 struct sockaddr_in6 rtpaddr6;
	 struct sockaddr *pAddr = NULL;
	 int addrLen = 0;
    int nNodeClientsNum = 0,nNode = 0;
    int bUDP_DataReady = 0,nUDPSendLen = 0;
    void *pUDP_Data = NULL;
#if 0
    if (m_bClient)
    {
        return -1;
    }

    if (m_nSocket_rtp == -1)
    {
        return -1;
    }
#endif
    if (pRTPHeader == NULL )
    {
        pRtcpHeader = (RTCPCommonHeader *)data;
        if (pRtcpHeader->packettype >= 200 &&
            pRtcpHeader->packettype <= 204)
        {
            return -1;
        }
    }

    m_tIPAddrLock.Lock();
    // 客户端拷贝
    if (m_nIPAddressNodeCnt > 0)
    {
        pNodeClients = new IPADDRESS_NODE_T[m_nIPAddressNodeCnt];
        if (pNodeClients == NULL)
        {
            m_tIPAddrLock.Unlock();
            return -1;
        }
        nNodeClientsNum = 0;
        pNode = m_pIPAddressNode_Head;
        while (pNode != NULL)
        {
            if (nNodeClientsNum >= m_nIPAddressNodeCnt)
            {
                break;
            }

            pNodeClients[nNodeClientsNum] = *pNode;
            nNodeClientsNum++;
            pNode = pNode->pNext;
        }


    }


    m_tIPAddrLock.Unlock();
    for(nNode = 0; nNode < nNodeClientsNum; nNode++)
    {
        pNode = &pNodeClients[nNode];



        if (pNode->bTCP == 1)
        {
            if (m_pRtpRtcp_DataPacketCallback_fxn != NULL)
            {
                if (pNode->nUseCnt > 1)
                {
                    RTSP_DEBUG("RTSP-TCP -USECNT = %d\n",pNode->nUseCnt);
                }

                nRet = m_pRtpRtcp_DataPacketCallback_fxn(pNode->pClient[0],pNode->uiIPV4,pNode->uiPort,pNode->uiCH,0,pRTPHeader,nRTPHeadLen,pAdd,nAddLen,(char *)data,len,m_pRtpRtcpCallbackUser);


            }


        }
        else if(m_nSocket_rtp > 0)
        {
			if (pNode->bIPV4)
			{
				memset(&rtpaddr,0,sizeof(struct sockaddr_in));
				rtpaddr.sin_family = AF_INET;
				rtpaddr.sin_port = htons(pNode->uiPort);
				rtpaddr.sin_addr.s_addr = pNode->uiIPV4[0];
				pAddr = (struct sockaddr *)&rtpaddr;
				addrLen = sizeof(struct sockaddr_in);
			}
			else
			{
				memset(&rtpaddr6,0,sizeof(struct sockaddr_in6));
				rtpaddr6.sin6_family = AF_INET6;
				rtpaddr6.sin6_port = htons(pNode->uiPort);
				memcpy(&rtpaddr6.sin6_addr,pNode->uiIPV4,16);
				pAddr = (struct sockaddr *)&rtpaddr6;
				addrLen = sizeof(struct sockaddr_in6);
			}


            if (!bUDP_DataReady)
            {
                if (pRTPHeader == NULL && pAdd == NULL)
                {
                    nUDPSendLen = len;
                    pUDP_Data = data;
                }
                else if (m_pRTPPacketBuffer == data)
                {
                    nUDPSendLen = len;
                    pUDP_Data = data;
                }
                else
                {
                    if (pRTPHeader != NULL)
                    {
                        if (m_pRTPPacketBuffer != (uint8_t *)pRTPHeader)
                        {
                            memcpy(m_pRTPPacketBuffer,pRTPHeader,nRTPHeadLen);

                        }
                        nUDPSendLen += nRTPHeadLen;
                    }
                    if (pAdd != NULL)
                    {
                        if (m_pRTPPacketBuffer + nRTPHeadLen != (uint8_t *)pRTPHeader)
                        {
                            memcpy(m_pRTPPacketBuffer + nRTPHeadLen,pAdd,nAddLen);

                        }
                        nUDPSendLen += nAddLen;
                    }
                    if (data != NULL)
                    {
                        if (m_pRTPPacketBuffer + nRTPHeadLen + nAddLen != data)
                        {
                            memcpy(m_pRTPPacketBuffer + nRTPHeadLen + nAddLen,data,len);

                        }
                        nUDPSendLen += len;
                    }
                    pUDP_Data = m_pRTPPacketBuffer;

                }
                bUDP_DataReady = 1;
            }

            nRet = sendto(m_nSocket_rtp,(const char *)pUDP_Data,nUDPSendLen,0,(const struct sockaddr *)pAddr,addrLen);

        }


    }

    if (pNodeClients != NULL)
    {
        delete [] pNodeClients;
    }

    return 0;
}

int RTPSession::SendRTCPData(const void *data,size_t len)
{
    IPADDRESS_NODE_T *pNode,*pNodeClients = NULL;
    int nRet;
    RTCPCommonHeader *pRtcpHeader;
    struct sockaddr_in rtcpaddr;
	 struct sockaddr_in6 rtcpaddr6;
	  struct sockaddr *paddr;
	  int AddrLen = 0;
    int nNodeClientsNum = 0,nNode = 0;
    if ((m_nTranType != 1) && m_nSocket_rtcp == -1)
    {
        return -1;
    }
    pRtcpHeader = (RTCPCommonHeader *)data;
    if (pRtcpHeader->packettype >= 200 &&
        pRtcpHeader->packettype <= 204)
    {

    }
    else
    {
        return -1;
    }
    if (m_bClient && m_nTranType == 1)
    {
        if (m_pRtpRtcp_DataPacketCallback_fxn != NULL)
        {
                m_pRtpRtcp_DataPacketCallback_fxn(NULL,0,0,1,1,NULL,0,NULL,0,data,len,m_pRtpRtcpCallbackUser);
        }
        return 0;
    }

    m_tIPAddrLock.Lock();
    // 客户端拷贝
    if (m_nIPAddressNodeCnt > 0)
    {
        pNodeClients = new IPADDRESS_NODE_T[m_nIPAddressNodeCnt];
        if (pNodeClients == NULL)
        {
            m_tIPAddrLock.Unlock();
            return -1;
        }
        nNodeClientsNum = 0;
        pNode = m_pIPAddressNode_Head;
        while (pNode != NULL)
        {
            if (nNodeClientsNum >= m_nIPAddressNodeCnt)
            {
                break;
            }

            pNodeClients[nNodeClientsNum] = *pNode;
            nNodeClientsNum++;
            pNode = pNode->pNext;
        }


    }


    m_tIPAddrLock.Unlock();
    for(nNode = 0; nNode < nNodeClientsNum; nNode++)
    {
        pNode = &pNodeClients[nNode];

        if (pNode->bTCP == 1)
        {
            if (m_pRtpRtcp_DataPacketCallback_fxn != NULL)
            {
                    m_pRtpRtcp_DataPacketCallback_fxn(pNode->pClient[0],pNode->uiIPV4,pNode->uiPort,pNode->uiCH+1,1,NULL,0,NULL,0,data,len,m_pRtpRtcpCallbackUser);
            }
        }
        else
        {
			if (pNode->bIPV4)
			{
				memset(&rtcpaddr,0,sizeof(struct sockaddr_in));
				rtcpaddr.sin_family = AF_INET;
				rtcpaddr.sin_port = htons(pNode->uiPort_rtcp);
				rtcpaddr.sin_addr.s_addr = pNode->uiIPV4[0];
				paddr = (struct sockaddr *)&rtcpaddr;
				AddrLen = sizeof(struct sockaddr_in);
			}
			else
			{
				memset(&rtcpaddr6,0,sizeof(struct sockaddr_in6));
				rtcpaddr6.sin6_family = AF_INET6;
				rtcpaddr6.sin6_port = htons(pNode->uiPort_rtcp);
				memcpy(&rtcpaddr6.sin6_addr,pNode->uiIPV4,16);
				paddr = (struct sockaddr *)&rtcpaddr6;
				AddrLen = sizeof(struct sockaddr_in6);
			}


            nRet = sendto(m_nSocket_rtcp,(const char *)data,len,0,(const struct sockaddr *)paddr,AddrLen);
        }
    }
    if (pNodeClients != NULL)
    {
        delete [] pNodeClients;
    }
    return 0;
}



#ifdef RTP_SUPPORT_SENDAPP

int RTPSession::SendRTCPAPPPacket(uint8_t subtype, const uint8_t name[4], const void *appdata, size_t appdatalen)
{
    int status;

    if (!created)
        return -1;

    return 0;
}

#endif // RTP_SUPPORT_SENDAPP

#ifdef RTP_SUPPORT_RTCPUNKNOWN

int RTPSession::SendUnknownPacket(bool sr, uint8_t payload_type, uint8_t subtype, const void *data, size_t len)
{
    int status;

    if (!created)
        return -1;


    return 0;
}

#endif // RTP_SUPPORT_RTCPUNKNOWN

int RTPSession::SetDefaultPayloadType(uint8_t pt)
{
    if (!created)
        return -1;

    int status;

    m_uiDefaultPayloadType = pt;
    return 0;
}

int RTPSession::SetSecondPayloadType(uint8_t pt)
{
	if (!created)
		return -1;

	int status;

	m_uiSecondPayloadType = pt;
	return 0;
}

int RTPSession::SetDefaultMark(bool m)
{
    if (!created)
        return -1;

    int status;

    m_uiDefaultMark = m;
    return 0;
}

int RTPSession::SetDefaultTimestampIncrement(uint32_t timestampinc)
{
    if (!created)
        return -1;

    int status;

    m_uiTimeStampInc = timestampinc;
    return 0;
}





int RTPSession::SetReceiveMode(int m)
{
    if (!created)
        return -1;
    return 0;
}

int RTPSession::AddToIgnoreList(uint32_t uiIPv4,int nPort)
{
    if (!created)
        return -1;
    return 0;
}

int RTPSession::DeleteFromIgnoreList(uint32_t uiIPv4,int nPort)
{
    if (!created)
        return -1;
    return 0;
}

void RTPSession::ClearIgnoreList()
{
    if (!created)
        return;

}

int RTPSession::AddToAcceptList(uint32_t uiIPv4,int nPort)
{
    if (!created)
        return -1;
    return 0;
}

int RTPSession::DeleteFromAcceptList(uint32_t uiIPv4,int nPort)
{
    if (!created)
        return -1;
    return 0;
}

void RTPSession::ClearAcceptList()
{
    if (!created)
        return;

}

int RTPSession::SetMaximumPacketSize(size_t s)
{
    if (!created)
        return -1;



    maxpacksize = s;
    return 0;
}


int RTPSession::SetTimestampUnit(double u)
{
    if (!created)
        return -1;

    int status;


    return status;
}

void RTPSession::SetNameInterval(int count)
{
    if (!created)
        return;

}

void RTPSession::SetEMailInterval(int count)
{
    if (!created)
        return;

}

void RTPSession::SetLocationInterval(int count)
{
    if (!created)
        return;

}

void RTPSession::SetPhoneInterval(int count)
{
    if (!created)
        return;

}

void RTPSession::SetToolInterval(int count)
{
    if (!created)
        return;

}

void RTPSession::SetNoteInterval(int count)
{
    if (!created)
        return;

}

int RTPSession::SetLocalName(const void *s,size_t len)
{
    if (!created)
        return -1;

    int status;

    return status;
}

int RTPSession::SetLocalEMail(const void *s,size_t len)
{
    if (!created)
        return -1;

    int status;

    return status;
}

int RTPSession::SetLocalLocation(const void *s,size_t len)
{
    if (!created)
        return -1;

    int status;

    return status;
}

int RTPSession::SetLocalPhone(const void *s,size_t len)
{
    if (!created)
        return -1;

    int status;

    return status;
}

int RTPSession::SetLocalTool(const void *s,size_t len)
{
    if (!created)
        return -1;

    int status;

    return status;
}

int RTPSession::SetLocalNote(const void *s,size_t len)
{
    if (!created)
        return -1;

    int status;

    return status;
}


int RTPSession::CreateCNAME(uint8_t *buffer,size_t *bufferlength,bool resolve)
{

    return -1;
}
static unsigned int m_uiRandom  = 0;
static JMutex m_RandomLock;
static int m_bRandomInit = 0;
uint32_t RTPSession::GetRandom32()
{
    uint32_t Random ;
    if (!m_bRandomInit)
    {
        time_t testTime;
        m_bRandomInit = 1;
        m_RandomLock.Init();
        time(&testTime);
        m_uiRandom = testTime;
    }
    m_RandomLock.Lock();
    m_uiRandom++;
    Random = m_uiRandom;
    m_RandomLock.Unlock();

    return Random;
}
uint32_t RTPSession::GetRandom16()
{
    uint32_t Random ;
    if (!m_bRandomInit)
    {
        time_t testTime;
        m_bRandomInit = 1;
        m_RandomLock.Init();
        time(&testTime);
        m_uiRandom = testTime;
    }
    m_RandomLock.Lock();
    m_uiRandom++;
    Random = m_uiRandom & 0xFFFF;
    m_RandomLock.Unlock();
    return Random;
}



} // end namespace

