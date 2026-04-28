#include "libcommon_api.h"
#include "update_struct.h"
#include "libupdate_api.h"
#include "libcrypto_api.h"
#include "update_version.h"
#include "update_broadcast.h"

/*
烧录与设备交互资源
Rest（Broadcast/HTTP）:
// 搜索设备
Uri:/Update/Discovery
TransType:Broadcast port :10008
Method:Get
Data:
{
	Status:Activated/Unactivated/Illegal 
	SN:"xxx...xxxx"
	Port:"xx"
	Ipv4:"xxx.xxx.xxx.xxx"
	Netmask:"xxx.xxx.xxx.xxx"
	Mac:"xx:xx:xx:xx:xx:xx"
}
// 修改IP
Uri:/Update/NetConfig
TransType:Broadcast port :10008
Method:Put
InData:
{
	Ipv4:"xxx.xxx.xxx.xxx"
	Netmask:"xxx.xxx.xxx.xxx",
	Mac:"xx:xx:xx:xx:xx:xx"
}

// 烧写授权信息
Uri:/Update/Crypto/Burn
TransType:http
Method:Put
InData:
{
	Crypto:xxx
}


// 授权信息向中心服务器上报
Uri:/Upload/CryptoInfo
Method:Put
InData:
{
	Status:Activated/Unactivated/Illegal 
	SN:"xxx...xxxx"
	Port:"xx"
	Ipv4:"xxx.xxx.xxx.xxx"
	Netmask:"xxx.xxx.xxx.xxx"
	Mac:"xx:xx:xx:xx:xx:xx"
}

*/
extern S32 LoadVersion(int bReal);
extern char *Update_GetUUID();
extern char *Update_GetMac();

