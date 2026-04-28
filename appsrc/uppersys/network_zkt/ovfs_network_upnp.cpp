/*For Get Network Card Info Phy Netcard Or Virtual NetCard*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
/**********************************************/

#include "ovfs_comm_tool.h"
#include "ovfs_network_upnp.h"
#include "ovfs_cfg_manage_api.h"
#include "libcommon_api.h"
#include "ovfs_network_resource.h"

using namespace ovfs_soft;
using namespace ovfs_network;

static S32 ovfs_upnp_thread(Common_Thread_T hThreadHandle,void* para)
{
    ovfs_network_upnp *upnp = static_cast<ovfs_network_upnp*>(para);
	
    LOGI("enter pid = %d!\n", getpid());

    while (1)
    {
        upnp->work();
		Common_Sleep(10, 0);
    }

    LOGI("exit pid = %d!\n", getpid());
    return 0;
}

ovfs_network_upnp::ovfs_network_upnp()
{
    m_upnp_enable = OVFS_FALSE;   
    for(U32 i = 0; i < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); i++)
    {
        memset(&m_port_detail_info[i], 0, sizeof(ovfs_upnp_port_detail_info));
        //m_port_detail_info[i].port_type = (ovfs_port_type)i;
        snprintf(m_port_detail_info[i].externalIP, sizeof(m_port_detail_info[i].externalIP), "%s", "0.0.0.0");
    }

    m_network_card_num = 0;
    for(U32 i = 0; i < sizeof(m_network_card_info) / sizeof(m_network_card_info[0]); i++)
    {
        memset(&m_network_card_info[i], 0, sizeof(ovfs_upnp_network_card_info));
        snprintf(m_network_card_info[i].last_ip_address, sizeof(m_network_card_info[i].last_ip_address), "%s", "0.0.0.0");
        snprintf(m_network_card_info[i].current_ip_address, sizeof(m_network_card_info[i].current_ip_address), "%s", "0.0.0.0");
    }
    
    load_network_card_info();
    load_upnp_cfg();
    
    m_last_lease_time = 0;
    m_lease_duration = 2 * 60 * 60; //默认租期为两个小时

	OVFS_CLR_ARG(m_lock);
	Common_Lock_Create(&m_lock,NULL);
	
    m_thread_handle = NULL;
	Common_Thread_Create(&m_thread_handle,"upnp",1024*128,
		                 COMMON_THREAD_CREATEFLAG_NORMAL,ovfs_upnp_thread,(void *)this);		
}

ovfs_network_upnp::~ovfs_network_upnp()
{
    //Common_Thread_Destroy(&m_thread_handle);

    Common_Lock_Destroy(&m_lock);
}

