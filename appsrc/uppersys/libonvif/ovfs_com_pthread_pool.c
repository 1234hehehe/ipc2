#include "ovfs_com_pthread_pool.h"

#include <pthread.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libcommon_api.h>
#include "ovfs_com_pthread_pool.h"
#define PTHREAD_POOL_VALID      0x0decca62
#define SEC_TO_NS           1000000000
#define SEC_TO_MS           1000
#define MS_TO_NS                1000000

#define    POOL_MALLOC(x)              Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define    POOL_FREE(x)                Common_Free(x,__func__,__LINE__)
#define    POOL_STRDUP(x)              Common_StrDup(x,__func__,__LINE__)

#define pthread_pool_assert(x) do  \
    {  \
        if (!(x))  {\
            LOGE("ASSERT %s FAILED in %s line %d errno %d error info %s\n",#x,__FUNCTION__,__LINE__,errno,strerror(errno));\
        }\
    } while(0)

typedef struct thread_cond
{
    struct thread_cond *next;
    pthread_cond_t cond;
} thread_cond;

typedef struct thread_worker
{
    struct thread_worker *next;
    struct thread_worker *prev;
    unsigned long id;
    int quit; /* if thread need quit ?      */
    int idle; /* thread wait timeout        */
    long long int wait_base; /* once wait: nanosecond      */
    long long int wait_sec; /* once wait: second          */
    long long int wait_nsec; /* once wait: nanosecond      */
    long long int wait_count; /* timeout of total wait      */
    pthread_job_t *job_first;  /* thread's work queue first  */
    pthread_job_t *job_last;  /* thread's work queue last   */
    int qlen; /* the work queue's length    */
    thread_cond *cond;
    pthread_mutex_t *mutex;
} thread_worker;

struct pthread_pool_t
{
    pthread_mutex_t worker_mutex; /* control access to queue    */
    pthread_cond_t cond; /* wait for worker quit       */
    pthread_mutex_t poller_mutex; /* just for wait poller exit  */
    pthread_cond_t poller_cond; /* just for wait poller exit  */
    pthread_attr_t attr; /* create detached            */
    pthread_job_t *job_first;  /* work queue first           */
    pthread_job_t *job_last;  /* work queue last            */
    pthread_job_t *job_slot_first;  /* work queue first           */
    pthread_job_t *job_slot_last;  /* work queue last            */
    thread_worker *thr_first;  /* first idle thread          */
    thread_worker *thr_iter;  /* for bat operation          */
    thread_cond *cond_first;
    int poller_running; /* is poller thread running ? */
    int qlen; /* the work queue's length    */
    int job_nslot;
    int qlen_warn; /* the work queue's length    */
    int valid; /* valid                      */
    int quit; /* worker should quit         */
    int poller_quit; /* poller should quit         */
    int parallelism; /* maximum threads            */
    int count; /* current threads            */
    int idle; /* idle threads               */
    int idle_timeout; /* idle timeout second        */
    long long int schedule_warn; /* schedule warn: millisecond */
    long long int schedule_wait; /* schedule wait: millisecond */
    int overload_wait; /* when too busy, sleep time  */
    time_t last_warn; /* last warn time             */
    int (*poller_fn)(void *arg);  /* worker poll function       */
    void *poller_arg;  /* the arg of poller_fn       */
    int (*worker_init_fn)(void *arg);  /* the arg is worker_init_arg */
    void *worker_init_arg;
    void (*worker_free_fn)(void *arg);  /* the arg is worker_free_arg */
    void *worker_free_arg;
};

static long long int GetMs(void)
{
    int sec = 0, msec = 0;

    Common_GetSystemCount(&sec, &msec);
    return (long long int)((long long int)sec * 1000LL + (long long int)msec);
}

static void SleepMs(unsigned long long msec)
{
    Common_Sleep((int) (msec / 1000), (int) ((msec % 1000)) * 1000);
}

static void *poller_thread(void *arg)
{
    pthread_pool_t *thr_pool = (pthread_pool_t *) arg;
    const int wait_msec = 1000, max_loop_persec = 81920;
    int loop_count;
    long long int now_t, pre_loop_t;

    if (thr_pool == NULL || thr_pool->poller_fn == NULL)
    {
        LOGE("thr_pool is null or poller_nf is null \n");
        return NULL;
    }
    loop_count = 0;
    pre_loop_t = GetMs();

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->poller_mutex) == 0);

    thr_pool->poller_running = 1;

    while (1)
    {
        if (thr_pool->poller_quit)
            break;

        now_t = GetMs();
        loop_count++;
        if (loop_count >= max_loop_persec)
        {
            /* avoid loop too quickly in one second */
            if (now_t - pre_loop_t <= wait_msec)
            {
                LOGW("%s loop too fast sleep %d ms\n", __func__, wait_msec);
                SleepMs(wait_msec);
                /* adjust the time of now */
                now_t = GetMs();
            }
            /*adjust the pre_loop_t time */
            pre_loop_t = now_t;
            loop_count = 0;
        }

        if (thr_pool->poller_fn(thr_pool->poller_arg) < 0)
            break;
    }

    thr_pool->poller_running = 0;
    pthread_pool_assert(pthread_cond_broadcast(&thr_pool->poller_cond) == 0);
    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->poller_mutex) == 0);
    return NULL;
}

