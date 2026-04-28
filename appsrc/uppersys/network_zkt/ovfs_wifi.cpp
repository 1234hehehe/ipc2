#include "ovfs_wifi.h"

namespace wifi {


int Wifi_CfgStructToCfgJson(void *cfgStruct, cJSON_Struct *cfgJson,WIFI_STU_TYPE type)
{
    int ret = 0;

    if(NULL == cfgJson)
        return -1;
    if(type == AP_STATUS)
    {
        WIFI_AP_STATUS *ap_status = (WIFI_AP_STATUS *)cfgStruct;
        Common_Json_SetAttrValue(cfgJson, -1, "APSSID", Common_Json_Type_String,ap_status->ssid, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "APPSK", Common_Json_Type_String,ap_status->psk, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "APIP", Common_Json_Type_String,ap_status->ipv4, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "APMac", Common_Json_Type_String,ap_status->mac, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "APAuthType", Common_Json_Type_Number,0,ap_status->auth_type, 0);
    }
    else if(type == STA_STATUS)
    {
        WIFI_STA_STATUS *sta_status = (WIFI_STA_STATUS *)cfgStruct;
        Common_Json_SetAttrValue(cfgJson, -1, "STASSID", Common_Json_Type_String,sta_status->ssid, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "STAIP", Common_Json_Type_String,sta_status->ipv4addr, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "STAStatus", Common_Json_Type_Number,0, sta_status->Status, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "STASignal", Common_Json_Type_Number,0,sta_status->signal, 0);
    }
    else if(type == STA_STORE_LIST)
    {
        WIFI_STA_APAUTHCFG_S *ap_list = (WIFI_STA_APAUTHCFG_S *)cfgStruct;
        Common_Json_SetAttrValue(cfgJson, -1, "Ssid", Common_Json_Type_String,ap_list->ssid, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "Psk", Common_Json_Type_String,ap_list->psk, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "Mac", Common_Json_Type_String,ap_list->mac, 0, 0);
        Common_Json_SetAttrValue(cfgJson, -1, "AuthType", Common_Json_Type_Number,0,ap_list->auth_type, 0);
    }
#if 0
    char* out = NULL;
    LOGI("[Status] tree:=%s\n",out = Common_Json_Print(cfgJson,NULL));
    if(out)
        Common_Free(out,__FUNCTION__,__LINE__);
#endif
    return ret;
}

int Wifi_CfgJsonToCfgStruct(cJSON_Struct *cfgJson,void *cfgStruct,WIFI_STU_TYPE type)
{
    int ret = 0;
    int retInt = 0;
    char *retStr = NULL;
    if(type == AP_STATUS)
    {
        WIFI_AP_STATUS *cfg = (WIFI_AP_STATUS *)cfgStruct;
        memset(cfg,0,sizeof(WIFI_AP_STATUS));
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"APSSID",NULL,&retStr,NULL,NULL);
        if(retStr != NULL)
        {
            snprintf(cfg->ssid, sizeof(cfg->ssid), "%s", retStr); 
        }
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"APPSK",NULL,&retStr,NULL,NULL);
        if(retStr != NULL)
        {
            snprintf(cfg->psk, sizeof(cfg->psk), "%s", retStr); 
        }
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"APIP",NULL,&retStr,NULL,NULL);
        if(retStr != NULL)
        {
            snprintf(cfg->ipv4, sizeof(cfg->ipv4), "%s", retStr); 
        }
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"APMac",NULL,&retStr,NULL,NULL);
        if(retStr != NULL)
        {
            snprintf(cfg->mac, sizeof(cfg->mac), "%s", retStr); 
        }
        retInt = -1;
        Common_Json_GetAttrValue(cfgJson,-1,"APAuthType",NULL,NULL,&retInt,NULL);
        if(retInt != -1)
        {
            cfg->auth_type = retInt;
        }
    }
    else if(type == STA_STATUS)
    {
        WIFI_STA_CONNECTIONCFG_S *cfg = (WIFI_STA_CONNECTIONCFG_S *)cfgStruct;
         memset(cfg,0,sizeof(WIFI_STA_CONNECTIONCFG_S));
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"SSID",NULL,&retStr,NULL,NULL);
        if (NULL != retStr)
        {
            snprintf(cfg->authCfg.ssid, sizeof(cfg->authCfg.ssid), "%s", retStr);
        }
        
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"PSK",NULL,&retStr,NULL,NULL);
        if (NULL != retStr)
        {
            snprintf(cfg->authCfg.psk, sizeof(cfg->authCfg.psk), "%s", retStr);
        }
        
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"Mac",NULL,&retStr,NULL,NULL);
        if (NULL != retStr)
        {
            snprintf(cfg->authCfg.mac, sizeof(cfg->authCfg.mac), "%s", retStr);
        }
        
        retInt = -1;
        Common_Json_GetAttrValue(cfgJson,-1,"AuthType",NULL,NULL,&retInt,NULL);
        if (-1 != retInt)
        {
            cfg->authCfg.auth_type = retInt;
        }
        
        retInt = -1;
        Common_Json_GetAttrValue(cfgJson,-1,"IsDhcp",NULL,NULL,&retInt,NULL);
        if (-1 != retInt)
        {
            cfg->ipAddrCfg.bDhcp = retInt;
        }   
        
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"IP",NULL,&retStr,NULL,NULL);
        if (NULL != retStr)
        {
            snprintf(cfg->ipAddrCfg.ipv4, sizeof(cfg->ipAddrCfg.ipv4), "%s", retStr);
        }
        
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"NetMask",NULL,&retStr,NULL,NULL);
        if (NULL != retStr)
        {
            snprintf(cfg->ipAddrCfg.netmask, sizeof(cfg->ipAddrCfg.netmask), "%s", retStr);
        }
        
        retStr = NULL;
        Common_Json_GetAttrValue(cfgJson,-1,"GateWay",NULL,&retStr,NULL,NULL);
        if (NULL != retStr)
        {
            snprintf(cfg->ipAddrCfg.gateway, sizeof(cfg->ipAddrCfg.gateway), "%s", retStr);
        }
    }
    return ret;
}


