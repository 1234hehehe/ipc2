/*
 * echo2.c --
 *
 *      Produce a page containing all the inputs (fcgiapp version)
 *
 *
 * Copyright (c) 1996 Open Market, Inc.
 *
 * See the file "LICENSE.TERMS" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */
//#ifndef lint
//static const char rcsid[] = "$Id: echo-x.c,v 1.1 2001/06/19 15:06:17 robs Exp $";
//#endif /* not lint */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "fcgi_config.h"
#include "goahead.h"
#include "webs.h"
#include "AntsWebCommon.h"
#include "cjson.h"

extern char **environ;
#include "ovfs_web_func.h"
#include "ovfs_web_rest.h"


BOOL ExitFlg = FALSE;
pthread_t fcgi_tid;//fcgi running thread

/*forward declaration*/
static void ovfs_web_actiondefine();
void* Fxn_WebService(void *arg);
void* Fxn_RequestHandle(void *arg);

int Ovfs_Fastcgi_Start(ANTS_WEBSITESDK_CONFIG *cfg)
{
    int iRet = 0;

    memset(&g_struWebSiteSDKInfo, 0, sizeof(ANTS_WEBSITESDK_CONFIG));

    if (0 == iRet)
    {
        iRet = web_init_global_website_info();
        if (iRet != 0)
        {
            LOGE("Get DeviceInfo Failed!\n");
        }
    }
    if (0 == iRet)
    {
        iRet = pthread_create(&fcgi_tid, &g_thread_attr, Fxn_WebService, NULL);
        if (iRet != 0)
        {
            LOGE("Fxn_WebService Failed:%s\n", strerror(iRet));
        }
    }
    if (0 == iRet)
    {
        ExitFlg = FALSE;
    }

    return iRet;
}

//fastcgi stop
void Ants_Fastcgi_Stop(ANTS_WEBSITESDK_CONFIG* cfg)
{
    ExitFlg = TRUE;
}

/*#ifndef SIMPLIFIED
#define THREAD_COUNT 10
#else*/
#define THREAD_COUNT 4
//#endif

typedef struct
{
    int listen_sock;
    pthread_t id[THREAD_COUNT];
} Global_Context;

void SleepMs(S32 nSec,S32 MicroSec)
{
    struct timespec req,rem;
    if (MicroSec == 0)
    {
        sleep(nSec);
    }
    else if(nSec * 1000 + MicroSec /1000 >= 10)
    {
        usleep(nSec * 1000000 + MicroSec);
    }
    else
    {
        req.tv_sec = (time_t)nSec;
        req.tv_nsec = ((long)MicroSec)*1000;
        nanosleep(&req,&rem);
    }
}

void *Fxn_Param_check(void *a)
{
    Utils_SetThreadName();
    pthread_detach(pthread_self());

    for (;;)
    {
        nonceList_check();
        subscribeList_done();
        loginFailedList_done();
        SleepMs(1,0);
    }

    return NULL;
}


void *Fxn_RequestHandle(void *a)
{
    int rc;
    int iloop = 0;
    int index = -1;
    Global_Context *ctx = NULL;
    FCGX_Request request;
    Webs webs_ctx;

    Utils_SetThreadName();

    ctx = (Global_Context*)a;

    // 1.获取处理进程的序号
    for (iloop = 0; iloop < 10; iloop ++)
    {
        if (ctx->id[iloop] == pthread_self())
        {
            index = iloop;
            break;
        }
    }
    if (FCGX_InitRequest(&request, ctx->listen_sock, 0) != 0)
    {
        DEBUGV3("\nCan not init request\n");
        return NULL;
    }
    for(;;)
    {
        //static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
        rc = FCGX_Accept_r(&request);
        //LOGE("thread_id:%d  rc:%d\n", index, rc);
        if(rc < 0 || ExitFlg)
        {
            LOGE("\nCan not accept new request\n");
            break;
        }
        initWebs(&webs_ctx, 1, 1);

        webs_ctx.req = &request;
        webs_ctx.timestamp = 0;// time(0);
        webs_ctx.thread_index = index;

        web_process_requests(&webs_ctx);

        termWebs(&webs_ctx, 1);

        //LOGE("thread_id:%d\n", index);
    }

    return NULL;
}

