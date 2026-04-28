#include <stdlib.h>
#include <string.h>
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"
#include "core_maintenance.h"
/*
Uri:/Core/Maintain
Method:Get/Put
Data:
{
    Mode:, // 0-禁用,1-每天,2-每周,3-每月,4-单次
    Week:{
         Mask:xxx,// 星期 :bit0-星期天,bit1-星期一,
         Time:123010
         }
    EveryDate:123010:,//
    Once:20170406123010,// 时间 20170406123010
    Monthly:{
        DaysMask:xxx,// 天 :bit0-1号,bit1-2号....
        Time:123010 // 每天的时间
    }
}
*/

#define MAJOR_OPERATION                 0x3
#define MINOR_REBOOT_DVR                0x44    /* 本地重启设备 */
#define MAX_DAYS                7       //每周天数

//自动维护参数
typedef struct tagOVFS_DVR_AUTOREBOOT
{
    U32
    byAutoRebootMode;   //自动维护模式:0--不维护，1--每天定时维护，2--每周定时维护，3--每月定时维护,4-单次维护
    U32                         dwSingleTime;       //单次维护时间:time_t类型
    U32
    dwEveryDayTime;     //每天维护时间:0-7位是分钟，8-15位是小时
    /*U32                       bWeeklyDay[MAX_DAYS];       //每周7天是否启动维护:0--星期天，1--星期一，依次往后*/
    U32
    dwMask;                  // 星期 :bit0-星期天,bit1-星期一
    U32
    dwWeeklyTime;   //每周维护时间:0-7位是秒，8-15位是分钟,16-24 是小时
    U32                         dwDayMaskOfMonthly; // 每月 的某天
    U32                         dwTimeOfMonthly; // 每月某天的时间
} OVFS_DVR_AUTOREBOOT, *LPOVFS_DVR_AUTOREBOOT;

typedef struct _tagCoreMaintainMgr
{
    OVFS_DVR_AUTOREBOOT  bAutoReBoot;
    S32                  bChange;// 配置改变
    Common_Lock_T        hLock;
    Common_Thread_T      hThread;
    S32                  bExitThread;
} CoreMaintainMgr_T;

static CoreMaintainMgr_T g_tMaintainMgr;

static S32 g_bInit = 0;
static ModuleHandle_T m_hModuleHandle;

static S32 Core_Maintenance_WriteLog()
{
    S32 nRet = -1;
    cJSON_Struct *pConfig = NULL, *pChild = NULL, *pArray = NULL,
                  *pOutParams = NULL;

    pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if (pConfig)
    {
        Common_Json_SetAttrValue(pConfig, -1, "Header", Common_Json_Type_Object, NULL,
                                 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Header/Uri", Common_Json_Type_String,
                                 "/EventLog/LogFunction", 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Header/Method", Common_Json_Type_String,
                                 "put", 0, 0);
        pChild = Common_Json_SetAttrValue(pConfig, -1, "Data", Common_Json_Type_Object,
                                          NULL, 0, 0);
        pArray = Common_Json_SetAttrValue(pChild, -1, "ResList", Common_Json_Type_Array,
                                          NULL, 0, 0);

        Common_Json_SetAttrValue(pArray, 0, "MajorType", Common_Json_Type_Number, NULL,
                                 MAJOR_OPERATION, 0);
        Common_Json_SetAttrValue(pArray, 0, "MinorType", Common_Json_Type_Number, NULL,
                                 MINOR_REBOOT_DVR, 0);
        Common_Json_SetAttrValue(pArray, 0, "Channel", Common_Json_Type_Number, NULL, 1,
                                 0);

        nRet = Module_CallFunctions(m_hModuleHandle, pConfig, &pOutParams, 3000);

        Common_Json_Delete(pConfig);
        Common_Json_Delete(pOutParams);
        pConfig = NULL;
        pOutParams = NULL;
    }

    return nRet;
}

