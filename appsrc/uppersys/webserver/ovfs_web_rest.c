#include "ovfs_web_rest.h"
#include "ovfs_web_func.h"

extern WebsHash actionTable;
extern int name_in_white_list(char* name);

#ifdef WEB_PRINT_DEBUG

static void web_init_logs(OVFS_WEB_CONTEXT_T *web)
{
    int i;
    for (i = 0; i < WEB_MAXLOGS; i++)
    {
        web->logfile[i] = NULL;
        web->fdebug[i] = NULL;
    }
}

void web_open_logfile(OVFS_WEB_CONTEXT_T *web, int i)
{
    if (web->logfile[i])
        web->fdebug[i] = fopen(web->logfile[i], i < 2 ? "ab" : "a");
}

static void web_close_logfile(OVFS_WEB_CONTEXT_T *web, int i)
{
    if (web->fdebug[i])
    {
        fclose(web->fdebug[i]);
        web->fdebug[i] = NULL;
    }
}

void web_close_logfiles(OVFS_WEB_CONTEXT_T *web)
{
    int i;
    for (i = 0; i < WEB_MAXLOGS; i ++)
        web_close_logfile(web, i);
}

void web_set_logfile(OVFS_WEB_CONTEXT_T *web, int i, const char *logfile)
{
    const char *s;
    char *t = NULL;
    web_close_logfile(web, i);
    s = web->logfile[i];
    web->logfile[i] = logfile;
    if (s)
        free((void*)s);
    if (logfile)
        if ((t = (char*)malloc(strlen(logfile) + 1)))
            strcpy(t, logfile);
    web->logfile[i] = t;
}

void web_set_recv_logfile(OVFS_WEB_CONTEXT_T *web, const char *logfile)
{
    web_set_logfile(web, WEB_INDEX_RECV, logfile);
}

void web_set_sent_logfile(OVFS_WEB_CONTEXT_T *web, const char *logfile)
{
    web_set_logfile(web, WEB_INDEX_SENT, logfile);
}

void web_set_test_logfile(OVFS_WEB_CONTEXT_T *web, const char *logfile)
{
    web_set_logfile(web, WEB_INDEX_TEST, logfile);
}
#endif

void ovfs_webt_init(OVFS_WEB_CONTEXT_T *web);

OVFS_WEB_CONTEXT_T *ovfs_web_new()
{
    OVFS_WEB_CONTEXT_T *web;

    web = (OVFS_WEB_CONTEXT_T *)Common_Calloc(1, sizeof(OVFS_WEB_CONTEXT_T), __FUNCTION__, __LINE__);
    if (web)
        ovfs_webt_init(web);

    return web;
}

/*
{
    WEB_CODE_OK = 0,
    // 以下错误编码,必须保持连续的整数递减. 修改后相应的增加字符串. 参考,s_errorStr 和 Ovfs_Web_StrError().
    WEB_CODE_BASE = -0xA0000,
    WEB_CODE_GeneralMistake = WEB_CODE_BASE-1, // 通常性错误
    WEB_CODE_InvalidArg = WEB_CODE_BASE-2, // 错误的参数
    WEB_CODE_LackingMem = WEB_CODE_BASE-3, // 内存不足
    WEB_CODE_Unauthorized = WEB_CODE_BASE-4, // 没有取得认证
    WEB_CODE_PermissionDenied = WEB_CODE_BASE-5, // 权限不足
    WEB_CODE_BlockingOperation = WEB_CODE_BASE-6, // 操作被阻塞
    WEB_CODE_InvalidJson = WEB_CODE_BASE-7, // json内容错误
    WEB_CODE_InternalMistake = WEB_CODE_BASE-8, // 内部错误
    WEB_CODE_Unsupported = WEB_CODE_BASE-9, // 不支持的功能
    WEB_CODE_LackingThread = WEB_CODE_BASE-10, // 无法创建线程
    WEB_CODE_TaskExist = WEB_CODE_BASE-11, // 同样任务已经存在
    WEB_CODE_FileNotAccess = WEB_CODE_BASE-12, // 文件无法访问
    WEB_CODE_IamBusy = WEB_CODE_BASE-13, // 正忙(通常是无法取得锁)

}
*/
static const char *s_errorStr[] =
{
    "Operation Ok",
    // "Base Error Place Holder",
    "General mistake",
    "Invalid argument",
    "Lacking memory",
    "Unauthorized",
    "Permission denied",//fuck it,少了个逗号两行字符串自动连接成一个了
    "Blocking operation",
    "Invalid json content",
    "Internal mistake",
    "Unsupported",
    "Lacking thread",
    "Task exist",
    "File not accessible",
    "Server is busy",
    "Single Account Login",
    "Session Need Login First",
    "Invalid Ip Mask Gateway",
    "Port Occupied",
    "Session Count Max"
};
static const int s_errorCount = sizeof(s_errorStr)/sizeof(s_errorStr[0]);
const char *Ovfs_Web_StrError(int error)
{
    const char *errorStr = "Unknown error";

    int index = 0;
    if (error >= 0)
    {
        errorStr = s_errorStr[index];
    }
    else
    {
        index = WEB_CODE_BASE - error;
        if (index > 0 && index < s_errorCount)
        {
            errorStr = s_errorStr[index];
        }
    }

    return errorStr;
}

static const char *s_methodTable[] = {"get", "put", "post", "delete"};

int Ovfs_Web_UpdateHeader(cJSON_Struct *header, int method, const char *uri)
{
    int ret = 0;
    if (header == NULL || (method < REST_GET || method > REST_DELETE) || uri == NULL)
    {
        ret = -1;
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValue(header, -1, "Method", Common_Json_Type_String, s_methodTable[method], 0, 0);
        Common_Json_SetAttrValue(header, -1, "Uri", Common_Json_Type_String, uri, 0, 0);
    }

    return ret;
}

