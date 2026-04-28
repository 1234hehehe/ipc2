#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcommon_struct.h"
#include "libcommon_api.h"

#include "cjson.h"
//JSON
cJSON_Struct * Common_Json_Parse(const S8 *szJsonString,const S8 **pEndString,const S8 **pszErrorString)
{
	return (cJSON_Struct *)Common_cJSON_Parse(szJsonString,pEndString,pszErrorString);
}

S8 * Common_Json_Print(cJSON_Struct *pJson,S32 *lpStrLen)
{
	return Common_cJSON_Print((Common_cJSON_T *)pJson,lpStrLen);
}

S8 * Common_Json_PrintUnformatted(cJSON_Struct *pJson,S32 *lpStrLen)
{
	return Common_cJSON_PrintUnformatted((Common_cJSON_T *)pJson,lpStrLen);
}
void Common_Json_StandardPrint(cJSON_Struct *pJson,S8 *pPrefix,S8 *pSuffix,Common_Json_Print_def fMyPrintf)
{
	S8 *pStr = Common_cJSON_Print((Common_cJSON_T *)pJson,NULL);
	if (pStr != NULL)
	{
		if (fMyPrintf != NULL)
		{
			fMyPrintf("%s%s%s",pPrefix?pPrefix:"",pStr,pSuffix?pSuffix:"");
		}
		else
		{
			printf("%s%s%s",pPrefix?pPrefix:"",pStr,pSuffix?pSuffix:"");
		}
		
		Common_Free(pStr,__FUNCTION__,__LINE__);
	}
}

