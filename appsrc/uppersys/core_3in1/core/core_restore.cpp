#include <string.h>
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "core_restore.h"
#include "core_power.h"
/*
Uri:/Core/Restore/Update
Method:Put // 各模块提交自己可恢复默认项
Data:
{
    ResList=[
            {
                Uri=,  // 恢复项的Uri
                Label=, // 恢复项
                NeedReboot=,// 1-重启生效,
            }
        ]
}
Uri:/Core/Restore/All
Method:Put
恢复所有
Uri:/Core/Restore/Items
Method:Get/Put
Data[Get]: // 获取支持哪些恢复默认项 ,以及恢复时是否需要重启生效
{
ResList=[
        {
        Label=, // 恢复项
        NeedReboot=,// 1-重启生效,Get时有效，指示该配置需要重启才生效
        }
    ]
}
Data[Put] // 指定某些项进行恢复默认
{
ResList=[Label1,Label2]
NeedReboot=,// 1-重启生效,指示设备重启
}
-- out          // 返回信息，当有部分成功时，在列表内会有相应 Code表示哪些项恢复成功，哪些失败
{
    ResList=[
    {
        Lable=,
        Code=
    }
    ]
}

Uri:/Core/


// class:
{
    classname1:[{module:xxx,Label:xxx}
    ]
}
*/

typedef struct _tagCoreRestoreItems
{
    S8 *szUri;
    S8 *szLabel;
    S8 *szModule;
    S32 bNeedReboot;
    struct _tagCoreRestoreItems  *pPrev;
    struct _tagCoreRestoreItems  *pNext;
} CoreRestoreItems_T;

typedef struct _tagCoreRestoreClassItem
{
    S8 *szModule;
    S8 *szLable;
    S8 *szCfgFile;
    struct _tagCoreRestoreClassItem *pPrev;
    struct _tagCoreRestoreClassItem *pNext;

} CoreRestoreClassItem_T;
typedef struct _tagCoreRestoreClass
{
    S8 szClassName[16]; // 分类:  默认 :NetConfig,UserConfig,AlarmConfig,OtherConfig,其中OtherConfig 必须有
    S32 bNeedReboot;
    CoreRestoreClassItem_T *pClassItems; // 归类项 规则
    CoreRestoreItems_T *pItems;// 被归类的项
    struct _tagCoreRestoreClass *pPrev;
    struct _tagCoreRestoreClass *pNext;
} CoreRestoreClass_T;

typedef struct _tagCoreRestoreMgr
{
    CoreRestoreClass_T *pClassHead;
    CoreRestoreClass_T tOtherClass; // OtherConfig
    Common_Lock_T hLock;
} CoreRestoreMgr_T;
static CoreRestoreMgr_T g_tRestoreMgr;
static S32 g_bRestoreInit = 0;
static const S8 *g_Restore_default = "{\"Classes\":{\
							 \"NetConfig\":[{\"Module\":\"NetWork\",\"CfgFile\":\"NetWork.json\",\"NeedReboot\":1},\
                                             {\"Module\":\"Onvif\",\"CfgFile\":\"Onvif.json\",\"NeedReboot\":1},\
											 {\"Module\":\"Webserver\",\"CfgFile\":\"Webserver.json\",\"NeedReboot\":1}],\
							 \"UserConfig\":[{\"Module\":\"Access\",\"CfgFile\":\"Access.json\",\"NeedReboot\":1}],\
							 \"AlarmConfig\":[{\"Module\":\"Alarm\",\"CfgFile\":\"Alarm.json\",\"NeedReboot\":1}]\
							 }}";
