#ifndef __ANTS_HOSTMGR_TYPE_H__
#define __ANTS_HOSTMGR_TYPE_H__

#ifdef WIN32
typedef __int64 S64;
typedef unsigned __int64 U64;

#else
typedef long long S64;
typedef unsigned long long U64;

#endif
typedef int S32;
typedef char S8;
typedef short S16;

typedef unsigned int U32;
typedef unsigned char U8;
typedef unsigned short U16;

typedef float F32;
typedef double F64;

typedef void VOID;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _WIN32
#ifndef		DWORD_T
#define		DWORD_T
typedef		unsigned int	DWORD;
#endif
#ifndef		WORD_T
#define		WORD_T
typedef		unsigned short	WORD;
#endif
#ifndef		USHORT_T
#define		USHORT_T
typedef		unsigned short	USHORT;
#endif
#ifndef		LONG_T
#define		LONG_T
typedef		int				LONG;
#endif
#ifndef		BYTE_T
#define		BYTE_T
typedef		unsigned char	BYTE;
#endif
#ifndef		BOOL_T
#define		BOOL_T
typedef		int				BOOL;
#endif
#ifndef		UINT_T
#define		UINT_T
typedef		unsigned int	UINT;
#endif
#ifndef		LPVOID_T
#define		LPVOID_T
typedef		void* 			LPVOID;
#endif
#ifndef		HANDLE_T
#define		HANDLE_T
typedef		void* 			HANDLE;
#endif
#ifndef		LPDWORD_T
#define		LPDWORD_T
typedef		unsigned int*	LPDWORD;
#endif

#ifndef    TRUE
#define    TRUE	1
#endif
#ifndef    FALSE
#define	   FALSE 0
#endif

#define __stdcall
#define CALLBACK
#endif


//宏定义
#define MAX_NAMELEN			    16		//DVR本地登陆名
#define MAX_RIGHT			    32		//设备支持的权限（1-12表示本地权限，13-32表示远程权限）
#define NAME_LEN			    32      //用户名长度
#define PASSWD_LEN			    16      //密码长度
#define SERIALNO_LEN		    48      //序列号长度
#define MACADDR_LEN			    6       //mac地址长度
#define MAX_ETHERNET		    2       //设备可配以太网络
#define MAX_NETWORK_CARD        4       //设备可配最大网卡数目
#define PATHNAME_LEN		    128     //路径长度

#define MAX_TIMESEGMENT		    8       //设备最大时间段数

#define MAX_SHELTERNUM			4       //设备最大遮挡区域数
#define MAX_DAYS				7       //每周天数
#define PHONENUMBER_LEN			32      //pppoe拨号号码最大长度

#define MAX_DISKNUM				8		//设备最大硬盘数

#define MAX_DISKNUM_EX			24		//设备最大硬盘数


#define MAX_WINDOW				32      //设备本地显示最大播放窗口数
#define MAX_VGA					4       //设备最大可接VGA数

#define MAX_USERNUM				8       //设备最大用户数
#define MAX_USERNUM_V2		16			//!针对天地合众设备扩展用户数 Added by ItmanLee at 2012-10-11
#define MAX_EXCEPTIONNUM		32      //设备最大异常处理数
#define MAX_LINK				6       //设备单通道最大视频流连接数

#define MAX_DECPOOLNUM			4       //单路解码器每个解码通道最大可循环解码数
#define MAX_DECNUM				4       //单路解码器的最大解码通道数（实际只有一个，其他三个保留）
#define MAX_TRANSPARENTNUM		2       //单路解码器可配置最大透明通道数
#define MAX_CYCLE_CHAN			64      //单路解码器最大轮循通道数
#define MAX_DIRNAME_LENGTH		80      //最大目录长度
#define MAX_WINDOWS				16      //最大窗口数

#define MAX_STRINGNUM			1		//设备最大OSD字符块数
#define MAX_AUXOUT				16      //设备最大辅助输出数
#define MAX_HD_GROUP			8      //设备最大硬盘组数

//!#define IW_ESSID_MAX_SIZE	    32      //WIFI的SSID号长度
//!#define IW_ENCODING_TOKEN_MAX	32      //WIFI密锁最大字节数
#define MAX_SERIAL_NUM			64	    //最多支持的透明通道路数
#define MAX_DDNS_NUMS	        10      //设备最大可配ddns数
#define MAX_DOMAIN_NAME		    64		//最大域名长度
#define MAX_EMAIL_ADDR_LEN	    48      //最大email地址长度
#define MAX_EMAIL_PWD_LEN		32      //最大email密码长度

#define MAX_NFS_DISK			8
#define MAX_NET_DISK 			16		//!最大网络硬盘个数

#define MAXPROGRESS		        100     //回放时的最大百分率
#define MAX_SERIALNUM	        2       //设备支持的串口数 1-232， 2-485
#define MAX_VIDEOOUT	        4       //设备的视频输出数

#define MAX_PRESET				256		//设备支持的云台预置点数
#define MAX_TRACK				256		//设备支持的云台轨迹数
#define MAX_CRUISE				256		//设备支持的云台巡航数

#define CRUISE_MAX_PRESET_NUMS	32 	    //一条巡航最多的巡航点

#define MAX_SERIAL_PORT			8       //设备支持232串口数
#define MAX_PREVIEW_MODE		8       //设备支持最大预览模式数目 1画面,4画面,9画面,16画面....
#define LOG_INFO_LEN			11840   //日志附加信息
#define DESC_LEN				32      //云台描述字符串长度
#define DESC_LEN_64				64
#define PTZ_PROTOCOL_NUM		200     //最大支持的云台协议数

#define MAX_AUDIO				2       //语音对讲通道数
#define MAX_CHANNUM				16      //设备最大通道数
#define MAX_CHANNUM_EX			64      //设备最大通道数
#define MAX_ALARMIN				16      //设备最大报警输入数
#define MAX_ALARMIN_EX			64      //设备最大报警输入数
#define MAX_ALARMOUT			4       //设备最大报警输出数

#define MAX_ALARMOUT_EX			64       //设备最大报警输出数


#define MAX_IP_ALARMIN			128      //设备最大网络报警输入数
#define MAX_IP_ALARMOUT		64       //设备最大网络报警输出数


#define MAX_RECORD_FILE_NUM		20      // 每次删除或者刻录的最大文件数

#define MAX_FORTIFY_NUM			10   //最大布防个数
#define MAX_INTERVAL_NUM		4    //最大时间间隔个数

#define	MAX_NETDECNUM			32
#define	MAX_NETDECNUM_EX		64

#define ANTS_DVR_IPC_PROTOCOL_DESC_LEN  16

#define ANTS_DVR_IPC_PROTOCOL_NUM  128

#define ANTSMID_MULTILINEOSD_NUM      8
#define ANTSMID_MULTILINEOSD_STRINGLENTH      64


#define MAX_NODE_NUM         256  //节点个数
#define MAX_ABILITYTYPE_NUM_V2  32   //最大能力项


/*****JSON方式****/
#define WEB_GET_LOGINCFG    1 //获取登录配置
/*********/

/*************************参数配置命令 begin*******************************/
//ants_HostMgr_Dev_GetConfig和ants_HostMgr_Dev_SetConfig使用


#define ANTS_DVR_GET_CONFIGRECOVERY 90 // 恢复默认值 -> ANTS_DVR_CONFIG_RECOVERY_T
#define ANTS_DVR_SET_CONFIGRECOVERY 91 // 恢复默认值

#define ANTS_DVR_GET_DEVICECFG		100		//获取设备参数 +
#define ANTS_DVR_SET_DEVICECFG		101		//设置设备参数
#define ANTS_DVR_GET_NETCFG			102		//获取网络参数 +
#define ANTS_DVR_SET_NETCFG			103		//设置网络参数
#define ANTS_DVR_GET_PICCFG			104		//获取图象参数 +
#define ANTS_DVR_SET_PICCFG			105		//设置图象参数
#define ANTS_DVR_GET_COMPRESSCFG	106		//获取压缩参数 +
#define ANTS_DVR_SET_COMPRESSCFG	107		//设置压缩参数
#define ANTS_DVR_GET_RECORDCFG		108		//获取录像时间参数 +
#define ANTS_DVR_SET_RECORDCFG		109		//设置录像时间参数
#define ANTS_DVR_GET_DECODERCFG		110		//获取解码器参数 +
#define ANTS_DVR_SET_DECODERCFG		111		//设置解码器参数
#define ANTS_DVR_GET_RS232CFG 		112		//获取232串口参数 +
#define ANTS_DVR_SET_RS232CFG		113		//设置232串口参数
#define ANTS_DVR_GET_ALARMINCFG 	114		//获取报警输入参数 +
#define ANTS_DVR_SET_ALARMINCFG		115		//设置报警输入参数
#define ANTS_DVR_GET_ALARMOUTCFG 	116		//获取报警输出参数 +
#define ANTS_DVR_SET_ALARMOUTCFG	117		//设置报警输出参数
#define ANTS_DVR_GET_TIMECFG 		118		//获取DVR时间 +
#define ANTS_DVR_SET_TIMECFG		119		//设置DVR时间
#define ANTS_DVR_GET_USERCFG 		124		//获取用户参数 +
#define ANTS_DVR_SET_USERCFG		125		//设置用户参数
#define ANTS_DVR_GET_EXCEPTIONCFG 	126		//获取异常参数 +
#define ANTS_DVR_SET_EXCEPTIONCFG	127		//设置异常参数
#define ANTS_DVR_GET_ZONEANDDST		128		//获取时区和夏时制参数 +
#define ANTS_DVR_SET_ZONEANDDST		129		//设置时区和夏时制参数
#define ANTS_DVR_GET_SHOWSTRING		130		//获取叠加字符参数 +
#define ANTS_DVR_SET_SHOWSTRING		131		//设置叠加字符参数
#define ANTS_DVR_GET_EVENTCOMPCFG	132		//获取事件触发录像参数 +
#define ANTS_DVR_SET_EVENTCOMPCFG	133		//设置事件触发录像参数
#define ANTS_DVR_GET_AUTOREBOOT		134		//获取自动维护参数 +
#define ANTS_DVR_SET_AUTOREBOOT		135		//设置自动维护参数

#define ANTS_DVR_GET_VIDEOEFFECT    136		//获取图象亮度参数 ->ANTS_DVR_COLOR
#define ANTS_DVR_SET_VIDEOEFFECT	137		//设置图象亮度参数->ANTS_DVR_COLOR

#define ANTS_DVR_GET_MOTIONCFG      138		//获取移动侦测参数 +
#define ANTS_DVR_SET_MOTIONCFG  	139		//设置移动侦测参数
#define ANTS_DVR_GET_SHELTERCFG     140		//获取视频遮挡参数 +
#define ANTS_DVR_SET_SHELTERCFG 	141		//设置视频遮挡参数
#define ANTS_DVR_GET_HIDEALARMCFG   142		//获取遮挡报警参数 +
#define ANTS_DVR_SET_HIDEALARMCFG 	143		//设置遮挡报警参数
#define ANTS_DVR_GET_VIDEOLOSTCFG   144		//获取视频丢失参数 +
#define ANTS_DVR_SET_VIDEOLOSTCFG 	145		//设置视频丢失参
#define ANTS_DVR_GET_OSDCFG         146		//获取图象OSD参数 +
#define ANTS_DVR_SET_OSDCFG 	    147		//设置图象OSD参

#define ANTS_DVR_GET_VIDEOFORMAT        148		//获取制式参数 +
#define ANTS_DVR_SET_VIDEOFORMAT 	    149		//设置制式参
#define ANTS_DVR_GET_MOTIONEXCFG      150		//获取移动侦测扩展参数 +
#define ANTS_DVR_SET_MOTIONEXCFG  	151		//设置移动侦测扩展参数


#define ANTS_DVR_GET_VIDEOEFFECT_EX    152		//获取图象亮度参数 -> ANTS_DVR_COLOR_EX
#define ANTS_DVR_SET_VIDEOEFFECT_EX	   153		//设置图象亮度参数-> ANTS_DVR_COLOR_EX

#define ANTS_DVR_GET_TIMEUTC           154 // UTC时间
#define ANTS_DVR_SET_TIMEUTC           155 // UTC时间

#define ANTS_DVR_GET_SLAVE_NETCFG			156		//获取从片网络参数 +  -> ANTS_DVR_IPADDR[n]


#define ANTS_DVR_GET_DEFROUTENAME       182 // 获取默认路由的网卡设备.结构体ANTS_DVR_DEFROUTENAME_S




#define ANTS_DVR_GET_RECORDTYPES			158	//获取/设置某通道的录像类型

// 0-关掉录像，1-计划录像，2-手动录像
// 缓冲SIZE 决定操作的通道数。每个通道为BYTE型。
// lChannel: 当需要操作多通道时，为起始通道

#define ANTS_DVR_SET_RECORDTYPES			159	



#define ANTS_DVR_GET_ZEROCODEC			160	

#define ANTS_DVR_SET_ZEROCODEC			161	

#define ANTS_DVR_GET_RTSPCFG			162	

#define ANTS_DVR_SET_RTSPCFG			163


#define ANTS_DVR_GET_NETAPPCFG		222		//获取网络应用参数 NTP/DDNS/EMAIL +
#define ANTS_DVR_SET_NETAPPCFG		223		//设置网络应用参数 NTP/DDNS/EMAIL
#define ANTS_DVR_GET_NTPCFG			224		//获取网络应用参数 NTP +
#define ANTS_DVR_SET_NTPCFG			225		//设置网络应用参数 NTP
#define ANTS_DVR_GET_DDNSCFG		226		//获取网络应用参数 DDNS +
#define ANTS_DVR_SET_DDNSCFG		227		//设置网络应用参数 DDNS

#define ANTS_DVR_GET_EMAILCFG		228		//获取网络应用参数 EMAIL +
#define ANTS_DVR_SET_EMAILCFG		229		//设置网络应用参数 EMAIL

#define ANTS_DVR_GET_ENABLE_SPOTMODE      230 // DWORD , 0-不开启，1-开启
#define ANTS_DVR_SET_ENABLE_SPOTMODE      231

#define ANTS_DVR_NET_GET_MANAGERHOST_CFG		232 // ants 管理主机
#define ANTS_DVR_NET_SET_MANAGERHOST_CFG		233

#define ANTS_DVR_NET_GET_WIFI_CFG				234//!设置IP监控设备无线参数
#define ANTS_DVR_NET_SET_WIFI_CFG				235//!获取IP监控设备无线参数

#define ANTS_DVR_NET_GET_WIFI_WORKMODE			236//!设置IP监控设备网口工作模式参数
#define ANTS_DVR_NET_SET_WIFI_WORKMODE 			237//!获取IP监控设备网口工作模式参数

#define ANTS_DVR_NET_GET_AP_INFOLIST			238//!获取无线网络资源参数


#define ANTS_DVR_NET_SET_3G_CFG					240//!设置3G配置参数
#define ANTS_DVR_NET_GET_3G_CFG 					241//!获取3G配置参数

#define ANTS_DVR_GET_ROUTER_CFG                 242 // -> ANTS_DVR_ROUTER_CFG
#define ANTS_DVR_SET_ROUTER_CFG                 243



#define ANTS_DVR_GET_NETCFG_OTHER      244
#define ANTS_DVR_SET_NETCFG_OTHER      245

#define ANTS_DVR_GET_HDMI_AUDIO      246
#define ANTS_DVR_SET_HDMI_AUDIO      247

#define ANTS_DVR_GET_HDISK_SMART_ATTR 248

#define ANTS_DVR_GET_UPDATE_PATH 249  // -> ANTS_DVR_UPDATE_PATH_INFO


#define ANTS_DVR_GET_HDISK_FORCERECORD 250 // 硬盘出错了，仍然可以强制录像
#define ANTS_DVR_SET_HDISK_FORCERECORD 251

#define ANTS_DVR_GET_4K_OUTPUTMODE 252  // ANTS_DVR_4K_OUTPUTMODE
#define ANTS_DVR_SET_4K_OUTPUTMODE 253





#define	ANTS_DVR_GET_HDCFG			1054	//获取硬盘管理配置参数 +
#define	ANTS_DVR_SET_HDCFG			1055	//设置硬盘管理配置参数
#define	ANTS_DVR_GET_HDGROUP_CFG	1056	//获取盘组管理配置参数 +
#define	ANTS_DVR_SET_HDGROUP_CFG	1057	//设置盘组管理配置参数

#define	ANTS_DVR_GET_COMPRESSCFG_AUD	1058	//获取设备语音对讲编码参数 +
#define	ANTS_DVR_SET_COMPRESSCFG_AUD	1059	//设置设备语音对讲编码参数

#define ANTS_DVR_GET_SNMPCFG 		1060
#define ANTS_DVR_SET_SNMPCFG 		1061

#define ANTS_DVR_GET_NETCFG_MULTI	1062
#define ANTS_DVR_SET_NETCFG_MULTI	1063

#define ANTS_DVR_GET_NFSCFG			1064
#define ANTS_DVR_SET_NFSCFG			1065

#define ANTS_DVR_GET_NET_DISKCFG	1066
#define ANTS_DVR_SET_NET_DISKCFG	1067

#define ANTS_DVR_GET_NETDEVCFG		2012
#define ANTS_DVR_SET_NETDEVCFG		2013

#define	ANTS_DVR_SET_NETDEVCONNETCTCFG 2014
#define	ANTS_DVR_GET_NETDEVCONNETCTCFG 2015


#define ANTS_DVR_GET_DEVICE_CHANINFO		2018  // ->AntsHostMgrLibSearchDeviceInfo_T,通道为起始通道，缓冲大小决定获取的通道个数
#define ANTS_DVR_SET_DEVICE_CHANINFO		2019






#define ANTS_DVR_GET_NVRWORKMODE	2020
#define ANTS_DVR_SET_NVRWORKMODE	2021

#define ANTS_DVR_GET_DEVCHANNAME_CFG 2022
#define ANTS_DVR_SET_DEVCHANNAME_CFG 2023

#define ANTS_DVR_GET_DEVCHANNAME_CFG_V2 2024
#define ANTS_DVR_SET_DEVCHANNAME_CFG_V2 2025


#define	ANTS_DVR_GET_HDCFG_V2			2026	//获取硬盘管理配置参数 +
#define	ANTS_DVR_SET_HDCFG_V2			2027	//设置硬盘管理配置参数


#define ANTS_DVR_GET_SENSOR_CFG       3000
#define ANTS_DVR_SET_SENSOR_CFG       3001


#define ANTS_DVR_GET_MANAGERHOSTS_CFG       3002
#define ANTS_DVR_SET_MANAGERHOSTS_CFG       3003

#define ANTS_DVR_GET_IPALARMINCFG       3004
#define ANTS_DVR_SET_IPALARMINCFG       3005


#define ANTS_DVR_GET_IPALARMOUTCFG      3006
#define ANTS_DVR_SET_IPALARMOUTCFG      3007




#define ANTS_DVR_GET_FACTORY_INFO      3010
#define ANTS_DVR_SET_FACTORY_INFO      3011

#define ANTS_DVR_GET_REMOTE_ADJUSTTIME  3022     //  NVR ->远程设备对时配置
#define ANTS_DVR_SET_REMOTE_ADJUSTTIME  3023


#define ANTS_DVR_GET_TV_RECT  3026     //  配置TV RECT；<-> DISPRECT
#define ANTS_DVR_SET_TV_RECT  3027

#define ANTS_DVR_GET_FTPUPLOAD      3028
#define ANTS_DVR_SET_FTPUPLOAD      3029
#define ANTS_DVR_GET_DEFUSEREDIT    3034

// 配置冻结
#define ANTS_DVR_GET_FREEZECFG_STATUS       3036 // 获取配置冻结的状态.参考结构体ANTS_DVR_FREEZECFG_STATUS_S.
#define ANTS_DVR_SET_FREEZECFG_STATUS       3037 // 修改配置冻结的状态(需要带上密码).参考结构体ANTS_DVR_FREEZECFG_REQUEST_S.
#define ANTS_DVR_GET_FREEZECFG_PARAM       3038 // 参考结构体ANTS_DVR_FREEZECFG_PARAM_S
#define ANTS_DVR_SET_FREEZECFG_PARAM       3039 // 修改配置冻结的参数(如修改密码).参考结构体ANTS_DVR_FREEZECFG_PARAM_S

// 测试模式下读取和设置的操作接口
#define ANTS_DVR_GET_TESTMODE_PARAM       3040 // ANTS_DVR_TESTMODE_PARAM_S
#define ANTS_DVR_SET_TESTMODE_PARAM       3041 // ANTS_DVR_TESTMODE_PARAM_S

typedef struct
{
    char type[32]; // 具体操作类型
    unsigned int size; // 参数buffer的长度
    char *buffer; // 操作相关的参数指针
} ANTS_DVR_TESTMODE_PARAM_S;

// 平台配置命令
// 平台配置命令范围10000-20000

//  LPVOID lpParam:
// LONG pParm[N];
// pParm[0]: 平台在8个设置中的序号;
// pParm[1]: 平台的ID号

#define ANTS_DVR_CONFIG_MANAGERHOSTS_MIN         10000 



// 设备ID，可用于访问设备的
// 如P2P用于生成二维码的字符串
#define ANTS_DVR_CONFIG_MANAGERHOSTS_UUID         10001 
#define ANTS_DVR_CONFIG_MANAGERHOSTS_URL_APPLE    10002 // 苹果APP下载地址
#define ANTS_DVR_CONFIG_MANAGERHOSTS_URL_ANDROID  10003 // 安卓APP下载地址


// 上海地标,指定通道获取
#define ANTS_DVR_CONFIG_AGENT_CAPTUREPICTURE_CH      10100 //上海地标日常图像设置ANTS_DVR_SH_AGENT_CAPTUREPICTURE,指定通道

// 28181 ,  配置接口中通道参数扩展为数组指针

#define ANTS_DVR_CONFIG_MANAGERHOSTS_GB28181                 10500 // ANTS_DVR_GB28181CFG
#define ANTS_DVR_CONFIG_MANAGERHOSTS_GB28181_CHANINFO        10501 // ANTS_DVR_GB28181CFG_CHANINFO
#define ANTS_DVR_CONFIG_MANAGERHOSTS_GB28181_ALARMINFO       10502 // ANTS_DVR_GB28181CFG_CHANINFO


#define ANTS_DVR_CONFIG_MANAGERHOSTS_MAX         19999


#define ANTS_DVR_CONFIG_PICCFG			20100		//获取图象参数 ANTS_DVR_PICCFG_V2

// 新移动侦测配置
#define ANTS_DVR_CONFIG_MOTIONCFG       20101		//获取移动侦测参数 ANTS_DVR_MOTION_V2
#define ANTS_DVR_CONFIG_HIDEALARMCFG    20102		//获取遮挡报警参数 ANTS_DVR_HIDEALARM_V2
#define ANTS_DVR_CONFIG_VIDEOLOSTCFG 	20103		//设置视频丢失参数ANTS_DVR_VILOST_V2

// 多行OSD

#define ANTS_DVR_CONFIG_MULTILINEOSDCFG 20104 // 获取/设置多行OSD   <-> ANTS_DVR_MULTILINEOSDCFG

//===============================
#define ANTS_DVR_CONFIG_MOTION_PTZLINKCFG 20105 // 获取/设置移动PTZ联动配置   <-> ANTS_DVR_PTZLINKCFG
#define ANTS_DVR_CONFIG_HIDEALARM_PTZLINKCFG 20106 // 获取/设置遮挡报警PTZ联动配置   <-> ANTS_DVR_PTZLINKCFG

//=========================

// 多播配置
#define ANTS_DVR_CONFIG_MULTICASTCFG 20107   //   单个通道多播<-> ANTS_DVR_MULTICAST_CHANCFG


/*************************参数配置命令 end*******************************/


// 控制命令

// 远程IPC 对时
#define ANTS_DVR_CONTROL_REMOTE_ADJUSTTIME        110
#define ANTS_DVR_CONTROL_REMOTE_REBOOT            111

// 解码器播放控制

#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_PLAY            200
#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_PAUSE           201  // DWORD ：1-暂停
#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_FAST            202 // DWORD :倍速
#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_SLOW            203 // DWORD ： 倍速
#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_STEP            204 // DWORD 0-关，1-开
#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_AUDIO           205   // DWORD :0-关 1-开
#define ANTS_DVR_CONTROL_DECODER_PLAYBACK_NOVIDEO         206
#define ANTS_DVR_CONTROL_DECODER_CLEAR_BUFFER             207  //清除解码缓冲,暂时不分通道


#define ANTS_DVR_CONTROL_DDNS_ATLANTISDNS_CONFIRM         300 // 意大利DDNS定制



//能力集获取指令
#define COMPRESSIONCFG_ABILITY_V2  0x401    //获取压缩参数能力获取

#define CHANNEL_PTZ_ABILITY  0x500    //获取PTZ能力获取

#define MATRIXDECODER_ABILITY 0x200 //多路解码器显示、解码能力

#define ANTSMID_DEVICESUPPORT_ABILITY_SENSOR 0x801
#define ANTSMID_DEVICESUPPORT_ABILITY_BACKUPFILESUFFIX 0x802 // 备份文件后缀名


#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI 0x900
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_SEACH 0x901// 隐藏自动搜索

#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_DEVICE 0x902 // 设备参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_CHAN 0x903 // 通道参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_NET 0x904 // 网络参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_ALARM 0x905 // 报警参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_USER 0x906 // 用户参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_SYSTEM 0x907 // 系统参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_MAIN 0x908 // 主菜单参数
#define ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_SMART 0x909 // 智能参数，通道参数结构体不够用了





#define ANTSMID_FRAME_STARTCODE 					(0xAB010000)
#define ANTSMID_MOTION_STARTCODE 				   	(0xAC010000)


// 28181 配置
#define ANTS_DVR_MANAGERHOST_TYPE_AGENT_SH      1
#define ANTS_DVR_MANAGERHOST_TYPE_TIANST_UC     2
#define ANTS_DVR_MANAGERHOST_TYPE_TIANST_IPVS   3
#define ANTS_DVR_MANAGERHOST_TYPE_MINGSOFT      4
#define ANTS_DVR_MANAGERHOST_TYPE_GB28181       5
#define ANTS_DVR_MANAGERHOST_TYPE_ICLOUD        6

//face dect, fire dect, video diagnose, add by huang, 2015/12/16
#define ANTS_MID_MAX_FIRE_RECT 10
#define ANTS_MID_MAX_FACES	(50)
//人脸检测
#define ANTS_DVR_CONFIG_FACE_DETECT  20124   //ANTS_MID_FACE_DETECT_CFG
#define ANTS_DVR_CONFIG_FACE_DETECT_LINK	20125   //ANTS_DVR_IVS_DETECT_LINK
//视频诊断
#define ANTS_DVR_CONFIG_VIDEO_DIAGNOSE  20126   //ANTS_MID_VIDEO_DIAGNOSE_CFG
#define ANTS_DVR_CONFIG_VIDEO_DIAGNOSE_LINK	20127   //ANTS_DVR_IVS_DETECT_LINK
//火焰检测
#define ANTS_DVR_CONFIG_FIRE_DETECT  20128   //ANTS_MID_FIRE_DETECT_CFG
#define ANTS_DVR_CONFIG_FIRE_DETECT_LINK	20129   //ANTS_DVR_IVS_DETECT_LINK
//声音告警
#define ANTS_DVR_CONFIG_IVS_SOUND_ALARM_RULE		20121  //声音报警	 ANTS_MID_SOUND_ALARM_ALL
#define ANTS_DVR_CONFIG_IVS_SOUND_ALARM_LINK		20116  //声音检测	 ANTS_DVR_IVS_DETECT_LINK