S32 Common_UriOneParse(S8 *pszPath,S32 *pArrayIndex,S8 **CurrString,S8 **NextString)
{
	S8 cCurrChar,*pStartString = NULL,*pEndString=NULL,*pNewString = NULL,*pArray = NULL;
	S32 nCurrLen = 0,i,bArrayBefore = 0,nArrayLen;
	if (pszPath == NULL)
	{
		return -1;
	}
	i = 0;
	bArrayBefore = 0;
	nArrayLen = 0;
	if (pArrayIndex)
	{
		*pArrayIndex = -1;
	}
	if (NextString)
	{
		*NextString = NULL;
	}

	while (1)
	{
		cCurrChar = pszPath[i];
		if ((cCurrChar >= 'a' && cCurrChar <= 'z') ||
			(cCurrChar >= 'A' && cCurrChar <= 'Z') ||
			(cCurrChar >= '0' && cCurrChar <= '9')||
			cCurrChar == '_' ||
			cCurrChar == '-')
		{
			if (bArrayBefore == 1)
			{
				if (cCurrChar >= '0' && cCurrChar <= '9')
				{
					nArrayLen++;
				}
				else
				{
					return -1;
				}
				if (pArray == NULL)
				{
					pArray = pszPath + i;
				}
			}
			else if (bArrayBefore == 2)
			{
				if (pEndString)
				{
					if (NextString)
					{
						*NextString =pszPath + i;
					}
					break;
				}
				return -1;
			}
			else
			{
				if (pStartString == NULL)
				{
					pStartString = pszPath + i;
				}
				else if (pEndString)
				{
					if (NextString)
					{
						*NextString =pszPath + i;
					}
					break;
				}
				nCurrLen++;
			}

				
			
		}
		else if (cCurrChar == '[')
		{
			if (pStartString == NULL)
			{
				return -1;
			}
			if (bArrayBefore == 1)
			{
				return -1;
			}
			bArrayBefore = 1;

		}
		else if(cCurrChar == ']')
		{
			if (bArrayBefore != 1)
			{
				return -1;
			}
			if (nArrayLen == 0)
			{
				return -1;
			}
			bArrayBefore = 2;
		}
		else if (cCurrChar == '.' ||
			     cCurrChar == '/' ||
				 cCurrChar == '\\'||
				 cCurrChar == 0)
		{
			if (bArrayBefore == 0)
			{
				if (cCurrChar == 0)
				{
					break;
				}
				if (pStartString != NULL)
				{
					pEndString = pszPath + i;
				}
				
			}
			else if (bArrayBefore == 1)
			{
				return -1;
			}
			else
			{
				if (cCurrChar == 0)
				{
					break;
				}
				if (pStartString != NULL)
				{
					pEndString = pszPath + i;
				}
				
			}
			
		}
		else
		{
			return -1;
		}
		i++;
	}
	if (CurrString != NULL && pStartString != NULL)
	{
		pNewString = (S8 *)Common_Malloc(nCurrLen + 1,0,__FUNCTION__,__LINE__);
		if (pNewString == NULL)
		{
			return -1;
		}
		memcpy(pNewString,pStartString,nCurrLen);
		pNewString[nCurrLen] = 0;
		*CurrString = pNewString;
	}
	
	if (pArray)
	{
		*pArrayIndex = atoi(pArray);
	}
	return 0;
}
cJSON_Struct *Common_Json_New(const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat)
{
	Common_cJSON_T *pNew = NULL;
	Common_Trace_T hTrace = NULL;
	S8 *pfName = NULL,*pSyName = NULL;
	Common_Trace_Return(&hTrace,0,&pfName,&pSyName,NULL);
	if (hTrace == NULL)
	{
		pSyName = (S8 *)__FUNCTION__;
	}

	if (nType == Common_Json_Type_NULL)
	{
		pNew = Common_cJSON_CreateNull_ex(pSyName,__LINE__);
	}
	else if (nType == Common_Json_Type_False)
	{
		pNew = Common_cJSON_CreateFalse_ex(pSyName,__LINE__);
	}
	else if (nType == Common_Json_Type_True)
	{
		pNew = Common_cJSON_CreateTrue_ex(pSyName,__LINE__);
	}
	else if (nType == Common_Json_Type_Number)
	{
		pNew = Common_cJSON_CreateNumber_ex(nIntValue,pSyName,__LINE__);
	}
	else if (nType == Common_Json_Type_Double)
	{
		pNew = Common_cJSON_CreateDouble_ex(fFloat,pSyName,__LINE__);
	}
	else if (nType == Common_Json_Type_String)
	{
		if (pStringValue != NULL)
		{
			pNew = Common_cJSON_CreateString_ex(pStringValue,pSyName,__LINE__);
		}
		
	}
	else if (nType == Common_Json_Type_Array)
	{
		pNew = Common_cJSON_CreateArray_ex(pSyName,__LINE__);
	}
	else if (nType == Common_Json_Type_Object)
	{
		pNew = Common_cJSON_CreateObject_ex(pSyName,__LINE__);
	}
	if (pNew)
	{
		if (szObjectName != NULL)
		{
			pNew->string = Common_StrDup((S8 *)szObjectName,pSyName,__LINE__);
			if (pNew->string == NULL)
			{
				Common_cJSON_Delete_ex(pNew,pSyName,__LINE__);
				pNew = NULL;
			}
		}
		
	}
	Common_Trace_RetFree(&hTrace);
	return pNew;
	
}

cJSON_Struct *Common_Json_New_ex(const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat,const S8 *pDes,S32 nLine)
{
	Common_cJSON_T *pNew = NULL;
	
	if (nType == Common_Json_Type_NULL)
	{
		pNew = Common_cJSON_CreateNull_ex(pDes,nLine);
	}
	else if (nType == Common_Json_Type_False)
	{
		pNew = Common_cJSON_CreateFalse_ex(pDes,nLine);
	}
	else if (nType == Common_Json_Type_True)
	{
		pNew = Common_cJSON_CreateTrue_ex(pDes,nLine);
	}
	else if (nType == Common_Json_Type_Number)
	{
		pNew = Common_cJSON_CreateNumber_ex(nIntValue,pDes,nLine);
	}
	else if (nType == Common_Json_Type_Double)
	{
		pNew = Common_cJSON_CreateDouble_ex(fFloat,pDes,nLine);
	}
	else if (nType == Common_Json_Type_String)
	{
		if (pStringValue != NULL)
		{
			pNew = Common_cJSON_CreateString_ex(pStringValue,pDes,nLine);
		}

	}
	else if (nType == Common_Json_Type_Array)
	{
		pNew = Common_cJSON_CreateArray_ex(pDes,nLine);
	}
	else if (nType == Common_Json_Type_Object)
	{
		pNew = Common_cJSON_CreateObject_ex(pDes,nLine);
	}
	if (pNew)
	{
		if (szObjectName != NULL)
		{
			pNew->string = Common_StrDup((S8 *)szObjectName,pDes,nLine);
			if (pNew->string == NULL)
			{
				Common_cJSON_Delete_ex(pNew,pDes,nLine);
				pNew = NULL;
			}
		}

	}

	return pNew;

}

