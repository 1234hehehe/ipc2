#ifndef _RECORD_SHEDULE_H_
#define _RECORD_SHEDULE_H_

#include <libcommon_api.h>
#include "record_common.h"

#include "record_wr_file.h"

using namespace cv_soft;

/******************************************************************************************************
1. 分为三种模式：
   停止录像：啥都不做。
   手动录像：只要有空间有数据就一直录，
   计划录像：按照计划来。
2. 每个通道有7个计划项，分别对应周一到周日
3. 每个计划项有8个时间段，
4. 每个时间段有独立的起始、结束时间和类型。
   注：如果时间段的起始时间 >= 结束时间，认为该时间段无效
         如果几个时间段有重合，取第一个生效的时间段，后几个无效
5. 类型有五种：
   定时录像：只要是时间段内，就录像。
   移动侦测： 只要是时间段内，并且发生了移动侦测，就录像
   报警侦测： 只要是时间段内，并且发生了报警，就录像
  报警并且移动： 只要是时间段内，并且发生了报警和移动，就录像
  报警或者移动： 只要是时间段内，并且发生了报警或者移动，就录像
6. 在计划录像中，如果设置了预录、延迟录像的时间，就进行预录和延迟录像的处理。
   注：定时录像不需要处理预录和延迟录像，预录和延迟录像只在时间段内生效
******************************************************************************************************/
namespace cv_record
{

class cv_record_shedule_ch;
typedef void(cv_record_shedule_ch:: *shedule_ops) ();

typedef struct
{
    U32 is_zero;                   //联动源是否为全0 (全0直接跳过，节约检测时间)
    cv_event_type event_type;  //联动的事件类型
    U32 trigger_type;           //事件类型对应的触发类型

    union{
        U32 ch_mask[(CV_MAX_LOCAL_CH_NUM + (CV_U32_BIT_NUM - 1)) / CV_U32_BIT_NUM]; //通道类事件通道号掩码
        U32 ai_mask[(CV_MAX_LOCAL_AI_NUM + (CV_U32_BIT_NUM - 1)) / CV_U32_BIT_NUM];   //本地报警输入事件通道号掩码
    }linkage_mask;
}cv_record_linkage_src;

class cv_record_shedule_ch
{
public:
    CV_ERR stop_work(U32 need_sync);
    CV_ERR set_rec_plan(const cv_local_rec_plan *plan);
	CV_ERR set_rec_plan_day(const cv_record_arming_day *plan, U32 day);

    CV_ERR set_pre_rec_mode(U32 mode);
    CV_ERR set_pre_rec_sec(U32 sec);
    CV_ERR set_pre_rec_mem(U32 mem);
    CV_ERR set_delay_rec_sec(U32 sec);
    CV_ERR set_linkage(const cv_event_id *event_id, U32 enable);
    CV_ERR set_manual_enable(U32 enable);
    CV_ERR get_manual_enable(U32 *enable);
    CV_ERR set_record_mode(U32 mode);
	CV_ERR set_record_manualmask(U32 mask);
	CV_ERR get_record_mode(U32 *pmode, U32 *pmask, U32 *pshedmode);

public:
    void shedule_work();


private:
    void shedule_by_idle_stm();
    void shedule_by_pre_rec_stm();
    void shedule_by_regular_stm();

private:
    void switch_to_idle_stm() {m_shedule_ops = &cv_record_shedule_ch::shedule_by_idle_stm;};
    void switch_to_pre_rec_stm() {m_shedule_ops = &cv_record_shedule_ch::shedule_by_pre_rec_stm;};
    void switch_to_regular_stm() {m_shedule_ops = &cv_record_shedule_ch::shedule_by_regular_stm;};
	
private:
    cv_record_linkage_src *proc_find_linkage_src(const cv_event_id *event_id);
    
