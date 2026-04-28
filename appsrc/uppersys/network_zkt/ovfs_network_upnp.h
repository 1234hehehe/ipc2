#ifndef _OVFS_NETWORK_UPNP_H_
#define _OVFS_NETWORK_UPNP_H_

//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_comm_errno.h"
#include "ovfs_comm_def.h"
//#include "ovfs_comm_sys.h"
//#include "ovfs_utility_def.h"
#include "ovfs_network_def.h"

using namespace ovfs_soft;
//using ovfs_soft::OVFS_ERR;
//using ovfs_soft::ovfs_lock;
//using ovfs_soft::ovfs_port_type;
//using ovfs_soft::ovfs_upnp_port_protocol;

/***********************************************UPNP端口映射实现策略*********************************************************/
/*端口信息永远保存用户配置的最新信息(配置一次覆盖一次不管上次映射成功没有，简化操作逻辑) */
/*内部不去主动的删除端口信息，而是在映射的时候就采用一个固定租期的方式去映射端口,当租期到*/
/*时间到了，只要不续约(重新映射)，在路由器上端会自动删除该映射(这样不会导致用户总是在变动*/
/*端口信息从而导致路由器映射越来越多的情况)这样不管用户配置了多少映射的端口，也不管以前的*/
/*的配置成功与否，内部永远只需要维持一份最新的端口配置表即可，如果在租期时间内UPNP还处于使*/
/*能状态(用户没有停用UPNP)，内部模块开始向路由器发包续约即可*/

namespace ovfs_network
{

//当前采用第三方工具的方法使能或者UNPNP，为了简单方便不采用比较高深的进程间通信机制(IPC)
//在每次执行完毕系统调用后去Check一下该文件
#define OVFS_UPNP_CHECK_FILE_PATH                       ("/var/upnp.tmp")

//UPNP执行结果标识(第三方工具内部与其保持统一)
#define OVFS_UPNPC_EXTERNAL_IP                             ("External IP Address:")
#define OVFS_UPNPC_OPERATION_RESULT_PREFIX  ("UPNP Result Of Operation:")
#define OVFS_UPNPC_RESULT_SUCCESS                     ("SUCCESS")
#define OVFS_UPNPC_RESULT_FAILED                        ("FAILED")



//系统网卡信息，当配置端口对应的网卡IP地址发生变动时要重新进行UPNP端口映射
typedef struct
{
    S8 name[OVFS_MAX_ETH_IFNAME_SIZE];
    S8 last_ip_address[sizeof("xxx.xxx.xxx.xxx") + 1];
    S8 current_ip_address[sizeof("xxx.xxx.xxx.xxx") + 1];
}ovfs_upnp_network_card_info;

class ovfs_network_upnp
{
public:
    //使能UPNP
    OVFS_ERR enable_upnp();

    //停止UPNP
    OVFS_ERR disable_upnp();

    //获取UPNP工作状态
    OVFS_ERR upnp_is_start(OVFS_BOOL *is_start);

    //设置UPNP端口信息如果未配置external_port端口号，external_port端口号与 internal_port一致，若未配置port_protocol端口映射将默认使用TCP协议，若eth_name未配置默认使用默认网卡的IP地址映射
    OVFS_ERR set_port_info(OVFS_BOOL enable_upnp, U16 internal_port,  U16 external_port = 0, ovfs_upnp_port_protocol port_protocol  = ovfs_soft::OVFS_PROTOCOL_TCP, const S8 *eth_name = NULL);

    //获取UPNP端口信息
    OVFS_ERR get_port_info(S32 index, ovfs_upnp_port_detail_info *p_port_detail_info);

public:
    //线程工作主函数
    OVFS_VOID work();

private:
    //根据端口信息进行映射
    OVFS_ERR mapping(const ovfs_upnp_port_detail_info *port_info, char *externalIP, size_t ip_len);

    //检查默认路由是否发生变动
    //注意:代码处理中其实有一个强制性的机制，端口的绑定只能发生在一个网口上，此网口是连接外网
    //的网口,如果存在多个连接外网的网卡，代码需要修改，界面也需要修改.
    OVFS_VOID check_default_route_changes();

    //检查端口IP地址变动情况(若端口对应的网口IP地址发生变化要进行重新映射)
    OVFS_VOID check_port_ip_changes();

    //根据网卡名称获取网口IP地址
    OVFS_ERR get_ip_by_eth_name(const S8 *eth_name, S8 *ip_address, size_t ip_addr_size);

    //获取配置文件
    OVFS_ERR load_upnp_cfg();

    //获取系统网卡信息
    OVFS_ERR load_network_card_info();

    //根据端口类型生成端口的描述字符串
    OVFS_VOID generate_descrip_by_port_type(S32 index, S8 *description, size_t buff_len);

	
private:    
    OVFS_BOOL m_upnp_enable;//是否使能UPNP(总开关)
    ovfs_upnp_port_detail_info m_port_detail_info[OVFS_PORT_MAX];//UPNP端口信息

    //系统网卡信息，用于检测端口配置的网卡IP地址是否变动    
    U32 m_network_card_num;
    ovfs_upnp_network_card_info m_network_card_info[OVFS_MAX_ETH_CARD_NUM];
    
    time_t m_last_lease_time;//上一次(第一个端口参与映射的时间)端口映射的时间
    U32 m_lease_duration;//端口映射的租期(租期到了路由器自动删除该端口的映射)，单位为s

    Common_Lock_T m_lock;
    Common_Thread_T m_thread_handle;
    
public:
    ovfs_network_upnp();
    ~ovfs_network_upnp();
    const char *class_name() {return "ovfs_network_upnp";};

private:
    ovfs_network_upnp(const ovfs_network_upnp &other);
    ovfs_network_upnp&operator=(const ovfs_network_upnp &other); 
}; //END class ovfs_network_upnp

}//END namespace ovfs_network
#endif //END #ifndef _OVFS_NETWORK_UPNP_H_