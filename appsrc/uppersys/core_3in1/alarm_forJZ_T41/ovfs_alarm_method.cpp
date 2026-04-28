/*
 * ants_web.c
 *
 *  Created on: 2016-8-1
 *      Author: eric
 */

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>

#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "ovfs_alarm_common.h"
#include "ovfs_alarm_cfg.h"
#include "ovfs_linkage.h"
#include "ovfs_alarm.h"
#include "ovfs_alarm_method.h"

#define DELIM "/"

static ModuleHandle_T g_hModuleHandle;
static cJSON_Struct *g_rest_res = NULL;
static OVFS_ALARM_BKBD *g_alarmBkbd = NULL;
static S32 g_bCfgChange = 0,g_bPrintDbg = 0,g_iRestTime = 0,g_iDriveThread = 0;
Common_Lock_T g_bkbdLock;

//过滤条件
S32 FilterCondithon(cJSON_Struct *pData,S32 iIndex,cJSON_Struct *pCondition)
{
    S32 type = 0;
    S32 intValue = 0;
    S8 *pValue = NULL;
    S8 *pString = NULL;
    double floatValue = 0.0;
    S8 *pStringValue = NULL;
    cJSON_Struct *jItem = NULL;
    cJSON_Struct *jChild = NULL;
    cJSON_Struct *jRoot = NULL;

    if(!pCondition||!pData)
    {
        return 0;
    }

    if(-1 == iIndex)
        jRoot = pData;
    else
        jRoot = Common_Json_GetItem(pData, iIndex, NULL);
    if(!jRoot)
    {
        LOGE("jRoot is null\n");
        return -1;
    }
    jChild = Common_Json_GetFirstChild(pCondition);
    while(jChild)
    {
        if(-1 == Common_Json_GetAttr(jChild,NULL,&pString,NULL,&pValue,NULL,NULL))
        {
            return 0;
        }
        //ovfs_print_json(jRoot);
        jItem = Common_Json_GetItem(jRoot, -1, pString);
        if(!pString)
        {
            LOGE("pString is NULL\n");
            return -1;
        }
        if(jItem)
        {
            if(-1 == Common_Json_GetAttr(jItem,NULL,NULL,&type,&pStringValue,&intValue,&floatValue))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
        if(type == Common_Json_Type_String)
        {
            if(0 == Common_StriCmp((S8*)"all",pValue))
            {

            }
            else if(0 != Common_StriCmp(pStringValue,pValue))
            {
                return -1;
            }
        }
        else if(type == Common_Json_Type_Number)
        {
            if(-1 == atoi(pValue))
            {

            }
            else if(intValue != atoi(pValue))
            {
                return -1;
            }
        }
        else if(type == Common_Json_Type_Double)
        {
            if(-1 == atoi(pValue))
            {

            }
            else if(floatValue != atof(pValue))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
        jItem = Common_Json_GetNext(jChild);
        jChild = jItem;
    }
    return 0;
}

//返回最后一个出现指定字符的地址
S8 *FindLast(S8 *pData,S8 c)
{
    if(!pData)
        return NULL;
    int i = 0,iLstIdex = 0,iCount = 0;
    S8 *result = NULL;

    while(pData[i] != '\0')
    {
        if(pData[i] == c)
        {
            iCount++;
            iLstIdex = i;
        }
        i++;
    }
    result = pData+iLstIdex;
    return result;
}

S32 MakeResult(const S8 *pFileDes,const S8 *pFuncDes,S32 nLine,S32 iCode,const S8 *pDes,cJSON_Struct *pData,cJSON_Struct **pOutData)
{
    cJSON_Struct *pResult = NULL,*pItem = NULL;
    S8 sDes[128] = {0};
    if(!pOutData)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,iCode,0);
        if(iCode < 0)
        {
            snprintf(sDes,sizeof(sDes),"[%s][%s][%d] %s",pFileDes,pFuncDes,nLine,pDes);
            Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sDes,0,0);
        }
        else
        {
            Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,pDes,0,0);
        }
        pItem = Common_Json_Duplicate(pData, 1);
        Common_Json_AddItem(pResult,-1,"Data",pItem);
    }
    *pOutData = pResult;
    return 0;
}

/*int getIndexByDevChan(int iType,int iDev,int iChan)
{
    int i = 0,j = 0,iChanIndex = 0;
    int iTotalDevNum = 0,iTotalChanNum = 0,iChanNum = 0;
    OVFS_ABILITY * pAbility = ovfs_get_ability();
    OVFS_DEV_ABILITY *pDev = NULL;

    if(!pAbility)
        return -1;
    iTotalDevNum = pAbility->devNum;
#ifndef SIMPLIFIED
    if(0 == iType)
    {
        iTotalChanNum = pAbility->alarmInNum;
    }
    else
#endif//ifndef SIMPLIFIED
    {
        iTotalChanNum = pAbility->totalChanNum;
    }

    if(iDev >= iTotalDevNum)
        return -1;
    for(i = 0; i <= iDev; i++)
    {
#ifndef SIMPLIFIED
        if(0 == iType)
        {
            iChanNum = pAbility->alarmInNum;
        }
        else
#endif//ifndef SIMPLIFIED
        {
            pDev = pAbility->pDevAbility[i];
            if(!pDev)
                continue;
            iChanNum = pDev->chanNum;
        }
        if(iChan >= iChanNum)
            return -1;
        for(j = 0; j <= iChan; j++)
        {
            iChanIndex++;
        }
    }
    if(iChanIndex > iTotalChanNum)
        return -1;
    return (iChanIndex-1);
}*/

S32  set_method_callback(cJSON_Struct **pData,OVFS_GET_METHOD pGetMethod,OVFS_PUT_METHOD pPutMethod,OVFS_POST_METHOD pPostMethod,OVFS_DELETE_METHOD pDelMethod,void *pUserData,int DataLen)
{
    OVFS_REST_METHOD *pRestMethod = NULL;
    pRestMethod = (OVFS_REST_METHOD *)Common_Malloc(sizeof(OVFS_REST_METHOD),0,__FUNCTION__,__LINE__);
    if(!pRestMethod)
        return -1;
    memset(pRestMethod,0,sizeof(OVFS_REST_METHOD));
    pRestMethod->ovfs_get_method = pGetMethod;
    pRestMethod->ovfs_put_method = pPutMethod;
    pRestMethod->ovfs_post_method = pPostMethod;
    pRestMethod->ovfs_delete_method = pDelMethod;
    if(pUserData && DataLen)
    {
        pRestMethod->pUserData = Common_Malloc(DataLen,0,__FUNCTION__,__LINE__);
        memcpy(pRestMethod->pUserData,pUserData,DataLen);
    }
    Common_Json_SetItemExtData(*pData, (void *)pRestMethod, sizeof(OVFS_REST_METHOD));
    if(pRestMethod && pRestMethod->pUserData)
        Common_Free(pRestMethod->pUserData,__FUNCTION__,__LINE__);
    Common_Free(pRestMethod,__FUNCTION__,__LINE__);
    pRestMethod = NULL;
    return 0;
}

int get_alarm_top_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S8 sErrorDes[256] = {0};
    S32 i = 0,iCode = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

    S8 label[][64] = {"Arming","PushConfig","EmailCfg","FtpCfg","AlarmIn","Motion","Vhide","RegionalInvasion","DetectWire",
                        "PersonStaying","DetectAbsent","ParkingViolation","Retrograde","SensorAlarm","SetCustomAudio",
                        "CollectData","Status","Subscribe","Restore","PrintDebug"
        };
    S8 uri[][64] = {"/Alarm/Arming","/Alarm/PushConfig","/Alarm/EmailCfg","/Alarm/FtpCfg","/Alarm/AlarmIn","/Alarm/Motion","/Alarm/Vhide",
                    "/Alarm/RegionalInvasion","/Alarm/DetectWire","/Alarm/PersonStaying",
                    "/Alarm/DetectAbsent","/Alarm/ParkingViolation","/Alarm/Retrograde","/Alarm/SensorAlarm","/Alarm/SetCustomAudio",
                    "/Alarm/CollectData","/Alarm/Status","/Alarm/Subscribe","/Alarm/Restore","/Alarm/PrintDebug"
                   };

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,iCode,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sErrorDes,11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(label); i++)
        {
            Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,label[i],0,0);
            Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,uri[i],0,0);
            iArrayCount++;
        }
    }
    *pOutData = pResult;
    return 0;
}

int get_alarm_debug_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pResult = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Data/PrintDebug",Common_Json_Type_Number,NULL,g_bPrintDbg,0);
    }
    *pOutData = pResult;
    return 0;
}

int put_alarm_debug_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData;

    Common_Json_GetAttrValue(pJData, -1, "PrintDebug",NULL, NULL, &iValue, NULL);
    g_bPrintDbg= iValue;
    ovfs_make_result(0,"Succ",NULL,pOutData);
    return 0;

}
static int after_withdraw_garrison(void)
{
    OVFS_ABILITY * pAbility = ovfs_get_ability();
    int alarmOutNum = pAbility->alarmOutNum;
    int i;
    char uri[128];
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    for(i = 0; i < alarmOutNum; i++)
    {
        pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        if (pRoot)
        {
            sprintf(uri, "/BoardSys/alarmout/function/channel%d/disable",i);
            Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,uri,0,0);
            Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
            int iRet = Module_CallFunctions(g_hModuleHandle, pRoot, &pResult, 3000);
            if(iRet < 0)
            {
                LOGE("err:%d\n",iRet);
            }
            Common_Json_Delete(pRoot);
            Common_Json_Delete(pResult);
            pResult = NULL;
            pRoot = NULL;
        }
    }
    return 0;
}

int get_alarm_arming_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,alarmCfg->enable,0);
    }
    *pOutData = pResult;
    return 0;
}

int put_alarm_arming_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iIndex = 0,iObjType = 0,iSize = 0,iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pItem = NULL;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);

    if(iSize > 0)
    {
        pItem = Common_Json_GetAttrValue(pJData, iIndex, "Enable",NULL, NULL, &iValue, NULL);
        if(pItem&&(iValue!=alarmCfg->enable))
        {
            alarmCfg->enable = iValue;
            g_bCfgChange = 1;
            if(0 == iValue)
            {
                after_withdraw_garrison();

                /*char path[128];
                int i = 0;
                for(i=0; i<8; i++)
                {
                    sprintf(path, "/update/soundFile/sound_%d", i);
                    Action_SoundAlarm(g_hModuleHandle,i,path,0,0);
                }*/

                ovfs_reset_alarm_bkbd();
            }
            ovfs_save_alarm_cfg(g_hModuleHandle);
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    return 0;

}

#ifndef SIMPLIFIED
int get_alarm_email_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iArrayCount = 0,i = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArray1 = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

        pChild = Common_Json_SetAttrValue(pArray,iArrayCount, "Sender", Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "User", Common_Json_Type_String, alarmCfg->mailcfg.sender.pName, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "Password", Common_Json_Type_String, alarmCfg->mailcfg.sender.pPwd, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "SMTPServer", Common_Json_Type_String, alarmCfg->mailcfg.sender.pSmtpSvr, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "SMTPPort", Common_Json_Type_Number, 0, alarmCfg->mailcfg.sender.port, 0);
        Common_Json_SetAttrValue(pChild,-1, "EnableSSL", Common_Json_Type_Number, NULL, alarmCfg->mailcfg.sender.bEnSsl, 0);
        Common_Json_SetAttrValue(pChild,-1, "ServerVerify", Common_Json_Type_Number, NULL, alarmCfg->mailcfg.sender.SvrVerify, 0);
        pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount, "Receiver", Common_Json_Type_Array, NULL, 0, 0);
        for(i = 0; i < alarmCfg->mailcfg.recvList.count; i++)
        {
            pChild = Common_Json_SetAttrValue(pArray1, i, NULL, Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(pChild, -1, "Name", Common_Json_Type_String, alarmCfg->mailcfg.recvList.recver[i].pName, 0, 0);
            Common_Json_SetAttrValue(pChild, -1, "Addr", Common_Json_Type_String, alarmCfg->mailcfg.recvList.recver[i].pAddr, 0, 0);
        }
        Common_Json_SetAttrValue(pArray,iArrayCount, "Attachment", Common_Json_Type_Number, NULL, alarmCfg->mailcfg.bAttach, 0);
        Common_Json_SetAttrValue(pArray,iArrayCount, "MailInterval", Common_Json_Type_Number, NULL, alarmCfg->mailcfg.interval, 0);
    }
    *pOutData = pResult;
    return 0;
}