static S32 FillMaintainInfo(cJSON_Struct *parentItem, void *cfgData)
{
    char *pStringValue;
    S32 nIntValue;
    OVFS_DVR_AUTOREBOOT *p_auto_reboot = NULL;

    if (NULL == cfgData)
    {
        return -1;
    }

    if (parentItem != NULL)
    {
        p_auto_reboot = (OVFS_DVR_AUTOREBOOT *)cfgData;

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "Maintain/Mode", NULL, NULL,
                                 &nIntValue, NULL);
        if (-1 != nIntValue)
        {
            p_auto_reboot->byAutoRebootMode = (U32)nIntValue;
        }

        //if (1 == p_auto_reboot->byAutoRebootMode)
        {
            pStringValue = NULL;
            Common_Json_GetAttrValue(parentItem, -1, "Maintain/EveryDate", NULL,
                                     &pStringValue, NULL, NULL);
            if (NULL != pStringValue)
            {
                S32 nHour = 0, nMin = 0, nSec = 0;
                if (3 == sscanf(pStringValue, "%02d%02d%02d", &nHour, &nMin, &nSec))
                {
                    p_auto_reboot->dwEveryDayTime = ((nHour & 0xFF) << 16) | ((
                                                        nMin & 0xFF) << 8 ) | (nSec & 0xFF);
                }
                else if (2 == sscanf(pStringValue, "%02d%02d", &nHour, &nMin))
                {
                    p_auto_reboot->dwEveryDayTime = ((nHour & 0xFF) << 16) | ((nMin & 0xFF) << 8 );
                }
            }
        }
        //else if (2 == p_auto_reboot->byAutoRebootMode)
        {
            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "Maintain/Week/Mask", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                p_auto_reboot->dwMask = (U32)nIntValue;
            }

            pStringValue = NULL;
            Common_Json_GetAttrValue(parentItem, -1, "Maintain/Week/Time", NULL,
                                     &pStringValue, NULL, NULL);
            if (NULL != pStringValue)
            {
                S32 nHour = 0, nMin = 0, nSec = 0;
                if (3 == sscanf(pStringValue, "%02d%02d%02d", &nHour, &nMin, &nSec))
                {
                    p_auto_reboot->dwWeeklyTime = ((nHour & 0xFF) << 16) | ((nMin & 0xFF) << 8 ) |
                                                  (nSec & 0xFF);
                }
                else if (2 == sscanf(pStringValue, "%02d%02d", &nHour, &nMin))
                {
                    p_auto_reboot->dwWeeklyTime = ((nHour & 0xFF) << 16) | ((nMin & 0xFF) << 8 );
                }
            }

        }
        {
            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "Maintain/Monthly/DaysMask", NULL,
                                     NULL, &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                p_auto_reboot->dwDayMaskOfMonthly = (U32)nIntValue;
            }

            pStringValue = NULL;
            Common_Json_GetAttrValue(parentItem, -1, "Maintain/Monthly/Time", NULL,
                                     &pStringValue, NULL, NULL);
            if (NULL != pStringValue)
            {
                S32 nHour = 0, nMin = 0, nSec = 0;
                if (3 == sscanf(pStringValue, "%02d%02d%02d", &nHour, &nMin, &nSec))
                {
                    p_auto_reboot->dwTimeOfMonthly = ((nHour & 0xFF) << 16) | ((
                                                         nMin & 0xFF) << 8 ) | (nSec & 0xFF);
                }
                else if (2 == sscanf(pStringValue, "%02d%02d", &nHour, &nMin))
                {
                    p_auto_reboot->dwTimeOfMonthly = ((nHour & 0xFF) << 16) | ((nMin & 0xFF) << 8 );
                }
            }

        }
        //else if (3 == p_auto_reboot->byAutoRebootMode)
        {
            pStringValue = NULL;
            Common_Json_GetAttrValue(parentItem, -1, "Maintain/Once", NULL, &pStringValue,
                                     NULL, NULL);
            if (NULL != pStringValue)
            {
                Common_Time_T tTime;
                S32 nYear = 0, nMon = 0, nDay = 0, nHour = 0, nMin = 0, nSec = 0;
                if(6 == sscanf(pStringValue, "%04d%02d%02d%02d%02d%02d", &nYear, &nMon, &nDay,
                               &nHour, &nMin, &nSec))
                {
                    tTime.year = nYear;
                    tTime.month = nMon;
                    tTime.day = nDay;
                    tTime.hour = nHour;
                    tTime.min = nMin;
                    tTime.sec = nSec;

                    Common_Common2LinuxTime(&tTime, (time_t *)(&p_auto_reboot->dwSingleTime));
                }

            }
        }

    }
    else
    {
        LOGE("%s: Can't find %s\n", __FUNCTION__, "Maintain");
    }

    return 0;
}

