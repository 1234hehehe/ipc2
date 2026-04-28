/*
 * ovfs_alarm.c
 *
 *  Created on: 2017年3月07日
 *      Author:
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strtok_r strtok_s
#else

#include <dirent.h>

#include <unistd.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>
#endif

//#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "libaccess_api.h"
#include "access_auth.h"

//static Common_Thread_T g_hThread;
static cJSON_Struct *g_right_res = NULL;
static OVFS_ABILITY *g_ability = NULL;
static int       g_nUserCounter = 0;

int auth_MakeHandle(int nLoginHandle,int nSubIndex)
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

S8* auth_EncryptString(S32 nMethod,S8 *pOrgString,S8 *pNewString,S32 nNewSize)
{
	S8 *pNewReturnString = NULL;
	S32 i,nOrgLen;
	if (pOrgString == NULL)
	{
		return pNewReturnString;
	}
	nOrgLen = strlen(pOrgString);
	// 检查是否合法
	for (i = 0; i < nOrgLen; i++)
	{
		if (!(
			(pOrgString[i] >= 'A' && pOrgString[i] <= 'Z') ||
						(pOrgString[i] >= 'a' && pOrgString[i] <= 'z') ||
						(pOrgString[i] >= '0' && pOrgString[i] <= '9') ||
						pOrgString[i] == '_' ||
						pOrgString[i] == '-')
			)
		{
			return NULL;
		}
	}
	if (nMethod == 0)
	{
		if (pNewString == NULL)
		{
			pNewReturnString = Common_StrDup(pOrgString,__FUNCTION__,__LINE__);
		}
		else if(nOrgLen + 1 <= nNewSize)
		{

			Common_Strcpy(pNewString,pOrgString);
			pNewReturnString = pNewString;
		}
	}
	else if (nMethod == 1)
	{ // 加密 $<n chars length>$<n chars type>$[EncryptKey]$do_type(base64(pwd))
		S8 *pBase64;
		S32 nNew64Len = 0,nNewLen;
		S8 cTmp;
		pBase64 = Common_Base64_Encode(pOrgString,nOrgLen,(U32 *)&nNew64Len);
		if (pBase64 == NULL)
		{
			return pNewReturnString;
		}
		for (i = 0; i < nNew64Len / 2;i += 2)
		{
			cTmp = pBase64[i * 2];
			pBase64[i * 2] = pBase64[i * 2 + 1];
			pBase64[i * 2 + 1] = cTmp;
		}
		pNewReturnString = (S8 *)Common_Malloc(nNew64Len + 1 + 2 + 4 + 4,0,__FUNCTION__,__LINE__);
		if (pNewReturnString != NULL)
		{
			nNewLen = sprintf(pNewReturnString,"$%d$%d$$%s",nNew64Len,1,pBase64);
			if (pNewString != NULL)
			{
				if (nNewLen + 1 <= nNewSize)
				{
					Common_Strcpy(pNewString,pNewReturnString);
					Common_Free(pNewReturnString,__FUNCTION__,__LINE__);
					pNewReturnString = pNewString;
				}
				else
				{
					Common_Free(pNewReturnString,__FUNCTION__,__LINE__);
					pNewReturnString = NULL;
				}
			}
		}
		Common_Free(pBase64,__FUNCTION__,__LINE__);
		pBase64 = NULL;

	}
	return pNewReturnString;
}
S8* auth_DecryptString(S8 *pOrgString,S32 *lpEncryptType,S8 *pNewString,S32 nNewSize)
{
	S8 *pNewReturnString = NULL,*pNext = NULL;
	S32 nOrgLen,nCodeLen,nEncryptType;
	if (pOrgString == NULL)
	{
		return NULL;
	}
	nOrgLen = strlen(pOrgString);
	if (pOrgString[0] != '$')
	{// 未加密
		if (lpEncryptType != NULL)
		{
			*lpEncryptType = 0;
		}
		//LOGW("[Decrypt]pNewString:%s,Len:%d\n",pOrgString,nOrgLen);
		if (pNewString == NULL)
		{
			pNewReturnString = Common_StrDup(pOrgString,__FUNCTION__,__LINE__);
		}
		else if(nOrgLen + 1 <= nNewSize)
		{
			Common_Strcpy(pNewString,pOrgString);
			pNewReturnString = pNewString;
		}
		return pNewReturnString;
	}
	nCodeLen = atoi(pOrgString + 1);
	pNext = strchr(pOrgString + 1,'$');
	if (pNext != NULL)
	{// EncryptType
		nEncryptType = atoi(pNext + 1);
		pNext = strchr(pNext + 1,'$');
		if (pNext != NULL)
		{// EncryptKey
			pNext = strchr(pNext + 1,'$');
			if (pNext != NULL)
			{// EncryptCode
				S32 nNewCodeLen,nNewOrgLen = 0;
				S8 *pNewOrgString = NULL;
				nNewCodeLen = strlen(pNext + 1);
				if (nNewCodeLen == nCodeLen)
				{
					if (nEncryptType == 0)
					{

					}
					else if (nEncryptType == 1)
					{
						S8 cTmp;
						S32 i;
						S8 *pBase64 = Common_StrDup(pNext + 1,__FUNCTION__,__LINE__);
						if (pBase64 != NULL)
						{
							for (i = 0; i < nNewCodeLen/2;i += 2)
							{
								cTmp = pBase64[i * 2];
								pBase64[i * 2] = pBase64[i * 2 + 1];
								pBase64[i * 2 + 1] = cTmp;
							}
							pNewOrgString = Common_Base64_Decode(pBase64,nNewCodeLen,(U32 *)&nNewOrgLen);
							Common_Free(pBase64,__FUNCTION__,__LINE__);
							if (pNewOrgString != NULL)
							{
								pNewReturnString = pNewOrgString;
								if (lpEncryptType != NULL)
								{
									*lpEncryptType = nEncryptType;
								}
								if (pNewString != NULL)
								{
									pNewReturnString = NULL;
									//LOGW("[Decrypt]pNewOrgString:%s,Len:%d\n",pNewOrgString,nNewOrgLen);
									if (nNewOrgLen + 1 <= nNewSize)
									{
										Common_Strcpy(pNewString,pNewOrgString);
										pNewReturnString = pNewString;

									}
									Common_Free(pNewOrgString,__FUNCTION__,__LINE__);
									pNewOrgString = NULL;
								}
							}

						}

					}

				}
			}
		}
	}

	return pNewReturnString;
}

// 系统默认固定有二个用户
// admin - 0xFF 顶级管理员 密码:12345678
// guest - 1    访客       密码:guest

// 用户不能自己改自己权限
// 顶级管理员不能重置自己的密码
// 一般管理员能否重置自己的密码，要看是否被顶级管理员授权
// 一般管理员不能重置其他管理密码的密码
// 平级用户不能互相修改用户相关信息
// 权限对照表:
/*
权限          访客           操作员      管理员
预览（总开关）    1             1               1
回放              			0             1               1
设置              			0             0               1
查看设置          		0             1               1
录像              			0             0               1
PTZ               				0             1               1
备份              			0             0               1
日志              			0             1               1
设备信息          		0             1               1
升级              			0             0               1
电源 关机，重启	0		1		    1
格式化				0		0		   1
IP通道				0		1		   1
修改时间          		0             0               1
删除用户          		0             0               1
修改用户          		0             0               1
重置密码          		0             0               1
重置自己的密码    0             1               1
在线授权          		0             0               1

预览 (通道)       		1             1               1
回放              			0             1               1
设置              			0             0               1
查看设置          		0             1               1
录像              			0             0               1
PTZ               				0             1               1
备份              			0             0               1
*/

