/*
 * access_rest.h
 *
 *  Created on: 2017年4月17日
 *      Author: qinjx
 */
#ifndef ACCESS_REST_H_
#define ACCESS_REST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define BIT_PREVIEW			(0x1<<0)
#define BIT_PLAYBACK 		(0x1<<1)
#define BIT_SETTING 			(0x1<<2)
#define BIT_VIEWSETTING 	(0x1<<3)
#define BIT_RECORD 			(0x1<<4)
#define BIT_PTZ			 	(0x1<<5)
#define BIT_BACKUP		 	(0x1<<6)
#define BIT_LOG			 	(0x1<<7)
#define BIT_VIEWINFO		(0x1<<8)
#define BIT_UPGRADE		 	(0x1<<9)
#define BIT_POWER			(0x1<<10)
#define BIT_FORMAT		 	(0x1<<11)
#define BIT_IPCHANNEL		(0x1<<12)
#define BIT_CORRECTTIME		(0x1<<13)
#define BIT_DELETEUSER		(0x1<<14)
#define BIT_MODIFYUSER 		(0x1<<15)
#define BIT_RESETPWD 		(0x1<<16)
#define BIT_RESETPWDSELF	(0x1<<17)
#define BIT_ONLINEAUTH 		(0x1<<18)
#define BIT_REMOTETALK 		(0x1<<19)

#define BIT_CHAN_PREVIEW			(0x1<<0)
#define BIT_CHAN_PLAYBACK 			(0x1<<1)
#define BIT_CHAN_SETTING 			(0x1<<2)
#define BIT_CHAN_VIEWSETTING 		(0x1<<3)
#define BIT_CHAN_RECORD 			(0x1<<4)
#define BIT_CHAN_PTZ			 	(0x1<<5)
#define BIT_CHAN_BACKUP		 	(0x1<<6)

#define OVFS_MAX_LOGIN_USER   256

#define OVFS_LOGINHANDLE_BITSIZE  12
#define OVFS_LOGINHANDLE_BITMASK  0x0FFF

#define OVFS_SUBHANDLE_BITSIZE  8
#define OVFS_SUBHANDLE_BITMASK  0x0FF

#define OVFS_MODULE_BITSIZE  4
#define OVFS_MODULE_BITMASK  0x0F

#define OVFS_RAND_BITSIZE  7
#define OVFS_RAND_BITMASK  0x7F

#define MAX_IPTABLE_COUNT   100
/* 操作 */
//主类型
#define MAJOR_OPERATION					0x3
//次类型
#define MINOR_START_DVR					0x41	/* 开机 */
#define MINOR_STOP_DVR					0x42	/* 关机 */
#define MINOR_STOP_ABNORMAL				0x43	/* 异常关机 ,如段错误*/
#define MINOR_REBOOT_DVR                0x44    /* 本地重启设备 */
#define MINOR_STOP_DVR_ILLEGAL          0x45    /* 非法关机,如关掉电源*/


#define MINOR_LOCAL_ENTER_CFG_PARM		0x49	/* 进入配置参数 */

#define MINOR_LOCAL_LOGIN				0x50	/* 本地登陆 */
#define MINOR_LOCAL_LOGOUT				0x51	/* 本地注销登陆 */
#define MINOR_LOCAL_CFG_PARM			0x52	/* 本地配置参数 */
#define MINOR_LOCAL_PLAYBYFILE          0x53	/* 本地按文件回放或下载 */
#define MINOR_LOCAL_PLAYBYTIME          0x54	/* 本地按时间回放或下载*/
#define MINOR_LOCAL_START_REC			0x55	/* 本地开始录像 */
#define MINOR_LOCAL_STOP_REC			0x56	/* 本地停止录像 */
#define MINOR_LOCAL_PTZCTRL				0x57	/* 本地云台控制 */
#define MINOR_LOCAL_PREVIEW				0x58	/* 本地预览 (保留不使用)*/
#define MINOR_LOCAL_MODIFY_TIME         0x59	/* 本地修改时间(保留不使用) */
#define MINOR_LOCAL_UPGRADE             0x5a	/* 本地升级 */
#define MINOR_LOCAL_RECFILE_OUTPUT      0x5b    /* 本地备份录象文件 */
#define MINOR_LOCAL_FORMAT_HDD          0x5c    /* 本地初始化硬盘 */
#define MINOR_LOCAL_CFGFILE_OUTPUT      0x5d    /* 导出本地配置文件 */
#define MINOR_LOCAL_CFGFILE_INPUT       0x5e    /* 导入本地配置文件 */
#define MINOR_LOCAL_COPYFILE            0x5f    /* 本地备份文件 */
#define MINOR_LOCAL_LOCKFILE            0x60    /* 本地锁定录像文件 */
#define MINOR_LOCAL_UNLOCKFILE          0x61    /* 本地解锁录像文件 */
#define MINOR_LOCAL_DVR_ALARM           0x62    /* 本地手动清除和触发报警*/
#define MINOR_IPC_ADD                   0x63    /* 本地添加IPC */
#define MINOR_IPC_DEL                   0x64    /* 本地删除IPC */
#define MINOR_IPC_SET                   0x65    /* 本地设置IPC */
#define MINOR_LOCAL_START_BACKUP		0x66	/* 本地开始备份 */
#define MINOR_LOCAL_STOP_BACKUP			0x67	/* 本地停止备份*/
#define MINOR_LOCAL_COPYFILE_START_TIME 0x68	/* 本地备份开始时间*/
#define MINOR_LOCAL_COPYFILE_END_TIME	0x69	/* 本地备份结束时间*/
#define MINOR_LOCAL_ADD_NAS             0x6a	/* 本地添加网络硬盘 （nfs、iscsi）*/
#define MINOR_LOCAL_DEL_NAS             0x6b	/* 本地删除nas盘 （nfs、iscsi）*/
#define MINOR_LOCAL_SET_NAS             0x6c	/* 本地设置nas盘 （nfs、iscsi）*/