int put_alarm_email_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S8 *pString = NULL;
    S32 i = 0,iIndex = 0,iObjType = 0,iSize = 0,iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pArray1 = NULL,*pItem = NULL,*pChild = NULL;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);

    if(iSize > 0)
    {
        for(iIndex = 0; iIndex < iSize; iIndex++)
        {
            pArray = Common_Json_GetItem(pJData, iIndex, NULL);
            pItem = Common_Json_GetAttrValue(pArray, -1, "Sender/User", NULL, &pString, NULL, NULL);
            if(pItem &&0!=Common_StrCmp(alarmCfg->mailcfg.sender.pName,pString))
            {
                if(pString)
                {
                    Common_Strncpy(alarmCfg->mailcfg.sender.pName,pString,sizeof(alarmCfg->mailcfg.sender.pName) -1);
                    g_bCfgChange = 1;
                    pString = NULL;
                }
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "Sender/Password", NULL, &pString, NULL, NULL);
            if(pItem &&0!=Common_StrCmp(alarmCfg->mailcfg.sender.pPwd,pString))
            {
                if(pString)
                {
                    Common_Strncpy(alarmCfg->mailcfg.sender.pPwd,pString,sizeof(alarmCfg->mailcfg.sender.pPwd) -1);
                    g_bCfgChange = 1;
                    pString = NULL;
                }
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "Sender/SMTPServer", NULL, &pString, NULL, NULL);
            if(pItem &&0!=Common_StrCmp(alarmCfg->mailcfg.sender.pSmtpSvr,pString))
            {
                if(pString)
                {
                    Common_Strncpy(alarmCfg->mailcfg.sender.pSmtpSvr,pString,sizeof(alarmCfg->mailcfg.sender.pSmtpSvr) -1);
                    g_bCfgChange = 1;
                    pString = NULL;
                }
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "Sender/SMTPPort", NULL, NULL, &iValue, NULL);
            if(pItem&&(iValue != alarmCfg->mailcfg.sender.port))
            {
                alarmCfg->mailcfg.sender.port = iValue;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "Sender/EnableSSL", NULL, NULL, &iValue, NULL);
            if(pItem&&(iValue != alarmCfg->mailcfg.sender.bEnSsl))
            {
                alarmCfg->mailcfg.sender.bEnSsl = iValue;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "Sender/ServerVerify", NULL, NULL, &iValue, NULL);
            if(pItem&&(iValue != alarmCfg->mailcfg.sender.SvrVerify))
            {
                alarmCfg->mailcfg.sender.SvrVerify = iValue;
                g_bCfgChange = 1;
            }
            pArray1 = Common_Json_GetItem(pArray, -1, "Receiver");

            if(pArray1)
            {
                for(i = 0; i < alarmCfg->mailcfg.recvList.count; i++)
                {
                    pChild = Common_Json_GetItem(pArray1, i, NULL);
                    pItem = Common_Json_GetAttrValue(pChild, -1, "Name", NULL, &pString, NULL, NULL);
                    if(pItem &&0!=Common_StrCmp(alarmCfg->mailcfg.recvList.recver[i].pName,pString))
                    {
                        if(pString)
                        {
                            Common_Strncpy(alarmCfg->mailcfg.recvList.recver[i].pName,pString,sizeof(alarmCfg->mailcfg.recvList.recver[i].pName) -1);
                            g_bCfgChange = 1;
                            pString = NULL;
                        }
                    }
                    pItem = Common_Json_GetAttrValue(pChild, -1, "Addr", NULL, &pString, NULL, NULL);
                    if(pItem &&0!=Common_StrCmp(alarmCfg->mailcfg.recvList.recver[i].pAddr,pString))
                    {
                        if(pString)
                        {
                            Common_Strncpy(alarmCfg->mailcfg.recvList.recver[i].pAddr,pString,sizeof(alarmCfg->mailcfg.recvList.recver[i].pAddr) -1);
                            g_bCfgChange = 1;
                            pString = NULL;
                        }
                    }
                }
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "Attachment", NULL, NULL, &iValue, NULL);
            if(pItem&&(iValue != alarmCfg->mailcfg.bAttach))
            {
                alarmCfg->mailcfg.bAttach = iValue;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray, -1, "MailInterval", NULL, NULL, &iValue, NULL);
            if(pItem&&(iValue != alarmCfg->mailcfg.interval))
            {
                alarmCfg->mailcfg.interval = iValue;
                g_bCfgChange = 1;
            }
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;

}
#endif
//#ifndef SIMPLIFIED
/*int get_alarm_sound_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    unsigned int i = 0;
    S32 iArrayCount = 0, iRet;
    //S8 strcmd[64];
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pTemp = NULL;
    cJSON_Struct *pJCondition = (cJSON_Struct*)pCondition;
    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_SOUND_CFG *pSoundCfg = &pAlarmCfg->soundCfg;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < sizeof(pSoundCfg->soundName)/sizeof(pSoundCfg->soundName[0]); i++)
        {
            Common_Json_SetAttrValue(pArray,iArrayCount,"Id",Common_Json_Type_Number,NULL,i,0);
            Common_Json_SetAttrValue(pArray,iArrayCount,"Name",Common_Json_Type_String,pSoundCfg->soundName[i],0,0);
            Common_Json_SetAttrValue(pArray,iArrayCount,"NameEn",Common_Json_Type_String,pSoundCfg->soundNameEn[i],0,0);
            Common_Json_SetAttrValue(pArray,iArrayCount,"Times",Common_Json_Type_Number,NULL,pSoundCfg->playTimes[i],0);
            iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
            if(iRet != 0)
            {
                Common_Json_RemoveItem(pArray,iArrayCount,NULL);
            }
            else
            {
                iArrayCount++;
            }
        }

        Common_Json_SetAttrValue(pChild,-1,"Language",Common_Json_Type_Number,NULL,pSoundCfg->language,0);

        pTemp = Common_Json_SetAttrValue(pChild,-1,"Shedule",Common_Json_Type_Object,NULL,0,0);
        addSchduleTime(&pTemp,-1,&pSoundCfg->shedule);
    }
    *pOutData = pResult;
    return 0;
}

int put_alarm_sound_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S8 *pString = NULL;
    S8 strCmd[128];
    S32 iIndex = 0,iObjType = 0,iSize = 0,iValue = -1, id;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pTemp = NULL,*pItem = NULL;
    OVFS_ALARM_CFG *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_SOUND_CFG *pSoundCfg = &pAlarmCfg->soundCfg;
    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    if(0 == Common_Json_GetAttr(pArray,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        if(iObjType==Common_Json_Type_Array)
        {
            iSize= Common_Json_Size(pArray);
            if(iSize > 0)
            {
                for(iIndex = 0; iIndex < iSize; iIndex++)
                {
                    pItem = Common_Json_GetAttrValue(pArray, iIndex, "Id", NULL, NULL, &iValue, NULL);
                    if(pItem && iValue >= 0 && iValue < (S32)COMMON_ARRAY_ELEMENT_COUNT(pSoundCfg->soundName) )
                    {
                        id = iValue;
                        pItem = Common_Json_GetAttrValue(pArray, iIndex, "Name", NULL, &pString, NULL, NULL);
                        if(pItem &&0!=Common_StrCmp(pSoundCfg->soundName[id],pString))
                        {
                            if(pString)
                            {
                               if(id == 7)
                                	Common_Strncpy(pSoundCfg->soundName[id],pString,sizeof(pSoundCfg->soundName[id]) - 1);
                                g_bCfgChange = 1;
                                pString = NULL;
                            }
                        }
                        pItem = Common_Json_GetAttrValue(pArray, iIndex, "NameEn", NULL, &pString, NULL, NULL);
                        if(pItem &&0!=Common_StrCmp(pSoundCfg->soundNameEn[id],pString))
                        {
                            if(pString)
                            {
                                Common_Strncpy(pSoundCfg->soundNameEn[id],pString,sizeof(pSoundCfg->soundNameEn[id]) - 1);
                                g_bCfgChange = 1;
                                pString = NULL;
                            }
                        }
                        pItem = Common_Json_GetAttrValue(pArray, iIndex, "Path", NULL, &pString, NULL, NULL);
                        if(pItem)
                        {
                            if(pString)
                            {
                                if(Common_StrCmp(pString, (S8 *)"") == 0)
                                {
                                    //snprintf(strCmd, sizeof(strCmd),  "rm -fr /update/soundFile/sound_%d", id);
                                    snprintf(strCmd, sizeof(strCmd),  "rm -fr /usr/etc/cfgfiles/custom_audio");
                                    Common_System(strCmd);
                                }
                                else
                                {
                                    //snprintf(strCmd, sizeof(strCmd),  "mv \"%s\" /update/soundFile/sound_%d", pString, id);
                                    snprintf(strCmd, sizeof(strCmd),  "mv \"%s\" /usr/etc/cfgfiles/custom_audio",pString);
                                    Common_System(strCmd);
                                }
                                pString = NULL;
                            }
                        }

                        pItem = Common_Json_GetAttrValue(pArray, iIndex, "PathEn", NULL, &pString, NULL, NULL);
                        if(pItem)
                        {
                            if(pString)
                            {
                                if(Common_StrCmp(pString, (S8 *)"") == 0)
                                {
                                    //snprintf(strCmd, sizeof(strCmd),  "rm -fr /update/soundFile/soundEn_%d", id);
                                    snprintf(strCmd, sizeof(strCmd),  "rm -fr /usr/etc/cfgfiles/custom_audio");
                                    Common_System(strCmd);
                                }
                                else
                                {
                                    //snprintf(strCmd, sizeof(strCmd),  "mv \"%s\" /update/soundFile/soundEn_%d", pString, id);
                                    snprintf(strCmd, sizeof(strCmd),  "mv \"%s\" /usr/etc/cfgfiles/custom_audio",pString);
                                    Common_System(strCmd);
                                }
                                pString = NULL;
                            }
                        }

                        pItem = Common_Json_GetAttrValue(pArray, iIndex, "Times", NULL, NULL, &iValue, NULL);
                        if(pItem && iValue != pSoundCfg->playTimes[id])
                        {
                            pSoundCfg->playTimes[id] = iValue;
                            g_bCfgChange = 1;
                        }
                    }
                    else
                    {
                        LOGE("Input error\n");
                    }
                }
            }
        }
    }

    pTemp = Common_Json_GetAttrValue(pJData, -1, "Language", NULL, NULL, &iValue, NULL);
    if(NULL != pTemp && iValue != pSoundCfg->language)
    {
        pSoundCfg->language = iValue;
        g_bCfgChange = 1;
    }

    pTemp = Common_Json_GetItem(pJData, -1, "Shedule");
    if(NULL != pTemp && 1 == fillSchduleTime(pTemp,&pSoundCfg->shedule))
    {
        g_bCfgChange = 1;
    }

    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

//#ifndef SIMPLIFIED
int get_alarm_light_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pResult = NULL,*pChild = NULL,*pTemp = NULL;
    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_LIGHT_ALARM_CFG *pLightCfg = &pAlarmCfg->lightAlarmCfg;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pTemp = Common_Json_SetAttrValue(pChild,-1,"Shedule",Common_Json_Type_Object,NULL,0,0);
        addSchduleTime(&pTemp,-1,&pLightCfg->schTime);
    }
    *pOutData = pResult;
    return 0;
}

int put_alarm_light_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pTemp = NULL;
    OVFS_ALARM_CFG *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_LIGHT_ALARM_CFG *pLightCfg = &pAlarmCfg->lightAlarmCfg;

    pTemp = Common_Json_GetItem(pJData, -1, "Shedule");
    if(1 == fillSchduleTime(pTemp,&pLightCfg->schTime))
    {
        g_bCfgChange = 1;
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}
*/
#ifndef SIMPLIFIED

int get_ftp_snap_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pResult = NULL,*pChild = NULL,*pTemp = NULL;
    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_FTP_SNAP_CFG *pFtpCfg = &pAlarmCfg->ftpsnapCfg;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValueInt(pChild, "SnapInterval", pFtpCfg->snapInterval);
        Common_Json_SetAttrValueInt(pChild, "SnapStreamIndex", pFtpCfg->snapStreamIndex);
        pTemp = Common_Json_SetAttrValue(pChild,-1,"Shedule",Common_Json_Type_Object,NULL,0,0);
        //addSchduleTime(&pTemp,-1,&pFtpCfg->schTime);
        SchduleTimeStructToJson(pTemp, &pFtpCfg->schTime, 1);
    }
    *pOutData = pResult;
    return 0;
}

