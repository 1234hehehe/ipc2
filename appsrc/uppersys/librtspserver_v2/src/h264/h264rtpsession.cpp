#include "h264rtpsession.h"
#include "rtsp_common.h"
#include "rtsp.h"
#include "bitstream_264.h"

#include "rtspclient_api.h"
// MJPEG
static unsigned char const lum_dc_codelens[] = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static unsigned char const lum_dc_symbols[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static unsigned char const lum_ac_codelens[] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

static unsigned char const lum_ac_symbols[] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa,
};

static unsigned char const chm_dc_codelens[] = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

static unsigned char const chm_dc_symbols[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static unsigned char const chm_ac_codelens[] = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

static unsigned char const chm_ac_symbols[] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa,
};

// The default 'luma' and 'chroma' quantizer tables, in zigzag order:
static unsigned char const defaultQuantizers[128] = {
    // luma table:
    16, 11, 12, 14, 12, 10, 16, 14,
    13, 14, 18, 17, 16, 19, 24, 40,
    26, 24, 22, 22, 24, 49, 35, 37,
    29, 40, 58, 51, 61, 60, 57, 51,
    56, 55, 64, 72, 92, 78, 64, 68,
    87, 69, 55, 56, 80, 109, 81, 87,
    95, 98, 103, 104, 103, 62, 77, 113,
    121, 112, 100, 120, 92, 101, 103, 99,
    // chroma table:
    17, 18, 18, 24, 21, 24, 47, 26,
    26, 47, 99, 66, 56, 66, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};


// H.264
/*
NAL Unit Type Content of NAL Unit NRI (binary)
----------------------------------------------------------------
1 non-IDR coded slice 10
2 Coded slice data partition A 10
3 Coded slice data partition B 01
4 Coded slice data partition C 01
5 IDR                          11
*/

#define NAL_SLICE		1
#define NAL_DPA			2
#define NAL_DPB			3
#define NAL_DPC			4
#define NAL_IDR_SLICE		5
#define NAL_SEI			6
#define NAL_SPS			7
#define NAL_PPS			8
#define NAL_PICTURE_DELIMITER	9
#define NAL_FILTER_DATA		10

#define    SLICE_TYPE_P    0
#define    SLICE_TYPE_B    1
#define    SLICE_TYPE_I    2
#define    SLICE_TYPE_SP   3
#define    SLICE_TYPE_SI   4


#define BSWAP(a) { \
    unsigned int _temp0,_temp1,_temp2,_temp3,_temp4;\
    _temp1=(a & 0xFF00FF00)>>8;\
    _temp0=(a & 0x00FF00FF)<<8;\
    _temp2=_temp0+_temp1;\
    _temp3=(_temp2 & 0x0000FFFF)<<16;\
    _temp4=(_temp2 & 0xFFFF0000)>>16;\
    a=_temp3+_temp4;\
                }

#define NALU_GET_F(a) (((a)>>7)&1)
#define NALU_GET_NRI(a) (((a)>>5)&3)
#define NALU_GET_TYPE(a) (((a)>>0)&31)

#define NALU_RESET(a)  (a)=0
#define NALU_SET_F(a,f) (a) |= (((f)&1) << 7)
#define NALU_SET_NRI(a,nri) (a) |= (((nri)&3) << 5)
#define NALU_SET_TYPE(a,type) (a) |= (((type)&31) << 0)

#define FU_RESET(a)  (a)=0
#define FU_SET_F(a,f) (a) |= (((f)&1) << 7)
#define FU_SET_NRI(a,nri) (a) |= (((nri)&3) << 5)
#define FU_SET_TYPE(a,type) (a) |= (((type)&31) << 0)

#define FU_GET_F(f)  (((f) >> 7)&1)
#define FU_GET_NRI(nri)  (((nri) >> 5)&3)
#define FU_GET_TYPE(type)  (((type) >> 0)&31)


#define NALU2FU(to,from)  (to) = (from)

#define FUHEADER_RESET(a) (a)=0
#define FUHEADER_SET_TYPE(a,type) (a) |= (((type)&31) << 0)
#define FUHEADER_SET_R(a,r) (a) |= (((r)&1) << 5)
#define FUHEADER_SET_E(a,e) (a) |= (((e)&1) << 6)
#define FUHEADER_SET_S(a,s) (a) |= (((s)&1) << 7)

#define FUHEADER_GET_TYPE(a) (((a) >> 0)&31)
#define FUHEADER_GET_R(a)   (((a) >> 5)&1)
#define FUHEADER_GET_E(a)  (((a) >> 6)&1)
#define FUHEADER_GET_S(a)  (((a) >> 7)&1)


// SVAC
#define SVAC_NALU_GET_F(a) (((a)>>7)&1)
#define SVAC_NALU_GET_NRI(a) (((a)>>6)&1)
#define SVAC_NALU_GET_TYPE(a) (((a)>>2)&15)


static uint8_t ff_log2_tab[256]={
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static int ff_log2_c(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}



class CRtspServer;
static const uint32_t RTSP_STARTCODE_H264 = 0x01000000;
static int frametypeMap[] = {AntsPktPFrames,AntsPktBPFrames,AntsPktIFrames,AntsPktPFrames,AntsPktBPFrames};
static int H265_frametypeMap[] = {AntsPktBPFrames,AntsPktPFrames,AntsPktIFrames};

CH264RtpSession::CH264RtpSession(){
    m_nLastStamp = 0;m_bDefaultMark = 0; m_nDefaultPayloadType = -1;m_tAliveTime = 0;
    m_uiFrameNo = 0;
    m_uiLastFrameNo = 0;
    m_bFrameDealing = 0;
	m_nSecondPayloadType = -1;
	m_uiFrameNo_Sub = 0;

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
    m_nlog2_max_frame_num = 0;
    m_nframe_mbs_only_flag = 0;
    m_nresidual_color_transform_flag = 0;
    m_nRecvDataPos = 0;
    m_nRecvDataStart = 0;
    m_nRecvDataStart = sizeof(AntsFrameHeader);
    m_bRecvSPS = 0;
    m_bRecvPPS = 0;
    m_bRecvVPS = 0;
    m_nLastSeq= 0;
    m_nWidth = 0;
    m_nHeight = 0;
    m_nProfile_idc = 0;
    m_nLevel_idc = 0;
    m_bSpecial_Ex = 0;
    m_bSpecial_Fix = 0;
    m_bSpecial_MSlices = 0;
    m_dwLastFrameIndex = 0;
    m_dwLastKeyFrameIndex = 0;
    m_nLastSlicetype = 0;
    m_nCurrSlicetype = 0;
    m_nCurrIDR_pic_id = 0;
    m_nLastIDR_pic_id = 0;
    m_nCurrNalType = 0;
    m_nLastNalType = 0;
    m_nresidual_color_transform_flag = 0;
    m_nframe_mbs_only_flag = 0;
    m_nLastTimestamp = 0;
    m_nLastframe_num = 0;
    m_nLastfirst_mb_in_slice = 0;
    m_nfirst_mb_in_slice = 0;
    m_pSPS = NULL;
    m_pPPS = NULL;
    m_pVPS = NULL;
    m_nSPSLen = 0;
    m_nPPSLen = 0;
    m_nVPSLen = 0;
    m_nCurrStamp = 0;
	m_nCurrStamp_recv = 0;
    m_bLastRecvError = 0;
    m_uLastOffset24 = 0;
    m_wResetInterval = 0;
    m_nQTableNum = 0;
    m_bVideoReady = 0;
    m_byQTable[0] = m_byQTable[1] = m_byQTable[2] = m_byQTable[3] = NULL;
    m_wQTableLength[0] = m_wQTableLength[1] = m_wQTableLength[2] = m_wQTableLength[3] = 0;
    SetPayloadClockRate(90000);
    m_hMemLock.Init();
    m_bFirstFrame = 0;

    m_bLastMark = 0;
    // H.265
    m_dependent_slice_segments_enabled_flag = 0;
    m_num_extra_slice_header_bits = 0;
    m_slice_address_length = 0;

    m_status = ps_padding;
    //m_nRecvDataPos = 0;
	m_PesLen = 0;
	m_PesHeadLen = 0;
    m_PesLenAudio = 0;
	m_pARecvDataBuff = NULL;
	m_nARecvDataBuffSize = 0;
	m_nARecvDataStart = 0;
	m_bAudio = -1;
	m_uiAFrameNo = 0;
	m_bIFrame = 0;
	m_pAG711Buff = NULL;
	m_bPsLastRecvError = 0;
	m_nFrameCnt = 0;

	m_VideoCodecId = AntsH264_hisi_RTP;
	m_AudioCodecId = AntsG711A;

	m_PesLenBd = 0;
	m_nBdRecvDataPos = 0;
    m_first_slice_segment_in_pic_flag = 0;
}
CH264RtpSession::~CH264RtpSession()
{
    m_hMemLock.Lock();
    if (m_pRecvDataBuff != NULL)
    {
        free(m_pRecvDataBuff);
        m_pRecvDataBuff = NULL;
    }
    m_nRecvDataBuffSize = 0;
    if (m_pARecvDataBuff != NULL)
    {
        free(m_pARecvDataBuff);
        m_pARecvDataBuff = NULL;
    }
    m_nARecvDataBuffSize = 0;
    if (m_pAG711Buff != NULL)
    {
        free(m_pAG711Buff);
        m_pAG711Buff = NULL;
    }
    if (m_pSPS != NULL)
    {
        delete [] m_pSPS;
        m_pSPS = NULL;
    }
    if (m_pPPS != NULL)
    {
        delete [] m_pPPS;
        m_pPPS = NULL;
    }

    if (m_pVPS != NULL)
    {
        delete [] m_pVPS;
        m_pVPS = NULL;
    }
    m_nSPSLen = 0;
    m_nPPSLen = 0;
    m_hMemLock.Unlock();

}

int CH264RtpSession::SetDefaultPayloadType(uint8_t pt)
{
	int status;
	status = RTPSession::SetDefaultPayloadType(pt);
    m_nDefaultPayloadType = pt;
	if (!status)
	{
		m_nDefaultPayloadType = pt;
	}
    if (m_nDefaultPayloadType != -1)
    {
        m_bVideoReady = 1;
    }
	return status;


}

int CH264RtpSession::SetSecondPayloadType(uint8_t pt)
{
	int status;
	status = RTPSession::SetSecondPayloadType(pt);
	m_nSecondPayloadType = pt;

	return status;

}

int CH264RtpSession::SetSPS(char *pSPSData,int nSPSLen)
{
   // RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
    m_hMemLock.Lock();
    if (pSPSData == NULL || nSPSLen == 0)
    {
        if (m_pSPS != NULL)
        {
            delete []m_pSPS;
            m_pSPS = NULL;
            m_nSPSLen = 0;
        }
        m_hMemLock.Unlock();
       // RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
        return NULL;
    }
  // RTSP_DEBUG("[%s.%d] this = %x\n",__FUNCTION__,__LINE__,this);
    if (m_pSPS != NULL)
    {
         if (nSPSLen + 4 != m_nSPSLen || 0 != memcmp(m_pSPS + 4,pSPSData,nSPSLen))
         {
             delete m_pSPS;
             m_pSPS = NULL;
             m_nSPSLen = 0;
        //     RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
         }

    }
    //RTSP_DEBUG("[%s.%d] m_pSPS = %x\n",__FUNCTION__,__LINE__,m_pSPS);
    if (m_pSPS == NULL)
    {
        m_nSPSLen = 0;
        m_pSPS = new char [nSPSLen + 4];
        if (m_pSPS == NULL)
        {
            m_hMemLock.Unlock();
    //        RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
            return -1;
        }
        m_pSPS[0] = 0x00;
        m_pSPS[1] = 0x00;
        m_pSPS[2] = 0x00;
        m_pSPS[3] = 0x01;
        memcpy(m_pSPS + 4,pSPSData,nSPSLen);
        m_nSPSLen = nSPSLen + 4;
    //    RTSP_DEBUG("[%s.%d] [%x]m_pSPS = %x m_nSPSLen = %d\n",__FUNCTION__,__LINE__,this,m_pSPS,m_nSPSLen);
    }

    m_hMemLock.Unlock();
    return 0;

}
int CH264RtpSession::SetPPS(char *pPPSData,int nPPSLen)
{
    m_hMemLock.Lock();
    if (pPPSData == NULL || nPPSLen == 0)
    {
        if (m_pPPS != NULL)
        {
            delete []m_pPPS;
            m_pPPS = NULL;
            m_nPPSLen = 0;
        }
        m_hMemLock.Unlock();
        return NULL;
    }

    if (m_pPPS != NULL)
    {
        if (nPPSLen + 4 != m_nPPSLen || 0 != memcmp(m_pPPS + 4,pPPSData,nPPSLen))
        {
            delete []m_pPPS;
            m_pPPS = NULL;
            m_nPPSLen = 0;
        }

    }
    if (m_pPPS == NULL)
    {
        m_nPPSLen = 0;
        m_pPPS = new char [nPPSLen + 4];
        if (m_pPPS == NULL)
        {
            m_hMemLock.Unlock();
            return -1;
        }
        m_pPPS[0] = 0x00;
        m_pPPS[1] = 0x00;
        m_pPPS[2] = 0x00;
        m_pPPS[3] = 0x01;
        memcpy(m_pPPS + 4,pPPSData,nPPSLen);
        m_nPPSLen = nPPSLen + 4;
    }

    m_hMemLock.Unlock();
    return 0;

}
int CH264RtpSession::SetVPS(char *pVPSData,int nVPSLen)
{
    m_hMemLock.Lock();
    if (pVPSData == NULL || nVPSLen == 0)
    {
        if (m_pVPS != NULL)
        {
            delete []m_pVPS;
            m_pVPS = NULL;
            m_nVPSLen = 0;
        }
        m_hMemLock.Unlock();
        return NULL;
    }

    if (m_pVPS != NULL)
    {
        if (nVPSLen + 4 != m_nVPSLen || 0 != memcmp(m_pVPS + 4,pVPSData,nVPSLen))
        {
            delete []m_pVPS;
            m_pVPS = NULL;
            m_nVPSLen = 0;
        }

    }
    if (m_pVPS == NULL)
    {
        m_nVPSLen = 0;
        m_pVPS = new char [nVPSLen + 4];
        if (m_pVPS == NULL)
        {
            m_hMemLock.Unlock();
            return -1;
        }
        m_pVPS[0] = 0x00;
        m_pVPS[1] = 0x00;
        m_pVPS[2] = 0x00;
        m_pVPS[3] = 0x01;
        memcpy(m_pVPS + 4,pVPSData,nVPSLen);
        m_nVPSLen = nVPSLen + 4;
    }

    m_hMemLock.Unlock();
    return 0;
}
char * CH264RtpSession::GetPPS_Base64()
{
    char *pReturn = NULL;
    m_hMemLock.Lock();
    if (m_pPPS == NULL || m_nPPSLen < 4)
    {
        m_hMemLock.Unlock();
        return NULL;
    }

    pReturn =  Ants_Rtsp_Base64Encode(m_pPPS+4,m_nPPSLen -4);
    m_hMemLock.Unlock();
    return pReturn;
}
char * CH264RtpSession::GetSPS_Base64()
{
    char *pReturn = NULL;
   // RTSP_DEBUG("[%s.%d] this = %x \n",__FUNCTION__,__LINE__,this);
    m_hMemLock.Lock();
    if (m_pSPS == NULL || m_nSPSLen < 4)
    {
     //   RTSP_DEBUG("[%s.%d]m_pSPS = %x m_nSPSLen = %d\n",__FUNCTION__,__LINE__,m_pSPS,m_nSPSLen);
        m_hMemLock.Unlock();
        return NULL;
    }
    //RTSP_DEBUG("[%s.%d]m_pSPS = %x m_nSPSLen = %d\n",__FUNCTION__,__LINE__,m_pSPS,m_nSPSLen);
    pReturn = Ants_Rtsp_Base64Encode(m_pSPS+4,m_nSPSLen-4);
    m_hMemLock.Unlock();
    return pReturn;
}
char * CH264RtpSession::GetVPS_Base64()
{
    char *pReturn = NULL;
    m_hMemLock.Lock();
    if (m_pVPS == NULL || m_nVPSLen < 4)
    {
        m_hMemLock.Unlock();
        return NULL;
    }
    pReturn = Ants_Rtsp_Base64Encode(m_pVPS+4,m_nVPSLen-4);
    m_hMemLock.Unlock();
    return pReturn;
}
int  CH264RtpSession::GetProfileLevelID()
{
    int nRet = -1,nValue;
    m_hMemLock.Lock();
    if (m_pVPS == NULL)
    {
        if (m_pSPS != NULL)
        {
            nValue = m_pSPS[5];
            nRet = nValue & 0xFF;
            nRet <<= 8;
            nValue = m_pSPS[6];
            nRet |= nValue & 0xFF;
            nRet <<= 8;
            nValue = m_pSPS[7];
            nRet |= nValue & 0xFF;
        }
    }

    m_hMemLock.Unlock();
    return nRet;
}


/** Sets the default marker for RTP packets to \c m. */
int CH264RtpSession::SetDefaultMark(bool m)
{
	int status;
	status = RTPSession::SetDefaultMark(m);
	if (!status)
	{
		m_bDefaultMark = m;
	}
	return status;

}

int CH264RtpSession::H265_SplitPacket(const uint8_t *data,size_t len,uint32_t timestamp)
{//输入完整NAL数据,不包括起始码
    int PackSize = GetPacketMaxDataSize();
    int nPos;
    int bMark;
    int nSendSize;
    int nRet = -1;
    unsigned char LayerID,TID,nalType,forbidden_bit;

    uint8_t FU[2],FUheader0,FUheader1,FUheader2;
    uint8_t *pPackBuf = NULL;


    pPackBuf = GetPacketBuffer();
    if (pPackBuf == NULL)
    {
        return -1;
    }

    // printf("[%s] len = %d PackSize = %d type = %d nri = %d\n",__FUNCTION__,len,PackSize,pNALU->TYPE,pNALU->NRI);

    if (len <= PackSize)
    {

        nalType = HEVC_NALU_GET_TYPE(data[0]);
#if 0
        forbidden_bit = HEVC_NALU_GET_F(data[0]);
        LayerID = HEVC_NALU_GET_LAYERID(data[0],data[1]);
        TID = HEVC_NALU_GET_TID(data[1]);
        printf("nalType = %d forbidden_bit = %d LayerID = %d TID = %d\n",nalType,forbidden_bit,LayerID,TID);
#endif
        // return SendPacket(data,len,1,timestamp);
        if (m_nTranType == 1 && m_bExternSocket)
        {
           return SendPacketByBuffer(m_nInterleaved[0],(char *)data,len,NULL,0,nalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23,timestamp);
        }


        return SendPacketNoCopy((char *)data,len,NULL,0,nalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23,timestamp);
    }
    //开始分片
    nPos = 0;
    bMark = 0;
    nSendSize = 0;
    // pFU = (FU_INDICATOR *)&FU;
    // pFUHeader = (FU_HEADER *)&FUheader;






    nalType = HEVC_NALU_GET_TYPE(data[0]);
    forbidden_bit = HEVC_NALU_GET_F(data[0]);
    LayerID = HEVC_NALU_GET_LAYERID(data[0],data[1]);
    TID = HEVC_NALU_GET_TID(data[1]);
    //printf("nalType = %d forbidden_bit = %d LayerID = %d TID = %d\n",nalType,forbidden_bit,LayerID,TID);


    HEVC_NALU_RESET(FU[0],FU[1]);
    HEVC_NALU_SET_F(FU[0],forbidden_bit);
    HEVC_NALU_SET_LAYERID(FU[0],FU[1],LayerID);
    HEVC_NALU_SET_TID(FU[1],TID);
    HEVC_NALU_SET_TYPE(FU[0],HEVC_FU_TYPE);

    //start
    HEVC_FU_RESET(FUheader0);
    HEVC_FU_SET_TYPE(FUheader0,nalType);
    HEVC_FU_SET_E(FUheader0,0);
    HEVC_FU_SET_S(FUheader0,1);

    //mid
    HEVC_FU_RESET(FUheader1);
    HEVC_FU_SET_TYPE(FUheader1,nalType);
    HEVC_FU_SET_E(FUheader1,0);
    HEVC_FU_SET_S(FUheader1,0);

    //end
    HEVC_FU_RESET(FUheader2);
    HEVC_FU_SET_TYPE(FUheader2,nalType);
    HEVC_FU_SET_E(FUheader2,1);
    HEVC_FU_SET_S(FUheader2,0);

    while(1)
    {

        bMark = 0;


        if (nPos == 0)
        {
            nSendSize = PackSize;
            nPos += 2;
            //memcpy(pPackBuf + 2,(char *)data + nPos,nSendSize - 2);
            pPackBuf[0] = FU[0];
            pPackBuf[1] = FU[1];
            pPackBuf[2] = FUheader0;
            if (m_nTranType == 1 && m_bExternSocket)
            {
                nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,nSendSize - 3,(char *)pPackBuf,3,bMark,timestamp);
            }
            else
            {
                nRet = SendPacketNoCopy((char *)data + nPos,nSendSize - 3,(char *)pPackBuf,3,bMark,timestamp);
            }

            nPos += nSendSize - 3;


        }
        else
        {
            if (PackSize - 3 >= len - nPos)
            {//尾
                bMark = 1;
                nSendSize = len - nPos + 3;
                //memcpy(pPackBuf + 2,(char *)data + nPos,nSendSize - 2);
                pPackBuf[0] = FU[0];
                pPackBuf[1] = FU[1];
                pPackBuf[2] = FUheader2;
                if (m_nTranType == 1 && m_bExternSocket)
                {
                    nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,nSendSize - 3,(char *)pPackBuf,3,bMark,timestamp);
                }
                else
                {
                    nRet = SendPacketNoCopy((char *)data + nPos,nSendSize - 3,(char *)pPackBuf,3,bMark,timestamp);
                }
                nPos += nSendSize - 3;




            }
            else
            {
                nSendSize = PackSize;
                //memcpy(pPackBuf + 2,(char *)data + nPos,nSendSize - 2);
                pPackBuf[0] = FU[0];
                pPackBuf[1] = FU[1];
                pPackBuf[2] = FUheader1;
                if (m_nTranType == 1 && m_bExternSocket)
                {
                    nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,nSendSize - 3,(char *)pPackBuf,3,bMark,timestamp);
                }
                else
                {
                    nRet = SendPacketNoCopy((char *)data + nPos,nSendSize - 3,(char *)pPackBuf,3,bMark,timestamp);
                }
                nPos += nSendSize - 3;


            }

        }

        // nRet = SendPacket(pPackBuf,nSendSize,bMark,timestamp);
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

 int CH264RtpSession::SplitPacket(const void *data,size_t len,uint32_t timestamp)
 {//输入完整NAL数据,不包括起始码
     int PackSize = GetPacketMaxDataSize();
     int nPos;
     int bMark;
     int nSendSize;
     int nRet = -1;
     unsigned char NRI,nalType,forbidden_bit;
     FU_INDICATOR *pFU;
     NALU_HEADER *pNALU;
     FU_HEADER *pFUHeader;
     uint8_t FU,FUheader0,FUheader1,FUheader2;
     uint8_t *pPackBuf = NULL;


     pNALU = (NALU_HEADER *)data;
     pPackBuf = GetPacketBuffer();
     if (pPackBuf == NULL)
     {
         return -1;
     }

     // printf("[%s] len = %d PackSize = %d type = %d nri = %d\n",__FUNCTION__,len,PackSize,pNALU->TYPE,pNALU->NRI);

     if (len <= PackSize)
     {
         nalType = NALU_GET_TYPE(*(char *)data);
       // return SendPacket(data,len,1,timestamp);
         if (m_nTranType == 1 && m_bExternSocket)
         {
             return SendPacketByBuffer(m_nInterleaved[0],(char *)data,len,NULL,0,nalType == NAL_SLICE || nalType == NAL_IDR_SLICE,timestamp);
         }
         return SendPacketNoCopy((char *)data,len,NULL,0,nalType == NAL_SLICE || nalType == NAL_IDR_SLICE,timestamp);
     }
     //开始分片
     nPos = 0;
     bMark = 0;
     nSendSize = 0;
    // pFU = (FU_INDICATOR *)&FU;
    // pFUHeader = (FU_HEADER *)&FUheader;






     NRI = NALU_GET_NRI(*(char *)data);
     nalType = NALU_GET_TYPE(*(char *)data);
     forbidden_bit = NALU_GET_F(*(char *)data);


     FU_RESET(FU);
     FU_SET_F(FU,forbidden_bit);
     FU_SET_NRI(FU,NRI);
     FU_SET_TYPE(FU,FU_TYPE_H264);

     //start
     FUHEADER_RESET(FUheader0);
     FUHEADER_SET_TYPE(FUheader0,nalType);
     FUHEADER_SET_R(FUheader0,0);
     FUHEADER_SET_E(FUheader0,0);
     FUHEADER_SET_S(FUheader0,1);

     //mid
     FUHEADER_RESET(FUheader1);
     FUHEADER_SET_TYPE(FUheader1,nalType);
     FUHEADER_SET_R(FUheader1,0);
     FUHEADER_SET_E(FUheader1,0);
     FUHEADER_SET_S(FUheader1,0);

     //end
     FUHEADER_RESET(FUheader2);
     FUHEADER_SET_TYPE(FUheader2,nalType);
     FUHEADER_SET_R(FUheader2,0);
     FUHEADER_SET_E(FUheader2,1);
     FUHEADER_SET_S(FUheader2,0);

     while(1)
     {

         bMark = 0;


        if (nPos == 0)
        {
            nSendSize = PackSize;
            nPos += 1;
            //memcpy(pPackBuf + 2,(char *)data + nPos,nSendSize - 2);
            pPackBuf[0] = FU;
            pPackBuf[1] = FUheader0;
            if (m_nTranType == 1 && m_bExternSocket)
            {
                nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,nSendSize - 2,(char *)pPackBuf,2,bMark,timestamp);
            }
            else
            {
                nRet = SendPacketNoCopy((char *)data + nPos,nSendSize - 2,(char *)pPackBuf,2,bMark,timestamp);
            }


            nPos += nSendSize - 2;


        }
        else
        {
            if (PackSize - 2 >= len - nPos)
            {//尾
                bMark = 1;
                nSendSize = len - nPos + 2;
                //memcpy(pPackBuf + 2,(char *)data + nPos,nSendSize - 2);
                pPackBuf[0] = FU;
                pPackBuf[1] = FUheader2;
                if (m_nTranType == 1 && m_bExternSocket)
                {
                    nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,nSendSize - 2,(char *)pPackBuf,2,bMark,timestamp);
                }
                else
                {
                    nRet = SendPacketNoCopy((char *)data + nPos,nSendSize - 2,(char *)pPackBuf,2,bMark,timestamp);
                }
                nPos += nSendSize - 2;




            }
            else
            {
                nSendSize = PackSize;
                //memcpy(pPackBuf + 2,(char *)data + nPos,nSendSize - 2);
                pPackBuf[0] = FU;
                pPackBuf[1] = FUheader1;
                if (m_nTranType == 1 && m_bExternSocket)
                {
                    nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,nSendSize - 2,(char *)pPackBuf,2,bMark,timestamp);
                }
                else
                {
                    nRet = SendPacketNoCopy((char *)data + nPos,nSendSize - 2,(char *)pPackBuf,2,bMark,timestamp);
                }
                nPos += nSendSize - 2;


            }

        }

       // nRet = SendPacket(pPackBuf,nSendSize,bMark,timestamp);
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

static void scaling_list(Bitstream *bs,int sizeOfScalingList)
{
                    int lastScale = 8;
                    int nextScale = 8;
                    int j,delta_scale;
                    int useDefaultScalingMatrixFlag,scalingList;
                    for( j = 0; j < sizeOfScalingList; j++ )
                      {
                        if( nextScale != 0 )
                        {
                        delta_scale = eg_read_se(bs);
                        nextScale = ( lastScale + delta_scale + 256 ) &0xFF;
                        useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
                         if(useDefaultScalingMatrixFlag)
                             break;
                        }
                        scalingList = ( nextScale == 0 ) ? lastScale : nextScale;
                        lastScale = scalingList;
                  }
 }
int CH264RtpSession::ReadSPS(void *pData,int Size,int bNeedCheck)
{
    int i;//,list;
    int x,y;
    Bitstream tmpBs,*bs=&tmpBs;
    int mb_num,mb_big_num,mb_count;
    int profile_idc,level_idc,sps_id;
    int chroma_format_idc,residual_color_transform_flag;
    int bit_depth_luma,bit_depth_chroma,transform_bypass;
    int seq_scaling_matrix_present_flag,seq_scaling_list_present_flag;
    int log2_max_frame_num,gaps_in_frame_num_allowed_flag;
    int poc_type,log2_max_poc_lsb,ref_frame_count;
    int i_mb_width,i_mb_height;
    int bcrop,crop_left,crop_right,crop_top,crop_bottom;
    int nWidth,nHeight,direct_8x8_inference_flag;
    if (bNeedCheck)
    { // 查找SPS头
        unsigned char *pCurr,*pEnd,c;
        int nZero = 0,bFound = 0;
        pCurr = (unsigned char *)pData;
        pEnd = pCurr + Size;
        do
        {
            if (pCurr == pEnd)
            {
                break;
            }
            c = pCurr[0];
            if (c == 0)
            {
                nZero++;
            }
            else if (c == 0x01)
            {
                if (nZero >= 2)
                {
                    // get
                    if (pCurr + 1 == pEnd)
                    {
                        break;
                    }
                    if (m_VideoCodecId == AntsSVAC)
                    {
                        if (SVAC_NALU_GET_TYPE(pCurr[1]) == 0x07)
                        {
                            // sps
                            bFound = 1;
                            break;
                        }
                        else if (SVAC_NALU_GET_TYPE(pCurr[1]) == 0x01 ||
                            SVAC_NALU_GET_TYPE(pCurr[1]) == 0x05 )
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (NALU_GET_TYPE(pCurr[1]) == 0x07)
                        {
                            // sps
                            bFound = 1;
                            break;
                        }
                        else if (NALU_GET_TYPE(pCurr[1]) == 0x01 ||
                            NALU_GET_TYPE(pCurr[1]) == 0x05 )
                        {
                            break;
                        }
                    }

                }
                nZero = 0;
            }
            else
            {
                nZero = 0;
            }
            pCurr++;
        } while (1);
        if (bFound)
        {
            Size = pEnd - pCurr - 2;
            pData = pCurr + 2;
        }
        else
        {
            return -1;
        }
    }

    BitstreamInit(bs,pData,Size);
    if (m_VideoCodecId == AntsSVAC)
    {
        profile_idc =eg_read_direct(bs,8);
        level_idc=eg_read_direct(bs,8);
        sps_id=eg_read_ue(bs);

         chroma_format_idc = eg_read_direct(bs,2);
         bit_depth_luma = eg_read_ue(bs)+8;
         bit_depth_chroma = eg_read_ue(bs)+8;
         i_mb_width=eg_read_ue(bs)+1;
         i_mb_height=eg_read_ue(bs)+1;

         crop_left = 0;
         crop_right = 0;
         crop_top = 0;
         crop_bottom = 0;
    }
    else
    {


   /*
   *Baseline profile -> 66
   *Main profile     -> 77
   *Extended profile -> 88
   *High profile     -> 100
   *High 10 profile  -> 110
   *High 4:2:2 profile-> 122
   *High 4:4:4 profile-> 144
   *High 4:4:4 Predictive profile ->244
   *CAVLC 4:4:4 Intra profile ->44
   */
    profile_idc =eg_read_direct(bs,8);

    READ_MARKER();
    READ_MARKER();
    READ_MARKER();
    eg_read_direct(bs,5);//reserved
    level_idc=eg_read_direct(bs,8);

    sps_id=eg_read_ue(bs);
    if (sps_id >= 32)
    {
        return -1;
    }
    if(profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118 ||
        profile_idc == 128)
    {//high profile
        chroma_format_idc = eg_read_ue(bs);
        if (chroma_format_idc > 3)
        {
            return -1;
        }
        residual_color_transform_flag = 0;
        if(chroma_format_idc == 3)
        {
            residual_color_transform_flag = eg_read_direct1(bs);
        }
        m_nresidual_color_transform_flag = residual_color_transform_flag;
        bit_depth_luma = eg_read_ue(bs)+8;
        bit_depth_chroma = eg_read_ue(bs)+8;
        if (bit_depth_luma > 14 || bit_depth_chroma > 14)
        {
            return -1;
        }
        transform_bypass = eg_read_direct1(bs);
        seq_scaling_matrix_present_flag = eg_read_direct1(bs);
        if (seq_scaling_matrix_present_flag)
        {
            for (i = 0; i < 8 + 4; i++)
            {
                if (i < 8 || (i > 7 &&chroma_format_idc == 3))
                seq_scaling_list_present_flag = eg_read_direct1(bs);
                if (seq_scaling_list_present_flag)
                {

                    if (i < 6)
                    {
                        scaling_list(bs,16);
                    }
                    else if(i < 8)
                    {
                        scaling_list(bs,64);
                    }
                    else if (chroma_format_idc == 3)
                    {
                        scaling_list(bs,64);
                    }
                }
            }
        }

    }
    else
    {
        chroma_format_idc = 1;
        bit_depth_luma = 8;
        bit_depth_chroma = 8;
    }


    log2_max_frame_num=eg_read_ue(bs)+4;
    if (log2_max_frame_num < 4 || log2_max_frame_num > 16)
    {
        return -1;
    }
    m_nlog2_max_frame_num = log2_max_frame_num;
    poc_type=eg_read_ue(bs);
    if(poc_type == 0)
    {
        log2_max_poc_lsb=eg_read_ue(bs)+4;
        if (log2_max_poc_lsb > 12 + 4)
        {
            return -1;
        }
    }
    else if(poc_type == 1)
    {
        //
        int delta_pic_order_always_zero_flag,offset_for_non_ref_pic,
            offset_for_top_to_bottom_field,poc_cycle_length,
            offset_for_ref_frame_256_i;
        delta_pic_order_always_zero_flag = eg_read_direct1(bs);
        offset_for_non_ref_pic = eg_read_se(bs);
        offset_for_top_to_bottom_field = eg_read_se(bs);
        poc_cycle_length = eg_read_ue(bs);
        if (poc_cycle_length >= 256)
        {// overflow
            return -1;
        }
        for (i = 0; i < poc_cycle_length; i++)
        {
            offset_for_ref_frame_256_i = eg_read_se(bs);
        }

    }
    else if(poc_type != 2)
    {
        //error;
        return -1;
    }

    ref_frame_count=eg_read_ue(bs);
    if (ref_frame_count > 32 -2 || ref_frame_count > 16)
    {
        return -1;
    }

    gaps_in_frame_num_allowed_flag=eg_read_direct1(bs);
    //h->sps.mb_width
    i_mb_width=eg_read_ue(bs)+1;
    //h->sps.mb_height
    i_mb_height=eg_read_ue(bs)+1;
    m_nframe_mbs_only_flag = eg_read_direct1(bs);
    if (!m_nframe_mbs_only_flag)
    {
          int  mb_aff=eg_read_direct1(bs);
    }
    direct_8x8_inference_flag=eg_read_direct1(bs);
    bcrop = eg_read_direct1(bs);
    if(bcrop)
    {
        crop_left = eg_read_ue(bs);
        crop_right = eg_read_ue(bs);
        crop_top = eg_read_ue(bs);
        crop_bottom = eg_read_ue(bs);

    }
    else
    {
        crop_left = 0;
        crop_right = 0;
        crop_top = 0;
        crop_bottom = 0;
    }
}
#define MIN(x,y) ((x) > (y)?(y):(x))
    nWidth = (i_mb_width << 4) - 2*MIN(crop_right,7);
    if(m_nframe_mbs_only_flag)
    nHeight = (i_mb_height << 4) - 2 * MIN(crop_bottom,7);
    else
        nHeight = (i_mb_height << 4) - 4 * MIN(crop_bottom,3);



    if (nWidth < 0)
    {
        nWidth = 0;
    }
    if (nHeight < 0)
    {
        nHeight = 0;
    }
    //printf("%d  %d \n",i_mb_width << 4 ,i_mb_height << 4);

    if (m_pRtpSessionClientFxn != NULL)
    {
        if (m_nWidth != nWidth ||
            m_nHeight != nHeight)
        {
            AntsFileHeader fh;
            memset(&fh,0,sizeof(fh));
            fh.uiFileStartId = ANTS_FILE_STARTCODE;
            fh.uiFrameRate = 0;
            fh.uiStreamType = 3;
            //printf("%d x %d -> %d x %d\n",m_nWidth,m_nHeight,nWidth,nHeight);
            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,0,AntsPktSysHeader,&fh,sizeof(fh),m_pRtpSessionClientUser);
        }

    }
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_nProfile_idc = profile_idc;
    m_nLevel_idc = level_idc;



    return 0;
}