OVFS_VOID ovfs_network_upnp::work()
{
    Common_Lock(m_lock);
    if(!m_upnp_enable)
    {
		Common_UnLock(m_lock);
        return ;
    }

    //检查默认路由是否发生变化
    check_default_route_changes();
    
    for(U32 i = 0; i < m_network_card_num; i++)
    {
        //更新网卡最新的IP地址
        if(get_ip_by_eth_name(m_network_card_info[i].name, m_network_card_info[i].current_ip_address, sizeof(m_network_card_info[0].current_ip_address)) != OVFS_SUCCESS)
        {
            LOGW("Fetch Network Card Name:%s IP Failed\n", m_network_card_info[i].name);
        }
    }

    //使用获取的网卡最新IP地址，检查端口绑定的网卡IP地址是否发生变动
    //如果发生变动需要进行重新映射
    check_port_ip_changes();
    
    Common_UnLock(m_lock);

    for(U32 index = 0; index < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); index++)
    {
        Common_Lock(m_lock);
        ovfs_upnp_port_detail_info tmp_info = m_port_detail_info[index];
        //端口第一次映射，上一次未映射成功重复映射，租期快到了都开始重新映射一下
        if((tmp_info.enable_upnp && !tmp_info.mapping_success) || (tmp_info.need_re_lease))
        {
            if(m_last_lease_time == 0)
            {
                //以第一个开始映射的时间点为起始点
                m_last_lease_time = time(NULL);
            }
        }
        else
        {
            Common_UnLock(m_lock);
            continue;
        }
        Common_UnLock(m_lock);

        //耗时操作不要上锁
        S8 externalIP[sizeof("xxx.xxx.xxx.xxx") + 1];
        snprintf(externalIP, sizeof(externalIP), "%s", "0.0.0.0");
        OVFS_ERR map_ret = mapping(&tmp_info, externalIP, sizeof(externalIP));
             
        Common_Lock(m_lock);
        //之所以添加memcmp是防止在进行耗时操作的时候用户又更改了端口信息，不能贸然的
        //将m_port_detail_info[index].mapping_success至为true否则上层获取的状态可能是错误的
        if((map_ret == OVFS_SUCCESS) && (memcmp(&tmp_info, &m_port_detail_info[index], sizeof(ovfs_upnp_port_detail_info)) == 0)
            && (strncmp(externalIP, "0.0.0.0", sizeof("0.0.0.0") != 0)))
        {
            m_port_detail_info[index].mapping_success = OVFS_TRUE;
            m_port_detail_info[index].need_re_lease = OVFS_FALSE;
            if(strlen(externalIP) < strlen("0.0.0.0"))
                snprintf(m_port_detail_info[index].externalIP, sizeof(m_port_detail_info[index].externalIP), "%s", "0.0.0.0");           
            else
                snprintf(m_port_detail_info[index].externalIP, sizeof(m_port_detail_info[index].externalIP), "%s", externalIP);
			
			///获取成功调用一下ddns的wakeup,让线程不要再睡了。
			get_res_manage()->get_ddns()->wakeup_ddns(); 
			
        }
        Common_UnLock(m_lock);        
    }

    Common_Lock(m_lock);
    time_t current_time = time(NULL);    
    if((m_last_lease_time != 0) && ((current_time < m_last_lease_time) || (current_time - m_last_lease_time > (S32)m_lease_duration - 60 * 60)))
    {
        for(U32 index = 0; index < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); index++)
        {
            m_port_detail_info[index].need_re_lease = OVFS_TRUE;
        }

        m_last_lease_time = 0;//下一次准备续租了，将上一次的时间点清0
    }
    Common_UnLock(m_lock);
    
    return ;
}