void * WiFi::Wifi_Manager(void *__this)
{
    prctl(PR_SET_NAME, "Wifi_Manager", 0, 0, 0);
    WiFi * wifi =(WiFi *)__this;
    WIFI_WORK_STATUS msg = IDEL;
	WIFI_WORK_STA_STATUS_E satStatus;
    WIFI_WORK_MODE CurMode=CLOSE,WorkMode=CLOSE;
    if(wifi == NULL)
        return NULL;
    CurMode = wifi->getCurrentMode();
    LOGI("prhread Init OK!CurWorkMode=%d\n",CurMode);
    while(1)
    {
        Common_RWLock_RLock(wifi->rw_opt_lock);
        WorkMode = wifi->getCurrentMode();
        msg = wifi->getCurrentStatus();
        Common_RWLock_UnLock(wifi->rw_opt_lock);
        Common_Sleep(0, 200 * 1000);
        switch(msg)
        {
            case SWITCH_MODE:
                if(CurMode != WorkMode)
                {
                    wifi->change(CurMode,WorkMode);
                    CurMode = WorkMode;
                }
                break;
            case CONFIG_STA:
                wifi->isConfigSTA = true;
                if(wifi->change(AP,STA))
                {
                    if(wifi->isReady())
                    {
                        if(wifi->autoconnect())
                        {
                            CurMode = STA;
                        }
                    }
                }
                break;
            case STA_ADDAP:
            case STA_DELAP:
                if(CurMode == STA)
                {
                    if(wifi->isReady())
                    {
                        wifi->autoconnect();
                    }
                }
                break;
            case STA_SCAN:
                if(CurMode == STA)
                {
                    wifi->scan();
                }
                break;
            case STA_SCAN_END:
                if(CurMode == STA)
                {
                    wifi->setStatus(IDEL);
                }
                break;
            case IDEL:
                Common_Sleep(0, 1000 * 1000);
                if(wifi->isReady())
                {
                    satStatus = wifi->UpdateStatus();
					//LOGD("satStatus=%d\n",satStatus);
					if((WIFI_WORK_STA_STATUS_FAILED == satStatus) && (CurMode == STA))
					{
						wifi->wpactrl_err_cnt++;
						LOGE("wpactrl_err_cnt=%d\n",wifi->wpactrl_err_cnt);
						if(wifi->wpactrl_err_cnt > 20)
							wifi->Restart();
					}
                }
                break;
        }
    }
}

