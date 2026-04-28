#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

#include"ovfs_rest_common.h"
#include"common_json_str_ops.h"
/*
static REST_ER_T s_rest_errorInfo[] = 
{
    {EC_HAL_PARAM_INVALID,EC_HAL_PARAM_INVALID_STR},        
    {EC_HAL_OPRATE_FAIL,EC_HAL_OPRATE_FAIL_STR},        
    {EC_HAL_SENSOR_UNKNOW,EC_HAL_SENSOR_UNKNOW_STR},        
    {EC_HAL_MIPI_OPS_FAIL,EC_HAL_MIPI_OPS_FAIL_STR},      
    {EC_HAL_MODULE_NO_INIT,EC_HAL_MODULE_NO_INIT_STR},      
    {EC_HAL_MODULE_INIT_FAIL,EC_HAL_MODULE_INIT_FAIL_STR},      
    {EC_HAL_PLATFORM_UNKNOW,EC_HAL_PLATFORM_UNKNOW_STR},                
    {EC_HAL_GET_HAL_RES_FAIL,EC_HAL_GET_HAL_RES_FAIL_STR},                
    {EC_HAL_IDX_NOT_SUPPORT,EC_HAL_IDX_NOT_SUPPORT_STR},
    {EC_HAL_RE_OPERATION,EC_HAL_RE_OPERATION_STR},
    {EC_HAL_VI_DEV_NOT_STOP,EC_HAL_VI_DEV_NOT_STOP_STR},
    {EC_HAL_RES_RUN_OUT,EC_HAL_RES_RUN_OUT_STR},
    {EC_HAL_MATCH_ITEM_FAIL,EC_HAL_MATCH_ITEM_FAIL_STR},
    {EC_HAL_TIMEOUT,EC_HAL_TIMEOUT_STR},
	{EC_HAL_FUNCTION_NOT_SUPPORT,EC_HAL_FUNCTION_NOT_SUPPORT_STR},
	
	{EC_REST_METHOD_NOT_FOUND,EC_REST_METHOD_NOT_FOUND_STR},
	{EC_REST_URI_PATH_NOT_EXIST,EC_REST_URI_PATH_NOT_EXIST_STR},
	{EC_REST_OPERATION_FAIL,EC_REST_OPERATION_FAIL_STR},
};
*/
static REST_CB_TALBE_T		s_rest_cbTable;
//static Common_Timer_T	s_rest_cfgTimer = NULL;
static pthread_mutex_t		s_rest_lock;
//static Common_cJSON_T* 	s_rest_pathRoot = NULL;
static REST_RESO_STR_T         s_rest_resoStr [] = 
{
    {"12MP(4000*3000)",4000,3000},
    {"9MP(3000*3000)",3000,3000},
    {"8MP(3840*2160)",3840,2160},
    {"5MP(2592*1944)",2592,1944},
    {"3MP(2048*1536)",2048,1536},
    {"2MP(1500*1500)",1500,1500},
    {"2MP(2048*1024)",2048,1024},
    {"1080P(1920*1080)",1920,1080},
    {"960P(1280*960)",1280,960},
    {"720P(1280*720)",1280,720},
    {"QHD(960*540)",960,540},
    {"D1(720*576)",720,576},
    {"D1(720*480)",720,480},
    {"VGA(640*480)",640,480},
    {"Q720P(640*360)",640,360},
    {"CIF(352*288)",352,288},
    {NULL,0,0},     
};

