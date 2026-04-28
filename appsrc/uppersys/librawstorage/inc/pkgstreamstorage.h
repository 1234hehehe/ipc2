#ifndef PKGSTREAMSTORAGESDK_H
#define PKGSTREAMSTORAGESDK_H

#ifdef WIN32
	#ifdef __cplusplus
		#if defined( MAVS_EXPORTS)
			#define PSS_API extern "C" __declspec(dllexport)
		#else
			#define PSS_API extern "C"
		#endif
	#else
		#define PSS_API
	#endif
	#define API_CALL __stdcall
#else/*Linux*/
	#ifdef __cplusplus
		#define PSS_API extern "C"
	#else
		#define PSS_API
	#endif
	#define API_CALL
#endif

#include "time.h"


#ifndef HANDLE_T
#define	HANDLE_T
typedef void* HANDLE;
#endif

#ifndef BOOL_T
#define	BOOL_T
typedef int BOOL;
#endif

#ifndef DWORD_T
#define	DWORD_T
typedef unsigned long DWORD;
#endif

#define TRUE 		1
#define FALSE 		0

/*限制硬盘个数*/
#ifndef MAX_DISK_NUM
#define MAX_DISK_NUM   24   // MAX 8 SATA disk , 16 USB device
#define MAX_SATA_DISK  24
#endif

/*通道数限制*/
#define PSS_MAX_GROUP_CHANNEL     64      //单硬盘分组最大录像通道数
#define PSS_MAX_CHANNEL           64      //最大录像通道数
#define PSS_MAX_GROUP             16      //最大硬盘分组数
#define PSS_MAX_GROUP_PART        64      //分组含有的最大分区数

/*定义帧类型*/
#define PSS_FRAMETYPE_AV          0x8000  //音视频数据帧
#define PSS_FRAMETYPE_PICTURE     0x8001  //图片数据帧
#define PSS_FRAMETYPE_USER        0x8002  //用户自定义数据帧
#define PSS_FRAMETYPE_RECSTART    0x8003  //通道启动录像控制帧
#define PSS_FRAMETYPE_RECSTOP     0x8004  //通道停止录像控制帧
#define PSS_FRAMETYPE_POWERON     0x8005  //掉电控制帧

/*用户自定义帧类型范围*/
#define PSS_FT_USER_START		    0x0
#define PSS_FT_USER_END		      0x7FFF

/*磁盘分组轮转类型*/
#define PSS_CYCLEMODE_STOP        0x0      //硬盘分组写满后停止
#define PSS_CYCLEMODE_COVER       0x1      //硬盘分组写满后覆盖

/*查询器数限制*/
#define PSS_MAX_RECSEGREPORTER    32       //最多支持的录像片段查询器个数
#define PSS_MAX_CHANRSPSUPPORT    16       //一个录像片段查询器最大支持的同时查询的通道数

/*弹出器数限制*/
#define PSS_MAX_DATAPOPER           32     //最多支持的数据弹出器个数
#define PSS_MAX_CHANDATAPOPSUPPORT  16     //一个数据弹出器最大支持的同时进行输出弹出的通道数

/*备份器数限制*/
#define PSS_MAX_DATABACKUP          16     //最多支持的数据备份器个数

/*字符个数最大定义*/
#define MAX_STRING_LEN            24    // 字符串最多字符个数
#define MAX_GROUP_NAME            12    // 磁盘组名最大的字符个数
#define PSS_MAX_DISK_PARTITIONS   14    // 磁盘最大可用分区个数

/*数据弹出器回调函数中事件类型*/
#define DATAPOP_EVENT_NONE      0x00 //无通知事件
#define DATAPOP_EVENT_TIME_JUMP 0x01 //跳到新的时间片
#define DATAPOP_EVENT_TIME_OVER 0x02 //指定的时间片查询回调结束
#define DATAPOP_EVENT_TIME_CURR 0x03 //当前数据查询完毕，即播放时间已经到达最新录像的数据时间
#define DATAPOP_EVENT_DISK_ERR  0x04 //异常结束事件，比如read或者seek硬盘失败，把该异常回调通知用户

/*数据备份器回调函数中事件类型*/
#define DATABACKUP_EVENT_NONE           0x00 //无通知事件
#define DATABACKUP_EVENT_DISK_READ_ERR  0x04 //异常结束事件，比如read或者seek硬盘失败，把该异常回调通知用户
#define DATABACKUP_EVENT_DISK_WRITE_ERR 0x05 //异常结束事件，比如write或者seek硬盘失败，把该异常回调通知用户

typedef enum pkgstreamErr
{
    INIT_ERR = 0x01,               //初始化出错
    HARDDISK_ERR,                  //硬盘出错
    //往后添加
}PKGSTREAMERR;

/* 磁盘分组类型 */ 
typedef enum pkgstreamgrouptype
{
    DISKGROUP_NORMAL = 0x01,  //普通类型分组
    DISKGROUP_OVERPLUS,       //冗余类型分组
    DISKGROUP_BACKUP          //备份类型分组
} PKGSTREAMGROUPTYPE;

