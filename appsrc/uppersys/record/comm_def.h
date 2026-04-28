#ifndef _COMM_DEF_H_
#define _COMM_DEF_H_

#include <string.h>	//for memset

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
#define CV_ARRAY_DIM(arg)	(sizeof(arg) / sizeof(arg[0]))

#define CV_CLR_ARG(arg) 	memset(&arg, 0, sizeof(arg))

#define CV_ALIGN_ARG(arg, align) (((arg) + ((align)-1))/(align)) * (align)

#define CV_TST_BIT(arg, bit) (((arg) & (1 << (bit))) != 0)
#define CV_SET_BIT(arg, bit) ((arg) |= (1 << (bit)))
#define CV_CLR_BIT(arg, bit) ((arg) &= ~(1 << (bit)))

/*位数组操作宏(a代表数组指针，b代表数组中的第多少位，c代表a中每个元素的bits数目)*/
#define CV_BITMASK(b,c) (1<<((b)%(c)))
#define CV_BITSLOT(b,c) ((b)/(c))                                                                                         //将数据映射到所属字节内
#define CV_BITSET(a,b,c) ((a)[CV_BITSLOT(b,c)] |= CV_BITMASK(b,c))                  //置位
#define CV_BITCLEAR(a,b,c) ((a)[CV_BITSLOT(b,c)] &= ~CV_BITMASK(b,c))             //清位
#define CV_BITTEST(a,b,c) ((a)[CV_BITSLOT(b,c)] & CV_BITMASK(b,c))                //查看相应位数据


/*判断指针是否为空*/ 
#define CV_IS_NULL_PTR(para) (NULL == para)

/* 结构体成员在结构体中的偏移计算宏，s:结构体名, member:成员名，例如 CV_MEMBER_OFSET(cv_device_capability, video_ch_num) 计算出来等于68 */
#define CV_MEMBER_OFSET(s, m)   (S32)&(((s *)0)->m)

/*CV_U32类型包含的位数*/
#define CV_U32_BIT_NUM	(32)

/* 设备支持的最大本地通道数目*/
#define CV_MAX_LOCAL_CH_NUM	(2)

/*复制到中最大项的数*/
#define CV_MAX_COPY_TO_NUM    (64)

/* 录像文件夹的名称最大长度 */
#define CV_MAX_REC_DIR_NAME_LEN	(48)

//抓图图片的后缀
#define CV_SNAP_FILE_SUFFIX ".jpeg"

//抓图图片包的后缀
#define CV_SNAP_FILE_SUFFIX_EX ".pic"

/* 录像(抓图)文件名(相对路径下文件名)最大长度，例如171405_0.jpg */
#define CV_MAX_REC_FILE_NAME_LEN	(20)

/* 布防时间段的数目 */
#define CV_MAX_ARMING_TIME_QUANTUM	(8)	//最大八个时间段

/* 最大本地报警输入数目 */
#define CV_MAX_LOCAL_AI_NUM	(64)

/* 最大本地报警输出数目 */
#define CV_MAX_LOCAL_AO_NUM	(32)

/* 最大设备用户名长度 */
#define CV_MAX_DEV_USR_NAME_LEN 	(32)

/* 最大设备用户密码长度 */
#define CV_MAX_DEV_USR_PASSWD_LEN	(32)

/* 最大设备用户数目 */
#define CV_MAX_DEV_USR_NUM		(32)

/*最大在线用户数目*/
#define CV_MAX_LOGIN_USR_NUM      (128)

/* admin用户名 */
#define CV_DEV_ADMIN_USR_NAME		"admin"

/*default用户*/
#define CV_DEV_DEFAULT_USR_NAME   "default"

/* admin用户默认密码 */
#define CV_DEV_ADMIN_USR_DEF_PASS	"888888"

/* 最大日志条目数 */
#define CV_MAX_LOG_NODE_NUM	(100000)

/* 最大用邮箱户名长度 */
#define CV_MAX_EMAIL_USR_NAME_LEN 	(64)

/* 最大邮箱密码长度 */
#define CV_MAX_EMAIL_USR_PASSWD_LEN	(64)

/* ipv4 地址的最大长度 */
#define CV_IPV4_STRING_LEN 	sizeof("xxx.xxx.xxx.xxx")

/* ipv6 地址的最大长度 */
#define CV_IPV6_STRING_LEN 	sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")

/* mac 地址的最大长度 */
#define CV_MAC_STRING_LEN 	sizeof("xx:xx:xx:xx:xx:xx")

