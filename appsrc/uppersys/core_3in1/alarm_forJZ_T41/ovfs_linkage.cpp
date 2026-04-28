/*
 * ovfs_linkage.c
 *
 *  Created on: 2017-3-07
 *      Author:
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
#include <map>

#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "ovfs_alarm_common.h"
#include "ovfs_alarm_method.h"
#include "ovfs_alarm_cfg.h"
#include "ovfs_alarm.h"
#include "ovfs_linkage.h"

#include "curl/curl.h"
#include <map>

//#define MOTORCUSTOM
#define CUSTOM_AUDIO "/usr/etc/cfgfiles/custom_audio"

using namespace std;

typedef struct
{
    ModuleHandle_T hModuleHandle;
    OVFS_LINK_SNAP_CFG snapCfg;
    OVFS_ALARM_EVENT *pAlarmEvent;
    S32 bSnap;
} SNAP_DATA;
static SNAP_DATA g_SnapData;

typedef struct
{
    ModuleHandle_T hModuleHandle;
    S32 remainTime;
    S32 status;
} LIGHT_DATA;
static LIGHT_DATA g_LightData;

#ifndef SIMPLIFIED

typedef struct
{
    ModuleHandle_T hModuleHandle;
    S8 FileName[128];
    S32 bUpload;
} FTP_DATA;
static FTP_DATA g_FtpData;

extern int oper_snap(ModuleHandle_T hModuleHandle,int iDev,int iChan,OVFS_ALARM_EVENT *pAlarmEvent,S8 **pOutPath);

/*
int notifyReleaseFile(ModuleHandle_T hModuleHandle,S8 *szPath)
{
    S32 iRet = -1;
    cJSON_Struct *pInData = NULL,*pOutData = NULL;

    if(!szPath ||strlen(szPath) <= 0)
    {
        LOGE("szPath is null or len <= 0");
        return -1;
    }
    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pInData != NULL)
    {
        Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/BoardSys/Snap/Release",0,0);
        Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pInData,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInData,-1,"Data/Path",Common_Json_Type_String,szPath,0,0);
        iRet = Module_CallFunctions(hModuleHandle,pInData,&pOutData,3000);
        if(iRet!=0)
        {
            LOGE("iRet : %d\n",iRet);
            ovfs_print_json(pInData);
        }
        Common_Json_Delete(pInData);
        Common_Json_Delete(pOutData);
    }
    return 0;
}
*/
#endif

S32 Thread_AlarmPush(Common_Thread_T hThreadHandle,void *pUserData);


static int check_alarmLink(S32 iLinkType, S32 index)
{
    S32 iCount = 0;
    S32 i = 0,j = 0,k = 0;
    int devNum = 0,chanNum = 0;
    OVFS_LINKAGE_CFG *pLinkageCfg = NULL;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_ALARM_BKBD *pAlarmBkbd = ovfs_get_alarm_bkbd();
    OVFS_ALARM_EVENT *pAlarmEvent = NULL,*pTempEvent = NULL;

    if(!pAbility||!pAlarmBkbd)
        return -1;

    /*for(j = 0; j < ARRAYSIZE(g_alarmName); j++)
    {
#ifndef SIMPLIFIED
        if(j < ALARM_TYPE_MAX)
        {
            pTempEvent = pAlarmBkbd->pAlarmEvent[j];
        }
#else
        pTempEvent = pAlarmBkbd->pAlarmEvent[j];
#endif

        if(!pTempEvent)
        {
            continue;
        }
        devNum = pAbility->devNum;
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
                {
                    continue;
                }
                chanNum = pDev->chanNum;
            }
            for(k = 0; k < chanNum; k++)
            {
                int iChanIndex = -1;
//#ifndef JZT31N_8M
                if(ALARM_TYPE_ALARMIN == j)
                {
                    pLinkageCfg = &pAlarmCfg->alarmIncfg.linkage[i][k];
                }
                else if(ALARM_TYPE_MOTION == j)
                {
                    pLinkageCfg = &pAlarmCfg->motioncfg.linkage[i][k];
                }

                else if(ALARM_TYPE_VHIDE == j)
                {
                    pLinkageCfg = &pAlarmCfg->vhidecfg.linkage[i][k];
                }

                else if(ALARM_TYPE_DETECT_PERSON == j)
                {
                    pLinkageCfg = &pAlarmCfg->PersonDetcfg.linkage[i][k];
                }
                else if(ALARM_TYPE_CABLE_BREAK == j)
                {
                    pLinkageCfg = &pAlarmCfg->netBreakcfg.linkage[i][k];
                }
                else if(ALARM_TYPE_IP_CONFLIT == j)
                {
                    pLinkageCfg = &pAlarmCfg->ipConflitcfg.linkage[i][k];
                }
                else if(ALARM_TYPE_ILLEG_ACCESS == j)
                {
                    pLinkageCfg = &pAlarmCfg->illAccesscfg.linkage[i][k];
                }

                else
                {
                    continue;
                }
                iChanIndex = getChanIndexByDevChan(j,i, k);
                if(iChanIndex < 0)
                {
                    continue;
                }
                pAlarmEvent = &pTempEvent[iChanIndex];
                if(!pAlarmEvent)
                {
                    continue;
                }
                int bArming = 0;
                Common_Time_T t_happent;
                Common_Linux2CommonTime(time(NULL),&t_happent);

                if(3 == iLinkType)
                {
                    OVFS_LINK_RECORD_CFG *pLinkRecord = &pLinkageCfg->linkRecord;
                    bArming = checkSchedTime(pAlarmEvent,t_happent);
                    if(pAlarmEvent->iState && bArming)		//????????
                    {
                        if(pLinkRecord->enable)
                            iCount++;
                    }
                }
//#ifndef JZT31N_8M
                else if(6 == iLinkType)
                {
                    int chu,yu;
                    chu = index/8,yu = index%8;
                    OVFS_LINK_AOUT_CFG *pLinkAout = &pLinkageCfg->linkAout;
                    bArming = checkSchedTime(pAlarmEvent,t_happent);
                    if(pAlarmEvent->iState && bArming)
                    {
                        if(pLinkAout->mask[chu]&(1<<yu))
                            iCount++;
                    }
                }
//#endif
                else if(9 == iLinkType)
                {
                    OVFS_LINK_LIGHTALARM_CFG *pLinkLightAlarm = &pLinkageCfg->linkLightAlarm;
                    bArming = checkSchedTime(pAlarmEvent,t_happent);
                    if(pAlarmEvent->iState && bArming)
                    {
                        if(pLinkLightAlarm->enable)
                            iCount++;
                    }
                }
                else if(10 == iLinkType)
                {
                    OVFS_LINK_LIGHTALARM_CFG *pRedBlueLight = &pLinkageCfg->linkRedblueLight;
                    bArming = checkSchedTime(pAlarmEvent,t_happent);
                    if(pAlarmEvent->iState && bArming)
                    {
                        if(pRedBlueLight->enable)
                            iCount++;
                    }
                }
            }
        }
    }*/
    return iCount;
}