void* Fxn_WebService(void *arg)
{
    int err,i;
    Global_Context gCtx;
    WebsRoute* route;
    pthread_attr_t attr;

    gCtx.listen_sock= -1;
    Utils_SetThreadName();
    //open fcgi server first then nginx server
    if (websOpen() < 0)
    {
        LOGE("WebOpen Failed!\n");
        return NULL;
    }

    if(g_ovfs_web->enable_T8S)
    {

        route = websAddRoute("/t8s", "action", -1);
        websSetRouteAuth(route, "digest");
    }
    else
    {
        route = websAddRoute("/goform", "action", -1);
        websSetRouteAuth(route, "digest");
        route = websAddRoute("/digest", "action", -1);
        websSetRouteAuth(route, "digest");
    }




    ovfs_web_actiondefine();
    //support GET method,js parse xml file
    if(0 != is_file_exist("/root/nginx/html/factoryInfo.xml"))
    {
        Common_System("ln -s /root/res_xml/factoryInfo.xml  /root/nginx/html/factoryInfo.xml");
    }
    ///tmp/mmc/mmc1/snapshot
    if(0 != is_file_exist("/root/nginx/html/snapshot"))
    {
        Common_System("ln -s /tmp/mmc/mmc1/snapshot  /root/nginx/html/snapshot");
    }

    FCGX_Init();
#if 1
    err = mkdir("/var/run/webserver", 0);
    if (err < 0 && errno != EEXIST)
    {
        LOGE("Failed access dir %s. error:%s\n", "/var/run/webserver", strerror(errno));
    }
    gCtx.listen_sock = FCGX_OpenSocket("/var/run/webserver/unix", 1000);
#else
    gCtx.listen_sock = FCGX_OpenSocket("127.0.0.1:28", 1000);
#endif
    if(gCtx.listen_sock < 0)
    {
        LOGE("FCGI Socket Bind Failed!\n");
        return NULL;
    }
    //create thread do expired session check things
    pthread_create(&g_ovfs_web->param_check, &g_thread_attr, Fxn_Param_check, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, IPC_THREAD_STACK_MIN);
    for (i = 0; i < THREAD_COUNT; i++)
    {
        err = pthread_create(&gCtx.id[i], &attr, Fxn_RequestHandle, &gCtx);
        if (err != 0)
        {
            LOGE("Fxn_RequestHandle create err=%d(%s)!\n", err, strerror(err));
        }
    }
    //wait thread go die!
    for(i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(gCtx.id[i], NULL);
    }
    websClose();

    return NULL;
}

