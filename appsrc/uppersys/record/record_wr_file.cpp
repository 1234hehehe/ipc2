#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
//#include <sched.h>
//#include <pthread.h>

#include "record_resource.h"
#include "record_wr_file.h"
#include "ipcstorage.h"

#include "cfg_manage_api.h"
#include "libcommon_api.h"

using namespace cv_soft;
using namespace cv_record;

#define SCHED_OTHER	0
#define SCHED_FIFO	1
#define SCHED_RR		2

//-------------------------------------------- record_stop_work_stm ---------------------------------------------------
record_stop_work_stm::record_stop_work_stm(cv_record_wr_file_ch *ch)
{
    m_ch = ch;
}

record_stop_work_stm::~record_stop_work_stm()
{}

CV_ERR record_stop_work_stm::work()
{
    m_ch->m_record_status = CV_RECORD_STATUS_STOP;
    return CV_ERR_REC_BASE;
}

CV_ERR record_stop_work_stm::stop_work(U32 need_sync)//停止工作，关闭文件，为格式化、关机做准备
{
    return CV_SUCCESS;
}

//save_pre_rec描述是否保留预录文件
CV_ERR record_stop_work_stm::start_record(U32 trig_type, U32 save_pre_rec)
{
	switch(trig_type)
	{
	case CV_REC_TRIG_NONE:
	    //m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
	   m_ch->m_usr_set_trig_type =  CV_RECORD_BEGIN;
		break;
	case CV_REC_TRIG_BY_REGULAR:
		m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
		break;
	case CV_REC_TRIG_BY_MD:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTION;
		break;
#if 0
	case CV_REC_TRIG_BY_AI|CV_RECORD_MOTION:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTIONANDALARM;
		break;
#endif
	case CV_REC_TRIG_BY_AI:
	case CV_REC_TRIG_BY_COUNTER:
	case CV_REC_TRIG_BY_WIRE:
	case CV_REC_TRIG_BY_REGION:
	case CV_REC_TRIG_BY_OBJECT:
	case CV_REC_TRIG_BY_SCENCE:
	case CV_REC_TRIG_BY_SOUND:
	case CV_REC_TRIG_BY_SMOTION:
	case CV_REC_TRIG_BY_PLATE:
	case CV_REC_TRIG_BY_RUN:
	case CV_REC_TRIG_BY_VIOLENTMD:
	case CV_REC_TRIG_BY_MAXHEIGHT:
	case CV_REC_TRIG_BY_HIGHDENSITY:
	case CV_REC_TRIG_BY_RETROGRADE:
	case CV_REC_TRIG_BY_SCENCECHANGE:
	case CV_REC_TRIG_BY_TEMPERATURE:
	case CV_REC_TRIG_BY_NET_DISCONNECT:
	case CV_REC_TRIG_BY_NET_ILLEG_ACCESS:
	case CV_REC_TRIG_BY_NET_IP_CONFLICT:
	case CV_REC_TRIG_BY_NET_CLIENT_DISCONN:
    case CV_REC_TRIG_BY_DETECT_MOTOR:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	case CV_REC_TRIG_BY_FIRE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FIRE_DETECT;
		break;
	case CV_REC_TRIG_BY_RECOGNITION_FACE:
	case CV_REC_TRIG_BY_FACE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FACE_DETECT;
		break;
	case CV_REC_TRIG_BY_VIDEO_DIAGNOSE:
		m_ch->m_usr_set_trig_type = CV_RECORD_VIDEODIAGNOSE;
		break;
	case CV_REC_TRIG_BY_HUMANOID:
	case CV_REC_TRIG_BY_DETECT_PERSON:
		m_ch->m_usr_set_trig_type = CV_RECORD_PERSON_DETECT;
		break;
	
	case CV_REC_TRIG_BY_MANUAL:
		m_ch->m_usr_set_trig_type = CV_RECORD_MANUAL;
		break;
	default:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	}

    if (trig_type == CV_REC_TRIG_BY_PRE)
    {
        if (m_ch->m_pre_rec_sec > 0)
        {
            m_ch->switch_to_pre_rec_stm();
        }
    }
    else        
        m_ch->switch_to_normal_rec_stm();
	
    return CV_SUCCESS;
}

//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
CV_ERR record_stop_work_stm::stop_record()
{
    return CV_SUCCESS;
}

//获取当前录像的类型
U32 record_stop_work_stm::get_rec_trig_type()
{
    return CV_REC_TRIG_NONE;
}

CV_ERR record_stop_work_stm::set_pre_rec_mode(U32 mode)
{
    m_ch->m_pre_rec_mode = mode;
    return CV_SUCCESS;
}

CV_ERR record_stop_work_stm::set_pre_rec_sec(U32 sec)
{
    m_ch->m_pre_rec_sec = sec;
    return CV_SUCCESS;
}

CV_ERR record_stop_work_stm::set_pre_rec_mem(U32 mem)
{
    m_ch->PreRecordMem = mem;
    return CV_SUCCESS;
}

//-------------------------------------------- record_normal_rec_stm ---------------------------------------------------
record_normal_rec_stm::record_normal_rec_stm(cv_record_wr_file_ch *ch)
{
    m_ch = ch;
}

record_normal_rec_stm::~record_normal_rec_stm()
{}

CV_ERR record_normal_rec_stm::work()
{   
    //先尝试获取数据，获取成功之后才检查是否需要开文件，免得文件开了没有数据，浪费硬盘空间和节点
    if (m_ch->proc_fetch_rec_data() != CV_SUCCESS)
    {
        return CV_ERR_REC_OPERATE_FAIL;
    }
    return m_ch->proc_save_to_avss(0);
}

CV_ERR record_normal_rec_stm::stop_work(U32 need_sync)//停止工作，关闭文件，为格式化、关机做准备
{
    m_ch->proc_rel_rec_data(TRUE);
    m_ch->switch_to_stop_work_stm();

#if 0
    m_ch->m_usr_set_trig_type = CV_RECORD_BEGIN;
#endif
    m_ch->m_bRecord = FALSE;
    return CV_SUCCESS;
}

//save_pre_rec描述是否保留预录文件
CV_ERR record_normal_rec_stm::start_record(U32 trig_type, U32 save_pre_rec)
{
    //avss采用的是容器方式，直接修改之后灌输数据的类型就可以了
	switch(trig_type)
	{
	case CV_REC_TRIG_NONE:
	    //m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
	   m_ch->m_usr_set_trig_type =  CV_RECORD_BEGIN;
		break;
	case CV_REC_TRIG_BY_REGULAR:
		m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
		break;
	case CV_REC_TRIG_BY_MD:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTION;
		break;
#if 0
	case CV_REC_TRIG_BY_AI|CV_RECORD_MOTION:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTIONANDALARM;
		break;
#endif
	case CV_REC_TRIG_BY_AI:
	case CV_REC_TRIG_BY_COUNTER:
	case CV_REC_TRIG_BY_WIRE:
	case CV_REC_TRIG_BY_REGION:
	case CV_REC_TRIG_BY_OBJECT:
	case CV_REC_TRIG_BY_SCENCE:
	case CV_REC_TRIG_BY_SOUND:
	case CV_REC_TRIG_BY_SMOTION:
	case CV_REC_TRIG_BY_PLATE:
	case CV_REC_TRIG_BY_RECOGNITION_FACE:
	case CV_REC_TRIG_BY_TEMPERATURE:
	case CV_REC_TRIG_BY_NET_DISCONNECT:
	case CV_REC_TRIG_BY_NET_ILLEG_ACCESS:
	case CV_REC_TRIG_BY_NET_IP_CONFLICT:
	case CV_REC_TRIG_BY_NET_CLIENT_DISCONN:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	case CV_REC_TRIG_BY_FIRE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FIRE_DETECT;
		break;
	case CV_REC_TRIG_BY_FACE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FACE_DETECT;
		break;
	case CV_REC_TRIG_BY_VIDEO_DIAGNOSE:
		m_ch->m_usr_set_trig_type = CV_RECORD_VIDEODIAGNOSE;
		break;
	case CV_REC_TRIG_BY_MANUAL:
		m_ch->m_usr_set_trig_type = CV_RECORD_MANUAL;
		break;
	default:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	}
    if (trig_type == CV_REC_TRIG_BY_PRE)
    {
        m_ch->proc_rel_rec_data(TRUE);

        if (m_ch->m_pre_rec_sec > 0)
            m_ch->switch_to_pre_rec_stm();
        else
            m_ch->switch_to_stop_work_stm();
    }

    return CV_SUCCESS;
}

