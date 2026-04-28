#include <string.h>
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "core_power.h"
#include "core_restore.h"
/*
Uri:/Core/Power/Reboot  // 延时 10秒或者 指定时间重启/关机
Uri:/Core/Power/ShutDown
Uri:/Core/Power/Cancel // 取消所有操作
Method:Put
InData:{
Delay=10, // 10 秒后关机
Time=20170322100000, // 指定时间关机 ，如果Delay 同时存在，以最近的为主
Prompt = 20,//关机前 20 秒提示，
PromptInterval=1 //间隔时间为 1秒一次.
Force=1// 0-可取消,1-不可取消
}



Uri:/模块/Notify/Power/Reboot  // 通知各模块消息
Uri:/模块/Notify/Power/ShutDown
Uri:/模块/Notify/Power/Cancel  // 通知,操作被取消
Method:Put
InData:{Delay=10,Time=,}


Uri:/Core/Reset   // 复位键

*/
typedef struct _tagCorePowerMgr
{
    S32 nType; // 0- non,1-reboot,2-ShutDown;
    S32 nAfterSec;// -1 无效 0- 立即 , 秒，多少秒后操作
    S32 bTimeValid; // 时间是否有效,0-无效
    Common_Time_T tTime; // 指定时间关机
    S32 nBeforePromptSec; // 提前多少秒提示,0-不提示
    S32 nPromptIntervalSec; // 提示间隔,0- 提示一次，
    S32 bForce;
    S32 bChange;// 配置改变
    Common_Lock_T hLock;
    Common_Thread_T hThread;
    S32 bExitThread;
} CorePowerMgr_T;

