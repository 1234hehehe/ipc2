#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include <pthread.h>

#include "ovfs_network_rest.h"
#include "ovfs_network_app.h"
#include "ovfs_network_rest_common.h"



static Common_cJSON_T* 		  s_network_rest_pathRoot = NULL;
static NETWORK_REST_INIT_T	  s_network_rest_initInfo;
static pthread_mutex_t		  s_network_rest_lock = PTHREAD_MUTEX_INITIALIZER;

static int CallRestFunc(const char* uriString,const char* uriContion,Common_cJSON_T* jsonObj,Common_cJSON_T* in,Common_cJSON_T* out,int type) //get-0 put-1 post-2 delete-3
{
	int ret = 0;
	NETWORK_REST_NODE_ATTR_T* info = (NETWORK_REST_NODE_ATTR_T*)jsonObj->pExtData;
	if(info == NULL)
	{
		LOGE("ex data is NULL!\n");
		return -1;
	}
	
	if(type == 0)
	{
		if(info->get)
			ret = info->get(uriString,uriContion,jsonObj,in,out);
		else
			ret = EC_NETWORK_REST_METHOD_NOT_FOUND;
	}
	else if(type == 1)
	{
		if(info->put)
			ret = info->put(uriString,uriContion,jsonObj,in,out);
		else 
			ret = EC_NETWORK_REST_METHOD_NOT_FOUND;
	}
	else if(type == 2)
	{
		if(info->post)
			ret = info->post(uriString,uriContion,jsonObj,in,out);
		else 
			ret = EC_NETWORK_REST_METHOD_NOT_FOUND;
	}
	else if(type == 3)
	{
		if(info->fdelete)
			ret = info->fdelete(uriString,uriContion,jsonObj,in,out);
		else
			ret = EC_NETWORK_REST_METHOD_NOT_FOUND;
	}
	
	return ret;
}

static int ParseUriString(const char* uriString,const char* uriCondition,Common_cJSON_T* in,Common_cJSON_T* out,int type) //get-0 put-1 post-2 delete-3
{
	int ret = 0;
	char* uriBk = Common_StrDup((S8 *)uriString,__FUNCTION__,__LINE__);

	char* srcStr = uriBk;
	char* curStr = NULL;
	char* nextStr = NULL;
	Common_cJSON_T* tmp = s_network_rest_pathRoot;

	while(srcStr)
	{
		ret = Common_UriOneParse(srcStr,NULL,&curStr,&nextStr);
		if(ret != 0)
		{
			LOGE("Uri parse fail! lable=%s\n",curStr);
			if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			ret = EC_NETWORK_REST_OPERATION_FAIL;
			break;
		}

		tmp = Common_cJSON_GetObjectItem(tmp,curStr);
		if(tmp == NULL)
		{
			LOGE("Can't find:%s\n",curStr);
			if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			ret = EC_NETWORK_REST_URI_PATH_NOT_EXIST;
			break;
		}
		
		if(nextStr == NULL)
		{
			Common_Free(curStr,__FUNCTION__,__LINE__);
			Common_Free(uriBk,__FUNCTION__,__LINE__);;
			return CallRestFunc(uriString,uriCondition,tmp,in,out,type);	
		}

		
		srcStr = nextStr;
		if(curStr)
			Common_Free(curStr,__FUNCTION__,__LINE__);
		curStr = NULL;
	}

	Common_Free(uriBk,__FUNCTION__,__LINE__);
	return ret;
}

static Common_cJSON_T* RootUriInit()
{
    s_network_rest_pathRoot = Common_cJSON_CreateObject();

	Common_cJSON_T* networkRoot = Common_cJSON_CreateObject();
	NETWORK_REST_NODE_ATTR_T attr;
	memset(&attr,0,sizeof(attr));
	attr.get 			= NetWork_RestComm_GetUriList;
	attr.describtion 	= "NetWork root URI. Get URI list on it.";
	attr.label			= "NetWorkRoot";
	Common_cJSON_SetItemExtData(networkRoot,&attr,sizeof(attr));
	Common_cJSON_AddItemToObject(s_network_rest_pathRoot,"NetWork",networkRoot);

	return networkRoot;
}

static int InitModule(Common_cJSON_T* networkRoot)
{
    int ret = 0;

	ret = NetWork_Rest_NetAttr_init(networkRoot);				
	if(ret != 0)
	{
		LOGE("rest NetAttr init fail!\n");
		return -1;
	}

	ret = NetWork_Rest_NetApp_init(networkRoot);				
	if(ret != 0)
	{
		LOGE("rest NetApp init fail!\n");
		return -1;
	}
	
	ret = NetWork_Rest_Functions_init(networkRoot);				
	if(ret != 0)
	{
		LOGE("rest Functions init fail!\n");
		return -1;
	}	

	ret = NetWork_Rest_Status_init(networkRoot);				
	if(ret != 0)
	{
		LOGE("rest Status init fail!\n");
		return -1;
	}
	
	ret = NetWork_Rest_Subscribe_init(networkRoot);				
	if(ret != 0)
	{
		LOGE("rest Subscribe init fail!\n");
		return -1;
	}

	ret = NetWork_Rest_Restore_init(networkRoot);				
	if(ret != 0)
	{
		LOGE("rest Restore init fail!\n");
		return -1;
	}
	
	return 0;
}

