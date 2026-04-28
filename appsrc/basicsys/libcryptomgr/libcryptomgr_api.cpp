#include "libcryptomgr_api.h"
#include "libcommon_api.h"
#include "libcrypto_api.h"
#include "libupdate_api.h"
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
Uri:/Update/ConfigNet
TransType:Broadcast port :10008
Method:Put
InData:
{
	Ipv4:"xxx.xxx.xxx.xxx"
	Netmask:"xxx.xxx.xxx.xxx",
	Mac:"xx:xx:xx:xx:xx:xx"
}
//先获取Nonce
Uri:/Update/Crypto/GenNonce
TransType:http
Method:Get
Data:
{
	Nonce:xxx
}
// 再烧写授权信息
Uri:/Update/Crypto/Burn
TransType:http
Method:Put
InData:
{
	Crypto:xxx
}


*/
typedef struct _tagLibCryptoMgr
{
	Common_Lock_T hLock;
	S32 bInit;
}LibCryptoMgr_T;
static LibCryptoMgr_T g_tCryptoMgr;
int CryptoMgr_Init()
{
	memset(&g_tCryptoMgr,0,sizeof(g_tCryptoMgr));
	Common_Lock_Create(&g_tCryptoMgr.hLock,"CryptoMgrInit");
	g_tCryptoMgr.bInit = 1;
	return 0;
}

int CryptoMgr_UnInit()
{
	if (!g_tCryptoMgr.bInit)
	{
		return 0;
	}
	Common_Lock_Destroy(&g_tCryptoMgr.hLock);
	return 0;
}



/*
	szSN:由中心服务器生成的序列号
	szNonce:由设备产生的
	szCreateTime:SN产生的时间格式如"2017-05-09T09:30:00Z"
	lpOutLen:输出结果的长度，包含'\0'
	返回值：成功则为生成的密文结果,不使用时，需要用CryptoMgr_Free释放
*/


char * CryptoMgr_Enc(char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,int *lpOutLen) // return char *szOutString
{// p = sn+time+key
/*
Sn:xxx,
Time:xxx,
BadBoy:1,
MainLevel:0,// 维护等级，标记是否支持修复，人工识别正版使用
*/
	Common_Time_T tComTime;
	char szTime[32];
	int n,nTimeLen = 0;

	Common_GetLocalTime(&tComTime);
	nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	return ovfs_Licence_Enc(szSN,szHardware,szUUID,szTime,NULL,NULL,0,lpOutLen);

}
char * CryptoMgr_BadBoy(int nLevel/* = 0*/,char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,int *lpOutLen)
{
	char *szArray[3];
	char *szArrayValue[3],szLevel[32];
    int nArrayCnt = 2;
	Common_Time_T tComTime;
	char szTime[32];
	int n,nTimeLen = 0;

	Common_GetLocalTime(&tComTime);
	nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	szArray[0] = "BadBoy";
	szArray[1] = "MainLevel";
	szArrayValue[0] = "1";
	sprintf(szLevel,"%d",nLevel);
	szArrayValue[1] = szLevel;
	return ovfs_Licence_Enc(szSN,szHardware,szUUID,szTime,szArray,szArrayValue,nArrayCnt,lpOutLen);
	
}