int put_ftp_snap_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pTemp = NULL;
    OVFS_ALARM_CFG *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_FTP_SNAP_CFG *pFtpCfg = &pAlarmCfg->ftpsnapCfg;

    pTemp = Common_Json_GetItem(pJData, -1, "Shedule");
    if(1 == fillSchduleTime(pTemp,&pFtpCfg->schTime))
    {
        g_bCfgChange = 1;
    }
    if(Common_Json_GetAttrValueInt(pJData, "SnapInterval", &pFtpCfg->snapInterval))
    {
        g_bCfgChange = 1;
    }
    if(Common_Json_GetAttrValueInt(pJData, "SnapStreamIndex", &pFtpCfg->snapStreamIndex))
    {
        g_bCfgChange = 1;
    }

    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

#endif
/*
int get_redblue_light_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pResult = NULL,*pChild = NULL,*pTemp = NULL;
    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_LIGHT_ALARM_CFG *pRedblueCfg = &pAlarmCfg->redbluelightCfg;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pTemp = Common_Json_SetAttrValue(pChild,-1,"Shedule",Common_Json_Type_Object,NULL,0,0);
        addSchduleTime(&pTemp,-1,&pRedblueCfg->schTime);
    }
    *pOutData = pResult;
    return 0;
}

int put_redblue_light_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pTemp = NULL;
    OVFS_ALARM_CFG *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_LIGHT_ALARM_CFG *pRedblueCfg = &pAlarmCfg->redbluelightCfg;

    pTemp = Common_Json_GetItem(pJData, -1, "Shedule");
    if(1 == fillSchduleTime(pTemp,&pRedblueCfg->schTime))
    {
        g_bCfgChange = 1;
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

//#endif

int get_alarm_trigCfg_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(g_alarmName); i++)
        {
            if(i <= ALARM_TYPE_END || (i >= ACTION2_TYPE_START &&  i <= ACTION2_TYPE_END) )
            {
                S8 szUri[64] = {0};
                Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,g_alarmName[i],0,0);
                snprintf(szUri,sizeof(szUri),"/Alarm/TriggerCfg/%s",g_alarmName[i]);
                Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,szUri,0,0);
                iArrayCount++;
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int get_trigCfg_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 iRet = -1,i = 0,iArrayCount = 0;
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    if(!pAbility)
        return -1;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(; k < chanNum; k++)
                {
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,i,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,k,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,alarmCfg->motioncfg.cfgTri[i][k].enable,0);
                    iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
                    if(iRet != 0)
                    {
                        Common_Json_RemoveItem(pArray,iArrayCount,NULL);
                        continue;
                    }
                    addSchduleTime(&pArray,iArrayCount,&alarmCfg->motioncfg.cfgTri[i][k].shedule);
                    iArrayCount++;
                }
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int put_trigCfg_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_MOTION_TRI_CFG *pTriCfg = NULL;

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pTriCfg = &alarmCfg->motioncfg.cfgTri[iDev][iChan];
        Common_Json_GetAttrValue(pJData, iIndex, "Enable",NULL, NULL, &pTriCfg->enable, NULL);
        pChild = Common_Json_GetItem(pJData, iIndex, NULL);
        if(1 == fillSchduleTime(pChild,&pTriCfg->shedule))
        {
            g_bCfgChange = 1;
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}
*/
#if 0//ndef SIMPLIFIED
int get_trig_alarmIn_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 iRet = -1,i = 0,iArrayCount = 0;
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                chanNum = pAbility->alarmInNum;
                for(k = 0; k < chanNum; k++)
                {
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,i,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,k,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,alarmCfg->alarmIncfg.cfgTri[i][k].enable,0);
                    iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
                    if(iRet != 0)
                    {
                        Common_Json_RemoveItem(pArray,iArrayCount,NULL);
                        continue;
                    }
                    addSchduleTime(&pArray,iArrayCount,&alarmCfg->alarmIncfg.cfgTri[i][k].shedule);
                    iArrayCount++;
                }
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int put_trig_alarmIn_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0,iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ALARMIN_TRI_CFG *pTriCfg = NULL;

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);

    //ovfs_print_json(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pTriCfg = &alarmCfg->alarmIncfg.cfgTri[iDev][iChan];
        Common_Json_GetAttrValue(pJData, iIndex, "Enable",NULL, NULL, &iValue, NULL);

        if(iValue != -1 && (iValue != pTriCfg->enable))
        {
            pTriCfg->enable = iValue;
            g_bCfgChange = 1;
        }

        pChild = Common_Json_GetItem(pJData, iIndex, NULL);
        if(1 == fillSchduleTime(pChild,&pTriCfg->shedule))
        {
            g_bCfgChange = 1;
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;

}
#endif
/*
int get_trig_motion_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
//LOGW("enter get_trig_motion_res\n");
    int bDetail = 1;
    S32 iRet = -1,i = 0,iArrayCount = 0;
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    if(!pAbility)
        return -1;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
//LOGW("devNum=%d\n",devNum);
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(; k < chanNum; k++)
                {
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,i,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,k,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,alarmCfg->motioncfg.cfgTri[i][k].enable,0);
                    iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
//LOGW("iRet=%d\n",iRet);
                    if(iRet != 0)
                    {
                        Common_Json_RemoveItem(pArray,iArrayCount,NULL);
                        continue;
                    }
//LOGW("addSchduleTime 1\n");
                    addSchduleTime(&pArray,iArrayCount,&alarmCfg->motioncfg.cfgTri[i][k].shedule);
//LOGW("addSchduleTime 2\n");
                    iArrayCount++;
                }
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int put_trig_motion_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0,iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_MOTION_TRI_CFG *pTriCfg = NULL;

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pTriCfg = &alarmCfg->motioncfg.cfgTri[iDev][iChan];
        Common_Json_GetAttrValue(pJData, iIndex, "Enable",NULL, NULL, &iValue, NULL);

        if(iValue != -1 && (iValue != pTriCfg->enable))
        {
            pTriCfg->enable = iValue;
            g_bCfgChange = 1;
        }

        pChild = Common_Json_GetItem(pJData, iIndex, NULL);
        if(1 == fillSchduleTime(pChild,&pTriCfg->shedule))
        {
            g_bCfgChange = 1;
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

int get_trig_vhide_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iRet = -1,iArrayCount = 0;
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(; k < chanNum; k++)
                {
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,i,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,k,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,alarmCfg->vhidecfg.cfgTri[i][k].enable,0);
                    iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
                    if(iRet != 0)
                    {
                        Common_Json_RemoveItem(pArray,iArrayCount,NULL);
                        continue;
                    }
                    addSchduleTime(&pArray,iArrayCount,&alarmCfg->vhidecfg.cfgTri[i][k].shedule);
                    iArrayCount++;
                }
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int put_trig_vhide_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0,iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_VHIDE_TRI_CFG *pTriCfg = NULL;

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pTriCfg = &alarmCfg->vhidecfg.cfgTri[iDev][iChan];
        Common_Json_GetAttrValue(pJData, iIndex, "Enable",NULL, NULL, &iValue, NULL);

        if(iValue != -1 && (iValue != pTriCfg->enable))
        {
            pTriCfg->enable = iValue;
            g_bCfgChange = 1;
        }

        pChild = Common_Json_GetItem(pJData, iIndex, NULL);
        if(1 == fillSchduleTime(pChild,&pTriCfg->shedule))
        {
            g_bCfgChange = 1;
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

int get_trig_detPerson_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 iRet = -1,i = 0,iArrayCount = 0;
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(; k < chanNum; k++)
                {
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,i,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,k,0);
                    Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,alarmCfg->PersonDetcfg.cfgTri[i][k].enable,0);
                    iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
                    if(iRet != 0)
                    {
                        Common_Json_RemoveItem(pArray,iArrayCount,NULL);
                        continue;
                    }
                    addSchduleTime(&pArray,iArrayCount,&alarmCfg->PersonDetcfg.cfgTri[i][k].shedule);
                    iArrayCount++;
                }
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int put_trig_detPerson_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0,iValue = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_COMMON_TRI_CFG *pTriCfg = NULL;

    pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }
    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pTriCfg = &alarmCfg->PersonDetcfg.cfgTri[iDev][iChan];
        Common_Json_GetAttrValue(pJData, iIndex, "Enable",NULL, NULL, &iValue, NULL);

        if(iValue != -1 && (iValue != pTriCfg->enable))
        {
            pTriCfg->enable = iValue;
            g_bCfgChange = 1;
        }

        pChild = Common_Json_GetItem(pJData, iIndex, NULL);
        if(1 == fillSchduleTime(pChild,&pTriCfg->shedule))
        {
            g_bCfgChange = 1;
        }
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}
*/

//如果正在报警输出，则关闭报警输出
void closeAlarmOut()
{
    S8 str_state[64] = {0},str_cmd[64] = {0};
    S32 i = 0,iRet_state = 0,iRet_cmd = 0,iAoutNum = 0,alarmOutState = -1;
    cJSON_Struct *pResult_state = NULL,*pRoot_state = NULL;
    cJSON_Struct *pResult_cmd = NULL,*pRoot_cmd = NULL;
    OVFS_ABILITY * pAbility = ovfs_get_ability();
    iAoutNum= pAbility->alarmOutNum;

    for(i = 0; i < iAoutNum; i++)
    {
        memset(str_state,0,sizeof(str_state));
        memset(str_cmd,0,sizeof(str_cmd));

        pRoot_state = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        pRoot_cmd = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

        if (pRoot_state && pRoot_cmd)
        {
            sprintf(str_state,"/BoardSys/AlarmOut/State/Channel%d",i);

            Common_Json_SetAttrValue(pRoot_state,-1,"Header",Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pRoot_state,-1,"Header/Uri",Common_Json_Type_String,str_state,0,0);
            Common_Json_SetAttrValue(pRoot_state,-1,"Header/Method",Common_Json_Type_String,"get",0,0);

            iRet_state = Module_CallFunctions(g_hModuleHandle, pRoot_state, &pResult_state, 3000);

            if(iRet_state == 0)
            {
                alarmOutState = -1;

                Common_Json_GetAttrValueInt(pResult_state,"Data.IsEnable",&alarmOutState);

                //如果当前报警输出为正在报警，则关闭该报警
                if(alarmOutState == 1)
                {
                    sprintf(str_cmd,"/BoardSys/AlarmOut/Function/Channel%d/Disable?Delay=-1",i);

                    Common_Json_SetAttrValue(pRoot_cmd,-1,"Header",Common_Json_Type_Object,NULL,0,0);
                    Common_Json_SetAttrValue(pRoot_cmd,-1,"Header/Uri",Common_Json_Type_String,str_cmd,0,0);
                    Common_Json_SetAttrValue(pRoot_cmd,-1,"Header/Method",Common_Json_Type_String,"put",0,0);

                    iRet_cmd = Module_CallFunctions(g_hModuleHandle, pRoot_cmd, &pResult_cmd, 3000);

                    if(iRet_cmd < 0)
                    {
                        LOGE("%d,err:%d\n",iRet_cmd,i);
                    }
                    else
                    {
                        LOGW("Aout %d Disable!\n",i);
                    }
                }
            }
            else
            {
                LOGE("%d,err:%d\n",iRet_state,i);
            }
        }

        Common_Json_Delete(pRoot_state);
        Common_Json_Delete(pResult_state);
        pRoot_state = NULL;
        pResult_state = NULL;

        Common_Json_Delete(pRoot_cmd);
        Common_Json_Delete(pResult_cmd);
        pRoot_cmd = NULL;
        pResult_cmd = NULL;
    }
}

