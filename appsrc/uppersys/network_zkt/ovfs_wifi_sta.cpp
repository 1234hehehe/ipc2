#include <iostream>
#include "ovfs_wifi_sta.h"
#include "ovfs_network_api.h"

static char *config_file_name = (char *)"/tmp/wpa_supplicant.conf";
static char *config_ip_file_name = (char *)"/tmp/wpa_ip.conf";
static char *config_ip_bak_file_name = (char *)"/tmp/wpa_ip.conf.bak";
static char *udhcpc_pid = (char *)"/var/run/udhcpc_wlan0.pid";
const char *STA_CONFIG_FILE = "/tmp/wifi_sta_config";

namespace wifi
{
typedef long os_time_t;

/**
 * os_sleep - Sleep (sec, usec)
 * @sec: Number of seconds to sleep
 * @usec: Number of microseconds to sleep
 */

struct os_time
{
    os_time_t sec;
    os_time_t usec;
};

struct os_reltime
{
    os_time_t sec;
    os_time_t usec;
};


static string to_string(int val)
{
    char *buf = NULL;
    int size;
    int temp;
    if (val < 0)
    {
        temp = -val;
        size = 2;
    }
    else
    {
        temp = val;
        size = 1;
    }
    for (; temp > 0; temp = temp / 10, size++);
    size++;
    buf = (char *)malloc(size);
    if (buf == NULL)
    {
        return "";
    }
    memset(buf, 0, size);
    sprintf(buf, "%d", val);
    std::string re(buf);
    free(buf);
    return re;
}

vector<string> split(const string &str, const string &delim)
{
    vector<string> res;
    if("" == str) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1] ;  //不要忘了
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while(p)
    {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(NULL, d);
    }
    if(d != NULL) delete [] d;
    if(strs != NULL) delete [] strs;
    return res;
}

int close_dhcpc(void)
{
	char cmd[64] = {0};
	FILE *fp = fopen(udhcpc_pid, "r");
	if (NULL == fp)
	{
		// 不处理,很可能没有启动udhcpc.
		PRINT_DBG("Can not open udhcpc_pid\n");
	}
	else
	{
		char buf[32];
		fseek(fp, 0, SEEK_SET);
		fgets(buf, sizeof(buf), fp);
		fclose(fp);
		snprintf(cmd, sizeof(cmd), "kill -9 %s", buf);
		PRINT_DBG("kill udhcpc cmd: %s \n", cmd);
		Common_System(cmd);
		snprintf(cmd, sizeof(cmd), "rm %s", udhcpc_pid);
		PRINT_DBG("cmd: %s \n", cmd);
		Common_System(cmd);
	}
	return 0;
}

int start_dhcpc(void)
{
	char cmd[64] = {0};
	snprintf(cmd, sizeof(cmd), "udhcpc -i wlan0 -p %s -s udhcpc.script &", udhcpc_pid);
	PRINT_DBG("%s\n", cmd);
	if(Common_System(cmd) ==  0)
	{
		int trycount = 100;
		while(--trycount)
		{
			if(0 == access("/tmp/udhcpc_ip_wlan0.conf", 0))
				break;
			Common_Sleep(0, 100 * 1000);
		}
		LOGD("trycount=%d\n", trycount);
		if(trycount == 0) return -1;
	}
	return 0;
}

static inline void os_reltime_sub(struct os_reltime *a, struct os_reltime *b,
                                  struct os_reltime *res)
{
    res->sec = a->sec - b->sec;
    res->usec = a->usec - b->usec;
    if (res->usec < 0)
    {
        res->sec--;
        res->usec += 1000000;
    }
}

static inline int os_reltime_expired(struct os_reltime *now,
                                     struct os_reltime *ts,
                                     os_time_t timeout_secs)
{
    struct os_reltime age;

    os_reltime_sub(now, ts, &age);
    return (age.sec > timeout_secs) ||
           (age.sec == timeout_secs && age.usec > 0);
}

int os_get_reltime(struct os_reltime *t)
{
#ifndef __MACH__
#if defined(CLOCK_BOOTTIME)
    static clockid_t clock_id = CLOCK_BOOTTIME;
#elif defined(CLOCK_MONOTONIC)
    static clockid_t clock_id = CLOCK_MONOTONIC;
#else
    static clockid_t clock_id = CLOCK_REALTIME;
#endif
    struct timespec ts;
    int res;

    while (1)
    {
        res = clock_gettime(clock_id, &ts);
        if (res == 0)
        {
            t->sec = ts.tv_sec;
            t->usec = ts.tv_nsec / 1000;
            return 0;
        }
        switch (clock_id)
        {
#ifdef CLOCK_BOOTTIME
        case CLOCK_BOOTTIME:
            clock_id = CLOCK_MONOTONIC;
            break;
#endif
#ifdef CLOCK_MONOTONIC
        case CLOCK_MONOTONIC:
            clock_id = CLOCK_REALTIME;
            break;
#endif
        case CLOCK_REALTIME:
            return -1;
        }
    }
#else /* __MACH__ */
    uint64_t abstime, nano;
    static mach_timebase_info_data_t info = { 0, 0 };

    if (!info.denom)
    {
        if (mach_timebase_info(&info) != KERN_SUCCESS)
            return -1;
    }

    abstime = mach_absolute_time();
    nano = (abstime * info.numer) / info.denom;

    t->sec = nano / NSEC_PER_SEC;
    t->usec = (nano - (((uint64_t) t->sec) * NSEC_PER_SEC)) / NSEC_PER_USEC;

    return 0;
#endif /* __MACH__ */
}

