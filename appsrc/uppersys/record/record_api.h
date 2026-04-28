#ifndef _RECORD_API_H_
#define _RECORD_API_H_

#include <libcommon_api.h>
#include "record_common.h"

namespace cv_soft {

//------------------------------------ 系统控制 -----------------------------------
CV_ERR cv_record_init(void* handle);

CV_ERR cv_record_saveconfig();
//参数need_sync表示是否需要把文件同步到磁盘中去
CV_ERR cv_record_stop_work(U32 need_sync = TRUE);	//为格式化、关机、重启做准备
CV_ERR cv_record_resume_work();	//格式化完毕之后恢复工作

//-------------------------------------- 录像计划控制 -----------------------------------------------------
CV_ERR cv_record_set_record_manual_enable(U32 nvr_ch, U32 enable);
CV_ERR cv_record_get_record_manual_enable(U32 nvr_ch, U32 *enable);

//zhangw:设置录像模式:
//定时录像+报警录像：时间计划和预录、延迟都生效；
//定时录像：时间计划生效，预录、延迟都禁用；
//报警录像：预录、延迟都生效，时间计划禁用；
//停止录像:预录、延迟，时间计划禁用；
CV_ERR cv_record_get_shedule_status(U32 *pSheduleStatus);
CV_ERR cv_record_get_record_mode(U32 nvr_ch, U32 *pmode, U32 *pmask, U32 *pshedmode);
CV_ERR cv_record_set_record_mode(U32 nvr_ch, U32 mode);
CV_ERR cv_record_set_record_manualmask(U32 nvr_ch, U32 mask);
CV_ERR cv_record_set_rec_plan(U32 nvr_ch, const cv_local_rec_plan *plan);
CV_ERR cv_record_set_pre_rec_mode(U32 nvr_ch, U32 mode);
CV_ERR cv_record_set_pre_rec_sec(U32 nvr_ch, U32 sec);	//设置预录时间
CV_ERR cv_record_set_pre_rec_mem(U32 nvr_ch, U32 mem);	//设置预录内存大小
CV_ERR cv_record_set_delay_rec_sec(U32 nvr_ch, U32 sec); //设置延迟录像时间
CV_ERR cv_record_set_rec_plan_day(U32 nvr_ch, U32 day, const cv_record_arming_day *plan);

//-------------------------------------- 录像联动控制 -----------------------------------------------------	
CV_ERR cv_record_set_linkage(U32 rec_ch, const cv_event_id *event_id, U32 enable);

//-------------------------------------- 录像状态获取 -----------------------------------------------------	
CV_ERR cv_record_get_ch_state(U32 rec_ch, U32 *pRecordType, U32 *pRecordMask, U32 *pShedType);	//获取录像状态
CV_ERR cv_record_set_data_magic(U32 magic);
U32 cv_record_ch_is_recording(U32 nvr_ch);

//提出此接口的原因:预览进入实时回放会以当前系统时间为结束时间，开始时间以当前时间向前偏移Xs
//但是此时通道的录像数据可能还在内存中并未刷新至硬盘，因此这里提供一个强制刷新接口。
//---------------------------------------强制刷新cache数据至硬盘-----------------------------------------
CV_ERR cv_record_force_flush(U32 record_ch);

//------------------------断网补录-----------------------------------------------------------------
CV_ERR cv_record_set_replenishment_enable(U32 nvr_ch, U32 enable);
}//namespace cv_soft {
#endif	//#ifndef _RECORD_API_H_