int get_alarm_linkCfg_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    S8 szUri[64] = {0};
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(g_alarmName); i++)
        {
            Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,g_alarmName[i],0,0);
            snprintf(szUri,sizeof(szUri),"/Alarm/LinkageCfg/%s",g_alarmName[i]);
            Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,szUri,0,0);
            iArrayCount++;
        }
    }
    *pOutData = pResult;
    return 0;
}
/*
int get_link_common_res(cJSON_Struct **pJData,int nWhich,int iDev,int iChan,OVFS_LINKAGE_CFG *pLinkageCfg,void *pCondition)
{
    S8 strcmd[64] = {0};
    S32 iAoutNum = 0;
    S32 iDevNum = 0;
    S32 iRet = -1,i = 0,j = 0;
    S32 iArrayCount = nWhich;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition,*pArray = NULL,*pArray1 = NULL,*pChild = NULL,*pItem = NULL;
//#ifndef SIMPLIFIED
//    S8 linkName[][64] = {"LinkFtp","LinkMail","LinkRecord","LinkHttp","LinkSnap","LinkAlarmOut","LinkPtz","LinkSound","LinkLightAlarm"};
//#else
//    S8 linkName[][64] = {"LinkSound","LinkLightAlarm"};
//#endif
    if(!pAbility)
    {
        LOGE("pAbility == null\n");
        return -1;
    }
    iAoutNum = pAbility->alarmOutNum;
    iDevNum = pAbility->devNum;
    pArray = *pJData;
    if (pArray)
    {
#ifndef SIMPLIFIED
        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkFtp",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkFtp.enable,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkReportCentre",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkReportCenter.enable,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkMail",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkMail.enable,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Default",Common_Json_Type_Number,NULL,pLinkageCfg->linkMail.bDefault,0);
        pChild = Common_Json_SetAttrValue(pArray,iArrayCount, "Sender", Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "User", Common_Json_Type_String, pLinkageCfg->linkMail.sender.pName, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "Password", Common_Json_Type_String, pLinkageCfg->linkMail.sender.pPwd, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "SMTPServer", Common_Json_Type_String, pLinkageCfg->linkMail.sender.pSmtpSvr, 0, 0);
        Common_Json_SetAttrValue(pChild,-1, "SMTPPort", Common_Json_Type_Number, 0, pLinkageCfg->linkMail.sender.port, 0);
        Common_Json_SetAttrValue(pChild,-1, "EnableSSL", Common_Json_Type_Number, NULL, pLinkageCfg->linkMail.sender.bEnSsl, 0);
        Common_Json_SetAttrValue(pChild,-1, "ServerVerify", Common_Json_Type_Number, NULL, pLinkageCfg->linkMail.sender.SvrVerify, 0);
        pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount, "Receiver", Common_Json_Type_Array, NULL, 0, 0);
        for(i = 0; i < pLinkageCfg->linkMail.recvList.count; i++)
        {
            pChild = Common_Json_SetAttrValue(pArray1, i, NULL, Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(pChild, -1, "Name", Common_Json_Type_String, pLinkageCfg->linkMail.recvList.recver[i].pName, 0, 0);
            Common_Json_SetAttrValue(pChild, -1, "Addr", Common_Json_Type_String, pLinkageCfg->linkMail.recvList.recver[i].pAddr, 0, 0);
        }
        Common_Json_SetAttrValue(pArray,iArrayCount, "Attachment", Common_Json_Type_Number, NULL, pLinkageCfg->linkMail.bAttach, 0);
        Common_Json_SetAttrValue(pArray,iArrayCount, "MailInterval", Common_Json_Type_Number, NULL, pLinkageCfg->linkMail.interval, 0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkRecord",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkRecord.enable,0);
        for(i= 0; i <iDevNum; i++)
        {
            snprintf(strcmd, sizeof(strcmd), "Mask%d",i);
            Common_Json_SetAttrValue(pArray,iArrayCount,strcmd,Common_Json_Type_Number,NULL,pLinkageCfg->linkRecord.mask[i],0);
        }
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkHttp",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkHttp.enable,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkSnap",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkSnap.enable,0);
        for(i = 0; i < pAbility->devNum; i++)
        {
            OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
            if(!pDev)
                continue;
            memset(strcmd,0,sizeof(strcmd));
            snprintf(strcmd, sizeof(strcmd), "Device%d",i);
            pChild = Common_Json_SetAttrValue(pArray,iArrayCount,strcmd,Common_Json_Type_Object,NULL,0,0);
            if(!pChild)
                continue;
            for(j = 0; j < pDev->chanNum; j++)
            {
                snprintf(strcmd, sizeof(strcmd), "Channel%d",j);
                pItem = Common_Json_SetAttrValue(pChild,-1,strcmd,Common_Json_Type_Object,NULL,0,0);
                Common_Json_SetAttrValue(pItem,-1,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkSnap.snapInfo[i][j].enable,0);
                Common_Json_SetAttrValue(pItem,-1,"Count",Common_Json_Type_Number,NULL,pLinkageCfg->linkSnap.snapInfo[i][j].count,0);
                Common_Json_SetAttrValue(pItem,-1,"Interval",Common_Json_Type_Number,NULL,pLinkageCfg->linkSnap.snapInfo[i][j].interval,0);
            }
        }
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkAlarmOut",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkAout.enable,0);
        for(j = 0; j <(iAoutNum/8+1); j++)
        {
            sprintf(strcmd,"Mask%d",j);
            Common_Json_SetAttrValue(pArray,iArrayCount,strcmd,Common_Json_Type_Number,NULL,pLinkageCfg->linkAout.mask[j],0);
        }
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkPtz",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Type",Common_Json_Type_Number,NULL,pLinkageCfg->linkPtz.type,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Index",Common_Json_Type_Number,NULL,pLinkageCfg->linkPtz.index,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }
#endif

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkSound",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkSound.enable,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Index",Common_Json_Type_Number,NULL,pLinkageCfg->linkSound.index,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkLightAlarm",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkLightAlarm.enable,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

        Common_Json_SetAttrValue(pArray,iArrayCount,"ActionName",Common_Json_Type_String,"LinkRedblueLight",0,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,iDev,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,iChan,0);
        Common_Json_SetAttrValue(pArray,iArrayCount,"Enable",Common_Json_Type_Number,NULL,pLinkageCfg->linkRedblueLight.enable,0);
        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
        if(iRet != 0)
        {
            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
        }
        else
        {
            iArrayCount++;
        }

    }
    return iArrayCount;
}

int put_link_common_res(cJSON_Struct *pJData,int nWhich,OVFS_LINKAGE_CFG *pLinkageCfg,S32 iAlarmType,S32 iAlarmSrc)
{
    S32 i = 0,j = 0;
    int value = -1;
    S32 bChange = 0;
    S8 strcmd[64] = {0};
    S32 iAoutNum = 0,iDevNum = 0;
    S8 *pActName = NULL,*pString = NULL;
    cJSON_Struct *pArray = NULL,*pArray1 = NULL,*pItem = NULL,*pChild = NULL;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    //S8 linkName[][64] = {"LinkBuzzer","LinkReportCentre","LinkMail","LinkRecord","LinkShowDialogue","LinkSnap","LinkAlarmOut","LinkPtz"};

    if(!pAbility)
    {
        LOGE("pAbility == null\n");
        return bChange;
    }
    iAoutNum = pAbility->alarmOutNum;
    iDevNum = pAbility->devNum;
    pArray = pJData;

    if (pArray)
    {
        if(NULL == Common_Json_GetAttrValue(pArray, nWhich, "ActionName", NULL, &pActName, NULL, NULL))
            return bChange;
        if(0 == Common_StriCmp(pActName,(S8*)"LinkFtp"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkFtp.enable))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_FTP,value,0);
                pLinkageCfg->linkFtp.enable = value;
                g_bCfgChange = 1;
            }
            //LOGI("%s:%d\n",pActName,pLinkageCfg->linkFtp.enable);
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkReportCentre"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkReportCenter.enable))
            {
                pLinkageCfg->linkReportCenter.enable = value;
                g_bCfgChange = 1;
            }
            //LOGI("%s:%d\n",pActName,pLinkageCfg->linkReportCenter.enable);
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkMail"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkMail.enable))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_MAIL,value,0);
                pLinkageCfg->linkMail.enable = value;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Default", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkMail.bDefault))
            {
                pLinkageCfg->linkMail.bDefault = value;
                g_bCfgChange = 1;
            }
            pArray1 = Common_Json_GetItem(pArray, nWhich, "Sender");
            pItem = Common_Json_GetAttrValue(pArray1, -1, "User", NULL, &pString, NULL, NULL);
            if(pItem&&0!=Common_StrCmp(pLinkageCfg->linkMail.sender.pName,pString))
            {
                Common_Strncpy(pLinkageCfg->linkMail.sender.pName,pString,sizeof(pLinkageCfg->linkMail.sender.pName) -1);
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray1, -1, "Password", NULL, &pString, NULL, NULL);
            if(pItem&&0!=Common_StrCmp(pLinkageCfg->linkMail.sender.pPwd,pString))
            {
                Common_Strncpy(pLinkageCfg->linkMail.sender.pPwd,pString,sizeof(pLinkageCfg->linkMail.sender.pPwd)-1);
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray1, -1, "SMTPServer", NULL, &pString, NULL, NULL);
            if(pItem&&0!=Common_StrCmp(pLinkageCfg->linkMail.sender.pSmtpSvr,pString))
            {
                Common_Strncpy(pLinkageCfg->linkMail.sender.pSmtpSvr,pString,sizeof(pLinkageCfg->linkMail.sender.pSmtpSvr) -1);
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray1, -1, "SMTPPort", NULL, NULL, &value, NULL);
            if(pItem&&(value != pLinkageCfg->linkMail.sender.port))
            {
                pLinkageCfg->linkMail.sender.port = value;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray1, -1, "EnableSSL", NULL, NULL, &value, NULL);
            if(pItem&&(value != pLinkageCfg->linkMail.sender.bEnSsl))
            {
                pLinkageCfg->linkMail.sender.bEnSsl = value;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray1, -1, "ServerVerify", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkMail.sender.SvrVerify))
            {
                pLinkageCfg->linkMail.sender.SvrVerify = value;
                g_bCfgChange = 1;
            }
            pArray1 = Common_Json_GetItem(pArray, nWhich, "Receiver");
            for(i = 0; i < pLinkageCfg->linkMail.recvList.count; i++)
            {
                pChild = Common_Json_GetItem(pArray1, i, NULL);
                pItem = Common_Json_GetAttrValue(pChild, -1, "Name", NULL, &pString, NULL, NULL);
                if(pItem && 0!=Common_StrCmp(pLinkageCfg->linkMail.recvList.recver[i].pName,pString))
                {
                    Common_Strncpy(pLinkageCfg->linkMail.recvList.recver[i].pName,pString,sizeof(pLinkageCfg->linkMail.recvList.recver[i].pName) -1);
                    g_bCfgChange = 1;
                }
                pItem = Common_Json_GetAttrValue(pChild, -1, "Addr", NULL, &pString, NULL, NULL);
                if(pItem && 0!=Common_StrCmp(pLinkageCfg->linkMail.recvList.recver[i].pAddr,pString))
                {
                    Common_Strncpy(pLinkageCfg->linkMail.recvList.recver[i].pAddr,pString,sizeof(pLinkageCfg->linkMail.recvList.recver[i].pAddr) -1);
                    g_bCfgChange = 1;
                }

            }
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Attachment", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkMail.bAttach))
            {
                pLinkageCfg->linkMail.bAttach = value;
                g_bCfgChange = 1;
            }
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "MailInterval", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkMail.interval))
            {
                pLinkageCfg->linkMail.interval = value;
                g_bCfgChange = 1;
            }
            //LOGI("%s:%d\n",pActName,pLinkageCfg->linkMail.enable);
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkRecord"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkRecord.enable))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_RECORD,value,0);
                pLinkageCfg->linkRecord.enable = value;
                g_bCfgChange = 1;
            }
            //LOGI("%s:%d\n",pActName,pLinkageCfg->linkRecord.enable);
            for(j = 0; j <iDevNum; j++)
            {
                value = -1;
                snprintf(strcmd, sizeof(strcmd), "Mask%d",j);
                pItem = Common_Json_GetAttrValue(pArray, nWhich, strcmd, NULL, NULL, &value, NULL);
                if(pItem &&(value != (int)pLinkageCfg->linkRecord.mask[j]))
                {
                    pLinkageCfg->linkRecord.mask[j] = value;
                    g_bCfgChange = 1;
                }
            }
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkHttp"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkHttp.enable))
            {
                pLinkageCfg->linkHttp.enable = value;
                g_bCfgChange = 1;
            }
            //LOGI("%s:%d\n",pActName,pLinkageCfg->linkHttp.enable);
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkSnap"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkSnap.enable))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_SNAP,value,0);
                pLinkageCfg->linkSnap.enable = value;
                g_bCfgChange = 1;
            }
            pChild = Common_Json_GetItem(pArray, nWhich, NULL);
            for(i = 0; i < pAbility->devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                for(j = 0; j < pDev->chanNum; j++)
                {
                    value = -1;
                    snprintf(strcmd, sizeof(strcmd), "Device%d/Channel%d/Enable",i,j);
                    pItem = Common_Json_GetAttrValue(pChild, -1, strcmd, NULL, NULL, &value, NULL);
                    if(pItem &&(value != pLinkageCfg->linkSnap.snapInfo[i][j].enable))
                    {
                        pLinkageCfg->linkSnap.snapInfo[i][j].enable = value;
                        g_bCfgChange = 1;
                    }
                    value = -1;
                    snprintf(strcmd, sizeof(strcmd), "Device%d/Channel%d/Count",i,j);
                    pItem = Common_Json_GetAttrValue(pChild, -1, strcmd, NULL, NULL, &value, NULL);
                    if(pItem &&(value != pLinkageCfg->linkSnap.snapInfo[i][j].count))
                    {
                        pLinkageCfg->linkSnap.snapInfo[i][j].count = value;
                        g_bCfgChange = 1;
                    }
                    value = -1;
                    snprintf(strcmd, sizeof(strcmd), "Device%d/Channel%d/Interval",i,j);
                    pItem = Common_Json_GetAttrValue(pChild, -1, strcmd, NULL, NULL, &value, NULL);
                    if(pItem &&(value != pLinkageCfg->linkSnap.snapInfo[i][j].interval))
                    {
                        pLinkageCfg->linkSnap.snapInfo[i][j].interval = value;
                        g_bCfgChange = 1;
                    }
                }
            }
            //LOGI("%s:%d\n",pActName,pLinkageCfg->linkSnap.enable);
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkAlarmOut"))
        {
            int m;
            for(j = 0; j <(iAoutNum/8+1); j++)
            {
                value = -1;
                snprintf(strcmd, sizeof(strcmd), "Mask%d",j);
                pItem = Common_Json_GetAttrValue(pArray, nWhich, strcmd, NULL, NULL, &value, NULL);
                if(pItem &&(value != (int)pLinkageCfg->linkAout.mask[j]))
                {
                    int enableNew = 0;
                    int maskOld = pLinkageCfg->linkAout.mask[j];
                    if(value & ((1 << iAoutNum) - 1))
                        enableNew = 1;
                    else
                        enableNew = 0;
                    if(pLinkageCfg->linkAout.enable != enableNew)
                    {
                        pLinkageCfg->linkAout.enable = enableNew;
                    }
                    pLinkageCfg->linkAout.mask[j] = value;
                    for(m = 0; m < iAoutNum; m++)
                    {
                        if((maskOld & (1 << m)) > (value & (1 << m)))
                        {
                            ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_AOUT,0,m);
                        }
                        if((maskOld & (1 << m)) < (value & (1 << m)))
                        {
                            ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_AOUT,1,m);
                        }
                    }
                    g_bCfgChange = 1;
                }
            }
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkPtz"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Type", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkPtz.type))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_PTZ,value,0);
                pLinkageCfg->linkPtz.type = value;
                g_bCfgChange = 1;
            }
            value = -1;
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Index", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkPtz.index))
            {
                pLinkageCfg->linkPtz.index = value;
                g_bCfgChange = 1;
            }
            //LOGI("%s:%d,%d\n",pActName,pLinkageCfg->linkPtz.type,pLinkageCfg->linkPtz.index);
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkSound"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkSound.enable))
            {
                pLinkageCfg->linkSound.enable= value;
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_SOUND,value,0);
                g_bCfgChange = 1;
            }
            value = -1;
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Index", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkSound.index))
            {
                pLinkageCfg->linkSound.index = value;
                g_bCfgChange = 1;
            }
        }
        else if(0 == Common_StriCmp(pActName,(S8*)"LinkLightAlarm"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkLightAlarm.enable))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_LIGHTALARM,value,0);
                pLinkageCfg->linkLightAlarm.enable= value;
                g_bCfgChange = 1;
            }
        }
		else if(0 == Common_StriCmp(pActName,(S8*)"LinkRedblueLight"))
        {
            pItem = Common_Json_GetAttrValue(pArray, nWhich, "Enable", NULL, NULL, &value, NULL);
            if(pItem &&(value != pLinkageCfg->linkRedblueLight.enable))
            {
                ovfs_clearAlarmLink(g_hModuleHandle,iAlarmType,iAlarmSrc,pLinkageCfg,LINK_TYPE_REDBLUELIGHT,value,0);
                pLinkageCfg->linkRedblueLight.enable= value;
                g_bCfgChange = 1;
            }
        }

    }
    return 0;
}
*/
#ifndef SIMPLIFIED
int get_link_alarmIn_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                chanNum = pAbility->alarmInNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->alarmIncfg.linkage[i][k];
#if 0
                    get_link_common_res(&pArray,i,k,&linkCfg,pCondition);