mutex wpa_ctrl_lock;

struct wpa_ctrl *wpa_ctrl_open(const char *ctrl_path,
                               const char *cli_path)
{
    struct wpa_ctrl *ctrl;
    ctrl = (struct wpa_ctrl *)malloc(sizeof(struct wpa_ctrl));
    if (ctrl == nullptr)
    {
        return nullptr;
    }
    memset(ctrl, 0, sizeof(struct wpa_ctrl));

    static int counter = 0;
    int ret = -1;
    int tries = 0;
    size_t res = 0;
    int flags;
    ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (ctrl->s < 0)
    {
        free(ctrl);
        return nullptr;
    }
    ctrl->local.sun_family = AF_UNIX;
    counter++;

try_again:
    if (cli_path && cli_path[0] == '/')
    {
        ret = snprintf(ctrl->local.sun_path,
                       sizeof(ctrl->local.sun_path),
                       "%s/" "wpa_ctrl_" "%d-%d",
                       cli_path, (int) getpid(), counter);
    }
    else
    {
        ret = snprintf(ctrl->local.sun_path,
                       sizeof(ctrl->local.sun_path),
                       "/tmp" "/"
                       "wpa_ctrl_" "%d-%d",
                       (int) getpid(), counter);
    }

    if (ret < 0 || (size_t)ret >= sizeof(ctrl->local.sun_path))
    {
        close(ctrl->s);
        free(ctrl);
        return nullptr;
    }
    tries++;

    if (bind(ctrl->s, (struct sockaddr *) &ctrl->local, sizeof(ctrl->local)) < 0)
    {
        if (errno == EADDRINUSE && tries < 2)
        {
            /*
            * getpid() returns unique identifier for this instance
            * of wpa_ctrl, so the existing socket file must have
            * been left by unclean termination of an earlier run.
            * Remove the file and try again.
            */
            unlink(ctrl->local.sun_path);
            goto try_again;
        }
        close(ctrl->s);
        free(ctrl);
        return nullptr;
    }
    ctrl->dest.sun_family = AF_UNIX;
    //res = strlcpy(ctrl->dest.sun_path, ctrl_path, sizeof(ctrl->dest.sun_path));
    ret = snprintf(ctrl->dest.sun_path,sizeof(ctrl->dest.sun_path),"%s",ctrl_path);
    if (res >= sizeof(ctrl->dest.sun_path))
    {
        close(ctrl->s);
        free(ctrl);
        return nullptr;
    }

    if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest, sizeof(ctrl->dest)) < 0)
    {
        close(ctrl->s);
        unlink(ctrl->local.sun_path);
        free(ctrl);
        return nullptr;
    }

    /*
     * Make socket non-blocking so that we don't hang forever if
     * target dies unexpectedly.
     */
    flags = fcntl(ctrl->s, F_GETFL);
    if (flags >= 0)
    {
        flags |= O_NONBLOCK;
        if (fcntl(ctrl->s, F_SETFL, flags) < 0)
        {
            perror("fcntl(ctrl->s, O_NONBLOCK)");
            /* Not fatal, continue on.*/
        }
    }

    return ctrl;

}

void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
    if(ctrl == NULL)
    {
        return;
	}
	wpa_ctrl_lock.lock();
    unlink(ctrl->local.sun_path);
    if (ctrl->s >= 0)
        close(ctrl->s);
    free(ctrl);
	wpa_ctrl_lock.unlock();
}

int wpa_ctrl_pending(struct wpa_ctrl *ctrl)
{
    struct timeval tv;
    fd_set rfds;
	if (ctrl == NULL)
        return -1;
	int socketfd = ctrl->s;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(socketfd, &rfds);
    select(socketfd + 1, &rfds, NULL, NULL, &tv);
    return FD_ISSET(socketfd, &rfds);
}

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
                     char *reply, size_t *reply_len,
                     void (*msg_cb)(char *msg, size_t len))
{
    struct timeval tv;
    struct os_reltime started_at;
    int res;
    fd_set rfds;
    const char *_cmd;
    char *cmd_buf = NULL;
    size_t _cmd_len;

    _cmd = cmd;
    _cmd_len = cmd_len;

    errno = 0;
    started_at.sec = 0;
    started_at.usec = 0;

	if (ctrl == NULL)
        return -1;
	int socketfd = ctrl->s;

	wpa_ctrl_lock.lock();
retry_send:
    if (send(socketfd, _cmd, _cmd_len, 0) < 0)
    {
        if (errno == EAGAIN || errno == EBUSY || errno == EWOULDBLOCK)
        {
            /*
             * Must be a non-blocking socket... Try for a bit
             * longer before giving up.
             */
            if (started_at.sec == 0)
                os_get_reltime(&started_at);
            else
            {
                struct os_reltime n;
                os_get_reltime(&n);
                /* Try for a few seconds. */
                if (os_reltime_expired(&n, &started_at, 5))
                    goto send_err;
            }
            sleep(1);
            goto retry_send;
        }
send_err:
        free(cmd_buf);
		wpa_ctrl_lock.unlock();
        return -1;
    }
    free(cmd_buf);

    for (;;)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(socketfd, &rfds);
        res = select(socketfd + 1, &rfds, NULL, NULL, &tv);
        if (res < 0 && errno == EINTR)
            continue;
        if (res < 0)
        {
        	wpa_ctrl_lock.unlock();
            return res;
        }
        if (FD_ISSET(socketfd, &rfds))
        {
            res = recv(socketfd, reply, *reply_len, 0);
            if (res < 0)
            {
            	wpa_ctrl_lock.unlock();
                return res;
            }
            if ((res > 0 && reply[0] == '<') ||
                    (res > 6 && strncmp(reply, "IFNAME=", 7) == 0))
            {
                /* This is an unsolicited message from
                 * wpa_supplicant, not the reply to the
                 * request. Use msg_cb to report this to the
                 * caller. */
                if (msg_cb)
                {
                    /* Make sure the message is nul
                     * terminated. */
                    if ((size_t) res == *reply_len)
                        res = (*reply_len) - 1;
                    reply[res] = '\0';
                    msg_cb(reply, res);
                }
                continue;
            }
            *reply_len = res;
            break;
        }
        else
        {
        	wpa_ctrl_lock.unlock();
            return -2;
        }
    }
	wpa_ctrl_lock.unlock();
	return 0;
}


