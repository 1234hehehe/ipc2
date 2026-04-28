#ifndef _RTSP_COMMON_H_
#define _RTSP_COMMON_H_

#ifdef _DEBUG
#define PRINT_DEBUG
#endif
//#define PRINT_DEBUG
#ifndef WIN32

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>
#define GetLastError() errno
#define strnicmp strncasecmp
#define stricmp strcasecmp

#define closeSocket close
#define ioctlsocket ioctl








#ifdef PRINT_DEBUG

#ifdef ON_ANDROID
// 引入log头文件
#include  <android/log.h>
// log标签
#define  TAG    "RtspServer in JNI "
// 定义info信息
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
// 定义debug信息
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
// 定义error信息
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

#include "t_sprintf.h"
#define RTSP_DEBUG(x...)
#define RTSP_ERROR(x...)
#define snprintf(x...) t_snprintf(x)
#else
#define RTSP_DEBUG(x...) do{printf("[%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#define RTSP_ERROR(x...) do{printf("[%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#endif

#else
#define RTSP_DEBUG(x...)
#define RTSP_ERROR(x...)
#define LOGI(...)
#define LOGD(...)
#define LOGE(...)


#endif
//#define RTSP_DEBUG(x...) do{syslog(LOG_USER|LOG_NOTICE,"[%s.%d]",__FUNCTION__,__LINE__);syslog(LOG_USER|LOG_NOTICE,x);}while(0)
//#define RTSP_ERROR(x...) do{syslog(LOG_USER|LOG_NOTICE,"[%s.%d]",__FUNCTION__,__LINE__);syslog(LOG_USER|LOG_NOTICE,x);}while(0)


#else
#include <stdio.h>


typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef int socklen_t;

#define LOGI(...)
#define LOGD(...)
#define LOGE(...)

#define snprintf _snprintf

#ifdef PRINT_DEBUG

#define RTSP_DEBUG(...)  do{char xdbgstr[1024];snprintf(xdbgstr,1024,__VA_ARGS__);OutputDebugString(xdbgstr);printf("[%s.%d]",__FUNCTION__,__LINE__);printf(xdbgstr);/*sprintf(xdbgstr,"=>[%s.%d]\n",__FUNCTION__,__LINE__);OutputDebugString(xdbgstr);printf(xdbgstr);*/}while(0)
//#define RTSP_DEBUG(...)  do{printf(__VA_ARGS__);/*sprintf(xdbgstr,"=>[%s.%d]\n",__FUNCTION__,__LINE__);OutputDebugString(xdbgstr);printf(xdbgstr);*/}while(0)
//#define RTSP_DEBUG(...)  do{printf("[%s.%s.%d]",__FILE__,__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#else
#define  RTSP_DEBUG
#endif

#define RTSP_ERROR(...) do{char xdbgstr[1024];snprintf(xdbgstr,1024,__VA_ARGS__);OutputDebugString(xdbgstr);printf("[%s.%d]",__FUNCTION__,__LINE__);printf(xdbgstr);/*sprintf(xdbgstr,"=>[%s.%d]\n",__FUNCTION__,__LINE__);OutputDebugString(xdbgstr);printf(xdbgstr);*/}while(0)
//#define RTSP_ERROR(...) do{printf(__VA_ARGS__);/*sprintf(xdbgstr,"=>[%s.%d]\n",__FUNCTION__,__LINE__);OutputDebugString(xdbgstr);printf(xdbgstr);*/}while(0)
//#define RTSP_ERROR(...)  do{printf("[%s.%s.%d]",__FILE__,__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)

#endif


#ifndef EWOULDBLOCK

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE
#define EAGAIN                  WSATRY_AGAIN
#define EINTR                   WSAEINTR




#endif

#ifndef closeSocket
#define closeSocket closesocket
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR  SD_BOTH
#endif
#define CR             (0x0D)
#define LF             (0x0A)


#pragma pack(push,1)


struct RTPHeader
{
#ifdef RTP_BIG_ENDIAN
    uint8_t version:2;
    uint8_t padding:1;
    uint8_t extension:1;
    uint8_t csrccount:4;

    uint8_t marker:1;
    uint8_t payloadtype:7;
#else // little endian
    uint8_t csrccount:4;
    uint8_t extension:1;
    uint8_t padding:1;
    uint8_t version:2;

    uint8_t payloadtype:7;
    uint8_t marker:1;
#endif // RTP_BIG_ENDIAN

    uint16_t sequencenumber;
    uint32_t timestamp;
    uint32_t ssrc;
};

