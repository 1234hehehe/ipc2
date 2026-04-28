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
#include "access_rest.h"
#include "access_usercfg.h"

//static Common_Thread_T g_hThread;
static cJSON_Struct *g_rest_res = NULL;
//static cJSON_Struct *g_right_res = NULL;
static OVFS_ABILITY *g_ability = NULL;
static Access_SubscribeMgr_T g_subscribeMgr;
static int       g_nUserCounter = 0;
//static int       g_nModuleId[MAX_MODULE_COUNT] = {0};
static OnlineModuleList_T  g_OnlineModule;
int       g_iCfgChange = 0;			//用户配置修改上传标志;配置变化，则主动上传到订阅者
static S32 g_bPrintDbg = 0;

int notice_usercfg_change(void *pInData,void *pAddData,void *pCondition,void **pOutData);
int notice_onlineuser_change(void *pInData,void *pAddData,void *pCondition,void **pOutData);
int notice_iptable_change(void *pInData,void *pAddData,void *pCondition,void **pOutData);
S32 refreshOnlineUserInfo(AccessUserCfgMgr_T *pAccessUsrCfgMgr);

S32 CompareIsInIPList(S8 *szIP,BindInfo_T *pBindInfo)
{
	int ip_add[4] = {0};
	int start_ip_add[4] = {0},end_ip_add[4] = {0};
	int iStartAddr = 0,iEndAddr = 0,iIpAddr = 0;

	if(szIP)
		sscanf(szIP,"%d.%d.%d.%d",&ip_add[0],&ip_add[1],&ip_add[2],&ip_add[3]);
	iIpAddr = (ip_add[0]<<24)|(ip_add[1]<<16)|(ip_add[2]<<8)|(ip_add[3]);
	if(pBindInfo->szBindIpv4)
	{
		sscanf(pBindInfo->szBindIpv4,"%d.%d.%d.%d",&start_ip_add[0],&start_ip_add[1],&start_ip_add[2],&start_ip_add[3]);
		iStartAddr = (start_ip_add[0]<<24)|(start_ip_add[1]<<16)|(start_ip_add[2]<<8)|(start_ip_add[3]);
	}
	if(pBindInfo->szBindEndIpv4)
	{
		sscanf(pBindInfo->szBindEndIpv4,"%d.%d.%d.%d",&end_ip_add[0],&end_ip_add[1],&end_ip_add[2],&end_ip_add[3]);
		iEndAddr = (end_ip_add[0]<<24)|(end_ip_add[1]<<16)|(end_ip_add[2]<<8)|(end_ip_add[3]);
	}
	if(iEndAddr < iStartAddr)
		return 0;
	if(iIpAddr > iEndAddr || iIpAddr < iStartAddr)
	{
		return 0;
	}
	return 1;
}

S32 String2Time(S8 *szTime,S32 iLength,Common_Time_T *pComTime,S32 *pTime)
{
	Common_Time_T tTime = {0};
	S32 year = 0,mon = 0,day = 0,hour = 0,min = 0,sec = 0;
	if(!szTime)
	{
		LOGE("Data is null\n");
		return -1;
	}
	if((iLength != 6) && (iLength != 8) && (iLength != 14))
	{
		LOGE("iLength[%d] is not right\n",iLength);
		return -1;
	}
	if(14 == iLength)
		sscanf(szTime, "%04d%02d%02d%02d%02d%02d", &year,&mon,&day,&hour,&min,&sec);
	else if(8 == iLength)
		sscanf(szTime, "%04d%02d%02d", &year,&mon,&day);
	else
		sscanf(szTime, "%02d%02d%02d", &hour,&min,&sec);
	tTime.year = year;
	tTime.month = mon;
	tTime.day= day;
	tTime.hour= hour;
	tTime.min = min;
	tTime.sec = sec;
	if(pComTime)
		memcpy(pComTime,&tTime,sizeof(Common_Time_T));
	if(pTime)
		Common_Common2LinuxTime(&tTime, (time_t*)pTime);
	return 0;
}

int access_MakeHandle(int nLoginHandle,int nSubIndex)
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

static S8* static_EncryptString(S32 nMethod,S8 *pOrgString,S8 *pNewString,S32 nNewSize)
{
	S8 *pNewReturnString = NULL;
	S32 i,nOrgLen;
	if (pOrgString == NULL)
	{
		return pNewReturnString;
	}
	nOrgLen = strlen(pOrgString);
	// 检查是否合法		//找回密码时的加密,无此限制
	/*for (i = 0; i < nOrgLen; i++)
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
	}*/
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

static S8* static_DecryptString(S8 *pOrgString,S32 *lpEncryptType,S8 *pNewString,S32 nNewSize)
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

static S8* static_GenTempPwd(S8 *pOrgString,S32 lpEncryptType,S8 *pDestString,S32 nNewSize)
{
	U16 genRand = 0;
	S8 *pNewString = NULL;

	pNewString = (S8*)Common_Malloc(nNewSize+1,0, __FUNCTION__, __LINE__);
	if(!pNewString)
	{
		pNewString = pOrgString;
	}
	else
	{
		memset(pNewString,0,nNewSize+1);
		for(int i = 0;i < nNewSize;i++)
		{
			genRand = Common_Rand16();
			if(0 == (genRand%3))
				pNewString[i] = 'a' + (genRand%26);
			else if(1 == (genRand%3))
				pNewString[i] = 'A' + (genRand%26);
			else
				pNewString[i] = '0' + (genRand%10);
		}
	}
	if(pDestString)
		memcpy(pDestString,pNewString,nNewSize);
	return pNewString;
}

static S8* static_GetDevSerialNumber()
{
	S32 iRet = -1,nLen = 0;
	S8 *szSerialNumber = NULL,*pResult = NULL;
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL;

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Core/Version",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
	}
	iRet = Module_CallFunctions(g_subscribeMgr.hModuleHandle,pConfig,&pOutParams,3000);
	if(iRet < 0)
	{
		LOGE("iRet:%d\n",iRet);
		return NULL;
	}
	Common_Json_GetAttrValue(pOutParams, -1, "Data/SerialNumber" ,NULL, &szSerialNumber, NULL, NULL);
	nLen = strlen(szSerialNumber)+1;
	pResult = (S8*)Common_Malloc(nLen,0, __FUNCTION__, __LINE__);
	if(pResult)
		memcpy(pResult,szSerialNumber,nLen);
	Common_Json_Delete(pConfig);
	Common_Json_Delete(pOutParams);
	return pResult;
}

static S32  set_method_callback(cJSON_Struct **pData,OVFS_GET_METHOD pGetMethod,OVFS_PUT_METHOD pPutMethod,OVFS_POST_METHOD pPostMethod,OVFS_DELETE_METHOD pDelMethod)
{
	OVFS_REST_METHOD *pRestMethod;
	pRestMethod = (OVFS_REST_METHOD *)Common_Malloc(sizeof(OVFS_REST_METHOD),0,__FUNCTION__,__LINE__);
	if(!pRestMethod)
		return -1;
	pRestMethod->ovfs_get_method = pGetMethod;
	pRestMethod->ovfs_put_method = pPutMethod;
	pRestMethod->ovfs_post_method = pPostMethod;
	pRestMethod->ovfs_delete_method = pDelMethod;
	Common_Json_SetItemExtData(*pData, (void *)pRestMethod, sizeof(OVFS_REST_METHOD));
	Common_Free(pRestMethod, __FUNCTION__,__LINE__);
	pRestMethod = NULL;
	return 0;
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
权限          		访客        普通用户		操作员      管理员
预览（总开关）    	1             1				  1           1
回放              	0             1				  1           1
设置              	0             0				  0           1
查看设置          	0             0				  1           1
录像              	0             0				  0           1
PTZ               	0             0				  1           1
备份              	0             0				  0           1
日志              	0             1				  1           1
设备信息          	0             1				  1           1
升级              	0             0				  0           1
电源 关机，重启		0			  1				  1		      1
格式化				0			  0				  0		   	  1
IP通道				0			  0				  1		   	  1
修改时间          	0             0				  0           1
删除用户          	0             0				  0           1
修改用户          	0             0				  0           1
重置密码          	0             0				  0           1
重置自己的密码    	0             0				  1           1
在线授权          	0             0				  0           1

预览 (通道)       	1             1				  1           1
回放              	0             1				  1           1
设置              	0             0				  0           1
查看设置          	0             0				  1           1
录像              	0             0				  0           1
PTZ               	0             0				  1           1
备份              	0             0				  0           1
*/
S32 set_defaultChanRight(AccessChanRight_T *pChanRight,U32 level)
{
	U32 u32ChanRight = 0;

	if(!pChanRight)
		return -1;
	if(LEVEL_ROOT == level)
		u32ChanRight = 0xff;
	else if(LEVEL_ADMIN == level)
		u32ChanRight = 0x7F;
	else if(LEVEL_OPERATOR == level)
		u32ChanRight = 0x2b;
	else if(LEVEL_GUEST== level)
		u32ChanRight = 0x01;
	else if(LEVEL_DEFAULT == level)
		u32ChanRight = 0x03;
	if(u32ChanRight&BIT_CHAN_PREVIEW)  {pChanRight->bPreview = 1;} else {pChanRight->bPreview = 0;}
	if(u32ChanRight&BIT_CHAN_PLAYBACK) {pChanRight->bPlayback = 1;} else {pChanRight->bPlayback = 0;}
	if(u32ChanRight&BIT_CHAN_SETTING) {pChanRight->bSetting = 1;} else {pChanRight->bSetting = 0;}
	if(u32ChanRight&BIT_CHAN_VIEWSETTING) {pChanRight->bViewSetting = 1;} else {pChanRight->bViewSetting = 0;}
	if(u32ChanRight&BIT_CHAN_RECORD)  {pChanRight->bRecord = 1;} else {pChanRight->bRecord = 0;}
	if(u32ChanRight&BIT_CHAN_PTZ)  {pChanRight->bPTZ = 1;} else {pChanRight->bPTZ = 0;}
	if(u32ChanRight&BIT_CHAN_BACKUP) {pChanRight->bBackup = 1;} else {pChanRight->bBackup = 0;}
	return 0;
}

S32 set_defaultUsrRight(AccessUserRight_T *pUserRight,U32 level)
{
	U32 u32Right = 0;
	S32 i = 0,j = 0,s = 0;
	OVFS_ABILITY *pAbility = get_channel_ability();

	if(!pUserRight)
	{
		LOGE("pUserRight = null\n");
		return -1;
	}
	if(!pAbility)
	{
		LOGE("pAbility = null\n");
		return -1;
	}
	if(LEVEL_ROOT == level)
		u32Right = 0xffffffff;
	else if(LEVEL_ADMIN == level)
		u32Right = 0x000fffff;
	else if(LEVEL_OPERATOR == level)
		u32Right = 0x000215ab;
	else if(LEVEL_GUEST== level)
		u32Right = 0x00000001;
	else if(LEVEL_DEFAULT == level)
		u32Right = 0x00000583;

	if(u32Right&BIT_PREVIEW)	{pUserRight->bPreview = 1;} else {pUserRight->bPreview = 0;}
	if(u32Right&BIT_PLAYBACK) {pUserRight->bPlayback = 1;} else {pUserRight->bPlayback = 0;}
	if(u32Right&BIT_SETTING) {pUserRight->bSetting = 1;} else {pUserRight->bSetting = 0;}
	if(u32Right&BIT_VIEWSETTING) {pUserRight->bViewSetting = 1;} else {pUserRight->bViewSetting = 0;}
	if(u32Right&BIT_RECORD) {pUserRight->bRecord = 1;} else {pUserRight->bRecord = 0;}
	if(u32Right&BIT_PTZ) {pUserRight->bPtz = 1;} else {pUserRight->bPtz = 0;}
	if(u32Right&BIT_BACKUP) {pUserRight->bBackup = 1;} else {pUserRight->bBackup = 0;}
	if(u32Right&BIT_LOG)	{pUserRight->bLog = 1;} else {pUserRight->bLog = 0;}
	if(u32Right&BIT_VIEWINFO) {pUserRight->bViewInfo = 1;} else {pUserRight->bViewInfo = 0;}
	if(u32Right&BIT_UPGRADE)	 {pUserRight->bUpgrade = 1;} else {pUserRight->bUpgrade = 0;}
	if(u32Right&BIT_POWER) {pUserRight->bPower = 1;} else {pUserRight->bPower = 0;}
	if(u32Right&BIT_FORMAT) {pUserRight->bFormat = 1;} else {pUserRight->bFormat = 0;}
	if(u32Right&BIT_IPCHANNEL) {pUserRight->bIPChannel = 1;} else {pUserRight->bIPChannel = 0;}
	if(u32Right&BIT_CORRECTTIME) {pUserRight->bCorrectionTime = 1;} else {pUserRight->bCorrectionTime = 0;}
	if(u32Right&BIT_DELETEUSER) {pUserRight->bDeleteUser = 1;} else {pUserRight->bDeleteUser = 0;}
	if(u32Right&BIT_MODIFYUSER) {pUserRight->bModityUser = 1;} else {pUserRight->bModityUser = 0;}
	if(u32Right&BIT_RESETPWD) {pUserRight->bResetPassword = 1;} else {pUserRight->bResetPassword = 0;}
	if(u32Right&BIT_RESETPWDSELF) {pUserRight->bResetPasswordSelf = 1;} else {pUserRight->bResetPasswordSelf = 0;}
	if(u32Right&BIT_ONLINEAUTH) {pUserRight->bOnlineAuthority = 1;} else {pUserRight->bOnlineAuthority = 0;}
	if(u32Right&BIT_REMOTETALK) {pUserRight->bRemoteTalk= 1;} else {pUserRight->bRemoteTalk = 0;}


	pUserRight->nChanRightCount = pAbility->totalChanNum;
	pUserRight->pChanRight = (AccessChanRight_T *)Common_Malloc(pUserRight->nChanRightCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
	if(pUserRight->pChanRight){
		memset(pUserRight->pChanRight,0,pUserRight->nChanRightCount*sizeof(AccessChanRight_T));
		for(i = 0;i < pAbility->devNum;i++)
		{
			OVFS_DEV_ABILITY *pDev = pAbility->pDevAbility[i];
			if(!pDev)
				continue;
			for(j = 0;j < pDev->chanNum;j++)
			{
				pUserRight->pChanRight[s].wDeviceNo = i;
				pUserRight->pChanRight[s].wChannelNo = j;
				set_defaultChanRight(&pUserRight->pChanRight[s],level);
				s++;
			}
		}
	}
	return 0;
}

S32 get_chanRight(AccessChanRight_T *pChanRight)
{
	U32 u32ChanRight = 0;
	if(pChanRight->bPreview) u32ChanRight|=BIT_CHAN_PREVIEW;
	if(pChanRight->bPlayback) u32ChanRight|=BIT_CHAN_PLAYBACK;
	if(pChanRight->bSetting) u32ChanRight|=BIT_CHAN_SETTING;
	if(pChanRight->bViewSetting) u32ChanRight|=BIT_CHAN_VIEWSETTING;
	if(pChanRight->bRecord) u32ChanRight|=BIT_CHAN_RECORD;
	if(pChanRight->bPTZ) u32ChanRight|=BIT_CHAN_PTZ;
	if(pChanRight->bBackup) u32ChanRight|=BIT_CHAN_BACKUP;
	return u32ChanRight;
}

S32 set_chanRight(AccessChanRight_T *pChanRight,U32 u32ChanRight)
{
	S32 bChange = 0;

	if(!pChanRight)
		return -1;
	if(u32ChanRight&BIT_CHAN_PREVIEW)  {if(!pChanRight->bPreview) {pChanRight->bPreview = 1;bChange = 1;}}
	else {if(pChanRight->bPreview) {pChanRight->bPreview = 0;bChange = 1;}}
	if(u32ChanRight&BIT_CHAN_PLAYBACK) {if(!pChanRight->bPlayback) {pChanRight->bPlayback = 1;bChange = 1;}}
	else {if(pChanRight->bPlayback) {pChanRight->bPlayback = 0;bChange = 1;}}
	if(u32ChanRight&BIT_CHAN_SETTING) {if(!pChanRight->bSetting) {pChanRight->bSetting = 1;bChange = 1;}}
	else {if(pChanRight->bSetting) {pChanRight->bSetting = 0;bChange = 1;}}
	if(u32ChanRight&BIT_CHAN_VIEWSETTING) {if(!pChanRight->bViewSetting) {pChanRight->bViewSetting = 1;bChange = 1;}}
	else {if(pChanRight->bViewSetting) {pChanRight->bViewSetting = 0;bChange = 1;}}
	if(u32ChanRight&BIT_CHAN_RECORD)  {if(!pChanRight->bRecord) {pChanRight->bRecord = 1;bChange = 1;}}
	else {if(pChanRight->bRecord) {pChanRight->bRecord = 0;bChange = 1;}}
	if(u32ChanRight&BIT_CHAN_PTZ)  {if(!pChanRight->bPTZ) {pChanRight->bPTZ = 1;bChange = 1;}}
	else {if(pChanRight->bPTZ) {pChanRight->bPTZ = 0;bChange = 1;}}
	if(u32ChanRight&BIT_CHAN_BACKUP) {if(!pChanRight->bBackup) {pChanRight->bBackup = 1;bChange = 1;}}
	else {if(pChanRight->bBackup) {pChanRight->bBackup = 0;bChange = 1;}}
	return bChange;
}

S32 get_userRight(AccessUserRight_T *pUserRight)
{
	U32 u32Right = 0;
	if(pUserRight->bPreview)	u32Right|=BIT_PREVIEW;
	if(pUserRight->bPlayback)	u32Right|=BIT_PLAYBACK;
	if(pUserRight->bSetting)	u32Right|=BIT_SETTING;
	if(pUserRight->bViewSetting)	u32Right|=BIT_VIEWSETTING;
	if(pUserRight->bRecord)	u32Right|=BIT_RECORD;
	if(pUserRight->bPtz)	u32Right|=BIT_PTZ;
	if(pUserRight->bBackup)	u32Right|=BIT_BACKUP;
	if(pUserRight->bLog)	u32Right|=BIT_LOG;
	if(pUserRight->bViewInfo)	u32Right|=BIT_VIEWINFO;
	if(pUserRight->bUpgrade)	u32Right|=BIT_UPGRADE;
	if(pUserRight->bPower)	u32Right|=BIT_POWER;
	if(pUserRight->bFormat)	u32Right|=BIT_FORMAT;
	if(pUserRight->bIPChannel)	u32Right|=BIT_IPCHANNEL;
	if(pUserRight->bCorrectionTime)	u32Right|=BIT_CORRECTTIME;
	if(pUserRight->bDeleteUser)	u32Right|=BIT_DELETEUSER;
	if(pUserRight->bModityUser)	u32Right|=BIT_MODIFYUSER;
	if(pUserRight->bResetPassword)	u32Right|=BIT_RESETPWD;
	if(pUserRight->bResetPasswordSelf)	u32Right|=BIT_RESETPWDSELF;
	if(pUserRight->bOnlineAuthority)	u32Right|=BIT_ONLINEAUTH;
	if(pUserRight->bRemoteTalk)	u32Right|=BIT_REMOTETALK;
	return u32Right;
}

S32 set_userRight(AccessUserRight_T *pUserRight,U32 u32Right)
{
	S32 bChange = 0;

	if(!pUserRight)
		return -1;
	if(u32Right&BIT_PREVIEW)	{if(!pUserRight->bPreview) {pUserRight->bPreview = 1;bChange = 1;}}
	else {if(pUserRight->bPreview) {pUserRight->bPreview = 0;bChange = 1;}}
	if(u32Right&BIT_PLAYBACK) {if(!pUserRight->bPlayback) {pUserRight->bPlayback = 1;bChange = 1;}}
	else {if(pUserRight->bPlayback) {pUserRight->bPlayback = 0;bChange = 1;}}
	if(u32Right&BIT_SETTING) {if(!pUserRight->bSetting) {pUserRight->bSetting = 1;bChange = 1;}}
	else {if(pUserRight->bSetting) {pUserRight->bSetting = 0;bChange = 1;}}
	if(u32Right&BIT_VIEWSETTING)	{if(!pUserRight->bViewSetting) {pUserRight->bViewSetting = 1;bChange = 1;}}
	else {if(pUserRight->bViewSetting) {pUserRight->bViewSetting = 0;bChange = 1;}}
	if(u32Right&BIT_RECORD) {if(!pUserRight->bRecord) {pUserRight->bRecord = 1;bChange = 1;}}
	else {if(pUserRight->bRecord) {pUserRight->bRecord = 0;bChange = 1;}}
	if(u32Right&BIT_PTZ) {if(!pUserRight->bPtz) {pUserRight->bPtz = 1;bChange = 1;}}
	else {if(pUserRight->bPtz) {pUserRight->bPtz = 0;bChange = 1;}}
	if(u32Right&BIT_BACKUP) {if(!pUserRight->bBackup) {pUserRight->bBackup = 1;bChange = 1;}}
	else {if(pUserRight->bBackup) {pUserRight->bBackup = 0;bChange = 1;}}
	if(u32Right&BIT_LOG)	{if(!pUserRight->bLog) {pUserRight->bLog = 1;bChange = 1;}}
	else {if(pUserRight->bLog) {pUserRight->bLog = 0;bChange = 1;}}
	if(u32Right&BIT_VIEWINFO) {if(!pUserRight->bViewInfo) {pUserRight->bViewInfo = 1;bChange = 1;}}
	else {if(pUserRight->bViewInfo) {pUserRight->bViewInfo = 0;bChange = 1;}}
	if(u32Right&BIT_UPGRADE)	 {if(!pUserRight->bUpgrade) {pUserRight->bUpgrade = 1;bChange = 1;}}
	else {if(pUserRight->bUpgrade) {pUserRight->bUpgrade = 0;bChange = 1;}}
	if(u32Right&BIT_POWER) {if(!pUserRight->bPower) {pUserRight->bPower = 1;bChange = 1;}}
	else {if(pUserRight->bPower) {pUserRight->bPower = 0;bChange = 1;}}
	if(u32Right&BIT_FORMAT) {if(!pUserRight->bFormat) {pUserRight->bFormat = 1;bChange = 1;}}
	else {if(pUserRight->bFormat) {pUserRight->bFormat = 0;bChange = 1;}}
	if(u32Right&BIT_IPCHANNEL) {if(!pUserRight->bIPChannel) {pUserRight->bIPChannel = 1;bChange = 1;}}
	else {if(pUserRight->bIPChannel) {pUserRight->bIPChannel = 0;bChange = 1;}}
	if(u32Right&BIT_CORRECTTIME) {if(!pUserRight->bCorrectionTime) {pUserRight->bCorrectionTime = 1;bChange = 1;}}
	else {if(pUserRight->bCorrectionTime) {pUserRight->bCorrectionTime = 0;bChange = 1;}}
	if(u32Right&BIT_DELETEUSER) {if(!pUserRight->bDeleteUser) {pUserRight->bDeleteUser = 1;bChange = 1;}}
	else {if(pUserRight->bDeleteUser) {pUserRight->bDeleteUser = 0;bChange = 1;}}
	if(u32Right&BIT_MODIFYUSER) {if(!pUserRight->bModityUser) {pUserRight->bModityUser = 1;bChange = 1;}}
	else {if(pUserRight->bModityUser) {pUserRight->bModityUser = 0;bChange = 1;}}
	if(u32Right&BIT_RESETPWD) {if(!pUserRight->bResetPassword) {pUserRight->bResetPassword = 1;bChange = 1;}}
	else {if(pUserRight->bResetPassword) {pUserRight->bResetPassword = 0;bChange = 1;}}
	if(u32Right&BIT_RESETPWDSELF) {if(!pUserRight->bResetPasswordSelf) {pUserRight->bResetPasswordSelf = 1;bChange = 1;}}
	else {if(pUserRight->bResetPasswordSelf) {pUserRight->bResetPasswordSelf = 0;bChange = 1;}}
	if(u32Right&BIT_ONLINEAUTH) {if(!pUserRight->bOnlineAuthority) {pUserRight->bOnlineAuthority = 1;bChange = 1;}}
	else {if(pUserRight->bOnlineAuthority) {pUserRight->bOnlineAuthority = 0;bChange = 1;}}
	if(u32Right&BIT_REMOTETALK) {if(!pUserRight->bRemoteTalk) {pUserRight->bRemoteTalk = 1;bChange = 1;}}
	else {if(pUserRight->bRemoteTalk) {pUserRight->bRemoteTalk = 0;bChange = 1;}}
	return bChange;
}

S32 MakeResult(const S8 *pFileDes,const S8 *pFuncDes,S32 nLine,S32 iCode,const S8 *pDes,cJSON_Struct *pData,cJSON_Struct **pOutData)
{
	cJSON_Struct *pResult = NULL;
	S8 sDes[128] = {0};
	if(!pOutData)
	{
		LOGE("pOutData is null\n");
		return -1;
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,iCode,0);
		if(iCode < 0)
		{
			//sprintf(sDes,"[%s][%s][%d] %s",pFileDes,pFuncDes,nLine,pDes);
			sprintf(sDes,"%s",pDes);
			Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sDes,0,0);
		}
		else
		{
			Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,pDes,0,0);
		}
		Common_Json_AddItem(pResult,-1,"Data",pData);
	}
	*pOutData = pResult;
	return 0;
}

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

