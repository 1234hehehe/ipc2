#include "libcommon_api.h"
#include "libmodule_struct.h"
#include "libmodule_api.h"
#ifdef WIN32
#define URIPROXY_FILE_PATH "d:/tmp/modules/uriproxy/"
#else
#define URIPROXY_FILE_PATH "/tmp/modules/uriproxy/"
#endif

/*
uri:/Core/UriProxy/Register
Method:Post/Delete
InData:
{
  ResList:[
  { Uri:xxx,
   ProxyUri:xxx
   }
   ]
}

uri:/Broadcast/UriProxy/Update
Method:Put
InData:
{
ResList:[{
	Uri:xxx,
	ProxyUri:xxx
}]
RefreshFlag:xxx // 用此判断 是否需要更新
}
*/
extern S32 extern_Module_CallFunctions_MsgRouter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);

S32 Module_UriProxy_Load(ModuleHandle_T hModuleHandle)
{// 导入记录
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL;
	int nWhich;
	S8 *pJsonString = NULL,*pValueString;
	S8 *pUri = NULL,*pProxyUri = NULL;
	S32 nLen,nValueNumber;
	//S32 nUserCount = 0;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	{//
		char szFileName[128];
		FILE*pFD;
		sprintf(szFileName,"%s/%s.json",URIPROXY_FILE_PATH,pModuleMgr->pszModuleName);
#ifdef WIN32
		if (0 != _access(szFileName,0))
#else
		if (0 != access(szFileName,F_OK))
#endif
		{
			return -1;
		}
		pFD = Common_File_fOpen(szFileName,(char*)"rb");
		if (pFD == NULL)
		{
			return -1;
		}
		fseek(pFD,0,SEEK_END);
		nLen = ftell(pFD);
		fseek(pFD,0,SEEK_SET);
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
		if((U32)nLen != fread(pJsonString,1,nLen,pFD))
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
	Common_Json_GetAttrValue(pSaveJson,-1,"RefreshFlag",NULL,&pValueString,&nValueNumber,NULL);
	pModuleMgr->dwProxyRefreshFlag = nValueNumber;

	pArray = Common_Json_GetItem(pSaveJson,-1,"ProxyList");
	if (pArray != NULL)
	{
		S32 nOwerCount = 0;
		nOwerCount = Common_Json_Size(pArray);
		nWhich = 0;
		for(nWhich = 0; nWhich < nOwerCount;nWhich++)
		{
			pUri = NULL;
			pProxyUri = NULL;
			Common_Json_GetAttrValue(pArray,nWhich,"Uri",NULL,&pUri,NULL,NULL);
			Common_Json_GetAttrValue(pArray,nWhich,"ProxyUri",NULL,&pProxyUri,NULL,NULL);
			if (pUri != NULL && pProxyUri != NULL)
			{
				LibModuleProxyInfo_T *pNode = NULL;
				S32 bExist = 0;
				Common_Lock(pModuleMgr->hProxyLock);
				pNode = pModuleMgr->pResProxyListHead;
				while(pNode != NULL)
				{
					if (0 == Common_StriCmp(pNode->pResUri,pUri))
					{
						if (0 == Common_StriCmp(pNode->pProxyUri,pProxyUri))
						{
						}
						else
						{
							// 修改
							S8 *pNewUri;
							pNewUri = Common_StrDup(pProxyUri,__FUNCTION__,__LINE__);
							if (pNewUri != NULL)
							{
								Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
								pNode->pProxyUri = pNewUri;
								pModuleMgr->dwProxyRefreshFlag++;

							}
						}
						bExist = 1;
						break;
					}
					pNode = pNode->pNext;
				}
				if (!bExist)
				{// 新增
					pNode = (LibModuleProxyInfo_T *)Common_Malloc(sizeof(LibModuleProxyInfo_T),0,__FUNCTION__,__LINE__);
					if (pNode != NULL)
					{
						memset(pNode,0,sizeof(LibModuleProxyInfo_T));
						pNode->pResUri = Common_StrDup(pUri,__FUNCTION__,__LINE__);
						pNode->pProxyUri = Common_StrDup(pProxyUri,__FUNCTION__,__LINE__);
						if (pNode->pResUri == NULL || pNode->pProxyUri == NULL)
						{
							Common_Free(pNode->pResUri,__FUNCTION__,__LINE__);
							Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
							Common_Free(pNode,__FUNCTION__,__LINE__);
						}
						else
						{
							pNode->pNext = pModuleMgr->pResProxyListHead;
							if (pModuleMgr->pResProxyListHead != NULL)
							{
								pModuleMgr->pResProxyListHead->pPrev = pNode;
							}

						}
					}
				}
				Common_UnLock(pModuleMgr->hProxyLock);
			}
		}
	}

	Common_Json_Delete(pSaveJson);
	pSaveJson = NULL;



	return 0;

}