WiFi::WiFi()
{
    isConfigSTA = false;
    rw_opt_lock = NULL;
    mode = CLOSE;
    status = IDEL;
    scanlist = NULL;
    scanlistcount = 0;
	wpactrl_err_cnt = 0;
	wifi_running = false;
}

WiFi::~WiFi()
{
	wifi_running = false;
    if(scanlist != NULL)
        FREE(scanlist);
}

void WiFi::init()
{
    Common_RWLock_Create(&rw_opt_lock,NULL);
	if(wifi_running) return;
    int err = pthread_create(&wifi_workThread,NULL,Wifi_Manager,(void*)this);
    if(err!=0)
    {
        Common_RWLock_Destroy(&rw_opt_lock);
        LOGD("thread_create Failed:%s\n",strerror(errno));
    }
    else
    {
        LOGI("thread_create success\n");
    }
    sleep(1);
}

bool WiFi::setHostap(WIFI_AP_STATUS cfg)
{
    bool ret = false;
    if(ap.checkParam(cfg))
        ret = ap.writeConfig(cfg);
    if(ret)
    {
        cJSON_Struct *cfgJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(cfgJson,-1,"WorkMode",Common_Json_Type_Number,NULL,AP,0);
        cJSON_Struct *ap_cfgJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        Wifi_CfgStructToCfgJson(&cfg,ap_cfgJson,AP_STATUS);
        Common_Json_AddItem(cfgJson,-1,"AP",ap_cfgJson);
        cJSON_Struct *sta_cfgJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_AddItem(cfgJson,-1,"STA",sta_cfgJson);
#if 0
        char* out = NULL;
        LOGI("[Status] tree:=%s\n",out = Common_Json_Print(cfgJson,NULL));
        if(out)
            Common_Free(out,__FUNCTION__,__LINE__);
#endif
        ret = writeConfig(cfgJson);
		if(cfgJson != NULL)
        	Common_Json_Delete(cfgJson);
    }
    return ret;
}

int WiFi::setStatus(WIFI_WORK_STATUS status)
{
    Common_RWLock_WLock(rw_opt_lock);
    this->status = status;
    Common_RWLock_UnLock(rw_opt_lock);
    return 0;
}

int WiFi::setParam(WIFI_WORK_STATUS status,WIFI_WORK_MODE mode)
{
    Common_RWLock_WLock(rw_opt_lock);
    this->status = status;
    this->mode = mode;
    Common_RWLock_UnLock(rw_opt_lock);
    return 0;
}

bool WiFi::change(WIFI_WORK_MODE current_mode,WIFI_WORK_MODE set_mode)
{
    bool ret = false;
    LOGD("change CurMode[%d] to Mode[%d]:%d\n",current_mode,set_mode);
    if(current_mode == AP)
    {
        if(set_mode == STA)
        {
            ret |= ap.stop();
            ret |= sta.start();
            mode = STA;
			wpactrl_err_cnt = 0;
        }
        else if(set_mode == CLOSE)
        {
            ret |= ap.stop();
            mode = CLOSE;
        }
        else
        {
            ret |= ap.stop();
            ret |= ap.start();
			wpactrl_err_cnt = 0;
        }
    }
    else if(current_mode == STA)
    {
        if(set_mode == AP)
        {
            ret |= sta.stop();
            ret |= ap.start();
            mode = AP;
        }
        else if(set_mode == CLOSE)
        {
            ret |= sta.stop();
            mode = CLOSE;
        }
        else
        {
            ret |= sta.stop();
            ret |= sta.start();
			wpactrl_err_cnt = 0;
        }
    }
    else
    {
        if(set_mode == AP)
        {
            ret |= ap.start();
            ret |= mode = AP;
        }
        else if(set_mode == STA)
        {
            ret |= sta.start();
            ret |= mode = STA;
			wpactrl_err_cnt = 0;
        }
    }
    
    if(ret)
        this->status = IDEL;
    return ret;
}