/* 主分区系统类型 */ 
enum MainType
{
    Empty = 0x00,
    Fat12 = 0x01,
    Fat16 = 0x06,
    NTFS = 0x07,
    Fat32 = 0x0b,
    Linux = 0x83
};

/* 扩展分区系统类型 */
enum ExtendType
{
    Extended = 0x05,
    WinExt = 0x0f,
    LinuxExtend = 0x85
};


/* 磁盘分区信息相关的结构体 */
typedef struct tag_DISK_PARTITION_INFO
{
    char acPartName[12];
    long long dwStart;        // 分区的起始逻辑扇区
    long long  dwPartSize;    // 分区的总空间(扇区数)
} T_DISKPARTITIONINFO;

/* 磁盘信息相关的结构体 */
typedef struct _DISK_DEVICE_INFO
{
    char acModelName[MAX_STRING_LEN];   // 磁盘的型号
    char acSerialNo[MAX_STRING_LEN];    // 磁盘的序列号
    int  iParts;                        // 可用分区数
    unsigned long  dwTotal_Disk_Space;  // 总的磁盘空间
    T_DISKPARTITIONINFO  atDiskPartInfo[PSS_MAX_DISK_PARTITIONS];  // 磁盘各Linux分区信息(不包含扩展分区)
} T_DISKINFO;

/* 磁盘做分区时，传入的分区参数信息相关的结构体 */
typedef struct _MAKE_PART_PARAM_INFO
{
    int  iPartNum;                       // 需要分区的个数
    unsigned char ucExtType;             // 扩展分区类型
    struct PART_PARM
    {
        int  iPartSize;                  // 分区的大小(用MB表示)
        unsigned char ucSysType;         //分区的系统类型
    }tPartParm[PSS_MAX_DISK_PARTITIONS];
} T_PARTPARAMINFO;

/*录像片断信息结构体*/
typedef struct _PSS_REGSEG
{
    time_t             tBeginTime; //起始时间点
    time_t             tEndTime;   //结束时间点
    int                iRecType;   //该时间段的录像类型
    struct _PSS_REGSEG *ptNext;
} PSS_REGSEG, *pPPS_REGSEG;

/* 分组上的分区信息相关的结构体 */
typedef struct _PSS_PART_INFO
{
    char acModelName[MAX_STRING_LEN];   // 分区所在磁盘的型号
    char acSerialNo[MAX_STRING_LEN];    // 分区所在磁盘的序列号
    int iDevNo;                         // 该分区在这个磁盘的分区号(从1开始)
    char  acPartName[12];               // 分区名(分区的结点名)
    int iState;                         // 分区的状态，用来标记分区是否是新分区
    long long  dwStart;             // 分区起始扇区
    long long  dwPartSize;          // 分区大小(扇区数)
    //	char acReserved[];
} PSS_PARTINFO;

/*返回给用户的磁盘分组信息结构体*/
typedef struct _PSS_DISK_GROUP_INFO
{
    char  acGroupName[MAX_GROUP_NAME];   // 分组名
    time_t tCreateTime;                  // 分组创建时间
    int  iPartNum;                       // 组包含的分区个数
    int iValidPartNum;                   // 有效分区数
    int iCycleMode;                      // 轮转方式
    int iGroupType;                      // 组类型，包括普通、冗余、备份三种
    PSS_PARTINFO  atPartInfo[PSS_MAX_GROUP_PART];// 组上分区信息
    //	char acReserved[];
    int iGroupFull;                      //分组是否已经录满(只有在属性是录满停止时有效)1表示录满，0为没有录满
}PSS_DISKGROUPINFO;

/* 获取最新数据位置的结构体 */
typedef struct _MOST_NEW_DATA_POS
{
    char acModelName[MAX_STRING_LEN];   // 分区所在磁盘的型号
    char acSerialNo[MAX_STRING_LEN];    // 分区所在磁盘的序列号
    int iDevNo;                         // 该分区在这个磁盘的实际分区号(从1开始, 为主分区或逻辑分区)
    char  acPartName[12];               // 分区名
    time_t tNewPosTime;                 // 写入最新数据位置的时间
    int  iBigBlockNo;                   // 16G块号
    int  iSmallBlockNo;                 // 64M块号
} T_MOSTNEWDATAPOS;

/* 硬盘坏块分布结构体 */
typedef struct _BAD_BLOCK_INFO
{
    char acPartName[12]; // 分区名
    int iBigBlockNo;     // 大块号
    int iSmallBlockNo;   // 小块号
    int aiBadStatus[2];  // 表示小块中的64M坏信息,1bit表示1M
    struct _BAD_BLOCK_INFO *ptNext;
} T_BADBLOCKINFO;

/*系统错误码定义*/
#define PKG_ERR_NO                    0x00000000

