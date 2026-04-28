#ifndef __THIRD_PROTOCOL_API_H__
#define __THIRD_PROTOCOL_API_H__


#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32
#define THIRD_PROTOCOL_API __declspec(dllexport)
#else
#define THIRD_PROTOCOL_API
#endif

#define SCHEDULE_MAX_DAY		7
#define SCHEDULE_MAX_SEGMENT	8

typedef enum
{
    THIRD_CODE_OK = 0,
    // 以下错误编码,必须保持连续的整数递减. 修改后相应的增加字符串. 参考,s_errorStr 和 Ovfs_Web_StrError().
    THIRD_CODE_BASE = -0xA0000,
    THIRD_CODE_GeneralMistake = THIRD_CODE_BASE-1, // 通常性错误
    THIRD_CODE_InvalidArg = THIRD_CODE_BASE-2, // 错误的参数
    THIRD_CODE_LackingMem = THIRD_CODE_BASE-3, // 内存不足
    THIRD_CODE_Unauthorized = THIRD_CODE_BASE-4, // 没有取得认证
    THIRD_CODE_PermissionDenied = THIRD_CODE_BASE-5, // 权限不足
    THIRD_CODE_BlockingOperation = THIRD_CODE_BASE-6, // 操作被阻塞
    THIRD_CODE_InvalidJson = THIRD_CODE_BASE-7, // json内容错误
    THIRD_CODE_InternalMistake = THIRD_CODE_BASE-8, // 内部错误
    THIRD_CODE_Unsupported = THIRD_CODE_BASE-9, // 不支持的功能
    THIRD_CODE_LackingThread = THIRD_CODE_BASE-10, // 无法创建线程
    THIRD_CODE_TaskExist = THIRD_CODE_BASE-11, // 同样任务已经存在
    THIRD_CODE_FileNotAccess = THIRD_CODE_BASE-12, // 文件无法访问
    THIRD_CODE_IamBusy = THIRD_CODE_BASE-13, // 正忙(通常是无法取得锁)
    THIRD_CODE_SingleAccountLogin = THIRD_CODE_BASE-14, // 单用户登录
    THIRD_CODE_SessionNeedLoginFirst = THIRD_CODE_BASE-15, // 单用户登录
    THIRD_CODE_InvalidIpMaskGateway = THIRD_CODE_BASE-16, // ip 子网掩码 网关 不匹配
    THIRD_CODE_PortOccupied = THIRD_CODE_BASE-17, // 端口被占用
    THIRD_CODE_SessionCountMax = THIRD_CODE_BASE-18, // 在线用户已达最大数量
} THIRD_PROTOCOL_ERROR;