typedef enum
{
	eRetType_Success = 0, //成功	
	eRetType_Success_Reboot = 1, //成功，但是要重启
	eRetType_Fail = -1, //失败
	eRetType_Fail_Conflict= -2, //失败，与其他功能冲突
}ANTS_TYPE_RET;


typedef struct 
{
	char byEnable_infant; //检测婴儿哭声
	char bySensitiveGrade_infant; //=1 low   =2 mid  =3 high
	
	char byEnable_screaming;//检测尖叫声
	char bySensitiveGrade_screaming; //=1 low   =2 mid  =3 high
	
	char byEnable_gunshot;//检测枪声
	char bySensitiveGrade_gunshot; //=1 low   =2 mid  =3 high
	
	char byEnable_explosion;//检测爆炸声
	char bySensitiveGrade_explosion; //=1 low   =2 mid  =3 high
	
	BYTE  byRes[8];
}ANTS_MID_SOUND_ALARM_ALL;

typedef struct
{
    int size;
    char frozen; // 0-unfrozen, 1-frozen
    char res[3];
} ANTS_DVR_FREEZECFG_STATUS_S;

typedef struct
{
    int size;
    char frozen; // 0-unfrozen, 1-frozen
    char res1[3];
    char password[32];
    char res2[8];
} ANTS_DVR_FREEZECFG_REQUEST_S;

typedef struct 
{
    int size;
    char password[32];
    char res1[12];
} ANTS_DVR_FREEZECFG_PARAM_S;

typedef struct
{
    int size;
    char frozen; // 0-unfrozen, 1-frozen
    char res1[3];
    char password[32];
    char res2[8];
} ANTS_DVR_FREEZECFG_S;


// 相对坐标
typedef struct
{
	unsigned short numerator;						// 分子
	unsigned short denominator;						// 分母,必须大于0
}ANTS_MID_RELATIVE_POS;

// 目标矩形位置
typedef struct
{
	ANTS_MID_RELATIVE_POS x;							// 左上角x坐标
	ANTS_MID_RELATIVE_POS y;							// 左上角y坐标
	ANTS_MID_RELATIVE_POS w;							// 宽度
	ANTS_MID_RELATIVE_POS h;							// 高度
}ANTS_MID_RECT;


typedef struct 
{
	DWORD dwEnabled; //是否开启人脸检测
	ANTS_MID_RELATIVE_POS minSize;
	ANTS_MID_RELATIVE_POS maxSize;
}ANTS_MID_FACE_DETECT_CFG;

typedef struct
{
	ANTS_MID_RECT faces[ANTS_MID_MAX_FACES];				// 目标矩形
	int faceNum;					// 目标个数
}ANTS_MID_FACE_RESULT;


// 视频诊断类型枚举定义(顺序和视频诊断类型宏定义严格定义)
typedef enum
{
	ANTSMID_diagnose_type_color = 0,// 颜色异常(偏色) ==========
	ANTSMID_diagnose_type_dark, // 亮度过暗
	ANTSMID_diagnose_type_bright, // 亮度过亮  =============
	ANTSMID_diagnose_type_stripe, // 条纹干扰
	ANTSMID_diagnose_type_snowflake, // 雪花干扰
	ANTSMID_diagnose_type_shield, // 视频遮挡
	ANTSMID_diagnose_type_freeze, // 画面冻结	
	ANTSMID_diagnose_type_vlost, // 视频丢失 (暂未实现)
	ANTSMID_diagnose_type_blur, // 图像模糊  =============
	ANTSMID_diagnose_type_jitter, // 画面抖动 (暂未实现)
	ANTSMID_diagnose_type_ptz, // PTZ异常 (暂未实现)
	ANTSMID_diagnose_type_num				// 诊断类型个数
}ANTSMID_VIDEODIAGNOSE_TYPE;

typedef struct
{
	DWORD type; //为ANTSMID_VIDEODIAGNOSE_TYPE按位或, 为0时，停止视频诊断
				/*处理方式,处理方式的"或"结果*/
				/*0x00: 无响应*/
				/*0x01: ANTSMID_diagnose_type_color*/
				/*0x02: ANTSMID_diagnose_type_dark*/
				/*0x04: ANTSMID_diagnose_type_bright*/
				/*0x08: ANTSMID_diagnose_type_stripe*/
				/*0x10: ANTSMID_diagnose_type_snowflake*/
				/*0x20; ANTSMID_diagnose_type_shield*/
				/*0x40;ANTSMID_diagnose_type_freeze*/
				/*0x80;ANTSMID_diagnose_type_vlost*/
				/*0x100;ANTSMID_diagnose_type_blur*/
				/*0x200;ANTSMID_diagnose_type_jitter*/
				/*0x400;ANTSMID_diagnose_type_ptz*/
	int sensitive[ANTSMID_diagnose_type_num];
}ANTS_MID_VIDEO_DIAGNOSE_CFG;


typedef struct
{
	BYTE dwEnabled;
	BYTE dwSensitive;// 设置火灾检测灵敏度，灵敏度范围是40-100，数值代表的是火焰区域的大小，如果用户不设置的话，默认是50,值越小
					//可检测区域越小越容易报警，同时误检会增多，漏检减少，越大越不容易报警，同时误检减少，漏检增多
	ANTS_MID_RECT tRect;
}ANTS_MID_FIRE_DETECT_CFG;

//检测返回结果
typedef struct
{
	ANTS_MID_RECT result_rect[ANTS_MID_MAX_FIRE_RECT];
	unsigned short result_cnt;
}ANTS_MID_FIRE_RESULT;


//wifi setting, add by huang, 2015/09/23
#define ANTS_DVR_GET_WIFIWORKSTATUS     172 // 读取工作状态. 结构体ANTS_DVR_WIFI_WORKSTATUS_S
#define ANTS_DVR_GET_WIFIAPCOUNT        174	// 读取AP列表
#define ANTS_DVR_GET_WIFIAPLIST         175	// 读取AP列表. 结构体ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S
#define ANTS_DVR_GET_WIFISCANCOUNT      176 // 读取搜索AP结果
#define ANTS_DVR_GET_WIFISCANRESULT     177 // 读取搜索AP结果. 结构体ANTS_DVR_WIFI_SCANAPITEM_S
#define ANTS_DVR_SET_WIFIADDAP          178 // 添加AP,或者修改AP参数,包括连接AP动作. 结构体ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S
#define ANTS_DVR_SET_WIFIDELAP          179 // 删除AP,包括断开动作. 结构体ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S




//IVS setting, add by huang, 2015/10/16
//设置和获取智能检测联动方式
#define ANTS_DVR_CONFIG_IVS_COUNTER_WIRE_LINK		20112  //计数线检测  ANTS_DVR_IVS_DETECT_LINK

#define ANTS_DVR_CONFIG_IVS_DETECT_WIRE_LINK		20113  //过线检测     ANTS_DVR_IVS_DETECT_LINK

#define ANTS_DVR_CONFIG_IVS_DETECT_REGION_LINK		20114  //区域检测	 ANTS_DVR_IVS_DETECT_LINK

#define ANTS_DVR_CONFIG_IVS_OBJECT_REGION_LINK		20115  //物品检测	 ANTS_DVR_IVS_DETECT_LINK

#define ANTS_DVR_CONFIG_IVS_SOUND_ALARM_LINK		20116  //声音检测	 ANTS_DVR_IVS_DETECT_LINK


//设置和获取智能检测规则
#define ANTS_DVR_CONFIG_IVS_COUNTER_WIRE_RULE		20117  //计数线检测  ANTS_MID_COUNTER_WIRE_ALL  目标计数

#define ANTS_DVR_CONFIG_IVS_DETECT_WIRE_RULE		20118  //过线检测    ANTS_MID_DETECT_WIRE_ALL   虚拟警戒线

#define ANTS_DVR_CONFIG_IVS_DETECT_REGION_RULE		20119  //区域检测	 ANTS_MID_DETECT_REGION_ALL  区域检测

#define ANTS_DVR_CONFIG_IVS_OBJECT_REGION_RULE		20120  //物品检测	 ANTS_MID_OBJECT_REGION_ALL  物品检测

#define ANTS_DVR_CONFIG_IVS_SOUND_ALARM_RULE		20121  //声音报警	 ANTS_MID_SOUND_ALARM_ALL

//智能移动侦测
#define ANTS_DVR_CONFIG_MOTION_DETECT  20130   //ANTS_MID_MOTION_DETECT_ALL

#define ANTS_DVR_CONFIG_MOTION_DETECT_LINK	20131   //ANTS_DVR_IVS_DETECT_LINK


// 检测线/检测区域的最大顶点数
#define	ANTS_MID_IVS_MAX_VERTEX_CNT		(20)

// 最大的检测线条数
#define ANTS_MID_IVS_MAX_WIRE_CNT		(4)

// 最大的检测区域个数
#define ANTS_MID_IVS_MAX_REGION_CNT		(4)

// 单条规则同时输出的最大目标位置数
#define ANTS_MID_IVS_MAX_OBJ_CNT			(10)

#define ANTS_MID_IVS_MAX_MD_GRID_CNT	    (128)

// ivs类型
typedef enum
{
	ANTS_MID_IVS_TYPE_INVALId = 0x00,
	ANTS_MID_IVS_TYPE_COUNTER= 0x01,							// 目标计数
	ANTS_MID_IVS_TYPE_WIRE	= 0x02,							// 跨线检测
	ANTS_MID_IVS_TYPE_REGION	= 0x04,							// 区域检测
	ANTS_MID_IVS_TYPE_OBJECT = 0x08,							// 物品检测
	ANTS_MID_IVS_TYPE_MOTION	= 0x10,							// 移动侦测
	ANTS_MID_IVS_TYPE_all	= ANTS_MID_IVS_TYPE_COUNTER | ANTS_MID_IVS_TYPE_WIRE | ANTS_MID_IVS_TYPE_REGION | ANTS_MID_IVS_TYPE_OBJECT | ANTS_MID_IVS_TYPE_MOTION,

	ANTS_MID_IVS_SOUND_ALARM = 0xf0,
	ANTS_MID_IVS_PLATE_ALARM = 0xf1,
}ANTS_MID_IVS_TYPE;

// 计数检测的类型
typedef enum
{
	ANTS_MID_ecnt_add_only,									// 沿检测线的方向相加
	ANTS_MID_ecnt_add_sub,									// 沿检测线的方向相加;沿检测线的反方向相减
	ANTS_MID_ecnt_add_both									// 沿检测线的方向相加;沿检测线的反方向相加
}ANTS_MID_COUNTER_DETECT_TYPE;

// 区域检测的类型
typedef enum
{
	ANTS_MID_region_dir_in,									// 进入
	ANTS_MID_region_dir_out,									// 离开
	ANTS_MID_region_dir_both,								// 进入/离开
	ANTS_MID_region_linger									// 逗留/徘徊
}ANTS_MID_REGION_DETECT_TYPE;

// 物品检测类型
typedef enum  
{
	ANTS_MID_object_lost,									// 物品丢失, 判断超过最小检测尺寸的目标在检测区域内消失时间是否超过丢失时间	
	ANTS_MID_object_left,									// 物品遗留, 判断超过最小检测尺寸的目标在检测区域内静止时间是否超过遗留时间
	ANTS_MID_object_both										// 物品遗留/丢失
}ANTS_MID_OBJECT_DETECT_TYPE;

// 相对坐标
typedef struct ANTS_MID_RELATIVE_COORDINATE_T
{
	int numerator;									// 分子
	int denominator;								// 分母,必须大于0
}ANTS_MID_RELATIVE_COORDINATE;

// 相对面积
typedef struct ANTS_MID_RELATIVE_AREA_T
{
	int area;										// 指定的面积大小
	int total_area;									// 图像的总面积大小(w*h),必须大于0
}ANTS_MID_RELATIVE_AREA;

// 目标坐标点位置
typedef struct  
{
	ANTS_MID_RELATIVE_COORDINATE x;
	ANTS_MID_RELATIVE_COORDINATE y;
}ANTS_MID_OBJECT_COORDINATE;

// 目标矩形位置
typedef struct
{
	ANTS_MID_RELATIVE_COORDINATE x;							// 左上角x坐标
	ANTS_MID_RELATIVE_COORDINATE y;							// 左上角y坐标
	ANTS_MID_RELATIVE_COORDINATE w;							// 宽度
	ANTS_MID_RELATIVE_COORDINATE h;							// 高度
}ANTS_MID_OBJECT_POSITION;

// 检测线的方向
typedef struct  
{
	ANTS_MID_OBJECT_COORDINATE start_pt;						// 起始点坐标
	ANTS_MID_OBJECT_COORDINATE end_pt;						// 终点坐标
}ANTS_MID_WIRE_DIRECTION;

//点的相对坐标
typedef struct
{
	ANTS_MID_RELATIVE_COORDINATE	x;	//区域起点x在屏宽中的比例
	ANTS_MID_RELATIVE_COORDINATE	y;	//区域起点y在屏高中的比例
}ANTS_MID_ABSTRACT_POINT;

typedef struct
{
	int x;
	int y;
}ants_real_point;



/* 箭头点排列状态;
4  2    6  8
 \/0____1\/
 /\      /\
5  3    7  9
*/


typedef enum
{
	ANTS_ARROW_AT_START = 0x01,	//起点有箭头
	ANTS_ARROW_AT_STOP = 0x02,		//终点有箭头
	ANTS_ARROW_AT_START_EXT = 0x04,	//起点有反向箭头
	ANTS_ARROW_AT_STOP_EXT = 0x08,	//起点有反向箭头
}ants_arrow_type;

typedef struct
{
	int mask;
	ANTS_MID_OBJECT_COORDINATE points[10];
}ants_arrow_struct;

typedef struct
{
	BOOL rate_enable; //是否启用流量统计
	DWORD rate_alarm_interval; //在这段时间内通过的物品数
	DWORD alarm_num;

	BOOL total_enable;  //是否启用总量统计
	DWORD start_time;
	DWORD stop_time;
	DWORD total_alarm_num;

	ANTS_MID_OBJECT_COORDINATE result_pos; //结果OSD位置
}ANTS_MID_COUNTER_ALARM;

// 计数线信息
typedef struct
{
	ANTS_MID_COUNTER_DETECT_TYPE type;						// 检测类型
	ANTS_MID_OBJECT_COORDINATE vertex[ANTS_MID_IVS_MAX_VERTEX_CNT];	// 检测线的顶点
	int vertex_cnt;									// 检测线的顶点个数	, >= 2
	ANTS_MID_WIRE_DIRECTION direction;						// 检测方向,方向的起点一般是检测线中点的坐标,终点和起点不能相同
	ANTS_MID_RELATIVE_AREA min_area;							// 目标检测的最小尺寸, 分子为-1时表示使用默认尺寸
	ANTS_MID_COUNTER_ALARM counter_alarm;
	ants_arrow_struct arrow;						//将方向转换成箭头方便界面解析显示	
}ANTS_MID_COUNTER_WIRE;

// 检测线信息, 如果检测线方向的起始点和终止点的坐标相等,则表示双向检测
typedef struct 
{								
	ANTS_MID_OBJECT_COORDINATE vertex[ANTS_MID_IVS_MAX_VERTEX_CNT];	// 检测线的顶点
	int vertex_cnt;									// 检测线的顶点个数	, >= 2
	ANTS_MID_WIRE_DIRECTION direction;						// 检测方向,方向的起点一般是检测线中点的坐标,终点和起点相同时表示双向检测
	ANTS_MID_RELATIVE_AREA min_area;							// 目标检测的最小尺寸, 分子为-1时表示使用默认尺寸
	BOOL bidirectional;				//是否为双向检测
	ants_arrow_struct arrow;						//将方向转换成箭头方便界面解析显示
}ANTS_MID_DETECT_WIRE;

// 区域检测的信息,必须是一个闭合的区域
typedef struct 
{
	ANTS_MID_OBJECT_COORDINATE vertex[ANTS_MID_IVS_MAX_VERTEX_CNT];	// 检测区域的顶点
	int vertex_cnt;									// 检测区域的顶点个数, >= 3	
	ANTS_MID_REGION_DETECT_TYPE detect_type;					// 区域检测类型
	int linger_time;								// 目标逗留/徘徊时间, 单位:秒
	ANTS_MID_RELATIVE_AREA min_area;							// 目标检测的最小尺寸, 分子为-1时表示使用默认尺寸
	ants_arrow_struct arrow;						//将方向转换成箭头方便界面解析显示
}ANTS_MID_DETECT_REGION;

// 物品检测的信息,必须是一个闭合的区域
typedef struct 
{
	ANTS_MID_OBJECT_COORDINATE vertex[ANTS_MID_IVS_MAX_VERTEX_CNT];	// 检测区域的顶点
	int vertex_cnt;									// 检测区域的顶点个数, >= 3	
	ANTS_MID_OBJECT_DETECT_TYPE detect_type;					// 物品检测类型
	int delay_time;									// 目标遗留/丢失时间, 单位:秒, 范围: [0, IVS_MAX_DELAY_TIME]
	ANTS_MID_RELATIVE_AREA min_area;							// 目标检测的最小尺寸, 分子为-1时表示使用默认尺寸
	ants_arrow_struct arrow;						//将方向转换成箭头方便界面解析显示
}ANTS_MID_OBJECT_REGION;


typedef struct 
{
	BOOL bLocal;
	BOOL bEnable;  //是否启用
	DWORD dwNum; //检测线数目
	ANTS_MID_COUNTER_WIRE counter_wire[ANTS_MID_IVS_MAX_WIRE_CNT];
}ANTS_MID_COUNTER_WIRE_ALL;

typedef struct 
{
	BOOL bLocal;
	BOOL bEnable;
	DWORD dwNum;
	ANTS_MID_DETECT_WIRE detect_wire[ANTS_MID_IVS_MAX_WIRE_CNT];
}ANTS_MID_DETECT_WIRE_ALL;

typedef struct 
{
	BOOL bLocal;
	BOOL bEnable;
	DWORD dwNum;
	ANTS_MID_DETECT_REGION detect_region[ANTS_MID_IVS_MAX_REGION_CNT];
}ANTS_MID_DETECT_REGION_ALL;

typedef struct 
{
	BOOL bLocal;
	BOOL bEnable;
	DWORD dwNum;
	ANTS_MID_OBJECT_REGION object_region[ANTS_MID_IVS_MAX_REGION_CNT];
}ANTS_MID_OBJECT_REGION_ALL;


// 移动侦测区域类型
typedef enum
{
	ANTS_MD_REGION_GRID,								// 网格方式
	ANTS_MD_REGION_POLYGON								// 多边形方式
}ANTS_MID_MD_REGION_TYPE;

// 多边形移动侦测区域
typedef struct
{
	ANTS_MID_OBJECT_COORDINATE vertex[ANTS_MID_IVS_MAX_VERTEX_CNT];	// 检测区域的顶点
	int vertex_cnt;									// 检测区域的顶点个数, >= 3	
}ANTS_MID_MOTION_REGION_POLYGON;

// 网格移动侦测区域
// 以22*18为例: bit_flag[0]的bit0-21位对应第一行的0-21格;bit_flag[0]的bit22-31位对应第二行的0-9格; 依次类推...
typedef struct
{
	int rows;										// 网格的行数
	int cols;										// 网格的列数
	unsigned int bit_flag[ANTS_MID_IVS_MAX_MD_GRID_CNT];	// bit为1时表示开启检测
}ANTS_MID_MOTION_REGION_GRID;

typedef struct 
{
	ANTS_MID_MD_REGION_TYPE eType;							// 区域模式: 0-网格模式 1-坐标模式 
	union{
		ANTS_MID_MOTION_REGION_POLYGON polygon;
		ANTS_MID_MOTION_REGION_GRID grid;
	}region;
	ANTS_MID_RELATIVE_AREA min_area;							// 目标检测的最小尺寸, 分子为-1时表示使用默认尺寸
	DWORD dwSensitivity;								// 灵敏度, 0~4, 灵敏度越高越容易报警
}ANTS_MID_MOTION_DETECT_CFG;

typedef struct
{
	BOOL is_local;
	BOOL enable;
	DWORD num;		//如果是格子模式 num只能等于1
	ANTS_MID_MOTION_DETECT_CFG motion[ANTS_MID_IVS_MAX_REGION_CNT];
}ANTS_MID_MOTION_DETECT_ALL;



//规则结果结构体 ----推送至客户端

typedef struct
{
	ANTS_MID_COUNTER_DETECT_TYPE type;	
	ANTS_MID_WIRE_DIRECTION direction;
	ANTS_MID_RELATIVE_AREA min_area;	
	ANTS_MID_COUNTER_ALARM counter_alarm;
	int vertex_cnt;	
	DWORD start_seek;
}ANTS_MID_SFRAME_COUNTE_RULE;

typedef struct 
{
	ANTS_MID_WIRE_DIRECTION direction;	
	ANTS_MID_RELATIVE_AREA min_area;		
	int vertex_cnt;		
	DWORD start_seek;
}ANTS_MID_SFRAME_WIRE_RULE;

typedef struct 
{
	ANTS_MID_REGION_DETECT_TYPE detect_type;	
	int linger_time;							
	ANTS_MID_RELATIVE_AREA min_area;		
	int vertex_cnt;
	DWORD start_seek;
}ANTS_MID_SFRAME_DETECT_RULE;

typedef struct 
{
	ANTS_MID_OBJECT_DETECT_TYPE detect_type;	
	int delay_time;	
	ANTS_MID_RELATIVE_AREA min_area;	
	int vertex_cnt;	
	DWORD start_seek;
}ANTS_MID_SFRAME_OBJECT_RULE;

typedef struct 
{
	ANTS_MID_IVS_TYPE type;
	DWORD	id;
	union
	{
		ANTS_MID_SFRAME_COUNTE_RULE counter;
		ANTS_MID_SFRAME_WIRE_RULE wire;
		ANTS_MID_SFRAME_DETECT_RULE region;
		ANTS_MID_SFRAME_OBJECT_RULE object;
	}rule;
}ants_sframe_smart_check_rule;


typedef struct 
{
	ANTS_MID_IVS_TYPE type;
	DWORD	id;
	union
	{
		ANTS_MID_SFRAME_COUNTE_RULE counter;
		ANTS_MID_SFRAME_WIRE_RULE wire;
		ANTS_MID_SFRAME_DETECT_RULE region;
		ANTS_MID_SFRAME_OBJECT_RULE object;
	}rule;
}ANTS_MID_SFRAME_SMART_CHECK_RULE;


//结果结构体  ----推送至客户端

// 计数结果,目标运动方向和检测线方向一致时目标参与计数
typedef struct  
{
	int wire_id;									// 检测线id, [0, ANTS_IVS_MAX_WIRE_CNT)
	int count;										// 当前帧的计数结果

	BOOL total_enable;							//总量计数是否开启
	BOOL real_rate_enable;						//流量计数是否开启
	int total_count;								// 总共跨线的目标数。
	int real_count;									// 实时统计
	int total_alarm_num;							//	总量报警阈值
	int rate_alarm_num;								//	流量报警阈值
	BOOL is_need_alarm;						//	是否需要报警
	ANTS_MID_OBJECT_COORDINATE pos;						// 结果显示位置							
	char resv[8];									//	保留位
}ANTS_MID_COUNTER_RESULT;

// 跨线检测结果, 目标方向和检测线方向一致时产生报警
typedef struct 
{
	int wire_id;									// 检测线id, [0, ANTS_IVS_MAX_WIRE_CNT)
	ANTS_MID_OBJECT_POSITION obj_info[ANTS_MID_IVS_MAX_OBJ_CNT];	// 触发规则的目标位置
	int obj_cnt;									// 触发规则的目标个数
	char resv[16];									//	保留位
}ANTS_MID_WIRE_RESULT;

// 区域检测结果,检测结果和设置规则一致时产生报警
typedef struct
{
	int region_id;									// 区域id, [0, ANTS_IVS_MAX_REGION_CNT)
	ANTS_MID_REGION_DETECT_TYPE detect_type;					// 区域检测类型
	ANTS_MID_OBJECT_POSITION obj_info[ANTS_MID_IVS_MAX_OBJ_CNT];		// 触发规则的目标位置
	int obj_cnt;									// 触发规则的目标个数
	char resv[16];									//	保留位
}ANTS_MID_REGION_RESULT;

// 物品检测结果,检测结果和设置规则一致时产生报警
typedef struct
{
	int region_id;									// 区域id, [0, ANTS_IVS_MAX_REGION_CNT)
	ANTS_MID_OBJECT_DETECT_TYPE detect_type;					// 物品检测类型
	ANTS_MID_OBJECT_POSITION obj_info[ANTS_MID_IVS_MAX_OBJ_CNT];		// 触发规则的目标位置
	int obj_cnt;									// 触发规则的目标个数
	char resv[16];									//	保留位
}ANTS_MID_OBJECT_RESULT;

typedef struct
{
	char resv[128];	
}ANTS_MID_MOTION_RESULT ;

// ivs检测结果
typedef struct _ants_ivs_detect_result 
{
	DWORD size;
	ANTS_MID_IVS_TYPE type;									// ivs类型
	union{
		ANTS_MID_COUNTER_RESULT counter;
		ANTS_MID_WIRE_RESULT wire;
		ANTS_MID_REGION_RESULT region;
		ANTS_MID_OBJECT_RESULT object;
		ANTS_MID_MOTION_RESULT motion;
	}result;										// ivs检测结果
	char resv[4];	//保留
}ANTS_MID_IVS_DETECT_RESULT;


//车牌识别
#define ANTS_DVR_CONFIG_PLATE_DETECT  20122   //ANTS_MID_PLATE_DETECT_CFG

#define ANTS_DVR_CONFIG_PLATE_DETECT_LINK	20123   //ANTS_DVR_IVS_DETECT_LINK

// 单幅图片中的最大车牌个数
#define ANTS_MID_IMG_MAX_PLATE_CNT	10

// 车牌字符串的长度
#define ANTS_MID_PLATE_STRING_LEN    12

// 车牌省份/直辖市简称
typedef enum
{
	ANTS_MID_VLPR_PROVICE_CHUAN,	// "川", 
	ANTS_MID_VLPR_PROVICE_YU1,		// "渝", 
	ANTS_MID_VLPR_PROVICE_E,		// "鄂", 
	ANTS_MID_VLPR_PROVICE_GAN1,		// "赣", 
	ANTS_MID_VLPR_PROVICE_GUI1,		// "桂", 
	ANTS_MID_VLPR_PROVICE_HEI,		// "黑", 
	ANTS_MID_VLPR_PROVICE_HU,		// "沪", 
	ANTS_MID_VLPR_PROVICE_JI1,		// "冀", 
	ANTS_MID_VLPR_PROVICE_JIN1,		// "津", 
	ANTS_MID_VLPR_PROVICE_JING,		// "京", 
	ANTS_MID_VLPR_PROVICE_JI2,		// "吉", 
	ANTS_MID_VLPR_PROVICE_LIAO,		// "辽", 
	ANTS_MID_VLPR_PROVICE_LU,		// "鲁", 
	ANTS_MID_VLPR_PROVICE_MIN,		// "闽", 
	ANTS_MID_VLPR_PROVICE_NING,		// "宁", 
	ANTS_MID_VLPR_PROVICE_QING,		// "青", 
	ANTS_MID_VLPR_PROVICE_QIONG,	// "琼", 
	ANTS_MID_VLPR_PROVICE_SHAN,		// "陕", 
	ANTS_MID_VLPR_PROVICE_SU,		// "苏", 
	ANTS_MID_VLPR_PROVICE_JIN2,		// "晋", 
	ANTS_MID_VLPR_PROVICE_WAN,		// "皖", 
	ANTS_MID_VLPR_PROVICE_XIANG,	// "湘", 
	ANTS_MID_VLPR_PROVICE_YU2,		// "豫", 
	ANTS_MID_VLPR_PROVICE_YUE,		// "粤", 
	ANTS_MID_VLPR_PROVICE_YUN,		// "云", 
	ANTS_MID_VLPR_PROVICE_ZANG,		// "藏", 
	ANTS_MID_VLPR_PROVICE_ZHE,		// "浙", 
	ANTS_MID_VLPR_PROVICE_MENG,		// "蒙", 
	ANTS_MID_VLPR_PROVICE_GAN2,		// "甘", 
	ANTS_MID_VLPR_PROVICE_XIN,		// "新", 
	ANTS_MID_VLPR_PROVICE_GUI2,		// "贵"
	ANTS_MID_VLPR_PROVICE_CNT
}ANTS_MID_E_PLATE_PROVICE;

