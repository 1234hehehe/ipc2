#ifndef _OVFS_WIFI_H_
#define _OVFS_WIFI_H_

#include "ovfs_wifi_common.h"
#include "ovfs_wifi_sta.h"
#include "ovfs_wifi_ap.h"

namespace wifi {

int Wifi_CfgStructToCfgJson(void *cfgStruct, cJSON_Struct *cfgJson,WIFI_STU_TYPE type);
int Wifi_CfgJsonToCfgStruct(cJSON_Struct *cfgJson,void *cfgStruct,WIFI_STU_TYPE type);

class WiFi
{
private:
    STAMode sta;
    APMode ap;
    WIFI_WORK_MODE mode;
    WIFI_WORK_STATUS status;
    Common_RWLock_T rw_opt_lock;
    pthread_t wifi_workThread;
    WIFI_SCANAPITEM_S *scanlist;
    int scanlistcount;
	int wpactrl_err_cnt;
public:
    bool isConfigSTA;
	bool wifi_running;
private:
    static void * Wifi_Manager(void *__this);

public:
    WiFi();
    ~WiFi();
    
    void init();
	void Restart();
    bool change(WIFI_WORK_MODE current_mode,WIFI_WORK_MODE mode);
    bool setHostap(WIFI_AP_STATUS cfg);
    bool isReady();
    bool scan();
    bool autoconnect();
    bool setconnectCfg(WIFI_STA_CONNECTIONCFG_S wifiCfg);
    int addAP(cJSON_Struct *parentItem);
    int delAP(cJSON_Struct *parentItem);
    int getSTAList(cJSON_Struct *parentItem);
    int getAPList(cJSON_Struct *parentItem);
    int getScanList(cJSON_Struct *parentItem);
    WIFI_WORK_STATUS getCurrentStatus(void);
    WIFI_WORK_MODE getCurrentMode();
    WIFI_WORK_MODE getCurrentMode(cJSON_Struct *jsonCfg);
    WIFI_WORK_STA_STATUS_E GetConnStatus(void);
    WIFI_WORK_STA_STATUS_E UpdateStatus(void);
    bool getStatus(cJSON_Struct *status);
    int setStatus(WIFI_WORK_STATUS status);
    int setParam(WIFI_WORK_STATUS status,WIFI_WORK_MODE mode);
    bool writeConfig(cJSON_Struct *jsonCfg);
    //bool readConfig(cJSON_Struct **jsonCfg);
    bool connect(cJSON_Struct *cfgJson);
    bool update();
};

}

#endif

