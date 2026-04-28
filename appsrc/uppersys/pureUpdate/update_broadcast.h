#ifndef __UPDATE_BROADCAST_H__
#define __UPDATE_BROADCAST_H__

#include "update_version.h"


S32 update_broadCast_Init();
S32 update_broadCast_GetHelloUUID(S8 *pcUUID, S32 lUUIDLen);
void update_broadCast_AuthSayHello(DeviceVersion_S *pstVersion);

#endif
