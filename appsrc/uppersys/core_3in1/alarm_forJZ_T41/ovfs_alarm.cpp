/*
 * ovfs_alarm.c
 *
 *  Created on: 2017-3-07
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
#include "ovfs_alarm_method.h"
#include "ovfs_linkage.h"
#include "ovfs_alarm.h"
#include "cjson.h"

static OVFS_ABILITY *g_ability = NULL;
static OVFS_COMMON_LIST_T g_subscribeList;

extern int oper_snap(ModuleHandle_T hModuleHandle,int iDev,int iChan,OVFS_ALARM_EVENT *pAlarmEvent,S8 **pOutPath);

S32 SeparateExpression(S8 *pExpression,S8 **pString,S8 **pValue)
{
    int i = 0;
    S8 pCurChar = 0;

    if(!pExpression)
        return -1;
    while(1)
    {
        pCurChar = pExpression[i];
        if(pCurChar == '=')
        {
            if(pValue)
                *pValue = pExpression+i+1;
            if(pString)
            {
                *pString = (S8 *)Common_Malloc(i+1, 0, __FUNCTION__, __LINE__);
                memcpy(*pString,pExpression,i);
                (*pString)[i] = '\0';
            }
            break;
        }
        else if(pCurChar=='\0')
        {
            if(pString)
            {
                *pString = (S8 *)Common_Malloc(i+1, 0, __FUNCTION__, __LINE__);
                memcpy(*pString,pExpression,i);
                (*pString)[i] = '\0';
            }
            break;
        }
        i++;
    }
    return 0;
}

cJSON_Struct *SeparateCondition(S8 *pCondition)
{
    S8 delim[] = "&&";
    S8 *pSrc =  NULL;
    cJSON_Struct *pJCondition = NULL;
    S8 *pNext =  NULL,*pTemp = NULL;
    S8 *pString = NULL,*pValue = NULL;

    if(!pCondition)
        return NULL;
    pSrc = Common_StrDup(pCondition, __FUNCTION__, __LINE__);
    pTemp = strtok_r(pSrc, delim, &pNext);
    pJCondition = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    while(1)
    {
        if(pTemp)
        {
            if (pJCondition)
            {
                SeparateExpression(pTemp,&pString,&pValue);
                Common_Json_SetAttrValue(pJCondition,-1,pString,Common_Json_Type_String,pValue,0,0);
            }
            pTemp = strtok_r(NULL, delim, &pNext);
            if(pString)
            {
                Common_Free(pString, __FUNCTION__, __LINE__);
                pString = NULL;
            }
        }
        else
        {
            break;
        }
    }
    if(pSrc)
        Common_Free(pSrc, __FUNCTION__, __LINE__);
    //if(pJCondition)
    //	ovfs_print_json(pJCondition);
    return pJCondition;
}

cJSON_Struct *SeparateUriAndCondition(S8 *pSrcUri,S8 **pDstUri)
{
    int i = 0,iCount = 0;
    S8 pCurChar = 0;
    S8 *pCondition = NULL;

    if(!pSrcUri)
        return NULL;
    while(pSrcUri[i]!='\0')
    {
        if(pSrcUri[i]=='?')
            iCount++;
        i++;
    }
    if(iCount>1)
    {
        *pDstUri = NULL;
        return NULL;
    }
    i = 0;
    while(1)
    {
        pCurChar = pSrcUri[i];
        if(pCurChar=='?')
        {
            pCondition = pSrcUri+i+1;
            if(pDstUri)
            {
                *pDstUri = (S8 *)Common_Malloc(i+1, 0, __FUNCTION__, __LINE__);
                memcpy(*pDstUri,pSrcUri,i);
                (*pDstUri)[i] = '\0';
            }
            break;
        }
        else if(pCurChar=='\0')
        {
            if(pDstUri)
            {
                *pDstUri = (S8 *)Common_Malloc(i+1, 0, __FUNCTION__, __LINE__);
                memcpy(*pDstUri,pSrcUri,i);
                (*pDstUri)[i] = '\0';
            }
            break;
        }
        i++;
    }
    //LOGI("\n");
    return SeparateCondition(pCondition);
}

static S32 AnalyzeUriAndMakeResult(S32 method,char* pUri,cJSON_Struct *pAddData,cJSON_Struct **ppResult)
{
    S8 pResPath[128] = {0};
    OVFS_REST_METHOD *pMethod = NULL;
    S8 *pTemp = NULL,*pSrcUri = NULL,*pFirst = NULL,*pNext = NULL;
    cJSON_Struct *pRoot = ovfs_get_rest(),*pChild = NULL,*pJCondition = NULL;

    if(!pRoot)
    {
        LOGW("g_rest_res is null\n");
        return -1;
    }
    pJCondition = SeparateUriAndCondition(pUri, &pSrcUri);
    pTemp = pSrcUri;
    while(pTemp)
    {
        Common_UriOneParse(pTemp,NULL,&pFirst,&pNext);
        if(pFirst)
        {
            sprintf(pResPath,"%s/%s",pResPath,pFirst);
            if(pNext)
            {
                pTemp = pNext;
            }
            else
            {
                pChild = Common_Json_GetItem(pRoot,-1,pResPath);
                if(pChild)
                {
                    //LOGI("pResPath:%s,pFirst:%s,method:%d\n",pResPath,pFirst,method);
                    pMethod = (OVFS_REST_METHOD *)Common_Json_GetItemExtData(pChild,NULL);
                    if(!pMethod)
                    {
                        break;
                    }
                    if((MATHOD_GET==method)&&pMethod->ovfs_get_method)
                        pMethod->ovfs_get_method(pResPath, pAddData,pJCondition,ppResult);
                    if((MATHOD_PUT==method)&&pMethod->ovfs_put_method)
                        pMethod->ovfs_put_method(pResPath, pAddData,pJCondition,ppResult);
                    if((MATHOD_POST==method)&&pMethod->ovfs_post_method)
                        pMethod->ovfs_post_method(pResPath, pAddData,pJCondition,ppResult);
                    if((MATHOD_DELETE==method)&&pMethod->ovfs_delete_method)
                        pMethod->ovfs_delete_method(pResPath, pAddData,pJCondition,ppResult);
                }
                break;
            }
            if(pFirst != NULL)
            {
                Common_Free(pFirst, __FUNCTION__, __LINE__);
                pFirst = NULL;
            }
            pNext = NULL;
        }
        else
        {
            break;
        }
    }
    if(pFirst != NULL)
    {
        Common_Free(pFirst, __FUNCTION__, __LINE__);
        pFirst = NULL;
    }
    if(pSrcUri)
        Common_Free(pSrcUri, __FUNCTION__, __LINE__);
    if(pJCondition)
        Common_Json_Delete(pJCondition);
    return 0;
}

int AnalyzeSubscribeUriAndSend(ModuleHandle_T hModuleHandle,S32 nRecvID,void *pUriData,void *pCondition,cJSON_Struct **pOutParams)
{
    cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;

    if(!pOutParams)
    {
        LOGE("pOutParams is null\n");
        return -1;
    }

    *pOutParams = ovfs_get_alarm_status(pJCondition);
    if(!(*pOutParams))
    {
        LOGI("pOutParams is null!\n");
    }
    return 0;
}

int findAlarmType(char *AlarmName)
{
	int ret = -1;
    int i = 0;

    if(NULL == AlarmName)
	{
		return -1;
	}

    for(i=0; i<ARRAYSIZE(g_alarmName); i++)
    {
        if(strcmp(AlarmName,g_alarmName[i]) == 0 )
        {
            return g_alarmIndex[i];
        }
    }

	return ret;
}


int getChanIndexByDevChan(int iType,int iDev,int iChan)
{
    int iChanIndex = 0;
    OVFS_ABILITY * pAbility = ovfs_get_ability();

    if(!pAbility)
        return -1;

    if(0 == iType)
    {
        iChanIndex = pAbility->alarmInNum;
    }
    else if(ALARM_TYPE_SENSOR_ALARM == iType)
    {
        return iChan;
    }
    else
    {
        iChanIndex = MAX_CHANNEL_SUPPORT;//pAbility->totalChanNum;
    }

    return (iChanIndex-1);
}

int isSmartAlarm(int iAlarmType)
{
    int isSmart = 0;
    for(int i=0; i<ARRAYSIZE(g_alarmIndex_smart); i++)
    {
        if(g_alarmIndex_smart[i] == iAlarmType )
        {
            isSmart = 1;
            break;
        }
    }

    return isSmart;
}

int WriteAlarmLog(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S32 nRet = -1,nCount = 0;
    time_t tlogTime = 0;
    Common_Time_T t_happent;
    cJSON_Struct *pConfig = NULL,*pChild = NULL,*pOutParams = NULL;

    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if(pConfig)
    {
        Common_Json_SetAttrValueObj(pConfig, "Header");
        Common_Json_SetAttrValueStr(pConfig, "Header/Uri", "/EventLog/AlarmFunction");
        Common_Json_SetAttrValueStr(pConfig, "Header/Method", "put");
        pChild = Common_Json_SetAttrValueObj(pConfig, "Data");
        if(pAlarmEvent->iState)
        {
            Common_Json_SetAttrValueInt(pChild, "AlarmType", pAlarmEvent->iType);
            Common_Common2LinuxTime(&pAlarmEvent->t_start,(time_t*)&tlogTime);
            Common_Json_SetAttrValueInt(pChild, "StartTime", tlogTime);


            int isSmart = isSmartAlarm(pAlarmEvent->iType);
            LOGW("isSmart:[%d] File_IsExist:[%d]\n",isSmart,Common_File_IsExist("/tmp/smartsnap/start.jpg"));
            if(isSmart)
            {
                if(Common_File_IsExist("/tmp/smartsnap/start.jpg"))
                {
                    Common_Json_SetAttrValueStr(pChild, "AlarmPic", "/tmp/smartsnap/start.jpg");
                }
            }
            else
            {
                S8 *pTemp = NULL;
                oper_snap(hModuleHandle, 0, 0, pAlarmEvent, &pTemp);
                if(pTemp)
                {
                    Common_Json_SetAttrValueStr(pChild, "AlarmPic", pTemp);
                    Common_Free(pTemp,__FUNCTION__,__LINE__);
                }

                if(pAlarmEvent->iType == ALARM_TYPE_SENSOR_ALARM)
                {
                    char strTmp[128] = {0};
                    snprintf(strTmp, sizeof(strTmp), "%d,%s", pAlarmEvent->iChan+1, pAlarmEvent->sUser);
                    Common_Json_SetAttrValueStr(pChild, "ExtraInfo", strTmp);
                }

            }
        }
        else
        {
            if(pAlarmEvent->iLogId)
            {
                Common_Json_SetAttrValueInt(pChild, "LogId", pAlarmEvent->iLogId);
                Common_Common2LinuxTime(&pAlarmEvent->t_stop,(time_t*)&tlogTime);
                Common_Json_SetAttrValueInt(pChild, "EndTime", tlogTime);
                pAlarmEvent->iLogId = 0;
            }
        }

        nRet = Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
        if(pAlarmEvent->iState)
        {
            Common_Json_GetAttrValueInt(pOutParams, "Data/LogId", &pAlarmEvent->iLogId);
            LOGD("LogId:[%d]\n",pAlarmEvent->iLogId);
        }
        ovfs_print_json(pConfig);
        ovfs_print_json(pOutParams);
        Common_Json_Delete(pConfig);
        Common_Json_Delete(pOutParams);
        LOGW("nRet:[%d]\n",nRet);
        pConfig = NULL;
        pOutParams = NULL;
    }

    return nRet;
}

S32 Thread_CheckAlarmLog(Common_Thread_T hThreadHandle,void *pUserData)
{
    OVFS_ALARM_EVENT *pAlarmEvent = (OVFS_ALARM_EVENT *)pUserData;

    if(!pAlarmEvent)
    {
        return -1;
    }

    while(1)
    {
        if(Common_File_IsExist("/tmp/smartsnap/start.jpg"))
        {
            WriteAlarmLog(g_subscribeList.hModuleHandle, pAlarmEvent);
            break;
        }

        Common_Sleep(0, 100*1000);
    }
}

int ovfs_writeAlarm2Log(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent)
{
    if(!pAlarmEvent)
    {
        return -1;
    }

    int isSmart = isSmartAlarm(pAlarmEvent->iType);
    if(isSmart && pAlarmEvent->iState)
    {
        Common_Thread_T pcheckAlarmLogThread = NULL;
        Common_Thread_Create(&pcheckAlarmLogThread,__FUNCTION__,0,0,Thread_CheckAlarmLog, pAlarmEvent);
        Common_Thread_Detach(pcheckAlarmLogThread);
        return 0;
    }

    return WriteAlarmLog(hModuleHandle, pAlarmEvent);
}

int checkSchedTime(OVFS_ALARM_EVENT  *pAlarmEvent,Common_Time_T t_happent)
{
    U32 t_start;
    S8 *pAlarmName = NULL;
    int i = 0,j = 0,iDev = 0,iChan = 0;
    OVFS_ALARM_CFG *alarmCfg = ovfs_get_alarm_cfg();
    OVFS_SCHEDTIME_TIME_T *pSched = NULL;

    if(!pAlarmEvent)
    {
        return 0;
    }
    if(!pAlarmEvent->sName)
    {
        return 0;
    }
    if(!alarmCfg->enable)
    {
        return 0;
    }

    iDev = pAlarmEvent->iDev;
    iChan = pAlarmEvent->iChan;
    pAlarmName = pAlarmEvent->sName;

    if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_AI))
    {
        pSched = &alarmCfg->alarmIncfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_MOTION))
    {
        pSched = &alarmCfg->motioncfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_VHIDE))
    {
        pSched = &alarmCfg->vhidecfg.linkage.shedule;
    }
    /*else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_DETPERSON))
    {
        pSched = &alarmCfg->PersonDetcfg.linkage.shedule;
    }*/
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_REGIONALINVASION))
    {
        pSched = &alarmCfg->regionalInvasioncfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_DETWIRE))
    {
        pSched = &alarmCfg->traverseCfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_PERSONSTAYING))
    {
        pSched = &alarmCfg->personStayCfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_DETABSENT))
    {
        pSched = &alarmCfg->absentCfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_PARKINGVIOLATION))
    {
        pSched = &alarmCfg->parkViolationCfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_RETROGRADE))
    {
        pSched = &alarmCfg->vehicleRetrogradeCfg.linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_SENSORALARM))
    {
        pSched = &alarmCfg->sensorAlarm[pAlarmEvent->iChan].linkage.shedule;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_NETBREAK))
    {
        //pSched = &alarmCfg.netBreakcfg.cfgTri[iDev][iChan].shedule;
        return 1;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_IPCONFLICT))
    {
        //pSched = &alarmCfg.ipConflitcfg.cfgTri[iDev][iChan].shedule;
        return 1;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_ILLACCESS))
    {
        //pSched = &alarmCfg.illAccesscfg.cfgTri[iDev][iChan].shedule;
        return 1;
    }
    else if(0 == Common_StriCmp(pAlarmName, (S8*)ALARM_STR_LOWBATTERYALARM))
    {
        //pSched = &alarmCfg.illAccesscfg.cfgTri[iDev][iChan].shedule;
        return 1;
    }
    else
    {
        return 0;
    }

    if(pSched == NULL)
    {
        return 0;
    }
    t_start = t_happent.hour*100+t_happent.min;
    if(pSched->mode == 1)
    {
        return 1;
    }
    else if(pSched->mode == 2)
    {
        if(t_start >= 1800 || t_start <= 600)
        {
            return 1;
        }
    }
    else if(pSched->mode == 3)
    {
        i = t_happent.wday == 0 ? 6 : t_happent.wday - 1;

        if(pSched->weeklist[i])
        {
            for(j = 0; j < MAX_TIMESEGMENT; j++)
            {
                if((0 == pSched->depSchedTime[j].startTime) && (0 == pSched->depSchedTime[j].stopTime))
                {
                    continue;
                }
                if((t_start >= pSched->depSchedTime[j].startTime) && (t_start <= pSched->depSchedTime[j].stopTime))
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int ovfs_report_alarm(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent)
{
    S8 strcmd[128] = {0};
    OVFS_SUBSCRIBE_NODE *node = NULL;
    S32 i = 0,j = 0,iRet = -1,iCount = 0,nCount = 0;
    cJSON_Struct *pResult = NULL,*pRoot = NULL;

    if(!pAlarmEvent)
    {
        LOGE("pAlarmEvent is null!\n");
        return -1;
    }
    nCount = 1;
    for(j = 0; j < nCount; j++)
    {
        S32 iState = 0,iRegion = 0;
        Common_Time_T t_start,t_stop;

        memset(&t_start,0,sizeof(t_start));

        iRegion = 0;
        iState = pAlarmEvent->iState;
        t_start = pAlarmEvent->t_start;
        t_stop = pAlarmEvent->t_stop;

        pRoot = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        if (pRoot)
        {
            //Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
            //Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
            //Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
            Common_Json_SetAttrValue(pRoot,-1,"AlarmName",Common_Json_Type_String,pAlarmEvent->sName,0,0);
            Common_Json_SetAttrValue(pRoot,-1,"AlarmType",Common_Json_Type_Number,NULL,pAlarmEvent->iType,0);
            Common_Json_SetAttrValue(pRoot,-1,"AlarmSrcType",Common_Json_Type_Number,NULL,pAlarmEvent->iSrcType,0);
            Common_Json_SetAttrValue(pRoot,-1,"DevName",Common_Json_Type_String,pAlarmEvent->sDevName,0,0);
            Common_Json_SetAttrValue(pRoot,-1,"Device",Common_Json_Type_Number,NULL,pAlarmEvent->iDev,0);
            Common_Json_SetAttrValue(pRoot,-1,"Channel",Common_Json_Type_Number,NULL,pAlarmEvent->iChan,0);
            Common_Json_SetAttrValue(pRoot,-1,"Stream",Common_Json_Type_Number,NULL,pAlarmEvent->iStream,0);
            Common_Json_SetAttrValue(pRoot,-1,"RegionId",Common_Json_Type_Number,NULL,iRegion,0);
            Common_Json_SetAttrValue(pRoot,-1,"Status",Common_Json_Type_Number,NULL,iState,0);
#if 0
            if(t_start.year < 1970)
                t_start = t_stop;
            if(t_stop.year < 1970)
                t_stop = t_start;
#endif
            memset(strcmd,0,sizeof(strcmd));
            snprintf(strcmd, sizeof(strcmd), "%04d%02d%02d%02d%02d%02d",t_start.year,t_start.month,t_start.day,t_start.hour,t_start.min,t_start.sec);
            Common_Json_SetAttrValue(pRoot,-1,"StartTime",Common_Json_Type_String,strcmd,0,0);
            memset(strcmd,0,sizeof(strcmd));
            snprintf(strcmd, sizeof(strcmd), "%04d%02d%02d%02d%02d%02d",t_stop.year,t_stop.month,t_stop.day,t_stop.hour,t_stop.min,t_stop.sec);
            Common_Json_SetAttrValue(pRoot,-1,"StopTime",Common_Json_Type_String,strcmd,0,0);
        }
        else
        {
            LOGE("pRoot is null!\n");
            return -1;
        }
        iCount= Common_DList_GetCount(g_subscribeList.listdl);
        //LOGD("subscribeList iCount:%d\n",iCount);
        for(i = 0; i < iCount; i++)
        {
            node= (OVFS_SUBSCRIBE_NODE *)Common_DList_GetNode(g_subscribeList.listdl, i);
            if(!node)
            {
                continue;
            }

            iRet = FilterCondithon(pRoot, -1, node->pCondition);
            if(iRet != 0)
            {
                continue;
            }
			//ovfs_print_json(pResult);
            Module_SendEvent(hModuleHandle, node->nRecvID, pRoot, &pResult, 3000);
            Common_Json_Delete(pResult);
            pResult = NULL;
        }
        Common_Json_Delete(pRoot);
        pRoot = NULL;
    }

    return 0;
}

#if 0
S32 Thread_AlarmLinkage(Common_Thread_T hThreadHandle,void *pUserData)
{
    OVFS_ALARM_EVENT alarmEvent = *(OVFS_ALARM_EVENT *)pUserData;

    ovfs_linkOutput(g_subscribeList.hModuleHandle,&alarmEvent);
    return 0;
}
#endif

void freeSubscribeListNode(void * data)
{
    OVFS_SUBSCRIBE_NODE* node = (OVFS_SUBSCRIBE_NODE*)data;
    Common_Free(node->szSubscribeUri,__FUNCTION__,__LINE__);
    Common_Free(node->szValidUri,__FUNCTION__,__LINE__);
    Common_Json_Delete((cJSON_Struct *)node->pCondition);
    Common_Free(node,__FUNCTION__,__LINE__);
    return;
}

int insertSubscribeListNode(void * data)
{
    int iRet = -1;
    OVFS_SUBSCRIBE_NODE* node = (OVFS_SUBSCRIBE_NODE*)data;
    iRet = Common_DList_InsertTail(g_subscribeList.listdl, node, sizeof(OVFS_SUBSCRIBE_NODE));
    if(iRet < 0)
    {
        Common_Free(node->szSubscribeUri,__FUNCTION__,__LINE__);
        Common_Free(node->szValidUri,__FUNCTION__,__LINE__);
        Common_Json_Delete((cJSON_Struct *)node->pCondition);
        Common_Free(node,__FUNCTION__,__LINE__);
    }
    LOGD("insert node iCount = %d \n",Common_DList_GetCount(g_subscribeList.listdl));
    return iRet;
}

S32 findSubscribeListNode(void * a, void *b)
{

    OVFS_SUBSCRIBE_NODE* node = (OVFS_SUBSCRIBE_NODE*)a;
    OVFS_SUBSCRIBE_NODE *p = (OVFS_SUBSCRIBE_NODE *)b;
    if(!node)
        return -1;
    if(0 == Common_StriCmp(node->szSubscribeUri,p->szSubscribeUri)&&(node->nRecvID==p->nRecvID))
    {
        return 0;
    }
    return -1;
}

static S32 add_subscribeList_fxn(ModuleHandle_T hModuleHandle,S32 nType /* 0-subscribe,1-unsubscribe,2-QueryEvent*/,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData)
{
    int iRet = -1;
    S8 *pUri = NULL;
    void *p = NULL;
    cJSON_Struct *pJCondition = NULL;
    OVFS_SUBSCRIBE_NODE node;
    OVFS_SUBSCRIBE_NODE* pNode = NULL;

    memset(&node,0,sizeof(OVFS_SUBSCRIBE_NODE));
    if(2 == nType)
    {
        node.nRecvID = nRecvID;
        node.szSubscribeUri = szSubscribeUri;
        pNode = (OVFS_SUBSCRIBE_NODE*)Common_DList_Search(g_subscribeList.listdl, (void*)&node, findSubscribeListNode);
        iRet = AnalyzeSubscribeUriAndSend(hModuleHandle,pNode->nRecvID,pNode->szSubscribeUri,pNode->pCondition,pQueryEventInfo);
        return iRet;
    }
    else if(1 == nType)
    {
        node.nRecvID = nRecvID;
        node.szSubscribeUri = szSubscribeUri;
        return Common_DList_Delete(g_subscribeList.listdl, (void*)&node, findSubscribeListNode);
    }
    else if(0 == nType)
    {
        LOGD("[%s]:[%d ,%d]\n",szSubscribeUri,nType,nRecvID);
        node.nRecvID = nRecvID;
        node.szSubscribeUri = szSubscribeUri;
        p = Common_DList_Search(g_subscribeList.listdl,(void*)&node,findSubscribeListNode);
        if(p)
        {
            LOGD("exist subscribe node!");
            return 0;
        }
        pNode = (OVFS_SUBSCRIBE_NODE*)Common_Malloc(sizeof(OVFS_SUBSCRIBE_NODE), 0, __FUNCTION__, __LINE__);
        pJCondition = SeparateUriAndCondition(szSubscribeUri, &pUri);
        pNode->nRecvID = nRecvID;
        pNode->szSubscribeUri = Common_StrDup(szSubscribeUri,__FUNCTION__,__LINE__);
        pNode->pCondition = pJCondition;
        pNode->szValidUri = pUri;
        insertSubscribeListNode(pNode);
        iRet = AnalyzeSubscribeUriAndSend(hModuleHandle,nRecvID,pUri,pJCondition,pQueryEventInfo);
        return iRet;
    }
    return 0;
}
#if 0
static S32 recv_mediaEvents_fxn(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
#if 0
    if (pEventInfo)
    {
        S8 *pRemoteIP = NULL;
        Common_Time_T t_happent;
        OVFS_ALARM_EVENT *pAlarmEvent = NULL;
        cJSON_Struct *pJData = pEventInfo,*pItem = NULL;
        S32 iDev = 0,iChan = 0,iStream = 0,iChanIndex = 0;
        S32 bArming = 0,iState = -1,bEnable = 0,bPrintDbg = 0;
        OVFS_ALARM_BKBD *pAlarmBkbd = ovfs_get_alarm_bkbd();

        if(!pAlarmBkbd)
        {
            LOGE("pAlarmBkbd is null\n");
            return 0;
        }
        bPrintDbg =ovfs_get_debug();
        if(bPrintDbg)
        {
            ovfs_print_json(pEventInfo);
        }
        Common_Lock(pAlarmBkbd->alarmLock);
        bArming = ovfs_get_arming_state();
        /*
        if(!bArming)
        {
        	Common_UnLock(pAlarmBkbd->alarmLock);
        	LOGD("bArming:%d\n",bArming);
        	return 0;
        }
        */
        pItem = Common_Json_GetItem(pJData,-1,"Data/MediaDisconnect");
        if(!pItem)
        {
            Common_UnLock(pAlarmBkbd->alarmLock);
            LOGD("recv data error\n");
            return 0;
        }
        Common_Json_GetAttrValue(pItem,-1,"DisconnectIP",NULL,&pRemoteIP,NULL,NULL);
        Common_Json_GetAttrValue(pItem,-1,"DeviceId",NULL,NULL,&iDev,NULL);
        Common_Json_GetAttrValue(pItem,-1,"ChannelId",NULL,NULL,&iChan,NULL);
        Common_Json_GetAttrValue(pItem,-1,"StreamId",NULL,NULL,&iStream,NULL);
        Common_Json_GetAttrValue(pItem,-1,"State",NULL,NULL,&iState,NULL);

        OVFS_ALARM_EVENT *pTempEvent = NULL;
        pTempEvent =  pAlarmBkbd->pAlarmEvent[ALARM_TYPE_CLIENT_BREAK];
        if(!pTempEvent)
        {
            Common_UnLock(pAlarmBkbd->alarmLock);
            return -1;
        }
        iChanIndex = getChanIndexByDevChan(ALARM_TYPE_CLIENT_BREAK,iDev, iChan);
        if(iChanIndex < 0)
        {
            Common_UnLock(pAlarmBkbd->alarmLock);
            return -1;
        }
        pAlarmEvent = &pTempEvent[iChanIndex];
        if(pAlarmEvent)
        {
            pAlarmEvent->iDev = iDev;
            pAlarmEvent->iChan = iChan;
        }
        if(pRemoteIP)
            snprintf(pAlarmEvent->sRemoteIP, sizeof(pAlarmEvent->sRemoteIP), "%s",pRemoteIP);
        Common_Linux2CommonTime(time(NULL),&t_happent);
        bEnable = checkSchedTime(pAlarmEvent,t_happent);
        pAlarmEvent->iSysState = iState;

        if(bEnable)
        {
            pAlarmEvent->iState = pAlarmEvent->iSysState;
        }
        else
        {
            pAlarmEvent->iLstState = pAlarmEvent->iState;
            pAlarmEvent->iState = 0;
            LOGD("not recv alarm for sched time or alarm state[%s_dev%d_ch%d]\n",pAlarmEvent->sName,pAlarmEvent->iDev,pAlarmEvent->iChan);
        }
        if((pAlarmEvent->iState != pAlarmEvent->iLstState))
        {
            LOGD("[%s %d %d %d] curStatus:%d,lstStatus:%d,iSysState:%d\n",pAlarmEvent->sName,pAlarmEvent->iDev,pAlarmEvent->iChan,pAlarmEvent->iRegion,pAlarmEvent->iState,pAlarmEvent->iLstState,pAlarmEvent->iSysState);
            //Common_Thread_T t_Thread = NULL;
            if(bEnable ||((!bEnable) && pAlarmEvent->bArming))
            {
                if(iState)
                    pAlarmEvent->t_start = t_happent;
                else
                    pAlarmEvent->t_stop = t_happent;

                if(bArming)
                {
                    OVFS_ALARM_EVENT ev = {};
                    memcpy(&ev, pAlarmEvent,sizeof(OVFS_ALARM_EVENT));
                    Common_UnLock(pAlarmBkbd->alarmLock);

                    ovfs_writeAlarm2Log(hModuleHandle,&ev);
                    ovfs_linkOutput(hModuleHandle,&ev);
                    ovfs_report_alarm(hModuleHandle,&ev);
                    Common_Lock(pAlarmBkbd->alarmLock);
                }

                //Common_Thread_Create(&t_Thread,__FUNCTION__,0,2,Thread_AlarmLinkage,(void*)pAlarmEvent);
                pAlarmEvent->iLstState = pAlarmEvent->iState;
                pAlarmEvent->bArming = bEnable;
            }
        }
        Common_UnLock(pAlarmBkbd->alarmLock);
    }
#endif
    return 0;
}
#endif

static S32 recv_alarmEvents_fxn(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
    if (pEventInfo != NULL)
    {
        Common_Time_T t_happent;
        S32 iState = -1,bEnable = 0,bPrintDbg = 0,needLink = 0;		//布防状态bEnable
        S8 *pAlarmName = NULL,*pDevName = NULL,*pRemoteIP = NULL,*pUserName = NULL;
        S32 iIndex= 0,i = 0,iObjType = -1,iSize = 0,iDev = 0,iChan = 0,iChanIndex = 0;
        OVFS_ALARM_EVENT *pAlarmEvent = NULL;
        cJSON_Struct *pJData = pEventInfo,*pArray = NULL;
        OVFS_ALARM_BKBD *pAlarmBkbd = ovfs_get_alarm_bkbd();
        //OVFS_ALARM_CFG  *alarmCfg = ovfs_get_alarm_cfg();
        S32 bArming = ovfs_get_arming_state();			//OVFS_ALARM_CFG enable ?此处存疑，一直为1，有意义么？

        if(!pAlarmBkbd)
        {
            LOGE("pAlarmBkbd is null\n");
            return 0;
        }
        Common_Lock(pAlarmBkbd->alarmLock);

        bPrintDbg =ovfs_get_debug();
        if(bPrintDbg)
        {
            ovfs_print_json(pJData);
        }

        if(NULL == (pArray = Common_Json_GetItem(pJData,-1,"ResList")))
        {
            Common_UnLock(pAlarmBkbd->alarmLock);
            ovfs_print_json(pJData);
            LOGE("no ResList\n");
            return -1;
        }
        //ovfs_print_json(pJData);
        pJData= pArray;
        if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
        {
            Common_UnLock(pAlarmBkbd->alarmLock);
            ovfs_print_json(pJData);
            LOGE("no iObjType\n");
            return -1;
        }
        if(iObjType!=Common_Json_Type_Array)
        {
            Common_UnLock(pAlarmBkbd->alarmLock);
            ovfs_print_json(pJData);
            LOGE("no array\n");
            return -1;
        }
        iSize= Common_Json_Size(pJData);
        for(iIndex = 0; iIndex < iSize; iIndex++)
        {
            iDev = 0,iChan = 0;
            needLink = 0;
			pAlarmEvent = NULL;

            if(NULL == Common_Json_GetAttrValue(pJData,iIndex,"AlarmName",NULL,&pAlarmName,NULL,NULL))
            {
                ovfs_print_json(pJData);
                LOGE("no AlarmName\n");
                continue;
            }
			int alarmType = findAlarmType(pAlarmName);
            //if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_ILLACCESS)) return -1;
            Common_Json_GetAttrValue(pJData, iIndex, "Device",NULL, NULL, &iDev, NULL);
            Common_Json_GetAttrValue(pJData, iIndex, "Channel",NULL, NULL, &iChan, NULL);
            for(i = 0; i < ARRAYSIZE(g_alarmName); i++)
            {
                if(0 == Common_StriCmp(pAlarmName,(S8*)ALARM_STR_ILLACCESS)) continue;
                if(0 == Common_StriCmp(pAlarmName,(S8*)g_alarmName[i]))
                {
                    OVFS_ALARM_EVENT *pTempEvent = NULL;
                    pTempEvent =  pAlarmBkbd->pAlarmEvent[i];
                    if(!pTempEvent)
                        continue;
                    iChanIndex = getChanIndexByDevChan(alarmType,iDev, iChan);
                    if(iChanIndex < 0)
                    {
                        ovfs_print_json(pJData);
                        continue;
                    }
                    pAlarmEvent = &pTempEvent[iChanIndex];
                    if(pAlarmEvent)
                    {
                        pAlarmEvent->iDev = iDev;
                        pAlarmEvent->iChan = iChan;
                        break;
                    }
                }
            }
			if(i >= ARRAYSIZE(g_alarmName))
			{
				//LOGE("unknown AlarmName[%s]\n",pAlarmName);
				continue;
			}
            if(!pAlarmEvent)
            {
				LOGE("pAlarmEvent is null AlarmName[%s]\n",pAlarmName);
                continue;
            }
            pAlarmEvent->iType = alarmType;
            if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "DevName",NULL, &pDevName, NULL, NULL))
            {
                pDevName = NULL;
            }
            if(pDevName&&(!pAlarmEvent->sDevName))
            {
                pAlarmEvent->sDevName = Common_StrDup(pDevName, __FUNCTION__, __LINE__);
            }

            pDevName = NULL;
            if(Common_Json_GetAttrValue(pJData, iIndex, "SubName",NULL, &pDevName, NULL, NULL))
            {
                snprintf(pAlarmEvent->sUser,sizeof(pAlarmEvent->sUser),"%s",pDevName);
            }
            Common_Json_GetAttrValue(pJData, iIndex, "RemoteIP",NULL, &pRemoteIP, NULL, NULL);
            if(pRemoteIP)
                snprintf(pAlarmEvent->sRemoteIP,sizeof(pAlarmEvent->sRemoteIP),"%s",pRemoteIP);
            Common_Json_GetAttrValue(pJData, iIndex, "UserName",NULL, &pUserName, NULL, NULL);
            if(pUserName)
                snprintf(pAlarmEvent->sUser,sizeof(pAlarmEvent->sUser),"%s",pUserName);
            if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "Stream",NULL, NULL, &pAlarmEvent->iStream, NULL))
            {
                pAlarmEvent->iStream = 0;
            }
            if(NULL == Common_Json_GetAttrValue(pJData, iIndex, "RegionId",NULL, NULL, &pAlarmEvent->iRegion, NULL))
            {
                pAlarmEvent->iRegion = 0;
            }
            if(pAlarmEvent->iRegion >= MAX_REGION_SUPPORT)
                pAlarmEvent->iRegion = 0;

            if(NULL == Common_Json_GetAttrValue(pJData,iIndex,"IsHappent",NULL,NULL,&iState,NULL))
            {
                ovfs_print_json(pJData);
                LOGE("no IsHappent obj\n");
                continue;
            }
            //LOGW("before: hour:%d  min:%d  sec:%d\n",t_happent.hour,t_happent.min,t_happent.sec);
            Common_Linux2CommonTime(time(NULL),&t_happent);
            //LOGW("after: hour:%d  min:%d  sec:%d\n",t_happent.hour,t_happent.min,t_happent.sec);
            //获取布防状态
            bEnable = checkSchedTime(pAlarmEvent,t_happent);

            //Common_Linux2CommonGmTime(time(NULL),&t_happent);


            pAlarmEvent->iSysState = iState;

            //处于布防时间内
            if(bEnable)
            {
                pAlarmEvent->iState = pAlarmEvent->iSysState;
            }
            else
            {
                pAlarmEvent->iState = 0;
                LOGD("not recv alarm for sched time or alarm state[%s_dev%d_ch%d]\n",pAlarmEvent->sName,pAlarmEvent->iDev,pAlarmEvent->iChan);
            }

            //需要联动
            if(pAlarmEvent->iState != pAlarmEvent->iLstState)
            {
                if(bEnable ||((!bEnable) && pAlarmEvent->bArming))
                {
                    needLink = 1;
                }
            }
            if(needLink)
            {
                //记录 事件开始时间，结束时间
                if(iState)
                {
                    pAlarmEvent->t_start = t_happent;
                }
                else
                {
                    pAlarmEvent->t_stop = t_happent;
                }
                //当前配置为enable时
                if(bArming)
                {
                    OVFS_ALARM_EVENT ev = {};
                    memcpy(&ev, pAlarmEvent,sizeof(OVFS_ALARM_EVENT));
                    Common_UnLock(pAlarmBkbd->alarmLock);
                    ovfs_report_alarm(hModuleHandle,&ev);
                    ovfs_writeAlarm2Log(hModuleHandle,pAlarmEvent);
                    ovfs_linkOutput(hModuleHandle,pAlarmEvent);
                    Common_Lock(pAlarmBkbd->alarmLock);
                }

#ifndef SIMPLIFIED
                if(pAlarmEvent->iType == ALARM_TYPE_ILLEG_ACCESS)
                {
                    OVFS_ALARM_EVENT ev = {};
                    pAlarmEvent->iState = 0;
                    pAlarmEvent->iSysState = 0;
                    pAlarmEvent->t_stop = pAlarmEvent->t_start;
                    memcpy(&ev, pAlarmEvent,sizeof(OVFS_ALARM_EVENT));
                    Common_UnLock(pAlarmBkbd->alarmLock);
                    ovfs_linkOutput(hModuleHandle,&ev);
                    Common_Lock(pAlarmBkbd->alarmLock);
                }
#endif

                //Common_Thread_Create(&t_Thread,__FUNCTION__,0,2,Thread_AlarmLinkage,(void*)pAlarmEvent);
                pAlarmEvent->iLstState = pAlarmEvent->iState;
                pAlarmEvent->bArming = bEnable;
            }
        }
        Common_UnLock(pAlarmBkbd->alarmLock);
    }
    else
    {
        LOGE("pEventInfo is null\n");
    }
    return 0;
}