static struct
{
    const char *name;
    void *function;
} s_webApiTable[] =
{
    {"frmHelp", frmHelp},
    {"frmWebApiVersion", frmWebApiVersion},
    {"frmDeviceAbility", frmDeviceAbility},
    //登录
    {"frmUserLogin", frmUserLogin},
    //登出
    {"frmUserLogout", frmUserLogout},

    //获取rtsp地址(预览,对讲，报警，回放)
    {"frmGetRtspUrl", frmGetRtspUrl},
    {"frmOptimalVideoEncode", frmOptimalVideoEncode},

    //带验证的抓图
    {"CaptureV2", CaptureV2},
    //查询报警输入
    {"frmQueryAlarmInfo", frmQueryAlarmInfo},

    {"frmReqIFrame", frmReqIFrame},

    //获取报警信息(UDP)
    //获取报警信息(HTTP)(type 0-获取报警信息 1-布防 2-撤防)
    {"frmGetAlarmInfo", frmGetAlarmInfo},
    //获取日志
    //{"frmLogCtrl", frmLogCtrl},
    //设备信息(type 0-获取  1-设置)
    {"frmDevicePara", frmDevicePara},
    //获取视频制式(type 0-获取 1-设置)
    {"frmVideoFormatPara", frmVideoFormatPara},
    {"frmVideoFormatPara_v2", frmVideoFormatPara_v2},
    {"frmVideoImageModePara", frmVideoImageMode},
    //获取设备时间(type 0-获取 1-设置)
    {"frmDeviceTimeCtrl", frmDeviceTimeCtrl},
    //系统信息
    {"frmGetFactoryInfo", frmGetFactoryInfo},

    //获取二维码图片
//#endif
    //通道名称和状态
    //图片属性 (type 0-获取 1-设置)
    {"frmVideoEffect", frmVideoEffect},
    //视频编码 (tyep 0-获取 1-设置)
    {"frmVideoCompressAbility", frmVideoCompressAbility},
    {"frmGetSmartAbility", frmGetSmartAbility},
    //当前视频参数
    //视频参数(ISP)
    {"frmVideoParaEx", frmVideoParaEx},
    {"frmImageCapability", frmImageCapability},
	//设置双光源
	//{"frmVideoBoardAbility", frmVideoBoardAbility},
	//{"frmVideoTestHardWare", frmVideoTestHardWare},
	//设置onvif
	{"frmOnvifPara", frmOnvifPara},
#if 1//ndef SIMPLIFIED
    //镜头校准
    {"frmAutoLensCorrection", frmAutoLensCorrection},
    {"frmIspConfig", frmIspConfig},
    //自动光圈
    //{"frmAutoApertureCorrection", frmAutoApertureCorrection},
    //{"frmDCIrisCfg", frmDCIrisCfg},
    //坏点检测
    {"frmBadPointTest", frmBadPixelTest},
#endif
    //视频遮挡 (type 0-获取 1-设置)
    //{"frmVideoHidePara", frmVideoHidePara},
    //视频遮盖 (type 0-获取 1-设置)
    {"frmVideoShelterPara", frmVideoShelterPara},
    //IP通道信息
    //连接状态信息
    //网络设置 (type 0-获取 1-设置)
    {"frmNetworkSettings", frmNetworkSettings},
#if 1//ndef SIMPLIFIED
    //Email (type 0-获取 1-设置)
    {"frmEmailSetting", frmEmailSetting},
    //获取所有DDNS服务器信息
    {"frmGetDDNSServiceAbility", frmGetDDNSServiceAbility},
    //DDNS信息
    {"frmNetDDNSPara", frmNetDDNSPara},
    //{"frmNasSetting", frmNasSetting},
    //{"frmP2PCfg", frmP2PCfg},
    //{"frmP2PState", frmP2PState},
    //{"frmTurnServerCfg", frmTurnServerCfg},
    //{"frmTurnServerState", frmTurnServerState},
    //{"frmCustomPlatformCfg", frmCustomPlatformCfg},
#endif
    {"frmAliIoTCfg", frmAliIoTCfg},
    {"frmAliIoTState", frmAliIoTState},
    {"frmAliIoTReboot", frmAliIoTReboot},
    {"frmAliIoTUnbinding", frmAliIoTUnbinding},
    {"frmRtspCfg", frmRtspCfg},
    //Telnet (type 0-获取 1-设置)
    {"frmNetTelnetPara", frmNetTelnetPara},
    //NTP (type 0-获取 1-设置)
    {"frmNetNtpPara", frmNetNtpPara},
    //当前时间偏移,考虑夏令时
    {"frmCurTimeOffset", frmCurTimeOffset},

    //管理主机的信息
    {"frmGetManagerHostsPara", frmGetManagerHostsPara},
    {"frmGetDefaultRoute", frmGetDefaultRoute},
    {"frmLteCustomCfg", frmLteCustomCfg},

    {"frmThirdPartyProtocols", frmThirdPartyProtocols},
    {"frmAudioBroadcast", frmAudioBroadcast},


#if 1//ndef SIMPLIFIED
    //Net 3G
    //获取支持的wifi信息
    //wifi (type 0-获取 1-设置)
    //{"frmWifiPara", frmWifiPara},
    //{"frmSwitchSTAPara", frmSwitchSTAPara},
    //4G
    {"frmNetLtepara", frmNetLtepara},
    {"frmGetLteCardInfo", frmGetLteCardInfo},
    // SIP协议
    {"frmNetSipPara", frmNetSipPara},
    //获取管理主机协议
    //报警输入 (type 0-获取 1-设置)
    {"frmAlarmInPara_V2", frmAlarmInPara_V2},
    //报警输出 (type 0-获取 1-设置)
    {"frmAlarmOut", frmAlarmOut},
    //异常报警
    //{"frmAlarmException", frmAlarmException},
#endif

    //用户管理 (type 0-获取 1-修改和添加 2-删除)
    {"frmUserManage", frmUserManage},
    //获取单个用户权限
    {"frmUserRights_V2", frmUserRights_V2},
    //获取在线用户
    {"frmUserOnline", frmUserOnline},
    {"frmMaxUserOnlineNum", frmUserOnlineCfg},
    //获取升级信息
    {"frmSysUpdate", frmSysUpdate},
    //升级释放内存
    {"frmSysUpdateFree", frmSysUpdateFree},
    {"frmFreeAppMemory", frmSysUpdateFree},
    //上传文件并升级
    //获取升级进度
    {"GetProgress", GetProgress},
    //重启
    {"frmDeviceReboot", frmDeviceReboot},
    //自动维护 (type 0-获取 1-设置)
    {"frmAutoReboot", frmAutoReboot},
    {"frmLaserLight", frmLaserLight},

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)

    //格式化进度
    {"frmGetHDFormatProgress", frmGetHDFormatProgress},
    //磁盘格式化
    {"frmHDFormat", frmHDFormat},
    //磁盘信息
    {"frmGetHDInfo", frmGetHDInfo},
    //磁盘状态
    {"frmGetHDStatus", frmGetHDStatus},
    {"frmGetHDFSInfo", frmGetHDFSInfo},
    {"frmRecordConfig", frmRecordConfig},
