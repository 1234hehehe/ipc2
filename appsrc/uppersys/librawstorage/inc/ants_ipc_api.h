#ifndef __ANTS_IPC_API_H__
#define __ANTS_IPC_API_H__

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

//typedef void VOID;
#ifndef VOID
#define VOID void
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif


#define ANTS_IPC_FOURCC(c0,c1,c2,c3) ((c0)|(c1 << 8) |(c2 << 16) | (c3 << 24))

//#define DEBUGPRINT
#ifdef DEBUGPRINT

#define IPC_TRACE_INFO(x...) 		do{printf("[INFO.%s.%d][%s]",__FUNCTION__,__LINE__,IPC_NAME);printf(x);}while(0)
#define IPC_TRACE_ERROR(x...)		do{printf("[ERR .%s.%d][%s]",__FUNCTION__,__LINE__,IPC_NAME);printf(x);}while(0)
#define IPC_TRACE_DEBUG(x...)		do{printf("[DBG .%s.%d][%s]",__FUNCTION__,__LINE__,IPC_NAME);printf(x);}while(0)
#define IPC_TRACE_WARN(x...)		do{printf("[WARN.%s.%d][%s]",__FUNCTION__,__LINE__,IPC_NAME);printf(x);}while(0)
#define IPC_TRACE_FATAL(x...)		do{printf("[FATL.%s.%d][%s]",__FUNCTION__,__LINE__,IPC_NAME);printf(x);}while(0)

#else
#define IPC_TRACE_INFO(x)
#define IPC_TRACE_ERROR(x)
#define IPC_TRACE_DEBUG(x)
#define IPC_TRACE_WARN(x)
#define IPC_TRACE_FATAL(x)
#endif




// 支持的版本
#define ANTS_IPC_VERSION  0x00010000
#define ANTS_IPC_VERSION1 0x00010001

#define ANTS_IPC_VERSION_MAX ANTS_IPC_VERSION1








//定义IPC协议
#define ANTS_IPC_FLAG0 ANTS_IPC_FOURCC('A','N','T','S')
#define ANTS_IPC_FLAG1 ANTS_IPC_FOURCC('I','P','C','P')



// 恩智文件头，帧头
#pragma pack(push,1)

#define ANTS_FRAME_STARTCODE 					(0xAB010000)
#define ANTS_FILE_STARTCODE 					(0xAA010000)


typedef enum {
	//!主码流帧类型
	AntsIPCFrameType_Error=0x00,
	AntsIPCFrameType_IFrames=0x01,
	AntsIPCFrameType_AudioFrames=0x08,
	AntsIPCFrameType_PFrames=0x09,
	AntsIPCFrameType_BBPFrames=0x0a,
	AntsIPCFrameType_MotionDetection=0x0b,
	AntsIPCFrameType_DspStatus=0x0c,
	AntsIPCFrameType_OrigImage=0x0d,
	AntsIPCFrameType_SysHeader=0x0e,
	AntsIPCFrameType_BPFrames=0x0f,
	AntsIPCFrameType_SFrames=0x10,
	//!子码流帧类型
	AntsIPCFrameType_SubSysHeader=0x11,
	AntsIPCFrameType_SubIFrames=0x12,
	AntsIPCFrameType_SubPFrames=0x13,
	AntsIPCFrameType_SubBBPFrames=0x14,
	//!智能分析信息帧类型
	AntsIPCFrameType_VacEventZones=0x15,
	AntsIPCFrameType_VacObjects=0x16,
	//!第三码流帧类型
	AntsIPCFrameType_ThirdSysHeader=0x17,
	AntsIPCFrameType_ThirdIFrames=0x18,
	AntsIPCFrameType_ThirdPFrames=0x19,
	AntsIPCFrameType_ThirdBBPFrames=0x1a,

		//!智能检测帧类型
	Antsmid_FrameType_SmartIFrames=0x1b,
	Antsmid_FrameType_SmartPFrames=0x1c,

	Antsmid_FrameType_PlateInfoFrames=0x1d,
	// APP metedata帧.
    Antsmid_FrameType_AppFrames=0x20,
    // AntsComb 帧
    Antsmid_FrameType_AntsCombFrames=0x21,
	AntsIPCFrameType_BBIFrames=0xa1,
}AntsIPCFrameType_E;


typedef enum{
	AntsIPCVoiceCodecID_OggVorbis=0,
	AntsIPCVoiceCodecID_G711A=1,
	AntsIPCVoiceCodecID_G711U=2,
	AntsIPCVoiceCodecID_G722Ex=3,
	AntsIPCVoiceCodecID_G726Ex=4
}AntsIPCVoiceCodecID_E;

typedef enum {
	AntsIPCVideoCodecID_H264_hisi=1,
	AntsIPCVideoCodecID_MJPEG_hisi=2,
	AntsIPCVideoCodecID_H264_hisi_high=4,
	AntsIPCVideoCodecID_H264_hisi_RTP=8,
}AntsIPCVideoCodecID_E;


