#include "libcommon_api.h"
#include "libmodule_api.h"
#include "libmodule_struct.h"
#include "module_register.h"

#ifdef WIN32
#define REGISTER_FILE_PATH "d:/tmp/modules/registers/"
#else
#define REGISTER_FILE_PATH "/tmp/modules/registers/"
#endif



S32 Module_Register_Load(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL;
	LibModuleRegInfo_T *pRegNode = NULL;
	int nWhich;
	S8 *pJsonString = NULL;
	S32 nLen,nValueNumber;
	S32 nRegCount = 0,nIdx;
	S8 *pValueString=NULL;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	{//
		char szFileName[128];
		FILE*pFD;
		sprintf(szFileName,"%s/%s.json",REGISTER_FILE_PATH,pModuleMgr->pszModuleName);
#ifdef WIN32
		if (0 != _access(szFileName,0))
#else
		if (0 != access(szFileName,F_OK))
#endif
		{
			return -1;
		}
		pFD = Common_File_fOpen(szFileName,"rb");
		if (pFD == NULL)
		{
			return -1;
		}
		Common_File_fSeek(pFD,0,SEEK_END);
		nLen = Common_File_fTell(pFD);
		Common_File_fSeek(pFD,0,SEEK_SET);
		if (nLen <= 0)
		{
			Common_File_fClose(pFD);
			pFD = NULL;
			return 0;
		}

		pJsonString = (S8 *)Common_Malloc(nLen,0,__FUNCTION__,__LINE__);
		if (pJsonString == NULL)
		{
			Common_File_fClose(pFD);
			pFD = NULL;
			return -1;
		}
		if(nLen != fread(pJsonString,1,nLen,pFD))
		{
			Common_File_fClose(pFD);
			pFD = NULL;
			Common_Free(pJsonString,__FUNCTION__,__LINE__);
			return -1;
		}
		Common_File_fClose(pFD);
		pFD = NULL;
		// parse
		pSaveJson = Common_Json_Parse(pJsonString,NULL,NULL);
		Common_Free(pJsonString,__FUNCTION__,__LINE__);
		pJsonString = NULL;
		if (pSaveJson == NULL)
		{
			return -1;
		}
	}
	// 开始恢复
	nValueNumber = 0;
	Common_Json_GetAttrValue(pSaveJson,-1,"RegRefreshFlag",NULL,NULL,&nValueNumber,NULL);
	pModuleMgr->dwRegRefreshFlag = nValueNumber;

	pArray = Common_Json_GetItem(pSaveJson,-1,"RegsList");
	if (pArray != NULL)
	{
		nRegCount = Common_Json_Size(pArray);
		nWhich = 0;
		for(nWhich = 0; nWhich < nRegCount;nWhich++)
		{
			pValueString = NULL;
			Common_Json_GetAttrValue(pArray,nWhich,"ModuleName",NULL,&pValueString,NULL,NULL);
			if (pValueString != NULL)
			{
				nValueNumber = 0;
				Common_Json_GetAttrValue(pArray,nWhich,"ModuleId",NULL,NULL,&nValueNumber,NULL);
				nIdx = nValueNumber & 0xFF;
				if (nIdx < 0 || nIdx >= LIBMODULE_MAX_CLIENT_NUM ||
					pModuleMgr->pModuleRegInfoList[nIdx] != NULL)
				{
					continue;
				}

				pRegNode = (LibModuleRegInfo_T *)Common_Malloc(sizeof(LibModuleRegInfo_T),0,__FUNCTION__,__LINE__);
				if (pRegNode == NULL)
				{
					continue;
				}
				memset(pRegNode,0,sizeof(LibModuleRegInfo_T));
				pRegNode->pszModuleName = Common_StrDup(pValueString,__FUNCTION__,__LINE__);
				if (pRegNode->pszModuleName == NULL)
				{
					Common_Free(pRegNode,__FUNCTION__,__LINE__);
					pRegNode = NULL;
					continue;
				}
				
				pRegNode->uModuleID = nValueNumber;
				pValueString = NULL;
				Common_Json_GetAttrValue(pArray,nWhich,"Domain",NULL,&pValueString,NULL,NULL);
				pRegNode->pszDomain = Common_StrDup(pValueString,__FUNCTION__,__LINE__);
				pValueString = NULL;
				Common_Json_GetAttrValue(pArray,nWhich,"RemoteDomain",NULL,&pValueString,NULL,NULL);
				pRegNode->pszIpv4 = Common_StrDup(pValueString,__FUNCTION__,__LINE__);
				pValueString = NULL;
				Common_Json_GetAttrValue(pArray,nWhich,"Mac",NULL,&pValueString,NULL,NULL);
				pRegNode->pszMac = Common_StrDup(pValueString,__FUNCTION__,__LINE__);
				pValueString = NULL;
				Common_Json_GetAttrValue(pArray,nWhich,"SerialNumber",NULL,&pValueString,NULL,NULL);
				pRegNode->pszSerialNumber = Common_StrDup(pValueString,__FUNCTION__,__LINE__);
				nValueNumber = 0;
				Common_Json_GetAttrValue(pArray,nWhich,"Port",NULL,NULL,&nValueNumber,NULL);
				pRegNode->nPort = nValueNumber;
				nValueNumber = 0;
				Common_Json_GetAttrValue(pArray,nWhich,"IsOnline",NULL,NULL,&nValueNumber,NULL);
				pRegNode->bOnline = nValueNumber;
				nValueNumber = 0;
				Common_Json_GetAttrValue(pArray,nWhich,"ModuleMark",NULL,NULL,&nValueNumber,NULL);
				pRegNode->nModuleMark = nValueNumber;
				nValueNumber = 0;
				Common_Json_GetAttrValue(pArray,nWhich,"LastRegTime",NULL,NULL,&nValueNumber,NULL);
				pRegNode->nLastRegTime = nValueNumber;
				nValueNumber = 0;
				Common_Json_GetAttrValue(pArray,nWhich,"LastAliveTime",NULL,NULL,&nValueNumber,NULL);
				pRegNode->nLastAliveTime = nValueNumber;
				// 更新时间
				Common_GetSystemCount(&pRegNode->nLastAliveTime,NULL);
				Common_Lock(pModuleMgr->hRegModuleLock);
				if (pModuleMgr->pModuleRegInfoTail == NULL)
				{
					pModuleMgr->pModuleRegInfoTail = 
					pModuleMgr->pModuleRegInfoHead = pRegNode;
				}
				else
				{
					pModuleMgr->pModuleRegInfoTail->pNext = pRegNode;
					pRegNode->pPrev = pModuleMgr->pModuleRegInfoTail;
					pModuleMgr->pModuleRegInfoTail = pRegNode;
				}
				pModuleMgr->pModuleRegInfoList[nIdx] = pRegNode;
				pModuleMgr->nRegModuleNum++;
				Common_UnLock(pModuleMgr->hRegModuleLock);
				
				
			}
		}
	}

	Common_Json_Delete(pSaveJson);
	pSaveJson = NULL;



	return 0;
}