//解析原始URI和方法类型并执行相应的方法
//method: 0-get 1-put 2-post 3-delete
//"Uri:/Alarm/TriggerCfg/AlarmIn/Attr?Device=-1&&Channel=-1"
static S32 AnalyzeUriAndMakeResult(S32 method,char* pUri,cJSON_Struct *pAddData,cJSON_Struct **ppResult)
{
	S8 pResPath[128] = {0};
	OVFS_REST_METHOD *pMethod = NULL;
	S8 *pTemp = NULL,*pSrcUri = NULL,*pFirst = NULL,*pNext = NULL;
	cJSON_Struct *pRoot = g_rest_res,*pChild = NULL,*pJCondition = NULL;

	if(!g_rest_res)
	{
		LOGW("g_rest_res  == null\n");
		return -1;
	}
	pJCondition = SeparateUriAndCondition(pUri, &pSrcUri);
	pTemp = pSrcUri;
	while(pTemp)
	{
		Common_UriOneParse(pTemp,NULL,&pFirst,&pNext);
		if(pFirst)
		{
			sprintf(pResPath,"%s/%s",pResPath,pFirst);
			if(pNext)
			{
				pTemp = pNext;
			}
			else
			{
				pChild = Common_Json_GetItem(pRoot,-1,pResPath);
				if(pChild)
				{
					//LOGE("pResPath:%s,pFirst:%s,method:%d\n",pResPath,pFirst,method);
					pMethod = (OVFS_REST_METHOD *)Common_Json_GetItemExtData(pChild,NULL);
					if(!pMethod)
					{
						break;
					}
					if((MATHOD_GET==method)&&pMethod->ovfs_get_method)
						pMethod->ovfs_get_method(pResPath, pAddData,pJCondition,ppResult);
					if((MATHOD_PUT==method)&&pMethod->ovfs_put_method)
						pMethod->ovfs_put_method(pResPath, pAddData,pJCondition,ppResult);
					if((MATHOD_POST==method)&&pMethod->ovfs_post_method)
						pMethod->ovfs_post_method(pResPath, pAddData,pJCondition,ppResult);
					if((MATHOD_DELETE==method)&&pMethod->ovfs_delete_method)
						pMethod->ovfs_delete_method(pResPath, pAddData,pJCondition,ppResult);
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
	return 0;
}

//订阅URI转到相应资源的URI，如/Alarm/Subscribe/TriggerCfg转到/Alarm/TriggerCfg
int AnalyzeSubscribeUriAndSend(ModuleHandle_T hModuleHandle,S32 nRecvID,void *pUriData,void *pCondition,cJSON_Struct **pOutParams)
{
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;

	if(!pOutParams)
	{
		LOGE("pOutParams is null\n");
		return -1;
	}
	if(0 == Common_StriCmp((S8*)pUriData, (S8*)"/Access/Subscribe/UserCfg"/*,25*/))
	{
		notice_usercfg_change(NULL,NULL,pJCondition,pOutParams);
	}
	else if(0 == Common_StriCmp((S8*)pUriData, (S8*)"/Access/Subscribe/OnlineUser"/*,28*/))
	{
		notice_onlineuser_change(NULL,NULL,pJCondition,pOutParams);
	}
	else if(0 == Common_StriCmp((S8*)pUriData, (S8*)"/Access/Subscribe/Iptable"/*,28*/))
	{
		notice_iptable_change(NULL,NULL,pJCondition,pOutParams);
	}
	return 0;
}

bool checkIsValid(char *pData,int iDataLen)
{
	if(!pData ||iDataLen <= 0)
		return 0;
	/*for(int i = 0;i < iDataLen;i++)
	{
		if((pData[i] >= '0' && pData[i] <= '9') ||(pData[i] >= 'A' &&pData[i] <= 'Z')||(pData[i] >= 'a' &&pData[i] <= 'z')||(pData[i] == '_'))
		{
		}
		else
		{
			return 0;
		}
	}*/
	return 1;
}

int get_access_top_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 sErrorDes[256] = {0};
	S32 i = 0,iCode = 0,iArrayCount = 0;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
	S8 label[][64] = {"UserCfg","PasswordLost","InvalidFindPwd","RestorePassword","OnlineUser","SyncOnlineUser","GlobalId","Restore","AccessControl","PrintDebug"};
	S8 uri[][64] = {"/Access/UserCfg","/Access/PasswordLost","/Access/InvalidFindPwd","/Access/RestorePassword","/Access/OnlineUser","/Access/SyncOnlineUser","/Access/GlobalId","/Access/Restore","/Access/AccessControl","/Access/PrintDebug"};

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,iCode,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sErrorDes,11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		for(i = 0;i < ARRAYSIZE(label);i++){
			Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,label[i],0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,uri[i],0,0);
			iArrayCount++;
		}
	}
	*pOutData = pResult;
	return 0;
}

int post_access_usercfg(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bOk = 0;
	S8 *pUserName = NULL,*pValue = NULL,*pTemp = NULL;
	S32 i = 0,iIndex = 0,iObjType = 0,iArrayCount = 0,iValue = -1,iCount = 0;
	AccessUserCfg *pUserCfg = NULL,*pLstUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pItem = NULL,*pItem1 = NULL;
	AccessUserRight_T *pRight = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(!pAccessUsrCfgMgr->pUserCfg)
	{
		LOGE("pAccessUsrCfgMgr->pUserCfg == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;
	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);
	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		pValue = NULL;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "UserName" ,NULL, &pValue, NULL, NULL);
		if(pItem)
		{
			pUserName = pValue;
			if(!pUserName)
			{
				LOGW("pUserName==null\n");
				continue;
			}
			if(!checkIsValid(pUserName,strlen(pUserName)))
			{
				LOGE("pUserName is invalid\n");
				continue;
			}
			pValue = NULL;
			pItem1 = Common_Json_GetAttrValue(pJData, iIndex, "DefaultPwd" ,NULL, &pValue, NULL, NULL);
			if(pItem1)
			{
				if(0 == checkIsValid(pValue,strlen(pValue)))
				{
					LOGE("DefaultPwd invalid[%s]\n",pValue);
					continue;
				}
			}
			pArray = Common_Json_GetItem(pJData, iIndex, "Password");
			if(pArray)
			{
				cJSON_Struct *pItem2 = NULL;
				S32 iPwdLen = 0;
				bool bIsInvalid = 0;
				iCount = Common_Json_Size(pArray);
				for(i = 0;i < iCount;i++)
				{
					pValue = NULL;
					pItem2 = Common_Json_GetAttrValue(pArray, i, NULL ,NULL, &pValue, NULL, NULL);
					if(pItem2)
					{
						iPwdLen = strlen(pValue);
						if(0 == checkIsValid(pValue,iPwdLen))
						{
							LOGE("Password invalid[%s]\n",pValue);
							bIsInvalid = 1;
							break;
						}
					}
				}
				if(bIsInvalid)
					continue;
			}

            pItem1 = Common_Json_GetAttrValue(pJData, iIndex, "Priority" ,NULL, NULL, &iValue, NULL);
			if(pItem1)
			{
				if(iValue >= LEVEL_ADMIN)
                {
                    LOGE("No add admin!\n");
                    continue;
                }
			}

			pUserCfg = pAccessUsrCfgMgr->pUserCfg;
			while(pUserCfg)
			{
				if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
					break;
				pUserCfg = pUserCfg->pNext;
			}
			if(pUserCfg)
				continue;
			pLstUserCfg = pAccessUsrCfgMgr->pUserCfg;
			do{
				if(!pLstUserCfg->pNext)
					break;
				pLstUserCfg = pLstUserCfg->pNext;
			}while(pLstUserCfg);

			pLstUserCfg->pNext = (AccessUserCfg *)Common_Malloc(sizeof(AccessUserCfg), 0, __FUNCTION__, __LINE__);
			if(!pLstUserCfg->pNext)
				continue;
			memset(pLstUserCfg->pNext,0,sizeof(AccessUserCfg));
			pUserCfg = pLstUserCfg->pNext;
			pUserCfg->pNext = NULL;
			pUserCfg->pPrev = pLstUserCfg;
		}
		else
		{
			continue;
		}
		if(!pUserCfg)
			continue;
		pUserCfg->szUserName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "EncryptMethod" ,NULL, NULL, &iValue, NULL);
		if(pItem)
			pUserCfg->byEncryptMethod = iValue;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "DefaultPwd" ,NULL, &pValue, NULL, NULL);
		if(pItem)
		{
			pUserCfg->szDefaultPassword = static_EncryptString(SYSTEM_TEXT,pValue,pTemp,0);
			pTemp = NULL;
			//pUserCfg->szDefaultPassword = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		}
		pArray = Common_Json_GetItem(pJData, iIndex, "Password");
		if(pArray)
		{
			iCount = Common_Json_Size(pArray);
			pUserCfg->byPwdCount = iCount;
			for(i = 0;i < iCount;i++){
				pItem = Common_Json_GetAttrValue(pArray, i, NULL ,NULL, &pValue, NULL, NULL);
				if(pItem)
				{
					//pUserCfg->szPassword[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pUserCfg->szPassword[i] = static_EncryptString(SYSTEM_TEXT,pValue,pTemp,0);
					pTemp = NULL;
				}
			}
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "Priority" ,NULL, NULL, &iValue, NULL);
		if(pItem)
		{
			pUserCfg->byPriority = iValue;
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "Forbidden" ,NULL, NULL, &iValue, NULL);
		if(pItem)
			pUserCfg->bForbidden = iValue;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "Remote" ,NULL, NULL, &iValue, NULL);
		if(pItem)
			pUserCfg->bRemote = iValue;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "NeedOnlineAuth" ,NULL, NULL, &iValue, NULL);
		if(pItem)
			pUserCfg->bNeedOnlineAuth = iValue;
		else
			pUserCfg->bNeedOnlineAuth = 1;
		pArray = Common_Json_GetItem(pJData, iIndex, "OnlineAuthManList");
		if(pArray)
		{
			pUserCfg->byOnlineAuthListCount = Common_Json_Size(pArray);
			for(i = 0;i < pUserCfg->byOnlineAuthListCount;i++)
			{
				pItem = Common_Json_GetAttrValue(pArray, i, NULL ,NULL, &pValue, NULL, NULL);
				if(pItem)
				{
					if(pUserCfg->szOnlineAuthManList[i])
						Common_Free(pUserCfg->szOnlineAuthManList[i], __FUNCTION__, __LINE__);
					pUserCfg->szOnlineAuthManList[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				}
			}
		}
		if(!pArray ||(0 == pUserCfg->byOnlineAuthListCount))
		{
			pUserCfg->byOnlineAuthListCount = 1;
			pUserCfg->szOnlineAuthManList[0] = Common_StrDup(pUserName, __FUNCTION__, __LINE__);
		}
		if(pUserCfg->szOnlineAuthManList[0] && (0 != Common_StrCmp(pUserCfg->szUserName, pUserCfg->szOnlineAuthManList[0])))
		{
			if(pUserCfg->szUserName)
				Common_Free(pUserCfg->szUserName, __FUNCTION__, __LINE__);
			pUserCfg->szUserName = Common_StrDup(pUserCfg->szOnlineAuthManList[0], __FUNCTION__, __LINE__);
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "BindIPv4" ,NULL, &pValue, NULL, NULL);
		if(pItem)
		{
			pUserCfg->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "BindIPv6" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			pUserCfg->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "BindMAC" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			pUserCfg->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pRight = (AccessUserRight_T *)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
		if(pRight){
			memset(pRight,0,sizeof(AccessUserRight_T));
			set_defaultUsrRight(pRight,pUserCfg->byPriority);
			pUserCfg->pLocalRight = pRight;
		}
		pRight = (AccessUserRight_T *)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
		if(pRight){
			memset(pRight,0,sizeof(AccessUserRight_T));
			set_defaultUsrRight(pRight,pUserCfg->byPriority);
			pUserCfg->pRemoteRight = pRight;
		}
		LOGD("Add user[%s] succ\n",pUserName);
		g_iCfgChange = OVFS_CFG_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
		bOk = 1;
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	//if(bOk)			//已经连接的用户不处理
	//	refreshOnlineUserInfo(pAccessUsrCfgMgr);
	if(bOk)
		ovfs_make_result(0, "succ", NULL, pOutData);
	else
		ovfs_make_result(-1, "failed", NULL, pOutData);
	return 0;

}

int get_access_usercfg(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	U8 bAll = 0,bNull = 0;	//0:ALL	1:单个	bNull---0:不包括NULL用户  1:包括null用户
	S32 iArrayCount = 0;
	S8 *pValue = NULL,*pUserName = NULL;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pArray1 = NULL,*pItem = NULL,*pItem1 = NULL,*pChild = NULL;

    if(pAddData && Common_Json_Size((cJSON_Struct *)pAddData)>0)
    {
        pJCondition = (cJSON_Struct *)pAddData;
    }

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(pJCondition)		//所有用户或单个用户
	{
		pItem = Common_Json_GetAttrValue(pJCondition, -1, "UserName" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			pUserName = pValue;
		if(!pUserName)
		{
			LOGE("pUserName == NULL\n");
			return -1;
		}
		if(0 == Common_StriCmp(pUserName, (S8 *)"all"))
		{
			bAll = 1;
			bNull = 1;
		}
		else
		{
			bAll = 0;
		}
	}
	else					//(除null外?所有用户
	{
		bAll = 1;
		bNull = 0;
	}

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;

		do{
			int iIndex = 0;
			if(!pUserCfg)
				break;
			if(0 == bAll)
			{
				if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
				{
				}
				else
				{
					pUserCfg= pUserCfg->pNext;
					continue;
				}
			}
			else
			{
				if(bNull == 0)			//单用户
				{
					if(0 == Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
					{
						pUserCfg= pUserCfg->pNext;
						continue;
					}
				}
			}
			Common_Json_SetAttrValue(pArray,iArrayCount,"UserName",Common_Json_Type_String,pUserCfg->szUserName,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"EncryptMethod",Common_Json_Type_Number,NULL,pUserCfg->byEncryptMethod,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"DefaultPwd",Common_Json_Type_String,pUserCfg->szDefaultPassword,0,0);
			pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"Password",Common_Json_Type_Array,NULL,0,0);
			for(iIndex = 0;iIndex < ACCESS_USERCFG_PWD_MAX_NUM;iIndex++)
			{
				Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_String,pUserCfg->szPassword[iIndex],0,0);
			}

			Common_Json_SetAttrValue(pArray,iArrayCount,"Priority",Common_Json_Type_Number,NULL,pUserCfg->byPriority,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"Forbidden",Common_Json_Type_Number,NULL,pUserCfg->bForbidden,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"Remote",Common_Json_Type_Number,NULL,pUserCfg->bRemote,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"NeedOnlineAuth",Common_Json_Type_Number,NULL,pUserCfg->bNeedOnlineAuth,0);
			pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"OnlineAuthManList",Common_Json_Type_Array,NULL,0,0);
			for(iIndex = 0;iIndex < pUserCfg->byOnlineAuthListCount;iIndex++)
			{
				Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_String,pUserCfg->szOnlineAuthManList[iIndex],0,0);
			}
			Common_Json_SetAttrValue(pArray,iArrayCount,"BindIPv4",Common_Json_Type_String,pUserCfg->szBindIpv4,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"BindIPv6",Common_Json_Type_String,pUserCfg->szBindIpv6,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"BindMAC",Common_Json_Type_String,pUserCfg->szBindMac,0,0);
			if(pUserCfg->pLocalRight){
				AccessUserRight_T *pRight = pUserCfg->pLocalRight;
				U32 u32Right = get_userRight(pRight);
				pItem1 = Common_Json_SetAttrValue(pArray,iArrayCount,"LocalRight",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32Right,0);
				if(pRight->pChanRight&&pRight->nChanRightCount > 0){
					AccessChanRight_T *pChanRight = pRight->pChanRight;
					pArray1 = Common_Json_SetAttrValue(pItem1,-1,"ChanRight",Common_Json_Type_Array,NULL,0,0);

					for(iIndex = 0;iIndex < pRight->nChanRightCount;iIndex++){
						U32 u32ChanRight = get_chanRight(&pChanRight[iIndex]);

						pItem1 = Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pItem1,-1,"DeviceNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wDeviceNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"ChannelNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wChannelNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32ChanRight,0);
					}
				}
			}
			if(pUserCfg->pRemoteRight){
				AccessUserRight_T *pRight = pUserCfg->pRemoteRight;
				U32 u32Right = get_userRight(pRight);
				pItem1 = Common_Json_SetAttrValue(pArray,iArrayCount,"RemoteRight",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32Right,0);
				if(pRight->pChanRight&&pRight->nChanRightCount > 0){
					AccessChanRight_T *pChanRight = pRight->pChanRight;
					pArray1 = Common_Json_SetAttrValue(pItem1,-1,"ChanRight",Common_Json_Type_Array,NULL,0,0);
					pItem1 = Common_Json_SetAttrValue(pArray1,-1,NULL,Common_Json_Type_Object,NULL,0,0);
					for(iIndex = 0;iIndex < pRight->nChanRightCount;iIndex++){
						U32 u32ChanRight = get_chanRight(&pChanRight[iIndex]);

						pItem1 = Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pItem1,-1,"DeviceNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wDeviceNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"ChannelNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wChannelNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32ChanRight,0);
					}
				}
			}
			iArrayCount++;
			pUserCfg= pUserCfg->pNext;
		}while(pUserCfg);
	}
	//ovfs_print_json(pResult);
	*pOutData = pResult;
	return 0;
}

