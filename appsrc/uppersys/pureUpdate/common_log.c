
#include "libcommon_struct.h"
#include "libcommon_api.h"


#define DEBUG_FIFO_PATH_MAX         64
#define DEBUG_FIFO_PATH             "/tmp/"

#define NONE                        "\e[0m"
#define RED                         "\e[0;31m"
#define YELLOW                      "\e[0;33m"
#define CYAN                        "\e[0;36m"

#define DUMP_SYSTEM_INFO_INTERVAL   120

#define LOG_BUFFER_SIZE             1024
#define LOG_TAG_SIZE                32
#define LOG_HEAD_SIZE_MAX           128

/*创建log时，优先检查是否存在以下文件。
  如果存在，则使用以下log级别，
  如果不存在，则使用创建log时传入的级别。
  动态调整进程log级别，可以通过echo ［1-3］ > /tmp/［APP］
  例如：echo 3 > /tmp/MediaServer
  将ovfs_mediaserver LOG输出级别改为最高
*/
#define LOG_DEFAULT_LEVEL_BASE      "/tmp/loglv0"
#define LOG_DEFAULT_LEVEL_LOW       "/tmp/loglv1"
#define LOG_DEFAULT_LEVEL_MID       "/tmp/loglv2"
#define LOG_DEFAULT_LEVEL_HIGH      "/tmp/loglv3"


typedef struct
{
    S32 init;//
    U32 level;//LOG输出级别

    S8 tag[LOG_TAG_SIZE];
    S8 debugFifoFile[DEBUG_FIFO_PATH_MAX];
    int debugFifoFd;
    S8 logBuffer[LOG_BUFFER_SIZE];//LOG输出缓存

    S32 baseTime;//记录程序启动时的时间，对每条LOGo计算已经运行时间
    Common_Lock_T lockMutex;
    Common_Thread_T systemDumpPth;
} LOG_CONTEXT_T;

// static int GetCpuJiffy(int id)
// {
// #ifndef WIN32
//     char statStr[256] = "";
//     char fname[128] = "";
//     char processName[64] = "";
//     char state = 0;
//     char tcpu[32] = "";
//     int user = 0, nice = 0, sys = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
//     int ppid = 0, pgrp = 0, session = 0, tty = 0, tpgid = 0, utime = 0, stime = 0, cutime = 0, cstime = 0, counter = 0,
//             priority = 0, signal = 0, blocked = 0, sigignore = 0, sigcatch = 0, starttime = 0;
//     unsigned int flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0, timeout = 0, itrealvalue = 0, vsize = 0,
//             rss = 0, rlim = 0, startcode = 0, endcode = 0, startstack = 0, kstkesp = 0, kstkeip = 0, wchan = 0;
//     int jiffy = 0;
//     FILE* fp = NULL;
//     char *sp = NULL, *t = NULL;
//     if (id == 0)
//         snprintf(fname, sizeof(fname) - 1, "/proc/stat");
//     else if (id == getpid())
//         snprintf(fname, sizeof(fname) - 1, "/proc/%d/stat", id);
//     else
//         snprintf(fname, sizeof(fname) - 1, "/proc/%d/task/%d/stat", getpid(), id);
//     if ((fp = fopen(fname, "r")) == NULL)
//         return -errno;
//     fgets(statStr, sizeof(statStr), fp);
//     fclose(fp);
//     if (id > 0)
//     {
//         sscanf(statStr, "%u", &id);
//         sp = strchr(statStr, '(') + 1;
//         t = strchr(statStr, ')');
//         strncpy(processName, sp, t - sp);
//         sscanf(t + 2, "%c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
//                 &state, &ppid, &pgrp, &session, &tty, &tpgid, &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &counter,
//                 &priority, &timeout, &itrealvalue, &starttime, &vsize, &rss, &rlim, &startcode, &endcode, &startstack,
//                 &kstkesp, &kstkeip, &signal, &blocked, &sigignore, &sigcatch, &wchan);
//         jiffy = utime + stime + cutime + cstime;
//     }
//     else
//     {
//         sscanf(statStr, "%s%d%d%d%d%d%d%d%d", tcpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal);
//         jiffy = user + nice + sys + idle + iowait + irq + softirq + steal;
//     }
//     return jiffy;
// #else
// 	return -1;
// #endif
// }
// static void DumpProcessMemInfo()
// {
//     char statmStr[64] = "";
//     int i = 0;
//     int memSize = 0;
//     FILE * fp = NULL;
//     if ((fp = fopen("/proc/self/statm", "r")) == NULL)
//     {
//         LOGE("%s\n",strerror(errno));
//         return;
//     }
//     fgets(statmStr, sizeof(statmStr), fp);
//     fclose(fp);
//     sscanf(statmStr, "%d %d", &i, &memSize);
//     memSize *= 4;
//     LOGD("PROCESS MEM INFO: Virtual %d kB Real %d kB statm %s", i*4, memSize, statmStr);
// }
// static void DumpSystemMemInfo()
// {
//     FILE * fp = NULL;
//     char memInfo[128] = "";
//     int len = 0;
//     if ((fp = fopen("/proc/meminfo", "r")) == NULL)
//     {
//         LOGE("%s\n",strerror(errno));
//         return;
//     }
//     fgets(memInfo, sizeof(memInfo) - 1, fp);
//     len = strlen(memInfo);
//     memInfo[len - 1] = ' ';
//     fgets(memInfo + len, sizeof(memInfo) - 1 - len, fp);
//     fclose(fp);
//     LOGD("SYSTEM MEM INFO: %s", memInfo);
// }
// static void DumpCpuInfo(int *lastSysJiffy, int *lastProcJiffy)
// {
//     int sysJiffy = 0, procJiffy = 0;
//     double cpuPercent = 0;
// #ifndef WIN32
//     sysJiffy = GetCpuJiffy(0);
//     procJiffy = GetCpuJiffy(getpid());
//     cpuPercent = (double)(procJiffy - *lastProcJiffy)/(double)(sysJiffy - *lastSysJiffy);
// #endif
//     *lastSysJiffy = sysJiffy;
//     *lastProcJiffy = procJiffy;
//     LOGD("PROCESS CPU INFO: %f\n", cpuPercent*100);
// }
// static S32 SystemDumpInfo(Common_Thread_T hThreadHandle, void *data)
// {
//     LOG_CONTEXT_T *pContext = (LOG_CONTEXT_T *)data;
//     int cnt = 0, lastSysJiffy = 0, lastProcJiffy = 0;
//     while (pContext && pContext->init == 1)
//     {
//         Common_Sleep(1,0);
//         cnt++;
//         if (cnt < DUMP_SYSTEM_INFO_INTERVAL)
//             continue;
//         DumpProcessMemInfo();
//         DumpSystemMemInfo();
//         DumpCpuInfo(&lastSysJiffy,&lastProcJiffy);
//         cnt = 0;
//     }
//     return 0;
// }