S32 Module_Register_Save(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL;
	LibModuleRegInfo_T *pRegNode = NULL;
	int nWhich;
	S8 *pJsonString = NULL;
	S32 nLen;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	pSaveJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pSaveJson != NULL)
	{
		Common_Lock(pModuleMgr->hRegModuleLock);
		if (pModuleMgr->nRegModuleNum > 0)
		{
			Common_Json_SetAttrValue(pSaveJson,-1,"RegRefreshFlag",Common_Json_Type_Number,NULL,pModuleMgr->dwRegRefreshFlag,0);
			pArray = Common_Json_SetAttrValue(pSaveJson,-1,"RegsList",Common_Json_Type_Array,NULL,0,0);
			if (pArray != NULL)
			{
				Common_Time_T tComTime;
				S8 szTmp[128];
				time_t tLinuxTime;
				pRegNode = pModuleMgr->pModuleRegInfoHead;
				nWhich = 0;
				while (pRegNode != NULL)
				{
					Common_Json_SetAttrValue(pArray,nWhich,"ModuleId",Common_Json_Type_Number,NULL,pRegNode->uModuleID,0);
					Common_Json_SetAttrValue(pArray,nWhich,"ModuleName",Common_Json_Type_String,pRegNode->pszModuleName,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"Domain",Common_Json_Type_String,pRegNode->pszDomain,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"RemoteDomain",Common_Json_Type_String,pRegNode->pszIpv4,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"Mac",Common_Json_Type_String,pRegNode->pszMac,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"SerialNumber",Common_Json_Type_String,pRegNode->pszSerialNumber,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"Port",Common_Json_Type_Number,NULL,pRegNode->nPort,0);
					Common_Json_SetAttrValue(pArray,nWhich,"IsOnline",Common_Json_Type_Number,NULL,pRegNode->bOnline,0);
					Common_Json_SetAttrValue(pArray,nWhich,"LastAliveTime",Common_Json_Type_Number,NULL,pRegNode->nLastAliveTime,0);
					Common_Json_SetAttrValue(pArray,nWhich,"ModuleMark",Common_Json_Type_Number,NULL,pRegNode->nModuleMark,0);

					Common_Json_SetAttrValue(pArray,nWhich,"LastRegTime",Common_Json_Type_Number,NULL,pRegNode->nLastRegTime,0);
					tLinuxTime = pRegNode->nLastRegTime;
					Common_Linux2CommonTime(tLinuxTime,&tComTime);
					sprintf(szTmp,"%04d-%02d-%02d %02d:%02d:%02d",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
					Common_Json_SetAttrValue(pArray,nWhich,"LastRegTimeStr",Common_Json_Type_String,szTmp,0,0);
					nWhich++;
					pRegNode = pRegNode->pNext;
				}
			}
		}
		Common_UnLock(pModuleMgr->hRegModuleLock);
		// 写文件
		nLen = 0;
		pJsonString = Common_Json_Print(pSaveJson,&nLen);
		if (pJsonString != NULL)
		{
			if (!Common_File_MkDir(REGISTER_FILE_PATH))
			{//
				char szFileName[128];
				FILE*pFD;
				sprintf(szFileName,"%s/%s.json",REGISTER_FILE_PATH,pModuleMgr->pszModuleName);
				pFD = Common_File_fOpen(szFileName,"wb+");
				if (pFD != NULL)
				{
					Common_File_fWrite(pJsonString,1,nLen + 1,pFD);
					Common_File_fClose(pFD);
					pFD = NULL;
				}
			}
			Common_Free(pJsonString,__FUNCTION__,__LINE__);
			pJsonString = NULL;
		}

		Common_Json_Delete(pSaveJson);
		pSaveJson = NULL;

	}


	return 0;

}

