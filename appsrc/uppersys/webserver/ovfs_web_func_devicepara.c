#include "ovfs_web_func.h"

int BaudRate[15] = {50, 75, 110, 150, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 76800, 115200};

static int web_semantic_get_dstpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

//月周模式
//{"Enable":0,"Offset":7200,"Type":1,"StartDay":{"MonthWeekDay":"010100","Time":"020000",},"StopDay":{}}
// 普通日模式(不考虑闰日),YearDay范围1-365
//{"Enable":0,"Offset":7200,"Type":2,"StartDay":{"YearDay":121,"Time":"020000"},"StopDay":{}}
// 含闰日模式,YearDay范围0-365
//{"Enable":0,"Offset":7200,"Type":3,"StartDay":{"YearDay":122,"Time":"020000"},"StopDay":{}}

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Time/Dst");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            int valueInt;
            if (Common_Json_GetAttrValueInt(lowerData, "Enable", &valueInt))
            {
                Common_Json_SetAttrValueInt(outdata, "Enable", valueInt);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Bias", &valueInt))
            {
                Common_Json_SetAttrValueInt(outdata, "Offset", valueInt*60);
            }

            Common_Json_SetAttrValueInt(outdata, "Type", 1);

            cJSON_Struct *inStartTime;
            cJSON_Struct *outStartDay;
            if ((inStartTime = Common_Json_GetAttrValueObj(lowerData, "StartTime")) != NULL &&
                    (outStartDay = Common_Json_SetAttrValueObj(outdata, "StartDay")) != NULL)
            {
                int month;
                int weekIdx;
                int weekDay;
                int hour;
                int min;
                if (Common_Json_GetAttrValueInt(inStartTime, "Month", &month) &&
                        Common_Json_GetAttrValueInt(inStartTime, "WeekIdx", &weekIdx) &&
                        Common_Json_GetAttrValueInt(inStartTime, "WeekDay", &weekDay) &&
                        Common_Json_GetAttrValueInt(inStartTime, "Hour", &hour) &&
                        Common_Json_GetAttrValueInt(inStartTime, "Min", &min))
                {
                    char valueStr[8];
                    snprintf(valueStr, sizeof(valueStr), "%02d%02d%02d", month, weekIdx, weekDay);
                    Common_Json_SetAttrValueStr(outStartDay, "MonthWeekDay", valueStr);

                    snprintf(valueStr, sizeof(valueStr), "%02d%02d00", hour, min);
                    Common_Json_SetAttrValueStr(outStartDay, "Time", valueStr);
                }
            }

            if ((inStartTime = Common_Json_GetAttrValueObj(lowerData, "StopTime")) != NULL &&
                    (outStartDay = Common_Json_SetAttrValueObj(outdata, "StopDay")) != NULL)
            {
                int month;
                int weekIdx;
                int weekDay;
                int hour;
                int min;
                if (Common_Json_GetAttrValueInt(inStartTime, "Month", &month) &&
                        Common_Json_GetAttrValueInt(inStartTime, "WeekIdx", &weekIdx) &&
                        Common_Json_GetAttrValueInt(inStartTime, "WeekDay", &weekDay) &&
                        Common_Json_GetAttrValueInt(inStartTime, "Hour", &hour) &&
                        Common_Json_GetAttrValueInt(inStartTime, "Min", &min))
                {
                    char valueStr[8];
                    snprintf(valueStr, sizeof(valueStr), "%02d%02d%02d", month, weekIdx, weekDay);
                    Common_Json_SetAttrValueStr(outStartDay, "MonthWeekDay", valueStr);

                    snprintf(valueStr, sizeof(valueStr), "%02d%02d00", hour, min);
                    Common_Json_SetAttrValueStr(outStartDay, "Time", valueStr);
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_dstpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int autoreboot = 0;
//月周模式
//{"Enable":0,"Offset":7200,"Type":1,"StartDay":{"MonthWeekDay":"010100","Time":"020000",},"StopDay":{}}
// 普通日模式(不考虑闰日),YearDay范围1-365
//{"Enable":0,"Offset":7200,"Type":2,"StartDay":{"YearDay":121,"Time":"020000"},"StopDay":{}}
// 含闰日模式,YearDay范围0-365
//{"Enable":0,"Offset":7200,"Type":3,"StartDay":{"YearDay":122,"Time":"020000"},"StopDay":{}}

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
        int valueInt;
        if (Common_Json_GetAttrValueInt(indata, "Enable", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", valueInt);
        }

        if (Common_Json_GetAttrValueInt(indata, "Offset", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Bias", valueInt/60);
        }

        if (Common_Json_GetAttrValueInt(indata, "AutoReboot", &autoreboot) == NULL)
        {
            autoreboot = 1;
        }
        Common_Json_SetAttrValueInt(lowerData, "AutoReboot", autoreboot);
        Common_Json_SetAttrValueInt(lowerData, "Mode", 0);

        cJSON_Struct *inStartDay;
        cJSON_Struct *outStartTime;
        if ((inStartDay = Common_Json_GetAttrValueObj(indata, "StartDay")) != NULL &&
                (outStartTime = Common_Json_SetAttrValueObj(lowerData, "StartTime")) != NULL)
        {
            char *valueStr;
            if (Common_Json_GetAttrValueStr(inStartDay, "MonthWeekDay", &valueStr))
            {
                int month;
                int weekIdx;
                int weekDay;
                if (3 == sscanf(valueStr, "%02d%02d%02d", &month, &weekIdx, &weekDay))
                {
                    Common_Json_SetAttrValueInt(outStartTime, "Month", month);
                    Common_Json_SetAttrValueInt(outStartTime, "WeekIdx", weekIdx);
                    Common_Json_SetAttrValueInt(outStartTime, "WeekDay", weekDay);
                }
            }
            if (Common_Json_GetAttrValueStr(inStartDay, "Time", &valueStr))
            {
                int hour;
                int min;
                int sec;
                if (3 == sscanf(valueStr, "%02d%02d%02d", &hour, &min, &sec))
                {
                    Common_Json_SetAttrValueInt(outStartTime, "Hour", hour);
                    Common_Json_SetAttrValueInt(outStartTime, "Min", min);
                }
            }
        }

        if ((inStartDay = Common_Json_GetAttrValueObj(indata, "StopDay")) != NULL &&
                (outStartTime = Common_Json_SetAttrValueObj(lowerData, "StopTime")) != NULL)
        {
            char *valueStr;
            if (Common_Json_GetAttrValueStr(inStartDay, "MonthWeekDay", &valueStr))
            {
                int month;
                int weekIdx;
                int weekDay;
                if (3 == sscanf(valueStr, "%02d%02d%02d", &month, &weekIdx, &weekDay))
                {
                    Common_Json_SetAttrValueInt(outStartTime, "Month", month);
                    Common_Json_SetAttrValueInt(outStartTime, "WeekIdx", weekIdx);
                    Common_Json_SetAttrValueInt(outStartTime, "WeekDay", weekDay);
                }
            }
            if (Common_Json_GetAttrValueStr(inStartDay, "Time", &valueStr))
            {
                int hour;
                int min;
                int sec;
                if (3 == sscanf(valueStr, "%02d%02d%02d", &hour, &min, &sec))
                {
                    Common_Json_SetAttrValueInt(outStartTime, "Hour", hour);
                    Common_Json_SetAttrValueInt(outStartTime, "Min", min);
                }
            }
        }
        cJSON_Struct* od = NULL;
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Time/Dst");
        ret = Ovfs_Web_RestMethodA(header, lowerData, &od, 0);
        if(od)
        {
            int val = 0;
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

static int web_semantic_get_gb28181para(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *lowerGb28181Cfg = NULL;
    cJSON_Struct *upperGb28181Cfg = outdata;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/AccessHost/Gb28181/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerGb28181Cfg, 0);
    }

    if (0 == ret)
    {
        char *valueStr = NULL;
        int valueInt = 0;

        Common_Json_SetAttrValueObj(upperGb28181Cfg, "GB28181CFG");

        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipCode", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_Code", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipZone", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_Zone", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipDeviceID", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_DeviceID", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipPassword", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_Password", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipIP", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_IP", valueStr);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "SipPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.SIP_Port", valueInt);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipCode2", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_Code2", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipZone2", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_Zone2", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipPassword2", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_Password2", valueStr);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "SipIP2", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.SIP_IP2", valueStr);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "SipPort2", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.SIP_Port2", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "StreamType", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.StreamType", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "LocalPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.Local_Port", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "Expires", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.Expires", valueInt);
        }

        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "TalkOverTCP", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.TalkOverTCP", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "CmdTranType", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.CmdTranType", valueInt);
        }

        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "KeepAlive", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.Keepalive", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "KeepAliveMaxCount", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.KeepaliveCnt", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "OnlineStatus", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.OnlineStatus", valueInt==2 ? 1 : 0);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "MulticastEnable", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.MulticastEnable", valueInt);
        }
        if (Common_Json_GetAttrValueStr(lowerGb28181Cfg, "MulticastAddr", &valueStr))
        {
            Common_Json_SetAttrValueStr(upperGb28181Cfg, "GB28181CFG.MulticastAddr", valueStr);
        }
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "MulticastPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "GB28181CFG.MulticastPort", valueInt);
        }

        int chanNum = 0;
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "ChannelNum", &chanNum))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "ChanNum", chanNum);
        }

        int alarmNum = 0;
        if (Common_Json_GetAttrValueInt(lowerGb28181Cfg, "AlarmNum", &alarmNum))
        {
            Common_Json_SetAttrValueInt(upperGb28181Cfg, "AlarmNum", alarmNum);
        }

        cJSON_Struct *chanInfoLower = NULL;
        cJSON_Struct *chanInfoUpper = NULL;
        if (chanNum > 0)
        {
            chanInfoLower = Common_Json_GetAttrValueArr(lowerGb28181Cfg, "Channels");
            chanInfoUpper = Common_Json_SetAttrValueArr(upperGb28181Cfg, "ChanInfo");

            int i;
            for (i = 0; i < chanNum; i++)
            {
                if (chanInfoLower)
                {
                    if (Common_Json_GetAttrValue(chanInfoLower, i, "Index", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(chanInfoUpper, i, "Index", Common_Json_Type_Number, NULL, valueInt, 0);
                    }

                    if (Common_Json_GetAttrValue(chanInfoLower, i, "ChannelID", NULL, &valueStr, NULL, NULL))
                    {
                        Common_Json_SetAttrValue(chanInfoUpper, i, "ChanID", Common_Json_Type_String, valueStr, 0, 0);
                    }

                    if (Common_Json_GetAttrValue(chanInfoLower, i, "Level", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(chanInfoUpper, i, "Level", Common_Json_Type_Number, NULL, valueInt, 0);
                    }
                }
            }
        }

        cJSON_Struct *alarmInfoLower = NULL;
        cJSON_Struct *alarmInfoUpper = NULL;
        if (alarmNum > 0)
        {
            alarmInfoLower = Common_Json_GetAttrValueArr(lowerGb28181Cfg, "Alarms");
            alarmInfoUpper = Common_Json_SetAttrValueArr(upperGb28181Cfg, "AlarmInfo");

            int i;
            for (i = 0; i < alarmNum; i++)
            {
                if (Common_Json_GetAttrValue(alarmInfoLower, i, "Index", NULL, NULL, &valueInt, NULL))
                {
                    Common_Json_SetAttrValue(alarmInfoUpper, i, "Index", Common_Json_Type_Number, NULL, valueInt, 0);
                }

                if (Common_Json_GetAttrValue(alarmInfoLower, i, "AlarmID", NULL, &valueStr, NULL, NULL))
                {
                    Common_Json_SetAttrValue(alarmInfoUpper, i, "ChanID", Common_Json_Type_String, valueStr, 0, 0);
                }

                if (Common_Json_GetAttrValue(alarmInfoLower, i, "Level", NULL, NULL, &valueInt, NULL))
                {
                    Common_Json_SetAttrValue(alarmInfoUpper, i, "Level", Common_Json_Type_Number, NULL, valueInt, 0);
                }
            }
        }

    }

    Common_Json_Delete(lowerGb28181Cfg);
    lowerGb28181Cfg = NULL;

    return  ret;
}

static int web_semantic_set_gb28181para(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

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
        char *valueStr = NULL;
        int valueInt = 0;

        cJSON_Struct *UpperGb28181cfg = Common_Json_GetAttrValueObj(indata, "GB28181CFG");

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_Code", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipCode", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_Zone", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipZone", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_DeviceID", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipDeviceID", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_Password", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipPassword", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_IP", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipIP", valueStr);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "SIP_Port", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "SipPort", valueInt);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_Code2", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipCode2", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_Zone2", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipZone2", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_Password2", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipPassword2", valueStr);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "SIP_IP2", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "SipIP2", valueStr);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "SIP_Port2", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "SipPort2", valueInt);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "StreamType", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "StreamType", valueInt);
        }
        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "Local_Port", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "LocalPort", valueInt);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "Expires", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Expires", valueInt);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "Keepalive", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "KeepAlive", valueInt);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "TalkOverTCP", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "TalkOverTCP", valueInt);
        }
        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "CmdTranType", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "CmdTranType", valueInt);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "KeepaliveCnt", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "KeepAliveMaxCount", valueInt);
        }

        if (Common_Json_GetAttrValueStr(UpperGb28181cfg, "MulticastAddr", &valueStr))
        {
            Common_Json_SetAttrValueStr(lowerData, "MulticastAddr", valueStr);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "MulticastEnable", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "MulticastEnable", valueInt);
        }

        if (Common_Json_GetAttrValueInt(UpperGb28181cfg, "MulticastPort", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "MulticastPort", valueInt);
        }

        int chanNum = 0;
        if (Common_Json_GetAttrValueInt(indata, "ChanNum", &chanNum))
        {
            Common_Json_SetAttrValueInt(lowerData, "ChannelNum", chanNum);
        }

        int alarmNum = 0;
        if (Common_Json_GetAttrValueInt(indata, "AlarmNum", &alarmNum))
        {
            Common_Json_SetAttrValueInt(lowerData, "AlarmNum", alarmNum);
        }

        cJSON_Struct *chanInfoUpper = NULL;
        cJSON_Struct *chanInfoLower = NULL;
        if (chanNum > 0)
        {
            chanInfoUpper = Common_Json_GetAttrValueArr(indata, "ChanInfo");
            chanInfoLower = Common_Json_SetAttrValueArr(lowerData, "Channels");

            int i;
            for (i = 0; i < chanNum; i++)
            {
                if (chanInfoLower)
                {
                    if (Common_Json_GetAttrValue(chanInfoUpper, i, "Index", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(chanInfoLower, i, "Index", Common_Json_Type_Number, NULL, valueInt, 0);
                    }

                    if (Common_Json_GetAttrValue(chanInfoUpper, i, "ChanID", NULL, &valueStr, NULL, NULL))
                    {
                        Common_Json_SetAttrValue(chanInfoLower, i, "ChannelID", Common_Json_Type_String, valueStr, 0, 0);
                    }

                    if (Common_Json_GetAttrValue(chanInfoUpper, i, "Level", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(chanInfoLower, i, "Level", Common_Json_Type_Number, NULL, valueInt, 0);
                    }
                }
            }
        }

        cJSON_Struct *alarmInfoUpper = NULL;
        cJSON_Struct *alarmInfoLower = NULL;
        if (alarmNum > 0)
        {
            alarmInfoUpper = Common_Json_GetAttrValueArr(indata, "AlarmInfo");
            alarmInfoLower = Common_Json_SetAttrValueArr(lowerData, "Alarms");

            int i;
            for (i = 0; i < alarmNum; i++)
            {
                if (alarmInfoLower)
                {
                    if (Common_Json_GetAttrValue(alarmInfoUpper, i, "Index", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(alarmInfoLower, i, "Index", Common_Json_Type_Number, NULL, valueInt, 0);
                    }

                    if (Common_Json_GetAttrValue(alarmInfoUpper, i, "ChanID", NULL, &valueStr, NULL, NULL))
                    {
                        Common_Json_SetAttrValue(alarmInfoLower, i, "AlarmID", Common_Json_Type_String, valueStr, 0, 0);
                    }

                    if (Common_Json_GetAttrValue(alarmInfoUpper, i, "Level", NULL, NULL, &valueInt, NULL))
                    {
                        Common_Json_SetAttrValue(alarmInfoLower, i, "Level", Common_Json_Type_Number, NULL, valueInt, 0);
                    }
                }
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/AccessHost/Gb28181/Attribute");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}

static int web_semantic_get_audiopara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Audio/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if (Common_Json_GetAttrValueInt(lowerData, "AudioSource", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "AudioSource", i_num);
            }
            if (Common_Json_GetAttrValueInt(lowerData, "InputVol", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "AudioInVol", i_num);
            }
            if (Common_Json_GetAttrValueInt(lowerData, "OutputVol", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "AudioOutVol", i_num);
            }
            if (Common_Json_GetAttrValueInt(lowerData, "EncFormat", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "AudioEncFormat", i_num);
            }
            if (Common_Json_GetAttrValueInt(lowerData, "AudioEnable", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "AudioEnable", i_num);
            }
            if (Common_Json_GetAttrValueInt(lowerData, "FrameLen", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "FrameLen", i_num);
            }

            /*  JsonOper_MergeObj(outdata, lowerData, 0); */

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}

//get audio para ability
static int web_semantic_get_audioparaability(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    //int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Audio/Ability");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            JsonOper_MergeObj(outdata, lowerData, 0);

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}

static int web_semantic_set_audiopara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int num_tmp;

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
        if (Common_Json_GetAttrValueInt(indata, "AudioSource", &num_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "AudioSource", num_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "AudioInVol", &num_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "InputVol", num_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "AudioOutVol", &num_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "OutputVol", num_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "AudioEncFormat", &num_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "EncFormat", num_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "AudioEnable", &num_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "AudioEnable", num_tmp);
        }
        if (Common_Json_GetAttrValueInt(indata, "FrameLen", &num_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "FrameLen", num_tmp);
        }


        /* JsonOper_MergeObj(lowerData, indata, 0);*/

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Audio/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_devicepara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *version = NULL;
    char *build_date = NULL;
    char* customerSN = NULL;
    char *str_tmp = NULL;
    char sw_version[128] = {0};
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        cJSON_Struct *webApiVersion = Common_Json_SetAttrValueObj(outdata, "WebApiVersion");
        Common_Json_SetAttrValueStr(webApiVersion, "Standard", OVFS_WEB_API_VERSION);
        Common_Json_SetAttrValueStr(webApiVersion, "Build", QueryBuildString());
    }

    Common_Json_SetAttrValueInt(outdata, "FaceDetectNum", 0);

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            version = NULL;
            Common_Json_GetAttrValueStr(lowerData, "Version", &version);
            build_date = NULL;
            Common_Json_GetAttrValueStr(lowerData, "BuildDate", &build_date);
            snprintf(sw_version, sizeof(sw_version), "%s.%s", version?version:"", build_date?build_date:"");
            Common_Json_SetAttrValueStr(outdata, "SoftwareVersion", sw_version);
            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "HardVersion", &str_tmp);
            Common_Json_SetAttrValueStr(outdata, "HardwareVersion", str_tmp?str_tmp:"");
            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "SerialNumber", &str_tmp);
            Common_Json_SetAttrValueStr(outdata, "SerialNumber", str_tmp?str_tmp:"");
            Common_Json_SetAttrValueInt(outdata, "DVRType", 3);
            customerSN = NULL;
            Common_Json_GetAttrValueStr(lowerData, "CustomerSN", &customerSN);
            Common_Json_SetAttrValueStr(outdata, "CustomerSN", customerSN?customerSN:"");

            //add cfgVersion CfgDate
            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "CfgVersion", &str_tmp);
            Common_Json_SetAttrValueStr(outdata, "CfgVersion", str_tmp?str_tmp:"");
            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "CfgDate", &str_tmp);
            Common_Json_SetAttrValueStr(outdata, "CfgDate", str_tmp?str_tmp:"");


        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/DeviceName");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "DeviceName", &str_tmp);
            Common_Json_SetAttrValueStr(outdata, "DVRName", str_tmp?str_tmp:"");
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
#if 0//ndef SIMPLIFIED

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/FishEye/Ability");
        int temp = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == temp)
        {
            cJSON_Struct *viewModeArrGet = Common_Json_GetAttrValueArr(lowerData, "ViewMode");
            int viewModeArrCount = Common_Json_ArraySize(viewModeArrGet);
            if (viewModeArrCount > 0)
            {
                Common_Json_SetAttrValueStr(outdata, "IsOfFishEye", "y");
            }
        }

        if(lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;

        }
    }
