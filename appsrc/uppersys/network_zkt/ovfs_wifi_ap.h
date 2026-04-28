#ifndef _OVFS_WIFI_AP_MODE_H_
#define _OVFS_WIFI_AP_MODE_H_


#include "ovfs_wifi_common.h"

namespace wifi {

class APMode
{
private:
    
    char *cfgJson;

public:
    WIFI_AP_STATUS status;

private:

public:
    APMode();
    ~APMode();
    bool start(void);
    bool stop(void);
    bool isReady(void);
    bool checkParam(WIFI_AP_STATUS cfg);
    bool writeConfig(WIFI_AP_STATUS cfg);
    bool readConfig(WIFI_AP_STATUS *cfg);
    bool writeConfig(cJSON_Struct *jsonCfg);
    bool readConfig(cJSON_Struct **jsonCfg);
    bool getCurStatus(cJSON_Struct *jsonCfg);
    int getConnectList(cJSON_Struct *parentItem);
};
}
#endif