S32 Module_UriProxy_Save(ModuleHandle_T hModuleHandle)
{// 保存记录
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL;
	int nWhich;
	S8 *pJsonString = NULL;
	S32 nLen;
	LibModuleProxyInfo_T *pNode = NULL;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	pSaveJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pSaveJson != NULL)
	{
		Common_Lock(pModuleMgr->hProxyLock);
		if (pModuleMgr->pResProxyListHead != NULL)
		{
			Common_Json_SetAttrValue(pSaveJson,-1,"RefreshFlag",Common_Json_Type_Number,NULL,pModuleMgr->dwProxyRefreshFlag,0);
			pArray = Common_Json_SetAttrValue(pSaveJson,-1,"ProxyList",Common_Json_Type_Array,NULL,0,0);
			if (pArray != NULL)
			{
				nWhich = 0;
				pNode = pModuleMgr->pResProxyListHead;
				while(pNode != NULL)
				{
					Common_Json_SetAttrValue(pArray,nWhich,"Uri",Common_Json_Type_String,pNode->pResUri,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"ProxyUri",Common_Json_Type_String,pNode->pProxyUri,0,0);
					nWhich++;
					pNode = pNode->pNext;
				}
			}
		}
		
		// 写文件
		nLen = 0;
		pJsonString = Common_Json_Print(pSaveJson,&nLen);
		if (pJsonString != NULL)
		{
			if (!Common_File_MkDir((char*)URIPROXY_FILE_PATH))
			{//
				char szFileName[128];
				FILE*pFD;
				sprintf(szFileName,"%s/%s.json",URIPROXY_FILE_PATH,pModuleMgr->pszModuleName);
				pFD = Common_File_fOpen(szFileName,(char*)"wb+");
				if (pFD != NULL)
				{
					fwrite(pJsonString,1,nLen + 1,pFD);
					Common_File_fClose(pFD);
					pFD = NULL;
				}
			}
			Common_Free(pJsonString,__FUNCTION__,__LINE__);
			pJsonString = NULL;
		}
		Common_UnLock(pModuleMgr->hProxyLock);

		Common_Json_Delete(pSaveJson);
		pSaveJson = NULL;

	}


	return 0;

}
S32 Module_UriProxy_Init(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	Common_Lock_Create(&pModuleMgr->hProxyLock,"Module_Proxy_lock");
	Module_UriProxy_Load(hModuleHandle);
	return 0;
}

