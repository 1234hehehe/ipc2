
/*
 * ovfs_smart_rest.c
 *
 *  Created on: 2017骞?2鏈?28鏃?
 *      Author: eric
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <common_json_str_ops.h>
#include "ovfs_onvif.h"
#include "ovfs_com_utils.h"
#include "third_protocol_api.h"

typedef struct
{
    void *pipeHandle;
    MQ_HANDLE_H mqHandle;
    pthread_pool_t *tpoolHandle;
    //cJSON_Struct *jsonCfg;
    ONVIF_MGR_CFG_T cfg;
    ONVIF_ALARM_STATUS_T alarmStatusInfo;
    void *asyncHandle;
    fCallParamFun fCallFxn;
} ONVIF_CONTEXT_T;

extern ONVIF_CONTEXT_T s_onvif_ct;

const ONVIF_OSD_MAP_T s_osd_date_map[] =
{
    { 0, "yyyy-MM-dd" },
    { 1, "MM-dd-yyyy" },
    { 2, "dd-MM-yyyy" },
    { 0, "yyyy/MM/dd" },
    { 1, "MM/dd/yyyy" },
    { 2, "dd/MM/yyyy" },
    { 0, NULL}
};

const ONVIF_OSD_MAP_T s_osd_hour_map[] =
{
    { 0, "HH:mm:ss" },
    { 1, "hh:mm:ss tt" },
    { 0, NULL}

};

typedef int (*OVFS_ONVIF_REST_FUNC_F)(const char *uri, Common_cJSON_T *curObj,
                                      Common_cJSON_T *in, Common_cJSON_T *out, void *userData);

typedef struct
{
    int ec;
    const char *es;
} ONVIF_REST_ERR_T;

enum
{
    ONVIF_REST_STREAM_STATE_UNINIT = 0,
    ONVIF_REST_STREAM_STATE_INIT,
};

typedef struct
{
    int streamState;
    MQ_HANDLE_H mqHandle;
    AccessHandle_T accessHandle;
    int streamHandle;
    int streamRemoteHandle;
    pthread_mutex_t streamLock;
    Common_cJSON_T *restTree;
    unsigned long long int lastDefPasscheckTime;
    int isDefPass;
    char defIpAddr[32];
    ONVIF_VideoEncoderCapability_T videoAbility[3];
} OVFS_ONVIF_REST_CONTEXT_T;

typedef struct
{
    const char *uri;
    const char *label;
    const char *describtion;

    OVFS_ONVIF_REST_FUNC_F get;
    OVFS_ONVIF_REST_FUNC_F put;
    pthread_mutex_t subscribeLock;
    int subscribeTable[ONVIF_SUBSCRIBE_MAX_NUM];

} OVFS_ONVIF_REST_NODE_ATTR_T;

static ONVIF_REST_ERR_T s_restErrorInfo[] =
{
    {EC_ONVIF_REST_UNKNOWN,         EC_ONVIF_REST_UNKNOWN_STR},
    {EC_ONVIF_REST_PARAM_INVALID,   EC_ONVIF_REST_PARAM_INVALID_STR},
    {EC_ONVIF_REST_NO_METHOD,       EC_ONVIF_REST_NO_METHOD_STR},
    {EC_ONVIF_REST_NO_URI,          EC_ONVIF_REST_NO_URI_STR},
    {EC_ONVIF_REST_OP_FAILED,       EC_ONVIF_REST_OP_FAILED_STR},
};

static OVFS_ONVIF_REST_CONTEXT_T s_onvif_rest_ct;


int RestOnvif_Init(MQ_HANDLE_H mqHandle)
{
    int ret = 0;
    LOGD("do\n");

    if (mqHandle == NULL)
    {
        LOGE("mq handle was null\n");
        return -1;
    }

    s_onvif_rest_ct.mqHandle = mqHandle;

    ret = -1;
    Mq_Request(s_onvif_rest_ct.mqHandle, ONVIF_REQ_CFG_LOAD, NULL, 0, &ret, NULL,
               0);
    if (ret < 0)
    {
        LOGE("request load cfg failed\n");
        return -1;
    }

    LOGD("done\n");
    return 0;
}

int RestOnvif_Uninit()
{
    // Access_Unint(&s_onvif_rest_ct.accessHandle);
    Common_cJSON_Delete(s_onvif_rest_ct.restTree);
    s_onvif_rest_ct.accessHandle = NULL;
    s_onvif_rest_ct.restTree = NULL;
    return 0;
}

int RestOnvif_LoadCfg(ONVIF_MGR_CFG_T *cfg)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_ONVIF_CFG *onvifcfg = NULL;
    int cmd = THIRD_CMD_GET_ONVIF_CFG;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        onvifcfg = (THIRD_ONVIF_CFG *)outbuffer;
        cfg->authEnable = onvifcfg->AuthEnable;
        cfg->adaptiveIp = onvifcfg->AdaptiveIp;
        cfg->timeout = onvifcfg->Timeout;
        cfg->fixedIp = onvifcfg->FixedIp;
        snprintf(cfg->fixedIpAddr,sizeof(cfg->fixedIpAddr),"%s", onvifcfg->FixedIpAddr);

    }

    return ret;

}

int RestOnvif_SaveCfg(ONVIF_MGR_CFG_T cfg)
{
    int ret = 0;
    THIRD_ONVIF_CFG onvifcfg = {0};
    onvifcfg.Cmd = THIRD_CMD_SET_ONVIF_CFG;
    onvifcfg.UseMask = 0xff;
    onvifcfg.AuthEnable = cfg.authEnable;
    onvifcfg.AdaptiveIp = cfg.adaptiveIp;
    onvifcfg.Timeout = cfg.timeout;
    onvifcfg.FixedIp = cfg.fixedIp;
    Common_Strcpy(onvifcfg.FixedIpAddr, cfg.fixedIpAddr);

    ret = s_onvif_ct.fCallFxn(NULL,&onvifcfg,sizeof(onvifcfg),NULL,NULL,1000);

    return ret;
}

typedef struct
{
    char Ipv4[16];
    char Ipv6[16];
    char Mac[32];
} OVFS_IPADDR_T;

typedef struct
{
    OVFS_RESTMETHOD_E method;
    char *toUri;
    ONVIF_AUTH_INFO_T *auth;
    int isRemote;
    OVFS_IPADDR_T ipAddr;
    cJSON_Struct *data;
} OVFS_REST_INPARAM_T;

int g_encQuality[2] = {0,5};

static int RestOnvif_RestMethod(AccessHandle_T accessHandle,
                                OVFS_REST_INPARAM_T *inParam,
                                cJSON_Struct **outParam, int timeOut)
{
    int iRet = 0;
    char method[10] = {0};
    cJSON_Struct *pInParams = NULL;
    cJSON_Struct *pOutResults = NULL;
    if(outParam)
    {
        *outParam = NULL;
    }

    switch (inParam->method)
    {
    case REST_GET:
        snprintf(method, sizeof(method), "get");
        break;
    case REST_PUT:
        snprintf(method, sizeof(method), "put");
        break;
    case REST_POST:
        snprintf(method, sizeof(method), "post");
        break;
    case REST_DELETE:
        snprintf(method, sizeof(method), "delete");
        break;
    default:
        LOGE("Unknow Method!\n");
        return -1;
    }

    if (inParam->auth->authMethod == ONVIF_AUTH_TYPE_AUTO)
    {
        inParam->auth->authMethod = ONVIF_AUTH_TYPE_TEXT;
        inParam->auth->userName = (char *)"(null)";
        inParam->auth->password = (char *)"ovfsZSJQZLHL";
    }


    pInParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if (pInParams)
    {
        Common_Json_SetAttrValue(pInParams, -1, "Header", Common_Json_Type_Object, NULL,
                                 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Method",
                                 Common_Json_Type_String, method, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Uri", Common_Json_Type_String,
                                 inParam->toUri, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth", Common_Json_Type_Object,
                                 NULL, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Method",
                                 Common_Json_Type_Number, NULL, inParam->auth->authMethod, 0);

        if (inParam->auth->authMethod == ONVIF_AUTH_TYPE_AUTO)
        {
            Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/UserName",
                                     Common_Json_Type_String, "(null)",
                                     0, 0);
            Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Password",
                                     Common_Json_Type_String, "ovfsZSJQZLHL",
                                     0, 0);
        }
        else
        {

            Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/UserName",
                                     Common_Json_Type_String,
                                     inParam->auth->userName ? inParam->auth->userName : "",
                                     0, 0);
            Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Password",
                                     Common_Json_Type_String,
                                     inParam->auth->password ? inParam->auth->password : "",
                                     0, 0);
        }
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest",
                                 Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Realm",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.realm ? inParam->auth->digest.realm : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Qop",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.qop ? inParam->auth->digest.qop : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Nonce",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.nonce ? inParam->auth->digest.nonce : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Cnonce",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.cnonce ? inParam->auth->digest.cnonce : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Uri",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.uri ? inParam->auth->digest.uri : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Response",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.response ? inParam->auth->digest.response : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Opaque",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.opaque ? inParam->auth->digest.opaque : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Nc",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.nc ? inParam->auth->digest.nc : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Digest/Method",
                                 Common_Json_Type_String,
                                 inParam->auth->digest.method ? inParam->auth->digest.method : "", 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/UsernameToken",
                                 Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/UsernameToken/Nonce",
                                 Common_Json_Type_String, inParam->auth->nonce ? inParam->auth->nonce : "", 0,
                                 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/UsernameToken/Created",
                                 Common_Json_Type_String, inParam->auth->created ? inParam->auth->created : "",
                                 0, 0);
        Common_Json_SetAttrValue(pInParams, -1,
                                 "Header/Auth/UsernameToken/PasswordDigest", Common_Json_Type_String,
                                 inParam->auth->passwordDigest ? inParam->auth->passwordDigest : "", 0, 0);

        Common_Json_SetAttrValue(pInParams, -1, "Header/IsRemote",
                                 Common_Json_Type_Number, NULL, 1, 0);

#if 0
        Common_Json_SetAttrValue(pInParams, -1, "Header/ClientInfo",
                                 Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/ClientInfo/IPv4",
                                 Common_Json_Type_String, InParam.IpAddr.Ipv4, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/ClientInfo/IPv6",
                                 Common_Json_Type_String, InParam.IpAddr.Ipv6, 0, 0);
        Common_Json_SetAttrValue(pInParams, -1, "Header/ClientInfo/MAC",
                                 Common_Json_Type_String, InParam.IpAddr.Mac, 0, 0);
#endif
        if (inParam->data)
        {
            Common_Json_AddItem(pInParams, -1, "Data", inParam->data);
            inParam->data = NULL;
        }
    }
    if (timeOut < 30000)
    {
        timeOut = 30000;
    }

    iRet = Access_CallFunctions(accessHandle, pInParams, &pOutResults, timeOut);
    // DumpJson(pInParams);
    // DumpJson(pOutResults);
    /* if (pInParams) */
    /* { */
    /*     Common_Json_Delete(pInParams); */
    /* } */
    /* pInParams = NULL; */
    if (pOutResults)
    {
        char *uri = NULL;
        int errCode = 0;
        char *decribe = NULL;
        cJSON_Struct *data = NULL;

        Common_Json_GetAttrValue(pOutResults, -1, "Header/Uri", NULL, &uri, NULL, NULL);
        Common_Json_GetAttrValue(pOutResults, -1, "Header/Code", NULL, NULL, &errCode,
                                 NULL);
        Common_Json_GetAttrValue(pOutResults, -1, "Header/Decribe", NULL, &decribe,
                                 NULL, NULL);
        if (errCode)
        {
            if (-2 == errCode)
            {
                LOGE("Model not Init, Try Again Later!\n");
                iRet = -2;
            }
            else
            {
                LOGE("Access_CallFunctions call failed!  error: uri:%s code:%d decribe:%s\n",
                     uri ? uri : "NULL", errCode, decribe ? decribe : "NULL");
                iRet = -1;
            }
        }

        if(outParam != NULL)
        {
            data = Common_Json_GetAttrValue(pOutResults, -1, "Data", NULL, NULL, NULL,
                                            NULL);
            if (NULL == data)
            {
                *outParam = NULL;
            }
            else
            {
                // DumpJson(pInParams);
                // DumpJson(data);
                *outParam = Common_Json_Duplicate(data, 1);
            }
        }
        Common_Json_Delete(pOutResults);
    }
    if (pInParams)
    {
        Common_Json_Delete(pInParams);
    }

    pOutResults = NULL;
    //Fault打印
    if (iRet != 0)
    {
        LOGE("\nRestMethod Failed![Method:%s Uri:%s]\n", method, inParam->toUri);
    }

    return iRet;
}

