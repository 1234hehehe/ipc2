#ifndef __CORE_MAINTENANCE_H__
#define __CORE_MAINTENANCE_H__
S32 Core_Maintenance_Init(ModuleHandle_T hModuleHandle, cJSON_Struct *pConfig);
S32 Core_Maintenance_CallFunctions(ModuleHandle_T hModuleHandle,
                                   cJSON_Struct *pInParams, cJSON_Struct **pOutParams);
S32 Core_Maintenance_LoadConfig(ModuleHandle_T hModuleHandle,
                                cJSON_Struct **pOutParam);
S32 Core_Maintenance_SaveConfig(ModuleHandle_T hModuleHandle,
                                cJSON_Struct *pOutParam);
#endif
