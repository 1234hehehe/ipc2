#include "libcommon_struct.h"
#include "libcommon_api.h"
typedef struct _tagCOMMON_INTRA_TIMER
{
	U32 uInterval_ms;
	Common_Timer_Callback_def fxn;
	void *pUserData;
	Common_Thread_T hThread;
	S32 bExit;
	S32 bDestroy;// 自毁

	Common_InterSleep_T	intSleep;
}COMMON_INTRA_TIMER_T;

static S32 static_Common_Thread_Timer(Common_Thread_T hThreadHandle,void *pUserData)
{
	COMMON_INTRA_TIMER_T *pTimer = (COMMON_INTRA_TIMER_T *)pUserData;
	S32 nSec,nMSec;
	nSec = pTimer->uInterval_ms / 1000;
	nMSec = pTimer->uInterval_ms - nSec * 1000;
	do 
	{
		Common_InterSleep_Sleep(pTimer->intSleep,nSec,nMSec);
		if (pTimer->bExit)
		{
			break;
		}
		pTimer->fxn((Common_Timer_T)pTimer,pTimer->pUserData);
		if (pTimer->bExit || pTimer->bDestroy)
		{
			break;
		}
		
		
	} while (!pTimer->bExit);
	if (pTimer->bDestroy)
	{// 自毁
		//printf("destroy timer myself!\n");
		Common_InterSleep_Destroy(&pTimer->intSleep);
		Common_Free(pTimer,__FUNCTION__,__LINE__);
		pTimer = NULL;
	}
	//printf("destroy ok!\n");
	return 0;
}

S32 Common_Timer_Create(Common_Timer_T *phTimer,U32 uInterval_ms,Common_Timer_Callback_def fxn,void *pUserData)
{
	COMMON_INTRA_TIMER_T *pTimer;
	if (phTimer == NULL || fxn == NULL)
	{
		return -1;
	}
	pTimer = (COMMON_INTRA_TIMER_T *)Common_Malloc(sizeof(COMMON_INTRA_TIMER_T),0,__FUNCTION__,__LINE__);
	if (pTimer == NULL)
	{
		return -1;
	}
	memset(pTimer,0,sizeof(COMMON_INTRA_TIMER_T));
	pTimer->uInterval_ms = uInterval_ms;
	pTimer->fxn = fxn;
	pTimer->pUserData = pUserData;

	Common_InterSleep_Create(&pTimer->intSleep);
	if(pTimer->intSleep == NULL)
	{
		LOGE("intSleep malloc fail!\n");
		Common_Free(pTimer,__FUNCTION__,__LINE__);
		return -1;
	}
	
	if(Common_Thread_Create(&pTimer->hThread,__FUNCTION__,0,0,static_Common_Thread_Timer,pTimer))
	{
		Common_InterSleep_Destroy(&pTimer->intSleep);
		Common_Free(pTimer,__FUNCTION__,__LINE__);
		return -1;
	}
	*phTimer = (Common_Timer_T)pTimer;
	return 0;

}

S32 Common_Timer_Destroy(Common_Timer_T *phTimer)
{
	COMMON_INTRA_TIMER_T *pTimer;
	if (phTimer == NULL)
	{
		return -1;
	}
	pTimer = (COMMON_INTRA_TIMER_T *)*phTimer;
	pTimer->bExit = 1;

	if (pTimer->hThread == Common_Thread_Self())
	{// 线程内调用
		Common_Thread_Detach(pTimer->hThread);
		pTimer->bDestroy = 1;
		*phTimer = NULL;
		return 0;
	}

	Common_InterSleep_WakeUp(pTimer->intSleep);
	
	Common_Thread_Destroy(&pTimer->hThread);

	Common_InterSleep_Destroy(&pTimer->intSleep);
	
	Common_Free(pTimer,__FUNCTION__,__LINE__);
	*phTimer = NULL;
	return 0;
}