static CorePowerMgr_T g_tPowerMgr;
static S32 g_bPowerInit = 0;
S32 Core_Power_CallFunctions(ModuleHandle_T hModuleHandle,
                             cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{

    if (pInParams != NULL)
    {
        char *pStr = Common_Json_Print(pInParams, NULL);
        if (pStr)
        {
            printf("[%s.%d]in <%s>\n", __FUNCTION__, __LINE__, pStr);
            Common_Free(pStr, __FUNCTION__, __LINE__);
        }
    }

    S32 nRet = -1;
    S8 *szUri = NULL;
    S8 *szMethod = NULL;
    cJSON_Struct *pOutJson = NULL;
    S32 nCurrType = 0;
    Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &szUri, NULL, NULL);

    if(0 != Common_StrniCmp((char *)"/Core/Power", szUri, 11) &&
            0 != Common_StriCmp((char *)"/Core/Reset", szUri))
    {
        return -1;
    }
    Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &szMethod, NULL,
                             NULL);

    Core_Power_Init(hModuleHandle);
    do
    {
        if (0 == Common_StriCmp((char *)"Get", szMethod))
        {
            if(0 == Common_StriCmp((char *)"/Core/Power", szUri))
            {
                cJSON_Struct *pNode, *pNode1;
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

                    Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String,
                                             "/Core/Power/Reboot", 0, 0);
                    Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "Reboot",
                                             0, 0);
                    Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None",
                                             0, 0);
                    pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array,
                                                      NULL, 0, 0);
                    Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);

                    Common_Json_SetAttrValue(pNode, 1, "/Uri", Common_Json_Type_String,
                                             "/Core/Power/ShutDown", 0, 0);
                    Common_Json_SetAttrValue(pNode, 1, "/Label", Common_Json_Type_String,
                                             "ShutDown", 0, 0);
                    Common_Json_SetAttrValue(pNode, 1, "/Describe", Common_Json_Type_String, "None",
                                             0, 0);
                    pNode1 = Common_Json_SetAttrValue(pNode, 1, "/Method", Common_Json_Type_Array,
                                                      NULL, 0, 0);
                    Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);

                    Common_Json_SetAttrValue(pNode, 2, "/Uri", Common_Json_Type_String,
                                             "/Core/Power/Cancel", 0, 0);
                    Common_Json_SetAttrValue(pNode, 2, "/Label", Common_Json_Type_String, "Cancel",
                                             0, 0);
                    Common_Json_SetAttrValue(pNode, 2, "/Describe", Common_Json_Type_String, "None",
                                             0, 0);
                    pNode1 = Common_Json_SetAttrValue(pNode, 2, "/Method", Common_Json_Type_Array,
                                                      NULL, 0, 0);
                    Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
                }
                nRet = 0;
                break;
            }

        }

        if(0 != Common_StriCmp((char *)"Put", szMethod))
        {
            nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
            break;
        }
        if (0 == Common_StriCmp((char *)"/Core/Reset", szUri))
        {
            int iRet = 0;
            cJSON_Struct *pResult = NULL,*pRoot = NULL;
            char acSoundPath[128] = {0};
            pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
            if(pRoot)
            {
                Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);

                Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/BoardSys/Audio/Adec/PlayFile",0,0);

                Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);

                Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);

                sprintf(acSoundPath, "%s", "/update/soundFile/sound_reset");
                Common_Json_SetAttrValue(pRoot,-1,"Data/Path",Common_Json_Type_String,acSoundPath,0,0);
                Common_Json_SetAttrValue(pRoot,-1,"Data/Times",Common_Json_Type_Number,NULL,1,0);
                Common_Json_SetAttrValue(pRoot,-1,"Data/Priority",Common_Json_Type_Number,NULL,1,0);
                iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
                if(iRet < 0)
                {
                    LOGE("err:%d\n",iRet);
                }
                Common_Json_Delete(pRoot);
                Common_Json_Delete(pResult);

                Common_Sleep(2,0);
            }

            // 恢复默认
            S32 nNeedRestore = 0;
            cJSON_Struct *pRequrieJson = NULL;
            //cJSON_Struct *pReqOut = NULL;
            Common_Json_GetAttrValue(pInParams, -1, "/Data/NeedRestore", NULL, NULL,
                                     &nNeedRestore, NULL);
            if(nNeedRestore)
            {
                // ptz
                pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if (pRequrieJson != NULL)
                {
                    S8 szTmp[128];
                    sprintf(szTmp, "/ptz/restore");
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                             Common_Json_Type_String, szTmp, 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Method",
                                             Common_Json_Type_String, "Put", 0, 0);
                    Module_CallFunctions(hModuleHandle, pRequrieJson, NULL, 3000);
                    Common_Json_Delete_ex(pRequrieJson, __FUNCTION__, __LINE__);
                    pRequrieJson = NULL;
                }

                pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if (pRequrieJson != NULL)
                {
                    S8 szTmp[128];
                    sprintf(szTmp, "/Core/Restore/All");
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                             Common_Json_Type_String, szTmp, 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Method",
                                             Common_Json_Type_String, "Put", 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Data", Common_Json_Type_Object,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Data/NeedReboot",
                                             Common_Json_Type_Number, NULL, 1, 0);
                    Core_Restore_CallFunctions(hModuleHandle, pRequrieJson, NULL);
                    Common_Json_Delete_ex(pRequrieJson, __FUNCTION__, __LINE__);
                    pRequrieJson = NULL;
                }
            }
            else
            {
                Common_Lock(g_tPowerMgr.hLock);
                g_tPowerMgr.nType = 1; //
                g_tPowerMgr.bForce = 1;
                g_tPowerMgr.bChange = 1;
                Common_UnLock(g_tPowerMgr.hLock);
            }


        }
        else if (0 == Common_StriCmp((char *)"/Core/Power/Cancel", szUri) )
        {
            cJSON_Struct *pRequrieJson = NULL;
            //cJSON_Struct *pReqOut = NULL;
            Common_Lock(g_tPowerMgr.hLock);
            if (!g_tPowerMgr.bForce)
            {
                g_tPowerMgr.nType = 0; // 取消
                g_tPowerMgr.bChange = 1;
                nRet = 0;
            }
            else
            {
                nRet = MODULE_ERROR_TYPE_LIMITED;
            }

            Common_UnLock(g_tPowerMgr.hLock);

            if (!g_tPowerMgr.bForce)
            {
                pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if (pRequrieJson != NULL)
                {
                    // 广播
                    S8 szTmp[128];
                    sprintf(szTmp, "/Broadcast/Notify/Power/Cancel");
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                             Common_Json_Type_String, szTmp, 0, 0);
                    Module_CallFunctions(hModuleHandle, pRequrieJson, NULL, 3000);
                }
            }

            break;
        }
        if (0 == Common_StriCmp((char *)"/Core/Power/Reboot", szUri) )
        {
            nCurrType = 1;
        }
        else if (0 == Common_StriCmp((char *)"/Core/Power/ShutDown", szUri) )
        {
            nCurrType = 2;
        }
        if (nCurrType > 0)
        {
            S32 nDelay = -1, nPrompt = 0, nPromptInterval = 0, bForce = 0;
            S8 *szTime = NULL;
            Common_Json_GetAttrValue(pInParams, -1, "Data/Delay", NULL, NULL, &nDelay,
                                     NULL);
            Common_Json_GetAttrValue(pInParams, -1, "Data/Time", NULL, &szTime, NULL, NULL);
            Common_Json_GetAttrValue(pInParams, -1, "Data/Prompt", NULL, NULL, &nPrompt,
                                     NULL);
            Common_Json_GetAttrValue(pInParams, -1, "Data/PromptInterval", NULL, NULL,
                                     &nPromptInterval, NULL);
            Common_Json_GetAttrValue(pInParams, -1, "Data/Force", NULL, NULL, &bForce,
                                     NULL);
            Common_Lock(g_tPowerMgr.hLock);
            g_tPowerMgr.nType = nCurrType;
            g_tPowerMgr.nAfterSec = nDelay;
            g_tPowerMgr.nBeforePromptSec = nPrompt;
            g_tPowerMgr.nPromptIntervalSec = nPromptInterval;
            g_tPowerMgr.bForce = bForce;

            g_tPowerMgr.bTimeValid = 0;
            if (szTime != NULL)
            {
                int nYear = 0, nMon = 0, nDay = 0, nHour = 0, nMin = 0, nSec = 0;
                if(6 == sscanf(szTime, "%04d%02d%02d%02d%02d%02d", &nYear, &nMon, &nDay, &nHour,
                               &nMin, &nSec))
                {
                    g_tPowerMgr.tTime.year = nYear;
                    g_tPowerMgr.tTime.month = nMon;
                    g_tPowerMgr.tTime.day = nDay;
                    g_tPowerMgr.tTime.hour = nHour;
                    g_tPowerMgr.tTime.min = nMin;
                    g_tPowerMgr.tTime.sec = nSec;
                    g_tPowerMgr.bTimeValid = 1;

                }
            }
            g_tPowerMgr.nType = nCurrType;
            g_tPowerMgr.bChange = 1;
            Common_UnLock(g_tPowerMgr.hLock);
            nRet = 0;


        }

    }
    while (0);
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
                                     NULL, nRet, 0);
            *pOutParams = pOutJson;
            pOutJson = NULL;
        }

    }
    Common_Json_Delete(pOutJson);
    pOutJson = NULL;
    return 0;
}
static S32 static_Common_Thread_def(Common_Thread_T hThreadHandle,
                                    void *pUserData)
{
    S32 nType = 0; // 0- non,1-reboot,2-ShutDown;
    S32 nAfterSec = 0;// -1 无效 0- 立即 , 秒，多少秒后操作
    S32 bTimeValid = 0; // 时间是否有效,0-无效
    Common_Time_T tTime; // 指定时间关机
    //S32 nBeforePromptSec = 0; // 提前多少秒提示,0-不提示
    //S32 nPromptIntervalSec = 0; // 提示间隔,0- 提示一次，
    //S32 bChange = 0;// 配置改变
    //S32 nPromptCount = 0;
    S32 bPrompt = 0, bDoPower = 0;
    U32 uTime = 0, uCurrTime = 0;
    ModuleHandle_T hModuleHandle = (ModuleHandle_T)pUserData;
    while(1)
    {
        if (g_tPowerMgr.bExitThread)
        {
            break;
        }
        Common_Sleep(1, 0);
        bPrompt = 0;
        bDoPower = 0;
        Common_Lock(g_tPowerMgr.hLock);
        nType = g_tPowerMgr.nType;
        nAfterSec = g_tPowerMgr.nAfterSec;
        bTimeValid = g_tPowerMgr.bTimeValid;
        tTime = g_tPowerMgr.tTime;
        if (bTimeValid)
        {
            Common_GetCurrentTime((S32 *)&uCurrTime, NULL);
            uTime = Common_GetLocalTime(&tTime) / 1000;
            if (uTime <= uCurrTime)
            {
                nAfterSec = 0;
            }
            else
            {
                if ((S32)(uTime - uCurrTime) < nAfterSec)
                {
                    nAfterSec = uTime - uCurrTime;
                }
            }
        }
        //nBeforePromptSec = g_tPowerMgr.nBeforePromptSec;
        //nPromptIntervalSec = g_tPowerMgr.nPromptIntervalSec;
        //bChange = g_tPowerMgr.bChange;
        // 更新状态
        g_tPowerMgr.bChange = 0;
        if (g_tPowerMgr.nBeforePromptSec > 0)
        {
            if (g_tPowerMgr.nAfterSec <= g_tPowerMgr.nBeforePromptSec)
            {
                bPrompt = 1;
                g_tPowerMgr.nBeforePromptSec = g_tPowerMgr.nAfterSec;
            }
            g_tPowerMgr.nBeforePromptSec -= g_tPowerMgr.nPromptIntervalSec;
        }

        if (g_tPowerMgr.nAfterSec > 0)
        {
            g_tPowerMgr.nAfterSec--;
        }
        else
        {
            bDoPower = 1;
        }


        Common_UnLock(g_tPowerMgr.hLock);
        if (nType <= 0)
        {
            continue;
        }
        if ((bPrompt && nType < 3 && nType > 0) || bDoPower)
        {
            S8 szTmp[128];
            if (nType == 1)
            {
                sprintf(szTmp, "/Broadcast/Notify/Power/Reboot");
            }
            else if (nType == 2)
            {
                sprintf(szTmp, "/Broadcast/Notify/Power/ShutDown");
            }
            cJSON_Struct *pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object,
                                         NULL, 0, 0);
            if (pRequrieJson != NULL)
            {
                // 广播
                Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                         NULL, 0, 0);
                Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                         Common_Json_Type_String, szTmp, 0, 0);
                Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Method",
                                         Common_Json_Type_String, "Put", 0, 0);
                Common_Json_SetAttrValue(pRequrieJson, -1, "Data", Common_Json_Type_Object,
                                         NULL, 0, 0);
                Common_Json_SetAttrValue(pRequrieJson, -1, "Data/Delay",
                                         Common_Json_Type_Number, NULL, nAfterSec, 0);
                Module_CallFunctions(hModuleHandle, pRequrieJson, NULL, 3000);
                Common_Json_Delete(pRequrieJson);
                pRequrieJson = NULL;
