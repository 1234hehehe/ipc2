#ifndef _OVFS_NETWORK_BASE_DEF_H_
#define _OVFS_NETWORK_BASE_DEF_H_

#include "ovfs_comm_errno.h"
#include "ovfs_comm_def.h"
//#include "ovfs_comm_sys.h"
//#include "ovfs_comm_type.h"
#include "libcommon_api.h"
#include "ovfs_network_def.h"
#include "i8_ddns_api.h"
#include <string>
#include <iostream>    
#include <sstream>    
#include <fstream> 

#define OVFS_MAX_EMAIL_RECIEVER_NUM 64
#define OVFS_MAX_EMAIL_ATTACHMENT 64
#define OVFS_MAX_EMAIL_TASK_NUM 256
#define OVFS_MAX_EMAIL_SEND_TIMES 1

#define OVFS_MAX_EMAIL_RECV_MAS_NUM 4
#define OVFS_SOCKET_ERROR -1



//#define OVFS_RC_MUTTPATH		"/usr/local/rcmutt"
//#define OVFS_RC_MUTTPATH_TEST	"/usr/local/rcmutt_test"

//#define OVFS_RC_MSMTPPATH		"/usr/local/msmtprc"
//#define OVFS_RC_MSMTPPATH_TEST		"/usr/local/msmtprc_test"

#define OVFS_RC_MUTTPATH		"/tmp/rcmutt"
#define OVFS_RC_MUTTPATH_TEST	"/tmp/rcmutt_test"

#define OVFS_RC_MSMTPPATH		"/tmp/msmtprc"
#define OVFS_RC_MSMTPPATH_TEST		"/tmp/msmtprc_test"

#define OVFS_EMAIL_TEST_ATTACH_PATH0		"/mnt/mtd/res/email_test0.jpg"
#define OVFS_EMAIL_TEST_ATTACH_PATH1		"/mnt/mtd/res/email_test1.jpg"

#define OVFS_SMTP_BONDERY_TEXT "__MESSAGE__ID__54yg6f6h6y456345"
#define OVFS_SMTP_BONDERY_TEXT_EX "WC_MAIL_PaRt_BoUnDaRy_08192009"

#define OVFS_DHCP_BAT_PATH "/var/.dhcp_bat"

#define OVFS_SMTP_BUFFER_SIZE 102400

#define OVFS_NETWORK_CHECK_SEND_RESULT(s, data, size, flag) do{\
		int Send_Len = 0;\
		int send_times = 3;\
		do{\
			send_times--;\
			int Ret_Len = send(s, data + Send_Len, size - Send_Len, flag);\
			if(Ret_Len == -1)\
				return OVFS_ERR_NETWORK_BASE;\
			Send_Len += Ret_Len;\
		}while(Send_Len != size && send_times != 0);\
		if(Send_Len != size)\
		{\
			LOGE("send err ret_code = %d\n", Send_Len);\
			return OVFS_ERR_NETWORK_BASE;\
		}\
}while(0)

#define OVFS_NETWORK_CHECK_FUNC_RESULT(express, name)do{\
		if(OVFS_SUCCESS != express)\
		{\
			LOGE("%s fail \n", name);\
			return OVFS_ERR_NETWORK_BASE;\
		}\
}while(0)

#define OVFS_NETWORK_CHECK_SEND_RESULT_GOTO(s, data, size, flag) do{\
		int Send_Len = 0;\
		int send_times = 3;\
		do{\
			send_times--;\
			int Ret_Len = send(s, data + Send_Len, size - Send_Len, flag);\
			if(Ret_Len == -1)\
				goto leave_fun;\
			Send_Len += Ret_Len;\
		}while(Send_Len != size && send_times != 0);\
		if(Send_Len != size)\
		{\
			LOGE("send err ret_code = %d\n", Send_Len);\
			goto leave_fun;\
		}\
}while(0)

#define OVFS_NETWORK_CHECK_FUNC_RESULT_GOTO(express, name)do{\
		if(OVFS_SUCCESS != express)\
		{\
			LOGE("%s fail \n", name);\
			goto leave_fun;\
		}\
}while(0)

namespace ovfs_network
{
typedef struct
{
	U32 node_num;
	ovfs_soft::ovfs_email_receiver_cfg receiver[OVFS_MAX_EMAIL_RECIEVER_NUM];
}ovfs_email_reciver_list;

typedef struct _tagovfs_email_task_node_t
{
	U32 send_times;

	OVFS_BOOL ssl_enable;
	S32  server_port;
	S8 server_addr[OVFS_MAX_EMAIL_USR_NAME_LEN];
	
	S8 user_name[OVFS_MAX_EMAIL_USR_NAME_LEN];
	S8 password[OVFS_MAX_EMAIL_USR_PASSWD_LEN];
	
	S8 *title;
	S8 *content;

	ovfs_soft::ovfs_email_attachment_fname *attachment;
	U32 attach_num;

	ovfs_soft::ovfs_email_receiver_cfg *receiver;
	U32 receiver_num;

	OVFS_BOOL is_test;
	OVFS_BOOL is_need_rm_attachment;
	
	struct _tagovfs_email_task_node_t *prev;
	struct _tagovfs_email_task_node_t *next;

	std::string m_strMessage;// 整个MIME协议字符串
	std::stringstream stream; 
}ovfs_email_task_node;

typedef struct 
{
	U32 task_num;
	ovfs_email_task_node *head;
	ovfs_email_task_node *tail;
}ovfs_email_task_list;


typedef struct _tagovfs_ddns_info_node_T
{
	I8_DDNS_FUNCTION_T ddns_func; 
	OVFS_BOOL enable; 
	void *lib;
	ovfs_soft::ovfs_ddns_cfg param;
	time_t m_last_update_time;
	
	struct _tagovfs_ddns_info_node_T *prev;
	struct _tagovfs_ddns_info_node_T *next;
}ovfs_ddns_info_node;

typedef struct _tagovfs_ddns_info_list_T
{
	U32 node_num;
	ovfs_ddns_info_node *head;
	ovfs_ddns_info_node *tail;
}ovfs_ddns_info_list;

typedef enum
{
	NETWORK_SET_NET_CAED_CFG,
	NETWORK_SET_BOND_CFG,
	NETWORK_SET_DEFAULT_ROUTE_CFG,
	NETWORK_SET_DNS_CFG,
	NETWORK_RESET_NETCARD,
}network_task_type;


typedef struct _tagnetwork_setting_task_node_t_
{
	network_task_type task_type;
	union
	{
		S8 default_route[16];
		ovfs_soft::ovfs_bounding_eth_config bond_cfg;
		ovfs_soft::ovfs_netcard_config card_cfg;
		ovfs_soft::ovfs_local_net_dns dns_cfg;
	}net_cfg;
	
	struct _tagnetwork_setting_task_node_t_ *next;
	struct _tagnetwork_setting_task_node_t_ *prev;
	
}network_setting_task_node;

typedef struct
{
	U32 task_num;
	network_setting_task_node *head;
	network_setting_task_node *tail;
	
}network_setting_task_list;

}

#endif