#else
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
#endif
                }
            }
        }*/
    }
    //printf("get_link_alarmIn_res:\n%s\n", Common_Json_Print(pResult, NULL));
    *pOutData = pResult;
    return 0;
}
int put_link_alarmIn_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_ALARMIN;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg = NULL;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->alarmIncfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getChanIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
#endif
int get_link_common_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    cJSON_Struct *pResult = NULL, *pChild = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);

        if(Common_StriCmp((S8 *)pInData, "/Alarm/AlarmIn") == 0)
        {
            ovfs_make_common_json(&alarmCfg->alarmIncfg,(char *)ALARM_STR_AI,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/Motion") == 0)
        {
            ovfs_make_common_json(&alarmCfg->motioncfg,(char *)ALARM_STR_MOTION,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/Vhide") == 0)
        {
            ovfs_make_common_json(&alarmCfg->vhidecfg,(char *)ALARM_STR_VHIDE,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/RegionalInvasion") == 0)
        {
            ovfs_make_common_json(&alarmCfg->regionalInvasioncfg,(char *)ALARM_STR_REGIONALINVASION,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/DetectWire") == 0)
        {
            ovfs_make_common_json(&alarmCfg->traverseCfg,(char *)ALARM_STR_DETWIRE,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/PersonStaying") == 0)
        {
            ovfs_make_common_json(&alarmCfg->personStayCfg,(char *)ALARM_STR_PERSONSTAYING,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/DetectAbsent") == 0)
        {
            ovfs_make_common_json(&alarmCfg->absentCfg,(char *)ALARM_STR_DETABSENT,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/ParkingViolation") == 0)
        {
            ovfs_make_common_json(&alarmCfg->parkViolationCfg,(char *)ALARM_STR_PARKINGVIOLATION,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/Retrograde") == 0)
        {
            ovfs_make_common_json(&alarmCfg->vehicleRetrogradeCfg,(char *)ALARM_STR_RETROGRADE,&pChild , 0);
        }
        else if(Common_StriCmp((S8 *)pInData, "/Alarm/SensorAlarm") == 0)
        {
            int id = 0;
            cJSON_Struct *indata = (cJSON_Struct *)pAddData;
            if(Common_Json_GetAttrValueInt(indata, "SensorId", &id))
            {
                if(id>=0 && id<MAX_SENSORALARM)
                {
                    ovfs_make_common_json(&alarmCfg->sensorAlarm[id],(char *)ALARM_STR_SENSORALARM,&pChild , 0);
                }
            }
            else
            {
                cJSON_Struct *list = Common_Json_SetAttrValueArr(pChild, "List");
                for(int i=0; i<MAX_SENSORALARM; i++)
                {
                    cJSON_Struct *item = Common_Json_SetAttrValueArrObj(list, i);
                    ovfs_make_common_json(&alarmCfg->sensorAlarm[i],(char *)ALARM_STR_SENSORALARM,&item, 0);
                }
            }
        }
        else
        {
            Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,-1,0);
        }

    }
    *pOutData = pResult;
    return 0;
}
int put_link_common_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_MOTION;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    if(Common_StriCmp((S8 *)pInData, "/Alarm/AlarmIn") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->alarmIncfg, (char *)ALARM_STR_AI, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/Motion") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->motioncfg, (char *)ALARM_STR_MOTION, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/Vhide") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->vhidecfg, (char *)ALARM_STR_VHIDE, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/RegionalInvasion") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->regionalInvasioncfg, (char *)ALARM_STR_REGIONALINVASION, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/DetectWire") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->traverseCfg, (char *)ALARM_STR_DETWIRE, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/PersonStaying") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->personStayCfg, (char *)ALARM_STR_PERSONSTAYING, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/DetectAbsent") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->absentCfg, (char *)ALARM_STR_DETABSENT, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/ParkingViolation") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->parkViolationCfg, (char *)ALARM_STR_PARKINGVIOLATION, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/Retrograde") == 0)
    {
        ovfs_make_common_struct(pJData, &alarmCfg->vehicleRetrogradeCfg, (char *)ALARM_STR_RETROGRADE, 0);
    }
    else if(Common_StriCmp((S8 *)pInData, "/Alarm/SensorAlarm") == 0)
    {
        //ovfs_make_common_struct(pJData, &alarmCfg->sensorAlarm, (char *)ALARM_STR_SENSORALARM, 0);
        int id = 0;
        ovfs_print_json(pInData);
        if(Common_Json_GetAttrValueInt(pJData, "SensorId", &id))
        {
            if(id>=0 && id<MAX_SENSORALARM)
            {
                ovfs_make_common_struct(pJData, &alarmCfg->sensorAlarm[id], (char *)ALARM_STR_SENSORALARM, 0);
            }
        }
        else
        {
            cJSON_Struct *list = Common_Json_GetAttrValueArr(pJData, "List");
            for(int i=0; i<MAX_SENSORALARM; i++)
            {
                cJSON_Struct *item = Common_Json_GetAttrValueArrItem(list, i);
                ovfs_make_common_struct(item, &alarmCfg->sensorAlarm[i], (char *)ALARM_STR_SENSORALARM, 0);
            }
        }
    }
    else
    {
        ovfs_make_result(-1,"Error",NULL,pOutData);
        return 0;
    }

    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);

    return 0;
}

int get_link_motion_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    cJSON_Struct *pResult = NULL, *pChild = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);

        ovfs_make_common_json(&alarmCfg->motioncfg,(char *)ALARM_STR_MOTION,&pChild , 0);
    }
    *pOutData = pResult;
    return 0;
}
int put_link_motion_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_MOTION;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    ovfs_make_common_struct(pJData, &alarmCfg->motioncfg, (char *)ALARM_STR_MOTION, 0);
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);

    return 0;
}
int get_link_vhide_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        ovfs_make_common_json(&alarmCfg->vhidecfg,(char *)ALARM_STR_VHIDE,&pChild , 0);

    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_vhide_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_VHIDE;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    ovfs_make_common_struct(pJData, &alarmCfg->vhidecfg, (char *)ALARM_STR_VHIDE, 0);
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);

    return 0;
}

