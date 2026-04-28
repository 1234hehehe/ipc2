#include <stdlib.h>
#include <string.h>
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"
#include "core_export.h"
/*
Uri:/Core/ExportCfg
Method:Get
InData:
{
FileName:; // 导出文件名,包括路径；由调用者指定且管理删除。
}

Uri:/Core/ImportCfg
Method:Put
inData:
{
FileName: // 导入文件名,包括路径；由调用者指定且管理删除。
}
*/

#define CONFIG_ABSOLUTE_PATH "/usr/etc/cfgfiles"
#define CONFIG_LOCKCFG_ABSOLUTE_PATH "/usr/etc/default"
#define TMP_EXPORT_CONFIG_FILE_ABSOLUTE_PATH "/tmp/exprot_config.tar.gz"
#define TMP_IMPORT_CONFIG_FILE_ABSOLUTE_PATH "/usr/etc/import_confignew.tar.gz"
#define LOGO_PATH "/usr/etc/logo.png"

static S32 g_bInit = 0;

typedef struct _tagExport_CfgHead
{
    S32 nDataLen;
} Export_CfgHead;

static S32 Core_Export_ExportCfg(const char *uriString, const char *condition,
                                 Common_cJSON_T *in, Common_cJSON_T *out)
{
    int export_type = 0;
    cJSON_Struct *parentItem = NULL;
    cJSON_Struct *outItem = NULL;
    char *pStringValue;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;
    outItem = (cJSON_Struct *)out;

    Common_Json_GetAttrValueInt(parentItem, "ExportType", &export_type);

    pStringValue = NULL;
    Common_Json_GetAttrValue(parentItem, -1, "FileName", NULL, &pStringValue, NULL,
                             NULL);
    if (NULL != pStringValue)
    {
        FILE *fpExport = NULL;
        FILE *fpTmp = NULL;
        S32 nLen = 0;
        S8 *pBuff = NULL;
        S8 system_cmd[128];
        Export_CfgHead nHead;

        if(export_type == 1)
        {
           snprintf(system_cmd, sizeof(system_cmd), "cd /;tar -cf %s %s/* %s",
                    TMP_EXPORT_CONFIG_FILE_ABSOLUTE_PATH+1, CONFIG_LOCKCFG_ABSOLUTE_PATH+1, LOGO_PATH+1);
        }
        else if(export_type == 0)
        {
            snprintf(system_cmd, sizeof(system_cmd), "cd /;tar -cf %s %s/*.json",
                     TMP_EXPORT_CONFIG_FILE_ABSOLUTE_PATH+1, CONFIG_ABSOLUTE_PATH+1);

            if(Common_File_IsExist((char *)"/usr/etc/cfgfiles/custom_audio"))
            {
                snprintf(system_cmd, sizeof(system_cmd), "%s %s/custom_audio",system_cmd, CONFIG_ABSOLUTE_PATH+1);
            }
        }
        else
        {
            return -1;
        }

        Common_System(system_cmd);

        fpExport = Common_File_fOpen(pStringValue, (char *)"wb+");
        if (NULL == fpExport)
        {
            LOGE("Common_File_fOpen fail fpExport.\n");
            return -1;
        }

        fpTmp = Common_File_fOpen((char *)TMP_EXPORT_CONFIG_FILE_ABSOLUTE_PATH,
                                  (char *)"rb");
        if (NULL == fpTmp)
        {
            LOGE("Common_File_fOpen fail fpTmp.\n");
            Common_File_fClose(fpExport);
            fpExport = NULL;
            return -1;
        }

        Common_File_fSeek(fpTmp, 0, SEEK_END);
        nLen = Common_File_fTell(fpTmp);
        Common_File_fSeek(fpTmp, 0, SEEK_SET);
        if (nLen <= 0)
        {
            Common_File_fClose(fpExport);
            fpExport = NULL;
            Common_File_fClose(fpTmp);
            fpTmp = NULL;
            return 0;
        }

        pBuff = (S8 *)Common_Malloc(nLen + sizeof(Export_CfgHead), 0, __FUNCTION__,
                                    __LINE__);
        if (NULL == pBuff)
        {
            Common_File_fClose(fpExport);
            fpExport = NULL;
            Common_File_fClose(fpTmp);
            fpTmp = NULL;
            return -1;
        }

        COMMON_CLR_ARG(nHead);
        nHead.nDataLen = nLen;
        Common_Copy(pBuff, &nHead, sizeof(Export_CfgHead));
        Common_File_fRead(pBuff + sizeof(Export_CfgHead), nLen, 1, fpTmp);
        Common_File_fWrite(pBuff, nLen + sizeof(Export_CfgHead), 1, fpExport);

        if(NULL != pBuff)
        {
            Common_Free(pBuff, __FUNCTION__, __LINE__);
            pBuff = NULL;
        }

        if(NULL != fpExport)
        {
            Common_File_fClose(fpExport);
            fpExport = NULL;
        }

        if(NULL != fpTmp)
        {
            Common_File_fClose(fpTmp);
            fpTmp = NULL;
        }

        snprintf(system_cmd, sizeof(system_cmd), "rm %s",
                 TMP_EXPORT_CONFIG_FILE_ABSOLUTE_PATH);
        Common_System(system_cmd);

        Common_Json_SetAttrValueInt(outItem, "ExportType", export_type);

    }

    return 0;
}