int CryptoMgr_BurnAuth(char *szUserName,char *szPassword,char *szNonce/*20 + 1*/,char *szRealm,char *szCNonce,char *szNc,char *szQop,char *szMethod/* = "PUT"*/,char *szUri,char *szOutResponse/* [32 + 1]*/)
{
	S8 szInterPassword[16]="J5xv7_I2";// "J5xv7_I2"
	S8 szMyResponse[32+1] = {0},szHA1[32+1] = {0};
	if (szUserName == NULL)
	{
		szUserName = "Admin";
	}
	if (szNonce == NULL || szOutResponse == NULL)
	{
		return -1;
	}
	if(20 != strlen(szNonce))
	{
		return -1;
	}
	
	if (Common_StrCmp(szUserName,"Admin"))
	{
		return -1;
	}
	if (szPassword == NULL)
	{
		szPassword = szInterPassword;
	}
	else
	{
		if (strlen(szPassword) < 8)
		{
			return -1;
		}
	}
	if (szQop == NULL)
	{
		szQop = "";
	}
	
	if (szNc == NULL)
	{
		szNc = "";
	}
	if (szCNonce == NULL)
	{
		szCNonce = "";
	}

	if (szMethod == NULL)
	{
		szMethod = "PUT";
	}
	szInterPassword[2] =szNonce[7]; 
	szInterPassword[4] =szNonce[11];
	szInterPassword[6] =szNonce[19]; 
	printf("%s",szInterPassword);

	Common_Digest_CalcHA1((S8*)"", "Admin", szRealm, szPassword, szNonce, szCNonce, szHA1);
	Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCNonce, szQop, szMethod, szUri, (S8*)"", szOutResponse);
	return 0;
}
static UPDATE_BROADCAST_HANDLE g_hSearchHandle = NULL;
static CRYPTO_SEARCH_LAN_CALLBACK_DEF g_fSearchCallback = NULL;
static void *g_pSearchUserData = NULL;
static S32 static_Update_Broadcast_Search_Callback_fxn(UPDATE_BROADCAST_HANDLE handle,S8 *szFromIP,S32 nFromPort,cJSON_Struct *pResult,void *pUserData)
{
	if (g_fSearchCallback != NULL)
	{
		return g_fSearchCallback(szFromIP,nFromPort,pResult,g_pSearchUserData);
	}
	return -1;
}
 int CryptoMgr_Lan_StartSearch(int nSearchPort/*=10008*/,int nIntervalSec,int nCount,CRYPTO_SEARCH_LAN_CALLBACK_DEF fxn,void *pUserData)
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	int nRet = -1;
	if (g_hSearchHandle != NULL)
	{
		return -1;
	}
	pInParam = Common_Json_New_ex(NULL,Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
	if (pInParam != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Update/Discovery",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Get",0,0);
	}
	g_fSearchCallback = fxn;
	g_pSearchUserData = pUserData;
	nRet = Update_Broadcast_Start(&g_hSearchHandle,nSearchPort,pInParam,NULL,0,nCount,nIntervalSec,static_Update_Broadcast_Search_Callback_fxn,pUserData);

	Common_Json_Delete(pInParam);
	
	return nRet;
}

int CryptoMgr_Lan_StopSearch()
{
	int nRet = -1;
	if (g_hSearchHandle == NULL)
	{
		return 0;
	}

	nRet = Update_Broadcast_Stop(&g_hSearchHandle);
	return nRet;
}

typedef struct
{
	char *pKey;
	char *szUUID;
	char *szRealm;
	char *szNonce;
	char *szOpaque;
	int bBurnOk;// 0- ,1-ok,2-401
	Common_Sem_T tSem;
}Crypto_BurnStatus_T;