int put_access_usercfg(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bChange = 0,bSave = 0;
	S8 *pUserName = NULL,*pValue = NULL,*pTemp = NULL;
	S32 i = 0,iRet = -1,iIndex = 0,iObjType = 0,iArrayCount = 0,iValue = -1,iCount = 0;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pItem = NULL,*pItem1 = NULL;

	//ovfs_print_json(pJData);
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);

	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	if(!pUserCfg)
	{
		LOGE("pUserCfg==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		pValue = NULL;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "UserName" ,NULL, &pValue, NULL, NULL);
		if(pItem)
		{
			pUserName = pValue;
			if(!pUserName)
			{
				LOGE("pUserName==null\n");
				continue;
			}
			//superuser is not modify;
			if(0 == Common_StrCmp((S8*)"(null)", pUserName))
			{
				continue;
			}
			while(pUserCfg)
			{
				if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
				{
					break;
				}
				pUserCfg = pUserCfg->pNext;
			}
		}
		else
		{
			continue;
		}
		if(!pUserCfg)
			continue;
		pValue = NULL;
		pItem1 = Common_Json_GetAttrValue(pJData, iIndex, "DefaultPwd" ,NULL, &pValue, NULL, NULL);
		if(pItem1)
		{
			if(0 == checkIsValid(pValue,strlen(pValue)))
			{
				LOGE("DefaultPwd invalid[%s]\n",pValue);
				continue;
			}
		}
		pArray = Common_Json_GetItem(pJData, iIndex, "Password");
		if(pArray)
		{
			cJSON_Struct *pItem2 = NULL;
			S32 iPwdLen = 0;
			bool bIsInvalid = 0;
			iCount = Common_Json_Size(pArray);
			for(i = 0;i < iCount;i++)
			{
				pValue = NULL;
				pItem2 = Common_Json_GetAttrValue(pArray, i, NULL ,NULL, &pValue, NULL, NULL);
				if(pItem2)
				{
					iPwdLen = strlen(pValue);
					if(0 == checkIsValid(pValue,iPwdLen))
					{
						LOGE("password invalid[%s]\n",pValue);
						bIsInvalid = 1;
						break;
					}
				}
			}
			if(bIsInvalid)
				continue;
		}
        iValue = -1;
		pItem1 = Common_Json_GetAttrValue(pJData, iIndex, "Priority" ,NULL, NULL, &iValue, NULL);
		if(pItem1 && (pUserCfg->byPriority!=iValue))
		{
            if(iValue >= LEVEL_ADMIN)
            {
                LOGE("Priority can't be modified to admin!\n");
                continue;
            }
		}
		iValue = -1;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "EncryptMethod" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->byEncryptMethod!=iValue))
		{
			bChange = 1;
			pUserCfg->byEncryptMethod = iValue;
		}
		pValue = NULL;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "DefaultPwd" ,NULL, &pValue, NULL, NULL);
		if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szDefaultPassword)))
		{
			bChange = 1;
			if(pUserCfg->szDefaultPassword)
				Common_Free(pUserCfg->szDefaultPassword, __FUNCTION__, __LINE__);
			//pUserCfg->szDefaultPassword = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pUserCfg->szDefaultPassword = static_EncryptString(SYSTEM_TEXT,pValue,pTemp,0);
			pValue = NULL;
			pTemp = NULL;
		}
		pArray = Common_Json_GetItem(pJData, iIndex, "Password");
		if(pArray)
		{
			iCount = Common_Json_Size(pArray);
			pUserCfg->byPwdCount = iCount;
			for(i = 0;i < iCount;i++){
				pItem = Common_Json_GetAttrValue(pArray, i, NULL ,NULL, &pValue, NULL, NULL);
				if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szPassword[i])))
				{
					bChange = 1;
					if(pUserCfg->szPassword[i])
						Common_Free(pUserCfg->szPassword[i], __FUNCTION__, __LINE__);
					//pUserCfg->szPassword[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pUserCfg->szPassword[i] = static_EncryptString(SYSTEM_TEXT,pValue,pTemp,0);
					pValue = NULL;
					pTemp = NULL;
				}
			}
		}
		pValue = NULL;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "TempPassword" ,NULL, &pValue, NULL, NULL);
		if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szTempPassword)))
		{
			bChange = 1;
			if(pUserCfg->szTempPassword)
				Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
			pUserCfg->szTempPassword = static_EncryptString(SYSTEM_TEXT,pValue,pTemp,0);
			pValue = NULL;
			pTemp = NULL;
		}
		iValue = -1;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "TempCreateTime" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->tTempCreateTime!=iValue))
		{
			bChange = 1;
			pUserCfg->tTempCreateTime = iValue;
		}
		iValue = -1;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "TempValidTime" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->tTempValidTime!=iValue))
		{
			bChange = 1;
			pUserCfg->tTempValidTime = iValue;
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "Priority" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->byPriority!=iValue))
		{
			bChange = 1;
			pUserCfg->byPriority = iValue;
			set_defaultUsrRight(pUserCfg->pLocalRight,pUserCfg->byPriority);
			set_defaultUsrRight(pUserCfg->pRemoteRight,pUserCfg->byPriority);
		}
		iValue = -1;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "Forbidden" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->bForbidden!=iValue))
		{
			bChange = 1;
			pUserCfg->bForbidden = iValue;
		}
		iValue = -1;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "Remote" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->bRemote!=iValue))
		{
			bChange = 1;
			pUserCfg->bRemote = iValue;
		}
		iValue = -1;
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "NeedOnlineAuth" ,NULL, NULL, &iValue, NULL);
		if(pItem&&(pUserCfg->bNeedOnlineAuth!=iValue))
		{
			bChange = 1;
			pUserCfg->bNeedOnlineAuth = iValue;
		}
		iValue = -1;
		pArray = Common_Json_GetItem(pJData, iIndex, "OnlineAuthManList");
		if(pArray)
		{
			pUserCfg->byOnlineAuthListCount = Common_Json_Size(pArray);
			if(pUserCfg->byOnlineAuthListCount > 0){
				for(i = 0;i < pUserCfg->byOnlineAuthListCount;i++)
				{
					pItem = Common_Json_GetAttrValue(pArray, i, NULL ,NULL, &pValue, NULL, NULL);
					if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szOnlineAuthManList[i])))
					{
						bChange = 1;
						if(pUserCfg->szOnlineAuthManList[i])
							Common_Free(pUserCfg->szOnlineAuthManList[i], __FUNCTION__, __LINE__);
						pUserCfg->szOnlineAuthManList[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
						pValue = NULL;
					}
				}
			}
			else
			{
				pUserCfg->byOnlineAuthListCount = 1;
				for(i = 0;i < ACCESS_USERCFG_AUTH_MAX_NUM;i++)
				{
					if(pUserCfg->szOnlineAuthManList[i])
						Common_Free(pUserCfg->szOnlineAuthManList[i], __FUNCTION__, __LINE__);
					pUserCfg->szOnlineAuthManList[i] = NULL;
				}
				pUserCfg->szOnlineAuthManList[0] = Common_StrDup(pUserCfg->szUserName, __FUNCTION__, __LINE__);
			}
		}
		if(pUserCfg->szOnlineAuthManList[0] &&(0 != Common_StrCmp(pUserCfg->szUserName, pUserCfg->szOnlineAuthManList[0])))
		{
			if(pUserCfg->szUserName)
				Common_Free(pUserCfg->szUserName, __FUNCTION__, __LINE__);
			pUserCfg->szUserName = Common_StrDup(pUserCfg->szOnlineAuthManList[0], __FUNCTION__, __LINE__);
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "BindIPv4" ,NULL, &pValue, NULL, NULL);
		if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szBindIpv4)))
		{
			bChange = 1;
			if(pUserCfg->szBindIpv4)
				Common_Free(pUserCfg->szBindIpv4, __FUNCTION__, __LINE__);
			pUserCfg->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "BindIPv6" ,NULL, &pValue, NULL, NULL);
		if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szBindIpv6)))
		{
			bChange = 1;
			if(pUserCfg->szBindIpv6)
				Common_Free(pUserCfg->szBindIpv6, __FUNCTION__, __LINE__);
			pUserCfg->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "BindMAC" ,NULL, &pValue, NULL, NULL);
		if(pItem&&(0!=Common_StrCmp(pValue,pUserCfg->szBindMac)))
		{
			bChange = 1;
			if(pUserCfg->szBindMac)
				Common_Free(pUserCfg->szBindMac, __FUNCTION__, __LINE__);
			pUserCfg->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
		}

		pItem = Common_Json_GetItem(pJData, iIndex, "LocalRight");
		if(pItem)
		{
			U32 u32Right = 0;
			AccessUserRight_T *pRight = pUserCfg->pLocalRight;
			Common_Json_GetAttrValue(pItem, -1, "RightMask" ,NULL, NULL, (S32*)&u32Right, NULL);
			bChange = set_userRight(pRight, u32Right);
			pArray = Common_Json_GetItem(pItem, -1, "ChanRight");
			if(pArray){
				AccessChanRight_T *pChanRight = pRight->pChanRight;
				iCount = Common_Json_Size(pArray);
				for(i = 0;i < iCount;i++){
					pItem = Common_Json_GetItem(pArray, i, NULL);
					if(pItem)
					{
						U32 u32ChanRight = 0;
						pItem1 = Common_Json_GetAttrValue(pItem, -1, "RightMask" ,NULL, NULL, (S32*)&u32ChanRight, NULL);
						if(pItem1)
						{
							Common_Json_GetAttrValue(pItem, -1, "DeviceNo" ,NULL, NULL, (S32*)&pChanRight[i].wDeviceNo, NULL);
							Common_Json_GetAttrValue(pItem, -1, "ChannelNo" ,NULL, NULL, (S32*)&pChanRight[i].wChannelNo, NULL);
							bChange = set_chanRight(&pChanRight[i], u32ChanRight);
						}
					}
				}
			}
		}
		pItem = Common_Json_GetItem(pJData, iIndex, "RemoteRight");
		if(pItem)
		{
			U32 u32Right = 0;
			AccessUserRight_T *pRight = pUserCfg->pRemoteRight;
			pItem1 = Common_Json_GetAttrValue(pItem, -1, "RightMask" ,NULL, NULL, (S32*)&u32Right, NULL);
			if(pItem1)
				bChange = set_userRight(pRight, u32Right);
			pArray = Common_Json_GetItem(pItem, -1, "ChanRight");
			if(pArray){
				AccessChanRight_T *pChanRight = pRight->pChanRight;
				iCount = Common_Json_Size(pArray);
				for(i = 0;i < iCount;i++){
					pItem = Common_Json_GetItem(pArray, i, NULL);
					if(pItem)
					{
						U32 u32ChanRight = 0;
						pItem1 = Common_Json_GetAttrValue(pItem, -1, "RightMask" ,NULL, NULL, (S32*)&u32ChanRight, NULL);
						if(pItem1)
						{
							Common_Json_GetAttrValue(pItem, -1, "DeviceNo" ,NULL, NULL, (S32*)&pChanRight[i].wDeviceNo, NULL);
							Common_Json_GetAttrValue(pItem, -1, "ChannelNo" ,NULL, NULL, (S32*)&pChanRight[i].wChannelNo, NULL);
							bChange = set_chanRight(&pChanRight[i], u32ChanRight);
						}
					}
				}
			}
		}
		bSave = 1;
	}
	if(bSave && bChange)
	{
		g_iCfgChange = OVFS_CFG_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
		//refreshOnlineUserInfo(pAccessUsrCfgMgr);	//已经连接的用户不处理
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	if(bSave)
		ovfs_make_result(0, "succ", NULL, pOutData);
	else
		ovfs_make_result(-1, "failed", NULL, pOutData);
	LOGD("set user succ\n");
	return iRet;

}

int delete_access_usercfg(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 *pValue = NULL,*pUserName = NULL;
	AccessUserCfg *pUserCfg = NULL,*pTempCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition,*pItem = NULL;

    if(pAddData)
    {
        pJCondition = (cJSON_Struct *)pAddData;
    }

	do{
		if(!pAccessUsrCfgMgr)
		{
			LOGE("pAccessUsrCfgMgr == NULL!\n");
			break;
		}
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;
		if(!pUserCfg)
		{
			LOGE("pUserCfg == NULL!\n");
			break;
		}
		if(pJCondition)
		{
			pItem = Common_Json_GetAttrValue(pJCondition, -1, "UserName" ,NULL, &pValue, NULL, NULL);
			if(pItem)
				pUserName = pValue;
			if(!pUserName)
			{
				LOGE("pUserName == NULL\n");
				break;
			}
			pUserCfg = pAccessUsrCfgMgr->pUserCfg;
			while(pUserCfg)
			{
				if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
				{
					if(pUserCfg->byPriority == LEVEL_ADMIN)
					{
						LOGE("No delete admin!\n");
						break;
					}
					Common_Lock(g_subscribeMgr.hCfgLock);
					pTempCfg = pUserCfg->pNext;
					if(pUserCfg->pPrev)
						pUserCfg->pPrev->pNext = pUserCfg->pNext;
					if(pTempCfg)
						pTempCfg->pPrev = pUserCfg->pPrev;
					if(pUserCfg->szUserName)
						Common_Free(pUserCfg->szUserName, __FUNCTION__, __LINE__);
					if(pUserCfg->szDefaultPassword)
						Common_Free(pUserCfg->szDefaultPassword, __FUNCTION__, __LINE__);
					if(pUserCfg->szTempPassword)
						Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
					if(pUserCfg->szSerialNumber)
						Common_Free(pUserCfg->szSerialNumber, __FUNCTION__, __LINE__);
					for(int i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
					{
						if(pUserCfg->szPassword[i])
							Common_Free(pUserCfg->szPassword[i], __FUNCTION__, __LINE__);
					}
					for(int i = 0;i < ACCESS_USERCFG_AUTH_MAX_NUM;i++){
						if(pUserCfg->szOnlineAuthManList[i])
							Common_Free(pUserCfg->szOnlineAuthManList[i], __FUNCTION__, __LINE__);
					}
					if(pUserCfg->szBindIpv4)
						Common_Free(pUserCfg->szBindIpv4, __FUNCTION__, __LINE__);
					if(pUserCfg->szBindIpv6)
						Common_Free(pUserCfg->szBindIpv6, __FUNCTION__, __LINE__);
					if(pUserCfg->szBindMac)
						Common_Free(pUserCfg->szBindMac, __FUNCTION__, __LINE__);
					if(pUserCfg->pLocalRight)
					{
						if(pUserCfg->pLocalRight->pChanRight)
							Common_Free(pUserCfg->pLocalRight->pChanRight, __FUNCTION__, __LINE__);
						Common_Free(pUserCfg->pLocalRight, __FUNCTION__, __LINE__);
					}
					if(pUserCfg->pRemoteRight){
						if(pUserCfg->pRemoteRight->pChanRight)
							Common_Free(pUserCfg->pRemoteRight->pChanRight, __FUNCTION__, __LINE__);
						Common_Free(pUserCfg->pRemoteRight, __FUNCTION__, __LINE__);
					}
					if(pUserCfg->pCreateTime)
						Common_Free(pUserCfg->pCreateTime, __FUNCTION__, __LINE__);
					if(pUserCfg->pStopTime)
						Common_Free(pUserCfg->pStopTime, __FUNCTION__, __LINE__);
					pUserCfg->pBindFromUser = NULL;
					pUserCfg->pBindToUser = NULL;
					pUserCfg->pBindUserNext = NULL;
					pUserCfg->pBindUserPrev = NULL;
					if(pUserCfg)
						Common_Free(pUserCfg, __FUNCTION__, __LINE__);
					pUserCfg = NULL;
					LOGD("Delete user[%s] succ\n",pUserName);
					g_iCfgChange = OVFS_CFG_FLAG|0x01;
					access_saveCfg(g_subscribeMgr.hModuleHandle);
					//refreshOnlineUserInfo(pAccessUsrCfgMgr);		//已经连接的用户不处理
					Common_UnLock(g_subscribeMgr.hCfgLock);
					ovfs_make_result(0, "succ", NULL, pOutData);
					return 0;
				}
				pUserCfg = pUserCfg->pNext;
			}
		}
	}while(0);
	ovfs_make_result(-1, "failed", NULL, pOutData);
	return -1;
}

int get_access_findpwd(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iRet = -1;
	S8 *pUserName = NULL,*pValue = NULL;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJData = (cJSON_Struct *)pCondition,*pResult = NULL;

    if(pAddData)
    {
        pJData = (cJSON_Struct *)pAddData;
    }

	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	if(!pUserCfg)
	{
		LOGE("pUserCfg==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	Common_Lock(g_subscribeMgr.hCfgLock);
	pValue = NULL;
	Common_Json_GetAttrValue(pJData, -1, "UserName" ,NULL, &pValue, NULL, NULL);
	pUserName = pValue;
	if(!pUserName)
	{
		pUserName = (S8*)"(null)";
	}
	while(pUserCfg)
	{
		if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
			break;
		pUserCfg = pUserCfg->pNext;
	}
	if(!pUserCfg)
	{
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_USER_NOTEXIST, "failed,the user is not exist", NULL, pOutData);
		return -1;
	}

	pValue = NULL;
	if(!pUserCfg->szTempPassword)
		pUserCfg->szTempPassword = static_GenTempPwd(pValue, 0, NULL, RESET_PWD_MAX_NUM);
	if(!pUserCfg->szSerialNumber)
		pUserCfg->szSerialNumber = static_GetDevSerialNumber();
	pUserCfg->tTempCreateTime = time(NULL);
	pUserCfg->tTempValidTime = 7*24*60*60;
	pUserCfg->tFailTime = 0;
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Data/SerialNumber",Common_Json_Type_String,pUserCfg->szSerialNumber,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Data/Token",Common_Json_Type_String,pUserCfg->szTempPassword,0,0);
	}
	g_iCfgChange = OVFS_CFG_FLAG|0x01;
#if 0
	access_saveCfg(g_subscribeMgr.hModuleHandle);
#endif

	*pOutData = pResult;
	Common_UnLock(g_subscribeMgr.hCfgLock);
	return iRet;
}

