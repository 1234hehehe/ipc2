#ifndef __I8_DDNS_API_H__
#define __I8_DDNS_API_H__

#ifdef WIN32
typedef __int64 S64;
typedef unsigned __int64 U64;
#else
typedef long long S64;
typedef unsigned long long U64;
#endif

typedef int S32;
typedef char S8;
typedef short S16;
typedef unsigned int U32;
typedef unsigned char U8;
typedef unsigned short U16;
typedef float F32;
typedef double F64;
typedef void VOID;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define I8_DDNS_FOURCC(c0,c1,c2,c3) ((c0)|(c1 << 8) |(c2 << 16) | (c3 << 24))

//!#define I8_DDNS_DEBUG_PRINT
#ifdef I8_DDNS_DEBUG_PRINT
#define I8_DDNS_TRACE_INFO(x...) 		do{printf("[INFO.%s.%d][%s]",__FUNCTION__,__LINE__,DDNS_NAME);printf(x);}while(0)
#define I8_DDNS_TRACE_ERROR(x...)		do{printf("[ERR .%s.%d][%s]",__FUNCTION__,__LINE__,DDNS_NAME);printf(x);}while(0)
#define I8_DDNS_TRACE_DEBUG(x...)		do{printf("[DBG .%s.%d][%s]",__FUNCTION__,__LINE__,DDNS_NAME);printf(x);}while(0)
#define I8_DDNS_TRACE_WARN(x...)		do{printf("[WARN.%s.%d][%s]",__FUNCTION__,__LINE__,DDNS_NAME);printf(x);}while(0)
#define I8_DDNS_TRACE_FATAL(x...)		do{printf("[FATL.%s.%d][%s]",__FUNCTION__,__LINE__,DDNS_NAME);printf(x);}while(0)
#else
#define I8_DDNS_TRACE_INFO(x...)
#define I8_DDNS_TRACE_ERROR(x...)
#define I8_DDNS_TRACE_DEBUG(x...)
#define I8_DDNS_TRACE_WARN(x...)
#define I8_DDNS_TRACE_FATAL(x...)
#endif

#define I8_DDNS_PRIVATE				(0)
#define I8_DDNS_DYNDNS				(1)
#define I8_DDNS_PEANUTHULL			(2)
#define I8_DDNS_NOIP					(3)
#define I8_DDNS_3322					(4)

#define I8_DDNS_HINOVOM				(5) //!海视诺私有域名解析服务器
#define I8_DDNS_LIMAI					(5) //!立迈DDNS
#define I8_DDNS_MAGUS				(5) //!印度Magus DDNS
#define I8_DDNS_ELMO					(6) //!
#define I8_DDNS_DNSDYNAMIC			(7) 

//!用户定制选项
#define I8_DDNS_COMELITDNS			(8)
#define I8_DDNS_CNBDNS				(8)
#define I8_DDNS_ALTANTISDNS			(8)
#define I8_DDNS_DANA                			(8) //!大拿
#define I8_DDNS_ESOLCCTV				(8) //!ESOLCCTV
#define I8_DDNS_SHANYDNS				(9) //!夏尼

//!定义I8_DDNS支持的版本
#define I8_DDNS_VERSION  				0x20131220


//!定义I8_DDNS协议
#define I8_DDNS_FLAG0 I8_DDNS_FOURCC('A','N','T','S')
#define I8_DDNS_FLAG1 I8_DDNS_FOURCC('D','D','N','S')

//!接口返回值定义
typedef enum{
	I8_DDNS_OK = 0 ,
	I8_DDNS_FAILED = -1000 ,
	I8_DDNS_NO_INIT,
	I8_DDNS_NO_MEMORY,
	I8_DDNS_INVALID_PARAM,
	I8_DDNS_BUFFER_TOO_SMALL,
	I8_DDNS_CONNECT_ERROR,
	I8_DDNS_SEND_ERROR,
	I8_DDNS_RECV_ERROR,	
}I8_DDNS_RET_CODE;

//!接口函数定义
/*!
* \brief 初始化函数
* \return 返回值为0,表示成功,否则返回错误码
*/
typedef S32 (*I8_DDNS_INIT)( );

/*!
* \brief 反初始化函数
* \return 返回值为0,表示成功,否则返回错误码
*/
typedef S32 (*I8_DDNS_UNINIT)( );

/*!
* \brief 获取库能力集合函数
* \param lpBuffer 数据内容指针
* \param dwSize 数据内容大小
* \return 返回值为0,表示成功,否则返回错误码
*/
typedef S32 (*I8_DDNS_GETSUPPORTFUNCTIONS)(void *lpBuffer,U32 dwSize);

/*!
* \brief 设置相关参数函数
* \param wCtrlPort 设备控制端口
* \param wMediaPort 设备流媒体端口
* \param wHttpPort设备HTTP端口
* \return 返回值为0,表示成功,否则返回错误码
*/
typedef S32 (*I8_DDNS_SETPARAM)(U16 uCtrlPort,U16 uMediaPort,U16 uHttpPort);

/*!
* \brief DDNS IP更新函数
* \param lpServerHost DDNS服务器地址
* \param uServerPort DDNS服务器端口
* \param lpUserName DDNS用户名
* \param lpPassword DDNS密码
* \param lpDomainHost DDNS设备域名地址
* \return 返回值为0,表示成功,否则返回错误码
*/
typedef S32 (*I8_DDNS_UPGRADE)(S8 *lpServerHost,U16 uServerPort,S8 *lpUsername,S8 *lpPassword,S8 *lpDomainHost);

/*!
* \brief 获取外网IP地址函数
* \param lpUserName DDNS用户名
* \param lpPassword DDNS密码
* \return 返回值为0,表示成功,否则返回错误码
*/
typedef S32 (*I8_DDNS_GETEXTERNALIP)(S8 *lpUsername,S8 *lpPassword);

typedef S32 (*I8_DDNS_GETDEFAULTUSER)(S8 *lpUsername,S8 *lpPassword);

typedef struct{
	U32 u32Flag[2];				//!fourcc("OVFSDDNS") I8_DDNS_FLAG0,I8_DDNS_FLAG1
	U32 u32Size;					//!sizeof(I8_DDNS_FUNCTION_T)
	U32 u32Version;				//!I8_DDNS_VERSION
	U32 u32Index;					//!DDNS协议索引号	
	U32 u32ServerPort ;			//!DDNS域名服务器端口
	S8 szName[16];				//!DDNS协议名称(必须)
	S8 szServerHost[48] ;			//!DDNS域名服务器地址	
	I8_DDNS_INIT fpInit ;
	I8_DDNS_UNINIT fpUnInit ;
	I8_DDNS_UPGRADE fpUpgrade ;
	I8_DDNS_GETEXTERNALIP fpGetExternalIp ;
	I8_DDNS_SETPARAM fpSetParam ;
	I8_DDNS_GETDEFAULTUSER fpGetDefaultUser ;
}I8_DDNS_FUNCTION_T,*LPI8_DDNS_FUNCTION_T;

#ifdef __cplusplus
extern "C" {
#endif
//!获取本协议支持的功能
//!lpBuffer: I8_DDNS_FUNCTION_T,不同的版本可能此结构大小不一样
//!dwBufferSize: 指示提供缓冲的大小.当缓冲大小小于本版本协议功能结构的大小时,可以提供能接受的较低版本功能.
//!如果提供的缓冲大小比最低版本需要的缓冲还小，则返回失败.
//!返回:0-成功,-1失败
S32 I8_DDNS_GetSupportFunctions(void *lpBuffer,U32 dwSize);

#ifdef __cplusplus
}
#endif
#endif

