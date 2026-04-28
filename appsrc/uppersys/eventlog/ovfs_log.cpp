/*
 * ovfs_alarm.c
 *
 *  Created on: 2017年3月07日
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

#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "ovfs_log.h"


static OVFS_LOG_MGR g_LogMgr;
//static Common_Thread_T g_hThread;
static cJSON_Struct *g_rest_res = NULL;
static int	g_nUserCounter = 0;
// 0:关机  1:重启 2:不写日志
static int 	g_iReboot = 0;
S32 updateRunInfo();

int ovfs_MakeHandle(int nLoginHandle,int nSubIndex)
{
	int nUserIdx = nLoginHandle & OVFS_LOGINHANDLE_BITMASK;
	int nSubIdx= nSubIndex & OVFS_SUBHANDLE_BITMASK;
	int nHandle;
	// 生成句柄
		g_nUserCounter++;
		if(g_nUserCounter == 0 || g_nUserCounter > OVFS_RAND_BITMASK)
		{
			g_nUserCounter = 1;
		}
		nHandle = (nUserIdx) | (nSubIdx << OVFS_LOGINHANDLE_BITSIZE) | ((g_nUserCounter & OVFS_RAND_BITMASK) << (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE));
	return nHandle;
}

S32 MakeResult(const S8 *pFileDes,const S8 *pFuncDes,S32 nLine,S32 iCode,const S8 *pDes,cJSON_Struct *pData,cJSON_Struct **pOutData)
{
	cJSON_Struct *pResult = NULL;
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
			sprintf(sDes,"[%s][%s][%d] %s",pFileDes,pFuncDes,nLine,pDes);
			Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sDes,0,0);
		}
		else
		{
			Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,pDes,0,0);
		}
		Common_Json_AddItem(pResult,-1,"Data",pData);
	}
	*pOutData = pResult;
	return 0;
}

S32  set_method_callback(cJSON_Struct **pData,OVFS_GET_METHOD pGetMethod,OVFS_PUT_METHOD pPutMethod,OVFS_POST_METHOD pPostMethod,OVFS_DELETE_METHOD pDelMethod)
{
	OVFS_REST_METHOD *pRestMethod;
	pRestMethod = (OVFS_REST_METHOD *)Common_Malloc(sizeof(OVFS_REST_METHOD),0,__FUNCTION__,__LINE__);
	if(!pRestMethod)
		return -1;
	pRestMethod->ovfs_get_method = pGetMethod;
	pRestMethod->ovfs_put_method = pPutMethod;
	pRestMethod->ovfs_post_method = pPostMethod;
	pRestMethod->ovfs_delete_method = pDelMethod;
	Common_Json_SetItemExtData(*pData, (void *)pRestMethod, sizeof(OVFS_REST_METHOD));
	Common_Free(pRestMethod,__FUNCTION__,__LINE__);
	pRestMethod = NULL;
	return 0;
}

//解析表达式
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

//分离Uri里面的条件为
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

//分离原始的pSrcUri，并返回条件
cJSON_Struct *SeparateUriAndCondition(S8 *pSrcUri,S8 **pDstUri)
{
	int i = 0,iCount = 0;;
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

//解析原始URI和方法类型并执行相应的方法
//method: 0-get 1-put 2-post 3-delete
//"Uri:/Alarm/TriggerCfg/AlarmIn/Attr?Device=-1&&Channel=-1"
static S32 AnalyzeUriAndMakeResult(S32 method,char* pUri,cJSON_Struct *pAddData,cJSON_Struct **ppResult)
{
	S8 pResPath[128] = {0};
	OVFS_REST_METHOD *pMethod = NULL;
	S8 *pTemp = NULL,*pSrcUri = NULL,*pFirst = NULL,*pNext = NULL;
	cJSON_Struct *pRoot = g_rest_res,*pChild = NULL,*pJCondition = NULL;

	if(!g_rest_res)
	{
		LOGW("g_rest_res  == null\n");
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
						break;
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
static S64 g_ShutDownCount = 0;
/*S32 checkWriteShutdownLog()
{
	S64 iLstRebootTime = 0;
	int iReboot = -1;
	FILE *fp = Common_File_fOpen((S8*)"/tmp/mmc/mmc1/reboottime", (S8*)"r");
	if(fp)
	{
		fscanf(fp,"reboot=%lld manreboot=%d",&iLstRebootTime,&iReboot);
		Common_File_fClose(fp);

	}
	// 更新
	g_iReboot = 0;
	updateRunInfo();
	if(0==iReboot)
	{
		S32 iRet = 0;
		OVFS_LOG log;
		SQLITE3MGR	*pSqlMgr = g_LogMgr.pSqlMgr;

		if(pSqlMgr)
		{
			memset(&log,0,sizeof(OVFS_LOG));
			log.dwMajorType = MAJOR_OPERATION;
			log.dwMinorType = MINOR_STOP_DVR;
			log.dwLogTime = time(NULL) - 30;// iLstRebootTime;
			log.dwChannel = 1;
			iRet = pSqlMgr->InsertLog(log);
			if(TRUE == iRet)
				LOGD("InsertLog %d %d %d %s %s %d %d %d %d %s\n",log.dwLogTime,log.dwMajorType,log.dwMinorType,log.sUser,log.sRemoteHostAddr,log.dwChannel,log.dwAlarmInPort,log.dwAlarmOutPort,log.bStatus,iRet>0?"succ":"failed");
		}
	}
	return 0;
}*/

