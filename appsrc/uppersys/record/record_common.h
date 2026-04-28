#ifndef _COMM_ERRNO_H_
#define _COMM_ERRNO_H_

#include "comm_def.h"
#include "libcommon_api.h"

// 录像类型
#define 	CV_RECORD_BEGIN					(0)
#define	CV_RECORD_TIMER					(1)
#define	CV_RECORD_MOTION				(2)
#define	CV_RECORD_ALARM				(3)
#define	CV_RECORD_MOTIONORALARM		(4)
#define	CV_RECORD_MOTIONANDALARM		(5)
#define	CV_RECORD_COMMAND			(6)
#define	CV_RECORD_MANUAL				(7)
#define	CV_RECORD_ALL					(8)
#define	CV_RECORD_IVSDETECT				(9)
#define	CV_RECORD_FACE_DETECT			(10)
#define	CV_RECORD_FIRE_DETECT			(11)
#define	CV_RECORD_VIDEODIAGNOSE		(12)
#define	CV_RECORD_PERSON_DETECT		(13)


#define  OVFS_PRINT_JSON(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		LOGI("%s\n",ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)


namespace cv_soft {

//录像触发类型，用不同的bit岔开定义(触发类型用4Bytes来区分已经捉襟见肘了，后面不够用的时候要想办法了)
enum
{
	CV_REC_TRIG_NONE				= 0,

	CV_REC_TRIG_BY_REGULAR		= 0x01,		//常规录像
	CV_REC_TRIG_BY_MD				= 0x02, 		//移动侦测录像
	CV_REC_TRIG_BY_AI				= 0x04, 		//报警录像
	CV_REC_TRIG_BY_COUNTER		= 0x08,	  //计数报警
	CV_REC_TRIG_BY_WIRE			= 0x10,			  //跨线报警
	CV_REC_TRIG_BY_REGION			= 0x20, 		//区域报警
	CV_REC_TRIG_BY_OBJECT			= 0x40, 		//物品检测
	CV_REC_TRIG_BY_SCENCE			= 0x80, 		//场景突变录像
	CV_REC_TRIG_BY_SOUND			= 0x100,    //声音告警录像
	CV_REC_TRIG_BY_SMOTION		= 0x200,
	
	CV_REC_TRIG_BY_FIRE			= 0x400,			   //火灾检测录像
	CV_REC_TRIG_BY_FACE			= 0x800,	  //人脸检测录像
	CV_REC_TRIG_BY_RECOGNITION_FACE = 0x1000,
	CV_REC_TRIG_BY_TEMPERATURE = 0x2000,
	
	CV_REC_TRIG_BY_PLATE			= 0x4000,  //车牌检测录像
	CV_REC_TRIG_BY_PLATE_WHITE	= 0x8000,	//车牌白名单
	CV_REC_TRIG_BY_PLATE_BLACK	= 0x10000,	//车牌黑名单
	CV_REC_TRIG_BY_VIDEO_DIAGNOSE = 0x20000,	//视频诊断
	CV_REC_TRIG_BY_MANUAL			= 0x40000,		//手动
	CV_REC_TRIG_BY_NET_DISCONNECT	= 0x80000, 		//网络断录像
	CV_REC_TRIG_BY_NET_IP_CONFLICT	= 0x100000,		//IP冲突录像
	CV_REC_TRIG_BY_NET_ILLEG_ACCESS	= 0x200000,		//非法访问录像
	CV_REC_TRIG_BY_NET_CLIENT_DISCONN	= 0x400000,		//客户端断开连接录像
	CV_REC_TRIG_BY_HUMANOID	            = 0x800000,		//人形
	CV_REC_TRIG_BY_RUN	                = 0x1000000,		//
	CV_REC_TRIG_BY_VIOLENTMD	        = 0x2000000,		//
	CV_REC_TRIG_BY_MAXHEIGHT	        = 0x4000000,		//
	CV_REC_TRIG_BY_HIGHDENSITY	        = 0x8000000,		//
	CV_REC_TRIG_BY_RETROGRADE	        = 0x10000000,		//
	CV_REC_TRIG_BY_SCENCECHANGE	        = 0x20000000,		//
	CV_REC_TRIG_BY_DETECT_PERSON	    = 0x40000000,	// 新人形
    CV_REC_TRIG_BY_DETECT_MOTOR	        = 0x80000000,