/*
int ovfs_linkReportCentre(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 iRet = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/alarm/triggercfg/alarmin",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/AlarmName",Common_Json_Type_String,pAlarmEvent->sName,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/DeviceNo",Common_Json_Type_Number,NULL,pAlarmEvent->iDev,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/ChannelNo",Common_Json_Type_Number,NULL,pAlarmEvent->iChan,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);

    }
    return iRet;
}
*/
#ifndef SIMPLIFIED
int ovfs_linkMail(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 iCount = 0;
    S8 *pTemp = NULL;
    S8 strcmd[1025] = {0},sDevName[513] = {0},sChName[513] = {0};
    S32 iRet = 0,i = 0,j = 0,iArrayCount = 0;
    OVFS_LINK_MAIL_CFG *pLinkMail = NULL;
    OVFS_ALARM_CFG *alarmcfg = ovfs_get_alarm_cfg();
    cJSON_Struct *pResult = NULL,*pRoot = NULL,*pData = NULL,*pChild = NULL,*pArray = NULL,*pItem = NULL;

    if(!pAlarmEvent||!pLinkageCfg)
    {
        LOGE("pAlarmEvent ||pLinkageCfg is null\n");
        return -1;
    }
	LOGD("Link Mail\n");
    pLinkMail = &pLinkageCfg->linkMail;

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/Core/DeviceName",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
        Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        Common_Json_Delete(pRoot);
        Common_Json_GetAttrValue(pResult, -1, "Data/DeviceName", NULL, &pTemp, NULL, NULL);
        if(pTemp)
            snprintf(sDevName,512,"%s",pTemp);
        Common_Json_Delete(pResult);
        pRoot = NULL,pResult = NULL,pTemp = NULL;
    }
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        if(0 != Common_StriCmp(pAlarmEvent->sName,(S8*)ALARM_STR_AI))
            snprintf(strcmd,1024,"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream0",pAlarmEvent->iDev,pAlarmEvent->iChan);
        else
            snprintf(strcmd,1024,"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel0/Stream0",pAlarmEvent->iDev);
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
        Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        Common_Json_Delete(pRoot);
        Common_Json_GetAttrValue(pResult, -1, "Data/String", NULL, &pTemp, NULL, NULL);
        if(pTemp)
            snprintf(sChName,512,"%s",pTemp);
        Common_Json_Delete(pResult);
        pRoot = NULL,pResult = NULL,pTemp = NULL;
    }

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(Common_File_IsExist("/root/lib/libnetwork_sdk.so"))
        {
            Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Webserver/Network/Functions/SendMail");
        }
        else
        {
    	    Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Network/Functions/SendMail");
    	}

        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        pData = Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pChild = Common_Json_SetAttrValue(pData,-1,"Sender",Common_Json_Type_Object,NULL,0,0);
        LOGD("Link Mail bDefault:[%d][%s][%s]\n",pLinkMail->bDefault,pLinkMail->sender.pName,alarmcfg->mailcfg.sender.pName);
        Common_Json_SetAttrValue(pChild,-1,"User",Common_Json_Type_String,pLinkMail->bDefault?pLinkMail->sender.pName:alarmcfg->mailcfg.sender.pName,0,0);
        Common_Json_SetAttrValue(pChild,-1,"Password",Common_Json_Type_String,pLinkMail->bDefault?pLinkMail->sender.pPwd:alarmcfg->mailcfg.sender.pPwd,0,0);
        Common_Json_SetAttrValue(pChild,-1,"SMTPServer",Common_Json_Type_String,pLinkMail->bDefault?pLinkMail->sender.pSmtpSvr:alarmcfg->mailcfg.sender.pSmtpSvr,0,0);
        Common_Json_SetAttrValue(pChild,-1,"SMTPPort",Common_Json_Type_Number,NULL,pLinkMail->bDefault?pLinkMail->sender.port:alarmcfg->mailcfg.sender.port,0);
        Common_Json_SetAttrValue(pChild,-1,"EnableSSL",Common_Json_Type_Number,NULL,pLinkMail->bDefault?pLinkMail->sender.bEnSsl:alarmcfg->mailcfg.sender.bEnSsl,0);
        Common_Json_SetAttrValue(pChild,-1,"ServerVerify",Common_Json_Type_Number,NULL,pLinkMail->bDefault?pLinkMail->sender.SvrVerify:alarmcfg->mailcfg.sender.SvrVerify,0);

        char realName[MAX_NAMELEN];
        memset(realName,0,sizeof(realName));
        strncpy(realName,pAlarmEvent->sName,sizeof(realName));

        snprintf(strcmd,1024,"%s %s",realName,pAlarmEvent->iState?"generator":"release");

        Common_Json_SetAttrValue(pRoot,-1,"Data/Title",Common_Json_Type_String,strcmd,0,0);

        if((pLinkMail->bDefault&&pLinkMail->bAttach)||((!pLinkMail->bDefault)&&alarmcfg->mailcfg.bAttach))
        {
            pArray = Common_Json_SetAttrValue(pData,-1,"Attachment",Common_Json_Type_Array,NULL,0,0);
            oper_snap(hModuleHandle, 0, 0, pAlarmEvent, &pTemp);
            if(pTemp)
            {
                Common_Json_SetAttrValue(pArray,0,NULL,Common_Json_Type_String,pTemp,0,0);
                Common_Free(pTemp, __FUNCTION__, __LINE__);
                pTemp = NULL;
            }
        }

        Common_Time_T t_start;
        Common_Time_T t_stop;

        if(0 == Common_StriCmp((S8 *)ALARM_STR_AI, pAlarmEvent->sName))
        {
            memcpy(&t_start, &pAlarmEvent->t_start, sizeof(Common_Time_T));
            memcpy(&t_stop, &pAlarmEvent->t_stop, sizeof(Common_Time_T));
        }
        else
        {
            memcpy(&t_start, &pAlarmEvent->t_start, sizeof(Common_Time_T));
            memcpy(&t_stop, &pAlarmEvent->t_stop, sizeof(Common_Time_T));
        }

        tzset();


        if(pAlarmEvent->iState)
        {
            time_t time;
            Common_CommonGm2LinuxTime(&t_start, &time);

            struct tm p;
            localtime_r (&time, &p);
            t_start.year = 1900 + p.tm_year;
            t_start.month = 1 + p.tm_mon;
            t_start.day = p.tm_mday;

            t_start.hour = p.tm_hour;
            t_start.min = p.tm_min;
            t_start.sec = p.tm_sec;

            snprintf(strcmd, sizeof(strcmd), "DeviceName:%s\nChannelName:%s\nStartTime[%04d-%02d-%02d %02d:%02d:%02d] TRIGGER %s %02d",
                     sDevName,sChName,t_start.year,t_start.month,t_start.day,t_start.hour,t_start.min,t_start.sec,pAlarmEvent->sName,pAlarmEvent->iChan+1);
        }
        else
        {
            time_t time;
            Common_CommonGm2LinuxTime(&t_stop, &time);

            struct tm p;
            localtime_r (&time, &p);
            t_stop.year = 1900 + p.tm_year;
            t_stop.month = 1 + p.tm_mon;
            t_stop.day = p.tm_mday;

            t_stop.hour = p.tm_hour;
            t_stop.min = p.tm_min;
            t_stop.sec = p.tm_sec;

            snprintf(strcmd, sizeof(strcmd), "DeviceName:%s\nChannelName:%s\nStopTime[%04d-%02d-%02d %02d:%02d:%02d] %s %02d",
                     sDevName,sChName,t_stop.year,t_stop.month,t_stop.day,t_stop.hour,t_stop.min,t_stop.sec,pAlarmEvent->sName,pAlarmEvent->iChan+1);
        }

        //LOGE("strcmd=%s\n", strcmd);

        //snprintf(strcmd, sizeof(strcmd), "dev%02d ch%02d %s %s",pAlarmEvent->iDev,pAlarmEvent->iChan,pAlarmEvent->sName,pAlarmEvent->iState?"generator":"release");
        Common_Json_SetAttrValue(pRoot,-1,"Data/Content",Common_Json_Type_String,strcmd,0,0);
        iArrayCount = 0;
        pArray = Common_Json_SetAttrValue(pData,-1,"Receiver",Common_Json_Type_Array,NULL,0,0);
        if(pLinkMail->bDefault)
            iCount = pLinkMail->recvList.count;
        else
            iCount = alarmcfg->mailcfg.recvList.count;
        for(i = 0; i < iCount; i++)
        {
            if(pLinkMail->bDefault/*&&strlen(pLinkMail->recvList.recver[i].pName) > 1*/&&strlen(pLinkMail->recvList.recver[i].pAddr)>1)
            {
                pItem = Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Object,NULL,0,0);
                Common_Json_SetAttrValue(pItem,-1,"Name",Common_Json_Type_String,pLinkMail->recvList.recver[i].pName,0,0);
                Common_Json_SetAttrValue(pItem,-1,"Address",Common_Json_Type_String,pLinkMail->recvList.recver[i].pAddr,0,0);
                iArrayCount++;
            }
            else if(!pLinkMail->bDefault/*&&strlen(alarmcfg->mailcfg.recvList.recver[i].pName) > 1*/&&strlen(alarmcfg->mailcfg.recvList.recver[i].pAddr)>1)
            {
                pItem = Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Object,NULL,0,0);
                Common_Json_SetAttrValue(pItem,-1,"Name",Common_Json_Type_String,alarmcfg->mailcfg.recvList.recver[i].pName,0,0);
                Common_Json_SetAttrValue(pItem,-1,"Address",Common_Json_Type_String,alarmcfg->mailcfg.recvList.recver[i].pAddr,0,0);
                iArrayCount++;
            }
        }
        if(iArrayCount == 0)
        {
            LOGW("Not set Email param!\n");
            ovfs_print_json(pRoot);
            Common_Json_Delete(pRoot);
            return 0;
        }
        if(pLinkMail->bDefault&&(strlen(pLinkMail->sender.pSmtpSvr)<1||strlen(pLinkMail->sender.pName)<2||strlen(pLinkMail->sender.pPwd)<1))
        {
            LOGW("Not set Email param!\n");
            ovfs_print_json(pRoot);
            Common_Json_Delete(pRoot);
            return 0;
        }
        if(!pLinkMail->bDefault &&(strlen(alarmcfg->mailcfg.sender.pSmtpSvr)<1||strlen(alarmcfg->mailcfg.sender.pName)<2||strlen(alarmcfg->mailcfg.sender.pPwd)<1))
        {
            LOGW("Not set Email param!\n");
            ovfs_print_json(pRoot);
            Common_Json_Delete(pRoot);
            return 0;
        }
        Common_Json_SetAttrValue(pRoot,-1,"Data/MailInterval",Common_Json_Type_Number,NULL,pLinkMail->bDefault?pLinkMail->interval:alarmcfg->mailcfg.interval,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
            ovfs_print_json(pRoot);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
    }
    return iRet;
}

int ovfs_linkRecord(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    OVFS_ABILITY * pAbility = ovfs_get_ability();
    S32 i = 0,j = 0,iRet = 0,iDevNum = 0,iChanNum = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL,*pData = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_LINK_RECORD_CFG *pLinkRecord = &pLinkageCfg->linkRecord;

    if(!pAbility)
    {
        LOGE("pAbility is null\n");
        return -1;
    }
    //iDevNum= pAbility->devNum;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(pAlarmEvent->iState)
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/Record/Functions/StartAlarmRecord",0,0);
            LOGW("StartAlarmRecord\n");
        }
        else
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/Record/Functions/StopAlarmRecord",0,0);
            LOGW("StopAlarmRecord\n");
        }

        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        pData= Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pData,-1,"LinkChannel",Common_Json_Type_Array,NULL,0,0);
        pChild = Common_Json_SetAttrValue(pArray,0,NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pChild,-1,"Device",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pChild,-1,"Channel",Common_Json_Type_Number,NULL,0,0);

        /*for(i = 0; i < iDevNum; i++)
        {
            OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
            if(!pDev)
                continue;
            iChanNum = pDev->chanNum;
            for(j = 0; j < iChanNum; j++)
            {
                if(pLinkRecord->mask[i]>>j)
                {
                    pChild = Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Object,NULL,i,0);
                    Common_Json_SetAttrValue(pChild,-1,"Device",Common_Json_Type_Number,NULL,i,0);
                    Common_Json_SetAttrValue(pChild,-1,"Channel",Common_Json_Type_Number,NULL,j,0);
                    iArrayCount++;
                }
            }
        }*/

        /*if(iArrayCount == 0)
        {
            LOGW("not set record channel!\n");
            Common_Json_Delete(pRoot);
            return 0;
        }*/

        int alarmType = pAlarmEvent->iType;
#if 1//def PLATFORM_JZT32
        alarmType = ALARM_TYPE_MOTION;
#else
        for(i=0; i<ARRAYSIZE(g_alarmIndex_smart); i++)
        {
            if(alarmType == g_alarmIndex_smart[i])
            {
                alarmType = ALARM_TYPE_DETECT_WIRE;
                break;
            }
        }

        if(alarmType == ALARM_TYPE_SENSOR_ALARM)
        {
            alarmType = ALARM_TYPE_MOTION;
        }