struct RTPExtensionHeader
{
    uint16_t extid;
    uint16_t length;
};

struct RTPExtensionOnvifHeader
{
    //extid= 0xABAC
    //length=3 3
    uint32_t ntpTimeStamp0; // ntp 时间戳
    uint32_t ntpTimeStamp1;
    uint8_t mbz:5;
    uint8_t discontinuity:1;// 标识与上一帧不连续,一般用于倒放。每个GOP第一包设置为1,因为它与前一包无关。
    uint8_t eSectionEnd:1; // 录像片段结束
    uint8_t cIFrameStart:1;// I帧开始 clean point
    uint8_t seq;// play 命令中的seq低字节值，以便让客户端知道当前帧是哪个play命令作用的结果
    uint16_t padding;
};

#define ANTS_RTSP_COMB_HEADER_MARK   0xC4DB
typedef struct _tagRTPExtensionAntsCombHeader
{
    //extid= 0xC4DB
    //length=4

    unsigned char byType:6; // 0- 音视频
    unsigned char byStart:1; // 一帧开始
    unsigned char byEnd:1; // 一帧结束
    unsigned char byStreamIdx;
    unsigned short wChan;
    unsigned int dwRes[3];
}RTPExtensionAntsCombHeader_T;

struct RTPSourceIdentifier
{
    uint32_t ssrc;
};

struct RTCPCommonHeader
{
#ifdef RTP_BIG_ENDIAN
    uint8_t version:2;
    uint8_t padding:1;
    uint8_t count:5;
#else // little endian
    uint8_t count:5;
    uint8_t padding:1;
    uint8_t version:2;
#endif // RTP_BIG_ENDIAN

    uint8_t packettype;
    uint16_t length;
};

struct RTCPSenderReport
{
    uint32_t ntptime_msw;
    uint32_t ntptime_lsw;
    uint32_t rtptimestamp;
    uint32_t packetcount;
    uint32_t octetcount;
};

struct RTCPReceiverReport
{
    uint32_t ssrc; // Identifies about which SSRC's data this report is...
    uint8_t fractionlost;
    uint8_t packetslost[3];
    uint32_t exthighseqnr;
    uint32_t jitter;
    uint32_t lsr;
    uint32_t dlsr;
};

struct RTCPSDESHeader
{
    uint8_t sdesid;
    uint8_t length;
};


#ifndef ANTS_RTSP_ANTS_HEADER_DEFINE
#define ANTS_RTSP_ANTS_HEADER_DEFINE
#pragma pack(push,1)
typedef  struct _tagAudioHeader{
    char cCodecId;			//!音频编码类型
    char cSampleRate;			//!采样率
    char cBitRate;				//!比特率
    char cChannels;			//!通道数
    char cResolution;			//!分辨力
    char cResv[3];			//!保留位
}AntsAudioHeader,*pAntsAudioHeader;

typedef  struct _tagVideoHeader{
    unsigned short usWidth;				//!视频宽度
    unsigned short usHeight;				//!视频高度
    char cCodecId;				//!视频编码类型
    char cColorSpace;         // 0-yuv420,1-yuv422,2-444
    char cResv[2];					//!保留位
}AntsVideoHeader,*pAntsVideoHeader;

typedef  struct _tagANTSAPPHeader{
    unsigned char byAppPayloadType;				//!APP 负载类型
    unsigned char byAppNameLen;                 //! APP 名称长度,接在帧头之后，数据之前,最大255个字节 nul结束
    unsigned char byRes[6];
}AntsAppHeader,*pAntsAppHeader;

typedef  struct _tagAntDTMFHeader{
	unsigned char byDTMFPayloadType;				//!DTMF 负载类型
	unsigned char byDTMFNameLen;                 //! DTMF 名称长度,接在帧头之后，数据之前,最大255个字节 nul结束
	unsigned char byRes[6];
}AntsDTMFHeader,*pAntsDTMFHeader;


typedef  struct _tagAntsFrameHeader{
    unsigned int uiStartId;					//!帧同步头
    unsigned int uiFrameType;				//!帧类型
    unsigned int uiFrameNo;				//!帧号
    unsigned int uiFrameTime;				//!UTC时间
    unsigned int uiFrameTickCount;			//!毫秒为单位的毫秒时间
    unsigned int uiFrameLen;				//!帧载长度
    //!联合体,用于存储音频帧或是视频帧信息
    union {
        AntsAudioHeader struAudioHeader;	//!音频帧信息
        AntsVideoHeader struVideoHeader;	//!视频帧信息
        AntsAppHeader   struAppHeader;
		AntsDTMFHeader struDTMFHeader;
    }uMedia; // 8bytes
    //unsigned char ucReserve[4];				//!保留位
    unsigned int uiTimeStamp;
}AntsFrameHeader,*pAntsFrameHeader;

