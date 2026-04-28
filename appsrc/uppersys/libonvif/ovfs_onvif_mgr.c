/**
 *   \file ovfs_onvif_mgr.c
 *   \brief
 *
 *  Detailed description
 *  simple onvif manage file
 *  use the following command to format source code:
 *  astyle   --style=bsd  --convert-tabs --indent=spaces=4 -p -xC110 -z2 -k3 -W3 *.c *.h *.cpp
 *
 *
 */

// #include "config.h"
#include "ovfs_onvif_mgr.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>

#include <libcommon_api.h>

#include <net/route.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>

#include <netdb.h>
#include <net/ethernet.h>
#include <linux/if_arp.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <mxml.h>
//#include <linux/route.h>
#include <netinet/ip_icmp.h> //struct icmp


#include "ovfs_com_utils.h"
#include "ovfs_onvif.h"
#include "ovfs_onvif_parser.h"


#define HW_ADDR_BUF_LEN 32
#define IPV4_ADDR_BUF_LEN 32
#define ONVIF_EVENT_UPDATE_TIME_DELAY 86400000LLU //150 seconds

#define ADAPTIVEIP_MAXTIME 3600000LLU * 24LLU

enum
{
    ONVIF_MGR_STATE_UNINIT = 0,
    ONVIF_MGR_STATE_INIT,
    ONVIF_MGR_STATE_START,
};

typedef struct
{
    COMMON_DLIST_T sendList;
    int isHappen;
} ONVIF_EVENT_DUMP_PARAM_T;

typedef struct
{
    int state;
    MQ_HANDLE_H mqHandle;
    ONVIF_MGR_CFG_T cfg;
    ONVIF_CORE_VERSION_T coreVersion;
    void *udsServerHandle;
    void *udpMultiHandle;
    int subscribeWorking;
    COMMON_DLIST_T subscribeList;
    COMMON_DLIST_T pullList;
    unsigned long long int basetime;
    ONVIF_GPS_EXT_INFO_T gpsinfo;
    ONVIF_CAPABILITY_SET_T ability;

    int rtspPort;
    struct timespec rtspCfgTime;
} ONVIF_MGR_CONTEXT_T;

