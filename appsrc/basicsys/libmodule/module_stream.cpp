#include "libcommon_api.h"
#include "libmodule_api.h"
#include "libmodule_struct.h"
#include "libstreamqueue_api.h"
#include "common_media_struct.h"

#ifdef WIN32
#define STREAM_FILE_PATH "c:/tmp/modules/queue/"
#else
#define STREAM_FILE_PATH "/tmp/modules/queue/"
#endif
/* ress
操作(szUri):
/模块名/StreamQueue/Post    --- 客户端打开流   szUri
/模块名/StreamQueue/Delete    --- 客户端关闭流
/模块名/StreamQueue/Control    --- 客户端控制流
/模块名/StreamQueue/Kick    --- 服务器踢出客户端
资源:
/模块名/StreamQueue/Ress/<OwnerId/CoOwner>/<UserId>    --- 服务器资源对应user资源 szResUri for delete
/模块名/StreamQueue/Stream/<OwnerId>    --- 服务器资源对应底层流缓冲队列 szStreamRess
/模块名/StreamQueue/Receive/<ModuleMark>/<UserId>/<StreamRessIndex>   --- 客户端接收资源 ，szReceiveUri
*/

static S32 g_hInvalidStreamQueueId = -1;


static S32 g_nTestQueue_status1=0;
static S32 g_nTestQueue_status2=0;
static S32 g_nTestQueue_status3=0;
static S32 g_nTestQueue_status4=0;

S32 Module_StreamQueue_Test(S8 *szName)
{
	//printf("[%s]*****Stream Queue Test status[%d,%d,%d,%d]\n",szName?szName:"",g_nTestQueue_status1,g_nTestQueue_status2,g_nTestQueue_status3,g_nTestQueue_status4);
	return 0;
}

static S32 static_StreamQueue_loadCfg(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL;
	//LibModuleStreamQueueOwner_T *pOwerInfo = NULL;
	//LibModuleStreamQueueCoOwner_T *pCoOwerInfo = NULL;
	//LibModuleStreamQueueUser_T *pUserInfo = NULL;
	//int nSubIdx,nUserIdx,nWhich,nWhichUser;
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
		sprintf(szFileName,"%s/%s.json",STREAM_FILE_PATH,pModuleMgr->pszModuleName);
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
	Common_Json_GetAttrValue(pSaveJson,-1,"HandleCount",NULL,&pValueString,&nValueNumber,NULL);
	pModuleMgr->tStreamQueueInfo.nStaticCount = nValueNumber;
	// receive list
	pArray = Common_Json_GetAttrValue(pSaveJson,-1,"OwnerList",NULL,NULL,NULL,NULL);
	if(pArray != NULL)
	{
		//cJSON_Struct *pWhich = NULL;
		S32 nArrayCount,i;
		S8 *szReceiveUri = NULL;
		nArrayCount = Common_Json_ArraySize(pArray);

		for(i = 0; i < nArrayCount;i++)
		{
			szReceiveUri= NULL;

			Common_Json_GetAttrValue(pArray,i,NULL,NULL,&szReceiveUri,NULL,NULL);
			if(szReceiveUri != NULL)
			{
				cJSON_Struct *pInParams = NULL;
				S8 *pToModuleName = NULL;
				S8 szTmp[128];

				// 处理,通知使用者
				pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if (pInParams != NULL)
				{
					pToModuleName = NULL;
					Common_UriOneParse(szReceiveUri,NULL,&pToModuleName,NULL);
					if (pToModuleName!= NULL)
					{
						//S32 nRet = -1;
						sprintf(szTmp,"/%s/StreamQueue/Kick",pToModuleName);
						Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
						Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
						Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,szReceiveUri,0,0);
						Module_CallFunctions(hModuleHandle,pInParams,NULL,0);

						Common_Free(pToModuleName,__FUNCTION__,__LINE__);
						pToModuleName = NULL;

					}
					Common_Json_Delete(pInParams);
					pInParams = NULL;
				}

			}
		}
	}

	Common_Json_Delete(pSaveJson);
	pSaveJson = NULL;



	return 0;
}

static S32 static_StreamQueue_SaveCfg(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pSaveJson = NULL,*pArray = NULL,*pArrayUser = NULL;
	LibModuleStreamQueueOwner_T *pOwerInfo = NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwerInfo = NULL;
	LibModuleStreamQueueUser_T *pUserInfo = NULL;
	int nSubIdx,nUserIdx,nWhich,nWhichUser,i;
	S8 *pJsonString = NULL;
	S32 nLen;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	pSaveJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pSaveJson != NULL)
	{
		S32 nPoolIdx = 0,nOwerIdx=0,nUserIdx=0;
		S32 nStreamQueueId = -1,nType = 0;
		S8 *szReceiveUri = NULL;
		Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=1;

		Common_Json_SetAttrValue(pSaveJson,-1,"HandleCount",Common_Json_Type_Number,NULL,pModuleMgr->tStreamQueueInfo.nStaticCount,0);
		pArray = Common_Json_SetAttrValue(pSaveJson,-1,"OwnerList",Common_Json_Type_Array,NULL,0,0);
		//owner
		if(pArray != NULL)
		{
			// printf("####################nInfoPoolCount = %d \n",pModuleMgr->tStreamQueueInfo.nInfoPoolCount);
			for(i = 0; i < LIBMODULE_STREAMQUEUE_MAX_POOL;i++)
			{
				if(nPoolIdx >= pModuleMgr->tStreamQueueInfo.nInfoPoolCount)
				{
					break;
				}
				pUserInfo = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[i];
				if(pUserInfo == NULL)
				{
					continue;
				}
				nPoolIdx++;
				nStreamQueueId = pUserInfo->nStreamQueueId;
				nType = (nStreamQueueId >> 14) & 0x3;
				if(nType == 0)
				{
					pOwerInfo = (LibModuleStreamQueueOwner_T *)pUserInfo;
				//printf("[%d] type = %d id = %x receive = %s res=%p\n",i,nType,nStreamQueueId,pOwerInfo->szStreamRess?pOwerInfo->szStreamRess:"",pOwerInfo->szUri);

				}
				else if(nType == 1)
				{
					pCoOwerInfo = (LibModuleStreamQueueCoOwner_T *)pUserInfo;
				//printf("[%d] type = %d id = %x receive = %s res=%p\n",i,nType,nStreamQueueId,pCoOwerInfo->szUri?pCoOwerInfo->szUri:"",pCoOwerInfo->szUri);

				}
				else if(nType == 2)
				{// user
				//printf("[%d] type = %d id = %x receive = %s res=%p nOwerIdx = %d\n",i,nType,nStreamQueueId,pUserInfo->szReceiveUri?pUserInfo->szReceiveUri:"",pUserInfo->szResUri,nOwerIdx);

					if(pUserInfo->szReceiveUri != NULL)
					{
						Common_Json_SetAttrValue(pArray,nOwerIdx,NULL,Common_Json_Type_String,pUserInfo->szReceiveUri,0,0);
						nOwerIdx++;
						//Common_Json_StandardPrint(pSaveJson,"json <",">",NULL);
					}
				}
			}
		}
		//user

		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		// 写文件
		nLen = 0;
		pJsonString = Common_Json_Print(pSaveJson,&nLen);
		if (pJsonString != NULL)
		{
			if (!Common_File_MkDir(STREAM_FILE_PATH))
			{//
				char szFileName[128];
				FILE*pFD;
				sprintf(szFileName,"%s/%s.json",STREAM_FILE_PATH,pModuleMgr->pszModuleName);
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

S32 Module_StreamQueue_Require_Filter_Delete(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);

void StreamQueue_CheckValid(LibModuleInfo_T *pModuleMgr)
{
	LibModuleStreamQueueUser_T *pCurrUser = NULL;
	LibModuleStreamQueueStreamRess_T *pStreamRess = NULL;
	char szTmp[256];
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	// ?? ??????
			{
				int nIdx,nPoolCnt = 0,nPoolTotalCnt = pModuleMgr->tStreamQueueInfo.nInfoPoolCount;
				// LOGD("nInfoPoolCount = %d \n",nPoolTotalCnt);
				for (nIdx = 0;nIdx < LIBMODULE_STREAMQUEUE_MAX_POOL && nPoolCnt < nPoolTotalCnt;nIdx++)
				{
					if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] !=NULL)
					{
						nPoolCnt++;

						pCurrUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
						int nStreamQueueId = pCurrUser->nStreamQueueId;
						int nType = (nStreamQueueId >> 14) & 0x3;
						if(pCurrUser->szReceiveUri != NULL && nType == 2)
						{
							LibModuleRegInfo_T *pRegInfo = NULL;
							int bFound = 0,nLen = 0;
							pRegInfo = pModuleMgr->pModuleRegInfoHead;
							while (pRegInfo != NULL)
							{

									 nLen = sprintf(szTmp,"/%s/StreamQueue/Receive/%d/",pRegInfo->pszModuleName,pRegInfo->nModuleMark);
									// LOGD("[%d ]bOnline <%d> nType <%d> <%s>-> <%s> \n",nIdx,pRegInfo->bOnline,nType,szTmp,pCurrUser->szReceiveUri);
									 if(0 == Common_StrnCmp(pCurrUser->szReceiveUri,szTmp,nLen))
									 {
										if(pRegInfo->bOnline)
										{
										//	LOGD("found\n");
											bFound = 1;
										}
										break;
									 }
									 pRegInfo = pRegInfo->pNext;


							}
							if(!bFound)
							{
								// delete
								cJSON_Struct *pInParams = NULL,*pOutParams = NULL;

								sprintf(szTmp,"/%s/StreamQueue/Ress/%d/%d",pModuleMgr->pszModuleName,pCurrUser->nOwnerStreamQueueId,pCurrUser->nStreamQueueId);

								  pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
								  if(pInParams != NULL)
								  {

									 Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
									 Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,szTmp,0,0);

									 Module_StreamQueue_Require_Filter_Delete(pModuleMgr,pInParams,NULL);
									 if (pInParams != NULL)
									 {
										 Common_Json_Delete(pInParams);
										 pInParams = NULL;
									 }
								  }
							}
						}
					}
				}
			}

	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
}

static S32 staticStreamQueue_ReConnect_Thread(Common_Thread_T hThreadHandle,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)pUserData;
	LibModuleStreamQueueStreamRess_T *pReConnectRessHead = NULL;
	LibModuleStreamQueueStreamRess_T *pDeleteRess = NULL,*pStreamRess = NULL;
	S8 *pToModuleName = NULL;
	cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	S8 szTmp[128];
	S32 nArrayIdx = 0;
	S8 *pStringValue = NULL,*pRess = NULL;
	cJSON_Struct *pOutStreamInfo = NULL;
	cJSON_Struct *pStreamList = NULL;
	LibModuleStreamQueueStreamMultiRess_T *pMultiRess = NULL;
	S32 nMultiRessCount = 0;
	S32 nCode = -1,nRet;
	S32 bError = 0,bNeedRemoteDelete = 0;
	LibModuleStreamQueueUser_T *pUser = NULL;

	while(!pModuleMgr->tStreamQueueInfo.bReConnectThreadExit)
	{
		//try
		Common_InterSleep_Sleep(pModuleMgr->tStreamQueueInfo.hInterSleep,5,0);
		Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);

		if(pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList == NULL &&
		   pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList == NULL)
		{
			//pModuleMgr->tStreamQueueInfo.hThread_ReConnect = NULL;
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			continue;
		}
			pDeleteRess = pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList;

