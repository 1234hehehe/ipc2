#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "record_resource.h"
#include "record_shedule.h"
#include "ipcstorage.h"

#include "cfg_manage_api.h"
#include "libcommon_api.h"
#include <libmodule_api.h>

using namespace cv_soft;
using namespace cv_record;
extern int g_sdcard_errno;

#define CHENML_TEST_PRINTF
#undef CHENML_TEST_PRINTF
//------------------------------------------- cv_record_shedule_ch --------------------------------------
static S32 record_shedule_thread(Common_Thread_T pThHandle, void* para)
{
    LOGD("#################### thread PID = %d enter\n", getpid());

    cv_record_shedule_manage *shedule_manage = (cv_record_shedule_manage *)para;
    while (1)
    {
        shedule_manage->shedule_work();
        Common_Sleep(1, 0);
    }

    LOGD("#################### thread PID = %d exit\n", getpid());
    return 0;
}
//------------------------------------------- cv_record_shedule_ch --------------------------------------
cv_record_shedule_ch::cv_record_shedule_ch(U32 ch_idx, U32 max_ch, U32 max_ai)
{
    m_ch_idx = ch_idx;
    m_max_ch = max_ch;
    m_max_ai = max_ai;
    CV_CLR_ARG(m_rec_plan);

    m_pre_rec_mode = 0;
    m_pre_rec_sec = 0;
    m_pre_rec_mem = (5 << 20);
    m_delay_rec_sec = 0;
    m_delay_cnt = 0;    
    m_rec_mode = STOP_REC;

    for(U32 i = 0; i < sizeof(m_linkage_src) / sizeof(m_linkage_src[0]); i++)
    {
        memset(&m_linkage_src[i], 0, sizeof(cv_record_linkage_src));
        m_linkage_src[i].is_zero = TRUE;
    }

    proc_init_linkage_src();    
    m_wr_file_manage = get_res_manage()->get_wr_file_manage();
	m_last_record_type = CV_REC_TRIG_NONE;
    switch_to_idle_stm();
}

cv_record_shedule_ch::~cv_record_shedule_ch()
{
	//暂时不支持析构
}