S32 Core_Restore_CallFunctions(ModuleHandle_T hModuleHandle,
                               cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{
    S32 nRet = -1;
    S8 *szUri = NULL;
    S8 *szMethod = NULL;
    // CoreRestoreItems_T *pNode = NULL,*pCurrNode = NULL;
    CoreRestoreClassItem_T *pClassItemNode = NULL;
    cJSON_Struct *pOutJson = NULL;
    S32 nPutNeedReboot = 0;
    Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &szUri, NULL, NULL);
    Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &szMethod, NULL,
                             NULL);

    if(0 == Common_StriCmp((char *)"/Core/Restore", szUri) &&
            0 == Common_StriCmp((char *)"Get", szMethod))
    {
        cJSON_Struct *pNode, *pNode1;
        S32 nWhich = 0;
        pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pOutJson != NULL)
        {
            Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                     NULL, 0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                     0);

            pNode = Common_Json_SetAttrValue(pOutJson, -1, "Data/ResList",
                                             Common_Json_Type_Array, NULL, 0, 0);


            Common_Json_SetAttrValue(pNode, nWhich, "/Uri", Common_Json_Type_String,
                                     "/Core/Restore/Update", 0, 0);
            Common_Json_SetAttrValue(pNode, nWhich, "/Label", Common_Json_Type_String,
                                     "Update", 0, 0);
            Common_Json_SetAttrValue(pNode, nWhich, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nWhich, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
            nWhich++;

            Common_Json_SetAttrValue(pNode, nWhich, "/Uri", Common_Json_Type_String,
                                     "/Core/Restore/Items", 0, 0);
            Common_Json_SetAttrValue(pNode, nWhich, "/Label", Common_Json_Type_String,
                                     "Items", 0, 0);
            Common_Json_SetAttrValue(pNode, nWhich, "/Describe", Common_Json_Type_String,
                                     "None", 0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, nWhich, "/Method",
                                              Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
            nWhich++;
        }
    }

    if(0 != Common_StrniCmp((char *)"/Core/Restore", szUri, 13))
    {
        return -1;
    }
    // 以防未初始化
    Core_Restore_Init(hModuleHandle);

    if (0 == Common_StriCmp((char *)"/Core/Restore/Update", szUri) &&
            0 == Common_StriCmp((char *)"Put", szMethod))
    {
#if 0
        cJSON_Struct *pArray;
        nRet = 0;
        pArray = Common_Json_GetItem(pInParams, -1, "/Data/ResList");
        if (pArray != NULL)
        {
            S8 *szItemUri = NULL, *szItemLabel = NULL;
            S32 nNeedReboot;
            S32 nArrayNum = 0, nWhich = 0;
            nArrayNum = Common_Json_ArraySize(pArray);
            for (nWhich = 0; nWhich < nArrayNum; nWhich++)
            {
                szItemUri = NULL;
                szItemLabel = NULL;
                S8 *pModuleName = NULL;
                nNeedReboot = 0;
                Common_Json_GetAttrValue(pArray, nWhich, "Uri", NULL, &szItemUri, NULL, NULL);
                Common_Json_GetAttrValue(pArray, nWhich, "Label", NULL, &szItemLabel, NULL,
                                         NULL);
                Common_Json_GetAttrValue(pArray, nWhich, "NeedReboot", NULL, NULL, &nNeedReboot,
                                         NULL);
                Common_UriOneParse(szItemUri, NULL, &pModuleName, NULL);
                if (szItemLabel != NULL && pModuleName != NULL )
                {
                    S32 bExist = 0;
                    CoreRestoreClass_T *pClass = NULL;
                    CoreRestoreClassItem_T *pClassItems = NULL;


                    // 检查是否存在
                    Common_Lock(g_tRestoreMgr.hLock);
                    // 先检查归类
                    pClass = g_tRestoreMgr.pClassHead;
                    while(pClass != NULL)
                    {
                        pClassItems = pClass->pClassItems;
                        while(pClassItems != NULL)
                        {
                            if (pClassItems->szModule != NULL)
                            {
                                if ( 0 == Common_StriCmp(pClassItems->szModule, pModuleName))
                                {
                                    //
                                    if (pClassItems->szLable != NULL)
                                    {
                                        // 某一项处理
                                        if (0 == Common_StriCmp(pClassItems->szLable, szItemLabel))
                                        {
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        // 整个模块都处理
                                        break;
                                    }

                                }
                            }
                            pClassItems = pClassItems->pNext;
                        }
                        if (pClassItems != NULL)
                        {
                            // 找到
                            break;
                        }
                        pClass = pClass->pNext;
                    }
                    //
                    if (pClassItems == NULL)
                    {
                        // 归类为Other
                        pClass = &g_tRestoreMgr.tOtherClass;
                    }


                    pNode = pClass->pItems;
                    while(pNode != NULL)
                    {
                        pCurrNode = pNode;
                        pNode = pNode->pNext;
                        if (0 == Common_StriCmp(pCurrNode->szLabel, szItemLabel) &&
                                0 == Common_StriCmp(pCurrNode->szModule, pModuleName))
                        {
                            // 存在
                            bExist = 1;
                            if (0 != Common_StriCmp(pCurrNode->szUri, szItemUri))
                            {
                                //更新
                                S8 *pNewUri = NULL;
                                pNewUri = Common_StrDup(szItemUri, __FUNCTION__, __LINE__);
                                if (pNewUri != NULL)
                                {
                                    Common_Free(pCurrNode->szUri, __FUNCTION__, __LINE__);
                                    pCurrNode->szUri = pNewUri;
                                }

                            }
                            break;
                        }

                    }
                    if (!bExist)
                    {
                        // 加一新
                        S8 *pNewUri = NULL, *pNewLabel = NULL;
                        pNewUri = Common_StrDup(szItemUri, __FUNCTION__, __LINE__);
                        pNewLabel = Common_StrDup(szItemLabel, __FUNCTION__, __LINE__);
                        if (pNewLabel != NULL && pNewUri != NULL)
                        {
                            pNode = (CoreRestoreItems_T *)Common_Malloc(sizeof(CoreRestoreItems_T), 0,
                                    __FUNCTION__, __LINE__);
                            if (pNode != NULL)
                            {
                                memset(pNode, 0, sizeof(CoreRestoreItems_T));
                                pNode->szUri = pNewUri;
                                pNewUri = NULL;
                                pNode->szLabel = pNewLabel;
                                pNewLabel = NULL;
                                pNode->szModule = pModuleName;
                                pModuleName = NULL;
                                pNode->bNeedReboot = nNeedReboot;
                                if (nNeedReboot)
                                {
                                    pClass->bNeedReboot = 1;
                                }
                                pNode->pNext = pClass->pItems;
                                if (pClass->pItems != NULL)
                                {
                                    pClass->pItems->pPrev = pNode;
                                }
                                pClass->pItems = pNode;
                            }
                        }


                        Common_Free(pNewLabel, __FUNCTION__, __LINE__);
                        Common_Free(pNewUri, __FUNCTION__, __LINE__);

                    }
                    Common_UnLock(g_tRestoreMgr.hLock);
                }
                Common_Free(pModuleName, __FUNCTION__, __LINE__);
                pModuleName = NULL;
            }


        }
#endif
    }
    else if (0 == Common_StriCmp((char *)"/Core/Restore/All", szUri))
    {
        if (0 == Common_StriCmp((char *)"Put", szMethod))
        {
            //CoreRestoreClass_T *pClassNode = NULL;
            //CoreRestoreItems_T *pItems = NULL;
            //S32 nReqRet = -1;

            // ptz
            cJSON_Struct *pRequrieJson = NULL;
            pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pRequrieJson != NULL)
            {
                S8 szTmp[128];
                sprintf(szTmp, "/ptz/restore");
                Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                         NULL, 0, 0);
                Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                         Common_Json_Type_String, szTmp, 0, 0);
                Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Method",
                                         Common_Json_Type_String, "Put", 0, 0);
                Module_CallFunctions(hModuleHandle, pRequrieJson, NULL, 3000);
                Common_Json_Delete_ex(pRequrieJson, __FUNCTION__, __LINE__);
                pRequrieJson = NULL;
            }

            Common_Json_GetAttrValue(pInParams, -1, "/Data/NeedReboot", NULL, NULL,
                                     &nPutNeedReboot, NULL);