S32 Module_UriProxy_Register(ModuleHandle_T hModuleHandle,S8 *szUri,S8 *szProxyUri)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	//LibModuleProxyInfo_T *pNode = NULL;
	//S32 bExist = 0;
	S32 nRet = -1;
	S8 *pProxyModule = NULL;
	cJSON_Struct *pJson = NULL,*pArrayJson = NULL,*pResult = NULL;
	if (pModuleMgr == NULL)
	{
		MODULE_ERROR("Module handle == NULL \n");
		return -1;
	}
	if (szUri == NULL || szProxyUri == NULL)
	{
		MODULE_ERROR("Invalid Parameter\n");
		return -1;
	}
	Common_UriOneParse(szProxyUri,NULL,&pProxyModule,NULL);
	if (pProxyModule == NULL)
	{
		return -1;
	}
	pJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	//
	if (pJson != NULL)
	{
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header/Method",Common_Json_Type_String,"Post",0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header/Uri",Common_Json_Type_String,"/Core/UriProxy/Register",0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Data",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
		pArrayJson = Common_Json_SetAttrValue_ex(pJson,-1,"/Data/ResList",Common_Json_Type_Array,NULL,0,0,__FUNCTION__,__LINE__);
		if (pArrayJson != NULL)
		{
			Common_Json_SetAttrValue_ex(pArrayJson,0,"Uri",Common_Json_Type_String,szUri,0,0,__FUNCTION__,__LINE__);
			Common_Json_SetAttrValue_ex(pArrayJson,0,"ProxyUri",Common_Json_Type_String,szProxyUri,0,0,__FUNCTION__,__LINE__);
		}
		if (0 != Common_StriCmp(pProxyModule,pModuleMgr->pszModuleName))
		{
			// 重定向到其他模块
			if (!pModuleMgr->bManager)
			{// 只有管理模块才可以自由添加重定向项
				Common_Json_Delete_ex(pJson,__FUNCTION__,__LINE__);
				Common_Free(pProxyModule,__FUNCTION__,__LINE__);
				return nRet;
			}
		}
		Module_CallFunctions(hModuleHandle,pJson,&pResult,3000);
		if (pResult != NULL)
		{
			S32 nCode = -1;
			Common_Json_GetAttrValue(pResult,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
			if (nCode == 0 || nCode == 200)
			{
				nRet = 0;
			}
		}
	}
	

	return nRet;
}

S32 Module_UriProxy_UnRegister(ModuleHandle_T hModuleHandle,S8 *szUri)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	//LibModuleProxyInfo_T *pNode = NULL;
	//S32 bExist = 0;
	S32 nRet = -1;
	cJSON_Struct *pJson = NULL,*pArrayJson = NULL,*pResult = NULL;
	if (pModuleMgr == NULL)
	{
		MODULE_ERROR("Module handle == NULL \n");
		return -1;
	}
	if (szUri == NULL)
	{
		MODULE_ERROR("Invalid Parameter\n");
		return -1;
	}
	
	pJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	//
	if (pJson != NULL)
	{
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header/Method",Common_Json_Type_String,"Delete",0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header/Uri",Common_Json_Type_String,"/Core/UriProxy/Register",0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Data",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
		pArrayJson = Common_Json_SetAttrValue_ex(pJson,-1,"/Data/ResList",Common_Json_Type_Array,NULL,0,0,__FUNCTION__,__LINE__);
		if (pArrayJson != NULL)
		{
			Common_Json_SetAttrValue_ex(pArrayJson,0,"Uri",Common_Json_Type_String,szUri,0,0,__FUNCTION__,__LINE__);
		}
	
		Module_CallFunctions(hModuleHandle,pJson,&pResult,3000);
		if (pResult != NULL)
		{
			S32 nCode = -1;
			Common_Json_GetAttrValue(pResult,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
			if (nCode == 0 || nCode == 200)
			{
				nRet = 0;
			}
		}
	}

	return nRet;
}

