#ifndef _RECORD_WR_FILE_H_
#define _RECORD_WR_FILE_H_

#include "record_common.h"

using namespace cv_soft;

namespace cv_record
{

typedef enum
{
    CV_RECORD_STATUS_UNKNOW = 0x0,
    CV_RECORD_STATUS_FAILED_NO_VIDEO,
    CV_RECORD_STATUS_FAILED_MALLOC_DISK_SPACE,
    CV_RECORD_STATUS_FAILED_WRITE_DISK,

    CV_RECORD_STATUS_STOP,
    CV_RECORD_STATUS_PAUSE,
    CV_RECORD_STATUS_NORMAL,
}cv_record_status;

typedef struct
{
	U32	frametype;
	U32 framesize;
	U32 time_sec;
	U32 time_usec;
	void *pNext;
}PreRecord_Node;

class cv_record_wr_file_ch;
	
//用状态机来描述各种状态下的响应函数
class record_wr_file_stm
{
public:
    virtual CV_ERR work() = 0;
    virtual CV_ERR stop_work(U32 need_sync) = 0;//停止工作，关闭文件，为格式化、关机做准备

    virtual CV_ERR start_record(U32 trig_type, U32 save_pre_rec) = 0;	//save_pre_rec描述是否保留预录文件
    virtual CV_ERR stop_record() = 0;	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件