void Common_Sleep(S32 nSec,S32 MicroSec)
{
#ifdef WIN32

	U32 t;

	t = ((U32)nSec)*1000+(((U32)MicroSec)/1000);
	Sleep(t);
#else
	struct timespec req,rem;
	if (MicroSec == 0)
	{
		sleep(nSec);
	}
	else if(nSec * 1000 + MicroSec /1000 >= 10)
	{
		usleep(nSec * 1000000 + MicroSec);
	}
	else
	{
		req.tv_sec = (time_t)nSec;
		req.tv_nsec = ((long)MicroSec)*1000;
		nanosleep(&req,&rem);
	}
	
	
#endif

}
#ifdef WIN32
static  U64 CalculateMicroseconds(U64 performancecount,U64 performancefrequency)
{
	U64 f = performancefrequency;
	U64 a = performancecount;
	U64 b = a/f;
	U64 c = a%f; // a = b*f+c => (a*1000000)/f = b*1000000+(c*1000000)/f

	return b*1000000ui64+(c*1000000ui64)/f;
}
#endif

U64  Common_GetLocalTime(Common_Time_T *pCommon_Time)
{
	U64 uTimeU64;
	U32 dwSec;
#ifdef WIN32

	static S32 inited = 0;
	static U64 microseconds, initmicroseconds;
	static LARGE_INTEGER performancefrequency;

	U64 emulate_microseconds, microdiff;
	SYSTEMTIME systemtime;
	FILETIME filetime;

	LARGE_INTEGER performancecount;

	QueryPerformanceCounter(&performancecount);

	if(!inited){
		inited = 1;
		QueryPerformanceFrequency(&performancefrequency);
		GetSystemTime(&systemtime);
		SystemTimeToFileTime(&systemtime,&filetime);
		microseconds = ( ((U64)(filetime.dwHighDateTime) << 32) + (U64)(filetime.dwLowDateTime) ) / 10ui64;
		microseconds-= 11644473600000000ui64; // EPOCH
		initmicroseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);
	}

	emulate_microseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);

	microdiff = emulate_microseconds - initmicroseconds;
	uTimeU64 = (microseconds + microdiff) / 1000;
	dwSec = (U32)((microseconds + microdiff) / 1000000ui64);

	


#else
	struct timeval tv;
	// S32 nRet;

	gettimeofday(&tv,0);
	uTimeU64 = tv.tv_sec;
    uTimeU64 = uTimeU64 * 1000 + tv.tv_usec /1000;
	dwSec = tv.tv_sec;
	
#endif
	Common_Linux2CommonTime(dwSec,pCommon_Time);
	return uTimeU64;
}
S32 Common_GetCurrentTime(S32 *pSec,S32 *pMSec)
{
#ifdef WIN32

	static S32 inited = 0;
	static U64 microseconds, initmicroseconds;
	static LARGE_INTEGER performancefrequency;

	U64 emulate_microseconds, microdiff;
	SYSTEMTIME systemtime;
	FILETIME filetime;

	LARGE_INTEGER performancecount;

	QueryPerformanceCounter(&performancecount);

	if(!inited){
		inited = 1;
		QueryPerformanceFrequency(&performancefrequency);
		GetSystemTime(&systemtime);
		SystemTimeToFileTime(&systemtime,&filetime);
		microseconds = ( ((U64)(filetime.dwHighDateTime) << 32) + (U64)(filetime.dwLowDateTime) ) / 10ui64;
		microseconds-= 11644473600000000ui64; // EPOCH
		initmicroseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);
	}

	emulate_microseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);

	microdiff = emulate_microseconds - initmicroseconds;

	if (pSec)
	{
		*pSec = (U32)((microseconds + microdiff) / 1000000ui64);
	}
	if (pMSec)
	{
		*pMSec = ((U32)((microseconds + microdiff) % 1000000ui64)) / 1000;
	}


#else
	struct timeval tv;
	S32 nRet;

	nRet = gettimeofday(&tv,0);
	if (pSec)
	{
		*pSec = tv.tv_sec;
	}
	if (pMSec)
	{
		*pMSec = tv.tv_usec / 1000;
	}
	return nRet;