static thread_cond *thread_cond_create(void)
{
    thread_cond *cond = (thread_cond *) POOL_MALLOC(sizeof(thread_cond));
    if (cond == NULL)
    {
        LOGE("alloc mem failed\n");
        return NULL;
    }
    memset(cond, 0, sizeof(thread_cond));

    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_pool_assert(pthread_cond_init(&cond->cond, &attr) == 0);
    return cond;
}

static void thread_cond_free(thread_cond *cond)
{
    pthread_cond_destroy(&cond->cond);
    POOL_FREE(cond);
}

static thread_worker *worker_create(pthread_pool_t *thr_pool)
{
    thread_worker *thr = (thread_worker *) POOL_MALLOC(sizeof(thread_worker));

    if (thr == NULL)
    {
        LOGE("alloc mem failed\n");
        return NULL;
    }

    memset(thr, 0, sizeof(thread_worker));
    thr->id = (unsigned long) pthread_self();
    thr->idle = thr_pool->idle_timeout;
    if (thr->idle > 0 && thr_pool->schedule_wait > 0)
    {
        thr->wait_sec = thr_pool->schedule_wait / SEC_TO_MS;
        thr->wait_nsec = (thr_pool->schedule_wait * MS_TO_NS) % SEC_TO_NS;
        thr->wait_count = (SEC_TO_MS * thr->idle) / thr_pool->schedule_wait;

        if (thr->wait_count == 0)
            thr->idle = 0;
    }
    else
        thr->idle = 0;

    if (thr_pool->cond_first != NULL)
    {
        thr->cond = thr_pool->cond_first;
        thr_pool->cond_first = thr_pool->cond_first->next;
    }
    else
        thr->cond = thread_cond_create();

    thr->mutex = &thr_pool->worker_mutex;
    return thr;
}

static void worker_free(pthread_pool_t *thr_pool, thread_worker *thr)
{
    thr->cond->next = thr_pool->cond_first;
    thr_pool->cond_first = thr->cond;
    POOL_FREE(thr);
}

static void worker_run(pthread_pool_t *thr_pool, thread_worker *thr,
                       pthread_job_t *job)
{
    void (*worker_fn)(void *) = job->worker_fn;
    void *worker_arg = job->worker_arg;

    /* shuld unlock before enter working process */
    pthread_pool_assert(pthread_mutex_unlock(thr->mutex) == 0);

    if (job->start > 0)
    {
        long long int now = GetMs();

        now -= job->start;
        if (now >= thr_pool->schedule_warn)
        {
            LOGW("%s schedule %llu >= %llu\n", __func__, now, thr_pool->schedule_warn);
        }
    }

    if (!job->fixed)
        POOL_FREE(job);

    worker_fn(worker_arg);

    /* lock again */
    pthread_pool_assert(pthread_mutex_lock(thr->mutex) == 0);
}