static int CheckActionState(OVFS_ALARM_EVENT *pAlarmEvent)
{

    OVFS_ALARM_BKBD *pAlarmBkbd = ovfs_get_alarm_bkbd();
    OVFS_ACTION_STATE_T* actState = &(pAlarmBkbd->actionState);
    S32 iDev = 0,iChan = 0;
    OVFS_COMMON_CFG *pCfg = NULL;

    OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();

    char path[128] = {0};

    if(pAlarmEvent->iType == ALARM_TYPE_LOW_BATTERY_ALARM)
    {
        return 0;
    }


#ifdef AWSIOT
    if(pAlarmEvent->iAudioState)
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
    snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pAlarmEvent->iAudioState);
#endif

    //LOGD("alarmName:[%s] iAudioState:[%d]\n",pAlarmEvent->sName,pAlarmEvent->iAudioState);

    if(!pAlarmEvent)
        return -1;
    iDev = pAlarmEvent->iDev;
    iChan = pAlarmEvent->iChan;

    pCfg = getCfgByAlarmName(pAlarmCfg,iDev,iChan,pAlarmEvent->sName);
    //LOGD("soundCfg.enable:[%d]\n",pCfg->soundCfg.enable);

    if(!pCfg)
    {
        LOGE("Invalid sName(%s) or iDev(%d) or iChan(%d)!\n",pAlarmEvent->sName,iDev,iChan);
        return -1;
    }

    if(pCfg->soundCfg.enable)
    {
        int need_link = 0;

        if(pAlarmEvent->iAudioState != -1)
        {
            if(CheckIsInSchduleTime(-1,&(pCfg->soundCfg.shedule)) == 0)
            {
                //snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pAlarmEvent->iAudioState);
                Action_SoundAlarm(g_subscribeList.hModuleHandle,pAlarmEvent->iAudioState,path,pAlarmEvent->iAudioTimes,0);
                pAlarmEvent->iAudioState = -1;
            }
            else
            {
                if(pCfg->soundCfg.audioSelected != pAlarmEvent->iAudioState)
                {
                    //snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pAlarmEvent->iAudioState);
                    Action_SoundAlarm(g_subscribeList.hModuleHandle,pAlarmEvent->iAudioState,path,pAlarmEvent->iAudioTimes,0);

                    need_link = 1;
                }

                if(pCfg->soundCfg.playTimes != pAlarmEvent->iAudioTimes)
                {
                    need_link = 1;
                }
            }


        }
        else
        {
            need_link = 1;
        }

        if(need_link)
        {
            ovfs_linkSound(g_subscribeList.hModuleHandle,&pCfg->soundCfg, pAlarmEvent,-1);
        }
    }
    else
    {
        if(pAlarmEvent->iAudioState != -1)
        {
            //snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", pAlarmEvent->sName,pAlarmEvent->iAudioState);
            Action_SoundAlarm(g_subscribeList.hModuleHandle,pAlarmEvent->iAudioState,path,pAlarmEvent->iAudioTimes,0);
            pAlarmEvent->iAudioState = -1;
        }
    }

    return 0;
}

