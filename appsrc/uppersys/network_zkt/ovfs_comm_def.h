#ifndef _OVFS_COMM_DEF_H_
#define _OVFS_COMM_DEF_H_

#include <string.h>	//for memset
#include"libcommon_api.h"

#define  ovfs_print_json(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		printf("[%s.%d]%s\n",__FUNCTION__,__LINE__,ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)

#define FOX_HELPER_DLL_IMPORT __attribute__ ((visibility("default")))
#define FOX_HELPER_DLL_EXPORT __attribute__ ((visibility("default")))
#define FOX_HELPER_DLL_LOCAL  __attribute__ ((visibility("hidden")))

# ifndef likely
#  define likely(x)	(__builtin_expect(!!(x), 1))
# endif
# ifndef unlikely
#  define unlikely(x)	(__builtin_expect(!!(x), 0))
# endif

#define barrier() __asm__ __volatile__("": : :"memory")

#define __maybe_unused	__attribute__((unused))


/* 数组维数大小宏 */
#define OVFS_ARRAY_DIM(arg)	(sizeof(arg) / sizeof(arg[0]))

#define OVFS_CLR_ARG(arg) 	memset(&arg, 0, sizeof(arg))

#define OVFS_ALIGN_ARG(arg, align) (((arg) + ((align)-1))/(align)) * (align)

#define OVFS_TST_BIT(arg, bit) (((arg) & (1 << (bit))) != 0)
#define OVFS_SET_BIT(arg, bit) ((arg) |= (1 << (bit)))
#define OVFS_CLR_BIT(arg, bit) ((arg) &= ~(1 << (bit)))

/*位数组操作宏(a代表数组指针，b代表数组中的第多少位，c代表a中每个元素的bits数目)*/
#define OVFS_BITMASK(b,c) (1<<((b)%(c)))
#define OVFS_BITSLOT(b,c) ((b)/(c))                                                                                         //将数据映射到所属字节内
#define OVFS_BITSET(a,b,c) ((a)[OVFS_BITSLOT(b,c)] |= OVFS_BITMASK(b,c))                  //置位
#define OVFS_BITCLEAR(a,b,c) ((a)[OVFS_BITSLOT(b,c)] &= ~OVFS_BITMASK(b,c))             //清位
#define OVFS_BITTEST(a,b,c) ((a)[OVFS_BITSLOT(b,c)] & OVFS_BITMASK(b,c))                //查看相应位数据


/*判断指针是否为空*/
#define OVFS_IS_NULL_PTR(para) (NULL == para)

#define OVFS_THROW_BUILD_ERR yuhjasdfeiapeiafje;ajfeijdk;jf

/* 结构体成员在结构体中的偏移计算宏，s:结构体名, member:成员名，例如 OVFS_MEMBER_OFSET(ovfs_device_capability, video_ch_num) 计算出来等于68 */
#define OVFS_MEMBER_OFSET(s, m)   (S32)&(((s *)0)->m)

/*OVFS_U32类型包含的位数*/
#define OVFS_U32_BIT_NUM	(32)

/* 设备支持的最大本地通道数目*/
#define OVFS_MAX_LOCAL_CH_NUM	(256)

/*复制到中最大项的数*/
#define OVFS_MAX_COPY_TO_NUM    (64)

/* 录像文件夹的名称最大长度 */
#define OVFS_MAX_REC_DIR_NAME_LEN	(48)

//抓图图片的后缀
#define OVFS_SNAP_FILE_SUFFIX ".jpeg"

//抓图图片包的后缀
#define OVFS_SNAP_FILE_SUFFIX_EX ".pic"

/* 录像(抓图)文件名(相对路径下文件名)最大长度，例如171405_0.jpg */
#define OVFS_MAX_REC_FILE_NAME_LEN	(20)

/* 最大设备用户名长度 */
#define OVFS_MAX_DEV_USR_NAME_LEN 	(32)

/* 最大设备用户密码长度 */
#define OVFS_MAX_DEV_USR_PASSWD_LEN	(32)