	CV_REC_TRIG_BY_PRE = CV_REC_TRIG_NONE,	//预录
};

enum {
    //!主码流帧类型
    CvPktError=0x00,
    CvPktIFrames=0x01,
    CvPktVIFrames=0xA1,
    CvPktAudioFrames=0x08,
    CvPktPFrames=0x09,
    CvPktBBPFrames=0x0a,
    CvPktMotionDetection=0x0b,
    CvPktDspStatus=0x0c,
    CvPktOrigImage=0x0d,
    CvPktSysHeader=0x0e,
    CvPktBPFrames=0x0f,
    CvPktSFrames=0x10,
    //!子码流帧类型
    CvPktSubSysHeader=0x11,
    CvPktSubIFrames=0x12,
    CvPktSubVIFrames=0xA2,
    CvPktSubPFrames=0x13,
    CvPktSubBBPFrames=0x14,
    //!智能分析信息帧类型
    CvPktVacEventZones=0x15,
    CvPktVacObjects=0x16,
    //!第三码流帧类型
    CvPktThirdSysHeader=0x17,
    CvPktThirdIFrames=0x18,
    CvPktThirdVIFrames=0xA8,
    CvPktThirdPFrames=0x19,
    CvPktThirdBBPFrames=0x1a,

    //!智能检测帧类型
    CvPktSmartIFrames=0x1b,
    CvPktSmartPFrames=0x1c
};

enum
{
	ALARM_TYPE_ALARMIN = 0,
	ALARM_TYPE_MOTION,
	ALARM_TYPE_VHIDE,
	ALARM_TYPE_VDIAGNOSE,
	ALARM_TYPE_COUNTER_WIRE,
	ALARM_TYPE_DETECT_WIRE,
	ALARM_TYPE_DETECT_REGION,
	ALARM_TYPE_OBJECT_REGION,
	ALARM_TYPE_SOUND_DETECT,

	ALARM_TYPE_SMART_MOTION,
	ALARM_TYPE_DETECT_FIRE,
	ALARM_TYPE_DETECT_FACE,
	ALARM_TYPE_DETECT_PLATE,
	ALARM_TYPE_RECOGNITION_FACE,
	ALARM_TYPE_TEMPERATURE,
	
	ALARM_TYPE_DISKFULL,
	ALARM_TYPE_DISKERR,
	ALARM_TYPE_CABLE_BREAK,
	ALARM_TYPE_IP_CONFLIT,
	ALARM_TYPE_ILLEG_ACCESS,
	ALARM_TYPE_UNMATCH_FORMAT,
	ALARM_TYPE_RECORD_ERR,
	ALARM_TYPE_CLIENT_BREAK,
	ALARM_TYPE_HUMANOID,
	ALARM_TYPE_RUN,
	ALARM_TYPE_VIOLENTMD,
	ALARM_TYPE_MAXHEIGHT,
	ALARM_TYPE_HIGHDENSITY,
	ALARM_TYPE_RETROGRADE,
	ALARM_TYPE_SCENCECHANGE,
	ALARM_TYPE_DETECT_PERSON,

    ALARM_TYPE_DETECT_ABSENT,//31
    ALARM_TYPE_DETECT_HELMET,
    ALARM_TYPE_HUMAN_TEMP,
    ALARM_TYPE_FACE_MASK,
    ALARM_TYPE_DETECT_MOTOR,

	ALARM_TYPE_MAX,
};

enum
{
    CV_ENC_STR_MAIN = 0,		//主码流
    CV_ENC_STR_SUB,			//子码流
    CV_ENC_STR_AUDIO,			//音频流
    CV_ENC_STR_SMART,			//智能流

    CV_ENC_STR_MAX,
};

//时间段
typedef struct
{
    U32 start_hour_min;	//开始时间	小时x100 + 分钟，例如2345表示 23:45
    U32 stop_hour_min;		//结束时间	小时x100 + 分钟，例如2345表示 23:45
}cv_time_quantum;

//录像类型查询条件
typedef enum
{
	CV_INQUIRY_EITHER,		  //任意一种类型满足
	CV_INQUIRY_ALL,			  //同时满足
}cv_inquiry_type;


//录像计划布防类型
enum
{
	CV_REC_ARMING_NONE = 0,