S32 updateRunInfo()
{
	S8 temp[64] = {0};
	S64 s64StartUpTime = 0;
	FILE *fp = NULL;
	Common_Lock(g_LogMgr.tLock);
	snprintf(temp,sizeof(temp),"reboot=%ld manreboot=%d",time(NULL),g_iReboot);
	fp = Common_File_fOpen((S8*)"/tmp/mmc/mmc1/reboottime", (S8*)"w+");
	if(fp)
	{
		Common_File_fWrite(temp,sizeof(temp),1,fp);
		Common_File_fClose(fp);
	}
	Common_UnLock(g_LogMgr.tLock);
	return 0;
}

S32 Thread_LogHandle(Common_Thread_T hThreadHandle,void *pUserData)
{
//	ModuleHandle_T hModuleHandle = *(ModuleHandle_T*)pUserData;
	S32 i = 0, iLogHandle = 0, iLogType = 0;
	S32 dwRunTime = 0;
	OVFS_USER_INFO *pUserInfo = NULL;

	prctl(PR_SET_NAME,__FUNCTION__);
	while(1)
	{
#if 0
		if(nCount == 0)
		{
			updateRunInfo();
		}
		nCount++;
		if(nCount >= 10 * 60)
		{// 10 分钟刷新一次
			nCount = 0;
		}
#endif

		//超过1分钟，销毁相关的资源
		for(i = 0; i < OVFS_MAX_LOGIN_USER && g_LogMgr.nUserCount > 0;i++)
		{
            Common_Lock(g_LogMgr.tLock);
			pUserInfo = g_LogMgr.pUserInfo[i];
			if(pUserInfo)
			{
				Common_GetSystemCount((S32 *)&dwRunTime, NULL);
				if(dwRunTime-pUserInfo->dwCreateTime>60*5){
					//LOGW("delete handle(%d)\n",pUserInfo->dwHandle);
					//Common_Free(pUserInfo->pLogData, __FUNCTION__, __LINE__);
					iLogType = pUserInfo->dwLogType;
                    iLogHandle = pUserInfo->dwLogHandle;
					Common_Free(pUserInfo, __FUNCTION__, __LINE__);
					g_LogMgr.pUserInfo[i] = NULL;
					g_LogMgr.nUserCount--;

                    g_LogMgr.pSqlMgr->QueryStop(iLogType, iLogHandle);
				}
			}
            Common_UnLock(g_LogMgr.tLock);
            Common_Sleep(0, 10*1000);
		}
		Common_Sleep(1, 0);
	}
	return 0;
}

