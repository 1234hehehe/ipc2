/*
 * ants_web.h
 *
 *  Created on: 2016-8-1
 *      Author: eric
 */
#ifndef OVFS_ALARM_CFG_H_
#define OVFS_ALARM_CFG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include"ovfs_alarm_common.h"

OVFS_COMMON_CFG * getCommCfgByAlarmName(POVFS_ALARM_CFG pAlarmCfg,char *pAlarmName);
OVFS_COMMON_CFG * getCfgByAlarmName(POVFS_ALARM_CFG pAlarmCfg,int iDev,int iChan,char *pAlarmName);
OVFS_COMMON_TRI_CFG *getTriCfgByAlarmName(POVFS_ALARM_CFG pAlarmCfg,int iDev,int iChan,char *pAlarmName);
int addSchduleTime(cJSON_Struct **pInOut,S32 index,OVFS_SCHEDTIME_TIME_T* schedtime);
int fillSchduleTime(cJSON_Struct *pInData,OVFS_SCHEDTIME_TIME_T *pSchedtime);
int ovfs_get_arming_state();
void ovfs_set_arming_state(int bEnable);
OVFS_ALARM_CFG *ovfs_get_alarm_cfg();
int ovfs_set_alarm_cfg(OVFS_ALARM_CFG *pAlarmCfg);
int ovfs_load_alarm_cfg(ModuleHandle_T hModuleHandle);
int ovfs_save_alarm_cfg(ModuleHandle_T hModuleHandle);
int ovfs_load_alarm_custom(char *filePath);

int SchduleTimeStructToJson(cJSON_Struct* pInOut,OVFS_SCHEDTIME_TIME_T* schedtime,int isStructToJson);
int CheckIsInSchduleTime(long timeSec,OVFS_SCHEDTIME_TIME_T* schedtime);

int LightAlarmCfgStruct2Json(OVFS_LIGHT_ALARM_CFG* cfg,cJSON_Struct* jsonObj,int isStructToJson);
int RedblueLightStruct2Json(OVFS_LIGHT_ALARM_CFG* cfg,cJSON_Struct* jsonObj,int isStructToJson);

int ovfs_make_common_json(POVFS_COMMON_CFG pCommonCfg,char *pAlarmName,cJSON_Struct **outJson, int needNameNode);
int ovfs_make_common_struct(cJSON_Struct * dataJson,POVFS_COMMON_CFG pCommonCfg,char *pAlarmName, int needNameNode);

#ifdef __cplusplus
}
#endif

#endif /* OVFS_ALARM_CFG_H_ */