#endif

    //{"frmSpeechAlarmCfg", frmSpeechAlarmCfg},
#ifndef WIFIDOME
    //语音报警
    //{"frmAudioAlarmCfg", frmAudioAlarmCfg},
    //检测声光布防时间
    //{"frmActionAlarmTimePara", frmActionAlarmTimePara},
    //录像计划
    //{"frmVideoPlanPara", frmVideoPlanPara},
    //检测联动参数
    //{"frmDetectLinkPara", frmDetectLinkPara},
    //移动侦测 (type 0-获取 1-设置)
    //{"frmMotionDetPara", frmMotionDetPara},
    //单行OSD (type 0-获取 1-设置)
    {"frmSingleLineOSD", frmSingleLineOSD},
//#ifndef SIMPLIFIED
    //多行OSD (type 0-获取 1-设置)
    {"frmMultiLineOSD", frmMultiLineOSD},

    //人形检测
    //{"frmVideoPersonPara", frmVideoPersonPara},

#endif

    {"frmMotionDetect_V2", frmMotionDetect_V2},
    {"frmVideoHide_V2", frmVideoHide_V2},
    {"frmSetCustomAudio", frmSetCustomAudio},
    {"frmAudioSpeech", frmAudioSpeech},
    {"frmRegionalInvasion", frmRegionalInvasion},
    {"frmTraverseDetect", frmTraverseDetect},
    {"frmPersonStaying", frmPersonStaying},
    {"frmPersonAbsent", frmPersonAbsent},
    {"frmParkingViolation", frmParkingViolation},
    {"frmVehicleRetrograde", frmVehicleRetrograde},
    {"frmDisableAllSmart", frmDisableAllSmart},

    {"frmCruiseControl", frmCruiseControl},
    //云台控制
    {"frmPTZControl", frmPTZControl},
    {"frmPTZStepControl", frmPTZStepControl},
    //预置点
    {"frmPTZPreset", frmPTZPreset},

    {"frmUartConfig", frmUartConfig},
    {"frmExtendOSD", frmExtendOSD},
    {"frmExpandAlarmOut", frmExpandAlarmOut},
    {"frmSensorAlarm", frmSensorAlarm},

    //PTZ设置
    {"frmGetPTZProtocal", frmGetPTZProtocal},
    //解码参数 (type 0-获取 1-设置)
    {"frmDecoderPara", frmDecoderPara},
#if 0
    //巡航(type:0-获取巡航设置 1-添加巡航预置点 2-删除巡航预置点)
    {"frmPTZCruise", frmPTZCruise},

    //轨迹(type:0-设置轨迹 1-调用轨迹)
    {"frmPTZTrack", frmPTZTrack},

//#if 0//def WITH_PTZ
    /*!--------------- IPC私有语义------------------!*/
    // 两点扫描
    {"frmPTZExtend_SetScan", frmPTZExtend_SetScan},
    // 空闲操作
    {"frmPTZExtend_IdleOperation", frmPTZExtend_IdleOperation},
    // 3D定位
    {"frmPTZExtend_3DPosition", frmPTZExtend_3DPosition},
    //红外补光颜色切换
    {"frmPTZExtend_IrlightCtrl",frmPTZExtend_IrlightCtrl},
    //隐私遮挡
    {"frmPTZExtend_SetCover", frmPTZExtend_SetCover},
    //雨刷
    {"frmPTZExtend_Wiper", frmPTZExtend_Wiper},
    //喷淋位置
    {"frmPTZExtend_SprayPos", frmPTZExtend_SprayPos},
    //喷淋模式
    {"frmPTZExtend_SprayMode", frmPTZExtend_SprayMode},

