#ifndef OVFS_ONVIF_PARSER_H_
#define OVFS_ONVIF_PARSER_H_

#include "ovfs_onvif.h"


typedef struct
{
    int actionId;
    int isAuth;
    const char *actionName;
} ONVIF_XML_ACTION_T;

typedef struct
{
    char *xmlKey;
    char *xmlValue;
} ONVIF_XML_ACTION_DETAIL_NODE_T;

typedef struct
{
    char *uriStr;
    int uriLen;
    char *authStr;
    int authLen;
    char *contentStr;
    int contentLen;
    char *remoteAddrStr;
    int remoteAddrLen;
    char *hostAddrStr;
    int hostAddrLen;
    char *scheme;
    int schemeLen;
} HTTP_PARSER_RESULT_T;

typedef struct
{
    int actionId;
    const char *actionName;
    char actionNs[64];
    void *xmlRoot;
    void *xmlParam;
    int media2;
    HTTP_PARSER_RESULT_T *httpResult;
} ONVIF_XML_ACTION_DETAIL_T;

int HttpParser_Post(char *buf, int len, HTTP_PARSER_RESULT_T *result,
                    ONVIF_AUTH_INFO_T *authResult);

void HttpParser_GetAuthResult(char *authStr, int authLen,
                              ONVIF_AUTH_INFO_T *authResult);

void HttpRsp_Unauth401(char *buf, int len);

void HttpRsp_BadReq400(char *buf, int len);

