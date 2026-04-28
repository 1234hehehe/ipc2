/*
 * ovfs_com_mq.c
 *
 *  Created on: 2016年4月8日
 *      Author: eric
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include <libcommon_api.h>
#include "ovfs_com_mq.h"

#define    MQ_MALLOC(x)              Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define    MQ_FREE(x)                Common_Free(x,__func__,__LINE__)
#define    MQ_STRDUP(x)              Common_StrDup(x,__func__,__LINE__)
enum
{
    MQ_TYPE_DATA,   // 进程间通讯 IPC 消息
    MQ_TYPE_TIMER, //定时器消息
    MQ_TYPE_MSG,   //线程间通讯消息报告
    MQ_TYPE_REQ,    //线程间同步请求
};

/* 消息节点*/
typedef struct
{
    unsigned int type; //消息类型
    unsigned int id;     // 消息ID 自定义
    unsigned int size;  // 消息大小
    unsigned int token; //IPC 数据token
    unsigned int subType; // IPC 数据类型

    void *reqInData;
    void *reqOutData;
    unsigned int reqInSize;
    unsigned int reqOutSize;
    unsigned int reqFlag;
    pthread_mutex_t reqMutex;
    pthread_cond_t reqCond;
} MQ_NODE_T;

/* 消息队列handle*/
typedef struct
{
    int init;                        //是否初始化
    COMMON_DLIST_T list;                    //消息队列链表
    int queueMaxLength;              //消息队列链表最大长度
    pthread_mutex_t mutex;
    pthread_mutex_t mutexHandle;
    pthread_cond_t cond;
    unsigned int
    timeout;            //从消息队列中获取消息超时时长, 0表示一直等待新的消息进入消息队列
    int state;                       //消息队列状态 , 0 表示空闲 , 1 表示繁忙
    long int mainLoopTid;
    MQ_REQ_F ReqCallback;
} MQ_CONTEXT_T;

/*
 * 名称:    found_timer_by_id_callback
 *              通过timer id查找消息队列节点
 * 参数:
 * 返回:
 */
static int FoundTimerByIdCallback(void *a, void *b)
{
    MQ_NODE_T *node = (MQ_NODE_T *) a;
    unsigned int *timerId = (unsigned int *) b;
    if (a == NULL || b == NULL)
        return -1;

    if (node->type == MQ_TYPE_TIMER && node->id == *timerId)
        return 0;

    return -1;
}

/*
 * 名称:    found_node_by_addr_callback
 *              通过消息节点地址查找消息队列节点
 * 参数:
 * 返回:
 */
static int FoundNodeByAddrCallback(void *a, void *b)
{
    MQ_NODE_T *nodeA = (MQ_NODE_T *) a;
    MQ_NODE_T *nodeB = (MQ_NODE_T *) b;

    if (nodeA == nodeB)
        return 0;

    return -1;
}

/*
 * 名称:    MqSend
 *              将一个新的消息插入消息队列
 * 参数:
 * 返回:
 */
static int MqSend(MQ_HANDLE_H mqHandle, MQ_NODE_T *node, unsigned int size)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    int count = 0;

    if ((mqHandle == NULL) || node == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    if (ct->init == 0)
    {
        LOGE("message queue is not init \n");
        MQ_FREE(node);
        return -1;
    }

    count = Common_DList_GetCount(ct->list);

    /* 判断消息队列当前状态,如果消息队列长度大于设定的上限,则消息队列设置状态为繁忙
     * 如果当前消息队列状态为繁忙,此时消息队列长度小于设定的下限,则消息队列设置为空闲*/
    if (ct->state == 0 && count > OVFS_MQ_QUEUE_WARN_MAX)
    {
        ct->state = 1;
    }

    if (ct->state == 1 && count < OVFS_MQ_QUEUE_WARN_MIN)
    {
        ct->state = 0;
    }

    /* 如果消息队列当前长度大于队列最大值,并且此消息为IPC消息时,将丢掉这条消息 */
    if (count >= ct->queueMaxLength && node->type == MQ_TYPE_DATA)
    {
        LOGE("message queue is full , will drop this ipc data . type %u id %u ipc_type %u token %u.  list max is %u\n",
             node->type, node->id, node->subType, node->token, ct->queueMaxLength);
        MQ_FREE(node);
        return -1;
    }
    else if (count >= ct->queueMaxLength)
    {
        LOGW("message queue is full, but still insert data \n");
    }

    Common_DList_InsertTail(ct->list, node, size);
    pthread_mutex_lock(&ct->mutex);
    pthread_cond_signal(&ct->cond);
    pthread_mutex_unlock(&ct->mutex);
    return 0;
}

/*
 * 名称:    MqRecv
 *              从消息队列中取出一个消息
 * 参数:
 * 返回:
 */