int put_access_invalidpwd(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 *pUserName = NULL;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pItem = NULL;

	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	if(!pUserCfg)
	{
		LOGE("pUserCfg==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	Common_Lock(g_subscribeMgr.hCfgLock);
	pItem = Common_Json_GetAttrValue(pJData, -1, "UserName" ,NULL, &pUserName, NULL, NULL);
	if(pItem)
	{
		while(pUserCfg)
		{
			if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
				break;
			pUserCfg = pUserCfg->pNext;
		}
		if(!pUserCfg)
		{
			Common_UnLock(g_subscribeMgr.hCfgLock);
			ovfs_make_result(ERR_USER_NOTEXIST, "failed,the user is not exist", NULL, pOutData);
			return -1;
		}
		if(pUserCfg->szTempPassword)
			Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
		pUserCfg->szTempPassword = NULL;
		if(pUserCfg->szSerialNumber)
			Common_Free(pUserCfg->szSerialNumber, __FUNCTION__, __LINE__);
		pUserCfg->szSerialNumber = NULL;
		pUserCfg->tTempCreateTime = 0;
		pUserCfg->tTempValidTime = 0;
		pUserCfg->tFailTime = 0;
	}
	else
	{
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	ovfs_make_result(0, "succ", NULL, pOutData);
	return 0;
}

int put_access_restorepwd(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 bChange = 0;
	//U32 nLength = 0;
	time_t t_Curtime = time(NULL);
	S32 tTimeInfo = 0,tOutTimeInfo = 0;
	S8 *pUserName = NULL,*pRestoreInfo = NULL,*pValue = NULL,*pToken = NULL,*pSerialNo = NULL;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pJRestoreInfo = NULL;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	if(!pUserCfg)
	{
		LOGE("pUserCfg==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	Common_Lock(g_subscribeMgr.hCfgLock);
	Common_Json_GetAttrValue(pJData, -1, "UserName" ,NULL, &pUserName, NULL, NULL);
	if(!pUserName)
		pUserName = (S8*)"(null)";
	while(pUserCfg)
	{
		if(0 == Common_StrCmp(pUserCfg->szUserName, pUserName))
			break;
		pUserCfg = pUserCfg->pNext;
	}
	if(!pUserCfg)
	{
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_USER_NOTEXIST, "failed,the user is not exist", NULL, pOutData);
		return -1;
	}
	if(pUserCfg->tFailTime >= 3)
	{
		if(pUserCfg->szTempPassword)
		{
			Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
			pUserCfg->szTempPassword = NULL;
		}
		if(pUserCfg->szSerialNumber)
		{
			Common_Free(pUserCfg->szSerialNumber, __FUNCTION__, __LINE__);
			pUserCfg->szSerialNumber = NULL;
		}
		LOGE("failed,exceed the max retry times\n");
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_RETRY_INVALID, "failed,exceed the max retry times", NULL, pOutData);
		return -1;
	}
	if(pUserCfg->tTempValidTime <= 0)
	{
		pUserCfg->tFailTime++;
		LOGE("failed,TempValidTime %d\n",pUserCfg->tTempValidTime);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_TIME_INVALID, "failed,TempValidTime <= 0", NULL, pOutData);
		return -1;
	}
	if((t_Curtime < pUserCfg->tTempCreateTime) ||(t_Curtime > pUserCfg->tTempCreateTime+pUserCfg->tTempValidTime))
	{
		pUserCfg->tFailTime++;
		LOGE("failed,Curtime %d CreateTime %d ValidTime %d\n",t_Curtime,pUserCfg->tTempCreateTime,pUserCfg->tTempValidTime);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_TIME_INVALID, "failed,curent time is not in the right time", NULL, pOutData);
		return -1;
	}
	pValue = NULL;
	Common_Json_GetAttrValue(pJData, -1, "RestoreInfo" ,NULL, &pValue, NULL, NULL);

	//pRestoreInfo = Common_Base64_Decode(pValue, strlen(pValue), &nLength);
	pRestoreInfo = static_DecryptString(pValue, NULL, NULL, 128);
	if(!pRestoreInfo)
	{
		pUserCfg->tFailTime++;
		LOGE("failed,pRestoreInfo is null\n");
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_INFO_INVALID, "failed", NULL, pOutData);
		return -1;
	}
	pJRestoreInfo = Common_Json_Parse(pRestoreInfo,NULL,NULL);
	//ovfs_print_json(pJRestoreInfo);
	Common_Free(pRestoreInfo, __FUNCTION__, __LINE__);
	#if 0
	pValue = NULL;
	Common_Json_GetAttrValue(pJRestoreInfo, -1, "Time" ,NULL, &pValue, NULL, NULL);
	String2Time(pValue,14,NULL,&tTimeInfo);
	if(t_Curtime < tTimeInfo)
	{
		pUserCfg->tFailTime++;
		ovfs_print_json(pJRestoreInfo);
		Common_Json_Delete(pJRestoreInfo);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_INFO_INVALID, "failed,Time is invalid", NULL, pOutData);
		return -1;
	}
	#endif
	pValue = NULL;
	Common_Json_GetAttrValue(pJRestoreInfo, -1, "OutTime" ,NULL, &pValue, NULL, NULL);
	String2Time(pValue,14,NULL,&tOutTimeInfo);
	if(t_Curtime > tOutTimeInfo)
	{
		pUserCfg->tFailTime++;
		ovfs_print_json(pJRestoreInfo);
		Common_Json_Delete(pJRestoreInfo);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_INFO_INVALID, "failed,OutTime is invalid", NULL, pOutData);
		return -1;
	}
	if(tTimeInfo > tOutTimeInfo)
	{
		pUserCfg->tFailTime++;
		ovfs_print_json(pJRestoreInfo);
		Common_Json_Delete(pJRestoreInfo);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_INFO_INVALID, "failed,RestoreInfo is not right", NULL, pOutData);
		return -1;
	}
	pValue = NULL;
	Common_Json_GetAttrValue(pJRestoreInfo, -1, "SerialNumber" ,NULL, &pSerialNo, NULL, NULL);
	if(0 != Common_StrCmp(pSerialNo, pUserCfg->szSerialNumber))
	{
		pUserCfg->tFailTime++;
		ovfs_print_json(pJRestoreInfo);
		Common_Json_Delete(pJRestoreInfo);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_SERIALNO_INVALID, "failed,serialnumber is not right", NULL, pOutData);
		return -1;
	}
	pValue = NULL;
	Common_Json_GetAttrValue(pJRestoreInfo, -1, "Token" ,NULL, &pToken, NULL, NULL);
	if(0 != Common_StrCmp(pToken, pUserCfg->szTempPassword))
	{
		pUserCfg->tFailTime++;
		ovfs_print_json(pJRestoreInfo);
		Common_Json_Delete(pJRestoreInfo);
		Common_UnLock(g_subscribeMgr.hCfgLock);
		ovfs_make_result(ERR_TOKEN_INVALID, "failed,token is not right", NULL, pOutData);
		return -1;
	}
	if(0 == Common_StrCmp(pUserCfg->szUserName, (S8*)"(null)"))
	{
		#if 0
		remove(USER_CONFIG_PATH);
		#else
		AccessUserCfg *pTmpCfg  = pAccessUsrCfgMgr->pUserCfg;
		while(pTmpCfg)
		{
			if(0 != Common_StrCmp(pTmpCfg->szUserName, pUserName))
			{
				pTmpCfg->byPwdCount = 1;
				for(int i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
				{
					if(pTmpCfg->szPassword[i])
						Common_Free(pTmpCfg->szPassword[i], __FUNCTION__, __LINE__);
					pTmpCfg->szPassword[i] = NULL;
				}
				//pTmpCfg->szPassword[0] = static_EncryptString(SYSTEM_TEXT, (S8*)"123456", NULL, 0);
				pTmpCfg->szPassword[0] = Common_StrDup(pTmpCfg->szDefaultPassword, __FUNCTION__, __LINE__);
				bChange = 1;
			}
			pTmpCfg = pTmpCfg->pNext;
		}
		#endif
	}
	else
	{
		pUserCfg->byPwdCount = 1;
		for(int i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
		{
			if(pUserCfg->szPassword[i])
				Common_Free(pUserCfg->szPassword[i], __FUNCTION__, __LINE__);
			pUserCfg->szPassword[i] = NULL;
		}
		//pUserCfg->szPassword[0] = static_EncryptString(SYSTEM_TEXT, (S8*)"123456", NULL, 0);
		pUserCfg->szPassword[0] = Common_StrDup(pUserCfg->szDefaultPassword, __FUNCTION__, __LINE__);
		bChange = 1;

	}
	if(pUserCfg->szTempPassword)
		Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
	pUserCfg->szTempPassword = NULL;
	if(pUserCfg->szSerialNumber)
		Common_Free(pUserCfg->szSerialNumber, __FUNCTION__, __LINE__);
	pUserCfg->szSerialNumber = NULL;
	pUserCfg->tTempCreateTime = 0;
	pUserCfg->tTempValidTime = 0;
	pUserCfg->tFailTime = 0;
	Common_Json_Delete(pJRestoreInfo);

	if(bChange)
	{
		g_iCfgChange = OVFS_CFG_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	ovfs_make_result(ERR_NONE, "succ", NULL, pOutData);
	return 0;
}

int notice_usercfg_change(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	U8 bAll = 0,bNull = 0;	//0:ALL	1:单个	bNull---0:不包括NULL用户  1:包括null用户
	S32 iArrayCount = 0;
	S8 *pValue = NULL,*pUserName = NULL;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pArray1 = NULL,*pItem = NULL,*pItem1 = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(pJCondition)		//所有用户或单个用户
	{
		pItem = Common_Json_GetAttrValue(pJCondition, -1, "UserName" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			pUserName = pValue;
		if(!pUserName)
		{
			LOGE("pUserName == NULL\n");
			ovfs_make_result(-1, "failed", NULL, pOutData);
			return -1;
		}
		if(0 == Common_StriCmp(pUserName, (S8 *)"all"))
		{
			bAll = 1;
			bNull = 1;
		}
		else
		{
			bAll = 0;
		}
	}
	else					//(除null外?所有用户
	{
		bAll = 1;
		bNull = 0;
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		pArray = Common_Json_SetAttrValue(pResult,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;
		if(!pUserCfg)
		{
			LOGE("pUserCfg == null\n");
			Common_Json_Delete(pResult);
			pResult = NULL;
		}
		while(pUserCfg){
			int iIndex = 0;
			//if(0 != Common_StriCmp(pUserCfg->szUserName, (S8 *)"default"))
			//if(0 != Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
			if(0 == bAll)
			{
				if(0 != Common_StrCmp(pUserCfg->szUserName, pUserName))
				{
					pUserCfg= pUserCfg->pNext;
					continue;
				}
			}
			else
			{
				if(bNull == 0)			//单用户
				{
					if(0 == Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
					{
						pUserCfg= pUserCfg->pNext;
						continue;
					}
				}
			}
			Common_Json_SetAttrValue(pArray,iArrayCount,"UserName",Common_Json_Type_String,pUserCfg->szUserName,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"EncryptMethod",Common_Json_Type_Number,NULL,pUserCfg->byEncryptMethod,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"DefaultPwd",Common_Json_Type_String,pUserCfg->szDefaultPassword,0,0);
			pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"Password",Common_Json_Type_Array,NULL,0,0);
			for(iIndex = 0;iIndex < ACCESS_USERCFG_PWD_MAX_NUM;iIndex++)
			{
				Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_String,pUserCfg->szPassword[iIndex],0,0);
			}
			Common_Json_SetAttrValue(pArray,iArrayCount,"TempPassword",Common_Json_Type_String,pUserCfg->szTempPassword,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"SerialNumber",Common_Json_Type_String,pUserCfg->szSerialNumber,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"TempCreateTime",Common_Json_Type_Number,NULL,pUserCfg->tTempCreateTime,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"TempValidTime",Common_Json_Type_Number,NULL,pUserCfg->tTempValidTime,0);

			Common_Json_SetAttrValue(pArray,iArrayCount,"Priority",Common_Json_Type_Number,NULL,pUserCfg->byPriority,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"Forbidden",Common_Json_Type_Number,NULL,pUserCfg->bForbidden,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"Remote",Common_Json_Type_Number,NULL,pUserCfg->bRemote,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"NeedOnlineAuth",Common_Json_Type_Number,NULL,pUserCfg->bNeedOnlineAuth,0);
			pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"OnlineAuthManList",Common_Json_Type_Array,NULL,0,0);
			for(iIndex = 0;iIndex < pUserCfg->byOnlineAuthListCount;iIndex++)
			{
				Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_String,pUserCfg->szOnlineAuthManList[iIndex],0,0);
			}
			Common_Json_SetAttrValue(pArray,iArrayCount,"BindIPv4",Common_Json_Type_String,pUserCfg->szBindIpv4,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"BindIPv6",Common_Json_Type_String,pUserCfg->szBindIpv6,0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"BindMAC",Common_Json_Type_String,pUserCfg->szBindMac,0,0);
			if(pUserCfg->pLocalRight){
				AccessUserRight_T *pRight = pUserCfg->pLocalRight;
				U32 u32Right = get_userRight(pRight);
				pItem1 = Common_Json_SetAttrValue(pArray,iArrayCount,"LocalRight",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32Right,0);
				if(pRight->pChanRight&&pRight->nChanRightCount > 0){
					AccessChanRight_T *pChanRight = pRight->pChanRight;
					pArray1 = Common_Json_SetAttrValue(pItem1,-1,"ChanRight",Common_Json_Type_Array,NULL,0,0);

					for(iIndex = 0;iIndex < pRight->nChanRightCount;iIndex++){
						U32 u32ChanRight = get_chanRight(&pChanRight[iIndex]);

						pItem1 = Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pItem1,-1,"DeviceNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wDeviceNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"ChannelNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wChannelNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32ChanRight,0);
					}
				}
			}
			if(pUserCfg->pRemoteRight){
				AccessUserRight_T *pRight = pUserCfg->pRemoteRight;
				U32 u32Right = get_userRight(pRight);
				pItem1 = Common_Json_SetAttrValue(pArray,iArrayCount,"RemoteRight",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32Right,0);
				if(pRight->pChanRight&&pRight->nChanRightCount > 0){
					AccessChanRight_T *pChanRight = pRight->pChanRight;
					pArray1 = Common_Json_SetAttrValue(pItem1,-1,"ChanRight",Common_Json_Type_Array,NULL,0,0);
					pItem1 = Common_Json_SetAttrValue(pArray1,-1,NULL,Common_Json_Type_Object,NULL,0,0);
					for(iIndex = 0;iIndex < pRight->nChanRightCount;iIndex++){
						U32 u32ChanRight = get_chanRight(&pChanRight[iIndex]);

						pItem1 = Common_Json_SetAttrValue(pArray1,iIndex,NULL,Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pItem1,-1,"DeviceNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wDeviceNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"ChannelNo",Common_Json_Type_Number,NULL,pChanRight[iIndex].wChannelNo,0);
						Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32ChanRight,0);
					}
				}
			}
			iArrayCount++;
			pUserCfg= pUserCfg->pNext;
		}
	}
	//ovfs_print_json(pResult);
	*pOutData = pResult;
	return 0;
}

S32 refreshOnlineUserInfo(AccessUserCfgMgr_T *pAccessUsrCfgMgr)
{
	S8 szTemp[64] = {0};
	Common_Time_T t_login;
	S32 i = 0,j = 0,iArrayCount = 0,iCount = 0,nOnlineCount = 0,nModuleId = 0;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pArray1 = NULL,*pItem = NULL;
	AccessUserCfg *pUserCfg = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pUserCfg is null!\n");
		return -1;
	}
	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	//nOnlineCount = pAccessUsrCfgMgr->nOnlineUserCount;
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	pArray= Common_Json_SetAttrValue(pResult,-1,"OnlineList",Common_Json_Type_Array,NULL,0,0);
	do{
		if(!pUserCfg)
		{
			LOGE("pUserCfg = null\n");
			break;
		}
		if(pUserCfg->nSessionCnt > 0){
			pItem = Common_Json_SetAttrValue(pArray,iArrayCount,NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pItem,-1,"UserName",Common_Json_Type_String,pUserCfg->szUserName,0,0);
			Common_Json_SetAttrValue(pItem,-1,"Remote",Common_Json_Type_String,pUserCfg->bRemote?(S8*)"ok":(S8*)"no",0,0);
			if(pUserCfg->bRemote)
			{
				Common_Json_SetAttrValue(pItem,-1,"BindIPv4",Common_Json_Type_String,pUserCfg->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pItem,-1,"BindIPv6",Common_Json_Type_String,pUserCfg->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pItem,-1,"BindMAC",Common_Json_Type_String,pUserCfg->szBindMac,0,0);
			}
			pArray1= Common_Json_SetAttrValue(pItem,-1,"ConnectInfo",Common_Json_Type_Array,NULL,0,0);
			iCount = 0;
			for(i = 0;i < OVFS_IPCONNECT_MAX_NUM;i++)
			{
				IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[i] ;
				if(pIpConnect)
				{
					if(pIpConnect->hSessionId > 0)
					{
						pItem = Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Object,NULL,0,0);
						Common_Json_SetAttrValue(pItem,-1,"Id",Common_Json_Type_Number,NULL,pIpConnect->hSessionId,0);
						Common_Linux2CommonTime(pIpConnect->tCreateTime,&t_login);
						sprintf(szTemp,"%04d-%02d-%02d %02d:%02d:%02d",t_login.year,t_login.month,t_login.day,t_login.hour,t_login.min,t_login.sec);
						Common_Json_SetAttrValue(pItem,-1,"LoginTime",Common_Json_Type_String,szTemp,0,0);
						if(pIpConnect->szBindIpv4)
							Common_Json_SetAttrValue(pItem,-1,"IPv4",Common_Json_Type_String,pIpConnect->szBindIpv4,0,0);
						nModuleId = (pUserCfg->hSessionId[i]>> (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
						for(j = 0;j < MAX_MODULE_COUNT;j++)
						{
							if(g_OnlineModule.nModuleInfo[j].nModuleId == nModuleId && g_OnlineModule.nModuleInfo[j].szName)
							{
								sprintf(szTemp,"%s",g_OnlineModule.nModuleInfo[j].szName);
								Common_Json_SetAttrValue(pItem,-1,"From",Common_Json_Type_String,szTemp,0,0);
								break;
							}
						}
						iCount++;
						nOnlineCount++;
					}
				}
			}
			iArrayCount++;
		}
		pUserCfg = pUserCfg->pNext;
	}while(pUserCfg);
	pAccessUsrCfgMgr->nOnlineUserCount = nOnlineCount;
	Common_Json_SetAttrValue(pResult,-1,"OnlineCount",Common_Json_Type_Number,NULL,nOnlineCount,0);

	pArray= Common_Json_SetAttrValue(pResult,-1,"ModuleMap",Common_Json_Type_Array,NULL,0,0);
	iCount = 0;
	if(g_OnlineModule.nModuleCount > 0)
	{
		for(i = 0;i < MAX_MODULE_COUNT;i++)
		{
			if(g_OnlineModule.nModuleInfo[i].nModuleId > 0 && g_OnlineModule.nModuleInfo[i].szName)
			{
				pItem = Common_Json_SetAttrValue(pArray,iCount,NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem,-1,"ModuleId",Common_Json_Type_Number,NULL,g_OnlineModule.nModuleInfo[i].nModuleId,0);
				Common_Json_SetAttrValue(pItem,-1,"ModuleName",Common_Json_Type_String,g_OnlineModule.nModuleInfo[i].szName,0,0);
				iCount++;
			}
		}
	}
	Common_Json_SetAttrValue(pResult,-1,"ModuleCount",Common_Json_Type_Number,NULL,iCount,0);

	Module_SaveTempData(g_subscribeMgr.hModuleHandle, pResult);
	if(pResult)
		Common_Json_Delete(pResult);
	return 0;
}

