//#include "upload.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>

#include "AntsWebCommon.h"
#include "web_inter_api.h"
#include "ovfs_web_rest.h"

int g_videoformat_weight;
int g_videoformat_height;

extern uint strtoi(char *s);
static const char s_optJsonDefault[] = "{\"Ch\":1,\"Type\":0,\"Data\":{}}";

/*New*/
int web_init_global_website_info()
{
    int iRet = 0;

    g_struWebSiteSDKInfo.g_ChanNum = 1;

    return iRet;
}

static int web_semantic_auth_tryclear(webs_t wp, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    const char *queryString = wp->queryString;
    const char *pathlast = wp->pathlast;

    char *begin = NULL;
    if (strcmp(pathlast, "frmCapture") == 0 || strcmp(pathlast, "frmGetFactoryInfo") == 0 ||
        strcmp(wp->pathlast, "frmGetConfigFile") == 0 || strcmp(wp->pathlast, "frmSetConfigFile") == 0 ||
        strcmp(wp->pathlast, "frmWebApiVersion") == 0 || strcmp(wp->pathlast, "frmHelp") == 0 ||
        strcmp(wp->pathlast, "frmPasswordLost") == 0  || strcmp(wp->pathlast, "frmUploadInfo") == 0 ||
        strcmp(wp->pathlast, "frmLocalSettings") == 0 || strcmp(wp->pathlast, "frmSwitchSTAPara") == 0)
    {
        // 这些接口不需要验证密码.
    }
    else if (queryString == NULL || (begin = strstr(queryString, "s_username=")) == NULL)
    {
        ret = -1;
    }
    else
    {
        char *username = NULL;
        char *password = NULL;
        char *end = NULL;

        begin += strlen("s_username=");
        for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);
        if (end > begin)
        {
            username = strndup(begin, (int)(end - begin));
        }

        begin = strstr(queryString, "s_psw=");
        if (begin)
        {
            begin += strlen("s_psw=");
            for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);

            if (end > begin)
            {
                password = strndup(begin, end - begin);
            }
        }

        if (username)
        {
            if (wp->username)
            {
                wfree(wp->username);
            }
            wp->username = username;
        }
        if (password)
        {
            if (wp->password)
            {
                wfree(wp->password);
            }
            wp->password = password;
        }
    }

    return ret;
}

static int web_semantic_auth_trytoken(webs_t wp)
{
    int ret = 0;
    const char *queryString = wp->queryString;

    char *begin = NULL;

    if (queryString == NULL || (begin = strstr(queryString, "Username=")) == NULL)
    {
        ret = -1;
    }
    else
    {
        char *username = NULL;
        char *password = NULL;
        char *created = NULL;
        char *nonce = NULL;
        char *end = NULL;

        begin += strlen("Username=");
        for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);
        if (end > begin)
        {
            username = strndup(begin, end - begin);
        }

        if ((begin = strstr(queryString, "PasswordDigest=")))
        {
            begin += strlen("PasswordDigest=");
            for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);

            if (end > begin)
            {
                password = strndup(begin, end - begin);
            }
        }

        if ((begin = strstr(queryString, "Created=")))
        {
            begin += strlen("Created=");
            for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);

            if (end > begin)
            {
                created = strndup(begin, end - begin);
            }
        }

        if ((begin = strstr(queryString, "Nonce=")))
        {
            begin += strlen("Nonce=");
            for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);

            if (end > begin)
            {
                nonce = strndup(begin, end - begin);
            }
        }

        if (username)
        {
            if (wp->username)
            {
                free(wp->username);
            }
            wp->username = username;
        }
        if (password)
        {
            if (wp->wsse_passwordDigest)
            {
                free(wp->wsse_passwordDigest);
            }
            wp->wsse_passwordDigest = password;
        }
        if (created)
        {
            if (wp->wsse_created)
            {
                free(wp->wsse_created);
            }
            wp->wsse_created = created;
        }
        if (nonce)
        {
            if (wp->wsse_nonce)
            {
                free(wp->wsse_nonce);
            }
            wp->wsse_nonce = nonce;
        }
    }

    return ret;
}

static int web_semantic_auth_trydigest(webs_t wp)
{
    int ret = 0;

    if (0 == ret)
    {
        WebsRoute *route = wp->route;
        if (wp->authDetails && route->parseAuth)
        {
            if (!(route->parseAuth)(wp))
            {
                wp->username = NULL;
            }
        }
        if ((NULL == wp->username) || (*wp->username == '\0'))
        {
            if (route->askLogin)
            {
                (route->askLogin)(wp);
            }
            websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
            ret = WEB_CODE_Unauthorized;
        }
        if ((0 == ret) && (wp->username != NULL) && (*wp->username != '\0'))
        {
            //check nonce in Nonce List
            WEB_NONCE_NODE_T *node_tmp = (WEB_NONCE_NODE_T *)Common_DList_Search(g_ovfs_web->pNonceList, wp->nonce, web_nonce_nodecompare);
            if (node_tmp)
            {
                Common_DList_Delete(g_ovfs_web->pNonceList, wp->nonce, web_nonce_nodecompare);
            }
            else
            {
                if (route->askLogin)
                {
                    (route->askLogin)(wp);
                }
                websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
                ret = WEB_CODE_Unauthorized;
            }
        }
    }

    return ret;
}