typedef struct _tagAntsIPCAudioHeader{
	S8 cCodecId;			//!音频编码类型
	S8 cSampleRate;			//!采样率
	S8 cBitRate;				//!比特率
	S8 cChannels;			//!通道数
	S8 cResolution;			//!分辨力
	S8 cResv[3];				//!保留位
}AntsIPCAudioHeader_T;

typedef struct _tagAntsIPCVideoHeader{
	U16 usWidth;				//!视频宽度
	U16 usHeight;				//!视频高度
	S8 cCodecId;				//!视频编码类型
	S8 cResv[3];					//!保留位
}AntsIPCVideoHeader_T;

typedef struct _tagAntsIPCFrameHeader{
	U32 uiStartId;					//!帧同步头ANTS_FRAME_STARTCODE
	U32 uiFrameType;				//!帧类型
	U32 uiFrameNo;					//!帧号
	U32 uiFrameTime;				//!UTC时间
	U32 uiFrameTickCount;			//!微秒为单位的微秒时间
	U32 uiFrameLen;				//!帧载长度
										//!联合体,用于存储音频帧或是视频帧信息
	union {
		AntsIPCAudioHeader_T struAudioHeader;	//!音频帧信息
		AntsIPCVideoHeader_T struVideoHeader;	//!视频帧信息
	}uMedia;
	U32 dwTimeStamp;				//!相对时间戳 ms * 90		
}AntsIPCFrameHeader_T;

typedef struct _tagAntsIPCFileHeader{
	U32 uiFileStartId;				//!文件起始头标记ANTS_FILE_STARTCODE
	U32 uiStreamType;				//!流方式
	U32 uiFrameRate;				//!帧率
	U32 uiReserve[4];				//!保留位	
}AntsIPCFileHeader_T;

#pragma pack(pop)

typedef struct _tagAntsIPCAlarmInfo
{
	U32 dwType;// 1- Motion,2-HideAlarm,3-AlarmIn
	U32 dwStatus; // 
	U32 dwParams[6];
}AntsIPCAlarmInfo_T;


//状态类型
typedef enum _tagAntsIPCStatusType
{
	AntsIPCStatusType_Connected = 1,
	AntsIPCStatusType_Disconnected,
	AntsIPCStatusType_LoginOk,
	AntsIPCStatusType_LoginFailed,
	AntsIPCStatusType_RealPlayOK,
	AntsIPCStatusType_RealPlayFailed,
}AntsIPCStatusType_T;

typedef enum _tagAntsIPCAlarmType
{
	AntsIPCAlarmType_No = 0,
	AntsIPCAlarmType_Motion =1,
	AntsIPCAlarmType_AlarmIn,
	AntsIPCAlarmType_VideoLoss,
	AntsIPCAlarmType_Mask,// 遮挡报警
	AntsIPCAlarmType_DiskFull,
	AntsIPCAlarmType_DiskError,
	AntsIPCAlarmType_IllegeAccess, // 非法访问
	AntsIPCAlarmType_ReticleDisconnect, // 网线断
	AntsIPCAlarmType_IpConelict, // IP冲突
	AntsIPCAlarmType_MAX,
}AntsIPCAlarmType_E;

typedef struct _tagAntsIPCStatusAlarm
{
	U32 dwAlarmType; // AntsIPCAlarmType_E
	U32 dwAlarmParam[32];// dwAlarmParam[0]：源通道,dwAlarmParam[1]:是否警报(0/1)
}AntsIPCStatusAlarm_T;

//配置命令
typedef enum _tagAntsIPCConfigCommand
{
	AntsIPCConfig_OSD                = 1, // OSD  AntsIPCOSD_T
	AntsIPCConfig_VideoEffect, 			  //亮度，对比度，饱和度...AntsIPCColor_T
	AntsIPCConfig_VideoEncode, 			  //视频编码AntsIPCVideoEnc_T
	AntsIPCConfig_AudioEncode,            //音频编码AntsIPCAudioEnc_T
	AntsIPCConfig_Motion             = 5, //移动AntsIPCMotion_T
	AntsIPCConfig_HideAlarm,              //遮挡报警 AntsIPCHideAlarm_T
	AntsIPCConfig_Shelter,                //遮挡AntsIPCShelter_T
	AntsIPCConfig_PTZDecoder,             //PTZ Decoder配置AntsIPCPTZDecoder_T
	AntsIPCConfig_Time,                   // 时间配置AntsIPCTime_T
	AntsIPCConfig_Sensor             = 10,// Sensor配置AntsIPCSensorCfg_T
	AntsIPCConfig_DeviceInfo,             // 设备信息AntsIPCDeviceInfo_T
	AntsIPCConfig_VideoEncodeV2, 			  //视频编码AntsIPCVideoEncV2_T
}AntsIPCConfigCommand_E;
//配置结构





