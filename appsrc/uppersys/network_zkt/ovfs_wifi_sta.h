#ifndef _OVFS_WIFI_STA_MODE_H_
#define _OVFS_WIFI_STA_MODE_H_


#include "ovfs_wifi_common.h"

namespace wifi {
using namespace std;

const string WPA_PATH = "/var/run/wpa_supplicant/wlan0";  //进程间通信地址加上网络接口额名称

typedef struct wpa_ctrl {
	int s;
	struct sockaddr_un local;
	struct sockaddr_un dest;
}WPAContext;

struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path);
/* 建立并初始化一个Unix domain socket的client结点，并与作为server的wpa_supplicant结点绑定 */

void wpa_ctrl_close(struct wpa_ctrl *ctrl);
/* 撤销并销毁已建立的Unix domain socket的client结点 */

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
                   char *reply, size_t *reply_len,
                   void (*msg_cb)(char *msg, size_t len));
/* 用户模块直接调用该函数对wpa_supplicant发送命令并获取所需信息 */

int wpa_ctrl_attach(struct wpa_ctrl *ctrl);
/* 注册 某个 control interface 作为 monitor interface */

int wpa_ctrl_detach(struct wpa_ctrl *ctrl);
/* 撤销某个 monitor interface 为 普通的 control interface  */

int wpa_ctrl_pending(struct wpa_ctrl *ctrl);
/* 判断是否有挂起的event 事件 */

int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len);
/* 获取挂起的event 事件 */


class WPAClient {
public:
  WPAClient();
  ~WPAClient();
  bool Init();
  bool UnInit();
  bool GetInitStatus(){return wpa_ctrl_conn!=nullptr;}   //获取wpa进程间通信是否建立连接
  int GetWiFiRssi();         //获取wifi信号强度，需要在连接成功之后调用
  string GetCurrentSSID();   //获取当前连接的wifi的名称
  bool CleanAllWiFi();//清除已保存WiFi列表
  bool EnableAllWiFi();//启用已保存WiFi列表
  bool ReConfigure();
  bool ReConnect();
  bool SaveConfigure();
  bool ConnectWiFi(const string& ssid, const string& password,int en_crypt,int priority);  //连接加密wifi，传入wifi名称和密码
  bool SaveWiFi(const string& ssid, const string& password,int en_crypt,int priority);//保存网络
  bool ForgetWiFi(const std::string& ssid);//忘记网络
  WIFI_WORK_STA_STATUS_E UpdateStatus(WIFI_STA_CONNECTIONCFG_S *connCfg);    //获取wifi连接状态
  bool Scan();
  bool GetScanResult(WIFI_SCANAPITEM_S **scanresultList,int *totalNum);
protected:
  bool Request(WPAContext * context, const string & cmd, string& reply);
  
  bool CheckCommandWithOk(const std::string cmd);
  bool AddWiFi(int& id);
  bool SetScanSSID(int id);
  bool SetSSID(const string& ssid, int id);
  bool SetPassword(const string& password, int id);
  bool SetProtocol(int id, int en_crypt);
  bool SetPriority(int id, int priority);
  bool EnableWiFi(int id);
protected:
  struct wpa_ctrl* wpa_ctrl_conn;
  struct wpa_ctrl* wpa_monitor_conn;
private:
    mutex wpa_lock;
public:
    WIFI_STA_STATUS status;
};

class STAMode
{
public:
    WIFI_STA_CONNECTIONCFG_S connectCfg;

private:
    //mutex wpa_lock;
public:
    STAMode();
    ~STAMode();

    bool start(void);//启动wpa_supplicant服务
    bool stop(void);//停止STA模式
    bool isReady(void);//获取是否准备就绪，wpa进程间通信是否建立连接
    bool scan(WIFI_SCANAPITEM_S **scanresultList,int *totalNum);//扫描周围热点列表
    bool connect(void);//连接wpa进程间通信
    bool disconnect(void);//断开wpa进程间通信
    bool autoconnect(void);//更新优先级并自动连接已保存WiFi列表
    bool removeall(void);//清除已保存WiFi列表
    bool enableall(void);//启用已保存WiFi列表
    bool checkParam(WIFI_STA_CONNECTIONCFG_S cfg);
    bool saveConf(WIFI_STA_CONNECTIONCFG_S conf);
    bool save(WIFI_STA_CONNECTIONCFG_S connectCfg);//保存WiFi
    bool forget(WIFI_STA_CONNECTIONCFG_S connectCfg);//忘记WiFi
    WIFI_WORK_STA_STATUS_E GetConnStatus();
    WIFI_WORK_STA_STATUS_E UpdateStatus();
    bool writeConfig(cJSON_Struct *jsonCfg);
    bool readConfig(cJSON_Struct **jsonCfg);
    bool getCurStatus(cJSON_Struct *jsonCfg);
    int getlist(cJSON_Struct *parentItem);//获取保存WiFi列表
    bool checkIsCurSsid(char *ssid);
    bool update(void);//连接上网络之后更新网络配置
protected:
      WPAClient *wpa;
};

}
#endif

