#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include"ovfs_rest_comm_cfg.h"
#include"ovfs_rest_common.h"
#include"common_json_str_ops.h"

static COMMON_DLIST_T s_rest_cfgNodeHdl;
static REST_CFG_SAVE_INFO_T    s_cfg_save;
static pthread_mutex_t                s_cfg_lock;
static int                                        s_cfg_fileLock;

static void NodeFree(void* data)
{
	REST_CFG_NODE_T* node = (REST_CFG_NODE_T*)data;

        if(node->uri)
        {
            Common_Free(node->uri,__FUNCTION__,__LINE__);
            node->uri = NULL;
        }

        if(node->initParam)
        {
            Common_cJSON_Delete(node->initParam);
            node->initParam = NULL;
        }
	Common_Free(data,__FUNCTION__,__LINE__);
        data = NULL;
	return;
}

static int GetSizeFormRecuce(const char* uri,int reduceCnt)
{
    int len = strlen(uri);
    if(reduceCnt == 0)
        return len;
    
    int i = len -1;

    int cnt = 0;
    for(i = len -1; i > 0; i--)
    {
        if((*(uri+i)=='/') && (i != (len -1)))
            cnt++;

        if(cnt >= reduceCnt)
            break;
    }

    return i;
}

static int MatchUriNode(void* a, void* b)
{
	REST_CFG_NODE_T* node = (REST_CFG_NODE_T*)a;
	char* inUriStr 		  = (char*)b;
        char* cfgUriStr           = node->uri;

        int inUriLen = strlen(inUriStr);
        int cfgUriLen = strlen(cfgUriStr);

        char* inBuff = Common_Malloc(inUriLen+1,0,__FILE__,__LINE__);
        char* cfgBuff = Common_Malloc(cfgUriLen+1,0,__FILE__,__LINE__);
        memset(inBuff,0,inUriLen+1);
        memset(cfgBuff,0,cfgUriLen+1);

        int reductCnt = node->reduceCnt;
        if(node->reduceCnt < 0)
            reductCnt = 0;
            
        RestComm_LowerStr(inUriStr, inBuff,inUriLen);
        RestComm_LowerStr(cfgUriStr, cfgBuff,GetSizeFormRecuce(cfgUriStr,reductCnt));

        int ret = -1;
        int cfgBuffLen = strlen(cfgBuff);
	if((inUriLen >= cfgBuffLen) && (memcmp(inBuff,cfgBuff,cfgBuffLen)==0))
            ret = 0;

        Common_Free(inBuff, __FILE__, __LINE__);
        Common_Free(cfgBuff, __FILE__, __LINE__);

	return ret;
}

int RestCfg_AddCfgNode(REST_CFG_NODE_T* cfgNode)
{
	if(cfgNode == NULL)
		return -1;
	
	LOGW("Add cfg node uri:%s\n",cfgNode->uri);

	return Common_DList_InsertTail(s_rest_cfgNodeHdl,(void*)cfgNode,sizeof(REST_CFG_NODE_T));
}

REST_CFG_NODE_T* RestCfg_MatchNode(const char* uriString)
{
	char* uri = Common_StrDup((S8*)uriString,__FUNCTION__,__LINE__);
	REST_CFG_NODE_T* matched= Common_DList_Search(s_rest_cfgNodeHdl,(void*)uri,MatchUriNode);
	Common_Free(uri,__FUNCTION__,__LINE__);
	return matched;
}

int RestCfg_SaveCfgFile(void)
{
        if(s_cfg_fileLock)
        {
            LOGW("cfg file is locked.\n");
            return 0;
        }
        
	LOGI("begin save file!\n");
	int nodeCnt = Common_DList_GetCount(s_rest_cfgNodeHdl);
	if(nodeCnt < 0)
	{
		LOGE("get count fail!\n");
		return -1;
	}
        pthread_mutex_lock(&s_cfg_lock);

	Common_cJSON_T* saveObj = Common_cJSON_CreateObject();
	int i = 0;
	for(i = 0; i < nodeCnt;i++)
	{
		REST_CFG_NODE_T* nodeTmp = (REST_CFG_NODE_T*)Common_DList_GetNode(s_rest_cfgNodeHdl,i); // 开始从链表中获取URI
		if(nodeTmp == NULL)
		{
                        LOGE("get dlist fail!\n");
                        pthread_mutex_unlock(&s_cfg_lock);
                        return -1;
		}

                   if(nodeTmp->reduceCnt < 0)
                        continue;
                    
		Common_cJSON_T* tmpOut = Common_cJSON_CreateObject();
		RestComm_GetMyself(nodeTmp->uri,NULL,NULL,tmpOut);  // 从该URI中get，获取参数
		
		if(tmpOut->child == NULL)
		{
			Common_cJSON_Delete(tmpOut);
			tmpOut = NULL;
		}
		RestComm_MargeJsonWithUri(saveObj,nodeTmp->uri,tmpOut); //合并到一个大json中
	}
	
        if( saveObj->child && (s_cfg_fileLock == 0) )
        {
            int ret = Module_SaveConfig((RestComm_GetModuleHdl()),(cJSON_Struct*)saveObj); //保存配置文件
            if(ret != 0)
            {
                LOGE("!!!!!!!!!!! save cfg fail ! ret=%d\n",ret);
            }
        }
	
	Common_cJSON_Delete(saveObj);
	
	LOGI("save file ok!\n");
        pthread_mutex_unlock(&s_cfg_lock);
	return 0;
}


