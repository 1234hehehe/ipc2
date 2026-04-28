#ifndef _FLASHRW_H_
#define _FLASHRW_H_

#include <stdio.h>

#define MAX_MTDBLOCK	6

enum FlashPartition
{
	UbootKernelPartition = 0,
	RfsPartition = 1,
	ConfigPartition = 2,
	LogoPartition = 3,
};

enum FlashWriteStatus
{
	NoWriteNow = 0,
	StartNow,
	EraseNow,
	WriteNow,
	VerifyNow,
};

#ifdef __cplusplus
extern "C" {
#endif

int Ovfs_update_Flash_WriteMtd(int iMtdBlockNum, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize);
int Ovfs_update_Flash_WriteFileMtd(int iMtdBlockNum, FILE *UpFile, int iWriteSize, int iStartPos, int iEraseSize);
int Ovfs_update_Flash_WriteMtd_ByName(char *pMtdName, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize);
int Ovfs_update_Flash_WriteFileMtd_ByName(char *pMtdName, FILE *UpFile, int iWriteSize, int iStartPos, int iEraseSize);

int Ovfs_update_Flash_GetStatus(int *pStatus, int *pRatio);
unsigned int Ovfs_update_Flash_GetEWStatus();

#ifdef __cplusplus
}
#endif

#endif
