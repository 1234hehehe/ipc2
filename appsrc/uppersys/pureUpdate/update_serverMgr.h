#ifndef __UPDATE_SERVERMGR_H__
#define __UPDATE_SERVERMGR_H__


#include <time.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <pthread.h>
#include <sys/time.h>
#endif
//#include "ants_update_md5.h"
#include "libcommon_api.h"

#define LONG int
#define DWORD unsigned int
#define BOOL int
#define TRUE  1
#define FALSE 0
#ifdef WIN32
#define TRACEINFO(...) 		do{printf("[INF.%s.%d]",__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#define TRACEINFOEX(...) 		do{printf("[INF.%s.%d]",__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#define TRACEERROR(...)		do{printf("[ERR.%s.%d]",__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#define TRACEDEBUG(...)		do{printf("[DBG.%s.%d]",__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#define TRACEWARN(...)			do{printf("[WAR.%s.%d]",__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#define TRACEFATAL(...)		do{printf("[FAT.%s.%d]",__FUNCTION__,__LINE__);printf(__VA_ARGS__);}while(0)
#else
#define TRACEINFO(x...) 		do{printf("[INF.%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#define TRACEINFOEX(x...) 		do{printf("[INF.%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#define TRACEERROR(x...)		do{printf("[ERR.%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#define TRACEDEBUG(x...)		do{printf("[DBG.%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#define TRACEWARN(x...)			do{printf("[WAR.%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#define TRACEFATAL(x...)		do{printf("[FAT.%s.%d]",__FUNCTION__,__LINE__);printf(x);}while(0)
#endif

#define	UPDATASTARTCODE		0xC5010001

#define	KERNELHEAD			0x56190527
#define	KERNELSTARTADDR		0x80000

typedef enum _tagUpdateStatus{
    UPDATE_STATUS_E_FAILED    = -1,
    UPDATE_STATUS_E_IDLE      =  0,
    UPDATE_STATUS_E_BEGIN     =  1,
    UPDATE_STATUS_E_UPDATING  =  2,
    UPDATE_STATUS_E_UPDATING_PLUS1 = 3,
    UPDATE_STATUS_E_UPDATING_PLUS2 = 4,
	UPDATE_STATUS_E_FINISH	  =  5,
}UpdateStatus_T;

/*
-------------------------------------------------------------------------
|updateFileHeader|packageoffsetsTable|boardTypesTable|packageheader|package data|package header|package data|...
-------------------------------------------------------------------------
*/

typedef struct UpdateSvrMgr
{
	void (*SetBoardType)(DWORD type);
	void (*SetSoftwareVersion)(DWORD version);
 	LONG (*CreateTask)(char *pBuffer);
 	LONG (*CreateTaskFile)(char *pFileName);
 	BOOL (*DestroyTask)(LONG lHandle);
 	BOOL (*GetTaskStatus)(LONG lHandle, LONG *lUpdataStatus, LONG *lUpdataProgress);
	int (*DoUpdate)();
	void (*SetStatusFlag)(int bSucc);
	int (*UpdataThread)(Common_Thread_T hThread,void *pParam);

    BOOL					m_bUpdataOK;
	Common_Thread_T			m_UpdataThreadId;

	FILE					*m_hUpgradeFile;
    char                     m_acUpgradeFileName[256];

	LONG					m_lHandleCnt;
	BOOL					m_bStartTask;

	BOOL					m_bWriteChannelLogo;
	DWORD                   m_dwBoardType;
	DWORD                   m_dwSoftwareVersion;

    /** RPU(RawPartionUpgrade)/AUPF(AntsUpdatePackageFile) */
    LONG                    m_lUpgradeFileFormat;
    LONG                    m_lUpgradeFileFormatCheckOK;
}UpdateSvrMgr;

UpdateSvrMgr* updateServer_getUpdateItem();

#endif