int CH264RtpSession::H265_profile_tier_level(void *pbs,int profilePresentFlag,int maxNumSubLayersMinus1)
{
    Bitstream *bs = (Bitstream *)pbs;
    int i;
    int general_profile_idc;
    int general_profile_compatibility_flag[32];
    int general_level_idc;
    int sub_layer_profile_present_flag[8];
    int sub_layer_level_present_flag[8];
    if(profilePresentFlag)
    {
        eg_read_direct(bs,2);//general_profile_space
        eg_read_direct(bs,1);//general_tier_flag
        general_profile_idc = eg_read_direct(bs,5);//general_profile_idc
        for (i = 0; i < 32; i++)
        {
            general_profile_compatibility_flag[i] = eg_read_direct(bs,1);
        }
        eg_read_direct(bs,1);//general_progressive_source_flag
        eg_read_direct(bs,1);//general_interlaced_source_flag
        eg_read_direct(bs,1);//general_non_packed_constraint_flag
        eg_read_direct(bs,1);//general_frame_only_constraint_flag
        if( general_profile_idc == 4 ||
            general_profile_compatibility_flag[4] ||
            general_profile_idc == 5 ||
            general_profile_compatibility_flag[5] ||
            general_profile_idc == 6 ||
            general_profile_compatibility_flag[6] ||
            general_profile_idc == 7 ||
            general_profile_compatibility_flag[7])
        {
            eg_read_direct(bs,9);
             eg_read_direct(bs,16);// 34
              eg_read_direct(bs,18);
              //add by HG_Panda 文档上总共44个0
               eg_read_direct(bs,1);
        }
        else
        {
            eg_read_direct(bs,16);// 43
            eg_read_direct(bs,16);
            eg_read_direct(bs,11);
            if( ( general_profile_idc >= 1 && general_profile_idc <= 5 ) ||
                general_profile_compatibility_flag[ 1 ] ||
                general_profile_compatibility_flag[ 2 ] ||
                general_profile_compatibility_flag[ 3 ] ||
                general_profile_compatibility_flag[ 4 ] ||
                general_profile_compatibility_flag[ 5 ] )
            {
                 eg_read_direct(bs,1); // general_inbld_flag
            }
            else
            {
                eg_read_direct(bs,1);
            }
        }
    }

    general_level_idc = eg_read_direct(bs,8);


    //add by HG_Panda
    for(i = 0;i < maxNumSubLayersMinus1;i++)
    {
        sub_layer_profile_present_flag[i] = eg_read_direct(bs,1); //sub_layer_profile_present_flag[i]
        sub_layer_level_present_flag[i] = eg_read_direct(bs,1); //sub_layer_level_present_flag[i]
    }

    if(maxNumSubLayersMinus1 > 0){
        for(i = maxNumSubLayersMinus1;i < 8;i++)
        {
            eg_read_direct(bs,2); //reserverd_zero_2bits[i]
        }
    }

    for(i = 0;i < maxNumSubLayersMinus1;i++)
    {
        if(sub_layer_profile_present_flag[i]){
            eg_read_direct(bs,2); //sub_layer_profile_space
            eg_read_direct(bs,1); //sub_layer_tier_flag
            eg_read_direct(bs,5); //sub_layer_profile_idc

            for(int j = 0;j<32;j++){
                eg_read_direct(bs,1); //sub_layer_profile_compatibility_flag[i][j]
            }

            eg_read_direct(bs,1);  //sub_layer_progressive_source_flag[i]
            eg_read_direct(bs,1); //sub_layer_interlaced_source_flag
            eg_read_direct(bs,1); //sub_layer_non_packed_constraint_flag
            eg_read_direct(bs,1); //sub_layer_frame_only_constraint_flag
            eg_read_direct(bs,44);  //sub_layer_reserverd_zero_44bits
        }

        if(sub_layer_level_present_flag[i]){
            eg_read_direct(bs,8); //sub_layer_level_idc
        }
    }


    return 0;

}

int CH264RtpSession::decodeNal(void *pData,int Size,void **pOut)
{
    char *pOutBuf = new char[Size];
    int nOutSize = 0,i;
    uint8_t *pIn = (uint8_t *)pData,a,b;
    int nZeroCnt = 0;

    for (i = 0;i<Size;i++)
    {
        if (pIn[i] == 0)
        {
            nZeroCnt++;
        }
        else if (pIn[i] == 0x03 && nZeroCnt == 2)
        {
            nZeroCnt = 0;
            continue;
        }
        else
        {
            nZeroCnt = 0;
        }
        pOutBuf[nOutSize] = pIn[i];
        nOutSize++;


    }
    if (nOutSize == 0)
    {
        if (pOut)
        {
            *pOut = NULL;
            delete[] pOutBuf;
        }
        return 0;
    }
    if (pOut)
    {
        *pOut = pOutBuf;
    }
    return nOutSize;
}


int CH264RtpSession::H265_ReadSPS(void *pData,int Size)
{
    int i;//,list;
    int x,y;
    Bitstream tmpBs,*bs=&tmpBs;
    int mb_num,mb_big_num,mb_count;

    int i_mb_width,i_mb_height;
    int bcrop,crop_left,crop_right,crop_top,crop_bottom;

    int nWidth,nHeight;
    int sps_video_parameter_set_id,sps_max_sub_layers_minus1,sps_temporal_id_nesting_flag,sps_seq_parameter_set_id,
        chroma_format_idc,separate_colour_plane_flag,pic_width_in_luma_samples,pic_height_in_luma_samples,
        conformance_window_flag,conf_win_left_offset,conf_win_right_offset,conf_win_top_offset,conf_win_bottom_offset,
        sps_sub_layer_ordering_info_present_flag,log2_min_luma_coding_block_size_minus3,
        log2_diff_max_min_luma_coding_block_size,bit_depth_luma_minus8,bit_depth_chroma_minus8,log2_max_pic_order_cnt_lsb_minus4;
    int log2_ctb_size,ctb_width,ctb_height;
    int sps_max_dec_pic_buffering_minus1[8],sps_max_num_reorder_pics[8],sps_max_latency_increase_plus1[8];
    void *pNewBuf = NULL;
    int nNewSize;
    nNewSize = decodeNal(pData,Size,&pNewBuf);
    if (pNewBuf != NULL)
    {
        BitstreamInit(bs,pNewBuf,nNewSize);
    }
    else
    {
        BitstreamInit(bs,pData,Size);
    }


    sps_video_parameter_set_id = eg_read_direct(bs,4);
    sps_max_sub_layers_minus1 = eg_read_direct(bs,3);
    sps_temporal_id_nesting_flag = eg_read_direct(bs,1);
    H265_profile_tier_level(bs,1, sps_max_sub_layers_minus1 );
    sps_seq_parameter_set_id = eg_read_ue(bs);
    chroma_format_idc = eg_read_ue(bs);
    if (chroma_format_idc == 3)
    {
        separate_colour_plane_flag = eg_read_direct(bs,1);
    }
    pic_width_in_luma_samples = eg_read_ue(bs);
    pic_height_in_luma_samples = eg_read_ue(bs);
    conformance_window_flag = eg_read_direct(bs,1);
    if (conformance_window_flag)
    {
        conf_win_left_offset = eg_read_ue(bs) * 2;
        conf_win_right_offset = eg_read_ue(bs) * 2;
        conf_win_top_offset = eg_read_ue(bs) * 2;
        conf_win_bottom_offset = eg_read_ue(bs) * 2;
    }
    else
    {
        conf_win_left_offset = 0;
        conf_win_right_offset = 0;
        conf_win_top_offset = 0;
        conf_win_bottom_offset =0;

    }

    bit_depth_luma_minus8 = eg_read_ue(bs);
    bit_depth_chroma_minus8 = eg_read_ue(bs);
    log2_max_pic_order_cnt_lsb_minus4 = eg_read_ue(bs);
    sps_sub_layer_ordering_info_present_flag = eg_read_direct1(bs);
    for( i = ( sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1 ); i <= sps_max_sub_layers_minus1; i++ )
    {
        sps_max_dec_pic_buffering_minus1[ i ]=eg_read_ue(bs);
        sps_max_num_reorder_pics[ i ] =eg_read_ue(bs);
        sps_max_latency_increase_plus1[ i ]=eg_read_ue(bs);
    }
    log2_min_luma_coding_block_size_minus3 = eg_read_ue(bs) + 3;
    log2_diff_max_min_luma_coding_block_size = eg_read_direct(bs,3);

    log2_ctb_size     = log2_min_luma_coding_block_size_minus3 + log2_diff_max_min_luma_coding_block_size;
    ctb_width         = (pic_width_in_luma_samples  + (1 << log2_ctb_size) - 1) >> log2_ctb_size;
    ctb_height        = (pic_height_in_luma_samples + (1 << log2_ctb_size) - 1) >> log2_ctb_size;
    m_slice_address_length = ff_log2_c((ctb_width * ctb_height - 1) << 1);// log2((ctb_width * ctb_height - 1) << 1)

   nWidth = pic_width_in_luma_samples - (conf_win_left_offset + conf_win_right_offset);
   nHeight = pic_height_in_luma_samples - (conf_win_top_offset + conf_win_bottom_offset) ;

    //printf("%d  %d \n",i_mb_width << 4 ,i_mb_height << 4);
   // printf("%d x %d -> %d x %d  [%x %x %x %x] [%d]\n",pic_width_in_luma_samples,pic_height_in_luma_samples,sps_seq_parameter_set_id,chroma_format_idc ,((char*)pNewBuf)[12],((char*)pNewBuf)[13],((char*)pNewBuf)[14],((char*)pNewBuf)[15],sps_max_sub_layers_minus1);
    // printf("%d x %d -> %d x %d  [%p %d] [%d]\n",pic_width_in_luma_samples,pic_height_in_luma_samples,sps_seq_parameter_set_id,chroma_format_idc ,pNewBuf,nNewSize,sps_max_sub_layers_minus1);

     if (pNewBuf != NULL)
    {
        delete []pNewBuf;
        pNewBuf = NULL;
    }


    if (m_pRtpSessionClientFxn != NULL)
    {
        if (m_nWidth != nWidth ||
            m_nHeight != nHeight)
        {
            AntsFileHeader fh;
            memset(&fh,0,sizeof(fh));
            fh.uiFileStartId = ANTS_FILE_STARTCODE;
            fh.uiFrameRate = 0;
            fh.uiStreamType = 3;
            //printf("%d x %d -> %d x %d\n",m_nWidth,m_nHeight,nWidth,nHeight);
            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H265,0,AntsPktSysHeader,&fh,sizeof(fh),m_pRtpSessionClientUser);
        }

    }
    m_nWidth = nWidth;
    m_nHeight = nHeight;
   // m_nProfile_idc = profile_idc;
   // m_nLevel_idc = level_idc;
    return 0;
}

int CH264RtpSession::H265_ReadPPS(void *pData,int Size)
{
    int i;//,list;
    int x,y;
    Bitstream tmpBs,*bs=&tmpBs;
    int mb_num,mb_big_num,mb_count;

    int i_mb_width,i_mb_height;
    int bcrop,crop_left,crop_right,crop_top,crop_bottom;

    int nWidth,nHeight;
    int pps_pic_parameter_set_id,pps_seq_parameter_set_id,output_flag_present_flag;
    void *pNewBuf = NULL;
    int nNewSize;
    nNewSize = decodeNal(pData,Size,&pNewBuf);
    if (pNewBuf != NULL)
    {
        BitstreamInit(bs,pNewBuf,nNewSize);
    }
    else
    {
        BitstreamInit(bs,pData,Size);
    }


    pps_pic_parameter_set_id = eg_read_ue(bs);
    pps_seq_parameter_set_id = eg_read_ue(bs);
    m_dependent_slice_segments_enabled_flag = eg_read_direct1(bs);
    output_flag_present_flag = eg_read_direct1(bs);
    m_num_extra_slice_header_bits = eg_read_direct1(bs);


    if (pNewBuf != NULL)
    {
        delete []pNewBuf;
        pNewBuf = NULL;
    }


    return 0;
}