//			printf("[%s.%d]list :%p DE\n",__FUNCTION__,__LINE__,pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList);

		while(pDeleteRess != NULL)
		{
			pStreamRess = pDeleteRess;
//			printf("[%s.%d]del pStreamRess :%p :prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pStreamRess,pStreamRess->pPrev,pStreamRess->pNext,pStreamRess->pPrev_De,pStreamRess->pNext_De,pStreamRess->pPrev_Re,pStreamRess->pNext_Re);
			pDeleteRess = pDeleteRess->pNext_De;
			if(pStreamRess->nUseCount > 0)
			{
//				printf("[%s.%d]\n",__FUNCTION__,__LINE__);
				continue;
			}
//			printf("[%s.%d]del pStreamRess :%p :prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pStreamRess,pStreamRess->pPrev,pStreamRess->pNext,pStreamRess->pPrev_De,pStreamRess->pNext_De,pStreamRess->pPrev_Re,pStreamRess->pNext_Re);

			// out
			if (pStreamRess->pPrev_De == NULL)
			{

				pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList = pStreamRess->pNext_De;
				if (pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList != NULL)
				{
					pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList->pPrev_De = NULL;
				}


			}
			else if (pStreamRess->pNext_De == NULL)
			{
				pStreamRess->pPrev_De->pNext_De = NULL;
			}
			else
			{
				pStreamRess->pPrev_De->pNext_De = pStreamRess->pNext_De;
				pStreamRess->pNext_De->pPrev_De = pStreamRess->pPrev_De;
			}
			pStreamRess->pNext_De = NULL;
			pStreamRess->pPrev_De = NULL;
			//
			//printf("[%s.%d] delete <%p> <%s> <%s>\n",__FUNCTION__,__LINE__,pStreamRess,pStreamRess->szResUri ?pStreamRess->szResUri:"<null>",pStreamRess->szReceiveUri?pStreamRess->szReceiveUri:"<null>");
			 pToModuleName = NULL;
			 Common_UriOneParse(pStreamRess->szUri,NULL,&pToModuleName,NULL);
			 if (pToModuleName != NULL)
			 {
				 sprintf(szTmp,"/%s/StreamQueue/Delete",pToModuleName);

				  pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				  if(pInParams != NULL)
				  {

					 Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					 Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
					 Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,pStreamRess->szResUri,0,0);
					 Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);

					 // Common_Json_StandardPrint(pInParams,">>>> <",">",NULL);
					 nRet = Module_CallFunctions(pModuleMgr->hModuleHandle,pInParams,NULL,3000);
					  Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
					 if (pInParams != NULL)
					 {
						 Common_Json_Delete(pInParams);
						 pInParams = NULL;
					 }
				  }
				Common_Free(pToModuleName,__FUNCTION__,__LINE__);
			 	pToModuleName = NULL;
			  }

			 //printf("[%s.%d]nRet = %d \n",__FUNCTION__,__LINE__,nRet);
			 if (pStreamRess->pStreamRessArray != NULL)
			 {
				 for (nArrayIdx = 0;nArrayIdx < pStreamRess->nStreamRessArrayCount;nArrayIdx++)
				 {
						 Common_Free(pStreamRess->pStreamRessArray[nArrayIdx].szStreamRess,__FUNCTION__,__LINE__);
						 pStreamRess->pStreamRessArray[nArrayIdx].szStreamRess = NULL;




						 int nShmId = pStreamRess->pStreamRessArray[nArrayIdx].nShmId;
							pStreamRess->pStreamRessArray[nArrayIdx].nShmId = -1;

							Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
							if(nShmId > 0)
							{
								StreamQueue_Close(nShmId);

							}
							//LOGD("wait enter\n");long long test=Common_GetSystemCount64();
							Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
							//LOGD("enter %lld\n",Common_GetSystemCount64() - test);

				 }
				 Common_Free(pStreamRess->pStreamRessArray,__FUNCTION__,__LINE__);
				 pStreamRess->pStreamRessArray = NULL;
				 pStreamRess->nStreamRessArrayCount = 0;
			 }


			 Common_Free(pStreamRess->szUri,__FUNCTION__,__LINE__);
			 pStreamRess->szUri = NULL;
			 Common_Free(pStreamRess->szResUri,__FUNCTION__,__LINE__);
			 pStreamRess->szResUri= NULL;
			 Common_Free(pStreamRess->szReceiveUri,__FUNCTION__,__LINE__);
			 pStreamRess->szReceiveUri = NULL;
			 if(pStreamRess->pOpenParams != NULL)
			 {
			 	 Common_Json_Delete(pStreamRess->pOpenParams);
				 pStreamRess->pOpenParams = NULL;
			 }
			 //printf("Free <%p> %p %p %p %p\n",pStreamRess,pStreamRess->pNext_De,pStreamRess->pPrev_De,pStreamRess->pPrev_Re,pStreamRess->pNext_Re);
			   Common_Free(pStreamRess,__FUNCTION__,__LINE__);
			 pStreamRess = NULL;

			 if (pInParams != NULL)
			 {
				 Common_Json_Delete(pInParams);
				 pInParams = NULL;
			 }




		}


		pReConnectRessHead = pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList;
		while(pReConnectRessHead != NULL)
		{
			nCode = -1;
			bError = 0;
			bNeedRemoteDelete = 0;
			pStreamRess = pReConnectRessHead;
//			printf("[%s.%d]recon pStreamRess :%p :prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pStreamRess,pStreamRess->pPrev,pStreamRess->pNext,pStreamRess->pPrev_De,pStreamRess->pNext_De,pStreamRess->pPrev_Re,pStreamRess->pNext_Re);

			//printf("reconn = %p",pStreamRess);
			pReConnectRessHead = pReConnectRessHead->pNext_Re;
			//printf("[%s.%d] reconn<%p> <%s> <%s> <%s>\n",__FUNCTION__,__LINE__,pStreamRess,pStreamRess->szUri?pStreamRess->szUri:"null",pStreamRess->szResUri?pStreamRess->szResUri:"<null>",pStreamRess->szReceiveUri?pStreamRess->szReceiveUri:"<null>");
			pUser = (LibModuleStreamQueueUser_T *)pStreamRess->pOwner;
			if(pUser->bNeedDelete)
			{
				continue;
			}
			pUser->nUseCount++;
			// ready
			if(pStreamRess->pStreamRessArray != NULL)
			{
//				printf("[%s.%d] delete first\n",__FUNCTION__,__LINE__);
				pMultiRess = pStreamRess->pStreamRessArray;
				for(nArrayIdx = 0; nArrayIdx < nMultiRessCount;nArrayIdx++)
				{

					if(pStreamRess->nMode == 0)
					{
							STREAM_QUEUE_EPOLL_EVENT_T tEvent;
							memset(&tEvent,0,sizeof(tEvent));
							tEvent.streamQueueHandle = pMultiRess[nArrayIdx].nShmId;
							tEvent.event = pStreamRess->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
							tEvent.userData = (void *)((pStreamRess->nIndex << 16) | (nArrayIdx));

							StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_RM,&tEvent);

					}

					int nShmId = pMultiRess[nArrayIdx].nShmId;
					pMultiRess[nArrayIdx].nShmId = -1;

					Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
					if(nShmId > 0)
					{
						StreamQueue_Close(nShmId);

					}
					Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
				}

				Common_Free(pStreamRess->pStreamRessArray,__FUNCTION__,__LINE__);
				pStreamRess->pStreamRessArray= NULL;
				pStreamRess->nStreamRessArrayCount = 0;

			}

			  Common_Free(pStreamRess->szResUri,__FUNCTION__,__LINE__);
			  pStreamRess->szResUri = NULL;
			//do


			if (pToModuleName != NULL)
			 {
				 Common_Free(pToModuleName,__FUNCTION__,__LINE__);
				 pToModuleName = NULL;
			 }

			 Common_UriOneParse(pStreamRess->szUri,NULL,&pToModuleName,NULL);
			 if (pToModuleName == NULL)
			 {
				pUser->nUseCount--;
				 continue;
			 }
			 if (pInParams != NULL)
			 {
				 Common_Json_Delete(pInParams);
				 pInParams = NULL;
			 }
			 sprintf(szTmp,"/%s/StreamQueue/Post",pToModuleName);
			  Common_Free(pToModuleName,__FUNCTION__,__LINE__);
			  pToModuleName = NULL;
			  pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			  if (pInParams == NULL)
			  {
			  	if (pToModuleName != NULL)
				 {
					 Common_Free(pToModuleName,__FUNCTION__,__LINE__);
					 pToModuleName = NULL;
				 }
				pUser->nUseCount--;
				  continue;
			  }


			  Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			  Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
			  Common_Json_SetAttrValue(pInParams,-1,"Header/PostUri",Common_Json_Type_String,pStreamRess->szUri,0,0);

			  Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,pStreamRess->szReceiveUri,0,0);
			  if (pStreamRess->pOpenParams != NULL)
			  {
			  	cJSON_Struct *pData = Common_Json_Duplicate(pStreamRess->pOpenParams,1);
				if(pData != NULL)
				{
					 Common_Json_AddItem(pInParams,-1,"Data",pData);
				}
			  }

			  Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			  if (pOutParams != NULL)
			  {
				   Common_Json_Delete(pOutParams);
				   pOutParams = NULL;
			  }
			  //printf("[%s.%d]pModuleMgr = %p %p\n",__FUNCTION__,__LINE__,pModuleMgr,pModuleMgr->hModuleHandle);
			  nRet = Module_CallFunctions(pModuleMgr->hModuleHandle,pInParams,&pOutParams,3000);

			   Common_Json_Delete(pInParams);
				 pInParams = NULL;
				 Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);

			  if (pOutParams != NULL)
			  {
				  Common_Json_GetAttrValue(pOutParams,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
				  pRess = NULL;
				  Common_Json_GetAttrValue(pOutParams,-1,"/Header/ResUri",NULL,&pRess,NULL,NULL);
				  pStreamList = Common_Json_GetItem(pOutParams,-1,"/Header/StreamsList");

			  }
			  else
			  {
			  	bError = 1;
			  }

			  if (pRess == NULL)
			  {
				 Common_Json_Delete(pOutParams);
				 pOutParams = NULL;
				 if (pToModuleName != NULL)
				 {
					 Common_Free(pToModuleName,__FUNCTION__,__LINE__);
					 pToModuleName = NULL;
				 }
				 pUser->nUseCount--;
				 continue;
			  }
			  bNeedRemoteDelete = 1;
			  nMultiRessCount = Common_Json_ArraySize(pStreamList);
			  if (pStreamList == NULL || nMultiRessCount == 0)
			  {
				 bError = 1;
			  }
			 if(!bError)
			 {
				  pStreamRess->szResUri = Common_StrDup(pRess,__FUNCTION__,__LINE__);
				  if (pStreamRess->szResUri == NULL)
				  {
					  bError = 1;
				  }
			 }
			 if(!bError)
			 {
				   pMultiRess = (LibModuleStreamQueueStreamMultiRess_T *)Common_Malloc(sizeof(LibModuleStreamQueueStreamMultiRess_T) * nMultiRessCount,0,__FUNCTION__,__LINE__);
				  if(pMultiRess == NULL)
				  {
				  	  bError = 1;
				  }
			 }


			  if (pUser->nEpollReadHandle <= 0)
			  {
				  pUser->nEpollReadHandle = StreamQueue_EpollCreate();
			  }
			  // 打开共享内存



			  if(!bError)
			  {

				    memset(pMultiRess,0,sizeof(LibModuleStreamQueueStreamMultiRess_T) * nMultiRessCount);

					for (nArrayIdx = 0; nArrayIdx < nMultiRessCount;nArrayIdx++)
					{
						pStringValue = NULL;
						Common_Json_GetAttrValue(pStreamList,nArrayIdx,NULL,NULL,&pStringValue,NULL,NULL);
						pMultiRess[nArrayIdx].szStreamRess = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
						if (pMultiRess[nArrayIdx].szStreamRess != NULL)
						{
							pMultiRess[nArrayIdx].nShmId = StreamQueue_Open(pMultiRess[nArrayIdx].szStreamRess,pStreamRess->nMode?STREAM_QUEUE_OPEN_FLAG_WRITE:STREAM_QUEUE_OPEN_FLAG_READ,0,0);
							if (pMultiRess[nArrayIdx].nShmId <= 0)
							{
								 bError = 1;
								 break;
							}
							else if(pStreamRess->nMode == 0)
							{
								STREAM_QUEUE_EPOLL_EVENT_T tEvent;
								memset(&tEvent,0,sizeof(tEvent));
								tEvent.streamQueueHandle = pMultiRess[nArrayIdx].nShmId;
								tEvent.event = pStreamRess->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
								tEvent.userData = (void *)((pStreamRess->nIndex << 16) | (nArrayIdx));

								StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_ADD,&tEvent);
							}
						}

					}
			  	}

				if(bError)
				{
					S32 nErIdx;
					if(bNeedRemoteDelete)
					{
						if(pInParams != NULL)
						{
							Common_Json_Delete(pInParams);
							pInParams = NULL;
						}
						pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
					  if (pInParams != NULL)
					  {
					 	 sprintf(szTmp,"/%s/StreamQueue/Delete",pToModuleName);

						 Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
						 Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
						 Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,pStreamRess->szResUri,0,0);
						 Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
						 Module_CallFunctions(pModuleMgr->hModuleHandle,pInParams,NULL,3000);
						 Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
						 if(pInParams != NULL)
						{
							Common_Json_Delete(pInParams);
							pInParams = NULL;
						}
					  }

					}
					for(nErIdx = 0; nErIdx <= nArrayIdx && nErIdx < nMultiRessCount;nErIdx++)
					{

						if(pStreamRess->nMode == 0)
						{
								STREAM_QUEUE_EPOLL_EVENT_T tEvent;
								memset(&tEvent,0,sizeof(tEvent));
								tEvent.streamQueueHandle = pMultiRess[nErIdx].nShmId;
								tEvent.event = pStreamRess->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
								tEvent.userData = (void *)((pStreamRess->nIndex << 16) | (nErIdx));

								StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_RM,&tEvent);
						}
						int nShmId = pMultiRess[nErIdx].nShmId;
						pMultiRess[nErIdx].nShmId = -1;

						Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
						if(nShmId > 0)
						{
							StreamQueue_Close(nShmId);

						}
						Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
					}

					  Common_Free(pStreamRess->szResUri,__FUNCTION__,__LINE__);
					  pStreamRess->szResUri = NULL;
					  Common_Free(pStreamRess->pStreamRessArray,__FUNCTION__,__LINE__);
				      pStreamRess->pStreamRessArray= NULL;
				}
				else
				{

					  pStreamRess->pStreamRessArray = pMultiRess;
					  pStreamRess->nStreamRessArrayCount = nMultiRessCount;
					  pStreamRess->bKicked = 0;
					  if (pStreamRess->pNext_Re != NULL ||
						  pStreamRess->pPrev_Re != NULL ||
						  pStreamRess == pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList)
					  {

						  // out
						  if (pStreamRess->pPrev_Re == NULL)
						 {

							 pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList = pStreamRess->pNext_Re;
							 if (pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList != NULL)
							 {
								 pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList->pPrev_Re = NULL;
							 }


						 }
						 else if (pStreamRess->pNext_Re == NULL)
						 {
							 pStreamRess->pPrev_Re->pNext_Re = NULL;
						 }
						 else
						 {
							pStreamRess->pPrev_Re->pNext_Re = pStreamRess->pNext_Re;
							pStreamRess->pNext_Re->pPrev_Re = pStreamRess->pPrev_Re;
						 }
						 pStreamRess->pNext_Re = NULL;
						 pStreamRess->pPrev_Re = NULL;
					  }
				}

				//Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);


				// free

			   if(pInParams != NULL)
				{
					Common_Json_Delete(pInParams);
					pInParams = NULL;
				}
			   if(pOutParams != NULL)
			   {
			    Common_Json_Delete(pOutParams);
				pOutParams = NULL;
			   }
			   if (pToModuleName != NULL)
	 			 {
	 				 Common_Free(pToModuleName,__FUNCTION__,__LINE__);
	 				 pToModuleName = NULL;
	 			 }
			   pUser->nUseCount--;

		}
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	}

	return 0;
}

static void staticStreamQueue_CheckThread(LibModuleInfo_T *pModuleMgr,LibModuleStreamQueueStreamRess_T *pStreamRess_De,LibModuleStreamQueueStreamRess_T *pStreamRess_Re)
{
	S32 bNeedThread = 0;
// 	if (pStreamRess_De != NULL)
// 	{
// 		printf("[%s.%d]list :%p DE %p:prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList,pStreamRess_De,pStreamRess_De->pPrev,pStreamRess_De->pNext,pStreamRess_De->pPrev_De,pStreamRess_De->pNext_De,pStreamRess_De->pPrev_Re,pStreamRess_De->pNext_Re);
// 	}
// 	if (pStreamRess_Re != NULL)
// 	{
// 		printf("[%s.%d]list:%p RE %p:prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList,pStreamRess_Re,pStreamRess_Re->pPrev,pStreamRess_Re->pNext,pStreamRess_Re->pPrev_De,pStreamRess_Re->pNext_De,pStreamRess_Re->pPrev_Re,pStreamRess_Re->pNext_Re);
// 	}
//
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);

	if(pStreamRess_De != NULL )
	{
		if(pStreamRess_De->pNext_De != NULL || pStreamRess_De->pPrev_De != NULL || pStreamRess_De == pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList)
		{// 已经在删除 表里
		//	 do nothing
		}
		else
		{// add
			pStreamRess_De->pNext_De = pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList;
			if(pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList != NULL)
			{
				pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList->pPrev_De = pStreamRess_De;
			}
			pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList = pStreamRess_De;
			pStreamRess_De->bNeedDelete = 1;
		}
		if(pStreamRess_De->pNext_Re != NULL || pStreamRess_De->pPrev_Re != NULL || pStreamRess_De == pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList)
		{
			//printf("in re!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!<%p>\n",pStreamRess_De);
			// remove from reconnect list
			 if (pStreamRess_De->pPrev_Re == NULL)
			 {

				 pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList = pStreamRess_De->pNext_Re;
				 if (pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList != NULL)
				 {
					 pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList->pPrev_Re = NULL;
				 }


			 }
			 else if (pStreamRess_De->pNext_Re == NULL)
			 {
				 pStreamRess_De->pPrev_Re->pNext_Re = NULL;
			 }
			 else
			 {
				pStreamRess_De->pPrev_Re->pNext_Re = pStreamRess_De->pNext_Re;
				pStreamRess_De->pNext_Re->pPrev_Re = pStreamRess_De->pPrev_Re;
			 }
			 pStreamRess_De->pNext_Re = NULL;
			 pStreamRess_De->pPrev_Re = NULL;
		}
//		printf("[%s.%d]ok DE %p:prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pStreamRess_De,pStreamRess_De->pPrev,pStreamRess_De->pNext,pStreamRess_De->pPrev_De,pStreamRess_De->pNext_De,pStreamRess_De->pPrev_Re,pStreamRess_De->pNext_Re);

	}
	if(pStreamRess_Re != NULL)
	{
		if(pStreamRess_Re->pNext_De != NULL || pStreamRess_Re->pPrev_De != NULL || pStreamRess_Re == pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList)
		{// 竟然在删除 表里。。。。
			// do nothing
//			printf("[%s.%d]in de!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!<%p>  %p %p Re %p %p\n",__FUNCTION__,__LINE__,pStreamRess_Re,pStreamRess_Re->pPrev_Re,pStreamRess_Re->pNext_Re,pStreamRess_Re->pPrev_De,pStreamRess_Re->pNext_De);
		}
		else if(pStreamRess_Re->pNext_Re != NULL || pStreamRess_Re->pPrev_Re != NULL || pStreamRess_Re == pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList)
		{// 已经在重连表里
			// do nothing
		}
		else
		{
			// add
			pStreamRess_Re->pNext_Re = pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList;
			if(pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList != NULL)
			{
				pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList->pPrev_Re = pStreamRess_Re;
			}
			pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList = pStreamRess_Re;
		}
//		printf("[%s.%d]ok RE %p:prev=%p next=%p,dp = %p dn=%p,rp=%p rn=%p\n",__FUNCTION__,__LINE__,pStreamRess_Re,pStreamRess_Re->pPrev,pStreamRess_Re->pNext,pStreamRess_Re->pPrev_De,pStreamRess_Re->pNext_De,pStreamRess_Re->pPrev_Re,pStreamRess_Re->pNext_Re);


	}

	if(pModuleMgr->tStreamQueueInfo.hThread_ReConnect == NULL)
	{



		bNeedThread = pModuleMgr->tStreamQueueInfo.pStreamRessDeleteList != NULL || pModuleMgr->tStreamQueueInfo.pStreamRessReConnectList != NULL;
		if(bNeedThread)
		{
			if(pModuleMgr->tStreamQueueInfo.hInterSleep == NULL)
			{
				 Common_InterSleep_Create(&pModuleMgr->tStreamQueueInfo.hInterSleep);
			}

			Common_Thread_Create(&pModuleMgr->tStreamQueueInfo.hThread_ReConnect,"StreadQueue_checkRecon",0,0,staticStreamQueue_ReConnect_Thread,pModuleMgr);
		}


	}

	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	Common_InterSleep_WakeUp(pModuleMgr->tStreamQueueInfo.hInterSleep);

}

