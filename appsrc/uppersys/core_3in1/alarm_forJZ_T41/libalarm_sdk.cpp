#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/**********/
#include <dlfcn.h>
#include <signal.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>

#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "ovfs_alarm_common.h"
#include "ovfs_alarm_cfg.h"
#include "ovfs_alarm_method.h"
#include "ovfs_alarm.h"
#include "ovfs_linkage.h"
#include "libalarm_sdk.h"

static Common_Lock_T g_RestLock;
static ModuleHandle_T g_hModuleHandle = NULL;

static int AnalyzeDataAndMakeResult(cJSON_Struct *dataJson,cJSON_Struct **pOutData)
{
    int iRet = -1;
    char *pUri = NULL,*pMethod = NULL;
    cJSON_Struct *pData = NULL,*pChild = NULL;
    if(!dataJson||!pOutData)
    {
        return -1;
    }
    pChild = Common_Json_GetAttrValue(dataJson, -1, "Header/Method", NULL, &pMethod, NULL, NULL);
    if(!pChild)
    {
        return iRet;
    }
    pChild = Common_Json_GetAttrValue(dataJson, -1, "Header/Uri", NULL, &pUri, NULL, NULL);
    if(!pChild)
    {
        return iRet;
    }
    pData = Common_Json_GetItem(dataJson,-1,"Data");
    if(pMethod)
    {
        Common_Lock(g_RestLock);
        if(0 == Common_StriCmp(pMethod,(S8*)"get"))
        {
            iRet = ovfs_get_alarm_res(pUri,pData,pOutData);
        }
        else if (0 == Common_StriCmp(pMethod,(S8*)"put"))
        {
            iRet = ovfs_put_alarm_res(pUri,pData,pOutData);
        }
        else if(0 == Common_StriCmp(pMethod,(S8*)"post"))
        {
            iRet = ovfs_post_alarm_res(pUri,pData,pOutData);
        }
        else if (0 == Common_StriCmp(pMethod,(S8*)"delete"))
        {
            iRet = ovfs_delete_alarm_res(pUri,pData,pOutData);
        }
        Common_UnLock(g_RestLock);
    }
    return iRet;
}

static S32 static_Module_CallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
    int iRet = 0,bPrintDbg = 0;
    if (pInParams)
    {
        bPrintDbg =ovfs_get_debug();
        if(bPrintDbg)
        {
            ovfs_print_json(pInParams);
        }
        AnalyzeDataAndMakeResult(pInParams,pOutParams);
    }
    return iRet;
}

void SignalHandleAlarm(int param)
{
    LOGD("rec SIGINT! now exit\n");
}

void SignalPipeAlarm(int param)
{
    LOGD("rec SIGPIPE!\n");
}

S32 libalarm_sdk_init()
{
    S8 *pModuleName = (S8*)"Alarm";
    cJSON_Struct *pConfig;
    S32 nRet = -1;

    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig,-1,"SystemName",Common_Json_Type_String,"ovfs",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"ModuleName",Common_Json_Type_String,pModuleName,0,0);
    }
    Module_Init_Ex(&g_hModuleHandle,pConfig,NULL,static_Module_CallFunctions,NULL);
    if(pConfig)
        Common_Json_Delete(pConfig);
    do
    {
        nRet = ovfs_init_ability(g_hModuleHandle);
        if(0 != nRet)
        {
            LOGW("======Retry request ability======\n");
            Common_Sleep(1,0);
        }
    }
    while(nRet != 0);
    ovfs_load_alarm_cfg(g_hModuleHandle);
    ovfs_init_alarm_bkbd(g_hModuleHandle);
#ifndef SIMPLIFIED
    ovfs_initLink(g_hModuleHandle);
#endif
    ovfs_init_alarm_res(g_hModuleHandle);
    ovfs_init_subscribe(g_hModuleHandle);

    Common_Lock_Create(&g_RestLock,NULL);

    //Common_Thread_Create(&g_hCfgThread,__FUNCTION__,0,0,Thread_SaveCfg,hModuleHandle);

    return 0;
}


S32 libalarm_sdk_uninit()
{
    return 0;
}

