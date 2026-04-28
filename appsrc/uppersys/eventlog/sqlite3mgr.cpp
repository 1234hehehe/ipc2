#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include "libcommon_api.h"
#include "sqlite3mgr.h"

SQLITE3MGR* SQLITE3MGR::m_pInstance = NULL;

SQLITE3MGR* SQLITE3MGR::GetSqlMgrItem()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new SQLITE3MGR();
		if(0 != access(LOGROOTPATH,0))
		{
			mkdir(LOGROOTPATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		}
		if(0 == access(LOGROOTPATH,0))
		{
			if(0 != access(LOGPATH, 0))
			{
				mkdir(LOGPATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			}

            if(0 != access(ALARMPICPATH, 0))
			{
				mkdir(ALARMPICPATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			}
			if(m_pInstance->OpenDB((char*)LOGFILENAME, 0))
			{
				if(0 == access(LOGPATH, 0))
				{
					char str[128];
					sprintf(str, "rm -r %s", LOGPATH);
					Common_System(str);
					mkdir((char*)LOGPATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			        	m_pInstance->OpenDB((char*)LOGFILENAME, 0);
				}
			}
		}
	}

	return m_pInstance;
}

void SQLITE3MGR::DelSqlMgrItem()
{
	if(m_pInstance != NULL)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

void SQLITE3MGR::Lock()
{
	pthread_mutex_lock(&m_pMutex);
}

void SQLITE3MGR::UnLock()
{
	pthread_mutex_unlock(&m_pMutex);
}

int SQLITE3MGR::CreateTable(char *sSql)
{
    int iRet = 0;
    sqlite3_stmt *pStmt = NULL;

    iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, strlen(sSql), &pStmt, NULL);
	if(iRet != SQLITE_OK)
	{
		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
		LOGE("Create table sqlite prepare err : ret = %d sql:[%s]\n", iRet, sSql);
		sqlite3_close(m_pDBHandle);
		m_pDBHandle = NULL;
		return -1;
	}

	if(pStmt != NULL)
	{
		iRet = sqlite3_step(pStmt);
		if (SQLITE_DONE != iRet)
		{
			if(pStmt != NULL)
			{
				sqlite3_finalize(pStmt);
				pStmt = NULL;
			}
			LOGE("Create Table OPL failed : ret = %d sql:[%s]\n", iRet, sSql);
			sqlite3_close(m_pDBHandle);
			m_pDBHandle = NULL;
			return -1;
		}
		sqlite3_finalize(pStmt);
        pStmt = NULL;
	}

    return 0;
}

int SQLITE3MGR::GetLogCount(char *sTableName)
{
    int iRet = 0;
    sqlite3_stmt *pStmt = NULL;
	char sSql[128] = {0};

    snprintf(sSql, sizeof(sSql), "select count(*) from %s", sTableName);

    //LOGD("sSql:[%s] sTableName:[%s]\n",sSql,sTableName);
    iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, -1, &pStmt, NULL);
	//iRet |= sqlite3_bind_text(pStmt, 1, sTableName, -1, SQLITE_STATIC);

    if(iRet != SQLITE_OK)
    {
        if(pStmt != NULL)
        {
            sqlite3_finalize(pStmt);
            pStmt = NULL;
        }
        LOGE("GetLogCount prepare err : ret = %d sSql:[%s]\n", iRet, sSql);
        return -1;
    }

    if(pStmt != NULL)
    {
        iRet = sqlite3_step(pStmt);
        if (SQLITE_ROW != iRet)
		{
			LOGD("Selecet count : ret = %d\n", iRet);
			if(iRet == SQLITE_CORRUPT)
			{
				if(pStmt != NULL)
				{
					sqlite3_finalize(pStmt);
					pStmt = NULL;
				}
				sqlite3_close(m_pDBHandle);
				m_pDBHandle = NULL;
				return -1;
			}
		}

		iRet = sqlite3_column_int(pStmt, 0);
		sqlite3_finalize(pStmt);
        pStmt = NULL;
    }

    return iRet;
}

int SQLITE3MGR::DeleteLog(char *sTableName)
{
    int iRet = 0;
    sqlite3_stmt *pStmt = NULL;
	char sSql[128] = {0};
    char cmd[128] = {0};

    if(Common_StriCmp(sTableName, TABLE_ALARM) == 0)
    {
        //check image and delete
        snprintf(sSql, sizeof(sSql), "select [AlarmImagePath] from [%s] limit 0, 50", sTableName);
        iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, strlen(sSql), &pStmt, NULL);
        if (SQLITE_OK == iRet)
        {
            while(1)
            {
                iRet = sqlite3_step(pStmt);
                if (SQLITE_ROW == iRet)
                {
                    char *pPath = (char *)sqlite3_column_text(pStmt, 0);
                    LOGW("pPath:[%s]\n", pPath);

                    if(pPath && strlen(pPath)>0)
                    {
                        snprintf(cmd, sizeof(cmd),"rm -r %s/%s", ALARMPICPATH, pPath);
                        LOGD("cmd:[%s]\n", cmd);
                        Common_System(cmd);
                    }
                }
                else if (SQLITE_DONE == iRet)
                {
                    iRet = 0;
                    break;
                }
                else
                {
                    break;
                }
            }
        }

        sqlite3_finalize(pStmt);
        pStmt = NULL;
    }

    snprintf(sSql, sizeof(sSql), "delete from [%s] where [LogID] in (select [LogID] from [%s] limit 0, 50)", sTableName, sTableName);

    iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, strlen(sSql), &pStmt, NULL);
    if (SQLITE_OK == iRet)
    {
        iRet = sqlite3_step(pStmt);
    }

    sqlite3_finalize(pStmt);
    pStmt = NULL;

    return (iRet==SQLITE_DONE) ? 0 : -1;
}