void WiFi::Restart(void)
{
	if(STA == this->mode)
    {
        sta.stop();
		sta.start();
		wpactrl_err_cnt = 0;
    }
}

bool WiFi::connect(cJSON_Struct *cfgJson)
{
    int retInt = 0;
    char *retStr = NULL;
    cJSON_Struct *  pTmpObj;
    cJSON_Struct *jsonCfg = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    pTmpObj = Common_Json_GetAttrValue(cfgJson,-1,"ModifyAp",NULL,NULL,NULL,NULL);
    if (pTmpObj != NULL)
    {
        pTmpObj = Common_Json_GetAttrValue(cfgJson,-1,"ModifyAp/STACfg",NULL,NULL,NULL,NULL);
        if (pTmpObj != NULL)
        {
            retStr = NULL;
            Common_Json_GetAttrValue(pTmpObj,-1,"Ssid",NULL,&retStr,NULL,NULL);
            if(retStr != NULL)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"SSID",Common_Json_Type_String,retStr,0,0);
            }
            retStr = NULL;
            Common_Json_GetAttrValue(pTmpObj,-1,"Psk",NULL,&retStr,NULL,NULL);
            if(retStr != NULL)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"PSK",Common_Json_Type_String,retStr,0,0);
            }
            retStr = NULL;
            Common_Json_GetAttrValue(pTmpObj,-1,"Mac",NULL,&retStr,NULL,NULL);
            if(retStr != NULL)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"MAC",Common_Json_Type_String,retStr,0,0);
            }
            retInt = -1;
            Common_Json_GetAttrValue(pTmpObj,-1,"AuthType",NULL,NULL,&retInt,NULL);
            if(retInt != -1)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"AuthType",Common_Json_Type_Number,NULL,retInt,0);
            }
        }
        pTmpObj = Common_Json_GetAttrValue(cfgJson,-1,"ModifyAp/IpAddrCfg",NULL,NULL,NULL,NULL);
        if (pTmpObj != NULL)
        {
            retInt = -1;
            Common_Json_GetAttrValue(pTmpObj,-1,"Dhcp",NULL,NULL,&retInt,NULL);
            if (retInt == 0 || retInt == 1)
                Common_Json_SetAttrValue(jsonCfg,-1,"IsDhcp",Common_Json_Type_Number,NULL,retInt,0);

            retStr = NULL;
            Common_Json_GetAttrValue(pTmpObj,-1,"IP",NULL,&retStr,NULL,NULL);
            if(retStr != NULL)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"IP",Common_Json_Type_String,retStr,0,0);
            }

            retStr = NULL;
            Common_Json_GetAttrValue(pTmpObj,-1,"NetMask",NULL,&retStr,NULL,NULL);
            if(retStr != NULL)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"NetMask",Common_Json_Type_String,retStr,0,0);
            }

            retStr = NULL;
            Common_Json_GetAttrValue(pTmpObj,-1,"GateWay",NULL,&retStr,NULL,NULL);
            if(retStr != NULL)
            {
                Common_Json_SetAttrValue(jsonCfg,-1,"GateWay",Common_Json_Type_String,retStr,0,0);
            }

        }
    }
    addAP(jsonCfg);
    if(jsonCfg != NULL)
        Common_Json_Delete(jsonCfg);
    return true;
}

bool WiFi::update()
{
    bool ret = false;
    if(STA == this->mode)
    {
        sta.update();
    }
    return ret;
}

bool WiFi::writeConfig(cJSON_Struct *jsonCfg)
{
    bool ret = false;
    WIFI_AP_STATUS ap_cfg;
    cJSON_Struct *ap_config = Common_Json_GetItem(jsonCfg,-1,"AP");
    if(ap_config != NULL)
    {
        ap.writeConfig(ap_config);
        Wifi_CfgJsonToCfgStruct(ap_config,&ap_cfg,AP_STATUS);
        if(ap.checkParam(ap_cfg))
            ap.writeConfig(ap_cfg);
    }
    cJSON_Struct *sta_config = Common_Json_GetItem(jsonCfg,-1,"STA");
    if(sta_config != NULL)
        sta.writeConfig(sta_config);
    return ret;
}