OVFS_ERR ovfs_network_upnp::enable_upnp()
{
    Common_Lock(m_lock);
    m_upnp_enable = OVFS_TRUE;
    Common_UnLock(m_lock);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_upnp::disable_upnp()
{
    Common_Lock(m_lock);
    m_upnp_enable = OVFS_FALSE;

    m_last_lease_time = 0;
    for(U32 i = 0; i < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); i++)
    {
        m_port_detail_info[i].mapping_success = OVFS_FALSE;
        m_port_detail_info[i].need_re_lease = OVFS_FALSE;
        snprintf(m_port_detail_info[i].externalIP, sizeof(m_port_detail_info[i].externalIP), "%s", "0.0.0.0");
    }
    Common_UnLock(m_lock);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_upnp::upnp_is_start(OVFS_BOOL *is_start)
{
    if(is_start == NULL)
    {
        LOGE("NULL Pointer\n");
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }
	
    Common_Lock(m_lock);
    *is_start = m_upnp_enable ;
    Common_UnLock(m_lock);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_upnp::set_port_info(OVFS_BOOL enable, U16 internal_port,  U16 external_port, ovfs_upnp_port_protocol port_protocol, const S8 *eth_name)
{
    S32 index = -1, i = 0;
    S8 real_eth_name[OVFS_MAX_ETH_IFNAME_SIZE];
        
    if(!enable)
    {
        //对于不需要映射的端口等待路由器自动删除，只要不续租即可
        return OVFS_SUCCESS;
    }
	
    if(eth_name == NULL)
    {
        if(ovfs_cfgm_get_default_if(real_eth_name, sizeof(real_eth_name)) != OVFS_SUCCESS)
        {
            snprintf(real_eth_name, sizeof(real_eth_name), "eth0");
        }
    }
    else
    {
        snprintf(real_eth_name, sizeof(real_eth_name), "%s", eth_name);
    }
	
    Common_Lock(m_lock);

	for (i = 0; i < OVFS_PORT_MAX; i++)
	{
	    if (m_port_detail_info[i].internal_port == 0)
	    {
	        if (index == -1)
	        {
                index = i;
			}
            continue;
		}
		
        if (m_port_detail_info[i].internal_port == internal_port && internal_port != 0)
        {
            index = i;
		    if(external_port == 0)
		    {
		        memset(&m_port_detail_info[i], 0, sizeof(ovfs_upnp_port_detail_info));
		        snprintf(m_port_detail_info[i].externalIP, sizeof(m_port_detail_info[i].externalIP), "%s", "0.0.0.0");			
				Common_UnLock(m_lock);
				return OVFS_SUCCESS;
		    }				
            break;
		}
	}

	if(index >= (S32)(sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0])) || index == -1)
    {
        LOGE("index OverFlow\n");
		Common_UnLock(m_lock);
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    if(external_port == 0)
    {
        external_port = internal_port;
    }

    //一个端口---->IP,内外端口都相同且已经映射成功了就不用重新映射了
    if((m_port_detail_info[index].internal_port == internal_port) && (m_port_detail_info[index].external_port == external_port)
        && (m_port_detail_info[index].port_protocol == port_protocol) && (strcmp(real_eth_name, m_port_detail_info[index].eth_name) == 0)
        && m_port_detail_info[index].mapping_success)
    {
        LOGE("Port Type %d Internal Port %hu<------>External Port %hu Have Been Mapping\n", port_protocol, internal_port, external_port);
        Common_UnLock(m_lock);
        return OVFS_SUCCESS;
    }
     
    m_port_detail_info[index].enable_upnp = enable;
    m_port_detail_info[index].port_protocol = port_protocol;
    m_port_detail_info[index].internal_port = internal_port;
    m_port_detail_info[index].external_port = external_port;
    m_port_detail_info[index].mapping_success = OVFS_FALSE;
    m_port_detail_info[index].need_re_lease = OVFS_FALSE;
    snprintf(m_port_detail_info[index].externalIP, sizeof(m_port_detail_info[index].externalIP), "%s", "0.0.0.0");
    snprintf(m_port_detail_info[index].eth_name, sizeof(m_port_detail_info[index].eth_name), "%s", real_eth_name);
    Common_UnLock(m_lock);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_upnp::get_port_info(S32 index, ovfs_upnp_port_detail_info *p_port_detail_info)
{	
    if(index >= (S32)(sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0])))
    {
        LOGE("index OverFlow\n");
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    Common_Lock(m_lock);
	
    memcpy(p_port_detail_info, &m_port_detail_info[index], sizeof(ovfs_upnp_port_detail_info));
	
    Common_UnLock(m_lock);
	
    return OVFS_SUCCESS;
}


OVFS_VOID ovfs_network_upnp::check_default_route_changes()
{
    S8 default_route_card[OVFS_MAX_ETH_IFNAME_SIZE];        
    if(ovfs_cfgm_get_default_if(default_route_card, sizeof(default_route_card)) != OVFS_SUCCESS)
    {
        snprintf(default_route_card, sizeof(default_route_card), "eth0");
    }

    for(U32 i = 0; i < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); i++)
    {
        if(strcmp(m_port_detail_info[i].eth_name, default_route_card) != 0)
        {
            m_port_detail_info[i].mapping_success = OVFS_FALSE;
            m_port_detail_info[i].need_re_lease = OVFS_FALSE;
			snprintf(m_port_detail_info[i].eth_name, sizeof(m_port_detail_info[i].eth_name), "%s", default_route_card);
            snprintf(m_port_detail_info[i].externalIP, sizeof(m_port_detail_info[i].externalIP), "%s", "0.0.0.0");
        }
    }
}