typedef struct _tagAntsIPCDeviceInfo
{
	
	S8	sDVRName[32];     //DVR名称
	U32 dwDVRID;				//DVR ID,用于遥控器
	U32 dwRecycleRecord;		//是否循环录像,0:不是; 1:是

	//以下只读
	U8	sSerialNumber[48];  //序列号
	U32	dwSoftwareVersion;		//软件版本号,高16位是主版本,低16位是次版本
	U32	dwSoftwareBuildDate;	//软件生成日期,0xYYYYMMDD
	U32	dwDSPSoftwareVersion;	//DSP软件版本,高16位是主版本,低16位是次版本
	U32	dwDSPSoftwareBuildDate;	// DSP软件生成日期,0xYYYYMMDD
	U32	dwPanelVersion;			// 前面板版本,高16位是主版本,低16位是次版本
	U32	dwHardwareVersion;		//硬件版本,高16位是主版本,低16位是次版本
	U8	byAlarmInPortNum;		//DVR报警输入个数
	U8	byAlarmOutPortNum;		//DVR报警输出个数
	U8	byRS232Num;				//DVR 232串口个数
	U8	byRS485Num;				//DVR 485串口个数
	U8	byNetworkPortNum;		//网络口个数
	U8	byDiskCtrlNum;			//DVR 硬盘控制器个数
	U8	byDiskNum;				//DVR 硬盘个数
	U8	byDVRType;				//DVR类型, 1:DVR 2:NVR 3:IPC 4:DEC ......
	U8	byChanNum;				//DVR 通道个数
	U8	byStartChan;			//起始通道号,例如DVS-1,DVR - 1
	U8	byDecodeChans;			//DVR 解码路数
	U8	byVGANum;				//VGA口的个数
	U8	byUSBNum;				//USB口的个数
    U8	byAuxoutNum;			//辅口的个数
    U8	byAudioNum;				//语音口的个数
    U8	byIPChanNum;			//最大数字通道数
    U8  byRes[64];
}AntsIPCDeviceInfo_T;


// ->AntsIPCConfig_VideoEffect
typedef struct _tagAntsIPCColor
{
	S32	lBrightness;  	/*亮度,0-255,-1不支持或者恢复默认*/
	S32	lContrast;    	/*对比度,0-255,-1不支持*/	
	S32	lSaturation;  	/*饱和度,0-255, -1不支持*/
	S32	lHue;    		/*色调,0-255,-1不支持*/
}AntsIPCColor_T;


typedef struct _tagAntsIPCSensorDayNightMode
{
	S32 lDayNightMode;// -1--不支持;0--外部红外控制;1--自动模式;2--强制白天;3--强制黑夜
	S32 lDelay ;// 自动转换延迟，自动模式有效。0-30
	S32 lNighttoDayThreshold ;// 自动转换黑夜到白天的阈值0-255，默认0xEE
	S32 lDaytoNightThreshold ;// 自动转换白天到黑夜的阈值0-255，默认0x57
}AntsIPCSensorDayNightMode_T;


typedef struct _tagAntsIPCSensorCfg
{
	U32 dwSize;
	U32 dwValidMask;// 相应位0-无效，1-有效; 
	// bit0 - DayNightMode,bit1-lMinorMode,bit2-lGainMode,bit3-lAntiflickerMode,
	// bit4-lPicQualityMode,bit5-lWBMode   ,bit6-lBacklightMode,bit7-lShutterMode
	// bit8-lIrisMode         ,bit9-lSharpnessMode,bit10-l3DNRMode,bit11-3DNRTfode,
	// bit12-WDMode,bit13-GammaMode,bit14-AntiflickerFreqMode
	AntsIPCSensorDayNightMode_T DayNightMode;
	S32 lMinorMode ;//镜像 -1--不支持;0--正常;1--水平翻转;2--垂直翻转;3--180°翻转
	S32 lGainMode ;//增益 -1--不支持;0--低;1--中;2--高
	S32 lAntiflickerMode ;//抗闪 -1--不支持;0--关;1--开
	S32 lPicQualityMode ;//图像效果 -1--不支持;0--正常;1--艳丽;2--自然
	S32 lWBMode ;//白平衡 -1--不支持;0--自动白平衡;1--室内模式;2--室外模式
	S32 lBacklightMode;// 背光补偿模式-1--不支持; 0--关闭;1--BLC;2--HBLC
	S32 lShutterMode;// 快门模式 -1--不支持; 0--自动快门;
				/*其他模式:
				0x01:1/30(1/25), 
				0x02:1/60(1/50), 
				0x03:Flicker, 
				0x04:1/250, 
				0x05:1/500, 
				0x06:1/1000, 
				0x07:1/2000, 
				0x08:1/5000, 
				0x09:1/10000, 
				0x0A:1/50000, 
				0x0B:x2, 
				0x0C:x4, 
				0x0D:x6, 
				0x0E:x8, 
				0x0F:x10, 
				0x10:x15, 
				0x11:x20,
				0x12:x25,
				0x13:x30 */
	S32 lIrisMode;// 镜头光圈模式:-1--不支持; 0--自动光圈;1--手动或固定光圈
	S32 lSharpnessMode;// 锐度模式: -1--不支持;0--关闭;1--打开
	S32 lSharpnessLevel;// 0-100
	S32 l3DNRMode;// 3D降噪模式 : -1--不支持;0--关闭;1--打开
	S32 l3DNRLevel;// 0-100
	S32 l3DNRTfode; // 3D降噪时域-1--不支持 0--关闭;1--低;2--中;3--较高;4--高
	S32 lWDMode; // 宽动态模式-1--不支持0--关闭;1--低;2--中;3--高
	S32 lGammaMode;// Gamma模式-1--不支持 0--Curve_1_6;1--Curve_1_6;2--Curve_2_0;3--Curve_2_2
	S32 lAntiflickerFreqMode; // 抗闪模式	-1--不支持;0--自动;1--50HZ;2--60HZ
	S32 lRes[1];
}AntsIPCSensorCfg_T;