static int worker_wait(pthread_pool_t *thr_pool, thread_worker *thr)
{
    int status, idle_count = 0, got_job = 0;
    struct timespec timeout;
    struct timeval tv;

    /* add the thread to the idle threads pool */

    if (thr_pool->thr_first == NULL)
    {
        thr_pool->thr_first = thr;
        thr->next = NULL;
        thr->prev = NULL;
    }
    else
    {
        thr_pool->thr_first->prev = thr;
        thr->next = thr_pool->thr_first;
        thr->prev = NULL;
        thr_pool->thr_first = thr;
    }

    thr_pool->idle++;

    while (1)
    {

        if (thr->idle > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &timeout);
            timeout.tv_sec = tv.tv_sec + thr->wait_sec;
            timeout.tv_nsec = timeout.tv_nsec + thr->wait_nsec;
            if (timeout.tv_nsec > 1000000000L)
            {
                timeout.tv_nsec = timeout.tv_nsec % 1000000000L;
                timeout.tv_sec++;
            }
            status = pthread_cond_timedwait(&thr->cond->cond, thr->mutex, &timeout);
        }
        else
            status = pthread_cond_wait(&thr->cond->cond, thr->mutex);

        /* if thr->job_first not null, the thread had been remove
         * from idle threads pool by the main thread in job_deliver(),
         * so just return 1 here.
         */
        if (thr->job_first)
            return 1;

        /* else if threads pool's job not empty, the thread should
         * handle it and remove itself from the idle threads pool
         */
        if (thr_pool->job_first)
        {
            got_job = 1;
            break;
        }

        if (thr_pool->quit)
            break;

        if (status == ETIMEDOUT)
        {
            idle_count++;
            if (idle_count < thr->wait_count)
                continue;
            break;
        }
        else if (status == 0)
        {
            idle_count = 0;
            continue;
        }

        LOGW("%s status %d cond timewait error %d %s\n", __func__, status, errno,
             strerror(errno));
        break;
    }

    /* remove the thread from thread pool */

    if (thr_pool->thr_first == thr)
    {
        if (thr->next)
            thr->next->prev = NULL;
        thr_pool->thr_first = thr->next;
    }
    else
    {
        if (thr->next)
            thr->next->prev = thr->prev;
        thr->prev->next = thr->next;
    }

    thr_pool->idle--;

    /* if none job got, this must because the thread need to quit */
    if (!got_job)
        thr->quit = 1;

    return got_job;
}

static void *worker_thread(void *arg)
{
    pthread_pool_t *thr_pool = (pthread_pool_t *) arg;
    pthread_job_t *job;
    pthread_mutex_t *mutex;
    thread_worker *thr;

    if (thr_pool->worker_init_fn != NULL)
    {
        if (thr_pool->worker_init_fn(thr_pool->worker_init_arg) < 0)
        {
            pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);
            thr_pool->count--;
            pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
            return NULL;
        }
    }

    /* lock the thread pool's global mutex at first */

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    thr = worker_create(thr_pool);
    mutex = thr->mutex;

    while (1)
    {
        /* handle thread self's job first */
        if (thr->job_first != NULL)
        {
            job = thr->job_first;
            thr->job_first = job->next;
            if (thr->job_last == job)
                thr->job_last = NULL;
            thr->qlen--;

            worker_run(thr_pool, thr, job);
        }

        /* then handle thread pool's job */
        else if (thr_pool->job_first != NULL)
        {
            job = thr_pool->job_first;
            thr_pool->job_first = job->next;
            if (thr_pool->job_last == job)
                thr_pool->job_last = NULL;
            thr_pool->qlen--;

            worker_run(thr_pool, thr, job);
        }

        if (thr->job_first != NULL || thr_pool->job_first != NULL)
            continue;

        else if (thr_pool->quit)
            break;

        else if (worker_wait(thr_pool, thr) > 0)
            continue;

        /* when wait timeout, wait error or thread pool is quiting */
        if (thr->quit)
            break;
    }

    if (thr_pool->worker_free_fn != NULL)
        thr_pool->worker_free_fn(thr_pool->worker_free_arg);

    worker_free(thr_pool, thr);

    thr_pool->count--;

    if (thr_pool->quit) /* && thr_pool->count == 0) */
        pthread_pool_assert(pthread_cond_signal(&thr_pool->cond) == 0);

    pthread_pool_assert(pthread_mutex_unlock(mutex) == 0);

    return NULL;
}