static int wpa_ctrl_attach_helper(struct wpa_ctrl *ctrl, int attach)
{
    char buf[10];
    int ret;
    size_t len = 10;

    ret = wpa_ctrl_request(ctrl, attach ? "ATTACH" : "DETACH", 6,
                           buf, &len, NULL);
    if (ret < 0)
        return ret;
    if (len == 3 && memcmp(buf, "OK\n", 3) == 0)
        return 0;
    return -1;
}


int wpa_ctrl_attach(struct wpa_ctrl *ctrl)
{
    return wpa_ctrl_attach_helper(ctrl, 1);
}


int wpa_ctrl_detach(struct wpa_ctrl *ctrl)
{
    return wpa_ctrl_attach_helper(ctrl, 0);
}

int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
    int res;

    res = recv(ctrl->s, reply, *reply_len, 0);
    if (res < 0)
        return res;
    *reply_len = res;
    return 0;
}

STAMode::STAMode()
{
    wpa = new WPAClient();
    memset(&connectCfg, 0, sizeof(connectCfg));
}

STAMode::~STAMode()
{
    delete wpa;
    stop();
}

bool STAMode::start()
{
    char cmd[80];
    // 启动wpa_supplicant
    //Common_System("ifconfig wlan0 up");
    snprintf(cmd, sizeof(cmd), "wpa_supplicant -B -Dwext -iwlan0 -c %s", config_file_name);
    PRINT_DBG("%s\n", cmd);
    Common_System(cmd);
    int trycount = 5;
    while(trycount--)
    {
        if(connect()) break;
    }
    if(isReady())
    {
        autoconnect();
    }
    return true;
}

bool STAMode::stop()
{
    char cmd[128];
    // 关闭wpa_supplicant,udhcpc
    Common_System("killall wpa_supplicant");
    if(0 == access(udhcpc_pid, 0))
    {
        close_dhcpc();
        Common_System("killall wpa_supplicant");
        snprintf(cmd, sizeof(cmd), "[ -f %s ] && mv %s %s", config_ip_file_name, config_ip_file_name,
                 config_ip_bak_file_name);
        PRINT_DBG("%s\n", cmd);
        Common_System(cmd);
        //Common_System("ifconfig wlan0 down");
    }
    disconnect();
    return true;
}

bool STAMode::isReady()
{
    if(wpa == NULL)
        return false;
    return wpa->GetInitStatus();
}

bool STAMode::scan(WIFI_SCANAPITEM_S **scanresultList, int *totalNum)
{
    bool ret = false;
    //LOGD("Wait Scan Result\n");
    if(wpa == NULL)
        return false;
    ret = wpa->Scan();
    ret = wpa->GetScanResult(scanresultList, totalNum);
    //LOGD("Scan Result TotalNum=%d\n", *totalNum);
    return ret;
}

bool STAMode::autoconnect()
{
    int retInt = -1;
    char *retStr = NULL;
    PRINT_WARN("autoconnect\n");
    if(wpa == NULL)
        return false;
    cJSON_Struct *jsonCfg = NULL;
    readConfig(&jsonCfg);
    cJSON_Struct *cfgArray = Common_Json_GetItem(jsonCfg, -1, "APList");

    removeall();
    if (NULL != cfgArray)
    {
        int i = 0, num = Common_Json_Size(cfgArray);
        LOGD("cfgArray Size =%d\n", num);
        if (num > 0)
        {
            WIFI_STA_CONNECTIONCFG_S connectCfg;
            memset(&connectCfg, 0, sizeof(connectCfg));
            for (i = 0; i < num; i++)
            {
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "SSID", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.authCfg.ssid, sizeof(connectCfg.authCfg.ssid), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "PSK", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.authCfg.psk, sizeof(connectCfg.authCfg.psk), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "Mac", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.authCfg.mac, sizeof(connectCfg.authCfg.mac), "%s", retStr);
                }
                retInt = -1;
                Common_Json_GetAttrValue(cfgArray, i, "AuthType", NULL, NULL, &retInt, NULL);
                if(retInt != -1)
                {
                    connectCfg.authCfg.auth_type = retInt;
                }
                retInt = -1;
                Common_Json_GetAttrValue(cfgArray, i, "IsDhcp", NULL, NULL, &retInt, NULL);
                if((retInt == 0) || (retInt == 1))
                {
                    connectCfg.ipAddrCfg.bDhcp = retInt;
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "IP", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.ipAddrCfg.ipv4, sizeof(connectCfg.ipAddrCfg.ipv4), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "NetMask", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.ipAddrCfg.netmask, sizeof(connectCfg.ipAddrCfg.netmask), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "GateWay", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.ipAddrCfg.gateway, sizeof(connectCfg.ipAddrCfg.gateway), "%s", retStr);
                }
                retInt = -1;
                Common_Json_GetAttrValue(cfgArray, i, "Select", NULL, NULL, &retInt, NULL);
                if(retInt != -1)
                {
                    connectCfg.priority = retInt;
                }
                if(checkParam(connectCfg))
                    saveConf(connectCfg);
            }
        }
    }
    enableall();
    if(jsonCfg)
        Common_Json_Delete(jsonCfg);
    return true;
}