    virtual U32 get_rec_trig_type() = 0;	//获取当前录像的类型
    virtual CV_ERR set_pre_rec_mode(U32 mode) = 0;
    virtual CV_ERR set_pre_rec_sec(U32 sec) = 0;
    virtual CV_ERR set_pre_rec_mem(U32 mem) = 0;
	
public:
    virtual ~record_wr_file_stm() {};
};

//停止工作，关闭文件
class record_stop_work_stm : public record_wr_file_stm
{
public:
    CV_ERR work();
    CV_ERR stop_work(U32 need_sync);//停止工作，关闭文件，为格式化、关机做准备
    CV_ERR start_record(U32 trig_type, U32 save_pre_rec);	//save_pre_rec描述是否保留预录文件
    CV_ERR stop_record();	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
    U32 get_rec_trig_type();	//获取当前录像的类型
    CV_ERR set_pre_rec_mode(U32 mode);
    CV_ERR set_pre_rec_sec(U32 sec);
    CV_ERR set_pre_rec_mem(U32 mem);
		
private:
    cv_record_wr_file_ch *m_ch;
	
public:
    record_stop_work_stm(cv_record_wr_file_ch *ch);
    ~record_stop_work_stm();
    const char* class_name() {return "record_stop_work_stm";};
    
private:
    record_stop_work_stm(const record_stop_work_stm &other);
    record_stop_work_stm &operator=(const record_stop_work_stm &other);
};

//正常录像模式                 
class record_normal_rec_stm : public record_wr_file_stm
{
public:
    CV_ERR work();
    CV_ERR stop_work(U32 need_sync);//停止工作，关闭文件，为格式化、关机做准备
    CV_ERR start_record(U32 trig_type, U32 save_pre_rec);	//save_pre_rec描述是否保留预录文件
    CV_ERR stop_record();	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
    U32 get_rec_trig_type();	//获取当前录像的类型
    CV_ERR set_pre_rec_mode(U32 mode);
    CV_ERR set_pre_rec_sec(U32 sec);
    CV_ERR set_pre_rec_mem(U32 mem);
	
private:
    cv_record_wr_file_ch *m_ch;
	//上一次触发类型改变的时间,只在normal状态时有效

public:
    record_normal_rec_stm(cv_record_wr_file_ch *ch);
    ~record_normal_rec_stm();
    const char* class_name() {return "record_normal_rec_stm";};
    
private:
    record_normal_rec_stm(const record_normal_rec_stm &other);
    record_normal_rec_stm &operator=(const record_normal_rec_stm &other);
};

//停止录像，但是没有停止工作，为了减少文件数目，这时候并不会关闭文件，而是怠速等待唤醒，如果到了半小时还没有唤醒，就关闭文件
class record_pause_rec_stm : public record_wr_file_stm
{
public:
    CV_ERR work();
    CV_ERR stop_work(U32 need_sync);//停止工作，关闭文件，为格式化、关机做准备
    CV_ERR start_record(U32 trig_type, U32 save_pre_rec);	//save_pre_rec描述是否保留预录文件
    CV_ERR stop_record();	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
    U32 get_rec_trig_type();	//获取当前录像的类型
    CV_ERR set_pre_rec_mode(U32 mode);
    CV_ERR set_pre_rec_sec(U32 sec);
    CV_ERR set_pre_rec_mem(U32 mem);
	
private:
    cv_record_wr_file_ch *m_ch;

public:
    record_pause_rec_stm(cv_record_wr_file_ch *ch);
    ~record_pause_rec_stm();
    const char* class_name() {return "record_pause_rec_stm";};
    
private:
    record_pause_rec_stm(const record_pause_rec_stm &other);
    record_pause_rec_stm &operator=(const record_pause_rec_stm &other);
};


//预录模式
class record_pre_rec_stm : public record_wr_file_stm
{
public:
    CV_ERR work();
    CV_ERR stop_work(U32 need_sync);//停止工作，关闭文件，为格式化、关机做准备
    CV_ERR start_record(U32 trig_type, U32 save_pre_rec);	//save_pre_rec描述是否保留预录文件
    CV_ERR stop_record();	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
    U32 get_rec_trig_type();	//获取当前录像的类型
    CV_ERR set_pre_rec_mode(U32 mode);
    CV_ERR set_pre_rec_sec(U32 sec);
    CV_ERR set_pre_rec_mem(U32 mem);
	
private:
    cv_record_wr_file_ch *m_ch;

public:
    record_pre_rec_stm(cv_record_wr_file_ch *ch);
    ~record_pre_rec_stm();
    const char* class_name() {return "record_pre_rec_stm";};
    
private:
    record_pre_rec_stm(const record_pre_rec_stm &other);
    record_pre_rec_stm &operator=(const record_pre_rec_stm &other);
};

typedef struct
{
    U32 force_next_I;

    U32 data_ok;	//数据是否已经获取到了，没有释放
    S8 *data_and_head;
    void *data_node;
    S32  data_len;

    U32 is_history;  //是不是历史流，是不是不需要释放了                              
}record_get_data_info;

typedef struct
{
    U32 dir_is_valid;		//是否已经申请了有效的文件夹，如果没有，就要申请
    S8 dir_path[64];
    U32 part_magic;
    U32 dev_magic;

    U32 dir_is_used;	//文件夹是否已经被删除，预录的是时候用这个标记来标示是否被淘汰
}record_req_rec_dir_info;

class cv_record_wr_file_ch
{
//对外控制接口，全部委托给状态机来调用
public:
    CV_ERR work();
    CV_ERR readQueue();
    CV_ERR stop_work(U32 need_sync);//停止工作，关闭文件，为格式化、关机做准备	
    CV_ERR start_record(U32 trig_type, U32 save_pre_rec);	//save_pre_rec描述是否保留预录文件
    CV_ERR stop_record();	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件
    U32 get_rec_trig_type();	//获取当前录像的类型

    CV_ERR set_pre_rec_mode(U32 mode);
    CV_ERR set_pre_rec_sec(U32 sec);
    CV_ERR set_pre_rec_mem(U32 mem);
	
    void update_ch_recording();
    U32 is_recording();
    CV_ERR force_flush();
	CV_ERR checkstreamhandle(U32 type, U32* pcheck);
	CV_ERR setStreamHandel(U32 type, S32 ConnHandle);
	CV_ERR setMask(U32 mask);

