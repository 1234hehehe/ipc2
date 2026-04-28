#ifndef __UPDATE_COMMON_H__
#define __UPDATE_COMMON_H__

#include "libcommon_api.h"

S8 *update_common_GetUUID();
S8 *update_common_GetSerialNumber();
S8 *update_common_GetHardwareVersion();
S32 udpate_common_UmountPartition();
S32 udpate_common_Flash_WriteMtd(int iMtdBlockNum, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize);

#endif
