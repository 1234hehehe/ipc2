#ifndef _H264RTPSESSION_H_
#define _H264RTPSESSION_H_


#include "rtpsession.h"

#include "rtsp_common.h"

#include "streamdef.h"

using namespace jrtplib ;

#pragma pack(push,1)
// audio
#define PAYLAODTYPE_G711U                    0
#define PAYLAODTYPE_G711A                    8
#define PAYLAODTYPE_G726_16K                    97
#define AUDIO_PACK_NUM     1 // 8 //8帧一调

#define AUDIORTPSESSION_RECV_BUFFSIZE      (64 * 1024)//((36 + 320) * 8 + 16)

#define AUDIO_SIZE_PER_FRAME    (36 + 320)
#define ANTS_RTSP_G711A      1
#define ANTS_RTSP_G711U      2
// APP

// MJPEG
#define PAYLAODTYPE_MJPEG                    26



#define ANTS_RTSP_MJPEG_MARK_SOF             0xC0 /* baseline */
#define ANTS_RTSP_MJPEG_MARK_SOF1             0xC1 /* extended sequential, huffman */
#define ANTS_RTSP_MJPEG_MARK_SOF2             0xC2 /* progressive, huffman */
#define ANTS_RTSP_MJPEG_MARK_SOF3             0xC3  /* lossless, huffman */

#define ANTS_RTSP_MJPEG_MARK_SOF5             0xC5 /* differential sequential, huffman */
#define ANTS_RTSP_MJPEG_MARK_SOF6             0xC6 /* differential progressive, huffman */
#define ANTS_RTSP_MJPEG_MARK_SOF7             0xC7 /* differential lossless, huffman */
#define ANTS_RTSP_MJPEG_MARK_JPGEX             0xC8  /* reserved for JPEG extension */
#define ANTS_RTSP_MJPEG_MARK_SOF9             0xC9 /* extended sequential, arithmetic */
#define ANTS_RTSP_MJPEG_MARK_SOF10             0xCA /* progressive, arithmetic */
#define ANTS_RTSP_MJPEG_MARK_SOF11             0xCB /* lossless, arithmetic */

#define ANTS_RTSP_MJPEG_MARK_SOF13             0xCD /* differential sequential, arithmetic */
#define ANTS_RTSP_MJPEG_MARK_SOF14             0xCE /* differential progressive, arithmetic */
#define ANTS_RTSP_MJPEG_MARK_SOF15             0xCF/* differential lossless, arithmetic */

#define ANTS_RTSP_MJPEG_MARK_DHT             0xC4 /* define huffman tables */
#define ANTS_RTSP_MJPEG_MARK_DAC             0xCC /* define arithmetic-coding conditioning */

 /* restart with modulo 8 count "m" */
#define ANTS_RTSP_MJPEG_MARK_RST0             0xD0
#define ANTS_RTSP_MJPEG_MARK_RST1             0xD1
#define ANTS_RTSP_MJPEG_MARK_RST2             0xD2
#define ANTS_RTSP_MJPEG_MARK_RST3             0xD3
#define ANTS_RTSP_MJPEG_MARK_RST4             0xD4
#define ANTS_RTSP_MJPEG_MARK_RST5             0xD5
#define ANTS_RTSP_MJPEG_MARK_RST6             0xD6
#define ANTS_RTSP_MJPEG_MARK_RST7             0xD7


#define ANTS_RTSP_MJPEG_MARK_SOI             0xD8 /* start of image */

#define ANTS_RTSP_MJPEG_MARK_DQT             0xDB  /* define quantization tables */
#define ANTS_RTSP_MJPEG_MARK_DNL             0xDC /* define number of lines */
#define ANTS_RTSP_MJPEG_MARK_DRI             0xDD  /* define restart interval */
#define ANTS_RTSP_MJPEG_MARK_DHP             0xDE  /* define hierarchical progression */
#define ANTS_RTSP_MJPEG_MARK_EXP             0xDF /* expand reference components */

#define ANTS_RTSP_MJPEG_MARK_SOS             0xDA  /* start of scan */
#define ANTS_RTSP_MJPEG_MARK_EOI             0xD9 /* end of image */