//get online user list
int get_online_user(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	U8 bAll = 0,bNull = 0;	//0:ALL	1:单个	bNull---0:不包括NULL用户  1:包括null用户
	S8 *pValue = NULL,*pUserName = NULL;
	S32 iArrayCount = 0,iCount = 0,iModuleId = -1,iCurIdx = -1;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pArray1 = NULL,*pItem = NULL,*pChild = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(pJCondition)		//所有用户或单个用户
	{
		pItem = Common_Json_GetAttrValue(pJCondition, -1, "UserName" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			pUserName = pValue;
		pValue = NULL;
		pItem = Common_Json_GetAttrValue(pJCondition, -1, "ModuleId" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			iModuleId = atoi(pValue);
		pValue = NULL;
		if(!pUserName)
		{
			LOGE("pUserName == NULL\n");
			ovfs_make_result(-1, "failed", NULL, pOutData);
			return -1;
		}
		if(0 == Common_StriCmp(pUserName, (S8 *)"all"))
		{
			bAll = 1;
			bNull = 1;
		}
		else
		{
			bAll = 0;
		}
	}
	else					//(除null外?所有用户
	{
		bAll = 1;
		bNull = 0;
	}

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		if(pAccessUsrCfgMgr->nOnlineUserCount > 0)
		{
			pUserCfg = pAccessUsrCfgMgr->pUserCfg;
			do{
				if(!pUserCfg)
					break;
				if(0 == bAll)
				{
					if(0 != Common_StrCmp(pUserCfg->szUserName, pUserName))
					{
						pUserCfg= pUserCfg->pNext;
						continue;
					}
				}
				else
				{
					if(bNull == 0)			//单用户
					{
					#if 0
						if(0 == Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
						{
							pUserCfg= pUserCfg->pNext;
							continue;
						}
					#endif
					}
				}
				if(pUserCfg->nSessionCnt <= 0||pUserCfg->nIpConnCnt <= 0)
				{
					pUserCfg= pUserCfg->pNext;
					continue;
				}
				if(0 == Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
					Common_Json_SetAttrValue(pArray,iArrayCount,"UserName",Common_Json_Type_String,(S8*)"",0,0);
				else
					Common_Json_SetAttrValue(pArray,iArrayCount,"UserName",Common_Json_Type_String,pUserCfg->szUserName,0,0);
				pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"SessionId",Common_Json_Type_Array,NULL,0,0);
				iCount = 0;
				for(int i = 0;i < MAX_LOGIN_USER;i++)
				{
					if(pUserCfg->hSessionId[i] > 0)
					{
						if(iModuleId < 0)
						{
							Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Number,NULL,pUserCfg->hSessionId[i],0);
							iCount++;
						}
						else
						{
							iCurIdx = (pUserCfg->hSessionId[i]>> (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
							if(iCurIdx == iModuleId)
							{
								Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Number,NULL,pUserCfg->hSessionId[i],0);
								iCount++;
							}
						}
					}
				}
				pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"ConnectInfo",Common_Json_Type_Array,NULL,0,0);
				iCount = 0;
				if(pUserCfg->nIpConnCnt > 0)
				{
					for(int i = 0;i < OVFS_IPCONNECT_MAX_NUM;i++)
					{
						IpConnectInfo_T *pIpConnect =  pUserCfg->pIpConnList[i];
						if(pIpConnect)
						{
							if(pIpConnect->hSessionId > 0)
							{
								if(iModuleId < 0)
								{
									S32 iTmpModuleId = pIpConnect->hSessionId;
									pItem = Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Object,NULL,0,0);
									Common_Json_SetAttrValue(pItem,-1,"SessionId",Common_Json_Type_Number,NULL,pIpConnect->hSessionId,0);
									if(pIpConnect->szBindIpv4)
										Common_Json_SetAttrValue(pItem,-1,"IPv4",Common_Json_Type_String,pIpConnect->szBindIpv4,0,0);
									Common_Json_SetAttrValue(pItem,-1,"CreateTime",Common_Json_Type_Number,NULL,pIpConnect->tCreateTime,0);

									iTmpModuleId = (iTmpModuleId>> (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
									for(int j = 0;j < MAX_MODULE_COUNT;j++)
									{
										if(g_OnlineModule.nModuleInfo[j].nModuleId == iTmpModuleId && g_OnlineModule.nModuleInfo[j].szName)
										{
											S8 szTemp[64] = {0};
											sprintf(szTemp,"%s",g_OnlineModule.nModuleInfo[j].szName);
											Common_Json_SetAttrValue(pItem,-1,"From",Common_Json_Type_String,szTemp,0,0);
											break;
										}
									}
									iCount++;
								}
								else
								{
									iCurIdx = (pUserCfg->hSessionId[i]>> (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
									if(iCurIdx == iModuleId)
									{
										pItem = Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Object,NULL,0,0);
										Common_Json_SetAttrValue(pItem,-1,"SessionId",Common_Json_Type_Number,NULL,pIpConnect->hSessionId,0);
										if(pIpConnect->szBindIpv4)
											Common_Json_SetAttrValue(pItem,-1,"IPv4",Common_Json_Type_String,pIpConnect->szBindIpv4,0,0);
										Common_Json_SetAttrValue(pItem,-1,"CreateTime",Common_Json_Type_Number,NULL,pIpConnect->tCreateTime,0);
										for(int j = 0;j < MAX_MODULE_COUNT;j++)
										{
											if(g_OnlineModule.nModuleInfo[j].nModuleId == iModuleId && g_OnlineModule.nModuleInfo[j].szName)
											{
												S8 szTemp[64] = {0};
												sprintf(szTemp,"%s",g_OnlineModule.nModuleInfo[j].szName);
												Common_Json_SetAttrValue(pItem,-1,"From",Common_Json_Type_String,szTemp,0,0);
												break;
											}
										}
										iCount++;
									}
								}
							}
						}
					}
				}
				if(0 == bAll)
					break;
				iArrayCount++;
				pUserCfg= pUserCfg->pNext;
			}while(pUserCfg);
		}
	}
	//ovfs_print_json(pResult);
	*pOutData = pResult;
	return 0;
}

//update online user time
int put_online_user(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bChange = 0;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0;
	S32 hSessionId = -1;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pItem = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	if(!pUserCfg)
	{
		LOGE("pAccessUsrCfgMgr->pUserCfg == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;
	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);

	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		Common_Json_GetAttrValue(pJData, iIndex, "SessionId" ,NULL, NULL, &hSessionId, NULL);
		if(hSessionId >= 0)
		{
			while(pUserCfg)
			{
				for(int i = 0;i < MAX_LOGIN_USER;i++)
				{
					if(pUserCfg->hSessionId[i] == hSessionId)
					{
						bChange = 1;
						break;
					}
				}
				if(bChange)
					break;
				pUserCfg = pUserCfg->pNext;
			}
		}
		else
		{
			LOGE("SessionId[%d] error!\n",hSessionId);
			continue;
		}
		pItem = Common_Json_GetAttrValue(pJData, iIndex, "ConnectInfo" ,NULL, NULL, NULL, NULL);
		if(pItem)
		{
			S8 *szIPv4 = NULL;
			S32 hSessionId = -1,tCreateTime = 0;
			Common_Json_GetAttrValue(pItem, -1, "SessionId" ,NULL, NULL, &hSessionId, NULL);
			Common_Json_GetAttrValue(pItem, -1, "IPv4" ,NULL, &szIPv4, NULL, NULL);
			Common_Json_GetAttrValue(pItem, -1, "CreateTime" ,NULL, NULL, &tCreateTime, NULL);
			while(pUserCfg)
			{
				for(int i = 0;i < OVFS_IPCONNECT_MAX_NUM;i++)
				{
					if(pUserCfg->pIpConnList[i])
					{
						IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[i];
						if(pIpConnect->hSessionId == hSessionId)
						{
							bChange = 1;
							break;
						}
					}
				}
				if(bChange)
					break;
				pUserCfg = pUserCfg->pNext;
			}
		}
		else
		{
			LOGE("SessionId[%d] error!\n",hSessionId);
			continue;
		}
	}
	if(bChange)
	{
		g_iCfgChange = OVFS_ONLINE_FLAG|0x01;
		refreshOnlineUserInfo(pAccessUsrCfgMgr);
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	ovfs_make_result(0, "succ", NULL, pOutData);
	return 0;
}

int notice_onlineuser_change(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	U8 bAll = 0,bNull = 0;	//0:ALL	1:单个	bNull---0:不包括NULL用户  1:包括null用户
	S32 iArrayCount = 0,iCount = 0;
	S8 *pValue = NULL,*pUserName = NULL;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pArray1 = NULL,*pItem = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(pJCondition)		//所有用户或单个用户
	{
		pItem = Common_Json_GetAttrValue(pJCondition, -1, "UserName" ,NULL, &pValue, NULL, NULL);
		if(pItem)
			pUserName = pValue;
		if(!pUserName)
		{
			LOGE("pUserName == NULL\n");
			ovfs_make_result(-1, "failed", NULL, pOutData);
			return -1;
		}
		if(0 == Common_StriCmp(pUserName, (S8 *)"all"))
		{
			bAll = 1;
			bNull = 1;
		}
		else
		{
			bAll = 0;
		}
	}
	else					//(除null外?所有用户
	{
		bAll = 1;
		bNull = 0;
	}

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		pArray = Common_Json_SetAttrValue(pResult,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;
		do{
			if(!pUserCfg)
				break;
			//if(0 != Common_StriCmp(pUserCfg->szUserName, (S8 *)"default"))
			//if(0 != Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
			if(0 == bAll)
			{
				if(0 != Common_StrCmp(pUserCfg->szUserName, pUserName))
				{
					pUserCfg= pUserCfg->pNext;
					continue;
				}
			}
			else
			{
				if(bNull == 0)			//单用户
				{
					if(0 == Common_StriCmp(pUserCfg->szUserName, (S8 *)"(null)"))
					{
						pUserCfg= pUserCfg->pNext;
						continue;
					}
				}
			}
			Common_Json_SetAttrValue(pArray,iArrayCount,"UserName",Common_Json_Type_String,pUserCfg->szUserName,0,0);
			pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"SessionId",Common_Json_Type_Array,NULL,0,0);
			iCount = 0;
			for(int i = 0;i < MAX_LOGIN_USER;i++)
			{
				if(pUserCfg->hSessionId[i] > 0)
				{
					Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Number,NULL,pUserCfg->hSessionId[i],0);
					iCount++;
				}
			}
			pArray1 = Common_Json_SetAttrValue(pArray,iArrayCount,"ConnectInfo",Common_Json_Type_Array,NULL,0,0);
			iCount = 0;
			if(pUserCfg->nIpConnCnt > 0)
			{
				for(int i = 0;i < OVFS_IPCONNECT_MAX_NUM;i++)
				{
					IpConnectInfo_T *pIpConnect =  pUserCfg->pIpConnList[i];
					if(pIpConnect)
					{
						if(pIpConnect->hSessionId > 0)
						{
							pItem = Common_Json_SetAttrValue(pArray1,iCount,NULL,Common_Json_Type_Object,NULL,0,0);
							Common_Json_SetAttrValue(pItem,-1,"SessionId",Common_Json_Type_Number,NULL,pIpConnect->hSessionId,0);
							if(pIpConnect->szBindIpv4)
								Common_Json_SetAttrValue(pItem,-1,"IPv4",Common_Json_Type_String,pIpConnect->szBindIpv4,0,0);
							Common_Json_SetAttrValue(pItem,-1,"CreateTime",Common_Json_Type_Number,NULL,pIpConnect->tCreateTime,0);
							iCount++;
						}
					}
				}
			}
			if(0 == bAll)
				break;
			iArrayCount++;
			pUserCfg= pUserCfg->pNext;
		}while(pUserCfg);
	}
	//ovfs_print_json(pResult);
	*pOutData = pResult;
	return 0;
}

//sync(add/delete) online user
int sync_online_user(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 *szIPv4 = NULL;
	S32 bExist = 0,bChange = 0;
	S32 iIndex = 0,iObjType = 0,iSessionCount = 0,iConnectCnt = 0;
	S32 iCurrIdx = 0,iModuleId = 0,hSessionId = -1,tCreateTime = 0;
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL,*pItem = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	if(!pUserCfg)
	{
		LOGE("pAccessUsrCfgMgr->pUserCfg == null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "SessionId");
	Common_Json_GetAttrValue(pJData, -1, "ModuleId" ,NULL, NULL, &iModuleId, NULL);

	if(-1 == Common_Json_GetAttr(pArray,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iSessionCount = Common_Json_Size(pArray);

	Common_Lock(g_subscribeMgr.hCfgLock);
	//ovfs_print_json(pJData);
	while(pUserCfg)
	{
		if(pUserCfg->nSessionCnt > 0)
		{
			for(int i = 0;i < MAX_LOGIN_USER;i++)
			{
				if(pUserCfg->hSessionId[i] > 0)
				{
					iCurrIdx = (pUserCfg->hSessionId[i]>>(OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
					if(iCurrIdx == iModuleId)
					{
						pUserCfg->hSessionId[i] = -1;
						pUserCfg->nSessionCnt--;
					}
				}
			}
		}
		pUserCfg = pUserCfg->pNext;
	}
	if(0 == iSessionCount)
		bChange = 1;
	hSessionId = -1;
	for(iIndex = 0;iIndex < iSessionCount;iIndex++)
	{
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;
		Common_Json_GetAttrValue(pArray, iIndex, NULL ,NULL, NULL, &hSessionId, NULL);
		if(hSessionId >= 0)
		{
			int lstIdx = 0;
			int usrIdx = 0;
			bExist = 0;
			while(pUserCfg)
			{
				for(iCurrIdx = 0;iCurrIdx < MAX_LOGIN_USER;iCurrIdx++)
				{
					if(pUserCfg->hSessionId[iCurrIdx] == hSessionId)
					{
						bExist = 1;
						break;
					}
				}
				if(bExist)
					break;
				if(usrIdx == (hSessionId&0xff))
				{
					for(iCurrIdx = lstIdx;iCurrIdx < MAX_LOGIN_USER;iCurrIdx++)
					{
						if(pUserCfg->hSessionId[iCurrIdx] <= 0)
						{
							pUserCfg->hSessionId[iCurrIdx] = hSessionId;
							pUserCfg->nSessionCnt++;
							lstIdx = iCurrIdx;
							bChange = 1;
							bExist = 1;
							break;
						}
					}
					if(bExist)
						break;
				}
				pUserCfg = pUserCfg->pNext;
				usrIdx++;
			}
		}
		else
		{
			LOGE("SessionId[%d] error!\n",hSessionId);
			continue;
		}
	}
	pArray = Common_Json_GetItem(pJData, -1, "ConnectInfo");
	pUserCfg = pAccessUsrCfgMgr->pUserCfg;
	while(pUserCfg)
	{
		if(pUserCfg->nIpConnCnt > 0)
		{
			for(int i = 0;i < OVFS_IPCONNECT_MAX_NUM;i++)
			{
				IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[i];
				if(pIpConnect)
				{
					iCurrIdx = (pIpConnect->hSessionId>>(OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
					if(iCurrIdx == iModuleId)
					{
						if(pIpConnect->szBindIpv4)
							Common_Free(pIpConnect->szBindIpv4, __FUNCTION__, __LINE__);
						if(pIpConnect->szBindIpv6)
							Common_Free(pIpConnect->szBindIpv6, __FUNCTION__, __LINE__);
						if(pIpConnect->szBindMac)
							Common_Free(pIpConnect->szBindMac, __FUNCTION__, __LINE__);
						Common_Free(pIpConnect, __FUNCTION__, __LINE__);
						pUserCfg->pIpConnList[i] = NULL;
						if(pUserCfg->nIpConnCnt > 0)
							pUserCfg->nIpConnCnt--;
					}
				}
			}
		}
		pUserCfg = pUserCfg->pNext;
	}
	iConnectCnt = Common_Json_Size(pArray);
	if(0 == iConnectCnt)
		bChange = 1;
	hSessionId = -1;
	for(iIndex = 0;iIndex < iConnectCnt;iIndex++)
	{
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;
		pItem = Common_Json_GetAttrValue(pArray, iIndex, NULL ,NULL, NULL, NULL, NULL);
		Common_Json_GetAttrValue(pItem, -1, "SessionId" ,NULL, NULL, &hSessionId, NULL);
		Common_Json_GetAttrValue(pItem, -1, "IPv4" ,NULL, &szIPv4, NULL, NULL);
		Common_Json_GetAttrValue(pItem, -1, "CreateTime" ,NULL, NULL, &tCreateTime, NULL);
		if(hSessionId >= 0)
		{
			int usrIdx = 0;
			bExist = 0;
			while(pUserCfg)
			{
				if(usrIdx == (hSessionId&0xff))
				{
					for(iCurrIdx = 0;iCurrIdx < OVFS_IPCONNECT_MAX_NUM;iCurrIdx++)
					{
						if(!pUserCfg->pIpConnList[iCurrIdx])
						{
							pUserCfg->pIpConnList[iCurrIdx] = (IpConnectInfo_T *)Common_Malloc(sizeof(IpConnectInfo_T), 0, __FUNCTION__, __LINE__);
							memset(pUserCfg->pIpConnList[iCurrIdx],0,sizeof(IpConnectInfo_T));
							if(pUserCfg->pIpConnList[iCurrIdx])
							{
								IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[iCurrIdx];
								pIpConnect->hSessionId = hSessionId;
								if(szIPv4)
									pIpConnect->szBindIpv4 = Common_StrDup(szIPv4, __FUNCTION__, __LINE__);
								pIpConnect->tCreateTime = tCreateTime;
								pUserCfg->nIpConnCnt++;
								bChange = 1;
								bExist = 1;
								break;
							}
						}
					}
					if(bExist)
						break;
				}
				pUserCfg = pUserCfg->pNext;
				usrIdx++;
			}
		}
		else
		{
			LOGE("SessionId[%d] error!\n",hSessionId);
			continue;
		}
	}
	if(bChange)
	{
		LOGD("sync succ!\n");
		g_iCfgChange = OVFS_ONLINE_FLAG|0x01;
		refreshOnlineUserInfo(pAccessUsrCfgMgr);
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	ovfs_make_result(0, "succ", NULL, pOutData);
	return 0;
}

int post_global_userid(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bExist = 0;
	S8 *pValue = NULL;
	S32 nModuleCnt = 0,nGlobalUserId = -1;
	cJSON_Struct *pResult = NULL,*pChild = NULL;
	ModuleInfo *pModuleInfo = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	nModuleCnt = g_OnlineModule.nModuleCount;
	if(nModuleCnt  >= MAX_MODULE_COUNT)
	{
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}

	Common_Json_GetAttrValue((cJSON_Struct *)pAddData, -1, "ModuleName", NULL, &pValue, NULL, NULL);
	if(!pValue)
	{
		ovfs_make_result(-1, "failed,ModuleName is null", NULL, pOutData);
		return -1;
	}
	for(int i = 0;i < MAX_MODULE_COUNT;i++)
	{
		pModuleInfo = &g_OnlineModule.nModuleInfo[i];
		if(0 == Common_StriCmp(pValue,pModuleInfo->szName))
		{
			bExist = 1;
			break;
		}
		pModuleInfo = NULL;
	}
	if(bExist)
	{
		nGlobalUserId = pModuleInfo->nModuleId;
	}
	else
	{
		if(nModuleCnt <= 0)
			g_OnlineModule.nModuleCount = 1;
		else
			g_OnlineModule.nModuleCount++;
		for(int i = 0;i < MAX_MODULE_COUNT;i++)
		{
			if(!g_OnlineModule.nModuleInfo[i].szName)
			{
				g_OnlineModule.nModuleInfo[i].szName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				g_OnlineModule.nModuleInfo[i].nModuleId = i+1;
				nGlobalUserId = g_OnlineModule.nModuleInfo[i].nModuleId;
				refreshOnlineUserInfo(pAccessUsrCfgMgr);
				break;
			}
		}
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pChild,-1,"GlobalId",Common_Json_Type_Number,NULL,nGlobalUserId,0);
		*pOutData = pResult;
	}
	else
	{
		ovfs_make_result(-1, "failed[malloc pResult failed]", NULL, pOutData);
	}
	return 0;
}

int delete_global_userid(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 *pValue = NULL;
	S32 nGlobalUserId = 0;
	cJSON_Struct *pJCondition = (cJSON_Struct *)pCondition;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(pJCondition)
	{
		Common_Json_GetAttrValue(pJCondition, -1, "GlobalUserId", NULL, &pValue, NULL, NULL);
		if(pValue)
			nGlobalUserId = atoi(pValue);
		if(nGlobalUserId > 0)
		{
			if(g_OnlineModule.nModuleInfo[nGlobalUserId-1].nModuleId == nGlobalUserId)
			{
				Common_Free(g_OnlineModule.nModuleInfo[nGlobalUserId-1].szName, __FUNCTION__, __LINE__);
				g_OnlineModule.nModuleInfo[nGlobalUserId-1].szName = NULL;
				g_OnlineModule.nModuleInfo[nGlobalUserId-1].nModuleId = -1;
				refreshOnlineUserInfo(pAccessUsrCfgMgr);
				ovfs_make_result(0, "succ", NULL, pOutData);
				return 0;
			}
		}
		else
		{
			LOGW("nGlobalUserId[%d] error!\n",nGlobalUserId);
		}
	}
	ovfs_make_result(-1, "failed", NULL, pOutData);
	return -1;
}

int put_access_restore(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	LOGW("======Start Restoring======\n");
	Common_System("rm /usr/etc/cfgfiles/access.json");
	ovfs_make_result(0,"Succ",NULL,pOutData);
	return 0;
}

int notice_iptable_change(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 i = 0,nWhich = 0;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pItem = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Mode",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->iAccessMode,0);
		Common_Json_SetAttrValue(pResult,-1,"StartTime",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->starttime,0);
		Common_Json_SetAttrValue(pResult,-1,"EndTime",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->endtime,0);
		pArray = Common_Json_SetAttrValue(pResult,-1,"WhiteList",Common_Json_Type_Array,NULL,0,0);
		for(i = 0,nWhich = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pWhiteList[i];
			if(pBindInfo)
			{
				pItem = Common_Json_SetAttrValue(pArray,nWhich,NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem,-1,"Ipv4",Common_Json_Type_String,pBindInfo->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pItem,-1,"EndIpv4",Common_Json_Type_String,pBindInfo->szBindEndIpv4,0,0);
				Common_Json_SetAttrValue(pItem,-1,"Ipv6",Common_Json_Type_String,pBindInfo->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pItem,-1,"EndIpv6",Common_Json_Type_String,pBindInfo->szBindEndIpv6,0,0);
				Common_Json_SetAttrValue(pItem,-1,"Direction",Common_Json_Type_Number,NULL,pBindInfo->iDirection,0);
				Common_Json_SetAttrValue(pItem,-1,"Protocol",Common_Json_Type_Number,NULL,pBindInfo->iProtocol,0);
				nWhich++;
			}
		}
		pArray = Common_Json_SetAttrValue(pResult,-1,"BlackList",Common_Json_Type_Array,NULL,0,0);
		for(i = 0,nWhich = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pBlackList[i];
			if(pBindInfo)
			{
				pItem = Common_Json_SetAttrValue(pArray,nWhich,NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem,-1,"Ipv4",Common_Json_Type_String,pBindInfo->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pItem,-1,"EndIpv4",Common_Json_Type_String,pBindInfo->szBindEndIpv4,0,0);
				Common_Json_SetAttrValue(pItem,-1,"Ipv6",Common_Json_Type_String,pBindInfo->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pItem,-1,"EndIpv6",Common_Json_Type_String,pBindInfo->szBindEndIpv6,0,0);
				Common_Json_SetAttrValue(pItem,-1,"Direction",Common_Json_Type_Number,NULL,pBindInfo->iDirection,0);
				Common_Json_SetAttrValue(pItem,-1,"Protocol",Common_Json_Type_Number,NULL,pBindInfo->iProtocol,0);
				nWhich++;
			}
		}
	}
	//ovfs_print_json(pResult);
	*pOutData = pResult;
	return 0;
}

int get_access_control_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 sErrorDes[256] = {0};
	S32 i = 0,iCode = 0,iArrayCount = 0;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;
	S8 label[][64] = {"WhiteList","BlackList","AccessMode"};
	S8 uri[][64] = {"/Access/AccessControl/WhiteList","/Access/AccessControl/BlackList","/Access/AccessControl/AccessMode"};

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,iCode,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,sErrorDes,11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		for(i = 0;i < ARRAYSIZE(label);i++){
			Common_Json_SetAttrValue(pArray,iArrayCount,"label",Common_Json_Type_String,label[i],0,0);
			Common_Json_SetAttrValue(pArray,iArrayCount,"uri",Common_Json_Type_String,uri[i],0,0);
			iArrayCount++;
		}
	}
	*pOutData = pResult;
	return 0;
}

//add whitelist
int post_access_whitelist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 *pValue = NULL,*pValue1 = NULL;
	S32 bSave = 0,iRet = 0;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0,iValue = -1;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);

	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		S32 iFindIndex = -1;
		S32 bExist = 0,bFind = 0;
		BindInfo_T *pBindInfo = NULL;
		U32 start_ip_add[4] = {0},end_ip_add[4] = {0};
		U32 iStartAddr = 0,iEndAddr = 0;

		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv4" ,NULL, &pValue, NULL, NULL);
		if(!pValue)
			continue;
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv4" ,NULL, &pValue1, NULL, NULL);
		if(!pValue1)
			pValue1 = pValue;
		if(pValue)
		{
			sscanf(pValue,"%d.%d.%d.%d",&start_ip_add[0],&start_ip_add[1],&start_ip_add[2],&start_ip_add[3]);
			iStartAddr = (start_ip_add[0]<<24)|(start_ip_add[1]<<16)|(start_ip_add[2]<<8)|(start_ip_add[3]);
		}
		if(pValue1)
		{
			sscanf(pValue1,"%d.%d.%d.%d",&end_ip_add[0],&end_ip_add[1],&end_ip_add[2],&end_ip_add[3]);
			iEndAddr = (end_ip_add[0]<<24)|(end_ip_add[1]<<16)|(end_ip_add[2]<<8)|(end_ip_add[3]);
		}
		if(iEndAddr < iStartAddr)
		{
			iRet = -1;
			break;
		}
		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			pBindInfo = pAccessUsrCfgMgr->pWhiteList[i];
			if(pBindInfo)
			{
				if(CompareIsInIPList(pValue, pBindInfo) || CompareIsInIPList(pValue1, pBindInfo))
				{
					bExist = 1;
					break;
				}
			}
			else
			{
				if(0 == bFind)
				{
					bFind = 1;		//find a valid pos
					iFindIndex = i;
				}
			}
		}
		if(bExist)			//the ip is exist
		{
			iRet = -1;
			break;
		}
		if(0 == bFind)		//iptabel is full
		{
			iRet = -1;
			LOGE("Iptable is full\n");
			break;
		}
		pBindInfo = (BindInfo_T*)Common_Malloc(sizeof(BindInfo_T), 0,  __FUNCTION__, __LINE__);
		if(!pBindInfo)		//no memmory
		{
			iRet = -1;
			LOGE("No memmory\n");
			break;
		}
		memset(pBindInfo,0,sizeof(BindInfo_T));
		pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pValue = NULL;
		pBindInfo->szBindEndIpv4 = Common_StrDup(pValue1, __FUNCTION__, __LINE__);
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv6" ,NULL, &pValue, NULL, NULL);
		if(pValue)
			pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv6" ,NULL, &pValue, NULL, NULL);
		if(pValue)
			pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);

		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Direction" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iDirection = iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Protocol" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iProtocol = iValue;
		pAccessUsrCfgMgr->pWhiteList[iFindIndex] = pBindInfo;
		bSave = 1;
	}
	if(bSave)
	{
		g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	if(0 == iRet)
	{
		ovfs_make_result(0, "succ", NULL, pOutData);
		LOGD("Add whitelist succ!\n");
	}
	else		//no memmory or iptable is full
		ovfs_make_result(-1, "failed", NULL, pOutData);
	return 0;
}

int get_access_whitelist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		int iIndex = 0;
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pWhiteList[i];
			if(pBindInfo)
			{
				Common_Json_SetAttrValue(pArray,iIndex,"Index",Common_Json_Type_Number,NULL,iIndex,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Ipv4",Common_Json_Type_String,pBindInfo->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"EndIpv4",Common_Json_Type_String,pBindInfo->szBindEndIpv4,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Ipv6",Common_Json_Type_String,pBindInfo->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"EndIpv6",Common_Json_Type_String,pBindInfo->szBindEndIpv6,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Direction",Common_Json_Type_Number,NULL,pBindInfo->iDirection,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Protocol",Common_Json_Type_Number,NULL,pBindInfo->iProtocol,0);
				iIndex++;
			}
		}
	}
	*pOutData = pResult;
	return 0;
}

