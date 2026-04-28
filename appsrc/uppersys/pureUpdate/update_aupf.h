#ifndef __UPDATE_AUPF_H__
#define __UPDATE_AUPF_H__

#include "update_serverMgr.h"

#define UPDATE_FILE_HEADER_MAGIC 0x41555046  //"AUPF"
#define UPDATE_PACKAGE_HEADER_MAGIC 0x5350502A // "SPP*"
#pragma pack(push,1)

typedef enum _tagUpdateType
{
	UpdateType_Nothing = 0,
	UpdateType_Uboot, 
	UpdateType_Kernel,  
	UpdateType_Rfs,  
	UpdateType_App,  
	UpdateType_Config = 5,  
	UpdateType_Logo,  
	UpdateType_ChLogo,  
	UpdateType_File,
	UpdateType_Tar,
	UpdateType_FileEx = 10, // 文件操作,删除/
	UpdateType_PartName = 20, //  通过分区名升级方式
	UpdateType_PartIndex , //  通过分区索引升级方式,0- 无效分区,1-第一分区,2-第二分区
	UpdateType_Flash, // 升级整个flash
	UpdateType_Butt,
}UpdateType_T;

typedef enum _tagUpdateCheckType
{
	UpdateCheckType_MD5 = 0,
	UpdateCheckType_BUTT
}UpdateCheckType_T;

typedef struct _tagUpdateFileHeader
{
	unsigned int uMagicNumber;// "AUPF" -ants update package file
	unsigned int uPackageTypeMark;// package type bits mark
	unsigned int uVersion;// 0- ignore
	unsigned int uRequireVersion;// 0- ignore
	unsigned int uRequireClearConfig;
	unsigned int uSupportBoardNum;// 0- do nothing
	unsigned int uSupportBoardTypesTable_Offset;// 1-N array,,from file start,include file header:0- do nothing
	unsigned int uPackageNum; // [1-32]
	unsigned int uPackageOffsetsTable_Offset;// 1- 32 array ,from file start,include file header
}UpdateFileHeader_T;

typedef struct _tagUpdatePkgFileExHeader
{
	unsigned int uOperateion;// 0-覆盖;1-删除;2-移动;3-执行
	unsigned int uFileType;// 0-目录，1-普通文件
	unsigned int uFilePathLen;// 指定操作的路径长度
}UpdatePkgFileExHeader_T;

typedef struct _tagUpdatePackageHeader
{
	unsigned int uMagicNumber;// "SPP*" single part package 
	unsigned int uPackageType;
	unsigned int uVersion;// 0- ignore
	unsigned int uRequireVersion;// 0- ignore
	unsigned int uPackageLength;// not include header
	unsigned int uDependPackage;// package type bits mark
	unsigned int uPackageCheckType;// 0- MD5
	unsigned int uPackageCheckValue[4];
	unsigned int uExtPackHeaderLengh; // 4对齐 记录是否根据分区名称进行升级,非零则记录分区名称长度(Header_T后面紧跟分区名字).
	union{
	unsigned int uRes[2];
	unsigned int uPartIndx;
	};
}UpdatePackageHeader_T;

typedef struct _tagAupfUpdateStatusMgr_T{
    BOOL                   m_bFileUp;
    BOOL                   m_bNoFlashWrite;
    LONG                   m_lNoFlashStatus;
    LONG                   m_lNoFlashRatio;

    Common_Lock_T          m_pMutex;

    // 该内存不得释放
    unsigned char         *m_pUpdateByBuffer;
    char                   m_pTarFileName[256];

    int                    m_bClearConfig;

    unsigned int           m_uTotalUpdateSize;     //总的升级数据大小
    unsigned int           m_uCurrUpdatedSize;     //当前正在升级的数据包大小
    unsigned int           m_uUpdatedSize;         //已升级的数据大小(不包括当前正在升级的数据)

    unsigned int          *m_pPackageOffsetsTable;
    unsigned int           m_uPackageOffsetsNum;
    unsigned int           m_dwBoardType;

    unsigned char         *m_pUpdateBuff;

    FILE                  *m_hUpgradeFile;

    //Common_Md5_T           m_md5;
    //unsigned int           m_md5_result[4];
}AupfUpdateStatusMgr_T, *AupfUpdateStatusMgr_T_PTR;
#pragma pack(pop)

//升级AntsUpdatePackageFile
int updateAupf_Upgrade(UpdateSvrMgr* pstUpdateSvrMgr, S8 *pcFileName);
int updateAupf_GetUpgradeStatus(int *plStatus, int *plUpgradeProgress);

#endif