//解析表达式

static S32 SeparateExpression(S8 *pExpression,S8 **pString,S8 **pValue)
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
static cJSON_Struct *SeparateCondition(S8 *pCondition)
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
static cJSON_Struct *SeparateUriAndCondition(S8 *pSrcUri,S8 **pDstUri)
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

static U8 CheckNodeRight(cJSON_Struct *pData,S8 *pUri,S8 *pMethod,U32 u32Right)
{
	U8 bOk = 1;
	S32 s32Value = -1;
	cJSON_Struct *pChild = NULL,*pItem = NULL;

	pChild = Common_Json_GetItem(pData,-1,pUri);
	if(pChild)
	{
		//ovfs_print_json(pChild);
		if(0==Common_StriCmp(pMethod, (S8*)"get"))
		{
			pItem = Common_Json_GetAttrValue(pChild, -1, "GetType", NULL, NULL, &s32Value, NULL);
			if(pItem){
				if(-1 == s32Value)
					bOk = 1;
				else
					bOk = (s32Value&u32Right?1:0);
			}
		}
		else if(0==Common_StriCmp(pMethod, (S8*)"put"))
		{
			pItem = Common_Json_GetAttrValue(pChild, -1, "PutType", NULL, NULL, &s32Value, NULL);
			if(pItem){
				if(-1 == s32Value)
					bOk = 1;
				else
					bOk = (s32Value&u32Right?1:0);
			}
		}
		else if(0==Common_StriCmp(pMethod, (S8*)"post"))
		{
			pItem = Common_Json_GetAttrValue(pChild, -1, "PostType", NULL, NULL, &s32Value, NULL);
			if(pItem){
				if(-1 == s32Value)
					bOk = 1;
				else
					bOk = (s32Value&u32Right?1:0);
			}
		}
		else if(0==Common_StriCmp(pMethod, (S8*)"delete"))
		{
			pItem = Common_Json_GetAttrValue(pChild, -1, "DeleteType", NULL, NULL, &s32Value, NULL);
			if(pItem){
				if(-1 == s32Value)
					bOk = 1;
				else
					bOk = (s32Value&u32Right?1:0);
			}
		}
	}
	return bOk;
}

