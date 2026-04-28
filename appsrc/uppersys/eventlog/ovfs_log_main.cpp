#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/**********/
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "ovfs_log.h"


static int AnalyzeDataAndMakeResult(cJSON_Struct *dataJson,cJSON_Struct **pOutData)
{
	int iRet = -1;
    int iMethod = 0;
	char *pUri = NULL,*pMethod = NULL;
	cJSON_Struct *pData = NULL;
	if(!dataJson||!pOutData)
	{
		return -1;
	}

	if(Common_Json_GetAttrValueStr(dataJson, "Header/Method", &pMethod) == NULL ||
       Common_Json_GetAttrValueStr(dataJson, "Header/Uri", &pUri) == NULL)
	{
		return iRet;
	}

	pData = Common_Json_GetItem(dataJson,-1,"Data");
	if(pMethod)
	{
		if(0 == Common_StriCmp(pMethod,(S8*)"get"))
		{
			iMethod = MATHOD_GET;
		}
		else if (0 == Common_StriCmp(pMethod,(S8*)"put"))
		{
			iMethod = MATHOD_PUT;
		}
		else if(0 == Common_StriCmp(pMethod,(S8*)"post"))
		{
			iMethod = MATHOD_POST;
		}
		else if (0 == Common_StriCmp(pMethod,(S8*)"delete"))
		{
			iMethod = MATHOD_DELETE;
		}

        iRet = ovfs_deal_log_res(iMethod,pUri,pData,pOutData);
	}
	return iRet;
}

static S32 static_Module_CallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
	int iRet = 0;
	if (pInParams)
	{
		//ovfs_print_json(pInParams);
		AnalyzeDataAndMakeResult(pInParams,pOutParams);
	}
	return iRet;
}

S32 main(S32 argc,char *argv[])
{
	S8 *pModuleName = (S8*)"EventLog";
	ModuleHandle_T hModuleHandle = NULL;
	cJSON_Struct *pConfig = NULL;
	S32 nRet = 0;

	Common_RegistSigHandle(SIGSEGV);
    Common_RegistSigHandle(SIGILL);
	Common_RegistSigHandle(SIGABRT);

	LOG_INIT(pModuleName,COMMON_LOG_LV_HIGH);

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig,-1,"SystemName",Common_Json_Type_String,"ovfs",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"ModuleName",Common_Json_Type_String,pModuleName,0,0);
	}

	nRet = Module_Init(&hModuleHandle,pConfig,NULL,static_Module_CallFunctions,NULL);

    if(nRet != 0)
    {
        LOGD("Module init failed! [%d]\n",nRet);
        return -1;
    }

	Common_Json_Delete(pConfig);
	pConfig = NULL;

	ovfs_init_log(hModuleHandle);

	while(1)
	{
		Common_Sleep(5,0);
	}

	return 0;
}