#define ANTS_RTSP_MJPEG_MARK_APP0            0xE0
#define ANTS_RTSP_MJPEG_MARK_APP1            0xE1
#define ANTS_RTSP_MJPEG_MARK_APP2            0xE2
#define ANTS_RTSP_MJPEG_MARK_APP3            0xE3
#define ANTS_RTSP_MJPEG_MARK_APP4            0xE4
#define ANTS_RTSP_MJPEG_MARK_APP5            0xE5
#define ANTS_RTSP_MJPEG_MARK_APP6            0xE6
#define ANTS_RTSP_MJPEG_MARK_APP7            0xE7
#define ANTS_RTSP_MJPEG_MARK_APP8            0xE8
#define ANTS_RTSP_MJPEG_MARK_APP9            0xE9
#define ANTS_RTSP_MJPEG_MARK_APP10            0xEA
#define ANTS_RTSP_MJPEG_MARK_APP11            0xEB
#define ANTS_RTSP_MJPEG_MARK_APP12            0xEC
#define ANTS_RTSP_MJPEG_MARK_APP13            0xED
#define ANTS_RTSP_MJPEG_MARK_APP14            0xEE
#define ANTS_RTSP_MJPEG_MARK_APP15            0xEF

#define ANTS_RTSP_MJPEG_MARK_JPG0            0xF0
#define ANTS_RTSP_MJPEG_MARK_JPG1            0xF1
#define ANTS_RTSP_MJPEG_MARK_JPG2            0xF2
#define ANTS_RTSP_MJPEG_MARK_JPG3            0xF3
#define ANTS_RTSP_MJPEG_MARK_JPG4            0xF4
#define ANTS_RTSP_MJPEG_MARK_JPG5            0xF5
#define ANTS_RTSP_MJPEG_MARK_JPG6            0xF6
#define ANTS_RTSP_MJPEG_MARK_SOF48            0xF7 ///< JPEG-LS
#define ANTS_RTSP_MJPEG_MARK_LSE            0xF8  ///< JPEG-LS extension parameters
#define ANTS_RTSP_MJPEG_MARK_JPG9            0xF9
#define ANTS_RTSP_MJPEG_MARK_JPG10            0xFA
#define ANTS_RTSP_MJPEG_MARK_JPG11            0xFB
#define ANTS_RTSP_MJPEG_MARK_JPG12            0xFC
#define ANTS_RTSP_MJPEG_MARK_JPG13            0xFD
#define ANTS_RTSP_MJPEG_MARK_COM            0xFE  /* comment */
#define ANTS_RTSP_MJPEG_MARK_TEM            0x01 /* temporary private use for arithmetic coding */
/* 0x02 -> 0xbf reserved */

typedef struct {
    unsigned int bySpecificType:8 ;// Type-specific
    unsigned int uOffset24:24 ; // 分段偏移,网络顺序
    unsigned char byType; // 0/64: 422; 1/65:420
    unsigned char byQ;
    unsigned char byWidth; // width/8
    unsigned char byHeight;

} MJPEG_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned short wResetInterval;// htons 网络顺序
    unsigned short F:1; 
    unsigned short L:1;
    unsigned short uResetCnt:14;// 网络顺序

} MJPEG_RESETMARKER; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char byMBZ;
    unsigned char byPrecision;
    unsigned short wQTableLength; // 网络顺序
    unsigned char byQTable[0];   // 64? 
} MJPEG_QUANTHEADER; /**//* 1 BYTES */

typedef struct  
{
    unsigned char byMark;
    unsigned char byType;
    unsigned short wLen;
    char szJFIF[5];// 标识符
    unsigned char byVersion[2]; // [0]主版本号+[1]次版本号
    unsigned char byXYUnits; // 0-无单位，1-点数/英寸,2-点数/厘米
    unsigned short wXDensity; // X方向像素密度
    unsigned short wYDensity; // Y方向像素密度
    unsigned char byThumbnailHoriPixels; // 缩略图水平像素数目
    unsigned char byThumbnailVertPixels; // 缩略图垂直像素数目
    unsigned char byThumbnail[0]; // [n]缩略RGB位图，由上数值决定大小，3n
}MJPEG_APP; 