	CV_REC_ARMING_BY_TIMER,		//定时触发
	CV_REC_ARMING_BY_EVNET,		//事件触发
};

typedef struct
{
	cv_time_quantum		time;
	S32				arming_type;		// 1:定时触发 2:事件触发 3:定时和事件触发
	S32				recordmask;
}cv_record_arming;

//录像日布防计划
typedef struct
{
	S32 bAlldayRec;					//全天录像使能
	cv_record_arming arming[CV_MAX_ARMING_TIME_QUANTUM];
}cv_record_arming_day;

//本地录像计划
typedef struct
{
	cv_record_arming_day monday;
	cv_record_arming_day tuesday;
	cv_record_arming_day wednesday;
	cv_record_arming_day thursday;
	cv_record_arming_day friday;
	cv_record_arming_day saturday;
	cv_record_arming_day sunday;
}cv_local_rec_plan;

enum
{
	STOP_REC = 0,
	REGULAR_REC,
	MANUAL_REC,
};

typedef enum
{
    CV_SUCCESS = 0,	//函数成功

    //------------------------------- Record ------------------------
    CV_ERR_REC_BASE	 		= 0x40050000,
    CV_ERR_REC_UNKNOWN 		= CV_ERR_REC_BASE + 0x01,
    CV_ERR_REC_OPERATE_FAIL 	= CV_ERR_REC_BASE + 0x02,
    CV_ERR_REC_INVALID_PARA 	= CV_ERR_REC_BASE + 0x03,
    CV_ERR_REC_NOT_INIT		= CV_ERR_REC_BASE + 0x04,
    CV_ERR_REC_MALLOC_FAIL	= CV_ERR_REC_BASE + 0x05,
    CV_ERR_REC_CLOSE_WAIT         = CV_ERR_REC_BASE + 0x06,
}CV_ERR;

//事件信息使用者ID
typedef enum
{
    CV_EVENT_USER_UNKNOW = 0x0,                                 //注:若添加用户枚举必须添加到此枚举下方

    CV_EVENT_USER_LOCAL_GUI,                                      //本地GUI
    CV_EVENT_USER_REMOTE,                                            //远程设备(CMS, IE等)

    CV_EVENT_USER_MAX,                                                  //用户最大值(注:若添加用户枚举必须添加到此枚举上方)
}cv_event_user_id;

//事件主类型(千万不要改变枚举值)
typedef enum
{
    CV_EVENT_MAIN_TYPE_UNKNOW = 0x0,  //未知主类型(代码中不应该出现此类型)
    CV_EVENT_MAIN_TYPE_ANY,                    //任意主类型

    CV_EVENT_CH_EVENT,                               //通道相关事件
    CV_EVENT_DISK_EVENT,                           //硬盘相关事件
    CV_EVENT_NET_EVENT,                            //网络相关事件
    CV_EVENT_ALARM_EVENT,                      //报警相关事件

    CV_EVENT_MAIN_MAX,                            //事件主类型最大值，代码中不应该出现此类型
}cv_event_main_type;

//事件子类型(千万不要改变枚举值)
typedef enum
{
    CV_EVENT_SUB_TYPE_UNKNOW = 0x0,               //未知子类型(只有主类型为CV_EVENT_MAIN_TYPE_ANY才允许出现此子类型)
    CV_EVENT_SUB_TYPE_ANY,                                 //任意子类型
    
    /*通道子事件*/
    //切记:添加通道类子事件必须在枚举CV_EVENT_MD2下方和枚举CV_EVENT_SMART_VIDEO_DIAGNOSE上方
    CV_EVENT_MD,                                                     //移动侦测
    CV_EVENT_OD,                                                     //视频遮挡
    CV_EVENT_VIDEO_LOST,                                      //视频丢失
    CV_EVENT_SMART_OBJECT_COUNT,                   //目标计数
    CV_EVENT_SMART_CROSS_LINE,                        //跨线检测
    CV_EVENT_SMART_REGION_DETECT,                 //区域检测 
    CV_EVENT_SMART_GOODS_DETECT,                  //物品检测
    CV_EVENT_SMART_SCENE_CHNAGE,                  //情景变换
    CV_EVENT_SMART_SOUND_DETECT,                  //声音检测
    CV_EVENT_SMART_MOTION,                    		//智能移动侦测
    CV_EVENT_SMART_FIRE_DETECT,                     //火灾检测
    CV_EVENT_SMART_FACE_DETECT,                    //人脸检测
    CV_EVENT_SMART_RECOGNITION_FACE,		//人脸识别
    CV_EVENT_TEMPERATURE,					//温度报警
    CV_EVENT_SMART_CAR_PLATE_DETECT,         //车牌检测
    CV_EVENT_SMART_CAR_PLATE_BLACK,          //车牌黑名单
    CV_EVENT_SMART_CAR_PLATE_WHITE,          //车牌白名单    
    CV_EVENT_SMART_VIDEO_DIAGNOSE,            //视频诊断
    CV_EVENT_SMART_HUMANOID       ,// 人形检测
	CV_EVENT_SMART_RUN            ,
	CV_EVENT_SMART_VIOLENTMD      ,
	CV_EVENT_SMART_MAXHEIGHT       ,
	CV_EVENT_SMART_HIGHDENSITY     ,
	CV_EVENT_SMART_RETROGRADE      ,
	CV_EVENT_SMART_SCENCECHANGE    ,
	CV_EVENT_SMART_DETECT_PERSON    ,// 人形检测(新)
	CV_EVENT_SMART_DETECT_ABSENT,//
    CV_EVENT_SMART_DETECT_HELMET,
    CV_EVENT_SMART_HUMAN_TEMP,
    CV_EVENT_SMART_FACE_MASK,
    CV_EVENT_SMART_DETECT_MOTOR,

    /*磁盘子事件*/
    //切记:添加磁盘类子事件必须在枚举CV_EVENT_DISK_FULL下方和枚举CV_EVENT_DISK_SMART_ERROR上方
    CV_EVENT_DISK_FULL,                                     //磁盘满
    CV_EVENT_DISK_LOST,                                    //磁盘丢失
    CV_EVENT_DISK_ERROR,                                //磁盘错误
    CV_EVENT_DISK_DISCONNECT,                     //磁盘掉线(网络磁盘)
    CV_EVENT_DISK_READ_ONLY,                       //磁盘只读
    CV_EVENT_DISK_RAID_DEGRADED,              //磁盘降级(RAID)
    CV_EVENT_DISK_RAID_FAULT,                     //磁盘下线(RAID)
    CV_EVENT_DISK_SMART_ERROR,                 //磁盘Smart信息错误

    /*网络子事件*/
    //切记:添加网络类子事件必须在枚举CV_EVENT_NET_DISCONNECT下方和枚举CV_EVENT_NET_IP_CONFLICT上方
    CV_EVENT_NET_DISCONNECT,                        //网络断开
    CV_EVENT_NET_IP_CONFLICT,                      //IP冲突
    CV_EVENT_NET_ILLEG_ACCESS,			//非法访问
    CV_EVENT_NET_CLIENT_DISCONNECT,	//客户端断开连接

    /*报警子事件*/
    //切记:添加报警类子事件必须在枚举CV_EVENT_LOCAL_ALRAM_IN下方和枚举CV_EVENT_REMOTE_ALARM_IN上方
    CV_EVENT_LOCAL_ALRAM_IN,                      //本地报警输入
    CV_EVENT_REMOTE_ALARM_IN,                  //远端报警输入

    CV_EVENT_SUB_MAX,                                    //事件子类型最大值,代码中不应该出现此子类型(注:若添加子类型枚举则添加在此枚举上方)
}cv_event_sub_type;

//事件三级子类型(千万不要改变枚举值)
typedef enum
{
    CV_EVENT_THIRD_TYPE_UNKNOW = 0x0,          //未知三级类型(子类型不存在三级类型或者三级类型未知的情况下使用此类型)
    CV_EVENT_THIRD_TYPE_ANY,                            //任意子类型(只有存在三级类型才允许使用该枚举)    

    /*声音报警三级事件*/
    //切记:添加声音三级事件必须在枚举CV_EVENT_SMART_BABY_CRY下方和枚举CV_EVENT_SMART_EXPLOSION上方
    CV_EVENT_SMART_BABY_CRY,                           //婴儿哭声
    CV_EVENT_SMART_SCREAM,                               //尖叫声
    CV_EVENT_SMART_GUNSHOTS,                          //枪声
    CV_EVENT_SMART_EXPLOSION,                        //爆炸声

    /*视频诊断三级事件*/
    //切记:添加是视频诊断三级事件必须在枚举CV_EVENT_SMART_VIDEO_DIM下方和枚举CV_EVENT_SMART_VIDEO_PTZ_ABNORMAL上方
    CV_EVENT_SMART_VIDEO_DIM,                         //视频昏暗
    CV_EVENT_SMART_VIDEO_BRIGHT,                  //视频过亮
    CV_EVENT_SMART_VIDEO_FRINGE,                  //视频条纹
    CV_EVENT_SMART_VIDEO_SNOW,                    //视频雪花
    CV_EVENT_SMART_VIDEO_OD,                         //视频遮挡
    CV_EVENT_SMART_VIDEO_FREEZE,                  //视频冻结
    CV_EVENT_SMART_VIDEO_LOST,                     //视频丢失
    CV_EVENT_SMART_VIDEO_OBSCURE,             //视频模糊            
    CV_EVENT_SMART_VIDEO_SHAKE,                  //视频抖动
    CV_EVENT_SMART_VIDEO_COLOR_CAST,      //视频偏色
    CV_EVENT_SMART_VIDEO_PTZ_ABNORMAL, //PTZ异常

    CV_EVENT_THIRD_MAX,                                  //事件三级类型最大值,代码中不应该出现此类型
}cv_event_third_type;

//报警输入源
typedef struct
{
    U32 is_local;                                                       //是否为本地报警(通过该值确定src的取值)
    
    union{
        U32 ch_idx;                                                      //远程触发的报警使用通道号描述
        U32 pin_idx;                                                    //本地触发的报警使用本地报警引脚编号描述
    }src;
}cv_alarm_id;

//事件类型
typedef struct
{
    cv_event_main_type main_type;                            //事件主类型
    cv_event_sub_type   sub_type;                              //事件子类型
    cv_event_third_type third_type;                            //事件三级类型
}cv_event_type;

//事件描述
typedef struct
{
    U32 is_local;                                                                     //是否为本地事件    
    
    union{
        U32 ch_idx;                                                                   //通道号相关事件描述
//        cv_disk_id disk_describe;                                                    //磁盘相关事件描述 
//        cv_net_describe net_describe;                                            //网络相关事件描述
//        cv_alarm_describe alarm_describe;                                    //报警相关事件描述
    }describe;
}cv_event_describe;

//事件信息(类型 +  描述)
typedef struct
{
    cv_event_type event_type;                //类型
    cv_event_describe event_describe;      //描述
}cv_event_info;

//事件ID(该结构体与cv_event_info大部分相同，之所以分开为两个结构体是为了避免使用者产生混淆,如果
//都采用cv_event_info，调用者在调用时必须明白哪些参数有效哪些参数无效，太容易犯错了)
typedef struct
{
    cv_event_type event_type;                                                 //事件类型
    
    union{
        U32 ch_idx;																//通道号相关事件ID
        cv_alarm_id alarm_id;                                                         //报警输入相关事件ID
    }event_id;
}cv_event_id;

//事件详细信息(时间信息+ 额外信息)
typedef struct
{
    //事件描述
    cv_event_info event_info;

    //触发标志
    U32 is_trigger;

    //事件开始时间
    time_t start_time;

    //事件结束时间(初始化时stop_time = start_time).
    //注意:a>事件触发但是并未结束该值是不准确的，这个时候应该忽略此项.
    //              b>事件触发但在结束前，用户更改了系统时间导致结束时间<开始时间，该值也不准确,但是没有办法，只能认为结束时间=开始时间
    time_t stop_time;
}cv_event_detail_info;

typedef struct _tagAudioHeader{
    char cCodecId;			//!音频编码类型
    char cSampleRate;			//!采样率
    char cBitRate;				//!比特率
    char cChannels;			//!通道数
    char cResolution;			//!分辨力
    char cResv[3];				//!保留位
}CV_AUDIO_HEADER,*LPCV_AUDIO_HEADER;

typedef struct _tagVideoHeader{
    unsigned short usWidth;				//!视频宽度
    unsigned short usHeight;				//!视频高度
    char cCodecId;				//!视频编码类型
    char cResv[3];					//!保留位
}CV_VIDEO_HEADER,*LPCV_VIDEO_HEADER;

typedef struct _tagCvFrameHeader{
    unsigned int uiStartId;					//!帧同步头
    unsigned int uiFrameType;				//!帧类型
    unsigned int uiFrameNo;					//!帧号
    unsigned int uiFrameTime;				//!UTC时间
    unsigned int uiFrameTickCount;			//!毫秒为单位的毫秒时间
    unsigned int uiFrameLen;				//!帧载长度
	//!联合体,用于存储音频帧或是视频帧信息
    union
    {
    	CV_AUDIO_HEADER struAudioHeader;	//!音频帧信息
    	CV_VIDEO_HEADER struVideoHeader;	//!视频帧信息
    }uMedia;
    unsigned int ucReserve;				//相对时间戳，必须填充，NVR根据这个来累加计算本地时间
}CvFrameHeader,*pCvFrameHeader;

typedef struct
{
	U32								Channel;
	Common_Time_T					StartTime;
	Common_Time_T					StopTime;
	U32								StreamMask;
	U32								QueryMode;
}CV_RECORD_QUESTPARA;

typedef struct
{
	//S8								sFileName[100];
	Common_Time_T					StartTime;
	Common_Time_T					StopTime;
	U32								FileSize;
	U32								FileType;
	U32								key;
	U32								blocked;
}CV_RECORD_SEGDATA,*LPCV_RECORD_SEGDATA;


}//namespace cv_soft {
#endif //#ifndef _COMM_ERRNO_H_

