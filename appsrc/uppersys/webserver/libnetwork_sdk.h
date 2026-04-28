#ifndef _LIBNETWORK_SDK_H_
#define _LIBNETWORK_SDK_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*NETWORKSDK_CALLBACK_FUNCTION_F)(void *pInParams,void **pOutParams);
typedef int (*NETWORKSDK_CALLBACK_STREAM_OPEN) (char *pUrl);
typedef int (*NETWORKSDK_CALLBACK_STREAM_CLOSE) (int hStreamHandle);
//typedef int (*NETWORKSDK_CALLBACK_STREAM_CONTROL) (int hStreamHandle,void *pInParam);
typedef int (*NETWORKSDK_CALLBACK_STREAM_READ) (int hStreamHandle,char **pData,int *pDataSize);
typedef int (*NETWORKSDK_CALLBACK_STREAM_RELEASE) (int hStreamHandle);

typedef struct
{
    NETWORKSDK_CALLBACK_FUNCTION_F           fxnCallFun;
    NETWORKSDK_CALLBACK_STREAM_OPEN          fxnStreamOpen;
    NETWORKSDK_CALLBACK_STREAM_CLOSE         fxnStreamClose;
    NETWORKSDK_CALLBACK_STREAM_READ          fxnStreamRead;
    NETWORKSDK_CALLBACK_STREAM_RELEASE       fxnStreamRelease;
    //NETWORKSDK_CALLBACK_STREAM_CONTROL     fxnStreamControl;
} NETWORKSDK_CALLBACK_T;

typedef int (*NETWORKSDK_INIT_F)(NETWORKSDK_CALLBACK_T *fxn);
typedef int (*NETWORKSDK_UNINIT_F)();
typedef int (*NETWORKSDK_CONFIG_F)(void *pInParams,void **pOutParams);
//typedef int	(*NETWORKSDK_CALLBACK_F)(NETWORKSDK_CALL_FUNCTION_F callfun);
//typedef int (*NETWORKSDK_STREAMCALLBACK_F)(NETWORKSDK_STREAMCONTROL *sc);

typedef struct
{
    NETWORKSDK_INIT_F      init;
    NETWORKSDK_UNINIT_F    uninit;
	NETWORKSDK_CONFIG_F    config;
//    NETWORKSDK_CALLBACK_F  callback;
//    NETWORKSDK_STREAMCALLBACK_F  streamCallback;
}NETWORKSDK_EXTERNAL_LIBS_T;

int libnetwork_sdk_init(NETWORKSDK_CALLBACK_T *fxn);
int libnetwork_sdk_uninit();
int libnetwork_sdk_config(void *pInParams,void **pOutParams);
//int libnetwork_sdk_callback(NETWORKSDK_CALL_FUNCTION_F callfun);
//int libnetwork_sdk_streamcallback(NETWORKSDK_STREAMCONTROL *sc);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETWORK_SDK_H_ */