int put_access_whitelist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bSave = 0;
	S8 *pValue = NULL,*pValue1 = NULL;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0,iValue = -1;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);

	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		S32 bExist = 0;
		BindInfo_T *pBindInfo = NULL;

		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv4" ,NULL, &pValue, NULL, NULL);
		if(!pValue)
			continue;
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv4" ,NULL, &pValue1, NULL, NULL);
		if(!pValue1)
			pValue1 = pValue;
		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			pBindInfo = pAccessUsrCfgMgr->pWhiteList[i];
			if(pBindInfo)
			{
				if(0 == Common_StrCmp(pValue, pBindInfo->szBindIpv4) && 0 == Common_StrCmp(pValue1, pBindInfo->szBindEndIpv4))
				{
					bExist = 1;
					break;
				}
			}
		}
		if(!bExist)			//the ip is exist
			continue;
		if(pBindInfo->szBindIpv4)
			Common_Free(pBindInfo->szBindIpv4, __FUNCTION__, __LINE__);
		pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pValue = NULL;
		if(pBindInfo->szBindEndIpv4)
			Common_Free(pBindInfo->szBindEndIpv4, __FUNCTION__, __LINE__);
		pBindInfo->szBindEndIpv4 = Common_StrDup(pValue1, __FUNCTION__, __LINE__);
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv6" ,NULL, &pValue, NULL, NULL);
		if(pBindInfo->szBindIpv6)
			Common_Free(pBindInfo->szBindIpv6, __FUNCTION__, __LINE__);
		pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv6" ,NULL, &pValue, NULL, NULL);
		if(pBindInfo->szBindEndIpv6)
			Common_Free(pBindInfo->szBindEndIpv6, __FUNCTION__, __LINE__);
		pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Direction" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iDirection = iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Protocol" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iProtocol = iValue;
		bSave = 1;
	}
	if(bSave)
	{
		g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	ovfs_make_result(0, "succ", NULL, pOutData);
	LOGD("put whitelist succ\n");
	Common_UnLock(g_subscribeMgr.hCfgLock);
	return 0;

}

int delete_access_whitelist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bSave = 0;
	S8 *pValue = NULL,*pValue1 = NULL;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL!\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	//ovfs_print_json(pJData);
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);
	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv4" ,NULL, &pValue, NULL, NULL);
		if(!pValue)
		{
			LOGE("Ipv4 == NULL\n");
			continue;
		}
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv4" ,NULL, &pValue1, NULL, NULL);
		if(!pValue1)
			pValue1 = pValue;
		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pWhiteList[i];
			if(pBindInfo)
			{
				if(0 == Common_StrCmp(pBindInfo->szBindIpv4,pValue)&&0 == Common_StrCmp(pBindInfo->szBindEndIpv4,pValue1))
				{
					if(pBindInfo->szBindIpv4)
						Common_Free(pBindInfo->szBindIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv4)
						Common_Free(pBindInfo->szBindEndIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindIpv6)
						Common_Free(pBindInfo->szBindIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv6)
						Common_Free(pBindInfo->szBindEndIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindMac)
						Common_Free(pBindInfo->szBindMac, __FUNCTION__, __LINE__);
					Common_Free(pBindInfo, __FUNCTION__, __LINE__);
					pAccessUsrCfgMgr->pWhiteList[i] = NULL;
					bSave = 1;
					break;
				}
			}
		}
	}
	if(bSave)
	{
		g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	ovfs_make_result(0, "succ", NULL, pOutData);
	LOGD("delete whitelist succ\n");
	Common_UnLock(g_subscribeMgr.hCfgLock);
	return 0;
}

int post_access_blacklist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S8 *pValue = NULL,*pValue1 = NULL;
	S32 bSave = 0,iRet = 0;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0,iValue = -1;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);

	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		S32 iFindIndex = -1;
		S32 bExist = 0,bFind = 0;
		BindInfo_T *pBindInfo = NULL;
		U32 start_ip_add[4] = {0},end_ip_add[4] = {0};
		U32 iStartAddr = 0,iEndAddr = 0;

		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv4" ,NULL, &pValue, NULL, NULL);
		if(!pValue)
			continue;
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv4" ,NULL, &pValue1, NULL, NULL);
		if(!pValue1)
			pValue1 = pValue;
		if(pValue)
		{
			sscanf(pValue,"%d.%d.%d.%d",&start_ip_add[0],&start_ip_add[1],&start_ip_add[2],&start_ip_add[3]);
			iStartAddr = (start_ip_add[0]<<24)|(start_ip_add[1]<<16)|(start_ip_add[2]<<8)|(start_ip_add[3]);
		}
		if(pValue1)
		{
			sscanf(pValue1,"%d.%d.%d.%d",&end_ip_add[0],&end_ip_add[1],&end_ip_add[2],&end_ip_add[3]);
			iEndAddr = (end_ip_add[0]<<24)|(end_ip_add[1]<<16)|(end_ip_add[2]<<8)|(end_ip_add[3]);
		}
		if(iEndAddr < iStartAddr)
		{
			iRet = -1;
			break;
		}
		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			pBindInfo = pAccessUsrCfgMgr->pBlackList[i];
			if(pBindInfo)
			{
				if(CompareIsInIPList(pValue, pBindInfo) || CompareIsInIPList(pValue1, pBindInfo))
				{
					bExist = 1;
					break;
				}
			}
			else
			{
				if(0 == bFind)
				{
					bFind = 1;
					iFindIndex = i;
				}
			}
		}
		if(bExist)			//the ip is exist
		{
			iRet = -1;
			break;
		}
		if(0 == bFind)		//iptabel is full
		{
			iRet = -1;
			LOGE("Iptable is full\n");
			break;
		}
		pBindInfo = (BindInfo_T*)Common_Malloc(sizeof(BindInfo_T), 0,  __FUNCTION__, __LINE__);
		if(!pBindInfo)		//no memmory
		{
			iRet = -1;
			LOGE("No memmory\n");
			break;
		}
		memset(pBindInfo,0,sizeof(BindInfo_T));
		pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pValue = NULL;
		pBindInfo->szBindEndIpv4 = Common_StrDup(pValue1, __FUNCTION__, __LINE__);
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv6" ,NULL, &pValue, NULL, NULL);
		if(pValue)
			pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv6" ,NULL, &pValue, NULL, NULL);
		if(pValue)
			pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Direction" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iDirection = iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Protocol" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iProtocol = iValue;
		pAccessUsrCfgMgr->pBlackList[iFindIndex] = pBindInfo;
		bSave = 1;
	}
	if(bSave)
	{
		g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	if(0 == iRet)
	{
		ovfs_make_result(0, "succ", NULL, pOutData);
		LOGD("Add blacklist succ!\n");
	}
	else		//no memmory or iptable is full
		ovfs_make_result(-1, "failed", NULL, pOutData);
	return 0;
}

int get_access_blacklist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pResult = NULL,*pArray = NULL,*pChild = NULL;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		int iIndex = 0;
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pBlackList[i];
			if(pBindInfo)
			{
				Common_Json_SetAttrValue(pArray,iIndex,"Index",Common_Json_Type_Number,NULL,iIndex,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Ipv4",Common_Json_Type_String,pBindInfo->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"EndIpv4",Common_Json_Type_String,pBindInfo->szBindEndIpv4,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Ipv6",Common_Json_Type_String,pBindInfo->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"EndIpv6",Common_Json_Type_String,pBindInfo->szBindEndIpv6,0,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Direction",Common_Json_Type_Number,NULL,pBindInfo->iDirection,0);
				Common_Json_SetAttrValue(pArray,iIndex,"Protocol",Common_Json_Type_Number,NULL,pBindInfo->iProtocol,0);
				iIndex++;
			}
		}
	}
	*pOutData = pResult;
	return 0;
}

int put_access_blacklist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bSave = 0;
	S8 *pValue = NULL,*pValue1 = NULL;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0,iValue = -1;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);

	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		S32 bExist = 0;
		BindInfo_T *pBindInfo = NULL;

		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv4" ,NULL, &pValue, NULL, NULL);
		if(!pValue)
			continue;
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv4" ,NULL, &pValue1, NULL, NULL);
		if(!pValue1)
			pValue1 = pValue;
		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			pBindInfo = pAccessUsrCfgMgr->pBlackList[i];
			if(pBindInfo)
			{
				if(0 == Common_StrCmp(pValue, pBindInfo->szBindIpv4) && 0 == Common_StrCmp(pValue1, pBindInfo->szBindEndIpv4))
				{
					bExist = 1;
					break;
				}
			}
		}
		if(!bExist)			//the ip is exist
			continue;
		if(pBindInfo->szBindIpv4)
			Common_Free(pBindInfo->szBindIpv4, __FUNCTION__, __LINE__);
		pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pValue = NULL;
		if(pBindInfo->szBindEndIpv4)
			Common_Free(pBindInfo->szBindEndIpv4, __FUNCTION__, __LINE__);
		pBindInfo->szBindEndIpv4 = Common_StrDup(pValue1, __FUNCTION__, __LINE__);
		pValue1 = NULL;

		Common_Json_GetAttrValue(pJData, iIndex, "Ipv6" ,NULL, &pValue, NULL, NULL);
		if(pBindInfo->szBindIpv6)
			Common_Free(pBindInfo->szBindIpv6, __FUNCTION__, __LINE__);
		pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv6" ,NULL, &pValue, NULL, NULL);
		if(pBindInfo->szBindEndIpv6)
			Common_Free(pBindInfo->szBindEndIpv6, __FUNCTION__, __LINE__);
		pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);

		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Direction" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iDirection = iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pJData, iIndex, "Protocol" ,NULL, NULL, &iValue, NULL);
		if(-1 != iValue)
			pBindInfo->iProtocol = iValue;
		bSave = 1;
	}
	if(bSave)
	{
		g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	ovfs_make_result(0, "succ", NULL, pOutData);
	LOGD("put blacklist succ\n");
	Common_UnLock(g_subscribeMgr.hCfgLock);
	return 0;
}

int delete_access_blacklist_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 bSave = 0;
	S8 *pValue = NULL,*pValue1 = NULL;
	S32 iIndex = 0,iObjType = 0,iArrayCount = 0;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData,*pArray = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL!\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	//ovfs_print_json(pJData);
	pArray = Common_Json_GetItem(pJData, -1, "ResList");
	pJData= pArray;

	if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
	{
		LOGE("ResList==null\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	if(iObjType!=Common_Json_Type_Array)
	{
		LOGE("ResList!=Common_Json_Type_Array\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	iArrayCount = Common_Json_Size(pJData);
	Common_Lock(g_subscribeMgr.hCfgLock);
	for(iIndex = 0;iIndex < iArrayCount;iIndex++)
	{
		pValue = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "Ipv4" ,NULL, &pValue, NULL, NULL);
		if(!pValue)
		{
			LOGE("Ipv4 == NULL\n");
			continue;
		}
		pValue1 = NULL;
		Common_Json_GetAttrValue(pJData, iIndex, "EndIpv4" ,NULL, &pValue1, NULL, NULL);
		if(!pValue1)
			pValue1 = pValue;
		for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pBlackList[i];
			if(pBindInfo)
			{
				if(0 == Common_StrCmp(pBindInfo->szBindIpv4,pValue)&&0 == Common_StrCmp(pBindInfo->szBindEndIpv4,pValue1))
				{
					if(pBindInfo->szBindIpv4)
						Common_Free(pBindInfo->szBindIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv4)
						Common_Free(pBindInfo->szBindEndIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindIpv6)
						Common_Free(pBindInfo->szBindIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv6)
						Common_Free(pBindInfo->szBindEndIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindMac)
						Common_Free(pBindInfo->szBindMac, __FUNCTION__, __LINE__);
					Common_Free(pBindInfo, __FUNCTION__, __LINE__);
					pAccessUsrCfgMgr->pBlackList[i] = NULL;
					bSave = 1;
					break;
				}
			}
		}
	}
	if(bSave)
	{
		g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
		access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	ovfs_make_result(0, "succ", NULL, pOutData);
	LOGD("delete whitelist succ\n");
	Common_UnLock(g_subscribeMgr.hCfgLock);
	return 0;
}

int get_access_mode_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	cJSON_Struct *pResult = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Data/Mode",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->iAccessMode,0);
		Common_Json_SetAttrValue(pResult,-1,"Data/StartTime",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->starttime,0);
		Common_Json_SetAttrValue(pResult,-1,"Data/EndTime",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->endtime,0);
	}
	*pOutData = pResult;
	return 0;
}

int put_access_mode_res(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iValue = -1,bChange = 0;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;

	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL\n");
		ovfs_make_result(-1, "failed", NULL, pOutData);
		return -1;
	}

	Common_Lock(g_subscribeMgr.hCfgLock);
	Common_Json_GetAttrValue(pJData, -1, "Mode" ,NULL, NULL, &iValue, NULL);
	if(iValue != -1)
	{
		if(pAccessUsrCfgMgr->iAccessMode != iValue)
			pAccessUsrCfgMgr->iAccessMode = iValue;
	}
	iValue = -1;
	Common_Json_GetAttrValue(pJData, -1, "StartTime" ,NULL, NULL, &iValue, NULL);
	if(iValue != -1)
	{
		if(pAccessUsrCfgMgr->starttime!= iValue)
			pAccessUsrCfgMgr->starttime = iValue;
	}
	iValue = -1;
	Common_Json_GetAttrValue(pJData, -1, "EndTime" ,NULL, NULL, &iValue, NULL);
	if(iValue != -1)
	{
		if(pAccessUsrCfgMgr->endtime!= iValue)
			pAccessUsrCfgMgr->endtime = iValue;
	}
	if(bChange)
	{
			g_iCfgChange = OVFS_IPTABLE_FLAG|0x01;
			access_saveCfg(g_subscribeMgr.hModuleHandle);
	}
	Common_UnLock(g_subscribeMgr.hCfgLock);
	ovfs_make_result(0, "succ", NULL, pOutData);
	LOGD("put access mode succ\n");
	return 0;

}

int get_access_debug(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	cJSON_Struct *pResult = NULL;

	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,"",11,0);
		Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Data/PrintDebug",Common_Json_Type_Number,NULL,g_bPrintDbg,0);
	}
	*pOutData = pResult;
	return 0;
}

int put_access_debug(void *pInData,void *pAddData,void *pCondition,void **pOutData)
{
	S32 iValue = -1;
	cJSON_Struct *pJData = (cJSON_Struct *)pAddData;

	Common_Json_GetAttrValue(pJData, -1, "PrintDebug" ,NULL, NULL, &iValue, NULL);
	g_bPrintDbg= iValue;
	ovfs_make_result(0,"Succ",NULL,pOutData);
	return 0;
}

S32 ovfs_get_debug()
{
	return g_bPrintDbg;
}

void freeSubscribeListNode(void * data)
{
	Access_SubscribeNode* node = (Access_SubscribeNode*)data;
	Common_Free(node->szSubscribeUri,__FUNCTION__,__LINE__);
	Common_Free(node->szValidUri,__FUNCTION__,__LINE__);
	Common_Json_Delete((cJSON_Struct *)node->pCondition);
	Common_Free(node,__FUNCTION__,__LINE__);
	return;
}

int insertSubscribeListNode(void * data)
{
	int iRet = -1;
	Access_SubscribeNode* node = (Access_SubscribeNode*)data;
	iRet = Common_DList_InsertTail(g_subscribeMgr.listdl, node, sizeof(Access_SubscribeNode));
	if(iRet < 0){
		Common_Free(node->szSubscribeUri,__FUNCTION__,__LINE__);
		Common_Free(node->szValidUri,__FUNCTION__,__LINE__);
		Common_Json_Delete((cJSON_Struct *)node->pCondition);
		Common_Free(node,__FUNCTION__,__LINE__);
	}
	LOGD("insert node iCount = %d \n",Common_DList_GetCount(g_subscribeMgr.listdl));
	return iRet;
}

S32 findSubscribeListNode(void * a, void *b)
{

	Access_SubscribeNode* node = (Access_SubscribeNode*)a;
	Access_SubscribeNode* p = (Access_SubscribeNode*)b;
	if(!node)
		return -1;
	if(0 == Common_StriCmp(node->szSubscribeUri,p->szSubscribeUri)&&(node->nRecvID==p->nRecvID))
	{
		return 0;
	}
	return -1;
}

static S32 static_Access_Subscribe_fxn(ModuleHandle_T hModuleHandle,S32 nType /* 0-subscribe,1-unsubscribe,2-QueryEvent*/,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData)
{
	int iRet = -1;
	S8 *pUri = NULL;
	void *p = NULL;
	cJSON_Struct *pJCondition = NULL;
	Access_SubscribeNode node;
	Access_SubscribeNode* pNode = NULL;

	memset(&node,0,sizeof(Access_SubscribeNode));
	if(g_bPrintDbg && pQueryEventInfo && *pQueryEventInfo)
	{
		ovfs_print_json(*pQueryEventInfo);
	}
	if(2 == nType)
	{
		node.nRecvID = nRecvID;
		node.szSubscribeUri = szSubscribeUri;
		pNode = (Access_SubscribeNode*)Common_DList_Search(g_subscribeMgr.listdl, (void*)&node, findSubscribeListNode);
		iRet = AnalyzeSubscribeUriAndSend(hModuleHandle,pNode->nRecvID,pNode->szValidUri,pNode->pCondition,pQueryEventInfo);
		return iRet;
	}
	else if(1 == nType)
	{
		node.nRecvID = nRecvID;
		node.szSubscribeUri = szSubscribeUri;
		return Common_DList_Delete(g_subscribeMgr.listdl, (void*)&node, findSubscribeListNode);
	}
	else if(0 == nType)
	{
		LOGD("%s %d %d\n",szSubscribeUri,nType,nRecvID);
		node.nRecvID = nRecvID;
		node.szSubscribeUri = szSubscribeUri;
		p = Common_DList_Search(g_subscribeMgr.listdl,(void*)&node,findSubscribeListNode);
		if(p)
		{
			LOGD("exist subscribe node!");
			return 0;
		}
		pNode = (Access_SubscribeNode*)Common_Malloc(sizeof(Access_SubscribeNode), 0, __FUNCTION__, __LINE__);
		pJCondition = SeparateUriAndCondition(szSubscribeUri, &pUri);
		pNode->nRecvID = nRecvID;
		pNode->szSubscribeUri = Common_StrDup(szSubscribeUri,__FUNCTION__,__LINE__);
		pNode->szValidUri = pUri;
		pNode->pCondition = pJCondition;
		insertSubscribeListNode((void *)pNode);
		iRet = AnalyzeSubscribeUriAndSend(hModuleHandle,nRecvID,pUri,pJCondition,pQueryEventInfo);
		return iRet;
	}
	return 0;
}

S32 Thread_SubscribeHandle(Common_Thread_T hThreadHandle,void *pUserData)
{
	AccessUserCfg *pUserCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	ModuleHandle_T hModuleHandle = *(ModuleHandle_T*)pUserData;
	S32 iRet = -1,i = 0,iCount = 0,iCfgChange = 0,iOnlineChange = 0,iIptableChange = 0,iFlag = 0;
	cJSON_Struct *pResult = NULL,*pRoot = NULL;
	Access_SubscribeNode *pNode = NULL;

	while(1)
	{
		pUserCfg = pAccessUsrCfgMgr->pUserCfg;
		while(pUserCfg)
		{
			if(pUserCfg->szTempPassword && pUserCfg->tTempValidTime > 0 && pUserCfg->tTempCreateTime > 0)
			{
				S32 tCurTime = time(NULL);
				if((tCurTime >= pUserCfg->tTempCreateTime) && (tCurTime <= pUserCfg->tTempValidTime+pUserCfg->tTempCreateTime))
				{
				}
				else
				{
					Common_Lock(g_subscribeMgr.hCfgLock);
					if(pUserCfg->szTempPassword)
						Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
					pUserCfg->szTempPassword = NULL;
					if(pUserCfg->szSerialNumber)
						Common_Free(pUserCfg->szSerialNumber, __FUNCTION__, __LINE__);
					pUserCfg->szSerialNumber = NULL;
					pUserCfg->tTempCreateTime = 0;
					pUserCfg->tTempValidTime = 0;
					pUserCfg->tFailTime = 0;
					g_iCfgChange = OVFS_CFG_FLAG|0x01;
					#if 0
					access_saveCfg(g_subscribeMgr.hModuleHandle);
					#endif
					Common_UnLock(g_subscribeMgr.hCfgLock);
				}
			}
			pUserCfg = pUserCfg->pNext;
		}
		if(g_iCfgChange){
			iCfgChange = g_iCfgChange&OVFS_CFG_FLAG;
			iOnlineChange = g_iCfgChange&OVFS_ONLINE_FLAG;
			iIptableChange = g_iCfgChange&OVFS_IPTABLE_FLAG;
			Common_Lock(g_subscribeMgr.hCfgLock);
			iCount= Common_DList_GetCount(g_subscribeMgr.listdl);
			for(i = 0;i < iCount;i++)
			{
				pNode= (Access_SubscribeNode *)Common_DList_GetNode(g_subscribeMgr.listdl, i);
				if(!pNode)
				{
					LOGW("node[%d] is null!\n",i);
					continue;
				}
				if(iCfgChange &&(0 == Common_StriCmp(pNode->szValidUri, (S8*)"/Access/Subscribe/UserCfg")))
				{
					iFlag = 1;
				}
				else if(iOnlineChange &&(0 == Common_StriCmp(pNode->szValidUri, (S8*)"/Access/Subscribe/OnlineUser")))
				{
					iFlag = 1;
				}
				else if(iIptableChange &&(0 == Common_StriCmp(pNode->szValidUri, (S8*)"/Access/Subscribe/Iptable")))
				{
					iFlag = 1;
				}
				if(iFlag)
				{
					AnalyzeSubscribeUriAndSend(hModuleHandle, pNode->nRecvID,pNode->szValidUri,pNode->pCondition,&pRoot);
					iRet = Module_SendEvent(hModuleHandle, pNode->nRecvID, pRoot, &pResult, 3000);
					if(iRet < 0)		//接口返回值 有问题，成功也返回负值
					{
						//LOGD("Module_SendEvent failed,szSubscribeUri:%s,nRecvID:%d,iRet:%d\n",pNode->szSubscribeUri,pNode->nRecvID,iRet);
					}
					else
					{
						//LOGD("Module_SendEvent succ!nRecvID:%d\n",pNode->nRecvID);
					}
					Common_Json_Delete(pRoot);
					Common_Json_Delete(pResult);
					pRoot = NULL;
					pResult = NULL;
					iFlag = 0;
				}
			}
			g_iCfgChange = 0;
			Common_UnLock(g_subscribeMgr.hCfgLock);
		}
		Common_Sleep(1, 0);
	}
	return 0;
}