S32 Module_StreamQueue_Init(ModuleHandle_T hModuleHandle)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char sztmp[128];
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	sprintf(sztmp,"%s_Module_StreamQueue_lock",pModuleMgr->pszModuleName);
	Common_Lock_Create(&pModuleMgr->tStreamQueueInfo.hLock,sztmp);
	static_StreamQueue_loadCfg(hModuleHandle);
	return 0;
}
S32 Module_StreamQueue_SetGlobalCallback(ModuleHandle_T hModuleHandle,Module_StreamQueue_Callbacks *pFxns,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (pFxns == NULL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=2;

	pModuleMgr->tStreamQueueInfo.tGlobalFxn = *pFxns;
	pModuleMgr->tStreamQueueInfo.pUserData = pUserData;
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	return 0;
}
S32 Module_StreamQueue_Create(ModuleHandle_T hModuleHandle,char *szUri,cJSON_Struct *pCreateParams,cJSON_Struct **pOutParams,Module_StreamQueue_Callbacks *pFxns,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueOwner_T *pOwner = NULL;
	S32 nCount = 0,nIdx,nFreeIdx;
	S32 nStreamQueueId = -1;
	S32 bNeedFound = 0,nFoundStart = 0;
	S8 szTmp[128];
	S32 nMaxMemSize = 1024 * 1024,nMaxNodeNum=25,nQueueType = 0,nMode = 1;
	S32 nShmId;
	if (pCreateParams == NULL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=3;
	nFreeIdx = -1;
	nCount = 0;
	if (szUri != NULL)
	{
		LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
		pOwner = pModuleMgr->tStreamQueueInfo.pOwnerListHead;
		while(pOwner != NULL)
		{
			if (pOwner->szUri != NULL &&
				0 == Common_StriCmp(pOwner->szUri,szUri))
			{
				// 已经有了，则返回失败
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				return MODULE_ERROR_TYPE_RESUBMIT;
			}
			pOwner = pOwner->pNext;
		}

		pCoOwner = pModuleMgr->tStreamQueueInfo.pCoOwnerListHead;
		while(pCoOwner != NULL)
		{
			if (pCoOwner->szUri != NULL &&
				0 == Common_StriCmp(pCoOwner->szUri,szUri))
			{
				// 已经有了，则返回失败
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				return MODULE_ERROR_TYPE_RESUBMIT;
			}
			pCoOwner = pCoOwner->pNext;
		}

	}
	if (pModuleMgr->tStreamQueueInfo.nOwnerCount >= LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	else if (pModuleMgr->tStreamQueueInfo.nInfoPoolCount >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	nFreeIdx = -1;
	if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx != -1 ||
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		if(NULL != pModuleMgr->tStreamQueueInfo.pInfoPool[pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx])
		{
			bNeedFound = 1;
			nFoundStart = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx + 1;
			if (nFoundStart >= LIBMODULE_STREAMQUEUE_MAX_POOL)
			{
				nFoundStart = 0;
			}
		}
	}
	else
	{
		bNeedFound = 1;
		nFoundStart = 0;
	}

	if (bNeedFound)
	{
		for(nIdx = nFoundStart;nIdx < LIBMODULE_STREAMQUEUE_MAX_POOL;nIdx++)
		{
			if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
			{//找到
				nFreeIdx = nIdx;
				break;
			}
		}
		if (nFreeIdx == -1)
		{// 不可能到达的
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return MODULE_ERROR_TYPE_INTERNALERROR;
		}
	}
	else
	{
		 nFreeIdx = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx;
	}

	pOwner = (LibModuleStreamQueueOwner_T *)Common_Malloc(sizeof(LibModuleStreamQueueOwner_T),0,__FUNCTION__,__LINE__);
	if (pOwner == NULL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	memset(pOwner,0,sizeof(LibModuleStreamQueueOwner_T));
	if (szUri != NULL)
	{
		pOwner->szUri = Common_StrDup(szUri,__FUNCTION__,__LINE__);
		if (pOwner->szUri == NULL)
		{
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			Common_Free(pOwner,__FUNCTION__,__LINE__);
			return MODULE_ERROR_TYPE_LIMITED;
		}
	}
	pModuleMgr->tStreamQueueInfo.nStaticCount++;
	if (pModuleMgr->tStreamQueueInfo.nStaticCount >= 0x7FFF)
	{
		pModuleMgr->tStreamQueueInfo.nStaticCount = 1;
	}
	nStreamQueueId = (pModuleMgr->tStreamQueueInfo.nStaticCount << 16) | (0 << 14) | nFreeIdx;
	pOwner->nStreamQueueId = nStreamQueueId;
	if(pFxns != NULL)
	{
		 pOwner->tFxn = *pFxns;
		 pOwner->pUserData = pUserData;
	}
	sprintf(szTmp,"%s_StreamQueue_%d",pModuleMgr->pszModuleName,nStreamQueueId);
	pOwner->szStreamRess = Common_StrDup(szTmp,__FUNCTION__,__LINE__);
	if (pOwner->szStreamRess == NULL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		Common_Free(pOwner->szUri,__FUNCTION__,__LINE__);
		Common_Free(pOwner,__FUNCTION__,__LINE__);
		return MODULE_ERROR_TYPE_LIMITED;
	}

	// 打开共享缓冲
	S32 nIntValue = 0,nFlag = 0,bEliminated = 0;
	Common_Json_GetAttrValue(pCreateParams,-1,"QueueType",NULL,NULL,&nIntValue,NULL);
	if (nIntValue > 0)
	{
		nQueueType = nIntValue;
	}
	nMode = 1;
	Common_Json_GetAttrValue(pCreateParams,-1,"Mode",NULL,NULL,&nIntValue,NULL);
	if (nIntValue >= 0)
	{

		if (nIntValue == 0)
		{
			nMode = 0;
		}
		else if (nIntValue == 1)
		{
			nMode = 1;
		}

	}
	Common_Json_GetAttrValue(pCreateParams,-1,"MaxMemSize",NULL,NULL,&nIntValue,NULL);
	if (nIntValue > 0)
	{
		if (nIntValue < 1024)
		{
			nIntValue = 1024;
		}
		nMaxMemSize = nIntValue;

	}
	Common_Json_GetAttrValue(pCreateParams,-1,"MaxMemNum",NULL,NULL,&nIntValue,NULL);
	if (nIntValue > 0)
	{
		nMaxNodeNum = nIntValue;
	}
	Common_Json_GetAttrValue(pCreateParams,-1,"HasEliminated",NULL,NULL,&nIntValue,NULL);
	if (nIntValue > 0)
	{
		bEliminated = 1;
	}
	if (nMode == 0)
	{
		nFlag |= STREAM_QUEUE_OPEN_FLAG_READ | STREAM_QUEUE_OPEN_FLAG_CREATE;
	}
	if(nMode == 1)
	{
		nFlag |= STREAM_QUEUE_OPEN_FLAG_WRITE | STREAM_QUEUE_OPEN_FLAG_CREATE;
	}
	if (nQueueType == 1)
	{
		nFlag |= STREAM_QUEUE_CREATE_FLAG_ELIMINATED_SMALL;
	}
	if (bEliminated)
	{

	}

	nShmId = StreamQueue_Open(szTmp,nFlag,nMaxMemSize,nMaxNodeNum);
	if (nShmId <= 0)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		Common_Free(pOwner->szUri,__FUNCTION__,__LINE__);
		Common_Free(pOwner,__FUNCTION__,__LINE__);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	pOwner->nShmId = nShmId;
	pOwner->nMode = nMode;
	//
	// 失败，释放资源 返回错误
	//打开成功，开始占有资源

	pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] = pOwner;
	pModuleMgr->tStreamQueueInfo.nInfoPoolCount++;
	pOwner->pNext = pModuleMgr->tStreamQueueInfo.pOwnerListHead;
	if (pModuleMgr->tStreamQueueInfo.pOwnerListHead != NULL)
	{
		pModuleMgr->tStreamQueueInfo.pOwnerListHead->pPrev = pOwner;
	}
	pModuleMgr->tStreamQueueInfo.pOwnerListHead = pOwner;
	pModuleMgr->tStreamQueueInfo.nOwnerCount++;
	// 下一个可用的可能位置
	pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nFreeIdx + 1;

	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	static_StreamQueue_SaveCfg(hModuleHandle);
	return nStreamQueueId;
}
/*
pCreateParams = {StreamType=,MemSize=,FrameMaxNum=,Mode=}
StreamType:0-av
Mode:0-write,1-read
pOutParams={code=,Describe=}
*/
S32 Module_StreamQueue_CoCreate(ModuleHandle_T hModuleHandle,char *szCoUri,S32 *FdSets,U32 dwFdCount,Module_StreamQueue_Callbacks *pFxns,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
	LibModuleStreamQueueOwner_T *pOwner = NULL;
	S32 nCount = 0,nIdx,nFreeIdx;
	S32 nStreamQueueId = -1;
	S32 bNeedFound = 0,nFoundStart = 0;
	if (FdSets == NULL || dwFdCount <= 0)
	{
		return -1;
	}

	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=4;
	nFreeIdx = -1;
	nCount = 0;
	if (szCoUri != NULL)
	{
		pCoOwner = pModuleMgr->tStreamQueueInfo.pCoOwnerListHead;
		while(pCoOwner != NULL)
		{
			if (pCoOwner->szUri != NULL &&
				0 == Common_StriCmp(pCoOwner->szUri,szCoUri))
			{
				// 已经有了，则返回失败
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				return MODULE_ERROR_TYPE_RESUBMIT;
			}
			pCoOwner = pCoOwner->pNext;
		}
		pOwner = pModuleMgr->tStreamQueueInfo.pOwnerListHead;
		while(pOwner != NULL)
		{
			if (pOwner->szUri != NULL &&
				0 == Common_StriCmp(pOwner->szUri,szCoUri))
			{
				// 已经有了，则返回失败
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				return MODULE_ERROR_TYPE_RESUBMIT;
			}
			pOwner = pOwner->pNext;
		}

	}
	if (pModuleMgr->tStreamQueueInfo.nCoOwnerCount >= LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	else if (pModuleMgr->tStreamQueueInfo.nInfoPoolCount >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	nFreeIdx = -1;
	if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx != -1 ||
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		if(NULL != pModuleMgr->tStreamQueueInfo.pInfoPool[pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx])
		{
			bNeedFound = 1;
			nFoundStart = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx + 1;
			if (nFoundStart >= LIBMODULE_STREAMQUEUE_MAX_POOL)
			{
				nFoundStart = 0;
			}
		}
	}
	else
	{
		bNeedFound = 1;
		nFoundStart = 0;
	}

	if (bNeedFound)
	{
		for(nIdx = nFoundStart;nIdx < LIBMODULE_STREAMQUEUE_MAX_POOL;nIdx++)
		{
			if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
			{//找到
				nFreeIdx = nIdx;
				break;
			}
		}
		if (nFreeIdx == -1)
		{// 不可能到达的
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return MODULE_ERROR_TYPE_INTERNALERROR;
		}
	}
	else
	{
		nFreeIdx = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx;
	}

	pCoOwner = (LibModuleStreamQueueCoOwner_T *)Common_Malloc(sizeof(LibModuleStreamQueueCoOwner_T),0,__FUNCTION__,__LINE__);
	if (pCoOwner == NULL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return MODULE_ERROR_TYPE_LIMITED;
	}
	memset(pCoOwner,0,sizeof(LibModuleStreamQueueCoOwner_T));
	if (szCoUri != NULL)
	{
		pCoOwner->szUri = Common_StrDup(szCoUri,__FUNCTION__,__LINE__);
		if (pCoOwner->szUri == NULL)
		{
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			Common_Free(pCoOwner,__FUNCTION__,__LINE__);
			return MODULE_ERROR_TYPE_LIMITED;
		}
	}
	pModuleMgr->tStreamQueueInfo.nStaticCount++;
	if (pModuleMgr->tStreamQueueInfo.nStaticCount >= 0x7FFF)
	{
		pModuleMgr->tStreamQueueInfo.nStaticCount = 1;
	}
	nStreamQueueId = (pModuleMgr->tStreamQueueInfo.nStaticCount << 16) | (1 << 14) | nFreeIdx;
	pCoOwner->nStreamQueueId = nStreamQueueId;
	// 打开共享缓冲
	//
	//打开成功，开始占有资源
	S32 nFdIdx,bError = 0,nOkIdx = 0;
	S32 nOwnerIdx,nOwnerId,nCoIdx;

	for (nFdIdx = 0; nFdIdx < dwFdCount;nFdIdx++)
	{
		nOwnerId = FdSets[nFdIdx];
		if (!((nOwnerId >> 14 ) & 3))
		{
			bError = 1;
			break;
		}
		nOwnerIdx = nOwnerId & 0x3FFF;
		if (nOwnerIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
		{
			bError = 1;
			break;
		}
		if(pModuleMgr->tStreamQueueInfo.pInfoPool[nOwnerIdx] == NULL)
		{
			bError = 1;
			break;
		}
		pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwnerIdx];
		if (pOwner->nStreamQueueId != nOwnerId)
		{
			bError = 1;
			break;
		}
		if (pOwner->nCoOwnerCount >= LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM)
		{
			bError = 1;
			break;
		}
		for (nCoIdx = 0; nCoIdx < LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM;nCoIdx++)
		{
			if (pOwner->pCoOwnerIdList[nCoIdx] == 0)
			{
				pOwner->pCoOwnerIdList[nCoIdx] = nStreamQueueId;
				pOwner->nCoOwnerCount++;
				break;
			}
		}
		nOkIdx = nFdIdx;
		pCoOwner->pOwerIdList[nFdIdx] = nOwnerId;
		pCoOwner->nOwnerCount++;
	}
	if (bError)
	{// 回滚
		for (nFdIdx = 0; nFdIdx <= nOkIdx ;nFdIdx++)
		{
			nOwnerId = FdSets[nFdIdx];
			nOwnerIdx = nOwnerId & 0x3FFF;
			for (nCoIdx = 0; nCoIdx < LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM;nCoIdx++)
			{
				if (pOwner->pCoOwnerIdList[nCoIdx] == nStreamQueueId)
				{
					pOwner->pCoOwnerIdList[nCoIdx] = 0;
					pOwner->nCoOwnerCount--;
					break;
				}
			}
		}
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		Common_Free(pCoOwner->szUri,__FUNCTION__,__LINE__);
		Common_Free(pCoOwner,__FUNCTION__,__LINE__);
		return -1;
	}

	if(pFxns != NULL)
	{
		pCoOwner->tFxn = *pFxns;
		pCoOwner->pUserData = pUserData;
	}
	pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] = pCoOwner;
	pModuleMgr->tStreamQueueInfo.nInfoPoolCount++;
	pCoOwner->pNext = pModuleMgr->tStreamQueueInfo.pCoOwnerListHead;
	if (pModuleMgr->tStreamQueueInfo.pCoOwnerListHead != NULL)
	{
		pModuleMgr->tStreamQueueInfo.pCoOwnerListHead->pPrev = pCoOwner;
	}
	pModuleMgr->tStreamQueueInfo.pCoOwnerListHead = pCoOwner;
	pModuleMgr->tStreamQueueInfo.nCoOwnerCount++;
	// 下一个可用的可能位置
	pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nFreeIdx + 1;

	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	static_StreamQueue_SaveCfg(hModuleHandle);
	return nStreamQueueId;
}
// 联合创建，可以将几个流合成一个新流，定义一个新名字，产生一个新的句柄。

S32 Module_StreamQueue_Destroy(ModuleHandle_T hModuleHandle,S32 fd)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueOwner_T *pOwner = NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
	S32 nCount = 0,nTotalCount,nIdx,nType,nUserIndex,nUserId,i;
	S32 nStreamQueueId = -1;
	nIdx = fd & 0x3FFF;
	nType = (fd >> 14) & 0x3;
	if (nType == 2 || nType == 3)
	{
		return -1;
	}
	if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=5;
	if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return -1;
	}
	pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
	if (pOwner->nStreamQueueId != fd)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return -1;
	}
	if (nType == 0)
	{// owner
		if (pOwner->nCoOwnerCount > 0)
		{//有组合创建
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return MODULE_ERROR_TYPE_BUSY;
		}
		if (pOwner->nUserCount > 0)
		{// 有使用者，需要处理
			nCount = 0;
			nTotalCount = pOwner->nUserCount;
			for (i = 0; i < LIBMODULE_STREAMQUEUE_MAX_USER_NUM && nCount < nTotalCount;i++)
			{
				if (pOwner->pUserIdList[i] != 0)
				{
					nUserId = pOwner->pUserIdList[i];
					nUserIndex = nUserId & 0x3FFF;
					if (pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] != NULL)
					{
						cJSON_Struct *pInParams = NULL;
						S8 *pToModuleName = NULL;
						S8 szTmp[128];
						LibModuleStreamQueueUser_T *pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex];
						// 处理,通知使用者
						pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
						if (pInParams != NULL)
						{
							pToModuleName = NULL;
							Common_UriOneParse(pUser->szReceiveUri,NULL,&pToModuleName,NULL);
							if (pToModuleName!= NULL)
							{
								sprintf(szTmp,"/%s/StreamQueue/Kick",pToModuleName);
								Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
								Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
								Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
								Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,pUser->szReceiveUri,0,0);
								 Module_CallFunctions(hModuleHandle,pInParams,NULL,0);

								Common_Free(pToModuleName,__FUNCTION__,__LINE__);

							}
							Common_Json_Delete(pInParams);
							pInParams = NULL;
						}
					    //删除

						Common_Free(pUser->szResUri,__FUNCTION__,__LINE__);
						Common_Free(pUser->szReceiveUri,__FUNCTION__,__LINE__);
						Common_Free(pUser,__FUNCTION__,__LINE__);
						pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] = NULL;
                        pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
						pOwner->pUserIdList[i] = 0;
						pOwner->nUserCount--;
					}

					nCount++;
				}
			}
		}
		if (pOwner->pPrev == NULL)
		{
			pModuleMgr->tStreamQueueInfo.pOwnerListHead = pOwner->pNext;
			if (pModuleMgr->tStreamQueueInfo.pOwnerListHead != NULL)
			{
				pModuleMgr->tStreamQueueInfo.pOwnerListHead->pPrev = NULL;
			}
		}
		else if (pOwner->pNext == NULL)
		{
			pOwner->pPrev->pNext = NULL;
		}
		else
		{
			pOwner->pPrev->pNext = pOwner->pNext;
			pOwner->pNext->pPrev = pOwner->pPrev;
		}
		pModuleMgr->tStreamQueueInfo.nOwnerCount--;

		//销毁
		int nShmId = pOwner->nShmId;
		pOwner->nShmId = -1;

		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		if (nShmId >= 0)
		{
			StreamQueue_Close(nShmId);
		}
		Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);

		if (pOwner->szStreamRess != NULL)
		{
			Common_Free(pOwner->szStreamRess,__FUNCTION__,__LINE__);
			pOwner->szStreamRess = NULL;

		}
		Common_Free(pOwner->szUri,__FUNCTION__,__LINE__);
		Common_Free(pOwner,__FUNCTION__,__LINE__);
		pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] = NULL;
        pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
	}
	else
	{
		pCoOwner = (LibModuleStreamQueueCoOwner_T *)pOwner;
		if (pCoOwner->nUserCount > 0)
		{// 有使用者，需要处理
			nCount = 0;
			nTotalCount = pCoOwner->nUserCount;
			for (i = 0; i < LIBMODULE_STREAMQUEUE_MAX_USER_NUM && nCount < nTotalCount;i++)
			{
				if (pCoOwner->pUserIdList[i] != 0)
				{
					nUserId = pCoOwner->pUserIdList[i];
					nUserIndex = nUserId & 0x3FFF;
					if (pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] != NULL)
					{
						cJSON_Struct *pInParams = NULL;
						S8 *pToModuleName = NULL;
						S8 szTmp[128];
						LibModuleStreamQueueUser_T *pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex];
						// 处理,通知使用者
						pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
						if (pInParams != NULL)
						{
							pToModuleName = NULL;
							Common_UriOneParse(pUser->szReceiveUri,NULL,&pToModuleName,NULL);
							if (pToModuleName!= NULL)
							{
								sprintf(szTmp,"/%s/StreamQueue/Kick",pToModuleName);
								Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
								Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
								Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
								Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,pUser->szReceiveUri,0,0);
								Module_CallFunctions(hModuleHandle,pInParams,NULL,0);

								Common_Free(pToModuleName,__FUNCTION__,__LINE__);

							}
							Common_Json_Delete(pInParams);
							pInParams = NULL;
						}
						//删除
						Common_Free(pUser->szResUri,__FUNCTION__,__LINE__);
						Common_Free(pUser->szReceiveUri,__FUNCTION__,__LINE__);
						Common_Free(pUser->szUri,__FUNCTION__,__LINE__);
						Common_Free(pUser,__FUNCTION__,__LINE__);
						pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] = NULL;
                        pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
						pCoOwner->pUserIdList[i] = 0;
						pCoOwner->nUserCount--;
					}

					nCount++;
				}
			}
		}
		if (pCoOwner->nOwnerCount > 0)
		{// 有使用者，需要处理
			nCount = 0;
			nTotalCount = pCoOwner->nOwnerCount;
			for (i = 0; i < LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM && nCount < nTotalCount;i++)
			{
				if (pCoOwner->pOwerIdList[i] > 0)
				{
					nUserId = pCoOwner->pOwerIdList[i];
					nUserIndex = nUserId & 0x3FFF;
					if (pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] != NULL)
					{
						cJSON_Struct *pInParams = NULL;
						S8 *pToModuleName = NULL;
						S8 szTmp[128];
						S32 nOweCount = 0,nOweTotalCount = pOwner->nCoOwnerCount;
						LibModuleStreamQueueOwner_T *pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex];
						// 处理,通知使用者
						for (S32 n = 0; n < LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM && nOweCount < nOweTotalCount;n++)
						{
							if (pOwner->pCoOwnerIdList[n] > 0)
							{
								nOweCount++;
							}
							if (pOwner->pCoOwnerIdList[n] == nUserId)
							{
								pOwner->pCoOwnerIdList[n] = 0;
								pOwner->nCoOwnerCount--;
								break;
							}
						}
					}
					pCoOwner->pOwerIdList[i] = 0;

					nCount++;
				}
			}
		}
		if (pCoOwner->pPrev == NULL)
		{
			pModuleMgr->tStreamQueueInfo.pCoOwnerListHead = pCoOwner->pNext;
			if (pModuleMgr->tStreamQueueInfo.pCoOwnerListHead != NULL)
			{
				pModuleMgr->tStreamQueueInfo.pCoOwnerListHead->pPrev = NULL;
			}
		}
		else if (pCoOwner->pNext == NULL)
		{
			pCoOwner->pPrev->pNext = NULL;
		}
		else
		{
			pCoOwner->pPrev->pNext = pCoOwner->pNext;
			pCoOwner->pNext->pPrev = pCoOwner->pPrev;
		}
		pModuleMgr->tStreamQueueInfo.nCoOwnerCount--;
		//销毁
		Common_Free(pCoOwner->szUri,__FUNCTION__,__LINE__);
		Common_Free(pCoOwner,__FUNCTION__,__LINE__);
		pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] = NULL;
        pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
	}
	// 计算下次可用索引
	if(pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx == -1 ||
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nIdx;
	}
	else if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= nIdx)
	{
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nIdx;
	}
	else if (pModuleMgr->tStreamQueueInfo.pInfoPool[pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx] != NULL)
	{
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nIdx;
	}
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	return 0;
}