bool STAMode::connect()
{
    bool ret = false;
    ret = wpa->Init();
    return ret;
}

bool STAMode::disconnect()
{
    bool ret = false;
    ret = wpa->UnInit();
    return ret;
}

bool STAMode::save(WIFI_STA_CONNECTIONCFG_S connectCfg)
{
    if(wpa == NULL)
        return false;
    return wpa->ConnectWiFi(connectCfg.authCfg.ssid, connectCfg.authCfg.psk, connectCfg.authCfg.auth_type,
                            connectCfg.priority);
}

bool STAMode::forget(WIFI_STA_CONNECTIONCFG_S connectCfg)
{
    if(wpa == NULL)
        return false;
    return wpa->ForgetWiFi(connectCfg.authCfg.ssid);
}

WIFI_WORK_STA_STATUS_E STAMode::GetConnStatus()
{
    WIFI_WORK_STA_STATUS_E CurStatus;
    if(wpa == NULL)
    {
        return WIFI_WORK_STA_STATUS_DISCONNECTED;
    }
    //LOGD("status.Status=%d\n",wpa->status.Status);
    CurStatus = wpa->status.Status;
    return CurStatus;
}

WIFI_WORK_STA_STATUS_E STAMode::UpdateStatus()
{
    if(wpa == NULL)
        return WIFI_WORK_STA_STATUS_DISCONNECTED;
    return wpa->UpdateStatus(&connectCfg);
}

int STAMode::getlist(cJSON_Struct *parentItem)
{
    cJSON_Struct *jsonCfg = NULL;
    readConfig(&jsonCfg);
    cJSON_Struct *cfgArray = Common_Json_GetItem(jsonCfg, -1, "APList");
    cJSON_Struct *reList = Common_Json_Duplicate(cfgArray, 1);
    Common_Json_AddItem(parentItem, -1, "ResList", reList);
    Common_Json_Delete(jsonCfg);
#if 0
    char *out = NULL;
    LOGI("[Status] tree:=%s\n", out = Common_Json_Print(parentItem, NULL));
    if(out)
        Common_Free(out, __FUNCTION__, __LINE__);
#endif
    return 0;
}


bool STAMode::enableall()
{
    bool ret = false;
    if(wpa == NULL)
        return false;
    ret = wpa->EnableAllWiFi();
    ret |= wpa->ReConfigure();
    ret |= wpa->SaveConfigure();
    return ret;
}

bool STAMode::removeall()
{
    if(wpa == NULL)
        return false;
    return wpa->CleanAllWiFi();
}

bool STAMode::checkParam(WIFI_STA_CONNECTIONCFG_S conf)
{
    bool ret = true;
    LOGD("ssid={%s},priority=%d\n", conf.authCfg.ssid, conf.priority);
    if(wpa == NULL)
    {
        return false;
    }
    if(strlen(conf.authCfg.ssid) <= 0)
    {
        ret = false;
    }
    if(conf.priority < 0)
    {
        ret = false;
    }
    return ret;
}


bool STAMode::saveConf(WIFI_STA_CONNECTIONCFG_S conf)
{
    bool ret = false;
    if(wpa == NULL)
        return false;
    ret = wpa->SaveWiFi(conf.authCfg.ssid, conf.authCfg.psk, conf.authCfg.auth_type, conf.priority);
    LOGD("ssid={%s},priority=%d,auth_type=%d,save[%d]\n", conf.authCfg.ssid, conf.priority,
         conf.authCfg.auth_type, ret);
    return ret;
}

bool STAMode::writeConfig(cJSON_Struct *jsonCfg)
{
    int res = 0;
    bool ret = false;
    FILE *fp;
    char *cfgJson = NULL;
    S32 nStrLen = 0;
    fp = fopen(STA_CONFIG_FILE, "wb+");
    if (fp != NULL)
    {
        cfgJson = Common_Json_Print(jsonCfg, &nStrLen);
        //LOGD("%s\n",cfgJson);
        if (cfgJson && nStrLen > 0)
        {
            res = fwrite(cfgJson, 1, (U32)nStrLen + 1, fp);
            if((nStrLen + 1) == res)
                ret = true;
            LOGD("%s\n", cfgJson);
        }
        if(cfgJson)
            Common_Free(cfgJson, __FUNCTION__, __LINE__);
        fclose(fp);
    }
    fp = Common_File_fOpen((S8 *)config_file_name, (S8 *)"w");
    if(NULL == fp)
    {
        LOGE("open file err\n");
        return false;
    }


    fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
    fprintf(fp, "update_config=1\n");

    Common_File_fClose(fp);
    return ret;
}