typedef struct  
{
    unsigned char byMark;
    unsigned char byType;
    unsigned short wLen;
    unsigned char byQT; // bits[0-3]QT号(0~3,否则错误),bits[4~7]，QT精度,0-8bits,1-16bits
    unsigned char byQTable[0]; // QT表,n = 64 *(精度 + 1)
}MJPEG_DQT; 

typedef struct  
{
    unsigned char byMark;
    unsigned char byType;
    unsigned short wLen;
    unsigned short wRi; // bits[0-3]QT号(0~3,否则错误),bits[4~7]，QT精度,0-8bits,1-16bits
}MJPEG_DRI; 

typedef struct  
{
    unsigned char byMark;
    unsigned char byType;
    unsigned short wLen;
    unsigned char byPrecision; // 数据精度 ,每个颜色分量每个像素的位数,通常是 8
    unsigned short wHeight; // 高 网络字节顺序
    unsigned short wWidth; // 宽 
    unsigned char byComponentNum; // 1-灰度图，3-YCBCR/YIQ彩色图，4-CMYK,
    unsigned char byComponents[9];// [n] 每个com:3bytes,分别是 ID(1-Y,2-Cb,3-Cr,4-I,5-Q);采样系统(bit[0-3] Vert,bits[4-7] hor);量化表编号
}MJPEG_SOF;

typedef struct  
{
    unsigned char byMark;
    unsigned char byType;
    unsigned short wLen;
    unsigned char byHT; // Bits[0-3] HT号(0..3,否则错误),bits[4-7] HT类型: 0-DC 1-AC,高3位必须为0
    unsigned char byIndex[16]; //索引表头,长度是 1到 16范式Huffman编码对应的符号个数
    unsigned char byValue[1];// [n] 值表,n=代码总数
}MJPEG_DHT;

typedef struct  
{
    unsigned char byMark;
    unsigned char byType;
    unsigned short wLen;
    unsigned char byScanNum; // 扫描内组件的数量级，通常为 3
    unsigned char byComps[1]; //[n] = byScanNum * 2 每个组件2bytes,ID (1-Y,2-Cb,3-Cr,4-I,5-Q);使用的Huffman表 (bits[0-3]:ACtable,bits[4-7] DC table)
    
    // 0x00; // start of spectral
    // 0x3F; // end of spectral
    // 0x00; // successive approximation bit position (high, low)
}MJPEG_SOS;



// H.264

#define PAYLAODTYPE_H264                    96

#define STAP_A_H264                           (24) // 单时间多包 NALU+[2bytesSize + nalhdr + data]xN
#define STAP_B_H264                           (25)
#define MTAP16_H264                           (26)//多个时间的组合包
#define MTAP24_H264                           (27)//多个时间的组合包
#define FU_TYPE_H264                          (28)
#define FU_B_TYPE_H264                        (29)

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

// H.265 (HEVC)
#define PAYLAODTYPE_H265                    98


#define    HEVC_SLICE_TYPE_B    0 
#define    HEVC_SLICE_TYPE_P    1 
#define    HEVC_SLICE_TYPE_I    2 


#define HEVC_NAL_SLICE_TRAIL_N  0
#define HEVC_NAL_SLICE_TRAIL_R  1
#define HEVC_NAL_SLICE_TSR_N    2
#define HEVC_NAL_SLICE_TSA_R    3
#define HEVC_NAL_SLICE_STSA_N   4
#define HEVC_NAL_SLICE_STSA_R   5
#define HEVC_NAL_SLICE_RADL_N   6
#define HEVC_NAL_SLICE_RADL_R   7
#define HEVC_NAL_SLICE_RASL_N   8
#define HEVC_NAL_SLICE_RASL_R   9

// 16-20 相当于一个新的序列没有参考帧
#define HEVC_NAL_SLICE_BLA_W_LP      16
#define HEVC_NAL_SLICE_BLA_W_RADL    17
#define HEVC_NAL_SLICE_BLA_N_LP      18
#define HEVC_NAL_SLICE_IDR_W_RADL    19
#define HEVC_NAL_SLICE_IDR_N_LP       20
#define HEVC_NAL_SLICE_CRA_NUT        21