int get_handle(int iLogType, int iType, int iStartTime, int iEndTime, int iSort, cJSON_Struct **pOutData)
{
    int iRet = 0;
    int dwHandle = -1;
    int nCurrIdx = -1;
    int nLogCnt = 0;
	OVFS_USER_INFO *pUserInfo = NULL;
    cJSON_Struct *pData = NULL;

    if(iRet == 0)
    {
        Common_Lock(g_LogMgr.tLock);
            //分配句柄
        for(int i = 0; i < OVFS_MAX_LOGIN_USER && g_LogMgr.nUserCount < OVFS_MAX_LOGIN_USER;i++)
        {
            if(g_LogMgr.pUserInfo[i] == NULL)
            {
                nCurrIdx = i;
                break;
            }
        }

        if(nCurrIdx < 0)
        {
            LOGE("nCurrIdx:%d\n",nCurrIdx);
            Common_UnLock(g_LogMgr.tLock);
            ovfs_make_result(-12,"handle is used up!waiting 5 minute",NULL,pOutData);
            return -12;
        }

        pUserInfo = (OVFS_USER_INFO*)Common_Malloc(sizeof(OVFS_USER_INFO), 0, __FUNCTION__, __LINE__);
        if(!pUserInfo)
        {
            //ovfs_make_result(-1,"Failed",NULL,pOutData);
            LOGE("pUserInfo:null\n");

        }
        memset(pUserInfo,0,sizeof(OVFS_USER_INFO));
        g_LogMgr.pUserInfo[nCurrIdx] = pUserInfo;
        g_LogMgr.nUserCount++;

        Common_UnLock(g_LogMgr.tLock);
    }

    if(iRet == 0)
    {
        dwHandle = g_LogMgr.pSqlMgr->QueryStart(iLogType, iType, iStartTime, iEndTime, iSort);
        if(dwHandle < 0)
        {
            LOGE("dwLogHandle:%d\n",dwHandle);
            iRet = -1;
            Common_Lock(g_LogMgr.tLock);
            Common_Free(pUserInfo, __FUNCTION__, __LINE__);
            pUserInfo = NULL;
            Common_UnLock(g_LogMgr.tLock);
            return iRet;
        }

        if(g_LogMgr.pSqlMgr->GetQueryLogsCnt(dwHandle, &nLogCnt) && nLogCnt >= 0 )
        {
            LOGD("nLogCnt:%d\n",nLogCnt);

            pUserInfo->dwLogType = iLogType;
            pUserInfo->nLogCnt = nLogCnt;
            pUserInfo->dwLogHandle = dwHandle;
            Common_GetSystemCount((S32 *)&pUserInfo->dwCreateTime, NULL);

            pUserInfo->dwHandle = ovfs_MakeHandle(nCurrIdx, 0);


            pData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
            if (pData)
            {
                Common_Json_SetAttrValueInt(pData, "Handle", pUserInfo->dwHandle);
                Common_Json_SetAttrValueInt(pData, "TotalCount", pUserInfo->nLogCnt);
            }
            *pOutData = pData;
        }
    }

    return iRet;
}

