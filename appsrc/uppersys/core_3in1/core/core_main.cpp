#include <stdio.h>
#ifdef WIN32
#include <Windows.h>
#else
#endif
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "core_time.h"
#include "core_power.h"
#include "core_restore.h"
#include "core_maintenance.h"
#include "core_version.h"
#include "core_export.h"
#include "ovfs_wtdg.h"
#include "core_copyright.h"
#include "core_debug.h"
#include "cjson.h"
#include "common_json_str_ops.h"
#include "libaccess_sdk.h"

#ifndef NOALARM
#include "libalarm_sdk.h"
#endif

#define  CORE_DEBUG_APP     "/tmp/core_debug" /*it will reboot system when some app crash and such file exist, or ovfs_core will respawn the app crashed again*/

typedef struct
{
    const char *registName;
    const char *binaryName;
    int reboot;
} APP_NAME_MAP_T;

APP_NAME_MAP_T s_name_map[] =
{
    {"BoardSys",      "ovfs_boardsystem",       1}, /*ovfs_core will reboot when boardsystem crashed, no matter debug or release version*/
#ifndef AOV
    {"Alarm",         "ovfs_alarm",             0},
//  {"FileManage",    "ovfs_filemanage",        0},
//  {"EventLog",      "ovfs_eventlog",          0},
    {"NetWork",       "ovfs_network",           0},
//  {"Record",        "ovfs_record",            0},
    {"Access",        "ovfs_access",            0},
    {"SmartServer",   "ovfs_smartserver",       0},
    {"MediaServer",   "ovfs_mediaserver",       0},
    {"AccessHost",    "ovfs_access_host",       0},
    {"Onvif",         "ovfs_onvif",             0},
    {"Ptz",           "ovfs_ptz",               0},
    {"Webserver",     "ovfs_webserver",         0},
    {"ZCTest",        "ovfs_zkdc",              0},
    {"Mojing",        "mj_tmac",                0},
    {"ovfs_tushi",    "ovfs_tushi",             0},
	{"ovfs_snap",     "ovfs_snap",              0},
    {"Hikserver",     "ovfs_hiserver",          0},
	{"Ovfs_websocket","ovfs_websocket",			0},
#endif
};

typedef struct __tagCoreModuleStatus
{
    S8 szModuleName[32];
    struct __tagCoreModuleStatus *pPrev;
    struct __tagCoreModuleStatus *pNext;
} CoreModuleStatus_T;

static CoreModuleStatus_T *g_pOfflineModuleList = NULL;
static CoreModuleStatus_T *g_pVipModuleList = NULL;
static Common_Lock_T g_hModuleStatusLock = NULL;
static int g_iDiskStatus = 0;
extern Core_Version_T g_tVersion;

static const char *s_coreDefaultCfg =
    "{"
    "\"Time\":{"
    "\"NTP\":{\"Enable\":0,\"Server\":\"\",\"Interval\":0},"
    "\"TimeZone\":{\"Zone\":27,\"EnableBias\":0,\"ZoneBias\":0},"
    "\"DST\":{"
    "\"Enable\":0,\"Mode\":0,\"Bias\":0,"
    "\"StartTime\":{\"Month\":0,\"WeekIdx\":0,\"WeekDay\":0,\"Hour\":0,\"Min\":0},"
    "\"StopTime\":{\"Month\":0,\"WeekIdx\":0,\"WeekDay\":0,\"Hour\":0,\"Min\":0}"
    "}"
    "},"
    "\"Maintenance\":{"
    "\"Maintain\":{"
    "\"Mode\":0,"
    "\"EveryDate\":\"000000\","
    "\"Week\":{\"Mask\":0,\"Time\":\"000000\"},"
    "\"Monthly\":{\"DaysMask\":0,\"Time\":\"000000\"},"
    "\"Once\":\"19700101000000\""
    "}"
    "}"
    "}";

#define CORE_DOG_FEED_INTERVAL    20
typedef struct __tagCoreDogInfo
{
    S32 bEnable;
    S32 nLastFeedSec;
} CoreDogInfo_T;
static CoreDogInfo_T g_tDogInfo;
S32 Core_Dog_IsEnable()
{
    return g_tDogInfo.bEnable;
}

S32 Core_Dog_Enable_First(S32 bEnable)
{
    g_tDogInfo.bEnable = bEnable;
#ifndef WIN32
    int stopOptWdt  = 0;
    if(access("/root/stopoptwdt", F_OK) == 0)
        stopOptWdt = 1;

    if (bEnable)
    {
        if(!stopOptWdt)
        {
            Wtdg_SetTime(5 * 60);
            Wtdg_Start();
        }
    }
    else
    {
        if(!stopOptWdt)
            Wtdg_Stop();
    }
#endif
    return 0;
}

S32 Core_Dog_Enable(S32 bEnable)
{
    g_tDogInfo.bEnable = bEnable;
#ifndef WIN32
    int stopOptWdt  = 0;
    if(access("/root/stopoptwdt", F_OK) == 0)
        stopOptWdt = 1;

    if (bEnable)
    {
        if(!stopOptWdt)
        {
            Wtdg_SetTime(2 * 60);
            Wtdg_Start();
        }
    }
    else
    {
        if(!stopOptWdt)
            Wtdg_Stop();
    }
#endif
    return 0;
}

