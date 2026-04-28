#include <stdio.h>
#include "libcommon_api.h"
#include "libcommon_struct.h"
#include "cjson.h"
//////////////////////////////////////////////////////////////////////////

static Common_Log_T g_hCommonMicroxxxLog = NULL;
Common_Log_T Common_Log_GetDefaultHandle()
{
	if (g_hCommonMicroxxxLog == NULL)
	{// 当前进程名
		S8 szName[128];
		sprintf(szName,"Default");
		Common_GetSelfExeName(szName,128);
		Common_Log_Create(&g_hCommonMicroxxxLog,szName,COMMON_LOG_LV_BASE);
	}
	return g_hCommonMicroxxxLog;
}
void Common_Log_SetDefaultHandle(Common_Log_T hLog)
{
	if (g_hCommonMicroxxxLog != NULL)
	{
		Common_Log_Destroy(&g_hCommonMicroxxxLog);
	}
	g_hCommonMicroxxxLog = hLog;
}
//////////////////////////////////////////////////////////////////////////


