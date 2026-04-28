#include "ovfs_web_func.h"
int ovfs_web_get_alarmlink(cJSON_Struct *header, int ch, OVFS_WEB_ALARMLINK_T *palarm_link, int *handle_type,OVFS_WEB_OPTION_S *opt,cJSON_Struct *inParam)
{
    int iRet = 0;
    int linknum = 0;
    int nloop = 0;
    int i_num = 0;
    int val = 0;
    char *str_tmp = NULL;
    char *str_tmp1 = NULL;
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *pObj_root = NULL;
    cJSON_Struct *lowerData = NULL;

    if (header == NULL || palarm_link == NULL || handle_type == NULL)
    {
        iRet = -1;
    }

    if (0 == iRet)
    {
        iRet = Ovfs_Web_RestMethodA(header, inParam, &lowerData, 0);
    }

    if (0 == iRet)
    {
        *handle_type = 0;
        pArray_root = Common_Json_GetAttrValue(lowerData, -1, "ResList", NULL, NULL, NULL, NULL);
        linknum = Common_Json_Size(pArray_root);
        for (nloop=0; nloop<linknum; nloop++)
        {
            str_tmp = NULL;


            Common_Json_GetAttrValue(pArray_root, nloop, "Device", NULL, NULL,&val, NULL);
            if(opt->dev != val)continue;

            Common_Json_GetAttrValue(pArray_root, nloop, "Channel", NULL,NULL, &val, NULL);
            if(ch != val)continue;


            Common_Json_GetAttrValue(pArray_root, nloop, "ActionName", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                if (!strcmp(str_tmp, "LinkShowDialogue"))
                {
                    //上报监视器 0x01
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x1;
                    }
                }
                else if (!strcmp(str_tmp, "LinkBuzzer"))
                {
                    //声音告警 0x02
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x2;
                    }
                }
                else if (!strcmp(str_tmp, "LinkReportCentre"))
                {
                    //上传中心 0x04
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x4;
                    }
                }
                else if (!strcmp(str_tmp, "LinkAlarmOut"))
                {
                    //触发报警输出 0x08
                    int alarm_enable = 0;
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &alarm_enable, NULL);
                    if (1 == alarm_enable)
                    {
                        *handle_type |= 0x8;
                    }
                    i_num = 0;
                    Common_Json_GetAttrValue(pArray_root, nloop, "Mask0", NULL, NULL, &i_num, NULL);
                    palarm_link->alarmout_para.mask0 = alarm_enable?i_num:0;
                    i_num = 0;
                    Common_Json_GetAttrValue(pArray_root, nloop, "Mask1", NULL, NULL, &i_num, NULL);
                    palarm_link->alarmout_para.mask1 = i_num;
                }
                else if (!strcmp(str_tmp, "LinkMail"))
                {
                    //邮件联动 0x10
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x10;
                    }
                    Common_Json_GetAttrValue(pArray_root, nloop, "Default", NULL, NULL, &i_num, NULL);
                    palarm_link->mail_para.type= i_num;
                    pObj_root = Common_Json_GetAttrValue(pArray_root, nloop, "Sender", NULL, NULL, NULL, NULL);
                    if (pObj_root)
                    {
                        Common_Json_GetAttrValue(pObj_root, -1, "User", NULL, &str_tmp1, NULL, NULL);
                        snprintf(palarm_link->mail_para.sender.mailAccount, sizeof(palarm_link->mail_para.sender.mailAccount), "%s", str_tmp1);
                        Common_Json_GetAttrValue(pObj_root, -1, "Password", NULL, &str_tmp1, NULL, NULL);
                        snprintf(palarm_link->mail_para.sender.password, sizeof(palarm_link->mail_para.sender.password), "%s", str_tmp1);
                        Common_Json_GetAttrValue(pObj_root, -1, "SMTPServer", NULL, &str_tmp1, NULL, NULL);
                        snprintf(palarm_link->mail_para.sender.smtpServer, sizeof(palarm_link->mail_para.sender.smtpServer), "%s", str_tmp1);
                        Common_Json_GetAttrValue(pObj_root, -1, "SMTPPort", NULL, NULL, &i_num, NULL);
                        palarm_link->mail_para.sender.smtpPort = i_num;
                        Common_Json_GetAttrValue(pObj_root, -1, "EnableSSL", NULL, NULL, &i_num, NULL);
                        palarm_link->mail_para.sender.bEnableSSL = i_num;
                        Common_Json_GetAttrValue(pObj_root, -1, "ServerVerify", NULL, NULL, &i_num, NULL);
                        palarm_link->mail_para.sender.serverVerify= i_num;
                    }
                    pObj_root = Common_Json_GetAttrValue(pArray_root, nloop, "Receiver", NULL, NULL, NULL, NULL);
                    if (pObj_root)
                    {
                        Common_Json_GetAttrValue(pObj_root, -1, "Name", NULL, &str_tmp1, NULL, NULL);
                        snprintf(palarm_link->mail_para.receiver.name, sizeof(palarm_link->mail_para.receiver.name), "%s", str_tmp1);
                        Common_Json_GetAttrValue(pObj_root, -1, "Addr", NULL, &str_tmp1, NULL, NULL);
                        snprintf(palarm_link->mail_para.receiver.mailAccount, sizeof(palarm_link->mail_para.receiver.mailAccount), "%s", str_tmp1);
                    }
                    Common_Json_GetAttrValue(pArray_root, nloop, "Attachment", NULL, NULL, &i_num, NULL);
                    palarm_link->mail_para.attachment = i_num;
                    Common_Json_GetAttrValue(pArray_root, nloop, "MailInterval", NULL, NULL, &i_num, NULL);
                    palarm_link->mail_para.mailinterval = i_num;
                }
                else if (!strcmp(str_tmp, "LinkRecord"))
                {
                    //触发报警录象 0x20
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x20;
                    }
                    Common_Json_GetAttrValue(pArray_root, nloop, "Mask0", NULL, NULL, &i_num, NULL);
                    palarm_link->record_para.mask0 = i_num;
                }