WIFI_WORK_STATUS WiFi::getCurrentStatus(void)
{
    return this->status;
}

WIFI_WORK_MODE WiFi::getCurrentMode(void)
{
    return this->mode;
}

WIFI_WORK_STA_STATUS_E WiFi::GetConnStatus(void)
{
    return this->sta.GetConnStatus();
}


WIFI_WORK_STA_STATUS_E WiFi::UpdateStatus(void)
{
    return this->sta.UpdateStatus();
}

WIFI_WORK_MODE WiFi::getCurrentMode(cJSON_Struct *jsonCfg)
{
    Common_Json_SetAttrValue(jsonCfg,-1,"WorkMode",Common_Json_Type_Number,NULL,(S32)mode,0);
    cJSON_Struct *ap_jsonCfg = NULL;
    ap.readConfig(&ap_jsonCfg);
    Common_Json_AddItem(jsonCfg,-1,"AP",ap_jsonCfg);
    cJSON_Struct *sta_jsonCfg = NULL;
    sta.readConfig(&sta_jsonCfg);
    Common_Json_AddItem(jsonCfg,-1,"STA",sta_jsonCfg);
#if 0
	char* out = NULL;
	LOGI("%s\n",out = Common_Json_Print(jsonCfg,NULL));
	if(out)
		Common_Free(out,__FUNCTION__,__LINE__);
#endif

    return this->mode;
}


bool WiFi::getStatus(cJSON_Struct *status)
{
    bool ret = false;
    WIFI_WORK_MODE curMode = getCurrentMode();
    Common_Json_SetAttrValue(status,-1,"WorkMode",Common_Json_Type_Number,NULL,(S32)curMode,0);
    if(AP == curMode)
    {
        ap.getCurStatus(status);
    }
    else if(STA == curMode)
    {
        sta.getCurStatus(status);
    }
    return ret;
}

bool WiFi::isReady()
{
    if(mode == AP)
        return ap.isReady();
    else if(mode == STA)
        return sta.isReady();
    else
        return false;
}

bool WiFi::autoconnect()
{
    bool ret = false;
    if(mode != STA)
        return false;
    ret = sta.autoconnect();
    if(ret)
        status = IDEL;
    return ret;
}