/*
static int StartModule()
{
    int ret = 0;
	//int ret = RestVideo_Start();
	if(ret != 0)
	{
		LOGE("start video fail!\n");
		return -1;
	}
	
	return 0;
}
*/
int NetWork_Rest_Get_NetworkItem_OfRootUri(Common_cJSON_T** networkRoot)
{
	Common_cJSON_T* tmp = NULL;
	
	tmp = Common_cJSON_GetObjectItem(s_network_rest_pathRoot, "NetWork");

	*networkRoot = tmp;
	
	return 0;
}

int NetWork_Rest_Init(NETWORK_REST_INIT_T* initInfo)
{
    int ret = 0;
		
	LOGW("start Initialization.\n");
	//pthread_mutex_init(&s_network_rest_lock,NULL);
	
	memcpy(&s_network_rest_initInfo,initInfo,sizeof(NETWORK_REST_INIT_T));

	//ret = RestList_Init();								// init task list
	if(ret != 0)
	{
		LOGE("task list init fail!\n");
		return -1;
	}

	Common_cJSON_T* networkRoot = RootUriInit();	// generate root URI
	if(networkRoot == NULL)
	{
		LOGE("Add boardsys root fail!\n");
		return -1;
	}

	NETWORK_REST_CB_TALBE_T cbTable;
	memset(&cbTable,0,sizeof(cbTable));
	cbTable.callFunc   	=	 	(NETWORK_REST_FUNC_CALL)initInfo->funcCall;
	cbTable.put			= 		NetWork_Rest_Put;
	cbTable.get			= 		NetWork_Rest_Get;
	cbTable.moduleHdl	= 		initInfo->moduleHdl;
	ret = NetWork_RestComm_Init(&cbTable);						// common Initialization . cfg list & task list
	if(ret != 0)
	{
		LOGE("common init fail!\n");
		return -1;
	}

	ret = NetWork_App_Init();
	if(ret != 0)
	{
		LOGE("NetWork_App init fail!\n");
		return -1;
	}
	
	ret = NetWork_Cfgm_init();
	if(ret != 0)
	{
		LOGE("NetWork_Cfgm init fail!\n");
		return -1;
	}
	
	ret = InitModule(networkRoot);
	if(ret != 0)
	{
		LOGE("module init fail!\n");
		return -1;
	}

	ret = NetWork_LoadCfgAndStart();
	if(ret != 0)
	{
		LOGE("load cfg fail!\n");
		return -1;
	}

	//ret = StartModule();
	//if(ret != 0)
	//{
	//	LOGE("start module fail!\n");
	//	return -1;
	//}
	
	return 0;
}

int NetWork_Rest_Destroy()
{
	LOGW("start Destroy.\n");
	//RestVideo_Destroy();
	
	if(s_network_rest_pathRoot)
	{
		Common_cJSON_Delete(s_network_rest_pathRoot);
	}	

	pthread_mutex_destroy(&s_network_rest_lock);
	
	return 0;
}

int NetWork_Rest_Get(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	if(access("/tmp/network_debug",F_OK) == 0)
	{
		char* outStr = NULL;
		LOGI("uriString:%s	condition:%s in:%s\n",uriString,condition,outStr = Common_cJSON_PrintUnformatted(in,NULL));
		if(outStr)
			Common_Free(outStr,__FUNCTION__,__LINE__);
	}
		
	pthread_mutex_lock(&s_network_rest_lock);
	
	int ret = ParseUriString(uriString,condition,in,out,0);

	pthread_mutex_unlock(&s_network_rest_lock);
	return ret;
}

int NetWork_Rest_Put(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	if(access("/tmp/network_debug",F_OK) == 0)
	{
		char* outStr = NULL;
		LOGI("uriString:%s	condition:%s in:%s\n",uriString,condition,outStr =Common_cJSON_PrintUnformatted(in,NULL));
		if(outStr)
			Common_Free(outStr,__FUNCTION__,__LINE__);
	}
		
	pthread_mutex_lock(&s_network_rest_lock);
	
	int ret = ParseUriString(uriString,condition,in,out,1);	
	if(ret == 0)
	{
		//int cfgHasChaned = RestCfg_MatchNode(uriString);
		//if(cfgHasChaned)
		//NetWork_RestComm_StartCfgChangedTimer();
		if(strstr(uriString,"Eth") == NULL)
			NetWork_SaveCfg();
	}
	
	pthread_mutex_unlock(&s_network_rest_lock);
	return ret > 0 ? 0 : ret;
}

int NetWork_Rest_Post(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	if(access("/tmp/network_debug",F_OK) == 0)
	{
		char* outStr = NULL;
		LOGI("uriString:%s	condition:%s in:%s\n",uriString,condition,outStr = Common_cJSON_PrintUnformatted(in,NULL));
		if(outStr)
			Common_Free(outStr,__FUNCTION__,__LINE__);
	}
		
	pthread_mutex_lock(&s_network_rest_lock);
	int ret = ParseUriString(uriString,condition,in,out,2);
	pthread_mutex_unlock(&s_network_rest_lock);
	return ret;
}

int NetWork_Rest_Delete(const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	if(access("/tmp/network_debug",F_OK) == 0)
	{
		char* outStr = NULL;
		LOGI("uriString:%s	condition:%s in:%s\n",uriString,condition,outStr = Common_cJSON_PrintUnformatted(in,NULL));
		if(outStr)
			Common_Free(outStr,__FUNCTION__,__LINE__);
	}
		
	pthread_mutex_lock(&s_network_rest_lock);
	int ret = ParseUriString(uriString,condition,in,out,3);
	pthread_mutex_unlock(&s_network_rest_lock);
	return ret;
}

const char* NetWork_Rest_GetErrString(int errorCode)
{
	return NetWork_RestComm_GetErrString(errorCode);
}