#endif
	return 0;
}
U64  Common_GetSystemCount64()
{
	U64 uTimeU64;
#ifdef WIN32
	uTimeU64 = GetTickCount64();
	
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);
	uTimeU64 = ts.tv_sec;
	uTimeU64 = uTimeU64 * 1000 + ts.tv_nsec/1000000;
	
#endif
	return uTimeU64;
}

S32 Common_GetSystemCount(S32 *pSec,S32 *pMSec)
{
#ifdef WIN32
	U64 uTimeU64;
	uTimeU64 = GetTickCount64();
	if (pSec)
	{
		*pSec = uTimeU64 / 1000;
	}
	if (pMSec)
	{
		*pMSec = uTimeU64 - (uTimeU64/1000) * 1000;
	}
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);
	if (pSec)
	{
		*pSec = ts.tv_sec;
	}
	if (pMSec)
	{
		*pMSec = ts.tv_nsec/1000000;
	}
#endif
	return 0;
}

struct tm * Common_LocalTime_r(time_t *t,struct tm *pTm)
{
#ifdef WIN32
	S32 nRet;
	nRet = localtime_s(pTm,t);
	if (nRet == 0)
	{
		return pTm;
	}
	else
	{
		return NULL;
	}
#else
	return localtime_r(t,pTm);
#endif
}
S32 Common_Linux2CommonTime(time_t linux_time, Common_Time_T *pCommon_time)
{
	struct tm p;
	if (pCommon_time == NULL)
	{
		return -1;
	}
	memset(&p, 0, sizeof(p));
	Common_LocalTime_r(&linux_time, &p);
	pCommon_time->year = 1900 + p.tm_year;
	pCommon_time->month = 1 + p.tm_mon;
	pCommon_time->day = p.tm_mday;

	pCommon_time->hour = p.tm_hour;
	pCommon_time->min = p.tm_min;
	pCommon_time->sec = p.tm_sec;
	pCommon_time->wday = p.tm_wday;
	pCommon_time->mday = p.tm_mday;
	pCommon_time->yday = p.tm_yday;
	return 0;

}
S32 Common_Common2LinuxTime(Common_Time_T *pCommon_time, time_t *plinux_time)
{
	struct tm p;
	if (pCommon_time == NULL || plinux_time == NULL)
	{
		return -1;
	}
	memset(&p, 0, sizeof(p));
	*plinux_time = time(NULL);
    Common_LocalTime_r (plinux_time, &p);
	p.tm_year = pCommon_time->year - 1900;
	p.tm_mon = pCommon_time->month - 1;
	p.tm_mday = pCommon_time->day;

	p.tm_hour = pCommon_time->hour;
	p.tm_min = pCommon_time->min;
	p.tm_sec = pCommon_time->sec;
	*plinux_time = mktime(&p);
	return 0;
}

S32 Common_CommonGm2LinuxTime(Common_Time_T *pCommon_time, time_t *plinux_time)
{
	struct tm p;
	if (pCommon_time == NULL || plinux_time == NULL)
	{
		return -1;
	}
	memset(&p, 0, sizeof(p));
	*plinux_time = time(NULL);
    Common_LocalTime_r (plinux_time, &p);
	p.tm_year = pCommon_time->year - 1900;
	p.tm_mon = pCommon_time->month - 1;
	p.tm_mday = pCommon_time->day;

	p.tm_hour = pCommon_time->hour;
	p.tm_min = pCommon_time->min;
	p.tm_sec = pCommon_time->sec;
	*plinux_time = mktime(&p);

	*plinux_time += Common_GetTimeDiff();
	return 0;
}


S32 Common_Linux2CommonGmTime(time_t linux_time, Common_Time_T *pCommon_time)
{
	struct tm p;
	if (pCommon_time == NULL)
	{
		return -1;
	}
	gmtime_r(&linux_time, &p);
	pCommon_time->year = 1900 + p.tm_year;
	pCommon_time->month = 1 + p.tm_mon;
	pCommon_time->day = p.tm_mday;

	pCommon_time->hour = p.tm_hour;
	pCommon_time->min = p.tm_min;
	pCommon_time->sec = p.tm_sec;
	pCommon_time->wday = p.tm_wday;
	pCommon_time->mday = p.tm_mday;
	pCommon_time->yday = p.tm_yday;
	return 0;

}

