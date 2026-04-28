/*
 * libstreamqueue_api.c
 *
 *  Created on: 2016年6月6日
 *      Author: eric
 */
#ifdef WIN32
#else
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>
#include<sys/prctl.h>
#endif
#include "libcommon_api.h"
#include "libstreamqueue_api.h"

#define STREAM_MALLOC(x)                    Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define STREAM_FREE(x)                      Common_Free(x,__func__,__LINE__)
#define STREAM_QUEUE_DIR                    "/var/run/stream/"
#define STREAM_QUEUE_LOCK                   "/var/run/stream/lock"
#define STREAM_QUEUE_PATH_MAX               (128)
#define STREMA_QUEUE_NAME_SUFFIX_LEN        (5)
#define STREAM_QUEUE_NAME_MAX               (STREAM_QUEUE_PATH_MAX - STREMA_QUEUE_NAME_SUFFIX_LEN - strlen(STREAM_QUEUE_DIR))
#define STREAM_QUEUE_RECOVERY_TIME          (3000)


#define STREAM_QUEUE_MAGIC_NUM              (0x9c081246)
#define STREAM_QUEUE_MAX_SIZE               (1024*1024*100) //共享内存数据节点和值最大大小
#define STREAM_QUEUE_MAX_NUM                (1000)                    //共享内存数据节点最大node个数
#define STREAM_QUEUE_NODE_SIZE              (1024*1024*3)   //共享内存数据节点单个node最大值
#define STREAM_QUEUE_HEAD_SIZE              (1024*64)            //共享内存头索引节点最大存储空间
#define STREAM_QUEUE_KEY_BASE               (0xbebc0000)     //所有共享内存key值起始值

/*共享内存队列key值跨度,如果调整SHM_MAX_NUM,那么SHM_KEY_SEGMENT也要调整
 * 比如第一个共享内存队列所有索引节点和数据节点的起始key为 SHM_KEY_BASE + SHM_KEY_SEGMENT*0
 * 第二个共享内存队列所有索引节点和数据节点的起始key为 SHM_KEY_BASE + SHM_KEY_SEGMENT*1
 */
#define STREAM_QUEUE_KEY_SEGMENT            (1024)

#define MAX_STREAMQUEUE_EPOLL_NUM 256

#define MAX_STREAMQUEUE_EPOLL_EVNET_PTR_NUM 128
typedef struct tagSTREAMQUEUE_EPOLL_EVNET_MGR
{
    int epoll_handle;
    void *pPtrArray[MAX_STREAMQUEUE_EPOLL_EVNET_PTR_NUM];
    int nPtrArrayCount;
} STREAMQUEUE_EPOLL_EVNET_MGR_T;
static STREAMQUEUE_EPOLL_EVNET_MGR_T *g_EpollEvent[MAX_STREAMQUEUE_EPOLL_NUM];
static Common_Lock_T g_EpollEventLock = NULL;

#ifdef WIN32
int StreamQueue_Open(char *queueName,int openFlag,int maxMemSize,int maxNodeNum)
{
	static int staticHandle = 0;
	staticHandle++;
	return staticHandle;
}
int StreamQueue_Close(int streamQueueHandle)
{
	return 0;
}
int StreamQueue_WriteData(int streamQueueHandle, void *userData, unsigned int userSize, void *headData,
						  unsigned int headSize, void *data, unsigned int dataSize, int tag)
{
	return -1;
}
int StreamQueue_ReadData(int streamQueueHandle, void **userData, unsigned int *userSize, void **data, unsigned int *size, unsigned int timeout)
{
	static S8 bybuff[1024];
	if (data)
	{
		*data = bybuff;
	}
	if (size)
	{
		*size = 1024;
	}


	return 0;
}
int StreamQueue_ReleaseData(int streamQueueHandle)
{
	return -1;
}
int StreamQueue_Control(int streamQueueHandle, int tag)
{
	return -1;
}
int StreamQueue_EpollCreate()
{
	static int staticHandle = 0;
	staticHandle++;
	return staticHandle;
}
int StreamQueue_EpollDestroy(int epollHandle)
{
	return 0;
}
int StreamQueue_EpollCtl(int epollHandle, int ctl, STREAM_QUEUE_EPOLL_EVENT_T *events)
{
	return 0;
}
int StreamQueue_EpollWait(int epollHandle, STREAM_QUEUE_EPOLL_EVENT_T *events, int maxEvents, int timeout)
{
	return 0;
}
#else
typedef struct
{
    unsigned int nodeOffset;             //当前node对应的偏移,一旦分配后就此固定,不会修改
    unsigned int prevOffset;             //当前node对应的前一个node的偏移
    unsigned int nextOffset;             //当前node对应的后一个node的偏移
    unsigned int size;                   //当前node的size
    long long unsigned int seq;          //当前node的序号
    long long int uptime;                //update time
    int tag;                             //当前node用户数据标签
    key_t nodeKey;                       //当前node对应的共享内存key值,一旦分配后就此固定,不会修改
} SHM_QUEUE_NODE_T;

typedef struct
{
   unsigned int userDataLen;
   unsigned int dataLen;
   char reserve[24];
} SHM_QUEUE_NODE_DATA_HEAD_T;

typedef struct
{
    int createHandle;
    int writeHandleCnt;
    int readHandleCnt;
    int readHandleArray[128];
    int writeHandleArray[128];
} SHM_HANDLE_INFO_T;

/* 共享队列总索引结构*/
typedef struct
{
    int magic;
    int streamIdx;
    char streamName[STREAM_QUEUE_PATH_MAX];
    char creator[32];

    SHM_HANDLE_INFO_T shmInfo;

    unsigned int shmSizeMax;          // 共享内存总共分配的大小所有node节点size的和值
    unsigned int shmSize;             // 共享内存当前使用的空间大小
    unsigned int shmNumMax;           // 共享内存总共分配的node节点个数
    unsigned int shmNum;              // 当前节点个数
    unsigned int nodeBeginOffset;     // node节点起始偏移,一旦分配后就此固定,不会修改
    unsigned int headOffset;          // node头节点偏移
    unsigned int tailOffset;          // node尾节点偏移
    long long int uptime;             // 上一次更新时间
    int shmElimination;               // 淘汰机制,0 - 表示默认淘汰,即队列满后,淘汰最老的节点.最开始从最后一个节点获取
                                      //        1 - 表示最少节点淘汰策略,即每次读完释放一个节点后,就淘汰这个节点(适用回放),最开始从第一个节点获取,每次都是释放第一个节点
/* the following shm node table will be attached at end of this structure
 *SHM_QUEUE_NODE_T[0]
 *SHM_QUEUE_NODE_T[1]
 *SHM_QUEUE_NODE_T[2]
 *.......
 */
} SHM_QUEUE_HEAD_T;

typedef struct
{
    char namePath[STREAM_QUEUE_PATH_MAX];
    char flockPath[STREAM_QUEUE_PATH_MAX];
    int shmIdx;
    int shmId;
    key_t shmKey;
    int shmNotifyFd;                  // 共享内存事件通知监听fd
    int shmNotifyEpollFd;
    int shmFlockFd;                   // 文件记录锁fd
    int waitFlag;                     // 共享内存等待标识
    pthread_rwlock_t shmRwMutex;      // 共享内存读写锁
    char *addr;                       // 共享内存队列地址, 一般用来做地址偏移
    pthread_mutex_t notifyMutex;      // 共享内存消息通知琐
    pthread_cond_t notifyCond;        // 共享内存消息通知条件
    pthread_t notifyTh;
    long long int uptime;             // 上次更新时间
    SHM_QUEUE_HEAD_T *head;           // 共享内存队列索引指针,也就是变量addr
} SHM_QUEUE_CONTEXT_T;

typedef struct
{
    void *shmHandle;
    void *shmNodeHandle;
    void *attachAddr;
    int shmFd;
    char streamName[STREAM_QUEUE_PATH_MAX];
    int streamIdx;
    int openFlag;
} STREAM_OPEN_NODE_T;

static COMMON_DLIST_T s_openStreamList;

static inline long long int GetMs(void)
{
    int sec = 0, msec = 0;

    Common_GetSystemCount(&sec,&msec);
    return (long long int)((long long int)sec * 1000LL + (long long int)msec);
}

static inline void SleepMs(unsigned long long msec)
{
    Common_Sleep((int) (msec / 1000), (int) ((msec % 1000))*1000);
}

static void ListNodeFree(void *ptr)
{
    STREAM_FREE(ptr);
}

static void NotifySend(SHM_QUEUE_CONTEXT_T *ct)
{
    int ret = 0;
    char buf[512] = "s";
    ret = write(ct->shmNotifyFd, buf, 1);
    if (ret < 0)
    {
        read(ct->shmNotifyFd, buf, sizeof(buf));
        write(ct->shmNotifyFd, buf, 1);
    }
}