#if 0
            Common_Lock(g_tRestoreMgr.hLock);
            pClassNode = g_tRestoreMgr.pClassHead;
            if (pClassNode == NULL)
            {
                pClassNode = &g_tRestoreMgr.tOtherClass;
            }
            while(pClassNode != NULL)
            {
                // 恢复
                pItems = pClassNode->pItems;
                while(pItems != NULL)
                {

                    cJSON_Struct *pRequrieJson = NULL, *pReqOut = NULL;

                    pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                    if (pRequrieJson != NULL)
                    {
                        Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                                 NULL, 0, 0);
                        Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                                 Common_Json_Type_String, pItems->szUri, 0, 0);
                        nReqRet = Module_CallFunctions(hModuleHandle, pRequrieJson, &pReqOut, 3000);

                        Common_Json_Delete(pRequrieJson);
                        Common_Json_Delete(pReqOut);
                        pRequrieJson = NULL;
                        pReqOut = NULL;

                    }

                    pItems = pItems->pNext;
                }

                pClassNode = pClassNode->pNext;
                if (pClassNode == NULL && pClassNode != &g_tRestoreMgr.tOtherClass)
                {
                    pClassNode = &g_tRestoreMgr.tOtherClass;
                }

            }

            Common_UnLock(g_tRestoreMgr.hLock);
