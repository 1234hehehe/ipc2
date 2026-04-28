#ifndef __UPDATE_ENCRY_H__
#define __UPDATE_ENCRY_H__

#include "libcommon_api.h"

S32 updateEncry_GetLot(U8 *ucpData);
S32 updateEncry_GetUserZoneInf(U8 ucUZId, U8 *ucpData, U8 ucDataCount);

#endif