#endif
        Common_Json_SetAttrValue(pRoot,-1,"Data/Stream",Common_Json_Type_Number,NULL,pAlarmEvent->iStream,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/AlarmType",Common_Json_Type_Number,NULL,alarmType,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/AlarmDevice",Common_Json_Type_String,pAlarmEvent->sDevName,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/AlarmSrc",Common_Json_Type_Number,NULL,pAlarmEvent->iChan,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        //ovfs_print_json(pRoot);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
    }

    return iRet;
}


int ovfs_linkFtp(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 iRet = 0;
    S8 *pTemp = NULL,*pFileName = NULL;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;

	LOGD("Link FTP\n");

    if(0 != Common_StriCmp(pAlarmEvent->sName,(S8*)ALARM_STR_AI))
        oper_snap(hModuleHandle, pAlarmEvent->iDev, pAlarmEvent->iChan, pAlarmEvent, &pTemp);
    else
    {
        if(pAlarmEvent->iState == 0)
        {
            LOGW("alarm in is stop! no link ftp!\n");
            return -2;
        }
        oper_snap(hModuleHandle, 0, 0, pAlarmEvent, &pTemp);
    }
    if(!pTemp)
    {
        LOGE("snap failed\n");
        return -1;
    }
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(Common_File_IsExist("/root/lib/libnetwork_sdk.so"))
        {
            Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Webserver/Network/Functions/DoFtp");
        }
        else
        {
    	    Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Network/Functions/DoFtp");
    	}

        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/ClientFileName",Common_Json_Type_String,pTemp,0,0);
        pFileName = rindex(pTemp,'/');
        pFileName = pFileName+1;
        Common_Json_SetAttrValue(pRoot,-1,"Data/ServerFileName",Common_Json_Type_String,pFileName,0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        Common_Free(pTemp, __FUNCTION__, __LINE__);
    }
    return iRet;
}

int ovfs_linkSnap(ModuleHandle_T hModuleHandle,OVFS_LINK_SNAP_CFG *pLinkSnap,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 iCount = 0,iInterval = 0,iIndex = 0;
    S32 i = 0,j = 0,iRet = 0,iDevNum = 0,iChanNum = 0;
    U64 u64Tick1 = Common_GetSystemCount64();
    S64 s64CostMs = 0,s64Interval = 0;

    if(!pAlarmEvent->iState)
    {
        LOGD("%s iState:%d\n",pAlarmEvent->sName,pAlarmEvent->iState);
        return 0;
    }
	LOGD("Link SNAP\n");
    if(pLinkSnap->enable)
    {
        iCount= pLinkSnap->count;
        iInterval = pLinkSnap->interval;
        /*if(iCount <= 0)
            continue;*/
        iIndex = 0;
        while(iIndex < iCount)
        {
            u64Tick1 = Common_GetSystemCount64();
            oper_snap(hModuleHandle,0,0,pAlarmEvent,NULL);
            iIndex++;
            if(iIndex >= iCount)
                break;
            s64CostMs = Common_GetSystemCount64()-u64Tick1;
            s64Interval = iInterval*1000-s64CostMs;
            if(s64Interval > 0)
            {
                if(s64Interval > iInterval*1000)
                {
                    LOGD("iInterval %d[%lld]\n",iInterval,s64Interval);
                }
                else
                {
                    Common_Sleep(s64Interval/1000,(s64Interval%1000)*1000);
                }
            }
        }
        LOGD("Snap succ!\n");
    }
    return iRet;
}

int alarmout_status_list[3] = {0,0,0};

int ovfs_linkAlarmOut(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S8 strcmd[64] = {0};
    S32 yu = 0,chu = 0;
    S32 i = 0,iRet = 0,iAoutNum = 0,nlinkCount = 0;
    OVFS_ABILITY * pAbility = ovfs_get_ability();
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    OVFS_LINK_AOUT_CFG *pLinkAout = &pLinkageCfg->linkAlarmOut;
    if(!pAbility)
    {
        LOGE("pAbility is null\n");
        return -1;
    }
    iAoutNum= pAbility->alarmOutNum;

    //LOGE("iAoutNum=%d\n", iAoutNum);
    for(i = 0; i < iAoutNum; i++)
    {
        //chu = i/8,yu = i%8;
        //if(0 ==( pLinkAout->mask[chu]&(1<<yu)))
        //	continue;
        //nlinkCount = check_alarmLink(LINK_TYPE_AOUT, i);

        LOGW("i=%d nlinkCount=%d pAlarmEvent->iState=%d\n", i, nlinkCount, pAlarmEvent->iState);

        if(pLinkAout->mask[i] == 0)
        {
            continue;
        }


        memset(strcmd,0,sizeof(strcmd));

        if(pAlarmEvent->iState)
        {
            snprintf(strcmd, sizeof(strcmd), "/BoardSys/AlarmOut/Function/Channel%d/Enable",i);
            LOGW("Aout[%d] Enable![%d]\n", i,alarmout_status_list[i]);
            alarmout_status_list[i]++;

            if(alarmout_status_list[i]>1)
            {
                continue;
            }
        }
        else
        {
            snprintf(strcmd, sizeof(strcmd), "/BoardSys/AlarmOut/Function/Channel%d/Disable",i);
            LOGW("Aout[%d] Disable![%d]\n", i,alarmout_status_list[i]);
            alarmout_status_list[i]--;

            if(alarmout_status_list[i]>0)
            {
                continue;
            }
        }
    }

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        LOGW("alarmout call![%d]\n",iRet);
        if(iRet < 0)
        {
            LOGE("%d,err:%d\n",iRet,i);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return iRet;
}

map<char *,int> g_mapExpandAlarmOut;

int ovfs_linkExpandAlarmOut(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 iRet = 0;
    int need_set = 0;
    S8 strcmd[64] = {0};
    OVFS_LINK_EXPAND_AOUT_CFG *pLinkAout = &pLinkageCfg->linkExpandAlarmOut;
    cJSON_Struct *pRoot = NULL, *pList = NULL;

    snprintf(strcmd, sizeof(strcmd), "/Ptz/AlarmOut/Function/%s", pAlarmEvent->iState?"Enable":"Disable");

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValueObj(pRoot, "Data");
        pList = Common_Json_SetAttrValueArr(pRoot, "Data/List");
    }

    if(pAlarmEvent->iState)
    {
        memset(pAlarmEvent->sExpandAlarmOut, 0, sizeof(pAlarmEvent->sExpandAlarmOut));

        for(int i = 0; i < pLinkAout->iCount; i++)
        {
            if(pLinkAout->list[i].dev == 0 || pLinkAout->list[i].ch == 0)
            {
                continue;
            }

            cJSON_Struct *pTemp = Common_Json_SetAttrValueArrObj(pList, i);
            Common_Json_SetAttrValueInt(pTemp, "Dev", pLinkAout->list[i].dev);
            Common_Json_SetAttrValueInt(pTemp, "Ch", pLinkAout->list[i].ch);

            if(pAlarmEvent->iState)
            {
                snprintf(pAlarmEvent->sExpandAlarmOut[i], sizeof(pAlarmEvent->sExpandAlarmOut[i]), "%d-%d",
                    pLinkAout->list[i].dev, pLinkAout->list[i].ch);

                map<char *, int>::iterator it = g_mapExpandAlarmOut.find(pAlarmEvent->sExpandAlarmOut[i]);
                if(it != g_mapExpandAlarmOut.end())
                {
                    g_mapExpandAlarmOut[pAlarmEvent->sExpandAlarmOut[i]]++;
                }
                else
                {
                    g_mapExpandAlarmOut.insert(map<char *, int>::value_type (pAlarmEvent->sExpandAlarmOut[i], 1));
                    need_set = 1;
                }

                LOGD("[%s][%d]\n",pAlarmEvent->sExpandAlarmOut[i],g_mapExpandAlarmOut[pAlarmEvent->sExpandAlarmOut[i]]);
            }
        }
    }
    else
    {
        int dev = 0, ch = 0;
        for(int i = 0; i < 32; i++)
        {
            if(pAlarmEvent->sExpandAlarmOut[i] && strlen(pAlarmEvent->sExpandAlarmOut[i]) > 0)
            {
                map<char *, int>::iterator it = g_mapExpandAlarmOut.find(pAlarmEvent->sExpandAlarmOut[i]);
                if(it != g_mapExpandAlarmOut.end())
                {
                    g_mapExpandAlarmOut[pAlarmEvent->sExpandAlarmOut[i]]--;
                    LOGD("[%s][%d]\n",pAlarmEvent->sExpandAlarmOut[i],g_mapExpandAlarmOut[pAlarmEvent->sExpandAlarmOut[i]]);
                    if(g_mapExpandAlarmOut[pAlarmEvent->sExpandAlarmOut[i]] < 1)
                    {
                        sscanf(pAlarmEvent->sExpandAlarmOut[i], "%d-%d", &dev, &ch);
                        if(dev>0 && ch>0)
                        {
                            cJSON_Struct *pTemp = Common_Json_SetAttrValueArrObj(pList, i);
                            Common_Json_SetAttrValueInt(pTemp, "Dev", dev);
                            Common_Json_SetAttrValueInt(pTemp, "Ch", ch);
                            need_set = 1;
                        }

                        g_mapExpandAlarmOut.erase(it);
                    }
                }
            }
        }

    }

    LOGD("need_set:[%d]\n",need_set);
    if(need_set)
    {
        iRet = Module_CallFunctions(hModuleHandle, pRoot, NULL, 3000);
        LOGW("expand alarmout call![%d]\n",iRet);
    }
    if(iRet < 0)
    {
        LOGE("err:%d\n",iRet);
    }

    Common_Json_Delete(pRoot);
    pRoot = NULL;

    return iRet;
}

int ovfs_linkPtz(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 iRet = 0,iPtzCmd = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL,*pChild = NULL;
    OVFS_LINK_PTZ_CFG *pLinkPtz = &pLinkageCfg->linkPtz;

    if(!pAlarmEvent->iState)
        return 0;
    if(0 == pLinkPtz->type)			//????		return 0;
        return 0;
    else if(1 == pLinkPtz->type)		//?????
        iPtzCmd = 39;
    else if(2 == pLinkPtz->type)		//?????		iPtzCmd = 37;
        iPtzCmd = 37;
    else if(3 == pLinkPtz->type)		//?????		iPtzCmd = 36;
        iPtzCmd = 36;
    else
        return 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/Ptz/Cmd",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/Type",Common_Json_Type_Number,NULL,iPtzCmd,0);
        pChild = Common_Json_SetAttrValue(pRoot,-1,"Data/CmdParam",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pChild,-1,"Idx",Common_Json_Type_Number,NULL,pLinkPtz->index,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
    }
    return iRet;
}
#endif