static S32 staticCheckRedirect(S8 *szUri,S8 *szNewUri)
{
	S32 nPos = 0;
	S8 c,c1;
	if (szUri == NULL || szNewUri == NULL)
	{
		return 0;
	}
	while(1)
	{
		c=szUri[nPos];
		c1 = szNewUri[nPos];
		if (c >= 'A' && c <= 'Z')
		{
			c = c - 'A' + 'a';
		}
		if (c1 >= 'A' && c1 <= 'Z')
		{
			c1 = c1 - 'A' + 'a';
		}
		if (c1 == 0)
		{
			if (c == 0 || c == '\\' || c == '/' || c == '.')
			{
				break;
			}
			nPos = -nPos;
			break;
		}
		else if (c == 0 || c != c1)
		{
			nPos = -nPos;
			break;
		}
		nPos++;
	}
	return nPos;
}
S32 Module_UriProxy_Update(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pJson = NULL,*pArrayJson = NULL;
	//cJSON_Struct *pResult = NULL;
	LibModuleProxyInfo_T *pNode = NULL;
	S32 nWhich = 0;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	if (!pModuleMgr->bManager)
	{
		return 0;
	}
	if ((!pModuleMgr->bNeedReportProxy))
	{
		return 0;
	}
	pModuleMgr->bNeedReportProxy = 0;
	pJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	//
	if (pJson != NULL)
	{
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header/Method",Common_Json_Type_String,"Put",0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Header/Uri",Common_Json_Type_String,"/Broadcast/UriProxy/Update",0,0,__FUNCTION__,__LINE__);
		Common_Json_SetAttrValue_ex(pJson,-1,"/Data",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
		pArrayJson = Common_Json_SetAttrValue_ex(pJson,-1,"/Data/ResList",Common_Json_Type_Array,NULL,0,0,__FUNCTION__,__LINE__);
		if (pArrayJson != NULL)
		{
			Common_Lock(pModuleMgr->hProxyLock);
			pNode = pModuleMgr->pResProxyListHead;
			while(pNode != NULL)
			{
				Common_Json_SetAttrValue_ex(pArrayJson,nWhich,"Uri",Common_Json_Type_String,pNode->pResUri,0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pArrayJson,nWhich,"ProxyUri",Common_Json_Type_String,pNode->pProxyUri,0,0,__FUNCTION__,__LINE__);
				nWhich++;
				pNode = pNode->pNext;
			}
			Common_UnLock(pModuleMgr->hProxyLock);
			
		}

		Module_CallFunctions(hModuleHandle,pJson,NULL,3000);
		Common_Json_Delete_ex(pJson,__FUNCTION__,__LINE__);
		
	}
	return 0;
}

