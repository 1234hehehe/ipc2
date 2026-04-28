/*For Get Network Card Info Phy Netcard Or Virtual NetCard*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
/**********************************************/
#include "libcommon_api.h"

#include "ovfs_comm_tool.h"
#include "ovfs_network_pppoe.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_utility_file_operate.h"

using namespace ovfs_soft;
using namespace ovfs_network;
extern char str_RNDISifname[8];

static S32 ovfs_pppoe_thread(Common_Thread_T hThreadHandle,void* para)
{
    ovfs_network_pppoe *pppoe = static_cast<ovfs_network_pppoe*>(para);
	
    while (1)
    {
        pppoe->refresh();
		Common_Sleep(1, 0);
    }

    LOGD("exit pid = %d!\n", getpid());
	
    return 0;
}

ovfs_network_pppoe::ovfs_network_pppoe()
{
    OVFS_CLR_ARG(m_eth_name);
    OVFS_CLR_ARG(m_pppoe_config);
    OVFS_CLR_ARG(m_pppoe_status);

    if(load_pppoe_cfg() == OVFS_SUCCESS)
    {
        m_config_changed = OVFS_TRUE;
    }

    OVFS_CLR_ARG(m_config_rw_lock);
    OVFS_CLR_ARG(m_status_rw_lock);
	Common_RWLock_Create(&m_config_rw_lock, NULL);
	Common_RWLock_Create(&m_status_rw_lock, NULL);
	
    m_thread_handle = NULL;
	Common_Thread_Create(&m_thread_handle,"pppoe",1024*256,
		                 COMMON_THREAD_CREATEFLAG_NORMAL,ovfs_pppoe_thread,(void *)this);	
}

ovfs_network_pppoe::~ovfs_network_pppoe()
{
	//Common_Thread_Destroy(&m_thread_handle);

    Common_RWLock_Destroy(&m_config_rw_lock);
	Common_RWLock_Destroy(&m_status_rw_lock);
}

