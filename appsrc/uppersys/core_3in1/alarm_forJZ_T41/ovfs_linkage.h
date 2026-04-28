/*
 * ovfs_linkage.h
 *
 *  Created on: 2017-3-07
 *      Author:
 */
#ifndef OVFS_LINKAGE_H_
#define OVFS_LINKAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

int ovfs_linkFtp(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkReportCentre(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkMail(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkRecord(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkHttp(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkSnap(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkAlarmOut(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkPtz(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkSound(ModuleHandle_T hModuleHandle,OVFS_SOUND_CFG *pSoundCfg,OVFS_ALARM_EVENT *pAlarmEvent,int compulsiveState);
int ovfs_linkLightAlarm(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_linkRedBlueLight(ModuleHandle_T hModuleHandle,OVFS_LINKAGE_CFG *pLinkageCfg,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_clearAlarmLink(ModuleHandle_T hModuleHandle,S32 iAlarmType,S32 iAlarmSrc,OVFS_LINKAGE_CFG *pLinkageCfg,S32 iLinkType,S32 bEnable, S32 index);
int ovfs_linkOutput(ModuleHandle_T hModuleHandle,OVFS_ALARM_EVENT *pAlarmEvent);
int ovfs_initLink(ModuleHandle_T hModuleHandle);

int Action_SoundAlarm(ModuleHandle_T hModuleHandle,int index,char* patch,int times,int isStart);
int Action_LightAlarm(ModuleHandle_T hModuleHandle,int enable);
int Action_LightAlarmv2(ModuleHandle_T hModuleHandle,int enable);
int Action_RedBlueLight(ModuleHandle_T hModuleHandle,int enable);
int Action_SnapAlarm(ModuleHandle_T hModuleHandle,int streamIdx);

#ifdef __cplusplus
}
#endif

#endif /* OVFS_LINKAGE_H_ */