int get_result(cJSON_Struct *pInData, int iLogType, void **pList, int *iListSize)
{
    int iRet = 0;
    int iHandle = 0;
    int iStart = 0;
    int iCount = 0;
    int nCurrIdx = -1;
    int iLogHandle = 0;
    OVFS_USER_INFO *pUserInfo = NULL;

    Common_Json_GetAttrValueInt(pInData, "Handle", &iHandle);
    Common_Json_GetAttrValueInt(pInData, "Start", &iStart);
    Common_Json_GetAttrValueInt(pInData, "Count", &iCount);

    if(iHandle == 0 || iCount == 0)
    {
        iRet = -1;
    }
    else
    {
        iRet = -11;
        nCurrIdx = iHandle & OVFS_LOGINHANDLE_BITMASK;
        if(nCurrIdx >= 0 && nCurrIdx < OVFS_MAX_LOGIN_USER)
        {
            Common_Lock(g_LogMgr.tLock);
            pUserInfo = g_LogMgr.pUserInfo[nCurrIdx];
            if(pUserInfo && pUserInfo->dwHandle == iHandle)
            {
                iLogHandle = pUserInfo->dwLogHandle;

                int curTime = 0;
                Common_GetSystemCount(&curTime, NULL);
                pUserInfo->dwCreateTime = curTime;

                iRet = 0;
                LOGD("find iLogHandle\n");
            }
            Common_UnLock(g_LogMgr.tLock);
        }
    }

    LOGD("iRet:[%d] iLogHandle:[%d]\n",iRet,iLogHandle);

    if(iRet == 0 && iLogHandle > 0)
    {
        iRet = g_LogMgr.pSqlMgr->GetQueryList(iLogType, iLogHandle, iStart, iCount, pList, iListSize);
    }

    return iRet;
}

int del_handle(cJSON_Struct *pInData, int iLogType)
{
    int iRet = 0;
    int iHandle = -1;
    int nCurrIdx = -1;
    int iLogHandle = 0;
	OVFS_USER_INFO *pUserInfo = NULL;

    LOGD("del_handle\n");
    if(pInData)
    {
        Common_Json_GetAttrValueInt(pInData, "Handle", &iHandle);
        if(iHandle > 0)
        {
            iRet = -1;
            nCurrIdx = iHandle & OVFS_LOGINHANDLE_BITMASK;
        	if(nCurrIdx >= 0 && nCurrIdx < OVFS_MAX_LOGIN_USER)
        	{
        		Common_Lock(g_LogMgr.tLock);
                pUserInfo = g_LogMgr.pUserInfo[nCurrIdx];
                if(pUserInfo && pUserInfo->dwHandle == iHandle)
            	{
            		iLogHandle = pUserInfo->dwLogHandle;
                    iRet = 0;
                    LOGD("find iLogHandle\n");

                    Common_Free(pUserInfo, __FUNCTION__, __LINE__);
                    g_LogMgr.pUserInfo[nCurrIdx] = NULL;
                    g_LogMgr.nUserCount--;
            	}
                Common_UnLock(g_LogMgr.tLock);
        	}
        }

        if(iRet == 0)
        {
            iRet = g_LogMgr.pSqlMgr->QueryStop(1, iLogHandle);
        }
    }
}

int get_log_top_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 sErrorDes[256] = {0};
	S32 i = 0,iCode = 0;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
	S8 label[][64] = {"LogFunction","AlarmFunction","DeleteAllLog"};
	S8 uri[][64] = {"/EventLog/LogFunction","/EventLog/AlarmFunction","/EventLog/DeleteAllLog"};

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,iCode,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sErrorDes,11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

        for(i = 0;i < ARRAYSIZE(label);i++)
        {
			Common_Json_SetAttrValue(pArray,i,"label",Common_Json_Type_String,label[i],0,0);
			Common_Json_SetAttrValue(pArray,i,"uri",Common_Json_Type_String,uri[i],0,0);
		}
	}
	*pOutData = pResult;
	return 0;
}