// 车牌颜色
typedef enum  
{
	ANTS_MID_plate_color_blue,								// 蓝牌(蓝底白字)
	ANTS_MID_plate_color_yellow,							// 黄牌(黄底黑字)
	ANTS_MID_plate_color_white,								// 白牌(白底黑字)
	ANTS_MID_plate_color_black,								// 黑牌(黑底白底)
}ANTS_MID_PLATE_COLOR;

// 车牌类型
typedef enum
{
	ANTS_MID_plate_type_common_military,					// 普通军牌
	ANTS_MID_plate_type_common_civil,						// 普通民用车牌
}ANTS_MID_PLATE_TYPE;

// 车牌检测的区域
typedef ANTS_MID_RECT ANTS_MID_REGION_RECT;

typedef struct 
{
	DWORD dwEnabled; //是否开启车牌检测
	DWORD dwSensitive; //置信度 0-100
	ANTS_MID_E_PLATE_PROVICE ePlateProvice;
	ANTS_MID_REGION_RECT tRect; //区域的宽高设置为0时，为检测整个区域
}ANTS_MID_PLATE_DETECT_CFG;


//end IVS setting, add by huang, 2015/10/16

typedef struct _tagANTS_DVR_GB28181CFG_CHANINFO
{
	DWORD dwLevel; // 报警级别
	char szChanID[32]; // 通道编号
}ANTS_DVR_GB28181CFG_CHANINFO,*LPANTS_DVR_GB28181CFG_CHANINFO;

typedef struct _tagANTS_DVR_GB28181CFG
{
	
	// SIP
	char szSIP_Code[32]; // SIP服务器编号,如34020000002000000001
	char szSIP_Zone[32]; // SIP服务器域,如3402000000
	char szSIP_DeviceID[32]; // SIP设备编号,如 34020000001320000001
	char szSIP_Password[32]; // SIP服务器密码
	char szSIP_IP[32]; // IP
	WORD wSIP_Port; // SIP服务器端口,默认5060
	WORD wLocal_Port; // 本地端口，默认5060
	DWORD wExpires; // 注册有效期,秒默认 3600
	WORD wKeepalive; // 心跳周期,默认60秒
	WORD wKeepaliveCnt; // 最大心跳超时次数 默认 3次
	DWORD dwRes[8];
	
}ANTS_DVR_GB28181CFG,*LPANTS_DVR_GB28181CFG;


  typedef struct{		
	ANTS_DVR_GB28181CFG tGB28181CFG;
	ANTS_DVR_GB28181CFG_CHANINFO atChanInfo[MAX_CHANNUM_EX]; 
	ANTS_DVR_GB28181CFG_CHANINFO atAlarmInfo[MAX_CHANNUM_EX]; 
}ANTS_GB28181PARAM;



typedef enum {
	AntsHostMgrLibFrameType_IFrames=0x01,
	AntsHostMgrLibFrameType_AudioFrames=0x08,
	AntsHostMgrLibFrameType_PFrames=0x09,
	AntsHostMgrLibFrameType_MotionDetection=0x0b,
	AntsHostMgrLibFrameType_SysHeader=0x0e,
	AntsHostMgrLibFrameType_SubIFrames=0x12,
	AntsHostMgrLibFrameType_SubPFrames=0x13,
}AntsHostMgrLibFrameType_E;


typedef enum{
	Antsmid_VoiceCodecID_OggVorbis=0,
	Antsmid_VoiceCodecID_G711A=1,
	Antsmid_VoiceCodecID_G711U=2,
	Antsmid_VoiceCodecID_G722Ex=3,
	Antsmid_VoiceCodecID_G726Ex=4,
	Antsmid_VoiceCodecID_ADPCM=8
}Antsmid_VoiceCodecID_E;

typedef enum {
	Antsmid_VideoCodecID_H264_hisi=1,
	Antsmid_VideoCodecID_MJPEG_hisi=2,
	Antsmid_VideoCodecID_H264_hisi_high=4,
	Antsmid_VideoCodecID_H264_hisi_RTP=8,
}Antsmid_VideoCodecID_E;

typedef struct _tagAntsmid_AudioHeader{
	char cCodecId;			//!音频编码类型
	char cSampleRate;			//!采样率 单位KHz
	char cBitRate;				//!比特率 单位Kbps
	char cChannels;			//!通道数
	char cResolution;			//!分辨力
	char cResv[3];				//!保留位
}Antsmid_AudioHeader_T;

typedef struct _tagAntsmid_VideoHeader{
	WORD usWidth;				//!视频宽度
	WORD usHeight;				//!视频高度
	char cCodecId;				//!视频编码类型
	char cColorSpace;         // 0-yuv420,1-yuv422,2-444
	char cResv[2];					//!保留位
}Antsmid_VideoHeader_T;

typedef struct _tagAntsmid_MDEVTHeader{
	WORD usWidth;				//!视频宽度
	WORD usHeight;				//!视频高度
	char cMdAppear;				//!底层检测算法是否判定有移动发生
	char cResv[3];					//!保留位
}Antsmid_MDEVTHeader_T;


typedef struct _tagAntsmid_FrameHeader{
	DWORD uiStartId;					//!帧同步头ANTSMID_FRAME_STARTCODE
	DWORD uiFrameType;				//!帧类型
	DWORD uiFrameNo;					//!帧号
	DWORD uiFrameTime;				//!UTC时间
	DWORD uiFrameTickCount;			//!毫秒为单位的毫秒时间
	DWORD uiFrameLen;				//!帧载长度
										//!联合体,用于存储音频帧或是视频帧信息
	union {
		Antsmid_AudioHeader_T struAudioHeader;	//!音频帧信息
		Antsmid_VideoHeader_T struVideoHeader;	//!视频帧信息
		Antsmid_MDEVTHeader_T struMdevtHeader; // ! 移动侦测信息
	}uMedia;
	DWORD dwTimeStamp;				//!相对时间戳 ms * 90		
}Antsmid_FrameHeader_T;

//报警回调类型参数

typedef enum _tagAntsHostMgrLibAlarmType
{
	AntsHostMgrLibAlarmType_No = 0,
	AntsHostMgrLibAlarmType_Motion,
	AntsHostMgrLibAlarmType_AlarmIn,
	AntsHostMgrLibAlarmType_VideoLoss,
	AntsHostMgrLibAlarmType_Mask,// 遮挡报警
	AntsHostMgrLibAlarmType_DiskFull,
	AntsHostMgrLibAlarmType_DiskError,
	AntsHostMgrLibAlarmType_IllegeAccess, // 非法访问
	AntsHostMgrLibAlarmType_ReticleDisconnect, // 网线断
	AntsHostMgrLibAlarmType_IpConelict, // IP冲突
	AntsHostMgrLibAlarmType_MAX,
}AntsHostMgrLibAlarmType_E;



// 录像类型
typedef enum _tagAntsHostMgrLibRecordType
{	
	AntsHostMgrLibRecordType_All = 0,
	AntsHostMgrLibRecordType_Timer = 1,
	AntsHostMgrLibRecordType_Motion,
	AntsHostMgrLibRecordType_Alarm,
	AntsHostMgrLibRecordType_MotionOrAlarm ,
	AntsHostMgrLibRecordType_MotionAndAlarm = 5,
	AntsHostMgrLibRecordType_Command ,
	AntsHostMgrLibRecordType_Manual,
	AntsHostMgrLibRecordType_Snap = 23,
}AntsHostMgrLibRecordType_E;


typedef enum _tagAntsHostMgrLibConfigCmd 
{
	AntsHostMgrLibConfigCmd_UUID = 1, // 设备标识,比如P2P用于访问许可.
	
	// 上海地标,指定通道获取
	AntsHostMgrLibConfigCmd_AgentSH_CapturePictureCH=10100, //上海地标日常图像设置ANTS_DVR_SH_AGENT_CAPTUREPICTURE,指定通道
	// 28181 ,	配置接口中通道参数扩展为数组指针
	
	AntsHostMgrLibConfigCmd_GB28181 =10500, // ANTS_DVR_GB28181CFG
	AntsHostMgrLibConfigCmd_GB28181_ChanInfo =10501, // ANTS_DVR_GB28181CFG_CHANINFO
	AntsHostMgrLibConfigCmd_GB28181_AlarmInfo=10502, // ANTS_DVR_GB28181CFG_CHANINFO
	
}AntsHostMgrLibConfigCmd_E;


//远程控制命令
typedef enum _tagAntsHostMgrLibControlCmd 
{
	AntsHostMgrControlCmd_PTZ = 2,
	AntsHostMgrControlCmd_Reboot = 10,
	AntsHostMgrControlCmd_Halt,
	AntsHostMgrControlCmd_EnableAlarm, // 布防，撤防 U32
	AntsHostMgrControlCmd_RemoteHistoryPlayControl=20,
	AntsHostMgrControlCmd_RemoteRecordControl=25, // 远程录像控制
	AntsHostMgrControlCmd_RestoreConfig = 30, // 恢复默认配置	
	//AntsHostMgrControlCmd_GetUpdatePath = 35, // 获取升级路径	
	AntsHostMgrControlCmd_Sensor_BadPixelTest = 40, // 坏点检测返回缓冲int *pResult:	pResult[0] : 返回坏点个数pResult[1] : 返回停止阈值
	AntsHostMgrControlCmd_Sensor_AutoApertureCorrection,// 自动光圈校正,返回缓冲int *irisStopValue
	AntsHostMgrControlCmd_ExportConfig = 50, // 导入配置
	AntsHostMgrControlCmd_ImportConfig = 51, // 导出配置

}AntsHostMgrLibControlCmd_E;



typedef enum
{
    PTZ_OP_SetPreset = 0,
    PTZ_OP_DelPreset,
    PTZ_OP_CallPreset,

    PTZ_OP_SetCruise = 3,
    PTZ_OP_GetCruise,
    PTZ_OP_DelCruise,
    PTZ_OP_CallCruise,

    PTZ_OP_SetTrackStart = 7,
    PTZ_OP_SetTrackStop,
    PTZ_OP_DelTrack,
    PTZ_OP_CallTrack,

    PTZ_OP_SetScanStart = 11,
    PTZ_OP_SetScanStop,
    PTZ_OP_DelScan,
    PTZ_OP_CallScan,

    PTZ_OP_TurnStart = 15,
    PTZ_OP_TurnStop,

    PTZ_OP_SetGuardPos = 17, // 设置看守位
    PTZ_OP_GetGuardPos, // 设置看守位
    PTZ_OP_CallGuardPos, // 调用看守位操作

    PTZ_OP_SetIrLightCtrl = 20, // 红外灯
    PTZ_OP_GetIrLightCtrl, // 红外灯

    PTZ_OP_SetCoverStart = 22, // 设置遮盖区域
    PTZ_OP_SetCoverStop,
    PTZ_OP_SetCoverMove,
    PTZ_OP_SetCoverEnable,
    PTZ_OP_GetCoverEnable,

    PTZ_OP_WarmReset = 27,
    PTZ_OP_Restore = 28,
    PTZ_OP_SetAddr = 29,

    PTZ_OP_CheckStart = 0x20, // 核对参数开始
    PTZ_OP_CheckStop = 0x21, // 核对参数结束
	
	PTZ_OP_MenuUp = 0x30,
	PTZ_OP_MenuDown,
	PTZ_OP_MenuLeft,
	PTZ_OP_MenuRight,
	PTZ_OP_MenuEnter,

      PTZ_OP_3DPosition = 0x40, // 3D定位命令
      PTZ_OP_GETPTZPOS, // 获取球机坐标(方位角,仰角,倍率),数据结构定义PTZ_OP_GETPTZPOS_S
      PTZ_OP_SETPTZPOS, // 设置球机坐标(方位角,仰角,倍率),数据结构定义PTZ_OP_SETPTZPOS_S


	 // 辅助功能
    PTZ_OP_Wiper = 0x50, // 雨刷 0-关闭 1-开启
    PTZ_OP_SprayPos = 0x51, // 喷淋位置设置 0-设置 1-删除
    PTZ_OP_SprayMode = 0x52, // 喷淋模式 0-自动 1-手动(默认)
	
} PTZ_OP_E;

typedef struct 
{
    unsigned char type; // 0-不动作 1-预置点 2-巡航 3-轨迹 4-两点扫描
    unsigned char index; // 设置为第几组(第几组预置点或巡航等),从0开始计数.
    unsigned char idletime; // 空闲时间多久开始触发看守位
} PTZ_OP_GUARDPOS_S;

typedef struct
{
    unsigned char mode; // 0-自动 1-手动
    unsigned char lighting; // 按位表示每个灯,bit0~bit2分别表示近中远灯.
} PTZ_OP_IRLIGHTCTRL_S;

typedef struct
{
    unsigned char enable; // 按位表示每组cover是否启用.bit0代表第0组.
} PTZ_OP_COVERENABLE_S;


typedef struct
{
    // 3D定位时输入参数应传递起点和终点的坐标,以视频窗口为坐标系,原点为视频窗口左上角.
    // 将视频窗口沿两个坐标方向等分65536份,因此任一点坐标值的范围为(0,0)~(65535,65535).
    short azimuth; // 方位角,范围[-17999,18000]
    short evation; // 仰角,范围[-8999,9000]
    short zoom; // 倍率,范围[10,]
    unsigned char reserved[10]; // 保留字节
} PTZ_OP_GETPTZPOS_S;

typedef struct
{
    // 3D定位时输入参数应传递起点和终点的坐标,以视频窗口为坐标系,原点为视频窗口左上角.
    // 将视频窗口沿两个坐标方向等分65536份,因此任一点坐标值的范围为(0,0)~(65535,65535).
    char setAngle;//是否设置角度,1是   ,0否
    char setZoom;//是否设置倍率,1是   ,0否
    short azimuth; // 方位角,范围[-17999,18000]
    short evation; // 仰角,范围[-8999,9000]
    short zoom; // 倍率,范围[10,]
    unsigned char reserved[8]; // 保留字节
} PTZ_OP_SETPTZPOS_S;


typedef struct
{
    // 3D定位时输入参数应传递起点和终点的坐标,以视频窗口为坐标系,原点为视频窗口左上角.
    // 将视频窗口沿两个坐标方向等分65536份,因此任一点坐标值的范围为(0,0)~(65535,65535).
    unsigned short startx; // 起点水平坐标
    unsigned short starty; // 起点垂直坐标
    unsigned short stopx; // 终点水平坐标
    unsigned short stopy; // 终点垂直坐标
    unsigned char reserved[8]; // 保留字节
} PTZ_OP_3DPOSITION_S;



#define AntsHostMgrLibPTZCmd_PtzOp(cmd) AntsHostMgrLibPTZCmd_PtzOp##cmd
#define DEFINE_AntsHostMgrLibPTZCmd_PtzOp(cmd) AntsHostMgrLibPTZCmd_PtzOp##cmd = AntsHostMgrLibPTZCmd_PtzOp_Base + PTZ_OP_##cmd
typedef enum _tagAntsHostMgrLibPTZCmd
{
	AntsHostMgrLibPTZCmd_LightPowerOn = 2,
	AntsHostMgrLibPTZCmd_WiperPowerOn,
	AntsHostMgrLibPTZCmd_FanPowerOn,
	AntsHostMgrLibPTZCmd_HeaterPowerOn = 5,
	AntsHostMgrLibPTZCmd_AuxPowerOn1,
	AntsHostMgrLibPTZCmd_AuxPowerOn2,
	AntsHostMgrLibPTZCmd_SetPreset,
	AntsHostMgrLibPTZCmd_ClearPreset,
	AntsHostMgrLibPTZCmd_ZoomIn = 11,
	AntsHostMgrLibPTZCmd_ZoomOut,
	AntsHostMgrLibPTZCmd_FocusNear,
	AntsHostMgrLibPTZCmd_FocusFar,
	AntsHostMgrLibPTZCmd_IrisOpen = 15,
	AntsHostMgrLibPTZCmd_IrisClose,
	AntsHostMgrLibPTZCmd_Up = 21,
	AntsHostMgrLibPTZCmd_Down,
	AntsHostMgrLibPTZCmd_Left,
	AntsHostMgrLibPTZCmd_Right,
	AntsHostMgrLibPTZCmd_UpLeft = 25,
	AntsHostMgrLibPTZCmd_UpRight,
	AntsHostMgrLibPTZCmd_DownLeft,
	AntsHostMgrLibPTZCmd_DownRight,
	AntsHostMgrLibPTZCmd_PanAuto = 29,//以SS的速度左右自动扫描
	
	AntsHostMgrLibPTZCmd_SeqAddPreset = 30,
	AntsHostMgrLibPTZCmd_SeqSetDWell,// 巡航点停顿时间
	AntsHostMgrLibPTZCmd_SeqSetSpeed,// 速度
	AntsHostMgrLibPTZCmd_SeqDelPreset,//删除预置点

    AntsHostMgrLibPTZCmd_CruiseRecStart,// 开始记录轨迹
	AntsHostMgrLibPTZCmd_CruiseRecStop=35,// 停止记录轨迹
	AntsHostMgrLibPTZCmd_CruiseRun,//开始轨迹

    AntsHostMgrLibPTZCmd_SeqRun,//开始巡航
	AntsHostMgrLibPTZCmd_SeqStop,//停止巡航
	AntsHostMgrLibPTZCmd_GotoPreset,// 转到点
	AntsHostMgrLibPTZCmd_SeqFill=40,//将巡航序列设置到云台
	AntsHostMgrLibPTZCmd_Home,
	AntsHostMgrLibPTZCmd_Set3D,


    AntsHostMgrLibPTZCmd_TransPTZ = 100,// 透明通道
	AntsHostMgrLibPTZCmd_GetCruisePoint = 0x10000, // 获取巡航点
        AntsHostMgrLibPTZCmd_PtzOp_Base = 0x400,
        AntsHostMgrLibPTZCmd_PtzOp_Bottom = 0x7ff,
    
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetPreset),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(DelPreset),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CallPreset),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetCruise),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(GetCruise),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(DelCruise),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CallCruise),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetTrackStart),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetTrackStop),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(DelTrack),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CallTrack),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetScanStart),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetScanStop),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(DelScan),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CallScan),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(TurnStart),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(TurnStop),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetGuardPos), // 设置看守位
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(GetGuardPos), // 设置看守位
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CallGuardPos), // 调用看守位操作

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetIrLightCtrl), // 红外灯
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(GetIrLightCtrl), // 红外灯

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetCoverStart), // 设置遮盖区域
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetCoverStop),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetCoverMove),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetCoverEnable),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(GetCoverEnable),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(WarmReset),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(Restore),
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SetAddr),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CheckStart), // 核对参数开始
    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(CheckStop), // 核对参数结束
	
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(MenuUp),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(MenuDown),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(MenuLeft),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(MenuRight),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(MenuEnter),

    DEFINE_AntsHostMgrLibPTZCmd_PtzOp(3DPosition), // 3D定位命令
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(GETPTZPOS),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SETPTZPOS),
    
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(Wiper),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SprayPos),
	DEFINE_AntsHostMgrLibPTZCmd_PtzOp(SprayMode),
	
}AntsHostMgrLibPTZCmd_E;

typedef enum _tagAntsHostMgrLibRemoteHistoryPlayCmd
{
	AntsHostMgrLibRemoteHistoryPlayCmd_Play = 1, // 开始播放,参数uParams[0] ,0/1-正常播放,>0快放，<0慢放,1-1x,2-2x,3-3x,4-4x....
	AntsHostMgrLibRemoteHistoryPlayCmd_Pause, // 暂停uParams[0] :0-恢复,1-暂停
	AntsHostMgrLibRemoteHistoryPlayCmd_Seek, // 参数tPlayTime
	AntsHostMgrLibRemoteHistoryPlayCmd_KeyFrame,// 是否只放关键帧,参数 uParams[0]:0-所有，1-只弹I帧，2-每2I帧取1，3-每3I帧取1,...
	AntsHostMgrLibRemoteHistoryPlayCmd_Audio = 5,//是否打开音频uParams[0],0-不打开，1打开
	AntsHostMgrLibRemoteHistoryPlayCmd_GetPlayTime,// 输入 uParams[0]:0-当前时间，1-开始时间，2-结束时间,返回tPlayTime
	AntsHostMgrLibRemoteHistoryPlayCmd_PlayDirect, // 0-正放，1-倒放
	AntsHostMgrLibRemoteHistoryPlayCmd_AddCH, // 加入同步通道uParams[0] 通道号
	AntsHostMgrLibRemoteHistoryPlayCmd_DelCH, // 从同步组中移除通道
	AntsHostMgrLibRemoteHistoryPlayCmd_SetStreamType, // 切换流类型:uParams[0]:通道号 1,2,3;uParams[1]:0/1-主码流,2-子码流,3-主+子
}AntsHostMgrLibHistoryPlayCmd_E;


typedef enum _COMPRESSION_ABILITY_TYPE_
{
	COMPRESSION_STREAM_ABILITY = 0,
	MAIN_RESOLUTION_ABILITY,
	SUB_RESOLUTION_ABILITY,
	EVENT_RESOLUTION_ABILITY,
	FRAME_ABILITY, // 主码流帧率
	BITRATE_TYPE_ABILITY = 5,// 主码流码流类型
	BITRATE_ABILITY, // 主码流码率
	ENC_TYPE_ABILITY,// 主码流，子码流，事件
	STREAM_TYPE_ABILITY,// 视频流，复合流
	VIDEOENC_TYPE_ABILITY, // 0-H264,1-MPEG4,2-MJPEG
	H264PROFILE_TYPE_ABILITY = 10, // 0-Baseline,1-main,2-high
	// 以下新增,如果不存在则使用以上替代或者默认
	SUB_VIDEOENC_TYPE_ABILITY,// 子码流编码类型
	SUB_H264PROFILE_TYPE_ABILITY,// 子码流H264等级
	SUB_BITRATE_ABILITY, // 子码流码率
	SUB_BITRATE_TYPE_ABILITY,// 子码流码流类型
	SUB_FRAME_ABILITY = 15,//子码流帧率
	SUB_STREAM_TYPE_ABILITY,// 子码流视频流，复合流
	MAIN_QUALITY_ABILITY, // 主码流图像质量
	SUB_QUALITY_ABILITY,// 子码流图像质量
	MAIN_INTERVALFRAMEI_ABILITY,//主码流I帧间隔
	SUB_INTERVALFRAMEI_ABILITY = 20,// 子码流I帧间隔
}COMPRESSION_ABILITY_TYPE;

//校时结构参数
typedef struct _tagAntsHostMgrLibTime
{
	U32	dwYear;			//年
	U32	dwMonth;		//月
	U32	dwDay;			//日
	U32	dwHour;			//时
	U32	dwMinute;		//分
	U32	dwSecond;		//秒
	S32 dwZone;         //时区
}AntsHostMgrLibTime_T;

typedef struct _tagAntsHostMgrLibTimeV2
{
	U16	wYear;			//年
	U8	byMonth;		//月
	U8	byDay;			//日
	U8	byHour;			//时
	U8	byMinute;		//分
	U8	bySecond;		//秒
	U8  byZone;         //时区
}AntsHostMgrLibTimeV2_T;


//抓图相关结构
typedef struct _tagAntsHostCapturePictureInfo
{	
	U32 dwChannel; // 以1开始
	U32 dwStreamIdx;
	U32 dwStatus; // 0 -正常,1-超时,2-有错
	U32 dwType;// 0- Alarm(多张)，1-定时(1张),2-实时抓图(1张)
	U32 dwCapIdx; // 一次触发的计数,0开始
	AntsHostMgrLibTime_T tTigerTime;
	AntsHostMgrLibTime_T tCapTime;
	U32 dwFileSize;
	S8 sFileName[260];// 路径名
}AntsHostCapturePictureInfo_T;

typedef struct _tagAntsHostCapturePictureCfg
{	
	U32 dwType;// 0- Alarm(多张)，1-定时(1张),2-实时抓图(1张)
	U32 dwPreTimeSec; //  前多少秒
	U32 dwIntervalTimeMSec; // 间隔,最小1秒,单位是毫秒
	U32 dwTotalTimeSec; // 总多少秒dwTotalTimeSec >= dwPreTimeSec
	U32 dwDefTime;//定时: 时分秒 h<< 16 |m<<8|s,目前精确到分钟
	U32 dwRes[3];
}AntsHostCapturePictureCfg_T;

//查询相关结构
typedef struct _tagAntsHostMgrLibQueryFile
{
	S32 lFileType;//见AntsHostMgrLibRecordType_E
	AntsHostMgrLibTime_T tStartTime;
	AntsHostMgrLibTime_T tStopTime;
}AntsHostMgrLibQueryFile_T;

typedef struct _tagAntsHostMgrLibQueryFileResult
{
	S32 lChannel; // 1,2,3...
	S32 lFileType;
	S32 lResultType; // 1- 文件名,2-时间
	union{
		S8 szFileName[64];
		struct{
			AntsHostMgrLibTime_T tStartTime;
			AntsHostMgrLibTime_T tStopTime;
		}tTime;
	}uGoup;
	U64 llSize;// bytes
}AntsHostMgrLibQueryFileResult_T;


// 语音对讲
typedef struct _tagAntsHostMgrLibTalkHeader
{// 无效时缺省值
	U8 dwType;// 0-无效; 1-G711u;2- G711.a;2-G726;
	U8	dwSampleBitsWidth; //8 or 16, 0-无效(16)
	U8	dwChannels; //0:无效(mono);1: mono, 2: stero
	U8	dwFrameRate;//0-无效(25)
	U32	dwSampleRate;//0-无效(8000)
	U32	dwBitRate;//0-无效
	U32 dwPayloadLength;
	U64 llTimeStamp;//0-无效
}AntsHostMgrLibTalkHeader_T;


typedef struct _tagAntsHostMgrLibTalkParam
{
	U8 dwType;// 0- G711.u;1-G711.a;2-G.726.3;
	U8	dwSampleBitsWidth; //0无效，8 or 16
	U8	dwChannels; //0:无效(mono);1: mono, 2: stero
	U8	dwFrameRate;//0:无效(25)
	U32	dwSampleRate;// 0无效(8000)
	U32	dwBitRate;//0无效
}AntsHostMgrLibTalkParam_T;