static S32 static_Update_Broadcast_Burn_Callback_fxn(UPDATE_BROADCAST_HANDLE handle,S8 *szFromIP,S32 nFromPort,cJSON_Struct *pResult,void *pUserData)
{
	Crypto_BurnStatus_T *pStatus = (Crypto_BurnStatus_T *)pUserData;
	char *pKey = NULL,*szUUID = NULL;
	int nCode = -1;
	if (pStatus == NULL || pResult == NULL)
	{
		return 0;
	}
	if (pStatus->bBurnOk == 0)
	{
	
		Common_Json_GetAttrValue(pResult,-1,"Header/Code",NULL,NULL,&nCode,NULL);
		Common_Json_GetAttrValue(pResult,-1,"Data/Key",NULL,&pKey,NULL,NULL);
		Common_Json_GetAttrValue(pResult,-1,"Data/UUID",NULL,&szUUID,NULL,NULL);
		if (pKey != NULL)
		{
			if (0 == Common_StrCmp(pKey,pStatus->pKey))
			{
				if (nCode == 0 || nCode == 200)
				{
					pStatus->bBurnOk = 1;
				}
				else if (nCode == 401)
				{
					char *szRealm = NULL,*szNonce=NULL,*szOpaque=NULL;
					Common_Json_GetAttrValue(pResult,-1,"Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
					Common_Json_GetAttrValue(pResult,-1,"Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
					Common_Json_GetAttrValue(pResult,-1,"Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
					pStatus->szRealm = Common_StrDup(szRealm,__FUNCTION__,__LINE__);
					pStatus->szNonce = Common_StrDup(szNonce,__FUNCTION__,__LINE__);
					pStatus->szOpaque = Common_StrDup(szOpaque,__FUNCTION__,__LINE__);
					pStatus->bBurnOk = 2;
				}
				
				Common_Sem_Post(pStatus->tSem);
			}
		}
	}
	

	return 0;
}
int CryptoMgr_Lan_Broadcast_Burn(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szMac,char *szUUID,char *szCryptoInfo) // return char *szOutString
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	int nRet = -1;
	char *szNonce = NULL;
	char *szCreateTime = NULL;
	Common_Time_T tComTime;
	char szTime[32];
	char szKey[64],*pKey = NULL;
	int n,nTimeLen = 0;
	UPDATE_BROADCAST_HANDLE hBurnHandle = NULL;
	Crypto_BurnStatus_T tStatus;
	memset(&tStatus,0,sizeof(tStatus));

	Common_GetLocalTime(&tComTime);
	nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	pInParam = Common_Json_New_ex(NULL,Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
	if (pInParam != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Update/Crypto/Burn",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Crypto",Common_Json_Type_String,szCryptoInfo,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Time",Common_Json_Type_String,szTime,0,0);
		if (szUUID == NULL || szUUID[0] == 0)
		{
			char szDealMac[32];
			int x = 0;
			memset(szDealMac,0,sizeof(szDealMac));
			if (szMac != NULL)
			{
				for (int i =0;i<strlen(szMac);i++)
				{
					if (szMac[i] != ':')
					{
						szDealMac[x] = szMac[i];
						x++;
					}
				}
			}
			
			sprintf(szKey,"%s:(%s)",szDealMac,szTime);
		}
		else
		{
			 sprintf(szKey,"%s:(%s)",szUUID,szTime);
		}
		
		pKey = ovfs_auth_EncryptString(2,szKey,NULL,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Key",Common_Json_Type_String,pKey,0,0);

	
		tStatus.pKey = pKey;
		tStatus.szUUID = szUUID;
		Common_Sem_Create(&tStatus.tSem,0,1,NULL);
		
		nRet = Update_Broadcast_Start(&hBurnHandle,nPort,pInParam,NULL,0,1,1,static_Update_Broadcast_Burn_Callback_fxn,&tStatus);
		if (!nRet)
		{
			int nTime = 0;
			
			if(0 == Common_Sem_TryPend(tStatus.tSem,6000))
			{
				// 
				if (tStatus.bBurnOk)
				{// 成功
		
				}
			}
			
		}
		Update_Broadcast_Stop(&hBurnHandle);
		Common_Sem_Destroy(&tStatus.tSem);
		int nTest = 3;
		do{
		if (tStatus.bBurnOk != 1)
		{
			S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
				*szCnonce=NULL,*szAuthUri=NULL,*szNc = NULL,*szMethod = NULL;
			S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
			S8 szNewPassword[16]="J5xv7_I2";// "J5xv7_I2"
			tStatus.bBurnOk = 0;
			szRealm = tStatus.szRealm;
			szNonce = tStatus.szNonce;
			szOpaque = tStatus.szOpaque;
			szAuthUri = "/Update/Crypto/Burn";
			szMethod = "Put";
			szUserName = "Admin";
			
			if (szRealm != NULL && szNonce != NULL && szOpaque != NULL)
			{
				
				if (szAuthUri == NULL)
				{
					szAuthUri = "";
				}
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
				if (szMethod == NULL)
				{
					szMethod = "Get";
				}
				szNewPassword[2] = szNonce[7]; 
				szNewPassword[4] = szNonce[11];
				szNewPassword[6] = szNonce[19]; 
				Common_Digest_CalcHA1((S8*)"", szUserName, szRealm, szNewPassword, szNonce, szCnonce, szHA1);
				Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCnonce, szQop, szMethod, szAuthUri, (S8*)"", szResponse);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Method",Common_Json_Type_Number,NULL,2,0);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/UserName",Common_Json_Type_String,szUserName,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Realm",Common_Json_Type_String,szRealm,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Qop",Common_Json_Type_String,szQop,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nonce",Common_Json_Type_String,szNonce,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Opaque",Common_Json_Type_String,szOpaque,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Cnonce",Common_Json_Type_String,szCnonce,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Uri",Common_Json_Type_String,szAuthUri,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Response",Common_Json_Type_String,szResponse,NULL,NULL);
				Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nc",Common_Json_Type_String,szNc,NULL,NULL);

			}
			Common_Free(tStatus.szRealm,__FUNCTION__,__LINE__);
			tStatus.szRealm = NULL;
			Common_Free(tStatus.szNonce,__FUNCTION__,__LINE__);
			tStatus.szNonce = NULL;
			Common_Free(tStatus.szOpaque,__FUNCTION__,__LINE__);
			tStatus.szOpaque = NULL;
			Common_Sem_Create(&tStatus.tSem,0,1,NULL);
			 Common_Json_StandardPrint(pInParam,"<",">\n",NULL);
			nRet = Update_Broadcast_Start(&hBurnHandle,nPort,pInParam,NULL,0,1,1,static_Update_Broadcast_Burn_Callback_fxn,&tStatus);
			if (!nRet)
			{
				int nTime = 0;

				if(0 == Common_Sem_TryPend(tStatus.tSem,6000))
				{
					// 
					if (tStatus.bBurnOk)
					{// 成功
					}
				}

			}
			Update_Broadcast_Stop(&hBurnHandle);
			Common_Sem_Destroy(&tStatus.tSem);
			if (tStatus.bBurnOk != 2)
			{
				break;
			}
			
		}
		nTest--;
		}while(nTest > 0);
		

		Common_Json_Delete(pInParam);;
		pInParam = NULL;

	
	}
	Common_Free(tStatus.szRealm,__FUNCTION__,__LINE__);
	tStatus.szRealm = NULL;
	Common_Free(tStatus.szNonce,__FUNCTION__,__LINE__);
	tStatus.szNonce = NULL;
	Common_Free(tStatus.szOpaque,__FUNCTION__,__LINE__);
	tStatus.szOpaque = NULL;
	if (pKey != NULL)
	{
		Common_Free(pKey,__FUNCTION__,__LINE__);
		pKey = NULL;
	}
	return tStatus.bBurnOk == 1?0:-1;
}


static S32 static_Update_Broadcast_BurnKey_Callback_fxn(UPDATE_BROADCAST_HANDLE handle,S8 *szFromIP,S32 nFromPort,cJSON_Struct *pResult,void *pUserData)
{
	Crypto_BurnStatus_T *pStatus = (Crypto_BurnStatus_T *)pUserData;
	char *pKey = NULL,*szUUID = NULL;
	int nCode = -1;
	if (pStatus == NULL || pResult == NULL)
	{
		return 0;
	}
	if (pStatus->bBurnOk == 0)
	{

		Common_Json_GetAttrValue(pResult,-1,"Header/Code",NULL,NULL,&nCode,NULL);
		Common_Json_GetAttrValue(pResult,-1,"Data/Key",NULL,&pKey,NULL,NULL);
		Common_Json_GetAttrValue(pResult,-1,"Data/UUIDReadyKey",NULL,&szUUID,NULL,NULL);
		if (pKey != NULL)
		{
			if (0 == Common_StrCmp(pKey,pStatus->pKey))
			{
				if (nCode == 0 || nCode == 200)
				{
					pStatus->bBurnOk = 1;
				}
				else if (nCode == 401)
				{
					char *szRealm = NULL,*szNonce=NULL,*szOpaque=NULL;
					Common_Json_GetAttrValue(pResult,-1,"Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
					Common_Json_GetAttrValue(pResult,-1,"Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
					Common_Json_GetAttrValue(pResult,-1,"Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
					pStatus->szRealm = Common_StrDup(szRealm,__FUNCTION__,__LINE__);
					pStatus->szNonce = Common_StrDup(szNonce,__FUNCTION__,__LINE__);
					pStatus->szOpaque = Common_StrDup(szOpaque,__FUNCTION__,__LINE__);
					pStatus->bBurnOk = 2;
				}
				Common_Sem_Post(pStatus->tSem);
			}
		}
	}


	return 0;
}
int CryptoMgr_Lan_Broadcast_BurnUUIDKey(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szUUIDReadyKey,char *szUUIDKeyInfo) // return char *szOutString
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	int nRet = -1;
	char *szNonce = NULL;
	char *szCreateTime = NULL;
	Common_Time_T tComTime;
	char szTime[32];
	char szKey[64],*pKey = NULL;
	int n,nTimeLen = 0;
	UPDATE_BROADCAST_HANDLE hBurnHandle = NULL;
	Crypto_BurnStatus_T tStatus;
	memset(&tStatus,0,sizeof(tStatus));

	Common_GetLocalTime(&tComTime);
	nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	pInParam = Common_Json_New_ex(NULL,Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
	if (pInParam != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Update/Crypto/BurnKey",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/UUIDKey",Common_Json_Type_String,szUUIDKeyInfo,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Time",Common_Json_Type_String,szTime,0,0);
		sprintf(szKey,"%s:(%s)",szUUIDReadyKey,szTime);
		pKey = ovfs_auth_EncryptString(2,szKey,NULL,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Key",Common_Json_Type_String,pKey,0,0);


		tStatus.pKey = pKey;
		tStatus.szUUID = szUUIDReadyKey;
		Common_Sem_Create(&tStatus.tSem,0,1,NULL);

		nRet = Update_Broadcast_Start(&hBurnHandle,nPort,pInParam,NULL,0,3,1,static_Update_Broadcast_BurnKey_Callback_fxn,&tStatus);
		if (!nRet)
		{
			int nTime = 0;

			if(0 == Common_Sem_TryPend(tStatus.tSem,6000))
			{
				// 
				if (tStatus.bBurnOk)
				{// 成功

				}
			}

		}
		Update_Broadcast_Stop(&hBurnHandle);
		Common_Sem_Destroy(&tStatus.tSem);
		int nTest = 3;
		do{
			if (tStatus.bBurnOk != 1)
			{
				S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
					*szCnonce=NULL,*szAuthUri=NULL,*szNc = NULL,*szMethod = NULL;
				S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
				S8 szNewPassword[16]="J5xv7_I2";// "J5xv7_I2"
				tStatus.bBurnOk = 0;
				szRealm = tStatus.szRealm;
				szNonce = tStatus.szNonce;
				szOpaque = tStatus.szOpaque;
				szAuthUri = "/Update/Crypto/BurnKey";
				szMethod = "Put";
				szUserName = "Admin";

				if (szRealm != NULL && szNonce != NULL && szOpaque != NULL)
				{

					if (szAuthUri == NULL)
					{
						szAuthUri = "";
					}
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
					if (szMethod == NULL)
					{
						szMethod = "Get";
					}
					szNewPassword[2] = szNonce[7]; 
					szNewPassword[4] = szNonce[11];
					szNewPassword[6] = szNonce[19]; 
					Common_Digest_CalcHA1((S8*)"", szUserName, szRealm, szNewPassword, szNonce, szCnonce, szHA1);
					Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCnonce, szQop, szMethod, szAuthUri, (S8*)"", szResponse);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Method",Common_Json_Type_Number,NULL,2,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/UserName",Common_Json_Type_String,szUserName,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Realm",Common_Json_Type_String,szRealm,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Qop",Common_Json_Type_String,szQop,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nonce",Common_Json_Type_String,szNonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Opaque",Common_Json_Type_String,szOpaque,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Cnonce",Common_Json_Type_String,szCnonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Uri",Common_Json_Type_String,szAuthUri,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Response",Common_Json_Type_String,szResponse,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nc",Common_Json_Type_String,szNc,NULL,NULL);

				}
				Common_Free(tStatus.szRealm,__FUNCTION__,__LINE__);
				tStatus.szRealm = NULL;
				Common_Free(tStatus.szNonce,__FUNCTION__,__LINE__);
				tStatus.szNonce = NULL;
				Common_Free(tStatus.szOpaque,__FUNCTION__,__LINE__);
				tStatus.szOpaque = NULL;
				Common_Sem_Create(&tStatus.tSem,0,1,NULL);
				nRet = Update_Broadcast_Start(&hBurnHandle,nPort,pInParam,NULL,0,3,1,static_Update_Broadcast_BurnKey_Callback_fxn,&tStatus);
				if (!nRet)
				{
					int nTime = 0;

					if(0 == Common_Sem_TryPend(tStatus.tSem,6000))
					{
						// 
						if (tStatus.bBurnOk)
						{// 成功
						}
					}

				}
				Update_Broadcast_Stop(&hBurnHandle);
				Common_Sem_Destroy(&tStatus.tSem);
				if (tStatus.bBurnOk == 1)
				{
					break;
				}
				
			}
			
			nTest--;
		}while(nTest > 0);


		Common_Json_Delete(pInParam);;
		pInParam = NULL;


	}
	Common_Free(tStatus.szRealm,__FUNCTION__,__LINE__);
	tStatus.szRealm = NULL;
	Common_Free(tStatus.szNonce,__FUNCTION__,__LINE__);
	tStatus.szNonce = NULL;
	Common_Free(tStatus.szOpaque,__FUNCTION__,__LINE__);
	tStatus.szOpaque = NULL;
	if (pKey != NULL)
	{
		Common_Free(pKey,__FUNCTION__,__LINE__);
		pKey = NULL;
	}
	return tStatus.bBurnOk == 1?0:-1;
}

int CryptoMgr_Wan_Require(char *szIpv4,int nPort,char *szUserName,char *szPassword,void *pInJson,void **pOutJson)
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	int nRet = -1;
	if (pInJson == NULL)
	{
		return -1;
	}

	pInParam = Common_Json_Duplicate((cJSON_Struct *)pInJson,1);
	if (pInParam != NULL)
	{
		nRet = Update_Tcp_Require(szIpv4,nPort,pInParam,&pOutParam,30000);
		if(pOutParam != NULL)
		{
			int nCode = -1;
			Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);

			if (nCode == 401)
			{
				int nAuthMethod = 0;
				S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
					*szCnonce=NULL,*szAuthUri=NULL,*szNc = NULL,*szMethod = NULL;
				S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
				Common_Json_GetAttrValue(pInParam,-1,"Header/Uri",NULL,&szAuthUri,NULL,NULL);
				Common_Json_GetAttrValue(pInParam,-1,"Header/Method",NULL,&szMethod,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Method",NULL,NULL,&nAuthMethod,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
				if (nAuthMethod == 2 && szRealm != NULL && szNonce != NULL && szOpaque != NULL)
				{
					if (szAuthUri == NULL)
					{
						szAuthUri = "";
					}
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
					if (szMethod == NULL)
					{
						szMethod = "Get";
					}
					Common_Digest_CalcHA1((S8*)"", szUserName, szRealm, szPassword, szNonce, szCnonce, szHA1);
					Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCnonce, szQop, szMethod, szAuthUri, (S8*)"", szResponse);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Method",Common_Json_Type_Number,NULL,2,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/UserName",Common_Json_Type_String,szUserName,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Realm",Common_Json_Type_String,szRealm,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Qop",Common_Json_Type_String,szQop,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nonce",Common_Json_Type_String,szNonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Opaque",Common_Json_Type_String,szOpaque,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Cnonce",Common_Json_Type_String,szCnonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Uri",Common_Json_Type_String,szAuthUri,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Response",Common_Json_Type_String,szResponse,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nc",Common_Json_Type_String,szNc,NULL,NULL);

				}
				Common_Json_Delete(pOutParam);
				pOutParam = NULL;
				nRet = Update_Tcp_Require(szIpv4,nPort,pInParam,&pOutParam,30000);
				if(pOutParam != NULL)
				{
					if (pOutJson != NULL)
					{
						*pOutJson = pOutParam;
						pOutParam = NULL;
					}
				}
			}
		}
		if (pOutParam != NULL)
		{
			Common_Json_Delete(pOutParam);
			pOutParam = NULL;
		}
		
		Common_Json_Delete(pInParam);
		pInParam = NULL;

	}

	return nRet;
}

int CryptoMgr_Wan_Register(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szOrderNo,char *szHardware,char *szUUID,char **szNewSerialNumber,char **szNewOrderNo,char **szCryptoInfo) // return char *szOutString
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	int nRet = -1;
	char *szNonce = NULL;
	char *szCreateTime = NULL;
	Common_Time_T tComTime;
	char szTime[32];
	char szKey[64],*pKey = NULL;
	int n,nTimeLen = 0;
	if (szCryptoInfo == NULL || szHardware == NULL || szUUID == NULL)
	{
		return -1;
	}

	Common_GetLocalTime(&tComTime);
	nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	pInParam = Common_Json_New_ex(NULL,Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
	if (pInParam != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/LicenceServer/Licence/Register",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Hardware",Common_Json_Type_String,szHardware,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/UUID",Common_Json_Type_String,szUUID,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Time",Common_Json_Type_String,szTime,0,0);
		if (szOrderNo != NULL)
		{
			Common_Json_SetAttrValue(pInParam,-1,"Data/OrderNo",Common_Json_Type_String,szOrderNo,0,0);
		}
		Update_Tcp_Require(szIpv4,nPort,pInParam,&pOutParam,30000);
		if(pOutParam != NULL)
		{
			int nCode = -1;
			Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
			nRet = nCode;
			
			if (nCode == 401)
			{
				int nAuthMethod = 0;
				S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
					*szCnonce=NULL,*szAuthUri=NULL,*szNc = NULL;
				S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Method",NULL,NULL,&nAuthMethod,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
				if (nAuthMethod == 2 && szRealm != NULL && szNonce != NULL && szOpaque != NULL)
				{
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
					szAuthUri = "/LicenceServer/Licence/Register";
					Common_Digest_CalcHA1((S8*)"", szUserName, szRealm, szPassword, szNonce, szCnonce, szHA1);
					Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCnonce, szQop, "Put", szAuthUri, (S8*)"", szResponse);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Method",Common_Json_Type_Number,NULL,2,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/UserName",Common_Json_Type_String,szUserName,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Realm",Common_Json_Type_String,szRealm,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Qop",Common_Json_Type_String,szQop,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nonce",Common_Json_Type_String,szNonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Opaque",Common_Json_Type_String,szOpaque,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Cnonce",Common_Json_Type_String,szCnonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Uri",Common_Json_Type_String,szAuthUri,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Response",Common_Json_Type_String,szResponse,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nc",Common_Json_Type_String,szNc,NULL,NULL);

				}
				Common_Json_Delete(pOutParam);
				pOutParam = NULL;
				Update_Tcp_Require(szIpv4,nPort,pInParam,&pOutParam,30000);
				if(pOutParam != NULL)
				{
					int nCode = -1;
					Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
					nRet = nCode;
					if (nCode == 0 || nCode == 200)
					{
						char *szLicence = NULL,*szOrderNo = NULL, *szSerialNumber = NULL;
						Common_Json_GetAttrValue(pOutParam,-1,"/Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);
						Common_Json_GetAttrValue(pOutParam,-1,"/Data/OrderNo",NULL,&szOrderNo,NULL,NULL);
						Common_Json_GetAttrValue(pOutParam,-1,"/Data/Licence",NULL,&szLicence,NULL,NULL);
						if (szLicence != NULL)
						{
							*szCryptoInfo = Common_StrDup(szLicence,__FUNCTION__,__LINE__);
						}
						if (szNewOrderNo != NULL)
						{
							*szNewOrderNo = Common_StrDup(szOrderNo,__FUNCTION__,__LINE__);
						}
						if (szNewSerialNumber != NULL)
						{
							*szNewSerialNumber = Common_StrDup(szSerialNumber,__FUNCTION__,__LINE__);
						}
					}
					Common_Json_Delete(pOutParam);
					pOutParam = NULL;
				}
			}
		}
		
	}
	if (pKey != NULL)
	{
		Common_Free(pKey,__FUNCTION__,__LINE__);
		pKey = NULL;
	}
	return nRet;
}
int CryptoMgr_Wan_GetUUIDKey(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szHardware,char **szUUIDKeyInfo)
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	int nRet = -1;
	char *szNonce = NULL;
	char *szCreateTime = NULL;
	Common_Time_T tComTime;
	char szTime[32];
	char szKey[64],*pKey = NULL;
	int n,nTimeLen = 0;
	if (szUUIDKeyInfo == NULL ||  szHardware == NULL)
	{
		return -1;
	}

	Common_GetLocalTime(&tComTime);
	nTimeLen = sprintf(szTime,"%04d-%02d-%02dT%02d:%02d:%02dZ",tComTime.year,tComTime.month,tComTime.day,tComTime.hour,tComTime.min,tComTime.sec);
	pInParam = Common_Json_New_ex(NULL,Common_Json_Type_Object,NULL,0,0,__FUNCTION__,__LINE__);
	if (pInParam != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/LicenceServer/Licence/GetKey",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Hardware",Common_Json_Type_String,szHardware,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Time",Common_Json_Type_String,szTime,0,0);
	
		Update_Tcp_Require(szIpv4,nPort,pInParam,&pOutParam,30000);
		if(pOutParam != NULL)
		{
			int nCode = -1;
			Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
			nRet = nCode;

			if (nCode == 401)
			{
				int nAuthMethod = 0;
				S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
					*szCnonce=NULL,*szAuthUri=NULL,*szNc = NULL;
				S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Method",NULL,NULL,&nAuthMethod,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
				Common_Json_GetAttrValue(pOutParam,-1,"Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
				if (nAuthMethod == 2 && szRealm != NULL && szNonce != NULL && szOpaque != NULL)
				{
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
					szAuthUri = "/LicenceServer/Licence/GetKey";
					Common_Digest_CalcHA1((S8*)"", szUserName, szRealm, szPassword, szNonce, szCnonce, szHA1);
					Common_Digest_CalcResponse(szHA1, szNonce, szNc, szCnonce, szQop, "Put", szAuthUri, (S8*)"", szResponse);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Method",Common_Json_Type_Number,NULL,2,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/UserName",Common_Json_Type_String,szUserName,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Realm",Common_Json_Type_String,szRealm,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Qop",Common_Json_Type_String,szQop,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nonce",Common_Json_Type_String,szNonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Opaque",Common_Json_Type_String,szOpaque,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Cnonce",Common_Json_Type_String,szCnonce,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Uri",Common_Json_Type_String,szAuthUri,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Response",Common_Json_Type_String,szResponse,NULL,NULL);
					Common_Json_SetAttrValue(pInParam,-1,(S8 *)"/Header/Auth/Digest/Nc",Common_Json_Type_String,szNc,NULL,NULL);

				}
				Common_Json_Delete(pOutParam);
				pOutParam = NULL;
				Update_Tcp_Require(szIpv4,nPort,pInParam,&pOutParam,30000);
				if(pOutParam != NULL)
				{
					int nCode = -1;
					Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
					nRet = nCode;
					if (nCode == 0 || nCode == 200)
					{
						char *szLicence = NULL,*szOrderNo = NULL, *szSerialNumber = NULL;
						Common_Json_GetAttrValue(pOutParam,-1,"/Data/UUIDKey",NULL,&szLicence,NULL,NULL);
						if (szLicence != NULL)
						{
							*szUUIDKeyInfo = Common_StrDup(szLicence,__FUNCTION__,__LINE__);
						}
					
					}
					Common_Json_Delete(pOutParam);
					pOutParam = NULL;
				}
			}
		}

	}
	if (pKey != NULL)
	{
		Common_Free(pKey,__FUNCTION__,__LINE__);
		pKey = NULL;
	}
	return nRet;
}
/*
本库内部分配内存，由此接口释放
*/
void CryptoMgr_Free(void *lpMem)
{
	if (lpMem != NULL)
	{
		Common_Free(lpMem,__FUNCTION__,__LINE__);
	}
	return;
}