    //把当前正在操作的文件夹的写入器给补录的通道
    CV_ERR invalid_avss_writer();
		
//状态机调用接口
public:
    void switch_to_stop_work_stm() {m_cur_stm = &m_stop_work_stm;  return;};
    void switch_to_normal_rec_stm() {m_cur_stm = &m_normal_rec_stm; return;};
    void switch_to_pause_rec_stm() {m_cur_stm = &m_pause_rec_stm; return;};
    void switch_to_pre_rec_stm() {m_cur_stm = &m_pre_rec_stm; return;};

    //申明状态机的友元对象，方便直接访问内部资源
    friend class record_stop_work_stm;
    friend class record_normal_rec_stm;
    friend class record_pause_rec_stm;
    friend class record_pre_rec_stm;

private:
    //-----------------------
    CV_ERR ops_fetch_smart();
    CV_ERR ops_rel_smart(U32 invalid_history);
    CV_ERR ops_write_smart();

    CV_ERR ops_fetch_rec(U32 type, record_get_data_info *info);
    CV_ERR ops_rel_rec(U32 type, record_get_data_info *info, U32 invalid_history);
    CV_ERR ops_write_rec(U32 type, record_get_data_info *info, U32 *is_wait);
	CV_ERR ops_free_stream();
    CV_ERR ops_write_stream(U32 type, record_get_data_info *info);
	CV_ERR ops_input_stream(char *pFrameHead, int nHeadLen, char *pFrameData, int nDataLen, U32 dwCurTimeSec, U32 dwCurTimeUsec);

    CV_ERR proc_fetch_rec_data();	//force_new_data表示抛弃原来的，重新获取最新的数据
    CV_ERR proc_rel_rec_data(U32 invalid_history);	//invalid_history表示忘掉历史数据，重新获取最新的I帧
    U32 proc_is_fetch_fail_too_long(U32 th_value);

    CV_ERR proc_open_file();
    CV_ERR proc_close_file(U32 close_sync = TRUE, U32 need_sync = FALSE);
    CV_ERR proc_switch_file(U32 close_sync);

    CV_ERR proc_open_pre_rec_file();
    CV_ERR proc_close_pre_rec_file(U32 move_next, U32 close_sync = TRUE, U32 need_sync = FALSE);
    CV_ERR proc_switch_pre_rec_file(U32 close_sync);
    CV_ERR proc_del_pre_rec_file();
    CV_ERR proc_modify_pre_rec_file();
		
    CV_ERR proc_save_to_avss(U32 bPreRecord);
    CV_ERR proc_flush_avss();	//将avss中缓存的信息刷到硬盘，在停止录像或者长时间没有数据的时候调用

    U32 proc_is_need_switch_file(U32 file_time_len, U32 &close_sync, U32 &dir_lost);	//检查是否需要切换文件
    U32 proc_is_ok_for_switch_file(U32 dir_lost);	//检查切文件时机是否成熟

    void proc_update_disk_record_status(U32 is_recording);	//修改磁盘中的录像状态

    //管理补录的数据
    //void proc_init_replenish_para();
    //实时流切换文件,不关闭之前的文件，但是要吧数据刷进去
    CV_ERR proc_switch_new_filw_without_close();

private:
    record_wr_file_stm *m_cur_stm;
    record_stop_work_stm m_stop_work_stm;		//关闭文件，啥都不做
    record_normal_rec_stm m_normal_rec_stm;		//开着文件，录像
    record_pause_rec_stm m_pause_rec_stm;		//开着文件，不录像
    record_pre_rec_stm m_pre_rec_stm;			//开着文件，预录
	
    U32 m_ch_idx;
    U32 m_usr_set_trig_type;	//用户设置的触发类型
    U32 m_avss_trig_type;		//avss中正在录的类型，当用户设置类型变化的时候，从下一个I帧开始变化
	S32 m_ConnHandle[CV_ENC_STR_MAX];
    U32 m_bChangeHandle[CV_ENC_STR_MAX];

    U32 m_pre_rec_mode;		//0:不启用时间限制	1:启用时间限制;注内存限制始终启用
    U32 m_normal_rec_sec;
    U32 m_pre_rec_sec;

    time_t m_file_open_time;
    U32 m_file_is_open;