int CH264RtpSession::SendVideoPacket(int nFrameType,void *data,int len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)//毫秒
{
    uint32_t msw = 0,lsw = 0;
    int nStreamType;
    int nRet = -1;

    if((!m_bExternSocket) && 0 == GetDestinationCnt())
    {
        return -1;
    }
    if(DoCheckSend() > 0)
    {// 还有数据未发送完
        return -1;
    }
    //Ants_rtsp_GetNTPTime(&msw,&lsw);
    if((bTimeStampValid & 2) && m_bRecording)
    {
        Ants_rtsp_GetRTP2NTPTime(AbstimestampSec,AbstimestampUSec,&msw,&lsw);
        SetRecordNTP(msw,lsw);
    }

	m_totalPacketSize += len;
	m_totalPacketCount += 1;

    nStreamType = GetRTSPStreamType();
    if (nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_G711U ||
        nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_G711A ||
        nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_AAC)
    {
        nRet = SendAudioPacket(data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }
    else if (nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_APP)
    {
        nRet = SendAppPacket(data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }
    else if (nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG)
    {
        nRet = SendMJPEGPacket(data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }
    else if (nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB)
    {
       // nRet = SendAntsCombPacket(data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }
    else if (nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_PS)
    {// 暂时未实现
        nRet = -1;
    }
    else if(nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_H265)
    {
        if (bTimeStampValid & (1 << 31))
        {
            //
            nRet = SendH264RtpPacked(data,len);
        }
        nRet = SendH265Packet(nFrameType,data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }
    else if(nStreamType == ANTS_RTSP_CALLBACKBYPE_STREAM_H264)
    {
        if (bTimeStampValid & (1 << 31))
        {
            //
            nRet = SendH264RtpPacked(data,len);
        }
        nRet = SendH264Packet(nFrameType,data,len,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }
    // 最后检查是否有数据要发
    DoCheckSend();
    return nRet;
}

int CH264RtpSession::SendAntsCombPacket(int nType,int nChan,int nStreamIdx,void *data,int len)//毫秒
{
    int nRet = -1;
    int PackSize = GetAntsCombPacketMaxDataSize();
    int nPos;
    int bMark;
    int nSendSize;
    uint8_t *pPackBuf = NULL;
    if (data == NULL)
    {
        return nRet;
    }
    if(0 == GetDestinationCnt())
    {
        return -1;
    }

    pPackBuf = GetAntsCombPacketBuffer();
    if (pPackBuf == NULL)
    {
        return -1;
    }

    // printf("[%s] len = %d PackSize = %d type = %d nri = %d\n",__FUNCTION__,len,PackSize,pNALU->TYPE,pNALU->NRI);

    if (len <= PackSize)
    {
        return SendAntsCombPacketNoCopy(nType,nChan,nStreamIdx,(char *)data,len,1);
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
            //memcpy(pPackBuf,(char *)data + nPos,nSendSize);
            nRet = SendAntsCombPacketNoCopy(nType,nChan,nStreamIdx,(char *)data + nPos,nSendSize,bMark,1);

            nPos += nSendSize;


        }
        else
        {
            if (PackSize>= len - nPos)
            {//尾
                bMark = 1;
                nSendSize = len - nPos;
               // memcpy(pPackBuf,(char *)data + nPos,nSendSize);
                nRet = SendAntsCombPacketNoCopy(nType,nChan,nStreamIdx,(char *)data + nPos,nSendSize,bMark);
                nPos += nSendSize;



            }
            else
            {
                nSendSize = PackSize;
                //memcpy(pPackBuf,(char *)data + nPos,nSendSize);
                 nRet = SendAntsCombPacketNoCopy(nType,nChan,nStreamIdx,(char *)data + nPos,nSendSize,bMark);
                nPos += nSendSize;

            }

        }


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

int CH264RtpSession::SendH265Packet(int nFrameType,void *data,int len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{
    unsigned char *pNalStart,*pNalEnd,*pCurr;
    int NalLen;
    int nRet = -1;
    unsigned int timestampinc;

    uint32_t uStartCode = 0;
    uint32_t *p32;
    uint32_t msw = 0,lsw = 0;
    int nFps;


    int nmul ;
    if (data == NULL)
    {
        return nRet;
    }

    pCurr = (uint8_t *)data;
    if (!(pCurr[0] == 0x00 &&
        pCurr[1] == 0x00 &&
        ((pCurr[2] == 0x00 && pCurr[3] == 0x01) || (pCurr[2] == 0x01) )))
    {
        return -1;
    }

    nmul = 90;//CRtspServer::GetPayloadClockRate(m_nDefaultPayloadType) / 1000;
    nmul = GetPayloadClockRate() / 1000;
    nFps = GetFrameRate();
    if (nFps > 0)
    {
        m_nCurrStamp += nmul * 1000 / nFps;
    }
    else
    {


        if(m_bFirstFrame)
        {
            if (bTimeStampValid & 1)
            {
#if 0
                if (m_nLastStamp == 0 || Reltimestamp < m_nLastStamp)
                {
                    if (Reltimestamp < m_nLastStamp)
                    {
                        timestampinc = (Reltimestamp + 0xFFFFFFFF - m_nLastStamp)/90;
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
                    timestampinc = (Reltimestamp - m_nLastStamp)/90 * nmul;
                }
#else
                m_nCurrStamp = Reltimestamp;
#endif

            }
            else
            {
                int64_t timestamp;
                timestamp = AbstimestampSec * 1000;
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
        }
        else
        {
            m_bFirstFrame = 1;
            if (bTimeStampValid & 1)
            {
                m_nCurrStamp = Reltimestamp;
            }
            else
            {
                m_nCurrStamp = 0;
            }

            m_nFirstStamp = AbstimestampSec * 1000;
            m_nFirstStamp += AbstimestampUSec / 1000;
            m_dwFirstStamp = Reltimestamp;
        }
    }


    if (nFrameType == ANTS_RTSPSERVER_FRAMETYPE_IFRAME)
    {
        SetRecordExHeader_C(1);
        SetRecordExHeader_D(1);

    }
    else
    {
        SetRecordExHeader_C(0);
        SetRecordExHeader_D(0);
    }
    m_nFrameType = nFrameType;
    m_dwFrameCnt++;
    //分片..
    // printf("video bRelTimeStam = %d stamp = %u\n",bRelTimeStamp,m_nCurrStamp);
    pNalStart = NULL;
    pNalEnd = NULL;
    while(1)
    {
        if (pCurr - (uint8_t *)data >= len)
        {
            if (pNalStart != NULL)
            {
                pNalEnd = (uint8_t *)data + len;

                NalLen = pNalEnd - pNalStart;
                if (NalLen <= 4)
                {
                    break;
                }
                if (HEVC_NALU_GET_TYPE(pNalStart[0]) == HEVC_NAL_SPS)
                {// SPS

                   // SetSPS((char *)pNalStart,NalLen);
                    m_bVideoReady = 1;

                }
                else  if (HEVC_NALU_GET_TYPE(pNalStart[0]) == HEVC_NAL_PPS)
                {// SPS
                    //SetPPS((char *)pNalStart,NalLen);
                    m_bVideoReady = 1;
                }
                //完整一帧
                //处理一帧
                nRet = H265_SplitPacket(pNalStart,NalLen,m_nCurrStamp);
                if (nRet)
                {
                    break;
                }
            }
            break;
        }
        uStartCode >>= 8;
        uStartCode |= (*pCurr) << 24;
        if (uStartCode == RTSP_STARTCODE_H264)
        {

            if (pNalStart == NULL)
            {
                NalLen = 0;
                pNalStart = pCurr + 1;
                if (HEVC_NALU_GET_TYPE(pNalStart[0]) >= HEVC_NAL_SLICE_TRAIL_N &&
                    HEVC_NALU_GET_TYPE(pNalStart[0]) <= HEVC_NAL_SLICE_RSV_IRAP_VCL23)
                {
                    pCurr = (uint8_t *)data +  len;
                    continue;
                }
            }
            else if (pNalEnd == NULL)
            {
                pNalEnd = pCurr - 3;
                //完整一帧
                NalLen = pNalEnd - pNalStart;
                if (HEVC_NALU_GET_TYPE(pNalStart[0]) == HEVC_NAL_SPS)
                {// SPS

                    //SetSPS((char *)pNalStart,NalLen);
                    m_bVideoReady = 1;

                }
                else  if (HEVC_NALU_GET_TYPE(pNalStart[0]) == HEVC_NAL_PPS)
                {// SPS
                    //SetPPS((char *)pNalStart,NalLen);
                    m_bVideoReady = 1;
                }

                //处理一帧
                nRet = H265_SplitPacket(pNalStart,NalLen,m_nCurrStamp);
                if (nRet)
                {
                    break;
                }
                //处理完
                pNalStart = pCurr + 1;
                pNalEnd = NULL;

            }

        }

        pCurr++;

    }


    return nRet;
}

#if 1
 int CH264RtpSession::SendH264Packet(int nFrameType,void *data,int len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
 {
     char *pNalStart,*pNalEnd,*pCurr;
     int NalLen;
     int nRet = -1;
     unsigned int timestampinc;

     uint32_t uStartCode = 0;
     uint32_t *p32;
     uint32_t msw = 0,lsw = 0;
     int nFps;


     int nmul ;
     if (data == NULL)
     {
         return nRet;
     }

      pCurr = (char *)data;
     if (!(pCurr[0] == 0x00 &&
         pCurr[1] == 0x00 &&
         ((pCurr[2] == 0x00 && pCurr[3] == 0x01) || (pCurr[2] == 0x01) )))
     {
         return -1;
     }

     nmul = 90;//CRtspServer::GetPayloadClockRate(m_nDefaultPayloadType) / 1000;
     nmul = GetPayloadClockRate() / 1000;
     nFps = GetFrameRate();
     if (nFps > 0)
     {
         m_nCurrStamp += nmul * 1000 / nFps;
     }
     else
     {


         if(m_bFirstFrame)
         {
             if (bTimeStampValid & 1)
             {
    #if 0
                 if (m_nLastStamp == 0 || Reltimestamp < m_nLastStamp)
                 {
                     if (Reltimestamp < m_nLastStamp)
                     {
                         timestampinc = (Reltimestamp + 0xFFFFFFFF - m_nLastStamp)/90;
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
                     timestampinc = (Reltimestamp - m_nLastStamp)/90 * nmul;
                 }
    #else
                 m_nCurrStamp = Reltimestamp;
    #endif

             }
             else
             {
                 int64_t timestamp;
                 timestamp = AbstimestampSec * 1000;
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
         }
         else
         {
            m_bFirstFrame = 1;
             if (bTimeStampValid & 1)
             {
                m_nCurrStamp = Reltimestamp;
             }
             else
             {
                m_nCurrStamp = 0;
             }

            m_nFirstStamp = AbstimestampSec * 1000;
            m_nFirstStamp += AbstimestampUSec / 1000;
            m_dwFirstStamp = Reltimestamp;
         }
    }


     if (nFrameType == ANTS_RTSPSERVER_FRAMETYPE_IFRAME)
     {
         SetRecordExHeader_C(1);
         SetRecordExHeader_D(1);

     }
     else
     {
         SetRecordExHeader_C(0);
         SetRecordExHeader_D(0);
     }
     m_nFrameType = nFrameType;
     m_dwFrameCnt++;
  //分片..
   // printf("video bRelTimeStam = %d stamp = %u\n",bRelTimeStamp,m_nCurrStamp);
     pNalStart = NULL;
     pNalEnd = NULL;
     while(1)
     {
         if (pCurr - (char *)data >= len)
         {
             if (pNalStart != NULL)
             {
                 pNalEnd = (char *)data + len;

                 NalLen = pNalEnd - pNalStart;
                 if (NalLen <= 4)
                 {
                     break;
                 }
                 if ((pNalStart[0] & 31) == NAL_SPS)
                 {// SPS

                     SetSPS(pNalStart,NalLen);
                     m_bVideoReady = 1;

                 }
                 else  if ((pNalStart[0] & 31) == NAL_PPS)
                 {// SPS
                     SetPPS(pNalStart,NalLen);
                      m_bVideoReady = 1;
                 }
                 //完整一帧
                 //处理一帧
                 nRet = SplitPacket(pNalStart,NalLen,m_nCurrStamp);
                 if (nRet)
                 {
                     break;
                 }
             }
             break;
         }
         uStartCode >>= 8;
         uStartCode |= (*pCurr) << 24;
         if (uStartCode == RTSP_STARTCODE_H264)
         {
             if (pNalStart == NULL)
             {
                 NalLen = 0;
                 pNalStart = pCurr + 1;
                 if ((pNalStart[0] & 31) == NAL_SLICE ||
                     (pNalStart[0] & 31) == NAL_IDR_SLICE )
                 {
                     pCurr = (char *)data +  len;
                     continue;
                 }
             }
             else if (pNalEnd == NULL)
             {
                 pNalEnd = pCurr - 3;
                 //完整一帧
                 NalLen = pNalEnd - pNalStart;
                 if ((pNalStart[0] & 31) == NAL_SPS)
                 {// SPS

                      SetSPS(pNalStart,NalLen);
                      m_bVideoReady = 1;

                 }
                 else  if ((pNalStart[0] & 31) == NAL_PPS)
                 {// SPS
                      SetPPS(pNalStart,NalLen);
                      m_bVideoReady = 1;
                 }

                 //处理一帧
                 nRet = SplitPacket(pNalStart,NalLen,m_nCurrStamp);
                 if (nRet)
                 {
                     break;
                 }
                 //处理完
                 pNalStart = pCurr + 1;
                 pNalEnd = NULL;

             }

         }

        pCurr++;

     }


     return nRet;
 }
#else
int CH264RtpSession::SendH264Packet(const void *data,size_t len,int64_t timestamp,int bRelTimeStam)
{

    char *pNalStart,*pNalEnd,*pCurr;
    int NalLen;
    int nRet = -1;

    uint32_t timestampinc;
    uint32_t uStartCode = 0;


	int nmul ;
	nmul = CRtspServer::GetPayloadClockRate(m_nDefaultPayloadType) / 1000;

    if (m_nLastStamp == 0 || timestamp < m_nLastStamp)
    {

        timestampinc = 40 * nmul;
    }
    else
    {
        timestampinc = (timestamp - m_nLastStamp) * nmul;
    }

    m_nLastStamp = timestamp;


    //分片..
    pCurr = (char *)data;
    pNalStart = NULL;
    pNalEnd = NULL;
    while(1)
    {
        if (pCurr - (char *)data >= len)
        {
            if (pNalStart != NULL)
            {
                pNalEnd = (char *)data + len;

                NalLen = pNalEnd - pNalStart;
                if (NalLen <= 4)
                {
                    break;
                }
                //完整一帧
                //处理一帧
                nRet = SplitPacket(pNalStart,NalLen,timestampinc);
                if (nRet)
                {
                    break;
                }
            }
            break;
        }
        uStartCode = *((uint32_t *)pCurr);
        if (uStartCode == RTSP_STARTCODE_H264)
        {
            if (pNalStart == NULL)
            {
                NalLen = 0;
                pNalStart = pCurr + 4;
            }
            else if (pNalEnd == NULL)
            {
                pNalEnd = pCurr;
                //完整一帧
                NalLen = pNalEnd - pNalStart;

                //处理一帧
                nRet = SplitPacket(pNalStart,NalLen,timestampinc);
                if (nRet)
                {
                    break;
                }
                //处理完
                pNalStart = pCurr + 4;
                pNalEnd = NULL;

            }
            pCurr += 4;
        }
        else
        {
            pCurr++;
        }


    }


    return nRet;
}
#endif



void CH264RtpSession::SetClientCallback(void *hClient,ANTS_RTPSESSION_CLIENT_CALLBACK sessCallback,void *pUser)
{

    m_pRtpSessionClientHandle = hClient;
    m_pRtpSessionClientUser = pUser;
    m_pRtpSessionClientFxn = sessCallback;

}
void CH264RtpSession::SetRecvDataBuffer(void *pDataBuff,size_t nDataBuffSize)
{

    return;
//    m_pRecvDataBuff = (uint8_t *)pDataBuff;
 //   m_nRecvDataBuffSize = nDataBuffSize;
 //   m_nRecvDataStart = sizeof(AntsFrameHeader) + 4;
 //   m_nRecvDataPos = 0;
}


int CH264RtpSession::DealRecvAntsCombPacket(unsigned char *pRecvData,int nDataLen)
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
    RTPExtensionAntsCombHeader_T tExtAntsCombHeader;
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
         RTSP_ERROR("[%s.%d]invalid Video Payloadtype = %d  [%d]\n",__FUNCTION__,__LINE__,type,m_nDefaultPayloadType);
#if 0
         RTSP_ERROR("[%02x %02x %02x %02x | %02x %02x %02x %02x  | %02x %02x %02x %02x |%02x %02x %02x %02x ] \n",
             pRecvData[0],pRecvData[1],pRecvData[2],pRecvData[3],
             pRecvData[4],pRecvData[5],pRecvData[6],pRecvData[7],
             pRecvData[8],pRecvData[9],pRecvData[10],pRecvData[11],
             pRecvData[12],pRecvData[13],pRecvData[14],pRecvData[15]);
#endif
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
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
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
        RTPExtensionHeader tExtHeader;

        memcpy(&tExtHeader,pdata,4);
        if (tExtHeader.extid == htons(ANTS_RTSP_COMB_HEADER_MARK))
        {// AntsComb
            if (htons(tExtHeader.length) >= 4)
            {
                memcpy(&tExtAntsCombHeader,pdata + sizeof(RTPExtensionHeader),htons(tExtHeader.length) * 4);

            }

        }
        // pExtHeader = (struct RTPExtensionHeader *)(pdata);
        // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();

    }

    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        m_bLastRecvError = 1;
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);


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
            if (!tExtAntsCombHeader.byStart)
            {
                bSeqError = 1;
            }

        }
    }
    m_nLastSeq = nSeq;


    if (bSeqError)
    {

        m_bLastRecvError = 1;
        //RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
        return -1;
    }
    if(tExtAntsCombHeader.byStart)
    {
        // 新一包起始
        m_nRecvDataPos = 0;
    }


    m_nRecvDataStart = 0;


    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bLastRecvError = 1;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bLastRecvError = 1;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bLastRecvError = 1;
                    m_hMemLock.Unlock();
                    return -1;
                }
                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
                //free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();




    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataPos);

    memcpy(pDstData,pdata,npktlen);
    m_nRecvDataPos += npktlen;
    if (tExtAntsCombHeader.byEnd)
    {


        if (m_pRtpSessionClientFxn != NULL)
        {
            int prop = 0,bFree = 0;
            if(m_dwProp & 1)
            {
                prop = 1;
                bFree = 1;
            }
            prop |= (m_bRecording << 1) | (tExtAntsCombHeader.byStreamIdx << 8) | (tExtAntsCombHeader.wChan << 16);
            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB,prop,AntsPktAntsCombFrames,m_pRecvDataBuff,m_nRecvDataPos,m_pRtpSessionClientUser);
            if (bFree)
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

int CH264RtpSession::H265_ReadFrameHeader(char *pdata,int Size)
{
    unsigned int d32;
    int m,i,pos,info;
    int first_mb_in_slice,slice_type;
    int pic_parameter_set_id,colour_plane_id;
    int frame_num,field_pic_flag,bottom_field_flag;
    int IDR_pic_id;
    Bitstream bs;
    int first_slice_segment_in_pic_flag,no_output_of_prior_pics_flag,slice_pic_parameter_set_id,dependent_slice_segment_flag = 0;
    int slice_segment_address;
    void *pNewBuf = NULL;
    int nNewSize;
    int slice_reserved_flag[8];
    nNewSize = decodeNal(pdata,Size> 20?20:Size,&pNewBuf);
    if (pNewBuf != NULL)
    {
        BitstreamInit(&bs,pNewBuf,nNewSize);
    }
    else
    {
        BitstreamInit(&bs,pdata,Size);
    }
    first_slice_segment_in_pic_flag = eg_read_direct1(&bs);
    if( m_nCurrNalType >= HEVC_NAL_SLICE_BLA_W_LP && m_nCurrNalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23 )
    {
        no_output_of_prior_pics_flag = eg_read_direct1(&bs);
    }
    slice_pic_parameter_set_id = eg_read_ue(&bs);
    if( !first_slice_segment_in_pic_flag )
    {
        if( m_dependent_slice_segments_enabled_flag ) // PPS read
        {
            dependent_slice_segment_flag = eg_read_direct1(&bs);
        }
            slice_segment_address = eg_read_direct(&bs,m_slice_address_length); // slice_address_length -> SPS

    }
    if( !dependent_slice_segment_flag )
    {
        for( i = 0; i < m_num_extra_slice_header_bits; i++ ) // PPS
        {
            slice_reserved_flag[i] = eg_read_direct1(&bs);
        }

    //  HG_Panda  分片的解析不对 slice_segment_address 影响了 暂时跳过
        if(first_slice_segment_in_pic_flag){
            slice_type = eg_read_ue(&bs);
            //RTSP_DEBUG("H265_ReadFrameHeader %d [%x %x] [%d %d]\n",slice_type,pdata[0],pdata[1],m_slice_address_length,m_num_extra_slice_header_bits);
            m_nCurrSlicetype = slice_type;
        }

    }


    m_first_slice_segment_in_pic_flag = first_slice_segment_in_pic_flag;

   // RTSP_DEBUG("*frame_num = %d IDR_pic_id = %d first_mb_in_slice = %d first_slice_segment_in_pic_flag=%d\n",m_nframe_num,IDR_pic_id,first_mb_in_slice,first_slice_segment_in_pic_flag);
    if (pNewBuf != NULL)
    {
        delete []pNewBuf;
        pNewBuf = NULL;
    }
    return 0;
}

int CH264RtpSession::ReadH264FrameHeader(char *pdata,int Size)
{
    unsigned int d32;
    int m,i,pos,info;
    int first_mb_in_slice,slice_type;
    int pic_parameter_set_id,colour_plane_id;
    int frame_num,field_pic_flag,bottom_field_flag;
    int IDR_pic_id;
    Bitstream bs;
    BitstreamInit(&bs,pdata,Size);
    first_mb_in_slice = eg_read_ue(&bs);
    m_nLastfirst_mb_in_slice = m_nfirst_mb_in_slice;
    m_nfirst_mb_in_slice = first_mb_in_slice;
    slice_type = eg_read_ue(&bs);
    if (slice_type >= 5)
    {
        slice_type -= 5;
    }

    m_nCurrSlicetype = slice_type;
    pic_parameter_set_id = eg_read_ue(&bs);
    if (m_nresidual_color_transform_flag == 1)
    {
        colour_plane_id = eg_read_direct(&bs,2);
    }
    m_nframe_num = eg_read_direct(&bs,m_nlog2_max_frame_num);
    // printf("!!%d!!m_nlog2_max_frame_num = %d\n",m_nframe_num,m_nlog2_max_frame_num);


    if (!m_nframe_mbs_only_flag)
    {
        field_pic_flag = eg_read_direct1(&bs);
        if (field_pic_flag)
        {
            bottom_field_flag = eg_read_direct1(&bs);
        }
    }
    IDR_pic_id = 0;
    if (m_nCurrNalType == NAL_IDR_SLICE)
    {
        IDR_pic_id = eg_read_ue(&bs);
    }

    m_nCurrIDR_pic_id = IDR_pic_id;

    //printf("*frame_num = %d IDR_pic_id = %d first_mb_in_slice = %d\n",m_nframe_num,IDR_pic_id,first_mb_in_slice);

    return 0;
}

int CH264RtpSession::GetH264FrameType()
{


    return m_nCurrSlicetype;



}

unsigned int CH264RtpSession::AnalyticsFrameType(void *pData,int Size,int *pZeroLoad)
{
    int nPos = 0;
    int nZero = 0;
    int type = -1;
    unsigned int nRetValue = 0;
    int bNal = 0;

    unsigned char *pSrc,c;
    pSrc = (unsigned char *)pData;
    *pZeroLoad = 0;
    if (Size < 4)
    {

        bNal = 1;
    }
    else if(pSrc[0] == 0x00 &&
       pSrc[1] == 0x00 &&
       ((pSrc[2] == 0x00 &&
       pSrc[3] == 0x01) || pSrc[2] == 0x01) )
    {
       if (pSrc[2] == 0x01)
       {
           *pZeroLoad = 2;
       }
       else
       {
           *pZeroLoad = 3;
       }
    }
    else
    {
      bNal = 1;

    }
    for (nPos = 0; nPos < Size; nPos++)
    {
        c = pSrc[nPos];
        if (bNal)
        {
            type = NALU_GET_TYPE(c);
           // printf("type = %d\n",type);
            m_nCurrNalType = type;
            nRetValue |= (1 << type);
            if (type == NAL_SLICE ||
                type == NAL_IDR_SLICE)
            {// read frame header;
                if (type == NAL_IDR_SLICE)
                {
                    unsigned char *pDstData;
                    m_hMemLock.Lock();
                    if (!m_bRecvSPS && m_pSPS != NULL)
                    {
                        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                        memcpy(pDstData,m_pSPS,m_nSPSLen);
                        m_nRecvDataPos += m_nSPSLen;
                        m_bRecvSPS = 1;
                        ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
                    }
                    if (!m_bRecvPPS && m_pPPS != NULL)
                    {
                        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                        memcpy(pDstData,m_pPPS,m_nPPSLen);
                        m_nRecvDataPos += m_nPPSLen;
                        m_bRecvPPS = 1;
                    }
                    m_hMemLock.Unlock();
                }
                ReadH264FrameHeader((char *)pSrc + nPos + 1,Size - nPos - 1);
                //printf("m_nCurrSlicetype = %d\n",m_nCurrSlicetype);
                return nRetValue;
            }
            else if (type == NAL_SPS)
            {
                ReadSPS(pSrc + nPos + 1,Size - nPos - 1);
                m_bRecvSPS = 1;
            }
            else if (type == NAL_PPS)
            {
                m_bRecvPPS = 1;
            }

            bNal = 0;
        }

        if (c == 0)
        {
            nZero++;
            continue;
        }

        if (c == 0x01 && nZero >= 2)
        {
            bNal = 1;
        }
        nZero = 0;
    }
    return nRetValue;
}

unsigned int CH264RtpSession::H265AnalyticsFrameType(int nNalType,void *pData,int Size,int *pZeroLoad)
{
    int nPos = 0;
    int nZero = 0;
    int type = nNalType;
    unsigned int nRetValue = 0;
    int bNal = 0;

    unsigned char *pSrc,c;
    pSrc = (unsigned char *)pData;
    *pZeroLoad = 0;


        // printf("type = %d\n",type);
        m_nCurrNalType = nNalType;
        nRetValue |= (1 << type);
        if (type >= HEVC_NAL_SLICE_TRAIL_N &&
            type <= HEVC_NAL_SLICE_RSV_IRAP_VCL23)
        {// read frame header;
            // if (type >= HEVC_NAL_SLICE_BLA_W_LP && type <= HEVC_NAL_SLICE_CRA_NUT)
            // {
            //     unsigned char *pDstData;
            //     m_hMemLock.Lock();
            //     if (!m_bRecvSPS && m_pSPS != NULL)
            //     {
            //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
            //         memcpy(pDstData,m_pSPS,m_nSPSLen);
            //         m_nRecvDataPos += m_nSPSLen;
            //         m_bRecvSPS = 1;
            //         H265_ReadSPS(m_pSPS + 6,m_nSPSLen - 6);
            //     }
            //     if (!m_bRecvPPS && m_pPPS != NULL)
            //     {
            //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
            //         memcpy(pDstData,m_pPPS,m_nPPSLen);
            //         m_nRecvDataPos += m_nPPSLen;
            //         m_bRecvPPS = 1;
            //         H265_ReadPPS(m_pPPS + 6,m_nPPSLen - 6);
            //     }
            //     if (!m_bRecvVPS && m_pVPS != NULL)
            //     {
            //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
            //         memcpy(pDstData,m_pVPS,m_nVPSLen);
            //         m_nRecvDataPos += m_nVPSLen;
            //         m_bRecvVPS = 1;

            //     }
            //     m_hMemLock.Unlock();
            // }

             H265_ReadFrameHeader((char *)pData,Size);
            // printf("m_nCurrSlicetype = %d size=%d\n",m_nCurrSlicetype,Size);
            // return nRetValue;
        }
        else if (type == HEVC_NAL_SPS)
        {
            H265_ReadSPS(pData,Size);
            m_bRecvSPS = 1;
        }
        else if (type == HEVC_NAL_PPS)
        {
            H265_ReadPPS(pData,Size);
            m_bRecvPPS = 1;
        }
        else if (type == HEVC_NAL_VPS)
        {
            //vps 之前需要发数据
            m_bRecvVPS = 1;

            CallBackH265Packet(0);

        }


        bNal = 0;

    return nRetValue;
}


#if 1

int CH264RtpSession::CallBackH265Packet(int bAbs)
{
    AntsFrameHeader *pFrame;
    uint64_t Tmptimestamp;
    uint32_t dwSec = 0,dwUSec = 0;
    if(m_nRecvDataPos == 0 || m_nLastSlicetype == 0 ){
        return 0;
    }

    if(m_nLastNalType >= HEVC_NAL_VPS)
    {
        return 0;
    }

    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;
    memset(pFrame,0,sizeof(AntsFrameHeader));
    bAbs = 0;
    m_uiFrameNo++;
    pFrame->uiStartId = ANTS_FRAME_STARTCODE;
    pFrame->uiFrameType = H265_frametypeMap[m_nLastSlicetype];
    pFrame->uiFrameNo = m_uiFrameNo;
    if (bAbs)
    {
        pFrame->uiFrameTime = dwSec;
        pFrame->uiFrameTickCount = dwUSec;
    }
    else
    {
        Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
        pFrame->uiFrameTime = Tmptimestamp;
        Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
        pFrame->uiFrameTickCount = Tmptimestamp;
    }

    pFrame->uiFrameLen = m_nRecvDataPos;
    Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
    pFrame->uiTimeStamp = Tmptimestamp;
    pFrame->uMedia.struVideoHeader.cCodecId = AntsH265;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
    pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
    pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
    RTSP_DEBUG("[RTSP]%d m_uiFrameNo = %d %d %d  len=%d  nalType=%d  %d\n",__LINE__,m_uiFrameNo,m_nLastSlicetype,pFrame->uiFrameType,m_nRecvDataPos,m_nLastNalType,m_nWidth);

    if (m_pRtpSessionClientFxn != NULL)
    {
        int prop = 0;
        if(m_dwProp & 1)
            prop = 1;

        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H265,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
        if (prop)
        {
            m_pRecvDataBuff = NULL;
        }
    }

    m_nRecvDataPos = 0;


    return 0;
}

#endif

int CH264RtpSession::DealRecvH265Packet(unsigned char *pRecvData,int nDataLen)
{
    FU_INDICATOR *pFU;
    NALU_HEADER *pNALU;
    FU_HEADER *pFUHeader;
    unsigned char LayerID,TID,nalType,fuType;
    unsigned char forbidden_bit;
    int e,s;
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
    int bMark = 0;
    int version;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    uint32_t dwSec = 0,dwUSec = 0,bAbs = 0;
    uint32_t ssrc;

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
        RTSP_ERROR("[%s.%d]invalid Video Payloadtype = %d  [%d]\n",__FUNCTION__,__LINE__,type,m_nDefaultPayloadType);
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
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
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
    ssrc = (pRecvData[8] << 24) | (pRecvData[9] << 16) | (pRecvData[10] << 8) | pRecvData[11];
   // printf("len = %d  seq:%d\n",nDataLen,nSeq);
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
        RTPExtensionHeader tExtHeader;
        RTPExtensionOnvifHeader tExtOnvifHeader;
        memcpy(&tExtHeader,pdata,4);
        if (tExtHeader.extid == htons(0xABAC))
        {// onvif
            if (htons(tExtHeader.length) >= 3)
            {
                memcpy(&tExtOnvifHeader,pdata + sizeof(RTPExtensionHeader),htons(tExtHeader.length) * 4);
                bAbs = 1;
                Ants_rtsp_GetNTP2RTPTime(htonl(tExtOnvifHeader.ntpTimeStamp0),htonl(tExtOnvifHeader.ntpTimeStamp1),&dwSec,&dwUSec);
            }

        }
        // pExtHeader = (struct RTPExtensionHeader *)(pdata);
        // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();

    }

    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        m_bLastRecvError = 1;
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
        return -1;
    }
    //printf("curr = %d,last = %d\n",nSeq,m_nLastSeq);
    // printf("Recv Seq = %u mark = %d\n",nSeq,bMark);
    bSeqError = 0;
    if (m_nLastSeq != 0)
    {
        nReqSeq = m_nLastSeq + 1;
        if (nSeq != nReqSeq)
        {
            RTSP_ERROR("[RTSP]lost seq no. %d <= %d ssrc = %x\n",nSeq,m_nLastSeq,ssrc);
            bSeqError = 1;
        }
    }
    m_nLastSeq = nSeq;

    fuType = HEVC_NALU_GET_TYPE(pdata[0]);
     // printf("fuType = %d pos = %d\n",fuType,m_nRecvDataPos);
    forbidden_bit = HEVC_NALU_GET_F(pdata[0]);
    LayerID = HEVC_NALU_GET_LAYERID(pdata[0],pdata[1]);
    TID = HEVC_NALU_GET_TID(pdata[1]);

    m_bRecvSPS = 0;
    m_bRecvPPS = 0;
    m_bRecvVPS = 0;


    if (fuType == HEVC_FU_TYPE)
    {
        //s=1 e=0 第一个FU-A s=0 e=1 最后一个  s=0 e=0 中间的分片
        s = HEVC_FU_GET_S(pdata[2]);
        e = HEVC_FU_GET_E(pdata[2]);
        nalType = HEVC_FU_GET_TYPE(pdata[2]);

        //printf("s = %d, e = %d, r = %d len = %d nalType = %d \n",s,e,r,nDataLen,nalType);
#if 0
        if (s+e > 1)
        {
            //  m_bFrameDealing = 0;
            //  m_nRecvDataPos = 0;
            //   RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            //  return -1;
        }
#endif
        if (s)
        {


            // if (m_bFrameDealing != 0)
            // {
            //     m_nRecvDataPos = 0;
            // }
            // m_bFrameDealing = 1;

             unsigned int type;
        int nZeroLoad = 0;
        m_nLastNalType = m_nCurrNalType;
        nalType = HEVC_FU_GET_TYPE(pdata[2]);
        //printf("S nalType = %d\n",nalType);
        type = H265AnalyticsFrameType(nalType,pdata + 3,npktlen - 3,&nZeroLoad);

        if(m_first_slice_segment_in_pic_flag)
        {
            //第一个slice 意味着要把之前的数据发出去 不能是sps pps 这样的数据 一定要确定是视频数据
            CallBackH265Packet(bAbs);
            m_bFrameDealing = 0;
        }

        //开始了新一帧
        m_nCurrNalType = nalType;
        m_nRecvDataStart = sizeof(AntsFrameHeader);

        //  if(nalType == 19){
        //     m_nCurrSlicetype = 2;
        // }else if(nalType == 1){
        //     m_nCurrSlicetype = 1;
        // }else{
        //     printf("--------%d \n",nalType);
        // }

        if (bAbs)
        {
            pFrame->uiFrameTime = dwSec;
            pFrame->uiFrameTickCount = dwUSec;
        }



        }
        else if (bSeqError)
        {

            m_bLastRecvError = 1;
            RTSP_DEBUG("[rtp265]line = %d \n",__LINE__);
            return -1;
        }

        // if (m_bFrameDealing == 0)
        // {
        //     m_bLastRecvError = 1;
        //      RTSP_DEBUG("[rtp265]line = %d \n",__LINE__);
        //     return -1;
        // }
    }else{
        //不分片
#if 1
        unsigned int type;
        int nZeroLoad = 0;

        nalType = fuType;//FUHEADER_GET_TYPE(pdata[0]);


        m_nLastNalType = m_nCurrNalType;
        type = H265AnalyticsFrameType(nalType,pdata + 2,npktlen - 2,&nZeroLoad);
        m_nCurrNalType = nalType;
        //m_bFrameDealing = 0;

        if(m_first_slice_segment_in_pic_flag)
        {
            //第一个slice 意味着要把之前的数据发出去
            CallBackH265Packet(bAbs);
            m_bFrameDealing = 0;
        }


#endif

    }

    //根据实际大小重新分配内存
    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen + 8;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bLastRecvError = 1;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bLastRecvError = 1;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bLastRecvError = 1;
                    m_hMemLock.Unlock();
                    return -1;
                }

                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
                // free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();



    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;
    memset(pFrame,0,sizeof(AntsFrameHeader));
    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
    pNALU = (NALU_HEADER *)pDstData;

    if (fuType != HEVC_FU_TYPE)
    {// 不分片

    #if 0
        unsigned int type;
        int nZeroLoad = 0;

        nalType = fuType;//FUHEADER_GET_TYPE(pdata[0]);


        m_nLastNalType = m_nCurrNalType;
        type = H265AnalyticsFrameType(nalType,pdata + 2,npktlen - 2,&nZeroLoad);
         m_nCurrNalType = nalType;
        m_bFrameDealing = 0;

    #endif


        // if (m_nCurrNalType >= HEVC_NAL_SLICE_BLA_W_LP && m_nCurrNalType <= HEVC_NAL_SLICE_CRA_NUT)
        // {
        //     m_hMemLock.Lock();
        //     if (!m_bRecvSPS && m_pSPS != NULL)
        //     {
        //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //         memcpy(pDstData,m_pSPS,m_nSPSLen);
        //         m_nRecvDataPos += m_nSPSLen;
        //         m_bRecvSPS = 1;
        //        // H265_ReadSPS(m_pSPS + 6,m_nSPSLen - 6);
        //     }
        //     if (!m_bRecvPPS && m_pPPS != NULL)
        //     {
        //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //         memcpy(pDstData,m_pPPS,m_nPPSLen);
        //         m_nRecvDataPos += m_nPPSLen;
        //         m_bRecvPPS = 1;
        //        // H265_ReadPPS(m_pPPS + 6,m_nPPSLen - 6);
        //     }
        //     if (!m_bRecvVPS && m_pVPS != NULL)
        //     {
        //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //         memcpy(pDstData,m_pVPS,m_nVPSLen);
        //         m_nRecvDataPos += m_nVPSLen;
        //         m_bRecvVPS = 1;

        //     }
        //     m_hMemLock.Unlock();
        // }

        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);

        pDstData[0] = 0x00;
        pDstData[1] = 0x00;
        pDstData[2] = 0x00;
        pDstData[3] = 0x01;
        m_nRecvDataPos += 4;
        memcpy(pDstData + 4,pdata,npktlen);
        m_nRecvDataPos += npktlen;


        m_nLastTimestamp = nTimestamp;
        m_nLastSlicetype = m_nCurrSlicetype;
        m_nLastframe_num = m_nframe_num;
        m_nLastIDR_pic_id = m_nCurrIDR_pic_id;

        if (m_nCurrNalType >= HEVC_NAL_SLICE_BLA_W_LP && m_nCurrNalType <= HEVC_NAL_SLICE_CRA_NUT)
        {
            if (m_bRecvSPS + m_bRecvPPS + m_bRecvVPS!= 3)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
                m_bRecvVPS = 0;
                RTSP_DEBUG("[rtp265]line = %d m_bRecvSPS = %d m_bRecvPPS = %d\n",__LINE__,m_bRecvSPS,m_bRecvPPS);
                return -1;
            }
        }

#if 0


        if(m_nCurrNalType >= HEVC_NAL_SLICE_TRAIL_N && m_nCurrNalType <= HEVC_NAL_SLICE_CRA_NUT)
        {//I帧


            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                // m_bFrameDealing = 0;
                // m_nRecvDataPos = 0;

                RTSP_DEBUG("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
               // return -1;
            }
            if (bSeqError && Slicetype == HEVC_SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }
            if (m_nCurrNalType >= HEVC_NAL_SLICE_TRAIL_N && m_nCurrNalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23)
            {
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
                m_bRecvVPS = 0;
            }
            // m_nLastTimestamp = nTimestamp;
            // m_nLastSlicetype = m_nCurrSlicetype;
            // m_nLastframe_num = m_nframe_num;
            // m_nLastIDR_pic_id = m_nCurrIDR_pic_id;
            if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
            {
                RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
				//m_bFrameDealing = 0;
				//m_nRecvDataPos = 0;

				//return -1;
            }
            m_uiFrameNo++;
            pFrame->uiStartId = ANTS_FRAME_STARTCODE;
            pFrame->uiFrameType = H265_frametypeMap[m_nLastSlicetype];
            pFrame->uiFrameNo = m_uiFrameNo;
            RTSP_DEBUG("[RTSP]%d m_uiFrameNo = %d %d %d  len=%d\n",__LINE__,m_uiFrameNo,m_nLastSlicetype,pFrame->uiFrameType,m_nRecvDataPos);
            if (bAbs)
            {
                pFrame->uiFrameTime = dwSec;
                pFrame->uiFrameTickCount = dwUSec;
            }
            else
            {
                Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTime = Tmptimestamp;
                Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameTickCount = Tmptimestamp;
            }
            pFrame->uiFrameLen = m_nRecvDataPos;
            Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
            pFrame->uiTimeStamp = Tmptimestamp;
            pFrame->uMedia.struVideoHeader.cCodecId = AntsH265;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
            pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
            pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
            if (m_pRtpSessionClientFxn != NULL)
            {
                int prop = 0;
                if(m_dwProp & 1)
                    prop = 1;

                m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H265,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                if (prop)
                {
                    m_pRecvDataBuff = NULL;
                }
            }
            m_nRecvDataPos = 0;
            bFrame = 1;
            return bFrame;// Frame

        }

#endif

        return 0;

    }


    if (s)
    {//


        // if(m_first_slice_segment_in_pic_flag){
        //     if (m_nCurrNalType >= HEVC_NAL_SLICE_BLA_W_LP && m_nCurrNalType <= HEVC_NAL_SLICE_CRA_NUT)
        // {
        //     m_hMemLock.Lock();
        //     if (!m_bRecvSPS && m_pSPS != NULL)
        //     {
        //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //         memcpy(pDstData,m_pSPS,m_nSPSLen);
        //         m_nRecvDataPos += m_nSPSLen;
        //         m_bRecvSPS = 1;
        //        // H265_ReadSPS(m_pSPS + 6,m_nSPSLen - 6);
        //        printf("H265_ReadSPS",__FUNCTION__,__LINE__);
        //     }
        //     if (!m_bRecvPPS && m_pPPS != NULL)
        //     {
        //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //         memcpy(pDstData,m_pPPS,m_nPPSLen);
        //         m_nRecvDataPos += m_nPPSLen;
        //         m_bRecvPPS = 1;
        //        // H265_ReadPPS(m_pPPS + 6,m_nPPSLen - 6);
        //     }
        //     if (!m_bRecvVPS && m_pVPS != NULL)
        //     {
        //         pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //         memcpy(pDstData,m_pVPS,m_nVPSLen);
        //         m_nRecvDataPos += m_nVPSLen;
        //         m_bRecvVPS = 1;

        //     }
        //     m_hMemLock.Unlock();
        // }

        //     if (m_nCurrNalType >= HEVC_NAL_SLICE_BLA_W_LP && m_nCurrNalType <= HEVC_NAL_SLICE_CRA_NUT)
        // {
        //     if (m_bRecvSPS + m_bRecvPPS != 2)
        //     {
        //         m_bFrameDealing = 0;
        //         m_nRecvDataPos = 0;
        //         m_bRecvSPS = 0;
        //         m_bRecvPPS = 0;
        //         m_bRecvVPS = 0;
        //         RTSP_DEBUG("[rtp264]line = %d m_bRecvSPS = %d m_bRecvPPS = %d\n",__LINE__,m_bRecvSPS,m_bRecvPPS);
        //         return -1;
        //     }
        // }


        // }





        if (m_nCurrNalType >= HEVC_NAL_SLICE_TRAIL_N && m_nCurrNalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23)
        {
            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 2)
            {
                //m_bFrameDealing = 0;
                //m_nRecvDataPos = 0;
                RTSP_DEBUG("[rtp265]line = %d %d %d %d\n",__LINE__,Slicetype,m_nCurrNalType,bSeqError);
                //return -1;
            }

            if (bSeqError && m_nCurrSlicetype == HEVC_SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp265]line = %d \n",__LINE__);
                return -1;
            }

        }

        if (m_nCurrNalType >= HEVC_NAL_SLICE_TRAIL_N && m_nCurrNalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23)
        {
            m_bRecvSPS = 0;
            m_bRecvPPS = 0;
            m_bRecvVPS = 0;
        }
        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        // if (0 != nalType)
        {
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
        }
        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);



        //  RTSP_DEBUG("[rtp264]start line = %d clicetype = %d naltype = %d NRI = %d\n",__LINE__,m_nCurrSlicetype,nalType,NRI);
        HEVC_NALU_RESET(pDstData[0],pDstData[1]);
        HEVC_NALU_SET_F(pDstData[0],forbidden_bit);
        HEVC_NALU_SET_TYPE(pDstData[0],nalType);
        HEVC_NALU_SET_LAYERID(pDstData[0],pDstData[1],LayerID);
        HEVC_NALU_SET_TID(pDstData[1],TID);
        m_nRecvDataPos += 2;
        memcpy(pDstData+2,pdata + 3,npktlen - 3);
        m_nRecvDataPos += npktlen - 3;

    }
    if (e)
    {

        //最后一帧完成
        if(!s)
        {
            memcpy(pDstData,pdata + 3,npktlen - 3);
            m_nRecvDataPos += npktlen -3;
        }

        m_bFrameDealing = 0;
        if (m_nCurrNalType >= HEVC_NAL_SLICE_TRAIL_N && m_nCurrNalType <= HEVC_NAL_SLICE_RSV_IRAP_VCL23)
        {
            // memset(pFrame,0,sizeof(AntsFrameHeader));
            // m_uiLastFrameNo = m_uiFrameNo;
            if (m_nCurrSlicetype <0 || m_nCurrSlicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                RTSP_DEBUG("[rtp265]line = %d \n",__LINE__);
                return -1;
            }


            m_nLastTimestamp = nTimestamp;
            m_nLastSlicetype = m_nCurrSlicetype;
            m_nLastframe_num = m_nframe_num;
            m_nLastIDR_pic_id = m_nCurrIDR_pic_id;
#if 0
            if(m_nLastSlicetype < 0 || m_nLastSlicetype > 2)
            {
                RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
				m_bFrameDealing = 0;
				m_nRecvDataPos = 0;

				return -1;
            }
            m_uiFrameNo++;
            pFrame->uiStartId = ANTS_FRAME_STARTCODE;
            pFrame->uiFrameType = H265_frametypeMap[m_nLastSlicetype];
            pFrame->uiFrameNo = m_uiFrameNo;
             RTSP_DEBUG("[RTSP]%d m_uiFrameNo = %d %d %d  len=%d\n",__LINE__,m_uiFrameNo,m_nLastSlicetype,pFrame->uiFrameType,m_nRecvDataPos);
            if (bAbs)
            {
                pFrame->uiFrameTime = dwSec;
                pFrame->uiFrameTickCount = dwUSec;
            }
            else
            {
                Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTime = Tmptimestamp;
                Tmptimestamp = (m_nLastTimestamp * 1000* 1000.0 / GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameTickCount = Tmptimestamp;
            }
            //printf("%d %d - %d\n",m_nLastTimestamp,pFrame->uiFrameTime,pFrame->uiFrameTickCount);
            pFrame->uiFrameLen = m_nRecvDataPos;
            Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
            pFrame->uiTimeStamp = Tmptimestamp;

            pFrame->uMedia.struVideoHeader.cCodecId = AntsH265;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
            pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
            pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;

            if (m_pRtpSessionClientFxn != NULL)
            {
                int prop = 0;
                if(m_dwProp & 1)
                    prop = 1;

                m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H265,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                if (prop)
                {
                    m_pRecvDataBuff = NULL;
                }
            }
            m_nRecvDataPos = 0;
            bFrame = 1;

            return bFrame;
#endif
        }

        m_bFrameDealing = 0;
    }
    else if(!(s+e))
    {
        memcpy(pDstData,pdata + 3,npktlen - 3);
        m_nRecvDataPos += npktlen -3;

    }


    return 0;

}

 int CH264RtpSession::SplitAppPacket(const void *data,size_t len,uint32_t timestamp)
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

int CH264RtpSession::SendAppPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
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
    nRet = SplitAppPacket(data,len,m_nCurrStamp);

    return nRet;
}

int CH264RtpSession::DealRecvAppPacket(unsigned char *pRecvData,int nDataLen)
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
        pFrame->uMedia.struAppHeader.byAppPayloadType  = m_nDefaultPayloadType;

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

int CH264RtpSession::SendAudioPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
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

    //printf("m_bDefaultMark = %d %d\n",m_bDefaultMark,timestampinc);
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
        if (m_nTranType == 1 && m_bExternSocket)
        {
            nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)data + nPos,PackSize,NULL,0,bMark,m_nCurrStamp + nPos);
        }
        else
        {
            nRet = SendPacket((uint8_t *)data + nPos,PackSize,bMark,m_nCurrStamp + nPos);
        }
        nPos += PackSize;
    }




    return nRet;
}

