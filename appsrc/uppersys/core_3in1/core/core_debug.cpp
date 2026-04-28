#include <stdlib.h>
#include <string.h>
#include "libcommon_api.h"
#include "libmodule_api.h"
#define DEBUG_PATH "/usr/etc/debug.json"
/*
Uri:/Core/Debug/Memory
Method:Get/Put
{
Enable:, // 0- 禁用,1-启用
}
Uri:/Core/Debug/Export
Method:Get
InData:
{
FileName:; // 导出文件名,包括路径；由调用者指定且管理删除。
}
*/

S32 Core_Debug_CallFunctions(ModuleHandle_T hModuleHandle,
                             cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{


    S32 nRet = -1;
    S8 *szUri = NULL;
    S8 *szMethod = NULL;
    cJSON_Struct *pOutJson = NULL;
    //S32 nCurrType = 0;
    Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &szUri, NULL, NULL);
    Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &szMethod, NULL,
                             NULL);
    if(0 != Common_StriCmp((char *)"/Core/Debug", szUri))
    {
        return -1;
    }
    do
    {
        if(0 == Common_StriCmp((char *)"/Core/Debug/Memory", szUri))
        {
            cJSON_Struct *pDebugJson = NULL;
            S32 bEnable = 0;
            Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Debug, &pDebugJson);
            Common_Json_GetAttrValue(pDebugJson, -1, "/Debug/Mem/Enable", NULL, NULL,
                                     &bEnable, NULL);
            if(0 == Common_StriCmp((char *)"Get", szMethod))
            {
                pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if (pOutJson != NULL)
                {
                    Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                             0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                             0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Enable", Common_Json_Type_Number,
                                             NULL, bEnable, 0);
                }
                nRet = 0;
            }
            else if(0 == Common_StriCmp((char *)"Put", szMethod))
            {

                Common_Json_GetAttrValue(pInParams, -1, "Data/Enable", NULL, NULL, &bEnable,
                                         NULL);
                if (pDebugJson == NULL)
                {
                    pDebugJson = Common_Json_New("/Debug", Common_Json_Type_Object, NULL, 0, 0);
                }
                if (pDebugJson != NULL)
                {
                    if (NULL == Common_Json_GetItem(pDebugJson, -1, "/Debug/Mem"))
                    {
                        Common_Json_SetAttrValue(pDebugJson, -1, "/Debug/Mem", Common_Json_Type_Object,
                                                 NULL, 0, 0);
                    }
                    Common_Json_SetAttrValue(pDebugJson, -1, "/Debug/Mem/Enable",
                                             Common_Json_Type_Number, NULL, bEnable, 0);
                }

                Module_SaveConfigByType(hModuleHandle, Module_ConfigType_Debug, pDebugJson);

                nRet = 0;

            }
            else
            {
                Common_Json_Delete_ex(pDebugJson, __FUNCTION__, __LINE__);
                pDebugJson = NULL;
                nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
                break;
            }
            Common_Json_Delete_ex(pDebugJson, __FUNCTION__, __LINE__);
            pDebugJson = NULL;

            nRet = 0;
        }
        else if(0 == Common_StriCmp((char *)"/Core/Debug/Export", szUri))
        {
            if(0 == Common_StriCmp((char *)"Get", szMethod))
            {
                S8 *szFileName = NULL;
                Common_Json_GetAttrValue(pInParams, -1, "Data/FileName", NULL, &szFileName,
                                         NULL, NULL);
                if (szFileName != NULL)
                {
                    S8 system_cmd[128];
                    snprintf(system_cmd, sizeof(system_cmd), "tar -czf %s /tmp/debug /tmp/modules",
                             szFileName);
                    Common_System(system_cmd);
                    pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                    if (pOutJson != NULL)
                    {
                        Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                                 0, 0);
                        Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                                 NULL, 0, 0);
                        Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                                 0);
                        Common_Json_SetAttrValue(pOutJson, -1, "Data/FileName", Common_Json_Type_String,
                                                 szFileName, 0, 0);
                    }
                    nRet = 0;
                }

            }
            else
            {
                nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
                break;
            }
        }

    }
    while (0);
    if (pOutJson != NULL)
    {
        if (pOutParams != NULL)
        {
            *pOutParams = pOutJson;
            pOutJson = NULL;
        }
    }
    else if(pOutParams != NULL)
    {
        pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pOutJson != NULL)
        {
            Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                     NULL, nRet, 0);
            *pOutParams = pOutJson;
            pOutJson = NULL;
        }

    }
    Common_Json_Delete(pOutJson);
    pOutJson = NULL;
    return 0;
}