//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，等到半小时切换文件的时候，如果还不恢复录像，则关闭文件后切到停止工作模式
CV_ERR record_normal_rec_stm::stop_record()
{
    m_ch->proc_rel_rec_data(TRUE);
    m_ch->switch_to_pause_rec_stm();

#if 0
    m_ch->m_usr_set_trig_type = CV_RECORD_BEGIN;
    m_ch->m_avss_trig_type = CV_RECORD_BEGIN;
#endif
    m_ch->m_bRecord = FALSE;
    return CV_SUCCESS;
}

//获取当前录像的类型
U32 record_normal_rec_stm::get_rec_trig_type()
{
    return m_ch->m_avss_trig_type;
}

CV_ERR record_normal_rec_stm::set_pre_rec_mode(U32 mode)
{
    m_ch->m_pre_rec_mode = mode;
    return CV_SUCCESS;
}

CV_ERR record_normal_rec_stm::set_pre_rec_sec(U32 sec)
{
    m_ch->m_pre_rec_sec = sec;
    return CV_SUCCESS;
}

CV_ERR record_normal_rec_stm::set_pre_rec_mem(U32 mem)
{
    m_ch->PreRecordMem = mem;
    return CV_SUCCESS;
}

//-------------------------------------------- record_pause_rec_stm ---------------------------------------------------
record_pause_rec_stm::record_pause_rec_stm(cv_record_wr_file_ch *ch)
{
    m_ch = ch;
}

record_pause_rec_stm::~record_pause_rec_stm()
{}

CV_ERR record_pause_rec_stm::work()
{
    //暂停状态下到达切文件的时机，关闭文件，切换到停止状态。
    m_ch->m_record_status = CV_RECORD_STATUS_PAUSE; 

    return  CV_ERR_REC_BASE; 
}

CV_ERR record_pause_rec_stm::stop_work(U32 need_sync)//停止工作，关闭文件，为格式化、关机做准备
{
    m_ch->proc_flush_avss();
    m_ch->switch_to_stop_work_stm();
    return CV_SUCCESS;
}

//save_pre_rec描述是否保留预录文件
CV_ERR record_pause_rec_stm::start_record(U32 trig_type, U32 save_pre_rec)
{
	switch(trig_type)
	{
	case CV_REC_TRIG_NONE:
		//m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
		m_ch->m_usr_set_trig_type =  CV_RECORD_BEGIN;
		break;
	case CV_REC_TRIG_BY_REGULAR:
		m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
		break;
	case CV_REC_TRIG_BY_MD:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTION;
		break;
#if 0
	case CV_REC_TRIG_BY_AI|CV_RECORD_MOTION:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTIONANDALARM;
		break;
#endif
	case CV_REC_TRIG_BY_AI:
	case CV_REC_TRIG_BY_COUNTER:
	case CV_REC_TRIG_BY_WIRE:
	case CV_REC_TRIG_BY_REGION:
	case CV_REC_TRIG_BY_OBJECT:
	case CV_REC_TRIG_BY_SCENCE:
	case CV_REC_TRIG_BY_SOUND:
	case CV_REC_TRIG_BY_SMOTION:
	case CV_REC_TRIG_BY_PLATE:
	case CV_REC_TRIG_BY_RECOGNITION_FACE:
	case CV_REC_TRIG_BY_TEMPERATURE:
	case CV_REC_TRIG_BY_NET_DISCONNECT:
	case CV_REC_TRIG_BY_NET_ILLEG_ACCESS:
	case CV_REC_TRIG_BY_NET_IP_CONFLICT:
	case CV_REC_TRIG_BY_NET_CLIENT_DISCONN:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	case CV_REC_TRIG_BY_FIRE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FIRE_DETECT;
		break;
	case CV_REC_TRIG_BY_FACE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FACE_DETECT;
		break;
	case CV_REC_TRIG_BY_VIDEO_DIAGNOSE:
		m_ch->m_usr_set_trig_type = CV_RECORD_VIDEODIAGNOSE;
		break;
	case CV_REC_TRIG_BY_MANUAL:
		m_ch->m_usr_set_trig_type = CV_RECORD_MANUAL;
		break;
	default:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	}
    if (trig_type == CV_REC_TRIG_BY_PRE)
    {
        if (m_ch->m_pre_rec_sec > 0)
            m_ch->switch_to_pre_rec_stm();
        else
            m_ch->switch_to_stop_work_stm();
    }
    else
    {
        m_ch->switch_to_normal_rec_stm();
    }

    return CV_SUCCESS;
}

//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，等到半小时切换文件的时候，如果还不恢复录像，则关闭文件后切到停止工作模式
CV_ERR record_pause_rec_stm::stop_record()
{
    m_ch->proc_flush_avss();
    return CV_SUCCESS;
}

//获取当前录像的类型
U32 record_pause_rec_stm::get_rec_trig_type()
{
    return CV_REC_TRIG_NONE;
}

CV_ERR record_pause_rec_stm::set_pre_rec_mode(U32 mode)
{
    m_ch->m_pre_rec_mode = mode;
    return CV_SUCCESS;
}

CV_ERR record_pause_rec_stm::set_pre_rec_sec(U32 sec)
{
    m_ch->m_pre_rec_sec = sec;
    return CV_SUCCESS;
}

CV_ERR record_pause_rec_stm::set_pre_rec_mem(U32 mem)
{
    m_ch->PreRecordMem = mem;
    return CV_SUCCESS;
}

//-------------------------------------------- record_pre_rec_stm ---------------------------------------------------
record_pre_rec_stm::record_pre_rec_stm(cv_record_wr_file_ch *ch)
{
    m_ch = ch;
}

record_pre_rec_stm::~record_pre_rec_stm()
{}

CV_ERR record_pre_rec_stm::work()
{
    //先尝试获取数据，获取成功之后才检查是否需要开文件，免得文件开了没有数据，浪费硬盘空间和节点
    if (m_ch->proc_fetch_rec_data() != CV_SUCCESS)
    {
        return CV_ERR_REC_OPERATE_FAIL;
    }
    return m_ch->proc_save_to_avss(1);
}

//这里有个小bug，格式化的时候会导致预录丢失，如果要解决这个问题，需要改很多东西，暂时不花精力在这上面，等用户提了再说
CV_ERR record_pre_rec_stm::stop_work(U32 need_sync)
{
    m_ch->proc_rel_rec_data(TRUE);
    m_ch->proc_flush_avss();

    m_ch->switch_to_stop_work_stm();
#if 0
    m_ch->m_usr_set_trig_type = CV_RECORD_BEGIN;
#endif
    m_ch->m_bRecord = FALSE;
    return CV_SUCCESS;
}