//static void *NotifyRecvThread(void *data)
//{
//    int evNum = 0, i = 0, epollFd = 0;
//    struct epoll_event events[16], ev;
//    SHM_QUEUE_CONTEXT_T *ct = (SHM_QUEUE_CONTEXT_T *) data;
//
//    prctl(PR_SET_NAME,__func__);
//
//    LOGD("listen the shm signal %s fd %d\n",ct->namePath,ct->shmNotifyFd);
//
//    epollFd = epoll_create(10);
//    if (epollFd < 0)
//    {
//        LOGE("create epoll failed %s\n", strerror(errno));
//        return NULL;
//    }
//    ev.events = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLET;
//    ev.data.fd = ct->shmNotifyFd;
//
//    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, ct->shmNotifyFd, &ev) < 0)
//    {
//        LOGE("epoll add fd %d failed %s\n", ct->shmNotifyFd,strerror(errno));
//        close(ct->shmNotifyFd);
//        close(epollFd);
//        return NULL;
//    }
//
//    while (ct->shmNotifyFd > 0)
//    {
//        evNum = epoll_wait(epollFd, events, 8, 500);
//        if (ct->shmNotifyFd <= 0)
//            break;
//
//        if (evNum < 0)
//        {
//            if (errno == EAGAIN || errno == EINTR)
//            {
//                LOGW("epoll wait error %d: %s\n", errno, strerror(errno));
//                continue;
//            }
//            LOGE("epoll wait failed %d %s\n", errno, strerror(errno));
//            break;
//        }
//
//        for (i = 0; i < evNum; i++)
//        {
//            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
//            {
//                LOGW("fd error %d num %d\n", events[i].events, evNum);
//                break;
//            }
//            else if (events[i].events & EPOLLIN)
//            {
//                pthread_mutex_lock(&ct->notifyMutex);
//                if (ct->waitFlag)
//                {
//                    pthread_cond_broadcast(&ct->notifyCond);
//                }
//                pthread_mutex_unlock(&ct->notifyMutex);
//            }
//            else
//            {
//                LOGW("unknow epoll event id %u\n", events[i].events);
//                continue;
//            }
//        }
//    }
//    close(epollFd);
//    LOGD("listhen thread exit %s\n",ct->namePath);
//    return NULL;
//}

static void CreateNodeTable(SHM_QUEUE_CONTEXT_T *ct)
{
    int i = 0;
    SHM_QUEUE_NODE_T *node = NULL;
    key_t key = 0;
    node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->nodeBeginOffset);

    for (i = 0; i < STREAM_QUEUE_MAX_NUM; i++)
    {
        key = ct->shmKey + i + 1;
        node[i].nodeKey = key;
        node[i].nodeOffset = ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T) * i;
        node[i].seq = 0;
        node[i].size = 0;
        node[i].tag = 0;
        node[i].prevOffset = 0;
        node[i].nextOffset = 0;
//        LOGD("create node shm done %p , key %d\t offset %u\t refs %d\t seq %llu\t size %u\t user %u\t prev %u\t next %u\n", node, node->node_key, node->node_offset, node->refs, node->seq, node->size, node->user, node->prev_offset, node->next_offset);
            }
        }

static int ShmQueue_NotifyWait(int epollFd, int fd, int timeout)
{
    int ret = 0;
    struct epoll_event events[16];

    ret = epoll_wait(epollFd, events, 2, timeout);

    return ret;
}

static int ShmQueue_Init(void ** shmHandle, char *streamName, int shmIdx)
{
    int ern = 0;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
//    pthread_condattr_t condattr;
//    pthread_attr_t attr;
    char sname[STREAM_QUEUE_PATH_MAX] = { 0 };

    if (shmHandle == NULL || shmIdx < 0 || streamName == NULL)
    {
        ern = -EINVAL;
        LOGE("init failed %s\n", strerror(-ern));
        return ern;
    }

    snprintf(sname,sizeof(sname)-1,"%s",streamName);

    *shmHandle = STREAM_MALLOC(sizeof(SHM_QUEUE_CONTEXT_T));
    memset(*shmHandle,0,sizeof(SHM_QUEUE_CONTEXT_T));
    ct = (SHM_QUEUE_CONTEXT_T *) *shmHandle;
    if (ct == NULL)
    {
        ern = -errno;
        LOGE("calloc failed %s\n", strerror(-ern));
        return ern;
    }

    memset(ct, 0, sizeof(SHM_QUEUE_CONTEXT_T));
    snprintf(ct->namePath, sizeof(ct->namePath) - 1, "%s%s", STREAM_QUEUE_DIR, sname);
    snprintf(ct->flockPath, sizeof(ct->flockPath) - 1, "%s%s.%04d",
    STREAM_QUEUE_DIR, sname, shmIdx);
    mkfifo(ct->namePath, S_IRWXU | S_IRWXG | S_IRWXO);
    mkfifo(ct->flockPath, S_IRWXU | S_IRWXG | S_IRWXO);

    ct->shmIdx = shmIdx;
    ct->shmNotifyFd = open(ct->namePath, O_RDWR | O_NONBLOCK, 0666);
    if (ct->shmNotifyFd < 0)
    {
        ern = -errno;
        STREAM_FREE(ct);
        LOGE("open notify file failed, %s\n", strerror(-ern));
        return ern;
    }

    ct->shmNotifyEpollFd = epoll_create(10);
    if (ct->shmNotifyEpollFd < 0)
    {
        ern = -errno;
        STREAM_FREE(ct);
        LOGE("create notify epoll failed, %s\n", strerror(-ern));
        return ern;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLET;
    ev.data.fd = ct->shmNotifyFd;
    if (epoll_ctl(ct->shmNotifyEpollFd, EPOLL_CTL_ADD, ct->shmNotifyFd, &ev) < 0)
    {
        ern = -errno;
        close(ct->shmNotifyEpollFd);
        STREAM_FREE(ct);
        LOGE("add notify epoll failed, %s\n", strerror(-ern));
        return ern;
    }

    ct->shmFlockFd = open(ct->flockPath, O_RDWR | O_NONBLOCK, 0666);
    if (ct->shmFlockFd < 0)
    {
        ern = -errno;
        STREAM_FREE(ct);
        LOGE("open lock file %s failed %s\n", ct->flockPath ,strerror(-ern));
        return ern;
    }

    pthread_rwlockattr_t rwattr;
    pthread_rwlockattr_init(&rwattr);
    pthread_rwlockattr_setkind_np(&rwattr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&ct->shmRwMutex, &rwattr);
    pthread_rwlockattr_destroy(&rwattr);

//    pthread_mutex_init(&ct->notifyMutex, NULL);
//    pthread_condattr_init(&condattr);
//    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
//    pthread_cond_init(&ct->notifyCond, &condattr);
//    pthread_condattr_destroy(&condattr);

//    pthread_attr_init(&attr);
//    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 64);
//    pthread_create(&ct->notifyTh, &attr, NotifyRecvThread, (void * ) ct);
	if(g_EpollEventLock == NULL)
	{
		Common_Lock_Create(&g_EpollEventLock, NULL);
		memset(g_EpollEvent,0,sizeof(g_EpollEvent));
	}

    return ct->shmNotifyFd;
}

static void ShmQueue_Uninit(void *shmHandle)
{
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    if (shmHandle == NULL)
        return;

    ct = (SHM_QUEUE_CONTEXT_T *) (shmHandle);
    close(ct->shmNotifyFd);
    close(ct->shmFlockFd);
    close(ct->shmNotifyEpollFd);
    ct->shmNotifyFd = -1;
    ct->shmFlockFd = -1;

    if (ct->notifyTh > 0)
    {
        pthread_join(ct->notifyTh, NULL);
    }
    STREAM_FREE(ct);
}

static int ShmQueue_SetElimination(void *streamHandle, int createFlag)
{
    SHM_QUEUE_CONTEXT_T *ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    if (streamHandle == NULL || ct->head == NULL)
        return -EINVAL;

    pthread_rwlock_wrlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);
    if (COMMON_TST_BIT(createFlag,16))
        ct->head->shmElimination = 1;
    else
        ct->head->shmElimination = 0;
    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
    return 0;
}

static int ShmQueue_GetShmConnCnt(void *streamHandle)
{
    SHM_QUEUE_CONTEXT_T *ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    int ern = 0, shmId = 0;
    if (streamHandle == NULL)
        return -EINVAL;

    struct shmid_ds shmseg;

    if ((shmId = shmget(ct->shmKey, STREAM_QUEUE_HEAD_SIZE, 0666)) == -1)
    {
        ern = -errno;
        return ern;
    }

    if (shmctl(shmId, SHM_STAT, &shmseg) < 0)
    {
        ern = -errno;
        return ern;
    }

    return shmseg.shm_nattch;
}