typedef enum
{
	THIRD_CMD_GET_DEVICE_INFO		=	0x0001,		//设备信息 获取
	THIRD_CMD_RESTORE				=	0x0002,		//恢复默认
	THIRD_CMD_REBOOT				=   0x0003,		//重启
	THIRD_CMD_GET_MOTION_TIME		=	0x0004,		//移动侦测时间 获取
	THIRD_CMD_SET_MOTION_TIME		=   0x0005,		//移动侦测时间 设置
	THIRD_CMD_GET_VHIDE_TIME		=	0x0006,		//视频遮挡时间 获取
	THIRD_CMD_SET_VHIDE_TIME		=	0x0007,		//视频遮挡时间 设置
	THIRD_CMD_GET_MOTION_CFG		=	0x0008,		//移动侦测区域 获取
	THIRD_CMD_SET_MOTION_CFG		=	0x0009,		//移动侦测区域 设置
	THIRD_CMD_GET_VHIDE_CFG			= 	0x000a,		//视频遮挡区域 获取
	THIRD_CMD_SET_VHIDE_CFG			= 	0x000b,		//视频遮挡区域 设置
	THIRD_CMD_GET_MASK_RECT			=	0x000c,		//隐私遮蔽 获取
	THIRD_CMD_SET_MASK_RECT			= 	0x000d,		//隐私遮蔽 设置

	THIRD_CMD_GET_ALL_TIME			=	0x0011,		//UTC时间 获取
	THIRD_CMD_SET_ALL_TIME			=	0x0012,		//UTC时间 设置
	THIRD_CMD_GET_IMAGE				=	0x0013,		//ISP资源 获取
	THIRD_CMD_SET_IMAGE				=	0x0014,		//ISP资源 设置
	THIRD_CMD_GET_OSD_TIME			=	0x0015,		//OSD时间 获取
	THIRD_CMD_SET_OSD_TIME			=	0x0016,		//OSD时间 设置
	THIRD_CMD_GET_OSD_NAME			=	0x0017,		//OSD通道名 获取
	THIRD_CMD_SET_OSD_NAME			=	0x0018,		//OSD通道名 设置
	THIRD_CMD_FORCE_IFRAME			=	0x0019,		//强制I帧
	THIRD_CMD_GET_HTTPPORT			=	0x001a,		//HTTP端口 获取
	THIRD_CMD_SET_HTTPPORT			=	0x001b,		//HTTP端口 设置
	THIRD_CMD_GET_RTSPPORT			=	0x001c,		//RTSP端口 获取
	THIRD_CMD_SET_RTSPPORT			=	0x001d,		//RTSP端口 设置

	THIRD_CMD_GET_ETHCFG			=	0x0021,		//eth信息 获取
	THIRD_CMD_SET_ETHCFG			=	0x0022,		//eth信息 设置
	THIRD_CMD_GET_DNSCFG			=	0x0023,		//dns信息 获取
	THIRD_CMD_SET_DNSCFG			=	0x0024,		//dns信息 设置
	THIRD_CMD_GET_VIDEO_VENC_ABILITY=	0x0025,		//视频编码能力集 获取
	THIRD_CMD_GET_VIDEO_VENC		=	0x0026,		//视频编码参数 获取
	THIRD_CMD_SET_VIDEO_VENC		=	0x0027,		//视频编码参数 设置
	THIRD_CMD_GET_AUDIO_PARAM		=	0x0028,		//音频参数 获取
	THIRD_CMD_SET_AUDIO_PARAM		=	0x0029,		//音频参数 设置
	THIRD_CMD_SET_DEVICE_NAME		=	0x002a,		//设置设备名称
	THIRD_CMD_GET_VIDEO_SOURCE		=	0x002b,
	THIRD_CMD_GET_VIDEO_SNAPSHOT	=	0x002c,
	THIRD_CMD_GET_OSD_MULTI	        =	0x002d,		//多行OSD通道名 获取
	THIRD_CMD_SET_OSD_MULTI	        =	0x002e,		//多行OSD通道名 设置

	THIRD_CMD_GET_PRESET	        =	0x0031,
	THIRD_CMD_SET_PRESET	        =	0x0032,
	THIRD_CMD_DEL_PRESET	        =	0x0033,
	THIRD_CMD_GOTO_PRESET	        =	0x0034,
	THIRD_CMD_PTZ_CONTROL	        =	0x0035,

    THIRD_CMD_GET_ONVIF_CFG	        =	0x0041,
    THIRD_CMD_SET_ONVIF_CFG	        =	0x0042,

    THIRD_CMD_GET_ZOOM_CFG	        =	0x0043,
    THIRD_CMD_SET_ZOOM_CFG	        =	0x0044,
} THIRD_PROTOCOL_CMD;


//*********************//
//设备信息 获取
typedef struct
{
	int Cmd;				//详见THIRD_COMMON_CMD
	int UseMask;			//bitn--表示参数n(不考虑cmd)是否需要设置
	int IsOfDome;			//是否球机.0--不是,1--是
	int LensSupport;		//是否支持电动镜头
	int IrisSupport;		//是否支持电动光圈
	char DeviceName[48];	//设备名称
	char DeviceModel[32];	//设备型号
	char DeviceType[8];		//设备类型
	char SerialNumber[32];	//序列号
	char HardVersion[32];	//硬件版本
	char ProductDate[16];	//产品日期
	char Manufacturer[64];
	char Hardware[32];
	char Country[32];
	char City[32];
} THIRD_DEVICE_INFO;