#define PKG_ERR_HANDLE_ALLOC_ERROR    0xc0000100
#define PKG_ERR_DDRAW_CREATE_FAILED   0xc0000101
#define PKG_ERR_RUNTIME_ERROR         0xc0000102
#define PKG_ERR_INVALID_HANDLE        0xc0000103
#define PKG_ERR_WAIT_TIMEOUT          0xc0000104
#define PKG_ERR_INVALID_ARGUMENT      0xc0000105
#define PKG_ERR_INVALID_FILENAME      0xc0000108
#define PKG_ERR_TMMAN_FAILURE         0xC0000109
#define PKG_ERR_OUTOF_MEMORY          0xc000010A
#define PKG_ERR_NOT_SUPPORT           0xc000010C
#define PKG_ERR_SYS_NOT_INIT          0xc000010D
#define PKG_ERR_CHANNEL_NOT_REC       0xc000010E
#define PKG_ERR_DISK_OPEN             0xc000010F
#define PKG_ERR_DISK_SEEK             0xc0000110
#define PKG_ERR_DISK_FULL             0xc0000111

#define PKG_ERR_GET_BUF               0xc0000112
#define PKG_ERR_REACH_MOST_NEW_POS    0xc0000113
#define PKG_USER_KEYPKG_GET_FINISH    0xc0000114  //用来表示用户关键帧查询结束
#define PKG_USER_KEYPKG_FIND_OK       0xc0000115  //用于在用户关键帧查询回调中控制查询结束
#define PKG_ERR_DISK_NOT_EXIST        0xc0000116  //硬盘分区可以OPEN但是无法访问
#define PKG_ERR_DISK_HEADAREA_NOT_USEABLE    0xc0000117 //硬盘分区的头部区域有坏扇区，无法使用
#define PKG_ERR_DISK_INDEXAREA_TOO_MUCH      0xc0000118 //硬盘分区的所有大块索引区都有坏块，无法使用

/* 报告出错类型，出错文件名，报错代码所在行 */
typedef int (API_CALL *ANTS_PKG_SYSERR_CALLBACK)( PKGSTREAMERR nPkgErr, char *sFileName, char *Function, int nLine);

/*!
 * \brief 存储库硬盘访问异常错误回调函数
 * \param acDiskModelName 硬盘型号
 * \param acDiskSerialNo 硬盘序列号
 * \param busNum 硬盘总线编号
 * \param dev 分区节点
 * \param errtype 异常错误类型
 * \return
 */
typedef int (*ANTS_PKG_DISKERR_CALLBACK)(char *acDiskModelName, char *acDiskSerialNo, unsigned int busNum, char *dev, int errtype);

/*!
 * \brief 录像片段回调函数
 * \param hRecSegReporter 录像片段报告器句柄
 * \param iChannel 该录像片段对应的通道
 * \param tBeginTime 该录像片段对应的起始时间
 * \param tEndTime 该录像片段对应的结束时间
 * \param RecType 该录像片段对应的录像类型
 * \param pChContext 暂时没用
 * \return
 */
typedef int (*ANTS_REC_SEGMENT_CALLBACK)(HANDLE hRecSegReporter, int iChannel, time_t tBeginTime, time_t tEndTime, unsigned short RecType, void *pChContext);

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
 * \return
 */
typedef int (*ANTS_REPLAY_DATAPOP_CALLBACK)(HANDLE hDataPopper, int iChannel, BOOL bIndexFrame, unsigned short wFrameType, time_t tFrameTime, unsigned char *pBuf, unsigned long dwSize, unsigned long EventID, void *pContext);

/*!
 * \brief 用户索引信息判别定位条件的回调函数
 * \param hDataPopper 数据弹出器句柄
 * \param iChannel 该用户数据对应的通道
 * \param UserIndex 该用户数据帧对应的索引值
 * \param FrameTime 该用户数据帧对应的时间
 * \return
 */
typedef int (*ANTS_SEEK_USERINFOFILTER_CALLBACK)(HANDLE hDataPopper, int iChannel, int UserIndex, time_t FrameTime);

/*!
 * \brief 数据备份回调函数
 * \param hDataBackup 数据备份器句柄
 * \param BackupSize 已备份大小(单位：KB,全F表示结束)
 * \param BackupFrameTime 当前绝对时间
 * \param EventID 发生的事件类型，如果是正常的数据备份，EventID为DATABACKUP_EVENT_NONE，其他事件时，帧相关的参数无效
 * \return
 */
typedef int (*ANTS_REPLAY_DATABACKUP_CALLBACK)(HANDLE hDataBackup, unsigned long long BackupSize, time_t BackupFrameTime, unsigned long EventID);

/*!
 * \brief 磁盘分组满回调函数
 * \param hDiskGroup 磁盘分组句柄
 * \return
 */
typedef int (*ANTS_DATAINPUT_DISKGROUPFULL_CALLBACK)(HANDLE hDiskGroup);



/*!
 * \brief
 * 用户数据查询判断数据是否符合条件的回调.
 *
 * \param hKeyPkgQuery
 * 信息查询器句柄.
 *
 * \param iChannel
 * 用户数据的通道号.
 *
 * \param FrameTime
 * 用户数据所在的时间.
 *
 * \param iUserIndex
 * 用户数据的索引值.
 *
 * \return
 * 满足条件返回0, 不满足返回非0的错误码.
 */