/* admin用户名 */
#define OVFS_DEV_ADMIN_USR_NAME		"admin"

/*default用户*/
#define OVFS_DEV_DEFAULT_USR_NAME   "default"

/* admin用户默认密码 */
#define OVFS_DEV_ADMIN_USR_DEF_PASS	"888888"

/* 最大日志条目数 */
#define OVFS_MAX_LOG_NODE_NUM	(100000)

/* 最大用邮箱户名长度 */
#define OVFS_MAX_EMAIL_USR_NAME_LEN 	(64)

/* 最大邮箱密码长度 */
#define OVFS_MAX_EMAIL_USR_PASSWD_LEN	(64)

/* ipv4 地址的最大长度 */
#define OVFS_IPV4_STRING_LEN 	sizeof("xxx.xxx.xxx.xxx")

/* ipv6 地址的最大长度 */
#define OVFS_IPV6_STRING_LEN 	sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")

/* mac 地址的最大长度 */
#define OVFS_MAC_STRING_LEN 	sizeof("xx:xx:xx:xx:xx:xx")

/*用户名最大长度*/
#define OVFS_MAX_USER_NAME_STRING_LEN       (128)

/*密码最大长度*/
#define OVFS_MAX_PASSWORD_STRING_LEN        (64)

/* DNS域名最大长度 */
#define OVFS_MAX_DNS_NAME_LEN	(32)

/* 最大文件路径长度 */
#define OVFS_MAX_FILE_PATH_LEN	(80)

/* 备份 、播放最大文件路径长度 */
#define OVFS_BACKUP_PLAY_FILE_PATH	(512)

//最大网卡数
#define OVFS_MAX_ETH_CARD_NUM (16)
#define OVFS_MAX_ETH_IFNAME_SIZE sizeof("ethxx")

#define OVFS_PORT_MAX (64)

//最大域名长度
#define OVFS_MAX_DOMAIN_NAME (64)

#define OVFS_MAX_BOUND_NUM  4 //最大的bounding网卡个数
#define OVFS_MAX_IF_DIM_NUM  16 //单个虚拟网卡最多可以挂载多少个实际网卡

/* 各种代码需要用的文件或者工具的路径 */
#define OVFS_IPC_OLD_PROTOCAL_DIR 	"/mnt/mtd/app/protocal_lib/old"
#define OVFS_IPC_NEW_PROTOCAL_DIR 	"/mnt/mtd/app/protocal_lib/new"

#define OVFS_IPC_OLD_PROTOCAL_CUSTOM_DIR 	"/mnt/mtd/custom/protocal_lib/old"	//IPC 客户定制路径
#define OVFS_IPC_NEW_PROTOCAL_CUSTOM_DIR 	"/mnt/mtd/custom/protocal_lib/new"

/* 第三方工具 */
#define OVFS_IPSAN_TOOL_PATH               "/mnt/mtd/app/tool/iscsiadm"        //IPSAN管理工具
#define OVFS_NFS_TOOL_PATH                   "/mnt/mtd/app/tool/showmount"   //NFS共享目录查看工具
#define OVFS_RAID_TOOL_PATH                 "/mnt/mtd/app/tool/mdadm"         //Raid管理工作
#define OVFS_RAID_TUNE_SCRIPT              "/mnt/mtd/app/tool/tuneraid.sh"    //Raid调优脚本
#define OVFS_MKXFS_PATH                         "/mnt/mtd/app/tool/mkfs.xfs"	//XFS文件系统
#define OVFS_MKE2FS_PATH			"/mnt/mtd/app/tool/mkfs.ext4"	//格式化工具
#define OVFS_PARTED_PATH 			"/mnt/mtd/app/tool/parted"		//GPT分区
#define OVFS_NTFS_3G_PATH 			"/mnt/mtd/app/tool/ntfs-3g"		//移动硬盘ntfs读写
#define OVFS_CDRECORD_PATH 			"/mnt/mtd/app/tool/cdrecord"	//刻录
//#define OVFS_UPNP_TOOL_PATH			"/mnt/mtd/app/tool/upnpc-static"	//UPNP工具
//#define OVFS_UPNP_TOOL_PATH			"/root/bin/upnpc-static"	//UPNP工具
#define OVFS_UPNP_TOOL_PATH			"upnpc-static"	//UPNP工具
#define OVFS_DISK_INFO_TOOL_PATH      "/mnt/mtd/app/tool/sg_vpd"          //获取硬盘信息工具(转速等)
#define OVFS_CDRECORD_DIAG_FILE		"/usr/local/tmp_cdr_info"		//解析刻录机的信息
#define OVFS_HOST_LIB_PATH			"/mnt/mtd/app/host"				//网络平台库路径
#define OVFS_HOST_LIB_CUSTOM_PATH	"/mnt/mtd/custom/host"			//网络平台客户定制路径
#define OVFS_HOST_QR_PATH			"/tmp/"			//网络平台库二维码生成存放路径