S32 Core_Dog_Feed()
{
    S32 nSec = 0;
    Common_GetSystemCount(&nSec, NULL);
    if (nSec >= g_tDogInfo.nLastFeedSec + CORE_DOG_FEED_INTERVAL
            || nSec < g_tDogInfo.nLastFeedSec)
    {
        // feed
        g_tDogInfo.nLastFeedSec = nSec;
#ifndef WIN32

        int stopOptWdt  = 0;
        if(access("/root/stopoptwdt", F_OK) == 0)
            stopOptWdt = 1;

        if(!stopOptWdt)
            Wtdg_Feed();

        LOGW("Wtdg_Feed do none!\n");
#endif
    }
    return 0;
}
//
#define CORE_COPYRIGHT_CHECK_INTERVAL    60
static S32 g_nCopyRight_CheckCount = 0;
static S32 g_nLastRebootTime = 0;
extern S32 Version_IsActived();
void Core_CheckCopyRight(ModuleHandle_T hModuleHandle)
{
    cJSON_Struct *pInParam = NULL, *pArray = NULL;
    if (g_nCopyRight_CheckCount >= CORE_COPYRIGHT_CHECK_INTERVAL)
    {
        //  check
        S32 nCurrTime = 0;
        g_nCopyRight_CheckCount = 0;
        if( Version_IsActived())
        {
            return;
        }
#if 0
        Common_Time_T tMytime;
        Common_GetLocalTime(&tMytime);
        if (tMytime.year < 2017 || (tMytime.year == 2017 && tMytime.month < 12))
        {
            return;
        }
#endif
        Common_GetSystemCount(&nCurrTime, NULL);
        if (nCurrTime >= g_nLastRebootTime + 2 * 60 * 60)
        {
            Common_System("reboot");
        }
        pInParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pInParam != NULL)
        {
            Common_Json_SetAttrValue(pInParam, -1, "/Header", Common_Json_Type_Object, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pInParam, -1, "/Header/Method",
                                     Common_Json_Type_String, "Put", 0, 0);
            Common_Json_SetAttrValue(pInParam, -1, "/Header/Uri", Common_Json_Type_String,
                                     "/BoardSys/Osd/CopyRight", 0, 0);
            pArray = Common_Json_SetAttrValue(pInParam, -1, "/Data",
                                              Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(pArray, -1, "Device", Common_Json_Type_Number, NULL, 0,
                                     0);
            Common_Json_SetAttrValue(pArray, -1, "Channel", Common_Json_Type_Number, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pArray, -1, "Enable", Common_Json_Type_Number, NULL, 1,
                                     0);
            Common_Json_SetAttrValue(pArray, -1, "X", Common_Json_Type_Number, NULL, 32, 0);
            Common_Json_SetAttrValue(pArray, -1, "Y", Common_Json_Type_Number, NULL, 240,
                                     0);
            Common_Json_SetAttrValue(pArray, -1, "StringColor", Common_Json_Type_Number,
                                     NULL, 2, 0);
            Common_Json_SetAttrValue(pArray, -1, "Size", Common_Json_Type_Number, NULL,
                                     g_nCopyRightSize, 0);
            Common_Json_SetAttrValue(pArray, -1, "String", Common_Json_Type_String,
                                     szCopyRightString, 0, 0);
            Common_Json_SetAttrValue(pArray, -1, "BitMapArr", Common_Json_Type_String,
                                     g_szCopyRightPixels, 0, 0);
            Common_Json_SetAttrValue(pArray, -1, "BitMapArrLen", Common_Json_Type_Number,
                                     NULL, g_nCopyRightLenth, 0);
            Common_Json_SetAttrValue(pArray, -1, "BitMapW", Common_Json_Type_Number, NULL,
                                     g_nCopyRightW, 0);
            Common_Json_SetAttrValue(pArray, -1, "BitMapH", Common_Json_Type_Number, NULL,
                                     g_nCopyRightH, 0);
            Module_CallFunctions(hModuleHandle, pInParam, NULL, 3000);
            Common_Json_Delete(pInParam);
        }

    }
    g_nCopyRight_CheckCount++;
}
//#define TEST_CHECKPATH
//#define TEST_JSONADD

static void *static_DiskFormat_Thread(void* param)
{
	LOGE("start Format Disk\n");
	Common_System("touch /tmp/core_debug");
	Common_System("pkill filemanage");
	Common_System("pkill record");
	Common_System("cd /tmp/mmc/mmc1/;ls |grep -v faces |xargs rm -fr;");
	Common_System("ovfs_record&");
	Common_System("ovfs_filemanage&");
	Common_System("rm /tmp/core_debug");

	g_iDiskStatus = 0;
	LOGE("end Format Disk\n");

    return 0;
}