//*********************//
//设备名称 设置
typedef struct
{
	int cmd;				//详见THIRD_COMMON_CMD
	int UseMask;			//bitn--表示参数n(不考虑cmd)是否需要设置
	char DeviceName[48];	//设备名称
} THIRD_DEVICE_NAME;


//*********************//
//恢复默认
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int NetConfig;
	int AlarmConfig;
	int UserConfig;
	int Others;
} THIRD_RESTORE;


//*********************//
//重启
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int Delay;			//延迟时间
} THIRD_REBOOT;


//*********************//
//移动侦测时间/视频遮挡时间 获取/设置
typedef struct
{
    //开始时间:小时
    int StartHour;
    //开始时间:分钟
    int StartMinute;
    //结束时间:小时
    int EndHour;
    //结束时间:分钟
    int EndMinute;
} THIRD_TIME_RANGED;


typedef struct
{
	int Cmd;			//详见WEBSERVER_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int Enable;
	THIRD_TIME_RANGED Schedule[SCHEDULE_MAX_DAY][SCHEDULE_MAX_SEGMENT];
} THIRD_TIME_SCHEDULE;


//*********************//
//移动侦测区域 获取/设置
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    int Enable;			//使能
    int Sensitivity;	//灵敏度.1~6
    char RectStr[32 * 32 + 1];
    int BlockW;
    int BlockH;
} THIRD_MOTION_CFG;


//*********************//
//视频遮挡区域 获取
typedef struct
{
    int X;
    int Y;
    int W;
    int H;
} THIRD_VHIDE_CFG;

typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    int Enable;			//遮挡检测开启
    int Sensitivity;	//灵敏度：1～3
    THIRD_VHIDE_CFG VhideRegionCfg;
} THIRD_VIDEO_HIDE;


//*********************//
//隐私遮蔽 获取
typedef struct
{
    int Enable;
    int X;
    int Y;
    int W;
    int H;
} THIRD_MASK_CFG;

typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int MaskCount;
	THIRD_MASK_CFG  *pMaskCfg;
} THIRD_MASK_RECT;


//*********************//
//UTC时间 获取/设置
typedef struct
{
    int Year;
    int Month;
    int Day;
    int Hour;
    int Min;
    int Sec;
} THIRD_TIME_CFG;

typedef struct
{
    int Enable;
    char Server[64];
    int Interval;
} THIRD_NTP_CFG;

typedef struct
{
    int Zone;
    int EnableBias;
    int ZoneBias;
} THIRD_TIME_ZONE;

typedef struct
{
    int Month;
    int WeekIdx;
    int WeekDay;
    int Hour;
    int Min;
} THIRD_DST_NODE;

typedef struct
{
    int Enable;
    int Mode;
    int Bias;
    THIRD_DST_NODE StartTime;
    THIRD_DST_NODE StopTime;
} THIRD_DST_CFG;

typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	THIRD_TIME_CFG UTCTime;
	THIRD_TIME_CFG SysTime;
    THIRD_NTP_CFG Ntp;
    THIRD_DST_CFG Dst;
    THIRD_TIME_ZONE TimeZone;
} THIRD_ALL_TIME;


//*********************//
//ISP资源 获取/设置
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int Brightness;     //亮度 范围：0-255
    int Contrast;   	//对比度 范围：0-255
    int Staturation;    //饱和度 范围：0-255
    int Hue; 			//色度 范围：0-255
	int Sharpness;		//锐度 范围：0到255
	int Mirror;     	//镜像 范围：0-正常 1-水平翻转 2-垂直翻转 3-180度翻转 4-90度翻转 5-270度翻转
	int WdrEnable;      //宽动态使能
    int WdrMode;       	//宽动态模式. 0-线性 1-2帧合成
    int WdrLevel;      	//宽动态级别. 0-低 1-中 2-高
    int DayNightMode;
    int DayToNightThreshold;
    int NightToDayThreshold;
    int DayNightTime;
    int DayNightStartTime;
    int DayNightEndTime;
} THIRD_IMAGE;