int WiFi::addAP(cJSON_Struct *parentItem)
{
    int i=0,ret = -1;
    int match_id=-1,priority=0,is_same_one=0;
    int retInt = -1,retIntItem = -1;
    char *retStr = NULL,*retStrItem = NULL;
    Common_RWLock_WLock(rw_opt_lock);
    cJSON_Struct *sta_jsonCfg = NULL;
    sta.readConfig(&sta_jsonCfg);
    cJSON_Struct *pArray = Common_Json_GetItem(sta_jsonCfg,-1,"APList");
    int apListCount = Common_Json_ArraySize(pArray);
    LOGD("apListCount=%d\n",apListCount);
#if 0
        char* out = NULL;
        LOGI("%s\n",out = Common_Json_Print(parentItem,NULL));
        if(out)
            Common_Free(out,__FUNCTION__,__LINE__);
#endif

    for (i = 0; i<apListCount; i++)
    {
        retStr = NULL,retStrItem = NULL;
        Common_Json_GetAttrValue(pArray,i,"SSID",NULL,&retStrItem,NULL,NULL);
        Common_Json_GetAttrValue(parentItem,-1,"SSID",NULL,&retStr,NULL,NULL);
        if((retStr != NULL) && (retStrItem != NULL))
        {
            LOGD("Add SSID={%s},List SSID={%s}\n",retStr,retStrItem);
            if(strcmp(retStr,retStrItem) == 0)
            {
                match_id = i;
                is_same_one = 1;
            }
        }
		Common_Json_GetAttrValue(pArray,i,"Select",NULL,NULL,&retIntItem,NULL);
		if(retIntItem != -1)
		{
			if(retIntItem > priority)
			{
				priority = retIntItem;
			}
		}
		
    }
	LOGD("match_id=%d,priority=%d\n",match_id,priority);
	for (i = 0; i<apListCount; i++)
	{
		Common_Json_GetAttrValue(pArray,i,"Select",NULL,NULL,&retIntItem,NULL);
		if(retIntItem != -1)
		{
			LOGD("Select=%d\n",retIntItem);
			if(match_id == i)
				Common_Json_SetAttrValue(pArray, i, "Select", Common_Json_Type_Number,NULL, apListCount, 0);
			else if(retIntItem == priority)
				Common_Json_SetAttrValue(pArray, i, "Select", Common_Json_Type_Number,NULL, apListCount-1, 0);
			else
				Common_Json_SetAttrValue(pArray, i, "Select", Common_Json_Type_Number,NULL, 1, 0);
		}
	}
	
	if(apListCount == 0)//如果列表为空
	{
		pArray = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
		cJSON_Struct *addItem = Common_Json_Duplicate(parentItem,1);
		Common_Json_SetAttrValue(addItem,-1,"Select",Common_Json_Type_Number,NULL,apListCount+1,0);
		Common_Json_AddItem(pArray,apListCount,NULL,addItem);
		Common_Json_AddItem(sta_jsonCfg,-1,"APList",pArray);
	}
    else if(0 == is_same_one)//如果列表中没有相同的配置，则添加一项
    {
        retStr = NULL;
        Common_Json_GetAttrValue(parentItem,-1,"SSID",NULL,&retStr,NULL,NULL);
        if(retStr != NULL)
        {
        	cJSON_Struct *addItem = Common_Json_Duplicate(parentItem,1);
            Common_Json_SetAttrValue(addItem,-1,"Select",Common_Json_Type_Number,NULL,apListCount+1,0);
            Common_Json_AddItem(pArray,apListCount,NULL,addItem);
        }
    }
	else//如果在列表里面有相同的，则覆盖配置
	{
		retStr = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"PSK",NULL,&retStr,NULL,NULL);
		if(retStr != NULL)
		{
			Common_Json_SetAttrValue(pArray, match_id, "PSK", Common_Json_Type_String,retStr, 0, 0);
		}
		retInt = -1;
		Common_Json_GetAttrValue(parentItem,-1,"IsDhcp",NULL,NULL,&retInt,NULL);
		if(retInt != -1)
		{
			Common_Json_SetAttrValue(pArray, match_id, "IsDhcp", Common_Json_Type_Number,NULL, retInt, 0);
		}
		retStr = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"IP",NULL,&retStr,NULL,NULL);
		if(retStr != NULL)
		{
			Common_Json_SetAttrValue(pArray, match_id, "IP", Common_Json_Type_String,retStr, 0, 0);
		}
		retStr = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"NetMask",NULL,&retStr,NULL,NULL);
		if(retStr != NULL)
		{
			Common_Json_SetAttrValue(pArray, match_id, "NetMask", Common_Json_Type_String,retStr, 0, 0);
		}
		retStr = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"GateWay",NULL,&retStr,NULL,NULL);
		if(retStr != NULL)
		{
			Common_Json_SetAttrValue(pArray, match_id, "GateWay", Common_Json_Type_String,retStr, 0, 0);
		}
	}
    sta.writeConfig(sta_jsonCfg);
	if(sta_jsonCfg)
		Common_Json_Delete(sta_jsonCfg);
    Common_RWLock_UnLock(rw_opt_lock);
    return ret;
}