S32 Module_Register_Init(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char sztmp[128];
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	sprintf(sztmp,"%s_Module_Register_lock",pModuleMgr->pszModuleName);
	Common_Lock_Create(&pModuleMgr->hRegModuleLock,sztmp);
	Module_Register_Load(hModuleHandle);
	// 检查是否有update
	LibModuleRegInfo_T *pRegNode = NULL;
	S32 nIdx;
	Common_Lock(pModuleMgr->hRegModuleLock);
	pRegNode = pModuleMgr->pModuleRegInfoHead;
	while(pRegNode != NULL)
	{
		if (0 == Common_StriCmp("update",pRegNode->pszModuleName))
		{
			pRegNode->bOnline = 1;
			break;
		}
		pRegNode = pRegNode->pNext;
	}
	if (pRegNode == NULL)
	{
		pRegNode = (LibModuleRegInfo_T *)Common_Malloc(sizeof(LibModuleRegInfo_T),0,__FUNCTION__,__LINE__);
		if (pRegNode != NULL)
		{
			memset(pRegNode,0,sizeof(LibModuleRegInfo_T));
			nIdx = pModuleMgr->nRegModuleNum;
			pRegNode->bOnline = 1;
			pRegNode->nPort = 10008;
			pRegNode->pszModuleName = (char *)Common_StrDup("update",__FUNCTION__,__LINE__);
			pRegNode->pszIpv4 = (char *)Common_StrDup("127.0.0.1",__FUNCTION__,__LINE__);
			S32 nSec = 0;
			Common_GetSystemCount(&nSec,NULL);
			Common_GetCurrentTime(&pRegNode->nLastRegTime,NULL);
			pRegNode->uModuleID = (nIdx) | (nSec << 8);
			pRegNode->uModuleID &= 0x7FFFFFFF;
			pRegNode->nLastAliveTime = nSec;
			if (pModuleMgr->pModuleRegInfoTail == NULL)
			{
				pModuleMgr->pModuleRegInfoTail = 
				pModuleMgr->pModuleRegInfoHead = pRegNode;
			}
			else
			{
				pModuleMgr->pModuleRegInfoTail->pNext = pRegNode;
				pRegNode->pPrev = pModuleMgr->pModuleRegInfoTail;
				pModuleMgr->pModuleRegInfoTail = pRegNode;
			}
			
			pModuleMgr->pModuleRegInfoList[nIdx] = pRegNode;
			pModuleMgr->nRegModuleNum++;
			
		}
	}
	
	Common_UnLock(pModuleMgr->hRegModuleLock);
	
	return 0;
}