int SQLITE3MGR::OpenDB(char *sDBFileName, DWORD dwSerial)
{
	int iRet;
	sqlite3_stmt *pStmt = NULL;

	char *sSqlCreateTable = (char*)"create table if not exists UsageLog (LogID integer primary key autoincrement, \
															LogTime interger, \
															MajorType interger, \
															MinorType interger, \
															UserName nvarchar(16), \
															HostAddr nvarchar(128), \
															Channel interger, \
															ExtraInfo nvarchar(128) \
															)";
	char *sSqlInsertLog = (char*)"insert into UsageLog (LogTime, MajorType, MinorType, UserName, HostAddr, Channel, ExtraInfo) values (?, ?, ?, ?, ?, ?, ?)";
	char *sSqlDeleteLog = (char*)"delete from [?] where [LogID] in (select [LogID] from [LOG] limit 0, 50)";

	char *sSqlCreateTableAlarm = (char*)"create table if not exists AlarmInfo (LogID integer primary key autoincrement, \
															AlarmType interger, \
															StartTime interger, \
															EndTime interger, \
															AlarmImagePath nvarchar(32), \
															ExtraInfo nvarchar(128) \
															)";
    char *sSqlInsertAlarm = (char*)"insert into AlarmInfo (AlarmType, StartTime, EndTime, AlarmImagePath, ExtraInfo) values (?, ?, ?, ?, ?)";

	if(NULL == sDBFileName)
	{
		LOGE("File name is NULL!\n");
		return -1;
	}

	if(m_pDBHandle != NULL)
	{
		sqlite3_close(m_pDBHandle);
		m_pDBHandle = NULL;
	}

	iRet = sqlite3_open(sDBFileName, &m_pDBHandle);
	if(iRet != SQLITE_OK)
	{
		LOGE("sqlite open failed : ret = %d\n", iRet);
		return -1;
	}
	if(m_pDeleteLogStmt != NULL)
	{
		sqlite3_finalize(m_pDeleteLogStmt);
		m_pDeleteLogStmt = NULL;
	}
	if(m_pInsertLogStmt != NULL)
	{
		sqlite3_finalize(m_pInsertLogStmt);
		m_pInsertLogStmt = NULL;
	}

	//Create DB table
	iRet = CreateTable(sSqlCreateTable);
    if(iRet!= 0)
    {
        return -1;
    }

    iRet = CreateTable(sSqlCreateTableAlarm);
    if(iRet!= 0)
    {
        return -1;
    }

    m_lLogCount = GetLogCount(TABLE_LOG);
    m_lAlarmCount = GetLogCount(TABLE_ALARM);

    LOGD("m_lLogCount:[%d] m_lAlarmCount:[%d]\n",m_lLogCount, m_lAlarmCount);

	/*iRet = sqlite3_prepare_v2(m_pDBHandle, sSqlDeleteLog, strlen(sSqlDeleteLog), &m_pDeleteLogStmt, NULL);
	if (SQLITE_OK != iRet)
	{
		LOGE("Delete count prepare err : ret = %d\n",iRet);
		m_pDeleteLogStmt = NULL;
	}*/

	iRet = sqlite3_prepare_v2(m_pDBHandle, sSqlInsertLog, strlen(sSqlInsertLog), &m_pInsertLogStmt, NULL);
	if (SQLITE_OK != iRet)
	{
		LOGE("Insert prepare err : ret = %d\n",iRet);
		m_pInsertLogStmt = NULL;
	}

	iRet = sqlite3_prepare_v2(m_pDBHandle, sSqlInsertAlarm, strlen(sSqlInsertAlarm), &m_pInsertAlarmStmt, NULL);
	if (SQLITE_OK != iRet)
	{
		LOGE("Insert prepare err : ret = %d\n",iRet);
		m_pInsertAlarmStmt = NULL;
	}

	m_bOpen = TRUE;
	m_lQureyCount = 0x7FFFFFFF;

	return 0;
}

BOOL SQLITE3MGR::CloseDB()
{
	if(!m_bOpen)
		return FALSE;
	if(m_pDBHandle != NULL)
	{
		sqlite3_close(m_pDBHandle);
		m_pDBHandle = NULL;
	}
	return TRUE;
}