S32 Common_Json_SetItemExtData(cJSON_Struct *pJson,void *pData,S32 nDataSize)
{
	return Common_cJSON_SetItemExtData((Common_cJSON_T *)pJson,pData,nDataSize);
}
void * Common_Json_GetItemExtData(cJSON_Struct *pJson,S32 *lpDataSize)
{
	return Common_cJSON_GetItemExtData((Common_cJSON_T *)pJson,lpDataSize);
}

S32 Common_Json_AddItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName,cJSON_Struct *pAddItem)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	Common_cJSON_T *pItem = NULL;
	S8 *pCurrString = NULL,*pNextString = NULL,*pszPath = NULL;
	S32 nIndex = -1,nRet = -1;
	if (pJsonRoot == NULL || pAddItem == NULL)
	{
		return -1;
	}
	pItem = (Common_cJSON_T *)pJson;
	pszPath = (S8 *)szWhichName;
	while (pszPath != NULL) 
	{
		nIndex = -1;
		pCurrString = NULL;
		pNextString = NULL;
		nRet = Common_UriOneParse(pszPath,&nIndex,&pCurrString,&pNextString);
		if (nRet == -1)
		{
			if (pCurrString)
			{
				Common_Free(pCurrString,__FUNCTION__,__LINE__);
				pCurrString = NULL;
			}
			return -1;
		}
		if (nIndex != -1)
		{
			if (pItem->type != Common_Json_Type_Array)
			{
				if (pCurrString)
				{
					Common_Free(pCurrString,__FUNCTION__,__LINE__);
					pCurrString = NULL;
				}
				return -1;
			}
			pItem = Common_cJSON_GetArrayItem(pItem,nIndex);
		}
		else if (pItem->type == Common_Json_Type_Array)
		{
			if (pNextString == NULL && nWhich >= 0)
			{
				break;
			}
			if (pCurrString)
			{
				Common_Free(pCurrString,__FUNCTION__,__LINE__);
				pCurrString = NULL;
			}
			return -1;
		}
		else
		{
			if (pNextString == NULL)
			{
				break;
			}
			pItem = Common_cJSON_GetObjectItem(pItem,pCurrString);
		}
		if (pCurrString)
		{
			Common_Free(pCurrString,__FUNCTION__,__LINE__);
			pCurrString = NULL;
		}
		if (pItem == NULL)
		{
			return -1;
		}
		pszPath = pNextString;
	};
	
	if (nWhich != -1)
	{
		Common_cJSON_T *pWhich = NULL;
		if (pItem->type != Common_Json_Type_Array)
		{
			if (pCurrString)
			{
				Common_Free(pCurrString,__FUNCTION__,__LINE__);
				pCurrString = NULL;
			}
			return -1;
		}
		if (pCurrString !=NULL)
		{
			pWhich = Common_cJSON_GetArrayItem(pItem,nWhich);
			if (pWhich == NULL)
			{
				pWhich = Common_cJSON_CreateObject();
				Common_cJSON_AddItemToObject(pWhich,pCurrString,(Common_cJSON_T *)pAddItem);
				Common_cJSON_AddItemToArrayByIndex(pItem,nWhich,(Common_cJSON_T *)pWhich);
			}
			else
			{
				Common_cJSON_AddItemToObject(pWhich,pCurrString,(Common_cJSON_T *)pAddItem);
			}
		}
		else
		{
			Common_cJSON_AddItemToArrayByIndex(pItem,nWhich,(Common_cJSON_T *)pAddItem);
		}
		
		
	}
	else
	{
		if (pItem->type == Common_Json_Type_Array)
		{
			if (pCurrString)
			{
				Common_Free(pCurrString,__FUNCTION__,__LINE__);
				pCurrString = NULL;
			}
			return -1;
		}
		Common_cJSON_AddItemToObject(pItem,pCurrString,(Common_cJSON_T *)pAddItem);
		
	}
	if (pCurrString)
	{
		Common_Free(pCurrString,__FUNCTION__,__LINE__);
		pCurrString = NULL;
	}
	return 0;
}

 S32 Common_Json_RemoveItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	Common_cJSON_T *pItem = NULL;
	if (pJsonRoot == NULL )
	{
		return -1;
	}
	if (szWhichName != NULL)
	{
		pItem = (Common_cJSON_T *)Common_Json_GetItem(pJson,-1,szWhichName);
	}
	else
	{
		pItem = pJsonRoot;
	}
	
	if (nWhich != -1)
	{
		if(pItem->type != Common_Json_Type_Array)
		{
			return -1;
		}
		Common_cJSON_DeleteItemFromArray(pItem,nWhich);
	}
	else
	{
		cJSON_Struct *pParent;
		pParent = Common_Json_GetParent((cJSON_Struct *)pItem);
		if (pParent != NULL)
		{
			Common_cJSON_DeleteItemFromObject((Common_cJSON_T *)pParent,szWhichName);
		}
		else
		{
			return -1;
		}
		
	}
	
	return 0;
}
 cJSON_Struct *Common_Json_DetachItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	Common_cJSON_T *pItem = NULL;
	if (pJsonRoot == NULL )
	{
		return NULL;
	}
	pItem = (Common_cJSON_T *)Common_Json_GetItem(pJson,-1,szWhichName);
	if (nWhich != -1)
	{
		if(pItem->type != Common_Json_Type_Array)
		{
			return NULL;
		}
		return Common_cJSON_DetachItemFromArray(pItem,nWhich);
	}
	else
	{
		cJSON_Struct *pParent;
		pParent = Common_Json_GetParent((cJSON_Struct *)pItem);
		if (pParent != NULL)
		{
			return Common_cJSON_DetachItemFromObject((Common_cJSON_T *)pParent,szWhichName);
		}

	}
	return NULL;
}
 cJSON_Struct *Common_Json_GetItem(cJSON_Struct *pJson,S32 nWhich,const S8 *szWhichName)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	Common_cJSON_T *pItem;
	S8 *pCurrString = NULL,*pNextString = NULL,*pszPath = NULL;
	S32 nIndex = -1,nRet = -1;
	if (pJsonRoot == NULL )
	{
		return NULL;
	}
	if (nWhich == -1 &&szWhichName == NULL)
	{
		return NULL;
	}
	pItem = (Common_cJSON_T *)pJson;
	pszPath = (S8 *)szWhichName;
	while (pszPath != NULL) 
	{
		nIndex = -1;
		pCurrString = NULL;
		pNextString = NULL;
		nRet = Common_UriOneParse(pszPath,&nIndex,&pCurrString,&pNextString);
		if (nRet == -1)
		{
			if (pCurrString)
			{
				Common_Free(pCurrString,__FUNCTION__,__LINE__);
			}
			return NULL;
		}
		if (nIndex != -1)
		{
			if (pItem->type != Common_Json_Type_Array)
			{
				if (pCurrString)
				{
					Common_Free(pCurrString,__FUNCTION__,__LINE__);
				}
				return NULL;
			}
			pItem = Common_cJSON_GetArrayItem(pItem,nIndex);
		}
		else if (pItem->type == Common_Json_Type_Array)
		{
			if (nWhich != -1 && pNextString == NULL)
			{
				pItem = Common_cJSON_GetArrayItem(pItem,nWhich);
				if (pItem == NULL)
				{
					if (pCurrString)
					{
						Common_Free(pCurrString,__FUNCTION__,__LINE__);
					}
					return NULL;
				}
				pItem = Common_cJSON_GetObjectItem(pItem,pCurrString);
				if (pCurrString)
				{
					Common_Free(pCurrString,__FUNCTION__,__LINE__);
				}
				return pItem;
			}
			if (pCurrString)
			{
				Common_Free(pCurrString,__FUNCTION__,__LINE__);
			}
			return NULL;
		}
		else
		{
			pItem = Common_cJSON_GetObjectItem(pItem,pCurrString);
		}
		if (pCurrString)
		{
			Common_Free(pCurrString,__FUNCTION__,__LINE__);
		}
		if (pItem == NULL)
		{
			return NULL;
		}
		pszPath = pNextString;
	};
	if (nWhich != -1)
	{
		if (pItem->type != Common_Json_Type_Array)
		{
			return NULL;
		}
		pItem = Common_cJSON_GetArrayItem(pItem,nWhich);
		if (pItem == NULL)
		{
			return NULL;
		}
	}
	return (cJSON_Struct *)pItem;
}
 cJSON_Struct *Common_Json_GetFirstChild(cJSON_Struct *pJson)
 {
	 Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	 if (pJsonRoot == NULL )
	 {
		 return NULL;
	 }
	 return pJsonRoot->child;
 }

 cJSON_Struct *Common_Json_GetLastChild(cJSON_Struct *pJson)
 {
	 Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	 if (pJsonRoot == NULL )
	 {
		 return NULL;
	 }
	 return pJsonRoot->LastChild;
 }

 cJSON_Struct *Common_Json_GetParent(cJSON_Struct *pJson)
 {
	 Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	 if (pJsonRoot == NULL )
	 {
		 return NULL;
	 }
	 return pJsonRoot->pParent;
	 
 }
 cJSON_Struct *Common_Json_GetNext(cJSON_Struct *pJson)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	if (pJsonRoot == NULL )
	{
		return NULL;
	}
	return pJsonRoot->next;
}
 cJSON_Struct *Common_Json_GetPrev(cJSON_Struct *pJson)
{
	{
		Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
		if (pJsonRoot == NULL )
		{
			return NULL;
		}
		return pJsonRoot->prev;
	}
}
 S32 Common_Json_Size(cJSON_Struct *pJson)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	if (pJsonRoot == NULL )
	{
		return 0;
	}
	if (pJsonRoot->type == Common_Json_Type_Array)
	{
		return pJsonRoot->nArrayNum;
	}
	else
	{
		return pJsonRoot->nChildNum;
	}

}

