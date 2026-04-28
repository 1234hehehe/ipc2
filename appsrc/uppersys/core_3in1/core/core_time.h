#ifndef __CORE_TIME_H__
#define __CORE_TIME_H__
S32 Core_Time_Init(ModuleHandle_T hModuleHandle, cJSON_Struct *pConfig);
S32 Core_Time_CallFunctions(ModuleHandle_T hModuleHandle,
                            cJSON_Struct *pInParams, cJSON_Struct **pOutParams);
S32 Core_Time_LoadConfig(ModuleHandle_T hModuleHandle,
                         cJSON_Struct **pOutParam);
S32 Core_Time_SaveConfig(ModuleHandle_T hModuleHandle, cJSON_Struct *pOutParam);
#endif