Common_cJSON_T* GetDefCfgItem(REST_CFG_NODE_T* node)
{
    Common_cJSON_T* restoreParam = Common_cJSON_Duplicate(node->initParam,1);       //拿到初始化参数

    Common_cJSON_T* defCfg = NULL;
    Module_LoadConfigByType(RestComm_GetModuleHdl(), Module_ConfigType_Default,(cJSON_Struct**)&defCfg); //拿到用户定制参数
    if(defCfg)
    {
        Common_cJSON_T* getParam = RestComm_GetItemWithUri(defCfg,node->uri);
        if(getParam)
        {
            Common_cJSON_AddNumberToObject(getParam,"isCheck",0);		// add check flag
            int ret = RestComm_PutMyself(node->uri,NULL,getParam,NULL);            //检查定制参数
            Common_cJSON_DeleteItemFromObject(getParam,"isCheck");	       // remove check flag                        
            if(ret == 0)
            {
                LOGW("def param check pass! merge params! uri=%s\n",node->uri);
                JsonOper_MergeObj(restoreParam, RestComm_GetItemWithUri(defCfg,node->uri),2);       //如果用户参数存在，则合并两个参数
            }
            else
            {
                LOGW("=== CHECK def param fail!  using init params! uri=%s\n",node->uri);
            }        
        }
        else
        {
            LOGW("Have not def param. using init params!:%s\n",node->uri);
        }

        Common_cJSON_Delete(defCfg);
        defCfg = NULL;   
    }
    
    char* outP = NULL;
    LOGI("DefCfgItem=%s\n",outP = Common_cJSON_Print(restoreParam,NULL));
    if(outP)
        Common_Free(outP,__FUNCTION__,__LINE__);
    
    return restoreParam;
}

static int CheckAndSetCfg(REST_CFG_NODE_T* nodeTmp,Common_cJSON_T* cfgParam,int type) //0: check 1: focus Set Def 2: focus put param
{
    int needUpdate = 0;
    int ret = 0;

    int useDef = 0;
    int putParam = 0;

    if(type == 0)
    {
        Common_cJSON_AddNumberToObject(cfgParam,"isCheck",0);		// add check flag
        ret = RestComm_PutMyself(nodeTmp->uri,NULL,cfgParam,NULL);      //检查参数
        Common_cJSON_DeleteItemFromObject(cfgParam,"isCheck");		// remove check flag
        if(ret != 0)  //检查失败
        {
            LOGW("Check params fail. Will use def uri=%s\n",nodeTmp->uri);
            useDef     = 1;
            putParam = 0;
        }
        else            //检查通过
        {   
            useDef     = 0;
            putParam = 1;
        }
    }
    else if(type == 1)
    {
            useDef     = 1;
            putParam = 0;        
    }
    else if(type == 2)
    {
            useDef = 0;
            putParam = 1;       
    }
    
    if(useDef)   
    {
            Common_cJSON_T* defParm = GetDefCfgItem(nodeTmp);       //生成默认配置参数
            if(defParm)
            {
                ret = RestComm_PutMyself(nodeTmp->uri,NULL,defParm,NULL); //生效默认配置参数
                if(ret != 0)     
                    LOGE("put cfg fail! uri=%s\n",nodeTmp->uri);
                
                Common_cJSON_Delete(defParm);
                needUpdate = 1;
            }
    }     

    if(putParam)        
    {
        ret = RestComm_PutMyself(nodeTmp->uri,NULL,cfgParam,NULL);//put该uri生效配置
        if(ret != 0)     
            LOGE("put cfg fail! uri=%s\n",nodeTmp->uri);
    }      
 
    return needUpdate;
}

