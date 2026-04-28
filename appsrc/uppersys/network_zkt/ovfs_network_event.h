#ifndef _OVFS_NETWORK_EVENT_H_
#define _OVFS_NETWORK_EVENT_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"

int NetWork_Event_Init();
int NetWork_Event_Destroy();
int NetWork_Event_Start();
int NetWork_Event_Stop();

#ifdef __cplusplus
}
#endif

#endif	//#ifndef _OVFS_NETWORK_API_H_