static void ShmQueue_DumpNodeInfo(void *streamHandle)
{
#ifdef START_MEM_DEBUG
//    SHM_QUEUE_CONTEXT_T *ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
//    SHM_QUEUE_NODE_T *head = NULL, *tail = NULL, *node = NULL;
//    int i = 0, shmId = 0;
//    if (ct->head == NULL || ct->head->magic == 0 || ct->head->shmElimination == 1)
//        return;
//    if (ct->uptime == 0)
//        ct->uptime = GetMs();
//    if (GetMs() - ct->uptime < 30*1000)
//        return;
//    tail = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
//    head = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->headOffset);
//    LOGD("stream idx %d head offset %u seq %u tail offset %u seq %u, num %u, num max %u, size %u, size max %u\n",
//                    ct->shmIdx, ct->head->headOffset,head->nodeOffset,ct->head->tailOffset,tail->nodeOffset,ct->head->shmNum,ct->head->shmNumMax,ct->head->shmSize,ct->head->shmSizeMax);
//    for (i = 0; i < ct->head->shmNumMax; i++)
//    {
//        struct shmid_ds shmseg;
//        node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T) * (i % ct->head->shmNumMax));
//        if ((shmId = shmget(node->nodeKey, 1,0666)) == -1)
//        {
//            LOGD("get node failed %x size %u %s %d\n", node->nodeKey, node->size, strerror(errno), errno);
//            continue;
//        }
//        if (shmctl(shmId, SHM_STAT, &shmseg) < 0)
//        {
//            LOGD("get node info failed %x\n",node->nodeKey);
//            continue;
//        }
//        if (shmseg.shm_nattch > 0)
//        {
//            LOGD("node %x was attached , attached cnt %lu , last op pid %d\n",node->nodeKey,shmseg.shm_nattch,shmseg.shm_lpid);
//        }
//    }
//    ct->uptime = GetMs();
#endif
}
static int ShmQueue_CreateShm(void *streamHandle, int maxMemSize, int maxMemNum)
{
    int ern = 0, shmId = 0, i = 0;
    key_t key;
    SHM_QUEUE_CONTEXT_T *ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    if (streamHandle == NULL || ct->shmFlockFd <= 2 || ct->shmNotifyFd <= 2 || maxMemSize > STREAM_QUEUE_MAX_SIZE
                    || maxMemSize < 0 || maxMemNum > STREAM_QUEUE_MAX_NUM || maxMemNum < 0)
    {
        ern = -EINVAL;
        LOGE("create failed %s\n", strerror(-ern));
        return ern;
    }

    pthread_rwlock_wrlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);

    key = STREAM_QUEUE_KEY_BASE + STREAM_QUEUE_KEY_SEGMENT * ct->shmIdx;
    shmId = shmget(key, STREAM_QUEUE_HEAD_SIZE, IPC_CREAT | 0666);
    if (shmId == -1)
    {
        ern = -errno;
        LOGE("get failed %s\n", strerror(-ern));
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    ct->addr = (char *)shmat(shmId, NULL, 0);
    if (ct->addr == (void *) -1)
    {
        ern = -errno;
        LOGE("attach failed %s\n", strerror(-ern));
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    ct->shmId = shmId;
    ct->shmKey = key;
    LOGD("create shm idx done , key %x\n", key);
    ct->head = (SHM_QUEUE_HEAD_T *) ct->addr;

    /*如果索引节点中有数据而且间隔时间小于STREAM_QUEUE_RECOVERY_TIME毫秒,则不需要清空索引节点*/
    if (ct->head->shmElimination == 0 && ct->head->magic == STREAM_QUEUE_MAGIC_NUM && GetMs() - ct->head->uptime < STREAM_QUEUE_RECOVERY_TIME)
    {
        ct->head->uptime = GetMs();
        LOGI("Intervals is less than %d seconds , reuse the queue\n",STREAM_QUEUE_RECOVERY_TIME);
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return 0;
    }

    /*create shm */
    memset(ct->addr, 0, STREAM_QUEUE_HEAD_SIZE);
    ct->head->magic = STREAM_QUEUE_MAGIC_NUM;
    ct->head->nodeBeginOffset = sizeof(SHM_QUEUE_HEAD_T);
    ct->head->headOffset = ct->head->tailOffset = ct->head->nodeBeginOffset;
    ct->head->shmNumMax = maxMemNum;
    ct->head->shmSizeMax = maxMemSize;

    CreateNodeTable(ct);
    ct->head->uptime = GetMs();
    Common_GetSelfExeName(ct->head->creator, sizeof(ct->head->creator) - 1);

    snprintf(ct->head->streamName, sizeof(ct->head->streamName) - 1, "%s", ct->namePath);

//    LOGD("total num %u total size %u offset %u\n", ct->head->shmNumMax, ct->head->shmSizeMax, ct->head->nodeBeginOffset);
    for (i = 0; i < ct->head->shmNumMax; i++)
    {
        SHM_QUEUE_NODE_T *node = NULL;
        node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T) * i);
//        LOGW("get node shm %p, key %x\t offset %u\t refs %u\t seq %llu\t size %u\t user %u\t prev %u\t next %u\n", node,
//                        node->nodeKey, node->nodeOffset, node->refs, node->seq, node->size, node->user, node->prevOffset,
//                        node->nextOffset);
        shmId = shmget(node->nodeKey, 1, 0666);
        shmctl(shmId, IPC_RMID, NULL);
    }
    ct->uptime = GetMs();
    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
    return 0;
}

static int ShmQueue_DestroyShm(void *shmHandle)
{
    int shmId = 0, i = 0;
    SHM_QUEUE_CONTEXT_T *ct = (SHM_QUEUE_CONTEXT_T *) shmHandle;

    if (shmHandle == NULL || ct->head == NULL || ct->head->magic != STREAM_QUEUE_MAGIC_NUM)
    {
        LOGE("%s\n", strerror(errno));
        return -EINVAL;
    }

    pthread_rwlock_wrlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);
    /*destroy all shm data node*/
    for (i = 0; i < ct->head->shmNumMax; i++)
    {
        SHM_QUEUE_NODE_T *node = NULL;
        node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T) * i);
        shmId = shmget(node->nodeKey, 1, 0666);
        shmctl(shmId, IPC_RMID, NULL);
    }

    /*clear the magic to make this head invalid*/
    ct->head->magic = 0;
    memset(ct->head->creator,0,sizeof(ct->head->creator));
    shmctl(ct->shmId, IPC_RMID, NULL);
    shmdt(ct->addr);
    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
    return 0;
}

static int ShmQueue_ConnectShm(void *shmHandle)
{
    int ern = 0, shmId = 0;
    key_t key;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    ct = (SHM_QUEUE_CONTEXT_T *) shmHandle;

    if (shmHandle == NULL)
    {
        ern = -EINVAL;
        LOGE("connect failed %s\n", strerror(-ern));
        return ern;
    }

    pthread_rwlock_wrlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);

    key = STREAM_QUEUE_KEY_BASE + STREAM_QUEUE_KEY_SEGMENT * ct->shmIdx;
    shmId = shmget(key, STREAM_QUEUE_HEAD_SIZE, IPC_CREAT | 0666);
    if (shmId == -1)
    {
        ern = -errno;
        LOGE("get failed %s\n", strerror(-ern));
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    ct->addr = (char *)shmat(shmId, NULL, 0);
    if (ct->addr == (void *) -1)
    {
        ern = -errno;
        LOGE("attach failed %s\n", strerror(-ern));
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    ct->shmId = shmId;
    ct->head = (SHM_QUEUE_HEAD_T *) ct->addr;
    ct->shmKey = key;
    ct->uptime = GetMs();
    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
    return 0;
}

static int ShmQueue_DisconnectShm(void *shmHandle)
{
    int ern = 0;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    ct = (SHM_QUEUE_CONTEXT_T *) shmHandle;

    if (shmHandle == NULL)
    {
        ern = -EINVAL;
        LOGE("disconnect failed %s\n", strerror(-ern));
        return ern;
    }

    pthread_rwlock_wrlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);

    shmdt(ct->addr);
    ct->addr = NULL;
    ct->head = NULL;
    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
    return 0;
}

//static int ShmQueue_Recycle(char *streamName)
//{
//    DIR *sp_dp = NULL;
//    struct dirent *ptr = NULL;
//    int idx = -1;
//
//    sp_dp = opendir(STREAM_QUEUE_DIR);
//    if (sp_dp == NULL)
//    {
//        LOGE("open dir failed! %s\n", STREAM_QUEUE_DIR);
//        return -1;
//    }
//
//    while ((ptr = readdir(sp_dp)) != NULL)
//    {
//        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 || strcmp(ptr->d_name, "lock") == 0)
//        {
//            continue;
//        }
//
//        if (sscanf(ptr->d_name, "%*[^.].%04d", &idx) > 0 && idx > -1)
//        {
//            char sName[256] = { 0 };
//            sscanf(ptr->d_name, "%[^.]", sName);
//            if (strcmp(sName,streamName) == 0)
//            {
//                void *h = NULL;
//                if (ShmQueue_Init(&h,streamName,idx) > 0)
//                {
//                    if (ShmQueue_ConnectShm(h) == 0)
//                    {
//                        ShmQueue_DestroyShm(h);
//                        LOGD("recycle stream %s done\n",streamName);
//                    }
//                    ShmQueue_Uninit(h);
//                }
//                break;
//            }
//        }
//    }
//
//    closedir(sp_dp);
//    return 0;
//}

static int ShmQueue_WriteData(void *streamHandle, void *userData, unsigned int userSize,
                void *headData, unsigned int headSize, void *data, unsigned int dataSize, int tag)
{
    int ern = 0, nodeId = -1, i = 0, shmId = 0, refsCnt = 0;
    unsigned int size = 0;
    void *addr = NULL;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    SHM_QUEUE_NODE_T * node = NULL, *head = NULL, *tail = NULL;
    SHM_QUEUE_NODE_DATA_HEAD_T nodeDataHeader;
    ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;

    if (streamHandle == NULL || data == NULL || dataSize == 0 || (userData == NULL && userSize > 0) || (headData == NULL && headSize > 0)
                    || dataSize + headSize + userSize + sizeof(SHM_QUEUE_NODE_DATA_HEAD_T) > STREAM_QUEUE_NODE_SIZE)

    {
        LOGE("args error\n");
        ern = -EINVAL;
        return ern;
    }

    size = sizeof(SHM_QUEUE_NODE_DATA_HEAD_T) + userSize + headSize + dataSize;

    if (ct->addr == NULL || ct->head->magic == 0)
    {
        LOGE("invalid connection\n");
//        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
//        pthread_rwlock_unlock(&ct->shmRwMutex);
        return -EINVAL;
    }

    if (size > ct->head->shmSizeMax)
    {
        LOGE("insert size was too big %d\n",size);
        return -EINVAL;
    }

    if (ct->head->shmElimination == 1 && ct->head->shmNumMax <= ct->head->shmNum)
    {
//        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
//        pthread_rwlock_unlock(&ct->shmRwMutex);
        SleepMs(10);
        return -EBUSY;
    }

    ShmQueue_DumpNodeInfo(streamHandle);
    pthread_rwlock_wrlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);
    ct->head->uptime = GetMs();

    tail = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
    head = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->headOffset);

