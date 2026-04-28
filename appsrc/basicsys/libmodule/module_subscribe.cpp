#include "libcommon_api.h"
#include "libmodule_struct.h"
#include "libmodule_api.h"
#ifdef WIN32
#define SUBSCRIBE_FILE_PATH "d:/tmp/modules/subscribes/"
#else
#define SUBSCRIBE_FILE_PATH "/tmp/modules/subscribes/"
#endif


/*
/模块名/ModuleSubscribe/Post              -> 提交订阅
/模块名/ModuleSubscribe/Delete            -> 取消订阅
/模块名/ModuleSubscribe/Pull            -> 查询订阅
/模块名/ModuleSubscribe/Push            -> 推送消息
/模块名/ModuleSubscribe/Renew            -> 刷新订阅, 1分钟 renew 一次，三次未收到则认为掉线，需要删除
/模块名/ModuleSubscribe/Receive/本地ID号    -> 接收URI 
/模块名/ModuleSubscribe/Ress/<本地ID号>/<userID>     -> 订阅的资源

*/
// 服务器端导入、保存记录函数 
S32 Module_Subscribe_Load(ModuleHandle_T hModuleHandle)
{// 导入记录
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL,*pArrayUser = NULL;
	LibModuleSubscribeOwner_T *pOwerInfo = NULL;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	int nSubIdx,nUserIdx,nWhich,nWhichUser;
	S8 *pJsonString = NULL,*pValueString;
	S32 nLen,nValueNumber,nSubscribeId;
	S32 nUserCount = 0;
	S8 *szRessUri=NULL,*szSubscribeUri = NULL,*szResponceUri = NULL,*szModuleName = NULL;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	{//
		char szFileName[128];
		FILE*pFD;
		sprintf(szFileName,"%s/%s.json",SUBSCRIBE_FILE_PATH,pModuleMgr->pszModuleName);
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
	Common_Json_GetAttrValue(pSaveJson,-1,"HandleCount",NULL,&pValueString,&nValueNumber,NULL);
	pModuleMgr->tSubscribeInfo.dwHandleCount = nValueNumber;
	
	pArray = Common_Json_GetItem(pSaveJson,-1,"OwerInfo");
	if (pArray != NULL)
	{
		S32 nOwerCount = 0;
		nOwerCount = Common_Json_Size(pArray);
		nWhich = 0;
		for(nWhich = 0; nWhich < nOwerCount;nWhich++)
		{
			szSubscribeUri = NULL;
			Common_Json_GetAttrValue(pArray,nWhich,"SubscribeUri",NULL,&szSubscribeUri,&nValueNumber,NULL);
			nSubscribeId = 0;
			Common_Json_GetAttrValue(pArray,nWhich,"SubscribeId",NULL,&pValueString,&nSubscribeId,NULL);
			if (szSubscribeUri != NULL)
			{
				nSubIdx = (nSubscribeId )&0xFFFF;
				pOwerInfo = (LibModuleSubscribeOwner_T *)Common_Malloc(sizeof(LibModuleSubscribeOwner_T),0,__FUNCTION__,__LINE__);
				if (pOwerInfo != NULL)
				{
					memset(pOwerInfo,0,sizeof(LibModuleSubscribeOwner_T));
					pOwerInfo->bInvalid = 1;// 肯定是无效的，需要重新注册后才有效
					pOwerInfo->nSubscribeId = nSubscribeId;
					pOwerInfo->szSubscribeUri = Common_StrDup(szSubscribeUri,__FUNCTION__,__LINE__);
					if (pOwerInfo->szSubscribeUri == NULL)
					{
						Common_Free(pOwerInfo,__FUNCTION__,__LINE__);
						pOwerInfo = NULL;
						continue;
					}
					pModuleMgr->tSubscribeInfo.nOwerCount++;
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] = pOwerInfo;
					pOwerInfo->pNext = pModuleMgr->tSubscribeInfo.pOwnerInfoHead;
					if (pModuleMgr->tSubscribeInfo.pOwnerInfoHead != NULL)
					{
						pModuleMgr->tSubscribeInfo.pOwnerInfoHead->pPrev = pOwerInfo;
					}
					pModuleMgr->tSubscribeInfo.pOwnerInfoHead = pOwerInfo;
					// 使用者数据
					pArrayUser = Common_Json_GetItem(pArray,nWhich,"UserList");
					if (pArrayUser != NULL)
					{
						S32 nRecvId = 0;
						nUserCount = Common_Json_Size(pArrayUser);
						nWhichUser = 0;
						for(nWhichUser = 0; nWhichUser < nUserCount;nWhichUser++)
						{
							Common_Json_GetAttrValue(pArrayUser,nWhichUser,"RessUri",NULL,&szRessUri,&nValueNumber,NULL);
							nValueNumber = 0;
							Common_Json_GetAttrValue(pArrayUser,nWhichUser,"RecvId",NULL,&pValueString,&nRecvId,NULL);
							Common_Json_GetAttrValue(pArrayUser,nWhichUser,"SubscribeUri",NULL,&szSubscribeUri,&nValueNumber,NULL);
							Common_Json_GetAttrValue(pArrayUser,nWhichUser,"ResponceUri",NULL,&szResponceUri,&nValueNumber,NULL);
							Common_Json_GetAttrValue(pArrayUser,nWhichUser,"ModuleName",NULL,&szModuleName,&nValueNumber,NULL);
							if (szSubscribeUri != NULL)
							{
								pUserInfo = (LibModuleSubscribeUser_T *)Common_Malloc(sizeof(LibModuleSubscribeUser_T),0,__FUNCTION__,__LINE__);
								if (pUserInfo != NULL)
								{
									memset(pUserInfo,0,sizeof(LibModuleSubscribeUser_T));
									pUserInfo->nSubscribeId = nRecvId;
									if (szModuleName)
									{
										pUserInfo->szModuleName = Common_StrDup(szModuleName,__FUNCTION__,__LINE__);
										if (pUserInfo->szModuleName == NULL)
										{
											Common_Free(pUserInfo->szModuleName,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szRessUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo,__FUNCTION__,__LINE__);
											continue;
										}
									}
									if (szRessUri)
									{
										pUserInfo->szRessUri = Common_StrDup(szRessUri,__FUNCTION__,__LINE__);
										if (pUserInfo->szRessUri == NULL)
										{
											Common_Free(pUserInfo->szModuleName,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szRessUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo,__FUNCTION__,__LINE__);
											continue;
										}
									}
									if (szSubscribeUri)
									{
										pUserInfo->szSubscribeUri = Common_StrDup(szSubscribeUri,__FUNCTION__,__LINE__);
										if (pUserInfo->szSubscribeUri == NULL)
										{
											Common_Free(pUserInfo->szModuleName,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szRessUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo,__FUNCTION__,__LINE__);
											continue;
										}
									}
									if (szResponceUri)
									{
										pUserInfo->szResponceUri = Common_StrDup(szResponceUri,__FUNCTION__,__LINE__);
										if (pUserInfo->szResponceUri == NULL)
										{
											Common_Free(pUserInfo->szModuleName,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szRessUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
											Common_Free(pUserInfo,__FUNCTION__,__LINE__);
											continue;
										}
									}
									Common_Json_GetAttrValue(pArrayUser,nWhichUser,"LastRenewTime",NULL,NULL,&pUserInfo->nLastRenewTime,NULL);
									nUserIdx = (nRecvId & 0xFF) - 1;
									pOwerInfo->pUserInfoList[nUserIdx] = pUserInfo;
									pOwerInfo->nUserCount++;
									pUserInfo->pNext = pOwerInfo->pUserInfoHead;
									if(pOwerInfo->pUserInfoHead != NULL)
									{
										pOwerInfo->pUserInfoHead->pPrev = pUserInfo;
									}
									pOwerInfo->pUserInfoHead = pUserInfo;
									
									
								}

							}
						}
					}

				}
			}
		}
	}
	
	Common_Json_Delete(pSaveJson);
	pSaveJson = NULL;



	return 0;

}
S32 Module_Subscribe_Save(ModuleHandle_T hModuleHandle)
{// 保存记录
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL,*pArrayUser = NULL;
	LibModuleSubscribeOwner_T *pOwerInfo = NULL;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	int nSubIdx,nUserIdx,nWhich,nWhichUser;
	S8 *pJsonString = NULL;
	S32 nLen;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	pSaveJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pSaveJson != NULL)
	{
		Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
		if (pModuleMgr->tSubscribeInfo.nOwerCount > 0)
		{
			Common_Json_SetAttrValue(pSaveJson,-1,"HandleCount",Common_Json_Type_Number,NULL,pModuleMgr->tSubscribeInfo.dwHandleCount,0);
			pArray = Common_Json_SetAttrValue(pSaveJson,-1,"OwerInfo",Common_Json_Type_Array,NULL,0,0);
			if (pArray != NULL)
			{
				nWhich = 0;
				for (nSubIdx = 0; nSubIdx < LIBMODULE_MAX_SUBSCRIBE_NUM && nWhich < pModuleMgr->tSubscribeInfo.nOwerCount;nSubIdx++)
				{
					if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL)
					{
						pOwerInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx];
						Common_Json_SetAttrValue(pArray,nWhich,"SubscribeId",Common_Json_Type_Number,NULL,pOwerInfo->nSubscribeId,0);
						Common_Json_SetAttrValue(pArray,nWhich,"IsInvalid",Common_Json_Type_Number,NULL,pOwerInfo->bInvalid,0);
						Common_Json_SetAttrValue(pArray,nWhich,"SubscribeUri",Common_Json_Type_String,pOwerInfo->szSubscribeUri,0,0);
						if (pOwerInfo->nUserCount > 0)
						{
							pArrayUser = Common_Json_SetAttrValue(pArray,nWhich,"UserList",Common_Json_Type_Array,NULL,0,0);
							if (pArrayUser != NULL)
							{
								nWhichUser = 0;
								for (nUserIdx = 0; nUserIdx < LIBMODULE_MAX_SUBSCRIBE_NUM && nWhichUser < pOwerInfo->nUserCount;nUserIdx++)
								{
									if (pOwerInfo->pUserInfoList[nUserIdx] != NULL)
									{
										pUserInfo = pOwerInfo->pUserInfoList[nUserIdx];
										Common_Json_SetAttrValue(pArrayUser,nWhichUser,"RecvId",Common_Json_Type_Number,NULL,pUserInfo->nSubscribeId,0);
										Common_Json_SetAttrValue(pArrayUser,nWhichUser,"SubscribeUri",Common_Json_Type_String,pUserInfo->szSubscribeUri,0,0);
										Common_Json_SetAttrValue(pArrayUser,nWhichUser,"RessUri",Common_Json_Type_String,pUserInfo->szRessUri,0,0);
										Common_Json_SetAttrValue(pArrayUser,nWhichUser,"ResponceUri",Common_Json_Type_String,pUserInfo->szResponceUri,0,0);
										Common_Json_SetAttrValue(pArrayUser,nWhichUser,"ModuleName",Common_Json_Type_String,pUserInfo->szModuleName,0,0);
										Common_Json_SetAttrValue(pArrayUser,nWhichUser,"LastRenewTime",Common_Json_Type_Number,NULL,pUserInfo->nLastRenewTime,0);
										nWhichUser++;
									}
									
								}
								
							}
						}
						nWhich++;
						
					}
				}
			}
		}
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		// 写文件
		nLen = 0;
		pJsonString = Common_Json_Print(pSaveJson,&nLen);
		if (pJsonString != NULL)
		{
			if (!Common_File_MkDir(SUBSCRIBE_FILE_PATH))
			{//
				char szFileName[128];
				FILE*pFD;
				sprintf(szFileName,"%s/%s.json",SUBSCRIBE_FILE_PATH,pModuleMgr->pszModuleName);
				pFD = Common_File_fOpen(szFileName,"wb+");
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
		
		
		

		Common_Json_Delete(pSaveJson);
		pSaveJson = NULL;

	}

	
	return 0;

}