static S32 static_Module_CallFunctions(ModuleHandle_T hModuleHandle,
                                       cJSON_Struct *pInParams, cJSON_Struct **pOutParams, void *pUserData)
{
    S8 *szUri = NULL;
    S8 *szMethod = NULL;
    cJSON_Struct *pOutJson = NULL;
    cJSON_Struct *pNode, *pNode1;
    S32 nArrarIdx = 0;
#if 0
    if (pInParams != NULL)
    {
        char *pStr = Common_Json_Print(pInParams, NULL);
        if (pStr)
        {
            printf("[%s.%d]in <%s>\n", __FUNCTION__, __LINE__, pStr);
            Common_Free(pStr, __FUNCTION__, __LINE__);
        }
    }
#endif

    Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &szUri, NULL, NULL);
    Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &szMethod, NULL,
                             NULL);
    if(0 == Common_StriCmp((char *)"/Core", szUri) &&
            0 == Common_StriCmp((char *)"get", szMethod))
    {
        pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pOutJson != NULL)
        {
            Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                     NULL, 0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                     0);

            pNode = Common_Json_SetAttrValue(pOutJson, -1, "Data/ResList",
                                             Common_Json_Type_Array, NULL, 0, 0);
            nArrarIdx = 0;
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/Version", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "Version", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/DeviceName", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "DeviceName", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/Time", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "Time", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/Power", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "Power", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/Reset", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "Reset", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "The Key [Reset] pressed", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/Restore", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "Restore", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/Maintain", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "Maintain", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/ExportCfg", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "ExportCfg", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            nArrarIdx++;

            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/ImportCfg", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "ImportCfg", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
            nArrarIdx++;
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/WatchDog", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "WatchDog", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Get", 0, 0);
            nArrarIdx++;
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/FeedDog", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "FeedDog", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
            nArrarIdx++;

			Common_Json_SetAttrValue(pNode, nArrarIdx, "/Uri", Common_Json_Type_String,
                                     "/Core/DiskFormat", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Label", Common_Json_Type_String,
                                     "DiskFormat", 0, 0);
            Common_Json_SetAttrValue(pNode, nArrarIdx, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nArrarIdx, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Get", 0, 0);
            nArrarIdx++;
        }

        if (pOutJson != NULL)
        {
            if (pOutParams != NULL)
            {
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }
        }
        else if(pOutParams != NULL)
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, -1, 0);
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }

        }
        Common_Json_Delete(pOutJson);
        pOutJson = NULL;

        return 0;
    }
    if(0 == Common_StriCmp((char *)"/Core/Version", szUri) ||
            0 == Common_StriCmp((char *)"/Core/DeviceName", szUri) ||
            0 == Common_StriCmp((char *)"/Core/VersionAuth", szUri) ||
	   		0 == Common_StriCmp("/Core/Sid",szUri))
    {
        if (0 == Core_Version_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }
    else  if (Common_StrniCmp(szUri, (char *)"/Core/Time",
                              strlen("/Core/Time")) == 0)
    {

        if (pOutParams != NULL)
        {
            Common_Json_Delete(*pOutParams);
            *pOutParams = NULL;
        }
        if (0 == Core_Time_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }
    else if(0 == Common_StrniCmp((char *)"/Core/Power", szUri, 11) ||
            0 == Common_StriCmp((char *)"/Core/Reset", szUri))
    {
        if (pOutParams != NULL)
        {
            Common_Json_Delete(*pOutParams);
            *pOutParams = NULL;
        }
        if (0 == Core_Power_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }
    else if(0 == Common_StrniCmp((char *)"/Core/Restore", szUri, 13))
    {
        if (pOutParams != NULL)
        {
            Common_Json_Delete(*pOutParams);
            *pOutParams = NULL;
        }
        if (0 == Core_Restore_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }
    else  if (Common_StrniCmp(szUri, (char *)"/Core/Maintain",
                              strlen("/Core/Maintain")) == 0)
    {
        if (pOutParams != NULL)
        {
            Common_Json_Delete(*pOutParams);
            *pOutParams = NULL;
        }
        if (0 == Core_Maintenance_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }
    else if (Common_StrniCmp(szUri, (char *)"/Core/ExportCfg",
                             strlen("/Core/ExportCfg")) == 0 ||
             Common_StrniCmp(szUri, (char *)"/Core/ImportCfg",
                             strlen("/Core/ImportCfg")) == 0)
    {
        if (pOutParams != NULL)
        {
            Common_Json_Delete(*pOutParams);
            *pOutParams = NULL;
        }
        if (0 == Core_Export_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }
    else if(0 == Common_StriCmp((char *)"/Core/Debug", szUri))
    {
        if (pOutParams != NULL)
        {
            Common_Json_Delete(*pOutParams);
            *pOutParams = NULL;
        }
        if (0 == Core_Debug_CallFunctions(hModuleHandle, pInParams, pOutParams))
        {
            return 0;
        }
    }



    if (pOutParams != NULL)
    {
        Common_Json_Delete(*pOutParams);
        *pOutParams = NULL;
    }
    if (0 == Common_StriCmp((char *)"/Core/WatchDog", szUri))
    {
        if (0 == Common_StriCmp((char *)"Get", szMethod))
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, 0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                         0);
                Common_Json_SetAttrValue(pOutJson, -1, "Data/Enable", Common_Json_Type_Number,
                                         NULL, g_tDogInfo.bEnable != 0, 0);
            }
        }
        else  if (0 == Common_StriCmp((char *)"Put", szMethod))
        {
            S32 bEnable = 0;
            Common_Json_GetAttrValue(pInParams, -1, "Data/Enable", NULL, NULL, &bEnable,
                                     NULL);
            Core_Dog_Enable(bEnable);
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, 0, 0);

            }

        }
        if (pOutJson != NULL)
        {
            if (pOutParams != NULL)
            {
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }
        }
        else if(pOutParams != NULL)
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, -1, 0);
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }

        }
        Common_Json_Delete(pOutJson);
        pOutJson = NULL;
        return 0;

    }
    if (0 == Common_StriCmp((char *)"/Core/FeedDog", szUri))
    {
        if (0 == Common_StriCmp((char *)"Put", szMethod))
        {

            Core_Dog_Feed();
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, 0, 0);
            }

        }
        if (pOutJson != NULL)
        {
            if (pOutParams != NULL)
            {
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }
        }
        else if(pOutParams != NULL)
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, -1, 0);
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }

        }
        Common_Json_Delete(pOutJson);
        pOutJson = NULL;
        return 0;

    }
    if(0 == Common_StriCmp((char *)"/Core/Notify/Offline", szUri))
    {
        S8 *pModuleName_notify = NULL;
        Common_Json_GetAttrValue(pInParams, -1, "/Data/ModuleName", NULL,
                                 &pModuleName_notify, NULL, NULL);
        if(pModuleName_notify != NULL)
        {
            if(0 != Common_StriCmp((char *)"Uritool", pModuleName_notify))
            {
                CoreModuleStatus_T *pStatus = NULL;
                Common_Lock(g_hModuleStatusLock);
                pStatus = g_pOfflineModuleList;
                while(pStatus != NULL)
                {
                    if(0 == Common_StriCmp(pStatus->szModuleName, pModuleName_notify))
                    {
                        break;
                    }
                    pStatus = pStatus->pNext;
                }
                if(pStatus == NULL)
                {
                    // 新增
                    pStatus = (CoreModuleStatus_T *)Common_Malloc(sizeof(CoreModuleStatus_T), 0,
                              __FUNCTION__, __LINE__);
                    if(pStatus != NULL)
                    {
                        memset(pStatus, 0, sizeof(CoreModuleStatus_T));
                        strncpy(pStatus->szModuleName, pModuleName_notify, 31);
                        pStatus->pNext = g_pOfflineModuleList;
                        if(g_pOfflineModuleList != NULL)
                        {
                            g_pOfflineModuleList->pPrev = pStatus;
                        }
                        g_pOfflineModuleList = pStatus;
                    }
                }
                Common_UnLock(g_hModuleStatusLock);
            }
        }

        return 0;
    }
    if(0 == Common_StriCmp((char *)"/Core/Notify/Online", szUri))
    {
        S8 *pModuleName_notify = NULL;
        Common_Json_GetAttrValue(pInParams, -1, "/Data/ModuleName", NULL,
                                 &pModuleName_notify, NULL, NULL);
        if(pModuleName_notify != NULL)
        {
            if(0 != Common_StriCmp((char *)"Uritool", pModuleName_notify))
            {
                CoreModuleStatus_T *pStatus = NULL;
                Common_Lock(g_hModuleStatusLock);
                pStatus = g_pOfflineModuleList;
                while(pStatus != NULL)
                {
                    if(0 == Common_StriCmp(pStatus->szModuleName, pModuleName_notify))
                    {
                        break;
                    }
                    pStatus = pStatus->pNext;
                }
                if(pStatus != NULL)
                {
                    // 删除
                    if(pStatus->pPrev == NULL)
                    {
                        g_pOfflineModuleList = pStatus->pNext;

                    }
                    else
                    {
                        pStatus->pPrev->pNext = pStatus->pNext;

                    }
                    if(pStatus->pNext != NULL)
                    {
                        pStatus->pNext->pPrev = pStatus->pPrev;
                    }
                    Common_Free(pStatus, __FUNCTION__, __LINE__);
                }
                Common_UnLock(g_hModuleStatusLock);
            }
        }

        return 0;

    }

	if (0 == Common_StriCmp((char *)"/Core/DiskFormat", szUri))
    {
        if (0 == Common_StriCmp((char *)"Put", szMethod))
        {
            if(0 == g_iDiskStatus)
        	{
        		g_iDiskStatus = 1;
				pthread_attr_t attr;
				pthread_t tThreadDisk;
			    pthread_attr_init(&attr);
			    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
				pthread_create(&tThreadDisk,&attr,static_DiskFormat_Thread,NULL);
        	}
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, 0, 0);
				Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL,
                                         0, 0);
				Common_Json_SetAttrValue(pOutJson, -1, "Data/Status", Common_Json_Type_Number, NULL, g_iDiskStatus, 0);
            }
        }
		else if (0 == Common_StriCmp((char *)"Get", szMethod))
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, 0, 0);
				Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL,
                                         0, 0);
				Common_Json_SetAttrValue(pOutJson, -1, "Data/Status", Common_Json_Type_Number, NULL, g_iDiskStatus, 0);
            }
        }

        if (pOutJson != NULL)
        {
            if (pOutParams != NULL)
            {
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }
        }
        else if(pOutParams != NULL)
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, -1, 0);
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }
        }
        Common_Json_Delete(pOutJson);
        pOutJson = NULL;
        return 0;

    }

    if (pOutParams != NULL)
    {
        Common_Json_Delete(*pOutParams);
        *pOutParams = NULL;
    }
    return -1;
}

