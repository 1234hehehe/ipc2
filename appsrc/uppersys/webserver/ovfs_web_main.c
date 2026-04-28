#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ovfs_web_func.h"
#include "libaccess_api.h"
#include "libnetwork_sdk.h"
#include <dlfcn.h>

#define TIMER_SIG_TIMER          (SIGRTMIN+1)

THIRD_PROTOCOL_OPS_INFO_T s_libinfo[4] = {0};
int s_libinfo_count = 0;

extern int JsonOper_MergeObj(Common_cJSON_T* dst,Common_cJSON_T* src,int type);
extern int ovfs_web_discovery_init(COMMON_DLIST_T *sockList, int port);
extern int Fxn_Web_Discovery(Common_Thread_T hThreadHandle,void *pUserData);

extern void *OnvifUdpThread(void *data);
extern void *HikUdpThread(void *data);
extern void *Fxn_TST_Discovery();
extern void *AudioBroadcastThread(void *data);

OVFS_WEB_CONTEXT_T *g_ovfs_web = NULL;
cJSON_Struct *g_ovfs_config = NULL;
cJSON_Struct *g_ovfs_uiconfig = NULL;
//WEB_CUSTOM_UI_T g_ovfs_ui_custom;
AccessHandle_T g_AccessHandle = NULL;
ANTS_WEBSITESDK_CONFIG g_struWebSiteSDKInfo;
pthread_attr_t g_thread_attr;
/**********/
#include <dlfcn.h>
#include <signal.h>
#include <ucontext.h>

typedef struct
{
    int (*init)();
    int (*uninit)();
}MEDIASERVER_SDK_EXTERNAL_LIBS_T;


NETWORKSDK_EXTERNAL_LIBS_T s_networkfuncCb;
MEDIASERVER_SDK_EXTERNAL_LIBS_T s_mediaserverfuncCb;

static int web_register_to_core(AccessHandle_T *handler);
static int ovfs_web_init();
static void ovfs_web_uninit();

#if (OVFS_WEB_DISCOVERY == 1)
static Common_Thread_T s_ovfs_webDiscoveryThread = NULL;

static int ovfs_web_discovery()
{
    int iRet = 0;

    LOGW("SLINK Discovery!\n");
    iRet = ovfs_web_discovery_init(&g_ovfs_web->discovery.sockList, g_ovfs_web->discovery.port);
    if (0 == iRet)
    {
        iRet = Common_Thread_Create(&s_ovfs_webDiscoveryThread, __FUNCTION__, 0, 0, Fxn_Web_Discovery, NULL);
        if (iRet != 0)
        {
            LOGE("Fxn_Web_Discovery Failed:%s\n", strerror(iRet));
        }
    }

    return iRet;
}
#endif


cJSON_Struct * ovfs_web_parse_jsonfile(const char *filePath)
{
    int ret = 0;

    char * jsonStrBuffer = NULL;
    unsigned int jsonStrSize = 0;

    if (0 == ret)
    {
        struct stat statTmp;
        if (stat(filePath, &statTmp) != 0 || statTmp.st_size <= 0)
        {
            ret = -1;
            LOGW("No such file %s.\n", filePath);
        }
        else
        {
            jsonStrSize = statTmp.st_size;
        }
    }

    if (0 == ret)
    {
        if ((jsonStrBuffer = (char*)malloc(jsonStrSize)) == NULL)
        {
            ret = -1;
            LOGW("Malloc failed.\n");
        }
    }

    if (0 == ret)
    {
        int fd = -1;
        fd = open(filePath, O_RDONLY);
        if (fd < 0)
        {
            LOGW("open failed.\n");
            ret = -1;
        }
        else if (read(fd, jsonStrBuffer, jsonStrSize) <= 0)
        {
            LOGW("read failed.\n");
            ret = -1;
        }

        if (fd >= 0)
        {
            close(fd);
            fd = -1;
        }
    }
//LOGW("jsonStrBuffer[%d]:[%s]\n",jsonStrSize,jsonStrBuffer);
    cJSON_Struct *jsonUiconfig = NULL;
    if (0 == ret)
    {
        const char *end = NULL;
        const char *error = NULL;
        if ((jsonUiconfig = Common_Json_Parse(jsonStrBuffer, &end, &error)) == NULL)
        {
            ret = -1;
            LOGW("offset=%d error=%s\n", end - jsonStrBuffer, error);
        }
    }

    if (jsonStrBuffer)
    {
        free(jsonStrBuffer);
        jsonStrBuffer = NULL;
    }

    return jsonUiconfig;
}

static cJSON_Struct * ovfs_web_uiconfig_init()
{
    int ret = 0;

    cJSON_Struct *tempJson = NULL;
    if (0 == ret)
    {
        struct stat statTmp;
        if (stat(NGX_HTML"/static/uiconfig.json", &statTmp) != 0 || statTmp.st_size <= 0)
        {
            LOGW("Can not read uiconfig.json\n");

            if (stat(NGX_HTML"/static/uiconfig.json.bak", &statTmp) != 0 || statTmp.st_size <= 0)
            {
                ret = -1;
                LOGW("Can not read uiconfig.json.bak\n");
            }
            else
            {
                Common_System("cp "NGX_HTML"/static/uiconfig.json.bak "NGX_HTML"/static/uiconfig.json");
            }
        }
        else
        {
            if (stat(NGX_HTML"/static/uiconfig.json.bak", &statTmp) != 0)
            {
                Common_System("cp "NGX_HTML"/static/uiconfig.json "NGX_HTML"/static/uiconfig.json.bak");
            }
        }
    }

    if (0 == ret)
    {
        if ((tempJson = ovfs_web_parse_jsonfile(NGX_HTML"/static/uiconfig.json")) == NULL)
        {
            if ((tempJson = ovfs_web_parse_jsonfile(NGX_HTML"/static/uiconfig.json.bak")) == NULL)
            {
                ret = -1;
            }
            else
            {
                Common_System("cp "NGX_HTML"/static/uiconfig.json.bak "NGX_HTML"/static/uiconfig.json");
            }
        }
    }

    return tempJson;
}

int ovfs_web_uiconfig_save(cJSON_Struct *uiconfig)
{
    int ret = 0;

    int jsonStrSize = 0;
    char * jsonStrBuffer = NULL;
    if (0 == ret)
    {
        if ((jsonStrBuffer = Common_Json_Print(uiconfig, &jsonStrSize)) == NULL)
        {
            ret = -1;
            LOGW("Wrong\n");
        }
    }

    if (0 == ret)
    {
        //LOGW("jsonStrSize=%d\n", jsonStrSize);
        int fd = -1;
        if ((fd = open(NGX_HTML"/static/uiconfig.json", O_RDWR | O_TRUNC | O_CREAT, 00666)) < 0)
        {
            ret = -1;
        }
        else if (write(fd, jsonStrBuffer, (unsigned int)jsonStrSize) < 0)
        {
            ret = -1;
        }

        if (fd >= 0)
        {
            close(fd);
            fd = -1;
        }
    }

    if (jsonStrBuffer)
    {
        Common_Free(jsonStrBuffer, __FUNCTION__, __LINE__);
    }

    return ret;
}
int test_device_type(char* url, cJSON_Struct** out)
{
    cJSON_Struct* lowerData = NULL;

    int ret = 0;

    cJSON_Struct* pInParams = Common_Json_New ( NULL, Common_Json_Type_Object, NULL, 0, 0 );

    if ( !pInParams )
    {
        ret = WEB_CODE_LackingMem;
    }
    else
    {
        Common_Json_SetAttrValue ( pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0 );

        Common_Json_SetAttrValue ( pInParams, -1, "Header/Method", Common_Json_Type_String, "get", 0, 0 );
        Common_Json_SetAttrValue ( pInParams, -1, "Header/Uri", Common_Json_Type_String, url, 0, 0 );
    }

    if ( ret == 0 )
    {
        int TimeOut = 60000;
        ret = Module_CallFunctions ( ( ModuleHandle_T ) g_AccessHandle,pInParams,&lowerData,TimeOut );
        if ( lowerData )
        {
            //char* str = Common_cJSON_Print ( lowerData, NULL );
            //LOGD ( "str=%s\n\n",str );
            //wfree ( str );
            Common_Json_GetAttrValue ( lowerData, -1, "Header/Code", NULL, NULL, &ret, NULL );

            if ( out )
            {

                if ( ret == 0 )
                {
                    *out = Common_cJSON_DetachItemFromObject(lowerData, "Data");
                }
                else
                {
                    *out = NULL;
                }
            }
            Common_Json_Delete(lowerData);
            lowerData = NULL;

        }
    }

    if ( pInParams )
    {
        Common_Json_Delete ( pInParams );
        pInParams = NULL;
    }

    return ret;

}

