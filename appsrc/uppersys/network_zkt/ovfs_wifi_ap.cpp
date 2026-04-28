#include "ovfs_wifi_ap.h"
#include "ovfs_network_api.h"

namespace wifi {
const char * HOSTAP_CONFIG_FILE = "/tmp/hostapd.conf";
const char * AP_CONFIG_FILE = "/tmp/wifi_ap_config";

APMode::APMode()
{
    cfgJson = NULL;
    memset(&status,0,sizeof(status));
}

APMode::~APMode()
{
    stop();
}

bool APMode::start()
{
    bool ret = false;
    // 启动hostapd,dhcpd
    Common_System("udhcpd -S /root/wifi/udhcpd.conf");
    Common_System("hostapd -B /tmp/hostapd.conf");
    printf("AP Start!\n");
    ovfs_soft::ovfs_netcard_config wifiCfg;
	memset(&wifiCfg,0,sizeof(ovfs_soft::ovfs_netcard_config));
	snprintf(wifiCfg.if_name, sizeof(wifiCfg.if_name), "wlan0");
	wifiCfg.dhcp = OVFS_FALSE;
	wifiCfg.ip.is_ipv4 = OVFS_TRUE;
	snprintf(wifiCfg.ip.ipv4,sizeof(wifiCfg.ip.ipv4), "192.168.10.1");
	wifiCfg.netmask.is_ipv4 = OVFS_TRUE;
	snprintf(wifiCfg.netmask.ipv4,sizeof(wifiCfg.netmask.ipv4), "255.255.255.0");
	wifiCfg.gate_way.is_ipv4 = OVFS_TRUE;
	snprintf(wifiCfg.gate_way.ipv4,sizeof(wifiCfg.gate_way.ipv4), "192.168.10.1");
	ovfs_network_set_netcard_cfg(&wifiCfg);
    return ret;
}

bool APMode::stop()
{
    bool ret = false;
    // 关闭hostapd,udhcpd
    Common_System("killall hostapd udhcpd; ifconfig wlan0 0.0.0.0;");
    //snprintf(cmd, sizeof(cmd), "[ -f %s ] && mv %s %s", config_ip_bak_file_name, config_ip_bak_file_name, config_ip_file_name);
    //Common_System(cmd);
    Common_System("ifconfig wlan0 down");
    printf("AP Stopy!\n");
    return ret;
}


bool APMode::isReady()
{
    bool ret = false;
    if(0 == access(HOSTAP_CONFIG_FILE, 0))
    {
        ret = true;
    }
    else
    {
        
    }
    return ret;
}

bool APMode::checkParam(WIFI_AP_STATUS cfg)
{
	bool ret = true;
    LOGD("ssid is [%s]\n",cfg.ssid);
    LOGD("psk is [%s]\n",cfg.psk);
    LOGD("ipv4 is [%s]\n",cfg.ipv4);
    LOGD("mac is [%s]\n",cfg.mac);
    LOGD("auth_type is %d\n",cfg.auth_type);
    struct in_addr addr;
	if (0 == inet_pton(AF_INET, cfg.ipv4, &addr))
	{
		LOGE("%s\n", cfg.ipv4);
		ret = false;
	}
	if(strlen(cfg.ssid) <= 0)
    	ret = false;
    return ret;
}

bool APMode::writeConfig(WIFI_AP_STATUS cfg)
{
    char exefile[] = "/tmp/hostapd.conf.bak";
    char tmpfile[] = "/tmp/hostapd.conf";
    FILE *fp = NULL;
    fp = Common_File_fOpen((S8 *)exefile, (S8 *)"w");  
    if(NULL == fp)  
    {   
        LOGE("open file err\n");
        return false;  
    }
    
    fprintf(fp, "##### hostapd configuration file ######\n");
    fprintf(fp, "interface=wlan0\ndump_file=/tmp/hostapd.dump\nctrl_interface=/var/run/hostapd\n");
    fprintf(fp, "ctrl_interface_group=0\nssid=%s\n", cfg.ssid);
    fprintf(fp, "hw_mode=g\nchannel=6\n");
    
    if (AUTH_MODE_WEP == cfg.auth_type)
    {
        fprintf(fp, "wep_default_key=0\nwep_key0=%s\n", cfg.psk);
    }
    else if (AUTH_MODE_WPA == cfg.auth_type ||
             AUTH_MODE_WPA2 == cfg.auth_type ||
             (AUTH_MODE_WPA2 | AUTH_MODE_WPA)  == cfg.auth_type)
    {
        fprintf(fp, "wpa=2\nwpa_passphrase=%s\n", cfg.psk);
        fprintf(fp, "wpa_key_mgmt=WPA-PSK\nwpa_pairwise=CCMP\n");
    }
    
    PRINT_DBG("write ssid=%s\n",cfg.ssid);
    PRINT_DBG("write wpa_passphrase=%s\n",cfg.psk);
    Common_File_fClose(fp);
    remove(tmpfile);
    rename(exefile, tmpfile);
    
    return true;
}

bool APMode::readConfig(WIFI_AP_STATUS *cfg)
{
    if(NULL == cfg)
        return false;
    memcpy(cfg,&status,sizeof(WIFI_AP_STATUS));
    return true;
}

bool APMode::writeConfig(cJSON_Struct *jsonCfg)
{
    int res=0;
    bool ret = false;
    FILE *fp;
    S32 nStrLen = 0;
    fp = fopen(AP_CONFIG_FILE,"wb+");
    if (fp != NULL)
    {
        cfgJson = Common_Json_Print(jsonCfg,&nStrLen);
        //LOGD("%s\n",cfgJson);
        if (cfgJson && nStrLen > 0)
        {
            res = fwrite(cfgJson,1,(U32)nStrLen + 1,fp);
            if((nStrLen + 1) == res)
                ret = true;
            LOGD("%s\n",cfgJson);
        }
		if(cfgJson)
			Common_Free(cfgJson,__FUNCTION__,__LINE__);
        fclose(fp);
    }
    return ret;
}

bool APMode::readConfig(cJSON_Struct **jsonCfg)
{
	if (jsonCfg == NULL) return false;
    if (*jsonCfg != NULL) return false;

    int res=0;
    bool ret = true;
    char *pConfigString = NULL;
    S32 nStrLen = 0;
    FILE *fp;
    fp = fopen(AP_CONFIG_FILE,"rb");
    if (fp != NULL)
    {
        fseek(fp,0,SEEK_END);
        nStrLen = ftell(fp);
        fseek(fp,0,SEEK_SET);
        if (nStrLen > 0)
        {
            pConfigString = (char *)MALLOC(nStrLen);
            if (pConfigString != NULL)
            {
                res = fread(pConfigString,1,(U32)nStrLen,fp);
                if(nStrLen == res)
                {
                    *jsonCfg = Common_Json_Parse(pConfigString,NULL,NULL);
                }
            }
            
        }
        fclose(fp);
    }
    if (pConfigString != NULL)
    {
        FREE(pConfigString);
        pConfigString = NULL;
    }
	if(*jsonCfg == NULL)
	{
		return false;
	}
    return ret;
}

bool APMode::getCurStatus(cJSON_Struct *jsonCfg)
{
    char buf[4096], *tmp;
    FILE* fp = fopen("/tmp/hostapd.conf", "r");
    if (NULL == fp)
    {
        LOGE("Can not open /tmp/hostapd.conf !\n");
        return false;
    }
    while (fgets(buf, sizeof(buf), fp))
    {
        if (((tmp = strstr(buf, "ssid=")) != NULL) && tmp == buf)
        {
            tmp += 5;
            //strlcpy(status.ssid, tmp, sizeof(status.ssid));
            snprintf(status.ssid,sizeof(status.ssid),"%s",tmp);
            status.ssid[strlen(status.ssid) - 1] = '\0';
        }
        else if (((tmp = strstr(buf, "wpa_passphrase=")) != NULL) && tmp == buf)
        {
            tmp += 15;
            //strlcpy(status.psk, tmp,sizeof(status.psk));
            snprintf(status.psk,sizeof(status.psk),"%s",tmp);
            status.psk[strlen(status.psk) - 1] = '\0';
            
            status.auth_type |= AUTH_MODE_WPA | AUTH_MODE_WPA2;
            break;
        }
        else if (((tmp =strstr(buf, "wep_key0=")) != NULL) && tmp == buf)
        {
            tmp += 9;
            //strlcpy(status.psk, tmp,sizeof(status.psk));
            snprintf(status.psk,sizeof(status.psk),"%s",tmp);
            status.auth_type |= AUTH_MODE_WEP;
            break;
        }
    }
    if (status.auth_type == 0)
        status.auth_type |= AUTH_MODE_ESS;
    fclose(fp);
    
    NetWorkTool_GetMacAddr((char *)"wlan0",status.mac);
    NetWorkTool_GetIPAddr((char *)"wlan0",status.ipv4);

    Common_Json_SetAttrValue(jsonCfg, -1, "APSSID", Common_Json_Type_String,status.ssid, 0, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "APPSK", Common_Json_Type_String,status.psk, 0, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "APIP", Common_Json_Type_String,status.ipv4, 0, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "APMac", Common_Json_Type_String,status.mac, 0, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "APAuthType", Common_Json_Type_Number,0,status.auth_type, 0);
    return true;
}

int APMode::getConnectList(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
    	LOGE("parentItem is NULL\n");
        return -1;
	}
    
