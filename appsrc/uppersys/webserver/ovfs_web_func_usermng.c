#include "ovfs_web_func.h"

extern WebsHash sessions;
extern pthread_rwlock_t rw_user;
static int web_semantic_get_UserOnline(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    cJSON_Struct *pArry = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *pConnectInfo = NULL;
    cJSON_Struct *str_tmp = NULL;
    cJSON_Struct* pIpList=NULL;
    int nloop;
    int k = 0;
    S32 val = 0;

    pArry = Common_Json_SetAttrValue(outdata, -1, "UserInfoList", Common_Json_Type_Array, NULL, 0, 0);
    //printf("pArry:%p , %s\n",pArry,__FUNCTION__);
    if(pArry)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/OnlineUser");
        Common_Json_StandardPrint(header, NULL, NULL, NULL);
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        //printf("pResult:%p , %s\n",pResult,__FUNCTION__);
        Common_Json_StandardPrint(pResult, NULL, NULL, NULL);

        if (0 == ret)
        {
            pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            //printf("pArry_tmp:%p , %s\n",pArry_tmp,__FUNCTION__);
            if (pArry_tmp)
            {
                int userSize = Common_Json_Size(pArry_tmp);
                int ip_count = 0;

                for (nloop = 0; nloop < userSize; nloop++)
                {
                    str_tmp = NULL;
                    Common_Json_GetAttrValue(pArry_tmp, nloop, "UserName", NULL, (S8**)&str_tmp, NULL, NULL);
                    Common_Json_SetAttrValue(pArry, nloop, "UserName", Common_Json_Type_String, str_tmp, 0, 0);

                    pConnectInfo = Common_Json_GetAttrValue(pArry_tmp, nloop, "ConnectInfo", NULL, NULL, NULL, NULL);
                    pIpList = Common_Json_SetAttrValue(pArry, nloop, "ConnectInfo", Common_Json_Type_Array, NULL, 0, 0);
                    Common_Json_SetAttrValue(pArry, nloop, "StreamCount", Common_Json_Type_Number, NULL,Common_Json_Size(pConnectInfo), 0);


                    //ip
                    ip_count = Common_Json_Size(pConnectInfo);
                    for(; k < ip_count; ++k)
                    {

                        Common_Json_GetAttrValue(pConnectInfo, k,"IPV4",NULL,(S8**)&str_tmp,NULL,NULL);
                        Common_Json_SetAttrValue(pIpList, k,"IPV4",Common_Json_Type_String,str_tmp,0,0);
                        //printf("k:%d %s\n",k,str_tmp);
                        Common_Json_GetAttrValue(pConnectInfo, k,"CreateTime",NULL,NULL,&val,NULL);
                        //printf("k:%d %d\n",k,val);
                        Common_Json_SetAttrValue(pIpList, k,"CreateTime",Common_Json_Type_Number,NULL,val,0);

                        Common_Json_GetAttrValue(pConnectInfo, k,"SessionId",NULL,NULL,&val,NULL);
                        //printf("k:%d %d\n",k,val);
                        Common_Json_SetAttrValue(pIpList, k,"SessionId",Common_Json_Type_Number,NULL,val,0);
                        //printf("========\n");
                    }

                }
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;

    }
    return ret;
}

static int web_semantic_get_UserInfo(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    int nloop = 0;
    char *str_tmp = NULL;
    int i_num = 0;
    int userSize = 0;
    int i_priority = 0xff;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pArry_root = NULL;
    cJSON_Struct *pResult = NULL;

    pArry_root = Common_Json_SetAttrValue(outdata, -1, "UserInfoList", Common_Json_Type_Array, NULL, 0, 0);
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/UserCfg");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            if (pArry_tmp)
            {
                userSize = Common_Json_Size(pArry_tmp);
                for (nloop = 0; nloop < userSize; nloop++)
                {
                    str_tmp = NULL;
                    Common_Json_GetAttrValue(pArry_tmp, nloop, "UserName", NULL, &str_tmp, NULL, NULL);
                    Common_Json_SetAttrValue(pArry_root, nloop, "UserName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                    Common_Json_SetAttrValue(pArry_root, nloop, "Password", Common_Json_Type_String, "", 0, 0);

                    if (Common_Json_GetAttrValue(pArry_tmp, nloop, "Priority", NULL, NULL, &i_num, NULL))
                    {
                        if (i_num == 0x10)
                        {
                            i_priority = 2;
                        }
                        else if (i_num == 2)
                        {
                            i_priority = 1;
                        }
                        else if (i_num == 1)
                        {
                            i_priority = 0;
                        }
                        else
                        {
                            i_priority = 0xff;
                        }
                        Common_Json_SetAttrValue(pArry_root, nloop, "Priority", Common_Json_Type_Number, NULL, i_priority, i_priority);
                    }
                    if(Common_Json_GetAttrValue(pArry_tmp, nloop, "BindIPv4", NULL, &str_tmp, NULL, NULL) == NULL)
                    {
                        str_tmp = "";
                    }
                    Common_Json_SetAttrValue(pArry_root, nloop, "BindIPv4", Common_Json_Type_String, str_tmp, 0, 0);
                    if(Common_Json_GetAttrValue(pArry_tmp, nloop, "BindIPv6", NULL, &str_tmp, NULL, NULL) == NULL)
                    {
                        str_tmp = "";
                    }
                    Common_Json_SetAttrValue(pArry_root, nloop, "BindIPv6", Common_Json_Type_String, str_tmp, 0, 0);
                    //MACAddr
                    if(Common_Json_GetAttrValue(pArry_tmp, nloop, "BindMAC", NULL, &str_tmp, NULL, NULL) == NULL)
                    {
                        str_tmp = "";
                    }
                    Common_Json_SetAttrValue(pArry_root, nloop, "BindMAC", Common_Json_Type_String, str_tmp, 0, 0);
                    //pArry_root1 = Common_Json_SetAttrValue(pArry_root, nloop, "MACAddr", Common_Json_Type_Array, NULL, 0, 0);
                    // for (iloop=0; iloop<6; iloop++)
                    // {
                    //     Common_Json_SetAttrValue(pArry_root1, iloop, NULL, Common_Json_Type_Number, NULL, 0, 0);
                    // }
                }
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return ret;
}

static int web_semantic_set_UserInfo(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    int b_adduser = 0;
    int i_num = 0;
    int i_priority = 0xff;
    char *str_tmp = NULL;
    cJSON_Struct *pArry_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //UserIndex
    Common_Json_GetAttrValue(indata, -1, "UserIndex", NULL, NULL, &b_adduser, NULL);
    if (0 == ret)
    {
        pArry_tmp = Common_Json_SetAttrValue(lowerData, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
        if (pArry_tmp)
        {
            if (Common_Json_GetAttrValueStr(indata, "UserName", &str_tmp))
            {
                Common_Json_SetAttrValue(pArry_tmp, 0, "UserName", Common_Json_Type_String, str_tmp, 0, 0);
            }
            if (Common_Json_GetAttrValueStr(indata, "Password", &str_tmp))
            {
                cJSON_Struct *pArry_tmp1 = Common_Json_SetAttrValue(pArry_tmp, 0, "Password", Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_SetAttrValue(pArry_tmp1, 0, NULL, Common_Json_Type_String, str_tmp, 0, 0);
            }
            if (Common_Json_GetAttrValueInt(indata, "Priority", &i_num))
            {
                if (i_num == 2)
                {
                    i_priority = 0x10;
                }
                else if (i_num == 1)
                {
                    i_priority = 2;
                }
                else if (i_num == 0)
                {
                    i_priority = 1;
                }
                else
                {
                    i_priority = 0;
                }
                Common_Json_SetAttrValue(pArry_tmp, 0, "Priority", Common_Json_Type_Number, NULL, i_priority, i_priority);
            }
            if (Common_Json_GetAttrValueStr(indata, "BindIPv4", &str_tmp))
            {
                Common_Json_SetAttrValue(pArry_tmp, 0, "BindIPv4", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
            }

            if (Common_Json_GetAttrValueStr(indata, "BindIPv6", &str_tmp))
            {
                Common_Json_SetAttrValue(pArry_tmp, 0, "BindIPv6", Common_Json_Type_String, str_tmp, 0, 0);
            }
            //Mac TODO
            if (Common_Json_GetAttrValueStr(indata, "BindMAC", &str_tmp))
            {
                Common_Json_SetAttrValue(pArry_tmp, 0, "BindMAC", Common_Json_Type_String, str_tmp, 0, 0);
            }
        }

        if (-1 == b_adduser)
        {
            //添加用户
            Ovfs_Web_UpdateHeader(header, REST_POST, "/Access/UserCfg");
        }
        else
        {
            //修改用户
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Access/UserCfg");
        }
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_del_UserInfo(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    char uriPath[128] = {0};

    if (Common_Json_GetAttrValue(indata, -1, "UserName", NULL, &str_tmp, NULL, NULL) == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    if (0 == ret)
    {
        //snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg?UserName=%s", str_tmp);
        snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg");
        Ovfs_Web_UpdateHeader(header, REST_DELETE, uriPath);
        ret = Ovfs_Web_RestMethodA(header, indata, NULL, 0);

        //	MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
        //	Common_DList_Delete(g_ovfs_web->userLoginList, (void *)str_tmp, find_item_by_username);
        //	MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
    }

    return ret;
}

static const int s_webRightsToAccess[] =
{
    BIT_PTZ, BIT_RECORD, BIT_PLAYBACK, BIT_SETTING, BIT_LOG, BIT_UPGRADE | BIT_FORMAT,
    BIT_REMOTETALK, BIT_PREVIEW, 0, 0, 0, BIT_VIEWSETTING, 0, BIT_POWER,
};

static int web_semantic_get_user_right(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    int rightMask = 0;
    char *userName = NULL;
    char uriPath[64] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pResult = NULL;
    int iloop = 0;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if ((Common_Json_GetAttrValueStr(indata, "UserName", &userName) == NULL &&
            Common_Json_GetAttrValueStr(header, "Auth/UserName", &userName) == NULL) || strlen(userName) < 1)
    {
        ret = WEB_CODE_InvalidArg;
    }

    Common_Json_SetAttrValueStr(lowerData, "UserName", userName);

    if (0 == ret)
    {
        // 本地权限 10
        cJSON_Struct *tmp = NULL;
        tmp = Common_Json_SetAttrValue(outdata, -1, "LocalRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < 10; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, 1, 1);
        }

        //snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg?UserName=%s", userName);
        snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg");
        Ovfs_Web_UpdateHeader(header, REST_GET, uriPath);
        ret = Ovfs_Web_RestMethodA(header, lowerData, &pResult, 0);
        if (0 == ret)
        {
            pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            pObj_tmp = Common_Json_GetAttrValue(pArry_tmp, 0, "RemoteRight", NULL, NULL, NULL, NULL);
            Common_Json_GetAttrValue(pObj_tmp, -1, "RightMask", NULL, NULL, &rightMask, NULL);

            //远程权限 11
            tmp = Common_Json_SetAttrValue(outdata, -1, "RemoteRight", Common_Json_Type_Array, NULL, 0, 0);

            for (iloop = 0; iloop < 14; ++iloop)
            {
                Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, rightMask & s_webRightsToAccess[iloop] ? 1 : 0, 0);
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (0 == ret)
    {
        cJSON_Struct *tmp = NULL;

        //本地预览 12
        tmp = Common_Json_SetAttrValue(outdata, -1, "LocalPreviewRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, 1, 1);
        }
        //远程预览 13
        tmp = Common_Json_SetAttrValue(outdata, -1, "NetPreviewRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, rightMask & s_webRightsToAccess[7], 0);
        }
        //本地回放 14
        tmp = Common_Json_SetAttrValue(outdata, -1, "LocalPlaybackRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, 1, 1);
        }
        //远程回放 15
        tmp = Common_Json_SetAttrValue(outdata, -1, "NetPlaybackRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, rightMask & s_webRightsToAccess[2], 0);
        }
        //本地备份 16
        tmp = Common_Json_SetAttrValue(outdata, -1, "LocalBackupRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, 1, 1);
        }

        //本地PTZ 17
        tmp = Common_Json_SetAttrValue(outdata, -1, "LocalPTZRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, 1, 1);
        }
        //远程PTZ 18
        tmp = Common_Json_SetAttrValue(outdata, -1, "NetPTZRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, rightMask & s_webRightsToAccess[0], 0);
        }
        //本地录像 19
        tmp = Common_Json_SetAttrValue(outdata, -1, "LocalRecordRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, 1, 1);
        }
        //远程录像 20
        tmp = Common_Json_SetAttrValue(outdata, -1, "NetRecordRight", Common_Json_Type_Array, NULL, 0, 0);
        for(iloop = 0; iloop < g_struWebSiteSDKInfo.g_ChanNum; ++iloop)
        {
            Common_Json_SetAttrValue(tmp, iloop, NULL, Common_Json_Type_Number, NULL, rightMask & s_webRightsToAccess[1], 1);
        }
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

static int web_semantic_set_user_right(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    int iloop = 0;
    char *userName = NULL;
    unsigned int rightMask = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pObj_tmp = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if ((Common_Json_GetAttrValueStr(indata, "UserName", &userName) == NULL &&
            Common_Json_GetAttrValueStr(header, "Auth/UserName", &userName) == NULL) || strlen(userName) < 1)
    {
        ret = WEB_CODE_InvalidArg;
    }

    Common_Json_SetAttrValueStr(lowerData, "UserName", userName);

    if (0 == ret)
    {
        char uriPath[64] = {0};
        //snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg?UserName=%s", userName);
        snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg");
        Ovfs_Web_UpdateHeader(header, REST_GET, uriPath);
        ret = Ovfs_Web_RestMethodA(header, lowerData, &pResult, 0);
        if (0 == ret)
        {
            pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            pObj_tmp = Common_Json_GetAttrValue(pArry_tmp, 0, "RemoteRight", NULL, NULL, NULL, NULL);
            Common_Json_GetAttrValue(pObj_tmp, -1, "RightMask", NULL, NULL, (int *)&rightMask, NULL);
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    if (0 == ret)
    {
        pArry_tmp = Common_Json_GetAttrValue(indata, -1, "RemoteRight", NULL, NULL, NULL, NULL);
        if (pArry_tmp)
        {
            int i_num = 0;
            for (iloop=0; iloop < 14; iloop++)
            {
                Common_Json_GetAttrValue(pArry_tmp, iloop, NULL, NULL, NULL, &i_num, NULL);
                if (i_num)
                {
                    rightMask |= s_webRightsToAccess[iloop];
                }
                else
                {
                    rightMask &= ~s_webRightsToAccess[iloop];
                }
            }
        }
    }

//    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        if ((pArry_tmp = Common_Json_SetAttrValueArr(lowerData, "ResList")) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(pArry_tmp, 0, "UserName", Common_Json_Type_String, userName, 0, 0);
            pObj_tmp = Common_Json_SetAttrValue(pArry_tmp, 0, "RemoteRight", Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(pObj_tmp, -1, "RightMask", Common_Json_Type_Number, NULL, rightMask, 0);

            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Access/UserCfg");
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

//在线用户
int frmUserOnline(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_UserOnline(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    return ret;
}

static int web_semantic_get_UserOnlineCfg(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int ret = 0;
    Common_Json_SetAttrValueInt(outdata, "SessionCount", g_ovfs_web->session_count);
    return ret;
}

static int web_semantic_set_UserOnlineCfg(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata)
{
    int i_num = 0;
    int ret = 0;
    if(Common_Json_GetAttrValueInt(indata, "SessionCount", &i_num))
    {
        if(i_num<1)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            g_ovfs_web->session_count = i_num;
            Common_Json_SetAttrValueInt(g_ovfs_config, "SessionCount",g_ovfs_web->session_count);
            Access_SaveConfig(g_AccessHandle, g_ovfs_config);
        }
    }
    return ret;

}

int frmUserOnlineCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_UserOnlineCfg(header, indata, outdata);
            break;
        case 1:
            //获取参数
            ret = web_semantic_set_UserOnlineCfg(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}


int frmUserManage(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_UserInfo(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_UserInfo(header, indata, outdata);
            break;

        case 2:
            //删除用户
            ret = web_semantic_del_UserInfo(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1 || opt->type == 2)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int frmUserRights_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_user_right(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_user_right(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_get_passwordlost(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    //int valueInt;
    char *valueStr;
    char *userName=NULL;
    char uriPath[64] = {0};
    cJSON_Struct *lowerData = NULL;
    //cJSON_Struct *tmp = NULL;

    //cJSON_Struct* d = NULL;
    //cJSON_Struct *property = NULL;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueStr(indata, "UserName", &userName) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }
    if (0 == ret)
    {
        //snprintf(uriPath, sizeof(uriPath), "/Access/PasswordLost?UserName=%s", userName);
        snprintf(uriPath, sizeof(uriPath), "/Access/PasswordLost");
        Ovfs_Web_UpdateHeader(header, REST_GET, uriPath);
        ret = Ovfs_Web_RestMethodA(header, indata, &lowerData, 0);
        if(ret == 0)
        {

            Common_Json_GetAttrValueStr(lowerData,"SerialNumber",(S8**)&valueStr);
            Common_Json_SetAttrValueStr(outdata, "SerialNumber", valueStr);

            Common_Json_GetAttrValueStr(lowerData,"Token",(S8**)&valueStr);
            Common_Json_SetAttrValueStr(outdata, "Token", valueStr);


        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

static int web_semantic_set_passwordlost(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    //int valueInt;
    char *valueStr;

    cJSON_Struct *lowerData = NULL;
    //cJSON_Struct *obj = NULL;
    //cJSON_Struct *property = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {

        if (Common_Json_GetAttrValueStr(indata, "RestoreInfo", (S8**)&valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "RestoreInfo", valueStr);
        }


        if (Common_Json_GetAttrValueStr(indata, "UserName", (S8**)&valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "UserName", valueStr);
        }

    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Access/RestorePassword");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;
    return ret;
}

int frmPasswordLost(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_passwordlost(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_passwordlost(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int web_semantic_get_userright_remote(cJSON_Struct *header,int right_id)
{
    int ret = 0;
    int rightMask = 0;
    char *userName = NULL;
    char uriPath[64] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pResult = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if ((Common_Json_GetAttrValueStr(header, "Auth/UserName", &userName) == NULL) || strlen(userName) < 1)
    {
        LOGD("---->ERROR USERNAME!\n");
        ret = WEB_CODE_InvalidArg;
    }

    if(right_id<0 || right_id>13)
    {
        LOGD("---->ERROR right_id!\n");
        ret = WEB_CODE_InvalidArg;
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(lowerData, "UserName", userName);
        //cJSON_Struct *tmp = NULL;
        snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg");
        Ovfs_Web_UpdateHeader(header, REST_GET, uriPath);
        ret = Ovfs_Web_RestMethodA(header, lowerData, &pResult, 0);
        if (0 == ret)
        {
            pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            pObj_tmp = Common_Json_GetAttrValue(pArry_tmp, 0, "RemoteRight", NULL, NULL, NULL, NULL);
            Common_Json_GetAttrValue(pObj_tmp, -1, "RightMask", NULL, NULL, &rightMask, NULL);

            ret = (rightMask & s_webRightsToAccess[right_id]) ? WEB_CODE_OK : WEB_CODE_PermissionDenied;

        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;

}

int frmValidateTokens(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
        {
            //int iIndex = 0;
            int bOK = 0;
            S8 *pPassword = NULL;
            S8 *pEncodePassword = NULL;
            S8 *pUsername = NULL;
            Common_Json_GetAttrValueStr(indata, "UserName",&pUsername);
            Common_Json_GetAttrValueStr(indata, "Password",&pPassword);
            S8* pValue = NULL;

            if(pUsername == NULL || pPassword == NULL)
            {
                ret = WEB_CODE_InvalidArg;
            }
            else
            {

                cJSON_Struct *tmp = NULL;
                char uriPath[128] = {0};
                snprintf(uriPath, sizeof(uriPath), "/Access/UserCfg?UserName=%s", pUsername);
                Ovfs_Web_UpdateHeader(header, REST_GET, uriPath);
                ret = Ovfs_Web_RestMethodA(header, NULL, &tmp, 0);
                if (0 == ret)
                {
                    cJSON_Struct*  pArry_tmp = Common_Json_GetAttrValue(tmp, -1, "ResList", NULL, NULL, NULL, NULL);
                    if(Common_Json_ArraySize(pArry_tmp) > 0)
                    {
                        cJSON_Struct*  pObj_tmp = Common_Json_GetAttrValue(pArry_tmp, 0, "Password", NULL, NULL, NULL, NULL);
                        cJSON_Struct*  ar_item = Common_Json_GetAttrValueArrItem(pObj_tmp, 0);
                        pEncodePassword = ((Common_cJSON_T*)ar_item)->valuestring;
                    }
                }

                if(tmp)
                {
                    Common_Json_Delete(tmp);
                    tmp = NULL;
                }

                pValue = auth_DecryptString(pPassword, NULL, NULL, 128);
                if(pValue)
                {

                    LOGD("try match:[%s %s]\n",pValue, pEncodePassword);
                    if(pEncodePassword == NULL || 0 != Common_StrCmp(pValue, pEncodePassword))
                    {
                        bOK = -1;
                        LOGE("Password auth failed!\n");
                    }
                    wfree(pValue);
                }


                if(bOK != -1)
                {

                    //get user rightds
                    ret = web_semantic_get_user_right(header,indata,outdata);
                }
                else
                {
                    ret = WEB_CODE_InternalMistake;
                }


            }


        }
        break;

        default:
            ret = WEB_CODE_Unsupported;
            break;
        }
    }

    /* if (0 == ret)
     {
         if (opt->type == 1)
         {
             Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
         }
     }*/

    return ret;
}

