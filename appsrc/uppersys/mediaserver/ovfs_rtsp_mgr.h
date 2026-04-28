/**
 * @file ovfs_rtsp_mgr.h
 * @date create on: 2016年10月11日
 * @author eric
 * @brief 
 * 
 * @defgroup ovfs_rtsp_mgr ovfs_rtsp_mgr
 * @{
 *  @note
 *
 */
#ifndef OVFS_RTSP_MGR_H_
#define OVFS_RTSP_MGR_H_

#include <libcommon_api.h>
#include <cjson.h>

typedef struct
{
    int enable;
    int streamMaxNum;
    int rtspPort;
    int httpPort;
    int authMask;               // MEDIA_AUTH_TYPE_TEXT, MEDIA_AUTH_TYPE_DIGEST , MEDIA_AUTH_TYPE_TEXT | MEDIA_AUTH_TYPE_DIGEST
    int multiEnable;

    char multiMainVideoIp[16];
    int multiMainVideoPort;
    int multiMainVideoTtl;

    char multiSubVideoIp[16];
    int multiSubVideoPort;
    int multiSubVideoTtl;

    char multiThirdVideoIp[16];
    int multiThirdVideoPort;
    int multiThirdVideoTtl;
    char multiMainAudioIp[16];
    int multiMainAudioPort;
    int multiMainAudioTtl;

    char multiSubAudioIp[16];
    int multiSubAudioPort;
    int multiSubAudioTtl;
    char multiThirdAudioIp[16];
    int multiThirdAudioPort;
    int multiThirdAudioTtl;

} RTSP_CONFIG_T;

int RtspMgr_Init(MQ_HANDLE_H mqHandlee);
int RtspMgr_Start();
int RtspMgr_Stop();
int RtspMgr_Restart();
int RtspMgr_SendAppData(MEDIA_ALARM_STATUS_T * alarmStatusInfo, int sendAll);
int RtspMgr_UpdateBoardState(MEDIA_BOARD_STATE_T boardState);
int RtspMgr_GetDebugPrint();
int RtspMgr_SetDebugPrint(int enable);

#endif /* OVFS_RTSP_MGR_H_ */
/**
 * @}
 */
