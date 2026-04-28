#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ovfs_network_rest_common.h"

static NETWORK_REST_ER_T s_network_rest_errorInfo[] =
{
    {EC_NETWORK_REST_METHOD_NOT_FOUND,EC_NETWORK_REST_METHOD_NOT_FOUND_STR},
    {EC_NETWORK_REST_URI_PATH_NOT_EXIST,EC_NETWORK_REST_URI_PATH_NOT_EXIST_STR},
    {EC_NETWORK_REST_OPERATION_FAIL,EC_NETWORK_REST_OPERATION_FAIL_STR},
};

static NETWORK_REST_CB_TALBE_T s_network_rest_cbTable;
static Common_Timer_T		   s_network_rest_cfgTimer = NULL;

char* NetWork_RestComm_GetUriStrFromJson(const Common_cJSON_T* jsonObj)
{
    const Common_cJSON_T* tmp = jsonObj;
    char strBuff[1024*32];
    memset(strBuff,0,sizeof(strBuff));

    int usedLen = 1;
    int buffSize = sizeof(strBuff);

    while(tmp && tmp->string)
    {
        int strLen = strlen(tmp->string);
        if((buffSize-usedLen-strLen-1) < 0)
        {
            LOGE("Buffer is too small.\n");
            return NULL;
        }
        memcpy(strBuff+(buffSize-usedLen-strLen),tmp->string,strLen);
        strBuff[buffSize-usedLen-strLen-1] = '/';
        usedLen = usedLen + strLen + 1;

        tmp = tmp->pParent;
    }

    if(usedLen > 1)
    {
        return Common_StrDup(&(strBuff[sizeof(strBuff)-usedLen]),__FUNCTION__,__LINE__);
    }

    return NULL;
}

int NetWork_RestComm_GetUriList(const char* uri,const char* condition,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out)
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

        char* uriString = NetWork_RestComm_GetUriStrFromJson(tmpCnt);
        Common_cJSON_AddStringToObject(tmpObj,"Uri",uriString);
        Common_Free(uriString,__FUNCTION__,__LINE__);

        NETWORK_REST_NODE_ATTR_T* info = (NETWORK_REST_NODE_ATTR_T*)tmpCnt->pExtData;
        if(info)
        {
            Common_cJSON_AddStringToObject(tmpObj,"Label",info->label);
            Common_cJSON_AddStringToObject(tmpObj,"Describe",info->describtion);
            Common_cJSON_T* methodArr = Common_cJSON_CreateArray();
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

            if(info->fdelete)
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

int NetWork_RestComm_GetIdxFormString(const char* string,int* eth)
{
    char* tmp = NULL;

    if(string == NULL)
    {
        return -1;
    }

    if(eth)
    {
        tmp = (char*)strstr(string,ETH_IDX_NAME);
        if(tmp)
        {
            *eth = atoi(tmp+strlen(ETH_IDX_NAME));
        }
    }

    return 0;
}

const char* NetWork_RestComm_GetErrString(int errorCode)
{
    unsigned int i = 0;

    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_network_rest_errorInfo); i++)
    {
        if(s_network_rest_errorInfo[i].errorCode == errorCode)
        {
            return s_network_rest_errorInfo[i].errorString;
        }
    }

    return NULL;
}

int NetWork_RestComm_Init(NETWORK_REST_CB_TALBE_T* cbTable)
{
    memcpy(&s_network_rest_cbTable,cbTable,sizeof(NETWORK_REST_CB_TALBE_T));

    return 0;
}

int NetWork_RestComm_Destroy()
{
    return 0;
}

int NetWork_RestComm_FuncCall(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inParam,Common_cJSON_T* outParam)
{
    if(s_network_rest_cbTable.callFunc)
    {
        return s_network_rest_cbTable.callFunc(method,uriPath,condition,inParam,outParam);
    }
    else
    {
        LOGE("callFunc is NULL\n");
    }

    return -1;
}