S32 Thread_AlarmHandle(Common_Thread_T hThreadHandle,void *pUserData)
{
    OVFS_ALARM_EVENT *pAlarmEvent = NULL,*pTemp = NULL;
    OVFS_ABILITY *pAbility = ovfs_get_ability();
    OVFS_ALARM_BKBD *pAlarmBkbd = ovfs_get_alarm_bkbd();

    //OVFS_ALARM_CFG * alarmCfg = ovfs_get_alarm_cfg();

    S32 i = 0,j = 0,totalChanNum = 0,iAlarmCount = 0;
    static int cnt = 0;
    prctl(PR_SET_NAME,__func__);
    if(!pAlarmBkbd)
    {
        LOGE("pAlarmBkbd is null\n");
        return -1;
    }
    if(!pAbility)
    {
        LOGE("pAbility is null\n");
        return -1;
    }
    iAlarmCount = pAlarmBkbd->alarmCount;
    totalChanNum = MAX_CHANNEL_SUPPORT;//pAbility->totalChanNum;

    //int iCurTime = time(NULL);

    LOGW("iAlarmCount:[%d]\n",iAlarmCount);

    while(1)
    {

        //LOGW("File_IsExist:[%d]\n",Common_File_IsExist("/tmp/smartsnap/start.jpg"));
        for(i = 0; i < iAlarmCount; i++)
        {
            pAlarmEvent = pAlarmBkbd->pAlarmEvent[i];
            if(!pAlarmEvent)
                continue;

            if(ALARM_TYPE_ALARMIN == i)
            {
                totalChanNum = pAbility->alarmInNum;
            }
            else if(ALARM_TYPE_SENSOR_ALARM == i)
            {
                totalChanNum = MAX_SENSORALARM;
            }
            else
            {
                totalChanNum = MAX_CHANNEL_SUPPORT;//pAbility->totalChanNum;
            }
            Common_Lock(pAlarmBkbd->alarmLock);
            for(j = 0; j < totalChanNum; j++)
            {
                pTemp = &pAlarmEvent[j];
                if(!pTemp)
                    continue;
                int bArming = 0;
                Common_Time_T t_happent;
                Common_Linux2CommonTime(time(NULL),&t_happent);
                bArming = checkSchedTime(pTemp,t_happent);
                //Common_Linux2CommonGmTime(time(NULL),&t_happent);

                if(pTemp->iSysState)
                {
                    OVFS_ALARM_EVENT *pTempEvent = NULL;
                    pTempEvent = pTemp;

                    //LOGW("[%d/%d] Name:[%s] [%d][%d][%d]\n",i,iAlarmCount,pTemp->sName,pTemp->iSysState,bArming,pTemp->bArming);
                    if(bArming !=  pTemp->bArming)
                    {
                        if(bArming)
                        {
                            pTempEvent->iState = 1;
                            pTempEvent->t_start = t_happent;
                        }
                        else
                        {
                            pTempEvent->t_stop = t_happent;
                            pTempEvent->iState = 0;
                        }
                        if(pTempEvent->iLstState != pTempEvent->iState)
                        {
                            OVFS_ALARM_EVENT ev = {};
                            memcpy(&ev, pTempEvent,sizeof(OVFS_ALARM_EVENT));
                            Common_UnLock(pAlarmBkbd->alarmLock);
                            ovfs_report_alarm(g_subscribeList.hModuleHandle,&ev);
                            //ovfs_writeAlarm2Log(g_subscribeList.hModuleHandle,&ev);
                            ovfs_linkOutput(g_subscribeList.hModuleHandle,pTempEvent);
                            Common_Lock(pAlarmBkbd->alarmLock);
                        }
                        pTempEvent->bArming = bArming;
                        pTempEvent->iLstState = pTempEvent->iState;
                    }
                    //else if(bArming)
                    {
                        //LOGW("iType:[%d]\n",pTempEvent->iType);
                        if(pTempEvent->iType == ALARM_TYPE_LOW_BATTERY_ALARM)
                        {
                           // int offset = (t_happent.sec+60-pTempEvent->t_start.sec)%60;
                            //LOGW("iState:[%d] sec[%d][%d] offset[%d]\n",pTempEvent->iState,t_happent.sec,pTempEvent->t_start.sec,offset);
                            if(pTempEvent->iState && ((t_happent.sec+60-pTempEvent->t_start.sec)%60 >= 3))
                            {
                                LOGW("alarm[ALARM_TYPE_LOW_BATTERY_ALARM] stop\n");
                                pTempEvent->iState = 0;
                                pTempEvent->t_stop = t_happent;
                                OVFS_ALARM_EVENT ev = {};
                                memcpy(&ev, pTempEvent,sizeof(OVFS_ALARM_EVENT));
                                Common_UnLock(pAlarmBkbd->alarmLock);
                                ovfs_report_alarm(g_subscribeList.hModuleHandle,&ev);
                                //ovfs_writeAlarm2Log(g_subscribeList.hModuleHandle,&ev);
                                //ovfs_linkOutput(g_subscribeList.hModuleHandle,pTempEvent);
                                Common_Lock(pAlarmBkbd->alarmLock);

                            }
                        }
                        CheckActionState(pTempEvent);
                    }
                }
                else
                {
                    pTemp->bArming = bArming;
                }
            }
            Common_UnLock(pAlarmBkbd->alarmLock);
            Common_Sleep(0, 10*1000);
        }



        //CheckActionState();


        Common_Sleep(0, 500 * 1000);
        cnt++;
    }
    return 0;
}