typedef int (*ANTS_FIND_KEYPKGSEEK_CALLBACK)(HANDLE hKeyPkgQuery,int iChannel,char *pcGroupName, char *pcPartName,int iLargeBlock, int iBlock, int iUserIndex, time_t FrameTime);



/*!
 * \brief
 * 存储系统的初始化.
 * 
 * \param void
 * 
 * \return
 * 成功返回0，失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_Init(const char *sDeviceSerial);



/*!
 * \brief
 * 存储系统重要错误信息回调函数注册.
 * 
 * \param cbSysErrCallback
 * 回调用函数.
 * 
 * \return
 * 成功返回0，失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_SysErrorCallbackRegister (ANTS_PKG_SYSERR_CALLBACK cbSysErrCallback);



/*!
 * \brief
 * 获取硬盘个数.
 *
 * \return
 * 获取含有的sata硬盘个数.
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskCount();



/*!
 * \brief
 * 获取磁盘分组的个数.
 *
 * \return
 * 返回磁盘分组的个数.
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskGroupCount();



/*!
 * \brief
 * 获取磁盘信息, 存入ptDiskInfo.
 *
 * \param iDiskIndex
 * 磁盘编号(取值范围在0——(硬盘个数-1)).
 *
 * \param ptDiskInfo
 * [out]磁盘信息结构体指针.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskInfo(int iDiskIndex, T_DISKINFO *ptDiskInfo);



/*
 * \brief
 * 对指定的硬盘用指定的分区信息进行分区.
 *
 * \param iDiskIndex
 * 硬盘编号(取值范围在0——(硬盘个数-1)).
 *
 * \param ptPartParamInfo
 * 硬盘分区参数信息结构体指针.
 *
 * \return
 * 成功返回0，失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_DiskFdisk(int iDiskIndex, T_PARTPARAMINFO *ptPartParamInfo);



/*!
 * \brief
 * 创建磁盘分组.
 *
 * \param pcDiskGroupName
 * 磁盘分组名
 *
 * \param ucCycleMode
 * 磁盘轮转的方式(停止或覆盖).
 *
 * \return
 * 磁盘分组句柄结构体信息指针.
 */
PSS_API HANDLE API_CALL ANTS_PkgStream_DiskGroupCreate(char *pcDiskGroupName, int iCycleMode, PKGSTREAMGROUPTYPE GroupType);



/*!
 * \brief
 * 向指定硬盘分组中添加新硬盘分区.
 *
 * \param ptDiskGroup
 * 磁盘组句柄结构体指针.
 *
 * \param pcDevPart
 * 分区节点名.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_DiskGroupPartAdd(HANDLE hDiskGroup, char *pcDevPart);



/*!
 * \brief
 * 删除指定分组上的指定分区.
 *
 * \param ptDiskGroup
 * 分组句柄.
 *
 * \param iGroupValidPartIndex
 * 分区在分组中的有效编号
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_DiskGroupPartDel(HANDLE hDiskGroup, int iGroupValidPartIndex);



/*!
 * \brief
 * 获取指定的磁盘分组信息.
 *
 * \param iGroupIndex
 * 磁盘分组编号(从0开始).
 *
 * \param ptDiskGroupInfo
 * 磁盘分组信息.
 *
 * \return
 * 成功返回磁盘组句柄, 失败返回NULL.
 */
PSS_API HANDLE API_CALL ANTS_PkgStream_GetDiskGroupInfo(int iGroupIndex, PSS_DISKGROUPINFO *ptDiskGroupInfo);



/*!
 * \brief
 * 删除指定的硬盘分组.
 *
 * \param ptDiskGroup
 * 硬盘分组句柄.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_DiskGroupDestroy(HANDLE hDiskGroup);



/*!
 * \brief
 * 设置指定磁盘分组的轮转属性
 *
 * \param ptDiskGroup
 * 磁盘分组句柄
 *
 * \param iCycleMode
 * 轮转属性值
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_SetDiskGroupCycleMode(HANDLE hDiskGroup, int iCycleMode);



/*!
 * \brief
 * 打开指定磁盘分组类型的可查读属性.
 *
 * \param GroupType
 * 磁盘分组类型.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_SetDiskGroupReadable(PKGSTREAMGROUPTYPE GroupType);



/*!
 * \brief
 * 设置磁盘分组的满回调
 *
 * \param cbDiskGroupFullCallback
 * 回调函数句柄
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_SetDiskGroupFullCallback(ANTS_DATAINPUT_DISKGROUPFULL_CALLBACK cbDiskGroupFullCallback);



/*!
 * \brief
 * 对指定磁盘分区格式化.
 *
 * \param iDiskIndex
 * 磁盘编号.
 *
 * \param iPartNo
 * 磁盘分区在磁盘中的编号.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_DiskPartFormat(int iDiskIndex, int iPartNo);



/*!
 * \brief
 * 获取最新数据位置.
 *
 * \param ptDiskGroup
 * 磁盘分区句柄结构体指针.
 *
 * \parm ptDataPos
 * 存放分组最新数据位置的结构体指针.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_GetMostNewDataPos(HANDLE hDiskGroup, T_MOSTNEWDATAPOS *ptDataPos);



/*!
 * \brief
 * 获取分组上的坏块信息.
 *
 * \param hDiskGroup
 * 磁盘组句柄.
 *
 * \param ptBadBlockInfo
 * [out]存放坏块信息的结构体指针.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_GetBadBlockInfo(HANDLE hDiskGroup, T_BADBLOCKINFO **ptBadBlockInfo);



/*!
 * \brief
 * 释放磁盘分组坏块信息表
 *
 * \param ptBadBlockInfo
 * 存放坏块信息的结构体指针.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_ReleaseBlockInfo(T_BADBLOCKINFO *ptBadBlockInfo);



/*!
 * \brief 将数据写入录像通道中
 * \param iChannel 录像通道号
 * \param wFrameType 数据帧类型
 * \param bIndexable 该帧数据是否建立关键帧索引（通常AV数据的I帧和用户信息数据需要建立索引）
 * \param dwUserIndex 用户信息索引值，当数据帧类型为用户数据时，需要指定该帧用户数据的索引值
 * \param pFirstBuf 写入的第一块数据缓存指针
 * \param iFirstLength 写入的第一块数据长度
 * \param pSecondBuf 写入的第二块数据缓存指针
 * \param iSecondLength 写入的第二块数据长度
 * \param tFrameTime 写入帧的绝对时间簇，精确度为秒
 * \param tFrameTimeInUs 写入帧的绝对时间簇，秒内的精度(微秒)
 * \return 成功返回0，失败返回小于0的错误码
 */