#else
            // 直接删除相应的配置
            S8 szCmd[128];
            sprintf(szCmd, "rm /usr/etc/cfgfiles/*;touch /usr/etc/restore_other; rm /usr/etc/OnvifDiscovery_No_Response;rm /usr/etc/soundFile/*; touch /usr/etc/reset.conf");
            Common_System(szCmd);

            int deletelockedcfg = 0;
            Common_Json_GetAttrValue(pInParams, -1, "/Data/DeleteLockedCfg", NULL, NULL,
                                     &deletelockedcfg, NULL);

            if(deletelockedcfg)
            {
                Common_System("rm /usr/etc/default/*; rm /usr/etc/logo.png; rm /usr/etc/CCIDFile.json; touch /usr/etc/reset.conf");
            }
#endif
            nRet = 0;
        }
        else
        {
            nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
        }

    }
    else if (0 == Common_StriCmp((char *)"/Core/Restore/Items", szUri))
    {
        if (0 == Common_StriCmp((char *)"Get", szMethod))
        {
            // 获取列表
            S32 nWhich = 0;
            cJSON_Struct *pArray;
            nRet = 0;
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, nRet, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                         0);
                pArray = Common_Json_SetAttrValue(pOutJson, -1, "Data/ResList",
                                                  Common_Json_Type_Array, NULL, 0, 0);
                if (pArray != NULL)
                {
                    CoreRestoreClass_T *pClassNode = NULL;
                    Common_Lock(g_tRestoreMgr.hLock);
                    pClassNode = g_tRestoreMgr.pClassHead;
                    while(pClassNode != NULL)
                    {
                        //Common_Json_SetAttrValue(pArray,nWhich,"Uri",Common_Json_Type_String,pNode->szUri,0,0);
                        Common_Json_SetAttrValue(pArray, nWhich, "Label", Common_Json_Type_String,
                                                 pClassNode->szClassName, 0, 0);
                        Common_Json_SetAttrValue(pArray, nWhich, "NeedReboot", Common_Json_Type_Number,
                                                 NULL, pClassNode->bNeedReboot, 0);
                        nWhich++;
                        pClassNode = pClassNode->pNext;

                    }
                    Common_Json_SetAttrValue(pArray, nWhich, "Label", Common_Json_Type_String,
                                             g_tRestoreMgr.tOtherClass.szClassName, 0, 0);
                    Common_Json_SetAttrValue(pArray, nWhich, "NeedReboot", Common_Json_Type_Number,
                                             NULL, g_tRestoreMgr.tOtherClass.bNeedReboot, 0);
                    nWhich++;
                    Common_UnLock(g_tRestoreMgr.hLock);

                }


            }


        }
        else if (0 == Common_StriCmp((char *)"Put", szMethod))
        {
            cJSON_Struct *pArray, *pArrayResult = NULL;
            nRet = 0;
            Common_Json_GetAttrValue(pInParams, -1, "/Data/NeedReboot", NULL, NULL,
                                     &nPutNeedReboot, NULL);
            pArray = Common_Json_GetItem(pInParams, -1, "/Data/ResList");
            if (pArray != NULL)
            {
                S8 *szItemLabel = NULL;
                //S32 nNeedReboot;
                S32 nArrayNum = 0, nWhich = 0, nUriCount = 0;
                //S32 nReqRet = 0;
                nArrayNum = Common_Json_ArraySize(pArray);
                if (nArrayNum > 0)
                {

                    CoreRestoreClass_T *pClassNode = NULL;
                    //CoreRestoreItems_T *pItems = NULL;
                    S32 nSuccOunt = 0;

                    pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                    if (pOutJson != NULL)
                    {
                        Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                                 0);
                        pArrayResult = Common_Json_SetAttrValue(pOutJson, -1, "Data/ResList",
                                                                Common_Json_Type_Array, NULL, 0, 0);
                    }


                    for (nWhich = 0; nWhich < nArrayNum; nWhich++)
                    {
                        S32 bSucc = -1;
                        szItemLabel = NULL;
                        //nNeedReboot = 0;
                        Common_Json_GetAttrValue(pArray, nWhich, NULL, NULL, &szItemLabel, NULL, NULL);
                        if (szItemLabel != NULL)
                        {
                            Common_Lock(g_tRestoreMgr.hLock);
                            pClassNode = g_tRestoreMgr.pClassHead;
                            if (pClassNode == NULL)
                            {
                                pClassNode = &g_tRestoreMgr.tOtherClass;
                            }
                            while(pClassNode != NULL)
                            {

                                if (0 == Common_StriCmp(pClassNode->szClassName, szItemLabel))
                                {
                                    // 找到

#if 0                                       // 恢复
                                    pItems = pClassNode->pItems;
                                    while(pItems != NULL)
                                    {

                                        cJSON_Struct *pRequrieJson = NULL, *pReqOut = NULL;

                                        pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                                        if (pRequrieJson != NULL)
                                        {
                                            Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                                                     NULL, 0, 0);
                                            Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                                                     Common_Json_Type_String, pItems->szUri, 0, 0);
                                            Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Method",
                                                                     Common_Json_Type_String, "Put", 0, 0);
                                            nReqRet = Module_CallFunctions(hModuleHandle, pRequrieJson, &pReqOut, 3000);
                                            S32 nCode = -1;
                                            Common_Json_GetAttrValue(pReqOut, nWhich, "Header/Code", NULL, NULL, &nCode,
                                                                     NULL);

                                            if(nCode == 0)
                                            {
                                                bSucc = 0;

                                            }
                                            else if (bSucc == 0)
                                            {
                                                bSucc = MODULE_ERROR_TYPE_PARTIAL_SUCCESS;
                                            }
                                            Common_Json_Delete(pRequrieJson);
                                            Common_Json_Delete(pReqOut);
                                            pRequrieJson = NULL;
                                            pReqOut = NULL;

                                        }

                                        pItems = pItems->pNext;
                                    }
#else

                                    // 直接删除相应的配置
                                    S8 szCmd[128];
                                    if (0 == Common_StriCmp(pClassNode->szClassName, (char *)"Others"))
                                    {
                                        // 打开目录 查找
                                        // ptz
                                        cJSON_Struct *pRequrieJson = NULL;
                                        pRequrieJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                                        if (pRequrieJson != NULL)
                                        {
                                            S8 szTmp[128];
                                            sprintf(szTmp, "/ptz/restore");
                                            Common_Json_SetAttrValue(pRequrieJson, -1, "Header", Common_Json_Type_Object,
                                                                     NULL, 0, 0);
                                            Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Uri",
                                                                     Common_Json_Type_String, szTmp, 0, 0);
                                            Common_Json_SetAttrValue(pRequrieJson, -1, "Header/Method",
                                                                     Common_Json_Type_String, "Put", 0, 0);
                                            Module_CallFunctions(hModuleHandle, pRequrieJson, NULL, 3000);
                                            Common_Json_Delete_ex(pRequrieJson, __FUNCTION__, __LINE__);
                                            pRequrieJson = NULL;
                                        }
#ifdef WIN32
#else
                                        DIR *sp_dp = NULL;
                                        struct dirent *ptr = NULL;
                                        S32 bFound = 0;
                                        sp_dp = opendir("/usr/etc/cfgfiles");
                                        if (sp_dp != NULL)
                                        {
                                            while((ptr = readdir(sp_dp)) != NULL)
                                            {
                                                if(0 == Common_StrCmp(ptr->d_name, (char *)".") ||
                                                        0 == Common_StrCmp(ptr->d_name, (char *)".."))
                                                {
                                                    continue;
                                                }
                                                bFound = 0;
                                                CoreRestoreClass_T *pClassNodeCheck = NULL;
                                                pClassNodeCheck = g_tRestoreMgr.pClassHead;
                                                while(pClassNodeCheck != NULL)
                                                {
                                                    pClassItemNode = pClassNodeCheck->pClassItems;
                                                    while(pClassItemNode != NULL)
                                                    {
                                                        if (0 == Common_StriCmp(ptr->d_name, pClassItemNode->szCfgFile))
                                                        {
                                                            bFound = 1;
                                                            break;
                                                        }
                                                        pClassItemNode = pClassItemNode->pNext;
                                                    }
                                                    if (bFound)
                                                    {
                                                        break;
                                                    }
                                                    pClassNodeCheck = pClassNodeCheck->pNext;
                                                }

                                                if (bFound)
                                                {
                                                    continue;
                                                }
                                                sprintf(szCmd, "rm /usr/etc/cfgfiles/%s", ptr->d_name);
                                                Common_System(szCmd);
                                            }
                                            closedir(sp_dp);
                                        }
                                        Common_System("touch /usr/etc/restore_other");
#endif
                                    }
                                    else
                                    {
                                        pClassItemNode = pClassNode->pClassItems;
                                        while(pClassItemNode != NULL)
                                        {
                                            sprintf(szCmd, "rm /usr/etc/cfgfiles/%s", pClassItemNode->szCfgFile);
                                            Common_System(szCmd);
                                            pClassItemNode = pClassItemNode->pNext;
                                        }
                                    }




                                    bSucc = 0;
                                    //nNeedReboot = 1;
#endif

                                    // 错误码

                                    break;

                                }


                                if (pClassNode->pNext == NULL && pClassNode != &g_tRestoreMgr.tOtherClass)
                                {
                                    pClassNode = &g_tRestoreMgr.tOtherClass;
                                }
                                else
                                {
                                    pClassNode = pClassNode->pNext;
                                }

                            }

                            if(pArrayResult != NULL)
                            {
                                Common_Json_SetAttrValue(pArrayResult, nWhich, "Label", Common_Json_Type_String,
                                                         pClassNode->szClassName, 0, 0);
                                Common_Json_SetAttrValue(pArrayResult, nWhich, "Code", Common_Json_Type_Number,
                                                         NULL, bSucc, 0);
                            }
                            if (bSucc)
                            {
                                nSuccOunt++;
                            }


                            Common_UnLock(g_tRestoreMgr.hLock);
                        }
                    }

                    if (nSuccOunt == nUriCount)
                    {
                        // 成功
                        nRet = 0;
                        Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                                 0, 0);
                        Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                                 NULL, nRet, 0);
                    }
                    else
                    {
                        nRet = MODULE_ERROR_TYPE_PARTIAL_SUCCESS;
                        Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                                 0, 0);
                        Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                                 NULL, nRet, 0);
                    }



                }


            }
        }
    }
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
    if (nPutNeedReboot)
    {
        cJSON_Struct *pRebootParam;
        pRebootParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pRebootParam != NULL)
        {
            Common_Json_SetAttrValue(pRebootParam, -1, "Header", Common_Json_Type_Object,
                                     NULL, 0, 0);
            Common_Json_SetAttrValue(pRebootParam, -1, "Header/Uri",
                                     Common_Json_Type_String, "/Core/Power/Reboot", 0, 0);
            Common_Json_SetAttrValue(pRebootParam, -1, "Header/Method",
                                     Common_Json_Type_String, "Put", 0, 0);
            Common_Json_SetAttrValue(pRebootParam, -1, "Data", Common_Json_Type_Object,
                                     NULL, 0, 0);
            Common_Json_SetAttrValue(pRebootParam, -1, "Data/Delay",
                                     Common_Json_Type_Number, NULL, 3, 0);
            Core_Power_CallFunctions(hModuleHandle, pRebootParam, NULL);
            Common_Json_Delete_ex(pRebootParam, __FUNCTION__, __LINE__);
        }
    }
    return 0;
}