//    LOGD("shm num %d max %d size %d size max %d\n",ct->head->shmNum,ct->head->shmNumMax,ct->head->shmSize,ct->head->shmSizeMax);
    /*检查是否有空闲的可用节点*/
    if (ct->head->shmNum + 1 <= ct->head->shmNumMax && ct->head->shmSize + size <= ct->head->shmSizeMax)
    {
        for (i = 0; i < ct->head->shmNumMax; i++)
        {
            struct shmid_ds shmseg;

            node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T) * (i % ct->head->shmNumMax));

            if ((shmId = shmget(node->nodeKey, 1,0666)) == -1)
            {
                if (errno == ENOENT)/*如果这个节点不存在，表示可以创建*/
                {
                    nodeId = i;
                    break;
                }
                else
                {
                    LOGW("get node failed %x size %u %s %d\n", node->nodeKey, node->size, strerror(errno), errno);
                    continue;
                }
            }

            if (shmctl(shmId, SHM_STAT, &shmseg) < 0)
            {
                LOGW("get node info failed %x\n",node->nodeKey);
                continue;
            }

            if (shmseg.shm_nattch > 0)
                refsCnt++;

            /*prev offset next offset  refs == 0 说明这个节点是空闲的(被释放了的或者是从来没有使用过的)*/
            if (node->prevOffset == 0 && node->nextOffset == 0 && shmseg.shm_nattch == 0)
            {
//                LOGD("node offset is %d\n", node->nodeOffset);
                nodeId = i;
                break;
            }
        }
    }

    if (refsCnt >= ct->head->shmNumMax)
    {
        LOGD("all node was userd, please remember free that node you got, refsCnt %d max %d\n",refsCnt, ct->head->shmNumMax);
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        SleepMs(10);
        return -EBUSY;
    }
//    LOGD("used node num %u max num %u  used size %u max size %u\n",
//                    ct->head->shmNum,ct->head->shmNumMax,ct->head->shmSize,ct->head->shmSizeMax);

    /*没有可用节点,淘汰历史节点*/
    if (nodeId < 0 && ct->head->shmElimination == 0)
    {
        struct shmid_ds shmseg;
        do
        {
            if (ct->head->shmNum == 0)
            {
                LOGE("num error\n");
                ct->head->headOffset = ct->head->tailOffset = ct->head->nodeBeginOffset;
                ct->head->shmNum = 0;
                ct->head->shmSize = 0;
                ern = -1;
                break;
            }

            if(ct->head->headOffset >  sizeof(SHM_QUEUE_NODE_T)*(STREAM_QUEUE_MAX_NUM - 1) ||
                            ct->head->headOffset > ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T)*(STREAM_QUEUE_MAX_NUM - 1))
            {
                LOGE("head nodeOffset %u clear all node\n",ct->head->headOffset);
                ct->head->headOffset = ct->head->tailOffset = ct->head->nodeBeginOffset;
                ct->head->shmNum = 0;
                ct->head->shmSize = 0;
                ern = -1;
                break;
            }

            node = head;
            if (ct->head->shmSize < node->size)
            {
                LOGE("size error\n");
                ern = -1;
                break;
            }

            head = (SHM_QUEUE_NODE_T *) (ct->addr + head->nextOffset);
            head->prevOffset = head->nodeOffset;
            ct->head->headOffset = head->nodeOffset;
            ct->head->shmNum--;
            ct->head->shmSize -= node->size;

            node->prevOffset = node->nextOffset = 0;
            shmId = shmget(node->nodeKey, node->size, IPC_CREAT | 0666);
            shmctl(shmId, SHM_STAT, &shmseg);
            shmctl(shmId, IPC_RMID, NULL);
//            LOGD("remove node offset %d\n",node->nodeOffset);
        } while (ct->head->shmSize + size > ct->head->shmSizeMax || shmseg.shm_nattch > 0); //总大小合适,并且这个节点没有引用
    }

    /*读端淘汰，队列满时，只有release后才有一个空余节点*/
    if (nodeId < 0 && ct->head->shmElimination == 1)
    {
//        LOGD("no free node, please remember free that node you got\n");
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        SleepMs(10);
        return -EBUSY;
    }

    if (ern < 0)
    {
        LOGE("queue state error, shm size %d shm num %d , try to insert node size %d \n",
                        ct->head->shmSize,ct->head->shmNum,size);
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        SleepMs(10);
        return -EBUSY;
    }

//    LOGI("offset %u prev %u next %u seq %llu size %u time %lld head offset %u seq %llu tail offset %u tail next %u seq %llu\n", node->node_offset, node->prev_offset, node->next_offset, node->seq, node->size, node->uptime, head->node_offset, head->seq,
//                        tail->node_offset, tail->next_offset, tail->seq);

    /*创建节点并插入节点*/
    tail->nextOffset = node->nodeOffset;
    node->nextOffset = node->nodeOffset;
    node->prevOffset = tail->nodeOffset;
    ct->head->tailOffset = node->nodeOffset;
    node->seq = tail->seq + 1;
    tail = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);

    node->size = size;
    node->tag = tag;
    node->uptime = GetMs();

//    LOGD("offset %u prev %u next %u seq %llu size %u time %lld head offset %u seq %llu tail offset %u tail next %u seq %llu\n", node->nodeOffset, node->prevOffset, node->nextOffset, node->seq, node->size, node->uptime, head->nodeOffset, head->seq,
//                    tail->nodeOffset, tail->nextOffset, tail->seq);

    /*get shm address */
    shmId = shmget(node->nodeKey, node->size, IPC_CREAT | 0666);
    if (shmId == -1)
    {
        ern = -errno;
        LOGE("get failed size %d key %x %s\n", node->size, node->nodeKey, strerror(-ern));
        node->nextOffset = node->prevOffset = 0;
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    addr = shmat(shmId, NULL, 0);
    if (addr == (void *) -1)
    {
        ern = -errno;
        node->nextOffset = node->prevOffset = 0;
        shmctl(shmId, IPC_RMID, NULL);
        LOGE("attach failed %s\n", strerror(-ern));
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    ct->head->shmNum++;
    ct->head->shmSize += size;

    nodeDataHeader.userDataLen = userSize;
    nodeDataHeader.dataLen = dataSize + headSize;
//    memset(nodeDataHeader.reserve,0,sizeof(nodeDataHeader.reserve));

    memcpy(addr, &nodeDataHeader,sizeof(SHM_QUEUE_NODE_DATA_HEAD_T));

    if (userData != NULL)
        memcpy((char *)addr + sizeof(SHM_QUEUE_NODE_DATA_HEAD_T), userData, userSize);

    if (headData != NULL)
        memcpy((char *)addr + sizeof(SHM_QUEUE_NODE_DATA_HEAD_T) + userSize, headData, headSize);

    memcpy((char *)addr + sizeof(SHM_QUEUE_NODE_DATA_HEAD_T) + userSize + headSize, data, dataSize);

    shmdt(addr);

    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
    NotifySend(ct);
    return 0;
}

static int ShmQueue_ReadData(void *streamHandle, void ** nodeHandle, void **attachAddr, void **userData, unsigned int *userSize,
                void **data,unsigned int *size, int timeout)
{

    int ern = 0, shmId = 0, ret = 0;
    long long unsigned int lastSeq = 0;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    SHM_QUEUE_NODE_T * node = NULL;
    SHM_QUEUE_NODE_DATA_HEAD_T *nodeDataHeader = NULL;
    unsigned int lockOffset = 0;
    void *addr = NULL;

    if (streamHandle == NULL || nodeHandle == NULL || attachAddr == NULL || data == NULL || size == NULL || timeout < 0)
    {
        LOGE("args error handle %p \n", streamHandle);
        ern = -EINVAL;
        return ern;
    }

    ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    node = *(SHM_QUEUE_NODE_T **) nodeHandle;
    if (ct->addr == NULL || ct->head->magic == 0)
    {
        LOGE("stream not available , stream maybe destroyed\n");
        return -EADDRNOTAVAIL;
    }
    ShmQueue_DumpNodeInfo(streamHandle);

    if (ct->head->shmNum == 0)
    {
        ret = ShmQueue_NotifyWait(ct->shmNotifyEpollFd,ct->shmNotifyFd,timeout);
        if (ct->head->shmNum == 0 || ret <= 0)
        {
//            LOGW("the queue is empty , wait timeout\n");
            return -ETIMEDOUT;
        }
    }

    if (*nodeHandle == NULL)
    {
        if (ct->head->shmElimination == 0)
        {
            if (ct->head->shmNum < ct->head->shmNumMax)
            {
                lockOffset = ct->head->headOffset;
                node = (SHM_QUEUE_NODE_T *)(ct->addr + lockOffset);

                while(node->nodeOffset != 0 && node->nextOffset != node->nodeOffset)
                {
                    if (node->uptime >= ct->uptime)
                    {
                        lockOffset = node->nodeOffset;
                        break;
                    }
                    else
                        node = (SHM_QUEUE_NODE_T *)(ct->addr + node->nextOffset);
                }
            }
            else
                lockOffset = ct->head->tailOffset;
        }
        else
        {
            lockOffset = ct->head->headOffset;
        }

        pthread_rwlock_rdlock(&ct->shmRwMutex);
        Common_File_Lock(ct->shmFlockFd, F_RDLCK, SEEK_SET, 0, 0, 0);
        node = (SHM_QUEUE_NODE_T *) (ct->addr + lockOffset);
//        LOGD("get tail node\n");
    }
    else
    {
        if (node->nextOffset == node->prevOffset && (node->nextOffset == 0 || node->nodeOffset == 0))
        {
            SHM_QUEUE_NODE_T * tmp_head_node = NULL, *tmp_tail_node = NULL;
            tmp_head_node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->headOffset);
            tmp_tail_node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
            LOGE("queue %s node was freed %llu head seq %llu tail seq %llu used node num %u used node size %u \n",
                            ct->head->streamName,node->seq, tmp_head_node->seq, tmp_tail_node->seq,ct->head->shmNum,ct->head->shmSize);
            *nodeHandle = NULL;
            return -EINVAL;
        }

        /*if there is no new data in shm queue , should be waiting*/
        if (node->nodeOffset == node->nextOffset)
        {
            if (timeout == 0)
                return -ETIMEDOUT;

            ret = ShmQueue_NotifyWait(ct->shmNotifyEpollFd,ct->shmNotifyFd,timeout);
            if (ct->head == NULL || ct->head->magic != STREAM_QUEUE_MAGIC_NUM)
            {
                LOGE("stream not available , stream destroyed\n");
                return -EADDRNOTAVAIL;
            }

            if (node->nodeOffset == node->nextOffset)
            {
//                LOGW("timeout %d %d\n",ret,timeout);
                return -ETIMEDOUT;
            }
        }
        pthread_rwlock_rdlock(&ct->shmRwMutex);
        Common_File_Lock(ct->shmFlockFd, F_RDLCK, SEEK_SET, 0, 0, 0);
        if (ct->addr == NULL || ct->head->magic != STREAM_QUEUE_MAGIC_NUM)
        {
            LOGE("stream not available , stream maybe destroyed\n");
            *nodeHandle = NULL;
            Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
            pthread_rwlock_unlock(&ct->shmRwMutex);
            return -EADDRNOTAVAIL;
        }
        if (ct->head->shmNum == 0)
        {
            Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
            pthread_rwlock_unlock(&ct->shmRwMutex);
            return -ETIMEDOUT;
        }
//        LOGD("node seq %llu\n", node->seq);
        if (node->nextOffset == 0 || node->nodeOffset == 0)
        {
            LOGE("stream not available , queue was reset or destroy\n");
            *nodeHandle = NULL;
            Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
            pthread_rwlock_unlock(&ct->shmRwMutex);
            return -EADDRNOTAVAIL;
        }

        lockOffset = node->nextOffset;
        lastSeq = node->seq;
        node = (SHM_QUEUE_NODE_T *) (ct->addr + node->nextOffset);
//        LOGD("next node seq %llu\n", node->seq);
    }

    *nodeHandle = (void *) node;

    /*get shm address */
    shmId = shmget(node->nodeKey, node->size, 0666);
    if (shmId == -1)
    {
        ern = -errno;
        LOGE("shmget failed key %x nodeoffset %d size %d seq %llu %s\n",
                        node->nodeKey, node->nodeOffset, node->size, node->seq,strerror(-ern));

        *nodeHandle = NULL;
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    addr = shmat(shmId, NULL, SHM_RDONLY);
    if (addr == (void *) -1)
    {
        ern = -errno;
        LOGE("attach key %x failed %s\n", node->nodeKey,strerror(-ern));
        *nodeHandle = NULL;
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0,0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return ern;
    }

    if (lastSeq != node->seq - 1 && lastSeq != node->seq && lastSeq != 0)
    {
        shmdt(addr);
        *nodeHandle = NULL;
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        LOGE("missing %llu %llu\n", lastSeq, node->seq);
        return -EIDRM;
    }

    nodeDataHeader = (SHM_QUEUE_NODE_DATA_HEAD_T *)addr;
    *attachAddr = addr;

    if (userData != NULL && userSize != NULL)
    {
        *userData = (char *)addr + sizeof(SHM_QUEUE_NODE_DATA_HEAD_T);
        *userSize = nodeDataHeader->userDataLen;
    }

    *size = node->size - nodeDataHeader->userDataLen - sizeof(SHM_QUEUE_NODE_DATA_HEAD_T);
    *data = (char *)addr + nodeDataHeader->userDataLen +  sizeof(SHM_QUEUE_NODE_DATA_HEAD_T);
    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);
//    LOGD("offset %u prev %u next %u seq %llu size %u time %lld\n",
//            node->nodeOffset, node->prevOffset, node->nextOffset, node->seq, node->size, node->uptime);
    return 0;
}