int RestOnvif_PasswordCheck(ONVIF_AUTH_INFO_T *authResult)
{
    return 0;
    #if 0
    int iRet = -1;
    cJSON_Struct *pResult = NULL;
    OVFS_REST_INPARAM_T inParam = {};

    inParam.method = REST_GET;
    inParam.toUri = (char *)"/Access/UserCfg";
    inParam.data = NULL;
    inParam.auth = authResult;

    iRet = RestOnvif_RestMethod(s_onvif_rest_ct.accessHandle, &inParam, &pResult,
                                0);
    Common_Json_Delete(pResult);
    pResult = NULL;
    if (-8 == iRet)
    {
        LOGE("Auth Failed!\n");
        return iRet;
    }

    return iRet;
    #endif
}


int RestOnvif_RequestCoreVersion(ONVIF_CORE_VERSION_T *coreVersion)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_DEVICE_INFO *devinfo = NULL;
    int cmd = THIRD_CMD_GET_DEVICE_INFO;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            devinfo = (THIRD_DEVICE_INFO *)outbuffer;

            if(devinfo->UseMask & 0x1)
            {
                coreVersion->IsOfDome = devinfo->IsOfDome;
            }

            if(devinfo->UseMask & 0x2)
            {
                coreVersion->LensSupport = devinfo->LensSupport;
            }

            if(devinfo->UseMask & 0x4)
            {
                coreVersion->IrisSupport = devinfo->IrisSupport;
            }

            if(devinfo->UseMask & 0x8)
            {
                snprintf(coreVersion->DeviceName, sizeof(coreVersion->DeviceName), "%s", devinfo->DeviceName);
            }

            if(devinfo->UseMask & 0x10)
            {
                snprintf(coreVersion->DeviceModel, sizeof(coreVersion->DeviceModel), "%s", devinfo->DeviceModel);
            }

            if(devinfo->UseMask & 0x20)
            {
                snprintf(coreVersion->DeviceType, sizeof(coreVersion->DeviceType), "%s", devinfo->DeviceType);
            }

            if(devinfo->UseMask & 0x40)
            {
                snprintf(coreVersion->SerialNumber, sizeof(coreVersion->SerialNumber), "%s", devinfo->SerialNumber);
            }

            if(devinfo->UseMask & 0x80)
            {
                snprintf(coreVersion->HardVersion, sizeof(coreVersion->HardVersion), "%s", devinfo->HardVersion);
            }

            if(devinfo->UseMask & 0x100)
            {
                snprintf(coreVersion->ProductDate, sizeof(coreVersion->ProductDate), "%s", devinfo->ProductDate);
            }

            if(devinfo->UseMask & 0x200)
            {
                snprintf(coreVersion->Manufacturer, sizeof(coreVersion->Manufacturer), "%s", devinfo->Manufacturer);
            }

            if(devinfo->UseMask & 0x400)
            {
                snprintf(coreVersion->Hardware, sizeof(coreVersion->Hardware), "%s", devinfo->Hardware);
            }

            if(devinfo->UseMask & 0x800)
            {
                snprintf(coreVersion->Country, sizeof(coreVersion->Country), "%s", devinfo->Country);
            }

            if(devinfo->UseMask & 0x1000)
            {
                snprintf(coreVersion->City, sizeof(coreVersion->City), "%s", devinfo->City);
            }
        }

    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_RequestGetTimeAll(ONVIF_TIME_ALL_T *timeAll)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_ALL_TIME *timeinfo = NULL;
    int cmd = THIRD_CMD_GET_ALL_TIME;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            timeinfo = (THIRD_ALL_TIME *)outbuffer;
            if(timeinfo->UseMask & 0x1)
            {
                timeAll->utc = *((ONVIF_TIME_T *)&timeinfo->UTCTime);
            }

            if(timeinfo->UseMask & 0x2)
            {
                timeAll->systime = *((ONVIF_TIME_T *)&timeinfo->SysTime);
            }

            if(timeinfo->UseMask & 0x4)
            {
                timeAll->ntp = *((ONVIF_TIME_NTP_T *)&timeinfo->Ntp);
            }

            if(timeinfo->UseMask & 0x8)
            {
                timeAll->dst = *((ONVIF_TIME_DST_T *)&timeinfo->Dst);
            }

            if(timeinfo->UseMask & 0x10)
            {
                timeAll->timezone = *((ONVIF_TIME_ZONE_T *)&timeinfo->TimeZone);
            }

        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_RequestNetworkAttr(ONVIF_NETWORK_ATTR_T *networkAttr)
{

    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_DNSCFG *dnscfg = NULL;
    int cmd = THIRD_CMD_GET_DNSCFG;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            dnscfg = (THIRD_DNSCFG *)outbuffer;
            if(dnscfg->UseMask & 0x1)
            {
                snprintf(networkAttr->Dns1V4,sizeof(networkAttr->Dns1V4),"%s", dnscfg->Dns1);
                snprintf(networkAttr->Dns2V4,sizeof(networkAttr->Dns2V4),"%s", dnscfg->Dns2);
            }
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_GetChanNameOsd(THIRD_OSD_NAME *nameosd)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_OSD_NAME *osdcfg = NULL;
    int cmd = THIRD_CMD_GET_OSD_NAME;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            osdcfg = (THIRD_OSD_NAME *)outbuffer;
            memcpy(nameosd,osdcfg,sizeof(THIRD_OSD_NAME));
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_GetMultiNameOsd(THIRD_OSD_NAME *multiosd)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_OSD_NAME *osdcfg = NULL;
    int cmd = THIRD_CMD_GET_OSD_MULTI;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            osdcfg = (THIRD_OSD_NAME *)outbuffer;
            memcpy(multiosd,osdcfg,sizeof(THIRD_OSD_NAME));
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_GetCustomOsd(cJSON_Struct **outJsonChannel, int custIdx)
{
    #if 0
    OVFS_REST_INPARAM_T inParamChannel = {};
    ONVIF_AUTH_INFO_T auth = {};
    int ret = 0;
    char uri[128] = {};
    snprintf(uri, sizeof(uri), "/BoardSys/Osd/SingleOsd%d/Attribute/All", custIdx);
    inParamChannel.toUri = uri;

    inParamChannel.method = REST_GET;
    inParamChannel.auth = &auth;
    inParamChannel.isRemote = 1;

    ret = RestOnvif_RestMethod(s_onvif_rest_ct.accessHandle,
                               &inParamChannel, outJsonChannel, 1000);

    if (ret != 0)
    {
        if (*outJsonChannel)
        {
            Common_Json_Delete(*outJsonChannel);
            *outJsonChannel = NULL;
        }
        LOGE("error\n");
        return -1;
    }
    #endif
    return 0;
}

int RestOnvif_GetDateTimeOsd(THIRD_OSD_TIME *timeosd)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_OSD_TIME *timeosdcfg = NULL;
    int cmd = THIRD_CMD_GET_OSD_TIME;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            timeosdcfg = (THIRD_OSD_TIME *)outbuffer;
            memcpy(timeosd, timeosdcfg,sizeof(THIRD_OSD_TIME));
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

/*NVR端没有制定配置具体的视频流，所以获取参数只获取主码流
  设置的时候，设置两个码流的参数
 */
int RestOnvif_RequestGetOsd(ONVIF_OSD_ATTR_T *osdAttr)
{
    cJSON_Struct *outJsonChannel = NULL, *outJsonTime = NULL;

    THIRD_OSD_NAME nameosd = {0};
    THIRD_OSD_TIME timeosd = {0};

    RestOnvif_GetChanNameOsd(&nameosd);
    RestOnvif_GetDateTimeOsd(&timeosd);

    snprintf(osdAttr->channelOsdAttr.osdString, sizeof(osdAttr->channelOsdAttr.osdString),
                 "%s", nameosd.Name);
    osdAttr->channelOsdAttr.osdEnable  = nameosd.Enable;
    osdAttr->channelOsdAttr.osdSize = nameosd.FontSize;
    osdAttr->channelOsdAttr.osdX = nameosd.X* 1.f / 512.f - 1.0;
    osdAttr->channelOsdAttr.osdY = -nameosd.Y* 1.f / 512.f + 1.0;

    osdAttr->datetimeOsdAttr.osdEnable = timeosd.Enable;
    osdAttr->datetimeOsdAttr.osdSize = timeosd.FontSize;
    osdAttr->datetimeOsdAttr.osdX = timeosd.X * 1.f / 512.f - 1.0;
    osdAttr->datetimeOsdAttr.osdY = -timeosd.Y * 1.f / 512.f + 1.0;
    osdAttr->datetimeOsdAttr.osdDataFormat = &s_osd_date_map[timeosd.DateFormat];
    osdAttr->datetimeOsdAttr.osdTimeFormat = &s_osd_hour_map[timeosd.TimeFormat];

    return 0;
}

int RestOnvif_RequestGetMultiOsd(ONVIF_OSD_ATTR_T *osdAttr)
{

    THIRD_OSD_NAME multiosd = {0};

    RestOnvif_GetMultiNameOsd(&multiosd);

    snprintf(osdAttr->multiOsdAttr.osdString, sizeof(osdAttr->multiOsdAttr.osdString),
             "%s", multiosd.Name);

    osdAttr->multiOsdAttr.osdEnable = multiosd.Enable;
    osdAttr->multiOsdAttr.osdX = multiosd.X * 1.f / 512.f - 1.0;
    osdAttr->multiOsdAttr.osdY = -multiosd.Y * 1.f / 512.f + 1.0;
    osdAttr->multiOsdAttr.osdSize = multiosd.FontSize;

    return 0;
}

int RestOnvif_RequestGetCustomOsd(ONVIF_OSD_ATTR_T *osdAttr)
{
#if (defined ONVIF_EXT_CUSTOM_OSD)
    osdAttr->custOsdAttr[0].osdEnable =
        osdAttr->custOsdAttr[1].osdEnable =
            osdAttr->custOsdAttr[2].osdEnable = 1;

    int cnt = COMMON_ARRAY_ELEMENT_COUNT(osdAttr->custOsdAttr);
    int i = 0;

    for (i = 0; i < cnt; i++)
    {
        cJSON_Struct *outJsonChannel = NULL;

        RestOnvif_GetCustomOsd(&outJsonChannel, i);

        char *vstr = NULL;
        int vint = 0;
        char tmpStr[32] = {};
        snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/String", i);
        Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, &vstr, NULL, NULL);
        if (vstr)
            snprintf(osdAttr->custOsdAttr[i].osdString,
                     sizeof(osdAttr->custOsdAttr[i].osdString), "%s", vstr);

        LOGE("osd %d %s\n", i, osdAttr->custOsdAttr[i].osdString);
        snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/Enable", i);
        Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, NULL, &vint, NULL);
        osdAttr->custOsdAttr[i].osdEnable = vint;

        snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/X", i);
        Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, NULL, &vint, NULL);
        osdAttr->custOsdAttr[i].osdX = vint * 1.f / 512.f - 1.0;

        snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/Y", i);
        Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, NULL, &vint, NULL);
        osdAttr->custOsdAttr[i].osdY = -vint * 1.f / 512.f + 1.0;

        snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/Size", i);
        Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, NULL, &vint, NULL);
        osdAttr->custOsdAttr[i].osdSize = vint;

        snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/UseRemoteBm", i);
        Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, NULL, &vint, NULL);
        if (vint == 1)
        {
            snprintf(tmpStr, sizeof(tmpStr), "SingleOsd%dList/BitMapSize", i);
            Common_Json_GetAttrValue(outJsonChannel, 0, tmpStr, NULL, NULL, &vint, NULL);
            osdAttr->custOsdAttr[i].osdSize = vint;
        }

        // DumpJson(outJsonChannel);
        if (outJsonChannel)
            Common_Json_Delete(outJsonChannel);

        ///BoardSys/Osd/SingleOsd0/Attribute/All
    }