int SQLITE3MGR::InsertLog(OVFS_LOG strLog)
{
	int	iRet = 0;

	if(!m_bOpen)
		return -1;

	if (m_lLogCount >= MAX_LOG_COUNT)
	{
		iRet = DeleteLog(TABLE_LOG);
		if (iRet == 0)
		{
			m_lLogCount -= 50;
		}
	}

	//sqlite3_reset(m_pInsertLogStmt);
	//sqlite3_clear_bindings(m_pInsertLogStmt);

	iRet = sqlite3_bind_int(m_pInsertLogStmt, 1, strLog.dwLogTime);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set first  err : lRet = %d\n",iRet);
	}

	iRet = sqlite3_bind_int(m_pInsertLogStmt, 2, strLog.dwMajorType);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set second  err : lRet = %d\n",iRet);
	}

    iRet = sqlite3_bind_int(m_pInsertLogStmt, 3, strLog.dwMinorType);
        if (SQLITE_OK != iRet)
        {
            LOGE("Set 3th  err : lRet = %d\n",iRet);
        }

	iRet = sqlite3_bind_text(m_pInsertLogStmt, 4, (char*)strLog.sUser, -1, SQLITE_STATIC);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set 4th  err : lRet = %d\n",iRet);
	}

	iRet = sqlite3_bind_text(m_pInsertLogStmt, 5, (char*)strLog.sIP, -1, SQLITE_STATIC);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set 5th  err : lRet = %d\n",iRet);
	}

	iRet = sqlite3_bind_int(m_pInsertLogStmt, 6, strLog.dwChannel);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set 6th  err : lRet = %d\n",iRet);
	}

	iRet = sqlite3_bind_text(m_pInsertLogStmt, 7, (char*)strLog.sExtraInfo, -1, SQLITE_STATIC);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set 7th  err : lRet = %d\n",iRet);
	}

	Lock();
	//执行插入语句
	iRet = sqlite3_step(m_pInsertLogStmt);
	UnLock();

	if (SQLITE_DONE == iRet)
	{
		iRet = 0;
		//TRACEINFO("Insert Log, Affected Rows = %d\n", nResult);
	}
	else
	{
		LOGE("Insert Log Failed, lRet = %d", iRet);
	}

	//提交到数据库
	sqlite3_reset(m_pInsertLogStmt);
	m_lLogCount++;


	return iRet;
}

int SQLITE3MGR::InsertAlarmInfo(OVFS_ALARMNFO strInfo, int *iId)
{
	int		iRet = 0;
    char path[128]={0};
    //LOGD("InsertAlarmInfo\n");
	if(!m_bOpen)
		return -1;


	if (m_lAlarmCount >= MAX_ALARM_COUNT)
	{
		iRet = DeleteLog(TABLE_ALARM);
		if (0 == iRet)
		{
			m_lAlarmCount -= 50;
		}
	}

    //sqlite3_reset(m_pInsertAlarmStmt);
	//sqlite3_clear_bindings(m_pInsertAlarmStmt);

	iRet = sqlite3_bind_int(m_pInsertAlarmStmt, 1, strInfo.dwAlarmType);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set first  err : lRet = %d\n",iRet);
	}

	iRet = sqlite3_bind_int(m_pInsertAlarmStmt, 2, strInfo.dwStartTime);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set second  err : lRet = %d\n",iRet);
	}

	iRet = sqlite3_bind_int(m_pInsertAlarmStmt, 3, strInfo.dwEndTime);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set third  err : lRet = %d\n",iRet);
	}

    if(strlen(strInfo.sAlarmPic) > 0)
    {
        snprintf(path, sizeof(path),"%d_%d_%d.jpg", strInfo.dwAlarmType,strInfo.dwStartTime, m_lAlarmCount);
        LOGD("path:[%s]\n", path);
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd),"cp -r %s %s/%s", strInfo.sAlarmPic, ALARMPICPATH, path);
        LOGD("cmd:[%s]\n", cmd);
        Common_System(cmd);
    }
	iRet = sqlite3_bind_text(m_pInsertAlarmStmt, 4, (char*)path, -1, SQLITE_STATIC);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set 4th  err : lRet = %d\n",iRet);
	}

    iRet = sqlite3_bind_text(m_pInsertAlarmStmt, 5, strInfo.sExtraInfo, -1, SQLITE_STATIC);
	if (SQLITE_OK != iRet)
	{
		LOGE("Set 5th  err : lRet = %d\n",iRet);
	}

	Lock();
	//执行插入语句
	iRet = sqlite3_step(m_pInsertAlarmStmt);
    int last_id = sqlite3_last_insert_rowid(m_pDBHandle);
	UnLock();
    LOGD("iRet:[%d] last_id:[%d]\n",iRet,last_id);
	if (SQLITE_DONE == iRet)
	{
        iRet = 0;
        *iId = last_id;
		//bResult = ((sqlite3_changes(m_pDBHandle) == 1) ? TRUE : FALSE);
		//TRACEINFO("Insert Log, Affected Rows = %d\n", nResult);
	}
	else
	{
		LOGE("Insert Log Failed, lRet = %d", iRet);
		iRet = -1;
	}

	//提交到数据库
	sqlite3_reset(m_pInsertAlarmStmt);
	m_lAlarmCount++;


	return iRet;
}

int SQLITE3MGR::UpdateAlarmInfo(OVFS_ALARMNFO strInfo)
{
	int		iRet = 0;
	sqlite3_stmt *pStmt = NULL;

    char sSql[128] = {0};

    snprintf(sSql, sizeof(sSql),"update AlarmInfo set EndTime=%d where LogId=%d",strInfo.dwEndTime,strInfo.dwId);

    //LOGD("sSql:[%s]\n",sSql);

	if(!m_bOpen)
		return -1;

	Lock();

    iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, -1, &pStmt, NULL);
    if(iRet != SQLITE_OK)
    {
        if(pStmt != NULL)
        {
            sqlite3_finalize(pStmt);
            pStmt = NULL;
        }
        LOGE("UpdateAlarmInfo prepare err : ret = %d\n", iRet);
        return -1;
    }

    if(pStmt != NULL)
    {
        iRet = sqlite3_step(pStmt);
        //LOGW("iRet = %d\n",iRet);
        if(iRet == SQLITE_DONE)
        {
            iRet = 0;
        }

		sqlite3_finalize(pStmt);
        pStmt = NULL;
    }

	UnLock();

	return iRet;
}

