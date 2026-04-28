#ifndef _RTSPSERVERNEW_H_
#define _RTSPSERVERNEW_H_

#ifdef WIN32
#define RTSSERVERNEWAPI __declspec(dllexport)
#else
#define RTSSERVERNEWAPI
#endif



#ifdef WIN32
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#include <sys/types.h>
#endif


#ifndef __ANTS_RTSP_DAYTIME__
#define __ANTS_RTSP_DAYTIME__
typedef struct _Ants_RtspDayTime
{
    unsigned short wYear;
    unsigned char byMon;
    unsigned char byDay;

    unsigned char byHour;
    unsigned char byMin;
    unsigned char bySec;
    unsigned char byZone;

    unsigned short wMsec;
    unsigned short wRes;

} Ants_RtspDayTime;
#endif


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
	AntsPktVIFrames=0xA1,// 虚拟I 帧
	AntsPktSubVIFrames=0xA2,
	AntsPktThirdVIFrames=0xA8,
}eAntsFrameType;
#define ANTS_FRAME_STARTCODE    0xAB010000
#define ANTS_FILE_STARTCODE    0xAA010000
#define ANTS_MOTION_STARTCODE    0xAC010000
#define ANTS_APP_STARTCODE    0xAD010000
#endif

typedef enum
{
	ANTS_RTSPSERVER_ERROR_NoError = 0,
	ANTS_RTSPSERVER_ERROR_NoAnyData,
	ANTS_RTSPSERVER_ERROR_MallocError,
	ANTS_RTSPSERVER_ERROR_BufferTooSmall,
	ANTS_RTSPSERVER_ERROR_SocketClose,
	ANTS_RTSPSERVER_ERROR_SocketError = 5,
	ANTS_RTSPSERVER_ERROR_InvSeqNo,
	ANTS_RTSPSERVER_ERROR_ResError,
	ANTS_RTSPSERVER_ERROR_ParseError,
	ANTS_RTSPSERVER_ERROR_InvSdpParam,
	ANTS_RTSPSERVER_ERROR_SDP_PlayloadType = 10,
	ANTS_RTSPSERVER_ERROR_SDP_Rtpmap,
	ANTS_RTSPSERVER_ERROR_InvSessionID,
	ANTS_RTSPSERVER_ERROR_InvTransPort,
	ANTS_RTSPSERVER_ERROR_InvStreamDataLen,
	ANTS_RTSPSERVER_ERROR_Authenticate = 15,
	ANTS_RTSPSERVER_ERROR_NoSupportIPV6,
	ANTS_RTSPSERVER_ERROR_InvalidMulticast,
	ANTS_RTSPSERVER_ERROR_DisMatchPlayloadType = 18,
	ANTS_RTSPSERVER_ERROR_Close_Normal = 19,
}ANTS_RTSPSERVER_ERROR_TYPE;

typedef struct _Ants_RTSPV2_InParam
{
    char *pUrl;
    unsigned int dwClientIP;
    short wRemotePort;
    short wSize; // 结构大小
    unsigned int dwSessionID;// 会话ID
    char *pRequire; // 双向流  时有效

    int bRecording;// 回放请求.
    int nStreamType;// 0-VA,1-V,2-A,3-VA
    int nChan; // 0(0通道编码),1,2...通道
    int nStream;//1,2,主/子码流
    int bRateControl;// -1 此值不存在，意义同1(比如实时流)；0- 客户端控制回放速度,服务器尽可能快的发送数据;1-服务器控制回放速度
    // 回放时为0;
    int nFrame:8; // 0-all ;1-I Frame,2- I+P Frame
    int nInterval:24;// msec I帧间隔最小时间.bFrame = 1时有效.
    int nScale;
    int bPause;// 0无效,1-暂停
    int nPlaySeq;
    int bRangeValid;// bit0-start,bit1-stop
    Ants_RtspDayTime tRangeStart;
    Ants_RtspDayTime tRangeStop;

    int nAppPayloadType;
    int nAntsComb;
    char *pChChange;
	int nReason;
	int bIPv6;
	unsigned int dwClientIPv6[4];
} Ants_RTSPV2_InParam_T;

typedef struct _Ants_RTSPV2_OutParam
{
    int nRet;// 0- 成功,
    int hStreamHandle;
    char *pDescribe;
    int nRequireType;// 是否接收REQUIRE;如果pRequire有效的话，必须要处理,否则认为不支持而关闭流. 0- 不处理/无效;1-视频,2-音频;3-音视频
    int nRequireVideoType;
    int nRequireAudioType;

    int nCh;
    int nStream;
} Ants_RTSPV2_OutParam_T;