static U32 GenLogHeader(LOG_CONTEXT_T *pContext,const S8 * prefix, S32 tid, const S8 *funcStr, S32 line)
{

    S32 tp = 0,tp_msec = 0,ret = 0;
    U32 count = 0;
    S8 tname[LOG_TAG_SIZE] = {0};
//    struct tm newtime;
//    time_t longTime;

//    time(&longTime);
//    localtime_r(&longTime, &newtime);
    Common_GetSystemCount(&tp,&tp_msec);
//    count = snprintf(s_log_ct.logBuffer, sizeof(s_log_ct.logBuffer) - 1, "[%s]%04d-%02d-%02d %02d:%02d:%02d(%06d.%03d) - %s [%ld] %s: ",
//            s_log_ct.tag, newtime.tm_year + 1900, newtime.tm_mon + 1, newtime.tm_mday, newtime.tm_hour, newtime.tm_min, newtime.tm_sec,
//            (S32) (tp.tv_sec - s_log_ct.baseTime.tv_sec), (S32) (tp.tv_nsec / 1000000), prefix,tid,funcStr);
#ifdef WIN32
#else
    prctl(PR_GET_NAME, tname);
#endif
    ret = snprintf(pContext->logBuffer, LOG_HEAD_SIZE_MAX, "%s(%06d.%03d) - [%s] [%s:%d] %s/%-3d: ",
                   prefix,(S32) (tp - pContext->baseTime), (S32) (tp_msec),pContext->tag ,tname,tid,funcStr, line);

    if (ret > 0 && ret < LOG_HEAD_SIZE_MAX)
        count = (U32)ret;
    else if (ret > 0 && ret >= LOG_HEAD_SIZE_MAX)
        count = LOG_HEAD_SIZE_MAX;
    return count;
}


static void CheckLogLevel(LOG_CONTEXT_T *pContext)
{
    S8 lvBuf[4] = { 0 };
    if (pContext == NULL || pContext->debugFifoFd < 0)
        return;
    int ret = read(pContext->debugFifoFd,lvBuf,sizeof(lvBuf));
    if (ret > 0)
    {
        if (atoi(lvBuf) >= COMMON_LOG_LV_BASE && atoi(lvBuf) <= COMMON_LOG_LV_HIGH)
        {
            pContext->level = atoi(lvBuf);
        }
    }
}