S32 Module_StreamQueue_Open(ModuleHandle_T hModuleHandle,S8 *szUri,cJSON_Struct *pOpenParams,cJSON_Struct **pStreamInfo,S32 nMSecTimeout)
{
	CoOpen_Param_T tParam;
	S32 nStreamQueueId,nIntValue,nMode = 0;
	memset(&tParam,0,sizeof(tParam));
	tParam.szUri = szUri;
	tParam.pOpenParams = pOpenParams;
	tParam.nErrorCode = -1;
	nIntValue = 0;
	Common_Json_GetAttrValue(pOpenParams,-1,"Mode",NULL,NULL,&nIntValue,NULL);
	if (nIntValue >= 0)
	{
		if (nIntValue == 0)
		{
			nMode = 0;
		}
		else if (nIntValue == 1)
		{
			nMode = 1;
		}


	}
	tParam.nMode = nMode;
	nStreamQueueId = Module_StreamQueue_CoOpen(hModuleHandle,-1,&tParam,1,nMSecTimeout);
	if (nStreamQueueId > 0)
	{
		if (tParam.nErrorCode != 0)
		{
			Common_Json_Delete(tParam.pOutStreamInfo);
			tParam.pOutStreamInfo = NULL;
			return tParam.nErrorCode;
		}
	}
	if (pStreamInfo)
	{
		*pStreamInfo = tParam.pOutStreamInfo;
	}
	else
	{
		Common_Json_Delete(tParam.pOutStreamInfo);
		tParam.pOutStreamInfo = NULL;
	}
	return nStreamQueueId;
}
 S32 Module_StreamQueue_CoOpen(ModuleHandle_T hModuleHandle,S32 fd,CoOpen_Param_T *pSets,S32 nSetsNum,S32 nMSecTimeout)
 {
	 LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	 LibModuleStreamQueueUser_T *pUser = NULL,*pNewUser = NULL;
	 S32 nCount = 0,nIdx;
	 S32 nFreeIdx = 0;
	 S32 nStreamQueueId = -1;
	 S32 bNeedFound = 0,nFoundStart = 0;
	 cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	 S8 *pToModuleName = NULL;
	 S32 nCode = -1,nRet = -1;
	 int nSetIdx;
	 LibModuleStreamQueueStreamRess_T *pStreamRess = NULL;
	 if (pSets == NULL || nSetsNum <= 0)
	 {
		 return -1;
	 }

	 Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	 g_nTestQueue_status4=6;
	 do{


		 if (fd > 0)
		 {
			 int nType = (fd >> 14)&0x03;
			 nIdx = fd & 0x3FFF;
			 if (nType != 2 || nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL || nIdx >= LIBMODULE_STREAMQUEUE_MAX_USER_NUM)
			 {
				break;
			 }
			  pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
			  if (pUser == NULL || pUser->nStreamQueueId != fd)
			  {
				  break;
			  }
			  nStreamQueueId = fd;
			  nRet = 0;
		 }
		 else
		 {
			 nFreeIdx = -1;
			 nCount = 0;

			 if (pModuleMgr->tStreamQueueInfo.nUserCount >= LIBMODULE_STREAMQUEUE_MAX_USER_NUM ||
				 pModuleMgr->tStreamQueueInfo.nInfoPoolCount >= LIBMODULE_STREAMQUEUE_MAX_POOL)
			 {

				 nRet =  MODULE_ERROR_TYPE_LIMITED;
				 break;
			 }

			 nFreeIdx = -1;
			 if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx != -1 ||
				 pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
			 {
				 if(NULL != pModuleMgr->tStreamQueueInfo.pInfoPool[pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx])
				 {
					 bNeedFound = 1;
					 nFoundStart = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx + 1;
					 if (nFoundStart >= LIBMODULE_STREAMQUEUE_MAX_POOL)
					 {
						 nFoundStart = 0;
					 }
				 }
			 }
			 else
			 {
				 bNeedFound = 1;
				 nFoundStart = 0;
			 }

			 if (bNeedFound)
			 {
				 for(nIdx = nFoundStart;nIdx < LIBMODULE_STREAMQUEUE_MAX_POOL;nIdx++)
				 {
					 if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
					 {//找到
						 nFreeIdx = nIdx;
						 break;
					 }
				 }
				 if (nFreeIdx == -1)
				 {// 不可能到达的
					 nRet = MODULE_ERROR_TYPE_INTERNALERROR;
					 break;
				 }
			 }
			 else
			 {
				 nFreeIdx = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx;
			 }

			 pNewUser = (LibModuleStreamQueueUser_T *)Common_Malloc(sizeof(LibModuleStreamQueueUser_T),0,__FUNCTION__,__LINE__);
			 if (pNewUser == NULL)
			 {

				 nRet = MODULE_ERROR_TYPE_LIMITED;
				 break;
			 }
			 pUser = pNewUser;
			 memset(pUser,0,sizeof(LibModuleStreamQueueUser_T));
			 pUser->nLastReadIndex = -1;

			 pModuleMgr->tStreamQueueInfo.nStaticCount++;
			 if (pModuleMgr->tStreamQueueInfo.nStaticCount >= 0x7FFF)
			 {
				 pModuleMgr->tStreamQueueInfo.nStaticCount = 1;
			 }
			 nStreamQueueId = (pModuleMgr->tStreamQueueInfo.nStaticCount << 16) | (2 << 14) | nFreeIdx;
			 pUser->nStreamQueueId = nStreamQueueId;
			 nRet = 0;
		 }
		 // 请求资源

		 for (nSetIdx = 0; nSetIdx < nSetsNum;nSetIdx++)
		 {
			 S8 szTmp[128];
			 S8 *pRess = NULL;
			 nCode = -1;
			 if (pInParams != NULL)
			 {
				 Common_Json_Delete(pInParams);
				 pInParams = NULL;
			 }

			 if(pSets[nSetIdx].nIndex < 0 ||
				pSets[nSetIdx].nIndex >= LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM)
			 {
				 // 资源不可用
				 pSets[nSetIdx].nErrorCode = -1;
				 continue;
			 }
			 if (pUser->pStreamRessList[pSets[nSetIdx].nIndex] != NULL)
			 {
				 // 检查是否有改变
				 if (pSets[nSetIdx].szUri != NULL &&
					 pUser->pStreamRessList[pSets[nSetIdx].nIndex]->szUri != NULL &&
					 0 == Common_StrCmp(pSets[nSetIdx].szUri,pUser->pStreamRessList[pSets[nSetIdx].nIndex]->szUri) &&
					 pSets[nSetIdx].nMode == pUser->pStreamRessList[pSets[nSetIdx].nIndex]->nMode)
				 {
					 pSets[nSetIdx].nErrorCode = 0;
					 continue;
				 }
				 // 关闭，删除
				 // 关闭
				 //删除
				 pStreamRess = pUser->pStreamRessList[pSets[nSetIdx].nIndex];
				 pUser->pStreamRessList[pSets[nSetIdx].nIndex] = NULL;
				 pUser->nStreamRessCount--;
				 if (pStreamRess->nMode)
				 {
					 pUser->nStreamRessWriteCount--;
				 }



				 if (pStreamRess->pPrev == NULL)
				 {
					 if (pStreamRess->nMode == 0)
					 {// 读

						 pUser->pStreamRessHead = pStreamRess->pNext;
						 if (pUser->pStreamRessHead != NULL)
						 {
							 pUser->pStreamRessHead->pPrev = NULL;
						 }

					  }
					 else
					 {
						 pUser->pStreamRessWriteHead = pStreamRess->pNext;
						 if (pUser->pStreamRessWriteHead != NULL)
						 {
							 pUser->pStreamRessWriteHead->pPrev = NULL;
						 }

					 }
				 }
				 else if (pStreamRess->pNext == NULL)
				 {
					 pStreamRess->pPrev->pNext = NULL;
				 }
				 else
				 {
					pStreamRess->pPrev->pNext = pStreamRess->pNext;
					pStreamRess->pNext->pPrev = pStreamRess->pPrev;
				 }
				 pStreamRess->pNext = NULL;
				 pStreamRess->pPrev = NULL;
				 if (pStreamRess->pStreamRessArray != NULL)
				 {
					 S32 nArrayIdx;
					 for (nArrayIdx = 0;nArrayIdx < pStreamRess->nStreamRessArrayCount;nArrayIdx++)
					 {
						 Common_Free(pStreamRess->pStreamRessArray[nArrayIdx].szStreamRess,__FUNCTION__,__LINE__);
						 pStreamRess->pStreamRessArray[nArrayIdx].szStreamRess = NULL;
						 if(pStreamRess->nMode == 0)
							{
								STREAM_QUEUE_EPOLL_EVENT_T tEvent;
								memset(&tEvent,0,sizeof(tEvent));
								tEvent.streamQueueHandle = pStreamRess->pStreamRessArray[nArrayIdx].nShmId;
								tEvent.event = pStreamRess->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
								tEvent.userData = (void *)((pStreamRess->nIndex << 16) | (nArrayIdx));

								StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_RM,&tEvent);
							}

                         if (pStreamRess->pStreamRessArray[nArrayIdx].nShmId > 0)
                           {
                               if (pUser->nEpollReadHandle > 0)
                               {
                                   STREAM_QUEUE_EPOLL_EVENT_T tEvent;
                                   memset(&tEvent,0,sizeof(tEvent));
                                   tEvent.streamQueueHandle = pStreamRess->pStreamRessArray[nArrayIdx].nShmId;
                                   tEvent.event = pStreamRess->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
                                   StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_RM,&tEvent);
                               }
							   int nShmId = pStreamRess->pStreamRessArray[nArrayIdx].nShmId;
							   pStreamRess->pStreamRessArray[nArrayIdx].nShmId = -1;


							   Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
							   StreamQueue_Close(nShmId);
							   Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
                           }


					 }
					 Common_Free(pStreamRess->pStreamRessArray,__FUNCTION__,__LINE__);
					 pStreamRess->pStreamRessArray = NULL;
					 pStreamRess->nStreamRessArrayCount = 0;
				 }

				 // add delete list
				//printf("[%s.%d] delete....\n",__FUNCTION__,__LINE__);
				 staticStreamQueue_CheckThread(pModuleMgr,pStreamRess,NULL);
				 pStreamRess = NULL;
			 }
			 if(pSets[nSetIdx].szUri == NULL)
			 {
				 pSets[nSetIdx].nErrorCode = 0;
				 continue;
			 }
			 pStreamRess = (LibModuleStreamQueueStreamRess_T *)Common_Malloc(sizeof(LibModuleStreamQueueStreamRess_T),0,__FUNCTION__,__LINE__);
			 if (pStreamRess == NULL)
			 {
				  pSets[nSetIdx].nErrorCode = MODULE_ERROR_TYPE_LIMITED;
				 continue;
			 }
			 memset(pStreamRess,0,sizeof(LibModuleStreamQueueStreamRess_T));
			 pStreamRess->nIndex = pSets[nSetIdx].nIndex;
			 pStreamRess->szUri = Common_StrDup(pSets[nSetIdx].szUri,__FUNCTION__,__LINE__);
			 if (pStreamRess->szUri == NULL)
			 {
				  pSets[nSetIdx].nErrorCode = MODULE_ERROR_TYPE_LIMITED;
				  Common_Free(pStreamRess,__FUNCTION__,__LINE__);
				  pStreamRess = NULL;
				 continue;
			 }
			  sprintf(szTmp,"/%s/StreamQueue/Receive/%d/%d/%d",pModuleMgr->pszModuleName,pModuleMgr->nModuleMark,nStreamQueueId,pStreamRess->nIndex);
			  pStreamRess->szReceiveUri = Common_StrDup(szTmp,__FUNCTION__,__LINE__);
			  if (pStreamRess->szReceiveUri == NULL)
			  {
				  pSets[nSetIdx].nErrorCode = MODULE_ERROR_TYPE_LIMITED;
				  Common_Free(pStreamRess->szUri,__FUNCTION__,__LINE__);
				  Common_Free(pStreamRess,__FUNCTION__,__LINE__);
				  pStreamRess = NULL;
				  continue;
			  }
			  if(pSets[nSetIdx].pOpenParams != NULL)
			  {
			  	pStreamRess->pOpenParams = Common_Json_Duplicate(pSets[nSetIdx].pOpenParams,1);
			  }


			  nRet = 0;
			  pSets[nSetIdx].nErrorCode = 0;
			  pSets[nSetIdx].pOutStreamInfo = NULL;


			  // 成功
			  pStreamRess->pOwner = pUser;
			  pStreamRess->nMode = pSets[nSetIdx].nMode;
			  pUser->pStreamRessList[pStreamRess->nIndex] = pStreamRess;
			  pUser->nStreamRessCount++;

			  if (pStreamRess->nMode == 0)
			  {
				  pStreamRess->pNext = pUser->pStreamRessHead;
				  if (pUser->pStreamRessHead != NULL)
				  {
					  pUser->pStreamRessHead->pPrev = pStreamRess;
				  }
				  pUser->pStreamRessHead = pStreamRess;
			  }
			  else
			  {
				  pStreamRess->pNext = pUser->pStreamRessWriteHead;
				  if (pUser->pStreamRessWriteHead != NULL)
				  {
					  pUser->pStreamRessWriteHead->pPrev = pStreamRess;
				  }
				  pUser->pStreamRessWriteHead = pStreamRess;
				  pUser->nStreamRessWriteCount++;
			  }
			   // add Reconnect list
			   //printf("[%s.%d] add <%p>....\n",__FUNCTION__,__LINE__,pStreamRess);
			   staticStreamQueue_CheckThread(pModuleMgr,NULL,pStreamRess);

			   pStreamRess = NULL;


		 }



		if (fd <= 0)
		{
			pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] = pUser;
			pModuleMgr->tStreamQueueInfo.nInfoPoolCount++;
			pUser->pNext = pModuleMgr->tStreamQueueInfo.pUserListHead;
			if (pModuleMgr->tStreamQueueInfo.pUserListHead != NULL)
			{
				pModuleMgr->tStreamQueueInfo.pUserListHead->pPrev = pUser;
			}
			pModuleMgr->tStreamQueueInfo.pUserListHead = pUser;
			pNewUser = NULL;
			pModuleMgr->tStreamQueueInfo.nUserCount++;
			// 下一个可用的可能位置
			pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nFreeIdx + 1;
		}





	}while(0);
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	Common_Free(pToModuleName,__FUNCTION__,__LINE__);
	pToModuleName = NULL;
	if (pInParams != NULL)
	{
		Common_Json_Delete(pInParams);
		pInParams = NULL;
	}
	if (pOutParams != NULL)
	{
		Common_Json_Delete(pOutParams);
		pOutParams = NULL;
	}
	if (pStreamRess != NULL)
	{
		S32 nArrayIdx;
		if (pStreamRess->pStreamRessArray != NULL)
		{
			for (nArrayIdx = 0;nArrayIdx < pStreamRess->nStreamRessArrayCount;nArrayIdx++)
			{
				Common_Free(pStreamRess->pStreamRessArray[nArrayIdx].szStreamRess,__FUNCTION__,__LINE__);
				pStreamRess->pStreamRessArray[nArrayIdx].szStreamRess = NULL;
				if (pStreamRess->pStreamRessArray[nArrayIdx].nShmId > 0)
				{
					StreamQueue_Close(pStreamRess->pStreamRessArray[nArrayIdx].nShmId);
					pStreamRess->pStreamRessArray[nArrayIdx].nShmId = -1;
				}

			}
			pStreamRess->pStreamRessArray = NULL;
			pStreamRess->nStreamRessArrayCount = 0;
		}
		Common_Free(pStreamRess->szUri,__FUNCTION__,__LINE__);
		Common_Free(pStreamRess->szResUri,__FUNCTION__,__LINE__);
		Common_Free(pStreamRess->szReceiveUri,__FUNCTION__,__LINE__);
		Common_Free(pStreamRess,__FUNCTION__,__LINE__);
		pStreamRess = NULL;
	}
	if (pNewUser != NULL)
	{
		if (pNewUser->nEpollReadHandle > 0)
		{
			StreamQueue_EpollDestroy(pNewUser->nEpollReadHandle);
			pNewUser->nEpollReadHandle = 0;
		}
		Common_Free(pNewUser->szUri,__FUNCTION__,__LINE__);
		Common_Free(pNewUser->szResUri,__FUNCTION__,__LINE__);
		Common_Free(pNewUser->szReceiveUri,__FUNCTION__,__LINE__);
		Common_Free(pNewUser,__FUNCTION__,__LINE__);
		pNewUser = NULL;
	}
	static_StreamQueue_SaveCfg(hModuleHandle);
	if (nRet)
	{
		return nRet;
	}
	 return nStreamQueueId;

 }
