#ifndef __OVFS_NETWORK_4G_H__
#define __OVFS_NETWORK_4G_H__

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "ovfs_comm_tool.h"
#include "ovfs_network_rest_common.h"


S32 ovfs_network_4g_init();
S32 ovfs_network_4g_Save(cJSON_Struct *pJson);
int RebootSystem();
int set_4g_route_change(int nNetType);

int NetWork_Get_LTECfg_Json(cJSON_Struct* out);
int NetWork_Put_LTECfg_Json(cJSON_Struct* in);
int NetWork_Get_4GCustomer_Json(cJSON_Struct* out);
int Network_Put_4GSpecified(cJSON_Struct* in);
int Network_Put_SpecifiedCard(cJSON_Struct* in);
int NetWork_Bind_Ccid(cJSON_Struct* in);
int NetWork_Unbind_Ccid(cJSON_Struct* in);

#endif