/*用户名最大长度*/
#define CV_MAX_USER_NAME_STRING_LEN       (128)

/*密码最大长度*/
#define CV_MAX_PASSWORD_STRING_LEN        (64)

/*设备名的最大长度*/
#define CV_MAX_DEV_NAME_LEN               (sizeof("md_d**") + 1)

/*设备分区名的最大长度*/
#define CV_MAX_DEV_PART_NAME_LEN          (sizeof("md_d**p**") + 1)

/*设备分区挂载路径最大长度*/
#define CV_MAX_DEV_MOUNT_DIR_LEN          (32)

/*磁盘描述名最大长度*/
#define CV_MAX_DISK_DESCRIBE_NAME_LEN     (32)

/*网络磁盘连接目标最大长度*/
#define CV_MAX_NET_DISK_TARGET_NAME_LEN         (128)

/*网络磁盘(IPSAN)一个目标能够管理的最大磁盘数目*/
#define CV_MAX_IPSAN_TARGET_ATTACH_DISK_NUM   (1)

/*RAID支持的最大硬盘数目*/
#define CV_MAX_RAID_ASSEMBLE_DISK_NUM                 (16)

/* DNS域名最大长度 */
#define CV_MAX_DNS_NAME_LEN	(32)

/*硬盘smart信息最多数目*/
#define ANTTS_MAX_DISK_SMART_VALUE_NUM  (30)

/* 最大硬盘数目 */
#define CV_MAX_HARD_DISK_NUM		(64)

/*最大U盘数目*/
#define CV_MAX_USB_DISK_NUM		(4)

/* 存储设备最大分区数目 */
#define CV_MAX_STORAGE_DEV_PART	(30)

/* 最大磁盘分组数目 */
#define CV_MAX_DISK_GROUP_NUM		(16)

/*磁盘录像最大有效保留时长(以秒为单位)*/
#define CV_MAX_REC_RETAIN_DURATION     (1000 * 60 * 60)

/*磁盘录像最小有效保留时长(以秒为单位)*/
#define CV_MIN_REC_RETAIN_DURATION      (1 * 60 * 60)

/* 每个磁盘分组最多支持的录像通道数目 */
#define CV_MAX_REC_CH_PER_DISK_GROUP		(64)

/* 自动分组的时候每个硬盘支持的录像通道数目的阈值 */
#define CV_MAX_REC_CH_PER_DISK_AUTO		    (32)

/* 最大组织架构分组数目 */
#define CV_MAX_GROUP_STRUCT_NUM		(32)

/* 组织架构每个分组最大通道数目 */
#define CV_MAX_GROUP_STRUCT_CHANNEL_NUM (64)

/* 最大组织架构组名长度 */
#define CV_MAX_GROUP_STRUCT_NAME_LEN		(64)

/* 最大通道轮询分组数目 */
#define CV_MAX_CH_POLLLING_NUM		  (32)

/* 通道轮询每个分组最大通道数目 */
#define CV_MAX_CH_POLLLING_CHANNEL_NUM   (64)

/* 最大通道轮询组名长度 */
#define CV_MAX_CH_POLLLING_NAME_LEN		(64)
	

/* 支持的PTZ协议的最大数目 */
#define CV_MAX_PTZ_PROTOCOL_NUM	(16)

/* 最大文件路径长度 */
#define CV_MAX_FILE_PATH_LEN	(80)

/* 备份 、播放最大文件路径长度 */
#define CV_BACKUP_PLAY_FILE_PATH	(512)

/*报警持续时间*/
#define CV_ALARM_DELAY_TIME (5)

//最大网卡数
#define CV_MAX_ETH_CARD_NUM (16)
#define CV_MAX_ETH_IFNAME_SIZE sizeof("ethxx")

//最大域名长度
#define CV_MAX_DOMAIN_NAME (64)

//面板名称最大长度
#define CV_KEY_BOARD_NAME  (64)

//NVR本地最大分辨率个数
#define CV_NVR_MAX_RES_ABILITY_NUM 	16

#define CV_MAX_OD_REC_NUM (4)
#define CV_MAX_MASK_REC_NUM (4)
#define CV_MAX_MD_REC_NUM (4)
#define CV_MAX_MD_SCOPE_NUM (64 * 64)

//车牌和人脸的坐标计算时的基准坐标
#define CV_REAL_RECT_WIDTH 1920
#define CV_REAL_RECT_HEIGHT 1080
#define CV_MAX_PLATE_RECT_NUM (1)


