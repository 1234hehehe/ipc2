#ifndef __MODULE_STREAM_H__
#define __MODULE_STREAM_H__
S32 Module_StreamQueue_Init(ModuleHandle_T hModuleHandle);
S32 Module_StreamQueue_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_StreamQueue_Require_Filter_Kick(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_StreamQueue_Require_Filter_Control(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_StreamQueue_Require_Filter_Delete(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_StreamQueue_Require_Filter_Post(LibModuleInfo_T *pModuleMgr,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
#endif