#endif
    if (0 == ret)
    {

        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Event/Ability");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValueInt(lowerData, "AlarmInNum", &i_num);
            //AINum
            Common_Json_SetAttrValueInt(outdata, "AlarmInPortNum", i_num);
        }

        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    Common_Json_SetAttrValueStr(outdata, "DeviceTypeString", g_ovfs_web->DeviceTypeString);

    /*cJSON_Struct* pp = Common_Json_SetAttrValueObj(outdata, "FunctionInfo");
    if(pp){
        //JsonOper_MergeObj(pp, Common_Json_Duplicate(g_ovfs_web->fnInfo, 1), 0);
        JsonOper_MergeObj(pp, g_ovfs_web->fnInfo, 0);
        Common_Json_SetAttrValueInt(pp, "EnableAlarming", 1);
    }*/

    if (0 == ret)
    {
        int ret_tmp = 0;
        cJSON_Struct *tmpdata = NULL;
        if ((tmpdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret_tmp = WEB_CODE_LackingMem;
        }

        if (0 == ret_tmp)
        {
            Common_Json_SetAttrValueInt(tmpdata, "Type", 47);
            Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/Cmd");
            ret_tmp = Ovfs_Web_RestMethodA(header, tmpdata, &lowerData, 0);
        }

        if (0 == ret_tmp)
        {
            if(Common_Json_GetAttrValueStr(lowerData, "version", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "MovementVersion", str_tmp);
            }
        }

        Common_Json_Delete(tmpdata);
        tmpdata = NULL;

        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}

static int web_semantic_set_devicepara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    int num_tmp;

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
        str_tmp = NULL;
        Common_Json_GetAttrValue(indata, -1, "DVRName", NULL, &str_tmp, NULL, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "DeviceName", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/DeviceName");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    /*
        if (0 == ret)
        {
            if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
        }

        if (0 == ret)
        {
            str_tmp = NULL;
            if (Common_Json_GetAttrValueInt(indata, "AudioSource", &num_tmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "AudioSource", num_tmp);
            }

            if (Common_Json_GetAttrValueInt(indata, "AudioInVol", &num_tmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "InputVol", num_tmp);
            }

            if (Common_Json_GetAttrValueInt(indata, "AudioOutVol", &num_tmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "OutputVol", num_tmp);
            }

            if (Common_Json_Size(lowerData) > 0)
            {
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Audio/Attribute/All");
                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            }
        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    */
    return ret;
}

static int web_semantic_get_videoformatpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        char buf[256]= {0};
        snprintf(buf,sizeof(buf),"/BoardSys/VideoInput/Attribute/Device%d",opt->dev);
        //printf("[%s]\n",buf);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            int VideoFormat = 0;
            Common_Json_GetAttrValue(lowerData, -1, "Format", NULL, NULL, &VideoFormat, 0);
            // 底层定义,0/2为N,1为P. slink定义1为N,2为P.
            if (VideoFormat == 0 || VideoFormat == 2)
            {
                VideoFormat = 1;
            }
            else if (VideoFormat == 1)
            {
                VideoFormat = 2;
            }
            Common_Json_SetAttrValue(outdata, -1, "VideoFormat", Common_Json_Type_Number, NULL, VideoFormat, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}

static int web_semantic_set_videoformatpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int VideoFormat = 0;

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
        Common_Json_GetAttrValue(indata,-1,"VideoFormat",NULL,NULL,&VideoFormat,0);
        // 底层定义,0/2为N,1为P. slink定义0/1为N,2为P.
        if (VideoFormat == 2)
        {
            VideoFormat = 1;
        }
        else if (VideoFormat == 1 || VideoFormat == 0)
        {
            VideoFormat = 0;
        }
        if (0 == VideoFormat)
        {
            //N制
            Common_Json_SetAttrValue(lowerData,-1,"Format",Common_Json_Type_Number,NULL,0,0);
            Common_Json_SetAttrValue(lowerData,-1,"Fps",Common_Json_Type_Number,NULL,30,0);
        }
        else
        {
            Common_Json_SetAttrValue(lowerData,-1,"Format",Common_Json_Type_Number,NULL,1,0);
            Common_Json_SetAttrValue(lowerData,-1,"Fps",Common_Json_Type_Number,NULL,25,0);
        }

        {
            char buf[256]= {0};
            snprintf(buf,sizeof(buf),"/BoardSys/VideoInput/Attribute/Device%d",opt->dev);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
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

static int web_semantic_get_videoformatpara_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        char buf[256]= {0};
        snprintf(buf,sizeof(buf),"/BoardSys/VideoInput/Attribute/Device%d",opt->dev);
        //	printf("[%s]\n",buf);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            int VideoFormat = 0;
            Common_Json_GetAttrValue(lowerData, -1, "Format", NULL, NULL, &VideoFormat, 0);
            Common_Json_SetAttrValue(outdata, -1, "VideoFormat", Common_Json_Type_Number, NULL, VideoFormat, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return  ret;
}
static int web_semantic_set_videoformatpara_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
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
        Common_Json_GetAttrValue(indata,-1,"VideoFormat",NULL,NULL,&i_num,0);
        Common_Json_SetAttrValue(lowerData,-1,"Format",Common_Json_Type_Number,NULL,i_num,0);
        Common_Json_GetAttrValue(indata,-1,"Fps",NULL,NULL,&i_num,0);
        Common_Json_SetAttrValue(lowerData,-1,"Fps",Common_Json_Type_Number,NULL,i_num,0);
        Common_Json_GetAttrValue(indata,-1,"W",NULL,NULL,&i_num,0);
        Common_Json_SetAttrValue(lowerData,-1,"Width",Common_Json_Type_Number,NULL,i_num,0);
        Common_Json_GetAttrValue(indata,-1,"H",NULL,NULL,&i_num,0);
        Common_Json_SetAttrValue(lowerData,-1,"Height",Common_Json_Type_Number,NULL,i_num,0);
        {
            char buf[256]= {0};
            snprintf(buf,sizeof(buf),"/BoardSys/VideoInput/Attribute/Device%d",opt->dev);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
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
static int web_semantic_get_devicetimectrl(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    WEB_TIME_T dataAndTime;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Time/SysTime");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Year", NULL, NULL, &dataAndTime.Year, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Month", NULL, NULL, &dataAndTime.Month, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Day", NULL, NULL, &dataAndTime.Day, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Hour", NULL, NULL, &dataAndTime.Hour, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Min", NULL, NULL, &dataAndTime.Minute, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Sec", NULL, NULL, &dataAndTime.Second, NULL);

            cJSON_Struct *pArry_root = NULL;
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "Time", Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pArry_root, 0, NULL, Common_Json_Type_Number, NULL, dataAndTime.Year, dataAndTime.Year);
            Common_Json_SetAttrValue(pArry_root, 1, NULL, Common_Json_Type_Number, NULL, dataAndTime.Month, dataAndTime.Month);
            Common_Json_SetAttrValue(pArry_root, 2, NULL, Common_Json_Type_Number, NULL, dataAndTime.Day, dataAndTime.Day);
            Common_Json_SetAttrValue(pArry_root, 3, NULL, Common_Json_Type_Number, NULL, dataAndTime.Hour, dataAndTime.Hour);
            Common_Json_SetAttrValue(pArry_root, 4, NULL, Common_Json_Type_Number, NULL, dataAndTime.Minute, dataAndTime.Minute);
            Common_Json_SetAttrValue(pArry_root, 5, NULL, Common_Json_Type_Number, NULL, dataAndTime.Second, dataAndTime.Second);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_set_devicetimectrl(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

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
        WEB_TIME_T dataAndTime;
        cJSON_Struct *pArry_tmp;
        pArry_tmp = Common_Json_GetAttrValue(indata, -1, "Time", NULL, NULL, NULL, NULL);
        Common_Json_GetAttrValue(pArry_tmp, 0, NULL, NULL, NULL, &dataAndTime.Year, NULL);
        Common_Json_GetAttrValue(pArry_tmp, 1, NULL, NULL, NULL, &dataAndTime.Month, NULL);
        Common_Json_GetAttrValue(pArry_tmp, 2, NULL, NULL, NULL, &dataAndTime.Day, NULL);
        Common_Json_GetAttrValue(pArry_tmp, 3, NULL, NULL, NULL, &dataAndTime.Hour, NULL);
        Common_Json_GetAttrValue(pArry_tmp, 4, NULL, NULL, NULL, &dataAndTime.Minute, NULL);
        Common_Json_GetAttrValue(pArry_tmp, 5, NULL, NULL, NULL, &dataAndTime.Second, NULL);

        Common_Json_SetAttrValue(lowerData, -1, "Year", Common_Json_Type_Number, NULL, dataAndTime.Year, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Month", Common_Json_Type_Number, NULL, dataAndTime.Month, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Day", Common_Json_Type_Number, NULL, dataAndTime.Day, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Hour", Common_Json_Type_Number, NULL, dataAndTime.Hour, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Min", Common_Json_Type_Number, NULL, dataAndTime.Minute, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Sec", Common_Json_Type_Number, NULL, dataAndTime.Second, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Time/SysTime");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_factoryInfo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    cJSON_Struct *jsonCompose = outdata;
    if (0 == ret)
    {
        const char *label[][2] =
        {
            {"DeviceName", "DeviceName"},
            {"ProductName", "ProductName"},
            {"DeviceType", "DeviceType"},
            {"DeviceTypeString", "DeviceTypeString"},
            {"DeviceModel", "DeviceModel"},
            {"Country", "Country"},
            {"City", "City"},
            {"Web", "Web"},
            {"Tel", "Tel"},
            {"Copyright", "Copyright"},
            {"Manufacturer", "Manufacturer"},
            {"Brand", "Brand"},
            {"Customer", "Customer"},
            {"SensorModel", "SensorModel"},

            {"HardVersion", "HwVersion"},
            {"Version", "SwVersion"},
            {"BuildDate", "BuildDate"},
            {"ProductDate", "ProductDate"},
            {"Vendor", "Vendor"},
            {"SerialNumber", "SerialNumber"},
            {"IsOfDome", "IsofDome"},
            {"Status", "Status"},
            {"CfgVersion","CfgVersion"},
            //{"Platform", "Platform"},
            //{"Project", "Project"},

            {"LensSupport", "LensSupport"},
            {"LensDrvType", "LensDrvType"},
            {"LensType", "LensType"},
            {"IrisSupport", "IrisSupport"},
            {"IrisType", "IrisType"},
        };

        int i;
        for (i = 0; i < sizeof(label)/sizeof(label[0]); i++)
        {
            char *valueStr = NULL;
            int valueInt = 0;
            if(strcasecmp("IsOfDome", label[i][0]) == 0 || strcasecmp("LensSupport", label[i][0]) == 0 || strcasecmp("IrisSupport", label[i][0]) == 0)
            {
                Common_Json_GetAttrValueInt(lowerData, label[i][0], &valueInt);
                if (valueInt == 0)
                {
                    Common_Json_SetAttrValueStr(jsonCompose, label[i][1], "n");
                }
                else if(valueInt == 1)
                {
                    Common_Json_SetAttrValueStr(jsonCompose, label[i][1], "y");
                }
                else if(valueInt == 2)
                {
                    Common_Json_SetAttrValueStr(jsonCompose, label[i][1], "y_ex");
                }
            }
            else
            {
                if (Common_Json_GetAttrValueStr(lowerData, label[i][0], &valueStr))
                {
                    Common_Json_SetAttrValueStr(jsonCompose, label[i][1], valueStr);
                }
            }
        }

        // {"LensSupport", "AutoLens/LensSupport"}
        // {"LensDrvType", "AutoLens/LensDrvType"}
        // {"LensSupport", "AutoLens/LensType"}
        // {"IrisSupport", "AutoIris/IrisSupport"}
        // {"IrisSupport", "AutoIris/IrisType"}
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(0 == ret)
    {
        /*
        {
        "DevTotalNum":  1,
        "ChanTotalNum": 1,
        "StreamNum":    3,
        "viDev0":{"viChanNum":1,"viChan0":3}
        }
        */
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Video/Ability/Number");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            int devCount = 0;
            cJSON_Struct *optStreamObjSet = Common_Json_SetAttrValueArr(outdata, "OptionalDevChanStreams");
            Common_Json_GetAttrValueInt(lowerData, "DevTotalNum", &devCount);
            PRINT_DBG("devCount=%d\n", devCount);
            int devIndex;
            for (devIndex = 0; devIndex < devCount; devIndex++)
            {
                cJSON_Struct *viDevChanStreamEach = Common_Json_SetAttrValue(optStreamObjSet, devIndex, NULL, Common_Json_Type_Array, NULL, 0, 0);

                char labelName[32];
                snprintf(labelName, sizeof(labelName), "viDev%d/viChanNum", devIndex);
                int chanCount = 0;
                Common_Json_GetAttrValueInt(lowerData, labelName, &chanCount);
                PRINT_DBG("chanCount[%d]=%d\n", devIndex, chanCount);

                int chanIndex;
                for (chanIndex = 0; chanIndex < chanCount; chanIndex++)
                {
                    snprintf(labelName, sizeof(labelName), "viDev%d/viChan%d", devIndex, chanIndex);
                    int streamCount;
                    Common_Json_GetAttrValueInt(lowerData, labelName, &streamCount);
                    PRINT_DBG("streamCount[%d][%d]=%d\n", devIndex, chanIndex, streamCount);
                    Common_Json_SetAttrValueArrInt(viDevChanStreamEach, chanIndex, streamCount);
                }
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

#ifndef SIMPLIFIED

    if (0 == ret)
    {
		/*Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/FishEye/Ability");
		int temp = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

        if (0 == temp)
        {
            cJSON_Struct *viewModeArrGet = Common_Json_GetAttrValueArr(lowerData, "ViewMode");
            int viewModeArrCount = Common_Json_ArraySize(viewModeArrGet);
            if (viewModeArrCount > 0)
            {
                Common_Json_SetAttrValueStr(jsonCompose, "IsOfFishEye", "y");
            }
            else
            {
                Common_Json_SetAttrValueStr(jsonCompose, "IsOfFishEye", "n");
            }
        }
        else*/
        {
            Common_Json_SetAttrValueStr(jsonCompose, "IsOfFishEye", "n");
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
#endif

    return ret;
}

int g_bSupportSnap = -1;
static int web_semantic_get_deviceAbility(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int i = 0;
    int size = 0;
    char *strTmp = NULL;
    cJSON_Struct *list = NULL;
    cJSON_Struct *lowerData = NULL;

    Common_Json_SetAttrValueInt(outdata, "AlarminNum", 0);//
    Common_Json_SetAttrValueInt(outdata, "AlarmoutNum", 0);//
    Common_Json_SetAttrValueInt(outdata, "Wifi", 0);//
    Common_Json_SetAttrValueInt(outdata, "Lte4g5g", 0);//
    Common_Json_SetAttrValueInt(outdata, "SDCard", 0);//

    /*Common_Json_SetAttrValueObj(outdata, "Uart");
    cJSON_Struct *modeCap = Common_Json_SetAttrValueArr(outdata, "Uart/ModeCapability");//
    Common_Json_SetAttrValueArrInt(modeCap, 0, 0);
    Common_Json_SetAttrValueArrInt(modeCap, 1, 1);
    Common_Json_SetAttrValueArrInt(modeCap, 2, 3);*/
    /*Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/uartosd");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    if(ret == 0)
    {
        //Common_Json_SetAttrValueInt(outdata, "UartExtend", 1);
    }*/

    Common_Json_SetAttrValueObj(outdata, "Protocols");
    Common_Json_SetAttrValueInt(outdata, "Protocols/Gb28181", 0);//
    Common_Json_SetAttrValueInt(outdata, "Protocols/Onvif", 0);//
    Common_Json_SetAttrValueInt(outdata, "Protocols/Smart1400", 0);//
    Common_Json_SetAttrValueInt(outdata, "Protocols/I8w", 0);//
    Common_Json_SetAttrValueInt(outdata, "Protocols/Wiegand", 0);
    Common_Json_SetAttrValueInt(outdata, "Protocols/I8s", 0);

    Common_Json_SetAttrValueObj(outdata, "Service");
    Common_Json_SetAttrValueInt(outdata, "Service/Rtsp", 0);//
    Common_Json_SetAttrValueInt(outdata, "Service/Rtmp", 0);//
    Common_Json_SetAttrValueInt(outdata, "Service/Ws", 0);//
    Common_Json_SetAttrValueInt(outdata, "Service/AliIot", 0);//
    Common_Json_SetAttrValueInt(outdata, "Service/AlarmArm", 1);
    Common_Json_SetAttrValueInt(outdata, "Service/LinkAudio", 1);//
    Common_Json_SetAttrValueInt(outdata, "Service/LinkLight", 1);//
    Common_Json_SetAttrValueInt(outdata, "Service/HttpCapPicV2", 0);//
    Common_Json_SetAttrValueInt(outdata, "Service/AlarmTime7x8", 0);
    Common_Json_SetAttrValueInt(outdata, "Service/AlarmTime7x1", 0);
    Common_Json_SetAttrValueInt(outdata, "Service/PushArm", 0);//
    Common_Json_SetAttrValueInt(outdata, "Service/OneKeyDrive", 1);

    Common_Json_SetAttrValueObj(outdata, "Ptz");
    Common_Json_SetAttrValueInt(outdata, "Ptz/PtzReverse", 0);
    Common_Json_SetAttrValueInt(outdata, "Ptz/Cruise", 0);//
    Common_Json_SetAttrValueInt(outdata, "Ptz/StepControl", 0);//

    if(g_ovfs_web->devInfo.bPTZ == 4 || g_ovfs_web->devInfo.bPTZ == 5)
    {
        Common_Json_SetAttrValueInt(outdata, "Ptz/PtzReverse", 1);

        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/cruisecontrol");
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        if(ret == 0)
        {
            Common_Json_SetAttrValueInt(outdata, "Ptz/Cruise", 1);
        }
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/ability");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "SingleStepSupport", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Ptz/StepControl", i_num);
        }
        ovfs_print_json(lowerData);
        cJSON_Struct *modeCap = Common_Json_GetAttrValueArr(lowerData, "ModeCapability");
        if(modeCap)
        {
            cJSON_Struct *pUart = Common_Json_SetAttrValueObj(outdata, "Uart");
            Common_Json_AddItem(pUart, -1, "ModeCapability", Common_Json_Duplicate(modeCap, 1));
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    Common_Json_SetAttrValueObj(outdata, "Event");
    Common_Json_SetAttrValueInt(outdata, "Event/MotionDetect", 2);//
    Common_Json_SetAttrValueInt(outdata, "Event/VideoHide", 2);//
    Common_Json_SetAttrValueInt(outdata, "Event/AlarmIn", 2);

	Common_Json_SetAttrValueInt(outdata, "Service/AudioBroadcast", 1);//

    Common_Json_SetAttrValueObj(outdata, "Service/AppSpecial");

    int app_special = 1;

    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotPtzPreset", 0);
    if(g_ovfs_web->devInfo.bPTZ == 4 || g_ovfs_web->devInfo.bPTZ == 5)
    {
        Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotPtzPreset", 1);
    }
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotRecord", app_special);
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotAlarmCfg", 0);
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotCustomAudio", 0);
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotOsdCfg", 0);
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotNetworkSetting", app_special);
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotLightCfg", 1);
    Common_Json_SetAttrValueInt(outdata, "Service/AppSpecial/IotHomePositionCfg", app_special);

    Common_Json_SetAttrValueObj(outdata, "Image");
    Common_Json_SetAttrValueInt(outdata, "Image/Minor", 1);
    Common_Json_SetAttrValueInt(outdata, "Image/Minor_90de", 0);//
    Common_Json_SetAttrValueInt(outdata, "Image/Minor_270de", 0);//
    Common_Json_SetAttrValueInt(outdata, "Image/IspConfig", 0);
    Common_Json_SetAttrValueInt(outdata, "Image/SwitchSensor", 0);

    Common_Json_SetAttrValueObj(outdata, "Upgrade");
    Common_Json_SetAttrValueInt(outdata, "Upgrade/Httpnew", 1);

    //char *str = Common_Json_Print(g_ovfs_uiconfig, NULL);
    //LOGD("g_ovfs_uiconfig:[%s]\n",g_ovfs_uiconfig);
    //free(str);

    /*if(Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "CVConfigVideoPara/MinorMode/90Rotation/visible", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "Image/Minor_90de", i_num);
    }

    if(Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "CVConfigVideoPara/MinorMode/270Rotation/visible", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "Image/Minor_270de", i_num);
    }*/

    /*if(Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "CVConfigMenu/CVConfigLightAlarm/visible", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "Service/LinkLight", i_num);
    }

    if(Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "CVConfigMenu/CVConfigAudioUpload/visible", &i_num))
    {
        LOGD("LinkAudio:[%d]\n",i_num);
        Common_Json_SetAttrValueInt(outdata, "Service/LinkAudio", i_num);
    }*/

    Ovfs_Web_UpdateHeader(header, REST_GET, "/boardsys/image/ability");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        i_num = 0;
        list = Common_Json_GetAttrValueArr(lowerData, "AbilityList");
        if(Common_Json_GetAttrValue(list, 0, "bSupportExposureLight", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValueInt(outdata, "Image/ExposureLight", i_num);
        }
        if(Common_Json_GetAttrValue(list, 0, "bSupportISPScene", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValueInt(outdata, "Image/IspConfig", i_num);
        }
        if(Common_Json_GetAttrValue(list, 0, "bSupportCorridor", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValueInt(outdata, "Image/Minor_90de", i_num);//
            Common_Json_SetAttrValueInt(outdata, "Image/Minor_270de", i_num);//
        }

        if(Common_Json_GetAttrValue(list, 0, "bSupportSwitch", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValueInt(outdata, "Image/SwitchSensor", i_num);
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    if(g_bSupportSnap == -1)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Snap/Device0/Channel0/Stream0");
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

        g_bSupportSnap = (ret==0 ? 1 : 0);
    }

    Common_Json_SetAttrValueInt(outdata, "Service/HttpCapPicV2", g_bSupportSnap);

    Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/AlarmOut/Ability");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        i_num = 0;
        Common_Json_GetAttrValueInt(lowerData, "AlaramOutNum", &i_num);
        Common_Json_SetAttrValueInt(outdata, "AlarmoutNum", i_num);
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Event/Ability");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        i_num = 0;
        Common_Json_GetAttrValueInt(lowerData, "AlarmInNum", &i_num);
        Common_Json_SetAttrValueInt(outdata, "AlarminNum", i_num);
        if(i_num == 0)
        {
            Common_Json_SetAttrValueInt(outdata, "Event/AlarmIn", 0);
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, "/");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        list = Common_Json_GetAttrValueArr(lowerData, "ResList");
        size = Common_Json_ArraySize(list);
        i_num = 0;
        for(i=0; i<size; i++)
        {
            Common_Json_GetAttrValue(list, i, "Uri", NULL, &strTmp, NULL, NULL);
            if(smatch(strTmp, "/AliIoT4ovfs"))
            {
                Common_Json_SetAttrValueInt(outdata, "Service/AliIot", 1);
            }
            else if(smatch(strTmp, "/Ovfs_websocket"))
            {
                Common_Json_SetAttrValueInt(outdata, "Service/Ws", 1);
            }
            else if(smatch(strTmp, "/Onvif"))
            {
                Common_Json_SetAttrValueInt(outdata, "Protocols/Onvif", 1);
            }
            else if(smatch(strTmp, "/AccessHost"))
            {
                if(Common_File_IsExist("/root/lib/libants_28181_sdk.so"))
                {
                    Common_Json_SetAttrValueInt(outdata, "Protocols/Gb28181", 1);
                }
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    if(Common_File_IsExist("/root/lib/libonvif.so"))
    {
        Common_Json_SetAttrValueInt(outdata, "Protocols/Onvif", 1);
    }
/*
    Ovfs_Web_UpdateHeader(header, REST_GET, "/Smartserver");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        list = Common_Json_GetAttrValueArr(lowerData, "List");
        size = Common_Json_ArraySize(list);
        for(i=0; i<size; i++)
        {
            Common_Json_GetAttrValue(list, i, "Uri", NULL, &strTmp, NULL, NULL);
            if(strstr(strTmp, "SmartProtocol"))
            {
                Common_Json_SetAttrValueInt(outdata, "Protocols/I8w", 1);
            }
            else if(strstr(strTmp, "Smart1400"))
            {
                Common_Json_SetAttrValueInt(outdata, "Protocols/Smart1400", 1);
            }
            else if(strstr(strTmp, "SmartProtocolNotDisturb"))
            {
                Common_Json_SetAttrValueInt(outdata, "Service/PushArm", 1);
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }
*/
    Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        list = Common_Json_GetAttrValueArr(lowerData, "List");
        size = Common_Json_ArraySize(list);
        for(i=0; i<size; i++)
        {
            Common_Json_GetAttrValue(list, i, "Uri", NULL, &strTmp, NULL, NULL);
            if(strstr(strTmp, "Rtsp"))
            {
                Common_Json_SetAttrValueInt(outdata, "Service/Rtsp", 1);
            }
            else if(strstr(strTmp, "Rtmp"))
            {
                Common_Json_SetAttrValueInt(outdata, "Service/Rtmp", 1);
            }
            else if(strstr(strTmp, "SmartProtocol"))
            {
                Common_Json_SetAttrValueInt(outdata, "Protocols/I8w", 1);
            }
            else if(strstr(strTmp, "SmartProtocolNotDisturb"))
            {
                Common_Json_SetAttrValueInt(outdata, "Service/PushArm", 1);
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    ret = web_semantic_get_wifisupported(header);
    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(outdata, "Wifi", 1);
    }

    ret = web_semantic_get_ltesupported(header);
    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(outdata, "Lte4g5g", 1);
    }

    if(Common_File_IsExist("/dev/mmcblk0p1"))
    {
        Common_Json_SetAttrValueInt(outdata, "SDCard", 1);
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Mediaserver/RtmpPush/NotDisturb");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(outdata, "Service/PushArm", 1);
    }

    return 0;
}
//#ifdef WITH_PTZ

static int web_semantic_get_decoderpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int iloop = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Address", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "DecoderAddress", Common_Json_Type_Number, NULL, i_num, i_num);
            Common_Json_GetAttrValue(lowerData, -1, "ProtocolIdx", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "DecoderType", Common_Json_Type_Number, NULL, i_num, i_num);
            Common_Json_GetAttrValue(lowerData, -1, "BaudRate", NULL, NULL, &i_num, NULL);
            for (iloop=0; iloop<sizeof(BaudRate)/sizeof(int); iloop++)
            {
                if (i_num == BaudRate[iloop])
                {
                    Common_Json_SetAttrValue(outdata,-1, "BaudRate", Common_Json_Type_Number, NULL, iloop, iloop);
                    break;
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_decoderpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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
        Common_Json_GetAttrValue(indata, -1, "DecoderAddress", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "Address", Common_Json_Type_Number, NULL, i_num, i_num);
        Common_Json_GetAttrValue(indata, -1, "DecoderType", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "ProtocolIdx", Common_Json_Type_Number, NULL, i_num, i_num);
        Common_Json_GetAttrValue(indata, -1, "BaudRate", NULL, NULL, &i_num, NULL);
        Common_Json_SetAttrValue(lowerData, -1, "BaudRate", Common_Json_Type_Number, NULL, BaudRate[i_num], BaudRate[i_num]);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_PTZProtocal(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/Ability");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    cJSON_Struct *pArry_root = NULL;
    if (0 == ret)
    {
        if ((pArry_root = Common_Json_SetAttrValue(outdata, -1, "PTZProtocal", Common_Json_Type_Array, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        int nloop = 0;
        int i_num = 0;
        char *str_tmp = NULL;
        int protocolSize = 0;
        cJSON_Struct *pArry_tmp = NULL;

        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "Protocol", NULL, NULL, NULL, NULL);
        if (pArry_tmp)
        {
            protocolSize = Common_Json_Size(pArry_tmp);
            for (nloop = 0; nloop < protocolSize; nloop ++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, nloop, "Name", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(pArry_root, nloop, "Describe", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
                Common_Json_GetAttrValue(pArry_tmp, nloop, "Idx", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValue(pArry_root, nloop, "Type", Common_Json_Type_Number, NULL, i_num, i_num);
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}
//#endif
static int web_semantic_get_QRCode(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int nCount = 0;
    int nIdx,nIdx1 = 0,nIdx2 = 0;
    // 底层未实现,暂时屏蔽.需要平台管理接口的支持.
#if 1
    char *str_proto = NULL, *str_uuid = NULL,*str_apple = NULL, *str_android = NULL;
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *pArry_root = NULL;
    cJSON_Struct *pArry_root1 = NULL;
    cJSON_Struct *pArry_root2 = NULL;
    cJSON_Struct *pArry_root3 = NULL;
    cJSON_Struct *lowerData = NULL;

    pArry_root = Common_Json_SetAttrValue(outdata, -1, "QRSelects", Common_Json_Type_Array, NULL, 0, 0);

    if (0 == ret)
    {

        Ovfs_Web_UpdateHeader(header,REST_GET,"/AccessHost/GetUuid");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            pArry_root2 = Common_Json_GetAttrValue(pResult,-1,"ResList",NULL,NULL,NULL,NULL);
            if(pArry_root2 != NULL)
            {

                nCount = Common_Json_ArraySize(pArry_root2);
                for(nIdx = 0; nIdx < nCount; nIdx++)
                {
                    str_proto= NULL;
                    str_uuid = NULL;
                    str_apple = NULL;
                    str_android= NULL;
                    Common_Json_GetAttrValue(pArry_root2,nIdx,"ServerName",NULL,&str_proto,NULL,NULL);
                    Common_Json_GetAttrValue(pArry_root2,nIdx,"UUID",NULL,&str_uuid,NULL,NULL);
                    Common_Json_GetAttrValue(pArry_root2,nIdx,"AppleUrl",NULL,&str_apple,NULL,NULL);
                    Common_Json_GetAttrValue(pArry_root2,nIdx,"AndroidUrl",NULL,&str_android,NULL,NULL);
                    if(str_proto != NULL)
                    {
                        pArry_root1 = Common_Json_SetAttrValue(pArry_root, nIdx1, NULL, Common_Json_Type_Array, NULL, 0, 0);
                        ///////
                        if(pArry_root1 != NULL)
                        {
                            nIdx2 = 0;
                            Common_Json_SetAttrValue(pArry_root1, nIdx2, NULL, Common_Json_Type_String, str_proto, 0, 0);
                            nIdx2++;
                            if(str_apple != NULL)
                            {
                                pArry_root3 = Common_Json_SetAttrValue(pArry_root1, nIdx2, NULL, Common_Json_Type_Array, NULL, 0, 0);
                                Common_Json_SetAttrValue(pArry_root3, 0, NULL, Common_Json_Type_String, "IOS App", 0, 0);
                                Common_Json_SetAttrValue(pArry_root3, 1, NULL, Common_Json_Type_String, str_apple?str_apple:"", 0, 0);
                                nIdx2++;
                            }
                            if(str_android != NULL)
                            {
                                pArry_root3 = Common_Json_SetAttrValue(pArry_root1, nIdx2, NULL, Common_Json_Type_Array, NULL, 0, 0);
                                Common_Json_SetAttrValue(pArry_root3, 0, NULL, Common_Json_Type_String, "Android App", 0, 0);
                                Common_Json_SetAttrValue(pArry_root3, 1, NULL, Common_Json_Type_String, str_android?str_android:"", 0, 0);
                                nIdx2++;
                            }
                            if(str_uuid != NULL)
                            {
                                pArry_root3 = Common_Json_SetAttrValue(pArry_root1, nIdx2, NULL, Common_Json_Type_Array, NULL, 0, 0);
                                Common_Json_SetAttrValue(pArry_root3, 0, NULL, Common_Json_Type_String, "Device ID", 0, 0);
                                Common_Json_SetAttrValue(pArry_root3, 1, NULL, Common_Json_Type_String, str_uuid?str_uuid:"", 0, 0);
                                nIdx2++;
                            }
                        }
                        ///////
                        nIdx1++;
                    }
                }
            }
            if(pResult)
            {
                Common_Json_Delete(pResult);
                pResult = NULL;
            }
            if(lowerData)
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }
        }
        {
            bool flg = (0 == access("/root/xiaodingp2p",F_OK)) || (0 == access("/root/p2p/autop2p4ovfs",F_OK)
                       ||0 == access("/root/bin/xiaodingp2p",F_OK)) || (0 == access("/root/bin/autop2p4ovfs",F_OK));
            if(nCount == 0 && flg)
            {
                pArry_root1 = Common_Json_SetAttrValue(pArry_root, nIdx1, NULL, Common_Json_Type_Array, NULL, 0, 0);
                if(pArry_root1 != NULL)
                {
                    Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, "UUID", 0, 0);
                    Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
                    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
                    if (0 == ret)
                    {
                        Common_Json_GetAttrValueStr(lowerData, "SerialNumber", &str_uuid);
                        pArry_root3 = Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Array, NULL, 0, 0);
                        Common_Json_SetAttrValue(pArry_root3, 0, NULL, Common_Json_Type_String, " ", 0, 0);
                        Common_Json_SetAttrValue(pArry_root3, 1, NULL, Common_Json_Type_String, str_uuid?str_uuid:"", 0, 0);

                    }
                }
            }
        }

    }
#endif
    return  ret;
}

int frmDevicePara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_devicepara(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_devicepara(header, indata, outdata);
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

int frmVideoFormatPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_videoformatpara(header, indata, outdata,opt);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_videoformatpara(header, indata, outdata,opt);
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

int frmVideoFormatPara_v2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_videoformatpara_v2(header, indata, outdata,opt);
            break;
        case 1:
            ret = web_semantic_set_videoformatpara_v2(header, indata, outdata,opt);
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
//设备校时提交
int frmDeviceTimeCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_devicetimectrl(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_devicetimectrl(header, indata, outdata);
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

int frmDstPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_dstpara(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_dstpara(header, indata, outdata);
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

int frmGetFactoryInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_factoryInfo(header, indata, outdata);
    }

    return ret;
}

int frmDeviceAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
        ret = web_semantic_get_deviceAbility(header, indata, outdata);
	}

    return ret;
}
//#ifdef WITH_PTZ

//解码参数提交
int frmDecoderPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_decoderpara(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_decoderpara(header, indata, outdata);
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

int frmGetPTZProtocal(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_PTZProtocal(header, indata, outdata);
    }

    return ret;
}
//#endif

//得到bmp图的base64
int web_semantic_get_qrcode_base64(cJSON_Struct * header, cJSON_Struct * indata, cJSON_Struct * outdata)
{
    int ret = 0;
    int list_size = 0;
    int i_loop = 0;
    //char m_code[64];
    char *str = NULL;
    cJSON_Struct *list = NULL;
    cJSON_Struct *setlist = NULL;

    list = Common_Json_GetAttrValueArr(indata, "QrCodeList");
    if (list)
    {
        setlist = Common_Json_SetAttrValueArr(outdata, "QrCodeList");
        list_size = Common_Json_ArraySize(list);

        for(i_loop = 0; i_loop < list_size; i_loop++)
        {
            str = NULL;
            Common_Json_GetAttrValue(list, i_loop, NULL, NULL, &str, NULL, NULL);
            //     memset(m_code,0,sizeof(m_code));
            //     memcpy(m_code, str, strlen(str));
            //     LOGW("str:[%d][%s] code:[%s]\n",strlen(str),str,m_code);
            if (str)//m_code[0] != 0)
            {
                Common_QRCode_T tPixelInfo;
                memset(&tPixelInfo, 0, sizeof(Common_QRCode_T));
                tPixelInfo.byMargin = 1;
                tPixelInfo.byPicType =1;
                tPixelInfo.byPixelWide = 3;
                if(!Common_QuickResponseCode(str,&tPixelInfo))
                {


                    // websWriteBlock(wp, (char*)tPixelInfo.pPixelData, tPixelInfo.dwDataSize);
                    int len = 0;
                    char *pic_encode = NULL;
                    pic_encode = Common_Base64_Encode((char*)tPixelInfo.pPixelData, tPixelInfo.dwDataSize, &len);

                    //LOGD("pic_encode len:[%d] data:[%s]\n",len,pic_encode);

                    Common_Json_SetAttrValueArrStr(setlist, i_loop, pic_encode);
                    if(pic_encode)
                    {
                        Common_Free(pic_encode, __FUNCTION__, __LINE__);
                    }

                    Common_Free(tPixelInfo.pPixelData, __FUNCTION__, __LINE__);
                }
                else
                {
                    ret = WEB_CODE_InternalMistake;

                    Common_Json_SetAttrValueArrStr(setlist, i_loop, "QR failed!");
                    Common_Free(tPixelInfo.pPixelData, __FUNCTION__, __LINE__);
                }
            }
        }
    }

    return ret;

}

int frmGetQRCodePictureV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if(opt->type == 0)
    {
        ret = web_semantic_get_qrcode_base64(header, indata, outdata);
    }
    else
    {
        ret = WEB_CODE_InvalidArg;
    }

    return ret;
}

int frmAudioPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_audiopara(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_audiopara(header, indata, outdata);
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
int frmAudioParaAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_audioparaability(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    return ret;
}


//初始化获得28181协议参数
int frmParaPlatform28181(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_gb28181para(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_gb28181para(header, indata, outdata);
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

static int web_semantic_get_formatability(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/boardSys/videoinput/ability/all");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }
    cJSON_Struct *pArry_root = NULL;
    if (0 == ret)
    {
        if ((pArry_root = Common_Json_SetAttrValue(outdata, -1, "FormatList", Common_Json_Type_Array, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }
    if (0 == ret)
    {
        int nloop = 0;
        int i_num = 0;
        char *str_tmp = NULL;
        int protocolSize = 0;
        cJSON_Struct *pArry_tmp = NULL;
        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "AbilityList", NULL, NULL, NULL, NULL);
        if (pArry_tmp)
        {
            protocolSize = Common_Json_Size(pArry_tmp);
            for (nloop = 0; nloop < protocolSize; nloop ++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, nloop, "Device", NULL, &str_tmp, NULL, NULL);
                if((int)str_tmp != opt->dev)continue;
                cJSON_Struct *pResoList = Common_Json_GetAttrValue(pArry_tmp, nloop, "ResoList", NULL, NULL, NULL, NULL);
                int iResoSize = Common_Json_Size(pResoList);
                int tloop = 0;
                for (tloop = 0; tloop < iResoSize; tloop ++)
                {
                    Common_Json_GetAttrValue(pResoList, tloop, "W", NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArry_root, tloop, "W", Common_Json_Type_Number, NULL, i_num, 0);
                    Common_Json_GetAttrValue(pResoList, tloop, "H", NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArry_root, tloop, "H", Common_Json_Type_Number, NULL, i_num, 0);
                    Common_Json_GetAttrValue(pResoList, tloop, "Fps", NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArry_root, tloop, "Fps", Common_Json_Type_Number, NULL, i_num, 0);
                    Common_Json_GetAttrValue(pResoList, tloop, "Format", NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValue(pArry_root, tloop, "Format", Common_Json_Type_Number, NULL, i_num, 0);
                }
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return  ret;
}
int frmGetFormatAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_formatability(header, indata, outdata, opt);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }
    return ret;
}

static int web_semantic_get_imagemodeability(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/boardSys/videoinput/imagemode/ability");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }


    if (0 == ret)
    {
        JsonOper_MergeObj(outdata, lowerData, 0);
    }


    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return  ret;
}

int frmGetImageModeAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_imagemodeability(header,indata,outdata,opt);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }
    return ret;
}
static int web_semantic_get_videoimagemode(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/VideoInput/imagemode/attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            int VideoFormat = 0;
            Common_Json_GetAttrValueInt(lowerData, "Value", &VideoFormat);
            Common_Json_SetAttrValueInt(outdata, "Value", VideoFormat);
            char* str = NULL;
            Common_Json_GetAttrValueStr(lowerData, "DesString",  &str);
            Common_Json_SetAttrValueStr(outdata,  "DesString",  str);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return  ret;
}
static int web_semantic_set_videoimagemode(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char* str = NULL;
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
        Common_Json_GetAttrValueInt(indata,"Value",&i_num);
        Common_Json_SetAttrValueInt(lowerData,"Value",i_num);
        Common_Json_GetAttrValueStr(indata,"DesString",&str);
        Common_Json_SetAttrValueStr(lowerData,"DesString",str);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/VideoInput/imagemode/attribute");

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

int frmVideoImageMode(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_videoimagemode(header, indata, outdata,opt);
            break;
        case 1:
            ret = web_semantic_set_videoimagemode(header, indata, outdata,opt);
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

int GetDeviceInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {

            *nOutBufSize = sizeof(THIRD_DEVICE_INFO);
            *pOutBuffer = (THIRD_DEVICE_INFO *)Common_Calloc(1,sizeof(THIRD_DEVICE_INFO),__FUNCTION__,__LINE__);
            THIRD_DEVICE_INFO *devinfo = NULL;
            devinfo = (THIRD_DEVICE_INFO *)(*pOutBuffer);
            devinfo->UseMask = 0xffff;

            Common_Json_GetAttrValueInt(lowerData, "IsOfDome", &devinfo->IsOfDome);
            Common_Json_GetAttrValueInt(lowerData,"LensSupport", &devinfo->LensSupport);
            Common_Json_GetAttrValueInt(lowerData, "IrisSupport", &devinfo->IrisSupport);

			str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "DeviceName", &str_tmp);
            snprintf(devinfo->DeviceName,sizeof(devinfo->DeviceName),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "DeviceModel", &str_tmp);
            snprintf(devinfo->DeviceModel,sizeof(devinfo->DeviceModel),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "DeviceType", &str_tmp);
            snprintf(devinfo->DeviceType,sizeof(devinfo->DeviceType),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "SerialNumber", &str_tmp);
            snprintf(devinfo->SerialNumber,sizeof(devinfo->SerialNumber),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "HardVersion", &str_tmp);
            snprintf(devinfo->HardVersion,sizeof(devinfo->HardVersion),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "ProductDate", &str_tmp);
            snprintf(devinfo->ProductDate,sizeof(devinfo->ProductDate),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "Manufacturer", &str_tmp);
            snprintf(devinfo->Manufacturer,sizeof(devinfo->Manufacturer),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "Hardware", &str_tmp);
            snprintf(devinfo->Hardware,sizeof(devinfo->Hardware),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "Country", &str_tmp);
            snprintf(devinfo->Country,sizeof(devinfo->Country),"%s",str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "City", &str_tmp);
            snprintf(devinfo->City,sizeof(devinfo->City),"%s",str_tmp);

        }

    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;

}

int DeviceRestore(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
    int iloop = 0;
    cJSON_Struct *lowerArray = NULL;
    cJSON_Struct *lowerData = NULL;

	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

    if (1 == nOutDataType)
    {
    	THIRD_RESTORE *pDevResore = NULL;

		pDevResore = (THIRD_RESTORE *)pInBuffer;

        iloop = 0;
        lowerArray = Common_Json_SetAttrValueArr(lowerData, "ResList");

        if (NULL != pDevResore && 1 == pDevResore->NetConfig)
        {
            Common_Json_SetAttrValueArrStr(lowerArray, iloop, "NetConfig");
            iloop ++;
        }

        if (NULL != pDevResore && 1 == pDevResore->AlarmConfig)
        {
            Common_Json_SetAttrValueArrStr(lowerArray, iloop, "AlarmConfig");
            iloop ++;
        }


        if (NULL != pDevResore && 1 == pDevResore->UserConfig)
        {
            Common_Json_SetAttrValueArrStr(lowerArray, iloop, "UserConfig");
            iloop ++;
        }

        if (NULL != pDevResore && 1 == pDevResore->Others)
        {
            Common_Json_SetAttrValueArrStr(lowerArray, iloop, "Others");
            iloop ++;
        }

		Common_Json_SetAttrValueInt(lowerData, "NeedReboot", 1);
    }

	Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Restore/Items");

	ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Attribute/Restore");

	ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

    return WEB_CODE_OK;
}


int DeviceReboot(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

    if (1 == nOutDataType)
    {
		THIRD_REBOOT *pDevReboot = NULL;

		pDevReboot = (THIRD_REBOOT *)pInBuffer;

        Common_Json_SetAttrValueInt(lowerData, "Delay", pDevReboot->Delay);
    }

	Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Power/Reboot");

	ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if (lowerData)
    {
        Common_Json_Delete(lowerData);

        lowerData = NULL;
    }

    return WEB_CODE_OK;
}


int GetMotionTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	int vint = -1;
	char uriname[128] = {0};
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Alarm/TriggerCfg/Motion");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if(nOutDataType == 1)
    {
		THIRD_TIME_SCHEDULE *pTimeSchedule = NULL;
		pTimeSchedule = (THIRD_TIME_SCHEDULE *)Common_Calloc(1,sizeof(THIRD_TIME_SCHEDULE),__FUNCTION__,__LINE__);
		memset(pTimeSchedule, 0, sizeof(THIRD_TIME_SCHEDULE));

	    cJSON_Struct *pArry_list = Common_Json_GetAttrValueArr(lowerData, "ResList");

		if (pArry_list)
	    {
			cJSON_Struct *pFromJson = Common_Json_GetAttrValueArrItem(pArry_list, 0);

			if (pFromJson)
			{
				vint = -1;
				Common_Json_GetAttrValueInt(pFromJson, "Enable", &vint);
				pTimeSchedule->Enable = vint;

		        for (int i = 0; i < SCHEDULE_MAX_DAY; i++)
		        {
		            for (int j = 0; j < SCHEDULE_MAX_SEGMENT; j++)
		            {
		                vint = -1;
						memset(uriname, 0, sizeof(uriname));
		                snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Start", i, j);
						Common_Json_GetAttrValueInt(pFromJson, uriname, &vint);
		                pTimeSchedule->Schedule[i][j].StartHour = vint / 100;
		                pTimeSchedule->Schedule[i][j].StartMinute = vint % 100;

		                vint = -1;
						memset(uriname, 0, sizeof(uriname));
		                snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Stop", i, j);
						Common_Json_GetAttrValueInt(pFromJson, uriname, &vint);
		                pTimeSchedule->Schedule[i][j].EndHour = vint / 100;
		                pTimeSchedule->Schedule[i][j].EndMinute = vint % 100;
		            }
		        }
			}
	    }

		*pOutBuffer = pTimeSchedule;
        *nOutBufSize = sizeof(THIRD_TIME_SCHEDULE);
    }

	if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

    return ret;
}


int SetMotionTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	char uriname[128] = {0};
	cJSON_Struct *lowerData = NULL;
	cJSON_Struct *lowerArray = NULL;
	cJSON_Struct *pFromJson = NULL;

	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

	lowerArray = Common_Json_SetAttrValueArr(lowerData, "ResList");
	pFromJson = Common_Json_SetAttrValueArrObj(lowerArray, 0);
	Common_Json_SetAttrValueInt(pFromJson, "Device", 0);
	Common_Json_SetAttrValueInt(pFromJson, "Channel", 0);
	if (1 == nOutDataType)
	{
		THIRD_TIME_SCHEDULE *pTimeSchedule = NULL;
		pTimeSchedule = (THIRD_TIME_SCHEDULE *)pInBuffer;

		if (pTimeSchedule->UseMask & 0x01)
		{
			Common_Json_SetAttrValueInt(pFromJson, "Enable", pTimeSchedule->Enable);
		}

		for (int i = 0; i < SCHEDULE_MAX_DAY; i++)
		{
			for (int j = 0; j < SCHEDULE_MAX_SEGMENT; j++)
			{
				snprintf(uriname, sizeof(uriname), "Weekday%d", i);
				Common_Json_SetAttrValueObj(pFromJson, uriname);
				snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d", i, j);
				Common_Json_SetAttrValueObj(pFromJson, uriname);
				snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Start", i, j);
				Common_Json_SetAttrValueInt(pFromJson, uriname, pTimeSchedule->Schedule[i][j].StartHour * 100 + pTimeSchedule->Schedule[i][j].StartMinute);

				snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Stop", i, j);
				Common_Json_SetAttrValueInt(pFromJson, uriname, pTimeSchedule->Schedule[i][j].EndHour * 100 + pTimeSchedule->Schedule[i][j].EndMinute);
			}
		}
	}

	Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/TriggerCfg/Motion");

	ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

	return WEB_CODE_OK;
}

int GetVHideTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	int vint = -1;
	char uriname[128] = {0};
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Alarm/TriggerCfg/Vhide");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if(nOutDataType == 1)
    {
		THIRD_TIME_SCHEDULE *pTimeSchedule = NULL;
		pTimeSchedule = (THIRD_TIME_SCHEDULE *)Common_Calloc(1,sizeof(THIRD_TIME_SCHEDULE),__FUNCTION__,__LINE__);
		memset(pTimeSchedule, 0, sizeof(THIRD_TIME_SCHEDULE));

	    cJSON_Struct *pArry_list = Common_Json_GetAttrValueArr(lowerData, "ResList");

		if (pArry_list)
	    {
			cJSON_Struct *pFromJson = Common_Json_GetAttrValueArrItem(pArry_list, 0);

			if (pFromJson)
			{
				vint = -1;
				Common_Json_GetAttrValueInt(pFromJson, "Enable", &vint);
				pTimeSchedule->Enable = vint;

		        for (int i = 0; i < SCHEDULE_MAX_DAY; i++)
		        {
		            for (int j = 0; j < SCHEDULE_MAX_SEGMENT; j++)
		            {
		                vint = -1;
						memset(uriname, 0, sizeof(uriname));
		                snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Start", i, j);
						Common_Json_GetAttrValueInt(pFromJson, uriname, &vint);
		                pTimeSchedule->Schedule[i][j].StartHour = vint / 100;
		                pTimeSchedule->Schedule[i][j].StartMinute = vint % 100;

		                vint = -1;
						memset(uriname, 0, sizeof(uriname));
		                snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Stop", i, j);
						Common_Json_GetAttrValueInt(pFromJson, uriname, &vint);
		                pTimeSchedule->Schedule[i][j].EndHour = vint / 100;
		                pTimeSchedule->Schedule[i][j].EndMinute = vint % 100;
		            }
		        }
			}
	    }

		*pOutBuffer = pTimeSchedule;
        *nOutBufSize = sizeof(THIRD_TIME_SCHEDULE);
    }

	if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

    return ret;
}


int SetVHideTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	char uriname[128] = {0};
	cJSON_Struct *lowerData = NULL;
	cJSON_Struct *lowerArray = NULL;
	cJSON_Struct *pFromJson = NULL;
	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

	lowerArray = Common_Json_SetAttrValueArr(lowerData, "ResList");
	pFromJson = Common_Json_SetAttrValueArrObj(lowerArray, 0);
	Common_Json_SetAttrValueInt(pFromJson, "Device", 0);
	Common_Json_SetAttrValueInt(pFromJson, "Channel", 0);
	if (1 == nOutDataType)
	{
		THIRD_TIME_SCHEDULE *pTimeSchedule = NULL;
		pTimeSchedule = (THIRD_TIME_SCHEDULE *)pInBuffer;




		if (pTimeSchedule->UseMask & 0x01)
		{
			Common_Json_SetAttrValueInt(pFromJson, "Enable", pTimeSchedule->Enable);
		}

		if ((pTimeSchedule->UseMask >> 1) & 0x01)
		{
			for (int i = 0; i < SCHEDULE_MAX_DAY; i++)
			{
				for (int j = 0; j < SCHEDULE_MAX_SEGMENT; j++)
				{
					snprintf(uriname, sizeof(uriname), "Weekday%d", i);
					Common_Json_SetAttrValueObj(pFromJson, uriname);

					snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d", i, j);
					Common_Json_SetAttrValueObj(pFromJson, uriname);

					snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Start", i, j);
					Common_Json_SetAttrValueInt(pFromJson, uriname, pTimeSchedule->Schedule[i][j].StartHour * 100 + pTimeSchedule->Schedule[i][j].StartMinute);

					snprintf(uriname, sizeof(uriname), "Weekday%d/Sched%d/Stop", i, j);
					Common_Json_SetAttrValueInt(pFromJson, uriname, pTimeSchedule->Schedule[i][j].EndHour * 100 + pTimeSchedule->Schedule[i][j].EndMinute);
				}
			}
		}
	}

	Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/TriggerCfg/Vhide");

	ret = Ovfs_Web_RestMethodA(header, lowerArray, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerArray)
	    {
	        Common_Json_Delete(lowerArray);

			lowerArray = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if (lowerArray)
    {
        Common_Json_Delete(lowerArray);

		lowerArray = NULL;
    }

	return WEB_CODE_OK;
}

int GetMotionCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	int vint = -1;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Event/Motion/Attribute/Device0/Channel0");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if(nOutDataType == 1)
    {
		THIRD_MOTION_CFG *pMotionCfg = NULL;
		pMotionCfg = (THIRD_MOTION_CFG *)Common_Calloc(1,sizeof(THIRD_MOTION_CFG),__FUNCTION__,__LINE__);
		memset(pMotionCfg, 0, sizeof(THIRD_MOTION_CFG));

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "Enable", &vint);
		pMotionCfg->Enable = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "Sensitivity", &vint);
		pMotionCfg->Sensitivity = vint;

		str_tmp = NULL;
		Common_Json_GetAttrValueStr(lowerData, "Rect", &str_tmp);
		strcpy(pMotionCfg->RectStr,str_tmp);

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "BlockW", &vint);
		pMotionCfg->BlockW = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "BlockH", &vint);
		pMotionCfg->BlockH = vint;

		*pOutBuffer = pMotionCfg;
        *nOutBufSize = sizeof(THIRD_MOTION_CFG);
    }

	if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

    return ret;
}

int SetMotionCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
	int ret = 0;
	cJSON_Struct *lowerData = NULL;

	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

	if (1 == nOutDataType)
	{
		THIRD_MOTION_CFG *pMotionCfg = NULL;

		pMotionCfg = (THIRD_MOTION_CFG *)pInBuffer;

		if ((pMotionCfg->UseMask >> 0) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "Enable", pMotionCfg->Enable);
		}

		if ((pMotionCfg->UseMask >> 1) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "Sensitivity", pMotionCfg->Sensitivity);
		}

		if ((pMotionCfg->UseMask >> 2) & 0x01)
		{
			Common_Json_SetAttrValueStr(lowerData, "Rect", pMotionCfg->RectStr);
		}

		if ((pMotionCfg->UseMask >> 3) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "BlockW", pMotionCfg->BlockW);
		}

		if ((pMotionCfg->UseMask >> 4) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "BlockH", pMotionCfg->BlockH);
		}
	}

	Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Event/Motion/Attribute/Device0/Channel0");

	ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

	return WEB_CODE_OK;
}

int GetVHideCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	int vint = -1;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Event/Hide/Attribute/Device0/Channel0");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if(nOutDataType == 1)
    {
		THIRD_VIDEO_HIDE *pVHideCfg = NULL;

		pVHideCfg = (THIRD_VIDEO_HIDE *)Common_Calloc(1,sizeof(THIRD_VIDEO_HIDE),__FUNCTION__,__LINE__);

		memset(pVHideCfg, 0, sizeof(THIRD_VIDEO_HIDE));

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "Enable", &vint);
		pVHideCfg->Enable = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "Sensitivity", &vint);
		pVHideCfg->Sensitivity = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "X", &vint);
		pVHideCfg->VhideRegionCfg.X = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "Y", &vint);
		pVHideCfg->VhideRegionCfg.Y = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "W", &vint);
		pVHideCfg->VhideRegionCfg.W = vint;

		vint = -1;
		Common_Json_GetAttrValueInt(lowerData, "H", &vint);
		pVHideCfg->VhideRegionCfg.H = vint;

		*pOutBuffer = pVHideCfg;
        *nOutBufSize = sizeof(THIRD_VIDEO_HIDE);
    }

	if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

    return WEB_CODE_OK;
}

int SetVHideCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
	int ret = 0;
	cJSON_Struct *lowerData = NULL;

	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

	if (1 == nOutDataType)
	{
		THIRD_VIDEO_HIDE *pVHideCfg = NULL;

		pVHideCfg = (THIRD_VIDEO_HIDE *)pInBuffer;

		if ((pVHideCfg->UseMask >> 0) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "Enable", pVHideCfg->Enable);
		}

		if ((pVHideCfg->UseMask >> 1) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "Sensitivity", pVHideCfg->Sensitivity);
		}

		if ((pVHideCfg->UseMask >> 2) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "X", pVHideCfg->VhideRegionCfg.X);
		}

		if ((pVHideCfg->UseMask >> 3) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "Y", pVHideCfg->VhideRegionCfg.Y);
		}

		if ((pVHideCfg->UseMask >> 4) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "W", pVHideCfg->VhideRegionCfg.W);
		}

		if ((pVHideCfg->UseMask >> 5) & 0x01)
		{
			Common_Json_SetAttrValueInt(lowerData, "W", pVHideCfg->VhideRegionCfg.H);
		}
	}

	Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Event/Hide/Attribute/Device0/Channel0");

	ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

	if (0 != ret)
	{
		LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

		if (lowerData)
	    {
	        Common_Json_Delete(lowerData);

			lowerData = NULL;
	    }

		return WEB_CODE_InternalMistake;
	}

    if (lowerData)
    {
        Common_Json_Delete(lowerData);

		lowerData = NULL;
    }

	return WEB_CODE_OK;
}

int GetVideoMaskCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	int nSuccCount = 0;
	int vint = -1;
	char uriname[128] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
	THIRD_MASK_CFG struMaskCfg[4] = {0};

	if(nOutDataType == 1)
	{
		for(int i = 0; i < 4; i++)
		{
			snprintf(uriname, sizeof(uriname), "/BoardSys/Mask/Device0/Channel0/Rect%d",i);

			Ovfs_Web_UpdateHeader(header, REST_GET, uriname);

		    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

			//失败一个就返回失败
			if (0 != ret)
			{
				LOGE("call Ovfs_Web_RestMethodA failed(%d)",ret);

				if (lowerData)
			    {
			        Common_Json_Delete(lowerData);

					lowerData = NULL;
			    }

				return WEB_CODE_InternalMistake;
			}
			else
			{
				vint = -1;
				Common_Json_GetAttrValueInt(lowerData, "Enable", &vint);
				struMaskCfg[i].Enable = vint;

				str_tmp = NULL;
				Common_Json_GetAttrValueInt(lowerData, "X", &vint);
				struMaskCfg[i].X = vint;

				vint = -1;
				Common_Json_GetAttrValueInt(lowerData, "Y", &vint);
				struMaskCfg[i].Y = vint;

				vint = -1;
				Common_Json_GetAttrValueInt(lowerData, "W", &vint);
				struMaskCfg[i].W = vint;

				vint = -1;
				Common_Json_GetAttrValueInt(lowerData, "H", &vint);
				struMaskCfg[i].H = vint;

				nSuccCount++;
			}

            if (lowerData)
            {
                Common_Json_Delete(lowerData);

                lowerData = NULL;
            }
		}

		THIRD_MASK_RECT *pVMaskCfg = NULL;
		pVMaskCfg = (THIRD_MASK_RECT *)Common_Calloc(1,sizeof(THIRD_MASK_RECT),__FUNCTION__,__LINE__);
		memset(pVMaskCfg, 0, sizeof(THIRD_MASK_RECT));

		pVMaskCfg->MaskCount = 4;
		pVMaskCfg->pMaskCfg = Common_Calloc(pVMaskCfg->MaskCount,sizeof(THIRD_MASK_CFG),__FUNCTION__,__LINE__);
		memcpy(pVMaskCfg->pMaskCfg, &struMaskCfg, sizeof(THIRD_MASK_CFG) * pVMaskCfg->MaskCount);

		*pOutBuffer = pVMaskCfg;
		*nOutBufSize = sizeof(THIRD_MASK_RECT);
	}

    return ret;
}

int SetVideoMaskCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
	char uriname[128] = {0};
	cJSON_Struct *lowerData = NULL;

	lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

	if (NULL == lowerData)
    {
		LOGE("create memory failed");

        return WEB_CODE_LackingMem;
    }

	if(nOutDataType == 1)
	{
		THIRD_MASK_RECT *pVMaskCfg = NULL;
		pVMaskCfg = (THIRD_MASK_RECT *)pInBuffer;

		for(int i = 0; i < pVMaskCfg->MaskCount; i++)
		{
			if (pVMaskCfg->UseMask & 0x01)
			{
				Common_Json_SetAttrValueInt(lowerData, "Enable", pVMaskCfg->pMaskCfg[i].Enable);

				Common_Json_SetAttrValueInt(lowerData, "X", pVMaskCfg->pMaskCfg[i].X);

				Common_Json_SetAttrValueInt(lowerData, "Y", pVMaskCfg->pMaskCfg[i].Y);

				Common_Json_SetAttrValueInt(lowerData, "W", pVMaskCfg->pMaskCfg[i].W);

				Common_Json_SetAttrValueInt(lowerData, "H", pVMaskCfg->pMaskCfg[i].H);
			}

			snprintf(uriname, sizeof(uriname), "/BoardSys/Mask/Device0/Channel0/Rect%d",i);

			Ovfs_Web_UpdateHeader(header, REST_PUT, uriname);

			ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

			//有一个mask设置失败,直接返回失败
			if (ret != 0)
			{
				break;
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

int GetAllTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Time/AllTime");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_ALL_TIME);
            *pOutBuffer = (THIRD_ALL_TIME *)Common_Calloc(1,sizeof(THIRD_ALL_TIME),__FUNCTION__,__LINE__);
            THIRD_ALL_TIME *timeinfo = NULL;
            timeinfo = (THIRD_ALL_TIME *)(*pOutBuffer);
            timeinfo->UseMask = 0xff;

            Common_Json_GetAttrValueInt(lowerData, "UTCTime/Year", &timeinfo->UTCTime.Year);
            Common_Json_GetAttrValueInt(lowerData, "UTCTime/Month", &timeinfo->UTCTime.Month);
            Common_Json_GetAttrValueInt(lowerData, "UTCTime/Day", &timeinfo->UTCTime.Day);
            Common_Json_GetAttrValueInt(lowerData, "UTCTime/Hour", &timeinfo->UTCTime.Hour);
            Common_Json_GetAttrValueInt(lowerData, "UTCTime/Min", &timeinfo->UTCTime.Min);
            Common_Json_GetAttrValueInt(lowerData, "UTCTime/Sec", &timeinfo->UTCTime.Sec);

            Common_Json_GetAttrValueInt(lowerData, "SysTime/Year", &timeinfo->SysTime.Year);
            Common_Json_GetAttrValueInt(lowerData, "SysTime/Month", &timeinfo->SysTime.Month);
            Common_Json_GetAttrValueInt(lowerData, "SysTime/Day", &timeinfo->SysTime.Day);
            Common_Json_GetAttrValueInt(lowerData, "SysTime/Hour", &timeinfo->SysTime.Hour);
            Common_Json_GetAttrValueInt(lowerData, "SysTime/Min", &timeinfo->SysTime.Min);
            Common_Json_GetAttrValueInt(lowerData, "SysTime/Sec", &timeinfo->SysTime.Sec);

            Common_Json_GetAttrValueInt(lowerData, "NTP/Enable", &timeinfo->Ntp.Enable);
            if(Common_Json_GetAttrValueStr(lowerData, "NTP/Server", &str_tmp))
            {
                snprintf(timeinfo->Ntp.Server,sizeof(timeinfo->Ntp.Server),"%s",str_tmp);
            }
            Common_Json_GetAttrValueInt(lowerData, "NTP/Interval", &timeinfo->Ntp.Interval);

            Common_Json_GetAttrValueInt(lowerData, "DST/Enable", &timeinfo->Dst.Enable);
            Common_Json_GetAttrValueInt(lowerData, "DST/Mode", &timeinfo->Dst.Mode);
            Common_Json_GetAttrValueInt(lowerData, "DST/Bias", &timeinfo->Dst.Bias);
            Common_Json_GetAttrValueInt(lowerData, "DST/StartTime/Month", &timeinfo->Dst.StartTime.Month);
            Common_Json_GetAttrValueInt(lowerData, "DST/StartTime/WeekIdx", &timeinfo->Dst.StartTime.WeekIdx);
            Common_Json_GetAttrValueInt(lowerData, "DST/StartTime/WeekDay", &timeinfo->Dst.StartTime.WeekDay);
            Common_Json_GetAttrValueInt(lowerData, "DST/StartTime/Hour", &timeinfo->Dst.StartTime.Hour);
            Common_Json_GetAttrValueInt(lowerData, "DST/StartTime/Min", &timeinfo->Dst.StartTime.Min);

            Common_Json_GetAttrValueInt(lowerData, "DST/StopTime/Month", &timeinfo->Dst.StopTime.Month);
            Common_Json_GetAttrValueInt(lowerData, "DST/StopTime/WeekIdx", &timeinfo->Dst.StopTime.WeekIdx);
            Common_Json_GetAttrValueInt(lowerData, "DST/StopTime/WeekDay", &timeinfo->Dst.StopTime.WeekDay);
            Common_Json_GetAttrValueInt(lowerData, "DST/StopTime/Hour", &timeinfo->Dst.StopTime.Hour);
            Common_Json_GetAttrValueInt(lowerData, "DST/StopTime/Min", &timeinfo->Dst.StopTime.Min);

            Common_Json_GetAttrValueInt(lowerData, "TimeZone/Zone", &timeinfo->TimeZone.Zone);
            Common_Json_GetAttrValueInt(lowerData, "TimeZone/EnableBias", &timeinfo->TimeZone.EnableBias);
            Common_Json_GetAttrValueInt(lowerData, "TimeZone/ZoneBias", &timeinfo->TimeZone.ZoneBias);

        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetAllTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_ALL_TIME *timeinfo = NULL;
            timeinfo = (THIRD_ALL_TIME *)pInBuffer;

            if(timeinfo->UseMask & 0x1)
            {
                Common_Json_SetAttrValueObj(lowerData, "UTCTime");

                Common_Json_SetAttrValueInt(lowerData, "UTCTime/Year", timeinfo->UTCTime.Year);
                Common_Json_SetAttrValueInt(lowerData, "UTCTime/Month", timeinfo->UTCTime.Month);
                Common_Json_SetAttrValueInt(lowerData, "UTCTime/Day", timeinfo->UTCTime.Day);
                Common_Json_SetAttrValueInt(lowerData, "UTCTime/Hour", timeinfo->UTCTime.Hour);
                Common_Json_SetAttrValueInt(lowerData, "UTCTime/Min", timeinfo->UTCTime.Min);
                Common_Json_SetAttrValueInt(lowerData, "UTCTime/Sec", timeinfo->UTCTime.Sec);
            }

            if(timeinfo->UseMask & 0x2)
            {
                Common_Json_SetAttrValueObj(lowerData, "SysTime");

                Common_Json_SetAttrValueInt(lowerData, "SysTime/Year", timeinfo->SysTime.Year);
                Common_Json_SetAttrValueInt(lowerData, "SysTime/Month", timeinfo->SysTime.Month);
                Common_Json_SetAttrValueInt(lowerData, "SysTime/Day", timeinfo->SysTime.Day);
                Common_Json_SetAttrValueInt(lowerData, "SysTime/Hour", timeinfo->SysTime.Hour);
                Common_Json_SetAttrValueInt(lowerData, "SysTime/Min", timeinfo->SysTime.Min);
                Common_Json_SetAttrValueInt(lowerData, "SysTime/Sec", timeinfo->SysTime.Sec);
            }

            if(timeinfo->UseMask & 0x4)
            {
                Common_Json_SetAttrValueObj(lowerData, "NTP");

                Common_Json_SetAttrValueInt(lowerData, "NTP/Enable", timeinfo->Ntp.Enable);
                Common_Json_SetAttrValueStr(lowerData, "NTP/Server", timeinfo->Ntp.Server);
                Common_Json_SetAttrValueInt(lowerData, "NTP/Interval", timeinfo->Ntp.Interval);
            }

            if(timeinfo->UseMask & 0x8)
            {
                Common_Json_SetAttrValueObj(lowerData, "DST");

                Common_Json_SetAttrValueInt(lowerData, "DST/Enable", timeinfo->Dst.Enable);
                Common_Json_SetAttrValueInt(lowerData, "DST/Mode", timeinfo->Dst.Mode);
                Common_Json_SetAttrValueInt(lowerData, "DST/Bias", timeinfo->Dst.Bias);

                Common_Json_SetAttrValueObj(lowerData, "DST/StartTime");

                Common_Json_SetAttrValueInt(lowerData, "DST/StartTime/Month", timeinfo->Dst.StartTime.Month);
                Common_Json_SetAttrValueInt(lowerData, "DST/StartTime/WeekIdx", timeinfo->Dst.StartTime.WeekIdx);
                Common_Json_SetAttrValueInt(lowerData, "DST/StartTime/WeekDay", timeinfo->Dst.StartTime.WeekDay);
                Common_Json_SetAttrValueInt(lowerData, "DST/StartTime/Hour", timeinfo->Dst.StartTime.Hour);
                Common_Json_SetAttrValueInt(lowerData, "DST/StartTime/Min", timeinfo->Dst.StartTime.Min);

                Common_Json_SetAttrValueObj(lowerData, "DST/StopTime");

                Common_Json_SetAttrValueInt(lowerData, "DST/StopTime/Month", timeinfo->Dst.StopTime.Month);
                Common_Json_SetAttrValueInt(lowerData, "DST/StopTime/WeekIdx", timeinfo->Dst.StopTime.WeekIdx);
                Common_Json_SetAttrValueInt(lowerData, "DST/StopTime/WeekDay", timeinfo->Dst.StopTime.WeekDay);
                Common_Json_SetAttrValueInt(lowerData, "DST/StopTime/Hour", timeinfo->Dst.StopTime.Hour);
                Common_Json_SetAttrValueInt(lowerData, "DST/StopTime/Min", timeinfo->Dst.StopTime.Min);

            }


            if(timeinfo->UseMask & 0x10)
            {
                Common_Json_SetAttrValueObj(lowerData, "TimeZone");

                Common_Json_SetAttrValueInt(lowerData, "TimeZone/Zone", timeinfo->TimeZone.Zone);
                Common_Json_SetAttrValueInt(lowerData, "TimeZone/EnableBias", timeinfo->TimeZone.EnableBias);
                Common_Json_SetAttrValueInt(lowerData, "TimeZone/ZoneBias", timeinfo->TimeZone.ZoneBias);
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Time/AllTime");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetImageCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    char buf[128] = {0};

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        /*if (g_ovfs_web->devInfo.bPTZ == 1)
        {
            snprintf(buf,sizeof(buf),"/Ptz/Image/Attribute/All");
        }
        else*/
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Image/Attribute/All");
        }
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_IMAGE);
            *pOutBuffer = (THIRD_IMAGE *)Common_Calloc(1,sizeof(THIRD_IMAGE),__FUNCTION__,__LINE__);
            THIRD_IMAGE *imageinfo = NULL;
            imageinfo = (THIRD_IMAGE *)(*pOutBuffer);
            imageinfo->UseMask = 0xf;


            cJSON_Struct *pArry_root = NULL;
            pArry_root = Common_Json_GetAttrValue(lowerData, -1, "ImageList", NULL, NULL, NULL, NULL);

            int i;
            int arraySize = Common_Json_Size(pArry_root);
            for (i = 0; i < arraySize; i++)
            {
                int type = 0;
                int value = 0;
                int enable = 0;
                cJSON_Struct *tmp = NULL;

                Common_Json_GetAttrValue(pArry_root, i, "Type", NULL, NULL, &type, NULL);
                tmp = Common_Json_GetAttrValue(pArry_root,i,"Param",NULL,NULL,NULL,NULL);
                switch(type)
                {
                    case 0:
                        Common_Json_GetAttrValueInt(tmp, "Mode", &imageinfo->DayNightMode);
                        Common_Json_GetAttrValueInt(tmp, "DayToNightThreshold", &imageinfo->DayToNightThreshold);
                        Common_Json_GetAttrValueInt(tmp, "NightToDayThreshold", &imageinfo->NightToDayThreshold);
                        Common_Json_GetAttrValueInt(tmp, "Delay", &imageinfo->DayNightTime);
                        Common_Json_GetAttrValueInt(tmp, "DayStart", &imageinfo->DayNightStartTime);
                        Common_Json_GetAttrValueInt(tmp, "DayEnd", &imageinfo->DayNightEndTime);
                        break;
                    case 4:
                        Common_Json_GetAttrValueInt(tmp, "Enable", &enable);
                        Common_Json_GetAttrValueInt(tmp, "Level", &value);
                        imageinfo->Sharpness = enable?value:enable;
                        break;
                    case 6://wdr
                        Common_Json_GetAttrValueInt(tmp, "Enable", &imageinfo->WdrEnable);
                        Common_Json_GetAttrValueInt(tmp, "Level", &imageinfo->WdrLevel);
                        Common_Json_GetAttrValueInt(tmp, "Mode", &imageinfo->WdrMode);
                        break;
                    case 10://mirror
                        Common_Json_GetAttrValueInt(tmp, "Mode", &imageinfo->Mirror);
                        break;
                    case 15://bright
                        Common_Json_GetAttrValueInt(tmp, "Brightness", &imageinfo->Brightness);
                        Common_Json_GetAttrValueInt(tmp, "Contrast", &imageinfo->Contrast);
                        Common_Json_GetAttrValueInt(tmp, "Saturation", &imageinfo->Staturation);
                        Common_Json_GetAttrValueInt(tmp, "Hue", &imageinfo->Hue);
                        break;
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

int SetImageCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int i = 0;
    int ret = 0;
    char buf[128] = {0};

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    cJSON_Struct *pArry_root = Common_Json_SetAttrValueArr(lowerData, "ImageList");

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_IMAGE *imageinfo = NULL;
            imageinfo = (THIRD_IMAGE *)pInBuffer;
            if(imageinfo->UseMask & 0xf)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i++);
                Common_Json_SetAttrValueInt(tmp, "Type", 15);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                if(imageinfo->UseMask & 0x1)
                {
                    Common_Json_SetAttrValueInt(tmp, "Param/Brightness", imageinfo->Brightness);
                }

                if(imageinfo->UseMask & 0x2)
                {
                    Common_Json_SetAttrValueInt(tmp, "Param/Contrast", imageinfo->Contrast);
                }

                if(imageinfo->UseMask & 0x4)
                {
                    Common_Json_SetAttrValueInt(tmp, "Param/Saturation", imageinfo->Staturation);
                }

                if(imageinfo->UseMask & 0x8)
                {
                    Common_Json_SetAttrValueInt(tmp, "Param/Hue", imageinfo->Hue);
                }
            }

            if(imageinfo->UseMask & 0x10)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i++);
                Common_Json_SetAttrValueInt(tmp, "Type", 4);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                Common_Json_SetAttrValueInt(tmp,"Param/Enable",imageinfo->Sharpness?1:0);
                Common_Json_SetAttrValueInt(tmp,"Param/Level",imageinfo->Sharpness);
            }

            if(imageinfo->UseMask & 0x20)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i++);
                Common_Json_SetAttrValueInt(tmp, "Type", 10);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                Common_Json_SetAttrValueInt(tmp,"Param/Mode",imageinfo->Mirror);
            }

            if(imageinfo->UseMask & 0x1c0)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i++);
                Common_Json_SetAttrValueInt(tmp, "Type", 6);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                if(imageinfo->UseMask & 0x40)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/Enable",imageinfo->WdrEnable);
                }

                if(imageinfo->UseMask & 0x80)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/Level",imageinfo->WdrLevel);
                }

                if(imageinfo->UseMask & 0x100)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/Mode",imageinfo->WdrMode);
                }
            }

            if(imageinfo->UseMask & 0x7e00)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i++);
                Common_Json_SetAttrValueInt(tmp, "Type", 0);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                if(imageinfo->UseMask & 0x200)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/Mode",imageinfo->DayNightMode);
                }

                if(imageinfo->UseMask & 0x400)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/DayToNightThreshold",imageinfo->DayToNightThreshold);
                }

                if(imageinfo->UseMask & 0x800)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/NightToDayThreshold",imageinfo->NightToDayThreshold);
                }
                if(imageinfo->UseMask & 0x1000)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/Delay",imageinfo->DayNightTime);
                }

                if(imageinfo->UseMask & 0x2000)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/DayStart",imageinfo->DayNightStartTime);
                }

                if(imageinfo->UseMask & 0x4000)
                {
                    Common_Json_SetAttrValueInt(tmp,"Param/DayEnd",imageinfo->DayNightEndTime);
                }
            }
        }
    }

    if (0 == ret)
    {
        /*if (g_ovfs_web->devInfo.bPTZ == 1)
        {
            snprintf(buf,sizeof(buf),"/Ptz/Image/Attribute/All");
        }
        else*/
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Image/Attribute/All");
        }
        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetOsdTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Osd/Time/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_OSD_TIME);
            *pOutBuffer = (THIRD_OSD_TIME *)Common_Calloc(1,sizeof(THIRD_OSD_TIME),__FUNCTION__,__LINE__);
            THIRD_OSD_TIME *osdinfo = NULL;
            osdinfo = (THIRD_OSD_TIME *)(*pOutBuffer);
            osdinfo->UseMask = 0xff;

            Common_Json_GetAttrValue(lowerData, 0, "TimeOsdList/Enable", NULL, NULL, &osdinfo->Enable, NULL);

            Common_Json_GetAttrValue(lowerData, 0, "TimeOsdList/X", NULL, NULL, &osdinfo->X, NULL);

            Common_Json_GetAttrValue(lowerData, 0, "TimeOsdList/Y", NULL, NULL, &osdinfo->Y, NULL);

            Common_Json_GetAttrValue(lowerData, 0, "TimeOsdList/Size", NULL, NULL, &osdinfo->FontSize, NULL);

            Common_Json_GetAttrValue(lowerData, 0, "TimeOsdList/Style", NULL, NULL, &osdinfo->DateFormat, NULL);

            Common_Json_GetAttrValue(lowerData, 0, "TimeOsdList/HourStyle", NULL, NULL, &osdinfo->TimeFormat, NULL);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetOsdTime(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    cJSON_Struct *pArry_root = Common_Json_SetAttrValueArr(lowerData, "TimeOsdList");

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_OSD_TIME *osdinfo = NULL;
            osdinfo = (THIRD_OSD_TIME *)pInBuffer;
            if(osdinfo->UseMask & 0xff)
            {
                int i = 0;
                for(i=0; i<2; i++)
                {
                    cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i);
                    Common_Json_SetAttrValueInt(tmp, "Device", 0);
                    Common_Json_SetAttrValueInt(tmp, "Channel", 0);
                    Common_Json_SetAttrValueInt(tmp, "Stream", i);
                    if(osdinfo->UseMask & 0x1)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Enable", osdinfo->Enable);
                    }

                    if(osdinfo->UseMask & 0x2)
                    {
                        Common_Json_SetAttrValueInt(tmp, "X", osdinfo->X);
                        Common_Json_SetAttrValueInt(tmp, "Location", 0);
                    }

                    if(osdinfo->UseMask & 0x4)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Y", osdinfo->Y);
                        Common_Json_SetAttrValueInt(tmp, "Location", 0);
                    }

                    if(osdinfo->UseMask & 0x8 && i== 0)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Size", osdinfo->FontSize);
                    }

                    if(osdinfo->UseMask & 0x10)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Style", osdinfo->DateFormat);
                    }

                    if(osdinfo->UseMask & 0x20)
                    {
                        Common_Json_SetAttrValueInt(tmp, "HourStyle", osdinfo->TimeFormat);
                    }
                }

            }

        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Osd/Time/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetOsdName(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType, int osdType)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    char buf[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *list = NULL;
    if (0 == ret)
    {
        if(osdType == 1)
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/All");
        }
        else
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/All");
        }

        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_OSD_NAME);
            *pOutBuffer = (THIRD_OSD_NAME *)Common_Calloc(1,sizeof(THIRD_OSD_NAME),__FUNCTION__,__LINE__);
            THIRD_OSD_NAME *osdinfo = NULL;
            osdinfo = (THIRD_OSD_NAME *)(*pOutBuffer);
            osdinfo->UseMask = 0xff;

            list = Common_Json_GetAttrValueArr(lowerData, osdType == 1?"MulOsdList":"NameOsdList");

            Common_Json_GetAttrValue(list, 0, "Enable", NULL, NULL, &osdinfo->Enable, NULL);

            Common_Json_GetAttrValue(list, 0, "X", NULL, NULL, &osdinfo->X, NULL);

            Common_Json_GetAttrValue(list, 0, "Y", NULL, NULL, &osdinfo->Y, NULL);

            Common_Json_GetAttrValue(list, 0, "UseRemoteBm", NULL, NULL, &i_num, NULL);

            if(i_num == 1)
            {
                Common_Json_GetAttrValue(list, 0, "BitMapSize", NULL, NULL, &osdinfo->FontSize, NULL);
            }
            else
            {
                Common_Json_GetAttrValue(list, 0, "Size", NULL, NULL, &osdinfo->FontSize, NULL);
            }

            Common_Json_GetAttrValue(list, 0, "String", NULL, &str_tmp, NULL, NULL);
            snprintf(osdinfo->Name, sizeof(osdinfo->Name), "%s", str_tmp);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetOsdName(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType, int osdType)
{
    int ret = 0;
    char buf[128] = {0};

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    cJSON_Struct *pArry_root = Common_Json_SetAttrValueArr(lowerData, osdType == 1?"MulOsdList":"NameOsdList");

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_OSD_NAME *osdinfo = NULL;
            osdinfo = (THIRD_OSD_NAME *)pInBuffer;
            if(osdinfo->UseMask & 0xff)
            {
                int i = 0;
                for(i=0; i<2; i++)
                {
                    cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArry_root, i);
                    Common_Json_SetAttrValueInt(tmp, "Device", 0);
                    Common_Json_SetAttrValueInt(tmp, "Channel", 0);
                    Common_Json_SetAttrValueInt(tmp, "Stream", i);
                    if(osdinfo->UseMask & 0x1)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Enable", osdinfo->Enable);
                    }

                    if(osdinfo->UseMask & 0x2)
                    {
                        Common_Json_SetAttrValueInt(tmp, "X", osdinfo->X);
                        Common_Json_SetAttrValueInt(tmp, "Location", 0);
                    }

                    if(osdinfo->UseMask & 0x4)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Y", osdinfo->Y);
                        Common_Json_SetAttrValueInt(tmp, "Location", 0);
                    }

                    if(osdinfo->UseMask & 0x8)
                    {
                        Common_Json_SetAttrValueStr(tmp, "String", osdinfo->Name);
                    }

                    if(osdinfo->UseMask & 0x10 && i== 0)
                    {
                        Common_Json_SetAttrValueInt(tmp, "Size", osdinfo->FontSize);
                        Common_Json_SetAttrValueInt(tmp, "UseRemoteBm", 0);
                    }
                }

            }

        }
    }

    if (0 == ret)
    {
        if(osdType == 1)
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/All");
        }
        else
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/All");
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int ForceIFrame(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    int streamNo = 0;
    char uri_path[128] = {0};

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_FORCE_IFRAME *forceiframe = NULL;
            forceiframe = (THIRD_FORCE_IFRAME *)pInBuffer;

            if(forceiframe->UseMask & 0x1)
            {
                streamNo = forceiframe->Stream;
            }
        }
    }

    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/ReqIFrame?Device=0&Channel=0&Stream=%d",streamNo);
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

int GetHttpInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
    if(nOutDataType == 1)
    {
        *nOutBufSize = sizeof(THIRD_HTTPPORT);
        *pOutBuffer = (THIRD_HTTPPORT *)Common_Calloc(1,sizeof(THIRD_HTTPPORT),__FUNCTION__,__LINE__);
        THIRD_HTTPPORT *httpinfo = NULL;
        httpinfo = (THIRD_HTTPPORT *)(*pOutBuffer);
        httpinfo->UseMask = 0xf;

        httpinfo->HttpPort = g_ovfs_web->httpport;
        httpinfo->HttpsPort = g_ovfs_web->httpsport;

    }

    return ret;
}

int SetHttpInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)//0-json,1-struct
{
    int ret = 0;
    if(nOutDataType == 1)
    {
        THIRD_HTTPPORT *httpinfo = NULL;
        httpinfo = (THIRD_HTTPPORT *)pInBuffer;
        if(httpinfo->UseMask & 0xff)
        {
            if(httpinfo->UseMask & 0x1)
            {
                g_ovfs_web->httpport = httpinfo->HttpPort;
            }

            if(httpinfo->UseMask & 0x2)
            {
                g_ovfs_web->httpsport = httpinfo->HttpsPort;
            }

            web_stop_nginx();
            generate_default_ngx_conf();
            web_change_web_port(g_ovfs_web->httpport, g_ovfs_web->httpsport);
            Access_SaveConfig(g_AccessHandle, g_ovfs_config);

        }
    }

    return ret;
}

int GetRtspInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_RTSPPORT);
            *pOutBuffer = (THIRD_RTSPPORT *)Common_Calloc(1,sizeof(THIRD_RTSPPORT),__FUNCTION__,__LINE__);
            THIRD_RTSPPORT *rtspinfo = NULL;
            rtspinfo = (THIRD_RTSPPORT *)(*pOutBuffer);
            rtspinfo->UseMask = 0xff;

            Common_Json_GetAttrValueInt(lowerData, "Rtsp/Enable", &rtspinfo->Enable);
            Common_Json_GetAttrValueInt(lowerData, "Rtsp/RtspPort", &i_num);
            rtspinfo->RtspPort = (short)i_num;
            Common_Json_GetAttrValueInt(lowerData, "Rtsp/MultiCast/Enable", &rtspinfo->MulcastEnable);
            Common_Json_GetAttrValueStr(lowerData, "Rtsp/MultiCast/MainVideo/IP", &str_tmp);
            snprintf(rtspinfo->MainVideo.Ip, sizeof(rtspinfo->MainVideo.Ip), "%s", str_tmp);
            Common_Json_GetAttrValueInt(lowerData, "Rtsp/MultiCast/MainVideo/Port", &rtspinfo->MainVideo.Port);
            Common_Json_GetAttrValueInt(lowerData, "Rtsp/MultiCast/MainVideoTTL", &rtspinfo->MainVideo.TTL);

            Common_Json_GetAttrValueStr(lowerData, "Rtsp/MultiCast/SubVideo/IP", &str_tmp);
            snprintf(rtspinfo->SubVideo.Ip, sizeof(rtspinfo->SubVideo.Ip), "%s", str_tmp);
            Common_Json_GetAttrValueInt(lowerData, "Rtsp/MultiCast/SubVideo/Port", &rtspinfo->SubVideo.Port);
            Common_Json_GetAttrValueInt(lowerData, "Rtsp/MultiCast/SubVideo/TTL", &rtspinfo->SubVideo.TTL);

        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetRtspInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        Common_Json_SetAttrValueObj(lowerData, "Rtsp");

        if(nOutDataType == 1)
        {
            THIRD_RTSPPORT *rtspinfo = NULL;
            rtspinfo = (THIRD_RTSPPORT *)pInBuffer;

            if(rtspinfo->UseMask & 0x1)
            {
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/Enable", rtspinfo->Enable);
            }

            if(rtspinfo->UseMask & 0x2)
            {
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/RtspPort", (int)rtspinfo->RtspPort);
            }

            Common_Json_SetAttrValueObj(lowerData, "Rtsp/MultiCast");

            if(rtspinfo->UseMask & 0x8)
            {
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/MultiCast/Enable", rtspinfo->MulcastEnable);
            }

            if(rtspinfo->UseMask & 0x10)
            {
                Common_Json_SetAttrValueStr(lowerData, "Rtsp/MultiCast/MainVideo/IP", rtspinfo->MainVideo.Ip);
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/MultiCast/MainVideo/Port", rtspinfo->MainVideo.Port);
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/MultiCast/MainVideoTTL", rtspinfo->MainVideo.TTL);
            }

            if(rtspinfo->UseMask & 0x20)
            {
                Common_Json_SetAttrValueStr(lowerData, "Rtsp/MultiCast/SubVideo/IP", rtspinfo->MainVideo.Ip);
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/MultiCast/SubVideo/Port", rtspinfo->MainVideo.Port);
                Common_Json_SetAttrValueInt(lowerData, "Rtsp/MultiCast/SubVideo", rtspinfo->MainVideo.TTL);
            }
        }
    }

    if (0 == ret)
    {
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

int GetNetInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    char *str_tmp = NULL;
    char defaultRoute[8] = {0};
    char netUrl[32] = {0};

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/DefaultRoute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueStr(lowerData, "DefaultRoute",&str_tmp))
        {
            snprintf(defaultRoute,sizeof(defaultRoute),"%s",str_tmp);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
       // if(smatch(defaultRoute, "eth0"))
       // {
            snprintf(netUrl,sizeof(netUrl),"/NetWork/NetAttr/eth/0");
        /*}
        else*/ if(smatch(defaultRoute, "wlan0"))
        {
            snprintf(netUrl,sizeof(netUrl),"/NetWork/NetAttr/Wlan/0");
        }

        Ovfs_Web_UpdateHeader(header, REST_GET, netUrl);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_ETHCFG);
            *pOutBuffer = (THIRD_ETHCFG *)Common_Calloc(1,sizeof(THIRD_ETHCFG),__FUNCTION__,__LINE__);
            THIRD_ETHCFG *netinfo = NULL;
            netinfo = (THIRD_ETHCFG *)(*pOutBuffer);
            netinfo->UseMask = 0xff;

            Common_Json_GetAttrValueInt(lowerData, "EnableDhcp", &netinfo->Dhcp);
            Common_Json_GetAttrValueStr(lowerData, "IpAddrV4", &str_tmp);
            snprintf(netinfo->Ip, sizeof(netinfo->Ip), "%s", str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "IpMaskV4", &str_tmp);
            snprintf(netinfo->Mask, sizeof(netinfo->Mask), "%s", str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "GatewayV4", &str_tmp);
            snprintf(netinfo->Gateway, sizeof(netinfo->Gateway), "%s", str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "MacAddr", &str_tmp);
            snprintf(netinfo->Mac, sizeof(netinfo->Mac), "%s", str_tmp);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetNetInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/NetWork/NetAttr/DefaultRoute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueStr(lowerData, "DefaultRoute",&str_tmp))
        {
            if(smatch(str_tmp, "wlan0"))
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
                return 0;
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {

        if(nOutDataType == 1)
        {
            THIRD_ETHCFG *netinfo = NULL;
            netinfo = (THIRD_ETHCFG *)pInBuffer;

            Common_Json_SetAttrValueStr(lowerData, "EthName", "eth0");

            if(netinfo->UseMask & 0x1)
            {
                Common_Json_SetAttrValueInt(lowerData, "EnableDhcp", netinfo->Dhcp);
            }

            if(netinfo->UseMask & 0x2)
            {
                Common_Json_SetAttrValueStr(lowerData, "IpAddrV4", netinfo->Ip);
            }

            if(netinfo->UseMask & 0x4)
            {
                Common_Json_SetAttrValueStr(lowerData, "IpMaskV4", netinfo->Mask);
            }

            if(netinfo->UseMask & 0x8)
            {
                Common_Json_SetAttrValueStr(lowerData, "GatewayV4", netinfo->Gateway);
            }

        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetAttr/Eth/0");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetDnsInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetAttr/DNS");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_DNSCFG);
            *pOutBuffer = (THIRD_DNSCFG *)Common_Calloc(1,sizeof(THIRD_DNSCFG),__FUNCTION__,__LINE__);
            THIRD_DNSCFG *dnsinfo = NULL;
            dnsinfo = (THIRD_DNSCFG *)(*pOutBuffer);
            dnsinfo->UseMask = 0xf;

            Common_Json_GetAttrValueStr(lowerData, "Dns1V4", &str_tmp);
            snprintf(dnsinfo->Dns1, sizeof(dnsinfo->Dns1), "%s", str_tmp);

            str_tmp = NULL;
            Common_Json_GetAttrValueStr(lowerData, "Dns2V4", &str_tmp);
            snprintf(dnsinfo->Dns2, sizeof(dnsinfo->Dns2), "%s", str_tmp);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetDnsInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    //char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_DNSCFG *dnsinfo = NULL;
            dnsinfo = (THIRD_DNSCFG *)pInBuffer;

            if(dnsinfo->UseMask & 0x1)
            {
                Common_Json_SetAttrValueStr(lowerData, "Dns1V4", dnsinfo->Dns1);
            }

            if(dnsinfo->UseMask & 0x2)
            {
                Common_Json_SetAttrValueStr(lowerData, "Dns2V4", dnsinfo->Dns2);
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetAttr/DNS");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetVideoEncodeAbility(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
	char *str_tmp = NULL;
    char buf[128] = {0};
    cJSON_Struct *lowerData = NULL;

    THIRD_VIDEO_ENC_ABILITY *inbuf = (THIRD_VIDEO_ENC_ABILITY *)pInBuffer;

    snprintf(buf,sizeof(buf),"/BoardSys/Video/Ability/Venc/Device0/Channel0/Stream%d",inbuf->Stream);

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
        	cJSON_Struct *tmp = NULL;
			THIRD_VIDEO_ENC_ABILITY *pVideoEncAbility = NULL;
			pVideoEncAbility = (THIRD_VIDEO_ENC_ABILITY *)Common_Calloc(1,sizeof(THIRD_VIDEO_ENC_ABILITY),__FUNCTION__,__LINE__);

            tmp = Common_Json_GetAttrValueArr(lowerData, "Resolution");
			pVideoEncAbility->ResolutionCount = Common_Json_ArraySize(tmp);
			pVideoEncAbility->pResolutionAbility = (THIRD_RESOLUTION_ABILITY *)Common_Calloc(pVideoEncAbility->ResolutionCount,sizeof(THIRD_RESOLUTION_ABILITY),__FUNCTION__,__LINE__);
            for(int i = 0; i < pVideoEncAbility->ResolutionCount; i++)
            {
                Common_Json_GetAttrValue(tmp, i, "W", NULL, NULL, &(pVideoEncAbility->pResolutionAbility[i].Width), NULL);
                Common_Json_GetAttrValue(tmp, i, "H", NULL, NULL, &(pVideoEncAbility->pResolutionAbility[i].Height), NULL);
                Common_Json_GetAttrValue(tmp, i, "Fps", NULL, NULL, &(pVideoEncAbility->pResolutionAbility[i].Fps), NULL);
				str_tmp = NULL;
                Common_Json_GetAttrValue(tmp, i, "ResoStr", NULL, &str_tmp, NULL, NULL);
                snprintf(pVideoEncAbility->pResolutionAbility[i].ResoStr,sizeof(pVideoEncAbility->pResolutionAbility[i].ResoStr),"%s",str_tmp);
            }

            tmp = Common_Json_GetAttrValueArr(lowerData, "EncType");
            pVideoEncAbility->EncTypeCount = Common_Json_ArraySize(tmp);
			pVideoEncAbility->EncTypeAbility = (char (*)[16])Common_Calloc(pVideoEncAbility->EncTypeCount,sizeof(char)*16,__FUNCTION__,__LINE__);
            for(int i = 0; i < pVideoEncAbility->EncTypeCount; i++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(tmp, i, NULL, NULL, &str_tmp, NULL, NULL);
				snprintf(pVideoEncAbility->EncTypeAbility[i], sizeof(pVideoEncAbility->EncTypeAbility[i]), "%s", str_tmp);
            }

            tmp = Common_Json_GetAttrValueArr(lowerData, "BitrateType");
            pVideoEncAbility->BitrateTypeCount = Common_Json_ArraySize(tmp);
			pVideoEncAbility->BitrateTypeAbility = (char (*)[16])Common_Calloc(pVideoEncAbility->BitrateTypeCount,sizeof(char)*16,__FUNCTION__,__LINE__);
            for(int i = 0; i < pVideoEncAbility->BitrateTypeCount; i++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(tmp, i, NULL, NULL, &str_tmp, NULL, NULL);
                snprintf(pVideoEncAbility->BitrateTypeAbility[i],sizeof(pVideoEncAbility->BitrateTypeAbility[i]),"%s",str_tmp);
            }

            tmp = Common_Json_GetAttrValueArr(lowerData, "Profiles");
            pVideoEncAbility->ProfilesCount = Common_Json_ArraySize(tmp);
           	pVideoEncAbility->ProfilesAbility = (char (*)[16])Common_Calloc(pVideoEncAbility->ProfilesCount,sizeof(char)*16,__FUNCTION__,__LINE__);
            for(int i = 0; i< pVideoEncAbility->ProfilesCount; i++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(tmp, i, NULL, NULL, &str_tmp, NULL, NULL);
                snprintf(pVideoEncAbility->ProfilesAbility[i],sizeof(pVideoEncAbility->ProfilesAbility[i]),"%s",str_tmp);
            }

            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, 0, "FpsRange", NULL, &str_tmp, NULL, NULL);
            sscanf(str_tmp, "%d-%d", &(pVideoEncAbility->FpsRange[0]), &(pVideoEncAbility->FpsRange[1]));

            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, 0, "BitrateRange", NULL, &str_tmp, NULL, NULL);
            sscanf(str_tmp, "%d-%d", &(pVideoEncAbility->BitrateRange[0]), &(pVideoEncAbility->BitrateRange[1]));

            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, 0, "Iinterval", NULL, &str_tmp, NULL, NULL);
            sscanf(str_tmp, "%d-%d", &(pVideoEncAbility->Iinterval[0]), &(pVideoEncAbility->Iinterval[1]));
            if(pVideoEncAbility->Iinterval[1]>50)pVideoEncAbility->Iinterval[1]=50;

            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, 0, "EncQuality", NULL, &str_tmp, NULL, NULL);
            sscanf(str_tmp, "%d-%d", &(pVideoEncAbility->EncQuality[0]), &(pVideoEncAbility->EncQuality[1]));

			*pOutBuffer = pVideoEncAbility;
            *nOutBufSize = sizeof(THIRD_VIDEO_ENC_ABILITY);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int cur_fps[2] = {0};

int GetVideoEncodeInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    int need_set = 0;
    char buf[128] = {0};
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    THIRD_VIDEO_ENC *inbuf = (THIRD_VIDEO_ENC *)pInBuffer;

    snprintf(buf,sizeof(buf),"/BoardSys/Video/Attribute/Device0/Channel0/Stream%d",inbuf->Stream);

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_VIDEO_ENC);
            *pOutBuffer = (THIRD_VIDEO_ENC *)Common_Calloc(1,sizeof(THIRD_VIDEO_ENC),__FUNCTION__,__LINE__);
            THIRD_VIDEO_ENC *videoencinfo = NULL;
            videoencinfo = (THIRD_VIDEO_ENC *)(*pOutBuffer);
            videoencinfo->Stream = inbuf->Stream;
            videoencinfo->UseMask = 0xfff;

            Common_Json_GetAttrValueInt(lowerData, "Width", &videoencinfo->Width);
            Common_Json_GetAttrValueInt(lowerData, "Height", &videoencinfo->Height);
            Common_Json_GetAttrValueInt(lowerData, "Quality", &videoencinfo->Quality);
            Common_Json_GetAttrValueInt(lowerData, "Fps", &videoencinfo->Fps);
            cur_fps[inbuf->Stream] = videoencinfo->Fps;
            LOGW("cur_fps[%d]:[%d]\n",inbuf->Stream,cur_fps[inbuf->Stream]);
            //注释掉, hk需要全帧率, onvif内部自己查能力集

            if(videoencinfo->Fps == 0)
            {
                cJSON_Struct *outTmp = NULL;
                memset(buf,0,sizeof(buf));
                snprintf(buf,sizeof(buf),"/BoardSys/Video/Ability/Venc/Device0/Channel0/Stream%d",inbuf->Stream);

                Ovfs_Web_UpdateHeader(header, REST_GET, buf);
                Ovfs_Web_RestMethodA(header, NULL, &outTmp, 0);
                if(outTmp)
                {
                    cJSON_Struct *list = Common_Json_GetAttrValueArr(outTmp, "Resolution");
                    int size = Common_Json_ArraySize(list);
                    int i = 0;
                    for(i=0; i<size; i++)
                    {
                        int w = 0;
                        int h = 0;
                        int fps = 0;
                        Common_Json_GetAttrValue(list, i, "W", NULL, NULL, &w, NULL);
                        Common_Json_GetAttrValue(list, i, "H", NULL, NULL, &h, NULL);
                        Common_Json_GetAttrValue(list, i, "Fps", NULL, NULL, &fps, NULL);
                        if(videoencinfo->Width == w && videoencinfo->Height == h)
                        {
                            cur_fps[inbuf->Stream] = fps;
                            break;
                        }
                    }

                    Common_Json_Delete(outTmp);
                    outTmp = NULL;
                }
            }

            Common_Json_GetAttrValueInt(lowerData, "Iinterval", &videoencinfo->Iinterval);
            LOGW("Iinterval[%d] fps[%d]\n",videoencinfo->Iinterval,cur_fps[inbuf->Stream]);
            if(videoencinfo->Iinterval>cur_fps[inbuf->Stream]*2)
            {
                need_set = 1;
                videoencinfo->Iinterval = cur_fps[inbuf->Stream]*2;
                LOGW("Iinterval[%d]\n",videoencinfo->Iinterval);
                Common_Json_SetAttrValueInt(lowerData, "Iinterval", videoencinfo->Iinterval);
            }
            Common_Json_GetAttrValueInt(lowerData, "BitrateCtrlMode", &videoencinfo->BitrateCtrlMode);
            Common_Json_GetAttrValueInt(lowerData, "EncodeFormat", &videoencinfo->EncodeFormat);
            if(videoencinfo->EncodeFormat>1)
            {
                need_set = 1;
                videoencinfo->EncodeFormat = 1;
                Common_Json_SetAttrValueInt(lowerData, "EncodeFormat", videoencinfo->EncodeFormat);
            }
            Common_Json_GetAttrValueInt(lowerData, "Profiles", &videoencinfo->Profiles);
            Common_Json_GetAttrValueInt(lowerData, "Bitrate", &videoencinfo->Bitrate);
            Common_Json_GetAttrValueInt(lowerData, "BitrateIsCustom", &videoencinfo->BitrateIsCustom);

            Common_Json_GetAttrValueStr(lowerData, "ResoStr", &str_tmp);
            snprintf(videoencinfo->ResolutionStr,sizeof(videoencinfo->ResolutionStr),"%s",str_tmp);
        }

        if(need_set)
        {
            LOGW("need_set\n");
            snprintf(buf,sizeof(buf),"/BoardSys/Video/Attribute/Device0/Channel0/Stream%d",inbuf->Stream);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
            Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetVideoEncodeInfo(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    int streamNo = 0;
    int need_get = 1;
    char buf[128] = {0};

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_VIDEO_ENC *viceoencinfo = NULL;
            viceoencinfo = (THIRD_VIDEO_ENC *)pInBuffer;

            streamNo = viceoencinfo->Stream;

            if(viceoencinfo->UseMask & 0x1)
            {
                Common_Json_SetAttrValueInt(lowerData, "Width", viceoencinfo->Width);
            }

            if(viceoencinfo->UseMask & 0x2)
            {
                Common_Json_SetAttrValueInt(lowerData, "Height", viceoencinfo->Height);
            }

            if(viceoencinfo->UseMask & 0x4)
            {
                Common_Json_SetAttrValueInt(lowerData, "Quality", viceoencinfo->Quality);
            }

            if(viceoencinfo->UseMask & 0x8)
            {
                Common_Json_SetAttrValueInt(lowerData, "Fps", viceoencinfo->Fps);
                LOGW("fps[%d]\n",viceoencinfo->Fps);
                cur_fps[streamNo] = viceoencinfo->Fps;
                need_get = 0;
            }

            if(viceoencinfo->UseMask & 0x10)
            {
                if(need_get)
                {
                    int outsize_tmp = 0;
                    void *outbuf_tmp = NULL;
                    THIRD_VIDEO_ENC inbuf_tmp = {0};
                    inbuf_tmp.Cmd = THIRD_CMD_GET_VIDEO_VENC;
                    inbuf_tmp.Stream = streamNo;
                    GetVideoEncodeInfo(header, &inbuf_tmp,sizeof(inbuf_tmp),&outbuf_tmp,&outsize_tmp, 1);
                    Common_cJSON_free(outbuf_tmp);
                }

                int inter = viceoencinfo->Iinterval;
                LOGW("Iinterval[%d] fps[%d]\n",inter,cur_fps[streamNo]);
                Common_Json_SetAttrValueInt(lowerData, "Iinterval", inter>cur_fps[streamNo]*2?cur_fps[streamNo]*2:inter);
            }

            if(viceoencinfo->UseMask & 0x20)
            {
                Common_Json_SetAttrValueInt(lowerData, "BitrateCtrlMode", viceoencinfo->BitrateCtrlMode);
            }

            if(viceoencinfo->UseMask & 0x40)
            {
                int enc = viceoencinfo->EncodeFormat;
                Common_Json_SetAttrValueInt(lowerData, "EncodeFormat", enc>1?1:enc);
            }

            if(viceoencinfo->UseMask & 0x80)
            {
                Common_Json_SetAttrValueInt(lowerData, "Profiles", viceoencinfo->Profiles);
            }


            if(viceoencinfo->UseMask & 0x100)
            {
                Common_Json_SetAttrValueInt(lowerData, "Bitrate", viceoencinfo->Bitrate);
            }


            if(viceoencinfo->UseMask & 0x200)
            {
                Common_Json_SetAttrValueInt(lowerData, "BitrateIsCustom", viceoencinfo->BitrateIsCustom);
            }

            if(viceoencinfo->UseMask & 0x400)
            {
                Common_Json_SetAttrValueStr(lowerData, "ResoStr", viceoencinfo->ResolutionStr);
            }
        }
    }

    if (0 == ret)
    {
        snprintf(buf,sizeof(buf),"/BoardSys/Video/Attribute/Device0/Channel0/Stream%d",streamNo);
        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetAudioCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    //char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Audio/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_AUDIO_VENC);
            *pOutBuffer = (THIRD_AUDIO_VENC *)Common_Calloc(1,sizeof(THIRD_AUDIO_VENC),__FUNCTION__,__LINE__);
            THIRD_AUDIO_VENC *audiocfg = NULL;
            audiocfg = (THIRD_AUDIO_VENC *)(*pOutBuffer);
            audiocfg->UseMask = 0xff;

            Common_Json_GetAttrValueInt(lowerData, "AudioEnable", &audiocfg->AudioEnable);
            Common_Json_GetAttrValueInt(lowerData, "AudioSource", &audiocfg->AudioSource);
            Common_Json_GetAttrValueInt(lowerData, "InputVol", &audiocfg->InputVol);
            Common_Json_GetAttrValueInt(lowerData, "OutputVol", &audiocfg->OutputVol);
            Common_Json_GetAttrValueInt(lowerData, "EncFormat", &audiocfg->EncFormat);
            Common_Json_GetAttrValueInt(lowerData, "FrameLen", &audiocfg->FrameLen);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetAudioCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    //char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_AUDIO_VENC *audiocfg = NULL;
            audiocfg = (THIRD_AUDIO_VENC *)pInBuffer;
            if(audiocfg->UseMask & 0x1)
            {
                Common_Json_SetAttrValueInt(lowerData, "AudioEnable", audiocfg->AudioEnable);
            }

            if(audiocfg->UseMask & 0x2)
            {
                Common_Json_SetAttrValueInt(lowerData, "AudioSource", audiocfg->AudioSource);
            }

            if(audiocfg->UseMask & 0x4)
            {
                Common_Json_SetAttrValueInt(lowerData, "InputVol", audiocfg->InputVol);
            }

            if(audiocfg->UseMask & 0x8)
            {
                Common_Json_SetAttrValueInt(lowerData, "OutputVol", audiocfg->OutputVol);
            }

            if(audiocfg->UseMask & 0x10)
            {
                Common_Json_SetAttrValueInt(lowerData, "EncFormat", audiocfg->EncFormat);
            }

            if(audiocfg->UseMask & 0x20)
            {
                Common_Json_SetAttrValueInt(lowerData, "FrameLen", audiocfg->FrameLen);
            }

        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Audio/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetDeviceName(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    //char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_DEVICE_NAME *devinfo = NULL;
            devinfo = (THIRD_DEVICE_NAME *)pInBuffer;

            if(devinfo->UseMask & 0x1)
            {
                Common_Json_SetAttrValueStr(lowerData, "DeviceName", devinfo->DeviceName);
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/DeviceName");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetVideoSource(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    int i_num = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/VideoInput/Attribute/Device0");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_RESOLUTION_ABILITY);
            *pOutBuffer = (THIRD_RESOLUTION_ABILITY *)Common_Calloc(1,sizeof(THIRD_RESOLUTION_ABILITY),__FUNCTION__,__LINE__);
            THIRD_RESOLUTION_ABILITY *viinfo = NULL;
            viinfo = (THIRD_RESOLUTION_ABILITY *)(*pOutBuffer);
            //viinfo->UseMask = 0xf;

            Common_Json_GetAttrValueInt(lowerData, "Width", &i_num);
            viinfo->Width = (int)i_num;
            Common_Json_GetAttrValueInt(lowerData, "Height", &i_num);
            viinfo->Height= (int)i_num;
            Common_Json_GetAttrValueInt(lowerData, "Fps", &i_num);
            viinfo->Fps= (int)i_num;
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetVideoSnapshot(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    char buf[128] = {0};
    char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    THIRD_PICTURE_SNAPSHOT *inbuf = (THIRD_PICTURE_SNAPSHOT *)pInBuffer;

    snprintf(buf,sizeof(buf),"/BoardSys/Snap/Device0/Channel0/Stream%d",inbuf->Stream);

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_PICTURE_SNAPSHOT);
            *pOutBuffer = (THIRD_PICTURE_SNAPSHOT *)Common_Calloc(1,sizeof(THIRD_PICTURE_SNAPSHOT),__FUNCTION__,__LINE__);
            THIRD_PICTURE_SNAPSHOT *snapinfo = NULL;
            snapinfo = (THIRD_PICTURE_SNAPSHOT *)(*pOutBuffer);
            snapinfo->UseMask = 0xf;

            Common_Json_GetAttrValueStr(lowerData, "Path", &str_tmp);
            snprintf(snapinfo->PicPath,sizeof(snapinfo->PicPath),"%s",str_tmp);
            Common_Json_GetAttrValueInt(lowerData, "TimeStamp", &snapinfo->TimeStamp);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int GetPreset(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int i = 0;
    int ret = 0;
    int i_num = 0;
    //char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/Preset");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_PRESET_CFG);
            *pOutBuffer = (THIRD_PRESET_CFG *)Common_Calloc(1,sizeof(THIRD_PRESET_CFG),__FUNCTION__,__LINE__);
            THIRD_PRESET_CFG *presetinfo = NULL;
            presetinfo = (THIRD_PRESET_CFG *)(*pOutBuffer);
            presetinfo->UseMask = 0xf;

            cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "Presets");
            presetinfo->PresetNum = Common_Json_ArraySize(list);

            for(i=0; i<presetinfo->PresetNum; i++)
            {
                Common_Json_GetAttrValue(list, i, "Token", NULL, NULL, &i_num, NULL);
                presetinfo->PresetArray[i] = i_num;
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

int OperatePreset(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType, int OperateType)
{
    int i = 0;
    int ret = 0;
    //int i_num = 0;
    char uri[128] = {0};

    int index = 0;

    int presetlist[256] = {0};

    cJSON_Struct *lowerData = NULL;

    THIRD_PRESET_CFG *presetcfg = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_PRESET_CFG *presetinfo = NULL;
            presetinfo = (THIRD_PRESET_CFG *)pInBuffer;

            for(i=0; i<presetinfo->PresetNum; i++)
            {
                if(OperateType == 1)//set
                {
                    if(checkPreset(header, presetinfo->PresetArray[i]))
                    {
                        if(presetinfo->PresetArray[i] == -1)
                        {
                            if(presetcfg == NULL)
                            {
                                void *outbuf = NULL;
                                int outsize = 0;
                                GetPreset(header, NULL, 0, &outbuf, &outsize, 1);
                                presetcfg = (THIRD_PRESET_CFG *)outbuf;
                                if(presetcfg)
                                {
                                    for(index=0; index<presetcfg->PresetNum; index++)
                                    {
                                        presetlist[presetcfg->PresetArray[index]] = 1;
                                    }
                                }
                            }

                            for(index=1; index<256; index++)
                            {
                                if(presetlist[index] == 0)
                                {
                                    presetinfo->PresetArray[i] = index;
                                    LOGW("match preset:[%d]\n",index);
                                    break;
                                }
                            }
                        }

                        snprintf(uri, sizeof(uri), "/Ptz/Preset?Token=%d", presetinfo->PresetArray[i]);
                        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
                    }
                }
                else if(OperateType == 2)//delete
                {
                    snprintf(uri, sizeof(uri), "/Ptz/Preset?Token=%d", presetinfo->PresetArray[i]);
                    Ovfs_Web_UpdateHeader(header, REST_DELETE, uri);
                }
                else if(OperateType == 3)//call
                {
                    if(checkPreset(header, presetinfo->PresetArray[i]))
                    {
                        snprintf(uri, sizeof(uri), "/Ptz/Preset?Goto=%d", presetinfo->PresetArray[i]);
                        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
                    }
                }

                if(slen(uri)>0)
                {
                    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
                }
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(presetcfg)
    {
        Common_cJSON_free(presetcfg);
    }

    return ret;
}

int PTZControl(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    //int i = 0;
    int ret = 0;
    //int i_num = 0;
    //char uri[128] = {0};

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_PTZ_CONTROL *ptzcontrol = NULL;
            ptzcontrol = (THIRD_PTZ_CONTROL *)pInBuffer;

            Common_Json_SetAttrValueInt(lowerData, "Type", ptzcontrol->Type);
            Common_Json_SetAttrValueObj(lowerData, "CmdParam");
            Common_Json_SetAttrValueInt(lowerData, "CmdParam/Device", 0);
            Common_Json_SetAttrValueInt(lowerData, "CmdParam/Speed", ptzcontrol->Speed);
            Common_Json_SetAttrValueInt(lowerData, "CmdParam/Stop", ptzcontrol->Speed?0:1);

            if(ptzcontrol->Speed == 0)
            {
                //发两遍停止, 兼容非球机带但是带ptz的设备形态
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Cmd");
                Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

                Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Cmd");
                Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            }
            else
            {
                if ((0 == g_ovfs_web->devInfo.bPTZ)&&(ptzcontrol->Type > 10)&& (ptzcontrol->Type < 17))
                {
                    Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Cmd");
                }
                else
                {
                    Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Cmd");
                }

                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
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

int GetOnvifCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_ONVIF_CFG);
            *pOutBuffer = (THIRD_ONVIF_CFG *)Common_Calloc(1,sizeof(THIRD_ONVIF_CFG),__FUNCTION__,__LINE__);
            THIRD_ONVIF_CFG *onvifcfg = NULL;
            onvifcfg = (THIRD_ONVIF_CFG *)(*pOutBuffer);

            onvifcfg->UseMask = 0xff;
            Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/AuthEnable", &onvifcfg->AuthEnable);
            Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/AdaptiveIp", &onvifcfg->AdaptiveIp);
            Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/Timeout", &onvifcfg->Timeout);
            Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/FixedIp", &onvifcfg->FixedIp);
            Common_Json_GetAttrValueStr(g_ovfs_config, "OnvifCfg/FixedIpAddr", &str_tmp);

            if(str_tmp){
                snprintf(onvifcfg->FixedIpAddr,sizeof(onvifcfg->FixedIpAddr),"%s",str_tmp);
            }

            LOGD("[%d][%d][%d][%d][%d][%s]\n",onvifcfg->UseMask,onvifcfg->AuthEnable,onvifcfg->AdaptiveIp,
    onvifcfg->Timeout,onvifcfg->FixedIp,onvifcfg->FixedIpAddr);
        }
    }

    return ret;
}

int SetOnvifCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    //char *str_tmp = NULL;

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_ONVIF_CFG *onvifcfg = NULL;
            onvifcfg = (THIRD_ONVIF_CFG *)pInBuffer;
LOGD("[%d][%d][%d][%d][%d][%s]\n",onvifcfg->UseMask,onvifcfg->AuthEnable,onvifcfg->AdaptiveIp,
    onvifcfg->Timeout,onvifcfg->FixedIp,onvifcfg->FixedIpAddr);
            if(Common_Json_GetAttrValueObj(g_ovfs_config, "OnvifCfg") == NULL)
            {
                Common_Json_SetAttrValueObj(g_ovfs_config, "OnvifCfg");
            }

            if(onvifcfg->UseMask & 0x1)
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/AuthEnable", onvifcfg->AuthEnable);
            }

            if(onvifcfg->UseMask & 0x2)
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/AdaptiveIp", onvifcfg->AdaptiveIp);
            }

            if(onvifcfg->UseMask & 0x4)
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/Timeout", onvifcfg->Timeout);
            }

            if(onvifcfg->UseMask & 0x8)
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/FixedIp", onvifcfg->FixedIp);
            }


            if(onvifcfg->UseMask & 0x10)
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "OnvifCfg/FixedIpAddr", onvifcfg->FixedIpAddr);
            }

            Access_SaveConfig(g_AccessHandle, g_ovfs_config);
        }
    }

    return ret;
}

int GetZoomCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    cJSON_Struct *inData = NULL;
    cJSON_Struct *lowerData = NULL;

    if ((inData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(inData, "Type", 45);
        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/cmd");
        ret = Ovfs_Web_RestMethodA(header, inData, &lowerData, 0);
        Common_Json_Delete(inData);
        inData = NULL;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            *nOutBufSize = sizeof(THIRD_ZOOM_CFG);
            *pOutBuffer = (THIRD_ZOOM_CFG *)Common_Calloc(1,sizeof(THIRD_AUDIO_VENC),__FUNCTION__,__LINE__);
            THIRD_ZOOM_CFG *zoomcfg = NULL;
            zoomcfg = (THIRD_ZOOM_CFG *)(*pOutBuffer);
            zoomcfg->UseMask = 0xff;

            Common_Json_GetAttrValueInt(lowerData, "integer", &zoomcfg->ZoomInteger);
            Common_Json_GetAttrValueInt(lowerData, "decimal", &zoomcfg->ZoomDecimal);
            Common_Json_GetAttrValueInt(lowerData, "maxmultiple", &zoomcfg->ZoomMax);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int SetZoomCfg(cJSON_Struct *header,void *pInBuffer,int nInBufSize,void **pOutBuffer,int *nOutBufSize,int nOutDataType)
{
    int ret = 0;
    //int i_num = 0;
    //char *str_tmp = NULL;

    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(0 == ret)
    {
        if(nOutDataType == 1)
        {
            THIRD_ZOOM_CFG *zoomcfg = NULL;
            zoomcfg = (THIRD_ZOOM_CFG *)pInBuffer;

            Common_Json_SetAttrValueInt(lowerData, "Type", 44);
            Common_Json_SetAttrValueObj(lowerData, "CmdParam");

            if(zoomcfg->UseMask & 0x1)
            {
                Common_Json_SetAttrValueInt(lowerData, "CmdParam/integer", zoomcfg->ZoomInteger);
            }

            if(zoomcfg->UseMask & 0x2)
            {
                Common_Json_SetAttrValueInt(lowerData, "CmdParam/decimal", zoomcfg->ZoomDecimal);
            }

        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/cmd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int Preset_SetAudioCfg(cJSON_Struct *header, int enable, int audioIn, int audioOut)
{
    int ret = 0;
    int i_num = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;

    /*if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }*/

    snprintf(uri, sizeof(uri), "/BoardSys/Audio/Attribute/All");
    Ovfs_Web_UpdateHeader(header, REST_GET, uri);

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if (0 == ret)
    {
        if(enable != -1)
        {
            Common_Json_SetAttrValueInt(lowerData, "AudioEnable", enable);
        }

        if(audioIn != -1)
        {
            Common_Json_GetAttrValueInt(lowerData, "InputVol", &i_num);
            if(audioIn)
            {
                i_num+=5;
            }
            else
            {
                i_num-=5;
            }

            if(i_num>100)i_num=100;
            if(i_num<0)i_num=0;
            Common_Json_SetAttrValueInt(lowerData, "InputVol", i_num);
        }

        if(audioOut != -1)
        {
            Common_Json_GetAttrValueInt(lowerData, "OutputVol", &i_num);
            if(audioOut)
            {
                i_num+=5;
            }
            else
            {
                i_num-=5;
            }

            if(i_num>100)i_num=100;
            if(i_num<0)i_num=0;
            Common_Json_SetAttrValueInt(lowerData, "OutputVol", i_num);
        }

        //snprintf(uri, sizeof(uri), "/BoardSys/Audio/Attribute/All");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

int Preset_SetLightCfg(cJSON_Struct *header, char *light, int mode, int bright)
{
    int ret = 0;
    int i_num = 0;
    char uri[128] = {0};
    cJSON_Struct *indata = NULL;
    cJSON_Struct *lowerData = NULL;

    if ((indata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        ret = web_semantic_get_lightcfg(header, NULL, indata);
    }

    if (0 == ret)
    {
        if(light)
        {
            Common_Json_SetAttrValueStr(indata, "Mode", light);
        }

        if(mode != -1)
        {
            Common_Json_SetAttrValueInt(indata, "Control", mode?2:1);
        }

        if(bright != -1)
        {
            Common_Json_GetAttrValueInt(indata, "Brightness", &i_num);
            if(bright)
            {
                i_num+=5;
            }
            else
            {
                i_num-=5;
            }

            if(i_num>100)i_num=100;
            if(i_num<0)i_num=0;
            Common_Json_SetAttrValueInt(indata, "Brightness", i_num);
        }

        parse_lightcfg(indata, lowerData);

        snprintf(uri, sizeof(uri), "/BoardSys/Image/Attribute/All");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (indata)
    {
        Common_Json_Delete(indata);
        indata = NULL;
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

int Preset_SetPersonCfg(cJSON_Struct *header, int enable)
{
    int ret = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueObj(lowerData, "Persons");
        Common_Json_SetAttrValueObj(lowerData, "Persons/Device0");
        Common_Json_SetAttrValueObj(lowerData, "Persons/Device0/Channel0");
        Common_Json_SetAttrValueInt(lowerData, "Persons/Device0/Channel0/Enable", enable);

        snprintf(uri, sizeof(uri), "/SmartServer/Attribute/Persons");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int Preset_SetPersonLinkCfg(cJSON_Struct *header, int audio, int light)
{
    int ret = 0;
    int count = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *list = NULL;
    cJSON_Struct *tmp = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        list = Common_Json_SetAttrValueArr(lowerData, "ResList");
        if(audio != -1)
        {
            tmp = Common_Json_SetAttrValueArrObj(list, count++);
            Common_Json_SetAttrValueStr(tmp, "ActionName", "LinkSound");
            Common_Json_SetAttrValueInt(tmp, "Device", 0);
            Common_Json_SetAttrValueInt(tmp, "Channel", 0);
            Common_Json_SetAttrValueInt(tmp, "Enable", audio);
        }

        if(light != -1)
        {
            tmp = Common_Json_SetAttrValueArrObj(list, count++);
            Common_Json_SetAttrValueStr(tmp, "ActionName", "LinkLightAlarm");
            Common_Json_SetAttrValueInt(tmp, "Device", 0);
            Common_Json_SetAttrValueInt(tmp, "Channel", 0);
            Common_Json_SetAttrValueInt(tmp, "Enable", light);
        }

        snprintf(uri, sizeof(uri), "/Alarm/LinkageCfg/DetectPerson");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}


int Preset_SetRetOnVideo(cJSON_Struct *header, int enable)
{
    int ret = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(lowerData, "Enable", enable);

        snprintf(uri, sizeof(uri), "/Boardsys/Osd/Rect");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int Preset_SetVideoEncFormat(cJSON_Struct *header, int format)
{
    int ret = 0;
    int i = 0;
    int size = 0;
    int i_num = 0;
    int streamidx = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *tmpdata = NULL;
    cJSON_Struct *loopdata = NULL;

    snprintf(uri, sizeof(uri), "/BoardSys/Video/Attribute/All");
    Ovfs_Web_UpdateHeader(header, REST_GET, uri);

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if (0 == ret)
    {
        tmpdata = Common_Json_GetAttrValueArr(lowerData, "AttributeList");
        size = Common_Json_ArraySize(tmpdata);

        for(i=0; i<size; i++)
        {
            //loopdata = Common_Json_GetAttrValueArrItem(tmpdata, i);
            Common_Json_SetAttrValue(tmpdata, i, "EncodeFormat", Common_Json_Type_Number, NULL, format, 0);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Boardsys/Video/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

int Preset_SetVideoResolution(cJSON_Struct *header, int stream, int width, int height)
{
    int ret = 0;
    int i = 0;
    int size = 0;
    int i_num = 0;
    int w = 0;
    int h = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *resolution = NULL;
    cJSON_Struct *loopdata = NULL;

    snprintf(uri, sizeof(uri), "/BoardSys/Video/Ability/Venc/Device0/Channel0/Stream%d", stream);
    Ovfs_Web_UpdateHeader(header, REST_GET, uri);
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if (0 == ret)
    {
        resolution = Common_Json_GetAttrValueArr(lowerData, "Resolution");
        size = Common_Json_ArraySize(resolution);

        for(i=0; i<size; i++)
        {
            Common_Json_GetAttrValue(resolution, i, "W", NULL, NULL, &w, NULL);
            Common_Json_GetAttrValue(resolution, i, "H", NULL, NULL, &h, NULL);
            if(w == width)
            {
                if(h == height ||
                    (w == 1920 && h == 1088) ||
                    (w == 2592 && h == 1904))
                {
                    break;
                }
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(i == size)
    {
        return -1;
    }

    if (0 == ret)
    {
        snprintf(uri, sizeof(uri), "/BoardSys/Video/Attribute/Device0/Channel0/Stream%d", stream);
        Ovfs_Web_UpdateHeader(header, REST_GET, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

        if (0 == ret)
        {
            Common_Json_SetAttrValueInt(lowerData, "Width", w);
            Common_Json_SetAttrValueInt(lowerData, "Height", h);
            Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
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

int Preset_SetOnvifCfg(cJSON_Struct *header, int enable)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(lowerData, "adaptiveIp", enable);
        ret = web_semantic_set_onvifpara(header,lowerData,NULL,NULL);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int Preset_SetDhcpCfg(cJSON_Struct *header, int enable)
{
    int ret = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueStr(lowerData, "EthName", "eth0");
        Common_Json_SetAttrValueInt(lowerData, "EnableDhcp", enable);

        snprintf(uri, sizeof(uri), "/Network/NetAttr/Eth/0");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int Preset_SetImageCfg(cJSON_Struct *header, int type, int mode)
{
    int ret = 0;
    char uri[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *list = NULL;
    cJSON_Struct *tmp = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        list = Common_Json_SetAttrValueArr(lowerData, "ImageList");
        tmp = Common_Json_SetAttrValueArrObj(list, 0);
        Common_Json_SetAttrValueInt(tmp, "Device", 0);
        Common_Json_SetAttrValueInt(tmp, "Type", type);
        Common_Json_SetAttrValueObj(tmp, "Param");
        Common_Json_SetAttrValueInt(tmp, "Param/Mode", mode);

        snprintf(uri, sizeof(uri), "/BoardSys/Image/Attribute/All");
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int checkPreset(cJSON_Struct *header, int presetNo)
{
    int ret = 0;
    LOGW("presetNo:[%d]\n",presetNo);
    switch(presetNo)
    {
        case 77://restore all
            web_semantic_factoryrestore(header, NULL, NULL, 0);
            break;

        case 91://audio input open
            Preset_SetAudioCfg(header, 1, -1, -1);
            break;
        case 92://audio input close
            Preset_SetAudioCfg(header, 0, -1, -1);
            break;
        case 93://input volume +5
            Preset_SetAudioCfg(header, -1, 1, -1);
            break;
        case 94://input volume -5
            Preset_SetAudioCfg(header, -1, 0, -1);
            break;
        case 95://output volume +5
            Preset_SetAudioCfg(header, -1, -1, 1);
            break;
        case 96://output volume -5
            Preset_SetAudioCfg(header, -1, -1, 0);
            break;

        case 111://ir_warm
            Preset_SetLightCfg(header, "Ir_Warm", -1, -1);
            break;
        case 112://warm
            Preset_SetLightCfg(header, "Warm", -1, -1);
            break;
        case 113://ir
            Preset_SetLightCfg(header, "Ir", -1, -1);
            break;
        case 114://auto mode
            Preset_SetLightCfg(header, NULL, 1, -1);
            break;
        case 115://manual mode
            Preset_SetLightCfg(header, NULL, 0, -1);
            break;
        case 116://light bright +5
            Preset_SetLightCfg(header, NULL, -1, 1);
            break;
        case 117://light bright -5
            Preset_SetLightCfg(header, NULL, -1, 0);
            break;

        case 131://person enable
            Preset_SetPersonCfg(header, 1);
            break;
        case 132://person disable
            Preset_SetPersonCfg(header, 0);
            break;
        case 133://link audio open
            Preset_SetPersonLinkCfg(header, 1, -1);
            break;
        case 134://link audio close
            Preset_SetPersonLinkCfg(header, 0, -1);
            break;
        case 135://link light open
            Preset_SetPersonLinkCfg(header, -1, 1);
            break;
        case 136://link light close
            Preset_SetPersonLinkCfg(header, -1, 0);
            break;
        case 137://rect on video open
            Preset_SetRetOnVideo(header, 1);
            break;
        case 138://rect on video close
            Preset_SetRetOnVideo(header, 0);
            break;

        case 148://
            Preset_SetVideoResolution(header, 0, 3200, 1800);
            break;
        case 149://
            Preset_SetVideoResolution(header, 0, 3072, 2048);
            break;
        case 150://
            Preset_SetVideoResolution(header, 0, 2592, 1944);
            break;
        case 151://
            Preset_SetVideoResolution(header, 0, 3840, 2160);
            break;
        case 152://
            Preset_SetVideoResolution(header, 0, 2880, 1620);
            break;
        case 153://
            Preset_SetVideoResolution(header, 0, 2560, 1440);
            break;
        case 154://
            Preset_SetVideoResolution(header, 0, 2304, 1296);
            break;
        case 155://
            Preset_SetVideoResolution(header, 0, 1920, 1080);
            break;
        case 156://
            Preset_SetVideoResolution(header, 0, 1280, 720);
            break;
        case 157://
            Preset_SetVideoResolution(header, 1, 720, 576);
            break;
        case 158://
            Preset_SetVideoResolution(header, 1, 864, 480);
            break;
        case 159://
            Preset_SetVideoResolution(header, 1, 640, 480);
            break;
        case 160://
            Preset_SetVideoResolution(header, 1, 640, 360);
            break;
        case 161://
            Preset_SetVideoResolution(header, 1, 352, 288);
            break;

        case 164://set encformat h264
            Preset_SetVideoEncFormat(header, 0);
            break;
        case 165://set encformat h265
            Preset_SetVideoEncFormat(header, 1);
            break;

        case 177://
            web_semantic_devicereboot(header, NULL, NULL);
            break;

        case 180://Onvif AllNet open
            Preset_SetOnvifCfg(header, 1);
            break;
        case 181://Onvif AllNet close
            Preset_SetOnvifCfg(header, 0);
            break;

        case 184://dhcp open
            Preset_SetDhcpCfg(header, 1);
            break;
        case 185://dhcp close
            Preset_SetDhcpCfg(header, 0);
            break;

        case 192://Plate exposure
            Preset_SetImageCfg(header, 28, 2);
            break;
        case 193://person exposure
            Preset_SetImageCfg(header, 28, 1);
            break;
        case 194://no exposure
            Preset_SetImageCfg(header, 28, 0);
            break;
        case 195://dark scene
            Preset_SetImageCfg(header, 25, 0);
            break;
        case 196://Shimmer scene
            Preset_SetImageCfg(header, 25, 1);
            break;
        default:
            ret = 1;
    }
    return ret;
}