int Action_SoundAlarm(ModuleHandle_T hModuleHandle,int index,char* path,int times,int isStart)
{
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);

        if(isStart)
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/BoardSys/Audio/Adec/PlayFile",0,0);
            LOGW("sound start\n");
        }
        else
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/BoardSys/Audio/Adec/StopPlayFile",0,0);
            LOGW("sound stop\n");
        }

        LOGI("link sound state=%d path=%s times=%d\n",isStart,(path==NULL)?"NULL":path,times);

        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);

        /*if(path)
        {
            if( (access(CUSTOM_AUDIO,F_OK) == 0) && index == 7)
	         Common_Json_SetAttrValue(pRoot,-1,"Data/Path",Common_Json_Type_String,CUSTOM_AUDIO,0,0);
	     else*/
	     {
                Common_Json_SetAttrValue(pRoot,-1,"Data/Path",Common_Json_Type_String,path,0,0);
	     }
        //}

        Common_Json_SetAttrValue(pRoot,-1,"Data/Times",Common_Json_Type_Number,NULL,times?times:86400,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
    }

    return 0;
}

int ovfs_linkSound(ModuleHandle_T hModuleHandle,OVFS_SOUND_CFG *pSoundCfg,OVFS_ALARM_EVENT *pAlarmEvent,int compulsiveState)
{
    S32 iRet = 0;
    char path[128];
    //cJSON_Struct *pResult = NULL,*pRoot = NULL;
    //OVFS_LINK_SOUND_CFG *pLinkSound = &pLinkageCfg->linkSound;
    //OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    //OVFS_SOUND_CFG* pSoundCfg = &(pAlarmCfg->soundCfg);

    OVFS_ALARM_BKBD* ovfs_get_alarm_bkbd();
    OVFS_ALARM_BKBD* bkbdInfo = ovfs_get_alarm_bkbd();
    ACTION_SOUND_STATE_T* soundState = &(bkbdInfo->actionState.soundAlarm);

    if(pAlarmEvent->iState && CheckIsInSchduleTime(-1,&(pSoundCfg->shedule)) == 0)
    {
        return 0;
    }

#ifdef AWSIOT
    if(pSoundCfg->audioSelected)
    {
        snprintf(path, sizeof(path), "/usr/etc/cfgfiles/custom_audio");
    }
    else
    {
        snprintf(path, sizeof(path), "/usr/etc/cfgfiles/default/custom_audio");
        if(!Common_File_IsExist(path))
        {
            snprintf(path, sizeof(path), "/update/soundFile/sound_alarm_default");
        }
    }
#else
    snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pSoundCfg->audioSelected);
#endif
    Action_SoundAlarm(hModuleHandle,pSoundCfg->audioSelected,path,pSoundCfg->playTimes,pAlarmEvent->iState);
    pAlarmEvent->iAudioState = pAlarmEvent->iState?pSoundCfg->audioSelected:-1;
    pAlarmEvent->iAudioTimes = pSoundCfg->playTimes;

    /*int realState = pAlarmEvent->iState;
    if(compulsiveState >= 0)
        realState = compulsiveState;

    if(realState && (!CheckIsInSchduleTime(-1,&(pSoundCfg->shedule))))
    {
        Common_Lock(bkbdInfo->actionLock);

        soundState->linkCnt++;
        soundState->curState = -1;
        soundState->lastTime = time(NULL);
        memset(soundState->path,0,sizeof(soundState->path));

        snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pSoundCfg->audioSelected);
        soundState->times = pSoundCfg->playTimes;

        Common_UnLock(bkbdInfo->actionLock);
        return 0;
    }

    int statTmp = soundState->curState;

    Common_Lock(bkbdInfo->actionLock);
    soundState->curState = realState;
    soundState->lastTime = time(NULL);
    if(realState)
        soundState->linkCnt++;
    else
        soundState->linkCnt--;

    if(soundState->linkCnt < 0)
        soundState->linkCnt = 0;

    Common_UnLock(bkbdInfo->actionLock);

    if( (realState == 0) && soundState->linkCnt)
    {
        LOGI("sound alarm cnt=%d return when stop sound alarm.\n",soundState->linkCnt);
        soundState->curState = statTmp;


        snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pSoundCfg->audioSelected);

	    soundState->linkIdx = pSoundCfg->audioSelected;
        if(pLinkSound->index_last != -1)
        {
            if(pLinkSound->index_last != pLinkSound->index)
            {
                //stop last index audio
                char url_last[128] = {0};
                snprintf(url_last, sizeof(url_last), "/update/soundFile/sound_%d", pLinkSound->index_last);
                Action_SoundAlarm(hModuleHandle,pLinkSound->index_last,url_last,0,0);
            }
        }
        pLinkSound->index_last = -1;
        Action_SoundAlarm(hModuleHandle,pSoundCfg->audioSelected,path,pSoundCfg->playTimes,0);

        return 0;
    }

    if(pLinkSound->index >= 0 && pLinkSound->index < (int)COMMON_ARRAY_ELEMENT_COUNT(pSoundCfg->soundName))
    {
        if(pSoundCfg->language == 1)
        {
            snprintf(path, sizeof(path), "/update/soundFile/soundEn_%d", pLinkSound->index);
        }
        else
        {
            snprintf(path, sizeof(path), "/update/soundFile/sound_%d", pLinkSound->index);
        }
	    soundState->linkIdx = pLinkSound->index;
        if(pLinkSound->index_last != -1)
        {
            if(pLinkSound->index_last != pLinkSound->index)
            {
                //stop last index audio
                char url_last[128] = {0};
                snprintf(url_last, sizeof(url_last), "/update/soundFile/sound_%d", pLinkSound->index_last);
                Action_SoundAlarm(hModuleHandle,pLinkSound->index_last,url_last,0,0);
            }
        }

        if(realState)
        {
            pLinkSound->index_last = pSoundCfg->audioSelected;
        }
        else
        {
            pLinkSound->index_last = -1;
        }

        Action_SoundAlarm(hModuleHandle,pSoundCfg->audioSelected,path,pSoundCfg->playTimes,realState);
    }*/
    return iRet;
}


int Action_LightAlarm(ModuleHandle_T hModuleHandle,int enable)
{
    S8 strcmd[64] = {0};

    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(enable)
        {
            sprintf(strcmd,"/BoardSys/Sys/LightAlarm/Enable");
            g_LightData.status++;
            LOGI("LightAlarm Enable![%d]\n",g_LightData.status);
        }
        else
        {
            sprintf(strcmd,"/BoardSys/Sys/LightAlarm/Disable");
            g_LightData.status--;
            LOGI("LightAlarm Disable![%d]\n",g_LightData.status);
        }
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        if((enable == 0 && g_LightData.status <= 0) || (enable && g_LightData.status > 0))
        {
            if(g_LightData.status < 0)
            {
                g_LightData.status = 0;
            }
            LOGI("Call!\n");
            iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        }
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return 0;
}

#ifndef SIMPLIFIED

int Action_SnapAlarm(ModuleHandle_T hModuleHandle,int streamIdx)
{
    if(g_FtpData.bUpload == 1)return 1;
    //int snapTime = 0;
    S8 strTmp[64] = {0};
    S8 *pTemp = NULL;
    S8 *pFileName = NULL;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        snprintf(strTmp, sizeof(strTmp), "/BoardSys/Snap/Device0/Channel0/Stream%d",streamIdx);
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strTmp,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);

        if(iRet == 0)
        {
            if(Common_Json_GetAttrValue(pResult, -1, "Header/Code", NULL, NULL, &iRet, NULL) == NULL)
            {
                iRet = -1;
            }
        }

        if(iRet == 0)
        {
            Common_Json_GetAttrValue(pResult, -1, "Data/Path", NULL, &pTemp, NULL, NULL);

            if(pTemp)
            {
                snprintf(strTmp, sizeof(strTmp), "cp %s /tmp/ftp_pic.jpg",pTemp);
                Common_System(strTmp);

                pFileName = rindex(pTemp,'/');
                pFileName = pFileName+1;

                char *result = strtok(pFileName, ".");

                snprintf(g_FtpData.FileName, sizeof(g_FtpData.FileName), "%s.jpg",result);

            }
        }

        Common_Json_Delete(pRoot);
        pRoot = NULL;
        Common_Json_Delete(pResult);
        pResult= NULL;
    }

    return iRet;
}

S32 Check_Ftp_Status()
{
    int ret = 0;
    int status = -1;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(Common_File_IsExist("/root/lib/libnetwork_sdk.so"))
        {
            Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Webserver/Network/status/ftp");
        }
        else
        {
    	    Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Network/status/ftp");
    	}
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
    }

    ret = Module_CallFunctions(g_SnapData.hModuleHandle, pRoot, &pResult, 3000);
    if(ret == 0)
    {
        Common_Json_GetAttrValueInt(pResult, "Data/Status", &status);
        LOGD("status:[%d]\n",status);
    }

    Common_Json_Delete(pRoot);
    pRoot = NULL;

    Common_Json_Delete(pResult);
    pResult = NULL;

    return status;
}

S32 Start_Ftp_Upload()
{
    int ret = 0;
    int status = -1;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(Common_File_IsExist("/root/lib/libnetwork_sdk.so"))
        {
            Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Webserver/Network/Functions/DoFtp");
        }
        else
        {
    	    Common_Json_SetAttrValueStr(pRoot, "Header/Uri", "/Network/Functions/DoFtp");
    	}

        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/ClientFileName",Common_Json_Type_String,"/tmp/ftp_pic.jpg",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data/ServerFileName",Common_Json_Type_String,g_FtpData.FileName,0,0);
    }

    ret = Module_CallFunctions(g_SnapData.hModuleHandle, pRoot, &pResult, 3000);

    Common_Json_Delete(pRoot);
    pRoot = NULL;

    Common_Json_Delete(pResult);
    pResult = NULL;

    return ret;
}