bool STAMode::readConfig(cJSON_Struct **jsonCfg)
{
    if (jsonCfg == NULL) return false;
    if (*jsonCfg != NULL) return false;

    int res = 0;
    bool ret = true;
    char *pConfigString = NULL;
    S32 nStrLen = 0;
    FILE *fp;
    fp = fopen(STA_CONFIG_FILE, "rb");
    if (fp != NULL)
    {
        fseek(fp, 0, SEEK_END);
        nStrLen = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (nStrLen > 0)
        {
            pConfigString = (char *)MALLOC(nStrLen);
            if (pConfigString != NULL)
            {
                res = fread(pConfigString, 1, (U32)nStrLen, fp);
                LOGD("%s\n", pConfigString);
                if(nStrLen == res)
                {
                    *jsonCfg = Common_Json_Parse(pConfigString, NULL, NULL);
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

bool STAMode::checkIsCurSsid(char *ssid)
{
    if(wpa == NULL)
        return false;
    if(ssid == NULL)
        return false;
    if(strcmp(wpa->status.ssid, ssid) != 0)
        return false;
    return true;
}

bool STAMode::update()//更新网络连接信息，返回值表示配置已生效
{
    bool ret = true;
    int retInt = -1;
    char *retStr = NULL;

    cJSON_Struct *savejsonCfg = NULL;
    readConfig(&savejsonCfg);
    cJSON_Struct *cfgArray = Common_Json_GetItem(savejsonCfg, -1, "APList");

    WIFI_STA_CONNECTIONCFG_S connectCfg;
    memset(&connectCfg, 0, sizeof(connectCfg));
    if (NULL != cfgArray)
    {
        int i = 0, num = Common_Json_Size(cfgArray);
        if (num > 0)
        {
            for (i = 0; i < num; i++)
            {
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "SSID", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.authCfg.ssid, sizeof(connectCfg.authCfg.ssid), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "PSK", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.authCfg.psk, sizeof(connectCfg.authCfg.psk), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "Mac", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.authCfg.mac, sizeof(connectCfg.authCfg.mac), "%s", retStr);
                }
                retInt = -1;
                Common_Json_GetAttrValue(cfgArray, i, "IsDhcp", NULL, NULL, &retInt, NULL);
                if((retInt == 0) || (retInt == 1))
                {
                    connectCfg.ipAddrCfg.bDhcp = retInt;
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "IP", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.ipAddrCfg.ipv4, sizeof(connectCfg.ipAddrCfg.ipv4), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "NetMask", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.ipAddrCfg.netmask, sizeof(connectCfg.ipAddrCfg.netmask), "%s", retStr);
                }
                retStr = NULL;
                Common_Json_GetAttrValue(cfgArray, i, "GateWay", NULL, &retStr, NULL, NULL);
                if(retStr != NULL)
                {
                    snprintf(connectCfg.ipAddrCfg.gateway, sizeof(connectCfg.ipAddrCfg.gateway), "%s", retStr);
                }
                retInt = -1;
                Common_Json_GetAttrValue(cfgArray, i, "Select", NULL, NULL, &retInt, NULL);
                if(retInt != -1)
                {
                    connectCfg.priority = retInt;
                }
                if(checkParam(connectCfg))
                {
                    if(checkIsCurSsid(connectCfg.authCfg.ssid)) break;
                }
            }
        }
    }

    if(savejsonCfg)
    {
        Common_Json_Delete(savejsonCfg);
    }

	ovfs_soft::ovfs_netcard_config wifiCfg;
	memset(&wifiCfg,0,sizeof(ovfs_soft::ovfs_netcard_config));
	snprintf(wifiCfg.if_name, sizeof(wifiCfg.if_name), "wlan0");
	wifiCfg.ip.is_ipv4 = OVFS_TRUE;
	snprintf(wifiCfg.ip.ipv4,sizeof(wifiCfg.ip.ipv4), "%s",connectCfg.ipAddrCfg.ipv4);
	wifiCfg.netmask.is_ipv4 = OVFS_TRUE;
	snprintf(wifiCfg.netmask.ipv4,sizeof(wifiCfg.netmask.ipv4), "%s",connectCfg.ipAddrCfg.netmask);
	wifiCfg.gate_way.is_ipv4 = OVFS_TRUE;
	snprintf(wifiCfg.gate_way.ipv4,sizeof(wifiCfg.gate_way.ipv4), "%s",connectCfg.ipAddrCfg.gateway);
    if (connectCfg.ipAddrCfg.bDhcp)
    {
#if 0
        if(0 == access(udhcpc_pid, 0))//如果udhcpc存在也需要kill掉重新启动，以更新dhcp地址；
        {
			close_dhcpc();
        }
		ret = start_dhcpc();
#else
		wifiCfg.dhcp = OVFS_TRUE;
#endif
    }
    else
    {
#if 0
    	char cmd[64] = {0};
        snprintf(cmd, sizeof(cmd), "ifconfig wlan0 %s netmask %s;", connectCfg.ipAddrCfg.ipv4,
                 connectCfg.ipAddrCfg.netmask);
        PRINT_DBG("%s\n", cmd);
        Common_System(cmd);

        snprintf(cmd, sizeof(cmd), "route add default dev wlan0 gw %s;", connectCfg.ipAddrCfg.gateway);
        PRINT_DBG("%s\n",cmd);
        Common_System(cmd);
#else
		wifiCfg.dhcp = OVFS_FALSE;
#endif
    }
	ovfs_network_set_netcard_cfg(&wifiCfg);
    return ret;
}

bool STAMode::getCurStatus(cJSON_Struct *jsonCfg)
{
    if(wpa == NULL)
        return false;

    Common_Json_SetAttrValue(jsonCfg, -1, "STASSID", Common_Json_Type_String, wpa->status.ssid, 0, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "STAIP", Common_Json_Type_String, wpa->status.ipv4addr, 0, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "STAStatus", Common_Json_Type_Number, 0, wpa->status.Status, 0);
    Common_Json_SetAttrValue(jsonCfg, -1, "STASignal", Common_Json_Type_Number, 0, wpa->status.signal, 0);
    return true;
}

WPAClient::WPAClient()
{
    wpa_ctrl_conn = nullptr;
	wpa_monitor_conn = nullptr;
    memset(&status, 0, sizeof(status));
}

WPAClient::~WPAClient()
{
    wpa_ctrl_close(wpa_ctrl_conn);
	wpa_ctrl_close(wpa_monitor_conn);
}

bool WPAClient::Init()
{
    wpa_ctrl_conn = wpa_ctrl_open(WPA_PATH.data(),NULL);
    if (wpa_ctrl_conn == nullptr)
    {
        PRINT_ERR("wpa_cli_ctrl is not Open!\n");
        return false;
    }
	wpa_monitor_conn = wpa_ctrl_open(WPA_PATH.data(),NULL);
	if (wpa_monitor_conn == nullptr)
    {
        PRINT_ERR("wpa_cli_monitor is not Open!\n");
        return false;
    }
	wpa_ctrl_attach(wpa_monitor_conn);
    return true;
}

bool WPAClient::UnInit()
{
    if (wpa_ctrl_conn != nullptr)
    {
        wpa_ctrl_close(wpa_ctrl_conn);
    }
	if (wpa_monitor_conn != nullptr)
    {
        wpa_ctrl_close(wpa_monitor_conn);
    }
    return true;
}

bool WPAClient::Request(WPAContext * context, const string & cmd, string& reply)
{
	int ret;
	char buf[1024] = {0};
	size_t len = 1024;

	if(NULL == context) return false;

	//PRINT_DBG("Request[%d]:%s\n",cmd.length(),cmd.data());

	ret = wpa_ctrl_request(context, cmd.data(), cmd.length(),buf, &len, NULL);
	if (ret < 0)
		return false;
	
	//PRINT_WARN("Recv[%d]:%s\n",len,buf);
	
	reply = buf;
	return true;
}

std::string WPAClient::GetCurrentSSID()
{
    std::string cmd = "STATUS";
    std::string ssid_key = "\nssid=";
    std::string recv;
    std::string ssid = "";
    if (!Request(wpa_ctrl_conn, cmd, recv))
    {
        return "";
    }
    char temp[1024] = {0};
    strcpy(temp, recv.data());
    char *key = NULL;
    key = strstr(temp, ssid_key.data());
    if (key == NULL)
    {
        return "";
    }
    key += ssid_key.length();
    for (; (*key != '\0') && (*key != '\n') && (*key != '\r'); key++)
    {
        ssid += *key;
    }
    return ssid;
}
int WPAClient::GetWiFiRssi()
{
    std::string cmd = "STATUS";
    std::string rssi_key = "signal_level";
    std::string recv;
    if (!Request(wpa_ctrl_conn, cmd, recv))
    {
        return 0;
    }
    char temp[1024] = {0};
    strcpy(temp, recv.data());
    char *key = NULL;
    key = strstr(temp, rssi_key.data());
    if (key == NULL)
    {
        return 0;
    }
    for (; (*key != '\0') && (*key != '\n') && (*key != '\r'); key++)
    {
        if ((*key >= '0') && (*key <= '9'))
        {
            return atoi(key);
        }
    }
    return 0;
}

bool WPAClient::ConnectWiFi(const string &ssid, const string &password, int en_crypt, int priority)
{
    int net_id;
    if (!CleanAllWiFi())
    {
        return false;
    }
    if (!AddWiFi(net_id))
    {
        return false;
    }
    if (!SetSSID(ssid, net_id))
    {
        return false;
    }
    if (!SetPassword(password, net_id))
    {
        return false;
    }
    if (!SetProtocol(net_id, en_crypt))
    {
        return false;
    }
    if (!SetPriority(net_id, priority))
    {
        return false;
    }
    SetScanSSID(net_id);
    if (!EnableWiFi(net_id))
    {
        return false;
    }
    return CheckCommandWithOk("SAVE_CONFIG");
}


bool WPAClient::SaveWiFi(const string &ssid, const string &password, int en_crypt, int priority)
{
    int net_id = -1;
    if(!GetInitStatus())
    {
        PRINT_ERR("wpa client is not ready!\n");
        return false;
    }
    if (!AddWiFi(net_id))
    {
        PRINT_ERR("add network error!\n");
        return false;
    }
    PRINT_DBG("net_id=%d\n", net_id);
    if (!SetSSID(ssid, net_id))
    {
        PRINT_ERR("SetSSID error!\n");
        return false;
    }
    if (!SetPassword(password, net_id))
    {
        PRINT_ERR("SetPassword error!\n");
        return false;
    }
    if (!SetProtocol(net_id, en_crypt))
    {
        PRINT_ERR("SetProtocol error!\n");
        return false;
    }
    if (!SetPriority(net_id, priority))
    {
        PRINT_ERR("SetPriority error!\n");
        return false;
    }
    SetScanSSID(net_id);
    if (!EnableWiFi(net_id))
    {
        PRINT_ERR("EnableWiFi error!\n");
        return false;
    }
    if (!SaveConfigure())
    {
        PRINT_ERR("save_config error!\n");
        return false;
    }
    return true;
}

bool WPAClient::ForgetWiFi(const string &ssid)
{
    if(!GetInitStatus())
    {
        return false;
    }
    if (!CheckCommandWithOk("REMOVE_NETWORK all"))
    {
        return false;
    }
    return true;
}

WIFI_WORK_STA_STATUS_E WPAClient::UpdateStatus(WIFI_STA_CONNECTIONCFG_S *connCfg)
{
    string cmd = "STATUS";
    string recv;
    if (!Request(wpa_ctrl_conn, cmd, recv))
    {
        return WIFI_WORK_STA_STATUS_FAILED;
    }
    vector<string> AllStr = split(recv, "\n");
    int resultSize = AllStr.size();
    //PRINT_DBG("resultSize=%d\n",resultSize);
    if(resultSize > 0)
    {
        for (int i = 0; i < resultSize ; i++)
        {
            vector<string> tempStr = split(AllStr[i], "=");
            if(tempStr.size() >= 2)
            {
                //PRINT_DBG("Name:%s  Value:%s\n",tempStr[0].c_str(),tempStr[1].c_str());
                if(tempStr[0] == "ssid")
                {
                    snprintf(status.ssid, sizeof(status.ssid), "%s", tempStr[1].c_str());
                }
                if(tempStr[0] == "ip_address")
                {
                    snprintf(status.ipv4addr, sizeof(status.ipv4addr), "%s", tempStr[1].c_str());
                }
                if(tempStr[0] == "signal_level")
                {
                    status.signal = atoi(tempStr[1].c_str());
                }
                if(tempStr[0] == "wpa_state")
                {
                    //PRINT_DBG("%s\n",tempStr[1].c_str());
                    if(tempStr[1] == "COMPLETED")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_COMPLETED;
                    }
                    if(tempStr[1] == "DISCONNECTED")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_DISCONNECTED;
                    }
                    if(tempStr[1] == "SCANNING")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_SCANNING;
                    }
                    if(tempStr[1] == "INACTIVE")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_INACTIVE;
                    }
                    if(tempStr[1] == "ASSOCIATING")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_ASSOCIATING;
                    }
                    if(tempStr[1] == "ASSOCIATED")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_ASSOCIATED;
                    }
                    if(tempStr[1] == "4WAY_HANDSHAKE")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_4WAY_HANDSHAKE;
                    }
                    if(tempStr[1] == "GROUP_HANDSHAKE")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_GROUP_HANDSHAKE;
                    }
                    if(tempStr[1] == "AUTHENTICATING")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_AUTHENTICATING;
                    }
                    if(tempStr[1] == "INTERFACE_DISABLED")
                    {
                        status.Status = WIFI_WORK_STA_STATUS_INTERFACE_DISABLED;
                        Common_System("ifconfig wlan0 up");
                    }
                    //PRINT_DBG("SSID[%s]:%s\n",tempStr[1].c_str(),status.ssid);
                }
            }
        }
        if(connCfg != NULL)
        {
            snprintf(connCfg->authCfg.ssid, sizeof(connCfg->authCfg.ssid), "%s", status.ssid);
        }
    }
    else
    {
        return WIFI_WORK_STA_STATUS_FAILED;
    }
    return status.Status;
}