PSS_API int API_CALL ANTS_PkgStream_DataInput(int iChannel, unsigned short wFrameType, BOOL bIndexable, DWORD dwUserIndex, void *pFirstBuf, int iFirstLength, void *pSecondBuf, int iSecondLength, time_t tFrameTime, unsigned long tFrameTimeInUs);



/*!
 * \brief 将指定录像通道的指定录像类型绑定到指定硬盘分组
 * \param iChannel 录像通道号
 * \param iRecType 录像类型
 * \param hDiskGroup 硬盘分组句柄
 * \return 成功返回0，失败返回小于0的错误码
 */
PSS_API int API_CALL ANTS_PkgStream_DiskGroupBind(int iChannel, int iRecType, HANDLE hDiskGroup);



/*!
 * \brief 将指定录像通道的指定录像类型取消绑定到指定硬盘分组
 * \param iChannel 录像通道号
 * \param iRecType 录像类型
 * \param hDiskGroup 硬盘分组句柄
 * \return 成功返回0，失败返回小于0的错误码
 */
PSS_API int API_CALL ANTS_PkgStream_DiskGroupUnBind (int iChannel, int iRecType, HANDLE hDiskGroup);



/*!
 * \brief 启动指定录像通道的指定录像类型
 * \param iChannel 录像通道号
 * \param iRecType 录像类型
 * \return 成功返回0，失败返回小于0的错误码
 */
PSS_API int API_CALL ANTS_PkgStream_RecStart(int iChannel, int iRecType);



/*!
 * \brief 停止指定录像通道的录像
 * \param iChannel 录像通道号
 * \return 成功返回0，失败返回小于0的错误码
 */
PSS_API int API_CALL ANTS_PkgStream_RecStop(int iChannel);



/*!
 * \brief 获得指定通道在指定时间段内指定录像类型的录像片断
 * \param GroupType 硬盘分组类型
 * \param iChannel 录像通道号
 * \param tBeginTime 起始时间
 * \param tEndTime 结束时间
 * \param iRecType 录像类型
 * \param pRecSegHead 存放录像片段的缓冲区头指针
 * \param pRecSegCount 保存返回获取片断的个数
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_GetRecSegList(PKGSTREAMGROUPTYPE GroupType, int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG **pRecSegHead, int *pRecSegCount);



/*!
 * \brief 释放录像片断信息列表所占内存
 * \param pRecSegHead 存放录像片段的缓冲区头指针
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_ReleaseRecSegList(PSS_REGSEG *pRecSegHead);



/*!
 * \brief 创建录像片段报告器
 * \param GroupType 硬盘分组类型
 * \param cbGetIndexCallback 回调函数，返回0时，分段报告器中止查询
 * \param pChContext 上下文
 * \return 成功返回录像片段报告器的句柄，失败返回NULL。
 */
PSS_API HANDLE API_CALL ANTS_PkgStream_RecSegReporterCreate(PKGSTREAMGROUPTYPE GroupType, ANTS_REC_SEGMENT_CALLBACK cbGetIndexCallback, void *pChContext);