OVFS_ERR ovfs_network_pppoe::set_config(const ovfs_pppoe_config *pppoe_config)
{
    if(pppoe_config == NULL)
    {
        LOGE("NULL Pointer\n");
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

	Common_RWLock_WLock(m_config_rw_lock);
    //If User Call set_config, No Matter The Config Option Has Modify, We Refresh Config File.
    memcpy(&m_pppoe_config, pppoe_config, sizeof(ovfs_pppoe_config));
    m_config_changed = OVFS_TRUE;
	ovfs_cfgm_set_pppoe_cfg(&m_pppoe_config);
	Common_RWLock_UnLock(m_config_rw_lock);

    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_pppoe::get_status(ovfs_pppoe_status *pppoe_status)
{
    if(pppoe_status == NULL)
    {
        LOGE("NULL Pointer\n");
        return OVFS_ERR_NETWORK_INVALID_PARA;
    }

    //Common_RWLock_RLock(m_status_rw_lock);
    memcpy(pppoe_status, &m_pppoe_status, sizeof(ovfs_pppoe_status));
    //Common_RWLock_UnLock(m_status_rw_lock);

    return OVFS_SUCCESS;
}

OVFS_VOID ovfs_network_pppoe::refresh()
{	
    Common_RWLock_RLock(m_status_rw_lock);
    Common_RWLock_RLock(m_config_rw_lock);

    OVFS_BOOL config_changed = m_config_changed;
    ovfs_pppoe_config pppoe_config = m_pppoe_config;
    ovfs_pppoe_status pppoe_status = m_pppoe_status;
    if(config_changed)
    {
        refresh_config();
        m_config_changed = OVFS_FALSE;
    }

    Common_RWLock_UnLock(m_config_rw_lock);
    Common_RWLock_UnLock(m_status_rw_lock);
    //LOGD("pppoe_config.enable: %d %d\n",pppoe_config.enable,pppoe_status.pppoe_ok);   
    if(pppoe_config.enable)
    {
        if(!pppoe_status.pppoe_ok)
        {
            connect();
        }            
        else if(config_changed)
        {
            disconnect();
            connect();
        }
    }
    else
    {
        if(pppoe_status.pppoe_ok)
        {
            disconnect();
        }
    }
}

OVFS_ERR ovfs_network_pppoe::connect()
{
    S8 system_cmd[64];
    snprintf(system_cmd, sizeof(system_cmd), "pppoe-start");
	LOGD("connect system_cmd: %s\n",system_cmd);
	Common_System(system_cmd);

	int trycount = 50;
	while(--trycount)
	{
		if(0 == NetWorkTool_GetIFFlags(str_RNDISifname))
			break;
		Common_Sleep(0, 100 * 1000);
	}
	if(trycount == 0) return OVFS_ERR_NETWORK_BASE;

	int family = 0;
	char ip[OVFS_IPV6_STRING_LEN + 1]={0},netmask[OVFS_IPV6_STRING_LEN + 1]={0};
	NetWorkTool_GetIfAddr(str_RNDISifname,ip,netmask,&family);
	m_pppoe_status.ip.is_ipv4 = (OVFS_BOOL)family;
	if(family == 1)
	{
		snprintf(m_pppoe_status.ip.ipv4,sizeof(m_pppoe_status.ip.ipv4),"%s",ip);
	}
	else
	{
		snprintf(m_pppoe_status.ip.ipv6,sizeof(m_pppoe_status.ip.ipv6),"%s",ip);
	}
	LOGD("m_pppoe_status ip=%s\n",m_pppoe_status.ip.ipv4);
	m_pppoe_status.pppoe_ok = OVFS_TRUE;
	ovfs_cfgm_set_pppoe_status(&m_pppoe_status);
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_pppoe::disconnect()
{
    S8 system_cmd[64];
    snprintf(system_cmd, sizeof(system_cmd), "pppoe-stop");
	LOGD("disconnect system_cmd: %s\n",system_cmd);	
	Common_System(system_cmd);
	memset(&m_pppoe_status,0,sizeof(m_pppoe_status));
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_pppoe::refresh_config()
{
    if(!Common_File_IsExist((S8 *)OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH))
    {
	    if(create_config_file() != OVFS_SUCCESS)
	    {
	        LOGE("create_config_file %s File Failed, PPPOE Pehaps Can't Connect!!!\n", OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH);
	        return OVFS_ERR_NETWORK_OPERATE_FAIL;
	    }		
    }
	else
	{
	    if(refresh_config_file() != OVFS_SUCCESS)
	    {
	        LOGE("refresh_config_file %s File Failed, PPPOE Pehaps Can't Connect!!!\n", OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH);
	        return OVFS_ERR_NETWORK_OPERATE_FAIL;
	    }
	}
	
    if(refresh_secrets_file(OVFS_PPPOE_PAP_FILE_ABSOLUTE_PATH) != OVFS_SUCCESS)
    {
        LOGE("refresh_secrets_file %s File Failed, PPPOE Pehaps Can't Connect!!!\n", OVFS_PPPOE_PAP_FILE_ABSOLUTE_PATH);
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    if(refresh_secrets_file(OVFS_PPPOE_CHAP_FILE_ABSOLUTE_PATH) != OVFS_SUCCESS)
    {
        LOGE("refresh_secrets_file %s File Failed, PPPOE Pehaps Can't Connect!!!\n", OVFS_PPPOE_CHAP_FILE_ABSOLUTE_PATH);
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_pppoe::create_config_file()
{
    S8 data_buffer[256];
	OVFS_CLR_ARG(data_buffer);
	txt_file_ops *file_operate = new txt_file_ops();
    OVFS_ASSERT(file_operate != NULL);

    if(file_operate->open(OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH, OVFS_FILE_OPEN_RW_TRUNC) != OVFS_SUCCESS)
    {
        LOGE("Open %s File Failed, PPPOE Pehaps Connect Failed\n", OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH);
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	
	
	snprintf(data_buffer, sizeof(data_buffer), "ETH='%s'\n", m_eth_name);
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "USER='%s'\n", m_pppoe_config.user_name);
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "DEMAND=no");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "DNSTYPE=SERVER");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "PEERDNS=yes");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "DNS1=");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "DNS2=");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "DEFAULTROUTE=yes");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "CONNECT_TIMEOUT=30");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "CONNECT_POLL=2");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "ACNAME=");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "SERVICENAME=");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "PING=\".\"");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "CF_BASE=`basename $CONFIG`");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }
	
	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "PIDFILE=\"/usr/etc/pppoe_adsl.pid\"");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "SYNCHRONOUS=no");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "CLAMPMSS=1412");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "LCP_INTERVAL=20");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "LCP_FAILURE=3");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "PPPOE_TIMEOUT=80");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "FIREWALL=NONE");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }		

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "LINUX_PLUGIN=");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "PPPOE_EXTRA=\"\"");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	snprintf(data_buffer, sizeof(data_buffer), "%s\n", "PPPD_EXTRA=\"\"");
    if(file_operate->write(data_buffer) != OVFS_SUCCESS)
    {
        delete file_operate;       
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }	

	file_operate->sync();
    file_operate->close();
	delete file_operate;

	return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_pppoe::refresh_config_file()
{
    S8 data_buffer[256];
    OVFS_CLR_ARG(data_buffer);
    S8 bak_file_absolute_path[OVFS_MAX_FILE_PATH_LEN];
    snprintf(bak_file_absolute_path, sizeof(bak_file_absolute_path), "%s%s", OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH, OVFS_PPPOE_FILE_BAK_SUFFIX);
    //ovfs_txt_file_operate *file_operate = ovfs_utility_new_txt_operate(OVFS_MODULE_USER_ID_NETWORK, "pppoe file operate");
    //ovfs_txt_file_operate *bak_file_operate = ovfs_utility_new_txt_operate(OVFS_MODULE_USER_ID_NETWORK, "pppoe bak file operate");
    txt_file_ops *file_operate = new txt_file_ops();
	txt_file_ops *bak_file_operate = new txt_file_ops();
    OVFS_ASSERT(file_operate != NULL);
    OVFS_ASSERT(bak_file_operate != NULL);

    //Create New Bak File.
    if(bak_file_operate->open(bak_file_absolute_path, OVFS_FILE_OPEN_RW_TRUNC) != OVFS_SUCCESS)
    {
        LOGE("Create %s File Failed, PPPOE Pehaps Connect Failed\n", bak_file_absolute_path);
        //ovfs_utility_delete_txt_operate(bak_file_operate, __FILE__, __FUNCTION__);
        //ovfs_utility_delete_txt_operate(file_operate, __FILE__, __FUNCTION__);
        delete file_operate;
		delete bak_file_operate;
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    //Open Current Config File.
    if(file_operate->open(OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH, OVFS_FILE_OPEN_RD) != OVFS_SUCCESS)
    {
        LOGE("Open %s File Failed, PPPOE Pehaps Connect Failed\n", OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH);
        bak_file_operate->close();
        //ovfs_utility_delete_txt_operate(bak_file_operate, __FILE__, __FUNCTION__);
        //ovfs_utility_delete_txt_operate(file_operate, __FILE__, __FUNCTION__);
        delete file_operate;
		delete bak_file_operate;        
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    while(file_operate->read(data_buffer, sizeof(data_buffer)) == OVFS_SUCCESS)
    {
        if(strncmp(data_buffer, "ETH=", sizeof("ETH=") - 1) == 0)
        {
            snprintf(data_buffer, sizeof(data_buffer), "ETH='%s'\n", m_eth_name);
        }
        else if(strncmp(data_buffer, "USER=", sizeof("USER=") - 1) == 0)
        {
            snprintf(data_buffer, sizeof(data_buffer), "USER='%s'\n", m_pppoe_config.user_name);
        }
        else if(strncmp(data_buffer, "DNSTYPE=", sizeof("DNSTYPE=") - 1) == 0)//Default We Get DNS From ISP
        {
            snprintf(data_buffer, sizeof(data_buffer), "DNSTYPE=SERVER\n");
        }

        if(bak_file_operate->write(data_buffer) != OVFS_SUCCESS)
        {
            LOGE("Write %s File Failed, PPPOE Pehaps Connect Failed\n", bak_file_absolute_path);
            LOGE("Why??????????\n");
            break;
        }
    }

    bak_file_operate->sync();
    bak_file_operate->close();
    file_operate->close();
    //ovfs_utility_delete_txt_operate(bak_file_operate, __FILE__, __FUNCTION__);
    //ovfs_utility_delete_txt_operate(file_operate, __FILE__, __FUNCTION__);
    delete file_operate;
	delete bak_file_operate;    
    bak_file_operate = file_operate = NULL;

    S8 system_cmd[128];
    snprintf(system_cmd, sizeof(system_cmd), "cp %s %s", bak_file_absolute_path, OVFS_PPPOE_CONFIG_FILE_ABSOLUTE_PATH);
    return (OVFS_ERR)Common_System(system_cmd);
}

OVFS_ERR ovfs_network_pppoe::refresh_secrets_file(const S8 *absolute_file_name)
{
    S8 data_buffer[1024];
    OVFS_CLR_ARG(data_buffer);
    ovfs_file_open_mode open_mode = OVFS_FILE_OPEN_RW_APPEND;
    if(!Common_File_IsExist((S8 *)absolute_file_name))
    {
        LOGW("%s Not Exist, We Create.", absolute_file_name);
        open_mode = OVFS_FILE_OPEN_RW_TRUNC;
    }

    txt_file_ops *file_operate = new txt_file_ops();
    OVFS_ASSERT(file_operate != NULL);

    if(file_operate->open(absolute_file_name, open_mode) != OVFS_SUCCESS)
    {
        LOGE("Open %s File Failed, PPPOE Pehaps Connect Failed\n", absolute_file_name);
        delete file_operate;
        return OVFS_ERR_NETWORK_OPERATE_FAIL;
    }

    snprintf(data_buffer, sizeof(data_buffer), "\"%s\"	*	\"%s\"\n", m_pppoe_config.user_name, m_pppoe_config.password);
    file_operate->write(data_buffer);
    file_operate->sync();
    file_operate->close();
    delete file_operate;
	
    return OVFS_SUCCESS;
}

OVFS_ERR ovfs_network_pppoe::load_pppoe_cfg()
{
    //Just For Test, This Should Load From xml file.
    /*
        <pppoe>
            <enable>int</enable>
            <user_name>str</user_name>
            <password>str</password>                    should I use 3des encode???
        </pppoe>
    */
    
    /*
        snprintf(m_eth_name, sizeof(m_eth_name), "eth0");
        m_pppoe_config.enable = OVFS_TRUE;
        snprintf(m_pppoe_config.user_name, sizeof(m_pppoe_config.user_name), "295389803");
        snprintf(m_pppoe_config.password, sizeof(m_pppoe_config.password), "zxd19910202ZXD");
    */

    //Dose This Api Is OK?????
    if(ovfs_cfgm_get_default_if(m_eth_name, sizeof(m_eth_name)) != OVFS_SUCCESS)
    {
        snprintf(m_eth_name, sizeof(m_eth_name), "eth0");
    }

    LOGD("PPPOE Use PHY %s\n", m_eth_name);
    if(ovfs_cfgm_get_pppoe_cfg(&m_pppoe_config) != OVFS_SUCCESS)
    {
        m_pppoe_config.enable = OVFS_FALSE;
    }

    LOGD("PPPOE Enable:%d\n", (S32)m_pppoe_config.enable);
    LOGD("PPPOE Username:%s\n", m_pppoe_config.user_name);
    LOGD("PPPOE Password:%s\n", m_pppoe_config.password);

    return OVFS_SUCCESS;
}
