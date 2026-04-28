#ifndef _ANTS_AVI_COMMON_H_
#define _ANTS_AVI_COMMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h> 
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>

#include "ipcstorage.h"


#define INVALID_HANDLE_VALUE  (void *)0xFFFFFFFF

#define DEL_POINTER(p)			{ if(NULL != p) { delete p; p = NULL; } }
#define DEL_ARR_POINTER(arr)	{ if(NULL != arr) { delete []arr; arr = NULL; } }

	// 一天的秒数
#define DAY_SEC_CNT		(26*3600)
	// 查询文件时, 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
#define  STRIDE_EXCEED_1_DAY(start_time, end_time)			(end_time - start_time > DAY_SEC_CNT)

	// 1K的大小
#define KBYTE_UNIT      (1024)
	// 1M的大小
#define MBYTE_UNIT		(KBYTE_UNIT * 1024)
	// 1G的大小
#define GBYTE_UNIT		(MBYTE_UNIT * 1024)

// O_DIRECT requires that I/O occur in multiples of 512 bytes and from memory aligned on a 512-byte boundary because O_DIRECT performs direct memory access (DMA) straight to the backing store
#define DMA_ADDR_ALIGN	(512)

// 每次读写的数据块大小要是文件所在块设备block size的整数倍
#define	DMA_BLK_SIZE	(4096)

//写缓冲定义为64K  每一次写的大小
#define PER_WRITESIZE (128 * DMA_BLK_SIZE)

#define PER_READSIZE (2 * 16 * DMA_BLK_SIZE)

#ifdef JZT31N
//读缓冲定义为512K
#define MAX_READERBUFFERSIZE (512*1024)
#else
//读缓冲定义为1M
#define MAX_READERBUFFERSIZE (1024*1024)
#endif

#define ByteAlignmentUseIO

// 录像类型
#define	AVIS_RECORDTYPE_TIMER			(1 << 0)
#define	AVIS_RECORDTYPE_MOTION			(1 << 1)
#define	AVIS_RECORDTYPE_ALARM			(1 << 2)
#define	AVIS_RECORDTYPE_MOTIONORALARM	(1 << 3)
#define	AVIS_RECORDTYPE_MOTIONANDALARM	(1 << 4)
#define	AVIS_RECORDTYPE_COMMAND			(1 << 5)
#define	AVIS_RECORDTYPE_MANUAL			(1 << 6)
#define	AVIS_RECORDTYPE_IVSDETECT		(1 << 8)
#define	AVIS_RECORDTYPE_FACEDETECT		(1 << 9)
#define	AVIS_RECORDTYPE_FIREDETECT		(1 << 10)
#define	AVIS_RECORDTYPE_VIDEODIAGNOSE	(1 << 11)

#define AVIS_RECORDTYPE_ALL				(AVIS_RECORDTYPE_TIMER | AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | AVIS_RECORDTYPE_MANUAL \
| AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE)


#define AVI_DBG(x...)  printf("[AVI_DBG][%s:%d] ", __FUNCTION__, __LINE__); printf(x);printf("\n")
#define AVI_WAR(x...)  printf("[AVI_WAR][%s:%d] ", __FUNCTION__, __LINE__); printf(x)
#define AVI_ERR(x...)  fprintf(stderr, "[AVI_ERR][%s:%d] ", __FUNCTION__, __LINE__); fprintf(stderr, x);printf("\n")

#define OUTPUT_FUNC_LINE printf("[AVI_DBG][%s:%d] ", __FUNCTION__, __LINE__);printf("\n")


//打开大于2G的文件，需要加以下定义，并且要用lssek64
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif


typedef struct
{
	unsigned short wYear;
	unsigned short wMonth;
	unsigned short wDay;
	unsigned short wHour;
	unsigned short wMinute;
	unsigned short wSecond;
}avis_time_info;


/*录像片断信息结构体*/
typedef struct _indexlist
{
    S64 				startAddr;
	S64					filesize;
	time_t 				begin_sec;
	time_t 				end_sec;
    struct _indexlist  	*ptNext;
} avis_recfileindexlist;




//#define DEVPATH "/mnt/test.txt"  
//#define DEVPATH "/dev/mmcblk0p2"


typedef struct
{
	S64 startAddr;
	S64 filesize;
}avis_address;

typedef struct   //SD卡头，512字节
{
	char startMark[32];

	//日期索引未写到SD卡头，每次恢复的时候获取
	avis_address tRecFileIndexAddr; //精确索引
	avis_address tRecFileAddr; //录像文件地址

	S64 llSDCardSize;
	S64 llheadSize;
	S64 llRecIndexSize;
	S64 llRecFileSize;
	
	char endMark[32];
	char reserved[384];
}avis_SDCardHead;

typedef enum file_io_mode
{
	io_mode_r=0,
	io_mode_w,
	io_mode_rw,
}file_io_mode;

typedef struct
{
	int fd;
	S64 startAddr;
	S64 filesize;
	file_io_mode m_io_mode;
}avis_fileHanedle;



//每一个索引中记录的I帧的数目
#define IFRAMEOFFSETNUM 20

//精确索引
typedef struct //128字节
{
	char startMask[8];
	S64 startAddr;
	int file_size;							// 文件的大小
	int file_type;							// 文件类型
	time_t begin_sec;								
	time_t end_sec;
	int use_size;							// 文件占用的大小
	int iIndexNum;
	int iFrameOffset[IFRAMEOFFSETNUM];  //每个I帧到文件首的偏移地址
	char endMask[8];
}RecIndexInfo;


typedef struct _iFramelist
{
    RecIndexInfo		recIndexInfo;
    struct _iFramelist  	*ptNext;
} avis_iFrameindexlist;

const int REC_INDEX_NODE_LEN = sizeof(RecIndexInfo);
const int FRAME_HEAD_LEN = sizeof(AntsIPCFrameHeader_T);

int ANTSAVI_Pthread_Create(pthread_t * threadid, pthread_attr_t * attr, void*(*routine)(void*), void* param);

S64 safe_write(int fd, void *buf, S64 count);
S64 safe_read(int fd, void *buf, S64 count);

S64 ANTS_AVI_SEEK(avis_fileHanedle *fd, S64 offset, int where);
int ANTS_AVI_OPEN(avis_fileHanedle *fd, file_io_mode io_mode, BOOL io_direct);
S64 ANTS_AVI_WRITE(avis_fileHanedle *fd, void *buff, S64 count);
S64 ANTS_AVI_READ(avis_fileHanedle *fd, void *buff, S64 count);
int ANTS_AVI_CLOSE(avis_fileHanedle *fd);

BOOL IsFileOen(avis_fileHanedle *fd);

BOOL is_i_frame(int frame_type);
BOOL is_video_frame(int frame_type);
BOOL is_audio_frame(int frame_type);
BOOL is_smart_frame(int frame_type);

void sleep_ms(int ms);

int trans_to_rectype_mask(int rectype);
int trans_from_rectype_mask(int recmask);

int mktime_utc(avis_time_info *pTimeInfo, time_t *dwTime);
int gmtime_utc(time_t dwTime, avis_time_info *pTimeInfo);
int localtime_utc(time_t dwTime, avis_time_info *pTimeInfo);

BOOL compareYMD(time_t time1, time_t time2);
int compareYMD_Ex(time_t time1, time_t time2);

BOOL GetStrFromTime(time_t iTimeStamp, char *pszTime);

unsigned int get_cur_time_ms();

void AVI_PrintBuffer(void *buffer, int len);

#endif