S32 Module_StreamQueue_Close(ModuleHandle_T hModuleHandle,S32 fd)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nCount = 0,nTotalCount,nIdx,nType,nUserIndex,nUserId,i;
	S32 nStreamQueueId = -1;
	cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	S8 *pToModuleName = NULL;
	S32 nCode = -1,nRet = -1;
	S8 szTmp[128];
	nIdx = fd & 0x3FFF;
	nType = (fd >> 14) & 0x3;
	if (nType == 0 || nType == 1)
	{
		return -1;
	}
	if (pModuleMgr == NULL || nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=7;
	if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return -1;
	}
	pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
	if (pUser->nStreamQueueId != fd)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return -1;
	}
	if (pUser->nOwnerStreamQueueId != 0)
	{// 使用本地资源的远程用户
		S32 nType = (pUser->nOwnerStreamQueueId >> 14) & 0x3;
		S32 nPoolIdx = pUser->nOwnerStreamQueueId & 0x3FFF;
		S32 i;
		LibModuleStreamQueueOwner_T *pOwner;
		LibModuleStreamQueueCoOwner_T *pCoOwner;

		if (nPoolIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL ||
			(nType != 0 && nType != 1))
		{
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return -1;
		}
		if (pModuleMgr->tStreamQueueInfo.pInfoPool[nPoolIdx] == NULL)
		{
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return -1;
		}
		pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nPoolIdx];
		if (pOwner->nStreamQueueId != pUser->nOwnerStreamQueueId)
		{
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return -1;
		}
		pUser->bNeedDelete = 1; // 表示要删除
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pInParams != NULL)
		{
			pToModuleName = NULL;
			Common_UriOneParse(pUser->szReceiveUri,NULL,&pToModuleName,NULL);
			if (pToModuleName!= NULL)
			{
				nCode = -1;
				sprintf(szTmp,"/%s/StreamQueue/Kick",pToModuleName);
				Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
				Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
				Common_Json_SetAttrValue(pInParams,-1,"Header/ReceiveUri",Common_Json_Type_String,pUser->szReceiveUri,0,0);
				nRet = Module_CallFunctions(hModuleHandle,pInParams,NULL,3000);

				Common_Free(pToModuleName,__FUNCTION__,__LINE__);

			}
			Common_Json_Delete(pInParams);
			pInParams = NULL;
		}
		// 检查是否可以删除了
		do
		{
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=8;
			if (pUser->nUseCount <= 0)
			{
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				break;
			}
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			Common_Sleep(0,10000);
		} while (1);
		Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=9;
		if (nType == 0)
		{// Owner
			for (i = 0; i < LIBMODULE_STREAMQUEUE_MAX_USER_NUM;i++)
			{
				if (pOwner->pUserIdList[i] == fd)
				{
					// 找到，处理


					//删除
					pOwner->pUserIdList[i] = 0;
					pOwner->nUserCount--;
					break;
				}
			}
		}
		else
		{// CoOwner;
			pCoOwner = (LibModuleStreamQueueCoOwner_T *)pOwner;
			for (i = 0; i < LIBMODULE_STREAMQUEUE_MAX_USER_NUM;i++)
			{
				if (pCoOwner->pUserIdList[i] == fd)
				{
					// 找到，处理
					//删除
					pCoOwner->pUserIdList[i] = 0;
					pCoOwner->nUserCount--;
					break;
				}
			}
		}
	}
	else
	{
		LibModuleStreamQueueStreamRess_T *pRessNode = NULL,*pDelNode = NULL;
		S32 nSetIdx;
		cJSON_Struct **pInParams_array = NULL;
		S32 *nshmt_array= NULL;
		S32 nArrayCount = 0;
		// 使用远程资源的本地用户
		// 处理
		//pInParams_array = (cJSON_Struct **)Common_Malloc(sizeof(cJSON_Struct *) * pUser->nStreamRessCount,0,__FUNCTION__,__LINE__);
		//nshmt_array = (S32 *)Common_Malloc(sizeof(S32) * pUser->nStreamRessCount,0,__FUNCTION__,__LINE__);
		if (pInParams_array == NULL || nshmt_array == NULL)
		{
			//MODULE_LOGE("Malloc failed\n");
		}


		pUser->bNeedDelete = 1;
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		// 检查是否可以删除了
		do
		{
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=10;
			if (pUser->nUseCount <= 0)
			{
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				break;
			}
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			Common_Sleep(0,10000);
		} while (1);
		Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=11;

		pRessNode = pUser->pStreamRessHead;
		while(pRessNode != NULL)
		{
			pDelNode = pRessNode;
			pRessNode = pRessNode->pNext;


			if(pDelNode->pPrev == NULL)
			{
				pUser->pStreamRessHead = pDelNode->pNext;
				if (pUser->pStreamRessHead != NULL)
				{
					pUser->pStreamRessHead->pPrev = NULL;
				}
			}
			else if (pDelNode->pNext == NULL)
			{
				pDelNode->pPrev->pNext = NULL;
			}
			else
			{
				pDelNode->pPrev->pNext = pDelNode->pNext;
				pDelNode->pNext->pPrev = pDelNode->pPrev;
			}
			pDelNode->pPrev = NULL;
			pDelNode->pNext = NULL;

			pUser->pStreamRessList[pDelNode->nIndex] = NULL;
			pUser->nStreamRessCount--;
			S32 nArrayIdx;
			if (pDelNode->pStreamRessArray != NULL)
			{
				for (nArrayIdx = 0;nArrayIdx < pDelNode->nStreamRessArrayCount;nArrayIdx++)
				{
					Common_Free(pDelNode->pStreamRessArray[nArrayIdx].szStreamRess,__FUNCTION__,__LINE__);
					pDelNode->pStreamRessArray[nArrayIdx].szStreamRess = NULL;
					if (pDelNode->pStreamRessArray[nArrayIdx].nShmId > 0)
					{
						if (pUser->nEpollReadHandle > 0)
						{
							STREAM_QUEUE_EPOLL_EVENT_T tEvent;
							memset(&tEvent,0,sizeof(tEvent));
							tEvent.streamQueueHandle = pDelNode->pStreamRessArray[nArrayIdx].nShmId;
							tEvent.event = pDelNode->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
							StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_RM,&tEvent);
						}

						int nShmId = pDelNode->pStreamRessArray[nArrayIdx].nShmId;
						pDelNode->pStreamRessArray[nArrayIdx].nShmId = -1;
						Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);

						StreamQueue_Close(nShmId);

						Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
					}


				}
				Common_Free(pDelNode->pStreamRessArray,__FUNCTION__,__LINE__);
				pDelNode->pStreamRessArray = NULL;
				pDelNode->nStreamRessArrayCount = 0;
			}



			// add delete list
			//printf("[%s.%d] delete....\n",__FUNCTION__,__LINE__);
			staticStreamQueue_CheckThread(pModuleMgr,pDelNode,NULL);
			pDelNode = NULL;

		}
		pRessNode = pUser->pStreamRessWriteHead;
		while(pRessNode != NULL)
		{
			pDelNode = pRessNode;
			pRessNode = pRessNode->pNext;

			if(pDelNode->pPrev == NULL)
			{
					pUser->pStreamRessWriteHead = pDelNode->pNext;
					if (pUser->pStreamRessWriteHead != NULL)
					{
						pUser->pStreamRessWriteHead->pPrev = NULL;
					}
			}
			else if (pDelNode->pNext == NULL)
			{
				pDelNode->pPrev->pNext = NULL;
			}
			else
			{
				pDelNode->pPrev->pNext = pDelNode->pNext;
				pDelNode->pNext->pPrev = pDelNode->pPrev;
			}
			pDelNode->pPrev = NULL;
			pDelNode->pNext = NULL;

			pUser->nStreamRessWriteCount--;
			pUser->pStreamRessList[pDelNode->nIndex] = NULL;
			pUser->nStreamRessCount--;
			S32 nArrayIdx;
			if (pDelNode->pStreamRessArray != NULL)
			{
				for (nArrayIdx = 0;nArrayIdx < pDelNode->nStreamRessArrayCount;nArrayIdx++)
				{
					Common_Free(pDelNode->pStreamRessArray[nArrayIdx].szStreamRess,__FUNCTION__,__LINE__);
					pDelNode->pStreamRessArray[nArrayIdx].szStreamRess = NULL;
					if (pDelNode->pStreamRessArray[nArrayIdx].nShmId > 0)
					{
						if (pUser->nEpollReadHandle > 0)
						{
							STREAM_QUEUE_EPOLL_EVENT_T tEvent;
							memset(&tEvent,0,sizeof(tEvent));
							tEvent.streamQueueHandle = pDelNode->pStreamRessArray[nArrayIdx].nShmId;
							tEvent.event = pDelNode->nMode?STREAM_QUEUE_EPOLL_EVENT_WRITE:STREAM_QUEUE_EPOLL_EVENT_READ;
							StreamQueue_EpollCtl(pUser->nEpollReadHandle,STREAM_QUEUE_EPOLL_CTL_RM,&tEvent);
						}
						int nShmId = pDelNode->pStreamRessArray[nArrayIdx].nShmId;
						pDelNode->pStreamRessArray[nArrayIdx].nShmId = -1;
						Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
						StreamQueue_Close(nShmId);
						Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
					}

				}
				Common_Free(pDelNode->pStreamRessArray,__FUNCTION__,__LINE__);
				pDelNode->pStreamRessArray = NULL;
				pDelNode->nStreamRessArrayCount = 0;
			}



			// add delete list
			//printf("[%s.%d] delete....\n",__FUNCTION__,__LINE__);
			staticStreamQueue_CheckThread(pModuleMgr,pDelNode,NULL);
			pDelNode = NULL;

		}


		// 摘除
		if (pUser->pPrev == NULL)
		{
			pModuleMgr->tStreamQueueInfo.pUserListHead = pUser->pNext;
			if (pModuleMgr->tStreamQueueInfo.pUserListHead != NULL)
			{
				pModuleMgr->tStreamQueueInfo.pUserListHead->pPrev = NULL;
			}
		}
		else if (pUser->pNext == NULL)
		{
			pUser->pPrev->pNext = NULL;
		}
		else
		{
			pUser->pPrev->pNext = pUser->pNext;
			pUser->pNext->pPrev = pUser->pPrev;
		}
		pUser->pNext = NULL;
		pUser->pPrev = NULL;
		pModuleMgr->tStreamQueueInfo.nUserCount--;



	}
	//删除
    pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] = NULL;
	pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
	if (pUser->nEpollReadHandle > 0)
	{
		StreamQueue_EpollDestroy(pUser->nEpollReadHandle);
		pUser->nEpollReadHandle = 0;
	}
	Common_Free(pUser->szUri,__FUNCTION__,__LINE__);
	Common_Free(pUser->szResUri,__FUNCTION__,__LINE__);
	Common_Free(pUser->szReceiveUri,__FUNCTION__,__LINE__);
	Common_Free(pUser,__FUNCTION__,__LINE__);

	// 计算下次可用索引
	if(pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx == -1 ||
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nIdx;
	}
	else if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= nIdx)
	{
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nIdx;
	}
	else if (pModuleMgr->tStreamQueueInfo.pInfoPool[pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx] != NULL)
	{
		pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nIdx;
	}
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	return 0;
}


