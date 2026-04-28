#ifndef __MODULE_URIPROXY_H__
#define __MODULE_URIPROXY_H__

S32 Module_UriProxy_Init(ModuleHandle_T hModuleHandle);
S32 Module_UriProxy_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_UriProxy_Update(ModuleHandle_T hModuleHandle);
#endif