// ->AntsIPCConfig_OSD
typedef struct _tagAntsIPCOSD
{ //  区域大小704*576
	S8	sText[32]; // 一般为通道名
	
	//显示通道名
	U32	dwShowOsd; 				// bit0:通道名显示,0-不显示,1-显示;bit1:时间显示0-不显示，1-显示
	U32	dwTextTopLeftX;				/* 通道名称显示位置的x坐标 */
	U32	dwTextTopLeftY;				/* 通道名称显示位置的y坐标 */
	U32	dwTimeTopLeftX;					/* OSD的x坐标 */
	U32	dwTimeTopLeftY;					/* OSD的y坐标 */
	U32 dwTimeAttrib;// byte0:OSD格式;byte1:是否显示星期,byte2:透明属性,byte3:0-24小时制,1-12小时制
         /* OSD类型(主要是年月日格式) */
		/* 0: XXXX-XX-XX 年月日 */
		/* 1: XX-XX-XXXX 月日年 */
		/* 2: XXXX年XX月XX日 */
		/* 3: XX月XX日XXXX年 */
		/* 4: XX-XX-XXXX 日月年*/
		/* 5: XX日XX月XXXX年 */
}AntsIPCOSD_T;


typedef enum _tagAntsIPCResolution
{
	AntsIPCResolution_QQCIF = 1, // 88 x72
	AntsIPCResolution_SQCIF,     //128x96
	AntsIPCResolution_QQVGA,     //160x120
	AntsIPCResolution_QCIF,      //176x144
	AntsIPCResolution_SQVGA = 5,     //220x176
	AntsIPCResolution_QQ960H ,      //240x144
	AntsIPCResolution_QQ720P,       //320x180
	AntsIPCResolution_CGA,       //320x200
	AntsIPCResolution_QVGA,      //320x240
	AntsIPCResolution_CIF=10,       //352x288
	AntsIPCResolution_WQVGA ,     //480x240
	AntsIPCResolution_Q960H ,      //480x288
	AntsIPCResolution_HQVGA ,     //480x320
	AntsIPCResolution_DCIF,      //528x384
	AntsIPCResolution_VGA=15,       //640x480
	AntsIPCResolution_Q720P,       //640x360
	AntsIPCResolution_2CIF ,      // 704x288
	AntsIPCResolution_480P ,      // 720x 480
	AntsIPCResolution_4CIF ,       //704x576
	AntsIPCResolution_D1P=20,        //720x576
	AntsIPCResolution_SVGA,      //800x600
	AntsIPCResolution_960H ,      //960x576
	AntsIPCResolution_DVGA ,      //960x640
	AntsIPCResolution_XGA ,       //1024x768
	AntsIPCResolution_720P=25 ,      // 1280x720P
	AntsIPCResolution_720I,      // 1280x720I
	AntsIPCResolution_960P,    // 1280x960
	AntsIPCResolution_1024P ,    // 1280x1024
	AntsIPCResolution_UXGA ,      //1600x1200
	AntsIPCResolution_1080P=30 ,     // 1920x1080P
	AntsIPCResolution_1080I,     // 1920x1080I
	AntsIPCResolution_3MP,       // 2048x1536
	AntsIPCResolution_5MP ,       // 2592x1920
	AntsIPCResolution_USERDEFINE = 100
	
}AntsIPCResolution_E;
// ->AntsIPCConfig_VideoEncode
typedef struct _tagAntsIPCVideoEnc
{
	U8		byVideoEncType;		//视频编码类型: 0- 不支持,1-h264;2-mpeg4;3-M-JPEG
	U8		byIntervalBPFrame;	//0- P帧,1-BP帧; 2-BP帧 (保留)
	U8		byBitrateType;		//码率类型 0:变码率, 1:定码率
	U8		byPicQuality;		//图象质量 0-最好 1-次好 2-较好 3-一般 4-较差 5-差
	U8		byResolution;  		//分辨率0-自定义,AntsIPCResolution_E
	U32     dwResWidth;      //分辨率 宽,自定义时必填
	U32     dwResHeight;     // 分辨率高
	U32		dwVideoBitrate; 	//视频码率 kbps,最高位为1时为自定义。
	U32		dwVideoFrameRate;	//帧率 0-满帧; 
	U32		dwIntervalFrameI;	//I帧间隔
 	
}AntsIPCVideoEnc_T;