int updateStoreItem(ModuleHandle_T hModuleHandle)
{
	S32 nRet = -1;
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL,*pChild = NULL,*pArray = NULL,*pItem = NULL;
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Core/Restore/Update",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"put",0,0);

		pChild = Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

		pItem = Common_Json_SetAttrValue(pArray,0,NULL,Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pItem,-1,"Uri",Common_Json_Type_String,"/Access/Restore",0,0);
		Common_Json_SetAttrValue(pItem,-1,"Label",Common_Json_Type_String,"UserCfg",0,0);
		Common_Json_SetAttrValue(pItem,-1,"NeedReboot",Common_Json_Type_Number,NULL,1,0);

		nRet = Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
		if(nRet != 0)
		{
			ovfs_print_json(pConfig);
			LOGE("nRet:%d\n",nRet);
		}
		Common_Json_Delete(pConfig);
		Common_Json_Delete(pOutParams);
	}
	return nRet;
}

S32 access_loadCfg(ModuleHandle_T hModuleHandle)
{
	S8 *pValue = NULL;
	S32 iOnLineCount = 0;
	bool bFindSurperUsr = 0;
	S8 *szSurperUsr = (S8*)"(null)",*szSurperPwd = (S8*)"ovfsZSJQZLHL";
	S32 i = 0,iIndex = 0,nUserCount = 0,nCount = 0,iValue = -1,iRet = -1;
	AccessUserCfg *pUserCfg = NULL,*pTempCfg = NULL,*pCurCfg = NULL;
	cJSON_Struct *pConfig = NULL,*pOnlineInfo = NULL,*pArray = NULL,*pArray1 = NULL,*pItem = NULL,*pItem1 = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = NULL;

	memset(&g_subscribeMgr,0,sizeof(g_subscribeMgr));
	memset(&g_OnlineModule,0,sizeof(g_OnlineModule));

	g_subscribeMgr.hModuleHandle = hModuleHandle;

	pAccessUsrCfgMgr = (AccessUserCfgMgr_T*)Common_Malloc(sizeof(AccessUserCfgMgr_T), 0, __FUNCTION__, __LINE__);
	if(!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL!\n");
		return -1;
	}
	memset(pAccessUsrCfgMgr,0,sizeof(AccessUserCfgMgr_T));
	iRet = Module_LoadConfig(hModuleHandle,&pConfig);
	if(iRet<0){
		LOGD("Load default cfg!\n");
		iRet = Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Default,&pConfig);
		if(iRet<0)
		{
			return iRet;
		}
	}
	do{
		if(!pConfig)
		{
			LOGE("LoadConfig failed!\n");
			Common_Json_Delete(pConfig);
			break;
		}
		pArray = Common_Json_GetItem(pConfig,-1,"Users");
		if(!pArray)
		{
			LOGE("Find Item Users failed!\n");
			Common_Json_Delete(pConfig);
			break;
		}
		nUserCount = Common_Json_Size(pArray);
		pAccessUsrCfgMgr->nUserCfgCount = nUserCount;
		for(iIndex = 0;iIndex < nUserCount;iIndex++)
		{
			pCurCfg = (AccessUserCfg*)Common_Malloc(sizeof(AccessUserCfg), 0, __FUNCTION__, __LINE__);
			if(!pCurCfg)
			{
				LOGE("malloc AccessUserCfg failed!\n");
				break;
			}
			memset(pCurCfg,0,sizeof(AccessUserCfg));
			if(!pAccessUsrCfgMgr->pUserCfg)
			{
				pAccessUsrCfgMgr->pUserCfg = pCurCfg;
				pUserCfg = pCurCfg;
			}
			if(pTempCfg)
				pTempCfg->pNext = pCurCfg;
			pCurCfg->pPrev = pTempCfg;
			pCurCfg->pNext = NULL;
			pTempCfg = pCurCfg;

			pItem = Common_Json_GetItem(pArray,iIndex,NULL);
			if(!pItem)
				continue;
			Common_Json_GetAttrValue(pItem,-1,"UserName",NULL,&pValue,NULL,NULL);
			if(pValue)
				pCurCfg->szUserName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			if(0 == Common_StrCmp(pCurCfg->szUserName, szSurperUsr))
				bFindSurperUsr = 1;
			pValue = NULL;

			Common_Json_GetAttrValue(pItem,-1,"EncryptMethod",NULL,NULL,(S32*)&iValue,NULL);

			if(0 == Common_StrCmp(pCurCfg->szUserName, szSurperUsr ))
			{
				S8 *pEncrpt = NULL,*pTemp = NULL;

				pEncrpt = static_EncryptString(SYSTEM_TEXT, szSurperPwd, pTemp, 0);
				pCurCfg->szDefaultPassword= Common_StrDup(pEncrpt, __FUNCTION__, __LINE__);
				pCurCfg->byPwdCount = 1;
				pCurCfg->szPassword[0] = Common_StrDup(pEncrpt, __FUNCTION__, __LINE__);
				if(pEncrpt)
					Common_Free(pEncrpt, __FUNCTION__, __LINE__);
				pEncrpt = NULL;
			}
			else
			{
			Common_Json_GetAttrValue(pItem,-1,"DefaultPwd",NULL,&pValue,NULL,NULL);
			if(pValue)
				pCurCfg->szDefaultPassword= Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			pArray1 = Common_Json_GetItem(pItem,-1,"Password");
			if(pArray1){
				nCount = Common_Json_Size(pArray1);
				pCurCfg->byPwdCount = nCount;
				for(i = 0;i < nCount;i++){
					Common_Json_GetAttrValue(pArray1,i,NULL,NULL,&pValue,NULL,NULL);
					if(pValue)
						pCurCfg->szPassword[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
				}
			}
			}
			#if 0
			Common_Json_GetAttrValue(pItem,-1,"TempPassword",NULL,&pValue,NULL,NULL);
			if(pValue)
				pCurCfg->szTempPassword= Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pItem,-1,"TempCreateTime",NULL,NULL,&pUserCfg->tTempCreateTime,NULL);
			Common_Json_GetAttrValue(pItem,-1,"TempValidTime",NULL,NULL,&pUserCfg->tTempValidTime,NULL);
			#endif
			pItem1 = Common_Json_GetAttrValue(pItem,-1,"Priority",NULL,NULL,&iValue,NULL);
			if(pItem1)
				pCurCfg->byPriority = iValue;
			pItem1 = Common_Json_GetAttrValue(pItem,-1,"Forbidden",NULL,NULL,&iValue,NULL);
			if(pItem1)
				pCurCfg->bForbidden = iValue;
			pItem1 = Common_Json_GetAttrValue(pItem,-1,"Remote",NULL,NULL,&iValue,NULL);
			if(pItem1)
				pCurCfg->bRemote = iValue;
			pItem1 = Common_Json_GetAttrValue(pItem,-1,"NeedOnlineAuth",NULL,NULL,&iValue,NULL);
			if(pItem1)
				pCurCfg->bNeedOnlineAuth = iValue;

			pArray1 = Common_Json_GetItem(pItem,-1,"OnlineAuthManList");
			if(pArray1){
				nCount = Common_Json_Size(pArray1);
				if(nCount > 0){
					for(i = 0;i < nCount;i++){
						Common_Json_GetAttrValue(pArray1,i,NULL,NULL,&pValue,NULL,NULL);
						if(pValue)
							pCurCfg->szOnlineAuthManList[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
						pValue = NULL;
					}
					pCurCfg->byOnlineAuthListCount = nCount;
				}
			}
			if(!pArray1||(0 == pCurCfg->byOnlineAuthListCount))
			{
				pCurCfg->szOnlineAuthManList[0] = Common_StrDup(pCurCfg->szUserName, __FUNCTION__, __LINE__);
				pCurCfg->byOnlineAuthListCount = 1;
			}
			if(pCurCfg->szOnlineAuthManList[0]&&(0 != Common_StrCmp(pCurCfg->szOnlineAuthManList[0], pCurCfg->szUserName)))
			{
				if(pCurCfg->szUserName)
					Common_Free(pCurCfg->szUserName, __FUNCTION__, __LINE__);
				pCurCfg->szUserName = Common_StrDup(pCurCfg->szOnlineAuthManList[0], __FUNCTION__, __LINE__);
			}

			Common_Json_GetAttrValue(pItem,-1,"BindIPv4",NULL,&pValue,NULL,NULL);
			if(pValue)
				pCurCfg->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pItem,-1,"BindIPv6",NULL,&pValue,NULL,NULL);
			if(pValue)
				pCurCfg->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pItem,-1,"BindMAC",NULL,&pValue,NULL,NULL);
			if(pValue)
				pCurCfg->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;

			pItem1 = Common_Json_GetItem(pItem,-1,"LocalRight");
			if(pItem1)
			{
				U32 u32Right = 0;
				AccessUserRight_T *pLocalR = (AccessUserRight_T *)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);

				if(pLocalR){
					memset(pLocalR,0,sizeof(AccessUserRight_T));
					pCurCfg->pLocalRight = pLocalR;
					Common_Json_GetAttrValue(pItem1,-1,"RightMask",NULL,NULL,(S32*)&u32Right,NULL);
					set_userRight(pLocalR,u32Right);
					pArray1 = Common_Json_GetItem(pItem1,-1,"ChanRight");
					if(pArray1)
					{
						AccessChanRight_T *pChanRight = NULL;

						nCount = Common_Json_Size(pArray1);
						pLocalR->nChanRightCount = nCount;
						pChanRight = (AccessChanRight_T *)Common_Malloc(nCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
						if(pChanRight){
							memset(pChanRight,0,nCount*sizeof(AccessChanRight_T));
							pLocalR->pChanRight = pChanRight;
							for(i = 0;i < nCount;i++){
								U32 u32ChanRight = 0;
								Common_Json_GetAttrValue(pArray1,i,"DeviceNo",NULL,NULL,(S32*)&pChanRight[i].wDeviceNo,NULL);
								Common_Json_GetAttrValue(pArray1,i,"ChannelNo",NULL,NULL,(S32*)&pChanRight[i].wChannelNo,NULL);
								Common_Json_GetAttrValue(pArray1,i,"RightMask",NULL,NULL,(S32*)&u32ChanRight,NULL);
								set_chanRight(&pChanRight[i],u32ChanRight);
							}
						}
					}
				}
			}

			pItem1 = Common_Json_GetItem(pItem,-1,"RemoteRight");
			if(pItem1)
			{
				U32 u32Right = 0;
				AccessUserRight_T *pRemoteR = (AccessUserRight_T *)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);

				if(pRemoteR){
					memset(pRemoteR,0,sizeof(AccessUserRight_T));
					pCurCfg->pRemoteRight = pRemoteR;
					Common_Json_GetAttrValue(pItem1,-1,"RightMask",NULL,NULL,(S32*)&u32Right,NULL);
					set_userRight(pRemoteR,u32Right);
					pArray1 = Common_Json_GetItem(pItem1,-1,"ChanRight");
					if(pArray1)
					{
						AccessChanRight_T *pChanRight = NULL;

						nCount = Common_Json_Size(pArray1);
						pRemoteR->nChanRightCount = nCount;
						pChanRight = (AccessChanRight_T *)Common_Malloc(nCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
						if(pChanRight){
							memset(pChanRight,0,nCount*sizeof(AccessChanRight_T));
							pRemoteR->pChanRight = pChanRight;
							for(i = 0;i < nCount;i++){
								U32 u32ChanRight = 0;
								Common_Json_GetAttrValue(pArray1,i,"DeviceNo",NULL,NULL,(S32*)&pChanRight[i].wDeviceNo,NULL);
								Common_Json_GetAttrValue(pArray1,i,"ChannelNo",NULL,NULL,(S32*)&pChanRight[i].wChannelNo,NULL);
								Common_Json_GetAttrValue(pArray1,i,"RightMask",NULL,NULL,(S32*)&u32ChanRight,NULL);
								set_chanRight(&pChanRight[i],u32ChanRight);
							}
						}
					}
				}
			}
			//LOGE("[%d %d]:%s\n",iIndex,nUserCount,pUserCfg->szUserName);
		}
		if(0 == bFindSurperUsr)
		{
			S8 *pEncrpt = NULL,*pTemp = NULL;

			pCurCfg = (AccessUserCfg*)Common_Malloc(sizeof(AccessUserCfg), 0, __FUNCTION__, __LINE__);
			if(!pCurCfg)
			{
				LOGE("pUserCfg == NULL\n");
				break;
			}
			memset(pCurCfg,0,sizeof(AccessUserCfg));
			if(!pAccessUsrCfgMgr->pUserCfg)
				pAccessUsrCfgMgr->pUserCfg = pCurCfg;
			if(pTempCfg)
				pTempCfg->pNext = pCurCfg;
			pCurCfg->pPrev = pTempCfg;
			pCurCfg->pNext = NULL;
			pTempCfg = pCurCfg;
			pCurCfg->szUserName = Common_StrDup(szSurperUsr, __FUNCTION__, __LINE__);
			pEncrpt = static_EncryptString(SYSTEM_TEXT, szSurperPwd, pTemp, 0);
			pCurCfg->szDefaultPassword = Common_StrDup(pEncrpt, __FUNCTION__, __LINE__);
			pCurCfg->szPassword[0] = Common_StrDup(pEncrpt, __FUNCTION__, __LINE__);
			pCurCfg->byPwdCount = 1;
			pCurCfg->byEncryptMethod = SYSTEM_TEXT;
			pCurCfg->byPriority = LEVEL_ADMIN;
			pCurCfg->bForbidden = 0;
			pCurCfg->bNeedOnlineAuth = 1;
			pCurCfg->byOnlineAuthListCount = 1;
			pCurCfg->szOnlineAuthManList[0] = Common_StrDup(szSurperUsr, __FUNCTION__, __LINE__);
			pCurCfg->pLocalRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
			if(pCurCfg->pLocalRight)
			{
				memset(pCurCfg->pLocalRight,0,sizeof(AccessUserRight_T));
				set_defaultUsrRight(pCurCfg->pLocalRight,pCurCfg->byPriority);
			}
			pCurCfg->pRemoteRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
			if(pCurCfg->pRemoteRight)
			{
				memset(pCurCfg->pRemoteRight,0,sizeof(AccessUserRight_T));
				set_defaultUsrRight(pCurCfg->pRemoteRight,pCurCfg->byPriority);
			}
			if(pEncrpt)
				Common_Free(pEncrpt, __FUNCTION__, __LINE__);
			pEncrpt = NULL;
			pAccessUsrCfgMgr->nUserCfgCount++;
		}
		iValue = -1;
		Common_Json_GetAttrValue(pConfig, -1, "IpTable/Mode" ,NULL, NULL, &iValue, NULL);
		if(iValue != -1)
			pAccessUsrCfgMgr->iAccessMode = iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pConfig, -1, "IpTable/StartTime" ,NULL, NULL, &iValue, NULL);
		if(iValue != -1)
			pAccessUsrCfgMgr->starttime= iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pConfig, -1, "IpTable/EndTime" ,NULL, NULL, &iValue, NULL);
		if(iValue != -1)
			pAccessUsrCfgMgr->endtime= iValue;
		pArray = Common_Json_GetItem(pConfig,-1,"IpTable/WhiteList");
		if(pArray)
		{
			nCount = Common_Json_Size(pArray);
			for(i = 0;i < nCount && nCount < MAX_IPTABLE_COUNT;i++)
			{
				BindInfo_T *pBindInfo = (BindInfo_T*)Common_Malloc(sizeof(BindInfo_T), 0,  __FUNCTION__, __LINE__);
				if(!pBindInfo)		//no memmory
				{
					LOGE("No memmory\n");
					break;
				}
				if(pBindInfo)
				{
					memset(pBindInfo,0,sizeof(BindInfo_T));
					Common_Json_GetAttrValue(pArray, i, "Ipv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "Ipv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					iValue = -1;
					Common_Json_GetAttrValue(pArray, i, "Direction" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iDirection = iValue;
					Common_Json_GetAttrValue(pArray, i, "Protocol" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iProtocol= iValue;
					pAccessUsrCfgMgr->pWhiteList[i] = pBindInfo;
				}
			}
		}
		pArray = Common_Json_GetItem(pConfig,-1,"IpTable/BlackList");
		if(pArray)
		{
			nCount = Common_Json_Size(pArray);
			for(i = 0;i < nCount && nCount < MAX_IPTABLE_COUNT;i++)
			{
				BindInfo_T *pBindInfo = (BindInfo_T*)Common_Malloc(sizeof(BindInfo_T), 0,  __FUNCTION__, __LINE__);
				if(!pBindInfo)		//no memmory
				{
					LOGE("No memmory\n");
					break;
				}
				if(pBindInfo)
				{
					memset(pBindInfo,0,sizeof(BindInfo_T));
					Common_Json_GetAttrValue(pArray, i, "Ipv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "Ipv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					iValue = -1;
					Common_Json_GetAttrValue(pArray, i, "Direction" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iDirection = iValue;
					Common_Json_GetAttrValue(pArray, i, "Protocol" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iProtocol= iValue;
					pAccessUsrCfgMgr->pBlackList[i] = pBindInfo;
				}
			}
		}
		Module_LoadTempData(hModuleHandle, &pOnlineInfo);
		Common_Json_GetAttrValue(pOnlineInfo,-1,"OnlineCount",NULL,NULL,&iOnLineCount,NULL);
		if(iOnLineCount > 0)
		{
			S8 *szIPv4 = NULL,*szLoginTime = NULL;
			S32 iUsrIdx = 0,iModuleId = 0,hSessionId = -1,iCurrIdx = 0;

			//ovfs_print_json(pOnlineInfo);
			pArray = Common_Json_GetItem(pOnlineInfo,-1,"OnlineList");
			nUserCount = Common_Json_Size(pArray);
			pAccessUsrCfgMgr->nOnlineUserCount = 0;
			pUserCfg = pAccessUsrCfgMgr->pUserCfg;
			pValue = NULL,iValue = -1;
			while(pUserCfg)
			{
				for(i = 0;i < nUserCount;i++)
				{
					pUserCfg->nSessionCnt = 0;
					pUserCfg->nIpConnCnt = 0;
					pItem = Common_Json_GetItem(pArray,i,NULL);
					Common_Json_GetAttrValue(pItem,-1,"UserName",NULL,&pValue,NULL,NULL);
					if(0 == Common_StrCmp(pValue, pUserCfg->szUserName))
					{
						pValue = NULL;
						pArray1 = Common_Json_GetItem(pItem,-1,"ConnectInfo");
						nCount = Common_Json_Size(pArray1);
						for(iIndex = 0;iIndex < nCount;iIndex++)
						{
							pItem1 = Common_Json_GetItem(pArray1,iIndex,NULL);
							Common_Json_GetAttrValue(pItem1,-1,"Id",NULL,NULL,&hSessionId,NULL);
							Common_Json_GetAttrValue(pItem1,-1,"IPv4",NULL,&szIPv4,NULL,NULL);
							Common_Json_GetAttrValue(pItem1,-1,"LoginTime",NULL,&szLoginTime,NULL,NULL);
							iUsrIdx = hSessionId & OVFS_LOGINHANDLE_BITMASK;
							iModuleId = (hSessionId>>(OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
							if(iUsrIdx < 0 || iUsrIdx >= MAX_LOGIN_USER || iModuleId <=0 ||iModuleId > MAX_MODULE_COUNT)
							{
								LOGE("hSessionId:%d,iUsrIdx:%d,iModuleId:%d\n",hSessionId,iUsrIdx,iModuleId);
								continue;
							}
							#if 0
							LOGW("iModuleId:%d\n",iModuleId);
							g_OnlineModule.nModuleInfo[iModuleId-1].nModuleId = iModuleId;
							Common_Json_GetAttrValue(pItem1,-1,"From",NULL,&pValue,NULL,NULL);
							if(!g_OnlineModule.nModuleInfo[iModuleId-1].szName)
							{
								g_OnlineModule.nModuleInfo[iModuleId-1].szName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
								g_OnlineModule.nModuleCount++;
							}
							#endif
							iCurrIdx = pAccessUsrCfgMgr->nOnlineUserCount;
							pUserCfg->hSessionId[iCurrIdx] = hSessionId;
							pUserCfg->nSessionCnt++;

							if(!pUserCfg->pIpConnList[iCurrIdx])
							{
								pUserCfg->pIpConnList[iCurrIdx] = (IpConnectInfo_T *)Common_Malloc(sizeof(IpConnectInfo_T), 0, __FUNCTION__, __LINE__);
								memset(pUserCfg->pIpConnList[iCurrIdx],0,sizeof(IpConnectInfo_T));
								if(pUserCfg->pIpConnList[iCurrIdx])
								{
									S32 tCreateTime = 0;
									Common_Time_T tlogintime;
									S32 year = 0,mon = 0,day = 0,hour = 0,min = 0,sec = 0;
									IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[iCurrIdx];
									pIpConnect->hSessionId = hSessionId;
									if(szIPv4)
										pIpConnect->szBindIpv4 = Common_StrDup(szIPv4, __FUNCTION__, __LINE__);
									sscanf(szLoginTime, "%04d-%02d-%02d %02d:%02d:%02d", &year,&mon,&day,&hour,&min,&sec);
									tlogintime.year = year;
									tlogintime.month = mon;
									tlogintime.day= day;
									tlogintime.hour= hour;
									tlogintime.min = min;
									tlogintime.sec = sec;
									Common_Common2LinuxTime(&tlogintime, (time_t*)&tCreateTime);
									pIpConnect->tCreateTime = tCreateTime;
									pUserCfg->nIpConnCnt++;
								}
							}
							pAccessUsrCfgMgr->nOnlineUserCount++;
						}
						break;
					}
				}
				pUserCfg = pUserCfg->pNext;
			}
			pValue = NULL;
			iValue = -1;
			pArray = Common_Json_GetItem(pOnlineInfo,-1,"ModuleMap");
			nUserCount = Common_Json_Size(pArray);
			LOGW("ModuleMap:\n");
			for(i = 0;i < nUserCount;i++)
			{
				pItem = Common_Json_GetItem(pArray,i,NULL);
				Common_Json_GetAttrValue(pItem,-1,"ModuleId",NULL,NULL,&iValue,NULL);
				g_OnlineModule.nModuleInfo[i].nModuleId = iValue;
				Common_Json_GetAttrValue(pItem,-1,"ModuleName",NULL,&pValue,NULL,NULL);
				g_OnlineModule.nModuleInfo[i].szName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				LOGW("%d====>%s\n",iValue,pValue);
				g_OnlineModule.nModuleCount++;
			}
		}
		LOGD("nOnlineUserCount:%d\n",pAccessUsrCfgMgr->nOnlineUserCount);
		g_subscribeMgr.pAccessUsrCfgMgr = pAccessUsrCfgMgr;
		Common_Json_Delete(pConfig);
		Common_Json_Delete(pOnlineInfo);
		return 0;
	}while(0);
	Common_Json_Delete(pConfig);
	Common_Free(pAccessUsrCfgMgr, __FUNCTION__, __LINE__);
	return -1;
}

S32 access_saveCfg(ModuleHandle_T hModuleHandle)
{
	AccessUserCfg *pUserCfg = NULL;
	S32 nWhich = 0,nIndex = 0,i = 0;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	cJSON_Struct *pConfig = NULL,*pItem = NULL,*pItem1 = NULL,*pArray = NULL,*pArray1 = NULL;

	if (!pAccessUsrCfgMgr)
	{
		LOGE("pAccessUsrCfgMgr == NULL!\n");
		return -1;
	}

	if(pAccessUsrCfgMgr->nUserCfgCount > 0)
	{
		pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (!pConfig)
		{
			LOGE("pConfig == NULL!\n");
			return -1;
		}
		pArray = Common_Json_SetAttrValue(pConfig,-1,"Users",Common_Json_Type_Array,NULL,0,0);
		if (pArray)
		{
			//LOGE("nUserCfgCount:%d\n",pAccessUsrCfgMgr->nUserCfgCount);
			pUserCfg = pAccessUsrCfgMgr->pUserCfg;
			while(pUserCfg)
			{
				//LOGE("pUserCfg->szUserName:%s\n",pUserCfg->szUserName);
				pItem = Common_Json_SetAttrValue(pArray,nWhich,NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem,-1,"UserName",Common_Json_Type_String,pUserCfg->szUserName,0,0);
				Common_Json_SetAttrValue(pItem,-1,"EncryptMethod",Common_Json_Type_Number,NULL,pUserCfg->byEncryptMethod,0);
				Common_Json_SetAttrValue(pItem,-1,"DefaultPwd",Common_Json_Type_String,pUserCfg->szDefaultPassword,0,0);
				pArray1 = Common_Json_SetAttrValue(pItem,-1,"Password",Common_Json_Type_Array,NULL,0,0);
				if(pArray1){
					for(i = 0,nIndex = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++){
						if(pUserCfg->szPassword[i]){
							Common_Json_SetAttrValue(pArray1,nIndex,NULL,Common_Json_Type_String,pUserCfg->szPassword[i],0,0);
							nIndex++;
						}
					}
				}
				#if 0
				Common_Json_SetAttrValue(pItem,-1,"TempPassword",Common_Json_Type_String,pUserCfg->szTempPassword,0,0);
				Common_Json_SetAttrValue(pItem,-1,"SerialNumber",Common_Json_Type_String,pUserCfg->szSerialNumber,0,0);
				Common_Json_SetAttrValue(pItem,-1,"TempCreateTime",Common_Json_Type_Number,NULL,pUserCfg->tTempCreateTime,0);
				Common_Json_SetAttrValue(pItem,-1,"TempValidTime",Common_Json_Type_Number,NULL,pUserCfg->tTempValidTime,0);
				#endif
				Common_Json_SetAttrValue(pItem,-1,"Priority",Common_Json_Type_Number,NULL,pUserCfg->byPriority,0);
				Common_Json_SetAttrValue(pItem,-1,"Forbidden",Common_Json_Type_Number,NULL,pUserCfg->bForbidden,0);
				Common_Json_SetAttrValue(pItem,-1,"Remote",Common_Json_Type_Number,NULL,pUserCfg->bRemote,0);
				Common_Json_SetAttrValue(pItem,-1,"NeedOnlineAuth",Common_Json_Type_Number,NULL,pUserCfg->bNeedOnlineAuth,0);
				Common_Json_SetAttrValue(pItem,-1,"OnlineAuthListCount",Common_Json_Type_Number,NULL,pUserCfg->byOnlineAuthListCount,0);
				//pArray1 = Common_Json_SetAttrValue(pArray,-1,"Password",Common_Json_Type_Array,pUserCfg->szOnlineAuthManList,0,0);
				pArray1 = Common_Json_SetAttrValue(pItem,-1,"OnlineAuthManList",Common_Json_Type_Array,NULL,0,0);
				if(pArray1){
					for(i = 0,nIndex = 0;i < pUserCfg->byOnlineAuthListCount;i++){
						if(pUserCfg->szOnlineAuthManList[i]){
							Common_Json_SetAttrValue(pArray1,nIndex,NULL,Common_Json_Type_String,pUserCfg->szOnlineAuthManList[i],0,0);
							nIndex++;
						}
					}
				}
				Common_Json_SetAttrValue(pItem,-1,"BindIPv4",Common_Json_Type_String,pUserCfg->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pItem,-1,"BindIPv6",Common_Json_Type_String,pUserCfg->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pItem,-1,"BindMAC",Common_Json_Type_String,pUserCfg->szBindMac,0,0);

				pItem1 = Common_Json_SetAttrValue(pItem,-1,"LocalRight",Common_Json_Type_Object,NULL,0,0);
				if(pItem1)
				{
					AccessUserRight_T *pLocalR = pUserCfg->pLocalRight;
					if(pLocalR)
					{
						U32 u32Right = 0;

						u32Right = get_userRight(pLocalR);
						Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32Right,0);
						if(pLocalR->nChanRightCount>0&&pLocalR->pChanRight)
						{
							AccessChanRight_T tChanR;
							pArray1 = Common_Json_SetAttrValue(pItem1,-1,"ChanRight",Common_Json_Type_Array,NULL,0,0);
							for(i = 0;i < pLocalR->nChanRightCount;i++)
							{
								U32 u32ChanRight = 0;

								pItem1 = Common_Json_SetAttrValue(pArray1,i,NULL,Common_Json_Type_Object,NULL,0,0);
								if(!pItem1)
									continue;
								memset(&tChanR,0,sizeof(AccessChanRight_T));
								tChanR = pLocalR->pChanRight[i];
								u32ChanRight = get_chanRight(&tChanR);
								Common_Json_SetAttrValue(pItem1,-1,"DeviceNo",Common_Json_Type_Number,NULL,tChanR.wDeviceNo,0);
								Common_Json_SetAttrValue(pItem1,-1,"ChannelNo",Common_Json_Type_Number,NULL,tChanR.wChannelNo,0);
								Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32ChanRight,0);
							}
						}
					}
				}
				pItem1 = Common_Json_SetAttrValue(pItem,-1,"RemoteRight",Common_Json_Type_Object,NULL,0,0);
				if(pItem1)
				{
					AccessUserRight_T *pRemoteR = pUserCfg->pRemoteRight;
					if(pRemoteR)
					{
						U32 u32Right = 0;

						u32Right = get_userRight(pRemoteR);
						Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32Right,0);
						if(pRemoteR->nChanRightCount>0&&pRemoteR->pChanRight)
						{
							AccessChanRight_T tChanR;
							pArray1 = Common_Json_SetAttrValue(pItem1,-1,"ChanRight",Common_Json_Type_Array,NULL,0,0);
							for(i = 0;i < pRemoteR->nChanRightCount;i++)
							{
								U32 u32ChanRight = 0;

								pItem1 = Common_Json_SetAttrValue(pArray1,i,NULL,Common_Json_Type_Object,NULL,0,0);
								if(!pItem1)
									continue;
								memset(&tChanR,0,sizeof(AccessChanRight_T));
								tChanR = pRemoteR->pChanRight[i];
								u32ChanRight = get_chanRight(&tChanR);
								Common_Json_SetAttrValue(pItem1,-1,"DeviceNo",Common_Json_Type_Number,NULL,tChanR.wDeviceNo,0);
								Common_Json_SetAttrValue(pItem1,-1,"ChannelNo",Common_Json_Type_Number,NULL,tChanR.wChannelNo,0);
								Common_Json_SetAttrValue(pItem1,-1,"RightMask",Common_Json_Type_Number,NULL,u32ChanRight,0);
							}
						}
					}
				}
				nWhich++;
				pUserCfg = pUserCfg->pNext;
			}
		}
		pItem = Common_Json_SetAttrValue(pConfig,-1,"IpTable",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pItem,-1,"Mode",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->iAccessMode,0);
		Common_Json_SetAttrValue(pItem,-1,"StartTime",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->starttime,0);
		Common_Json_SetAttrValue(pItem,-1,"EndTime",Common_Json_Type_Number,NULL,pAccessUsrCfgMgr->endtime,0);
		pArray = Common_Json_SetAttrValue(pItem,-1,"WhiteList",Common_Json_Type_Array,NULL,0,0);
		for(i = 0,nWhich = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pWhiteList[i];
			if(pBindInfo)
			{
				pItem1 = Common_Json_SetAttrValue(pArray,nWhich,NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"Ipv4",Common_Json_Type_String,pBindInfo->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"EndIpv4",Common_Json_Type_String,pBindInfo->szBindEndIpv4,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"Ipv6",Common_Json_Type_String,pBindInfo->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"EndIpv6",Common_Json_Type_String,pBindInfo->szBindEndIpv6,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"Direction",Common_Json_Type_Number,NULL,pBindInfo->iDirection,0);
				Common_Json_SetAttrValue(pItem1,-1,"Protocol",Common_Json_Type_Number,NULL,pBindInfo->iProtocol,0);
				nWhich++;
			}
		}
		pArray = Common_Json_SetAttrValue(pItem,-1,"BlackList",Common_Json_Type_Array,NULL,0,0);
		for(i = 0,nWhich = 0;i < MAX_IPTABLE_COUNT;i++)
		{
			BindInfo_T *pBindInfo = pAccessUsrCfgMgr->pBlackList[i];
			if(pBindInfo)
			{
				pItem1 = Common_Json_SetAttrValue(pArray,nWhich,NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"Ipv4",Common_Json_Type_String,pBindInfo->szBindIpv4,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"EndIpv4",Common_Json_Type_String,pBindInfo->szBindEndIpv4,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"Ipv6",Common_Json_Type_String,pBindInfo->szBindIpv6,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"EndIpv6",Common_Json_Type_String,pBindInfo->szBindEndIpv6,0,0);
				Common_Json_SetAttrValue(pItem1,-1,"Direction",Common_Json_Type_Number,NULL,pBindInfo->iDirection,0);
				Common_Json_SetAttrValue(pItem1,-1,"Protocol",Common_Json_Type_Number,NULL,pBindInfo->iProtocol,0);
				nWhich++;
			}
		}
		Module_SaveConfig(hModuleHandle,pConfig);
		Common_Json_Delete(pConfig);
		pConfig = NULL;
	}
	return 0;
}

int access_CreateDefCfg(ModuleHandle_T hModuleHandle)
{
	S32 u32UserCfgCount = 0;
	S32 levelList[] = {LEVEL_ADMIN,LEVEL_DEFAULT,LEVEL_ADMIN};
	S8 *pUserList[] = {(S8*)"admin",(S8*)"guest",(S8*)"(null)"};
	S8 *pPwdList[] = {(S8*)"123456",(S8*)"123456",(S8*)"ovfsZSJQZLHL"};
	S8 *pEncrpt = NULL,*pTemp = NULL;
	OVFS_ABILITY *pAbility = get_channel_ability();
	AccessUserCfg *pUserCfg = NULL,*pTempCfg = NULL;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr = g_subscribeMgr.pAccessUsrCfgMgr;
	if(!pAbility)
	{
		LOGE("pAbility == NULL\n");
		return -1;
	}
	if(!pAccessUsrCfgMgr)
	{
		pAccessUsrCfgMgr = (AccessUserCfgMgr_T*)Common_Malloc(sizeof(AccessUserCfgMgr_T), 0, __FUNCTION__, __LINE__);
		if(!pAccessUsrCfgMgr)
		{
			LOGE("pAccessUsrCfgMgr == NULL\n");
			return -1;
		}
		memset(pAccessUsrCfgMgr,0,sizeof(AccessUserCfgMgr_T));
		g_subscribeMgr.pAccessUsrCfgMgr = pAccessUsrCfgMgr;
	}

	do{
		pUserCfg = (AccessUserCfg*)Common_Malloc(sizeof(AccessUserCfg), 0, __FUNCTION__, __LINE__);
		if(!pUserCfg)
		{
			LOGE("pUserCfg == NULL\n");
			break;
		}
		memset(pUserCfg,0,sizeof(AccessUserCfg));
		if(!pAccessUsrCfgMgr->pUserCfg)
			pAccessUsrCfgMgr->pUserCfg = pUserCfg;
		if(pTempCfg)
			pTempCfg->pNext = pUserCfg;
		pUserCfg->pPrev = pTempCfg;
		pUserCfg->pNext = NULL;
		pTempCfg = pUserCfg;
		pUserCfg->szUserName = Common_StrDup(pUserList[u32UserCfgCount], __FUNCTION__, __LINE__);
		pEncrpt = static_EncryptString(SYSTEM_TEXT, pPwdList[u32UserCfgCount], pTemp, 0);
		pUserCfg->szDefaultPassword = Common_StrDup(pEncrpt, __FUNCTION__, __LINE__);
		pUserCfg->szPassword[0] = Common_StrDup(pEncrpt, __FUNCTION__, __LINE__);
		pUserCfg->byPwdCount = 1;
		pUserCfg->byEncryptMethod = SYSTEM_TEXT;
		pUserCfg->byPriority = levelList[u32UserCfgCount];
		pUserCfg->bForbidden = 0;
		pUserCfg->bNeedOnlineAuth = 1;
		pUserCfg->byOnlineAuthListCount = 1;
		pUserCfg->szOnlineAuthManList[0] = Common_StrDup(pUserList[u32UserCfgCount], __FUNCTION__, __LINE__);
		pUserCfg->pLocalRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
		if(pUserCfg->pLocalRight)
		{
			memset(pUserCfg->pLocalRight,0,sizeof(AccessUserRight_T));
			set_defaultUsrRight(pUserCfg->pLocalRight,pUserCfg->byPriority);
		}
		pUserCfg->pRemoteRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
		if(pUserCfg->pRemoteRight)
		{
			memset(pUserCfg->pRemoteRight,0,sizeof(AccessUserRight_T));
			set_defaultUsrRight(pUserCfg->pRemoteRight,pUserCfg->byPriority);
		}
		if(pEncrpt)
			Common_Free(pEncrpt, __FUNCTION__, __LINE__);
		pEncrpt = NULL;
		u32UserCfgCount++;
	}while(u32UserCfgCount<ARRAYSIZE(levelList));
	pAccessUsrCfgMgr->nUserCfgCount = u32UserCfgCount;
	access_saveCfg(hModuleHandle);
	return 0;
}

OVFS_ABILITY * get_channel_ability()
{
	return g_ability;
}

int request_channel_ability(ModuleHandle_T hModuleHandle)
{
	S32 iRet = 0;
	S8 strcmd[64] = {0};
	POVFS_ABILITY pAbility = NULL;
	cJSON_Struct *pConfig,*pOutParams = NULL;
	S32 i = 0,j = 0,iDevNum = -1,iChanNum = -1,iStreamNum = -1,iMaxChanNum = -1;
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Boardsys/Video/Ability/Number",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
	}
	iRet = Module_CallFunctions(hModuleHandle,pConfig,&pOutParams,3000);
	Common_Json_Delete(pConfig);
	pConfig = NULL;
	if(!pOutParams||(iRet != 0))
	{
		iDevNum = 1;
		iChanNum = 1;
		iStreamNum = 2;
		LOGW("pOutParams == NULL!iRet:%d\n",iRet);
	}
	else
	{
		Common_Json_GetAttrValue(pOutParams,-1,"Data/DevTotalNum",NULL,NULL,&iDevNum,NULL);
		if(iDevNum<0)
		{
			iDevNum = 1;
			iRet = -1;
		}
		Common_Json_GetAttrValue(pOutParams,-1,"Data/ChanTotalNum",NULL,NULL,&iChanNum,NULL);
		if(iChanNum<0)
		{
			iChanNum = 1;
			iRet = -1;
		}
		Common_Json_GetAttrValue(pOutParams,-1,"Data/MaxChanNum",NULL,NULL,&iMaxChanNum,NULL);
		if(iMaxChanNum > 0)
		{
			if(iChanNum < iMaxChanNum)
				iChanNum = iMaxChanNum;
		}
		Common_Json_GetAttrValue(pOutParams,-1,"Data/StreamNum",NULL,NULL,&iStreamNum,NULL);
		if(iStreamNum<0)
		{
			iStreamNum = 2;
			iRet = -1;
		}
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
		if(iMaxChanNum > 0)
		{
			iChanNum = iMaxChanNum;
		}
		else
		{
			sprintf(strcmd,"Data/viDev%d/viChanNum",i);
			Common_Json_GetAttrValue(pOutParams,-1,strcmd,NULL,NULL,&iChanNum,NULL);
			if(iChanNum <= 0)
				iChanNum = 1;
		}
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
				iStreamNum = 2;
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
	return iRet;
}

int init_access_res(ModuleHandle_T hModuleHandle)
{
	cJSON_Struct *pRoot = NULL,*pChild = NULL,*pItem = NULL;

	g_rest_res = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (g_rest_res)
	{
		pRoot = Common_Json_SetAttrValue(g_rest_res,-1,"Access",Common_Json_Type_Object,NULL,0,0);
		set_method_callback(&pRoot,get_access_top_res,NULL,NULL,NULL);
		if(pRoot){
			pChild = Common_Json_SetAttrValue(pRoot,-1,"UserCfg",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_access_usercfg,put_access_usercfg,post_access_usercfg,delete_access_usercfg);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"PasswordLost",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_access_findpwd,NULL,NULL,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"InvalidFindPwd",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,put_access_invalidpwd,NULL,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"RestorePassword",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,put_access_restorepwd,NULL,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"OnlineUser",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_online_user,put_online_user,NULL,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"SyncOnlineUser",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,NULL,sync_online_user,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"GlobalId",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,NULL,post_global_userid,delete_global_userid);
			#if 0
			pChild = Common_Json_SetAttrValue(pRoot,-1,"BindUser",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,NULL,NULL,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"Subscribe",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,NULL,NULL,NULL);
			#endif
			pChild = Common_Json_SetAttrValue(pRoot,-1,"Restore",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,NULL,put_access_restore,NULL,NULL);
			pChild = Common_Json_SetAttrValue(pRoot,-1,"AccessControl",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_access_control_res,NULL,NULL,NULL);
			if(pChild)
			{
				pItem = Common_Json_SetAttrValue(pChild,-1,"WhiteList",Common_Json_Type_Object,NULL,0,0);
				set_method_callback(&pItem,get_access_whitelist_res,put_access_whitelist_res,post_access_whitelist_res,delete_access_whitelist_res);
				pItem = Common_Json_SetAttrValue(pChild,-1,"BlackList",Common_Json_Type_Object,NULL,0,0);
				set_method_callback(&pItem,get_access_blacklist_res,put_access_blacklist_res,post_access_blacklist_res,delete_access_blacklist_res);
				pItem = Common_Json_SetAttrValue(pChild,-1,"AccessMode",Common_Json_Type_Object,NULL,0,0);
				set_method_callback(&pItem,get_access_mode_res,put_access_mode_res,NULL,NULL);
			}
			pChild = Common_Json_SetAttrValue(pRoot,-1,"PrintDebug",Common_Json_Type_Object,NULL,0,0);
			set_method_callback(&pChild,get_access_debug,put_access_debug,NULL,NULL);
		}
	}
	updateStoreItem(hModuleHandle);
	g_subscribeMgr.hModuleHandle = hModuleHandle;
	Common_Lock_Create(&g_subscribeMgr.hCfgLock, NULL);
	Common_DList_Init(&g_subscribeMgr.listdl,freeSubscribeListNode);
	Module_RegisterSubscribe(hModuleHandle,(S8*)"/Access/Subscribe/UserCfg",static_Access_Subscribe_fxn,NULL);
	Module_RegisterSubscribe(hModuleHandle,(S8*)"/Access/Subscribe/OnlineUser",static_Access_Subscribe_fxn,NULL);
	Module_RegisterSubscribe(hModuleHandle,(S8*)"/Access/Subscribe/Iptable",static_Access_Subscribe_fxn,NULL);
	Common_Thread_Create(&g_subscribeMgr.hThread,__FUNCTION__,0,0,Thread_SubscribeHandle,(void*)&g_subscribeMgr.hModuleHandle);
	//LoadDefRightCfg(hModuleHandle,&g_right_res);
	//ovfs_print_json(g_right_res);
	return 0;
}

int get_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
	int iRet = -1;
	iRet = AnalyzeUriAndMakeResult(MATHOD_GET,pUri,pAddData,outJson);
	return iRet;
}

int put_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
	int iRet = -1;
	iRet = AnalyzeUriAndMakeResult(MATHOD_PUT,pUri,pAddData,outJson);
	return iRet;
}

int post_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
	int iRet = -1;
	iRet = AnalyzeUriAndMakeResult(MATHOD_POST,pUri,pAddData,outJson);
	return iRet;
}

int delete_access_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson)
{
	int iRet = -1;
	iRet = AnalyzeUriAndMakeResult(MATHOD_DELETE,pUri,pAddData,outJson);
	return iRet;
}