OVFS_VOID ovfs_network_upnp::check_port_ip_changes()
{  
    for(U32 i = 0; i < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); i++)
    {
        ovfs_upnp_network_card_info *match_card_info = NULL;
        for(U32 j = 0; j < m_network_card_num; j++)
        {
            if(strcmp(m_network_card_info[j].name, m_port_detail_info[i].eth_name) == 0)
           {
                match_card_info = &m_network_card_info[j];
                break;
            }
        }

        //理论上端口绑定的网口，一定在在侦测的网卡范围内
        OVFS_ASSERT(match_card_info != NULL);
        if(match_card_info == NULL)
        {
            continue;
        }

        if(strcmp(match_card_info->last_ip_address, match_card_info->current_ip_address) != 0)
        {                            
            m_port_detail_info[i].mapping_success = OVFS_FALSE;
            m_port_detail_info[i].need_re_lease = OVFS_FALSE;
            snprintf(m_port_detail_info[i].externalIP, sizeof(m_port_detail_info[i].externalIP), "%s", "0.0.0.0");
        }
    }

    //将历时IP地址更新(这一次的已经失效了)
    for(U32 i = 0; i < m_network_card_num; i++)
    {
        snprintf(m_network_card_info[i].last_ip_address, sizeof(m_network_card_info[i].last_ip_address), m_network_card_info[i].current_ip_address);
    }
}