S32 Thread_FtpUpload(Common_Thread_T hThreadHandle,void *pUserData)
{
    int ret = 0;
    while(1)
    {
        if(Common_File_IsExist("/tmp/ftp_pic.jpg"))
        {
            if(Check_Ftp_Status() == 0)
            {
                LOGD("g_FtpData.bUpload:[%d]\n",g_FtpData.bUpload);
                if(g_FtpData.bUpload == 1)
                {
                    g_FtpData.bUpload = 0;
                    Common_System("rm -r /tmp/ftp_pic.jpg");
                }
                else
                {
                    g_FtpData.bUpload = 1;
                    Start_Ftp_Upload();
                }
            }
        }

        Common_Sleep(1, 0);
    }

    return 0;
}

#endif

int Action_LightAlarmv2(ModuleHandle_T hModuleHandle,int enable)
{
    S8 strcmd[64] = {0};

    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(enable)
        {
            sprintf(strcmd,"/BoardSys/Sys/LightAlarmv2/Enable");
            LOGI("LightAlarmv2 Enable!\n");
        }
        else
        {
            sprintf(strcmd,"/BoardSys/Sys/LightAlarmv2/Disable");
            LOGI("LightAlarmv2 Disable!\n");
        }
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return 0;
}

int ovfs_linkLightAlarm(ModuleHandle_T hModuleHandle,OVFS_LIGHT_ALARM_CFG *pLightCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    LOGD("[%s][%d][%d]\n",pAlarmEvent->sName,pAlarmEvent->iState,pAlarmEvent->iLightState);
    S8 strcmd[64] = {0};
    S32 iRet = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;

    int bArming = CheckIsInSchduleTime(-1,&(pLightCfg->schTime));

    if(pAlarmEvent->iState)
    {
        if(pLightCfg->enable == 0)
        {
            if(pAlarmEvent->iLightState)
            {
                pAlarmEvent->iLightState = 0;
                Action_LightAlarm(hModuleHandle, 0);
            }
        }
        else
        {
            if(pAlarmEvent->iState && bArming)
            {
                if(pAlarmEvent->iLightState == 0)
                {
                    pAlarmEvent->iLightState = 1;
                    Action_LightAlarm(hModuleHandle, 1);
                }
            }
        }
    }
    else
    {
        if(pAlarmEvent->iLightState)
        {
            pAlarmEvent->iLightState = 0;
            Action_LightAlarm(hModuleHandle, 0);
        }
    }

    /*if(pAlarmEvent->iState && bArming && pLightCfg->enable)
    {
        g_LightData.status++;
        LOGD("++[%d]\n",g_LightData.status);
        *//*if(g_LightData.status)
        {
            if(pLightCfg->duration>g_LightData.remainTime)
            {
                g_LightData.remainTime = pLightCfg->duration;
            }
        }
        else
        {
            g_LightData.status = 1;
            g_LightData.remainTime = pLightCfg->duration;
        }*//*
    }
    else
    {
        g_LightData.status--;
        LOGD("--[%d]\n",g_LightData.status);
        if(pLightCfg->duration>g_LightData.remainTime)
        {
            g_LightData.remainTime = pLightCfg->duration;
        }
        if(g_LightData.status<0)
        {
            g_LightData.status = 0;

        }
    }*/

    /*pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(pAlarmEvent->iState && 1 == bArming)
        {
            sprintf(strcmd,"/BoardSys/Sys/LightAlarm/Enable");
            LOGW("LightAlarm Enable!\n");
        }
        else
        {
            sprintf(strcmd,"/BoardSys/Sys/LightAlarm/Disable");
            LOGW("LightAlarm Disable!\n");
        }
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }*/
    return iRet;
}
/*
int Action_RedBlueLight(ModuleHandle_T hModuleHandle,int enable)
{
    S8 strcmd[64] = {0};

    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(enable)
        {
            sprintf(strcmd,"/BoardSys/Sys/RedBlueLight/Enable");
            LOGI("LightAlarm Enable!\n");
        }
        else
        {
            sprintf(strcmd,"/BoardSys/Sys/RedBlueLight/Disable");
            LOGI("LightAlarm Disable!\n");
        }
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return 0;
}

int ovfs_linkRedBlueLight(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S8 strcmd[64] = {0};
    S32 iRet = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;

    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
    OVFS_LIGHT_ALARM_CFG *pRedblueCfg = &pAlarmCfg->redbluelightCfg;
    int bArming = 0;
    Common_Time_T t_happent;
    Common_Linux2CommonTime(time(NULL),&t_happent);

    U32 t_start = t_happent.hour*100+t_happent.min;
    int i = 0,j = 0;

    i = t_happent.wday;
    for(j = 0; j < MAX_TIMESEGMENT; j++)
    {
        if((t_start >= pRedblueCfg->schTime.depSchedTime[i][j].startTime) &&(t_start <= pRedblueCfg->schTime.depSchedTime[i][j].stopTime))
        {
            bArming = 1;
        }
    }

    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        if(pAlarmEvent->iState && 1 == bArming)
        {
            sprintf(strcmd,"/BoardSys/Sys/RedBlueLight/Enable");
            LOGW("LightAlarm Enable!\n");
        }
        else
        {
            sprintf(strcmd,"/BoardSys/Sys/RedBlueLight/Disable");
            LOGW("LightAlarm Disable!\n");
        }
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet < 0)
        {
            LOGE("err:%d\n",iRet);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return iRet;
}
*/
int ovfs_clearAlarmLink(ModuleHandle_T hModuleHandle,S32 iAlarmType,S32 iAlarmSrc,OVFS_LINKAGE_CFG *pLinkageCfg,S32 iLinkType,S32 bEnable, S32 index)
{
    S8 strcmd[64] = {0};
    OVFS_ABILITY * pAbility = ovfs_get_ability();
    S32 iAlarmStatus = 0,iLinkCount = 0, iAlarmSrcIndex = -1;
    //S32 chu = 0,yu = 0,iAoutNum = 0;
    S32 i = 0,j = 0,iRet = 0,iDevNum = 0,iChanNum = 0,iArrayCount = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL,*pData = NULL,*pArray = NULL,*pChild = NULL;
    OVFS_ALARM_BKBD *pAlarmBkbd = ovfs_get_alarm_bkbd();
    OVFS_ALARM_EVENT *pTempEvent = NULL,*pAlarmEvent = NULL;
	for(i = 0; i < pAlarmBkbd->alarmCount; i ++)
	{
		if(iAlarmType == g_alarmIndex[i])
		{
			iAlarmSrcIndex = i;
			break;
		}
	}

    if(-1 == iAlarmSrc || iAlarmSrcIndex < 0)
    {
        LOGW("iAlarmSrc:%d, iAlarmType:%d\n",iAlarmSrc, iAlarmType);
        return -1;
    }
    if(!pAbility)
    {
        LOGW("pAbility is null\n");
        return -1;
    }
    if(!pAlarmBkbd)
    {
        LOGW("pAlarmBkbd is null\n");
        return 0;
    }
    pTempEvent =  pAlarmBkbd->pAlarmEvent[iAlarmSrcIndex];
    if(!pTempEvent)
        return 0;

    pAlarmEvent = &pTempEvent[iAlarmSrc];
    iAlarmStatus = pAlarmEvent->iState;
    iDevNum= 0;//pAbility->devNum;
    //iAoutNum= pAbility->alarmOutNum;
    if(LINK_TYPE_MAIL == iLinkType)
    {
#if 1
        if(iAlarmStatus && bEnable)
            ovfs_linkMail(hModuleHandle,pLinkageCfg,pAlarmEvent);
#endif
    }
    else if(LINK_TYPE_SNAP == iLinkType)
    {
#if 1
        if(iAlarmStatus && bEnable)
            ovfs_linkSnap(hModuleHandle,&pLinkageCfg->linkSnap,pAlarmEvent);
#endif
    }
    else if(LINK_TYPE_PTZ == iLinkType)
    {
#if 1
        if(iAlarmStatus && bEnable)
            ovfs_linkPtz(hModuleHandle,pLinkageCfg,pAlarmEvent);
#endif
    }
    else if(LINK_TYPE_RECORD == iLinkType)
    {
        OVFS_LINK_RECORD_CFG *pLinkRecord = &pLinkageCfg->linkRecord;
        pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        if (pRoot)
        {
            Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
            if(bEnable && iAlarmStatus)
            {
                Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/Record/Functions/StartAlarmRecord",0,0);
                LOGW("StartAlarmRecord\n");
            }
            else
            {
                Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/Record/Functions/StopAlarmRecord",0,0);
                LOGW("StopAlarmRecord\n");
            }
            Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
            pData= Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
            pArray = Common_Json_SetAttrValue(pData,-1,"LinkChannel",Common_Json_Type_Array,NULL,0,0);
            for(i = 0; i < iDevNum; i++)
            {
                /*OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
                if(!pDev)
                    continue;*/
                iChanNum = 0;//pDev->chanNum;
                for(j = 0; j < iChanNum; j++)
                {
                    if(pLinkRecord->mask[i]>>j)
                    {
                        pChild = Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Object,NULL,i,0);
                        Common_Json_SetAttrValue(pChild,-1,"Device",Common_Json_Type_Number,NULL,i,0);
                        Common_Json_SetAttrValue(pChild,-1,"Channel",Common_Json_Type_Number,NULL,j,0);
                        iArrayCount++;
                    }
                }
            }
            if(iArrayCount == 0)
            {
                LOGW("not set record channel!\n");
                Common_Json_Delete(pRoot);
                return -1;
            }

            Common_Json_SetAttrValue(pRoot,-1,"Data/AlarmType",Common_Json_Type_Number,NULL,iAlarmType,0);
            Common_Json_SetAttrValue(pRoot,-1,"Data/AlarmSrc",Common_Json_Type_Number,NULL,iAlarmSrc,0);
            //ovfs_print_json(pRoot);
            iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
            if(iRet < 0)
            {
                LOGE("err:%d\n",iRet);
            }
            Common_Json_Delete(pRoot);
            Common_Json_Delete(pResult);
            pRoot = NULL;
            pResult = NULL;
        }
        return 0;
    }

    else if(LINK_TYPE_AOUT == iLinkType)
    {
        iLinkCount = check_alarmLink(iLinkType, index);
        if(iLinkCount >= 0)
        {
            //LOGW("######bEnable:%d, iLinkCount:%d#####\n",bEnable, iLinkCount);

            //OVFS_LINK_AOUT_CFG *pLinkAout = &pLinkageCfg->linkAout;
            memset(strcmd,0,sizeof(strcmd));
            pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

            if (pRoot)
            {
                Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);

                if (bEnable && iAlarmStatus)
                {
                    sprintf(strcmd,"/BoardSys/AlarmOut/Function/Channel%d/Enable?Delay=32140800",index);
                    LOGW("Aout Enable\n");
                }
                else if(bEnable == 0)
                {
                    sprintf(strcmd,"/BoardSys/AlarmOut/Function/Channel%d/Disable?Delay=-1",index);
                    LOGW("Aout Disable\n");
                }

                Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
                Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);

                iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
                if(iRet < 0)
                {
                    char* outP = Common_Json_Print(pRoot,NULL);

                    LOGE("%d,err:%d bEnable=%d iAlarmStatus=%d inJson=%s \n",iRet,i,bEnable,iAlarmStatus,outP);
                    if(outP)
                        free(outP);
                }

                Common_Json_Delete(pRoot);
                Common_Json_Delete(pResult);
                pRoot = NULL;
                pResult = NULL;

            }
        }
    }
    else if(LINK_TYPE_LIGHTALARM == iLinkType)
    {
        iLinkCount = check_alarmLink(iLinkType, index);
        if(iLinkCount >= 0)
        {
            LOGW("######bEnable:%d, iLinkCount:%d,iAlarmStatus:%d  #####\n",bEnable, iLinkCount,iAlarmStatus);


            //OVFS_LINK_LIGHTALARM_CFG *pLinkLightAlarm = &pLinkageCfg->linkLightAlarm;
            memset(strcmd,0,sizeof(strcmd));
            pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

            if (pRoot)
            {
                Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);

                if(bEnable && iAlarmStatus)
                {
                    sprintf(strcmd,"/BoardSys/Sys/LightAlarm/Enable");
                    LOGW("LightAlarm Enable!\n");
                }
                else if(bEnable == 0)
                {
                    sprintf(strcmd,"/BoardSys/Sys/LightAlarm/Disable");
                    LOGW("LightAlarm Disable!\n");
                }

                Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
                Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
                iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);

                if(iRet < 0)
                {
                    LOGE("/BoardSys/Sys/LightAlarm %d,err:%d\n",iRet,i);
                }

                Common_Json_Delete(pRoot);
                Common_Json_Delete(pResult);
                pRoot = NULL;
                pResult = NULL;
            }

        }
    }
    else if(LINK_TYPE_REDBLUELIGHT == iLinkType)
    {
        iLinkCount = check_alarmLink(iLinkType, index);
        if(iLinkCount >= 0)
        {
            LOGW("######bEnable:%d, iLinkCount:%d,iAlarmStatus:%d  #####\n",bEnable, iLinkCount,iAlarmStatus);


            //OVFS_LINK_LIGHTALARM_CFG *pLinkLightAlarm = &pLinkageCfg->linkLightAlarm;
            memset(strcmd,0,sizeof(strcmd));
            pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

            if (pRoot)
            {
                Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);

                if(bEnable && iAlarmStatus)
                {
                    sprintf(strcmd,"/BoardSys/Sys/RedBlueLight/Enable");
                    LOGW("RedblueLight Enable!\n");
                }
                else if(bEnable == 0)
                {
                    sprintf(strcmd,"/BoardSys/Sys/RedBlueLight/Disable");
                    LOGW("RedblueLight Disable!\n");
                }

                Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
                Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
                iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);

                if(iRet < 0)
                {
                    LOGE("Module_CallFunctions ret:%d,err:%d\n",iRet,i);
                }

                Common_Json_Delete(pRoot);
                Common_Json_Delete(pResult);
                pRoot = NULL;
                pResult = NULL;
            }

        }
    }
    else if(LINK_TYPE_SOUND == iLinkType)
    {
        iLinkCount = check_alarmLink(iLinkType, index);
        if(iLinkCount >= 0)
        {
            if(bEnable && iAlarmStatus)
            {
                //ovfs_linkSound(hModuleHandle,pLinkageCfg,pAlarmEvent,1);
            }
            else if(bEnable == 0)
            {
                //ovfs_linkSound(hModuleHandle,pLinkageCfg,pAlarmEvent,0);
            }
        }
    }
    else if(LINK_TYPE_FTP == iLinkType)
    {
        if(iAlarmStatus && bEnable)
            ovfs_linkFtp(hModuleHandle,pLinkageCfg,pAlarmEvent);
    }

    return 0;
}