S32 Common_Json_ArraySize(cJSON_Struct *pJson)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	if (pJsonRoot == NULL )
	{
		return 0;
	}
	if (pJsonRoot->type == Common_Json_Type_Array)
	{
		return pJsonRoot->nArrayNum;
	}
	else
	{
		return 0;
	}
}

S32 Common_Json_ChildSize(cJSON_Struct *pJson)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	if (pJsonRoot == NULL )
	{
		return 0;
	}
	if (pJsonRoot->type == Common_Json_Type_Array)
	{
		return 0;
	}
	else
	{
		return pJsonRoot->nChildNum;
	}
}

 S32 Common_Json_GetAttr(cJSON_Struct *pJson,S32 *nWhich,S8 **szObjectName,S32 *nType,S8 **pStringValue,S32 *nIntValue,double *fFloat)
{
	Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;

	if (pJsonRoot == NULL )
	{
		return -1;
	}
	

	if (szObjectName)
	{
#if 1
		*szObjectName = pJsonRoot->string;
#else
		if (pJsonRoot->type == Common_Json_Type_Array)
		{
			*szObjectName = NULL;
		}
		else
		{
			*szObjectName = pJsonRoot->string;
		}
#endif
		
	}
	if (nType)
	{
		*nType = pJsonRoot->type;
	}
	if (pStringValue)
	{
		*pStringValue = pJsonRoot->valuestring;
	}
	if (nIntValue)
	{
		*nIntValue = pJsonRoot->valueint;
	}
	if (fFloat)
	{
		*fFloat = pJsonRoot->valuedouble;
	}
	return 0;

}
 S32 Common_Json_SetAttrName(cJSON_Struct *pJson,S8 *szOldPathName,S8 *szObjectName)
 {
	 Common_cJSON_T *pJsonRoot = (Common_cJSON_T*) pJson;
	 S8 *pNewString = NULL;
	 if (pJsonRoot == NULL || szObjectName == NULL)
	 {
		 return -1;
	 }
	 if (szOldPathName != NULL)
	 {
		 pJsonRoot = (Common_cJSON_T*)Common_Json_GetItem(pJson,-1,szOldPathName);
		 if (pJsonRoot == NULL)
		 {
			 return -1;
		 }
	 }
	
	 pNewString = Common_StrDup(szObjectName,__FUNCTION__,__LINE__);
	 if (pNewString == NULL)
	 {
		 return -1;
	 }
	 if (pJsonRoot->string != NULL)
	 {
		 Common_Free(pJsonRoot->string,__FUNCTION__,__LINE__);
		 pJsonRoot->string = NULL;
	 }
	 pJsonRoot->string = pNewString;
	 return 0;

 }
 cJSON_Struct * Common_Json_SetAttrValue(cJSON_Struct *pJson,S32 nWhich,const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat)
 {
	 cJSON_Struct *pNewItem = NULL;

	 if (pJson == NULL)
	 {
		 return NULL;
	 }
	 Common_Trace_T hTrace = NULL;
	 S8 *pfName = NULL,*pSyName = NULL;
	 Common_Trace_Return(&hTrace,0,&pfName,&pSyName,NULL);
	 if (hTrace == NULL)
	 {
		 pSyName = (S8 *)__FUNCTION__;
	 }
	 pNewItem = Common_Json_New(NULL,nType,pStringValue,nIntValue,fFloat);
	 if (pNewItem == NULL)
	 {
		 Common_Trace_RetFree(&hTrace);
		 return NULL;
	 }
	 if(Common_Json_AddItem(pJson,nWhich,szObjectName,pNewItem))
	 {
		 Common_Json_Delete(pNewItem);
		 Common_Trace_RetFree(&hTrace);
		 return NULL;
	 }
	 Common_Trace_RetFree(&hTrace);
	 return pNewItem;
 }