/*LONG SQLITE3MGR::QueryStart(LONG lMode, DWORD dwMajorType, DWORD dwMinorType, time_t tStartTime, time_t tStopTime,int nTry)
{
	sqlite3_stmt *pStmt = NULL, *m_pGetQueryCountStmt = NULL;
	char *szSqlCreateTmpTable;
	char *szTableName;
	LONG lParamIndex = 1;
	LONG ret = -1;
	char *pTmpMem = NULL;
	LONG lQureyCount_Curr = -1;
	LOGD("Enter lMode = %d dwMajorType = %d dwMinorType = %d tStartTime= %d tStopTime= %d\n",lMode,dwMajorType,dwMinorType,(int)tStartTime,(int)tStopTime);

	if(!bOpen)
		return -1;
	if(Shutdown)
		return -1;

	pTmpMem = new char [1024];
	if(pTmpMem == NULL)
	{

		return -1;
	}
	szSqlCreateTmpTable = pTmpMem;
	szTableName = pTmpMem + 512;
	QueryParam *hQuery = new QueryParam;
	if (NULL == hQuery)
	{

		if(pTmpMem != NULL)
		{
			delete []pTmpMem;
			pTmpMem = NULL;
		}
		return -1;
	}
	hQuery->pData = NULL;

	pthread_mutex_lock(&pQueryMutex);

	lQureyCount --;
	if(lQureyCount <= 0)
		lQureyCount = 0x7FFFFFFF;
	lQureyCount_Curr = lQureyCount;
	pthread_mutex_unlock(&pQueryMutex);

    Lock();
	sprintf(szTableName, "LOGTMP%u", lQureyCount_Curr);
	if(QUERY_ALL == lMode)
	{
		sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] ORDER BY [LogID] DESC", szTableName);
		ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, strlen(szSqlCreateTmpTable), &pStmt, NULL);
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
	}
	else if(QUERY_BY_TYPE == lMode)
	{
		if(dwMajorType == 0)
		{
			sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] ORDER BY [LogID] DESC", szTableName);
			ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, strlen(szSqlCreateTmpTable), &pStmt, NULL);
			TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
		}
		else
		{
			if(dwMinorType == 0)
			{
				sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] WHERE [EventType] = ? ORDER BY [LogID] DESC", szTableName);
				ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, -1, &pStmt, NULL);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
				ret |= sqlite3_bind_int(pStmt, 1, dwMajorType);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
			}
			else
			{
				sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] WHERE [EventType] = ? AND [MinorType] = ? ORDER BY [LogID] DESC", szTableName);
				ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, -1, &pStmt, NULL);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
				ret |= sqlite3_bind_int(pStmt, 1, dwMajorType);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
				ret |= sqlite3_bind_int(pStmt, 2, dwMinorType);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
			}
		}
	}
	else if(QUERY_BY_TIME == lMode)
	{
		sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] WHERE [EventTime] >= ? AND [EventTime] <= ? ORDER BY [LogID] DESC", szTableName);
		ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, -1, &pStmt, NULL);
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);

		ret |= sqlite3_bind_int(pStmt, 1, (int)tStartTime);
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
		ret |= sqlite3_bind_int(pStmt, 2, (int)tStopTime);
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
	}
	else if(QUERY_BY_TYPE_AND_TIME == lMode)
	{
		if(dwMajorType == 0)
		{
			sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] WHERE [EventTime] >= ? AND [EventTime] <= ? ORDER BY [LogID] DESC", szTableName);
			ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, -1, &pStmt, NULL);
			TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
		}
		else
		{
			if(dwMinorType == 0)
			{
				sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] WHERE [EventType] = ? AND [EventTime] >= ? AND [EventTime] <= ? ORDER BY [LogID] DESC", szTableName);
				ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, -1, &pStmt, NULL);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);

				ret |= sqlite3_bind_int(pStmt, lParamIndex, dwMajorType);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
				lParamIndex ++;
			}
			else
			{
				sprintf(szSqlCreateTmpTable, "CREATE TEMP TABLE [%s] AS SELECT * FROM [LOG] WHERE [EventType] = ? AND [MinorType] = ? AND [EventTime] >= ? AND [EventTime] <= ?  ORDER BY [LogID] DESC", szTableName);
				ret = sqlite3_prepare_v2(pDBHandle, szSqlCreateTmpTable, -1, &pStmt, NULL);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);

				ret |= sqlite3_bind_int(pStmt, lParamIndex, dwMajorType);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
				lParamIndex ++;
				ret |= sqlite3_bind_int(pStmt, lParamIndex, dwMinorType);
				TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
				lParamIndex ++;
			}
		}

		ret |= sqlite3_bind_int(pStmt, lParamIndex, (int)tStartTime);
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
		lParamIndex ++;
		ret |= sqlite3_bind_int(pStmt, lParamIndex, (int)tStopTime);
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szSqlCreateTmpTable,ret);
	}
	else
	{
		TRACEDEBUG_LOG("here [%s] ret = %d\n",szTableName,ret);
		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
		UnLock();
		TRACEERROR_LOG("QueryMode is Invalid\n");
		if(hQuery->pData != NULL)
		{
			ret = sqlite3_finalize((sqlite3_stmt *)(hQuery->pData));
			hQuery->pData = NULL;
		}
		delete hQuery;

		if(pTmpMem != NULL)
		{
			delete []pTmpMem;
			pTmpMem = NULL;
		}
		return -1;
	}

	if(ret != SQLITE_OK)
	{
		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
		UnLock();
		TRACEERROR_LOG("Bind Parameter failed\n");
		if(hQuery->pData != NULL)
		{
			ret = sqlite3_finalize((sqlite3_stmt *)(hQuery->pData));
			hQuery->pData = NULL;
		}
		delete hQuery;
		if(pTmpMem != NULL)
		{
			delete []pTmpMem;
			pTmpMem = NULL;
		}
		return -1;
	}
	ret = sqlite3_step(pStmt);
	if (ret != SQLITE_DONE)
	{

		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
		TRACEERROR_LOG("Create Tmp Table, ret = %d\n", ret);
		if(hQuery->pData != NULL)
		{
			ret = sqlite3_finalize((sqlite3_stmt *)(hQuery->pData));
			hQuery->pData = NULL;
		}
		delete hQuery;
			if(pTmpMem != NULL)
			{
				delete []pTmpMem;
				pTmpMem = NULL;
			}
			if(nTry)
			{
				UnLock();
				return -1;
			}

			UnLock();
			return -1;
	}
	if(pStmt != NULL)
	{
		sqlite3_finalize(pStmt);
		pStmt = NULL;
	}

	char szSqlGetQueryCount[256] = "";
	sprintf(szSqlGetQueryCount, "SELECT COUNT(*) FROM [%s]", szTableName);
	TRACEERROR_LOG("here \n");
	ret = sqlite3_prepare_v2(m_pDBHandle, szSqlGetQueryCount, strlen(szSqlGetQueryCount), &m_pGetQueryCountStmt, NULL);
	TRACEERROR_LOG("here \n");
	if ((SQLITE_OK == ret) && (SQLITE_ROW == sqlite3_step(m_pGetQueryCountStmt)))
	{
		hQuery->lTotalCnt = sqlite3_column_int(m_pGetQueryCountStmt, 0);
		hQuery->lCount = 0;
		if (hQuery->lTotalCnt > 0)
		{
			char szSqlGetLogFromTmp[256] = "";
			sprintf(szSqlGetLogFromTmp, "SELECT * FROM [%s] LIMIT ?, ?", szTableName);
			TRACEERROR_LOG("here \n");
			ret = sqlite3_prepare_v2(pDBHandle, szSqlGetLogFromTmp, strlen(szSqlGetLogFromTmp), (sqlite3_stmt **)&(hQuery->pData), NULL);
		}
	}
	else
	{
		UnLock();
		TRACEERROR_LOG("here \n");
		if(m_pGetQueryCountStmt != NULL)
		{
			sqlite3_finalize(m_pGetQueryCountStmt);
			m_pGetQueryCountStmt = NULL;
		}
		if(hQuery->pData != NULL)
		{
			ret = sqlite3_finalize((sqlite3_stmt *)(hQuery->pData));
			hQuery->pData = NULL;
		}
		delete hQuery;
		TRACEERROR_LOG("GetQueryCount, ret = %d\n", ret);
		if(pTmpMem != NULL)
		{
			delete []pTmpMem;
			pTmpMem = NULL;
		}
		return -1;
	}
	TRACEERROR_LOG("here \n");
	sqlite3_finalize(m_pGetQueryCountStmt);
	UnLock();

	QureyInfo.insert(pair<LONG, LPQueryParam>(lQureyCount_Curr, hQuery));


	if(pTmpMem != NULL)
	{
		delete []pTmpMem;
		pTmpMem = NULL;
	}
	TRACEDEBUG_LOG("Leave\n");

	return lQureyCount_Curr;
}*/