typedef  struct _tagAntsFileHeader{
    unsigned int uiFileStartId;				//!文件起始头标记
    unsigned int uiStreamType;				//!流方式
    unsigned int uiFrameRate;				//!帧率
    unsigned int uiReserve[4];				//!保留位
}AntsFileHeader,*pAntsFileHeader;

typedef enum {
    AntsH264_hisi=1,//base/main
    AntsMJPEG_hisi=2,
    AntsH264_advance=3,
    AntsH264_hisi_high=4,//high
    AntsH264_hisi_RTP=8,//high
    AntsH264_RTP_PACK=9, //
    AntsH265=16, //
    AntsH265_RTP_PACK=17, //
    AntsSVAC=20, //
}eAntsVideoCodecId;
typedef enum{
    AntsOggVorbis=0,
    AntsG711A=1,
    AntsG711U=2,
    AntsG722Ex=3,
    AntsG726Ex=4,
    AntsAAC=5,
    AntsADPCM=8,
}eAntsVoiceCodecId;
#pragma pack(pop)
typedef enum {
    //!主码流帧类型
    AntsPktError=0x0,
    AntsPktIFrames=0x01,
    AntsPktAudioFrames=0x08,
    AntsPktPFrames=0x09,
    AntsPktBBPFrames=0x0a,
    AntsPktMotionDetection=0x0b,
    AntsPktDspStatus=0x0c,
    AntsPktOrigImage=0x0d,
    AntsPktSysHeader=0x0e,
    AntsPktBPFrames=0x0f,
    AntsPktSFrames=0x10,
    //!子码流帧类型
    AntsPktSubSysHeader=0x11,
    AntsPktSubIFrames=0x12,
    AntsPktSubPFrames=0x13,
    AntsPktSubBBPFrames=0x14,
    //!智能分析信息帧类型
    AntsPktVacEventZones=0x15,
    AntsPktVacObjects=0x16,
	//!第三码流帧类型
	AntsPktThirdSysHeader=0x17,
	AntsPktThirdIFrames=0x18,
	AntsPktThirdPFrames=0x19,
	AntsPktThirdBBPFrames=0x1a,
	//!智能检测流帧类型
	AntsPktSmartIFrames=0x1b,
	AntsPktSmartPFrames=0x1c,
	AntsPktPlateInfoFrames=0x1d,
    // APP metedata帧.
    AntsPktAppFrames=0x20,
    // AntsComb 帧
    AntsPktAntsCombFrames=0x21,
	AntsPktDTMFFrames=0x22,
	AntsPktVIFrames=0xA1,// 虚拟I 帧
	AntsPktSubVIFrames=0xA2,
	AntsPktThirdVIFrames=0xA8,
}eAntsFrameType;

#define ANTS_FRAME_STARTCODE    0xAB010000
#define ANTS_FILE_STARTCODE    0xAA010000
#define ANTS_MOTION_STARTCODE    0xAC010000
#define ANTS_APP_STARTCODE    0xAD010000
#endif

typedef struct
{
    int nCode;
    char pDesc[40];

}RTSP_STATUS_CODE_T;