int get_link_detPerson_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    /*if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->PersonDetcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }
    }*/
    ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_detPerson_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_DETECT_PERSON;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->PersonDetcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}

#ifndef SIMPLIFIED
int get_link_diskFull_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->diskFullcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_diskFull_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_DISKFULL;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->diskFullcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
int get_link_diskErr_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->diskErrcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    *pOutData = pResult;
    return 0;
}
int put_link_diskErr_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_DISKERR;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->diskErrcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
#endif
int get_link_netBreak_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->netBreakcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_netBreak_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_CABLE_BREAK;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->netBreakcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
int get_link_ipConflit_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->ipConflitcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_ipConflit_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_IP_CONFLIT;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->ipConflitcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
int get_link_illegalAcc_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->illAccesscfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_illegalAcc_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_ILLEG_ACCESS;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->illAccesscfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}

#if 0//ndef SIMPLIFIED
int get_link_unFormat_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->unFormatcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_unFormat_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_UNMATCH_FORMAT;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->unFormatcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
int get_link_recErr_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->vRecErrcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_recErr_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_RECORD_ERR;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->vRecErrcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}

int get_link_clientBreak_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,iArrayCount = 0;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL,*pArrayItem = NULL;
    if(!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        /*pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            int k = 0;
            int devNum = pAbility->devNum,chanNum = MAX_CHANNEL_SUPPORT;
            for(i = 0; i < devNum; i++)
            {
                OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;
                chanNum = pDev->chanNum;
                for(k = 0; k < chanNum; k++)
                {
                    OVFS_LINKAGE_CFG *pLinkCfg = &alarmCfg->cliBreakcfg.linkage[i][k];
                    pArrayItem = pArray;//Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Array,NULL,0,0);
                    iArrayCount = get_link_common_res(&pArrayItem,iArrayCount,i,k,pLinkCfg,pCondition);
                    //iArrayCount++;
                }
            }
        }*/
    }
    //ovfs_print_json(pResult);
    *pOutData = pResult;
    return 0;
}
int put_link_clientBreak_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iAlarmType = ALARM_TYPE_CLIENT_BREAK;
    S32 iIndex = 0,iObjType = 0;
    S32 i = 0,iSize = 0,iDev = 0,iChan = 0;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_LINKAGE_CFG *pLinkCfg;

    /*pArray = Common_Json_GetItem(pJData, -1, "ResList");
    pJData= pArray;
    if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
    {
        LOGE("ResList:null\n");
        return -1;
    }
    if(iObjType!=Common_Json_Type_Array)
    {
        LOGE("ResList!=Common_Json_Type_Array\n");
        return -1;
    }

    iSize= Common_Json_Size(pJData);
    for(i = 0; i < iSize; i++,iIndex++)
    {
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL))
            continue;
        if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL))
            continue;
        pLinkCfg = &alarmCfg->cliBreakcfg.linkage[iDev][iChan];
        put_link_common_res(pJData,iIndex,pLinkCfg,iAlarmType,getIndexByDevChan(iAlarmType,iDev,iChan));
    }
    ovfs_make_result(0,"Succ",NULL,pOutData);
    ovfs_save_alarm_cfg(g_hModuleHandle);*/
    return 0;
}
#endif
int put_custom_audio_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int ret = 0;
    char strCmd[128] = {0};
    char *alarmName = NULL, *path = NULL;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData;
LOGD("put_custom_audio_res\n");

#ifndef AWSIOT
    if(Common_Json_GetAttrValueStr(pJData, "AlarmName", &alarmName) == NULL)
    {
        ret = -1;
    }
#endif

    if(Common_Json_GetAttrValueStr(pJData, "Path", &path) == NULL)
    {
        ret = -1;
    }

    if(ret == 0)
    {
#ifdef AWSIOT
        snprintf(strCmd, sizeof(strCmd),  "mv \"%s\"  /usr/etc/cfgfiles/custom_audio",path);
#else
        if(!Common_File_IsExist("/usr/etc/soundFile"))
        {
            snprintf(strCmd, sizeof(strCmd),  "mkdir /usr/etc/soundFile");
            Common_System(strCmd);
        }
        snprintf(strCmd, sizeof(strCmd),  "mv \"%s\" /usr/etc/soundFile/sound_%s_1",path,alarmName);
#endif

        Common_System(strCmd);
    }

    ovfs_make_result(ret,ret==0?"Succ":"Error",NULL,pOutData);

    return 0;
}

int get_alarm_collect_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S32 i = 0,j = 0,iArrayCount = 0;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_EVENT *pAlarmEvent = NULL,*pTempEvent = NULL;
    OVFS_ALARM_BKBD *pAlarmBkbd = g_alarmBkbd;

    if(!pAlarmBkbd||!pAbility)
        return -1;
    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        /*if(bDetail)
        {
            Common_Time_T t_start,t_stop;
            int k = 0,iRegion = 0,nCount = 0,iState = 0;
            int devNum = 1,chanNum = MAX_CHANNEL_SUPPORT;
            devNum = pAbility->devNum;
            for(j = 0; j < ARRAYSIZE(g_alarmName); j++)
            {
                pTempEvent = pAlarmBkbd->pAlarmEvent[j];
                if(!pTempEvent)
                    continue;
                for(i = 0; i < devNum; i++)
                {
                    OVFS_DEV_ABILITY *pDev = NULL;
#ifndef SIMPLIFIED
                    if(0 == j)
                    {
                        chanNum = pAbility->alarmInNum;
                    }
                    else
#endif//ifndef SIMPLIFIED
                    {
                        pDev = pAbility->pDevAbility[i];
                        if(!pDev)
                            continue;
                        chanNum = pDev->chanNum;
                    }
                    for(k = 0; k < chanNum; k++)
                    {
                        int iChanIndex = -1;
                        iChanIndex = getIndexByDevChan(j,i, k);
                        if(iChanIndex < 0)
                        {
                            continue;
                        }
                        pAlarmEvent = &pTempEvent[iChanIndex];
                        if(!pAlarmEvent)
                        {
                            continue;
                        }
                        pAlarmEvent->iDev = i;
                        pAlarmEvent->iChan = k;

                        nCount = 1;
                        for(int s = 0; s < nCount; s++)
                        {
                            iRegion = s;

                            iState = pAlarmEvent->iState;
                            t_start = pAlarmEvent->t_start;
                            t_stop = pAlarmEvent->t_stop;

                            Common_Json_SetAttrValue(pArray,iArrayCount,"AlarmName",Common_Json_Type_String,pAlarmEvent->sName,0,0);
                            Common_Json_SetAttrValue(pArray,iArrayCount,"IsHappent",Common_Json_Type_Number,NULL,iState,0);
                            Common_Json_SetAttrValue(pArray,iArrayCount,"DevName",Common_Json_Type_String,pAlarmEvent->sDevName,0,0);
                            Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,pAlarmEvent->iDev,0);
                            Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,pAlarmEvent->iChan,0);
                            Common_Json_SetAttrValue(pArray,iArrayCount,"Stream",Common_Json_Type_Number,NULL,pAlarmEvent->iStream,0);
                            Common_Json_SetAttrValue(pArray,iArrayCount,"RegionId",Common_Json_Type_Number,NULL,iRegion,0);
                            iArrayCount++;
                        }
                    }
                }
            }
        }*/
    }
    *pOutData = pResult;
    return 0;
}

int put_alarm_collect_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    ovfs_recv_alarm(pInData,pAddData,pCondition,pOutData);
    ovfs_make_result(0,"Succ",NULL,pOutData);
    return 0;
}


int get_alarm_status_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int bDetail = 1;
    S8 strcmd[64] = {0};
    S32 i = 0,j = 0,iRet = -1,iArrayCount = 0;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    OVFS_ALARM_EVENT *pAlarmEvent = NULL,*pTempEvent = NULL;
    OVFS_ALARM_BKBD *pAlarmBkbd = g_alarmBkbd;
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

    if(!pAlarmBkbd||!pAbility)
        return -1;
    //ovfs_print_json(pJCondition);

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        if(bDetail)
        {
            Common_Time_T t_start,t_stop;
	     int iChanIndex = -1,alarmType = -1;
            int k = 0,iRegion = 0,iState = 0;
            int devNum = 1,chanNum = MAX_CHANNEL_SUPPORT;
            //devNum = pAbility->devNum;
            for(j = 0; j < ARRAYSIZE(g_alarmName); j++)
            {
                pTempEvent = pAlarmBkbd->pAlarmEvent[j];
                if(!pTempEvent)
                    continue;
                //for(i = 0; i < devNum; i++)
                {
                    //OVFS_DEV_ABILITY *pDev = NULL;

					if(strcmp(ALARM_STR_AI,g_alarmName[j]) == 0 )//when alarmName is AlarmIn,Maybe has alarmInNum event.
					{
						chanNum = pAbility->alarmInNum;
					}
                    else if(0 == strcmp(g_alarmName[j],ALARM_STR_SENSORALARM))
                    {
                        chanNum = MAX_SENSORALARM;
					}
                    else
			      	{
						/*pDev = pAbility->pDevAbility[i];
						if(!pDev)
						continue;*/
						chanNum = MAX_CHANNEL_SUPPORT;//pDev->chanNum;
						iChanIndex = chanNum-1;
			      	}
					alarmType = g_alarmIndex[j];//findAlarmType((char *)g_alarmName[j]);
                    for(k = 0; k < chanNum; k++)
                    {
                        iChanIndex = getChanIndexByDevChan(alarmType,i, k);
                        if(iChanIndex < 0)
                        {
                            continue;
                        }
                        pAlarmEvent = &pTempEvent[iChanIndex];
                        if(!pAlarmEvent)
                        {
                            continue;
                        }
                        pAlarmEvent->iDev = i;
                        pAlarmEvent->iChan = k;


                        iState = pAlarmEvent->iState;
                        t_start = pAlarmEvent->t_start;
                        t_stop = pAlarmEvent->t_stop;

                        Common_Json_SetAttrValue(pArray,iArrayCount,"AlarmName",Common_Json_Type_String,g_alarmName[j],0,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"AlarmType",Common_Json_Type_Number,NULL,alarmType,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"AlarmSrcType",Common_Json_Type_Number,NULL,pAlarmEvent->iSrcType,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"DevName",Common_Json_Type_String,pAlarmEvent->sDevName,0,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"Device",Common_Json_Type_Number,NULL,pAlarmEvent->iDev,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"Channel",Common_Json_Type_Number,NULL,pAlarmEvent->iChan,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"Stream",Common_Json_Type_Number,NULL,pAlarmEvent->iStream,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"RegionId",Common_Json_Type_Number,NULL,iRegion,0);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"Status",Common_Json_Type_Number,NULL,iState,0);

                        memset(strcmd,0,sizeof(strcmd));
                        snprintf(strcmd, sizeof(strcmd), "%04d%02d%02d%02d%02d%02d",t_start.year,t_start.month,t_start.day,t_start.hour,t_start.min,t_start.sec);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"StartTime",Common_Json_Type_String,strcmd,0,0);
                        memset(strcmd,0,sizeof(strcmd));
                        snprintf(strcmd, sizeof(strcmd), "%04d%02d%02d%02d%02d%02d",t_stop.year,t_stop.month,t_stop.day,t_stop.hour,t_stop.min,t_stop.sec);
                        Common_Json_SetAttrValue(pArray,iArrayCount,"StopTime",Common_Json_Type_String,strcmd,0,0);
                        iRet = FilterCondithon(pArray,iArrayCount,pJCondition);
                        if(iRet != 0)
                        {
                            Common_Json_RemoveItem(pArray,iArrayCount,NULL);
                            continue;
                        }
                        iArrayCount++;
                    }
                }
            }
        }
    }
    *pOutData = pResult;
    return 0;
}
#ifndef SIMPLIFIED
int get_alarm_subscribe_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    S8 label[][64] = {"TriggerCfg","LinkageCfg","Status","Action"};
    S8 uri[][64] = {"/Alarm/Subscribe/TriggerCfg","/Alarm/Subscribe/LinkageCfg","/Alarm/Subscribe/Status","/Alarm/Subscribe/Action"};

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(label); i++)
        {
            Common_Json_SetAttrValue(pArray,iArrayCount,"   ",Common_Json_Type_String,label[i],0,0);
            Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,uri[i],0,0);
            iArrayCount++;
        }
    }
    *pOutData = pResult;
    return 0;
}