/*LONG SQLITE3MGR::GetLogs(LONG lHandle, POVFS_LOG pLogBuf)
{
	LogSeachMap::iterator	iter;
	LONG lRet = 0;
	LONG lReturnValue = ANTS_DVR_FILE_SUCCESS;
	QueryParam *pQuery;
	sqlite3_stmt *pStmt;

	if(!bOpen)
		return -1;
	if(Shutdown)
		return -1;

	iter = m_QureyInfo.find(lHandle);
	if(iter == m_QureyInfo.end())
	{
		LOGE("Handle not exist");
		return -1;
	}
	pQuery = iter->second;
	pStmt = (sqlite3_stmt *)(pQuery->pData);

	if (pQuery->lCount >= pQuery->lTotalCnt)
	{
		if(pQuery->lTotalCnt == 0)
			return ANTS_DVR_FILE_NOFIND;
		else
			return ANTS_DVR_NOMOREFILE;
	}

	if (NULL == pStmt)
	{
		return ANTS_DVR_FILE_EXCEPTION;
	}

	Lock();

	lRet = sqlite3_bind_int(pStmt, 1, pQuery->lCount);
	lRet |= sqlite3_bind_int(pStmt, 2, 1);
	if(lRet != SQLITE_OK)
		TRACEERROR_LOG("sqlite3_bind_int err : ret = %d", lRet);

	lRet = sqlite3_step(pStmt);
	if (SQLITE_ROW == lRet)
	{
		pLogBuf->dwMajorType = sqlite3_column_int(pStmt, 1);
		pLogBuf->dwMinorType = sqlite3_column_int(pStmt, 2);
		strcpy(pLogBuf->sUser, reinterpret_cast<const char *>(sqlite3_column_text(pStmt, 3)));
		strcpy(pLogBuf->sRemoteHostAddr, reinterpret_cast<const char *>(sqlite3_column_text(pStmt, 4)));
		pLogBuf->dwLogTime = sqlite3_column_int(pStmt, 5);
		pLogBuf->dwChannel = sqlite3_column_int(pStmt, 6);
		pLogBuf->dwDiskNum = sqlite3_column_int(pStmt, 7);
		pLogBuf->dwAlarmInPort = sqlite3_column_int(pStmt, 8);
		pLogBuf->dwAlarmOutPort = sqlite3_column_int(pStmt, 9);
		pLogBuf->bStatus = sqlite3_column_int(pStmt, 10);
		pQuery->lCount ++;
	}
	else if (SQLITE_DONE == lRet)
	{
		TRACEINFO_LOG("End Select\n");
		lReturnValue = ANTS_DVR_NOMOREFILE;
	}
	else
	{
		TRACEERROR_LOG("Select Stmt Failed lRet = %x\n",lRet);
		lReturnValue = ANTS_DVR_FILE_EXCEPTION;
	}

	sqlite3_reset(pStmt);
	sqlite3_clear_bindings(pStmt);

	UnLock();

	return lReturnValue;
}*/

