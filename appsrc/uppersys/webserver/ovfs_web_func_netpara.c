#include "ovfs_web_func.h"

extern uint strtoi(char *s);
extern int query_devchan_streamcount(cJSON_Struct *header, int devIndex, int chanIndex);
extern bool validateip_submask_gateway(char *ip,char *submask,char *gateway);
static int web_semantic_get_emailsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    int i_num =0;
    int iloop = 0;
    char pathname[128] = {0};
    cJSON_Struct *parray_root = NULL;
    cJSON_Struct *pArray_root2 = NULL;

    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Alarm/EmailCfg");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            parray_root = Common_Json_GetAttrValue(lowerData, -1, "ResList", NULL, NULL, NULL, NULL);

            //SenderAddress
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Sender.User", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "SenderAddress", Common_Json_Type_String, str_tmp, 0, 0);
            //Password
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Sender.Password", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "Password", Common_Json_Type_String, str_tmp, 0, 0);
            //EnableSSL
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Sender.EnableSSL", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "EnableSSL", Common_Json_Type_Number, NULL, i_num, i_num);
            //Attachment
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Attachment", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "Attachment", Common_Json_Type_Number, NULL, i_num, i_num);
            //EnableVerify
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Sender.ServerVerify", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "EnableVerify", Common_Json_Type_Number, NULL, i_num, i_num);
            //RecvArray
            Common_Json_SetAttrValue(outdata, -1, "RecvArray", Common_Json_Type_Array, NULL, 0, 0);
            pArray_root2 = Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Receiver", NULL, NULL, NULL, NULL);
            for (iloop=0; iloop<3; iloop++)
            {
                str_tmp = NULL;
                snprintf(pathname, sizeof(pathname), "Receiver[%d].Name", iloop);
                Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(outdata, iloop, "RecvArray.RecvName", Common_Json_Type_String, str_tmp, 0, 0);

                str_tmp = NULL;
                snprintf(pathname, sizeof(pathname), "Receiver[%d].Addr", iloop);
                Common_Json_GetAttrValue(pArray_root2, -1, pathname, NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(outdata, iloop, "RecvArray.RecvAddress", Common_Json_Type_String, str_tmp, 0, 0);
            }
            //MailInterval
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].MailInterval", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "MailInterval", Common_Json_Type_Number, NULL, i_num, i_num);
            //SmtpServer
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Sender.SMTPServer", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "SmtpServer", Common_Json_Type_String, str_tmp, 0, 0);
            //Pop3Server
            Common_Json_SetAttrValue(outdata, -1, "Pop3Server", Common_Json_Type_String, "", 0, 0);
            //SmtpPort
            Common_Json_GetAttrValue(parray_root, -1, "ResList[0].Sender.SMTPPort", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "SmtpPort", Common_Json_Type_Number, NULL, i_num, i_num);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int s_TestEmailRunning = 0;
static char s_TestEmailResult[256] = "Have not start a test email.";

static int web_test_email_proc(Common_Thread_T arga, void *argb)
{
    int ret = 0;

#if 1
    if (s_TestEmailRunning)
    {
        LOGW("Another test email is in the way.\n");
        return 0;
    }
    s_TestEmailRunning = 1;

    snprintf(s_TestEmailResult, sizeof(s_TestEmailResult), "Preparing test email. Please Wait.");

    cJSON_Struct *header = (cJSON_Struct *)argb;

    cJSON_Struct *emailTest = NULL;

    if (0 == ret)
    {
        cJSON_Struct *emailCfg = NULL;
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Alarm/EmailCfg");
        if ((ret = Ovfs_Web_RestMethodA(header, NULL, &emailCfg, 0)) == 0)
        {
            cJSON_Struct *resList = Common_Json_GetAttrValue(emailCfg, -1, "ResList", NULL, NULL, NULL, NULL);
            cJSON_Struct *temp = Common_Json_GetAttrValue(resList, 0, NULL, NULL, NULL, NULL, NULL);

            emailTest = Common_Json_Duplicate(temp, 1);
            //LOGW("\n");
            //Common_Json_StandardPrint(emailTest, NULL, NULL, NULL);

            Common_Json_Delete(emailCfg);
            emailCfg = NULL;
        }
    }

    if (0 == ret)
    {
        cJSON_Struct *networkCfg = NULL;
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetAttr/Eth/0");
        if ((ret = Ovfs_Web_RestMethodA(header, NULL, &networkCfg, 0)) == 0)
        {

            Common_Json_SetAttrValueInt(emailTest, "Sync", 1);

            cJSON_Struct *recvList = Common_Json_GetAttrValueArr(emailTest, "Receiver");
            char *valueStr = NULL;
            if (Common_Json_GetAttrValue(recvList, 0, "Addr", NULL, &valueStr, NULL, NULL))
            {
                Common_Json_SetAttrValue(recvList, 0, "Address", Common_Json_Type_String, valueStr, 0, 0);
            }
            if (Common_Json_GetAttrValue(recvList, 1, "Addr", NULL, &valueStr, NULL, NULL))
            {
                Common_Json_SetAttrValue(recvList, 1, "Address", Common_Json_Type_String, valueStr, 0, 0);
            }
            if (Common_Json_GetAttrValue(recvList, 2, "Addr", NULL, &valueStr, NULL, NULL))
            {
                Common_Json_SetAttrValue(recvList, 2, "Address", Common_Json_Type_String, valueStr, 0, 0);
            }

            char *ipv4addr = NULL;
            Common_Json_GetAttrValueStr(networkCfg, "IpAddrV4", &ipv4addr);
            char *hwaddr = NULL;
            Common_Json_GetAttrValueStr(networkCfg, "MacAddr", &hwaddr);

            char titleBuf[128];
            snprintf(titleBuf, sizeof(titleBuf), "Test email from the camera %s / %s", ipv4addr, hwaddr);
            Common_Json_SetAttrValueStr(emailTest, "Title", titleBuf);

            Common_Json_AddItem(emailTest, -1, "Content", networkCfg);
        }

        //if(networkCfg)
        //{
        //    Common_Json_Delete(networkCfg);
        //    networkCfg = NULL;
        //}
    }

    if (0 == ret)
    {
        //LOGW("\n");
        //Common_Json_StandardPrint(emailTest, NULL, NULL, NULL);

        snprintf(s_TestEmailResult, sizeof(s_TestEmailResult), "Sending test email. Please Wait.");

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/NetWork/Functions/SendMail");
        ret = Ovfs_Web_RestMethodA(header, emailTest, NULL, 0);

        snprintf(s_TestEmailResult, sizeof(s_TestEmailResult), "Result of test email:%d.", ret);
        LOGW("Result of test email:%d.\n", ret);
    }

    if (emailTest)
    {
        Common_Json_Delete(emailTest);
        emailTest = NULL;
    }

    if (s_TestEmailRunning)
    {
        s_TestEmailRunning = 0;
    }
#endif
    return ret;
}

static Common_Thread_T s_ovfs_webTestEmailThread = NULL;
static int web_test_emailsetting(cJSON_Struct *header)
{
    int ret = 0;

    ret = Common_Thread_Create(&s_ovfs_webTestEmailThread, __FUNCTION__, 0, 0, web_test_email_proc, header);

    return ret;
}

static int web_semantic_get_emailtesting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    Common_Json_SetAttrValueStr(outdata, "TestEmailResult", s_TestEmailResult);

    return 0;
}