S32 AnalyzeUriAndAuthRight(S8 *pMethod,S8 *pUri,U32 u32Right)
{
	S8 pResPath[128] = {0};
	S8 *pTemp = NULL,*pSrcUri = NULL,*pFirst = NULL,*pNext = NULL;
	cJSON_Struct *pRoot = g_right_res,*pChild = NULL,*pJCondition = NULL;
	U8 bOk = 1,bResult = 1;

	if(!pRoot)
	{
		LOGE("g_rest_res is null\n");
		return 0;
	}
	pJCondition = SeparateUriAndCondition(pUri, &pSrcUri);
	pTemp = pSrcUri;
	while(pTemp)
	{
		Common_UriOneParse(pTemp,NULL,&pFirst,&pNext);
		if(pFirst){
			sprintf(pResPath,"%s/%s",pResPath,pFirst);
			if(pNext)
			{
				bResult = CheckNodeRight(pRoot,pResPath,pMethod,u32Right);
				if(0 == bResult)
				{
					bOk= 0;
					LOGE("no right![Uri:%s,u32Right:%#x]\n",pResPath,u32Right);
					break;
				}
				pTemp = pNext;
			}
			else
			{
				pChild = Common_Json_GetItem(pRoot,-1,pResPath);
				if(pChild)
				{
					//LOGE("pResPath:%s:,u32Right:%d\n",pResPath,u32Right);
					bOk = CheckNodeRight(pRoot,pResPath,pMethod,u32Right);
					//LOGD("Auth right bOk:%d!Request Uri:%s\n",bOk,pResPath);
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
	return bOk;
}

OVFS_ABILITY * get_channel_ability()
{
	return g_ability;
}

cJSON_Struct *get_conditionFromUri(S8 *pUri)
{
	return SeparateUriAndCondition(pUri,NULL);
}

int request_channel_ability(ModuleHandle_T hModuleHandle)
{
	S8 strcmd[64] = {0};
	POVFS_ABILITY pAbility = NULL;
	cJSON_Struct *pConfig,*pOutParams = NULL;
	S32 i = 0,j = 0,iDevNum = -1,iChanNum = -1,iStreamNum = -1;
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Boardsys/Video/Ability/Number",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
	}
	Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
	Common_Json_Delete(pConfig);
	pConfig = NULL;
	if(!pOutParams)
	{
		LOGE("pOutParams == NULL!\n");
		return -1;
	}
	Common_Json_GetAttrValue(pOutParams,-1,"Data/DevTotalNum",NULL,NULL,&iDevNum,NULL);
	if(iDevNum<0)
	{
		LOGE("DevTotalNum:%d!\n",iDevNum);
		Common_Json_Delete(pOutParams);
		return -1;
	}
	Common_Json_GetAttrValue(pOutParams,-1,"Data/ChanTotalNum",NULL,NULL,&iChanNum,NULL);
	if(iChanNum<0)
	{
		LOGE("ChanTotalNum:%d!\n",iChanNum);
		Common_Json_Delete(pOutParams);
		return -1;
	}
	Common_Json_GetAttrValue(pOutParams,-1,"Data/StreamNum",NULL,NULL,&iStreamNum,NULL);
	if(iStreamNum<0)
	{
		LOGE("StreamNum:%d!\n",iStreamNum);
		Common_Json_Delete(pOutParams);
		return -1;
	}
	pAbility = (OVFS_ABILITY*)Common_Malloc(sizeof(OVFS_ABILITY),0,__FUNCTION__,__LINE__);
	if(!pAbility)
	{
		LOGE("pAbility:null!\n");
		Common_Json_Delete(pOutParams);
		return -1;
	}
	memset(pAbility,0,sizeof(OVFS_ABILITY));
	pAbility->devNum = iDevNum;
	pAbility->totalChanNum = iChanNum;
	pAbility->totalStreamNum = iStreamNum;
	if(iDevNum <= 0)
	{
		LOGE("iDevNum:%d!\n",iDevNum);
		Common_Json_Delete(pOutParams);
		Common_Free(pAbility,__FUNCTION__,__LINE__);
		return -1;
	}
	pAbility->pDevAbility = (OVFS_DEV_ABILITY**)Common_Malloc(iDevNum*sizeof(OVFS_DEV_ABILITY*),0,__FUNCTION__,__LINE__);
	if(!pAbility->pDevAbility)
	{
		LOGE("pDevAbility:null!\n");
		Common_Json_Delete(pOutParams);
		Common_Free(pAbility,__FUNCTION__,__LINE__);
		return -1;
	}
	memset(pAbility->pDevAbility,0,iDevNum*sizeof(OVFS_DEV_ABILITY*));
	for(i = 0;i < iDevNum;i++)
	{
		OVFS_DEV_ABILITY *pdev = NULL;
		iChanNum = -1;
		sprintf(strcmd,"Data/viDev%d/viChanNum",i);
		Common_Json_GetAttrValue(pOutParams,-1,strcmd,NULL,NULL,&iChanNum,NULL);
		if(iChanNum <= 0)
			continue;
		pdev = (OVFS_DEV_ABILITY*)Common_Malloc(sizeof(OVFS_DEV_ABILITY),0,__FUNCTION__,__LINE__);
		if(!pdev)
			continue;
		memset(pdev,0,sizeof(OVFS_DEV_ABILITY));
		pdev->chanNum = iChanNum;
		pAbility->pDevAbility[i] = pdev;
		pdev->pChanAbility = (OVFS_CHANNEL_ABILITY**)Common_Malloc(iChanNum*sizeof(OVFS_CHANNEL_ABILITY*),0,__FUNCTION__,__LINE__);
		if(!pdev->pChanAbility)
		{
			Common_Free(pdev,__FUNCTION__,__LINE__);
			continue;
		}
		memset(pdev->pChanAbility,0,pdev->chanNum*sizeof(OVFS_CHANNEL_ABILITY*));
		for(j = 0;j < iChanNum;j++)
		{
			iStreamNum = -1;
			sprintf(strcmd,"Data/viDev%d/viChan%d",i,j);
			Common_Json_GetAttrValue(pOutParams,-1,strcmd,NULL,NULL,&iStreamNum,NULL);
			if(iStreamNum <= 0)
				continue;
			OVFS_CHANNEL_ABILITY *pchan = (OVFS_CHANNEL_ABILITY*)Common_Malloc(sizeof(OVFS_CHANNEL_ABILITY),0,__FUNCTION__,__LINE__);
			if(!pchan)
				continue;
			memset(pchan,0,sizeof(OVFS_CHANNEL_ABILITY));
			pchan->streamNum = iStreamNum;
			pdev->pChanAbility[j] = pchan;
		}
	}
	if(pOutParams)
		Common_Json_Delete(pOutParams);

	if(g_ability)
	{
		if(g_ability->pDevAbility){
			for(i = 0;i < g_ability->devNum;i++){
				OVFS_DEV_ABILITY *pdev = g_ability->pDevAbility[i];
				if(pdev){
					if(pdev->pChanAbility){
						for(j = 0;j < pdev->chanNum;j++){
							OVFS_CHANNEL_ABILITY *pchan = pdev->pChanAbility[j];
							if(pchan)
							{
								Common_Free(pchan,__FUNCTION__,__LINE__);
								pdev->pChanAbility[j] = NULL;
							}
						}
						Common_Free(pdev->pChanAbility,__FUNCTION__,__LINE__);
						pdev->pChanAbility = NULL;
					}
					Common_Free(pdev,__FUNCTION__,__LINE__);
					g_ability->pDevAbility[i] = NULL;
				}
			}
			Common_Free(g_ability->pDevAbility,__FUNCTION__,__LINE__);
			g_ability->pDevAbility = NULL;
		}
		Common_Free(g_ability,__FUNCTION__,__LINE__);
		g_ability = NULL;
	}
	g_ability = pAbility;
	for(i = 0;i < g_ability->devNum;i++)
	{
		OVFS_DEV_ABILITY *pdev = g_ability->pDevAbility[i];
		if(!pdev)
			continue;
		for(j = 0;j < pdev->chanNum;j++)
		{
			OVFS_CHANNEL_ABILITY *pchan = pdev->pChanAbility[j];
			if(!pchan)
				continue;
			LOGD("Dev%d:%d,Chan%d:%d\n",i,pdev->chanNum,j,pchan->streamNum);
		}
	}
	LOGD("DevNum:%d\n",g_ability->devNum);
	return 0;
}

 S32 SetNodeRightType(cJSON_Struct *pNode,S32 iPost,S32 iGet,S32 iPut,S32 iDelete)
 {
	if(!pNode)
	{
		return -1;
	}
	Common_Json_SetAttrValue(pNode,-1,"PostType",Common_Json_Type_Number,NULL,iPost,0);
	Common_Json_SetAttrValue(pNode,-1,"GetType",Common_Json_Type_Number,NULL,iGet,0);
	Common_Json_SetAttrValue(pNode,-1,"PutType",Common_Json_Type_Number,NULL,iPut,0);
	Common_Json_SetAttrValue(pNode,-1,"DeleteType",Common_Json_Type_Number,NULL,iDelete,0);
	return 0;
 }

 S32 LoadDefRightCfg(ModuleHandle_T hModuleHandle,cJSON_Struct **pRightCfg)
 {
	//ResRight_T iRight[4] = {{-1},{-1},{-1},{-1}};
	//ResRight_T iDefRight[4] = {{-1},{-1},{-1},{-1}};
	cJSON_Struct *pConfig = NULL,*pItem = NULL;
	//NodeRight_T *pNodeRight = NULL;

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pConfig)
	{
		pItem = Common_Json_SetAttrValue(pConfig,-1,"Core",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem = Common_Json_SetAttrValue(pConfig,-1,"Core/Version",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,-1,-1,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Core/Time",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Core/Maintain",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Core/Power",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_POWER,BIT_POWER,BIT_POWER,BIT_POWER);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Core/DeviceName",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Core/Restore",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Core/ImportCfg",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);

		pItem = Common_Json_SetAttrValue(pConfig,-1,"Update",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_UPGRADE,BIT_UPGRADE,BIT_UPGRADE,BIT_UPGRADE);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Boardsys",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/LiveStream",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,BIT_PREVIEW,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/Ability",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/Ability/Number",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,-1,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/Ability/Venc",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/Roi",Common_Json_Type_Object,NULL,0,0);
		//SetNodeRightType(pItem,-1,-1,-1,-1);
		SetNodeRightType(pItem,0,-1,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Video/Roi/All",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Audio",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Audio/Ability",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_PREVIEW,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Audio/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Audio/Aenc",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Audio/Aenc/LiveStream",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,BIT_PREVIEW,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Speak",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Speak/Ability",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_PREVIEW,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Speak/Aenc",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_REMOTETALK,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Speak/Adec",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_REMOTETALK,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/VideoInput",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/VideoInput/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Osd",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Image",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Mask",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Serial",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Serial/Ability",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_PREVIEW,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Ability",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_PREVIEW,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/AlarmIn",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/AlarmIn/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/AlarmIn/Status",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_LOG,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Motion",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Motion/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Motion/Status",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_LOG,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Hide",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Hide/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Event/Hide/Status",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_LOG,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/AlarmOut",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/AlarmOut/Ability",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_PREVIEW,0,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/AlarmOut/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Osd/ChannelName",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Osd/MulString",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Osd/Time",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Security",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"BoardSys/Security/SerialNum",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO,0,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Ptz",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Ptz/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_SETTING,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,BIT_SETTING);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Ptz/Protocol",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_SETTING,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,BIT_SETTING);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/conf",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_SETTING,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,BIT_SETTING);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/image",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_SETTING,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,BIT_SETTING);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/videoinput",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_SETTING,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,BIT_SETTING);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/videosourcemode",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_SETTING,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,BIT_SETTING);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Ptz/Cmd",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/move",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/preset",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/cruises",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/tracks",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/twopointscanf",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/homeposition",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/spray",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/privacymask",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/idleoperate",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/auxoperate",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/stop",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"/Ptz/update",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_PTZ,BIT_PTZ,BIT_PTZ,BIT_PTZ);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"MediaServer",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"MediaServer/Rtmp",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"MediaServer/Rtmp/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,-1,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"MediaServer/Rtsp",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"MediaServer/Rtsp/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,-1,BIT_SETTING,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Alarm",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		Common_Json_SetAttrValue(pConfig,-1,"Alarm/RightType",Common_Json_Type_Number,NULL,0,0);
		SetNodeRightType(pItem,0,-1,-1,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Alarm/Arming",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Alarm/EmailCfg",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Alarm/TriggerCfg",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Alarm/LinkageCfg",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Alarm/Status",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_LOG,0,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"EventLog",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"EventLog/LogFunction",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_LOG,BIT_LOG,BIT_LOG,BIT_LOG);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Access",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Access/UserCfg",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_MODIFYUSER,-1,BIT_MODIFYUSER,BIT_DELETEUSER);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Access/BindUser",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_MODIFYUSER,-1,BIT_MODIFYUSER,BIT_DELETEUSER);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Access/OnlineUser",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Access/Subscribe",Common_Json_Type_Object,NULL,0,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Network",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Network/NetAttr",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Network/NetApp",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Record",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Record/RecordConfig",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Record/Replay",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_PLAYBACK,BIT_PLAYBACK,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"Record/DiskManage",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWINFO|BIT_VIEWSETTING|BIT_SETTING,BIT_FORMAT,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"SmartServer",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"SmartServer/Attribute",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"AccessHost",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,-1,-1,-1,-1);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"AccessHost/HostLists",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"AccessHost/Gb28181",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);
		pItem= Common_Json_SetAttrValue(pConfig,-1,"AccessHost/Burn",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,0,BIT_VIEWSETTING|BIT_SETTING,BIT_SETTING,0);

		pItem= Common_Json_SetAttrValue(pConfig,-1,"Update",Common_Json_Type_Object,NULL,0,0);
		SetNodeRightType(pItem,BIT_FORMAT,BIT_FORMAT,BIT_FORMAT,BIT_FORMAT);

	}
	*pRightCfg = pConfig;
	return 0;
 }