#endif

    //获取二维码图片
    {"frmGetQRCodePictureV2", frmGetQRCodePictureV2},

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)
    //录像查询
    {"frmFlashVideoQuery", frmFlashVideoQuery},
    {"frmVideoRecordsQuery", frmVideoRecordsQuery},
    {"frmVideoQueryByMonth", frmVideoQueryByMonth},
    //检索SD卡
    {"frmSearchSDCardPics", frmSearchSDCardPics},

    {"frmRtmpPushCfg",frmRtmpPushCfg},
    {"frmPushSetting", frmPushSetting},
#endif

    {"frmProductTest",frmProductTest},

    {"frmOneKeyDriveCfg", frmOneKeyDriveCfg},
    {"frmOneKeyDriveControl", frmOneKeyDriveControl},

    {"frmIotRecordCfg", frmIotRecordCfg},
    {"frmIotSDStatus", frmIotSDStatus},
    {"frmIotSDFormat", frmIotSDFormat},
    {"frmIotNetworkStatus", frmIotNetworkStatus},
    {"frmIotPresetControl", frmIotPresetControl},
    {"frmIotHomePositionCfg", frmIotHomePositionCfg},

    {"frmIotLightCfg", frmIotLightCfg},
    {"frmExposureLight", frmExposureLight},

    {"frmAovSleep", frmAovSleep},
    {"frmSwitchSensor", frmSwitchSensor},
    {"frmBatteryConfig", frmBatteryConfig},
    {"frmIndicatorLightConfig", frmIndicatorLightConfig},
    {"frmSimCardSwitch", frmSimCardSwitch},

    {"frmUsageLog", frmUsageLog},
    {"frmAlarmLog", frmAlarmLog},

    //视频参数
    {"frmVideoIPCSetPara", frmVideoIPCSetPara},
	//PTZ联动
    //{"frmPTZLinkCFG", frmPTZLinkCFG},
#if 0//ndef SIMPLIFIED
    {"frmHttpLinkPara", frmHttpLinkPara},
#endif

    //获取配置文件
    {"frmGetConfigFileV2", frmGetConfigFileV2},
    {"frmExportConfig", frmExportConfig},
    //设置配置文件
    {"frmSetConfigFileV2", frmSetConfigFileV2},
    //恢复默认
    {"frmParaSysRestore", frmParaSysRestore},
    {"frmFactoryRestore", frmFactoryRestore},
    {"frmDeviceRestore", frmDeviceRestore},

    //更改文件名
    {"frmChangeFileName", frmChangeFileName},
    //多播
    {"frmMulticast", frmMulticast},
#if 1//ndef SIMPLIFIED
    //FTP
    {"frmFTPSetting", frmFTPSetting},
    {"frmNetSnmp", frmNetSnmp},
    //    {"frmNetIPCPortMapping",frmNetIPCPortMapping},
#endif
    //初始化获取28181协议参数
    {"frmParaPlatform28181", frmParaPlatform28181},
    //音频参数
    {"frmAudioPara", frmAudioPara},
    {"frmAudioParaAbility", frmAudioParaAbility},
    //夏令时
    {"frmDstPara", frmDstPara},
    {"frmKeepSessionAlive", frmKeepSessionAlive},

    //加载升级程序
    {"upload", upload},
    {"frmGetPNFormatAbility", frmGetFormatAbility},
    //图像模式
    {"frmGetImageModeAbility", frmGetImageModeAbility},
    {"frmUploadInfo", frmUploadInfo},
    {"frmUpgradeProgress", GetProgress},

    {"frmSetUuid", frmSetUuid},

    //平台断开连接
    {"frmMediaDisconnect", frmMediaDisconnect},
    {"frmPasswordLost", frmPasswordLost},
    //http https config
    {"frmHttpHttpsConfig",frmHttpHttpsConfig},
#if 1//ndef SIMPLIFIED
    //UPNP设置
    {"frmNetUPNPPara", frmNetUPNPPara},
    //HTTP事件
    {"frmHttpEventSetting", frmHttpEventSetting},
    //硬件检查
    //{"frmHwCheck",frmHwCheck},
    //water mark
    //{"frmWaterMark",frmWaterMark},
    //white black list
    {"frmBlackWhiteList",frmBlackWhiteList},
    //http推送
    {"frmHttpPushCfg",frmHttpPushCfg},
	//http服务器地址测试
	{"frmHttpAddrTest",frmHttpAddrTest},