int get_subscribe_trigCfg_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(g_alarmName); i++)
        {
            if(i <= ALARM_TYPE_END || (i >= ACTION2_TYPE_START &&  i <= ACTION2_TYPE_END))
            {
                S8 szUri[128] = {0};
                Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,g_alarmName[i],0,0);
                snprintf(szUri,sizeof(szUri),"/Alarm/Subscribe/TriggerCfg/%s",g_alarmName[i]);
                Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,szUri,0,0);
                iArrayCount++;
            }
        }
    }
    *pOutData = pResult;
    return 0;
}

int get_subscribe_linkCfg_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(g_alarmName); i++)
        {
            S8 szUri[128] = {0};
            Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,g_alarmName[i],0,0);
            snprintf(szUri,sizeof(szUri),"/Alarm/Subscribe/LinkageCfg/%s",g_alarmName[i]);
            Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,szUri,0,0);
            iArrayCount++;
        }
    }
    *pOutData = pResult;
    return 0;
}

int get_subscribe_status_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(g_alarmName); i++)
        {
            S8 szUri[128] = {0};
            Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,g_alarmName[i],0,0);
            snprintf(szUri,sizeof(szUri),"/Alarm/Subscribe/Status/%s",g_alarmName[i]);
            Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,szUri,0,0);
        }
    }
    *pOutData = pResult;
    return 0;
}

int get_subscribe_action_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    return 0;
}
#endif

int put_alarm_restore_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    LOGW("======Start Restoring======\n");
    Common_System("rm /usr/etc/cfgfiles/alarm.json");
    ovfs_make_result(0,"Succ",NULL,pOutData);
    return 0;
}

int get_alarm_onekeydrive_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 i = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
    S8 label[][64] = {"OneKeyDriveCfg","OneKeyDriveCtl"};
    S8 uri[][64] = {"/Alarm/OneKeyDrive/OneKeyDriveCfg","/Alarm/OneKeyDrive/OneKeyDriveCtl"};

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        for(i = 0; i < ARRAYSIZE(label); i++)
        {
            Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,label[i],0,0);
            Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,uri[i],0,0);
            iArrayCount++;
        }
    }
    *pOutData = pResult;
    return 0;
}

S32 Thread_OneKeyDrive(Common_Thread_T hThreadHandle,void *pUserData)
{
	OVFS_ALARM_CFG *alarmCfg = NULL;
	int iAudioIndex = 0;
	int iDuration = 0;
	int iEnableAudio = 0;
	int iEnableLight = 0;
	int iResetEnable = 0;

    char path[128] = {0};

	Common_Thread_Detach(Common_Thread_Self());

	alarmCfg = ovfs_get_alarm_cfg();


#ifdef AWSIOT
    //snprintf(path, sizeof(path), "/usr/etc/cfgfiles/custom_audio");
    //if(!Common_File_IsExist(path))
    {
        snprintf(path, sizeof(path), "/usr/etc/cfgfiles/default/custom_audio");
        if(!Common_File_IsExist(path))
        {
            snprintf(path, sizeof(path), "/update/soundFile/sound_alarm_default");
        }
    }
#else
    snprintf(path, sizeof(path), "/update/soundFile/sound_RegionalInvasion_%d", iAudioIndex);
#endif

	while(1)
	{
		if(g_iRestTime <= 0 || g_iDriveThread == 0)
		{
			//char path[128] = {0};
	        //sprintf(path, "/update/soundFile/sound_RegionalInvasion_%d", iAudioIndex);
	        Action_SoundAlarm(g_hModuleHandle, iAudioIndex, path, 1, 0);

			Action_LightAlarmv2(g_hModuleHandle, 0);

			g_iDriveThread = 0;

			break;
		}

		if(alarmCfg->oneKeyDriveCfg.EnableAudio)
		{
			//char path[128] = {0};
	        //sprintf(path, "/update/soundFile/sound_RegionalInvasion_%d", iAudioIndex);
	        Action_SoundAlarm(g_hModuleHandle, iAudioIndex, path, 1, 1);
		}
		else
		{
			//char path[128] = {0};
	        //sprintf(path, "/update/soundFile/sound_RegionalInvasion_%d", iAudioIndex);
	        Action_SoundAlarm(g_hModuleHandle, iAudioIndex, path, 1, 0);
		}

        if(iEnableLight != alarmCfg->oneKeyDriveCfg.EnableLight)
        {
            iEnableLight = alarmCfg->oneKeyDriveCfg.EnableLight;
            if(alarmCfg->oneKeyDriveCfg.EnableLight)
    		{
    			Action_LightAlarmv2(g_hModuleHandle, 1);
    		}
    		else
    		{
    			Action_LightAlarmv2(g_hModuleHandle, 0);
    		}
        }

		g_iRestTime--;
		Common_Sleep(1, 0);
	}

	return NULL;
}

int get_onekey_drive_ctl(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pChild = NULL;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pChild,-1,"Enable",Common_Json_Type_Number,NULL,g_iDriveThread,0);
		Common_Json_SetAttrValue(pChild,-1,"RemainTime",Common_Json_Type_Number,NULL,g_iRestTime,0);
    }
    *pOutData = pResult;
    return 0;
}

int put_onekey_drive_ctl(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iEnable = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

	iEnable = -1;
	Common_Json_GetAttrValue(pJData, -1, "Enable",NULL, NULL, &iEnable, NULL);

	LOGD("iEnable=[%d]\n", iEnable);

	if(iEnable > 1 || iEnable < 0)
	{
		ovfs_make_result(-1,"Failed",NULL,pOutData);
		return 0;
	}

    if(iEnable == 1)
    {
		g_iRestTime = alarmCfg->oneKeyDriveCfg.Duration;
		if(g_iDriveThread == 0)
		{
			g_iDriveThread = 1;
			Common_Thread_T pOneKeyDriveThread = NULL;
			Common_Thread_Create(&pOneKeyDriveThread,__FUNCTION__,0,0,Thread_OneKeyDrive,NULL);
		}
    }
	else if(iEnable == 0)
	{
		g_iRestTime = 0;
		g_iDriveThread = 0;
	}

    ovfs_make_result(0,"Succ",NULL,pOutData);
    return 0;
}

int get_onekey_drive_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pChild = NULL;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pChild,-1,"Duration",Common_Json_Type_Number,NULL,alarmCfg->oneKeyDriveCfg.Duration,0);
		Common_Json_SetAttrValue(pChild,-1,"EnableAudio",Common_Json_Type_Number,NULL,alarmCfg->oneKeyDriveCfg.EnableAudio,0);
		Common_Json_SetAttrValue(pChild,-1,"EnableLight",Common_Json_Type_Number,NULL,alarmCfg->oneKeyDriveCfg.EnableLight,0);
    }
    *pOutData = pResult;
    return 0;
}

int put_onekey_drive_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iDuration = -1,iEnableAudio = -1,iEnableLight = -1;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

	iDuration = -1;
	Common_Json_GetAttrValue(pJData, -1, "Duration",NULL, NULL, &iDuration, NULL);

	iEnableAudio = -1;
    Common_Json_GetAttrValue(pJData, -1, "EnableAudio",NULL, NULL, &iEnableAudio, NULL);

	iEnableLight = -1;
    Common_Json_GetAttrValue(pJData, -1, "EnableLight",NULL, NULL, &iEnableLight, NULL);

	LOGD("iDuration=[%d], iEnableAudio = [%d], iEnableLight = [%d]\n", iDuration, iEnableAudio, iEnableLight);

	if(iDuration < 0 || iEnableAudio > 1 || iEnableAudio < 0 || iEnableLight > 1 || iEnableLight < 0)
	{
		ovfs_make_result(-1,"Failed",NULL,pOutData);
		return 0;
	}

    if(iDuration != alarmCfg->oneKeyDriveCfg.Duration)
    {
		alarmCfg->oneKeyDriveCfg.Duration = iDuration;
		g_bCfgChange = 1;
    }

	if(iEnableAudio != alarmCfg->oneKeyDriveCfg.EnableAudio)
	{
		alarmCfg->oneKeyDriveCfg.EnableAudio = iEnableAudio;
		g_bCfgChange = 1;
	}

	if(iEnableLight != alarmCfg->oneKeyDriveCfg.EnableLight)
	{
		alarmCfg->oneKeyDriveCfg.EnableLight = iEnableLight;
		g_bCfgChange = 1;
	}

	g_iRestTime = alarmCfg->oneKeyDriveCfg.Duration;

    ovfs_make_result(0,"Succ",NULL,pOutData);
	ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

int get_push_config_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pChild = NULL;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pResult)
    {
        Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",0,0);
        pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pChild,-1,"Enable",Common_Json_Type_Number,NULL,alarmCfg->pushCfg.enable,0);
		Common_Json_SetAttrValue(pChild,-1,"Url",Common_Json_Type_String,alarmCfg->pushCfg.url,0,0);
		Common_Json_SetAttrValue(pChild,-1,"PushInterval",Common_Json_Type_Number,NULL,alarmCfg->pushCfg.interval,0);
		Common_Json_SetAttrValue(pChild,-1,"PushImage",Common_Json_Type_Number,NULL,alarmCfg->pushCfg.withImage,0);
		Common_Json_SetAttrValue(pChild,-1,"UpdateImage",Common_Json_Type_Number,NULL,alarmCfg->pushCfg.updateImage,0);
    }
    *pOutData = pResult;
    return 0;
}

int put_push_config_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    S32 iNum = -1;
    char *pStr = NULL;
    cJSON_Struct *pJData = (cJSON_Struct *)pAddData;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();

    if(Common_Json_GetAttrValueInt(pJData, "Enable", &iNum))
    {
		alarmCfg->pushCfg.enable = iNum;
		g_bCfgChange = 1;
    }

    if(Common_Json_GetAttrValueStr(pJData, "Url", &pStr))
    {
        snprintf(alarmCfg->pushCfg.url, sizeof(alarmCfg->pushCfg), "%s", pStr);
		g_bCfgChange = 1;
    }

    if(Common_Json_GetAttrValueInt(pJData, "PushInterval", &iNum))
    {
		alarmCfg->pushCfg.interval = iNum;
		g_bCfgChange = 1;
    }

    if(Common_Json_GetAttrValueInt(pJData, "PushImage", &iNum))
    {
		alarmCfg->pushCfg.withImage = iNum;
		g_bCfgChange = 1;
    }

    if(Common_Json_GetAttrValueInt(pJData, "UpdateImage", &iNum))
    {
        alarmCfg->pushCfg.updateImage = iNum;
        g_bCfgChange = 1;
    }

    ovfs_make_result(0,"Succ",NULL,pOutData);
	ovfs_save_alarm_cfg(g_hModuleHandle);
    return 0;
}

int ovfs_get_cfgChange()
{
    return g_bCfgChange;
}

void ovfs_set_cfgChange(int bChange)
{
    g_bCfgChange = bChange;
}

cJSON_Struct * ovfs_get_alarm_status(cJSON_Struct *pJCondition)
{
    cJSON_Struct *pJResult = NULL;
    get_alarm_status_res(NULL,NULL,pJCondition,&pJResult);
    return pJResult;
}

cJSON_Struct * ovfs_get_rest()
{
    return g_rest_res;
}

S32 ovfs_get_debug()
{
    return g_bPrintDbg;
}

