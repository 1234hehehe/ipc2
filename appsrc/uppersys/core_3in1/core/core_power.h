#ifndef __CORE_POWER_H__
#define __CORE_POWER_H__
#include "libcommon_api.h"
#include "libmodule_api.h"
S32 Core_Power_Init(ModuleHandle_T hModuleHandle);
S32 Core_Power_CallFunctions(ModuleHandle_T hModuleHandle,
                             cJSON_Struct *pInParams, cJSON_Struct **pOutParams);

#endif

