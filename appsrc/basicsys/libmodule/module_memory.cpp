#include "libcommon_api.h"
#include "libmodule_struct.h"
#include "libmodule_api.h"
#include "module_memory.h"
S32 Module_Memory_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{// 返回 0-表示已处理
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char *pStringValue;
	char *pModuleName = NULL;
	S8 szTmp[128],*pDeal = NULL;
	S32 nLen;
	S32 nCode = -1,nSubIdx,nUserIdx,nCount,nFreeIdx;
	S8 *pSubScribeUri = NULL,*pResponceUri = NULL,*pRessUri = NULL;
	LibModuleSubscribeOwner_T *pOwnInfo;
	LibModuleSubscribeUser_T *pUserInfo;
	cJSON_Struct *pOutParamsJson = NULL;
	S32 bSave = 0;
	S32 nSec = 0;
	S32 nOwnerId = 0,nUserId = 0;
	if (pModuleMgr == NULL || pInParams == NULL)
	{
		return -1;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pStringValue,NULL,NULL);
	if (pStringValue == NULL)
	{
		return -1;
	}
	nLen = sprintf(szTmp,"/%s/ModuleDebug/Memory",pModuleMgr->pszModuleName);
	if (0 != Common_StrniCmp(pStringValue,szTmp,nLen))
	{// 非本模块
		return -1;
	}
	pDeal = pStringValue + nLen;

	 if (0 == Common_StriCmp(pDeal,"/StartDebug"))
	{// 
		S32 bAutoPrint = 0;
		S32 nIntervalSec = 60;
		S8 *pFilePath = NULL;
		Common_Json_GetAttrValue(pInParams,-1,"Data/AutoPrint",NULL,NULL,&bAutoPrint,NULL);
		Common_Json_GetAttrValue(pInParams,-1,"Data/IntervalSec",NULL,NULL,&nIntervalSec,NULL);
		Common_Json_GetAttrValue(pInParams,-1,"Data/FileName",NULL,&pFilePath,NULL,NULL);
		Common_Memory_StartDebug(bAutoPrint,nIntervalSec,pFilePath);
		pOutParamsJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pOutParamsJson != NULL)
		{
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
		}
		nCode = 0; 
	}
	else  if (0 == Common_StriCmp(pDeal,"/DebugPrint"))
	{// 
		S8 *pFilePath = NULL;
		Common_Json_GetAttrValue(pInParams,-1,"Data/FileName",NULL,&pFilePath,NULL,NULL);
		Common_Memory_Print(pFilePath);
		pOutParamsJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pOutParamsJson != NULL)
		{
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
		}
		nCode = 0; 
	}
	else if (0 == Common_StriCmp(pDeal,"/StopDebug"))
	{//
		Common_Memory_StopDebug();
		pOutParamsJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pOutParamsJson != NULL)
		{
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
		}
		nCode = 0; 
	}

	 
	if (nCode != 0)
	{
		pOutParamsJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pOutParamsJson != NULL)
		{
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutParamsJson,-1,"Header/Code",Common_Json_Type_Number,0,nCode,0);
		}
	}
	if (pOutParamsJson != NULL)
	{
		if (pOutParams != NULL)
		{
			*pOutParams = pOutParamsJson;
			pOutParamsJson = NULL;
		}
	}
	Common_Json_Delete(pOutParamsJson);
	pOutParamsJson = NULL;
	
	return 0;
}