int post_log_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	int nRet = 0;
    int nType = -1;
    int nStartTime = 0, nEndTime = 0;
    int nSort = 0;
    cJSON_Struct *pData = NULL;

    if(pAddData)
    {
        Common_Json_GetAttrValueInt(pAddData, "MajorType", &nType);
        Common_Json_GetAttrValueInt(pAddData, "Sort", &nSort);

        if(Common_Json_GetAttrValueInt(pAddData, "StartTime", &nStartTime) == NULL ||
           Common_Json_GetAttrValueInt(pAddData, "EndTime", &nEndTime) == NULL ||
           nStartTime > nEndTime)
        {
            LOGE("time error! [%d][%d]\n",nStartTime,nEndTime);
            nRet = -1;
        }

        if(nRet == 0)
        {
            nRet = get_handle(0, nType, nStartTime, nEndTime, nSort, &pData);
        }
    }
    else
    {
        LOGE("IN Para is NULL!\n");
        nRet = -1;
    }

    ovfs_make_result(nRet,"",pData,pOutData);
	return 0;
}

int get_log_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int iRet = 0;
    int iListCount = 0;
    OVFS_LOG *pInfoList = NULL;
    cJSON_Struct *pData = NULL;

    iRet = get_result(pAddData, 0, (void **)&pInfoList, &iListCount);

    if(iRet == 0)
    {
        pData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        cJSON_Struct *pList = Common_Json_SetAttrValueArr(pData, "List");
        for(int i=0; i<iListCount; i++)
        {
            cJSON_Struct *pItem = Common_Json_SetAttrValueArrObj(pList, i);
            Common_Json_SetAttrValueInt(pItem, "Id", pInfoList[i].dwId);
            Common_Json_SetAttrValueInt(pItem, "LogTime", pInfoList[i].dwLogTime);
            Common_Json_SetAttrValueInt(pItem, "MajorType", pInfoList[i].dwMajorType);
            Common_Json_SetAttrValueInt(pItem, "MinorType", pInfoList[i].dwMinorType);
            Common_Json_SetAttrValueStr(pItem, "UserName", (char *)pInfoList[i].sUser);
            Common_Json_SetAttrValueStr(pItem, "IP", (char *)pInfoList[i].sIP);
            Common_Json_SetAttrValueInt(pItem, "Channel", pInfoList[i].dwChannel);
            Common_Json_SetAttrValueStr(pItem, "ExtraInfo", (char *)pInfoList[i].sExtraInfo);
        }
    }

    if(pInfoList)
    {
        free(pInfoList);
        pInfoList = NULL;
    }

	ovfs_make_result(iRet,"",pData,pOutData);
	return 0;
}

/*
tLogTime = (time_t)pLog->dwLogTime;
tm_set = Common_LocalTime_r((time_t *)&tLogTime,&tm_save);
t_logtime.year = tm_set->tm_year + 1900;
t_logtime.month = tm_set->tm_mon + 1;
t_logtime.day = tm_set->tm_mday;
t_logtime.hour = tm_set->tm_hour;
t_logtime.min = tm_set->tm_min;
t_logtime.sec = tm_set->tm_sec;
sprintf(strTemp,"%04d-%02d-%02d %02d:%02d:%02d",t_logtime.year,t_logtime.month,t_logtime.day,t_logtime.hour,t_logtime.min,t_logtime.sec);

*/

int put_log_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int iRet = 0;
	OVFS_LOG log = {0};
	S8 *pTemp = NULL;

    if(pAddData)
    {
        Common_Json_GetAttrValueInt(pAddData, "LogTime", &log.dwLogTime);
        Common_Json_GetAttrValueInt(pAddData, "MajorType", &log.dwMajorType);
        Common_Json_GetAttrValueInt(pAddData, "MinorType", &log.dwMinorType);

        if(Common_Json_GetAttrValueStr(pAddData, "UserName", &pTemp) && pTemp)
        {
            snprintf(log.sUser, sizeof(log.sUser), "%s", pTemp);
        }

        pTemp = NULL;
        if(Common_Json_GetAttrValueStr(pAddData, "IP", &pTemp) && pTemp)
        {
            snprintf(log.sIP, sizeof(log.sIP), "%s", pTemp);
        }

        Common_Json_GetAttrValueInt(pAddData, "Channel", &log.dwChannel);

        pTemp = NULL;
        if(Common_Json_GetAttrValueStr(pAddData, "ExtraInfo", &pTemp) && pTemp)
        {
            snprintf(log.sExtraInfo, sizeof(log.sExtraInfo), "%s", pTemp);
        }

        /*if(log.dwMajorType == MAJOR_OPERATION && (log.dwMinorType == MINOR_REMOTE_REBOOT ||log.dwMinorType == MINOR_REBOOT_DVR))
		{
			g_iReboot = 1;
			updateRunInfo();
		}*/

		{
			iRet = g_LogMgr.pSqlMgr->InsertLog(log);
		}
    }

	ovfs_make_result(iRet,"",NULL,pOutData);
	return iRet;
}

