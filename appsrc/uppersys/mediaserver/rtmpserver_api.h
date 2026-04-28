#ifndef __RTMPSERVER_API_H__
#define __RTMPSERVER_API_H__


#define MAX_RTMPSERVER_NUM            1
#define MAX_RTMPSERVER_STREAM_NUM     100

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

//编码类型
#define ANTS_RTMPSERVER_CODECTYPE_G711U               (0)
#define ANTS_RTMPSERVER_CODECTYPE_G711A               (8)
#define ANTS_RTMPSERVER_CODECTYPE_MJPEG               (26)
#define ANTS_RTMPSERVER_CODECTYPE_H264                (96)
#define ANTS_RTMPSERVER_CODECTYPE_G726_16             (97)


//帧类型
#define ANTS_RTMPSERVER_FRAMETYPE_VIDEO_P             (0)
#define ANTS_RTMPSERVER_FRAMETYPE_VIDEO_KEY           (1)
#define ANTS_RTMPSERVER_FRAMETYPE_AUDIO               (2)

//流类型
#define ANTS_RTMPSERVER_STREAMTYPE_LIVE               (0)
#define ANTS_RTMPSERVER_STREAMTYPE_FILE                (1)

//回调类型
#define ANTS_RTMPSERVER_STREAM_START                  (0)
#define ANTS_RTMPSERVER_STREAM_STOP                   (1)
#define ANTS_RTMPSERVER_STREAM_PAUSE                   (2)// &pause=[0,1]
#define ANTS_RTMPSERVER_STREAM_SPEED                   (3) // &speed=[..-8,-4,-2,-1,0,1,2,4,8,16,32]
#define ANTS_RTMPSERVER_STREAM_STEP                   (4)
#define ANTS_RTMPSERVER_STREAM_BUFFSTATUS               (5) 
#define ANTS_RTMPSERVER_STREAM_LIVEID                 (6) 
#define ANTS_RTMPSERVER_STREAM_FILEID                 (7) 

// 推流方式 
#define ANTS_RTMP_PUSH_MAX_STREAM_NUM 32


//不带头的帧信息
typedef struct {
    unsigned int  uiFrameType;
	unsigned int  uiCodecID;
	unsigned int  uiWidth;
	unsigned int  uiHeight;
	unsigned int  timestamp;
	unsigned char unRes[32];
}rtmp_stream_info;

typedef int (*ANTS_RTMPServer_STREAM_CALLBACK)(int hRtmpHandle, char *pToken, int nDataSizeint, int ntype,int nStreamType,void *pRtmpStreamHandle, void *pUser);

//用户可以通过pStreamUser返回一个句柄hId，后面的接口都会把这个句柄回传，pRtmpStreamHandle是每个连接的唯一标志
typedef int (*ANTS_RTMPServer_OPEN_CALLBACK)(int hRtmpHandle, void *pRtmpStreamHandle, char *pToken, int nDataSizeint, int nStreamType, void **pStreamUser, void *pUser);
typedef int (*ANTS_RTMPServer_CLOSE_CALLBACK)(int hRtmpHandle, void *pRtmpStreamHandle, void *pStreamUser, int nStreamType);
//nReadType为1时，表示传的ppBuffer是带ants头信息的，nReadType为2时，表示传的ppBuffer是不带ants头信息的，此时需要用户填充pStreamInfo字段，结构为rtmp_stream_info
typedef int (*ANTS_RTMPServer_READ_CALLBACK)(int hRtmpHandle, void *pRtmpStreamHandle, void *pStreamUser, int nStreamType, char **ppBuffer, unsigned int *dwBuffsize, void *pStreamInfo, int *nReadType);
typedef int (*ANTS_RTMPServer_RELEASE_CALLBACK)(int hRtmpHandle, void *pRtmpStreamHandle, void *pStreamUser, int nStreamType);
//此接口只对历史流有用，实时流的时候可以不设此回调
typedef int (*ANTS_RTMPServer_CONTROL_CALLBACK)(int hRtmpHandle, void *pRtmpStreamHandle, void *pStreamUser, int ntype, char *pToken, int nDataSizeint);


typedef struct {
    ANTS_RTMPServer_OPEN_CALLBACK    fxnOpen;
	ANTS_RTMPServer_CLOSE_CALLBACK   fxnClose;
	ANTS_RTMPServer_READ_CALLBACK    fxnRead;
	ANTS_RTMPServer_RELEASE_CALLBACK fxnRelease;
	ANTS_RTMPServer_CONTROL_CALLBACK fxnControl;
	unsigned char                    unRes[32];	
}RTMPServer_StreamControl;


// 推流

typedef void * RTMPPUSH_HANDLE;

