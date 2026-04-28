#ifndef __MODULE_SUBSCRIBE_H__
#define __MODULE_SUBSCRIBE_H__
S32 Module_Subscribe_Init(ModuleHandle_T hModuleHandle);
S32 Module_Subscribe_UnInit(ModuleHandle_T hModuleHandle);
S32 Module_Subscribe_Load(ModuleHandle_T hModuleHandle);
S32 Module_Subscribe_Save(ModuleHandle_T hModuleHandle);
S32 Module_Subscribe_CheckSubmit(ModuleHandle_T hModuleHandle);
S32 Module_Subscribe_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_Subscribe_do_Offline(ModuleHandle_T hModuleHandle);
#endif