#define CV_MAX_SMART_BUFFER_NUM 32

#define CV_SMART_PLATE_MAX_LEN    (32)   //车牌号的最大的长度

#define CV_SMART_PIC_MAX_RECT_CNT 32 //一张智能图片中最多有多少张脸或者车牌

//最大支持的网路平台个数
#define CV_MAX_SUPPORT_HOST_NUM 32
//最大支持的网络平台库参数个数和单个参数长度
#define CV_MAX_HOST_PARAM_NUM 8
#define CV_MAX_PARAM_LEN 32
#define CV_MAX_BOUND_NUM  4 //最大的bounding网卡个数
#define CV_MAX_IF_DIM_NUM  16 //单个虚拟网卡最多可以挂载多少个实际网卡

/* 各种代码需要用的文件或者工具的路径 */
#define CV_IPC_OLD_PROTOCAL_DIR 	"/mnt/mtd/app/protocal_lib/old"
#define CV_IPC_NEW_PROTOCAL_DIR 	"/mnt/mtd/app/protocal_lib/new"

#define CV_IPC_OLD_PROTOCAL_CUSTOM_DIR 	"/mnt/mtd/custom/protocal_lib/old"	//IPC 客户定制路径
#define CV_IPC_NEW_PROTOCAL_CUSTOM_DIR 	"/mnt/mtd/custom/protocal_lib/new"

/* 第三方工具 */
#define CV_IPSAN_TOOL_PATH               "/mnt/mtd/app/tool/iscsiadm"        //IPSAN管理工具
#define CV_NFS_TOOL_PATH                   "/mnt/mtd/app/tool/showmount"   //NFS共享目录查看工具
#define CV_RAID_TOOL_PATH                 "/mnt/mtd/app/tool/mdadm"         //Raid管理工作
#define CV_RAID_TUNE_SCRIPT              "/mnt/mtd/app/tool/tuneraid.sh"    //Raid调优脚本
#define CV_MKXFS_PATH                         "/mnt/mtd/app/tool/mkfs.xfs"	//XFS文件系统
#define CV_MKE2FS_PATH			"/mnt/mtd/app/tool/mkfs.ext4"	//格式化工具
#define CV_PARTED_PATH 			"/mnt/mtd/app/tool/parted"		//GPT分区
#define CV_NTFS_3G_PATH 			"/mnt/mtd/app/tool/ntfs-3g"		//移动硬盘ntfs读写
#define CV_CDRECORD_PATH 			"/mnt/mtd/app/tool/cdrecord"	//刻录
#define CV_UPNP_TOOL_PATH			"/mnt/mtd/app/tool/upnpc-static"	//UPNP工具
#define CV_DISK_INFO_TOOL_PATH      "/mnt/mtd/app/tool/sg_vpd"          //获取硬盘信息工具(转速等)
#define CV_CDRECORD_DIAG_FILE		"/usr/local/tmp_cdr_info"		//解析刻录机的信息
#define CV_HOST_LIB_PATH			"/mnt/mtd/app/host"				//网络平台库路径
#define CV_HOST_LIB_CUSTOM_PATH	"/mnt/mtd/custom/host"			//网络平台客户定制路径
#define CV_HOST_QR_PATH			"/tmp/"			//网络平台库二维码生成存放路径

#define CV_DDNS_LIB_PATH 			"/mnt/mtd/app/ddns"				//ddns 动态库路径
#define CV_DDNS_LIB_CUSTOM_PATH 	"/mnt/mtd/custom/ddns"				//ddns 动态库客户定制路径

#define CV_NTPCLIENT_PATH 		"/mnt/mtd/app/tool/ntpclient"  		//ntp校时工具路径
#define CV_APP_MUTTPATH			"/mnt/mtd/app/tool/mutt"		//mutt邮件管理工具路径
#define CV_APP_MSMTPPATH			"/mnt/mtd/app/tool/msmtp"		//mstmp非ssl邮件发送工具

#define CV_APP_TCPDUMP_PATH 		"/mnt/mtd/app/tool/tcpdump"		//网络抓包tcpdump  工具路径

#define CV_PLAYER_PATH			"/mnt/mtd/res/cv_player.rar"		//我们自己的播放器存放路径