BOOL SQLITE3MGR::GetQueryLogsCnt(LONG lHandle, LONG *lplGetCnt)
{
	LogSeachMap::iterator	iter;
	QueryParam *pQuery;

	if(!m_bOpen)
		return FALSE;

	iter = m_QureyInfo.find(lHandle);
	if(iter == m_QureyInfo.end())
	{
		LOGE("Handle not exist\n");
		return FALSE;
	}
	pQuery = iter->second;

	*lplGetCnt = pQuery->lTotalCnt;

	return TRUE;
}

/*LONG SQLITE3MGR::GetLogsEX(LONG lHandle, POVFS_LOG pLogBuf, LONG lStartCnt, LONG lGetCnt)
{
	LogSeachMap::iterator	iter;
	LONG lRet = 0;
	QueryParam *pQuery;
	sqlite3_stmt *pStmt;
	LONG lLogCountTmp = 0;

	if(!bOpen)
		return -1;
	if(Shutdown)
		return -1;

	iter = QureyInfo.find(lHandle);
	if(iter == QureyInfo.end())
	{
		TRACEERROR_LOG("Handle not exist");
		return -1;
	}
	pQuery = iter->second;
	pStmt = (sqlite3_stmt *)(pQuery->pData);

	if (lStartCnt >= pQuery->lTotalCnt)
	{
		if(pQuery->lTotalCnt == 0)
			return 0;
		else
			return 0;
	}

	if (NULL == pStmt)
	{
		return -1;
	}

	Lock();

	lRet = sqlite3_bind_int(pStmt, 1, lStartCnt);
	lRet |= sqlite3_bind_int(pStmt, 2, lGetCnt);
	if(lRet != SQLITE_OK)
		TRACEERROR_LOG("sqlite3_bind_int err : ret = %d", lRet);

	while(1)
	{
		if((lStartCnt + lLogCountTmp) >= pQuery->lTotalCnt)
		{
			break;
		}
		lRet = sqlite3_step(pStmt);
		if (SQLITE_ROW == lRet)
		{
			pLogBuf[lLogCountTmp].dwMajorType = sqlite3_column_int(pStmt, 1);
			pLogBuf[lLogCountTmp].dwMinorType = sqlite3_column_int(pStmt, 2);
			strcpy(pLogBuf[lLogCountTmp].sUser, reinterpret_cast<const char *>(sqlite3_column_text(pStmt, 3)));
			strcpy(pLogBuf[lLogCountTmp].sRemoteHostAddr, reinterpret_cast<const char *>(sqlite3_column_text(pStmt, 4)));
			pLogBuf[lLogCountTmp].dwLogTime = sqlite3_column_int(pStmt, 5);
			pLogBuf[lLogCountTmp].dwChannel = sqlite3_column_int(pStmt, 6);
			pLogBuf[lLogCountTmp].dwDiskNum = sqlite3_column_int(pStmt, 7);
			pLogBuf[lLogCountTmp].dwAlarmInPort = sqlite3_column_int(pStmt, 8);
			pLogBuf[lLogCountTmp].dwAlarmOutPort = sqlite3_column_int(pStmt, 9);
			pLogBuf[lLogCountTmp].bStatus = sqlite3_column_int(pStmt, 10);
			lLogCountTmp ++;
		}
		else if (SQLITE_DONE == lRet)
		{
			TRACEINFO_LOG("End Select\n");
			break;
		}
		else
		{
			TRACEERROR_LOG("Select Stmt Failed %x\n",lRet);
			break;
		}
	}

	sqlite3_reset(pStmt);
	sqlite3_clear_bindings(pStmt);

	UnLock();

	return lLogCountTmp;
}*/