#define MINOR_REMOTE_LOGIN				0x70	/* 远程登录 */
#define MINOR_REMOTE_LOGOUT				0x71	/* 远程注销登陆 */
#define MINOR_REMOTE_START_REC			0x72	/* 远程开始录像 */
#define MINOR_REMOTE_STOP_REC			0x73	/* 远程停止录像 */
#define MINOR_START_TRANS_CHAN			0x74	/* 开始透明传输 */
#define MINOR_STOP_TRANS_CHAN			0x75	/* 停止透明传输 */
#define MINOR_REMOTE_GET_PARM			0x76	/* 远程获取参数 */
#define MINOR_REMOTE_CFG_PARM			0x77	/* 远程配置参数 */
#define MINOR_REMOTE_GET_STATUS         0x78	/* 远程获取状态 */
#define MINOR_REMOTE_ARM				0x79	/* 远程布防 */
#define MINOR_REMOTE_DISARM				0x7a	/* 远程撤防 */
#define MINOR_REMOTE_REBOOT				0x7b	/* 远程重启 */
#define MINOR_START_VT					0x7c	/* 开始语音对讲 */
#define MINOR_STOP_VT					0x7d	/* 停止语音对讲 */
#define MINOR_REMOTE_UPGRADE			0x7e	/* 远程升级 */
#define MINOR_REMOTE_PLAYBYFILE         0x7f	/* 远程按文件回放 */
#define MINOR_REMOTE_PLAYBYTIME         0x80	/* 远程按时间回放 */
#define MINOR_REMOTE_PTZCTRL			0x81	/* 远程云台控制 */
#define MINOR_REMOTE_FORMAT_HDD         0x82    /* 远程格式化硬盘 */
#define MINOR_REMOTE_STOP               0x83    /* 远程关机 */
#define MINOR_REMOTE_LOCKFILE			0x84	/* 远程锁定文件 */
#define MINOR_REMOTE_UNLOCKFILE         0x85	/* 远程解锁文件 */
#define MINOR_REMOTE_CFGFILE_OUTPUT     0x86    /* 远程导出配置文件 */
#define MINOR_REMOTE_CFGFILE_INTPUT     0x87    /* 远程导入配置文件 */
#define MINOR_REMOTE_RECFILE_OUTPUT     0x88    /* 远程导出录象文件 */
#define MINOR_REMOTE_DVR_ALARM          0x89    /* 远程手动清除和触发报警*/
#define MINOR_REMOTE_IPC_ADD			0x8a	/* 远程添加IPC */
#define MINOR_REMOTE_IPC_DEL			0x8b	/* 远程删除IPC */
#define MINOR_REMOTE_IPC_SET			0x8c	/* 远程设置IPC */
#define MINOR_REBOOT_VCA_LIB            0x8d    /*重启智能库*/
#define MINOR_REMOTE_ADD_NAS            0x8e   /* 远程添加nas盘 （nfs、iscsi）*/
#define MINOR_REMOTE_DEL_NAS            0x8f   /* 远程删除nas盘 （nfs、iscsi）*/
#define MINOR_REMOTE_SET_NAS            0x90   /* 远程设置nas盘 （nfs、iscsi）*/