#endif
    {"frmSetLogoPic",frmSetLogoPic},

    {"frmAlarmPushConfig",frmAlarmPushConfig},
    {"frmNetworkStatus",frmNetworkStatus},
    {"frmLightConfig",frmLightConfig},
    {"frmAllOSDConfig",frmAllOSDConfig},
    {"frmWLANConfig",frmWLANConfig},


    //localsettings
    {"frmLocalSettings",frmLocalSettings},
    {"frmValidateTokens",frmValidateTokens}

};

char* white_list[] =
{
    "frmUserLogin",
    "frmCapture",
    "frmGetFactoryInfo",
    "frmGetConfigFile",
    "frmSetConfigFile",
    "frmWebApiVersion",
    "frmHelp",
    "frmPasswordLost",
    "frmLocalSettings",
    //"frmDevicePara",
    "frmSwitchSTAPara",
    "frmUploadInfo",
    NULL
};

int name_in_white_list(char* name)
{
    int i = 0;
    int ret = 0;
    while(white_list[i] != NULL)
    {
        if(smatch(white_list[i],name))
        {
            ret = 1;
            break;
        }
        i++;
    }
    return ret;
}

static void ovfs_web_actiondefine()
{
    int count = sizeof(s_webApiTable)/sizeof(s_webApiTable[0]);
    int i;
    for (i = 0; i < count; i++)
    {
        websDefineAction(s_webApiTable[i].name, s_webApiTable[i].function);
    }
}

void nonceList_check()
{
    int iloop = 0;
    int size = 0;
    WEB_NONCE_NODE_T *node_tmp = NULL;

    size = Common_DList_GetCount(g_ovfs_web->pNonceList);
    for (iloop = 0; iloop < size; iloop ++)
    {
        node_tmp = (WEB_NONCE_NODE_T *)Common_DList_GetNode(g_ovfs_web->pNonceList, iloop);
        if (node_tmp)
        {
            if (abs(node_tmp->time_gen - time(NULL)) >= 20)
            {
                Common_DList_Delete(g_ovfs_web->pNonceList, node_tmp->nonce, web_nonce_nodecompare);
            }
        }
    }

    return ;
}


static int web_httpLast = -1;
static int web_http = -1;
static int web_httpsLast = -1;
static int web_https = -1;

void subscribeList_done()
{
    int iloop = 0;
    int size = 0;
    WEB_SUBSCRIBE_NODE_T *subscribe_tmp = NULL;
    cJSON_Struct *pInPut = NULL;


    size = Common_DList_GetCount(g_ovfs_web->subscribeList);
    for (iloop = 0; iloop < size; iloop ++)
    {
        subscribe_tmp = (WEB_SUBSCRIBE_NODE_T *)Common_DList_GetNode(g_ovfs_web->subscribeList, iloop);
        if (subscribe_tmp)
        {
            if (strstr(subscribe_tmp->uri, "Wifi"))
            {
                //subscribe_tmp->bFirst = 0;
            }
            else if (strstr(subscribe_tmp->uri, "DeviceStatus"))
            {
                if ((subscribe_tmp->bFirst == 1) || g_ovfs_web->need_send_devicestatus)
                {

                    LOGW("send [%d  Uri:%s  bFirst:%d  ToID:%d]\n", iloop, subscribe_tmp->uri, subscribe_tmp->bFirst, subscribe_tmp->toId);
                    if(g_ovfs_web->need_send_devicestatus)
                    {
                        g_ovfs_web->need_send_devicestatus = 0;
                    }
                    if(subscribe_tmp->bFirst)
                    {
                        subscribe_tmp->bFirst = 0;
                    }

                    if(pInPut == NULL)
                    {
                        pInPut = Common_Json_Duplicate(g_ovfs_web->pDeviceStatus, 1);
                        int netType = 0;
                        if(Common_Json_GetAttrValueInt(pInPut, "NetType", &netType))
                        {
                            if(netType != 2)
                            {
                                Common_Json_RemoveItem(pInPut, -1, "SSID");
                                Common_Json_RemoveItem(pInPut, -1, "Password");
                            }
                            if(netType != 3)
                            {
                                Common_Json_RemoveItem(pInPut, -1, "CCID");
                                Common_Json_RemoveItem(pInPut, -1, "SimId");
                                Common_Json_RemoveItem(pInPut, -1, "SimSwitchStatus");
                            }

                            if(netType == 3)
                            {
                                Common_Json_RemoveItem(pInPut, -1, "NetMask");
                                Common_Json_RemoveItem(pInPut, -1, "Gateway");
                            }
                        }

                        ovfs_print_json(pInPut);
                    }

                    Access_SendEvent(g_AccessHandle, subscribe_tmp->toId, pInPut, NULL, 0);
                }
            }
        }
    }

    Common_Json_Delete(pInPut);
    pInPut = NULL;
    return ;
}