/*!
 * \brief 释放录像片段报告器
 * \param hIndexReporter 录像片段报告器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_RecSegReporterRelease(HANDLE hIndexReporter);



/*!
 * \brief 添加获取录像片段的通道
 * \param hIndexReporter 录像片段报告器的句柄
 * \param iChannel 通道号
 * \param bDynReport 设定是否需要动态报告索引变化,默认不动态报告索引变化(用来动态更新录像片段信息)。
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_RecSegChanAdd(HANDLE hIndexReporter, int iChannel, int bDynReport);



/*!
 * \brief 移除获取录像片段的通道
 * \param hIndexReporter 录像片段报告器的句柄
 * \param iChannel 通道号
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_RecSegChanDel(HANDLE hIndexReporter, int iChannel);



/*!
 * \brief 获得当前设定时段的所有录像时间片段，通过设定的ANTS_REC_SEGMENT_CALLBACK回调函数返回结果
 * \param hIndexReporter 录像片段报告器的句柄
 * \param tBeginTime 获取录像片段的开始时间
 * \param tEndTime 获取录像片段的结束时间
 * \return 成功返回0，失败返回小于0的错误码。
 * \说明：
         若设置了动态报告，则在ANTS_PkgStream_RecSegGet设定的BeginTime、EndTime之间有索引被覆盖或有新的索引诞生将通过ANTS_REC_SEGMENT_CALLBACK回调告诉上层程序。
         设定获取索引的时间区间，不用关闭hIndexReporter即可再次设定新的区间。ANTS_REC_SEGMENT_CALLBACK调用的线程环境：
         1、调用ANTS_PkgStream_RecSegGet后回调函数使用ANTS_PkgStream_RecSegGet的线程环境搜索片段，这意味着直到所有请求的索引片段回调完毕后ANTS_PkgStream_RecSegGet才返回
         2、自动报告时调用的回调函数线程环境为内部工作线程
 */
PSS_API int API_CALL ANTS_PkgStream_RecSegGet(HANDLE hIndexReporter, time_t tBeginTime, time_t tEndTime);



/*!
 * \brief 创建数据弹出器
 * \param GroupType 硬盘分组类型
 * \param nRefChan 参考通道(可以不设置，值为0xFFFFFFFF表示不设置)
 * \param tStartSeekTime 初始弹出数据的时间
 * \param tEndSeekTime 弹出数据的结束时间。当此时间为0时，数据进行动态弹出。如果结束时间是一个未来时间，则到达当前时间时，通过回调接口告之该事件。
 * \param cbReplayCallback 设定数据弹出的回调函数接口，弹出的数据按弹出器句柄、通道、帧类型、时间簇、事件类型
 *       （跳到新的时间片/查询结束/当前数据查询完毕等）等参数形式送回放线程。此回调与DSP上送编码数据接口相当
 * \param bDynPop 是否动态定位弹出，即播放完毕后，是否主动重新定位继续弹出。
 *        当用于回放时，设置为TRUE，用于备份时，设置为FALSE
 * \param pContext 上下文，上层软件可以传入此参数，在弹出回调中会给出该参数
 * \return 成功返回创建的数据弹出器的句柄,失败返回NULL。
 */
PSS_API HANDLE API_CALL ANTS_PkgStream_DataPopCreate(PKGSTREAMGROUPTYPE GroupType, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime, ANTS_REPLAY_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext);



/*!
 * \brief 添加弹出数据的通道
 * \param hDataPopper 数据弹出器的句柄
 * \param iChannel 通道号
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopChanAdd(HANDLE hDataPopper, int iChannel);



/*!
 * \brief 移除弹出数据的通道
 * \param hDataPopper 数据弹出器的句柄
 * \param iChannel 通道号
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopChanDel(HANDLE hDataPopper, int iChannel);



/*!
 * \brief 添加弹出数据的录像类型
 * \param hDataPopper 数据弹出器的句柄
 * \param iRecType 录像类型
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopRecTypeAdd(HANDLE hDataPopper, int iRecType);



/*!
 * \brief 移除弹出数据的录像类型
 * \param hDataPopper 数据弹出器的句柄
 * \param iRecType 录像类型
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopRecTypeDel(HANDLE hDataPopper, int iRecType);



/*!
 * \brief 数据弹出器定位(设定了参考通道，按参考通道定位，未设参考通道，定位为时序上第一个满足条件的通道)
 * \param hDataPopper 数据弹出器的句柄
 * \param SeekTime 设置的弹出数据的时间
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopTimeSeek(HANDLE hDataPopper, time_t SeekTime);



/*!
 * \brief 按用户信息索引值定位
 * \param hDataPopper 数据弹出器的句柄
 * \param cbSeekUserFilter 根据用户索引信息判别定位条件的回调函数，返回BOOL值，表示是否满足定位条件。
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopUserinfoSeek(HANDLE hDataPopper, ANTS_SEEK_USERINFOFILTER_CALLBACK cbSeekUserFilter);



/*!
 * \brief 开始数据弹出
 * \param hDataPopper 数据弹出器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopStart(HANDLE hDataPopper);



/*!
 * \brief 停止数据弹出
 * \param hDataPopper 数据弹出器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopStop(HANDLE hDataPopper);



/*!
 * \brief 释放数据弹出器
 * \param hDataPopper 数据弹出器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopRelease(HANDLE hDataPopper);



/*!
 * \brief
 * 创建用户关键帧信息查询器.
 *
 * \param iRefChan
 * 参考通道号.
 *
 * \param bDynPop
 * 判断是否动态弹出.
 *
 * \param tFirstSeekTime
 * 初始弹出数据的时间.
 *
 * \param cbKeyPkgFilter
 * 用户关键数据查询过滤回调.
 *
 * \return
 * 成功返回信息查询器句柄, 失败返回NULL.
 */
