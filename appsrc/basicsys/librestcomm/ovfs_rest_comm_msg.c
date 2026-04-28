#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"libcommon_api.h"
//#include"ovfs_rest_common.h"
#include"ovfs_rest_comm_msg.h"

int RestComm_PraseInputJson(Common_cJSON_T* inputData,char** method,char** uri,Common_cJSON_T** inData)
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

    #if 0
    tmp = Common_cJSON_GetObjectItem(header,"From");
    if(tmp)
    {
        Common_cJSON_T* fromUri = Common_cJSON_GetObjectItem(tmp,"Uri");
        if(fromUri)
            LOGW("fromUri=%s\n",fromUri->valuestring);
    }
    #endif
    return 0;
}

int RestComm_GetUriAndQue(const char* srcUri,char** uriStr, char** conditionStr)
{
    char* tmp = strstr(srcUri,"?");
    if(tmp)
    {
        char buff[256];
        memset(buff,0,sizeof(buff));
        memcpy(buff,srcUri,(int)(tmp-srcUri));
        *uriStr         = Common_StrDup((S8*)buff,__FUNCTION__,__LINE__);
        *conditionStr   = Common_StrDup((S8*)(tmp+1),__FUNCTION__,__LINE__);
    }
    else
    {
        *uriStr         = Common_StrDup((S8*)srcUri,__FUNCTION__,__LINE__);
        *conditionStr   = NULL;
    }
    return 0;
}

Common_cJSON_T* RestComm_GenerateOutParam(int retCode,Common_cJSON_T* outData,const char* errStr)
{
    Common_cJSON_T* root = Common_cJSON_CreateObject();
    Common_cJSON_T* header = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(root,"Header",header);
    
    Common_cJSON_AddNumberToObject(header,"Code",retCode);
    if(retCode != 0)
        Common_cJSON_AddStringToObject(header,"Decribe",errStr);

    if(outData)
        Common_cJSON_AddItemToObject(root,"Data",outData);
    
    return root;
}

Common_cJSON_T* RestComm_GenerateInParam(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inData)
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
    Common_cJSON_AddItemToObject(root,"Data",Common_cJSON_Duplicate(inData,1));
    return root;
}

                                                  
