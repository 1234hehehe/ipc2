#ifndef __WSSERVER_API_H__
#define __WSSERVER_API_H__

#define    WEBSOCKET_REQUEST_TYPE_REAL            0
#define    WEBSOCKET_REQUEST_TYPE_RECORD          1
#define    WEBSOCKET_REQUEST_TYPE_ALARM           2
#define    WEBSOCKET_REQUEST_TYPE_TALKING         3

typedef int (*ANTS_WSServer_OPEN_CALLBACK) (void *pInParam/*,void *pOutParam*/);

typedef int (*ANTS_WSServer_CLOSE_CALLBACK) (int hStreamHandle);
typedef int (*ANTS_WSServer_CONTROL_CALLBACK) (int hStreamHandle,int nType,void *pInParam,void *pOutParam);

typedef int (*ANTS_WSServer_READ_CALLBACK) (int hStreamHandle,void *pInParam,int *nReqIFrame,void **pData,int *pDataSize);
typedef int (*ANTS_WSServer_WRITE_CALLBACK) (int hStreamHandle,void *pData,int nDataSize);

typedef int (*ANTS_WSServer_RELEASE_CALLBACK) (int hStreamHandle);

typedef struct {
    ANTS_WSServer_OPEN_CALLBACK    fxnOpen;
	ANTS_WSServer_CLOSE_CALLBACK   fxnClose;
	ANTS_WSServer_READ_CALLBACK    fxnRead;
    ANTS_WSServer_WRITE_CALLBACK    fxnWrite;
	ANTS_WSServer_RELEASE_CALLBACK fxnRelease;
	ANTS_WSServer_CONTROL_CALLBACK fxnControl;
    unsigned char                  unRes[32];
}WSServer_StreamControl;

#ifdef __cplusplus
extern "C" {
#endif

int WSServerInit();
int WSServerUnInit();
int WSServerStart();
int WSServerStop();
int WSServerSetStreamControlCallBack(int hWsHandle, WSServer_StreamControl *pStreamControl, void *pUser);
int WSServerSendAlarm(void *pEventInfo);

#ifdef __cplusplus
}
#endif

#endif