OVFS_ERR ovfs_network_upnp::mapping(const ovfs_upnp_port_detail_info *port_info, char *externalIP, size_t ip_len)
{
    S32 try_cnt = 3;
    //S8 description[64];
    S8 system_cmd[256];
    S8 ipv4_address[OVFS_IPV4_STRING_LEN];
	S8 ipv4_gw[OVFS_IPV4_STRING_LEN];
    OVFS_CLR_ARG(ipv4_address);
	OVFS_CLR_ARG(ipv4_gw);
    snprintf(externalIP, ip_len, "%s", "0.0.0.0");
    if(get_ip_by_eth_name(port_info->eth_name, ipv4_address, sizeof(ipv4_address)) != OVFS_SUCCESS)
    {
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    //generate_descrip_by_port_type(port_info->port_type, description, sizeof(description));

	NetWorkTool_GetDefaultRoute(ipv4_gw);
#if 1
    snprintf(system_cmd, sizeof(system_cmd), "%s -e %s -a %s %d %d %s %d %s > %s", OVFS_UPNP_TOOL_PATH, "OVFS_UPnP", ipv4_address, port_info->internal_port, port_info->external_port, \
            (port_info->port_protocol == OVFS_PROTOCOL_TCP) ? "TCP" : "UDP", m_lease_duration, ipv4_gw,OVFS_UPNP_CHECK_FILE_PATH);

#else
	snprintf(system_cmd, sizeof(system_cmd), "%s -e %s -a %s %d %d %s > %s", OVFS_UPNP_TOOL_PATH, description, ipv4_address, port_info->internal_port, port_info->external_port, \
            (port_info->port_protocol == OVFS_PROTOCOL_TCP) ? "TCP" : "UDP", OVFS_UPNP_CHECK_FILE_PATH);
#endif
    while(try_cnt--)
    {
        LOGD("System Call:%s\n", system_cmd);
		LOGE("System Call:%s\n", system_cmd);
        if(Common_System(system_cmd) != OVFS_SUCCESS)
        {
            Common_Sleep(0,10000);
            continue;
        }
        
        FILE *fd = fopen(OVFS_UPNP_CHECK_FILE_PATH, "r");
        if (fd == NULL)
        {
            Common_Sleep(0,10000);
            continue;
        }

        char buf[128];
        OVFS_BOOL mapping_success = OVFS_FALSE;
        while(fgets(buf, sizeof(buf), fd) != NULL)
        {
            char *external_ip = strstr(buf, OVFS_UPNPC_EXTERNAL_IP);            
            char *success_flag = strstr(buf, OVFS_UPNPC_RESULT_SUCCESS);
            
            if(external_ip != NULL)
            {                
                snprintf(externalIP, ip_len, "%s", external_ip + strlen(OVFS_UPNPC_EXTERNAL_IP));
                //去掉fgets中的\n字符
                for(U32 i = 0; i < strlen(externalIP); i++)
                {
                    if(externalIP[i] == '\n')
                    {                        
                        externalIP[i] = '\0';
                        break;
                    }
                }
            }
            
            if(success_flag != NULL)
            {
                mapping_success = OVFS_TRUE;
            }
        }
            
        fclose(fd);
        fd = NULL;        
        if(mapping_success)
        {
            return OVFS_SUCCESS;
        }
    }

    return OVFS_ERR_NETWORK_OPERATE_FAIL;
}


OVFS_ERR ovfs_network_upnp::get_ip_by_eth_name(const S8 *eth_name, S8 *ip_address, size_t ip_addr_size)
{
    struct ifreq ifr;
    struct ifconf ifc;
    struct sockaddr_in *sin = NULL;
    OVFS_CLR_ARG(ifr);
    OVFS_CLR_ARG(ifc);
	
    S8 array_data[2048];
    OVFS_CLR_ARG(array_data);
    OVFS_ERR ret = OVFS_ERR_NETWORK_OPERATE_FAIL;

    S32 socket_fd = Common_Socket_Open(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socket_fd == -1) 
    {
        LOGE("Open Socket For Get Network Card Info Failed\n");
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    ifc.ifc_len = sizeof(array_data);
    ifc.ifc_buf = array_data;
    if (ioctl(socket_fd, SIOCGIFCONF, &ifc) == -1) 
    {
        LOGE("PPPOE Ioctl  SIOCGIFCONF Failed\n");
        Common_Socket_Close(socket_fd);
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    for (it = ifc.ifc_req; it != end; ++it)
    {
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", it->ifr_name);
        if(strcmp(ifr.ifr_name, eth_name) != 0)
            continue;

        if (ioctl(socket_fd, SIOCGIFADDR, &ifr) == 0) 
        {
            sin = (struct sockaddr_in *)(void *)&ifr.ifr_addr;
            snprintf(ip_address, ip_addr_size, "%s", inet_ntoa(sin->sin_addr));
            LOGD("Interface name : %s , IP address : %s\n", ifr.ifr_name,ip_address);
            ret = OVFS_SUCCESS;
            break;	
        }
    }

    Common_Socket_Close(socket_fd);
    return ret;
}

OVFS_ERR ovfs_network_upnp::load_upnp_cfg()
{
    /*Load Cfg Form XML
        <upnp>
        <enable>"int"</enable>
        <ovfs_port_type(int) eth_name="str" protocol="int" internal_port="int" externel_port="int" enable_upnp="int">
        <ovfs_port_type(int) eth_name="str" protocol="int" internal_port="int" externel_port="int" enable_upnp="int">
        <ovfs_port_type(int) eth_name="str" protocol="int" internal_port="int" externel_port="int" enable_upnp="int">
        ...
        ...
        <ovfs_port_type(int) eth_name="str" protocol="int" internal_port="int" externel_port="int" enable_upnp="int">
        </upnp>
    */
	
    if(ovfs_cfgm_get_upnp_cfg_ex(&m_upnp_enable) == OVFS_SUCCESS)
    {
        //LOGD("\n\nUPNP Enable:%d\n", m_upnp_enable);		
        for(U32 index = 0; index < sizeof(m_port_detail_info) / sizeof(m_port_detail_info[0]); index++)
        {
            ovfs_cfgm_get_upnp_port_cfg((S32)index, m_port_detail_info[index].eth_name, sizeof(m_port_detail_info[index].eth_name),  &m_port_detail_info[index].internal_port, \
                &m_port_detail_info[index].external_port, &m_port_detail_info[index].enable_upnp, &m_port_detail_info[index].port_protocol);

            if(m_port_detail_info[index].external_port == 0)
            {
                m_port_detail_info[index].external_port = m_port_detail_info[index].internal_port;
            }

            //LOGD("\n\nPort Index:%u\n", index);
            //LOGD("Eth Name:%s\n", m_port_detail_info[index].eth_name);
            //LOGD("Protocol:%s\n", (m_port_detail_info[index].port_protocol == OVFS_PROTOCOL_TCP) ? "TCP" : "UDP");
            //LOGD("Internal Port: %hu\n", m_port_detail_info[index].internal_port);
            //LOGD("External Port:%hu\n", m_port_detail_info[index].external_port);
            //LOGD("Enable:%d\n\n\n", m_port_detail_info[index].enable_upnp);		
        }
    }
    else
    {
        m_upnp_enable = OVFS_FALSE;
    }	

	return OVFS_SUCCESS; 
}

//获取系统网卡信息
OVFS_ERR ovfs_network_upnp::load_network_card_info()
{
    OVFS_ERR ret = OVFS_ERR_NETWORK_OPERATE_FAIL;
    const ovfs_network_capability *network_capability = ovfs_cfgm_get_network_cap();
    OVFS_ASSERT(network_capability != NULL);
    if(network_capability == NULL)
    {
        LOGE("network_capability Is Empty, That's is impossible, Why??????????\n");
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    //理论上一个开发板的网卡数目应该是大于0的
    OVFS_ASSERT(network_capability->total_eth_num + network_capability->bond_num > 0);
    if(network_capability->total_eth_num + network_capability->bond_num == 0)
    {
        LOGE("Total Network Card Is 0(Include Bond), That's So Strange, Why????????\n");
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    for(U32 i = 0; i < network_capability->total_eth_num; i++)
    {
        if(m_network_card_num >= sizeof(m_network_card_info) / sizeof(m_network_card_info[0]))
        {
            LOGE("Network Array Overflow Real Num %u >= Array Num %u\n", m_network_card_num, sizeof(m_network_card_info) / sizeof(m_network_card_info[0]));
            break;
        }

        //如果网卡存在且可路由(表明可与路由器进行通信)则添加至侦测数组内
        if(network_capability->eth_status[i].is_exist && (network_capability->eth_status[i].ability_mask &  (1 << 2)))
        {
            snprintf(m_network_card_info[m_network_card_num].name, sizeof(m_network_card_info[0].name), network_capability->eth_status[i].if_name);
            m_network_card_num++;
        }       
    }

    for(U32 i = 0; i < network_capability->bond_num; i++)
    {
        if(m_network_card_num >= sizeof(m_network_card_info) / sizeof(m_network_card_info[0]))
        {
            LOGE("Network Array Overflow Real Num %u >= Array Num %u\n", m_network_card_num, sizeof(m_network_card_info) / sizeof(m_network_card_info[0]));
            break;
        }

        if(network_capability->bond_status[i].is_support)
        {
            snprintf(m_network_card_info[m_network_card_num].name, sizeof(m_network_card_info[0].name), network_capability->bond_status[i].bound_name);
            m_network_card_num++;
        }
    }

    LOGD("Upnp Detect Network Card Total Num: %u\n", m_network_card_num);
    //OK，获取一下当前的IP地址
    for(U32 i = 0; i < m_network_card_num; i++)
    {
        ret = get_ip_by_eth_name(m_network_card_info[i].name, m_network_card_info[i].current_ip_address, sizeof(m_network_card_info[0].current_ip_address));
        if(ret != OVFS_SUCCESS)
        {
            LOGW("Fetch Network Card Name:%s IP Failed\n", m_network_card_info[i].name);
        }

        LOGD("Detect Network Card Index %u, Name: %s, IP: %s\n", i, m_network_card_info[i].name, m_network_card_info[i].current_ip_address);
    }

	return ret;
}


//根据端口类型生成端口的描述字符串
OVFS_VOID ovfs_network_upnp::generate_descrip_by_port_type(S32 index, S8 *description, size_t buff_len)
{
#if 0
    switch(port_type)
    {
        case OVFS_PORT_RTSP:
            snprintf(description, buff_len, "IPC2.0-RTSP");
            break;
            
        case OVFS_PORT_RTMP:
            snprintf(description, buff_len, "IPC2.0-RTMP");
            break;
            
        case OVFS_PORT_HTTP:
            snprintf(description, buff_len, "IPC2.0-HTTP");
            break;
			
        case OVFS_PORT_HTTPS:
            snprintf(description, buff_len, "IPC2.0-HTTPS");
            break;			

		case OVFS_PORT_ONVIF:
            snprintf(description, buff_len, "IPC2.0-ONVIF");
            break;
            
        default:
            snprintf(description, buff_len, "Unknow");
            break;
    }
#endif	

    return ;
}