static RTSP_STATUS_CODE_T m_tStatusCode[]=
{
    {100,"Continue"},
    {200,"OK"},
    {201,"Created"},
    {250,"Low on Storage Space"},
    {300,"Multiple Choices"},
    {301,"Moved Permanently"},
    {302,"Moved Temporarily"},
    {303,"See Other"},
    {304,"Not Modified"},
    {305,"Use Proxy"},
    {400,"Bad Request"},
    {401,"Unauthorized"},
    {402,"Payment Required"},
    {403,"Forbidden"},
    {404,"Not Found"},
    {405,"Method Not Allowed"},
    {406,"Not Acceptable"},
    {407,"Proxy Authentication Required"},
    {408,"Request Time-out"},
    {410,"Gone"},
    {411,"Length Required"},
    {412,"Precondition Failed"},
    {413,"Request Entity Too Large"},
    {414,"Request-URI Too Large"},
    {415,"Unsupported Media Type"},
    {451,"Parameter Not Understood"},
    {452,"Conference Not Found"},
    {453,"Not Enough Bandwidth"},
    {454,"Session Not Found"},
    {455,"Method Not Valid in This State"},
    {456,"Header Field Not Valid for Resource"},
    {457,"Invalid Range"},
    {458,"Parameter Is Read-Only"},
    {459,"Aggregate operation not allowed"},
    {460,"Only aggregate operation allowed"},
    {461,"Unsupported transport"},
    {462,"Destination unreachable"},
    {500,"Internal Server Error"},
    {501,"Not Implemented"},
    {502,"Bad Gateway"},
    {503,"Service Unavailable"},
    {504,"Gateway Time-out"},
    {505,"RTSP Version not supported"},
    {551,"Option not supported"}
};
typedef struct
{
    int nPayloadType;
    int bVideo;
    int clockRate;
    char Name[16];

}PAYLOADTYPEMAP_T;
static PAYLOADTYPEMAP_T m_tPayloadTypes[]=
{
    {0,0,8000,"PCMU"},//G711U

    {3,0,8000,"GSM"},
    {4,0,8000,"G723"},
    {5,0,8000,"DVI4"},
    {6,0,16000,"DVI4"},
    {7,0,8000,"LPC"},
    {8,0,8000,"PCMA"},//G711A
    {9,0,8000,"G722"},
    {10,0,44100,"L16"},
    {11,0,44100,"L16"},
    {12,0,8000,"QCELP"},
    {13,0,8000,"CN"},
    {14,0,90000,"MPA"},
    {15,0,8000,"G728"},
    {16,0,11025,"DVI4"},
    {17,0,22050,"DVI4"},
    {18,0,8000,"G729"},

    {25,1,90000,"CelB"},
    {26,1,90000,"JPEG"},

    {28,1,90000,"nv"},

    {31,1,90000,"H261"},
    {32,1,90000,"MPV"},
    {33,1,90000,"MP2T"},
    {34,1,90000,"H263"},
    //96-127 dynamic
    {96,1,90000,"H264"},
    {97,0,8000,"G726-16"},
    {98,1,90000,"H265"},
    {99,1,8000,"mpeg4-generic"},
    {110,1,90000,"AntsComb"},

};
typedef struct
{
    unsigned char cDollar;
    unsigned char nCH;
    short nLen;
}DOLLAR_HEAER_T;


typedef enum
{
    RTSP_URL_PROTO_RTP,
    RTSP_URL_PROTO_RTSP,
    RTSP_URL_PROTO_URTSP,
    RTSP_URL_PROTO_HTTP,
    RTSP_URL_PROT_UNKNOW
}RTSP_URL_PROTO;

typedef enum
{
    RTSP_CMD_TYPE_OPTION = 1,
    RTSP_CMD_TYPE_DESCRIBE,
    RTSP_CMD_TYPE_SETUP,
    RTSP_CMD_TYPE_SETUP_V,
    RTSP_CMD_TYPE_SETUP_A,
    RTSP_CMD_TYPE_SETUP_APP,
    RTSP_CMD_TYPE_SETUP_TALK,
    RTSP_CMD_TYPE_PLAY,
    RTSP_CMD_TYPE_PAUSE,
    RTSP_CMD_TYPE_GET_PARAMETER,
    RTSP_CMD_TYPE_SET_PARAMETER,
    RTSP_CMD_TYPE_ANTSCOMB_ADDCH,
    RTSP_CMD_TYPE_ANTSCOMB_DECCH,
    RTSP_CMD_TYPE_TEARDOWN
}RTSP_CMD_TYPE;
#define ANTS_RTSP_MAX_DNSNAME_LEN  128
#define ANTS_RTSP_MAX_FILENAME_LEN  256
#define ANTS_RTSP_MAX_QUERY_NUM 20

#define RTSP_BASE_PORT_NUM (6970)

#define ANTS_RTSP_CALLBACKBYPE_STREAM   0
#define ANTS_RTSP_CALLBACKBYPE_RTCP   1
#define ANTS_RTSP_CALLBACKBYPE_STATUS   2
#define ANTS_RTSP_CALLBACKBYPE_ERROR   3

