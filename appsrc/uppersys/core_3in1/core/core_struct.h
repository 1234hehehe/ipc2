#ifndef __CORE_STRUCT_H__
#define __CORE_STRUCT_H__
#define CORE_MODULE_MAX_NUM 128

#define CORE_MAKEFOURCC(ch0, ch1, ch2, ch3) ((U32)(U8)(ch0) | ((U32)(U8)(ch1) << 8) | ((U32)(U8)(ch2) << 16) | ((U32)(U8)(ch3) << 24 ))
#define CORE_PACKET_STARTCODE CORE_MAKEFOURCC('o','n','v','s')
typedef struct _tagCorePacketHeader
{
    unsigned int uStartCode;// 数据同步码
    int nDataLen;// 数据长度.
    unsigned int nDataFormat: 16; // 0- JSON
    unsigned int nExternHeaderLen: 16; // 扩展头长度
} CorePacketHeader_T;
typedef struct _tagCoreModuleInfo
{
    unsigned int uModuleID;// 1,2,3...
    char *pszModuleName;
    int nSocket;// 模块接入的socket,长连接
    char *pRecvBuffer;
    int nRecvLen;
    struct _tagCoreModuleInfo *pNext;
} CoreModuleInfo_T;

typedef struct _tagCoreModuleMgr
{
    CoreModuleInfo_T *pModuleInfo[CORE_MODULE_MAX_NUM];
    CoreModuleInfo_T *pModuleHead;
    int nModuleNum;
    int nListenSocket;
    int nMaxSocket;
    fd_set tReadSet, tWriteSet, tExceptionSet;
    int nReadNum, nWriteNum, nExcepNum;
} CoreModuleMgr_T;


#define closeSocket closesocket

#define CORE_DEBUG(x...) printf(x)
#define CORE_INFO(x...) printf(x)
#define CORE_ERROR(x...) printf(x)

#endif