/* 资源文件 */
#define CV_FORMATE_NOT_SUPPORT_LOGO_PATH  "/mnt/mtd/res/platform/not_support.nvr"//解码图片不支持的路径
#define CV_FORMATE_NOT_SUPPORT_LOGO_BIG_PATH  "/mnt/mtd/res/platform/not_support_big.nvr"//解码图片不支持的路径
#define CV_FORMATE_SVAC_LOGO_PATH			"/mnt/mtd/res/platform/svac.nvr"//解码为SVAC的图片路径	
#define CV_FORMATE_SVAC_LOGO_BIG_PATH			"/mnt/mtd/res/platform/svac_big.nvr"//解码为SVAC的图片路径	

/*磁盘挂载目录前缀*/
#define CV_NET_NAS_MOUNT_PRE_DIR                  "/nas"
#define CV_NET_IPSAN_MOUNT_PRE_DIR              "/ipsan"
#define CV_LOCAL_DISK_MOUNT_PRE_DIR           "/dat"
#define CV_LOCAL_RAID_MOUNT_PRE_DIR           "/raid"

/* avss录像文件的描述文件名，录像、解析模块都需要使用这个，所以统一约定 */
#define CV_AVSS_DESCRIB_FILE_NAME		"cv.avss"
#define CV_AVSS_I_A_CONTENT_FILE_NAME	"cv.avss.ia"
#define CV_AVSS_M_P_CONTENT_FILE_NAME	"cv.avss.mp"
#define CV_AVSS_S_P_CONTENT_FILE_NAME	"cv.avss.sp"
#define CV_AVSS_SMART_FILE_NAME       "cv.avss.smart"

/* 升级包解压缩路径 */
#define CV_UPGRADE_EXTRACT_DIR	"/tmp/cv_upgrade"
/* 网络平台临时升级包存放路径*/
#define CV_UPGRADE_PATH	"/tmp/hisixx.update"
//网络平台库配置文件路径



//nginx 上传目录的软连接
#define CV_UPDATE_PATH_LN "/tmp/update_ln"


//nginx 上传目录的的默认值
#define CV_UPDATE_PATH_DEFAULT "/tmp/update_default"


//零时升级包名字
#define CV_UPGRADE_FILE_TMP_NAME "hisixx.update"

//数据库日志文件的存放目录
#define CV_LOGDB_PATH  "/reserve/"
#define CV_LOGDB_NAME  "cv_nvr2.0_log"
#define CV_LOGDB_TABLENAME "cv_loginfo"
#define CV_MAX_SQL_LENGTH (1024)
#define CV_TMPLOG_FILE_PATH "/tmp/"
#define CV_LOGDB_DEL_NUM_PER_TIME  (1000)   //日志每次删除的数目
#define CV_SMART_COUNTER_SAVE_PATH "/reserve/smart_counter" //临时记录计数检测结果的文件

//软看门狗重启信息记录文件
#define CV_SOFT_WDT_INFO_FILE "/reserve/wdt_info.txt"		//软狗重启记录
#define CV_WDT_TAG_FILE_NAME	"/reserve/wdt_tag.xml"		//软狗重启标记文件
#define CV_WDT_TAG_FILE_KEY0	"WDT_TAG"
#define CV_WDT_TAG_FILE_INFO	"info"
#define CV_WDT_TAG_FILE_PURPOSE	"purpose"
#define CV_WDT_TAG_FILE_THRESHOLD	"threshold"
#define CV_WDT_TAG_FILE_SRC	"src"
#define CV_WDT_TAG_FILE_TIME	"time"

#define CV_LOG_TIME_LENGTH  (sizeof("2014-12-12 12:00:00"))

//Notify That:3DES Key Which we use length is 16, So CV_3DES_KEY Length Must > 16,  But In the process, we only interception 15 bytes, the last bytes is terminate '\0'
#define CV_3DES_KEY   "WuHanMeiDianEnZhi" 
#define CV_3DES_PADDING_VALUE (0x3E)
#define CV_3DS_STR_CHANGE_KEY ('a')

#define CV_ARGB_1555_WHITE    (0xFFFF)
#define CV_ARGB_1555_BLACK    (0x8000)

#define CV_PLAYBACK_PIC_PATH "/reserve/"           //回放图片的保存路径

#define  CV_GUI_SERVER_IP  "127.0.0.1"
#define  CV_GUI_SERVER_PORT (9527)

//本地
#define CV_LIVE_ROI_TIMES   (16)
#define CV_ROI_RECT_LENGHT (1000)          //计算区域模板

//海思手册中描述宽度和高度对齐为2个像素点，但是如果VO为高清且为隔行扫描时序，高度
//必须为4像素点对齐，为了保险起见高度还是4pixels对齐
#define CV_WIND_WIDTH_ALIGN       (2)
#define CV_WIND_HEIGHT_ALIGN      (4)