static int web_semantic_set_emailsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    int i_num = 0;
    int iloop = 0;
    char pathname[128] = {0};
    cJSON_Struct *parray_root = NULL;
    cJSON_Struct *parray_root2 = NULL;
    cJSON_Struct *pobj_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValue(lowerData, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);

        pobj_tmp = Common_Json_SetAttrValue(lowerData, 0, "ResList.Sender", Common_Json_Type_Object, NULL, 0, 0);
        //SenderAddress
        Common_Json_GetAttrValue(indata, -1, "SenderAddress", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(pobj_tmp, -1, "User", Common_Json_Type_String, str_tmp, 0, 0);
        //Password
        Common_Json_GetAttrValue(indata, -1, "Password", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(pobj_tmp, -1, "Password", Common_Json_Type_String, str_tmp, 0, 0);
        //EnableSSL
        Common_Json_GetAttrValue(indata, -1, "EnableSSL", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(pobj_tmp, -1, "EnableSSL", Common_Json_Type_Number, NULL, i_num, 0);
        //Attachment
        //Common_Json_GetAttrValue(indata, -1, "Attachment", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, 0, "ResList.Attachment", Common_Json_Type_Number, NULL, 1, 0);
        //EnableVerify
        Common_Json_GetAttrValue(indata, -1, "EnableVerify", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(pobj_tmp, -1, "ServerVerify", Common_Json_Type_Number, NULL, i_num, 0);
        //RecvArray
        parray_root = Common_Json_GetAttrValue(indata, -1, "RecvArray", NULL, NULL, NULL, NULL);
        parray_root2 = Common_Json_SetAttrValue(lowerData, 0, "ResList.Receiver", Common_Json_Type_Array, NULL, 0, 0);
        if (parray_root2)
        {
            LOGW("Type:%d\n", ((Common_cJSON_T *)parray_root2)->type);
        }
        for (iloop=0; iloop<3; iloop++)
        {
            snprintf(pathname, sizeof(pathname), "RecvArray[%d].RecvName", iloop);
            Common_Json_GetAttrValue(parray_root, -1, pathname, NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(parray_root2, iloop, "Name", Common_Json_Type_String, str_tmp, 0, 0);
            snprintf(pathname, sizeof(pathname), "RecvArray[%d].RecvAddress", iloop);
            Common_Json_GetAttrValue(parray_root, -1, pathname, NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(parray_root2, iloop, "Addr", Common_Json_Type_String, str_tmp, 0, 0);
        }
        //MailInterval
        Common_Json_GetAttrValue(indata, -1, "MailInterval", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, 0, "ResList.MailInterval", Common_Json_Type_Number, NULL, i_num, i_num);
        //SmtpServer
        Common_Json_GetAttrValue(indata, -1, "SmtpServer", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(pobj_tmp, -1, "SMTPServer", Common_Json_Type_String, str_tmp, 0, 0);
        //Pop3Server

        //SmtpPort
        Common_Json_GetAttrValue(indata, -1, "SmtpPort", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(pobj_tmp, -1, "SMTPPort", Common_Json_Type_Number, NULL, i_num, i_num);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/EmailCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        int valueInt = 0;
        if (Common_Json_GetAttrValueInt(indata, "test", &valueInt) && valueInt == 1)
        {
            web_test_emailsetting(header);
        }
    }

    return ret;
}

static int web_semantic_get_ddnsserviceability(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iloop = 0;
    int i_num = 0;
    int ddnsSize = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_root = NULL;

    //ServiceList
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/DDNS");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            //ServiceList
            pArray_root = Common_Json_SetAttrValue(outdata, -1, "ServiceList", Common_Json_Type_Array, NULL, 0, 0);
            pArray_tmp = Common_Json_GetAttrValue(lowerData, -1, "DDNS", NULL, NULL, NULL, NULL);
            ddnsSize = Common_Json_Size(pArray_tmp);
            for (iloop=0; iloop<ddnsSize; iloop++)
            {
                //Describe
                Common_Json_GetAttrValue(pArray_tmp, iloop, "DdnsProtoName", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(pArray_root, iloop, "Describe", Common_Json_Type_String, str_tmp, 0, 0);
                //ServerName
                Common_Json_GetAttrValue(pArray_tmp, iloop, "ServerName", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(pArray_root, iloop, "ServerName", Common_Json_Type_String, str_tmp, 0, 0);
                //Index
                Common_Json_SetAttrValue(pArray_root, iloop, "Index", Common_Json_Type_Number, NULL, iloop, iloop);
                //ServerPort
                Common_Json_GetAttrValue(pArray_tmp, iloop, "DdnsPort", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValue(pArray_root, iloop, "ServerPort", Common_Json_Type_Number, NULL, i_num, i_num);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_netddnspara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    int i_num = 0;
    int HostIndex = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_tmp = NULL;

    if (Common_Json_GetAttrValue(indata, -1, "HostIndex", NULL, NULL, &HostIndex, NULL))
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/DDNS");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            //EnableDDNS
            Common_Json_SetAttrValue(outdata, -1, "EnableDDNS", Common_Json_Type_Number, NULL, i_num, i_num);
            pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "DDNS", NULL, NULL, NULL, NULL);
            if (pArry_tmp)
            {
                //HostIndex
                Common_Json_SetAttrValue(outdata, -1, "HostIndex", Common_Json_Type_Number, NULL, HostIndex, HostIndex);
                //UserName
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, HostIndex, "User", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(outdata, -1, "UserName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                //Password
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, HostIndex, "Password", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(outdata, -1, "Password", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                //DomainName
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, HostIndex, "DomainName", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(outdata, -1, "DomainName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    else
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/DDNS");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "EnableDDNS", Common_Json_Type_Number, NULL, i_num, i_num);

            pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "DDNS", NULL, NULL, NULL, NULL);
            if (pArry_tmp)
            {
                int i;
                int count = Common_Json_ArraySize(pArry_tmp);

                int valueInt = 0;
                cJSON_Struct *ddnsList = Common_Json_SetAttrValueArr(outdata, "DdnsList");
                for (i = 0; i < count; i++)
                {
                    if (Common_Json_GetAttrValue(pArry_tmp, i, "Enable", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(ddnsList, i, "Enable", Common_Json_Type_Number, NULL, valueInt, 0);
                    }

                    if (Common_Json_GetAttrValue(pArry_tmp, i, "User", NULL, &str_tmp, NULL, NULL))
                    {
                        Common_Json_SetAttrValue(ddnsList, i, "UserName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                    }

                    if (Common_Json_GetAttrValue(pArry_tmp, i, "Password", NULL, &str_tmp, NULL, NULL))
                    {
                        Common_Json_SetAttrValue(ddnsList, i, "Password", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                    }

                    if (Common_Json_GetAttrValue(pArry_tmp, i, "DomainName", NULL, &str_tmp, NULL, NULL))
                    {
                        Common_Json_SetAttrValue(ddnsList, i, "DomainName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                    }
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_netddnspara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int hostIndex = 0;

    cJSON_Struct *upperDdnsList = NULL;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "HostIndex", &hostIndex))
        {
        }
        else if ((upperDdnsList = Common_Json_GetAttrValueArr(indata, "DdnsList")) != NULL)
        {
        }
        else
        {
            ret = -1;
        }
    }

    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *lowerDdnsList = NULL;
    int lowerDdnsCount = 0;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/DDNS");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            if ((lowerDdnsList = Common_Json_GetAttrValueArr(lowerData, "DDNS")) != NULL)
            {
                lowerDdnsCount = Common_Json_ArraySize(lowerDdnsList);
            }
        }

    }

    if (0 == ret)
    {
        int valueInt = 0;
        if (Common_Json_GetAttrValueInt(indata, "EnableDDNS", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", valueInt);
        }

        char *valueStr = NULL;
        if (upperDdnsList)
        {
            // ?¨?????§?????|a€￠?′?¤???aDDNS?￥??a€”?¨????
            for (hostIndex = 0; hostIndex < lowerDdnsCount; hostIndex++)
            {
                if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "Enable", NULL, NULL, &valueInt, NULL))
                {
                    Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "Enable", Common_Json_Type_Number, NULL, valueInt, 0);
                }
                if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "UserName", NULL, &valueStr, NULL, NULL))
                {
                    Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "User", Common_Json_Type_String, valueStr, 0, 0);
                }
                if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "Password", NULL, &valueStr, NULL, NULL))
                {
                    Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "Password", Common_Json_Type_String, valueStr, 0, 0);
                }
                if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "DomainName", NULL, &valueStr, NULL, NULL))
                {
                    Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "DomainName", Common_Json_Type_String, valueStr, 0, 0);
                }
            }
        }
        else
        {
            // ?¨?????§?????￥??a€￠?¤???aDDNS.
            if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "Enable", NULL, NULL, &valueInt, NULL))
            {
                Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "Enable", Common_Json_Type_Number, NULL, valueInt, 0);
            }
            if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "UserName", NULL, &valueStr, NULL, NULL))
            {
                Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "User", Common_Json_Type_String, valueStr, 0, 0);
            }
            if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "Password", NULL, &valueStr, NULL, NULL))
            {
                Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "Password", Common_Json_Type_String, valueStr, 0, 0);
            }
            if (Common_Json_GetAttrValue(upperDdnsList, hostIndex, "DomainName", NULL, &valueStr, NULL, NULL))
            {
                Common_Json_SetAttrValue(lowerDdnsList, hostIndex, "DomainName", Common_Json_Type_String, valueStr, 0, 0);
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetApp/DDNS");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_nettelnetpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/Telnetd");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            // Enable
            i_num = 0;
            Common_Json_GetAttrValue(lowerData, -1, "TEnable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);

            // Port
            i_num = 0;
            Common_Json_GetAttrValue(lowerData, -1, "TPort", NULL, NULL, &i_num, NULL);
            if (0 == i_num)
            {
                Common_Json_SetAttrValue(outdata, -1, "Port", Common_Json_Type_Number, NULL, 23, 0);
            }
            else
            {
                Common_Json_SetAttrValue(outdata, -1, "Port", Common_Json_Type_Number, NULL, i_num, 0);
            }
            // Password
            Common_Json_SetAttrValue(outdata, -1, "Password", Common_Json_Type_String, "******", 0, 0);

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_nettelnetpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        i_num = 0;
        Common_Json_GetAttrValue(indata, -1, "Enable", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "TEnable", Common_Json_Type_Number, NULL, i_num, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetApp/Telnetd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_netntppara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    LOCAL_TZ_T local_TZ;
    NET_TZ_T net_TZ;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Time/NTP");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            //EnableNTP
            Common_Json_SetAttrValue(outdata, -1, "EnableNTP", Common_Json_Type_Number, NULL, i_num, i_num);
            Common_Json_GetAttrValue(lowerData, -1, "Server", NULL, &str_tmp, NULL, NULL);
            //NTPServer
            Common_Json_SetAttrValue(outdata, -1, "NTPServer", Common_Json_Type_String, str_tmp, 0, 0);
            Common_Json_GetAttrValue(lowerData, -1, "Interval", NULL, NULL, &i_num, NULL);
            //TimeInterval
            Common_Json_SetAttrValue(outdata, -1, "TimeInterval", Common_Json_Type_Number, NULL, i_num, i_num);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Time/TimeZone");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Zone", NULL, NULL, &local_TZ.TZ, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "EnableBias", NULL, NULL, &local_TZ.bias_enable, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "ZoneBias", NULL, NULL, &local_TZ.bias, NULL);
            web_TZ_translate_lton(&local_TZ, &net_TZ);
            //NTPPort
            Common_Json_SetAttrValue(outdata, -1, "NTPPort", Common_Json_Type_Number, NULL, 123, 123);
            //TimeOffsetHour
            Common_Json_SetAttrValue(outdata, -1, "TimeOffsetHour", Common_Json_Type_Number, NULL, net_TZ.offsetHour, net_TZ.offsetHour);
            //TimeOffsetMinute
            Common_Json_SetAttrValue(outdata, -1, "TimeOffsetMinute", Common_Json_Type_Number, NULL, net_TZ.offsetMinute, net_TZ.offsetMinute);
            Common_Json_SetAttrValue(outdata, -1, "CurrentTimeOffset", Common_Json_Type_Number, NULL, (int)Net_GetTimeDiff(), 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_set_netntppara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int val = 0;
    int autoreboot = 0;
    char *str_tmp = NULL;
    //LOCAL_TZ_T local_TZ;
    //NET_TZ_T net_TZ;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        cJSON_Struct* od = NULL;
        //EnableNTP
        if(Common_Json_GetAttrValueInt(indata, "EnableNTP",  &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData,  "Enable",   i_num);
        }
        //NTPServer
        if(Common_Json_GetAttrValueStr(indata,  "NTPServer",  &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData,  "Server", str_tmp);
        }
        //TimeInterval
        if(Common_Json_GetAttrValueInt(indata, "TimeInterval", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData,  "Interval", i_num);
        }
        if (Common_Json_GetAttrValueInt(indata, "AutoReboot", &autoreboot) == NULL)
        {
            autoreboot = 1;
        }
        Common_Json_SetAttrValueInt(lowerData, "AutoReboot", autoreboot);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Time/NTP");
        ret = Ovfs_Web_RestMethodA(header, lowerData, &od, 0);

        if(od)
        {
            Common_Json_GetAttrValueInt(od, "NeedReboot", &val);
            Common_Json_SetAttrValueInt(outdata, "NeedReboot", val);
            Common_Json_Delete(od);
            od = NULL;
        }




    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        cJSON_Struct* od = NULL;
        NET_TZ_T net_TZ = {0};
        //NTPPort
        //Common_Json_GetAttrValue(indata, -1, "NTPPort", NULL, NULL, &i_num, NULL);
        if( Common_Json_GetAttrValueInt(indata,  "TimeOffsetHour",  &net_TZ.offsetHour)&&
                Common_Json_GetAttrValueInt(indata,  "TimeOffsetMinute",  &net_TZ.offsetMinute)
          )
        {
            LOCAL_TZ_T local_TZ = {0};
            web_TZ_translate_ntol(&net_TZ, &local_TZ);
            Common_Json_SetAttrValue(lowerData, -1, "Zone", Common_Json_Type_Number, NULL, local_TZ.TZ, 0);
            Common_Json_SetAttrValue(lowerData, -1, "EnableBias", Common_Json_Type_Number, NULL, local_TZ.bias_enable, 0);
            Common_Json_SetAttrValue(lowerData, -1, "ZoneBias", Common_Json_Type_Number, NULL, local_TZ.bias, 0);

        }
        //TimeOffsetHour

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Time/TimeZone");
        ret = Ovfs_Web_RestMethodA(header, lowerData, &od, 0);
        if(od)
        {
            Common_Json_GetAttrValueInt(od, "NeedReboot", &val);
            Common_Json_SetAttrValueInt(outdata, "NeedReboot", val);
            Common_Json_Delete(od);
            od = NULL;
        }


    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

MUTEX_TYPE port_check;
int web_semantic_get_port_occupancy(int port)
{
    int ret = 0;
    char cmd[512]= {0};

    if(port<1 || port>65535)
    {
        ret = WEB_CODE_InvalidArg;
    }

    if(ret == 0)
    {
        snprintf(cmd,sizeof(cmd),"netstat -anp | grep LISTEN | grep ':%d ' > /tmp/xxxx",port);
        MUTEX_LOCK(port_check);
        system(cmd);

        FILE *fp;
        fp=fopen("/tmp/xxxx","r");
        int i_eof = -1;
        i_eof = fgetc(fp);
        LOGD("---->I_EOF:%d | port:%d\n",i_eof,port);
        if(!feof(fp))    //返回0非空，返回1为空
        {
            ret = WEB_CODE_PortOccupied;
        }
        fclose(fp);
        memset(cmd,0,sizeof(cmd));
        snprintf(cmd,sizeof(cmd),"rm -rf /tmp/xxxx");
        system(cmd);
        MUTEX_UNLOCK(port_check);
    }

    return ret;

}

static int web_semantic_get_networksettings(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iret = 0;
    char *str_tmp = NULL;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetAttr/Eth/0");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            //NetInterface    ---10/100M?¨a€??a??a??a€??￥?oa€?
            Common_Json_SetAttrValue(outdata, -1, "NetInterface", Common_Json_Type_Number, NULL, 5, 5);

            //UseDhcp
            Common_Json_GetAttrValue(lowerData, -1, "EnableDhcp", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "UseDhcp", Common_Json_Type_Number, NULL, i_num, 0);
            //DVRIP
            Common_Json_GetAttrValue(lowerData, -1, "IpAddrV4", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "DVRIP", Common_Json_Type_String, str_tmp, 0, 0);
            //DvrPort
            Common_Json_SetAttrValue(outdata, -1, "DvrPort", Common_Json_Type_Number, NULL, 8000, 8000);
            //DVRIPMask
            Common_Json_GetAttrValue(lowerData, -1, "IpMaskV4", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "DVRIPMask", Common_Json_Type_String, str_tmp, 0, 0);
            //GatewayIpAddr
            Common_Json_GetAttrValue(lowerData, -1, "GatewayV4", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "GatewayIpAddr", Common_Json_Type_String, str_tmp, 0, 0);
            //MACAddr
            Common_Json_GetAttrValue(lowerData, -1, "MacAddr", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "MACAddr", Common_Json_Type_String, str_tmp, 0, 0);

            Common_Json_GetAttrValue(lowerData, -1, "EnableBindGateway", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "EnableBindGateway", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "GatewayMacAddr", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "GatewayMacAddr", Common_Json_Type_String, str_tmp, 0, 0);

            if(Common_Json_GetAttrValueInt(lowerData, "EnableDhcpV6", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "EnableDhcpV6", i_num);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "IpAddrV6", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "IpAddrV6", str_tmp);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "IpV6PrefixLen", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "IpV6PrefixLen", i_num);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "GatewayV6", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "GatewayV6", str_tmp);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    //RTSP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            //RtspPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RtspPort", Common_Json_Type_Number, NULL, i_num, i_num);

            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.Auth", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RtspAuthMode", Common_Json_Type_Number, NULL, i_num, i_num);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    //RTMP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
        iret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iret)
        {
            //RTMPPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtmp.RtmpPort", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RTMPPort", Common_Json_Type_Number, NULL, i_num, i_num);

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        //HttpPortNo
        Common_Json_SetAttrValue(outdata, -1, "HttpPortNo", Common_Json_Type_Number, NULL, g_ovfs_web->httpport, 0);
        //HttpsPort
        Common_Json_SetAttrValue(outdata, -1, "HttpsPort", Common_Json_Type_Number, NULL, g_ovfs_web->httpsport, 0);
        //MulticastIpAddr
        Common_Json_SetAttrValue(outdata, -1, "MulticastIpAddr", Common_Json_Type_String, "238.255.255.255", 0, 0);
        //MulticastPort
        Common_Json_SetAttrValue(outdata, -1, "MulticastPort", Common_Json_Type_Number, NULL, 28080, 0);

        //DefaultRoute
        Common_Json_SetAttrValue(outdata, -1, "DefaultRoute", Common_Json_Type_Number, NULL, 0, 0);
        //NetworkCardNum
        Common_Json_SetAttrValue(outdata, -1, "NetworkCardNum", Common_Json_Type_Number, NULL, 1, 1);
        //AlarmHostIpAddr
        Common_Json_SetAttrValue(outdata, -1, "AlarmHostIpAddr", Common_Json_Type_String, "", 0, 0);
        //AlarmHostIpPort
        Common_Json_SetAttrValue(outdata, -1, "AlarmHostIpPort", Common_Json_Type_Number, NULL, 0, 0);
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetAttr/DNS");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, -1, "Dns1V4", NULL, &str_tmp, NULL, NULL);
            //DnsServer1IpAddr
            if (str_tmp)
                Common_Json_SetAttrValue(outdata, -1, "DnsServer1IpAddr", Common_Json_Type_String, str_tmp, 0, 0);
            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, -1, "Dns2V4", NULL, &str_tmp, NULL, NULL);
            //DnsServer2IpAddr
            if (str_tmp)
                Common_Json_SetAttrValue(outdata, -1, "DnsServer2IpAddr", Common_Json_Type_String, str_tmp, 0, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

#if 1//ndef SIMPLIFIED
    //EnableUPNP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/SNMP");
        iret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iret)
        {
            //EnableSnmp
            i_num = 0;
            Common_Json_GetAttrValue(lowerData, -1, "SNMPEnable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "EnableSnmp", Common_Json_Type_Number, NULL, i_num, 0);
            //SnmpHostIp
            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, -1, "TrapHostIP", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "SnmpHostIp", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
            //SnmpCount
            i_num = 0;
            Common_Json_GetAttrValue(lowerData, -1, "SendCount", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "SnmpCount", Common_Json_Type_Number, NULL, i_num, 0);
            //SnmpInterval
            i_num = 0;
            Common_Json_GetAttrValue(lowerData, -1, "SendInterval", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "SnmpInterval", Common_Json_Type_Number, NULL, i_num, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetApp/PPPoE");
        iret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iret)
        {
            //EnablePPPOE
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "EnablePPPOE", Common_Json_Type_Number, NULL, i_num, i_num);
            //PPPoEUser
            Common_Json_GetAttrValue(lowerData, -1, "User", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "PPPoEUser", Common_Json_Type_String, str_tmp, 0, 0);
            //PPPoEIP
            Common_Json_GetAttrValue(lowerData, -1, "IpV4", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "PPPoEIP", Common_Json_Type_String, str_tmp, 0, 0);
            //PPPoEPassword
            Common_Json_GetAttrValue(lowerData, -1, "Password", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "PPPoEPassword", Common_Json_Type_String, str_tmp, 0, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
#endif

    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(outdata, "SupportIpv6", g_ovfs_web->support_ipv6);
    }

    return ret;
}

static int web_semantic_set_networksettings(Webs *wp, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iret = 0;
    int i_num = 0;
    int bchanged = 0;
    int rtsp_port = 0;
    int rtmp_port = 0;
    int http_port = 80;
    int https_port = 443;
    char *str_tmp = NULL;
    char str_ip[16] = {0};
    char str_mask[16] = {0};
    char str_gateway[16] = {0};
    cJSON_Struct *pObj_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //RTSP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            //RtspPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &rtsp_port, NULL);
            Common_Json_GetAttrValue(indata, -1, "RtspPort", NULL, NULL, &i_num, NULL);
            if(rtsp_port != i_num)
            {
                ret = web_semantic_get_port_occupancy(i_num);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    //RTMP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
        iret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iret)
        {
            //RTMPPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtmp.RtmpPort", NULL, NULL, &rtmp_port, NULL);
            Common_Json_GetAttrValue(indata, -1, "RTMPPort", NULL, NULL, &i_num, NULL);

            if(rtmp_port != i_num)
            {
                ret = web_semantic_get_port_occupancy(i_num);
            }

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        //HttpPortNo
        Common_Json_GetAttrValue(indata, -1, "HttpPortNo", NULL, NULL, &http_port, NULL);

        //HttpsPort
        Common_Json_GetAttrValue(indata, -1, "HttpsPort", NULL, NULL, &https_port, NULL);
        if (0 == ret)
        {
            if (http_port == https_port)
            {
                ret = WEB_CODE_PortOccupied;
            }
        }

        if (0 == ret)
        {
            if (g_ovfs_web->httpport != http_port)
            {
                ret = web_semantic_get_port_occupancy(http_port);
                if(0 == ret)
                {
                    g_ovfs_web->httpport = http_port;
                    pObj_tmp = Common_Json_GetAttrValue(g_ovfs_config, -1, "HttpPort", NULL, NULL, NULL, NULL);
                    ((Common_cJSON_T *)pObj_tmp)->valueint = g_ovfs_web->httpport;
                    bchanged = 1;
                }
            }
            if (g_ovfs_web->httpsport != https_port)
            {
                ret = web_semantic_get_port_occupancy(https_port);
                if(0 == ret)
                {
                    g_ovfs_web->httpsport = https_port;
                    pObj_tmp = Common_Json_GetAttrValue(g_ovfs_config, -1, "HttpsPort", NULL, NULL, NULL, NULL);
                    ((Common_cJSON_T *)pObj_tmp)->valueint = g_ovfs_web->httpsport;
                    bchanged = 1;
                }
            }
            if (1 == bchanged)
            {
                web_stop_nginx();
                generate_default_ngx_conf();
                web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);
                Access_SaveConfig(g_AccessHandle, g_ovfs_config);
            }
        }
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //NetInterface
    if (0 == ret)
    {
        Common_Json_GetAttrValue(indata, -1, "NetInterface", NULL, NULL, &i_num, NULL);

        Common_Json_SetAttrValue(lowerData, -1, "EthName", Common_Json_Type_String, "eth0", 0, 0);
        //UseDhcp
        Common_Json_GetAttrValue(indata, -1, "UseDhcp", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "EnableDhcp", Common_Json_Type_Number, NULL, i_num, 0);
        //DVRIP
        Common_Json_GetAttrValue(indata, -1, "DVRIP", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "IpAddrV4", Common_Json_Type_String, str_tmp, 0, 0);
        strncpy(str_ip,str_tmp,16);
        if(strcasecmp(wp->ifaddr, str_ip))
        {
            bchanged = 1;
        }
        //DvrPort
        //    Common_Json_GetAttrValue(indata, -1, "DvrPort", NULL, NULL, &i_num, NULL);
        //     LOGI("DvrPort:%d\n", i_num);
        //DVRIPMask
        Common_Json_GetAttrValue(indata, -1, "DVRIPMask", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "IpMaskV4", Common_Json_Type_String, str_tmp, 0, 0);
        strncpy(str_mask,str_tmp,16);
        //GatewayIpAddr
        Common_Json_GetAttrValue(indata, -1, "GatewayIpAddr", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "GatewayV4", Common_Json_Type_String, str_tmp, 0, 0);
        strncpy(str_gateway,str_tmp,16);
        //MACAddr
        Common_Json_GetAttrValue(indata, -1, "MACAddr", NULL, &str_tmp, NULL, NULL);
        LOGI("MACAddr:%s\n", str_tmp);

        Common_Json_GetAttrValue(indata, -1, "EnableBindGateway", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "EnableBindGateway", Common_Json_Type_Number, NULL, i_num, 0);

        Common_Json_GetAttrValue(indata, -1, "GatewayMacAddr", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "GatewayMacAddr", Common_Json_Type_String, str_tmp, 0, 0);

        if(Common_Json_GetAttrValueInt(indata, "EnableDhcpV6", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "EnableDhcpV6", i_num);
        }

        if(Common_Json_GetAttrValueStr(indata, "IpAddrV6", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "IpAddrV6", str_tmp);
        }

        if(Common_Json_GetAttrValueInt(indata, "IpV6PrefixLen", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "IpV6PrefixLen", i_num);
        }

        if(Common_Json_GetAttrValueStr(indata, "GatewayV6", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "GatewayV6", str_tmp);
        }

        if(validateip_submask_gateway(str_ip,str_mask,str_gateway))
        {

            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetAttr/Eth/0");
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
        else
        {

            ret = WEB_CODE_InvalidIpMaskGateway;
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //RtspPort
    if (0 == ret)
    {
        Common_Json_SetAttrValue(lowerData, -1, "Rtsp", Common_Json_Type_Object, NULL, 0, 0);

        Common_Json_GetAttrValue(indata, -1, "RtspPort", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "Rtsp.RtspPort", Common_Json_Type_Number, NULL, i_num, i_num);

        Common_Json_GetAttrValue(indata, -1, "RtspAuthMode", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "Rtsp.Auth", Common_Json_Type_Number, NULL, i_num, i_num);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //RTMPPort
    if (0 == ret && iret == 0)
    {
        Common_Json_GetAttrValue(indata, -1, "RTMPPort", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "Rtmp", Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Rtmp.RtmpPort", Common_Json_Type_Number, NULL, i_num, i_num);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/Rtmp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        //MulticastIpAddr
        Common_Json_GetAttrValue(indata, -1, "MulticastIpAddr", NULL, &str_tmp, NULL, NULL);
        LOGI("MulticastIpAddr:%s\n", str_tmp);
        //MulticastPort
        Common_Json_GetAttrValue(indata, -1, "MulticastPort", NULL, NULL, &i_num, NULL);
        LOGI("MulticastPort:%d\n", i_num);

        //DnsServer1IpAddr
        Common_Json_GetAttrValue(indata, -1, "DnsServer1IpAddr", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "Dns1V4", Common_Json_Type_String, str_tmp, 0, 0);
        //DnsServer2IpAddr
        Common_Json_GetAttrValue(indata, -1, "DnsServer2IpAddr", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "Dns2V4", Common_Json_Type_String, str_tmp, 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetAttr/DNS");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

#if 1//ndef SIMPLIFIED
    if (0 == ret)
    {
        //NetworkCardNum
        Common_Json_GetAttrValue(indata, -1, "NetworkCardNum", NULL, NULL, &i_num, NULL);
        LOGI("NetworkCardNum:%d\n", i_num);

        //AlarmHostIpAddr
        Common_Json_GetAttrValue(indata, -1, "AlarmHostIpAddr", NULL, &str_tmp, NULL, NULL);
        LOGI("AlarmHostIpAddr:%s\n", str_tmp);
        //AlarmHostIpPort
        Common_Json_GetAttrValue(indata, -1, "AlarmHostIpPort", NULL, NULL, &i_num, NULL);
        LOGI("AlarmHostIpPort:%d\n", i_num);

        //EnableSnmp
        i_num = 0;
        if(Common_Json_GetAttrValue(indata, -1, "EnableSnmp", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "SNMPEnable", Common_Json_Type_Number, NULL, i_num, 0);
        }
        //SnmpHostIp
        str_tmp = NULL;
        if(Common_Json_GetAttrValue(indata, -1, "SnmpHostIp", NULL, &str_tmp, NULL, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "TrapHostIP", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
        }
        //SnmpCount
        i_num = 0;
        if(Common_Json_GetAttrValue(indata, -1, "SnmpCount", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "SendCount", Common_Json_Type_Number, NULL, i_num, 0);
        }
        //SnmpInterval
        i_num = 0;
        if(Common_Json_GetAttrValue(indata, -1, "SnmpInterval", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "SendInterval", Common_Json_Type_Number, NULL, i_num, 0);
        }

        if(Common_Json_Size(lowerData)>0)
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetApp/SNMP");
            Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        //EnablePPPOE
        if(Common_Json_GetAttrValue(indata, -1, "EnablePPPOE", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Enable", Common_Json_Type_Number, NULL, i_num, i_num);
        }
        //PPPoEUser
        if(Common_Json_GetAttrValue(indata, -1, "PPPoEUser", NULL, &str_tmp, NULL, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "User", Common_Json_Type_String, str_tmp, 0, 0);
        }
        //PPPoEIP
        if(Common_Json_GetAttrValue(indata, -1, "PPPoEIP", NULL, &str_tmp, NULL, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "IpV4", Common_Json_Type_String, str_tmp, 0, 0);
        }
        //PPPoEPassword
        if(Common_Json_GetAttrValue(indata, -1, "PPPoEPassword", NULL, &str_tmp, NULL, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Password", Common_Json_Type_String, str_tmp, 0, 0);
        }

        if(Common_Json_Size(lowerData)>0)
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetApp/PPPoE");
            Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
#endif
    if (1 == bchanged)
    {
        if(g_ovfs_web->enable_session)
        {
            MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
            int sessionid = strtoi(wp->session_id);
            Common_DList_Delete(g_ovfs_web->userLoginList, (void *)sessionid, web_loginsuccess_nodecompare);
            MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
            if (0 == ret)
            {
                char uri[128];
                snprintf(uri, sizeof(uri), "/Access/OnlineUser?SessionId=%d", strtoi(wp->session_id));

                Ovfs_Web_UpdateHeader(header, REST_DELETE, uri);
                ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
            }
        }
    }
    return ret;
}
static int web_semantic_get_httphttpsconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    //char *str_tmp = NULL;
    //int i_num = 0;
    //cJSON_Struct *lowerData = NULL;



    if (0 == ret)
    {
        //HttpPortNo
        Common_Json_SetAttrValue(outdata, -1, "HttpPortNo", Common_Json_Type_Number, NULL, g_ovfs_web->httpport, g_ovfs_web->httpport);
        //MulticastIpAddr
        Common_Json_SetAttrValue(outdata, -1, "EnableHttp", Common_Json_Type_Number, NULL, g_ovfs_web->enable_http, g_ovfs_web->enable_http);
        //MulticastPort
        Common_Json_SetAttrValue(outdata, -1, "HttpsSupported", Common_Json_Type_Number, NULL, g_ovfs_web->https_support, 0);
        Common_Json_SetAttrValue(outdata, -1, "EnableHttps", Common_Json_Type_Number, NULL, g_ovfs_web->enable_https, g_ovfs_web->enable_https);
        //HttpsPort
        Common_Json_SetAttrValue(outdata, -1, "HttpsPort", Common_Json_Type_Number, NULL, g_ovfs_web->httpsport, g_ovfs_web->httpsport);

        Common_Json_SetAttrValue(outdata, -1, "RedirectHttpToHttps", Common_Json_Type_Number, NULL, g_ovfs_web->enbale_http_redirect_to_https, g_ovfs_web->enbale_http_redirect_to_https);
        //DefaultRoute
        //Common_Json_SetAttrValue(outdata, -1, "DefaultRoute", Common_Json_Type_Number, NULL, 0, 0);
        //NetworkCardNum
        //Common_Json_SetAttrValue(outdata, -1, "NetworkCardNum", Common_Json_Type_Number, NULL, 1, 1);
        //AlarmHostIpAddr
        //Common_Json_SetAttrValue(outdata, -1, "AlarmHostIpAddr", Common_Json_Type_String, "", 0, 0);
        //AlarmHostIpPort
        //Common_Json_SetAttrValue(outdata, -1, "AlarmHostIpPort", Common_Json_Type_Number, NULL, 0, 0);
    }

    return ret;
}

static int web_semantic_set_httphttpsconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int bchanged = 0;
    int enable_http = -1;
    int enable_https = -1;
    int http_port = -1;
    int https_port = -1;
    int redirect_http_to_https = -1;

    if (0 == ret)
    {
        if(Common_Json_GetAttrValue(indata, -1, "EnableHttp", NULL, NULL, &enable_http, NULL))
        {
            if(g_ovfs_web->enable_http != enable_http)
            {
                bchanged = 1;
            }
        }

        if(Common_Json_GetAttrValue(indata, -1, "HttpPortNo", NULL, NULL, &http_port, NULL))
        {
            if(g_ovfs_web->httpport != http_port)
            {
                ret = web_semantic_get_port_occupancy(http_port);

                if(ret != 0)
                {
                    return ret;
                }

                bchanged = 1;
            }
        }

        if(g_ovfs_web->https_support)
        {

            if(Common_Json_GetAttrValue(indata, -1, "EnableHttps", NULL, NULL, &enable_https, NULL))
            {
                if(g_ovfs_web->enable_https != enable_https)
                {
                    bchanged = 1;
                }
            }

            if(Common_Json_GetAttrValue(indata, -1, "HttpsPort", NULL, NULL, &https_port, NULL))
            {
                if(g_ovfs_web->httpsport != https_port)
                {
                    ret = web_semantic_get_port_occupancy(https_port);

                    if(ret != 0)
                    {
                        return ret;
                    }

                    bchanged = 1;
                }
            }

            if(Common_Json_GetAttrValue(indata, -1, "RedirectHttpToHttps", NULL, NULL, &redirect_http_to_https,NULL))
            {
                if(enable_https != -1)
                {
                    if(g_ovfs_web->enable_https == 0)
                    {
                        redirect_http_to_https = 0;
                    }
                }
                else
                {
                    if(enable_https == 0)
                    {
                        redirect_http_to_https= 0;
                    }
                }

                if(g_ovfs_web->enbale_http_redirect_to_https != redirect_http_to_https)
                {
                    bchanged = 1;
                }
            }

        }

        if (0 == ret)
        {
            if (1 == bchanged)
            {
                if((http_port == -1 ? g_ovfs_web->httpport : http_port) ==
                   (https_port == -1 ? g_ovfs_web->httpsport : https_port))
                {
                    return WEB_CODE_PortOccupied;
                }

                if((enable_http == -1 ? g_ovfs_web->enable_http : enable_http) == 0 &&
                   (enable_https == -1 ? g_ovfs_web->enable_https : enable_https) == 0)
                {
                    enable_http = 1;
                    redirect_http_to_https = 0;
                    LOGD("can't disbale both http https service when you set config!\n");
                }

                if(enable_http != -1)
                {
                    g_ovfs_web->enable_http = enable_http;
                    Common_Json_SetAttrValueInt(g_ovfs_config, "EnableHttp", g_ovfs_web->enable_http);
                }
                if(http_port != -1)
                {
                    g_ovfs_web->httpport = http_port;
                    Common_Json_SetAttrValueInt(g_ovfs_config, "HttpPort", g_ovfs_web->httpport);
                }
                if(enable_https != -1)
                {
                    g_ovfs_web->enable_https = enable_https;
                    Common_Json_SetAttrValueInt(g_ovfs_config, "EnableHttps", g_ovfs_web->enable_https);
                }
                if(https_port != -1)
                {
                    g_ovfs_web->httpsport = https_port;
                    Common_Json_SetAttrValueInt(g_ovfs_config, "HttpsPort", g_ovfs_web->httpsport);
                }
                if(redirect_http_to_https != -1)
                {
                    g_ovfs_web->enbale_http_redirect_to_https = redirect_http_to_https;
                    Common_Json_SetAttrValueInt(g_ovfs_config, "EnableHttpRedirectToHttps", g_ovfs_web->enbale_http_redirect_to_https);
                }

                web_stop_nginx();
                generate_default_ngx_conf();
                web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);
                Access_SaveConfig(g_AccessHandle, g_ovfs_config);

                if(g_ovfs_web->enable_session)
                {
                    MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
                    int session_id = 0;
                    if(Common_Json_GetAttrValueInt(header, "Auth/SessionId", &session_id))
                    {
                        Common_DList_Delete(g_ovfs_web->userLoginList, (void *)session_id, web_loginsuccess_nodecompare);
                        MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
                        if (0 == ret)
                        {
                            char uri[128];
                            snprintf(uri, sizeof(uri), "/Access/OnlineUser?SessionId=%d", session_id);

                            Ovfs_Web_UpdateHeader(header, REST_DELETE, uri);
                            ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

                        }
                    }
                }
            }
        }
    }

    return ret;
}

static char multicast_type[][12] = {"Video","Audio"};
static char multicast_sort[][12] = {"Main","Sub","Third","Fourth","Fifth"};
static char multicast_value[][12] = {"IP","Port","TTL"};
static char multicast_show[][12] = {"","Aux","Third","Fourth","Fifth"};

static int web_semantic_get_multicast(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    int streamCount = 0;
    char *str_tmp = NULL;
    char get_labelName[64] = {0};
    char set_labelName[64] = {0};
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);

            int i = 0, j = 0, k = 0;

            for(i = 0; i < streamCount; i++)
            {
                for(j = 0; j < sizeof(multicast_type)/sizeof(multicast_type[0]); j++)
                {
                    for(k = 0; k < sizeof(multicast_value)/sizeof(multicast_value[0]); k++)
                    {
                        memset(get_labelName,0,sizeof(get_labelName));
                        snprintf(get_labelName, sizeof(get_labelName), "Rtsp.MultiCast.%s%s.%s", multicast_sort[i],multicast_type[j],multicast_value[k]);

                        memset(set_labelName,0,sizeof(set_labelName));
                        snprintf(set_labelName, sizeof(set_labelName), "%s%s%s", multicast_show[i],j==0?"":multicast_type[j],multicast_value[k]);

                        if(k == 0)
                        {
                            if(Common_Json_GetAttrValue(lowerData, -1, get_labelName, NULL, &str_tmp, NULL, NULL))
                            {
                                Common_Json_SetAttrValue(outdata, -1, set_labelName, Common_Json_Type_String, str_tmp, 0, 0);
                            }
                        }
                        else
                        {
                            if(Common_Json_GetAttrValue(lowerData, -1, get_labelName, NULL, NULL, &i_num, NULL))
                            {
                                Common_Json_SetAttrValue(outdata, -1, set_labelName, Common_Json_Type_Number, NULL, i_num, 0);
                            }
                        }

                    }
                }

            }

            //Enable
            if(Common_Json_GetAttrValue(lowerData, -1, "Rtsp.MultiCast.Enable", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(outdata, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);
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

static int web_semantic_set_multicast(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    int streamCount = 0;
    char *str_tmp = NULL;
    char get_labelName[64] = {0};
    char set_labelName[64] = {0};

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {

        Common_Json_SetAttrValue(lowerData, -1, "Rtsp", Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Rtsp.MultiCast", Common_Json_Type_Object, NULL, 0, 0);

        streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);

        int i = 0, j = 0, k = 0;

        for(i = 0; i < streamCount; i++)
        {
            for(j = 0; j < sizeof(multicast_type)/sizeof(multicast_type[0]); j++)
            {

                memset(set_labelName,0,sizeof(set_labelName));
                snprintf(set_labelName, sizeof(set_labelName), "Rtsp.MultiCast.%s%s", multicast_sort[i],multicast_type[j]);

                Common_Json_SetAttrValue(lowerData, -1, set_labelName, Common_Json_Type_Object, NULL, 0, 0);

                for(k = 0; k < sizeof(multicast_value)/sizeof(multicast_value[0]); k++)
                {
                    memset(get_labelName,0,sizeof(get_labelName));
                    snprintf(get_labelName, sizeof(get_labelName), "%s%s%s", multicast_show[i],j==0?"":multicast_type[j],multicast_value[k]);

                    memset(set_labelName,0,sizeof(set_labelName));
                    snprintf(set_labelName, sizeof(set_labelName), "Rtsp.MultiCast.%s%s.%s", multicast_sort[i],multicast_type[j],multicast_value[k]);

                    if(k == 0)
                    {
                        if(Common_Json_GetAttrValue(indata, -1, get_labelName, NULL, &str_tmp, NULL, NULL))
                        {
                            Common_Json_SetAttrValue(lowerData, -1, set_labelName, Common_Json_Type_String, str_tmp, 0, 0);
                        }
                    }
                    else
                    {
                        if(Common_Json_GetAttrValue(indata, -1, get_labelName, NULL, NULL, &i_num, NULL))
                        {
                            Common_Json_SetAttrValue(lowerData, -1, set_labelName, Common_Json_Type_Number, NULL, i_num, 0);
                        }
                    }

                }
            }

        }

        //Enable
        if(Common_Json_GetAttrValue(indata, -1, "Enable", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Rtsp.MultiCast.Enable", Common_Json_Type_Number, NULL, i_num, i_num);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_ftpsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/network/netapp/ftp");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {

            if(Common_Json_GetAttrValueInt(lowerData, "FLinkMode",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"FLinkMode",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "FPort",&i_num))
            {
                if(i_num == 0)
                {
                    i_num = 21;
                }
                Common_Json_SetAttrValueInt(outdata,"FPort",i_num);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "FServer",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"FServer",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "FUserName",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"FUserName",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "FPassword",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"FPassword",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "FServerDir",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"FServerDir",str_tmp);
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }


    Ovfs_Web_UpdateHeader(header, REST_GET, "Alarm/ftpcfg");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "SnapInterval", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "SnapInterval", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "SnapStreamIndex", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "SnapStreamIndex", i_num);
        }
        cJSON_Struct *shedule = Common_Json_GetAttrValueObj(lowerData, "shedule");
        if(shedule)
        {
            cJSON_Struct *alarmtime = Common_Json_SetAttrValueObj(outdata, "AlarmTimeV2");
            web_get_alarmtime(shedule, alarmtime);
        }
    }

    return ret;
}

static int web_semantic_set_ftpsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (Common_Json_GetAttrValueInt(indata, "FPort", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "FPort", i_num);
    }

    if (Common_Json_GetAttrValueInt(indata, "FLinkMode", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "FLinkMode", i_num);
    }

    if (Common_Json_GetAttrValueStr(indata, "FServer", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "FServer", str_tmp);
    }

    if (Common_Json_GetAttrValueStr(indata, "FUserName", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "FUserName", str_tmp);
    }

    if (Common_Json_GetAttrValueStr(indata, "FPassword", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "FPassword", str_tmp);
    }

    if (Common_Json_GetAttrValueStr(indata, "FServerDir", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "FServerDir", str_tmp);
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/network/netapp/ftp");
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {

        if(Common_Json_GetAttrValueInt(indata, "SnapInterval",&i_num))
        {
            Common_Json_SetAttrValueInt(lowerData,"SnapInterval",i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "SnapStreamIndex",&i_num))
        {
            Common_Json_SetAttrValueInt(lowerData,"SnapStreamIndex",i_num);
        }

        cJSON_Struct *shedule = Common_Json_SetAttrValueObj(lowerData, "shedule");
        cJSON_Struct *alarmtime = Common_Json_GetAttrValueObj(indata, "AlarmTimeV2");
        web_set_alarmtime(alarmtime, shedule);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "Alarm/FtpCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}
int web_semantic_get_wifisupported(cJSON_Struct *header)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/Wlan/0");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;

}

static int web_semantic_get_managerhostspara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int iloop = 0;
    int protocolSize = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/AccessHost/HostLists");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            cJSON_Struct *lowReslist = Common_Json_GetAttrValue(lowerData, -1, "ResList", NULL, NULL, NULL, NULL);
            protocolSize = Common_Json_Size(lowReslist);

            cJSON_Struct *outProtocols = Common_Json_SetAttrValue(outdata, -1, "Protocols", Common_Json_Type_Array, NULL, 0, 0);
            cJSON_Struct *outPlatform = Common_Json_SetAttrValue(outdata, -1, "Platform", Common_Json_Type_Array, NULL, 0, 0);

            for (iloop = 0; iloop < protocolSize; iloop ++)
            {
                Common_Json_GetAttrValue(lowReslist, iloop, "ServerId", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValue(outProtocols, iloop, "Type", Common_Json_Type_Number, NULL, i_num, 0);
                Common_Json_SetAttrValue(outPlatform, iloop, "Type", Common_Json_Type_Number, NULL, i_num, 0);

                Common_Json_GetAttrValue(lowReslist, iloop, "ServerName", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(outProtocols, iloop, "ProtocolName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);

                Common_Json_GetAttrValue(lowReslist, iloop, "Enable", NULL, NULL, &i_num, NULL);

                //Common_Json_GetAttrValue(lowReslist, iloop, "Statu", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValue(outPlatform, iloop, "EnableManagerHost", Common_Json_Type_Number, NULL, i_num, 0);

                Common_Json_GetAttrValue(lowReslist, iloop, "Fixed", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValue(outPlatform, iloop, "Fixed", Common_Json_Type_Number, NULL, i_num, 0);

                Common_Json_GetAttrValue(lowReslist, iloop, "ParamsCnt", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValue(outProtocols, iloop, "ItemCount", Common_Json_Type_Number, NULL, i_num, 0);

                cJSON_Struct *lowReslistParams = Common_Json_GetAttrValue(lowReslist, iloop, "Params", NULL, NULL, NULL, NULL);
                cJSON_Struct *outProtocolsItemlabels = Common_Json_SetAttrValue(outProtocols, iloop, "ItemLables", Common_Json_Type_Array, NULL, 0, 0);
                cJSON_Struct *outPlatformOtheritemvalues = Common_Json_SetAttrValue(outPlatform, iloop, "OtherItemValues", Common_Json_Type_Array, NULL, 0, 0);
                cJSON_Struct *lowReslistParamsEach = NULL;

                char *nameStr = NULL;
                char *valueStr = NULL;
                int type = 0;
                int index = 0;
                for (lowReslistParamsEach = Common_Json_GetFirstChild(lowReslistParams); lowReslistParamsEach != NULL; lowReslistParamsEach = Common_Json_GetNext(lowReslistParamsEach))
                {
                    Common_Json_GetAttr(lowReslistParamsEach, NULL, &nameStr, &type, &valueStr, NULL, NULL);
                    if (type == Common_Json_Type_String)
                    {
                        Common_Json_SetAttrValueArrStr(outProtocolsItemlabels, index, nameStr);
                        Common_Json_SetAttrValueArrStr(outPlatformOtheritemvalues, index, valueStr);

                        index++;
                    }
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_managerhostspara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int iloop = 0;

    cJSON_Struct *resultData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/AccessHost/HostLists");
        ret = Ovfs_Web_RestMethodA(header, NULL, &resultData, 0);
    }

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //Platform
    if (0 == ret)
    {
        cJSON_Struct *inPlatform = Common_Json_GetAttrValue(indata, -1, "Platform", NULL, NULL, NULL, NULL);
        int platform_num = Common_Json_Size(inPlatform);
        cJSON_Struct *lowReslist = Common_Json_SetAttrValueArr(lowerData, "ResList");
        cJSON_Struct *resultReslist = Common_Json_GetAttrValueArr(resultData, "ResList");
        for (iloop = 0; iloop < platform_num; iloop ++)
        {
            cJSON_Struct *inPlatformEach = Common_Json_GetAttrValueArrItem(inPlatform, iloop);
            cJSON_Struct *lowReslistEach = Common_Json_SetAttrValueArrObj(lowReslist, iloop);

            int serverId = 0;
            if (Common_Json_GetAttrValueInt(inPlatformEach, "Type", &serverId))
            {
                Common_Json_SetAttrValueInt(lowReslistEach, "ServerId", serverId);
            }

            if (Common_Json_GetAttrValueInt(inPlatformEach, "EnableManagerHost", &i_num))
            {
                Common_Json_SetAttrValueInt(lowReslistEach, "Enable", i_num);
            }

            cJSON_Struct *inPlatformItemvalues = Common_Json_GetAttrValueArr(inPlatformEach, "OtherItemValues");
            int inPlatformItemvaluesCount = Common_Json_ArraySize(inPlatformItemvalues);
            if (inPlatformItemvaluesCount == 0)
            {
                continue;
            }

            // ?¤????result?¤???-?|a€°???￥???°?¨?ˉ?￥serverId?§??a€?params?§??a€??|??a€?
            int i;
            int resultResCount = Common_Json_ArraySize(resultReslist);
            cJSON_Struct *resultResListEachParam = NULL;
            for (i = 0; i < resultResCount; i++)
            {
                cJSON_Struct *resultResListEach = NULL;
                resultResListEach = Common_Json_GetAttrValueArrItem(resultReslist, i);
                if (Common_Json_GetAttrValueInt(resultResListEach, "ServerId", &i_num) && i_num == serverId)
                {
                    resultResListEachParam = Common_Json_GetAttrValueObj(resultResListEach, "Params");
                    break;
                }
            }

            int index = 0;
            cJSON_Struct *lowReslistEachParam = Common_Json_SetAttrValueObj(lowReslistEach, "Params");
            cJSON_Struct *resultResListEachParamEach = NULL;
            for (resultResListEachParamEach = Common_Json_GetFirstChild(resultResListEachParam); resultResListEachParamEach != NULL; resultResListEachParamEach = Common_Json_GetNext(resultResListEachParamEach))
            {
                if (index < inPlatformItemvaluesCount)
                {
                    char *nameStr = NULL;
                    char *valueStr = NULL;
                    Common_Json_GetAttr(resultResListEachParamEach, NULL, &nameStr, NULL, NULL, NULL, NULL);
                    Common_Json_GetAttrValue(inPlatformItemvalues, index, NULL, NULL, &valueStr, NULL, NULL);
                    Common_Json_SetAttrValueStr(lowReslistEachParam, nameStr, valueStr);

                    index++;
                }
            }
            Common_Json_SetAttrValueInt(lowReslistEach, "ParamsCnt", index);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/AccessHost/HostLists");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (resultData)
    {
        Common_Json_Delete(resultData);
        resultData = NULL;
    }

    return ret;
}

static int web_semantic_del_managerhostspara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int index = -1;
    cJSON_Struct *pArry_root = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    Common_Json_GetAttrValue(indata, -1, "HostIndex", NULL, NULL, &index, NULL);
    if (0 == ret)
    {
        pArry_root = Common_Json_SetAttrValue(lowerData, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
        Common_Json_SetAttrValue(pArry_root, 0, "ServerId", Common_Json_Type_Number, NULL, index, 0);
        Common_Json_SetAttrValue(pArry_root, 0, "Enable", Common_Json_Type_Number, NULL, 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/AccessHost/HostLists");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


//utils
void web_TZ_translate_lton(LOCAL_TZ_T *local_TZ, NET_TZ_T *net_TZ)
{
    int i_time = 0;
    int nloop = 0;
    int zoneCount = sizeof(arry_zone)/sizeof(arry_zone[0]);

    for (nloop=0; nloop < zoneCount; nloop++)
    {
        if (local_TZ->TZ == arry_zone[nloop].tz_name)
        {
            i_time = arry_zone[nloop].time_offset;
            break;
        }
    }
    if (1 == local_TZ->bias_enable)
    {
        i_time += local_TZ->bias;
    }

    net_TZ->offsetHour = i_time/100;
    net_TZ->offsetMinute = i_time%100;

    return ;
}

void web_TZ_translate_ntol(NET_TZ_T *net_TZ, LOCAL_TZ_T *local_TZ)
{
    int i_time = 0;
    int nloop = 0;
    int zoneCount = sizeof(arry_zone)/sizeof(arry_zone[0]);

    i_time = net_TZ->offsetHour*100 + net_TZ->offsetMinute;
    for (nloop=0; nloop<zoneCount-1; nloop++)
    {
        if (i_time == arry_zone[nloop].time_offset || i_time < arry_zone[nloop+1].time_offset)
        {
            break;
        }
    }

    local_TZ->TZ = arry_zone[nloop].tz_name;
    if (i_time != arry_zone[nloop].time_offset)
    {
        local_TZ->bias_enable = 1;
        local_TZ->bias = i_time - arry_zone[nloop].time_offset;
    }
    else
    {
        local_TZ->bias_enable = 0;
        local_TZ->bias = 0;
    }

    return ;
}

static int web_semantic_get_sippara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    char *valueStr;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/SIP/Config");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    cJSON_Struct *lowerSipConfig = NULL;
    cJSON_Struct *outSipConfig = NULL;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(lowerData, "Enable", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", valueInt);
        }

        if ((lowerSipConfig = Common_Json_GetAttrValueArr(lowerData, "SIPConfig")) != NULL)
        {
            if ((outSipConfig = Common_Json_SetAttrValueArr(outdata, "SIPConfig")) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
        }
    }

    if (0 == ret)
    {
        int lowerSipConfigCount = Common_Json_ArraySize(lowerSipConfig);
        int i;
        for (i = 0; i < lowerSipConfigCount; i++)
        {
            cJSON_Struct *lowerSipConfigEach = Common_Json_GetAttrValueArrItem(lowerSipConfig, i);
            cJSON_Struct *outSipConfigEach = Common_Json_SetAttrValueArrObj(outSipConfig, i);

            if (Common_Json_GetAttrValueStr(lowerSipConfigEach, "SIPName", &valueStr))
            {
                Common_Json_SetAttrValueStr(outSipConfigEach, "SIPName", valueStr);
            }

            if (Common_Json_GetAttrValueInt(lowerSipConfigEach, "Enable", &valueInt))
            {
                Common_Json_SetAttrValueInt(outSipConfigEach, "Enable", valueInt);
            }

            if (Common_Json_GetAttrValueInt(lowerSipConfigEach, "StreamType", &valueInt))
            {
                Common_Json_SetAttrValueInt(outSipConfigEach, "StreamType", valueInt);
            }

            if (Common_Json_GetAttrValueInt(lowerSipConfigEach, "CmdTranType", &valueInt))
            {
                Common_Json_SetAttrValueInt(outSipConfigEach, "CmdTranType", valueInt);
            }

            if (Common_Json_GetAttrValueStr(lowerSipConfigEach, "CallNumber", &valueStr))
            {
                Common_Json_SetAttrValueStr(outSipConfigEach, "CallNumber", valueStr);
            }

            int j;
            for(j=1; j<255; j++)
            {
                char callnumber[16]= {0};
                snprintf(callnumber, sizeof(callnumber), "CallNumber%d", j);

                if (Common_Json_GetAttrValueStr(lowerSipConfigEach, callnumber, &valueStr))
                {
                    Common_Json_SetAttrValueStr(outSipConfigEach, callnumber, valueStr);
                }
                else
                {
                    break;
                }

            }

            if (Common_Json_GetAttrValueInt(lowerSipConfigEach, "CallDelayMSec", &valueInt))
            {
                Common_Json_SetAttrValueInt(outSipConfigEach, "CallDelayMSec", valueInt);
            }



            cJSON_Struct *lowerRegServer = Common_Json_GetAttrValueObj(lowerSipConfigEach, "RegServer");
            if (lowerRegServer)
            {
                cJSON_Struct *outRegServer = Common_Json_SetAttrValueObj(outSipConfigEach, "RegServer");

                if (Common_Json_GetAttrValueStr(lowerRegServer, "Name", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outRegServer, "Name", valueStr);
                }

                if (Common_Json_GetAttrValueStr(lowerRegServer, "Number", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outRegServer, "Number", valueStr);
                }

                if (Common_Json_GetAttrValueStr(lowerRegServer, "UserName", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outRegServer, "UserName", valueStr);
                }

                if (Common_Json_GetAttrValueStr(lowerRegServer, "PassWord", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outRegServer, "PassWord", valueStr);
                }

                if (Common_Json_GetAttrValueStr(lowerRegServer, "IP", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outRegServer, "IP", valueStr);
                }

                if (Common_Json_GetAttrValueInt(lowerRegServer, "Port", &valueInt))
                {
                    Common_Json_SetAttrValueInt(outRegServer, "Port", valueInt);
                }
            }

            cJSON_Struct *lowerSipServer = Common_Json_GetAttrValueObj(lowerSipConfigEach, "SIPServer");
            if (lowerSipServer)
            {
                cJSON_Struct *outSipServer = Common_Json_SetAttrValueObj(outSipConfigEach, "SIPServer");

                if (Common_Json_GetAttrValueStr(lowerSipServer, "IP", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outSipServer, "IP", valueStr);
                }

                if (Common_Json_GetAttrValueInt(lowerSipServer, "Port", &valueInt))
                {
                    Common_Json_SetAttrValueInt(outSipServer, "Port", valueInt);
                }
            }

            cJSON_Struct *lowerMusicCfg = Common_Json_GetAttrValueObj(lowerSipConfigEach, "OnHoldMusicCfg");
            if (lowerMusicCfg)
            {
                cJSON_Struct *outMusicCfg = Common_Json_SetAttrValueObj(outSipConfigEach, "OnHoldMusicCfg");

                if (Common_Json_GetAttrValueStr(lowerMusicCfg, "SoundName", &valueStr))
                {
                    Common_Json_SetAttrValueStr(outMusicCfg, "SoundName", valueStr);
                }

                if (Common_Json_GetAttrValueInt(lowerMusicCfg, "Enable", &valueInt))
                {
                    Common_Json_SetAttrValueInt(outMusicCfg, "Enable", valueInt);
                }

                if (Common_Json_GetAttrValueInt(lowerMusicCfg, "Playtimes", &valueInt))
                {
                    Common_Json_SetAttrValueInt(outMusicCfg, "Playtimes", valueInt);
                }
            }

            cJSON_Struct *lowerDTMF = Common_Json_GetAttrValueObj(lowerSipConfigEach, "DTMF");
            if (lowerDTMF)
            {
                cJSON_Struct *outDTMF = Common_Json_SetAttrValueObj(outSipConfigEach, "DTMF");

                if (Common_Json_GetAttrValueStr(lowerDTMF, "Key_1", &valueStr))
                {
                    //   if(slen(valueStr)>0)
                    //   {
                    //       valueStr = valueStr+1;
                    //   }
                    Common_Json_SetAttrValueStr(outDTMF, "Key_1", valueStr);
                }

                if (Common_Json_GetAttrValueInt(lowerDTMF, "Enable", &valueInt))
                {
                    Common_Json_SetAttrValueInt(outDTMF, "Enable", valueInt);
                }

                if (Common_Json_GetAttrValueInt(lowerDTMF, "DelayTime", &valueInt))
                {
                    Common_Json_SetAttrValueInt(outDTMF, "DelayTime", valueInt);
                }
            }
        }
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

static int web_semantic_set_sippara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    char *valueStr;
    char *str_name = NULL;
    char *str_data = NULL;
    char name_arr[128] = {0};

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    cJSON_Struct *inSipConfig = NULL;
    cJSON_Struct *lowSipConfig = NULL;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Enable", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", valueInt);
        }

        if ((inSipConfig = Common_Json_GetAttrValueArr(indata, "SIPConfig")) != NULL)
        {
            if ((lowSipConfig = Common_Json_SetAttrValueArr(lowerData, "SIPConfig")) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
        }
    }

    if (0 == ret)
    {
        int inSipConfigCount = Common_Json_ArraySize(inSipConfig);
        int i;
        for (i = 0; i < inSipConfigCount; i++)
        {
            cJSON_Struct *inSipConfigEach = Common_Json_GetAttrValueArrItem(inSipConfig, i);
            cJSON_Struct *lowSipConfigEach = Common_Json_SetAttrValueArrObj(lowSipConfig, i);

            if (Common_Json_GetAttrValueStr(inSipConfigEach, "SIPName", &valueStr))
            {
                Common_Json_SetAttrValueStr(lowSipConfigEach, "SIPName", valueStr);
            }

            if (Common_Json_GetAttrValueInt(inSipConfigEach, "Enable", &valueInt))
            {
                Common_Json_SetAttrValueInt(lowSipConfigEach, "Enable", valueInt);
            }

            if (Common_Json_GetAttrValueInt(inSipConfigEach, "StreamType", &valueInt))
            {
                Common_Json_SetAttrValueInt(lowSipConfigEach, "StreamType", valueInt);
            }

            if (Common_Json_GetAttrValueInt(inSipConfigEach, "CmdTranType", &valueInt))
            {
                Common_Json_SetAttrValueInt(lowSipConfigEach, "CmdTranType", valueInt);
            }

            if (Common_Json_GetAttrValueStr(inSipConfigEach, "CallNumber", &valueStr))
            {
                Common_Json_SetAttrValueStr(lowSipConfigEach, "CallNumber", valueStr);
            }

            int j;
            for(j=1; j<255; j++)
            {
                char callnumber[16]= {0};
                snprintf(callnumber, sizeof(callnumber), "CallNumber%d", j);

                if (Common_Json_GetAttrValueStr(inSipConfigEach, callnumber, &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowSipConfigEach, callnumber, valueStr);
                }
                else
                {
                    break;
                }

            }

            if (Common_Json_GetAttrValueInt(inSipConfigEach, "CallDelayMsec", &valueInt))
            {
                Common_Json_SetAttrValueInt(lowSipConfigEach, "CallDelayMsec", valueInt);
            }

            cJSON_Struct *inRegServer = Common_Json_GetAttrValueObj(inSipConfigEach, "RegServer");
            if (inRegServer)
            {
                cJSON_Struct *lowRegServer = Common_Json_SetAttrValueObj(lowSipConfigEach, "RegServer");

                if (Common_Json_GetAttrValueStr(inRegServer, "Name", &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowRegServer, "Name", valueStr);
                }

                if (Common_Json_GetAttrValueStr(inRegServer, "Number", &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowRegServer, "Number", valueStr);
                }

                if (Common_Json_GetAttrValueStr(inRegServer, "UserName", &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowRegServer, "UserName", valueStr);
                }

                if (Common_Json_GetAttrValueStr(inRegServer, "PassWord", &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowRegServer, "PassWord", valueStr);
                }

                if (Common_Json_GetAttrValueStr(inRegServer, "IP", &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowRegServer, "IP", valueStr);
                }

                if (Common_Json_GetAttrValueInt(inRegServer, "Port", &valueInt))
                {
                    Common_Json_SetAttrValueInt(lowRegServer, "Port", valueInt);
                }
            }

            cJSON_Struct *inSipServer = Common_Json_GetAttrValueObj(inSipConfigEach, "SIPServer");
            if (inSipServer)
            {
                cJSON_Struct *lowSipServer = Common_Json_SetAttrValueObj(lowSipConfigEach, "SIPServer");

                if (Common_Json_GetAttrValueStr(inSipServer, "IP", &valueStr))
                {
                    Common_Json_SetAttrValueStr(lowSipServer, "IP", valueStr);
                }

                if (Common_Json_GetAttrValueInt(inSipServer, "Port", &valueInt))
                {
                    Common_Json_SetAttrValueInt(lowSipServer, "Port", valueInt);
                }
            }

            cJSON_Struct *inMusicCfg = Common_Json_GetAttrValueObj(inSipConfigEach, "OnHoldMusicCfg");
            if (inMusicCfg)
            {
                cJSON_Struct *lowMusicCfg = Common_Json_SetAttrValueObj(lowSipConfigEach, "OnHoldMusicCfg");

                if (Common_Json_GetAttrValueStr(inMusicCfg, "SoundName", &str_name))
                {
                    Common_Json_SetAttrValueStr(lowMusicCfg, "SoundName", str_name);
                }

                if (Common_Json_GetAttrValueInt(inMusicCfg, "Enable", &valueInt))
                {
                    Common_Json_SetAttrValueInt(lowMusicCfg, "Enable", valueInt);
                }

                if (Common_Json_GetAttrValueInt(inMusicCfg, "Playtimes", &valueInt))
                {
                    Common_Json_SetAttrValueInt(lowMusicCfg, "Playtimes", valueInt);
                }

                if(Common_Json_GetAttrValueStr(inMusicCfg,"AudioData", &str_data))
                {
                    char *path = "/tmp/";
                    LOGD("str_name:[%s]\n",str_name);
                    if(str_name && slen(str_data)>0)
                    {
                        int len = 0;
                        char *pic_decode = NULL;
                        pic_decode = Common_Base64_Decode(str_data, slen(str_data), &len);
                        LOGD("AudioData len:%d\n",len);

                        FILE *fd;
                        memset(name_arr,0,sizeof(name_arr));
                        snprintf(name_arr,127,"%s%s",path,str_name);
                        LOGD("name_arr:[%s]\n",name_arr);
                        fd = Common_File_fOpen(name_arr, "wb");
                        if(fd)
                        {
                            Common_File_fWrite((void *)pic_decode, 1, len, fd);
                            Common_File_fClose(fd);
                        }

                        Common_Free(pic_decode, __FUNCTION__, __LINE__);
                    }
                    else
                    {
                        path = "";

                    }
                    Common_Json_SetAttrValueStr(lowMusicCfg, "TempPath", name_arr);
                }
            }

            cJSON_Struct *inDTMF = Common_Json_GetAttrValueObj(inSipConfigEach, "DTMF");
            if (inDTMF)
            {
                cJSON_Struct *lowDTMF = Common_Json_SetAttrValueObj(lowSipConfigEach, "DTMF");

                if (Common_Json_GetAttrValueStr(inDTMF, "Key_1", &str_name))
                {
                    //   memset(name_arr,0,sizeof(name_arr));
                    //   snprintf(name_arr,127,"0%s",str_name);
                    Common_Json_SetAttrValueStr(lowDTMF, "Key_1", str_name);
                }

                if (Common_Json_GetAttrValueInt(inDTMF, "Enable", &valueInt))
                {
                    Common_Json_SetAttrValueInt(lowDTMF, "Enable", valueInt);
                }

                if (Common_Json_GetAttrValueInt(inDTMF, "DelayTime", &valueInt))
                {
                    Common_Json_SetAttrValueInt(lowDTMF, "DelayTime", valueInt);
                }

            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/SIP/Config");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

static int web_semantic_get_snmppara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    char *valueStr;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/Netapp/Snmp");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(lowerData, "SNMPEnable", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "SNMPEnable", valueInt);
        }

        if (Common_Json_GetAttrValueStr(lowerData, "SNMPVersion", &valueStr))
        {
            Common_Json_SetAttrValueStr(outdata, "SNMPVersion", valueStr);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "ServerPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "ServerPort", valueInt);
        }

        if (Common_Json_GetAttrValueStr(lowerData, "ReadCommunity", &valueStr))
        {
            Common_Json_SetAttrValueStr(outdata, "ReadCommunity", valueStr);
        }

        if (Common_Json_GetAttrValueStr(lowerData, "WriteCommunity", &valueStr))
        {
            Common_Json_SetAttrValueStr(outdata, "WriteCommunity", valueStr);
        }

        if (Common_Json_GetAttrValueStr(lowerData, "TrapHostIP", &valueStr))
        {
            Common_Json_SetAttrValueStr(outdata, "TrapHostIP", valueStr);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "TrapHostPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "TrapHostPort", valueInt);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "SendCount", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "SendCount", valueInt);
        }

        if (Common_Json_GetAttrValueInt(lowerData, "SendInterval", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "SendInterval", valueInt);
        }

    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

static int web_semantic_set_snmppara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    char *valueStr;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        /*  if (Common_Json_GetAttrValueInt(indata, "SNMPEnable", &valueInt))
          {
              Common_Json_SetAttrValueInt(lowerData, "SNMPEnable", valueInt);
          }

          if (Common_Json_GetAttrValueStr(indata, "TrapHostIP", &valueStr))
          {
              Common_Json_SetAttrValueStr(lowerData, "TrapHostIP", valueStr);
          }

          if (Common_Json_GetAttrValueInt(indata, "SendInterval", &valueInt))
          {
              Common_Json_SetAttrValueInt(lowerData, "SendInterval", valueInt);
          }
          */
        //============================================
        if (Common_Json_GetAttrValueInt(indata, "SNMPEnable", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "SNMPEnable", valueInt);
        }

        if (Common_Json_GetAttrValueStr(indata, "SNMPVersion", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SNMPVersion", valueStr);
        }

        if (Common_Json_GetAttrValueInt(indata, "ServerPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "ServerPort", valueInt);
        }

        if (Common_Json_GetAttrValueStr(indata, "ReadCommunity", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "ReadCommunity", valueStr);
        }

        if (Common_Json_GetAttrValueStr(indata, "WriteCommunity", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "WriteCommunity", valueStr);
        }

        if (Common_Json_GetAttrValueStr(indata, "TrapHostIP", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "TrapHostIP", valueStr);
        }

        if (Common_Json_GetAttrValueInt(indata, "TrapHostPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "TrapHostPort", valueInt);
        }

        if (Common_Json_GetAttrValueInt(indata, "SendCount", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "SendCount", valueInt);
        }

        if (Common_Json_GetAttrValueInt(indata, "SendInterval", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "SendInterval", valueInt);
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/Netapp/Snmp");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

static int web_semantic_get_netupnppara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    int iloop;
    int upnpSize = 0;
    char *valueStr;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *UpnpList_tmp = NULL;
    //cJSON_Struct *str_tmp = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/Netapp/UPnP");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(lowerData, "Enable", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", valueInt);
        }
        UpnpList_tmp = Common_Json_SetAttrValue(outdata, -1, "UpnpList", Common_Json_Type_Array, NULL, 0, 0);
        pArray_tmp = Common_Json_GetAttrValue(lowerData, -1, "Map", NULL, NULL, NULL, NULL);
        upnpSize = Common_Json_Size(pArray_tmp);
        for (iloop=0; iloop<upnpSize; iloop++)
        {
            Common_Json_GetAttrValue(pArray_tmp, iloop, "InPort", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(UpnpList_tmp, iloop, "InPort", Common_Json_Type_Number, NULL, valueInt, 0);
            Common_Json_GetAttrValue(pArray_tmp, iloop, "OutPort", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(UpnpList_tmp, iloop, "OutPort", Common_Json_Type_Number, NULL, valueInt, 0);
            Common_Json_GetAttrValue(pArray_tmp, iloop, "OutIP", NULL, &valueStr, NULL, NULL);
            Common_Json_SetAttrValue(UpnpList_tmp, iloop, "OutIP", Common_Json_Type_String, valueStr, 0, 0);
            Common_Json_GetAttrValue(pArray_tmp, iloop, "TcpOrUdp", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(UpnpList_tmp, iloop, "TcpOrUdp", Common_Json_Type_Number, NULL, valueInt, 0);
            Common_Json_GetAttrValue(pArray_tmp, iloop, "Status", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(UpnpList_tmp, iloop, "Status", Common_Json_Type_Number, NULL, valueInt, 0);
        }
    }
    Common_Json_Delete(lowerData);
    lowerData = NULL;
    return ret;
}
static int web_semantic_set_netupnppara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iloop;
    int valueInt;
    char *valueStr = "eth0";
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Enable", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", valueInt);
        }
        cJSON_Struct *UpnpList_tmp = Common_Json_GetAttrValue(indata, -1, "UpnpList", NULL, NULL, NULL, NULL);
        int upnpSize = Common_Json_Size(UpnpList_tmp);
        cJSON_Struct *pMap_tmp = Common_Json_SetAttrValueArr(lowerData, "Map");
        for (iloop = 0; iloop < upnpSize; iloop ++)
        {
            cJSON_Struct *inUpnpListEach = Common_Json_GetAttrValueArrItem(UpnpList_tmp, iloop);
            cJSON_Struct *pArray_tmp = Common_Json_SetAttrValueArrObj(pMap_tmp, iloop);
            valueInt = 1;
            Common_Json_SetAttrValueInt(pArray_tmp, "Enable", valueInt);
            if (Common_Json_GetAttrValueInt(inUpnpListEach, "InPort", &valueInt))
            {
                Common_Json_SetAttrValueInt(pArray_tmp, "InPort", valueInt);
            }
            if (Common_Json_GetAttrValueInt(inUpnpListEach, "OutPort", &valueInt))
            {
                Common_Json_SetAttrValueInt(pArray_tmp, "OutPort", valueInt);
            }
            if (Common_Json_GetAttrValueInt(inUpnpListEach, "TcpOrUdp", &valueInt))
            {
                Common_Json_SetAttrValueInt(pArray_tmp, "TcpOrUdp", valueInt);
            }
            Common_Json_SetAttrValueStr(pArray_tmp, "Eth", valueStr);
        }
    }
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/Netapp/UPnP");

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }
    Common_Json_Delete(lowerData);
    lowerData = NULL;
    return ret;
}
int frmEmailSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_emailsetting(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_emailsetting(header, indata, outdata);
            break;

        case 2:
            ret = web_semantic_get_emailtesting(header, indata, outdata);
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

int frmNetDDNSPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_netddnspara(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_netddnspara(header, indata, outdata);
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

int frmGetDDNSServiceAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_ddnsserviceability(header, indata, outdata);
    }

    return ret;
}

// Telnet?|?“???￥????
int frmNetTelnetPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        if (opt->type == 0)
        {
            char *queryString = wp->queryString;
            char *begin = NULL;
            // ?￥?|a€??|???“?|???ˉ??a?????¨??a€?url?§??a€??￥??a€??|a€￠?°?|???μ?|?’a€??￥?????|???ˉ?￥???|?￥??a???￥???ˉtelnet,?￥??a?￠???“a???¨?|?????￠a€??￥?¤a€??§??a€??¤??a???¤??a€??￥?1???¤???“?¤???o?¨?????§?????￥??a€??|a€￠?°?￥?¤a€??§??a€?
            if (queryString && (begin = strstr(queryString, "Enable=")) != NULL)
            {
                opt->type = 1;

                int valueInt;
                char *end = NULL;

                begin += strlen("Enable=");
                for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);
                if (end > begin)
                {
                    valueInt = atoi(begin);
                    Common_Json_SetAttrValueInt(indata, "Enable", valueInt);
                }

                //! Port?§a€o???￥a€°???|?“?a?§a€????|a€￠???￥a€o???¤???oweb_semantic_set_nettelnetpara?¤???-?￥?????§a€￠?￥?¤?oa€??|?-?¤?￥a????.
                begin = strstr(queryString, "Port=");
                if (begin)
                {
                    begin += strlen("Port=");
                    for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);
                    if (end > begin)
                    {
                        valueInt = atoi(begin);
                        Common_Json_SetAttrValueInt(indata, "Port", valueInt);
                    }
                }
            }
        }

        switch (opt->type)
        {
        //?¨???·?￥??a€“?￥??a€??|a€￠??
        case 0:
        {
            ret = web_semantic_get_nettelnetpara(header, indata, outdata);
            break;
        }

        //?¨?????§?????￥??a€??|a€￠??
        case 1:
        {
            wp->ext= ".txt";
            ret = web_semantic_set_nettelnetpara(header, indata, outdata);
            break;
        }

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

//?§??a€??§???“NTP
int frmNetNtpPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_netntppara(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_netntppara(header, indata, outdata);
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

int frmNetworkSettings(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_networksettings(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_networksettings(wp, header, indata, outdata);
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

int frmHttpHttpsConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header,3);

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_httphttpsconfig(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_httphttpsconfig(header, indata, outdata);
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

static int web_semantic_get_httppushcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/SmartProtocol");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        Common_Json_GetAttrValueInt(lowerData, "SmartProtocol/Enable", &i_num);
        Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        Common_Json_GetAttrValueStr(lowerData, "SmartProtocol/ServerAddr", &str_tmp);
        Common_Json_SetAttrValueStr(outdata, "ServerAddr", str_tmp);
        Common_Json_GetAttrValueInt(lowerData, "SmartProtocol/ServerPort", &i_num);
        Common_Json_SetAttrValueInt(outdata, "ServerPort", i_num);
        Common_Json_GetAttrValueInt(lowerData, "SmartProtocol/HeartBeatInterval", &i_num);
        Common_Json_SetAttrValueInt(outdata, "HeartBeatInterval", i_num);
        Common_Json_GetAttrValueInt(lowerData, "SmartProtocol/EventListMaxLen", &i_num);
        Common_Json_SetAttrValueInt(outdata, "EventListMaxLen", i_num);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


static int web_semantic_set_httppushcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    //int rtmp_port = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }
    if (0 == ret)
    {
        Common_Json_SetAttrValueObj(lowerData, "SmartProtocol");
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "SmartProtocol/Enable", i_num);
        }
        if(Common_Json_GetAttrValueStr(indata, "ServerAddr", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "SmartProtocol/ServerAddr", str_tmp);
        }
        if(Common_Json_GetAttrValueInt(indata, "ServerPort", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "SmartProtocol/ServerPort", i_num);
        }
        if(Common_Json_GetAttrValueInt(indata, "HeartBeatInterval", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "SmartProtocol/HeartBeatInterval", i_num);
        }
        if(Common_Json_GetAttrValueInt(indata, "EventListMaxLen", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "SmartProtocol/EventListMaxLen", i_num);
        }
    }
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/SmartProtocol");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }


    return ret;
}

int frmHttpPushCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //è?·??–?????°
            ret = web_semantic_get_httppushcfg(header, indata, outdata);
            break;

        case 1:
            //è??????????°
            ret = web_semantic_set_httppushcfg(header, indata, outdata);
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

static int web_semantic_get_httpaddrtest(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iRet = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *outData2 = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;

        }
    }
    if(0 == ret)
    {
        if (Common_Json_GetAttrValueStr(indata, "ServerAddr", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "uri", str_tmp);
        }
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/SmartProtocolUriCheck");
        iRet = Ovfs_Web_RestMethodA(header,lowerData,&outData2, 0);
    }
    if(0 == ret)
    {
        Common_Json_SetAttrValueInt(outdata, "Code", iRet);
        if(iRet != 0)
        {
            if (Common_Json_GetAttrValueStr(outData2, "ErrorInfo", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "ErrorInfo", str_tmp);
            }
            else
            {
                Common_Json_SetAttrValueStr(outdata, "ErrorInfo", "");
            }
        }
    }

    if (outData2)
    {
        Common_Json_Delete(outData2);
        outData2 = NULL;
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}


int frmHttpAddrTest(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        ret = web_semantic_get_httpaddrtest(header, indata, outdata);
    }

    return ret;
}


int frmMulticast(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header,3);
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_multicast(header, indata, outdata,opt);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_multicast(header, indata, outdata,opt);
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

int frmFTPSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_ftpsetting(header, indata, outdata,opt);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_ftpsetting(header, indata, outdata,opt);
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

int web_semantic_get_ltesupported(cJSON_Struct *header)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/LteCfg");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "IsSupported",&i_num))
        {
            ret = i_num-1;
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;

}


static int web_semantic_get_lteparam(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *GetInfo = NULL;
    cJSON_Struct *SetInfo = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/LteCfg");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValueInt(lowerData, "IsSupported",&i_num);
            Common_Json_SetAttrValueInt(outdata,"IsSupported",i_num);

            Common_Json_GetAttrValueInt(lowerData, "Enable",&i_num);
            Common_Json_SetAttrValueInt(outdata,"Enable",i_num);

            GetInfo = Common_Json_GetAttrValueObj(lowerData, "Info");
            SetInfo = Common_Json_SetAttrValueObj(outdata, "Info");
            if(GetInfo)
            {
                Common_Json_GetAttrValueStr(GetInfo, "IMEI",&str);
                Common_Json_SetAttrValueStr(SetInfo,"IMEI",str);

                Common_Json_GetAttrValueInt(GetInfo, "InsertedStatus",&i_num);
                Common_Json_SetAttrValueInt(SetInfo,"InsertedStatus",i_num);

                Common_Json_GetAttrValueStr(GetInfo, "CCID",&str);
                Common_Json_SetAttrValueStr(SetInfo,"CCID",str);

                Common_Json_GetAttrValueStr(GetInfo, "NetInfo",&str);
                Common_Json_SetAttrValueStr(SetInfo,"NetInfo",str);

                Common_Json_GetAttrValueStr(GetInfo, "ServiceInfo",&str);
                Common_Json_SetAttrValueStr(SetInfo,"ServiceInfo",str);

                Common_Json_GetAttrValueInt(GetInfo, "SignalStrength",&i_num);
                Common_Json_SetAttrValueInt(SetInfo,"SignalStrength",i_num);

                Common_Json_GetAttrValueInt(GetInfo, "LinkStatus",&i_num);
                Common_Json_SetAttrValueInt(SetInfo,"LinkStatus",i_num);

                Common_Json_GetAttrValueStr(GetInfo, "IpAddrV4",&str);
                Common_Json_SetAttrValueStr(SetInfo,"IpAddrV4",str);

                Common_Json_GetAttrValueStr(GetInfo, "DNS1V4",&str);
                Common_Json_SetAttrValueStr(SetInfo,"DNS1V4",str);

                Common_Json_GetAttrValueStr(GetInfo, "DNS2V4",&str);
                Common_Json_SetAttrValueStr(SetInfo,"DNS2V4",str);

                Common_Json_GetAttrValueStr(GetInfo, "IpAddrV6",&str);
                Common_Json_SetAttrValueStr(SetInfo,"IpAddrV6",str);

                Common_Json_GetAttrValueStr(GetInfo, "DNS1V6",&str);
                Common_Json_SetAttrValueStr(SetInfo,"DNS1V6",str);

                Common_Json_GetAttrValueStr(GetInfo, "DNS2V6",&str);
                Common_Json_SetAttrValueStr(SetInfo,"DNS2V6",str);

                if(Common_Json_GetAttrValueInt(GetInfo, "ApnType",&i_num))
                {
                    Common_Json_SetAttrValueInt(SetInfo,"ApnType",i_num);
                }

                if(Common_Json_GetAttrValueStr(GetInfo, "ApnStr",&str))
                {
                    Common_Json_SetAttrValueStr(SetInfo,"ApnStr",str);
                }

                if(Common_Json_GetAttrValueInt(GetInfo, "StateOfCharge",&i_num))
                {
                    Common_Json_SetAttrValueInt(SetInfo,"ChargeState",i_num);
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

static int web_semantic_set_lteparam(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    //int val = 0;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }
    if(Common_Json_GetAttrValueInt(indata,"Enable",&i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);
    }

    cJSON_Struct *info = Common_Json_SetAttrValueObj(lowerData, "Info");

    if(Common_Json_GetAttrValueInt(indata,"Info/ApnType",&i_num))
    {
        Common_Json_SetAttrValueInt(info, "ApnType", i_num);
    }

    if(Common_Json_GetAttrValueStr(indata,"Info/ApnStr",&str_tmp))
    {
        Common_Json_SetAttrValueStr(info, "ApnStr", str_tmp);
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/NetWork/NetAttr/LteCfg");
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

//4G
int frmNetLtepara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    ret = web_semantic_get_ltesupported(header);
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //????
            ret = web_semantic_get_lteparam(header, indata, outdata);
            break;

        case 1:
            //????
            ret = web_semantic_set_lteparam(header, indata, outdata);
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

static int web_semantic_get_ltecardinfo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *GetInfo = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/LteCfg");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {

        	GetInfo = Common_Json_GetAttrValueObj(lowerData, "Info");
            if(GetInfo)
            {
                Common_Json_GetAttrValueStr(GetInfo, "IMEI",&str);
                Common_Json_SetAttrValueStr(outdata,"IMEI",str);

                Common_Json_GetAttrValueStr(GetInfo, "CCID",&str);
                Common_Json_SetAttrValueStr(outdata,"CCID",str);

                Common_Json_GetAttrValueInt(GetInfo, "LinkStatus",&i_num);
                Common_Json_SetAttrValueInt(outdata,"LinkStatus",i_num);

                Common_Json_GetAttrValueInt(GetInfo, "SignalStrength",&i_num);
                Common_Json_SetAttrValueInt(outdata,"SignalStrength",i_num);

                if(Common_Json_GetAttrValueInt(GetInfo, "StateOfCharge",&i_num))
                {
                    Common_Json_SetAttrValueInt(outdata,"ChargeState",i_num);
                }

            }

            i_num = 0;
            Common_Json_GetAttrValueInt(lowerData, "ErrorCode",&i_num);
            Common_Json_SetAttrValueInt(outdata,"ErrorCode",i_num);

        }

        if(lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }

    }

    return ret;
}

int frmGetLteCardInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
 		switch (opt->type)
		{
		case 0:
            ret = web_semantic_get_ltecardinfo(header, indata, outdata);
			break;

		default:
            ret = WEB_CODE_InvalidArg;
			break;
		}
	}

    return ret;
}

int frmGetManagerHostsPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_managerhostspara(header, indata, outdata);
            break;

        case 1:
            //?￥??????a?￠??
            ret = web_semantic_del_managerhostspara(header, indata, outdata);
            break;

        case 2:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_managerhostspara(header, indata, outdata);
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

int frmNetSipPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_sippara(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_sippara(header, indata, outdata);
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

int frmNetSnmp(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_snmppara(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_snmppara(header, indata, outdata);
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


static int web_semantic_get_mediadisconnect(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    char *valueStr;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *tmp = NULL;

    //cJSON_Struct* d = NULL;
    //cJSON_Struct *property = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/mediaserver/mediaDisconnect/attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if(ret == 0)
        {
            tmp = Common_Json_GetAttrValueObj(lowerData, "MediaDisconnect");
            if (tmp)
            {
                //	d = Common_Json_SetAttrValueObj(outdata, "MediaDisconnect");

                Common_Json_GetAttrValueStr(tmp,"DisconnectIP",(S8**)&valueStr);
                Common_Json_SetAttrValueStr(outdata, "DisconnectIP", valueStr);

                Common_Json_GetAttrValueInt(tmp,"DeviceId",(S32*)&valueInt);
                Common_Json_SetAttrValueInt(outdata, "DeviceId", valueInt);

                Common_Json_SetAttrValueInt(tmp,"ChannelId",valueInt);
                Common_Json_SetAttrValueInt(outdata, "ChannelId", valueInt);

                Common_Json_SetAttrValueInt(tmp,"StreamId",valueInt);
                Common_Json_SetAttrValueInt(outdata, "StreamId", valueInt);
            }

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

static int web_semantic_set_mediadisconnect(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int valueInt;
    char *valueStr;

    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *obj = NULL;
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
        obj = Common_Json_SetAttrValueObj(lowerData, "MediaDisconnect");

        if (Common_Json_GetAttrValueStr(indata, "DisconnectIP", &valueStr))
        {
            Common_Json_SetAttrValueStr(obj, "DisconnectIP", valueStr);
        }

        if (Common_Json_GetAttrValueInt(indata, "DeviceId", &valueInt))
        {
            Common_Json_SetAttrValueInt(obj, "DeviceId", valueInt);
        }

        if (Common_Json_GetAttrValueInt(indata, "ChannelId", &valueInt))
        {
            Common_Json_SetAttrValueInt(obj, "ChannelId", valueInt);
        }

        if (Common_Json_GetAttrValueInt(indata, "StreamId", &valueInt))
        {
            Common_Json_SetAttrValueInt(obj, "StreamId", valueInt);
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/mediaserver/mediaDisconnect/attribute");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;
    return ret;
}



int frmMediaDisconnect(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_mediadisconnect(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_mediadisconnect(header, indata, outdata);
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

int frmNetUPNPPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_netupnppara(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_netupnppara(header, indata, outdata);
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

static int web_semantic_get_httpeventsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/network/netapp/http");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {

            if(Common_Json_GetAttrValueInt(lowerData, "HPort",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"HPort",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "HType",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"HType",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "HAuthMode",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"HAuthMode",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "HSSL",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"HSSL",i_num);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "HServer",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"HServer",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "HUserName",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"HUserName",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "HPassword",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"HPassword",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "HParam",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"HParam",str_tmp);
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_httpeventsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (Common_Json_GetAttrValueInt(indata, "HPort", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "HPort", i_num);
    }

    if (Common_Json_GetAttrValueInt(indata, "HType", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "HType", i_num);
    }

    if (Common_Json_GetAttrValueInt(indata, "HAuthMode", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "HAuthMode", i_num);
    }

    if (Common_Json_GetAttrValueInt(indata, "HSSL", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "HSSL", i_num);
    }

    if (Common_Json_GetAttrValueStr(indata, "HServer", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "HServer", str_tmp);
    }

    if (Common_Json_GetAttrValueStr(indata, "HUserName", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "HUserName", str_tmp);
    }

    if (Common_Json_GetAttrValueStr(indata, "HPassword", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "HPassword", str_tmp);
    }

    if (Common_Json_GetAttrValueStr(indata, "HParam", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "HParam", str_tmp);
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/network/netapp/http");
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


int frmHttpEventSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_httpeventsetting(header, indata, outdata);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_httpeventsetting(header, indata, outdata);
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

static int web_semantic_get_rtmppushcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int i = 0;
    int j = 0;
    int ret = 0;
    int i_num = 0;
    char str_arr[16] = {0};
    char uri_path[128] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        Common_Json_GetAttrValue(lowerData, -1, "Rtmp.Enable", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(outdata, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);
        //RTMPPort
        Common_Json_GetAttrValue(lowerData, -1, "Rtmp.RtmpPort", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(outdata, -1, "RTMPPort", Common_Json_Type_Number, NULL, i_num, 0);

    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/mediaserver/rtmppush/attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            cJSON_Struct *pPushData = Common_Json_SetAttrValueObj(outdata, "PushCfg");
            for(i=0; i<4; i++)
            {
                memset(uri_path,0,sizeof(char)*128);
                snprintf(uri_path, sizeof(uri_path), "RtmpPush/Client%d",i);
                cJSON_Struct *pClient = Common_Json_GetAttrValueObj(lowerData, uri_path);
                if(pClient)
                {
                    memset(str_arr,0,sizeof(char)*16);
                    snprintf(str_arr, sizeof(str_arr), "Client%d",i);
                    cJSON_Struct *pPushClient = Common_Json_SetAttrValueObj(pPushData, str_arr);
                    snprintf(uri_path + slen(uri_path), sizeof(uri_path), "/Device%d/Channel%d",opt->dev,opt->ch);
                    cJSON_Struct *pCh = Common_Json_GetAttrValueObj(lowerData, uri_path);
                    if(pCh)
                    {
                        int streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);
                        int thirdStreamSupport = 1;
                        //Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "/CVStreamSelect/thirdStream/visible", &thirdStreamSupport);
                        if(thirdStreamSupport == 0 && streamCount == 3)
                        {
                            streamCount = 2;
                        }
                        for(j=0; j<streamCount; j++)
                        {
                            memset(str_arr,0,sizeof(char)*16);
                            snprintf(str_arr, sizeof(str_arr), "Stream%d",j);
                            cJSON_Struct *pStream = Common_Json_GetAttrValueObj(pCh, str_arr);
                            if(pStream)
                            {
                                cJSON_Struct *pPushStream = Common_Json_SetAttrValueObj(pPushClient, str_arr);
                                if (Common_Json_GetAttrValueInt(pStream, "Enable", &i_num))
                                {
                                    Common_Json_SetAttrValueInt(pPushStream, "Enable", i_num);
                                }

                                if (Common_Json_GetAttrValueInt(pStream, "EnableAudio", &i_num))
                                {
                                    Common_Json_SetAttrValueInt(pPushStream, "EnableAudio", i_num);
                                }

                                if (Common_Json_GetAttrValueStr(pStream, "Url", &str_tmp))
                                {
                                    Common_Json_SetAttrValueStr(pPushStream, "Url", str_tmp);
                                }
                            }
                        }
                    }

                }
            }

        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_rtmppushcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int i = 0;
    int j = 0;
    int ret = 0;
    int i_num = 0;
    int rtmp_port = 0;
    char uri_path[128] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if(Common_Json_GetAttrValue(indata, -1, "RTMPPort", NULL, NULL, &i_num, NULL))
        {
            Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
            if (0 == ret)
            {
                //RTMPPort
                Common_Json_GetAttrValue(lowerData, -1, "Rtmp.RtmpPort", NULL, NULL, &rtmp_port, NULL);

                if(rtmp_port != i_num)
                {
                    ret = web_semantic_get_port_occupancy(i_num);
                }

                if(0 == ret)
                {
                    Common_Json_SetAttrValue(lowerData, -1, "Rtmp", Common_Json_Type_Object, NULL, 0, 0);
                    Common_Json_SetAttrValue(lowerData, -1, "Rtmp.RtmpPort", Common_Json_Type_Number, NULL, i_num, 0);
                    if(Common_Json_GetAttrValue(indata, -1, "Enable", NULL, NULL, &i_num, NULL))
                    {
                        Common_Json_SetAttrValue(lowerData, -1, "Rtmp.Enable", Common_Json_Type_Number, NULL, i_num, 0);
                    }

                    Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/Rtmp/Attribute");
                    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

                }

            }
        }
        if(lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }

    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueObj(indata, "PushCfg"))
        {
            Common_Json_SetAttrValueObj(lowerData, "RtmpPush");
            for(i=0; i<4; i++)
            {
                memset(uri_path,0,sizeof(char)*32);
                snprintf(uri_path, sizeof(uri_path), "PushCfg/Client%d",i);
                cJSON_Struct *pClient = Common_Json_GetAttrValueObj(indata, uri_path);
                if(pClient)
                {
                    memset(uri_path,0,sizeof(char)*128);
                    snprintf(uri_path, sizeof(uri_path), "RtmpPush/Client%d",i);
                    Common_Json_SetAttrValueObj(lowerData, uri_path);
                    snprintf(uri_path + slen(uri_path), sizeof(uri_path), "/Device%d",opt->dev);
                    Common_Json_SetAttrValueObj(lowerData, uri_path);
                    snprintf(uri_path + slen(uri_path), sizeof(uri_path), "/Channel%d",opt->ch);
                    cJSON_Struct *pPushCh = Common_Json_SetAttrValueObj(lowerData, uri_path);
                    int streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);
                    /*int thirdStreamSupport = 0;
                    Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "/CVStreamSelect/thirdStream/visible", &thirdStreamSupport);
                    if(thirdStreamSupport == 0 && streamCount == 3){
                        streamCount = 2;
                    }*/
                    for(j=0; j<streamCount; j++)
                    {
                        memset(uri_path,0,sizeof(char)*128);
                        snprintf(uri_path, sizeof(uri_path), "Stream%d",j);
                        cJSON_Struct *pStream = Common_Json_GetAttrValueObj(pClient, uri_path);
                        if(pStream)
                        {
                            cJSON_Struct *pPushStream = Common_Json_SetAttrValueObj(pPushCh, uri_path);
                            if(Common_Json_GetAttrValue(pStream, -1, "Enable", NULL, NULL, &i_num, 0))
                            {
                                Common_Json_SetAttrValue(pPushStream, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);
                            }
                            if(Common_Json_GetAttrValue(pStream, -1, "EnableAudio", NULL, NULL, &i_num, 0))
                            {
                                Common_Json_SetAttrValue(pPushStream, -1, "EnableAudio", Common_Json_Type_Number, NULL, i_num, 0);
                            }
                            if(Common_Json_GetAttrValue(pStream, -1, "Url", NULL, &str_tmp, 0, 0))
                            {
                                Common_Json_SetAttrValue(pPushStream, -1, "Url", Common_Json_Type_String, str_tmp, 0, 0);
                            }
                        }

                    }

                }

            }

            Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/RtmpPush/Attribute");
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_rtspcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    //char str_arr[16] = {0};
    //char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        Common_Json_GetAttrValue(lowerData, -1, "Rtsp.Enable", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(outdata, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);
        //RTMPPort
        Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(outdata, -1, "RTSPPort", Common_Json_Type_Number, NULL, i_num, 0);

        Common_Json_GetAttrValue(lowerData, -1, "Rtsp.HttpPort", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(outdata, -1, "RtspHttpPort", Common_Json_Type_Number, NULL, i_num, 0);

        Common_Json_GetAttrValue(lowerData, -1, "Rtsp.Auth", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(outdata, -1, "Auth", Common_Json_Type_Number, NULL, i_num, 0);

    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;

}

static int web_semantic_set_rtspcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int rtsp_port = 0;
    //char str_arr[16] = {0};
    //char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    //RTSP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            //RtspPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &rtsp_port, NULL);
            LOGD("rtsp_port:[%d]\n",rtsp_port);
            if(Common_Json_GetAttrValue(indata, -1, "RTSPPort", NULL, NULL, &i_num, NULL))
            {
                LOGD("i_num:[%d]\n",i_num);
                if(rtsp_port != i_num)
                {
                    ret = web_semantic_get_port_occupancy(i_num);
                    LOGD("ret:[%d]\n",ret);
                }
                if(0 == ret)
                {
                    Common_Json_SetAttrValue(lowerData, -1, "Rtsp.RtspPort", Common_Json_Type_Number, NULL, i_num, 0);
                }
            }

            if(0 == ret)
            {

                if(Common_Json_GetAttrValue(indata, -1, "Enable", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValue(lowerData, -1, "Rtsp.Enable", Common_Json_Type_Number, NULL, i_num, 0);
                }

                if(Common_Json_GetAttrValue(indata, -1, "RtspHttpPort", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValue(lowerData, -1, "Rtsp.HttpPort", Common_Json_Type_Number, NULL, i_num, 0);
                }

                if(Common_Json_GetAttrValue(indata, -1, "Auth", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValue(lowerData, -1, "Rtsp.Auth", Common_Json_Type_Number, NULL, i_num, 0);
                }
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/MediaServer/Rtsp/Attribute");
                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

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

static char p2p_name_list[][16] = {"/AliIoT4ovfs", "/AwsIoT4ovfs", "/Umeye4ovfs"};

int get_p2p_index(cJSON_Struct *header)
{
    static int index = -1;
    int ret = 0;
    int i = 0;
    int j = 0;
    int need_return = 0;
    char *strTmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if(index != -1)
    {
        LOGD("index:[%d]\n",index);
        return index;
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, "/");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "ResList");
        int size = Common_Json_ArraySize(list);
        for(i=0; i<size; i++)
        {
            Common_Json_GetAttrValue(list, i, "Uri", NULL, &strTmp, NULL, NULL);

            for(j=0; j<sizeof(p2p_name_list)/sizeof(p2p_name_list[0]); j++)
            {
                if(scaselessmatch(strTmp, p2p_name_list[j]))
                {
                    ret = j;
                    index = j;
                    need_return = 1;
                    break;
                }
            }

            if(need_return == 1)break;
        }
    }
    else
    {
        ret = 0;
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    return ret;

}

static int web_semantic_get_aliiotcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int index = 0;
    char *str_tmp = NULL;
    char url[128] = {0};
    cJSON_Struct *lowerData = NULL;

    index = get_p2p_index(header);

    if (0 == ret)
    {
        snprintf(url, sizeof(url), "%s/iotcfg", p2p_name_list[index]);
        Ovfs_Web_UpdateHeader(header, REST_GET, url);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_SetAttrValueStr(outdata, "Name", p2p_name_list[index]+1);

            if(Common_Json_GetAttrValueInt(lowerData, "EnableIoT",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"EnableIoT",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "EnableAlarm",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"EnableAlarm",i_num);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "Version",&str_tmp))
			{
                Common_Json_SetAttrValueStr(outdata,"Version",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "BuildDate",&str_tmp))
			{
                Common_Json_SetAttrValueStr(outdata,"BuildDate",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "QRCode",&str_tmp))
			{
                Common_Json_SetAttrValueStr(outdata,"QRCode",str_tmp);
            }

        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_aliiotcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int index = 0;
    char url[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (Common_Json_GetAttrValueInt(indata, "EnableIoT", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "EnableIoT", i_num);
    }

    if (Common_Json_GetAttrValueInt(indata, "EnableAlarm", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "EnableAlarm", i_num);
    }

    index = get_p2p_index(header);
    snprintf(url, sizeof(url), "%s/iotcfg", p2p_name_list[index]);

    Ovfs_Web_UpdateHeader(header, REST_PUT, url);
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmAliIoTCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
		switch (opt->type)
		{
		case 0:
			//获取参数
            ret = web_semantic_get_aliiotcfg(header, indata, outdata);
			break;

		case 1:
			//设置参数
            ret = web_semantic_set_aliiotcfg(header, indata, outdata);
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

static int web_semantic_get_aliiotstate(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int index = 0;
    char url[128] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        index = get_p2p_index(header);
        snprintf(url, sizeof(url), "%s/iotcfg", p2p_name_list[index]);
        Ovfs_Web_UpdateHeader(header, REST_GET, url);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {

            if(Common_Json_GetAttrValueInt(lowerData, "BurnRunState",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"BurnRunState",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "BurnState",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"BurnState",i_num);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "ModelRunState",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"IoTRunState",i_num);
            }

        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_aliiotreboot(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int index = 0;
    char url[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(lowerData,"IsReboot",1);

        index = get_p2p_index(header);
        snprintf(url, sizeof(url), "%s/iotreboot", p2p_name_list[index]);

        Ovfs_Web_UpdateHeader(header, REST_PUT, url);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_aliiotrunbinging(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int index = 0;
    char url[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(lowerData,"IsUnbinding",1);

        index = get_p2p_index(header);
        snprintf(url, sizeof(url), "%s/unbinding", p2p_name_list[index]);
        LOGW("url:[%s]\n",url);
        Ovfs_Web_UpdateHeader(header, REST_PUT, url);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmAliIoTState(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_aliiotstate(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    return ret;
}

int frmAliIoTReboot(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            //获取参数
            ret = web_semantic_set_aliiotreboot(header, indata, outdata);
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

int frmAliIoTUnbinding(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            //获取参数
            ret = web_semantic_aliiotrunbinging(header, indata, outdata);
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
            web_semantic_set_aliiotreboot(header, indata, outdata);
        }
    }

    return ret;
}
int frmRtmpPushCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //?¨???·?￥??a€“?￥??a€??|a€￠??
            ret = web_semantic_get_rtmppushcfg(header, indata, outdata, opt);
            break;

        case 1:
            //?¨?????§?????￥??a€??|a€￠??
            ret = web_semantic_set_rtmppushcfg(header, indata, outdata, opt);
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

int frmRtspCfg(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header,3);

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //è?·??–?????°
            ret = web_semantic_get_rtspcfg(header, indata, outdata);
            break;

        case 1:
            //è??????????°
            ret = web_semantic_set_rtspcfg(header, indata, outdata);
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
static int web_semantic_get_pushsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
	int ret = 0;
    int i_num = 0;
    int iloop = 0;
    int nloop = 0;
    char path[128] = {0};
    char pathname[128] = {0};
    char *strType = NULL;
    char *str_tmp = NULL;
	cJSON_Struct *lowerData = NULL;
    cJSON_Struct *alarmtime = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_tmp2 = NULL;

    if(Common_Json_GetAttrValueStr(indata, "PushType", &strType) == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    if(0 == ret)
    {
        if(smatch(strType, "HttpPush"))
        {
            snprintf(path, sizeof(path),"/MediaServer/SmartProtocolNotDisturb");
        }
        else if(smatch(strType, "RtmpPush"))
        {
            snprintf(path, sizeof(path),"/Mediaserver/RtmpPush/NotDisturb");
        }
        else
            ret = WEB_CODE_InvalidArg;
    }

    if(0 == ret)
    {
    	Ovfs_Web_UpdateHeader(header, REST_GET, path);
    	ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

	if (0 == ret)
	{

		alarmtime = Common_Json_SetAttrValueArr(outdata, "NotDisturbTime");
		// 获取星期一到星期六
        for (iloop = 1; iloop < 7; iloop ++)
        {
            pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
            for (nloop=0; nloop<8; nloop++)
            {
                pArray_tmp2 = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);

                snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Start", iloop, nloop);
                Common_Json_GetAttrValueInt(lowerData, pathname, &i_num);

				Common_Json_SetAttrValueArrInt(pArray_tmp2, 0, i_num/100);
				Common_Json_SetAttrValueArrInt(pArray_tmp2, 1, i_num%100);

                snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Stop",iloop, nloop);
                Common_Json_GetAttrValueInt(lowerData, pathname, &i_num);

				Common_Json_SetAttrValueArrInt(pArray_tmp2, 2, i_num/100);
				Common_Json_SetAttrValueArrInt(pArray_tmp2, 3, i_num%100);

                Common_Json_AddItem(pArray_tmp, nloop, NULL, pArray_tmp2);
            }
            Common_Json_AddItem(alarmtime, iloop - 1, NULL, pArray_tmp);
        }
        // 获取星期日
        {
            pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
            for (nloop=0; nloop<8; nloop++)
            {
                pArray_tmp2 = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);

                snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Start", 0, nloop);
                Common_Json_GetAttrValueInt(lowerData, pathname, &i_num);

				Common_Json_SetAttrValueArrInt(pArray_tmp2, 0, i_num/100);
				Common_Json_SetAttrValueArrInt(pArray_tmp2, 1, i_num%100);

                snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d.Sched%d.Stop",0, nloop);
                Common_Json_GetAttrValueInt(lowerData, pathname, &i_num);

				Common_Json_SetAttrValueArrInt(pArray_tmp2, 2, i_num/100);
				Common_Json_SetAttrValueArrInt(pArray_tmp2, 3, i_num%100);

                Common_Json_AddItem(pArray_tmp, nloop, NULL, pArray_tmp2);
            }
            Common_Json_AddItem(alarmtime, 6, NULL, pArray_tmp);
        }

        cJSON_Struct* NotDisturbCfg = Common_Json_SetAttrValueObj(outdata, "NotDisturbCfg");

        if(smatch(strType, "HttpPush"))
        {
            i_num = 0;
            Common_Json_GetAttrValueInt(lowerData, "EnableAlarm", &i_num);
            Common_Json_SetAttrValueInt(NotDisturbCfg, "EnableAlarm", i_num);
            i_num = 0;
            Common_Json_GetAttrValueInt(lowerData, "EnableSmartResult", &i_num);
            Common_Json_SetAttrValueInt(NotDisturbCfg, "EnableSmartResult", i_num);
            i_num = 0;
            Common_Json_GetAttrValueInt(lowerData, "EnableBkgPic", &i_num);
            Common_Json_SetAttrValueInt(NotDisturbCfg, "EnableBkgPic", i_num);
            i_num = 0;
            Common_Json_GetAttrValueInt(lowerData, "EnableConfig", &i_num);
            Common_Json_SetAttrValueInt(NotDisturbCfg, "EnableConfig", i_num);

            cJSON_Struct* AlarmList_set = Common_Json_SetAttrValueArr(NotDisturbCfg, "AlarmList");
            cJSON_Struct* AlarmList_get = Common_Json_GetAttrValueArr(lowerData, "AlarmList");
            int Size = Common_Json_ArraySize(AlarmList_get);
            for(iloop=0; iloop<Size; iloop++)
            {
                if(Common_Json_GetAttrValue(AlarmList_get, iloop, NULL, NULL, &str_tmp, NULL, NULL))
                {
                    Common_Json_SetAttrValueArrStr(AlarmList_set, iloop, str_tmp);
                }
            }
        }
        else if(smatch(strType, "RtmpPush"))
        {
            cJSON_Struct* ClientList_set = Common_Json_SetAttrValueArr(NotDisturbCfg, "ClientList");

            cJSON_Struct* ClientList_get = Common_Json_GetAttrValueArr(lowerData, "ClientList");
            int Size = Common_Json_ArraySize(ClientList_get);
            for(iloop=0; iloop<Size; iloop++)
            {
                if(Common_Json_GetAttrValue(ClientList_get, iloop, NULL, NULL, &str_tmp, NULL, NULL))
                {
                    Common_Json_SetAttrValueArrStr(ClientList_set, iloop, str_tmp);
                }
            }
        }

	}

	if (lowerData)
	{
		Common_Json_Delete(lowerData);
		lowerData = NULL;
	}

	return ret;
}

static int web_semantic_set_pushsetting(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
	int ret = 0;
	int i_num = 0;
    int i_num2 = 0;
    int iloop = 0;
    int nloop = 0;
    char path[128] = {0};
    char pathname[128] = {0};
    char *strType = NULL;
    char *str_tmp = NULL;
    char pathtmp[128] = {0};
	cJSON_Struct *lowerData = NULL;
    cJSON_Struct *alarmtime = NULL;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_tmp2 = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if(Common_Json_GetAttrValueStr(indata, "PushType", &strType) == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

	if (0 == ret)
	{
        if(smatch(strType, "HttpPush"))
        {
            snprintf(path, sizeof(path),"/MediaServer/SmartProtocolNotDisturb");

            if(Common_Json_GetAttrValueInt(indata, "NotDisturbCfg/EnableAlarm", &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "EnableAlarm", i_num);
            }

            if(Common_Json_GetAttrValueInt(indata, "NotDisturbCfg/EnableSmartResult", &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "EnableSmartResult", i_num);
            }

            if(Common_Json_GetAttrValueInt(indata, "NotDisturbCfg/EnableBkgPic", &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "EnableBkgPic", i_num);
            }

            if(Common_Json_GetAttrValueInt(indata, "NotDisturbCfg/EnableConfig", &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "EnableConfig", i_num);
            }

            cJSON_Struct* AlarmList_get = Common_Json_GetAttrValueArr(indata, "NotDisturbCfg/AlarmList");
            if(AlarmList_get)
            {
                cJSON_Struct* AlarmList_set = Common_Json_SetAttrValueArr(lowerData, "AlarmList");
                i_num2 = Common_Json_ArraySize(AlarmList_get);
                for(iloop = 0; iloop < i_num2; iloop++)
                {
                    if(Common_Json_GetAttrValue(AlarmList_get, iloop, NULL, NULL, &str_tmp, NULL, NULL))
                    {
                        LOGD("[%d][%s]\n",iloop,str_tmp);
                        Common_Json_SetAttrValueArrStr(AlarmList_set, iloop, str_tmp);
                    }
                }
            }
        }
        else if(smatch(strType, "RtmpPush"))
        {
            snprintf(path, sizeof(path),"/Mediaserver/RtmpPush/NotDisturb");

            cJSON_Struct* ClientList_get = Common_Json_GetAttrValueArr(indata, "NotDisturbCfg/ClientList");
            if(ClientList_get)
            {
                cJSON_Struct* ClientList_set = Common_Json_SetAttrValueArr(lowerData, "ClientList");
                i_num2 = Common_Json_ArraySize(ClientList_get);
                for(iloop = 0; iloop < i_num2; iloop++)
                {
                    if(Common_Json_GetAttrValue(ClientList_get, iloop, NULL, NULL, &str_tmp, NULL, NULL))
                    {
                        LOGD("[%d][%s]\n",iloop,str_tmp);
                        Common_Json_SetAttrValueArrStr(ClientList_set, iloop, str_tmp);
                    }
                }
            }

        }
        else
            ret = WEB_CODE_InvalidArg;
	}

    if (0 == ret)
	{
        Common_Json_SetAttrValueObj(lowerData, "NotDisturbTime");

        alarmtime = Common_Json_GetAttrValueArr(indata, "NotDisturbTime");
        // 设置星期日
        snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d", 0);
        Common_Json_SetAttrValueObj(lowerData, pathname);

        pArray_tmp = Common_Json_GetAttrValue(alarmtime, 6, NULL, NULL, NULL, NULL, NULL);
        for (nloop=0; nloop<8; nloop++)
        {
            snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d/Sched%d", 0, nloop);
            Common_Json_SetAttrValueObj(lowerData, pathname);
            pArray_tmp2 = Common_Json_GetAttrValue(pArray_tmp, nloop, NULL, NULL, NULL, NULL, NULL);

            Common_Json_GetAttrValue(pArray_tmp2, 0, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_GetAttrValue(pArray_tmp2, 1, NULL, NULL, NULL, &i_num2, NULL);

            snprintf(pathtmp, sizeof(pathtmp), "%s.Start", pathname);
            Common_Json_SetAttrValueInt(lowerData, pathtmp, i_num*100+i_num2);

			Common_Json_GetAttrValue(pArray_tmp2, 2, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_GetAttrValue(pArray_tmp2, 3, NULL, NULL, NULL, &i_num2, NULL);

            snprintf(pathtmp, sizeof(pathtmp), "%s.Stop", pathname);
            Common_Json_SetAttrValueInt(lowerData, pathtmp, i_num*100+i_num2);
		}

        // 设置星期一到星期六
        for (iloop = 0; iloop < 6; iloop ++)
        {
            snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d", iloop + 1);
            Common_Json_SetAttrValueObj(lowerData, pathname);

            pArray_tmp = Common_Json_GetAttrValue(alarmtime, iloop, NULL, NULL, NULL, NULL, NULL);
            for (nloop=0; nloop<8; nloop++)
            {

                snprintf(pathname, sizeof(pathname), "NotDisturbTime/Weekday%d/Sched%d", iloop + 1, nloop);
                Common_Json_SetAttrValueObj(lowerData, pathname);

                pArray_tmp2 = Common_Json_GetAttrValue(pArray_tmp, nloop, NULL, NULL, NULL, NULL, NULL);

                Common_Json_GetAttrValue(pArray_tmp2, 0, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp2, 1, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathtmp, sizeof(pathtmp), "%s.Start", pathname);
                Common_Json_SetAttrValueInt(lowerData, pathtmp, i_num*100+i_num2);

				Common_Json_GetAttrValue(pArray_tmp2, 2, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_GetAttrValue(pArray_tmp2, 3, NULL, NULL, NULL, &i_num2, NULL);

                snprintf(pathtmp, sizeof(pathtmp), "%s.Stop", pathname);
                Common_Json_SetAttrValueInt(lowerData, pathtmp, i_num*100+i_num2);
			}
        }
	}

	if (0 == ret)
	{
		Ovfs_Web_UpdateHeader(header, REST_PUT, path);
		ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
	}

	if(lowerData)
    {
		Common_Json_Delete(lowerData);
		lowerData = NULL;
	}

	return ret;
}

int frmPushSetting(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
 		switch (opt->type)
		{
		case 0:
            ret = web_semantic_get_pushsetting(header, indata, outdata);
			break;
        case 1:
            ret = web_semantic_set_pushsetting(header, indata, outdata);
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

static int web_semantic_get_defaultroute(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iType = 0;
    char *str = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/DefaultRoute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if(Common_Json_GetAttrValueStr(lowerData, "DefaultRoute",&str))
            {
                Common_Json_SetAttrValueStr(outdata,"DefaultRoute",str);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "NetType",&iType)== NULL)
            {
                if(Common_StrnCmp(str, "eth", 3) == 0)
                {
                    iType= 1;
                }
                else if(Common_StrnCmp(str, "wlan", 4) == 0)
                {
                    iType= 2;
                }
                else if(Common_StrnCmp(str, "ppp", 3) == 0)
                {
                    if(access("/tmp/lte", F_OK) == 0)
                    {
                        iType= 3;
                    }
                    else
                    {
                        iType= 1;
                    }
                }
            }

            Common_Json_SetAttrValueInt(outdata, "NetType",iType);

        }

        if(lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }

    }

    return ret;
}

int frmGetDefaultRoute(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
 		switch (opt->type)
		{
		case 0:
            ret = web_semantic_get_defaultroute(header, indata, outdata);
			break;

		default:
            ret = WEB_CODE_InvalidArg;
			break;
		}
	}

    return ret;
}

static int web_semantic_get_ltecustomcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/Lte4GCustomCfg");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {

            if(Common_Json_GetAttrValueStr(lowerData, "Customer",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"Customer",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "CustomerID",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"CustomerID",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "BindCCID",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"BindCCID",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "BindNetInfo",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"BindNetInfo",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "UseCCID",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"UseCCID",str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "UseNetInfo",&str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata,"UseNetInfo",str_tmp);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "IsBind",&i_num))
            {
                Common_Json_SetAttrValueInt(outdata,"IsBind",i_num);
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_del_ltecustomcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, int type)
{
    int ret = 0;
    char *str_tmp = NULL;
    char str_url[64] = {0};
    cJSON_Struct *lowerData = NULL;

    snprintf(str_url, sizeof(str_url),"/NetWork/NetAttr/Lte4G%sbind",type == 2 ? "Un" : "");

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {

        if(Common_Json_GetAttrValueStr(indata, "Customer",&str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData,"Customer",str_tmp);
        }

        if(Common_Json_GetAttrValueStr(indata, "CustomerID",&str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData,"CustomerID",str_tmp);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, str_url);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_notify_ltecustomcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/NetWork/NetAttr/Lte4GSpecified");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;
}

static int web_semantic_set_ltecustomcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (Common_Json_GetAttrValueInt(indata, "Status", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "Status", i_num);
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/NetWork/NetAttr/LteSpecifiedCard");
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

int frmLteCustomCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_ltecustomcfg(header, indata, outdata);
            break;
        case 1:
        case 2:
            ret = web_semantic_del_ltecustomcfg(header, indata, outdata,opt->type);
            break;
        case 3:
            ret = web_semantic_notify_ltecustomcfg(header, indata, outdata);
            break;
        case 4:
            ret = web_semantic_set_ltecustomcfg(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_get_thirdpartyprotocols(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Common_Json_SetAttrValueObj(outdata, "TST");
    Common_Json_SetAttrValueInt(outdata, "TST/Supported", 0);
    Common_Json_SetAttrValueInt(outdata, "TST/Enable", 0/*g_ovfs_web->enable_TST*/);

    Common_Json_SetAttrValueObj(outdata, "HK");
    Common_Json_SetAttrValueInt(outdata, "HK/Supported", g_ovfs_web->support_HK);
    Common_Json_SetAttrValueInt(outdata, "HK/Enable", g_ovfs_web->enable_HK);

    return ret;
}

int web_semantic_set_thirdpartyprotocols(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int tst_enable = -1;
    int hk_enable = -1;
    int need_save = 0;

    /*if(Common_Json_GetAttrValueInt(indata, "TST/Enable", &tst_enable))
    {
        if(tst_enable != g_ovfs_web->enable_TST)
        {
            need_save = 1;
            g_ovfs_web->enable_TST = tst_enable;
            Common_Json_SetAttrValueInt(g_ovfs_config, "ThirdPartyProtocols/TST", tst_enable);

            ovfs_tst_init(tst_enable);
        }
    }*/

    if(g_ovfs_web->support_HK && Common_Json_GetAttrValueInt(indata, "HK/Enable", &hk_enable))
    {
        if(hk_enable != g_ovfs_web->enable_HK)
        {
            need_save = 1;
            g_ovfs_web->enable_HK = hk_enable;
            Common_Json_SetAttrValueInt(g_ovfs_config, "ThirdPartyProtocols/HK", hk_enable);

            ovfs_hk_init(hk_enable);
        }
    }

    LOGD("tst:[%d] hk:[%d]\n",tst_enable,hk_enable);
    if(tst_enable == -1 && hk_enable == -1)return 0;

    if(need_save)
    {
        Access_SaveConfig(g_AccessHandle, g_ovfs_config);
    }

    return ret;
}


int frmThirdPartyProtocols(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_thirdpartyprotocols(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_thirdpartyprotocols(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_start_audiobroadcast(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int port = 0;
    char *ip = NULL;
    char *sn = NULL;

    if(Common_Json_GetAttrValueStr(indata, "MulticastIp", &ip) &&
        Common_Json_GetAttrValueInt(indata, "MulticastPort", &port) &&
        Common_Json_GetAttrValueStr(indata, "SerialNo", &sn))
    {
        if(g_ovfs_web->broadcast.status == 1)
        {
            ret = WEB_CODE_TaskExist;
        }
        else
        {
            Common_Strncpy(g_ovfs_web->broadcast.ip, ip, sizeof(g_ovfs_web->broadcast.ip));
            g_ovfs_web->broadcast.port = port;
            Common_Strncpy(g_ovfs_web->broadcast.sn, sn, sizeof(g_ovfs_web->broadcast.sn));

            g_ovfs_web->broadcast.status = 1;
            start_audio_broadcast();
        }
    }
    else
    {
        ret = WEB_CODE_InvalidArg;
    }

    return ret;
}

static int web_semantic_stop_audiobroadcast(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int port = 0;
    char *ip = NULL;
    char *sn = NULL;

    if(Common_Json_GetAttrValueStr(indata, "SerialNo", &sn))
    {
        if(g_ovfs_web->broadcast.status == 1)
        {
            if(smatch(g_ovfs_web->broadcast.sn, sn))
            {
                g_ovfs_web->broadcast.status = 0;
            }
            else
            {
                ret = WEB_CODE_InvalidArg;
            }
        }
    }
    else
    {
        ret = WEB_CODE_InvalidArg;
    }

    return ret;
}


int frmAudioBroadcast(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_start_audiobroadcast(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_stop_audiobroadcast(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int web_semantic_get_networkstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iRet = 0;
    int i_num = 0;
    int nettype = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    char defaultRoute[8] = {0};
    char netUrl[32] = {0};
    char dnsUrl[32] = {0};

    ret = web_semantic_get_defaultroute(header, indata, outdata);
    if (0 == ret)
    {
        Common_Json_GetAttrValueInt(outdata, "NetType",&nettype);
    }

    if(nettype == 1)//eth
    {
        snprintf(netUrl,sizeof(netUrl),"/NetWork/NetAttr/eth/0");
        snprintf(dnsUrl,sizeof(dnsUrl),"/NetWork/NetAttr/DNS");
    }
    else if(nettype == 2)//wifi
    {
        snprintf(netUrl,sizeof(netUrl),"/NetWork/NetAttr/Wlan/0");
        snprintf(dnsUrl,sizeof(dnsUrl),"/NetWork/NetAttr/WlanDns");

    }
    else if(nettype == 3)//4g
    {
        snprintf(netUrl,sizeof(netUrl),"/NetWork/NetAttr/LteCfg");
        snprintf(dnsUrl,sizeof(dnsUrl),"/NetWork/NetAttr/Lte4GCustomCfg");
    }
    else
    {
        LOGW("str_tmp:[%s]\n",defaultRoute);
        //ret = WEB_CODE_InternalMistake;
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, netUrl);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if(nettype == 3)
            {
                if(Common_Json_GetAttrValueStr(lowerData, "Info/IpAddrV4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Ip", str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Info/Dns1V4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Dns1", str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Info/Dns2V4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Dns2", str_tmp);
                }

                if(Common_Json_GetAttrValueInt(lowerData, "Info/SimId", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "SimId", i_num+1);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Info/IMEI",&str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata,"IMEI",str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Info/CCID",&str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata,"CCID",str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Info/ServiceInfo",&str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata,"ServiceInfo",str_tmp);
                }

                if(Common_Json_GetAttrValueInt(lowerData, "Info/SignalStrength",&i_num))
                {
                    Common_Json_SetAttrValueInt(outdata,"SignalStrength",i_num);
                }

                if(Common_Json_GetAttrValueInt(lowerData, "Info/LinkStatus",&i_num))
                {
                    Common_Json_SetAttrValueInt(outdata,"Status",i_num);
                }

                if(Common_Json_GetAttrValueInt(lowerData, "Info/SimSwitchStatus", &i_num) && i_num == 1)
                {
                    Common_Json_SetAttrValueInt(outdata, "Status", 3);
                }

				if(Common_Json_GetAttrValueInt(lowerData, "Info/StateOfCharge",&i_num))
                {
                    Common_Json_SetAttrValueInt(outdata,"ChargeState",i_num);
                }
            }
            else
            {
                if(Common_Json_GetAttrValueInt(lowerData, "EnableDhcp", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "Dhcp", i_num);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "IpAddrV4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Ip", str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "IpMaskV4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "NetMask", str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "GatewayV4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Gateway", str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "MacAddr", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "MacAddr", str_tmp);
                }
            }

        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    if(ret == 0 && slen(dnsUrl) > 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, dnsUrl);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if(nettype == 3)
            {
                if(Common_Json_GetAttrValueStr(lowerData, "Customer",&str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata,"Customer",str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "CustomerID",&str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata,"CustomerID",str_tmp);
                }
            }
            else
            {
                if(Common_Json_GetAttrValueInt(lowerData, "AutoDNS", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "AutoDns", i_num);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Dns1V4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Dns1", str_tmp);
                }

                if(Common_Json_GetAttrValueStr(lowerData, "Dns2V4", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "Dns2", str_tmp);
                }
            }
        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(outdata, "HttpPort", g_ovfs_web->httpport);
        Common_Json_SetAttrValueInt(outdata, "HttpsPort", g_ovfs_web->httpsport);
    }

    if (0 == ret && nettype == 2)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/WifiConfig");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if(Common_Json_GetAttrValueStr(lowerData, "Ssid", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "SSID", str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "Psk", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "Password", str_tmp);
            }
        }
        else
        {
            Common_Json_SetAttrValueStr(outdata, "SSID", "");
            Common_Json_SetAttrValueStr(outdata, "Password", "");
            ret = 0;
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iRet)
        {
            //RtspPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RtspPort", Common_Json_Type_Number, NULL, i_num, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //ovfs_print_json(outdata);

    return ret;
}


int frmNetworkStatus(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
 		switch (opt->type)
		{
		case 0:
            ret = web_semantic_get_networkstatus(header, indata, outdata);
			break;

		default:
            ret = WEB_CODE_InvalidArg;
			break;
		}
	}

    return ret;
}

static int web_semantic_get_wlanconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int i = 0;
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/WifiConfig");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if(Common_Json_GetAttrValueStr(lowerData, "Ssid", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "SSID", str_tmp);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "Psk", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "Password", str_tmp);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/WifiScanSsidResult");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            cJSON_Struct *list_set = Common_Json_SetAttrValueArr(outdata, "APList");
            cJSON_Struct *list_get = Common_Json_GetAttrValueArr(lowerData, "ResList");
            int size = Common_Json_ArraySize(list_get);
            for(i=0; i<size; i++)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(list_set, i);
                if(Common_Json_GetAttrValue(list_get, i, "SSID", NULL, &str_tmp, NULL, NULL))
                {
                    Common_Json_SetAttrValueStr(tmp, "SSID", str_tmp);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Mac", NULL, &str_tmp, NULL, NULL))
                {
                    Common_Json_SetAttrValueStr(tmp, "Mac", str_tmp);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Flags", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "Auth", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Signal", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "Signal", i_num);
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_wlanconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if(Common_Json_GetAttrValueStr(indata, "SSID", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "Ssid", str_tmp);
    }

    if(Common_Json_GetAttrValueStr(indata, "Password", &str_tmp))
    {
        Common_Json_SetAttrValueStr(lowerData, "Psk", str_tmp);
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/NetWork/NetAttr/WifiConfig");
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmWLANConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
		switch (opt->type)
		{
		case 0:
			//获取参数
            ret = web_semantic_get_wlanconfig(header, indata, outdata);
			break;

		case 1:
			//设置参数
            ret = web_semantic_set_wlanconfig(header, indata, outdata);
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