static int ShmQueue_ReleaseData(void *streamHandle, void **nodeHandle, void **attachAddr)
{
    int ern = 0;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    SHM_QUEUE_NODE_T * node = NULL;

    if (streamHandle == NULL || nodeHandle == NULL || *nodeHandle == NULL || attachAddr == NULL || *attachAddr == NULL)
    {
        LOGE(" args error\n");
        ern = -EINVAL;
        return ern;
    }

    ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    node = *(SHM_QUEUE_NODE_T **) nodeHandle;
    if (ct->addr == NULL || ct->head->magic == 0)
    {
        LOGE("stream not available , stream destroyed\n");
        return -EADDRNOTAVAIL;
    }

    if (ct->head->shmElimination == 1)
    {
        pthread_rwlock_wrlock(&ct->shmRwMutex);
        Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);

        SHM_QUEUE_NODE_T *head = NULL;
        int shmId = 0;
//            tail = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
        head = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->headOffset);

        /*
         * 这个节点是唯一的节点
         */
        if (node->nextOffset == node->nodeOffset)
        {
            ct->head->headOffset = ct->head->tailOffset = ct->head->nodeBeginOffset;
            if (ct->head->shmNum == 0)
                LOGE("num error\n");
            else
                ct->head->shmNum--;

            if (ct->head->shmSize < node->size)
            {
                ct->head->shmSize = 0;
                LOGE("size error\n");
            }
            else
                ct->head->shmSize -= node->size;

            node->nextOffset = node->prevOffset = 0;
            shmId = shmget(node->nodeKey, 1, 0666);
            shmctl(shmId, IPC_RMID, NULL);
            *nodeHandle = NULL;
        }
        else if (node->nextOffset == node->prevOffset && node->nextOffset == 0)
        {
            LOGD("no data in this node\n");
        }
        else
        {
            head = (SHM_QUEUE_NODE_T *) (ct->addr + node->nextOffset);
            head->prevOffset = head->nodeOffset;
            ct->head->headOffset = head->nodeOffset;
            ct->head->shmNum--;
            ct->head->shmSize -= node->size;
            node->prevOffset = node->nextOffset = 0;
            shmId = shmget(node->nodeKey, 1, 0666);
            shmctl(shmId, IPC_RMID, NULL);
            *nodeHandle = NULL;
        }
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
    }

    shmdt(*attachAddr);
    *attachAddr = NULL;
    return 0;
}

static int ShmQueue_ClearData(void *streamHandle)
{
    int ern = 0;
    SHM_QUEUE_CONTEXT_T *ct = NULL;

    if (streamHandle == NULL)
    {
        LOGE(" args error\n");
        //printf("[%s:%d] streamHandle is null\n",__FUNCTION__,__LINE__);
        ern = -EINVAL;
        return ern;
    }

    ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    if (ct->addr == NULL || ct->head->magic == 0)
    {
        LOGE("stream not available , stream destroyed\n");
        //printf("[%s:%d]\n",__FUNCTION__,__LINE__);
        return -EADDRNOTAVAIL;
    }

    if (ct->head->shmElimination == 1)
    {
        pthread_rwlock_wrlock(&ct->shmRwMutex);
        Common_File_Lock(ct->shmFlockFd, F_WRLCK, SEEK_SET, 0, 0, 0);

        SHM_QUEUE_NODE_T *node = NULL;
        int shmId = 0;
//            tail = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
        //node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->headOffset);
//printf("[%s:%d] shmNum:[%d] shmSize:[%d] nodeBeginOffset[%d] headOffset[%d]\n",__FUNCTION__,__LINE__,
//    ct->head->shmNum,ct->head->shmSize,ct->head->nodeBeginOffset,ct->head->headOffset);
        int shmNum = ct->head->shmNumMax;
        int nodesize = 0;
        while(shmNum > 0)
        {
            node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->nodeBeginOffset + sizeof(SHM_QUEUE_NODE_T) * ((shmNum-1) % ct->head->shmNumMax));
            //LOGD("cleardata shmNum[%d] nodeKey[%x] size[%d] [%d][%d][%d]\n",ct->head->shmNum-1,node->nodeKey,node->size,node->nodeOffset,node->prevOffset,node->nextOffset);



            shmNum--;
            shmId = shmget(node->nodeKey, 1, 0666);
            if(shmId == -1)
            {
                //空节点
                //LOGW("node is null\n");
                continue;
            }

            struct shmid_ds shmseg;
			if (shmctl(shmId, SHM_STAT, &shmseg) >= 0)
			{
                if (shmseg.shm_nattch > 0)
    			{
                    nodesize = node->size;
                    printf("node is attch [%d]\n",nodesize);
                    ct->head->headOffset = node->nodeOffset;
                    ct->head->tailOffset = node->nodeOffset;
                    node->prevOffset = node->nodeOffset;
                    node->nextOffset = node->nodeOffset;
                    continue;
                }

                 if (node->prevOffset == 0 && node->nextOffset == 0 && shmseg.shm_nattch == 0)
                 {
                    //LOGW("node is null! 2\n");
                    continue;
                 }
            }

            //ct->head->headOffset = node->nodeOffset;
            ct->head->shmNum--;
            ct->head->shmSize -= node->size;
            node->prevOffset = node->nextOffset = 0;

            shmctl(shmId, IPC_RMID, NULL);
            node = NULL;
        }

        //LOGW("shmNum:[%d] shmSize:[%d] nodesize[%d]\n", ct->head->shmNum,ct->head->shmSize,nodesize);
        if(nodesize)
        {
            ct->head->shmSize = nodesize;
        }
        else
        {
            ct->head->shmSize = 0;
            ct->head->headOffset = ct->head->tailOffset = ct->head->nodeBeginOffset;
        }
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
    }

    return 0;
}