//save_pre_rec描述是否保留预录文件
CV_ERR record_pre_rec_stm::start_record(U32 trig_type, U32 save_pre_rec)
{
    if (trig_type == CV_REC_TRIG_BY_PRE)
    {
        return CV_SUCCESS;
    }

	switch(trig_type)
	{
	case CV_REC_TRIG_NONE:
		//m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
		m_ch->m_usr_set_trig_type =  CV_RECORD_BEGIN;
		break;
	case CV_REC_TRIG_BY_REGULAR:
		m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
		break;
	case CV_REC_TRIG_BY_MD:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTION;
		break;
#if 0
	case CV_REC_TRIG_BY_AI|CV_RECORD_MOTION:
		m_ch->m_usr_set_trig_type = CV_RECORD_MOTIONANDALARM;
		break;
#endif
	case CV_REC_TRIG_BY_AI:
	case CV_REC_TRIG_BY_COUNTER:
	case CV_REC_TRIG_BY_WIRE:
	case CV_REC_TRIG_BY_REGION:
	case CV_REC_TRIG_BY_OBJECT:
	case CV_REC_TRIG_BY_SCENCE:
	case CV_REC_TRIG_BY_SOUND:
	case CV_REC_TRIG_BY_SMOTION:
	case CV_REC_TRIG_BY_PLATE:
	case CV_REC_TRIG_BY_RECOGNITION_FACE:
	case CV_REC_TRIG_BY_TEMPERATURE:
	case CV_REC_TRIG_BY_NET_DISCONNECT:
	case CV_REC_TRIG_BY_NET_ILLEG_ACCESS:
	case CV_REC_TRIG_BY_NET_IP_CONFLICT:
	case CV_REC_TRIG_BY_NET_CLIENT_DISCONN:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	case CV_REC_TRIG_BY_FIRE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FIRE_DETECT;
		break;
	case CV_REC_TRIG_BY_FACE:
		m_ch->m_usr_set_trig_type = CV_RECORD_FACE_DETECT;
		break;
	case CV_REC_TRIG_BY_VIDEO_DIAGNOSE:
		m_ch->m_usr_set_trig_type = CV_RECORD_VIDEODIAGNOSE;
		break;
	case CV_REC_TRIG_BY_MANUAL:
		m_ch->m_usr_set_trig_type = CV_RECORD_MANUAL;
		break;
	default:
		m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
		break;
	}
    if (save_pre_rec)	//如果需要预录数据，就处理数据合并
    {
		switch(trig_type)
		{
		case CV_REC_TRIG_NONE:
		    m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
			break;
		case CV_REC_TRIG_BY_REGULAR:
			m_ch->m_usr_set_trig_type = CV_RECORD_TIMER;
			break;
		case CV_REC_TRIG_BY_MD:
			m_ch->m_usr_set_trig_type = CV_RECORD_MOTION;
			break;
#if 0
		case CV_REC_TRIG_BY_AI|CV_RECORD_MOTION:
			m_ch->m_usr_set_trig_type = CV_RECORD_MOTIONANDALARM;
			break;
#endif
		case CV_REC_TRIG_BY_AI:
		case CV_REC_TRIG_BY_COUNTER:
		case CV_REC_TRIG_BY_WIRE:
		case CV_REC_TRIG_BY_REGION:
		case CV_REC_TRIG_BY_OBJECT:
		case CV_REC_TRIG_BY_SCENCE:
		case CV_REC_TRIG_BY_SOUND:
		case CV_REC_TRIG_BY_SMOTION:
		case CV_REC_TRIG_BY_PLATE:
		case CV_REC_TRIG_BY_RECOGNITION_FACE:
		case CV_REC_TRIG_BY_TEMPERATURE:
		case CV_REC_TRIG_BY_NET_DISCONNECT:
		case CV_REC_TRIG_BY_NET_ILLEG_ACCESS:
		case CV_REC_TRIG_BY_NET_IP_CONFLICT:
		case CV_REC_TRIG_BY_NET_CLIENT_DISCONN:
			m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
			break;
		case CV_REC_TRIG_BY_FIRE:
			m_ch->m_usr_set_trig_type = CV_RECORD_FIRE_DETECT;
			break;
		case CV_REC_TRIG_BY_FACE:
			m_ch->m_usr_set_trig_type = CV_RECORD_FACE_DETECT;
			break;
		case CV_REC_TRIG_BY_VIDEO_DIAGNOSE:
			m_ch->m_usr_set_trig_type = CV_RECORD_VIDEODIAGNOSE;
			break;
		case CV_REC_TRIG_BY_MANUAL:
			m_ch->m_usr_set_trig_type = CV_RECORD_MANUAL;
			break;
		default:
			m_ch->m_usr_set_trig_type = CV_RECORD_ALARM;
			break;
		}
		m_ch->m_bRecord = TRUE;
		LOGW("RecStop!. m_avss_trig_type=[%d]\n",m_ch->m_avss_trig_type);
		ANTS_AviFile_RecStop(m_ch->m_ch_idx);
		LOGW("RecStart!. m_usr_set_trig_type=[%d]\n",m_ch->m_usr_set_trig_type);
		if(avis_ret_ok == ANTS_AviFile_RecStart(m_ch->m_ch_idx, m_ch->m_usr_set_trig_type))
		{
			m_ch->m_avss_trig_type = m_ch->m_usr_set_trig_type;
		}
    }
    else
    {
        m_ch->proc_rel_rec_data(TRUE);
    }
    m_ch->switch_to_normal_rec_stm();

    return CV_SUCCESS;
}

//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
CV_ERR record_pre_rec_stm::stop_record()
{
    m_ch->proc_rel_rec_data(TRUE);
    m_ch->proc_flush_avss();
    m_ch->switch_to_stop_work_stm();
#if 0
    m_ch->m_usr_set_trig_type = CV_RECORD_BEGIN;
#endif
    m_ch->m_bRecord = FALSE;
    return CV_SUCCESS;
}

//获取当前录像的类型
U32 record_pre_rec_stm::get_rec_trig_type()
{
    return CV_REC_TRIG_NONE;
}

CV_ERR record_pre_rec_stm::set_pre_rec_mode(U32 mode)
{
    m_ch->m_pre_rec_mode = mode;
    return CV_SUCCESS;
}

CV_ERR record_pre_rec_stm::set_pre_rec_sec(U32 sec)
{
    m_ch->m_pre_rec_sec = sec;
    if (m_ch->m_pre_rec_sec == 0)
    {
        m_ch->proc_rel_rec_data(TRUE);

        m_ch->switch_to_stop_work_stm();
#if 0
        m_ch->m_usr_set_trig_type = CV_RECORD_BEGIN;
#endif
    }
    return CV_SUCCESS;
}

CV_ERR record_pre_rec_stm::set_pre_rec_mem(U32 mem)
{
    m_ch->PreRecordMem = mem;
    return CV_SUCCESS;
}
	
//-------------------------------------------- cv_record_wr_file_ch --------------------------------------------------
cv_record_wr_file_ch::cv_record_wr_file_ch(U32 ch_idx)
	: m_stop_work_stm(this),
	m_normal_rec_stm(this),
	m_pause_rec_stm(this),
	m_pre_rec_stm(this)
{
    m_ch_idx = ch_idx;
    m_usr_set_trig_type = CV_RECORD_BEGIN;
    m_avss_trig_type = CV_RECORD_BEGIN;

    m_pre_rec_mode = 0;
    m_normal_rec_sec = 0;
    m_pre_rec_sec = 0;
    m_file_open_time = 0;
    m_file_is_open = FALSE;
    m_bRecord = FALSE;
	m_recordmask = 0;
	m_lstrecordmask = 0;
	for(int i = 0; i < CV_ENC_STR_MAX; i ++)
	{
		m_ConnHandle[i] = 0;
		m_bChangeHandle[i] = FALSE;
		CV_CLR_ARG(m_data[i]);
	}

    m_fetch_ok_time = 0;

    CV_CLR_ARG(m_rec_dir);
    m_cur_avss_dir_idx = 0;

    m_lock = NULL;
    Common_Lock_Create(&m_lock, "cv_record_wr_file_ch");
    if(m_lock == NULL)
    {
		LOGE("lock err\n");
    }

    m_record_status = CV_RECORD_STATUS_UNKNOW;
    m_stop_cnt = 25;

	prestarttime.tv_sec = 0;
	prestarttime.tv_usec = 0;
	precurtime.tv_sec = 0;
	precurtime.tv_usec = 0;
	bPreRecordNeedKey_Main = 1;
	bPreRecordNeedKey_Aux = 1;
	pQueueHead = NULL;
	pQueueTail = NULL;
	PreRecordUseMem = 0;
	PreRecordMem = (5 << 20);
	m_lstFrameNo = 0;
    switch_to_stop_work_stm();
}