static int job_deliver(pthread_pool_t *thr_pool, thread_worker *thr,
                       pthread_job_t *job)
{
    thread_cond *cond = thr->cond;

    thr->job_first = job;
    thr->job_last = job;
    thr->qlen++;

    if (thr_pool->thr_first == thr)
    {
        if (thr->next)
            thr->next->prev = NULL;
        thr_pool->thr_first = thr->next;
    }
    else
    {
        if (thr->next)
            thr->next->prev = thr->prev;
        thr->prev->next = thr->next;
    }

    thr_pool->idle--;

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
    pthread_pool_assert(pthread_cond_signal(&cond->cond) == 0);

    return 1;
}

static void job_add(pthread_pool_t *thr_pool, pthread_job_t *job)
{
    thread_worker *thr;

    /* must reset the job's next to NULL */
    job->next = NULL;

    if (thr_pool->schedule_warn > 0)
        job->start = GetMs();
    else
        job->start = 0;

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    /* at first, select one idle thread which qlen is 0 */

    thr = thr_pool->thr_first;
    if (thr && thr->qlen == 0 && job_deliver(thr_pool, thr, job) > 0)
        return;

    /* then, add job to the pool's queue and anyone can handle it */

    if (thr_pool->job_first == NULL)
        thr_pool->job_first = job;
    else
        thr_pool->job_last->next = job;
    thr_pool->job_last = job;
    thr_pool->qlen++;

    /* if not reach the max threads limit, create one thread */

    if (thr_pool->count < thr_pool->parallelism)
    {
        pthread_t id;

        pthread_pool_assert(pthread_create(&id, &thr_pool->attr, worker_thread,
                                           (void * ) thr_pool) == 0);
        thr_pool->count++;

        pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
        return;
    }

    /* if qlen is too long, should warning, event sleep a while */

    if (thr_pool->qlen > thr_pool->qlen_warn)
    {
        time_t now = time(NULL);

        if (now - thr_pool->last_warn >= 2)
        {
            thr_pool->last_warn = now;
            LOGW("%s overload , max_thread qlen %d idle %d\n", __func__, thr_pool->qlen,
                 thr_pool->idle);
        }
        if (thr_pool->overload_wait > 0)
        {
            LOGW("%s sleep %d sec\n", __func__, thr_pool->overload_wait);
            SleepMs(thr_pool->overload_wait * 1000);
        }
    }

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
}

int pthread_pool_add_one(pthread_pool_t *thr_pool, void (*run_fn)(void *),
                         void *run_arg)
{
    pthread_job_t *job;

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID || run_fn == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    job = pthread_pool_alloc_job(run_fn, run_arg, 0);

    job_add(thr_pool, job);
    return 0;
}

int pthread_pool_add_job(pthread_pool_t *thr_pool, pthread_job_t *job)
{
    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID || job == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    job_add(thr_pool, job);
    return 0;
}

int pthread_pool_bat_add_begin(pthread_pool_t *thr_pool)
{
    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    thr_pool->thr_iter = thr_pool->thr_first;

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);
    return 0;
}

static void job_append(pthread_pool_t *thr_pool, pthread_job_t *job)
{
    /* must reset the job's next to NULL */
    job->next = NULL;

    if (thr_pool->thr_iter != NULL)
    {

        /* if the idle thread has no job append, just it */

        if (thr_pool->thr_iter->qlen == 0)
        {
            thr_pool->thr_iter->job_first = job;
            thr_pool->thr_iter->job_last = job;
            thr_pool->thr_iter->qlen++;
            thr_pool->thr_iter = thr_pool->thr_iter->next;
            return;
        }

        /* iterator the left idle threads */

        for (thr_pool->thr_iter = thr_pool->thr_iter->next;
                thr_pool->thr_iter != NULL;
                thr_pool->thr_iter = thr_pool->thr_iter->next)
        {
            /* skip busy thread */
            if (thr_pool->thr_iter->qlen > 0)
                continue;

            thr_pool->thr_iter->job_first = job;
            thr_pool->thr_iter->job_last = job;
            thr_pool->thr_iter->qlen++;
            thr_pool->thr_iter = thr_pool->thr_iter->next;
            return;
        }
    }

    /* add the job to the thread pool's queue, anyone can handle it */

    if (thr_pool->job_first == NULL)
        thr_pool->job_first = job;
    else
        thr_pool->job_last->next = job;
    thr_pool->job_last = job;
    thr_pool->qlen++;

    /* if not reach the max threads limit, create one thread */

    if (thr_pool->count < thr_pool->parallelism)
    {
        pthread_t id;

        pthread_pool_assert(pthread_create(&id, &thr_pool->attr, worker_thread,
                                           (void * ) thr_pool) == 0);
        thr_pool->count++;
    }

    /* if there are too many jobs in thread pool's queue, do warning */

    if (thr_pool->qlen > thr_pool->qlen_warn)
    {
        long long int now = GetMs();

        if (now - thr_pool->last_warn >= 2)
        {
            thr_pool->last_warn = now;
            LOGW("%s overload , max_thread qlen %d idle %d\n", __func__, thr_pool->qlen,
                 thr_pool->idle);
        }
        if (thr_pool->overload_wait > 0)
        {
            LOGW("%s sleep %d sec\n", __func__, thr_pool->overload_wait);
            SleepMs(thr_pool->overload_wait * 1000);
        }
    }
}