int Ovfs_Web_RestMethodA(cJSON_Struct *header, cJSON_Struct *data, cJSON_Struct **OutParam, int TimeOut)
{
    int ret = 0;

    // 合并header和data部分为一个完整的rest请求.
    cJSON_Struct *pInParams = NULL;
    if (0 == ret)
    {
        if ((pInParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_AddItem(pInParams, -1, "Header", header);
            Common_Json_AddItem(pInParams, -1, "Data", data);
        }
    }

    // 调用access方法.
    cJSON_Struct *pOutResults = NULL;
    if (0 == ret)
    {
        if (TimeOut == 0)
        {
            TimeOut = 60000;
        }
        else
        {
            TimeOut = MIN2(60000, MAX2(3000, TimeOut));
        }

        char *path = NULL;
        Common_Json_GetAttrValueStr(header, "Auth/Path", &path);
        //LOGD("Auth/PAth:%s\n",path?path:"");
        //????session? ????sessionid?????????ban??
        if(!name_in_white_list(path))
        {
            if(g_ovfs_web->enable_session)
            {

                //bugfix 2019/11/07
                /*
                ??€??ˉsession?—? Auth/Method ??¨?¤????digest/upload??????????o§?—? ????”¨Method= 4. é?€??–??°sessionid??￥éa?èˉ??€?
                ??–è€… ??????sessionid?€?é?????Method= 3??????????”¨UsernameTokenéa?èˉ?
                */

                if(path && strstr(path,"upload") != NULL)
                {
                    Common_Json_SetAttrValueInt(header, "Auth/Method", 3);
                }
                else
                {
                    Common_Json_SetAttrValueInt(header, "Auth/Method", 4);
                }

            }
            else
            {
                //未开启session时，上传请求 直接 走 超级用户，效果等同于免验证
                if(path && (strstr(path,"upload") != NULL || strstr(path,"frmSysUpdateFree") != NULL))
                {
                    Common_Json_SetAttrValueInt(header, "Auth/Method", 1);
                    Common_Json_SetAttrValueStr(header, "Auth/Username", "(null)");
                    Common_Json_SetAttrValueStr(header, "Auth/Password", "ovfsZSJQZLHL");
                }
            }
        }

        char *uri = NULL;
        Common_Json_GetAttrValueStr(header, "Uri", &uri);

        if (g_ovfs_web->debugPrint == 1)
        {
            char *method = NULL;
            char *str = NULL;

            Common_Json_GetAttrValueStr(header, "Method", &method);
            str = Common_cJSON_Print(data, NULL);
            printf("[pid:%d,pos:%s/%d]Req(%s %s):%s : timeout:%d\n", (unsigned int)syscall(SYS_gettid), __FUNCTION__, __LINE__, method, uri, str,TimeOut);
            if(str)Common_Free(str,__FUNCTION__,__LINE__);

            if(Common_StrniCmp("/network", uri, 8) == 0 && s_networkfuncCb.config != NULL)
            {
                LOGW("s_networkfuncCb.config!\n");
                s_networkfuncCb.config((void *)pInParams, (void **)&pOutResults);
            }
            else
            {
                ret = Access_CallFunctions(g_AccessHandle, pInParams, &pOutResults, TimeOut);
            }

            str = Common_cJSON_Print(pOutResults, NULL);
            printf("[pid:%d,pos:%s/%d]Rsp(%s %s):%s\n", (unsigned int)syscall(SYS_gettid), __FUNCTION__, __LINE__, method, uri, str);
            if(str)Common_Free(str,__FUNCTION__,__LINE__);
        }
        else
        {
            if(Common_StrniCmp("/network", uri, 8) == 0 && s_networkfuncCb.config != NULL)
            {
                LOGW("s_networkfuncCb.config!\n");
                s_networkfuncCb.config((void *)pInParams, (void **)&pOutResults);
            }
            else
            {
                ret = Access_CallFunctions(g_AccessHandle, pInParams, &pOutResults, TimeOut);
            }
        }

        if (0 != ret)
        {
            char *uri = NULL;
            Common_Json_GetAttrValueStr(header, "Uri", &uri);
            LOGE("Access_CallFunctions(uri:%s) return %x\n", uri, ret);
            if (ret == ACCESS_ERROR_TYPE_NORIGHT)
            {
                ret = WEB_CODE_PermissionDenied;
            }
            else if (ret == ACCESS_ERROR_TYPE_AUTH)
            {
                ret = WEB_CODE_Unauthorized;
            }
        }
    }

    // 释放reset请求所用资源,但不包括"Header"和"Data"部分.这两部分是在接口外部申请和释放的.本接口仅仅引用了它们.
    if (pInParams)
    {
        Common_cJSON_DetachItemFromObject(pInParams, "Header");
        Common_cJSON_DetachItemFromObject(pInParams, "Data");
        Common_Json_Delete(pInParams);
        pInParams = NULL;
    }

    // 如果有返回的信息,则对其进行解析,是否有错误码.
    // 是否有"Data"部分,如果有则返回出去.

    if (pOutResults)
    {
        int ErrCode = 0;

        Common_Json_GetAttrValue(pOutResults,-1,"Header/Code",NULL,NULL,&ErrCode,NULL);

        if (ErrCode)
        {
            char *uri = NULL;
            Common_Json_GetAttrValueStr(header, "Uri", &uri);
            char *method = NULL;
            Common_Json_GetAttrValueStr(header, "Method", &method);
            LOGE("Access_CallFunctions reply error %d when (%s %s).\n", ErrCode, method, uri);
            if (0 == ret)
            {
                ret = ErrCode;
            }
        }


        if (OutParam != NULL)
        {
            *OutParam = Common_cJSON_DetachItemFromObject(pOutResults, "Data");
        }

        Common_Json_Delete(pOutResults);
        pOutResults = NULL;
    }

    return ret;
}

/*
 * 函数名称: ovfs_web_parse_ui_custom
 *
 * 函数描述: 解析用户界面定制的需求
 * @path: 保存用户定制界面json格式文件的路径
 * @pResult: 成功返回的json结构
 *
 * 返回值: 成功 0 失败 -1
 */
int ovfs_web_parse_ui_custom(const char *path, cJSON_Struct **pResult)
{
    int iRet = 0;
    char *pConfigStr = NULL;
    unsigned int fileSize = 0;
    FILE *fp;

    if ((path != NULL)&&(*path != '\0'))
    {
        fp = fopen(path, "rb");
        if (fp != NULL)
        {
            fseek(fp, 0, SEEK_END);
            fileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            if (fileSize > 0)
            {
                pConfigStr = (char *)Common_Malloc(fileSize, 0, __FUNCTION__, __LINE__);
                if (pConfigStr != NULL)
                {
                    if (fileSize == fread(pConfigStr, 1, fileSize, fp))
                    {
                        *pResult = Common_Json_Parse(pConfigStr, NULL, NULL);
                    }
                }
            }
        }
        fclose(fp);
    }

    if (pConfigStr != NULL)
    {
        Common_Free(pConfigStr, __FUNCTION__, __LINE__);
        pConfigStr = NULL;
    }
    if (*pResult == NULL)
    {
        iRet = -1;
    }

    return iRet;
}

// Web RRST API
void ovfs_web_rest_get_port(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    cJSON_Struct *pdata = NULL;

    pdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(pdata, -1, "httpPort", Common_Json_Type_Number, NULL, g_ovfs_web->httpport, g_ovfs_web->httpport);
    Common_Json_SetAttrValue(pdata, -1, "httpsPort", Common_Json_Type_Number, NULL, g_ovfs_web->httpsport, g_ovfs_web->httpsport);

    response->Data = pdata;

    return ;
}

void ovfs_web_rest_put_port(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    int i_num;
    int bchanged = 0;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pObj_check = NULL;

    if (request->Data)
    {
        pObj_check = Common_Json_GetAttrValue(request->Data, -1, "httpPort", NULL, NULL, &i_num, NULL);
        if ((pObj_check != NULL)&&(g_ovfs_web->httpport != i_num))
        {
            g_ovfs_web->httpport = i_num;
            pObj_tmp = Common_Json_GetAttrValue(g_ovfs_config, -1, "HttpPort", NULL, NULL, NULL, NULL);
            ((Common_cJSON_T *)pObj_tmp)->valueint = g_ovfs_web->httpport;
            bchanged = 1;
        }
        //HttpsPort
        pObj_check = Common_Json_GetAttrValue(request->Data, -1, "httpsPort", NULL, NULL, &i_num, NULL);
        if ((pObj_check != NULL)&&(g_ovfs_web->httpsport != i_num))
        {
            g_ovfs_web->httpsport = i_num;
            pObj_tmp = Common_Json_GetAttrValue(g_ovfs_config, -1, "HttpsPort", NULL, NULL, NULL, NULL);
            ((Common_cJSON_T *)pObj_tmp)->valueint = g_ovfs_web->httpsport;
            bchanged = 1;
        }
        if (1 == bchanged)
        {
            web_stop_nginx();
            generate_default_ngx_conf();
            web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);
            Access_SaveConfig(g_AccessHandle, g_ovfs_config);
        }

    }
    else
    {
        response->codeNum = -1;
    }

    return ;
}

void ovfs_web_rest_get_version(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    cJSON_Struct *pdata = NULL;
    if ((pdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) != NULL)
    {
        Common_Json_SetAttrValueStr(pdata, "Standard", OVFS_WEB_API_VERSION);
        Common_Json_SetAttrValueStr(pdata, "Build", QueryBuildString());
    }

    response->Data = pdata;

    return ;
}

void ovfs_web_rest_get_debug(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    cJSON_Struct *pdata = NULL;
    if ((pdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) != NULL)
    {
        Common_Json_SetAttrValueInt(pdata, "debugPrint", g_ovfs_web->debugPrint);
    }

    response->Data = pdata;

    return ;
}

void ovfs_web_rest_put_debug(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    if (request->Data)
    {
        int i_num;
        if (Common_Json_GetAttrValueInt(request->Data, "debugPrint", &i_num))
        {
            g_ovfs_web->debugPrint = i_num;
            //Common_Json_SetAttrValue(g_ovfs_config, -1, "debugPrint", NULL, NULL, NULL, NULL);
        }
    }
    else
    {
        response->codeNum = WEB_CODE_InvalidArg;
    }

    return ;
}

void ovfs_web_rest_get_customaction(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    LOGD("ovfs_web_rest_get_customaction enter!\n");

    Webs *wp;
    OVFS_WEB_OPTION_S opt = {0};

    if(response->codeNum == 0)
    {
        cJSON_Struct * header = Common_cJSON_CreateObject();
        cJSON_Struct * outdata = Common_cJSON_CreateObject();

        response->codeNum = frmHelp(wp, &opt, header, NULL, outdata);

        LOGD("response->codeNum:%d\n",response->codeNum);

        response->Data = outdata;

    }

    return ;
}