typedef struct _tagAntsIPCVideoEncV2
{
	U8		byVideoEncType;		//视频编码类型: 0- 不支持,1-h264;2-mpeg4;3-M-JPEG
	U8		byIntervalBPFrame;	//0- P帧,1-BP帧; 2-BP帧 (保留)
	U8		byBitrateType;		//码率类型 0:变码率, 1:定码率
	U8		byPicQuality;		//图象质量 0-最好 1-次好 2-较好 3-一般 4-较差 5-差
	U8		byResolution;  		//分辨率0-自定义,AntsIPCResolution_E
	U32     dwResWidth;      //分辨率 宽,自定义时必填
	U32     dwResHeight;     // 分辨率高
	U32		dwVideoBitrate; 	//视频码率 kbps,最高位为1时为自定义。
	U32		dwVideoFrameRate;	//帧率 0-满帧; 
	U32		dwIntervalFrameI;	//I帧间隔
 	U8		byVideoH264Profile;	 //视频编码类型: 0-Baseline;1-MainProfile;2-HighProfile
	U8      byRes[14];
}AntsIPCVideoEncV2_T;


// ->AntsIPCConfig_AudioEncode
typedef struct _tagAntsIPCAudioEnc
{
 	U8		byAudioEncType;		//音频编码类型 0-不支持;1-G711_U;2-G711_A,3-G726
 	U8      byAudioChan;//  音频通道,1-单通道,
 	U8      byAudiobitWidth; // 8位,16位
 	U8      byEnable; // 0-关闭，1-开启
 	U32     dwSampleRate;// 采样率8K
 	U32     dwBitRate;// 码率
}AntsIPCAudioEnc_T;

// ->AntsIPCConfig_Motion
typedef struct _tagAntsIPCMotion
{//22 x18
	U32      byEnable;//0-不使能，1-使能
	U32     dwMotionScope[2 * 64]; // 最大支持 64x64,按位
	U32     dwMotionSize;// 高16位:宽,低16位：高
	U32		byMotionSensitive;		/* 移动侦测灵敏度, 0 - 5,越高越灵敏 */
}AntsIPCMotion_T;

// ->AntsIPCConfig_HideAlarm
typedef struct _tagAntsIPCHideAlarm
{// 704 x576
	U32	dwEnable;				/* 是否启动遮挡报警 ,0-否,1-低灵敏度 2-中灵敏度 3-高灵敏度*/
	U16	wAreaTopLeftX;			/* 遮挡区域的x坐标 */
	U16	wAreaTopLeftY;			/* 遮挡区域的y坐标 */
	U16	wAreaWidth;			/* 遮挡区域的宽 */
	U16	wAreaHeight;			/* 遮挡区域的高*/
}AntsIPCHideAlarm_T;

// ->AntsIPCConfig_Shelter
typedef struct _tagAntsIPCShelter
{
	//遮挡	区域大小704*576
	U32	dwEnable;					/* 是否启动遮挡 ,0-否,1-是*/
	U32 dwTopLeftX[4];
	U32 dwTopLeftY[4];
	U32 dwWidth[4];
	U32 dwHeight[4];
}AntsIPCShelter_T;



typedef enum _tagAntsIPCAbilityType
{
	AntsIPCAbilityType_VideoEncode = 1,
	AntsIPCAbilityType_VideoEncode_TransType, // 0-主码流，1-子码流，2-事件码流
	AntsIPCAbilityType_VideoEncode_StreamType, //0- 视频流，1-复合流
	AntsIPCAbilityType_VideoEncode_MainResolution,
	AntsIPCAbilityType_VideoEncode_SubResolution = 5,
	AntsIPCAbilityType_VideoEncode_EventResolution,
	AntsIPCAbilityType_VideoEncode_BitrateType,//变码率，定码率
	AntsIPCAbilityType_VideoEncode_Bitrate,
	AntsIPCAbilityType_VideoEncode_Framerate,
	AntsIPCAbilityType_VideoEncode_PicQuality = 10, // 最好，次好，较好，一般，较差，差
	AntsIPCAbilityType_VideoEncode_EncType, // 0-H264,1-MPEG4,2-MJPEG
	AntsIPCAbilityType_VideoEncode_H264Profiles, // 0-Baseline,1-main,2-high
	AntsIPCAbilityType_AudioEncode_EncType, // 0-无音频或不支持,1-G711u,2-G711a,3-G726
	AntsIPCAbilityType_PTZDecoderProtocol = 20,
}AntsIPCAbilityType_T;