int WiFi::delAP(cJSON_Struct *parentItem)
{
    int ret = -1;
	int retIntItem = -1,priority=0,priority_id=-1,match_id=-1;
    char *tmpCfg = NULL,*delcfg = NULL;
    cJSON_Struct *tmpObj =NULL;
    
    Common_RWLock_WLock(rw_opt_lock);
    cJSON_Struct *sta_jsonCfg = NULL;
    sta.readConfig(&sta_jsonCfg);
    cJSON_Struct *pArray = Common_Json_GetItem(sta_jsonCfg,-1,"APList");
    int apListCount = Common_Json_ArraySize(pArray);
    LOGD("apListCount=%d\n",apListCount);
    for(int i=0;i<apListCount;i++)
    {
        Common_Json_GetAttrValue(parentItem,-1,"SSID",NULL,&delcfg,NULL,NULL);
        tmpObj = Common_Json_GetItem(pArray,i,NULL);
        if(tmpObj != NULL)
        {
            Common_Json_GetAttrValue(tmpObj,-1,"SSID",NULL,&tmpCfg,NULL,NULL);
            if((delcfg != NULL) && (tmpCfg != NULL))
            {
                if (strcmp(delcfg, tmpCfg) == 0)
                {
                	match_id = i;
                }
            }
			Common_Json_GetAttrValue(pArray,i,"Select",NULL,NULL,&retIntItem,NULL);
			if(retIntItem != -1)
			{
				if(retIntItem > priority)
				{
					priority = retIntItem;
					priority_id = i;
				}
			}
        }
    }

	LOGD("priority=%d,priority_id=%d\n",priority,priority_id);
	
	if(match_id != -1)
	{
		if(match_id != priority_id)
		{
			Common_Json_SetAttrValue(pArray, priority_id, "Select", Common_Json_Type_Number,NULL, apListCount-1, 0);
		}
		Common_Json_RemoveItem(pArray,match_id,NULL);
	}

    sta.writeConfig(sta_jsonCfg);
	if(sta_jsonCfg != NULL)
		Common_Json_Delete(sta_jsonCfg);
    Common_RWLock_UnLock(rw_opt_lock);
    return ret;
}

int WiFi::getSTAList(cJSON_Struct *parentItem)
{
    if(getCurrentMode() != AP)
        return -1;
    else
        return ap.getConnectList(parentItem);
}

int WiFi::getAPList(cJSON_Struct *parentItem)
{
    if(getCurrentMode() != STA)
        return -1;
    else
        return sta.getlist(parentItem);
}

bool WiFi::scan()
{
	int trycount = 1;
	scanlistcount = 0;
	do
	{
		Common_RWLock_WLock(rw_opt_lock);
		sta.scan(&scanlist, &scanlistcount);
		Common_RWLock_UnLock(rw_opt_lock);
		LOGD("scanlist=%p,scanlistcount=%d,trycount=%d\n",scanlist,scanlistcount,trycount);
		Common_Sleep(0, 1000);
		trycount--;
	}
	while ((scanlistcount == 0) && (trycount > 0));
	
	this->status = STA_SCAN_END;
    return true;
}

int WiFi::getScanList(cJSON_Struct *parentItem)
{
    int i=0;
    if(STA != getCurrentMode())
    {
        LOGE("Not STA Mode!\n");
        return -1;
    }
    else
    {
        this->status = STA_SCAN;
        while(this->status == STA_SCAN)
        {
        	Common_Sleep(0, 1000);
        	if(scanlist) break;
        }

		Common_RWLock_RLock(rw_opt_lock);
        if(scanlist != NULL)
        {
            cJSON_Struct * pArray = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
            for(i=0;i<scanlistcount;i++)
            {
                Common_Json_SetAttrValue(pArray,i,"SSID",Common_Json_Type_String,scanlist[i].ssid,0,0);
                Common_Json_SetAttrValue(pArray,i,"Mac",Common_Json_Type_String,scanlist[i].mac,0,0);
                Common_Json_SetAttrValue(pArray,i,"AuthType",Common_Json_Type_Number,NULL,scanlist[i].auth_type,0);
                Common_Json_SetAttrValue(pArray,i,"Signal",Common_Json_Type_Number,NULL,scanlist[i].signal,0);
            }
            Common_Json_AddItem(parentItem,-1,"ResList",pArray);
            //LOGW("FREE scanlist=%p\n",scanlist);
            FREE(scanlist);
            scanlist = NULL;
        }
		Common_RWLock_UnLock(rw_opt_lock);
#if 0
        char* out = NULL;
        PRINT_INF("%s\n",out = Common_Json_Print(parentItem,NULL));
        if(out)
            Common_Free(out,__FUNCTION__,__LINE__);
#endif

    }
    return 0;
}

}
