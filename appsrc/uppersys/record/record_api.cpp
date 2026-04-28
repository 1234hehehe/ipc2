#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "cfg_manage_api.h"

#include "record_api.h"
#include "record_resource.h"
#include "disk_manage_api.h"

namespace cv_record{
static U32 s_api_init = FALSE;
static pthread_mutex_t s_api_init_lock = PTHREAD_MUTEX_INITIALIZER;
}//namespace cv_record{

using namespace cv_soft;
using namespace cv_record;

static CV_ERR cv_record_load_config()
{
    if(!s_api_init)
    {
        LOGE("Record Module Not Init\n");
        return CV_ERR_REC_NOT_INIT;
    }
	
    const cv_device_capability *sys_cap = cv_cfgm_get_dev_cap();
    for(U32 i = 0; i < sys_cap->video_ch_num; i++)
    {
        U32 mode = STOP_REC;
        if (CV_SUCCESS == cv_cfgm_get_record_mode(i, &mode))
            cv_record_set_record_mode(i, mode);
        U32 mask = 0;
        if (CV_SUCCESS == cv_cfgm_get_manual_mask(i, &mask))
            cv_record_set_record_manualmask(i, mask);

        cv_local_rec_plan plan;
        CV_CLR_ARG(plan);
        if(CV_SUCCESS == cv_cfgm_get_local_rec_plan(i, &plan))
            cv_record_set_rec_plan(i, &plan);

        mode = 0;
        if(CV_SUCCESS == cv_cfgm_get_pre_rec_mode(i, &mode))
            cv_record_set_pre_rec_mode(i, mode);

        U32 sec = 0;
        if(CV_SUCCESS == cv_cfgm_get_pre_rec_sec(i, &sec))
        {
        	RECORD_FUNCTION sRecFunc = {0};

			cv_cfgm_get_record_func(&sRecFunc);
        	if(sRecFunc.PreRecord)
	        	cv_record_set_pre_rec_sec(i, sec);
			else
				cv_record_set_pre_rec_sec(i, 0);
		}

	 U32 mem = 0;
        if(CV_SUCCESS == cv_cfgm_get_pre_rec_mem(i, &mem))
            cv_record_set_pre_rec_mem(i,mem);

        if(CV_SUCCESS == cv_cfgm_get_delay_rec_sec(i, &sec))
            cv_record_set_delay_rec_sec(i, sec);
	}

	S32 Recycle;
	PRESERVERECORDRULE Rule;
	if(CV_SUCCESS == cv_cfgm_get_diskconfig(&Recycle, &Rule))
		cv_diskm_init(Recycle, Rule.PreserveMode, Rule.PreserveTime);

    return CV_SUCCESS;
}

static S32 record_getstream_thread(Common_Thread_T pThHandle, void* para)
{
    LOGD("#################### thread PID = %d enter\n", getpid());
    
    while (1)
    {
		const cv_device_capability *sys_cap = cv_cfgm_get_dev_cap();
		for(U32 i = 0; i < sys_cap->video_ch_num; i++)
		{
			for(U32 j = 0; j < sys_cap->video_stream_num[i]; j ++)
			{
				U32 bOK;
				if(CV_SUCCESS == get_res_manage()->get_wr_file_manage()->checkstreamhandle(i, j, &bOK))
				{
					if(!bOK)
					{
						U32 handle = cv_cfgm_get_streamhandle(i, j);
						if(handle)
						{
							get_res_manage()->get_wr_file_manage()->setStreamHandel(i, j, handle);
						}
					}
				}
			}
			if(i < sys_cap->audio_ch_num)
			{
				U32 bOK;
				if(CV_SUCCESS == get_res_manage()->get_wr_file_manage()->checkstreamhandle(i, CV_ENC_STR_AUDIO, &bOK))
				{
					if(!bOK)
					{
						U32 handle = cv_cfgm_get_streamhandle(i, CV_ENC_STR_AUDIO);
						if(handle)
						{
							get_res_manage()->get_wr_file_manage()->setStreamHandel(i, CV_ENC_STR_AUDIO, handle);
						}
					}
				}
			}
			if(0)
			{
				U32 bOK;
				if(CV_SUCCESS == get_res_manage()->get_wr_file_manage()->checkstreamhandle(i, CV_ENC_STR_SMART, &bOK))
				{
					if(!bOK)
					{
						U32 handle = cv_cfgm_get_streamhandle(i, CV_ENC_STR_SMART);
						if(handle)
						{
							get_res_manage()->get_wr_file_manage()->setStreamHandel(i, CV_ENC_STR_SMART, handle);
						}
					}
				}
			}
		}
        Common_Sleep(1, 0);
    }

    LOGD("#################### thread PID = %d exit\n", getpid());
    return 0;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_init(void* handle)
{
    if (s_api_init == FALSE)
    {
        pthread_mutex_lock(&s_api_init_lock);
        if (s_api_init == FALSE)
        {			
            //初始化依赖的模块
			LOGD("cv_record_init first!\n");
            cv_cfgm_init(handle);

            get_res_manage()->construct_res();

            s_api_init = TRUE;
            //导入参数
            cv_record_load_config();
			Common_Thread_T ThreadHandle = NULL;
			Common_Thread_Create(&ThreadHandle, "record_getstream_thread", 1024*256, COMMON_THREAD_CREATEFLAG_NORMAL, record_getstream_thread, NULL);
			LOGD("cv_record_init first ok!\n");
        }
        pthread_mutex_unlock(&s_api_init_lock);
    }
    return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_saveconfig()
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }
    cv_cfgm_saveconfig();
    return CV_SUCCESS;
}

