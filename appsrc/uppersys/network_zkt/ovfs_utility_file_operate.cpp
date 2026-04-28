#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

 
//#include "ovfs_comm_debug.h"
#include "ovfs_comm_errno.h"

#include "ovfs_utility_file_operate.h"
//#include "ovfs_utility_resource.h"
//include "ovfs_utility_base.h"
#include <errno.h> 

using namespace ovfs_soft;
//using namespace ovfs_utility;

txt_file_ops::txt_file_ops()
{
    m_fd = NULL;
    m_permit_wr = OVFS_FALSE;
    m_fpath[0] = 0;
    m_file_size = 0;
}

txt_file_ops::~txt_file_ops()
{
    if (m_fd != NULL)
        fclose(m_fd);
}

OVFS_ERR txt_file_ops::open(const char *fname, ovfs_file_open_mode mode)
{
    if (fname == NULL)
    {
        LOGE("fname is null!\n");
        return OVFS_ERR_UTL_INVALID_PARA;
    }

    char open_para[8];
    switch (mode)
    {
        case OVFS_FILE_OPEN_RD:
            snprintf(open_para, sizeof(open_para), "r");
            m_permit_wr = OVFS_FALSE;
            break;

        case OVFS_FILE_OPEN_RW:
            snprintf(open_para, sizeof(open_para), "r+");
            m_permit_wr = OVFS_TRUE;
            break;

        case OVFS_FILE_OPEN_RW_APPEND:
            snprintf(open_para, sizeof(open_para), "a+");
            m_permit_wr = OVFS_TRUE;
            break;

        case OVFS_FILE_OPEN_RW_TRUNC:
            snprintf(open_para, sizeof(open_para), "w+");
            m_permit_wr = OVFS_TRUE;
            break;

        default:
            LOGE("mode = %d not support!\n", mode);
            return OVFS_ERR_UTL_NOT_SUPPORT;
    }

    close();
    m_fd = fopen(fname, open_para);
    if (m_fd == NULL)
    {
        LOGE("%s open fail!\n", fname);
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    snprintf(m_fpath, sizeof(m_fpath), "%s", fname);

    //记录文件大小
    long cur_pos = ftell(m_fd);
    fseek(m_fd, 0, SEEK_END);
    m_file_size = ftell(m_fd);
    fseek(m_fd, cur_pos, SEEK_SET);

    return OVFS_SUCCESS;
}

OVFS_ERR txt_file_ops::close()
{
    if (m_fd != NULL)
    {
        fclose(m_fd);
        m_fd = NULL;
        m_file_size = 0;
    }
    m_fpath[0] = 0;

    return OVFS_SUCCESS;
}

//一次读取一行
OVFS_ERR txt_file_ops::read(S8 *buf, size_t buf_size)
{
    if (m_fd == NULL)
    {
        LOGE("fd not open!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    if (buf == NULL)
    {
        LOGE("buf == NULL!\n");
        return OVFS_ERR_UTL_INVALID_PARA;
    }
	
    return (fgets(buf, buf_size, m_fd) != NULL) ? OVFS_SUCCESS : OVFS_ERR_UTL_OPERATE_FAIL;
}

OVFS_ERR txt_file_ops::write(const S8 *str)
{
    if (m_fd == NULL)
    {
        LOGE("fd not open!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    if (str == NULL)
    {
        LOGE("buf == NULL!\n");
        return OVFS_ERR_UTL_INVALID_PARA;
    }

    if (!m_permit_wr)
    {
        LOGE("not permit write!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    OVFS_ERR ret = (fputs(str, m_fd) != EOF) ? OVFS_SUCCESS : OVFS_ERR_UTL_OPERATE_FAIL;
    //刷新文件大小
    long cur_pos = ftell(m_fd);
    if (cur_pos > m_file_size)
        m_file_size = cur_pos;

    return ret;
}
	
	
//ofset为负数表示向前偏移，为正数表示向后偏移
OVFS_ERR txt_file_ops::seek(ovfs_file_cnt_ofset_mode mode, S32 ofset)
{
    if (m_fd == NULL)
    {
        LOGE("fd not open!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    switch (mode)
    {
        case OVFS_FILE_CNT_OFSET_HEAD:
            if (ofset <= 0)
                fseek(m_fd, 0, SEEK_SET);
            else
                fseek(m_fd, ofset, SEEK_SET);
            break;

        case OVFS_FILE_CNT_OFSET_TAIL:
            if (ofset >= 0)
                fseek(m_fd, 0, SEEK_END);
            else
                fseek(m_fd, ofset, SEEK_END);
            break;

        case OVFS_FILE_CNT_OFSET_CUR:
            fseek(m_fd, ofset, SEEK_CUR);
            break;

        default:
            LOGE("mode = %d not support!\n", mode);
            return OVFS_ERR_UTL_NOT_SUPPORT;
    }
	
    return OVFS_SUCCESS;
}

//同步到磁盘，成功返回OVFS_SUCCESS，失败返回其他
OVFS_ERR txt_file_ops::sync()
{
    if (m_fd == NULL)
    {
        LOGE("fd not open!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    //fflush()标准IO函数（如fread，fwrite等）会在内存中建立缓冲，该函数刷新内存缓冲，将内容写入内核缓冲，
    //要想将其真正写入磁盘，还需要调用fsync。（即先调用fflush然后再调用fsync，否则不会起作用）
    //fflush以指定的文件流描述符为参数（对应以fopen等函数打开的文件流），仅仅是把上层缓冲区中的数据刷新到内核缓冲区就返回，
    //因此相对于fsync而言不是很安全，还需要再调用一下fsync来把数据真正写入硬盘。	
    if(fflush(m_fd) == 0)
    {
        //将文件流描述符转换成文件描述符
        int fp = fileno(m_fd);
        return (fsync(fp) == 0) ? OVFS_SUCCESS : OVFS_ERR_UTL_OPERATE_FAIL;
    }
    else
    {
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }		
    //return (fflush(m_fd) == 0) ? OVFS_SUCCESS : OVFS_ERR_UTL_OPERATE_FAIL;
}

//获取文件大小，以字节为单位
S32 txt_file_ops::get_file_size()
{
    return m_file_size;
}

//---------------------------------------------- binary_file_ops -----------------------------
binary_file_ops::binary_file_ops()
{
    m_fd = -1;
    m_permit_wr = OVFS_FALSE;
    m_fpath[0] = 0;
    m_file_size = 0;
}

binary_file_ops::~binary_file_ops()
{
    if (m_fd > 0)
        ::close(m_fd);
}

OVFS_ERR binary_file_ops::open(const char *fname, ovfs_file_open_mode mode, ovfs_file_io_schedule io_shedule)
{
    if (fname == NULL)
    {
        LOGE("fname is null!\n");
        return OVFS_ERR_UTL_INVALID_PARA;
    }

    int flags = O_LARGEFILE;	//支持2G以上大文件
    mode_t op_mode = 0;
    switch (mode)
    {
        case OVFS_FILE_OPEN_RD:
            flags |= O_RDONLY;
            m_permit_wr = OVFS_FALSE;
            break;

        case OVFS_FILE_OPEN_RW:
            flags |= O_RDWR;
            m_permit_wr = OVFS_TRUE;
            break;

        case OVFS_FILE_OPEN_RW_APPEND:
            flags |= O_RDWR | O_CREAT | O_APPEND;
            op_mode = S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH;
            m_permit_wr = OVFS_TRUE;
            break;

        case OVFS_FILE_OPEN_RW_TRUNC:
            flags |= O_RDWR | O_CREAT | O_TRUNC;
            op_mode = S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH;
            m_permit_wr = OVFS_TRUE;
            break;

        default:
            LOGE("mode = %d not support!\n", mode);
            return OVFS_ERR_UTL_NOT_SUPPORT;
    }

    switch (io_shedule)
    {
        case OVFS_FILE_IO_DSYNC:	//当向文件写入数据的时候，只有当数据写到了磁盘时，写入操作才算完成(write才返回成功)。
            flags |= O_DSYNC;
            break;

        case OVFS_FILE_IO_RSYNC:	//表示文件读取时，该文件的OS cache必须已经全部flush到磁盘了;
            flags |= O_RSYNC;
            break;

        //比O_DSYNC更严格，不仅要求数据已经写到了磁盘，而且对应的数据文件的属性(例如文件长度等)也需要更新完成才算write操作成功。可见O_SYNC较之O_DSYNC要多做一些操作。
        case OVFS_FILE_IO_SYNC:
            flags |= O_SYNC;
            break;

        //则读/写操作都会跳过OS cache，直接在device(disk)上读/写。因为没有了OS cache，所以会O_DIRECT降低文件的顺序读写的效率。
        case OVFS_FILE_IO_DIRECT:
            flags |= O_DIRECT;
            break;

        default:
            break;
    }

    close();
    m_fd = ::open(fname, flags, op_mode);
    if (m_fd < 0)
    {
        LOGE("%s open fail!. flags=%d, op_mode=%d, user_mode = %d, Error %d\n", fname, flags, op_mode, mode, errno);
        //对于IO错误添加一个特别的返回值，方便上层进行特殊的处理
        if(errno == EIO)
            return OVFS_ERR_UTL_FILE_IO_ERR;

        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    //UTL_CLASS_PD("%s open OK!\n", fname);
    snprintf(m_fpath, sizeof(m_fpath), "%s", fname);

    //记录文件大小
    struct stat64 file_stat64;
    if (fstat64(m_fd, &file_stat64) < 0)
    {
        LOGE("%s fstat64 fail!\n", fname);
        off64_t cur_pos = lseek64(m_fd, 0, SEEK_CUR);
        m_file_size = lseek64(m_fd, 0, SEEK_END);
        lseek64(m_fd, cur_pos, SEEK_SET);
    }
    else
        m_file_size = file_stat64.st_size;
	
    return OVFS_SUCCESS;
}

OVFS_ERR binary_file_ops::close()
{
    if (m_fd > 0)
    {
        ::close(m_fd);
        m_fd = -1;
        m_file_size = 0;
    }
    
    m_fpath[0] = 0;
    return OVFS_SUCCESS;
}

//读指定大小数据，返回实际读取的大小
S32 binary_file_ops::read(void* buf, size_t buf_size, size_t need_rd_size)
{
    if (m_fd < 0)
    {
        LOGE("fd not open!\n");
        return 0;
    }

    if (buf == NULL)
    {
        LOGE("buf == NULL!\n");
        return 0;
    }

    if (buf_size < need_rd_size)
    {
        LOGE("buf_size = %d, need_rd_size = %d err\n", buf_size, need_rd_size);
        return 0;
    }

    S32 rd_size = ::read(m_fd, buf, need_rd_size);
    return (rd_size < 0) ? 0 : rd_size;
}

//写指定大小数据，返回实际写入的大小
S32 binary_file_ops::write(const void* buf, size_t need_wr_size)
{
    if (m_fd < 0)
    {
        //LOGE("fd not open!\n");
        return 0;
    }

    if (buf == NULL)
    {
        LOGE("buf == NULL!\n");
        return 0;
    }

    if (!m_permit_wr)
    {
        LOGE("not permit write!\n");
        return 0;
    }

    S32 wr_size = ::write(m_fd, buf, need_wr_size);
    //S32 wr_size = need_wr_size;

    //刷新文件大小
    if (wr_size > 0)
        m_file_size += (S64)wr_size;

    return (wr_size < 0) ? 0 : wr_size;
}	

//ofset为负数表示向前偏移，为正数表示向后偏移
OVFS_ERR binary_file_ops::seek(ovfs_file_cnt_ofset_mode mode, S64 offset)
{
    if (m_fd < 0)
        return OVFS_ERR_UTL_OPERATE_FAIL;
        
    S32 seek_pos = SEEK_SET;
    S64 real_offset = offset;
    switch (mode)
    {
        case OVFS_FILE_CNT_OFSET_HEAD:
            real_offset = offset < 0 ? 0 : offset;
            seek_pos = SEEK_SET;
            break;

        case OVFS_FILE_CNT_OFSET_TAIL:
            real_offset = offset > 0 ? 0 : offset;
            seek_pos = SEEK_END;
            break;

        case OVFS_FILE_CNT_OFSET_CUR:
            real_offset = offset;
            seek_pos = SEEK_CUR;
            break;

        default:
            LOGE("mode = %d not support!\n", mode);
            return OVFS_ERR_UTL_NOT_SUPPORT;
    }

    return (lseek64(m_fd, real_offset, seek_pos) == -1) ? OVFS_ERR_UTL_OPERATE_FAIL : OVFS_SUCCESS;
}

OVFS_ERR binary_file_ops::truncate(S64 size)
{
    if (m_fd < 0)
    {
        LOGE("fd not open!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    if (ftruncate64(m_fd, size) < 0)
    {
        LOGE("%s ftruncate64 %llu fail!\n", m_fpath, size);
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    return OVFS_SUCCESS;
}

//同步到磁盘，成功返回OVFS_SUCCESS，失败返回其他
OVFS_ERR binary_file_ops::sync()
{
    if (m_fd < 0)
    {
        //LOGE("fd not open!\n");
        return OVFS_ERR_UTL_OPERATE_FAIL;
    }

    return (fsync(m_fd) == 0) ? OVFS_SUCCESS : OVFS_ERR_UTL_OPERATE_FAIL;
}

//获取文件大小，以字节为单位
S64 binary_file_ops::get_file_size()
{
    return m_file_size;
}

//获取当前文件指针位置
S64 binary_file_ops::get_fd_pos()
{
    if (m_fd < 0)
        return 0;

    return lseek64(m_fd, 0, SEEK_CUR);
}