typedef struct
{
    int alarmType;
    int curTime;
    int startTime;
    int stopTime;
    int status;
    char picPath[32];
    int pushResult;
}ALARMPUSH_CFG;

char g_sn[32] = {0};
map<int, ALARMPUSH_CFG> g_alarmMap;
Common_Lock_T g_alarmPushLock;
ModuleHandle_T g_moduleHandle = NULL;

#ifndef SIMPLIFIED
int oper_saveSnap(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent,S8 *szPath,S32 snapTime)
{
    S32 iRet = -1;
    cJSON_Struct *pInData = NULL,*pArray = NULL,*pChild = NULL,*pOutData = NULL;

    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pInData != NULL)
    {
        Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/FileManage/OperateFile",0,0);
        Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        pChild = Common_Json_SetAttrValue(pInData,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

        Common_Json_SetAttrValue(pArray,0,"MainType",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pArray,0,"SubType",Common_Json_Type_Number,NULL,pAlarmEvent->iType,0);
        Common_Json_SetAttrValue(pArray,0,"DeviceNo",Common_Json_Type_Number,NULL,pAlarmEvent->iDev,0);
        Common_Json_SetAttrValue(pArray,0,"ChannelNo",Common_Json_Type_Number,NULL,pAlarmEvent->iChan,0);
        Common_Json_SetAttrValue(pArray,0,"StreamNo",Common_Json_Type_Number,NULL,pAlarmEvent->iStream,0);
        Common_Json_SetAttrValue(pArray,0,"FilePath",Common_Json_Type_String,szPath,0,0);
        Common_Json_SetAttrValue(pArray,0,"CreateTime",Common_Json_Type_Number,NULL,snapTime,0);
		LOGD("pAlarmEvent->iType=%d\n",pAlarmEvent->iType);
        iRet = Module_CallFunctions(hModuleHandle,pInData,&pOutData,10000);
        if(iRet!=0)
        {
            LOGW("iRet : %d\n",iRet);
            ovfs_print_json(pInData);
        }

        Common_Json_Delete(pInData);
        Common_Json_Delete(pOutData);
        //?????????,????????		//notifyReleaseFile(hModuleHandle,szPath);
    }
    return iRet;
}

//using namespace ants_soft;
int oper_snap(ModuleHandle_T hModuleHandle,int iDev,int iChan,OVFS_ALARM_EVENT *pAlarmEvent,S8 **pOutPath)
{
    S32 iRet = -1;
    S32 snapTime = -1;
    S8 strCmd[128] = {0};
    S8 *pTemp = NULL;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    OVFS_ABILITY * pAbility = ovfs_get_ability();

    if(!pAbility)
    {
        LOGE("pAbility is null\n");
        return -1;
    }
#if 0
    if(0 == Common_File_IsExist(pAbility->snapPath))
    {
        iRet = get_mount_path(hModuleHandle, pAbility);
        if(iRet < 0)
        {
            LOGE("get snap path failed,iRet:%d\n",iRet);
            return -1;
        }
        if(0 == Common_File_IsExist(pAbility->snapPath))
        {
            LOGW("path[%s] is not exist!\n",pAbility->snapPath);
            return -1;
        }
    }
#endif
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        snprintf(strCmd, sizeof(strCmd), "/BoardSys/Snap/Device%d/Channel%d/Stream0",iDev,iChan);
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,strCmd,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet != 0)
        {
            LOGE("error!iRet:%d,iDev %d,iChan %d\n",iRet,iDev,iChan);
            Common_Json_Delete(pRoot);
            Common_Json_Delete(pResult);
            return -1;
        }
        Common_Json_Delete(pRoot);
        Common_Json_GetAttrValue(pResult, -1, "Data/Path", NULL, &pTemp, NULL, NULL);
        Common_Json_GetAttrValue(pResult, -1, "Data/TimeStamp", NULL, NULL, &snapTime, NULL);
        if(snapTime < 0)
        {
            ovfs_print_json(pResult);
            Common_Json_Delete(pResult);
            return -1;
        }
        do
        {
            if(!pTemp)
            {
                ovfs_print_json(pResult);
                break;
            }
            snprintf(strCmd, sizeof(strCmd), "%s",pTemp);
            if(pOutPath)
            {
                LOGD("%s\n",strCmd);
                *pOutPath = (S8*)Common_Malloc(strlen(strCmd)+1, 0, __FUNCTION__, __LINE__);
                Common_Strcpy(*pOutPath, strCmd);
            }
            else
            {
#if 0
                S8 szFname[128] = {0};
                Common_Time_T snapTime;
                S8 *pFileName = NULL,*pSuffix = NULL;
                pFileName = FindLast(pTemp, '/');
                if(!pFileName)
                {
                    ovfs_print_json(pResult);
                    break;
                }
                pFileName = pFileName+1;
                pSuffix = FindLast(pTemp, '.');
                Common_Linux2CommonTime(time(NULL),&snapTime);
                snprintf(szFname, sizeof(szFname),"dev%d_ch%d_%04d%02d%02d%02d%02d%02d%s",iDev,iChan,snapTime.year,snapTime.month,snapTime.day,snapTime.hour,snapTime.min,snapTime.sec,pSuffix?pSuffix:"");
                snprintf(strcmd, sizeof(strcmd), "mv %s %s/%s",pTemp,pAbility->snapPath,szFname);
                LOGD("%s\n",strCmd);
                Common_System(strCmd);
                snprintf(strcmd, sizeof(strcmd), "%s/%s",pAbility->snapPath,szFname);
#else
                //oper_saveSnap(hModuleHandle,pAlarmEvent,pTemp,snapTime);
#endif
            }
        }
        while(0);
        Common_Json_Delete(pResult);
    }
    return 0;
}


