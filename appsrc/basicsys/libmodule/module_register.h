#ifndef __MODULE_REGISTER_H__
#define __MODULE_REGISTER_H__
S32 Module_Register_Init(ModuleHandle_T hModuleHandle);
S32 Module_Register_Load(ModuleHandle_T hModuleHandle);
S32 Module_Register_Save(ModuleHandle_T hModuleHandle);
S32 Module_Register_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_Register_Report_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_Register_Heart_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_Register_Offline_Check(ModuleHandle_T hModuleHandle);

#endif