int CH264RtpSession::DealRecvAudioPacket_G711(unsigned char *pRecvData,int len)
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
//RTSP_ERROR("[%s.%d] nSeq = %d nType = %d (%d,%d)\n",__FUNCTION__,__LINE__,nSeq,nType,m_nDefaultPayloadType,m_nSecondPayloadType);
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

		if (nType == m_nDefaultPayloadType)
		{
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
					if (nType == m_nDefaultPayloadType)
					{
						pheader->uiFrameType = AntsPktAudioFrames;
						pheader->uMedia.struAudioHeader.cCodecId = GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U? ANTS_RTSP_G711U:ANTS_RTSP_G711A;
						pheader->uMedia.struAudioHeader.cChannels = 1;
						pheader->uMedia.struAudioHeader.cSampleRate = 8;
						pheader->uMedia.struAudioHeader.cBitRate = 16;
						pheader->uMedia.struAudioHeader.cResolution = 0;
					}
					else if (nType == m_nSecondPayloadType)
					{
						pheader->uiFrameType = AntsPktAppFrames;
						pheader->uMedia.struAppHeader.byAppPayloadType = m_nSecondPayloadType;
					}

					pheader->uiFrameNo = m_uiFrameNo++;
					if (bAbs)
					{
						pheader->uiFrameTime = dwSec;
						pheader->uiFrameTickCount = dwUSec;
					}
					else
					{
						Tmptimestamp = m_nCurrStamp_recv / GetPayloadClockRate() ;
						pheader->uiFrameTime = Tmptimestamp;
						Tmptimestamp = (m_nCurrStamp_recv * 1000.0 * 1000/ GetPayloadClockRate()  - pheader->uiFrameTime * 1000* 1000);
						pheader->uiFrameTickCount = Tmptimestamp;
					}

					Tmptimestamp = m_nCurrStamp_recv * 8000.0/GetPayloadClockRate();
					if (nType == m_nDefaultPayloadType)
					{
						m_nCurrStamp_recv += 320;
						 pheader->uiFrameLen = 320;
					}
					else
					{
						pheader->uiFrameLen = nDatalen - nReadPos;
					}
					pheader->uiTimeStamp = Tmptimestamp;


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
		else if (nType == m_nSecondPayloadType)
		{
			 // 如果有音频，则回调

			if(m_nRecvDataPos > 0)
			{
				// 回调
				int prop = 0;
				if(m_dwProp & 1)
					prop = 1;
				m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,
					ANTS_RTSP_CALLBACKBYPE_STREAM,
					// nType == 0?ANTS_RTSP_CALLBACKBYPE_STREAM_G711U:ANTS_RTSP_CALLBACKBYPE_STREAM_G711A,
					GetRTSPStreamType(),prop,
					AntsPktAudioFrames,m_pRecvDataBuff,m_nRecvDataPos,m_pRtpSessionClientUser);
				m_nRecvDataPos = 0;
				m_nFrameCnt = 0;
				if (prop)
				{
					m_pRecvDataBuff = NULL;

				}
			}
			if(m_pRecvDataBuff == NULL)
			{
				m_pRecvDataBuff = (unsigned char*)malloc(AUDIORTPSESSION_RECV_BUFFSIZE);
				m_nRecvDataPos = 0;
			}
			if(m_pRecvDataBuff != NULL)
			{
			 //
			    pheader = (AntsFrameHeader *)(m_pRecvDataBuff + m_nRecvDataPos);
				memset(pheader,0,sizeof(AntsFrameHeader));
				pheader->uiStartId = ANTS_FRAME_STARTCODE;
				pheader->uiFrameType = AntsPktDTMFFrames;
				pheader->uMedia.struDTMFHeader.byDTMFPayloadType = m_nSecondPayloadType;
				pheader->uiFrameNo = m_uiFrameNo_Sub++;
				if (bAbs)
				{
					pheader->uiFrameTime = dwSec;
					pheader->uiFrameTickCount = dwUSec;
				}
				else
				{
					Tmptimestamp = m_nCurrStamp_recv / GetPayloadClockRate() ;
					pheader->uiFrameTime = Tmptimestamp;
					Tmptimestamp = (m_nCurrStamp_recv * 1000.0 * 1000/ GetPayloadClockRate()  - pheader->uiFrameTime * 1000* 1000);
					pheader->uiFrameTickCount = Tmptimestamp;
				}

				Tmptimestamp = m_nCurrStamp_recv * 8000.0/GetPayloadClockRate();
				pheader->uiFrameLen = nDatalen - nReadPos;

				pheader->uiTimeStamp = Tmptimestamp;



				m_nRecvDataPos += sizeof(AntsFrameHeader);
				memcpy(m_pRecvDataBuff + m_nRecvDataPos,pData + nReadPos,nDatalen - nReadPos);
				m_nRecvDataPos += nDatalen - nReadPos;
				{
					// 回调
					int prop = 0;
					if(m_dwProp & 1)
						prop = 1;
					m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,
						ANTS_RTSP_CALLBACKBYPE_STREAM,
						// nType == 0?ANTS_RTSP_CALLBACKBYPE_STREAM_G711U:ANTS_RTSP_CALLBACKBYPE_STREAM_G711A,
						GetSecondStreamType(),prop,
						AntsPktDTMFFrames,m_pRecvDataBuff,m_nRecvDataPos,m_pRtpSessionClientUser);
					m_nRecvDataPos = 0;
					m_nFrameCnt = 0;
					if (prop)
					{
						m_pRecvDataBuff = NULL;
					}
				}
			}
		}

    }
    //printf(" last [%d]\n",m_nRecvDataPos);
    return 1;
}
int CH264RtpSession::DealRecvAudioPacket(unsigned char *pRecvData,int len)
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

    if (nType != m_nDefaultPayloadType && nType != m_nSecondPayloadType)
    {
        return -1;
    }

    // if (nType == 0 || nType == 8)
    if (GetRTSPStreamType() == ANTS_RTSP_STREAM_G711A ||
        GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U)
    {
        return DealRecvAudioPacket_G711(pRecvData,len);
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
	if(nType == m_nDefaultPayloadType)
	{
		pheader->uiFrameType = AntsPktAudioFrames;
		pheader->uMedia.struAudioHeader.cCodecId = GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U? ANTS_RTSP_G711U:ANTS_RTSP_G711A;
		pheader->uMedia.struAudioHeader.cChannels = 1;
		pheader->uMedia.struAudioHeader.cSampleRate = 8;
		pheader->uMedia.struAudioHeader.cBitRate = 16;
		pheader->uMedia.struAudioHeader.cResolution = 0;
	}
	else if (nType == m_nSecondPayloadType)
	{
		pheader->uiFrameType = AntsPktAppFrames;
		pheader->uMedia.struAppHeader.byAppPayloadType = m_nSecondPayloadType;
	}
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

int CH264RtpSession::DealRecvPacket(unsigned char *pRecvData,int nDataLen)
{
    FU_INDICATOR *pFU;
    NALU_HEADER *pNALU;
    FU_HEADER *pFUHeader;
    unsigned char nalType,fuType;
    unsigned char NRI;
    unsigned char forbidden_bit;
    int r,e,s;
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
    int bMark = 0;
    int version;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    uint32_t dwSec = 0,dwUSec = 0,bAbs = 0;
    uint32_t ssrc;
    //printf("len = %d\n",nDataLen);
    version = (pRecvData[0] >> 6)&0x3;
    if(version != 2)
    {
        return 0;
    }
    type = pRecvData[1] & 0x7F;
    bMark = (pRecvData[1] >> 7) & 1;
    if(type != m_nDefaultPayloadType && type != m_nSecondPayloadType)
    {
        m_nRecvDataPos = 0;
        m_bFrameDealing = 0;
        free(m_pRecvDataBuff);
        m_pRecvDataBuff = NULL;
        m_nRecvDataBuffSize = 0;
        // RTSP_ERROR("invalid Video Payloadtype = %d\n",type);
        RTSP_ERROR("[%s.%d]invalid Video Payloadtype = %d  [%d,%d]\n",__FUNCTION__,__LINE__,type,m_nDefaultPayloadType,m_nSecondPayloadType);
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
    if (GetRTSPStreamType() == ANTS_RTSP_STREAM_JPEG)
    {
        return DealRecvMJPEGPacket(pRecvData,nDataLen);
    }
    else if (GetRTSPStreamType() == ANTS_RTSP_STREAM_H265)
    {
        return DealRecvH265Packet(pRecvData,nDataLen);
    }
    else if (GetRTSPStreamType() == ANTS_RTSP_STREAM_AntsComb)
    {
        return DealRecvAntsCombPacket(pRecvData,nDataLen);
    }
    else if (GetRTSPStreamType() == ANTS_RTSP_STREAM_PS)
    {
        return DealRecvPSPacket(pRecvData,nDataLen);
    }
    else if (GetRTSPStreamType() == ANTS_RTSP_STREAM_APP)
    {
        return DealRecvAppPacket(pRecvData,nDataLen);
    }
    else if (GetRTSPStreamType() == ANTS_RTSP_STREAM_G711U ||
             GetRTSPStreamType() == ANTS_RTSP_STREAM_G711A)
    {
        return DealRecvAudioPacket(pRecvData,nDataLen);
    }

    if (m_bSpecial_Ex)
    {
        return DealRecvPacket_ex(pRecvData,nDataLen);
    }
    if (m_bSpecial_MSlices)
    {
      // printf("[MM]");

        return DealRecvPacket_mSlices(pRecvData,nDataLen);
    }

    m_hMemLock.Lock();
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
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
    ssrc = (pRecvData[8] << 24) | (pRecvData[9] << 16) | (pRecvData[10] << 8) | pRecvData[11];

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
        RTPExtensionHeader tExtHeader;
        RTPExtensionOnvifHeader tExtOnvifHeader;
        memcpy(&tExtHeader,pdata,4);
        if (tExtHeader.extid == htons(0xABAC))
        {// onvif
            if (htons(tExtHeader.length) >= 3)
            {
                memcpy(&tExtOnvifHeader,pdata + sizeof(RTPExtensionHeader),htons(tExtHeader.length) * 4);
                bAbs = 1;
                Ants_rtsp_GetNTP2RTPTime(htonl(tExtOnvifHeader.ntpTimeStamp0),htonl(tExtOnvifHeader.ntpTimeStamp1),&dwSec,&dwUSec);
            }

        }
       // pExtHeader = (struct RTPExtensionHeader *)(pdata);
       // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
       // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();

    }

    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        m_bLastRecvError = 1;
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
        return -1;
    }
    //printf("curr = %d,last = %d\n",nSeq,m_nLastSeq);
   // printf("Recv Seq = %u mark = %d\n",nSeq,bMark);
    bSeqError = 0;
    if (m_nLastSeq != 0 || m_nRecvDataPos != 0)
    {
		if (nSeq == m_nLastSeq)
		{
			RTSP_ERROR("[RTSP]Same package seq no. %d = %d ssrc = %x\n",nSeq,m_nLastSeq,ssrc);
			return 0;
		}
        nReqSeq = m_nLastSeq + 1;
        if (nSeq != nReqSeq)
        {
            RTSP_ERROR("[RTSP]lost seq no. %d <= %d ssrc = %x\n",nSeq,m_nLastSeq,ssrc);
            bSeqError = 1;
        }
    }
    m_nLastSeq = nSeq;

    fuType = FU_GET_TYPE(pdata[0]);
 //   printf("fuType = %d pos = %d\n",fuType,m_nRecvDataPos);
    forbidden_bit = FU_GET_F(pdata[0]);
    NRI = FU_GET_NRI(pdata[0]);

    if (fuType == FU_TYPE_H264)
    {
        s = FUHEADER_GET_S(pdata[1]);
        e = FUHEADER_GET_E(pdata[1]);
        r = FUHEADER_GET_R(pdata[1]);
        nalType = FUHEADER_GET_TYPE(pdata[1]);

        // printf("s = %d, e = %d, r = %d len = %d nalType = %d \n",s,e,r,nDataLen,nalType);
#if 0
        if (s+e > 1)
        {
          //  m_bFrameDealing = 0;
          //  m_nRecvDataPos = 0;
         //   RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
          //  return -1;
        }
#endif
        if (s)
        {


            if (m_bFrameDealing != 0)
            {
                m_nRecvDataPos = 0;
            }
            m_bFrameDealing = 1;

        }
        else if (bSeqError)
        {

            m_bLastRecvError = 1;
           RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (r)
        {
           m_bLastRecvError = 1;
            RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            return -1;

        }
        if (m_bFrameDealing == 0)
        {
            m_bLastRecvError = 1;
            RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
    }


    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen + 8; // 避免overflow
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bLastRecvError = 1;
            m_hMemLock.Unlock();
            RTSP_ERROR("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bLastRecvError = 1;
                m_hMemLock.Unlock();
                RTSP_ERROR("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bLastRecvError = 1;
                    m_hMemLock.Unlock();
                    return -1;
                }

                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
               // free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();



    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;
    memset(pFrame,0,sizeof(AntsFrameHeader));
    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
    pNALU = (NALU_HEADER *)pDstData;

    if (fuType == STAP_A_H264)
    {
        int nSize = 0;
        int i,nleft;
        uint8_t fuType_sub;
        uint8_t *pSize8 = (uint8_t *)&nSize;
        i = 1;
        do
        {
            if (i >= npktlen)
            {
                break;
            }
            nSize = 0;
            pSize8[0] = pdata[i+1];
            pSize8[1] = pdata[i];
            nleft = npktlen - i - 2;
            if (nSize > nleft)
            {
                m_bLastRecvError = 1;
                RTSP_ERROR("error size %d > %d\n",nSize,nleft);
                return -1;
            }
            fuType_sub = FUHEADER_GET_TYPE(pdata[i + 2]);

            m_nLastNalType = m_nCurrNalType;
            m_nCurrNalType = fuType_sub;
            m_bFrameDealing = 0;
            //判断上一帧是否该送出
            if (fuType_sub == NAL_IDR_SLICE || fuType_sub == NAL_SLICE)
            {//读取当前头
                if (fuType_sub == NAL_IDR_SLICE)
                {
                    m_hMemLock.Lock();
                    if (!m_bRecvSPS && m_pSPS != NULL)
                    {
                        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                        memcpy(pDstData,m_pSPS,m_nSPSLen);
                        m_nRecvDataPos += m_nSPSLen;
                        m_bRecvSPS = 1;
                        ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
                    }
                    if (!m_bRecvPPS && m_pPPS != NULL)
                    {
                        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                        memcpy(pDstData,m_pPPS,m_nPPSLen);
                        m_nRecvDataPos += m_nPPSLen;
                        m_bRecvPPS = 1;
                    }
                    m_hMemLock.Unlock();
                }
                nRet = ReadH264FrameHeader((char *)&pdata[i + 3],nSize - 1);
                if (nRet < 0)
                {//错误,直接丢掉
                    if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                    {//回调


                    }
                    m_nRecvDataPos = 0;
                    m_nLastNalType = 0;
                    //m_bRecvPPS = 0;
                    //m_bRecvSPS = 0;
                    return bFrame;
                }
            }
            if ((m_nCurrNalType == m_nLastNalType) && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE) && m_nfirst_mb_in_slice > 0)
            {
                if ((m_nCurrNalType == NAL_IDR_SLICE && m_nCurrIDR_pic_id == m_nLastIDR_pic_id) ||
                    (m_nCurrNalType == NAL_SLICE && m_nLastframe_num == m_nframe_num))
                {
                    m_bSpecial_MSlices = 1;
                    m_nRecvDataPos = 0;
                    m_bRecvPPS = 0;
                    m_bRecvSPS = 0;
                    m_bRecvVPS = 0;
                    m_nLastNalType = 0;
                    m_nLastIDR_pic_id = 0;
                    m_nLastframe_num = 0;
                    m_bFrameDealing = 0;
                    m_uiFrameNo = 0;
                    return 0;
                }
            }


            if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
            {
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
                m_bRecvVPS = 0;
            }



            pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
            memcpy(pDstData + 4,&pdata[i + 2],nSize);
            m_nRecvDataPos += nSize;

            if(fuType_sub == NAL_IDR_SLICE || fuType_sub == NAL_SLICE)
            {//I帧


                if (m_bRecvSPS + m_bRecvPPS != 2)
                {

                    m_bLastRecvError = 1;
                     RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
                    return -1;
                }

                Slicetype = GetH264FrameType();
                if (Slicetype < 0 || Slicetype > 4)
                {
                    m_bLastRecvError = 1;
                    // m_bRecvPPS = 0;
                    // m_bRecvSPS = 0;
                    RTSP_ERROR("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
                    return -1;
                }
                if (bSeqError && Slicetype == SLICE_TYPE_P)
                {//丢包了，且当前包不为I帧，则也丢掉
                    m_bLastRecvError = 1;
                    //m_bRecvPPS = 0;
                    //m_bRecvSPS = 0;
                    RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
                    return -1;
                }

                m_nLastTimestamp = nTimestamp;
                m_nLastSlicetype = m_nCurrSlicetype;
                m_nLastframe_num = m_nframe_num;
                m_nLastIDR_pic_id = m_nCurrIDR_pic_id;
                if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
                {
                    RTSP_ERROR("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
					m_bFrameDealing = 0;
					m_nRecvDataPos = 0;
					return -1;
                }
                m_uiFrameNo++;
                pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                pFrame->uiFrameNo = m_uiFrameNo;
                if (bAbs)
                {
                     pFrame->uiFrameTime = dwSec;
                     pFrame->uiFrameTickCount = dwUSec;
                }
                else
                {
                    Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate();
                    pFrame->uiFrameTime = Tmptimestamp;
                    Tmptimestamp = (m_nLastTimestamp * 1000 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                    pFrame->uiFrameTickCount = Tmptimestamp;
                }


                pFrame->uiFrameLen = m_nRecvDataPos;
                Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                pFrame->uiTimeStamp = Tmptimestamp;
                pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                if (m_pRtpSessionClientFxn != NULL)
                {
                    int prop = 0;
                    if(m_dwProp & 1)
                        prop = 1;
                    m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                    if (prop)
                    {
                            m_pRecvDataBuff = NULL;
                    }
                }
                m_nRecvDataPos = 0;
                bFrame = 1;



            }
            else if (fuType_sub == NAL_SPS)
            {
                m_bRecvSPS = 1;
                ReadSPS(&pdata[i + 3],nSize - 1);

                //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
            }
            else if (fuType_sub == NAL_PPS)
            {
                m_bRecvPPS = 1;
            }


            i += nSize + 2;

        }while(1);

        return 0;
    }
    else if (fuType != FU_TYPE_H264)
    {// 不分片
        unsigned int type;
        int nZeroLoad = 0;

        nalType = fuType;//FUHEADER_GET_TYPE(pdata[0]);

        //printf("naltype = %d no\n",nalType);
        m_nLastNalType = m_nCurrNalType;
        type = AnalyticsFrameType(pdata,npktlen,&nZeroLoad);
        m_bFrameDealing = 0;

        if ((m_nCurrNalType == m_nLastNalType) && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE) && m_nfirst_mb_in_slice > 0)
        {
            if ((m_nCurrNalType == NAL_IDR_SLICE && m_nCurrIDR_pic_id == m_nLastIDR_pic_id) ||
                (m_nCurrNalType == NAL_SLICE && m_nLastframe_num == m_nframe_num))
            {
                m_bSpecial_MSlices = 1;
                m_nRecvDataPos = 0;
                m_bRecvPPS = 0;
                m_bRecvSPS = 0;
                m_nLastNalType = 0;
                m_nLastIDR_pic_id = 0;
                m_nLastframe_num = 0;
                m_bFrameDealing = 0;
                m_uiFrameNo = 0;
                return 0;
            }
        }


        if (m_nCurrNalType == NAL_IDR_SLICE)
        {
            m_hMemLock.Lock();
            if (!m_bRecvSPS && m_pSPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pSPS,m_nSPSLen);
                m_nRecvDataPos += m_nSPSLen;
                m_bRecvSPS = 1;
                ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
            }
            if (!m_bRecvPPS && m_pPPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pPPS,m_nPPSLen);
                m_nRecvDataPos += m_nPPSLen;
                m_bRecvPPS = 1;
            }
            m_hMemLock.Unlock();
        }

        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);

        pDstData[0] = 0x00;
        pDstData[1] = 0x00;
        pDstData[2] = 0x00;
        pDstData[3] = 0x01;
        m_nRecvDataPos += 4;
        memcpy(pDstData + 4,pdata,npktlen);
        m_nRecvDataPos += npktlen;

        if (m_nCurrNalType == NAL_IDR_SLICE)
        {
            if (m_bRecvSPS + m_bRecvPPS != 2)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
                RTSP_ERROR("[rtp264]line = %d m_bRecvSPS = %d m_bRecvPPS = %d\n",__LINE__,m_bRecvSPS,m_bRecvPPS);
                return -1;
            }
        }




        if(m_nCurrNalType == NAL_IDR_SLICE || m_nCurrNalType == NAL_SLICE)
        {//I帧


            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;

                RTSP_ERROR("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
                return -1;
            }
            if (bSeqError && Slicetype == SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
                return -1;
            }
            if (m_nCurrNalType == NAL_IDR_SLICE || m_nCurrNalType == NAL_SLICE)
            {
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
            }
            m_nLastTimestamp = nTimestamp;
            m_nLastSlicetype = m_nCurrSlicetype;
            m_nLastframe_num = m_nframe_num;
            m_nLastIDR_pic_id = m_nCurrIDR_pic_id;
            if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
            {
                RTSP_ERROR("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
				m_bFrameDealing = 0;
				m_nRecvDataPos = 0;
				return -1;
            }
            m_uiFrameNo++;
            pFrame->uiStartId = ANTS_FRAME_STARTCODE;
            pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
            pFrame->uiFrameNo = m_uiFrameNo;
            if (bAbs)
            {
                pFrame->uiFrameTime = dwSec;
                pFrame->uiFrameTickCount = dwUSec;
            }
            else
            {
                Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTime = Tmptimestamp;
                Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameTickCount = Tmptimestamp;
            }
            pFrame->uiFrameLen = m_nRecvDataPos;
            Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
            pFrame->uiTimeStamp = Tmptimestamp;
            pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
            pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
            pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
            if (m_pRtpSessionClientFxn != NULL)
            {
                int prop = 0;
                if(m_dwProp & 1)
                    prop = 1;

                m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                if (prop)
                {
                    m_pRecvDataBuff = NULL;
                }
            }
            m_nRecvDataPos = 0;
            bFrame = 1;
            return bFrame;// Frame

        }


        return 0;

    }


    if (s)
    {//
        unsigned int type;
        int nZeroLoad = 0;
        m_nLastNalType = m_nCurrNalType;
        nalType = FUHEADER_GET_TYPE(pdata[1]);
       type = AnalyticsFrameType(pdata + 1,npktlen - 1,&nZeroLoad);
        //开始了新一帧

        m_nRecvDataStart = sizeof(AntsFrameHeader);

        if (bAbs)
        {
            pFrame->uiFrameTime = dwSec;
            pFrame->uiFrameTickCount = dwUSec;
        }

        //printf("pos = %d,m_nLastNalType = %d ,%d,id = %d %d [%d,%d]\n",m_nRecvDataPos,m_nLastNalType,m_nCurrNalType,m_nLastIDR_pic_id,m_nCurrIDR_pic_id,m_nLastframe_num,m_nframe_num);
        if ((m_nCurrNalType == m_nLastNalType) && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE) && m_nfirst_mb_in_slice > 0)
        {
            if ((m_nCurrNalType == NAL_IDR_SLICE && m_nCurrIDR_pic_id == m_nLastIDR_pic_id) ||
                (m_nCurrNalType == NAL_SLICE && m_nLastframe_num == m_nframe_num))
            {
                m_bSpecial_MSlices = 1;
                m_nRecvDataPos = 0;
                m_bRecvPPS = 0;
                m_bRecvSPS = 0;
                m_nLastNalType = 0;
                m_nLastIDR_pic_id = 0;
                m_nLastframe_num = 0;
                m_bFrameDealing = 0;
                m_uiFrameNo = 0;
                return 0;
            }
        }



        if (m_nCurrNalType == NAL_IDR_SLICE)
        {
            m_hMemLock.Lock();
            if (!m_bRecvSPS && m_pSPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pSPS,m_nSPSLen);
                m_nRecvDataPos += m_nSPSLen;
                m_bRecvSPS = 1;
                ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
            }
            if (!m_bRecvPPS && m_pPPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pPPS,m_nPPSLen);
                m_nRecvDataPos += m_nPPSLen;
                m_bRecvPPS = 1;
            }
            m_hMemLock.Unlock();
        }
        if (m_nCurrNalType == NAL_IDR_SLICE)
        {
            if (m_bRecvSPS + m_bRecvPPS != 2)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
                RTSP_ERROR("[rtp264]line = %d m_bRecvSPS = %d m_bRecvPPS = %d\n",__LINE__,m_bRecvSPS,m_bRecvPPS);
                return -1;
            }
        }
        if (m_nCurrNalType == NAL_SLICE || m_nCurrNalType == NAL_IDR_SLICE)
        {



            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
                return -1;
            }

            if (bSeqError && m_nCurrSlicetype == SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
                return -1;
            }




        }

        if (m_nCurrNalType == NAL_IDR_SLICE || m_nCurrNalType == NAL_SLICE)
        {
            m_bRecvSPS = 0;
            m_bRecvPPS = 0;
        }
        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
       // if (0 != nalType)
        {
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
        }
        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);







        //  RTSP_DEBUG("[rtp264]start line = %d clicetype = %d naltype = %d NRI = %d\n",__LINE__,m_nCurrSlicetype,nalType,NRI);
        NALU_RESET(pDstData[0]);
        NALU_SET_NRI(pDstData[0],NRI);
        NALU_SET_F(pDstData[0],forbidden_bit);
        NALU_SET_TYPE(pDstData[0],nalType);
        m_nRecvDataPos += 1;
        memcpy(pDstData+1,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen - 2;

    }
    if (e)
    {

        //最后一帧完成
        if(!s)
        {
            memcpy(pDstData,pdata + 2,npktlen - 2);
            m_nRecvDataPos += npktlen -2;
        }

        m_bFrameDealing = 0;
        if (m_nCurrNalType == NAL_SLICE || m_nCurrNalType == NAL_IDR_SLICE)
        {
           // memset(pFrame,0,sizeof(AntsFrameHeader));
            // m_uiLastFrameNo = m_uiFrameNo;
            if (m_nCurrSlicetype <0 || m_nCurrSlicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
                return -1;
            }


            m_nLastTimestamp = nTimestamp;
            m_nLastSlicetype = m_nCurrSlicetype;
            m_nLastframe_num = m_nframe_num;
            m_nLastIDR_pic_id = m_nCurrIDR_pic_id;
            if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
            {
                RTSP_ERROR("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
				m_bFrameDealing = 0;
				m_nRecvDataPos = 0;
				return -1;
            }
            m_uiFrameNo++;
            pFrame->uiStartId = ANTS_FRAME_STARTCODE;
            pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
            pFrame->uiFrameNo = m_uiFrameNo;
            if (bAbs)
            {
                pFrame->uiFrameTime = dwSec;
                pFrame->uiFrameTickCount = dwUSec;
            }
            else
            {
                Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTime = Tmptimestamp;
                Tmptimestamp = (m_nLastTimestamp * 1000* 1000.0 / GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameTickCount = Tmptimestamp;
            }
            //printf("%d %d - %d\n",m_nLastTimestamp,pFrame->uiFrameTime,pFrame->uiFrameTickCount);
            pFrame->uiFrameLen = m_nRecvDataPos;
             Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
             pFrame->uiTimeStamp = Tmptimestamp;

            pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
            pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
            pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;

            if (m_pRtpSessionClientFxn != NULL)
            {
                int prop = 0;
                if(m_dwProp & 1)
                    prop = 1;

                m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                if (prop)
                {
                    m_pRecvDataBuff = NULL;
                }
            }
            m_nRecvDataPos = 0;
            bFrame = 1;

            return bFrame;
        }
        m_bFrameDealing = 0;
    }
    else if(!(s+e))
    {
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;

    }


    return 0;

}


#if 0
int CH264RtpSession::DealRecvPacket1(char *pRecvData,int nDataLen)
{
    FU_INDICATOR *pFU;
    NALU_HEADER *pNALU;
    FU_HEADER *pFUHeader;
    unsigned char nalType,fuType;
    unsigned char NRI;
    unsigned char forbidden_bit;
    int r,e,s;
    char *pdata,*pDstData;
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

    type = pRecvData[1] & 0x7F;
    if(type != m_nDefaultPayloadType)
    {
       // RTSP_ERROR("invalid Video Payloadtype = %d\n",type);
        return 0;
    }
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
    if (m_bSpecial_Ex)
    {
        return DealRecvPacket_ex(pRecvData,nDataLen);
    }
    if (m_bSpecial_MSlices)
    {
        return DealRecvPacket_mSlices(pRecvData,nDataLen);
    }

    m_hMemLock.Lock();
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = (uint8_t *)new char [H264RTPSESSION_RECV_BUFFSIZE];
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
        }
    }
    if (m_pRecvDataBuff == NULL)
    {
        m_hMemLock.Unlock();
        RTSP_ERROR("Memory malloc failed\n");
        return -1;
    }
    m_hMemLock.Unlock();
    //    printf("%u\n",packet->GetTimestamp());
    nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
    nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
   //printf("time = %d \n",nTimestamp);

    cc = pRecvData[0] &0x0F;

    pdata = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    npktlen = nDataLen - 12 - cc * 4;//packet->GetPayloadLength();
    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
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

    fuType = FU_GET_TYPE(pdata[0]);
    printf("fuType = %d pos = %d\n",fuType,m_nRecvDataPos);
    forbidden_bit = FU_GET_F(pdata[0]);
    NRI = FU_GET_NRI(pdata[0]);

    if (fuType == FU_TYPE_H264)
    {
        s = FUHEADER_GET_S(pdata[1]);
        e = FUHEADER_GET_E(pdata[1]);
        r = FUHEADER_GET_R(pdata[1]);
        if(pdata[2 + 0] == 0x00 &&
           pdata[2 + 1] == 0x00 &&
           pdata[2 + 2] == 0x01)
        {
            b00_00_01 = 1;
            pdata += 4;
            npktlen -= 4;
            nalType = FUHEADER_GET_TYPE(pdata[1]);
            forbidden_bit = FU_GET_F(pdata[1]);
            NRI = FU_GET_NRI(pdata[1]);

        }
        else if(pdata[2 + 0] == 0x00 &&
                pdata[2 + 1] == 0x00 &&
                pdata[2 + 2] == 0x00 &&
                pdata[2 + 3] == 0x01)
        {
            b00_00_00_01 = 1;
            pdata += 5;
            npktlen -= 5;
            nalType = FUHEADER_GET_TYPE(pdata[1]);
            forbidden_bit = FU_GET_F(pdata[1]);
            NRI = FU_GET_NRI(pdata[1]);

        }
        else
        {
            nalType = FUHEADER_GET_TYPE(pdata[1]);
        }


        // printf("s = %d, e = %d, r = %d len = %d nalType = %d \n",s,e,r,nDataLen,nalType);
        if (s+e > 1)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (s)
        {
            if (nalType == NAL_SLICE ||
                nalType == NAL_IDR_SLICE)
            {
              //  m_uiFrameNo++;
            }

            if (m_bFrameDealing != 0)
            {
                m_nRecvDataPos = 0;
            }
            m_bFrameDealing = 1;

        }
        else if (bSeqError)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
             //RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (r)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;

        }
        if (m_bFrameDealing == 0)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
             // RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
    }


    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = new char [m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC];
                if (pTemp == NULL)
                {
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    m_hMemLock.Unlock();
                    return -1;
                }
                memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
                delete [] pDel;
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();



    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;

    pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
    pNALU = (NALU_HEADER *)pDstData;

    if (fuType == STAP_A_H264)
    {
        int nSize = 0;
        int i,nleft;
        uint8_t fuType_sub;
        uint8_t *pSize8 = (uint8_t *)&nSize;
        i = 1;
        do
        {
            if (i >= npktlen)
            {
                break;
            }
            nSize = 0;
            pSize8[0] = pdata[i+1];
            pSize8[1] = pdata[i];
            nleft = npktlen - i - 2;
            if (nSize > nleft)
            {
                printf("error size %d > %d\n",nSize,nleft);
                return -1;
            }
            fuType_sub = FUHEADER_GET_TYPE(pdata[i + 2]);

            m_nLastNalType = m_nCurrNalType;
            m_nCurrNalType = fuType_sub;
            m_bFrameDealing = 0;
            //判断上一帧是否该送出
            if (fuType_sub == NAL_IDR_SLICE || fuType_sub == NAL_SLICE)
            {//读取当前头
                nRet = ReadH264FrameHeader((char *)&pdata[i + 3],nSize - 1);
                if (nRet < 0)
                {//错误,直接丢掉
                    if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                    {//回调


                    }
                    m_nRecvDataPos = 0;
                    m_nLastNalType = 0;
                    //m_bRecvPPS = 0;
                    //m_bRecvSPS = 0;
                    return bFrame;
                }
            }
            if ((m_nCurrNalType == m_nLastNalType) && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE) && m_nfirst_mb_in_slice > 0)
            {
                if ((m_nCurrNalType == NAL_IDR_SLICE && m_nCurrIDR_pic_id == m_nLastIDR_pic_id) ||
                    (m_nCurrNalType == NAL_SLICE && m_nLastframe_num == m_nframe_num))
                {
                      m_bSpecial_MSlices = 1;
                      m_nRecvDataPos = 0;
                      m_bRecvPPS = 0;
                      m_bRecvSPS = 0;
                      m_nLastNalType = 0;
                      m_nLastIDR_pic_id = 0;
                      m_nLastframe_num = 0;
                      m_bFrameDealing = 0;
                      m_uiFrameNo = 0;
                      return 0;
                }
            }


            if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
            {
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
            }

            if (fuType_sub == NAL_IDR_SLICE)
            {
                m_hMemLock.Lock();
                if (!m_bRecvSPS && m_pSPS != NULL)
                {
                    pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                    memcpy(pDstData,m_pSPS,m_nSPSLen);
                    m_nRecvDataPos += m_nSPSLen;
                    m_bRecvSPS = 1;
                    ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
                }
                if (!m_bRecvPPS && m_pPPS != NULL)
                {
                    pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                    memcpy(pDstData,m_pPPS,m_nPPSLen);
                    m_nRecvDataPos += m_nPPSLen;
                    m_bRecvPPS = 1;
                }
                m_hMemLock.Unlock();
            }

            pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
            memcpy(pDstData + 4,&pdata[i + 2],nSize);
            m_nRecvDataPos += nSize;

            if(fuType_sub == NAL_IDR_SLICE || fuType_sub == NAL_SLICE)
            {//I帧


                if (m_bRecvSPS + m_bRecvPPS != 2)
                {

                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    // RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                    return -1;
                }

                Slicetype = GetH264FrameType();
                if (Slicetype < 0 || Slicetype > 4)
                {
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    // m_bRecvPPS = 0;
                    // m_bRecvSPS = 0;
                    RTSP_DEBUG("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
                    return -1;
                }
                if (bSeqError && Slicetype == SLICE_TYPE_P)
                {//丢包了，且当前包不为I帧，则也丢掉
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    //m_bRecvPPS = 0;
                    //m_bRecvSPS = 0;
                    RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                    return -1;
                }

                m_nLastTimestamp = nTimestamp;
                m_nLastSlicetype = m_nCurrSlicetype;
                m_nLastframe_num = m_nframe_num;
                m_nLastIDR_pic_id = m_nCurrIDR_pic_id;

                m_uiFrameNo++;
                pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                pFrame->uiFrameNo = m_uiFrameNo;
                pFrame->uiFrameTime = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTickCount = (m_nLastTimestamp * 1000 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameLen = m_nRecvDataPos;
                pFrame->uiTimeStamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                if (m_pRtpSessionClientFxn != NULL)
                {
                    m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                }
                m_nRecvDataPos = 0;
                bFrame = 1;



            }
            else if (fuType_sub == NAL_SPS)
            {
                m_bRecvSPS = 1;
                ReadSPS(&pdata[i + 3],nSize - 1);

                //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
            }
            else if (fuType_sub == NAL_PPS)
            {
                m_bRecvPPS = 1;
            }


            i += nSize + 2;

        }while(1);

         return 0;
    }
    else if (fuType != FU_TYPE_H264)
    {// 不分片
        if(pdata[0] == 0x00 &&
            pdata[1] == 0x00 &&
            pdata[2] == 0x01)
        {
            b00_00_01 = 1;
            pdata += 3;
            npktlen -= 3;
            nalType = FUHEADER_GET_TYPE(pdata[0]);
            forbidden_bit = FU_GET_F(pdata[0]);
            NRI = FU_GET_NRI(pdata[0]);
        }
        else if(pdata[0] == 0x00 &&
            pdata[1] == 0x00 &&
            pdata[2] == 0x00 &&
            pdata[3] == 0x01)
        {
            b00_00_00_01 = 1;
            pdata += 4;
            npktlen -= 4;
            nalType = FUHEADER_GET_TYPE(pdata[0]);
            forbidden_bit = FU_GET_F(pdata[0]);
            NRI = FU_GET_NRI(pdata[0]);
        }
        else
        {
            nalType = fuType;//FUHEADER_GET_TYPE(pdata[0]);
        }

        //printf("naltype = %d no\n",nalType);
        m_nLastNalType = m_nCurrNalType;
        m_nCurrNalType = nalType;
        m_bFrameDealing = 0;
        //判断上一帧是否该送出
        if (nalType == NAL_IDR_SLICE || nalType == NAL_SLICE)
        {//读取当前头
            nRet = ReadH264FrameHeader((char *)&pdata[1],npktlen - 1);
            if (nRet < 0)
            {//错误,直接丢掉
                if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                {//回调


                }
                m_nRecvDataPos = 0;
                m_nLastNalType = 0;

                return bFrame;
            }
        }
        if ((m_nCurrNalType == m_nLastNalType) && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE) && m_nfirst_mb_in_slice > 0)
        {
            if ((m_nCurrNalType == NAL_IDR_SLICE && m_nCurrIDR_pic_id == m_nLastIDR_pic_id) ||
                (m_nCurrNalType == NAL_SLICE && m_nLastframe_num == m_nframe_num))
            {
                m_bSpecial_MSlices = 1;
                m_nRecvDataPos = 0;
                m_bRecvPPS = 0;
                m_bRecvSPS = 0;
                m_nLastNalType = 0;
                m_nLastIDR_pic_id = 0;
                m_nLastframe_num = 0;
                m_bFrameDealing = 0;
                m_uiFrameNo = 0;
                return 0;
            }
        }

        if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
        {
            m_bRecvSPS = 0;
            m_bRecvPPS = 0;
        }
        if (nalType == NAL_IDR_SLICE)
        {
            m_hMemLock.Lock();
            if (!m_bRecvSPS && m_pSPS != NULL)
            {
                pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pSPS,m_nSPSLen);
                m_nRecvDataPos += m_nSPSLen;
                m_bRecvSPS = 1;
                 ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
            }
            if (!m_bRecvPPS && m_pPPS != NULL)
            {
                pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pPPS,m_nPPSLen);
                m_nRecvDataPos += m_nPPSLen;
                m_bRecvPPS = 1;
            }
            m_hMemLock.Unlock();
        }

        pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);

            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
            memcpy(pDstData + 4,pdata,npktlen);
            m_nRecvDataPos += npktlen;







        if(nalType == NAL_IDR_SLICE || nalType == NAL_SLICE)
        {//I帧


            if (m_bRecvSPS + m_bRecvPPS != 2)
            {

                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
               // RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }



            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
               // m_bRecvPPS = 0;
               // m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
                return -1;
            }
            if (bSeqError && Slicetype == SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
               //m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }

            m_nLastTimestamp = nTimestamp;
             m_nLastSlicetype = m_nCurrSlicetype;
             m_nLastframe_num = m_nframe_num;
             m_nLastIDR_pic_id = m_nCurrIDR_pic_id;

             m_uiFrameNo++;
             pFrame->uiStartId = ANTS_FRAME_STARTCODE;
             pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
             pFrame->uiFrameNo = m_uiFrameNo;
             pFrame->uiFrameTime = m_nLastTimestamp / GetPayloadClockRate() ;
             pFrame->uiFrameTickCount = (m_nLastTimestamp * 1000 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
             pFrame->uiFrameLen = m_nRecvDataPos;
             pFrame->uiTimeStamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
             pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
             pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
             pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
             if (m_pRtpSessionClientFxn != NULL)
             {
                 m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
             }
             m_nRecvDataPos = 0;
             bFrame = 1;
            return bFrame;// Frame

        }
        else if (nalType == NAL_SPS)
        {
            m_bRecvSPS = 1;
            Profile_idc = pdata[1];
            Constraint_sets = pdata[2];
            Level_idc = pdata[3];
            ReadSPS(pdata+1,npktlen - 1);

            //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
        }
        else if (nalType == NAL_PPS)
        {
            m_bRecvPPS = 1;
        }

        return 0;

    }


    if (s)
    {
        //开始了新一帧
        m_nLastNalType = m_nCurrNalType;
        m_nCurrNalType = nalType;

        m_nRecvDataStart = sizeof(AntsFrameHeader);
        //判断上一帧是否该送出
        if (nalType == NAL_IDR_SLICE || nalType == NAL_SLICE)
        {//读取当前头
            nRet = ReadH264FrameHeader((char *)&pdata[2],npktlen - 2);
            if (nRet < 0)
            {//错误,直接丢掉
                printf("error \n");
                if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                {//回调


                }
                m_nRecvDataPos = 0;
                m_nLastNalType = 0;

                return bFrame;
            }
        }
        if (m_nCurrNalType == NAL_IDR_SLICE)
        {
            int ii = 0;
        }
       //printf("pos = %d,m_nLastNalType = %d ,%d,id = %d %d [%d,%d]\n",m_nRecvDataPos,m_nLastNalType,m_nCurrNalType,m_nLastIDR_pic_id,m_nCurrIDR_pic_id,m_nLastframe_num,m_nframe_num);
        if ((m_nCurrNalType == m_nLastNalType) && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE) && m_nfirst_mb_in_slice > 0)
        {
            if ((m_nCurrNalType == NAL_IDR_SLICE && m_nCurrIDR_pic_id == m_nLastIDR_pic_id) ||
                (m_nCurrNalType == NAL_SLICE && m_nLastframe_num == m_nframe_num))
            {
                m_bSpecial_MSlices = 1;
                m_nRecvDataPos = 0;
                m_bRecvPPS = 0;
                m_bRecvSPS = 0;
                m_nLastNalType = 0;
                m_nLastIDR_pic_id = 0;
                m_nLastframe_num = 0;
                m_bFrameDealing = 0;
                m_uiFrameNo = 0;
                return 0;
            }
        }

        if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
        {
            m_bRecvSPS = 0;
            m_bRecvPPS = 0;
        }

        if (nalType == NAL_IDR_SLICE)
        {
            m_hMemLock.Lock();
            if (!m_bRecvSPS && m_pSPS != NULL)
            {
                pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pSPS,m_nSPSLen);
                m_nRecvDataPos += m_nSPSLen;
                m_bRecvSPS = 1;
                ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
            }
            if (!m_bRecvPPS && m_pPPS != NULL)
            {
                pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pPPS,m_nPPSLen);
                m_nRecvDataPos += m_nPPSLen;
                m_bRecvPPS = 1;
            }
            m_hMemLock.Unlock();
        }

        pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        if (nalType != 0)
        {
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
        }
        pDstData = (char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);


        if (nalType == NAL_SLICE || nalType == NAL_IDR_SLICE)
        {


            if (m_bRecvSPS + m_bRecvPPS != 2)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //RTSP_DEBUG("[rtp264]line = %d m_bRecvSPS = %d m_bRecvPPS = %d\n",__LINE__,m_bRecvSPS,m_bRecvPPS);
                return -1;
            }
            //ReadH264FrameHeader((char *)&pdata[2],npktlen - 2);
            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }

            if (bSeqError && m_nCurrSlicetype == SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }




        }
        else if (nalType == NAL_SPS)
        {
            m_bRecvSPS = 1;
            Profile_idc = pdata[1];
            Constraint_sets = pdata[2];
            Level_idc = pdata[3];
            ReadSPS(pdata+2,npktlen - 2);
            m_nCurrNalType = nalType;
           // printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
        }
        else if (nalType == NAL_PPS)
        {
            m_bRecvPPS = 1;
            m_nCurrNalType = nalType;
        }
        else
        {
            m_nCurrNalType = nalType;
        }



        //  RTSP_DEBUG("[rtp264]start line = %d clicetype = %d naltype = %d NRI = %d\n",__LINE__,m_nCurrSlicetype,nalType,NRI);
        NALU_RESET(pDstData[0]);
        NALU_SET_NRI(pDstData[0],NRI);
        NALU_SET_F(pDstData[0],forbidden_bit);
        NALU_SET_TYPE(pDstData[0],nalType);
        m_nRecvDataPos += 1;
        memcpy(pDstData+1,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen - 2;

    }
    else if (e)
    {

        //最后一帧完成
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;
       m_bFrameDealing = 0;
        if (m_nCurrNalType == NAL_SLICE || m_nCurrNalType == NAL_IDR_SLICE)
        {
            memset(pFrame,0,sizeof(AntsFrameHeader));
           // m_uiLastFrameNo = m_uiFrameNo;
            if (m_nCurrSlicetype <0 || m_nCurrSlicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }


            m_nLastTimestamp = nTimestamp;
            m_nLastSlicetype = m_nCurrSlicetype;
            m_nLastframe_num = m_nframe_num;
            m_nLastIDR_pic_id = m_nCurrIDR_pic_id;

            m_uiFrameNo++;
            pFrame->uiStartId = ANTS_FRAME_STARTCODE;
            pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
            pFrame->uiFrameNo = m_uiFrameNo;
            pFrame->uiFrameTime = m_nLastTimestamp / GetPayloadClockRate() ;
            pFrame->uiFrameTickCount = (m_nLastTimestamp * 1000* 1000 / GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
            //printf("%d %d - %d\n",m_nLastTimestamp,pFrame->uiFrameTime,pFrame->uiFrameTickCount);
            pFrame->uiFrameLen = m_nRecvDataPos;
            pFrame->uiTimeStamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
            pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
            pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
            pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
            if (m_pRtpSessionClientFxn != NULL)
            {
                m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
            }
            m_nRecvDataPos = 0;
            bFrame = 1;

            return bFrame;
        }
        m_bFrameDealing = 0;
    }
    else
    {
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;

    }


    return 0;

}
#endif
int CH264RtpSession::DealRecvPacket_mSlices(unsigned char *pRecvData,int nDataLen)
{
    FU_INDICATOR *pFU;
    NALU_HEADER *pNALU;
    FU_HEADER *pFUHeader;
    unsigned char nalType,fuType;
    unsigned char NRI;
    unsigned char forbidden_bit;
    int r,e,s;
    unsigned char *pdata,*pDstData;
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
    int b00_00_01 = 0,b00_00_00_01 = 0;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    uint32_t ssrc;
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
    if (m_bSpecial_Ex)
    {
        return DealRecvPacket_ex(pRecvData,nDataLen);
    }
    m_hMemLock.Lock();
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
        }
    }
    if (m_pRecvDataBuff == NULL)
    {
        m_bFrameDealing = 0;
        m_nRecvDataPos = 0;
        m_hMemLock.Unlock();
        RTSP_ERROR("Memory malloc failed\n");
        return -1;
    }
    m_hMemLock.Unlock();
    //    printf("%u\n",packet->GetTimestamp());
    //nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
    nSeq = (pRecvData[2] << 8) | pRecvData[3];
    // nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
    nTimestamp = (pRecvData[4] << 24) | (pRecvData[5] << 16) | (pRecvData[6] << 8) | pRecvData[7];
    //printf("time = %d \n",nTimestamp);
     ssrc = (pRecvData[8] << 24) | (pRecvData[9] << 16) | (pRecvData[10] << 8) | pRecvData[11];

    cc = pRecvData[0] &0x0F;

    bExtHeader = (pRecvData[0] >> 4) & 1;


    pdata = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    npktlen = nDataLen - 12 - cc * 4;//packet->GetPayloadLength();

    if(bExtHeader)
    {
        uint16_t length;
        // pExtHeader = (struct RTPExtensionHeader *)(pdata);
        // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();
    }


    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        m_bFrameDealing = 0;
        m_nRecvDataPos = 0;
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
        return -1;
    }
    //printf("curr = %d,last = %d\n",nSeq,m_nLastSeq);
    bSeqError = 0;
    if (m_nLastSeq != 0)
    {
        nReqSeq = m_nLastSeq + 1;
        if (nSeq != nReqSeq)
        {
            RTSP_ERROR("[RTSP]lost seq no. %d <= %d m ssrc = %x\n",nSeq,m_nLastSeq,ssrc);
            bSeqError = 1;
        }
    }
    m_nLastSeq = nSeq;

    fuType = FU_GET_TYPE(pdata[0]);
    // printf("fuType = %d\n",fuType);
    forbidden_bit = FU_GET_F(pdata[0]);
    NRI = FU_GET_NRI(pdata[0]);

    if (fuType == FU_TYPE_H264)
    {
        s = FUHEADER_GET_S(pdata[1]);
        e = FUHEADER_GET_E(pdata[1]);
        r = FUHEADER_GET_R(pdata[1]);

        if(pdata[2 + 0] == 0x00 &&
            pdata[2 + 1] == 0x00 &&
            pdata[2 + 2] == 0x01)
        {
            b00_00_01 = 1;
            pdata += 4;
            npktlen -= 4;
            nalType = FUHEADER_GET_TYPE(pdata[1]);
            forbidden_bit = FU_GET_F(pdata[1]);
            NRI = FU_GET_NRI(pdata[1]);

        }
        else if(pdata[2 + 0] == 0x00 &&
            pdata[2 + 1] == 0x00 &&
            pdata[2 + 2] == 0x00 &&
            pdata[2 + 3] == 0x01)
        {
            b00_00_00_01 = 1;
            pdata += 5;
            npktlen -= 5;
            nalType = FUHEADER_GET_TYPE(pdata[1]);
            forbidden_bit = FU_GET_F(pdata[1]);
            NRI = FU_GET_NRI(pdata[1]);

        }
        else
        {
            nalType = FUHEADER_GET_TYPE(pdata[1]);
        }
        //printf("s = %d, e = %d, r = %d len = %d nalType = %d \n",s,e,r,nDataLen,nalType);
        if (s+e > 1)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (s)
        {
            if (nalType == NAL_SLICE ||
                nalType == NAL_IDR_SLICE)
            {
                //  m_uiFrameNo++;
            }

            if (m_bFrameDealing != 0)
            {
                m_nRecvDataPos = 0;
            }
            m_bFrameDealing = 1;

        }
        else if (bSeqError)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            //RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (r)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;

        }
        if (m_bFrameDealing == 0)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            // RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
    }


    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen + 8;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    m_hMemLock.Unlock();
                    return -1;
                }
                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
                //free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();



    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;

    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
    pNALU = (NALU_HEADER *)pDstData;

    if (fuType == STAP_A_H264)
    {
        int nSize = 0;
        int i,nleft;
        uint8_t fuType_sub;
        uint8_t *pSize8 = (uint8_t *)&nSize;
        i = 1;
        do
        {
            if (i >= npktlen)
            {
                break;
            }
            nSize = 0;
            pSize8[0] = pdata[i+1];
            pSize8[1] = pdata[i];
            nleft = npktlen - i - 2;
            if (nSize > nleft)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                RTSP_DEBUG("error size %d > %d\n",nSize,nleft);
                return -1;
            }
            fuType_sub = FUHEADER_GET_TYPE(pdata[i + 2]);

            m_nLastNalType = m_nCurrNalType;
            m_nCurrNalType = fuType_sub;
            m_bFrameDealing = 0;
            //判断上一帧是否该送出
            if (fuType_sub == NAL_IDR_SLICE || fuType_sub == NAL_SLICE)
            {//读取当前头
                nRet = ReadH264FrameHeader((char *)&pdata[i + 3],nSize - 1);
                if (nRet < 0)
                {//错误,直接丢掉
                    if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                    {//回调
                        if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
                        {
                            RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
							m_bFrameDealing = 0;
							m_nRecvDataPos = 0;
							 m_bSpecial_MSlices = 0;
							return -1;
                        }
                        m_uiFrameNo++;
                        memset(pFrame,0,sizeof(AntsFrameHeader));
                        pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                        pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                        pFrame->uiFrameNo = m_uiFrameNo;
                        Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                        pFrame->uiFrameTime = Tmptimestamp;
                        Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                        pFrame->uiFrameTickCount = Tmptimestamp;
                        pFrame->uiFrameLen = m_nRecvDataPos;
                        Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                        pFrame->uiTimeStamp = Tmptimestamp;
                        pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                        pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                        pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                        if (m_pRtpSessionClientFxn != NULL)
                        {
                            int prop = 0;
                            if(m_dwProp & 1)
                                prop = 1;

                            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                            if (prop)
                            {
                                m_pRecvDataBuff = NULL;
                            }
                        }
                        bFrame = 1;

                    }
                    m_nRecvDataPos = 0;
                    m_nLastNalType = 0;
                    //m_bRecvPPS = 0;
                    //m_bRecvSPS = 0;
                    return bFrame;
                }
            }
            if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE))
            {
                //printf("*pos = %d,m_nLastNalType = %d ,%d,id = %d %d [%d,%d]\n",m_nRecvDataPos,m_nLastNalType,m_nCurrNalType,m_nLastIDR_pic_id,m_nCurrIDR_pic_id,m_nLastframe_num,m_nframe_num);
                if ((m_nCurrIDR_pic_id != m_nLastIDR_pic_id) ||
                    (m_nLastNalType != m_nCurrNalType) ||
                    (m_nLastframe_num != m_nframe_num))
                {
                    if (m_nLastSlicetype <0 || m_nLastSlicetype > 4)
                    {
                        RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
						m_bFrameDealing = 0;
						m_nRecvDataPos = 0;
						m_bSpecial_MSlices = 0;
						return -1;
                    }
                    m_uiFrameNo++;
                    pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                    pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                    pFrame->uiFrameNo = m_uiFrameNo;
                    Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                    pFrame->uiFrameTime = Tmptimestamp;
                    Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                    pFrame->uiFrameTickCount = Tmptimestamp;
                    pFrame->uiFrameLen = m_nRecvDataPos;
                    Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                    pFrame->uiTimeStamp = Tmptimestamp;
                    pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                    pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                    pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                    if (m_pRtpSessionClientFxn != NULL)
                    {
                        int prop = 0;
                        if(m_dwProp & 1)
                            prop = 1;

                        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                        if (prop)
                        {
                            m_pRecvDataBuff = NULL;
                        }
                    }
                    m_nRecvDataPos = 0;
                    bFrame = 1;
                }

            }

				if (m_pRecvDataBuff == NULL)
				{
					int nMaxSize = H264RTPSESSION_RECV_BUFFSIZE;
					if (nMaxSize < m_nSPSLen + m_nPPSLen + 5 + 5 + nSize + 5)
					{
						nMaxSize = m_nSPSLen + m_nPPSLen + 5 + 5 + nSize + 5;
					}
					m_pRecvDataBuff = (uint8_t *)malloc(nMaxSize);
					if (m_pRecvDataBuff != NULL)
					{
						m_nRecvDataBuffSize = nMaxSize;
					}
				}


            if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
            {
                m_bRecvSPS = 0;
                m_bRecvPPS = 0;
            }

            if (fuType_sub == NAL_IDR_SLICE)
            {
                m_hMemLock.Lock();
                if (!m_bRecvSPS && m_pSPS != NULL)
                {
                    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                    memcpy(pDstData,m_pSPS,m_nSPSLen);
                    m_nRecvDataPos += m_nSPSLen;
                    m_bRecvSPS = 1;
                    ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
                }
                if (!m_bRecvPPS && m_pPPS != NULL)
                {
                    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                    memcpy(pDstData,m_pPPS,m_nPPSLen);
                    m_nRecvDataPos += m_nPPSLen;
                    m_bRecvPPS = 1;
                }
                m_hMemLock.Unlock();
            }

            pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
            memcpy(pDstData + 4,&pdata[i + 2],nSize);
            m_nRecvDataPos += nSize;

            if(fuType_sub == NAL_IDR_SLICE || fuType_sub == NAL_SLICE)
            {//I帧


                if (m_bRecvSPS + m_bRecvPPS != 2)
                {

                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    // RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                    return -1;
                }

                Slicetype = GetH264FrameType();
                if (Slicetype < 0 || Slicetype > 4)
                {
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
					m_bSpecial_MSlices = 0;
                    // m_bRecvPPS = 0;
                    // m_bRecvSPS = 0;
                    RTSP_DEBUG("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
                    return -1;
                }
                if (bSeqError && Slicetype == SLICE_TYPE_P)
                {//丢包了，且当前包不为I帧，则也丢掉
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    //m_bRecvPPS = 0;
                    //m_bRecvSPS = 0;
                    RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                    return -1;
                }

                m_nLastTimestamp = nTimestamp;
                m_nLastSlicetype = m_nCurrSlicetype;
                m_nLastframe_num = m_nframe_num;
                m_nLastIDR_pic_id = m_nCurrIDR_pic_id;



            }
            else if (fuType_sub == NAL_SPS)
            {
                m_bRecvSPS = 1;
                ReadSPS(&pdata[i + 3],nSize - 1);

                //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
            }
            else if (fuType_sub == NAL_PPS)
            {
                m_bRecvPPS = 1;
            }


            i += nSize + 2;

        }while(1);

        return 0;
    }
    else if (fuType != FU_TYPE_H264)
    {// 不分片
        if(pdata[0] == 0x00 &&
            pdata[1] == 0x00 &&
            pdata[2] == 0x01)
        {
            b00_00_01 = 1;
            pdata += 3;
            npktlen -= 3;
            nalType = FUHEADER_GET_TYPE(pdata[0]);
            forbidden_bit = FU_GET_F(pdata[0]);
            NRI = FU_GET_NRI(pdata[0]);
        }
        else if(pdata[0] == 0x00 &&
            pdata[1] == 0x00 &&
            pdata[2] == 0x00 &&
            pdata[3] == 0x01)
        {
            b00_00_00_01 = 1;
            pdata += 4;
            npktlen -= 4;
            nalType = FUHEADER_GET_TYPE(pdata[0]);
            forbidden_bit = FU_GET_F(pdata[0]);
            NRI = FU_GET_NRI(pdata[0]);
        }
        else
        {
            nalType = fuType;//FUHEADER_GET_TYPE(pdata[0]);
        }
        //printf("naltype = %d no\n",nalType);
        m_nLastNalType = m_nCurrNalType;
        m_nCurrNalType = nalType;
        m_bFrameDealing = 0;
        //判断上一帧是否该送出
        if (nalType == NAL_IDR_SLICE || nalType == NAL_SLICE)
        {//读取当前头
            nRet = ReadH264FrameHeader((char *)&pdata[1],npktlen - 1);
            if (nRet < 0)
            {//错误,直接丢掉
                if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                {//回调
                    if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
                    {
                        RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
						m_bFrameDealing = 0;
						m_nRecvDataPos = 0;
						m_bSpecial_MSlices = 0;
						return -1;
                    }
                    m_uiFrameNo++;
                    memset(pFrame,0,sizeof(AntsFrameHeader));
                    pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                    pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                    pFrame->uiFrameNo = m_uiFrameNo;
                    Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                    pFrame->uiFrameTime = Tmptimestamp;
                    Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                    pFrame->uiFrameTickCount = Tmptimestamp;
                    pFrame->uiFrameLen = m_nRecvDataPos;
                    Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                    pFrame->uiTimeStamp = Tmptimestamp;
                    pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                    pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                    pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                    if (m_pRtpSessionClientFxn != NULL)
                    {
                        int prop = 0;
                        if(m_dwProp & 1)
                            prop = 1;

                        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                        if (prop)
                        {
                            m_pRecvDataBuff = NULL;
                        }
                    }
                    bFrame = 1;

                }
                m_nRecvDataPos = 0;
                m_nLastNalType = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                return bFrame;
            }
        }
        if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE))
        {
            //printf("*pos = %d,m_nLastNalType = %d ,%d,id = %d %d [%d,%d]\n",m_nRecvDataPos,m_nLastNalType,m_nCurrNalType,m_nLastIDR_pic_id,m_nCurrIDR_pic_id,m_nLastframe_num,m_nframe_num);
            if ((m_nCurrIDR_pic_id != m_nLastIDR_pic_id) ||
                (m_nLastNalType != m_nCurrNalType) ||
                (m_nLastframe_num != m_nframe_num))
            {
                if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
                {
                    RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
					m_bFrameDealing = 0;
					m_nRecvDataPos = 0;
					m_bSpecial_MSlices = 0;
					return -1;
                }
                m_uiFrameNo++;
                pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                pFrame->uiFrameNo = m_uiFrameNo;
                Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTime = Tmptimestamp;
                Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameTickCount = Tmptimestamp;
                pFrame->uiFrameLen = m_nRecvDataPos;
                Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                pFrame->uiTimeStamp = Tmptimestamp;
                pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                if (m_pRtpSessionClientFxn != NULL)
                {
                    int prop = 0;
                    if(m_dwProp & 1)
                        prop = 1;

                    m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                    if (prop)
                    {
                        m_pRecvDataBuff = NULL;
                    }
                }
                m_nRecvDataPos = 0;
                bFrame = 1;
            }

        }

		if (m_pRecvDataBuff == NULL)
		{
			int nMaxSize = H264RTPSESSION_RECV_BUFFSIZE;
			if (nMaxSize < m_nSPSLen + m_nPPSLen + 5 + 5 + npktlen + 5)
			{
				nMaxSize = m_nSPSLen + m_nPPSLen + 5 + 5 + npktlen + 5;
			}
			m_pRecvDataBuff = (uint8_t *)malloc(nMaxSize);
			if (m_pRecvDataBuff != NULL)
			{
				m_nRecvDataBuffSize = nMaxSize;
			}
		}

        if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
        {
            m_bRecvSPS = 0;
            m_bRecvPPS = 0;
        }
        if (nalType == NAL_IDR_SLICE)
        {
            m_hMemLock.Lock();
            if (!m_bRecvSPS && m_pSPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pSPS,m_nSPSLen);
                m_nRecvDataPos += m_nSPSLen;
                m_bRecvSPS = 1;
                ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
            }
            if (!m_bRecvPPS && m_pPPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pPPS,m_nPPSLen);
                m_nRecvDataPos += m_nPPSLen;
                m_bRecvPPS = 1;
            }
            m_hMemLock.Unlock();
        }

        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        pDstData[0] = 0x00;
        pDstData[1] = 0x00;
        pDstData[2] = 0x00;
        pDstData[3] = 0x01;
        m_nRecvDataPos += 4;
        memcpy(pDstData + 4,pdata,npktlen);
        m_nRecvDataPos += npktlen;



        if(nalType == NAL_IDR_SLICE || nalType == NAL_SLICE)
        {//I帧


            if (m_bRecvSPS + m_bRecvPPS != 2)
            {

                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                // RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }



            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                // m_bRecvPPS = 0;
                // m_bRecvSPS = 0;
				m_bSpecial_MSlices = 0;
                RTSP_DEBUG("[rtp264]line = %d Slicetype = %d naltype = %d\n",__LINE__,Slicetype,nalType);
                return -1;
            }
            if (bSeqError && Slicetype == SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }

            m_nLastTimestamp = nTimestamp;
            m_nLastSlicetype = m_nCurrSlicetype;
            m_nLastframe_num = m_nframe_num;
            m_nLastIDR_pic_id = m_nCurrIDR_pic_id;


            return bFrame;// Frame

        }
        else if (nalType == NAL_SPS)
        {
            m_bRecvSPS = 1;
            Profile_idc = pdata[1];
            Constraint_sets = pdata[2];
            Level_idc = pdata[3];
            ReadSPS(pdata+1,npktlen - 1);

            //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
        }
        else if (nalType == NAL_PPS)
        {
            m_bRecvPPS = 1;
        }

        return 0;

    }


    if (s)
    {
        //开始了新一帧
        m_nLastNalType = m_nCurrNalType;
        m_nCurrNalType = nalType;

        m_nRecvDataStart = sizeof(AntsFrameHeader);
        //判断上一帧是否该送出
        if (nalType == NAL_IDR_SLICE || nalType == NAL_SLICE)
        {//读取当前头
            nRet = ReadH264FrameHeader((char *)&pdata[2],npktlen - 2);
            if (nRet < 0)
            {//错误,直接丢掉
                RTSP_DEBUG("error \n");
                if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE ))
                {//回调
                    if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
                    {
                        RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
						m_bFrameDealing = 0;
						m_nRecvDataPos = 0;
						m_bSpecial_MSlices = 0;
						return -1;
                    }
                    m_uiFrameNo++;
                    memset(pFrame,0,sizeof(AntsFrameHeader));
                    pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                    pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                    pFrame->uiFrameNo = m_uiFrameNo;
                    Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                    pFrame->uiFrameTime = Tmptimestamp;
                    Tmptimestamp = (m_nLastTimestamp * 1000.0* 1000 / GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                    pFrame->uiFrameTickCount = Tmptimestamp;
                    pFrame->uiFrameLen = m_nRecvDataPos;
                    Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                    pFrame->uiTimeStamp = Tmptimestamp;
                    pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                    pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                    pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                    if (m_pRtpSessionClientFxn != NULL)
                    {
                        int prop = 0;
                        if(m_dwProp & 1)
                            prop = 1;

                        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                        if (prop)
                        {
                            m_pRecvDataBuff = NULL;
                        }
                    }
                    bFrame = 1;

                }
                m_nRecvDataPos = 0;
                m_nLastNalType = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                return bFrame;
            }
        }
        if (m_nCurrNalType == NAL_IDR_SLICE)
        {
            int ii = 0;
        }
        //printf("pos = %d,m_nLastNalType = %d ,%d,id = %d %d [%d,%d]\n",m_nRecvDataPos,m_nLastNalType,m_nCurrNalType,m_nLastIDR_pic_id,m_nCurrIDR_pic_id,m_nLastframe_num,m_nframe_num);
        if (m_nRecvDataPos > 0 && (m_nLastNalType == NAL_IDR_SLICE || m_nLastNalType == NAL_SLICE))
        {
            if ((m_nCurrIDR_pic_id != m_nLastIDR_pic_id) ||
                (m_nLastNalType != m_nCurrNalType) ||
                (m_nLastframe_num != m_nframe_num))
            {
                if(m_nLastSlicetype < 0 || m_nLastSlicetype > 4)
                {
                    RTSP_DEBUG("[RTSP]%d slicetype = %d\n",__LINE__,m_nLastSlicetype);
					m_bFrameDealing = 0;
					m_nRecvDataPos = 0;
					m_bSpecial_MSlices = 0;
					return -1;
                }
                m_uiFrameNo++;
                pFrame->uiStartId = ANTS_FRAME_STARTCODE;
                pFrame->uiFrameType = frametypeMap[m_nLastSlicetype];
                pFrame->uiFrameNo = m_uiFrameNo;
                Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
                pFrame->uiFrameTime = Tmptimestamp;
                Tmptimestamp= (m_nLastTimestamp * 1000* 1000 / GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
                pFrame->uiFrameTickCount = Tmptimestamp;
                //printf("%d %d - %d\n",m_nLastTimestamp,pFrame->uiFrameTime,pFrame->uiFrameTickCount);
                pFrame->uiFrameLen = m_nRecvDataPos;
                Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
                pFrame->uiTimeStamp = Tmptimestamp;
                pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
                pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
                pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
                if (m_pRtpSessionClientFxn != NULL)
                {
                    int prop = 0;
                    if(m_dwProp & 1)
                        prop = 1;

                    m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
                    if (prop)
                    {
                        m_pRecvDataBuff = NULL;
                    }
                }
                m_nRecvDataPos = 0;
                bFrame = 1;
            }

        }

		if (m_pRecvDataBuff == NULL)
		{
			int nMaxSize = H264RTPSESSION_RECV_BUFFSIZE;
			if (nMaxSize < m_nSPSLen + m_nPPSLen + 5 + 5 + npktlen + 5)
			{
				nMaxSize = m_nSPSLen + m_nPPSLen + 5 + 5 + npktlen + 5;
			}
			m_pRecvDataBuff = (uint8_t *)malloc(nMaxSize);
			if (m_pRecvDataBuff != NULL)
			{
				m_nRecvDataBuffSize = nMaxSize;
			}
		}


        if (m_nCurrNalType == NAL_IDR_SLICE && m_nLastNalType == NAL_SLICE)
        {
            m_bRecvSPS = 0;
            m_bRecvPPS = 0;
        }

        if (nalType == NAL_IDR_SLICE)
        {
            m_hMemLock.Lock();
            if (!m_bRecvSPS && m_pSPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pSPS,m_nSPSLen);
                m_nRecvDataPos += m_nSPSLen;
                m_bRecvSPS = 1;
                ReadSPS(m_pSPS + 5,m_nSPSLen - 5);
            }
            if (!m_bRecvPPS && m_pPPS != NULL)
            {
                pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
                memcpy(pDstData,m_pPPS,m_nPPSLen);
                m_nRecvDataPos += m_nPPSLen;
                m_bRecvPPS = 1;
            }
            m_hMemLock.Unlock();
        }

        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        if (nalType != 0)
        {
            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;
        }
        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);


        if (nalType == NAL_SLICE || nalType == NAL_IDR_SLICE)
        {


            if (m_bRecvSPS + m_bRecvPPS != 2)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //RTSP_DEBUG("[rtp264]line = %d m_bRecvSPS = %d m_bRecvPPS = %d\n",__LINE__,m_bRecvSPS,m_bRecvPPS);
                return -1;
            }
            //ReadH264FrameHeader((char *)&pdata[2],npktlen - 2);
            Slicetype = GetH264FrameType();
            if (Slicetype < 0 || Slicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
				m_bSpecial_MSlices = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }

            if (bSeqError && m_nCurrSlicetype == SLICE_TYPE_P)
            {//丢包了，且当前包不为I帧，则也丢掉
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                //m_bRecvPPS = 0;
                //m_bRecvSPS = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }




        }
        else if (nalType == NAL_SPS)
        {
            m_bRecvSPS = 1;
            Profile_idc = pdata[1];
            Constraint_sets = pdata[2];
            Level_idc = pdata[3];
            ReadSPS(pdata+2,npktlen - 2);
            m_nCurrNalType = nalType;
            // printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
        }
        else if (nalType == NAL_PPS)
        {
            m_bRecvPPS = 1;
            m_nCurrNalType = nalType;
        }
        else
        {
            m_nCurrNalType = nalType;
        }



        //  RTSP_DEBUG("[rtp264]start line = %d clicetype = %d naltype = %d NRI = %d\n",__LINE__,m_nCurrSlicetype,nalType,NRI);
        NALU_RESET(pDstData[0]);
        NALU_SET_NRI(pDstData[0],NRI);
        NALU_SET_F(pDstData[0],forbidden_bit);
        NALU_SET_TYPE(pDstData[0],nalType);
        m_nRecvDataPos += 1;
        memcpy(pDstData+1,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen - 2;

    }
    else if (e)
    {

        //最后一帧完成
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;
        m_bFrameDealing = 0;
        if (m_nCurrNalType == NAL_SLICE || m_nCurrNalType == NAL_IDR_SLICE)
        {
            memset(pFrame,0,sizeof(AntsFrameHeader));
            // m_uiLastFrameNo = m_uiFrameNo;
            if (m_nCurrSlicetype <0 || m_nCurrSlicetype > 4)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
				m_bSpecial_MSlices = 0;
                RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
                return -1;
            }


            m_nLastTimestamp = nTimestamp;
            m_nLastSlicetype = m_nCurrSlicetype;
            m_nLastframe_num = m_nframe_num;
            m_nLastIDR_pic_id = m_nCurrIDR_pic_id;

            //printf("size = %d\n",m_nRecvDataBuffSize);

            return 0;// frame
        }
        m_bFrameDealing = 0;
    }
    else
    {
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;

    }


    return 0;

}
int CH264RtpSession::DealRecvPacket_ex(unsigned char *pRecvData,int nDataLen)
{
    FU_INDICATOR *pFU;
    NALU_HEADER *pNALU;
    FU_HEADER *pFUHeader;
    unsigned char nalType,fuType;
    unsigned char NRI;
    unsigned char forbidden_bit;
    int r,e,s;
    unsigned char *pdata,*pDstData;
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
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;

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
    if (!m_bSpecial_Ex)
    {
        m_bFrameDealing = 0;
        m_nRecvDataPos = 0;
        return -1;
    }
    m_hMemLock.Lock();
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
        }
    }
    if (m_pRecvDataBuff == NULL)
    {
        m_bFrameDealing = 0;
        m_nRecvDataPos = 0;
        m_hMemLock.Unlock();
        RTSP_ERROR("Memory malloc failed\n");
        return -1;
    }
    m_hMemLock.Unlock();
    //    printf("%u\n",packet->GetTimestamp());
    //nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
    nSeq = (pRecvData[2] << 8) | pRecvData[3];
    // nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
    nTimestamp = (pRecvData[4] << 24) | (pRecvData[5] << 16) | (pRecvData[6] << 8) | pRecvData[7];
       //printf("time = %d \n",nTimestamp);

    cc = pRecvData[0] &0x0F;

    bExtHeader = (pRecvData[0] >> 4) & 1;


    pdata = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    npktlen = nDataLen - 12 - cc * 4;//packet->GetPayloadLength();

    if(bExtHeader)
    {
        uint16_t length;
        // pExtHeader = (struct RTPExtensionHeader *)(pdata);
        // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();
    }


    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        m_nRecvDataPos = 0;
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
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

    fuType = FU_GET_TYPE(pdata[0]);
    //printf("fuType = %d\n",fuType);
    forbidden_bit = FU_GET_F(pdata[0]);
    NRI = FU_GET_NRI(pdata[0]);

  if(fuType == FU_TYPE_H264)
  {
        s = FUHEADER_GET_S(pdata[1]);
        e = FUHEADER_GET_E(pdata[1]);
        r = FUHEADER_GET_R(pdata[1]);

        nalType = FUHEADER_GET_TYPE(pdata[1]);
         //printf("s = %d, e = %d, r = %d len = %d nalType = %d \n",s,e,r,nDataLen,nalType);
        if (s+e > 1)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            RTSP_ERROR("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (s)
        {
            if (m_bFrameDealing != 0)
            {
                m_nRecvDataPos = 0;
            }
            m_bFrameDealing = 1;

        }
        else if (bSeqError)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
             //RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
        if (r)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;

        }
        if (m_bFrameDealing == 0)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
             //RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }
  }


    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen + 8;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bFrameDealing = 0;
            m_nRecvDataPos = 0;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d \n",__LINE__);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bFrameDealing = 0;
                m_nRecvDataPos = 0;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bFrameDealing = 0;
                    m_nRecvDataPos = 0;
                    m_hMemLock.Unlock();
                    return -1;
                }
                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
                //free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();



    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;

    pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
    pNALU = (NALU_HEADER *)pDstData;
    if(fuType != FU_TYPE_H264)
    {
        TST_VIDEO_FRAME_HEADER *pTstHeader;
        unsigned char *p8,tmp8[sizeof(TST_VIDEO_FRAME_HEADER) + 4];
        int bIFrame = 0;
        nalType = fuType;
        m_bFrameDealing = 0;
   // printf("fuType = %d\n",fuType);
        if(pdata[1] == 0x00 &&
            pdata[2] == 0x00 &&
            pdata[3] == 0x01)
        {

        }
        else if(pdata[1] == 0x00 &&
            pdata[2] == 0x00 &&
            pdata[3] == 0x00 &&
            pdata[4] == 0x01)
        {

        }
        else
        {
            pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);

            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;

        }

        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //  RTSP_DEBUG("[rtp264]start line = %d clicetype = %d naltype = %d NRI = %d\n",__LINE__,m_nCurrSlicetype,nalType,NRI);
        NALU_RESET(pDstData[0]);
        NALU_SET_NRI(pDstData[0],NRI);
        NALU_SET_F(pDstData[0],forbidden_bit);
        NALU_SET_TYPE(pDstData[0],nalType);
        m_nRecvDataPos += 1;
        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);

        memcpy(pDstData,pdata + 1,npktlen - 1);
        m_nRecvDataPos += npktlen - 1;
        pTstHeader =(TST_VIDEO_FRAME_HEADER *) ((((uint64_t)tmp8) + 4) & (~3));
        p8 = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos - sizeof(TST_VIDEO_FRAME_HEADER));
        memcpy(pTstHeader,p8,sizeof(TST_VIDEO_FRAME_HEADER));
        if(pTstHeader->flag != 0x1a2b3c4d)
        {
            //RTSP_ERROR("[rtp264.%d]invalid Tst header \n",__LINE__);
            return -1;
        }


        //printf("frame_index = %u,keyframe_index = %u m_dwLastFrameIndex = %u m_dwLastKeyFrameIndex = %u\n",pTstHeader->frame_index , pTstHeader->keyframe_index,m_dwLastFrameIndex,m_dwLastKeyFrameIndex);
        if(pTstHeader->frame_index == pTstHeader->keyframe_index)  //是关键帧
        {

            m_dwLastFrameIndex    =
                m_dwLastKeyFrameIndex = pTstHeader->keyframe_index;
            bIFrame = 1;
          // printf("KEY!!!!\n");

        }
        else
        {
            if(pTstHeader->frame_index !=m_dwLastFrameIndex + 1)
            {//不进行解此帧，跳过
                m_nRecvDataPos = 0;
                return -1;
            }
            else if(pTstHeader->keyframe_index != m_dwLastKeyFrameIndex)
            {//不进行解此帧，跳过
                m_nRecvDataPos = 0;
                return -1;
            }
            else
            {//记录最后一次解码帧号
                m_dwLastFrameIndex++;
            }
        }
        if(bIFrame)
        {
            // 查找SPS，获取宽高信息
            int i;
            int zeroCnt = 0;
            int tmpNalType = nalType;
            unsigned char cc;
            unsigned char *pFind;
            int nFindLen;
            int bNal = 0;
            bNal = 0;
            pFind = m_pRecvDataBuff + m_nRecvDataStart;
            nFindLen = m_nRecvDataPos;
            for (i = 0; i < nFindLen; i++)
            {
                cc = pFind[i];
                if (bNal)
                {
                    bNal = 0;
                    tmpNalType = NALU_GET_TYPE(cc);
                    //printf("tmpNalType = %d\n",tmpNalType);
                    if (tmpNalType == NAL_SPS)
                    {
                        m_bRecvSPS = 1;
                        ReadSPS(pFind+i+1,nFindLen - i - 1);
                        break;
                        //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
                    }
                    else if (tmpNalType == NAL_PPS)
                    {
                        m_bRecvPPS = 1;

                    }
                    else if (tmpNalType == NAL_SLICE || tmpNalType == NAL_IDR_SLICE)
                    {

                        break;
                    }
                }
                if (cc == 0x01 && zeroCnt >= 2)
                {
                    if (zeroCnt == 2)
                    {
                        // 丢掉

                    }
                    // else if (zeroCnt > 2)
                    {
                        bNal = 1;
                    }

                }
                if (cc == 0)
                {
                    zeroCnt++;
                }
                else
                {
                    zeroCnt = 0;
                }

            }
        }
        m_nLastTimestamp = nTimestamp;
        // 开始回调
        pFrame->uiStartId = ANTS_FRAME_STARTCODE;
        pFrame->uiFrameType = bIFrame?AntsPktIFrames:AntsPktPFrames;
        pFrame->uiFrameNo = m_dwLastFrameIndex;
        Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
        pFrame->uiFrameTime = Tmptimestamp;
        Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
        pFrame->uiFrameTickCount = Tmptimestamp;
        // printf("%d - %d\n",pFrame->uiFrameTime,pFrame->uiFrameTickCount);
        m_nRecvDataPos -= sizeof(TST_VIDEO_FRAME_HEADER);
        pFrame->uiFrameLen = m_nRecvDataPos;
        Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
        pFrame->uiTimeStamp = Tmptimestamp;
        pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
        pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
        pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
        if (m_pRtpSessionClientFxn != NULL)
        {
            int prop = 0;
            if(m_dwProp & 1)
                prop = 1;

            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
            if (prop)
            {
                m_pRecvDataBuff = NULL;
            }
        }
        m_nRecvDataPos = 0;
        bFrame = 1;

      //  printf("fuType = %d\n",fuType);
        return 0;
    }
    if (s)
    {
        //开始了新一帧

        if(pdata[2] == 0x00 &&
           pdata[3] == 0x00 &&
           pdata[4] == 0x01)
        {

        }
        else if(pdata[2] == 0x00 &&
                pdata[3] == 0x00 &&
                pdata[4] == 0x00 &&
                pdata[5] == 0x01)
        {

        }
        else
        {
            pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);

            pDstData[0] = 0x00;
            pDstData[1] = 0x00;
            pDstData[2] = 0x00;
            pDstData[3] = 0x01;
            m_nRecvDataPos += 4;

        }

        pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        //  RTSP_DEBUG("[rtp264]start line = %d clicetype = %d naltype = %d NRI = %d\n",__LINE__,m_nCurrSlicetype,nalType,NRI);
        NALU_RESET(pDstData[0]);
        NALU_SET_NRI(pDstData[0],NRI);
        NALU_SET_F(pDstData[0],forbidden_bit);
        NALU_SET_TYPE(pDstData[0],nalType);
        m_nRecvDataPos += 1;
        memcpy(pDstData+1,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen - 2;

    }
    else if (e)
    {
        TST_VIDEO_FRAME_HEADER *pTstHeader;
        unsigned char *p8,tmp8[sizeof(TST_VIDEO_FRAME_HEADER) + 4];
        int bIFrame = 0;
        //最后一帧完成
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;
        m_bFrameDealing = 0;
        pTstHeader =(TST_VIDEO_FRAME_HEADER *) ((((uint64_t)tmp8) + 4) & (~3));
        p8 = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos - sizeof(TST_VIDEO_FRAME_HEADER));
        memcpy(pTstHeader,p8,sizeof(TST_VIDEO_FRAME_HEADER));
        if(pTstHeader->flag != 0x1a2b3c4d)
        {
            m_nRecvDataPos = 0;
            if(!m_bSpecial_Fix)
            {
                m_bSpecial_Ex = 0;
            }
           RTSP_ERROR("[rtp264.%d]invalid Tst header flag = %x addr = %p [%x %x %x %x]\n",__LINE__,pTstHeader->flag,pTstHeader,tmp8[0],tmp8[1],tmp8[2],tmp8[3]);
           return -1;
        }
        else if (!m_bSpecial_Fix)
        {
            m_bSpecial_Fix = 1;
        }

        if(pTstHeader->frame_index == pTstHeader->keyframe_index)  //是关键帧
        {

            m_dwLastFrameIndex    =
            m_dwLastKeyFrameIndex = pTstHeader->keyframe_index;
            bIFrame = 1;
           // printf("KEY!!!!\n");
        }
        else
        {
            if(pTstHeader->frame_index !=m_dwLastFrameIndex + 1)
            {//不进行解此帧，跳过
                m_nRecvDataPos = 0;
                return -1;
            }
            else if(pTstHeader->keyframe_index != m_dwLastKeyFrameIndex)
            {//不进行解此帧，跳过
                m_nRecvDataPos = 0;
                return -1;
            }
            else
            {//记录最后一次解码帧号
                m_dwLastFrameIndex++;
            }
        }
        if(bIFrame)
        {
            // 查找SPS，获取宽高信息
            int i;
            int zeroCnt = 0;
            int tmpNalType = nalType;
            unsigned char cc;
            unsigned char *pFind;
            int nFindLen;
            int bNal = 0;
            bNal = 0;
            pFind = m_pRecvDataBuff + m_nRecvDataStart;
            nFindLen = m_nRecvDataPos;
            for (i = 0; i < nFindLen; i++)
            {
                cc = pFind[i];
                if (bNal)
                {
                    bNal = 0;
                    tmpNalType = NALU_GET_TYPE(cc);
                       //printf("tmpNalType = %d\n",tmpNalType);
                    if (tmpNalType == NAL_SPS)
                    {
                        m_bRecvSPS = 1;
                        ReadSPS(pFind+i+1,nFindLen - i - 1);
                        break;
                        //printf("%d,%d,%d\n",Profile_idc,Constraint_sets,Level_idc);
                    }
                    else if (tmpNalType == NAL_PPS)
                    {
                        m_bRecvPPS = 1;

                    }
                    else if (tmpNalType == NAL_SLICE || tmpNalType == NAL_IDR_SLICE)
                    {

                       break;
                    }
                }
                if (cc == 0x01 && zeroCnt >= 2)
                {
                    if (zeroCnt == 2)
                    {
                        // 丢掉

                    }
                    // else if (zeroCnt > 2)
                    {
                        bNal = 1;
                    }

                }
                if (cc == 0)
                {
                    zeroCnt++;
                }
                else
                {
                    zeroCnt = 0;
                }

            }
        }
        m_nLastTimestamp = nTimestamp;
        // 开始回调
        pFrame->uiStartId = ANTS_FRAME_STARTCODE;
        pFrame->uiFrameType = bIFrame?AntsPktIFrames:AntsPktPFrames;
        pFrame->uiFrameNo = m_dwLastFrameIndex;
        Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate() ;
        pFrame->uiFrameTime = Tmptimestamp;
        Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
        pFrame->uiFrameTickCount = Tmptimestamp;
        // printf("%d - %d\n",pFrame->uiFrameTime,pFrame->uiFrameTickCount);
        m_nRecvDataPos -= sizeof(TST_VIDEO_FRAME_HEADER);
        pFrame->uiFrameLen = m_nRecvDataPos;
        Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
        pFrame->uiTimeStamp = Tmptimestamp;
        pFrame->uMedia.struVideoHeader.cCodecId = AntsH264_hisi_RTP;//m_nProfile_idc >= 100 ?AntsH264_hisi_high:AntsH264_hisi;
        pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
        pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;


        if (m_pRtpSessionClientFxn != NULL)
        {
            int prop = 0;
            if(m_dwProp & 1)
                prop = 1;

            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_H264,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
            if (prop)
            {
                m_pRecvDataBuff = NULL;
            }
        }
        m_nRecvDataPos = 0;
        bFrame = 1;



    }
    else
    {
        memcpy(pDstData,pdata + 2,npktlen - 2);
        m_nRecvDataPos += npktlen -2;

    }


    return bFrame;

}