static int web_semantic_auth_makeheader(webs_t wp, int authType, cJSON_Struct *headerThis)
{
    int ret = 0;

    if (0 == ret)
    {
    	Common_Json_SetAttrValue(headerThis, -1, "IsRemote", Common_Json_Type_Number, NULL, 1, 0);

        cJSON_Struct * headerAuth = Common_Json_SetAttrValue(headerThis, -1, "Auth", Common_Json_Type_Object, NULL, 0, 0);
        if (headerAuth == NULL)
        {
            LOGW("Failed set object.\n");
        }
        else
        {
        	Common_Json_SetAttrValue(headerAuth, -1, "Method", Common_Json_Type_Number, NULL, authType, 0);
        	Common_Json_SetAttrValue(headerAuth, -1, "UserName", Common_Json_Type_String, wp->username?wp->username:"", 0, 0);

            if(slen(wp->session_id)){
                Common_Json_SetAttrValue(headerAuth, -1, "SessionId", Common_Json_Type_Number, NULL, strtoi(wp->session_id), 0);
            }
            Common_Json_SetAttrValue(headerAuth, -1, "Path", Common_Json_Type_String, wp->pathlast, 0, 0);
        }

        if (authType == 0 || authType == 1)
        {
            // 明文认证方式
        	Common_Json_SetAttrValue(headerAuth, -1, "Password", Common_Json_Type_String, wp->password, 0, 0);
        }
        else if (authType == 2)
        {
            // digest认证方式
            cJSON_Struct * headerAuthDigest = Common_Json_SetAttrValue(headerAuth, -1, "Digest", Common_Json_Type_Object, NULL, 0, 0);
            if (headerAuthDigest == NULL)
            {
                LOGW("Failed set object.\n");
            }
            else
            {
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Realm", Common_Json_Type_String, wp->realm?wp->realm:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Qop", Common_Json_Type_String, wp->qop?wp->qop:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Nonce", Common_Json_Type_String, wp->nonce?wp->nonce:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Cnonce", Common_Json_Type_String, wp->cnonce?wp->cnonce:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Uri", Common_Json_Type_String, wp->digestUri?wp->digestUri:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Response", Common_Json_Type_String, wp->password?wp->password:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Opaque", Common_Json_Type_String, wp->opaque?wp->opaque:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Nc", Common_Json_Type_String, wp->nc?wp->nc:"", 0, 0);
            	Common_Json_SetAttrValue(headerAuthDigest, -1, "Method", Common_Json_Type_String, wp->method?wp->method:"", 0, 0);
            }
        }
        else if (authType == 3)
        {
            // token认证方式
        	cJSON_Struct * headerAuthUsertok = Common_Json_SetAttrValue(headerAuth, -1, "UsernameToken", Common_Json_Type_Object, NULL, 0, 0);
            if (headerAuthUsertok == NULL)
            {
                LOGW("Failed set object.\n");
            }
            else
            {
                Common_Json_SetAttrValue(headerAuthUsertok, -1, "Nonce", Common_Json_Type_String, wp->wsse_nonce?wp->wsse_nonce:"", 0, 0);
                Common_Json_SetAttrValue(headerAuthUsertok, -1, "Created", Common_Json_Type_String, wp->wsse_created?wp->wsse_created:"", 0, 0);
                Common_Json_SetAttrValue(headerAuthUsertok, -1, "PasswordDigest", Common_Json_Type_String, wp->wsse_passwordDigest?wp->wsse_passwordDigest:"", 0, 0);
            }
        }
        else
        {
            // 不会进入的分支.
        }

    	cJSON_Struct * headerClient = Common_Json_SetAttrValue(headerThis, -1, "ClientInfo", Common_Json_Type_Object, NULL, 0, 0);
        if (headerAuth == NULL)
        {
            LOGW("Failed set object.\n");
        }
        else
        {
    	    Common_Json_SetAttrValue(headerClient, -1, "IPv4", Common_Json_Type_String, wp->ipaddr?wp->ipaddr:"", 0, 0);
        }
    }

    return ret;
}