typedef int (*ANTS_RTMPPUSH_OPEN_CALLBACK)(RTMPPUSH_HANDLE hRtmpPushHandle, void **pStreamUser, void *pUser);

/* close ;
	nType:0-连接 失败,1-远程正常关闭,2,-本地正常关闭,3-网络异常关闭,4- 本地流打开失败,5- 不知道的控制命令,6- 是否重连
*/
#define ANTS_RTMPPUSH_CLOSE_TYPE_CONNECT_FAIL   0
#define ANTS_RTMPPUSH_CLOSE_TYPE_REMOTE_CLOSE   1
#define ANTS_RTMPPUSH_CLOSE_TYPE_LOCAL_CLOSE   2
#define ANTS_RTMPPUSH_CLOSE_TYPE_NET_FAIL   3
#define ANTS_RTMPPUSH_CLOSE_TYPE_OPENSTREAM_FAIL   4
#define ANTS_RTMPPUSH_CLOSE_TYPE_UNKNOW_CONTROL        5
#define ANTS_RTMPPUSH_CLOSE_TYPE_RECONNECT        6


// control type
#define ANTS_RTMPPUSH_CONTROL_TYPE_ENABLE_AUDIO    0    // enable audio  (int *)

typedef int (*ANTS_RTMPPUSH_CLOSE_CALLBACK)(RTMPPUSH_HANDLE hRtmpPushHandle,int nType, void **pStreamUser,void *pUser);
//nReadType为1时，表示传的ppBuffer是带ants头信息的，nReadType为2时，表示传的ppBuffer是不带ants头信息的，此时需要用户填充pStreamInfo字段，结构为rtmp_stream_info
typedef int (*ANTS_RTMPPUSH_READ_CALLBACK)(RTMPPUSH_HANDLE hRtmpPushHandle,char **ppBuffer, int *dwBuffsize, void *pStreamInfo, int *nReadType, void *pStreamUser, void *pUser);
typedef int (*ANTS_RTMPPUSH_RELEASE_CALLBACK)(RTMPPUSH_HANDLE hRtmpPushHandle, void *pStreamUser, void *pUser);
//此接口只对历史流有用，实时流的时候可以不设此回调
typedef int (*ANTS_RTMPPUSH_CONTROL_CALLBACK)(RTMPPUSH_HANDLE hRtmpPushHandle, int ntype, char *pData, int nDataSize,void *pStreamUser,void *pUser);

typedef struct {
	ANTS_RTMPPUSH_OPEN_CALLBACK    fxnOpen;
	ANTS_RTMPPUSH_CLOSE_CALLBACK   fxnClose;
	ANTS_RTMPPUSH_READ_CALLBACK    fxnRead;
	ANTS_RTMPPUSH_RELEASE_CALLBACK fxnRelease;
	ANTS_RTMPPUSH_CONTROL_CALLBACK fxnControl;
	unsigned char                    unRes[32];	
}RTMPPush_StreamControl;


#ifdef __cplusplus
extern "C" {
#endif
//成功返回1，失败返回0
int Ants_RTMPServer_Init();
int Ants_RTMPServer_UnInit();


//创建RTSP服务器
int Ants_RTMPServer_Start(unsigned short wListPort);//返回hRtmpHandle
int Ants_RTMPServer_Stop(int hRtmpHandle);

int Ants_RTMPServer_SetStreamCallBack(int hRtmpHandle, ANTS_RTMPServer_STREAM_CALLBACK fxn, void *pUser);

//输入流
int Ants_RTMPServer_InputData(int hRtmpHandle, void *pRtmpStreamHandle, void *pData, unsigned int nDataSize, void *pStreamInfo);
int Ants_RTMPServer_InputAntsData(int hRtmpHandle, void *pRtmpStreamHandle, void *pData,unsigned int nDataSize);

int Ants_RTMPServer_SetStreamCallBackV2(int hRtmpHandle, RTMPServer_StreamControl *pStreamControl, void *pUser);

// 推流方式


int Ants_RTMPPush_Open(RTMPPUSH_HANDLE *hRtmpPushHandle,char *szUrl,char *szUserName,char *szPassword,RTMPPush_StreamControl *fxns,void *pUser);
int Ants_RTMPPush_Start(RTMPPUSH_HANDLE hRtmpPushHandle);
int Ants_RTMPPush_Control(RTMPPUSH_HANDLE hRtmpPushHandle,int nType,char *byData,int nDataSize);
int Ants_RTMPPush_Stop(RTMPPUSH_HANDLE hRtmpPushHandle);
int Ants_RTMPPush_Close(RTMPPUSH_HANDLE *hRtmpRushHandle);

#ifdef __cplusplus
}
#endif

#endif