static int ShmQueue_Control(void *streamHandle, int tag, void **nodeHandle)
{
    int ern = 0;
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    SHM_QUEUE_NODE_T * node = NULL, *pre = NULL;

    if (streamHandle == NULL || nodeHandle == NULL)
    {
        LOGE(" args error\n");
        ern = -EINVAL;
        return ern;
    }

    ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    if (ct->addr == NULL || ct->head->magic == 0)
    {
        LOGE("stream not available , stream destroyed\n");
        return -EADDRNOTAVAIL;
    }

    /*没有节点或者只有一个节点*/
    if (ct->head->tailOffset == ct->head->headOffset)
    {
        LOGD("not match\n");
        return -1;
    }

    pthread_rwlock_rdlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_RDLCK, SEEK_SET, 0, 0, 0);

    node = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
    pre = (SHM_QUEUE_NODE_T *) (ct->addr + node->prevOffset);
    while (node->tag != tag && node != pre)
    {
        node = (SHM_QUEUE_NODE_T *) (ct->addr + node->prevOffset);
        pre = (SHM_QUEUE_NODE_T *) (ct->addr + node->prevOffset);
    }

    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);

    if (node->tag != tag)
    {
        LOGD("not match\n");
        return -1;
    }

    *nodeHandle = pre;
    return 0;
}

static int ShmQueue_GetNodeCnt(void *streamHandle, void *nodeHandle)
{
    SHM_QUEUE_CONTEXT_T *ct = NULL;
    SHM_QUEUE_NODE_T * node = NULL, *head = NULL, *tail = NULL;
    int cnt = 0;
    ct = (SHM_QUEUE_CONTEXT_T *) streamHandle;
    node = (SHM_QUEUE_NODE_T *) nodeHandle;

    if (streamHandle == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -EINVAL;
    }

    pthread_rwlock_rdlock(&ct->shmRwMutex);
    Common_File_Lock(ct->shmFlockFd, F_RDLCK, SEEK_SET, 0, 0, 0);

    if (ct->addr == NULL || ct->head->magic == 0)
    {
        LOGE("invalid connection , stream maybe destroyed\n");
        Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
        pthread_rwlock_unlock(&ct->shmRwMutex);
        return -1;
    }

    tail = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->tailOffset);
    head = (SHM_QUEUE_NODE_T *) (ct->addr + ct->head->headOffset);

    if (node != NULL)
    {
        cnt = tail->seq - node->seq;
    }
    else
    {
        cnt = tail->seq - head->seq;
    }

    if (cnt < 0 || cnt > ct->head->shmNumMax + 1)
    {
        LOGE("availiable node cnt is %d max node cnt is %d\n",cnt,ct->head->shmNumMax);
        cnt = -1;
    }

    Common_File_Lock(ct->shmFlockFd, F_UNLCK, SEEK_SET, 0, 0, 0);
    pthread_rwlock_unlock(&ct->shmRwMutex);

    return cnt;
}

//static int GetStreamNameByStreamIdx(int streamIdx, char *streamName, int streamNameLen)
//{
//    DIR *sp_dp = NULL;
//    struct dirent *ptr = NULL;
//    int ret = -1;
//
//    sp_dp = opendir(STREAM_QUEUE_DIR);
//    if (sp_dp == NULL)
//    {
//        LOGE("open dir failed!\n", STREAM_QUEUE_DIR);
//        return -1;
//    }
//
//    while ((ptr = readdir(sp_dp)) != NULL)
//    {
//        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 || strcmp(ptr->d_name, "lock") == 0)
//        {
//            continue;
//        }
//        else
//        {
//            int idx = -1;
//            sscanf(ptr->d_name, "%*[^.].%04d", &idx);
//            if (idx == streamIdx && strlen(ptr->d_name) <= streamNameLen)
//            {
//                sscanf(ptr->d_name,"%[^.]",streamName);
////                snprintf(streamName, streamNameLen - 1, "%s", ptr->d_name);
//                ret = 0;
//                break;
//            }
//        }
//    }
//
//    closedir(sp_dp);
//    return ret;
//}

static int GetStreamIdxByStreamName(char *streamName)
{
    int streamIdx = -1, ern = 0;
    DIR *sp_dp = NULL;
    struct dirent *ptr = NULL;

    sp_dp = opendir(STREAM_QUEUE_DIR);
    if (sp_dp == NULL)
    {
        ern = -errno;
        LOGE("open dir failed! %s %s\n", STREAM_QUEUE_DIR,strerror(errno));
        return ern;
    }

    while ((ptr = readdir(sp_dp)) != NULL)
    {
        if (NULL == strstr(ptr->d_name, streamName) || strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0
                        || strcmp(ptr->d_name, "lock") == 0)
        {
            continue;
        }
        sscanf(ptr->d_name, "%*[^.].%04d", &streamIdx);
    }

    closedir(sp_dp);
    return streamIdx;
}

static int CreateShmIdx(char *streamName)
{
    int streamIdx = -1, ern = 0;
    DIR *sp_dp = NULL;
    struct dirent *ptr = NULL;
    char *fifoName = (char *)STREAM_MALLOC(STREAM_QUEUE_PATH_MAX);
    if (fifoName == NULL)
    {
        LOGE("calloc failed\n");
        return -ENOMEM;
    }
    memset(fifoName,0,STREAM_QUEUE_PATH_MAX);

    int streamFdLock = open(STREAM_QUEUE_LOCK, O_RDWR | O_NONBLOCK, 0666);
    if (streamFdLock < 0)
    {
        ern = -errno;
        LOGE("open stream lock failed %s\n",strerror(errno));
        STREAM_FREE(fifoName);
        return ern;
    }

    Common_File_Lock(streamFdLock, F_WRLCK, SEEK_SET, 0, 0, 0);
    sp_dp = opendir(STREAM_QUEUE_DIR);
    if (sp_dp == NULL)
    {
        ern = -errno;
        LOGE("open dir failed! %s %s\n", STREAM_QUEUE_DIR,strerror(errno));
        Common_File_Lock(streamFdLock, F_UNLCK, SEEK_SET, 0, 0, 0);
        close(streamFdLock);
        STREAM_FREE(fifoName);
        return ern;
    }

    while ((ptr = readdir(sp_dp)) != NULL)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 || strcmp(ptr->d_name, "lock") == 0)
        {
            continue;
        }
        else
        {
            int idx = -1;
            sscanf(ptr->d_name, "%*[^.].%04d", &idx);
            if (idx > streamIdx)
                streamIdx = idx;
        }
    }

    closedir(sp_dp);

    streamIdx += 1;
    snprintf(fifoName, STREAM_QUEUE_PATH_MAX - 1, "%s%s", STREAM_QUEUE_DIR, streamName);
    mkfifo(fifoName, S_IRWXU | S_IRWXG | S_IRWXO);
    memset(fifoName, 0, STREAM_QUEUE_PATH_MAX);
    snprintf(fifoName, STREAM_QUEUE_PATH_MAX - 1, "%s%s.%04d", STREAM_QUEUE_DIR, streamName, streamIdx);
    mkfifo(fifoName, S_IRWXU | S_IRWXG | S_IRWXO);
    Common_File_Lock(streamFdLock, F_UNLCK, SEEK_SET, 0, 0, 0);
    close(streamFdLock);
    STREAM_FREE(fifoName);
    return streamIdx;
}

static int DestroyShmIdx(char *streamName, int streamIdx)
{
    char *fifoName = (char *)STREAM_MALLOC(STREAM_QUEUE_PATH_MAX);
    if (fifoName == NULL)
    {
        LOGE("calloc failed\n");
        return -ENOMEM;
    }
    memset(fifoName,0,STREAM_QUEUE_PATH_MAX);
    int streamFdLock = open(STREAM_QUEUE_LOCK, O_RDWR | O_NONBLOCK, 0666);
    if (streamFdLock < 0)
    {
        LOGE("open stream lock failed\n");
        STREAM_FREE(fifoName);
        return -1;
    }

    Common_File_Lock(streamFdLock, F_WRLCK, SEEK_SET, 0, 0, 0);
    snprintf(fifoName, STREAM_QUEUE_PATH_MAX - 1, "%s%s", STREAM_QUEUE_DIR, streamName);
    unlink(fifoName);
    memset(fifoName, 0, STREAM_QUEUE_PATH_MAX);
    snprintf(fifoName, STREAM_QUEUE_PATH_MAX - 1, "%s%s.%04d", STREAM_QUEUE_DIR, streamName, streamIdx);
    unlink(fifoName);
    Common_File_Lock(streamFdLock, F_UNLCK, SEEK_SET, 0, 0, 0);
    close(streamFdLock);
    STREAM_FREE(fifoName);
    return 0;
}

static int SearchOpenNodeByFd(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;

    if (((STREAM_OPEN_NODE_T *) a)->shmFd == *(int *) b)
        return 0;

    return -1;
}