//为格式化、关机、重启做准备
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_stop_work(U32 need_sync)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->stop_work(need_sync);
}

//格式化完毕之后恢复工作
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_resume_work()
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->start_work();
}

//-------------------------------------- 录像计划控制 -----------------------------------------------------

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_record_manual_enable(U32 nvr_ch, U32 enable)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_manual_enable(nvr_ch, enable);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_get_record_manual_enable(U32 nvr_ch, U32 * enable)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->get_manual_enable(nvr_ch, enable);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_get_record_mode(U32 nvr_ch, U32 *pmode, U32 *pmask, U32 *pshedmode)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }
    return get_res_manage()->get_shedule_manage()->get_record_mode(nvr_ch, pmode,pmask,pshedmode);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_get_shedule_status(U32 *pSheduleStatus)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }
    return get_res_manage()->get_shedule_manage()->get_shedule_status(pSheduleStatus);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_record_mode(U32 nvr_ch, U32 mode)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_record_mode(nvr_ch, mode);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_record_manualmask(U32 nvr_ch, U32 mask)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_record_manualmask(nvr_ch, mask);
}


FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_rec_plan(U32 nvr_ch, const cv_local_rec_plan *plan)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_rec_plan(nvr_ch, plan);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_rec_plan_day(U32 nvr_ch, U32 day, const cv_record_arming_day *plan)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_rec_plan_day(nvr_ch, day, plan);
}

//设置预录时间
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_pre_rec_mode(U32 nvr_ch, U32 mode)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_pre_rec_mode(nvr_ch, mode);
}

//设置预录时间
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_pre_rec_sec(U32 nvr_ch, U32 sec)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_pre_rec_sec(nvr_ch, sec);
}

//设置预录内存大小
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_pre_rec_mem(U32 nvr_ch, U32 mem)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_pre_rec_mem(nvr_ch, mem);
}

//设置延迟录像时间
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_delay_rec_sec(U32 nvr_ch, U32 sec)
{
    if (s_api_init == FALSE)
    {
        LOGE("call recfrw_init first!\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->set_delay_rec_sec(nvr_ch, sec);
}

//-------------------------------------- 录像联动控制 -----------------------------------------------------	
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_linkage(U32 rec_ch, const cv_event_id *event_id, U32 enable)
{
    if(!s_api_init)
    {
        LOGE("Record Moudle Not Init\n");
        return CV_ERR_REC_NOT_INIT;
    }

    if(event_id == NULL)
    {
        LOGE("NULL Pointer\n");
        return CV_ERR_REC_INVALID_PARA;
    }
    
    return get_res_manage()->get_shedule_manage()->set_linkage(rec_ch, event_id, enable);
}


//-------------------------------------- 录像状态获取 -----------------------------------------------------	
FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_get_ch_state(U32 rec_ch, U32 *pRecordType, U32 *pRecordMask, U32 *pShedType)
{
    if(!s_api_init)
    {
        LOGE("Record Moudle Not Init\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_shedule_manage()->get_record_mode(rec_ch, pRecordType, pRecordMask, pShedType);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_set_data_magic(U32 magic)
{
    if(!s_api_init)
    {
        LOGE("Record Moudle Not Init\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->set_data_magic(magic);
}

FOX_HELPER_DLL_EXPORT U32 cv_soft::cv_record_ch_is_recording(U32 nvr_ch)
{
    if(!s_api_init)
    {
        LOGE("Record Moudle Not Init\n");
        return FALSE;
    }
    
    return get_res_manage()->get_wr_file_manage()->is_recording(nvr_ch);
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_record_force_flush(U32 record_ch)
{
    if(!s_api_init)
    {
        LOGE("Record Module Not Init\n");
        return CV_ERR_REC_NOT_INIT;
    }

    return get_res_manage()->get_wr_file_manage()->force_flush(record_ch);
}