//static int btst = 0;
int CH264RtpSession::SendMJPEGPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)//毫秒
{
    unsigned char *pNalStart,*pNalEnd,*pCurr,*pJpegData = NULL;
    int NalLen;
    int nRet = -1;
    unsigned int timestampinc;

    uint32_t uStartCode = 0;
    uint32_t *p32;
    MJPEG_HEADER *pMjpegHeader;
    MJPEG_RESETMARKER *pMjpegResetMarker;
    MJPEG_QUANTHEADER *pMjpegQuantHeader;
    unsigned char CC,CC1,CC2;
    unsigned short wLen;
    int nReadPos = 0;
    int nJpegDataSize = 0 ,nJpegDataPos = 0;
    int nQTableLen = 0;
    int nPktLen;
    int nFrameLen;
    int nOffset = 0;

    int PackSize = GetPacketMaxDataSize();
    int nPos;
    int bMark = 0;
    int nSendSize;
    uint8_t *pPackBuf = NULL;
    unsigned char *pQTable[6] = {NULL,NULL,NULL,NULL,NULL,NULL};
    int nQtableLeng[6] = {0,0,0,0,0,0};
    int nQtableNum = 0;
    int nLumId= -1,nCbId = -1,nCrId = -1;
    int nmul ;


    pPackBuf = GetPacketBuffer();
    if (pPackBuf == NULL)
    {
        return -1;
    }
#if 0
    if(!btst)
    {
        FILE *pf;
            btst = 1;
            pf = fopen("/mnt/zqf/test_save.jpg","wb+");
            if (pf != NULL)
            {
                fwrite(data,1,len,pf);
                fclose(pf);
            }
    }
#endif
    if (data == NULL)
    {
        return nRet;
    }

    pMjpegHeader = (MJPEG_HEADER *) pPackBuf;
    pMjpegResetMarker = (MJPEG_RESETMARKER *)(pMjpegHeader + 1);
    pMjpegQuantHeader = (MJPEG_QUANTHEADER *)(pMjpegResetMarker + 1);
    // 检查是否合法
    pCurr = (unsigned char *)data;
    // 检查起始 SOI
    if (pCurr[0] != 0xFF || pCurr[1] != 0xD8)
    {
         return -1;
    }
    // 检查结束
    if (pCurr[len - 1] != 0xD9 || pCurr[len - 2] != 0xFF)
    {
        return -1;
    }
     m_bVideoReady = 1;
    memset(pMjpegHeader,0,sizeof(MJPEG_HEADER));
    memset(pMjpegResetMarker,0,sizeof(MJPEG_RESETMARKER));
    memset(pMjpegQuantHeader,0,sizeof(MJPEG_QUANTHEADER));
    nReadPos += 2;
    do
    {
        if (nReadPos >= len)
        {
            break;
        }
        if (len - nReadPos >= 2)
        {
            if (pCurr[nReadPos] != 0xFF)
            {
                return -1;
            }



            if (len - nReadPos < 4)
            {
                return -1;
            }
            wLen = pCurr[nReadPos + 3] | (pCurr[nReadPos + 2] << 8);

            if (len - nReadPos < 2 + wLen)
            {
                return -1;
            }
           // printf("%02X -",pCurr[nReadPos + 1]);
            // 处理
            switch (pCurr[nReadPos + 1])
            {
            case 0xE0:
                {
                    // APP0
                    break;
                }
            case 0xE1:
                {
                    // APP1
                    break;
                }
            case 0xDB:
                {
                    // DQT
                    MJPEG_DQT tDQT;
                    int Qlen = 0;
                    int Qid = 0,QPr = 0;
                    memcpy(&tDQT,pCurr + nReadPos,sizeof(tDQT));

                        int m;
                        //printf("Qlen = %d\n",wLen - 3);
                        for (m = 0; m < wLen -2;)
                        {
                            Qid = tDQT.byQT & 0xF;
                            QPr = (tDQT.byQT >> 4 ) & 1;
                            Qlen = 64 * (QPr + 1);
                            m++;
                            if (Qid >= 0 && Qid <= 3)
                            {
#if 0
                                pQTable[Qid] = pCurr + nReadPos + 4 + m;
                                nQtableLeng[Qid] = Qlen;
                                nQtableNum ++;
#else
                                //printf("Qid = %d Qpr = %d Qlen = %d nQTableLen = %d\n",Qid,QPr,Qlen,nQTableLen);
                                memcpy(pMjpegQuantHeader->byQTable + nQTableLen,pCurr + nReadPos + 4 + m ,Qlen);
                                nQTableLen += Qlen;
                                pMjpegQuantHeader->wQTableLength = htons(nQTableLen);
                                pMjpegQuantHeader->byPrecision |= (QPr << Qid);
#endif
                                if (nQTableLen >= 128)
                                { // 多余的丢掉，否则 VLC播放器会色彩不对
                                    break;
                                }
                           }

                            m += Qlen;
                         }

                    break;
                }
            case 0xDD:
                {
                    //DRI
                    MJPEG_DRI tDRI;
                    memcpy(&tDRI,pCurr + nReadPos,sizeof(tDRI));
                    if (tDRI.wRi > 0)
                    {
                        pMjpegResetMarker->F = 1;
                        pMjpegResetMarker->L = 1;
                        pMjpegResetMarker->uResetCnt = 0x3FFF;
                        pMjpegResetMarker->wResetInterval = htons(tDRI.wRi);
                        pMjpegHeader->byType |= 64;
                    }

                    break;
                }
            case 0xC4:
                {
                    // DHT
                    break;
                }
            case 0xC0:
                {
                    // SOF
                    MJPEG_SOF tSof;
                    int yVH,uVH,vVH;
                    memcpy(&tSof,pCurr + nReadPos ,sizeof(MJPEG_SOF));
                    pMjpegHeader->byQ = 0xFF;
                    pMjpegHeader->byWidth = (htons(tSof.wWidth) + 7) / 8;
                    pMjpegHeader->byHeight = (htons(tSof.wHeight) + 7) / 8;
                    if (tSof.byComponentNum != 3)
                    {
                        return -1;
                    }
                    yVH = tSof.byComponents[1]; // 0 1 2
                    uVH = tSof.byComponents[4];// 3 4 5
                    vVH = tSof.byComponents[7];// 6 7 8
                    if (tSof.byComponents[0] == 1)
                    {
                        nLumId = tSof.byComponents[2];
                    }
                    if (tSof.byComponents[3] == 2)
                    {
                        nCbId = tSof.byComponents[5];
                    }
                    if (tSof.byComponents[6] == 3)
                    {
                        nCrId = tSof.byComponents[8];

                    }
                    if (yVH == 0x21 && uVH == 0x11 && vVH == 0x11)
                    {// 422
                        pMjpegHeader->byType |= 0;
                    }
                    else if (yVH == 0x22 && uVH == 0x11 && vVH == 0x11)
                    {// 420
                        pMjpegHeader->byType |= 1;
                    }
                    else
                    {
                        return -1;
                    }


                    break;
                }
            case 0xDA:
                {
                    // SOS
                    MJPEG_SOS tSos;
                    memcpy(&tSos,pCurr + nReadPos,sizeof(MJPEG_SOS));
                    pJpegData = pCurr + nReadPos + 2 + wLen;
                    nJpegDataSize = len - nReadPos - 2 - wLen - 2;
                    break;
                }
            default:
                {
                    // printf("JPEG Code = 0x%0x wlen = %d\n",pCurr[nReadPos + 1],wLen);
                    break;
                }
            }
            if (pJpegData != NULL)
            {
                break;
            }

            nReadPos += (2 + wLen);
        }
    } while (1);
//printf("\n");
    if (pJpegData == NULL)
    {
        return -1;
    }
  nmul = 90;//CRtspServer::GetPayloadClockRate(m_nDefaultPayloadType) / 1000;
    nmul = GetPayloadClockRate() / 1000;

    int nFps = GetFrameRate();
    if (nFps > 0)
    {
        m_nCurrStamp += nmul * 1000 / nFps;
    }
    else
    {
        if (bTimeStampValid & 1)
        {
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


            m_nLastStamp = timestamp;

            m_nCurrStamp += timestampinc;
        }
    }
#if 0
    // 量化表
    if (nQtableNum > 0)
    {

        memcpy(pMjpegQuantHeader->byQTable,pQTable[nLumId],nQtableLeng[nLumId]);
        memcpy(pMjpegQuantHeader->byQTable+ nQtableLeng[nLumId],pQTable[nCbId],nQtableLeng[nCbId]);
        nQTableLen = nQtableLeng[nLumId] + nQtableLeng[nCbId];
        pMjpegQuantHeader->wQTableLength = htons(nQTableLen);
        pMjpegQuantHeader->byPrecision = 0;
    }
#endif

    //分片..

        bMark = 0;
        nPos = sizeof(MJPEG_HEADER) + sizeof(MJPEG_RESETMARKER);
        nJpegDataPos = 0;
        // 分片
        do
        {
            pMjpegHeader->uOffset24 = htonl(nOffset)>>8;
            if (nOffset == 0)
            { // 第一片
                nPos = sizeof(MJPEG_HEADER) + sizeof(MJPEG_QUANTHEADER) + nQTableLen;

            }
            else
            {
                nPos = sizeof(MJPEG_HEADER) ;

            }
            if ( pMjpegHeader->byType > 63)
            {
                nPos +=  sizeof(MJPEG_RESETMARKER);
            }
            else if(nOffset == 0)
            {
                memmove(pMjpegResetMarker,pMjpegQuantHeader,sizeof(MJPEG_QUANTHEADER) + nQTableLen);
            }
            if (PackSize - nPos >= nJpegDataSize - nJpegDataPos)
            {
                bMark = 1;
                nSendSize = nPos + nJpegDataSize - nJpegDataPos;
            }
            else
            {
                bMark = 0;
                nSendSize = PackSize;
            }
            memcpy(pPackBuf + nPos,pJpegData + nJpegDataPos,nSendSize - nPos);
            // nRet = SendPacketNoCopy(pJpegData + nJpegDataPos,nSendSize - nPos,NULL,0,bMark,m_nCurrStamp);
            nJpegDataPos += nSendSize - nPos;
            nOffset = pJpegData + nJpegDataPos - pCurr;
            if (m_nTranType == 1 && m_bExternSocket)
            {
                nRet = SendPacketByBuffer(m_nInterleaved[0],(char *)pPackBuf,nSendSize,NULL,0,bMark,m_nCurrStamp);
            }
            else
            {
                nRet = SendPacket(nSendSize,bMark,m_nCurrStamp);
            }

            if (bMark)
            {
                break;
            }
        } while (1);




    return nRet;
}