typedef struct _Ants_RTSPV2_AuthInParam
{
    char *pUrl;
    unsigned int dwClientIP;
    short wRemotePort;
    short wSize; // 结构大小
    unsigned int dwSessionID;// 会话ID
    char *pUserName;// 验证用户名,可能为NULL
    int bRecording;// 回放请求.
	// auth base64
	int nAuthType; // 0-无验证，1-base64,2-http-digest
	char *pPassword;
	//digest
	char *pRealm;
	char *pNonce;
	char *pUri;
	char *pResponse;
	char *pCmd;
	unsigned int dwClientIPv6[4];

} Ants_RTSPV2_AuthInParam_T;

typedef struct _Ants_RTSPV2_AuthOutParam
{
    int nAuthType; // -1: 不验证,0:自动(由客户端决定),1:base64,2:http-Digest(强制),3:Ext-Base64(外部验证),4-Ext-Digest(外部验证)
    char *pUserName;// 验证用户名,可能为NULL,malloc
    char *pPassword; // 对应的密码 ,malloc
	int bAuthOk;// 0- 验证失败，1-验证成功 (type = 3,4时有效)

} Ants_RTSPV2_AuthOutParam_T;


typedef struct
{
    unsigned int  uiFrameType;
    unsigned int  uiCodecID;
    unsigned int  uiWidth;
    unsigned int  uiHeight;
    unsigned int  timestamp;
    unsigned short wCh;
    unsigned char byStream;
    unsigned char unRes[29];
} rtsp_stream_info;


#ifndef ANTS_RTSP_COM_DEFINE
#define ANTS_RTSP_COM_DEFINE

#define MAX_RTSPSERVER_NUM     10
#define MAX_RTSPSERVER_STREAM_NUM     512

#define ANTS_RTSPSERVER_PAYLOADTYPE_G711U             (0)
#define ANTS_RTSPSERVER_PAYLOADTYPE_G711A             (8)
#define ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG             (26)
#define ANTS_RTSPSERVER_PAYLOADTYPE_H264             (96)
#define ANTS_RTSPSERVER_PAYLOADTYPE_G726_16             (97)
#define ANTS_RTSPSERVER_PAYLOADTYPE_H265             (98)
#define ANTS_RTSPSERVER_PAYLOADTYPE_AAC             (99)
#define ANTS_RTSPSERVER_PAYLOADTYPE_APP             (106)
#define ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB         (110) // name="AntsComb"



//流类型

#define ANTS_RTSPSERVER_STREAMTYPE_ALL             0
#define ANTS_RTSPSERVER_STREAMTYPE_VIDEO           1
#define ANTS_RTSPSERVER_STREAMTYPE_AUDIO           2
#define ANTS_RTSPSERVER_STREAMTYPE_APP              4

#define ANTS_RTSPSERVER_FRAMETYPE_IFRAME           1
#define ANTS_RTSPSERVER_FRAMETYPE_PFRAME           2
#define ANTS_RTSPSERVER_FRAMETYPE_BFRAME           3

#define ANTS_RTSPSERVER_FRAMETYPE_AUDIO            8

#define ANTS_RTSPSERVER_FRAMETYPE_XML              10



//源类型
#define ANTS_RTSPSERVER_SOURCETYPE_LIVE            (0)
#define ANTS_RTSPSERVER_SOURCETYPE_FILE            (1)
#define ANTS_RTSPSERVER_SOURCETYPE_BACK            (2)

//回调类型

// 告知上层当前请求的URL,
// hRtspHandle:RTSP服务器句柄
// hStreamHandle:忽略
// pInParam:Ants_RTSPV2_InParam_T
// pOutParam:Ants_RTSPV2_OutParam_T
// 返回：0-成功，-1失败

#define ANTS_RTSPSERVER_CALLBACK_TYPE_OPEN_URL  1 // 打开流,可用于打开相应流，如果没有打开的话
#define ANTS_RTSPSERVER_CALLBACK_TYPE_CLOSE_URL  2 // 关闭流,可用于关闭相应流，如果没有其他会话在使用的话

#define ANTS_RTSPSERVER_CALLBACK_TYPE_SET_PARAM  3
#define ANTS_RTSPSERVER_CALLBACK_TYPE_SET_APP    4

#define ANTS_RTSPSERVER_CALLBACK_TYPE_ADDCH    5 // 添加通道
#define ANTS_RTSPSERVER_CALLBACK_TYPE_DECCH    6 // 减少通道