static S32 Maintain_Get_Info(const char *uriString, const char *condition,
                             Common_cJSON_T *in, Common_cJSON_T *out)
{
    S8 tmp[64];
    Common_cJSON_T *tmpObj = NULL;

    if (NULL == out || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    Common_cJSON_AddNumberToObject(out, "Mode",
                                   g_tMaintainMgr.bAutoReBoot.byAutoRebootMode);

    //if (1 == g_tMaintainMgr.bAutoReBoot.byAutoRebootMode)
    {
        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%02d%02d%02d",
                 (g_tMaintainMgr.bAutoReBoot.dwEveryDayTime >> 16) & 0xFF,
                 (g_tMaintainMgr.bAutoReBoot.dwEveryDayTime >> 8) & 0xFF,
                 (g_tMaintainMgr.bAutoReBoot.dwEveryDayTime) & 0xFF);

        Common_cJSON_AddStringToObject(out, "EveryDate", tmp);
    }
    //else if (2 == g_tMaintainMgr.bAutoReBoot.byAutoRebootMode)
    {
        tmpObj = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(out, "Week", tmpObj);

        Common_cJSON_AddNumberToObject(tmpObj, "Mask",
                                       g_tMaintainMgr.bAutoReBoot.dwMask);

        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%02d%02d%02d",
                 (g_tMaintainMgr.bAutoReBoot.dwWeeklyTime >> 16) & 0xFF,
                 (g_tMaintainMgr.bAutoReBoot.dwWeeklyTime >> 8) & 0xFF,
                 g_tMaintainMgr.bAutoReBoot.dwWeeklyTime & 0xFF);

        Common_cJSON_AddStringToObject(tmpObj, "Time", tmp);
    }
    {
        tmpObj = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(out, "Monthly", tmpObj);

        Common_cJSON_AddNumberToObject(tmpObj, "DaysMask",
                                       g_tMaintainMgr.bAutoReBoot.dwDayMaskOfMonthly);

        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%02d%02d%02d",
                 (g_tMaintainMgr.bAutoReBoot.dwTimeOfMonthly >> 16) & 0xFF,
                 (g_tMaintainMgr.bAutoReBoot.dwTimeOfMonthly >> 8) & 0xFF,
                 (g_tMaintainMgr.bAutoReBoot.dwTimeOfMonthly) & 0xFF);

        Common_cJSON_AddStringToObject(tmpObj, "Time", tmp);
    }
    //else if (3 == g_tMaintainMgr.bAutoReBoot.byAutoRebootMode)
    {
        Common_Time_T tTime;

        Common_Linux2CommonTime((time_t)g_tMaintainMgr.bAutoReBoot.dwSingleTime,
                                &tTime);

        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%04d%02d%02d%02d%02d%02d", tTime.year, tTime.month,
                 tTime.day,
                 tTime.hour, tTime.min, tTime.sec);

        Common_cJSON_AddStringToObject(out, "Once", tmp);
    }

    return 0;
}