typedef struct _tagAntsIPCAbilityNode
{
	// 帧率, 0-满帧
	U32 dwValue;// 对码率，-1为自定义,0-保留
	S8  sDescribe[32];
}AntsIPCAbilityNode_T;

typedef struct _tagAntsIPCAbilityList
{
	U32	dwAbilityType;
	U32	dwNodeNum;
	AntsIPCAbilityNode_T tNode[256];
}AntsIPCAbilityList_T;

typedef struct _tagAntsIPCAbilitySets
{
	U32	dwAbilityNum;
	AntsIPCAbilityList_T tList[12];
}AntsIPCAbilitySets_T;


// ->AntsIPCConfig_PTZDecoder
//通道解码器(云台)参数配置
typedef struct _tagAntsIPCPTZDecoder
{
	U32	dwBaudRate;						//波特率(bps)，
	U8	byDataBit;						//数据有几位5位，6位，7位，8位;
	U8	byStopBit;						//停止位 :1位，2位;
	U8	byParity;						//校验 0－无校验，1－奇校验，2－偶校验;
	U8	byFlowcontrol;					//0－无，1－软流控,2-硬流控
	U16	wDecoderType;					//解码器类型  从能力集里得到
	U16	wDecoderAddress;				/*解码器地址:0 - 255*/
}AntsIPCPTZDecoder_T;

//校时结构参数
typedef struct _tagAntsIPCTime
{
	U32	dwYear;			//年
	U32	dwMonth;		//月
	U32	dwDay;			//日
	U32	dwHour;			//时
	U32	dwMinute;		//分
	U32	dwSecond;		//秒
	S32 dwZone;         //时区
}AntsIPCTime_T;

typedef struct _tagAntsIPCRect
{
	S32 lx;
	S32 ly;
	S32 lw;
	S32 lh;
}AntsIPCRect;


//远程控制命令
typedef enum _tagAntsIPCControlCmd 
{
	AntsIPCControlCmd_PTZ = 2,
	AntsIPCControlCmd_Reboot = 10,
	AntsIPCControlCmd_Halt,
	AntsIPCControlCmd_EnableAlarm, // 布防，撤防 U32
	AntsIPCControlCmd_RemoteHistoryPlayControl=20,
	AntsIPCControlCmd_RemoteRecordControl=25, // 远程录像控制
	AntsIPCControlCmd_VideoEffectDefault = 30, // 恢复图像参数为默认值
}AntsIPCControlCmd_E;


typedef enum _tagAntsIPCPTZCmd
{
	AntsIPCPTZCmd_LightPowerOn = 2,
	AntsIPCPTZCmd_WiperPowerOn,
	AntsIPCPTZCmd_FanPowerOn,
	AntsIPCPTZCmd_HeaterPowerOn = 5,
	AntsIPCPTZCmd_AuxPowerOn1,
	AntsIPCPTZCmd_AuxPowerOn2,
	AntsIPCPTZCmd_SetPreset,
	AntsIPCPTZCmd_ClearPreset,
	AntsIPCPTZCmd_ZoomIn = 11,
	AntsIPCPTZCmd_ZoomOut,
	AntsIPCPTZCmd_FocusNear,
	AntsIPCPTZCmd_FocusFar,
	AntsIPCPTZCmd_IrisOpen = 15,
	AntsIPCPTZCmd_IrisClose,
	AntsIPCPTZCmd_Up = 21,
	AntsIPCPTZCmd_Down,
	AntsIPCPTZCmd_Left,
	AntsIPCPTZCmd_Right,
	AntsIPCPTZCmd_UpLeft = 25,
	AntsIPCPTZCmd_UpRight,
	AntsIPCPTZCmd_DownLeft,
	AntsIPCPTZCmd_DownRight,
	AntsIPCPTZCmd_PanAuto = 29,//以SS的速度左右自动扫描
	AntsIPCPTZCmd_SeqAddPreset = 30,
	AntsIPCPTZCmd_SeqSetDWell,// 巡航点停顿时间
	AntsIPCPTZCmd_SeqSetSpeed,// 速度
	AntsIPCPTZCmd_SeqDelPreset,//删除预置点
	AntsIPCPTZCmd_CruiseRecStart,// 开始记录轨迹
	AntsIPCPTZCmd_CruiseRecStop=35,// 停止记录轨迹
	AntsIPCPTZCmd_CruiseRun,//开始轨迹
	AntsIPCPTZCmd_SeqRun,//开始巡航
	AntsIPCPTZCmd_SeqStop,//停止巡航
	AntsIPCPTZCmd_GotoPreset,// 转到点
	AntsIPCPTZCmd_SeqFill=40,//将巡航序列设置到云台
	AntsIPCPTZCmd_Home,
	AntsIPCPTZCmd_TransPTZ = 100,// 透明通道

	AntsIPCPTZCmd_GetCruisePoint = 0x10000, // 获取巡航点
}AntsIPCPTZCmd_E;
//控制相关结构
// ->AntsIPCControlCmd_PTZ
typedef struct _tagAntsIPCPTZControl
{
	U32 dwCmd;
	U32 dwParams[6]; // 0- 无效值
	// 方向控制:dwParams[0] :1-停止;dwParams[1]:速度1-16
	//预置点:dwParams[0]:预置点
	//巡航:dwParams[0]:巡航序号，dwParams[1]巡航点，dwParams[2]输入参数
	//轨迹:不用参数
	//辅助 电源:dwParams[0],1-开，2-关
	// 透明通道: dwParams[0]:0-无效，否则缓冲指针；dwParams[1]，缓冲长度
	// 获取巡航点: dwParams[0] 巡航序号
}AntsIPCPTZControl_T;