#define ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING   8 //开始Play，可用于强制关键帧
#define ANTS_RTSPSERVER_CALLBACK_TYPE_STOPPING  9 //停止播放

#define ANTS_RTSPSERVER_CALLBACK_TYPE_AUTH     10 // 验证

#define ANTS_RTSPSERVER_CALLBACK_TYPE_SENDFAIL_ASKING  16 //发送数据失败,询问是否继续发送,返回0-不发送，1-继续发送


// 类型类型
#define ANTS_RTSPSERVER_DATATYPE_SYSHEAD							1		//!系统头数据
#define ANTS_RTSPSERVER_DATATYPE_STREAMDATA						2		//!视频流数据（包括复合流和音视频分开的视频流数据）
#define ANTS_RTSPSERVER_DATATYPE_AUDIOSTREAMDATA			        3		//!音频流数据
#define ANTS_RTSPSERVER_DATATYPE_STD_VIDEODATA				    4		//!标准视频流数据
#define ANTS_RTSPSERVER_DATATYPE_STD_AUDIODATA				    5		//!标准音频流数据
#define ANTS_RTSPSERVER_DATATYPE_TALK_AUDIODATA				    6		//!对讲音频流数据




// 来自客户端的流(双向流)
// dwSessionID：某个会话ID
typedef void (*ANTS_RTSPSERVER_STREAMDATA)(int hStreamHandle,unsigned int dwSessionID,unsigned long dwDataType,unsigned long dwProp,unsigned char *lpBuffer,unsigned long dwBufSize,void* lpUser);


typedef int (*ANTS_RTSPSTATUSCALLBACK) (int hRtspHandle,int hSessionHandle,int nType, void *pInParam,void *pOutParam,void *pUser);
#endif
// 读数据回调
// 返回值:0-成功,-1失败,
typedef int (*ANTS_RTSP_CALLBACK_OPEN) (int hRtspHandle,int hStreamHandle,void *pInParam,void *pOutParam,void **pStreamUser,void *pUser);

typedef int (*ANTS_RTSP_CALLBACK_CLOSE) (int hRtspHandle,int hStreamHandle,void *pStreamUser,int nStreamType,char *pUrl);
typedef int (*ANTS_RTSP_CALLBACK_CONTROL) (int hRtspHandle,int hStreamHandle,int nType,void *pInParam,void *pOutParam,void *pStreamUser);

typedef int (*ANTS_RTSP_CALLBACK_READ) (int hRtspHandle,int hStreamHandle,void *pStreamUser,int nStreamType,char **pData,int *pDataSize,void *pStreamInfo, int *nReadType, int *bAdpcm2G711u);
typedef int (*ANTS_RTSP_CALLBACK_READ_RELEASE) (int hRtspHandle,int hStreamHandle,void *pStreamUser,int nStreamType);


typedef struct
{
    ANTS_RTSP_CALLBACK_OPEN          fxnOpen;
    ANTS_RTSP_CALLBACK_CLOSE         fxnClose;
    ANTS_RTSP_CALLBACK_READ          fxnRead;
    ANTS_RTSP_CALLBACK_READ_RELEASE  fxnRelease;
    ANTS_RTSP_CALLBACK_CONTROL       fxnControl;
    unsigned char                    unRes[32];
} RTSPServer_StreamControl;