//报警结构
typedef struct _tagAntsHostMgrLibStatusAlarm
{
	U32 dwAlarmType; // AntsHostMgrAlarmType_E
	U32 dwAlarmParam[32];// dwAlarmParam[0]：源通道,dwAlarmParam[1]:是否警报(0/1)
}AntsHostMgrLibStatusAlarm_T;


//PTZ控制
// ->AntsHostMgrControlCmd_PTZ
typedef struct _tagAntsHostMgrLibPTZControl
{
	U32 dwCmd;
	U32 dwParams[6]; // 0- 无效值
	// 方向控制:dwParams[0] :1-停止;dwParams[1]:速度
	//预置点:dwParams[0]:预置点
	//巡航:dwParams[0]:巡航序号，dwParams[1]巡航点，dwParams[2]输入参数
	//轨迹:dwParams[0]:轨迹号0/1均为第一条轨迹，2-第二第轨迹，3-第三条轨迹...
	//辅助 电源:dwParams[0],1-开，2-关
	// 获取巡航点: dwParams[0] 巡航序号
	// Set3D:dwParams[0]   -XPoint, dwParams[1]-YPoint, dwParams[2]-Scale
}AntsHostMgrLibPTZControl_T;

typedef struct _tagAntsHostMgrLibPTZCruisePoint
{
	U8	PresetNum;
	U8	Dwell;
	U8	Speed;
	U8	Reserve;
}AntsHostMgrLibPTZCruisePoint_T;

typedef struct _tagAntsHostMgrLibPTZCruiseSets
{
	AntsHostMgrLibPTZCruisePoint_T		struCruisePoint[32];
}AntsHostMgrLibPTZCruiseSets_T;


typedef struct _tagAntsHostMgrLibRemoteHistoryPlayControlParam
{
	U32 dwControlCmd;// AntsHostMgrHistoryPlayCmd_E
	S32 lHistoryHandle;
	U32 uParams[6];
	AntsHostMgrLibTime_T tPlayTime;// 输入，输出
}AntsHostMgrLibRemoteHistoryPlayControlParam_T;

typedef struct _tagAntsHostMgrLibRemoteRecordControlParam
{
	U32 dwCmd;// 0-无效，1-时间计划录像，2-开关录像功能，3-手动录像
	U32 uParams[6]; // 0- 无效值,uParams[0]:1-关闭，2-开启;
}AntsHostMgrLibRemoteRecordControlParam_T;


//获取升级路径
typedef struct _tagAntsHostMgrLibUpdatePath
{
	S8    sPath[256]; // 完整绝对路径，包括文件名
	U32   dwFreeSize; // 当前可用空间；仅获取瞬间有效。字节单位
	U32   dwRes[5];
}AntsHostMgrLibUpdatePath_T;



//搜索相关
// IPV6/域名支持,可用于搜索结果或者通道设备配置
typedef struct _tagAntsHostMgrLibSearchDeviceInfo
{
	U32		dwSize;
	U32		dwProtocolType;//协议类型(即协议ID)
	S8        szProtocolName[16];// 协议名(希望仅做显示用,对内部来说，它不是重要的)
	U8        byDeviceType;//0-未定义的 (可默认为IPC),  1-dvr,2-nvr,3-ipc,4-dec,5-DES,6-DEM
	U8	    byEnableQuickAdd;// 支持自动配置按位,
	                             // bit0-IP是否可修改的,
	                             // bit1-修改需要认证,
	                             // bit2-OSD是否可以修改
	U8        byMac[6];// MAC ,6 bytes HEX
	U32		dwChanNum; // 设备的通道数0-不定，可认为是一个通道;
	
	U8		byTransMode;   //可用于指定默认模式：0-主码流模式，1-子码流模式
	U8		byLinkProtocol;	//可用于指定默认模式: 0-TCP;1-UDP;2-多播
	U8		byEXMode;		//可用于指定默认模式:是否启用流模式 1:开启
	U8		byRTSPMode; // 0: 一般模式;1:该协议同RTSP协议,szDomainAux有效,配置或PTZ都走RTSP流方式。
	U16		wChannel; // 使用的设备通道,默认是第一通道
	
	U16		wLinkAblility; // 连接支持能力,支持或操作:(0-TCP + UDP ,)1-TCP,2-UDP,...
	
	U32		dwVideoPort;//!协议端口
	S8	    szDomain[128];// 域名/IPv4/IPv6，登陆访问用,当为IP时应跟szIP同
	S8	    szDomainAux[128];// 域名/IPv4/IPv6 当RTSP时有效
	S8        szIP[40]; // 设备IP,可用于修改IP
	U8        byNetMask;// 24->255.255.255.0;16->255.255.0.0
	U8        byPhyIdx; //0-自动, 1- eth0,2-eth1
	U8        byProp;// 保留0
	U8        byConnNum;// 连接数,0- NA/0连接，1-1连接，2-2连接，3-3连接
	S8        szGateway[40];
	S8        szDns1[40];
	S8        szDns2[40];
	S8		szDeviceName[64 + 1];
	S8        szUserName[64 + 1];
	S8        szPassword[64 + 1];
	S8		szDescription[64 + 1];
	U8		byRes2[128];
}AntsHostMgrLibSearchDeviceInfo_T;





// IPC协议管理

typedef struct
{
  U32   dwType;
  U8    byDescribe[16];
  U8    byEnable; // 当前是否启用
  U8    byRes[15];
}AntsHostMgrLibIPCProtoType_T;

//平台协议管理

typedef struct 
{
	S32 nItemCount;
	U32 dwType;// 类型号
	U32 dwSupportHostCount;// 支持同时注册主机数
	S8 szProtocolName[32];// 类型名
	S8 szItemLables[8][32];// 参数显示名
	
}AntsHostMgrLibManagerHostProtocol_T;


//!3G上网卡类型及描述
typedef struct {
	U32 dwType;//!3G类型值
	U8 byDescribe[32];
	U8 byISPDescribe[16];
}AntsHostMgrLib3GDevice_T;

//!DDNS 服务名称
typedef struct {
	U32 dwIndex;//!DDNS索引值
	U8 byDescribe[32];
	U8 byServerName[64];
	U16 wServerPort;
	U16 wRes;
}AntsHostMgrLibDDNSService_T;

typedef struct _tagAntsHostMgrLibALarmStatusInfo
{
	U8 byAlarmType;// 
	/*0-信号量报警,
	  1-硬盘满,
	  2-信号丢失,
	  3－移动侦测,
	  4－硬盘未格式化,
	  5-读写硬盘出错,
	  6-遮挡报警,
	  7-制式不匹配, 
	  8-非法访问, 
	  9-视频信号异常，
	  10-录像异常
	  11-网线断
	  12-IP冲突*/
	U8 byAlarmStatus; // 1-有报警,0-无报警
	U16 wAlarmSource; // 报警源,0-无源,起始通道1开始
	
	AntsHostMgrLibTimeV2_T tStartTime;
	AntsHostMgrLibTimeV2_T tStopTime;
	U32 dwRes[3];
}AntsHostMgrLibALarmStatusInfo_T;





typedef struct _tagANTS_DVR_CONFIG_RECOVERY
{
	DWORD bNetwork:1; // 恢复网络配置
	DWORD bIPChannel:1;
	DWORD bAlarm:1;
	DWORD bAccount:1;//用户管理
	DWORD bDisplaySet:1; // 显示设置（语言,分辨率）
	DWORD bResolveMode:1;// 设备支持的分辨率模式
	DWORD bRecordCfg:1; // 录像计划
	DWORD bOtherCfg:1; // 其他配置
	DWORD dwRes:24;
	DWORD dwRes1[3];
}ANTS_DVR_CONFIG_RECOVERY_T;



typedef struct tagANTSMID_HDISK_SMART_VALUE
{
  BYTE byID;
  // meaning of flag bits: see MACROS just below
  // WARNING: MISALIGNED!
  WORD wFlags; 
  /*
  bit0 -> bit5
  ||||||_ K auto-keep                            
  |||||__ C event count                            
  ||||___ R error rate                           
  |||____ S speed/performance                           
  ||_____ O updated online                          
  |______ P prefailure warning

  bit0->P


  严重情况从上到下 :
1. 最严重的情况WHEN_FAILED = FAILING_NOW 并且 TYPE=Pre-failed, 表示现在这个属性已经出问题
了. 并且硬盘也已经failed了.
2. 次严重的情况WHEN_FAILED = in_the_past 并且 TYPE=Pre-failed, 表示这个属性曾经出问题了. 但是现
在是正常的.
3. WHEN_FAILED = FAILING_NOW 并且 TYPE=Old_age, 表示现在这个属性已经出问题了. 但是硬盘可能
还没有failed.
4. WHEN_FAILED = in_the_past 并且 TYPE=Old_age, 表示现在这个属性曾经出问题了. 但是现在是正常
的.
为了避免这4种情况的发生.
1. 对于UPDATE=Offline的属性, 应该让smartd定期进行测试(smartd还可以发邮件). 或者crontab进行测试.
2. 应该时刻关注磁盘的Normalized value以及WORST的值是否接近THRESH的值了. 当有值要接近
THRESH了, 提前更换硬盘.
3. 温度, 有些磁盘对温度比较敏感, 例如PCI-E SSD硬盘. 如果温度过高可能就挂了. 这里读取RAW_VALUE
就比较可靠了.

  */
  BYTE byCurrent;
  
  BYTE byWorst;
  BYTE byThreshold;
  BYTE byRaw[6];
  BYTE byStatus;// 0- OK,1-Failed,2-Warning
  
  BYTE byReserv[19];
}ANTSMID_HDISK_SMART_VALUE_T;

#define ANTSMID_HDISK_SMART_VALUE_MAX_NUM 30

typedef struct tagANTSMID_HDISK_SMART_ATTR
{
	DWORD  dwValueNum;
	ANTSMID_HDISK_SMART_VALUE_T tValue[ANTSMID_HDISK_SMART_VALUE_MAX_NUM];
	DWORD dwRes[8];
}ANTSMID_HDISK_SMART_ATTR_T;


typedef struct
{
	U32	    bLocal;
	S8		szUserName[32];
	AntsHostMgrLibTimeV2_T tLoginTime;
	S8		szLoginIP[40]; // 兼容IPv6
	S8      szRes[80];
}AntsHostMgrLibOnlineUserInfo_T;



typedef enum
{
	AntsHostMgrLibLogMajorType_Alarm = 1,/* 报警 */
	AntsHostMgrLibLogMajorType_Exception = 2,/* 异常 */
	AntsHostMgrLibLogMajorType_Operation = 3,/*操作 */
	AntsHostMgrLibLogMajorType_Setting = 4,/*设置*/
}AntsHostMgrLibLogMajorType_E;

typedef enum
{
	/* 报警 */

	AntsHostMgrLibLogMinorType_AlarmIn = 1,/* 报警输入 */
	AntsHostMgrLibLogMinorType_AlarmOut = 2,	/* 报警输出 */
	AntsHostMgrLibLogMinorType_Alarm_MotionStart = 3,/* 移动侦测报警开始 */
	AntsHostMgrLibLogMinorType_Alarm_MotionStop = 4,	/* 移动侦测报警结束 */
	AntsHostMgrLibLogMinorType_Alarm_HideAlarmStart = 5,	/* 遮挡报警开始 */
	AntsHostMgrLibLogMinorType_Alarm_HideAlarmStop = 6,/* 遮挡报警结束 */
		/* 异常 */

	AntsHostMgrLibLogMinorType_Exception_VideoLost = 0x21,	/* 视频信号丢失 */
	AntsHostMgrLibLogMinorType_Exception_IllegalAccess = 0x22,	/* 非法访问 */
	AntsHostMgrLibLogMinorType_Exception_HDFull = 0x23,	/* 硬盘满 */
	AntsHostMgrLibLogMinorType_Exception_HDError = 0x24,	/* 硬盘错误 */
	AntsHostMgrLibLogMinorType_Exception_IPConflict = 0x26,	/* IP地址冲突 */
	AntsHostMgrLibLogMinorType_Exception_NetBroken = 0x27,	/* 网络断开*/
	AntsHostMgrLibLogMinorType_Exception_RecError = 0x28,    /* 录像出错 */
	AntsHostMgrLibLogMinorType_Exception_VideoInput = 0x2a,    /* 视频输入异常(只针对模拟通道) */

	AntsHostMgrLibLogMinorType_Operation_Boot = 0x41,	/* 开机 */
	AntsHostMgrLibLogMinorType_Operation_Halt = 0x42,	/* 关机 */
	AntsHostMgrLibLogMinorType_Operation_Abnormal = 0x43,	/* 异常关机 ,如段错误*/
	AntsHostMgrLibLogMinorType_Operation_Reboot = 0x44,   /* 本地重启设备 */
	AntsHostMgrLibLogMinorType_Operation_Illegal = 0x45,    /* 非法关机,如关掉电源*/


	AntsHostMgrLibLogMinorType_Operation_Local_EnterCfgParam =0x49,	/* 进入配置参数 */

	AntsHostMgrLibLogMinorType_Operation_Local_Login =0x50,	/* 本地登陆 */
	AntsHostMgrLibLogMinorType_Operation_Local_Logout =0x51,	/* 本地注销登陆 */
	AntsHostMgrLibLogMinorType_Operation_Local_CfgParam =0x52,	/* 本地配置参数 */
	AntsHostMgrLibLogMinorType_Operation_Local_PlayByFile =0x53,	/* 本地按文件回放或下载 */
	AntsHostMgrLibLogMinorType_Operation_Local_PlayByTime =0x54,	/* 本地按时间回放或下载*/
	AntsHostMgrLibLogMinorType_Operation_Local_StartRec =0x55,	/* 本地开始录像 */
	AntsHostMgrLibLogMinorType_Operation_Local_StopRec =0x56,	/* 本地停止录像 */
	AntsHostMgrLibLogMinorType_Operation_Local_PTZControl =0x57,	/* 本地云台控制 */
	AntsHostMgrLibLogMinorType_Operation_Local_Preview =0x58,	/* 本地预览 (保留不使用)*/
	AntsHostMgrLibLogMinorType_Operation_Local_ModifyTime =0x59,	/* 本地修改时间(保留不使用) */
	AntsHostMgrLibLogMinorType_Operation_Local_Update =0x5a,	/* 本地升级 */
	AntsHostMgrLibLogMinorType_Operation_Local_BackupRec =0x5b,    /* 本地备份录象文件 */
	AntsHostMgrLibLogMinorType_Operation_Local_FormatHDD =0x5c,    /* 本地初始化硬盘 */
	AntsHostMgrLibLogMinorType_Operation_Local_ExportCfgFile =0x5d,    /* 导出本地配置文件 */
	AntsHostMgrLibLogMinorType_Operation_Local_ImportCfgFile =0x5e,    /* 导入本地配置文件 */
	AntsHostMgrLibLogMinorType_Operation_Local_BackupFile =0x5f,    /* 本地备份文件 */
	AntsHostMgrLibLogMinorType_Operation_Local_LockRecFile =0x60,    /* 本地锁定录像文件 */
	AntsHostMgrLibLogMinorType_Operation_Local_UnlockRecFile =0x61,    /* 本地解锁录像文件 */
	AntsHostMgrLibLogMinorType_Operation_Local_ManualAlarm =0x62,    /* 本地手动清除和触发报警*/
	AntsHostMgrLibLogMinorType_Operation_Local_AddIPC =0x63,    /* 本地添加IPC */
	AntsHostMgrLibLogMinorType_Operation_Local_DelIPC =0x64,    /* 本地删除IPC */
	AntsHostMgrLibLogMinorType_Operation_Local_SetIPC =0x65,    /* 本地设置IPC */
	AntsHostMgrLibLogMinorType_Operation_Local_StartBackup =0x66,	/* 本地开始备份 */
	AntsHostMgrLibLogMinorType_Operation_Local_StopBackup =0x67,	/* 本地停止备份*/
	AntsHostMgrLibLogMinorType_Operation_Local_StartBackupTime =0x68,	/* 本地备份开始时间*/
	AntsHostMgrLibLogMinorType_Operation_Local_StopBackupTime =0x69,	/* 本地备份结束时间*/
	AntsHostMgrLibLogMinorType_Operation_Local_AddNAS =0x6a,	/* 本地添加网络硬盘 （nfs、iscsi）*/
	AntsHostMgrLibLogMinorType_Operation_Local_DelNAS =0x6b,	/* 本地删除nas盘 （nfs、iscsi）*/
	AntsHostMgrLibLogMinorType_Operation_Local_SetNAS =0x6c,	/* 本地设置nas盘 （nfs、iscsi）*/

	AntsHostMgrLibLogMinorType_Operation_Remote_Login =0x70,	/* 远程登录 */
	AntsHostMgrLibLogMinorType_Operation_Remote_Logout =0x71,	/* 远程注销登陆 */
	AntsHostMgrLibLogMinorType_Operation_Remote_StartRec =0x72,	/* 远程开始录像 */
	AntsHostMgrLibLogMinorType_Operation_Remote_StopRec =0x73,	/* 远程停止录像 */
	AntsHostMgrLibLogMinorType_Operation_Remote_StartTrans =0x74,	/* 开始透明传输 */
	AntsHostMgrLibLogMinorType_Operation_Remote_StopTrans =0x75,	/* 停止透明传输 */
	AntsHostMgrLibLogMinorType_Operation_Remote_GetCfgParam =0x76,	/* 远程获取参数 */
	AntsHostMgrLibLogMinorType_Operation_Remote_SetCfgParam =0x77,	/* 远程配置参数 */
	AntsHostMgrLibLogMinorType_Operation_Remote_GetStatus =0x78,	/* 远程获取状态 */
	AntsHostMgrLibLogMinorType_Operation_Remote_AlarmDefenceOn =0x79,	/* 远程布防 */
	AntsHostMgrLibLogMinorType_Operation_Remote_AlarmDefenceOff =0x7a,	/* 远程撤防 */
	AntsHostMgrLibLogMinorType_Operation_Remote_Reboot =0x7b,	/* 远程重启 */
	AntsHostMgrLibLogMinorType_Operation_Remote_StartVoiceTalk =0x7c,	/* 开始语音对讲 */
	AntsHostMgrLibLogMinorType_Operation_Remote_StopVoiceTalk =0x7d,	/* 停止语音对讲 */
	AntsHostMgrLibLogMinorType_Operation_Remote_Update =0x7e,	/* 远程升级 */
	AntsHostMgrLibLogMinorType_Operation_Remote_PlayByFile =0x7f,	/* 远程按文件回放 */
	AntsHostMgrLibLogMinorType_Operation_Remote_PlayByTime =0x80,	/* 远程按时间回放 */
	AntsHostMgrLibLogMinorType_Operation_Remote_PTZControl =0x81,	/* 远程云台控制 */
	AntsHostMgrLibLogMinorType_Operation_Remote_FormatHDD =0x82,    /* 远程格式化硬盘 */
	AntsHostMgrLibLogMinorType_Operation_Remote_Halt =0x83,    /* 远程关机 */
	AntsHostMgrLibLogMinorType_Operation_Remote_LockFile =0x84,	/* 远程锁定文件 */
	AntsHostMgrLibLogMinorType_Operation_Remote_UnlockFile =0x85,	/* 远程解锁文件 */
	AntsHostMgrLibLogMinorType_Operation_Remote_ExportCfgFile =0x86,    /* 远程导出配置文件 */
	AntsHostMgrLibLogMinorType_Operation_Remote_ImportCfgFile =0x87,    /* 远程导入配置文件 */
	AntsHostMgrLibLogMinorType_Operation_Remote_ExportRecFile =0x88,    /* 远程导出录象文件 */
	AntsHostMgrLibLogMinorType_Operation_Remote_ManualAlarm =0x89,    /* 远程手动清除和触发报警*/
	AntsHostMgrLibLogMinorType_Operation_Remote_AddIPC =0x8a,	/* 远程添加IPC */
	AntsHostMgrLibLogMinorType_Operation_Remote_DelIPC =0x8b,	/* 远程删除IPC */
	AntsHostMgrLibLogMinorType_Operation_Remote_SetIPC =0x8c,	/* 远程设置IPC */
	AntsHostMgrLibLogMinorType_Operation_Remote_ReStartSmartLib =0x8d,    /*重启智能库*/
	AntsHostMgrLibLogMinorType_Operation_Remote_AddNAS =0x8e,   /* 远程添加nas盘 （nfs、iscsi）*/
	AntsHostMgrLibLogMinorType_Operation_Remote_DelNAS =0x8f,   /* 远程删除nas盘 （nfs、iscsi）*/
	AntsHostMgrLibLogMinorType_Operation_Remote_SetNAS =0x90,   /* 远程设置nas盘 （nfs、iscsi）*/
}AntsHostMgrLibLogMinorType_E;


typedef struct tag_AntsHostMgrLibLog
{
	U32						dwLogTime;
	U32						dwMajorType;					//!主类型 1-报警; 2-异常; 3-操作; 0-全部
	U32						dwMinorType;					//!次类型 0-全部;
	U8						sUser[16];				//!用户名
	U8						sRemoteHostAddr[16];	//!远程主机地址
	U32						dwChannel;						//!通道号
	U32						dwDiskNum;						//!硬盘号
	U32						dwAlarmInPort;					//!报警输入端口
	U32						dwAlarmOutPort;					//!报警输出端口
	U32						bStatus;						//!报警状态
}AntsHostMgrLibLog;


typedef struct
{
	AntsHostMgrLibTimeV2_T		tLogTime;
	U32						dwMajorType;					//!主类型
	U32						dwMinorType;					//!次类型
	S8						szPanelUser[32];		//!操作面板的用户名
	S8						szNetUser[32];			//!网络操作的用户名
	S8				        szRemoteHostAddr[40];				//!远程主机地址
	U32						dwParaType;						//!参数类型,9000设备MINOR_START_VT/MINOR_STOP_VT时，表示语音对讲的端子号
	U32						dwChannel;						//!通道号
	U32						dwDiskNumber;					//!硬盘号
	U32						dwAlarmInPort;					//!报警输入端口
	U32						dwAlarmOutPort;					//!报警输出端口
	U32						dwInfoLen;
	S8						sInfo[4];
}AntsHostMgrLibLogResult_T;


typedef struct _tagAntsHostMgrLibQRCode
{ // 整个点阵尺寸为(wWidth + byMargin * 2) x (wWidth + byMargin * 2);
  // wStride = ((wWidth + byMargin * 2 + 7) >>3)<<3;
	U16 wWidth; // 二维码宽度，不包括边框，像素[输出]
	U16 wStride; // 宽度stride,包括边框，像素，字节对齐[输出]
	U8 byPixelWide;// 每个点宽度占几个像素,0-1个,1-1个，2-2个，3-3个，相当于放大[输入/输出]
	U8 byMargin; // 二维码边框宽度,像素,0-无边框[输入/输出]
	U8 byBitsOrder;// 0-字节从高到低，1-字节从低到高. [输入/输出]
	U8 byPicType; // 0- 点阵;1-BMP-bit1;
	U8 *pPixelData; // 需要用free 释放[输出] ，使用完后，一定要调用标准C的free释放内存，否则内存泄露.
	U32 dwDataSize; // 数据长度[输出]
	U32 dwRes[4];
}AntsHostMgrLibQRCode_T;




//DVR设备参数
typedef struct
{
	DWORD						dwSize;
	BYTE						sDVRName[NAME_LEN];     //DVR名称
	DWORD						dwDVRID;				//DVR ID,用于遥控器
	BYTE						dwRecycleRecord;		//是否循环录像,0:不是; 1:是
	BYTE                        byRes;    // 
	WORD						wRecycleHour;			// 小时为单位0-不启用，
	
		

	//以下不可设置
	BYTE						sSerialNumber[SERIALNO_LEN];  //序列号
	DWORD						dwSoftwareVersion;		//软件版本号,高16位是主版本,低16位是次版本
	DWORD						dwSoftwareBuildDate;	//软件生成日期,0xYYYYMMDD
	DWORD						dwDSPSoftwareVersion;	//DSP软件版本,高16位是主版本,低16位是次版本
	DWORD						dwDSPSoftwareBuildDate;	// DSP软件生成日期,0xYYYYMMDD
	DWORD						dwPanelVersion;			// 前面板版本,高16位是主版本,低16位是次版本
	DWORD						dwHardwareVersion;		//硬件版本,高16位是主版本,低16位是次版本
	BYTE						byAlarmInPortNum;		//DVR报警输入个数
	BYTE						byAlarmOutPortNum;		//DVR报警输出个数
	BYTE						byRS232Num;				//DVR 232串口个数
	BYTE						byRS485Num;				//DVR 485串口个数
	BYTE						byNetworkPortNum;		//网络口个数
	BYTE						byDiskCtrlNum;			//DVR 硬盘控制器个数
	BYTE						byDiskNum;				//DVR 硬盘个数
	BYTE						byDVRType;				//DVR类型, 1:DVR 2:NVR 3:DVS/IPC,4-DEC ......
	BYTE						byChanNum;				//DVR 通道个数
	BYTE						byStartChan;			//起始通道号,例如DVS-1,DVR - 1
	BYTE						byDecodeChans;			//DVR 解码路数
	BYTE						byVGANum;				//VGA口的个数
	BYTE						byUSBNum;				//USB口的个数
    BYTE						byAuxoutNum;			//辅口的个数
    BYTE						byAudioNum;				//语音口的个数
    BYTE						byIPChanNum;			//最大数字通道数
}ANTS_DVR_DEVICECFG, *LPANTS_DVR_DEVICECFG;