S32 Module_Register_Offline_Check(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleRegInfo_T *pReg;
	LibModuleRegInfo_T *pReg_del,*pReg_del_head = NULL,*pReg_next = NULL,*pReg_prev = NULL;
	S32 nSec,nMSec = 0;
	if (!pModuleMgr->bManager)
	{
		return 0;
	}
	S32 bChange = 0;
	// 检查 注册的模块是否掉线
	Common_GetSystemCount(&nSec,&nMSec);
	Common_Lock(pModuleMgr->hRegModuleLock);
	pReg = pModuleMgr->pModuleRegInfoHead;
	while(pReg != NULL)
	{
		if (!pReg->bOnline)
		{
			break;
		}
		//MODULE_LOGD("[%s] check Online %d %d\n",pReg->pszModuleName,pReg->nLastAliveTime,nSec);
		if (pReg->nLastAliveTime + LIBMODULE_HEART_INV + 10 < nSec && 0 != Common_StriCmp("Update",pReg->pszModuleName))
		{
			MODULE_LOGD("[%s] -> Offline\n",pReg->pszModuleName);

			if(pModuleMgr->fCallFunction != NULL)
			{
				cJSON_Struct *pInJson;
				S8 szTmp[128];
				pInJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if(pInJson != NULL)
				{
					Common_Json_SetAttrValue(pInJson,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
					sprintf(szTmp,"/%s/Notify/Offline",pModuleMgr->pszModuleName);
					Common_Json_SetAttrValue(pInJson,-1,"/Header/Uri",Common_Json_Type_String,szTmp,0,0);
					Common_Json_SetAttrValue(pInJson,-1,"/Data",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInJson,-1,"/Data/ModuleName",Common_Json_Type_String,pReg->pszModuleName,0,0);
					pModuleMgr->fCallFunction(hModuleHandle,pInJson,NULL,pModuleMgr->pCallUserData);
				}
				
			}
			
			pReg->bOnline = 0;
			pReg_next = pReg->pNext;
			pReg_prev = pReg->pPrev;
			// 移到最后
			if (pReg_next != NULL)
			{
				if (pReg_next->bOnline)
				{// 移动
					pReg_next->pPrev = pReg_prev;
					if (pReg_prev == NULL)
					{
						pModuleMgr->pModuleRegInfoHead = pReg_next;
					}
					else
					{
						 pReg_prev->pNext = pReg_next;
					}
					
					pReg->pPrev = pModuleMgr->pModuleRegInfoTail;
					pReg->pNext = NULL;
					pModuleMgr->pModuleRegInfoTail->pNext = pReg;
					pModuleMgr->pModuleRegInfoTail = pReg;
				}
			}
			bChange = 1;
			pReg = pReg_next;

			continue;

		}

		pReg = pReg->pNext;
	}
	if (bChange)
	{
		pModuleMgr->dwRegRefreshFlag++;
		pModuleMgr->bNeedReport = 1;
	}
	Common_UnLock(pModuleMgr->hRegModuleLock);
	if (pModuleMgr->bNeedReport)
	{// 有改变，通知更新
		MODULE_LOGD("Report all\n");
		
		pModuleMgr->bNeedReport = 0;
		
		return 1;
	}
	return 0;
}
S32 Module_Register_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pResponce = NULL,*pRegisterJson = NULL;
	S32 nIntValue = 0;
	char *pStringValue = NULL,*pModuleName = NULL,*pNewStringValue = NULL;
	S32 nRet = -1;
	S32 nExist = 0,nIdx = -1,i,bSave = 0;
	LibModuleRegInfo_T * pRegNode = NULL;
	U32 uModuleId = 0;
	S32 nModuleMark = 0;
	pRegisterJson= Common_Json_GetItem(pInParams,-1,"Module/Register");
	if (NULL == pRegisterJson)
	{
		return -1;
	}
		
	if (!pModuleMgr->bManager)
	{
		return 0;
	}
	nRet = 0;
	pStringValue = NULL;
	Common_Json_GetAttrValue(pRegisterJson,-1,"SystemName",NULL,&pStringValue,NULL,NULL);
	pModuleName = NULL;
	Common_Json_GetAttrValue(pRegisterJson,-1,"ModuleName",NULL,&pModuleName,NULL,NULL);
	if (pStringValue == NULL ||
		pModuleName == NULL ||
		0 != Common_StriCmp(pStringValue,pModuleMgr->pszSystemName))
	{
		if (pOutParams != NULL)
		{
			pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Register",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Register/Code",Common_Json_Type_Number,0,MODULE_ERROR_TYPE_REFUSED,0);
			*pOutParams = pResponce;
		}
		
		return 0;
	}
	// 检查是否已经注册
	Common_Lock(pModuleMgr->hRegModuleLock);
	pRegNode = pModuleMgr->pModuleRegInfoHead;
	while(pRegNode != NULL)
	{
		if (0 == Common_StriCmp(pRegNode->pszModuleName,pModuleName))
		{
			nExist = 1;
			break;
		}
		pRegNode = pRegNode->pNext;
	}
	if (nExist)
	{
		S32 bChange = 0;
		S32 nSec = 0;
		if (!pRegNode->bOnline)
		{
			pRegNode->bOnline = 1;
			if (pRegNode->pPrev != NULL)
			{
				if (!pRegNode->pPrev->bOnline)
				{ // 放前
					pRegNode->pPrev->pNext = pRegNode->pNext;
					if (pRegNode->pNext != NULL)
					{
						pRegNode->pNext->pPrev = pRegNode->pPrev;
					}
					else
					{
						pModuleMgr->pModuleRegInfoTail = pRegNode->pPrev;
					}
					pRegNode->pNext = pModuleMgr->pModuleRegInfoHead;
					pModuleMgr->pModuleRegInfoHead->pPrev = pRegNode;
					pModuleMgr->pModuleRegInfoHead = pRegNode;
					pRegNode->pPrev = NULL;
				}
			}
			if(pModuleMgr->fCallFunction != NULL)
			{
				cJSON_Struct *pInJson;
				S8 szTmp[128];
				pInJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if(pInJson != NULL)
				{
					Common_Json_SetAttrValue(pInJson,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
					sprintf(szTmp,"/%s/Notify/Online",pModuleMgr->pszModuleName);
					Common_Json_SetAttrValue(pInJson,-1,"/Header/Uri",Common_Json_Type_String,szTmp,0,0);
					Common_Json_SetAttrValue(pInJson,-1,"/Data",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInJson,-1,"/Data/ModuleName",Common_Json_Type_String,pRegNode->pszModuleName,0,0);
					pModuleMgr->fCallFunction(hModuleHandle,pInJson,NULL,pModuleMgr->pCallUserData);
				}
				
			}
			
			bChange = 1;
		}
		Common_Json_GetAttrValue(pRegisterJson,-1,"ModuleMark",NULL,NULL,&nModuleMark,NULL);
		if (nModuleMark != pRegNode->nModuleMark)
		{// 不为同一个实例,更新信息
			pRegNode->nModuleMark = nModuleMark;
			pStringValue = NULL;
			Common_Json_GetAttrValue(pRegisterJson,-1,"LocalDomain",NULL,&pStringValue,NULL,NULL);
			if (pStringValue == NULL || 0 != Common_StriCmp(pStringValue,pRegNode->pszDomain))
			{
				Common_Free(pRegNode->pszDomain,__FUNCTION__,__LINE__);
				pRegNode->pszDomain = NULL;
				pRegNode->pszDomain =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				
			}
			pStringValue = NULL;// pClientInfo
			Common_Json_GetAttrValue(pClientInfo,-1,"RemoteDomain",NULL,&pStringValue,NULL,NULL);
			if (pStringValue == NULL ||0 != Common_StriCmp(pStringValue,pRegNode->pszIpv4))
			{
				Common_Free(pRegNode->pszIpv4,__FUNCTION__,__LINE__);
				pRegNode->pszIpv4 = NULL;
				pRegNode->pszIpv4 =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

			}
			pStringValue = NULL;
			Common_Json_GetAttrValue(pRegisterJson,-1,"Mac",NULL,&pStringValue,NULL,NULL);
			if (pStringValue == NULL ||0 != Common_StriCmp(pStringValue,pRegNode->pszMac))
			{
				Common_Free(pRegNode->pszMac,__FUNCTION__,__LINE__);
				pRegNode->pszMac = NULL;
				pRegNode->pszMac =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

			}
			Common_Json_GetAttrValue(pRegisterJson,-1,"SerialNumber",NULL,&pStringValue,NULL,NULL);
			if (pStringValue == NULL ||0 != Common_StriCmp(pStringValue,pRegNode->pszSerialNumber))
			{
				Common_Free(pRegNode->pszSerialNumber,__FUNCTION__,__LINE__);
				pRegNode->pszSerialNumber = NULL;
				pRegNode->pszSerialNumber =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

			}
			nIntValue = -1;
			Common_Json_GetAttrValue(pRegisterJson,-1,"ServerPort",NULL,NULL,&nIntValue,NULL);
			pRegNode->nPort = nIntValue;
			
			
			bChange = 1;
		}
		
		Common_GetSystemCount(&nSec,NULL);
		pRegNode->nLastAliveTime = nSec;
		uModuleId = pRegNode->uModuleID;
		// 检查注册信息是否改变,暂放
		//////////////////////////////////////////////////////////////////////////
		if (bChange)
		{
			pModuleMgr->dwRegRefreshFlag++;
			pModuleMgr->bNeedReport = 1;
		}
		Common_GetCurrentTime(&pRegNode->nLastRegTime,NULL);
		bSave = 1;

	}
	else
	{
		if (pModuleMgr->nRegModuleNum >= LIBMODULE_MAX_CLIENT_NUM)
		{
			nRet = MODULE_ERROR_TYPE_LIMITED; 
		}
		else
		{
			pRegNode = (LibModuleRegInfo_T *)Common_Malloc(sizeof(LibModuleRegInfo_T),0,__FUNCTION__,__LINE__);
			if (pRegNode == NULL)
			{
				nRet = MODULE_ERROR_TYPE_LIMITED; 
			}
			else
			{
				memset(pRegNode,0,sizeof(LibModuleRegInfo_T));
				pRegNode->pszModuleName = Common_StrDup(pModuleName,__FUNCTION__,__LINE__);
				nIntValue = 0;
				Common_Json_GetAttrValue(pRegisterJson,-1,"ServerPort",NULL,NULL,&nIntValue,NULL);
				pRegNode->nPort = nIntValue;
				nIntValue = 0;
				Common_Json_GetAttrValue(pRegisterJson,-1,"ModuleMark",NULL,NULL,&nIntValue,NULL);
				pRegNode->nModuleMark = nIntValue;
				pStringValue = NULL;
				Common_Json_GetAttrValue(pRegisterJson,-1,"LocalDomain",NULL,&pStringValue,NULL,NULL);
				if (pStringValue)
				{
					pRegNode->pszDomain = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
		

				pStringValue = NULL;
				Common_Json_GetAttrValue(pClientInfo,-1,"RemoteDomain",NULL,&pStringValue,NULL,NULL);
				if (pStringValue)
				{
					pRegNode->pszIpv4 = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
				pStringValue = NULL;
				Common_Json_GetAttrValue(pRegisterJson,-1,"Mac",NULL,&pStringValue,NULL,NULL);
				if (pStringValue)
				{
					pRegNode->pszMac = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
				pStringValue = NULL;
				Common_Json_GetAttrValue(pRegisterJson,-1,"SerialNumber",NULL,&pStringValue,NULL,NULL);
				if (pStringValue)
				{
					pRegNode->pszSerialNumber = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
				pRegNode->bOnline = 1;
				for (i = 0; i < LIBMODULE_MAX_CLIENT_NUM;i++)
				{
					if (pModuleMgr->pModuleRegInfoList[i] == NULL)
					{
						nIdx = i;
						break;
					}
				}
				if (nIdx != -1)
				{
					S32 nSec = 0;
					Common_GetSystemCount(&nSec,NULL);
					Common_GetCurrentTime(&pRegNode->nLastRegTime,NULL);
					pRegNode->uModuleID = (nIdx) | (nSec << 8);
					pRegNode->uModuleID &= 0x7FFFFFFF;
					uModuleId = pRegNode->uModuleID;
					pRegNode->nLastAliveTime = nSec;
					pRegNode->pNext = pModuleMgr->pModuleRegInfoHead;
					if (pModuleMgr->pModuleRegInfoHead == NULL)
					{
						pModuleMgr->pModuleRegInfoHead =  
						pModuleMgr->pModuleRegInfoTail = pRegNode;
					}
					else
					{
						pModuleMgr->pModuleRegInfoHead->pPrev = pRegNode;
						pRegNode->pNext = pModuleMgr->pModuleRegInfoHead;
						pModuleMgr->pModuleRegInfoHead = pRegNode;
					}

					pModuleMgr->pModuleRegInfoList[nIdx] = pRegNode;
					pModuleMgr->nRegModuleNum++;
					pModuleMgr->dwRegRefreshFlag++;
					pModuleMgr->bNeedReport = 1;
					bSave = 1;
					if(pModuleMgr->fCallFunction != NULL)
					{
						cJSON_Struct *pInJson;
						S8 szTmp[128];
						pInJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
						if(pInJson != NULL)
						{
							Common_Json_SetAttrValue(pInJson,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
							sprintf(szTmp,"/%s/Notify/Online",pModuleMgr->pszModuleName);
							Common_Json_SetAttrValue(pInJson,-1,"/Header/Uri",Common_Json_Type_String,szTmp,0,0);
							Common_Json_SetAttrValue(pInJson,-1,"/Data",Common_Json_Type_Object,NULL,0,0);
							Common_Json_SetAttrValue(pInJson,-1,"/Data/ModuleName",Common_Json_Type_String,pRegNode->pszModuleName,0,0);
							pModuleMgr->fCallFunction(hModuleHandle,pInJson,NULL,pModuleMgr->pCallUserData);
						}
						
					}

				}
			}

		}

	}
	Common_UnLock(pModuleMgr->hRegModuleLock);

	pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pResponce,-1,"Module/Register",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pResponce,-1,"Module/Register/Code",Common_Json_Type_Number,0,nRet,0);
	

	if (!nRet)
	{
		Common_Json_SetAttrValue(pResponce,-1,"Module/Register/CoreId",Common_Json_Type_Number,0,pModuleMgr->uModuleID,0);
		if (pModuleMgr->pszMac != NULL)
		{
			Common_Json_SetAttrValue(pResponce,-1,"Module/Register/CoreMac",Common_Json_Type_String,pModuleMgr->pszMac,0,0);
		}
		if (pModuleMgr->pszSerialNumber != NULL)
		{
			Common_Json_SetAttrValue(pResponce,-1,"Module/Register/CoreSerialNumber",Common_Json_Type_String,pModuleMgr->pszSerialNumber,0,0);
		}
		Common_Json_SetAttrValue(pResponce,-1,"Module/Register/ModuleId",Common_Json_Type_Number,0,uModuleId,0);
	}
	if (pOutParams)
	{
		*pOutParams = pResponce;
		pResponce = NULL;
	}
	if (pResponce)
	{
		Common_Json_Delete(pResponce);
		pResponce = NULL;
	}
	if (bSave)
	{
		Module_Register_Save(hModuleHandle);
	}
	// 应答注册
	return 0;

}

S32 Module_Register_Report_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pResponce = NULL,*pReportJson = NULL;
	S32 nIntValue = 0;
	char *pStringValue = NULL,*pModuleName = NULL;
	S32 nRet = -1;
	S32 nExist = 0,nIdx = -1,i;
	LibModuleRegInfo_T * pRegNode = NULL;
	U32 uModuleId = 0;
	S32 nSec = 0;
	S32 bOnline = 0,nArraySize,nModuleMark;
	cJSON_Struct *pArrayJson;
	U32 dwRegRefreshFlag = 0;
	pReportJson= Common_Json_GetItem(pInParams,-1,"Module/Report");
	if (NULL == pReportJson)
	{
		return -1;
	}
	if (pModuleMgr->bManager)
	{
		return 0;
	}
	nIntValue = 0;
	Common_Json_GetAttrValue(pReportJson,-1,"CoreId",NULL,NULL,&nIntValue,NULL);
	if (pModuleMgr->uCoreID != (U32 )nIntValue)
	{// 需要重新注册
		pModuleMgr->bRegister = 0;
		return 0;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pReportJson,-1,"CoreDomain",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		if (pModuleMgr->pszCoreDomain != NULL)
		{
			if (0 != stricmp(pModuleMgr->pszCoreDomain,pStringValue))
			{
				Common_Free(pModuleMgr->pszCoreDomain,__FUNCTION__,__LINE__);
				pModuleMgr->pszCoreDomain = NULL;
			}
		}
		if (pModuleMgr->pszCoreDomain == NULL)
		{
			pModuleMgr->pszCoreDomain = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
		}
	}
	nIntValue = 0;
	Common_Json_GetAttrValue(pReportJson,-1,"RegRefreshFlag",NULL,NULL,(S32 *)&dwRegRefreshFlag,NULL);
	nArraySize = 0;
	pModuleMgr->dwRegRefreshFlag = dwRegRefreshFlag;
	pArrayJson = Common_Json_GetItem(pReportJson,-1,"Lists");
	if (pArrayJson != NULL)
	{
		nArraySize = Common_Json_Size(pArrayJson);
		if (nArraySize > 0)
		{
			for (i = 0; i < nArraySize;i++)
			{
				nIntValue = -1;
				Common_Json_GetAttrValue(pArrayJson,i,"ModuleId",NULL,NULL,&nIntValue,NULL);
				nIdx = nIntValue & 0xFF;
				if (nIdx < 0 || nIdx >= LIBMODULE_MAX_CLIENT_NUM)
				{
					continue;
				}
				Common_Json_GetAttrValue(pArrayJson,i,"Online",NULL,NULL,&bOnline,NULL);
				Common_Lock(pModuleMgr->hRegModuleLock);

				{
					LibModuleRegInfo_T *p = NULL;

					if (pModuleMgr->pModuleRegInfoList[nIdx] == NULL)
					{// 新的
						p = (LibModuleRegInfo_T *)Common_Malloc(sizeof(LibModuleRegInfo_T),0,__FUNCTION__,__LINE__);
						if (p != NULL)
						{
							memset(p,0,sizeof(LibModuleRegInfo_T));
							p->uModuleID = (U32)nIntValue;
							p->bOnline = bOnline;
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"ModuleName",NULL,&pStringValue,&nIntValue,NULL);
							if (pStringValue != NULL)
							{
								p->pszModuleName = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"Mac",NULL,&pStringValue,&nIntValue,NULL);
							if (pStringValue != NULL)
							{
								p->pszMac = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"SerialNumber",NULL,&pStringValue,&nIntValue,NULL);
							if (pStringValue != NULL)
							{
								p->pszSerialNumber = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"LocalDomain",NULL,&pStringValue,&nIntValue,NULL);
							if (pStringValue != NULL)
							{
								p->pszDomain = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"RemoteDomain",NULL,&pStringValue,&nIntValue,NULL);
							if (pStringValue != NULL)
							{
								p->pszIpv4 = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
							}
							nIntValue = -1;
							Common_Json_GetAttrValue(pArrayJson,i,"ServerPort",NULL,&pStringValue,&nIntValue,NULL);
							if (nIntValue != -1)
							{
								p->nPort =nIntValue;
							}
							nIntValue = -1;
							Common_Json_GetAttrValue(pArrayJson,i,"ModuleMark",NULL,&pStringValue,&nIntValue,NULL);
							if (nIntValue != -1)
							{
								p->nModuleMark =nIntValue;
							}
							if (bOnline)
							{// 放前
								
								if (pModuleMgr->pModuleRegInfoHead == NULL)
								{
									pModuleMgr->pModuleRegInfoHead = pModuleMgr->pModuleRegInfoTail = p;
								}
								else
								{
									p->pNext = pModuleMgr->pModuleRegInfoHead;
									pModuleMgr->pModuleRegInfoHead->pPrev = p;
									pModuleMgr->pModuleRegInfoHead = p;
								}
							}
							else
							{
								// 放尾
								if(pModuleMgr->pModuleRegInfoTail != NULL)
								{
									pModuleMgr->pModuleRegInfoTail->pNext = p;
									p->pPrev = pModuleMgr->pModuleRegInfoTail;
									pModuleMgr->pModuleRegInfoTail = p;
								}
								else
								{
									pModuleMgr->pModuleRegInfoTail = 
									pModuleMgr->pModuleRegInfoHead = p;
								}
							}

							pModuleMgr->pModuleRegInfoList[nIdx] = p;
							pModuleMgr->nRegModuleNum++;
						}
					}
					else
					{
						// 存在,只更新状态。不考虑参数改变
						Common_Json_GetAttrValue(pArrayJson,i,"ModuleMark",NULL,NULL,&nModuleMark,NULL);
						p = pModuleMgr->pModuleRegInfoList[nIdx];
						if (p->nModuleMark != nModuleMark)
						{
							pRegNode = p;
							pRegNode->nModuleMark = nModuleMark;
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"LocalDomain",NULL,&pStringValue,NULL,NULL);
							if (0 != Common_StriCmp(pStringValue,pRegNode->pszDomain))
							{
								Common_Free(pRegNode->pszDomain,__FUNCTION__,__LINE__);
								pRegNode->pszDomain = NULL;
								pRegNode->pszDomain =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"RemoteDomain",NULL,&pStringValue,NULL,NULL);
							if (0 != Common_StriCmp(pStringValue,pRegNode->pszIpv4))
							{
								Common_Free(pRegNode->pszIpv4,__FUNCTION__,__LINE__);
								pRegNode->pszIpv4 = NULL;
								pRegNode->pszIpv4 =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"Mac",NULL,&pStringValue,NULL,NULL);
							if (0 != Common_StriCmp(pStringValue,pRegNode->pszMac))
							{
								Common_Free(pRegNode->pszMac,__FUNCTION__,__LINE__);
								pRegNode->pszMac = NULL;
								pRegNode->pszMac =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

							}
							pStringValue = NULL;
							Common_Json_GetAttrValue(pArrayJson,i,"SerialNumber",NULL,&pStringValue,NULL,NULL);
							if (0 != Common_StriCmp(pStringValue,pRegNode->pszSerialNumber))
							{
								Common_Free(pRegNode->pszSerialNumber,__FUNCTION__,__LINE__);
								pRegNode->pszSerialNumber = NULL;
								pRegNode->pszSerialNumber =  Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

							}
							Common_Json_GetAttrValue(pArrayJson,i,"ServerPort",NULL,NULL,&nIntValue,NULL);
							pRegNode->nPort = nIntValue;
							
						}
						p->bOnline = bOnline;
						if (bOnline)
						{
							// 
							if (p->pPrev != NULL)
							{
								if (!p->pPrev->bOnline)
								{ // 放前
									p->pPrev->pNext = p->pNext;
									if (p->pNext != NULL)
									{
										p->pNext->pPrev = p->pPrev;
									}
									else
									{
										pModuleMgr->pModuleRegInfoTail = p->pPrev;
									}
									p->pNext = pModuleMgr->pModuleRegInfoHead;
									pModuleMgr->pModuleRegInfoHead->pPrev = p;
									pModuleMgr->pModuleRegInfoHead = p;
									p->pPrev = NULL;
								}
							}
						}
						else if (p->pNext != NULL)
						{
							if (p->pNext->bOnline)
							{// 放后
								if (p->pPrev == NULL)
								{
									pModuleMgr->pModuleRegInfoHead = p->pNext;
									pModuleMgr->pModuleRegInfoHead->pPrev = NULL;
									pModuleMgr->pModuleRegInfoTail->pNext = p;
									p->pPrev = pModuleMgr->pModuleRegInfoTail;
									p->pNext = NULL;
									pModuleMgr->pModuleRegInfoTail = p;

								}
								else
								{
									p->pNext->pPrev = p->pPrev;
									p->pPrev->pNext = p->pNext;
									p->pNext = NULL;
									p->pPrev = NULL;
									pModuleMgr->pModuleRegInfoTail->pNext = p;
									p->pPrev = pModuleMgr->pModuleRegInfoTail;
									pModuleMgr->pModuleRegInfoTail = p;
								}
							}
						}
					}
				}
				Common_UnLock(pModuleMgr->hRegModuleLock);

			}
		}
	}
	pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pResponce,-1,"Module/Report",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pResponce,-1,"Module/Report/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_SUCC,0);
	if (pOutParams)
	{
		*pOutParams = pResponce;
		pResponce = NULL;
	}
	if (pResponce)
	{
		Common_Json_Delete(pResponce);
		pResponce = NULL;
	}
	return 0;
}

S32 Module_Register_Heart_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pResponce = NULL,*pHeartJson = NULL;
	S32 nIntValue = 0;
	char *pStringValue = NULL,*pModuleName = NULL;
	S32 nRet = -1;
	S32 nExist = 0,nIdx = -1;
	LibModuleRegInfo_T * pRegNode = NULL;
	U32 uModuleId = 0;
	S32 nModuleMark = 0;
	S32 nSec = 0;
	pHeartJson= Common_Json_GetItem(pInParams,-1,"Module/Heart");
	if (NULL == pHeartJson)
	{
		return -1;
	}
	if (!pModuleMgr->bManager)
	{
		MODULE_LOGD("Here\n");
		return 0;
	}
	Common_GetSystemCount(&nSec,NULL);
	// 检测模块名及ID一致性
	nIntValue = 0;
	Common_Json_GetAttrValue(pHeartJson,-1,"ModuleId",NULL,NULL,&nIntValue,NULL);
	pStringValue = NULL;
	Common_Json_GetAttrValue(pHeartJson,-1,"ModuleName",NULL,&pStringValue,NULL,NULL);
	nIdx = nIntValue & 0xFF;
	if(nIdx < LIBMODULE_MAX_CLIENT_NUM)
	{
		Common_Lock(pModuleMgr->hRegModuleLock);
		if (pModuleMgr->pModuleRegInfoList[nIdx] == NULL)
		{
			Common_UnLock(pModuleMgr->hRegModuleLock);

			pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Heart",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Heart/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_NOTFOUND,0);
			if (pOutParams)
			{
				*pOutParams = pResponce;
				pResponce = NULL;
			}
			if (pResponce)
			{
				Common_Json_Delete(pResponce);
				pResponce = NULL;
			}
			return 0;
		}
		if (pModuleMgr->pModuleRegInfoList[nIdx]->uModuleID != (U32 )nIntValue || 
			0 != Common_StriCmp(pStringValue,pModuleMgr->pModuleRegInfoList[nIdx]->pszModuleName))
		{
			Common_UnLock(pModuleMgr->hRegModuleLock);
			//MODULE_LOGD("Here [%d ,%d] [%s,%s]\n",pModuleMgr->pModuleRegInfoList[nIdx]->uModuleID,(U32 )nIntValue,pStringValue,pModuleMgr->pModuleRegInfoList[nIdx]->pszModuleName);
			pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Heart",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Heart/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_MISMATCH,0);
			if (pOutParams)
			{
				*pOutParams = pResponce;
				pResponce = NULL;
			}
			if (pResponce)
			{
				Common_Json_Delete(pResponce);
				pResponce = NULL;
			}
			return 0;
		}
		Common_Json_GetAttrValue(pHeartJson,-1,"ModlueMark",NULL,NULL,&nModuleMark,NULL);
		if (pModuleMgr->pModuleRegInfoList[nIdx]->nModuleMark != nModuleMark ||
			(!pModuleMgr->pModuleRegInfoList[nIdx]->bOnline))
		{// 离线状态的，可能中间卡住未收到心跳
			Common_UnLock(pModuleMgr->hRegModuleLock);
			pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Heart",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Module/Heart/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_MISMATCH,0);
			if (pOutParams)
			{
				*pOutParams = pResponce;
				pResponce = NULL;
			}
			if (pResponce)
			{
				Common_Json_Delete(pResponce);
				pResponce = NULL;
			}
			return 0;
		}

		pModuleMgr->pModuleRegInfoList[nIdx]->nLastAliveTime = nSec;
		Common_Json_GetAttrValue(pHeartJson,-1,"RegRefreshFlag",NULL,NULL,&nIntValue,NULL);
		if (pModuleMgr->dwRegRefreshFlag != (U32)nIntValue)
		{
			pModuleMgr->bNeedReport = 1;
		}
		Common_UnLock(pModuleMgr->hRegModuleLock);
		pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResponce,-1,"Module",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResponce,-1,"Module/Heart",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResponce,-1,"Module/Heart/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_SUCC,0);
		if (pOutParams)
		{
			*pOutParams = pResponce;
			pResponce = NULL;
		}
		if (pResponce)
		{
			Common_Json_Delete(pResponce);
			pResponce = NULL;
		}
	}
	return 0;
}