void HttpRsp_DeviceInfo(char *buf, int len, char *manufacturer, char *model, char *firmwareVersion,
                        char *serialNumber, char *hardwareId,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_DateTime(char *buf, int len, ONVIF_TIME_ALL_T *timeAll,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *tzName, int tzOffset);

void HttpRsp_NetworkIf(char *buf, int len, ONVIF_NET_INFO_T *networkAttr,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_Capabilities(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                          ONVIF_WEB_ATTR_T *webAttr,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_Profiles(char *buf, int len,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_PROFILES_T *profiles,
                      char *chProfile);

void HttpRsp_Servcies(char *buf, int len,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                      ONVIF_WEB_ATTR_T *webAttr);

void HttpRsp_Scope(char *buf, int len,
                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *country, char *city, char *name);

void HttpRsp_SetScope(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *outName);

int XmlParser_SetScope(ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *outName);

void HttpRsp_Dns(char *buf, int len,
                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                 ONVIF_NETWORK_ATTR_T *networkAttr);

void HttpRsp_VideoSource(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                         ONVIF_VI_ATTR_T *viAttr);

void HttpRsp_StreamUri(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                       ONVIF_MultiCast_T *rtspAttr);

void HttpRsp_VideoSourceCfg(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                            ONVIF_VI_ATTR_T *viAttr);

void HttpRsp_GetOsds(char *buf, int len,
                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                     ONVIF_OSD_ATTR_T *osdAttr);

void HttpRsp_SetOsds(char *buf, int len,
                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_CreateOSD(char *buf, int len,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *fieldName);

void HttpRsp_DeleteOSD(char *buf, int len,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

int XmlParser_UdpDiscovery(char *buf, int len,
                           char *maca, char *ip, int httpPort,ONVIF_CAPABILITY_SET_T *ptzCapability);

int XmlParser_FreeXmlDetail(ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

int XmlParser_OnvifAction(char *buf, int len, ONVIF_AUTH_INFO_T *authResult,
                          ONVIF_XML_ACTION_T *xmlAction,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

int XmlParser_SetOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                          ONVIF_OSD_ATTR_T *osdAttr, char *contentStr);
int XmlParser_CreateOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                             ONVIF_OSD_ATTR_T *osdAttr, char *contentStr);
int XmlParser_DeleteOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                             ONVIF_OSD_ATTR_T *osdAttr);

void HttpRsp_ImageSet(char *buf, int len, ONVIF_IMAGE_SET_T *image,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_Options(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_SetImaging(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetVideoEncoderConfigurationOptions(char *buf, int len,
        ONVIF_VideoEncoderCapability_T *video,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_SetVideoEncoderConfiguration(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_SetNetworkInterfaces(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetOSDOptions(char *buf, int len,
                           ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetVideoEncoderConfiguration(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        int devNo, int channelNo, int streamNo, ONVIF_VideoEncoderCfg_T *pVideCfg);

void HttpRsp_GetVideoEncoderConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        ONVIF_VideoEncoderCfg_T *pVideCfg, int streamNo);
void HttpRsp_GetAudioEncoderConfiguration(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioEncoderConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetAudioEncoderConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetMoveOptions(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetVideoAnalyticsConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_MOTION_ATTR_T *pMotion);

void HttpRsp_SetVideoAnalyticsConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_SetSystemDateAndTime(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetAnalyticsModules(char *buf, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_MOTION_ATTR_T *pMotion);

void HttpRsp_ModifyAnalyticsModules(char *buf, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetRules(char *buf, int len,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_MOTION_ATTR_T *pMotion);

void HttpRsp_ModifyRules(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_Subscribe(char *buf, int len,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                       ONVIF_SUBSCRIBE_NODE_T *subNode, ONVIF_WEB_ATTR_T *webAttr);

int HttpRsp_PostMotionNotify(char *buf, int len,
                             ONVIF_SUBSCRIBE_NODE_T *subNode, int isHappen);

void HttpRsp_Renew(char *buf, int len,
                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_Unsubscribe(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetServiceCapability(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail);


int XmlParser_DateTimeParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                            ONVIF_TIME_ALL_T *timeAll,
                            char *tzName, int *tzOffset, char *tzDstName);

int XmlParser_Subscrib(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                       ONVIF_SUBSCRIBE_NODE_T *subNode);

void HttpRsp_GetEventProperties(char *buf, int len,
                                ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_CreatePullPointSubscription(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        ONVIF_SUBSCRIBE_NODE_T *subNode, ONVIF_WEB_ATTR_T *webAttr);

void HttpRsp_PullMessage(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                         ONVIF_SUBSCRIBE_NODE_T *subNode, int isHappen);

int XmlParser_PushAnalogInfo(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                             ONVIF_OSD_ATTR_T *osdAttr, ONVIF_TIME_ALL_T *timeAll,
                             ONVIF_GPS_EXT_INFO_T *gpsinfo,
                             char *contentStr);
int XmlParser_PushStationInfo(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                              ONVIF_OSD_ATTR_T *osdAttr, ONVIF_TIME_ALL_T *timeAll,
                              ONVIF_GPS_EXT_INFO_T *gpsinfo,
                              char *contentStr);
void HttpRsp_PushAnalogInfo(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_PushStationInfo(char *buf, int len,
                             ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioSource(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetAudioSourceConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioSourceConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetMetadataConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioOutputConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioOutputConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioDecoderConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_GetAudioDecoderConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetSnapshotUri(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *picUrl);
void HttpRsp_GetOsd(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                    ONVIF_OSD_ATTR_T *osdAttr);

void HttpRsp_GetNetworkProtocols(char *buf, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetConfigurations(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetConfiguration(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetConfigurationOptions(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetPresetTours(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetPresetTour(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetNode(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetNodes(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetPresets(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_SetPreset(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GotoPreset(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_DelPreset(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_ContinuousMove(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetStatus(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_AbsoluteMove(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_Move(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_Stop(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_HK_MaskOptions(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);


void HttpRsp_HK_PrivacyMask(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                        ONVIF_CAPABILITY_SET_T *ptzCapability);

void HttpRsp_GetVideoSourceConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        ONVIF_VI_ATTR_T *viAttr);

void HttpRsp_GetNTP(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetDiscoveryMode(char *buf, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetNetworkDefaultGateway(char *buf, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                      ONVIF_NET_INFO_T *networkAttr);

void HttpRsp_SetNetworkDefaultGateway(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetHostname(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_SetSynchronizationPoint(char *buf, int len,
                                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail);
void HttpRsp_SystemReboot(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpReq_SendHello(char *buf, int len, char *ip, char *mac, int http_port, int addptz);

void HttpRsp_GetSupportedAnalyticsModules(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_GetSupportedRules(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail);

void HttpRsp_AddVideoEncoderConfiguration(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail);


#endif