int web_semantic_authA(webs_t wp, cJSON_Struct **header, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;

    if (header == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    int authType = 0; //0,1-明文认证; 2-digest认证; 3-token认证.
    if (0 == ret)
    {
        if (web_semantic_auth_tryclear(wp, opt) == 0)
        {
            authType = 0;
        }
        else if (web_semantic_auth_trytoken(wp) == 0)
        {
            authType = 3;
        }
        else if (!strcmp(wp->pathlast, "upload"))
        {
            authType = 3;
        }
    	else
    	{
            authType = 2;
            ret = web_semantic_auth_trydigest(wp);
    	}
    }

    cJSON_Struct * headerThis = NULL;
    if (0 == ret)
    {
        headerThis = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (headerThis == NULL)
        {
            ret = WEB_CODE_LackingMem;
            LOGE("Failed to make header.\n");
        }
    }

    if (0 == ret)
    {
        ret = web_semantic_auth_makeheader(wp, authType, headerThis);
    }

    if (0 == ret)
    {
        *header = headerThis;
    }

    return ret;
}

void web_semantic_auth_freeA(cJSON_Struct *header)
{
    Common_Json_Delete(header);
}

int web_semantic_parse_json(webs_t wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct **p_data)
{
    int ret = 0;

    if (wp == NULL || opt == NULL || p_data == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    // 如果请求中遗漏了这部分,按照默认内容补全.
    if (wp->query == NULL)
    {
        wp->query = strdup(s_optJsonDefault);
    }

    cJSON_Struct *p_json_local = NULL;
    if (0 == ret)
    {
        p_json_local = Common_cJSON_Parse(wp->query, NULL, NULL);
        if ((NULL == p_json_local))
        {
            // 有时query不是json文本而可能是二进制文本,如导入配置的时候.
            //ret = WEB_CODE_LackingMem;
            p_json_local = Common_cJSON_Parse(s_optJsonDefault, NULL, NULL);
        }
        else if ((((Common_cJSON_T *)p_json_local)->type) != Common_cJSON_Object)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        int valueInt;
        if (Common_Json_GetAttrValueInt(p_json_local, TYPE, &valueInt))
        {
            //ret = WEB_CODE_InvalidArg;
            opt->type = valueInt;
        }
        if (Common_Json_GetAttrValueInt(p_json_local, DEV, &valueInt))
        {
            //ret = WEB_CODE_InvalidArg;
            if(valueInt <= 0){
				ret = WEB_CODE_InvalidArg;
			}else{
            	opt->dev = valueInt - 1;
			}
        }else{
			opt->dev = 0;
		}
        if (Common_Json_GetAttrValueInt(p_json_local, CH, &valueInt))
        {
            //ret = WEB_CODE_InvalidArg;

			if(valueInt <= 0){
				opt->ch = 0;
			}else{
            	opt->ch = valueInt - 1;
			}
        }else{
			opt->ch = 0;
		}
        if ((*p_data = Common_Json_GetAttrValueObj(p_json_local, DATA)) == NULL)
        {
            //ret = WEB_CODE_InvalidArg;
            *p_data = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        }
        else
        {
            Common_Json_DetachItem(p_json_local, -1, DATA);
        }
    }

    Common_Json_Delete(p_json_local);
    p_json_local = NULL;

    return ret;
}

/*
 * 函数名称: web_semantic_func_end
 *
 * 函数描述: 响应客户端程序的响应,响应格式的形式为{"Result": retCode,"Data":{retData or errorString}}
 * @wp 当前连接的context内容
 * @retCode
 * @retObj
 * @retData
 * @errorString
 *
 *
 */
int web_semantic_func_endA(webs_t wp, int retCode, Common_cJSON_T *retData)
{
    int iRet = 0;
    char *data_mtemp = NULL;

    Common_cJSON_T *retObj = Common_cJSON_CreateObject();
    if (retObj == NULL)
    {
        iRet = WEB_CODE_LackingMem;
    }

    if (0 == iRet)
    {
        Common_Json_SetAttrValueInt(retObj, RESULT, retCode);

        int extDataSize = 0;
        char *extData = NULL;
        if (retCode == 0)
        {
            extData = Common_Json_GetItemExtData(retData, &extDataSize);
            if (extData == NULL)
            {
                Common_Json_AddItem(retObj, -1, DATA, retData);
            }
        }
        else
        {
            char *errorStr = NULL;
            if (Common_Json_GetAttrValueStr(retData, ERROR_STRING, &errorStr) == NULL)
            {
                Common_Json_SetAttrValueStr(retObj, ERROR_STRING, Ovfs_Web_StrError(retCode));
            }
            else
            {
                Common_Json_SetAttrValueStr(retObj, ERROR_STRING, errorStr);
            }
        }

        if (extData == NULL)
        {
            data_mtemp = Common_cJSON_PrintUnformatted(retObj, NULL);
            if (retCode == 0)
            {
                Common_Json_DetachItem(retObj, -1, DATA);
            }


            websResponse(wp, retCode == ACCESS_ERROR_TYPE_AUTH || retCode == WEB_CODE_Unauthorized ? 401 : 200, NULL);

            websWriteBlock(wp, data_mtemp, strlen(data_mtemp));
            Common_cJSON_free(data_mtemp);
        }
        else
        {
            websResponse(wp, retCode == ACCESS_ERROR_TYPE_AUTH || retCode == WEB_CODE_Unauthorized ? 401 : 200, NULL);

            websWriteBlock(wp, extData, extDataSize);
        }
    }

    if(retObj)
    {
        Common_cJSON_Delete(retObj);
        retObj = NULL;
    }

    return iRet;
}

S32 web_ReleaseAlarmStatus(AntsHostMgrLibALarmStatusInfo_T *pAlarmStatus)
{
	int iRet = 0;

	return iRet;
}

S32 web_QueryFile(S32 hLogin, S32 lChannel, AntsHostMgrLibQueryFile_T *pQuery)
{
	int iRet = 0;

	return iRet;
}

S32 web_GetQueryFileResult(S32 hLogin,S32 lIndex,AntsHostMgrLibQueryFileResult_T *pResult,U32 dwResultCount)
{
	int iRet = 0;

	return iRet;
}

S32 web_CapturePicture(S32 hLogin,S32 lChannel,S32 nStreamIdx, S8 *sPicBuffer, U32 dwBufferSize, U32 *lpReturnSize)
{
	int iRet = 0;

	return iRet;
}

S32 web_QuickResponseCode(S8 *pString,AntsHostMgrLibQRCode_T *pPixelInfo)
{
	int iRet = 0;

	return iRet;
}

int web_action_prepare(webs_t wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct **header, cJSON_Struct **indata, cJSON_Struct **outdata)
{
    int ret = 0;

    cJSON_Struct *temp = NULL;
    if (0 == ret)
    {
        ret = web_semantic_parse_json(wp, opt, &temp);
        if (ret == 0)
        {
            if (indata)
            {
                *indata = temp;
            }
        }
    }

    if (0 == ret)
    {
        ret = web_semantic_authA(wp, &temp, opt);
        if (ret == 0)
        {
            if (header)
            {
                *header = temp;
            }
        }
    }

    if (0 == ret)
	{
        temp = Common_cJSON_CreateObject();
        if (temp == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            if (outdata)
            {
                *outdata = temp;
            }
        }
    }

    return ret;
}

int web_action_clean(webs_t wp, int retCode, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    web_semantic_func_endA(wp, retCode, outdata);

    Common_Json_Delete(header);

    Common_Json_Delete(indata);

    Common_Json_Delete(outdata);

    return 0;
}

#if OLD_VERSION
/*Semantic Interface Start*/
/*
 * 函数名: web_semantic_auth
 *
 * 函数描述: 填充digest的认证信息
 *
 * 返回值: 成功 0 失败 -1
 */
int web_semantic_auth(webs_t wp, WEB_REST_INPARAM_T *inparam)
{
    int iRet = 0;
    int isWhiteNames = 0;
    WebsRoute *route = NULL;
    WEB_NONCE_NODE_T *node_tmp = NULL;

    memset(inparam, 0, sizeof(WEB_REST_INPARAM_T));
    route = wp->route;

    if(strstr(wp->path, "Capture") || strstr(wp->path, "frmSetConfigFile")
        	                 || strstr(wp->path, "frmGetConfigFile"))
    {
       isWhiteNames = 1;
    }
    else if (strstr(wp->path, "upload"))
    {
       isWhiteNames = 2;    //使用UserNameToken认证
    }
	else
	{
       isWhiteNames = 0;    //使用Digest认证
    }

    if (0 == isWhiteNames)
    {
        if (wp->authDetails && route->parseAuth)
        {
            if (!(route->parseAuth)(wp))
            {
                wp->username = NULL;
            }
        }
        if ((NULL == wp->username) || (*wp->username == '\0'))
        {
            if (route->askLogin)
            {
                (route->askLogin)(wp);
            }
            websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
            iRet = -1;
        }
        if ((0 == iRet) && (wp->username != NULL) && (*wp->username != '\0'))
        {
            //check nonce in Nonce List
            node_tmp = (WEB_NONCE_NODE_T *)Common_DList_Search(g_ovfs_web->pNonceList, wp->nonce, web_nonce_nodecompare);
            if (node_tmp)
            {
                Common_DList_Delete(g_ovfs_web->pNonceList, wp->nonce, web_nonce_nodecompare);

                inparam->Auth.Type = 2; // 1-明文 2-digest
                inparam->Auth.UserName = Common_StrDup(wp->username?wp->username:"", __FUNCTION__, __LINE__);
                inparam->Auth.Password = Common_StrDup("", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Realm = Common_StrDup(wp->realm?wp->realm:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Nonce = Common_StrDup(wp->nonce?wp->nonce:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Uri = Common_StrDup(wp->digestUri?wp->digestUri:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Qop = Common_StrDup(wp->qop?wp->qop:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Nc = Common_StrDup(wp->nc?wp->nc:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Cnonce = Common_StrDup(wp->cnonce?wp->cnonce:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Response = Common_StrDup(wp->password?wp->password:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Opaque = Common_StrDup(wp->opaque?wp->opaque:"", __FUNCTION__, __LINE__);
                inparam->Auth.digest.Method = Common_StrDup(wp->method?wp->method:"", __FUNCTION__, __LINE__);
            }
            else
            {
                if (route->askLogin)
                {
                    (route->askLogin)(wp);
                }
                websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
                iRet = -1;
            }
        }
    }
    else if (2 == isWhiteNames)
    {
        inparam->Auth.Type = 3; // 1-明文 2-digest 3-UserNameToken
        inparam->Auth.UserName = Common_StrDup(wp->username?wp->username:"", __FUNCTION__, __LINE__);
        inparam->Auth.Password = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.userToken.created = Common_StrDup(wp->wsse_created?wp->wsse_created:"", __FUNCTION__, __LINE__);
        inparam->Auth.userToken.nonce = Common_StrDup(wp->wsse_nonce?wp->wsse_nonce:"", __FUNCTION__, __LINE__);
        inparam->Auth.userToken.passwdDigest = Common_StrDup(wp->wsse_passwordDigest?wp->wsse_passwordDigest:"", __FUNCTION__, __LINE__);
    }
    else
    {
        inparam->Auth.Type = 0; // 1-明文 2-digest
        inparam->Auth.UserName = Common_StrDup("admin", __FUNCTION__, __LINE__);
        inparam->Auth.Password = Common_StrDup("123456", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Realm = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Nonce = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Uri = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Qop = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Nc = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Cnonce = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Response = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Opaque = Common_StrDup("", __FUNCTION__, __LINE__);
        inparam->Auth.digest.Method = Common_StrDup("", __FUNCTION__, __LINE__);
    }
    snprintf(inparam->IpAddr.Ipv4, sizeof(inparam->IpAddr.Ipv4), "%s", wp->ipaddr?wp->ipaddr:"");

    return iRet;
}

void web_semantic_auth_free(WEB_REST_INPARAM_T *inparam)
{
    if (inparam->Auth.UserName)
        Common_Free(inparam->Auth.UserName, __FUNCTION__, __LINE__);
    inparam->Auth.UserName = NULL;
    if (inparam->Auth.Password)
        Common_Free(inparam->Auth.Password, __FUNCTION__, __LINE__);
    inparam->Auth.Password = NULL;
    if (inparam->Auth.digest.Realm)
        Common_Free(inparam->Auth.digest.Realm, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Realm = NULL;
    if (inparam->Auth.digest.Nonce)
        Common_Free(inparam->Auth.digest.Nonce, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Nonce = NULL;
    if (inparam->Auth.digest.Uri)
        Common_Free(inparam->Auth.digest.Uri, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Uri = NULL;
    if (inparam->Auth.digest.Qop)
        Common_Free(inparam->Auth.digest.Qop, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Qop = NULL;
    if (inparam->Auth.digest.Nc)
        Common_Free(inparam->Auth.digest.Nc, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Nc = NULL;
    if (inparam->Auth.digest.Cnonce)
        Common_Free(inparam->Auth.digest.Cnonce, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Cnonce = NULL;
    if (inparam->Auth.digest.Response)
        Common_Free(inparam->Auth.digest.Response, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Response = NULL;
    if (inparam->Auth.digest.Opaque)
        Common_Free(inparam->Auth.digest.Opaque, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Opaque = NULL;
    if (inparam->Auth.digest.Method)
        Common_Free(inparam->Auth.digest.Method, __FUNCTION__, __LINE__);
    inparam->Auth.digest.Method = NULL;
    if (inparam->Auth.userToken.created)
        Common_Free(inparam->Auth.userToken.created, __FUNCTION__, __LINE__);
    inparam->Auth.userToken.created = NULL;
    if (inparam->Auth.userToken.nonce)
        Common_Free(inparam->Auth.userToken.nonce, __FUNCTION__, __LINE__);
    inparam->Auth.userToken.nonce = NULL;
    if (inparam->Auth.userToken.passwdDigest)
        Common_Free(inparam->Auth.userToken.passwdDigest, __FUNCTION__, __LINE__);
    inparam->Auth.userToken.passwdDigest = NULL;
}

int web_semantic_func_end(webs_t wp, int retCode, Common_cJSON_T *retData, char *errorString)
{
    int iRet = 0;
    char *data_mtemp = NULL;

    Common_cJSON_T *retObj = Common_cJSON_CreateObject();
    if (retObj == NULL)
    {
        iRet = -1;
    }

    if (0 == iRet)
    {
        Common_cJSON_AddItemToObject(retObj, RESULT, Common_cJSON_CreateNumber(retCode));

        if (retCode == WEB_CODE_OK)
        {
            Common_cJSON_AddItemToObject(retObj, DATA, retData);
        }
        else
        {
            Common_cJSON_AddItemToObject(retObj, ERROR_STRING, Common_cJSON_CreateString(errorString?errorString:""));
            Common_cJSON_Delete(retData);
        }

        data_mtemp = Common_cJSON_PrintUnformatted(retObj, NULL);
        Common_cJSON_Delete(retObj);

        //LOGW("Response:%s\n", data_mtemp);
        websResponse(wp, retCode == ACCESS_ERROR_TYPE_AUTH ? 401 : 200, NULL);
        websWriteBlock(wp, data_mtemp, strlen(data_mtemp));
        Common_cJSON_free(data_mtemp);
    }

    return iRet;
}

/*Semantic Interface End*/
//获取配置
S32 web_GetConfig(S32 hLogin,U32 dwCommand, S32 lStartChan,S32 lChanNum,VOID *lpParam, U32 dwParamSize, VOID* lpOutBuffer, U32 dwOutBufferSize, U32 *lpBytesReturned)
{
	int iRet = 0;
	if ( hLogin < 0)
	{
		iRet = -1;
		return iRet;
	}

	switch(dwCommand)
	{
		case ANTS_DVR_GET_DEVICECFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_DEVICECFG_V2 *pcfg = (ANTS_DVR_DEVICECFG_V2 *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_DEVICECFG_V2));
				break;
		    }
		case ANTS_DVR_GET_NETCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_NETCFG *pcfg = (ANTS_DVR_NETCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_NETCFG));
				break;
			}
		case ANTS_DVR_GET_RTSPCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_RTSPCFG *pcfg = (ANTS_DVR_RTSPCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_RTSPCFG));
				break;
			}
		case ANTS_DVR_GET_MANAGERHOSTS_CFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_MANAGERHOSTS_CFG *pcfg = (ANTS_DVR_MANAGERHOSTS_CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_MANAGERHOSTS_CFG));
			}
			break;
		case ANTS_DVR_CONFIG_MANAGERHOSTS_UUID:
			{
				int param[2];
				char pcfg[128];
				memcpy(param, lpParam, sizeof(param));
				memcpy(pcfg, lpOutBuffer, sizeof(pcfg));
			}
			break;
		case ANTS_DVR_GET_ZEROCODEC:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_ZEROCODEC_PARA *pcfg = (ANTS_DVR_ZEROCODEC_PARA *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_ZEROCODEC_PARA));
				break;
			}
		case ANTS_DVR_GET_SHELTERCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_SHELTER_EX *pcfg = (ANTS_DVR_SHELTER_EX *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_SHELTER_EX));
				break;
			}
		case ANTS_DVR_GET_ALARMINCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_ALARMINCFG_EX *pcfg = (ANTS_DVR_ALARMINCFG_EX *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_ALARMINCFG_EX));
			}
			break;
		case ANTS_DVR_GET_FACTORY_INFO:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_FACTORY_INFO *pcfg = (ANTS_DVR_FACTORY_INFO *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_FACTORY_INFO));
			}
			break;
		case ANTS_DVR_GET_VIDEOFORMAT:
			{
				*lpBytesReturned = dwOutBufferSize;
				unsigned int *pcfg = (unsigned int *)lpOutBuffer;
				memset(pcfg, 0, sizeof(unsigned int));
			}
			break;
		case ANTS_DVR_GET_TIMECFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_TIME *pcfg = (ANTS_DVR_TIME *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_TIME));
			}
			break;
		case ANTS_DVR_CONFIG_MANAGERHOSTS_URL_APPLE:
			{
				*lpBytesReturned = dwOutBufferSize;
				int param[2];
				char pcfg[128];
				memcpy(param, lpParam, sizeof(param));
				memcpy(pcfg, lpOutBuffer, sizeof(pcfg));
			}
			break;
		case ANTS_DVR_CONFIG_MANAGERHOSTS_URL_ANDROID:
			{
				*lpBytesReturned = dwOutBufferSize;
				int param[2];
				char pcfg[128];
				memcpy(param, lpParam, sizeof(param));
				memcpy(pcfg, lpOutBuffer, sizeof(pcfg));
			}
			break;
		case ANTS_DVR_GET_DECODERCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_DECODERCFG *pcfg = (ANTS_DVR_DECODERCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_DECODERCFG));
			}
			break;
		case ANTS_DVR_GET_ZONEANDDST:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_ZONEANDDST *pcfg = (ANTS_DVR_ZONEANDDST *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_ZONEANDDST));
			}
			break;
		case ANTS_DVR_GET_OSDCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_OSDCFG_V2 *pcfg = (ANTS_DVR_OSDCFG_V2 *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_OSDCFG_V2));
			}
			break;
		case ANTS_DVR_GET_SHOWSTRING:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_SHOWSTRING *pcfg = (ANTS_DVR_SHOWSTRING *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_SHOWSTRING));
			}
			break;
		case ANTS_DVR_GET_VIDEOEFFECT_EX:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_COLOR_EX *pcfg = (ANTS_DVR_COLOR_EX *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_COLOR_EX));
			}
			break;
		case ANTS_DVR_GET_CHANOSDTEXTLATTICE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_OSD_TEXT_LATTICE *pcfg = (ANTS_DVR_OSD_TEXT_LATTICE *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_OSD_TEXT_LATTICE));
			}
			break;
		case ANTS_DVR_GET_TIMEOSDPOS:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_TIME_OSD_POS *pcfg = (ANTS_DVR_TIME_OSD_POS *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_TIME_OSD_POS));
			}
			break;
		case ANTS_DVR_GET_MULTIOSDTEXTLATTICE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_OSD_TEXT_LATTICE *pcfg = (ANTS_DVR_OSD_TEXT_LATTICE *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_OSD_TEXT_LATTICE));
			}
			break;
		case ANTS_DVR_GET_COMPRESSCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_COMPRESSIONCFG *pcfg = (ANTS_DVR_COMPRESSIONCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_COMPRESSIONCFG));
			}
			break;
		case ANTS_DVR_GET_MOTIONEXCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_MOTION_EX2 *pcfg = (ANTS_DVR_MOTION_EX2 *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_MOTION_EX2));
			}
			break;
		case ANTS_DVR_CONFIG_MOTION_PTZLINKCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_PTZLINKCFG *pcfg = (ANTS_DVR_PTZLINKCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_PTZLINKCFG));
			}
			break;
		case ANTS_DVR_CONFIG_HIDEALARM_PTZLINKCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_PTZLINKCFG *pcfg = (ANTS_DVR_PTZLINKCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_PTZLINKCFG));
			}
			break;
		case ANTS_DVR_CONFIG_HIDEALARMCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_HIDEALARM_V2 *pcfg = (ANTS_DVR_HIDEALARM_V2 *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_HIDEALARM_V2));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_COUNTER_WIRE_RULE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_COUNTER_WIRE_ALL *pcfg = (ANTS_MID_COUNTER_WIRE_ALL *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_COUNTER_WIRE_ALL));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_OBJECT_REGION_RULE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_OBJECT_REGION_ALL *pcfg = (ANTS_MID_OBJECT_REGION_ALL *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_OBJECT_REGION_ALL));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_DETECT_REGION_RULE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_DETECT_REGION_ALL *pcfg = (ANTS_MID_DETECT_REGION_ALL *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_DETECT_REGION_ALL));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_DETECT_WIRE_RULE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_DETECT_WIRE_ALL *pcfg = (ANTS_MID_DETECT_WIRE_ALL *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_DETECT_WIRE_ALL));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_SOUND_ALARM_RULE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_SOUND_ALARM_ALL *pcfg = (ANTS_MID_SOUND_ALARM_ALL *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_SOUND_ALARM_ALL));
			}
			break;
		case ANTS_DVR_CONFIG_MOTION_DETECT:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_MOTION_DETECT_ALL *pcfg = (ANTS_MID_MOTION_DETECT_ALL *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_MOTION_DETECT_ALL));
			}
			break;
		case ANTS_DVR_CONFIG_VIDEO_DIAGNOSE:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_VIDEO_DIAGNOSE_CFG *pcfg = (ANTS_MID_VIDEO_DIAGNOSE_CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_VIDEO_DIAGNOSE_CFG));
			}
			break;
		case ANTS_DVR_CONFIG_FIRE_DETECT:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_FIRE_DETECT_CFG *pcfg = (ANTS_MID_FIRE_DETECT_CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_FIRE_DETECT_CFG));
			}
			break;
		case ANTS_DVR_CONFIG_FACE_DETECT:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_FACE_DETECT_CFG *pcfg = (ANTS_MID_FACE_DETECT_CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_FACE_DETECT_CFG));
			}
			break;
		case ANTS_DVR_CONFIG_PLATE_DETECT:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_MID_PLATE_DETECT_CFG *pcfg = (ANTS_MID_PLATE_DETECT_CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_MID_PLATE_DETECT_CFG));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_COUNTER_WIRE_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_DETECT_WIRE_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_DETECT_REGION_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_OBJECT_REGION_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_IVS_SOUND_ALARM_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_MOTION_DETECT_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_VIDEO_DIAGNOSE_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_FIRE_DETECT_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_PLATE_DETECT_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_CONFIG_FACE_DETECT_LINK:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_IVS_DETECT_LINK *pcfg = (ANTS_DVR_IVS_DETECT_LINK *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_IVS_DETECT_LINK));
			}
			break;
		case ANTS_DVR_GET_SENSOR_CFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_SENSOREX_CFG *pcfg = (ANTS_DVR_SENSOREX_CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_SENSOREX_CFG));
			}
			break;
		case ANTS_DVR_GET_AUTOREBOOT:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_AUTOREBOOT *pcfg = (ANTS_DVR_AUTOREBOOT *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_AUTOREBOOT));
			}
			break;
		case ANTS_DVR_GET_HDCFG_V2:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_HDCFG_V2 *pcfg = (ANTS_DVR_HDCFG_V2 *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_HDCFG_V2));
			}
			break;
		case ANTS_DVR_GET_HDISK_SMART_ATTR:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTSMID_HDISK_SMART_ATTR_T *pcfg = (ANTSMID_HDISK_SMART_ATTR_T *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTSMID_HDISK_SMART_ATTR_T));
			}
			break;
		case ANTS_DVR_GET_HDISK_FORCERECORD:
			{
				*lpBytesReturned = dwOutBufferSize;
				unsigned int *pcfg = (unsigned int *)lpOutBuffer;
				memset(pcfg, 0, sizeof(unsigned int));
			}
			break;
		case ANTS_DVR_GET_UPDATE_PATH:
			{
				*lpBytesReturned = dwOutBufferSize;
				AntsHostMgrLibUpdatePath_T *pcfg = (AntsHostMgrLibUpdatePath_T *)lpOutBuffer;
				memset(pcfg, 0, sizeof(AntsHostMgrLibUpdatePath_T));
			}
			break;
		case ANTS_DVR_GET_USERCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_USER_EX *pcfg = (ANTS_DVR_USER_EX *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_USER_EX));
			}
			break;
		case ANTS_DVR_GET_EXCEPTIONCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_EXCEPTION *pcfg = (ANTS_DVR_EXCEPTION *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_EXCEPTION));
			}
			break;
		case ANTS_DVR_GET_ALARMOUTCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_ALARMOUTCFG *pcfg = (ANTS_DVR_ALARMOUTCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_ALARMOUTCFG));
			}
			break;
		case ANTS_DVR_GET_EMAILCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_EMAILCFG *pcfg = (ANTS_DVR_EMAILCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_EMAILCFG));
			}
			break;
		case ANTS_DVR_GET_NTPCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_NTPPARA *pcfg = (ANTS_DVR_NTPPARA *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_NTPPARA));
			}
			break;
		case ANTS_DVR_GET_DDNSCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_DDNSPARA *pcfg = (ANTS_DVR_DDNSPARA *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_DDNSPARA));
			}
			break;
		case ANTS_DVR_GET_NETCFG_MULTI:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_NETCFG_MULTI *pcfg = (ANTS_DVR_NETCFG_MULTI *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_NETCFG_MULTI));
			}
			break;
		case ANTS_DVR_GET_SNMPCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_SNMPCFG *pcfg = (ANTS_DVR_SNMPCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_SNMPCFG));
			}
			break;
		case ANTS_DVR_CONFIG_MULTICASTCFG:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_MULTICAST_CHANCFG *pcfg = (ANTS_DVR_MULTICAST_CHANCFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_MULTICAST_CHANCFG));
			}
			break;
		case ANTS_DVR_GET_FTPUPLOAD:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_FTPUPLOAD *pcfg = (ANTS_DVR_FTPUPLOAD *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_FTPUPLOAD));
			}
			break;
		case ANTS_DVR_GET_WIFIWORKSTATUS:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_WIFI_WORKSTATUS_S *pcfg = (ANTS_DVR_WIFI_WORKSTATUS_S *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_WIFI_WORKSTATUS_S));
			}
			break;
		case ANTS_DVR_GET_WIFIAPCOUNT:
			{
				*lpBytesReturned = dwOutBufferSize;
				int *pcfg = (int *)lpOutBuffer;
				memset(pcfg, 0, sizeof(int));
			}
			break;
		case ANTS_DVR_GET_WIFISCANCOUNT:
			{
				*lpBytesReturned = dwOutBufferSize;
				int *pcfg = (int *)lpOutBuffer;
				memset(pcfg, 0, sizeof(int));
			}
			break;
		case ANTS_DVR_GET_WIFIAPLIST:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S *pcfg = (ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S));
			}
			break;
		case ANTS_DVR_GET_WIFISCANRESULT:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_WIFI_SCANAPITEM_S *pcfg = (ANTS_DVR_WIFI_SCANAPITEM_S *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_WIFI_SCANAPITEM_S));
			}
			break;
		case ANTS_DVR_CONFIG_MANAGERHOSTS_GB28181:
			{
				*lpBytesReturned = dwOutBufferSize;
				ANTS_DVR_GB28181CFG *pcfg = (ANTS_DVR_GB28181CFG *)lpOutBuffer;
				memset(pcfg, 0, sizeof(ANTS_DVR_GB28181CFG));
			}
			break;
		default:
			{
		    }
			break;
	}

	return iRet;
}
#endif