int StreamQueue_Open(char *streamQueueName,int openFlag,int maxMemSize,int maxNodeNum)
{
    if (streamQueueName == NULL || strlen(streamQueueName) >= STREAM_QUEUE_NAME_MAX || maxMemSize > STREAM_QUEUE_MAX_SIZE
                    || maxMemSize < 0 || maxNodeNum > STREAM_QUEUE_MAX_NUM || maxNodeNum < 0)
        return -EINVAL;

    int streamIdx = -1, ern = 0;
    char *streamFullName = (char *)STREAM_MALLOC(STREAM_QUEUE_PATH_MAX);
    if (streamFullName == NULL)
    {
        LOGE("calloc failed\n");
        return -ENOMEM;
    }
    memset(streamFullName,0,STREAM_QUEUE_PATH_MAX);
    STREAM_OPEN_NODE_T *node = NULL;

    if (access(STREAM_QUEUE_LOCK, F_OK) != 0)
    {
        mkdir(STREAM_QUEUE_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkfifo(STREAM_QUEUE_LOCK, S_IRWXU | S_IRWXG | S_IRWXO);
    }

    if (s_openStreamList == NULL)
    {
        Common_DList_Init(&s_openStreamList,ListNodeFree);
    }

    snprintf(streamFullName, STREAM_QUEUE_PATH_MAX - 1, "%s%s", STREAM_QUEUE_DIR, streamQueueName);
    /*connect stream queue*/

    if ((openFlag & STREAM_QUEUE_OPEN_FLAG_CREATE) == 0)
    {
        if (access(streamFullName, F_OK) != 0)
        {
            LOGE("stream queue was not exist, %s\n", streamFullName);
            STREAM_FREE(streamFullName);
            return -EINVAL;
        }

        STREAM_FREE(streamFullName);

        streamIdx = GetStreamIdxByStreamName(streamQueueName);
        if (streamIdx < 0)
        {
            LOGE("get stream idx failed\n");

            return streamIdx;
        }

        node = (STREAM_OPEN_NODE_T *)STREAM_MALLOC(sizeof(STREAM_OPEN_NODE_T));
        node->streamIdx = streamIdx;
        snprintf(node->streamName, sizeof(node->streamName) - 1, "%s", streamQueueName);
        node->shmHandle = NULL;
        node->openFlag = openFlag;
        node->shmFd = 0;
        node->shmNodeHandle = NULL;
        node->attachAddr = NULL;

        if ((node->shmFd = ShmQueue_Init(&node->shmHandle, node->streamName, node->streamIdx)) < 0)
        {
            ern = node->shmFd;
            DestroyShmIdx(node->streamName, node->streamIdx);
            STREAM_FREE(node);
            return ern;
        }

        if ((ern = ShmQueue_ConnectShm(node->shmHandle)) < 0)
        {
            ShmQueue_Uninit(node->shmHandle);
            DestroyShmIdx(node->streamName, node->streamIdx);
            STREAM_FREE(node);
            return ern;
        }

    }
    /*create stream queue*/
    else
    {
        if (access(streamFullName, F_OK) != 0)
        {
            STREAM_FREE(streamFullName);
            streamIdx = CreateShmIdx(streamQueueName);
            if (streamIdx < 0)
            {
                LOGE("create stream idx error\n");
                return streamIdx;
            }

        }
        /*当前的streamname这个文件已经存在，不做删除动作，继续使用*/
        else
        {
            STREAM_FREE(streamFullName);
            streamIdx = GetStreamIdxByStreamName(streamQueueName);
            if (streamIdx < 0)
            {
                LOGE("get stream idx failed\n");
                return streamIdx;
            }
        }

        node = (STREAM_OPEN_NODE_T *)STREAM_MALLOC(sizeof(STREAM_OPEN_NODE_T));
        node->streamIdx = streamIdx;
        snprintf(node->streamName, sizeof(node->streamName) - 1, "%s", streamQueueName);
        node->shmHandle = NULL;
        node->shmNodeHandle = NULL;
        node->openFlag = openFlag;
        node->shmFd = 0;

        if ((node->shmFd = ShmQueue_Init(&node->shmHandle, node->streamName, node->streamIdx)) < 0)
        {
            ern = node->shmFd;
            STREAM_FREE(node);
            return ern;
        }

        if ((ern = ShmQueue_CreateShm(node->shmHandle,maxMemSize,maxNodeNum)) < 0)
        {
            ShmQueue_Uninit(node->shmHandle);
            STREAM_FREE(node);
            return ern;
        }

        if ((ern = ShmQueue_SetElimination(node->shmHandle, openFlag)) < 0)
        {
            ShmQueue_Uninit(node->shmHandle);
            DestroyShmIdx(node->streamName, node->streamIdx);
            STREAM_FREE(node);
            return ern;
        }
    }

    Common_DList_InsertTail(s_openStreamList, node, sizeof(STREAM_OPEN_NODE_T));
    LOGD("open stream done name %s fd %d\n", streamQueueName, node->shmFd);
    return node->shmFd;
}

int StreamQueue_Close(int streamQueueHandle)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0)
    {
        LOGE("streamQueueHandle error\n");
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened\n");
        return -EINVAL;
    }

    /*if this stream was opened by other one , just disconnect it , or destroy it*/
    if (ShmQueue_GetShmConnCnt(node->shmHandle) > 1)
    {
        ShmQueue_DisconnectShm(node->shmHandle);
    }
    else
    {
        ShmQueue_DestroyShm(node->shmHandle);
        DestroyShmIdx(node->streamName, node->streamIdx);
    }

    ShmQueue_Uninit(node->shmHandle);
    Common_DList_Delete(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);

    return 0;
}

int StreamQueue_WriteData(int streamQueueHandle, void *userData, unsigned int userSize, void *headData,
                unsigned int headSize, void *data, unsigned int dataSize, int tag)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0 || data == NULL || dataSize <= 0)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened\n");
        return -EINVAL;
    }
    else if ((node->openFlag & STREAM_QUEUE_OPEN_FLAG_WRITE) == 0)
    {
        LOGE("have no right to write\n");
        return -EINVAL;
    }

    return ShmQueue_WriteData(node->shmHandle, userData, userSize, headData, headSize, data, dataSize, tag);
}

int StreamQueue_ReadData(int streamQueueHandle, void **userData, unsigned int *userSize, void **data,
                unsigned int *size, unsigned int timeout)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0)
    {
        LOGE("streamQueueHandle error, %d\n",streamQueueHandle);
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened, %d\n",streamQueueHandle);
        return -EINVAL;
    }
    else if ((node->openFlag & STREAM_QUEUE_OPEN_FLAG_READ) == 0)
    {
        LOGE("have no right to read, %d\n",node->openFlag);
        return -EINVAL;
    }

    if (node->shmHandle && node->shmNodeHandle && node->attachAddr)
        ShmQueue_ReleaseData(node->shmHandle, &node->shmNodeHandle, &node->attachAddr);

//    if (timeout == 0)
//        timeout = 10;

    return ShmQueue_ReadData(node->shmHandle, &node->shmNodeHandle, &node->attachAddr, userData, userSize, data, size, timeout);
}

int StreamQueue_ReleaseData(int streamQueueHandle)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0)
    {
        LOGE("streamQueueHandle error\n");
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened %d\n",streamQueueHandle);
        return -EINVAL;
    }
    return ShmQueue_ReleaseData(node->shmHandle, &node->shmNodeHandle, &node->attachAddr);
}

int StreamQueue_ClearData(int streamQueueHandle)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0)
    {
        LOGE("streamQueueHandle error\n");
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened %d\n",streamQueueHandle);
        //printf("handle not opened\n");
        return -EINVAL;
    }

    if(node->shmNodeHandle != NULL)
    {
        SHM_QUEUE_NODE_T *queue_node = (SHM_QUEUE_NODE_T *) node->shmNodeHandle;
        //printf("[%s][%d] nodeKey[%d] size[%d]\n",__FUNCTION__,__LINE__,queue_node->nodeKey,queue_node->size);
    }
    else
    {
        //printf("[%s][%d] shmNodeHandle is null\n",__FUNCTION__,__LINE__);
    }

    return ShmQueue_ClearData(node->shmHandle);
}

int StreamQueue_GetRestCnt(int streamQueueHandle)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0)
    {
        LOGE("streamQueueHandle error\n");
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened %d\n",streamQueueHandle);
        return -EINVAL;
    }

    return ShmQueue_GetNodeCnt(node->shmHandle, node->shmNodeHandle);
}

int StreamQueue_Control(int streamQueueHandle, int tag)
{
    STREAM_OPEN_NODE_T *node = NULL;

    if (streamQueueHandle < 0)
    {
        LOGE("streamQueueHandle error\n");
        return -EINVAL;
    }

    node = (STREAM_OPEN_NODE_T *)Common_DList_Search(s_openStreamList, &streamQueueHandle, SearchOpenNodeByFd);
    if (node == NULL)
    {
        LOGE("this handle was not opened\n");
        return -EINVAL;
    }

//    if (node->shmNodeHandle != NULL)
//    {
//        LOGE("please release data firstly\n");
//        return -EINVAL;
//    }

    return ShmQueue_Control(node->shmHandle, tag, &node->shmNodeHandle);
}