bool WPAClient::CheckCommandWithOk(const string cmd)
{
    string recv;
    recv.clear();
    if (!Request(wpa_ctrl_conn, cmd, recv))
    {
        PRINT_ERR("Request failed!\n");
        return false;
    }
    if (strstr(recv.data(), "OK") == NULL)
    {
        PRINT_ERR("cmd:%s\n", cmd.data());
        PRINT_ERR("recv[%s]\n", recv.data());
        return false;
    }
    return true;
}

bool WPAClient::AddWiFi(int &id)
{
    string add_cmd = "ADD_NETWORK";
    string recv;
    if (!Request(wpa_ctrl_conn, add_cmd, recv))
    {
        return false;
    }
    id = atoi(recv.data());
    return true;
}

bool WPAClient::Scan()
{
    bool ret = CheckCommandWithOk("SCAN");
    //return CheckCommandWithOk("SCAN_INTERVAL 30");
    return ret;
}

bool WPAClient::GetScanResult(WIFI_SCANAPITEM_S **scanresultList, int *totalNum)
{
    int i = 0, n = 0;
    bool ret = false;
    string max_reslut;
    Common_Sleep(0, 500 * 1000);
    ret = Request(wpa_ctrl_conn, "SCAN_RESULTS", max_reslut);
    PRINT_DBG("{%s}\n\n\n", max_reslut.data());
    if(max_reslut.length() > 0)
    {
        n = max_reslut.find('\n');
        if (n != (int)string::npos) //查找 \n 出现的位置
        {
            vector<string> vec;
            string tmp, str = max_reslut.substr(n + 1);
            string::iterator it = str.begin();
            for (; it != str.end(); it++)
            {
                char c = *it;
                if (c != '\n')
                {
                    tmp += c;
                }
                else
                {
                    vec.push_back(tmp);
                    tmp.clear();
                }
            }
            //int frequency;
            char s_auth_type[64], *tmp_auth_type;
            *totalNum = vec.size();
            PRINT_DBG("totalNum=%d,scanresultList=%p\n", *totalNum, scanresultList);
            if((scanresultList == NULL) || (*totalNum <= 0))
                return false;
			
            *scanresultList = (WIFI_SCANAPITEM_S *)MALLOC(*totalNum * sizeof(WIFI_SCANAPITEM_S));
            LOGW("MALLOC scanlist=%p\n", *scanresultList);
			if(*scanresultList == NULL)
				return false;
            memset(*scanresultList, 0, *totalNum * sizeof(WIFI_SCANAPITEM_S));
            for (auto it : vec)
            {
                (*scanresultList + i)->auth_type = 0;
                vector<string> result = split(it, "\t");
				
				//for(int i=0 ;i<result.size();i++)
        		//std::cout<<result[i]<<"\t";
				//std::cout<<std::endl;
				
                if(result.size() >= 4)
                {
                    snprintf((*scanresultList + i)->mac, sizeof((*scanresultList + i)->mac), "%s", result[0].c_str());
                    //frequency = atoi(result[1].c_str());
                    (*scanresultList + i)->signal = atoi(result[2].c_str());
                    snprintf(s_auth_type, sizeof(s_auth_type), "%s", result[3].c_str());
                    if(result.size() == 5)
                    {
                        snprintf((*scanresultList + i)->ssid, sizeof((*scanresultList + i)->ssid), "%s", result[4].c_str());
                    }
                    //pHx((unsigned char *)(*scanresultList+i)->ssid,sizeof((*scanresultList+i)->ssid));
                }
                else
                {
                    PRINT_DBG("The result[%d]:%s\n", result.size(), it.c_str());
                }
                //sscanf(it.c_str(),"%s %d %d %s %s%[^\n]",&(*scanresultList+i)->mac,&frequency,&(*scanresultList+i)->signal,s_auth_type,&(*scanresultList+i)->ssid);
                
                if (strstr(s_auth_type, "WEP") != NULL)
                    (*scanresultList + i)->auth_type |= AUTH_MODE_WEP;
                else if ((tmp_auth_type = strstr(s_auth_type, "WPA")) != NULL)
                {
                    while (tmp_auth_type != NULL)
                    {
                        if (tmp_auth_type[3] == '2')
                            (*scanresultList + i)->auth_type |= AUTH_MODE_WPA2;
                        else
                            (*scanresultList + i)->auth_type |= AUTH_MODE_WPA;
                        tmp_auth_type = strstr(tmp_auth_type + 3, "WPA");
                    }
                }
                if (strstr(s_auth_type, "ESS") != NULL)
                    (*scanresultList + i)->auth_type |= AUTH_MODE_ESS;
                i++;
            }
        }
    }
    return ret;
}