    U32 proc_fetch_trigger_type(U32 *pMask);
    U32 proc_need_timing_record(U32 *pMask);
    U32 proc_need_event_record(U32 *pMask);
	U32 proc_fetch_shed_type();

    void proc_init_linkage_src();
    U32 proc_mapping_trigger_type(const cv_event_type *event_type);

private:
    U32 m_ch_idx;
    U32 m_max_ch;
    U32 m_max_ai;
    cv_record_wr_file_manage *m_wr_file_manage;

    shedule_ops m_shedule_ops;
    cv_local_rec_plan m_rec_plan;

    U32 m_pre_rec_mode;	//预录mode
    U32 m_pre_rec_sec;	//预录时间长度
    U32 m_pre_rec_mem;	//预录内存大小
    U32 m_delay_rec_sec;	//延迟录像时间长度
    U32 m_delay_cnt;		//延迟闹钟计时
    S32 m_rec_mode;		 //录像模式
    S32 m_manuelmask;
    U32 m_last_record_type;  //上一次录像的触发类型，为了延时录像时候判断类型
    cv_record_linkage_src m_linkage_src[(S32)CV_EVENT_SUB_MAX -2];
    
public:
    cv_record_shedule_ch(U32 ch_idx, U32 max_ch, U32 max_ai);
    ~cv_record_shedule_ch();
    const char* class_name() {return "cv_record_shedule_ch";};
    
private:
    cv_record_shedule_ch(const cv_record_shedule_ch &other);
    cv_record_shedule_ch &operator=(const cv_record_shedule_ch &other);
};

class cv_record_shedule_manage
{
public:
	CV_ERR start_work();
    CV_ERR stop_work(U32 need_sync);	//停止工作，为格式化、关机做准备

    CV_ERR set_manual_enable(U32 nvr_ch, U32 enable);
    CV_ERR get_manual_enable(U32 nvr_ch, U32 *enable);
    CV_ERR set_record_mode(U32 nvr_ch, U32 mode);
	CV_ERR set_record_manualmask(U32 nvr_ch, U32 mask);
	CV_ERR get_record_mode(U32 nvr_ch, U32 *pmode, U32 *pmask, U32 *pshedmode);
	CV_ERR get_shedule_status(U32 *pSheduleStatus);
    CV_ERR set_rec_plan(U32 nvr_ch, const cv_local_rec_plan *plan);
	CV_ERR set_rec_plan_day(U32 nvr_ch, U32 day, const cv_record_arming_day *plan);
    CV_ERR set_pre_rec_mode(U32 nvr_ch, U32 mode);
    CV_ERR set_pre_rec_sec(U32 nvr_ch, U32 sec);
    CV_ERR set_pre_rec_mem(U32 nvr_ch, U32 mem);
    CV_ERR set_delay_rec_sec(U32 nvr_ch, U32 sec);
    CV_ERR set_linkage(U32 rec_ch, const cv_event_id *event_id, U32 enable);

public:    
    void shedule_work();
    void shedule_replenish_work();
    
private:
    U32 proc_is_valid_ch(U32 nvr_ch);
    U32 proc_is_valid_event_id(const cv_event_id *event_id);

private:
    //pthread_t m_thr_hand;
	//pthread_t m_thr_replenish_hand;
	
    Common_Lock_T m_lock;//同步锁
    U32 m_need_work;

    U32 m_ch_num;
    U32 m_max_ai;
    cv_record_shedule_ch *m_ch[CV_MAX_LOCAL_CH_NUM];//每一个通道存在一个计划管理的类
public:
    cv_record_shedule_manage();
    ~cv_record_shedule_manage();
    const char* class_name() {return "cv_record_shedule_manage";};
    
private:
    cv_record_shedule_manage(const cv_record_shedule_manage &other);
    cv_record_shedule_manage &operator=(const cv_record_shedule_manage &other);
};

} //namespace cv_record{
#endif //#ifndef _RECORD_SHEDULE_H_

