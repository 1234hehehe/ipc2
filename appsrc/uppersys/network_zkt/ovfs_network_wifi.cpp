#include <common_json_str_ops.h>
#include "ovfs_comm_tool.h"
#include "ovfs_network_wifi.h"
#include "ovfs_network_rest_common.h"
#include "ovfs_wifi.h"

using namespace wifi;

WiFi g_Wifi;

int Call_SpeakInfo(char *source)
{
	S32 iRet = 0;
	S32 language = 1;
	char path[256];
	cJSON_Struct *pInData = NULL,*pOutData = NULL;
	
	pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pInData != NULL)
	{
        int trycount = 20;
		Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/Alarm/SoundAlarmCfg",0,0);
		Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
		Common_Json_SetAttrValue(pInData,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		
		while(--trycount)
		{
            pOutData = NULL;
            iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
            if(0 == iRet) 
            {
                Common_Json_GetAttrValue(pOutData, -1, "Header/Code", NULL, NULL, &iRet, NULL);
                Common_Json_GetAttrValue(pOutData, -1, "Data/Language", NULL, NULL, &language, NULL);
                
                if(0 == iRet)
                    break;
                else
                {
                    Common_Sleep(0, 1000 * 1000);
                }
            }
            else
            {
                Common_Sleep(0, 1000 * 1000);
            }
            //LOGD("iRet=%d\n",iRet);
		}
        if(trycount == 0) 
        {
            iRet = -1;
            LOGE("get Language failed! language=%d\n",language);
        }
        else
        {
#if 0
            char* out = NULL;
            printf("[pOutData] tree:=%s\n",out = Common_Json_Print(pOutData,NULL));
            if(out)
                Common_Free(out,__FUNCTION__,__LINE__);
#endif
        }
        
        Common_Json_Delete(pOutData);
		pOutData = NULL;
		LOGD("trycount=%d,language=%d\n",20-trycount,language);
	
		Common_Json_SetAttrValue(pInData,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInData,-1,"Header/Uri",Common_Json_Type_String,"/Boardsys/Audio/Adec/PlayFile",0,0);
		Common_Json_SetAttrValue(pInData,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
		Common_Json_SetAttrValue(pInData,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		if(language == 0)
			sprintf(path, "/update/soundFile/%s", source);
		else if(language == 1)
			sprintf(path, "/update/soundFile/%s_en", source);
        Common_Json_SetAttrValue(pInData,-1,"Data/Path",Common_Json_Type_String,path,0,0);

        trycount = 5;
        while(--trycount)
		{
            iRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
            Common_Json_GetAttrValue(pOutData, -1, "Header/Code", NULL, NULL, &iRet, NULL);
            if(0 == iRet)
                break;
           else
                Common_Sleep(0, 1000 * 1000);
		}
        if(trycount == 0) 
        {
            iRet = -1;
            LOGE("PlayFile failed! source=%s\n",source);
        }
		Common_Json_Delete(pInData);
		Common_Json_Delete(pOutData);
	}

	return 0;
}


int NetWork_Wifi_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
    int ret = 0;
    int work_mode = -1;
    WIFI_AP_STATUS ap_cfg;
    cJSON_Struct *pJsonTmp = NULL;
    cJSON_Struct *pJsonTmpDefault = NULL;
    
    pJsonTmp = Common_Json_GetItem(pJson, -1, "NetAttr/Wifi");
    if (NULL == pJsonTmp)
    {
       LOGW("[%s:%d]:get Wifi info fail\n",__FUNCTION__, __LINE__);
    }
    
    pJsonTmpDefault = Common_Json_GetItem(pJsonDefault, -1, "NetAttr/Wifi");
    if (NULL == pJsonTmpDefault)
    {
       LOGW("[%s:%d]:Default get Wifi info fail\n",__FUNCTION__, __LINE__);
    }

    if ((NULL == pJsonTmp) && (NULL != pJsonTmpDefault))
    {
        Common_Json_GetAttrValue(pJsonTmpDefault,-1,"WorkMode",NULL,NULL,(S32 *)&work_mode,NULL);
        if (-1 != work_mode)
        {
            ret = -1;
        }
        else
        {
            g_Wifi.writeConfig(pJsonTmpDefault);
            g_Wifi.init();
            //g_Wifi.setConfig(pJsonTmpDefault);
        }
    }
    else if(NULL != pJsonTmp)
    {
        Common_Json_GetAttrValue(pJsonTmp,-1,"WorkMode",NULL,NULL,(S32 *)&work_mode,NULL);
        if (-1 == work_mode)
        {
            ret = -2;
        }
        else
        {
            g_Wifi.writeConfig(pJsonTmp);
            g_Wifi.init();
            //g_Wifi.setConfig(pJsonTmp);
        }
    }
    else
    {
        ret = -3;
    }
    LOGI("ret=%d\n",work_mode);
    
    if(0 == ret)
	{
		if((work_mode >= AP) && (work_mode <= STA))
		{
        	Common_System("ifconfig wlan0 up");
        	g_Wifi.setParam(SWITCH_MODE,(WIFI_WORK_MODE)work_mode);
		}
	}
	else if(0 == NetWorkTool_GetIFFlags((char *)"wlan0"))
	{
	    char mac[32];
        NetWorkTool_GetMacAddr((char *)"wlan0", mac);
	    char *pSerial = Module_GetSerialNumber(0);
	    if (NULL != pSerial)
        {
            snprintf(ap_cfg.ssid, sizeof(ap_cfg.ssid), "IPC-AP-%c%c%c%c-%02x%02x",*(pSerial),*(pSerial+1),*(pSerial+2),*(pSerial+3),mac[4], mac[5]);
        }
        else
        {
            snprintf(ap_cfg.ssid, sizeof(ap_cfg.ssid), "IPC-AP-0000-%02x%02x",mac[4], mac[5]);
        }
        snprintf(ap_cfg.mac, sizeof(ap_cfg.mac), "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4], mac[5]);
        snprintf(ap_cfg.psk, sizeof(ap_cfg.psk), "12345678");
        snprintf(ap_cfg.ipv4, sizeof(ap_cfg.ipv4), "192.168.10.1");
        ap_cfg.auth_type = AUTH_MODE_WPA2;
        g_Wifi.setHostap(ap_cfg);
        g_Wifi.init();
        g_Wifi.setParam(SWITCH_MODE,AP);
	}
	else
	{
		return -4;
	}

    //Common_Timer_T timer = NULL;
    //Common_Timer_Create(&timer,4000,CheckSwitchWorkMode,NULL);
    
    return 0;
}

int NetWork_Get_WifiWorkMode_Json(cJSON_Struct *parentItem)
{
    LOGD("NetWork_Get_WifiWorkMode_Json\n");
    g_Wifi.getCurrentMode(parentItem);
#if 0
    char* out = NULL;
    LOGI("[Status] tree:=%s\n",out = Common_Json_Print(parentItem,NULL));
    if(out)
        Common_Free(out,__FUNCTION__,__LINE__);
#endif
    return 0;
}

int NetWork_Config_STA_Json(cJSON_Struct *parentItem)
{
    int ret = 0;
    LOGD("NetWork_Config_STA_Json\n");
#if 0
        char* out = NULL;
        LOGI("%s\n",out = Common_Json_Print(parentItem,NULL));
        if(out)
            Common_Free(out,__FUNCTION__,__LINE__);
#endif
    g_Wifi.connect(parentItem);
    ret |= g_Wifi.setStatus(CONFIG_STA);
    Call_SpeakInfo((char *)"wifi_connecting");
    return ret;
}

int NetWork_Get_WifiWorkStatus_Json(cJSON_Struct *parentItem)
{
    LOGD("NetWork_Get_WifiWorkStatus_Json\n");
    g_Wifi.getStatus(parentItem);
#if 0
    char* out = NULL;
    LOGI("%s\n",out = Common_Json_Print(parentItem,NULL));
    if(out)
        Common_Free(out,__FUNCTION__,__LINE__);
#endif
    return 0;
}

int NetWork_Put_WifiWorkStatus_Json(cJSON_Struct *parentItem)
{
    int ret = -1,nIntValue = -1;
#if 0
    char* out = NULL;
    LOGI("[Status] tree:=%s\n",out = Common_Json_Print(parentItem,NULL));
    if(out)
        Common_Free(out,__FUNCTION__,__LINE__);
#endif
    Common_Json_GetAttrValue(parentItem,-1,"WorkMode",NULL,NULL,&nIntValue,NULL);
    if(-1 != nIntValue)
    {
        ret = g_Wifi.setParam(SWITCH_MODE,(WIFI_WORK_MODE)nIntValue);
    }
    return ret;
}

int NetWork_Get_WifiSTAList_Json(cJSON_Struct *parentItem)
{
    LOGD("NetWork_Get_WifiSTAList_Json\n");
    return g_Wifi.getSTAList(parentItem);
}

int NetWork_Get_WifiAPList_Json(cJSON_Struct *parentItem)
{
    LOGD("NetWork_Get_WifiAPList_Json\n");
    return g_Wifi.getAPList(parentItem);
}

int NetWork_Get_WifiScanList_Json(cJSON_Struct *parentItem)
{
    LOGD("NetWork_Get_WifiScanList_Json\n");
    int ret = g_Wifi.getScanList(parentItem);
    return ret;
}

int NetWork_Put_AddAP_Json(cJSON_Struct *parentItem)
{
    int ret = -1;
    LOGD("NetWork_Put_AddAP_Json\n");
    
    if(g_Wifi.addAP(parentItem))
    {
        ret = g_Wifi.setStatus(STA_ADDAP);
    }

    return ret;
}

int NetWork_Put_DelAP_Json(cJSON_Struct *parentItem)
{
    int ret = -1;
    LOGD("NetWork_Put_DelAP_Json\n");

    if(g_Wifi.delAP(parentItem))
    {
        ret = g_Wifi.setStatus(STA_DELAP);
    }
    return ret;
}