bool WPAClient::SetScanSSID(int id)
{
    string cmd = "SET_NETWORK " + to_string(id) + " scan_ssid 1";
    return CheckCommandWithOk(cmd);
}

bool WPAClient::SetSSID(const string &ssid, int id)
{
    string cmd = "SET_NETWORK " + to_string(id) + " ssid " + "\"" + ssid + "\"";
    return CheckCommandWithOk(cmd);
}

bool WPAClient::SetPassword(const string &password, int id)
{
    string cmd = "SET_NETWORK " + to_string(id) + " psk " + "\"" + password + "\"";
    PRINT_DBG("%s\n", cmd.c_str());
    return CheckCommandWithOk(cmd);
}

bool WPAClient::SetProtocol(int id, int en_crypt)
{
    string cmd = "SET_NETWORK " + to_string(id);
    PRINT_DBG("en_crypt=%d\n", en_crypt);
    if (en_crypt & (AUTH_MODE_WPA | AUTH_MODE_WPA2))
    {
        cmd += " key_mgmt WPA-PSK";
    }
    else
    {
        cmd += " key_mgmt NONE";
    }
    PRINT_DBG("%s\n", cmd.c_str());
    return CheckCommandWithOk(cmd);
}

bool WPAClient::SetPriority(int id, int priority)
{
    string cmd = "SET_NETWORK " + to_string(id) + " priority " + to_string(priority);
    return CheckCommandWithOk(cmd);
}


bool WPAClient::CleanAllWiFi()
{
    CheckCommandWithOk("REMOVE_NETWORK all");
    SaveConfigure();
    CheckCommandWithOk("DISABLE_NETWORK all");
    return true;
}

bool WPAClient::EnableWiFi(int id)
{
    string cmd = "ENABLE_NETWORK " + to_string(id);
    return CheckCommandWithOk(cmd);
}

bool WPAClient::EnableAllWiFi()
{
    string cmd = "ENABLE_NETWORK all";
    return CheckCommandWithOk(cmd);
}

bool WPAClient::ReConfigure()
{
    string cmd = "RECONFIGURE";
    return CheckCommandWithOk(cmd);
}

bool WPAClient::ReConnect()
{
    string cmd = "RECONNECT";
    return CheckCommandWithOk(cmd);
}

bool WPAClient::SaveConfigure()
{
    string list_cmd = "LIST_NETWORKS";
    string recv;
    if (Request(wpa_ctrl_conn, list_cmd, recv))
    {
        PRINT_DBG("%s\n", recv.c_str());
    }
    string cmd = "SAVE_CONFIG";
    return CheckCommandWithOk(cmd);
}

}