static MQ_NODE_T *MqRecv(MQ_HANDLE_H mqHandle)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    struct timespec tv;

    if (mqHandle == NULL)
    {
        LOGE("message handle is null\n");
        return NULL;
    }

    if (ct->init == 0)
    {
        LOGE("message queue is not init\n");
        return NULL;
    }

    MQ_NODE_T *node = NULL;
    node = (MQ_NODE_T *) Common_DList_GetFirst(ct->list);
    if (node == NULL)
    {
        if (ct->timeout > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &tv);
            tv.tv_nsec = tv.tv_nsec + (ct->timeout % 1000) * 1000L * 1000L;
            tv.tv_sec = tv.tv_sec + ct->timeout / 1000;
            if (tv.tv_nsec > 1000000000L)
            {
                tv.tv_nsec = tv.tv_nsec % 1000000000L;
                tv.tv_sec++;
            }
            pthread_mutex_lock(&ct->mutex);
            pthread_cond_timedwait(&ct->cond, &ct->mutex, &tv);
            pthread_mutex_unlock(&ct->mutex);
        }
        else
        {
            pthread_mutex_lock(&ct->mutex);
            pthread_cond_wait(&ct->cond, &ct->mutex);
            pthread_mutex_unlock(&ct->mutex);
        }

        node = (MQ_NODE_T *) Common_DList_GetFirst(ct->list);
    }
    return node;
}

static void DlistNodeFreeFun(void *addr)
{
    if (addr)
        MQ_FREE(addr);
    else
        LOGW("try to free NULL addr\n");
}

int Mq_Init(MQ_HANDLE_H *mqHandle, unsigned int timeout)
{
    int ern = 0;
    MQ_CONTEXT_T *ct = NULL;
    ct = (MQ_CONTEXT_T *) MQ_MALLOC(sizeof(MQ_CONTEXT_T));
    if (ct == NULL)
    {
        ern = -errno;
        LOGE("calloc failed %d %s\n", errno, strerror(errno));
        return ern;
    }

    memset(ct, 0, sizeof(MQ_CONTEXT_T));
    (*mqHandle) = (MQ_HANDLE_H) ct;
    Common_DList_Init(&ct->list, DlistNodeFreeFun);
    ct->timeout = timeout;
    ct->queueMaxLength = OVFS_MQ_QUEUE_MIN_LEN;

    pthread_condattr_t condattr;

    pthread_mutex_init(&ct->mutex, NULL);
    pthread_mutex_init(&ct->mutexHandle, NULL);
    pthread_condattr_init(&condattr);
    pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&ct->cond, &condattr);
    pthread_condattr_destroy(&condattr);

    ct->init = 1;
    return 0;
}

int Mq_SetMaxLength(MQ_HANDLE_H mqHandle, int length)
{
    if (mqHandle == NULL || length < OVFS_MQ_QUEUE_MIN_LEN)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    ct->queueMaxLength = length;
    return 0;
}

int Mq_GetMaxLength(MQ_HANDLE_H mqHandle)
{
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    return ct->queueMaxLength;
}

int MqGetLength(MQ_HANDLE_H mqHandle)
{
    int ret = 0;
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    ret = Common_DList_GetCount(ct->list);

    return ret;
}

void Mq_Uninit(MQ_HANDLE_H *mqHandle)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) (*mqHandle);
    if (ct == NULL)
    {
        LOGE("mqHandle is NULL\n");
        return;
    }
    if (ct->list)
    {
        Common_DList_Uninit(&ct->list);
        pthread_mutex_destroy(&ct->mutex);
        pthread_cond_destroy(&ct->cond);
    }

    if (ct)
    {
        MQ_FREE(ct);
        *mqHandle = NULL;
    }
}

void Mq_DelTimer(MQ_HANDLE_H mqHandle, unsigned int timerId)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    void *node = NULL;

    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return;
    }
    node = Common_DList_Search(ct->list, &timerId, FoundTimerByIdCallback);

    if (node != NULL)
    {
        Common_DList_Delete(ct->list, &timerId, FoundTimerByIdCallback);
    }
}