void ovfs_web_rest_put_customaction(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    //LOGD("ovfs_web_rest_put_customaction enter!\n");

    int i_num = 0;
    Webs wp;
    char *actionName = NULL;
    OVFS_WEB_OPTION_S opt = {0};
    cJSON_Struct *tmp = NULL;
    cJSON_Struct *header = NULL;
    cJSON_Struct *data = NULL;
    cJSON_Struct *indata = NULL;
    cJSON_Struct *outdata = Common_cJSON_CreateObject();


    initWebs(&wp, 1, 0);

    wp.method = strdup("POST");

    if (request->Data)
    {
        if (g_ovfs_web->debugPrint == 1)
        {
            char *str = Common_cJSON_Print(request->Data, NULL);
            printf("request->Data:%s\n",str);
            if(str)Common_cJSON_free(str);
        }

        if((tmp = Common_Json_GetAttrValueObj(request->Data, "Header")) == NULL)
        {
            response->codeNum = WEB_CODE_InvalidArg;
        }
        else
        {
            header = Common_Json_Duplicate(tmp, 1);
        }

        if((tmp = Common_Json_GetAttrValueObj(request->Data, "Data")) == NULL)
        {
            response->codeNum = WEB_CODE_InvalidArg;
        }
        else
        {
            data = Common_Json_Duplicate(tmp, 1);
        }

        if(response->codeNum == 0)
        {
            if (Common_Json_GetAttrValueInt(data, "Dev", &i_num))
            {

                if(i_num<=0)
                {
                    response->codeNum = WEB_CODE_InvalidArg;
                }
                else
                {
                    opt.dev = i_num-1;
                }
            }

            if (Common_Json_GetAttrValueInt(data, "Type", &i_num))
            {
                opt.type = i_num;
            }

            if (Common_Json_GetAttrValueInt(data, "Ch", &i_num))
            {
                if(i_num<=0)
                {
                    opt.ch = 0;
                }
                else
                {
                    opt.ch = i_num-1;
                }
            }

            if(Common_Json_GetAttrValueStr(header, "Uri", &actionName) == NULL)
            {
                response->codeNum = WEB_CODE_InvalidArg;
            }
        }

        if(response->codeNum == 0)
        {
            WebsKey *sp = NULL;
            WebsProc  web_action_proc;
            sp = hashLookup(actionTable, actionName);
            if (sp == NULL)
            {
                LOGE("Action %s is not defined\n", actionName);
                response->codeNum = WEB_CODE_Unsupported;
            }
            else
            {
                web_action_proc = (WebsProc)sp->content.value.symbol;
                indata = Common_Json_GetAttrValueObj(data, "Data");
                if(indata)
                {
                    response->codeNum = (*web_action_proc)(&wp, &opt, header, indata, outdata);

                    LOGD("response->codeNum:%d\n",response->codeNum);

                    response->Data = Common_Json_Duplicate(outdata, 1);
                    if (g_ovfs_web->debugPrint == 1)
                    {
                        char *str = Common_cJSON_Print(response->Data, NULL);
                        printf("response->Data:%s\n",str);
                        if(str)Common_cJSON_free(str);
                    }
                }
                else
                {
                    response->codeNum = WEB_CODE_InvalidArg;
                }
            }


        }
    }
    else
    {
        response->codeNum = WEB_CODE_InvalidArg;
    }

    if(outdata)
    {
        Common_Json_Delete(outdata);
        outdata = NULL;
    }

    if(header)
    {
        Common_Json_Delete(header);
        header = NULL;
    }

    if(data)
    {
        Common_Json_Delete(data);
        data = NULL;
    }

    termWebs(&wp, 1);

    return ;
}

void ovfs_web_rest_put_wifi(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    int i_num;
    int bchanged = 0;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pObj_check = NULL;

    if (request->Data)
    {
        int size = 0;
        int iloop = 0;
        WEB_SUBSCRIBE_NODE_T *subscribe_tmp = NULL;

        size = Common_DList_GetCount(g_ovfs_web->subscribeList);
        char *str = Common_Json_Print(request->Data, NULL);
        LOGW("size:[%d] str:[%s]\n",size, str);
        if(str)Common_Free(str,__FUNCTION__,__LINE__);

        for (iloop = 0; iloop < size; iloop ++)
        {
            subscribe_tmp = (WEB_SUBSCRIBE_NODE_T *)Common_DList_GetNode(g_ovfs_web->subscribeList, iloop);
            LOGW("[%d  Uri:%s  bFirst:%d  ToID:%d]\n", iloop, subscribe_tmp->uri, subscribe_tmp->bFirst, subscribe_tmp->toId);
            if (subscribe_tmp)
            {
                if (strstr(subscribe_tmp->uri, "Wifi"))
                {
                    Access_SendEvent(g_AccessHandle, subscribe_tmp->toId, request->Data, NULL, 0);
                }
            }
        }
    }
    else
    {
        response->codeNum = -1;
    }

    return ;
}

void ovfs_web_rest_get_deice_status(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    response->Data = Common_Json_Duplicate(g_ovfs_web->pDeviceStatus, 1);
    return ;
}

void ovfs_web_rest_put_deice_status(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    int i_num;
    int bchanged = 0;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pObj_check = NULL;

    if (request->Data)
    {
        int size = 0;
        int iloop = 0;

        ovfs_print_json(request->Data);
        if(Common_Json_GetAttrValueInt(request->Data, "SimId", &i_num))
        {
            Common_Json_SetAttrValueInt(request->Data, "SimId", i_num+1);
        }

        JsonOper_MergeObj(g_ovfs_web->pDeviceStatus, request->Data, 0);
        g_ovfs_web->need_send_devicestatus = 1;
    }
    else
    {
        response->codeNum = -1;
    }

    return ;
}

typedef void (*webRestFunc)(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response);

static struct
{
    char *url;
    char *describe;
    webRestFunc func[4]; // post, delete, put, get.
} s_webRestFunc[7] =
{
    {
        "Port", "Ports of webservice(http,https)",
        {NULL, NULL, ovfs_web_rest_put_port, ovfs_web_rest_get_port},
    },
    {
        "Version", "Query this web api version.",
        {NULL, NULL, NULL, ovfs_web_rest_get_version},
    },
    {
        "Debug", "Print log information for debuging.",
        {NULL, NULL, ovfs_web_rest_put_debug, ovfs_web_rest_get_debug},
    },
    {
        "CustomAction", "Put parameters to custon actions.",
        {NULL, NULL, ovfs_web_rest_put_customaction, ovfs_web_rest_get_customaction},
    },
    {
        "Wifi", "Forwarding WiFi Info.",
        {NULL, NULL, ovfs_web_rest_put_wifi, NULL},
    },
    {
        "DeviceStatus", "Forwarding Device Status.",
        {NULL, NULL, ovfs_web_rest_put_deice_status, ovfs_web_rest_get_deice_status},
    }
};