int ovfs_recv_alarm(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    return recv_alarmEvents_fxn(g_subscribeList.hModuleHandle,0,pAddData,pOutData,NULL);
}

#ifndef SIMPLIFIED

S32 Thread_Snap2Ftp(Common_Thread_T hThreadHandle,void *pUserData)
{
    time_t lastTime = 0;

    while(1)
    {
        OVFS_ALARM_CFG  *pAlarmCfg = ovfs_get_alarm_cfg();
        time_t cutT = time(NULL);

        if( pAlarmCfg->ftpsnapCfg.snapInterval > 0 )
        {
            if((cutT - lastTime >= pAlarmCfg->ftpsnapCfg.snapInterval
                || cutT - lastTime < 0)
                && CheckIsInSchduleTime(cutT,&(pAlarmCfg->ftpsnapCfg.schTime)))
            {
                LOGW("ftp snap!\n");
                Action_SnapAlarm(g_subscribeList.hModuleHandle,pAlarmCfg->ftpsnapCfg.snapStreamIndex);
                lastTime = cutT;
            }
        }
        else
        {
            lastTime = 0;
        }

        Common_Sleep(0, 500 * 1000);
    }
    return 0;
}

int get_alarmIn_ability(ModuleHandle_T hModuleHandle)
{
    int iAinNum = 0;
    cJSON_Struct *pConfig,*pOutParams = NULL;
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Boardsys/Event/Ability",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
    }
    Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
    Common_Json_Delete(pConfig);
    if(!pOutParams)
    {
        LOGE("pOutParams is null!\n");
        return 0;
    }
    Common_Json_GetAttrValue(pOutParams,-1,"Data/AlarmInNum",NULL,NULL,&iAinNum,NULL);
    Common_Json_Delete(pOutParams);
    return iAinNum;
}

