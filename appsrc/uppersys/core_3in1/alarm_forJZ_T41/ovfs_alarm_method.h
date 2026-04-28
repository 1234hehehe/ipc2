/*
 * ants_web.h
 *
 *  Created on: 2016-08-01
 *      Author: eric
 */
#ifndef OVFS_ALARM_METHOD_H_
#define OVFS_ALARM_METHOD_H_

#ifdef __cplusplus
extern "C"
{
#endif

S32 FilterCondithon(cJSON_Struct *pData,S32 iIndex,cJSON_Struct *pCondition);
S8 *FindLast(S8 *pData,S8 c);
S32 MakeResult(const S8 *pFileDes,const S8 *pFuncDes,S32 nLine,S32 Code,const S8 *pDes,cJSON_Struct *pData,cJSON_Struct **pOutData);
cJSON_Struct * ovfs_get_alarm_status(cJSON_Struct *pJCondition);
cJSON_Struct * ovfs_get_rest();
S32 ovfs_get_debug();
int ovfs_get_cfgChange();
void ovfs_set_cfgChange(int bChange);
int ovfs_reset_alarm_bkbd();
int ovfs_init_alarm_res(ModuleHandle_T hModuleHandle);
OVFS_ALARM_BKBD* ovfs_get_alarm_bkbd();
int ovfs_init_alarm_bkbd(ModuleHandle_T hModuleHandle);
void closeAlarmOut();
#ifdef __cplusplus
}
#endif

#endif /* OVFS_ALARM_METHOD_H_ */