    char szCommand[128];
	char cmd[128];
	char mac[32];
	char ip[32];
    char exefile[] = "/tmp/wifi_sta_list";
	FILE *fp = NULL;
	int flag = 0;
	
	snprintf(szCommand, sizeof(szCommand), "hostapd_cli al > %s", exefile);
	Common_System(szCommand);

    fp = Common_File_fOpen((S8 *)exefile, (S8 *)"r");
    if(NULL == fp)
    {  	
		LOGE("open file err\n");
        return -2;  
    }

	cJSON_Struct *pNode = NULL;
	int i =0;
	pNode = Common_Json_SetAttrValue(parentItem,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
	
	while(fgets(cmd, sizeof(cmd), fp) != NULL)  
    {  
		if(strstr(cmd, "Selected interface") != NULL)
		{
            flag = 1;
			continue;
		}

    /*only get mac addr lines */
    if (strlen(cmd) != 18)
        continue;

		if (1 == flag)
		{
            snprintf(mac, strlen(cmd), "%s", cmd);
            // LOGE("@%s@\n",mac);
            if (NetWorkTool_GetIpaddrByMacFromUdhcpdLease((char *)"/var/lib/misc/udhcpd.leases",mac, ip) >0)
            {
                // snprintf(ip, strlen(buf), "%s", buf);
                Common_Json_SetAttrValue(pNode,i,"Mac",Common_Json_Type_String,mac,0,0);
                Common_Json_SetAttrValue(pNode,i,"IP",Common_Json_Type_String,ip,0,0);
                LOGD("mac %s ip %s\n",mac, ip);
                i++;
            
            }
		}
    } 

	Common_File_fClose(fp);

    return 0;
}
}