int StreamQueue_EpollCreate()
{
    int i = 0,ep = 0;
    STREAMQUEUE_EPOLL_EVNET_MGR_T *pEvent = NULL;
    if(g_EpollEventLock == NULL)
    {
        Common_Lock_Create(&g_EpollEventLock, NULL);
        memset(g_EpollEvent,0,sizeof(g_EpollEvent));
    }
    Common_Lock(g_EpollEventLock);
    for(i = 0; i < MAX_STREAMQUEUE_EPOLL_NUM;i++)
    {
        if(g_EpollEvent[i] == NULL)
        {
            pEvent= (STREAMQUEUE_EPOLL_EVNET_MGR_T *)Common_Malloc(sizeof(STREAMQUEUE_EPOLL_EVNET_MGR_T), 0,NULL,0);
            break;
        }
    }
    if(pEvent == NULL)
    {
        Common_UnLock(g_EpollEventLock);
        return -1;
    }
    memset(pEvent,0,sizeof(STREAMQUEUE_EPOLL_EVNET_MGR_T));
    ep = epoll_create(10);
    if(ep < 0)
    {
        Common_UnLock(g_EpollEventLock);
        Common_Free(pEvent,NULL,0);
        return ep;
    }
    pEvent->epoll_handle = ep;

    g_EpollEvent[i] = pEvent;
    Common_UnLock(g_EpollEventLock);
    return i + 1;
}

int StreamQueue_EpollDestroy(int epollHandle)
{
    int i = 0,nCount = 0;
    STREAMQUEUE_EPOLL_EVNET_MGR_T *pEvent = NULL;
    if(epollHandle < 1 || epollHandle > MAX_STREAMQUEUE_EPOLL_NUM )
    {
        return -1;
    }
    epollHandle--;
    Common_Lock(g_EpollEventLock);
    if(g_EpollEvent[epollHandle] == NULL)
    {
        Common_UnLock(g_EpollEventLock);
        return 0;
    }
    pEvent = g_EpollEvent[epollHandle];
    g_EpollEvent[epollHandle] = NULL;
    close(pEvent->epoll_handle);
    for(i = 0; i < MAX_STREAMQUEUE_EPOLL_EVNET_PTR_NUM && nCount < pEvent->nPtrArrayCount;i++)
    {
        if(pEvent->pPtrArray[i] != NULL)
        {
            Common_Free(pEvent->pPtrArray[i],NULL,0);
            pEvent->pPtrArray[i] = NULL;
            nCount++;
        }
    }
    Common_Free(pEvent,NULL,0);
    Common_UnLock(g_EpollEventLock);

    return 0;
}

int StreamQueue_EpollCtl(int epollHandle, int ctl, STREAM_QUEUE_EPOLL_EVENT_T *events)
{
    int i,nCount = 0,iFreeIdx = -1,nFoundIdx = -1,nRet = -1;
    struct epoll_event ep;
    STREAMQUEUE_EPOLL_EVNET_MGR_T *pEvent = NULL;
    STREAM_QUEUE_EPOLL_EVENT_T *pCheckEvents = NULL;
    if (epollHandle <= 0 || ctl < STREAM_QUEUE_EPOLL_CTL_ADD || ctl > STREAM_QUEUE_EPOLL_CTL_MDD || events == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    if(epollHandle < 1 || epollHandle > MAX_STREAMQUEUE_EPOLL_NUM )
    {
        return -EINVAL;
    }
    epollHandle--;
    Common_Lock(g_EpollEventLock);
    if(g_EpollEvent[epollHandle] == NULL)
    {
        Common_UnLock(g_EpollEventLock);
        return -EINVAL;
    }
    pEvent = g_EpollEvent[epollHandle];
    epollHandle = pEvent->epoll_handle;
    /////
    for(i = 0 ;i <  MAX_STREAMQUEUE_EPOLL_EVNET_PTR_NUM && (iFreeIdx == -1 || nCount < pEvent->nPtrArrayCount);i++)
    {

        if(pEvent->pPtrArray[i] != NULL)
        {
            pCheckEvents = (STREAM_QUEUE_EPOLL_EVENT_T *)pEvent->pPtrArray[i];
            if(pCheckEvents->streamQueueHandle == events->streamQueueHandle)
            {
                nFoundIdx = i;
                break;
            }
            pCheckEvents = NULL;

            nCount++;
        }
        else if(iFreeIdx == -1)
        {
            iFreeIdx = i;
        }
    }

    if(ctl == STREAM_QUEUE_EPOLL_CTL_ADD ||
       ctl == STREAM_QUEUE_EPOLL_CTL_MDD)
    {
        if(pCheckEvents == NULL)
        {
            if(iFreeIdx == -1)
            {
                Common_UnLock(g_EpollEventLock);
                return -EINVAL;
            }
            pCheckEvents = (STREAM_QUEUE_EPOLL_EVENT_T *)Common_Malloc(sizeof(STREAM_QUEUE_EPOLL_EVENT_T),0,NULL,0);
            if(pCheckEvents == NULL)
            {
                Common_UnLock(g_EpollEventLock);
                return -EINVAL;
            }
            memset(pCheckEvents,0,sizeof(STREAM_QUEUE_EPOLL_EVENT_T));
            pEvent->pPtrArray[iFreeIdx] = pCheckEvents;
            pEvent->nPtrArrayCount++;
            nFoundIdx = iFreeIdx;
        }
        memcpy(pCheckEvents,events,sizeof(STREAM_QUEUE_EPOLL_EVENT_T));
        ep.data.u32 = nFoundIdx + 1;
    }
    else
    {
        if(pCheckEvents != NULL)
        {
            pEvent->pPtrArray[nFoundIdx] = NULL;
            pEvent->nPtrArrayCount--;
            Common_Free(pCheckEvents,NULL,0);
        }
        ep.data.u32 = 0;
    }
    Common_UnLock(g_EpollEventLock);

    ep.events = EPOLLHUP | EPOLLERR | EPOLLET;



    if (events->event == STREAM_QUEUE_EPOLL_EVENT_READ)
        ep.events = ep.events | EPOLLIN;
    if (events->event == STREAM_QUEUE_EPOLL_EVENT_WRITE)
        ep.events = ep.events | EPOLLOUT;
    if (events->event == STREAM_QUEUE_EPOLL_EVENT_RW)
        ep.events = ep.events | EPOLLIN | EPOLLOUT;

    nRet = epoll_ctl(epollHandle, ctl, events->streamQueueHandle, &ep);
    if(nRet != 0)
    {
        if(ctl == STREAM_QUEUE_EPOLL_CTL_ADD)
        {
            Common_Lock(g_EpollEventLock);
            if(pEvent->pPtrArray[nFoundIdx] != NULL)
            {
                Common_Free(pEvent->pPtrArray[nFoundIdx],NULL,0);
                pEvent->pPtrArray[nFoundIdx] = NULL;
                pEvent->nPtrArrayCount--;
            }
            Common_UnLock(g_EpollEventLock);
        }
    }
    return nRet;
}

int StreamQueue_EpollWait(int epollHandle, STREAM_QUEUE_EPOLL_EVENT_T *events, int maxEvents, int timeout)
{
    STREAMQUEUE_EPOLL_EVNET_MGR_T *pEvent = NULL;

    if (epollHandle <= 0 || events == NULL || maxEvents < 1 || (timeout < 0 && timeout != -1))
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    if(epollHandle < 1 || epollHandle > MAX_STREAMQUEUE_EPOLL_NUM)
    {
        return -EINVAL;
    }
    epollHandle--;
    Common_Lock(g_EpollEventLock);
    if(g_EpollEvent[epollHandle] == NULL)
    {
        Common_UnLock(g_EpollEventLock);
        return -EINVAL;
    }
    pEvent = g_EpollEvent[epollHandle];
    epollHandle = pEvent->epoll_handle;
    Common_UnLock(g_EpollEventLock);

    struct epoll_event *ev = (struct epoll_event *)STREAM_MALLOC(sizeof(struct epoll_event)*maxEvents);
    if(ev == NULL)
    {
        LOGE("%s\n", strerror(ENOMEM));
        return -ENOMEM;
    }

    memset(ev,0,sizeof(struct epoll_event)*maxEvents);
    int evNum = 0, i = 0;

    evNum = epoll_wait(epollHandle,ev,maxEvents,timeout);

    /*timeout*/
    if (evNum == 0)
    {

        STREAM_FREE(ev);
        return 0;
    }

    /*error happen*/
    if (evNum < 0)
    {
        STREAM_FREE(ev);
        LOGE("%s\n",strerror(errno));
        return -errno;
    }

    for (i = 0; i < evNum; i++)
    {
        if ((ev[i].events & EPOLLERR) || (ev[i].events & EPOLLHUP))
        {
            LOGW("event fd error %d num %d\n", ev[i].events, evNum);
            evNum = i - 1;
            break;
        }
        else
        {
            memset(&events[i],0,sizeof(STREAM_QUEUE_EPOLL_EVENT_T));
            if(ev[i].data.u32 > 0)
            {
                Common_Lock(g_EpollEventLock);
                STREAM_QUEUE_EPOLL_EVENT_T *pCheckEvents = (STREAM_QUEUE_EPOLL_EVENT_T *)pEvent->pPtrArray[ev[i].data.u32 - 1];
                if(pCheckEvents != NULL)
                {
                    memcpy(&events[i],pCheckEvents,sizeof(STREAM_QUEUE_EPOLL_EVENT_T));
                }
                Common_UnLock(g_EpollEventLock);
            }



            if (events[i].streamQueueHandle == 0)
            {
                LOGW("event %x i %d timeout %d evNum %d\n",ev[i].events,i, timeout,evNum);
                continue;
            }

            if (ev[i].events & EPOLLOUT)
            {
                events[i].event = STREAM_QUEUE_EPOLL_EVENT_WRITE;
            }

            if (ev[i].events & EPOLLIN)
            {
                events[i].event = STREAM_QUEUE_EPOLL_EVENT_READ;
            }

            if((ev[i].events & EPOLLIN) && (ev[i].events & EPOLLOUT))
            {
                events[i].event = STREAM_QUEUE_EPOLL_EVENT_RW;
            }
        }
    }

    STREAM_FREE(ev);
    return evNum;
}

#endif