S32 Core_LoadConfig(ModuleHandle_T hModuleHandle, cJSON_Struct **pOutParam)
{
    if (pOutParam == NULL || *pOutParam != NULL)
    {
        return -1;
    }

    cJSON_Struct *thisCfg = NULL;

    // 使用硬编码方式初始化配置参数.
    cJSON_Struct *codeDefaultCfg = NULL;
    codeDefaultCfg = Common_cJSON_Parse(s_coreDefaultCfg, NULL, NULL);
    if (codeDefaultCfg)
    {
        thisCfg = codeDefaultCfg;
    }

    // 读取默认配置文件,并将其中参数覆盖之前读取的参数.
    cJSON_Struct *fileDefaultCfg = NULL;
    Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Default,
                            &fileDefaultCfg);
    if (fileDefaultCfg)
    {
        JsonOper_MergeObj((Common_cJSON_T *)thisCfg, (Common_cJSON_T *)fileDefaultCfg,
                          0);
        Common_Json_Delete(fileDefaultCfg);
        fileDefaultCfg = NULL;
    }

    // 读取当前配置文件,并将其中参数覆盖之前读取的参数.
    cJSON_Struct *fileCurrentCfg = NULL;
    Module_LoadConfig(hModuleHandle, &fileCurrentCfg);
    if (fileCurrentCfg)
    {
        JsonOper_MergeObj((Common_cJSON_T *)thisCfg, (Common_cJSON_T *)fileCurrentCfg,
                          0);
        Common_Json_Delete(fileCurrentCfg);
        fileCurrentCfg = NULL;
    }
    //LOGW("\n");
    //Common_Json_StandardPrint(thisCfg, NULL, NULL, NULL);

    *pOutParam = thisCfg;

    return 0;
}

