/*
 * ovfs_com_timer.c
 *
 *  Created on: 2016年4月9日
 *      Author: eric
 */

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>
#include <syscall.h>
#include <errno.h>
#include <limits.h>

#include <libcommon_api.h>
#include "ovfs_media.h"

#define TIMER_SIG_TIMER          (SIGRTMIN+1)
#define TIMER_INVALID_TIMER      ((timer_t)((int)(-1)))

/* 定时器全局handle*/
typedef struct
{
    int init;                                               //是否初始化
    pthread_t timerTh;
    int tid;
    unsigned int maxId;                    //最大定时器ID
    timer_t *timers;                                //timer
    pthread_mutex_t mutex;
    MQ_HANDLE_H mqHandle;       //消息队列handle
} TIMER_CONTEXT_T;

static TIMER_CONTEXT_T s_timer_ct;

/* 定时器主循环线程*/
static void *TimerWaitThread(void * arg)
{
    sigset_t waitset;
    siginfo_t info;
    s_timer_ct.tid = syscall(SYS_gettid);
    prctl(PR_SET_NAME,__func__);

    pthread_t ppid = pthread_self();
    pthread_detach(ppid);

    sigemptyset(&waitset);
    sigaddset(&waitset, TIMER_SIG_TIMER);

    while (s_timer_ct.init)
    {
        if (sigwaitinfo(&waitset, &info) != -1)
        {
            if (s_timer_ct.init == 0)
                break;

            unsigned int timer_id = (unsigned int) info.si_value.sival_int;

            pthread_mutex_lock(&s_timer_ct.mutex);
//            timer_delete(s_timer_ct.timers[timer_id]);
//            s_timer_ct.timers[timer_id] = TIMER_INVALID_TIMER;
            pthread_mutex_unlock(&s_timer_ct.mutex);
            Mq_PostTimer(s_timer_ct.mqHandle, timer_id);
        }
    }
    LOGW("timer sigwait thread exit\n");
    return NULL;
}

int Timer_Init(unsigned int maxId, MQ_HANDLE_H mq_handle)
{
    int tryCnt = 0;
    if (s_timer_ct.init)
    {
        LOGD("already init\n");
        return 0;
    }

    if (mq_handle == NULL)
    {
        LOGE("mq_handle is null\n");
        return -EINVAL;
    }

    Timer_SignalMask();

    s_timer_ct.maxId = maxId;
    s_timer_ct.timers = (timer_t *) MEDIA_MALLOC(maxId * sizeof(timer_t));

    memset(s_timer_ct.timers, (int) -1, maxId * sizeof(timer_t));

    pthread_mutex_init(&s_timer_ct.mutex, NULL);

    s_timer_ct.mqHandle = mq_handle;
    s_timer_ct.init = 1;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN*64);

    pthread_create(&s_timer_ct.timerTh, &attr, TimerWaitThread, NULL);
    Utils_Sleep(100);

    while (s_timer_ct.tid == 0 && tryCnt < 5)
    {
        LOGE("could not get timer recv thread tid\n");
        Utils_Sleep(100);
        tryCnt++;
    }

    if (s_timer_ct.tid == 0)
    {
        s_timer_ct.init = 0;
        return -1;
    }

    return 0;
}

void Timer_Uninit()
{
    unsigned int i = 0;
    s_timer_ct.init = 0;

    pthread_cancel(s_timer_ct.timerTh);
    pthread_join(s_timer_ct.timerTh,NULL);
    for (i = 0; i < s_timer_ct.maxId; i++)
    {
        if (s_timer_ct.timers[i] != TIMER_INVALID_TIMER)
            timer_delete(s_timer_ct.timers[i]);
    }

    pthread_mutex_destroy(&s_timer_ct.mutex);

    if (s_timer_ct.timers)
    {
        MEDIA_FREE(s_timer_ct.timers);
        s_timer_ct.timers = NULL;
    }
    memset(&s_timer_ct,0,sizeof(TIMER_CONTEXT_T));
}

int Timer_Start(unsigned int timerId, unsigned int interval)
{
    struct sigevent se;
    struct itimerspec ts;
    int ern = 0;

    if ((timerId >= s_timer_ct.maxId) || (interval == 0))
    {
       LOGE("%s\n",strerror(EINVAL));
        return -EINVAL;
    }
    Mq_DelTimer(s_timer_ct.mqHandle, timerId);

    pthread_mutex_lock(&s_timer_ct.mutex);

    if (s_timer_ct.timers[timerId] == TIMER_INVALID_TIMER)
    {
        memset(&se, 0, sizeof(se));
        se.sigev_notify = SIGEV_SIGNAL;
        se.sigev_signo = TIMER_SIG_TIMER;
#ifndef __mips__		
        se._sigev_un._tid = s_timer_ct.tid;
#endif		
        se.sigev_value.sival_int = timerId;
        if (timer_create(CLOCK_MONOTONIC, &se, &s_timer_ct.timers[timerId]) < 0)
        {
            ern = -errno;
            LOGE("timer creat fail %d %s\n", -ern, strerror(-ern));
            pthread_mutex_unlock(&s_timer_ct.mutex);
            return ern;
        }
    }

    ts.it_value.tv_sec = interval / 1000;
    ts.it_value.tv_nsec = (interval % 1000) * 1000000;
    ts.it_interval.tv_sec = ts.it_value.tv_sec;
    ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
    if (timer_settime(s_timer_ct.timers[timerId], 0, &ts, NULL) < 0)
    {
        ern = -errno;
        LOGE("timer set fail %d %s\n", -ern, strerror(-ern));
    }
    pthread_mutex_unlock(&s_timer_ct.mutex);

    return ern;
}

unsigned int Timer_ReadLeft(unsigned int timer_id)
{
    unsigned int left = 0;
    struct itimerspec ts;

    pthread_mutex_lock(&s_timer_ct.mutex);

    if (s_timer_ct.timers[timer_id] != TIMER_INVALID_TIMER)
    {
        timer_gettime(s_timer_ct.timers[timer_id], &ts);
        left = ts.it_value.tv_sec * 1000 + ts.it_value.tv_nsec / 1000000;
    }

    pthread_mutex_unlock(&s_timer_ct.mutex);

    return left;
}

int Timer_Stop(unsigned int timerId)
{
    if (timerId >= s_timer_ct.maxId)
    {
        LOGE("timer id is larger than max\n");
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    pthread_mutex_lock(&s_timer_ct.mutex);

    if (s_timer_ct.timers[timerId] != TIMER_INVALID_TIMER)
    {
        struct itimerspec ts;

        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;
        timer_settime(s_timer_ct.timers[timerId], 0, &ts, NULL);
        timer_delete(s_timer_ct.timers[timerId]);
    }

    s_timer_ct.timers[timerId] = TIMER_INVALID_TIMER;
    pthread_mutex_unlock(&s_timer_ct.mutex);

    Mq_DelTimer(s_timer_ct.mqHandle, timerId);
    return 0;
}

void Timer_SignalMask()
{
    sigset_t bset;
    sigemptyset(&bset);
    sigaddset(&bset, TIMER_SIG_TIMER);
    pthread_sigmask(SIG_BLOCK, &bset, NULL);
}