//DVR设备参数
typedef struct
{
	DWORD						dwSize;
	BYTE						sDVRName[NAME_LEN];     //DVR名称
	DWORD						dwDVRID;				//DVR ID,用于遥控器
	BYTE						dwRecycleRecord;		//是否循环录像,0:不是; 1:是
	BYTE                        byPreviewDecodeChans;    // 支持的解码通道个数(NVR 预览最大支持的解码数路)
	WORD						wRecycleHour;			// 小时为单位0-不启用，

	//以下不可设置
	BYTE						sSerialNumber[SERIALNO_LEN];  //序列号
	DWORD						dwSoftwareVersion;		//软件版本号,高16位是主版本,低16位是次版本
	DWORD						dwSoftwareBuildDate;	//软件生成日期,0xYYYYMMDD
	DWORD						dwDSPSoftwareVersion;	//DSP软件版本,高16位是主版本,低16位是次版本
	DWORD						dwDSPSoftwareBuildDate;	// DSP软件生成日期,0xYYYYMMDD
	DWORD						dwPanelVersion;			// 前面板版本,高16位是主版本,低16位是次版本
	DWORD						dwHardwareVersion;		//硬件版本,高16位是主版本,低16位是次版本
	BYTE						byAlarmInPortNum;		//DVR报警输入个数
	BYTE						byAlarmOutPortNum;		//DVR报警输出个数
	BYTE						byRS232Num;				//DVR 232串口个数
	BYTE						byRS485Num;				//DVR 485串口个数
	BYTE						byNetworkPortNum;		//网络口个数
	BYTE						byDiskCtrlNum;			//DVR 硬盘控制器个数
	BYTE						byDiskNum;				//DVR 硬盘个数
	BYTE						byDVRType;				//DVR类型, 1:DVR 2:NVR 3:DVS/IPC,4-DEC,5-DEC_SLAVE,6-DEC_MASTER......
	BYTE						byChanNum;				//DVR 通道个数
	BYTE						byStartChan;			//起始通道号,例如DVS-1,DVR - 1
	BYTE						byDecodeChans;			//DVR 解码路数(回放路数)
	BYTE						byVGANum;				//VGA口的个数
	BYTE						byUSBNum;				//USB口的个数
    BYTE						byAuxoutNum;			//辅口的个数
    BYTE						byAudioNum;				//语音口的个数
    BYTE						byIPChanNum;			//最大数字通道数

    BYTE                        byAudioSource;      // 音频源,0-LineIn(线入,默认), 1-MicIn(麦克风).
    BYTE                        byEnableAuxRecord;      // 子码流是否录像,1-录像，0-不录像
    BYTE                    	bI8_ServerValid:1;       // bit0:I8有效,服务状态未启用，则按老版本处理
	BYTE                    	bI8_ServerStatus:1;       //  bit1: I8 服务。状态启用时，服务状态有效,1-启用
	BYTE                    	bOnvif_ServerValid:1;       // bit2:ONVIF 服务
	BYTE                    	bOnvif_ServerStatus:1;       // bit3:ONVIF服务
	BYTE                    	bRTSP_ServerValid:1;       // bit4:RTSP 服务
	BYTE                    	bRTSP_ServerStatus:1;       // bit5:RTSP服务
	BYTE                    	bRTMP_ServerValid:1;       // bit6:RTMP 服务
	BYTE                    	bRTMP_ServerStatus:1;       // bit7:RTMP服务
	BYTE                    	bI8S_ServerValid:1;       // bit8:I8S 服务
	BYTE                    	bI8S_ServerStatus:1;       // bit9:I8S服务
	BYTE                    	bGB28181_ServerValid:1;       // bit10:GB28181 服务
	BYTE                    	bGB28181_ServerStatus:1;       // bit11:GB28181服务
	BYTE                    	bWebApi_ServerValid:1;       // bit11:GB28181 服务
	BYTE                    	bWebApi_ServerStatus:1;       // bit12:GB28181服务
	BYTE                    	bRes:2;       // 保留
    BYTE                        byReserved[4];
	BYTE                        byAudioInVol; // 输入音量(MIC或LineIn).取值0~100.默认值为50.
	BYTE                        byAudioOutVol; // 输出音量.取值0~100.默认值为50.
    BYTE                        byReserved2[22];

}ANTS_DVR_DEVICECFG_V2, *LPANTS_DVR_DEVICECFG_V2;

typedef struct
{		
	char						sIpV4[16];				/* IPv4地址 */
	BYTE						byIPv6[128];			/* 保留 */
}ANTS_DVR_IPADDR, *LPANTS_DVR_IPADDR;

//pppoe结构
typedef struct 
{
	DWORD						dwPPPOE;						//0-不启用,1-启用
	BYTE						sPPPoEUser[NAME_LEN];			//PPPoE用户名
	char						sPPPoEPassword[PASSWD_LEN];		// PPPoE密码
	ANTS_DVR_IPADDR				struPPPoEIP;					//PPPoE IP地址
}ANTS_DVR_PPPOECFG, *LPANTS_DVR_PPPOECFG;

/*网络数据结构(子结构)*/
typedef struct 
{
	ANTS_DVR_IPADDR				struDVRIP;          			//DVR IP地址
	ANTS_DVR_IPADDR				struDVRIPMask;					//DVR IP地址掩码
	DWORD						dwNetInterface;					//网络接口1-10MBase-T 2-10MBase-T全双工 3-100MBase-TX 4-100M全双工 5-10M/100M自适应
	WORD						wDVRPort;						//端口号
	WORD						wMTU;							//增加MTU设置，默认1500。
	BYTE						byMACAddr[MACADDR_LEN];			// 物理地址
	BYTE						byRes[2];						//保留对齐
}ANTS_DVR_ETHERNET, *LPANTS_DVR_ETHERNET;

//网络配置结构
typedef struct
{
	DWORD						dwSize;
	ANTS_DVR_ETHERNET			struEtherNet[MAX_ETHERNET];		//以太网口
	ANTS_DVR_IPADDR				struRes1[2];					/*保留*/
	ANTS_DVR_IPADDR				struAlarmHostIpAddr;			/* 报警主机IP地址 */
	WORD                        wHttpsPort;                     /* HTTPS 端口*/
	WORD						wMulticastPort;						/*多播端口 */
	WORD						wAlarmHostIpPort;				/* 报警主机端口号 */
	BYTE						byUseDhcp;						/* 是否启用DHCP 0xff-无效 0-不启用 1-启用*/
	BYTE						byUPNP;                         // byUPNP;                 //! 1-开启UPNP，0-关闭UPNP
	ANTS_DVR_IPADDR				struDnsServer1IpAddr;			/* 域名服务器1的IP地址 */
	ANTS_DVR_IPADDR				struDnsServer2IpAddr;			/* 域名服务器2的IP地址 */
	BYTE						byIpResolver[MAX_DOMAIN_NAME];	/* IP解析服务器域名或IP地址 */
	WORD						wIpResolverPort;				/* IP解析服务器端口号 */
	WORD						wHttpPortNo;					/* HTTP端口号 */
	ANTS_DVR_IPADDR				struMulticastIpAddr;			/* 多播组地址 */
	ANTS_DVR_IPADDR				struGatewayIpAddr;				/* 网关地址 */
	ANTS_DVR_PPPOECFG			struPPPoE;	
	char						szManagerHostIpV4[32];			/*主动注册服务器IP地址0-不启用1-启用*/
	WORD						wManagerHostPort;				/*主动注册服务器端口*/
	BYTE						byUseManagerHost;				/*是否启用主动注册服务0-不启用1-启用*/
	BYTE						byOnvif;				// ! 1- 开启ONVIF，0-关闭ONVIF
	WORD                        wOnvifPort;             // ! ONVIF端口
	WORD                        wRTMPPort;             // ! RTMP端口
    BYTE                        byRTMP;                 // ! 1-开启RTMP，0-关闭RTMP
    BYTE                        byEnableMulticast; //! 1-开启组播 0-关闭组播
    BYTE                        byWebPortMultiplex; //! 0-不复用 1-端口复用
    BYTE                        byautoadapterip;
	BYTE						byRes[20];
} ANTS_DVR_NETCFG, *LPANTS_DVR_NETCFG;
// 多网卡
typedef struct
{
	ANTS_DVR_IPADDR				struDVRIP;						//!DVR IP地址
	ANTS_DVR_IPADDR				struDVRIPMask;					//!DVR IP地址掩码
	DWORD						dwNetInterface;					//!网络接口1-10MBase-T 2-10MBase-T全双工 3-100MBase-TX 4-100M全双工 5-10M/100M自适应
	BYTE						byPhyIdx;                       //! 0:eth0; 1-eth1;2-eth2...
	BYTE						byRes1;
	WORD						wMTU;							//!增加MTU设置，默认1500。
	BYTE						byMACAddr[MACADDR_LEN];			//!物理地址，只用于显示
	BYTE						byRes2[2];						//!保留
	BYTE						byUseDhcp;						//!是否启用DHCP 
	BYTE						byRes3[3];
	ANTS_DVR_IPADDR				struGatewayIpAddr;				//!网关地址 
	ANTS_DVR_IPADDR				struDnsServer1IpAddr;			//!域名服务器1的IP地址 
	ANTS_DVR_IPADDR				struDnsServer2IpAddr;			//!域名服务器2的IP地址 
}ANTS_DVR_ETHERNET_MULTI,*LPANTS_DVR_ETHERNET_MULTI;


typedef struct
{
	DWORD						dwSize;
	BYTE						byDefaultRoute;					//!默认路由，0表示struEtherNet[0]，1表示struEtherNet[1]
	BYTE						byNetworkCardNum;				//!设备实际可配置的网卡数目
	BYTE 						byUPNP;                 //! 1-开启UPNP，0-关闭UPNP
	BYTE						byOnvif;				// ! 1- 开启ONVIF，0-关闭ONVIF
	ANTS_DVR_ETHERNET_MULTI		struEtherNet[MAX_NETWORK_CARD];	//!以太网口
	ANTS_DVR_IPADDR				struManageHost1IpAddr;			//!主管理主机IP地址 
	ANTS_DVR_IPADDR				struManageHost2IpAddr;			//!辅管理主机IP地址 
	ANTS_DVR_IPADDR				struAlarmHostIpAddr;			//!报警主机IP地址 
	WORD						wManageHost1Port;				//!主管理主机端口号 
	WORD						wManageHost2Port;				//!辅管理主机端口号 
	WORD						wAlarmHostIpPort;				//!报警主机端口号 
	BYTE						byUseManagerHost1;				/*是否启用主动注册服务0-不启用1-启用*/
	BYTE						byUseManagerHost2;				/*是否启用主动注册服务0-不启用1-启用*/
	BYTE						byIpResolver[MAX_DOMAIN_NAME];	//!IP解析服务器域名或IP地址 
	WORD						wIpResolverPort;				//!IP解析服务器端口号 
	WORD						wDvrPort;						//!通讯端口 默认8000 
	WORD						wHttpPortNo;					//!HTTP端口号 
	WORD                        wHttpsPort;                   //! HTTPS端口
	WORD                        wMulticastPort;               //! 多播端口
	WORD                        wOnvifPort;             // ! ONVIF端口
	ANTS_DVR_IPADDR				struMulticastIpAddr;			//!多播组地址
	ANTS_DVR_PPPOECFG			struPPPoE;
	WORD                        wRTMPPort;             // ! RTMP端口
	BYTE                        byRTMP;                 // ! 1-开启RTMP，0-关闭RTMP
    BYTE                        byEnableMulticast; //! 0-关闭组播 1-开启组播
    BYTE                        byWebPortMultiplex; //! 0-不复用 1-端口复用
    BYTE                        byRTSP;             // 1-开启RTSP,0-不启用
    WORD                        wRTSPPort;          // RTSP端口
    WORD                        wRTSPHttpPort;      // RTSP HTTP端口
    BYTE                       byautoadapterip;
	BYTE						byRes3[13];
}ANTS_DVR_NETCFG_MULTI, *LPANTS_DVR_NETCFG_MULTI;



//strat :wifi setting , add by huang, 2015/9/23
typedef enum
{
	ANTS_DVR_WIFI_WORK_MODE_AP = 0,
	ANTS_DVR_WIFI_WORK_MODE_STA,
}ANTS_DVR_WIFI_WORK_MODE_E;

typedef enum
{
	ANTS_DVR_WIFI_WORK_STA_STATUS_DISCONNECTED,

	ANTS_DVR_WIFI_WORK_STA_STATUS_INTERFACE_DISABLED,

	ANTS_DVR_WIFI_WORK_STA_STATUS_INACTIVE,

	ANTS_DVR_WIFI_WORK_STA_STATUS_SCANNING,

	ANTS_DVR_WIFI_WORK_STA_STATUS_AUTHENTICATING,

	ANTS_DVR_WIFI_WORK_STA_STATUS_ASSOCIATING,

	ANTS_DVR_WIFI_WORK_STA_STATUS_ASSOCIATED,

	ANTS_DVR_WIFI_WORK_STA_STATUS_4WAY_HANDSHAKE,

	ANTS_DVR_WIFI_WORK_STA_STATUS_GROUP_HANDSHAKE,

	ANTS_DVR_WIFI_WORK_STA_STATUS_COMPLETED
} ANTS_DVR_WIFI_WORK_STA_STATUS_E;

typedef struct
{
	ANTS_DVR_WIFI_WORK_MODE_E workMode;
	union {
		struct{//STA
			char ssid[64];
			ANTS_DVR_WIFI_WORK_STA_STATUS_E Status; //wpa_cli status			
			char ipv4addr[16];
		}WIFI_STA_STATUS;
		struct {//AP (Readonly for IPC)
			char ssid[64];
			char psk[80];
			char ipv4[16];
			char mac[32];
			int auth_type;
		}WIFI_AP_STATUS;
	}WIFI_STATUS_INFO;
} ANTS_DVR_WIFI_WORKSTATUS_S;

typedef struct 
{
	char ipv4[16];
	char mac[32];
	char reserved[10];
} ANTS_DVR_WIFI_APMODE_STALIST_S;

typedef struct 
{
	char ssid[64];
	char psk[80]; 
	char mac[32]; //Readonly
	int auth_type; //Readonly
} ANTS_DVR_WIFI_STAMODE_APAUTHCFG_S;

typedef struct 
{
	int bDhcp; //使用动态或是静态IP
	char ipv4[16];
	char netmask[16];
	char gateway[16];
} ANTS_DVR_WIFI_STAMODE_IPADDRCFG_S;

typedef struct 
{
	ANTS_DVR_WIFI_STAMODE_APAUTHCFG_S authCfg;
	ANTS_DVR_WIFI_STAMODE_IPADDRCFG_S ipAddrCfg;
} ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S;


typedef struct 
{
	char ssid[64]; //Readonly
	char mac[32]; //Readonly
	int auth_type;  //Readonly //AUTH_MODE_E
	int signal; //Readonly
} ANTS_DVR_WIFI_SCANAPITEM_S;

typedef enum
{
    AUTH_MODE_WEP = 0x01,
    AUTH_MODE_WPA = 0x02,
    AUTH_MODE_WPA2 = 0x04,
    AUTH_MODE_ESS = 0x08,
} AUTH_MODE_E;


//end :wifi setting , add by huang, 2015/9/23


typedef struct
{
	BYTE						byBrightness;  	/*亮度,0-255*/
	BYTE						byContrast;    	/*对比度,0-255*/	
	BYTE						bySaturation;  	/*饱和度,0-255*/
	BYTE						byHue;    		/*色调,0-255*/
}ANTS_DVR_COLOR, *LPANTS_DVR_COLOR;

typedef struct
{
	LONG						lBrightness;  	/*亮度,0-255,-1不支持*/
	LONG						lContrast;    	/*对比度,0-255,-1不支持*/	
	LONG						lSaturation;  	/*饱和度,0-255, -1不支持*/
	LONG						lHue;    		/*色调,0-255,-1不支持*/
}ANTS_DVR_COLOR_EX, *LPANTS_DVR_COLOR_EX;

//时间段(子结构)
typedef struct
{
	//开始时间
    BYTE						byStartHour;
	BYTE						byStartMin;
	//结束时间
	BYTE						byStopHour;
	BYTE						byStopMin;
}ANTS_DVR_SCHEDTIME, *LPANTS_DVR_SCHEDTIME;

//报警和异常处理结构(子结构)(多处使用)
typedef struct
{
	DWORD						dwHandleType;	/*处理方式,处理方式的"或"结果*/
												/*0x00: 无响应*/
												/*0x01: 监视器上警告*/
												/*0x02: 声音警告*/
												/*0x04: 上传中心*/
												/*0x08: 触发报警输出*/
												/*0x10: Jpeg抓图并上传EMail*/
												/*0x20; 联动录像*/
												/*0x40; 联动抓图*/

	BYTE						byRelAlarmOut[MAX_ALARMOUT];	//报警触发的输出通道,报警触发的输出,为1表示触发该输出
}ANTS_DVR_HANDLEEXCEPTION, *LPANTS_DVR_HANDLEEXCEPTION;

typedef struct 
{
	BYTE						byEnableHandleVILost;	/* 是否处理信号丢失报警 */
	BYTE						byRes[3];
	ANTS_DVR_HANDLEEXCEPTION	strVILostHandleType;	/* 处理方式 */
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//布防时间
}ANTS_DVR_VILOST, *LPANTS_DVR_VILOST;

//移动侦测(子结构)
typedef struct 
{
	BYTE						byMotionScope[64][96];									/* 侦测区域,0-22位,表示18行,共有22*18个小宏块,为1表示是移动侦测区域,0-表示不是 */
	BYTE						byMotionSensitive;										/* 移动侦测灵敏度, 0 - 5,越高越灵敏,oxff关闭 */
	BYTE						byEnableHandleMotion;									/* 是否处理移动侦测 0－否 1－是*/ 
	BYTE						byPrecision;											/* 移动侦测算法的进度: 0--16*16, 1--32*32, 2--64*64 ... (暂时固定为0)*/
	char						reservedData;	
	ANTS_DVR_HANDLEEXCEPTION	struMotionHandleType;									/* 处理方式 */
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];				/* 布防时间 */
	BYTE						byRelRecordChan[MAX_CHANNUM];							/* 报警触发的录象通道*/
}ANTS_DVR_MOTION, *LPANTS_DVR_MOTION;

//移动侦测(子结构)
typedef struct 
{
	BYTE						byMotionScope[64][96];									/* 侦测区域,0-22位,表示18行,共有22*18个小宏块,为1表示是移动侦测区域,0-表示不是 */
	BYTE						byMotionSensitive;										/* 移动侦测灵敏度, 0 - 5,越高越灵敏,oxff关闭 */
	BYTE						byEnableHandleMotion;									/* 是否处理移动侦测 0－否 1－是*/ 
	BYTE						byPrecision;											/* 移动侦测算法的进度: 0--16*16, 1--32*32, 2--64*64 ... (暂时固定为0)*/
	char						reservedData;	
	ANTS_DVR_HANDLEEXCEPTION	struMotionHandleType;									/* 处理方式 */
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];				/* 布防时间 */
	BYTE						byRelRecordChan[MAX_CHANNUM_EX];	
	/* 报警触发的录象通道*/
}ANTS_DVR_MOTION_EX, *LPANTS_DVR_MOTION_EX;



//移动侦测(子结构)
typedef struct 
{
	BYTE						byMotionScope[64][96];									/* 侦测区域,0-22位,表示18行,共有22*18个小宏块,为1表示是移动侦测区域,0-表示不是 */
	BYTE						byMotionSensitive;										/* 移动侦测灵敏度, 0 - 5,越高越灵敏,oxff关闭 */
	BYTE						byEnableHandleMotion;									/* 是否处理移动侦测 0－否 1－是*/ 
	BYTE						byPrecision;											/* 移动侦测算法的进度: 0--16*16, 1--32*32, 2--64*64 ... (暂时固定为0)*/
	char						reservedData;	
	ANTS_DVR_HANDLEEXCEPTION	struMotionHandleType;									/* 处理方式 */
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];				/* 布防时间 */
	BYTE						byRelRecordChan[MAX_CHANNUM_EX];	
	BYTE						bySnapCount;
	BYTE						bySnapInterval;
	BYTE						byRes[62];
	/* 报警触发的录象通道*/
}ANTS_DVR_MOTION_EX2, *LPANTS_DVR_MOTION_EX2;



//遮挡报警(子结构)  区域大小704*576
typedef struct 
{
	DWORD						dwEnableHideAlarm;				/* 是否启动遮挡报警 ,0-否,1-低灵敏度 2-中灵敏度 3-高灵敏度*/
	WORD						wHideAlarmAreaTopLeftX;			/* 遮挡区域的x坐标 */
	WORD						wHideAlarmAreaTopLeftY;			/* 遮挡区域的y坐标 */
	WORD						wHideAlarmAreaWidth;			/* 遮挡区域的宽 */
	WORD						wHideAlarmAreaHeight;			/* 遮挡区域的高*/
	ANTS_DVR_HANDLEEXCEPTION	strHideAlarmHandleType;			/* 处理方式 ,仅联动本通道录像*/
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//布防时间
}ANTS_DVR_HIDEALARM, *LPANTS_DVR_HIDEALARM;



typedef struct 
{
	BYTE		dwEnableHideAlarm;				/* 是否启动遮挡报警 ,0-否,1-低灵敏度 2-中灵敏度 3-高灵敏度*/
	BYTE        bySnapCount;
    BYTE        bySnapInterval;
	BYTE        byRes;
	WORD		wHideAlarmAreaTopLeftX;			/* 遮挡区域的x坐标 */
	WORD		wHideAlarmAreaTopLeftY;			/* 遮挡区域的y坐标 */
	WORD		wHideAlarmAreaWidth;			/* 遮挡区域的宽 */
	WORD		wHideAlarmAreaHeight;			/* 遮挡区域的高*/
	DWORD       dwHandleType;	/*处理方式,处理方式的"或"结果*/
							/*0x00: 无响应*/
							/*0x01: 监视器上警告*/
							/*0x02: 声音警告*/
							/*0x04: 上传中心*/
							/*0x08: 触发报警输出*/
							/*0x10: Jpeg抓图并上传EMail*/
							/*0x20: 联动录像 ,仅联动本通道录像*/
							/*0x40; 联动抓图*/
	DWORD     dwRelAlarmOut[16]; // 按位，HIGH->LOW ,最大支持16 * 32
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];//布防时间
}ANTS_DVR_HIDEALARM_V2, *LPANTS_DVR_HIDEALARM_V2;

typedef struct 
{
	WORD                        wSrcChan;                      // 源通道号
	WORD                        wLinkChan;                      //联动通道号
    BYTE						byEnablePreset;				/* 是否调用预置点 0-否,1-是*/
	BYTE						byPresetNo;					/* 调用的云台预置点序号,一个报警输入可以调用多个通道的云台预置点, 0xff表示不调用预置点。*/
	BYTE						byEnableCruise;				/* 是否调用巡航 0-否,1-是*/
	BYTE						byCruiseNo;					/* 巡航 */
	BYTE						byEnablePtzTrack;				/* 是否调用轨迹 0-否,1-是*/
	BYTE						byPTZTrack;					/* 调用的云台的轨迹序号 */
	BYTE                        byRes[2];
    DWORD						dwRes[13];
}ANTS_DVR_PTZLINKCFG, *LPANTS_DVR_PTZLINKCFG;






typedef struct 
{
	ANTS_DVR_SCHEDTIME	struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];				/* 布防时间 */							
	
	ANTS_DVR_PTZLINKCFG strPtzLink; //PTZ联动
	
	DWORD   dwRelAlarmOut[16]; //是否触发报警输出
	DWORD   dwRelRecordChan[16]; //是否触发录像

	DWORD	dwHandleType;	/*处理方式,处理方式的"或"结果*/
							/*0x00: 无响应*/
							/*0x01: 监视器上警告*/
							/*0x02: 声音警告*/
							/*0x04: 上传中心*/
							/*0x08: 触发报警输出*/
							/*0x10: Jpeg抓图并上传EMail*/
							/*0x20; 联动录像*/
							/*0x40; 联动抓图*/
	BYTE	byEnableHandle;					
    BYTE    bySnapCount;
    BYTE    bySnapInterval;
    BYTE    byRes[13];
}ANTS_DVR_IVS_DETECT_LINK, *LPANTS_DVR_IVS_DETECT_LINK;



//遮挡区域(子结构)
typedef struct 
{
	WORD						wHideAreaTopLeftX;				/* 遮挡区域的x坐标 */
	WORD						wHideAreaTopLeftY;				/* 遮挡区域的y坐标 */
	WORD						wHideAreaWidth;					/* 遮挡区域的宽 */
	WORD						wHideAreaHeight;				/*遮挡区域的高*/
}ANTS_DVR_SHELTER, *LPANTS_DVR_SHELTER;

typedef struct 
{
	//遮挡	区域大小704*576
		DWORD						dwEnableHide;					/* 是否启动遮挡 ,0-否,1-是*/
		ANTS_DVR_SHELTER			struShelter[MAX_SHELTERNUM];

}ANTS_DVR_SHELTER_EX, *LPANTS_DVR_SHELTER_EX;

typedef struct 
{
	DWORD						dwSize;
	BYTE						sChanName[NAME_LEN];
	
	//显示通道名
	DWORD						dwShowChanName; 				// 预览的图象上是否显示通道名称,0-不显示,1-显示 区域大小352*288
	WORD						wShowNameTopLeftX;				/* 通道名称显示位置的x坐标 */
	WORD						wShowNameTopLeftY;				/* 通道名称显示位置的y坐标 */

	//OSD
	DWORD						dwShowOsd;						// 预览的图象上是否显示OSD,0-不显示,1-显示 区域大小352*288
	WORD						wOSDTopLeftX;					/* OSD的x坐标 */
	WORD						wOSDTopLeftY;					/* OSD的y坐标 */

	BYTE						byOSDType;						/* OSD类型(主要是年月日格式) */
																/* 0: XXXX-XX-XX 年月日 */
																/* 1: XX-XX-XXXX 月日年 */
																/* 2: XX-XX-XXXX 日月年*/
																/* 3: XXXX年XX月XX日 */
																/* 4: XX月XX日XXXX年 */
																/* 5: XX日XX月XXXX年 */

	BYTE						byDispWeek;						/* 是否显示星期 */
	BYTE						byOSDAttrib;					/* OSD属性:透明，闪烁 (保留)*/
    BYTE						byHourOSDType;					/* OSD小时制:0-24小时制,1-12小时制 */
}ANTS_DVR_OSDCFG,*LPANTS_DVR_OSDCFG;


typedef struct 
{
	DWORD						dwSize;
	BYTE						sChanName[NAME_LEN];
	
	//显示通道名
	DWORD						dwShowChanName; 				// 预览的图象上是否显示通道名称,0-不显示,1-显示 区域大小352*288
	WORD						wShowNameTopLeftX;				/* 通道名称显示位置的x坐标 */
	WORD						wShowNameTopLeftY;				/* 通道名称显示位置的y坐标 */

	//OSD
	DWORD						dwShowOsd;						// 预览的图象上是否显示OSD,0-不显示,1-显示 区域大小352*288
	WORD						wOSDTopLeftX;					/* OSD的x坐标 */
	WORD						wOSDTopLeftY;					/* OSD的y坐标 */

	BYTE						byOSDType;						/* OSD类型(主要是年月日格式) */
																/* 0: XXXX-XX-XX 年月日 */
																/* 1: XX-XX-XXXX 月日年 */
																/* 2: XXXX年XX月XX日 */
																/* 3: XX月XX日XXXX年 */
																/* 4: XX-XX-XXXX 日月年*/
																/* 5: XX日XX月XXXX年 */

	BYTE						byDispWeek;						/* 是否显示星期 */
	BYTE						byOSDAttrib;					/* OSD属性:透明，闪烁 (保留)*/
    BYTE						byHourOSDType;					/* OSD小时制:0-24小时制,1-12小时制 */
    
// 码流OSD大小: value="10", text="X1", value="15", text="X1.5", value="20", text="X2", value="30", text="X3", value="40", text="X4",
    BYTE                        byFirstStreamOsdSize;           // 主码流OSD大小
    BYTE                        bySecondStreamOsdSize;          // 子码流OSD大小
    WORD                        wOsdColorContent; // OSD字体色彩,RGB1555格式
    WORD                        wOsdColorBack;  // OSD背景色彩,RGB1555格式
    WORD                        wOsdColorMargin;    // OSD边缘色彩,RGB1555格式
	BYTE						byRes[56];
}ANTS_DVR_OSDCFG_V2,*LPANTS_DVR_OSDCFG_V2;




