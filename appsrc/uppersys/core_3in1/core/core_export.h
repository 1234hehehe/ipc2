#ifndef __CORE_EXPORT_H__
#define __CORE_EXPORT_H__

S32 Core_Export_Init(ModuleHandle_T hModuleHandle);
S32 Core_Export_CallFunctions(ModuleHandle_T hModuleHandle,
                              cJSON_Struct *pInParams, cJSON_Struct **pOutParams);

#endif