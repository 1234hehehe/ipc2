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
#include "ovfs_alarm.h"
#include "ovfs_alarm_cfg.h"
#include "ovfs_alarm_method.h"

static OVFS_ALARM_CFG g_alarm_cfg;

int g_light_duration = 0;

int ovfs_get_light_dealy(ModuleHandle_T hModuleHandle)
{
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/boardsys/sys/LightAlarm/Delay",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"get",0,0);

        iRet = Module_CallFunctions(hModuleHandle, pRoot, &pResult, 3000);
        if(iRet == 0)
        {
            Common_Json_GetAttrValueInt(pResult, "Delay", &g_light_duration);
        }
        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return 0;
}

int ovfs_set_light_dealy(ModuleHandle_T hModuleHandle)
{
    cJSON_Struct *pResult = NULL,*pRoot = NULL;
    int iRet = 0;
    pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pRoot)
    {
        Common_Json_SetAttrValue(pRoot,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Uri",Common_Json_Type_String,"/boardsys/sys/LightAlarm/Delay",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pRoot,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValueInt(pRoot,"Data/Delay",g_light_duration);

        iRet = Module_CallFunctions(hModuleHandle, pRoot, NULL, 3000);

        Common_Json_Delete(pRoot);
        Common_Json_Delete(pResult);
        pRoot = NULL;
        pResult = NULL;
    }
    return iRet;
}

/*int addSchduleTime(cJSON_Struct **pInOut,S32 index,OVFS_SCHEDTIME_TIME_T* schedtime)
{
    if(!pInOut ||!(*pInOut))
        return -1;
    int j = 0,s = 0;
    char cmd[64] = {0};
    cJSON_Struct* pRoot = *pInOut,*pTemp1 = NULL,*pTemp2 = NULL;

    for(j = 0; j < MAX_DAYS; j++)
    {
        memset(cmd,0,sizeof(cmd));
        sprintf(cmd,"Weekday%d",j);

        pTemp1 = Common_Json_SetAttrValue(pRoot,index,cmd,Common_Json_Type_Object,NULL,0,0);
        if(!pTemp1)
            continue;
        for(s = 0; s < MAX_TIMESEGMENT; s++)
        {
            memset(cmd,0,sizeof(cmd));
            sprintf(cmd,"Sched%d",s);
            pTemp2 =Common_Json_SetAttrValue(pTemp1,-1,cmd,Common_Json_Type_Object,NULL,0,0);
            if(!pTemp2)
                continue;
            Common_Json_SetAttrValue(pTemp2,-1,"Start",Common_Json_Type_Number,NULL, schedtime->depSchedTime[j][s].startTime, 0);
            Common_Json_SetAttrValue(pTemp2,-1,"Stop",Common_Json_Type_Number,NULL, schedtime->depSchedTime[j][s].stopTime, 0);
        }
    }
    return 0;
}*/

//返回??:参数无变??:参数发生变化
int fillSchduleTime(cJSON_Struct *pInData,OVFS_SCHEDTIME_TIME_T *pSchedtime)
{
    int i = 0;
    int iNum = 0;
    cJSON_Struct* pTemp1 = NULL,*pTemp2 = NULL;
    if(!pInData)
    {
        LOGE("pInData is null\n");
        return -1;
    }

    if(Common_Json_GetAttrValueInt(pInData, "AlarmMode", &iNum))
    {
        pSchedtime->mode = iNum;
    }

    pTemp1 = Common_Json_GetAttrValueArr(pInData, "WeekList");
    if(pTemp1)
    {
        for(i = 0; i < MAX_DAYS; i++)
        {
            if(Common_Json_GetAttrValue(pTemp1, i, NULL, NULL, NULL, &iNum, NULL))
            {
                pSchedtime->weeklist[i] = iNum;
            }
        }
    }

    pTemp1 = Common_Json_GetAttrValueArr(pInData, "TimeList");
    if(pTemp1)
    {
        for(i = 0; i < MAX_TIMESEGMENT; i++)
        {
            pTemp2 = Common_Json_GetAttrValueArrItem(pTemp1, i);
            if(Common_Json_GetAttrValue(pTemp2, 0, NULL, NULL, NULL, &iNum, NULL))
            {
                pSchedtime->depSchedTime[i].startTime = iNum;
            }

            if(Common_Json_GetAttrValue(pTemp2, 1, NULL, NULL, NULL, &iNum, NULL))
            {
                pSchedtime->depSchedTime[i].stopTime = iNum;
            }
        }
    }
    return 0;
}

int SchduleTimeStructToJson(cJSON_Struct* pInOut,OVFS_SCHEDTIME_TIME_T* schedtime,int isStructToJson)
{
    if(!pInOut || !schedtime)
        return -1;

    int i = 0;
    cJSON_Struct *pTemp1 = NULL,*pTemp2 = NULL;

    int ret = 0;

    if(isStructToJson)
    {
        Common_Json_SetAttrValueInt(pInOut, "AlarmMode", schedtime->mode);
        pTemp1 = Common_Json_SetAttrValueArr(pInOut, "WeekList");
        for(i = 0; i < MAX_DAYS; i++)
        {
            Common_Json_SetAttrValueArrInt(pTemp1, i, schedtime->weeklist[i]);
        }

        pTemp1 = Common_Json_SetAttrValueArr(pInOut, "TimeList");

        for(i = 0; i < MAX_TIMESEGMENT; i++)
        {
            pTemp2 = Common_Json_SetAttrValue(pTemp1, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValueArrInt(pTemp2, 0, schedtime->depSchedTime[i].startTime);
            Common_Json_SetAttrValueArrInt(pTemp2, 1, schedtime->depSchedTime[i].stopTime);
        }
    }
    else
    {
        fillSchduleTime(pInOut, schedtime);
    }

    return ret;
}

int CheckIsInSchduleTime(long timeSec,OVFS_SCHEDTIME_TIME_T* pSched)
{
    if(pSched == NULL)
        return 0;

    time_t curTime = timeSec;
    if(curTime < 0)
        curTime = time(NULL);


    Common_Time_T commT;
    Common_Linux2CommonTime(curTime,&commT);
    int i = 0, j = 0, wday = commT.wday;
    if(wday < 0 || wday > 6)
    {
        LOGE("wday=%d invail\n",wday);
        return 0;
    }

    unsigned int curT = commT.hour*100 + commT.min;

    /*for(i = 0; i < (int)COMMON_ARRAY_ELEMENT_COUNT(schedtime->depSchedTime[wday]); i++)
    {
        if( (curT >= schedtime->depSchedTime[wday][i].startTime) && (curT < schedtime->depSchedTime[wday][i].stopTime) )
            return 1;
        else if( (curT > schedtime->depSchedTime[wday][i].startTime) && (curT <= schedtime->depSchedTime[wday][i].stopTime) )
            return 1;
        else
            continue;
    }*/


    if(pSched->mode == 1)
    {
        return 1;
    }
    else if(pSched->mode == 2)
    {
        if(curT >= 1800 || curT <= 600)
        {
            return 1;
        }
    }
    else if(pSched->mode == 3)
    {
        i = wday == 0 ? 6 : wday - 1;

        if(pSched->weeklist[i])
        {
            for(j = 0; j < MAX_TIMESEGMENT; j++)
            {
                if((0 == pSched->depSchedTime[j].startTime) && (0 == pSched->depSchedTime[j].stopTime))
                {
                    continue;
                }
                if((curT >= pSched->depSchedTime[j].startTime) && (curT <= pSched->depSchedTime[j].stopTime))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*OVFS_COMMON_CFG * getCommCfgByAlarmName(POVFS_ALARM_CFG pAlarmCfg,char *pAlarmName)
{
    OVFS_COMMON_CFG *pCommCfg = NULL;

    if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_MOTION))		pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->motioncfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_VHIDE))		pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->vhidecfg;

    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DETPERSON))	pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->PersonDetcfg;