int pthread_pool_bat_add_one(pthread_pool_t *thr_pool, void (*run_fn)(void *),
                             void *run_arg)
{
    pthread_job_t *job;

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID || run_fn == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    job = pthread_pool_alloc_job(run_fn, run_arg, 0);

    job_append(thr_pool, job);
    return 0;
}

int pthread_pool_bat_add_job(pthread_pool_t *thr_pool, pthread_job_t *job)
{

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    job_append(thr_pool, job);
    return 0;
}

int pthread_pool_bat_add_end(pthread_pool_t *thr_pool)
{
    int qlen;
    thread_worker *thr_iter, *next;

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    qlen = thr_pool->qlen;
    thr_iter = thr_pool->thr_first;

    /* iterator all the idle threads, signal one if it has job */

    for (; thr_iter != NULL; thr_iter = next)
    {

        /* handle thread self's job first */

        if (thr_iter->qlen > 0)
        {
            next = thr_iter->next;
            if (thr_pool->thr_first == thr_iter)
            {
                if (thr_iter->next)
                    thr_iter->prev = NULL;
                thr_pool->thr_first = thr_iter->next;
            }
            else
            {
                if (thr_iter->next)
                    thr_iter->next->prev = thr_iter->prev;
                thr_iter->prev->next = thr_iter->next;
            }

            pthread_pool_assert(pthread_cond_signal(&thr_iter->cond->cond) == 0);
            continue;
        }

        /* if thread pool's job not empty , let idle thread handle */

        else if (qlen > 0)
        {
            next = thr_iter->next;
            pthread_pool_assert(pthread_cond_signal(&thr_iter->cond->cond) == 0);
            qlen--;
            continue;
        }
        else
            next = thr_iter->next;
    }

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);

    thr_pool->thr_iter = NULL;
    return 0;
}

static void thread_pool_init(pthread_pool_t *thr_pool)
{
    thr_pool->quit = 0;
    thr_pool->poller_quit = 0;
    thr_pool->poller_running = 0;
    thr_pool->job_first = NULL;
    thr_pool->job_last = NULL;
    thr_pool->job_slot_first = NULL;
    thr_pool->job_slot_last = NULL;
    thr_pool->job_nslot = 0;
    thr_pool->thr_first = NULL;
    thr_pool->thr_iter = NULL;
    thr_pool->qlen = 0;
    thr_pool->overload_wait = 0;
    thr_pool->count = 0;
    thr_pool->idle = 0;
    thr_pool->schedule_warn = 100;
    thr_pool->schedule_wait = 1000;
    thr_pool->cond_first = NULL;
}

/* create work queue */

pthread_pool_t *thread_pool_create(int threads_limit, int idle_timeout,
                                   int stack_size)
{
    pthread_pool_t *thr_pool;
    pthread_pool_attr_t attr;

    pthread_pool_attr_init(&attr);
    pthread_pool_attr_set_threads_limit(&attr, threads_limit);
    pthread_pool_attr_set_idle_timeout(&attr, idle_timeout);
    pthread_pool_attr_set_stacksize(&attr, stack_size);

    thr_pool = pthread_pool_create(&attr);
    return thr_pool;
}

