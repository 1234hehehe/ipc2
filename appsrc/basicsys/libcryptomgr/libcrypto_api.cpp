#include "libcrypto_api.h"
#include "libcommon_api.h"

static char g_keyModel[]="ovfs-2017*5%9_3468abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int __inline Char2Int(char c)
{
	if (c >= '0' && c <= '9')
	{
		return c- '0';
	}
	if (c >= 'a' && c <= 'f')
	{
		return 10 + c - 'a';
	}
	if (c >= 'A' && c <= 'F')
	{
		return 10 + c - 'A';
	}
	return 0;
}
static void __inline Calc_4Char(int n,U8 *p4Out)
{
	U8 byBuf[80];
	S32 m = n + 1,nTotalSize,nPos = 0,nDstPos;
	int nNeed,nInterval,nPos1;
	nTotalSize = strlen(g_keyModel);
	nDstPos = 0;
	for (nPos = n;nPos < nTotalSize;nPos+=m)
	{
		byBuf[nDstPos] = g_keyModel[nPos];
		nDstPos++;
	}
	if (nDstPos < 4)
	{
		nNeed = 4 - nDstPos;
		nInterval = n / nNeed;
		nPos1 = 0;
		while(nDstPos < 4)
		{
			if (n - 1 - nPos1 >= 0)
			{
				byBuf[nDstPos] = g_keyModel[n - 1 - nPos1];
			}
			else
			{
				byBuf[nDstPos] = g_keyModel[nPos1];
			}
			
			nDstPos++;
			nPos1 += nInterval;
		}
	}
	nInterval = nDstPos / 4;
	p4Out[0] = byBuf[0];
	p4Out[1] = byBuf[0 + nInterval];
	p4Out[2] = byBuf[0 + nInterval * 2];
	p4Out[3] = byBuf[0 + nInterval * 3];
}


static U8 UPLOW(U8 c)
{
	U8 cc = 0;
	S32 i;
	for (i = 0; i < 8;i++)
	{
		cc |= ((c >> i) & 1) << (7 - i);
	}
	return cc;
}
S8* ovfs_auth_DecryptString(S8 *pOrgString,S32 *lpEncryptType,S8 *pNewString,S32 nNewSize)
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
					else if (nEncryptType == 2)
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

								for (i = 0; i < nNewOrgLen;i++)
								{
									pNewOrgString[i] = UPLOW(pNewOrgString[i]);
								}
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