S32 Module_Subscribe_Init(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char sztmp[128];
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	sprintf(sztmp,"%s_Module_Subscribe_lock",pModuleMgr->pszModuleName);
	Common_Lock_Create(&pModuleMgr->tSubscribeInfo.hLock,sztmp);
	Module_Subscribe_Load(hModuleHandle);
	return 0;
}
S32 Module_Subscribe_UnInit(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char sztmp[128];
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	// 反注册
	// 反订阅
	Common_Lock_Destroy(&pModuleMgr->tSubscribeInfo.hLock);
	return 0;
}

S32 Module_Subscribe_SetPrivateInfo(ModuleHandle_T hModuleHandle,S32 nRecvID/*or nSubscribeID*/,void *pBuff,S32 nSize)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	S32 nSubIdx = 0,nUserIdx = 0,bOwner = 0;
	S32 nRet = -1;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	bOwner = nRecvID & 0xFF;
	if (bOwner)
	{
		nSubIdx = (nRecvID >> 8) & 0xFF;
		nUserIdx = (nRecvID & 0xFF) - 1;
	}
	else
	{
		nUserIdx = (nRecvID >> 8) & 0xFF;
	}

	if (nUserIdx < 0 || nUserIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM ||
		nSubIdx < 0 || nSubIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	if (bOwner)
	{
		if(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL)
		{
			if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] != NULL &&
				pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nSubscribeId == nRecvID)
			{//
				if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo != NULL)
				{
					Common_Free(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo,__FUNCTION__,__LINE__);
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo = NULL;
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nPrivateInfoSize = 0;
				}
				if (pBuff != NULL && nSize > 0)
				{
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo = Common_Malloc(nSize,0,__FUNCTION__,__LINE__);
					if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo != NULL)
					{
						Common_Copy(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo,pBuff,nSize);
						pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nPrivateInfoSize = nSize;
					}
				}
				
				nRet = 0;
			}
		}
	}
	else
	{

		if (pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx] != NULL &&
			pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->nSubscribeId == nRecvID)
		{//
			if (pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo != NULL)
			{
				Common_Free(pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo,__FUNCTION__,__LINE__);
				pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo = NULL;
				pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->nPrivateInfoSize = 0;
			}
			if (pBuff != NULL && nSize > 0)
			{
				pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo = Common_Malloc(nSize,0,__FUNCTION__,__LINE__);
				if (pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo != NULL)
				{
					Common_Copy(pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo,pBuff,nSize);
					pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->nPrivateInfoSize = nSize;
				}
			}
			nRet = 0;
		}
	}
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	return nRet;
}
S32 Module_Subscribe_GetPrivateInfo(ModuleHandle_T hModuleHandle,S32 nRecvID,void **pBuff,S32 *lpSize)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	S32 nSubIdx = 0,nUserIdx = 0,bOwner = 0;
	S32 nRet = -1;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	bOwner = nRecvID & 0xFF;
	if (bOwner)
	{
		nSubIdx = (nRecvID >> 8) & 0xFF;
		nUserIdx = (nRecvID & 0xFF) - 1;
	}
	else
	{
		nUserIdx = (nRecvID >> 8) & 0xFF;
	}

	if (nUserIdx < 0 || nUserIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM ||
		nSubIdx < 0 || nSubIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	if (bOwner)
	{
		if(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL)
		{
			if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] != NULL &&
				pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nSubscribeId == nRecvID)
			{//
				if (pBuff != NULL)
				{
					*pBuff = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->pPrivateInfo;
				}
				if (lpSize != NULL)
				{
					*lpSize = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nPrivateInfoSize;
				}
				
				

				nRet = 0;
			}
		}
	}
	else
	{

		if (pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx] != NULL &&
			pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->nSubscribeId == nRecvID)
		{//
			if (pBuff != NULL)
			{
				*pBuff = pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->pPrivateInfo;
			}
			if (lpSize != NULL)
			{
				*lpSize = pModuleMgr->tSubscribeInfo.pUserInfoList[nUserIdx]->nPrivateInfoSize;
			}
			
			nRet = 0;
		}
	}
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	return nRet;
}