int pthread_pool_set_schedule_warn(pthread_pool_t *thr_pool, long long int n)
{
    if (thr_pool == NULL || n <= 0)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->schedule_warn = n;

    return 0;
}

int pthread_pool_set_schedule_wait(pthread_pool_t *thr_pool, long long int n)
{
    if (thr_pool == NULL || n <= 0)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    thr_pool->schedule_wait = n;

    return 0;
}

pthread_pool_t *pthread_pool_create(const pthread_pool_attr_t *attr)
{
    pthread_pool_t *thr_pool;

    thr_pool = (pthread_pool_t *) POOL_MALLOC(sizeof(*thr_pool));
    if (thr_pool == NULL)
    {
        LOGE("alloc mem failed\n");
        return NULL;
    }
    memset(thr_pool, 0, sizeof(*thr_pool));
    pthread_pool_assert(pthread_attr_init(&thr_pool->attr) == 0);

    if (attr && attr->stack_size > 0)
        pthread_attr_setstacksize(&thr_pool->attr, attr->stack_size);

    pthread_pool_assert(pthread_attr_setdetachstate(&thr_pool->attr,
                        PTHREAD_CREATE_DETACHED) == 0);
    pthread_pool_assert(pthread_mutex_init(&thr_pool->worker_mutex, NULL) == 0);
    pthread_pool_assert(pthread_cond_init(&thr_pool->cond, NULL) == 0);
    pthread_pool_assert(pthread_mutex_init(&thr_pool->poller_mutex, NULL) == 0);
    pthread_pool_assert(pthread_cond_init(&thr_pool->poller_cond, NULL) == 0);

    thread_pool_init(thr_pool);

    thr_pool->parallelism = (attr && attr->threads_limit > 0) ?
                            attr->threads_limit :
                            PTHREAD_POOL_DEF_THREADS;
    thr_pool->qlen_warn = thr_pool->parallelism * 10;
    thr_pool->idle_timeout = (attr && attr->idle_timeout > 0) ?
                             attr->idle_timeout :
                             PTHREAD_POOL_DEF_IDLE;
    thr_pool->poller_fn = NULL;
    thr_pool->poller_arg = NULL;

    thr_pool->worker_init_fn = NULL;
    thr_pool->worker_init_arg = NULL;
    thr_pool->worker_free_fn = NULL;
    thr_pool->worker_free_arg = NULL;

    thr_pool->valid = PTHREAD_POOL_VALID;

    return thr_pool;
}

int pthread_pool_set_qlen_warn(pthread_pool_t *thr_pool, int max)
{
    if (thr_pool == NULL || max <= 0)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    thr_pool->qlen_warn = max;
    return 0;
}

int pthread_pool_set_timewait(pthread_pool_t *thr_pool, int timewait)
{

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID || timewait < 0)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->overload_wait = timewait;
    return 0;
}

int pthread_pool_atinit(pthread_pool_t *thr_pool, int (*init_fn)(void *),
                        void *init_arg)
{

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->worker_init_fn = init_fn;
    thr_pool->worker_init_arg = init_arg;

    return 0;
}

int pthread_pool_atfree(pthread_pool_t *thr_pool, void (*free_fn)(void *),
                        void *free_arg)
{

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->worker_free_fn = free_fn;
    thr_pool->worker_free_arg = free_arg;

    return 0;
}

static int wait_poller_exit(pthread_pool_t *thr_pool)
{
    int status = 0, nwait = 0;

    if (thr_pool == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    thr_pool->poller_quit = 1;

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->poller_mutex) == 0);

    while (thr_pool->poller_running != 0)
    {

        nwait++;

        status = pthread_cond_wait(&thr_pool->poller_cond, &thr_pool->poller_mutex);
        if (status == ETIMEDOUT)
        {
            LOGW("%s wait timeout %d\n", __func__, nwait);
        }
        else if (status != 0)
        {
            LOGE("wait  error %d %s\n", errno, strerror(errno));
            pthread_pool_assert(pthread_mutex_unlock(&thr_pool->poller_mutex) == 0);
            return status;
        }
    }

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->poller_mutex) == 0);

    return status;
}