int SQLITE3MGR::QueryStop(int iLogType, int lHandle)
{
	LogSeachMap::iterator	iter;
	QueryParam *pQuery;
	int iRet;
	sqlite3_stmt *pStmt = NULL;
    char sSql[128] = {0};

    LOGD("QueryStop\n");
	if(!m_bOpen)
		return -1;

	iter = m_QureyInfo.find(lHandle);
	if(iter == m_QureyInfo.end())
	{
		LOGE("Handle not exist! lHandle:[%d] size:[%d]\n", lHandle,m_QureyInfo.size());
		return -1;
	}
	pQuery = iter->second;
    if(iLogType != pQuery->iLogType)
    {
        LOGE("iLogType not match![%d][%d]\n",iLogType, pQuery->iLogType);
        return -1;
    }

	snprintf(sSql, sizeof(sSql), "DROP TABLE [%s]", pQuery->pTableName);
    //LOGD("sSql:[%s]\n", sSql);
	iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, strlen(sSql), &pStmt, NULL);
    //LOGD("iRet:[%d]\n", iRet);
	if(iRet != SQLITE_OK)
		LOGE("sqlite3_prepare_v2 err : lRet = %d", iRet);
	if(pStmt != NULL)
	{
        Lock();
		iRet = sqlite3_step(pStmt);
        UnLock();
        //LOGD("lRet:[%d]\n",iRet);
		if(iRet == SQLITE_DONE)
		{
            iRet = 0;
        }

		sqlite3_finalize(pStmt);
        pStmt = NULL;
	}

    if(pQuery->pTableName != NULL)
    {
        Common_Free(pQuery->pTableName, __FUNCTION__, __LINE__);
        pQuery->pTableName = NULL;
    }

	m_QureyInfo.erase(iter);
	delete pQuery;


	return 0;
}

LONG SQLITE3MGR::QueryStart(int iLogType, int dwType, int tStartTime, int tStopTime, int iSort)
{
	int ret = -1;
	int lQureyCount_Curr = -1;
	sqlite3_stmt *pStmt = NULL;
	char szSql[256] = {0};
	char szTableName[32] = {0};
    const char *sOriginTable = iLogType ? TABLE_ALARM : TABLE_LOG;

	if(!m_bOpen)
		return -1;

	QueryParam *hQuery = new QueryParam;
	if (NULL == hQuery)
	{
		return -1;
	}

	pthread_mutex_lock(&m_pQueryMutex);
	m_lQureyCount --;
	if(m_lQureyCount <= 0)
		m_lQureyCount = 0x7FFFFFFF;
	lQureyCount_Curr = m_lQureyCount;
	pthread_mutex_unlock(&m_pQueryMutex);

    LOGD("lQureyCount_Curr:[%d]\n",lQureyCount_Curr);
	snprintf(szTableName, sizeof(szTableName), "%s_TMP%u", sOriginTable, lQureyCount_Curr);

    LOGD("szTableName:[%s]\n",szTableName);

    hQuery->iLogType = iLogType;
    hQuery->pTableName = Common_StrDup(szTableName, __FUNCTION__, __LINE__);

    if(iLogType)
    {
        snprintf(szSql, sizeof(szSql),
            "CREATE TEMP TABLE [%s] AS SELECT * FROM [%s] WHERE [StartTime] >= %d AND [StartTime] <= %d",
            szTableName, sOriginTable, tStartTime, tStopTime);

    	if(-1 != dwType)//query all
    	{
            snprintf(szSql+strlen(szSql),sizeof(szSql)," AND [AlarmType] = %d", dwType);
        }
    }
    else
    {
        snprintf(szSql, sizeof(szSql),
                    "CREATE TEMP TABLE [%s] AS SELECT * FROM [%s] WHERE [LogTime] >= %d AND [LogTime] <= %d",
                    szTableName, sOriginTable, tStartTime, tStopTime);

        if(0 != dwType)//query all
        {
            snprintf(szSql+strlen(szSql),sizeof(szSql)," AND [MajorType] = %d", dwType);
        }
    }

    snprintf(szSql+strlen(szSql),sizeof(szSql)," ORDER BY [LogId] %s", iSort?"DESC":"");

    LOGW("sql len:[%d]\n", strlen(szSql));

    ret = sqlite3_prepare_v2(m_pDBHandle, szSql, -1, &pStmt, NULL);

    //LOGD("here [%s] ret = %d\n",szSql,ret);
	if(ret != SQLITE_OK)
	{
		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
		UnLock();
		LOGE("Bind Parameter failed\n");

		delete hQuery;

		return -1;
	}

    Lock();
	ret = sqlite3_step(pStmt);
	UnLock();

	if (ret != SQLITE_DONE)
	{

		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
		LOGD("Create Tmp Table, ret = %d\n", ret);

		delete hQuery;
		UnLock();
		return -1;
	}

	if(pStmt != NULL)
	{
		sqlite3_finalize(pStmt);
		pStmt = NULL;
	}
    //LOGD("szTableName:[%s]\n",szTableName);

    ret = GetLogCount(hQuery->pTableName);
    LOGD("GetQueryCount, ret = %d\n", ret);
    if(ret >= 0)
    {
        hQuery->lTotalCnt = ret;
    }
    else
    {
        if(hQuery->pTableName != NULL)
		{
			Common_Free(hQuery->pTableName, __FUNCTION__, __LINE__);
			hQuery->pTableName = NULL;
		}
		delete hQuery;
		return -1;
    }

	m_QureyInfo.insert(pair<LONG, LPQueryParam>(lQureyCount_Curr, hQuery));

	//LOGD("Leave\n");

	return lQureyCount_Curr;
}