CV_ERR cv_record_shedule_ch::set_manual_enable(U32 enable)
{
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::get_manual_enable(U32 *enable)
{
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_record_mode(U32 mode)
{
	m_rec_mode = mode;
	return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_record_manualmask(U32 mask)
{
    m_manuelmask = mask;
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::get_record_mode(U32 *pmode, U32 *pmask, U32 *pshedmode)
{
	U32 triggermode;
    	triggermode = proc_fetch_trigger_type(pmask);
	if(pmode)
	{
		if(triggermode == CV_REC_TRIG_NONE)
			*pmode = 0;
		else if(triggermode == CV_REC_TRIG_BY_REGULAR)
			*pmode = 1;
		else if(triggermode == CV_REC_TRIG_BY_MANUAL)
			*pmode = 2;
		else
			*pmode = 3;
	}
	if(pshedmode)
		*pshedmode = proc_fetch_shed_type();
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::stop_work(U32 need_sync)
{
    m_wr_file_manage->stop_work(m_ch_idx, need_sync);
    switch_to_idle_stm();
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_rec_plan(const cv_local_rec_plan *plan)
{
    m_rec_plan = *plan;
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_rec_plan_day(const cv_record_arming_day *plan, U32 day)
{
	switch(day)
	{
	case 0:
	    m_rec_plan.sunday = *plan;
		break;
	case 1:
		m_rec_plan.monday = *plan;
		break;
	case 2:
		m_rec_plan.tuesday = *plan;
		break;
	case 3:
		m_rec_plan.wednesday = *plan;
		break;
	case 4:
		m_rec_plan.thursday = *plan;
		break;
	case 5:
		m_rec_plan.friday = *plan;
		break;
	case 6:
		m_rec_plan.saturday = *plan;
		break;
	default:
		return CV_ERR_REC_INVALID_PARA;
		break;
	}
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_pre_rec_mode(U32 mode)
{
    m_pre_rec_mode = mode;
    m_wr_file_manage->set_pre_rec_mode(m_ch_idx, mode);
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_pre_rec_sec(U32 sec)
{
    m_pre_rec_sec = sec;
    m_wr_file_manage->set_pre_rec_sec(m_ch_idx, sec);
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_pre_rec_mem(U32 mem)
{
    m_pre_rec_mem = mem;
    m_wr_file_manage->set_pre_rec_mem(m_ch_idx, mem);
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_delay_rec_sec(U32 sec)
{
    m_delay_rec_sec = sec;
    m_delay_cnt = sec;
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_ch::set_linkage(const cv_event_id *event_id, U32 enable)
{
    cv_record_linkage_src *linkage_src = proc_find_linkage_src(event_id);
    if(linkage_src == NULL)
    {
		LOGE("linkage src not found,main_type %d sub_type %d\n",event_id->event_type.main_type,event_id->event_type.sub_type);
		return CV_ERR_REC_INVALID_PARA;
    }

    if((linkage_src->event_type.main_type == CV_EVENT_CH_EVENT) || (linkage_src->event_type.sub_type == CV_EVENT_REMOTE_ALARM_IN))
    {
        U32 ch_idx = (linkage_src->event_type.sub_type == CV_EVENT_REMOTE_ALARM_IN) ? event_id->event_id.alarm_id.src.ch_idx : event_id->event_id.ch_idx;
        if(enable)
            CV_BITSET(linkage_src->linkage_mask.ch_mask, ch_idx, CV_U32_BIT_NUM);
        else 
            CV_BITCLEAR(linkage_src->linkage_mask.ch_mask, ch_idx, CV_U32_BIT_NUM);

        U32 zero_mask[(CV_MAX_LOCAL_CH_NUM + (CV_U32_BIT_NUM - 1)) / CV_U32_BIT_NUM];
        memset(zero_mask, 0, sizeof(zero_mask));
        linkage_src->is_zero = (memcmp(linkage_src->linkage_mask.ch_mask, zero_mask, sizeof(zero_mask)) == 0) ? TRUE : FALSE;
    }
    else if(linkage_src->event_type.main_type == CV_EVENT_ALARM_EVENT)
    {
        U32 pin_idx = event_id->event_id.alarm_id.src.pin_idx;
        if(enable)
            CV_BITSET(linkage_src->linkage_mask.ai_mask, pin_idx, CV_U32_BIT_NUM);
        else
            CV_BITCLEAR(linkage_src->linkage_mask.ai_mask, pin_idx, CV_U32_BIT_NUM);

        U32 zero_mask[(CV_MAX_LOCAL_AI_NUM + (CV_U32_BIT_NUM - 1)) / CV_U32_BIT_NUM];
        memset(zero_mask, 0, sizeof(zero_mask));
        linkage_src->is_zero = (memcmp(linkage_src->linkage_mask.ai_mask, zero_mask, sizeof(zero_mask)) == 0) ? TRUE : FALSE; 
    }
    else if(linkage_src->event_type.main_type == CV_EVENT_NET_EVENT)
    {
        U32 ch_idx = (linkage_src->event_type.sub_type == CV_EVENT_REMOTE_ALARM_IN) ? event_id->event_id.alarm_id.src.ch_idx : event_id->event_id.ch_idx;
        if(enable)
            CV_BITSET(linkage_src->linkage_mask.ch_mask, ch_idx, CV_U32_BIT_NUM);
        else 
            CV_BITCLEAR(linkage_src->linkage_mask.ch_mask, ch_idx, CV_U32_BIT_NUM);

        U32 zero_mask[(CV_MAX_LOCAL_CH_NUM + (CV_U32_BIT_NUM - 1)) / CV_U32_BIT_NUM];
        memset(zero_mask, 0, sizeof(zero_mask));
        linkage_src->is_zero = (memcmp(linkage_src->linkage_mask.ch_mask, zero_mask, sizeof(zero_mask)) == 0) ? TRUE : FALSE;     
    }
    if(linkage_src->trigger_type != CV_REC_TRIG_NONE && (0 == enable))
    {	
       //m_last_record_type = linkage_src->trigger_type;
		//m_delay_cnt = m_delay_rec_sec;
				LOGW("m_last_record_type %d\n",m_last_record_type);
    }
    return CV_SUCCESS;
}

void cv_record_shedule_ch::shedule_work()
{
    if(m_rec_mode == STOP_REC || g_sdcard_errno != 0)
    {
        m_wr_file_manage->stop_record(m_ch_idx);
        m_last_record_type = 0;
        switch_to_idle_stm();
    }
    else
        (this->*(m_shedule_ops))();
    
    return ;
}

void cv_record_shedule_ch::shedule_by_pre_rec_stm()
{
	U32 mask = 0;
    U32 trigger_type = proc_fetch_trigger_type(&mask);
	U32 nSchedMode = proc_fetch_shed_type();
    m_wr_file_manage->set_mask(m_ch_idx, mask);
    if(trigger_type != 0)
    {
        //有事件触发且非常规录像，保留预录文件
        U32 save_rec = ((trigger_type != CV_REC_TRIG_BY_MANUAL) && (trigger_type != CV_REC_TRIG_BY_REGULAR)) ? TRUE : FALSE;
        m_wr_file_manage->start_record(m_ch_idx, trigger_type, save_rec, nSchedMode);        
        m_last_record_type = trigger_type;
        switch_to_regular_stm();
    }
    else if (m_pre_rec_sec <= 0)
    {
        m_wr_file_manage->stop_record(m_ch_idx);
        m_last_record_type = 0;
        switch_to_idle_stm();
    }
    
    return;	
}

void cv_record_shedule_ch::shedule_by_idle_stm()
{
	U32 mask = 0;
    U32 trigger_type = proc_fetch_trigger_type(&mask);
	U32 nSchedMode = proc_fetch_shed_type();
    if(trigger_type != 0)
    {
    	m_wr_file_manage->set_mask(m_ch_idx, mask);
        m_wr_file_manage->start_record(m_ch_idx, trigger_type, FALSE, nSchedMode);
        m_last_record_type = trigger_type;
        switch_to_regular_stm();
    }
    else if (m_pre_rec_sec > 0)
    {
    	m_wr_file_manage->set_mask(m_ch_idx, mask);
        m_wr_file_manage->start_record(m_ch_idx, 0, FALSE, nSchedMode);	//切到预录模式/常规录像
        m_last_record_type = 0;
        switch_to_pre_rec_stm();
    }
    else 
    {
	    //上一次的录像有非常规的事件被触发，可以进入延迟录像阶段
	    U32 need_delay_rec = ((m_last_record_type != CV_REC_TRIG_BY_MANUAL) && (m_last_record_type != CV_REC_TRIG_BY_REGULAR) && (m_last_record_type != CV_REC_TRIG_NONE)) ? TRUE : FALSE;       
	    if(need_delay_rec && (m_delay_rec_sec > 0))
	    {            
	        if(m_delay_cnt > 0)
	        {
	            LOGW("m_delay_cnt %d m_last_record_type %#x\n",m_delay_cnt,m_last_record_type);
	            m_wr_file_manage->start_record(m_ch_idx, m_last_record_type, FALSE, nSchedMode);
	            m_delay_cnt--;
	        }
			else if(m_last_record_type != CV_REC_TRIG_NONE)
			{
				m_wr_file_manage->set_mask(m_ch_idx, mask);
		        m_wr_file_manage->stop_record(m_ch_idx);
		        m_last_record_type = 0;
			}
	    }
    }
    return ;
}

void cv_record_shedule_ch::shedule_by_regular_stm()
{
	U32 mask = 0;
    U32 trigger_type = proc_fetch_trigger_type(&mask);
	U32 nSchedMode = proc_fetch_shed_type();
    if(trigger_type != 0)
    {
		if(trigger_type != m_last_record_type && trigger_type != 1 && m_last_record_type == 1)
		{
			//enter in alarm
			m_last_record_type = trigger_type;
			m_delay_cnt = m_delay_rec_sec;
		}
		else if(trigger_type == 1 && m_last_record_type != 1 && m_delay_cnt == 0)
		{
			//exit in alarm
			m_last_record_type = 0;
			m_delay_cnt = m_delay_rec_sec;
		}
		else if(m_last_record_type != 0 && trigger_type == 1 && m_last_record_type != 1 && m_delay_cnt >= 0)
		{
			//alarm delay
			m_wr_file_manage->start_record(m_ch_idx, m_last_record_type, FALSE, nSchedMode);
            m_delay_cnt--;
			//LOGW("m_delay_cnt %d m_last_record_type %d trigger_type=%d\n",m_delay_cnt,m_last_record_type, trigger_type);
		}
		else
		{
			//alarming
			m_wr_file_manage->set_mask(m_ch_idx, mask);
			m_wr_file_manage->start_record(m_ch_idx, trigger_type, FALSE, nSchedMode);
			m_delay_cnt = m_delay_rec_sec;
			m_last_record_type = trigger_type;
		}
		
        return ;
    }

    //上一次的录像有非常规的事件被触发，可以进入延迟录像阶段
    //U32 need_delay_rec = ((trigger_type != CV_REC_TRIG_BY_MANUAL) && (trigger_type != CV_REC_TRIG_BY_REGULAR)) ? TRUE : FALSE;
    U32 need_delay_rec = ((m_last_record_type != CV_REC_TRIG_BY_REGULAR) && (m_last_record_type != CV_REC_TRIG_BY_MANUAL)&& (m_last_record_type != CV_REC_TRIG_NONE)) ? TRUE : FALSE;
    if(need_delay_rec&& (m_delay_rec_sec > 0))
    {            
        if(m_delay_cnt > 0)
        {
            LOGW("m_delay_cnt %d m_last_record_type %#x\n",m_delay_cnt,m_last_record_type);
            m_wr_file_manage->start_record(m_ch_idx, m_last_record_type, FALSE, nSchedMode);
            m_delay_cnt--;	
            return ;
        }        
    }
    
    //没有设置延迟录像或者延迟录像时间已经为0了还是没有任何事件触发，准备进入预录状态或停止状态
    if (m_pre_rec_sec <= 0)
    {
    	m_wr_file_manage->set_mask(m_ch_idx, mask);
        m_wr_file_manage->stop_record(m_ch_idx);
        switch_to_idle_stm();
    }
    else
    {
    	m_wr_file_manage->set_mask(m_ch_idx, mask);
        m_wr_file_manage->start_record(m_ch_idx, trigger_type, FALSE, nSchedMode);
        switch_to_pre_rec_stm();
    }
    
    return;
}

cv_record_linkage_src *cv_record_shedule_ch::proc_find_linkage_src(const cv_event_id *event_id)
{
    for(U32 src_idx = 0; src_idx < sizeof(m_linkage_src) / sizeof(m_linkage_src[0]); src_idx++)
    {
        cv_record_linkage_src *linkage_src = &m_linkage_src[src_idx];
        if((event_id->event_type.main_type == linkage_src->event_type.main_type) && ((event_id->event_type.sub_type == linkage_src->event_type.sub_type)))
            return linkage_src;
    }

    return NULL;
}

U32 cv_record_shedule_ch::proc_fetch_trigger_type(U32 *pMask)
{
    	U32 trigger_mask = CV_REC_TRIG_NONE;
	if(!pMask)
		return trigger_mask;
	if(m_rec_mode == MANUAL_REC)
	{
		*pMask = m_manuelmask;
		trigger_mask = CV_REC_TRIG_BY_MANUAL;
	}
	else if(m_rec_mode == REGULAR_REC)
	{
		if(proc_need_event_record(pMask))
		{
		    for(U32 src_idx = 0; src_idx < sizeof(m_linkage_src) / sizeof(m_linkage_src[0]); src_idx++)
		    {
		        cv_record_linkage_src *linkage_src = &m_linkage_src[src_idx];
		        if(linkage_src->is_zero)
		            continue;

		        if((linkage_src->event_type.main_type != CV_EVENT_NET_EVENT)&&(linkage_src->event_type.main_type != CV_EVENT_CH_EVENT) && (linkage_src->event_type.main_type != CV_EVENT_ALARM_EVENT))
		        {
					LOGE("linkage_src->event_type.main_type err : %d\n", linkage_src->event_type.main_type);
					return CV_ERR_REC_UNKNOWN;
		        }
		        U32 max_value = (linkage_src->event_type.sub_type == CV_EVENT_LOCAL_ALRAM_IN) ? m_max_ai : m_max_ch;
		        U32 *value = (linkage_src->event_type.sub_type == CV_EVENT_LOCAL_ALRAM_IN) ? linkage_src->linkage_mask.ai_mask : linkage_src->linkage_mask.ch_mask;

		        for(U32 idx = 0; idx < max_value; idx++)
		        {
		            U32 is_trigger = FALSE;
		            if(CV_BITTEST(value, idx,  CV_U32_BIT_NUM) == 0)
		                continue;

		            cv_event_id event_id;
		            memcpy(&event_id.event_type, &linkage_src->event_type, sizeof(cv_event_type));
		            if(event_id.event_type.main_type == CV_EVENT_CH_EVENT)
		                event_id.event_id.ch_idx = idx;
		            else if(event_id.event_type.sub_type == CV_EVENT_LOCAL_ALRAM_IN)
		            {
		                event_id.event_id.alarm_id.is_local = TRUE;
		                event_id.event_id.alarm_id.src.pin_idx = idx;
		            }
		            else if(event_id.event_type.sub_type == CV_EVENT_REMOTE_ALARM_IN)
		            {
		                event_id.event_id.alarm_id.is_local = FALSE;
		                event_id.event_id.alarm_id.src.ch_idx = idx;
		            }
		            else if(event_id.event_type.main_type == CV_EVENT_NET_EVENT)
		                event_id.event_id.ch_idx = idx;
		            else
		            {
						LOGE("linkage_src->event_type err : %d, %d\n", linkage_src->event_type.main_type, linkage_src->event_type.sub_type);
						return CV_ERR_REC_UNKNOWN;
		            }
		            trigger_mask |= linkage_src->trigger_type;

		            //如果已经触发了这种类型的录像，就不用继续查看别的通道了
		            if(is_trigger)
		            {
		                break;
		            }
		        }                        
		    }
		}
		if(CV_REC_TRIG_NONE == trigger_mask)
		{
			if(proc_need_timing_record(pMask))
			{
				trigger_mask = CV_REC_TRIG_BY_REGULAR;
				//m_last_record_type = 0;
				//LOGW("m_last_record_type %d\n",m_last_record_type);
			}
		}
	}

    return trigger_mask;
}

U32 cv_record_shedule_ch::proc_fetch_shed_type()
{
	if(m_rec_mode == REGULAR_REC)
	{
		struct tm p;
		U32 cmp_time = 0;
		time_t current_time = time(NULL);
		Common_LocalTime_r (&current_time, &p);
		cmp_time = p.tm_hour * 100 + p.tm_min;
		const cv_record_arming_day *day[] = {&m_rec_plan.sunday, &m_rec_plan.monday, &m_rec_plan.tuesday, &m_rec_plan.wednesday, &m_rec_plan.thursday, &m_rec_plan.friday, &m_rec_plan.saturday};
		if((p.tm_wday < 0) || (p.tm_wday > 6))
		{
			LOGE("time err : p.day = %d\n", p.tm_wday);
			return CV_ERR_REC_UNKNOWN;
		}
		const cv_record_arming_day *check_day = day[p.tm_wday]; //p.tm_wday = 0 表示周天，后面依次类推
		
		for(U32 idx = 0; idx < sizeof(check_day->arming) / sizeof(check_day->arming[0]); idx++)
		{
			U32 min_time = check_day->arming[idx].time.start_hour_min;
			U32 max_time = check_day->arming[idx].time.stop_hour_min;
#if (defined PLATFORM_JZT32)
			if((cmp_time >= min_time) && (cmp_time <= max_time))
			{
				if(check_day->arming[idx].arming_type&CV_REC_ARMING_BY_TIMER)
				{
					return check_day->arming[idx].arming_type;
				}
				else if(check_day->arming[idx].arming_type&CV_REC_ARMING_BY_EVNET)
				{
					return check_day->arming[idx].arming_type;
				}
			}
#else
			if(check_day->arming[idx].arming_type&CV_REC_ARMING_BY_EVNET)
				return check_day->arming[idx].arming_type;
			else if((cmp_time >= min_time) && (cmp_time <= max_time))
				return check_day->arming[idx].arming_type;
#endif
		}
	}

    return CV_REC_ARMING_NONE;
}

U32 cv_record_shedule_ch::proc_need_timing_record(U32 *pMask)
{
    struct tm p;
    U32 cmp_time = 0;
    time_t current_time = time(NULL);
    Common_LocalTime_r (&current_time, &p);
    cmp_time = p.tm_hour * 100 + p.tm_min;
    const cv_record_arming_day *day[] = {&m_rec_plan.sunday, &m_rec_plan.monday, &m_rec_plan.tuesday, &m_rec_plan.wednesday, &m_rec_plan.thursday, &m_rec_plan.friday, &m_rec_plan.saturday};
    if((p.tm_wday < 0) || (p.tm_wday > 6))
    {
		LOGE("time err : p.day = %d\n", p.tm_wday);
		return FALSE;
    }
    const cv_record_arming_day *check_day = day[p.tm_wday]; //p.tm_wday = 0 表示周天，后面依次类推
    
    for(U32 idx = 0; idx < sizeof(check_day->arming) / sizeof(check_day->arming[0]); idx++)
    {
        U32 min_time = check_day->arming[idx].time.start_hour_min;
        U32 max_time = check_day->arming[idx].time.stop_hour_min;
        if((cmp_time >= min_time) && (cmp_time <= max_time))
        {
			//if(check_day->arming[idx].arming_type == CV_REC_ARMING_BY_TIMER)
#ifdef CHANGE_STREAM
			if(check_day->arming[idx].arming_type == 3)
			{
				*pMask = 14;									//record substream
				//LOGW("time record true\n");
	            return TRUE;
			}
			else if(check_day->arming[idx].arming_type == 1)
			{
				*pMask = check_day->arming[idx].recordmask;		//record mainstream
				//LOGW("time record true\n");
	            return TRUE;
			}
#else
			if(check_day->arming[idx].arming_type&CV_REC_ARMING_BY_TIMER)
			{
				*pMask = check_day->arming[idx].recordmask;
				//LOGW("time record true\n");
	            return TRUE;
			}
#endif
        }
    }

    return FALSE;            
}

U32 cv_record_shedule_ch::proc_need_event_record(U32 *pMask)
{
	struct tm p;
    U32 cmp_time = 0;
    time_t current_time = time(NULL);
    Common_LocalTime_r (&current_time, &p);
    cmp_time = p.tm_hour * 100 + p.tm_min;
    const cv_record_arming_day *day[] = {&m_rec_plan.sunday, &m_rec_plan.monday, &m_rec_plan.tuesday, &m_rec_plan.wednesday, &m_rec_plan.thursday, &m_rec_plan.friday, &m_rec_plan.saturday};
    if((p.tm_wday < 0) || (p.tm_wday > 6))
    {
		LOGE("time err : p.day = %d\n", p.tm_wday);
		return FALSE;
    }
    const cv_record_arming_day *check_day = day[p.tm_wday]; //p.tm_wday = 0 表示周天，后面依次类推
    
    for(U32 idx = 0; idx < sizeof(check_day->arming) / sizeof(check_day->arming[0]); idx++)
    {
#if (defined PLATFORM_JZT32)
        U32 min_time = check_day->arming[idx].time.start_hour_min;
        U32 max_time = check_day->arming[idx].time.stop_hour_min;
        if((cmp_time >= min_time) && (cmp_time <= max_time))
        {
			if(check_day->arming[idx].arming_type&CV_REC_ARMING_BY_EVNET)
			{
				*pMask = check_day->arming[idx].recordmask;
				//LOGW("time record true\n");
	            return TRUE;
			}
        }
#else
#ifdef CHANGE_STREAM
		if(check_day->arming[idx].arming_type == 3)
		{
			*pMask = 14;									//record substream
			//LOGW("time record true\n");
			return TRUE;
		}
		else if(check_day->arming[idx].arming_type == 1)
		{
			*pMask = check_day->arming[idx].recordmask; 	//record mainstream
			//LOGW("time record true\n");
			return TRUE;
		}
#else
		if(check_day->arming[idx].arming_type&CV_REC_ARMING_BY_EVNET)
		{
			*pMask = check_day->arming[idx].recordmask;
			//LOGW("event record true\n");
            return TRUE;
		}
#endif
#endif
    }

    return FALSE;
}

void cv_record_shedule_ch::proc_init_linkage_src()
{        
    U32 fill_idx = 0;
    S32 min_idx = (S32)CV_EVENT_SUB_TYPE_ANY + 1, max_idx = (S32)CV_EVENT_SUB_MAX;
    for(S32 idx = min_idx; idx < max_idx; idx++)
    {
        cv_record_linkage_src *linkage_src = &m_linkage_src[fill_idx];
        linkage_src->event_type.sub_type = (cv_event_sub_type)idx;
        linkage_src->event_type.third_type = CV_EVENT_THIRD_TYPE_UNKNOW;
        
        if((idx > (S32)CV_EVENT_SUB_TYPE_ANY) && (idx < (S32)CV_EVENT_DISK_FULL))
            linkage_src->event_type.main_type = CV_EVENT_CH_EVENT;
        else if((idx > (S32)CV_EVENT_NET_CLIENT_DISCONNECT) && (idx < (S32)CV_EVENT_SUB_MAX))
            linkage_src->event_type.main_type = CV_EVENT_ALARM_EVENT;
        else if((idx > (S32)CV_EVENT_SMART_VIDEO_DIAGNOSE) && (idx < (S32)CV_EVENT_NET_DISCONNECT))
            linkage_src->event_type.main_type = CV_EVENT_DISK_EVENT;
        else             
            linkage_src->event_type.main_type = CV_EVENT_NET_EVENT;

        //录像中不添加具体的三级事件
        if((idx == (S32)CV_EVENT_SMART_SOUND_DETECT) || (idx == (S32)CV_EVENT_SMART_VIDEO_DIAGNOSE))
            linkage_src->event_type.third_type = CV_EVENT_THIRD_TYPE_ANY;

        linkage_src->trigger_type = proc_mapping_trigger_type(&linkage_src->event_type);            
        fill_idx++;
    }
    
    if(fill_idx != CV_ARRAY_DIM(m_linkage_src))
    {
		LOGE("fill_idx = %d\n", fill_idx);
    }
}

U32 cv_record_shedule_ch::proc_mapping_trigger_type(const cv_event_type *event_type)
{
    struct mapping_value {S32 type; U32 trigger_type;};
    mapping_value mapping_array[] = {{(S32)CV_EVENT_MD, (U32)CV_REC_TRIG_BY_MD}, \
                                     {(S32)CV_EVENT_SMART_OBJECT_COUNT, (U32)CV_REC_TRIG_BY_COUNTER}, \
                                     {(S32)CV_EVENT_SMART_CROSS_LINE, (U32)CV_REC_TRIG_BY_WIRE}, \
                                     {(S32)CV_EVENT_SMART_REGION_DETECT, (U32)CV_REC_TRIG_BY_REGION}, \
                                     {(S32)CV_EVENT_SMART_GOODS_DETECT, (U32)CV_REC_TRIG_BY_OBJECT}, \
                                     {(S32)CV_EVENT_SMART_SCENE_CHNAGE, (U32)CV_REC_TRIG_BY_SCENCE}, \
                                     {(S32)CV_EVENT_SMART_SOUND_DETECT, (U32)CV_REC_TRIG_BY_SOUND}, \
                                     {(S32)CV_EVENT_SMART_MOTION, (U32)CV_REC_TRIG_BY_SMOTION}, \
                                     {(S32)CV_EVENT_SMART_FIRE_DETECT, (U32)CV_REC_TRIG_BY_FIRE}, \
                                     {(S32)CV_EVENT_SMART_FACE_DETECT, (U32)CV_REC_TRIG_BY_FACE}, \
                                     {(S32)CV_EVENT_SMART_RECOGNITION_FACE, (U32)CV_REC_TRIG_BY_RECOGNITION_FACE}, \
                                     {(S32)CV_EVENT_TEMPERATURE, (U32)CV_REC_TRIG_BY_TEMPERATURE}, \
                                     {(S32)CV_EVENT_SMART_CAR_PLATE_DETECT, (U32)CV_REC_TRIG_BY_PLATE},\
                                     {(S32)CV_EVENT_SMART_CAR_PLATE_BLACK, (U32)CV_REC_TRIG_BY_PLATE_WHITE}, \
                                     {(S32)CV_EVENT_SMART_CAR_PLATE_WHITE, (U32)CV_REC_TRIG_BY_PLATE_BLACK}, \
                                     {(S32)CV_EVENT_SMART_VIDEO_DIAGNOSE, (U32)CV_REC_TRIG_BY_VIDEO_DIAGNOSE}, \
                                     {(S32)CV_EVENT_SMART_HUMANOID      , (U32)CV_REC_TRIG_BY_HUMANOID	    }, \
                                     {(S32)CV_EVENT_SMART_RUN           , (U32)CV_REC_TRIG_BY_RUN	        }, \
                                     {(S32)CV_EVENT_SMART_VIOLENTMD     , (U32)CV_REC_TRIG_BY_VIOLENTMD	}, \
                                     {(S32)CV_EVENT_SMART_MAXHEIGHT     , (U32)CV_REC_TRIG_BY_MAXHEIGHT	}, \
                                     {(S32)CV_EVENT_SMART_HIGHDENSITY   , (U32)CV_REC_TRIG_BY_HIGHDENSITY	}, \
                                     {(S32)CV_EVENT_SMART_RETROGRADE    , (U32)CV_REC_TRIG_BY_RETROGRADE	}, \
                                     {(S32)CV_EVENT_SMART_SCENCECHANGE  , (U32)CV_REC_TRIG_BY_SCENCECHANGE	}, \
                                     {(S32)CV_EVENT_SMART_DETECT_PERSON , (U32)CV_REC_TRIG_BY_DETECT_PERSON}, \
                                     {(S32)CV_EVENT_SMART_DETECT_MOTOR, (U32)CV_REC_TRIG_BY_DETECT_MOTOR}, \
                                     {(S32)CV_EVENT_NET_DISCONNECT, (U32)CV_REC_TRIG_BY_NET_DISCONNECT}, \
                                     {(S32)CV_EVENT_NET_IP_CONFLICT, (U32)CV_REC_TRIG_BY_NET_IP_CONFLICT}, \
                                     {(S32)CV_EVENT_NET_ILLEG_ACCESS, (U32)CV_REC_TRIG_BY_NET_ILLEG_ACCESS}, \
                                     {(S32)CV_EVENT_NET_CLIENT_DISCONNECT, (U32)CV_REC_TRIG_BY_NET_CLIENT_DISCONN}, \
                                     {(S32)CV_EVENT_LOCAL_ALRAM_IN, (U32)CV_REC_TRIG_BY_AI}, \
                                     {(S32)CV_EVENT_REMOTE_ALARM_IN, (U32)CV_REC_TRIG_BY_AI}};
                                                                 
    S32 cmp_type = (S32)event_type->sub_type;
    for(U32 idx = 0; idx < sizeof(mapping_array) / sizeof(mapping_array[0]); idx++)
    {
        if(cmp_type == mapping_array[idx].type)
            return mapping_array[idx].trigger_type;
    }

    return 0;
}

//-------------------------------------------- cv_record_shedule_manage ----------------------------------------------
cv_record_shedule_manage::cv_record_shedule_manage()
{
    const cv_device_capability *sys_cap = cv_cfgm_get_dev_cap();
    m_ch_num = sys_cap->video_ch_num;
    if(sys_cap->alarm_in_num > sys_cap->video_ch_num)
    {
	LOGW("sys_cap->alarm_in_num= %d, sys_cap->video_ch_num = %d\n", sys_cap->alarm_in_num, sys_cap->video_ch_num);
    }
    m_max_ai = sys_cap->alarm_in_num - sys_cap->video_ch_num;

    for (U32 i = 0; i < m_ch_num; i++)
    {
        m_ch[i] = new cv_record_shedule_ch(i, m_ch_num, m_max_ai);
        if(m_ch[i] == NULL)
        {
			LOGE("new cv_record_shedule_ch_%d err\n", i);
			return;
        }
    }

    m_lock = NULL;
    Common_Lock_Create(&m_lock, "cv_record_shedule_manage");
    if(m_lock == NULL)
    {
		LOGE("lock err\n");
		return;
    }

    m_need_work = FALSE;
	Common_Thread_T ThreadHandle = NULL;
    Common_Thread_Create(&ThreadHandle, "record_shedule_thread", 1024*256, COMMON_THREAD_CREATEFLAG_NORMAL, record_shedule_thread, this);
}

cv_record_shedule_manage::~cv_record_shedule_manage()
{
	//暂时不支持析构
}
 
CV_ERR cv_record_shedule_manage::set_manual_enable(U32 nvr_ch, U32 enable)
{    
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->set_manual_enable(enable);
}

CV_ERR cv_record_shedule_manage::get_manual_enable(U32 nvr_ch, U32 *enable)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }
    
    return m_ch[nvr_ch]->get_manual_enable(enable);
}

CV_ERR cv_record_shedule_manage::set_record_mode(U32 nvr_ch, U32 mode)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->set_record_mode(mode);
}

CV_ERR cv_record_shedule_manage::set_record_manualmask(U32 nvr_ch, U32 mask)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->set_record_manualmask(mask);
}

CV_ERR cv_record_shedule_manage::get_record_mode(U32 nvr_ch, U32 *pmode, U32 *pmask, U32 *pshedmode)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->get_record_mode(pmode, pmask, pshedmode);
}

CV_ERR cv_record_shedule_manage::start_work()
{
    m_need_work = TRUE;
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_manage::stop_work(U32 need_sync)
{
    Common_Lock(m_lock);

    m_need_work = FALSE;
    for (U32 i = 0; i < m_ch_num; i++)
    {
        m_ch[i]->stop_work(need_sync);
    }

    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_manage::get_shedule_status(U32 *pSheduleStatus)
{
    *pSheduleStatus = m_need_work;
    return CV_SUCCESS;
}

CV_ERR cv_record_shedule_manage::set_rec_plan(U32 nvr_ch, const cv_local_rec_plan *plan)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->set_rec_plan(plan);
}

CV_ERR cv_record_shedule_manage::set_rec_plan_day(U32 nvr_ch, U32 day, const cv_record_arming_day *plan)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->set_rec_plan_day(plan, day);
}

CV_ERR cv_record_shedule_manage::set_pre_rec_mode(U32 nvr_ch, U32 mode)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }
    
    return m_ch[nvr_ch]->set_pre_rec_mode(mode);
}

CV_ERR cv_record_shedule_manage::set_pre_rec_sec(U32 nvr_ch, U32 sec)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }
    
    return m_ch[nvr_ch]->set_pre_rec_sec(sec);
}

CV_ERR cv_record_shedule_manage::set_pre_rec_mem(U32 nvr_ch, U32 mem)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }
    
    return m_ch[nvr_ch]->set_pre_rec_mem(mem);
}

CV_ERR cv_record_shedule_manage::set_delay_rec_sec(U32 nvr_ch, U32 sec)
{
    if(!proc_is_valid_ch(nvr_ch))
    {
        LOGE("rec_ch = %d invalid\n", nvr_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[nvr_ch]->set_delay_rec_sec(sec);
}

CV_ERR cv_record_shedule_manage::set_linkage(U32 rec_ch, const cv_event_id *event_id, U32 enable)
{
    if(!proc_is_valid_ch(rec_ch))
    {
        LOGE("Invalid Ch Index[%u]\n", rec_ch);
        return CV_ERR_REC_INVALID_PARA;
    }

    if(!proc_is_valid_event_id(event_id))
    {
        //REC_CLASS_PE("Invalid Event ID\n");
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[rec_ch]->set_linkage(event_id, enable);
}

void cv_record_shedule_manage::shedule_work()
{
    Common_Lock(m_lock);
    if(!m_need_work)
    {
		Common_UnLock(m_lock);
        return ;
    }
       
    for (U32 i = 0; i < m_ch_num; i++)
    {
        m_ch[i]->shedule_work();
    }

    Common_UnLock(m_lock);
    return;
}

void cv_record_shedule_manage::shedule_replenish_work()
{
    return;
}


inline U32 cv_record_shedule_manage::proc_is_valid_ch(U32 rec_ch)
{
    if(rec_ch >= m_ch_num)
        return FALSE;

    return TRUE;
}

U32 cv_record_shedule_manage::proc_is_valid_event_id(const cv_event_id *event_id)
{
    const cv_event_type *event_type = &event_id->event_type;
    //当前至于通道类和报警输入类事件支持录像联动
    if((event_type->main_type != CV_EVENT_NET_EVENT)&&(event_type->main_type != CV_EVENT_CH_EVENT) && (event_type->main_type != CV_EVENT_ALARM_EVENT))
        return FALSE;

    U32 max_value = (event_type->sub_type == CV_EVENT_LOCAL_ALRAM_IN) ? m_max_ai : m_ch_num;
    U32 cmp_value = 0;
    if(event_type->main_type == CV_EVENT_CH_EVENT)
        cmp_value = event_id->event_id.ch_idx;
    else if(event_type->sub_type == CV_EVENT_LOCAL_ALRAM_IN)   
        cmp_value = event_id->event_id.alarm_id.src.pin_idx;
    else if(event_type->sub_type == CV_EVENT_REMOTE_ALARM_IN)
        cmp_value = event_id->event_id.alarm_id.src.ch_idx;
    else if(event_type->main_type == CV_EVENT_NET_EVENT)
	cmp_value = event_id->event_id.ch_idx;
    else
    {
		LOGE("event_type err : %d, %d\n", event_type->main_type, event_type->sub_type);
        return FALSE;
    }

    if(cmp_value >= max_value)
        return FALSE;
        
    return TRUE;    
}