#define ANTS_RTSP_CALLBACKBYPE_STREAM_H264    0
#define ANTS_RTSP_CALLBACKBYPE_STREAM_MPEG4   1
#define ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG    2
#define ANTS_RTSP_CALLBACKBYPE_STREAM_G711A   3
#define ANTS_RTSP_CALLBACKBYPE_STREAM_G711U   4
#define ANTS_RTSP_CALLBACKBYPE_STREAM_G726    5
#define ANTS_RTSP_CALLBACKBYPE_STREAM_AAC     6
#define ANTS_RTSP_CALLBACKBYPE_STREAM_H265     7
#define ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB     8
#define ANTS_RTSP_CALLBACKBYPE_STREAM_PS     9
#define ANTS_RTSP_CALLBACKBYPE_STREAM_APP     20
#define ANTS_RTSP_CALLBACKBYPE_STREAM_DTMF     21




#define ANTS_RTSP_CALLBACKBYPE_RTCP_VIDEO   0
#define ANTS_RTSP_CALLBACKBYPE_RTCP_AUDIO   1

#define ANTS_RTSP_CALLBACKBYPE_ERROR_PAYLAODTYPE 18

#define ANTS_RTSP_FRAMETYPE_IFRAME           1
#define ANTS_RTSP_FRAMETYPE_PFRAME           2
#define ANTS_RTSP_FRAMETYPE_BFRAME           3

#define ANTS_RTSP_FRAMETYPE_AUDIO            8

#define ANTS_RTSP_FRAMETYPE_XML              10


#define ANTS_RTSP_TMP_BUFSIZE 1400

typedef int (*ANTS_RTPSESSION_CLIENT_CALLBACK) (void *hClient,int nCallbackType,int nSubType,int nProp,int nDataType, void *pData,int nDataLen,void *pUser);


typedef struct
{
    //protocol ://user:pwd@ hostname[:port] / path / [;parameters][?query]#fragment
    char nProtoType;//0-rtsp,1-urtsp
	char IsIPv6;
    char *pProtoType;
    char bDnsName;//0-dnsname,1-IP
    char *pUserName;
    char *pPassword;
    char *pAddress;//ip or name
    char *pPath;
    char *pPathName;
    char *pFileName;
    char *pParameters;
    char *pFragment;
    char *pPort;//0-[rtsp/urtsp:554]
    char *pQueryName[ANTS_RTSP_MAX_QUERY_NUM];
    char *pQueryValue[ANTS_RTSP_MAX_QUERY_NUM];
    char *pIgnore;
}RTSP_COMMON_URL_T;

// 天视通
typedef struct
{
    unsigned long flag;			//标记，固定为0x1a2b3c4d
    unsigned long data;			//最后面字节，即data & 0xff如果值不为0，则为当前实际帧率
    unsigned long frame_index;		//当前帧ID
    unsigned long keyframe_index;	//当前帧依赖的关键帧ID
}TST_VIDEO_FRAME_HEADER;


#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif
extern int Ants_Rtsp_Com_ParseUrl(char *pUrl,RTSP_COMMON_URL_T *pResult);
extern char *Ants_strndup(char *pstr,unsigned int nMaxLen);
extern void Ants_strFree(char *pstr);
extern int Ants_Rtsp_Com_ReleaseUrl(RTSP_COMMON_URL_T *pResult);
extern char* Ants_Rtsp_Base64Decode(char *base64code, unsigned int base64length,unsigned int *ResultSize);
extern char* Ants_Rtsp_Base64Encode(char *base64code, unsigned int base64length);
extern int Ants_Rtsp_IsBase64(char *base64code, unsigned int base64length);
extern const char* strstri(const char* str, const char* subStr);
extern void Ants_WaitTime(int nSec,int MicroSec);
extern int Ants_rtsp_CurrentTime(long *pSec,long *puSec);
extern int Ants_rtsp_GetSysRunTime(unsigned int *pSec,unsigned int *puSec);
extern int Ants_rtsp_GetNTPTime(unsigned int  *pmsw,unsigned int  *plsw);
extern struct tm * Ants_rtsp_LocalTime_r(time_t *t,struct tm *pTm);
extern int Ants_rtsp_GetRTP2NTPTime(unsigned int sec,unsigned int usec,unsigned int  *pmsw,unsigned int  *plsw);
extern int Ants_rtsp_GetNTP2RTPTime(unsigned int  msw,unsigned int  lsw,unsigned int *psec,unsigned int *pusec);
extern int Ants_rtsp_Payload2StreamType(int PayloadType);
extern int Ants_rtsp_Stream2PayloadType(int nStreamType);
extern int Ants_Rtsp_GetPayloadClockRate(int nPayloadType);
extern char *Ants_Rtsp_GetPayloadTypeName(int nPayloadType);

#ifdef __cplusplus
}
#endif


#endif
