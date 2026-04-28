
/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 主要是处理网络模块收到的消息
 *	源 文 件 : ovfs_network_msg.cpp
 *	作    者 : 舒适
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2014/12/12
 *****************************************************************************/

#ifndef _OVFS_NETWORK_APP_H_
#define _OVFS_NETWORK_APP_H_


#ifdef __cplusplus
extern "C"{
#endif

#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"

int NetWork_App_Init();
int NetWork_LoadCfgAndStart();
int NetWork_Cfgm_init();
int NetWork_SaveCfg();

int NetWork_Rest_NetAttr_init(Common_cJSON_T* jsonTree);
int NetWork_Rest_NetAttr_initwifi(Common_cJSON_T* jsonTree);
int NetWork_Rest_NetAttr_init4g(Common_cJSON_T* jsonTree);
int NetWork_Rest_NetApp_init(Common_cJSON_T* jsonTree);
int NetWork_Rest_Functions_init(Common_cJSON_T* jsonTree);
int NetWork_Rest_Status_init(Common_cJSON_T* jsonTree);
int NetWork_Rest_Subscribe_init(Common_cJSON_T* jsonTree);
int NetWork_Rest_Subscribe_init4g(Common_cJSON_T* jsonTree);
int NetWork_Rest_Restore_init(Common_cJSON_T* jsonTree);

#ifdef __cplusplus
}
#endif

#endif	//#ifndef _OVFS_NETWORK_API_H_