// Web RRST API
void ovfs_web_rest_get_help(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    int ret = 0;

    cJSON_Struct *pdata = NULL;
    cJSON_Struct *array = NULL;
    if (0 == ret)
    {
        if ((pdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL ||
                (array = Common_Json_SetAttrValueArr(pdata, "ResList")) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        int i;
        for (i = 0; i < sizeof(s_webRestFunc)/sizeof(s_webRestFunc[0]); i++)
        {
            if(!s_webRestFunc[i].url)continue;
            cJSON_Struct *arrayEach = Common_Json_SetAttrValueArrObj(array, i);
            char tempStr[256];
            snprintf(tempStr, sizeof(tempStr), "/Webserver/%s", s_webRestFunc[i].url);
            Common_Json_SetAttrValueStr(arrayEach, "Uri", tempStr);
            Common_Json_SetAttrValueStr(arrayEach, "Describe", s_webRestFunc[i].describe);
            cJSON_Struct *methodArray = Common_Json_SetAttrValueArr(arrayEach, "Method");
            int tempIndex = 0;
            if (s_webRestFunc[i].func[0])
            {
                Common_Json_SetAttrValueArrStr(methodArray, tempIndex++, "POST");
            }
            if (s_webRestFunc[i].func[1])
            {
                Common_Json_SetAttrValueArrStr(methodArray, tempIndex++, "DELETE");
            }
            if (s_webRestFunc[i].func[2])
            {
                Common_Json_SetAttrValueArrStr(methodArray, tempIndex++, "PUT");
            }
            if (s_webRestFunc[i].func[3] || smatch(s_webRestFunc[i].url, "Network"))
            {
                Common_Json_SetAttrValueArrStr(methodArray, tempIndex++, "GET");
            }
        }
    }

    response->Data = pdata;

    return ;
}

void ovfs_web_rest_dispatch(WEB_REST_FUNC_CALL_IN_T *request, WEB_REST_FUNC_CALL_OUT_T *response)
{
    int iRet = 0;

    if (0 == iRet)
    {
        if ((!request->method) || (!request->url))
        {
            response->codeNum = -1;
            iRet = -1;
        }
    }

    if (0 == iRet)
    {
        if (strncasecmp(request->url, "/Webserver", 10) != 0)
        {
            iRet = -1;
            response->codeNum = -1;
        }
    }

    int methodIndex = 0;
    if (0 == iRet)
    {
        if (strcasecmp(request->method, "POST") == 0)
        {
            methodIndex = 0;
        }
        else if (strcasecmp(request->method, "DELETE") == 0)
        {
            methodIndex = 1;
        }
        else if (strcasecmp(request->method, "PUT") == 0)
        {
            methodIndex = 2;
        }
        else if (strcasecmp(request->method, "GET") == 0)
        {
            methodIndex = 3;
        }
        else
        {
            response->codeNum = -9;
            LOGW("Has no such interface(%s %s).\n", request->method, request->url);
            iRet = -1;
        }
    }

    if (0 == iRet)
    {
        //response->url = strdup(request->url);
        char *url = request->url + 10;

        if (strcasecmp(request->method, "GET") == 0 && (url[0] == '\0' || (url[0] == '/' && url[1] == '\0')))
        {
            ovfs_web_rest_get_help(request, response);
        }
        else
        {
            url = request->url + 11;
            int i;
            for (i = 0; i < sizeof(s_webRestFunc)/sizeof(s_webRestFunc[0]); i++)
            {
                if (s_webRestFunc[i].url && strcasecmp(url, s_webRestFunc[i].url) == 0)
                {
                    if (s_webRestFunc[i].func[methodIndex] == NULL)
                    {
                        response->codeNum = -9;
                        LOGW("Has no such method(%s %s).\n", request->method, request->url);
                    }
                    else
                    {
                        s_webRestFunc[i].func[methodIndex](request, response);
                    }
                    break;
                }
            }

            if (i >= sizeof(s_webRestFunc)/sizeof(s_webRestFunc[0]))
            {
                response->codeNum = -9;
                LOGW("Has no such interface(%s %s).\n", request->method, request->url);
            }
        }

    }

    return ;
}

void web_list_string_nodefree(void *data)
{
    WEB_DLIST_STRING_T *node = (WEB_DLIST_STRING_T *)data;

    if (node->str)
    {
        Common_Free(node->str, __FUNCTION__, __LINE__);
        node->str = NULL;
    }
    Common_Free(node, __FUNCTION__, __LINE__);

    return ;
}

int web_list_string_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_DLIST_STRING_T* node = (WEB_DLIST_STRING_T*)a;
    char *tmp = (char *)b;

    if (tmp != NULL)
    {
        if (!strcmp(node->str, tmp))
        {
            iRet = 0;
        }
        else
        {
            iRet = -1;
        }
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}


// nonce
void web_nonce_nodefree(void* data)
{
    WEB_NONCE_NODE_T* node = (WEB_NONCE_NODE_T*)data;

    if(node->nonce)
    {
        Common_Free(node->nonce, __FUNCTION__, __LINE__);
        node->nonce = NULL;
    }
    Common_Free(data, __FUNCTION__, __LINE__);

    return;
}

int web_nonce_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_NONCE_NODE_T* node = (WEB_NONCE_NODE_T*)a;
    char *tmp = (char *)b;

    if ((tmp != NULL)&&(node->nonce != NULL))
    {
        iRet = Common_StrCmp(b, node->nonce);
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}

// subscribe
void web_subscribe_nodefree(void* data)
{
    WEB_SUBSCRIBE_NODE_T *node = (WEB_SUBSCRIBE_NODE_T*)data;

    if(node->uri)
    {
        Common_Free(node->uri, __FUNCTION__, __LINE__);
        node->uri = NULL;
    }
    Common_Free(node, __FUNCTION__, __LINE__);

    return;
}

int web_subscribe_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_SUBSCRIBE_NODE_T* node = (WEB_SUBSCRIBE_NODE_T*)a;
    int *tmp = (int *)b;

    if ((tmp != NULL) && (node->toId == *tmp))
    {
        iRet = 0;
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}

// LonginFailed
void web_loginfailed_nodefree(void* data)
{
    WEB_LOGINFAILED_NODE_T *node = (WEB_LOGINFAILED_NODE_T*)data;

    if(node->username)
    {
        Common_Free(node->username, __FUNCTION__, __LINE__);
        node->username = NULL;
    }

    if(node->ip)
    {
        Common_Free(node->ip, __FUNCTION__, __LINE__);
        node->ip = NULL;
    }
    Common_Free(node, __FUNCTION__, __LINE__);

    return;
}
// Longin Sucess
void web_loginsuccess_nodefree(void* data)
{
    WEB_LOGINSUCCESS_NODE_T *node = (WEB_LOGINSUCCESS_NODE_T*)data;

    Common_Free(node, __FUNCTION__, __LINE__);

    return;
}

int web_loginfailed_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_LOGINFAILED_NODE_T* node = (WEB_LOGINFAILED_NODE_T *)a;
    Webs *wp = (Webs *)b;

    if (wp && wp->ipaddr && wp->username && node && node->ip && node->username)
    {
        iRet = Common_StrCmp(wp->ipaddr, node->ip) || Common_StrCmp(wp->username, node->username);
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}
int web_loginsuccess_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_LOGINSUCCESS_NODE_T* node = (WEB_LOGINSUCCESS_NODE_T *)a;
    long sessionID = (long *)b;

    if (node && sessionID && node->session_id)
    {

        if(sessionID != node->session_id)
        {
            iRet =  -1;
        }

    }
    else
    {
        iRet = -1;
    }

    return iRet;
}

/*
int find_item_by_username(void *a, void *b)
{
    int iRet = 0;
    WEB_LOGINSUCCESS_NODE_T* node = (WEB_LOGINSUCCESS_NODE_T *)a;
    char* username = (char *)b;

    if (username && node  && node->username)
    {
        iRet =  Common_StrCmp(username, node->username);
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}
*/
// disk
void web_disk_nodefree(void* data)
{
    WEB_DISK_NODE_T* node = (WEB_DISK_NODE_T*)data;

    if (node->path)
    {
        Common_Free(node->path, __FUNCTION__, __LINE__);
        node->path = NULL;
    }
    if (node->disk_uri)
    {
        Common_Free(node->disk_uri, __FUNCTION__, __LINE__);
        node->disk_uri = NULL;
    }
    Common_Free(node, __FUNCTION__, __LINE__);

    return;
}

int web_disk_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_DISK_NODE_T* node = (WEB_DISK_NODE_T*)a;
    int *tmp = (int *)b;

    if ((tmp != NULL) && (node->diskNo == *tmp))
    {
        iRet = 0;
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}

// diskformat
void web_diskformat_nodefree(void* data)
{
    WEB_DISKFORMAT_NODE_T* node = (WEB_DISKFORMAT_NODE_T*)data;

    Common_Free(node, __FUNCTION__, __LINE__);

    return;
}

int web_diskformat_nodecompare(void *a, void *b)
{
    int iRet = 0;
    WEB_DISKFORMAT_NODE_T* node = (WEB_DISKFORMAT_NODE_T*)a;
    int *tmp = (int *)b;

    if ((tmp != NULL) && (node->diskNo == *tmp))
    {
        iRet = 0;
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}

int ovfs_web_alarmtime_trans2webA(cJSON_Struct *header, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt,cJSON_Struct *inParam)
{
    int iRet = 0;
    int iloop = 0;
    int nloop = 0;
    int jloop = 0;
    int i_num = 0;
    int i_val = 0;
	int val = 0;
    char pathname[128] = {0};
    cJSON_Struct *pArray_root2 = NULL;
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_tmp2 = NULL;
    cJSON_Struct *pResult = NULL;

    if (header == NULL || outdata == NULL)
    {
        iRet = -1;
    }

    if (0 == iRet)
    {
        iRet = Ovfs_Web_RestMethodA(header, inParam, &pResult, 60000);
        if (0 == iRet)
        {
            pArray_root2 = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            pArray_root = Common_Json_SetAttrValue(outdata, -1, "AlarmTime", Common_Json_Type_Array, NULL, 0, 0);

            i_num = Common_Json_ArraySize(pArray_root2);
            for(jloop = 0; jloop < i_num; ++jloop)
            {
                int iEnable = 1;
                Common_Json_GetAttrValue(pArray_root2, jloop, "Device", NULL, NULL, &val, NULL);
                if(val != opt->dev)continue;
                Common_Json_GetAttrValue(pArray_root2, jloop, "Channel", NULL, NULL, &val, NULL);
                if(val != opt->ch)continue;

                Common_Json_GetAttrValue(pArray_root2,jloop,"Enable", NULL,NULL,&iEnable,NULL);
                Common_Json_SetAttrValue(outdata, -1, "EnableHandle", Common_Json_Type_Number, NULL, iEnable, 0);

                // 获取星期一到星期六
                for (iloop = 1; iloop < 7; iloop ++)
                {
                    pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                    for (nloop=0; nloop<8; nloop++)
                    {
                        pArray_tmp2 = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);

	                    snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Start",jloop, iloop, nloop);
	                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_val, NULL);
	                    Common_Json_SetAttrValue(pArray_tmp2, 0, NULL, Common_Json_Type_Number, NULL, i_val/100, i_val/100);
	                    Common_Json_SetAttrValue(pArray_tmp2, 1, NULL, Common_Json_Type_Number, NULL, i_val%100, i_val%100);

	                    snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Stop",jloop, iloop, nloop);
	                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_val, NULL);
	                    Common_Json_SetAttrValue(pArray_tmp2, 2, NULL, Common_Json_Type_Number, NULL, i_val/100, i_val/100);
	                    Common_Json_SetAttrValue(pArray_tmp2, 3, NULL, Common_Json_Type_Number, NULL, i_val%100, i_val%100);

                        Common_Json_AddItem(pArray_tmp, nloop, NULL, pArray_tmp2);
                    }
                    Common_Json_AddItem(pArray_root, iloop - 1, NULL, pArray_tmp);
                }
                // 获取星期日
                {
                    pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                    for (nloop=0; nloop<8; nloop++)
                    {
                        pArray_tmp2 = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);

	                    snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Start",jloop, 0, nloop);
	                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_val, NULL);
	                    Common_Json_SetAttrValue(pArray_tmp2, 0, NULL, Common_Json_Type_Number, NULL, i_val/100, i_val/100);
	                    Common_Json_SetAttrValue(pArray_tmp2, 1, NULL, Common_Json_Type_Number, NULL, i_val%100, i_val%100);

	                    snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Stop",jloop, 0, nloop);
	                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_val, NULL);
	                    Common_Json_SetAttrValue(pArray_tmp2, 2, NULL, Common_Json_Type_Number, NULL, i_val/100, i_val/100);
	                    Common_Json_SetAttrValue(pArray_tmp2, 3, NULL, Common_Json_Type_Number, NULL, i_val%100, i_val%100);

                        Common_Json_AddItem(pArray_tmp, nloop, NULL, pArray_tmp2);
                    }
                    Common_Json_AddItem(pArray_root, 6, NULL, pArray_tmp);
                }

            }

        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return iRet;
}