S32 Core_Time_LoadConfig(ModuleHandle_T hModuleHandle, cJSON_Struct **pOutParam)
{
    if (pOutParam == NULL || *pOutParam != NULL)
    {
        return -1;
    }

    cJSON_Struct *thisCfg = NULL;

    // 使用硬编码方式初始化配置参数.
    cJSON_Struct *codeDefaultCfg = NULL;
    codeDefaultCfg = Common_cJSON_Parse(s_coreDefaultCfg, NULL, NULL);
    if (codeDefaultCfg)
    {
        thisCfg = codeDefaultCfg;
    }

    // 读取默认配置文件,并将其中参数覆盖之前读取的参数.
    cJSON_Struct *fileDefaultCfg = NULL;
    Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Default,
                            &fileDefaultCfg);
    if (fileDefaultCfg)
    {
        JsonOper_MergeObj((Common_cJSON_T *)thisCfg, (Common_cJSON_T *)fileDefaultCfg,
                          0);
        Common_Json_Delete(fileDefaultCfg);
        fileDefaultCfg = NULL;
    }

    // 读取当前配置文件,并将其中参数覆盖之前读取的参数.
    cJSON_Struct *fileCurrentCfg = NULL;
    Module_LoadConfig(hModuleHandle, &fileCurrentCfg);
    if (fileCurrentCfg)
    {
        JsonOper_MergeObj((Common_cJSON_T *)thisCfg, (Common_cJSON_T *)fileCurrentCfg,
                          0);
        Common_Json_Delete(fileCurrentCfg);
        fileCurrentCfg = NULL;
    }
    LOGW("\n");
    Common_Json_StandardPrint(thisCfg, NULL, NULL, NULL);

    if (thisCfg != NULL)
    {
        *pOutParam = Common_Json_DetachItem(thisCfg, -1, "Time");
    }
    Common_Json_Delete(thisCfg);
    return 0;
}

S32 Core_Time_SaveConfig(ModuleHandle_T hModuleHandle, cJSON_Struct *pOutParam)
{
    cJSON_Struct *pLoadConfig = NULL;
    S32 bChange = 0;

    Module_LoadConfig(hModuleHandle, &pLoadConfig);
    if (pLoadConfig != NULL)
    {
        cJSON_Struct *pTimeJson;
        pTimeJson = Common_Json_DetachItem(pLoadConfig, -1, "Time");
        if (pTimeJson != NULL)
        {
            bChange = 1;
            Common_Json_Delete(pTimeJson);
        }

        if (pOutParam != NULL)
        {
            Common_Json_AddItem(pLoadConfig, -1, "/Time", pOutParam);
            bChange = 1;
        }

    }
    else if (pOutParam != NULL)
    {
        pLoadConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pLoadConfig != NULL)
        {
            Common_Json_AddItem(pLoadConfig, -1, "/Time", pOutParam);
            bChange = 1;
        }
    }
    if (bChange)
    {
        Module_SaveConfig(hModuleHandle, pLoadConfig);
    }
    Common_Json_Delete(pLoadConfig);

    return 0;

}


S32 Core_Version_load(ModuleHandle_T hModuleHandle, Module_ConfigType_E nType,
                      cJSON_Struct **pOutParam)
{
    cJSON_Struct *pLoadConfig = NULL;
    if (pOutParam == NULL || *pOutParam != NULL)
    {
        return -1;
    }
    Module_LoadConfigByType(hModuleHandle, nType, &pLoadConfig);
    if (pLoadConfig != NULL)
    {
        *pOutParam = Common_Json_DetachItem(pLoadConfig, -1, "Version");
    }
    Common_Json_Delete(pLoadConfig);
    return 0;
}

S32 Core_Version_Save(ModuleHandle_T hModuleHandle, cJSON_Struct *pOutParam)
{
    cJSON_Struct *pLoadConfig = NULL;
    S32 bChange = 0;

    Module_LoadConfig(hModuleHandle, &pLoadConfig);
    if (pLoadConfig != NULL)
    {
        cJSON_Struct *pTimeJson;
        pTimeJson = Common_Json_DetachItem(pLoadConfig, -1, "Version");
        if (pTimeJson != NULL)
        {
            bChange = 1;
            Common_Json_Delete(pTimeJson);
        }

        if (pOutParam != NULL)
        {
            Common_Json_AddItem(pLoadConfig, -1, "/Version", pOutParam);
            pOutParam = NULL;
            bChange = 1;
        }

    }
    else if (pOutParam != NULL)
    {
        pLoadConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pLoadConfig != NULL)
        {
            Common_Json_AddItem(pLoadConfig, -1, "/Version", pOutParam);
            pOutParam = NULL;
            bChange = 1;
        }
    }
    if (bChange)
    {
        Module_SaveConfig(hModuleHandle, pLoadConfig);
    }
    Common_Json_Delete(pLoadConfig);
    Common_Json_Delete(pOutParam);

    return 0;
}
S32 Core_Restore_LoadCustom(ModuleHandle_T hModuleHandle,
                            cJSON_Struct **pOutParam)
{
    cJSON_Struct *pLoadConfig = NULL;
    if (pOutParam == NULL || *pOutParam != NULL)
    {
        return -1;
    }
    Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Custom, &pLoadConfig);
    if (pLoadConfig != NULL)
    {
        *pOutParam = Common_Json_DetachItem(pLoadConfig, -1, "Restore");
    }
    Common_Json_Delete(pLoadConfig);
    return 0;
}

S32 Core_Maintenance_LoadConfig(ModuleHandle_T hModuleHandle,
                                cJSON_Struct **pOutParam)
{
    cJSON_Struct *pLoadConfig = NULL;
    if (pOutParam == NULL || *pOutParam != NULL)
    {
        return -1;
    }
    Module_LoadConfig(hModuleHandle, &pLoadConfig);
    if (pLoadConfig != NULL)
    {
        *pOutParam = Common_Json_DetachItem(pLoadConfig, -1, "Maintenance");
    }
    Common_Json_Delete(pLoadConfig);
    return 0;
}