#define MAJOR_CONFIG_SET  0x4
/*************************参数配置命令 begin*******************************/
//用于ANTS_DVR_LogicLayer_SetDVRConfig和ANTS_DVR_LogicLayer_GetDVRConfig,注意其对应的配置结构
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
#define ANTS_DVR_GET_TIMECFG 		118		//获取DVR时间 localtime(含有时区偏移和夏令时偏移的时间)
#define ANTS_DVR_SET_TIMECFG		119		//设置DVR时间 localtime(含有时区偏移和夏令时偏移的时间)
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

#define ANTS_DVR_GET_TIMEUTC           154 // UTC时间 gmtime(不含时区偏移和夏令时偏移的时间)
#define ANTS_DVR_SET_TIMEUTC           155 // UTC时间 gmtime(不含时区偏移和夏令时偏移的时间)

#define ANTS_DVR_GET_SLAVE_NETCFG			156		//获取从片网络参数 +  -> ANTS_DVR_IPADDR[n]

#define ANTS_DVR_GET_RECORDTYPES			158	//获取/设置某通道的录像类型

// 0-关掉录像，1-计划录像，2-手动录像
// 缓冲SIZE 决定操作的通道数。每个通道为BYTE型。
// lChannel: 当需要操作多通道时，为起始通道

#define ANTS_DVR_SET_RECORDTYPES			159	



#define ANTS_DVR_GET_ZEROCODEC			160	
#define ANTS_DVR_SET_ZEROCODEC			161	

#define ANTS_DVR_GET_RTSPCFG			162	
#define ANTS_DVR_SET_RTSPCFG			163	

#define ANTS_DVR_GET_TIMEZONE			164	 // 获取时区sizeof(int ) ,分钟,中国 +8 * 60
#define ANTS_DVR_SET_TIMEZONE			165	 // 设置时区



#define ANTS_DVR_GET_KEYWORDS           170


#define ANTS_DVR_GET_WIFIWORKSTATUS     172 // 读取工作状态. 结构体ANTS_DVR_WIFI_WORKSTATUS_S
#define ANTS_DVR_GET_WIFIAPCOUNT        174	// 读取AP列表
#define ANTS_DVR_GET_WIFIAPLIST         175	// 读取AP列表. 结构体ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S
#define ANTS_DVR_GET_WIFISCANCOUNT      176 // 读取搜索AP结果
#define ANTS_DVR_GET_WIFISCANRESULT     177 // 读取搜索AP结果. 结构体ANTS_DVR_WIFI_SCANAPITEM_S
#define ANTS_DVR_SET_WIFIADDAP          178 // 添加AP,或者修改AP参数,包括连接AP动作. 结构体ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S
#define ANTS_DVR_SET_WIFIDELAP          179 // 删除AP,包括断开动作. 结构体ANTS_DVR_WIFI_STAMODE_CONNECTIONCFG_S
#define ANTS_DVR_SET_WIFIDISCONNECT     180 // 断开动作.
#define ANTS_DVR_SET_WIFIWORKMODE       181 // 设置WIFI工作模式
#define ANTS_DVR_GET_DEFROUTENAME       182 // 获取默认路由的网卡设备.结构体ANTS_DVR_DEFROUTENAME_S

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

#define ANTS_DVR_GET_WORKSTATE     254// ANTS_DVR_WORKSTATE_V2

#define ANTS_DVR_GET_NETDEVABILITY  255 // 获取NVR 网络接入能力 Kbps // DWORD[2],sizeof(DWORD[2]);[0]=总能力，[1]= 当前使用大小

#define ANTS_DVR_GET_CHANOSDTEXTLATTICE 256 //获取通道OSD文件及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE
#define ANTS_DVR_SET_CHANOSDTEXTLATTICE 257 //设置通道OSD文本及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE

#define ANTS_DVR_GET_MULTIOSDTEXTLATTICE 258 //获取多行OSD文件及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE
#define ANTS_DVR_SET_MULTIOSDTEXTLATTICE 259 //设置多行OSD文本及点阵设置 参考数据结构ANTS_DVR_OSD_TEXT_LATTICE

#define ANTS_DVR_SET_SENSORCFG	3001

#define ANTS_DVR_SET_ROICFG		20133
#define ANTS_DVR_SET_AUDIOCFG	20134

#define ANTS_DVR_SET_VDIAGNOSE_PARAM 	20126	//修改视频诊断参数
#define ANTS_DVR_SET_CNTWIRT_PARAM 		20117	//修改目标计数参数
#define ANTS_DVR_SET_DETECTWIRE_PARAM 	20118	//修改过线检测参数
#define ANTS_DVR_SET_DETECTREG_PARAM 	20119	//修改区域检测参数
#define ANTS_DVR_SET_DETECTOBJ_PARAM 	20120	//修改物品检测参数
#define ANTS_DVR_SET_DETECTSOUND_PARAM 	20121	//修改声音检测参数

