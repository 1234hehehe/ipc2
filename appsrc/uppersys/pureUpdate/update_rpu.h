#ifndef __UPDATE_RPU_H__
#define __UPDATE_RPU_H__

#include "update_serverMgr.h"

#define MAX_MERGE_BOARDID_COUNT 16
#define MAX_MERGE_PART_COUNT 16

#define ARPUHEADERMAGIC ('A' | 'R'<<8 | 'P'<<16 | 'U'<<24) //0x41525055  //"A"R"P"U"

typedef struct
{
    unsigned int fileStartKB;  //升级文件打包地址
    unsigned int fileSizeKB;   //升级文件大小(kB)
    unsigned int partStartKB;  //升级文件Flash地址
    unsigned int partSizeKB;   //擦除大小(kB)
    char partChecksum[16];
} RawPartUpgradeFilePart_T;

typedef struct RawPartUpgradeFileHeader_T
{
    unsigned int uMagicNumber;// "ARPU"-ants raw partition update.
    unsigned int headSize; // header长度,包括magic number在内.
    unsigned char headChecksum[16]; // 计算校验值
    union
    {
        struct
        {
            char versionMajor;
            char versionMinor;
            char versionRev;
            char versionBuild;
        } verPart;

        int verValue;
    };
    int boardid[MAX_MERGE_BOARDID_COUNT];

    char res2[3];
    char partCount;
    RawPartUpgradeFilePart_T partInfo[MAX_MERGE_PART_COUNT];

    char res3[416];
} RawPartUpgradeFileHeader_T;

typedef struct 
{
    unsigned int erasingSize; // 需要擦除的大小
    unsigned int erasedSize; // 已经擦除的大小
    unsigned int writingSize; // 需要写入的大小
    unsigned int writtenSize; // 已经写入的大小
    unsigned int curPosition; // 当前操作Flash地址
    char status[16]; // 当前操作状态
} ProgramStatus_T;

int updateRpu_Upgrade(UpdateSvrMgr* pstUpdateSvrMgr, S8 *pcFileName);
int updateRpu_GetUpgradeStatus(int *plStatus, int *plUpgradeProgress);

#endif