S32 Core_Maintenance_SaveConfig(ModuleHandle_T hModuleHandle,
                                cJSON_Struct *pOutParam)
{
    cJSON_Struct *pLoadConfig = NULL;
    S32 bChange = 0;

    Module_LoadConfig(hModuleHandle, &pLoadConfig);
    if (pLoadConfig != NULL)
    {
        cJSON_Struct *pTimeJson;
        pTimeJson = Common_Json_DetachItem(pLoadConfig, -1, "Maintenance");
        if (pTimeJson != NULL)
        {
            bChange = 1;
            Common_Json_Delete(pTimeJson);
        }

        if (pOutParam != NULL)
        {
            Common_Json_AddItem(pLoadConfig, -1, "/Maintenance", pOutParam);
            bChange = 1;
        }

    }
    else if (pOutParam != NULL)
    {
        pLoadConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pLoadConfig != NULL)
        {
            Common_Json_AddItem(pLoadConfig, -1, "/Maintenance", pOutParam);
            bChange = 1;
        }
    }
    if (bChange)
    {
        Module_SaveConfig(hModuleHandle, pLoadConfig);
    }
    Common_Json_Delete(pLoadConfig);

    return 0;
}

static S32 static_SystemServer_Thread(Common_Thread_T hThreadHandle,
                                      void *pUserData)
{
    Common_SystemServerStart();
    return 0;
}
#define CORE_VIP_CHECK_INTERVAL    10

static S32 g_nBoardSys_CheckCount = 0;
static S32 g_bBoardSys_Ok = 0;
void Core_CheckBoardSysState(ModuleHandle_T hModuleHandle)
{
    S32 nCode = -1;
    cJSON_Struct *pInParam = NULL, *pOutParam = NULL;
    if (g_nBoardSys_CheckCount >= CORE_VIP_CHECK_INTERVAL)
    {
        //  check
        //S32 nCurrTime = 0;
        g_nBoardSys_CheckCount = 0;

        //printf("%s.%d\n",__FUNCTION__,__LINE__);
        pInParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pInParam != NULL)
        {
            Common_Json_SetAttrValue(pInParam, -1, "/Header", Common_Json_Type_Object, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pInParam, -1, "/Header/Method",
                                     Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pInParam, -1, "/Header/Uri", Common_Json_Type_String,
                                     "/BoardSys/Video/StreamState", 0, 0);
            Module_CallFunctions(hModuleHandle, pInParam, &pOutParam, 3000);
            if(pOutParam != NULL)
            {
                // Common_Json_StandardPrint(pOutParam,"check boardsys state<",">\n",NULL);
                Common_Json_GetAttrValue(pOutParam, -1, "/Header/Code", NULL, NULL, &nCode,
                                         NULL);

                Common_Json_Delete(pOutParam);
            }
            Common_Json_Delete(pInParam);
        }
        if(nCode)
        {
            g_bBoardSys_Ok = 0;
        }
        else
        {
            g_bBoardSys_Ok = 1;
        }

    }
    g_nBoardSys_CheckCount++;

    return;
}