int get_alarmOut_ability(ModuleHandle_T hModuleHandle)
{
    int iAoutNum = 0;
    cJSON_Struct *pConfig,*pOutParams = NULL;
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Boardsys/AlarmOut/Ability",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
    }
    Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
    Common_Json_Delete(pConfig);
    if(!pOutParams)
    {
        LOGE("pOutParams is null!\n");
        return 0;
    }
    Common_Json_GetAttrValue(pOutParams,-1,"Data/AlaramOutNum",NULL,NULL,&iAoutNum,NULL);
    Common_Json_GetAttrValue(pOutParams,-1,"Data/AlarmOutNum",NULL,NULL,&iAoutNum,NULL);
    Common_Json_Delete(pOutParams);
    return iAoutNum;
}
#endif

int get_mount_path(ModuleHandle_T hModuleHandle,OVFS_ABILITY *ppAbility)
{
    S8 *pValue = NULL,*pPath = NULL;
    int iRet = -1,iArraySize = 0;
    OVFS_ABILITY *pAbility = ppAbility;
    cJSON_Struct *pConfig = NULL,*pOutParams = NULL,*pArray = NULL;

    if(!ppAbility)
    {
        LOGW("ppAbility == NULL\n");
        return -1;
    }
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Record/DiskManage/Disk0/Attribute",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
    }
    iRet = Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
    Common_Json_Delete(pConfig);
    if(!pOutParams||iRet < 0)
    {
        LOGE("pOutParams == NULL||iRet:%d\n",iRet);
        return -1;
    }
    pArray = Common_Json_GetItem(pOutParams,-1,"Data/Partition");
    if(!pArray)
    {
        LOGW("Not find partition param\n");
        ovfs_print_json(pOutParams);
        Common_Json_Delete(pOutParams);
        return -1;
    }
    iArraySize= Common_Json_Size(pArray);
    for(int i = 0; i < iArraySize; i++)
    {
        if(NULL == Common_Json_GetAttrValue(pArray,i,(S8*)"MountPath",NULL,&pPath,NULL,NULL))
        {
            LOGW("Not find mount path param\n");
            ovfs_print_json(pOutParams);
            Common_Json_Delete(pOutParams);
            return -1;
        }
        if(NULL == Common_Json_GetAttrValue(pArray,i,(S8*)"PartitionType",NULL,&pValue,NULL,NULL))
        {
            LOGW("Not find partition type param\n");
            ovfs_print_json(pOutParams);
            Common_Json_Delete(pOutParams);
            return -1;
        }
        /*if(0 == Common_StriCmp(pValue, (S8*)"Normal"))
        {
            if(pValue&&(0 != Common_StriCmp(pAbility->snapPath, pValue)))
            {
                memset(pAbility->snapPath,0,sizeof(pAbility->snapPath));
                strcpy(pAbility->snapPath,pPath);
                LOGD("snapPath:%s\n",pAbility->snapPath);
            }
        }
        else if(0 == Common_StriCmp(pValue, (S8*)"Record"))
        {
            if(pValue&&(0 != Common_StriCmp(pAbility->recordPath, pValue)))
            {
                memset(pAbility->recordPath,0,sizeof(pAbility->recordPath));
                strcpy(pAbility->recordPath,pPath);
            }
        }*/
    }
    Common_Json_Delete(pOutParams);
    return 0;
}