PSS_API HANDLE API_CALL ANTS_PkgStream_KeyPkgQueryCreate(int iRefChan, time_t tFirstSeekTime, ANTS_FIND_KEYPKGSEEK_CALLBACK cbKeyPkgFilter);



/*!
 * \brief
 * 定位用户关键帧数据包.
 *
 * \param hKeyPkgQuery
 * 信息查询器句柄.
 *
 * \param pFrameType
 * 保存关键数据包类型的指针.
 *
 * \param pLength
 * 保存关键数据包大小的指针.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_KeyPkgSeek(HANDLE hKeyPkgQuery, unsigned short *pFrameType, unsigned int *pLength);



/*!
 * \brief
 * 读取用户关键数据包.
 *
 * \param pFrameType
 * 保存关键数据包类型的指针.
 *
 * \param pLength
 * 保存关键数据包大小的指针.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_KeyPkgGet(HANDLE hKeyPkgQuery, void *pBuf, unsigned short *pFrameType, unsigned int *pLength);



/*!
 * \brief
 * 释放关键帧信息查询器.
 *
 * \param hKeyPkgQuery
 * 关键帧信息查询器句柄.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_KeyPkgQueryRelease(HANDLE hKeyPkgQuery);



/*!
 * \brief
 * 删除磁盘分组上指定通道在指定时间段内的数据.
 *
 * \param GroupType
 * 磁盘分组类型.
 *
 * \param nChannel
 * 指定通道号.
 *
 * \param iRecType
 * 录像类型.
 *
 * \param tStartTime
 * 时间段的起始时间.
 *
 * \param tEndTime
 * 时间段的结束时间.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_RecDataRemove(HANDLE hDiskGroup, int iChannel, int iRecType, time_t tStartTime, time_t tEndTime);



/*!
 * \brief
 * 删除指定磁盘分组上在指定时间段内的录像数据.
 *
 * \param hDiskGroup
 * 磁盘分组句柄.
 *
 * \param tStartTime
 * 时间段的起始时间.
 *
 * \param tEndTime
 * 时间段的结束时间.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_RecDataRemoveEx(HANDLE hDiskGroup, time_t tStartTime, time_t tEndTime);


/*!
 * \brief
 * 热插入新硬盘.
 *
 * \param sDevName.
 * 硬盘结点名称.
 *
 * \return
 * 成功返回0, 失败返回错误码.
 */
PSS_API int API_CALL ANTS_PkgStream_HotplugDiskAdd(const char *sDevName);



/*!
 * \brief
 * 热拔硬盘.
 *
 * \param sDevName.
 * 硬盘结点名称.
 *
 * \return
 * 成功返回0, 失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_HotplugDiskRemove(const char *sDevName);


/*!
 * \brief
 * 分区后更新分组信息.
 *
 * \param sDevName.
 * 硬盘结点名称.
 *
 * \return
 * 无.
 */
PSS_API void API_CALL ANTS_PkgStream_UpdateGroupInfo(char *pcDevName);



/*!
 * \brief 数据弹出时是否只弹出关键帧
 * \param hDataPopper 数据弹出器的句柄
 * \param bKeyFrame 是否只弹出关键帧，如果为TRUE则只弹出关键帧，如果为FALSE则弹出所有帧。
 * \param PopKeyInterval 当只弹出关键帧时，该参数有效，
 *  表示通道弹出每隔多少关键帧弹出一个，此值限制在0-3，0表示全部弹出
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopSetKeyFrame(HANDLE hDataPopper, BOOL bKeyFrame, unsigned long PopKeyInterval);



/*!
 * \brief 设置数据弹出方向
 * \param hDataPopper 数据弹出器的句柄
 * \param bKeyFrame 数据弹出方向，如果为TRUE则向后弹出，如果为FALSE则向前弹出
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopSetDirect(HANDLE hDataPopper, BOOL bSequential);



/*!
 * \brief 获取一帧关键帧
 * \param hDataPopper 数据弹出器的句柄
 * \param iChannel 通道号
 * \param tSeekTime 指定时间簇
 * \param precision 时间簇误差
 * \param pbuf 保存该关键帧的缓冲区指针
 * \param bufsize 输入时为缓冲区大小，输出时为关键帧大小
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopGetOneKeyFrame(HANDLE hDataPopper, int iChannel, time_t tSeekTime, unsigned int precision, unsigned char *pbuf, unsigned int *bufsize);



/*!
 * \brief 创建数据备份器
 * \param GroupType 硬盘分组类型
 * \param hBackupDiskGroup 备份分组句柄
 * \param tStartSeekTime 备份数据的起始时间
 * \param tEndSeekTime 备份数据的结束时间
 * \param cbBackupCallback 回调接口，参数为已备份大小(单位：KB,全F表示结束)，当前绝对时间
 * \return 成功返回创建的数据备份器的句柄,失败返回NULL。
 */