#define HEVC_NAL_SLICE_RSV_IRAP_VCL22      22
#define HEVC_NAL_SLICE_RSV_IRAP_VCL23      23

#define HEVC_NAL_VPS            32
#define HEVC_NAL_SPS            33
#define HEVC_NAL_PPS            34
#define HEVC_PICTURE_DELIMITER	35
#define HEVC_NAL_EOS            36
#define HEVC_NAL_EOB            37
#define HEVC_NAL_FILTER_DATA    38
#define HEVC_NAL_SEI            39
#define HEVC_NAL_SEI_SUFFIX     40



#define HEVC_AP_TYPE                          (48)
#define HEVC_FU_TYPE                          (49)
#define HEVC_PACI_TYPE                        (50)


#define HEVC_NALU_GET_F(a) (((a)>>7)&1)
#define HEVC_NALU_GET_TYPE(a) (((a)>>1)&63)
#define HEVC_NALU_GET_LAYERID(a,b) ((((a)&1)<<5) | (((b) >> 3 ) & 31))
#define HEVC_NALU_GET_TID(b) (((b)>>0)&7)

#define HEVC_NALU_RESET(a,b)  (a)=0,(b)=0
#define HEVC_NALU_SET_F(a,f) (a) |= (((f)&1) << 7)
#define HEVC_NALU_SET_TYPE(a,type) (a) |= (((type)&63) << 1)
#define HEVC_NALU_SET_LAYERID(a,b,LID) (a) |= ((((LID)>> 5)&1) << 0),(b)|=(((LID) & 31)<<3)
#define HEVC_NALU_SET_TID(b,TID) (b)|=(((TID) & 7)<<0)




#define HEVC_FU_RESET(c)  (c)=0
#define HEVC_FU_SET_S(c,s) (c)|=(((s) & 1) << 7)
#define HEVC_FU_SET_E(c,e) (c)|=(((e) & 1) << 6)
#define HEVC_FU_SET_TYPE(c,type) (c)|=(((type) & 63) << 0)

#define HEVC_FU_GET_S(c) (((c)>>7) & 1)
#define HEVC_FU_GET_E(c) (((c) >> 6) & 1)
#define HEVC_FU_GET_TYPE(c) ((c) & 63)




#define H264RTPSESSION_RECV_BUFFSIZE     (8 * 1024)
#define H264RTPSESSION_RECV_BUFFSIZE_INC  (8 * 1024)  //每次增加数,最大不超过 H264RTPSESSION_RECV_BUFFSIZE * H264RTPSESSION_RECV_BUFFSIZE_X = 1M
#define H264RTPSESSION_RECV_BUFFSIZE_X     (2048 * 1024 /H264RTPSESSION_RECV_BUFFSIZE)

#define DATAPACKNODE_NUM     512

typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2;
    unsigned char F:1;    

} NALU_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2; 
    unsigned char F:1;    


} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char R:1;
    unsigned char E:1;
    unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */

// AntsComb
#define PAYLAODTYPE_AntsComb                    110

#pragma pack(pop)



class CH264RtpSession:public RTPSession
{
public:
    CH264RtpSession();
    ~CH264RtpSession();
	int SetDefaultPayloadType(uint8_t pt);
	int GetDefaultPayloadType(){return m_nDefaultPayloadType;}

	int SetSecondPayloadType(uint8_t pt);
	int GetSecondPayloadType(){return m_nSecondPayloadType;}