typedef struct _tagAntsIPCPTZCruisePoint
{
	U8	PresetNum;
	U8	Dwell;
	U8	Speed;
	U8	Reserve;
}AntsIPCPTZCruisePoint_T;

typedef struct _tagAntsIPCPTZCruiseSets
{
	AntsIPCPTZCruisePoint_T		struCruisePoint[32];
}AntsIPCPTZCruiseSets_T;


//搜索相关
typedef struct _tagAntsIPCIPv4
{
	U32 dwValid;// 指示有效项,bit0-mac,1-ip,2-mask,3-gateway,4-dns1,5-dns2
	U8  byMac[8];	// 修改时，此项必须提供
	S8  szIpAddr[16];
	S8  szNetMask[16];
	S8  szGateway[16];
	S8  szDns1[16];
	S8  szDns2[16];
}AntsIPCIPv4_T;


typedef struct _tagAntsIPCSearchInfo
{
	U16       bAddressType;// 0- ipv4,1-dnsname,2-ipv6
	U16		  wPort;
	U8       dwTransType;// 支持的传输类型,可或:1-TCP,2-UDP
	U8       byDeviceType;// 0-未定义的 (可默认为IPC),  1-dvr,2-nvr,3-ipc,4-dec
	U8       byRes[2];
	S8		  szDeviceName[64];
	S8        szUserName[64];
	S8        szPassword[64];
	union 
	{
		S8            szUrl[256];
		S8            szIPv6[64];
		AntsIPCIPv4_T struIPInfo;
	}uAddress;
}AntsIPCSearchInfo_T;


//回调函数

//////////////////////////////
// ANTS_IPC_STATUS_CALLBACK 状态回调
//S32 hLogin: 状态对应的句柄, 0- 句柄无关的状态
// lChannel: 0- 通道无关
// nStreamIdx: 0- 流无关
//dwStatusType：状态类型AntsIPCStatusType_T
//pBuffer:状态对应的数据
//dwBufSize:数据长度
//pUser:回调的用户数据


typedef VOID (*ANTS_IPC_STATUS_CALLBACK)(S32 hLogin,S32 lChannel,S32 lStreamIdx,U32 dwStatusType,U8 *pBuffer,U32 dwBufSize,VOID* pUser);

///////////////////////////////////
//ANTS_IPC_STREAM_CALLBACK,流回调
//S32 hLogin: 登陆获取的句柄
//lChannel: 通道号,0-通道无关,1,2,3,4...通道号
//nStreamIdx:流序号,0-流序号无关,1-主码流，2-子码流，3-第三码流
//dwDataType:数据类型，0-文件头,1-视频,2-音频3-报警信息(AntsIPCAlarmInfo_T)
//pBuffer:数据缓冲，如果是音视频，须包括恩智帧头
//dwBufSize:数据大小
//pUser:回调时设置的用户数据


typedef VOID (*ANTS_IPC_STREAM_CALLBACK)(S32 hLogin,S32 lChannel,S32 lStreamIdx,U32 dwDataType,U8 *pBuffer,U32 dwBufSize,VOID* pUser);



typedef VOID (*ANTS_IPC_ALARM_CALLBACK)(S32 hLogin,S32 lChannel,S32 lStreamIdx,U32 dwAlarmType,U8 *pBuffer,U32 dwBufSize,VOID* pUser);


//接口函数定义

typedef S32 (*ANTS_IPC_INIT)(VOID *pParam);
typedef S32 (*ANTS_IPC_UNINIT)(VOID *pParam);


typedef S32 (*ANTS_IPC_GETSUPPORTFUNCTIONS)(VOID *pFxnsBuffer,U32 dwBufferSize);


//////
//设置状态回调函数
typedef S32 (*ANTS_IPC_SETSTATUSCALLBACK)(ANTS_IPC_STATUS_CALLBACK fxn,VOID *pUser);

typedef S32 (*ANTS_IPC_SETALARMCALLBACK)(S32 hLogin,ANTS_IPC_ALARM_CALLBACK fxn,VOID *pUser);


/////////////////////////////////////////////////
//ANTS_IPC_LOGIN:登陆IPC
//nLinkProtocol:0-TCP,1-UDP
//pIPv4:IPC地址
//dwPort:端口号
// pUsername:用户名
//pPassword:密码
//fxn:流回调函数
// pUser:回调函数用户数据
//返回:<= 0-失败，>0返回句柄