PSS_API HANDLE API_CALL ANTS_PkgStream_DataBackupCreate(PKGSTREAMGROUPTYPE GroupType, HANDLE hBackupDiskGroup, time_t tStartSeekTime, time_t tEndSeekTime, ANTS_REPLAY_DATABACKUP_CALLBACK cbBackupCallback);



/*!
 * \brief 添加备份的录像类型
 * \param hDataBackup 数据备份器的句柄
 * \param nRecType 录像类型
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataBackupAddRecType(HANDLE hDataBackup, int nRecType);



/*!
 * \brief 添加备份数据的通道
 * \param hDataBackup 数据备份器的句柄
 * \param nChannel 通道号
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataBackupChanAdd(HANDLE hDataBackup, int nChannel);



/*!
 * \brief 获取备份数据的总容量(单位：KB)
 * \param hDataBackup 数据备份器的句柄
 * \param pSize 总容量
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_GetDataBackupSize(HANDLE hDataBackup, unsigned long long *pSize);



/*!
 * \brief 开始数据备份
 * \param hDataBackup 数据备份器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataBackupStart(HANDLE hDataBackup);



/*!
 * \brief 停止数据备份
 * \param hDataBackup 数据备份器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataBackupStop(HANDLE hDataBackup);



/*!
 * \brief 释放数据备份器
 * \param hDataBackup 数据备份器的句柄
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataBackupRelease(HANDLE hDataBackup);



/*!
 * \brief 获取指定分区所含有的录像数据的开始时间和结束时间
 * \param hDiskGroup 硬盘分组句柄
 * \param pcDevPart 分区节点名
 * \param tBeginTime 存放开始时间
 * \param tEndTime 存放结束时间
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskPartDataInfo(HANDLE hDiskGroup, char *pcDevPart, time_t *tBeginTime, time_t *tEndTime);



/*!
 * \brief 获取指定分区所含有的录像数据的开始时间和结束时间
 * \param pcDevPart 分区节点号
 * \param tBeginTime 存放开始时间
 * \param tEndTime 存放结束时间
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskPartDataInfoEx(char *pcDevPart, time_t *tBeginTime, time_t *tEndTime);



/*!
 * \brief 存储系统硬盘访问异常错误回调函数注册.
 * \param cbDiskErrCallback 回调用函数.
 * \return 成功返回0，失败返回-1.
 */
PSS_API int API_CALL ANTS_PkgStream_DiskErrorCallbackRegister(ANTS_PKG_DISKERR_CALLBACK cbDiskErrCallback);



/*!
 * \brief 获取指定正在写入的分组的的位置信息（即最新数据位置信息）
 * \param hDiskGroup 磁盘分组句柄
 * \param ptDataPos 存放分组最新数据位置的结构体指针
 * \return 成功返回0，当前分组在写，失败返回-1. 如果当前分组没有写，则返回-1
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskGroupWrittingDiskInfo(HANDLE hDiskGroup, T_MOSTNEWDATAPOS *ptDataPos);



/*!
 * \brief 获取指定分区所含有的录像数据的大小(MB为单位)
 * \param pcDevPart 分区节点名
 * \param DataSize 存放录像数据的大小
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskPartDataSize(char *pcDevPart, unsigned long *DataSize);



/*!
 * \brief 获取指定分组类型是否有指定时间段的录像数据
 * \param GroupType 指定的分组类型
 * \param tBeginTime 指定时间段的开始时间
 * \param tEndTime 指定时间段的结束时间
 * \return 有录像数据返回0，其他返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_JudgeIFHaveDataByTime(PKGSTREAMGROUPTYPE GroupType, time_t tBeginTime, time_t tEndTime);



/*!
 * \brief 获取指定分组类型是否有指定天数(一般用于一个月)的录像数据
 * \param GroupType 指定的分组类型
 * \param tBeginTime 指定时间段的开始时间
 * \param numOfdays 从起始天开始的天数(24小时)
 * \param pResult 返回结果，采用位与方式bit0-bit30分别对应31天
 * \return 成功返回0，其他返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_JudgeIFHaveDataByDays(PKGSTREAMGROUPTYPE GroupType, time_t tBeginTime, unsigned int numOfdays, unsigned int *pResult);



/*!
 * \brief 获取弹出器备份数据的总容量(单位：KB)
 * \param hDataPopper 数据弹出器的句柄
 * \param pSize 总容量
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_DataPopGetDataSize(HANDLE hDataPopper, unsigned long long *pSize);



/*!
 * \brief 获取指定盘组的剩余容量(单位：MB)
 * \param hDiskGroup 磁盘分组句柄
 * \param pSize 总容量
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_GetDiskGroupRemainSize(HANDLE hDiskGroup, unsigned long *pSize);




/*!
 * \brief 设置盘组的数据导入过滤条件
 * \param hDiskGroup 
 * \param FilterVal 过滤条件为0表示没有限制，1表示只允许关键帧
 * \return 成功返回0，失败返回小于0的错误码。
 */
PSS_API int API_CALL ANTS_PkgStream_SetDiskGroupInputFilter(HANDLE hDiskGroup, unsigned long FilterVal);





#endif

