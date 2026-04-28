#ifndef IPC_STORAGE_API_H
#define IPC_STORAGE_API_H


#include "time.h"
#include "pkgstreamstorage.h"
#include "ants_ipc_api.h"

#ifdef WIN32
#ifdef __cplusplus
#if defined( AVI_STORAGE_EXPORTS)
#define AAS_API extern "C" __declspec(dllexport)
#else
#define AAS_API extern "C"
#endif
#else
#define AAS_API
#endif
#define AAS_CALL __stdcall
#else/*Linux*/
#ifdef __cplusplus
#define AAS_API extern "C"
#else
#define AAS_API
#endif
#define AAS_CALL
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
#ifndef    NULL
#define	   NULL 0
#endif

#define __stdcall
#define CALLBACK
#endif


////////////////////////////////////////////////////////////////////////////////
// 宏定义
////////////////////////////////////////////////////////////////////////////////

// 输入参数
#define IN
// 输出参数
#define OUT
// 输入输出参数
#define INOUT

// 最大另存为句柄数
#define MAX_AVI_SAVEAS_HANDLE_CNT		(32)

// 最大录像文件查询句柄数
#define MAX_REC_FILE_QUERY_HANDLE_CNT	(8)


// 最大录像文件路径长度
#define MAX_REC_FILE_NAME_LEN	(64)


// 录像类型
#define ANTS_RECORDTYPE_BEGIN			(0)
#define	ANTS_RECORDTYPE_TIMER			(1)
#define	ANTS_RECORDTYPE_MOTION			(2)
#define	ANTS_RECORDTYPE_ALARM			(3)
#define	ANTS_RECORDTYPE_MOTIONORALARM	(4)
#define	ANTS_RECORDTYPE_MOTIONANDALARM	(5)
#define	ANTS_RECORDTYPE_COMMAND			(6)
#define	ANTS_RECORDTYPE_MANUAL			(7)
#define	ANTS_RECORDTYPE_ALL				(8)
#define	ANTS_RECORDTYPE_IVSDETECT		(9)
#define	ANTS_RECORDTYPE_FACE_DETECT		(10)
#define	ANTS_RECORDTYPE_FIRE_DETECT		(11)
#define	ANTS_RECORDTYPE_VIDEODIAGNOSE	(12)

#define ANTS_RECORDTYPE_END				(13)

////////////////////////////////////////////////////////////////////////////////
// 枚举定义
////////////////////////////////////////////////////////////////////////////////

// 接口返回值定义
typedef enum
{
	avis_ret_ok = 0,
	avis_ret_failed = -1000,
	avis_ret_sdk_no_init,
	avis_ret_no_mem,
	avis_ret_invalid_para,
	avis_ret_invalid_handle,
	avis_ret_invalid_file,
	avis_ret_buffer_too_small,
	avis_ret_no_handle,
	avis_ret_has_started,
	avis_ret_not_start,
	avis_ret_has_create,
	avis_ret_not_create,
	avis_ret_no_more_file,
	avis_ret_no_more_data,
}ants_avis_ret_code;

// 录像优先级
typedef enum
{
	eRecPriority_Lowest,
	eRecPriority_Low,
	eRecPriority_Normal,
	eRecPriority_High,
	eRecPriority_Highest,
}eRecPriority;

////////////////////////////////////////////////////////////////////////////////
// 结构体定义
////////////////////////////////////////////////////////////////////////////////

// 录像分区信息
typedef struct tagRecPartitionInfo
{	
	char acPath[32];						// 分区路径, 如"/dev/hda1/"或"c:/"...
	eRecPriority ePriority;					// 录像优先级, 数值越大优先级越高
	unsigned long long nRecCanUseSpace;		// 录像可使用的空间（字节）
	unsigned long long nPartitionTotalSpace;// 分区总大小（字节）
	DWORD nRecMaxSaveHour;					// 录像最大保存时间 0xFFFFFFFF-分区满停止录像； 0-分区满覆盖； other-录像最大保存时间（小时）
}PartitionInfo;

// 录像设备标识
typedef struct tagRecDeviceId
{
	char acDeviceId[32];					// 设备id，ants的设备一般传入设备序列号
}RecDeviceId;

// 录像文件信息
typedef struct tagRecFileInfo
{
	char acPath[MAX_REC_FILE_NAME_LEN];		// 录像文件名称（包括完整路径）
	unsigned long lBeginTime;				// 起始时间
	unsigned long lEndtime;					// 结束时间
	unsigned int nFileSize;					// 文件大小
	unsigned int nRecTypeMask;				// 录像类型掩码（预留）
}RecFileInfo;

