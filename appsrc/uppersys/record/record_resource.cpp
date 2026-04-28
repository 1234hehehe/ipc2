#include <pthread.h>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>

#include "record_resource.h"

namespace cv_record{
static cv_record_res_manage *s_resource_manage = NULL;
static pthread_mutex_t s_resource_lock = PTHREAD_MUTEX_INITIALIZER;
}

using namespace cv_soft;
using namespace cv_record;

cv_record_res_manage* cv_record::get_res_manage()
{
    if (unlikely(s_resource_manage == NULL))
    {
        pthread_mutex_lock(&s_resource_lock);
        if (s_resource_manage == NULL)
        {
            s_resource_manage = new(cv_record_res_manage);
            if(s_resource_manage == NULL)
            {
				LOGE("new s_resource_manage err\n");
            }
        }
        pthread_mutex_unlock(&s_resource_lock);
    }
    
    return s_resource_manage;
}

//---------------------------------------------- cv_record_res_manage --------------------------------------
cv_record_res_manage::cv_record_res_manage()
{
    m_shedule_manage = NULL;
    m_wr_file_manage = NULL;
    m_data_magic = 0;
}

cv_record_res_manage::~cv_record_res_manage()
{
	//暂时不支持析构
}

void cv_record_res_manage::construct_res()
{
    m_wr_file_manage = new cv_record_wr_file_manage;
    if(m_wr_file_manage == NULL)
    {
		LOGE("new m_wr_file_manage failed\n");
    }

    m_shedule_manage = new cv_record_shedule_manage;
    if(m_shedule_manage == NULL)
    {
		LOGE("new m_shedule_manage failed\n");
    }
	return;
}

cv_record_shedule_manage* cv_record_res_manage::get_shedule_manage()
{
    return m_shedule_manage;
}

cv_record_wr_file_manage* cv_record_res_manage::get_wr_file_manage()
{
    return m_wr_file_manage;
}

U32 cv_record_res_manage::get_data_magic()
{
    return m_data_magic;
}

CV_ERR cv_record_res_manage::set_data_magic(U32 magic)
{
    m_data_magic = magic;
    return CV_SUCCESS;
}