int delete_log_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int iRet = del_handle(pAddData, 0);
    ovfs_make_result(iRet,"",NULL,pOutData);
    return 0;
}



int get_alarm_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int iRet = 0;
    int iListCount = 0;
    OVFS_ALARMNFO *pInfoList = NULL;
    cJSON_Struct *pData = NULL;

    iRet = get_result(pAddData, 1, (void **)&pInfoList, &iListCount);

    if(iRet == 0)
    {
        pData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        cJSON_Struct *pList = Common_Json_SetAttrValueArr(pData, "List");
        for(int i=0; i<iListCount; i++)
        {
            cJSON_Struct *pItem = Common_Json_SetAttrValueArrObj(pList, i);
            Common_Json_SetAttrValueInt(pItem, "Id", pInfoList[i].dwId);
            Common_Json_SetAttrValueInt(pItem, "AlarmType", pInfoList[i].dwAlarmType);
            Common_Json_SetAttrValueInt(pItem, "StartTime", pInfoList[i].dwStartTime);
            Common_Json_SetAttrValueInt(pItem, "EndTime", pInfoList[i].dwEndTime);
            Common_Json_SetAttrValueStr(pItem, "AlarmPic", (char *)pInfoList[i].sAlarmPic);
            Common_Json_SetAttrValueStr(pItem, "ExtraInfo", (char *)pInfoList[i].sExtraInfo);
        }
    }

    if(pInfoList)
    {
        free(pInfoList);
        pInfoList = NULL;
    }

	ovfs_make_result(iRet,"",pData,pOutData);
	return 0;
}

int put_alarm_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int iRet = 0;
    int iLogId = 0;
    char *pTemp = NULL;
    OVFS_ALARMNFO struInfo = {0};
    cJSON_Struct *pDataTmp = pAddData;
    cJSON_Struct *pData = NULL;
    if(pDataTmp == NULL)
    {
        iRet = -1;
    }

    Common_Json_GetAttrValueInt(pDataTmp, "LogId", &struInfo.dwId);
    Common_Json_GetAttrValueInt(pDataTmp, "AlarmType", &struInfo.dwAlarmType);
    Common_Json_GetAttrValueInt(pDataTmp, "StartTime", &struInfo.dwStartTime);
    Common_Json_GetAttrValueInt(pDataTmp, "EndTime", &struInfo.dwEndTime);

    if(Common_Json_GetAttrValueStr(pDataTmp, "AlarmPic", &pTemp) && pTemp)
    {
        snprintf(struInfo.sAlarmPic,sizeof(struInfo.sAlarmPic),"%s",pTemp);
    }

    pTemp = NULL;
    if(Common_Json_GetAttrValueStr(pAddData, "ExtraInfo", &pTemp) && pTemp)
    {
        snprintf(struInfo.sExtraInfo, sizeof(struInfo.sExtraInfo), "%s", pTemp);
    }
    LOGD("sExtraInfo:[%s]\n",struInfo.sExtraInfo);

    /*if(struInfo.dwAlarmType == 0)
    {
        iRet = -1;
    }*/

    if(struInfo.dwId)
    {
        if(struInfo.dwEndTime == 0)
        {
            iRet = -1;
        }
    }
    else
    {
        if(struInfo.dwStartTime == 0)
        {
            iRet = -1;
        }
    }

    if(iRet == 0)
    {
        LOGD("dwId:[%d]\n",struInfo.dwId);
        if(struInfo.dwId)
        {
            iRet = g_LogMgr.pSqlMgr->UpdateAlarmInfo(struInfo);
        }
        else
        {
            iRet = g_LogMgr.pSqlMgr->InsertAlarmInfo(struInfo, &iLogId);
            if(iRet == 0)
            {
                 pData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
                 Common_Json_SetAttrValueInt(pData, "LogId", iLogId);
            }
        }
    }

    ovfs_make_result(iRet ,"",pData,pOutData);
    return iRet;
}

