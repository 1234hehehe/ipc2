#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"ovfs_rest_comm_sub.h"

int RestComm_SetSubId(REST_SUB_INFO_T* subInfo, int offset,int id)
{
    int ret = -1;
    int i = 0;
    
    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(subInfo[offset].id);i++)
    {
        if(subInfo[offset].id[i] <= 0)
        {
            subInfo[offset].id[i] = id;
            ret =  0;
            break;
        }
    }
    return ret;
}

int RestComm_UnSetSubId(REST_SUB_INFO_T* subInfo,int offset,int id)
{
    int ret = -1;
    int i = 0;
    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(subInfo[offset].id);i++)
    {
        if(subInfo[offset].id[i] == id)
        {
            subInfo[offset].id[i] = 0;
            ret =  0;
            break;
        }
    }
    return ret;
}

int RestComm_CheckSubId(REST_SUB_INFO_T* subInfo,int offset,int id)
{
    int ret = -1;
    int i = 0;
    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(subInfo[offset].id);i++)
    {
        if(subInfo[offset].id[i] == id)
        {
            ret =  id;
            break;
        }
    }
    return ret;
}

int RestComm_FindMatchEventSubUri(REST_SUB_INFO_T* subInfo,int offset,const char* uri)
{
    if( subInfo[offset].subUri && uri && (strcmp(subInfo[offset].subUri,uri) == 0))
    {
        return 0;
    }
    return -1;
}

int RestComm_SendSubInfo(ModuleHandle_T* mdHdl,REST_SUB_INFO_T* subInfo,int offset,Common_cJSON_T* info)
{
    int i = 0;
    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(subInfo[offset].id);i++)
    {
        if(subInfo[offset].id[i] <= 0)
            continue;

        Module_SendEvent(mdHdl,subInfo[offset].id[i],(cJSON_Struct*)info,NULL,2000);
    }
    return 0;
}