static int CfgLoadCheckAndSet(Common_cJSON_T* cfgObj)
{
        int needUpdate = 0;
        int updateTmp = 0;
	int nodeCnt = Common_DList_GetCount(s_rest_cfgNodeHdl);
	if(nodeCnt < 0)
	{
		LOGE("get cfg node count fail!\n");
		return -1;
	}
	
	Common_cJSON_T* cfgParam = NULL;
	int i = 0;
	//int ret;
	for(i = 0; i < nodeCnt;i++)
	{
		REST_CFG_NODE_T* nodeTmp = (REST_CFG_NODE_T*)Common_DList_GetNode(s_rest_cfgNodeHdl,i); //从配置项URI列表中，获取配置项URI
		if(nodeTmp == NULL)
		{
			LOGE("get dlist fail!\n");
			return -1;
		}

		if(nodeTmp->reduceCnt < 0) // don't need to check this uri
		    continue;

                if(cfgObj)
                {
                	cfgParam = RestComm_GetItemWithUri(cfgObj,nodeTmp->uri);        //从配置json对象中获取URI下的参数。
                	if(cfgParam)
                	{       
                            updateTmp = CheckAndSetCfg(nodeTmp, cfgParam, 0);
                	}
                	else        //参数不存在
                	{  LOGW("!!!!!!!!!!!! param not existence.!\n");
                            updateTmp = CheckAndSetCfg(nodeTmp, NULL,1);
                        }
                }
                else        //文件不存在
                {
                    LOGW("!!!!!!!!!!!! file not existence.!\n");
                    updateTmp = CheckAndSetCfg(nodeTmp, NULL,1);
                }

                if(updateTmp)
                    needUpdate = 1;      
                
	}	
	return needUpdate;
}


int RestCfg_LoadCfg()
{
	Common_cJSON_T* pConfig = NULL;
	int ret = Module_LoadConfig(RestComm_GetModuleHdl(),(cJSON_Struct**)&pConfig); //读取配置项，获取配置项json对象
	if(ret != 0)
            LOGW("load cfg fail . when generate def cfg.\n");

	ret = CfgLoadCheckAndSet(pConfig);      //配置项的检查和生效
	if(ret < 0)
	{
		Common_cJSON_Delete(pConfig);
		LOGW("cfg check fail! genrate def cfg file.\n");		
		return ret;
	}
	Common_cJSON_Delete(pConfig);

        if(ret > 0)
            RestCfg_SaveCfgFile();
        
	return (ret>=0)?0:-1;
}

int RestCfg_DelaySaveCfg()
{
    pthread_mutex_lock(&s_cfg_save.lock);
    s_cfg_save.cnt = 5; // 5*200ms = 1s
    pthread_mutex_unlock(&s_cfg_save.lock);
    return 0;
}

static int CfgFileSaveCntThread(Common_Thread_T hThreadHandle,void *pUserData)
{
    while(s_cfg_save.exit == 0)
    {
        Common_Sleep(0,200000);
        pthread_mutex_lock(&s_cfg_save.lock);

        if(s_cfg_save.cnt < 0)
        {
            pthread_mutex_unlock(&s_cfg_save.lock);
            continue;
        }
        else if(s_cfg_save.cnt == 0)
        {
            RestCfg_SaveCfgFile();
        }

         s_cfg_save.cnt--;
         pthread_mutex_unlock(&s_cfg_save.lock);
    }
    return 0;
}


int RestCfg_Init()
{
    int ret = Common_DList_Init(&s_rest_cfgNodeHdl,NodeFree);
    if(ret != 0)
    {
    	LOGE("dlist init fail!\n");
    	return -1;
    }

    pthread_mutex_init(&s_cfg_lock,NULL);
    
    memset(&s_cfg_save,0,sizeof(s_cfg_save));

    s_cfg_fileLock = 0;
    s_cfg_save.exit = 0;
    s_cfg_save.cnt  = -1;
    pthread_mutex_init(&s_cfg_save.lock,NULL);
    Common_Thread_Create(&s_cfg_save.threadHdl, "CfgFileSaveCntThread", 0, 0, CfgFileSaveCntThread, NULL);

    return 0;
}
	
int RestCfg_Destroy()
{
    s_cfg_save.exit = 1;
    Common_Thread_Destroy(&s_cfg_save.threadHdl);
    pthread_mutex_destroy(&s_cfg_save.lock);
    
    Common_DList_Uninit(&s_rest_cfgNodeHdl);
    return 0;
}

int RestCfg_Lock(void)
{
    LOGI("lock file.\n");
    s_cfg_fileLock = 1;
    return 0;
}

int RestCfg_UnLock(void)
{
    s_cfg_fileLock = 0;
     LOGI("unlock file.\n");
    return 0;
}