/*录像片断信息结构体*/
typedef struct _PSS_REGSEG_EX
{
    time_t             tBeginTime; //起始时间点
    time_t             tEndTime;   //结束时间点
    int                iRecType;   //该时间段的录像类型
    int				   iIndexNum;    //录像索引序号
    struct _PSS_REGSEG_EX *ptNext;
} PSS_REGSEG_EX, *pPPS_REGSEG_EX;

////////////////////////////////////////////////////////////////////////////////
// 回调函数定义
////////////////////////////////////////////////////////////////////////////////

/*!
* \brief 磁盘分组满回调函数
* \param 无
* \return 存储库不检查该回调函数的返回值
*/
typedef int (*ANTS_DATAINPUT_DISKFULL_CALLBACK)();


/*!
* \brief 数据弹出回调函数
* \param hDataPopper 数据弹出器句柄
* \param iChannel 数据对应的通道
* \param bIndexFrame 是否是建立索引的帧（关键帧）
* \param wFrameType 数据对应的类型
* \param tFrameTime 数据帧的绝对时间簇，精确度为秒
* \param pBuf 数据对应的缓冲区地址
* \param dwSize 数据对应的大小
* \param EventID 发生的事件类型，如果是正常的数据弹出，EventID为DATAPOP_EVENT_NONE，其他事件时，帧相关的参数无效
* \param pContext 创建弹出器时传入的上下文
* \return 存储库不检查该回调函数的返回值
*/
typedef int (*ANTS_AVI_DATAPOP_CALLBACK)(HANDLE hDataPopper, int iChannel, BOOL bIndexFrame, unsigned short wFrameType, time_t tFrameTime, unsigned char *pBuf, unsigned long dwSize, unsigned long EventID, void *pContext);


////////////////////////////////////////////////////////////////////////////////
// 接口定义
////////////////////////////////////////////////////////////////////////////////