void loginFailedList_done()
{
    int iloop = 0;
    int user_num = 0;
    COMMON_DLIST_T del_user_list;
    WEB_DLIST_STRING_T *pstr_node_tmp = NULL;
    WEB_LOGINFAILED_NODE_T *pTmp = NULL;

    Common_DList_Init(&del_user_list, web_list_string_nodefree);

    MUTEX_LOCK(g_ovfs_web->hLoginFailedLock);
    {
        user_num = Common_DList_GetCount(g_ovfs_web->loginFailedList);
        for (iloop = 0; iloop < user_num; iloop ++)
        {
            pTmp = (WEB_LOGINFAILED_NODE_T *)Common_DList_GetNode(g_ovfs_web->loginFailedList, iloop);
            if (1 == pTmp->blocked)
            {
                pTmp->remain_time -= 1;
                //LOGW("IP:%s   Time:%d\n", pTmp->ip?pTmp->ip:"", pTmp->remain_time);
                if (pTmp->remain_time <= 0)
                {
                    pstr_node_tmp = (WEB_DLIST_STRING_T *)Common_Calloc(1, sizeof(WEB_DLIST_STRING_T), __FUNCTION__, __LINE__);
                    pstr_node_tmp->str = Common_StrDup(pTmp->ip, __FUNCTION__, __LINE__);
                    Common_DList_InsertTail(del_user_list, (void *)pstr_node_tmp, sizeof(WEB_DLIST_STRING_T));
                }
            }
        }
    }
    // 删除对应的用户node
    {
        user_num = Common_DList_GetCount(del_user_list);
        for (iloop = 0; iloop < user_num; iloop ++)
        {
            pstr_node_tmp = (WEB_DLIST_STRING_T *)Common_DList_GetNode(del_user_list, iloop);
            Common_DList_Delete(g_ovfs_web->loginFailedList, (void *)pstr_node_tmp->str, web_list_string_nodecompare);
        }
        Common_DList_DeleteAll(del_user_list);
        Common_DList_Uninit(&del_user_list);
    }
    MUTEX_UNLOCK(g_ovfs_web->hLoginFailedLock);

    return ;
}