#ifndef SIMPLIFIED
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_AI))				pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->alarmIncfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DISKFULL))	pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->diskFullcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DISKERR))		pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->diskErrcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_UNFORMAT))	pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->unFormatcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_RECERR))		pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->vRecErrcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_CLIENTBREAK))		pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->cliBreakcfg;
#endif
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_NETBREAK))	pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->netBreakcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_IPCONFLICT))	pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->ipConflitcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_ILLACCESS))	pCommCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->illAccesscfg;

    return pCommCfg;
}

OVFS_COMMON_TRI_CFG *getTriCfgByAlarmName(POVFS_ALARM_CFG pAlarmCfg,int iDev,int iChan,char *pAlarmName)
{
    OVFS_COMMON_TRI_CFG *pTriCfg = NULL;

    if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_MOTION)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->motioncfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_VHIDE)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->vhidecfg.cfgTri[iDev][iChan];

    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DETPERSON)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->PersonDetcfg.cfgTri[iDev][iChan];

    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_NETBREAK)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->netBreakcfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_IPCONFLICT)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->ipConflitcfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_ILLACCESS)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->illAccesscfg.cfgTri[iDev][iChan];
#ifndef SIMPLIFIED
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_AI)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->alarmIncfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DISKFULL)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->diskFullcfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DISKERR)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->diskErrcfg.cfgTri[iDev][iChan];

    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_UNFORMAT)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->unFormatcfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_RECERR)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->vRecErrcfg.cfgTri[iDev][iChan];
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_CLIENTBREAK)) pTriCfg = (OVFS_COMMON_TRI_CFG *)&pAlarmCfg->cliBreakcfg.cfgTri[iDev][iChan];

#endif
    return pTriCfg;
}
*/

OVFS_COMMON_CFG * getCfgByAlarmName(POVFS_ALARM_CFG pAlarmCfg,int iDev,int iChan,char *pAlarmName)
{
    OVFS_COMMON_CFG *pCfg = NULL;

    if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_MOTION)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->motioncfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_VHIDE)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->vhidecfg;

    //else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DETPERSON)) pLinkageCfg = (OVFS_LINKAGE_CFG *)&pAlarmCfg->PersonDetcfg.linkage[iDev][iChan];

#ifndef SIMPLIFIED
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_AI)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->alarmIncfg.linkage;
    /*else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DISKFULL)) pCfg = (OVFS_LINKAGE_CFG *)&pAlarmCfg->diskFullcfg.linkage;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DISKERR)) pCfg = (OVFS_LINKAGE_CFG *)&pAlarmCfg->diskErrcfg.linkage;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_UNFORMAT)) pCfg = (OVFS_LINKAGE_CFG *)&pAlarmCfg->unFormatcfg.linkage;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_RECERR)) pCfg = (OVFS_LINKAGE_CFG *)&pAlarmCfg->vRecErrcfg.linkage;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_CLIENTBREAK)) pCfg = (OVFS_LINKAGE_CFG *)&pAlarmCfg->cliBreakcfg.linkage;*/
#endif

    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_REGIONALINVASION)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->regionalInvasioncfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DETWIRE)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->traverseCfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_PERSONSTAYING)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->personStayCfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_DETABSENT)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->absentCfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_PARKINGVIOLATION)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->parkViolationCfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_RETROGRADE)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->vehicleRetrogradeCfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_SENSORALARM)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->sensorAlarm[iChan];

    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_NETBREAK)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->netBreakcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_IPCONFLICT))pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->ipConflitcfg;
    else if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_ILLACCESS)) pCfg = (OVFS_COMMON_CFG *)&pAlarmCfg->illAccesscfg;
    return pCfg;
}


int ovfs_make_linkage_json(POVFS_COMMON_CFG pCommonCfg,cJSON_Struct *outJson)
{
    int i = 0;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    cJSON_Struct* pRoot = NULL,*pTemp = NULL;

    if(!pAbility)
    {
        LOGE("pAbility == NULL\n");
        return -1;
    }
    if(!pCommonCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }

    if(NULL == outJson)
    {
        outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }
    pRoot = outJson;

    Common_Json_SetAttrValueObj(pRoot, "Ftp");
    Common_Json_SetAttrValueInt(pRoot, "Ftp/Enable", pCommonCfg->linkage.linkFtp.enable);

    Common_Json_SetAttrValueObj(pRoot, "Email");
    Common_Json_SetAttrValueInt(pRoot, "Email/Enable", pCommonCfg->linkage.linkMail.enable);

    Common_Json_SetAttrValueObj(pRoot, "Snap");
    Common_Json_SetAttrValueInt(pRoot, "Snap/Enable", pCommonCfg->linkage.linkSnap.enable);
    Common_Json_SetAttrValueInt(pRoot, "Snap/Count", pCommonCfg->linkage.linkSnap.count);
    Common_Json_SetAttrValueInt(pRoot, "Snap/Interval", pCommonCfg->linkage.linkSnap.interval);

    Common_Json_SetAttrValueObj(pRoot, "Ptz");
    Common_Json_SetAttrValueInt(pRoot, "Ptz/Enable", pCommonCfg->linkage.linkPtz.type);
    Common_Json_SetAttrValueInt(pRoot, "Ptz/PresetNo", pCommonCfg->linkage.linkPtz.index);

    Common_Json_SetAttrValueObj(pRoot, "AlarmOut");
    Common_Json_SetAttrValueInt(pRoot, "AlarmOut/Enable", pCommonCfg->linkage.linkAlarmOut.enable);
    pTemp = Common_Json_SetAttrValueArr(pRoot, "AlarmOut/List");
    for(i=0; i<pAbility->alarmOutNum && i<MAX_ALARMOUT; i++)
    {
        Common_Json_SetAttrValueArrInt(pTemp, i, pCommonCfg->linkage.linkAlarmOut.mask[i]);
    }

    Common_Json_SetAttrValueObj(pRoot, "ExpandAlarmOut");
    Common_Json_SetAttrValueInt(pRoot, "ExpandAlarmOut/Enable", pCommonCfg->linkage.linkExpandAlarmOut.enable);
    pTemp = Common_Json_SetAttrValueArr(pRoot, "ExpandAlarmOut/List");
    for(i=0; i<pCommonCfg->linkage.linkExpandAlarmOut.iCount; i++)
    {
        cJSON_Struct *item = Common_Json_SetAttrValueArrObj(pTemp, i);
        Common_Json_SetAttrValueInt(item, "Dev", pCommonCfg->linkage.linkExpandAlarmOut.list[i].dev);
        Common_Json_SetAttrValueInt(item, "Ch", pCommonCfg->linkage.linkExpandAlarmOut.list[i].ch);
    }

    SchduleTimeStructToJson(pRoot, &pCommonCfg->linkage.shedule, 1);

    return 0;
}