//通道图象结构
typedef struct
{
	DWORD						dwSize;
	BYTE						sChanName[NAME_LEN];
	DWORD						dwVideoFormat;					/*视频制式 1-NTSC 2-PAL*/
	ANTS_DVR_COLOR				struColor;						//图像参数

	char						reservedData [60];				/*保留*/

	//显示通道名
	DWORD						dwShowChanName;					// 预览的图象上是否显示通道名称,0-不显示,1-显示 区域大小352*288
	WORD						wShowNameTopLeftX;				/* 通道名称显示位置的x坐标 */
	WORD						wShowNameTopLeftY;				/* 通道名称显示位置的y坐标 */

	//视频信号丢失报警
	ANTS_DVR_VILOST				struVILost;
	ANTS_DVR_VILOST				struRes;						/*保留*/

	//移动侦测
	ANTS_DVR_MOTION_EX			struMotion;

	//遮挡报警
	ANTS_DVR_HIDEALARM			struHideAlarm;

	//遮挡  区域大小704*576
	DWORD						dwEnableHide;					/* 是否启动遮挡 ,0-否,1-是*/
	ANTS_DVR_SHELTER			struShelter[MAX_SHELTERNUM];

	//OSD
	DWORD						dwShowOsd;						// 预览的图象上是否显示OSD,0-不显示,1-显示 区域大小352*288
	WORD						wOSDTopLeftX;					/* OSD的x坐标 */
	WORD						wOSDTopLeftY;					/* OSD的y坐标 */

	BYTE						byOSDType;						/* OSD类型(主要是年月日格式) */
																/* 0: XXXX-XX-XX 年月日 */
																/* 1: XX-XX-XXXX 月日年 */
																/* 2: XX-XX-XXXX 日月年*/
																/* 3: XXXX年XX月XX日 */
																/* 4: XX月XX日XXXX年 */
																/* 5: XX日XX月XXXX年 */

	BYTE						byDispWeek;						/* 是否显示星期 */
	BYTE						byOSDAttrib;					/* OSD属性:透明，闪烁 (保留)*/
    BYTE						byHourOSDType;					/* OSD小时制:0-24小时制,1-12小时制 */
	BYTE						byRes[64];
}ANTS_DVR_PICCFG_EX, *LPANTS_DVR_PICCFG_EX;


//码流压缩参数(子结构)
typedef struct 
{
	BYTE						byStreamType;		//码流类型 0-视频流, 1-复合流, 表示事件压缩参数时最高位表示是否启用压缩参数
	BYTE						byResolution;  		//分辨率0-DCIF 1-CIF, 2-QCIF, 3-4CIF, 4-2CIF 5（保留）,5-640x352
	                        						//16-VGA（640*480）, 17-UXGA（1600*1200）, 18-SVGA （800*600）,
	                        						//19-HD720p（1280*720）,20-XVGA,  21-HD900p, 27-HD1080i, 
	                        						//28-2560*1920, 29-1600*304, 30-2048*1536, 31-2448*2048	
	BYTE						byBitrateType;		//码率类型 0:变码率, 1:定码率
	BYTE						byPicQuality;		//图象质量 0-最好 1-次好 2-较好 3-一般 4-较差 5-差
	DWORD						dwVideoBitrate; 	//视频码率 0-保留 1-16K 2-32K 3-48k 4-64K 5-80K 6-96K 7-128K 8-160k 9-192K 10-224K 11-256K 12-320K
													// 13-384K 14-448K 15-512K 16-640K 17-768K 18-896K 19-1024K 20-1280K 21-1536K 22-1792K 23-2048K
													//最高位(31位)置成1表示是自定义码流, 0-30位表示码流值。
	DWORD						dwVideoFrameRate;	//帧率 0-全部; 1-1/16; 2-1/8; 3-1/4; 4-1/2; 5-1; 6-2; 7-4; 8-6; 9-8; 10-10; 11-12; 12-16; 13-20; 14-15; 15-18; 16-22;
	WORD						wIntervalFrameI;	//I帧间隔

	BYTE						byIntervalBPFrame;	//0-BBP帧; 1-BP帧; 2-单P帧 (保留)
 	BYTE						byVideoH264Profile; // H264 Profiles:0-baseline,1-main,2-high
 	BYTE						byVideoEncType;		//视频编码类型 0 私有h264;1标准h264; 2标准mpeg4; 3-M-JPEG
 	BYTE						byAudioEncType;		//音频编码类型 0-OggVorbis;1-G711_U;2-G711_A
 	BYTE						byres[10];			//这里保留音频的压缩参数
}ANTS_DVR_COMPRESSION_INFO, *LPANTS_DVR_COMPRESSION_INFO;

//通道压缩参数
typedef struct 
{
	DWORD						dwSize;
	ANTS_DVR_COMPRESSION_INFO	struNormHighRecordPara;	//录像
	ANTS_DVR_COMPRESSION_INFO	struRes;				//保留 char reserveData[28];
    ANTS_DVR_COMPRESSION_INFO	struEventRecordPara;	//事件触发压缩参数
	ANTS_DVR_COMPRESSION_INFO	struNetPara;			//网传(子码流)
}ANTS_DVR_COMPRESSIONCFG, *LPANTS_DVR_COMPRESSIONCFG;


//时间段录像参数配置(子结构)
typedef struct 
{
	ANTS_DVR_SCHEDTIME			struRecordTime;
	BYTE						byRecordType;	//0:定时录像，1:移动侦测，2:报警录像，3:动测|报警，4:动测&报警, 5:命令触发, 6:手动录像
	char						reservedData[3];
}ANTS_DVR_RECORDSCHED, *LPANTS_DVR_RECORDSCHED;

//全天录像参数配置(子结构)
typedef struct 
{
	WORD						wAllDayRecord;				/* 是否全天录像 0-否 1-是*/
	BYTE						byRecordType;				/* 录象类型 0:定时录像，1:移动侦测，2:报警录像，3:动测|报警，4:动测&报警 5:命令触发*/
	char						reservedData;
}ANTS_DVR_RECORDDAY, *LPANTS_DVR_RECORDDAY;

//通道录像参数配置
typedef struct 
{
	DWORD						dwSize;
	DWORD						dwRecord;  						/*是否录像 0-否 1-是*/
	ANTS_DVR_RECORDDAY			struRecAllDay[MAX_DAYS];
	ANTS_DVR_RECORDSCHED		struRecordSched[MAX_DAYS][MAX_TIMESEGMENT];
	DWORD						dwRecordTime;					/* 录象延时长度 0-5秒， 1-20秒， 2-30秒， 3-1分钟， 4-2分钟， 5-5分钟， 6-10分钟*/
	DWORD						dwPreRecordTime;				/* 预录时间 0-不预录 1-5秒，2-10秒，3-15秒，4-20秒，5-25秒，6-30秒 7-0xffffffff(尽可能预录) */
	DWORD						dwRecorderDuration;				/* 录像保存的最长时间 */
	BYTE						byRedundancyRec;				/*是否冗余录像,重要数据双备份：0/1*/
	BYTE						byAudioRec;						/*录像时复合流编码时是否记录音频数据：国外有此法规*/
	BYTE						byReserve[10];	
}ANTS_DVR_RECORD, *LPANTS_DVR_RECORD;


//通道解码器(云台)参数配置
typedef struct 
{
	DWORD						dwSize;
	DWORD						dwBaudRate;						//波特率(bps)，0－50，1－75，2－110，3－150，4－300，5－600，6－1200，7－2400，8－4800，9－9600，10－19200， 11－38400，12－57600，13－76800，14－115.2k;
	BYTE						byDataBit;						//数据有几位 0－5位，1－6位，2－7位，3－8位;
	BYTE						byStopBit;						//停止位 0－1位，1－2位;
	BYTE						byParity;						//校验 0－无校验，1－奇校验，2－偶校验;
	BYTE						byFlowcontrol;					//0－无，1－软流控,2-硬流控
	WORD						wDecoderType;					//解码器类型  NET_DVR_IPC_PROTO_LIST中得到
	WORD						wDecoderAddress;				/*解码器地址:0 - 255*/
	BYTE						bySetPreset[MAX_PRESET];		/* 预置点是否设置,0-没有设置,1-设置*/
	BYTE						bySetCruise[MAX_CRUISE];		/* 巡航是否设置: 0-没有设置,1-设置 */
	BYTE						bySetTrack[MAX_TRACK];			/* 轨迹是否设置,0-没有设置,1-设置*/
}ANTS_DVR_DECODERCFG, *LPANTS_DVR_DECODERCFG;

//ppp参数配置(子结构)
typedef struct 
{
	ANTS_DVR_IPADDR				struRemoteIP;			//远端IP地址
	ANTS_DVR_IPADDR				struLocalIP;			//本地IP地址
	char						sLocalIPMask[16];		//本地IP地址掩码
	BYTE						sUsername[NAME_LEN];	/* 用户名 */
	BYTE						sPassword[PASSWD_LEN];	/* 密码 */
	BYTE						byPPPMode;				//PPP模式, 0－主动，1－被动
	BYTE						byRedial;				//是否回拨 ：0-否,1-是
	BYTE						byRedialMode;			//回拨模式,0-由拨入者指定,1-预置回拨号码
	BYTE						byDataEncrypt;			//数据加密,0-否,1-是
	DWORD						dwMTU;					//MTU
	char						sTelephoneNumber[PHONENUMBER_LEN];	//电话号码
}ANTS_DVR_PPPCFG, *LPANTS_DVR_PPPCFG;

//RS232串口参数配置
typedef struct
{
    DWORD						dwBaudRate;		/*波特率(bps)，0－50，1－75，2－110，3－150，4－300，5－600，6－1200，7－2400，8－4800，9－9600，10－19200， 11－38400，12－57600，13－76800，14－115.2k;*/
    BYTE						byDataBit;		/* 数据有几位 0－5位，1－6位，2－7位，3－8位 */
    BYTE						byStopBit;		/* 停止位 0－1位，1－2位 */
    BYTE						byParity;		/* 校验 0－无校验，1－奇校验，2－偶校验 */
    BYTE						byFlowcontrol;	/* 0－无，1－软流控,2-硬流控 */
    DWORD						dwWorkMode;		/* 工作模式，0－232串口用于PPP拨号，1－232串口用于参数控制，2－透明通道 */
}ANTS_DVR_SINGLE_RS232;

//RS232串口参数配置
typedef struct 
{
	DWORD						dwSize;
    ANTS_DVR_SINGLE_RS232		struRs232;
	BYTE						byRes[84]; 
	ANTS_DVR_PPPCFG				struPPPConfig;
}ANTS_DVR_RS232CFG, *LPANTS_DVR_RS232CFG;

//报警输入参数配置
typedef struct 
{
	DWORD						dwSize;
	BYTE						sAlarmInName[NAME_LEN];						/* 名称 */
	BYTE						byAlarmType;	            				//报警器类型,0：常开,1：常闭
	BYTE						byAlarmInHandle;	        				/* 是否处理 0-不处理 1-处理*/
    BYTE                        bySnapCount;
    BYTE                        bySnapInterval;
	ANTS_DVR_HANDLEEXCEPTION	struAlarmHandleType;						/* 处理方式 */
	ANTS_DVR_SCHEDTIME			struAlarmTime[MAX_DAYS][MAX_TIMESEGMENT];	//布防时间
	BYTE						byRelRecordChan[MAX_CHANNUM_EX];				//报警触发的录象通道,为1表示触发该通道
	BYTE						byEnablePreset[MAX_CHANNUM_EX];				/* 是否调用预置点 0-否,1-是*/
	BYTE						byPresetNo[MAX_CHANNUM_EX];					/* 调用的云台预置点序号,一个报警输入可以调用多个通道的云台预置点, 0xff表示不调用预置点。*/
	BYTE						byRes2[192];								/* 保留 */
	BYTE						byEnableCruise[MAX_CHANNUM_EX];				/* 是否调用巡航 0-否,1-是*/
	BYTE						byCruiseNo[MAX_CHANNUM_EX];					/* 巡航 */
	BYTE						byEnablePtzTrack[MAX_CHANNUM_EX];				/* 是否调用轨迹 0-否,1-是*/
	BYTE						byPTZTrack[MAX_CHANNUM_EX];					/* 调用的云台的轨迹序号 */
    BYTE						byRes3[16];
}ANTS_DVR_ALARMINCFG_EX, *LPANTS_DVR_ALARMINCFG_EX;

//DVR报警输出
typedef struct 
{
	DWORD						dwSize;
	BYTE						sAlarmOutName[NAME_LEN];	/* 名称 */
	DWORD						dwAlarmOutDelay;			/* 输出保持时间(-1为无限，手动关闭) */
															//0-5秒,1-10秒,2-30秒,3-1分钟,4-2分钟,5-5分钟,6-10分钟,7-手动,高位为1表示自定义

	ANTS_DVR_SCHEDTIME			struAlarmOutTime[MAX_DAYS][MAX_TIMESEGMENT];/* 报警输出激活时间段 */
	BYTE						byAlarmType;	            				//报警器类型,0：常开,1：常闭													
    BYTE						byRes[15];
}ANTS_DVR_ALARMOUTCFG, *LPANTS_DVR_ALARMOUTCFG;

//校时结构参数
typedef struct
{
	DWORD						dwYear;			//年
	DWORD						dwMonth;		//月
	DWORD						dwDay;			//日
	DWORD						dwHour;			//时
	DWORD						dwMinute;		//分
	DWORD						dwSecond;		//秒
}ANTS_DVR_TIME, *LPANTS_DVR_TIME;

//单用户参数(子结构)
typedef struct
{
	char						sUserName[NAME_LEN];		/* 用户名 */
	char  					sPassword[PASSWD_LEN];		/* 密码 */
	BYTE						byLocalRight[MAX_RIGHT];	/* 本地权限 */
															/*数组0: 本地控制云台*/
															/*数组1: 本地手动录象*/
															/*数组2: 本地回放*/
															/*数组3: 本地设置参数*/
															/*数组4: 本地查看状态、日志*/
															/*数组5: 本地高级操作(升级，格式化，重启，关机)*/
														    /*数组6: 本地查看参数 */
														    /*数组7: 本地管理模拟和IP camera */
														    /*数组8: 本地备份 */
														    /*数组9: 本地关机/重启 */

	BYTE						byRemoteRight[MAX_RIGHT];	/* 远程权限 */	
															/*数组0: 远程控制云台*/
															/*数组1: 远程手动录象*/
															/*数组2: 远程回放 */
															/*数组3: 远程设置参数*/
															/*数组4: 远程查看状态、日志*/
															/*数组5: 远程高级操作(升级，格式化，重启，关机)*/
															/*数组6: 远程发起语音对讲*/
															/*数组7: 远程预览*/
															/*数组8: 远程请求报警上传、报警输出*/
															/*数组9: 远程控制，本地输出*/
															/*数组10: 远程控制串口*/	
														    /*数组11: 远程查看参数 */
														    /*数组12: 远程管理模拟和IP camera */
														    /*数组13: 远程关机/重启 */

	BYTE						byNetPreviewRight[MAX_CHANNUM_EX];		/* 远程可以预览的通道 1-有权限，0-无权限*/
	BYTE						byLocalPlaybackRight[MAX_CHANNUM_EX];	/* 本地可以回放的通道 1-有权限，0-无权限*/
	BYTE						byNetPlaybackRight[MAX_CHANNUM_EX];	/* 远程可以回放的通道 1-有权限，0-无权限*/
	BYTE						byLocalRecordRight[MAX_CHANNUM_EX];	/* 本地可以录像的通道 1-有权限，0-无权限*/
	BYTE						byNetRecordRight[MAX_CHANNUM_EX];		/* 远程可以录像的通道 1-有权限，0-无权限*/
	BYTE						byLocalPTZRight[MAX_CHANNUM_EX];		/* 本地可以PTZ的通道 1-有权限，0-无权限*/
	BYTE						byNetPTZRight[MAX_CHANNUM_EX];			/* 远程可以PTZ的通道 1-有权限，0-无权限*/
	BYTE						byLocalBackupRight[MAX_CHANNUM_EX];	/* 本地备份权限通道 1-有权限，0-无权限*/
	ANTS_DVR_IPADDR				struUserIP;							/* 用户IP地址(为0时表示允许任何地址) */
	BYTE						byMACAddr[MACADDR_LEN];				/* 物理地址 */
	BYTE						byPriority;							/* 优先级，0xff-无，0--低，1--中，2--高 */
								                                    /*
								                                    无……表示不支持优先级的设置
								                                    低……默认权限:包括本地和远程回放,本地和远程查看日志和状态,本地和远程关机/重启
								                                    中……包括本地和远程控制云台,本地和远程手动录像,本地和远程回放,语音对讲和远程预览
								                                          本地备份,本地/远程关机/重启
								                                    高……管理员
								                                    */
	BYTE						byLocalPreviewRight[MAX_CHANNUM_EX];/* 本地可以预览的通道 1-有权限，0-无权限*/
	BYTE						byRes[1];	
}ANTS_DVR_USER_INFO_EX, *LPANTS_DVR_USER_INFO_EX;

//DVR用户参数
typedef struct
{
	DWORD						dwSize;
	ANTS_DVR_USER_INFO_EX			struUser[MAX_USERNUM];
}ANTS_DVR_USER_EX, *LPANTS_DVR_USER_EX;

//DVR异常参数
typedef struct 
{
	DWORD dwSize;
	ANTS_DVR_HANDLEEXCEPTION	struExceptionHandleType[MAX_EXCEPTIONNUM];
	/*数组0-盘满,1- 硬盘出错,2-网线断,3-局域网内IP 地址冲突, 4-非法访问, 5-输入/输出视频制式不匹配, 6-视频信号异常, 7-录像异常*/
}ANTS_DVR_EXCEPTION, *LPANTS_DVR_EXCEPTION;

//时间点(子结构)
typedef struct 
{
	DWORD						dwMonth;		//月 0-11表示1-12个月
	DWORD						dwWeekNo;		//第几周 0－第1周 1－第2周 2－第3周 3－第4周 4－最后一周
	DWORD						dwWeekDate;		//星期几 0－星期日 1－星期一 2－星期二 3－星期三 4－星期四 5－星期五 6－星期六
	DWORD						dwHour;			//小时	开始时间0－23 结束时间1－23
	DWORD						dwMin;			//分	0－59
}ANTS_DVR_TIMEPOINT, *LPANTS_DVR_TIMEPOINT;

//夏令时参数
typedef struct 
{
	DWORD						dwSize;
	BYTE						byRes1[16];			//保留
	DWORD						dwEnableDST;		//是否启用夏时制 0－不启用 1－启用
	BYTE						byDSTBias;			//夏令时偏移值，30min, 60min, 90min, 120min, 以分钟计，传递原始数值
	BYTE						byRes2[3];
	ANTS_DVR_TIMEPOINT			struBeginPoint;		//夏时制开始时间
	ANTS_DVR_TIMEPOINT			struEndPoint;		//夏时制停止时间
}ANTS_DVR_ZONEANDDST, *LPANTS_DVR_ZONEANDDST;

//单字符参数(子结构)
typedef struct 
{
	WORD						wShowString;				// 预览的图象上是否显示字符,0-不显示,1-显示 区域大小704*576,单个字符的大小为32*32
	WORD						wStringSize;				/* 该行字符的长度，不能大于44个字符 */
	WORD						wShowStringTopLeftX;		/* 字符显示位置的x坐标 */
	WORD						wShowStringTopLeftY;		/* 字符名称显示位置的y坐标 */
	char						sString[512];				/* 要显示的字符内容 */
}ANTS_DVR_SHOWSTRINGINFO, *LPANTS_DVR_SHOWSTRINGINFO;

//叠加字符
typedef struct 
{
	DWORD						dwSize;
	ANTS_DVR_SHOWSTRINGINFO		struStringInfo[MAX_STRINGNUM];				/* 要显示的字符内容 */
}ANTS_DVR_SHOWSTRING, *LPANTS_DVR_SHOWSTRING;

//语音对讲参数
typedef struct tagANTS_DVR_COMPRESSION_AUDIO
{
	BYTE						byAudioEncType;		//音频编码类型 0-OggVorbis;1-G711_A;2-G711U;3-G722;4-G726(默认)
	BYTE						byres[7];			//这里保留音频的压缩参数 
}ANTS_DVR_COMPRESSION_AUDIO, *LPANTS_DVR_COMPRESSION_AUDIO;

//自动维护参数
typedef struct tagANTS_DVR_AUTOREBOOT
{
	BYTE						byAutoRebootMode;	//自动维护模式:0--不维护，1--每天定时维护，2--每周定时维护，3--单次维护
	DWORD						dwSingleTime;		//单次维护时间:time_t类型
	DWORD						dwEveryDayTime;		//每天维护时间:0-7位是分钟，8-15位是小时
	BOOL						bWeeklyDay[MAX_DAYS];		//每周7天是否启动维护:0--星期天，1--星期一，依次往后
	DWORD						dwWeeklyTime[MAX_DAYS];	//每周维护时间:0-7位是分钟，8-15位是小时
}ANTS_DVR_AUTOREBOOT, *LPANTS_DVR_AUTOREBOOT;

//ntp
typedef struct 
{
	BYTE						sNTPServer[64];					/* Domain Name or IP addr of NTP server */
	WORD						wInterval;						/* adjust time interval(hours) */
	BYTE						byEnableNTP;					/* enable NPT client 0-no，1-yes*/
	signed char					cTimeDifferenceH;				/* 与国际标准时间的 小时偏移-12 ... +13 */
	signed char					cTimeDifferenceM;				/* 与国际标准时间的 分钟偏移0, 30, 45*/
	BYTE						res1;
	WORD						wNtpPort;						/* ntp server port 设备默认为123*/
	BYTE						res2[8];
}ANTS_DVR_NTPPARA, *LPANTS_DVR_NTPPARA;

//ddns
typedef struct 
{
	BYTE						byEnableDDNS;
	BYTE						byHostIndex;					/* 0-私有DDNS 1－Dyndns 2－PeanutHull(花生壳) 3- NO-IP 4-qdns*/
	BYTE						byRes1[2];
    struct
    {    
		BYTE					sUsername[NAME_LEN];			/* DDNS账号用户名*/
		BYTE					sPassword[PASSWD_LEN];			/* 密码 */
		BYTE					sDomainName[MAX_DOMAIN_NAME];	/* 设备配备的域名地址 */
		BYTE					sServerName[MAX_DOMAIN_NAME];	/* DDNS协议对应的服务器地址，可以是IP地址或域名 */
		WORD					wDDNSPort;						/* 端口号 */
		WORD                    wCheckIPIntervalTime;                 /*IP检测间隔时间,单位秒*/
		WORD                    wUpdateIPIntervalTime;                 /*IP更新间隔时间,单位秒*/
		BYTE					byRes[6];
    }struDDNS[MAX_DDNS_NUMS];
	BYTE						byRes2[16];
}ANTS_DVR_DDNSPARA, *LPANTS_DVR_DDNSPARA;

//FTP
enum {
    ANTS_DVR_FTPUPLOAD_ADDR_LEN = 256,
    ANTS_DVR_FTPUPLOAD_PATH_LEN = 256,
    ANTS_DVR_FTPUPLOAD_USERNAME_LEN = 64,
    ANTS_DVR_FTPUPLOAD_PASSWORD_LEN = 64,
};

typedef struct
{
    BYTE bEnable;
    BYTE bPasvmode; // 是否被动模式
    WORD wRemotePort; 
    char sRemoteHost[ANTS_DVR_FTPUPLOAD_ADDR_LEN];
    char sUsername[ANTS_DVR_FTPUPLOAD_USERNAME_LEN];
    char sPassword[ANTS_DVR_FTPUPLOAD_PASSWORD_LEN];
    char sUploadPath[ANTS_DVR_FTPUPLOAD_PATH_LEN];
    
    BYTE byReserved[32];
} ANTS_DVR_FTPUPLOAD, *LPANTS_DVR_FTPUPLOAD;


//网络参数配置
typedef struct 
{
	DWORD						dwSize;
	char						sDNSIp[16];						/* DNS服务器地址 */
	ANTS_DVR_NTPPARA			struNtpClientParam;				/* NTP参数 */
	ANTS_DVR_DDNSPARA			struDDNSClientParam;			/* DDNS参数 */
	BYTE						res[464];						/* 保留 */
}ANTS_DVR_NETAPPCFG, *LPANTS_DVR_NETAPPCFG;

/*EMAIL参数结构*/
typedef struct
{		
	DWORD						dwSize;
	BYTE						sAccount[NAME_LEN];				/* 账号*/ 
	BYTE						sPassword[MAX_EMAIL_PWD_LEN];	/*密码 */

	struct
	{
		BYTE					sName[NAME_LEN];				/* 发件人姓名 */
		BYTE					sAddress[MAX_EMAIL_ADDR_LEN];	/* 发件人地址 */
	}struSender;

	BYTE						sSmtpServer[MAX_EMAIL_ADDR_LEN];/* smtp服务器 */
	BYTE						sPop3Server[MAX_EMAIL_ADDR_LEN];/* pop3服务器 */

	struct
	{
		BYTE					sName[NAME_LEN];				/* 收件人姓名 */
		BYTE					sAddress[MAX_EMAIL_ADDR_LEN];	/* 收件人地址 */
	}struReceiver[3];											/* 最多可以设置3个收件人 */

	BYTE						byAttachment;					/* 是否带附件 */
	BYTE						bySmtpServerVerify;				/* 发送服务器要求身份验证 */
    BYTE        				byMailInterval;                 /* mail interval */
	BYTE        				byEnableSSL;					//ssl是否启用
	WORD        				wSmtpPort;						//gmail的465，普通的为25     
	BYTE        				byRes[74];						//保留
} ANTS_DVR_EMAILCFG, *LPANTS_DVR_EMAILCFG;

typedef struct
{
	DWORD						dwSize;							//!结构长度
	BYTE						byEnable;						//!0-禁用SNMP，1-表示启用SNMP
	BYTE						byRes1[3];						//!保留
	WORD						wVersion;						//!snmp 版本  v1 = 1, v2 =2, v3 =3，设备目前不支持 v3
	WORD						wServerPort;					//!snmp消息接收端口，默认 161
	BYTE						byReadCommunity[NAME_LEN];		//!读共同体，最多31,默认"public"
	BYTE						byWriteCommunity[NAME_LEN];		//!写共同体,最多31 字节,默认 "private"
	BYTE						byTrapHostIP [DESC_LEN_64];		//!自陷主机ip地址描述，支持IPV4 IPV6和域名描述    
	WORD						wTrapHostPort;					//!trap主机端口
	BYTE	 					bySendCount;	 				//!发送次数
	BYTE	 					bySendInterval;	 				//!发送时间间隔
	BYTE						byRes2[100];					//!保留
}ANTS_DVR_SNMPCFG, *LPANTS_DVR_SNMPCFG;

typedef struct {
    DWORD dwSize;
    char	sFirstDNSIP[16];
    char	sSecondDNSIP[16];
    char	sRes[32];
}ANTS_DVR_NETCFG_OTHER, *LPANTS_DVR_NETCFG_OTHER;


typedef struct
{
	DWORD						dwSize;							//!长度
	WORD						wPort;							//!RTSPrtsp服务器侦听端口
	WORD                        wHttpPort;                      //! RTSP http 端口
	BYTE                       byEnable;                       // 1-启用,0- 不启用
	BYTE						byReserve[51];					//!预留
}ANTS_DVR_RTSPCFG, *LPANTS_DVR_RTSPCFG;


typedef struct
{
    int             PictureFormat;          //设置编码格式  0 - CIF  1 - D1
    int             BitRate;                //Bit率,0 - 2048  默认 512
    int             FrameRate;              //帧率,1 - 25 默认 15
    int             PicQualityLever;        //编码图像质量，vbr有效
    int             bCbr;                   //码率控制类型：0为VBR，1为CBR
    int             KeyFrameInterval;       //关键帧间隔
    BYTE            byReserved[32];    
}ANTS_DVR_ZEROCODEC_PARA, *LPANTS_DVR_ZEROCODEC_PARA;