typedef S32 (*ANTS_IPC_LOGIN)(U32 nLinkProtocol,S8 *pUrl,U16 dwPort,S8 *pUsername,S8 *pPassword,ANTS_IPC_STREAM_CALLBACK fxn, VOID *pUser);

//返回0-成功，-1失败
typedef S32 (*ANTS_IPC_STARTREALPLAY)(S32 hLogin,S32 lChannel,S32 nStreamIdx);
typedef S32 (*ANTS_IPC_STOPREALPLAY)(S32 hLogin,S32 lChannel,S32 nStreamIdx);
typedef S32 (*ANTS_IPC_LOGOUT)(S32 hLogin);
typedef S32	(*ANTS_IPC_SETCONFIG)(S32 hLogin, U32 dwCommand, S32 lChannel,S32 nStreamIdx, VOID* lpInBuffer, U32 dwInBufferSize);
typedef S32	(*ANTS_IPC_GETCONFIG)(S32 hLogin, U32 dwCommand, S32 lChannel,S32 nStreamIdx, VOID* lpOutBuffer, U32 dwOutBufferSize, U32* lpBytesReturned);
typedef S32	(*ANTS_IPC_CONTROL)(S32 hLogin, U32 dwCommand, S32 lChannel,S32 nStreamIdx, VOID *pInBuf, U32 dwInLength, VOID *pOutBuf, U32 dwOutLength);
typedef S32	(*ANTS_IPC_CAPTUREPICTURE)(S32 hLogin,S32 lChannel,S32 nStreamIdx, S8 *sPicBuffer, U32 dwBufferSize, U32 *lpReturnSize);
typedef S32	(*ANTS_IPC_GETABILITY)(S32 hLogin, U32 dwAbilityType, S8 *pInBuf, U32 dwInLength, S8 *pOutBuf, U32 dwOutLength);

//搜索回调

// bOperation:0-开始，1-停止2-清除结果
typedef S32 (*ANTS_IPC_SEARCHIPC)(S32 bOperation);
//pResult==NULL时，返回搜索总个数
//否则返回缓冲中实际存放的个数
// idx == -1 则一次返回缓冲可容个数
// idx >= 0为获取索引的一个结果
//dwResultMaxCount ：多少个sizeof(AntsIPCSearchInfo_T)
typedef S32 (*ANTS_IPC_GETSEARCHRESULT)(S32 idx,AntsIPCSearchInfo_T *pResult,U32 dwResultMaxCount);
typedef S32 (*ANTS_IPC_ModifyIPInfo)(AntsIPCIPv4_T *pIPModifyInfo);






typedef struct _tagAntsIPCFunctions
{
	U32 dwFlag[2];//fourcc("ANTSIPCP") ANTS_IPC_FLAG0,ANTS_IPC_FLAG1
	U32 dwSize;// sizeof(AntsIPCFunctions_T)
	U32 dwVersion;// ANTS_IPC_VERSION
	U32 dwMaxNum;//最大支持的IPC设备数
	S8  sProtocolName[16];// 协议名,必须
	ANTS_IPC_SEARCHIPC fSearch;
	ANTS_IPC_GETSEARCHRESULT fGetSearchResult;
	ANTS_IPC_ModifyIPInfo  fModifyIPInfo;
	ANTS_IPC_SETSTATUSCALLBACK fSetStatusCallback;
	ANTS_IPC_LOGIN fLogin;
	ANTS_IPC_LOGOUT fLogout;
	ANTS_IPC_SETALARMCALLBACK fSetAlarmCallback;
	ANTS_IPC_GETCONFIG fGetConfig;
	ANTS_IPC_SETCONFIG fSetConfig;
	ANTS_IPC_STARTREALPLAY fStartRealPlay;
	ANTS_IPC_STOPREALPLAY fStopRealPlay;
	ANTS_IPC_CONTROL fControl;
	ANTS_IPC_GETABILITY fGetAbility;
	ANTS_IPC_CAPTUREPICTURE fCapturePicture;
	// Version > 10001
	ANTS_IPC_INIT  fInit;
	ANTS_IPC_UNINIT fUnInit;
	
}AntsIPCFunctions_T;

#ifdef __cplusplus
extern "C" {
#endif
// 获取本协议支持的功能
// pFxnsBuffer: AntsIPCFunctions_T,不同的版本可能此结构大小不一样
// dwBufferSize: 指示提供缓冲的大小.当缓冲大小小于本版本协议功能结构的大小时,可以提供能接受的较低版本功能.
//如果提供的缓冲大小比最低版本需要的缓冲还小，则返回失败.
//返回:0-成功,-1失败
S32 ants_ipc_GetSupportFunctions(VOID *pFxnsBuffer,U32 dwBufferSize);

#ifdef __cplusplus
}
#endif

#endif