#endif
    return 0;
}

int RestOnvif_RequestDeleteOsd(ONVIF_OSD_ATTR_T *osdAttr)
{
    int ret = 0;
    THIRD_OSD_NAME nameosd = {0}, multiosd = {0};
    THIRD_OSD_TIME timeosd = {0};

    if (osdAttr->channelOsdAttr.osdEnable != -1)
    {
        nameosd.Cmd = THIRD_CMD_SET_OSD_NAME;
        nameosd.UseMask = 0x1;
        nameosd.Enable = osdAttr->channelOsdAttr.osdEnable;
        ret = s_onvif_ct.fCallFxn(NULL,&nameosd,sizeof(nameosd),NULL,NULL,1000);

    }

    if (osdAttr->datetimeOsdAttr.osdEnable != -1)
    {
        timeosd.Cmd = THIRD_CMD_SET_OSD_TIME;
        timeosd.UseMask = 0x1;
        timeosd.Enable = osdAttr->datetimeOsdAttr.osdEnable;
        ret = s_onvif_ct.fCallFxn(NULL,&timeosd,sizeof(timeosd),NULL,NULL,1000);

    }

    if (osdAttr->multiOsdAttr.osdEnable != -1)
    {
        multiosd.Cmd = THIRD_CMD_SET_OSD_MULTI;
        multiosd.UseMask = 0x1;
        multiosd.Enable = osdAttr->multiOsdAttr.osdEnable;
        ret = s_onvif_ct.fCallFxn(NULL,&multiosd,sizeof(multiosd),NULL,NULL,1000);
    }

    return ret;
}