static void _Core_Restore_ApplyCustom(cJSON_Struct *pCustom)
{
    cJSON_Struct *pClass = NULL, *pClassItem = NULL, *pArray = NULL;
    CoreRestoreClass_T *pClassNode = NULL;

    if (pCustom == NULL)
    {
        return;
    }
    pClass = Common_Json_GetItem(pCustom, -1, "Classes");
    if (pClass != NULL)
    {
        S8 *szObjectName = NULL;
        pClassItem = Common_Json_GetFirstChild(pClass);
        while(pClassItem != NULL)
        {
            Common_Json_GetAttr(pClassItem, NULL, &szObjectName, NULL, NULL, NULL, NULL);
            if (szObjectName != NULL && 0 != Common_StriCmp(szObjectName, (char *)"Others"))
            {
                pClassNode = (CoreRestoreClass_T *)Common_Malloc(sizeof(CoreRestoreClass_T), 0,
                             __FUNCTION__, __LINE__);
                if (pClassNode != NULL)
                {
                    S32 bNeedReboot = 0;
                    memset(pClassNode, 0, sizeof(CoreRestoreClass_T));
                    strncpy(pClassNode->szClassName, szObjectName, 15);

                    pArray = pClassItem;
                    if (pArray != NULL)
                    {
                        S32 nIdx, nArraySize = Common_Json_ArraySize(pArray);
                        S8 *pStringValue = NULL, *pCfgFile = NULL;
                        S32 nNeedReboot = 0;
                        CoreRestoreClassItem_T *pItems = NULL;
                        for (nIdx = 0; nIdx < nArraySize; nIdx++)
                        {
                            pStringValue = NULL;
                            pCfgFile = NULL;
                            Common_Json_GetAttrValue(pArray, nIdx, "Module", NULL, &pStringValue, NULL,
                                                     NULL);
                            Common_Json_GetAttrValue(pArray, nIdx, "CfgFile", NULL, &pCfgFile, NULL, NULL);
                            nNeedReboot = 0;
                            Common_Json_GetAttrValue(pArray, nIdx, "NeedReboot", NULL, NULL, &nNeedReboot,
                                                     NULL);
                            if (nNeedReboot)
                            {
                                bNeedReboot = 1;
                            }
                            if (pStringValue != NULL && pCfgFile != NULL)
                            {
                                pItems = (CoreRestoreClassItem_T *)Common_Malloc(sizeof(CoreRestoreClassItem_T),
                                         0, __FUNCTION__, __LINE__);
                                if (pItems != NULL)
                                {
                                    memset(pItems, 0, sizeof(CoreRestoreClassItem_T));
                                    pItems->szModule = Common_StrDup(pStringValue, __FUNCTION__, __LINE__);
                                    pItems->szCfgFile = Common_StrDup(pCfgFile, __FUNCTION__, __LINE__);
                                    pStringValue = NULL;
                                    Common_Json_GetAttrValue(pArray, nIdx, "Label", NULL, &pStringValue, NULL,
                                                             NULL);
                                    if (pStringValue != NULL)
                                    {
                                        pItems->szLable = Common_StrDup(pStringValue, __FUNCTION__, __LINE__);
                                    }
                                    pItems->pNext = pClassNode->pClassItems;
                                    if (pClassNode->pClassItems != NULL)
                                    {
                                        pClassNode->pClassItems->pPrev = pItems;
                                    }
                                    pClassNode->pClassItems = pItems;
                                }
                            }
                        }

                    }
                    pClassNode->bNeedReboot = bNeedReboot;
                    pClassNode->pNext = g_tRestoreMgr.pClassHead;
                    if (g_tRestoreMgr.pClassHead != NULL)
                    {
                        g_tRestoreMgr.pClassHead->pPrev = pClassNode;
                    }
                    g_tRestoreMgr.pClassHead = pClassNode;


                }

            }
            pClassItem = Common_Json_GetNext(pClassItem);
        }
    }

}
S32 Core_Restore_Init(ModuleHandle_T hModuleHandle)
{
    cJSON_Struct *pCustom = NULL;
    //cJSON_Struct *pClass = NULL,*pClassItem = NULL,*pArray = NULL;
    if (g_bRestoreInit)
    {
        return 0;
    }
    if (Common_File_IsExist((char *)"/usr/etc/restore_other"))
    {
        Common_System("rm /usr/etc/cfgfiles/BoardSys.json;rm /usr/etc/cfgfiles/Ptz.json;rm /usr/etc/restore_other");
    }

    memset(&g_tRestoreMgr, 0, sizeof(g_tRestoreMgr));
    Common_Lock_Create(&g_tRestoreMgr.hLock, "Core_Restore_lock");


    Core_Restore_LoadCustom(hModuleHandle, &pCustom);
    if (pCustom != NULL)
    {
        _Core_Restore_ApplyCustom(pCustom);
        Common_Json_Delete_ex(pCustom, __FUNCTION__, __LINE__);
        pCustom = NULL;
    }

    if (g_tRestoreMgr.pClassHead == NULL)
    {
        // 未定义，则缺省值
        pCustom = Common_Json_Parse(g_Restore_default, NULL, NULL);
        if (pCustom != NULL)
        {
            _Core_Restore_ApplyCustom(pCustom);
            Common_Json_Delete_ex(pCustom, __FUNCTION__, __LINE__);
            pCustom = NULL;
        }

    }
    strcpy(g_tRestoreMgr.tOtherClass.szClassName, "Others");


    g_bRestoreInit = 1;
    return 0;
}