/*!
* \brief
* 存储系统的初始化.
* \param sDeviceSerial 当前未使用，可设置为空
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_Init(const char *sDeviceSerial);


/*!
* \brief
* 存储系统的反初始化.
* \param sDeviceSerial 当前未使用，可设置为空
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_UnInit(const char *sDeviceSerial);



/*!
* \brief 添加录像分区，如果相同路径的录像分区已经被添加了，则该接口将更新分区录像参数
* \param pInfo 欲添加的分区信息
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_AddRecPartition(PartitionInfo *pInfo);



/*!
* \brief 删除录像分区
* \param pInfo 欲删除的分区信息，注意分区路径必须和添加时的保存一致
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DelRecPartition(char *pPath);



/*!
* \brief 获取分区录像已使用空间
* \param pInfo 分区路径，必须和添加时的保存一致
* \param pRecUsedSpace 输出参数，录像已使用的空间（字节）
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetRecUsedSpace(char *pPath, unsigned long long *pRecUsedSpace);


/*!
* \brief 获取录像分区的总大小和使用的大小
* \param pInfo 分区路径，必须和添加时的保存一致
* \param lpdwAllSpace 输出参数，录像空间的大小（M字节）
* \param lpdwFreeSpace 输出参数，录像已使用的空间（M字节）
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetDiskStatus(char *pPath, unsigned int *lpdwAllSpace, unsigned int *lpdwFreeSpace);


/*!
* \brief 格式化录像分区
* \param pInfo 分区路径，必须和添加时的保存一致
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_FormatDisk(char *pPath);


/*!
* \brief 以指定的录像类型开始录像，如果该通道的录像已经开启了，则调用该接口表示设置新的录像类型
* \param iChannel 录像通道号
* \param iRecType 录像类型
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_RecStart(int iChannel, int iRecType);


/*!
* \brief 将数据写入录像通道中
* \param iChannel 录像通道号
* \param pFrameHead 帧头数据指针
* \param nHeadLen 帧头数据长度
* \param pFrameData 帧数据指针，不包含帧头
* \param nDataLen 帧数据长度，不包含帧头
* \param tFrameTime 写入帧的绝对时间簇，精确度为秒
* \param tFrameTimeInUs 写入帧的绝对时间簇，秒内的精度(微秒)
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataInput(int iChannel, char *pFrameHead, int nHeadLen, char *pFrameData, int nDataLen, DWORD dwCurTimeSec, DWORD dwCurTimeUsec);


/*!
* \brief 停止指定录像通道的录像
* \param iChannel 录像通道号
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_RecStop(int iChannel);

/*!
* \brief 获取指定时间段通道的录像的I帧数量
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间
* \param iRecType 录像类型
* \param pCount 保存返回获取录像的I帧数量
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetIFrameCount(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pCount);

/*!
* \brief 获取指定时间段通道的录像大小
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间
* \param iRecType 录像类型
* \param pSize 保存返回获取录像数据大小
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetDataSize(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pSize);

/*!
* \brief 获得指定通道在指定时间段内指定录像类型的录像片断
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间, 注意: 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
* \param iRecType 录像类型
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \param pRecSegCount 保存返回获取片断的个数
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetRecSegList(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG **pRecSegHead, int *pRecSegCount);



/*!
* \brief 释放录像片断信息列表所占内存
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_ReleaseRecSegList(PSS_REGSEG *pRecSegHead);



/*!
* \brief 获得指定通道在指定时间段内指定录像类型的录像片断
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间, 注意: 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
* \param iRecType 录像类型
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \param pRecSegCount 保存返回获取片断的个数
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetRecSegListEx(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG_EX **pRecSegHead, int *pRecSegCount);
/*!
* \brief 释放录像片断信息列表所占内存
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_ReleaseRecSegListEx(PSS_REGSEG_EX *pRecSegHead);



/*!
* \brief 创建数据弹出器
* \param nRefChan 参考通道(可以不设置，值为-1表示不设置)
* \param tStartSeekTime 初始弹出数据的时间
* \param tEndSeekTime 弹出数据的结束时间。当此时间为0时，数据进行动态弹出。如果结束时间是一个未来时间，则到达当前时间时，通过回调接口告之该事件。
					 注意: 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
* \param cbReplayCallback 设定数据弹出的回调函数接口，弹出的数据按弹出器句柄、通道、帧类型、时间簇、事件类型（跳到新的时间片/查询结束/当前数据查询完毕等）等参数形式送回放线程
* \param bDynPop 是否动态定位弹出，即播放完毕后，是否主动重新定位继续弹出。当用于回放时，设置为TRUE，用于备份时，设置为FALSE
* \param pContext 上下文，上层软件可以传入此参数，在弹出回调中会给出该参数
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \param bNeedSync 是否需要同步，回放时一般设置为true, 另存为时一般设置为false
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopCreate(HANDLE *pDataPopper, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime, ANTS_AVI_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext);



/*!
* \brief 添加弹出数据的通道
* \param hDataPopper 数据弹出器的句柄
* \param iChannel 通道号
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopChanAdd(HANDLE hDataPopper, int iChannel);



/*!
* \brief 移除弹出数据的通道
* \param hDataPopper 数据弹出器的句柄
* \param iChannel 通道号
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopChanDel(HANDLE hDataPopper, int iChannel);



/*!
* \brief 添加弹出数据的录像类型
* \param hDataPopper 数据弹出器的句柄
* \param iRecType 录像类型
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopRecTypeAdd(HANDLE hDataPopper, unsigned int iRecType);



/*!
* \brief 移除弹出数据的录像类型
* \param hDataPopper 数据弹出器的句柄
* \param iRecType 录像类型
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopRecTypeDel(HANDLE hDataPopper, unsigned int iRecType);



/*!
* \brief 数据弹出器定位(设定了参考通道，按参考通道定位，未设参考通道，定位为时序上第一个满足条件的通道)
* \param hDataPopper 数据弹出器的句柄
* \param SeekTime 设置的弹出数据的时间
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopTimeSeek(HANDLE hDataPopper, time_t SeekTime);


/*!
* \brief 设置数据弹出方向
* \param hDataPopper 数据弹出器的句柄
* \param bKeyFrame 数据弹出方向，如果为TRUE则向后弹出，如果为FALSE则向前弹出（暂未实现）
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopSetDirect(HANDLE hDataPopper, BOOL bSequential);


/*!
* \brief 数据弹出时是否只弹出关键帧
* \param hDataPopper 数据弹出器的句柄
* \param bKeyFrame 是否只弹出关键帧，如果为TRUE则只弹出关键帧，如果为FALSE则弹出所有帧。
* \param iSpeed 参数为-4 ~ 4 -1表示1/2倍速 1表示2倍速 0表示正常速度
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopSetKeyFrame(HANDLE hDataPopper, BOOL bKeyFrame, int iSpeed);


/*!
* \brief 获取弹出器备份数据的总容量(单位：字节)
* \param hDataPopper 数据弹出器的句柄
* \param pSize 总容量
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopGetDataSize(HANDLE hDataPopper, unsigned long long *pSize);


/*!
* \brief 开始数据弹出
* \param hDataPopper 数据弹出器的句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopStart(HANDLE hDataPopper);



/*!
* \brief 停止数据弹出
* \param hDataPopper 数据弹出器的句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopStop(HANDLE hDataPopper);



/*!
* \brief 释放数据弹出器
* \param hDataPopper 数据弹出器的句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopRelease(HANDLE hDataPopper);



/*!
* \brief
* 设置磁盘分组的满回调
* \param cbDiskGroupFullCallback 回调函数句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SetDiskGroupFullCallback(ANTS_DATAINPUT_DISKFULL_CALLBACK cbDiskGroupFullCallback);


/*!
* \brief 获取是否有指定天数(一般用于一个月)的录像数据
* \param tBeginTime 指定时间段的开始时间
* \param numOfdays 从起始天开始的天数(24小时)
* \param pResult 返回结果，采用位与方式bit0-bit30分别对应31天
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_JudgeIFHaveDataByDays(time_t tBeginTime, unsigned int numOfdays, unsigned int *pResult);



/*!
* \brief 将帧数据保存成avi文件
* \param pSaveHandle 输出参数，返回保存句柄
* \param pFullName 完整的文件路径
当pFullName是以".avi"结尾的合法文件名时，接口只负责存储录像数据到指定名称的avi文件中，何时切换录像文件由上层决定；
否则当pFullName是合法的路径名时，接口会在该路径下存储avi录像文件并自动切换文件
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveStart(HANDLE *pSaveHandle, char *pFullName);


/*!
* \brief 将帧数据保存成指定名称的avi文件
* \param pSaveHandle 输出参数，返回保存句柄
* \param pFullName 完整的文件路，无论输入的文件名是否以".avi"结尾，一律将avi格式的数据存储到指定名称的文件中，何时切换录像文件由上层决定
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveStart_ByName(HANDLE *pSaveHandle, char *pFullName);


/*!
* \brief 将数据写入avi文件中
* \param hSaveHandle 保存句柄，由ANTS_AviFile_SaveStart接口返回
* \param pFrameHead 帧头数据指针
* \param nHeadLen 帧头数据长度
* \param pFrameData 帧数据指针，不包含帧头
* \param nDataLen 帧数据长度，不包含帧头
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveInput(HANDLE hSaveHandle, char *pFrameHead, int nHeadLen, char *pFrameData, int nDataLen);


/*!
* \brief 停止指定录像通道的录像
* \param hSaveHandle 保存句柄，由ANTS_AviFile_SaveStart接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveStop(HANDLE hSaveHandle);



/*!
* \brief 开始查询指定通道在指定日期的录像文件。注意：正在录像的avi文件不会被返回
* \param pQuery 查询句柄，输出参数
* \param iChannel 录像通道号，从0开始，-1表示查询所有通道的录像文件
* \param tDate 录像查询日期，-1表示查询所有日期的录像文件
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_QueryRecFilesStart(OUT HANDLE *pQuery, IN int iChannel, IN time_t tDate);


/*!
* \brief 获取下一条录像文件信息
* \param hQuery 查询句柄，通过ANTS_AviFile_QueryRecFilesStart接口返回
* \param pInfo 查询到的录像文件信息
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetNextRecFile(IN HANDLE hQuery, OUT RecFileInfo *pInfo);


/*!
* \brief 停止查询录像文件
* \param hQuery 查询句柄，通过ANTS_AviFile_QueryRecFilesStart接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_QueryRecFilesStop(IN HANDLE hQuery);



/*!
* \brief 删除指定名称的录像文件
* \param pFullPath 录像文件的完整路径
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DelAviFileByName(IN char *pFullPath);


/*!
* \brief 删除指定条件的所有录像文件
* \param iChannel 录像通道号，从0开始，-1表示删除所有通道的录像文件
* \param tDate 录像查询日期，-1表示删除所有日期的录像文件
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DelAviFiles(IN int iChannel, IN time_t tDate);

#endif // end of AVI_STORAGE_API_H