int post_alarm_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int nRet = 0;
    int nType = -1;
    int nStartTime = 0, nEndTime = 0;
    int nSort = 0;
    cJSON_Struct *pData = NULL;

    if(pAddData)
    {
        Common_Json_GetAttrValueInt(pAddData, "AlarmType", &nType);
        Common_Json_GetAttrValueInt(pAddData, "Sort", &nSort);

        if(Common_Json_GetAttrValueInt(pAddData, "StartTime", &nStartTime) == NULL ||
           Common_Json_GetAttrValueInt(pAddData, "EndTime", &nEndTime) == NULL ||
           nStartTime > nEndTime)
        {
            LOGE("time error! [%d][%d]\n",nStartTime,nEndTime);
            nRet = -1;
        }

        if(nRet == 0)
        {
            nRet = get_handle(1, nType, nStartTime, nEndTime, nSort, &pData);
        }
    }
    else
    {
        LOGE("IN Para is NULL!\n");
        nRet = -1;
    }

    ovfs_make_result(nRet,"",pData,pOutData);
    return nRet;
}

int delete_alarm_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
    int iRet = del_handle(pAddData, 1);
	ovfs_make_result(iRet,"",NULL,pOutData);
	return 0;
}

int delete_allLog_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	if(Common_File_IsExist((S8 *)"/tmp/mmc/mmc1/log/UsrLogFile/Log.db"))
	{
		remove("/tmp/mmc/mmc1/log/UsrLogFile/Log.db");
	}

    Common_System("rm -rf /tmp/mmc/mmc1/log/AlarmPic/*");

	ovfs_make_result(0,"Succ",NULL,pOutData);
	g_iReboot = 2;
	updateRunInfo();
	return 0;
}

int ovfs_init_log(ModuleHandle_T hModuleHandle)
{
	cJSON_Struct *pRoot = NULL,*pChild = NULL;

	g_LogMgr.pSqlMgr = SQLITE3MGR::GetSqlMgrItem();
	if(!g_LogMgr.pSqlMgr)
	{
		LOGE("error!\n");
		return -1;
	}
	//checkWriteShutdownLog();
	Common_Lock_Create(&g_LogMgr.tLock,NULL);
	Common_Thread_Create(&g_LogMgr.hThread,__FUNCTION__,0,0,Thread_LogHandle,NULL);
	g_rest_res = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (g_rest_res)
	{
		pRoot = Common_Json_SetAttrValue(g_rest_res,-1,"EventLog",Common_Json_Type_Object,NULL,0,0);
		set_method_callback(&pRoot,get_log_top_res,NULL,NULL,NULL);
		if(pRoot){
			pChild = Common_Json_SetAttrValue(pRoot,-1,"LogFunction",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_log_res,put_log_res,post_log_res,delete_log_res);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"AlarmFunction",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_alarm_res,put_alarm_res,post_alarm_res,delete_alarm_res);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"DeleteAllLog",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,NULL,NULL,delete_allLog_res);
		}
	}
	return 0;
}

int ovfs_deal_log_res(int method, char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
	int iRet = -1;
	iRet = AnalyzeUriAndMakeResult(method,pUri,pAddData,outJson);
	return iRet;
}