static void makeDefaultQtables(unsigned char* resultTables, unsigned Q) {
    int factor = Q;
    int q;

    if (Q < 1) factor = 1;
    else if (Q > 99) factor = 99;

    if (Q < 50) {
        q = 5000 / factor;
    } else {
        q = 200 - factor*2;
    }

    for (int i = 0; i < 128; ++i) {
        int newVal = (defaultQuantizers[i]*q + 50)/100;
        if (newVal < 1) newVal = 1;
        else if (newVal > 255) newVal = 255;
        resultTables[i] = newVal;
    }
}

int CH264RtpSession::DealRecvMJPEGPacket(unsigned char *pRecvData,int nDataLen)
{
    int r,e,s;
    unsigned char *pdata,*pDstData;
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
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    struct RTPHeader tRTPHeader;
    MJPEG_HEADER tMjpegHeader;
    MJPEG_RESETMARKER tMjpegResetMarker;
    MJPEG_QUANTHEADER tMjpegQuantHeader;
    int nReadPos = 0;
    unsigned int   uOffset24;
    int bMark;
    uint32_t dwSec = 0,dwUSec = 0,bAbs = 0;

    type = pRecvData[1] & 0x7F;
    if(type != m_nDefaultPayloadType)
    {
        // RTSP_ERROR("invalid Video Payloadtype = %d\n",type);
        m_bLastRecvError = 1;
        m_bFrameDealing = 0;
        return 0;
    }
    m_hMemLock.Lock();
    if (m_pRecvDataBuff == NULL)
    {
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
        }
    }
    if (m_pRecvDataBuff == NULL)
    {
        m_bFrameDealing = 0;
        m_hMemLock.Unlock();
        RTSP_ERROR("Memory malloc failed\n");
        return -1;
    }
    m_hMemLock.Unlock();
    memcpy(&tRTPHeader,pRecvData,sizeof(tRTPHeader));
    //    printf("%u\n",packet->GetTimestamp());
    //nSeq = (uint32_t)ntohs(*(short *)(&pRecvData[2]));//packet->GetExtendedSequenceNumber();
    nSeq = (pRecvData[2] << 8) | pRecvData[3];
    // nTimestamp = ntohl(*(int *)(&pRecvData[4]));//packet->GetTimestamp();
    nTimestamp = (pRecvData[4] << 24) | (pRecvData[5] << 16) | (pRecvData[6] << 8) | pRecvData[7];
    //printf("time = %d \n",nTimestamp);

    cc = tRTPHeader.csrccount;

    bExtHeader = tRTPHeader.extension;


    pdata = pRecvData + 12 + cc * 4;//(char *)packet->GetPayloadData();
    npktlen = nDataLen - 12 - cc * 4;//packet->GetPayloadLength();

    if(bExtHeader)
    {
        uint16_t length;
        RTPExtensionHeader tExtHeader;
        RTPExtensionOnvifHeader tExtOnvifHeader;
        memcpy(&tExtHeader,pdata,4);
        if (tExtHeader.extid == htons(0xABAC))
        {// onvif
            if (htons(tExtHeader.length) >= 3)
            {
                memcpy(&tExtOnvifHeader,pdata + sizeof(RTPExtensionHeader),htons(tExtHeader.length) * 4);
                bAbs = 1;
                Ants_rtsp_GetNTP2RTPTime(htonl(tExtOnvifHeader.ntpTimeStamp0),htonl(tExtOnvifHeader.ntpTimeStamp1),&dwSec,&dwUSec);
            }

        }
        // pExtHeader = (struct RTPExtensionHeader *)(pdata);
        // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();
    }
  //  printf("[%d] type = %d,time = %u,marker = %d len = %d\n",nSeq,tRTPHeader.payloadtype,nTimestamp,tRTPHeader.marker,npktlen);
    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
        m_bFrameDealing = 0;
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

    nReadPos = 0;
    memcpy(&tMjpegHeader,pdata + nReadPos,sizeof(tMjpegHeader));
    nReadPos += sizeof(tMjpegHeader);
    uOffset24 = htonl(tMjpegHeader.uOffset24) >> 8;
  //  printf("offset :%u \n",uOffset24);
    //
    if (bSeqError)
    {
        if (uOffset24 != 0)
        {// 丢掉
           // printf("***Drop***\n");
            m_bFrameDealing = 0;
            return -1;
        }
    }
    bMark = tRTPHeader.marker;
    // SOI-APP0-DRI-DQT(LUM)-DQT(Chroma)-SOF0-SOS
    if (uOffset24 == 0)
    {// SOI
        MJPEG_APP *pSOI;
        MJPEG_DRI *pDRI;
        MJPEG_DQT *pDQT;
        MJPEG_SOF *pSOF;
        MJPEG_DHT *pDHT;
        MJPEG_SOS *pSOS;
        unsigned char *pMjpegHeader;
        int nQTableNum = 0;
        int nDHTLen;
        m_uiFrameNo++;
        m_nRecvDataPos = 0;
        m_bFrameDealing = 1;
        m_nRecvDataStart = sizeof(AntsFrameHeader);
        pMjpegHeader = m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos;
        // SOI
        pMjpegHeader[0] = 0xFF;
        pMjpegHeader[1] = ANTS_RTSP_MJPEG_MARK_SOI;
        m_nRecvDataPos += 2;
        // APP
        pSOI = (MJPEG_APP *)(pMjpegHeader + m_nRecvDataPos);
        pSOI->byMark = 0xFF;
        pSOI->byType = ANTS_RTSP_MJPEG_MARK_APP0;
        pSOI->wLen = htons(sizeof(MJPEG_APP) - 2);
        pSOI->szJFIF[0] = 'J';
        pSOI->szJFIF[1] = 'F';
        pSOI->szJFIF[2] = 'I';
        pSOI->szJFIF[3] = 'F';
        pSOI->szJFIF[4] = 0;
        pSOI->byVersion[0] = 0x01;
        pSOI->byVersion[1] = 0x01;
        pSOI->byXYUnits = 0;
        pSOI->wXDensity = htons(0x01);
        pSOI->wYDensity = htons(0x01);
        pSOI->byThumbnailHoriPixels = 0;
        pSOI->byThumbnailVertPixels = 0;
        m_nRecvDataPos += sizeof(MJPEG_APP);

        // DRI
        if (tMjpegHeader.byType > 63)
        {
            memcpy(&tMjpegResetMarker,pdata + nReadPos,sizeof(tMjpegResetMarker));
            nReadPos += sizeof(tMjpegResetMarker);
            if (tMjpegResetMarker.wResetInterval > 0)
            {
                pDRI = (MJPEG_DRI *)(pMjpegHeader + m_nRecvDataPos);
                pDRI->byMark = 0xFF;
                pDRI->byType = ANTS_RTSP_MJPEG_MARK_DRI;
                pDRI->wLen = htons(sizeof(MJPEG_DRI) - 2);
                pDRI->wRi = htons(tMjpegResetMarker.wResetInterval);
                m_nRecvDataPos += sizeof(MJPEG_DRI);
            }

        }

        //DQT
        if (tMjpegHeader.byQ <= 99)
        {// 固定表,生成量化表
            unsigned char resultTables[128];
            int Qid,Qpr,Qlen,QtotalLen;
            makeDefaultQtables(resultTables,tMjpegHeader.byQ);
            QtotalLen = 128;
            for (Qid = 0; Qid < 2;Qid++)
            {
                Qpr = 0;
                Qlen = 64 * (Qpr + 1);
                if (Qlen * (Qid + 1) > QtotalLen)
                {
                    break;
                }
                pDQT = (MJPEG_DQT *)(pMjpegHeader + m_nRecvDataPos);
                pDQT->byMark = 0xFF;
                pDQT->byType = ANTS_RTSP_MJPEG_MARK_DQT;
                pDQT->wLen = htons(Qlen + 3);
                pDQT->byQT = (Qpr << 4) | Qid;
                memcpy(pDQT->byQTable,resultTables + Qlen * Qid,Qlen);
                m_nRecvDataPos += sizeof(MJPEG_DQT) + Qlen;

            }
            nQTableNum = 2;

        }
        else if (tMjpegHeader.byQ >= 128)
        { // 必须带有量化表
            int Qid,Qpr,Qlen,QtotalLen;
            unsigned char *pQtable;
            memcpy(&tMjpegQuantHeader,pdata + nReadPos,sizeof(tMjpegQuantHeader));
            nReadPos += sizeof(tMjpegQuantHeader);
            pQtable = (unsigned char *)pdata + nReadPos;
            QtotalLen = htons(tMjpegQuantHeader.wQTableLength);


            for (Qid = 0; Qid < 3;Qid++)
            {
                Qpr = (tMjpegQuantHeader.byPrecision >> Qid) & 1;
                Qlen = 64 * (Qpr + 1);
                if (Qlen * (Qid + 1) > QtotalLen)
                {
                    break;
                }
                pDQT = (MJPEG_DQT *)(pMjpegHeader + m_nRecvDataPos);
                pDQT->byMark = 0xFF;
                pDQT->byType = ANTS_RTSP_MJPEG_MARK_DQT;
                pDQT->wLen = htons(Qlen + 3);
                pDQT->byQT = (Qpr << 4) | Qid;
                memcpy(pDQT->byQTable,pQtable + Qlen * Qid,Qlen);
                m_nRecvDataPos += sizeof(MJPEG_DQT) + Qlen;
                nQTableNum++;
                if (nQTableNum >= 2)
                {// 以防有多个量化表，会导致有些通用播放器会色彩不对。
                    break;
                }

            }
            nReadPos += QtotalLen;

        }

        // SOF
        pSOF = (MJPEG_SOF *)(pMjpegHeader + m_nRecvDataPos);
        pSOF->byMark = 0xFF;
        pSOF->byType = ANTS_RTSP_MJPEG_MARK_SOF;
        pSOF->wLen = htons(sizeof(MJPEG_SOF) - 2);
        pSOF->byPrecision = 8;
        if (tMjpegHeader.byHeight == 0)
        {
            pSOF->wHeight = htons(2048);
        }
        else
        {
            pSOF->wHeight = htons(tMjpegHeader.byHeight << 3);
        }

        if (tMjpegHeader.byWidth == 0)
        {
            pSOF->wWidth = htons(2048);
        }
        else
        {
            pSOF->wWidth = htons(tMjpegHeader.byWidth << 3);
        }

        pSOF->byComponentNum = 0x03;
        pSOF->byComponents[0] = 0x01;
        pSOF->byComponents[1] = (tMjpegHeader.byType & 1)?0x22:0x21;
        pSOF->byComponents[2] = 0x00;
        pSOF->byComponents[3] = 0x02;
        pSOF->byComponents[4] = 0x11;
        pSOF->byComponents[5] = nQTableNum == 1?0x00:0x01;
        pSOF->byComponents[6] = 0x03;
        pSOF->byComponents[7] = 0x11;
        pSOF->byComponents[8] = nQTableNum == 1?0x00:0x01;
        m_nRecvDataPos += sizeof(MJPEG_SOF);

        // DHT
        // LUM DC
        pDHT = (MJPEG_DHT *)(pMjpegHeader + m_nRecvDataPos);
        pDHT->byMark = 0xFF;
        pDHT->byType = ANTS_RTSP_MJPEG_MARK_DHT;
        nDHTLen = sizeof(lum_dc_codelens) + sizeof(lum_dc_symbols) + 5;
        pDHT->wLen = htons(nDHTLen - 2);
        pDHT->byHT = (0 << 4) | 0;
        memcpy(pDHT->byIndex,lum_dc_codelens,sizeof(lum_dc_codelens));
        memcpy(pDHT->byIndex + sizeof(lum_dc_codelens),lum_dc_symbols,sizeof(lum_dc_symbols));
        m_nRecvDataPos += nDHTLen;
        // LUM AC
        pDHT = (MJPEG_DHT *)(pMjpegHeader + m_nRecvDataPos);
        pDHT->byMark = 0xFF;
        pDHT->byType = ANTS_RTSP_MJPEG_MARK_DHT;
        nDHTLen = sizeof(lum_ac_codelens) + sizeof(lum_ac_symbols) + 5;
        pDHT->wLen = htons(nDHTLen - 2);
        pDHT->byHT = (1 << 4) | 0;
        memcpy(pDHT->byIndex,lum_ac_codelens,sizeof(lum_ac_codelens));
        memcpy(pDHT->byIndex + sizeof(lum_ac_codelens),lum_ac_symbols,sizeof(lum_ac_symbols));
        m_nRecvDataPos += nDHTLen;
        // CHROMA DC
        pDHT = (MJPEG_DHT *)(pMjpegHeader + m_nRecvDataPos);
        pDHT->byMark = 0xFF;
        pDHT->byType = ANTS_RTSP_MJPEG_MARK_DHT;
        nDHTLen = sizeof(chm_dc_codelens) + sizeof(chm_dc_symbols) + 5;
        pDHT->wLen = htons(nDHTLen - 2);
        pDHT->byHT = (0 << 4) | 1;
        memcpy(pDHT->byIndex,chm_dc_codelens,sizeof(chm_dc_codelens));
        memcpy(pDHT->byIndex + sizeof(chm_dc_codelens),chm_dc_symbols,sizeof(chm_dc_symbols));
        m_nRecvDataPos += nDHTLen;
        // CHROMA AC
        pDHT = (MJPEG_DHT *)(pMjpegHeader + m_nRecvDataPos);
        pDHT->byMark = 0xFF;
        pDHT->byType = ANTS_RTSP_MJPEG_MARK_DHT;
        nDHTLen = sizeof(chm_ac_codelens) + sizeof(chm_ac_symbols) + 5;
        pDHT->wLen = htons(nDHTLen - 2);
        pDHT->byHT = (1 << 4) | 1;
        memcpy(pDHT->byIndex,chm_ac_codelens,sizeof(chm_ac_codelens));
        memcpy(pDHT->byIndex + sizeof(chm_ac_codelens),chm_ac_symbols,sizeof(chm_ac_symbols));
        m_nRecvDataPos += nDHTLen;

        // SOS
         pSOS = (MJPEG_SOS *)(pMjpegHeader + m_nRecvDataPos);
         pSOS->byMark = 0xFF;
         pSOS->byType = ANTS_RTSP_MJPEG_MARK_SOS;
         pSOS->wLen = htons(0x0c);
         pSOS->byScanNum = 0x03;
         pSOS->byComps[0] = 0x01;
         pSOS->byComps[1] = 0x00;
         pSOS->byComps[2] = 0x02;
         pSOS->byComps[3] = 0x11;
         pSOS->byComps[4] = 0x03;
         pSOS->byComps[5] = 0x11;
         pSOS->byComps[6] = 0x00;
         pSOS->byComps[7] = 0x3F;
         pSOS->byComps[8] = 0x00;
         m_nRecvDataPos += sizeof(MJPEG_SOS) + 8;



    }
    else if (!m_bFrameDealing)
    {// 丢掉
        m_nRecvDataPos = 0;
        return -1;
    }

    CurrLen = m_nRecvDataStart + m_nRecvDataPos + npktlen - nReadPos + 2;
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bLastRecvError = 1;
            m_bFrameDealing = 0;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp JPEG]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bLastRecvError = 1;
                m_bFrameDealing = 0;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp JPEG]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bLastRecvError = 1;
                    m_bFrameDealing = 0;
                    m_hMemLock.Unlock();
                    return -1;
                }
               // memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
               // pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
               // free(pDel);
                break;
            }
            x++;
        }
    }
    m_hMemLock.Unlock();
    if (uOffset24 == 0)
    {
        memcpy(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos,pdata + nReadPos,npktlen - nReadPos);
        m_nRecvDataPos += npktlen - nReadPos;
    }
    else
    {


        if (tMjpegHeader.byType > 63)
        {
            memcpy(&tMjpegResetMarker,pdata + nReadPos,sizeof(tMjpegResetMarker));
            nReadPos += sizeof(tMjpegResetMarker);

        }
        memcpy(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos,pdata + nReadPos,npktlen - nReadPos);
        m_nRecvDataPos += npktlen - nReadPos;
    }



    if (bMark)
    {// 完成

        unsigned char *pEOI;
        pEOI = (m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
        if(pEOI[-1] != ANTS_RTSP_MJPEG_MARK_EOI || pEOI[-2] != 0xFF)
        {
            pEOI[0] = 0xFF;
            pEOI[1] = ANTS_RTSP_MJPEG_MARK_EOI;
            m_nRecvDataPos += 2;
        }

#if 0
        {
            static FILE *fp_save = NULL;
            if (fp_save == NULL)
            {
                fp_save = fopen("wTest.jpg","wb+");
                if (fp_save)
                {
                    fwrite(m_pRecvDataBuff + m_nRecvDataStart,1,m_nRecvDataPos,fp_save);
                    fclose(fp_save);
                }
            }
        }
#endif
        pFrame = (AntsFrameHeader *)m_pRecvDataBuff;
        memset(pFrame,0,sizeof(AntsFrameHeader));

        m_nWidth = tMjpegHeader.byWidth << 3;
        m_nHeight = tMjpegHeader.byHeight << 3;
        if (m_nWidth == 0)
        {
            m_nWidth = 256 * 8;
        }
        if (m_nHeight == 0)
        {
            m_nHeight = 256 *8;
        }

        m_nLastTimestamp = nTimestamp;
       // SOI-APP0-DRI-DQT(LUM)-DQT(Chroma)-SOF0-SOS
        pFrame->uiStartId = ANTS_FRAME_STARTCODE;
        pFrame->uiFrameType = AntsPktIFrames;
        pFrame->uiFrameNo = m_uiFrameNo;
        if (bAbs)
        {
            pFrame->uiFrameTime = dwSec;
            pFrame->uiFrameTickCount = dwUSec;
        }
        else
        {
            Tmptimestamp = m_nLastTimestamp / GetPayloadClockRate();
            pFrame->uiFrameTime = Tmptimestamp;
            Tmptimestamp = (m_nLastTimestamp * 1000 * 1000/ GetPayloadClockRate()  - pFrame->uiFrameTime * 1000* 1000);
            pFrame->uiFrameTickCount = Tmptimestamp;
        }
        pFrame->uiFrameLen = m_nRecvDataPos;
        Tmptimestamp = m_nLastTimestamp * 90000.0/GetPayloadClockRate();
        pFrame->uiTimeStamp = Tmptimestamp;
        pFrame->uMedia.struVideoHeader.cCodecId = AntsMJPEG_hisi;
        pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
        pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
        pFrame->uMedia.struVideoHeader.cColorSpace = (tMjpegHeader.byType&1) ? 0:1; //
        if (m_pRtpSessionClientFxn != NULL)
        {
            int prop = 0;
            if(m_dwProp & 1)
                prop = 1;

            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
            if (prop)
            {
                m_pRecvDataBuff = NULL;
            }
        }

        m_nRecvDataPos = 0;
        m_bFrameDealing = 0;
        return 1;
    }

    return 0;
}