// 订阅操作接口
S32 Module_SubscribeEvent(ModuleHandle_T hModuleHandle,S8 *szSubscribeUri,Module_Events_Def fxn,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	S32 i,nFreeIdx = -1,nCount = 0,nSubscribeId;
	S8 szTmp[128];
	if (pModuleMgr == NULL || szSubscribeUri == NULL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	for (i = 0; i < LIBMODULE_MAX_SUBSCRIBE_NUM; i++)
	{
		if (NULL != pModuleMgr->tSubscribeInfo.pUserInfoList[i])
		{
			if (0 == Common_StriCmp(szSubscribeUri,pModuleMgr->tSubscribeInfo.pUserInfoList[i]->szSubscribeUri))
			{
				// 存在,更新
				pUserInfo = pModuleMgr->tSubscribeInfo.pUserInfoList[i];
				pUserInfo->fxn = fxn;
				pUserInfo->pUserData = pUserData;
				nSubscribeId = pUserInfo->nSubscribeId;
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
				return nSubscribeId;

			}
		}
		else if (nFreeIdx == -1)
		{
			nFreeIdx = i;
		}
		nCount++;
		if (nCount >= pModuleMgr->tSubscribeInfo.nUserCount && nFreeIdx != -1)
		{
			break;
		}
	}
	if (nFreeIdx == -1)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pUserInfo = (LibModuleSubscribeUser_T *)Common_Malloc(sizeof(LibModuleSubscribeUser_T),0,__FUNCTION__,__LINE__);
	if (pUserInfo == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	memset(pUserInfo,0,sizeof(LibModuleSubscribeUser_T));
	pUserInfo->szSubscribeUri = Common_StrDup(szSubscribeUri,__FUNCTION__,__LINE__);
	if (pUserInfo->szSubscribeUri == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		Common_Free(pUserInfo,__FUNCTION__,__LINE__);
		pUserInfo = NULL;
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pModuleMgr->tSubscribeInfo.dwHandleCount++;
	if (pModuleMgr->tSubscribeInfo.dwHandleCount > 0x7FFF)
	{
		pModuleMgr->tSubscribeInfo.dwHandleCount = 1;
	}
	nSubscribeId = (pModuleMgr->tSubscribeInfo.dwHandleCount << 16) | (nFreeIdx);

	sprintf(szTmp,"/%s/ModuleSubscribe/Receive/%d",pModuleMgr->pszModuleName,nSubscribeId);
	pUserInfo->szResponceUri = Common_StrDup(szTmp,__FUNCTION__,__LINE__);
	if (pUserInfo->szResponceUri == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
		Common_Free(pUserInfo,__FUNCTION__,__LINE__);
		pUserInfo = NULL;
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pUserInfo->szModuleName = Common_StrDup(pModuleMgr->pszModuleName,__FUNCTION__,__LINE__);
	if (pUserInfo->szModuleName == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
		Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
		Common_Free(pUserInfo,__FUNCTION__,__LINE__);
		pUserInfo = NULL;
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pUserInfo->fxn = fxn;
	pUserInfo->pUserData = pUserData;
	
	pUserInfo->nSubscribeId = nSubscribeId;
	pModuleMgr->tSubscribeInfo.nUserCount++;
	// 加入待订阅列表
	pUserInfo->pNext = pModuleMgr->tSubscribeInfo.pUserInfoHead;
	if(pModuleMgr->tSubscribeInfo.pUserInfoHead != NULL)
	{
		pModuleMgr->tSubscribeInfo.pUserInfoHead->pPrev = pUserInfo;
	}
	pModuleMgr->tSubscribeInfo.pUserInfoHead = pUserInfo;
	pModuleMgr->tSubscribeInfo.pUserInfoList[nFreeIdx] = pUserInfo;
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	return nSubscribeId;
}
S32 Module_UnSubscribeEvent(ModuleHandle_T hModuleHandle,S32 nSubscribeID)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	S32 i,nSubIdx = -1;
	
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	nSubIdx = (nSubscribeID ) & 0xFFFF;
	if (nSubIdx < 0|| nSubIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	if (pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return 0;
	}
	pUserInfo = pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx];
	if (pUserInfo->nSubscribeId != nSubscribeID)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return 0;
	}
	pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx] = NULL;
	pUserInfo->bNeedDelete = 1;
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	return 0;
}
S32 Module_Subscribe_CheckSubmit(ModuleHandle_T hModuleHandle)
{// 检查是否需要重订阅或者删除
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeUser_T *pUserInfo = NULL,*pCurrUserInfo = NULL,*pDeleteInfo = NULL;
	S32 i,nSubIdx = -1,nRet = -1;
	cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	S32 nCode;
	S32 bDelete = 0,bDetach = 0;
	S8 szTmp[128];
	S32 nSec = 0;
	S8 *pToModuleName = NULL;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	Common_GetSystemCount(&nSec,NULL);

	
	do{
		Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
		if (pModuleMgr->tSubscribeInfo.pUserInfoHead == NULL)
		{ // 没有需要处理的
			Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
			break;
		}
		if (pUserInfo == NULL)
		{
			pUserInfo = pModuleMgr->tSubscribeInfo.pUserInfoHead;
		}
		pCurrUserInfo = pUserInfo;
		pUserInfo = pUserInfo->pNext;
		pCurrUserInfo->nUseCount++;
		
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		if (pToModuleName != NULL)
		{
			Common_Free(pToModuleName,__FUNCTION__,__LINE__);
			pToModuleName = NULL;
		}
		Common_UriOneParse(pCurrUserInfo->szSubscribeUri,NULL,&pToModuleName,NULL);
		// 开始处理
		bDelete = 0;
		bDetach = 0;
		if (pCurrUserInfo->bNeedDelete)
		{// 需要反订阅
			if (pCurrUserInfo->szRessUri != NULL)
			{// 有资源，说明订阅成功
				pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if (pInParams != NULL)
				{
					
					nCode = -1;
					pOutParams = NULL;
					
					if (pToModuleName != NULL)
					{
						sprintf(szTmp,"/%s/ModuleSubscribe/Delete",pToModuleName);
						Common_Free(pToModuleName,__FUNCTION__,__LINE__);
						pToModuleName = NULL;
						Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
						Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Delete",0,0);
						Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,pCurrUserInfo->szRessUri,0,0);
						//Common_Json_SetAttrValue(pInParams,-1,"Data",Common_Json_Type_Object,NULL,0,0);
						//Common_Json_SetAttrValue(pInParams,-1,"Data/DeleteUri",Common_Json_Type_String,pCurrUserInfo->szRessUri,0,0);
						nRet = Module_CallFunctions(hModuleHandle,pInParams,&pOutParams,3000);
					}
					
					if (pOutParams != NULL)
					{
						
						Common_Json_GetAttrValue(pOutParams,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
					}
					
				}
			}
			
			
		}
		else if(pCurrUserInfo->szRessUri == NULL)
		{
			// 需要订阅
			pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pInParams != NULL)
			{
				nCode = -1;
				if (pToModuleName != NULL)
				{
					sprintf(szTmp,"/%s/ModuleSubscribe/Post",pToModuleName);
					Common_Free(pToModuleName,__FUNCTION__,__LINE__);
					pToModuleName = NULL;
					Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Post",0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/SubscribeUri",Common_Json_Type_String,pCurrUserInfo->szSubscribeUri,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,pCurrUserInfo->szResponceUri,0,0);
					nRet = Module_CallFunctions(hModuleHandle,pInParams,&pOutParams,3000);
					
				}
				
				if (pOutParams != NULL)
				{
					S8 *pResUri = NULL;
					Common_Json_GetAttrValue(pOutParams,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
					Common_Json_GetAttrValue(pOutParams,-1,"/Header/ResUri",NULL,&pResUri,NULL,NULL);
					if (nCode == 0 && pResUri != NULL)
					{
						pCurrUserInfo->szRessUri = Common_StrDup(pResUri,__FUNCTION__,__LINE__);
						if(pCurrUserInfo->fxn != NULL)
						{
							cJSON_Struct *pEventJson;
							pEventJson = Common_Json_GetItem(pOutParams,-1,"Data");
							if (pEventJson)
							{
								pCurrUserInfo->fxn(hModuleHandle,pCurrUserInfo->nSubscribeId,pEventJson,NULL,pCurrUserInfo->pUserData);
							}
						}
						Common_GetSystemCount(&nSec,NULL);
						pCurrUserInfo->nLastRenewTime = nSec;
					}
					
					
				}
				
			}
		}
		else if (pCurrUserInfo->nLastRenewTime + MODULE_SUBSCRIBE_RENEW_INTERVAL <= nSec)
		{
			// Renew
			pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pInParams != NULL)
			{
				nCode = -1;
				if (pToModuleName != NULL)
				{
					sprintf(szTmp,"/%s/ModuleSubscribe/Renew",pToModuleName);
					Common_Free(pToModuleName,__FUNCTION__,__LINE__);
					pToModuleName = NULL;
					Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/SubscribeUri",Common_Json_Type_String,pCurrUserInfo->szSubscribeUri,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,pCurrUserInfo->szResponceUri,0,0);
					Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,pCurrUserInfo->szRessUri,0,0);
					nRet = Module_CallFunctions(hModuleHandle,pInParams,&pOutParams,3000);

				}
		
				if (pOutParams != NULL)
				{
					Common_Json_GetAttrValue(pOutParams,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
					if (nCode == 0)
					{
						if(pCurrUserInfo->fxn != NULL)
						{
							cJSON_Struct *pEventJson;
							pEventJson = Common_Json_GetItem(pOutParams,-1,"Data");
							if (pEventJson)
							{
								pCurrUserInfo->fxn(hModuleHandle,pCurrUserInfo->nSubscribeId,pEventJson,NULL,pCurrUserInfo->pUserData);
							}
						}
						Common_GetSystemCount(&nSec,NULL);
						pCurrUserInfo->nLastRenewTime = nSec;
					}
					else
					{
						 // 失败，需要重新订阅
						Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
						if (pCurrUserInfo->szRessUri != NULL)
						{
							Common_Free(pCurrUserInfo->szRessUri,__FUNCTION__,__LINE__);
							pCurrUserInfo->szRessUri = NULL;
						}
						
						Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
					}
					
				}
				
			}
		}
		Common_Json_Delete(pInParams);
		Common_Json_Delete(pOutParams);
		pInParams = NULL;
		pOutParams = NULL;
		pDeleteInfo = NULL;
		
			Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
			pCurrUserInfo->nUseCount--;
			if (pCurrUserInfo->bNeedDelete && pCurrUserInfo->nUseCount <= 0)
			{
				pDeleteInfo = pCurrUserInfo;
			
				if (pCurrUserInfo != NULL)
				{
					if (pCurrUserInfo->pPrev == NULL)
					{
						pModuleMgr->tSubscribeInfo.pUserInfoHead = pCurrUserInfo->pNext;
					}
					else 
					{
						pCurrUserInfo->pPrev->pNext = pCurrUserInfo->pNext;
					}
					if (pCurrUserInfo->pNext != NULL)
					{
						pCurrUserInfo->pNext->pPrev = pCurrUserInfo->pPrev;
					}
					pCurrUserInfo->pNext = NULL;
					pCurrUserInfo->pPrev = NULL;
				}
			}

			Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
			if (pDeleteInfo != NULL)
			{
				Common_Free(pDeleteInfo->szModuleName,__FUNCTION__,__LINE__);
				Common_Free(pDeleteInfo->szSubscribeUri,__FUNCTION__,__LINE__);
				Common_Free(pDeleteInfo->szRessUri,__FUNCTION__,__LINE__);
				Common_Free(pDeleteInfo->szResponceUri,__FUNCTION__,__LINE__);
				Common_Free(pDeleteInfo->pPrivateInfo,__FUNCTION__,__LINE__);
				Common_Free(pDeleteInfo,__FUNCTION__,__LINE__);
				
			}
			pDeleteInfo = NULL;
		
	}while(pUserInfo != NULL) ;
	if (pToModuleName != NULL)
	{
		Common_Free(pToModuleName,__FUNCTION__,__LINE__);
	}

	// 服务器端检查 是否掉线


	
	
	return 0;
}