int web_set_log(OVFS_WEB_LOG_T *log)
{
    int ret = 0;

    if (log == NULL)
    {
        ret = -1;
    }

    cJSON_Struct *header = NULL;
    if (0 == ret)
    {
        if ((header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/EventLog/LogFunction");
        }
    }

    cJSON_Struct *data = NULL;
    if (0 == ret)
    {
        data = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (data == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            log->logtime = time(NULL);
            cJSON_Struct * resList = Common_Json_SetAttrValue(data, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(resList, 0, "LogTime", Common_Json_Type_Number, NULL, log->logtime, 0);
            Common_Json_SetAttrValue(resList, 0, "MajorType", Common_Json_Type_Number, NULL, log->major_type, 0);
            Common_Json_SetAttrValue(resList, 0, "MinorType", Common_Json_Type_Number, NULL, log->minor_type, 0);
            Common_Json_SetAttrValue(resList, 0, "UserName", Common_Json_Type_String, log->username, 0, 0);
            Common_Json_SetAttrValue(resList, 0, "RemoteHostAddress", Common_Json_Type_String, log->ip, 0, 0);
            Common_Json_SetAttrValue(resList, 0, "Channel", Common_Json_Type_Number, NULL, log->channel, 0);
            Common_Json_SetAttrValue(resList, 0, "AlarmInPort", Common_Json_Type_Number, NULL, log->alarm_in_port, 0);
            Common_Json_SetAttrValue(resList, 0, "AlarmOutPort", Common_Json_Type_Number, NULL, log->alarm_out_port, 0);
            Common_Json_SetAttrValue(resList, 0, "Status", Common_Json_Type_Number, NULL, log->status, 0);
        }
    }

    if (0 == ret)
    {
        ret = Ovfs_Web_RestMethodA(header, data, NULL, 0);
        if (ret)
        {
            LOGW("Failed RestMethod %d.\n", ret);
        }
    }

    Common_Json_Delete(header);
    header = NULL;

    Common_Json_Delete(data);
    data = NULL;

    return ret;
}

int ovfs_web_send_alarm(OVFS_WEB_ALARM_T *alarm)
{
    int ret = 0;

    if (alarm == NULL)
    {
        ret = -1;
    }

    cJSON_Struct *header = NULL;
    if (0 == ret)
    {
        if ((header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/CollectData");
        }
    }

    cJSON_Struct *data = NULL;
    if (0 == ret)
    {
        if ((data = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) ==NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            cJSON_Struct * resList = Common_Json_SetAttrValue(data, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
            if (resList)
            {
                Common_Json_SetAttrValue(resList, 0, "AlarmName", Common_Json_Type_String, alarm->alarm_name, 0, 0);
                Common_Json_SetAttrValue(resList, 0, "IsHappent", Common_Json_Type_Number, NULL, alarm->ishappen, 0);
                Common_Json_SetAttrValue(resList, 0, "RemoteIP", Common_Json_Type_String, alarm->ip, 0, 0);
            }
        }
    }

    if (0 == ret)
    {
        //Common_Json_StandardPrint(header, NULL, NULL, NULL);
        //Common_Json_StandardPrint(data, NULL, NULL, NULL);
        ret = Ovfs_Web_RestMethodA(header, data, NULL, 0);
        if (ret)
        {
            LOGW("Failed RestMethod %d.\n", ret);
        }
    }

    Common_Json_Delete(header);
    header = NULL;

    Common_Json_Delete(data);
    data = NULL;

    return ret;
}

int frmHelp(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    cJSON_Struct *interfaceName = NULL;
    int count = sizeof(s_webApiTable)/sizeof(s_webApiTable[0]);
    Common_Json_SetAttrValueInt(outdata, "InterfaceCount", count);
    if (count > 0)
    {
        interfaceName = Common_Json_SetAttrValueArr(outdata, "InterfaceName");
        int i;
        for (i = 0; i < count; i++)
        {
            Common_Json_SetAttrValueArrStr(interfaceName, i, s_webApiTable[i].name);
        }
    }

    return ret;
}

const char* QueryBuildString()
{
    static char buildStr[32]= "";

    if (buildStr[0] == '\0')
    {
        const char *buildDate = __DATE__;
        char year[8];
        int month = 1;
        char mday[4];

        sscanf(buildDate, "%*s %s %s", mday, year);
        if (buildDate[0] == 'J' && buildDate[1] == 'a' && buildDate[2] == 'n')
        {
            month = 1;
        }
        else if (buildDate[0] == 'F')
        {
            month = 2;
        }
        else if (buildDate[0] == 'M' && buildDate[1] == 'a' && buildDate[2] == 'r')
        {
            month = 3;
        }
        else if (buildDate[0] == 'A' && buildDate[1] == 'p')
        {
            month = 4;
        }
        else if (buildDate[0] == 'M' && buildDate[1] == 'a' && buildDate[2] == 'y')
        {
            month = 5;
        }
        else if (buildDate[0] == 'J' && buildDate[1] == 'u' && buildDate[2] == 'n')
        {
            month = 6;
        }
        else if (buildDate[0] == 'J' && buildDate[1] == 'u' && buildDate[2] == 'l')
        {
            month = 7;
        }
        else if (buildDate[0] == 'A' && buildDate[1] == 'u')
        {
            month = 8;
        }
        else if (buildDate[0] == 'S')
        {
            month = 9;
        }
        else if (buildDate[0] == 'O')
        {
            month = 10;
        }
        else if (buildDate[0] == 'N')
        {
            month = 11;
        }
        else if (buildDate[0] == 'D')
        {
            month = 12;
        }

        snprintf(buildStr, sizeof(buildStr), "%s-%02d-%s %s", year, month, mday, __TIME__);
    }

    return buildStr;
}

int frmWebApiVersion(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Common_Json_SetAttrValueStr(outdata, "Standard", OVFS_WEB_API_VERSION);
    Common_Json_SetAttrValueStr(outdata, "Build", QueryBuildString());

    return ret;
}