int LoadRightModel(ModuleHandle_T hModuleHandle)
{
	LoadDefRightCfg(hModuleHandle,&g_right_res);
	//ovfs_print_json(g_right_res);
	return 0;
}

int UnLoadRightModel(ModuleHandle_T hModuleHandle)
{
	int i = 0,j = 0;
	if(g_right_res)
		Common_Json_Delete(g_right_res);
	g_right_res = NULL;
	if(g_ability)
	{
		if(g_ability->pDevAbility){
			for(i = 0;i < g_ability->devNum;i++){
				OVFS_DEV_ABILITY *pdev = g_ability->pDevAbility[i];
				if(pdev){
					if(pdev->pChanAbility){
						for(j = 0;j < pdev->chanNum;j++){
							OVFS_CHANNEL_ABILITY *pchan = pdev->pChanAbility[j];
							if(pchan)
							{
								Common_Free(pchan,__FUNCTION__,__LINE__);
								pdev->pChanAbility[j] = NULL;
							}
						}
						Common_Free(pdev->pChanAbility,__FUNCTION__,__LINE__);
						pdev->pChanAbility = NULL;
					}
					Common_Free(pdev,__FUNCTION__,__LINE__);
					g_ability->pDevAbility[i] = NULL;
				}
			}
			Common_Free(g_ability->pDevAbility,__FUNCTION__,__LINE__);
			g_ability->pDevAbility = NULL;
		}
		Common_Free(g_ability,__FUNCTION__,__LINE__);
		g_ability = NULL;
	}
	//ovfs_print_json(g_right_res);
	return 0;
}