//*********************//
//OSD时间 获取/设置
typedef struct
{
	int	Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    int	Enable;
    int	X;
    int	Y;
	int	FontSize;
	int	DateFormat;
	int	TimeFormat;
} THIRD_OSD_TIME;


//*********************//
//OSD通道名(包含多行OSD) 获取/设置
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    int	Enable;
    int	X;
    int	Y;
    char Name[512];
	int FontSize;
} THIRD_OSD_NAME;


//*********************//
//强制I帧
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    int Stream;			//0--主码流 1--子码流
} THIRD_FORCE_IFRAME;


//*********************//
//HTTP参数 获取
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	short HttpPort;
	short HttpsPort;
} THIRD_HTTPPORT;


//*********************//
//RTSP参数 获取
typedef struct
{
    char Ip[16];
    int Port;
    int TTL;
} THIRD_MULTICAST_T;

typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int Enable;
	short RtspPort;
	short Res;			//保留,4字节对齐
	int MulcastEnable;
	THIRD_MULTICAST_T MainVideo;
	THIRD_MULTICAST_T SubVideo;
} THIRD_RTSPPORT;


//*********************//
//eth信息 获取/设置
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    int  Dhcp;
    char Ip[16];
    char Gateway[16];
    char Mask[16];
	char Mac[32];
} THIRD_ETHCFG;


//*********************//
//dns信息 获取/设置
typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
    char Dns1[16];
    char Dns2[16];
} THIRD_DNSCFG;


//*********************//
//视频编码能力集 获取
typedef struct
{
	short Width;	//分辨率宽
	short Height;	//分辨率高
	int Fps;		//帧率
	char ResoStr[32];	//"D1(720*576)"
} THIRD_RESOLUTION_ABILITY;

typedef struct
{
	int Cmd;			//详见THIRD_COMMON_CMD
	int Stream;			//0--主码流; 1--子码流
	int UseMask;		//bitn--表示参数n(不考虑cmd)是否需要设置
	int ResolutionCount;
	THIRD_RESOLUTION_ABILITY *pResolutionAbility;
	int EncTypeCount;
	char (*EncTypeAbility)[16];			//编码类型能力集."H264", "H265"
	int BitrateTypeCount;
	char (*BitrateTypeAbility)[16];		//码率模式."VBR", "CBR"
	int ProfilesCount;
	char (*ProfilesAbility)[16];		//编码质量等级."BaseLine", "Main", "Hight"
	int FpsRange[2];					//帧率范围."1-25"
	int BitrateRange[2];				//比特率范围."16-1024"
	int Iinterval[2];					//帧间隔."2-255"
	int EncQuality[2];				//图像质量等级 0为最低等级."0-5"
} THIRD_VIDEO_ENC_ABILITY;


//*********************//
//视频编码参数 获取/设置
typedef struct
{
	int Cmd;							//详见THIRD_COMMON_CMD
	int Stream;							//0--主码流; 1--子码流
	int UseMask;						//bitn--表示参数n(不考虑cmd)是否需要设置
	int Width;							//分辨率宽
    int Height;							//分辨率高
	int Quality;						//图像质量等级 0-5 0-最差
    int Fps;							//帧率
	int Iinterval;						//I帧间隔（单位帧）
	int BitrateCtrlMode;				//位率模式 0-变码率 1-定码率
	int EncodeFormat;					//编码格式 0-H264 1-H265 2-MPEGMPEG4 3-MJPEG
	int Profiles;						//编码质量0-BaseLine 1-Main 2-Hight
	int Bitrate;						//比特率
	int BitrateIsCustom;				//是否为比特率自定义 为０或１
	char ResolutionStr[32];				//分辨率
} THIRD_VIDEO_ENC;


//*********************//
//音频参数 获取/设置
typedef struct
{
	int Cmd;							//详见THIRD_COMMON_CMD
	int UseMask;						//bitn--表示参数n(不考虑cmd)是否需要设置
	int AudioEnable;					//使能
	int AudioSource;					//0--线入 1--麦克风
    int InputVol;						//输入音量.0~100
	int OutputVol;						//输出音量.0~100
    int EncFormat;						//目前只支持G711U.2--G711U
	int FrameLen;						//音频长度
} THIRD_AUDIO_VENC;


