//#include "mxml.h"
#include "ovfs_web_func.h"

Common_cJSON_T *jsonAlarmInfo = NULL;
pthread_t session_tId;
extern uint strtoi(char *s);
extern int BaudRate[15];
extern int web_semantic_get_port_occupancy(int port);
static int web_semantic_login(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iret = 0;

    cJSON_Struct *pResult = NULL;
    int valueInt = 0;
    char *str_tmp = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/UserCfg");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        Common_Json_Delete(pResult);
        pResult = NULL;
        if (ret < 0)
        {
            LOGE("Auth Failed!\n");
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Video/Ability/Number");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(pResult, -1, "ChanTotalNum", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(outdata, -1, "ChanNum", Common_Json_Type_Number, NULL, valueInt, 0);
        }
        else
        {
            ret = WEB_CODE_InternalMistake;
        }

        Common_Json_Delete(pResult);
        pResult = NULL;
    }

#ifndef SIMPLIFIED

    if (0 == ret)
    {

        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/AlarmOut/Ability");
        iret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);

        if (0 == iret)
        {
            Common_Json_GetAttrValue(pResult, -1, "AlaramOutNum", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(outdata, -1, "AlarmOutPortNum", Common_Json_Type_Number, NULL, valueInt, 0);
        }

        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (0 == ret)
    {

        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Event/Ability");
        iret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);

        if (0 == iret)
        {
            Common_Json_GetAttrValue(pResult, -1, "AlarmInNum", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(outdata, -1, "AlarmInPortNum", Common_Json_Type_Number, NULL, valueInt, 0);
        }

        Common_Json_Delete(pResult);
        pResult = NULL;
    }
#endif

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(pResult, -1, "Rtsp.RtspPort", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RTSPPort", Common_Json_Type_Number, NULL, valueInt, 0);
        }
        else
        {
            ret = WEB_CODE_InternalMistake;
        }

        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
        iret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == iret)
        {
            Common_Json_GetAttrValue(pResult, -1, "Rtmp.RtmpPort", NULL, NULL, &valueInt, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RTMPPort", Common_Json_Type_Number, NULL, valueInt, 0);

        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    Common_Json_SetAttrValue(outdata, -1, "DVRType", Common_Json_Type_Number, NULL, 3, 0);
    Common_Json_SetAttrValue(outdata, -1, "HttpPort", Common_Json_Type_Number, NULL, g_ovfs_web->httpport, 0);
    Common_Json_SetAttrValue(outdata, -1, "DVRPort", Common_Json_Type_Number, NULL, 0, 0);

    if (0 == ret)
    {

        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if(ret == 0)
        {
            Common_Json_GetAttrValueStr(pResult, "SerialNumber", &str_tmp);
            Common_Json_SetAttrValueStr(outdata, "SerialNumber", str_tmp?str_tmp:"");
        }
        else
        {
            ret = WEB_CODE_InternalMistake;
        }

        if(pResult)
        {
            Common_Json_Delete(pResult);
            pResult = NULL;
        }
    }

    return ret;
}

static U64 ptz_start = 0;
static U64 ptz_stop = 0;

Common_Thread_T hPtzThread = NULL;
cJSON_Struct *ptzJson = NULL;
void *Fxn_Web_PtzStop()
{
    int interval = 100;
    cJSON_Struct *header = NULL;
    cJSON_Struct * lowerData = NULL;

    while(1)
    {
        Common_Sleep(0, interval*1000);

        U64 runtime = Common_GetSystemCount64();
        if(ptzJson && ptz_stop)
        {
            U64 offset = runtime - ptz_stop;
            //LOGW("offset:[%llu]\n",offset);
            if(offset >= 100)
            {
                //LOGW("ptzStop:[%llu][%llu]\n",runtime,ptz_stop);
                ptz_stop = 0;
                interval = 100;
                header = Common_Json_Duplicate(Common_Json_GetAttrValueObj(ptzJson, "Header"), 1);
                lowerData = Common_Json_Duplicate(Common_Json_GetAttrValueObj(ptzJson, "Data"), 1);
                Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

                if(header)
                {
                    Common_Json_Delete(header);
                    header = NULL;
                }

                if(lowerData)
                {
                    Common_Json_Delete(lowerData);
                    lowerData = NULL;
                }
            }
            else
            {
                interval = 100 - offset;
            }
        }
    }


}

int web_semantic_ptzcontrol(OVFS_WEB_OPTION_S *opt,cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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

    //int device = 0;
    int cmd = 0;
    int stop = 0;
    int speed = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Cmd", &cmd) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            Common_Json_SetAttrValueInt(lowerData, "Type", cmd);
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueObj(lowerData, "CmdParam");
        Common_Json_SetAttrValueInt(lowerData, "CmdParam/Device", opt->dev);

        if (Common_Json_GetAttrValueInt(indata, "IsStop", &stop) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            Common_Json_SetAttrValueInt(lowerData, "CmdParam/Stop", stop ? 1: 0);
        }

        if (Common_Json_GetAttrValueInt(indata, "Speed", &speed) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            Common_Json_SetAttrValueInt(lowerData, "CmdParam/Speed", speed);
        }
    }

    if (0 == ret)
    {
        LOGW("g_ovfs_web->devInfo.bPTZ[%d]  cmd:[%d]\n",g_ovfs_web->devInfo.bPTZ,cmd);
        if ((0 == g_ovfs_web->devInfo.bPTZ)&&(cmd > 10)&& (cmd < 17))
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Cmd");
        }
        else
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Cmd");
        }

        int call = 0;
        U64 runtime = Common_GetSystemCount64();
        if(stop)
        {
            if(runtime - ptz_stop > 100)
            {
                call = 1;
            }
            ptz_stop = runtime;
            if(ptzJson == NULL)
            {
                ptzJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                Common_Json_SetAttrValueObj(ptzJson, "Header");
                Common_Json_SetAttrValueObj(ptzJson, "Data");
            }

            cJSON_Struct *header_tmp = Common_Json_GetAttrValueObj(ptzJson, "Header");
            JsonOper_MergeObj(header_tmp, header, 0);
            cJSON_Struct *data_tmp = Common_Json_GetAttrValueObj(ptzJson, "Data");
            JsonOper_MergeObj(data_tmp, lowerData, 0);

            if(hPtzThread == NULL)
            {
                Common_Thread_Create(&hPtzThread, __FUNCTION__, 0, 0, Fxn_Web_PtzStop, NULL);
            }

        }
        else
        {
            if(runtime - ptz_start > 100)
            {
                ptz_start = runtime;
                call = 1;
            }
            //LOGW("call:[%d] [%llu]\n",call,runtime - ptz_start);
        }

        if(call)
        {
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

static int web_semantic_ptz_preset(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int cmd = -1;
    int PresetIdx = 0;

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Cmd", &cmd) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "Index", &PresetIdx) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    char uri[64]= {0};
    if (0 == ret)
    {
        if (cmd == 8)
        {
            snprintf(uri, sizeof(uri), "/Ptz/Preset?Token=%d", PresetIdx);
        }
        else if (cmd == 9)
        {
            snprintf(uri, sizeof(uri), "/Ptz/Preset?Token=%d", PresetIdx);
        }
        else if (cmd == 39)
        {
            snprintf(uri, sizeof(uri), "/Ptz/Preset?Goto=%d", PresetIdx);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        if(cmd == 9)
            Ovfs_Web_UpdateHeader(header, REST_DELETE, uri);
        else
            Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

#if 0
static int web_semantic_get_ptz_cruise(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int cruisePath = 1;
    Common_Json_GetAttrValueInt(indata, "CruisePath", &cruisePath);

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/Cruises?CruisePath=%d", cruisePath);

        Ovfs_Web_UpdateHeader(header, REST_GET, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    cJSON_Struct *cruise = NULL;
    if (0 == ret)
    {
        if ((cruise = Common_Json_GetAttrValueArr(lowerData, "Cruise")) == NULL)
        {
            ret = WEB_CODE_InternalMistake;
        }
    }

    if (0 == ret)
    {
        int value;
        int cruiseSize = Common_Json_ArraySize(cruise);
        cJSON_Struct *outCruise = Common_Json_SetAttrValueArr(outdata, "Cruise");
        int i;
        for (i = 0; i < cruiseSize; i++)
        {
            cJSON_Struct *cruiseContentEach = Common_Json_GetAttrValueArrItem(cruise, i);
            if (Common_Json_GetAttrValueInt(cruiseContentEach, "PresetNo", &value))
            {
                Common_Json_SetAttrValue(outCruise, i, "PresetNo", Common_Json_Type_Number, NULL, value, 0);
            }
            if (Common_Json_GetAttrValueInt(cruiseContentEach, "Dwell", &value))
            {
                Common_Json_SetAttrValue(outCruise, i, "Dwell", Common_Json_Type_Number, NULL, value, 0);
            }
            if (Common_Json_GetAttrValueInt(cruiseContentEach, "Speed", &value))
            {
                Common_Json_SetAttrValue(outCruise, i, "Speed", Common_Json_Type_Number, NULL, value, 0);
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

static int web_semantic_set_ptz_cruise(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int cruisePath = 0;
    int presetNo = 0;
    int dwell = 0;
    int speed = 0;

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "CruisePath", &cruisePath) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "PresetNo", &presetNo) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "Dwell", &dwell) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "Speed", &speed) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

    }

    char uri[128];
    if (0 == ret)
    {
        snprintf(uri, sizeof(uri), "/Ptz/Cruises?Operate=Add&CruisePath=%d&PresetNo=%d&Speed=%d&Dwell=%d",
                 cruisePath, presetNo, speed, dwell);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_call_ptz_cruise(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    static int cruisePathBak = -1;
    int cruisePath = 0;
    static int is_running = 0;

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "CruisePath", &cruisePath) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    /*if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Run", &run) == NULL)
        {
            run = 1;
        }
    }*/

    if(cruisePathBak != cruisePath)
    {
        is_running = 0;

    }

    char uri[128]= {0};
    if (0 == ret && is_running == 0)
    {
        snprintf(uri, sizeof(uri), "/PTZ/Cruises?Operate=Goto&CruisePath=%d", cruisePath);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        if(ret == 0)
        {
            is_running = 1;
            cruisePathBak = cruisePath;
        }
    }
    else if (0 == ret && is_running == 1)
    {
        snprintf(uri, sizeof(uri), "/PTZ/Cruises?Operate=Stop&CruisePath=%d", cruisePath);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        if(ret == 0)
        {
            is_running = 0;
            cruisePathBak = -1;
        }
    }

    return ret;
}
static int web_semantic_del_ptz_cruise(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int cruisePath = 0;
    int cruiseItemNo = -1;

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "CruisePath", &cruisePath) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "CruiseItemNo", &cruiseItemNo) == NULL)
        {
            //ret = WEB_CODE_InvalidArg;
        }
        else if (cruiseItemNo > 0)
        {
            cruiseItemNo -= 1;
        }
    }

    char uri[128]= {0};
    if (0 == ret)
    {
        if (cruiseItemNo >= 0)
        {
            snprintf(uri, sizeof(uri), "/Ptz/Cruises?CruisePath=%d&CruiseItemNo=%d", cruisePath, cruiseItemNo);
        }
        else
        {
            snprintf(uri, sizeof(uri), "/Ptz/Cruises?CruisePath=%d", cruisePath);
        }

        Ovfs_Web_UpdateHeader(header, REST_DELETE, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_set_ptz_track(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int run = 0;
    int trackIndex = 0;

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Run", &run) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "TrackIndex", &trackIndex) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/Tracks?Operate=%s&TrackPath=%d", run ? "Start" : "Stop", trackIndex);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_call_ptz_track(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int trackIndex = 0;
    static int trackIndexBak = 0;
    static int is_running = 0;



    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "TrackIndex", &trackIndex) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if(trackIndex != trackIndexBak)
    {
        is_running = 0;
    }


    if(is_running == 0)
    {
        if (0 == ret)
        {
            char uri[128] = {0};
            snprintf(uri, sizeof(uri), "/Ptz/Tracks?Operate=Goto&TrackPath=%d", trackIndex);

            Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
            ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        }
        if(ret == 0)
        {
            is_running = 1;
            trackIndexBak = trackIndex;
        }
    }
    else
    {
        //stop
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Stop");
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        if(ret == 0)
        {
            is_running = 0;
            trackIndexBak = -1;
        }

    }



    return ret;
}
/*
static int web_semantic_stop_ptz_track(cJSON_Struct *header,OVFS_WEB_OPTION_S *opt, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        char uri[128]={0};
        snprintf(uri, sizeof(uri), "/Ptz/Stop?DevNo=%d",opt->dev);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }
    return ret;
}
*/
//#if 0//def WITH_PTZ

static int web_semantic_ptz_start_setscan(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int index = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Index", &index) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/TwoPointScanf?SetStart=%d", index);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptz_stop_setscan(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int index = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Index", &index) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/TwoPointScanf?SetStop=%d", index);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptz_call_setscan(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int index = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Index", &index) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/TwoPointScanf?Goto=%d", index);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptz_get_idleoperation(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/IdleOperate");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if (0 == ret)
    {
        int valueInt;
        if (Common_Json_GetAttrValueInt(lowerData, "Action", &valueInt))
        {
            // 底层定义:0-无动作 1-预置点 2-两点扫描 3-巡航 4-轨迹
            // 上层定义:0-无动作 1-预置点 2-巡航 3-轨迹 4-两点扫描
            int lowupMap[] = {0, 1, 4, 2, 3};
            valueInt = MIN2(sizeof(lowupMap)/sizeof(lowupMap[0]), MAX2(0, valueInt));
            Common_Json_SetAttrValueInt(outdata, "Type", lowupMap[valueInt]);
        }
        if (Common_Json_GetAttrValueInt(lowerData, "Idx", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "Index", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerData, "IntervalTime", &valueInt))
        {
            Common_Json_SetAttrValueInt(outdata, "IdleTime", valueInt * 60);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_ptz_set_idleoperation(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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
        int valueInt;
        if (Common_Json_GetAttrValueInt(indata, "Type", &valueInt))
        {
            // 上层定义:0-无动作 1-预置点 2-巡航 3-轨迹 4-两点扫描
            // 底层定义:0-无动作 1-预置点 2-两点扫描 3-巡航 4-轨迹
            int uplowMap[] = {0, 1, 3, 4, 2};
            valueInt = MIN2(sizeof(uplowMap)/sizeof(uplowMap[0]), MAX2(0, valueInt));
            Common_Json_SetAttrValueInt(lowerData, "Action", uplowMap[valueInt]);
        }
        if (Common_Json_GetAttrValueInt(indata, "Index", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "Idx", valueInt);
        }
        if (Common_Json_GetAttrValueInt(indata, "IdleTime", &valueInt))
        {
            Common_Json_SetAttrValueInt(lowerData, "IntervalTime", valueInt / 60);
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/IdleOperate");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_ptz_call_idleoperation(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/IdleOperate?Goto");

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_get_ptzextend_irlightctrl(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/AuxOperate");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if (0 == ret)
    {
        int valueInt;
        if (Common_Json_GetAttrValueInt(lowerData, "IRLamp/Mode", &valueInt) == NULL)
        {
            ret = WEB_CODE_InternalMistake;
        }
        else
        {
            Common_Json_SetAttrValueInt(outdata, "Mode", valueInt);
        }
        if (Common_Json_GetAttrValueInt(lowerData, "IRLamp/Light", &valueInt) == NULL)
        {
            ret = WEB_CODE_InternalMistake;
        }
        else
        {
            Common_Json_SetAttrValueInt(outdata, "Lighting", valueInt);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_ptzextend_irlightctrl(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int mode = 0;
    int lighting = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Mode", &mode) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        if (Common_Json_GetAttrValueInt(indata, "Lighting", &lighting) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/AuxOperate?IRLamp.Mode=%d&IRLamp.Light=%d", mode, lighting);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptzextend_set_3dpositon(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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
        int startX;
        int startY;
        int stopX;
        int stopY;
        if (Common_Json_GetAttrValueInt(indata, "StartX", &startX) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        if (Common_Json_GetAttrValueInt(indata, "StartY", &startY) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        if (Common_Json_GetAttrValueInt(indata, "StopX", &stopX) == NULL)
        {
            stopX = startX;
        }
        if (Common_Json_GetAttrValueInt(indata, "StopY", &stopY) == NULL)
        {
            stopY = startY;
        }
        //int posX = ((startX + stopX) >> 1) * 8192 / 65536 - 4096;
        //int posY = ((startY + stopY) >> 1) * 8192 / 65536 - 4096;
        /*
               int posX = ((startX + stopX) >> 1) * 1000 / 65536;
               int posY = ((startY + stopY) >> 1) * 1000 / 65536;
               int scale = 0;
               if (stopX == startX || stopY == startY)
               {
               }
               else
               {
                   double ratio = (stopX - startX) * (stopY - startY);
                   ratio = 0xffffffffUL / ratio;
                   if (ratio < 0)
                   {
                       ratio = -ratio;
                   }

                   scale = MIN2(16, (int)(ratio-1));
                   if (scale > 0 && stopY < startY)
                   {
                       scale = -scale;
                   }
               }*/
        Common_Json_SetAttrValueInt(lowerData, "Type", 42);
        cJSON_Struct *cmdParam = Common_Json_SetAttrValueObj(lowerData, "CmdParam");
        /*
        			Common_Json_SetAttrValueInt(cmdParam, "X", posX);
         	 		Common_Json_SetAttrValueInt(cmdParam, "Y", posY);
          		Common_Json_SetAttrValueInt(cmdParam, "Scale", scale);*/
        Common_Json_SetAttrValueInt(cmdParam, "startX", startX);
        Common_Json_SetAttrValueInt(cmdParam, "startY", startY);
        Common_Json_SetAttrValueInt(cmdParam, "stopX", stopX);
        Common_Json_SetAttrValueInt(cmdParam, "stopY", stopY);
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/cmd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_ptzextend_get_coverstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/PrivacyMask");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if (0 == ret)
    {
        cJSON_Struct *privacyMasks = Common_Json_GetAttrValueArr(lowerData, "PrivacyMasks");
        cJSON_Struct *coverEnable = NULL;
        if ((coverEnable = Common_Json_SetAttrValue(outdata, -1, "CoverEnable", Common_Json_Type_Array, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            int i;
            int arrSize = Common_Json_ArraySize(privacyMasks);
            for (i = 0; i < arrSize; i++)
            {
                int valueInt = 0;
                Common_Json_GetAttrValue(privacyMasks, i, "Show", NULL, NULL, &valueInt, NULL);
                Common_Json_SetAttrValueArrInt(coverEnable, i, valueInt);
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

static int web_semantic_ptzextend_set_coverstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int enable = 0;
    int index = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Enable", &enable) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        if (Common_Json_GetAttrValueInt(indata, "Index", &index) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/PrivacyMask?%s=%d", enable ? "Show" : "Hide", index);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptzextend_start_setcover(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int index = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Index", &index) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/PrivacyMask?SetStart=%d", index);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptzextend_stop_setcover(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int index = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Index", &index) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/PrivacyMask?SetStop=%d", index);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptzextend_set_wiperstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int enable = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Enable", &enable) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/AuxOperate?Wiper=%d", enable);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptzextend_spraypos(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int code = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Code", &code) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/Spray?Operate=%d", code);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}

static int web_semantic_ptzextend_spraymode(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int code = 0;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "Code", &code) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        char uri[128];
        snprintf(uri, sizeof(uri), "/Ptz/Spray?Mode=%d", code);

        Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}
#endif
static int web_semantic_get_sessionid(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    S8 *szUserName = NULL;
    int ret = 0;
    int iEncMethod = -1;
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArray = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    Ovfs_Web_UpdateHeader(header, REST_POST, "/Access/OnlineUser");

    pArray = Common_Json_SetAttrValue(lowerData, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
//   cJSON_Struct *pAuth = Common_Json_Duplicate(Common_Json_GetAttrValueObj(header, "Auth"), 1);
//   Common_cJSON_AddItemToArrayByIndex(pArray, 0, pAuth);
    Common_Json_GetAttrValue(header, -1, "Auth/Method", NULL, NULL, &iEncMethod, NULL);
    Common_Json_SetAttrValue(pArray, 0, "AuthMethod", Common_Json_Type_Number, NULL, iEncMethod, 0);
    Common_Json_GetAttrValue(header, -1, "Auth/UserName", NULL, &szUserName, NULL, NULL);
    Common_Json_SetAttrValue(pArray, 0, "UserName", Common_Json_Type_String, szUserName, 0, 0);

    if(iEncMethod == 3)
    {
        S8 *s64Password = NULL,*szCreated = NULL,*szHexNonce = NULL;

        Common_Json_GetAttrValue(header, -1, "Auth/UsernameToken/PasswordDigest", NULL, &s64Password, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "PasswordDigest", Common_Json_Type_String, s64Password, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/UsernameToken/Created", NULL, &szCreated, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Created", Common_Json_Type_String, szCreated, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/UsernameToken/Nonce", NULL, &szHexNonce, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Nonce", Common_Json_Type_String, szHexNonce, 0, 0);

    }
    else if(iEncMethod == 2)
    {
        S8 *szSrcResponse = NULL,*szRealm = NULL,*szOpaque = NULL,*szNonce = NULL,*szNonceCount = NULL,*szQop = NULL,*szCNonce = NULL,*szDigestUri = NULL,*szMethod = NULL;

        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Realm", NULL, &szRealm, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Realm", Common_Json_Type_String, szRealm, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Qop", NULL, &szQop, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Qop", Common_Json_Type_String, szQop, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Nonce", NULL, &szNonce, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Nonce", Common_Json_Type_String, szNonce, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Opaque", NULL, &szOpaque, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Opaque", Common_Json_Type_String, szOpaque, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Cnonce", NULL, &szCNonce, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Cnonce", Common_Json_Type_String, szCNonce, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Method", NULL, &szMethod, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Method", Common_Json_Type_String, szMethod, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Uri", NULL, &szDigestUri, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Uri", Common_Json_Type_String, szDigestUri, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Response", NULL, (S8**)&szSrcResponse, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Response", Common_Json_Type_String, szSrcResponse, 0, 0);
        Common_Json_GetAttrValue(header, -1, "Auth/Digest/Nc", NULL, &szNonceCount, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Nc", Common_Json_Type_String, szNonceCount, 0, 0);

    }
    else if(iEncMethod == 1)
    {
        S8 *pPassword = NULL;
        Common_Json_GetAttrValue(header, -1, "Auth/Password", NULL, &pPassword, NULL, NULL);
        Common_Json_SetAttrValue(pArray, 0, "Password", Common_Json_Type_String, pPassword, 0, 0);
    }

    ret = Ovfs_Web_RestMethodA(header, lowerData, &pResult, 0);

    if(ret == 0)
    {
        pArray = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
        if(pArray)
        {
            iEncMethod = 0;
            if(Common_Json_GetAttrValue(pArray, 0, "SessionId", NULL, NULL, &iEncMethod, NULL))
            {
                Common_Json_SetAttrValueInt(outdata, "SessionId", iEncMethod);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(pResult)
    {
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return ret;

}

int isalarming = 1;

// webapi协议定义的报警类型值对应表.
// 0-报警输入
// 1-硬盘满；
//2-信号丢失；
//3－移动侦测；
//4－硬盘未格式化；
//5-读写硬盘出错；
// 6-遮挡报警；
//7-制式不匹配；
//8-非法访问；
//9-视频信号异常；
//10-录像异常；
// 20-网线断；
//21-IP冲突
//22-目标计数
//23-虚拟警戒线
//24-区域检测
//25 物品检测
// 26声音异常
// 27车牌检测
// 28人脸检测
// 29 火灾检测
// 30图像偏色
// 31亮度过亮
// 32 图像模糊
// 33智能移动侦测报警
// 34 视频亮度过暗
// 46人形检测
static struct
{
    int type;
    const char *name;
} const s_alarmTypeNameMap[] =
{
    {0, "AlarmIn"},
    {1, "DiskFull"},
    {3, "Motion"},
    {5, "DiskErr"},
    {6, "Vhide"},
    {7, "UnmatchFormat"},
    {8, "IllegallyAcc"},
    {10, "VideoRecordErr"},
    {20, "NetCableBreak"},
    {21, "IpConflict"},
    {23, "DetectWire"},
    {22, "CounterWire"},
    {24, "DetectRegion"},
    {25, "ObjectRegion"},
    {26, "SoundDetect"},
    {27, "DetectPlate"},
    {28, "DetectFace"},
    {29, "DetectFire"},
    {30, "Vdiagnose"}, // 视频诊断报警,没有区分具体是偏色/过亮/过暗/模糊.预留40-55给子类型
    {33, "SmartMotion"},
    {36, "MediaDisconnect"},//tyco平台断流报警
    {46, "DetectPerson"},
    {47, "RegionalInvasion"},
    {48, "PersonStaying"},
    {49, "DetectAbsent"},
    {50, "ParkingViolation"},
    {51, "Retrograde"}
    /*
        {22, "RecognitionFace"},
        {23, "Temperature"},
        {31, "AlarmIn"},
        {32, "AlarmIn"},
        {33, "AlarmIn"},
        {34, "AlarmIn"},
    */
};

//视频诊断子类型
static struct
{
    int regionId;
    int type;
    const char *name;
} const videoDiagnoseMap[] =
{
    {1<<0, 40,"VideoColor"},//颜色异常(偏色)
    {1<<1, 41,"VideoDark"},//亮度过暗
    {1<<2, 42,"VideoBright"},//亮度过亮
    {1<<3, 43,"VideoStripe"},//条纹干扰
    {1<<4, 44,"VideoSnowFlake"},//雪花干扰
    {1<<5, 45,"VideoShield"},//视频遮挡
    {1<<6, 46,"VideoFreeze"},//画面冻结
    {1<<7, 47,"VideoLost"},//视频丢失
    {1<<8, 48,"VideoBlur"},//画面模糊
    {1<<9, 49,"VideoJitter"},//画面抖动
    {1<<10, 50,"VideoPTZ"},//ptz异常
    {1<<11, 51,""},//预留
    {1<<12, 52,""},//预留
    {1<<13, 53,""},//预留
    {1<<14, 54,""},//预留
    {1<<15, 55,""}//预留
};

long int Net_GetTimeDiff()
{
    tzset();
    struct tm p = {};
    time_t t = time(NULL);
    localtime_r(&t,&p);
    return p.tm_gmtoff;
}


int TimeStr2UnixTime(const char *timeStr, int *ltime)
{
    int ret = 0;

    int year = 0;
    int month = 0;
    int mday = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (sscanf(timeStr, "%04d%02d%02d%02d%02d%02d", &year, &month, &mday, &hour, &minute, &second) < 6)
    {
        *ltime = 0;
        ret = -1;
    }
    else
    {
        struct tm tmTime = {0};
        tmTime.tm_sec = second;
        tmTime.tm_min = minute;
        tmTime.tm_hour = hour;
        tmTime.tm_mday = mday;
        tmTime.tm_mon = month - 1;
        tmTime.tm_year = year - 1900;
        tmTime.tm_isdst = -1;//depends sysytem control
        time_t thisTime = mktime(&tmTime);
        //tmTime.tm_isdst = 0;
        //time_t thisTime2 = mktime(&tmTime);

        //	struct tm tmpTime;
        //	time_t tt = time(NULL);
        //	localtime_r(&tt, &tmpTime);




        //	LOGD("dst[%d]  is_dst:%d\n",thisTime,tmpTime.tm_isdst);


        *ltime = thisTime;
    }

    return ret;
}

char UnixTime2TimeStr(int time, char **timeStr)
{
    char *strTemp = Common_Calloc(1,32,__FUNCTION__,__LINE__);
    struct tm tmTime = {0};
    Common_LocalTime_r((time_t *)&time,&tmTime);
    sprintf(strTemp,"%04d-%02d-%02d %02d:%02d:%02d",
        tmTime.tm_year + 1900,tmTime.tm_mon + 1,tmTime.tm_mday,
        tmTime.tm_hour,tmTime.tm_min,tmTime.tm_sec);

    *timeStr = strTemp;

    return 0;
}

static int web_semantic_getalarm_status(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    /*
    URL:/Alarm/Status
    "ResList":      [{
    	    "AlarmName":    "AlarmIn",
    	    "AlarmType":    0,
    	    "AlarmSrcType": 1,
    	    "Device":       0,
    	    "Channel":      0,
    	    "Stream":       0,
    	    "RegionId":     0,
    	    "Status":       0,
    	    "StartTime":    "20171114105851",
    	    "StopTime":     "20171114110427"
    	}]
    */
    int ret = 0;

    cJSON_Struct *alarmEventAll = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Alarm/Status?AlarmName=All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &alarmEventAll, 0);
    }

    if (0 == ret)
    {
        cJSON_Struct *resList = Common_Json_GetAttrValueArr(alarmEventAll, "ResList");
        int arraySize = Common_Json_ArraySize(resList);

        int outIndex = 0;
        cJSON_Struct *outList = NULL;
        if (arraySize > 0)
        {
            outList = Common_Json_SetAttrValueArr(outdata, "AlarmInfo");
        }

        time_t timeNow = time(NULL);
        int i;
        for (i = 0; i < arraySize; i++)
        {
            char *alarmName = NULL;
            int device = 0;
            int channel = 0;
            int stream = 0;
            int regionId = 0;
            int status = 0;
            char *startTime = NULL;
            char *stopTime = NULL;
            int valueStart = 0;
            int valueStop = 0;

            cJSON_Struct *alarmEach = Common_Json_GetAttrValueArrItem(resList, i);
            if (Common_Json_GetAttrValueStr(alarmEach, "AlarmName", &alarmName) == NULL)
            {
                LOGW("Can not read AlarmName.\n");
                continue;
            }
            if (Common_Json_GetAttrValueStr(alarmEach, "StartTime", &startTime) == NULL || TimeStr2UnixTime(startTime, &valueStart) < 0 || valueStart <= 0)
            {
                //LOGW("Can not read StartTime.\n");
                continue;
            }
            if (Common_Json_GetAttrValueInt(alarmEach, "Status", &status) == NULL)
            {
                LOGW("Can not read Status.\n");
                continue;
            }
            if (Common_Json_GetAttrValueStr(alarmEach, "StopTime", &stopTime) == NULL
                    || TimeStr2UnixTime(stopTime, &valueStop) < 0 || valueStop <= 0 || valueStop < valueStart)
            {
                //LOGW("Can not read StopTime.\n");
                //continue;
                valueStop = MAX2(valueStart, timeNow);
            }
            if (status == 0 && ( (valueStop + 5 < timeNow) ||  (valueStart > timeNow) ) )
            {
                // 太古老的报警事件就不再上报.
                continue;
            }
            Common_Json_GetAttrValueInt(alarmEach, "Device", &device);
            Common_Json_GetAttrValueInt(alarmEach, "Channel", &channel);
            Common_Json_GetAttrValueInt(alarmEach, "Stream", &stream);
            Common_Json_GetAttrValueInt(alarmEach, "RegionId", &regionId);

            int upperAlarmType = -1;
            int a;
            for (a = 0; a < sizeof(s_alarmTypeNameMap)/sizeof(s_alarmTypeNameMap[0]); a++)
            {
                if (strcmp(s_alarmTypeNameMap[a].name, alarmName) == 0)
                {
                    upperAlarmType = s_alarmTypeNameMap[a].type;
                    break;
                }
            }
            if (upperAlarmType < 0)
            {
                continue;
            }

            cJSON_Struct *outListEach = Common_Json_SetAttrValueArrObj(outList, outIndex++);
            {
                char strbuf[256] = {0};
                if(upperAlarmType >= 22 && upperAlarmType <= 25)
                {
                    snprintf(strbuf,sizeof(strbuf),"%d_%d",channel+1,regionId+1)	;
                    Common_Json_SetAttrValueStr(outListEach, "Source", strbuf);
                }
                else
                {
                    snprintf(strbuf,sizeof(strbuf),"%d",channel+1);
                    Common_Json_SetAttrValueStr(outListEach, "Source", strbuf);
                }
            }
            Common_Json_SetAttrValueInt(outListEach, "MajorType", 0);
            if(upperAlarmType == 30)//videodiagnose
            {
                //deal with video diagnose sub type
                for (a = 0; a < sizeof(videoDiagnoseMap)/sizeof(videoDiagnoseMap[0]); a++)
                {
                    if (videoDiagnoseMap[a].regionId == regionId)
                    {
                        upperAlarmType = videoDiagnoseMap[a].type;
                        break;
                    }
                }
            }
            Common_Json_SetAttrValueInt(outListEach, "MinorType", upperAlarmType);

            Common_Json_SetAttrValueInt(outListEach, "State", status);
            Common_Json_SetAttrValueInt(outListEach, "StartTime", valueStart);
            Common_Json_SetAttrValueInt(outListEach, "StopTime", valueStop);
        }
    }

    if (alarmEventAll)
    {
        Common_Json_Delete(alarmEventAll);
        alarmEventAll = NULL;
    }

    return  ret;
}

static int web_semantic_logctrl(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int logType = 0;
    int startTime = 0;
    int endTime = 0;
    int curPageNum = 0;
    int valueInt = 0;
    int arrySize = 0;
    int iloop = 0;
    int logHandle = 0;
    char *str_tmp = NULL;
    int totalItemNum = 0;
    int totalPageNum = 0;
    int itemEveryPage = 0;
    char uri_tmp[128] = {0};
    cJSON_Struct *pArry_root = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pResult = NULL;

    if (0 == ret)
    {
        char* localDTStr1;
        char* localDTStr2;

        if (Common_Json_GetAttrValueInt(indata, "MajorType", &logType) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValue(indata,-1, "DeviceLocalDateTimeStart",NULL, &localDTStr1,NULL,NULL) &&
                Common_Json_GetAttrValue(indata,-1, "DeviceLocalDateTimeStop",NULL, &localDTStr2,NULL,NULL))
        {
            LOGW("string -->localDateTime BeginDateTime:[%s] strin --> localDateTime EndDateTime:[%s]\n",localDTStr1,localDTStr2);
            TimeStr2UnixTime(localDTStr1,&startTime);
            TimeStr2UnixTime(localDTStr2,&endTime);

            LOGW("string --> BeginDateTime:[%d] strin --> EndDateTime:[%d]\n",startTime,endTime);
        }
        else if(Common_Json_GetAttrValue(indata,-1, "StartTime", NULL,NULL,&startTime,NULL) &&
                Common_Json_GetAttrValue(indata,-1, "EndTime",NULL, NULL,&endTime,NULL))
        {


        }
        else
        {

            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "CurrentPage", &curPageNum) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }

        if (Common_Json_GetAttrValueInt(indata, "PageNum", &itemEveryPage) == NULL || itemEveryPage < 1)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        snprintf(uri_tmp, sizeof(uri_tmp), "/EventLog/LogFunction?MajorType=%d&&StartTime=%d&&EndTime=%d", logType, startTime, endTime);
        Ovfs_Web_UpdateHeader(header, REST_POST, uri_tmp);
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            pArry_root = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
            if (pArry_root)
            {
                if (Common_Json_GetAttrValue(pArry_root, 0, "dwHandle", NULL, NULL, &logHandle, NULL) == NULL || logHandle == -1)
                {
                    ret = WEB_CODE_InternalMistake;
                }

                if (Common_Json_GetAttrValue(pArry_root, 0, "dwTotalCount", NULL, NULL, &totalItemNum, NULL) == NULL)
                {
                    totalItemNum = 0;
                }
                totalPageNum = (totalItemNum + itemEveryPage - 1)/itemEveryPage;
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (0 == ret)
    {
        cJSON_Struct *logResult = Common_Json_SetAttrValue(outdata, -1, "LogResults", Common_Json_Type_Object, NULL, 0, 0);

        Common_Json_SetAttrValueInt(logResult, "LogItemCount", totalItemNum);
        Common_Json_SetAttrValueInt(logResult, "TotalPages", totalPageNum);

        pArry_tmp = Common_Json_SetAttrValueArr(logResult, "Items");

        LOGW("totalItemNum=%d\n", totalItemNum);
        if (totalItemNum > 0)
        {
            snprintf(uri_tmp, sizeof(uri_tmp), "/EventLog/LogFunction?dwHandle=%d&&iIndex=%d&&iCount=%d", logHandle, (curPageNum - 1)*itemEveryPage, itemEveryPage);
            Ovfs_Web_UpdateHeader(header, REST_GET, uri_tmp);
            ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        }
        if (0 == ret)
        {
            if ((pArry_root = Common_Json_GetAttrValueArr(pResult, "ResList")) != NULL)
            {
                arrySize = Common_Json_Size(pArry_root);
                for (iloop = 0; iloop < arrySize; iloop ++)
                {
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "No", Common_Json_Type_Number, NULL, (curPageNum-1)*itemEveryPage+iloop+1, 0);

                    Common_Json_GetAttrValue(pArry_root, iloop, "LogTime", NULL, &str_tmp, NULL, NULL);
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "LogDateTime", Common_Json_Type_String, str_tmp, 0, 0);

                    Common_Json_GetAttrValue(pArry_root, iloop, "MajorType", NULL, NULL, &valueInt, NULL);
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "MajorType", Common_Json_Type_Number, NULL, valueInt, 0);

                    Common_Json_GetAttrValue(pArry_root, iloop, "MinorType", NULL, NULL, &valueInt, NULL);
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "MinorType", Common_Json_Type_Number, NULL, valueInt, 0);

                    Common_Json_GetAttrValue(pArry_root, iloop, "Channel", NULL, NULL, &valueInt, NULL);
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "ChannelOrPort", Common_Json_Type_Number, NULL, valueInt, 0);

                    Common_Json_GetAttrValue(pArry_root, iloop, "RemoteHostAddress", NULL, &str_tmp, NULL, NULL);
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "IP", Common_Json_Type_String, str_tmp, 0, 0);


                    Common_Json_GetAttrValue(pArry_root, iloop, "Status", NULL, NULL, &valueInt, NULL);
                    Common_Json_SetAttrValue(pArry_tmp, iloop, "Status", Common_Json_Type_Number, NULL, valueInt, 0);
                    }
                }
            }

        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (ret == 0 && logHandle != -1)
    {
        snprintf(uri_tmp, sizeof(uri_tmp), "/EventLog/LogFunction?dwHandle=%d", logHandle);
        logHandle = -1;

        Ovfs_Web_UpdateHeader(header, REST_DELETE, uri_tmp);
        Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    return ret;
}


void *Check_SessionID_timeout(void *arg)
{
    int i = 0;
    int count = 0;

    cJSON_Struct *header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    cJSON_Struct *headerAuth = Common_Json_SetAttrValueObj(header,"Auth");

    while(1)
    {
        count = Common_DList_GetCount(g_ovfs_web->userLoginList);

        for(i=0; i<count; i++)
        {
            long time_now = time(NULL);
            MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
            WEB_LOGINSUCCESS_NODE_T *tmp_sucess = Common_DList_GetNode(g_ovfs_web->userLoginList,i);
            if(tmp_sucess && tmp_sucess->start_time)
            {
                if(time_now - tmp_sucess->start_time > 180) //16s for test
                {
                    char uri[128];
                    snprintf(uri, sizeof(uri), "/Access/OnlineUser?SessionId=%d", tmp_sucess->session_id);
                    Common_Json_SetAttrValueInt(headerAuth, "SsessionId", tmp_sucess->session_id);

                    Ovfs_Web_UpdateHeader(header, REST_DELETE, uri);
                    int ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

                    int rrr = Common_DList_Delete(g_ovfs_web->userLoginList, (void *)tmp_sucess->session_id, web_loginsuccess_nodecompare);
                    LOGE("RRR:[%d]\n",rrr);
                }


            }
            MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
        }
        sleep(60);
    }
    if (header)
    {
        Common_Json_Delete(header);
        header = NULL;
    }
}

//用户登陆设备
int frmUserLogin(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    //Common_cJSON_T *retData = NULL;
    char errorStrBuf[128]= {0};
    int errorStrLen = 0;
    OVFS_WEB_ALARM_T alarm;
    const int maxCoolTime = g_ovfs_web->maxCoolTime;
    const int maxTryCount = g_ovfs_web->maxTryCount;
    int waitTime = 0;
    LOGW("--------maxTryCount:[%d] maxCoolTime:[%d]  |||->[%d][%d]\n",maxTryCount,maxCoolTime,g_ovfs_web->maxTryCount,g_ovfs_web->maxCoolTime);
    LOGD("IP:%s\n", wp->ipaddr);

    MUTEX_LOCK(g_ovfs_web->hLoginFailedLock);

    // 登录操作没有被阻塞吧?(连续多次错误密码登录将会导致被阻塞).
    WEB_LOGINFAILED_NODE_T *tmp = NULL;
    WEB_LOGINSUCCESS_NODE_T *tmp_sucess = NULL;
    if (0 == ret)
    {
        tmp = Common_DList_Search(g_ovfs_web->loginFailedList, (void *)wp, web_loginfailed_nodecompare);
        if ((tmp != NULL) && tmp->blocked)
        {
            ret = WEB_CODE_BlockingOperation;
            errorStrLen = snprintf(errorStrBuf + errorStrLen, sizeof(errorStrBuf) - errorStrLen, "Account is locked. Countdown time is %d.", tmp->remain_time);
        }
    }

    /*
    if (0 == ret && g_ovfs_web->enable_single_account_login_mode)
    {
    	MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
        tmp = Common_DList_Search(g_ovfs_web->userLoginList, (void *)wp, web_loginsuccess_nodecompareV2);
        if ((tmp != NULL))
        {
            ret = WEB_CODE_SingleAccountLogin;
            errorStrLen = snprintf(errorStrBuf + errorStrLen, sizeof(errorStrBuf) - errorStrLen, "%s %s has Logined!.",tmp->ip, tmp->username);
        }
    	MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
    }*/


    if(ret == 0 && g_ovfs_web->enable_single_account_login_mode)
    {

        char szUri[128] = {0};
        cJSON_Struct *pResult = NULL;
        snprintf(szUri,sizeof(szUri),"/Access/OnlineUser?UserName=%s",wp->username);
        Ovfs_Web_UpdateHeader(header, REST_GET, szUri);
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        cJSON_Struct *lowReslist = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
        if(lowReslist)
        {
            int hhhh = Common_Json_ArraySize(lowReslist);

            if( hhhh > 0)
            {

                cJSON_Struct *pArray = Common_Json_GetAttrValue(lowReslist, 0, "ConnectInfo", NULL, NULL, NULL, NULL);

                int cInfo_n = 0;
                char *szStr = NULL;
                for(cInfo_n = 0; cInfo_n<Common_Json_ArraySize(pArray); cInfo_n++)
                {
                    if(Common_Json_GetAttrValue(pArray, cInfo_n, "From", NULL, &szStr, NULL, NULL))
                    {
                        if(strcasecmp("Webserver",szStr) == 0)
                        {
                            ret = WEB_CODE_SingleAccountLogin;
                            Common_Json_GetAttrValue(pArray, cInfo_n, "IPv4", NULL, &szStr, NULL, NULL);
                            errorStrLen = snprintf(errorStrBuf + errorStrLen, sizeof(errorStrBuf) - errorStrLen, "%s %s has Logined!.",szStr, wp->username);
                            break;
                        }
                    }
                }
            }
        }

        if(pResult)
        {
            Common_Json_Delete(pResult);
            pResult = NULL;
        }

    }

    if(ret == 0 && g_ovfs_web->enable_session)
    {

        /*-------------------- Access session count
                cJSON_Struct *pResult = NULL;

                Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/OnlineUser");
                ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);

                cJSON_Struct *lowReslist = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
                if(lowReslist)
                {
                    int webserver_session_count = 0;
                    int hhhh = Common_Json_ArraySize(lowReslist);
                    int i = 0;
                    for(i = 0; i < hhhh; i++)
                    {
                        char *username_str = NULL;
                        if(Common_Json_GetAttrValue(lowReslist, i, "UserName", NULL, &username_str, NULL, NULL))
                        {
                            if(slen(username_str) == 0)
                            {
                                continue;
                            }
                            cJSON_Struct *pArray = Common_Json_GetAttrValue(lowReslist, i, "ConnectInfo", NULL, NULL, NULL, NULL);

                            if(pArray)
                            {
                                int cInfo_n = 0;
                                char *szStr = NULL;
                                for(cInfo_n = 0;cInfo_n<Common_Json_ArraySize(pArray);cInfo_n++)
                                {
                                    if(Common_Json_GetAttrValue(pArray, cInfo_n, "From", NULL, &szStr, NULL, NULL))
                                    {
                                        if(strcasecmp("Webserver",szStr) == 0)
                                        {
                                            webserver_session_count++;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if(webserver_session_count >= g_ovfs_web->session_count)
                    {
                        ret = WEB_CODE_SessionCountMax;
                    }

                }
        ----------------------*/
        // webserver dlist session count
        int webserver_session_count = Common_DList_GetCount(g_ovfs_web->userLoginList);
        LOGW("webserver_session_count:[%d]\n",webserver_session_count);
        if(webserver_session_count >= g_ovfs_web->session_count)
        {
            ret = WEB_CODE_SessionCountMax;
        }

    }


    // 继续login动作.
    if (0 == ret)
    {
        /*if (g_struWebSiteSDKInfo.g_isAlarming != -1)
        {
        	g_struWebSiteSDKInfo.g_isAlarming = 1;
        }*/

        int retLogin = web_semantic_login(header, indata, outdata);
        // 如果登录失败,记录登陆失败的状态.

        if (retLogin == WEB_CODE_InternalMistake)
        {
            //非用户名密码错误
            ret = WEB_CODE_InternalMistake;
        }
        else if (retLogin != 0)
        {
            tmp = Common_DList_Search(g_ovfs_web->loginFailedList, (void *)wp, web_loginfailed_nodecompare);
            if (tmp)
            {
                tmp->count += 1;

                if (tmp->count >= maxTryCount)
                {
                    if (0 == tmp->blocked)
                    {
                        tmp->remain_time = maxCoolTime;
                        tmp->blocked = 1;
                        ret = WEB_CODE_BlockingOperation;
                        errorStrLen = snprintf(errorStrBuf + errorStrLen, sizeof(errorStrBuf) - errorStrLen, "Account is locked. Countdown time is %d.", tmp->remain_time);

                        memset(&alarm, 0, sizeof(OVFS_WEB_ALARM_T));
                        snprintf(alarm.ip, sizeof(alarm.ip), "%s", wp->ipaddr);
                        alarm.ishappen = 1;
                        snprintf(alarm.alarm_name, sizeof(alarm.alarm_name), "IllegallyAcc");
                        ovfs_web_send_alarm(&alarm);
                    }
                }
                else
                {
                    ret = WEB_CODE_GeneralMistake;
                    errorStrLen = snprintf(errorStrBuf + errorStrLen, sizeof(errorStrBuf) - errorStrLen, "Username or password is illegal. Remaining try count is %d.", maxTryCount - tmp->count);
                    waitTime = (tmp->count - 1);
                }
            }
            else
            {
                tmp = Common_Calloc(1, sizeof(WEB_LOGINFAILED_NODE_T), __FUNCTION__, __LINE__);
                tmp->username = Common_StrDup(wp->username, __FUNCTION__, __LINE__);
                tmp->ip = Common_StrDup(wp->ipaddr, __FUNCTION__, __LINE__);
                tmp->count = 1;
                tmp->blocked = 0;
                tmp->remain_time = 0;
                Common_DList_InsertTail(g_ovfs_web->loginFailedList, (void *)tmp, sizeof(WEB_LOGINFAILED_NODE_T));
                ret = WEB_CODE_GeneralMistake;
                errorStrLen = snprintf(errorStrBuf + errorStrLen, sizeof(errorStrBuf) - errorStrLen, "Username or password is illegal. Remaining try count is %d.", maxTryCount - tmp->count);
                waitTime = (tmp->count - 1);
            }
        }
        else // 登录成功记录日志.清除登录失败的状态.
        {
#if 1//ndef SIMPLIFIED
            OVFS_WEB_LOG_T log;
            memset(&log, 0, sizeof(OVFS_WEB_LOG_T));
            log.major_type = 0x3;
            log.minor_type = 0x70;
            snprintf(log.username, sizeof(log.username), "%s", wp->username);
            snprintf(log.ip, sizeof(log.ip), "%s", wp->ipaddr);
            //web_set_log(&log);
#endif
            Common_DList_Delete(g_ovfs_web->loginFailedList, (void *)wp->ipaddr, web_loginfailed_nodecompare);

            //insert success
            if(g_ovfs_web->enable_session)
            {
                ret = web_semantic_get_sessionid(header, indata, outdata);

                if(ret == 0)
                {
                    Common_Json_SetAttrValueInt(outdata, "SessionTimeOut", g_ovfs_web->session_timeout);

                    MUTEX_LOCK(g_ovfs_web->hReqSessionLock);

                    tmp_sucess = Common_Calloc(1, sizeof(WEB_LOGINSUCCESS_NODE_T), __FUNCTION__, __LINE__);
                    tmp_sucess->start_time = time(NULL);
                    tmp_sucess->current_time = tmp_sucess->start_time;

                    Common_Json_GetAttrValueInt(outdata, "SessionId", (S32*)&(tmp_sucess->session_id));

                    Common_DList_InsertTail(g_ovfs_web->userLoginList, (void *)tmp_sucess, sizeof(WEB_LOGINSUCCESS_NODE_T));
                    MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);

                    int err = -1;

                    if(!session_tId)
                    {
                        err = pthread_create(&session_tId, NULL, Check_SessionID_timeout, NULL);
                        if (err != 0)
                        {
                            ret = WEB_CODE_LackingThread;
                        }
                    }


                }
            }

            int need_modify = 0;
            char buf[128] = {0};
            char *def_pwd = NULL;
            char *str_pwd = NULL;
            cJSON_Struct *lowerData = NULL;

            snprintf(buf,sizeof(buf),"/Access/UserCfg?UserName=%s",wp->username);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);
            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

            if(ret == 0)
            {
                cJSON_Struct *lowReslist = Common_Json_GetAttrValue(lowerData, -1, "ResList", NULL, NULL, NULL, NULL);

                if(Common_Json_GetAttrValue(lowReslist, 0, "DefaultPwd", NULL, &def_pwd, 0, 0))
                {

                    cJSON_Struct *pwd_arr = Common_Json_GetAttrValue(lowReslist, 0, "Password", NULL, NULL, NULL, NULL);
                    if(pwd_arr)
                    {
                        str_pwd = Common_cJSON_GetArrayItem(pwd_arr,0)->valuestring;

                        if(smatch(def_pwd, str_pwd))
                        {
                            need_modify = 1;
                        }

                    }

                }

            }
            Common_Json_SetAttrValueInt(outdata, "DefaultPwd", need_modify);

            if(lowerData)
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }

        }
    }

    MUTEX_UNLOCK(g_ovfs_web->hLoginFailedLock);

    if (errorStrLen > 0)
    {
        LOGW("%s\n", errorStrBuf);
        Common_Json_SetAttrValueStr(outdata, ERROR_STRING, errorStrBuf);
    }

    //web_semantic_func_endA(wp, ret, outdata);

    if (waitTime > 0)
    {
        usleep(1000000*waitTime);
    }

    return ret;
}

int frmUserLogout(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    OVFS_WEB_LOG_T log;

//   Common_cJSON_AddItemToObject(outdata, RESULT, Common_cJSON_CreateNumber(WEB_CODE_OK));
//   Common_cJSON_AddItemToObject(outdata, DATA, Common_cJSON_CreateString(""));
    Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
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
    // 应该维护一个在线用户列表,从列表中匹配用户,并且退出登录.
    memset(&log, 0, sizeof(OVFS_WEB_LOG_T));
    log.major_type = 0x3;
    log.minor_type = 0x71;
    snprintf(log.username, sizeof(log.username), "%s", wp->username?wp->username:"admin");
    snprintf(log.ip, sizeof(log.ip), "%s", wp->ipaddr);
    //web_set_log(&log);

    return ret;
}

//预览云镜操作
int frmPTZControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_ptzcontrol(opt,header, indata, outdata);
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

//预置点
int frmPTZPreset(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_ptz_preset(header, indata, outdata);
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;

}

#if 0
//巡航
int frmPTZCruise(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0: // 获取巡航路径
            ret = web_semantic_get_ptz_cruise(header, indata, outdata);
            break;

        case 1: // 添加预置点到巡航路径
            ret = web_semantic_set_ptz_cruise(header, indata, outdata);
            break;

        case 2: // 删除巡航路径中的某个预置点
        case 3: // 删除巡航路径
            ret = web_semantic_del_ptz_cruise(header, indata, outdata);
            break;
        case 4: // 调用巡航路径信息
            MUTEX_LOCK(g_ovfs_web->hReqPtzCruiseCallLock);
            ret = web_semantic_call_ptz_cruise(header, indata, outdata);
            MUTEX_UNLOCK(g_ovfs_web->hReqPtzCruiseCallLock);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret&& opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

//轨迹
int frmPTZTrack(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_set_ptz_track(header, indata, outdata);
            break;

        case 1:
            MUTEX_LOCK(g_ovfs_web->hReqPtzTackCallLock);
            ret = web_semantic_call_ptz_track(header, indata, outdata);
            MUTEX_UNLOCK(g_ovfs_web->hReqPtzTackCallLock);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret&& opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}
//#if 0//def WITH_PTZ

/*两点扫描*/
int frmPTZExtend_SetScan(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_ptz_start_setscan(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_ptz_stop_setscan(header, indata, outdata);
            break;

        case 2:
            ret = web_semantic_ptz_call_setscan(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret&& opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*空闲操作*/
int frmPTZExtend_IdleOperation(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_ptz_get_idleoperation(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_ptz_set_idleoperation(header, indata, outdata);
            break;

        case 2:
            ret = web_semantic_ptz_call_idleoperation(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret && opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*
红外补光
模式切换
类型切换
*/

int frmPTZExtend_IrlightCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_ptzextend_irlightctrl(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_ptzextend_irlightctrl(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret&& opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*
3D定位
*/
int frmPTZExtend_3DPosition(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            ret = web_semantic_ptzextend_set_3dpositon(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret&& opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*
隐私遮蔽
*/
int frmPTZExtend_SetCover(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_ptzextend_get_coverstatus(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_ptzextend_set_coverstatus(header, indata, outdata);
            break;

        case 2:
            ret = web_semantic_ptzextend_start_setcover(header, indata, outdata);
            break;

        case 3:
            ret = web_semantic_ptzextend_stop_setcover(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret&& opt->type != 0)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*
雨刷 开关
//雨刷0-关闭 1-开启
*/
int frmPTZExtend_Wiper(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            //设置参数
            ret = web_semantic_ptzextend_set_wiperstatus(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*
//喷淋位置设置 0-设置 1-删除
*/
int frmPTZExtend_SprayPos(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            //设置参数
            ret = web_semantic_ptzextend_spraypos(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

/*
//喷淋模式0-自动 1-手动(默认)
*/
int frmPTZExtend_SprayMode(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            //设置参数
            ret = web_semantic_ptzextend_spraymode(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}
#endif
int web_semantic_get_arming_status(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{

    int ret = 0;

    cJSON_Struct *result = NULL;
    // cJSON_Struct *ar = Common_Json_SetAttrValueArr(lowerData, "ResList");
    // cJSON_Struct *obj = Common_Json_SetAttrValueArrObj(ar, 0);
    // Common_Json_SetAttrValueInt(obj, "Enable", 1);

    // char* out = Common_Json_Print(lowerData,NULL);
    // wfree(out);
    Ovfs_Web_UpdateHeader(header, REST_GET, "/Alarm/arming");
    ret = Ovfs_Web_RestMethodA(header,NULL, &result, 0);

    if(ret == 0)
    {
        int enable = 0;
        cJSON_Struct *ar = Common_Json_GetAttrValueArr(result, "ResList");
        cJSON_Struct *obj = Common_Json_GetAttrValueArrItem(ar, 0);
        if(Common_Json_GetAttrValueInt(obj, "Enable", &enable))
        {
            Common_Json_SetAttrValueInt(outdata,"arming_status", enable);
            Common_Json_SetAttrValueInt(g_ovfs_web->pDeviceStatus,"OneArming", enable);
        }
    }

    if(result)
    {
        Common_Json_Delete(result);
        result = NULL;
    }

    return ret;
}

int web_semantic_enable_arming(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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

        cJSON_Struct *ar = Common_Json_SetAttrValueArr(lowerData, "ResList");
        cJSON_Struct *obj = Common_Json_SetAttrValueArrObj(ar, 0);
        Common_Json_SetAttrValueInt(obj, "Enable", 1);

        char* out = Common_Json_Print(lowerData,NULL);
        wfree(out);
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/arming");
        ret = Ovfs_Web_RestMethodA(header,lowerData, NULL, 0);
    }

    if (0 == ret)
    {
        int status= -1;
        Common_Json_GetAttrValueInt(g_ovfs_web->pDeviceStatus,"OneArming", &status);
        if(status != 1)
        {
            Common_Json_SetAttrValueInt(g_ovfs_web->pDeviceStatus,"OneArming", 1);
            g_ovfs_web->need_send_devicestatus = 1;
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
    }



    return ret;
}

int web_semantic_disable_arming(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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

        cJSON_Struct *ar = Common_Json_SetAttrValueArr(lowerData, "ResList");
        cJSON_Struct *obj = Common_Json_SetAttrValueArrObj(ar, 0);
        Common_Json_SetAttrValueInt(obj, "Enable", 0);
        char* out = Common_Json_Print(lowerData,NULL);
        wfree(out);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/arming");
        ret = Ovfs_Web_RestMethodA(header,lowerData, NULL, 0);
    }

    if (0 == ret)
    {
        int status= -1;
        Common_Json_GetAttrValueInt(g_ovfs_web->pDeviceStatus,"OneArming", &status);
        if(status != 0)
        {
            Common_Json_SetAttrValueInt(g_ovfs_web->pDeviceStatus,"OneArming", 0);
            g_ovfs_web->need_send_devicestatus = 1;
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
    }


    return ret;
}



//得到布防撤防信息
int frmGetAlarmInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取报警信息
            //ret = WEB_CODE_Unsupported;
            ret = web_semantic_get_arming_status(header, indata, outdata);
            break;

        case 1:
            //布防
            //ret = WEB_CODE_Unsupported;
            ret = web_semantic_enable_arming(header, indata, outdata);
            break;

        case 2:
            //撤防
            //ret = WEB_CODE_Unsupported;
            ret = web_semantic_disable_arming(header, indata, outdata);
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

U32 LocalTimeV2(AntsHostMgrLibTimeV2_T struTime, U32 timezone /*=8*/)
{
    long res = 0 ;
    //AntsHostMgrLibTime_T struTime ;
    //memcpy(&struTime, time, sizeof(AntsHostMgrLibTime_T)) ;

    if(struTime.byMonth <= 2)
    {
        struTime.byMonth += 10 ;
        struTime.wYear -= 1 ;
    }
    else
    {
        struTime.byMonth -= 2 ;
    }

    res = (long)(struTime.wYear/4 - struTime.wYear/100 + struTime.wYear/400) + 367 * struTime.byMonth/12 + struTime.byDay + struTime.wYear*365 - 719499 ;

    res = ((res*24 + struTime.byHour )*60 + struTime.byMinute)*60 + struTime.bySecond ;

    res -= timezone * 60 * 60 ;

    return res ;

}

int frmQueryAlarmInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取报警状态
            ret = web_semantic_getalarm_status(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    return ret;
}

// 查询数据提交
/*int frmLogCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_logctrl(header, indata, outdata);
    }

    return ret;
}*/

extern int JsonOper_MergeObj(Common_cJSON_T* dst,Common_cJSON_T* src,int type);

int frmKeepSessionAlive(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        if(opt->type == 0)
        {
            if(g_ovfs_web->enable_session)
            {
                MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
                int sessionid = strtoi(wp->session_id);
                WEB_LOGINSUCCESS_NODE_T *tmp = Common_DList_Search(g_ovfs_web->userLoginList, (void *)sessionid, web_loginsuccess_nodecompare);
                if(tmp)
                {
                    tmp->start_time = time(NULL);
                    LOGD("-----frmKeepSessionAlive! session_id:[%ld] start_time:[%d]----\n",tmp->session_id,tmp->start_time);
                }
                MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
            }
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    return ret;
}


static int web_semantic_get_cruisecontrol(cJSON_Struct *header, cJSON_Struct *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    Common_Json_SetAttrValueInt(outdata, "MaxListNum", 20);

    Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/cruisecontrol");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
    }

    if(Common_Json_GetAttrValueInt(lowerData, "ResumeTime", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "ResumeTime", i_num);
    }

    cJSON_Struct *list_get = Common_Json_GetAttrValueArr(lowerData, "List");
    cJSON_Struct *list_set = Common_Json_SetAttrValueArr(outdata, "List");
    int size = Common_Json_ArraySize(list_get);
    if(size > 0)
    {
        for(i=0; i<size; i++)
        {
            cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(list_set, i);
            if(Common_Json_GetAttrValue(list_get, i, "PresetNo", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(tmp, "PresetNo", i_num);
            }
            if(Common_Json_GetAttrValue(list_get, i, "Dwell", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(tmp, "Dwell", i_num);
            }
            if(Common_Json_GetAttrValue(list_get, i, "Speed", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(tmp, "Speed", i_num);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_cruisecontrol(cJSON_Struct *header, cJSON_Struct *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/cruisecontrol");
    ret = Ovfs_Web_RestMethodA(header, indata, NULL, 0);

    return ret;
}

int frmCruiseControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if(g_ovfs_web->devInfo.bPTZ != 4 && g_ovfs_web->devInfo.bPTZ != 5)
    {
        ret = WEB_CODE_Unsupported;
    }

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_cruisecontrol(header, indata, outdata,opt);
            break;
        case 1:
            ret = web_semantic_set_cruisecontrol(header, indata, outdata,opt);
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

static int web_semantic_get_laserlight(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *inParam = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((inParam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }


    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(inParam, "Type", 47);
        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/cmd");
        ret = Ovfs_Web_RestMethodA(header, inParam, &lowerData, 0);
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "laser", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "model", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Mode", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "multiple", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Threshold", i_num);
        }
    }

    if(inParam)
    {
        Common_Json_Delete(inParam);
        inParam = NULL;
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_laserlight(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
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
        Common_Json_SetAttrValueInt(lowerData, "Type", 46);
        cJSON_Struct *obj = Common_Json_SetAttrValueObj(lowerData, "CmdParam");
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(obj, "laser", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Mode", &i_num))
        {
            Common_Json_SetAttrValueInt(obj, "model", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Threshold", &i_num))
        {
            Common_Json_SetAttrValueInt(obj, "multiple", i_num);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/cmd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


int frmLaserLight(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_laserlight(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_laserlight(header, indata, outdata);
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

static int web_semantic_get_uartconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    int modeCapCount = 0;
    int bSupportTransData = 0;
    char buf[16] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *modeCap = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/ability");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        cJSON_Struct *modeCap = Common_Json_GetAttrValueArr(lowerData, "ModeCapability");
        if(modeCap)
        {
            Common_Json_AddItem(outdata, -1, "ModeCapability", Common_Json_Duplicate(modeCap, 1));

            int size = Common_Json_ArraySize(modeCap);
            for(i=0; i<size; i++)
            {
                i_num= 0;
                Common_Json_GetAttrValue(modeCap, i, NULL, NULL, NULL, &i_num, NULL);
                if(i_num == 3)
                {
                    bSupportTransData = 1;
                }
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/attribute/all");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if(ret == 0)
    {
        /*modeCap = Common_Json_SetAttrValueArr(outdata, "ModeCapability");
        Common_Json_SetAttrValueArrInt(modeCap, modeCapCount++, 0);*/
        if(Common_Json_GetAttrValueInt(lowerData, "UartMode", &i_num))
        {
            //Common_Json_SetAttrValueArrInt(modeCap, modeCapCount++, 1);
        }
        Common_Json_SetAttrValueInt(outdata, "Mode", i_num);

        if(Common_Json_GetAttrValueInt(lowerData, "BaudRate", &i_num))
        {
            cJSON_Struct *buadrateCap = Common_Json_SetAttrValueArr(outdata, "BaudRateCapability");
            for (i=0; i<sizeof(BaudRate)/sizeof(int); i++)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(buadrateCap, i);
                snprintf(buf, sizeof(buf), "%d", BaudRate[i]);
                Common_Json_SetAttrValueStr(tmp, "T", buf);
                Common_Json_SetAttrValueInt(tmp, "V", i);
                if (i_num == BaudRate[i])
                {
                    Common_Json_SetAttrValueInt(outdata, "BaudRate", i);
                }
            }
        }

        Common_Json_SetAttrValueObj(outdata, "Ptz");
        if(Common_Json_GetAttrValueInt(lowerData, "Address", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Ptz/Address", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "ProtocolIdx", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Ptz/Protocol", i_num);

            cJSON_Struct *tmpdata = NULL;
            Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/protocol");
            ret = Ovfs_Web_RestMethodA(header, NULL, &tmpdata, 0);
            if(ret == 0)
            {
                cJSON_Struct *protocolCap = Common_Json_SetAttrValueArr(outdata, "Ptz/ProtocolCapability");
                cJSON_Struct *list = Common_Json_GetAttrValueArr(tmpdata, "ProtocolList");
                int size = Common_Json_ArraySize(list);
                for(i=0; i<size; i++)
                {
                    cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(protocolCap, i);
                    if(Common_Json_GetAttrValue(list, i, "Name", NULL, &str_tmp, NULL, NULL))
                    {
                        Common_Json_SetAttrValueStr(tmp, "T", str_tmp);
                    }

                    if(Common_Json_GetAttrValue(list, i, "Idx", NULL, NULL, &i_num, NULL))
                    {
                        Common_Json_SetAttrValueInt(tmp, "V", i_num);
                    }
                }
            }

            if(tmpdata)
            {
                Common_Json_Delete(tmpdata);
                tmpdata = NULL;
            }

        }
    }
    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(ret == 0 && bSupportTransData)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/network/netapp/transdata");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if(ret == 0)
        {
            Common_Json_SetAttrValueArrInt(modeCap, modeCapCount++, 2);
            cJSON_Struct *tmpdata = Common_Json_SetAttrValueObj(outdata, "TransChannel");
            if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(tmpdata, "Enable", i_num);
            }

            if(Common_Json_GetAttrValueStr(lowerData, "DestIp", &str_tmp))
            {
                Common_Json_SetAttrValueStr(tmpdata, "DestIp", str_tmp);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "DestPort", &i_num))
            {
                Common_Json_SetAttrValueInt(tmpdata, "DestPort", i_num);
            }

            if(Common_Json_GetAttrValueStr(g_ovfs_web->pDeviceStatus, "Ip", &str_tmp))
            {
                Common_Json_SetAttrValueStr(tmpdata, "SrcIp", str_tmp);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "SrcPort", &i_num))
            {
                Common_Json_SetAttrValueInt(tmpdata, "SrcPort", i_num);
            }


        }

    }

    return ret;
}

static int web_semantic_set_uartconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int src_port = 0;
    int new_port = 0;
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
        if (Common_Json_GetAttrValueInt(indata, "Mode", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "UartMode", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "BaudRate", &i_num))
        {
            if(i_num>=0 && i_num<sizeof(BaudRate)/sizeof(int))
            {
                Common_Json_SetAttrValueInt(lowerData, "BaudRate", BaudRate[i_num]);
            }
        }

        if (Common_Json_GetAttrValueInt(indata, "Ptz/Protocol", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "ProtocolIdx", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "Ptz/Address", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Address", i_num);
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/attribute/all");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    if(ret == 0)
    {
        cJSON_Struct *tmpData = Common_Json_GetAttrValueObj(indata, "TransChannel");
        if(tmpData)
        {
            if(Common_Json_GetAttrValueInt(tmpData, "SrcPort", &new_port))
            {
                Ovfs_Web_UpdateHeader(header, REST_GET, "/network/netapp/transdata");
                ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
                if(ret == 0)
                {
                    Common_Json_GetAttrValueInt(lowerData, "SrcPort", &src_port);
                    if(new_port != src_port)
                    {
                        ret = web_semantic_get_port_occupancy(new_port);
                        LOGD("ret:[%d]\n",ret);
                    }
                }

                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }

            if(ret == 0)
            {
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/network/netapp/transdata");
                ret = Ovfs_Web_RestMethodA(header, tmpData, NULL, 0);
            }
        }
    }

    return ret;
}

static int web_semantic_set_uartconfig_single(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int isDel = 0;
    char *str_tmp = NULL;
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
        cJSON_Struct *pAlarmOut = Common_Json_GetAttrValueObj(indata, "AlarmOut");
        if(pAlarmOut)
        {
            cJSON_Struct *pDev = Common_Json_GetAttrValueObj(pAlarmOut, "Dev");
            cJSON_Struct *pCh = Common_Json_GetAttrValueObj(pAlarmOut, "Ch");
            if(pDev)
            {
                snprintf(url, sizeof(url),"/Ptz/AlarmOut/attribute/dev");
                Common_Json_GetAttrValueInt(pDev, "IsDel", &isDel);

                if (Common_Json_GetAttrValueInt(pDev, "Dev", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "Dev", i_num);
                }

                if (Common_Json_GetAttrValueStr(pDev, "Name", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(lowerData, "Name", str_tmp);
                }

                if (Common_Json_GetAttrValueInt(pDev, "Addr", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "Addr", i_num);
                }

                if (Common_Json_GetAttrValueInt(pDev, "ChNum", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "ChNum", i_num);
                }
            }
            else if(pCh)
            {
                snprintf(url, sizeof(url),"/Ptz/AlarmOut/attribute/ch");
                if (Common_Json_GetAttrValueInt(pDev, "Dev", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "Dev", i_num);
                }

                if (Common_Json_GetAttrValueInt(pDev, "Ch", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "Ch", i_num);
                }

                if (Common_Json_GetAttrValueStr(pDev, "Name", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(lowerData, "Name", str_tmp);
                }

                if (Common_Json_GetAttrValueInt(pDev, "Status", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "Status", i_num);
                }
            }
            else
            {
                ret = WEB_CODE_InvalidArg;
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, isDel?REST_DELETE:REST_PUT, url);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int frmUartConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_uartconfig(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_uartconfig(header, indata, outdata);
            break;
        /*case 2:
            ret = web_semantic_set_uartconfig_single(header, indata, outdata);
            break;*/

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

int web_semantic_ptzstepcontrol(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int cmd = 0;
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
        if (Common_Json_GetAttrValueInt(indata, "Cmd", &cmd) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            if(cmd<1 || cmd>8)
            {
                ret = WEB_CODE_InvalidArg;
            }
            else
            {
                Common_Json_SetAttrValueInt(lowerData, "Type", cmd+50);
            }
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Cmd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int frmPTZStepControl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:

            break;
        case 1:
            ret = web_semantic_ptzstepcontrol(header, indata, outdata);
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

static int alarmlog_get_handle(cJSON_Struct *header, int logtype, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret  = 0;
    int i_num = 0;
    int startTime = 0;
    int endTime = 0;
    char szUrl[64] = {0};
    char *str_tmp = NULL;
    char *strStartTime = NULL;
    char *strEndTime = NULL;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *reslutdata = NULL;

    snprintf(szUrl, sizeof(szUrl), "/EventLog/%sFunction", logtype ? "Alarm" : "Log");

    if (0 == ret)
    {
        if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "MajorType", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "MajorType", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "AlarmType", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "AlarmType", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Sort", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Sort", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "StartTime", &strStartTime) &&
                Common_Json_GetAttrValueStr(indata, "EndTime", &strEndTime))
        {
            LOGW("string -->localDateTime BeginDateTime:[%s] strin --> localDateTime EndDateTime:[%s]\n",strStartTime,strEndTime);
            TimeStr2UnixTime(strStartTime,&startTime);
            TimeStr2UnixTime(strEndTime,&endTime);

            LOGW("string --> BeginDateTime:[%d] strin --> EndDateTime:[%d]\n",startTime,endTime);

            Common_Json_SetAttrValueInt(lowerdata, "StartTime", startTime);
            Common_Json_SetAttrValueInt(lowerdata, "EndTime", endTime);
        }
    }

    if(0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_POST, szUrl);
        ret = Ovfs_Web_RestMethodA(header, lowerdata, &reslutdata, 60000);
    }
    LOGD("ret:[%d] logtype:[%d]\n", ret, logtype);

    if(0 == ret)
    {
        ovfs_print_json(reslutdata);
        if(Common_Json_GetAttrValueInt(reslutdata, "Handle", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Handle", i_num);
        }
        if(Common_Json_GetAttrValueInt(reslutdata, "TotalCount", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "TotalCount", i_num);
        }
    }

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

static int alarmlog_get_result(cJSON_Struct *header, int logtype, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int i = 0;
    int ret  = 0;
    int i_num = 0;
    int iStart = 0;
    int timetype = -1;
    int iPathType = 0; //0-web, 1-sd path
    char szUrl[64] = {0};
    char *str_tmp = NULL;
    cJSON_Struct *reslutdata = NULL;
    cJSON_Struct *lowerdata = NULL;

    snprintf(szUrl, sizeof(szUrl), "/EventLog/%sFunction", logtype ? "Alarm" : "Log");

    if (0 == ret)
    {
        if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "Handle", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Handle", i_num);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
            LOGE("SessionHandle [%d] is error!\n",i_num);
        }
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "StartCount", &iStart))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Start", iStart-1);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
            LOGE("StartCount [%d] is error!\n",i_num);
        }
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "Count", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Count", i_num);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
            LOGE("StopCount [%d] is error!\n",i_num);
        }
    }

    Common_Json_GetAttrValueInt(indata, "PicPathType", &iPathType);

    if(0 == ret)
    {
        ovfs_print_json(lowerdata);
        Ovfs_Web_UpdateHeader(header, REST_GET, szUrl);
        ret = Ovfs_Web_RestMethodA(header, lowerdata, &reslutdata, 60000);
    }
    LOGD("ret:[%d] logtype:[%d]\n", ret, logtype);

    if(0 == ret)
    {
        JsonOper_MergeObj(outdata, reslutdata, 0);
        cJSON_Struct *list = Common_Json_GetAttrValueArr(outdata, "List");
        int size = Common_Json_ArraySize(list);
        for(i=0; i<size; i++)
        {
            Common_Json_SetAttrValue(list, i, "Id", Common_Json_Type_Number, NULL, iStart+i, 0);

            if(Common_Json_GetAttrValue(list, i, "LogTime", NULL, NULL, &i_num, NULL))
            {
                char *pTimeStr = NULL;
                UnixTime2TimeStr(i_num, &pTimeStr);
                Common_Json_SetAttrValue(list, i, "LogTimeStr", Common_Json_Type_String, pTimeStr, 0, 0);
                Common_Free(pTimeStr, __FUNCTION__, __LINE__);
            }

            if(Common_Json_GetAttrValue(list, i, "StartTime", NULL, NULL, &i_num, NULL))
            {
                char *pTimeStr = NULL;
                UnixTime2TimeStr(i_num, &pTimeStr);
                Common_Json_SetAttrValue(list, i, "StartTimeStr", Common_Json_Type_String, pTimeStr, 0, 0);
                Common_Free(pTimeStr, __FUNCTION__, __LINE__);
            }

            if(Common_Json_GetAttrValue(list, i, "EndTime", NULL, NULL, &i_num, NULL))
            {
                if(i_num > 0)
                {
                    char *pTimeStr = NULL;
                    UnixTime2TimeStr(i_num, &pTimeStr);
                    Common_Json_SetAttrValue(list, i, "EndTimeStr", Common_Json_Type_String, pTimeStr, 0, 0);
                    Common_Free(pTimeStr, __FUNCTION__, __LINE__);
                }
                else
                {
                    Common_Json_SetAttrValue(list, i, "EndTimeStr", Common_Json_Type_String, "", 0, 0);
                }
            }

            if(Common_Json_GetAttrValue(list, i, "AlarmPic", NULL, &str_tmp, NULL, NULL))
            {
                if(slen(str_tmp)>0)
                {
                    char path[128] = {0};
                    snprintf(path, sizeof(path),"%s/AlarmPic/%s", iPathType?"/tmp/mmc/mmc1/log":"", str_tmp);
                    Common_Json_SetAttrValue(list, i, "AlarmPic", Common_Json_Type_String, path, 0, 0);
                }
            }
        }
    }
    else if(-11 == ret)
    {
        ret = WEB_CODE_HandleFreed;
    }

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

static int alarmlog_delete_handle(cJSON_Struct *header, int logtype, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret  = 0;
    int i_num = 0;
    char szUrl[64] = {0};
    cJSON_Struct *lowerdata = NULL;

    snprintf(szUrl, sizeof(szUrl), "/EventLog/%sFunction", logtype ? "Alarm" : "Log");
    if (0 == ret)
    {
        if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "Handle", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Handle", i_num);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
            LOGE("SessionHandle [%d] is error!\n",i_num);
        }
    }

    if(0 == ret)
    {
        ovfs_print_json(lowerdata);
        Ovfs_Web_UpdateHeader(header, REST_DELETE, szUrl);
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 60000);
    }
    LOGD("ret:[%d] logtype:[%d]\n", ret, logtype);

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

int get_log(int logtype, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
	int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
            case 0:
                ret = alarmlog_get_handle(header, logtype, indata, outdata);
                break;
            case 1:
                ret = alarmlog_get_result(header, logtype, indata, outdata);
                break;
            case 2:
                ret = alarmlog_delete_handle(header, logtype, indata, outdata);
                break;
            default:
                ret = WEB_CODE_InvalidArg;
                break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 2)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int frmUsageLog(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    return get_log(0, opt, header, indata, outdata);
}

int frmAlarmLog(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
	return get_log(1, opt, header, indata, outdata);
}