#if (defined PLATFORM_HI3516CV300) || (defined PLATFORM_HI3516E)
                int retryCnt = 4;
                do
                {
                    if (access("/tmp/BoardSysJsonDone", F_OK) == 0)
                    {
                        break;
                    }

                    Common_Sleep(3,0);
                    retryCnt--;
                }while(retryCnt > 0);
#endif
            }
        }
        if (bDoPower)
        {
            if (nType == 1)
            {
                LOGI("The Device is Reboot\n");
                // reboot已经包含sync的操作,不用额外调用.
                if (access("/tmp/updating", F_OK) == 0)
				{
					LOGE("is updateing\n");
				}
				else
				{
                    LOGW("/tmp/updatingis existed!\n");
                	Common_System("reboot");
				}

            }
            else if (nType == 2)
            {
                LOGI("The Device is halt\n");
                // halt已经包含sync的操作,不用额外调用.
                Common_System("halt");

            }
            // 重启了就重置状态
            nType = 0;
        }

    }
    return 0;
}
S32 Core_Power_Init(ModuleHandle_T hModuleHandle)
{
    if (g_bPowerInit)
    {
        return 0;
    }
    memset(&g_tPowerMgr, 0, sizeof(g_tPowerMgr));
    Common_Lock_Create(&g_tPowerMgr.hLock, "Core_Power_lock");
    Common_Thread_Create(&g_tPowerMgr.hThread, "Core_Power_Thread", 0, 0,
                         static_Common_Thread_def, hModuleHandle);
    g_bPowerInit = 1;
    return 0;
}