void update_crypto_AuthCheck(S8 *szHelloUUID, S8 *szSerialNumber)
{
    S32 ret = -1;
	S32 bNeedErase = 0;
    S8 acHelloUUID[64 + 1] = {0};
    S8 acSerialNum[256 + 1] = {0};

    if (szHelloUUID == NULL || szSerialNumber == NULL)
	{
		return;
	}
	//UPDATE_DEBUG("Auth crypto Hello <%s,%s>!\n",szHelloUUID,szSerialNumber);

    memset((void *)acHelloUUID, 0x00, sizeof(acHelloUUID));
    ret = update_broadCast_GetHelloUUID(acHelloUUID, sizeof(acHelloUUID));
    if (0 != ret)
    {
        LOGE("Get hello uuid error.\n");
        return;
    }

    ret = udpate_version_GetSN(acSerialNum, sizeof(acSerialNum));
    if (0 != ret)
    {
        LOGE("Get device SN error.\n");
        return;
    }

	if (acHelloUUID[0] != 0)
	{
		if (0 == Common_StrCmp(acHelloUUID, szHelloUUID))
		{
			//g_szHelloUUID[0] = 0;
			return;
		}

		if (0 == acSerialNum[0])
		{
			return;
		}

        if (0 == Common_StrCmp(acSerialNum, szSerialNumber))
		{
			bNeedErase = 1;
		}
	}

    if (bNeedErase)
	{
		cJSON_Struct *pCoreInParam = NULL,*pCoreOutParam = NULL;
		UPDATE_INFO("Auth crypto erase!\n");
		pCoreInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pCoreInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pCoreInParam,-1,"Header/Uri",Common_Json_Type_String,"/Core/VersionEraseAuth",0,0);
		Common_Json_SetAttrValue(pCoreInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
		Update_Tcp_Require("127.0.0.1",10009,pCoreInParam,&pCoreOutParam,6000);
		if (pCoreOutParam != NULL)
		{
			S32 nCode = -1;
			Common_Json_GetAttrValue(pCoreOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
			if (nCode == 0 || nCode == 200)
			{
                DeviceVersion_S *pstVersionHandle  = update_version_GetHandle();
                (void)update_version_LoadVer(pstVersionHandle, 1, LOADVER_FROM_CORE);
			}
			Common_Json_Delete(pCoreOutParam);
			pCoreOutParam = NULL;
		}
		Common_Json_Delete(pCoreInParam);
		pCoreInParam = NULL;
	}
}

extern int bstart;
S32 update_crypto_CallFunctions(UpdateClientInfo_T *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
	S32 nRet = -1;
	S8 *szUri = NULL;
	S8 *szMethod = NULL;
	cJSON_Struct *pOutJson = NULL;
	S32 bAuthOk = 0,nLastNonceGenTime = 0;
	UpdateMgr_T *pMgr = NULL;
	S8 szOrgNonce[20 + 1] = {0};
    S8 *pNewNonce  = NULL;
    S8 *pNewOpaque = NULL;
	S8 szOrgOpaque[8 + 1] = {0};
    S8 szLocalUUID[256 + 1] = {0};
	S32 nCurrSec = 0;
	char *pKey = NULL;
	char szKeyNew[64] = {0};
	S8 *szLicence = NULL,*szKey = NULL,*szTime = NULL;
	S32 bBurnOk = 0,bIsSelf = 0;

	if (pClientInfo == NULL)
	{
		return -1;
	}

	//Common_Json_StandardPrint(pInParams,"In Params < ",">\n",NULL);

	pMgr = (UpdateMgr_T *)pClientInfo->pMgr;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&szUri,NULL,NULL);
	if(0 != Common_StrniCmp("/Update/Crypto",szUri,strlen("/Update/Crypto")))
	{
		return -1;
	}
	if(bstart)
	{
		return 0;
	}
	Common_Json_GetAttrValue(pInParams,-1,"Header/Method",NULL,&szMethod,NULL,NULL);
	if (0 == Common_StriCmp("/Update/Crypto/Hello",szUri) &&
		0 == Common_StriCmp("Put",szMethod))
	{
	    // Seq:,SerialNumber,Status,mac,uuid,
		char *szHelloUUID = NULL,*szSerialNumber = NULL;
		Common_Json_GetAttrValue(pInParams,-1,"Data/HelloUUID",NULL,&szHelloUUID,NULL,NULL);
		Common_Json_GetAttrValue(pInParams,-1,"Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);
		update_crypto_AuthCheck(szHelloUUID,szSerialNumber);
		return 0;
	}

    // 验证
	Common_GetSystemCount(&nCurrSec,NULL);

    memset((void *)szLocalUUID, 0x00, sizeof(szLocalUUID));
    nRet = update_version_GetUUID(szLocalUUID, sizeof(szLocalUUID));
	if ((0 == szLocalUUID[0]) || (nRet != 0))
	{
	    LOGI("Get UUID failed.\n");
		nRet = update_version_GetMac(szLocalUUID, sizeof(szLocalUUID));
	}

    if ((0 == szLocalUUID[0]) || (nRet != 0))
    {
        LOGW("The UUID is ZERO.\n");
        //return -1;
    }

	Common_Json_GetAttrValue(pInParams,-1,"Data/Time",NULL,&szTime,NULL,NULL);
	Common_Json_GetAttrValue(pInParams,-1,"Data/Key",NULL,&szKey,NULL,NULL);
	if (szKey != NULL && szTime != NULL && szLocalUUID[0] != 0)
	{
		sprintf(szKeyNew, "%s:(%s)", szLocalUUID, szTime);
        LOGD("LocalUUID: %s.\n", szLocalUUID);

        pKey = ovfs_auth_EncryptString(2, szKeyNew, NULL, 0);
		if (pKey != NULL)
		{
		    LOGD("pKey=%s, szKey=%s\n", pKey, szKey);
			if (0 == Common_StrCmp(pKey,szKey))
			{
				bIsSelf = 1;
			}
			Common_Free(pKey,__FUNCTION__,__LINE__);
			pKey = NULL;
		}
	}

    if (!bIsSelf)
	{
	    LOGE("bIsSelf is NULL.\n"); 
		return -1;
	}

	Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Nonce",NULL,&pNewNonce,NULL,NULL);
	Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Opaque",NULL,&pNewOpaque,NULL,NULL);
	if ((pNewNonce != NULL) && (pNewOpaque != NULL) 
        && (strlen(pNewNonce) > 2) && (pNewNonce[0] >= '0') 
        && (pNewNonce[0] <= '9') && (pNewNonce[1] >= '0') && (pNewNonce[1] <= '9'))
	{
		int nDigestIdx = -1;
		nDigestIdx = (pNewNonce[0] - '0') * 10 + pNewNonce[1] - '0';
		if (nDigestIdx < MAX_LICENCESERVER_DIGEST_NUM)
		{
			Common_Lock(pMgr->tDigestLock);
			if (strcmp(pMgr->tDigest[nDigestIdx].szNonce,pNewNonce) == 0 &&
				strcmp(pMgr->tDigest[nDigestIdx].szOpaque,pNewOpaque) == 0)
			{
				nLastNonceGenTime = pMgr->tDigest[nDigestIdx].nLastNonceGenTime;
				strcpy(szOrgNonce,pMgr->tDigest[nDigestIdx].szNonce);
				strcpy(szOrgOpaque,pMgr->tDigest[nDigestIdx].szOpaque);
				// 清除
				pMgr->tDigest[nDigestIdx].szNonce[0] = 0;
				pMgr->nDigestCount--;
				if (pMgr->nDigestCount < 0)
				{
					pMgr->nDigestCount = 0;
				}
			}
			Common_UnLock(pMgr->tDigestLock);
			//printf("[%s.%d] time = %x <%s><%s>\n",__FUNCTION__,__LINE__,nLastNonceGenTime,szOrgNonce,szOrgOpaque);
		}

	}

    if(nCurrSec < nLastNonceGenTime + 30 &&
		szOrgOpaque[0] !=0 &&
		szOrgNonce[0] != 0&&
		Common_Json_GetItem(pInParams,-1,"/Header/Auth"))
	{
		S32 nAuthMethod = -1;
		S8 *szStringValue = NULL;
		Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Method",NULL,NULL,&nAuthMethod,NULL);
		if (nAuthMethod == 2)
		{// digest
			// 用户名
			szStringValue = NULL;
			Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/UserName",NULL,&szStringValue,NULL,NULL);
			if (szStringValue != NULL)
			{
				if (0 == Common_StrCmp(szStringValue,"Admin"))
				{
					// 用户名 ok
					S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
						*szCnonce=NULL,*szAuthUri=NULL,*szResponse=NULL,*szNc = NULL;
					S8 szPassword[16]={0};// "J5xv7_I2"
					snprintf(szPassword, sizeof(szPassword), "J5xv7_I2");
					S8 szMyResponse[32+1] = {0},szHA1[32+1] = {0};

					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Qop",NULL,&szQop,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Cnonce",NULL,&szCnonce,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Uri",NULL,&szAuthUri,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Response",NULL,&szResponse,NULL,NULL);
					Common_Json_GetAttrValue(pInParams,-1,"/Header/Auth/Digest/Nc",NULL,&szNc,NULL,NULL);
					if (szRealm != NULL && 
						0 == Common_StrCmp(szRealm,"Burn@Update.x") &&
						szNonce != NULL &&
						0 == Common_StrCmp(szNonce,szOrgNonce) &&
						szOpaque != NULL &&
						0 == Common_StrCmp(szOpaque,szOrgOpaque))
					{
						szPassword[2] = szOrgNonce[7]; 
						szPassword[4] = szOrgNonce[11];
						szPassword[6] = szOrgNonce[19]; 
						//LOGI("pwd:%s \n",szPassword);
						if (szQop == NULL)
						{
							szQop = "";
						}
						if (szOpaque == NULL)
						{
							szOpaque = "";
						}
						if (szNc == NULL)
						{
							szNc = "";
						}
						if (szCnonce == NULL)
						{
							szCnonce = "";
						}
						Common_Digest_CalcHA1((S8*)"", "Admin", szRealm, szPassword, szNonce, szCnonce, szHA1);
						Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCnonce, szQop, szMethod, szAuthUri, (S8*)"", szMyResponse);
						if (0 == Common_StrCmp(szResponse,szMyResponse))
						{
							bAuthOk = 1;
						}
					}


				}
			}
		}

	}
	if (!bAuthOk)
	{
    	int nd = 0;
		int nDigestIdx = -1;
		S32 nCurrSec = 0;
		Common_GetSystemCount(&nCurrSec,NULL);
		Common_Lock(pMgr->tDigestLock);
		for (nd = 0;nd < MAX_LICENCESERVER_DIGEST_NUM;nd++)
		{
			if(pMgr->tDigest[nd].szNonce[0] == 0)
			{
				nDigestIdx = nd;
				break;
			}
			if (pMgr->tDigest[nd].nLastNonceGenTime + 60 < nCurrSec)
			{
				nDigestIdx = nd;
				pMgr->tDigest[nd].szNonce[0] = 0;
				pMgr->nDigestCount--;
				if (pMgr->nDigestCount < 0)
				{
					pMgr->nDigestCount = 0;
				}
				break;
			}
		}
		if (nDigestIdx >= 0)
		{
			char szIdx[8];
			Common_Digest_CalcNonce(pMgr->tDigest[nDigestIdx].szNonce);
			Common_Digest_CalcOpaque(pMgr->tDigest[nDigestIdx].szOpaque);
			sprintf(szIdx,"%02d",nDigestIdx);
			pMgr->tDigest[nDigestIdx].szNonce[0] = szIdx[0];
			pMgr->tDigest[nDigestIdx].szNonce[1] = szIdx[1];
			pMgr->tDigest[nDigestIdx].nLastNonceGenTime = nCurrSec; 
			strcpy(szOrgNonce,pMgr->tDigest[nDigestIdx].szNonce);
			strcpy(szOrgOpaque,pMgr->tDigest[nDigestIdx].szOpaque);
			pMgr->nDigestCount++;
		}
		Common_UnLock(pMgr->tDigestLock);
		//printf("[%s.%d] nDigestIdx = %d time = %x <%s><%s>\n",__FUNCTION__,__LINE__,nDigestIdx,nLastNonceGenTime,szOrgNonce,szOrgOpaque);
		if (nDigestIdx == -1)
		{
			if(pOutParams != NULL)
			{
				pOutJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if (pOutJson != NULL)
				{
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Code",Common_Json_Type_Number,NULL,Common_ResResponceStatusCode_InternalServerError,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Describe",Common_Json_Type_String,"Internal Server Error",0,0,__FUNCTION__,__LINE__);
					if(szKey != NULL)
					{
						Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
						Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data/Key",Common_Json_Type_String,szKey,0,0,__FUNCTION__,__LINE__);
						Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data/UUID",Common_Json_Type_String,szLocalUUID,0,0,__FUNCTION__,__LINE__);

					}
					*pOutParams = pOutJson;
				}
			}

			return 0;
		}
		if (pOutParams != NULL)
		{
			pOutJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pOutJson != NULL)
			{
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Code",Common_Json_Type_Number,NULL,401,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Describe",Common_Json_Type_String,"Unauthorized",0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Auth",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Auth/Method",Common_Json_Type_Number,NULL,2,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Auth/Digest/Realm",Common_Json_Type_String,"Burn@Update.x",0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Auth/Digest/Nonce",Common_Json_Type_String,szOrgNonce,0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Auth/Digest/Opaque",Common_Json_Type_String,szOrgOpaque,0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Connection",Common_Json_Type_String,"Keep-Alive",0,0,__FUNCTION__,__LINE__);
				Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/SendType",Common_Json_Type_String,"Broadcast",0,0,__FUNCTION__,__LINE__);
				if(szKey != NULL)
				{
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data/Key",Common_Json_Type_String,szKey,0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data/UUID",Common_Json_Type_String,szLocalUUID,0,0,__FUNCTION__,__LINE__);

				}
				//Common_Json_StandardPrint(pOutJson,"out <",">\n",NULL);

				*pOutParams = pOutJson;
			}
		}
		return 0;
	}
	// 验证通过

	if (0 == Common_StriCmp("/Update/Crypto/Burn",szUri) &&
		0 == Common_StriCmp("Put",szMethod))
	{
		int bCanBurn = 0;
		Common_Json_GetAttrValue(pInParams,-1,"Data/Crypto",NULL,&szLicence,NULL,NULL);
		if (szLicence != NULL && szKey != NULL && szTime != NULL/* && Update_GetUUID() != NULL*/)
		{
			bCanBurn = 1;
		}

		// 开始烧录
		if(bCanBurn)
		{
			cJSON_Struct *pCoreInParam = NULL,*pCoreOutParam = NULL;
			pCoreInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pCoreInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pCoreInParam,-1,"Header/Uri",Common_Json_Type_String,"/Core/VersionAuth",0,0);
			Common_Json_SetAttrValue(pCoreInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
			Common_Json_SetAttrValue(pCoreInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pCoreInParam,-1,"Data/Licence",Common_Json_Type_String,szLicence,0,0);
			Update_Tcp_Require("127.0.0.1",10009,pCoreInParam,&pCoreOutParam,6000);
			if (pCoreOutParam != NULL)
			{
				S32 nCode = -1;
                //Common_Json_StandardPrint(pCoreOutParam,"out<",">\n",NULL);
				Common_Json_GetAttrValue(pCoreOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
				if (nCode == 0 || nCode == 200)
				{
					bBurnOk = 1;

                    DeviceVersion_S *pstVersionHandle  = update_version_GetHandle();
                    (void)update_version_LoadVer(pstVersionHandle, 1, LOADVER_FROM_CORE);

                    LOGD("stUpdateVersion.szUUID =%s, szSerialNumber =%s.\n", 
                        pstVersionHandle->stUpdateVersion.szUUID, pstVersionHandle->stUpdateVersion.szSerialNumber);
				}
				Common_Json_Delete(pCoreOutParam);
				pCoreOutParam = NULL;
			}
			Common_Json_Delete(pCoreInParam);
			pCoreInParam = NULL;
			// 烧录成功
			if (pOutParams != NULL)
			{
				pOutJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				if (pOutJson != NULL)
				{
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Code",Common_Json_Type_Number,NULL,bBurnOk?200:Common_ResResponceStatusCode_NotAcceptable,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/Describe",Common_Json_Type_String,bBurnOk?"Burn OK":"Burn Failed",0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Header/SendType",Common_Json_Type_String,"Broadcast",0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data",Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data/Key",Common_Json_Type_String,szKey,0,0,__FUNCTION__,__LINE__);
					Common_Json_SetAttrValue_ex(pOutJson,-1,(S8 *)"Data/UUID",Common_Json_Type_String,szLocalUUID,0,0,__FUNCTION__,__LINE__);
					*pOutParams = pOutJson;
				}
			}
		}
	}

	return 0;
}