int CH264RtpSession::DealBdRecvBuff(unsigned char *pdata, int nDataLen)
{
	if (pdata == NULL)
	{
	    m_bPsLastRecvError = 1;
	    return -1;
	}

	if (nDataLen == 0)
	{
	    return 0;
	}

    m_nBdRecvDataPos += nDataLen;

	return 0;
}

int CH264RtpSession::DealRecvBuff(unsigned char *pdata, int nDataLen)
{
    int CurrLen;
    unsigned char *pDstData = NULL;

	if (pdata == NULL)
	{
	    m_bPsLastRecvError = 1;
	    return -1;
	}

	if (nDataLen == 0)
	{
	    return 0;
	}

    CurrLen = m_nRecvDataStart + m_nRecvDataPos + nDataLen;
	//printf("CurrLen = %d m_nRecvDataBuffSize = %d!!!!!!!!!!!\n", CurrLen, m_nRecvDataBuffSize);
	//printf("m_nRecvDataStart = %d m_nRecvDataPos = %d nDataLen = %d!!!!!!!!!!!\n", m_nRecvDataStart, m_nRecvDataPos, nDataLen);
    m_hMemLock.Lock();
    if (CurrLen > m_nRecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bPsLastRecvError = 1;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bPsLastRecvError = 1;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pRecvDataBuff,m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bPsLastRecvError = 1;
                    m_hMemLock.Unlock();
                    return -1;
                }

                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pRecvDataBuff = (uint8_t *)pTemp;
                m_nRecvDataBuffSize = m_nRecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
				//printf("m_pRecvDataBuff4 = %p nDataLen = %d!!!!!!!!!!!\n", m_pRecvDataBuff, nDataLen);
                // free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();

	pDstData = (unsigned char *)(m_pRecvDataBuff + m_nRecvDataStart + m_nRecvDataPos);
	if (m_Type == 1)
	{
        //printf("m_nRecvDataStart = %d m_nRecvDataPos = %d nDataLen = %02x%02x%02x%02x!!!!!!!!!!!\n", m_nRecvDataStart, m_nRecvDataPos, m_pRecvDataBuff[0], m_pRecvDataBuff[1], m_pRecvDataBuff[2], m_pRecvDataBuff[3]);
	}
	memcpy(pDstData, pdata, nDataLen);
    m_nRecvDataPos += nDataLen;
	if (m_Type == 1)
	{
       // printf("m_nRecvDataStart1 = %d m_nRecvDataPos = %d nDataLen = %02x%02x%02x%02x!!!!!!!!!!!\n", m_nRecvDataStart, m_nRecvDataPos, m_pRecvDataBuff[0], m_pRecvDataBuff[1], m_pRecvDataBuff[2], m_pRecvDataBuff[3]);
	}
	return 0;
}

int CH264RtpSession::DealAudioRecvBuff(unsigned char *pdata, int nDataLen)
{
    int CurrLen;
    unsigned char *pDstData = NULL;

	if (pdata == NULL)
	{
	    m_bPsLastRecvError = 1;
	    return -1;
	}

	if (nDataLen == 0)
	{
	    return 0;
	}

    CurrLen = m_nARecvDataStart + m_nARecvDataPos + nDataLen;
	//printf("CurrLen = %d m_nRecvDataBuffSize = %d!!!!!!!!!!!\n", CurrLen, m_nRecvDataBuffSize);
	//printf("m_nRecvDataStart = %d m_nRecvDataPos = %d nDataLen = %d!!!!!!!!!!!\n", m_nRecvDataStart, m_nRecvDataPos, nDataLen);
    m_hMemLock.Lock();
    if (CurrLen > m_nARecvDataBuffSize)
    {
        int x = 1;
        if (CurrLen > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
        {
            m_bPsLastRecvError = 1;
            m_hMemLock.Unlock();
            RTSP_DEBUG("[rtp264]line = %d CurrLen = %d\n",__LINE__,CurrLen);
            return -1;
        }

        while(1)
        {
            if (m_nARecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC > H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X)
            {
                m_bPsLastRecvError = 1;
                m_hMemLock.Unlock();
                RTSP_DEBUG("[rtp264]line = %d too long\n",__LINE__);
                return -1;
            }
            if (CurrLen <= m_nARecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC)
            {
                char *pTemp,*pDel;
                //开始分配
                pTemp = (char *)realloc(m_pARecvDataBuff,m_nARecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC);
                if (pTemp == NULL)
                {
                    m_bPsLastRecvError = 1;
                    m_hMemLock.Unlock();
                    return -1;
                }

                //memcpy(pTemp,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos);
                //pDel = (char *)m_pRecvDataBuff;
                m_pARecvDataBuff = (uint8_t *)pTemp;
                m_nARecvDataBuffSize = m_nARecvDataBuffSize + x * H264RTPSESSION_RECV_BUFFSIZE_INC;
				//printf("m_pRecvDataBuff4 = %p nDataLen = %d!!!!!!!!!!!\n", m_pRecvDataBuff, nDataLen);
                // free(pDel);
                break;
            }
            x++;
        }



    }
    m_hMemLock.Unlock();

	pDstData = (unsigned char *)(m_pARecvDataBuff + m_nARecvDataStart + m_nARecvDataPos);
	if (m_Type == 1)
	{
        //printf("m_nRecvDataStart = %d m_nRecvDataPos = %d nDataLen = %02x%02x%02x%02x!!!!!!!!!!!\n", m_nRecvDataStart, m_nRecvDataPos, m_pRecvDataBuff[0], m_pRecvDataBuff[1], m_pRecvDataBuff[2], m_pRecvDataBuff[3]);
	}
	memcpy(pDstData, pdata, nDataLen);
    m_nARecvDataPos += nDataLen;
	if (m_Type == 1)
	{
       // printf("m_nRecvDataStart1 = %d m_nRecvDataPos = %d nDataLen = %02x%02x%02x%02x!!!!!!!!!!!\n", m_nRecvDataStart, m_nRecvDataPos, m_pRecvDataBuff[0], m_pRecvDataBuff[1], m_pRecvDataBuff[2], m_pRecvDataBuff[3]);
	}
	return 0;
}

int CH264RtpSession::DealAntsFrameHeader(unsigned int bAudio, unsigned int nTimestamp)
{
    AntsFrameHeader *pFrame;
    uint64_t Tmptimestamp;
    unsigned int PayloadClockRate;

	if (bAudio != 1)
	{
	    pFrame = (AntsFrameHeader *)m_pRecvDataBuff;
	}
	else
	{
	    pFrame = (AntsFrameHeader *)m_pARecvDataBuff;
	}

    memset(pFrame, 0, sizeof(AntsFrameHeader));
    pFrame->uiStartId = ANTS_FRAME_STARTCODE;

	m_nLastTimestamp = nTimestamp;
	if (bAudio != 1)
	{
        pFrame->uMedia.struVideoHeader.cCodecId = m_VideoCodecId;
	    PayloadClockRate = 90000;
        Tmptimestamp = m_nLastTimestamp* 90000.0 / PayloadClockRate;
        pFrame->uiTimeStamp = Tmptimestamp;


		m_uiFrameNo++;
        pFrame->uiFrameNo = m_uiFrameNo;
		pFrame->uiFrameLen = m_nRecvDataPos;

	    if (m_bIFrame == 1)
	    {
		    pFrame->uiFrameType = AntsPktIFrames;
	        if(ReadSPS(m_pRecvDataBuff + sizeof(AntsFrameHeader),m_nRecvDataPos,1))
	        {
	            //printf("m_nWidth = %d, m_nHeight = %d\n", m_nWidth, m_nHeight);
	           m_bPsLastRecvError = 1;
               return -1;

	        }
		}
		else
		{
		    pFrame->uiFrameType = AntsPktPFrames;

        }
        if(m_nWidth == 0 || m_nHeight == 0)
        {
            m_nWidth = 1920;
            m_nHeight = 1080;
        }
        pFrame->uMedia.struVideoHeader.usWidth = m_nWidth;
        pFrame->uMedia.struVideoHeader.usHeight = m_nHeight;
	}
	else
	{
		pFrame->uiFrameType = AntsPktAudioFrames;
        pFrame->uMedia.struAudioHeader.cCodecId = m_AudioCodecId;
		PayloadClockRate = 8000;
        Tmptimestamp = m_nLastTimestamp* 8000.0 / PayloadClockRate;
        pFrame->uiTimeStamp = Tmptimestamp;

		m_uiAFrameNo++;
        pFrame->uiFrameNo = m_uiAFrameNo;
		pFrame->uiFrameLen = m_nARecvDataPos;
	}

    Tmptimestamp = m_nLastTimestamp / PayloadClockRate;
    pFrame->uiFrameTime = Tmptimestamp;
    Tmptimestamp = (m_nLastTimestamp * 1000.0 * 1000/ PayloadClockRate  - pFrame->uiFrameTime * 1000* 1000);
    pFrame->uiFrameTickCount = Tmptimestamp;

	return 0;
}


int CH264RtpSession::DealAntsG711Frame(int dwProp)
{
    AntsFrameHeader *pFrame;
    uint64_t Tmptimestamp, TmpFrameTime;
	int len = 0, pos = 0;
    unsigned int PayloadClockRate;

	len  = m_nARecvDataPos;
	pFrame = (AntsFrameHeader *)m_pARecvDataBuff;
    Tmptimestamp = pFrame->uiTimeStamp;
	PayloadClockRate = 8000;

    while (1)
    {
        if (len < 320)
        {
            break;
		}

		m_hMemLock.Lock();

		if (m_pAG711Buff == NULL)
	    {
	        m_pAG711Buff = (uint8_t *)malloc(sizeof(AntsFrameHeader) + 320 + 4);
	    }

	    if (m_pAG711Buff == NULL)
	    {
	        //m_bLastRecvError=1;
	        m_hMemLock.Unlock();
	        RTSP_ERROR("Memory malloc failed\n");
	        return -1;
	    }

		m_hMemLock.Unlock();

        memcpy(m_pAG711Buff, m_pARecvDataBuff, m_nARecvDataStart);
		pFrame = (AntsFrameHeader *)m_pAG711Buff;

        pFrame->uiFrameNo = m_uiAFrameNo;
		m_uiAFrameNo++;
		pFrame->uiFrameLen = 320;
		pFrame->uiTimeStamp = Tmptimestamp;

	    TmpFrameTime = m_nLastTimestamp / PayloadClockRate;
	    pFrame->uiFrameTime = TmpFrameTime;
	    TmpFrameTime = (m_nLastTimestamp * 1000.0 * 1000/ PayloadClockRate  - pFrame->uiFrameTime * 1000* 1000);
	    pFrame->uiFrameTickCount = TmpFrameTime;


		memcpy(m_pAG711Buff + sizeof(AntsFrameHeader), m_pARecvDataBuff + m_nARecvDataStart + pos, 320);

	    //printf("len = %d pos = %d pFrame->uiFrameLen = %d pFrame->uiFrameNo = %d!!!!!!!!!!!\n", len, pos, pFrame->uiFrameLen, pFrame->uiFrameNo);
		m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,dwProp,pFrame->uiFrameType,m_pAG711Buff,sizeof(AntsFrameHeader) + 320,m_pRtpSessionClientUser);

		if (dwProp)
        {
            m_pAG711Buff = NULL;
        }

	    len -= 320;
		pos += 320;
		Tmptimestamp += 320;
		m_nLastTimestamp = Tmptimestamp * PayloadClockRate / 8000.0;
	}

    if (len > 0)
    {
		m_hMemLock.Lock();

		if (m_pAG711Buff == NULL)
	    {
	        m_pAG711Buff = (uint8_t *)malloc(sizeof(AntsFrameHeader) + 320 + 4);
	    }

	    if (m_pAG711Buff == NULL)
	    {
	        //m_bLastRecvError=1;
	        m_hMemLock.Unlock();
	        RTSP_ERROR("Memory malloc failed\n");
	        return -1;
	    }

		m_hMemLock.Unlock();

		memcpy(m_pAG711Buff, m_pARecvDataBuff, m_nARecvDataStart);
		pFrame = (AntsFrameHeader *)m_pAG711Buff;

	    pFrame->uiFrameNo = m_uiAFrameNo;
		m_uiAFrameNo++;
		pFrame->uiFrameLen = len;
		pFrame->uiTimeStamp = Tmptimestamp;

	    TmpFrameTime = m_nLastTimestamp / PayloadClockRate;
	    pFrame->uiFrameTime = TmpFrameTime;
	    TmpFrameTime = (m_nLastTimestamp * 1000.0 * 1000/ PayloadClockRate  - pFrame->uiFrameTime * 1000* 1000);
	    pFrame->uiFrameTickCount = TmpFrameTime;

		memcpy(m_pAG711Buff + sizeof(AntsFrameHeader), m_pARecvDataBuff + m_nARecvDataStart + pos, len);
	    printf("len = %d pos = %d pFrame->uiFrameLen = %d pFrame->uiFrameNo = %d!!!!!!!!!!!\n", len, pos, pFrame->uiFrameLen, pFrame->uiFrameNo);
		m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,dwProp,pFrame->uiFrameType,m_pAG711Buff,sizeof(AntsFrameHeader) + len,m_pRtpSessionClientUser);

		if (dwProp)
	    {
	        m_pAG711Buff = NULL;
	    }

		Tmptimestamp += len;
		m_nLastTimestamp = Tmptimestamp * PayloadClockRate / 8000.0;
    }

	m_uiAFrameNo--;

	return 0;
}

int CH264RtpSession::DealPsHead(unsigned char *pCurData, int nDataLen, int bMark, unsigned int nTimestamp, int flag)
{
    int bFrame = 0;
    int npktlen;
	uint16_t resLen = 0, PsmResLen = 0, MapLen = 0, TempLen = 0;
    unsigned char *pdata = NULL;
    AntsFrameHeader *pFrame;
	unsigned int uiFrameType;
	unsigned int cCodecId;
    uint64_t Tmptimestamp;
	unsigned int PayloadClockRate;

	if (flag == 0)
	{
		if (NULL == pCurData || sizeof(ps_header_t) > nDataLen)
		{
	        return -1;
		}

		m_nRecvDataStart = sizeof(AntsFrameHeader);

		pdata = pCurData;

		resLen = pdata[13] & 0x07;
		pdata += sizeof(ps_header_t) + resLen;
		npktlen = nDataLen - (sizeof(ps_header_t) + resLen);
	}
	else if (flag == 1)
	{
		pdata = pCurData;
		resLen = 0;
		npktlen = nDataLen;
	}

	if (6 <= npktlen && is_sh_header((sh_header_t *)pdata))
	{
	    resLen = (pdata[4] << 8) | (pdata[5]);
		pdata += 6 + resLen;
	    npktlen -= 6 + resLen;

		if (6 <= npktlen && is_psm_header((psm_header_t *)pdata))
		{
	        resLen = (pdata[4] << 8) | (pdata[5]);

			if (10 <= resLen)
			{
			    PsmResLen = (pdata[8] << 8) | (pdata[9]);

				if (10 + PsmResLen <= resLen)
				{
					MapLen = (pdata[10 + PsmResLen] << 8) | (pdata[11 + PsmResLen]);
				}

				if (10 + PsmResLen + MapLen <= resLen)
				{
					while (1)
					{
					    if (TempLen >= MapLen)
					    {
	                        break;
						}

		                if (0xC0 == pdata[13 + PsmResLen + TempLen])
		                {
		                    //uiFrameType = AntsPktAudioFrames;
		                    if (0x90 == pdata[12 + PsmResLen + TempLen])
		                    {
		                        m_AudioCodecId = AntsG711A;
							}
						}
						else if (0xE0 == pdata[13 + PsmResLen + TempLen])
						{
						    //uiFrameType = AntsPktIFrames;
							m_Type = 1;
						    //printf("m_VideoCodecId2 = 0x%02x 0x%02x  0x%02x %d!!!!!!!!!!!\n", pdata[12], pdata[15], pdata[28], resLen);
		                    if (0x1B == pdata[12 + PsmResLen + TempLen])
		                    {
		                        m_VideoCodecId = AntsH264_hisi_RTP;
							}
							else if (0x10 == pdata[12 + PsmResLen + TempLen])
							{
		                        m_VideoCodecId = AntsMJPEG_hisi;
							}
							else if (0x80 == pdata[12 + PsmResLen + TempLen])
							{
		                        m_VideoCodecId = AntsSVAC;
							}
						}

						TempLen += (pdata[14 + PsmResLen + TempLen] << 8) | (pdata[15 + PsmResLen + TempLen]) + 4;
					}
				}
			}

			pdata += 6 + resLen;
	        npktlen -= 6 + resLen;

			if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_header((pes_header_t *)pdata))
			{
			    resLen = pdata[8] & 0xff;

				if (0xC0 == pdata[3])
			    {
				    uiFrameType = AntsPktAudioFrames;
			        m_PesLenAudio = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
					m_bAudio = 1;
				}
				else if (0xE0 == pdata[3])
				{
				    uiFrameType = AntsPktIFrames;
			        m_PesLen = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
					m_bAudio = 0;
				}
				else if (0xBD == pdata[3])
				{
				     m_PesLenBd = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
		             m_bAudio = 2;
				}

                //printf("pdata1 %02x %02x!!!!!!!!!!!\n", pdata[4], pdata[5]);
				pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
		        npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
				//printf("npktlen2 = %d m_PesLen = %d!!!!!!!!!!!\n", npktlen, m_PesLen);
			}
			else
			{
                m_bPsLastRecvError = 1;
                return -1;
			}
		}
        else if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_header((pes_header_t *)pdata))
		{
		    resLen = pdata[8] & 0xff;

			if (0xC0 == pdata[3])
		    {
			    uiFrameType = AntsPktAudioFrames;
		        m_PesLenAudio= (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
				m_bAudio = 1;
			}
			else if (0xE0 == pdata[3])
			{
			    uiFrameType = AntsPktIFrames;
		        m_PesLen = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
				m_bAudio = 0;
			}
			else if (0xBD == pdata[3])
			{
				 m_PesLenBd = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
	             m_bAudio = 2;
			}

			pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
	        npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
		}
		else
		{
            m_bPsLastRecvError = 1;
            return -1;
		}

		m_bIFrame = 1;
	}
	else if (6 <= npktlen && is_psm_header((psm_header_t *)pdata))
	{
	    resLen = (pdata[4] << 8) | (pdata[5]);
		if (10 <= resLen)
		{
		    PsmResLen = (pdata[8] << 8) | (pdata[9]);

			if (10 + PsmResLen <= resLen)
			{
				MapLen = (pdata[10 + PsmResLen] << 8) | (pdata[11 + PsmResLen]);
			}

			if (10 + PsmResLen + MapLen <= resLen)
			{
				while (1)
				{
				    if (TempLen >= MapLen)
				    {
	                    break;
					}

	                if (0xC0 == pdata[13 + PsmResLen + TempLen])
	                {
	                    //uiFrameType = AntsPktAudioFrames;
	                    if (0x90 == pdata[12 + PsmResLen + TempLen])
	                    {
	                        m_AudioCodecId = AntsG711A;
						}
					}
					else if (0xE0 == pdata[13 + PsmResLen + TempLen])
					{
					    //uiFrameType = AntsPktIFrames;
						m_Type = 1;
					    //printf("m_VideoCodecId2 = 0x%02x 0x%02x  0x%02x %d!!!!!!!!!!!\n", pdata[12], pdata[15], pdata[28], resLen);
	                    if (0x1B == pdata[12 + PsmResLen + TempLen])
	                    {
	                        m_VideoCodecId = AntsH264_hisi_RTP;
						}
						else if (0x10 == pdata[12 + PsmResLen + TempLen])
						{
	                        m_VideoCodecId = AntsMJPEG_hisi;
						}
						else if (0x80 == pdata[12 + PsmResLen + TempLen])
						{
	                        m_VideoCodecId = AntsSVAC;
						}
					}

					TempLen += (pdata[14 + PsmResLen + TempLen] << 8) | (pdata[15 + PsmResLen + TempLen]) + 4;
				}
			}
		}
		pdata += 6 + resLen;
	    npktlen -= 6 + resLen;

		if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_header((pes_header_t *)pdata))
		{
		    resLen = pdata[8] & 0xff;

			if (0xC0 == pdata[3])
		    {
			    uiFrameType = AntsPktAudioFrames;
		        m_PesLenAudio= (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
				m_bAudio = 1;
			}
			else if (0xE0 == pdata[3])
			{
			    uiFrameType = AntsPktPFrames;
		        m_PesLen = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
				m_bAudio = 0;
			}
			else if (0xBD == pdata[3])
			{
				 m_PesLenBd = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
	             m_bAudio = 2;
			}

			pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
	        npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
		}
		else
		{
            m_bPsLastRecvError = 1;
            return -1;
		}
	}
	else if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_header((pes_header_t *)pdata))
	{
	    resLen = pdata[8] & 0xff;

		if (0xC0 == pdata[3])
	    {
		    uiFrameType = AntsPktAudioFrames;
	        m_PesLenAudio = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
			m_bAudio = 1;
		}
		else if (0xE0 == pdata[3])
		{
		    uiFrameType = AntsPktPFrames;
	        m_PesLen = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
            m_bAudio = 0;
		}
		else if (0xBD == pdata[3])
		{
		     m_PesLenBd = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
             m_bAudio = 2;
		}

       //printf("pdata1 %02x %02x!!!!!!!!!!!\n", pdata[4], pdata[5]);
		pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
	    npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
       //printf("npktlen1 = %d m_PesLen = %d resLen = %d!!!!!!!!!!!\n", npktlen, m_PesLen, resLen);
	}
	else
	{
        m_bPsLastRecvError = 1;
        return -1;
	}