static int init_onvif_discovery(fTHIRD_PROTOCOL_DealUdpPkg cb)
{
    LOGD("init_onvif_discovery!\n");

    ONVIF_UDP_CONTEXT_T *ct = calloc(1,sizeof(ONVIF_UDP_CONTEXT_T));

    snprintf(ct->addr, sizeof(ct->addr), "239.255.255.250");
    ct->port = 3702;
    ct->bufLen = 1024 * 4;
    ct->buf = calloc(1,ct->bufLen);
    ct->cb = cb;

    pthread_create(&ct->serverPth, NULL, OnvifUdpThread, ct);

    return 0;
}

static int init_hk_discovery(fTHIRD_PROTOCOL_DealUdpPkg cb)
{
	pthread_t hthread;

	pthread_create(&hthread, NULL, HikUdpThread, (void *)cb);

    return 0;
}

static int init_tst_discovery()
{
	pthread_t hthread;

	pthread_create(&hthread, NULL, Fxn_TST_Discovery, NULL);

    return 0;
}

static int AlarmDetail(cJSON_Struct *pAlarmInfo)
{
    char *alarmName = NULL,*devName = NULL,*startTime = NULL,*stopTime = NULL;

    Common_Json_GetAttrValue(pAlarmInfo, -1, "AlarmName", NULL, &alarmName, NULL, NULL);
    Common_Json_GetAttrValue(pAlarmInfo, -1, "StartTime", NULL, &startTime, NULL,  NULL);
    if(smatch(startTime, "00000000000000"))return 0;

    //only Motion/Vhide need send
    if(smatch(alarmName, "Motion") || smatch(alarmName, "Vhide"))
    {
        THIRD_ALARM_INFO alarminfo = {0};
        snprintf(alarminfo.AlarmName,sizeof(alarminfo.AlarmName),"%s",alarmName);

        Common_Json_GetAttrValue(pAlarmInfo, -1, "Channel", NULL, NULL, &alarminfo.Channel, NULL);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "Status", NULL, NULL, &alarminfo.Status, NULL);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "Stream", NULL, NULL, &alarminfo.Stream, NULL);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "Device", NULL, NULL, &alarminfo.Device, NULL);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "AlarmType", NULL, NULL, &alarminfo.AlarmType, NULL);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "AlarmSrcType", NULL, NULL, &alarminfo.AlarmSrcType, NULL);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "RegionId", NULL, NULL, &alarminfo.RegionId, NULL);

        Common_Json_GetAttrValue(pAlarmInfo, -1, "DevName", NULL, &devName, NULL, NULL);
        snprintf(alarminfo.DevName,sizeof(alarminfo.DevName),"%s",devName);

        snprintf(alarminfo.StartTime,sizeof(alarminfo.StartTime),"%s",startTime);
        Common_Json_GetAttrValue(pAlarmInfo, -1, "StopTime", NULL, &stopTime, NULL,  NULL);
        snprintf(alarminfo.StopTime,sizeof(alarminfo.StopTime),"%s",stopTime);

        for(int i = 0; i < s_libinfo_count; i++)
    	{
    		if(s_libinfo[i].fAlarmCallBack != NULL)
    		{
    			s_libinfo[i].fAlarmCallBack(&alarminfo, 1);
    		}
    	}
    }

    return 0;

}

static int RestAccessAlarmReport(AccessHandle_T hAccessHandle, int nSubscribeID,
                                 cJSON_Struct *pEventInfo,
                                 cJSON_Struct **pOutParams, void *pUserData)
{
    int arrySize = 0, i = 0;
    char *alarmName = NULL,startTime = NULL,stopTime = NULL;
    cJSON_Struct *pArry_root = NULL;
    if(pEventInfo == NULL)
    {
        LOGE("event info was null or user data was null\n");
        return 0;
    }

    pArry_root = Common_Json_GetAttrValue(pEventInfo, -1, "Data.ResList", NULL, NULL, NULL, NULL);
    if(pArry_root)
    {
        arrySize = Common_Json_Size(pArry_root);
        if (arrySize <= 0)
        {
            LOGE("array size error %d\n", arrySize);
            return 0;
        }

        for (i = 0; i < arrySize; i ++)
        {
            cJSON_Struct *loop = Common_Json_GetAttrValueArrItem(pArry_root, i);
            AlarmDetail(loop);
        }
    }
    else
    {
        AlarmDetail(pEventInfo);
    }

    return 0;
}

static int ResetKeyPress(AccessHandle_T hAccessHandle, int nSubscribeID,
                                 cJSON_Struct *pEventInfo,
                                 cJSON_Struct **pOutParams, void *pUserData)
{
    int iNeedRestore = 0;
    if(pEventInfo == NULL)
    {
        LOGE("event info was null or user data was null\n");
        return 0;
    }

    Common_Json_GetAttrValueInt(pEventInfo, "WifiNeedRestore", &iNeedRestore);
    LOGW("iNeedRestore:[%d]\n",iNeedRestore);

    if(iNeedRestore == 0)return 0;

    cJSON_Struct *header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValueObj(header, "Auth");
    Common_Json_SetAttrValueInt(header, "Auth/Method", 1);
    Common_Json_SetAttrValueStr(header, "Auth/UserName", "(null)");
    Common_Json_SetAttrValueStr(header, "Auth/Password", "ovfsZSJQZLHL");
    cJSON_Struct *indata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValueStr(indata, "RestoreType", "All");
    Common_Json_SetAttrValueInt(indata, "DelayTime", 5);
    OVFS_WEB_OPTION_S opt = {0};
    frmDeviceRestore(NULL, &opt, header, indata, NULL);

    Common_Json_Delete(header);
    header = NULL;
    Common_Json_Delete(indata);
    indata = NULL;
    return 0;
}