S32 Common_Log_Create(Common_Log_T *phLog,S8 *tag, U32 defaultLevel)
{
    LOG_CONTEXT_T *pContext = NULL;
    if (phLog == NULL)
    {
        return -1;
    }
	
    pContext = (LOG_CONTEXT_T *)Common_Malloc(sizeof(LOG_CONTEXT_T),0,__FUNCTION__,__LINE__);
    if (pContext == NULL)
    {
        return -1;
    }
	
    memset(pContext, 0, sizeof(LOG_CONTEXT_T));
    if (defaultLevel > COMMON_LOG_LV_HIGH)
        pContext->level = COMMON_LOG_LV_HIGH;
    else
        pContext->level = defaultLevel;

    if (access(LOG_DEFAULT_LEVEL_BASE, F_OK) == 0)
        pContext->level = COMMON_LOG_LV_BASE;
    else if (access(LOG_DEFAULT_LEVEL_LOW, F_OK) == 0)
        pContext->level = COMMON_LOG_LV_LOW;
    else if (access(LOG_DEFAULT_LEVEL_MID, F_OK) == 0)
        pContext->level = COMMON_LOG_LV_MID;
    else if (access(LOG_DEFAULT_LEVEL_HIGH, F_OK) == 0)
        pContext->level = COMMON_LOG_LV_HIGH;

    pContext->init = 1;
    if (tag != NULL)
    {
        snprintf(pContext->tag, sizeof(pContext->tag) - 1, "%s", tag);
        snprintf(pContext->debugFifoFile,sizeof(pContext->debugFifoFile),"%s%s",DEBUG_FIFO_PATH,pContext->tag);
    }
    else
    {
        snprintf(pContext->debugFifoFile,sizeof(pContext->debugFifoFile),"%s%d",DEBUG_FIFO_PATH,getpid());
    }
    Common_Lock_Create(&(pContext->lockMutex),NULL);
    Common_GetSystemCount(&pContext->baseTime,NULL);
    *phLog = (Common_Log_T)pContext;

    mkfifo(pContext->debugFifoFile, S_IRWXU | S_IRWXG | S_IRWXO);
    pContext->debugFifoFd = open(pContext->debugFifoFile, O_RDWR | O_NONBLOCK, 0666);
    if (pContext->debugFifoFile < 0)
    {
        return -1;
    }

//	Common_Thread_Create(&pContext->systemDumpPth,"SystemDumpInfo",32*1024,0,SystemDumpInfo,(void *)pContext);
    return 0;

}
void Common_Log_Out(Common_Log_T hLog,U32 level, S32 lineNum,const S8 * funcStr,const S8 * prefix,const S8 *fmt, ...)
{
    S32 tid = 0;
    U32 len = 0;
    LOG_CONTEXT_T *pContext = (LOG_CONTEXT_T *)hLog;
    if (pContext == NULL)
    {
        return;
    }

    CheckLogLevel(pContext);
#ifndef WIN32
    tid = syscall(SYS_gettid);
#endif
    if (level <= pContext->level)
    {
        va_list ap;
        S8 *outBuffer;
        Common_Time_T tNewTime;
        time_t longTime;
        Common_Lock(pContext->lockMutex);
        outBuffer = pContext->logBuffer;

        memset(pContext->logBuffer, 0, sizeof(pContext->logBuffer));

        if (prefix != NULL)
        {
            len = GenLogHeader(pContext,prefix, tid, funcStr, lineNum);
        }
        else
        {
            len = GenLogHeader(pContext,"", tid, funcStr, lineNum);
        }
        outBuffer += len;
        va_start(ap, fmt);
        vsnprintf(pContext->logBuffer + len, (sizeof(pContext->logBuffer) - len - 2), fmt, ap);
        va_end(ap);
#if (!defined WIN32) && (!defined PLATFORM_HI3516E)
        syslog(LOG_USER|LOG_NOTICE,"%s",pContext->logBuffer);
#endif

        time(&longTime);
        Common_Linux2CommonTime(longTime,&tNewTime);
#ifndef WIN32
        switch (level)
        {
        case COMMON_LOG_LV_BASE:
            printf(RED);
            break;
        case COMMON_LOG_LV_LOW:
            printf(YELLOW);
            break;
        case COMMON_LOG_LV_MID:
            printf(NONE);
            break;
        case COMMON_LOG_LV_HIGH:
            printf(CYAN);
            break;
        }
#endif
        printf("%02d-%02d %02d:%02d:%02d %s",
               tNewTime.month, tNewTime.day, tNewTime.hour, tNewTime.min, tNewTime.sec,
               pContext->logBuffer);
        Common_UnLock(pContext->lockMutex);
#ifndef WIN32
        printf(NONE);
#endif
        fflush(stdout);
		
    }

	
}
void Common_Log_SetLevel(Common_Log_T hLog,U32 level)
{
    LOG_CONTEXT_T *pContext = (LOG_CONTEXT_T *)hLog;
    if (pContext == NULL)
    {
        return;
    }
    Common_Lock(pContext->lockMutex);
    if (level > COMMON_LOG_LV_HIGH)
    {
        pContext->level = COMMON_LOG_LV_HIGH;
    }
    else
    {
        pContext->level = level;
    }
    Common_UnLock(pContext->lockMutex);

}
S32 Common_Log_Destroy(Common_Log_T *phLog)
{
    LOG_CONTEXT_T *pContext;
    if (phLog == NULL)
    {
        return -1;
    }
    pContext =(LOG_CONTEXT_T *) (*phLog);

    Common_Lock_Destroy(&(pContext->lockMutex));

    pContext->init = 0;
    close(pContext->debugFifoFd);
    Common_Thread_Destroy(&pContext->systemDumpPth);
    Common_Free(pContext,__FUNCTION__,__LINE__);
    *phLog = NULL;

    return 0;
}
