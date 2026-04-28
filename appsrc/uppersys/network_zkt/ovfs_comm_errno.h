#ifndef _OVFS_COMM_ERRNO_H_
#define _OVFS_COMM_ERRNO_H_

/* 是否启用调试宏 通过在Makefile.para中指定是否支持 */
#if defined (_SUPPORT_ASSERT_)
	#define OVFS_ASSERT(expr)     \
	do{                         \
	    if (!(expr)) { 			\
			OVFS_DUMP_RED;		\
	        printf("ASSERT!\n >File name: %s\n >Function: %s\n >Line No: %d\n >Condition: %s\n", \
	                __FILE__,__FUNCTION__, __LINE__, #expr);\
	        OVFS_DUMP_NONE;		\
	        exit(-1);\
	    } \
	}while(0)
#else
	#define OVFS_ASSERT(expr)
#endif

namespace ovfs_soft {

typedef enum
{
    OVFS_SUCCESS = 0,	//函数成功

    //统一规划各个模块的API的返回错误码，用OVFS_ERR_XXX_GG_YY之类的名字来描述，XXX表示模块名，后面的GGYY自己琢磨这怎么言简意赅的描述错误原因
    //每个模块的错误码划分一个范围，不要占用别人的范围
    //------------------------------- Network ------------------------
    OVFS_ERR_NETWORK_BASE 			= 0x40070000,
    OVFS_ERR_NETWORK_UNKNOWN 		= OVFS_ERR_NETWORK_BASE + 0x01,
    OVFS_ERR_NETWORK_OPERATE_FAIL 	= OVFS_ERR_NETWORK_BASE + 0x02,
    OVFS_ERR_NETWORK_INVALID_PARA 	= OVFS_ERR_NETWORK_BASE + 0x03,
    OVFS_ERR_NETWORK_NOT_INIT		= OVFS_ERR_NETWORK_BASE + 0x04,
    OVFS_ERR_NETWORK_CALLBACK_NULL	= OVFS_ERR_NETWORK_BASE + 0x05,

    //------------------------------- Cfg_manage ------------------------
    OVFS_ERR_CFGM_BASE 			= 0x40110000,
    OVFS_ERR_CFGM_UNKNOWN 		= OVFS_ERR_CFGM_BASE + 0x01,
    OVFS_ERR_CFGM_NOT_INIT		= OVFS_ERR_CFGM_BASE + 0x02,
    OVFS_ERR_CFGM_INVALID_PARA 	= OVFS_ERR_CFGM_BASE + 0x03,	
    OVFS_ERR_CFGM_OPERATE_FAIL  = OVFS_ERR_CFGM_BASE + 0x04,
    OVFS_ERR_CFGM_NOT_SUPPORT	= OVFS_ERR_CFGM_BASE + 0x05,

    //------------------------------- Utility ------------------------
    OVFS_ERR_UTL_BASE 			= 0x40120000,
    OVFS_ERR_UTL_UNKNOWN 		= OVFS_ERR_UTL_BASE + 0x01,
    OVFS_ERR_UTL_NOT_INIT		= OVFS_ERR_UTL_BASE + 0x02,
    OVFS_ERR_UTL_INVALID_PARA	= OVFS_ERR_UTL_BASE + 0x03,
    OVFS_ERR_UTL_OPERATE_FAIL	= OVFS_ERR_UTL_BASE + 0x04,
    OVFS_ERR_UTL_NOT_SUPPORT	= OVFS_ERR_UTL_BASE + 0x05,
    OVFS_ERR_UTL_MALLOC_FAIL	= OVFS_ERR_UTL_BASE + 0x06,

    OVFS_ERR_UTL_LOCK_BUSY		= OVFS_ERR_UTL_BASE + 0x20,	//锁被占用
    OVFS_ERR_UTL_LOCK_INIT_FAIL = OVFS_ERR_UTL_BASE + 0x21,	//锁初始化失败
    OVFS_ERR_UTL_DOUBLE_UNLOCK	= OVFS_ERR_UTL_BASE + 0x22,	//锁释放异常
    OVFS_ERR_UTL_FILE_IO_ERR    = OVFS_ERR_UTL_BASE + 0x23,	
}OVFS_ERR;


}//namespace ovfs_soft {
#endif //#ifndef _OVFS_COMM_ERRNO_H_