S32 Module_StreamQueue_WriteData(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,cJSON_Struct *pPrivInfo, void *pData1, S32 nSize1, void *pData2, S32 nSize2)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nCount = 0,nTotalCount,nIdx,nType,nUserIndex,nUserId,i;
	S32 nStreamQueueId = -1;
	S8 *pToModuleName = NULL;
	S32 nCode = -1,nRet = -1;
	//S8 szTmp[128];
	//S32 nStartIndex;
	LibModuleStreamQueueStreamRess_T *pRessNode= NULL;
	void *pPrivateData = NULL;
	int nPrivateDataSize;
	LibModuleStreamQueueOwner_T *pOwner= NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwner= NULL;
	S32 nShmId = -1;
	nIdx = fd & 0x3FFF;
	nType = (fd >> 14) & 0x3;
	if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL || nType > 2)
	{
		return -1;
	}
	if ((pData1 == NULL || nSize1 <= 0) && (pData2 == NULL || nSize2 <= 0))
	{
		return -1;
	}
	if (pData2 == NULL)
	{
		pData2 = pData1;
		nSize2 = nSize1;
		pData1 = NULL;
		nSize1 = 0;
	}

	g_nTestQueue_status1=1;
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=12;
	g_nTestQueue_status2=1;
	g_nTestQueue_status3=fd;
	do
	{

		if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
		{
			nRet = MODULE_ERROR_TYPE_NOTFOUND;
			break;
		}
		pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
		if (pUser->nStreamQueueId != fd)
		{
			nRet = MODULE_ERROR_TYPE_NOTFOUND;
			break;
		}
		if (nType == 0)
		{
			pOwner = (LibModuleStreamQueueOwner_T *)pUser;
			if (pOwner->nMode != 1)
			{// 没有权限printf("--%d--\n",__LINE__);

				nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
				break;
			}

			pPrivateData = NULL;
			nPrivateDataSize = 0;
			if (pPrivInfo != NULL)
			{
				pPrivateData = Common_Json_Print(pPrivInfo,&nPrivateDataSize);
			}


		    g_nTestQueue_status2=2;
			nShmId = pOwner->nShmId;


			nRet = 0;
			break;
		}
		else if(nType == 1)
		{
			S32 nCoIdx,nOwerId = 0,nOwerIdx;
			pCoOwner = (LibModuleStreamQueueCoOwner_T *)pUser;
			if (nIndex < 0 || nIndex >= pCoOwner->nOwnerCount)
			{
				break;
			}

			nOwerId = pCoOwner->pOwerIdList[nIndex];
			nOwerIdx = nOwerId & (0x3FFF);
			pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
			if (pOwner == NULL || pOwner->nStreamQueueId != nOwerId)
			{

				nRet = MODULE_ERROR_TYPE_STREAM_NOEXIST;
				break;
			}
			if (pOwner->nMode == 1)
			{

				nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
				break;
			}

			pPrivateData = NULL;
			nPrivateDataSize = 0;
			if (pPrivInfo != NULL)
			{
				pPrivateData = Common_Json_Print(pPrivInfo,&nPrivateDataSize);
			}
			g_nTestQueue_status2=5;
			nShmId = pOwner->nShmId;

			nRet = 0;
			break;
		}

		// user
		if (nIndex < 0)
		{
			nIndex = 0;
		}
		if(NULL == pUser->pStreamRessList[nIndex])
		{

			nRet = MODULE_ERROR_TYPE_STREAM_NOEXIST;
			break;
		}
		pRessNode = pUser->pStreamRessList[nIndex];
		if (pRessNode->nMode != 1)
		{

			nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
			break;
		}
		if (pRessNode->pStreamRessArray == NULL)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_INVALIDRESS;
			break;
		}
		if (pRessNode->pStreamRessArray[0].nShmId <= 0)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_INVALIDRESS;
			break;
		}


		pPrivateData = NULL;
		nPrivateDataSize = 0;
		if (pPrivInfo != NULL)
		{
			pPrivateData = Common_Json_Print(pPrivInfo,&nPrivateDataSize);
		}
		g_nTestQueue_status2=8;
		nShmId = pRessNode->pStreamRessArray[0].nShmId;

		nRet = 0;

		//操作完成
	}while(0);
	g_nTestQueue_status2=11;
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status2=12;
	g_nTestQueue_status1=2;
	if(nShmId > 0)
	{
		if(StreamQueue_WriteData(nShmId,pPrivateData,nPrivateDataSize,pData1,nSize1,pData2,nSize2,0))
		{
			g_nTestQueue_status2=3;
			nRet = MODULE_ERROR_TYPE_STREAM_WRITEFULL;
		}
	}
	g_nTestQueue_status2=4;
	if (pPrivateData != NULL)
	{
		 Common_Free(pPrivateData,__FUNCTION__,__LINE__);
		 pPrivateData = NULL;
	}


	return nRet;
}
static S32 static_StreamQueue_ReadDataByUser(LibModuleInfo_T *pModuleMgr,LibModuleStreamQueueUser_T *pUser,S32 nIndex,S32 *lpIndex,cJSON_Struct **pPrivInfo,void **pData, S32 *lpSize, S32 nTimeout)
{
	LibModuleStreamQueueStreamRess_T *pRessNode= NULL,*pRessNode_Last = NULL;
	S32 nRet = -1;
	void *pPrivateData = NULL;
	int nPrivateDataSize = 0;
	cJSON_Struct *pJsonData = NULL;
	S32 nleftCnt = 0;
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=13;

	do
	{
		if (pUser->bDataNeedRelease)
		{
			nRet =  MODULE_ERROR_TYPE_STREAM_NEEDRELEASE;
			break;
		}
		if (pUser->bNeedDelete)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_CLOSED;
			break;
		}
		pUser->nUseCount++;

		if (nIndex >= 0)
		{
			if (nIndex >= LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM)
			{
				break;
			}
			pRessNode = pUser->pStreamRessList[nIndex];
			if (pRessNode == NULL)
			{
				nRet = MODULE_ERROR_TYPE_NOTFOUND;
				break;
			}
			if (pRessNode->nMode != 0)
			{
				nRet = MODULE_ERROR_TYPE_LIMITED;
				break;
			}
			if (pRessNode->bKicked)
			{
				nRet = MODULE_ERROR_TYPE_STREAM_FORCECLOSE;
				break;
			}
			S32 nArrayIdx,bReadOk = 0,nReadIdx = -1;
			S32 EpollHandle = -1;
			STREAM_QUEUE_EPOLL_EVENT_T tEvent;
			pUser->nLastReadIndex = pRessNode->nIndex;


			bReadOk = 0,nReadIdx = -1;
			memset(&tEvent,0,sizeof(tEvent));
			EpollHandle = StreamQueue_EpollCreate();
			S32 nDoEventCount = 0;
			for (nArrayIdx = pRessNode->nLastReadIndex + 1; nArrayIdx < pRessNode->nStreamRessArrayCount;nArrayIdx++)
			{
				if (pRessNode->pStreamRessArray[nArrayIdx].nShmId > 0)
				{
					if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nArrayIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
					{
						nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nArrayIdx].nShmId);
						bReadOk = 1;
						nReadIdx = nArrayIdx;
						break;
					}
					tEvent.streamQueueHandle = pRessNode->pStreamRessArray[nArrayIdx].nShmId;
					tEvent.event = STREAM_QUEUE_EPOLL_EVENT_READ;
					tEvent.userData = (void *)nArrayIdx;
					StreamQueue_EpollCtl(EpollHandle,STREAM_QUEUE_EPOLL_CTL_ADD,&tEvent);
					nDoEventCount++;
				}
			}
			if (!bReadOk)
			{
				for (nArrayIdx = 0; nArrayIdx <= pRessNode->nLastReadIndex && nArrayIdx < pRessNode->nStreamRessArrayCount;nArrayIdx++)
				{
					if (pRessNode->pStreamRessArray[nArrayIdx].nShmId > 0)
					{
						if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nArrayIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
						{
							nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nArrayIdx].nShmId);
							bReadOk = 1;
							nReadIdx = nArrayIdx;
							break;
						}
						tEvent.streamQueueHandle = pRessNode->pStreamRessArray[nArrayIdx].nShmId;
						tEvent.event = STREAM_QUEUE_EPOLL_EVENT_READ;
						tEvent.userData = (void *)nArrayIdx;
						StreamQueue_EpollCtl(EpollHandle,STREAM_QUEUE_EPOLL_CTL_ADD,&tEvent);
						nDoEventCount++;
					}
				}
			}

			pRessNode->nLastReadIndex = nReadIdx;
			if (!bReadOk)
			{
				STREAM_QUEUE_EPOLL_EVENT_T *pEvents = NULL;
				if (nDoEventCount > 0)
				{
					S32 nRealCount = 0,nMultiIdx = 0,eIdx;
					pEvents = (STREAM_QUEUE_EPOLL_EVENT_T *)Common_Malloc(sizeof(STREAM_QUEUE_EPOLL_EVENT_T) * nDoEventCount,0,__FUNCTION__,__LINE__);
					if (pEvents != NULL)
					{
						Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
						nRealCount = StreamQueue_EpollWait(EpollHandle,pEvents,nDoEventCount,nTimeout);
						Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
						for (eIdx = 0; eIdx < nRealCount;eIdx++)
						{
							nMultiIdx = (S32)pEvents[eIdx].userData;

							if(0 == StreamQueue_ReadData(pEvents[eIdx].streamQueueHandle,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
							{
								nleftCnt = StreamQueue_GetRestCnt(pEvents[eIdx].streamQueueHandle);
								bReadOk = 1;
								nReadIdx = nMultiIdx;
								break;
							}
						}
						Common_Free(pEvents,__FUNCTION__,__LINE__);
						pEvents = NULL;
					}
				}
				pRessNode->nLastReadIndex = nReadIdx;
				if (!bReadOk)
				{
					nRet = MODULE_ERROR_TYPE_STREAM_NODATA;
					StreamQueue_EpollDestroy(EpollHandle);
					break;
				}
			}
			StreamQueue_EpollDestroy(EpollHandle);
			if (lpIndex != NULL)
			{
				*lpIndex = pRessNode->nIndex;
			}
			pUser->bDataNeedRelease = 1;
			pUser->nNeedReleaseIndex = pRessNode->nIndex;
			pUser->nNeedReleaseIndexMulti = nReadIdx;
			if (pPrivateData != NULL)
			{
				pJsonData = Common_Json_Parse((S8 *)pPrivateData,NULL,NULL);
			}
			if (pPrivInfo != NULL)
			{


				if(nleftCnt >= 0)
				{
					if (pJsonData == NULL)
					{
						pJsonData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
					}
					if (pJsonData != NULL)
					{
						Common_Json_SetAttrValue(pJsonData,-1,"RestCnt",Common_Json_Type_Number,NULL,nleftCnt,0);
					}
				}
				*pPrivInfo = pJsonData;
				pJsonData = NULL;
			}
			if (pJsonData != NULL)
			{
				Common_Json_Delete(pJsonData);
				pJsonData = NULL;

			}
			nRet = 0;
			break;
		}
		// 非指定索引的情况处理
		// 计算当前要读的位置
		pRessNode_Last = NULL;
		if(pUser->nLastReadIndex < 0)
		{
			pRessNode = pUser->pStreamRessHead;
		}
		else
		{
			pRessNode = pUser->pStreamRessList[pUser->nLastReadIndex];

			if (pRessNode == NULL)
			{
				pRessNode = pUser->pStreamRessHead;
			}
			else if(pRessNode->nMode != 0)
			{
				pRessNode = pUser->pStreamRessHead;
			}
			else
			{
				pRessNode_Last = pRessNode;
				pRessNode = pRessNode->pNext;
				if (pRessNode == NULL)
				{
					pRessNode = pUser->pStreamRessHead;
				}
			}
		}
		if (pRessNode == NULL)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_INVALIDRESS;
			break;
		}
		S32 nArrayIdx,bReadOk = 0,nReadIdx = -1;
		LibModuleStreamQueueStreamRess_T *pRessNodeOk= NULL;
		while(pRessNode != NULL)
		{
			for (nArrayIdx = pRessNode->nLastReadIndex + 1; nArrayIdx < pRessNode->nStreamRessArrayCount && pRessNode->pStreamRessArray != NULL;nArrayIdx++)
			{
				if (pRessNode->pStreamRessArray[nArrayIdx].nShmId > 0)
				{
					if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nArrayIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
					{
						nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nArrayIdx].nShmId);
						bReadOk = 1;
						nReadIdx = nArrayIdx;
						pRessNodeOk = pRessNode;
						break;
					}
				}

			}
			if (!bReadOk)
			{
				for (nArrayIdx = 0; nArrayIdx <= pRessNode->nLastReadIndex && nArrayIdx < pRessNode->nStreamRessArrayCount && pRessNode->pStreamRessArray != NULL;nArrayIdx++)
				{
					if (pRessNode->pStreamRessArray[nArrayIdx].nShmId > 0)
					{
						if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nArrayIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
						{
							nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nArrayIdx].nShmId);
							bReadOk = 1;
							nReadIdx = nArrayIdx;
							pRessNodeOk = pRessNode;
							break;
						}
					}

				}
			}
			if (bReadOk)
			{
				break;
			}
			// 往后
			pRessNode = pRessNode->pNext;

		};
		if ((!bReadOk) && pRessNode_Last != NULL)
		{
			pRessNode = pUser->pStreamRessHead;
			while(pRessNode != NULL)
			{
				for (nArrayIdx = pRessNode->nLastReadIndex + 1; nArrayIdx < pRessNode->nStreamRessArrayCount && pRessNode->pStreamRessArray != NULL;nArrayIdx++)
				{
					if (pRessNode->pStreamRessArray[nArrayIdx].nShmId > 0)
					{
						if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nArrayIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
						{
							nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nArrayIdx].nShmId);
							bReadOk = 1;
							nReadIdx = nArrayIdx;
							pRessNodeOk = pRessNode;
							break;
						}
					}

				}
				if (!bReadOk)
				{
					for (nArrayIdx = 0; nArrayIdx <= pRessNode->nLastReadIndex && nArrayIdx < pRessNode->nStreamRessArrayCount && pRessNode->pStreamRessArray!= NULL;nArrayIdx++)
					{
						if (pRessNode->pStreamRessArray[nArrayIdx].nShmId > 0)
						{
							if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nArrayIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
							{
								nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nArrayIdx].nShmId);
								bReadOk = 1;
								nReadIdx = nArrayIdx;
								pRessNodeOk = pRessNode;
								break;
							}
						}

					}
				}
				if (bReadOk)
				{
					break;
				}
				if (pRessNode == pRessNode_Last)
				{
					break;
				}
				pRessNode = pRessNode->pNext;

			};
		}
		// 全部没有数据，则需要epoll 一次
		if (!bReadOk)
		{
			STREAM_QUEUE_EPOLL_EVENT_T *pEvents;
			S32 nDoEventCount = 0,nRealCount = 0,eIdx,nResIdx,nMultiIdx;
			nDoEventCount = pUser->nStreamRessCount - pUser->nStreamRessWriteCount;
			if (pUser->nEpollReadHandle > 0 && nDoEventCount > 0)
			{
				pEvents = (STREAM_QUEUE_EPOLL_EVENT_T *)Common_Malloc(sizeof(STREAM_QUEUE_EPOLL_EVENT_T) * nDoEventCount,0,__FUNCTION__,__LINE__);
				if (pEvents != NULL)
				{
					S32 nEpollReadHandle = pUser->nEpollReadHandle;
					Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
					nRealCount = StreamQueue_EpollWait(nEpollReadHandle,pEvents,nDoEventCount,nTimeout);
					Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
					for (eIdx = 0; eIdx < nRealCount;eIdx++)
					{
						nMultiIdx = (S32)pEvents[eIdx].userData;
						nResIdx = nMultiIdx >> 16;
						nMultiIdx = nMultiIdx & 0xFFFF;
						if (nResIdx >= LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM)
						{
							continue;
						}
						pRessNode = pUser->pStreamRessList[nResIdx];
						if (pRessNode == NULL)
						{
							continue;
						}
						if (nMultiIdx >= pRessNode->nStreamRessArrayCount && pRessNode->pStreamRessArray == NULL)
						{
							continue;
						}
						if(0 == StreamQueue_ReadData(pRessNode->pStreamRessArray[nMultiIdx].nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,0))
						{
							nleftCnt = StreamQueue_GetRestCnt(pRessNode->pStreamRessArray[nMultiIdx].nShmId);
							bReadOk = 1;
							nReadIdx = nMultiIdx;
							pRessNodeOk = pRessNode;
							break;
						}
					}
					Common_Free(pEvents,__FUNCTION__,__LINE__);
					pEvents = NULL;
				}

			}
		}


		if (!bReadOk)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_NODATA;
			break;
		}
		pRessNodeOk->nLastReadIndex = nReadIdx;
		pUser->nLastReadIndex = pRessNodeOk->nIndex;
		if (lpIndex != NULL)
		{
			*lpIndex = pRessNodeOk->nIndex;
		}
		pUser->bDataNeedRelease = 1;
		pUser->nNeedReleaseIndex = pRessNodeOk->nIndex;
		pUser->nNeedReleaseIndexMulti = nReadIdx;
		if (pPrivateData != NULL)
		{
			pJsonData = Common_Json_Parse((S8 *)pPrivateData,NULL,NULL);
		}
		if (pPrivInfo != NULL)
		{
			if(nleftCnt >= 0)
			{
				if (pJsonData == NULL)
				{
					pJsonData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				}
				if (pJsonData != NULL)
				{
					Common_Json_SetAttrValue(pJsonData,-1,"RestCnt",Common_Json_Type_Number,NULL,nleftCnt,0);
				}
			}
			*pPrivInfo = pJsonData;
			pJsonData = NULL;
		}
		if (pJsonData != NULL)
		{
			Common_Json_Delete(pJsonData);
			pJsonData = NULL;

		}
		nRet = 0;
		break;




	} while (0);
	pUser->nUseCount--;
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	return nRet;
}

S32 Module_StreamQueue_ReadData(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,S32 *lpIndex,cJSON_Struct **pPrivInfo,void **pData, S32 *lpSize, S32 nTimeout)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nIdx,nType;
	//S32 nStreamQueueId = -1;
	//cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	//S8 *pToModuleName = NULL;
	S32 nRet = -1;
	//S8 szTmp[128];
	//S32 nStartIndex;
	//LibModuleStreamQueueStreamRess_T *pRessNode= NULL,*pLastRessNode = NULL;
	void *pPrivateData;
	int nPrivateDataSize;
	cJSON_Struct *pJsonData = NULL;
	LibModuleStreamQueueOwner_T *pOwner= NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwner= NULL;
	S32 nShmId = -1,nOwerIdx = -1;
	nIdx = fd & 0x3FFF;
	nType = (fd >> 14) & 0x3;

	if (pModuleMgr == NULL || nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL || nType > 2)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=15;
	do
	{

			if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
			{
				nRet = MODULE_ERROR_TYPE_NOTFOUND;
				break;
			}
			pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
			if (pUser->nStreamQueueId != fd)
			{
				nRet = MODULE_ERROR_TYPE_NOTFOUND;
				break;
			}
			if (nType == 0)
			{
				pOwner = (LibModuleStreamQueueOwner_T *)pUser;
				if (pOwner->nMode != 0)
				{// 没有权限
					nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
					break;
				}
				if (pOwner->bDataNeedRelease)
				{
					nRet = MODULE_ERROR_TYPE_STREAM_NEEDRELEASE;
					break;
				}

				nShmId = pOwner->nShmId;
				nOwerIdx = nIdx;


			}
			else if(nType == 1)
			{
				S32 nCoIdx,nOwerId = 0;
				pCoOwner = (LibModuleStreamQueueCoOwner_T *)pUser;
				if (pCoOwner->nLastReadIndex >= 0)
				{
					if (pCoOwner->pOwerIdList[pCoOwner->nLastReadIndex] != 0)
					{
						nOwerId = pCoOwner->pOwerIdList[pCoOwner->nLastReadIndex];
						nOwerIdx = nOwerId & (0x3FFF);
						pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
						if (pOwner != NULL && pOwner->nStreamQueueId == nOwerId)
						{
							if (pOwner->bDataNeedRelease)
							{
								nRet = MODULE_ERROR_TYPE_STREAM_NEEDRELEASE;
								break;
							}
						}

					}
				}
				else
				{
					pCoOwner->nLastReadIndex = -1;
				}
				for (nCoIdx = pCoOwner->nLastReadIndex + 1; nCoIdx < pCoOwner->nOwnerCount;nCoIdx++)
				{
					if (pCoOwner->pOwerIdList[nCoIdx] != 0)
					{
						nOwerId = pCoOwner->pOwerIdList[nCoIdx];
						nOwerIdx = nOwerId & (0x3FFF);
						pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
						if (pOwner != NULL && pOwner->nStreamQueueId == nOwerId)
						{
							if (pOwner->nMode == 0)
							{
								break;
							}
						}
						pOwner = NULL;

					}
				}
				if (pOwner == NULL)
				{
					for (nCoIdx = 0; nCoIdx <= pCoOwner->nLastReadIndex;nCoIdx++)
					{
						if (pCoOwner->pOwerIdList[nCoIdx] != 0)
						{
							nOwerId = pCoOwner->pOwerIdList[nCoIdx];
							nOwerIdx = nOwerId & (0x3FFF);
							pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
							if (pOwner != NULL && pOwner->nStreamQueueId == nOwerId)
							{
								if (pOwner->nMode == 0)
								{
									break;
								}
							}
							pOwner = NULL;

						}
					}
				}
				if (pOwner == NULL)
				{
					nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
					break;
				}
				pCoOwner->nLastReadIndex = nCoIdx;
				nShmId = pOwner->nShmId;

			}
		}while(0);
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);

		// type == 2
		if(nType == 2)
		{
			nRet = static_StreamQueue_ReadDataByUser(pModuleMgr,pUser,nIndex,lpIndex,pPrivInfo,pData,lpSize,nTimeout);
			return nRet;
		}
		else if (nShmId <= 0)
		{
			return nRet;
		}
		// type == 0 || type == 1
		if(StreamQueue_ReadData(nShmId,&pPrivateData,(U32 *)&nPrivateDataSize,pData,(U32 *)lpSize,nTimeout))
		{
			nRet = MODULE_ERROR_TYPE_STREAM_NODATA;

		}
		else
		{
			S32 bNeedRelease = 1;
			nRet = 0;
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=16;
			if (pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx] == pOwner)
			{// 确保对象还在，不在时需要释放流节点
				pOwner->bDataNeedRelease = 1;
				bNeedRelease = 0;
			}
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			if (bNeedRelease)
			{// 不在时需要释放流节点
				StreamQueue_ReleaseData(nShmId);
				nRet = MODULE_ERROR_TYPE_STREAM_NOEXIST;
			}

			if (pPrivateData != NULL)
			{
				pJsonData = Common_Json_Parse((S8 *)pPrivateData,NULL,NULL);
			}
			if (pPrivInfo != NULL && (!bNeedRelease))
			{
				S32 nleftCnt = 0;
				nleftCnt = StreamQueue_GetRestCnt(nShmId);
				if(nleftCnt >= 0)
				{
					if (pJsonData == NULL)
					{
						pJsonData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
					}
					if (pJsonData != NULL)
					{
						Common_Json_SetAttrValue(pJsonData,-1,"RestCnt",Common_Json_Type_Number,NULL,nleftCnt,0);
					}
				}
				*pPrivInfo = pJsonData;
				pJsonData = NULL;
			}
			if (pJsonData != NULL)
			{
				Common_Json_Delete(pJsonData);
				pJsonData = NULL;

			}



		}



		//操作完成

	return nRet;
}
S32 Module_StreamQueue_ReleaseData(ModuleHandle_T hModuleHandle,S32 fd)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nIdx,nType;
	//S32 nStreamQueueId = -1;
	//cJSON_Struct *pInParams = NULL,*pOutParams = NULL;
	//S8 *pToModuleName = NULL;
	S32 nRet = -1;
	//S8 szTmp[128];
	LibModuleStreamQueueOwner_T *pOwner;
	LibModuleStreamQueueCoOwner_T *pCoOwner;
	nIdx = fd & 0x3FFF;
	nType = (fd >> 14) & 0x3;

	if (pModuleMgr == NULL || nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL || nType > 2)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=17;
	do
	{
		if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_NOEXIST;
			break;
		}
		pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
		if (pUser->nStreamQueueId != fd)
		{
			nRet = MODULE_ERROR_TYPE_STREAM_NOEXIST;
			break;
		}
		if (nType == 0)
		{

			pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
			if (pOwner->bDataNeedRelease)
			{
				StreamQueue_ReleaseData(pOwner->nShmId);
				pOwner->bDataNeedRelease = 0;
			}
		}
		else if (nType == 1)
		{

			pCoOwner = (LibModuleStreamQueueCoOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
			if (pCoOwner->nLastReadIndex >= 0)
			{
				S32 nOwerId = pCoOwner->pOwerIdList[pCoOwner->nLastReadIndex];
				if (nOwerId != 0)
				{
					S32 nOwnerIdx = nOwerId & 0x3FFF;
					pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwnerIdx];
					if (pOwner != NULL && pOwner->nStreamQueueId == nOwerId)
					{
						if (pOwner->bDataNeedRelease)
						{
							StreamQueue_ReleaseData(pOwner->nShmId);
							pOwner->bDataNeedRelease = 0;
						}
					}
				}
			}
		}
		else if (nType == 2)
		{
			if (pUser->bDataNeedRelease && pUser->nNeedReleaseIndex >= 0 && pUser->nNeedReleaseIndex < LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM)
			{
				LibModuleStreamQueueStreamRess_T *pStreamRess = (LibModuleStreamQueueStreamRess_T *)pUser->pStreamRessList[pUser->nNeedReleaseIndex];
				if (pStreamRess != NULL)
				{
					if (pStreamRess->pStreamRessArray != NULL && pUser->nNeedReleaseIndexMulti >= 0 && pUser->nNeedReleaseIndexMulti < pStreamRess->nStreamRessArrayCount)
					{
						StreamQueue_ReleaseData(pStreamRess->pStreamRessArray[pUser->nNeedReleaseIndexMulti].nShmId);
					}

					pUser->bDataNeedRelease = 0;
				}

			}
		}
		nRet = 0;

		// 找到,开始 操作
	//操作完成
	} while (0);
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	return nRet;
}