#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)
                else if (!strcmp(str_tmp, "LinkSnap"))
                {
                    //屏幕截图  0x40
                    char buf[256] = {0};

                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x40;
                    }
                    memset(buf,0,sizeof(buf));
                    snprintf(buf,sizeof(buf),"Device%d",opt->dev);
                    pObj_root = Common_Json_GetAttrValue(pArray_root, nloop, buf, NULL, NULL, NULL, NULL);
                    if (pObj_root)
                    {
                        memset(buf,0,sizeof(buf));
                        snprintf(buf,sizeof(buf),"Channel0.Enable");
                        Common_Json_GetAttrValue(pObj_root, -1, buf, NULL, NULL, &i_num, NULL);
                        palarm_link->snap_para.benable = i_num;
                        memset(buf,0,sizeof(buf));
                        snprintf(buf,sizeof(buf),"Channel0.Count");
                        Common_Json_GetAttrValue(pObj_root, -1, buf, NULL, NULL, &i_num, NULL);
                        palarm_link->snap_para.count = i_num;
                        memset(buf,0,sizeof(buf));
                        snprintf(buf,sizeof(buf),"Channel0.Interval");
                        Common_Json_GetAttrValue(pObj_root, -1, buf, NULL, NULL, &i_num, NULL);
                        palarm_link->snap_para.interval = i_num;
                    }
                }
#endif
                else if (!strcmp(str_tmp, "LinkFtp"))
                {
                    // 0x80
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x80;
                    }
                }
                else if (!strcmp(str_tmp, "LinkHttp"))
                {
                    // 0x100
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x100;
                    }
                }
                else if (!strcmp(str_tmp, "LinkLightAlarm"))
                {
                    // 0x200
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    if (1 == i_num)
                    {
                        *handle_type |= 0x200;
                    }
                }
                else if (!strcmp(str_tmp, "LinkPtz"))
                {
                    Common_Json_GetAttrValue(pArray_root, nloop, "Type", NULL, NULL, &i_num, NULL);
                    palarm_link->ptz_para.type = i_num;
                    Common_Json_GetAttrValue(pArray_root, nloop, "Index", NULL, NULL, &i_num, NULL);
                    palarm_link->ptz_para.index = i_num;
                }
                else if (!strcmp(str_tmp, "LinkSound"))
                {
                    Common_Json_GetAttrValue(pArray_root, nloop, "Enable", NULL, NULL, &i_num, NULL);
                    palarm_link->audio_para.enable= i_num;
                    LOGD("LinkSound:Enable:[%d]\n",i_num);
                    Common_Json_GetAttrValue(pArray_root, nloop, "Index", NULL, NULL, &i_num, NULL);
                    palarm_link->audio_para.index = i_num + 1;
                    LOGD("LinkSound:Index:[%d]\n",i_num);
                }
                else
                {
                    LOGE("Unkonwn ActionName![%s]\n",str_tmp);
                }
            }
        }
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return iRet;
}