    record_get_data_info m_data[CV_ENC_STR_MAX];
	S32	m_recordmask;
	S32 m_lstrecordmask;

    cv_record_status m_record_status;
    U32 m_stop_cnt;

    time_t m_fetch_ok_time;	//获取数据成功的时间，用于计算，如果长时间获取失败，需要做处理

    //预录采用乒乓机制，录第三个淘汰第1个
    record_req_rec_dir_info m_rec_dir[3];
    U32 m_cur_avss_dir_idx;	//当前正在录的是哪个

    Common_Lock_T m_lock;//同步锁

	struct timeval prestarttime;
	struct timeval precurtime;
	U32	bPreRecordNeedKey_Main;
	U32	bPreRecordNeedKey_Aux;
	PreRecord_Node	*pQueueHead;
	PreRecord_Node	*pQueueTail;
	U32 PreRecordMem;
	U32 PreRecordUseMem;
	U32 m_bRecord;

	U32 m_lstFrameNo;

public:
    cv_record_wr_file_ch(U32 ch_idx); 	
    ~cv_record_wr_file_ch();   
    const char* class_name() {return "cv_record_wr_file_ch";};
    
private:
    cv_record_wr_file_ch(const cv_record_wr_file_ch &other);
    cv_record_wr_file_ch &operator=(const cv_record_wr_file_ch &other);
};

typedef struct
{
    pthread_t thr_hand;
    class cv_record_wr_file_manage *wr_file_manage;
    U32 thread_idx;
    U32 start_ch;
    U32 ch_num;  
}cv_rec_wr_file_thread_ctl;

class cv_record_wr_file_manage
{
public:
    CV_ERR stop_work(U32 ch_idx, U32 need_sync);//停止工作，关闭文件，为格式化、关机做准备

    CV_ERR start_record(U32 ch_idx, U32 trig_type, U32 save_pre_rec, U32 nSchedMode);	//save_pre_rec描述是否保留预录文件(保留预录文件即预录文件转正)
    CV_ERR stop_record(U32 ch_idx);	//停止录像，由于采用容器文件，只是不灌数据了，但是不关闭文件，如果是预录模式下，则关闭文件

    CV_ERR set_mask(U32 ch_idx, U32 mask);

    U32 get_rec_trig_type(U32 ch_idx);	//获取当前录像的类型

    CV_ERR set_pre_rec_mode(U32 ch_idx, U32 mode);

    CV_ERR set_pre_rec_sec(U32 ch_idx, U32 sec);

    CV_ERR set_pre_rec_mem(U32 ch_idx, U32 mem);

    U32 work(U32 start_ch, U32 ch_num);	//有数据写成功了返回true，表示需要继续工作，否则返回false，睡眠

    U32 readQueue(U32 start_ch, U32 ch_num);

    U32 is_recording(U32 ch);

    CV_ERR force_flush(U32 ch_idx);
	CV_ERR checkstreamhandle(U32 ch_idx, U32 type, U32* pcheck);
	CV_ERR setStreamHandel(U32 ch_idx, U32 type, S32 ConnHandle);

private:
    U32 is_valid_ch(U32 nvr_ch);
    CV_ERR set_replenishment_file_writer(U32 nvr_ch);

private:
    U32 m_ch_num;
    U32 m_handle[CV_MAX_LOCAL_CH_NUM];
    cv_record_wr_file_ch *m_ch[CV_MAX_LOCAL_CH_NUM];

    cv_rec_wr_file_thread_ctl	m_rec_thr[8];
    U32	m_rec_thr_num;

public:
    cv_record_wr_file_manage();
    ~cv_record_wr_file_manage();
    const char* class_name() {return "cv_record_wr_file_manage";};
    
private:
    cv_record_wr_file_manage(const cv_record_wr_file_manage &other);
    cv_record_wr_file_manage &operator=(const cv_record_wr_file_manage &other);
};



} //namespace cv_record{
#endif //#ifndef _RECORD_WR_FILE_H_