S32 Module_UriProxy_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{// 返回 0-表示已处理
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char *pStringValue;
	char *pModuleName = NULL;
	S8 szTmp[128],*pDeal = NULL,*pMethod = NULL;
	S32 nLen;
	S32 nCode = -1;
	//S32 nSubIdx,nUserIdx,nCount,nFreeIdx;
	cJSON_Struct *pOutParamsJson = NULL;
	S32 bSave = 0;
	//S32 nSec = 0;
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
	nLen = sprintf(szTmp,"/%s/UriProxy/",pModuleMgr->pszModuleName);
	if (0 != Common_StrniCmp(pStringValue,szTmp,nLen))
	{// 非本模块
		// 检查重定向
		S8 *szRedirect = NULL;
		Common_Json_GetAttrValue(pInParams,-1,"Header/RedirectUri",NULL,&szRedirect,NULL,NULL);
		if (szRedirect == NULL)
		{
			// 开始处理代理,重定向
			LibModuleProxyInfo_T *pNode = NULL;
			S32 nRedirect = 0,nLen1,nLen2;
			S8 *pReStringStart = NULL,*szNewUri = NULL;
			Common_Lock(pModuleMgr->hProxyLock);
			pNode = pModuleMgr->pResProxyListHead;
			while (pNode != NULL)
			{
				nRedirect = staticCheckRedirect(pStringValue,pNode->pResUri);
				if (nRedirect > 0)
				{// 重定向
					// 检查定向 是否同一模块
					S8 *pRedModuleName = NULL,*szFromUri = NULL;
					Common_UriOneParse(pNode->pProxyUri,NULL,&pRedModuleName,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"Header/From/Uri",NULL,&szFromUri,NULL,NULL);
					if (pRedModuleName != NULL)
					{
						if (szFromUri != NULL && szFromUri[0] != 0)
						{
							if (0 != Common_StriCmp(szFromUri+1,pRedModuleName))
							{// 避免 请求模块 定向到自身
								pReStringStart = pStringValue + nRedirect;
								nLen1 = strlen(pReStringStart);
								nLen2 = strlen(pNode->pProxyUri);
								szNewUri = (S8 *)Common_Malloc(nLen2 + nLen1 + 2,0,__FUNCTION__,__LINE__);
								if (szNewUri != NULL)
								{
									sprintf(szNewUri,"%s%s",pNode->pProxyUri,pReStringStart);
									Common_Json_SetAttrValue(pInParams,-1,"Header/From/OrgUri",Common_Json_Type_String,pStringValue,0,0);
									Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szNewUri,0,0);
									Common_Json_SetAttrValue(pInParams,-1,"Header/From/RedirectUri",Common_Json_Type_String,pNode->pProxyUri,0,0);
									Common_Free(szNewUri,__FUNCTION__,__LINE__);
								}
							}
						}
						Common_Free(pRedModuleName,__FUNCTION__,__LINE__);
						
					}
					
					break;
				}

				pNode = pNode->pNext;
			}
			Common_UnLock(pModuleMgr->hProxyLock);
		}

		return -1;
	}
	pMethod = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Method",NULL,&pMethod,NULL,NULL);

	pDeal = pStringValue + nLen;
	if (0 == Common_StriCmp(pDeal,(char*)"Register"))
	{// 
		cJSON_Struct *pArray = NULL;
		if (!pModuleMgr->bManager)
		{
			nCode = MODULE_ERROR_TYPE_NOTFOUND;
		}
		else if (0 == Common_StriCmp((char*)"Post",pMethod))
		{
			S32 nArrayCount = 0,nIdx;
			S8 *pUri,*pProxyUri;
			pArray = Common_Json_GetItem(pInParams,-1,"/Data/ResList");
			if (pArray != NULL)
			{
				nArrayCount = Common_Json_Size(pArray);
				for (nIdx = 0; nIdx < nArrayCount;nIdx++)
				{
					pUri = NULL;
					pProxyUri = NULL;
					Common_Json_GetAttrValue(pArray,nIdx,"Uri",NULL,&pUri,NULL,NULL);
					Common_Json_GetAttrValue(pArray,nIdx,"ProxyUri",NULL,&pProxyUri,NULL,NULL);
					if (pUri != NULL && pProxyUri != NULL)
					{
						LibModuleProxyInfo_T *pNode = NULL;
						S32 bExist = 0;
						Common_Lock(pModuleMgr->hProxyLock);
						pNode = pModuleMgr->pResProxyListHead;
						while(pNode != NULL)
						{
							if (0 == Common_StriCmp(pNode->pResUri,pUri))
							{
								if (0 == Common_StriCmp(pNode->pProxyUri,pProxyUri))
								{
									nCode = MODULE_ERROR_TYPE_SUCC;
								}
								else
								{
									// 修改
									S8 *pNewUri;
									pNewUri = Common_StrDup(pProxyUri,__FUNCTION__,__LINE__);
									if (pNewUri != NULL)
									{
										Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
										pNode->pProxyUri = pNewUri;
										pModuleMgr->dwProxyRefreshFlag++;
										pModuleMgr->bNeedReportProxy = 1;
										bSave = 1;

									}
									nCode = MODULE_ERROR_TYPE_SUCC;
								}
								bExist = 1;
								break;
							}
							pNode = pNode->pNext;
						}
						if (!bExist)
						{// 新增
							pNode = (LibModuleProxyInfo_T *)Common_Malloc(sizeof(LibModuleProxyInfo_T),0,__FUNCTION__,__LINE__);
							if (pNode != NULL)
							{
								memset(pNode,0,sizeof(LibModuleProxyInfo_T));
								pNode->pResUri = Common_StrDup(pUri,__FUNCTION__,__LINE__);
								pNode->pProxyUri = Common_StrDup(pProxyUri,__FUNCTION__,__LINE__);
								if (pNode->pResUri == NULL || pNode->pProxyUri == NULL)
								{
									Common_Free(pNode->pResUri,__FUNCTION__,__LINE__);
									Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
									Common_Free(pNode,__FUNCTION__,__LINE__);
									nCode = MODULE_ERROR_TYPE_LIMITED;
								}
								else
								{
									pNode->pNext = pModuleMgr->pResProxyListHead;
									if (pModuleMgr->pResProxyListHead != NULL)
									{
										pModuleMgr->pResProxyListHead->pPrev = pNode;
									}
									pModuleMgr->pResProxyListHead = pNode;
									pModuleMgr->dwProxyRefreshFlag++;
									pModuleMgr->bNeedReportProxy = 1;
									nCode = MODULE_ERROR_TYPE_SUCC;
									bSave = 1;

								}
							}
						}
						Common_UnLock(pModuleMgr->hProxyLock);
					}
				}
				
			}
		}
		else if (0 == Common_StriCmp((char*)"Delete",pMethod))
		{
			S32 nArrayCount = 0,nIdx;
			S8 *pUri,*pProxyUri;
			pArray = Common_Json_GetItem(pInParams,-1,"/Data/ResList");
			if (pArray != NULL)
			{
				nArrayCount = Common_Json_Size(pArray);
				for (nIdx = 0; nIdx < nArrayCount;nIdx++)
				{
					pUri = NULL;
					pProxyUri = NULL;
					Common_Json_GetAttrValue(pArray,nIdx,"Uri",NULL,&pUri,NULL,NULL);
					Common_Json_GetAttrValue(pArray,nIdx,"ProxyUri",NULL,&pProxyUri,NULL,NULL);
					if (pUri != NULL)
					{
						LibModuleProxyInfo_T *pNode = NULL;
						//LibModuleProxyInfo_T *pDeleteNode = NULL;
						Common_Lock(pModuleMgr->hProxyLock);
						pNode = pModuleMgr->pResProxyListHead;
						while(pNode != NULL)
						{
							if (0 == Common_StriCmp(pNode->pResUri,pUri))
							{
								// 删除
								if (pNode->pPrev == NULL)
								{// 头节点
									pModuleMgr->pResProxyListHead = pNode->pNext;
									if (pModuleMgr->pResProxyListHead != NULL)
									{
										pModuleMgr->pResProxyListHead->pPrev = NULL;
									}
								}
								else
								{
									pNode->pPrev->pNext = pNode->pNext;
									if (pNode->pNext != NULL)
									{
										pNode->pNext->pPrev = pNode->pPrev;
									}
								}
								pModuleMgr->dwProxyRefreshFlag++;
								pModuleMgr->bNeedReportProxy = 1;
								Common_Free(pNode->pResUri,__FUNCTION__,__LINE__);
								Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
								Common_Free(pNode,__FUNCTION__,__LINE__);
								bSave = 1;
								
								break;
							}
							pNode = pNode->pNext;
						}
						Common_UnLock(pModuleMgr->hProxyLock);
						nCode = MODULE_ERROR_TYPE_SUCC;

					}
				}

			}
		}
		else
		{
			nCode = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
		}
		
	}
	else if (0 == Common_StriCmp(pDeal,(char*)"Update"))
	{// Put
	
		if (pModuleMgr->bManager)
		{
			nCode = MODULE_ERROR_TYPE_NOTFOUND;
		}
		else if (0 == Common_StriCmp((char*)"Put",pMethod))
		{
			// 更新
			U32 dwFlag = 0;
			cJSON_Struct *pArray = NULL;
			S32 nArrayCount = 0,nIdx;
			S8 *pUri,*pProxyUri;
			LibModuleProxyInfo_T *pNode = NULL,*pDeleteNode = NULL;
			nCode = MODULE_ERROR_TYPE_SUCC;
			Common_Lock(pModuleMgr->hProxyLock);
			Common_Json_GetAttrValue(pInParams,-1,"Data/RefreshFlag",NULL,NULL,(S32 *)&dwFlag,NULL);
			if (pModuleMgr->dwProxyRefreshFlag != dwFlag)
			{
				pModuleMgr->dwProxyRefreshFlag = dwFlag;
				pArray = Common_Json_GetItem(pInParams,-1,"/Data/ResList");
				if (pArray != NULL)
				{
					nArrayCount = Common_Json_Size(pArray);
					for (nIdx = 0; nIdx < nArrayCount;nIdx++)
					{
						pUri = NULL;
						pProxyUri = NULL;
						Common_Json_GetAttrValue(pArray,nIdx,"Uri",NULL,&pUri,NULL,NULL);
						Common_Json_GetAttrValue(pArray,nIdx,"ProxyUri",NULL,&pProxyUri,NULL,NULL);
						if (pUri != NULL)
						{
							S32 bExist = 0;
							
							pNode = pModuleMgr->pResProxyListHead;
							while(pNode != NULL)
							{
								if (0 == Common_StriCmp(pNode->pResUri,pUri))
								{
									if (0 != Common_StriCmp(pNode->pProxyUri,pProxyUri))
									{// 更新
										S8 *pNewUri;
										pNewUri = Common_StrDup(pProxyUri,__FUNCTION__,__LINE__);
										if (pNewUri != NULL)
										{
											Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
											pNode->pProxyUri = pNewUri;
										}
									}
									pNode->dwFlag = dwFlag;
									bExist = 1;
									break;
								}
								pNode = pNode->pNext;
							}
							if (!bExist)
							{// 不存在则新加
								// 本模块的资源才添加
								Common_UriOneParse(pUri,NULL,&pModuleName,NULL);
								if (pModuleName != NULL)
								{
									if (0 == Common_StriCmp(pModuleName,pModuleMgr->pszModuleName))
									{
										pNode = (LibModuleProxyInfo_T *)Common_Malloc(sizeof(LibModuleProxyInfo_T),0,__FUNCTION__,__LINE__);
										if (pNode != NULL)
										{
											memset(pNode,0,sizeof(LibModuleProxyInfo_T));
											pNode->pResUri = Common_StrDup(pUri,__FUNCTION__,__LINE__);
											pNode->pProxyUri = Common_StrDup(pProxyUri,__FUNCTION__,__LINE__);
											pNode->dwFlag = dwFlag;
											if (pNode->pResUri == NULL || pNode->pProxyUri == NULL)
											{
												Common_Free(pNode->pResUri,__FUNCTION__,__LINE__);
												Common_Free(pNode->pProxyUri,__FUNCTION__,__LINE__);
												Common_Free(pNode,__FUNCTION__,__LINE__);
											}
											else
											{
												pNode->pNext = pModuleMgr->pResProxyListHead;
												if (pModuleMgr->pResProxyListHead != NULL)
												{
													pModuleMgr->pResProxyListHead->pPrev = pNode;
												}
												pModuleMgr->pResProxyListHead = pNode;
											}
										}
									}
									Common_Free(pModuleName,__FUNCTION__,__LINE__);
								}
								
							}
						}
					}

				}

				// 检查是否要删除的节点 
				pNode = pModuleMgr->pResProxyListHead;
				while(pNode != NULL)
				{
					pDeleteNode = pNode;
					pNode = pNode->pNext;
					if (pDeleteNode->dwFlag != dwFlag)
					{
						// 删除
						if (pDeleteNode->pPrev == NULL)
						{
							pModuleMgr->pResProxyListHead = pDeleteNode->pNext;
							if (pModuleMgr->pResProxyListHead != NULL)
							{
								pModuleMgr->pResProxyListHead->pPrev = NULL;
							}
						}
						else
						{
							pDeleteNode->pPrev->pNext = pDeleteNode->pNext;
							if (pDeleteNode->pNext != NULL)
							{
								pDeleteNode->pNext->pPrev = pDeleteNode->pPrev;
							}
						}
						Common_Free(pDeleteNode->pResUri,__FUNCTION__,__LINE__);
						Common_Free(pDeleteNode->pProxyUri,__FUNCTION__,__LINE__);
						Common_Free(pDeleteNode,__FUNCTION__,__LINE__);

					}
					
				}
			}
			Common_UnLock(pModuleMgr->hProxyLock);
		}
		else
		{
			nCode = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
		}
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

	if (bSave)
	{
		Module_UriProxy_Save(hModuleHandle);
	}
	
	return 0;
}