S32 Module_QueryEvent(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct **pOutEventInfo,int nMSecTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeUser_T *pUserInfo = NULL;
	S32 i,nSubIdx = -1,nRet = -1;
	cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	S8 *szRessUri = NULL;// 分配 的资源
	if (pModuleMgr == NULL || pOutEventInfo == NULL)
	{
		return -1;
	}
	nSubIdx = (nSubscribeID) & 0xFFFF;
	if (nSubIdx < 0|| nSubIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	if (pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_NOTFOUND;
	}
	pUserInfo = pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx];
	if (pUserInfo->nSubscribeId != nSubscribeID ||
		pUserInfo->bNeedDelete)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_NOTFOUND;
	}
	if (pUserInfo->szRessUri == NULL)
	{
	}
	szRessUri = Common_StrDup(pUserInfo->szRessUri,__FUNCTION__,__LINE__);
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	if (szRessUri == NULL)
	{
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pInParams != NULL)
	{
		S8 szTmp[128],*pToModuleName = NULL;
		Common_UriOneParse(szRessUri,NULL,&pToModuleName,NULL);
		if (pToModuleName != NULL)
		{
			sprintf(szTmp,"/%s/ModuleSubscribe/Pull",pToModuleName);
			Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Get",0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,szRessUri,0,0);
			nRet = Module_CallFunctions(hModuleHandle,pInParams,&pOutParams,nMSecTimeOut);
			Common_Free(pToModuleName,__FUNCTION__,__LINE__);
			pToModuleName = NULL;
		}
		
	}
	if (pOutParams != NULL)
	{
		S32 nCode = -1;
		Common_Json_GetAttrValue(pOutParams,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
		if (nCode == 0)
		{
			if (pOutEventInfo != NULL)
			{
				cJSON_Struct *pDataJson = NULL;
				pDataJson = Common_Json_DetachItem(pOutParams,-1,"/Data");
				*pOutEventInfo = pDataJson;
			}
			
		}

	}
	Common_Json_Delete(pInParams);
	Common_Json_Delete(pOutParams);
	pInParams = NULL;
	if (szRessUri != NULL)
	{
		Common_Free(szRessUri,__FUNCTION__,__LINE__);
		szRessUri = NULL;
	}
	return nRet;
}
// 被订阅操作接口
S32 Module_RegisterSubscribe(ModuleHandle_T hModuleHandle,S8 *szSubscribeUri,Module_Subscribe_Def fxn,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeOwner_T *pOwerInfo = NULL;
	S32 i,nFreeIdx = -1,nCount = 0,nSubscribeId;
	cJSON_Struct *pEventJson = NULL;
	if (pModuleMgr == NULL || szSubscribeUri == NULL || fxn == NULL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	// 查找是否存在
	for (i = 0;i < LIBMODULE_MAX_SUBSCRIBE_NUM;i++)
	{
		if (NULL != pModuleMgr->tSubscribeInfo.pOwnerInfoList[i])
		{
			if (0 == Common_StriCmp(szSubscribeUri,pModuleMgr->tSubscribeInfo.pOwnerInfoList[i]->szSubscribeUri))
			{// 存在,更新
				S32 nRecvId,nUserIdx,nUserCount;
				pEventJson = NULL;
				pOwerInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[i];
				if (pOwerInfo->bInvalid)
				{// 无效，则触发重订阅
					
					nUserCount = 0;
					for (nUserIdx = 0; nUserIdx < LIBMODULE_MAX_SUBSCRIBE_NUM&& nUserCount < pOwerInfo->nUserCount;nUserIdx++)
					{
						if (pOwerInfo->pUserInfoList[nUserIdx] != NULL)
						{
							S8 *szSubscribeUri = NULL;
							nRecvId = pOwerInfo->pUserInfoList[nUserIdx]->nSubscribeId;
							szSubscribeUri = Common_StrDup(pOwerInfo->pUserInfoList[nUserIdx]->szSubscribeUri,__FUNCTION__,__LINE__);
							Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
							fxn(hModuleHandle,0,
								nRecvId,
								szSubscribeUri,&pEventJson,pUserData);
							if (pEventJson != NULL)
							{
								Module_SendEvent(hModuleHandle,nRecvId,pEventJson,NULL,3000);
								Common_Json_Delete(pEventJson);
								pEventJson = NULL;
							}
							Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
							Common_Free(szSubscribeUri,__FUNCTION__,__LINE__);
							szSubscribeUri = NULL;
							nUserCount++;
						}
					}
					
				}
				pOwerInfo->bInvalid = 0;
				pOwerInfo->pUserData = pUserData;
				pOwerInfo->fxn = fxn;
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
				
				return 0;
			}
		}
		else if(nFreeIdx == -1)
		{
			nFreeIdx = i;
		}
		nCount++;
		if (nCount >= pModuleMgr->tSubscribeInfo.nOwerCount && nFreeIdx != -1)
		{
			break;
		}
	}
	if (nFreeIdx == -1)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pOwerInfo = (LibModuleSubscribeOwner_T *)Common_Malloc(sizeof(LibModuleSubscribeOwner_T),0,__FUNCTION__,__LINE__);
	if (pOwerInfo == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	memset(pOwerInfo,0,sizeof(LibModuleSubscribeOwner_T));
	pOwerInfo->szSubscribeUri = Common_StrDup(szSubscribeUri,__FUNCTION__,__LINE__);
	if (pOwerInfo->szSubscribeUri == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		Common_Free(pOwerInfo,__FUNCTION__,__LINE__);
		pOwerInfo = NULL;
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pOwerInfo->fxn = fxn;
	pOwerInfo->pUserData = pUserData;
	pModuleMgr->tSubscribeInfo.dwHandleCount++;
	if (pModuleMgr->tSubscribeInfo.dwHandleCount > 0x7FFF)
	{
		pModuleMgr->tSubscribeInfo.dwHandleCount = 1;
	}
	nSubscribeId = (pModuleMgr->tSubscribeInfo.dwHandleCount << 16) | (nFreeIdx);
	pOwerInfo->nSubscribeId = nSubscribeId;
	pModuleMgr->tSubscribeInfo.pOwnerInfoList[nFreeIdx] = pOwerInfo;
	pModuleMgr->tSubscribeInfo.nOwerCount++;

	pOwerInfo->pNext = pModuleMgr->tSubscribeInfo.pOwnerInfoHead;
	if (pModuleMgr->tSubscribeInfo.pOwnerInfoHead != NULL)
	{
		pModuleMgr->tSubscribeInfo.pOwnerInfoHead->pPrev = pOwerInfo;
	}
	pModuleMgr->tSubscribeInfo.pOwnerInfoHead = pOwerInfo;

	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	Module_Subscribe_Save(hModuleHandle);
	return 0;
}
S32 Module_UnRegisterSubscribe(ModuleHandle_T hModuleHandle,S8 *szSubscribeUri)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeOwner_T *pOwerInfo = NULL;
	S32 i,nFoundIdx = -1,nCount = 0,nSubscribeId;
	if (pModuleMgr == NULL || szSubscribeUri == NULL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	// 查找是否存在
	for (i = 0;i < LIBMODULE_MAX_SUBSCRIBE_NUM;i++)
	{
		if (NULL != pModuleMgr->tSubscribeInfo.pOwnerInfoList[i])
		{
			if (0 == Common_StriCmp(szSubscribeUri,pModuleMgr->tSubscribeInfo.pOwnerInfoList[i]->szSubscribeUri))
			{
				nFoundIdx = i;
				break;
			}
		}
		nCount++;
		if (nCount >= pModuleMgr->tSubscribeInfo.nOwerCount)
		{
			break;
		}
	}
	if (nFoundIdx == -1)
	{// 未找到
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_NOTFOUND;
	}
	pOwerInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nFoundIdx];
	if (pOwerInfo->nUserCount > 0)
	{
		pOwerInfo->bInvalid = 1;
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return 0;
	}
	if (pOwerInfo->pPrev == NULL)
	{
		pModuleMgr->tSubscribeInfo.pOwnerInfoHead = pOwerInfo->pNext;
		if (pModuleMgr->tSubscribeInfo.pOwnerInfoHead != NULL)
		{
			pModuleMgr->tSubscribeInfo.pOwnerInfoHead->pPrev = NULL;
		}
	}
	else if (pOwerInfo->pNext == NULL)
	{
		pOwerInfo->pPrev->pNext = NULL;
	}
	else
	{
		pOwerInfo->pPrev->pNext = pOwerInfo->pNext;
		pOwerInfo->pNext->pPrev = pOwerInfo->pPrev;
	}
	pModuleMgr->tSubscribeInfo.pOwnerInfoList[nFoundIdx] = NULL;
	pModuleMgr->tSubscribeInfo.nOwerCount--;
	if (pOwerInfo->szSubscribeUri != NULL)
	{
		Common_Free(pOwerInfo->szSubscribeUri,__FUNCTION__,__LINE__);
		pOwerInfo->szSubscribeUri = NULL;
	}
	Common_Free(pOwerInfo,__FUNCTION__,__LINE__);
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	Module_Subscribe_Save(hModuleHandle);
	return 0;
}
S32 Module_SendEvent(ModuleHandle_T hModuleHandle,S32 nRecvID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,S32 nMSecTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeOwner_T *pOwerInfo = NULL;
	S32 i,nFoundIdx = -1,nCount = 0;
	S32 nSubIdx,nUserIdx;
	S8 *szResponceUri = NULL;
	cJSON_Struct *pInParams = NULL;
	S32 nRet = -1;
	if (pModuleMgr == NULL || pEventInfo == NULL)
	{
		return -1;
	}
	nSubIdx = (nRecvID >> 8) & 0xFF;
	nUserIdx = (nRecvID & 0xFF) - 1;
	if (nSubIdx < 0 || nSubIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM || nUserIdx < 0 || nUserIdx >= LIBMODULE_MAX_SUBSCRIBE_NUM)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_NOTFOUND;
	}
	if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->bInvalid ||
		pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_NOTFOUND;
	}
	if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nSubscribeId != nRecvID)
	{
		Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		return MODULE_ERROR_TYPE_NOTFOUND;
	}
	// 条件满足
	szResponceUri = Common_StrDup(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->szResponceUri,__FUNCTION__,__LINE__);
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	if (szResponceUri == NULL)
	{
		return -1;
	}
	pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pInParams != NULL)
	{
		S8 szTmp[128],*pToModuleName = NULL;
		Common_UriOneParse(szResponceUri,NULL,&pToModuleName,NULL);
		if (pToModuleName != NULL)
		{
			cJSON_Struct *pOutJsonData = NULL;
			sprintf(szTmp,"/%s/ModuleSubscribe/Push",pToModuleName);
			Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,szResponceUri,0,0);
			Common_Json_AddItem(pInParams,-1,"Data",pEventInfo);
			nRet = Module_CallFunctions(hModuleHandle,pInParams,&pOutJsonData,nMSecTimeOut);
			if (pOutJsonData != NULL)
			{
				S32 nSec = 0;
				S32 nCode = -1;
				Common_Json_GetAttrValue(pOutJsonData,-1,"Header/Code",NULL,NULL,&nCode,NULL);
				if(nCode == 0)
				{
					Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
					Common_GetSystemCount(&nSec,NULL);
					if(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL && 
						pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] != NULL)
					{
						 pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nLastRenewTime = nSec;
					}
					Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
				}
				if (pOutParams != NULL)
				{
					*pOutParams = pOutJsonData;
					pOutJsonData = NULL;
				}
				Common_Json_Delete(pOutJsonData);

			}
			Common_Json_DetachItem(pInParams,-1,"Data");
			Common_Free(pToModuleName,__FUNCTION__,__LINE__);
			pToModuleName = NULL;
		}
		

	}
	Common_Json_Delete(pInParams);
	pInParams = NULL;

	if (szResponceUri != NULL)
	{
		Common_Free(szResponceUri,__FUNCTION__,__LINE__);
		szResponceUri = NULL;
	}
		
	return nRet;
}