OVFS_ABILITY * ovfs_get_ability()
{
    return g_ability;
}

int ovfs_init_ability(ModuleHandle_T hModuleHandle)
{
    g_ability = (OVFS_ABILITY*)Common_Malloc(sizeof(OVFS_ABILITY),0,__FUNCTION__,__LINE__);;

    g_ability->alarmOutNum = get_alarmOut_ability(hModuleHandle);
    g_ability->alarmInNum = get_alarmIn_ability(hModuleHandle);


    LOGD("AlarmInNum:%d,AlarmOutNum:%d\n",g_ability->alarmInNum,g_ability->alarmOutNum);
    return 0;
}

int ovfs_init_subscribe(ModuleHandle_T hModuleHandle)
{
    Common_Thread_T hThread = NULL;
    memset(&g_subscribeList,0,sizeof(g_subscribeList));
    g_subscribeList.hModuleHandle = hModuleHandle;
    Common_DList_Init(&g_subscribeList.listdl,freeSubscribeListNode);
    Module_RegisterSubscribe(hModuleHandle,(S8*)"/Alarm/Subscribe/Status",add_subscribeList_fxn,NULL);
    Module_SubscribeEvent(hModuleHandle,(S8*)"/BoardSys/Subscribe/Event",recv_alarmEvents_fxn,NULL);
    Module_SubscribeEvent(hModuleHandle,(S8*)"/SmartServer/Subscribe/Event",recv_alarmEvents_fxn,NULL);
    Module_SubscribeEvent(hModuleHandle,(S8*)"/ptz/subscribe/sensoralarm",recv_alarmEvents_fxn,NULL);
    //Module_SubscribeEvent(hModuleHandle,(S8*)"/Network/Subscribe/NetOut",recv_alarmEvents_fxn,NULL);
    //Module_SubscribeEvent(hModuleHandle,(S8*)"/Network/Subscribe/NetIpConflict",recv_alarmEvents_fxn,NULL);
    //Module_SubscribeEvent(hModuleHandle,(S8*)"/Record/DiskManage/SubScribe/NoDisk",recv_alarmEvents_fxn,NULL);
    //Module_SubscribeEvent(hModuleHandle,(S8*)"/Record/DiskManage/SubScribe/DiskFull",recv_alarmEvents_fxn,NULL);
    //Module_SubscribeEvent(hModuleHandle,(S8*)"/Record/DiskManage/SubScribe/DiskError",recv_alarmEvents_fxn,NULL);
#ifndef SIMPLIFIED
    //Module_SubscribeEvent(hModuleHandle,(S8*)"/MediaServer/Subscribe/MediaDisconnect/Event",recv_mediaEvents_fxn,NULL);
    Common_Thread_T hThread_ftp = NULL;
    Common_Thread_Create(&hThread_ftp,__FUNCTION__,0,0,Thread_Snap2Ftp,NULL);
#endif
    Common_Thread_Create(&hThread,__FUNCTION__,0,0,Thread_AlarmHandle,NULL);

    return 0;
}

int ovfs_get_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
    int iRet = -1;
    iRet = AnalyzeUriAndMakeResult(MATHOD_GET,pUri,pAddData,outJson);
    return iRet;
}

int ovfs_put_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
    int iRet = -1;
    iRet = AnalyzeUriAndMakeResult(MATHOD_PUT,pUri,pAddData,outJson);
    return iRet;
}

int ovfs_post_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
    int iRet = -1;
    iRet = AnalyzeUriAndMakeResult(MATHOD_POST,pUri,pAddData,outJson);
    return iRet;
}

int ovfs_delete_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
    int iRet = -1;
    iRet = AnalyzeUriAndMakeResult(MATHOD_DELETE,pUri,pAddData,outJson);
    return iRet;
}