char* RestComm_GetUriStrFromJson(const Common_cJSON_T* jsonObj)
{
	const Common_cJSON_T* tmp = jsonObj;
	char *strBuff= NULL;
	int usedLen = 1;
	int buffSize = 1024*32;
	strBuff = Common_Malloc(buffSize,0,__FUNCTION__,__LINE__);
	if(strBuff == NULL)
	{
		return NULL;
	}
	memset(strBuff,0,buffSize);
	
	while(tmp && tmp->string)
	{	
		int strLen = strlen(tmp->string);
		if((buffSize-usedLen-strLen-1) < 0)
		{
			LOGE("Buffer is too small.\n");
			Common_Free(strBuff,__FUNCTION__,__LINE__);
			strBuff = NULL;
			return NULL;
		}
		memcpy(strBuff+(buffSize-usedLen-strLen),tmp->string,strLen);
		strBuff[buffSize-usedLen-strLen-1] = '/';
		usedLen = usedLen + strLen + 1;

		tmp = tmp->pParent;
	}

	if(usedLen > 1)
	{
		char *pRetStr = Common_StrDup((S8*)(&(strBuff[buffSize-usedLen])),__FUNCTION__,__LINE__);
		Common_Free(strBuff,__FUNCTION__,__LINE__);
		strBuff = NULL;
		return pRetStr;
	}
	Common_Free(strBuff,__FUNCTION__,__LINE__);
	strBuff = NULL;
	return NULL;
}

int RestComm_GetUriList(const char* uri,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
	if(curObj == NULL || out == NULL)
	{
		LOGE("curObj or outObj is NULL!\n");
		return -1;
	}
	
	Common_cJSON_T* uriList = Common_cJSON_CreateArray();
	Common_cJSON_AddItemToObject(out,"ResList",uriList);

	Common_cJSON_T* tmpCnt = curObj->child;
	while(tmpCnt)
	{
	
		Common_cJSON_T* tmpObj = Common_cJSON_CreateObject();
		
		char* uriString = RestComm_GetUriStrFromJson(tmpCnt);
		Common_cJSON_AddStringToObject(tmpObj,"Uri",uriString);
		Common_Free(uriString,__FUNCTION__,__LINE__);
		
		REST_NODE_ATTR_T* info = (REST_NODE_ATTR_T*)tmpCnt->pExtData;
		if(info)
		{
			Common_cJSON_AddStringToObject(tmpObj,"Label",info->label);
			Common_cJSON_AddStringToObject(tmpObj,"Describe",info->describtion);
			Common_cJSON_T* methodArr =  Common_cJSON_CreateArray();
			if(info->get)
			{
				Common_cJSON_T* tmp = Common_cJSON_CreateString("get");
				Common_cJSON_AddItemToArray(methodArr,tmp);
			}

			if(info->put)
			{
				Common_cJSON_T* tmp = Common_cJSON_CreateString("put");
				Common_cJSON_AddItemToArray(methodArr,tmp);
			}
			
			if(info->post)
			{
				Common_cJSON_T* tmp = Common_cJSON_CreateString("post");
				Common_cJSON_AddItemToArray(methodArr,tmp);
			}

			if(info->delet)
			{
				Common_cJSON_T* tmp = Common_cJSON_CreateString("delete");
				Common_cJSON_AddItemToArray(methodArr,tmp);
			}	

			Common_cJSON_AddItemToObject(tmpObj,"Method",methodArr);
		}

		Common_cJSON_AddItemToArray(uriList,tmpObj);
		tmpCnt = tmpCnt->next;
	}
	return 0;
}

int RestComm_GetIdxFormString(const char* string,int* device,int* channel,int* stream)
{
	if(string == NULL)
		return -1;

	char uriBuff[256];
	memset(uriBuff,0,sizeof(uriBuff));
	int strLen = strlen(string);
	int i = 0;
	for(i = 0; i < strLen; i++)
	{
		uriBuff[i] = tolower(*(string + i));
	}
	char* tmp = NULL;
	if(device)
	{
		tmp = strstr(uriBuff,"/device");
		if(tmp)
			*device = atoi(tmp+strlen("/device"));
		else 
			return -1;
	}

	if(channel)
	{
		tmp = strstr(uriBuff,"/channel");
		if(tmp)
			*channel = atoi(tmp+strlen("/channel"));	
		else
			return -1;
	}

	if(stream)
	{
		tmp = strstr(uriBuff,"/stream");
		if(tmp)
			*stream = atoi(tmp+strlen("/stream"));
		else
			return -1;
	}	
	return 0;
}

