/*
 * ants_web.h
 *
 *  Created on: 2016-08-01
 *      Author: eric
 */
#ifndef OVFS_ALARM_H_
#define OVFS_ALARM_H_

#ifdef __cplusplus
extern "C"
{
#endif

const S8 g_alarmName[][32] = {ALARM_STR_AI,ALARM_STR_MOTION,ALARM_STR_VHIDE,\
                        ALARM_STR_REGIONALINVASION,ALARM_STR_DETWIRE,ALARM_STR_PERSONSTAYING,\
                        ALARM_STR_DETABSENT,ALARM_STR_PARKINGVIOLATION,ALARM_STR_RETROGRADE,\
                        ALARM_STR_SENSORALARM,ALARM_STR_LOWBATTERYALARM,\
						ALARM_STR_NETBREAK,ALARM_STR_IPCONFLICT,ALARM_STR_ILLACCESS};

const S8 g_alarmIndex[] = {ALARM_TYPE_ALARMIN,ALARM_TYPE_MOTION,ALARM_TYPE_VHIDE,\
                        ALARM_TYPE_REGIONAL_INVASION,ALARM_TYPE_DETECT_WIRE,ALARM_TYPE_PERSON_STAYING,\
                        ALARM_TYPE_DETECT_ABSENT,ALARM_TYPE_PARKING_VIOLATION,ALARM_TYPE_RETROGRADE,\
                        ALARM_TYPE_SENSOR_ALARM,ALARM_TYPE_LOW_BATTERY_ALARM,\
						ALARM_TYPE_CABLE_BREAK,ALARM_TYPE_IP_CONFLIT,ALARM_TYPE_ILLEG_ACCESS};


const S8 g_alarmIndex_smart[] = {
                        ALARM_TYPE_REGIONAL_INVASION,ALARM_TYPE_DETECT_WIRE,ALARM_TYPE_PERSON_STAYING,\
                        ALARM_TYPE_DETECT_ABSENT,ALARM_TYPE_PARKING_VIOLATION,ALARM_TYPE_RETROGRADE,\
						};

int findAlarmType(char *AlarmName);
int getChanIndexByDevChan(int iType,int iDev,int iChan);
int checkSchedTime(OVFS_ALARM_EVENT  *pAlarmEvent,Common_Time_T t_happent);
int ovfs_arm_operate(int bEnable);
int ovfs_report_alarm(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_recv_alarm(void *pInData,void *pAddData,void *pCondition,void **pOutData);
int get_mount_path(ModuleHandle_T hModuleHandle,OVFS_ABILITY *ppAbility);
OVFS_ABILITY * ovfs_get_ability();
int ovfs_init_ability(ModuleHandle_T hModuleHandle);
int ovfs_init_subscribe(ModuleHandle_T hModuleHandle);
int ovfs_get_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
int ovfs_put_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
int ovfs_post_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);
int ovfs_delete_alarm_res(char *pUri,cJSON_Struct *pAddData,cJSON_Struct **outJson);

#ifdef __cplusplus
}
#endif

#endif /* OVFS_ALARM_H_ */