#ifdef __cplusplus
extern "C" {
#endif
RTSSERVERNEWAPI int Ants_RTSPServerV2_Init();
//创建RTSP服务器
RTSSERVERNEWAPI int Ants_RTSPServerV2_Start(int nPort);//返回handle

RTSSERVERNEWAPI int Ants_RTSPServerV2_SetTunnelingOverHTTP(int hRtspHandle,int nHttpPort);
//创建流
// [rtsp,urtsp]://[ip,dnsname][:port]/file?ctype=[video;audio;videoback;audioback;appxxx]&stype=[unicast,multicast]&ptype=[tcp,http,udp]&ip=[224.x.x.x]&port=x
// 回放
//  rtsp://192.168.25.84/recordstream?starttime=20140627T154000Z&endtime=20140627T154002Z // 兼容IPC
//  rtsp://192.168.25.84/recording?ch=0&stream=0&start=20140627154000&stop=20140627154002
//  rtsp://192.168.25.84/recording_comb?ch=0&stream=0&start=20140627154000&stop=20140627154002 // 多通道回放
//  rtsp://192.168.25.84/living_comb01.264 // 多通道实时流

// "ANTS_ADDCH" "ANTS_DECCH"
// "Channel-Number: 3;9;11"
//int Ants_RTSPServer_CreateStream(int hRtspHandle,char *pToken,int bStreamType,int nVideoPayloadType,int nAudioPayloadType,int bFixed);//返回流handle

// 设置读数据回调
//
//int Ants_RTSPServer_SetReadDataCallback(int hRtspHandle,int hStreamHandle,ANTS_RTSP_READDATA_CALLBACK fRead,ANTS_RTSP_READDATA_RELEASE_CALLBACK fRelease,void *pUser);
//


//添加或者改变
RTSSERVERNEWAPI int Ants_RTSPServerV2_AddAppStream(int hRtspHandle,int hSessionHandle,int nAppPayloadType,unsigned int dwClockRate,char *szAppName);
//删除
//int Ants_RTSPServer_DeleteAppStream(int hStreamHandle,int nAppPayloadType);
//nCmdType:
//          1- 设置流类型 ,ANTS_RTSPSERVER_STREAMTYPE_ALL/
//          2- 设置视频负载类型自动 1-自动,0-非自动
//          3- 设置音频负载类型自动 1-自动,0-非自动
//          4- 设置视频负载类型
//          5- 设置音频负载类型
//          6- 设置流为录像源。 0-实时流,1-历史流
//          7- 设置操作影响的play Seq; // 从回调中得到
//          8- 设置是否发送数据流失败操作是否询问,0-不问，1-问.
//          9- 设置recordstream地址是否为新的回放方式,0-老方式，1-新方式，新方式流是内部创建好的，不用外部创建.使用此命令时参数hStreamHandle=hRtspHandle
//          10-设置帧率
//          11- TCP流创建方式,0-外部方式，1-内部方式，新方式流是内部创建好的，不用外部创建,实时流默认外部创建,历史流默认内部创建.使用此命令时参数hStreamHandle=hRtspHandle
//          12- SPS (h264/h265) // 不带 00 00 00 01  ->Ants_RTSPServer_SetConfigV2
//          13- PPS (h264/h265) 不带 00 00 00 01     -> Ants_RTSPServer_SetConfigV2
//          14- VPS (h265) 不带 00 00 00 01          ->Ants_RTSPServer_SetConfigV2
//          15- video SSRC_ID
//          16- audio SSRC_ID
//          17- 端口复用 pData[0] = socket ,nDatasize = sizeof(int)
//          18- video rtcp消息发送
//          19- audio rtcp消息发送

RTSSERVERNEWAPI int Ants_RTSPServerV2_SetConfig(int hRtspHandle,int hStreamHandle,int nCmdType,void *pData,int nDataSize);//
RTSSERVERNEWAPI int Ants_RTSPServerV2_SetStreamInfo(int hRtspHandle, int hSessionHandle,int bStreamType,int nVideoPayloadType,int nAudioPayloadType);
RTSSERVERNEWAPI int Ants_RTSPServerV2_InputDataEx(int hRtspHandle,int hStreamHandle,int nPayloadType,void *pData,int nDataSize,int64_t timestamp,int bRelTimeStamp);

// bTimeStampValid :bit0 - 1:Reltimestamp 有效; bit1 -1:AbstimestampSec/AbstimestampUSec 有效,绝对时间一般用于回放
RTSSERVERNEWAPI int Ants_RTSPServerV2_InputDataV2(int hRtspHandle,int hStreamHandle,int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid);

// "ANTSCOMB_ADDCH" "ANTSCOMB_DECCH"
// "Channel-Number: 3;9;11;-1"
// 负的表示子码流
// nType :0
// nChan/nStreamIdx:透传
RTSSERVERNEWAPI int Ants_RTSPServerV2_InputAntsCombData(int hRtspHandle,int hStreamHandle,int nType,int nChan,int nStreamIdx,void *pData,int nDataSize);

RTSSERVERNEWAPI int Ants_RTSPServerV2_SetStatusCallBack(ANTS_RTSPSTATUSCALLBACK fxn,void *pUser);

// dwProp : bit0(内存管理模式:0-由调用者释放，1-由被调用者释放);
// dwSessionID:-1总的回调，否则是对某一个会话设置回调。由状态回调获得。
RTSSERVERNEWAPI int Ants_RTSPServerV2_SetStreamCallBack(int hRtspHandle,unsigned int dwSessionID,unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser);
//输入流
RTSSERVERNEWAPI int Ants_RTSPServerV2_InputData(int hRtspHandle,int hStreamHandle,void *pData,int nDataSize,int64_t timestamp,int bAudio,int bAdpcm2G711u);

RTSSERVERNEWAPI int Ants_RTSPServerV2_InputAntsData(int hRtspHandle,int hStreamHandle,void *pData,int nDataSize,int bAdpcm2G711u);


// 指定某个客户端会话
//int Ants_RTSPServer_InputDataBySession(int hStreamHandle,unsigned int dwSessionID,void *pData,int nDataSize,int64_t timestamp,int bAudio,int bAdpcm2G711u);
//int Ants_RTSPServer_InputAntsDataBySession(int hStreamHandle,unsigned int dwSessionID,void *pData,int nDataSize,int bAdpcm2G711u);
// 组播
RTSSERVERNEWAPI int Ants_RTSPServerV2_SetMulticastAddressIPv4(int hRtspHandle,int hSessionHandle,int bAudio,const char *pIP,int nPort,int nTTL);
//int Ants_RTSPServer_SetMulticastAddressIPv4NoApply(int hStreamHandle,int bAudio,const char *pIP,int nPort,int nTTL);

// hStreamHandle == -1:表示流无关。
RTSSERVERNEWAPI int Ants_RTSPServerV2_EnableMulticast(int hRtspHandle,int hSessionHandle,int bEnable);

RTSSERVERNEWAPI int Ants_RTSPServerV2_CloseStream(int hRtspHandle, int hSessionHandle);
//销毁流
//int Ants_RTSPServer_DestroyStreamByToken(char *pToken);
//int Ants_RTSPServer_DestroyStreamByHandleToken(int hRtspHandle,char *pToken);
//int Ants_RTSPServer_DestroyStream(int hStreamHandle);
//销毁RTSP服务器
RTSSERVERNEWAPI int Ants_RTSPServerV2_Stop(int hRtspHandle);



RTSSERVERNEWAPI int Ants_RTSPServerV2_SetStreamControlCallBack(int hRtspHandle, RTSPServer_StreamControl *pStreamControl, void *pUser);

#ifndef ANTS_RTSP_COM_INTERFACE_DEFINE
#define ANTS_RTSP_COM_INTERFACE_DEFINE
typedef struct  _tagANTS_RTSP_H264_RTP_PARAM
{
    // input params
     int nMTUSize; // <= 1500
     int nWidth;
     int nHeight;
     uint8_t *pInData[16];
     int nInDataSize[16];
     int nInCount;
     int bIFrame;
     uint32_t Reltimestamp;
     uint32_t AbstimestampSec;
     uint32_t AbstimestampUSec;
     uint8_t *pOutData;// out,1Mbytes
     int nOutSize; // in/out
     uint32_t dwSSRC;
     uint32_t dwCodecType; // 0-264,1-265,2-g711u,3-g711a,4-mjpeg
     uint32_t dwRes[7];


     // ----no change---
     uint32_t dwFrameNo;
     int nStartIdx;
     int nStartPos;
     int nStopIdx;
     int nStopPos;
     uint16_t wSeq;
     int discontinuity;
     int eSectionEnd;
     int cIFrameStart;
     uint32_t ntpTimeStamp0;
     uint32_t ntpTimeStamp1;
     int nWritePos;
     uint32_t dwRes1[8];
}ANTS_RTSP_H264_RTP_PARAM_T;
// 264 接口可以通用于其他类型
RTSSERVERNEWAPI int Ants_RTSPServer_H264toRTP_InputData(ANTS_RTSP_H264_RTP_PARAM_T *pParam);
RTSSERVERNEWAPI int Ants_RTSPServer_H265toRTP_InputData(ANTS_RTSP_H264_RTP_PARAM_T *pParam);

typedef struct  _tagANTS_RTSP_RTP_H264_PARAM
{
    // input params
    uint8_t *pInData;
    int nInDataSize;


    uint8_t *pOutData;// out,1Mbytes
    int nOutSize; // in/out
    uint8_t byRes[32];

}ANTS_RTSP_RTP_H264_PARAM_T;
RTSSERVERNEWAPI int Ants_RTSPServer_RTPtoH264_InputData(ANTS_RTSP_RTP_H264_PARAM_T *pParam);
RTSSERVERNEWAPI int Ants_RTSPServer_RTPtoH265_InputData(ANTS_RTSP_RTP_H264_PARAM_T *pParam);

RTSSERVERNEWAPI int64_t Ants_RTSPServer_GetCurrentTime();

RTSSERVERNEWAPI void Ants_RTSPServer_WaitTime(int nSec,int MicroSec);

#endif


#ifdef __cplusplus
}
#endif

#endif