typedef int (* ONVIF_ACTION_FUN_F)(char *buf, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

typedef struct
{
    int actionId;
    ONVIF_ACTION_FUN_F actionFunc;
} ONVIF_ACTION_FUN_T;

static int OnvifAction_GetDeviceInfo(char *buf, int len,
                                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetDns(char *buf, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetCapbility(char *buf, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetScope(char *buf, int len,
                                ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_SetScope(char *httpStr, int len,
								ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetDateTime(char *buf, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetNetworkIf(char *buf, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetProfiles(char *buf, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetService(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetVideoSource(char *buf, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetProfile(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetStreamUri(char *buf, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetVideoSourceCfg(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetOsds(char *buf, int len,
                               ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_SetOsds(char *buf, int len,
                               ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetImageSet(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetOptions(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_SetImagimgSetting(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetVideoEncoderConfigurationOptions(char *httpStr,
        int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_SetVideoEncoderConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_SetNetworkInterfaces(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_SetNetworkDefaultGateway(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetOSDOptions(char *httpStr, int len,
                                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);


static int OnvifAction_GetMoveOptions(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_SetSystemDateAndTime(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetVideoAnalyticsConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);
static int OnvifAction_GetAnalyticsModules(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_ModifyAnalyticsModules(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetVideoEncoderConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetAudioEncoderConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetAudioEncoderConfigurationOptions(char *httpStr,
        int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetRules(char *httpStr, int len,
                                ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_ModifyRules(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_Subscribe(char *httpStr, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_SetVideoAnalyticsConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_Renew(char *httpStr, int len,
                             ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_Unsubscribe(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetServiceCapabilities(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetEventPropertyies(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_CreateOSD(char *httpStr, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_DeleteOSD(char *httpStr, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_CreatePullPointSubscription(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_PullMessages(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);
static int OnvifAction_GetAudioSources(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);
static int OnvifAction_GetAudioSourceConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);
static int OnvifAction_GetAudioSourceConfigurationOptions(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_GetMetadataConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_GetSnapshotUri(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                      char *contentStr);

static int OnvifAction_GetOSD(char *httpStr, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                              char *contentStr);
#if (defined ONVIF_EXT_GPS)
static int OnvifAction_PushAnalogInfo(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                      char *contentStr);

static int OnvifAction_PushStationInfo(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);
#endif


static int OnvifAction_GetNetworkProtocols(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_GetVideoEncoderConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetAudioEncoderConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr);

static int OnvifAction_GetConfigurations(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetConfiguration(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetConfigurationOptions(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetPresetTours(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetPresetTour(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetNode(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetNodes(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr);

static int OnvifAction_GetPresets(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_SetPreset(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_GotoPreset(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_DelPreset(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_GetStatus(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_AbsoluteMove(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_ContinuousMove(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_Move(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);


static int OnvifAction_Stop(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_HK_MaskOptions(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_HK_PrivacyMask(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr);

static int OnvifAction_SetSynchronizationPoint(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_GetVideoSourceConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_GetNTP(char *httpStr, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                              char *contentStr);
static int OnvifAction_GetDiscoveryMode(char *httpStr, int len,
                                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                        char *contentStr);

static int OnvifAction_GetNetworkDefaultGateway(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

static int OnvifAction_GetHostname(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                   char *contentStr);

static int OnvifAction_SystemReboot(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);

static int OnvifAction_GetSupportedAnalyticsModules(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);

static int OnvifAction_GetSupportedRules(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);

static int OnvifAction_AddVideoEncoderConfiguration(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);

static int OnvifAction_GetGuaranteedNumberOfVideoEncoderInstances(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);

static int OnvifAction_GetVideoEncoderInstances(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr);
static int OnvifAction_GetAudioOutputConfigurationOptions(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);
static int OnvifAction_GetAudioOutputConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);
static int OnvifAction_GetAudioDecoderConfigurationOptions(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);
static int OnvifAction_GetAudioDecoderConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr);

ONVIF_ACTION_FUN_T s_onvif_fun_map[] =
{
    {ONVIF_XML_ACTION_GET_DEVICE_INFO_ID, OnvifAction_GetDeviceInfo},
    {ONVIF_XML_ACTION_GET_DNS_ID, OnvifAction_GetDns},
    {ONVIF_XML_ACTION_GET_CAPBILITY_ID, OnvifAction_GetCapbility},
    {ONVIF_XML_ACTION_GET_SCOPE_ID, OnvifAction_GetScope},
    {ONVIF_XML_ACTION_SET_SCOPE_ID, OnvifAction_SetScope},
    {ONVIF_XML_ACTION_GET_DATE_TIME_ID, OnvifAction_GetDateTime},
    {ONVIF_XML_ACTION_GET_NETWORK_IF_ID, OnvifAction_GetNetworkIf},
    {ONVIF_XML_ACTION_GET_PROFILES_ID, OnvifAction_GetProfiles},
    {ONVIF_XML_ACTION_GET_SERVICE_ID, OnvifAction_GetService},
    {ONVIF_XML_ACTION_GET_VIDEO_SOURCE_ID, OnvifAction_GetVideoSource},
    {ONVIF_XML_ACTION_GET_PROFILE_ID, OnvifAction_GetProfile},
    {ONVIF_XML_ACTION_GET_STREAM_URI_ID, OnvifAction_GetStreamUri},
    {ONVIF_XML_ACTION_GET_VIDEO_SOURCE_CFG_ID, OnvifAction_GetVideoSourceCfg},
    {ONVIF_XML_ACTION_GET_OSDS_ID, OnvifAction_GetOsds},
    {ONVIF_XML_ACTION_SET_OSD_ID, OnvifAction_SetOsds},
    {ONVIF_XML_ACTION_GET_IMAGE_SET_ID, OnvifAction_GetImageSet},
    {ONVIF_XML_ACTION_GET_OPTIONS_ID, OnvifAction_GetOptions},
    {ONVIF_XML_ACTION_SET_IMAGING_SETTING_ID, OnvifAction_SetImagimgSetting},
    {ONVIF_XML_ACTION_GetVideoEncoderConfigurationOptions_ID, OnvifAction_GetVideoEncoderConfigurationOptions},
    {ONVIF_XML_ACTION_SetVideoEncoderConfiguration_ID, OnvifAction_SetVideoEncoderConfiguration},
    {ONVIF_XML_ACTION_SetNetworkInterfaces_ID, OnvifAction_SetNetworkInterfaces},
    {ONVIF_XML_ACTION_SetNetworkDefaultGateway_ID, OnvifAction_SetNetworkDefaultGateway},
    {ONVIF_XML_ACTION_GetOSDOptions_ID, OnvifAction_GetOSDOptions},
    {ONVIF_XML_ACTION_GetMoveOptions_ID, OnvifAction_GetMoveOptions},
    {ONVIF_XML_ACTION_SetSystemDateAndTime_ID, OnvifAction_SetSystemDateAndTime},
    {ONVIF_XML_ACTION_GetVideoAnalyticsConfigurations_ID, OnvifAction_GetVideoAnalyticsConfigurations},
    {ONVIF_XML_ACTION_GetAnalyticsModules_ID, OnvifAction_GetAnalyticsModules},
    {ONVIF_XML_ACTION_ModifyAnalyticsModules_ID, OnvifAction_ModifyAnalyticsModules},

    {ONVIF_XML_ACTION_GetVideoEncoderConfiguration_ID, OnvifAction_GetVideoEncoderConfiguration},
    {ONVIF_XML_ACTION_GetAudioEncoderConfigurationOptions_ID, OnvifAction_GetAudioEncoderConfigurationOptions},
    {ONVIF_XML_ACTION_GetAudioEncoderConfiguration_ID, OnvifAction_GetAudioEncoderConfiguration},
    {ONVIF_XML_ACTION_GetRules_ID, OnvifAction_GetRules},
    {ONVIF_XML_ACTION_ModifyRules_ID, OnvifAction_ModifyRules},
    {ONVIF_XML_ACTION_SetVideoAnalyticsConfiguration_ID, OnvifAction_SetVideoAnalyticsConfiguration},
    {ONVIF_XML_ACTION_Subscribe_ID, OnvifAction_Subscribe},
    {ONVIF_XML_ACTION_Renew_ID, OnvifAction_Renew},
    {ONVIF_XML_ACTION_Unsubscribe_ID, OnvifAction_Unsubscribe},
    {ONVIF_XML_ACTION_GetServiceCapabilities_ID, OnvifAction_GetServiceCapabilities},
    {ONVIF_XML_ACTION_CreateOSD_ID, OnvifAction_CreateOSD},
    {ONVIF_XML_ACTION_DeleteOSD_ID, OnvifAction_DeleteOSD},
    {ONVIF_XML_ACTION_GetEventProperties_ID, OnvifAction_GetEventPropertyies},
    {ONVIF_XML_ACTION_CreatePullPointSubscription_ID, OnvifAction_CreatePullPointSubscription},
    {ONVIF_XML_ACTION_PullMessages_ID, OnvifAction_PullMessages},
    {ONVIF_XML_ACTION_GetAudioSources_ID, OnvifAction_GetAudioSources},
    {ONVIF_XML_ACTION_GetAudioSourceConfigurations_ID, OnvifAction_GetAudioSourceConfigurations},
    {ONVIF_XML_ACTION_GetSnapshotUri_ID, OnvifAction_GetSnapshotUri},
    {ONVIF_XML_ACTION_GetOSD_ID, OnvifAction_GetOSD},

#if (defined ONVIF_EXT_GPS)
    {ONVIF_XML_ACTION_PushAnalogGpsInfo_ID, OnvifAction_PushAnalogInfo},
    {ONVIF_XML_ACTION_PushStationInfo_ID, OnvifAction_PushStationInfo},
#endif
    {ONVIF_XML_ACTION_GetNetworkProtocols_ID, OnvifAction_GetNetworkProtocols},
    {ONVIF_XML_ACTION_GetVideoEncoderConfigurations_ID, OnvifAction_GetVideoEncoderConfigurations},
    {ONVIF_XML_ACTION_GetVideoEncoderConfigurations_ID, OnvifAction_GetAudioEncoderConfigurations},

    {ONVIF_XML_ACTION_GetConfigurations_ID, OnvifAction_GetConfigurations},
    {ONVIF_XML_ACTION_GetConfiguration_ID, OnvifAction_GetConfiguration},
    {ONVIF_XML_ACTION_GetConfigurationOptions_ID, OnvifAction_GetConfigurationOptions},
    {ONVIF_XML_ACTION_GetPresetTours_ID, OnvifAction_GetPresetTours},
    {ONVIF_XML_ACTION_GetPresetTour_ID, OnvifAction_GetPresetTour},
    {ONVIF_XML_ACTION_GetNode_ID, OnvifAction_GetNode},
    {ONVIF_XML_ACTION_GetNodes_ID, OnvifAction_GetNodes},
    {ONVIF_XML_ACTION_GetPresets_ID, OnvifAction_GetPresets},
    {ONVIF_XML_ACTION_SetPreset_ID, OnvifAction_SetPreset},
    {ONVIF_XML_ACTION_GotoPreset_ID, OnvifAction_GotoPreset},
    {ONVIF_XML_ACTION_DelPreset_ID, OnvifAction_DelPreset},
    {ONVIF_XML_ACTION_GetStatus_ID, OnvifAction_GetStatus},
    {ONVIF_XML_ACTION_AbsoluteMove_ID, OnvifAction_AbsoluteMove},
    {ONVIF_XML_ACTION_ContinuousMove_ID, OnvifAction_ContinuousMove},
    {ONVIF_XML_ACTION_Move_ID, OnvifAction_Move},
    {ONVIF_XML_ACTION_Stop_ID, OnvifAction_Stop},
    {ONVIF_XML_ACTION_HK_MaskOptions_ID, OnvifAction_HK_MaskOptions},
    {ONVIF_XML_ACTION_HK_PrivacyMask_ID, OnvifAction_HK_PrivacyMask},
    {ONVIF_XML_ACTION_SetSynchronizationPoint, OnvifAction_SetSynchronizationPoint},
    {ONVIF_XML_ACTION_GetVideoSourceConfigurations, OnvifAction_GetVideoSourceConfigurations},
    {ONVIF_XML_ACTION_GetNTP, OnvifAction_GetNTP},
    {ONVIF_XML_ACTION_GetDiscoveryMode, OnvifAction_GetDiscoveryMode},
    {ONVIF_XML_ACTION_GetNetworkDefaultGateway, OnvifAction_GetNetworkDefaultGateway},
    {ONVIF_XML_ACTION_GetHostname, OnvifAction_GetHostname},
    {ONVIF_XML_ACTION_SystemReboot, OnvifAction_SystemReboot},
    {ONVIF_XML_ACTION_GetSupportedAnalyticsModules, OnvifAction_GetSupportedAnalyticsModules},
    {ONVIF_XML_ACTION_GetSupportedRules, OnvifAction_GetSupportedRules},
    {ONVIF_XML_ACTION_AddVideoEncoderConfiguration, OnvifAction_AddVideoEncoderConfiguration},
    {ONVIF_XML_ACTION_GetGuaranteedNumberOfVideoEncoderInstances, OnvifAction_GetGuaranteedNumberOfVideoEncoderInstances},
    {ONVIF_XML_ACTION_GetVideoEncoderInstances, OnvifAction_GetVideoEncoderInstances},
    {ONVIF_XML_ACTION_GetAudioSourceConfigurationOptions, OnvifAction_GetAudioSourceConfigurationOptions},
    {ONVIF_XML_ACTION_GetMetadataConfigurations, OnvifAction_GetMetadataConfigurations},
    {ONVIF_XML_ACTION_GetAudioOutputConfigurationOptions, OnvifAction_GetAudioOutputConfigurationOptions},
    {ONVIF_XML_ACTION_GetAudioOutputConfigurations, OnvifAction_GetAudioOutputConfigurations},
    {ONVIF_XML_ACTION_GetAudioDecoderConfigurationOptions, OnvifAction_GetAudioDecoderConfigurationOptions},
    {ONVIF_XML_ACTION_GetAudioDecoderConfigurations, OnvifAction_GetAudioDecoderConfigurations},

};

ONVIF_MGR_CONTEXT_T s_onvif_mgr_ct;

static void OnvifEventNodeFree(void *data)
{
    if (data != NULL)
        ONVIF_FREE(data);
}

static int OnvifEventDump(void *a, void *b)
{
    if (a != NULL && ((ONVIF_SUBSCRIBE_NODE_T *)a)->isPullPoint == 0)
    {
        ONVIF_SUBSCRIBE_NODE_T *node  =
            (ONVIF_SUBSCRIBE_NODE_T *)ONVIF_MALLOC(sizeof(ONVIF_SUBSCRIBE_NODE_T));
        memcpy(node, a, sizeof(ONVIF_SUBSCRIBE_NODE_T));
        ONVIF_EVENT_DUMP_PARAM_T *param = (ONVIF_EVENT_DUMP_PARAM_T *)b;
        Common_DList_InsertTail(param->sendList, node, sizeof(ONVIF_SUBSCRIBE_NODE_T));

        ((ONVIF_SUBSCRIBE_NODE_T *)a)->updateTime = Common_GetSystemCount64();
        if (param->isHappen == 1)
            ((ONVIF_SUBSCRIBE_NODE_T *)a)->isSend = 1;
        else
            ((ONVIF_SUBSCRIBE_NODE_T *)a)->isSend = 0;
    }
    return -1;
}

static int OnvifEventSearchByToken(void *a, void *b)
{
    if (a != NULL && b != NULL)
    {
        ONVIF_SUBSCRIBE_NODE_T *node = (ONVIF_SUBSCRIBE_NODE_T *)a;
        unsigned int token = *(unsigned int *)b;
        if (node->token == token)
        {
            node->updateTime = Common_GetSystemCount64();
            return 0;
        }
    }
    return -1;
}

static int OnvifEventSearchByContent(void *a, void *b)
{
    if (a != NULL && b != NULL)
    {
        ONVIF_SUBSCRIBE_NODE_T *node = (ONVIF_SUBSCRIBE_NODE_T *)a;
        char tokenStr[32] = {};
        snprintf(tokenStr, sizeof(tokenStr), "_%u", node->token);
        char * contentStr = (char *)b;
        if (strstr(contentStr, tokenStr) != NULL)
        {
            node->updateTime = Common_GetSystemCount64();
            return 0;
        }
    }
    return -1;
}

static int OnvifEventCheckTime(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;

    unsigned long long int curTime = *(unsigned long long int *)b;
    ONVIF_SUBSCRIBE_NODE_T *node = (ONVIF_SUBSCRIBE_NODE_T *)a;

    if (curTime - node->updateTime > ONVIF_EVENT_UPDATE_TIME_DELAY)
        return 0;
    return -1;
}

static int OnvifEventDetatch(void *a, void *b)
{
    if (a == NULL || ((ONVIF_SUBSCRIBE_NODE_T *)a)->isPullPoint == 1)
        return -1;

    return 0;
}

static int CheckIpInMap(char *ipaddr)
{
    FILE *fp = fopen("/proc/net/arp", "rb");
    if (fp == NULL)
        return -1;

    char *mapData = (char *)ONVIF_MALLOC(4096);
    fread(mapData, 1, 4095, fp);
    fclose(fp);

    char tmpAddr[32] = { 0 };
    snprintf(tmpAddr, sizeof(tmpAddr), "%s ", ipaddr);
    if (strstr(mapData, tmpAddr))
    {
        LOGD("get same ip in tab\n");
        ONVIF_FREE(mapData);
        return 1;
    }
    ONVIF_FREE(mapData);
    return 0;
}

struct arpMsg
{
    struct ethhdr ethhdr; /* Ethernet header */
    u_short htype; /* hardware type (must be ARPHRD_ETHER) */
    u_short ptype; /* protocol type (must be ETH_P_IP) */
    u_char hlen; /* hardware address length (must be 6) */
    u_char plen; /* protocol address length (must be 4) */
    u_short operation; /* ARP opcode */
    u_char sHaddr[6]; /* sender's hardware address */
    u_char sInaddr[4]; /* sender's IP address */
    u_char tHaddr[6]; /* target's hardware address */
    u_char tInaddr[4]; /* target's IP address */
    u_char pad[18]; /* pad for min. Ethernet payload (60 bytes) */
};

static unsigned short calc_cksum(char *buff, int len)
{
    int blen = len;
    unsigned short *mid = (unsigned short *)buff;
    unsigned short te = 0;
    unsigned int sum = 0;

    while(blen > 1)
    {
        sum += *mid++;
        blen -= 2;
    }
    //数据长度为奇数比如65 上面的while是按16计算的 最后就会剩下一字节不能计算
    if(blen == 1)
    {
        //将多出的一字节放入short类型的高位 低8位置0 加入到sum中
        te = *(unsigned char *)mid;
        te  = (te << 8) & 0xff;
        sum += te;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return (unsigned short)(~sum);
}


static void icmp_packet(char *buff, int len, int id, int seq)
{
    struct timeval *tval = NULL;
    struct icmp *icmp = (struct icmp *)buff;

    icmp->icmp_type = 8; //ECHO REQUEST
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;  //first set zero
    icmp->icmp_id = id & 0xffff;
    icmp->icmp_seq = seq;

    tval = (struct timeval *)icmp->icmp_data;
    gettimeofday(tval, NULL); //获得传输时间作为数据

    //计算校验和
    icmp->icmp_cksum = calc_cksum(buff, len);
    return;
}

static int parse_packet(char *buff, int len, char *curIp)
{
    struct timeval *val = NULL;
    struct timeval nv = {};
    struct icmp *icmp = NULL;
    struct iphdr *iphead = (struct iphdr *)buff;
    struct in_addr addr= {};
    addr.s_addr = iphead->saddr;
    char *recvIp  = inet_ntoa(addr);
    LOGD("comefrom ip=%s  \n", recvIp);

    if (strcmp(recvIp, curIp) == 0)
        return -1;
    //跳过ip头
    icmp = (struct icmp *)(buff + sizeof(struct iphdr));

    //看传输回的包校验和是否正确
    if(calc_cksum((char *)icmp, len - sizeof(struct iphdr)) > 1)
    {
        return -1;
    }
    gettimeofday(&nv, NULL);
    val = (struct timeval *)icmp->icmp_data;

    LOGD("type=%d  seq=%d id=%d pid=%d usec=%ld \n",
         icmp->icmp_type, icmp->icmp_seq, icmp->icmp_id, (getpid() & 0xffff),
         nv.tv_usec - val->tv_usec);

    return 0;

}


static int ping_address(char *srcDev, char *curIp, uint32_t dstIp, int timeout, int cnt)
{
    int skfd;
    struct sockaddr_in addr = {0};
    struct sockaddr_in saddr = {0};
    char buff[64] = {0};
    char recvbuff[512] = {0};
    int ret = 0, isOk = -1;
    int addrlen = 0;
    int count = cnt;
    int i = 1;

    skfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(skfd < 0)
    {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dstIp;//inet_addr(dstIp);

    /* struct ifreq ifr = {}; */
    /* strcpy(ifr.ifr_name, srcDev); */
    /* setsockopt(skfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)); */

    //每一秒发送一次 共发送count次
    while(count > 0 && isOk != 0)
    {
        LOGD("ping count %d\n", count);
        //序列号seq 从1 开始传输  buff的大小为64
        memset(buff, 0, sizeof(buff));
        icmp_packet(buff, 64, getpid(), i);
        i++;
        count --;

        //将数据发送出去
        ret = sendto(skfd, buff, 64, 0, (struct sockaddr *)&addr, sizeof(addr));
        if(ret <= 0)
        {
            LOGE("ping count %d\n", count);
            break;
        }

        //接收echo replay
        memset(recvbuff, 0, sizeof(recvbuff));
        memset(&saddr, 0, sizeof(saddr));
        addrlen = sizeof(saddr);

        fd_set descriptors;
        struct timeval time_to_wait;

        FD_ZERO(&descriptors);
        FD_SET(skfd, &descriptors);
        time_to_wait.tv_sec = timeout / 1000;
        time_to_wait.tv_usec = timeout % 1000 * 1000;
        int return_value = select(skfd + 1, &descriptors, NULL, NULL, &time_to_wait);
        if (return_value < 0)
        {
            continue;
        }
        else if (!return_value)
        {
            continue;
        }

        ret = recvfrom(skfd, recvbuff, sizeof(recvbuff), 0, (struct sockaddr *)&saddr,
                       (socklen_t *)&addrlen);
        if(ret <= 0)
        {
            continue;
        }
        if (parse_packet(recvbuff, ret, curIp) == 0)
            isOk = 0;
    }

    close(skfd);
    return isOk;
}


static int LoadRtspPortFromCfg(int *rtspPort)
{
    struct stat sb = {};

    if (stat(RTSP_CFG_PATH, &sb) == -1)
        return -1;

    /*获取配置文件的修改时间，如果修改时间相同的话，直接返回上次获得的端口号*/
    if (s_onvif_mgr_ct.rtspPort != 0 &&
            memcmp(&sb.st_mtime, &s_onvif_mgr_ct.rtspCfgTime, sizeof(struct timespec)) == 0)
    {
        *rtspPort = s_onvif_mgr_ct.rtspPort;
        return 0;
    }

    LOGD("load rtsp port from cfgfiles \n");
    FILE *fp = fopen(RTSP_CFG_PATH, "rb");
    if (fp == NULL)
        return -1;

    char lineBuf[256] = {};
    char *rp  = NULL;
    char rtspPortStr[8] = {};

    do
    {
        rp = fgets(lineBuf, sizeof(lineBuf), fp);
        if (rp == NULL)
        {
            break;
        }

        if (strstr(lineBuf, "RtspPort") != NULL)
        {
            sscanf(lineBuf, "%*[^1-9]%7[0-9]", rtspPortStr);
            break;
        }
    }
    while(rp != NULL);

    fclose(fp);

    if (strlen(rtspPortStr) > 0)
    {
        *rtspPort = atoi(rtspPortStr);
        memcpy(&s_onvif_mgr_ct.rtspCfgTime, &sb.st_mtime, sizeof(struct timespec));
        s_onvif_mgr_ct.rtspPort = *rtspPort;
    }
    else
        return -1;

    return 0;
}

static void GetGatewayFromIpMask(char *ip, char *mask, char *gateway)
{
    //LOGD("ip:[%s] mask:[%s]\n",ip,mask);
    int i = 0;
    int ip_tmp[4];
    int mask_tmp[4];
    int gatway_tmp[3];
    sscanf(ip,"%d.%d.%d.%d",&ip_tmp[0],&ip_tmp[1],&ip_tmp[2],&ip_tmp[3]);
    sscanf(mask,"%d.%d.%d.%d",&mask_tmp[0],&mask_tmp[1],&mask_tmp[2],&mask_tmp[3]);

    for(i=0; i<3; i++)
    {
        gatway_tmp[i] = mask_tmp[i]?ip_tmp[i]:0;
    }

    snprintf(gateway, 15, "%d.%d.%d.1", gatway_tmp[0], gatway_tmp[1], gatway_tmp[2]);
}

void *EventNotifyThread()
{
    while(1)
    {
        OnvifMgr_EventNotify();
        Common_Sleep(0,500 * 1000);
    }
}

void *EventCheckThread()
{
    while(1)
    {
        OnvifMgr_EventCheck();
        Common_Sleep(10,0);
    }
}


int fixedIp = 0;

void *checkRunTimeThread()
{
    int ret = 0;
    while(1)
    {
        unsigned long long int time = Common_GetSystemCount64() - s_onvif_mgr_ct.basetime;
        unsigned long long int timeout = s_onvif_mgr_ct.cfg.timeout * 1000LLU;
        //LOGD("\n-----------------time:[%lld] timeout:[%d]\n",time,s_onvif_mgr_ct.cfg.timeout);
        if (s_onvif_mgr_ct.cfg.fixedIp == 0 && time >= timeout)
        {
            LOGW("run time has been exceeded ADAPTIVEIP_MAXTIME!\n");
            fixedIp = 1;
            s_onvif_mgr_ct.cfg.fixedIp = 1;
            if (Mq_Request(s_onvif_mgr_ct.mqHandle, ONVIF_REQ_SET_CFG_STRUCT, &s_onvif_mgr_ct.cfg, sizeof(s_onvif_mgr_ct.cfg),
                               &ret, NULL, 0) < 0
                        || ret < 0)
            {
                LOGE("thread fixedIp failed\n");
                //return;
            }
            //return;
        }
        /*LOGD("\n-----------------SLEEP time:[%lld]\n",(timeout - time)*1000);
        int sleep_sec = (timeout - time)/1000;
        int sleep_usec = (timeout - time)%1000*1000;
        LOGD("sleep[%d][%d]\n",sleep_sec,sleep_usec);
        Common_Sleep(sleep_sec, sleep_usec);*/
        Common_Sleep(1,0);
    }

}

static void AdaptiveIpCheck(char *remoteIp, ONVIF_MGR_CONTEXT_T *ct)
{
    int ret = 0, r = 0; //conflict = 0;

    if (remoteIp[0] == '0' && remoteIp[1] == '.')
    {
        LOGW("\n---------remoteIp is NULL!\n");
        return;
    }

    if (RestOnvif_IsDefaultPassword() == 0)
    {
        LOGW("\n---------is not default password\n");
        return;
    }

    char curIpPrefix[32] = { 0 };
    char remoteIpPrefix[32] = { 0 };
    char curMac[32] = { 0 };
    char curIp[32] = { 0 };
    Utils_GetNetDevInfo((char *)"eth0", curMac, curIp, NULL);
/*
    if(strcmp(curIp, "192.168.1.188") != 0)
    {
        LOGW("is not default ip\n");
        return;
    }
*/

    char *p1 = strrchr(curIp, '.');
    if (p1 != NULL)
        snprintf(curIpPrefix, p1 - curIp + 1, "%s", curIp);

    p1 = strrchr(remoteIp, '.');
    if (p1 != NULL)
        snprintf(remoteIpPrefix, p1 - remoteIp + 1, "%s", remoteIp);

    if (strlen(curIpPrefix) == 0 || strlen(remoteIpPrefix) == 0
        /*|| strcmp(curIpPrefix, remoteIpPrefix) == 0*/)// bug fix: IP同网段, 也需改IP
        return;

    if((strcmp(curIp, ct->cfg.fixedIpAddr) == 0) &&
       (strcmp(curIpPrefix, remoteIpPrefix) == 0))//IP同网段,自适应的IP,不改IP
    {
        LOGW("Same network segment! adaptiveIp! NO MODIFY!\n");
        return;
    }

    //u_int32_t addr = inet_addr(curIp);

/*
    if (Arpping(addr, addr, (unsigned char *)curMac, (char *)"eth0") == 1)
    {
        LOGD("\n------------------ip conflict\n");
        conflict = 1;
    }
*/
    LOGD("\n------------------current ip prefix is %s\n", curIpPrefix);
 //   if (conflict == 1 || strcmp(curIpPrefix, remoteIpPrefix) != 0)
    {
        unsigned short rdip = 0;
        char changeIp[32] = { 0 };
        char *p = strrchr(remoteIp, '.');
        if (p == NULL)
        {
            LOGW("\n---------strrchr remoteIp is NULL!\n");
            return;
        }
        snprintf(changeIp, p - remoteIp + 1, "%s", remoteIp);

        do
        {
            //Common_Sleep(0, (Common_Rand16()%1000));

            char tmpIp[32] = { 0 };
            rdip = (Common_Rand16() % 253) + 2;

            if (rdip < 2 || rdip > 254)
            {
                LOGE("\n----------------- rdip:[%d]!\n",rdip);
                continue;
            }

            snprintf(tmpIp, sizeof(tmpIp), "%s.%d", changeIp, rdip);
            if(strcmp(tmpIp, "192.168.1.188") == 0) // 规避自适应到默认IP
            {
                continue;
            }

            if (CheckIpInMap(tmpIp) == 1)
            {
                LOGE("\n-----------------CheckIpInMap = 1!\n");
                continue;
            }
            char mac[32] = { 0 };
            char lip[32] = { 0 };

            Utils_GetNetDevInfo((char *)"eth0", mac, lip, NULL);

            LOGD("lip:[%s] mac:[%s]\n",lip,mac);

            unsigned long long int rtime = Common_Rand32() % 2000;

            LOGD("\n------------------try to checkip %s rtime is %llu\n", tmpIp,rtime);
            ret = Utils_ArpingCheckIp(tmpIp, lip, mac, (char *)"eth0", rtime);

            if (ret == 1)
            {
                LOGE("\n-----------------1]%s ip was used\n", tmpIp);
                r++;
                continue;
            }

            Utils_SetNetDevInfo((char *)"eth0", NULL, tmpIp, NULL);


            Common_Sleep(1, 0);

            rtime = Common_Rand32() % 2000;

            ret = Utils_ArpingCheckIp(tmpIp, tmpIp, mac, (char *)"eth0", rtime);

            if (ret == 1)
            {
                LOGE("\n-----------------2]%s ip was used\n", tmpIp);
                r++;
                continue;
            }

            LOGD("\n------------------check ok %s\n", tmpIp);
            ONVIF_NET_INFO_T ninfo;

            RestOnvif_RequesNetInfo(&ninfo);

            ninfo.bDhcp = 0;
            snprintf(ninfo.ipV4, sizeof(ninfo.ipV4), "%s", tmpIp);
            snprintf(ct->cfg.fixedIpAddr, sizeof(ct->cfg.fixedIpAddr), "%s", tmpIp);

            //note: 强制子网掩码为255.255.255.0, 防止连接端问题
            GetGatewayFromIpMask(ninfo.ipV4, "255.255.255.0"/*ninfo.ipMaskV4*/, ninfo.gateWayV4);

            //LOGD("ip:[%s] gateway:[%s]\n",ninfo.ipV4,ninfo.gateWayV4);
            RestOnvif_SetNetInfo(&ninfo);

            fixedIp = 1;

            if (Mq_Request(ct->mqHandle, ONVIF_REQ_SET_CFG_STRUCT, &ct->cfg, sizeof(ct->cfg),
    						   &ret, NULL, 0) < 0
    					|| ret < 0)
    		{
    			LOGE("save fixedIpAddr failed\n");
    			return;
    		}

            break;
        }
        while(r < 10);

    }

}

//static int UdpMultiServerCallBack(void *handle, int conHandle, char *devName, char *buf,
//                                  int len, char *remoteIp, void *userData)
unsigned long long int timeA = 0;

int UdpMultiServerCallBack(char *devName, char *remoteIp, char *buf,int len, char **outBuf,int *outSize)
{
    char maca[32] = { 0};
    char ip[32] = { 0 };
    int ret = 0;
    //ONVIF_MGR_CONTEXT_T *ct = (ONVIF_MGR_CONTEXT_T *)userData;
    if (s_onvif_mgr_ct.state != ONVIF_MGR_STATE_START)
    {
        LOGW("state error\n");
        return -1;
    }

    if (strstr(buf, "ZXDLLCXXJTX") != NULL)
    {
        //LOGW("Is Ants Nvr!!\n");
        fixedIp = 1;
    }

    if (strstr(buf, "discovery/Probe") == NULL)
        return -1;

    if (strstr(buf, "Probe") == NULL)
        return -1;

    if (strstr(buf, "ProbeMatch") != NULL)
        return -1;

    unsigned long long int timeB = Common_GetSystemCount64();
    if (timeB - timeA < 1000LLU)
    {
        return 1;
    }

    timeA = timeB;

    LOGE("recv discover from %s dev %s\n", remoteIp, devName);
    //LOGD("cfg auth mode %d ipadaptive %d\n", ct->cfg.authEnable, ct->cfg.adaptiveIp);

    /*TODO ip auto config , only support eth0 */
    /*authEnable == 2 should enable adaptiveIp and disable password check ,
      when ipc using default password*/
      //LOGD("fixedip:[%d]t[%d]\n",ct->cfg.fixedIp,fixedIp);
    if ((s_onvif_mgr_ct.cfg.fixedIp == 0 && fixedIp == 0) &&
        (s_onvif_mgr_ct.cfg.adaptiveIp == 1 || s_onvif_mgr_ct.cfg.authEnable == 2) &&
        strcmp(devName, "eth0") == 0)
    {
        /* OnvifMgr_GetNetInfo((char *)"eth0", maca, ip, NULL); */
        /* ret = XmlParser_UdpDiscovery(buf, len, maca, ip); */
        /* if (ret == 0) */

        /*从rtsp配置文件中获取端口号，并查询/proc/net/tcp中的连接端口数量，如果有连接则不修改IP*/
        int rtsp = 0, connNum = 0;
        LoadRtspPortFromCfg(&rtsp);
        LOGD("load rtsp port is %d\n", rtsp);
        if (rtsp > 0)
            Utils_CheckTcpPortIsConnected(rtsp, &connNum);
        LOGD("rtsp connectedNum is %d\n", connNum);
        if (connNum == 0)
            AdaptiveIpCheck(remoteIp, &s_onvif_mgr_ct);
    }

    OnvifMgr_GetNetInfo(devName, maca, ip, NULL);

    ONVIF_WEB_ATTR_T webAttr = {};
    RestOnvif_RequestWebServer(&webAttr);
    LOGD("\n------------------IP:[%s] maca:[%s] httpport:[%d]\n",ip,maca,webAttr.httpPort);
    ret = XmlParser_UdpDiscovery(buf, len, maca, ip, webAttr.httpPort,&s_onvif_mgr_ct.ability);
    if(ret == 0)
    {
        *outBuf = calloc(1,strlen(buf)+1);
        Common_Strncpy(*outBuf, buf, strlen(buf));
        *outSize = strlen(buf);
    }
    return ret;
}

static int UdsRspHandle401(void *udsHandle, int conHandle)
{
    LOGD("send 401\n");
    char *httpStr = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpStr, 0, ONVIF_MSG_BUF_LEN);
    HttpRsp_Unauth401(httpStr, ONVIF_MSG_BUF_LEN);
    DBGS("send @%s@\n", httpStr);
    Utils_UdsServerSend(udsHandle, conHandle, httpStr, strlen(httpStr));
    ONVIF_FREE(httpStr);
    return 0;

}

static int UdsRspHandle400(void *udsHandle, int conHandle)
{
    char *httpStr = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    HttpRsp_BadReq400(httpStr, ONVIF_MSG_BUF_LEN);
    Utils_UdsServerSend(udsHandle, conHandle, httpStr, strlen(httpStr));
    DBGS("send @%s@\n", httpStr);
    ONVIF_FREE(httpStr);

    return 0;
}

static int OnvifAction_GetDeviceInfo(char *httpStr, int len,
                                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    if(s_onvif_mgr_ct.coreVersion.SoftwareModel[0])
	{
		LOGD("[OnvifAction_GetDeviceInfo]:show SoftwareModel \n");
	    HttpRsp_DeviceInfo(httpStr, len,
					   s_onvif_mgr_ct.coreVersion.Manufacturer,
					   s_onvif_mgr_ct.coreVersion.DeviceModel,
                       s_onvif_mgr_ct.coreVersion.HardVersion,
                       s_onvif_mgr_ct.coreVersion.SerialNumber,
                       s_onvif_mgr_ct.coreVersion.SoftwareModel,xmlDetail);
    }
    else
    {
    	LOGD("[OnvifAction_GetDeviceInfo]:show Hardware \n");
    	HttpRsp_DeviceInfo(httpStr, len,
					   s_onvif_mgr_ct.coreVersion.Manufacturer,
					   s_onvif_mgr_ct.coreVersion.DeviceModel,
                       s_onvif_mgr_ct.coreVersion.HardVersion,
                       s_onvif_mgr_ct.coreVersion.SerialNumber,
                       s_onvif_mgr_ct.coreVersion.Hardware,xmlDetail);
   	}

    return 0;
}

static int OnvifAction_GetDns(char *httpStr, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_NETWORK_ATTR_T networkAttr;
    memset(&networkAttr, 0, sizeof(ONVIF_NETWORK_ATTR_T));
    RestOnvif_RequestNetworkAttr(&networkAttr);
    HttpRsp_Dns(httpStr, len, xmlDetail, &networkAttr);
    return 0;
}

static int OnvifAction_GetCapbility(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_WEB_ATTR_T webAttr;
    memset(&webAttr, 0, sizeof(ONVIF_WEB_ATTR_T));
    RestOnvif_RequestWebServer(&webAttr);
    HttpRsp_Capabilities(httpStr, len, xmlDetail, &webAttr,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_GetScope(char *httpStr, int len,
                                ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    RestOnvif_RequestCoreVersion(&s_onvif_mgr_ct.coreVersion);
    HttpRsp_Scope(httpStr, len, xmlDetail, s_onvif_mgr_ct.coreVersion.Country, s_onvif_mgr_ct.coreVersion.City, s_onvif_mgr_ct.coreVersion.DeviceName);
    return 0;
}

static int OnvifAction_SetScope(char *httpStr, int len,
								ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
	RestOnvif_RequestCoreVersion(&s_onvif_mgr_ct.coreVersion);
	HttpRsp_SetScope(httpStr, len, xmlDetail, s_onvif_mgr_ct.coreVersion.DeviceName);
	return 0;
}

static int OnvifAction_GetDateTime(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_TIME_ALL_T timeAll;
    memset(&timeAll, 0, sizeof(ONVIF_TIME_ALL_T));
    RestOnvif_RequestGetTimeAll(&timeAll);
    char tzName[32] = {0 };
    char tzDstName[32] = { 0 };
    int tzOffset = 0;
    //RestOnvif_GetCfgTimeZone(tzName, sizeof(tzName), &tzOffset, tzDstName,
    //                         sizeof(tzDstName));
    HttpRsp_DateTime(httpStr, len, &timeAll, xmlDetail, tzName, tzOffset);
    return 0;
}

static int OnvifAction_GetNetworkIf(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_NET_INFO_T pNetInfo = {0};
    //ONVIF_NETWORK_ATTR_T networkAttr;
    //memset(&networkAttr, 0, sizeof(ONVIF_NETWORK_ATTR_T));
    //RestOnvif_RequestNetworkAttr(&networkAttr);
    RestOnvif_RequesNetInfo(&pNetInfo);
    HttpRsp_NetworkIf(httpStr, len, &pNetInfo, xmlDetail);

    return 0;
}

static int OnvifAction_GetProfiles(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_PROFILES_T *profiles = (ONVIF_PROFILES_T *)ONVIF_MALLOC(sizeof(
                                     ONVIF_PROFILES_T));
    memset(profiles, 0, sizeof(ONVIF_PROFILES_T));

    RestOnvif_RequestViAttr(&profiles->viAttr);

    RestOnvif_GetVideoEncoderCfg(0, 0, 0, &profiles->stream[0]);
    RestOnvif_GetVideoEncoderCfg(0, 0, 1, &profiles->stream[1]);
    RestOnvif_GetVideoEncoderCfg(0, 0, 2, &profiles->stream[2]);
    RestOnvif_RequestMotionAttr(&profiles->motion);

    HttpRsp_Profiles(httpStr, len, xmlDetail, profiles, "s");

    ONVIF_FREE(profiles);

    return 0;
}

static int OnvifAction_GetProfile(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_PROFILES_T *profiles = (ONVIF_PROFILES_T *)ONVIF_MALLOC(sizeof(
                                     ONVIF_PROFILES_T));
    memset(profiles, 0, sizeof(ONVIF_PROFILES_T));

    RestOnvif_RequestViAttr(&profiles->viAttr);

    RestOnvif_GetVideoEncoderCfg(0, 0, 0, &profiles->stream[0]);
    RestOnvif_GetVideoEncoderCfg(0, 0, 1, &profiles->stream[1]);
    RestOnvif_GetVideoEncoderCfg(0, 0, 2, &profiles->stream[2]);

    RestOnvif_RequestMotionAttr(&profiles->motion);

    HttpRsp_Profiles(httpStr, len, xmlDetail, profiles, "");

    ONVIF_FREE(profiles);

    return 0;
}

static int OnvifAction_GetService(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_WEB_ATTR_T webAttr;
    memset(&webAttr, 0, sizeof(ONVIF_WEB_ATTR_T));
    RestOnvif_RequestWebServer(&webAttr);
    HttpRsp_Servcies(httpStr, len, xmlDetail, &webAttr);

    return 0;
}

static int OnvifAction_GetVideoSource(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_VI_ATTR_T viAttr;
    memset(&viAttr, 0, sizeof(ONVIF_VI_ATTR_T));
    RestOnvif_RequestViAttr(&viAttr);
    HttpRsp_VideoSource(httpStr, len, xmlDetail, &viAttr);
    return 0;
}


static int OnvifAction_GetStreamUri(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MultiCast_T rtspAttr;
    memset(&rtspAttr, 0, sizeof(ONVIF_MultiCast_T));
    RestOnvif_GetMultiCastCfg(0, 0, 0, &rtspAttr);
    HttpRsp_StreamUri(httpStr, len, xmlDetail, &rtspAttr);

    return 0;
}

static int OnvifAction_GetVideoSourceCfg(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    ONVIF_VI_ATTR_T viAttr;
    memset(&viAttr, 0, sizeof(ONVIF_VI_ATTR_T));
    RestOnvif_RequestViAttr(&viAttr);
    HttpRsp_VideoSourceCfg(httpStr, len, xmlDetail, &viAttr);

    return 0;
}

static int OnvifAction_GetOsds(char *httpStr, int len,
                               ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr = {};
    RestOnvif_RequestGetOsd(&osdAttr);
    RestOnvif_RequestGetMultiOsd(&osdAttr);
#if (defined ONVIF_EXT_CUSTOM_OSD)
    RestOnvif_RequestGetCustomOsd(&osdAttr);
#endif
    HttpRsp_GetOsds(httpStr, len, xmlDetail, &osdAttr);
    return 0;
}

static int OnvifAction_SetOsds(char *httpStr, int len,
                               ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr;
    memset(&osdAttr, 0, sizeof(ONVIF_OSD_ATTR_T));
    RestOnvif_RequestGetOsd(&osdAttr);
    osdAttr.channelOsdAttr.osdEnable = -1;
    osdAttr.datetimeOsdAttr.osdEnable = -1;
    osdAttr.multiOsdAttr.osdEnable = -1;
#if (defined ONVIF_EXT_CUSTOM_OSD)
    osdAttr.custOsdAttr[0].osdEnable = -1;
    osdAttr.custOsdAttr[1].osdEnable = -1;
    osdAttr.custOsdAttr[2].osdEnable = -1;
#endif
    XmlParser_SetOsdParam(xmlDetail, &osdAttr, contentStr);
    RestOnvif_RequestSetOsd(&osdAttr);
    if (osdAttr.multiOsdAttr.osdEnable == 1)
    {
        RestOnvif_RequestSetMultiOsd(&osdAttr);
    }

#if (defined ONVIF_EXT_CUSTOM_OSD)
    RestOnvif_RequestSetCustomOsd(&osdAttr);
#endif

    HttpRsp_SetOsds(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetImageSet(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_IMAGE_SET_T image;
    memset(&image, 0, sizeof(ONVIF_IMAGE_SET_T));
    RestOnvif_RequestImageAttr(&image);
    HttpRsp_ImageSet(httpStr, len, &image, xmlDetail);
    return 0;
}

static int OnvifAction_GetOptions(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    HttpRsp_Options(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_SetImagimgSetting(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    int channel = 0;
    ONVIF_IMAGE_SET_T image;
    memset(&image, 0, sizeof(ONVIF_IMAGE_SET_T));
    RestOnvif_RequestImageAttr(&image);
    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    char *pStr = NULL;

    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, "Brightness", NULL, NULL,
                                         MXML_DESCEND_ALL);
    if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        image.brightness = atof(pStr);

        token = mxmlFindElement(pRoot, pRoot, "ColorSaturation", NULL, NULL,
                                MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            image.saturation = atof(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "Contrast", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            LOGE("pStr = %s\n", pStr);
            image.contrast = atof(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "Sharpness", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            image.sharpness = atof(pStr);
        }
    }
    else if(token == NULL)
    {
        token = mxmlFindElement(pRoot, pRoot, "tt:Brightness", NULL, NULL,
                                MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            image.brightness = atof(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "tt:ColorSaturation", NULL, NULL,
                                MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            image.saturation = atof(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "tt:Contrast", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            LOGE("pStr = %s\n", pStr);
            image.contrast = atof(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "tt:Sharpness", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            image.sharpness = atof(pStr);
        }
    }

    LOGD("%d %d %d %d\n", image.brightness, image.saturation, image.contrast,
         image.sharpness);
    //RestOnvif_SetImageAttr(channel - 1, &image);
    HttpRsp_SetImaging(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetVideoEncoderConfigurationOptions(char *httpStr,
        int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    ONVIF_VideoEncoderCapability_T videoCfg;
    memset(&videoCfg, 0, sizeof(ONVIF_VideoEncoderCapability_T));
    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    int devNo = 1, channelNo = 1, streamNo = 1;
    char *pStr = NULL;
    char EleName[32] = {0};
    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, "ConfigurationToken", NULL,
                                         NULL, MXML_DESCEND_ALL);

    if(token == NULL)
    {
        snprintf(EleName,31,"%s:ConfigurationToken",xmlDetail->actionNs);
        token = mxmlFindElement(pRoot, pRoot, EleName, NULL,
                                NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            /*
            if (strstr(token->value.element.name, "trt") != NULL)
                sscanf(pStr, "VideoEncoderToken_%d_%d_%d", &devNo, &channelNo, &streamNo);
            else
            	sscanf(pStr, "VideoEncoder2Token_%d_%d_%d", &devNo, &channelNo, &streamNo);
            */
            sscanf(pStr, "%*[^_]_%d_%d_%d", &devNo, &channelNo, &streamNo);
        }
        else if(token == NULL)
        {
            token = mxmlFindElement(pRoot, pRoot, EleName, NULL,
                                    NULL, MXML_DESCEND_ALL);
            if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
            {
                sscanf(pStr, "VideoEncoderToken_%d_%d_%d", &devNo, &channelNo, &streamNo);
            }
        }
    }
    else if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        sscanf(pStr, "%*[^_]_%d_%d_%d", &devNo, &channelNo, &streamNo);
    }

    //LOGW("devNo=%d channelNo=%d streamNo=%d\n", devNo, channelNo, streamNo);

    RestOnvif_RequestVideoEncoderCapability(devNo - 1, channelNo - 1, streamNo - 1,
                                            &videoCfg);
    HttpRsp_GetVideoEncoderConfigurationOptions(httpStr, len,
            &videoCfg, xmlDetail);
    return 0;
}

static int OnvifAction_SetVideoEncoderConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)

{
    ONVIF_VideoEncoderCfg_T videoCfg;
    ONVIF_MultiCast_T tMultiCast;
    memset(&videoCfg, 0, sizeof(videoCfg));
    memset(&tMultiCast, 0, sizeof(tMultiCast));
    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    int devNo = 1, channelNo = 1, streamNo = 1;
    char *pStr = NULL;
    char EleName[32] = {0};
    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, "Configuration", NULL, NULL,
                                         MXML_DESCEND_ALL);

    if(token == NULL)
    {
        snprintf(EleName,31,"%s:Configuration",xmlDetail->actionNs);
        token = mxmlFindElement(pRoot, pRoot, EleName, NULL, NULL,
                                MXML_DESCEND_ALL);

        if (token != NULL
                && (pStr = (char *)mxmlElementGetAttr(token, "token")) != NULL)
        {
            /*
            if (strstr(token->value.element.name, "trt") != NULL)
                sscanf(pStr, "VideoEncoderToken_%d_%d_%d", &devNo, &channelNo, &streamNo);
            else
                sscanf(pStr, "VideoEncoder2Token_%d_%d_%d", &devNo, &channelNo, &streamNo);
            */
            sscanf(pStr, "%*[^_]_%d_%d_%d", &devNo, &channelNo, &streamNo);

            RestOnvif_GetVideoEncoderCfg(devNo - 1, channelNo - 1, streamNo - 1, &videoCfg);
            RestOnvif_GetMultiCastCfg(devNo - 1, channelNo - 1, streamNo - 1, &tMultiCast);

            if (token != NULL
                && (pStr = (char *)mxmlElementGetAttr(token, "GovLength")) != NULL)
            {
                videoCfg.Iinterval = atoi(pStr);
            }

            token = mxmlFindElement(pRoot, pRoot, "tt:Encoding", NULL, NULL, MXML_DESCEND_ALL);
            if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
            {
                if (strstr(pStr, "H264") != NULL)
                {
                    videoCfg.encodeFormat = 0;
                }
                else if (strstr(pStr, "H265+") != NULL)
                {
                    videoCfg.encodeFormat = 8;
                }
                else if (strstr(pStr, "H265") != NULL)
                {
                    videoCfg.encodeFormat = 1;
                }
                else if (strstr(pStr, "MJPEG") != NULL || strstr(pStr, "JPEG") != NULL)
                {
                    videoCfg.encodeFormat = 3;
                }
            }

            token = mxmlFindElement(pRoot, pRoot, "tt:Resolution", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if (token != NULL)
            {
                mxml_node_t *Child = mxmlFindElement(token, token, "tt:Width", NULL, NULL,
                                                     MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    videoCfg.resolution[0] = atoi(pStr);
                }

                Child = mxmlFindElement(token, token, "tt:Height", NULL, NULL, MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    videoCfg.resolution[1] = atoi(pStr);
                }
            }

            token = mxmlFindElement(pRoot, pRoot, "tt:Quality", NULL, NULL, MXML_DESCEND_ALL);
            if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
            {
                if(strstr(xmlDetail->actionNs, "trt") != NULL)
                {
                    videoCfg.encQuality = atoi(pStr);
                }
                else
                {
                    videoCfg.encQuality = atoi(pStr);
                }
            }

            token = mxmlFindElement(pRoot, pRoot, "tt:RateControl", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if (token != NULL)
            {
                mxml_node_t *Child = mxmlFindElement(token, token, "tt:FrameRateLimit", NULL,
                                                     NULL,
                                                     MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    videoCfg.resolution[2] = atoi(pStr);
                }

                Child = mxmlFindElement(token, token, "tt:BitrateLimit", NULL, NULL,
                                        MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    videoCfg.bitrate = atoi(pStr);

                    //videoCfg.bitrateCtrlMode = 1;//CBR
                }

                if (token != NULL
                        && (pStr = (char *)mxmlElementGetAttr(token, "ConstantBitRate")) != NULL)
                {
                    if (pStr != NULL && strcmp(pStr, "false") == 0)
                    {
                        videoCfg.bitrateCtrlMode = 0;
                        LOGD("set vbr\n");
                    }
                    else
                    {
                        LOGD("set cbr\n");
                        videoCfg.bitrateCtrlMode = 1;
                    }
                }
                else
                {
                    videoCfg.bitrateCtrlMode = 1;
                }
            }

            if(1)//videoCfg.encodeFormat == 0)
            {
                token = mxmlFindElement(pRoot, pRoot, "tt:H264", NULL, NULL, MXML_DESCEND_ALL);
                if (token != NULL)
                {
                    mxml_node_t *Child = mxmlFindElement(token, token, "tt:GovLength", NULL, NULL,
                                                         MXML_DESCEND_ALL);
                    if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                    {
                        videoCfg.Iinterval = atoi(pStr);
                    }

                    Child = mxmlFindElement(token, token, "tt:H264Profile", NULL, NULL,
                                            MXML_DESCEND_ALL);
                    if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                    {
                        if (strstr(pStr, "Baseline") != NULL)
                        {
                            videoCfg.profiles = 0;
                        }
                        else if (strstr(pStr, "Main") != NULL)
                        {
                            videoCfg.profiles = 1;
                        }
                        else if (strstr(pStr, "High") != NULL)
                        {
                            videoCfg.profiles = 2;
                        }
                    }
                }
            }

            token = mxmlFindElement(pRoot, pRoot, "tt:Multicast", NULL, NULL, MXML_DESCEND_ALL);
            if (token != NULL)
            {
                mxml_node_t *Child = mxmlFindElement(token, token, "tt:Address", NULL, NULL,
                                                     MXML_DESCEND_ALL);
                if (Child != NULL)
                {
                    mxml_node_t *pTemp = mxmlFindElement(Child, Child, "tt:IPv4Address", NULL, NULL,
                                                         MXML_DESCEND_ALL);
                    if (pTemp != NULL && (pStr = (char *)mxmlGetText(pTemp, NULL)) != NULL)
                    {
                        snprintf(tMultiCast.ipV4, sizeof(tMultiCast.ipV4), "%s", pStr);
                    }
                }

                Child = mxmlFindElement(token, token, "tt:Port", NULL, NULL, MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    tMultiCast.port = atoi(pStr);
                }

                Child = mxmlFindElement(token, token, "tt:TTL", NULL, NULL, MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    tMultiCast.ttl = atoi(pStr);
                }

                Child = mxmlFindElement(token, token, "tt:AutoStart", NULL, NULL, MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    if (strstr(pStr, "true") != NULL)
                    {
                        tMultiCast.bEnable = 1;
                    }
                    else
                    {
                        tMultiCast.bEnable = 0;
                    }
                }
            }
        }
    }
    else
    {
        if (token != NULL
                && (pStr = (char *)mxmlElementGetAttr(token, "token")) != NULL)
        {
            //sscanf(pStr, "VideoEncoderToken_%d_%d_%d", &devNo, &channelNo, &streamNo);
            sscanf(pStr, "%*[^_]_%d_%d_%d", &devNo, &channelNo, &streamNo);
        }

        RestOnvif_GetVideoEncoderCfg(devNo - 1, channelNo - 1, streamNo - 1, &videoCfg);
        RestOnvif_GetMultiCastCfg(devNo - 1, channelNo - 1, streamNo - 1, &tMultiCast);

        if (token != NULL
            && (pStr = (char *)mxmlElementGetAttr(token, "GovLength")) != NULL)
        {
            videoCfg.Iinterval = atoi(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "Encoding", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            if (strstr(pStr, "H264") != NULL)
            {
                videoCfg.encodeFormat = 0;
            }
            else if (strstr(pStr, "H265+") != NULL)
            {
                videoCfg.encodeFormat = 8;
            }
            else if (strstr(pStr, "H265") != NULL)
            {
                videoCfg.encodeFormat = 1;
            }
            else if (strstr(pStr, "MJPEG") != NULL)
            {
                videoCfg.encodeFormat = 3;
            }
        }

        token = mxmlFindElement(pRoot, pRoot, "Resolution", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL)
        {
            mxml_node_t *Child = mxmlFindElement(token, token, "Width", NULL, NULL,
                                                 MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                videoCfg.resolution[0] = atoi(pStr);
            }

            Child = mxmlFindElement(token, token, "Height", NULL, NULL, MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                videoCfg.resolution[1] = atoi(pStr);
            }
        }

        token = mxmlFindElement(pRoot, pRoot, "Quality", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
        {
            videoCfg.encQuality = atoi(pStr);
        }

        token = mxmlFindElement(pRoot, pRoot, "RateControl", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL)
        {
            mxml_node_t *Child = mxmlFindElement(token, token, "FrameRateLimit", NULL, NULL,
                                                 MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                videoCfg.resolution[2] = atoi(pStr);
            }

            Child = mxmlFindElement(token, token, "BitrateLimit", NULL, NULL, MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                videoCfg.bitrate = atoi(pStr);
                //videoCfg.bitrateCtrlMode = 1;//CBR
            }

            if (token != NULL
                    && (pStr = (char *)mxmlElementGetAttr(token, "ConstantBitRate")) != NULL)
            {
                if (pStr != NULL && strcmp(pStr, "false") == 0)
                {
                    videoCfg.bitrateCtrlMode = 0;
                    LOGD("set vbr\n");
                }
                else
                {
                    LOGD("set cbr\n");
                    videoCfg.bitrateCtrlMode = 1;
                }
            }
            else
            {
                videoCfg.bitrateCtrlMode = 1;
            }
        }

        if(1)//videoCfg.encodeFormat == 0)
        {
            token = mxmlFindElement(pRoot, pRoot, "H264", NULL, NULL, MXML_DESCEND_ALL);
            if (token != NULL)
            {
                mxml_node_t *Child = mxmlFindElement(token, token, "GovLength", NULL, NULL,
                                                     MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    videoCfg.Iinterval = atoi(pStr);
                }

                Child = mxmlFindElement(token, token, "H264Profile", NULL, NULL, MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
                {
                    if (strstr(pStr, "Baseline") != NULL)
                    {
                        videoCfg.profiles = 0;
                    }
                    else if (strstr(pStr, "Main") != NULL)
                    {
                        videoCfg.profiles = 1;
                    }
                    else if (strstr(pStr, "High") != NULL)
                    {
                        videoCfg.profiles = 2;
                    }
                }
            }
        }

        token = mxmlFindElement(pRoot, pRoot, "Multicast", NULL, NULL, MXML_DESCEND_ALL);
        if (token != NULL)
        {
            mxml_node_t *Child = mxmlFindElement(token, token, "Address", NULL, NULL,
                                                 MXML_DESCEND_ALL);
            if (Child != NULL)
            {
                mxml_node_t *pTemp = mxmlFindElement(Child, Child, "IPv4Address", NULL, NULL,
                                                     MXML_DESCEND_ALL);
                if (pTemp != NULL && (pStr = (char *)mxmlGetText(pTemp, NULL)) != NULL)
                {
                    snprintf(tMultiCast.ipV4, sizeof(tMultiCast.ipV4), "%s", pStr);
                }
            }

            Child = mxmlFindElement(token, token, "Port", NULL, NULL, MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                tMultiCast.port = atoi(pStr);
            }

            Child = mxmlFindElement(token, token, "TTL", NULL, NULL, MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                tMultiCast.ttl = atoi(pStr);
            }

            Child = mxmlFindElement(token, token, "AutoStart", NULL, NULL, MXML_DESCEND_ALL);
            if (Child != NULL && (pStr = (char *)mxmlGetText(Child, NULL)) != NULL)
            {
                if (strstr(pStr, "true") != NULL)
                {
                    tMultiCast.bEnable = 1;
                }
                else
                {
                    tMultiCast.bEnable = 0;
                }
            }
        }
    }

    RestOnvif_SetVideoEncoderCfg(devNo - 1, channelNo - 1, streamNo - 1, &videoCfg);

    RestOnvif_SetMultiCastCfg(devNo - 1, channelNo - 1, streamNo - 1, &tMultiCast);

    HttpRsp_SetVideoEncoderConfiguration(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_SetNetworkInterfaces(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_NET_INFO_T netInfo;
    memset(&netInfo, 0, sizeof(ONVIF_NET_INFO_T));
	if(httpStr == NULL)
	{
		LOGW("\n---------httpStr is NULL!\n");
		RestOnvif_RequesNetInfo(&netInfo);
	}

    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    char *pStr = NULL;

    mxml_node_t *token = NULL;
    char tokenName[32] = {0};
    int actionNs_len = strlen(xmlDetail->actionNs);

    if (actionNs_len)
        snprintf(tokenName, sizeof(tokenName), "%s:InterfaceToken", xmlDetail->actionNs);
    else
        snprintf(tokenName, sizeof(tokenName), "InterfaceToken");

    token = mxmlFindElement(pRoot, pRoot, tokenName, NULL,
                                         NULL,
                                         MXML_DESCEND_ALL);
    if (token && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        snprintf(netInfo.ethName, sizeof(netInfo.ethName), "%s", pStr);
    }

    if (actionNs_len)
        snprintf(tokenName, sizeof(tokenName), "%s:NetworkInterface", xmlDetail->actionNs);
    else
        snprintf(tokenName, sizeof(tokenName), "NetworkInterface");

    token = mxmlFindElement(pRoot, pRoot, tokenName, NULL, NULL,
                            MXML_DESCEND_ALL);
    if (token)
    {
        snprintf(tokenName, sizeof(tokenName), "%sIPv4",actionNs_len?"tt:":"");
        mxml_node_t *Child = mxmlFindElement(token, token, tokenName, NULL, NULL,
                                             MXML_DESCEND_ALL);
        if (Child != NULL)
        {
            snprintf(tokenName, sizeof(tokenName), "%sManual",actionNs_len?"tt:":"");
            mxml_node_t *pTemp = mxmlFindElement(Child, Child, tokenName, NULL, NULL,
                                                 MXML_DESCEND_ALL);
            if (pTemp != NULL)
            {
                snprintf(tokenName, sizeof(tokenName), "%sAddress",actionNs_len?"tt:":"");
                mxml_node_t *pChild2 = mxmlFindElement(pTemp, pTemp, tokenName, NULL, NULL,
                                                       MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(pChild2, NULL)) != NULL)
                {
                    snprintf(netInfo.ipV4, sizeof(netInfo.ipV4), "%s", pStr);
                }

                snprintf(tokenName, sizeof(tokenName), "%sPrefixLength",actionNs_len?"tt:":"");
                pChild2 = mxmlFindElement(pTemp, pTemp, tokenName, NULL, NULL,
                                          MXML_DESCEND_ALL);
                if (Child != NULL && (pStr = (char *)mxmlGetText(pChild2, NULL)) != NULL)
                {
                    int iPrefixLength = atoi(pStr);
                    if(iPrefixLength == 24)
                    {
                        snprintf(netInfo.ipMaskV4, sizeof(netInfo.ipMaskV4), "255.255.255.0");
                    }
                    else if(iPrefixLength == 16)
                    {
                        snprintf(netInfo.ipMaskV4, sizeof(netInfo.ipMaskV4), "255.255.0.0");
                    }
                    else if(iPrefixLength == 8)
                    {
                        snprintf(netInfo.ipMaskV4, sizeof(netInfo.ipMaskV4), "255.0.0.0");
                    }
                }
            }

            snprintf(tokenName, sizeof(tokenName), "%sDHCP",actionNs_len?"tt:":"");
            pTemp = mxmlFindElement(Child, Child, tokenName, NULL, NULL,
                                    MXML_DESCEND_ALL);
            if (pTemp != NULL && (pStr = (char *)mxmlGetText(pTemp, NULL)) != NULL)
            {
                if (strstr(pStr, "true") != NULL)
                {
                    netInfo.bDhcp = 1;
                }
                else
                {
                    netInfo.bDhcp = 0;
                }
            }
        }
    }

	if(httpStr)
	{
		HttpRsp_SetNetworkInterfaces(httpStr, len, xmlDetail);
	}
	else
	{
		RestOnvif_SetNetInfo(&netInfo);
	}
    return 0;
}

static int OnvifAction_SetNetworkDefaultGateway(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_NET_INFO_T netInfo;
    memset(&netInfo, 0, sizeof(ONVIF_NET_INFO_T));
    RestOnvif_RequesNetInfo(&netInfo);
    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    char *pStr = NULL;

    char tokenName[32] = {0};
    int actionNs_len = strlen(xmlDetail->actionNs);

    if (actionNs_len)
        snprintf(tokenName, sizeof(tokenName), "%s:IPv4Address", xmlDetail->actionNs);
    else
        snprintf(tokenName, sizeof(tokenName), "IPv4Address");

    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, tokenName, NULL,
                                         NULL,
                                         MXML_DESCEND_ALL);
    if (token && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        snprintf(netInfo.gateWayV4, sizeof(netInfo.gateWayV4), "%s", pStr);
    }

    HttpRsp_SetNetworkDefaultGateway(httpStr, len, xmlDetail);

    RestOnvif_SetNetInfo(&netInfo);

    return 0;
}


static int OnvifAction_GetOSDOptions(char *httpStr, int len,
                                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    HttpRsp_GetOSDOptions(httpStr, len, xmlDetail);
    return 0;
}


static int OnvifAction_GetMoveOptions(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    HttpRsp_GetMoveOptions(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_SetSystemDateAndTime(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{

    ONVIF_TIME_ALL_T timeAll;
    memset(&timeAll, 0, sizeof(ONVIF_TIME_ALL_T));
    char tzName[32] = { 0 };
    int tzOffset = 0;
    char tzDstName[32] = { 0 };
	RestOnvif_RequestGetTimeAll(&timeAll);
    XmlParser_DateTimeParam(xmlDetail, &timeAll, tzName, &tzOffset, tzDstName);
    /* RestOnvif_GetCfgTimeZone(tzName,sizeof(tzName),&tzOffset,tzDstName,sizeof(tzDstName)); */
    //RestOnvif_SetCfgTimeZone(tzName, tzOffset, tzDstName);
    RestOnvif_RequestSetTimeAll(&timeAll);
    HttpRsp_SetSystemDateAndTime(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetVideoAnalyticsConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MOTION_ATTR_T motion;
    memset(&motion, 0, sizeof(ONVIF_MOTION_ATTR_T));
    RestOnvif_RequestMotionAttr(&motion);

    HttpRsp_GetVideoAnalyticsConfigurations(httpStr, len, xmlDetail, &motion);
    return 0;
}


static int OnvifAction_GetAnalyticsModules(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MOTION_ATTR_T motion;
    memset(&motion, 0, sizeof(ONVIF_MOTION_ATTR_T));
    RestOnvif_RequestMotionAttr(&motion);

    HttpRsp_GetAnalyticsModules(httpStr, len, xmlDetail, &motion);
    return 0;
}

static int OnvifAction_GetVideoEncoderConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_VideoEncoderCfg_T stream;

    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;
    int devNo = 1, channelNo = 1, streamNo = 1;
    char *pStr = NULL;
    char EleName[32] = {0};

    snprintf(EleName,31,"%s:ConfigurationToken",xmlDetail->actionNs);
    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, EleName,
                                         NULL, NULL,
                                         MXML_DESCEND_ALL);

    if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        char *ele_name = mxmlGetElement(token);
        if (strstr(ele_name, "trt") != NULL)
            sscanf(pStr, "VideoEncoderToken_%d_%d_%d", &devNo, &channelNo, &streamNo);
        else
            sscanf(pStr, "VideoEncoder2Token_%d_%d_%d", &devNo, &channelNo, &streamNo);
    }

    RestOnvif_GetVideoEncoderCfg(devNo - 1, channelNo - 1, streamNo - 1,  &stream);

    HttpRsp_GetVideoEncoderConfiguration(httpStr, len, xmlDetail,
                                         devNo - 1, channelNo - 1, streamNo - 1, &stream);
    return 0;
}

static int OnvifAction_GetVideoEncoderConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_VideoEncoderCfg_T stream[2] = {};

    int devNo = 1, channelNo = 1, streamNo = 0;
    char *pStr = NULL;
    char EleName[32] = {0};
    mxml_node_t *token = NULL;
    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    snprintf(EleName,31,"%s:ConfigurationToken",xmlDetail->actionNs);

    token = mxmlFindElement(pRoot, pRoot, EleName, NULL,
                                NULL, MXML_DESCEND_ALL);
    if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        LOGD("\n-----------token != NULL!\n");
        char *ele_name = mxmlGetElement(token);
        if (strstr(ele_name, "trt") != NULL)
            sscanf(pStr, "VideoEncoderToken_%d_%d_%d", &devNo, &channelNo, &streamNo);
        else
            sscanf(pStr, "VideoEncoder2Token_%d_%d_%d", &devNo, &channelNo, &streamNo);
        LOGW("\n-----------devNo=%d channelNo=%d streamNo=%d\n", devNo, channelNo, streamNo);
    }

    if(streamNo)
    {
        RestOnvif_GetVideoEncoderCfg(0, 0, streamNo-1, &stream[streamNo-1]);
    }
    else
    {
        RestOnvif_GetVideoEncoderCfg(0, 0, 0, &stream[0]);
        RestOnvif_GetVideoEncoderCfg(0, 0, 1, &stream[1]);
    }

    HttpRsp_GetVideoEncoderConfigurations(httpStr, len, xmlDetail, stream,streamNo);

    return 0;
}

static int OnvifAction_GetAudioEncoderConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioEncoderConfiguration(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioEncoderConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioEncoderConfigurations(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioEncoderConfigurationOptions(char *httpStr,
        int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioEncoderConfigurationOptions(httpStr, len , xmlDetail);
    return 0;
}

static int OnvifAction_GetRules(char *httpStr, int len,
                                ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MOTION_ATTR_T motion;
    memset(&motion, 0, sizeof(ONVIF_MOTION_ATTR_T));
    RestOnvif_RequestMotionAttr(&motion);

    HttpRsp_GetRules(httpStr, len, xmlDetail, &motion);
    return 0;
}

static int OnvifAction_Subscribe(char *httpStr, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_SUBSCRIBE_NODE_T *subNode =
        (ONVIF_SUBSCRIBE_NODE_T *)ONVIF_MALLOC(sizeof(ONVIF_SUBSCRIBE_NODE_T));
    memset(subNode, 0, sizeof(ONVIF_SUBSCRIBE_NODE_T));

    if (XmlParser_Subscrib(xmlDetail, subNode) < 0)
    {
        ONVIF_FREE(subNode);
        return -1;
    }

    LOGW("[%s][%s][%d]",subNode->ip,subNode->uri,subNode->token);
    Common_DList_InsertTail(s_onvif_mgr_ct.subscribeList,
                            subNode, sizeof(ONVIF_SUBSCRIBE_NODE_T));

    ONVIF_WEB_ATTR_T webAttr;
    RestOnvif_RequestWebServer(&webAttr);
    HttpRsp_Subscribe(httpStr, len, xmlDetail, subNode, &webAttr);
    return 0;

}

static int OnvifAction_Renew(char *httpStr, int len,
                             ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    char uri[128] = { 0 };
    snprintf(uri, (xmlDetail->httpResult->uriLen < sizeof(uri)) ?
             xmlDetail->httpResult->uriLen + 1 : sizeof(uri), "%s",
             xmlDetail->httpResult->uriStr);


    unsigned int token = 0;
    char *p = strrchr((char *)uri, '_');
    if (p != NULL)
    {
        token = (unsigned int)atoi(p + 1);
    }
    else
    {

        if (Common_DList_Search(s_onvif_mgr_ct.subscribeList,
                                contentStr, OnvifEventSearchByContent) == NULL)
        {
            LOGW("Not match any token\n");
            HttpRsp_Unauth401(httpStr, len);
        }
        else
        {
            HttpRsp_Renew(httpStr, len, xmlDetail);
        }

        return 0;
    }

    if (Common_DList_Search(s_onvif_mgr_ct.subscribeList, &token, OnvifEventSearchByToken) == NULL)
        HttpRsp_Unauth401(httpStr, len);
    else
        HttpRsp_Renew(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_Unsubscribe(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    char uri[128] = { 0 };
    snprintf(uri, (xmlDetail->httpResult->uriLen < sizeof(uri)) ? xmlDetail->httpResult->uriLen :
             sizeof(uri) - 1, "%s", xmlDetail->httpResult->uriStr);

    unsigned int token = 0;
    char *p = strrchr((char *)uri, '_');
    if (p != NULL)
        token = (unsigned int)atoi(p + 1);

    Common_DList_Delete(s_onvif_mgr_ct.subscribeList, &token, OnvifEventSearchByToken);
    HttpRsp_Unsubscribe(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetServiceCapabilities(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    HttpRsp_GetServiceCapability(httpStr, len, xmlDetail);
    return 0;
}


static int OnvifAction_GetEventPropertyies(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    HttpRsp_GetEventProperties(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_CreateOSD(char *httpStr, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr = {};
    char *fieldName = (char *)"OSDToken_100";

    RestOnvif_RequestGetOsd(&osdAttr);
    RestOnvif_RequestGetMultiOsd(&osdAttr);
    /* osdAttr.ChannelEnable = -1; */
    /* osdAttr.TimeEnable = -1; */
    /* osdAttr.multiAttr[0].MultiEnable = -1; */
    int type = XmlParser_CreateOsdParam(xmlDetail, &osdAttr, contentStr);
    RestOnvif_RequestSetOsd(&osdAttr);

    if (type == 0 && osdAttr.channelOsdAttr.osdEnable != -1)
    {
        fieldName = (char *)"OSDToken_ChannelOSD";
    }
    else if (type == 1 && osdAttr.datetimeOsdAttr.osdEnable != -1)
    {
        fieldName = (char *)"OSDToken_DateTimeOSD";
    }
    else if (type == 2 && osdAttr.multiOsdAttr.osdEnable != -1)
    {
        fieldName = (char *)"OSDToken_MultiOSD";
    }

    HttpRsp_CreateOSD(httpStr, len, xmlDetail, fieldName);
    return 0;
}

static int OnvifAction_DeleteOSD(char *httpStr, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr;
    memset(&osdAttr, 0, sizeof(ONVIF_OSD_ATTR_T));
    osdAttr.channelOsdAttr.osdEnable = -1;
    osdAttr.datetimeOsdAttr.osdEnable = -1;
    osdAttr.multiOsdAttr.osdEnable = -1;
    XmlParser_DeleteOsdParam(xmlDetail, &osdAttr);
    LOGE("channel %d time %d mul %d\n", osdAttr.channelOsdAttr.osdEnable,
         osdAttr.datetimeOsdAttr.osdEnable,
         osdAttr.multiOsdAttr.osdEnable);

    RestOnvif_RequestDeleteOsd(&osdAttr);
    HttpRsp_DeleteOSD(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_CreatePullPointSubscription(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    if(strstr(contentStr,"Filter") != NULL)
    {
        HttpRsp_BadReq400(httpStr, len);
        return 0;
    }

    ONVIF_SUBSCRIBE_NODE_T *subNode =
        (ONVIF_SUBSCRIBE_NODE_T *)ONVIF_MALLOC(sizeof(ONVIF_SUBSCRIBE_NODE_T));
    memset(subNode, 0, sizeof(ONVIF_SUBSCRIBE_NODE_T));

    subNode->isPullPoint = 1;
    subNode->updateTime = Common_GetSystemCount64();
    subNode->token = Common_Rand32();

    LOGW("[%s][%s][%d]",subNode->ip,subNode->uri,subNode->token);

    Common_DList_InsertTail(s_onvif_mgr_ct.subscribeList,
                            subNode, sizeof(ONVIF_SUBSCRIBE_NODE_T));

    ONVIF_WEB_ATTR_T webAttr;
    RestOnvif_RequestWebServer(&webAttr);
    HttpRsp_CreatePullPointSubscription(httpStr, len, xmlDetail, subNode, &webAttr);
    return 0;
}

static int OnvifAction_PullMessages(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    ONVIF_SUBSCRIBE_NODE_T *subNode = NULL;
    char uri[128] = { 0 };
    snprintf(uri, (xmlDetail->httpResult->uriLen < sizeof(uri)) ?
             xmlDetail->httpResult->uriLen + 1 : sizeof(uri), "%s",
             xmlDetail->httpResult->uriStr);

    unsigned int token = 0;
    char *p = strrchr((char *)uri, '_');
    if (p != NULL)
        token = (unsigned int)atoi(p + 1);

    subNode = (ONVIF_SUBSCRIBE_NODE_T *)Common_DList_Search(
                  s_onvif_mgr_ct.subscribeList, &token, OnvifEventSearchByToken);

    if (subNode == NULL)
    {
        LOGE("not find subNode %d\n", token);
        subNode =
            (ONVIF_SUBSCRIBE_NODE_T *)ONVIF_MALLOC(sizeof(ONVIF_SUBSCRIBE_NODE_T));
        memset(subNode, 0, sizeof(ONVIF_SUBSCRIBE_NODE_T));

        subNode->isPullPoint = 1;
        subNode->updateTime = Common_GetSystemCount64();
        subNode->token = 0;
        LOGW("[%s][%s][%d]",subNode->ip,subNode->uri,subNode->token);

        Common_DList_InsertTail(s_onvif_mgr_ct.subscribeList,
                                subNode, sizeof(ONVIF_SUBSCRIBE_NODE_T));
    }

    if (subNode == NULL)
    {
        HttpRsp_BadReq400(httpStr, len);
        return 0;
    }

    ONVIF_ALARM_STATUS_T alarmStatusInfo = { 0 };
    int ret = -1, i = 0;
    Mq_Request(s_onvif_mgr_ct.mqHandle, ONVIF_REQ_RTSP_GET_APP_DATA, NULL, 0, &ret,
               (void *)(&alarmStatusInfo),
               sizeof(ONVIF_ALARM_STATUS_T));

    //if (ret != 0 || alarmStatusInfo.alarmStatusCnt <= 0)
    //{
    //    LOGE("ret %d cnt %d\n", ret , alarmStatusInfo.alarmStatusCnt);
    //    HttpRsp_PullMessage(httpStr, len, xmlDetail, subNode, -1);
    //    /* HttpRsp_BadReq400(httpStr, len); */
    //    return 0;
    //}

    /*waiting a second to send response message*/
    Common_Sleep(1, 0);

    int isHappen = 0;
    ret = -1;
    for (i = 0; i < alarmStatusInfo.alarmStatusCnt; i++)
    {
        ONVIF_ALARM_STATUS_NODE_T *node = alarmStatusInfo.alarmStatus + i;
        if (strcmp(node->alarmName, "Motion") == 0)
        {
            isHappen = node->status;
            ret = 0;
            break;
        }
    }

    if(alarmStatusInfo.alarmStatus)
        ONVIF_FREE(alarmStatusInfo.alarmStatus);

    if (ret < 0)
    {
        HttpRsp_PullMessage(httpStr, len, xmlDetail, subNode, -1);
        return 0;
    }

    /**/
    if (subNode->isSend != 1 && isHappen == 0)
    {
        HttpRsp_PullMessage(httpStr, len, xmlDetail, subNode, -1);
        return 0;
    }

    HttpRsp_PullMessage(httpStr, len, xmlDetail, subNode, isHappen);
    if (isHappen == 1)
        subNode->isSend = 1;
    else
        subNode->isSend = 2;

    return 0;
}


static int OnvifAction_GetAudioSources(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
    HttpRsp_GetAudioSource(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioSourceConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioSourceConfigurations(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioSourceConfigurationOptions(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioSourceConfigurationOptions(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetMetadataConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetMetadataConfigurations(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioOutputConfigurationOptions(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioOutputConfigurationOptions(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioOutputConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioOutputConfigurations(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioDecoderConfigurationOptions(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioDecoderConfigurationOptions(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetAudioDecoderConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetAudioDecoderConfigurations(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetSnapshotUri(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                      char *contentStr)
{

    int devNo = 0, channelNo = 0, streamNo = 0;
    char *pStr = NULL;
    char EleName[32] = {0};

    LOGW("[%s][%d]",xmlDetail->actionNs,strlen(xmlDetail->actionNs));

    if(strlen(xmlDetail->actionNs))
    {
        snprintf(EleName,31,"%s:ProfileToken",xmlDetail->actionNs);
    }
    else
    {
        snprintf(EleName,31,"ProfileToken");
    }

    mxml_node_t *token = mxmlFindElement(xmlDetail->xmlParam, xmlDetail->xmlParam, EleName, NULL, NULL,
                            MXML_DESCEND_ALL);

    if (token != NULL)
    {
        char *child_node = mxmlGetFirstChild(token);
        char *node_text = NULL;
        if (child_node) {
            node_text = mxmlGetText(child_node, NULL);
        }
        if (node_text != NULL) {
            if(0 == Common_StrnCmp(node_text, "CH", 2))
            {
                sscanf(node_text, "CH%02d",&channelNo);
                if(channelNo > 0)
                {
                    devNo = 1;
                    streamNo = 1;
                    if(strstr(node_text,"_sub"))
                    {
                        streamNo = 2;
                    }
                }
            }
            else
            {
                char strTmp[16] = {0};
                sscanf(node_text, "%[^_]_%d_%d_%d",&strTmp,&devNo,&channelNo,&streamNo);
            }
        }

        if (devNo > 0 && channelNo > 0 && streamNo > 0)
        {
            devNo--;
            channelNo--;
            streamNo--;
        }
        else
        {
            devNo = 0;
            channelNo = 0;
            streamNo = 0;
        }

    }
    else
    {
        LOGW("token is null!\n");
    }

    char hostaddr[16] = {0};
    snprintf(hostaddr,xmlDetail->httpResult->hostAddrLen,xmlDetail->httpResult->hostAddrStr);

    ONVIF_WEB_ATTR_T webAttr = {};
    RestOnvif_RequestWebServer(&webAttr);

    char picUrl[128] = {0};
    snprintf(picUrl, sizeof(picUrl),"http://%s:%d/onvif/GetSnapshot?stream=%d",hostaddr,webAttr.httpPort,streamNo);

    HttpRsp_GetSnapshotUri(httpStr, len, xmlDetail, picUrl);
    return 0;
}

static int OnvifAction_GetSnapShot(void *udsHandle, int conHandle, char *recv, int len)
{
    int stream = 0;
    sscanf(recv+23, "stream=%d", &stream);
    LOGD("stream:[%d]\n",stream);

    char PicPath[128] = {0};
    RestOnvif_RequestSnap(0, 0, stream, PicPath);
    if(strlen(PicPath) == 0)
    {
        LOGE("get pic failed!\n");
        return -1;
    }

    FILE *fd = NULL;
    int nlen = 0;
    fd = Common_File_fOpen(PicPath, "r");
    if(fd == NULL)
    {
        LOGE("fd open failed!\n");
        return -1;
    }

    Common_File_fSeek(fd,0,SEEK_END);
	nlen = Common_File_fTell(fd);
	Common_File_fSeek(fd,0,SEEK_SET);

    char tmp[256] = {0};

    snprintf(tmp,256,"HTTP/1.1 200 OK\r\nServer: onvifserver\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",nlen);

    int rsp_len = nlen+strlen(tmp);
    char *rsp = (char *)ONVIF_MALLOC(rsp_len);

    snprintf(rsp,rsp_len,"%s",tmp);

    Common_File_fRead(rsp+strlen(rsp), 1, nlen, fd);
    Common_File_fClose(fd);

    Utils_UdsServerSend(udsHandle, conHandle, rsp, rsp_len);

    ONVIF_FREE(rsp);

    return 0;
}

static int OnvifAction_GetOSD(char *httpStr, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                              char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr = {};
    RestOnvif_RequestGetOsd(&osdAttr);
    RestOnvif_RequestGetMultiOsd(&osdAttr);
    HttpRsp_GetOsd(httpStr, len, xmlDetail, &osdAttr);
    return 0;
}

#if (defined ONVIF_EXT_GPS)
static int OnvifAction_PushAnalogInfo(char *httpStr, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                      char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr = {};
    memset(&osdAttr, 0, sizeof(ONVIF_OSD_ATTR_T));
    osdAttr.channelOsdAttr.osdEnable = -1;
    osdAttr.datetimeOsdAttr.osdEnable = -1;
    ONVIF_TIME_ALL_T timeAll = {};

    RestOnvif_RequestGetTimeAll(&timeAll);
    XmlParser_PushAnalogInfo(xmlDetail, &osdAttr, &timeAll, &s_onvif_mgr_ct.gpsinfo, contentStr);
    XmlParser_PushStationInfo(xmlDetail, &osdAttr, &timeAll, &s_onvif_mgr_ct.gpsinfo, contentStr);

    if (timeAll.utc.Year > 0)
        RestOnvif_RequestSetTimeAll(&timeAll);

    osdAttr.multiOsdAttr.osdEnable = 1;
    RestOnvif_RequestSetMultiOsd(&osdAttr);

    HttpRsp_PushAnalogInfo(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_PushStationInfo(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
    ONVIF_OSD_ATTR_T osdAttr = {};
    memset(&osdAttr, 0, sizeof(ONVIF_OSD_ATTR_T));
    osdAttr.channelOsdAttr.osdEnable = -1;
    osdAttr.datetimeOsdAttr.osdEnable = -1;
    ONVIF_TIME_ALL_T timeAll = {};

    /* RestOnvif_RequestGetTimeAll(&timeAll); */
    XmlParser_PushStationInfo(xmlDetail, &osdAttr, &timeAll, &s_onvif_mgr_ct.gpsinfo, contentStr);
    XmlParser_PushAnalogInfo(xmlDetail, &osdAttr, &timeAll, &s_onvif_mgr_ct.gpsinfo, contentStr);

    osdAttr.multiOsdAttr.osdEnable = 1;
    RestOnvif_RequestSetMultiOsd(&osdAttr);

    HttpRsp_PushStationInfo(httpStr, len, xmlDetail);

    return 0;
}
#endif


static int OnvifAction_GetNetworkProtocols(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    HttpRsp_GetNetworkProtocols(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetConfigurations(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetConfigurations(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetConfiguration(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetConfiguration(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetConfigurationOptions(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetConfigurationOptions(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetPresetTours(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetPresetTours(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetPresetTour(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetPresetTour(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetNode(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetNode(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetNodes(char *httpStr, int len,
                                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                       char *contentStr)
{
	HttpRsp_GetNodes(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_GetPresets(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
	int presetCount = 0;
	ONVIF_PTZ_PRESET_INFO_T *presetArray = NULL;
	RestOnvif_RequestGetPresets(&presetArray,&presetCount);
	s_onvif_mgr_ct.ability.ptzInfo.presetsNum = presetCount;
	s_onvif_mgr_ct.ability.ptzInfo.presetsAttr = presetArray;
    HttpRsp_GetPresets(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	if(NULL != presetArray)
	{
		ONVIF_FREE(presetArray);
		s_onvif_mgr_ct.ability.ptzInfo.presetsAttr = NULL;
	}
    return 0;
}

static int OnvifAction_SetPreset(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_SetPreset(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_GotoPreset(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_GotoPreset(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_DelPreset(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_DelPreset(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_GetStatus(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_GetStatus(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_AbsoluteMove(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_AbsoluteMove(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_ContinuousMove(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_ContinuousMove(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_Move(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_Move(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_Stop(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
    HttpRsp_Stop(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
    return 0;
}

static int OnvifAction_HK_MaskOptions(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
	HttpRsp_HK_MaskOptions(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

static int OnvifAction_HK_PrivacyMask(char *httpStr, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                  char *contentStr)
{
	HttpRsp_HK_PrivacyMask(httpStr, len, xmlDetail,&s_onvif_mgr_ct.ability);
	return 0;
}

extern int ovfs_ovf_analytics_motion_sensitivity_net2local(int num);

static int OnvifAction_SetVideoAnalyticsConfiguration(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MOTION_ATTR_T motion;
    memset(&motion, 0, sizeof(ONVIF_MOTION_ATTR_T));
    RestOnvif_RequestMotionAttr(&motion);

    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;
    mxml_node_t *token = NULL, *Child = NULL, *pTemp = NULL;
    char *pStr = NULL;

    token = mxmlFindElement(pRoot, pRoot, "tt:AnalyticsModule", NULL,
                            NULL,
                            MXML_DESCEND_ALL);
    if (token != NULL)
        Child = mxmlFindElement(token, token, "tt:Parameters", NULL, NULL,
                                MXML_DESCEND_ALL);
    if(Child != NULL)
        pTemp = mxmlFindElement(Child, Child, "tt:SimpleItem", NULL, NULL,
                                MXML_DESCEND_ALL);
    if(pTemp != NULL && (pStr = (char *)mxmlElementGetAttr(pTemp, "Value")) != NULL)
    {
        int temp = atoi(pStr);
        //LOGW("pStr = %s temp=%d\n", pStr, temp);
        if(temp == 0)
        {
            motion.enable = 0;
        }
        else
        {
            motion.enable = 1;
            motion.Sensitivity = ovfs_ovf_analytics_motion_sensitivity_net2local(temp);
        }
    }

    token = Child = pTemp = NULL;

    token = mxmlFindElement(pRoot, pRoot, "tt:Rule", NULL, NULL,
                            MXML_DESCEND_ALL);
    if (token != NULL)
        Child = mxmlFindElement(token, token, "tt:Parameters", NULL, NULL,
                                MXML_DESCEND_ALL);
    if(Child != NULL)
        pTemp = mxmlFindElement(Child, Child, "tt:SimpleItem", NULL, NULL,
                                MXML_DESCEND_ALL);

    while(pTemp != NULL)
    {
        pStr = (char *)mxmlElementGetAttr(pTemp, "Name");
        if (pStr == NULL || strstr(pStr, "ActiveCells") == NULL)
        {
            //pTemp = pTemp->next;
            pTemp = mxmlWalkNext(pTemp, pRoot, MXML_DESCEND_ALL);
            continue;
        }

        pStr = (char *)mxmlElementGetAttr(pTemp, "Value");
        //LOGW("pStr = %s\n", pStr);

        unsigned char *lpCells = NULL;
        int iCellsLen = 0;
        char strValue[32 * 32 + 1];

        int i, j;
        int iByteNum, iBitNum;
        int iBitsLen = 0;
        unsigned char *lpBits = NULL;
        char *tempValue = (char *)ONVIF_MALLOC(22 * 18);;

        lpBits = (unsigned char *)ovfs_onvif_BASE64Decode(pStr, strlen(pStr),
                 &iBitsLen);
        if (lpBits != NULL && iBitsLen > 0)
        {
            lpCells = (unsigned char *)ONVIF_MALLOC(1024);
            iCellsLen = ovfs_onvif_UnPackbits(lpCells, lpBits, 1024, iBitsLen);
            // LOGW("cellLen:%d\n", iCellsLen);
            for (i = 0; i < iCellsLen; i ++)
            {
                printf("%x ", lpCells[i]);
            }
            printf("\n");
        }
        else
        {
            if(lpBits != NULL)
                ONVIF_FREE(lpBits);
            if (tempValue != NULL)
                ONVIF_FREE(tempValue);
            continue;
        }

        if(lpBits != NULL)
            ONVIF_FREE(lpBits);
        memset(strValue, 0, sizeof(strValue));

        for (i = 0; i < 18; i ++)
        {
            for (j = 0; j < 22; j ++)
            {
                iByteNum = (i * 22 + j) / 8;
                iBitNum = 7 - ((i * 22 + j) % 8);
                if((lpCells[iByteNum] >> iBitNum) & 1)
                {
                    tempValue[i * 22 + j] = 1;
                }
                else
                {
                    tempValue[i * 22 + j] = 0;
                }
            }
        }

        for (i = 0; i < 32; i ++)
        {
            for (j = 0; j < 32; j ++)
            {
                int x = (j * 22 + (32 >> 1)) / 32;
                int y = (i * 18 + (32 >> 1)) / 32;
                strValue[i * 32 + j] = tempValue[y * 22 + x] ? '1' : '0';
            }
        }
        memcpy(motion.rect, strValue, sizeof(strValue));
        ONVIF_FREE(lpCells);
        ONVIF_FREE(tempValue);
        break;
    }

    RestOnvif_SetMotionAttr(&motion);
    HttpRsp_SetVideoAnalyticsConfigurations(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_ModifyAnalyticsModules(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MOTION_ATTR_T motion;
    memset(&motion, 0, sizeof(ONVIF_MOTION_ATTR_T));
    RestOnvif_RequestMotionAttr(&motion);

    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;

    char *pStr = NULL;

    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, "tan:AnalyticsModule", NULL,
                                         NULL,
                                         MXML_DESCEND_ALL);
    if (token != NULL)
    {
        mxml_node_t *Child = mxmlFindElement(token, token, "tt:Parameters", NULL, NULL,
                                             MXML_DESCEND_ALL);
        if(Child != NULL)
        {
            mxml_node_t *pTemp = mxmlFindElement(Child, Child, "tt:SimpleItem", NULL, NULL,
                                                 MXML_DESCEND_ALL);
            if(pTemp != NULL && (pStr = (char *)mxmlElementGetAttr(pTemp, "Value")) != NULL)
            {
                int temp = atoi(pStr);
                //LOGW("pStr = %s temp=%d\n", pStr, temp);
                if(temp == 0)
                {
                    motion.enable = 0;
                }
                else
                {
                    motion.enable = 1;
                    motion.Sensitivity = ovfs_ovf_analytics_motion_sensitivity_net2local(temp);
                }
            }
        }
    }

    RestOnvif_SetMotionAttr(&motion);
    HttpRsp_ModifyAnalyticsModules(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_ModifyRules(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *contentStr)
{
    ONVIF_MOTION_ATTR_T motion;
    memset(&motion, 0, sizeof(ONVIF_MOTION_ATTR_T));
    RestOnvif_RequestMotionAttr(&motion);

    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;
    mxml_node_t *token = NULL, *Child = NULL, *pTemp = NULL;
    char *pStr = NULL;

    token = mxmlFindElement(pRoot, pRoot, "tan:Rule", NULL, NULL,
                            MXML_DESCEND_ALL);

    if (token != NULL)
        Child = mxmlFindElement(token, token, "tt:Parameters", NULL, NULL,
                                MXML_DESCEND_ALL);
    if(Child != NULL)
        pTemp = mxmlFindElement(Child, Child, "tt:SimpleItem", NULL, NULL,
                                MXML_DESCEND_ALL);

    while(pTemp != NULL)
    {
        pStr = (char *)mxmlElementGetAttr(pTemp, "Name");
        if (pStr == NULL || strstr(pStr, "ActiveCells") == NULL)
        {
            //pTemp = pTemp->next;
            pTemp = mxmlWalkNext(pTemp, pRoot, MXML_DESCEND_ALL);
            continue;
        }

        pStr = (char *)mxmlElementGetAttr(pTemp, "Value");
        //LOGW("pStr = %s\n", pStr);

        unsigned char *lpCells = NULL;
        int iCellsLen = 0;
        char strValue[32 * 32 + 1];

        int i, j;
        int iByteNum, iBitNum;
        int iBitsLen = 0;
        unsigned char *lpBits = NULL;
        char *tempValue = (char *)ONVIF_MALLOC(22 * 18);

        lpBits = (unsigned char *)ovfs_onvif_BASE64Decode(pStr, strlen(pStr),
                 &iBitsLen);
        if (lpBits != NULL && iBitsLen > 0)
        {
            lpCells = (unsigned char *)ONVIF_MALLOC(1024);
            iCellsLen = ovfs_onvif_UnPackbits(lpCells, lpBits, 1024, iBitsLen);
            // LOGW("cellLen:%d\n", iCellsLen);
            for (i = 0; i < iCellsLen; i ++)
            {
                printf("%x ", lpCells[i]);
            }
            printf("\n");
        }
        else
        {
            if(lpBits != NULL)
                ONVIF_FREE(lpBits);
            if(tempValue != NULL)
                ONVIF_FREE(tempValue);
            continue;
        }

        if(lpBits != NULL)
            ONVIF_FREE(lpBits);
        memset(strValue, 0, sizeof(strValue));

        for (i = 0; i < 18; i ++)
        {
            for (j = 0; j < 22; j ++)
            {
                iByteNum = (i * 22 + j) / 8;
                iBitNum = 7 - ((i * 22 + j) % 8);
                if((lpCells[iByteNum] >> iBitNum) & 1)
                {
                    tempValue[i * 22 + j] = 1;
                }
                else
                {
                    tempValue[i * 22 + j] = 0;
                }
            }
        }

        for (i = 0; i < 32; i ++)
        {
            for (j = 0; j < 32; j ++)
            {
                int x = (j * 22 + (32 >> 1)) / 32;
                int y = (i * 18 + (32 >> 1)) / 32;
                strValue[i * 32 + j] = tempValue[y * 22 + x] ? '1' : '0';
            }
        }
        memcpy(motion.rect, strValue, sizeof(strValue));
        ONVIF_FREE(lpCells);
        ONVIF_FREE(tempValue);
        break;
    }

    RestOnvif_SetMotionAttr(&motion);
    HttpRsp_ModifyRules(httpStr, len, xmlDetail);
    return 0;
}

static int UdsRspHandle(void *udsHandle, int conHandle,
                        ONVIF_XML_ACTION_T *xmlAction,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_AUTH_INFO_T *authResult,
                        HTTP_PARSER_RESULT_T *result)
{
    char *httpStr = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);
    /* LOGW("hasAuth %d isAuth %d\n", authResult->hasAuth, xmlAction->isAuth); */

    /*authEnable == 2 should enable adaptiveIp and disable password check ,
      when ipc using default password*/
    if (s_onvif_mgr_ct.cfg.authEnable == 0 ||
            (s_onvif_mgr_ct.cfg.authEnable == 2 && RestOnvif_IsDefaultPassword() == 1))
    {
        s_onvif_fun_map[xmlAction->actionId].actionFunc(httpStr, ONVIF_MAX_MSG_BUF_LEN,
                xmlDetail, result->contentStr);

        Utils_UdsServerSend(udsHandle, conHandle, httpStr, strlen(httpStr));
        DBGS("@%s@\n", httpStr);
        ONVIF_FREE(httpStr);

        if(xmlAction->actionId == ONVIF_XML_ACTION_SetNetworkInterfaces_ID)
        {
            LOGW("1|ONVIF_XML_ACTION_SetNetworkInterfaces_ID SetNetworkInterfaces !\n");
            OnvifAction_SetNetworkInterfaces(NULL, 0, xmlDetail, result->contentStr);
        }

        return 0;
    }

    if (xmlAction->isAuth == ONVIF_XML_ACTION_AUTH_REQ && authResult->hasAuth == 0)
    {
        LOGE("request auth\n");
        UdsRspHandle401(udsHandle, conHandle);
        Utils_UdsServerSend(udsHandle, conHandle, httpStr, strlen(httpStr));
        DBGS("@%s@\n", httpStr);
        ONVIF_FREE(httpStr);
        return -1;
    }

    if (xmlAction->isAuth == ONVIF_XML_ACTION_AUTH_REQ
            && RestOnvif_PasswordCheck(authResult) != 0)
    {
        LOGE("auth failed\n");
        UdsRspHandle400(udsHandle, conHandle);
        Utils_UdsServerSend(udsHandle, conHandle, httpStr, strlen(httpStr));
        DBGS("@%s@\n", httpStr);
        ONVIF_FREE(httpStr);
        return -1;
    }

    s_onvif_fun_map[xmlAction->actionId].actionFunc(httpStr, ONVIF_MAX_MSG_BUF_LEN,
            xmlDetail, result->contentStr);

    Utils_UdsServerSend(udsHandle, conHandle, httpStr, strlen(httpStr));
    DBGS("@%s@\n", httpStr);

    if(xmlAction->actionId == ONVIF_XML_ACTION_SetNetworkInterfaces_ID)
    {
        OnvifAction_SetNetworkInterfaces(NULL, 0, xmlDetail, result->contentStr);
    }

    ONVIF_FREE(httpStr);


    return 0;
}

static void FreeAuthResult(ONVIF_AUTH_INFO_T *authResult)
{
    /* if (authResult->hasAuth == 0) */
    /*     return; */

    if (authResult->userName)
        ONVIF_FREE(authResult->userName);
    if (authResult->password)
        ONVIF_FREE(authResult->password);
    if (authResult->digest.nonce)
        ONVIF_FREE(authResult->digest.nonce);
    if (authResult->digest.opaque)
        ONVIF_FREE(authResult->digest.opaque);
    if (authResult->digest.cnonce)
        ONVIF_FREE(authResult->digest.cnonce);
    if (authResult->digest.method)
        ONVIF_FREE(authResult->digest.method);
    if (authResult->digest.nc)
        ONVIF_FREE(authResult->digest.nc);
    if (authResult->digest.qop)
        ONVIF_FREE(authResult->digest.qop);
    if (authResult->digest.realm)
        ONVIF_FREE(authResult->digest.realm);
    if (authResult->digest.response)
        ONVIF_FREE(authResult->digest.response);
    if (authResult->digest.uri)
        ONVIF_FREE(authResult->digest.uri);

    if (authResult->nonce)
        ONVIF_FREE(authResult->nonce);
    if (authResult->created)
        ONVIF_FREE(authResult->created);
    if (authResult->passwordDigest)
        ONVIF_FREE(authResult->passwordDigest);

}


static void UdsRecvCb(void *udsHandle, int conHandle, char *recv, int len,
                      void *userData)
{
    HTTP_PARSER_RESULT_T result = {};

    if (userData == NULL)
    {
        LOGE("userData was null\n");
        return;
    }

    ONVIF_MGR_CONTEXT_T *ct = (ONVIF_MGR_CONTEXT_T *)userData;
    if (ct->state != ONVIF_MGR_STATE_START)
    {
        LOGW("state error:[%d]\n",ct->state);
        return;
    }

    //snapshot uri
    if(Common_StrnCmp(recv, "GET /onvif/GetSnapshot", 22) == 0)
    {
        OnvifAction_GetSnapShot(udsHandle, conHandle,recv, len);

        return;
    }

    ONVIF_AUTH_INFO_T authResult = {};

    HttpParser_Post(recv, len, &result, &authResult);
    if (result.contentLen <= 0)
    {
        LOGE("\n");
        FreeAuthResult(&authResult);
        return;
    }

    if (result.authStr != NULL)
        HttpParser_GetAuthResult(result.authStr, result.authLen, &authResult);

    ONVIF_XML_ACTION_T xmlAction = {};
    ONVIF_XML_ACTION_DETAIL_T xmlDetail = {};

    xmlDetail.httpResult = &result;

    int actionId = XmlParser_OnvifAction(result.contentStr, result.contentLen,
                                         &authResult, &xmlAction, &xmlDetail);

    DBGR("\nrecv\n@%s@\n", recv);

    if (actionId >= 0)
    {
        UdsRspHandle(udsHandle, conHandle, &xmlAction,
                     &xmlDetail, &authResult, &result);
    }
    else
    {
        DBGW("@%s@\n", recv);
        UdsRspHandle400(udsHandle, conHandle);
    }


    FreeAuthResult(&authResult);
    XmlParser_FreeXmlDetail(&xmlDetail);

}

int OnvifMgr_GetNetInfo(char *ifname, char *hwaddr, char *ipv4,
                        char *netmask)
{
    int ret = 0;
    if (ifname == NULL)
    {
        LOGW("ifname is null\n");
        return -1;
    }

    struct ifreq ifr;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        ret = -1;
    }
    else
    {
        strncpy(ifr.ifr_name, ifname, 16);
        if (hwaddr != NULL)
        {
            if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0)
            {
                snprintf(hwaddr, 32, "%02x%02x%02x%02x%02x%02x",
                         (unsigned char) ifr.ifr_hwaddr.sa_data[0],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[1],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[2],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[3],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[4],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[5]);
            }
            else
            {
                ret = errno;
                LOGE("ret=%d(%s)\n", ret, strerror(ret));
                ret = -1;
            }
        }

        if (ipv4 != NULL)
        {
            if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0)
            {
                struct sockaddr_in *myaddr;
                myaddr = (struct sockaddr_in *) &ifr.ifr_addr;
                memcpy(ipv4, inet_ntoa(myaddr->sin_addr), IPV4_ADDR_BUF_LEN);
            }
            else
            {
                ret = errno;
                LOGE("ret=%d(%s)\n", ret, strerror(ret));
                ret = -1;
            }
        }

        if (netmask != NULL)
        {
            if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0)
            {
                struct sockaddr_in *myaddr;
                myaddr = (struct sockaddr_in *) &ifr.ifr_netmask;
                memcpy(netmask, inet_ntoa(myaddr->sin_addr), IPV4_ADDR_BUF_LEN);
            }
            else
            {
                ret = errno;
                LOGE("ret=%d(%s)\n", ret, strerror(ret));
                ret = -1;
            }
        }
    }

    if (skfd >= 0)
    {
        close(skfd);
    }

    return ret;
}

int OnvifMgr_Init(MQ_HANDLE_H mqHandle)
{

    if (mqHandle == NULL || s_onvif_mgr_ct.state != ONVIF_MGR_STATE_UNINIT)
    {
        LOGE("mq handle was null or state error\n");
        return -1;
    }

    s_onvif_mgr_ct.mqHandle = mqHandle;
    s_onvif_mgr_ct.state = ONVIF_MGR_STATE_INIT;
    s_onvif_mgr_ct.basetime = Common_GetSystemCount64();
    return 0;
}

int OnvifMgr_Uninit()
{
    OnvifMgr_Stop();
    memset(&s_onvif_mgr_ct, 0, sizeof(s_onvif_mgr_ct));
    return 0;
}

int OnvifMgr_Start()
{
    int ret = 0;

    ret = OnvifMgr_GetCfg();
    if(ret != 0)
    {
        return ret;
    }

    s_onvif_mgr_ct.state = ONVIF_MGR_STATE_START;

    Common_DList_Init(&s_onvif_mgr_ct.subscribeList, OnvifEventNodeFree);
    Common_DList_Init(&s_onvif_mgr_ct.pullList, OnvifEventNodeFree);

    //Utils_UdpMultiServerStart((char *)ONVIF_MULTIADDR, ONVIF_MULTIPORT, 1000,
    //                          ONVIF_MSG_BUF_LEN, UdpMultiServerCallBack, &s_onvif_mgr_ct,
    //                          &s_onvif_mgr_ct.udpMultiHandle);

    //if(s_onvif_mgr_ct.cfg.fixedIp == 0 && s_onvif_mgr_ct.cfg.adaptiveIp == 1)
    {
        Common_Thread_T tCheckRunTime = NULL;
        Common_Thread_Create(&tCheckRunTime, __FUNCTION__, 0, 0, checkRunTimeThread, NULL);
        //Timer_Start(ONVIF_TIMER_FIXEDIP_CHECK, s_onvif_mgr_ct.cfg.timeout*1000);
    }


    mkdir("/var/run/onvif", 0);
    unlink("/var/run/onvif/unix");
    ret = Utils_UdsServerStart((char *)"/var/run/onvif/unix", 1000,
                               ONVIF_MAX_MSG_BUF_LEN, UdsRecvCb, &s_onvif_mgr_ct,
                               &s_onvif_mgr_ct.udsServerHandle);

    RestOnvif_RequestCoreVersion(&s_onvif_mgr_ct.coreVersion);

	s_onvif_mgr_ct.ability.ptzInfo.IsOfDome = s_onvif_mgr_ct.coreVersion.IsOfDome;
	s_onvif_mgr_ct.ability.ptzInfo.LensSupport = s_onvif_mgr_ct.coreVersion.LensSupport;
LOGD("Timer_Start\n");
//    Timer_Start(ONVIF_TIMER_EVENT_NOTIFY, 2000);
//    Timer_Start(ONVIF_TIMER_EVENT_CHECK, 10000);

    //Common_Thread_T tEventNotify = NULL;
    //Common_Thread_Create(&tEventNotify, __FUNCTION__, 0, 0, EventNotifyThread, NULL);

    Common_Thread_T tEventCheck = NULL;
    Common_Thread_Create(&tEventCheck, __FUNCTION__, 0, 0, EventCheckThread, NULL);


    int addptz = 0;
    char maca[32] = { 0};
    char ip[32] = { 0 };
    ONVIF_WEB_ATTR_T webAttr = {};
    LOGD("OnvifMgr_GetNetInfo\n");

    OnvifMgr_GetNetInfo("eth0", maca, ip, NULL);
    LOGD("RestOnvif_RequestWebServer[%s][%s]\n",ip,maca);
    RestOnvif_RequestWebServer(&webAttr);


    if((s_onvif_mgr_ct.ability.ptzInfo.IsOfDome > 0) || (s_onvif_mgr_ct.ability.ptzInfo.LensSupport== 1))
    {
        addptz = 1;
    }


    char req_hello[4096] = {0};
    LOGD("HttpReq_SendHello\n");

    HttpReq_SendHello(req_hello, sizeof(req_hello)-1, ip, maca, webAttr.httpPort, addptz);
    LOGD("Utils_SendtoMulticast\n");

    Utils_SendtoMulticast(req_hello, strlen(req_hello), "eth0", ip);

    return 0;
}

int OnvifMgr_Stop()
{
    s_onvif_mgr_ct.state = ONVIF_MGR_STATE_INIT;
    //Timer_Stop(ONVIF_TIMER_EVENT_NOTIFY);
    //Timer_Stop(ONVIF_TIMER_EVENT_CHECK);

    Common_DList_Uninit(&s_onvif_mgr_ct.subscribeList);
    Common_DList_Uninit(&s_onvif_mgr_ct.pullList);
    Utils_UdsServerStop(&s_onvif_mgr_ct.udsServerHandle);
    //Utils_UdpMultiServerStop(&s_onvif_mgr_ct.udpMultiHandle);
    return 0;
}

int OnvifMgr_Restart()
{
    OnvifMgr_Stop();
    OnvifMgr_Start();
    return 0;
}

int OnvifMgr_GetCfg()
{
    int ret = 0;
    LOGD("OnvifMgr_GetCfg!\n");
    if (Mq_Request(s_onvif_mgr_ct.mqHandle, ONVIF_REQ_GET_CFG_STRUCT, NULL, 0, &ret,
                       &s_onvif_mgr_ct.cfg, sizeof(s_onvif_mgr_ct.cfg)) < 0 || ret < 0)
    {
        LOGE("request cfg failed\n");
        return ret;
    }
    LOGD("authEnable %d adaptiveIp %d fixedIp %d!\n",s_onvif_mgr_ct.cfg.authEnable,s_onvif_mgr_ct.cfg.adaptiveIp,s_onvif_mgr_ct.cfg.fixedIp);
    if(s_onvif_mgr_ct.cfg.fixedIp == 0)
    {

        unsigned long long int time = Common_GetSystemCount64() - s_onvif_mgr_ct.basetime;
        unsigned long long int timeout = s_onvif_mgr_ct.cfg.timeout * 1000LLU;
        LOGD("time %lld timeout %lld!\n",time,timeout);
        if(time < timeout - 1)
        {
            fixedIp = 0;
        }
    }
    return 0;
}

int OnvifEvent_SendEvent(int isHappen)
{
    int ret = 0;
    ONVIF_SUBSCRIBE_NODE_T *node = NULL;
    COMMON_DLIST_T sendList;
    Common_DList_Init(&sendList, OnvifEventNodeFree);

    ONVIF_EVENT_DUMP_PARAM_T dumpParam = {sendList, isHappen};
    /*dump list*/
    Common_DList_Search(s_onvif_mgr_ct.subscribeList, &dumpParam , OnvifEventDump);

    char *buf = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);

    do
    {
        node = (ONVIF_SUBSCRIBE_NODE_T *)Common_DList_Detach(sendList, NULL, OnvifEventDetatch);
        if (node != NULL)
        {
            LOGW("send alarm [%d]\n",isHappen);
            ret = HttpRsp_PostMotionNotify(buf, ONVIF_MSG_BUF_LEN, node, isHappen);
            if (ret < 0)
            {
                LOGE("notify send to %s %d failed, remove node\n", node->ip, node->port);
                Common_DList_Delete(s_onvif_mgr_ct.subscribeList, &node->token, OnvifEventSearchByToken);
            }
            ONVIF_FREE(node);
        }
    }
    while(node != NULL);

    ONVIF_FREE(buf);
    Common_DList_Uninit(&sendList);

    return 0;
}


int OnvifMgr_EventNotify()
{
    int isHappen = 0;

    if (s_onvif_mgr_ct.subscribeWorking == 0)
        s_onvif_mgr_ct.subscribeWorking = 1;
    else
        return 0;

    if (Common_DList_GetCount(s_onvif_mgr_ct.subscribeList) <= 0)
    {
        s_onvif_mgr_ct.subscribeWorking = 0;
        return 0;
    }

    ONVIF_ALARM_STATUS_T alarmStatusInfo = { 0 };
    int ret = -1, i = 0;
    Mq_Request(s_onvif_mgr_ct.mqHandle, ONVIF_REQ_RTSP_GET_APP_DATA, NULL, 0, &ret,
               (void *)(&alarmStatusInfo),
               sizeof(ONVIF_ALARM_STATUS_T));

    if (ret != 0 || alarmStatusInfo.alarmStatusCnt <= 0)
    {
        LOGE("ret %d cnt %d\n", ret , alarmStatusInfo.alarmStatusCnt);
        s_onvif_mgr_ct.subscribeWorking = 0;
        return 0;
    }

    ret = -1;
    for (i = 0; i < alarmStatusInfo.alarmStatusCnt; i++)
    {
        ONVIF_ALARM_STATUS_NODE_T *node = alarmStatusInfo.alarmStatus + i;
        LOGW("alarm:[%s] status:[%d]\n",node->alarmName,node->status);
        if (strcmp(node->alarmName, "Motion") == 0/* && node->status == 1*/)
        {
            LOGW("alarm:[%s] status:[%d]\n",node->alarmName,node->status);
            isHappen = node->status;
            ret = 0;
            break;
        }
    }

    if(alarmStatusInfo.alarmStatus)
        ONVIF_FREE(alarmStatusInfo.alarmStatus);

    if (ret < 0)
    {
        s_onvif_mgr_ct.subscribeWorking = 0;
        return 0;
    }

    OnvifEvent_SendEvent(isHappen);
    s_onvif_mgr_ct.subscribeWorking = 0;
    return 0;
}

int OnvifMgr_EventCheck()
{
    unsigned long long int curTime = Common_GetSystemCount64();
    Common_DList_DeleteMulti(s_onvif_mgr_ct.subscribeList, &curTime, OnvifEventCheckTime);
    return 0;
}

int OnvifMgr_FixedIp()
{
    int ret = 0;

    s_onvif_mgr_ct.cfg.fixedIp = 1;
    if (Mq_Request(s_onvif_mgr_ct.mqHandle, ONVIF_REQ_SET_CFG_STRUCT, &s_onvif_mgr_ct.cfg, sizeof(s_onvif_mgr_ct.cfg),
                       &ret, NULL, 0) < 0
                || ret < 0)
    {
        LOGE("thread fixedIp failed\n");
    }

    LOGW("run time has been exceeded %d!\n",s_onvif_mgr_ct.cfg.timeout);

    //Timer_Stop(ONVIF_TIMER_FIXEDIP_CHECK);

    return 0;
}

static int OnvifAction_SetSynchronizationPoint(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    mxml_node_t *pRoot = (mxml_node_t *)xmlDetail->xmlParam;
    int devNo = 1, channelNo = 1, streamNo = 1;
    char *pStr = NULL;
    char tokenName[32] = {0};

    if (strlen(xmlDetail->actionNs) > 0)
        snprintf(tokenName, sizeof(tokenName), "%s:ProfileToken", xmlDetail->actionNs);
    else
        snprintf(tokenName, sizeof(tokenName), "ProfileToken");

    mxml_node_t *token = mxmlFindElement(pRoot, pRoot, tokenName,
                                         NULL, NULL,
                                         MXML_DESCEND_ALL);
    if (token != NULL && (pStr = (char *)mxmlGetText(token, NULL)) != NULL)
    {
        if(Common_StrnCmp(pStr, "CH01_third", 10) == 0)
    	{
            streamNo = 3;
        }
        else if(Common_StrnCmp(pStr, "CH01_sub", 7) == 0)
    	{
            streamNo = 2;
        }
        else if(Common_StrnCmp(pStr, "CH01", 4) == 0 && strlen(pStr) == 4)
    	{
            streamNo = 1;
        }
        else
        {
            sscanf(pStr, "%*[^_]_%d_%d_%d", &devNo, &channelNo, &streamNo);
        }

		LOGW("devNo=%d channelNo=%d streamNo=%d\n", devNo-1, channelNo-1, streamNo-1);

		RestOnvif_ReqIFrame(devNo-1, channelNo-1, streamNo-1);
    }

    HttpRsp_SetSynchronizationPoint(httpStr, len, xmlDetail);

	return 0;
}

static int OnvifAction_GetVideoSourceConfigurations(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    ONVIF_VI_ATTR_T viAttr;
    memset(&viAttr, 0, sizeof(ONVIF_VI_ATTR_T));
    RestOnvif_RequestViAttr(&viAttr);
    HttpRsp_GetVideoSourceConfigurations(httpStr, len, xmlDetail, &viAttr);

    return 0;
}

static int OnvifAction_GetNTP(char *httpStr, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                              char *contentStr)
{
    HttpRsp_GetNTP(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetDiscoveryMode(char *httpStr, int len,
                                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                        char *contentStr)
{
    HttpRsp_GetDiscoveryMode(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetNetworkDefaultGateway(char *httpStr, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        char *contentStr)
{
    ONVIF_NET_INFO_T pNetInfo = {0};
    //ONVIF_NETWORK_ATTR_T networkAttr;
    //memset(&networkAttr, 0, sizeof(ONVIF_NETWORK_ATTR_T));
    //RestOnvif_RequestNetworkAttr(&networkAttr);
    RestOnvif_RequesNetInfo(&pNetInfo);
    HttpRsp_GetNetworkDefaultGateway(httpStr, len, xmlDetail, &pNetInfo);
    return 0;
}

static int OnvifAction_GetHostname(char *httpStr, int len,
                                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                   char *contentStr)
{
    HttpRsp_GetHostname(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_SystemReboot(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    HttpRsp_SystemReboot(httpStr, len, xmlDetail);
    RestOnvif_RequestReboot();
    return 0;
}

static int OnvifAction_GetSupportedAnalyticsModules(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    HttpRsp_GetSupportedAnalyticsModules(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetSupportedRules(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    HttpRsp_GetSupportedRules(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_AddVideoEncoderConfiguration(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    HttpRsp_AddVideoEncoderConfiguration(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetGuaranteedNumberOfVideoEncoderInstances(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    HttpRsp_GetGuaranteedNumberOfVideoEncoderInstances(httpStr, len, xmlDetail);
    return 0;
}

static int OnvifAction_GetVideoEncoderInstances(char *httpStr, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                    char *contentStr)
{
    HttpRsp_GetVideoEncoderInstances(httpStr, len, xmlDetail);
    return 0;
}