static S32 Core_Export_ImportCfg(const char *uriString, const char *condition,
                                 Common_cJSON_T *in, Common_cJSON_T *out)
{
    cJSON_Struct *parentItem = NULL;
    char *pStringValue;
    int lockedParam = 0;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;

    pStringValue = NULL;
    Common_Json_GetAttrValue(parentItem, -1, "FileName", NULL, &pStringValue, NULL,
                             NULL);
    Common_Json_GetAttrValueInt(parentItem, "LockedParam", &lockedParam);
    if (NULL != pStringValue)
    {
        FILE *fpExport = NULL;
        FILE *fpTmp = NULL;
        S32 nLen = 0;
        S8 *pBuff = NULL;
        S8 system_cmd[128];
        Export_CfgHead nHead;

        fpExport = Common_File_fOpen(pStringValue, (char *)"rb");
        if (NULL == fpExport)
        {
            LOGE("Common_File_fOpen fail fpExport.\n");
            return -1;
        }

        Common_File_fSeek(fpExport, 0, SEEK_END);
        nLen = Common_File_fTell(fpExport);
        Common_File_fSeek(fpExport, 0, SEEK_SET);

        COMMON_CLR_ARG(nHead);
        Common_File_fRead(&nHead, sizeof(Export_CfgHead), 1, fpExport);
        if (nLen <= (S32)sizeof(Export_CfgHead)
                || (S32)(nHead.nDataLen + sizeof(Export_CfgHead)) != nLen)
        {
            Common_File_fClose(fpExport);
            fpExport = NULL;
            return 0;
        }

        fpTmp = Common_File_fOpen((char *)TMP_IMPORT_CONFIG_FILE_ABSOLUTE_PATH,
                                  (char *)"wb+");
        if (NULL == fpExport)
        {
            LOGE("Common_File_fOpen fail fpTmp.\n");
            Common_File_fClose(fpExport);
            fpExport = NULL;
            return -1;
        }

        pBuff = (S8 *)Common_Malloc(nLen - sizeof(Export_CfgHead), 0, __FUNCTION__,
                                    __LINE__);
        if (NULL == pBuff)
        {
            Common_File_fClose(fpExport);
            fpExport = NULL;
            Common_File_fClose(fpTmp);
            fpTmp = NULL;
            return -1;
        }

        Common_File_fRead(pBuff, nLen - sizeof(Export_CfgHead), 1, fpExport);
        Common_File_fWrite(pBuff, nLen - sizeof(Export_CfgHead), 1, fpTmp);

        if(NULL != pBuff)
        {
            Common_Free(pBuff, __FUNCTION__, __LINE__);
            pBuff = NULL;
        }

        if(NULL != fpExport)
        {
            Common_File_fClose(fpExport);
            fpExport = NULL;
        }

        if(NULL != fpTmp)
        {
            Common_File_fClose(fpTmp);
            fpTmp = NULL;
        }
        // 写导入标志
        snprintf(system_cmd, sizeof(system_cmd),
                 "cd /;tar -xf %s -C /;touch /usr/etc/import_flag",
                 TMP_IMPORT_CONFIG_FILE_ABSOLUTE_PATH);
        Common_System(system_cmd);

        if(lockedParam)
        {
            if(!Common_File_IsExist((char *)"/usr/etc/default"))
            {
                Common_System("mkdir /usr/etc/default");
            }

            snprintf(system_cmd, sizeof(system_cmd),
                     "cp -r /usr/etc/cfgfiles/* /usr/etc/default/");
            Common_System(system_cmd);
            if(Common_File_IsExist("/usr/etc/default/AliIoT4ovfs.json"))
            {
                Common_System("rm -r /usr/etc/default/AliIoT4ovfs.json");
            }
            if(Common_File_IsExist("/usr/etc/default/AwsIoT4ovfs.json"))
            {
                Common_System("rm -r /usr/etc/default/AwsIoT4ovfs.json");
            }
        }
    }

    return 0;
}