S32 Module_StreamQueue_ClearData(ModuleHandle_T hModuleHandle,S32 fd)
{
    LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
    LibModuleStreamQueueUser_T *pUser = NULL;
    S32 nCount = 0,nTotalCount,nIdx,nType,nUserIndex,nUserId,i;
    S32 nStreamQueueId = -1;
    S8 *pToModuleName = NULL;
    S32 nCode = -1,nRet = -1;
    LibModuleStreamQueueStreamRess_T *pRessNode= NULL;
    LibModuleStreamQueueOwner_T *pOwner= NULL;
    LibModuleStreamQueueCoOwner_T *pCoOwner= NULL;
    S32 nShmId = -1;
    nIdx = fd & 0x3FFF;
    nType = (fd >> 14) & 0x3;
    if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL || nType > 2)
    {
        //printf("[%s:%d] nRet [-1]\n",__FUNCTION__,__LINE__);
        return -1;
    }

    Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
    do
    {

        if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
        {
            nRet = MODULE_ERROR_TYPE_NOTFOUND;
            break;
        }
        pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
        if (pUser->nStreamQueueId != fd)
        {
            nRet = MODULE_ERROR_TYPE_NOTFOUND;
            break;
        }

        {
            pOwner = (LibModuleStreamQueueOwner_T *)pUser;
            if (pOwner->nMode != 1)
            {// 没有权限printf("--%d--\n",__LINE__);

                nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
                break;
            }

            nShmId = pOwner->nShmId;

            nRet = 0;
            break;
        }

    }while(0);

    Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);

    if(nShmId > 0)
    {
        if(StreamQueue_ClearData(nShmId))
        {
            nRet = MODULE_ERROR_TYPE_STREAM_WRITEFULL;
        }
    }
    //printf("[%s:%d] nRet:[%d]\n",__FUNCTION__,__LINE__,nRet);

    return nRet;
}

S32 Module_StreamQueue_GetRestCnt(ModuleHandle_T hModuleHandle,S32 fd)
{
    LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
    LibModuleStreamQueueUser_T *pUser = NULL;
    S32 nCount = 0,nTotalCount,nIdx,nType,nUserIndex,nUserId,i;
    S32 nStreamQueueId = -1;
    S8 *pToModuleName = NULL;
    S32 nCode = -1,nRet = -1;
    LibModuleStreamQueueStreamRess_T *pRessNode= NULL;
    LibModuleStreamQueueOwner_T *pOwner= NULL;
    LibModuleStreamQueueCoOwner_T *pCoOwner= NULL;
    S32 nShmId = -1;
    nIdx = fd & 0x3FFF;
    nType = (fd >> 14) & 0x3;
    if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL || nType > 2)
    {
        //printf("[%s:%d] nRet [-1]\n",__FUNCTION__,__LINE__);
        return -1;
    }

    Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
    do
    {

        if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
        {
            nRet = MODULE_ERROR_TYPE_NOTFOUND;
            break;
        }
        pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
        if (pUser->nStreamQueueId != fd)
        {
            nRet = MODULE_ERROR_TYPE_NOTFOUND;
            break;
        }

        {
            pOwner = (LibModuleStreamQueueOwner_T *)pUser;
            /*if (pOwner->nMode != 1)
            {// 没有权限printf("--%d--\n",__LINE__);

                nRet = MODULE_ERROR_TYPE_STREAM_NORWRIGHT;
                break;
            }*/

            nShmId = pOwner->nShmId;

            nRet = 0;
            break;
        }

    }while(0);

    Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);

    if(nShmId > 0)
    {
        nRet = StreamQueue_GetRestCnt(nShmId);
    }
    //printf("[%s:%d] nRet:[%d]\n",__FUNCTION__,__LINE__,nRet);

    return nRet;
}

S32 Module_StreamQueue_EPoll_Create(StreamQueue_EPollHandle_T *pHandle,S32 nSize)
{
	return -1;
}
S32 Module_StreamQueue_EPoll_Ctl(StreamQueue_EPollHandle_T hHandle,S32 nOp,S32 nFd,StreamQueue_EPoll_Event_T *pEvent)
{
return -1;
}
S32 Module_StreamQueue_EPoll_Wait(StreamQueue_EPollHandle_T hHandle,StreamQueue_EPoll_Event_T *pResults,S32 nMaxResultsNum,S32 nTimeout)
{
return -1;
}
S32 Module_StreamQueue_EPoll_Destroy(StreamQueue_EPollHandle_T *hHandle)
{
return -1;
}


S32 Module_StreamQueue_Control(ModuleHandle_T hModuleHandle,S32 fd,S32 nIndex,cJSON_Struct *pControlInfo,cJSON_Struct **pResults,S32 nMSecTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nIdx;
	//S32 nStreamQueueId = -1;
	cJSON_Struct *pInParams = NULL;
	//cJSON_Struct *pOutParams = NULL;
	S8 *pToModuleName = NULL;
	//S32 nCode = -1,nRet = -1;
	S8 szTmp[128];
	nIdx = fd & 0x3FFF;

	if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
	{
		return -1;
	}
	if (nIndex >= LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM)
	{
		return -1;
	}
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=18;
	if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return -1;
	}
	pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx];
	if (pUser->nStreamQueueId != fd)
	{
		Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
		return -1;
	}
	pUser->nUseCount++;
	// 找到,开始 操作

	//S32 nResIdx = 0;
	//LibModuleStreamQueueStreamRess_T *pRessNode = NULL,*pCurrNode = NULL;
	if (nIndex >= 0)
	{
		LibModuleStreamQueueStreamRess_T *pStreamRess = NULL;
		S32 nTimeCnt = 0;
		pStreamRess = pUser->pStreamRessList[nIndex];
		if (pStreamRess == NULL)
		{
			pUser->nUseCount--;
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			return MODULE_ERROR_TYPE_NOTFOUND;
		}
		pStreamRess->nUseCount++;
		while(1)
		{
			if (pStreamRess->bNeedDelete)
			{
				break;
			}
			if (pStreamRess->szResUri != NULL)
			{
				break;
			}
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			Common_Sleep(0,100000);
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
			nTimeCnt++;
			if (nTimeCnt > 3000/100)
			{
				pStreamRess->nUseCount--;
				pUser->nUseCount--;
				Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
				return MODULE_ERROR_TYPE_TIMEOUT;
			}
		}
		pStreamRess->nUseCount--;
		pInParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pInParams != NULL)
		{
			pToModuleName = NULL;
			Common_UriOneParse(pUser->pStreamRessList[nIndex]->szResUri,NULL,&pToModuleName,NULL);
			if (pToModuleName!= NULL)
			{
				sprintf(szTmp,"/%s/StreamQueue/Control",pToModuleName);
				Common_Json_SetAttrValue(pInParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
				Common_Json_SetAttrValue(pInParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
				Common_Json_SetAttrValue(pInParams,-1,"Header/ResUri",Common_Json_Type_String,pUser->pStreamRessList[nIndex]->szResUri,0,0);
				Common_Json_AddItem(pInParams,-1,"Data",pControlInfo);
				Common_Free(pToModuleName,__FUNCTION__,__LINE__);

			}

		}

	}
	//操作完成
	pUser->nUseCount--;
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);


	if (pInParams != NULL)
	{
		Module_CallFunctions(hModuleHandle,pInParams,pResults,3000);
		Common_Json_DetachItem(pInParams,-1,"Data");
		Common_Json_Delete(pInParams);
		pInParams = NULL;
	}

	return 0;
}
S32 Module_StreamQueue_Require_Filter_Post(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nIdx;
	S32 nStreamQueueId = -1;
	cJSON_Struct *pOutParamsJson = NULL,*pNewOutParam = NULL;
	//S8 *pToModuleName = NULL;
	S32 nCode = -1;
	S8 szTmp[128];
	S8 *pPostUri = NULL, *pReceiveUri = NULL;
	S32 nFreeIdx = -1,bNeedFound = 0,nFoundStart = 0;
	LibModuleStreamQueueOwner_T *pOwner = NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
	Module_StreamQueue_Open_def fOpen = NULL;
	void *pUserData = NULL;
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=19;
	do
	{

		Common_Json_GetAttrValue(pInParams,-1,"Header/PostUri",NULL,&pPostUri,NULL,NULL);
		Common_Json_GetAttrValue(pInParams,-1,"Header/ReceiveUri",NULL,&pReceiveUri,NULL,NULL);

		if (pPostUri == NULL || pReceiveUri == NULL)
		{
			break;
		}

		nFreeIdx = -1;

		if (pModuleMgr->tStreamQueueInfo.nUserCount >= LIBMODULE_STREAMQUEUE_MAX_USER_NUM ||
			pModuleMgr->tStreamQueueInfo.nInfoPoolCount >= LIBMODULE_STREAMQUEUE_MAX_POOL)
		{
			break;
		}

		nFreeIdx = -1;
		if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx != -1 ||
			pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
		{
			if(NULL != pModuleMgr->tStreamQueueInfo.pInfoPool[pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx])
			{
				bNeedFound = 1;
				nFoundStart = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx + 1;
				if (nFoundStart >= LIBMODULE_STREAMQUEUE_MAX_POOL)
				{
					nFoundStart = 0;
				}
			}
		}
		else
		{
			bNeedFound = 1;
			nFoundStart = 0;
		}

		if (bNeedFound)
		{
			for(nIdx = nFoundStart;nIdx < LIBMODULE_STREAMQUEUE_MAX_POOL;nIdx++)
			{
				if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
				{//找到
					nFreeIdx = nIdx;
					break;
				}
			}
			if (nFreeIdx == -1)
			{// 未找到
				for(nIdx = 0; nIdx < nFoundStart;nIdx++)
				{
					if (pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] == NULL)
					{//找到
						nFreeIdx = nIdx;
						break;
					}

				}
			}
			if (nFreeIdx == -1)
			{// 不可能到达的

				break;
			}
		}
		else
		{
			nFreeIdx = pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx;
		}

		pUser = (LibModuleStreamQueueUser_T *)Common_Malloc(sizeof(LibModuleStreamQueueUser_T),0,__FUNCTION__,__LINE__);
		if (pUser == NULL)
		{
			break;
		}
		memset(pUser,0,sizeof(LibModuleStreamQueueUser_T));

		pUser->szUri = Common_StrDup(pPostUri,__FUNCTION__,__LINE__);
		if (pUser->szUri == NULL)
		{
			Common_Free(pUser,__FUNCTION__,__LINE__);
			pUser = NULL;
			break;
		}
		pUser->szReceiveUri = Common_StrDup(pReceiveUri,__FUNCTION__,__LINE__);
		if (pUser->szReceiveUri == NULL)
		{
			Common_Free(pUser->szUri,__FUNCTION__,__LINE__);
			Common_Free(pUser,__FUNCTION__,__LINE__);
			pUser = NULL;
			break;
		}

		pModuleMgr->tStreamQueueInfo.nStaticCount++;
		if (pModuleMgr->tStreamQueueInfo.nStaticCount >= 0x7FFF)
		{
			pModuleMgr->tStreamQueueInfo.nStaticCount = 1;
		}
		nStreamQueueId = (pModuleMgr->tStreamQueueInfo.nStaticCount << 16) | (2 << 14) | nFreeIdx;
		pUser->nStreamQueueId = nStreamQueueId;
		S32 nOwnerStreamQueueId = -1;
		cJSON_Struct *pCallInParam = NULL;
		pOutParamsJson = NULL;
		pCallInParam = Common_Json_GetItem(pInParams,-1,"Data");
		// 查找同一个uri
		g_nTestQueue_status4=191;
		pOwner = pModuleMgr->tStreamQueueInfo.pOwnerListHead;
		while (pOwner != NULL)
		{
			if (0 == Common_StriCmp(pOwner->szUri,pPostUri))
			{
				nOwnerStreamQueueId = pOwner->nStreamQueueId;
				fOpen = pOwner->tFxn.fOpen;
				pUserData = pOwner->pUserData;
				break;
			}
			pOwner = pOwner->pNext;
		}
		g_nTestQueue_status4=192;
		if (nOwnerStreamQueueId <= 0)
		{
			pCoOwner = pModuleMgr->tStreamQueueInfo.pCoOwnerListHead;
			while (pCoOwner != NULL)
			{
				if (0 == Common_StriCmp(pCoOwner->szUri,pPostUri))
				{
					nOwnerStreamQueueId = pCoOwner->nStreamQueueId;
					fOpen = pCoOwner->tFxn.fOpen;
					pUserData = pCoOwner->pUserData;
					break;
				}
				pCoOwner = pCoOwner->pNext;
			}
		}
		if (fOpen == NULL)
		{
			fOpen = pModuleMgr->tStreamQueueInfo.tGlobalFxn.fOpen;
			pUserData = pModuleMgr->tStreamQueueInfo.pUserData;
		}
		if (fOpen != NULL)
		{
			void *pUserPrivateData = pUser->pUserPrivateData;
			pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] = &g_hInvalidStreamQueueId;// 先hold住
			pModuleMgr->tStreamQueueInfo.nInfoPoolCount++;
			g_nTestQueue_status4=193;
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			// printf("[%s]****Test : nOwnerStreamQueueId = %d nStreamQueueId = %d\n",pModuleMgr->pszModuleName,nOwnerStreamQueueId,nStreamQueueId);
			nCode = fOpen(pModuleMgr->hModuleHandle,&nOwnerStreamQueueId,nStreamQueueId,pPostUri,pCallInParam,&pOutParamsJson,&pUserPrivateData,pUserData);
			g_nTestQueue_status4=194;
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
			g_nTestQueue_status4=19401;
			pUser->pUserPrivateData = pUserPrivateData;
			pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] =NULL;
			pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
		}
		if (nCode != 0 || nOwnerStreamQueueId <= 0)
		{
			if (nCode == 0)
			{
				nCode = MODULE_ERROR_TYPE_STREAM_INVALID;
			}
			Common_Free(pUser->szResUri,__FUNCTION__,__LINE__);
			Common_Free(pUser->szUri,__FUNCTION__,__LINE__);
			Common_Free(pUser->szReceiveUri,__FUNCTION__,__LINE__);
			Common_Free(pUser,__FUNCTION__,__LINE__);
			pUser = NULL;
			break;
		}
		//检查句柄是否合法
		S32 nType,nOwerIdx;


		nType = (nOwnerStreamQueueId >> 14)&0x3;
		nOwerIdx = (nOwnerStreamQueueId & 0x3FFF);
		if (nType < 0 || nType > 1 || nOwerIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL)
		{// 不合法
			nCode = MODULE_ERROR_TYPE_INTERNALERROR;
			break;
		}
		if (pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx] == NULL ||
			((LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx])->nStreamQueueId != nOwnerStreamQueueId)
		{
			// 不合法
			nCode = MODULE_ERROR_TYPE_INTERNALERROR;
			break;
		}
		if (nType == 0)
		{

			pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
			if (pOwner->nUserCount >= LIBMODULE_STREAMQUEUE_MAX_USER_NUM)
			{// 不考虑满的情况!
				nCode = MODULE_ERROR_TYPE_LIMITED;
				break;
			}
			g_nTestQueue_status4=195;
			for (nIdx = 0;nIdx < LIBMODULE_STREAMQUEUE_MAX_USER_NUM;nIdx++)
			{
				if (pOwner->pUserIdList[nIdx] == 0)
				{
					pOwner->pUserIdList[nIdx] = nStreamQueueId;
					pOwner->nUserCount++;
					pUser->nOwnerStreamQueueId = nOwnerStreamQueueId;
					pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] = pUser;
					pModuleMgr->tStreamQueueInfo.nInfoPoolCount++;
					pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nFreeIdx + 1;
					nCode = 0;
					break;
				}
			}
			g_nTestQueue_status4=196;
			if (!nCode)
			{
				pNewOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if (pNewOutParam != NULL)
				{
					cJSON_Struct *pArray = NULL;
					Common_Json_SetAttrValue(pNewOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pNewOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,nCode,0);
					sprintf(szTmp,"/%s/StreamQueue/Ress/%d/%d",pModuleMgr->pszModuleName,pOwner->nStreamQueueId,pUser->nStreamQueueId);
					Common_Json_SetAttrValue(pNewOutParam,-1,"Header/ResUri",Common_Json_Type_String,szTmp,0,0);
					pArray = Common_Json_SetAttrValue(pNewOutParam,-1,"Header/StreamsList",Common_Json_Type_Array,NULL,0,0);
					if (pArray != NULL)
					{
						Common_Json_SetAttrValue(pArray,0,NULL,Common_Json_Type_String,pOwner->szStreamRess,0,0);
					}
					if (pOutParamsJson != NULL)
					{
						Common_Json_AddItem(pNewOutParam,-1,"Data",pOutParamsJson);
					}


				}
			}

		}
		else
		{
			LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
			pCoOwner = (LibModuleStreamQueueCoOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
			if (pCoOwner->nUserCount >= LIBMODULE_STREAMQUEUE_MAX_USER_NUM)
			{// 不考虑满的情况!
				nCode = MODULE_ERROR_TYPE_LIMITED;
				break;
			}
			g_nTestQueue_status4=197;
			for (nIdx = 0;nIdx < LIBMODULE_STREAMQUEUE_MAX_USER_NUM;nIdx++)
			{
				if (pCoOwner->pUserIdList[nIdx] == 0)
				{
					pCoOwner->pUserIdList[nIdx] = nStreamQueueId;
					pCoOwner->nUserCount++;
					pUser->nOwnerStreamQueueId = nOwnerStreamQueueId;
					pModuleMgr->tStreamQueueInfo.pInfoPool[nFreeIdx] = pUser;
					pModuleMgr->tStreamQueueInfo.nInfoPoolCount++;
					pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nFreeIdx + 1;
					nCode = 0;
					// 需要返回 所有的共享内存句柄
					break;
				}
			}
			g_nTestQueue_status4=198;
			if (!nCode)
			{
				pNewOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if (pNewOutParam != NULL)
				{
					cJSON_Struct *pArray = NULL;
					Common_Json_SetAttrValue(pNewOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pNewOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,nCode,0);
					sprintf(szTmp,"/%s/StreamQueue/Ress/%d/%d",pModuleMgr->pszModuleName,pCoOwner->nStreamQueueId,pUser->nStreamQueueId);
					Common_Json_SetAttrValue(pNewOutParam,-1,"Header/ResUri",Common_Json_Type_String,szTmp,0,0);
					pArray = Common_Json_SetAttrValue(pNewOutParam,-1,"Header/StreamsList",Common_Json_Type_Array,NULL,0,0);
					if (pArray != NULL)
					{
						S32 nTotalCount = 0;
						for (nIdx = 0; nIdx < LIBMODULE_STREAMQUEUE_MAX_OWNER_NUM && nTotalCount < pCoOwner->nOwnerCount;nIdx++)
						{
							if(0 != pCoOwner->pOwerIdList[nIdx])
							{
								S32 nOwnerId = pCoOwner->pOwerIdList[nIdx];
								nOwerIdx = nOwnerId & 0x3FFF;
								pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nOwerIdx];
								Common_Json_SetAttrValue(pArray,nTotalCount,NULL,Common_Json_Type_String,pOwner->szStreamRess,0,0);

								nTotalCount++;
							}
						}

					}
					if (pOutParamsJson != NULL)
					{
						Common_Json_AddItem(pNewOutParam,-1,"Data",pOutParamsJson);
					}


				}
			}
		}


	g_nTestQueue_status4=199;



	} while (0);
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	g_nTestQueue_status4=1910;
	if (nCode)
	{
		if (pUser != NULL)
		{
			Common_Free(pUser->szResUri,__FUNCTION__,__LINE__);
			Common_Free(pUser->szUri,__FUNCTION__,__LINE__);
			Common_Free(pUser->szReceiveUri,__FUNCTION__,__LINE__);
			Common_Free(pUser,__FUNCTION__,__LINE__);
			pUser = NULL;
		}
	}
	if (pOutParams)
	{
		if (pNewOutParam == NULL)
		{
			pNewOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pNewOutParam != NULL)
			{

				Common_Json_SetAttrValue(pNewOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pNewOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,nCode,0);

			}
		}
		*pOutParams = pNewOutParam;
		pNewOutParam = NULL;

	}
	if (pNewOutParam != NULL)
	{
		Common_Json_Delete(pNewOutParam);
		pNewOutParam = NULL;
	}
	static_StreamQueue_SaveCfg((ModuleHandle_T)pModuleMgr);
	return 0;
}