int Mq_PostIpcData(MQ_HANDLE_H mqHandle, unsigned char ipcType,
                   unsigned int appid, unsigned int token, void *data, unsigned int size)
{
    void *cdata = NULL;

    if (mqHandle == NULL || data == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    MQ_NODE_T *node = (MQ_NODE_T *) MQ_MALLOC(sizeof(MQ_NODE_T) + size);
    if (node == NULL)
    {
        LOGE("malloc failed\n");
        return -ENOMEM;
    }

    cdata = (char *) node + sizeof(MQ_NODE_T);
    memcpy(cdata, data, size);
    node->size = size;
    node->type = MQ_TYPE_DATA;
    node->id = appid;
    node->token = token;
    node->subType = ipcType;
    node->reqInData = node->reqOutData = NULL;
    node->reqInSize = node->reqOutSize = 0;

    MqSend(mqHandle, node, size + sizeof(MQ_NODE_T));

    return 0;
}

int Mq_PostIpcEvent(MQ_HANDLE_H mqHandle, unsigned char ipcType,
                    unsigned int appid, void *data, unsigned int size)
{
    void *cdata = NULL;
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    MQ_NODE_T *node = (MQ_NODE_T *) MQ_MALLOC(sizeof(MQ_NODE_T) + size);
    if (node == NULL)
    {
        LOGE("malloc failed\n");
        return -ENOMEM;
    }

    if (data != NULL && size > 0)
    {
        cdata = (char *) node + sizeof(MQ_NODE_T);
        memcpy(cdata, data, size);
    }

    node->size = size;
    node->type = MQ_TYPE_DATA;
    node->subType = ipcType;
    node->id = appid;
    node->reqInData = node->reqOutData = NULL;
    node->reqInSize = node->reqOutSize = 0;
    MqSend(mqHandle, node, size + sizeof(MQ_NODE_T));
    return 0;
}

int Mq_PostTimer(MQ_HANDLE_H mqHandle, unsigned int id)
{
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    MQ_NODE_T *node = (MQ_NODE_T *) MQ_MALLOC(sizeof(MQ_NODE_T));
    if (node == NULL)
    {
        LOGE("malloc failed\n");
        return -ENOMEM;
    }

    node->size = 0;
    node->type = MQ_TYPE_TIMER;
    node->id = id;
    node->token = 0;
    node->reqInData = node->reqOutData = NULL;
    node->reqInSize = node->reqOutSize = 0;

    return MqSend(mqHandle, node, sizeof(MQ_NODE_T));
}

int Mq_PostMsg(MQ_HANDLE_H handle, unsigned int id, void *data,
               unsigned int size)
{
    void *cdata = NULL;

    if (handle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }
    MQ_NODE_T *node = (MQ_NODE_T *) MQ_MALLOC(sizeof(MQ_NODE_T) + size);
    if (node == NULL)
    {
        LOGE("malloc failed\n");
        return -ENOMEM;
    }

    cdata = (char *) node + sizeof(MQ_NODE_T);
    if (data && size > 0)
    {
        memcpy(cdata, data, size);
        node->size = size;
    }
    else
        node->size = 0;

    node->type = MQ_TYPE_MSG;
    node->id = id;
    node->token = 0;
    node->reqInData = node->reqOutData = NULL;
    node->reqInSize = node->reqOutSize = 0;

    return MqSend(handle, node, size + sizeof(MQ_NODE_T));
}

int Mq_Request(MQ_HANDLE_H mqHandle, unsigned int id, void *reqInData,
               unsigned int reqInSize, int *reqRet, void *reqOutData, unsigned int reqOutSize)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;

    if (mqHandle == NULL || ct->ReqCallback == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    if (syscall(SYS_gettid) == ct->mainLoopTid)
    {
        LOGE("please do not doing request in main loop thread\n");
        return -1;
    }

    pthread_mutex_lock(&ct->mutexHandle);
    ct->ReqCallback(id, reqInData, reqInSize, reqRet, reqOutData, reqOutSize);
    pthread_mutex_unlock(&ct->mutexHandle);

    return 0;
}

int Mq_CheckState(MQ_HANDLE_H mqHandle)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    if (ct == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    return ct->state;
}

void Mq_QuitLoop(MQ_HANDLE_H mqHandle)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    if (ct == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return;
    }
    ct->init = 0;

    pthread_mutex_lock(&ct->mutex);
    pthread_cond_signal(&ct->cond);
    pthread_mutex_unlock(&ct->mutex);

}
void Mq_MsgLoop(MQ_HANDLE_H mqHandle, MQ_DATA_F DataCallback,
                MQ_TIMER_F TimerCallback,
                MQ_MSG_F MsgCallback, MQ_REQ_F ReqCallback)
{
    MQ_CONTEXT_T *ct = (MQ_CONTEXT_T *) mqHandle;
    if (ct == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return;
    }

    ct->mainLoopTid = syscall(SYS_gettid);
    ct->ReqCallback = ReqCallback;

    while (ct->init)
    {
        /* 从消息队列中取出一条消息*/
        MQ_NODE_T *node = MqRecv(mqHandle);
        void *data = NULL;
        if (node == NULL)
            continue;

//        LOGD("recv mq data type %u ipc_type %u %u\n", node->type, node->subType, node->token);

        pthread_mutex_lock(&ct->mutexHandle);
        if (node->type == MQ_TYPE_DATA && DataCallback)
        {
            data = ((char *) node + sizeof(MQ_NODE_T));
            DataCallback(node->id, node->subType, node->token, data, node->size);
        }
        else if (node->type == MQ_TYPE_TIMER && TimerCallback)
        {
            TimerCallback(node->id);
        }
        else if (node->type == MQ_TYPE_MSG && MsgCallback)
        {
            if (node->size != 0)
            {
                data = ((char *) node + sizeof(MQ_NODE_T));
                MsgCallback(node->id, data, node->size);
            }
            else
            {
                MsgCallback(node->id, NULL, 0);
            }
        }
        else
        {
            LOGE("unknow mq type %d or callback function is null\n", node->type);
        }
        pthread_mutex_unlock(&ct->mutexHandle);
        /*释放这条消息*/
        if (ct->list)
        {
            Common_DList_Delete(ct->list, node, FoundNodeByAddrCallback);
        }
        else
        {
            LOGE("ct or ct list is null\n");
            break;
        }

//        LOGD("handle mq data done\n");
    }
}