int updateStoreItem(ModuleHandle_T hModuleHandle)
{
    S32 nRet = -1;
    cJSON_Struct *pConfig = NULL,*pOutParams = NULL,*pChild = NULL,*pArray = NULL,*pItem = NULL;
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Core/Restore/Update",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"put",0,0);

        pChild = Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

        pItem = Common_Json_SetAttrValue(pArray,0,NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pItem,-1,"Uri",Common_Json_Type_String,"/Alarm/Restore",0,0);
        Common_Json_SetAttrValue(pItem,-1,"Label",Common_Json_Type_String,"AlarmCfg",0,0);
        Common_Json_SetAttrValue(pItem,-1,"NeedReboot",Common_Json_Type_Number,NULL,1,0);
        nRet = Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
        if(nRet != 0)
        {
            ovfs_print_json(pConfig);
            LOGE("nRet:%d\n",nRet);
        }
        Common_Json_Delete(pConfig);
        Common_Json_Delete(pOutParams);
    }
    return nRet;
}

int ovfs_init_alarm_res(ModuleHandle_T hModuleHandle)
{
    //int i = 0;
    cJSON_Struct *pRoot = NULL,*pChild = NULL,*pChild1 = NULL;
    g_hModuleHandle = hModuleHandle;
    g_rest_res = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (g_rest_res)
    {
        pRoot = Common_Json_SetAttrValue(g_rest_res,-1,"Alarm",Common_Json_Type_Object,NULL,0,0);
        set_method_callback(&pRoot,get_alarm_top_res,NULL,NULL,NULL,NULL,0);
        if(pRoot)
        {
            pChild = Common_Json_SetAttrValue(pRoot,-1,"PrintDebug",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_debug_res,put_alarm_debug_res,NULL,NULL,NULL,0);

            pChild = Common_Json_SetAttrValue(pRoot,-1,"Arming",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_arming_res,put_alarm_arming_res,NULL,NULL,NULL,0);
#ifndef SIMPLIFIED
            pChild = Common_Json_SetAttrValue(pRoot,-1,"EmailCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_email_res,put_alarm_email_res,NULL,NULL,NULL,0);
#endif
            /*pChild = Common_Json_SetAttrValue(pRoot,-1,"SoundAlarmCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_sound_res,put_alarm_sound_res,NULL,NULL,NULL,0);

            pChild = Common_Json_SetAttrValue(pRoot,-1,"LightAlarmCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_light_res,put_alarm_light_res,NULL,NULL,NULL,0);*/
#ifndef SIMPLIFIED

            pChild = Common_Json_SetAttrValue(pRoot,-1,"FTPCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_ftp_snap_res,put_ftp_snap_res,NULL,NULL,NULL,0);
#endif

            /*pChild = Common_Json_SetAttrValue(pRoot,-1,"RedBlueLightCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_redblue_light_res,put_redblue_light_res,NULL,NULL,NULL,0);

            pChild = Common_Json_SetAttrValue(pRoot,-1,"TriggerCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_trigCfg_res,NULL,NULL,NULL,NULL,0);
            if(pChild)
            {
#ifndef SIMPLIFIED
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_AI,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_trig_alarmIn_res,put_trig_alarmIn_res,NULL,NULL,NULL,0);
#endif
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_MOTION,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_trig_motion_res,put_trig_motion_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_VHIDE,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_trig_vhide_res,put_trig_vhide_res,NULL,NULL,NULL,0);

                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_DETPERSON,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_trig_detPerson_res,put_trig_detPerson_res,NULL,NULL,NULL,0);

            }

            pChild = Common_Json_SetAttrValue(pRoot,-1,"LinkageCfg",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_linkCfg_res,NULL,NULL,NULL,NULL,0);
            if(pChild)*/
            {
                pChild = pRoot;
#ifndef SIMPLIFIED
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_AI,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
#endif
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_MOTION,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_motion_res,put_link_motion_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_VHIDE,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_vhide_res,put_link_vhide_res,NULL,NULL,NULL,0);

                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_REGIONALINVASION,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_DETWIRE,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_PERSONSTAYING,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_DETABSENT,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_PARKINGVIOLATION,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_RETROGRADE,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_SENSORALARM,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_common_res,put_link_common_res,NULL,NULL,NULL,0);

                //pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_DETPERSON,Common_Json_Type_Object,NULL,0,0);
                //set_method_callback(&pChild1,get_link_detPerson_res,put_link_detPerson_res,NULL,NULL,NULL,0);

                pChild1 = Common_Json_SetAttrValue(pChild,-1,"SetCustomAudio",Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,NULL,put_custom_audio_res,NULL,NULL,NULL,0);


                pChild1	 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_NETBREAK,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_netBreak_res,put_link_netBreak_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_IPCONFLICT,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_ipConflit_res,put_link_ipConflit_res,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,ALARM_STR_ILLACCESS,Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_link_illegalAcc_res,put_link_illegalAcc_res,NULL,NULL,NULL,0);
            }
            pChild = Common_Json_SetAttrValue(pRoot,-1,"CollectData",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_collect_res,put_alarm_collect_res,NULL,NULL,NULL,0);
            pChild = Common_Json_SetAttrValue(pRoot,-1,"Status",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_alarm_status_res,NULL,NULL,NULL,NULL,0);
#ifndef SIMPLIFIED
            pChild = Common_Json_SetAttrValue(pRoot,-1,"Subscribe",Common_Json_Type_Object,"",11,0);
            set_method_callback(&pChild,get_alarm_subscribe_res,NULL,NULL,NULL,NULL,0);
            if(pChild)
            {
                /*pChild1 = Common_Json_SetAttrValue(pChild,-1,"TriggerCfg",Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_subscribe_trigCfg_res,NULL,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,"LinkageCfg",Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_subscribe_linkCfg_res,NULL,NULL,NULL,NULL,0);*/
                pChild1 = Common_Json_SetAttrValue(pChild,-1,"Status",Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_subscribe_status_res,NULL,NULL,NULL,NULL,0);
                pChild1 = Common_Json_SetAttrValue(pChild,-1,"Action",Common_Json_Type_Object,NULL,0,0);
                set_method_callback(&pChild1,get_subscribe_action_res,NULL,NULL,NULL,NULL,0);
            }
#endif
			pChild = Common_Json_SetAttrValue(pRoot,-1,"OneKeyDrive",Common_Json_Type_Object,"",11,0);
            set_method_callback(&pChild,get_alarm_onekeydrive_res,NULL,NULL,NULL,NULL,0);
			if(pChild)
            {
            	pChild1 = Common_Json_SetAttrValue(pChild,-1,"OneKeyDriveCfg",Common_Json_Type_Object,NULL,0,0);
	            set_method_callback(&pChild1,get_onekey_drive_res,put_onekey_drive_res,NULL,NULL,NULL,0);
				pChild1 = Common_Json_SetAttrValue(pChild,-1,"OneKeyDriveCtl",Common_Json_Type_Object,NULL,0,0);
	            set_method_callback(&pChild1,get_onekey_drive_ctl,put_onekey_drive_ctl,NULL,NULL,NULL,0);
			}
            pChild = Common_Json_SetAttrValue(pRoot,-1,"Restore",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,NULL,put_alarm_restore_res,NULL,NULL,NULL,0);

            pChild = Common_Json_SetAttrValue(pRoot,-1,"PushConfig",Common_Json_Type_Object,NULL,0,0);
            set_method_callback(&pChild,get_push_config_res,put_push_config_res,NULL,NULL,NULL,0);


            updateStoreItem(hModuleHandle);
        }
    }
    return 0;
}

OVFS_ALARM_BKBD* ovfs_get_alarm_bkbd()
{
    return g_alarmBkbd;
}

int ovfs_reset_alarm_bkbd()
{
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    OVFS_ALARM_EVENT *pAlarmEvent = NULL,*pTemp = NULL;
    int i = 0,j = 0,totalChanNum = MAX_CHANNEL_SUPPORT,iAlarmCount = 0;

    if(!pAbility)
        return -1;
    //totalChanNum = pAbility->totalChanNum;
    Common_Lock(g_alarmBkbd->alarmLock);
    if(g_alarmBkbd)
    {
        iAlarmCount = g_alarmBkbd->alarmCount;
        for(i = 0; i < iAlarmCount; i++)
        {
            pAlarmEvent = g_alarmBkbd->pAlarmEvent[i];
            if(pAlarmEvent)
            {
                if(strcmp(ALARM_STR_AI,g_alarmName[i]) == 0)
				{
					totalChanNum = pAbility->alarmInNum;
				}
                else if(0 == strcmp(g_alarmName[i],ALARM_STR_SENSORALARM))
                {
                    totalChanNum = MAX_SENSORALARM;
				}
                else
		      	{
					totalChanNum = MAX_CHANNEL_SUPPORT;
		      	}

                for(j = 0; j < totalChanNum; j++)
                {
                    pTemp = &pAlarmEvent[j];
                    if(pTemp)
                        pTemp->iState = 0;
                }
            }
        }
    }
    Common_UnLock(g_alarmBkbd->alarmLock);
    return 0;
}

int ovfs_init_alarm_bkbd(ModuleHandle_T hModuleHandle)
{
    OVFS_ALARM_EVENT *pAlarmEvent = NULL;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    int AlarmCount = ARRAYSIZE(g_alarmName);
    int i = 0,j = 0,iRet = -1,totalChanNum = MAX_CHANNEL_SUPPORT;

    if(!pAbility)
    {
        LOGE("pAbility is null\n");
        return -1;
    }
    //totalChanNum = pAbility->totalChanNum;
    if(!g_alarmBkbd)
    {
        g_alarmBkbd = (OVFS_ALARM_BKBD*)Common_Malloc(sizeof(OVFS_ALARM_BKBD), 0, __FUNCTION__, __LINE__);
        if(g_alarmBkbd)
        {
            memset(g_alarmBkbd,0,sizeof(OVFS_ALARM_BKBD));
            g_alarmBkbd->alarmCount = AlarmCount;
            iRet= Common_Lock_Create(&g_alarmBkbd->alarmLock,NULL);
            if(-1 == iRet)
            {
                Common_Free(g_alarmBkbd, __FUNCTION__, __LINE__);
                return -1;
            }
            g_alarmBkbd->pAlarmEvent = (OVFS_ALARM_EVENT **)Common_Malloc(AlarmCount*sizeof(OVFS_ALARM_EVENT *), 0, __FUNCTION__, __LINE__);
            if(g_alarmBkbd->pAlarmEvent)
            {
                memset(g_alarmBkbd->pAlarmEvent,0,AlarmCount*sizeof(OVFS_ALARM_EVENT *));
                for(i = 0; i < AlarmCount; i++)
                {
                    if(0 == strcmp(g_alarmName[i],ALARM_STR_AI)) //when alarmName is AlarmIn,Maybe has alarmInNum event.
                        totalChanNum = pAbility->alarmInNum;
                    else if(0 == strcmp(g_alarmName[i],ALARM_STR_SENSORALARM))
                        totalChanNum = MAX_SENSORALARM;
                    else
                        totalChanNum = MAX_CHANNEL_SUPPORT;//pAbility->totalChanNum;

                    pAlarmEvent = (OVFS_ALARM_EVENT *)Common_Malloc(totalChanNum*sizeof(OVFS_ALARM_EVENT), 0, __FUNCTION__, __LINE__);
                    if(pAlarmEvent)
                    {
                        memset(pAlarmEvent,0,totalChanNum*sizeof(OVFS_ALARM_EVENT));
                        g_alarmBkbd->pAlarmEvent[i] = pAlarmEvent;
                        for(j = 0; j < totalChanNum; j++)
                        {
                            pAlarmEvent[j].sName = Common_StrDup((S8*)g_alarmName[i], __FUNCTION__, __LINE__);
                            pAlarmEvent[j].sDevName = NULL;
                            pAlarmEvent[j].iIndexChan = j;
                            pAlarmEvent[j].iType = i;
                            if(i< ALARM_TYPE_DISKFULL)
                                pAlarmEvent[j].iSrcType = 1;
                            else
                                pAlarmEvent[j].iSrcType = 0;
                        }
                    }
                }
            }

            // action state init
            Common_Lock_Create(&g_alarmBkbd->actionLock,NULL);
            memset(&(g_alarmBkbd->actionState),0,sizeof(g_alarmBkbd->actionState));
        }
    }

    return 0;
}
