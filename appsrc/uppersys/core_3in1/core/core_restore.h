#ifndef __CORE_RESTORE_H__
#define __CORE_RESTORE_H__
#include "libcommon_api.h"
#include "libmodule_api.h"
S32 Core_Restore_Init(ModuleHandle_T hModuleHandle);
S32 Core_Restore_CallFunctions(ModuleHandle_T hModuleHandle,
                               cJSON_Struct *pInParams, cJSON_Struct **pOutParams);
S32 Core_Restore_LoadCustom(ModuleHandle_T hModuleHandle,
                            cJSON_Struct **pOutParam);
#endif