#if 0
    if (m_PesLen >= npktlen)
    {
		if (1 == bMark)
		{
			if (uiFrameType != AntsPktAudioFrames)
			{
		        DealRecvBuff(pdata, npktlen);
				m_bAudio = 0;
			}
			else
			{
			    DealAudioRecvBuff(pdata, npktlen);
				m_bAudio = 1;
			}

			if (1 == m_bPsLastRecvError)
			{
	            return -1;
			}

			if (m_nRecvDataPos > 0)
			{
			    if (m_pRtpSessionClientFxn != NULL)
			    {
			        int prop = 0;
			        if(m_dwProp & 1)
			            prop = 1;

					DealAntsFrameHeader(0, nTimestamp);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
		            pFrame = (AntsFrameHeader *)m_pRecvDataBuff;

			        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);

					if (prop)
			        {
			            m_pRecvDataBuff = NULL;
			        }
			    }

				m_nRecvDataPos = 0;
			    bFrame = 1;
			}

			if (m_nARecvDataPos > 0)
			{
			    if (m_pRtpSessionClientFxn != NULL)
			    {
			        int prop = 0;
			        if(m_dwProp & 1)
			            prop = 1;

					DealAntsFrameHeader(1, nTimestamp);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
		            pFrame = (AntsFrameHeader *)m_pARecvDataBuff;
	       //printf("m_nARecvDataPos1 = %d!!!!!!!!!!!\n", m_nARecvDataPos);
		            if (m_AudioCodecId == AntsG711A && m_nARecvDataPos > 320)
		            {
	                    DealAntsG711Frame(prop);
					}
					else
					{
				        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,prop,pFrame->uiFrameType,m_pARecvDataBuff,m_nARecvDataStart + m_nARecvDataPos,m_pRtpSessionClientUser);

						if (prop)
				        {
				            m_pARecvDataBuff = NULL;
				        }
					}
			    }

				m_nARecvDataPos = 0;
			    bFrame = 1;
			}

			m_status = ps_padding;

			return bFrame;
		}
		else if (0 == bMark)
		{
			if (uiFrameType != AntsPktAudioFrames)
			{
		        DealRecvBuff(pdata, npktlen);
				m_bAudio = 0;
			}
			else
			{
			    DealAudioRecvBuff(pdata, npktlen);
			    m_bAudio = 1;
			}

			if (1 == m_bPsLastRecvError)
			{
	            return -1;
			}

		    m_status = ps_ps;
		}
    }
	else
#endif
	{
		if (m_bAudio != 0 && m_bAudio != 1 && m_bAudio != 2)
		{
	        m_bPsLastRecvError = 1;
	        return -1;
		}

	    int PesLen = 0, PesLenAudio = 0, PesLenBd = 0;

		PesLen = m_PesLen;
		PesLenAudio = m_PesLenAudio;
		PesLenBd = m_PesLenBd;

        while(1)
        {
			if (m_bAudio == 0)
			{
			    if (npktlen > PesLen)
			    {
			        DealRecvBuff(pdata, PesLen);
					pdata += PesLen;
					npktlen -= PesLen;
			    }
				else
				{
			        DealRecvBuff(pdata, npktlen);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
					m_status = ps_ps;
					break;
				}
			}
			else if (m_bAudio == 1)
			{
			    if (npktlen > PesLenAudio)
			    {
				    DealAudioRecvBuff(pdata, PesLenAudio);
					pdata += PesLenAudio;
					npktlen -= PesLenAudio;
			    }
				else
				{
				    DealAudioRecvBuff(pdata, npktlen);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
				    m_status = ps_ps;
                    break;
				}
			}
			else if (m_bAudio == 2)
			{
			    if (npktlen > PesLenBd)
			    {
				    DealBdRecvBuff(pdata, PesLenBd);
					pdata += PesLenBd;
					npktlen -= PesLenBd;
			    }
				else
				{
				    DealBdRecvBuff(pdata, npktlen);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
				    m_status = ps_ps;
                    break;
				}
			}
			else
			{
		        m_bPsLastRecvError = 1;
		        return -1;
			}

			if (npktlen == 0)
			{
			    m_status = ps_ps;
                break;
			}

			if (1 == m_bPsLastRecvError)
			{
	            return -1;
			}

			if (npktlen < sizeof(pes_header_t) + sizeof(optional_pes_header_t))
			{
				memcpy(m_PesHead, pdata, npktlen);
				m_PesHeadLen = npktlen;
				m_status = ps_ps;
				break;
			}
			else
			{
			    if (is_pes_header((pes_header_t *)pdata))
				{
				    resLen = pdata[8] & 0xff;

					if (npktlen < sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen)
					{
						memcpy(m_PesHead, pdata, npktlen);
						m_PesHeadLen = npktlen;
						m_status = ps_ps;
						break;
					}
					else
					{
						if (0xC0 == pdata[3])
					    {
						    uiFrameType = AntsPktAudioFrames;
					        PesLenAudio = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
							m_PesLenAudio += PesLenAudio;
							m_bAudio = 1;
						}
						else if (0xE0 == pdata[3])
						{
						    uiFrameType = AntsPktPFrames;
							PesLen = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
					        m_PesLen += PesLen;
							m_bAudio = 0;
						}
						else if (0xBD == pdata[3])
						{
 							PesLenBd = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
					        m_PesLenBd += PesLenBd;
							m_bAudio = 2;
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}

						pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
					    npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
					}
				}
				else
				{
					m_bPsLastRecvError = 1;
					return -1;
				}
			}
		}

		if (1 == bMark)
		{
			if (m_nRecvDataPos > 0)
			{
			    if (m_pRtpSessionClientFxn != NULL)
			    {
			        int prop = 0;
			        if(m_dwProp & 1)
			            prop = 1;

					DealAntsFrameHeader(0, nTimestamp);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
		            pFrame = (AntsFrameHeader *)m_pRecvDataBuff;

			        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);

					if (prop)
			        {
			            m_pRecvDataBuff = NULL;
			        }
			    }

				m_nRecvDataPos = 0;
			    bFrame = 1;
			}

			if (m_nARecvDataPos > 0)
			{
			    if (m_pRtpSessionClientFxn != NULL)
			    {
			        int prop = 0;
			        if(m_dwProp & 1)
			            prop = 1;

					DealAntsFrameHeader(1, nTimestamp);
	                if (1 == m_bPsLastRecvError)
	                {
	                    return -1;
	                }
		            pFrame = (AntsFrameHeader *)m_pARecvDataBuff;
	       //printf("m_nARecvDataPos1 = %d!!!!!!!!!!!\n", m_nARecvDataPos);
		            if (m_AudioCodecId == AntsG711A && m_nARecvDataPos > 320)
		            {
	                    DealAntsG711Frame(prop);
					}
					else
					{
				        m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,prop,pFrame->uiFrameType,m_pARecvDataBuff,m_nARecvDataStart + m_nARecvDataPos,m_pRtpSessionClientUser);

						if (prop)
				        {
				            m_pARecvDataBuff = NULL;
				        }
					}
			    }

				m_nARecvDataPos = 0;
			    bFrame = 1;
			}

			if (m_nBdRecvDataPos > 0)
			{
                m_nBdRecvDataPos = 0;
				bFrame = 1;
			}

			m_status = ps_padding;

			return bFrame;
		}
	}

    return 0;
}

int CH264RtpSession::DealRecvPSPacket(unsigned char *pRecvData,int nDataLen)
{
    unsigned char *pdata = NULL,*pDstData = NULL;
    int npktlen;
    AntsFrameHeader *pFrame;
    unsigned short nSeq,nReqSeq;
    unsigned int nTimestamp;
    int bSeqError = 0;
	int bNeedLost = 0;
    int CurrLen;
    int cc;
    int nRet;
    int bFrame = 0;
    int type;
    int bMark = 0;
    int version;
	uint16_t resLen = 0;
	uint16_t leftLen = 0;
    uint64_t Tmptimestamp;
    int bExtHeader = 0;//
    struct RTPExtensionHeader *pExtHeader = NULL;
    uint32_t dwSec = 0,dwUSec = 0,bAbs = 0;
    uint32_t ssrc;
    //printf("len = %d\n",nDataLen);
    version = (pRecvData[0] >> 6)&0x3;
    if(version != 2)
    {
        return 0;
    }

	ssrc = (pRecvData[8] << 24) | (pRecvData[9] << 16) | (pRecvData[10] << 8) | pRecvData[11];
    if (ssrc != GetSSRC())
    {
        //m_bLastRecvError = 1;
        //RTSP_ERROR("[%s.%d]packet length ssrc %d m_uiSSRC = %d\n",__FUNCTION__,__LINE__,ssrc,GetSSRC());
        return 0;
	}

    type = pRecvData[1] & 0x7F;
    bMark = (pRecvData[1] >> 7) & 1;
    if(type != m_nDefaultPayloadType)
    {
    	m_status = ps_padding;
		m_Type = 0;
		m_PesHeadLen = 0;
		m_PesLen = 0;
		m_PesLenAudio = 0;
        m_nRecvDataPos = 0;
        m_bFrameDealing = 0;
		m_bIFrame = 0;
		m_bAudio = -1;
        if (m_pRecvDataBuff)
        {
            free(m_pRecvDataBuff);
        }
        m_pRecvDataBuff = NULL;
        m_nRecvDataBuffSize = 0;
        if (m_pARecvDataBuff)
        {
            free(m_pARecvDataBuff);
        }
        m_pARecvDataBuff = NULL;
        m_nARecvDataBuffSize = 0;
	    m_nARecvDataPos = 0;
		m_PesLenBd = 0;
		m_nBdRecvDataPos = 0;
        // RTSP_ERROR("invalid Video Payloadtype = %d\n",type);
        RTSP_ERROR("[%s.%d]invalid Video Payloadtype = %d  [%d]\n",__FUNCTION__,__LINE__,type,m_nDefaultPayloadType);
        if (m_pRtpSessionClientFxn)
        {
            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_ERROR,ANTS_RTSP_CALLBACKBYPE_ERROR_PAYLAODTYPE,0,type,NULL,0,m_pRtpSessionClientUser);
        }
        return 0;
    }
    m_hMemLock.Lock();
    if (m_bPsLastRecvError)
    {
    	m_status = ps_padding;
		m_Type = 0;
        m_PesHeadLen = 0;
		m_PesLen = 0;
        m_PesLenAudio = 0;
        m_bPsLastRecvError = 0;
        m_bFrameDealing = 0;
        m_nRecvDataPos = 0;
		m_bIFrame = 0;
		m_bAudio = -1;
        if (m_pRecvDataBuff)
        {
            free(m_pRecvDataBuff);
        }
        m_pRecvDataBuff = NULL;
        m_nRecvDataBuffSize = 0;
        if (m_pARecvDataBuff)
        {
            free(m_pARecvDataBuff);
        }
        m_pARecvDataBuff = NULL;
        m_nARecvDataBuffSize = 0;
	    m_nARecvDataPos = 0;
		m_PesLenBd = 0;
		m_nBdRecvDataPos = 0;
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
        m_pRecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pRecvDataBuff != NULL)
        {
            m_nRecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
        }
    }
    if (m_pRecvDataBuff == NULL)
    {
        m_bPsLastRecvError=1;
        m_hMemLock.Unlock();
        RTSP_ERROR("Memory malloc failed\n");
        return -1;
    }
    if (m_pARecvDataBuff == NULL)
    {
        m_pARecvDataBuff = (uint8_t *)malloc(H264RTPSESSION_RECV_BUFFSIZE);
        if (m_pARecvDataBuff != NULL)
        {
            m_nARecvDataBuffSize = H264RTPSESSION_RECV_BUFFSIZE;
        }
	    m_nARecvDataPos = 0;
	    m_nARecvDataStart = 0;
	    m_nARecvDataStart = sizeof(AntsFrameHeader);
    }
    if (m_pARecvDataBuff == NULL)
    {
        m_bPsLastRecvError=1;
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
        uint16_t length;
        // pExtHeader = (struct RTPExtensionHeader *)(pdata);
        // pdata += 4 + ntohs(pExtHeader->length) * 4;//(char *)packet->GetPayloadData();
        // npktlen -= 4 + ntohs(pExtHeader->length) * 4;//packet->GetPayloadLength();
        length = (pdata[2] << 8) | (pdata[3]);
        pdata += 4 + length * 4;//(char *)packet->GetPayloadData();
        npktlen -= 4 + length * 4;//packet->GetPayloadLength();

    }

    if (npktlen <= 0 || npktlen > H264RTPSESSION_RECV_BUFFSIZE_INC * H264RTPSESSION_RECV_BUFFSIZE_X)
    {
        m_bPsLastRecvError = 1;
        RTSP_ERROR("[%s.%d]packet length err %d nDataLen = %d\n",__FUNCTION__,__LINE__,npktlen,nDataLen);
        return -1;
    }
    //printf("curr = %d,last = %d\n",nSeq,m_nLastSeq);
    // printf("Recv Seq = %u mark = %d\n",nSeq,bMark);

    bSeqError = 0;
    if (m_nLastSeq != 0)
    {
        nReqSeq = m_nLastSeq + 1;
        if (nSeq != nReqSeq)
        {
            RTSP_ERROR("[RTSP]lost seq no. %d <= %d ssrc = %10d\n",nSeq,m_nLastSeq,ssrc);
            bSeqError = 1;
        }
    }
    m_nLastSeq = nSeq;


    if (0 == bSeqError)
    {
		if (ps_padding == m_status)
		{
	        if (sizeof(ps_header_t) <= npktlen && is_ps_header((ps_header_t *)pdata))
	        {
	            m_nRecvDataPos = 0;
				m_PesLen = 0;
				m_nARecvDataPos = 0;
				m_PesLenAudio = 0;
				m_PesHeadLen = 0;
				m_Type = 0;
				m_bIFrame = 0;
				m_bAudio = -1;
				m_PesLenBd = 0;
				m_nBdRecvDataPos = 0;

	            DealPsHead(pdata, npktlen, bMark, nTimestamp);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
			else if (sizeof(pes_header_t) <= npktlen && is_pes_audio_header((pes_header_t *)pdata))
			{
	            m_nRecvDataPos = 0;
				m_PesLen = 0;
				m_nARecvDataPos = 0;
				m_PesLenAudio = 0;
				m_PesHeadLen = 0;
				m_Type = 0;
				m_bIFrame = 0;
				m_bAudio = -1;
				m_PesLenBd = 0;
				m_nBdRecvDataPos = 0;

	            DealPsHead(pdata, npktlen, bMark, nTimestamp, 1);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
		}
		else if (ps_ps == m_status)
		{
            if (sizeof(ps_header_t) <= npktlen && is_ps_header((ps_header_t *)pdata))
			{
	            m_nRecvDataPos = 0;
				m_PesLen = 0;
				m_nARecvDataPos = 0;
				m_PesLenAudio = 0;
				m_PesHeadLen = 0;
				m_Type = 0;
				m_bIFrame = 0;
				m_bAudio = -1;
				m_PesLenBd = 0;
				m_nBdRecvDataPos = 0;

	            DealPsHead(pdata, npktlen, bMark, nTimestamp);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
			else if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_audio_header((pes_header_t *)pdata))
			{
	            m_nRecvDataPos = 0;
				m_PesLen = 0;
				m_nARecvDataPos = 0;
				m_PesLenAudio = 0;
				m_PesHeadLen = 0;
				m_Type = 0;
				m_bIFrame = 0;
				m_bAudio = -1;
				m_PesLenBd = 0;
				m_nBdRecvDataPos = 0;

	            DealPsHead(pdata, npktlen, bMark, nTimestamp, 1);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
			else
			{
#if 1
				int PesLen = 0, PesLenAudio = 0, PesLenBd = 0;

			    if (m_bAudio == 0)
			    {
				    if (m_nRecvDataPos < m_PesLen)
					{
	                    PesLen = m_PesLen - m_nRecvDataPos;
					}
					else if (m_nRecvDataPos == m_PesLen)
					{
						resLen = npktlen < (300 - m_PesHeadLen) ? npktlen : (300 - m_PesHeadLen);
						memcpy(m_PesHead + m_PesHeadLen, pdata, resLen);

						if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < resLen + m_PesHeadLen && is_pes_header((pes_header_t *)m_PesHead))
						{
							resLen = m_PesHead[8] & 0xff;
							pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
							npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
			                //printf("video npktlen3 = %d bMark = %d!!!!!!!!!!!\n", npktlen, bMark);
							if (0xE0 == m_PesHead[3])
							{
							    PesLen = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
							    m_PesLen += PesLen;
								//DealRecvBuff(pdata, npktlen);
								m_bAudio = 0;
							}
							else if (0xC0 == m_PesHead[3])
							{
	                            PesLenAudio = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								m_PesLenAudio += PesLenAudio;
								//DealAudioRecvBuff(pdata, npktlen);
								m_bAudio = 1;
							}

							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}

						m_PesHeadLen = 0;
					}
				}
				else if (m_bAudio == 1)
				{
					if (m_nARecvDataPos < m_PesLenAudio)
					{
                        PesLenAudio = m_PesLenAudio - m_nARecvDataPos;
					}
					else if (m_nARecvDataPos == m_PesLenAudio)
					{
						resLen = npktlen < (300 - m_PesHeadLen) ? npktlen : (300 - m_PesHeadLen);
						memcpy(m_PesHead + m_PesHeadLen, pdata, resLen);

						if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < resLen + m_PesHeadLen && is_pes_header((pes_header_t *)m_PesHead))
						{
							resLen = m_PesHead[8] & 0xff;
							pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
							npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
			                //printf("audio npktlen3 = %d bMark = %d!!!!!!!!!!!\n", npktlen, bMark);
							if (0xE0 == m_PesHead[3])
							{
							    PesLen = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
							    m_PesLen += PesLen;
								//DealRecvBuff(pdata, npktlen);
								m_bAudio = 0;
							}
							else if (0xC0 == m_PesHead[3])
							{
	                            PesLenAudio = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								m_PesLenAudio += PesLenAudio;
								//DealAudioRecvBuff(pdata, npktlen);
								m_bAudio = 1;
							}

							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}

						m_PesHeadLen = 0;
					}
				}
                else if (m_bAudio == 2)
                {
				    if (m_nBdRecvDataPos < m_PesLenBd)
					{
	                    PesLenBd = m_PesLenBd - m_nBdRecvDataPos;
					}
					else if (m_nBdRecvDataPos == m_PesLenBd)
					{
						resLen = npktlen < (300 - m_PesHeadLen) ? npktlen : (300 - m_PesHeadLen);
						memcpy(m_PesHead + m_PesHeadLen, pdata, resLen);

						if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < resLen + m_PesHeadLen && is_pes_header((pes_header_t *)m_PesHead))
						{
							resLen = m_PesHead[8] & 0xff;
							pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
							npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
			                //printf("video npktlen3 = %d bMark = %d!!!!!!!!!!!\n", npktlen, bMark);
							if (0xE0 == m_PesHead[3])
							{
							    PesLen = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
							    m_PesLen += PesLen;
								//DealRecvBuff(pdata, npktlen);
								m_bAudio = 0;
							}
							else if (0xC0 == m_PesHead[3])
							{
	                            PesLenAudio = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								m_PesLenAudio += PesLenAudio;
								//DealAudioRecvBuff(pdata, npktlen);
								m_bAudio = 1;
							}
							else
							{
                                PesLenBd = ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								m_PesLenBd += PesLenBd;
								m_bAudio = 2;
							}

							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}

						m_PesHeadLen = 0;
					}
				}
				else
				{
					m_bPsLastRecvError = 1;
					return -1;
				}

				while (1)
				{
					if (m_bAudio == 0)
					{
					    if (npktlen > PesLen)
					    {
					        DealRecvBuff(pdata, PesLen);
							pdata += PesLen;
							npktlen -= PesLen;
					    }
						else
						{
					        DealRecvBuff(pdata, npktlen);
						    if (1 == m_bPsLastRecvError)
			                {
			                    return -1;
			                }
							//m_status = ps_ps;
							break;
						}
					}
					else if (m_bAudio == 1)
					{
					    if (npktlen > PesLenAudio)
					    {
						    DealAudioRecvBuff(pdata, PesLenAudio);
							pdata += PesLenAudio;
							npktlen -= PesLenAudio;
					    }
						else
						{
						    DealAudioRecvBuff(pdata, npktlen);
						    if (1 == m_bPsLastRecvError)
			                {
			                    return -1;
			                }
						    //m_status = ps_ps;
		                    break;
						}
					}
					else if (m_bAudio == 2)
					{
					    if (npktlen > PesLenBd)
					    {
						    DealBdRecvBuff(pdata, PesLenBd);
							pdata += PesLenBd;
							npktlen -= PesLenBd;
					    }
						else
						{
						    DealBdRecvBuff(pdata, npktlen);
			                if (1 == m_bPsLastRecvError)
			                {
			                    return -1;
			                }
						    //m_status = ps_ps;
		                    break;
						}
					}
					else
					{
						m_bPsLastRecvError = 1;
						return -1;
					}

					if (npktlen == 0)
					{
					    //m_status = ps_ps;
		                break;
					}

					if (1 == m_bPsLastRecvError)
					{
			            return -1;
					}

					if (npktlen < sizeof(pes_header_t) + sizeof(optional_pes_header_t))
					{
						memcpy(m_PesHead, pdata, npktlen);
						m_PesHeadLen = npktlen;
						m_status = ps_ps;
						break;
					}
					else
					{
					    if (is_pes_header((pes_header_t *)pdata))
						{
						    resLen = pdata[8] & 0xff;

							if (npktlen < sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen)
							{
								memcpy(m_PesHead, pdata, npktlen);
								m_PesHeadLen = npktlen;
								//m_status = ps_ps;
								break;
							}
							else
							{
								if (0xC0 == pdata[3])
							    {
							        PesLenAudio = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
									m_PesLenAudio += PesLenAudio;
									m_bAudio = 1;
								}
								else if (0xE0 == pdata[3])
								{
									PesLen = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
							        m_PesLen += PesLen;
									m_bAudio = 0;
								}
								else if (0xBD == pdata[3])
								{
									PesLenBd = (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
							        m_PesLenBd += PesLenBd;
									m_bAudio = 0;
								}
								else
								{
									m_bPsLastRecvError = 1;
									return -1;
								}

								pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
							    npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
							}
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}
					}
				}

#else
				if (m_bAudio == 0)
				{
				    if (m_nRecvDataPos < m_PesLen)
					{
						if (m_nRecvDataPos + npktlen <= m_PesLen)
						{
							DealRecvBuff(pdata, npktlen);
							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
                        else
                        {
							int tmpLen = 0;

							tmpLen = m_PesLen - m_nRecvDataPos;

							if (0 < m_PesLen - m_nRecvDataPos)
							{
								DealRecvBuff(pdata, m_PesLen - m_nRecvDataPos);
								if (1 == m_bPsLastRecvError)
								{
									return -1;
								}
							}

							leftLen = npktlen - tmpLen;
							pdata += tmpLen;

						    if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < leftLen)
							{
								if (is_pes_header((pes_header_t *)pdata))
								{
									resLen = pdata[8] & 0xff;
									if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen <= leftLen)
									{
										int bAudio = -1;
										if (0xE0 == pdata[3])
										{
										    bAudio = 0;
										    m_PesLen += (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
										}
										else if (0xC0 == pdata[3])
										{
										    bAudio = 1;
										    m_PesLenAudio += (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
										}

										pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
										leftLen = leftLen - (sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen);
			                            //printf("leftLen = %d!!!!!!!!!!!\n", leftLen);
										if (0 < leftLen)
										{
											if (0 == bAudio)
											{
											    DealRecvBuff(pdata, leftLen);
												m_bAudio = 0;
											}
											else if (1 == bAudio)
											{
											    DealAudioRecvBuff(pdata, leftLen);
								                m_bAudio = 1;
											}

											if (1 == m_bPsLastRecvError)
											{
												return -1;
											}
										}
									}
									else
									{
										memcpy(m_PesHead, pdata, leftLen);
										m_PesHeadLen = leftLen;
									}
								}
								else
								{
									m_bPsLastRecvError = 1;
									return -1;
								}
							}
							else
							{
								if (0 < leftLen)
								{
									memcpy(m_PesHead, pdata, leftLen);
								}
								m_PesHeadLen = leftLen;
							}
						}
					}
					else if (m_nRecvDataPos == m_PesLen)
					{
						resLen = npktlen < (300 - m_PesHeadLen) ? npktlen : (300 - m_PesHeadLen);
						memcpy(m_PesHead + m_PesHeadLen, pdata, resLen);

						if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < resLen + m_PesHeadLen && is_pes_header((pes_header_t *)m_PesHead))
						{
							resLen = m_PesHead[8] & 0xff;
							pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
							npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
			                //printf("video npktlen3 = %d bMark = %d!!!!!!!!!!!\n", npktlen, bMark);
							if (0xE0 == m_PesHead[3])
							{
							    m_PesLen += ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								DealRecvBuff(pdata, npktlen);
								m_bAudio = 0;
							}
							else if (0xC0 == m_PesHead[3])
							{
	                            m_PesLenAudio += ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								DealAudioRecvBuff(pdata, npktlen);
								m_bAudio = 1;
							}

							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}

						m_PesHeadLen = 0;
					}
				}
				else if (m_bAudio == 1)
				{
				    if (m_nARecvDataPos < m_PesLenAudio)
					{
						if (m_nARecvDataPos + npktlen <= m_PesLenAudio)
						{
							DealAudioRecvBuff(pdata, npktlen);
							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
						else
						{
							int tmpLen = 0;

							tmpLen = m_PesLenAudio - m_nARecvDataPos;

							if (0 < m_PesLenAudio - m_nARecvDataPos)
							{
								DealAudioRecvBuff(pdata, m_PesLenAudio - m_nARecvDataPos);
								if (1 == m_bPsLastRecvError)
								{
									return -1;
								}
							}

							leftLen = npktlen - tmpLen;
							pdata += tmpLen;

							if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < leftLen)
							{
								if (is_pes_header((pes_header_t *)pdata))
								{
									resLen = pdata[8] & 0xff;
									if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen <= leftLen)
									{
										int bAudio = -1;
										if (0xE0 == pdata[3])
										{
										    bAudio = 0;
										    m_PesLen += (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
										}
										else if (0xC0 == pdata[3])
										{
										    bAudio = 1;
										    m_PesLenAudio += (((pdata[4] << 8) | (pdata[5])) & 0xffff) - (resLen + 3);
										}

										pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen;
										leftLen = leftLen - (sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen);
			                            //printf("leftLen = %d!!!!!!!!!!!\n", leftLen);
										if (0 < leftLen)
										{
											if (0 == bAudio)
											{
											    DealRecvBuff(pdata, leftLen);
												m_bAudio = 0;
											}
											else if (1 == bAudio)
											{
											    DealAudioRecvBuff(pdata, leftLen);
								                m_bAudio = 1;
											}

											if (1 == m_bPsLastRecvError)
											{
												return -1;
											}
										}
									}
									else
									{
										memcpy(m_PesHead, pdata, leftLen);
										m_PesHeadLen = leftLen;
									}
								}
								else
								{
									m_bPsLastRecvError = 1;
									return -1;
								}
							}
							else
							{
								if (0 < leftLen)
								{
									memcpy(m_PesHead, pdata, leftLen);
								}
								m_PesHeadLen = leftLen;
							}
						}
					}
					else if (m_nARecvDataPos == m_PesLenAudio)
					{
						resLen = npktlen < (300 - m_PesHeadLen) ? npktlen : (300 - m_PesHeadLen);
						memcpy(m_PesHead + m_PesHeadLen, pdata, resLen);

						if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) < resLen + m_PesHeadLen && is_pes_header((pes_header_t *)m_PesHead))
						{
							resLen = m_PesHead[8] & 0xff;
							pdata += sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
							npktlen -= sizeof(pes_header_t) + sizeof(optional_pes_header_t) + resLen - m_PesHeadLen;
			                //printf("audio npktlen3 = %d bMark = %d!!!!!!!!!!!\n", npktlen, bMark);
							if (0xE0 == m_PesHead[3])
							{
							    m_PesLen += ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								DealRecvBuff(pdata, npktlen);
								m_bAudio = 0;
							}
							else if (0xC0 == m_PesHead[3])
							{
	                            m_PesLenAudio += ((((m_PesHead[4] << 8) | (m_PesHead[5]))) & 0xffff) - (resLen + 3);
								DealAudioRecvBuff(pdata, npktlen);
								m_bAudio = 1;
							}

							if (1 == m_bPsLastRecvError)
							{
								return -1;
							}
						}
						else
						{
							m_bPsLastRecvError = 1;
							return -1;
						}

						m_PesHeadLen = 0;
					}
				}
#endif
				if (1 == bMark)
				{
					if (m_pRtpSessionClientFxn != NULL)
					{
						int prop = 0;
						if(m_dwProp & 1)
							prop = 1;

						if (0 < m_nRecvDataPos)
						{
						    DealAntsFrameHeader(0, nTimestamp);
                            if (1 == m_bPsLastRecvError)
                            {
                                return -1;
                            }
							pFrame = (AntsFrameHeader *)m_pRecvDataBuff;
		                    //printf("m_pRtpSessionClientFxn2 start pFrame->uiFrameType = %d m_nRecvDataPos = %d m_nRecvDataStart = %d!!!!!!!!!!!\n", pFrame->uiFrameType, m_nRecvDataPos, m_nRecvDataStart);
	#if 0
		if (fp2 == NULL && file_size2 == 0)
		{
			fp2 = fopen("/tmp/28181.dav", "wb");
		}

		if (fp2 != NULL)
		{
			file_size2 += fwrite((char *)m_pRecvDataBuff, m_nRecvDataPos + m_nRecvDataStart, 1, fp2);
			fflush(fp2);
		}

		if (file_size2 > 64 * 1024 * 1024 && fp2 != NULL)
		{
			fclose(fp2);
			fp2 = NULL;
		}
	#endif
							m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,prop,pFrame->uiFrameType,m_pRecvDataBuff,m_nRecvDataStart + m_nRecvDataPos,m_pRtpSessionClientUser);
		                    //printf("m_pRtpSessionClientFxn2 end!!!!!!!!!!!\n");
							if (prop)
							{
								m_pRecvDataBuff = NULL;
							}
						}

						if (0 < m_nARecvDataPos)
						{
						    DealAntsFrameHeader(1, nTimestamp);
                            if (1 == m_bPsLastRecvError)
                            {
                                return -1;
                            }
							pFrame = (AntsFrameHeader *)m_pARecvDataBuff;
	                       // printf("m_nARecvDataPos2 = %d!!!!!!!!!!!\n", m_nARecvDataPos);
				            if (m_AudioCodecId == AntsG711A && m_nARecvDataPos > 320)
				            {
			                    DealAntsG711Frame(prop);
							}
							else
							{
	                            m_pRtpSessionClientFxn(m_pRtpSessionClientHandle,ANTS_RTSP_CALLBACKBYPE_STREAM,ANTS_RTSP_CALLBACKBYPE_STREAM_PS,prop,pFrame->uiFrameType,m_pARecvDataBuff,m_nARecvDataStart + m_nARecvDataPos,m_pRtpSessionClientUser);
								if (prop)
								{
									m_pARecvDataBuff = NULL;
								}
							}
						}
					}

					m_nRecvDataPos = 0;
					m_nARecvDataPos = 0;
					m_nBdRecvDataPos = 0;
					bFrame = 1;

					m_status = ps_padding;

					return bFrame;
				}
				else if (0 == bMark)
				{
					m_status = ps_ps;
				}
			}
		}
    }
    else if (1 == bSeqError)
    {
        if (ps_padding == m_status)
        {
	        if (sizeof(ps_header_t) <= npktlen && is_ps_header((ps_header_t *)pdata))
	        {
		        m_nRecvDataPos = 0;
				m_PesLen = 0;
		        m_nARecvDataPos = 0;
				m_PesLenAudio = 0;
				m_PesHeadLen = 0;
				m_Type = 0;
				m_bIFrame = 0;
				m_bAudio = -1;
				m_PesLenBd = 0;
				m_nBdRecvDataPos = 0;

		        DealPsHead(pdata, npktlen, bMark, nTimestamp);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
			else if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_audio_header((pes_header_t *)pdata))
			{
	            m_nRecvDataPos = 0;
				m_PesLen = 0;
				m_nARecvDataPos = 0;
				m_PesLenAudio = 0;
				m_PesHeadLen = 0;
				m_Type = 0;
				m_bIFrame = 0;
				m_bAudio = -1;
				m_PesLenBd = 0;
				m_nBdRecvDataPos = 0;

	            DealPsHead(pdata, npktlen, bMark, nTimestamp, 1);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
		}
		else if (ps_ps == m_status)
		{
            m_nRecvDataPos = 0;
			m_PesLen = 0;
            m_nARecvDataPos = 0;
			m_PesLenAudio = 0;
			m_PesHeadLen = 0;
			m_Type = 0;
			m_bIFrame = 0;
			m_bAudio = -1;
			m_PesLenBd = 0;
			m_nBdRecvDataPos = 0;

	        if (sizeof(ps_header_t) <= npktlen && is_ps_header((ps_header_t *)pdata))
	        {
		        DealPsHead(pdata, npktlen, bMark, nTimestamp);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
			else if (sizeof(pes_header_t) + sizeof(optional_pes_header_t) <= npktlen && is_pes_audio_header((pes_header_t *)pdata))
			{
	            DealPsHead(pdata, npktlen, bMark, nTimestamp, 1);
				if (1 == m_bPsLastRecvError)
				{
                    return -1;
				}
			}
			else
			{
                m_status = ps_padding;
			}
		}
	}

    return 0;
}



