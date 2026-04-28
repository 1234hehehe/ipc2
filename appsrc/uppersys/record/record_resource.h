#ifndef _RECORD_RESOURCE_H_
#define _RECORD_RESOURCE_H_

#include <libcommon_api.h>
#include "record_common.h"

#include "record_wr_file.h"
#include "record_shedule.h"

using namespace cv_soft;

//管理utilitiy模块使用的所有对象资源
namespace cv_record
{

/***********************************************************************************
	cv_record_shedule_manage 负责录像计划的仲裁，例如，该录还是该停，属于决策层
	cv_record_wr_file_manage 负责文件的写入控制，例如获取数据啊、发送数据啊等等，属于机制层
***********************************************************************************/
class cv_record_res_manage
{
public:
    void construct_res();	//只允许初始化函数调用一次

    cv_record_shedule_manage* get_shedule_manage();
    cv_record_wr_file_manage* get_wr_file_manage();

    U32 get_data_magic();
    CV_ERR set_data_magic(U32 magic);

private:
    cv_record_shedule_manage *m_shedule_manage;
    cv_record_wr_file_manage *m_wr_file_manage;

private:
    U32 m_data_magic;
		
public:
    cv_record_res_manage();
    ~cv_record_res_manage();
    const char* class_name() {return "cv_record_res_manage";};
    
private:
    cv_record_res_manage(const cv_record_res_manage &other);
    cv_record_res_manage &operator=(const cv_record_res_manage &other);
};

cv_record_res_manage* get_res_manage();

} //namespace cv_record{
#endif //#ifndef _RECORD_RESOURCE_H_