S32 Thread_LinkSnap(Common_Thread_T hThreadHandle,void *pUserData)
{
    prctl(PR_SET_NAME,__func__);
    while(1)
    {
        if(g_SnapData.bSnap)
        {
            OVFS_ALARM_EVENT *pAlarmEvent = g_SnapData.pAlarmEvent;
            OVFS_LINK_SNAP_CFG *pSnapCfg = &g_SnapData.snapCfg;
            //ModuleHandle_T hModuleHandle = g_SnapData.hModuleHandle;
            ovfs_linkSnap(g_SnapData.hModuleHandle,pSnapCfg, pAlarmEvent);
            if(pAlarmEvent)
            {
                if(pAlarmEvent->sName)
                    Common_Free(pAlarmEvent->sName, __FUNCTION__, __LINE__);
                if(pAlarmEvent->sDevName)
                    Common_Free(pAlarmEvent->sDevName, __FUNCTION__, __LINE__);
                Common_Free(pAlarmEvent, __FUNCTION__, __LINE__);
                g_SnapData.pAlarmEvent = NULL;
            }
            g_SnapData.bSnap = 0;
        }
        Common_Sleep(0, 100*1000);
    }
    return 0;
}

S32 Thread_LinkLight(Common_Thread_T hThreadHandle,void *pUserData)
{
    prctl(PR_SET_NAME,__func__);
    int cur_status = 0;
    int status = 0;
    //int count = 0;
    while(1)
    {
        /*count++;
        if(count>3)
        {
            LOGI("LightAlarm status[%d]\n",g_LightData.status);
            count = 0;
        }*/
        status = g_LightData.status>0 ? 1 : 0;
        if(status)
        {
            if(cur_status != status)
            {
                cur_status = status;
                Action_LightAlarm(g_LightData.hModuleHandle, cur_status);
            }
        }
        else
        {
            if(cur_status)
            {
                if(g_LightData.remainTime>0)
                {
                    g_LightData.remainTime--;
                }
                else
                {
                    cur_status = 0;
                    Action_LightAlarm(g_LightData.hModuleHandle, 0);
                }
            }
        }

        /*if(cur_status != status)
        {
            cur_status = status;
            if(status)
            {
                Action_LightAlarm(g_LightData.hModuleHandle, cur_status);
            }
            else
            {

            }
        }
        else
        {
            if(cur_status)
            {
                if(g_LightData.remainTime>0)
                {
                    g_LightData.remainTime--;
                }
                else
                {
                    g_LightData.status = !g_LightData.status;
                    LOGW("g_LightData.status:[%d] remainTime:[%d]\n",g_LightData.status,g_LightData.remainTime);
                    continue;
                }
            }
        }*/

        Common_Sleep(1, 0);
    }
    return 0;
}

int ovfs_initLink(ModuleHandle_T hModuleHandle)
{
    Common_Thread_T t_Thread = NULL;
    Common_Thread_Create(&t_Thread,__FUNCTION__,0,0,Thread_LinkSnap,NULL);
    memset(&g_SnapData,0,sizeof(SNAP_DATA));
    g_SnapData.hModuleHandle = hModuleHandle;
    g_SnapData.bSnap = 0;

    //Common_Thread_T t_Thread_light = NULL;
    //Common_Thread_Create(&t_Thread_light,__FUNCTION__,0,0,Thread_LinkLight,NULL);
    memset(&g_LightData,0,sizeof(LIGHT_DATA));
    g_LightData.hModuleHandle = hModuleHandle;

#ifndef SIMPLIFIED

    Common_Thread_T t_Thread_ftp = NULL;
    Common_Thread_Create(&t_Thread_ftp,__FUNCTION__,0,0,Thread_FtpUpload,NULL);
    memset(&g_FtpData,0,sizeof(FTP_DATA));
    g_FtpData.hModuleHandle = hModuleHandle;

#endif

    Common_Lock_Create(&g_alarmPushLock,NULL);
    Common_Thread_T t_Thread_push = NULL;
    Common_Thread_Create(&t_Thread_push,__FUNCTION__,0,0,Thread_AlarmPush,NULL);


    return 0;
}

#endif

int get_sn(ModuleHandle_T hModuleHandle)
{
    char *pStr = 0;
    cJSON_Struct *pConfig,*pOutParams = NULL;
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Core/Version",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
    }
    Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
    Common_Json_Delete(pConfig);
    if(!pOutParams)
    {
        LOGE("pOutParams is null!\n");
        return 0;
    }
    Common_Json_GetAttrValueStr(pOutParams, "Data/SerialNumber", &pStr);
    if(pStr)
    {
        snprintf(g_sn, sizeof(g_sn), "%s", pStr);
    }
    Common_Json_Delete(pOutParams);
    return 0;
}

static size_t HttpWriteCallBack(void *buffer, size_t size, size_t count, void *response)
{

    char *ptr = NULL;
    ptr = (char *) Common_Malloc(count * size + 4,sizeof(int),__func__,__LINE__);
    memset(ptr, 0, count * size + 4);
    memcpy(ptr, buffer, count * size);
    void **p = (void **) response;
    *p = ptr;

    return count;
}

static int ResponseLog(CURL *curl, curl_infotype type, char *buffer, int size, void *user)
{
    if (access("/tmp/alarm_debug", F_OK) == 0)
        //printf("[HTTP] %s\n", buffer);
    return size;
}

int push_alarm(OVFS_PUSH_CFG pushcfg, ALARMPUSH_CFG alarmcfg)
{
    CURL * curl;
    CURLcode res;
    int ret = -1;
    char *contentStr = NULL;
    char *imageBase64 = NULL;
    char *response = NULL;
    cJSON_Struct *contentJson = NULL;
    cJSON_Struct *outJson = NULL;

    contentJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValueStr(contentJson, "Sn", g_sn);
    Common_Json_SetAttrValueInt(contentJson, "Ch", 1);
    Common_Json_SetAttrValueInt(contentJson, "AlarmType", alarmcfg.alarmType);
    Common_Json_SetAttrValueInt(contentJson, "CurrentTime", alarmcfg.curTime);
    Common_Json_SetAttrValueInt(contentJson, "StartTime", alarmcfg.startTime);
    Common_Json_SetAttrValueInt(contentJson, "StopTime", alarmcfg.status?0:alarmcfg.stopTime);
    Common_Json_SetAttrValueInt(contentJson, "Status", alarmcfg.status);
    if(pushcfg.withImage && alarmcfg.status)
    {
        char *fileBuffer = NULL;
        FILE *fp = Common_File_fOpen(alarmcfg.picPath, "rb");
        if(fp != NULL)
        {
            Common_File_fSeek(fp,0,SEEK_END);
            int fileSize = Common_File_fTell(fp);
            LOGW("fileSize:[%d]\n",fileSize);
            Common_File_fSeek(fp,0,SEEK_SET);

            fileBuffer = (char *)malloc(fileSize+1);
        	memset(fileBuffer,0,(fileSize+1));
        	Common_File_fRead(fileBuffer,fileSize,1,fp);

        	U32 len_encode = 0;
        	imageBase64 = Common_Base64_Encode(fileBuffer,fileSize,&len_encode);

        	free(fileBuffer);
        	fileBuffer = NULL;

        	fclose(fp);
        }
        else
        {
            LOGE("fOpen error\n");
        }

        Common_Json_SetAttrValueStr(contentJson, "ImageBase64", imageBase64?imageBase64:"");
    }

    contentStr = Common_Json_PrintUnformatted(contentJson, NULL);
    if(access("/tmp/alarm_debug", F_OK) == 0)
    {
        LOGD("contentStr:[%s]\n",contentStr);
    }

    Common_Json_Delete(contentJson);
    contentJson = NULL;

    if(imageBase64)
    {
        Common_Free(imageBase64,__FUNCTION__,__LINE__);
	    imageBase64 = NULL;
    }

    curl = curl_easy_init();

    if (curl == NULL)
    {
        LOGE("curl init failed\n");
        return -1;
    }

    struct curl_slist *headers = NULL;
    char tmp[128] = { 0 };

    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
    headers = curl_slist_append(headers, "User-Agent: Mozilla/4.0");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");

    // snprintf(tmp,sizeof(tmp),"%s:%d",ct->serverAddr,ct->serverPort);
    headers = curl_slist_append(headers, tmp);

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_URL, pushcfg.url);

    if (strstr(pushcfg.url, "https:") != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    }

//	LOGD("tmp url [%s]\n", tmp);
    // curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, contentStr);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpWriteCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )&response);
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1L);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/dev/null");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, ResponseLog);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        LOGE("curl_wasy_perform error = %s\n", curl_easy_strerror(res));
        ret = -1;
    }
    else
    {
        if(access("/tmp/alarm_debug", F_OK) == 0)
        {
            LOGW("response:[%s]\n",response);
        }
        ret = 1;
        /*contentJson = Common_Json_Parse(response, NULL, NULL);
        int status = 0;
        Common_Json_GetAttrValueInt(contentJson, "status", &status);

        ret = status == 200 ? 0 : -1;

        Common_Json_Delete(contentJson);*/
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    Common_Free(contentStr,__func__,__LINE__);
    Common_Free(response,__func__,__LINE__);

    return ret;

}