S32 Module_StreamQueue_Require_Filter_Delete(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nTotalCount,nIdx,nType,nUserIndex,nUserId,i,nUserType;
	S32 nStreamQueueId = -1;
	cJSON_Struct *pNewOutParam = NULL;
	S32 nCode = -1;
	S8 *pDeal = NULL;
	S8 *pResUri = NULL;
	//S32 nFreeIdx = -1,bNeedFound = 0,nFoundStart = 0;
	//S32 nOwnerIdx;
	LibModuleStreamQueueOwner_T *pOwner = NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
	Module_StreamQueue_Close_def fClose = NULL;
	//void *pUserPrivateData = NULL;
	void *pUserData = NULL;
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=20;
	do
	{
		Common_Json_GetAttrValue(pInParams,-1,"Header/ResUri",NULL,&pResUri,NULL,NULL);
		if (pResUri == NULL)
		{
			break;
		}
		pDeal = strstr(pResUri,"/Ress/");
		if (pDeal == NULL)
		{
			break;
		}
		nStreamQueueId = -1;
		nUserId = -1;
		if(2 != sscanf(pDeal+6,"%d/%d",&nStreamQueueId,&nUserId))
		{
			break;
		}
		nType = (nStreamQueueId >> 14)&0x3;
		nIdx = nStreamQueueId & 0x3FFF;
		nUserType = (nUserId >> 14)&0x3;
		nUserIndex = nUserId & 0x3FFF;
		if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL ||
			nUserIndex >= LIBMODULE_STREAMQUEUE_MAX_POOL ||
			nUserType != 2 ||
			nType < 0||nType > 1)
		{
			break;
		}





		pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] ;
		pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] ;
		if (pUser == NULL ||
			pOwner == NULL)
		{
			break;
		}
		if (pUser->nStreamQueueId != nUserId ||
			pOwner->nStreamQueueId != nStreamQueueId ||
			pUser->nOwnerStreamQueueId != nStreamQueueId)
		{
			break;
		}
		if (nType == 0)
		{
			S32 nUserCount = pOwner->nUserCount;
			nTotalCount = 0;

			for (i = 0; i< LIBMODULE_STREAMQUEUE_MAX_USER_NUM && nTotalCount < nUserCount;i++)
			{
				if (pOwner->pUserIdList[i] != 0)
				{
					if (pOwner->pUserIdList[i] == nUserId)
					{
						pOwner->pUserIdList[i] = 0;
						pOwner->nUserCount--;
						break;
					}
					nTotalCount++;
				}
			}
			fClose = pOwner->tFxn.fClose;
			pUserData = pOwner->pUserData;
		}
		else
		{
			S32 nUserCount = pOwner->nUserCount;
			pCoOwner = (LibModuleStreamQueueCoOwner_T *)pOwner;
			nTotalCount = 0;
			for (i = 0; i< LIBMODULE_STREAMQUEUE_MAX_USER_NUM && nTotalCount < nUserCount;i++)
			{
				if (pCoOwner->pUserIdList[i] != 0)
				{
					if (pCoOwner->pUserIdList[i] == nUserId)
					{
						pCoOwner->pUserIdList[i] = 0;
						pCoOwner->nUserCount--;
						break;
					}
					nTotalCount++;
				}
			}
			fClose = pCoOwner->tFxn.fClose;
			pUserData = pCoOwner->pUserData;
		}
		if (fClose == NULL)
		{
			fClose = pModuleMgr->tStreamQueueInfo.tGlobalFxn.fClose;
			pUserData = pModuleMgr->tStreamQueueInfo.pUserData;
		}
		// 销毁user
		pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] = NULL;
		pModuleMgr->tStreamQueueInfo.nInfoPoolCount--;
		if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx == -1)
		{
			pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx  = nUserIndex;
		}
		else if (pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx > nUserIndex)
		{
			pModuleMgr->tStreamQueueInfo.nInfoPoolFreeIdx = nUserIndex;
		}
		// 回调
		if (fClose != NULL)
		{
			void *pUserPrivateData = pUser->pUserPrivateData;
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			g_nTestQueue_status4=21;
			fClose(pModuleMgr->hModuleHandle,nStreamQueueId,nUserId,NULL,NULL,pUserPrivateData,pUserData);
			g_nTestQueue_status4=2101;
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
			g_nTestQueue_status4=2102;
		}

		Common_Free(pUser->szUri,__FUNCTION__,__LINE__);
		Common_Free(pUser->szResUri,__FUNCTION__,__LINE__);
		Common_Free(pUser->szReceiveUri,__FUNCTION__,__LINE__);
		Common_Free(pUser,__FUNCTION__,__LINE__);
		nCode = 0;




	} while (0);
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	if (pOutParams)
	{
		if (pNewOutParam == NULL)
		{
			pNewOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pNewOutParam != NULL)
			{

				Common_Json_SetAttrValue(pNewOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pNewOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,nCode,0);

			}
		}
		*pOutParams = pNewOutParam;
		pNewOutParam = NULL;

	}
	if (pNewOutParam != NULL)
	{
		Common_Json_Delete(pNewOutParam);
		pNewOutParam = NULL;
	}
	static_StreamQueue_SaveCfg((ModuleHandle_T)pModuleMgr);

	return 0;
}

S32 Module_StreamQueue_Require_Filter_Control(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nIdx,nType,nUserIndex,nUserId,nUserType;
	S32 nStreamQueueId = -1;
	cJSON_Struct *pNewOutParam = NULL;
	//S8 *pToModuleName = NULL;
	S32 nCode = -1;
	S8 *pDeal = NULL;
	S8 *pResUri = NULL;
	//S32 nFreeIdx = -1,bNeedFound = 0,nFoundStart = 0;
	LibModuleStreamQueueOwner_T *pOwner = NULL;
	LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
	Module_StreamQueue_Control_def fControl = NULL;
	//void *pUserPrivateData = NULL;
	void *pUserData = NULL;
	cJSON_Struct *pInParamData = NULL,*pOutParamData = NULL;
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=22;
	do
	{
	Common_Json_GetAttrValue(pInParams,-1,"Header/ResUri",NULL,&pResUri,NULL,NULL);
	if (pResUri == NULL)
	{
		break;
	}
	pDeal = strstr(pResUri,"/Ress/");
	if (pDeal == NULL)
	{
		break;
	}
	nStreamQueueId = -1;
	nUserId = -1;
	if(2 != sscanf(pDeal+6,"%d/%d",&nStreamQueueId,&nUserId))
	{
		break;
	}
	nType = (nStreamQueueId >> 14)&0x3;
	nIdx = nStreamQueueId & 0x3FFF;
	nUserType = (nUserId >> 14)&0x3;
	nUserIndex = nUserId & 0x3FFF;
	if (nIdx >= LIBMODULE_STREAMQUEUE_MAX_POOL ||
		nUserIndex >= LIBMODULE_STREAMQUEUE_MAX_POOL ||
		nUserType != 2 ||
		nType < 0||nType > 1)
	{
		break;
	}





		pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] ;
		pOwner = (LibModuleStreamQueueOwner_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nIdx] ;
		if (pUser == NULL ||
			pOwner == NULL)
		{
			break;
		}
		if (pUser->nStreamQueueId != nUserId ||
			pOwner->nStreamQueueId != nStreamQueueId ||
			pUser->nOwnerStreamQueueId != nStreamQueueId)
		{
			break;
		}
		if (nType == 0)
		{
			fControl = pOwner->tFxn.fControl;
			pUserData = pOwner->pUserData;
		}
		else
		{
			pCoOwner = (LibModuleStreamQueueCoOwner_T *)pOwner;
			fControl = pCoOwner->tFxn.fControl;
			pUserData = pCoOwner->pUserData;
		}
		if (fControl == NULL)
		{
			fControl = pModuleMgr->tStreamQueueInfo.tGlobalFxn.fControl;
			pUserData = pModuleMgr->tStreamQueueInfo.pUserData;
		}
		// 回调
		if (fControl != NULL)
		{
			void *pPrivateData = pUser->pUserPrivateData;
			pInParamData = Common_Json_GetItem(pInParams,-1,"Data");
			Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
			g_nTestQueue_status4=2201;
			fControl(pModuleMgr->hModuleHandle,nStreamQueueId,nUserId,pInParamData,&pOutParamData,pPrivateData,pUserData);
			g_nTestQueue_status4=2202;
			Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);
			g_nTestQueue_status4=2203;
		}

		nCode = 0;




	} while (0);
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	if (pOutParams)
	{
		if (pNewOutParam == NULL)
		{
			pNewOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pNewOutParam != NULL)
			{

				Common_Json_SetAttrValue(pNewOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pNewOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,nCode,0);
				if (pOutParamData != NULL)
				{
					Common_Json_AddItem(pNewOutParam,-1,"Data",pOutParamData);
					pOutParamData = NULL;
				}


			}
		}
		*pOutParams = pNewOutParam;
		pNewOutParam = NULL;

	}
	if (pNewOutParam != NULL)
	{
		Common_Json_Delete(pNewOutParam);
		pNewOutParam = NULL;
	}
	if (pOutParamData != NULL)
	{
		Common_Json_Delete(pOutParamData);
		pOutParamData = NULL;
	}

	return 0;
}

S32 Module_StreamQueue_Require_Filter_Kick(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleStreamQueueUser_T *pUser = NULL;
	S32 nUserIndex,nUserId,nUserType,nModuleMark = -1;
	//S32 nStreamQueueId = -1;
	cJSON_Struct *pNewOutParam = NULL;
	//S8 *pToModuleName = NULL;
	//char *pStringValue = NULL;
	//S32 nLen;
	//S8 szTmp[128];
	S8 *pDeal = NULL;
	S8 *pReceiveUri = NULL;
	//S32 nFreeIdx = -1,bNeedFound = 0,nFoundStart = 0;
	S32 nStreamIndex = -1;
	//LibModuleStreamQueueOwner_T *pOwner = NULL;
	//LibModuleStreamQueueCoOwner_T *pCoOwner = NULL;
	//Module_StreamQueue_Control_def fControl = NULL;
	//void *pUserPrivateData = NULL;
	//void *pUserData = NULL;
	//cJSON_Struct *pInParamData = NULL,*pOutParamData = NULL;
	LibModuleStreamQueueStreamRess_T *pStreamRess = NULL;
	Common_Lock(pModuleMgr->tStreamQueueInfo.hLock);g_nTestQueue_status4=23;
	do
	{

		Common_Json_GetAttrValue(pInParams,-1,"Header/ReceiveUri",NULL,&pReceiveUri,NULL,NULL);
		if (pReceiveUri == NULL)
		{

			break;
		}
		pDeal = strstr(pReceiveUri,"/Receive/");
		if (pDeal == NULL)
		{

			break;
		}
		if(3 != sscanf(pDeal+9,"%d/%d/%d",&nModuleMark,&nUserId,&nStreamIndex))
		{
			break;
		}

		nUserType = (nUserId >> 14)&0x3;
		nUserIndex = nUserId & 0x3FFF;
		if (nUserIndex >= LIBMODULE_STREAMQUEUE_MAX_POOL ||
			nUserType != 2 ||
			nStreamIndex < 0 || nStreamIndex >= LIBMODULE_STREAMQUEUE_MAX_USER_STREAM_NUM)
		{

			break;
		}

		pUser = (LibModuleStreamQueueUser_T *)pModuleMgr->tStreamQueueInfo.pInfoPool[nUserIndex] ;
		if (pUser == NULL)
		{

			break;
		}
		if (pUser->nStreamQueueId != nUserId)
		{

			break;
		}
		if (pUser->pStreamRessList[nStreamIndex] == NULL)
		{

			break;
		}
		pUser->pStreamRessList[nStreamIndex]->bKicked = 1;
		pStreamRess = pUser->pStreamRessList[nStreamIndex];

		// add Reconnect list
		//printf("[%s.%d] kick add....\n",__FUNCTION__,__LINE__);
		staticStreamQueue_CheckThread(pModuleMgr,NULL,pStreamRess);
		pStreamRess = NULL;


	} while (0);
	Common_UnLock(pModuleMgr->tStreamQueueInfo.hLock);
	if (pOutParams)
	{
		if (pNewOutParam == NULL)
		{
			pNewOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pNewOutParam != NULL)
			{

				Common_Json_SetAttrValue(pNewOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pNewOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);

			}
		}
		*pOutParams = pNewOutParam;
		pNewOutParam = NULL;

	}
	if (pNewOutParam != NULL)
	{
		Common_Json_Delete(pNewOutParam);
		pNewOutParam = NULL;
	}

	return 0;
}

S32 Module_StreamQueue_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	//LibModuleStreamQueueUser_T *pUser = NULL;
	//S32 nCount = 0,nTotalCount,nIdx,nType,nUserIndex,nUserId,i;
	//S32 nStreamQueueId = -1;
	//cJSON_Struct *pOutParamsJson = NULL;
	//S8 *pToModuleName = NULL;
	//S32 nCode = -1,nRet = -1;
	char *pStringValue = NULL;
	S32 nLen;
	S8 szTmp[128],*pDeal = NULL;
	S8 *pReceiveUri = NULL,*pKickUri = NULL,*pControlUri = NULL;
	//S32 nFreeIdx = -1,bNeedFound = 0,nFoundStart = 0;
	if (pInParams == NULL || pModuleMgr == NULL)
	{
		return -1;
	}
	// Common_Json_StandardPrint(pInParams,"------<",">\n",NULL);
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pStringValue,NULL,NULL);
	if (pStringValue == NULL)
	{
		return -1;
	}
	nLen = sprintf(szTmp,"/%s/StreamQueue/",pModuleMgr->pszModuleName);
	if (0 != Common_StrniCmp(pStringValue,szTmp,nLen))
	{// 非本模块
		return -1;
	}

	pDeal = pStringValue + nLen;

	Common_Json_GetAttrValue(pInParams,-1,"Header/ReceiveUri",NULL,&pReceiveUri,NULL,NULL);

	Common_Json_GetAttrValue(pInParams,-1,"Header/KickUri",NULL,&pKickUri,NULL,NULL);
	Common_Json_GetAttrValue(pInParams,-1,"Header/ControlUri",NULL,&pControlUri,NULL,NULL);


		if (0 == Common_StrniCmp(pDeal,(char*)"Post",4))
		{
			Module_StreamQueue_Require_Filter_Post(pModuleMgr,pInParams,pOutParams);
		}
		else if (0 == Common_StrniCmp(pDeal,(char*)"Delete",6))
		{
			Module_StreamQueue_Require_Filter_Delete(pModuleMgr,pInParams,pOutParams);
		}
		else if (0 == Common_StrniCmp(pDeal,(char*)"Control",7))
		{
			Module_StreamQueue_Require_Filter_Control(pModuleMgr,pInParams,pOutParams);
		}
		else if (0 == Common_StrniCmp(pDeal,(char*)"Kick",4))
		{
			Module_StreamQueue_Require_Filter_Kick(pModuleMgr,pInParams,pOutParams);
		}


	return 0;

}