static int wait_worker_exit(pthread_pool_t *thr_pool)
{
    int status = 0, nwait = 0;

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    thr_pool->quit = 1;

    if (thr_pool->count < 0)
    {
        LOGW("%s , pool count < 0 %d\n", __func__, thr_pool->count);
        pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
        return -1;
    }
    else if (thr_pool->count == 0)
    {
        pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
        return 0;
    }

    /* 1. set quit flag
     * 2. broadcast to wakeup any sleeping
     * 4. wait till all quit
     */
    /* then: thr_pool->count > 0 */

    if (thr_pool->thr_first != NULL)
    {
        thread_worker *thr;

        for (thr = thr_pool->thr_first; thr != NULL; thr = thr->next)
            pthread_cond_signal(&thr->cond->cond);
    }

    while (thr_pool->count > 0)
    {
        nwait++;

        /* status = pthread_cond_timedwait(&thr_pool->cond,
         *      &thr_pool->worker_mutex, &timeout);
         */
        status = pthread_cond_wait(&thr_pool->cond, &thr_pool->worker_mutex);

        if (status == ETIMEDOUT)
        {
            LOGW("%s wait timeout %d\n", __func__, nwait);
        }
        else if (status != 0)
        {
            LOGW("wait  error %d %s\n", errno, strerror(errno));
            pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
            return status;
        }
    }

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);

    return status;
}

int pthread_pool_destroy(pthread_pool_t *thr_pool)
{
    int status, s1, s2, s3, s4, s5;
    pthread_job_t *job;

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->valid = 0; /* prevent any other operations */

    status = wait_poller_exit(thr_pool);
    if (status != 0)
    {
        return status;
    }

    status = wait_worker_exit(thr_pool);
    if (status != 0)
    {
        return status;
    }

    for (job = thr_pool->job_slot_first; job != NULL;)
    {
        pthread_job_t *tmp = job;
        job = job->next;
        pthread_pool_free_job(tmp);
    }
    thr_pool->job_nslot = 0;

//    utils_sleep(1000);
    s1 = pthread_mutex_destroy(&thr_pool->poller_mutex);
    s2 = pthread_cond_destroy(&thr_pool->poller_cond);

    for (; thr_pool->cond_first != NULL;)
    {
        thread_cond *cond = thr_pool->cond_first;
        thr_pool->cond_first = thr_pool->cond_first->next;

        thread_cond_free(cond);
    }

    s3 = pthread_mutex_destroy(&thr_pool->worker_mutex);
    s4 = pthread_cond_destroy(&thr_pool->cond);
    s5 = pthread_attr_destroy(&thr_pool->attr);

    POOL_FREE(thr_pool);

    status = s1 ? s1 : (s2 ? s2 : (s3 ? s3 : (s4 ? s4 : s5)));

    return status;
}

int pthread_pool_stop(pthread_pool_t *thr_pool)
{
    int status;

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->valid = 0; /* prevent any other operations */

    status = wait_poller_exit(thr_pool);
    if (status != 0)
    {
        return status;
    }

    status = wait_worker_exit(thr_pool);
    if (status != 0)
    {
        return status;
    }

    /* restore the valid status */
    thr_pool->valid = PTHREAD_POOL_VALID;

    return 0;
}

int pthread_pool_set_poller(pthread_pool_t *thr_pool, int (*poller_fn)(void *),
                            void *poller_arg)
{
    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID
            || poller_fn == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    thr_pool->poller_fn = poller_fn;
    thr_pool->poller_arg = poller_arg;
    return 0;
}

int pthread_pool_start_poller(pthread_pool_t *thr_pool)
{
    pthread_t id;

    if (thr_pool == NULL || thr_pool->valid != PTHREAD_POOL_VALID)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    if (thr_pool->poller_fn == NULL)
    {
        return -1;
    }

    pthread_pool_assert(pthread_mutex_lock(&thr_pool->poller_mutex) == 0);

    if (thr_pool->poller_running)
    {
        pthread_pool_assert(pthread_mutex_unlock(&thr_pool->poller_mutex) == 0);
        return -1;
    }

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->poller_mutex) == 0);

    thread_pool_init(thr_pool);

    pthread_pool_assert(pthread_create(&id, &thr_pool->attr, poller_thread,
                                       (void * ) thr_pool) == 0);

    return 0;
}

