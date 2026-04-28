
#ifndef COMMON_MEDIA_STRUCT_H_
#define COMMON_MEDIA_STRUCT_H_

#define OVFS_FRAME_STARTCODE 					(0xAB010000)
#define OVFS_FILE_STARTCODE 				   	(0xAA010000)
#define OVFS_MOTION_STARTCODE 				   	(0xAC010000)
#define OVFS_APP_STARTCODE 				   		(0xAD010000)


typedef enum {
	//!主码流帧类型
	Ovfs_FrameType_Error=0x00,
	Ovfs_FrameType_IFrames=0x01,
	Ovfs_FrameType_VIFrames=0xA1,    // add by  longzhou 20171201
	Ovfs_FrameType_AudioFrames=0x08,
	Ovfs_FrameType_PFrames=0x09,
	Ovfs_FrameType_BBPFrames=0x0a,
	Ovfs_FrameType_MotionDetection=0x0b,
	Ovfs_FrameType_DspStatus=0x0c,
	Ovfs_FrameType_OrigImage=0x0d,
	Ovfs_FrameType_SysHeader=0x0e,
	Ovfs_FrameType_BPFrames=0x0f,
	Ovfs_FrameType_SFrames=0x10,
	//!子码流帧类型
	Ovfs_FrameType_SubSysHeader=0x11,
	Ovfs_FrameType_SubIFrames=0x12,
	Ovfs_FrameType_SubVIFrames=0xA2,
	Ovfs_FrameType_SubPFrames=0x13,
	Ovfs_FrameType_SubBBPFrames=0x14,
	//!智能分析信息帧类型
	Ovfs_FrameType_VacEventZones=0x15,
	Ovfs_FrameType_VacObjects=0x16,
	//!第三码流帧类型
	Ovfs_FrameType_ThirdSysHeader=0x17,
	Ovfs_FrameType_ThirdIFrames=0x18,
	Ovfs_FrameType_ThirdVIFrames=0xA8,
	Ovfs_FrameType_ThirdPFrames=0x19,
	Ovfs_FrameType_ThirdBBPFrames=0x1a,

	//!智能检测帧类型
	Ovfs_FrameType_SmartIFrames=0x1b,
	Ovfs_FrameType_SmartPFrames=0x1c,
	Ovfs_FrameType_PlateInfoFrames=0x1d,
	// APP metedata帧.
    Ovfs_FrameType_AppFrames=0x20,
    // AntsComb 帧
    Ovfs_FrameType_AntsCombFrames=0x21,

    Ovfs_FrameType_HorizonSmart_Frame=0x31,
    Ovfs_FrameType_HorizonSmart_Features=0x32,
    Ovfs_FrameType_HorizonSmart_Snap=0x33,
    //Rtsp Url 中的 MediaType取值  [1, 15] , 即Ovfs_FrameType_HorizonSmart_ & 0xF 后，多个类型或操作组合
    // 1 = 1 << 0 表示其他类型帧，一般为普通音视频帧
    // 2 = (1 << (0xF & Ovfs_FrameType_HorizonSmart_Frame ))
    // 15 = (1 << 0)
    //               | (1 << (0xF & Ovfs_FrameType_HorizonSmart_Frame ))
    //               | (1 << (0xF & Ovfs_FrameType_HorizonSmart_Features ))
    //               | (1 << (0xF & Ovfs_FrameType_HorizonSmart_Snap ))
    Ovfs_FrameType_ZkTrans_Frame=0x40,

}Ovfs_FrameType_E;

typedef struct _tagOvfs_AudioHeader{
	char cCodecId;			//!音频编码类型
	char cSampleRate;			//!采样率 单位KHz
	char cBitRate;				//!比特率 单位Kbps
	char cChannels;			//!通道数
	char cResolution;			//!分辨力
	char cResv[3];				//!保留位
}Ovfs_AudioHeader_T;

typedef struct _tagOvfs_VideoHeader{
	unsigned short usWidth;				//!视频宽度
	unsigned short usHeight;				//!视频高度
	char cCodecId;				//!视频编码类型
	char cColorSpace;         // 0-yuv420,1-yuv422,2-444
	char cResv[2];					//!保留位
}Ovfs_VideoHeader_T;