int ovfs_web_alarmtime_trans2webA_extB(cJSON_Struct *header, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int iloop = 0;
    int nloop = 0;
    int i_num = 0;
    //int val = 0;
    char pathname[128] = {0};
    cJSON_Struct *pArray_root2 = NULL;
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_tmp2 = NULL;
    cJSON_Struct *pResult = NULL;

    if (header == NULL || outdata == NULL)
    {
        iRet = -1;
    }

    if (0 == iRet)
    {
        iRet = Ovfs_Web_RestMethodA(header, NULL, &pResult, 60000);
        if (0 == iRet)
        {
            //语音报警
            pArray_root2 = Common_Json_GetAttrValue(pResult, -1, "Shedule", NULL, NULL, NULL, NULL);
            pArray_root = Common_Json_SetAttrValue(outdata, -1, "AlarmTime", Common_Json_Type_Array, NULL, 0, 0);
            //i_num = Common_Json_ArraySize(pArray_root2);
            // 获取星期一到星期六
            for (iloop = 1; iloop < 7; iloop ++)
            {
                pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                for (nloop=0; nloop<8; nloop++)
                {
                    pArray_tmp2 = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);

                    snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Start", iloop, nloop);
                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);

                    Common_Json_SetAttrValue(pArray_tmp2, 0, NULL, Common_Json_Type_Number, NULL, i_num/100, 0);
                    Common_Json_SetAttrValue(pArray_tmp2, 1, NULL, Common_Json_Type_Number, NULL, i_num%100, 0);

                    snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Stop",iloop, nloop);
                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArray_tmp2, 2, NULL, Common_Json_Type_Number, NULL, i_num/100, 0);
                    Common_Json_SetAttrValue(pArray_tmp2, 3, NULL, Common_Json_Type_Number, NULL, i_num%100, 0);
                    Common_Json_AddItem(pArray_tmp, nloop, NULL, pArray_tmp2);
                }
                Common_Json_AddItem(pArray_root, iloop - 1, NULL, pArray_tmp);
            }
            // 获取星期日
            {
                pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                for (nloop=0; nloop<8; nloop++)
                {
                    pArray_tmp2 = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);

                    snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Start", 0, nloop);
                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArray_tmp2, 0, NULL, Common_Json_Type_Number, NULL, i_num/100, i_num/100);
                    Common_Json_SetAttrValue(pArray_tmp2, 1, NULL, Common_Json_Type_Number, NULL, i_num%100, i_num%100);

                    snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Stop", 0, nloop);
                    Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArray_tmp2, 2, NULL, Common_Json_Type_Number, NULL, i_num/100, i_num/100);
                    Common_Json_SetAttrValue(pArray_tmp2, 3, NULL, Common_Json_Type_Number, NULL, i_num%100, i_num%100);

                    Common_Json_AddItem(pArray_tmp, nloop, NULL, pArray_tmp2);
                }
                Common_Json_AddItem(pArray_root, 6, NULL, pArray_tmp);
            }

                if(Common_Json_GetAttrValueInt(pResult, "SnapInterval", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "SnapInterval", i_num);
                }

                if(Common_Json_GetAttrValueInt(pResult, "SnapStreamIndex", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "SnapStreamIndex", i_num);
                }

        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }
    return iRet;
}


