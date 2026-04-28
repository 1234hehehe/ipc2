#ifndef _SQLITE3MGR_H_
#define _SQLITE3MGR_H_
#include<pthread.h>

#include "sqlite3.h"
#include "libcommon_struct.h"
#include "libcommon_api.h"
#include <map>

using	namespace std;

#define LOGROOTPATH  "/tmp/mmc/mmc1/log"
#define LOGPATH		 "/tmp/mmc/mmc1/log/UsrLogFile"
#define ALARMPICPATH "/tmp/mmc/mmc1/log/AlarmPic"
#define LOGFILENAME	 "/tmp/mmc/mmc1/log/UsrLogFile/Log.db"

#define TABLE_LOG "UsageLog"
#define TABLE_ALARM "AlarmInfo"

#define MAX_LOG_COUNT	1000
#define MAX_NAMELEN 32
#define MAX_HOSTLEN 128

#define MAX_ALARM_COUNT	500


#define LONG int
#define BOOL int
#define    TRUE	1
#define	   FALSE 0
#define DWORD U32
#define BYTE U8

/*******************查找文件和日志函数返回值*************************/
#define ANTS_DVR_FILE_SUCCESS		1000	//获得文件信息
#define ANTS_DVR_FILE_NOFIND		1001	//没有文件
#define ANTS_DVR_ISFINDING			1002	//正在查找文件
#define	ANTS_DVR_NOMOREFILE			1003	//查找文件时没有更多的文件
#define	ANTS_DVR_FILE_EXCEPTION		1004	//查找文件时异常

enum LOGQUERY_MODE
{
	QUERY_ALL = 0,
	QUERY_BY_TYPE,
	QUERY_BY_TIME,
	QUERY_BY_TYPE_AND_TIME,
};

typedef struct tag_Ovfs_Log
{
    int                     dwId;
	S32						dwLogTime;
	S32						dwMajorType;					//!主类型  0-全部;1-操作; 2-配置; 3-
	S32						dwMinorType;					//!次类型 0-全部;
	char					sUser[MAX_NAMELEN];				//!用户名
	char					sIP[MAX_HOSTLEN];	            //!远程主机地址
	S32						dwChannel;						//!通道号
	char					sExtraInfo[MAX_HOSTLEN];        //!额外信息
}OVFS_LOG, *POVFS_LOG;

typedef struct tag_Ovfs_AlarmInfo
{
    S32                     dwId;
	S32						dwAlarmType;					//!
	S32						dwStartTime;
    S32						dwEndTime;
	char					sAlarmPic[MAX_HOSTLEN];         //!AlarmPic
	char					sExtraInfo[MAX_HOSTLEN];        //!额外信息
}OVFS_ALARMNFO, *POVFS_ALARMINFO;

typedef struct tag_QueryParam
{
    int                         iLogType;
	char						*pTableName;
	LONG						lCount;
	LONG						lTotalCnt;
}QueryParam, *LPQueryParam;

typedef map<LONG, LPQueryParam>	LogSeachMap;

class SQLITE3MGR
{
public:
	static SQLITE3MGR*		GetSqlMgrItem();
	static void				DelSqlMgrItem();
	int					    OpenDB(char *sDBFileName, DWORD dwSerial);
	BOOL					CloseDB();
    int                     CreateTable(char *sSql);
    int                     GetLogCount(char *sTableName);
    int                     DeleteLog(char *sTableName);
	int					    InsertLog(OVFS_LOG strLog);
    int                     InsertAlarmInfo(OVFS_ALARMNFO strInfo, int *iId);
    int                     UpdateAlarmInfo(OVFS_ALARMNFO strInfo);

	//LONG					QueryStart(LONG lMode, DWORD dwMajorType, DWORD dwMinorType, time_t tStartTime, time_t tStopTime,int nTry =0);
	LONG					QueryStart(int iLogType, int dwType, int tStartTime, int tStopTime, int iSort);
	//LONG					GetLogs(LONG lHandle, POVFS_LOG pLogBuf);
	BOOL					GetQueryLogsCnt(LONG lHandle, LONG *lplGetCnt);
	LONG					GetLogsEX(LONG lHandle, POVFS_LOG pLogBuf, LONG lStartCnt, LONG lGetCnt);
	int					    QueryStop(int iLogType, int lHandle);

    int GetQueryList(int iLogType, LONG iHandle, int iStart, int iCount, void **pList, int *iListCount);

private:
	SQLITE3MGR();
	~SQLITE3MGR();

	void					Lock();
	void					UnLock();

	DWORD                   m_bBusy;

	BOOL					m_bOpen;

	pthread_mutex_t			m_pMutex;

	pthread_mutex_t			m_pQueryMutex;

	sqlite3					*m_pDBHandle;
	sqlite3_stmt			*m_pInsertLogStmt;
	sqlite3_stmt			*m_pInsertAlarmStmt;
	sqlite3_stmt			*m_pDeleteLogStmt;

	LONG					m_lLogCount;
    LONG                    m_lAlarmCount;

	LONG					m_lQureyCount;
	LogSeachMap				m_QureyInfo;

	static SQLITE3MGR*		m_pInstance;
};

#endif