static S32 Maintain_Put_Info(const char *uriString, const char *condition,
                             Common_cJSON_T *in, Common_cJSON_T *out)
{
    char *pStringValue;
    S32 nIntValue;
    cJSON_Struct *parentItem = NULL;
    OVFS_DVR_AUTOREBOOT *p_auto_reboot = NULL;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;
    p_auto_reboot = &g_tMaintainMgr.bAutoReBoot;

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Mode", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        p_auto_reboot->byAutoRebootMode = (U32)nIntValue;
    }

//  if (1 == p_auto_reboot->byAutoRebootMode)
    {
        pStringValue = NULL;
        Common_Json_GetAttrValue(parentItem, -1, "EveryDate", NULL, &pStringValue, NULL,
                                 NULL);
        if (NULL != pStringValue)
        {
            S32 nHour = 0, nMin = 0, nSec = 0;
            if (3 == sscanf(pStringValue, "%02d%02d%02d", &nHour, &nMin, &nSec))
            {
                p_auto_reboot->dwEveryDayTime = ((nHour & 0xFF) << 16) | ((
                                                    nMin & 0xFF) << 8 ) | (nSec & 0xFF);
            }
            else if (2 == sscanf(pStringValue, "%02d%02d", &nHour, &nMin))
            {
                p_auto_reboot->dwEveryDayTime = ((nHour & 0xFF) << 16) | ((
                                                    nMin & 0xFF) << 8 ) | (nSec & 0xFF);
            }
        }
    }
    //else if (2 == p_auto_reboot->byAutoRebootMode)
    {
        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "Week/Mask", NULL, NULL, &nIntValue,
                                 NULL);
        if (-1 != nIntValue)
        {
            p_auto_reboot->dwMask = (U32)nIntValue;
        }

        pStringValue = NULL;
        Common_Json_GetAttrValue(parentItem, -1, "Week/Time", NULL, &pStringValue, NULL,
                                 NULL);
        if (NULL != pStringValue)
        {
            S32 nHour = 0, nMin = 0, nSec = 0;
            if (3 == sscanf(pStringValue, "%02d%02d%02d", &nHour, &nMin, &nSec))
            {
                p_auto_reboot->dwWeeklyTime = ((nHour & 0xFF) << 16) | ((nMin & 0xFF) << 8 ) |
                                              (nSec & 0xFF);
            }
            else if (2 == sscanf(pStringValue, "%02d%02d", &nHour, &nMin))
            {
                p_auto_reboot->dwWeeklyTime = ((nHour & 0xFF) << 16) | ((nMin & 0xFF) << 8 ) |
                                              (nSec & 0xFF);
            }
        }

    }
    {
        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "Monthly/DaysMask", NULL, NULL,
                                 &nIntValue, NULL);
        if (-1 != nIntValue)
        {
            p_auto_reboot->dwDayMaskOfMonthly = (U32)nIntValue;
        }

        pStringValue = NULL;
        Common_Json_GetAttrValue(parentItem, -1, "Monthly/Time", NULL, &pStringValue,
                                 NULL, NULL);
        if (NULL != pStringValue)
        {
            S32 nHour = 0, nMin = 0, nSec = 0;
            if (3 == sscanf(pStringValue, "%02d%02d%02d", &nHour, &nMin, &nSec))
            {
                p_auto_reboot->dwTimeOfMonthly = ((nHour & 0xFF) << 16) | ((
                                                     nMin & 0xFF) << 8 ) | (nSec & 0xFF);
            }
            else if (2 == sscanf(pStringValue, "%02d%02d", &nHour, &nMin))
            {
                p_auto_reboot->dwTimeOfMonthly = ((nHour & 0xFF) << 16) | ((
                                                     nMin & 0xFF) << 8 ) | (nSec & 0xFF);
            }
        }

    }
    //else if (3 == p_auto_reboot->byAutoRebootMode)
    {
        pStringValue = NULL;
        Common_Json_GetAttrValue(parentItem, -1, "Once", NULL, &pStringValue, NULL,
                                 NULL);
        if (NULL != pStringValue)
        {
            Common_Time_T tTime;
            S32 nYear = 0, nMon = 0, nDay = 0, nHour = 0, nMin = 0, nSec = 0;
            if(6 == sscanf(pStringValue, "%04d%02d%02d%02d%02d%02d", &nYear, &nMon, &nDay,
                           &nHour, &nMin, &nSec))
            {
                tTime.year = nYear;
                tTime.month = nMon;
                tTime.day = nDay;
                tTime.hour = nHour;
                tTime.min = nMin;
                tTime.sec = nSec;

                Common_Common2LinuxTime(&tTime, (time_t *)(&p_auto_reboot->dwSingleTime));
            }

        }
    }

    return 0;
}