//*********************//
//抓图
typedef struct
{
	int Cmd;							//详见THIRD_COMMON_CMD
	int Stream;
	int UseMask;						//bitn--表示参数n(不考虑cmd)是否需要设置
	char PicPath[128];
	int TimeStamp;
} THIRD_PICTURE_SNAPSHOT;

//*********************//
//PTZ
typedef struct
{
    int Cmd;
    int UseMask;
    int PresetNum;
    int PresetArray[256];
} THIRD_PRESET_CFG;

typedef struct
{
    int Cmd;
    int UseMask;
    int Type;
    int Speed;
} THIRD_PTZ_CONTROL;


//*********************//
//digest认证
typedef struct
{
	int RealmLength;
	char *Realm;
	int QopLength;
	char *Qop;
	int NonceLength;
	char *Nonce;
	int OpaqueLength;
	char *Opaque;
	int CnonceLength;
	char *Cnonce;
	int UriLength;
	char *Uri;
	int ResponseLength;
	char *Response;
	int NcLength;
	char *Nc;
	int MethodLength;
	char *Method;
}THIRD_DIGEST_INFO;


//*********************//
//其它认证
typedef struct
{
    int HasAuth;
    int AuthMethod;
    char UserName[16];
    char Password[16];
    char IpStr[64];
	int NonceLength;
    char *Nonce;
	int CreatedLength;
    char *Created;
	int PasswordDigestLength;
    char *PasswordDigest;
} THIRD_AUTH_INFO;


//*********************//
//报警回调数据
typedef struct
{
    char AlarmName[64];				//报警名称
    int AlarmType;					//报警类型
    int AlarmSrcType;				//报警源
	int Device;
	int Channel;
	int Stream;
	int RegionId;
	int Status;						//报警状态 0-无报警 1-有报警
	char StartTime[20];				//"20170515092317"
    char StopTime[20];				//"20170515092317"
    char DevName[64];
} THIRD_ALARM_INFO;

typedef struct
{
    int Cmd;
    int UseMask;
    int AuthEnable;
    int AdaptiveIp;
    int Timeout;
    int FixedIp;
    char FixedIpAddr[16];

}THIRD_ONVIF_CFG;

typedef struct
{
    int UseMask;
    char DevName[8];
    char RemoteIp[16];

}THIRD_UDP_USERDATA;

typedef struct
{
    int Cmd;
    int UseMask;
    int ZoomInteger;
    int ZoomDecimal;
    int ZoomMax;

}THIRD_ZOOM_CFG;

typedef int (*fCallParamFun)(void *pEncrypt, void *pInBuffer, int nInBufSize, void **pOutBuffer, int *nOutBufSize, int timeOut);

typedef struct _talk_callback
{
    int (*TalkStart)();
    int (*TalkStop)(int handle);
    int (*TalkReleaseData)(int handle);
    int (*TalkInputData)(int handle,void *data,int size);
    int (*TalkOutputData)(int handle,void **data,int *size);
}TalkCallBack,*pTalkCallBack;

THIRD_PROTOCOL_API int THIRD_PROTOCOL_Init(fCallParamFun pCallParamFun);
THIRD_PROTOCOL_API int THIRD_PROTOCOL_Uninit();
THIRD_PROTOCOL_API int THIRD_PROTOCOL_AlarmCallBack(THIRD_ALARM_INFO *pAlarmStatus, int nAlarmStatusCnt);

THIRD_PROTOCOL_API int THIRD_PROTOCOL_DealUdpPkg(void *UserData, char *InBuffer,int InSize,char **OutBuffer,int *OutSize);

//onvif private api
THIRD_PROTOCOL_API int THIRD_PROTOCOL_SetConfig(THIRD_ONVIF_CFG *cfg);

THIRD_PROTOCOL_API int THIRD_PROTOCOL_SetTalkCallBack(TalkCallBack *pfxn);


#ifdef __cplusplus
}
#endif


#endif