#define CV_STR_CAT(a, b)  (a b) //字符串连接
#define CV_RS485_PTZ_PROTOCOL_PATH 						"/mnt/mtd/app/serial/protocol/" //485云台控制的协议路径
#define CV_RS485_PTZ_PROTOCOL_CUSTOM_PATH 						"/mnt/mtd/custom/serial/protocol/" //485云台控制的协议客户定制路径
#define CV_LIVE_DIS_LOG_PATH                               "/mnt/mtd/res/platform/"    //本地显示图片的路径
#define CV_LIVE_LOGO_COMPANY                               "company.nvr"         		     //无通道窗口的图片
#define CV_LIVE_LOGO_COMPANY_MID                           "company_mid.nvr"               //无通道窗口的图片
#define CV_LIVE_LOGO_COMPANY_BIG                           "company_big.nvr"               //无通道窗口的图片

#define CV_LIVE_LOGO_VIDEO_LOST                               "novideo.nvr"          //视频丢失显示的图片
#define CV_LIVE_LOGO_VIDEO_LOST_MID                       "novideo_mid.nvr"          //视频丢失显示的图片
#define CV_LIVE_LOGO_VIDEO_LOST_BIG                       "novideo_big.nvr"          //视频丢失显示的图片

#define CV_LIVE_LOGO_LOCK_OPS_PATH                       "lock_ops.nvr"			//操作无权限显示图片
#define CV_LIVE_LOGO_LOCK_OPS_PATH_MID               "lock_ops_mid.nvr"			//操作无权限显示图片
#define CV_LIVE_LOGO_LOCK_OPS_PATH_BIG               "lock_ops_big.nvr"			//操作无权限显示图片

#define CV_LIVE_LOGO_PB_NO_FILE_PATH                    "no_rec_file.nvr"    //回放没有录像文件显示图片
#define CV_LIVE_LOGO_PB_NO_FILE_PATH_MID            "no_rec_file_mid.nvr"    //回放没有录像文件显示图片
#define CV_LIVE_LOGO_PB_NO_FILE_PATH_BIG            "no_rec_file_big.nvr"    //回放没有录像文件显示图片

#define CV_PLATFORM_LOGO_NOT_SUPPORT_FILE_PATH                    "not_support.nvr"    //无法解码的显示log
#define CV_PLATFORM_LOGO_NOT_SUPPORT_FILE_PATH_MID            "not_support_mid.nvr"    //无法解码的显示log
#define CV_PLATFORM_LOGO_NOT_SUPPORT_FILE_PATH_BIG            "not_support_big.nvr"    //无法解码的显示log

#define CV_PLATFORM_LOGO_SVAC_FILE_PATH                    "svac.nvr"    //无法解码SVAC的显示log
#define CV_PLATFORM_LOGO_SVAC_FILE_PATH_MID            "svac_mid.nvr"    //无法解码SVAC的显示log
#define CV_PLATFORM_LOGO_SVAC_FILE_PATH_BIG            "svac_big.nvr"    //无法解码SVAC的显示log

/******************************************GUI图片路径划分*******************************************************/
//各个界面使用的文件名由各自界面在头文件中为宏定义表示，而最终的绝对路径名则使用
//CV_GUI_PREFIX_IMAGE_PATH  + 模块前缀(如预览界面则使用CV_GUI_PREVIEW_PREFIX) + 自定义文件名
#define CV_GUI_PREFIX_IMAGE_PATH                                      ("/mnt/mtd/res/qt/images/")               //资源文件根目录

#define CV_GUI_GLOBAL_PREFIX                                               ("global/")                                             //全局(整个GUI可见)
#define CV_GUI_MISC_PREFIX                                                    ("misc/")                                               //杂类(分散在各个页面)
#define CV_GUI_START_UP_PREFIX                                           ("startUp/")                                           //开机引导过程
#define CV_GUI_MENU_BAR_PREFIX                                          ("menuBar/")                                         //菜单栏
#define CV_GUI_PREVIEW_PREFIX                                             ("preview/")                                           //预览
#define CV_GUI_PLAYBACK_PREFIX                                           ("playback/")                                         //回放
#define CV_GUI_SOFT_KEYBOARD_PREFIX                               ("softKeyBoard/")                                  //软键盘



#define CV_PROUCT_DISK_LOCATION_MAP_PATH                   "/mnt/mtd/res/product/"    //显示磁盘位置图片的路径
/*************************************************End******************************************************************/