S32 Thread_AlarmPush(Common_Thread_T hThreadHandle,void *pUserData)
{
    prctl(PR_SET_NAME,__func__);
    int cur_status = 0;
    int status = 0;
    char cmd[128] = {0};
    //int count = 0;
    time_t lastTime = 0;
    while(1)
    {
        OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();

        if (pAlarmCfg->pushCfg.enable == 1 && strlen(pAlarmCfg->pushCfg.url) > 0)
        {
            int nSize = g_alarmMap.size();
            for(int nindex = 1; nindex <= 9; nindex++)
            {
                Common_Lock(g_alarmPushLock);
                if(g_alarmMap.count(nindex)==0)
                {
                    Common_UnLock(g_alarmPushLock);
                    continue;
                }
                ALARMPUSH_CFG alarmcfg = g_alarmMap[nindex];

                //LOGW("g_alarmMap[%d] alarmType:[%d] iState:[%d]\n",nindex, alarmcfg.alarmType,alarmcfg.status);
                Common_UnLock(g_alarmPushLock);

                time_t cutT = time(NULL);

                if((alarmcfg.status && ( cutT - alarmcfg.curTime >= pAlarmCfg->pushCfg.interval)) || alarmcfg.status == 0)
                {
                    if(alarmcfg.curTime != 0 && pAlarmCfg->pushCfg.withImage && pAlarmCfg->pushCfg.updateImage)
                    {
                        LOGD("update snap!\n");
                        alarmcfg.pushResult = 0;
                        alarmcfg.startTime = cutT;
                        S8 *picPath = NULL;

                        oper_snap(g_moduleHandle, 0, 0, NULL, &picPath);
                        if(picPath)
                        {
                            snprintf(alarmcfg.picPath, sizeof(alarmcfg.picPath),"/dev/push_%d.jpg",alarmcfg.alarmType);
                            snprintf(cmd, sizeof(cmd),"mv %s %s",picPath,alarmcfg.picPath);
                            Common_System(cmd);
                            Common_Free(picPath, __FUNCTION__, __LINE__);
                            picPath = NULL;
                        }
                        else
                        {
                            LOGE("snap failed\n");
                        }
                    }

                    alarmcfg.curTime = cutT;
                    alarmcfg.pushResult = push_alarm(pAlarmCfg->pushCfg, alarmcfg);

                    if(alarmcfg.status == 0)
                    {
                        Common_Lock(g_alarmPushLock);
                        snprintf(cmd, sizeof(cmd),"rm -r %s",alarmcfg.picPath);
                        Common_System(cmd);
                        g_alarmMap.erase(alarmcfg.alarmType);
                        Common_UnLock(g_alarmPushLock);
                    }
                    else
                    {
                        Common_Lock(g_alarmPushLock);
                        if(g_alarmMap[alarmcfg.alarmType].status != alarmcfg.status)
                        {
                            alarmcfg.status = g_alarmMap[alarmcfg.alarmType].status;
                            alarmcfg.curTime = 0;
                        }
                        g_alarmMap[alarmcfg.alarmType] = alarmcfg;
                        Common_UnLock(g_alarmPushLock);
                    }
                }
                else
                {
                }
            }
        }
        else
        {
            if(g_alarmMap.size()>0)
            {
                g_alarmMap.clear();
                Common_System("rm -r /dev/push_*.jpg");
            }
        }

        Common_Sleep(0, 500 * 1000);
    }
    return 0;
}

int get_pushType(int type)
{
    int ret = 0;
    if(type == ALARM_TYPE_MOTION){ret = 1;}
    else if(type == ALARM_TYPE_VHIDE){ret = 2;}
    else if(type == ALARM_TYPE_REGIONAL_INVASION){ret = 3;}
    else if(type == ALARM_TYPE_DETECT_WIRE){ret = 4;}
    else if(type == ALARM_TYPE_PERSON_STAYING){ret = 5;}
    else if(type == ALARM_TYPE_DETECT_ABSENT){ret = 6;}
    else if(type == ALARM_TYPE_PARKING_VIOLATION){ret = 7;}
    else if(type == ALARM_TYPE_RETROGRADE){ret = 8;}
    else if(type == ALARM_TYPE_ALARMIN){ret = 9;}

    return ret;
}

int ovfs_linkpush(ModuleHandle_T hModuleHandle, OVFS_ALARM_EVENT *pAlarmEvent)
{
    int ret = 0;
    int alarmType = 0;
    S8 *picPath = NULL;
    char cmd[128] = {0};
    ALARMPUSH_CFG alarmcfg = {0};

    if(strlen(g_sn) == 0)
    {
        get_sn(hModuleHandle);
    }

    if(!g_moduleHandle)
    {
        g_moduleHandle = hModuleHandle;
    }

    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();

    alarmType = get_pushType(pAlarmEvent->iType);
    alarmcfg.alarmType = alarmType;
    LOGW("alarmType:[%d] iState:[%d]\n",alarmType,pAlarmEvent->iState);
    //alarmcfg.ch = 1;
    int time = 0;
    Common_Common2LinuxTime(&pAlarmEvent->t_start, (time_t *)(&time));
    alarmcfg.startTime = time;
    time = 0;
    Common_Common2LinuxTime(&pAlarmEvent->t_stop, (time_t *)(&time));
    alarmcfg.stopTime = time;
    alarmcfg.status = pAlarmEvent->iState;

    if(pAlarmCfg->pushCfg.enable && pAlarmCfg->pushCfg.withImage)
    {
        if(pAlarmEvent->iState)
        {
            oper_snap(hModuleHandle, 0, 0, pAlarmEvent, &picPath);
            if(picPath)
            {
                snprintf(alarmcfg.picPath, sizeof(alarmcfg.picPath),"/dev/push_%d.jpg",alarmType);
                snprintf(cmd, sizeof(cmd),"mv %s %s",picPath,alarmcfg.picPath);
                Common_System(cmd);
                Common_Free(picPath, __FUNCTION__, __LINE__);
                picPath = NULL;
            }
            else
            {
                LOGE("snap failed\n");
            }
        }
    }

    Common_Lock(g_alarmPushLock);
    g_alarmMap[alarmType] = alarmcfg;
    LOGW("g_alarmMap alarmType:[%d] iState:[%d]\n",alarmcfg.alarmType,alarmcfg.status);
    Common_UnLock(g_alarmPushLock);

    return ret;
}

int ovfs_linkOutput(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent)
{
    LOGW("ALARM LINK, alarmname:%s\n", pAlarmEvent->sName);
    S32 iDev = 0,iChan = 0;
    OVFS_COMMON_CFG *pCfg = NULL;
    OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();

    if(!pAlarmEvent)
        return -1;
    if(pAlarmEvent->iType == ALARM_TYPE_LOW_BATTERY_ALARM)
    {
        return 0;
    }
    iDev = pAlarmEvent->iDev;
    iChan = pAlarmEvent->iChan;

    pCfg = getCfgByAlarmName(alarmCfg,iDev,iChan,pAlarmEvent->sName);

    if(!pCfg)
    {
        LOGE("Invalid sName(%s) or iDev(%d) or iChan(%d)!\n",pAlarmEvent->sName,iDev,iChan);
        return -1;
    }

    ovfs_linkpush(hModuleHandle, pAlarmEvent);

    //if(pLinkageCfg->linkReportCenter.enable)
    //    ovfs_linkReportCentre(hModuleHandle,pLinkageCfg, pAlarmEvent);
    if(pCfg->soundCfg.enable)
    {
        ovfs_linkSound(hModuleHandle,&pCfg->soundCfg, pAlarmEvent,-1);
    }

    //if(pCfg->lightCfg.enable)
    {
        ovfs_linkLightAlarm(hModuleHandle,&pCfg->lightCfg, pAlarmEvent);
    }

   /* if(pLinkageCfg->linkRedblueLight.enable)
    {
        ovfs_linkRedBlueLight(hModuleHandle,pLinkageCfg, pAlarmEvent);
    }*/

#ifndef SIMPLIFIED
    if(pCfg->linkage.linkFtp.enable)
        ovfs_linkFtp(hModuleHandle,&pCfg->linkage, pAlarmEvent);
  /*  if(pLinkageCfg->linkHttp.enable)
        ovfs_linkHttp(hModuleHandle,pLinkageCfg, pAlarmEvent);
*/
    //if(pCfg->linkage.linkAlarmOut.enable)
    {
        ovfs_linkAlarmOut(hModuleHandle,&pCfg->linkage, pAlarmEvent);
    }

    ovfs_linkExpandAlarmOut(hModuleHandle,&pCfg->linkage, pAlarmEvent);
    //if(pLinkageCfg->linkRecord.enable)
    {
        ovfs_linkRecord(hModuleHandle,&pCfg->linkage, pAlarmEvent);
    }
    if(pCfg->linkage.linkPtz.type)
    {
        ovfs_linkPtz(hModuleHandle,&pCfg->linkage, pAlarmEvent);
    }
    if(pCfg->linkage.linkMail.enable)
    {
        ovfs_linkMail(hModuleHandle,&pCfg->linkage, pAlarmEvent);
    }

    if(pCfg->linkage.linkSnap.enable)
    {
        if(!g_SnapData.bSnap)
        {
            //Common_Thread_T t_Thread = NULL;
            OVFS_ALARM_EVENT* pTempEvent = (OVFS_ALARM_EVENT*)Common_Malloc(sizeof(OVFS_ALARM_EVENT), 0, __FUNCTION__, __LINE__);
            if(pTempEvent)
            {
                memset(pTempEvent,0,sizeof(OVFS_ALARM_EVENT));
                memcpy(pTempEvent,pAlarmEvent,sizeof(OVFS_ALARM_EVENT));
                if(pAlarmEvent->sName)
                {
                    pTempEvent->sName = (S8 *)Common_StrDup(pAlarmEvent->sName, __FUNCTION__, __LINE__);
                }
                if(pAlarmEvent->sDevName)
                {
                    pTempEvent->sDevName = (S8 *)Common_StrDup(pAlarmEvent->sDevName, __FUNCTION__, __LINE__);
                }
                g_SnapData.pAlarmEvent = pTempEvent;
                memcpy(&g_SnapData.snapCfg,&pCfg->linkage.linkSnap,sizeof(OVFS_LINK_SNAP_CFG));
                g_SnapData.bSnap = 1;
                LOGD("[%s]snap!bSnap:%d\n",g_SnapData.pAlarmEvent->sName,g_SnapData.bSnap);
                //Common_Thread_Create(&t_Thread,__FUNCTION__,0,2,Thread_LinkSnap,(void*)pSnapData);
                //ovfs_linkSnap(hModuleHandle,pLinkageCfg, pAlarmEvent);
            }
        }
    }
#endif
	return 0;
}