static S32 Core_Maintenance_SaveCfg()
{
    Common_cJSON_T *saveObj = NULL;
    Common_cJSON_T *tmp = NULL;

    saveObj = Common_cJSON_CreateObject();

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(saveObj, "Maintain", tmp);
    Maintain_Get_Info("Maintain", NULL, NULL, tmp);

    Core_Maintenance_SaveConfig(m_hModuleHandle, (cJSON_Struct *)saveObj);

    return 0;
}

static S32 Core_Maintenance_PraseInputJson(Common_cJSON_T *inputData,
        S8 **method, S8 **uri, Common_cJSON_T **inData)
{
    Common_cJSON_T *header = Common_cJSON_GetObjectItem(inputData, "Header");
    if(header == NULL)
    {
        LOGE("Get header fail!\n");
        return -1;
    }

    Common_cJSON_T *tmp = NULL;
    tmp = Common_cJSON_GetObjectItem(header, "Method");
    if(tmp == NULL)
    {
        LOGE("Can't found method!\n");
        return -1;
    }
    if(method)
    {
        *method = tmp->valuestring;
    }

    tmp = Common_cJSON_GetObjectItem(header, "Uri");
    if(tmp == NULL)
    {
        LOGE("Can't found Uri.\n");
        return -1;
    }
    if(uri)
    {
        *uri = tmp->valuestring;
    }


    tmp = Common_cJSON_GetObjectItem(inputData, "Data");
    if(inData)
    {
        *inData = tmp;
    }

    return 0;
}

static S32 Core_Maintenance_GetUriAndQue(const S8 *srcUri, S8 **uriStr,
        S8 **conditionStr)
{
    S8 *tmp = (S8 *)strstr(srcUri, "?");
    if(tmp)
    {
        char buff[256];
        memset(buff, 0, sizeof(buff));
        memcpy(buff, srcUri, (int)(tmp - srcUri));
        *uriStr         = Common_StrDup(buff, __FUNCTION__, __LINE__);
        *conditionStr   = Common_StrDup(tmp + 1, __FUNCTION__, __LINE__);
    }
    else
    {
        *uriStr         = Common_StrDup((S8 *)srcUri, __FUNCTION__, __LINE__);
        *conditionStr   = NULL;
    }

    return 0;
}

static Common_cJSON_T *Core_Maintenance_GenerateOutParam(S32 retCode,
        Common_cJSON_T *outData)
{
    Common_cJSON_T *root = Common_cJSON_CreateObject();
    Common_cJSON_T *header = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(root, "Header", header);

    Common_cJSON_AddNumberToObject(header, "Code", retCode);
    if(retCode != 0)
    {
        Common_cJSON_AddStringToObject(header, "Decribe", "Operation fail.");
    }

    if(outData)
    {
        Common_cJSON_AddItemToObject(root, "Data", outData);
    }

    return root;
}