#define ANTS_DVR_SET_SMOTION_PARAM 		20130	//修改智能移动侦测参数
#define ANTS_DVR_SET_DETECTFIRE_PARAM 	20128	//修改火灾检测参数
#define ANTS_DVR_SET_DETECTFACE_PARAM 	20124	//修改人脸检测参数
#define ANTS_DVR_SET_DETECTPLATE_PARAM 	20122	//修改车牌检测参数
#define ANTS_DVR_CONFIG_PLATE_DETECT_LINK	20123   //ANTS_DVR_IVS_DETECT_LINK


#define ANTS_DVR_SET_TEMPERATURE_PARAM 	20132	//修改温度检测参数
#define ANTS_DVR_SET_REGFACE_PARAM 		20200	//修改人脸识别参数
#define ANTS_DVR_SET_PERSON_PARAM 		20202	//修改人形识别参数

#define ANTS_DVR_SET_HIGHDENSITY_PARAM 		20135	//修改人脸识别参数
#define ANTS_DVR_SET_RETROGRADE_PARAM 		20136	//修改人形识别参数
#define ANTS_DVR_SET_ABSENT_PARAM 		    20137	//修改离岗检测参数
#define ANTS_DVR_SET_EWALL_PARAM 		    20138	//区域检测重命名为 修改电子围栏参数 
#define ANTS_DVR_SET_HTTPPUSH_PARAM 		    20139	//http push setting
#define ANTS_DVR_SET_DETECT_HELMET_PARAM    20140	//detect helmet setting
#define ANTS_DVR_SET_HUMAN_TEMP_PARAM        20141	//human temperature setting
#define ANTS_DVR_SET_FACE_MASK_PARAM         20142	//face mask setting
#define ANTS_DVR_SET_EBIKE_PARAM 		     20143	//region rename ebike
#define ANTS_DVR_SET_DOME_TRACK 		     20144	//dome track



#define  OVFS_PRINT_JSON(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		LOGI("%s\n",ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)
#define ovfs_print_json OVFS_PRINT_JSON

#define ARRAYSIZE(ARRAY)	(int)(sizeof(ARRAY)/sizeof(ARRAY[0]))