void Core_ResetMirror(ModuleHandle_T hModuleHandle)
{
	int nMode = -1, ret = 0;
	cJSON_Struct *pInParam = NULL, *pOutParam = NULL;

	{
		pInParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
		if (pInParam != NULL)
		{
		    Common_Json_SetAttrValue(pInParam, -1, "/Header", Common_Json_Type_Object, NULL, 0, 0);
		    Common_Json_SetAttrValue(pInParam, -1, "/Header/Method", Common_Json_Type_String, "Get", 0, 0);
		    Common_Json_SetAttrValue(pInParam, -1, "/Header/Uri", Common_Json_Type_String, "/Boardsys/Image/Attribute/Device0", 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data", Common_Json_Type_Object, NULL, 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Type", Common_Json_Type_Number, NULL, 10, 0);

		    ret = Module_CallFunctions(hModuleHandle, pInParam, &pOutParam, 3000);

			if(pInParam != NULL)
			{
		    	Common_Json_Delete(pInParam);
				pInParam = NULL;
			}
		    if(pOutParam != NULL)
		    {
		        Common_Json_GetAttrValue(pOutParam, -1, "/Data/Mode", NULL, NULL, &nMode, NULL);
		        Common_Json_Delete(pOutParam);
				pOutParam = NULL;
		    }
		}
	}

	if(nMode != -1)
	{
		//先关闭镜像
		pInParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
		if (pInParam != NULL)
	    {
	        Common_Json_SetAttrValue(pInParam, -1, "/Header", Common_Json_Type_Object, NULL, 0, 0);
	        Common_Json_SetAttrValue(pInParam, -1, "/Header/Method", Common_Json_Type_String, "Put", 0, 0);
	        Common_Json_SetAttrValue(pInParam, -1, "/Header/Uri", Common_Json_Type_String, "/Boardsys/Image/Attribute/Device0", 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data", Common_Json_Type_Object, NULL, 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Type", Common_Json_Type_Number, NULL, 10, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Param", Common_Json_Type_Object, NULL, 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Param/Mode", Common_Json_Type_Number, NULL, 0, 0);

	        ret = Module_CallFunctions(hModuleHandle, pInParam, &pOutParam, 3000);

 			if(pInParam != NULL)
			{
	        	Common_Json_Delete(pInParam);
				pInParam = NULL;
			}
	        if(pOutParam != NULL)
	        {
	            Common_Json_Delete(pOutParam);
				pOutParam = NULL;
	        }
	    }

		Common_Sleep(5, 0);

		//再恢复原来镜像
		pInParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
		if (pInParam != NULL)
	    {
	        Common_Json_SetAttrValue(pInParam, -1, "/Header", Common_Json_Type_Object, NULL, 0, 0);
	        Common_Json_SetAttrValue(pInParam, -1, "/Header/Method", Common_Json_Type_String, "Put", 0, 0);
	        Common_Json_SetAttrValue(pInParam, -1, "/Header/Uri", Common_Json_Type_String, "/Boardsys/Image/Attribute/Device0", 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data", Common_Json_Type_Object, NULL, 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Type", Common_Json_Type_Number, NULL, 10, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Param", Common_Json_Type_Object, NULL, 0, 0);
			Common_Json_SetAttrValue(pInParam, -1, "/Data/Param/Mode", Common_Json_Type_Number, NULL, nMode, 0);

	        ret = Module_CallFunctions(hModuleHandle, pInParam, &pOutParam, 3000);

 			if(pInParam != NULL)
			{
	        	Common_Json_Delete(pInParam);
				pInParam = NULL;
			}
	        if(pOutParam != NULL)
	        {
	            Common_Json_Delete(pOutParam);
				pOutParam = NULL;
	        }
	    }
	}

	return;
}

static APP_NAME_MAP_T * MatchOfflineModule(char *registName)
{
    int n = COMMON_ARRAY_DIM(s_name_map);
    int i = 0;

    if (registName == NULL)
        return NULL;

    for (i = 0;i < n;i++)
    {
        if (strlen(s_name_map[i].registName) == strlen(registName) &&
            strcasecmp(s_name_map[i].registName, registName) == 0)
        {
            /*it will reboot system when some app crash and such file exist, or ovfs_core will respawn the app crashed again*/
            if (access(CORE_DEBUG_APP, F_OK) == 0)
                s_name_map[i].reboot = 1;
            return &s_name_map[i];
        }
    }
    return NULL;
}

static S32 static_CardDetect_Thread(Common_Thread_T hThreadHandle,
                                      void *pUserData)
{
	struct stat statFile;

	while(1)
	{
		if(stat("/dev/mmcblk0", &statFile) != 0)
		{
			LOGE("SD Card Removed\n\n");
			break;
		}

		Common_Sleep(1, 0);
	}

	while(1)
	{
		if(stat("/dev/mmcblk0", &statFile) == 0)
		{
			LOGE("SD Card is Exist\n\n");
			break;
		}

		Common_Sleep(1, 0);
	}

	if (access("/tmp/updating", F_OK) == 0)
	{
		//is updating
	}
	else
	{
		LOGW("SD card Reboot!!!\n");
		Common_System("reboot");
	}


    return 0;
}


S32 main(S32 argc, char *argv[])
{
    ModuleHandle_T hModuleHandle = NULL;
    cJSON_Struct *pConfig;
    //S32 nPos = 0;
    S32 nRet;
	int bNoFrameCount = 0;
    //S8 *pErrorString = NULL;
    //S8 *pEndString = NULL;
    Common_Thread_T hSystemServerThread = NULL;
    //S32 bDogEnable = 0;
    S32 bNeedDog_reboot = 0, bCheckUbootInfo = 0, nCheckUbootCount = 0, bBoardsysStartOK = 0;

    Common_Lock_Create(&g_hModuleStatusLock, "Core_module_status_lock");

    {
        S8 szVipModules[][32] = {"BoardSys"};
        S32 i;
        CoreModuleStatus_T *pStatusList;
        for(i = 0; i < (S32)(sizeof(szVipModules) / sizeof(szVipModules[0])); i++)
        {
            pStatusList = (CoreModuleStatus_T *)Common_Malloc(sizeof(CoreModuleStatus_T), 0,
                          __FUNCTION__, __LINE__);
            if(pStatusList != NULL)
            {
                memset(pStatusList, 0, sizeof(CoreModuleStatus_T));
                strncpy(pStatusList->szModuleName, szVipModules[i], 31);
                pStatusList->pNext = g_pVipModuleList;
                if(g_pVipModuleList != NULL)
                {
                    g_pVipModuleList->pPrev = pStatusList;
                }
                g_pVipModuleList = pStatusList;
            }
        }
    }



    LOG_INIT((S8 *)"Core", COMMON_LOG_LV_HIGH);
    pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig, -1, "SystemName", Common_Json_Type_String,
                                 "ovfs", 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "ModuleName", Common_Json_Type_String,
                                 "Core", 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "LocalPort", Common_Json_Type_Number,
                                 NULL, 10009, 0);
    }

    nRet = Module_Init(&hModuleHandle, pConfig, NULL, static_Module_CallFunctions,
                       NULL);
    if (pConfig)
    {
        Common_Json_Delete(pConfig);
        pConfig = NULL;
    }
    if (nRet < 0)
    {
        if (nRet == MODULE_ERROR_TYPE_RUNNING)
        {
            printf("Module[core] is Running,exit!\n");
        }
        return nRet;
    }