cv_record_wr_file_ch::~cv_record_wr_file_ch()
{
}


CV_ERR cv_record_wr_file_ch::work()
{
    Common_Lock(m_lock);
    CV_ERR ret = m_cur_stm->work();
    update_ch_recording();
    Common_UnLock(m_lock);
    return ret;
}

CV_ERR cv_record_wr_file_ch::readQueue()
{
	if(m_bRecord)
	{
		Common_Lock(m_lock);
		
		PreRecord_Node *pNode = pQueueHead;
		if(pNode == NULL)
		{
			m_bRecord= FALSE;
			Common_UnLock(m_lock);
			return CV_ERR_REC_OPERATE_FAIL;
		}
		pQueueHead = (PreRecord_Node *)pNode->pNext;
#ifdef PRINT_TIMEVAL
		S8 szTime[32] = {0};
		Common_Time_T t_time;
		static U32 lstFrameNo = 0;
		CvFrameHeader *pHead = (CvFrameHeader *)((S8*)pNode+sizeof(PreRecord_Node));
		Common_Linux2CommonGmTime(pHead->uiFrameTime, &t_time);
		sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
		if(pHead->uiFrameType == CvPktIFrames ||pHead->uiFrameType == CvPktPFrames || pHead->uiFrameType == CvPktVIFrames)
		{	
			//if(pHead->uiFrameType == CvPktIFrames)
			{
				LOGW("[%d.%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d\n",pHead->uiFrameTime,pHead->uiFrameTickCount,szTime,pHead->uiFrameNo,pHead->uiFrameType,pHead->uiFrameLen);
			}
			if(lstFrameNo != 0 && (pHead->uiFrameNo - lstFrameNo != 1))
			{
				LOGE("uiFrameNo %d, lstFrameNo %d\n",pHead->uiFrameNo , lstFrameNo);
			}
			lstFrameNo = pHead->uiFrameNo;
		}
#endif
		ANTS_AviFile_DataInput(m_ch_idx, (S8 *)pNode+ sizeof(PreRecord_Node), pNode->framesize, NULL, 0, pNode->time_sec, pNode->time_usec);
		//ANTS_AviFile_DataInput(m_ch_idx, (S8 *)pHead, pHead->uiFrameLen, NULL, 0, pHead->uiFrameTime, pHead->uiFrameTickCount);
		PreRecordUseMem -= pNode->framesize;
		
		Common_Free((void*)pNode, __FUNCTION__, __LINE__);
		if(pQueueHead == NULL)
		{
			prestarttime.tv_sec = 0;
			prestarttime.tv_usec = 0;
			precurtime.tv_sec = 0;
			precurtime.tv_usec = 0;
			pQueueTail = NULL;
			bPreRecordNeedKey_Main = 1;
			bPreRecordNeedKey_Aux = 1;
			m_bRecord= FALSE;
#ifdef PRINT_TIMEVAL
			lstFrameNo = 0;
#endif
			Common_UnLock(m_lock);
			return CV_ERR_REC_OPERATE_FAIL;
		}
		prestarttime.tv_sec = pQueueHead->time_sec;
		prestarttime.tv_usec = pQueueHead->time_usec;
		Common_UnLock(m_lock);
	}
	else
	{
		return CV_ERR_REC_OPERATE_FAIL;
	}
	return CV_SUCCESS;
}

//停止工作，关闭文件，为格式化、关机做准备
CV_ERR cv_record_wr_file_ch::stop_work(U32 need_sync)
{
    Common_Lock(m_lock);
	m_cur_stm->stop_work(need_sync);
    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::start_record(U32 trig_type, U32 save_pre_rec)
{
    Common_Lock(m_lock);
    m_cur_stm->start_record(trig_type, save_pre_rec);
    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
CV_ERR cv_record_wr_file_ch::stop_record()
{
    Common_Lock(m_lock);
    m_cur_stm->stop_record();
	m_recordmask = 0;
	m_lstrecordmask = 0;
    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

//获取当前录像的类型
U32 cv_record_wr_file_ch::get_rec_trig_type()
{
    return m_cur_stm->get_rec_trig_type();
}

CV_ERR cv_record_wr_file_ch::set_pre_rec_mode(U32 mode)
{
    Common_Lock(m_lock);
    m_cur_stm->set_pre_rec_mode(mode);
    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::set_pre_rec_sec(U32 sec)
{
    Common_Lock(m_lock);
    m_cur_stm->set_pre_rec_sec(sec);
    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::set_pre_rec_mem(U32 mem)
{
    Common_Lock(m_lock);
    m_cur_stm->set_pre_rec_mem(mem);
    Common_UnLock(m_lock);
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::force_flush()
{
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_fetch_rec(U32 type, record_get_data_info *info)
{
    if (info->data_ok)
        return CV_SUCCESS;

    S32 ret = -1;

    if(m_ConnHandle[type] > 0)
    {
	    ret = cv_cfgm_ReadData(m_ConnHandle[type], NULL, (void **)&info->data_and_head, &info->data_len, 0);
	    if (ret == CV_SUCCESS)
	    {
//#ifdef PRINT_TIMEVAL
#if 1
		S8 szTime[32] = {0};
		Common_Time_T t_time;
		CvFrameHeader *pHead = (CvFrameHeader *)info->data_and_head;
		Common_Linux2CommonGmTime(pHead->uiFrameTime, &t_time);
		sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
		if(pHead->uiFrameType == CvPktIFrames || pHead->uiFrameType == CvPktPFrames || pHead->uiFrameType == CvPktVIFrames)
		{
			if(m_lstFrameNo != 0 && (pHead->uiFrameNo - m_lstFrameNo != 1))
			{
				LOGE("[ch%02d][handel:%d]uiFrameType %d uiFrameNo %d, lstFrameNo %d\n",m_ch_idx+1,m_ConnHandle[type],pHead->uiFrameType,pHead->uiFrameNo , m_lstFrameNo);
			}
			m_lstFrameNo = pHead->uiFrameNo;
			//LOGW("[%d.%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d\n",pHead->uiFrameTime,pHead->uiFrameTickCount,szTime,pHead->uiFrameNo,pHead->uiFrameType,pHead->uiFrameLen);
		}
#endif
	        info->data_ok = TRUE;
	    }
    }
	if (ret != 0)
		return CV_ERR_REC_OPERATE_FAIL;

    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_rel_rec(U32 type, record_get_data_info *info, U32 invalid_history)
{
    if (info->data_ok)
    {
		if(m_ConnHandle[type] > 0 && info->data_and_head != NULL)
	        cv_cfgm_ReleaseData(m_ConnHandle[type]);
        info->data_ok = FALSE;
        info->data_and_head = NULL;
		if(invalid_history)
			info->data_node = NULL;
    }

    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_fetch_smart()
{
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_rel_smart(U32 invalid_history)
{
    return CV_SUCCESS;
}

U32 cv_record_wr_file_ch::proc_is_fetch_fail_too_long(U32 th_value)
{
    time_t cur_time = time(NULL);

    //修改系统时间了，把时间同步为当前时间
    if ((U32) cur_time < (U32)m_fetch_ok_time)
    {
        m_fetch_ok_time = cur_time;
        return FALSE;
    }

    if (((U32) cur_time - (U32)m_fetch_ok_time) > th_value)
    {
        m_fetch_ok_time = cur_time;		//赋一下值，免得一直调用flush函数
        return TRUE;
    }

    return FALSE;
}

//force_new_data表示抛弃原来的，重新获取最新的数据
CV_ERR cv_record_wr_file_ch::proc_fetch_rec_data()
{
    U32 fetch_ok = FALSE;
#ifdef CHANGE_STREAM	
	static U32 req_iframe = 0;	
	if(0 != (m_recordmask & (0x1 << CV_ENC_STR_MAIN)) || m_recordmask == 0)
	{
		if(m_lstrecordmask == m_recordmask)
		{
			while(ops_fetch_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]) == CV_SUCCESS)
			{
				ops_rel_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB], FALSE);
			}
			
			if (ops_fetch_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]) == CV_SUCCESS)
		    {
		        fetch_ok = TRUE;
		    }
		}
		else
		{
			if(!req_iframe)
			{
				while(ops_fetch_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]) == CV_SUCCESS)
				{
					ops_rel_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN], FALSE);
				}
			
				cv_cfgm_RequestVideoIFrame(CV_ENC_STR_MAIN);
				req_iframe = 1;
			}
			
			if (ops_fetch_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]) == CV_SUCCESS)
		    {
				if(m_lstrecordmask == 0)
				{
					CvFrameHeader *main_frame_head = (CvFrameHeader *)(void *)m_data[CV_ENC_STR_MAIN].data_and_head;
					if(main_frame_head && main_frame_head->uiFrameType == CvPktIFrames)
					{
						m_lstrecordmask = m_recordmask;
						req_iframe = 0;
						fetch_ok = TRUE;
					}
					else if(main_frame_head && main_frame_head->uiFrameType != CvPktIFrames)
					{
						ops_rel_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN], FALSE);
						fetch_ok = FALSE;
					}
				}
				else if((m_lstrecordmask & (0x1 << CV_ENC_STR_SUB)))
				{
					CvFrameHeader *main_frame_head = (CvFrameHeader *)(void *)m_data[CV_ENC_STR_MAIN].data_and_head;
					if(main_frame_head && main_frame_head->uiFrameType == CvPktIFrames)
					{
						m_lstrecordmask = m_recordmask;
						req_iframe = 0;
						fetch_ok = TRUE;
					}
					else if(main_frame_head && main_frame_head->uiFrameType != CvPktIFrames)
					{
						ops_rel_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN], FALSE);
						fetch_ok = FALSE;
						if (ops_fetch_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]) == CV_SUCCESS)
						{
							fetch_ok = TRUE;
						}
					}
				}
		    }
		}
	}
	if(0 != (m_recordmask & (0x1 << CV_ENC_STR_SUB)))
	{
		if(m_lstrecordmask == m_recordmask)
		{
			while(ops_fetch_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]) == CV_SUCCESS)
			{
				ops_rel_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN], FALSE);
			}
			if (ops_fetch_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]) == CV_SUCCESS)
		    {
		        fetch_ok = TRUE;
		    }
		}
		else
		{
			if(!req_iframe)
			{
				while(ops_fetch_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]) == CV_SUCCESS)
				{
					ops_rel_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB], FALSE);
				}
				cv_cfgm_RequestVideoIFrame(CV_ENC_STR_SUB);
				req_iframe = 1;
			}

			if (ops_fetch_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]) == CV_SUCCESS)
		    {
				if(m_lstrecordmask == 0)
				{
					CvFrameHeader *sub_frame_head = (CvFrameHeader *)(void *)m_data[CV_ENC_STR_SUB].data_and_head;
					if(sub_frame_head && sub_frame_head->uiFrameType == CvPktSubIFrames)
					{
						m_lstrecordmask = m_recordmask;
						req_iframe = 0;
						fetch_ok = TRUE;
					}
					else if(sub_frame_head && sub_frame_head->uiFrameType != CvPktSubIFrames)
					{
						ops_rel_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB], FALSE);
						fetch_ok = FALSE;
					}
				}
				else if((m_lstrecordmask & (0x1 << CV_ENC_STR_MAIN)))
				{
					CvFrameHeader *sub_frame_head = (CvFrameHeader *)(void *)m_data[CV_ENC_STR_SUB].data_and_head;
					if(sub_frame_head && sub_frame_head->uiFrameType == CvPktSubIFrames)
					{
						m_lstrecordmask = m_recordmask;
						req_iframe = 0;
						fetch_ok = TRUE;
					}
					else if(sub_frame_head && sub_frame_head->uiFrameType != CvPktSubIFrames)
					{
						ops_rel_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB], FALSE);
						fetch_ok = FALSE;
						if (ops_fetch_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]) == CV_SUCCESS)
						{
							fetch_ok = TRUE;
						}
					}
				}
		    }
		}
	}