const char* RestComm_GetErrString(REST_ER_T* errlist,int errorCode)
{
	int i = 0;
	while(errlist[i].errorString)
	{
		if(errlist[i].errorCode  == errorCode)
			return errlist[i].errorString;
		i++;
	}		
	return "Unknow ErrString";
}

int RestComm_Init(REST_CB_TALBE_T* cbTable)
{
	memcpy(&s_rest_cbTable,cbTable,sizeof(REST_CB_TALBE_T));
	pthread_mutex_init(&s_rest_lock,NULL);

	int ret = RestCfg_Init();
	if(ret != 0)
		return ret;
	
	ret = RestList_Init();
	if(ret != 0)
		return -1;
	
	return 0;
}

int RestComm_Destroy()
{
	RestCfg_Destroy();
	RestList_Destroy();
    	pthread_mutex_destroy(&s_rest_lock);
	return 0;
}


int RestComm_FuncCall(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T** outParam)
{
	if(s_rest_cbTable.callFunc)
		return s_rest_cbTable.callFunc(method,uriPath,condition,inParam,outParam);
	else
		LOGE("funcCall is NULL\n");
	return -1;
}

int RestComm_PutMyself(const char* uri,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{	
    if(s_rest_cbTable.put)
    {
       return  s_rest_cbTable.put(uri,condition,in,out);
    }
    else
        LOGE("put my self fail!\n");

    return -1;
}

int RestComm_GetMyself(const char* uri,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{	
    if(s_rest_cbTable.get)
    {
        return  s_rest_cbTable.get(uri,condition,in,out);
    }
    else
        LOGE("put my self fail!\n");
	
	return -1;
}

int RestComm_MargeJsonWithUri(Common_cJSON_T* saveObj,const char* uri,Common_cJSON_T* param)
{
	if(param == NULL)
		return 0;
	//LOGW("URI:%s\n param=%s \n saveobj=%s\n",uri,Common_cJSON_Print(param,NULL),Common_cJSON_Print(saveObj,NULL));
	int ret = 0;
	char* uriBk = Common_StrDup((S8*)uri,__FUNCTION__,__LINE__);

	char* srcStr = uriBk;
	char* curStr = NULL;
	char* nextStr = NULL;
	Common_cJSON_T* tmp = saveObj;
	Common_cJSON_T* tmp1 = NULL;

	while(srcStr)
	{
		ret = Common_UriOneParse(srcStr,NULL,&curStr,&nextStr);
		if(ret != 0)
		{
			LOGE("Uri parse fail! lable=%s\n",curStr);
			if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			curStr = NULL;
			ret = EC_REST_OPERATION_FAIL;
			break;
		}

		tmp1 = Common_cJSON_GetObjectItem(tmp,curStr);
		if(tmp1 == NULL)
		{
			if(nextStr)
			{
				Common_cJSON_T* tmpObj = Common_cJSON_CreateObject();
				Common_cJSON_AddItemToObject(tmp,curStr,tmpObj);
				tmp = tmpObj;
			}
			else
			{
				Common_cJSON_AddItemToObject(tmp,curStr,param);
				if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
				curStr = NULL;
				break;
			}
		}
		else
		{
			if(nextStr == NULL)
			{
				char*	name = Common_StrDup((S8*)(tmp1->string),__FUNCTION__,__LINE__);
				Common_cJSON_DeleteItemFromObject(tmp,name);
				Common_cJSON_AddItemToObject(tmp,name,param);
				Common_Free(name,__FUNCTION__,__LINE__);
				if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			curStr = NULL;
				break;
			}
			tmp = tmp1;
		}
		
		srcStr = nextStr;
		if(curStr)
			Common_Free(curStr,__FUNCTION__,__LINE__);
		curStr = NULL;
	}

	Common_Free(uriBk,__FUNCTION__,__LINE__);
	return 0;
}

ModuleHandle_T RestComm_GetModuleHdl()
{
	return *(s_rest_cbTable.moduleHdl);
}

Common_cJSON_T* RestComm_GetItemWithUri(Common_cJSON_T* obj,const char* uriString)
{
	int ret = 0;
	char* uriBk = Common_StrDup((S8*)uriString,__FUNCTION__,__LINE__);

	char* srcStr = uriBk;
	char* curStr = NULL;
	char* nextStr = NULL;
	Common_cJSON_T* tmp = obj;

	while(srcStr)
	{
		ret = Common_UriOneParse(srcStr,NULL,&curStr,&nextStr);
		if(ret != 0)
		{
			LOGE("Uri parse fail! lable=%s\n",curStr);
			if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			curStr = NULL;
			ret = EC_REST_OPERATION_FAIL;
			break;
		}

		tmp = Common_cJSON_GetObjectItem(tmp,curStr);
		if(tmp == NULL)
		{
			LOGE("Can't find:%s\n",curStr);
			if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			ret = EC_REST_URI_PATH_NOT_EXIST;
			break;
		}
		
		if(nextStr == NULL)
		{
			Common_Free(curStr,__FUNCTION__,__LINE__);
			Common_Free(uriBk,__FUNCTION__,__LINE__);
			return tmp;	
		}

		
		srcStr = nextStr;
		if(curStr)
			Common_Free(curStr,__FUNCTION__,__LINE__);
		curStr = NULL;
	}

	Common_Free(uriBk,__FUNCTION__,__LINE__);
	return NULL;
}

int RestComm_AddCfgNode(Common_cJSON_T* obj,int reduceCnt)
{
	char* uri 		= RestComm_GetUriStrFromJson(obj); //通过json对象的父节点指针，找到该json的URI
        if(uri == NULL)
            return 0;

        int ret = RestComm_AddCfgNodeWithStr(uri,reduceCnt);// 将该URI添加到链表中

        Common_Free(uri,__FUNCTION__,__LINE__);
        uri = NULL;
	return ret;
}

int RestComm_AddCfgNodeWithStr(const char* path,int reduceCnt)
{       
        Common_cJSON_T* initParam = Common_cJSON_CreateObject();
        int ret = RestComm_GetMyself(path,NULL,NULL,initParam);     //先获取保存一份参数，此时的参数是初始化是的默认参数。
        if(ret != 0)
        {
            LOGW("get %s init param is null!\n",path);
            Common_cJSON_Delete(initParam);
            initParam = NULL;
            //return -1;
        }
        
	REST_CFG_NODE_T* data = (REST_CFG_NODE_T*)Common_Malloc(sizeof(REST_CFG_NODE_T),0,__FUNCTION__,__LINE__);
	if(data == NULL)
	{
		LOGE("malloc fail. sizeo=%d\n",sizeof(REST_CFG_NODE_T));
                Common_cJSON_Delete(initParam);
		return -1;
	}
    
	memset(data,0,sizeof(REST_CFG_NODE_T));
	data->uri 		= Common_StrDup((S8*)path,__FUNCTION__,__LINE__);
        data->reduceCnt    = reduceCnt;
        data->initParam     = initParam;
	
        return RestCfg_AddCfgNode(data);        //添加到链表
}

int RestComm_StartCfgChangedTimer()
{
	return RestCfg_DelaySaveCfg();
}

Common_cJSON_T* RestComm_AddPathToTree(const char* uri,Common_cJSON_T* tree,const char* desc,const char* label,REST_FUNC_F get,REST_FUNC_F put,REST_FUNC_F post,REST_FUNC_F delet)
{
	REST_NODE_ATTR_T attr;
	attr.describtion = desc;
	attr.label		 = label;
	attr.put		 = put;
	attr.get		 = get;
	attr.post		 = post;
	attr.delet      = delet;

	int ret = 0;
	char* uriBk = Common_StrDup((S8*)uri,__FUNCTION__,__LINE__);

	char* srcStr = uriBk;
	char* curStr = NULL;
	char* nextStr = NULL;
	Common_cJSON_T* tmp = tree;
	Common_cJSON_T* tmp1 = NULL;

	Common_cJSON_T* retObj = NULL;

	while(srcStr)
	{
		ret = Common_UriOneParse(srcStr,NULL,&curStr,&nextStr);
		if(ret != 0)
		{
			LOGE("Uri parse fail! lable=%s\n",curStr);
			if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			curStr = NULL;
			ret = EC_REST_OPERATION_FAIL;
			break;
		}

		tmp1 = Common_cJSON_GetObjectItem(tmp,curStr);
		if(tmp1 == NULL)
		{
			if(nextStr)
			{
				Common_cJSON_T* tmpObj = Common_cJSON_CreateObject();
				Common_cJSON_AddItemToObject(tmp,curStr,tmpObj);
				tmp = tmpObj;
			}
			else
			{
				tmp->type = Common_cJSON_Object;
				Common_cJSON_T* tmpObj = Common_cJSON_CreateNumber(0);
				Common_cJSON_AddItemToObject(tmp,curStr,tmpObj);
				Common_cJSON_SetItemExtData(tmpObj,&attr,sizeof(attr));
				retObj = tmpObj;
				if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			curStr = NULL;
				break;
			}
		}
		else
		{
			if(nextStr == NULL)
			{
				Common_cJSON_SetItemExtData(tmp1,&attr,sizeof(attr));
				retObj = tmp1;
				if(curStr)
				Common_Free(curStr,__FUNCTION__,__LINE__);
			curStr = NULL;
				break;
			}
			tmp = tmp1;
		}
		
		srcStr = nextStr;
		if(curStr)
			Common_Free(curStr,__FUNCTION__,__LINE__);
		curStr = NULL;
	}

	if(curStr)
		Common_Free(curStr,__FUNCTION__,__LINE__);

	Common_Free(uriBk,__FUNCTION__,__LINE__);

	return retObj;
}

int RestComm_LowerStr(const char* inString,char* outString,int size)
{
	if(inString == NULL || outString == NULL || size <= 0)
	{
		LOGE("invail param!\n");
		return -1;
	}
	
	int inLen = strlen(inString);
	int i = 0;
	for(i = 0; i < inLen && i < size; i++)
	{
		outString[i] = tolower(inString[ i]);
	}
	return 0;
}

int RestComm_SaveCfgFilesImmediately()
{
	return RestCfg_SaveCfgFile();
}

int RestComm_RegisterRestore(Common_cJSON_T* obj,const char* label,int needRestart)
{ 
    char* uri = RestComm_GetUriStrFromJson(obj); //通过json对象的父节点指针，找到该json的URI
       
    Common_cJSON_T* inParam = Common_cJSON_CreateObject();
    Common_cJSON_T* restListArr = Common_cJSON_CreateArray();
    Common_cJSON_AddItemToObject(inParam,"ResList",restListArr);

    Common_cJSON_T* tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToArray(restListArr,tmp);

    Common_cJSON_AddStringToObject(tmp,"Uri", uri);
    Common_cJSON_AddStringToObject(tmp,"Label",label);
    Common_cJSON_AddNumberToObject(tmp,"NeedReboot",needRestart);
    
    int ret = RestComm_FuncCall("put","/Core/Restore/Update",NULL, inParam, NULL);    //向core注册，该逻辑已没有用，通过删除配置文件实现
    if(uri)
        Common_Free(uri,__FUNCTION__,__LINE__);

    Common_cJSON_Delete(inParam);
    return ret;
}

int RestComm_RestoreCfg(const char* uri,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
{
    REST_CFG_NODE_T* matchItem = RestCfg_MatchNode(uri);
    if(matchItem == NULL)
        return 0;

    Common_cJSON_T* restoreParam = Common_cJSON_Duplicate(matchItem->initParam,1);

    Common_cJSON_T* defCfg = NULL;
    Module_LoadConfigByType(RestComm_GetModuleHdl(), Module_ConfigType_Default,(cJSON_Struct**)&defCfg);

    if(defCfg)
    {
        JsonOper_MergeObj(restoreParam, RestComm_GetItemWithUri(defCfg,matchItem->uri),2);
        Common_cJSON_Delete(defCfg);
        defCfg = NULL;
    }

    int ret = RestComm_PutMyself(matchItem->uri,NULL,restoreParam, NULL);

    char* outP = NULL;
    LOGI("URI=%s RestoreItem=%s\n",matchItem->uri,Common_cJSON_Print(restoreParam,NULL));
    if(outP)
        Common_Free(outP,__FUNCTION__,__LINE__);

    //RestCfg_SaveCfgFile();
    return ret;
}


int RestComm_CallRestFunc(const char* uriString,const char* uriContion,Common_cJSON_T* jsonObj,Common_cJSON_T* in,Common_cJSON_T* out,int type) //get-0 put-1 post-2 delet-3
{
	int ret = 0;
	REST_NODE_ATTR_T* info = (REST_NODE_ATTR_T*)jsonObj->pExtData;
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
			ret = EC_REST_METHOD_NOT_FOUND;
	}
	else if(type == 1)
	{
		if(info->put)
		{
			ret = info->put(uriString,uriContion,jsonObj,in,out);
		}
		else 
			ret = EC_REST_METHOD_NOT_FOUND;
	}
	else if(type == 2)
	{
		if(info->post)
			ret = info->post(uriString,uriContion,jsonObj,in,out);
		else 
			ret = EC_REST_METHOD_NOT_FOUND;
	}
	else if(type == 3)
	{
		if(info->delet)
			ret = info->delet(uriString,uriContion,jsonObj,in,out);
		else
			ret = EC_REST_METHOD_NOT_FOUND;
	}
	
	return ret;
}

int RestComm_ParseUriString(Common_cJSON_T* rootTree,const char* uriString,const char* uriCondition,Common_cJSON_T* in,Common_cJSON_T* out,int type) //get-0 put-1 post-2 delet-3
{
	int ret = 0;
	char* uriBk = Common_StrDup((S8*)uriString,__FUNCTION__,__LINE__);

	char* srcStr = uriBk;
	char* curStr = NULL;
	char* nextStr = NULL;
	Common_cJSON_T* tmp = rootTree;
	if(tmp == NULL)
	{
		LOGE("path root is NULL!!!\n");
		return -1;
	}

	while(srcStr)
	{
		ret = Common_UriOneParse(srcStr,NULL,&curStr,&nextStr);
		if(ret != 0)
		{
			LOGE("Uri parse fail! lable=%s\n",curStr);
			ret = EC_REST_OPERATION_FAIL;
			break;
		}

		tmp = Common_cJSON_GetObjectItem(tmp,curStr);
		if(tmp == NULL)
		{
			LOGE("Can't find:%s\n",curStr);
			ret = EC_REST_URI_PATH_NOT_EXIST;
			break;
		}
		
		if(nextStr == NULL)
		{
			ret  =  RestComm_CallRestFunc(uriString,uriCondition,tmp,in,out,type);
			break;
		}
		
		srcStr = nextStr;
		if(curStr)
			Common_Free(curStr,__FUNCTION__,__LINE__);
		curStr = NULL;
	}

	if(uriBk)
		Common_Free(uriBk,__FUNCTION__,__LINE__);
	if(curStr)
		Common_Free(curStr,__FUNCTION__,__LINE__);

	return ret;
}


int RestComm_Get(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	pthread_mutex_lock(&s_rest_lock);
        
	char* outStr = NULL;
	//LOGI("uriString:%s	condition:%s in:%s\n",uriString,condition,outStr = Common_cJSON_PrintUnformatted(in,NULL));
	if(outStr)
		Common_Free(outStr,__FUNCTION__,__LINE__);
	
	int ret = RestComm_ParseUriString(rootTree,uriString,condition,in,out,0);

	pthread_mutex_unlock(&s_rest_lock);
	return ret;
}

int RestComm_Put(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	pthread_mutex_lock(&s_rest_lock);
        
	char* outStr = NULL;
	/* LOGI("uriString:%s condition:%s in:%s\n",uriString,condition,outStr =Common_cJSON_PrintUnformatted(in,NULL)); */
	/* if(outStr) */
	/* 	Common_Free(outStr,__FUNCTION__,__LINE__); */
	
	int ret = RestComm_ParseUriString(rootTree,uriString,condition,in,out,1);	
	if(0 == ret)
	{
	    //临时设置H264,不需要保存配置.
	    if ((NULL != condition) 
             && ((0 == strcmp(condition, "ChangeH264=1")) 
                 || (0 == strcmp(condition, "ChangeH264=0"))
				 || (0 == strcmp(condition, "CommCfgForceNoSaveCfg=1"))))
        {
            pthread_mutex_unlock(&s_rest_lock);
            LOGI("condition:%s DO NOT SAVE cfg file.\n", condition);
        	return ret;
        }

		REST_CFG_NODE_T* cfgHasMatched = RestCfg_MatchNode(uriString);      // 看是否是配置项相关URI
		if(cfgHasMatched)
			RestComm_StartCfgChangedTimer();        // 如果是的则开始1秒定时器，1秒后保存配置文件
	}

	pthread_mutex_unlock(&s_rest_lock);
	return ret;
}

int RestComm_Post(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	pthread_mutex_lock(&s_rest_lock);
	char* outStr = NULL;
	//LOGI("uriString:%s	condition:%s in:%s\n out%s\n",uriString,condition,outStr = Common_cJSON_PrintUnformatted(in,NULL));
	if(outStr)
		Common_Free(outStr,__FUNCTION__,__LINE__);

	int ret = RestComm_ParseUriString(rootTree,uriString,condition,in,out,2);
	pthread_mutex_unlock(&s_rest_lock);
	return ret;
}

int RestComm_Delete(Common_cJSON_T* rootTree,const char* uriString,const char* condition,Common_cJSON_T* in,Common_cJSON_T* out)
{
	pthread_mutex_lock(&s_rest_lock);
	
	char* outStr = NULL;
	//LOGI("uriString:%s	condition:%s in:%s\n out%s\n",uriString,condition,outStr = Common_cJSON_PrintUnformatted(in,NULL));
	if(outStr)
		Common_Free(outStr,__FUNCTION__,__LINE__);
	
	int ret = RestComm_ParseUriString(rootTree,uriString,condition,in,out,3);
	pthread_mutex_unlock(&s_rest_lock);
	return ret;
}

char* RestComm_GetResoStr(int w, int h)
{
    int i = 0;
   
    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_rest_resoStr);i++)
    {
        if(s_rest_resoStr[i].resoStr == NULL)
            break;

        if((s_rest_resoStr[i].w == w) && (s_rest_resoStr[i].h == h))
            return Common_StrDup(s_rest_resoStr[i].resoStr, __FUNCTION__, __LINE__);
    }

    char resoBuff[32];
    memset(resoBuff,0,sizeof(resoBuff));
    snprintf(resoBuff,sizeof(resoBuff),"%d*%d",w,h);
    
    return Common_StrDup(resoBuff, __FUNCTION__, __LINE__);
}