int ovfs_web_alarmtime_trans2web_extA(cJSON_Struct *header, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt,cJSON_Struct *inParam)
{
    int iRet = 0;
    int iloop = 0;
    int nloop = 0;
    int jloop = 0;
    int i_num = 0;
    char pathname[128] = {0};
    int val = 0;
    cJSON_Struct *pArray_root2 = NULL;
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pResult = NULL;
    //

    if (0 == iRet)
    {
        iRet = Ovfs_Web_RestMethodA(header, inParam, &pResult, 60000);
        if (0 == iRet)
        {
            pArray_root2 = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            pArray_root = Common_Json_SetAttrValue(outdata, -1, "AlarmTime", Common_Json_Type_Array, NULL, 0, 0);


            for(jloop = 0; jloop < Common_Json_ArraySize(pArray_root2); ++jloop)
            {
                int iEnable = 1;
                Common_Json_GetAttrValue(pArray_root2, jloop, "Device", NULL, NULL, &val, NULL);
                if(val != opt->dev)continue;
                Common_Json_GetAttrValue(pArray_root2, jloop, "Channel", NULL, NULL, &val, NULL);
                if(val != opt->ch)continue;



                Common_Json_GetAttrValue(pArray_root2,jloop,"Enable", NULL,NULL,&iEnable,NULL);
                Common_Json_SetAttrValue(outdata, -1, "EnableHandle", Common_Json_Type_Number, NULL, iEnable, 0);

                // 获取星期一到星期六
                for (iloop = 1; iloop < 7; iloop ++)
                {
                    pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                    for (nloop=0; nloop<1; nloop++)
                    {
                        snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Start", jloop,iloop, nloop);
                        Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                        Common_Json_SetAttrValue(pArray_tmp, 0, NULL, Common_Json_Type_Number, NULL, i_num/100, i_num/100);
                        Common_Json_SetAttrValue(pArray_tmp, 1, NULL, Common_Json_Type_Number, NULL, i_num%100, i_num%100);

                        snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Stop",jloop, iloop, nloop);
                        Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                        Common_Json_SetAttrValue(pArray_tmp, 2, NULL, Common_Json_Type_Number, NULL, i_num/100, i_num/100);
                        Common_Json_SetAttrValue(pArray_tmp, 3, NULL, Common_Json_Type_Number, NULL, i_num%100, i_num%100);
                    }

                    Common_Json_AddItem(pArray_root, iloop - 1, NULL, pArray_tmp);
                }
                // 获取星期日
                {
                    pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                    for (nloop=0; nloop<1; nloop++)
                    {
                        snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Start",jloop, 0, nloop);
                        Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                        Common_Json_SetAttrValue(pArray_tmp, 0, NULL, Common_Json_Type_Number, NULL, i_num/100, i_num/100);
                        Common_Json_SetAttrValue(pArray_tmp, 1, NULL, Common_Json_Type_Number, NULL, i_num%100, i_num%100);

                        snprintf(pathname, sizeof(pathname), "ResList[%d].Weekday%d.Sched%d.Stop",jloop, 0, nloop);
                        Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, NULL, &i_num, NULL);
                        Common_Json_SetAttrValue(pArray_tmp, 2, NULL, Common_Json_Type_Number, NULL, i_num/100, i_num/100);
                        Common_Json_SetAttrValue(pArray_tmp, 3, NULL, Common_Json_Type_Number, NULL, i_num%100, i_num%100);
                    }

                    Common_Json_AddItem(pArray_root, 6, NULL, pArray_tmp);
                }
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return iRet;
}

int ovfs_web_alarmtime_trans2localA(cJSON_Struct *header, int device_index, int chanel_index, cJSON_Struct *indata,OVFS_WEB_OPTION_S *opt,cJSON_Struct *data)
{
    int iRet = 0;
    int nloop = 0;
    int iloop = 0;
    int i_num = 0;
    int i_num2 = 0;
    char pathname[128] = {0};
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pObj_root1 = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_tmp2 = NULL;

    if (0 == iRet)
    {
        /*       cJSON_Struct *data = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
               if (data == NULL)
               {
                   iRet = -1;
               }
               else*/
        {
            Common_Json_SetAttrValue(data, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(data, 0, "ResList.Device", Common_Json_Type_Number, NULL, device_index, 0);
            Common_Json_SetAttrValue(data, 0, "ResList.Channel", Common_Json_Type_Number, NULL, chanel_index, 0);

            int iEnable = 1;

            Common_Json_GetAttrValue(indata, -1, "EnableHandle", NULL, NULL, &iEnable, NULL);

            Common_Json_SetAttrValue(data, 0, "ResList.Enable", Common_Json_Type_Number, NULL, iEnable, 0);

            //AlarmTime
            pArray_root = Common_Json_GetAttrValue(indata, -1, "AlarmTime", NULL, NULL, NULL, NULL);
            // 设置星期日
            snprintf(pathname, sizeof(pathname), "ResList.Weekday%d", 0);
            pObj_root1 = Common_Json_SetAttrValue(data, 0, pathname, Common_Json_Type_Object, NULL, 0, 0);
            pArray_tmp = Common_Json_GetAttrValue(pArray_root, 6, NULL, NULL, NULL, NULL, NULL);
            for (nloop=0; nloop<8; nloop++)
            {
                snprintf(pathname, sizeof(pathname), "Sched%d", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);
                pArray_tmp2 = Common_Json_GetAttrValue(pArray_tmp, nloop, NULL, NULL, NULL, NULL, NULL);

                Common_Json_GetAttrValue(pArray_tmp2, 0, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp2, 1, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathname, sizeof(pathname), "Sched%d.Start", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                Common_Json_GetAttrValue(pArray_tmp2, 2, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp2, 3, NULL, NULL, NULL, &i_num2, NULL);
                snprintf(pathname, sizeof(pathname), "Sched%d.Stop", nloop);

                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
            }

            // 设置星期一到星期六
            for (iloop = 0; iloop < 6; iloop ++)
            {
                snprintf(pathname, sizeof(pathname), "ResList.Weekday%d", iloop + 1);
                pObj_root1 = Common_Json_SetAttrValue(data, 0, pathname, Common_Json_Type_Object, NULL, 0, 0);
                pArray_tmp = Common_Json_GetAttrValue(pArray_root, iloop, NULL, NULL, NULL, NULL, NULL);
                for (nloop=0; nloop<8; nloop++)
                {
                    snprintf(pathname, sizeof(pathname), "Sched%d", nloop);
                    Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);
                    pArray_tmp2 = Common_Json_GetAttrValue(pArray_tmp, nloop, NULL, NULL, NULL, NULL, NULL);

                    Common_Json_GetAttrValue(pArray_tmp2, 0, NULL, NULL, NULL, &i_num, NULL);
                    Common_Json_GetAttrValue(pArray_tmp2, 1, NULL, NULL, NULL, &i_num2, NULL);

                    snprintf(pathname, sizeof(pathname), "Sched%d.Start", nloop);
                    Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                    Common_Json_GetAttrValue(pArray_tmp2, 2, NULL, NULL, NULL, &i_num, NULL);
                    Common_Json_GetAttrValue(pArray_tmp2, 3, NULL, NULL, NULL, &i_num2, NULL);
                    snprintf(pathname, sizeof(pathname), "Sched%d.Stop", nloop);

                    Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                }
            }

            iRet = Ovfs_Web_RestMethodA(header, data, NULL, 60000);

            //     Common_Json_Delete(data);
            //     data = NULL;
        }
    }

    return iRet;
}

int ovfs_web_alarmtime_trans2local_extB(cJSON_Struct *header, int device_index, int chanel_index, cJSON_Struct *indata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int nloop = 0;
    int iloop = 0;
    int i_num = 0;
    int i_num2 = 0;
    char pathname[128] = {0};
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pObj_root1 = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_tmp2 = NULL;

    if (0 == iRet)
    {
        cJSON_Struct *data = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        if (data == NULL)
        {
            iRet = -1;
        }
        else
        {
            Common_Json_SetAttrValue(data, -1, "Shedule", Common_Json_Type_Object, NULL, 0, 0);
            //AlarmTime
            pArray_root = Common_Json_GetAttrValue(indata, -1, "AlarmTime", NULL, NULL, NULL, NULL);
            // 设置星期日
            snprintf(pathname, sizeof(pathname), "Shedule.Weekday%d", 0);
            pObj_root1 = Common_Json_SetAttrValue(data, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);
            pArray_tmp = Common_Json_GetAttrValue(pArray_root, 6, NULL, NULL, NULL, NULL, NULL);
            for (nloop=0; nloop<8; nloop++)
            {
                snprintf(pathname, sizeof(pathname), "Sched%d", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);
                pArray_tmp2 = Common_Json_GetAttrValue(pArray_tmp, nloop, NULL, NULL, NULL, NULL, NULL);

                Common_Json_GetAttrValue(pArray_tmp2, 0, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp2, 1, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathname, sizeof(pathname), "Sched%d.Start", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                Common_Json_GetAttrValue(pArray_tmp2, 2, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp2, 3, NULL, NULL, NULL, &i_num2, NULL);
                snprintf(pathname, sizeof(pathname), "Sched%d.Stop", nloop);

                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
            }

            // 设置星期一到星期六
            for (iloop = 0; iloop < 6; iloop ++)
            {
                snprintf(pathname, sizeof(pathname), "Shedule.Weekday%d", iloop + 1);
                pObj_root1 = Common_Json_SetAttrValue(data, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);
                pArray_tmp = Common_Json_GetAttrValue(pArray_root, iloop, NULL, NULL, NULL, NULL, NULL);
                for (nloop=0; nloop<8; nloop++)
                {
                    snprintf(pathname, sizeof(pathname), "Sched%d", nloop);
                    Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);
                    pArray_tmp2 = Common_Json_GetAttrValue(pArray_tmp, nloop, NULL, NULL, NULL, NULL, NULL);

                    Common_Json_GetAttrValue(pArray_tmp2, 0, NULL, NULL, NULL, &i_num, NULL);
                    Common_Json_GetAttrValue(pArray_tmp2, 1, NULL, NULL, NULL, &i_num2, NULL);

                    snprintf(pathname, sizeof(pathname), "Sched%d.Start", nloop);
                    Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                    Common_Json_GetAttrValue(pArray_tmp2, 2, NULL, NULL, NULL, &i_num, NULL);
                    Common_Json_GetAttrValue(pArray_tmp2, 3, NULL, NULL, NULL, &i_num2, NULL);
                    snprintf(pathname, sizeof(pathname), "Sched%d.Stop", nloop);

                    Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                }
            }

            if(Common_Json_GetAttrValueInt(indata, "SnapInterval", &i_num))
            {
                Common_Json_SetAttrValueInt(data, "SnapInterval", i_num);
            }

            if(Common_Json_GetAttrValueInt(indata, "SnapStreamIndex", &i_num))
            {
                Common_Json_SetAttrValueInt(data, "SnapStreamIndex", i_num);
            }

            iRet = Ovfs_Web_RestMethodA(header, data, NULL, 60000);
            Common_Json_Delete(data);
            data = NULL;
        }
    }

    return iRet;
}


int ovfs_web_alarmtime_trans2local_extA(cJSON_Struct *header, int device_index, int chanel_index, cJSON_Struct *indata,OVFS_WEB_OPTION_S *opt,cJSON_Struct *data)
{
    int iRet = 0;
    int nloop = 0;
    int iloop = 0;
    int i_num = 0;
    int i_num2 = 0;
    char pathname[128] = {0};
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pObj_root1 = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    /*
        cJSON_Struct *data = NULL;
        if (0 == iRet)
        {
            if ((data = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
            {
                iRet = -1;
            }
        }
    */
    if (0 == iRet)
    {
        Common_Json_SetAttrValue(data, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
        Common_Json_SetAttrValue(data, 0, "ResList.Device", Common_Json_Type_Number, NULL, device_index, 0);
        Common_Json_SetAttrValue(data, 0, "ResList.Channel", Common_Json_Type_Number, NULL, chanel_index, 0);
        //AlarmTime
        pArray_root = Common_Json_GetAttrValue(indata, -1, "AlarmTime", NULL, NULL, NULL, NULL);

        int iType = -1;
        int iEnable = 1;
        Common_Json_GetAttrValue(indata, -1, "DetectLinkType", NULL, NULL, &iType, NULL);
        //	if(iType == 7)
        //	{
        Common_Json_GetAttrValue(indata, -1, "EnableHandle", NULL, NULL, &iEnable, NULL);
        //	}
        Common_Json_SetAttrValue(data, 0, "ResList.Enable", Common_Json_Type_Number, NULL, iEnable, 0);

        // 设置星期日
        {
            snprintf(pathname, sizeof(pathname), "ResList.Weekday%d", 0);
            pObj_root1 = Common_Json_SetAttrValue(data, 0, pathname, Common_Json_Type_Object, NULL, 0, 0);
            pArray_tmp = Common_Json_GetAttrValue(pArray_root, 6, NULL, NULL, NULL, NULL, NULL);
            for (nloop=0; nloop<1; nloop++)
            {
                snprintf(pathname, sizeof(pathname), "Sched%d", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);

                Common_Json_GetAttrValue(pArray_tmp, 0, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp, 1, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathname, sizeof(pathname), "Sched%d.Start", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                Common_Json_GetAttrValue(pArray_tmp, 2, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp, 3, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathname, sizeof(pathname), "Sched%d.Stop", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
            }
        }
        // 设置星期一到星期六
        for (iloop = 1; iloop < 7; iloop ++)
        {
            snprintf(pathname, sizeof(pathname), "ResList.Weekday%d", iloop);
            pObj_root1 = Common_Json_SetAttrValue(data, 0, pathname, Common_Json_Type_Object, NULL, 0, 0);
            pArray_tmp = Common_Json_GetAttrValue(pArray_root, iloop - 1, NULL, NULL, NULL, NULL, NULL);
            for (nloop=0; nloop<1; nloop++)
            {
                snprintf(pathname, sizeof(pathname), "Sched%d", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Object, NULL, 0, 0);

                Common_Json_GetAttrValue(pArray_tmp, 0, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp, 1, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathname, sizeof(pathname), "Sched%d.Start", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
                Common_Json_GetAttrValue(pArray_tmp, 2, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp, 3, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathname, sizeof(pathname), "Sched%d.Stop", nloop);
                Common_Json_SetAttrValue(pObj_root1, -1, pathname, Common_Json_Type_Number, NULL, i_num*100+i_num2, i_num*100+i_num2);
            }
        }
        iRet = Ovfs_Web_RestMethodA(header, data, NULL, 60000);
    }

//    Common_Json_Delete(data);
//    data = NULL;

    return iRet;
}

int ovfs_web_value_to_index(int *arry, int arry_size, int value)
{
    int iloop = 0;
    int index = 0;

    if (arry&&(arry_size >= 0)&&(arry_size <= 10))
    {
        for (iloop = 0; iloop < arry_size; iloop ++)
        {
            //LOGW("[%d:%d]\n", value, *(arry + iloop));
            if (value == *(arry + iloop))
            {
                index= iloop;
                break;
            }
        }
        if (iloop == arry_size)
        {
            index = arry_size - 1;
        }
    }

    return index;
}

void ovfs_webt_init(OVFS_WEB_CONTEXT_T *web)
{
    if (web)
    {
#ifdef WEB_PRINT_DEBUG
        web_init_logs(web);
        web_set_test_logfile(web, "TEST.log");
        web_set_sent_logfile(web, "SENT.log");
        web_set_recv_logfile(web, "RECV.log");
#endif
        web->version = 1;
        web->debugPrint = 0;
        web->custom_ciphers = web->custom_support_protocols = NULL;
        web->enbale_http_redirect_to_https = 0;

        memset(&web->plugin_params,0,sizeof(OVFS_WEB_PLUGIN_PARAMS_T));
        web->plugin_params.prev_buf_val = 0;
        web->plugin_params.wm = 0;
        web->plugin_params.rec_file_format = 1;//avi
        web->maxCoolTime = 60;
        web->maxTryCount = 5;

        if(s_networkfuncCb.config != NULL)
        {
            int size = sizeof(s_webRestFunc)/sizeof(s_webRestFunc[0]);
            s_webRestFunc[size-1].url = "Network";
            s_webRestFunc[size-1].describe = "Transparent data transmission to [Network].";
        }
    }
}

int ovfs_web_write_log(cJSON_Struct *header, char *apiname, OVFS_WEB_OPTION_S *opt, char *extInfo)
{
    int iRet = 0;
    char *pRemoteIP = NULL;
    char *pUserName = NULL;
    cJSON_Struct *pConfig = NULL;
    cJSON_Struct *pArray = NULL;
    cJSON_Struct *pOutParams = NULL;
	static time_t lstCmd_t = 0;
	static U32 iLstMajorType = 0;
	static U32 iLstMinorType = 0;
	static S8 szLstLoginIP[64] = {0};

    int iMajorType = MAJOR_CONFIG_SET;
	int iMinorType = ANTS_DVR_SET_DEVICECFG;
    int iLogTime = time(NULL);

    Common_Json_GetAttrValueStr(header, "Auth/UserName", &pUserName);

    if(scaselessmatch(apiname, "(null)"))
    {
        LOGW("is (null) user!\n");
        return -1;
    }

    Common_Json_GetAttrValueStr(header, "ClientInfo/IPv4", &pRemoteIP);

    if(scaselessmatch(apiname, "frmUserLogin"))
    {
        iMajorType = MAJOR_OPERATION;
        iMinorType = MINOR_LOGIN;
    }
    else if(scaselessmatch(apiname, "frmUserLogout"))
    {
        iMajorType = MAJOR_OPERATION;
        iMinorType = MINOR_LOGOUT;
    }
    else if(scaselessmatch(apiname, "frmDeviceReboot"))
    {
        iMajorType = MAJOR_OPERATION;
        iMinorType = MINOR_REBOOT;
    }
    else if(scaselessmatch(apiname, "frmDeviceRestore_Simple"))
    {
        iMajorType = MAJOR_OPERATION;
        iMinorType = MINOR_RESTORE_SIMPE;
    }
    else if(scaselessmatch(apiname, "frmDeviceRestore_All"))
    {
        iMajorType = MAJOR_OPERATION;
        iMinorType = MINOR_RESTORE_ALL;
    }
    else if(scaselessmatch(apiname, "frmHDFormat") ||
            scaselessmatch(apiname, "frmIotSDFormat"))
    {
        iMajorType = MAJOR_OPERATION;
        iMinorType = MINOR_FORMAT_SD;
    }
    else if(scaselessmatch(apiname, "frmExpandAlarmOut"))
    {
        iMajorType = MAJOR_OPERATION;
        if(opt->type == 1 || opt->type == 4 || opt->type == 5 || opt->type == 6)
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_EXPANDALARMOUTCFG;
        }
        else if(opt->type == 2)
        {
            iMinorType = MINOR_EXPAND_ALARMOUT_TRIGGER;
        }
        else if(opt->type == 3)
        {
            iMinorType = MINOR_EXPAND_ALARMOUT_STOP;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        //CFG SET
        if(opt != NULL)
        {
            //LOGW("opt is not null\n");
            if(opt->type == 0)
            {
                //LOGW("opt type == 0\n");
                return -1;
            }
        }

        if(scaselessmatch(apiname, "frmDevicePara"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_DEVICECFG;
        }
        else if(scaselessmatch(apiname, "frmDeviceTimeCtrl") ||
            scaselessmatch(apiname, "frmNetNtpPara") ||
            scaselessmatch(apiname, "frmDstPara"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_TIMECFG;
        }
        else if(scaselessmatch(apiname, "frmAudioPara"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_AUDIOCFG;
        }
        else if(scaselessmatch(apiname, "frmSingleLineOSD") ||
            scaselessmatch(apiname, "frmMultiLineOSD"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_OSDCFG;
        }
        else if(scaselessmatch(apiname, "frmVideoParaEx") ||
            scaselessmatch(apiname, "frmVideoEffect"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_IMAGECFG;
        }
        else if(scaselessmatch(apiname, "frmVideoIPCSetPara"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_VIDEOENCODECFG;
        }
        else if(scaselessmatch(apiname, "frmIotLightCfg") ||
            scaselessmatch(apiname, "frmLightConfig"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_LIGHTCFG;
        }
        else if(scaselessmatch(apiname, "frmVideoShelterPara"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_PRVACYAREACFG;
        }
        else if(scaselessmatch(apiname, "frmMotionDetect") ||
            scaselessmatch(apiname, "frmMotionDetect_V2"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_MOTIONCFG;
        }
        else if(scaselessmatch(apiname, "frmVideoHide") ||
            scaselessmatch(apiname, "frmVideoHide_V2"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_HIDECFG;
        }
        else if(scaselessmatch(apiname, "frmRegionalInvasion"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_REGIONALINVASIONCFG;
        }
        else if(scaselessmatch(apiname, "frmTraverseDetect"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_TRAVERSEDETECTCFG;
        }
        else if(scaselessmatch(apiname, "frmPersonStaying"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_PERSONSTAYINGCFG;
        }
        else if(scaselessmatch(apiname, "frmPersonAbsent"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_PERSONABSENTCFG;
        }
        else if(scaselessmatch(apiname, "frmParkingViolation"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_PAKINGVIOLATIONCFG;
        }
        else if(scaselessmatch(apiname, "frmVehicleRetrograde"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_VEHICLERETOGRADECFG;
        }
        else if(scaselessmatch(apiname, "frmAlarmInPara") ||
            scaselessmatch(apiname, "frmAlarmInPara_V2"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_ALARMINCFG;
        }
        else if(scaselessmatch(apiname, "frmAlarmOut"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_ALARMOUTCFG;
        }
        else if(scaselessmatch(apiname, "frmIotRecordCfg"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_RECORDCFG;
        }
        else if(scaselessmatch(apiname, "frmNetworkSettings"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_NETWORKCFG;
        }
        else if(scaselessmatch(apiname, "frmGetManagerHostsPara") ||
            scaselessmatch(apiname, "frmParaPlatform28181"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_GB28181CFG;
        }
        else if(scaselessmatch(apiname, "frmEmailSetting"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_EMAILCFG;
        }
        else if(scaselessmatch(apiname, "frmFTPSetting"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_FTPCFG;
        }
        else if(scaselessmatch(apiname, "frmRtspCfg"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_RTSPCFG;
        }
        else if(scaselessmatch(apiname, "frmRtmpPushCfg"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_RTMPCFG;
        }
        else if(scaselessmatch(apiname, "frmUserManage") ||
            scaselessmatch(apiname, "frmUserRights_V2"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_USERCFG;
        }
        else if(scaselessmatch(apiname, "frmAutoReboot"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_AUTOREBOOTCFG;
        }
        else if(scaselessmatch(apiname, "frmUartConfig"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_UARTCFG;
        }
        else if(scaselessmatch(apiname, "frmSensorAlarm"))
        {
            iMajorType = MAJOR_CONFIG_SET;
            iMinorType = MINOR_SET_SENSORALASRMCFG;
        }
        else
        {
            return -1;
        }
    }

    LOGW("Last:%d %d %d [%s]\n",lstCmd_t,iLstMajorType,iLstMinorType,szLstLoginIP);
    LOGW("Cur :%d %d %d [%s] [%s]\n",iLogTime,iMajorType,iMinorType,pRemoteIP,apiname);

    if(iLogTime - lstCmd_t < 2)
    {
        if(0 == Common_StrCmp(pRemoteIP, szLstLoginIP) && (iMajorType == iLstMajorType) && (iMinorType == iLstMinorType))
        {
            LOGW("same log! return\n");
            return -1;
        }
    }

    if(pRemoteIP)
    {
        snprintf(szLstLoginIP,sizeof(szLstLoginIP),"%s",pRemoteIP);
    }

    iLstMajorType = iMajorType;
    iLstMinorType= iMinorType;
    lstCmd_t = iLogTime;

    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig)
    {
        Common_Json_SetAttrValueInt(pConfig, "LogTime", iLogTime);
        Common_Json_SetAttrValueInt(pConfig, "MajorType", iMajorType);
        Common_Json_SetAttrValueInt(pConfig, "MinorType", iMinorType);
        Common_Json_SetAttrValueStr(pConfig, "UserName", pUserName?pUserName:"");
        Common_Json_SetAttrValueStr(pConfig, "IP", pRemoteIP?pRemoteIP:"");
        Common_Json_SetAttrValueInt(pConfig, "Channel", opt->ch+1);
        Common_Json_SetAttrValueStr(pConfig, "ExtraInfo", extInfo?extInfo:"");
/*        pArray = Common_Json_SetAttrValueArr(pConfig, "ResList");

        Common_Json_SetAttrValue(pArray,0,"LogTime",Common_Json_Type_Number,NULL,iLogTime,0);
        Common_Json_SetAttrValue(pArray,0,"MajorType",Common_Json_Type_Number,NULL,iMajorType,0);
        Common_Json_SetAttrValue(pArray,0,"MinorType",Common_Json_Type_Number,NULL,iMinorType,0);
        Common_Json_SetAttrValue(pArray,0,"IP",Common_Json_Type_String,pRemoteIP,0,0);
        Common_Json_SetAttrValue(pArray,0,"IP",Common_Json_Type_String,pRemoteIP,0,0);
        Common_Json_SetAttrValue(pArray,0,"Channel",Common_Json_Type_Number,NULL,opt->ch+1,0);
*/

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/EventLog/LogFunction");
        iRet = Ovfs_Web_RestMethodA(header, pConfig, &pOutParams, 60000);

        Common_Json_Delete(pConfig);
        Common_Json_Delete(pOutParams);
        pConfig = NULL;
        pOutParams = NULL;
    }

    return iRet;
}