int NetWork_RestComm_MargeJsonWithUri(Common_cJSON_T* saveObj,const char* uri,Common_cJSON_T* param)
{
    if(param == NULL)
        return 0;
    //LOGW("URI:%s\n param=%s \n saveobj=%s\n",uri,Common_cJSON_Print(param,NULL),Common_cJSON_Print(saveObj,NULL));
    int ret = 0;
    char* uriBk = Common_StrDup((S8 *)uri,__FUNCTION__,__LINE__);

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
            ret = EC_NETWORK_REST_OPERATION_FAIL;
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
                char* name = Common_StrDup(tmp1->string,__FUNCTION__,__LINE__);
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

ModuleHandle_T NetWork_RestComm_GetModuleHdl()
{
    return *(s_network_rest_cbTable.moduleHdl);
}

Common_cJSON_T* NetWork_RestComm_GetItemWithUri(Common_cJSON_T* obj,const char* uriString)
{
    int ret = 0;
    char* uriBk = Common_StrDup((S8 *)uriString,__FUNCTION__,__LINE__);

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

static int NetWork_AddSaveCfgFile(Common_Timer_T hTimer,void* pUserData)
{
    Common_Timer_Destroy(&s_network_rest_cfgTimer);
    //return RestCfg_SaveCfgFile();
    return 0;
}

int NetWork_RestComm_StartCfgChangedTimer()
{
    if(s_network_rest_cfgTimer)
    {
        Common_Timer_Destroy(&s_network_rest_cfgTimer);
    }

    int ret = Common_Timer_Create(&s_network_rest_cfgTimer,3*1000,NetWork_AddSaveCfgFile,NULL);
    if(ret != 0)
    {
        LOGE("create timer fail!\n");
        return -1;
    }

    return 0;
}

Common_cJSON_T* NetWork_RestComm_AddPathToTree(const char* uri,Common_cJSON_T* tree,const char* desc,const char* label,NETWORK_REST_FUNC_F get,NETWORK_REST_FUNC_F put,NETWORK_REST_FUNC_F post,NETWORK_REST_FUNC_F fdelete)
{
    NETWORK_REST_NODE_ATTR_T attr;
    attr.describtion = desc;
    attr.label		 = label;
    attr.put		 = put;
    attr.get		 = get;
    attr.post		 = post;
    attr.fdelete      = fdelete;

    int ret = 0;
    char* uriBk = Common_StrDup((S8 *)uri,__FUNCTION__,__LINE__);

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
            ret = EC_NETWORK_REST_OPERATION_FAIL;
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

    return retObj;
}


S32 NetWork_RestComm_LoadConfig(char *szPath,cJSON_Struct **pConfig)
{
    char *pConfigString = NULL;
    U32 nStrLen = 0;
    FILE *fp;

    if (szPath == NULL || pConfig == NULL || *pConfig != NULL)
    {
        return -1;
    }

    fp = fopen(szPath,"rb");
    if (fp != NULL)
    {
        fseek(fp,0,SEEK_END);
        nStrLen = ftell(fp);
        fseek(fp,0,SEEK_SET);
        if (nStrLen > 0)
        {
            pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
            if (pConfigString != NULL)
            {
                if(nStrLen == (U32)fread(pConfigString,1,nStrLen,fp))
                {
                    *pConfig = Common_Json_Parse(pConfigString,NULL,NULL);
                }
            }

        }
        fclose(fp);
    }
    if (pConfigString != NULL)
    {
        Common_Free(pConfigString,__FUNCTION__,__LINE__);
        pConfigString = NULL;
    }
    if (*pConfig == NULL)
    {
        return -1;
    }
    return 0;
}

S32 NetWork_RestComm_SaveConfig(char *szPath,cJSON_Struct *pConfig)
{
    char *pConfigString = NULL;
    S32 nStrLen = 0;
    FILE *fp;
    S32 nRet = 0;

    if (szPath == NULL || pConfig == NULL)
    {
    	LOGE("szPath=%d,pConfig=%d\n",szPath,pConfig);
        return -1;
    }
	
    fp = fopen(szPath,"wb+");
    if (fp != NULL)
    {
        pConfigString = Common_Json_Print(pConfig,&nStrLen);
        if (pConfigString && nStrLen > 0)
        {
            if((nStrLen + 1) == (S32)fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
            {
                nRet = 0;
            }
        }
        fclose(fp);
    }
    if (pConfigString != NULL)
    {
        Common_Free(pConfigString,__FUNCTION__,__LINE__);
        pConfigString = NULL;
    }
    return nRet;
}