S32 Module_Subscribe_do_Offline(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleSubscribeOwner_T *pOwnInfo;
	LibModuleSubscribeUser_T *pUserInfo,*pCurrUserInfo = NULL;
	S32 nSubIdx,nUserIdx,nCount,nUserCount,nUserOrgCount;
	S32 nSec = 0;
	Common_GetSystemCount(&nSec,NULL);
	Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
	nCount = 0;
	pOwnInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoHead;
	while(pOwnInfo != NULL)
	{
		if (pOwnInfo->bInvalid)
		{
			pOwnInfo = pOwnInfo->pNext;
			continue;
		}
		pUserInfo = pOwnInfo->pUserInfoHead;
		while(pUserInfo != NULL)
		{
			pCurrUserInfo = pUserInfo;
			pUserInfo = pUserInfo->pNext;
			//
			if (pCurrUserInfo->nLastRenewTime + MODULE_SUBSCRIBE_RENEW_INTERVAL * 3 < nSec)
			{
				Module_Subscribe_Def fxn = pOwnInfo->fxn;
				void *pUserData = pOwnInfo->pUserData;
				S32 nSubscribeId = pCurrUserInfo->nSubscribeId;
				S8 *pDeleteUri = NULL;
				pDeleteUri = Common_StrDup(pCurrUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
				// 掉线
				// 回调
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
				if (fxn != NULL)
				{
					fxn(hModuleHandle,1,nSubscribeId,pDeleteUri,NULL,pUserData);
				}
				
				Common_Free(pDeleteUri,__FUNCTION__,__LINE__);
				Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
				// 
				if (pCurrUserInfo->pPrev == NULL)
				{
					pOwnInfo->pUserInfoHead = pCurrUserInfo->pNext;
					if (pOwnInfo->pUserInfoHead != NULL)
					{
						pOwnInfo->pUserInfoHead->pPrev = NULL;
					}
				}
				else if (pCurrUserInfo->pNext == NULL)
				{
					pCurrUserInfo->pPrev->pNext = NULL;
				}
				else
				{
					pCurrUserInfo->pPrev->pNext = pCurrUserInfo->pNext;
					pCurrUserInfo->pNext->pPrev = pCurrUserInfo->pPrev;
				}
				nUserIdx = (pCurrUserInfo->nSubscribeId & 0xFF) - 1;
				pOwnInfo->pUserInfoList[nUserIdx] = NULL;
				pOwnInfo->nUserCount--;
				pCurrUserInfo->pNext = NULL;
				pCurrUserInfo->pPrev = NULL;
				pCurrUserInfo->pNext = pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead;
				if (pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead != NULL)
				{
					pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead->pPrev = pCurrUserInfo;
				}
				pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead = pCurrUserInfo;
				Module_Subscribe_Save(hModuleHandle);
				
			}
		}

		pOwnInfo = pOwnInfo->pNext;
	}
	pUserInfo = pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead;
	while(pUserInfo != NULL)
	{
		pCurrUserInfo = pUserInfo;
		pUserInfo = pUserInfo->pNext;
		if (pCurrUserInfo->nLastRenewTime + MODULE_SUBSCRIBE_RENEW_INTERVAL * 4 < nSec)
		{
			if (pCurrUserInfo->pPrev == NULL)
			{
				pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead = pCurrUserInfo->pNext;
				if (pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead != NULL)
				{
					pModuleMgr->tSubscribeInfo.pUserInfoDeleteHead->pPrev = NULL;
				}
			}
			else if (pCurrUserInfo->pNext == NULL)
			{
				pCurrUserInfo->pPrev->pNext = NULL;
			}
			else
			{
				pCurrUserInfo->pPrev->pNext = pCurrUserInfo->pNext;
				pCurrUserInfo->pNext->pPrev = pCurrUserInfo->pPrev;
			}
			Common_Free(pCurrUserInfo->szModuleName,__FUNCTION__,__LINE__);
			Common_Free(pCurrUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
			Common_Free(pCurrUserInfo->szRessUri,__FUNCTION__,__LINE__);
			Common_Free(pCurrUserInfo->szResponceUri,__FUNCTION__,__LINE__);
			Common_Free(pCurrUserInfo->pPrivateInfo,__FUNCTION__,__LINE__);
			Common_Free(pCurrUserInfo,__FUNCTION__,__LINE__);
			pCurrUserInfo = NULL;
		}
	}
	
		
	
	Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
	return 0;
}
S32 Module_Subscribe_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
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
	nLen = sprintf(szTmp,"/%s/ModuleSubscribe/",pModuleMgr->pszModuleName);
	if (0 != Common_StrniCmp(pStringValue,szTmp,nLen))
	{// 非本模块
		return -1;
	}
	pDeal = pStringValue + nLen;
	Common_Json_GetAttrValue(pInParams,-1,"Header/SubscribeUri",NULL,&pSubScribeUri,NULL,NULL);
	Common_Json_GetAttrValue(pInParams,-1,"Header/ReceiveUri",NULL,&pResponceUri,NULL,NULL);
	Common_Json_GetAttrValue(pInParams,-1,"Header/ResUri",NULL,&pRessUri,NULL,NULL);
	if (0 == Common_StriCmp(pDeal,"Renew"))
	{// Post
		cJSON_Struct *pNewResult = NULL;
		if (pSubScribeUri != NULL && 
			pResponceUri != NULL &&
			pRessUri != NULL)
		{// 
			S8 *pTmpStr = NULL;
			
			
			pTmpStr = strstr(pRessUri,"/Ress/");
			if (pTmpStr)
			{
				sscanf(pTmpStr + 6,"%d/%d",&nOwnerId,&nUserId);
			}
			nSubIdx = (nOwnerId) & 0xFFFF;
			nUserIdx = (nUserId & 0xFF) - 1;
			nCount = 0;
			pOwnInfo = NULL;
			if (nSubIdx < LIBMODULE_MAX_SUBSCRIBE_NUM && nUserIdx < LIBMODULE_MAX_SUBSCRIBE_NUM && nUserIdx >= 0)
			{
				Module_Subscribe_Def fxn = NULL;
				void *pUserData = NULL;
				S32 nRecvId = 0;
				Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
				if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL &&
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->nSubscribeId == nOwnerId)
				{
					if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] != NULL && 
						pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nSubscribeId == nUserId)
					{
						pOwnInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx];
						pUserInfo = pOwnInfo->pUserInfoList[nUserIdx];
						if (0 == Common_StriCmp(pUserInfo->szRessUri,pRessUri) && 
							0 == Common_StriCmp(pUserInfo->szSubscribeUri,pSubScribeUri) &&
							0 == Common_StriCmp(pUserInfo->szResponceUri,pResponceUri))
						{
							fxn = pOwnInfo->fxn;
							pUserData = pOwnInfo->pUserData;
							nRecvId = nUserId;
							Common_GetSystemCount(&nSec,NULL);
							pUserInfo->nLastRenewTime = nSec;
							nCode = 0;
						}
					}
				}
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
				if (fxn != NULL)
				{
					pOutParamsJson = NULL;
					fxn(hModuleHandle,2,nRecvId,pSubScribeUri,&pOutParamsJson,pUserData);
					pNewResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
					if (pNewResult != NULL)
					{
						Common_Json_SetAttrValue(pNewResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pNewResult,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
						if (pOutParamsJson != NULL)
						{
							Common_Json_AddItem(pNewResult,-1,"Data",pOutParamsJson);

						}
						pOutParamsJson = pNewResult;
					}
					
				}
			}
		}
	}
	else if (0 == Common_StriCmp(pDeal,"Post"))
	{// Post
		cJSON_Struct *pNewResult = NULL;
		if (pSubScribeUri != NULL)
		{// 
			nCount = 0;
			pOwnInfo = NULL;
			Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
			for (nSubIdx = 0;nSubIdx < LIBMODULE_MAX_SUBSCRIBE_NUM && nCount <  pModuleMgr->tSubscribeInfo.nOwerCount;nSubIdx++)
			{
				
				if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] == NULL)
				{
					continue;
				}
				nCount++;
				if (NULL != strstr(pSubScribeUri,pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->szSubscribeUri))
				{// 找到
					pOwnInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx];
					break;
				}
			}
			if (pOwnInfo != NULL)
			{
				nCount = 0;
				pUserInfo = NULL;
				nFreeIdx = -1;
				for (nUserIdx = 0;nUserIdx < LIBMODULE_MAX_SUBSCRIBE_NUM ;nUserIdx++)
				{

					if (pOwnInfo->pUserInfoList[nUserIdx] != NULL)
					{
						nCount++;
						if (0 == Common_StriCmp(pSubScribeUri,pOwnInfo->pUserInfoList[nUserIdx]->szSubscribeUri) && 
							0 == Common_StriCmp(pResponceUri,pOwnInfo->pUserInfoList[nUserIdx]->szResponceUri))
						{// 找到,已订阅
							nCode = 0;
							pUserInfo = pOwnInfo->pUserInfoList[nUserIdx];
							pOutParamsJson = NULL;
							if (pOwnInfo->fxn != NULL)
							{
								pOwnInfo->fxn(hModuleHandle,2,pUserInfo->nSubscribeId,pUserInfo->szSubscribeUri,&pOutParamsJson,pOwnInfo->pUserData);
							}
							pNewResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
							if (pNewResult != NULL)
							{
								Common_Json_SetAttrValue(pNewResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
								Common_Json_SetAttrValue(pNewResult,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
								Common_Json_SetAttrValue(pNewResult,-1,"Header/ResUri",Common_Json_Type_String,pUserInfo->szRessUri,0,0);
								if (pOutParamsJson != NULL)
								{
									Common_Json_AddItem(pNewResult,-1,"Data",pOutParamsJson);
									
								}
								pOutParamsJson = pNewResult;
							}
							Common_GetSystemCount(&nSec,NULL);
							pUserInfo->nLastRenewTime = nSec;
							
							break;
						}
					}
					else if (nFreeIdx == -1)
					{
						nFreeIdx = nUserIdx;
					}
					if (nFreeIdx != -1 && nCount >=  pOwnInfo->nUserCount)
					{
						break;
					}
				}
				if (pUserInfo == NULL)
				{// 未找到
					if (nFreeIdx != -1)
					{
						// 新的
						pUserInfo = (LibModuleSubscribeUser_T *)Common_Malloc(sizeof(LibModuleSubscribeUser_T),0,__FUNCTION__,__LINE__);
						if (pUserInfo != NULL)
						{
							U32 dwHandleCount = pModuleMgr->tSubscribeInfo.dwHandleCount;
							dwHandleCount++;
							if (dwHandleCount > 0x7FFF)
							{
								dwHandleCount = 1;
							}
							nUserId = (dwHandleCount << 16) | (pOwnInfo->nSubscribeId << 8)| (nFreeIdx + 1);
							sprintf(szTmp,"/%s/ModuleSubscribe/Ress/%d/%d",pModuleMgr->pszModuleName,pOwnInfo->nSubscribeId,nUserId);
							memset(pUserInfo,0,sizeof(LibModuleSubscribeUser_T));
							pUserInfo->szSubscribeUri = Common_StrDup(pSubScribeUri,__FUNCTION__,__LINE__);
							pUserInfo->szResponceUri = Common_StrDup(pResponceUri,__FUNCTION__,__LINE__);
							pUserInfo->szRessUri = Common_StrDup(szTmp,__FUNCTION__,__LINE__);
							Common_UriOneParse(pResponceUri,NULL,&pUserInfo->szModuleName,NULL);
							
							
							if(pUserInfo->szSubscribeUri != NULL &&
							   pUserInfo->szResponceUri != NULL && 
							   pUserInfo->szRessUri != NULL &&
							   pUserInfo->szModuleName != NULL)
							{
								// 只要有资源就一定成功，而不管是否支持
								pModuleMgr->tSubscribeInfo.dwHandleCount = dwHandleCount;
								pUserInfo->nSubscribeId = nUserId;
								pOwnInfo->pUserInfoList[nFreeIdx] = pUserInfo;
								pOwnInfo->nUserCount++;
								pUserInfo->pNext = pOwnInfo->pUserInfoHead;
								if (pOwnInfo->pUserInfoHead != NULL)
								{
									pOwnInfo->pUserInfoHead->pPrev = pUserInfo;
								}
								pOwnInfo->pUserInfoHead = pUserInfo;

								nCode = -1;
								pOutParamsJson = NULL;
								if(pOwnInfo->fxn != NULL && 0 == pOwnInfo->fxn(hModuleHandle,0,nUserId,pSubScribeUri,&pOutParamsJson,pOwnInfo->pUserData))
								{// 订阅处理
									nCode = 0;
									if (pOutParamsJson != NULL)
									{
										
										if(Common_Json_GetAttrValue(pOutParamsJson,-1,"Header/Code",NULL,NULL,&nCode,NULL))
										{
											if (nCode == 0)
											{// 成功,只有成功时才把结果返回,否则总是构成一个成功的结果。
												
											}

										}
									}
								}
									
									
									if (nCode != 0)
									{
										
										nCode = 0;
										pNewResult = NULL;
										pNewResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
										if (pNewResult != NULL)
										{
											Common_Json_SetAttrValue(pNewResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
											Common_Json_SetAttrValue(pNewResult,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
											Common_Json_SetAttrValue(pNewResult,-1,"Header/ResUri",Common_Json_Type_String,pUserInfo->szRessUri,0,0);
											if (pOutParamsJson != NULL)
											{
												Common_Json_AddItem(pNewResult,-1,"Data",pOutParamsJson);
											}
											pOutParamsJson = pNewResult;
										}
									}
									Common_GetSystemCount(&nSec,NULL);
									pUserInfo->nLastRenewTime = nSec;
									
									bSave = 1;
								
								

							}
							else
							{
								Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
								Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
								Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
								Common_Free(pUserInfo->szModuleName,__FUNCTION__,__LINE__);
								Common_Free(pUserInfo,__FUNCTION__,__LINE__);
								nCode = MODULE_ERROR_TYPE_LIMITED; 
							}
						}
						else
						{
							nCode = MODULE_ERROR_TYPE_LIMITED;
						}
					}
					else
					{
						nCode = MODULE_ERROR_TYPE_LIMITED;
					}
				}
			}
			Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
		}
	}
	else if (0 == Common_StriCmp(pDeal,"Delete"))
	{// Delete
		S8 *pTmpStr;
		Module_Subscribe_Def fxn = NULL;
		void *pUserData = NULL;
		S8 *pDeleteSubScribeUri = NULL;
		pTmpStr = strstr(pRessUri,"/Ress/");
		if (pTmpStr != NULL)
		{
			
			sscanf(pTmpStr + 6,"%d/%d",&nOwnerId,&nUserId);
			nSubIdx = (nOwnerId) & 0xFFFF;
			nUserIdx = ((nUserId) & 0xFF) - 1;
			if (nSubIdx >= 0 && nSubIdx < LIBMODULE_MAX_SUBSCRIBE_NUM &&
				nUserIdx >= 0 && nUserIdx < LIBMODULE_MAX_SUBSCRIBE_NUM)
			{
				Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
				if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL &&
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->nSubscribeId == nOwnerId)
				{
					if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] != NULL && 
						pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nSubscribeId == nUserId)
					{
	
							fxn = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->fxn;
							pUserData = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserData;
							pDeleteSubScribeUri = Common_StrDup(pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->szSubscribeUri,__FUNCTION__,__LINE__);

					}
				}
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
			}
			
		}
		if (fxn != NULL)
		{// 回调
			fxn(hModuleHandle,1,nUserId,pDeleteSubScribeUri,NULL,pUserData);
			//放在后面，避免用户数据删除
			Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
			if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL && 
				pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] != NULL && 
				pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx]->nSubscribeId == nUserId)
			{
				pOwnInfo =  pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx];
				pUserInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx];
				if (pUserInfo->pPrev == NULL)
				{
					pOwnInfo->pUserInfoHead = pUserInfo->pNext;
					if (pOwnInfo->pUserInfoHead != NULL)
					{
						pOwnInfo->pUserInfoHead->pPrev = NULL;
					}
				}
				else if (pUserInfo->pNext == NULL)
				{
					pUserInfo->pPrev->pNext = NULL;
				}
				else
				{
					pUserInfo->pPrev->pNext = pUserInfo->pNext;
					pUserInfo->pNext->pPrev = pUserInfo->pPrev;
				}
				
				pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->pUserInfoList[nUserIdx] = NULL;
				pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->nUserCount--;
				Common_Free(pUserInfo->szModuleName,__FUNCTION__,__LINE__);
				Common_Free(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
				Common_Free(pUserInfo->szRessUri,__FUNCTION__,__LINE__);
				Common_Free(pUserInfo->szResponceUri,__FUNCTION__,__LINE__);
				Common_Free(pUserInfo->pPrivateInfo,__FUNCTION__,__LINE__);
				Common_Free(pUserInfo,__FUNCTION__,__LINE__);
				pUserInfo = NULL;
				bSave = 1;
			}
			
			Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
			Common_Free(pDeleteSubScribeUri,__FUNCTION__,__LINE__);
			pOutParamsJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pOutParamsJson != NULL)
			{
				Common_Json_SetAttrValue(pOutParamsJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pOutParamsJson,-1,"Header/Code",Common_Json_Type_Number,0,0,0);
			}
			nCode = 0; 
		}
	}
	else if (0 == Common_StriCmp(pDeal,"Pull"))
	{// Ress
		S8 *pTmpStr = NULL;
		Module_Subscribe_Def fxn = NULL;
		void *pUserData = NULL;
		S8 *szSubscribeUri = NULL;
		pTmpStr = strstr(pRessUri,"/Ress/");
		sscanf(pTmpStr + 6,"%d/%d",&nOwnerId,&nUserId);
	
			nSubIdx = (nOwnerId) & 0xFFFF;
			nUserIdx = ((nUserId) & 0xFF) - 1;
			if (nSubIdx >= 0 && nSubIdx < LIBMODULE_MAX_SUBSCRIBE_NUM &&
				nUserIdx >= 0 && nUserIdx < LIBMODULE_MAX_SUBSCRIBE_NUM)
			{
				Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
				if (pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx] != NULL &&
					pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx]->nSubscribeId == nOwnerId)
				{
					pOwnInfo = pModuleMgr->tSubscribeInfo.pOwnerInfoList[nSubIdx];
					if (pOwnInfo->pUserInfoList[nUserIdx] != NULL && 
						pOwnInfo->pUserInfoList[nUserIdx]->nSubscribeId == nUserId)
					{
						pUserInfo = pOwnInfo->pUserInfoList[nUserIdx];

							fxn = pOwnInfo->fxn;
							pUserData = pOwnInfo->pUserData;
							szSubscribeUri = Common_StrDup(pUserInfo->szSubscribeUri,__FUNCTION__,__LINE__);
					}
				}
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
			}

	
		if (fxn != NULL)
		{// 回调
			fxn(hModuleHandle,2,nUserId,szSubscribeUri,pOutParams,pUserData);
			nCode = 0;
		}
		if (szSubscribeUri != NULL)
		{
			Common_Free(szSubscribeUri,__FUNCTION__,__LINE__);
			szSubscribeUri = NULL;
		}
	}
	else if (0 == Common_StriCmp(pDeal,"Push"))
	{// Receive
		S8 *pTmpStr = NULL;
		Module_Events_Def fxn = NULL;
		void *pUserData = NULL;
		if (pResponceUri != NULL)
		{
			pTmpStr = strstr(pResponceUri,"/Receive/");
			if (pTmpStr != NULL)
			{
				nUserId = atoi(pTmpStr + 9);
			}
			
			nSubIdx = (nUserId) & 0xFFFF;
			if (nSubIdx >= 0 && nSubIdx < LIBMODULE_MAX_SUBSCRIBE_NUM )
			{
				Common_Lock(pModuleMgr->tSubscribeInfo.hLock);
				if (pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx] != NULL)
				{
					if (pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx]->nSubscribeId == nUserId)
					{

						fxn = pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx]->fxn;
						pUserData = pModuleMgr->tSubscribeInfo.pUserInfoList[nSubIdx]->pUserData;

					}
				}
				Common_UnLock(pModuleMgr->tSubscribeInfo.hLock);
			}


			if (fxn != NULL)
			{// 回调
				cJSON_Struct *pDataInfo;
				pDataInfo = Common_Json_GetItem(pInParams,-1,"Data");
				if (pDataInfo != NULL)
				{
					fxn(hModuleHandle,nUserId,pDataInfo,pOutParams,pUserData);
				}

				nCode = 0;
			}
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
		Module_Subscribe_Save(hModuleHandle);
	}
	return 0;
}