	/** Sets the default marker for RTP packets to \c m. */
	int SetDefaultMark(bool m);
    int SendH264Packet(int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
    int SendH265Packet(int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒

    int SendAntsCombPacket(int nType,int nChan,int nStreamIdx,void *pData,int nDataSize);//毫秒
    int SendPacketByStreamType(int nFrameType,void *data,int len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
     int SendVideoPacket(int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
    time_t GetAliveTime(){return m_tAliveTime;}
    // audio
    int SendAudioPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
    int Send_Adpcm2G711U_Packet(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
    int DealRecvAudioPacket(unsigned char *pRecvData,int len);
    int DealRecvAudioPacket_G711(unsigned char *pRecvData,int len);
    // app
    int SendAppPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒
    int DealRecvAppPacket(unsigned char *pRecvData,int nDataLen);
    //
    void SetClientCallback(void *hClient,ANTS_RTPSESSION_CLIENT_CALLBACK sessCallback,void *pUser);
    void SetSpecialFlag(int bFlag){m_bSpecial_Ex = bFlag;}
    void SetRecvDataBuffer(void *pDataBuff,size_t nDataBuffSize);

    int DealRecvPacket(unsigned char *pRecvData,int nDataLen);
    int DealRecvPacket_mSlices(unsigned char *pRecvData,int nDataLen);
    int DealRecvPacket_ex(unsigned char *pRecvData,int nDataLen);
    int SetSPS(char *pSPSData,int nSPSLen);
    int SetPPS(char *pPPSData,int nPPSLen);
    int SetVPS(char *pVPSData,int nVPSLen);
    int GetProfileLevelID();
    int GetSPS(char *pSPSData,int nBuffSize)
    {
        int nLen = m_nSPSLen;
        m_hMemLock.Lock();
        if (pSPSData != NULL && nBuffSize >= m_nSPSLen)
        {
            memcpy(pSPSData,m_pSPS,m_nSPSLen);
            nLen = m_nSPSLen;
        }
        else if (pSPSData == NULL)
        {
            nLen = m_nSPSLen;
        }
        else
        {
            nLen = 0;
        }
        m_hMemLock.Unlock();
        return nLen;
    }

    int GetPPS(char *pPPSData,int nBuffSize)
    {
        int nLen = m_nPPSLen;
        m_hMemLock.Lock();
        if (pPPSData != NULL && nBuffSize >= m_nPPSLen)
        {
            memcpy(pPPSData,m_pPPS,m_nPPSLen);
            nLen = m_nPPSLen;
        }
        else if (pPPSData == NULL)
        {
            nLen = m_nPPSLen;
        }
        else
        {
            nLen = 0;
        }
        m_hMemLock.Unlock();
        return m_nPPSLen;
    }
    int GetVPS(char *pVPSData,int nBuffSize)
    {
        int nLen = m_nVPSLen;
        m_hMemLock.Lock();
        if (pVPSData != NULL && nBuffSize >= m_nVPSLen)
        {
            memcpy(pVPSData,m_pVPS,m_nVPSLen);
            nLen = m_nVPSLen;
        }
        else if (pVPSData == NULL)
        {
            nLen = m_nVPSLen;
        }
        else
        {
            nLen = 0;
        }
        m_hMemLock.Unlock();
        return m_nVPSLen;
    }

     char * GetPPS_Base64();
     char * GetSPS_Base64();
     char * GetVPS_Base64();

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

    // MJPEG
    int DealRecvMJPEGPacket(unsigned char *pRecvData,int nDataLen);
    int SendMJPEGPacket(const void *data,size_t len,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);//毫秒

    int CallBackH265Packet(int bAbs);
    int DealRecvH265Packet(unsigned char *pRecvData,int nDataLen);
    int DealRecvAntsCombPacket(unsigned char *pRecvData,int nDataLen);
	int DealBdRecvBuff(unsigned char *pdata, int nDataLen);
    int DealRecvBuff(unsigned char *pdata, int nDataLen);
    int DealAudioRecvBuff(unsigned char *pdata, int nDataLen);
    int DealPsHead(unsigned char *pCurData, int nDataLen, int bMark, unsigned int nTimestamp, int flag = 0);   
    int DealAntsFrameHeader(unsigned int bAudio, unsigned int nTimestamp);   
    int DealAntsG711Frame(int dwProp);
    int DealRecvPSPacket(unsigned char *pRecvData,int nDataLen);  
    
protected:
    

private:
    //app
    int SplitAppPacket(const void *data,size_t len,uint32_t timestamp);
    //
    int SplitPacket(const void *data,size_t len,uint32_t timestamp);
    int H265_SplitPacket(const uint8_t *data,size_t len,uint32_t timestamp);
 
    int ReadH264FrameHeader(char *pdata,int Size);
    int GetH264FrameType();
    int ReadSPS(void *pData,int Size,int bNeedCheck = 0);
    int H265_ReadSPS(void *pData,int Size);
    int H265_ReadPPS(void *pData,int Size);
    int H265_ReadFrameHeader(char *pdata,int Size);
    int H265_profile_tier_level(void *pbs,int profilePresentFlag,int maxNumSubLayersMinus1);
    unsigned int AnalyticsFrameType(void *pData,int Size,int *pZeroLoad);

    unsigned int H265AnalyticsFrameType(int nalType,void *pData,int Size,int *pZeroLoad);
    int decodeNal(void *pData,int Size,void **pOut);

	bool m_bDefaultMark;
	int m_nDefaultPayloadType;
	int m_nSecondPayloadType;
 
 
   


    uint8_t m_pPackBuf[1500 + 2];
    uint8_t *m_pRecvDataBuff;
    int m_nRecvDataPos;
    int m_nRecvDataStart;
    int m_nRecvDataBuffSize;
    int m_bFrameDealing;
    int m_nCurrSlicetype;
    int m_nLastSlicetype;
    int m_nCurrIDR_pic_id;
    int m_nLastIDR_pic_id;
    int m_nCurrNalType;
    int m_nLastNalType;
    int m_bRecvSPS;
    int m_bRecvPPS;
    int m_bRecvVPS;
    int m_nresidual_color_transform_flag;
    int m_nframe_mbs_only_flag;
    int m_nlog2_max_frame_num;
    uint64_t m_nLastTimestamp;

    int m_nFrameCnt; // 音频帧数 8帧
  

    JMutex m_hMemLock;

    int m_nProfile_idc;
    int m_nLevel_idc;
    int m_nWidth;
    int m_nHeight;
    uint16_t m_nLastSeq;
    uint32_t m_nframe_num;
    uint32_t m_nLastframe_num;

    uint32_t m_uiFrameNo;
    uint32_t m_uiLastFrameNo;
	 uint32_t m_uiFrameNo_Sub;
    char *m_pSPS;
    char *m_pPPS;
    int m_nSPSLen;
    int m_nPPSLen;
    char *m_pVPS;
    int m_nVPSLen;

    int m_bVideoReady; // 服务器 端INPUT一I帧数据，

    ANTS_RTPSESSION_CLIENT_CALLBACK m_pRtpSessionClientFxn;
    void *m_pRtpSessionClientHandle;
    void *m_pRtpSessionClientUser;
    time_t m_tAliveTime;
    
    int m_bFirstFrame;
    int64_t m_nFirstStamp;
    uint32_t m_dwFirstStamp;
    int64_t m_nLastStamp;
    uint32_t m_nCurrStamp;
	uint32_t m_nCurrStamp_recv;

    unsigned int test_seq ;
    int m_bSpecial_MSlices;
    int m_nfirst_mb_in_slice;
    int m_nLastfirst_mb_in_slice;

    int m_bSpecial_Ex;
    int m_bSpecial_Fix;
    unsigned long m_dwLastFrameIndex;
    unsigned long m_dwLastKeyFrameIndex;

    int m_bLastRecvError;
    int m_bLastMark;

    // MJPEG
    unsigned int   m_uLastOffset24;
    unsigned short m_wResetInterval;
    int m_wQTableLength[4]; 
    unsigned char *m_byQTable[4];   // 
    int m_nQTableNum;

    // H.265
    int m_dependent_slice_segments_enabled_flag;
    int m_num_extra_slice_header_bits;
    int m_slice_address_length;
    int m_first_slice_segment_in_pic_flag;

    //PS
    PSStatus      m_status;                     //当前状态
    int           m_bAudio;
    int           m_PesLen;
    int           m_PesLenAudio;  
    int           m_PesLenBd; 	
    char          m_PesHead[300];
    int           m_PesHeadLen;
	unsigned int  m_VideoCodecId;    
	unsigned int  m_AudioCodecId; 
	int           m_Type;
	int           m_bIFrame;
    uint8_t *m_pARecvDataBuff;
    int m_nARecvDataPos;
    int m_nARecvDataStart;
    int m_nARecvDataBuffSize;
    uint32_t m_uiAFrameNo;
    uint8_t *m_pAG711Buff; 
	int           m_nBdRecvDataPos;     
    int           m_bPsLastRecvError;
};


#endif