#else
	if(0 != (m_recordmask & (0x1 << CV_ENC_STR_MAIN)) || m_recordmask == 0)
	{
	    if (ops_fetch_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]) == CV_SUCCESS)
	        fetch_ok = TRUE;
	}
	if(0 != (m_recordmask & (0x1 << CV_ENC_STR_SUB)))
	{
	    if (ops_fetch_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]) == CV_SUCCESS)
	        fetch_ok = TRUE;
	}
#endif
	if(0 != (m_recordmask & (0x1 << CV_ENC_STR_AUDIO)) || m_recordmask == 0)
	{
	    if (ops_fetch_rec(CV_ENC_STR_AUDIO, &m_data[CV_ENC_STR_AUDIO]) == CV_SUCCESS)
	        fetch_ok = TRUE;
	}
	if(0 != (m_recordmask & (0x1 << CV_ENC_STR_SMART)) || m_recordmask == 0)
	{
	    if (ops_fetch_rec(CV_ENC_STR_SMART, &m_data[CV_ENC_STR_SMART]) == CV_SUCCESS)
	        fetch_ok = TRUE;
	}
    return fetch_ok ? CV_SUCCESS : CV_ERR_REC_OPERATE_FAIL;
}


//invalid_history表示忘掉历史数据，重新获取最新的I帧
CV_ERR cv_record_wr_file_ch::proc_rel_rec_data(U32 invalid_history)
{
	S32 ret = CV_SUCCESS;

	while(ret == CV_SUCCESS)
	{
		ret = ops_free_stream();
	}
    ops_rel_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN], invalid_history);
    ops_rel_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB], invalid_history);
    ops_rel_rec(CV_ENC_STR_AUDIO, &m_data[CV_ENC_STR_AUDIO], invalid_history);
    ops_rel_rec(CV_ENC_STR_SMART, &m_data[CV_ENC_STR_SMART], invalid_history);

    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_open_file()
{
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_close_file(U32 close_sync, U32 need_sync)
{
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_switch_file(U32 close_sync)
{
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_open_pre_rec_file()
{
	return CV_SUCCESS;
}

//move_next标示移动到下一个预录文件夹
CV_ERR cv_record_wr_file_ch::proc_close_pre_rec_file(U32 move_next, U32 close_sync, U32 need_sync)
{
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_del_pre_rec_file()
{
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_modify_pre_rec_file()
{
	do
	{
		PreRecord_Node *pNode = pQueueHead;
		if(pNode == NULL)
		{
			return CV_ERR_REC_OPERATE_FAIL;
		}
#ifdef PRINT_TIMEVAL
		S8 szTime[32] = {0};
		Common_Time_T t_time;
		CvFrameHeader *pHead = (CvFrameHeader *)((S8*)pNode+sizeof(PreRecord_Node));
		Common_Linux2CommonGmTime(pHead->uiFrameTime, &t_time);
		sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
		if(pHead->uiFrameType == CvPktIFrames ||pHead->uiFrameType == CvPktPFrames || pHead->uiFrameType == CvPktVIFrames)
			LOGW("uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d,[%d,%d]\n",szTime,pHead->uiFrameNo,pHead->uiFrameType,pHead->uiFrameLen,pHead->uiFrameTime,pHead->uiFrameTickCount);
#endif
		ANTS_AviFile_DataInput(m_ch_idx, (S8 *)pNode+ sizeof(PreRecord_Node), pNode->framesize, NULL, 0, pNode->time_sec, pNode->time_usec);
		//ANTS_AviFile_DataInput(m_ch_idx, (S8 *)pHead, pHead->uiFrameLen, NULL, 0, pHead->uiFrameTime, pHead->uiFrameTickCount);
		pQueueHead = (PreRecord_Node *)pQueueHead->pNext;
		PreRecordUseMem -= pNode->framesize;
		Common_Free((void*)pNode, __FUNCTION__, __LINE__);
		if(pQueueHead == NULL)
		{
			prestarttime.tv_sec = 0;
			prestarttime.tv_usec = 0;
			precurtime.tv_sec = 0;
			precurtime.tv_usec = 0;
			pQueueTail = NULL;
			bPreRecordNeedKey_Main = 1;
			bPreRecordNeedKey_Aux = 1;
			break;
		}
		prestarttime.tv_sec = pQueueHead->time_sec;
		prestarttime.tv_usec = pQueueHead->time_usec;
	}while(1);
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_switch_pre_rec_file(U32 close_sync)
{
	return CV_SUCCESS;	
}

CV_ERR cv_record_wr_file_ch::ops_input_stream(char *pFrameHead, int nHeadLen, char *pFrameData, int nDataLen, U32 dwCurTimeSec, U32 dwCurTimeUsec)
{
	CvFrameHeader *pHead = (CvFrameHeader *)pFrameHead;

	if(bPreRecordNeedKey_Main)
	{
		if(pHead->uiFrameType != CvPktIFrames)
		{
			return CV_ERR_REC_BASE;
		}
		bPreRecordNeedKey_Main = 0;
	}
	else if(bPreRecordNeedKey_Aux)
	{
		if(pHead->uiFrameType == CvPktSubIFrames)
		{
			bPreRecordNeedKey_Aux = 0;
		}
		else if(pHead->uiFrameType == CvPktSubPFrames)
		{
			return CV_ERR_REC_BASE;
		}
	}
	if(pQueueHead == NULL)
	{
		if(pHead->uiFrameType != CvPktIFrames)
		{
			bPreRecordNeedKey_Main = 1;
			bPreRecordNeedKey_Aux = 1;
			return CV_ERR_REC_BASE;
		}
	}
	S8 *pBuffer = NULL;
	while(pBuffer == NULL)
	{
		pBuffer = (S8 *)Common_Malloc(nHeadLen + sizeof(PreRecord_Node), 0, __FUNCTION__, __LINE__);
		if(pBuffer == NULL)
		{
			if(ops_free_stream() != CV_SUCCESS)
			{
				if(pHead->uiFrameType == CvPktIFrames)
				{
					bPreRecordNeedKey_Main = 0;
					bPreRecordNeedKey_Aux = 0;
				}
				else
				{
					return CV_ERR_REC_MALLOC_FAIL;
				}
			}
		}
	}
	PreRecord_Node *pNode = (PreRecord_Node *)pBuffer;
	pNode->pNext = NULL;
	pNode->frametype = pHead->uiFrameType;
	pNode->framesize = nHeadLen;
	pNode->time_sec = pHead->uiFrameTime;
	pNode->time_usec = pHead->uiFrameTickCount;
	if(pQueueHead == NULL)
	{
		pQueueHead = pNode;
		pQueueTail = pNode;
		prestarttime.tv_sec = pNode->time_sec;
		prestarttime.tv_usec = pNode->time_usec;
		precurtime = prestarttime;
	}
	else
	{
		pQueueTail->pNext = (void*)pNode;
		pQueueTail = pNode;
		precurtime.tv_sec = pNode->time_sec;
		precurtime.tv_usec = pNode->time_usec;
	}
	memcpy(pBuffer + sizeof(PreRecord_Node), pFrameHead, nHeadLen);
	if(1 == m_pre_rec_mode)
	{
		U32 prerecordtime = 0;
		do
		{
			prerecordtime = Common_cnt_delta_ms(&precurtime, &prestarttime) / 1000;
			if(prerecordtime > m_pre_rec_sec)
			{
	 			ops_free_stream();
			}
		}while(prerecordtime > m_pre_rec_sec);
	}
	PreRecordUseMem += nHeadLen;
	while(PreRecordUseMem > PreRecordMem)
	{
 		ops_free_stream();
	}

	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_free_stream()
{
	do
	{
		PreRecord_Node *pNode = pQueueHead;
		if(pNode == NULL)
		{
			return CV_ERR_REC_OPERATE_FAIL;
		}

		pQueueHead = (PreRecord_Node *)pQueueHead->pNext;
		PreRecordUseMem -= pNode->framesize;
		{
#if 0
			S8 szTime[32] = {0};
			Common_Time_T t_time;
			CvFrameHeader *pHead = (CvFrameHeader *)((S8*)pNode+sizeof(PreRecord_Node));
			Common_Linux2CommonTime(pHead->uiFrameTime, &t_time);
			sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
			if(pHead->uiFrameType == CvPktIFrames ||pHead->uiFrameType == CvPktPFrames)
			{	
				LOGE("[%d.%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d\n",pHead->uiFrameTime,pHead->uiFrameTickCount,szTime,pHead->uiFrameNo,pHead->uiFrameType,pHead->uiFrameLen);
			}
#endif	
		}
		Common_Free((void*)pNode, __FUNCTION__, __LINE__);

		if(pQueueHead == NULL)
		{
			prestarttime.tv_sec = 0;
			prestarttime.tv_usec = 0;
			precurtime.tv_sec = 0;
			precurtime.tv_usec = 0;
			pQueueTail = NULL;
			bPreRecordNeedKey_Main = 1;
			bPreRecordNeedKey_Aux = 1;
			return CV_ERR_REC_OPERATE_FAIL;
		}

		prestarttime.tv_sec = pQueueHead->time_sec;
		prestarttime.tv_usec = pQueueHead->time_usec;
	}while(0/*pQueueHead->frametype != CvPktIFrames*/);
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_write_stream(U32 type, record_get_data_info *info)
{
    if (!info->data_ok)
    {
        return CV_ERR_REC_OPERATE_FAIL;
    }

	struct timeval curtime;
	gettimeofday(&curtime, NULL);
    S32 ret = ops_input_stream(info->data_and_head, info->data_len, NULL, 0, curtime.tv_sec, curtime.tv_usec);
    if (ret != avis_ret_ok)
    {
//        LOGE("ch_%d drop it! ret = %d\n", m_ch_idx, ret);
    }
	ops_rel_rec(type, info, FALSE);
	info->data_ok = FALSE;
	info->data_and_head = NULL;
    if (ret != avis_ret_ok)
		return CV_ERR_REC_OPERATE_FAIL;

    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_write_rec(U32 type, record_get_data_info *info, U32 *is_wait)
{
    *is_wait = FALSE;
    if (!info->data_ok)
    {
        return CV_ERR_REC_OPERATE_FAIL;
    }

	struct timeval curtime;
#ifdef PRINT_TIMEVAL
	S8 szTime[32] = {0};
	static int lstFrameNo = 0;
	Common_Time_T t_time;
	CvFrameHeader *pHead = (CvFrameHeader *)info->data_and_head;
	Common_Linux2CommonGmTime(pHead->uiFrameTime, &t_time);
	sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
	if(pHead->uiFrameType == CvPktIFrames || pHead->uiFrameType == CvPktPFrames || pHead->uiFrameType == CvPktVIFrames)
	{
		//if(pHead->uiFrameType == CvPktIFrames)
			LOGW("[%d.%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d\n",pHead->uiFrameTime,pHead->uiFrameTickCount,szTime,pHead->uiFrameNo,pHead->uiFrameType,pHead->uiFrameLen);
		if(lstFrameNo != 0 && (pHead->uiFrameNo - lstFrameNo != 1))
		{
			LOGE("uiFrameNo %d, lstFrameNo %d\n",pHead->uiFrameNo , lstFrameNo);
		}
		lstFrameNo = pHead->uiFrameNo;
	}
#endif
	gettimeofday(&curtime, NULL);
	S32 ret = ANTS_AviFile_DataInput(m_ch_idx, info->data_and_head, info->data_len, NULL, 0, curtime.tv_sec, curtime.tv_usec);
	if (ret != avis_ret_ok)
	{
	//        LOGE("ch_%d drop it! ret = %d\n", m_ch_idx, ret);
	}
	ops_rel_rec(type, info, FALSE);
	info->data_ok = FALSE;
	info->data_and_head = NULL;
    if (ret != avis_ret_ok)
		return CV_ERR_REC_OPERATE_FAIL;

    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::ops_write_smart()
{
    return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::proc_save_to_avss(U32 bPreRecord)
{
	//如果类型发生变化，主码流I帧或子码流I帧时切换录像类型
	if (m_avss_trig_type != m_usr_set_trig_type)
	{
		if(0 != (m_recordmask & (0x1 << CV_ENC_STR_MAIN)))
		{
			CvFrameHeader *main_frame_head = (CvFrameHeader *)(void *)m_data[CV_ENC_STR_MAIN].data_and_head;
			if (m_data[CV_ENC_STR_MAIN].data_ok)
			{
				if(main_frame_head != NULL)
				{
					if (main_frame_head->uiFrameType == CvPktIFrames)
					{
						//if(m_avss_trig_type != CV_RECORD_BEGIN)
						{	
							LOGW("RecStop!. m_avss_trig_type=[%d]\n",m_avss_trig_type);
							ANTS_AviFile_RecStop(m_ch_idx);
						}
						if(m_usr_set_trig_type != CV_RECORD_BEGIN)
						{
							LOGW("RecStart!. m_usr_set_trig_type=[%d]\n",m_usr_set_trig_type);
							if(avis_ret_ok == ANTS_AviFile_RecStart(m_ch_idx, m_usr_set_trig_type))
							{
								m_avss_trig_type = m_usr_set_trig_type;
							}
						}
						else
						{
							m_avss_trig_type = m_usr_set_trig_type;
						}
					}
				}
			}
		}
		if(0 != (m_recordmask & (0x1 << CV_ENC_STR_SUB)))
		{
			CvFrameHeader *sub_frame_head = (CvFrameHeader *)(void *)m_data[CV_ENC_STR_SUB].data_and_head;
			if (m_data[CV_ENC_STR_SUB].data_ok)
			{
				if(sub_frame_head != NULL)
				{
					if (sub_frame_head->uiFrameType == CvPktSubIFrames)
					{
						//if(m_avss_trig_type != CV_RECORD_BEGIN)
						{	
							LOGW("RecStop!. m_avss_trig_type=[%d]\n",m_avss_trig_type);
							ANTS_AviFile_RecStop(m_ch_idx);
						}
						if(m_usr_set_trig_type != CV_RECORD_BEGIN)
						{
							LOGW("RecStart!. m_usr_set_trig_type=[%d]\n",m_usr_set_trig_type);
							if(avis_ret_ok == ANTS_AviFile_RecStart(m_ch_idx, m_usr_set_trig_type))
							{
								m_avss_trig_type = m_usr_set_trig_type;
							}
						}
						else
						{
							m_avss_trig_type = m_usr_set_trig_type;
						}
					}
				}
			}
		}
	}
	if(bPreRecord)
	{
		ops_write_stream(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]);
		ops_write_stream(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]);
		ops_write_stream(CV_ENC_STR_AUDIO, &m_data[CV_ENC_STR_AUDIO]);
		ops_write_stream(CV_ENC_STR_SMART, &m_data[CV_ENC_STR_SMART]);
		return CV_SUCCESS;
	}
	else
	{
		//队列不为空，继续往队列写数据，直到队列为空为止(读队列在另一个线程里执行)
		if(pQueueHead)		
		{
			ops_write_stream(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN]);
			ops_write_stream(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB]);
			ops_write_stream(CV_ENC_STR_AUDIO, &m_data[CV_ENC_STR_AUDIO]);
			ops_write_stream(CV_ENC_STR_SMART, &m_data[CV_ENC_STR_SMART]);
			return CV_SUCCESS;
		}
	}
	CV_ERR ret = CV_ERR_REC_OPERATE_FAIL;
	U32 main_wait = FALSE;
	U32 sub_wait = FALSE;
	U32 audio_wait = FALSE;
	//LOGW("step 1:%d %d\n",m_avss_trig_type,m_usr_set_trig_type);
	if (ops_write_rec(CV_ENC_STR_MAIN, &m_data[CV_ENC_STR_MAIN], &main_wait) == CV_SUCCESS)
	    ret = CV_SUCCESS;

	if (ops_write_rec(CV_ENC_STR_SUB, &m_data[CV_ENC_STR_SUB], &sub_wait) == CV_SUCCESS)
	    ret = CV_SUCCESS;

	if (ops_write_rec(CV_ENC_STR_AUDIO, &m_data[CV_ENC_STR_AUDIO], &audio_wait) == CV_SUCCESS)
	    ret = CV_SUCCESS;
	if (ops_write_rec(CV_ENC_STR_SMART, &m_data[CV_ENC_STR_SMART], &audio_wait) == CV_SUCCESS)
	    ret = CV_SUCCESS;
	return ret;
}

//将avss中缓存的信息刷到硬盘，在停止录像或者长时间没有数据的时候调用
CV_ERR cv_record_wr_file_ch::proc_flush_avss()
{
	return CV_SUCCESS;
}

//检查是否需要切换文件
U32 cv_record_wr_file_ch::proc_is_need_switch_file(U32 file_time_len, U32 &close_sync, U32 &dir_lost)
{
	return FALSE;
}

//检查切文件时机是否成熟
U32 cv_record_wr_file_ch::proc_is_ok_for_switch_file(U32 dir_lost)
{
	return FALSE;
}

void cv_record_wr_file_ch::update_ch_recording()
{
    if(m_record_status == CV_RECORD_STATUS_NORMAL)
    {
        m_stop_cnt = 0;
    }
    else
    {
        m_stop_cnt ++;
    }
}

U32 cv_record_wr_file_ch::is_recording()
{
    //如果是停止录像1秒钟才视为不录像
    if(m_stop_cnt < 25 && m_avss_trig_type!= 0)
    {
        return TRUE;       
    }
    
    //过滤掉预录
    return (m_record_status == CV_RECORD_STATUS_NORMAL && m_avss_trig_type!= CV_RECORD_BEGIN) ? TRUE: FALSE;
}

CV_ERR cv_record_wr_file_ch::checkstreamhandle(U32 type, U32* pcheck)
{
	if(type >= CV_ENC_STR_MAX)
	{
        LOGE("type = %d!\n", type);
        return CV_ERR_REC_INVALID_PARA;
	}
	*pcheck = m_ConnHandle[type] > 0 ? TRUE : FALSE;
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::setStreamHandel(U32 type, S32 ConnHandle)
{
	if(type >= CV_ENC_STR_MAX)
	{
        LOGE("type = %d!\n", type);
        return CV_ERR_REC_INVALID_PARA;
	}
	m_ConnHandle[type] = ConnHandle;
	m_bChangeHandle[type] = TRUE;
	return CV_SUCCESS;
}

CV_ERR cv_record_wr_file_ch::setMask(U32 mask)
{
	m_recordmask = mask;
	return CV_SUCCESS;
}

void cv_record_wr_file_ch::proc_update_disk_record_status(U32 recording)
{
}

CV_ERR cv_record_wr_file_ch::invalid_avss_writer()
{
	return CV_SUCCESS;
}
//----------------------------------- 录像线程 ----------------------------------------------
static S32 record_wr_file_thread(Common_Thread_T pThHandle, void* para)
{
    LOGD("#################### thread PID = %d enter\n", getpid());
    cv_rec_wr_file_thread_ctl *thr_ctl = (cv_rec_wr_file_thread_ctl *)para;
    LOGD("start_ch = %d, ch_num = %d!\n", thr_ctl->start_ch, thr_ctl->ch_num);
#if 0
    U64 u64CurMs = 0,u64LstMs = 0;
    u64CurMs = Common_GetSystemCount64();
    u64LstMs = u64CurMs;
    struct sched_param sparam;
    int iFIFO = SCHED_FIFO,iRR = SCHED_RR;
    memset(&sparam,0,sizeof(sparam));
    //sparam.sched_priority = sched_get_priority_max(iFIFO);
    sparam.sched_priority = sched_get_priority_max(iRR);
    if(pthread_setschedparam(pthread_self(),iRR,&sparam) != 0)
    {
	LOGE("pthread_setschedparam failed\n");
    }
#endif	
    while (1)
    {
#if 0
    	u64CurMs = Common_GetSystemCount64();
    	if(u64CurMs-u64LstMs > 1000)
    	{
    		LOGE("cost %llums\n",u64CurMs-u64LstMs);
	}
	u64LstMs = u64CurMs;
#endif
        U32 work_ok = thr_ctl->wr_file_manage->work(thr_ctl->start_ch, thr_ctl->ch_num);

        if (!work_ok)
        {
			Common_Sleep(0, 100000);
        }	
    }

    LOGD("#################### thread PID = %d exit\n", getpid());
    return 0;
}

//----------------------------------- 录像线程 ----------------------------------------------
static S32 record_read_queue_thread(Common_Thread_T pThHandle, void* para)
{
    LOGD("#################### thread PID = %d enter\n", getpid());
	
    cv_rec_wr_file_thread_ctl *thr_ctl = (cv_rec_wr_file_thread_ctl *)para;
    LOGD("start_ch = %d, ch_num = %d!\n", thr_ctl->start_ch, thr_ctl->ch_num);
    U64 u64CurMs = 0,u64LstMs = 0;

    u64CurMs = Common_GetSystemCount64();
    u64LstMs = u64CurMs;
    while (1)
    {
    	u64CurMs = Common_GetSystemCount64();
    	if(u64CurMs-u64LstMs > 500)
    	{
    		LOGW("cost %llums\n",u64CurMs-u64LstMs);
		}
		u64LstMs = u64CurMs;
        U32 bOk = thr_ctl->wr_file_manage->readQueue(thr_ctl->start_ch, thr_ctl->ch_num);

        if (!bOk)
        {
			Common_Sleep(0, 40000);
        }
    }

    LOGD("#################### thread PID = %d exit\n", getpid());
    return 0;
}

//-------------------------------------------- cv_record_wr_file_manage ----------------------------------------------
cv_record_wr_file_manage::cv_record_wr_file_manage()
{
    const cv_device_capability *sys_cap = cv_cfgm_get_dev_cap();
    m_ch_num = sys_cap->video_ch_num;

    for (U32 i = 0; i < m_ch_num; i++)
    {
        m_ch[i] = new cv_record_wr_file_ch(i);
        if(m_ch[i] == NULL)
        {
			LOGE("cv_record_wr_file_ch malloc err\n");
			m_ch_num = i;
			break;
        }
    }

    //一个线程处理16个通道
    m_rec_thr_num = (m_ch_num + 15) / 16;	
    if (m_rec_thr_num > CV_ARRAY_DIM(m_rec_thr))
        m_rec_thr_num = CV_ARRAY_DIM(m_rec_thr);	

    //创建线程
    CV_CLR_ARG(m_rec_thr);
    for (U32 i = 0; i < m_rec_thr_num; i++)
    {
        m_rec_thr[i].wr_file_manage = this;
        m_rec_thr[i].thread_idx = i;
        m_rec_thr[i].start_ch = i * (m_ch_num / m_rec_thr_num);
        m_rec_thr[i].ch_num = m_ch_num / m_rec_thr_num;
        if (i == (m_rec_thr_num - 1))
            m_rec_thr[i].ch_num += (m_ch_num % m_rec_thr_num);

		Common_Thread_T ThreadHandle = NULL;
		Common_Thread_Create(&ThreadHandle, "record_wr_file_thread", 1024*256, COMMON_THREAD_CREATEFLAG_NORMAL, record_wr_file_thread, &m_rec_thr[i]);

		Common_Thread_T ThreadRdHandle = NULL;
		Common_Thread_Create(&ThreadRdHandle, "record_read_queue_thread", 1024*256, COMMON_THREAD_CREATEFLAG_NORMAL, record_read_queue_thread, &m_rec_thr[i]);
    }
}

cv_record_wr_file_manage::~cv_record_wr_file_manage()
{
}

//停止工作，关闭文件，为格式化、关机做准备
CV_ERR cv_record_wr_file_manage::stop_work(U32 ch_idx, U32 need_sync)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[ch_idx]->stop_work(need_sync);
}

//save_pre_rec描述是否保留预录文件
CV_ERR cv_record_wr_file_manage::start_record(U32 ch_idx, U32 trig_type, U32 save_pre_rec, U32 nSchedMode)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }
	/*
	if(trig_type > CV_REC_TRIG_NONE)
	{
		if((trig_type == CV_REC_TRIG_BY_REGULAR || trig_type == CV_REC_TRIG_BY_MANUAL) && (nSchedMode == 3))
		{
			cv_cfgm_change_record_encode(1);
		}
		else
		{
			cv_cfgm_change_record_encode(0);
		}
	}
	*/
    return m_ch[ch_idx]->start_record(trig_type, save_pre_rec);
}

//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
CV_ERR cv_record_wr_file_manage::stop_record(U32 ch_idx)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }
	/*
	cv_cfgm_change_record_encode(0);
	*/
    return m_ch[ch_idx]->stop_record();
}

//save_pre_rec描述是否保留预录文件
CV_ERR cv_record_wr_file_manage::set_mask(U32 ch_idx, U32 mask)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[ch_idx]->setMask(mask);
}

//获取当前录像的类型
U32 cv_record_wr_file_manage::get_rec_trig_type(U32 ch_idx)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_REC_TRIG_NONE;
    }

    return m_ch[ch_idx]->get_rec_trig_type();
}