int ovfs_web_set_alarmlink(cJSON_Struct *header, int ch, OVFS_WEB_ALARMLINK_T *alarm_link, int handle_type,OVFS_WEB_OPTION_S *opt,cJSON_Struct *lowerData)
{
    int iRet = 0;

    if (0 == iRet)
    {
        cJSON_Struct *pArray_root = NULL;
        cJSON_Struct *pObj_root = NULL;

        pArray_root = Common_Json_SetAttrValue(lowerData, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);

        Common_Json_SetAttrValue(pArray_root, 0, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 0, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 0, "ActionName", Common_Json_Type_String, "LinkShowDialogue", 0, 0);
        if (handle_type&0x1)
        {
            Common_Json_SetAttrValue(pArray_root, 0, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 0, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
        Common_Json_SetAttrValue(pArray_root, 1, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 1, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 1, "ActionName", Common_Json_Type_String, "LinkBuzzer", 0, 0);
        if (handle_type&0x2)
        {
            Common_Json_SetAttrValue(pArray_root, 1, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 1, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }

        Common_Json_SetAttrValue(pArray_root, 3, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 3, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 3, "ActionName", Common_Json_Type_String, "LinkAlarmOut", 0, 0);

        if (handle_type&0x8)
        {
            Common_Json_SetAttrValue(pArray_root, 3, "Enable", Common_Json_Type_Number, NULL, alarm_link->alarmout_para.mask0?1:0, 0);
        }
        else
        {
            if(alarm_link->alarmout_para.mask0 == 0xff)
            {
                Common_Json_SetAttrValue(pArray_root, 3, "Enable", Common_Json_Type_Number, NULL, 0, 0);
            }
            else
            {
                Common_Json_SetAttrValue(pArray_root, 3, "Enable", Common_Json_Type_Number, NULL, alarm_link->alarmout_para.mask0?1:0, 0);
            }

        }
        if (alarm_link->balarmout_para_set)
        {
            Common_Json_SetAttrValue(pArray_root, 3, "Mask0", Common_Json_Type_Number, NULL, alarm_link->alarmout_para.mask0, 0);
            //  Common_Json_SetAttrValue(pArray_root, 3, "Mask1", Common_Json_Type_Number, NULL, alarm_link->alarmout_para.mask1, alarm_link->alarmout_para.mask1);
        }
        Common_Json_SetAttrValue(pArray_root, 4, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 4, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 4, "ActionName", Common_Json_Type_String, "LinkMail", 0, 0);
        if (handle_type&0x10)
        {
            Common_Json_SetAttrValue(pArray_root, 4, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 4, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
        if (alarm_link->bmail_para_set)
        {
            Common_Json_SetAttrValue(pArray_root, 4, "Default", Common_Json_Type_Number, NULL, alarm_link->mail_para.type, alarm_link->mail_para.type);
            pObj_root = Common_Json_SetAttrValue(pArray_root, 4, "Sender", Common_Json_Type_Object, NULL, 0, 0);
            if (pObj_root)
            {
                Common_Json_SetAttrValue(pObj_root, -1, "User", Common_Json_Type_String, alarm_link->mail_para.sender.mailAccount, 0, 0);
                Common_Json_SetAttrValue(pObj_root, -1, "Password", Common_Json_Type_String, alarm_link->mail_para.sender.password, 0, 0);
                Common_Json_SetAttrValue(pObj_root, -1, "SMTPServer", Common_Json_Type_String, alarm_link->mail_para.sender.smtpServer, 0, 0);
                Common_Json_SetAttrValue(pObj_root, -1, "SMTPPort", Common_Json_Type_Number, NULL, alarm_link->mail_para.sender.smtpPort, alarm_link->mail_para.sender.smtpPort);
                Common_Json_SetAttrValue(pObj_root, -1, "EnableSSL", Common_Json_Type_Number, NULL, alarm_link->mail_para.sender.bEnableSSL, alarm_link->mail_para.sender.bEnableSSL);
                Common_Json_SetAttrValue(pObj_root, -1, "ServerVerify", Common_Json_Type_Number, NULL, alarm_link->mail_para.sender.serverVerify, alarm_link->mail_para.sender.serverVerify);
            }
            pObj_root = Common_Json_SetAttrValue(pArray_root, 4, "Receiver", Common_Json_Type_Object, NULL, 0, 0);
            if (pObj_root)
            {
                Common_Json_SetAttrValue(pObj_root, -1, "Name", Common_Json_Type_String, alarm_link->mail_para.receiver.name, 0, 0);
                Common_Json_SetAttrValue(pObj_root, -1, "Addr", Common_Json_Type_String, alarm_link->mail_para.receiver.mailAccount, 0, 0);
            }
            Common_Json_SetAttrValue(pArray_root, 4, "Attachment", Common_Json_Type_Number, NULL, alarm_link->mail_para.attachment, alarm_link->mail_para.attachment);
            Common_Json_SetAttrValue(pArray_root, 4, "MailInterval", Common_Json_Type_Number, NULL, alarm_link->mail_para.mailinterval, alarm_link->mail_para.mailinterval);
        }
        Common_Json_SetAttrValue(pArray_root, 5, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 5, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 5, "ActionName", Common_Json_Type_String, "LinkRecord", 0, 0);
        if (handle_type&0x20)
        {
            Common_Json_SetAttrValue(pArray_root, 5, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 5, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
        if (alarm_link->brecord_para_set)
        {
            Common_Json_SetAttrValue(pArray_root, 5, "Mask0", Common_Json_Type_Number, NULL, alarm_link->record_para.mask0, alarm_link->record_para.mask0);
        }

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)
        Common_Json_SetAttrValue(pArray_root, 6, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 6, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 6, "ActionName", Common_Json_Type_String, "LinkSnap", 0, 0);
        if (handle_type&0x40)
        {
            Common_Json_SetAttrValue(pArray_root, 6, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 6, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
        if (alarm_link->bsnap_para_set)
        {
            char buf[256] = {0};
            snprintf(buf,sizeof(buf),"Device%d",opt->dev);
            pObj_root = Common_Json_SetAttrValue(pArray_root, 6, buf, Common_Json_Type_Object, NULL, 0, 0);
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"Channel%d",alarm_link->snap_para.snap_channle);
            Common_Json_SetAttrValue(pObj_root, -1, buf, Common_Json_Type_Object, NULL, 0, 0);

            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"Channel%d.Enable",alarm_link->snap_para.snap_channle);
            Common_Json_SetAttrValue(pObj_root, -1, buf, Common_Json_Type_Number, NULL, alarm_link->snap_para.benable, alarm_link->snap_para.benable);


            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"Channel%d.Count",alarm_link->snap_para.snap_channle);
            Common_Json_SetAttrValue(pObj_root, -1, buf, Common_Json_Type_Number, NULL, alarm_link->snap_para.count, alarm_link->snap_para.count);


            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"Channel%d.Interval",alarm_link->snap_para.snap_channle);
            Common_Json_SetAttrValue(pObj_root, -1, buf, Common_Json_Type_Number, NULL, alarm_link->snap_para.interval, alarm_link->snap_para.interval);
        }
#endif
        if (alarm_link->bptz_para_set)
        {
            Common_Json_SetAttrValue(pArray_root, 7, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
            Common_Json_SetAttrValue(pArray_root, 7, "Channel", Common_Json_Type_Number, NULL, ch, 0);
            Common_Json_SetAttrValue(pArray_root, 7, "ActionName", Common_Json_Type_String, "LinkPtz", 0, 0);

            Common_Json_SetAttrValue(pArray_root, 7, "Type", Common_Json_Type_Number, NULL, alarm_link->ptz_para.type, 0);
            Common_Json_SetAttrValue(pArray_root, 7, "Index", Common_Json_Type_Number, NULL, alarm_link->ptz_para.index, 0);
        }
        Common_Json_SetAttrValue(pArray_root, 8, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 8, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 8, "ActionName", Common_Json_Type_String, "LinkFtp", 0, 0);
        if (handle_type&0x80)
        {
            Common_Json_SetAttrValue(pArray_root, 8, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 8, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
        Common_Json_SetAttrValue(pArray_root, 9, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 9, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 9, "ActionName", Common_Json_Type_String, "LinkHttp", 0, 0);
        if (handle_type&0x100)
        {
            Common_Json_SetAttrValue(pArray_root, 9, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 9, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
        if(alarm_link->baudio_para_set)
        {
            Common_Json_SetAttrValue(pArray_root, 10, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
            Common_Json_SetAttrValue(pArray_root, 10, "Channel", Common_Json_Type_Number, NULL, ch, 0);
            Common_Json_SetAttrValue(pArray_root, 10, "ActionName", Common_Json_Type_String, "LinkSound", 0, 0);
            LOGD("set alarm_link:[%d][%d][%d]",alarm_link->baudio_para_set,alarm_link->audio_para.enable,alarm_link->audio_para.index);
            Common_Json_SetAttrValue(pArray_root, 10, "Enable", Common_Json_Type_Number, NULL, alarm_link->audio_para.enable, 0);
            Common_Json_SetAttrValue(pArray_root, 10, "Index", Common_Json_Type_Number, NULL, alarm_link->audio_para.index-1, 0);
        }
        Common_Json_SetAttrValue(pArray_root, 11, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArray_root, 11, "Channel", Common_Json_Type_Number, NULL, ch, 0);
        Common_Json_SetAttrValue(pArray_root, 11, "ActionName", Common_Json_Type_String, "LinkLightAlarm", 0, 0);
        if (handle_type&0x200)
        {
            Common_Json_SetAttrValue(pArray_root, 11, "Enable", Common_Json_Type_Number, NULL, 1, 1);
        }
        else
        {
            Common_Json_SetAttrValue(pArray_root, 11, "Enable", Common_Json_Type_Number, NULL, 0, 0);
        }
    }

    if (0 == iRet)
    {
        iRet = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }
    /*
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    */
    return iRet;
}


#if 1//ndef SIMPLIFIED
/*
static int web_semantic_get_alarmexception(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int i_handletype = 0;
    int sub_type = 0;
    char type[32] = {0};
    OVFS_WEB_ALARMLINK_T alarmPara;

    BOOL isOldAPIReq = FALSE;

    if(NULL == Common_Json_GetAttrValueInt(indata,"type", &sub_type))
    {
        isOldAPIReq = TRUE;
    }

    if(isOldAPIReq)
    {
        sub_type = opt->ch + 1;
    }


    if (2 == sub_type)
    {
        snprintf(type, sizeof(type), "NetCableBreak");
    }
    else if (3 == sub_type)
    {
        snprintf(type, sizeof(type), "IpConflict");
    }
    else if (4 == sub_type)
    {
        snprintf(type, sizeof(type), "IllegallyAcc");
    }
    else if (5 == sub_type)
    {
        snprintf(type, sizeof(type), "DiskErr");
    }
    else if (6 == sub_type)
    {
        snprintf(type, sizeof(type), "DiskFull");
    }
    else
    {
        iRet = WEB_CODE_InvalidArg;
    }

    if (0 == iRet)
    {
        char uri_path[128] = {0};
        snprintf(uri_path, sizeof(uri_path), "/Alarm/LinkageCfg/%s?ActionName=all&&Device=%d&&Channel=%d", type, opt->dev,isOldAPIReq?0:opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

        iRet = ovfs_web_get_alarmlink(header, isOldAPIReq?0:opt->ch, &alarmPara, &i_handletype,opt,NULL);
        Common_Json_SetAttrValue(outdata, -1, "HandleType", Common_Json_Type_Number, NULL, i_handletype, 0);
        Common_Json_SetAttrValue(outdata, -1, "AlarmOut", Common_Json_Type_Number, NULL, alarmPara.alarmout_para.mask0, 0);
    }

    return iRet;
}

static int web_semantic_set_alarmexception(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int i_relalarmout = 0;
    int i_handletype = 0;
    int sub_type = 0;
    char type[32] = {0};
    OVFS_WEB_ALARMLINK_T alarmPara;

    BOOL isOldAPIReq = FALSE;
    cJSON_Struct *lowerData = NULL;
    if (0 == iRet)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            iRet = WEB_CODE_LackingMem;
        }
    }

    if(NULL == Common_Json_GetAttrValueInt(indata,"type", &sub_type))
    {
        isOldAPIReq = TRUE;
    }

    if(isOldAPIReq)
    {
        sub_type = opt->ch + 1;
    }

    memset(&alarmPara, 0, sizeof(OVFS_WEB_ALARMLINK_T));
    if (2 == sub_type)
    {
        snprintf(type, sizeof(type), "NetCableBreak");
    }
    else if (3 == sub_type)
    {
        snprintf(type, sizeof(type), "IpConflict");
    }
    else if (4 == sub_type)
    {
        snprintf(type, sizeof(type), "IllegallyAcc");
    }
    else if (5 == sub_type)
    {
        snprintf(type, sizeof(type), "DiskErr");
    }
    else if (6 == sub_type)
    {
        snprintf(type, sizeof(type), "DiskFull");
    }
    else
    {
        iRet = WEB_CODE_InvalidArg;
    }

    //HandleType
    Common_Json_GetAttrValue(indata, -1, "HandleType", NULL, NULL, &i_handletype, NULL);
    Common_Json_GetAttrValue(indata, -1, "RelAlarmOut", NULL, NULL, &i_relalarmout, NULL);
    if (0 == iRet)
    {
        memset(&alarmPara,0,sizeof(OVFS_WEB_ALARMLINK_T));

        alarmPara.balarmout_para_set = 1;
        alarmPara.alarmout_para.mask0 = 0xff;
        alarmPara.alarmout_para.mask1 = 0xff;
        alarmPara.bmail_para_set = 0;
        alarmPara.brecord_para_set = 1;
        alarmPara.record_para.mask0 = 0x01;
        alarmPara.bsnap_para_set = 0;

        int valueInt;
        alarmPara.snap_para.benable = 1;
        if (Common_Json_GetAttrValueInt(indata, "SnapCount", &valueInt))
        {
            alarmPara.snap_para.count = valueInt;
            alarmPara.bsnap_para_set = 1;
        }
        if (Common_Json_GetAttrValueInt(indata, "SnapInterval", &valueInt))
        {
            alarmPara.snap_para.interval = valueInt;
            alarmPara.bsnap_para_set = 1;
        }
        if (Common_Json_GetAttrValueInt(indata, "SnapChannle", &valueInt))
        {
            alarmPara.snap_para.snap_channle = valueInt;
            alarmPara.bsnap_para_set = 1;
        }

        if (Common_Json_GetAttrValueInt(indata, "EnablePreset", &valueInt))
        {
            alarmPara.ptz_para.type = valueInt;
            alarmPara.bptz_para_set = 1;
        }
        if (Common_Json_GetAttrValueInt(indata, "PresetNo", &valueInt))
        {
            alarmPara.ptz_para.index = valueInt;
            alarmPara.bptz_para_set = 1;
        }
        if (Common_Json_GetAttrValueInt(indata, "AlarmOut", &valueInt))
        {
            alarmPara.alarmout_para.mask0 = valueInt;
            alarmPara.balarmout_para_set = 1;

        }
        char uri_path[128] = {0};
        snprintf(uri_path, sizeof(uri_path), "/Alarm/LinkageCfg/%s?ActionName=all&&Device=%d&&Channel=%d", type, opt->dev,isOldAPIReq?0:opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);

        iRet = ovfs_web_set_alarmlink(header, isOldAPIReq?0:opt->ch, &alarmPara, i_handletype,opt,lowerData);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return iRet;
}*/

static const int s_alarmOutDelayTable[] = {5, 10, 30, 60, 120, 300, 600};

static int web_semantic_get_alarmout(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    char buf[256] = {0};
    int AlarmCh = 0;
    cJSON_Struct *lowerData = NULL;
    if (0 == iRet)
    {

        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/AlarmOut/Ability");

        if ((iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
        {
            int valueInt;
            if (Common_Json_GetAttrValue(lowerData, -1, "AlaramOutNum", NULL, NULL, &valueInt, NULL))
            {
                Common_Json_SetAttrValue(outdata, -1, "AlarmOutCount", Common_Json_Type_Number, NULL, valueInt, valueInt);
            }
        }

        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    if(Common_Json_GetAttrValueInt(indata,"AlarmCh", &AlarmCh))
    {
        opt->ch = AlarmCh - 1;
    }

    if (0 == iRet)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/AlarmOut/Attribute/Channel%d",opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        if ((iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
        {
            int valueInt;

            if (Common_Json_GetAttrValue(lowerData, -1, "TriggerMode", NULL, NULL, &valueInt, NULL))
            {
                Common_Json_SetAttrValue(outdata, -1, "AlarmType", Common_Json_Type_Number, NULL, valueInt, 0);
            }
            //AlarmOutDelay  0-5s 1-10s 2-30s 3-1min 4-2min 5-5min 6-10min
            if (Common_Json_GetAttrValueInt(lowerData, "Delay", &valueInt))
            {
                Common_Json_SetAttrValueInt(outdata, "SelfDefinedDelay", valueInt);

                int i;
                for (i = 0; i < sizeof(s_alarmOutDelayTable)/sizeof(s_alarmOutDelayTable[0]); i++)
                {
                    if (valueInt == s_alarmOutDelayTable[i])
                    {
                        Common_Json_SetAttrValueInt(outdata, "AlarmOutDelay", i);
                        break;
                    }
                }
            }
        }

        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return iRet;
}

static int web_semantic_set_alarmout(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int AlarmCh = 0;

    char buf[256] = {0};
    cJSON_Struct *lowerData = NULL;
    if (0 == iRet)
    {
        if(Common_Json_GetAttrValueInt(indata,"AlarmCh", &AlarmCh))
        {
            opt->ch = AlarmCh - 1;
        }
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            iRet = WEB_CODE_LackingMem;
        }
        else
        {
            int i_num = 0;
            if (Common_Json_GetAttrValue(indata, -1, "AlarmType", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(lowerData, -1, "TriggerMode", Common_Json_Type_Number, NULL, i_num, i_num);
            }

            //AlarmOutDelay  0-5s 1-10s 2-30s 3-1min 4-2min 5-5min 6-10min
            if (Common_Json_GetAttrValueInt(indata, "SelfDefinedDelay", &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "Delay", i_num);
            }
            else if (Common_Json_GetAttrValueInt(indata, "AlarmOutDelay", &i_num))
            {
                i_num = MIN2(i_num, sizeof(s_alarmOutDelayTable)/sizeof(s_alarmOutDelayTable[0])-1);
                Common_Json_SetAttrValueInt(lowerData, "Delay", s_alarmOutDelayTable[i_num]);
            }
        }
    }

    if (0 == iRet)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/AlarmOut/Attribute/Channel%d",opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        iRet = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return iRet;
}

/*int frmAlarmException(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_alarmexception(header, indata, outdata,opt);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_alarmexception(header, indata, outdata,opt);
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
}*/

static int web_semantic_get_alarmout_status(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int AlarmCh = 0;
    char buf[256] = {0};
    cJSON_Struct *lowerData = NULL;
    if (0 == iRet)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/AlarmOut/Ability");
        if ((iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
        {
            int valueInt = 0;
            Common_Json_GetAttrValue(lowerData, -1, "AlaramOutNum", NULL, NULL, &valueInt, NULL);
            if (valueInt == 0)
            {
                LOGE("Alarmout num is Zero!!!Can't get alarmout status \n");
                iRet = WEB_CODE_InternalMistake;
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    if (0 == iRet)
    {
        if(Common_Json_GetAttrValueInt(indata,"AlarmCh", &AlarmCh))
        {
            opt->ch = AlarmCh - 1;
        }
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/AlarmOut/State/Channel%d",opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        if ((iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
        {
            int valueInt;
            if (Common_Json_GetAttrValue(lowerData, -1, "IsEnable", NULL, NULL, &valueInt, NULL))
            {
                Common_Json_SetAttrValue(outdata, -1, "Status", Common_Json_Type_Number, NULL, valueInt, 0);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return iRet;
}
static int web_semantic_enable_alarmout(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int AlarmCh = 0;
    char buf[256] = {0};
    cJSON_Struct *lowerData = NULL;
    if (0 == iRet)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/AlarmOut/Ability");
        if ((iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
        {
            int valueInt = 0;
            Common_Json_GetAttrValue(lowerData, -1, "AlaramOutNum", NULL, NULL, &valueInt, NULL);
            if (valueInt == 0)
            {
                LOGE("Alarmout num is Zero!!!Can't set alarmout enable status \n");
                iRet = WEB_CODE_InternalMistake;
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    if (0 == iRet)
    {
        if(Common_Json_GetAttrValueInt(indata,"AlarmCh", &AlarmCh))
        {
            opt->ch = AlarmCh - 1;
        }
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/AlarmOut/Function/Channel%d/Enable",opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        iRet = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }
    return iRet;
}
static int web_semantic_disable_alarmout(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int iRet = 0;
    int AlarmCh = 0;
    char buf[256] = {0};
    cJSON_Struct *lowerData = NULL;
    if (0 == iRet)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/AlarmOut/Ability");
        if ((iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
        {
            int valueInt = 0;
            Common_Json_GetAttrValue(lowerData, -1, "AlaramOutNum", NULL, NULL, &valueInt, NULL);
            if (valueInt == 0)
            {
                LOGE("Alarmout num is Zero!!!Can't set alarmout enable status \n");
                iRet = WEB_CODE_InternalMistake;
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    if (0 == iRet)
    {
        if(Common_Json_GetAttrValueInt(indata,"AlarmCh", &AlarmCh))
        {
            opt->ch = AlarmCh - 1;
        }
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/AlarmOut/Function/Channel%d/Disable",opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        iRet = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }
    return iRet;
}

int frmAlarmOut(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_alarmout(header, indata, outdata,opt);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_alarmout(header, indata, outdata,opt);
            break;
        case 2://get enable status
            ret = web_semantic_get_alarmout_status(header, indata, outdata,opt);
            break;
        case 3://enable alrmout
            ret = web_semantic_enable_alarmout(header, indata, outdata,opt);
            break;
        case 4://disable alarmout
            ret = web_semantic_disable_alarmout(header, indata, outdata,opt);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1 || opt->type == 3 ||opt->type == 4)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

#endif

static int web_semantic_get_onekeydrivecfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct * lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/alarm/OneKeyDrive/OneKeyDriveCfg");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Duration", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Duration", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "EnableAudio", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "EnableAudio", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "EnableLight", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "EnableLight", i_num);
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_onekeydrivecfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct * lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "Duration", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Duration", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "EnableAudio", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "EnableAudio", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "EnableLight", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "EnableLight", i_num);
        }
    }

    if(0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/alarm/OneKeyDrive/OneKeyDriveCfg");

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }


    return ret;
}

int frmOneKeyDriveCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_onekeydrivecfg(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_onekeydrivecfg(header, indata, outdata);
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

static int web_semantic_get_onekeydrivecontrol(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct * lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/alarm/OneKeyDrive/OneKeyDriveCtl");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "RemainTime", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "RemainTime", i_num);
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_onekeydrivecontrol(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct * lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);
        }
    }

    if(0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/alarm/OneKeyDrive/OneKeyDriveCtl");

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }


    return ret;
}

int frmOneKeyDriveControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
            case 0:
                ret = web_semantic_get_onekeydrivecontrol(header, indata, outdata);
                break;
            case 1:
                //设置参数
                ret = web_semantic_set_onekeydrivecontrol(header, indata, outdata);
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

static int web_semantic_get_alarmpushconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/alarm/pushconfig");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        if (Common_Json_GetAttrValueStr(lowerData, "Url", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "Url", str_tmp);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "PushInterval", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "PushInterval", i_num);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "PushImage", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "PushImage", i_num);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "UpdateImage", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "UpdateImage", i_num);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_alarmpushconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        if (Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "Url", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "Url", str_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "PushInterval", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "PushInterval", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "PushImage", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "PushImage", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "UpdateImage", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "UpdateImage", i_num);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/alarm/pushconfig");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


int frmAlarmPushConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_alarmpushconfig(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_alarmpushconfig(header, indata, outdata);
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

int web_semantic_get_expandalarmout(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int id = -1;
    int i_num = 0;
    double f_tmp = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/AlarmOut/attribute");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        JsonOper_MergeObj(outdata, lowerData, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int web_semantic_set_expandalarmout(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/AlarmOut/attribute");
    ret = Ovfs_Web_RestMethodA(header, indata, NULL, 0);

    return ret;
}

int web_semantic_set_expandalarmout_status(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int dev = 0;
    int ch = 0;
    char *str_tmp = NULL;
    char url[128] = {0};
    cJSON_Struct *lowerData = NULL;

    snprintf(url, sizeof(url), "/Ptz/AlarmOut/Function/%s", opt->type==2?"Enable":"Disable");
    Ovfs_Web_UpdateHeader(header, REST_PUT, url);
    ret = Ovfs_Web_RestMethodA(header, indata, NULL, 0);

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/AlarmOut/attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if(ret == 0)
        {
            cJSON_Struct *chlist = Common_Json_GetAttrValueArr(lowerData, "ChList");
            if(chlist)
            {
                int chsize = Common_Json_ArraySize(chlist);

                cJSON_Struct *list = Common_Json_GetAttrValueArr(indata, "List");
                int size = Common_Json_ArraySize(list);
                for(i=0; i<size; i++)
                {
                    dev = 0;
                    ch = 0;
                    str_tmp = NULL;
                    Common_Json_GetAttrValue(list, i, "Dev", NULL, NULL, &dev, NULL);
                    Common_Json_GetAttrValue(list, i, "Ch", NULL, NULL, &ch, NULL);
                    //LOGW("indata Dev:[%d] Ch:[%d]\n", dev, ch);
                    if(dev == 0 || ch == 0)continue;

                    for(j=0; j<chsize; j++)
                    {
                        int dev_tmp = 0;
                        int ch_tmp = 0;
                        str_tmp = NULL;
                        Common_Json_GetAttrValue(chlist, j, "Dev", NULL, NULL, &dev_tmp, NULL);
                        Common_Json_GetAttrValue(chlist, j, "Ch", NULL, NULL, &ch_tmp, NULL);
                        //LOGW("alarmout Dev:[%d] Ch:[%d]\n", dev_tmp, ch_tmp);
                        if(dev_tmp == dev && ch_tmp == ch)
                        {
                            Common_Json_GetAttrValue(chlist, j, "Name", NULL, &str_tmp, NULL, NULL);
                            //LOGW("j:[%d] Name:[%s]\n", j, str_tmp);
                            break;
                        }
                    }

                    //LOGD("Name:[%s]\n",str_tmp);
                    if(slen(str_tmp)>0)
                    {
                        ovfs_web_write_log(header, "frmExpandAlarmOut", opt, str_tmp);
                    }

                }

            }
        }

        if(lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    return ret;
}

int web_semantic_set_expandalarmout_simple(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, int type)
{
    int ret = 0;
    int method = REST_PUT;
    int i_num = 0;
    int min_cfg_num = 0;
    char url[128] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if(Common_Json_GetAttrValueInt(indata, "Dev", &i_num) == NULL)
    {
        return WEB_CODE_InvalidArg;
    }

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    Common_Json_SetAttrValueInt(lowerData, "Dev", i_num);

    if(Common_Json_GetAttrValueStr(indata, "Name", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "Name", str_tmp);
    }

    snprintf(url, sizeof(url), "/Ptz/AlarmOut/attribute/%s", type == 3?"ch":"dev");

    if(type == 1)
    {
        if(Common_Json_GetAttrValueInt(indata, "Addr", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Addr", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "ChNum", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "ChNum", i_num);
        }

        min_cfg_num = 2;
    }
    else if(type == 2)
    {
        method = REST_DELETE;
    }
    else if(type == 3)
    {
        if(Common_Json_GetAttrValueInt(indata, "Ch", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Ch", i_num);
        }
        else
        {
            return WEB_CODE_InvalidArg;
        }

        if(Common_Json_GetAttrValueInt(indata, "Status", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Status", i_num);
        }

        min_cfg_num = 3;
    }

    if(ret == 0)
    {
        if(Common_Json_Size(lowerData) < min_cfg_num)
        {
            return WEB_CODE_InvalidArg;
        }
    }

    LOGD("ret:[%d] method:[%d] url:[%s]\n", ret, method, url);
    ovfs_print_json(lowerData);

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, method, url);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

typedef struct
{
    int dev;
    cJSON_Struct *chList;
    int ch_idx;
}EXPAND_ALARMOUT_MAP;

int web_semantic_get_expandalarmout_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    EXPAND_ALARMOUT_MAP mapList[4] = {0};

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/AlarmOut/attribute");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        cJSON_Struct *tmp = Common_Json_GetAttrValueArr(lowerData, "DevList");
        cJSON_Struct *devList = Common_Json_Duplicate(tmp, 1);
        Common_Json_AddItem(outdata, -1, "DevList", devList);
        int devSize = Common_Json_ArraySize(devList);
        for(i=0; i<devSize&&i<4; i++)
        {
            Common_Json_GetAttrValue(devList, i, "Dev", NULL, NULL, &mapList[i].dev, NULL);
            mapList[i].chList = Common_Json_SetAttrValue(devList, i, "ChList", Common_Json_Type_Array, NULL, 0, 0);
        }

        cJSON_Struct *pChList = Common_Json_GetAttrValueArr(lowerData, "ChList");
        int size = Common_Json_ArraySize(pChList);
        for(i=0; i<size; i++)
        {
            i_num = 0;
            if(Common_Json_GetAttrValue(pChList, i, "Dev", NULL, NULL, &i_num, NULL) && i_num >0)
            {
                for(j=0; j<4; j++)
                {
                    if(mapList[j].dev == i_num)
                    {
                        cJSON_Struct * pItem = Common_Json_Duplicate(Common_Json_GetAttrValueArrItem(pChList, i), 1);
                        Common_Json_RemoveItem(pItem, -1, "Dev");
                        Common_Json_AddItem(mapList[j].chList, mapList[j].ch_idx++, NULL, pItem);
                        break;
                    }
                }
            }
        }
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int web_semantic_set_expandalarmout_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int i_num = 0;
    int ch_idx = 0;
    cJSON_Struct *lowerData = NULL;

    cJSON_Struct *devList_get = Common_Json_GetAttrValueArr(indata, "DevList");
    if(devList_get == NULL)
    {
        return WEB_CODE_InvalidArg;
    }

    /*if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }*/

    lowerData = Common_Json_Duplicate(indata, 1);

    cJSON_Struct *devList_set = Common_Json_GetAttrValueArr(lowerData, "DevList");
    cJSON_Struct *chList_set = Common_Json_SetAttrValueArr(lowerData, "ChList");
    int devSize = Common_Json_ArraySize(devList_get);
    for(i=0; i<devSize; i++)
    {
        cJSON_Struct *tmp = Common_Json_GetAttrValueArrItem(devList_set, i);
        Common_Json_RemoveItem(tmp, -1, "ChList");
        Common_Json_GetAttrValue(devList_get, i, "Dev", NULL, NULL, &i_num, NULL);
        cJSON_Struct *chlist_tmp = Common_Json_GetAttrValue(devList_get, i, "ChList", NULL, NULL, NULL, NULL);
        int chSize = Common_Json_ArraySize(chlist_tmp);
        for(j=0; j<chSize; j++)
        {
            cJSON_Struct *pItem = Common_Json_Duplicate(Common_Json_GetAttrValueArrItem(chlist_tmp, j), 1);
            Common_Json_SetAttrValueInt(pItem, "Dev", i_num);
            Common_Json_AddItem(chList_set, ch_idx++, NULL, pItem);
        }
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/AlarmOut/attribute");
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int frmExpandAlarmOut(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_expandalarmout(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_expandalarmout(header, indata, outdata);
            break;
        case 2:
        case 3:
            ret = web_semantic_set_expandalarmout_status(header, indata, outdata, opt);
            break;
        case 4:
        case 5:
        case 6:
            ret = web_semantic_set_expandalarmout_simple(header, indata, outdata, opt->type-3);
            break;
        case 10:
            ret = web_semantic_get_expandalarmout_v2(header, indata, outdata);
            break;
        case 11:
            ret = web_semantic_set_expandalarmout_v2(header, indata, outdata);
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