// IPV6/域名支持,可用于搜索结果或者通道设备配置
typedef struct _tagANTS_DVR_DEVICE_CHANINFO
{
	DWORD		dwSize;
	DWORD		dwProtocolType;//协议类型(即协议ID)
	char        szProtocolName[16];// 协议名(希望仅做显示用,对内部来说，它不是重要的)
	BYTE        byDeviceType;//0-未定义的 (可默认为IPC),  1-dvr,2-nvr,3-ipc,4-dec
	BYTE	    byEnableQuickAdd;// 支持自动配置,IP可修改的
	BYTE        byMac[6];// MAC ,6 bytes HEX
	DWORD		dwChanNum; // 设备的通道数0-不定，可认为是一个通道;
	
	BYTE		byTransMode;   //可用于指定默认模式：0-主码流模式，1-子码流模式
	BYTE		byLinkProtocol;	//可用于指定默认模式: 0-TCP;1-UDP;2-多播
	BYTE		byEXMode;		//可用于指定默认模式:是否启用流模式 1:开启
	BYTE		byRTSPMode; // 0: 一般模式;1:该协议同RTSP协议,szDomainAux有效,配置或PTZ都走RTSP流方式。
	WORD		wChannel; // 使用的设备通道,默认是第一通道
	
	WORD		wLinkAblility; // 连接支持能力,支持或操作:(0-TCP + UDP ,)1-TCP,2-UDP,...
	DWORD		dwVideoPort;//!协议端口
	char	    szDomain[128];// 域名/IPv4/IPv6，登陆访问用,当为IP时应跟szIP同
	char	    szDomainAux[128];// 域名/IPv4/IPv6 当RTSP时有效
	char        szIP[40]; // 设备IP,可用于修改IP
	BYTE        byNetMask;// 24->255.255.255.0;16->255.255.0.0
	BYTE        byPhyIdx; // 0- eth0,1-eth1
	BYTE        byRes[2];
	char        szGateway[40];
	char        szDns1[40];
	char        szDns2[40];
	char		szDeviceName[64 + 1];
	char        szUserName[64 + 1];
	char        szPassword[64 + 1];
	char		szDescription[64 + 1];
	BYTE		byRes2[128];
}ANTS_DVR_DEVICE_CHANINFO,*LPANTS_DVR_DEVICE_CHANINFO;

//multicast
typedef struct _tagANTS_DVR_MULTICAST_CHANCFG
{
    BYTE    byEnableMulticast; //! 0-关闭组播 1-开启组播
    BYTE    byTTL;
	BYTE    byTTL_aux;
	BYTE    byTTL_audio;
	char szMulicastIPv4[16];
	unsigned short wMulicastPort;

	char szMulicastIPv4_aux[16];
	unsigned short wMulicastPort_aux;
	
	char szMulicastIPv4_audio[16];
	unsigned short wMulicastPort_audio;
	
	char szMulicastIPv4_audioaux[16];
	unsigned short wMulicastPort_audioaux;
	BYTE    byTTL_audioaux;
    
	BYTE	byRes[17];
}ANTS_DVR_MULTICAST_CHANCFG, *LPANTS_DVR_MULTICAST_CHANCFG;

//!流媒体服务器基本配置
typedef struct {
	BOOL bEnableManagerHost;
	char szManagerHost[128];
	WORD wManagerHostPort;
	BYTE byRes[2];
}ANTS_DVR_NET_MANAGERHOSTV2,*LPANTS_DVR_NET_MANAGERHOSTV2;




#define ANTSMID_3G_DEVICE_DESC_LEN				32
#define ANTSMID_3G_DEVICE_NUM					200
#define ANTSMID_DDNS_SERVICE_DESC_LEN			32
#define ANTSMID_DDNS_SERVICE_NUM				200
#define ANTSMID_MAX_MANAGERHOST_NUM			2


#define ANTSMID_WIFI_ESSID_MAX_SIZE					32
#define ANTSMID_WIFI_WEP_MAX_KEY_COUNT				4
#define ANTSMID_WIFI_WEP_MAX_KEY_LENGTH				33
#define ANTSMID_WIFI_WPA_PSK_MAX_KEY_LENGTH			63
#define ANTSMID_WIFI_WPA_PSK_MIN_KEY_LENGTH			8
#define ANTSMID_WIFI_MAX_AP_COUNT					20

#define ANTSMID_WIFI_MACADDR_LEN						6


typedef struct {
	DWORD dwSize;
	ANTS_DVR_NET_MANAGERHOSTV2 struManagerHostSet[ANTSMID_MAX_MANAGERHOST_NUM];
	BYTE byRes[128];
}ANTS_DVR_NET_MANAGERHOST_CFG,*LPANTS_DVR_NET_MANAGERHOST_CFG;


typedef struct {	
	char sIpAddress[16];/*IP地址*/
	char sIpMask[16];/*掩码*/	
	BYTE byMACAddr[ANTSMID_WIFI_MACADDR_LEN];/*物理地址，只用来显示*/
	BYTE bRes[2];
	DWORD dwEnableDhcp;/*是否启动dhcp  0不启动 1启动*/
	DWORD dwAutoDns;/*如果启动dhcp是否自动获取dns,0不自动获取 1自动获取；对于有线如果启动dhcp目前自动获取dns*/	
	char sFirstDns[16];/*第一个dns域名*/
	char sSecondDns[16];/*第二个dns域名*/
	char sGatewayIpAddr[16];/* 网关地址*/
	BYTE bRes2[8];
}ANTS_DVR_NET_WIFIETHERNET,*LPANTS_DVR_NET_WIFIETHERNET;


typedef struct 
{
	/*wifi网口*/
	ANTS_DVR_NET_WIFIETHERNET struEtherNet;
	/*SSID*/
	char sEssid[ANTSMID_WIFI_ESSID_MAX_SIZE];
	/* 0 mange 模式;1 ad-hoc模式，参见*/
	DWORD dwMode;
	/*0 不加密；1 wep加密；2 wpa-psk; */
	DWORD dwSecurity;
	union
	{
		struct _tagkey
		{
			/*0 -开放式 1-共享式*/
			DWORD dwAuthentication;
			/* 0 -64位；1- 128位；2-152位*/
			DWORD dwKeyLength;
			/*0 16进制;1 ASCI */
			DWORD dwKeyType;
			/*0 索引：0---3表示用哪一个密钥*/
			DWORD dwActive;
			char sKeyInfo[ANTSMID_WIFI_WEP_MAX_KEY_COUNT][ANTSMID_WIFI_WEP_MAX_KEY_LENGTH];
		}wep;
		struct 
		{
			/*8-63个ASCII字符*/
			DWORD dwKeyLength;
			char sKeyInfo[ANTSMID_WIFI_WPA_PSK_MAX_KEY_LENGTH];
			char sRes;
		}wpa_psk;
	}key;
	
} ANTS_DVR_NET_WIFI_CFG_EX,*LPANTS_DVR_NET_WIFI_CFG_EX;


//!wifi配置结构
typedef struct 
{
	DWORD dwSize;
	ANTS_DVR_NET_WIFI_CFG_EX struWifiCfg;
}ANTS_DVR_NET_WIFI_CFG,*LPANTS_DVR_NET_WIFI_CFG;

//!wifi工作模式
typedef struct {
	DWORD dwSize;
	DWORD dwNetworkInterfaceMode; /*0 自动切换模式　1 有线模式*/
}ANTS_DVR_NET_WIFI_WORKMODE,*LPANTS_DVR_NET_WIFI_WORKMODE;

//!3g
typedef struct {
	DWORD dwSize;
	BOOL bEnable;//!是否启用3G上网功能
	char szAPNAddr[32];//!APN地址
	char szTelePhone[32];//!拔号号码
	char szIPAddr[32];//!3G IP地址
	DWORD dwWorkMode;/*!
				3G工作模式
				00-表示与ADSL网络并行工作
				01-表示ADSL网络断开030秒后工作
				02-表示ADSL网络断开035秒后工作
				03-表示ADSL网络断开040秒后工作
				04-表示ADSL网络断开045秒后工作
				05-表示ADSL网络断开050秒后工作
				06-表示ADSL网络断开055秒后工作
				07-表示ADSL网络断开060秒后工作
				08-表示ADSL网络断开065秒后工作
				09-表示ADSL网络断开070秒后工作
				10-表示ADSL网络断开075秒后工作
				11-表示ADSL网络断开080秒后工作
				12-表示ADSL网络断开085秒后工作
				13-表示ADSL网络断开090秒后工作
				14-表示ADSL网络断开095秒后工作					
				*/
	DWORD dwDeviceType;//!设备类型
				/*
				0-ZTE MF100 WCDMA
				1-HUAWEI E156G WCDMA
				2-VITION E1916 CDMA2000
				*/
	BYTE byRes[128];	
}ANTS_DVR_NET_3G_CFG,*LPANTS_DVR_NET_3G_CFG;


typedef struct {
	char  sSsid[ANTSMID_WIFI_ESSID_MAX_SIZE];
	DWORD dwMode;/* 0 mange 模式;1 ad-hoc模式，参见NICMODE */
	DWORD dwSecurity;/*0 不加密；1 wep加密；2 wpa-psk;3 wpa-Enterprise，参见WIFISECURITY*/
	DWORD dwChannel;/*1-11表示11个通道*/
	DWORD dwSignalStrength;/*0-100信号由最弱变为最强*/
	DWORD dwSpeed;/*速率,单位是0.01mbps*/
}ANTS_DVR_NET_AP_INFO,*LPANTS_DVR_NET_AP_INFO;

typedef struct {
	DWORD dwSize;
	DWORD dwCount;/*无线AP数量，不超过20*/
	ANTS_DVR_NET_AP_INFO struApInfo[ANTSMID_WIFI_MAX_AP_COUNT];
}ANTS_DVR_NET_AP_INFO_LIST,*LPANTS_DVR_NET_AP_INFO_LIST;

//!3G上网卡类型及描述
typedef struct {
	DWORD dwType;//!3G类型值
	BYTE byDescribe[ANTSMID_3G_DEVICE_DESC_LEN];
	BYTE byISPDescribe[16];
}ANTS_DVR_NET_3G_DEVICE,*LPANTS_DVR_NET_3G_DEVICE;

typedef struct {
	DWORD dwSize;
	DWORD dw3GDevNum;
	ANTS_DVR_NET_3G_DEVICE stru3GSet[ANTSMID_3G_DEVICE_NUM];
	BYTE byRes[256];
}ANTS_DVR_NET_3GDEVICE_CFG,*LPANTS_DVR_NET_3GDEVICE_CFG;

//!DDNS 服务名称
typedef struct {
	DWORD dwIndex;//!DDNS索引值
	BYTE byDescribe[ANTSMID_DDNS_SERVICE_DESC_LEN];
	BYTE byServerName[64];
	WORD wServerPort;
	WORD wRes;
}ANTS_DVR_NET_DDNS_SERVICE,*LPANTS_DVR_NET_DDNS_SERVICE;

typedef struct {
	DWORD dwSize;
	DWORD dwDdnsServiceNum;
	ANTS_DVR_NET_DDNS_SERVICE struDdnsServices[ANTSMID_DDNS_SERVICE_NUM];
	BYTE byRes[256];
}ANTS_DVR_NET_DDNSSERVICE_ABILITY,*LPANTS_DVR_NET_DDNSSERVICE_ABILITY;


typedef struct _tagANTS_DVR_ROUTER_CFG
{
	BYTE bySupported;// 是否支持,0-不支持,1-支持
	BYTE byEnable; // 0- 不启用
	BYTE byNetMode; // 0- static,1-dhcp,2-ppoe
	
	BYTE byMac[6];
	
	char sIPv4[16];
	char sMaskIPv4[16];
	char sGatewayIPv4[16];
	char sDns1IPv4[16];
	char sDns2IPv4[16];
	
	char sPPPoeUser[32];
	char sPPPoePassword[32];
	BYTE byRes[128];
}ANTS_DVR_ROUTER_CFG,*LPANTS_DVR_ROUTER_CFG;



//本地硬盘信息配置(子结构)
typedef struct
{
    DWORD						dwHDNo;				/*硬盘号, 取值0~MAX_DISKNUM-1*/
    DWORD						dwCapacity;			/*硬盘容量(不可设置)*/
    DWORD						dwFreeSpace;		/*硬盘剩余空间(不可设置)*/
    DWORD						dwHdStatus;			/*硬盘状态(不可设置) HD_STAT,  bit0: 0- 正常,1-异常；bit1:1-当前写的盘*/
    BYTE						byHDAttr;			/*0-默认, 1-冗余; 2-只读*/
	BYTE						byHDType;			/*0-本地硬盘,1-ESATA硬盘,2-NAS硬盘,3-iSCSI硬盘 4-Array虚拟磁盘,5-SD */
	BYTE						byRes1[2];          // byRes1[0] - SD卡序号,byRes1[1] -SD卡分区号
    DWORD						dwHdGroup;			/*属于哪个盘组 1-MAX_HD_GROUP*/
	BYTE                        byPhyPos[3];        // 对SATA ,[0] - 第几个主SATA口，[1]-第几个扩展SATA口,[2] -再一次扩展,以0开始
    BYTE						byRes2[117];
}ANTS_DVR_SINGLE_HD, *LPANTS_DVR_SINGLE_HD;

typedef struct
{
    DWORD						dwSize;
    DWORD						dwHDCount;					/*硬盘数(不可设置)*/
    ANTS_DVR_SINGLE_HD			struHDInfo[MAX_DISKNUM];	//硬盘相关操作都需要重启才能生效；
}ANTS_DVR_HDCFG, *LPANTS_DVR_HDCFG;

typedef struct
{
    DWORD						dwSize;
    DWORD						dwHDCount;					/*硬盘数(不可设置)*/
    ANTS_DVR_SINGLE_HD			struHDInfo[MAX_DISKNUM_EX];	//硬盘相关操作都需要重启才能生效；
}ANTS_DVR_HDCFG_V2, *LPANTS_DVR_HDCFG_V2;

//本地盘组信息配置
typedef struct
{
    DWORD						dwHDGroupNo;						/*盘组号(不可设置) 1-MAX_HD_GROUP*/        
    BYTE						byHDGroupChans[MAX_CHANNUM_EX];	/*盘组对应的录像通道, 0-表示该通道不录象到该盘组，1-表示录象到该盘组*/
    BYTE						byRes[8];
}ANTS_DVR_SINGLE_HDGROUP_EX, *LPANTS_DVR_SINGLE_HDGROUP_EX;

//网络端结构体
typedef struct{
	char		szDVRIP[16];
	DWORD		dwDVRPort;
	DWORD		dwChannel;
	DWORD		dwTransProtocol;
	DWORD		dwTransMode;
	DWORD		dwLinkProtocol;	///0-TCP;1-UDP
	DWORD		dwEXMode;		///是否启用增强连接模式
	char		szUserName[NAME_LEN];
	char		szPassword[PASSWD_LEN];
	char		szRtspMain[PATHNAME_LEN];
	char		szRtspAux[PATHNAME_LEN];
	char		szDescription[16];		///
}ANTS_DVR_NET_DECINFO, *LPANTS_DVR_NET_DECINFO;

typedef struct{
	DWORD						dwSize;
	ANTS_DVR_NET_DECINFO		struDecChanInfo[MAX_NETDECNUM_EX];
}ANTS_DVR_NET_DYNAMIC_DEC_EX, *LPANTS_DVR_NET_DYNAMIC_DEC_EX;

//!获取NVR/DVR/IPC通道名称，如果在获取参数接口中传入buffer size为多个结构，则可以同时获取到多个通道的通道名
typedef struct{
	DWORD dwSize;
	char szChanName[NAME_LEN];
	BYTE bEnable;
	BYTE byRes[3];
}ANTS_DVR_DEVCHANNELNAME_CFG,*LPANTS_DVR_DEVCHANNELNAME_CFG;

typedef struct{
	LONG lDayNightMode;// -1--不支持;0--外部红外控制;1--自动模式;2--强制白天;3--强制黑夜
	LONG lDelay ;// 自动转换延迟，自动模式有效。0-30
	LONG lNighttoDayThreshold ;// 自动转换黑夜到白天的阈值0-255，默认0xEE
	LONG lDaytoNightThreshold ;// 自动转换白天到黑夜的阈值0-255，默认0x57
}ANTS_DVR_SENSOR_DAYNIGHTMODE,*LPANTS_DVR_SENSOR_DAYNIGHTMODE;

typedef struct
{
	DWORD dwSize;
	DWORD dwValidMask;// 相应位0-无效，1-有效; 
	// bit0 - DayNightMode,bit1-lMinorMode,bit2-lGainMode,bit3-lAntiflickerMode,
	// bit4-lPicQualityMode,bit5-lWBMode   ,bit6-lBacklightMode,bit7-lShutterMode
	// bit8-lIrisMode         ,bit9-lSharpnessMode,bit10-l3DNRMode,bit11-3DNRTfode,
	// bit12-WDRMode,bit13-GammaMode,bit14-AntiflickerFreqMode,bit15-byMDICameraType,bit16-byMDICameraAutoConfigEnable
	// bit17-byAuxSupported bit18-WDRType
	ANTS_DVR_SENSOR_DAYNIGHTMODE DayNightMode;
	LONG lMinorMode ;//镜像 -1--不支持;0--正常;1--水平翻转;2--垂直翻转;3--180°翻转;  4--90度旋转； 5 --270度旋转 ;
	LONG lGainMode ;//增益 -1--不支持;0--低;1--较低;2--中3--较高;4--高;
	LONG lAntiflickerMode ;//抗闪 -1--不支持;0--关;1--开
	LONG lPicQualityMode ;//图像效果 -1--不支持;0--正常;1--艳丽;2--自然
	LONG lWBMode ;//白平衡 -1--不支持;0--自动白平衡;1--室内模式;2--室外模式; 3--ATW
	LONG lBacklightMode;// 背光补偿模式-1--不支持; 0-OFF 1-LOW 2-MID 3-HIGH
	LONG lShutterMode;// 快门模式 -1--不支持; 0--自动快门,手动增益;
				/*其他模式:0x1~0x7f:快门优先,手动快门,自动增益
				0x01:1/30(1/25), 
				0x02:1/60(1/50), 
				0x03:1/120(1/100), 
				0x04:1/200,
				0x05:1/250, 
				0x06:1/500, 
				0x07:1/1000, 
				0x08:1/2000, 
				0x09:1/5000, 
				0x0A:1/10000, 
				0x0B:1/50000, 
				0x0C:x2, 
				0x0D:x4, 
				0x0E:x6, 
				0x0F:x8, 
				0x10:x10, 
				0x11:x15, 
				0x12:x20,
				0x13:x25,
				0x14:x30 
				0x15:1/15000
				0x16:1/20000
				0x17:1/25000
				0x18:1/30000
				0x19:1/40000
				0x1A:1/50000
				0x100:全自动,快门自动,增益自动(最大)
				0x101~:全手动,快门手动,增益手动(增益值为lGainMode)
				0x200:光圈优先
				0x300~0x3ff:不解析快门时间,对应界面上value:0~0xff,透传到机芯的快门值
				*/
	LONG lIrisMode;// 镜头光圈模式:-1--不支持; 0--自动光圈;1--手动或固定光圈
	LONG lSharpnessMode;// 锐度模式: -1--不支持;0--关闭;1--打开
	LONG lSharpnessLevel;// 0-255
	LONG l3DNRMode;// 3D降噪模式 : -1--不支持;0--关闭;1--打开
	LONG l3DNRLevel;// 0-100
	LONG l3DNRTfode; // 3D降噪时域-1--不支持 0--关闭;1--低;2--中;3--较高;4--高
	LONG lWDMode; // 宽动态模式-1--不支持0--关闭;1--低;2--中;3--高
	LONG lGammaMode;// Gamma模式-1--不支持 0--Curve_1_6;1--Curve_1_8;2--Curve_2_0;3--Curve_2_2
	LONG lAntiflickerFreqMode; // 抗闪模式	-1--不支持;0--自动;1--50HZ;2--60HZ
	BYTE byMDICameraType;// MDI摄像机类型, 仅获取,0-NONE,1-LMOP72A34,2-LMOV72063,3-LMOV72063IR,-1-不支持
	BYTE byMDICameraAutoConfigEnable; // 是否开启摄像机自动配置,-1不支持
	BYTE byAuxSupported; //  bit0:是否支持坏点检测(0-不支持,1-支持);bit1:支持校正光圈;bit2:支持远程重启;bit3-是否支持恢复默认
	//BYTE byExpoMode; // 曝光模式 -1-不支持 0-全自动 1-手动 2-快门优先 3-光圈优先 4-自动快门(增益可变)
	//LONG lShutterTime; // 快门时间 正数N代表N秒,负数(-N)代表1/N秒,0-未定义
	BYTE byZoomSpeed; //变倍速度 0-低,1-中,2-高
	BYTE byFocusSpeed; //变焦速度 0-低,1-中,2-高
	BYTE byShowZoomRate; //显示变倍 0-隐藏,1-显示
	BYTE byShowCoordinate; //显示坐标 0-隐藏,1-显示
	BYTE byShowStatus; //显示状态 0-隐藏,1-显示
	BYTE byLightCorrectMode; // 背光模式 0-关闭 1-宽动态 2-强光抑制 3-背光补偿
	BYTE byLightCorrectLevel; // 背光模式校正强度 低-64 中-128 高-192
	BYTE byDarkCompensation; // 暗区补偿 0-关闭 1-低 2-中 3-高
	BYTE byDnrMode; // 数字降噪 0-关闭 1-低 2-中 3-高
	BYTE byElecAntiQuake; // 电子防抖 0-关闭 1-开启
	BYTE byAutoSlowShutter; // 自动慢快门 0-关闭 N-N秒
	BYTE byDefog; // 去雾功能 0-关闭 1-开启
	BYTE byDefogLv; // 去雾等级
	BYTE by3DNRTfodeNight;// 夜晚3D降噪时域-ff--不支持 0--关闭;1--低;2--中;3--较高;4--高
	BYTE byExtInTrig; // 日夜模式为外部触发时信号的输入电平为高或低时触发切换. 0-默认,1-低电平,2-高电平
	BYTE byIcrOutTrig; // 红外滤光片的触发方式. 0-正向,1-反向
	BYTE byWdrType;
	BYTE byAutoLensMode; //=1可以自动聚焦
	BYTE byRes[3];
}ANTS_DVR_SENSOREX_CFG,*LPANTS_DVR_SENSOREX_CFG;




typedef struct {
	BOOL bEnableManagerHost;
	DWORD dwType; // 类型,对应于管理主机类型列表索引
	DWORD dwFixed;// 0-可修改，1-不可修改
	char szRes[128]; // 注册主机IP或域名地址, 不使用
	DWORD dwRes; // 管理主机主端口,不使用
	char szOtherItemValues[8][32];
}ANTS_DVR_MANAGERHOST,*LPANTS_DVR_MANAGERHOST;

typedef struct {
	ANTS_DVR_MANAGERHOST HostArray[8];
}ANTS_DVR_MANAGERHOSTS_CFG,*LPANTS_DVR_MANAGERHOSTS_CFG;

typedef struct{
  BYTE     byIPID;
  BYTE     byAlarmIn;
  BYTE     byRes[18];
}ANTS_DVR_IPALARMININFO, *LPANTS_DVR_IPALARMININFO;

typedef struct{
  DWORD                      dwSize;
  ANTS_DVR_IPALARMININFO      struIPAlarmInInfo[MAX_IP_ALARMIN];
}ANTS_DVR_IPALARMINCFG, *LPANTS_DVR_IPALARMINCFG;

typedef struct{
  BYTE     byIPID;
  BYTE     byAlarmOut;
  BYTE     byRes[18];
}ANTS_DVR_IPALARMOUTINFO, *LPANTS_DVR_IPALARMOUTINFO;

typedef struct{
  DWORD                      dwSize;
  ANTS_DVR_IPALARMOUTINFO     struIPAlarmOutInfo[MAX_IP_ALARMOUT];
}ANTS_DVR_IPALARMOUTCFG, *LPANTS_DVR_IPALARMOUTCFG;

typedef struct
{
    char szName[48];
	char szValue[256];
}ANTS_DVR_FACTORY_INFO,*LPANTS_DVR_FACTORY_INFO;

typedef struct _tagANTS_DVR_REMOTE_ADJUSTTIME
{
	DWORD dwType; // 0- 不对时,1-定时对时，2-开机对时(登陆对时),4-掉线重连对时,可组合
	DWORD dwIntervalMin; // 间隔对时,分钟,0-不间隔
	DWORD dwHMS; // 定时间,bit0~7 秒，bit8~15分,bit16~时
	DWORD dwRes[5];
}ANTS_DVR_REMOTE_ADJUSTTIME,*LPANTS_DVR_REMOTE_ADJUSTTIME;

typedef struct tag_DISP_RECT
{
    int x;
    int y;
    int uWidth;                           
    int uHeight;
}DISPRECT;

typedef struct
{
	DWORD dwWidth;
	DWORD dwHeight;
}ANTS_DVR_DESC_NODE_RESOLVE;
typedef struct
{
	int							iValue;
	BYTE						byDescribe[DESC_LEN];
	union
	{
		ANTS_DVR_DESC_NODE_RESOLVE tResolve; // 用于分辨率能力
	  	BYTE					   byRes[16];
	}uInfo;
}ANTS_DVR_DESC_NODE, *LPANTS_DVR_DESC_NODE;

typedef struct
{
	DWORD						dwAbilityType;
	BYTE						byRes[32];
	DWORD						dwNodeNum;
	ANTS_DVR_DESC_NODE			struDescNode[MAX_NODE_NUM];
}ANTS_DVR_ABILITY_LIST, *LPANTS_DVR_ABILITY_LIST;

typedef struct
{
	DWORD						dwSize;
	DWORD						dwAbilityNum;
	ANTS_DVR_ABILITY_LIST		struAbilityNode[MAX_ABILITYTYPE_NUM_V2];
}ANTS_DVR_COMPRESSIONCFG_ABILITY_V2, *LPANTS_DVR_COMPRESSIONCFG_ABILITY_V2;


typedef struct
{
	DWORD						dwType;
	BYTE						byDescribe[DESC_LEN];
}ANTS_DVR_PTZ_PROTOCOL, *LPANTS_DVR_PTZ_PROTOCOL;

typedef struct
{
	DWORD						dwSize;
	ANTS_DVR_PTZ_PROTOCOL		struPtz[PTZ_PROTOCOL_NUM];
	DWORD						dwPtzNum;
	BYTE						byRes[8];
}ANTS_DVR_PTZCFG, *LPANTS_DVR_PTZCFG;

#define MAX_RESOLUTIONNUM    64 //支持的最大分辨率数目


typedef struct{
	DWORD   dwSize;
	BYTE    byDecNums;
	BYTE    byStartChan;
	BYTE    byVGANums;
	BYTE    byBNCNums;
	BYTE    byVGAWindowMode[8][12];
	BYTE    byBNCWindowMode[4];
	BYTE    byDspNums;
	BYTE    byHDMINums;
	BYTE    byDVINums;
	BYTE    byRes1[13];
	BYTE    bySupportResolution[MAX_RESOLUTIONNUM];
	BYTE    byHDMIWindowMode[4][8];
	BYTE    byDVIWindowMode[4][8];
	BYTE    byRes2[24];
}ANTS_DVR_MATRIX_ABILITY,*LPANTS_DVR_MATRIX_ABILITY;

typedef struct _tagANTS_DVR_DEVICESUPPORT_REQUIRE
{
	DWORD dwStartChan;// 通道编号从 0开始，通道无关的忽略,起始通道号
	DWORD dwChanNum;// 通道个数
	DWORD dwRes[14];// 0
}ANTS_DVR_DEVICESUPPORT_REQUIRE;

typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_NET
{
	DWORD bMain_info:2;
	DWORD bMain_ddns:2;
	DWORD bMain_ntp:2;
	DWORD bMain_email:2;
	DWORD bMain_platform:2;
	DWORD bMain_multicast:2;
	DWORD bMain_ftp:2;
	DWORD bMain_IPTables:2; // IPTables 端口映射。
	DWORD bMain_other:16;


	    // 1
	DWORD bInfo_wifi:2;
	DWORD bInfo_3G:2;
	DWORD bInfo_Advance:2;
	DWORD bInfo_Advance_Alarm:2;
	DWORD bInfo_Advance_ManHost1:2;
	DWORD bInfo_Advance_ManHost2:2;
	DWORD bInfo_Advance_PPPoe:2;
	DWORD bInfo_Advance_Upnp:2;

	DWORD bInfo_Nic:2;
	DWORD bInfo_IP:2;
	DWORD bInfo_SubMask:2;
	DWORD bInfo_DefaultGateway:2;
	DWORD bInfo_Dns1:2;
	DWORD bInfo_Dns2:2;
	DWORD bInfo_Mac:2;
	DWORD bInfo_DefaultRoute:2;
	// 2

	DWORD bInfo_RemotePort:2;
	DWORD bInfo_HttpPort:2;
	DWORD bInfo_MultiCast:2;
	DWORD bInfo_Route_Enable:2;// 隐藏勾选框
	DWORD bInfo_Route:2;	// 隐藏整个功能
	DWORD bInfo_Route_DHCP_DNS1:2; // 隐藏DNS
	DWORD bInfo_Route_DHCP_DNS2:2; // 隐藏DNS
	DWORD bInfo_Route_PPPOE_DNS1:2; // 隐藏DNS
	DWORD bInfo_Route_PPPOE_DNS2:2; // 隐藏DNS
	DWORD bInfo_RtspPort:2; // 隐藏RTSP
	DWORD bInfo_RtmpPort:2; // 隐藏RTMP
	DWORD bInfo_Other:10;


}ANTS_DVR_DEVICESUPPORT_HIDE_NET;


typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_DEVICE
{
	DWORD bMain_info:2;
	DWORD bMain_version:2;
	DWORD bMain_ptz:2;
	DWORD bMain_dst:2;
	DWORD bMain_QRCode:2;
	DWORD bMain_other:22;

	DWORD bDeviceInfo_DeviceName:2;
	DWORD bDeviceInfo_DeviceID:2;
	DWORD bDeviceInfo_RecordReplace:2;
	DWORD bDeviceInfo_RecordTime:2;
	DWORD bDeviceInfo_PannelVersion:2;
	DWORD bDeviceInfo_RS485:2;
	DWORD bDeviceInfo_DeviceSerialNum:2;
	DWORD bDeviceInfo_ChannelNum:2;
	DWORD bDeviceInfo_AlarmInputNum:2;
	DWORD bDeviceInfo_AlarmOuputNum:2;
	DWORD bDeviceInfo_HddNum:2;
	DWORD bDeviceInfo_VideoFormate:2;
	DWORD bDeviceInfo_StreamType:2;
	DWORD bDeviceInfo_Protocol:2;
	DWORD bDeviceInfo_TimeFormat:2;
	DWORD bDeviceInfo_DateFormat:2;

	// 2
	DWORD bDeviceInfo_HdmiAudioOut:2;
	DWORD bDeviceInfo_AuxRecord:2; // 定制是否支持子码流录像
	DWORD bDeviceInfo_ZeroChannel:2;// 零通道	
	DWORD bDeviceInfo_AudioSource:2;
	DWORD bDeviceInfo_Support50Fps:2;
	DWORD bDeviceInfo_Support60Fps:2;
	DWORD bDeviceInfo_other:20;
    // 3
	DWORD bSystemVersion_MasterVersion:2;
	DWORD bSystemVersion_HardwareVersion:2;
	DWORD bDeviceInfo_ForCnbUI:2;
	DWORD bDeviceInfo_Comment:2; // 波兰,注释
	DWORD bSystemVersion_other:24;
    // 4
	DWORD bPtzSetting_Channel:2;
	DWORD bPtzSetting_BitsPerSec:2;
	DWORD bPtzSetting_DataBits:2;
	DWORD bPtzSetting_StopBits:2;
	DWORD bPtzSetting_Parity:2;
	DWORD bPtzSetting_FlowControl:2;
	DWORD bPtzSetting_Protocol:2;
	DWORD bPtzSetting_PTZAdress:2;
	DWORD bPtzSetting_other:16;
    // 5
	DWORD bDstSetting_Enable:2;
	DWORD bDstSetting_From:2;
	DWORD bDstSetting_To:2;
	DWORD bDstSetting_DSTBias:2;
	DWORD bDstSetting_other:24;
}ANTS_DVR_DEVICESUPPORT_HIDE_DEVICE;

typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_CHANNEL
{
	// dwRes[0]
	DWORD bMain_Display:2;
	DWORD bMain_VideoParam:2;
	DWORD bMain_Schedule:2;
	DWORD bMain_Motion:2;
	DWORD bMain_VideoLost:2;
	DWORD bMain_TamperAlarm:2;
	DWORD bMain_Mask:2;
	DWORD bMain_Sensor:2;
	DWORD bMain_LanSearch:2; // new IE 局域网搜索
	DWORD bMain_CountWire:2;
	DWORD bMain_Detectwire:2;
	DWORD bMain_DetectRegion:2;
	DWORD bMain_ObjectRegion:2;	
	DWORD bMain_FaceDetect:2;
	DWORD bMain_FireDetect:2;
	DWORD bMain_VideoDiagnose:2;

	// dwRes[1]
	DWORD bDisplay_Sensor:2;  // 隐藏整个SENSOR
	DWORD bDisplay_Sensor_BadPixelTest:2;  // 
	DWORD bDisplay_Sensor_Reboot:2;  // 
	DWORD bDisplay_Sensor_IrisAdjust:2;  //  光圈校正
	
	DWORD bDisplay_Sensor_DayNight:2;  // 
	DWORD bDisplay_Sensor_Gain:2;  // 
	DWORD bDisplay_Sensor_Minor:2;  // 
	DWORD bDisplay_Sensor_Antiflicker:2;  // 
	
	DWORD bDisplay_Sensor_AntiflickerFreq:2;  // 
	DWORD bDisplay_Sensor_PicQuality:2;  //
	DWORD bDisplay_Sensor_WB:2;  // 
	DWORD bDisplay_Sensor_Backlight:2;  // 
	
	DWORD bDisplay_Sensor_Shutter:2;  // 
	DWORD bDisplay_Sensor_Iris:2;  // 
	DWORD bDisplay_Sensor_Sharpness:2;  // 
	DWORD bDisplay_Sensor_3DNR:2;  // 
	// 2
	DWORD bDisplay_Sensor_3DNRTfode:2;  // 
	DWORD bDisplay_Sensor_WD:2;  // 
	DWORD bDisplay_Sensor_Gamma:2;  // 
	DWORD bDisplay_Sensor_MDICameraType:2;  //
	DWORD bDisplay_Sensor_MDICameraAutoConfigEnable:2;  //
	DWORD bDisplay_Sensor_BadPixelDetect:2;  // 坏点检测
	DWORD bDisplay_Sensor_IrisCal:2;  // 校正光圈
	DWORD bDisplay_Sensor_RemoteReboot:2;  //远程重启
	DWORD bDisplay_Sensor_ZoomSpeed:2;  // 变倍速度
	DWORD bDisplay_Sensor_FocusSpeed:2;  // 变焦速度
	DWORD bDisplay_Sensor_ShowZoomRate:2;  // 显示变倍
	DWORD bDisplay_Sensor_ShowCoordinate:2;  // 显示坐标
	DWORD bDisplay_Sensor_ShowStatus:2;  // 显示状态
	DWORD bDisplay_Sensor_LightCorrectMode:2;  // 背光模式
	DWORD bDisplay_Sensor_LightCorrectLevel:2;  //背光水平
	DWORD bDisplay_Sensor_DarkCompensation:2;  //暗区补偿
	
	// dwRes[3]
	DWORD bDisplay_Channel:2;
	DWORD bDisplay_ShowLocalName:2;
	DWORD bDisplay_LocalCameraName:2;
	DWORD bDisplay_ShowName:2;
	DWORD bDisplay_CameraName:2;
	DWORD bDisplay_ShowDate:2;
	DWORD bDisplay_TimeFormat:2;
	DWORD bDisplay_DateFormat:2;
	DWORD bDisplay_OSDPosition:2;
	DWORD bDisplay_ImageSetting:2;
	DWORD bDisplay_SetMutliOsd:2;	
	DWORD bDisplay_MainOsdSize:2;
	DWORD bDisplay_AuxOsdSize:2;
	DWORD bDisplay_ImageSetting_other:6;	
	
	//dwres[4]
	DWORD bVideoparam_Channel:2;
	DWORD bVideoparam_EcodingType:2;
	DWORD bVideoparam_StreamType:2;
	DWORD bVideoparam_Resolution:2;
	DWORD bVideoparam_BitrateType:2;
	DWORD bVideoparam_Bitrate:2;
	DWORD bVideoparam_FrameRate:2;
	DWORD bVideoparam_Quality:2;
	DWORD bVideoparam_IFrameInteral:2;
	DWORD bVideoparam_other:14;
	//dwres[5]
	DWORD bScheduleRecord_Channel:2;
	DWORD bScheduleRecord_EnableEcoding:2;
	DWORD bScheduleRecord_AllDayRecord:2;
	DWORD bScheduleRecord_RecordTime:2;
	DWORD bScheduleRecord_Advanced:2;
	DWORD bScheduleRecord_other:22;
	//dwres[6]
	DWORD bMotion_Channel:2;
	DWORD bMotion_EnableMotion:2;
	DWORD bMotion_AreaSetting:2;
	DWORD bMotion_Sensitivity:2;
	DWORD bMotion_AlarmSchedule:2;
	DWORD bMotion_Linkage:2;
	DWORD bMotion_Output:2;
	DWORD bMotion_UploadEmail:2;
	DWORD bMotion_AllLinkAge:2;
	DWORD bMotion_other:14;
	
	//dwres[7]
	DWORD bVideoLost_Channel:2;
	DWORD bVideoLost_EnableVideoLost:2;
	DWORD bVideoLost_AlarmSchedule:2;
	DWORD bVideoLost_Linkage:2;
	DWORD bVideoLost_other:24;
	//dwres[8]
	DWORD bTamperingAlarm_Channel:2;
	DWORD bTamperingAlarm_EnableVideoTampering:2;
	DWORD bTamperingAlarm_AreaSetting:2;
	DWORD bTamperingAlarm_ArmScedule:2;
	DWORD bTamperingAlarm_Lingkage:2;
    DWORD bTamperingAlarm_Output:2;		
	DWORD bTamperingAlarm_UploadEmail:2;
	DWORD bTamperingAlarm_AllLinkAge:2;
	DWORD bTamperingAlarm_other:16;
	//dwres[9]
	DWORD bVideoMask_Channel:2;
	DWORD bVideoMask_EnableVideoMask:2;
	DWORD bVideoMask_AreaSetting:2;
	DWORD bVideoMask_ImageSetting_other:26;
    // dwres[10]
	DWORD bDisplay_Sensor_DnrMode:2;  //数字降噪 
	DWORD bDisplay_Sensor_ElecAntiQuake:2;  //电子防抖
	DWORD bDisplay_Sensor_AutoSlowShutter:2;  //自动慢快门 
	DWORD bDisplay_Sensor_Defog:2;  //去雾功能
	DWORD bDisplay_Sensor_DefogLv:2;  //去雾等级
	DWORD bDisplay_Sensor_3DNRTfodeNight:2;  //夜晚3D降噪时域
	DWORD bDisplay_Sensor_ExtInTrig:2;  //日夜模式外部触发切换
	DWORD bDisplay_Sensor_IcrOutTrig:2;  //红外滤光片的触发方式
	DWORD bDisplay_Sensor_WdrType:2;
	DWORD bDisplay_Sensor_AutoLens:2;
	DWORD bDisplay_Sensor_Minor_90_270:2;
	DWORD bDisplay_Sensor_other:10;  //红外滤光片的触发方式

	//dwRes[11]
	DWORD bSmart_CountWire_UploadEmail:2; // 计数线
	DWORD bSmart_CountWire_ArmScedule:2;
	DWORD bSmart_CountWire_Output:2;
	DWORD bSmart_CountWire_PtzLinkAge:2;
	DWORD bSmart_DetectWire_UploadEmail:2; // 虚拟警戒线
	DWORD bSmart_DetectWire_ArmScedule:2;
	DWORD bSmart_DetectWire_Output:2;
	DWORD bSmart_DetectWire_PtzLinkAge:2;
	DWORD bSmart_DetectRegion_UploadEmail:2; // 区域检测
	DWORD bSmart_DetectRegion_ArmScedule:2;
	DWORD bSmart_DetectRegion_Output:2;
	DWORD bSmart_DetectRegion_PtzLinkAge:2;
	DWORD bSmart_ObjectRegion_UploadEmail:2; // 物品检测 
	DWORD bSmart_ObjectRegion_ArmScedule:2;
	DWORD bSmart_ObjectRegion_Output:2;
	DWORD bSmart_ObjectRegion_PtzLinkAge:2;


	//dwRes[12]
	DWORD bFaceDetect_UploadEmail:2; //人脸检测
	DWORD bFaceDetect_ArmScedule:2;
	DWORD bFaceDetect_Output:2;
	DWORD bFaceDetect_PtzLinkAge:2;
	DWORD bFireDetect_UploadEmail:2;//火焰检测
	DWORD bFireDetect_ArmScedule:2;
	DWORD bFireDetect_Output:2;
	DWORD bFireDetect_PtzLinkAge:2;
	DWORD bVideoDiagnose_UploadEmail:2;//视频诊断
	DWORD bVideoDiagnose_ArmScedule:2;
	DWORD bVideoDiagnose_Output:2;
	DWORD bVideoDiagnose_PtzLinkAge:2;
	DWORD bOther:8;
}ANTS_DVR_DEVICESUPPORT_HIDE_CHANNEL;

typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_SMART_CHANNEL
{
	//dwRes[0]
	DWORD bMain_PlateDetect:2;
	DWORD bMain_Sound:2;
	DWORD bMain_MotionDetect:2;
	DWORD bMain_Other:26;

	//dwRes[1]
	DWORD bPlateDetect_UploadEmail:2;//车牌识别
	DWORD bPlateDetect_ArmScedule:2;
	DWORD bPlateDetect_Output:2;
	DWORD bPlateDetect_PtzLinkAge:2;
	DWORD bSound_UploadEmail:2;//异常声音报警
	DWORD bSound_ArmScedule:2;
	DWORD bSound_Output:2;
	DWORD bSound_PtzLinkAge:2;
	DWORD bMotion_UploadEmail:2;//智能移动侦测
	DWORD bMotion_ArmScedule:2;
	DWORD bMotion_Output:2;
	DWORD bMotion_PtzLinkAge:2;
	DWORD bOther:8;

}ANTS_DVR_DEVICESUPPORT_HIDE_SMART_CHANNEL;


typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_ALARM
{
	DWORD bMain_AlarmInput:2;
	DWORD bMain_AlarmOut:2;
	DWORD bMain_Exception:2;
	DWORD bMain_other:26;

	DWORD bAlarmInput_AlarmSchedule:2;
	DWORD bAlarmInput_Linkage:2;
	DWORD bAlarmInput_Output:2;   
	DWORD bAlarmInput_UploadEmail:2;
	DWORD bAlarmInput_AllLinkAge:2;
	DWORD bAlarmInput_other:22;

	DWORD bException_AlarmSchedule:2;
	DWORD bException_Linkage:2;
	DWORD bException_Output:2;       	
	DWORD bException_other:26;

}ANTS_DVR_DEVICESUPPORT_HIDE_ALARM;


typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_USERMGR
{
	DWORD bMain_UserSet:2;
	DWORD bMain_Online:2;
	DWORD bMain_UserLimit:2;
	DWORD bMain_other:26;

	DWORD bUserLimit_RemotePtz:2;
	DWORD bUserLimit_RemoteRecord:2;
	DWORD bUserLimit_RemotePlay:2;
	DWORD bUserLimit_RemoteSetPara:2;
	DWORD bUserLimit_RemoteCheckStates:2;
	DWORD bUserLimit_RemoteAdvance:2;
	DWORD bUserLimit_RemoteTalking:2;
	DWORD bUserLimit_RemotePreview:2;
	DWORD bUserLimit_RemoteAskAlarm:2;
	DWORD bUserLimit_RemoteLocalOutput:2;
	DWORD bUserLimit_RemoteSerial:2;
	DWORD bUserLimit_RemoteCheckParam:2;
	DWORD bUserLimit_RemoteManageCamera:2;
	DWORD bUserLimit_RemoteShutDown:2;
	DWORD bUserLimit_other:4;
}ANTS_DVR_DEVICESUPPORT_HIDE_USERMGR;




typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_SYSTEM
{
	DWORD bMain_Log:2;
	DWORD bMain_Info:2;
	DWORD bMain_HDD:2;
	DWORD bMain_Update:2;
	
	DWORD bMain_AutoReboot:2;
	DWORD bMain_Recovery:2;
	DWORD bMain_LocalConfig:2;// new IE 端
	DWORD bMain_Recovery_Export:2;
	
	DWORD bMain_Recovery_Import:2;
	DWORD bMain_Recovery_Default:2;
	DWORD bUpdate_IPCUpdate:2;	
	DWORD bLocalSetting_Playback:2;
	DWORD bLocalSetting_BackUp:2;
	DWORD bLocalSetting_FileManage:2;
	DWORD bLocalSetting_Plate:2;
	DWORD bLocalSetting_Face:2;

}ANTS_DVR_DEVICESUPPORT_HIDE_SYSTEM;

typedef struct _tagANTS_DVR_DEVICESUPPORT_HIDE_MAIN
{
	DWORD bMain_PlayBack:2;
	DWORD bMain_BackUp:2;
	DWORD bMain_PTZ:2;
	DWORD bMain_Capture:2;
	DWORD bMain_Alarm:2;
	DWORD bMain_FileManger:2;
	DWORD bMain_Setting:2;
	DWORD bMain_Record:2;
	DWORD bMain_DisplaySetting:2;
	DWORD bMain_ImageSetting:2;
	DWORD bMain_IPChannelSetting:2;
	DWORD bMain_FourScreen:2;
	DWORD bMain_Start:2;
	DWORD bMain_IPCVoice:2; // IPC对讲
	DWORD bMain_PreviewSmart:2;
	DWORD bMain_PlayBackSmart:2;
	
	// dwRes[0]


	DWORD bPtzControl_ControlPan:2;
	DWORD bPtzControl_Speed:2;
	DWORD bPtzControl_Zoom:2;
	DWORD bPtzControl_Iris:2;
	DWORD bPtzControl_Focus:2;
	DWORD bPtzControl_Aux:2;
	DWORD bPtzControl_Preset:2;
	DWORD bPtzControl_Cruise:2;
	DWORD bPtzControl_Track:2;
	DWORD bPtzControl_Other:14;

	DWORD bDisplaySetting_Language:2;
	DWORD bDisplaySetting_AutoLogOut:2;
	DWORD bDisplaySetting_TVAdjust:2;
	DWORD bDisplaySetting_Display:2;
	DWORD bDisplaySetting_Resolution:2;
	DWORD bDisplaySetting_Seq:2;
	DWORD bDisplaySetting_RotationInterval:2;
	DWORD bDisplaySetting_Display_Brightness:2;
	DWORD bDisplaySetting_Display_Contrast:2;
	DWORD bDisplaySetting_Display_Saturation:2;
	DWORD bDisplaySetting_Display_Hue:2;
	DWORD bDisplaySetting_Display_Transparency:2;
	DWORD bDisplaySetting_Display_Style:2;
	DWORD bDisplaySetting_Other:6;

	DWORD bImageSetting_Brightness:2;
	DWORD bImageSetting_Contrast:2;
	DWORD bImageSetting_Saturation:2;
	DWORD bImageSetting_Hue:2;
	DWORD bImageSetting_Other:24;

	DWORD bIPChannel_Manual:2;
	DWORD bIPChannel_Auto:2;
	DWORD bIPChannel_Sync:2;
	DWORD bIPChannel_ManulSync:2;
	DWORD bIPChannel_Drag:2;
	DWORD bIPChannel_Other:22;

	DWORD bStart_UserGuide:2;
	DWORD bStart_PowerOff:2;
	DWORD bStart_Reboot:2;
	DWORD bStart_LogInOut:2;
	DWORD bStart_Lock:2;
	DWORD bStart_ShowDeviceTime:2;
	DWORD bStart_Volume:2;
	DWORD bStart_Other:18;

	// 6
	DWORD bOther_AlarmLinkZoom:2; // 报警联动通道放大
	DWORD bOther_EasyBackup:2; // 快捷备份
	DWORD bFileManage_PlayCap:2;
	DWORD bFileManage_Back:2;
	DWORD bFileManage_LinkRec:2;
	DWORD bFileManage_PlateCap:2;
	DWORD bFileManage_FaceCap:2;
	DWORD bOther_Other:18;
}ANTS_DVR_DEVICESUPPORT_HIDE_MAIN;




// 结果数组类型，数组大小依赖dwOutLength大小,dwOutLength = sizeof(ANTS_DVR_DEVICESUPPORT_RESULTS) * Num
typedef struct _tagANTS_DVR_DEVICESUPPORT_RESULTS
{
	DWORD dwAbilityType; // 如 ANTSMID_DEVICESUPPORT_ABILITY_SENSOR,0-表示结果数组结束
	WORD wChan; // 通道编号从 0开始，通道无关的忽略
	WORD bSupported;// 0- 不支持,1-支持
    // 以下带"Hide"字样的,定制(GUI)某项是否隐藏
	union {
	ANTS_DVR_DEVICESUPPORT_HIDE_DEVICE tDeviceHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_DEVICE
	ANTS_DVR_DEVICESUPPORT_HIDE_CHANNEL tChannelHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_CHAN
	ANTS_DVR_DEVICESUPPORT_HIDE_SMART_CHANNEL tSmartChannelHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_SMART
	ANTS_DVR_DEVICESUPPORT_HIDE_NET tNetHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_NET
	ANTS_DVR_DEVICESUPPORT_HIDE_ALARM tAlarmHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_ALARM
	ANTS_DVR_DEVICESUPPORT_HIDE_USERMGR tUserMgrHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_USER
	ANTS_DVR_DEVICESUPPORT_HIDE_SYSTEM tSystemHide; // ANTSMID_DEVICESUPPORT_ABILITY_CUSTOM_GUI_SYSTEM
	ANTS_DVR_DEVICESUPPORT_HIDE_MAIN tMainHide;
	char sBackupPrivateFileSuffix[8];//定制文件后缀，默认为 "i8", ANTSMID_DEVICESUPPORT_ABILITY_BACKUPFILESUFFIX
	DWORD dwRes[14];// 0; 
		}uInfo;
}ANTS_DVR_DEVICESUPPORT_RESULTS;


/*
车牌识别
*/
#define  AntsPktPlateInfoFrames 0x1d


// 单幅图片中的最大车牌个数
#define IMG_MAX_PLATE_CNT	10

// 车牌字符串的长度
#define PLATE_STRING_LEN    12


// 车牌颜色
typedef enum  
{
	eplate_color_blue,								// 蓝牌(蓝底白字)
	eplate_color_yellow,							// 黄牌(黄底黑字)
	eplate_color_white,								// 白牌(白底黑字)
	eplate_color_black,								// 黑牌(黑底白底)
}vlpr_plate_color;

// 车牌类型
typedef enum
{
	eplate_type_common_military,					// 普通军牌
	eplate_type_common_civil,						// 普通民用车牌
}vlpr_plate_type;


////////////////////////////////////////////////////////////////////////////////
// 结构体定义
////////////////////////////////////////////////////////////////////////////////



// 相对坐标
typedef struct
{
	unsigned short numerator;						// 分子
	unsigned short denominator;						// 分母,必须大于0
}vlpr_relative_pos;

// 目标矩形位置
typedef struct
{
	vlpr_relative_pos x;							// 左上角x坐标
	vlpr_relative_pos y;							// 左上角y坐标
	vlpr_relative_pos w;							// 宽度
	vlpr_relative_pos h;							// 高度
}vlpr_plate_rect;


// 单个车牌识别结果
typedef struct
{
	vlpr_plate_rect rect;							// 对应输入图像中的车牌位置
	vlpr_plate_color color;
	vlpr_plate_type type;
	float alpha;									// 置信度, [0.f, 1.f]
	char license[PLATE_STRING_LEN];					// 车牌字符串
}vlpr_plate_info;

// 一帧中的所有车牌识别结果
typedef struct  
{
	vlpr_plate_info plate[IMG_MAX_PLATE_CNT];
	int plate_cnt;
}vlpr_plate_result;

#define ANTS_DVR_CONFIG_PLATE_DETECT  20122   //ANTS_MID_PLATE_DETECT_CFG






/*
whf add header define2015-11-19
*/
#define ANTS_DVR_GET_CHANOSDTEXTLATTICE 256 //获取通道OSD文件及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE
#define ANTS_DVR_SET_CHANOSDTEXTLATTICE 257 //设置通道OSD文本及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE

#define ANTS_DVR_GET_MULTIOSDTEXTLATTICE 258 //获取多行OSD文件及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE
#define ANTS_DVR_SET_MULTIOSDTEXTLATTICE 259 //设置多行OSD文本及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE

typedef struct _tagANTS_DVR_TEXT_LATTICE
{ 
    WORD width; // In pixel
    WORD height; // In pixel
    WORD stride; // In pixel
    BYTE fontSize;
	BYTE byRes;
    
    DWORD latticeLength;
    BYTE *latticeBuffer;
}ANTS_DVR_TEXT_LATTICE;


typedef struct _tagANTS_DVR_OSD_TEXT_LATTICE
{
	WORD bEnabled; //是否显示
	BYTE type; // 0-Bit MSB; 1-Bit LSB; 8-GrayImage(Reserved for future); 16-RGB1555(Reserved for future); 32-RGB8888(Reserved for future);
    BYTE version;
	WORD posX; // In image relative coordinates. Range is 0-999.
    WORD posY; // In image relative coordinates. Range is 0-999.
    DWORD textLength;
    BYTE *textBuffer;
	union
    {
        WORD reserved[6];
        WORD bitcolor[3]; // textcolor+backcolor+margincolor. Discribed in RGB1555. Only take effect in type=0/1;
    };
	
    ANTS_DVR_TEXT_LATTICE osdMainStream;
    ANTS_DVR_TEXT_LATTICE osdSubStream;
} ANTS_DVR_OSD_TEXT_LATTICE;




typedef struct
{
	char devName[16];
} ANTS_DVR_DEFROUTENAME_S, *LPANTS_DVR_DEFROUTENAME_S ;


//whf add new header define 2016-02-16
#define ANTS_DVR_GET_TIMEOSDPOS 260 //获取时间OSD位置 ANTS_DVR_TIME_OSD_POS
#define ANTS_DVR_SET_TIMEOSDPOS 261 //设置时间OSD位置 ANTS_DVR_TIME_OSD_POS

typedef struct
{
	WORD	wOSDTopLeftX;					/* OSD的x坐标 */
	WORD	wOSDTopLeftY;					/* OSD的y坐标 */
	char	chReserve[28];
}ANTS_DVR_TIME_OSD_POS,*LPANTS_DVR_TIME_OSD_POS;


#endif