static S32 Core_Export_PraseInputJson(Common_cJSON_T *inputData, S8 **method,
                                      S8 **uri, Common_cJSON_T **inData)
{
    Common_cJSON_T *header = Common_cJSON_GetObjectItem(inputData, "Header");
    if(header == NULL)
    {
        LOGE("Get header fail!\n");
        return -1;
    }

    Common_cJSON_T *tmp = NULL;
    tmp = Common_cJSON_GetObjectItem(header, "Method");
    if(tmp == NULL)
    {
        LOGE("Can't found method!\n");
        return -1;
    }
    if(method)
    {
        *method = tmp->valuestring;
    }

    tmp = Common_cJSON_GetObjectItem(header, "Uri");
    if(tmp == NULL)
    {
        LOGE("Can't found Uri.\n");
        return -1;
    }
    if(uri)
    {
        *uri = tmp->valuestring;
    }


    tmp = Common_cJSON_GetObjectItem(inputData, "Data");
    if(inData)
    {
        *inData = tmp;
    }

    return 0;
}

static S32 Core_Export_GetUriAndQue(const S8 *srcUri, S8 **uriStr,
                                    S8 **conditionStr)
{
    S8 *tmp = (S8 *)strstr(srcUri, "?");
    if(tmp)
    {
        char buff[256];
        memset(buff, 0, sizeof(buff));
        memcpy(buff, srcUri, (int)(tmp - srcUri));
        *uriStr         = Common_StrDup(buff, __FUNCTION__, __LINE__);
        *conditionStr   = Common_StrDup(tmp + 1, __FUNCTION__, __LINE__);
    }
    else
    {
        *uriStr         = Common_StrDup((S8 *)srcUri, __FUNCTION__, __LINE__);
        *conditionStr   = NULL;
    }

    return 0;
}

