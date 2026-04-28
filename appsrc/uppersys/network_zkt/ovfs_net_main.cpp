/*
 * ovfs_net_main.c
 *
 *  Created on: 2016年12月1日
 *      Author: shushi
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "libcommon_api.h"
#include "ovfs_network_rest.h"

static ModuleHandle_T s_network_hdl;
static int			  s_network_exit = 0;
static int            s_network_recv = 0;

static int PraseInputJson(Common_cJSON_T* inputData,char** method,char** uri,Common_cJSON_T** inData)
{
	Common_cJSON_T* header = Common_cJSON_GetObjectItem(inputData,"Header");
	if(header == NULL)
	{
		LOGE("Get header fail!\n");
		return -1;
	}

    Common_cJSON_T* tmp = NULL;
	tmp = Common_cJSON_GetObjectItem(header,"Method");
	if(tmp == NULL)
	{
		LOGE("Can't found method!\n");
		return -1;
	}
	if(method)
		*method = tmp->valuestring;

	tmp = Common_cJSON_GetObjectItem(header,"Uri");
	if(tmp == NULL)
	{
		LOGE("Can't found Uri.\n");
		return -1;
	}
	if(uri)
		*uri = tmp->valuestring;

	
	tmp = Common_cJSON_GetObjectItem(inputData,"Data");
	if(inData)
		*inData = tmp;

	return 0;
}

static int GetUriAndQue(const char* srcUri,char** uriStr, char** conditionStr)
{
	char* tmp = (char*)strstr(srcUri,"?");
	if(tmp)
	{
		char buff[256];
		memset(buff,0,sizeof(buff));
		memcpy(buff,srcUri,(int)(tmp-srcUri));
		*uriStr			= Common_StrDup(buff,__FUNCTION__,__LINE__);
		*conditionStr 	= Common_StrDup(tmp+1,__FUNCTION__,__LINE__);
	}
	else
	{
		*uriStr 		= Common_StrDup((S8 *)srcUri,__FUNCTION__,__LINE__);
		*conditionStr 	= NULL;
	}
	
	return 0;
}

static Common_cJSON_T* GenerateOutParam(int retCode,Common_cJSON_T* outData)
{
	Common_cJSON_T* root = Common_cJSON_CreateObject();
	Common_cJSON_T* header = Common_cJSON_CreateObject();
	Common_cJSON_AddItemToObject(root,"Header",header);
	
	Common_cJSON_AddNumberToObject(header,"Code",retCode);
	if(retCode != 0)
		Common_cJSON_AddStringToObject(header,"Decribe",NetWork_Rest_GetErrString(retCode));

	if(outData)
		Common_cJSON_AddItemToObject(root,"Data",outData);
	
	return root;
}

static Common_cJSON_T* GenerateInParam(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inData)
{
	if(uriPath == NULL)
		return NULL;
	
	Common_cJSON_T* root = Common_cJSON_CreateObject();
	Common_cJSON_T* header = Common_cJSON_CreateObject();
	Common_cJSON_AddItemToObject(root,"Header",header);

	Common_cJSON_AddStringToObject(header,"Method",method);
	char uriBuff[256];
	memset(uriBuff,0,sizeof(uriBuff));
	int uriPathLen = strlen(uriPath);
	memcpy(uriBuff,uriPath,uriPathLen);
	if(condition)
	{
		uriBuff[uriPathLen] = '?';
		memcpy(uriBuff+uriPathLen+1,condition,strlen(condition));
	}
	
	Common_cJSON_AddStringToObject(header,"Uri",uriBuff);
	Common_cJSON_AddItemToObject(root,"Data",inData);
	
	return root;
}

static int NetWork_FuncCall(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T* outParam)
{
	Common_cJSON_T* inObj = GenerateInParam(method,uriPath,condition,inParam);
	cJSON_Struct* outObj = NULL;
	int ret = Module_CallFunctions(s_network_hdl,inObj,&outObj,3);
	Common_cJSON_Delete(inObj);

	Common_cJSON_Delete((Common_cJSON_T*)outObj);
	
	return ret;
}

static S32 NetWork_Module_CallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
    
    if (s_network_recv != 1)
    {
        return -1;
	}

    if(access("/tmp/network_debug",F_OK) == 0)
    {
		char* out = NULL;
		LOGI("recv call input:%s \n",out = Common_cJSON_PrintUnformatted((Common_cJSON_T*)pInParams,NULL));
		if(out)
			Common_Free(out,__FUNCTION__,__LINE__);
    }
	
	char* method 			= NULL;
	char* uri				= NULL;
	Common_cJSON_T* inData 	= NULL;
	int ret = PraseInputJson((Common_cJSON_T*)pInParams,&method,&uri,&inData);
	if(ret != 0)
	{
		LOGE("inparam parse fail!\n");
		return -1;
	}

	char* uriString 	= NULL;
	char* uriCondition 	= NULL;
	GetUriAndQue(uri,&uriString,&uriCondition);

	if(method == NULL)
    {
        LOGE("method is NULL!\n");
        return -1;
    }
	
	Common_cJSON_T* outData = Common_cJSON_CreateObject();
	if(Common_StriCmp((S8 *)method,(S8 *)"get")== 0)
	{
		ret = NetWork_Rest_Get(uriString,uriCondition,inData,outData);
	}
	else if(Common_StriCmp((S8 *)method,(S8 *)"put")== 0)
	{
		ret = NetWork_Rest_Put(uriString,uriCondition,inData,outData);
	}
	else if(Common_StriCmp((S8 *)method,(S8 *)"post")== 0)
	{
		ret = NetWork_Rest_Post(uriString,uriCondition,inData,outData);
	}	
	else if(Common_StriCmp((S8 *)method,(S8 *)"delete")== 0)
	{
		ret = NetWork_Rest_Delete(uriString,uriCondition,inData,outData);
	}		
	else
	{
		if(uriString)
			Common_Free(uriString,__FUNCTION__,__LINE__);
		if(uriCondition)
			Common_Free(uriCondition,__FUNCTION__,__LINE__);
		
		LOGE("Unknow method=%s\n",method);
		return -1;
	}

	if(outData->child == NULL)
	{
		Common_cJSON_Delete(outData);
		outData = NULL;	
	}
	
	*pOutParams = GenerateOutParam(ret,outData);	

	if(uriString)
		Common_Free(uriString,__FUNCTION__,__LINE__);
	if(uriCondition)
		Common_Free(uriCondition,__FUNCTION__,__LINE__);
	
	return 0;
}


S32 main(S32 argc,char* argv[])
{
	LOG_INIT((S8 *)"NetWork",COMMON_LOG_LV_HIGH);

    Common_RegistSigHandle(SIGSEGV);
    Common_RegistSigHandle(SIGILL);
    Common_RegistSigHandle(SIGABRT);	

	int ret = 0;

	s_network_recv = 0;
	
	Common_cJSON_T* param = Common_cJSON_CreateObject();
	Common_cJSON_AddStringToObject(param,"SystemName","ovfs");
	Common_cJSON_AddStringToObject(param,"ModuleName","NetWork");	
	//Common_cJSON_AddStringToObject(param,"RemoteDomain","127.0.0.1");	
	//Common_cJSON_AddNumberToObject(param,"RemotePort",100);
	//Common_cJSON_AddNumberToObject(param,"LocalPort",10);
	
	ret = Module_Init(&s_network_hdl,param,NULL,NetWork_Module_CallFunctions,NULL);
	Common_cJSON_Delete(param);
	if(ret != 0)
	{
		LOGE("module init fail!\n");
		return -1;
	}
	
	NETWORK_REST_INIT_T initInfo;
	memset(&initInfo,0,sizeof(initInfo));
	initInfo.funcCall  = (void*)NetWork_FuncCall;
	initInfo.moduleHdl = &s_network_hdl;
	
	ret = NetWork_Rest_Init(&initInfo);
	if(ret != 0)
	{
		LOGE("NetWork_Rest init Fail!\n");
		Module_Unint(&s_network_hdl);
		return -1;
	}

	s_network_recv = 1;
#if (defined PLATFORM_JZT30 || defined PLATFORM_JZT40 )
	int cnt = 0;
#endif//ifdef PLATFORM_JZT30
	while(s_network_exit == 0)
	{
		//LOGI("network sleep 2.\n");
		Common_Sleep(2,0);
#if (defined PLATFORM_JZT30 || defined PLATFORM_JZT40 )
		cnt ++;
		if(cnt == 30)
		{
			LOGW("drop cache\n");
			Common_System("echo 3 > /proc/sys/vm/drop_caches");
//			cnt = 0;
		}
#endif//ifdef PLATFORM_JZT30
	}

	LOG_UNINIT();
	
	return 0;
}