S32 Common_GetTimeDiff()
{	
	struct tm p;
	time_t t = time(NULL);
	localtime_r(&t,&p);
	
	return p.tm_gmtoff;
}

U32 Common_cmp_time(struct timeval *time1, struct timeval *time2)
{
	if ((U32) time1->tv_sec < (U32) time2->tv_sec)
		return -1;

	if ((U32) time1->tv_sec > (U32) time2->tv_sec)
		return 1;

	if ((U32) time1->tv_usec < (U32) time2->tv_usec)
		return -1;

	if ((U32) time1->tv_usec > (U32) time2->tv_usec)
		return 1;

	return 0;
}

S64 Common_cnt_delta_ms(struct timeval *time1, struct timeval *time2)
{
	S64 msec1 = 0,msec2 = 0;
	
	if(time1 != NULL)
	{
		msec1 = time1->tv_sec;
		msec1 *= 1000;
		msec1 += time1->tv_usec / 1000;
	}
	if (time2 != NULL)
	{
		msec2 = time2->tv_sec;
		msec2 *= 1000;
		msec2 += time2->tv_usec / 1000;
	}
	return msec1 - msec2;
	
}
S64 Common_cnt_interval_ms(struct timeval *time1, struct timeval *time2)
{
	if (Common_cmp_time(time1, time2) < 0)
	{
		return Common_cnt_delta_ms(time2, time1);
	}
	return Common_cnt_delta_ms(time1, time2);
}
typedef struct _tagINTERSLEEP_STRUCT
{
	Common_Sem_T hSem;
	
}INTERSLEEP_STRUCT;


S32 Common_InterSleep_Create(Common_InterSleep_T *pInterSleepHandle)
{
	INTERSLEEP_STRUCT *pSleep = NULL;
	if (pInterSleepHandle == NULL)
	{
		return -1;
	}
	pSleep = (INTERSLEEP_STRUCT *)Common_Malloc(sizeof(INTERSLEEP_STRUCT),0,__FUNCTION__,__LINE__);
	if (pSleep == NULL)
	{
		return -1;
	}
	memset(pSleep,0,sizeof(INTERSLEEP_STRUCT));
	if(Common_Sem_Create(&pSleep->hSem,0,1,__FUNCTION__))
	{
		return -1;
	}

	*pInterSleepHandle = (Common_InterSleep_T)pSleep;
	return 0;

}
S32 Common_InterSleep_Sleep(Common_InterSleep_T hInterSleepHandle,S32 nSec,S32 nMSec)
{
	INTERSLEEP_STRUCT *pSleep = (INTERSLEEP_STRUCT *)hInterSleepHandle;
	U32 nDoMSec;
	if (pSleep == NULL)
	{
		return -1;
	}
	nDoMSec = nSec * 1000 + nMSec;
	return Common_Sem_TryPend(pSleep->hSem,nDoMSec);
	

}
S32 Common_InterSleep_WakeUp(Common_InterSleep_T hInterSleepHandle)
{
	INTERSLEEP_STRUCT *pSleep = (INTERSLEEP_STRUCT *)hInterSleepHandle;
	if (pSleep == NULL)
	{
		return -1;
	}
	Common_Sem_Post(pSleep->hSem);

	return 0;
}
S32 Common_InterSleep_Destroy(Common_InterSleep_T *pInterSleepHandle)
{
	INTERSLEEP_STRUCT *pSleep;
	if (pInterSleepHandle == NULL || *pInterSleepHandle == NULL)
	{
		return -1;
	}
	pSleep = (INTERSLEEP_STRUCT *)(*pInterSleepHandle);
	Common_Sem_Destroy(&pSleep->hSem);
	Common_Free(pSleep,__FUNCTION__,__LINE__);
	*pInterSleepHandle = NULL;
	return 0;
}
