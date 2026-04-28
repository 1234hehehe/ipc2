#ifndef _OVFS_UTILITY_FILE_OPERATE_H_
#define _OVFS_UTILITY_FILE_OPERATE_H_

#include "ovfs_comm_errno.h"
#include "ovfs_comm_def.h"
#include "libcommon_api.h"
//#include "ovfs_comm_sys.h"

//#include "ovfs_utility_def.h"
//#include "ovfs_utility_lock.h"

//using ovfs_soft::OVFS_ERR;
//using ovfs_soft::ovfs_user_id;
//using ovfs_soft::ovfs_txt_file_operate;
//using ovfs_soft::ovfs_binary_file_operate;

namespace ovfs_soft {//定义文件的操作方式

typedef enum
{
	OVFS_FILE_OPEN_UNKNOW,
		
	OVFS_FILE_OPEN_RD,	//只读，不创建文件，如果成功，文件指针位于文件头
	OVFS_FILE_OPEN_RW,	//读写，不创建文件，如果成功，文件指针位于文件头

	OVFS_FILE_OPEN_RW_APPEND,	//读写，如果不存在，创建文件，如果存在，继续添加类容，只能添加到文件末端，写入操作时会自动seek到文件尾
	OVFS_FILE_OPEN_RW_TRUNC,	//读写，如果不存在，创建文件，如果存在，覆盖原来的文件，如果成功，文件指针位于文件尾
}ovfs_file_open_mode;

typedef enum
{
	OVFS_FILE_CNT_OFSET_UNKNOW,
		
	OVFS_FILE_CNT_OFSET_HEAD,	//按照文件头来计算偏移
	OVFS_FILE_CNT_OFSET_TAIL,	//按照文件尾来计算偏移
	OVFS_FILE_CNT_OFSET_CUR,	//按照当前文件指针位置来计算偏移
}ovfs_file_cnt_ofset_mode;

typedef enum
{
	OVFS_FILE_IO_DEFAULT,	//内核调度，不管
	OVFS_FILE_IO_DSYNC,	//当向文件写入数据的时候，只有当数据写到了磁盘时，写入操作才算完成(write才返回成功)。
 	OVFS_FILE_IO_RSYNC,	//表示文件读取时，该文件的OS cache必须已经全部flush到磁盘了;
 	OVFS_FILE_IO_SYNC,	//比O_DSYNC更严格，不仅要求数据已经写到了磁盘，而且对应的数据文件的属性(例如文件长度等)也需要更新完成才算write操作成功。可见O_SYNC较之O_DSYNC要多做一些操作。
 	OVFS_FILE_IO_DIRECT,//则读/写操作都会跳过OS cache，直接在device(disk)上读/写。因为没有了OS cache，所以会O_DIRECT降低文件的顺序读写的效率。
}ovfs_file_io_schedule;



class txt_file_ops
{
public:
	OVFS_ERR	open(const char *fname, ovfs_file_open_mode mode);
	OVFS_ERR	close();
	
	OVFS_ERR	read(S8 *buf, size_t buf_size);	//一次读取一行
	OVFS_ERR	write(const S8 *str);
	
	OVFS_ERR	seek(ovfs_file_cnt_ofset_mode mode, S32 ofset);	//ofset为负数表示向前偏移，为正数表示向后偏移
	
	OVFS_ERR	sync();	//同步到磁盘，成功返回OVFS_SUCCESS，失败返回其他
	S32	get_file_size();	//获取文件大小，以字节为单位

private:
	FILE* m_fd;
	char m_fpath[OVFS_BACKUP_PLAY_FILE_PATH];	//记录打开的文件，用于诊断
	
	OVFS_BOOL m_permit_wr;
	S32 m_file_size;
	
public:
	txt_file_ops();
	~txt_file_ops();
	const char* class_name() {return "txt_file_ops";};
private:
	txt_file_ops(const txt_file_ops &other);
	txt_file_ops &operator=(const txt_file_ops &other);
};

class binary_file_ops
{
public:
	OVFS_ERR	open(const char *fname, ovfs_file_open_mode mode, ovfs_file_io_schedule io_shedule);
	OVFS_ERR	close();
	
	S32 read(void* buf, size_t buf_size, size_t need_rd_size);	//读指定大小数据，返回实际读取的大小
	S32 write(const void* buf, size_t need_wr_size);	//写指定大小数据，返回实际写入的大小
	
	OVFS_ERR	seek(ovfs_file_cnt_ofset_mode mode, S64 ofset);	//ofset为负数表示向前偏移，为正数表示向后偏移
	OVFS_ERR	truncate(S64 size);
	
	OVFS_ERR sync();			//同步到磁盘，成功返回OVFS_SUCCESS，失败返回其他
	S64 get_file_size();	//获取文件大小，以字节为单位
	S64 get_fd_pos();		//获取当前文件指针位置

private:
	int 	m_fd;
	char m_fpath[OVFS_BACKUP_PLAY_FILE_PATH];	//记录打开的文件，用于诊断
	OVFS_BOOL m_permit_wr;
	S64 m_file_size;
	
public:
	binary_file_ops();
	~binary_file_ops();
	const char* class_name() {return "binary_file_ops";};
private:
	binary_file_ops(const binary_file_ops &other);
	binary_file_ops &operator=(const binary_file_ops &other);
};

}//namespace ovfs_soft {
#endif