int ovfs_make_linkage_struct(cJSON_Struct *dataJson,POVFS_COMMON_CFG pCommonCfg)
{
    int iNum = 0;
    int size = 0,i = 0;
    cJSON_Struct* Temp = NULL;

    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }

    if(!pCommonCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Ftp/Enable", &iNum))
    {
        pCommonCfg->linkage.linkFtp.enable = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Email/Enable", &iNum))
    {
        pCommonCfg->linkage.linkMail.enable = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Snap/Enable", &iNum))
    {
        pCommonCfg->linkage.linkSnap.enable = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Snap/Count", &iNum))
    {
        pCommonCfg->linkage.linkSnap.count = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Snap/Interval", &iNum))
    {
        pCommonCfg->linkage.linkSnap.interval = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Ptz/Enable", &iNum))
    {
        pCommonCfg->linkage.linkPtz.type = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "Ptz/PresetNo", &iNum))
    {
        pCommonCfg->linkage.linkPtz.index = iNum;
    }

    if(Common_Json_GetAttrValueInt(dataJson, "AlarmOut/Enable", &iNum))
    {
        pCommonCfg->linkage.linkAlarmOut.enable = iNum;
    }

    if(Temp = Common_Json_GetAttrValueArr(dataJson, "AlarmOut/List"))
    {
        size = Common_Json_ArraySize(Temp);
        for(i=0; i<size && i<MAX_ALARMOUT; i++)
        {
            if(Common_Json_GetAttrValue(Temp, i, NULL, NULL, NULL, &iNum, NULL))
            {
                pCommonCfg->linkage.linkAlarmOut.mask[i] = iNum;
            }
        }
    }

    if(Common_Json_GetAttrValueInt(dataJson, "ExpandAlarmOut/Enable", &iNum))
    {
        pCommonCfg->linkage.linkExpandAlarmOut.enable = iNum;
    }

    if(Temp = Common_Json_GetAttrValueArr(dataJson, "ExpandAlarmOut/List"))
    {
        size = Common_Json_ArraySize(Temp);
        pCommonCfg->linkage.linkExpandAlarmOut.iCount = size;
        for(i=0; i<size; i++)
        {
            if(Common_Json_GetAttrValue(Temp, i, "Dev", NULL, NULL, &iNum, NULL))
            {
                pCommonCfg->linkage.linkExpandAlarmOut.list[i].dev = iNum;
            }

            if(Common_Json_GetAttrValue(Temp, i, "Ch", NULL, NULL, &iNum, NULL))
            {
                pCommonCfg->linkage.linkExpandAlarmOut.list[i].ch = iNum;
            }
        }
    }

    fillSchduleTime(dataJson, &pCommonCfg->linkage.shedule);

    return 0;
}

int ovfs_make_arm_json(POVFS_ALARM_CFG pAlarmCfg,cJSON_Struct **outJson)
{
    cJSON_Struct *pRoot = NULL;
    if(!outJson)
    {
        LOGE("outJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    if(NULL == (*outJson))
    {
        *outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }
    pRoot = *outJson;
    Common_Json_SetAttrValue(pRoot, -1, "Enable", Common_Json_Type_Number, NULL, pAlarmCfg->enable, 0);
    return 0;
}

int ovfs_make_arm_struct(cJSON_Struct * dataJson,POVFS_ALARM_CFG pAlarmCfg)
{
    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarm_cfg == NULL\n");
        return -1;
    }
    if(NULL == Common_Json_GetAttrValue(dataJson, -1, "Enable", NULL, NULL, &pAlarmCfg->enable, NULL))
    {
        pAlarmCfg->enable = 1;
    }
    return 0;
}
#ifndef SIMPLIFIED
int ovfs_make_email_json(POVFS_ALARM_CFG pAlarmCfg,cJSON_Struct **outJson)
{
    S32 i = 0;
    cJSON_Struct *pRoot = NULL,*pChild = NULL,*pChild1 = NULL,*pArray = NULL;

    if(!outJson)
    {
        LOGE("outJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    if(NULL == (*outJson))
    {
        *outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }
    pRoot = *outJson;
    pChild = Common_Json_SetAttrValue(pRoot, -1, "EmailCfg", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender/User", Common_Json_Type_String, pAlarmCfg->mailcfg.sender.pName, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender/Password", Common_Json_Type_String, pAlarmCfg->mailcfg.sender.pPwd, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender/SMTPServer", Common_Json_Type_String, pAlarmCfg->mailcfg.sender.pSmtpSvr, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender/SMTPPort", Common_Json_Type_Number, 0, pAlarmCfg->mailcfg.sender.port, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender/EnableSSL", Common_Json_Type_Number, NULL, pAlarmCfg->mailcfg.sender.bEnSsl, 0);
    Common_Json_SetAttrValue(pChild, -1, "Sender/ServerVerify", Common_Json_Type_Number, NULL, pAlarmCfg->mailcfg.sender.SvrVerify, 0);
    pArray = Common_Json_SetAttrValue(pChild, -1, "Receiver", Common_Json_Type_Array, NULL, 0, 0);
    for(i = 0; i < pAlarmCfg->mailcfg.recvList.count; i++)
    {
        pChild1 = Common_Json_SetAttrValue(pArray, i, NULL, Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(pChild1, -1, "Name", Common_Json_Type_String, pAlarmCfg->mailcfg.recvList.recver[i].pName, 0, 0);
        Common_Json_SetAttrValue(pChild1, -1, "Addr", Common_Json_Type_String,pAlarmCfg->mailcfg.recvList.recver[i].pAddr, 0, 0);
    }
    Common_Json_SetAttrValue(pChild, -1, "Attachment", Common_Json_Type_Number, NULL, pAlarmCfg->mailcfg.bAttach, 0);
    Common_Json_SetAttrValue(pChild, -1, "MailInterval", Common_Json_Type_Number, NULL, pAlarmCfg->mailcfg.interval, 0);

    return 0;
}

int ovfs_make_email_struct(cJSON_Struct * dataJson,POVFS_ALARM_CFG pAlarmCfg)
{
    S32 i = 0;
    S8 *pString = NULL;
    cJSON_Struct *pRoot = NULL,*pChild = NULL,*pArray = NULL;

    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    pRoot = dataJson;
    //Common_Json_GetAttrValue(pRoot, -1, "LinkMail/Sender/User", NULL, (S8**)&pLinkage_cfg->linkMail.sender.pName, NULL, NULL);
    //Common_Json_GetAttrValue(pRoot, -1, "LinkMail/Sender/Password", NULL, (S8**)&pLinkage_cfg->linkMail.sender.pPwd, NULL, NULL);
    //Common_Json_GetAttrValue(pRoot, -1, "LinkMail/Sender/SMTPServer", NULL, (S8**)&pLinkage_cfg->linkMail.sender.pSmtpSvr, NULL, NULL);
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Sender/User", NULL, &pString, NULL, NULL);
    if(pString)	Common_Strcpy(pAlarmCfg->mailcfg.sender.pName,pString);
    pString = NULL;
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Sender/Password", NULL, &pString, NULL, NULL);
    if(pString)	Common_Strcpy(pAlarmCfg->mailcfg.sender.pPwd,pString);
    pString = NULL;
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Sender/SMTPServer", NULL, &pString, NULL, NULL);
    if(pString)	Common_Strcpy(pAlarmCfg->mailcfg.sender.pSmtpSvr,pString);
    pString = NULL;

    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Sender/SMTPPort", NULL, NULL, &pAlarmCfg->mailcfg.sender.port, NULL);
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Sender/EnableSSL", NULL, NULL, &pAlarmCfg->mailcfg.sender.bEnSsl, NULL);
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Sender/ServerVerify", NULL, NULL, &pAlarmCfg->mailcfg.sender.SvrVerify, NULL);
    pArray = Common_Json_GetItem(pRoot, -1, "EmailCfg/Receiver");

    for(i = 0; i < pAlarmCfg->mailcfg.recvList.count; i++)
    {
        pChild = Common_Json_GetItem(pArray, i, NULL);
        //Common_Json_GetAttrValue(pArray, i, "Name", NULL, (S8**)&pAlarm_cfg->mailcfg.recvList.recver[i].pName, NULL, NULL);
        //Common_Json_GetAttrValue(pArray, i, "Addr", NULL, (S8**)&pAlarm_cfg->mailcfg.recvList.recver[i].pAddr, NULL, NULL);
        Common_Json_GetAttrValue(pChild, -1, "Name", NULL, &pString, NULL, NULL);
        if(pString)	Common_Strcpy(pAlarmCfg->mailcfg.recvList.recver[i].pName,pString);
        pString = NULL;
        Common_Json_GetAttrValue(pChild, -1, "Addr", NULL, &pString, NULL, NULL);
        if(pString)	Common_Strcpy(pAlarmCfg->mailcfg.recvList.recver[i].pAddr,pString);
        pString = NULL;
    }
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/Attachment", NULL, NULL, &pAlarmCfg->mailcfg.bAttach, NULL);
    Common_Json_GetAttrValue(pRoot, -1, "EmailCfg/MailInterval", NULL, NULL, &pAlarmCfg->mailcfg.interval, NULL);
    return 0;
}
#endif

int LightAlarmCfgStruct2Json(OVFS_LIGHT_ALARM_CFG* cfg,cJSON_Struct* jsonObj,int isStructToJson)
{
    if(!cfg || !jsonObj)
        return -1;

    cJSON_Struct* tmp = NULL;
    cJSON_Struct* tmp1 = NULL;
    int ret = 0;
    if(isStructToJson)
    {
        tmp = Common_Json_SetAttrValue(jsonObj, -1, "LightAlarmCfg",Common_Json_Type_Object,NULL,0,0);
        if(tmp)
        {
            tmp1 = Common_Json_SetAttrValue(tmp, -1, "SchedTime",Common_Json_Type_Object,NULL,0,0);
            if(tmp1)
            {
                SchduleTimeStructToJson(tmp1,&(cfg->schTime),isStructToJson);
            }
        }
    }
    else
    {
        tmp = Common_Json_GetItem(jsonObj,-1,"LightAlarmCfg/SchedTime");
        if(tmp)
            ret = SchduleTimeStructToJson(tmp,&(cfg->schTime),isStructToJson);
    }

    return ret;
}

int FTPSnapCfgStruct2Json(OVFS_FTP_SNAP_CFG* cfg,cJSON_Struct* jsonObj,int isStructToJson)
{
    if(!cfg || !jsonObj)
        return -1;

    cJSON_Struct* tmp = NULL;
    cJSON_Struct* tmp1 = NULL;
    int ret = 0;
    if(isStructToJson)
    {
        tmp = Common_Json_SetAttrValue(jsonObj, -1, "FTPSnapCfg",Common_Json_Type_Object,NULL,0,0);
        if(tmp)
        {
            tmp1 = Common_Json_SetAttrValue(tmp, -1, "SchedTime",Common_Json_Type_Object,NULL,0,0);
            if(tmp1)
            {
                SchduleTimeStructToJson(tmp1,&(cfg->schTime),isStructToJson);
            }

            Common_Json_SetAttrValueInt(tmp, "SnapInterval", cfg->snapInterval);
            Common_Json_SetAttrValueInt(tmp, "SnapStreamIndex", cfg->snapStreamIndex);
        }
    }
    else
    {
        tmp = Common_Json_GetItem(jsonObj,-1,"FTPSnapCfg/SchedTime");
        if(tmp)
            ret = SchduleTimeStructToJson(tmp,&(cfg->schTime),isStructToJson);

        Common_Json_GetAttrValueInt(jsonObj, "FTPSnapCfg/SnapInterval", &cfg->snapInterval);
        Common_Json_GetAttrValueInt(jsonObj, "FTPSnapCfg/SnapStreamIndex", &cfg->snapStreamIndex);
    }
    LOGD("Way:%d\n",isStructToJson);
    return ret;
}

int RedblueLightStruct2Json(OVFS_LIGHT_ALARM_CFG* cfg,cJSON_Struct* jsonObj,int isStructToJson)
{
    if(!cfg || !jsonObj)
        return -1;

    cJSON_Struct* tmp = NULL;
    cJSON_Struct* tmp1 = NULL;
    int ret = 0;
    if(isStructToJson)
    {
        tmp = Common_Json_SetAttrValue(jsonObj, -1, "RedblueLightCfg",Common_Json_Type_Object,NULL,0,0);
        if(tmp)
        {
            tmp1 = Common_Json_SetAttrValue(tmp, -1, "SchedTime",Common_Json_Type_Object,NULL,0,0);
            if(tmp1)
            {
                SchduleTimeStructToJson(tmp1,&(cfg->schTime),isStructToJson);
            }
        }
    }
    else
    {
        tmp = Common_Json_GetItem(jsonObj,-1,"RedblueLightCfg/SchedTime");
        if(tmp)
            ret = SchduleTimeStructToJson(tmp,&(cfg->schTime),isStructToJson);
    }

    return ret;
}

/*int ovfs_make_sound_json(POVFS_ALARM_CFG pAlarmCfg,cJSON_Struct **outJson)
{
    S32 i = 0;
    cJSON_Struct *pRoot = NULL,*pChild = NULL,*pArray = NULL;
    OVFS_SOUND_CFG *pSoundCfg = &pAlarmCfg->soundCfg;
    if(!outJson)
    {
        LOGE("outJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    if(NULL == (*outJson))
    {
        *outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }
    pRoot = *outJson;
    pChild = Common_Json_SetAttrValue(pRoot, -1, "SoundAlarmCfg", Common_Json_Type_Object, NULL, 0, 0);
    pArray = Common_Json_SetAttrValue(pChild, -1, "List", Common_Json_Type_Array, NULL, 0, 0);
    for(i = 0; i < (S32)COMMON_ARRAY_ELEMENT_COUNT(pSoundCfg->soundName); i++)
    {
        Common_Json_SetAttrValue(pArray, i, "Id", Common_Json_Type_Number, NULL, i, 0);
        Common_Json_SetAttrValue(pArray, i, "Name", Common_Json_Type_String, pSoundCfg->soundName[i], 0, 0);
        Common_Json_SetAttrValue(pArray, i, "NameEn", Common_Json_Type_String, pSoundCfg->soundNameEn[i], 0, 0);
        Common_Json_SetAttrValue(pArray, i, "Times", Common_Json_Type_Number, NULL, pSoundCfg->playTimes[i], 0);
    }

    Common_Json_SetAttrValue(pChild, -1, "Language", Common_Json_Type_Number, NULL, pSoundCfg->language, 0);

    pArray = Common_Json_SetAttrValue(pChild, -1, "Shedule", Common_Json_Type_Object, NULL, 0, 0);
    addSchduleTime(&pArray,-1,&pSoundCfg->shedule);
    return 0;
}

int ovfs_make_sound_struct(cJSON_Struct * dataJson,POVFS_ALARM_CFG pAlarmCfg)
{
    S32 i = 0, value = 0, iSize = 0, id;
    S8 *pString = NULL;
    cJSON_Struct *pRoot = NULL,*pArray = NULL,*pTemp = NULL;
    OVFS_SOUND_CFG *pSoundCfg = &pAlarmCfg->soundCfg;
    char path[256];

    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    pRoot = dataJson;
    pArray = Common_Json_GetAttrValue(pRoot, -1, "SoundAlarmCfg/List", NULL, NULL, NULL, NULL);
    iSize = Common_Json_Size(pArray);
    for(i = 0; i < iSize; i++)
    {

        //connect sound_cfg and sound_file,judge sound_file exist
        sprintf(path, "/update/soundFile/sound_%d", i);
        if (access(path, F_OK) == -1)
        {
            Common_Json_GetAttrValue(pArray, i, "Id", NULL, NULL, &value, NULL);
            id = value;
            Common_Json_GetAttrValue(pArray, i, "Name", NULL, NULL, NULL, NULL);

            Common_Json_GetAttrValue(pArray, i, "Times", NULL, NULL, &value, NULL);
            pSoundCfg->playTimes[id] = value;
            value = 0;

            continue;
        }

        Common_Json_GetAttrValue(pArray, i, "Id", NULL, NULL, &value, NULL);
        id = value;
        Common_Json_GetAttrValue(pArray, i, "Name", NULL, &pString, NULL, NULL);
        if(pString)
        {
            snprintf(pSoundCfg->soundName[id], sizeof(pSoundCfg->soundName[id]), "%s", pString);
            pString = NULL;
        }
        Common_Json_GetAttrValue(pArray, i, "NameEn", NULL, &pString, NULL, NULL);
        if(pString)
        {
            snprintf(pSoundCfg->soundNameEn[id], sizeof(pSoundCfg->soundNameEn[id]), "%s", pString);
            pString = NULL;
        }
        Common_Json_GetAttrValue(pArray, i, "Times", NULL, NULL, &value, NULL);
        pSoundCfg->playTimes[id] = value;
        value = 0;
    }

    Common_Json_GetAttrValue(pRoot, -1, "SoundAlarmCfg/Language", NULL, NULL, &pSoundCfg->language, NULL);

    pTemp = Common_Json_GetAttrValue(pRoot, -1, "SoundAlarmCfg/Shedule", NULL, NULL, NULL, NULL);
    fillSchduleTime(pTemp, &pSoundCfg->shedule);

    return 0;
}
//#endif
*/
int ovfs_make_onekeydrive_json(POVFS_ALARM_CFG pAlarmCfg,cJSON_Struct **outJson)
{
    cJSON_Struct *pRoot = NULL,*pChild = NULL;
    OVFS_ONEKEY_DRIVE_CFG *pOneKeyDriveCfg = &pAlarmCfg->oneKeyDriveCfg;
    if(!outJson)
    {
        LOGE("outJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    if(NULL == (*outJson))
    {
        *outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }

    pRoot = *outJson;
    pChild = Common_Json_SetAttrValue(pRoot, -1, "OneKeyDriveCfg", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Duration", Common_Json_Type_Number, NULL, pOneKeyDriveCfg->Duration, 0);
	Common_Json_SetAttrValue(pChild, -1, "EnableAudio", Common_Json_Type_Number, NULL, pOneKeyDriveCfg->EnableAudio, 0);
	Common_Json_SetAttrValue(pChild, -1, "EnableLight", Common_Json_Type_Number, NULL, pOneKeyDriveCfg->EnableLight, 0);

    return 0;
}

int ovfs_make_onekeydrive_struct(cJSON_Struct * dataJson,POVFS_ALARM_CFG pAlarmCfg)
{
    cJSON_Struct *pRoot = NULL;
    OVFS_ONEKEY_DRIVE_CFG *pOneKeyDriveCfg = &pAlarmCfg->oneKeyDriveCfg;

    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    pRoot = dataJson;
    Common_Json_GetAttrValue(pRoot, -1, "OneKeyDriveCfg/Duration", NULL, NULL, &pOneKeyDriveCfg->Duration, NULL);
	Common_Json_GetAttrValue(pRoot, -1, "OneKeyDriveCfg/EnableAudio", NULL, NULL, &pOneKeyDriveCfg->EnableAudio, NULL);
	Common_Json_GetAttrValue(pRoot, -1, "OneKeyDriveCfg/EnableLight", NULL, NULL, &pOneKeyDriveCfg->EnableLight, NULL);

    return 0;
}

int ovfs_make_pushcfg_json(POVFS_ALARM_CFG pAlarmCfg,cJSON_Struct **outJson)
{
    cJSON_Struct *pRoot = NULL,*pChild = NULL;
    OVFS_PUSH_CFG *pPushCfg = &pAlarmCfg->pushCfg;
    if(!outJson)
    {
        LOGE("outJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    if(NULL == (*outJson))
    {
        *outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }

    pRoot = *outJson;
    pChild = Common_Json_SetAttrValue(pRoot, -1, "PushConfig", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(pChild, -1, "Enable", Common_Json_Type_Number, NULL, pPushCfg->enable, 0);
	Common_Json_SetAttrValue(pChild, -1, "Url", Common_Json_Type_String, pPushCfg->url, 0, 0);
	Common_Json_SetAttrValue(pChild, -1, "Interval", Common_Json_Type_Number, NULL, pPushCfg->interval, 0);
	Common_Json_SetAttrValue(pChild, -1, "WithImage", Common_Json_Type_Number, NULL, pPushCfg->withImage, 0);
	Common_Json_SetAttrValue(pChild, -1, "UpdateImage", Common_Json_Type_Number, NULL, pPushCfg->updateImage, 0);

    return 0;
}

int ovfs_make_pushcfg_struct(cJSON_Struct * dataJson,POVFS_ALARM_CFG pAlarmCfg)
{
    char *pStr = NULL;
    cJSON_Struct *pRoot = NULL;
    OVFS_PUSH_CFG *pPushCfg = &pAlarmCfg->pushCfg;

    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }
    if(!pAlarmCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }
    pRoot = dataJson;
    Common_Json_GetAttrValueInt(pRoot, "PushConfig/Enable", &pPushCfg->enable);
    Common_Json_GetAttrValueStr(pRoot, "PushConfig/Url", &pStr);
    snprintf(pPushCfg->url, sizeof(pPushCfg->url), "%s", pStr?pStr:"");
    Common_Json_GetAttrValueInt(pRoot, "PushConfig/Interval", &pPushCfg->interval);
    Common_Json_GetAttrValueInt(pRoot, "PushConfig/WithImage", &pPushCfg->withImage);
    Common_Json_GetAttrValueInt(pRoot, "PushConfig/UpdateImage", &pPushCfg->updateImage);

    return 0;
}

int ovfs_make_common_json(POVFS_COMMON_CFG pCommonCfg,char *pAlarmName,cJSON_Struct **outJson, int needNameNode)
{
    int k = 0,i = 0;
    char cmd[64] = {0}, *str1 = NULL, *str2 = NULL;

    OVFS_DEV_ABILITY *pDev = NULL;
    int devNum = 1,chanNum = MAX_CHANNEL_SUPPORT;
    cJSON_Struct *pRoot = NULL,*pCfg = NULL,*pTemp = NULL;

    if(!outJson)
    {
        LOGW("outJson == NULL\n");
        return -1;
    }
    if(NULL == (*outJson))
    {
        *outJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    }
    pRoot = *outJson;

    if(needNameNode)
    {
        pCfg = Common_Json_SetAttrValueObj(pRoot, pAlarmName);
    }
    else
    {
        pCfg = pRoot;
    }

    pTemp = Common_Json_SetAttrValueObj(pCfg, "LinkageCfg");
    ovfs_make_linkage_json(pCommonCfg,pTemp);

    pTemp = Common_Json_SetAttrValueObj(pCfg, "LightCfg");
    Common_Json_SetAttrValueInt(pTemp, "Enable", pCommonCfg->lightCfg.enable);
    Common_Json_SetAttrValueInt(pTemp, "Duration", g_light_duration/*pCommonCfg->lightCfg.duration*/);
    SchduleTimeStructToJson(pTemp, &pCommonCfg->lightCfg.schTime, 1);

    pTemp = Common_Json_SetAttrValueObj(pCfg, "AudioCfg");
    Common_Json_SetAttrValueInt(pTemp, "Enable", pCommonCfg->soundCfg.enable);
    Common_Json_SetAttrValueInt(pTemp, "PlayCount", pCommonCfg->soundCfg.playTimes);
    Common_Json_SetAttrValueInt(pTemp, "AudioSelected", pCommonCfg->soundCfg.audioSelected);
    SchduleTimeStructToJson(pTemp, &pCommonCfg->soundCfg.shedule, 1);

    return 0;
}

int ovfs_make_common_struct(cJSON_Struct * dataJson,POVFS_COMMON_CFG pCommonCfg,char *pAlarmName, int needNameNode)
{
    int iNum = 0;
    char cmd[64] = {0}, *str1 = NULL, *str2 = NULL;

    cJSON_Struct *pCfg = NULL,*pTemp = NULL,*pTemp1 = NULL;

    if(!dataJson)
    {
        LOGE("dataJson == NULL\n");
        return -1;
    }
    if(!pCommonCfg)
    {
        LOGE("pAlarmCfg == NULL\n");
        return -1;
    }

    if(needNameNode)
    {
        pCfg = Common_Json_GetAttrValueObj(dataJson,pAlarmName);
    }
    else
    {
        pCfg = dataJson;
    }

    if(!pCfg)
    {
        //LOGE("Not find TriggerCfg Config, AlarmName:%s\n",pAlarmName);
        //return -1;
    }
    else
    {
        pTemp = Common_Json_GetAttrValueObj(pCfg, "LightCfg");
        if(pTemp)
        {
            if(Common_Json_GetAttrValueInt(pTemp, "Enable", &iNum))
            {
                pCommonCfg->lightCfg.enable = iNum;
            }

            if(Common_Json_GetAttrValueInt(pTemp, "Duration", &iNum))
            {
                pCommonCfg->lightCfg.duration = iNum;
                g_light_duration = iNum;
            }

            fillSchduleTime(pTemp, &pCommonCfg->lightCfg.schTime);
        }

        pTemp = Common_Json_GetAttrValueObj(pCfg, "AudioCfg");
        if(pTemp)
        {
            if(Common_Json_GetAttrValueInt(pTemp, "Enable", &iNum))
            {
                pCommonCfg->soundCfg.enable = iNum;
            }

            if(Common_Json_GetAttrValueInt(pTemp, "PlayCount", &iNum))
            {
                pCommonCfg->soundCfg.playTimes = iNum;
            }

            if(Common_Json_GetAttrValueInt(pTemp, "AudioSelected", &iNum))
            {
                pCommonCfg->soundCfg.audioSelected = iNum;
            }

            fillSchduleTime(pTemp, &pCommonCfg->soundCfg.shedule);
        }

        pTemp = Common_Json_GetAttrValueObj(pCfg, "LinkageCfg");
        if(pTemp)
        {
            ovfs_make_linkage_struct(pTemp,pCommonCfg);
        }

    }
    return 0;
}

int ovfs_get_arming_state()
{
    return g_alarm_cfg.enable;
}

void ovfs_set_arming_state(int bEnable)
{
    g_alarm_cfg.enable = bEnable;
}
/*
int set_trig_default_cfg(OVFS_SCHEDTIME_TIME_T *pSchedTime)
{
    if(!pSchedTime)
        return 0;
    for(int i = 0; i < MAX_DAYS; i++)
    {
        for(int j = 0; j < MAX_TIMESEGMENT; j++)
        {
            if(0 == j)
            {
                pSchedTime->depSchedTime[i][j].startTime = 0;
                pSchedTime->depSchedTime[i][j].stopTime = 2359;
            }
            else
            {
                pSchedTime->depSchedTime[i][j].startTime = 0;
                pSchedTime->depSchedTime[i][j].stopTime = 0;
            }
        }
    }
    return 0;
}
*/
int set_commoncfg_default(OVFS_COMMON_CFG *pcfg)
{
    pcfg->linkage.linkMail.bDefault = 0;
    pcfg->linkage.linkSnap.count = 1;
    pcfg->linkage.linkSnap.interval = 1;
    pcfg->linkage.shedule.mode = 1;

    pcfg->lightCfg.duration = 30;
    pcfg->lightCfg.schTime.mode = 1;

    pcfg->soundCfg.shedule.mode = 1;

    return 0;
}
/*
int set_link_default_cfg_ex(OVFS_LINKAGE_CFG *pLinkage_cfg)
{
    OVFS_ABILITY *pAbility = NULL;
    int j;
    if(!pLinkage_cfg)
        return 0;
    pLinkage_cfg->linkMail.recvList.count = 3;
    pLinkage_cfg->linkMail.recvList.recver = (OVFS_RECEIVE_INFO*)Common_Malloc(sizeof(OVFS_RECEIVE_INFO)*pLinkage_cfg->linkMail.recvList.count, 0, __FUNCTION__, __LINE__);
    memset(pLinkage_cfg->linkMail.recvList.recver,0,sizeof(OVFS_RECEIVE_INFO)*pLinkage_cfg->linkMail.recvList.count);

    pAbility = ovfs_get_ability();
    if(!pAbility)
        return -1;
    for(j = 0; j < MAX_ALARMOUT/8 + 1; j++)
    {
        pLinkage_cfg->linkAout.mask[j] = 1;
    }
    pLinkage_cfg->linkAout.enable = 1;

    pLinkage_cfg->linkRecord.enable = 1;
    for(int i = 0; i < pAbility->devNum; i++)
    {

        OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
        if(!pDev)
            continue;

        pLinkage_cfg->linkRecord.mask[i] = 0x1;

        for(int j = 0; j < pDev->chanNum; j++)
        {
            pLinkage_cfg->linkSnap.snapInfo[i][j].enable = 1;
            pLinkage_cfg->linkSnap.snapInfo[i][j].count = 1;
            pLinkage_cfg->linkSnap.snapInfo[i][j].interval = 1;


        }
    }
    return 0;
}

int set_onekeydrive_default_cfg(OVFS_ONEKEY_DRIVE_CFG *pOneKeyDriveCfg)
{
	pOneKeyDriveCfg->Duration = 60;
	pOneKeyDriveCfg->EnableAudio = 1;
	pOneKeyDriveCfg->EnableLight = 1;
}
*/
int ovfs_set_default_cfg()
{
    memset(&g_alarm_cfg, 0, sizeof(g_alarm_cfg));

    g_alarm_cfg.enable = 1;

    g_alarm_cfg.mailcfg.recvList.count = 3;
    g_alarm_cfg.mailcfg.recvList.recver = (OVFS_RECEIVE_INFO*)Common_Malloc(sizeof(OVFS_RECEIVE_INFO)*g_alarm_cfg.mailcfg.recvList.count, 0, __FUNCTION__, __LINE__);
    memset(g_alarm_cfg.mailcfg.recvList.recver,0,sizeof(OVFS_RECEIVE_INFO)*g_alarm_cfg.mailcfg.recvList.count);

    set_commoncfg_default(&g_alarm_cfg.alarmIncfg);

    set_commoncfg_default(&g_alarm_cfg.motioncfg);
    set_commoncfg_default(&g_alarm_cfg.vhidecfg);

    set_commoncfg_default(&g_alarm_cfg.regionalInvasioncfg);
    set_commoncfg_default(&g_alarm_cfg.traverseCfg);
    set_commoncfg_default(&g_alarm_cfg.personStayCfg);
    set_commoncfg_default(&g_alarm_cfg.absentCfg);
    set_commoncfg_default(&g_alarm_cfg.parkViolationCfg);
    set_commoncfg_default(&g_alarm_cfg.vehicleRetrogradeCfg);

    for(int i=0; i<MAX_SENSORALARM; i++)
    {
        set_commoncfg_default(&g_alarm_cfg.sensorAlarm[i]);
    }

    set_commoncfg_default(&g_alarm_cfg.netBreakcfg);
    set_commoncfg_default(&g_alarm_cfg.ipConflitcfg);
    set_commoncfg_default(&g_alarm_cfg.illAccesscfg);

	g_alarm_cfg.oneKeyDriveCfg.Duration = 60;
	g_alarm_cfg.oneKeyDriveCfg.EnableAudio = 1;
	g_alarm_cfg.oneKeyDriveCfg.EnableLight = 1;

    return 0;
}

OVFS_ALARM_CFG *ovfs_get_alarm_cfg()
{
    return &g_alarm_cfg;
}

int ovfs_set_alarm_cfg(OVFS_ALARM_CFG *pAlarmCfg)
{
    int bChange = ovfs_get_cfgChange();
    if(bChange)
        memcpy(&g_alarm_cfg,pAlarmCfg,sizeof(OVFS_ALARM_CFG));
    return 0;
}

int make_alarm_struct(cJSON_Struct *pConfig)
{
    if(!pConfig)
        return -1;
    ovfs_make_arm_struct(pConfig, &g_alarm_cfg);
    ovfs_make_email_struct(pConfig, &g_alarm_cfg);

    FTPSnapCfgStruct2Json(&(g_alarm_cfg.ftpsnapCfg),pConfig,0);

    ovfs_make_common_struct(pConfig, &g_alarm_cfg.motioncfg,(char *)ALARM_STR_MOTION, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.vhidecfg,(char *)ALARM_STR_VHIDE, 1);

    ovfs_make_common_struct(pConfig, &g_alarm_cfg.alarmIncfg,(char *)ALARM_STR_AI, 1);

    ovfs_make_common_struct(pConfig, &g_alarm_cfg.regionalInvasioncfg,(char *)ALARM_STR_REGIONALINVASION, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.traverseCfg,(char *)ALARM_STR_DETWIRE, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.personStayCfg,(char *)ALARM_STR_PERSONSTAYING, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.absentCfg,(char *)ALARM_STR_DETABSENT, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.parkViolationCfg,(char *)ALARM_STR_PARKINGVIOLATION, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.vehicleRetrogradeCfg,(char *)ALARM_STR_RETROGRADE, 1);

    cJSON_Struct *list = Common_Json_GetAttrValueArr(pConfig, ALARM_STR_SENSORALARM);
    for(int i=0; i<MAX_SENSORALARM; i++)
    {
        cJSON_Struct *item = Common_Json_GetAttrValueArrItem(list, i);

        ovfs_make_common_struct(item, &g_alarm_cfg.sensorAlarm[i],(char *)ALARM_STR_SENSORALARM, 0);
    }
    //ovfs_make_common_struct(pConfig, &g_alarm_cfg.sensorAlarm,(char *)ALARM_STR_SENSORALARM, 1);

    ovfs_make_common_struct(pConfig, &g_alarm_cfg.netBreakcfg,(char *)ALARM_STR_NETBREAK, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.ipConflitcfg,(char *)ALARM_STR_IPCONFLICT, 1);
    ovfs_make_common_struct(pConfig, &g_alarm_cfg.illAccesscfg,(char *)ALARM_STR_ILLACCESS, 1);

    ovfs_make_onekeydrive_struct(pConfig, &g_alarm_cfg);
    ovfs_make_pushcfg_struct(pConfig, &g_alarm_cfg);

    return 0;
}

static int ResetDefaultSoundFile()
{
#define DEF_SOUND_DIR "/update/res/default/soundFile"
#define CUR_SOUND_DIR "/update/soundFile"

    DIR * defDir = opendir(DEF_SOUND_DIR);
    DIR * curDir = opendir(CUR_SOUND_DIR);

    char cmdbuff[256];

    if(curDir && (defDir == NULL) )
    {
        memset(cmdbuff,0,sizeof(cmdbuff));
        snprintf(cmdbuff,sizeof(cmdbuff)-1,"cp %s %s -a",CUR_SOUND_DIR,DEF_SOUND_DIR);
        Common_System(cmdbuff);
        LOGD("reset def sound file. cur -> def\n");
    }

    if(curDir && defDir)
    {
        memset(cmdbuff,0,sizeof(cmdbuff));
        snprintf(cmdbuff,sizeof(cmdbuff)-1,"rm -r %s",CUR_SOUND_DIR);
        Common_System(cmdbuff);

        memset(cmdbuff,0,sizeof(cmdbuff));
        snprintf(cmdbuff,sizeof(cmdbuff)-1,"cp %s %s -a",DEF_SOUND_DIR,CUR_SOUND_DIR);
        Common_System(cmdbuff);

        LOGD("reset def sound file. rm cur, def -> cur\n");
    }

    if(defDir)
        closedir(defDir);
    if(curDir)
        closedir(curDir);

    LOGW("reset def sound file done.\n");

    return 0;
}

int ovfs_load_alarm_cfg(ModuleHandle_T hModuleHandle)
{
    int iRet = -1;
    cJSON_Struct *pConfig = NULL;

    memset(&g_alarm_cfg,0,sizeof(OVFS_ALARM_CFG));

    ovfs_set_default_cfg();

    ovfs_get_light_dealy(hModuleHandle);

    LOGD("Load current cfg!\n");
    iRet = Module_LoadConfig(hModuleHandle, &pConfig);
    if(iRet<0)
    {
        LOGD("Load default cfg!\n");
        //ResetDefaultSoundFile();

        Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Default,&pConfig);
        make_alarm_struct(pConfig);
        //ovfs_set_cfgChange(1);
        iRet = ovfs_save_alarm_cfg(hModuleHandle);
        //ovfs_set_cfgChange(0);
    }
    else
    {
        make_alarm_struct(pConfig);
    }

    /*if((access("/usr/etc/cfgfiles/custom_audio",F_OK) != 0))
    {
        if((access("/usr/etc/default/custom_audio",F_OK) == 0))
        {
            Common_System("cp -r /usr/etc/default/custom_audio /usr/etc/cfgfiles/");
        }
    }*/

    if(pConfig)
        Common_Json_Delete(pConfig);
    LOGD("LoadConfig succ!(%d)\n",g_alarm_cfg.enable);
    return iRet;
}

int ovfs_save_alarm_cfg(ModuleHandle_T hModuleHandle)
{
    //char cmd[64];
    int iRet = 0,bChange = 0;
    //char *pAlarmName = NULL;
    cJSON_Struct *pConfig = NULL;

    //bChange = ovfs_get_cfgChange();
    //if(bChange)
    {
        ovfs_set_light_dealy(hModuleHandle);

        ovfs_make_arm_json(&g_alarm_cfg,&pConfig);
//#ifndef SIMPLIFIED
        ovfs_make_email_json(&g_alarm_cfg,&pConfig);
	    FTPSnapCfgStruct2Json(&(g_alarm_cfg.ftpsnapCfg),pConfig,1);
//#endif

        ovfs_make_common_json(&g_alarm_cfg.motioncfg,(char *)ALARM_STR_MOTION,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.vhidecfg,(char *)ALARM_STR_VHIDE,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.alarmIncfg,(char *)ALARM_STR_AI,&pConfig, 1);


        ovfs_make_common_json(&g_alarm_cfg.regionalInvasioncfg,(char *)ALARM_STR_REGIONALINVASION,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.traverseCfg,(char *)ALARM_STR_DETWIRE,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.personStayCfg,(char *)ALARM_STR_PERSONSTAYING,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.absentCfg,(char *)ALARM_STR_DETABSENT,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.parkViolationCfg,(char *)ALARM_STR_PARKINGVIOLATION,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.vehicleRetrogradeCfg,(char *)ALARM_STR_RETROGRADE,&pConfig, 1);

        cJSON_Struct *list = Common_Json_SetAttrValueArr(pConfig, ALARM_STR_SENSORALARM);
        for(int i=0; i<MAX_SENSORALARM; i++)
        {
            cJSON_Struct *item = Common_Json_SetAttrValueArrObj(list, i);

            ovfs_make_common_json(&g_alarm_cfg.sensorAlarm[i],(char *)ALARM_STR_SENSORALARM,&item, 0);
        }

        ovfs_make_common_json(&g_alarm_cfg.netBreakcfg,(char *)ALARM_STR_NETBREAK,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.ipConflitcfg,(char *)ALARM_STR_IPCONFLICT,&pConfig, 1);
        ovfs_make_common_json(&g_alarm_cfg.illAccesscfg,(char *)ALARM_STR_ILLACCESS,&pConfig, 1);

        ovfs_make_onekeydrive_json(&g_alarm_cfg, &pConfig);
        ovfs_make_pushcfg_json(&g_alarm_cfg, &pConfig);

        iRet = Module_SaveConfig(hModuleHandle, pConfig);
        if(pConfig)
            Common_Json_Delete(pConfig);
        //ovfs_set_cfgChange(0);
        if(iRet < 0)
        {
            LOGE("save cfg failed!\n");
        }
    }
    return iRet;
}

#if 0//ndef SIMPLIFIED
int ovfs_load_alarm_custom(char *filePath)
{
    int fd=0,value=0;
    char buf[1024];
    cJSON_Struct *customJs = NULL, *tempJs = NULL;
    memset(buf, 0, sizeof(buf));
    if(NULL == filePath)
    {
        LOGW("filePath is null\n");
        return -1;
    }
    if(0 == access(filePath, F_OK))
    {
        if( (fd = open(filePath, O_RDONLY)) < 0)
        {
            LOGE("Open %s failed\n", filePath);
            return -1;
        }
        if(read(fd, buf, sizeof(buf) -1) < 0)
        {
            LOGE("Read file failed\n");
            return -1;
        }
        customJs = Common_Json_Parse(buf, NULL, NULL);
        if(NULL == customJs)
        {
            LOGE("CustomJson is null\n");
            return -1;
        }
        tempJs = Common_Json_GetAttrValue(customJs, -1, "DetPersonComp", NULL, NULL, &value, NULL);
        if(tempJs)
        {
            g_alarm_cfg.customcfg.detPersonComp = value;
            if(g_alarm_cfg.customcfg.detPersonComp)
                LOGW("DetPersonComp!!!!\n");
        }

        tempJs = Common_Json_GetAttrValue(customJs, -1, "RegionRename", NULL, NULL, &value, NULL);
        if(tempJs)
        {
            g_alarm_cfg.customcfg.regionRename = value;
            if(g_alarm_cfg.customcfg.regionRename)
                LOGW("RegionRename=%d!!!!\n",g_alarm_cfg.customcfg.regionRename);
        }

        tempJs = Common_Json_GetAttrValue(customJs, -1, "ReLinkAudio", NULL, NULL, &value, NULL);
        if(tempJs)
        {
            g_alarm_cfg.customcfg.relinkAudio = value;
            if(g_alarm_cfg.customcfg.relinkAudio)
                LOGW("relinkAudio=%d\n",g_alarm_cfg.customcfg.relinkAudio);
        }

        tempJs = NULL;
        if(customJs)
        {
            Common_Free(customJs, NULL, 0);
            customJs = NULL;
        }
    }
    else
        LOGW("%s doesn't exit\n", filePath);
    return 0;
}
#endif