typedef struct _tagOvfs_MDEVTHeader{
	unsigned short usWidth;				//!视频宽度
	unsigned short usHeight;				//!视频高度
	char cMdAppear;				//!底层检测算法是否判定有移动发生
	char cResv[3];					//!保留位
}Ovfs_MDEVTHeader_T;

typedef struct _tagSmartHeader{
	int ivs_node_num;//ants_ivs_type 结果节点个数
	char resv[4]; //保留位
}ANTS_SMART_HEADER,*LPANTS_SMART_HEADER;

typedef struct _tagOvfs_FrameHeader{
	unsigned int uiStartId;					//!帧同步头OVFS_FRAME_STARTCODE
	unsigned int uiFrameType;				//!帧类型
	unsigned int uiFrameNo;					//!帧号
	unsigned int uiFrameTime;				//!UTC时间
	unsigned int uiFrameTickCount;			//!毫秒为单位的毫秒时间
	unsigned int uiFrameLen;				//!帧载长度
										//!联合体,用于存储音频帧或是视频帧信息
	union {
		Ovfs_AudioHeader_T struAudioHeader;	//!音频帧信息
		Ovfs_VideoHeader_T struVideoHeader;	//!视频帧信息
		Ovfs_MDEVTHeader_T struMdevtHeader; // ! 移动侦测信息
		
		ANTS_SMART_HEADER smart_header; //智能检测帧信息
	}uMedia;
	unsigned int dwTimeStamp;				//!相对时间戳 ms * 90		
}Ovfs_FrameHeader_T;

typedef enum{
	Ovfs_VoiceCodecID_OggVorbis=0,
	Ovfs_VoiceCodecID_G711A=1,
	Ovfs_VoiceCodecID_G711U=2,
	Ovfs_VoiceCodecID_G722Ex=3,
	Ovfs_VoiceCodecID_G726Ex=4,
	Ovfs_VoiceCodecID_AAC=5,
	Ovfs_VoiceCodecID_ADPCM=8,
}Ovfs_VoiceCodecID_E;


typedef enum {
	Ovfsmid_VideoCodecID_H264_hisi=1,// 相对时间戳无效，不再使用
	Ovfsmid_VideoCodecID_MJPEG_hisi=2,// 相对时间戳有效
	Ovfsmid_VideoCodecID_H264_hisi_high=4,// 相对时间戳无效，不再使用
	Ovfsmid_VideoCodecID_H264_hisi_RTP=8, // 相对时间戳有效
	Ovfsmid_VideoCodecID_H264_RTP_PACK=9, // 
	Ovfsmid_VideoCodecID_H265 = 16,
	Ovfsmid_VideoCodecID_H265_RTP_PACK=17, // 
    Ovfsmid_VideoCodecID_SVAC=20, // SVAC
}Ovfsmid_VideoCodecID_E;

typedef int                     SMT_S32;
typedef unsigned int            SMT_U32;


#define MAX_SHAPE_NUM (5)

// face
typedef struct 
{
    SMT_U32 dwMagic;
    SMT_U32 dwVersion;
    SMT_U32 dwId;
    SMT_U32 dwDataLens;
    SMT_U32 dwType;
}ANTS_COM_SMART_NODE_INFO;

typedef struct
{
    SMT_S32 result_cnt;
    SMT_S32 start_seek;
    SMT_S32 data_len;
    
    SMT_S32 pic_start_seek;
    SMT_S32 pic_data_len;

    SMT_S32 face_rec_start_seek;
    SMT_S32 face_rec_data_len;
}ANTS_COM_SFRAME_FACE_RESULT;

typedef struct
{
	unsigned short numerator;						
	unsigned short denominator;						
}relative_val;


typedef struct
{
	relative_val x;							
	relative_val y;							
	relative_val w;							
	relative_val h;							
}faceRect;


typedef struct
{
	relative_val shape[MAX_SHAPE_NUM << 1];
}faceShape;

// 单个人脸信息
typedef struct  
{
	faceRect face;							
	faceShape shape;						
	int score;								
	int id;								
}faceInfo;
#endif