S8* ovfs_auth_EncryptString(S32 nMethod,S8 *pOrgString,S8 *pNewString,S32 nNewSize)
{
	S8 *pNewReturnString = NULL;
	S32 i,nOrgLen;
	if (pOrgString == NULL)
	{
		return pNewReturnString;
	}
	nOrgLen = strlen(pOrgString);
	// 检查是否合法
#if 0
	for (i = 0; i < nOrgLen; i++)
	{
		if (!(
			(pOrgString[i] >= 'A' && pOrgString[i] <= 'Z') ||
			(pOrgString[i] >= 'a' && pOrgString[i] <= 'z') ||
			(pOrgString[i] >= '0' && pOrgString[i] <= '9') ||
			pOrgString[i] == '_' ||
			pOrgString[i] == '-' ||
			pOrgString[i] == ':' ||
			pOrgString[i] == ','||
			pOrgString[i] == '\"' ||
			pOrgString[i] == '\''||
			pOrgString[i] == '{'||
			pOrgString[i] == '}'||
			pOrgString[i] == '['||
			pOrgString[i] == ']'||
			pOrgString[i] == ';')
			)
		{
			return NULL;
		}
	}
#endif
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
		pNewReturnString = (S8 *)Common_Malloc(nNew64Len + 1 + 8 + 16 + 16,0,__FUNCTION__,__LINE__);
		if (pNewReturnString != NULL)
		{
			nNewLen = sprintf(pNewReturnString,"$%d$%d$$%s",nNew64Len,nMethod,pBase64);
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
	else if (nMethod == 2)
	{ // 加密 $<n chars length>$<n chars type>$[EncryptKey]$do_type(base64(pwd))
		S8 *pBase64;
		S32 nNew64Len = 0,nNewLen;
		S8 cTmp;
		S8 *pStep1;
		pStep1 = (S8 *)Common_Malloc(nOrgLen,0,__FUNCTION__,__LINE__);
		if (pStep1 == NULL)
		{
			return NULL;
		}
		for (i = 0; i < nOrgLen;i++)
		{
			pStep1[i] = UPLOW(pOrgString[i]);
		}
		
		pBase64 = Common_Base64_Encode(pStep1,nOrgLen,(U32 *)&nNew64Len);
		Common_Free(pStep1,__FUNCTION__,__LINE__);
		pStep1 = NULL;
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
		pNewReturnString = (S8 *)Common_Malloc(nNew64Len + 1 + 8 + 16 + 16,0,__FUNCTION__,__LINE__);
		if (pNewReturnString != NULL)
		{
			nNewLen = sprintf(pNewReturnString,"$%d$%d$$%s",nNew64Len,nMethod,pBase64);
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
char * ovfs_Cert_Enc(char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,char *szTime,int *lpOutLen) // return char *szOutString
{
	S32 n;
	char *szCertificate = NULL;
	U8 byBuf1[65]="";
	Common_HMacSha256_T hMacSha256 = NULL;
	if (szUUID == NULL)
	{
		szUUID ="";
	}
	Calc_4Char(Char2Int(szSN[0]),byBuf1);
	Calc_4Char(Char2Int(szSN[1]),byBuf1 + 4);
	Calc_4Char(Char2Int(szSN[2]),byBuf1 + 8);
	Calc_4Char(Char2Int(szSN[3]),byBuf1 + 12);
	for (n = 0; n < 16; n++)
	{
		byBuf1[n] = UPLOW(byBuf1[n]);
	}
	Common_HMacSha256_Create(&hMacSha256,byBuf1,16);
	Common_HMacSha256_Append(hMacSha256,(U8 *)szSN,20);
	Common_HMacSha256_Append(hMacSha256,(U8 *)szUUID,strlen(szUUID)+1);
	Common_HMacSha256_Append(hMacSha256,(U8 *)szHardware,strlen(szHardware)+1);
	
	Common_HMacSha256_Append(hMacSha256,(U8 *)szTime,strlen(szTime) + 1);
	
	Common_HMacSha256_Finish(hMacSha256,NULL,byBuf1);
	Common_HMacSha256_Destroy(&hMacSha256);
	szCertificate = Common_StrDup((char *)byBuf1,__FUNCTION__,__LINE__);
	if(lpOutLen)
	{
		*lpOutLen = strlen((char *)byBuf1);
	}
	return szCertificate;

}
char * ovfs_Licence_Enc(char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,char *szTime,char **szOtherNames,char **szOtherValues,int nOtherCnt,int *lpOutLen) // return char *szOutString
{// p = sn+time+key
/*
Sn:xxx,
Time:xxx,
BadBoy:1,
MainLevel:0,// 维护等级，标记是否支持修复，人工识别正版使用
*/
	char *szNonce;
	char *szCreateTime;
	char *szCertificate = NULL;
	//Common_Time_T tComTime;
	//char szTime[32];
	int n;
	cJSON_Struct *pJson = NULL;
	char *pString = NULL,*pAuthString = NULL;
	int nStringLen = 0;

	if (szSN != NULL && strlen(szSN) != 20)
	{
		return NULL;
	}
	pJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (szSN != NULL)
	{
		Common_Json_SetAttrValue(pJson,-1,"SerialNumber",Common_Json_Type_String,szSN,0,0);
	}
	Common_Json_SetAttrValue(pJson,-1,"UUID",Common_Json_Type_String,szUUID,0,0);
	Common_Json_SetAttrValue(pJson,-1,"Hardware",Common_Json_Type_String,szHardware,0,0);
	//Common_GetLocalTime(&tComTime);
	//nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	Common_Json_SetAttrValue(pJson,-1,"Time",Common_Json_Type_String,szTime,0,0);
	if(szOtherNames != NULL && szOtherValues != NULL)
	{
		for(n = 0; n < nOtherCnt;n++)
		{
			if(szOtherNames[n] != NULL && szOtherValues[n] != NULL)
			{
				Common_Json_SetAttrValue(pJson,-1,szOtherNames[n],Common_Json_Type_String,szOtherValues[n],0,0);
			}
		}
	}
	
	if (szSN != NULL)
	{
		szCertificate = ovfs_Cert_Enc(szSN,szHardware,szUUID,szTime,NULL);
		if(szCertificate == NULL)
		{
			Common_Json_Delete(pJson);
			return NULL;
		}
		Common_Json_SetAttrValue(pJson,-1,"Certificate",Common_Json_Type_String,(char *)szCertificate,0,0);
	}
	
	
	pString = Common_Json_Print(pJson,&nStringLen);
	Common_Json_Delete(pJson);
	if (pString == NULL)
	{
		Common_Free(szCertificate,__FUNCTION__,__LINE__);
		szCertificate = NULL;
		return NULL;
	}
	nStringLen++;
	pAuthString = ovfs_auth_EncryptString(2,pString,NULL,0);
	if (lpOutLen != NULL)
	{
		*lpOutLen = strlen(pAuthString) + 1;
	}
	Common_Free(szCertificate,__FUNCTION__,__LINE__);
	szCertificate = NULL;
	Common_Free(pString,__FUNCTION__,__LINE__);
	pString = NULL;

	return pAuthString;
}

int ovfs_Licence_dec(char *szLicence,char **szHardware,char **szUUID,char **szSerialNumber,char **szCertificate,char **szTime,void/*cJSON_Struct*/ **pOutJson)
{
	S32 nEncryptType = 0,nRet = -1;
	S8 *szLicenceContext = NULL;
	if(szLicence == NULL)
	{
		return -1;
	}
	if(szUUID == NULL && szHardware == NULL && szSerialNumber == NULL && szCertificate == NULL && szTime == NULL)
	{
		return -1;
	}
	
						
	szLicenceContext = ovfs_auth_DecryptString(szLicence,&nEncryptType,NULL,0);
	if(szLicenceContext != NULL)
	{
		cJSON_Struct *pLicenceJson = Common_Json_Parse(szLicenceContext,NULL,NULL);
		if(pLicenceJson != NULL)
		{
			/*
				{
					"UUID":"","SerialNumber":"","Certificate":"","Time":""
				}
			*/
			S8 *szNewUUID = NULL,*szNewSerialNumber = NULL,*szNewCertificate=NULL,*szNewTime = NULL,*szNewHardware = NULL;
			Common_Json_GetAttrValue(pLicenceJson,-1,"UUID",NULL,&szNewUUID,NULL,NULL);
			Common_Json_GetAttrValue(pLicenceJson,-1,"SerialNumber",NULL,&szNewSerialNumber,NULL,NULL);
			Common_Json_GetAttrValue(pLicenceJson,-1,"Certificate",NULL,&szNewCertificate,NULL,NULL);
			Common_Json_GetAttrValue(pLicenceJson,-1,"Time",NULL,&szNewTime,NULL,NULL);
			Common_Json_GetAttrValue(pLicenceJson,-1,"Hardware",NULL,&szNewHardware,NULL,NULL);
			if(szNewSerialNumber != NULL && szNewCertificate != NULL)
			{
					// 进一步检查
					S8 *szNewCertficate1 = NULL;
					S32 nNewCertLen1 = 0;
					szNewCertficate1 = ovfs_Cert_Enc(szNewSerialNumber,szNewHardware,szNewUUID,szNewTime,&nNewCertLen1);
					//printf("[%s][%s][%s][%s][%s] ->[%s]\n",szNewSerialNumber,szNewHardware,szNewUUID,szNewTime,szNewCertificate,szNewCertficate1);
					if(0 == Common_StrCmp(szNewCertficate1,szNewCertificate))
					{
						// 成功
						S32 nSnLen = strlen(szNewSerialNumber);
						if(nSnLen < 32)
						{
							if(szUUID != NULL)
							{
								*szUUID = Common_StrDup(szNewUUID,__FUNCTION__,__LINE__);
							}
							if(szSerialNumber != NULL)
							{
								*szSerialNumber = Common_StrDup(szNewSerialNumber,__FUNCTION__,__LINE__);
							}
							if(szCertificate != NULL)
							{
								*szCertificate = Common_StrDup(szNewCertificate,__FUNCTION__,__LINE__);
							}
							if(szTime != NULL)
							{
								*szTime = Common_StrDup(szNewTime,__FUNCTION__,__LINE__);
							}
							if(szHardware != NULL)
							{
								*szHardware = Common_StrDup(szNewHardware,__FUNCTION__,__LINE__);
							}
							nRet = 0;
							if (pOutJson != NULL)
							{
								*pOutJson = pLicenceJson;
								pLicenceJson = NULL;
							}
						}
					}
					Common_Free(szNewCertficate1,__FUNCTION__,__LINE__);
					szNewCertficate1= NULL;
				
			}
			else if (szNewUUID != NULL && szNewCertificate == NULL)
			{
				if(szUUID != NULL)
				{
					*szUUID = Common_StrDup(szNewUUID,__FUNCTION__,__LINE__);
				}
				if(szTime != NULL)
				{
					*szTime = Common_StrDup(szNewTime,__FUNCTION__,__LINE__);
				}
				if(szHardware != NULL)
				{
					*szHardware = Common_StrDup(szNewHardware,__FUNCTION__,__LINE__);
				}
				nRet = 0;
				if (pOutJson != NULL)
				{
					*pOutJson = pLicenceJson;
					pLicenceJson = NULL;
				}
			}
			if (pLicenceJson != NULL)
			{
				Common_Json_Delete(pLicenceJson);
				pLicenceJson= NULL;
			}
			
		}
		Common_Free(szLicenceContext,__FUNCTION__,__LINE__);
		szLicenceContext= NULL;
	}
	return nRet;
}


char * CryptoMgr_Dec(char *pOrgString,char *pNewString,int nNewSize)
{
	char *pDecString = NULL;
	if (pOrgString == NULL)
	{
		return NULL;
	}
	return ovfs_auth_DecryptString(pOrgString,NULL,pNewString,nNewSize);
}