cJSON_Struct * Common_Json_SetAttrValue_ex(cJSON_Struct *pJson,S32 nWhich,const S8 *szObjectName,S32 nType,const S8 *pStringValue,S32 nIntValue,double fFloat,const S8 *pDes,S32 nLine)
 {
	 cJSON_Struct *pNewItem = NULL;

	 if (pJson == NULL)
	 {
		 return NULL;
	 }
	
	 pNewItem = Common_Json_New_ex(NULL,nType,pStringValue,nIntValue,fFloat,pDes,nLine);
	 if (pNewItem == NULL)
	 {
		 return NULL;
	 }
	 if(Common_Json_AddItem(pJson,nWhich,szObjectName,pNewItem))
	 {
		 Common_Json_Delete_ex(pNewItem,pDes,nLine);
		 return NULL;
	 }
	 return pNewItem;
 }
cJSON_Struct * Common_Json_GetAttrValue(cJSON_Struct *pJson,S32 nWhich,const S8 *szObjectName,S32 *nType,S8 **pStringValue,S32 *nIntValue,double *fFloat)
{
	Common_cJSON_T *pJsonRoot;
	cJSON_Struct *pItem;

	if (pJson == NULL)
	{
		return NULL;
	}
	
	
	pItem = Common_Json_GetItem(pJson,nWhich,szObjectName);
	if (pItem == NULL)
	{
		return NULL;
	}
	
	

	pJsonRoot = (Common_cJSON_T*) pItem;
	
	if (nType)
	{
		*nType = pJsonRoot->type;
	}
	if (pStringValue)
	{
		*pStringValue = pJsonRoot->valuestring;
	}
	if (nIntValue)
	{
		*nIntValue = pJsonRoot->valueint;
	}
	if (fFloat)
	{
		*fFloat = pJsonRoot->valuedouble;
	}
	return pItem;
}






void Common_Json_Delete(cJSON_Struct *pJson)
{
	Common_Trace_T hTrace = NULL;
	S8 *pfName = NULL,*pSyName = NULL;
	if(pJson == NULL)
	{
		return;
	}
	Common_Trace_Return(&hTrace,0,&pfName,&pSyName,NULL);
	if (hTrace == NULL)
	{
		pSyName = (S8 *)__FUNCTION__;
	}
	 Common_cJSON_Delete_ex((Common_cJSON_T *)pJson,pSyName,__LINE__);
	 Common_Trace_RetFree(&hTrace);
}
void Common_Json_Delete_ex(cJSON_Struct *pJson,const S8 *pDes,S32 nLine)
{
	return Common_cJSON_Delete_ex((Common_cJSON_T *)pJson,pDes,nLine);
}

cJSON_Struct *Common_Json_Duplicate(cJSON_Struct *item,S32 recurse)
{
	return Common_cJSON_Duplicate((Common_cJSON_T *)item,recurse);
}