CV_ERR cv_record_wr_file_manage::set_pre_rec_mode(U32 ch_idx, U32 mode)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[ch_idx]->set_pre_rec_mode(mode);
}

CV_ERR cv_record_wr_file_manage::set_pre_rec_sec(U32 ch_idx, U32 sec)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[ch_idx]->set_pre_rec_sec(sec);
}

CV_ERR cv_record_wr_file_manage::set_pre_rec_mem(U32 ch_idx, U32 mem)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[ch_idx]->set_pre_rec_mem(mem);
}

U32 cv_record_wr_file_manage::work(U32 start_ch, U32 ch_num)
{
    U32 work_ok = FALSE;
    for (U32 i = start_ch; i < start_ch + ch_num; i++)
    {
        if (m_ch[i]->work() == CV_SUCCESS)
            work_ok = TRUE;
    }

    return work_ok;
}

U32 cv_record_wr_file_manage::readQueue(U32 start_ch, U32 ch_num)
{
    U32 work_ok = FALSE;
    for (U32 i = start_ch; i < start_ch + ch_num; i++)
    {
        if (m_ch[i]->readQueue() == CV_SUCCESS)
            work_ok = TRUE;
    }

    return work_ok;
}

U32 cv_record_wr_file_manage::is_recording(U32 nvr_ch)
{
    if (!is_valid_ch(nvr_ch))
    {
        LOGE("ch_idx = %d!\n", nvr_ch);
        return FALSE;
    }

    return m_ch[nvr_ch]->is_recording();
}

CV_ERR cv_record_wr_file_manage::force_flush(U32 ch_idx)
{
    if (!is_valid_ch(ch_idx))
    {
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
    }

    return m_ch[ch_idx]->force_flush();
}

inline U32 cv_record_wr_file_manage::is_valid_ch(U32 nvr_ch)
{
    if ((nvr_ch >= 0) && (nvr_ch < m_ch_num))
        return TRUE;

    return FALSE;
}

CV_ERR cv_record_wr_file_manage::checkstreamhandle(U32 ch_idx, U32 type, U32* pcheck)
{
	if(!is_valid_ch(ch_idx))
	{
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
	}
	return m_ch[ch_idx]->checkstreamhandle(type, pcheck);
}

CV_ERR cv_record_wr_file_manage::setStreamHandel(U32 ch_idx, U32 type, S32 Handle)
{
	if(!is_valid_ch(ch_idx))
	{
        LOGE("ch_idx = %d!\n", ch_idx);
        return CV_ERR_REC_INVALID_PARA;
	}
	if(Handle <= 0)
		return CV_ERR_REC_OPERATE_FAIL;
	m_handle[ch_idx] = Handle;
	return m_ch[ch_idx]->setStreamHandel(type, Handle);
}