//    Core_Dog_Enable(1);
	Core_Dog_Enable_First(1);

    printf("Module init %d\n", nRet);
    // 启动系统调用服务
    Common_Thread_Create(&hSystemServerThread, "SystemServer", 0, 0,
                         static_SystemServer_Thread, NULL);

    // 初始化版本
    Core_Version_Init(hModuleHandle);

    // 读取配置
    cJSON_Struct *coreConfig = NULL;
    Core_LoadConfig(hModuleHandle, &coreConfig);

    // 初始化时间
    cJSON_Struct *timeCfg = Common_Json_GetAttrValueObj(coreConfig, "Time");
    Core_Time_Init(hModuleHandle, timeCfg);
    // 电源管理
    Core_Power_Init(hModuleHandle);
    // 恢复默认
    Core_Restore_Init(hModuleHandle);
    // 维护
    cJSON_Struct *maintainCfg = Common_Json_GetAttrValueObj(coreConfig,
                                "Maintenance");
    Core_Maintenance_Init(hModuleHandle, maintainCfg);
    // 配置导入导出
    Core_Export_Init(hModuleHandle);
    Common_GetSystemCount(&g_nLastRebootTime, NULL);

    if (coreConfig)
    {
        Common_Json_Delete(coreConfig);
        coreConfig = NULL;
    }

	Common_Thread_T hCardThread = NULL;
//	Common_Thread_Create(&hCardThread, "hCardThread", 0, 0,
//                         static_CardDetect_Thread, NULL);

	libaccess_sdk_init();
	LOGD("call libaccess_sdk_init\n");
#ifdef AOV
    LOGW("IS AOV\n");
#endif

#ifndef NOALARM
	libalarm_sdk_init();
	LOGD("call libalarm_sdk_init\n");
#else
    LOGW("NOALARM\n");
#endif
    //
    while(1)
    {
        if (!bCheckUbootInfo)
        {
            nCheckUbootCount++;
            if (nCheckUbootCount > 60)
            {
                S32 nCurrCount = 0;
                nCheckUbootCount = 0;
                Common_GetSystemCount(&nCurrCount, NULL);
                if (nCurrCount >= g_nLastRebootTime + 2 * 60)
                {
                    Version_WriteInfoToUbootEnv();
                    bCheckUbootInfo = 1;
                }
            }

        }

        Core_CheckBoardSysState(hModuleHandle);
        Common_Lock(g_hModuleStatusLock);
        if(g_pOfflineModuleList != NULL || (!g_bBoardSys_Ok))
        {
            S8 *pOffModuleName = NULL;
            APP_NAME_MAP_T * app = NULL;

            if (g_pOfflineModuleList != NULL)
            {
                pOffModuleName = g_pOfflineModuleList->szModuleName;
                app = MatchOfflineModule(pOffModuleName);
            }

			if (g_bBoardSys_Ok == 0) /*not found or need reboot*/
            {
                if(!bNeedDog_reboot)
                {
                    printf("boardsystem status[%d]\n", g_bBoardSys_Ok);
                    bNeedDog_reboot = 1;
                }
            }
			if (app != NULL && app->reboot == 1) /*not found or need reboot*/
            {
                if(!bNeedDog_reboot)
                {
                    printf("Check and Need dog to reboot .off[%d][%s]\n",
                           g_pOfflineModuleList != NULL, pOffModuleName ? pOffModuleName : "");
                    bNeedDog_reboot = 1;
                }
            }
            else if (app != NULL && app->reboot == 0)  /*respawn app*/
            {
                CoreModuleStatus_T *pStatus = g_pOfflineModuleList;

                if(pStatus != NULL)
                {
                    // 删除
                    if(pStatus->pPrev == NULL)
                    {
                        g_pOfflineModuleList = pStatus->pNext;
                    }
                    else
                    {
                        pStatus->pPrev->pNext = pStatus->pNext;

                    }
                    if(pStatus->pNext != NULL)
                    {
                        pStatus->pNext->pPrev = pStatus->pPrev;
                    }
                    Common_Free(pStatus, __FUNCTION__, __LINE__);
                }

                char cmd[128] = {};

                if (strcasecmp("NetWork", app->registName) == 0 &&
                    access("/root/bin/ovfs_network", F_OK) != 0)
                {
                    snprintf(cmd,sizeof(cmd),"pkill -9 \"ovfs_webserver$\";ovfs_webserver &");
                }
                else
                {
                	snprintf(cmd,sizeof(cmd),"pkill -9 \"%s$\";%s &",app->binaryName,app->binaryName);
                }
                LOGE("respawn app %s \n",cmd);
                Common_System(cmd);
            }
			else if (app == NULL)
			{
				//预留
			}

        }
        else if(bNeedDog_reboot)
        {
            printf("Check and cancel dog to reboot .off[%d] vip[%d]\n",
                   g_pOfflineModuleList != NULL, g_bBoardSys_Ok);
            bNeedDog_reboot = 0;
        }
		if(!g_bBoardSys_Ok)
		{
			bNoFrameCount++;
		}
		else
		{
			bNoFrameCount = 0;
		}
        Common_UnLock(g_hModuleStatusLock);

		if (access("/tmp/updating", F_OK) == 0)
		{
			//LOGE("is updateing\n");
		}
        else if(!bNeedDog_reboot)
        {
			if(!bBoardsysStartOK)
			{
				LOGW("Boardsys ok restart watchdog\n");
				Core_Dog_Enable(0);
				Core_Dog_Enable(1);
				bBoardsysStartOK = 1;
			}
            Core_Dog_Feed();
        }
		if(bNoFrameCount >= 20)
		{
			if(!Common_StriCmp(g_tVersion.szSensorModel,"GC2083") && (access("/tmp/updating", F_OK)))
			{
				Common_System("/update/gc2083.sh &");
				Common_Sleep(5, 0);
				Core_ResetMirror(hModuleHandle);
			}
			bNoFrameCount = 0;
		}
        Core_CheckCopyRight(hModuleHandle);
        Common_Sleep(1, 0);
    }

	libaccess_sdk_uninit();
	LOGD("call libaccess_sdk_uninit\n");

#ifndef NOALARM
	libalarm_sdk_uninit();
	LOGD("call libalarm_sdk_uninit\n");
#endif

    Common_SystemServerStop();
    return 0;
}