#define ACCESS_USERCFG_PWD_MAX_NUM 8
#define ACCESS_USERCFG_AUTH_MAX_NUM 8
#define ACCESS_USER_LOGIN_MAX_NUM 256
#define ACCESS_IPCONNECT_MAX_NUM 256
enum{
	LEVEL_GUEST = 0x00,
	LEVEL_DEFAULT = 0x01,
	LEVEL_OPERATOR = 0x02,
	LEVEL_ADMIN = 0x10,
	LEVEL_ROOT = 0xff
};
typedef struct _tagAccessChanRight
{
	U16 wDeviceNo; // 设备号 0,1,2...
	U16 wChannelNo; // 通道号 0,1,2...
	U32 u32RightMask;
}AccessChanRight_T;
typedef struct _tagAccessUserRight
{
	U32 u32RightMask;
	AccessChanRight_T *pChanRight; // 默认是有全权限的，即pChanRight == NULL 时有全权限,!=NULL 时按实际权限走.
	S32 nChanRightCount;
}AccessUserRight_T;
typedef struct _tagIpConnectInfo
{
	S32 hSessionId;
	S8 *szBindIpv4;
	S8 *szBindIpv6;
	S8 *szBindMac; // 绑定MAC，形如 "00:3d:66:ee:02:56"
	S32 tCreateTime;
}IpConnectInfo_T;	
typedef struct _tagAccessOnLineUser
{
	S8 *szUserName;
	S8 byPwdCount;
	S8 *szPassword[ACCESS_USERCFG_PWD_MAX_NUM];
	S32 nSessionCnt;
	S32 hSessionId[ACCESS_USER_LOGIN_MAX_NUM];
	S32 nIpConnCnt;				//不同的IP数
	IpConnectInfo_T *pIpConnList[ACCESS_IPCONNECT_MAX_NUM];
	struct _tagAccessOnLineUser *pPrev;
	struct _tagAccessOnLineUser *pNext;
}AccessOnLineUser_T;
typedef struct _tagAccessUserCfg
{
	S8 *szUserName;
	S8 byPwdCount;
	S8 *szPassword[ACCESS_USERCFG_PWD_MAX_NUM]; // 一用户，多密码联合验证方式,一般用于异地在一个时间范围内登陆
	S8 *szDefaultPassword; // 缺省密码
	S8 *szTempPassword;	// 找回密码时申请的临时密码
	S8 *szSerialNumber; 		// 找回密码时设备序列号
	S32 tTempCreateTime;	//临时密码创建时间
	S32 tTempValidTime;		//临时密码有效期
	U8 byEncryptMethod; // 0- 明文 1-加密
	U8 byPriority; //用户等级 0-禁止用户,1-访客，2-操作员,10-管理员,0xFF-顶级管理员（类似root）
	U8 bForbidden; // 0-不禁用 1-禁用
	U8 bNeedOnlineAuth;// 需要在线授权才能登陆
	S8 byOnlineAuthListCount;
	S8 *szOnlineAuthManList[ACCESS_USERCFG_AUTH_MAX_NUM];// 指定授权人列表，不指定，则有相关权限的都可以授权;在用户登陆时，授权人都应会收到通知，询问授权请求
	U8 bRemote;
	S8 *szBindIpv4;
	S8 *szBindIpv6;
	S8 *szBindMac; // 绑定MAC，形如 "00:3d:66:ee:02:56"
	AccessUserRight_T *pLocalRight; // 不存在时，表示 默认的权限 ,权限修改只能在等级范围内修改，不能越等级修改
	AccessUserRight_T *pRemoteRight;
	Common_Time_T *pCreateTime; // 用户创建的时间
	Common_Time_T *pStopTime;// 用户失效的时间,失效后 nForbidden将自动默为1,通过改时间都不会再恢复有效
	S32 nSessionCnt;
	S32 hSessionId[ACCESS_USER_LOGIN_MAX_NUM];
	S32 nIpConnCnt;				//不同的IP数
	IpConnectInfo_T *pIpConnList[ACCESS_IPCONNECT_MAX_NUM];
	struct _tagAccessUserCfg *pBindFromUser; // 哪些用户绑在此用户上;绑定的用户必须规定时间内一起登陆才有效,不能独立登陆使用,权限以此用户为准
	struct _tagAccessUserCfg *pBindToUser; // 绑在哪个用户上
	struct _tagAccessUserCfg *pBindUserPrev;
	struct _tagAccessUserCfg *pBindUserNext;
	struct _tagAccessUserCfg *pPrev;
	struct _tagAccessUserCfg *pNext;
}AccessUserCfg_T;
typedef struct _tagBindInfo
{
	S8 *szBindIpv4;			//start ipv4
	S8 *szBindEndIpv4;		//end ipv4
	S8 *szBindIpv6;			//start ipv6
	S8 *szBindEndIpv6;		//end ipv6
	S8 *szBindMac; 
	S8 iDirection;			//0:all 1:in 2:out
	S8 iProtocol;				//0:all 1:tcp 2:udp
}BindInfo_T;
typedef struct _tagIpTable
{
	S32 iAccessMode;		//0:disable 1:enable whitelist 2:enable blacklist
	S32 starttime;			
	S32 endtime;
	BindInfo_T *pWhiteList[MAX_IPTABLE_COUNT];
	BindInfo_T *pBlackList[MAX_IPTABLE_COUNT];
}IpTable_T;
typedef struct
{
	int streamNum;
}OVFS_CHANNEL_ABILITY;

typedef struct
{
	int chanNum;
	OVFS_CHANNEL_ABILITY **pChanAbility;
}OVFS_DEV_ABILITY;

typedef struct
{
	int devNum;
	int totalChanNum;
	int totalStreamNum;
	OVFS_DEV_ABILITY **pDevAbility;
}OVFS_ABILITY,*POVFS_ABILITY;

int auth_MakeHandle(int nLoginHandle,int nSubIndex);
S8* auth_EncryptString(S32 nMethod,S8 *pOrgString,S8 *pNewString,S32 nNewSize);
S8* auth_DecryptString(S8 *pOrgString,S32 *lpEncryptType,S8 *pNewString,S32 nNewSize);
S32 AnalyzeUriAndAuthRight(S8 *pMethod,S8 *pUri,U32 u32Right);
OVFS_ABILITY * get_channel_ability();
cJSON_Struct *get_conditionFromUri(S8 *pUri);
int request_channel_ability(ModuleHandle_T hModuleHandle);
int LoadRightModel(ModuleHandle_T hModuleHandle);
int UnLoadRightModel(ModuleHandle_T hModuleHandle);
#ifdef __cplusplus
}
#endif

#endif /* ACCESS_REST_H_ */