#define CV_LOGO_PIC_WIDTH 480                           //窗口的图片的宽高
#define CV_LOGO_PIC_HEIGHT 270
#define CV_MAX_PIC_BUFFER_SIZE  4*1024*1024             //图片的最大尺寸




// 检测线/检测区域的最大顶点数
#define	CV_IVS_MAX_VERTEX_CNT		(20)

// 最大的检测线条数
#define CV_IVS_MAX_WIRE_CNT		(4)

// 最大的检测区域个数
#define CV_IVS_MAX_REGION_CNT		(4)

// 单条规则同时输出的最大目标位置数
#define CV_IVS_MAX_OBJ_CNT			(10)

// 最大的遗留/丢失/逗留/徘徊检测时间, 单位:秒
#define CV_IVS_MAX_DELAY_TIME		(120)

// 轨迹个数
#define CV_IVS_MAX_TRAJECTORY_CNT	(50)

// 网格形式的移动侦测区域的最大网格数
#define CV_IVS_MAX_MD_GRID_CNT	    (128)   // (64*64 + 31) / 32

//智能检测抽帧间隔
#define CV_IVS_SEND_FRAME_INTERVAL (1)

//GUI关键色格式为(ARGB, A默认为0不透明.)
#define CV_FB_COLOR_KEY           (0x34 << 8 | 0x56)


#ifdef _USE_UTILITY_LOCK_
#define CV_RW_WR_LOCK(lock)  lock->wr_lock(__FILE__, __FUNCTION__)
#define CV_RW_RD_LOCK(lock)  lock->rd_lock(__FILE__, __FUNCTION__)
#define CV_RW_UNLOCK(lock)  lock->unlock(__FILE__, __FUNCTION__)
#define CV_LOCK(lock)  lock->lock(__FILE__, __FUNCTION__)
#define CV_UNLOCK(lock)  lock->unlock(__FILE__, __FUNCTION__)
#else 
#define CV_RW_WR_LOCK(lock)  pthread_rwlock_wrlock(&lock)
#define CV_RW_RD_LOCK(lock)  pthread_rwlock_rdlock(&lock)
#define CV_RW_UNLOCK(lock) pthread_rwlock_unlock(&lock)
#define CV_LOCK(lock)  pthread_mutex_lock(&lock)
#define CV_UNLOCK(lock)   pthread_mutex_unlock(&lock)
#endif

#define CV_CNT_MASK_NUM(arg) ((arg + CV_U32_BIT_NUM - 1)/CV_U32_BIT_NUM)

#undef CV_FRAME_STARTCODE
#define CV_FRAME_STARTCODE (0xAB010000)


#define CV_MAX_ABILITY_NUM 32

#define CV_MAX_ABILITY_RES_NUM 32
#define CV_MAX_ABILITY_BPS_NUM 32
#define CV_MAX_ABILITY_FPS_NUM 30
#define CV_MAX_ABILITY_QUALITY_NUM 6
#define CV_MAX_ABILITY_PTZ_DECODER_NUM 32
#define CV_MAX_ABILITY_PTZ_BAUNDRATE_NUM 32

#define CV_MAX_TRAIL_NUM   16
#define CV_MAX_CURSIE_NUM  32
#define CV_MAX_PRESET_NUM  255
#define CV_MAX_CRUISE_PRESET_NUM						32
#define CV_MAX_PTZ_SPEED 16
#define CV_MAX_SCAN_NUM   8        //两点扫描的个数

#define CV_MAX_SUPPORT_HDD_PANNEL_NUM 64

//平台
#define CV_HOSTMGR_NAME_MAX_LEN	(32)
#define CV_HOSTMGR_FILE_NAME_MAX_LEN	(128)
#define CV_HOSTMGR_ALIAS_MAX_LEN	(64)
#define CV_HOSTMGR_QR_MAX				(3)//三个页面支持二维码的最大数

//IE界面 0通道传入的通道号
#define CV_ZERO_CHANNEL_CH_NUM 1000


//最大的查询图片数目
#define CV_MAX_INQUIRY_PIC_NUM   10000

#define MAX_RECORDTYPE 11

#define FRAME_STARTCODE 					(0xAB010000)
#define FILE_STARTCODE 				   	    (0xAA010000)
#define MOTION_STARTCODE 				   	(0xAC010000)
#define APP_STARTCODE 				   		(0xAD010000)

#endif	//#ifndef _COMM_DEF_H_