int SQLITE3MGR::GetQueryList(int iLogType, LONG iHandle, int iStart, int iCount, void **pList, int *iListCount)
{
    int iRet = 0;
    int iIndex = 0;
    char sSql[128] = {0};
    char *pStr = NULL;
    LogSeachMap::iterator   iter;
    QueryParam *pQuery;
    sqlite3_stmt *pStmt;
    OVFS_ALARMNFO *pInfoList = NULL;
    OVFS_LOG *pLogList = NULL;
    //LOGD("GetQueryAlarmList\n");

    if(!m_bOpen)
        return -1;

    iter = m_QureyInfo.find(iHandle);
    if(iter == m_QureyInfo.end())
    {
        LOGE("Handle not exist\n");
        return -1;
    }
    pQuery = iter->second;

    if(iLogType != pQuery->iLogType)
    {
        LOGE("iLogType not match![%d][%d]\n",iLogType, pQuery->iLogType);
        return -1;
    }

    snprintf(sSql, sizeof(sSql), "SELECT * FROM [%s] LIMIT %d, %d", pQuery->pTableName, iStart, iCount);
    //LOGD("sSql:[%s]\n", sSql);
    iRet = sqlite3_prepare_v2(m_pDBHandle, sSql, strlen(sSql), &pStmt, NULL);
    //LOGD("iRet:[%d]\n", iRet);
    if(iRet != SQLITE_OK)
	{
		if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
    }

    int iInfoCount = iCount;
    if(iStart + iCount > pQuery->lTotalCnt)
    {
        iInfoCount = pQuery->lTotalCnt - iStart;
    }
    LOGD("iInfoCount:[%d] lTotalCnt:[%d]\n", iInfoCount,pQuery->lTotalCnt);

    if(pQuery->iLogType)
    {
        pInfoList = (OVFS_ALARMNFO*)calloc(iInfoCount, sizeof(OVFS_ALARMNFO));

    }
    else
    {
        pLogList = (OVFS_LOG*)calloc(iInfoCount, sizeof(OVFS_LOG));
    }

    if(pInfoList == NULL && pLogList == NULL)
    {
        LOGE("[%d] calloc failed!\n",pQuery->iLogType);
        if(pStmt != NULL)
		{
			sqlite3_finalize(pStmt);
			pStmt = NULL;
		}
        return -1;
    }

    Lock();
    while(1)
    {
        iRet = sqlite3_step(pStmt);
        //LOGD("iRet:[%d]\n", iRet);
        if (SQLITE_ROW == iRet)
        {
            if(pQuery->iLogType)
            {
                pInfoList[iIndex].dwId = sqlite3_column_int(pStmt, 0);
                pInfoList[iIndex].dwAlarmType = sqlite3_column_int(pStmt, 1);
                pInfoList[iIndex].dwStartTime = sqlite3_column_int(pStmt, 2);
                pInfoList[iIndex].dwEndTime = sqlite3_column_int(pStmt, 3);
                pStr = (char *)sqlite3_column_text(pStmt, 4);
                if(pStr)
                {
                    strncpy(pInfoList[iIndex].sAlarmPic, reinterpret_cast<const char *>(sqlite3_column_text(pStmt, 4)), MAX_NAMELEN);
                }
                pStr = NULL;
                pStr = (char *)sqlite3_column_text(pStmt, 5);
                if(pStr)
                {
                    strncpy(pInfoList[iIndex].sExtraInfo, pStr, MAX_HOSTLEN);
                }
            }
            else
            {
                pLogList[iIndex].dwId = sqlite3_column_int(pStmt, 0);
                pLogList[iIndex].dwLogTime = sqlite3_column_int(pStmt, 1);
                pLogList[iIndex].dwMajorType = sqlite3_column_int(pStmt, 2);
                pLogList[iIndex].dwMinorType = sqlite3_column_int(pStmt, 3);
                pStr = NULL;
                pStr = (char *)sqlite3_column_text(pStmt, 4);
                if(pStr)
                {
                    strncpy(pLogList[iIndex].sUser, pStr, MAX_NAMELEN);
                }
                pStr = NULL;
                pStr = (char *)sqlite3_column_text(pStmt, 5);
                if(pStr)
                {
                    strncpy(pLogList[iIndex].sIP, pStr, MAX_HOSTLEN);
                }
                pLogList[iIndex].dwChannel = sqlite3_column_int(pStmt, 6);
                pStr = NULL;
                pStr = (char *)sqlite3_column_text(pStmt, 7);
                if(pStr)
                {
                    strncpy(pLogList[iIndex].sExtraInfo, pStr, MAX_HOSTLEN);
                }
            }
            iIndex ++;
        }
        else if (SQLITE_DONE == iRet)
        {
            //LOGD("End Select\n");
            iRet = 0;
            break;
        }
        else
        {
            LOGE("Select Stmt Failed %d\n",iRet);
            break;
        }
    }
    UnLock();

    if(iRet == 0)
    {
        if(pQuery->iLogType)
        {
            *pList = pInfoList;
        }
        else
        {
            *pList = pLogList;
        }

        *iListCount = iInfoCount;
    }
    else
    {
        if(pLogList)
        {
            free(pLogList);
        }
        if(pInfoList)
        {
            free(pInfoList);
        }
    }

    return iRet;
}

SQLITE3MGR::SQLITE3MGR()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);

	pthread_mutex_init(&m_pMutex, &attr);
	pthread_mutex_init(&m_pQueryMutex, &attr);
	m_bOpen = FALSE;
	m_pDBHandle = NULL;
	m_bBusy = 0;
	m_pDeleteLogStmt = NULL;
	m_pInsertLogStmt = NULL;
    m_pInsertAlarmStmt = NULL;
}

SQLITE3MGR::~SQLITE3MGR()
{
	if(m_bOpen)
	{
		CloseDB();
		m_bOpen = FALSE;
	}
	m_bBusy = 0;
	pthread_mutex_destroy(&m_pMutex);
	pthread_mutex_destroy(&m_pQueryMutex);
}