/*
static Common_cJSON_T* Core_Maintenance_GenerateInParam(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inData)
{
    if(uriPath == NULL)
    {
        return NULL;
    }

    Common_cJSON_T* root = Common_cJSON_CreateObject();
    Common_cJSON_T* header = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(root,"Header",header);

    Common_cJSON_AddStringToObject(header,"Method",method);
    char uriBuff[256];
    memset(uriBuff,0,sizeof(uriBuff));
    int uriPathLen = strlen(uriPath);
    memcpy(uriBuff,uriPath,uriPathLen);
    if(condition)
    {
        uriBuff[uriPathLen] = '?';
        memcpy(uriBuff+uriPathLen+1,condition,strlen(condition));
    }

    Common_cJSON_AddStringToObject(header,"Uri",uriBuff);
    Common_cJSON_AddItemToObject(root,"Data",inData);

    return root;
}
*/

static S32 Core_Maintenance_Thread(Common_Thread_T hThreadHandle,
                                   void *pUserData)
{
    U32 bReboot = 0;
    struct tm *tm_set, tm_save;
    time_t tTimeNow;
    S32 uCurrTime;

    while(1)
    {
        if (g_tMaintainMgr.bExitThread)
        {
            break;
        }

        Common_Sleep(1, 0);

        bReboot = 0;

        if(g_tMaintainMgr.bAutoReBoot.byAutoRebootMode != 0)
        {
            Common_GetCurrentTime(&uCurrTime, NULL);
            tTimeNow = (time_t)uCurrTime;//time(NULL);
            tm_set = Common_LocalTime_r(&tTimeNow, &tm_save);
            if(g_tMaintainMgr.bAutoReBoot.byAutoRebootMode == 1)
            {
                if( tm_set->tm_sec <= 10 &&
                        (((g_tMaintainMgr.bAutoReBoot.dwEveryDayTime >> 16) & 0xFF) ==
                         (U32)tm_set->tm_hour) &&
                        ((g_tMaintainMgr.bAutoReBoot.dwEveryDayTime >> 8) & 0xFF) ==
                        (U32)tm_set->tm_min)
                {
                    bReboot = 1;
                }
            }
            else if(g_tMaintainMgr.bAutoReBoot.byAutoRebootMode == 2)
            {
                if(g_tMaintainMgr.bAutoReBoot.dwMask & (1 << tm_set->tm_wday))
                {
                    if( tm_set->tm_sec <= 10 &&
                            (((g_tMaintainMgr.bAutoReBoot.dwWeeklyTime >> 16) & 0xFF) ==
                             (U32)tm_set->tm_hour) &&
                            ((g_tMaintainMgr.bAutoReBoot.dwWeeklyTime >> 8) & 0xFF) == (U32)tm_set->tm_min)
                    {
                        bReboot = 1;
                    }
                }
            }
            else if(g_tMaintainMgr.bAutoReBoot.byAutoRebootMode == 3)
            {
                if(g_tMaintainMgr.bAutoReBoot.dwDayMaskOfMonthly & (1 << (tm_set->tm_mday - 1)))
                {
                    if( tm_set->tm_sec <= 10 &&
                            (((g_tMaintainMgr.bAutoReBoot.dwTimeOfMonthly >> 16) & 0xFF) ==
                             (U32)tm_set->tm_hour) &&
                            ((g_tMaintainMgr.bAutoReBoot.dwTimeOfMonthly >> 8) & 0xFF) ==
                            (U32)tm_set->tm_min)
                    {
                        bReboot = 1;
                    }
                }
            }
            else if(g_tMaintainMgr.bAutoReBoot.byAutoRebootMode == 4)
            {
                if(((U32)tTimeNow <= g_tMaintainMgr.bAutoReBoot.dwSingleTime + 10)
                        && ((U32)tTimeNow > (g_tMaintainMgr.bAutoReBoot.dwSingleTime)))
                {
                    bReboot = 1;
                }
            }
            else
            {

            }
        }

        if (1 == bReboot)
        {
            LOGI("Common_System reboot!!!!!!!!!!!!!!!\n");
            Core_Maintenance_WriteLog();
            Common_System("reboot");
        }
    }

    return 0;
}