static int CallParamFun(void *pEncrypt, void *pInBuffer, int nInBufSize, void **pOutBuffer, int *nOutBufSize, int timeOut)
{
    int ret = 0;
    int cmd = 0;
    cJSON_Struct *header = NULL;

    if ((header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    Common_Json_SetAttrValueObj(header, "Auth");

    if(pEncrypt)
    {
        THIRD_AUTH_INFO *authinfo = (THIRD_AUTH_INFO *)pEncrypt;

        Common_Json_SetAttrValueInt(header, "Auth/Method", authinfo->AuthMethod);
        Common_Json_SetAttrValueStr(header, "Auth/UserName", authinfo->UserName);
        Common_Json_SetAttrValueStr(header, "Auth/Password", authinfo->Password);

    }
    else
    {
        Common_Json_SetAttrValueInt(header, "Auth/Method", 1);
        Common_Json_SetAttrValueStr(header, "Auth/UserName", "(null)");
        Common_Json_SetAttrValueStr(header, "Auth/Password", "ovfsZSJQZLHL");
    }

    cmd = *((int *)pInBuffer);
    LOGW("cmd:[0x%04x]\n",cmd);

    switch(cmd)
    {
        case THIRD_CMD_GET_DEVICE_INFO:
            ret = GetDeviceInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
		case THIRD_CMD_RESTORE:
			ret = DeviceRestore(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_REBOOT:
			ret = DeviceReboot(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_GET_MOTION_TIME:
			ret = GetMotionTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_SET_MOTION_TIME:
			ret = SetMotionTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_GET_VHIDE_TIME:
			ret = GetVHideTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_SET_VHIDE_TIME:
			ret = SetVHideTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_GET_MOTION_CFG:
			ret = GetMotionCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_SET_MOTION_CFG:
			ret = SetMotionCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_GET_VHIDE_CFG:
			ret = GetVHideCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_SET_VHIDE_CFG:
			ret = SetVHideCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_GET_MASK_RECT:
			ret = GetVideoMaskCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_SET_MASK_RECT:
			ret = SetVideoMaskCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
			break;
		case THIRD_CMD_GET_ALL_TIME:
            ret = GetAllTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_ALL_TIME:
            ret = SetAllTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_IMAGE:
            ret = GetImageCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_IMAGE:
            ret = SetImageCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_OSD_TIME:
            ret = GetOsdTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_OSD_TIME:
            ret = SetOsdTime(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_OSD_NAME:
            ret = GetOsdName(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,0);
            break;
        case THIRD_CMD_SET_OSD_NAME:
            ret = SetOsdName(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,0);
            break;
        case THIRD_CMD_GET_OSD_MULTI:
            ret = GetOsdName(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,1);
            break;
        case THIRD_CMD_SET_OSD_MULTI:
            ret = SetOsdName(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,1);
            break;
        case THIRD_CMD_FORCE_IFRAME:
            ret = ForceIFrame(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_HTTPPORT:
            ret = GetHttpInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_HTTPPORT:
            ret = SetHttpInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_RTSPPORT:
            ret = GetRtspInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_RTSPPORT:
            ret = SetRtspInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;

        case THIRD_CMD_GET_ETHCFG:
            ret = GetNetInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_ETHCFG:
            ret = SetNetInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_DNSCFG:
            ret = GetDnsInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_DNSCFG:
            ret = SetDnsInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_VIDEO_VENC_ABILITY:
            ret = GetVideoEncodeAbility(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_VIDEO_VENC:
            ret = GetVideoEncodeInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_VIDEO_VENC:
            ret = SetVideoEncodeInfo(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_AUDIO_PARAM:
            ret = GetAudioCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_AUDIO_PARAM:
            ret = SetAudioCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_DEVICE_NAME:
            ret = SetDeviceName(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_VIDEO_SOURCE:
            ret = GetVideoSource(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_GET_VIDEO_SNAPSHOT:
            ret = GetVideoSnapshot(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;

        case THIRD_CMD_GET_PRESET:
            ret = GetPreset(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_PRESET:
            ret = OperatePreset(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,1);
            break;
        case THIRD_CMD_DEL_PRESET:
            ret = OperatePreset(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,2);
            break;
        case THIRD_CMD_GOTO_PRESET:
            ret = OperatePreset(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1,3);
            break;
        case THIRD_CMD_PTZ_CONTROL:
            ret = PTZControl(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;

        case THIRD_CMD_GET_ONVIF_CFG:
            ret = GetOnvifCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_ONVIF_CFG:
            ret = SetOnvifCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;

        case THIRD_CMD_GET_ZOOM_CFG:
            ret = GetZoomCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;
        case THIRD_CMD_SET_ZOOM_CFG:
            ret = SetZoomCfg(header,pInBuffer,nInBufSize,pOutBuffer,nOutBufSize,1);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
			break;
    }

    if (header)
    {
        Common_Json_Delete(header);
        header = NULL;
    }

	return ret;
}

static int Webserver_CallFunctions(void *pInParams,void **pOutParams)
{
    char *pUrl = NULL;

    Common_Json_GetAttrValueStr(pInParams, "Header.Uri", &pUrl);
    if(Common_StrniCmp("/Network", pUrl, 8) == 0)
    {
        char strTmp[128] = {0};
        snprintf(strTmp, sizeof(strTmp), "/Webserver%s", pUrl);
        Common_Json_SetAttrValueStr(pInParams, "Header.Uri", strTmp);
    }

    return Module_CallFunctions((ModuleHandle_T)g_AccessHandle, (cJSON_Struct *)pInParams, (cJSON_Struct **)pOutParams, 3000);
}

char talk_buf[512] = {0};

static int StreamOpen(char *pUrl)
{
    int ret = 0;
    CoOpen_Param_T *param = (CoOpen_Param_T *)calloc(1, sizeof(CoOpen_Param_T) * 1 * 1);

    param[0].nIndex = 0;
    param[0].szUri = pUrl;
    param[0].nMode = 0;

    ret = Access_StreamQueue_CoOpen(g_AccessHandle, -1, param, 1, 1000);

    free(param);
    param = NULL;

    return ret;
}


static int StreamClose(int hStreamHandle)
{
    return Access_StreamQueue_Close(g_AccessHandle, hStreamHandle);
}


static int StreamRead(int hStreamHandle,char **pData,int *pDataSize)
{
    return Access_StreamQueue_ReadData(g_AccessHandle, hStreamHandle, -1, NULL, NULL, (void **)pData, pDataSize, 100);
}

static int StreamRelease(int hStreamHandle)
{
    return Access_StreamQueue_ReleaseData(g_AccessHandle, hStreamHandle);
}

int talk_stream_open()
{
    int ret = 0;
    CoOpen_Param_T *param = (CoOpen_Param_T *)calloc(1, sizeof(CoOpen_Param_T) * 2 * 1);
    char addr1[128] = {0};
    char addr2[128] = {0};

    snprintf(addr1,sizeof(addr1),"/BoardSys/Audio/Aenc/Channel0");

    param[0].nIndex = 0;
    param[0].szUri = addr1;
    param[0].nMode = 0;

    snprintf(addr2,sizeof(addr2),"/BoardSys/Speak/Adec/LiveStream/Channel0/Address");

    param[1].nIndex = 1;
    param[1].szUri = addr2;
    param[1].nMode = 1;

    ret = Access_StreamQueue_CoOpen(g_AccessHandle, -1, param, 2, 1000);

    free(param);
    param = NULL;

    return ret;
}

int talk_stream_close(int handle)
{
    StreamClose(handle);
    memset(talk_buf, 0, sizeof(talk_buf));
    return 0;
}

int talk_stream_release(int handle)
{
    return StreamRelease(handle);
}

int talk_stream_read(int handle,void **data, int *size)
{
    int ret = 0;

    ret = Access_StreamQueue_ReadData(g_AccessHandle, handle, -1, NULL, NULL, data, size, 1000);
    if(ret == 0)
    {
        Ovfs_FrameHeader_T* header = ( Ovfs_FrameHeader_T* ) *data;
        if( header->uiStartId != OVFS_FRAME_STARTCODE ||
	        header->uiFrameLen != *size - sizeof ( Ovfs_FrameHeader_T ) ||
	        header->uiFrameType != Ovfs_FrameType_AudioFrames)
    	{
    		//LOGE ( "invalid frame format!\n" );
    		talk_stream_release(handle);
    		return 1;
    	}
    }

    return ret;
}


int talk_stream_write(int handle,void *data, int size)
{
    int ret = 0;
    if(size + slen(talk_buf) >= 320)
    {
        char *oneFrame = calloc(1, 356);
        if(oneFrame)
        {
            int offset = 0;
            Ovfs_FrameHeader_T head = {0};
            memset ( &head, 0, sizeof ( Ovfs_FrameHeader_T ) );
            head.uiStartId = OVFS_FRAME_STARTCODE;
            head.uiFrameType = Ovfs_FrameType_AudioFrames;
            head.uiFrameLen = 320;
            //编码类型
            head.uMedia.struAudioHeader.cCodecId = Ovfs_VoiceCodecID_G711U;
            //采样率
            head.uMedia.struAudioHeader.cSampleRate = 8 ;
            //通道数
            head.uMedia.struAudioHeader.cChannels = 1;
            //比特率
            head.uMedia.struAudioHeader.cBitRate = 64;

            memcpy ( oneFrame, ( char* ) &head,sizeof(Ovfs_FrameHeader_T));

            while(1)
            {
                int len = slen(talk_buf);
                //LOGW("len:[%d] offset:[%d]\n",len,offset);
                if(len + (size - offset) >= 320)
                {

                    memcpy(talk_buf+len, (char*)(data+offset), 320-len);

                    offset += (320-len);

                    memcpy(oneFrame+sizeof(Ovfs_FrameHeader_T), talk_buf, 320);

                    ret = Access_StreamQueue_WriteData(g_AccessHandle, handle, 1, NULL, NULL, 0, oneFrame, 356);

                    memset(talk_buf, 0, sizeof(talk_buf));

                }
                else
                {
                    memcpy(talk_buf+len, (char*)(data+offset), size - offset);
                    break;
                }

            }

        }

        free(oneFrame);
        oneFrame = NULL;
    }
    else
    {
        memcpy(talk_buf+slen(talk_buf), data, size);
    }

    return 0;
}

static int LoadExternalNetworkLibs()
{
    void * libHdl  = NULL;

    if(Common_File_IsExist("/tmp/webserver_debug"))
    {
        libHdl = dlopen("/dev/libnetwork_sdk.so", RTLD_LAZY);
    }
    else
    {
        libHdl = dlopen("/root/lib/libnetwork_sdk.so", RTLD_LAZY);
    }

    if(libHdl)
    {
        s_networkfuncCb.init  = (NETWORKSDK_INIT_F )dlsym(libHdl, "libnetwork_sdk_init");
        if(s_networkfuncCb.init == NULL)
        {
            LOGE("Not found libnetwork_sdk_init\n");
            return -1;
        }

        s_networkfuncCb.uninit  = (NETWORKSDK_UNINIT_F )dlsym(libHdl, "libnetwork_sdk_uninit");
        if(s_networkfuncCb.uninit == NULL)
        {
            LOGE("Not found libnetwork_sdk_uninit\n");
            return -1;
        }

        s_networkfuncCb.config = (NETWORKSDK_UNINIT_F )dlsym(libHdl, "libnetwork_sdk_config");
        if(s_networkfuncCb.config == NULL)
        {
            LOGE("Not found libnetwork_sdk_config\n");
            return -1;
        }
    }
    else
    {
        LOGW("Not found libnetwork_sdk.so.\n");
        return -1;
    }

	LOGD("dlopen /root/lib/libnetwork_sdk.so succ\n");

    if(s_networkfuncCb.init)
	{
		LOGD("call libnetwork_sdk_init\n");
        NETWORKSDK_CALLBACK_T fxn;
        fxn.fxnCallFun = Webserver_CallFunctions;
        fxn.fxnStreamOpen = StreamOpen;
        fxn.fxnStreamClose = StreamClose;
        fxn.fxnStreamRead = StreamRead;
        fxn.fxnStreamRelease = StreamRelease;
		s_networkfuncCb.init(&fxn);
	}

    return 0;
}

static int LoadExternalMediaServerLibs()
{
    void * libHdl  = NULL;

    if(Common_File_IsExist("/tmp/webserver_debug"))
    {
        libHdl = dlopen("/dev/libmediaserver.so", RTLD_LAZY);
    }
    else
    {
        libHdl = dlopen("/root/lib/libmediaserver.so", RTLD_LAZY);
    }

    if(libHdl)
    {
        s_mediaserverfuncCb.init  = (NETWORKSDK_INIT_F )dlsym(libHdl, "lib_init");
        if(s_mediaserverfuncCb.init == NULL)
        {
            LOGE("Not found lib_init\n");
            return -1;
        }

        s_mediaserverfuncCb.uninit  = (NETWORKSDK_UNINIT_F )dlsym(libHdl, "lib_uninit");
        if(s_mediaserverfuncCb.uninit == NULL)
        {
            LOGE("Not found lib_uninit\n");
            return -1;
        }

        /*s_networkfuncCb.config = (EXTER_NETWORKSDK_UNINIT_F )dlsym(libHdl, "libnetwork_sdk_config");
        if(s_networkfuncCb.config == NULL)
        {
            LOGE("Not found libnetwork_sdk_config\n");
            return -1;
        }*/
    }
    else
    {
        LOGW("Not found libmediaserver.so.\n");
        return -1;
    }

	LOGD("dlopen /root/lib/libmediaserver.so succ\n");

    if(s_mediaserverfuncCb.init)
	{
		LOGD("call init\n");
		s_mediaserverfuncCb.init();
	}

    return 0;
}

static int ovfs_load_thirdprotocol_lib()
{
	char path[32] = {0};
    if(Common_File_IsExist("/tmp/webserver_debug"))
    {
        snprintf(path, sizeof(path),LIB_DEBUG_PATH);
    }
    else
    {
        snprintf(path, sizeof(path),LIB_PATH);
    }

    LOGW("ovfs_load_thirdprotocol_lib[%s]\n",path);

    DIR* soDir = opendir(path);
    if(soDir == NULL)
    {
        LOGE("opendir(%s) fail! errStr=%s\n", path, dlerror());
        return -1;
    }

    char pathbuff[128] = {0};
    struct dirent* dirInfo = NULL;
	while( (dirInfo = readdir(soDir)) != 0)
	{

        int iLoadOk = 0;
		if(strstr(dirInfo->d_name,"libhikvision.so") == NULL &&
            strstr(dirInfo->d_name,"libonvif.so") == NULL)
        {
            continue;
        }

		memset(pathbuff,0,sizeof(pathbuff));
        snprintf(pathbuff,sizeof(pathbuff),"%s/%s",path,dirInfo->d_name);
        void* dlHdl = dlopen(pathbuff,RTLD_NOW|RTLD_LOCAL);
        if(dlHdl == NULL)
        {
            LOGW("file=%s dlopen fail! strerr=%s\n",pathbuff,strerror(errno));
            continue;
        }
		else
		{

            s_libinfo[s_libinfo_count].fInit = (fTHIRD_PROTOCOL_Init)dlsym(dlHdl,"THIRD_PROTOCOL_Init");
            if(s_libinfo[s_libinfo_count].fInit == NULL)
            {
                LOGE("load THIRD_PROTOCOL_Init from file:%s fail! %s\n",dirInfo->d_name,dlerror());
                break;
            }

    		s_libinfo[s_libinfo_count].fUninit = (fTHIRD_PROTOCOL_Uninit)dlsym(dlHdl,"THIRD_PROTOCOL_Uninit");
            if(s_libinfo[s_libinfo_count].fUninit == NULL)
            {
                LOGE("load THIRD_PROTOCOL_Uninit from file:%s fail! %s\n",dirInfo->d_name,dlerror());
                break;
            }

    		s_libinfo[s_libinfo_count].fAlarmCallBack = (fTHIRD_PROTOCOL_AlarmCallBack)dlsym(dlHdl,"THIRD_PROTOCOL_AlarmCallBack");
            if(s_libinfo[s_libinfo_count].fAlarmCallBack == NULL)
            {
                LOGE("load THIRD_PROTOCOL_AlarmCallBack from file:%s fail! %s\n",dirInfo->d_name,dlerror());
                break;
            }

            s_libinfo[s_libinfo_count].fDealUdpPkg = (fTHIRD_PROTOCOL_DealUdpPkg)dlsym(dlHdl,"THIRD_PROTOCOL_DealUdpPkg");
            if(s_libinfo[s_libinfo_count].fDealUdpPkg == NULL)
            {
                LOGE("load THIRD_PROTOCOL_DealUdpPkg from file:%s fail! %s\n",dirInfo->d_name,dlerror());
                break;
            }

    		s_libinfo[s_libinfo_count].fSetConfig = (fTHIRD_PROTOCOL_SetConfig)dlsym(dlHdl,"THIRD_PROTOCOL_SetConfig");

            s_libinfo[s_libinfo_count].fSetTalkCallBack = (fTHIRD_PROTOCOL_SetTalkCallBack)dlsym(dlHdl,"THIRD_PROTOCOL_SetTalkCallBack");
            if(s_libinfo[s_libinfo_count].fSetTalkCallBack == NULL)
            {
                LOGE("load fSetTalkCallBack from file:%s fail! %s\n",dirInfo->d_name,dlerror());
                //break;
            }

            Common_Strncpy(s_libinfo[s_libinfo_count].libName, dirInfo->d_name, slen(dirInfo->d_name));
            LOGW("load lib[%s][%s] ok!\n",dirInfo->d_name,s_libinfo[s_libinfo_count].libName);
            if(strstr(dirInfo->d_name,"libhikvision.so"))
            {
                g_ovfs_web->support_HK = 1;
            }
            s_libinfo_count++;

    		iLoadOk = 1;

		}

		if(iLoadOk == 0)
        {
            dlclose(dlHdl);
            dlHdl = NULL;
        }
	}
	closedir(soDir);
	return 0;
}

int ovfs_tst_init(int status)
{
#if 0
    if(status)
    {
        init_tst_server();

        init_tst_discovery();
    }
    else
    {
        uninit_tst_server();
    }
#endif
    return 0;
}

int ovfs_hk_init(int status)
{
    int i = 0;
    for(i=0; i<s_libinfo_count; i++)
    {
        if(strstr(s_libinfo[i].libName,"libhikvision.so"))
        {
            if(status)
            {
                if(s_libinfo[i].fInit != NULL)
        		{
        			s_libinfo[i].fInit(CallParamFun);
        		}

                if(s_libinfo[i].fDealUdpPkg != NULL)
        		{
                    init_hk_discovery(s_libinfo[i].fDealUdpPkg);
                }

                if(s_libinfo[i].fSetTalkCallBack != NULL)
                {
                    TalkCallBack pfxn = {0};
                    pfxn.TalkStart = talk_stream_open;
                    pfxn.TalkStop = talk_stream_close;
                    pfxn.TalkInputData = talk_stream_write;
                    pfxn.TalkOutputData = talk_stream_read;
                    pfxn.TalkReleaseData = talk_stream_release;
                    LOGW("fSetTalkCallBack\n");
                    s_libinfo[i].fSetTalkCallBack(&pfxn);
                }
            }
            else
            {
                if(s_libinfo[i].fUninit != NULL)
        		{
        			s_libinfo[i].fUninit();
        		}
            }

            break;
        }
    }

    return 0;
}

int start_audio_broadcast()
{
    LOGD("start_audio_broadcast!\n");

    /*ONVIF_UDP_CONTEXT_T *ct = calloc(1,sizeof(ONVIF_UDP_CONTEXT_T));

    snprintf(ct->addr, sizeof(ct->addr), "239.255.255.250");
    ct->port = 3702;
    ct->bufLen = 1024 * 4;
    ct->buf = calloc(1,ct->bufLen);
    ct->cb = cb;

    TalkCallBack pfxn = {0};
    pfxn.TalkStart = talk_stream_open;
    pfxn.TalkStop = talk_stream_close;
    pfxn.TalkInputData = talk_stream_write;
    pfxn.TalkOutputData = talk_stream_read;
    pfxn.TalkReleaseData = talk_stream_release;*/

	pthread_t hthread;

    pthread_create(&hthread, NULL, AudioBroadcastThread, NULL);

    return 0;
}

void start_nginx()
{
    int https_enable = g_ovfs_web->enable_https;
    int https_port = g_ovfs_web->httpsport;

    //先启用https启动nginx,来判断是否支持https
    if(g_ovfs_web->enable_https == 0)
    {
        g_ovfs_web->enable_https = 1;
    }

    web_stop_nginx();
    generate_default_ngx_conf();
    // 用新的配置启动nginx、
    web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);

    Common_Sleep(2, 0);

    if(!check_nginx_start_ok())
    {
        LOGE("nginx is not start ok!regenerate default http only conf,then start it!\n");
        //close https
        g_ovfs_web->enable_http = 1;
        g_ovfs_web->enable_https = 0;
        g_ovfs_web->enbale_http_redirect_to_https = 0;
        g_ovfs_web->https_support = 0;

        web_stop_nginx();
        generate_default_ngx_conf();
        // ngx配置生效
        web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);

        Common_Json_SetAttrValueInt(g_ovfs_config, "EnableHttp", g_ovfs_web->enable_http);
        Common_Json_SetAttrValueInt(g_ovfs_config, "EnableHttps", g_ovfs_web->enable_https);
        Common_Json_SetAttrValueInt(g_ovfs_config, "EnableHttpRedirectToHttps", g_ovfs_web->enbale_http_redirect_to_https);
        Access_SaveConfig(g_AccessHandle, g_ovfs_config);
    }
    else
    {
        g_ovfs_web->https_support = 1;
        if(https_enable != g_ovfs_web->enable_https)
        {
            g_ovfs_web->enable_https = https_enable;

            web_stop_nginx();
            generate_default_ngx_conf();
            web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);
        }

    }

    LOGW("https_support:[%d]\n",g_ovfs_web->https_support);
}

void Timer_SignalMask()
{
    sigset_t bset;
    sigemptyset(&bset);
    sigaddset(&bset, TIMER_SIG_TIMER);
    pthread_sigmask(SIG_BLOCK, &bset, NULL);
}

/****************************************************  MAIN  *******************************************************/
int main(int argc, char **argv)
{
    int iRet = 0;

    LOG_INIT("Webserver", COMMON_LOG_LV_HIGH);

    Timer_SignalMask();

    //RegistSigHandle(SIGSEGV);
    //RegistSigHandle(SIGILL);
    //RegistSigHandle(SIGABRT);
    Common_RegistSigHandle(SIGSEGV);
    Common_RegistSigHandle(SIGILL);
    Common_RegistSigHandle(SIGABRT);

    LoadExternalMediaServerLibs();

    //web服务初始化
    iRet = ovfs_web_init();
    if (iRet != 0)
    {
        ovfs_web_uninit();
        OVFS_WEB_DIE("WebServer Init Failed!\n");
        return -1;
    }

    g_ovfs_web->fnInfo = Common_Json_New ( NULL, Common_Json_Type_Object, NULL, 0, 0 );
    g_ovfs_web->pDeviceStatus = Common_Json_New ( NULL, Common_Json_Type_Object, NULL, 0, 0 );

    if ( !g_ovfs_web->fnInfo || !g_ovfs_web->pDeviceStatus)
    {
        LOGE ( "Common_Json_New failed!No memory!!\n" );
        return -1;
    }

    cJSON_Struct *header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValueObj(header, "Auth");
    Common_Json_SetAttrValueInt(header, "Auth/Method", 1);
    Common_Json_SetAttrValueStr(header, "Auth/Username", "(null)");
    Common_Json_SetAttrValueStr(header, "Auth/Password", "ovfsZSJQZLHL");

    web_semantic_get_arming_status(header, NULL, NULL);
    ovfs_print_json(g_ovfs_web->pDeviceStatus);

    Common_Json_Delete(header);
    header = NULL;

    cJSON_Struct* smartinfo = Common_Json_SetAttrValueObj(g_ovfs_web->fnInfo, "SmartInfo");
    cJSON_Struct* list = Common_Json_SetAttrValueArr(smartinfo, "List" );

    cJSON_Struct* faceinfo = Common_Json_SetAttrValueObj ( g_ovfs_web->fnInfo, "Face" );

    struct stat tmpStat;
    int fd = -1;
    if ( stat ( "/tmp/mmc/mmc1/person_record.db", &tmpStat ) != 0 || ( fd = open ( "/tmp/mmc/mmc1/person_record.db", O_RDWR ) ) < 0 )
    {
        Common_Json_SetAttrValueInt(g_ovfs_web->fnInfo, "PersonRecord",0);
    }
    else
    {
        Common_Json_SetAttrValueInt(g_ovfs_web->fnInfo, "PersonRecord",1);
    }

    if (0 == iRet)
    {
        iRet = Ovfs_Fastcgi_Start(&g_struWebSiteSDKInfo);
    }
    if (iRet != 0)
    {
        ovfs_web_uninit();
        OVFS_WEB_DIE("WebServer Start Failed!\n");
        return -1;
    }

    start_nginx();

#if (OVFS_WEB_DISCOVERY == 1)
    // WebServer Discovery Service (I8H, SLINK)
    iRet = ovfs_web_discovery();
#endif

	ovfs_load_thirdprotocol_lib();

	for(int i = 0; i < s_libinfo_count; i++)
	{

        if(Common_StriCmp(s_libinfo[i].libName, "libhikvision.so") == 0)
        {
            if(g_ovfs_web->support_HK && g_ovfs_web->enable_HK)
            {
                ovfs_hk_init(1);
            }
        }
        else
        {
            if(s_libinfo[i].fInit != NULL)
            {
                s_libinfo[i].fInit(CallParamFun);
            }

            if(Common_StriCmp(s_libinfo[i].libName, "libonvif.so") == 0)
            {
                init_onvif_discovery(s_libinfo[i].fDealUdpPkg);
            }
        }
	}

    if(g_ovfs_web->enable_TST)
    {
        ovfs_tst_init(1);
    }

    int ackSmartAbility = 0;
    int ackCoreFactoryInfo = 0;
    int trycount = 3;
    while(1)
    {
        Common_Sleep(1,0);
        //LOGW("webserver\n");

        if (ackSmartAbility == 0)
        {
            //int thirdStreamSupport = 0;
            //int roiSupport = 0;

            int isDetectFace = 0;
            int isRecogFace = 0;

            cJSON_Struct *pResult = NULL;

            int ret;

            ret = test_device_type("/SmartServer/Ability", &pResult);

            trycount--;

            if (0 == ret)
            {

                cJSON_Struct* AbilityList = Common_Json_GetAttrValueArr ( pResult, "AbilityList" );
                cJSON_Struct* AbilityVersionList = Common_Json_GetAttrValueArr ( pResult, "AbilityVersionList" );

                int ability_size = Common_Json_ArraySize ( AbilityList );

                if(ability_size > 0)
                {
                    ackSmartAbility = 1;
                }

                int i = 0;
                int i_num = 0;
                char *str_tmp = NULL;

                for(i=0; i<ability_size; i++)
                {
                    Common_Json_GetAttrValue(AbilityList, i, NULL, NULL, &str_tmp, NULL, NULL);
                    Common_Json_GetAttrValue(AbilityVersionList, i, NULL, NULL, NULL, &i_num, NULL);
                    cJSON_Struct *out_loop = Common_Json_SetAttrValueArrObj(list, i);
                    Common_Json_SetAttrValueInt(out_loop, str_tmp, i_num);
                    if ( smatch ( "RecognitionFace",str_tmp ) )
                    {
                        isRecogFace = 1;
                    }
                    if ( smatch ( "DetectFace",str_tmp ) )
                    {
                        isDetectFace = 1;
                    }
                }

            }


            Common_Json_SetAttrValueInt ( faceinfo, "EnableFaceTest",isDetectFace );
            Common_Json_SetAttrValueInt ( faceinfo, "EnableFaceSnap",isDetectFace );
            Common_Json_SetAttrValueInt ( faceinfo, "EnableFaceCompare",isRecogFace );

            if(pResult)
            {
                Common_Json_Delete(pResult);
                pResult = NULL;
            }

            if(trycount == 0)
            {
                ackSmartAbility = 1;
            }

        }

        if (ackCoreFactoryInfo == 0)
        {
            int ret;
            char *str_tmp = NULL;
            cJSON_Struct *pResult = NULL;
            cJSON_Struct *header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (header == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
            else
            {
                Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
            }

            ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
            if (0 == ret)
            {
                int valueInt = 0;
                ackCoreFactoryInfo = 1;
                memset(g_ovfs_web->DeviceTypeString, 0, sizeof(g_ovfs_web->DeviceTypeString));
                if (Common_Json_GetAttrValueInt(pResult, "IsOfDome", &valueInt))
                {
                    g_ovfs_web->devInfo.bPTZ = valueInt;
                    if(valueInt == 1)
                    {
                        scopy(g_ovfs_web->DeviceTypeString,32,"ipc_dome");
                    }
                    else if ( valueInt == 2 )
                    {
                        scopy(g_ovfs_web->DeviceTypeString,32,"ipc_netdome");
                    }
                    else
                    {
                        scopy(g_ovfs_web->DeviceTypeString,32,"ipc_normal");
                    }
                }

                if(Common_Json_GetAttrValueStr(pResult, "DeviceTypeString", &str_tmp))
                {

                    memset(g_ovfs_web->DeviceTypeString, 0, sizeof(g_ovfs_web->DeviceTypeString));
                    if(slen(str_tmp))
                    {
                        scopy(g_ovfs_web->DeviceTypeString,32,str_tmp);
                    }
                    else
                    {
                        scopy(g_ovfs_web->DeviceTypeString,32,"ipc_normal");
                    }
                }
            }

            Common_Json_Delete(pResult);
            pResult = NULL;

            Common_Json_Delete(header);
            header = NULL;
        }
    }

	if(s_networkfuncCb.uninit)
	{
		LOGD("call libnetwork_sdk_uninit\n");
		s_networkfuncCb.uninit();
	}

	for(int i = 0; i < s_libinfo_count; i++)
	{
		if(s_libinfo[i].fUninit != NULL)
		{
			s_libinfo[i].fUninit(CallParamFun);
		}
	}

	ovfs_web_uninit();

    return iRet;
}

/******************************************************************************************************************/

static int Web_AccessCallFunction(AccessHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
    WEB_REST_FUNC_CALL_IN_T request;
    WEB_REST_FUNC_CALL_OUT_T response;

    memset(&response, 0, sizeof(WEB_REST_FUNC_CALL_OUT_T));
    if (pInParams != NULL)
    {
        Common_Json_GetAttrValue(pInParams, -1, "Header.Method", NULL, &request.method, NULL, NULL);
        Common_Json_GetAttrValue(pInParams, -1, "Header.Uri", NULL, &request.url, NULL, NULL);
        request.Data = Common_Json_GetAttrValue(pInParams, -1, "Data", NULL, NULL, NULL, NULL);
    }
    //LOGW("CallFunction!! uri:[%s]\n",request.url);

    if(Common_StrniCmp("/Webserver/Network", request.url, 18) == 0 && s_networkfuncCb.config != NULL)
    {
        //LOGW("s_networkfuncCb.config!\n");
        char uri[128] = {0};
        snprintf(uri, sizeof(uri), "%s", request.url+10);
        Common_Json_SetAttrValueStr(pInParams, "Header.Uri", uri);
        s_networkfuncCb.config((void *)pInParams, (void **)pOutParams);
    }
    else
    {
        ovfs_web_rest_dispatch(&request, &response);
        *pOutParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (*pOutParams)
        {
            Common_Json_SetAttrValue(*pOutParams, -1, "Header", Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(*pOutParams, -1, "Header.Code", Common_Json_Type_Number, NULL, response.codeNum, 0);
            if (response.Data)
            {
                Common_Json_AddItem(*pOutParams, -1, "Data", response.Data);
            }
        }
    }
    return 0;
}

static S32 Web_AccessEventSubscribe(AccessHandle_T hAccessHandle,S32 nType, S32 nRecvID, S8 *szSubscribeUri, cJSON_Struct **pQueryEventInfo, void *pUserData)
{
    WEB_SUBSCRIBE_NODE_T *data_tmp = NULL;

    if (szSubscribeUri == NULL || hAccessHandle == NULL || pUserData == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }
/*
    LOGD("nRecvID:%d\n", nRecvID);
    LOGW("==============================================\n");
    LOGW("Uri:%s\n", szSubscribeUri);
    LOGW("==============================================\n");
*/
    switch (nType)
    {
    case 0: /*subscribe*/
    {
        LOGW("subscribe!\n");
        data_tmp = Common_Malloc(sizeof(WEB_SUBSCRIBE_NODE_T), 0, __FUNCTION__, __LINE__);
        data_tmp->uri = Common_StrDup(szSubscribeUri, __FUNCTION__, __LINE__);
        data_tmp->bFirst = 1;
        data_tmp->toId = nRecvID;
        Common_DList_InsertTail(g_ovfs_web->subscribeList, (void *)data_tmp, sizeof(WEB_SUBSCRIBE_NODE_T));
    }
    break;
    case 1: /*unsubscribe*/
    {
        LOGW("unsubscribe!\n");
        Common_DList_Delete(g_ovfs_web->subscribeList, (void *)&nRecvID, web_subscribe_nodecompare);
    }
    break;
    case 2: /*query*/
    {
        //LOGW("query!\n");
        //*pQueryEventInfo = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        //Common_Json_SetAttrValue(*pQueryEventInfo, -1, "Http", Common_Json_Type_Number, NULL, g_ovfs_web->httpport, 0);
        //Common_Json_SetAttrValue(*pQueryEventInfo, -1, "Https", Common_Json_Type_Number, NULL, g_ovfs_web->httpsport, 0);
    }
    break;
    default:
    {
        LOGE("subscribe err!\n");
    }
    break;
    }

    return 0;
}

static int web_register_to_core(AccessHandle_T *handler)
{
    int iRet = 0;
    int count = 5;
    cJSON_Struct *pConfig = NULL;

    pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig, -1, "SystemName", Common_Json_Type_String, "ovfs", 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "ModuleName", Common_Json_Type_String, "Webserver", 0, 0);
    }
    do
    {
        iRet = Access_Init(handler, pConfig, NULL, Web_AccessCallFunction, (void *)g_ovfs_web);
        if (0 == iRet)
        {
            LOGD("Web Register to Access (%d)\n", iRet);
            break;
        }
        LOGW("\n===================================Web Try Register to core [%d/5]!\n", 5 - count);
        Common_Sleep(5, 0);
    }
    while(count --);
    Common_Json_Delete(pConfig);

    if (0 == iRet)
    {
        Access_RegisterSubscribe(*handler, "/Webserver/Subscribe/Wifi", Web_AccessEventSubscribe, (void *)g_ovfs_web);
        Access_RegisterSubscribe(*handler, "/Webserver/Subscribe/DeviceStatus", Web_AccessEventSubscribe, (void *)g_ovfs_web);
        Access_SubscribeEvent(*handler, (char *)"/Alarm/Subscribe/Status", NULL,NULL,RestAccessAlarmReport, (void *)g_ovfs_web);
        Access_SubscribeEvent(*handler, (char *)"/BoardSys/Subscribe/WifiResetKey/KeyPress", NULL,NULL,ResetKeyPress, (void *)g_ovfs_web);
    }

    return iRet;
}

static const char *s_webserverDefaultCfg = "{\"EnableHttpRedirectToHttps\":0,\"EnableHttp\":1,\"HttpPort\":80,\"EnableHttps\":1, \"HttpsPort\":443, \"PlatFormReady\":1,\"SessionSupport\":0,\"SingleAccountLogin\":0,\"PasswordTips\":1}";

int check_nginx_start_ok()
{
    char cmdBuffer[128] = {0};
    snprintf(cmdBuffer, sizeof(cmdBuffer),
        "netstat -anp | grep nginx > /dev/ngx_start_status");
    Common_System(cmdBuffer);

    FILE* fp = fopen("/dev/ngx_start_status","r");
    char buf[128]= {0};

    if(fp)
    {
        fread(buf,sizeof(buf),1,fp);
        fclose(fp);
    }

    //return sncmp(buf,"ok",2) == 0;
    return slen(buf);
}
int check_asdpd_start_ok()
{
    Common_System("rm /dev/test_asdpd");

    Common_System("netstat -anp | grep 10000 > /dev/test_asdpd");


    FILE* fp = fopen("/dev/test_asdpd","r");
    char buf[128]= {0};

    if(fp)
    {
        fread(buf,sizeof(buf),1,fp);
        fclose(fp);
    }
    return slen(buf) > 0;
}




static int ovfs_web_init()
{
    int iRet = 0;

    g_ovfs_web = ovfs_web_new();
#ifdef WEB_PRINT_DEBUG
    web_set_test_logfile(g_ovfs_web, "/mnt/out/test.log");
#endif
    WEB_DBGFUN("ovfs_web_init");

    // thread_attr
    pthread_attr_init(&g_thread_attr);
    pthread_attr_setstacksize(&g_thread_attr, IPC_THREAD_STACK_MIN);
    pthread_attr_setdetachstate(&g_thread_attr, PTHREAD_CREATE_DETACHED);

    snprintf(g_ovfs_web->updatePath, sizeof(g_ovfs_web->updatePath), "/dev/ipc.update");
    //创建Nonce List
    Common_DList_Init(&g_ovfs_web->pNonceList, web_nonce_nodefree);
    Common_DList_Init(&g_ovfs_web->subscribeList, web_subscribe_nodefree);
    Common_DList_Init(&g_ovfs_web->loginFailedList, web_loginfailed_nodefree);
    Common_DList_Init(&g_ovfs_web->userLoginList, web_loginsuccess_nodefree);
    Common_DList_Init(&g_ovfs_web->devInfo.DiskList, web_disk_nodefree);
    Common_DList_Init(&g_ovfs_web->devInfo.DiskFormatList, web_diskformat_nodefree);

    MUTEX_SETUP(g_ovfs_web->hLoginFailedLock);
    MUTEX_SETUP(g_ovfs_web->hReqSessionLock);
    MUTEX_SETUP(g_ovfs_web->hReqPtzCruiseCallLock);
    MUTEX_SETUP(g_ovfs_web->hReqPtzTackCallLock);

    iRet = web_register_to_core(&g_AccessHandle);

	LoadExternalNetworkLibs();

    //Web配置获取
    if (0 == iRet)
    {
        // 使用硬编码方式初始化配置参数.
        Common_cJSON_T *codeDefaultCfg = NULL;
        codeDefaultCfg = Common_cJSON_Parse(s_webserverDefaultCfg, NULL, NULL);
        if (codeDefaultCfg)
        {
            g_ovfs_config = codeDefaultCfg;
        }

        // 读取默认配置文件,并将其中参数覆盖之前读取的参数.
        cJSON_Struct *fileDefaultCfg = NULL;
        Access_LoadConfigByType(g_AccessHandle,Access_ConfigType_Default,&fileDefaultCfg);
        LOGD("Access_LoadConfigByType:%p\n",fileDefaultCfg);
        if (fileDefaultCfg)
        {
            JsonOper_MergeObj(g_ovfs_config, (Common_cJSON_T*)fileDefaultCfg, 0);
            Common_Json_Delete(fileDefaultCfg);
            fileDefaultCfg = NULL;
        }
        else
        {

        }

        // 读取当前配置文件,并将其中参数覆盖之前读取的参数.
        cJSON_Struct *fileCurrentCfg = NULL;
        Access_LoadConfig(g_AccessHandle, &fileCurrentCfg);
        LOGD("Access_LoadConfig:%p\n",fileCurrentCfg);
        if (fileCurrentCfg)
        {
            JsonOper_MergeObj(g_ovfs_config, (Common_cJSON_T*)fileCurrentCfg, 0);
            Common_Json_Delete(fileCurrentCfg);
            fileCurrentCfg = NULL;
        }

        if(Common_File_IsExist("/proc/net/if_inet6"))
        {
            g_ovfs_web->support_ipv6 = 1;
        }

        if (g_ovfs_config)
        {
            //T8S enable
            int enable_T8S = 0;
            Common_Json_GetAttrValueInt(g_ovfs_config, "EnableT8S", &enable_T8S);
            g_ovfs_web->enable_T8S = enable_T8S;

            int enable_TST = 1;
            Common_Json_GetAttrValueInt(g_ovfs_config, "ThirdPartyProtocols/TST",&enable_TST);
            g_ovfs_web->enable_TST = enable_TST;

            int enable_HK = 1;
            Common_Json_GetAttrValueInt(g_ovfs_config, "ThirdPartyProtocols/HK",&enable_HK);
            g_ovfs_web->enable_HK = enable_HK;
            LOGW("enable_TST:[%d] enable_HK:[%d]\n",enable_TST,enable_HK);

            if(Common_Json_GetAttrValueObj(g_ovfs_config, "ThirdPartyProtocols") == NULL)
            {
                Common_Json_SetAttrValueObj(g_ovfs_config, "ThirdPartyProtocols");
            }

            int session_timeout = 0;
            Common_Json_GetAttrValueInt(g_ovfs_config, "SessionTimeOut", &session_timeout);
            if(session_timeout < 60)session_timeout = 60;
            g_ovfs_web->session_timeout = session_timeout;


            Common_Json_GetAttrValueStr(g_ovfs_config, "FacePicFormat",&g_ovfs_web->face_picture_format);

            //int AutoLens_Preset= 0;
            // Common_Json_GetAttrValueInt(g_ovfs_config, "AutoLensPreset", &AutoLens_Preset);
            // g_ovfs_web->AutoLensPreset = AutoLens_Preset;



            int discovery_port = 10001;
            //如果用户定制了discovery_port
            if(Common_Json_GetAttrValueInt(g_ovfs_config, "discovery_port",&discovery_port))
            {
                g_ovfs_web->discovery.port = discovery_port;

            }
            else
            {

                if(!check_asdpd_start_ok())
                {
                    g_ovfs_web->discovery.port = 10000;
                }
                else
                {
                    g_ovfs_web->discovery.port = 10001;

                }

            }

            int support_new_upgrade = 1;
            Common_Json_GetAttrValueInt(g_ovfs_config, "SupportNewUpgrade",&support_new_upgrade);
            g_ovfs_web->discovery.support_new_upgrade = support_new_upgrade;

            //get plugin params
            if(!Common_Json_GetAttrValueObj(g_ovfs_config, "PluginParams"))
            {
                Common_Json_SetAttrValue(g_ovfs_config, -1,"PluginParams", Common_Json_Type_Object,NULL,0,0);
            }

            Common_Json_GetAttrValueInt(g_ovfs_config, "PluginParams.WndMode",  (S32*)&g_ovfs_web->plugin_params.wm);

            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.PrevCapture",  &g_ovfs_web->plugin_params.prev_capture_path);
            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.PbCapture",  &g_ovfs_web->plugin_params.pb_capture_path);
            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.FileCapture",  &g_ovfs_web->plugin_params.file_capture_path);
            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.BackupPath",  &g_ovfs_web->plugin_params.backup_path);
            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.RecPath",  &g_ovfs_web->plugin_params.rec_path);
            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.PlatePics",  &g_ovfs_web->plugin_params.plate_pics);
            Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.FacePics",  &g_ovfs_web->plugin_params.face_pics);

            Common_Json_GetAttrValueInt(g_ovfs_config, "PluginParams.RecFormat",	(S32*)&g_ovfs_web->plugin_params.rec_file_format);
            Common_Json_GetAttrValueInt(g_ovfs_config, "PluginParams.PicQuality",	&g_ovfs_web->plugin_params.prev_buf_val);

            //get http https conf
            Common_Json_GetAttrValue(g_ovfs_config, -1, "HttpPort", NULL, NULL, &g_ovfs_web->httpport, NULL);
            Common_Json_GetAttrValue(g_ovfs_config, -1, "EnableHttp", NULL, NULL, &g_ovfs_web->enable_http, NULL);

            Common_Json_GetAttrValue(g_ovfs_config, -1, "HttpsPort", NULL, NULL, &g_ovfs_web->httpsport, NULL);
            Common_Json_GetAttrValue(g_ovfs_config, -1, "EnableHttps", NULL, NULL, &g_ovfs_web->enable_https, NULL);

            Common_Json_GetAttrValue(g_ovfs_config, -1, "EnableHttpRedirectToHttps", NULL, NULL, &g_ovfs_web->enbale_http_redirect_to_https, NULL);

            if(Common_Json_GetAttrValueInt(g_ovfs_config, "PasswordTips", &g_ovfs_web->password_tips))
            {
            }
            else
            {
                g_ovfs_web->password_tips = 1;
            }
            LOGD("password_tips:%d\n",g_ovfs_web->password_tips);

            if(g_ovfs_web->enable_https == 0 && g_ovfs_web->enable_http == 0)
            {
                LOGE("Can't disable http and https both!! will setup http service by default!\n");
                g_ovfs_web->enable_http = 1;
                g_ovfs_web->enbale_http_redirect_to_https = 0;
            }

            if(g_ovfs_web->enable_https == 0)
            {
                g_ovfs_web->enbale_http_redirect_to_https = 0;
            }
            else
            {
                //获取用户自定义配置的ssl_ciphers protocols
                Common_Json_GetAttrValue(g_ovfs_config, -1, "Protocols", NULL,  &g_ovfs_web->custom_support_protocols,NULL, NULL);
                Common_Json_GetAttrValue(g_ovfs_config, -1, "Ciphers", NULL, &g_ovfs_web->custom_ciphers,NULL, NULL);
            }

            Common_Json_GetAttrValue(g_ovfs_config, -1, "SingleAccountLogin", NULL, NULL, &g_ovfs_web->enable_single_account_login_mode, NULL);
            Common_Json_GetAttrValue(g_ovfs_config, -1, "SessionSupport", NULL, NULL, &g_ovfs_web->enable_session, NULL);
            Common_Json_GetAttrValue(g_ovfs_config, -1, "SessionCount", NULL, NULL, &g_ovfs_web->session_count, NULL);

            if(g_ovfs_web->enable_session == 0) //限制单用户，必须开启session、支持
            {
                g_ovfs_web->enable_single_account_login_mode = 0;
            }

            if(g_ovfs_web->session_count == 0)
            {
                g_ovfs_web->session_count == 5;
            }
            Common_Json_GetAttrValueInt(g_ovfs_config, "MaxTryCount",&g_ovfs_web->maxTryCount);
            Common_Json_GetAttrValueInt(g_ovfs_config, "MaxCoolTime",&g_ovfs_web->maxCoolTime);

            if(g_ovfs_web->maxTryCount == 0)
            {
                g_ovfs_web->maxTryCount = 5;
            }

            if(g_ovfs_web->maxCoolTime == 0)
            {
                g_ovfs_web->maxCoolTime = 60;
            }

            int timeout = 0;
            if(Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/Timeout", &timeout) == NULL)
            {
                Common_Json_SetAttrValueObj(g_ovfs_config, "OnvifCfg");
            }

            if(timeout<=0)
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/Timeout", 86400);
            }

            //if(g_ovfs_web->debugPrint == 1){
            char *out = Common_Json_Print(g_ovfs_config, NULL);
            LOGW("%s\n",out);
            wfree(out);
            //}


        }

        //加载用户配置
        //memset(&g_ovfs_ui_custom, 0, sizeof(WEB_CUSTOM_UI_T));
        //web_ui_custom_init(&g_ovfs_ui_custom);
        g_ovfs_uiconfig = ovfs_web_uiconfig_init();

    }

    return iRet;
}

static void ovfs_web_uninit()
{
    if (g_ovfs_web)
    {
        Common_DList_Uninit(&g_ovfs_web->pNonceList);
        Common_DList_Uninit(&g_ovfs_web->subscribeList);
        Common_DList_Uninit(&g_ovfs_web->loginFailedList);
        Common_DList_Uninit(&g_ovfs_web->userLoginList);
        Common_DList_Uninit(&g_ovfs_web->devInfo.DiskList);
        Common_DList_Uninit(&g_ovfs_web->devInfo.DiskFormatList);

        MUTEX_CLEANUP(g_ovfs_web->hLoginFailedLock);
        MUTEX_CLEANUP(g_ovfs_web->hReqSessionLock);
        MUTEX_CLEANUP(g_ovfs_web->hReqPtzCruiseCallLock);
        MUTEX_CLEANUP(g_ovfs_web->hReqPtzTackCallLock);
        Common_Free(g_ovfs_web, __FUNCTION__, __LINE__);
    }
    if (g_ovfs_config)
    {
        Common_Json_Delete(g_ovfs_config);
    }
    pthread_attr_destroy(&g_thread_attr);
#ifdef WITH_FACE_RECOGNIZE_AND_COMPARE
    ANTS_FaceSql_UnInit();
#endif
}