//#define OVFS_DDNS_LIB_PATH 			"/mnt/mtd/app/ddns"				//ddns 动态库路径
//#define OVFS_DDNS_LIB_CUSTOM_PATH 	"/mnt/mtd/custom/ddns"				//ddns 动态库客户定制路径
#define OVFS_DDNS_LIB_PATH 			"/root/ddns_libs"				//ddns 动态库路径
#define OVFS_DDNS_LIB_CUSTOM_PATH 	"/root/custom/ddns_libs"				//ddns 动态库客户定制路径

//#define OVFS_NTPCLIENT_PATH 		"/mnt/mtd/app/tool/ntpclient"  		//ntp校时工具路径
//#define OVFS_APP_MUTTPATH			"/mnt/mtd/app/tool/mutt"		//mutt邮件管理工具路径
//#define OVFS_APP_MSMTPPATH			"/mnt/mtd/app/tool/msmtp"		//mstmp非ssl邮件发送工具
//#define OVFS_APP_MUTTPATH			"/root/bin/mutt"		//mutt邮件管理工具路径
//#define OVFS_APP_MSMTPPATH			"/root/bin/msmtp"		//mstmp非ssl邮件发送工具
#define OVFS_APP_MUTTPATH			"mutt"		//mutt邮件管理工具路径
#define OVFS_APP_MSMTPPATH			"msmtp"		//mstmp非ssl邮件发送工具

#define OVFS_APP_TCPDUMP_PATH 		"/mnt/mtd/app/tool/tcpdump"		//网络抓包tcpdump  工具路径

#define OVFS_PLAYER_PATH			"/mnt/mtd/res/ovfs_player.rar"		//我们自己的播放器存放路径

#define OVFS_TMPLOG_FILE_PATH "/tmp/"


//Notify That:3DES Key Which we use length is 16, So OVFS_3DES_KEY Length Must > 16,  But In the process, we only interception 15 bytes, the last bytes is terminate '\0'
#define OVFS_3DES_KEY   "WuHanMeiDianEnZhi"
#define OVFS_3DES_PADDING_VALUE (0x3E)
#define OVFS_3DS_STR_CHANGE_KEY ('a')

#define  OVFS_GUI_SERVER_IP  "127.0.0.1"
#define  OVFS_GUI_SERVER_PORT (9527)


#define OVFS_STR_CAT(a, b)  (a b) //字符串连接


#define OVFS_MAX_PIC_BUFFER_SIZE  4*1024*1024             //图片的最大尺寸

#define OVFS_CNT_MASK_NUM(arg) ((arg + OVFS_U32_BIT_NUM - 1)/OVFS_U32_BIT_NUM)

//最大的查询图片数目
#define OVFS_MAX_INQUIRY_PIC_NUM   10000

//每个网卡最大ip个数
#define OVFS_MAX_IP_ADDR_NUM 64


typedef void OVFS_VOID;

typedef enum
{
    OVFS_FALSE = 0,
    OVFS_TRUE = 1,
} OVFS_BOOL;

#endif	//#ifndef _OVFS_COMM_DEF_H_