int pthread_pool_add_dispatch(void *dispatch_arg, void (*run_fn)(void *),
                              void *run_arg)
{
    pthread_pool_t *thr_pool;

    if (dispatch_arg == NULL || run_fn == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    thr_pool = (pthread_pool_t *) dispatch_arg;
    pthread_pool_bat_add_one(thr_pool, run_fn, run_arg);

    return 0;
}

int pthread_pool_dispatch(void *dispatch_arg, void (*run_fn)(void *),
                          void *run_arg)
{
    pthread_pool_t *thr_pool;

    if (dispatch_arg == NULL || run_fn == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    thr_pool = (pthread_pool_t *) dispatch_arg;

    pthread_pool_add(thr_pool, run_fn, run_arg);
    return 0;
}

int pthread_pool_limit(pthread_pool_t *thr_pool)
{
    if (thr_pool == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    return thr_pool->parallelism;
}

int pthread_pool_size(pthread_pool_t *thr_pool)
{
    int n;
    if (thr_pool == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    n = thr_pool->count;
    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);

    return n;
}

int pthread_pool_idle(pthread_pool_t *thr_pool)
{
    int n;
    if (thr_pool == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    n = thr_pool->idle;

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);

    return n;
}

int pthread_pool_busy(pthread_pool_t *thr_pool)
{
    int n;
    if (thr_pool == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    n = thr_pool->count - thr_pool->idle;

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);

    if (n < 0)
        LOGE("thread's count %d < idle %d\n", thr_pool->count, thr_pool->idle);

    return n;
}

int pthread_pool_qlen(pthread_pool_t *thr_pool)
{
    int n;
    if (thr_pool == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    n = thr_pool->qlen;

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);

    if (n < 0)
        LOGE("thread's qlen %d < 0\n", thr_pool->qlen);
    return n;
}

void pthread_pool_set_stacksize(pthread_pool_t *thr_pool, size_t size)
{
    if (thr_pool == NULL || size <= 0)
    {
        LOGE("%s\n", strerror(EINVAL));
        return;
    }

    pthread_attr_setstacksize(&thr_pool->attr, size);
}

void pthread_pool_attr_init(pthread_pool_attr_t *attr)
{
    if (attr)
        memset(attr, 0, sizeof(pthread_pool_attr_t));
}

void pthread_pool_attr_set_stacksize(pthread_pool_attr_t *attr, size_t size)
{
    if (attr && size > 0)
        attr->stack_size = size;
}

void pthread_pool_attr_set_threads_limit(pthread_pool_attr_t *attr,
        int threads_limit)
{
    if (attr && threads_limit > 0)
        attr->threads_limit = threads_limit;
}

void pthread_pool_attr_set_idle_timeout(pthread_pool_attr_t *attr,
                                        int idle_timeout)
{
    if (attr && idle_timeout > 0)
        attr->idle_timeout = idle_timeout;
}

pthread_job_t *pthread_pool_alloc_job(void (*run_fn)(void *), void *run_arg,
                                      int fixed)
{
    pthread_job_t *job = (pthread_job_t *) POOL_MALLOC(sizeof(pthread_job_t));
    if (job == NULL)
    {
        LOGE("alloc mem failed\n");
        return NULL;
    }
    job->worker_fn = run_fn;
    job->worker_arg = run_arg;
    job->next = NULL;
    job->fixed = fixed;
    return job;
}

void pthread_pool_free_job(pthread_job_t *job)
{
    POOL_FREE(job);
}


void pthread_pool_clear_wait_queue(pthread_pool_t *thr_pool,
                                   void (*worker_arg_free)(void *data))
{
    pthread_pool_assert(pthread_mutex_lock(&thr_pool->worker_mutex) == 0);

    pthread_job_t *job = NULL;
    for (job = thr_pool->job_first; job != NULL;)
    {
        pthread_job_t *tmp = job;
        job = job->next;
        if (worker_arg_free && tmp->worker_arg != NULL)
        {
            worker_arg_free(tmp->worker_arg);
            tmp->worker_arg = NULL;
        }
        pthread_pool_free_job(tmp);
        thr_pool->qlen--;
    }

    if (thr_pool->qlen <= 0)
        thr_pool->job_first = NULL;

    pthread_pool_assert(pthread_mutex_unlock(&thr_pool->worker_mutex) == 0);
}