int RestOnvif_RequestSetMultiOsd(ONVIF_OSD_ATTR_T *osdAttr)
{
    int ret = 0;
    THIRD_OSD_NAME multiosd = {0};
    multiosd.Cmd = THIRD_CMD_SET_OSD_MULTI;
    multiosd.UseMask = 0xff;
    multiosd.Enable = osdAttr->multiOsdAttr.osdEnable;
    multiosd.X = (512.f * (osdAttr->multiOsdAttr.osdX + 1.0));
    multiosd.Y = (512.f * (1.0 - osdAttr->multiOsdAttr.osdY));
    multiosd.FontSize = osdAttr->multiOsdAttr.osdSize;
    snprintf(multiosd.Name, sizeof(multiosd.Name),
                     "%s", osdAttr->multiOsdAttr.osdString);

    ret = s_onvif_ct.fCallFxn(NULL,&multiosd,sizeof(multiosd),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestSetCustomOsd(ONVIF_OSD_ATTR_T *osdAttr)
{
#if (defined ONVIF_EXT_CUSTOM_OSD)
    /*force custom osd enable*/

    OVFS_REST_INPARAM_T inParamChannel = {};
    cJSON_Struct *outJsonChannel = NULL;
    ONVIF_AUTH_INFO_T auth = {};
    int ret = 0 , k = 0;
    int custCnt = COMMON_ARRAY_DIM(osdAttr->custOsdAttr);

    for (k = 0; k < custCnt; k++)
    {
        if (osdAttr->custOsdAttr[k].osdEnable != 1)
            continue;

        RestOnvif_GetCustomOsd(&outJsonChannel, k);

        // DumpJson(outJsonChannel);
        char listName[32] = {};
        int i = 0;
        snprintf(listName, sizeof(listName), "SingleOsd%dList", k);
        cJSON_Struct *nameList = Common_Json_GetItem(outJsonChannel, -1, listName);
        int cnt = Common_Json_ArraySize(nameList);

        for (i = 0; i < cnt; i++)
        {
            Common_Json_SetAttrValue(nameList, i, "String",
                                     Common_Json_Type_String,
                                     osdAttr->custOsdAttr[k].osdString, 0, 0);

            Common_Json_SetAttrValue(nameList, i, "Enable",
                                     Common_Json_Type_Number,
                                     NULL, osdAttr->custOsdAttr[k].osdEnable, 0);

            Common_Json_SetAttrValue(nameList, i, "X",
                                     Common_Json_Type_Number,
                                     NULL, (int)(512.f * (osdAttr->custOsdAttr[k].osdX + 1.0)), 0);

            Common_Json_SetAttrValue(nameList, i, "Y",
                                     Common_Json_Type_Number,
                                     NULL, (int)(512.f * (1.0 - osdAttr->custOsdAttr[k].osdY)), 0);

            Common_Json_SetAttrValue(nameList, i , "UseRemoteBm",
                                     Common_Json_Type_Number,
                                     NULL, 0, 0);

            /*only set main stream osd font size*/
            // if (i  == 0)
            // {
            //     Common_Json_SetAttrValue(nameList, i , "Size",
            //                              Common_Json_Type_Number,
            //                              NULL, osdAttr->custOsdAttr[k].osdSize, 0);
            // }

            // Common_Json_SetAttrValue(nameList, i , "BitMapArr",
            //                          Common_Json_Type_String,
            //                          NULL, 0, 0);

            // Common_Json_SetAttrValue(nameList, i , "BitMapSize",
            //                          Common_Json_Type_Number,
            //                          NULL, 0, 0);
        }

        char uri[128] = {};
        snprintf(uri, sizeof(uri), "/BoardSys/Osd/SingleOsd%d/Attribute/All?CommCfgForceNoSaveCfg=1", k);
        inParamChannel.toUri = uri;
        inParamChannel.method = REST_PUT;
        inParamChannel.auth = &auth;
        inParamChannel.isRemote = 1;
        inParamChannel.data = outJsonChannel;
        // DumpJson(outJsonChannel);
        ret = RestOnvif_RestMethod(s_onvif_rest_ct.accessHandle,
                                   &inParamChannel, NULL, 1000);

        if (ret != 0)
        {
            LOGE("request set osd error\n");
        }
    }

#endif

    return 0;
}

int RestOnvif_RequestSetOsd(ONVIF_OSD_ATTR_T *osdAttr)
{
    int ret = 0;
    THIRD_OSD_NAME nameosd = {0};
    THIRD_OSD_TIME timeosd = {0};

    if (osdAttr->channelOsdAttr.osdEnable != -1)
    {
        LOGD("channelOsdAttr [%f][%f]\n",osdAttr->channelOsdAttr.osdX,osdAttr->channelOsdAttr.osdY);
        nameosd.Cmd = THIRD_CMD_SET_OSD_NAME;
        nameosd.UseMask = 0xff;
        nameosd.Enable = osdAttr->channelOsdAttr.osdEnable;
        nameosd.X = (512.f * (osdAttr->channelOsdAttr.osdX + 1.0));
        nameosd.Y = (512.f * (1.0 - osdAttr->channelOsdAttr.osdY));
        nameosd.FontSize = osdAttr->channelOsdAttr.osdSize;
        snprintf(nameosd.Name, sizeof(nameosd.Name),
                     "%s", osdAttr->channelOsdAttr.osdString);

        ret = s_onvif_ct.fCallFxn(NULL,&nameosd,sizeof(nameosd),NULL,NULL,1000);
    }

    if (osdAttr->datetimeOsdAttr.osdEnable != -1)
    {
        LOGD("datetimeOsdAttr [%f][%f]\n",osdAttr->datetimeOsdAttr.osdX,osdAttr->datetimeOsdAttr.osdY);
        timeosd.Cmd = THIRD_CMD_SET_OSD_TIME;
        timeosd.UseMask = 0xff;
        timeosd.Enable = osdAttr->datetimeOsdAttr.osdEnable;
        timeosd.X = (512.f * (osdAttr->datetimeOsdAttr.osdX + 1.0));
        timeosd.Y = (512.f * (1.0 - osdAttr->datetimeOsdAttr.osdY));
        timeosd.FontSize = osdAttr->datetimeOsdAttr.osdSize;
        timeosd.DateFormat = osdAttr->datetimeOsdAttr.osdDataFormat->formatId;
        timeosd.TimeFormat = osdAttr->datetimeOsdAttr.osdTimeFormat->formatId;

        ret = s_onvif_ct.fCallFxn(NULL,&timeosd,sizeof(timeosd),NULL,NULL,1000);
    }

    if (osdAttr->multiOsdAttr.osdEnable != -1)
    {
        RestOnvif_RequestSetMultiOsd(osdAttr);
    }

    return ret;
}


int RestOnvif_RequestImageAttr(ONVIF_IMAGE_SET_T *image)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_IMAGE *imagecfg = NULL;
    int cmd = THIRD_CMD_GET_IMAGE;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            imagecfg = (THIRD_IMAGE *)outbuffer;

            image->brightness = imagecfg->Brightness;
            image->contrast = imagecfg->Contrast;
            image->saturation = imagecfg->Staturation;
            image->sharpness = imagecfg->Sharpness;
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_SetImageAttr(int iChannel, ONVIF_IMAGE_SET_T *image)
{
    int ret = 0;
    THIRD_IMAGE imagecfg = {0};
    imagecfg.Cmd = THIRD_CMD_SET_IMAGE;
    imagecfg.UseMask = 0x17;
    imagecfg.Brightness = image->brightness;
    imagecfg.Contrast = image->contrast;
    imagecfg.Staturation = image->saturation;
    imagecfg.Sharpness = image->sharpness;

    ret = s_onvif_ct.fCallFxn(NULL,&imagecfg,sizeof(imagecfg),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestVideoEncoderCapability(int devNo, int channelNo,
        int streamNo, ONVIF_VideoEncoderCapability_T *video)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_VIDEO_ENC_ABILITY inbuffer = {0}, *ability = NULL;
    inbuffer.Cmd = THIRD_CMD_GET_VIDEO_VENC_ABILITY;
    inbuffer.Stream = streamNo;

    ret = s_onvif_ct.fCallFxn(NULL,&inbuffer,sizeof(inbuffer),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            ability = (THIRD_VIDEO_ENC_ABILITY *)outbuffer;

            LOGD("usemask:[%d] resolution count:[%d]\n",ability->UseMask,ability->ResolutionCount);

            int i = 0;
            for(i=0; i<ability->ResolutionCount; i++)
            {
                video->resolution[i][0] = ability->pResolutionAbility[i].Width;
                video->resolution[i][1] = ability->pResolutionAbility[i].Height;
                video->resolution[i][2] = ability->pResolutionAbility[i].Fps;

                LOGD("resolution[%d]:[%d][%d][%d]\n",i,ability->pResolutionAbility[i].Width,
                    ability->pResolutionAbility[i].Height,ability->pResolutionAbility[i].Fps);
            }

            LOGD("Enctype count:[%d]\n",ability->EncTypeCount);
            for(i=0; i<ability->EncTypeCount; i++)
            {
                if(Common_StriCmp(ability->EncTypeAbility[i], "H265+")!=0)
                {
                    snprintf(video->encType[i], sizeof(video->encType[i]), "%s", ability->EncTypeAbility[i]);
                }
                LOGD("EncType[%d]:[%s]\n",i,ability->EncTypeAbility[i]);
            }

            LOGD("bitratetype count:[%d]\n",ability->BitrateTypeCount);

            LOGD("profiles count:[%d]\n",ability->ProfilesCount);
            for(i=0; i<ability->ProfilesCount; i++)
            {
                snprintf(video->profiles[i], sizeof(video->profiles[i]), "%s", ability->ProfilesAbility[i]);
                LOGD("Profiles[%d]:[%s]\n",i,ability->ProfilesAbility[i]);
            }

            video->fpsRange[0] = ability->FpsRange[0];
            video->fpsRange[1] = ability->FpsRange[1];

            LOGD("fpsRange:[%d][%d]\n",ability->FpsRange[0],ability->FpsRange[1]);

            video->bitrateRange[0] = ability->BitrateRange[0];
            video->bitrateRange[1] = ability->BitrateRange[1];

            LOGD("bitrateRange:[%d][%d]\n",ability->BitrateRange[0],ability->BitrateRange[1]);

            video->Iinterval[0] = ability->Iinterval[0];
            video->Iinterval[1] = ability->Iinterval[1];
            LOGD("Iinterval:[%d][%d]\n",ability->Iinterval[0],ability->Iinterval[1]);

            g_encQuality[0] = video->encQuality[0] = ability->EncQuality[0];
            g_encQuality[1] = video->encQuality[1] = ability->EncQuality[1];
            LOGD("encQuality:[%d][%d]\n",ability->EncQuality[0],ability->EncQuality[1]);

            ONVIF_FREE(ability->pResolutionAbility);
            ONVIF_FREE(ability->EncTypeAbility);
            ONVIF_FREE(ability->BitrateTypeAbility);
            ONVIF_FREE(ability->ProfilesAbility);
            ONVIF_FREE(outbuffer);

        }
    }

    if (memcmp(video, &s_onvif_rest_ct.videoAbility[streamNo],
               sizeof(ONVIF_VideoEncoderCapability_T)) != 0)
    {
        memcpy(&s_onvif_rest_ct.videoAbility[streamNo], video,
               sizeof(ONVIF_VideoEncoderCapability_T));
    }

    return ret;

}

int RestOnvif_GetVideoEncoderCfg(int devNo, int channelNo, int streamNo,
                                 ONVIF_VideoEncoderCfg_T *video)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_VIDEO_ENC inbuffer = {0}, *videoencode = NULL;
    inbuffer.Cmd = THIRD_CMD_GET_VIDEO_VENC;
    inbuffer.Stream = streamNo;

    ret = s_onvif_ct.fCallFxn(NULL,&inbuffer,sizeof(inbuffer),&outbuffer,&outsize,1000);
    if(ret == 0)
    {

        if(outbuffer && outsize > 0)
        {
            videoencode = (THIRD_VIDEO_ENC *)outbuffer;
            video->isValid = 1;
            video->resolution[0] = videoencode->Width;
            video->resolution[1] = videoencode->Height;
            video->resolution[2] = videoencode->Fps;
            LOGD("W:[%d] H:[%d] Fps:[%d]\n",video->resolution[0],video->resolution[1],video->resolution[2]);
            if(video->resolution[2] == 0)
            {
                ONVIF_VideoEncoderCapability_T enccfg = {0};
                RestOnvif_RequestVideoEncoderCapability(0, 0,streamNo, &enccfg);

                int i = 0;
                for(i=0; i< sizeof(enccfg.resolution); i++)
                {
                    if(enccfg.resolution[i][0] == 0 && enccfg.resolution[i][1] == 0)
                    {
                        break;
                    }

                    if(enccfg.resolution[i][0] == video->resolution[0] &&
                        enccfg.resolution[i][1] == video->resolution[1])
                    {
                        video->resolution[2] = enccfg.resolution[i][2];
                        break;
                    }
                }

            }

            video->encQuality = videoencode->Quality;

            /*图像质量转换 onvif 对外输出范围 0 － 5*/
            video->encQuality = (g_encQuality[1] + g_encQuality[0] - video->encQuality) ;
            if (video->encQuality < g_encQuality[0] || video->encQuality > g_encQuality[1])
                video->encQuality = (g_encQuality[1] + g_encQuality[0])/2;

            video->Iinterval = videoencode->Iinterval;
            video->bitrateCtrlMode = videoencode->BitrateCtrlMode;
            video->encodeFormat = videoencode->EncodeFormat;
            video->profiles = videoencode->Profiles;
            video->bitrate= videoencode->Bitrate;
        }

    }

    Common_cJSON_free(outbuffer);

    return ret;

}

const ONVIF_OSD_MAP_T *RestOnvif_ParserDate(char *formartStr, int formartId)
{
    int i = 0, cnt = COMMON_ARRAY_DIM(s_osd_date_map);

    if (formartStr != NULL)
    {
        for (i = 0; i < cnt; i++)
        {
            if (strcmp(formartStr, s_osd_date_map[i].formatStr) == 0)
                return &s_osd_date_map[i];
        }
    }
    else
    {
        if (formartId >= 0 && formartId < cnt)
            return &s_osd_date_map[formartId];
    }

    return NULL;
}

const ONVIF_OSD_MAP_T *RestOnvif_ParserHour(char *formatStr, int formartId)
{
    int i = 0, cnt = COMMON_ARRAY_DIM(s_osd_hour_map);

    if (formatStr != NULL)
    {
        for (i = 0; i < cnt; i++)
        {
            if (strstr(s_osd_hour_map[i].formatStr, formatStr) != NULL)
                return &s_osd_hour_map[i];
        }
    }
    else
    {
        if (formartId >= 0 && formartId < cnt)
            return &s_osd_hour_map[formartId];
    }

    return NULL;
}

int RestOnvif_RequestWebServer(ONVIF_WEB_ATTR_T *webAttr)
{
    LOGD("RestOnvif_RequestWebServer!\n");
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_HTTPPORT *httpport_out = NULL;
    int Cmd = THIRD_CMD_GET_HTTPPORT;

    ret = s_onvif_ct.fCallFxn(NULL,&Cmd,sizeof(Cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            httpport_out = (THIRD_HTTPPORT *)outbuffer;
            if(httpport_out->UseMask & 0x1)
            {
                webAttr->httpPort = httpport_out->HttpPort;
            }

            if(httpport_out->UseMask & 0x2)
            {
                webAttr->httpsPort = httpport_out->HttpsPort;
            }
        }

    }

    Common_cJSON_free(outbuffer);

    return ret;

}

int RestOnvif_SetVideoEncoderCfg(int devNo, int channelNo, int streamNo,
                                 ONVIF_VideoEncoderCfg_T *video)
{
    int ret = 0;
    THIRD_VIDEO_ENC inbuffer = {0};
    inbuffer.Cmd = THIRD_CMD_SET_VIDEO_VENC;
    inbuffer.Stream = streamNo;
    inbuffer.UseMask = 0x1ff;
    inbuffer.Width = video->resolution[0];
    inbuffer.Height = video->resolution[1];
    inbuffer.Fps = video->resolution[2];

    inbuffer.Quality = video->encQuality;

    /*图像质量转换 onvif 对外输出范围 0 － 5*/
    inbuffer.Quality = (g_encQuality[1] + g_encQuality[0] - inbuffer.Quality) ;
    if (inbuffer.Quality < g_encQuality[0] || inbuffer.Quality > g_encQuality[1])
        inbuffer.Quality = (g_encQuality[1] + g_encQuality[0])/2;

    inbuffer.Iinterval= video->Iinterval;
    inbuffer.BitrateCtrlMode = video->bitrateCtrlMode;
    inbuffer.EncodeFormat = video->encodeFormat;
    inbuffer.Profiles = video->profiles;
    inbuffer.Bitrate = video->bitrate;

    ret = s_onvif_ct.fCallFxn(NULL,&inbuffer,sizeof(inbuffer),NULL,NULL,1000);

    return ret;

}

int RestOnvif_GetMultiCastCfg(int devNo, int channelNo, int streamNo,
                              ONVIF_MultiCast_T *pMultiCast)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_RTSPPORT *rtspcfg = NULL;
    int cmd = THIRD_CMD_GET_RTSPPORT;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            rtspcfg = (THIRD_RTSPPORT *)outbuffer;

            pMultiCast->bEnable = rtspcfg->MulcastEnable;
            pMultiCast->rtspPort = rtspcfg->RtspPort;
            pMultiCast->httpPort = 8002;

            if(streamNo == 0)
            {
                snprintf(pMultiCast->ipV4, sizeof(pMultiCast->ipV4), "%s", rtspcfg->MainVideo.Ip);
                pMultiCast->port = rtspcfg->MainVideo.Port;
                pMultiCast->ttl = rtspcfg->MainVideo.TTL;
            }
            else if(streamNo == 1)
            {
                snprintf(pMultiCast->ipV4, sizeof(pMultiCast->ipV4), "%s", rtspcfg->SubVideo.Ip);
                pMultiCast->port = rtspcfg->SubVideo.Port;
                pMultiCast->ttl = rtspcfg->SubVideo.TTL;
            }
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_SetMultiCastCfg(int devNo, int channelNo, int streamNo,
                              ONVIF_MultiCast_T *pMultiCast)
{
    int ret = 0;
    THIRD_RTSPPORT rtspcfg = {0};
    rtspcfg.Cmd = THIRD_CMD_SET_RTSPPORT;
    rtspcfg.MulcastEnable = pMultiCast->bEnable;
    if(streamNo == 0)
    {
        snprintf(rtspcfg.MainVideo.Ip, sizeof(rtspcfg.MainVideo.Ip), "%s", pMultiCast->ipV4);
        rtspcfg.MainVideo.Port = pMultiCast->port;
        rtspcfg.MainVideo.TTL = pMultiCast->ttl;
        rtspcfg.UseMask = 0x18;
    }
    else if(streamNo == 1)
    {
        snprintf(rtspcfg.SubVideo.Ip, sizeof(rtspcfg.SubVideo.Ip), "%s", pMultiCast->ipV4);
        rtspcfg.SubVideo.Port = pMultiCast->port;
        rtspcfg.SubVideo.TTL = pMultiCast->ttl;
        rtspcfg.UseMask = 0x28;
    }

    ret = s_onvif_ct.fCallFxn(NULL,&rtspcfg,sizeof(rtspcfg),NULL,NULL,1000);

    return ret;

}

int RestOnvif_RequestViAttr(ONVIF_VI_ATTR_T *viAttr)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_RESOLUTION_ABILITY *vicfg = NULL;
    int cmd = THIRD_CMD_GET_VIDEO_SOURCE;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            vicfg = (THIRD_RESOLUTION_ABILITY *)outbuffer;
            viAttr->Width = vicfg->Width;
            viAttr->Height = vicfg->Height;
            viAttr->Fps = vicfg->Fps;
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_RequesNetInfo(ONVIF_NET_INFO_T *pNetInfo)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_ETHCFG *netinfo = NULL;
    int cmd = 0x0021;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            netinfo = (THIRD_ETHCFG *)outbuffer;

            snprintf(pNetInfo->ethName, sizeof(pNetInfo->ethName), "eth0");
            if(netinfo->UseMask & 0x1)
            {
                pNetInfo->bDhcp = netinfo->Dhcp;
            }

            if(netinfo->UseMask & 0x2)
            {
                snprintf(pNetInfo->ipV4, sizeof(pNetInfo->ipV4), "%s", netinfo->Ip);
            }

            if(netinfo->UseMask & 0x4)
            {
                snprintf(pNetInfo->ipMaskV4, sizeof(pNetInfo->ipMaskV4), "%s", netinfo->Mask);
            }

            if(netinfo->UseMask & 0x8)
            {
                snprintf(pNetInfo->gateWayV4, sizeof(pNetInfo->gateWayV4), "%s", netinfo->Gateway);
            }

            if(netinfo->UseMask & 0x10)
            {
                snprintf(pNetInfo->mac, sizeof(pNetInfo->mac), "%s", netinfo->Mac);
            }
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_SetNetInfo(ONVIF_NET_INFO_T *pNetInfo)
{
    int ret = 0;
    THIRD_ETHCFG netinfo = {0};
    netinfo.Cmd = THIRD_CMD_SET_ETHCFG;
    netinfo.UseMask = 0xf;
    netinfo.Dhcp = pNetInfo->bDhcp;
    Common_Strcpy(netinfo.Ip, pNetInfo->ipV4);
    Common_Strcpy(netinfo.Mask, pNetInfo->ipMaskV4);
    Common_Strcpy(netinfo.Gateway, pNetInfo->gateWayV4);

    ret = s_onvif_ct.fCallFxn(NULL,&netinfo,sizeof(THIRD_ETHCFG),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestMotionAttr(ONVIF_MOTION_ATTR_T *pMotionAttr)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_MOTION_CFG *motioncfg = NULL;
    int Cmd = THIRD_CMD_GET_MOTION_CFG;

    ret = s_onvif_ct.fCallFxn(NULL,&Cmd,sizeof(Cmd),&outbuffer,&outsize,1000);
    if(ret == 0)
    {

        if(outbuffer && outsize > 0)
        {
            motioncfg = (THIRD_MOTION_CFG *)outbuffer;
            pMotionAttr->enable = motioncfg->Enable;
            pMotionAttr->Sensitivity = motioncfg->Sensitivity;
            pMotionAttr->blockW = motioncfg->BlockW;
            pMotionAttr->blockH = motioncfg->BlockH;

            snprintf(pMotionAttr->rect,sizeof(pMotionAttr->rect),"%s",motioncfg->RectStr);
        }

    }

    Common_cJSON_free(outbuffer);

    return ret;

}

int RestOnvif_SetMotionAttr(ONVIF_MOTION_ATTR_T *pMotionAttr)
{
    int ret = 0;
    THIRD_MOTION_CFG motioncfg = {0};
    motioncfg.Cmd = THIRD_CMD_SET_MOTION_CFG;
    motioncfg.UseMask = 0xff;
    motioncfg.Enable = pMotionAttr->enable;
    motioncfg.Sensitivity = pMotionAttr->Sensitivity;
    snprintf(motioncfg.RectStr,sizeof(motioncfg.RectStr),"%s",pMotionAttr->rect);

    ret = s_onvif_ct.fCallFxn(NULL,&motioncfg,sizeof(motioncfg),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestSetTimeAll(ONVIF_TIME_ALL_T *timeAll)
{
    int ret = 0;
    THIRD_ALL_TIME timecfg = {0};
    timecfg.Cmd = THIRD_CMD_SET_ALL_TIME;
    timecfg.UseMask = 0x1d;
    timecfg.UTCTime = *((THIRD_TIME_CFG *)&timeAll->utc);
    timecfg.Ntp = *((THIRD_NTP_CFG *)&timeAll->ntp);
    timecfg.Dst = *((THIRD_DST_CFG *)&timeAll->dst);
    timecfg.TimeZone = *((THIRD_TIME_ZONE *)&timeAll->timezone);

    ret = s_onvif_ct.fCallFxn(NULL,&timecfg,sizeof(timecfg),NULL,NULL,1000);

    return ret;
}

int RestOnvif_IsDefaultPassword()
{
    return 1;
    #if 0
    cJSON_Struct *pInParams = NULL, *pOutParams = NULL,
                  *pArray = NULL, *pArray2 = NULL;
    int nIsDefault = 0;
    char *szDefault = NULL, *szCurr = NULL;

    unsigned long long int nCurrCount = Common_GetSystemCount64();

    if(  s_onvif_rest_ct.lastDefPasscheckTime + 20000LLU > nCurrCount
            && s_onvif_rest_ct.lastDefPasscheckTime != 0LLU)
    {
        return s_onvif_rest_ct.isDefPass;
    }

    s_onvif_rest_ct.lastDefPasscheckTime = nCurrCount;
    pInParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    if(pInParams == NULL)
        return -1;

    Common_Json_SetAttrValue(pInParams, -1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(pInParams, -1, "Header/Uri", Common_Json_Type_String,
                             "/Access/Usercfg", 0, 0);
    Common_Json_SetAttrValue(pInParams, -1, "Header/Method", Common_Json_Type_String, "Get", 0, 0);

    Common_Json_SetAttrValue(pInParams, -1, "Header/Auth", Common_Json_Type_Object, NULL, 0, 0);

    Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Method", Common_Json_Type_Number, NULL, 1, 0);
    Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/UserName", Common_Json_Type_String, "(null)",
                             0, 0);
    Common_Json_SetAttrValue(pInParams, -1, "Header/Auth/Password", Common_Json_Type_String,
                             "ovfsZSJQZLHL", 0,
                             0);

    Access_CallFunctions(s_onvif_rest_ct.accessHandle, pInParams, &pOutParams, 3000);

    if (pOutParams != NULL)
        pArray = Common_Json_GetItem(pOutParams, -1, "Data/ResList");

    if (pArray != NULL)
    {
        Common_Json_GetAttrValue(pArray, 0, "DefaultPwd", NULL, &szDefault, NULL, NULL);
        pArray2 = Common_Json_GetItem(pArray, 0, "Password");
    }

    if(pArray2 != NULL)
    {
        Common_Json_GetAttrValue(pArray2, 0, NULL, NULL, &szCurr, NULL, NULL);

        if(szDefault == NULL && szCurr == NULL)
        {
            nIsDefault = 1;
        }
        else if(szDefault != NULL && szCurr != NULL && 0 == Common_StrCmp(szDefault, szCurr))
        {
            nIsDefault = 1;
        }
    }

    if(pOutParams != NULL)
        Common_Json_Delete(pOutParams);

    Common_Json_Delete(pInParams);
    s_onvif_rest_ct.isDefPass = nIsDefault;
    return nIsDefault;
    #endif
}

int RestOnvif_IsDefaultIp()
{
    char curIp[32] = { 0 };
    char *valueP = NULL;
    if (strlen(s_onvif_rest_ct.defIpAddr) < 4)
    {
        FILE *fp = fopen("/root/res/default/NetWork.json", "rb");
        if (fp == NULL)
        {
            LOGE("open network default json failed %s\n", strerror(errno));
            return -1;
        }
        fseek(fp, 0 , SEEK_END);
        int jSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *jData = NULL;
        if (jSize > 0)
            jData = (char *)ONVIF_MALLOC(jSize);

        if (jData != NULL)
            fread(jData, 1, jSize, fp);

        fclose(fp);
        if (jData == NULL)
        {
            LOGE("malloc failed\n");
            return -1;
        }

        cJSON_Struct *defNetJson = Common_Json_Parse(jData, NULL, NULL);
        if (defNetJson == NULL)
        {
            LOGE("load network default json failed\n");
            ONVIF_FREE(jData);
            return -1;
        }
        Common_Json_GetAttrValue(defNetJson, -1, "NetAttr/Eth/0/IpAddrV4", NULL, &valueP, NULL, NULL);
        if (valueP != NULL)
        {
            snprintf(s_onvif_rest_ct.defIpAddr, sizeof(s_onvif_rest_ct.defIpAddr), "%s", valueP);
        }
        ONVIF_FREE(jData);
        Common_Json_Delete(defNetJson);
    }


    if (Utils_GetNetDevInfo((char *)"eth0", NULL, curIp, NULL) < 0)
    {
        LOGE("get ip failed\n");
        return -1;
    }

    if (strcmp(s_onvif_rest_ct.defIpAddr, curIp) == 0)
        return 1;

    return 0;
}

int RestOnvif_RequestUri(char *uri,OVFS_RESTMETHOD_E method)
{
	OVFS_REST_INPARAM_T inParam = {};
	ONVIF_AUTH_INFO_T auth = {};
	cJSON_Struct *pOutParams = NULL;

	if(uri == NULL) return -1;
	if(strlen(uri) == 0) return -2;

	LOGD("method=%d,uri=%s\n",method,uri);

	inParam.toUri = uri;
    inParam.method = method;
    inParam.auth = &auth;
    inParam.isRemote = 1;
	inParam.data = NULL;

	int ret = RestOnvif_RestMethod(s_onvif_rest_ct.accessHandle, &inParam, &pOutParams, 3000);

	if (pOutParams != NULL)
		Common_Json_Delete(pOutParams);
	return ret;
}

int RestOnvif_RequestGetPresets(ONVIF_PTZ_PRESET_INFO_T **presetsAttr,int *presetsNum)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_PRESET_CFG *presetcfg = NULL;
    int Cmd = THIRD_CMD_GET_PRESET;

    ret = s_onvif_ct.fCallFxn(NULL,&Cmd,sizeof(Cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            presetcfg = (THIRD_PRESET_CFG *)outbuffer;

            *presetsNum = presetcfg->PresetNum;
            LOGW("presetsNum:[%d]\n",*presetsNum);
            *presetsAttr = (ONVIF_PTZ_PRESET_INFO_T *)calloc(*presetsNum,sizeof(ONVIF_PTZ_PRESET_INFO_T));
            int i = 0;
            for (i = 0; i < presetcfg->PresetNum; i++)
            {
                snprintf((*presetsAttr+i)->token, sizeof((*presetsAttr+i)->token), "%d", presetcfg->PresetArray[i]);
                snprintf((*presetsAttr+i)->name, sizeof((*presetsAttr+i)->name), "Preset%03d", presetcfg->PresetArray[i]);
            }
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_RequestSetPreset(int *presetsNum)
{
    LOGD("RequestSetPreset:[%d]\n",*presetsNum);
	int ret = 0;
    THIRD_PRESET_CFG presetcfg = {0};
    presetcfg.Cmd = THIRD_CMD_SET_PRESET;
    presetcfg.UseMask = 0xf;
    presetcfg.PresetNum = 1;
    presetcfg.PresetArray[0] = *presetsNum;
    LOGD("RequestSetPreset fCallFxn:[%d]\n",*presetsNum);

    ret = s_onvif_ct.fCallFxn(NULL,&presetcfg,sizeof(presetcfg),NULL,NULL,1000);

    LOGW("presetsNum:[%d][%d]\n",presetcfg.PresetArray[0],*presetsNum);
    *presetsNum = presetcfg.PresetArray[0];

    return ret;
}

int RestOnvif_RequestGotoPreset(int presetsNum)
{
	int ret = 0;
    THIRD_PRESET_CFG presetcfg = {0};
    presetcfg.Cmd = THIRD_CMD_GOTO_PRESET;
    presetcfg.UseMask = 0xf;
    presetcfg.PresetNum = 1;
    presetcfg.PresetArray[0] = presetsNum;

    ret = s_onvif_ct.fCallFxn(NULL,&presetcfg,sizeof(presetcfg),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestDelPreset(int presetsNum)
{
	int ret = 0;
    THIRD_PRESET_CFG presetcfg = {0};
    presetcfg.Cmd = THIRD_CMD_DEL_PRESET;
    presetcfg.UseMask = 0xf;
    presetcfg.PresetNum = 1;
    presetcfg.PresetArray[0] = presetsNum;

    ret = s_onvif_ct.fCallFxn(NULL,&presetcfg,sizeof(presetcfg),NULL,NULL,1000);

    return ret;
}

int RestOnvif_SetScope(char *nameP)
{
    int ret = 0;
    THIRD_DEVICE_NAME inbuffer = {0};
    inbuffer.cmd = THIRD_CMD_SET_DEVICE_NAME;
    inbuffer.UseMask = 0x1;
    snprintf(inbuffer.DeviceName,sizeof(inbuffer.DeviceName),"%s",inbuffer.DeviceName);

    ret = s_onvif_ct.fCallFxn(NULL,&inbuffer,sizeof(inbuffer),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestBoardLensZoom(int type,int speed)
{
    int ret = 0;
    THIRD_PTZ_CONTROL ptzcontrl = {0};
    ptzcontrl.Cmd = THIRD_CMD_PTZ_CONTROL;
    ptzcontrl.UseMask = 0xf;
    ptzcontrl.Type = type;
    ptzcontrl.Speed = speed;

    ret = s_onvif_ct.fCallFxn(NULL,&ptzcontrl,sizeof(ptzcontrl),NULL,NULL,1000);

    return ret;
}

int RestOnvif_ReqIFrame(int devNo, int channelNo, int streamNo)
{
    int ret = 0;
    THIRD_FORCE_IFRAME iframe = {0};
    iframe.Cmd = THIRD_CMD_FORCE_IFRAME;
    iframe.UseMask = 0x1;
    iframe.Stream = streamNo;

    ret = s_onvif_ct.fCallFxn(NULL,&iframe,sizeof(iframe),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestReboot()
{
    int ret = 0;
    int cmd = THIRD_CMD_REBOOT;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),NULL,NULL,1000);

    return ret;
}

int RestOnvif_RequestSnap(int devNo, int channelNo, int streamNo,char *PicName)
{

    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_PICTURE_SNAPSHOT inbuffer = {0}, *snapinfo = NULL;
    inbuffer.Cmd = THIRD_CMD_GET_VIDEO_SNAPSHOT;
    inbuffer.Stream = streamNo;

    ret = s_onvif_ct.fCallFxn(NULL,&inbuffer,sizeof(inbuffer),&outbuffer,&outsize,1000);
    if(ret == 0)
    {

        if(outbuffer && outsize > 0)
        {
            snapinfo = (THIRD_PICTURE_SNAPSHOT *)outbuffer;
            if(snapinfo->UseMask & 0x1)
            {
                sscanf(snapinfo->PicPath, "%s", PicName);
            }
        }

    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_RequestGetZoomCfg(float *zoom, int *max)
{
    int ret = 0;
    int outsize = 0;
    void *outbuffer = NULL;
    THIRD_ZOOM_CFG *zoomcfg = NULL;
    int cmd = THIRD_CMD_GET_ZOOM_CFG;

    ret = s_onvif_ct.fCallFxn(NULL,&cmd,sizeof(cmd),&outbuffer,&outsize,1000);

    if (ret == 0)
    {
        if(outbuffer && outsize > 0)
        {
            zoomcfg = (THIRD_ZOOM_CFG *)outbuffer;
            *zoom = zoomcfg->ZoomInteger + zoomcfg->ZoomDecimal * 0.1;
            *max = zoomcfg->ZoomMax;
            LOGW("[%d][%d][%f][%d]\n",zoomcfg->ZoomInteger, zoomcfg->ZoomDecimal,*zoom,*max);
        }
    }

    Common_cJSON_free(outbuffer);

    return ret;
}

int RestOnvif_RequestSetZoomCfg(float zoom)
{
    int ret = 0;
    THIRD_ZOOM_CFG zoomcfg = {0};
    zoomcfg.Cmd = THIRD_CMD_SET_ZOOM_CFG;
    zoomcfg.UseMask = 0xf;
    zoomcfg.ZoomInteger = zoom;
    zoomcfg.ZoomDecimal = (zoom - zoomcfg.ZoomInteger)*10;

    ret = s_onvif_ct.fCallFxn(NULL,&zoomcfg,sizeof(zoomcfg),NULL,NULL,1000);

    return ret;
}