S32 Core_Maintenance_CallFunctions(ModuleHandle_T hModuleHandle,
                                   cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{
#if 0
    char *out = NULL;
    LOGI("recv call input:%s \n",
         out = Common_cJSON_PrintUnformatted((Common_cJSON_T *)pInParams, NULL));
    if(out)
    {
        Common_Free(out, __FUNCTION__, __LINE__);
    }
#endif

    char *method            = NULL;
    char *uri               = NULL;
    Common_cJSON_T *inData  = NULL;
    int ret = Core_Maintenance_PraseInputJson((Common_cJSON_T *)pInParams, &method,
              &uri, &inData);
    if(ret != 0)
    {
        LOGE("inparam parse fail!\n");
        return -1;
    }

    if(method == NULL)
    {
        LOGE("method is NULL!\n");
        return -1;
    }

    char *uriString     = NULL;
    char *uriCondition  = NULL;
    Core_Maintenance_GetUriAndQue(uri, &uriString, &uriCondition);

    if (Common_StrniCmp(uriString, (char *)"/Core/Maintain",
                        strlen("/Core/Maintain")) != 0)
    {
        if(uriString)
        {
            Common_Free(uriString, __FUNCTION__, __LINE__);
        }

        if(uriCondition)
        {
            Common_Free(uriCondition, __FUNCTION__, __LINE__);
        }

        return -1;
    }

    ret = 0;

    Common_cJSON_T *outData = Common_cJSON_CreateObject();
    if(Common_StriCmp(method, (char *)"get") == 0)
    {
        if (Common_StriCmp(uriString, (char *)"/Core/Maintain") == 0)
        {
            Maintain_Get_Info(uriString, uriCondition, inData, outData);
        }
        else
        {
            ret = -1;
        }
    }
    else if(Common_StriCmp(method, (char *)"put") == 0)
    {
        if (Common_StriCmp(uriString, (char *)"/Core/Maintain") == 0)
        {
            Maintain_Put_Info(uriString, uriCondition, inData, outData);
            Core_Maintenance_SaveCfg();
        }
        else
        {
            ret = -1;
        }
    }
    else if(Common_StriCmp(method, (char *)"post") == 0)
    {
        ret = -1;
    }
    else if(Common_StriCmp(method, (char *)"delete") == 0)
    {
        ret = -1;
    }
    else
    {
        LOGE("Unknow method=%s\n", method);
        ret = -1;
    }

    if(outData->child == NULL)
    {
        Common_cJSON_Delete(outData);
        outData = NULL;
    }

    *pOutParams = Core_Maintenance_GenerateOutParam(ret, outData);

    if(uriString)
    {
        Common_Free(uriString, __FUNCTION__, __LINE__);
    }

    if(uriCondition)
    {
        Common_Free(uriCondition, __FUNCTION__, __LINE__);
    }

    return ret;
}

S32 Core_Maintenance_Init(ModuleHandle_T hModuleHandle, cJSON_Struct *pConfig)
{
    if (g_bInit)
    {
        return 0;
    }

    COMMON_CLR_ARG(g_tMaintainMgr);
    COMMON_CLR_ARG(m_hModuleHandle);

    m_hModuleHandle = hModuleHandle;
    FillMaintainInfo(pConfig, (void *)(&g_tMaintainMgr.bAutoReBoot));

    Common_Lock_Create(&g_tMaintainMgr.hLock, "Core_Maintenance_lock");
    Common_Thread_Create(&g_tMaintainMgr.hThread, "Core_Maintenance_Thread", 0, 0,
                         Core_Maintenance_Thread, NULL);

    Core_Maintenance_SaveCfg();

    g_bInit = 1;

    return 0;
}