static Common_cJSON_T *Core_Export_GenerateOutParam(S32 retCode,
        Common_cJSON_T *outData)
{
    Common_cJSON_T *root = Common_cJSON_CreateObject();
    Common_cJSON_T *header = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(root, "Header", header);

    Common_cJSON_AddNumberToObject(header, "Code", retCode);
    if(retCode != 0)
    {
        Common_cJSON_AddStringToObject(header, "Decribe", "Operation fail.");
    }

    if(outData)
    {
        Common_cJSON_AddItemToObject(root, "Data", outData);
    }

    return root;
}
/*
static Common_cJSON_T* Core_Export_GenerateInParam(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inData)
{
    if(uriPath == NULL)
    {
        return NULL;
    }

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
*/
S32 Core_Export_CallFunctions(ModuleHandle_T hModuleHandle,
                              cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{
#if 0
    char *out = NULL;
    LOGI("recv call input:%s \n",
         out = Common_cJSON_PrintUnformatted((Common_cJSON_T *)pInParams, NULL));
    if(out)
    {
        Common_Free(out, __FUNCTION__, __LINE__);
    }
#endif
    char *method            = NULL;
    char *uri               = NULL;
    Common_cJSON_T *inData  = NULL;
    int ret = Core_Export_PraseInputJson((Common_cJSON_T *)pInParams, &method, &uri,
                                         &inData);
    if(ret != 0)
    {
        LOGE("inparam parse fail!\n");
        return -1;
    }

    if(method == NULL)
    {
        LOGE("method is NULL!\n");
        return -1;
    }

    char *uriString     = NULL;
    char *uriCondition  = NULL;
    Core_Export_GetUriAndQue(uri, &uriString, &uriCondition);

    if (Common_StrniCmp(uriString, (char *)"/Core/ExportCfg",
                        strlen("/Core/ExportCfg")) != 0 &&
            Common_StrniCmp(uriString, (char *)"/Core/ImportCfg",
                            strlen("/Core/ImportCfg")) != 0)
    {
        if(uriString)
        {
            Common_Free(uriString, __FUNCTION__, __LINE__);
        }

        if(uriCondition)
        {
            Common_Free(uriCondition, __FUNCTION__, __LINE__);
        }

        return -1;
    }

    ret = 0;

    Common_cJSON_T *outData = Common_cJSON_CreateObject();
    if(Common_StriCmp(method, (char *)"get") == 0)
    {
        if (Common_StriCmp(uriString, (char *)"/Core/ExportCfg") == 0)
        {
            Core_Export_ExportCfg(uriString, uriCondition, inData, outData);
        }
        else
        {
            ret = -1;
        }
    }
    else if(Common_StriCmp(method, (char *)"put") == 0)
    {
        if (Common_StriCmp(uriString, (char *)"/Core/ImportCfg") == 0)
        {
            Core_Export_ImportCfg(uriString, uriCondition, inData, outData);
        }
        else
        {
            ret = -1;
        }
    }
    else if(Common_StriCmp(method, (char *)"post") == 0)
    {
        ret = -1;
    }
    else if(Common_StriCmp(method, (char *)"delete") == 0)
    {
        ret = -1;
    }
    else
    {
        LOGE("Unknow method=%s\n", method);
        ret = -1;
    }

    if(outData->child == NULL)
    {
        Common_cJSON_Delete(outData);
        outData = NULL;
    }

    *pOutParams = Core_Export_GenerateOutParam(ret, outData);

    if(uriString)
    {
        Common_Free(uriString, __FUNCTION__, __LINE__);
    }

    if(uriCondition)
    {
        Common_Free(uriCondition, __FUNCTION__, __LINE__);
    }

    return ret;
}

S32 Core_Export_Init(ModuleHandle_T hModuleHandle)
{
    S8 system_cmd[256];
    if (g_bInit)
    {
        return 0;
    }
    if (Common_File_IsExist((char *)"/usr/etc/import_flag"))
    {
        snprintf(system_cmd, sizeof(system_cmd), "cd /;tar -xf %s -C /;rm %s;rm /usr/etc/import_flag",
                 TMP_IMPORT_CONFIG_FILE_ABSOLUTE_PATH, TMP_IMPORT_CONFIG_FILE_ABSOLUTE_PATH);
        Common_System(system_cmd);
    }
    g_bInit = 1;
    return 0;
}
