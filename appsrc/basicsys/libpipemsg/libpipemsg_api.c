/*
 * libpipemsg_api.c
 *
 *  Created on: 2017骞�6鏈�23鏃�
 *      Author: eric
 */

#ifdef WIN32
#else

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/inotify.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libcommon_api.h>
#include "libpipemsg_api.h"

/*
 * pipe msg interface
 */
#define FIFO_MSG_FILE_NAME_MAX  128
#define FIFO_MSG_FILE_PATH             "/tmp/run"
#define FIFO_MSG_NAME_MAX           64
#define FIFO_MSG_READ_TIMEOUT_DEFAULT 100 // 100ms

#define    FIFO_MSG_MALLOC(x)              Common_Malloc(x,sizeof(int),__func__,__LINE__)
#define    FIFO_MSG_FREE(x)                Common_Free(x,__func__,__LINE__)
#define    FIFO_MSG_STRDUP(x)              Common_StrDup(x,__func__,__LINE__)

typedef void * FIFO_MSG_H;

static int FifoMsg_Open(FIFO_MSG_H *handle, char *fifoName,int oflag); //oflag, please reference O_EXCL , O_CREATE, O_RDWR, O_RDONLY ...
static int FifoMsg_Close(FIFO_MSG_H *handle);
static int FifoMsg_Read(FIFO_MSG_H handle, void *buf,int size,int timeout);//timeout N ms
static int FifoMsg_Write(FIFO_MSG_H handle, void *buf,int size);
//static int FifoMsg_GetFd(FIFO_MSG_H handle);
//static char * FifoMsg_GetErrMsg(int err);

typedef struct
{
    int fifoFd;
    int oflag;
    char fifoFullName[FIFO_MSG_FILE_NAME_MAX];
} FIFO_MSG_CONTEXT_T;

static int FifoMsg_Open(FIFO_MSG_H *handle, char *fifoName, int oflag)
{
    int ret = 0;
    FIFO_MSG_CONTEXT_T *ct = NULL;

    if (fifoName == NULL || oflag <= 0 || strlen(fifoName) > FIFO_MSG_NAME_MAX)
        return -EINVAL;

    *handle = FIFO_MSG_MALLOC(sizeof(FIFO_MSG_CONTEXT_T));
    if (*handle == NULL)
        return -ENOMEM;

    ct = (FIFO_MSG_CONTEXT_T *) (*handle);

    mkdir(FIFO_MSG_FILE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    snprintf(ct->fifoFullName, sizeof(ct->fifoFullName) - 1, "%s/%s", FIFO_MSG_FILE_PATH, fifoName);
    mkfifo(ct->fifoFullName, S_IRWXU | S_IRWXG | S_IRWXO);
    LOGD("ful name %s\n",ct->fifoFullName);
    ct->oflag = oflag;
    ct->fifoFd = open(ct->fifoFullName, ct->oflag, 0666);
    if (ct->fifoFd < 0)
    {
        LOGE("%s\n",strerror(errno));
        ret = -errno;
        FIFO_MSG_FREE(*handle);
        *handle = NULL;
        return ret;
    }

    return ct->fifoFd;
}

static int FifoMsg_Close(FIFO_MSG_H *handle)
{
    FIFO_MSG_CONTEXT_T * ct = NULL;
    if (handle == NULL || *handle == NULL)
        return -EINVAL;

    ct = (FIFO_MSG_CONTEXT_T *) (*handle);

    if (ct->fifoFd > 2)
        close(ct->fifoFd);

    FIFO_MSG_FREE(*handle);
    *handle = NULL;
    return 0;
}

static int FifoMsg_Read(FIFO_MSG_H handle, void *buf, int size, int timeOut)
{
    FIFO_MSG_CONTEXT_T * ct = (FIFO_MSG_CONTEXT_T *) handle;
    int ret = 0, readSize = 0;
    struct timeval timeout;
    fd_set rd_set;

    if (ct == NULL || ct->fifoFd < 3 || buf == NULL || size <= 0)
        return -EINVAL;

//    LOGD("read %s timeout %ld fd %d\n",ct->fifoFullName,timeout.tv_sec,ct->fifoFd);

    if (access(ct->fifoFullName, F_OK) != 0)
    {
        close(ct->fifoFd);
        ct->fifoFd = 0;
        return -ENOENT;
    }

    do
    {
        FD_ZERO(&rd_set);
        FD_SET(ct->fifoFd, &rd_set);

        if (timeOut > 0)
        {
            timeout.tv_sec = timeOut / 1000;
            timeout.tv_usec = (timeOut % 1000) * 1000;
        }
        else
        {
            timeout.tv_sec = FIFO_MSG_READ_TIMEOUT_DEFAULT / 1000;
            timeout.tv_usec = (FIFO_MSG_READ_TIMEOUT_DEFAULT % 1000) * 1000;
        }

        ret = select(ct->fifoFd + 1, &rd_set, NULL, NULL, &timeout);
        if (ret <= 0)
        {
            if (ret == 0)
                ret = -ETIMEDOUT;
            else
                ret = -errno;
            break;
        }

        ret = read(ct->fifoFd, buf, size);
        if (ret < 0 && (errno == EAGAIN || errno == EINTR))
        {
            LOGW("try again\n");
            continue;
        }
        else if (ret < 0)
        {
            LOGE("errno %d\n",errno);
            ret =  -errno;
            break;
        }
        else if (ret > 0)
        {
            readSize += ret;
            if (ret != size)
            {
                buf = (void *)((char *)buf + ret);
                size -= ret;
                if (size <= 0)
                {
                    ret = -1;
                    break;
                }
                else
                    continue;
            }
            else
                break;
        }
    } while (1);

    return readSize>0?readSize:ret;
}

static int FifoMsg_Write(FIFO_MSG_H handle, void *buf, int size)
{
    FIFO_MSG_CONTEXT_T * ct = (FIFO_MSG_CONTEXT_T *) handle;
    int ret = 0, sndSize = 0, tryCnt = 0;

    if (ct == NULL || ct->fifoFd < 3 || buf == NULL || size <= 0)
        return -EINVAL;

//    LOGD("write %s fd %d size %d\n",ct->fifoFullName,ct->fifoFd,size);

    if (access(ct->fifoFullName, F_OK) != 0)
    {
        close(ct->fifoFd);
        ct->fifoFd = 0;
        return -ENOENT;
    }

    do
    {
        ret = write(ct->fifoFd, buf, size);
        if (ret <= 0 && (errno == EAGAIN || errno == EINTR) && tryCnt < 10)
        {
            tryCnt++;
            usleep(1000);
            continue;
        }
        else if (ret < 0)
        {
            break;
        }
        else if (ret > 0)
        {
            tryCnt = 0;
            sndSize += ret;
            if (ret != size)
            {
                buf = (void *)((char *)buf + ret);
                size -= ret;
                if (size <= 0)
                {
                    ret = -1;
                    break;
                }
            }
            else
                break;
        }

    } while(1);

    return ret > 0 ? sndSize:ret;
}

static void FifoMsg_Sync(FIFO_MSG_H handle)
{
    FIFO_MSG_CONTEXT_T * ct = (FIFO_MSG_CONTEXT_T *) handle;
    if (ct == NULL || ct->fifoFd < 3)
        return ;
    fsync(ct->fifoFd);
}

static void FifoMsg_Clear(FIFO_MSG_H handle)
{
    FIFO_MSG_CONTEXT_T * ct = (FIFO_MSG_CONTEXT_T *) handle;
    int ret = 0;
    char *buf = NULL;

    if (ct == NULL || ct->fifoFd < 3)
        return ;

    buf = (char *)FIFO_MSG_MALLOC(1024);
    do
    {
        ret = read(ct->fifoFd, buf, 1024);
    } while(ret > 0 || (ret < 0 && (errno == EINTR)));
    FIFO_MSG_FREE(buf);

}
//static char * FifoMsg_GetErrMsg(int err)
//{
//    return strerror(-err);
//}
//
//static int FifoMsg_GetFd(FIFO_MSG_H handle)
//{
//    FIFO_MSG_CONTEXT_T * ct = (FIFO_MSG_CONTEXT_T *) handle;
//    if (ct == NULL || ct->fifoFd < 3)
//        return -EINVAL;
//
//    return ct->fifoFd;
//}

/*
 * pipe message interface
 */

/*
 * message format
 * START_CODE MSG_TYPE SUB_TYPE MSG_TOKEN  MSG_LEN MSG_DATA
 *  4B          4B        4B         4B     4B        NB
 */


//#define PIPE_MSG_FIFO_NAME_A   "pipe_msg_a"
//#define PIPE_MSG_FIFO_NAME_B   "pipe_msg_b"
#define PIPE_MSG_START_CODE    0xbeca1c1b

typedef struct
{
    unsigned int startCode;
    unsigned int msgType;
    unsigned int subType;
    unsigned int msgToken;
    unsigned int msgLen;
} PIPE_MSG_HEAD_T;

enum
{
    PIPE_MSG_STATE_UNINIT = 0,
    PIPE_MSG_STATE_INIT,
};

typedef struct
{
    int state;
    int mode;
    char sendFifoName[FIFO_MSG_FILE_NAME_MAX];
    char recvFifoName[FIFO_MSG_FILE_NAME_MAX];
    unsigned int msgSeq;
    FIFO_MSG_H msgSendHandle;
    FIFO_MSG_H msgRecvHandle;
    pthread_t msgPth;
    pthread_t monPth;
    pthread_mutex_t readLock;
    pthread_mutex_t writeLock;
    void *msgData;
    int msgSize;
    PipeRecvHandleF RecvHandle;

} PIPE_MSG_CONTEXT_T;

//PIPE_MSG_CONTEXT_T s_pipe_msg_ct;

static int PipeMsg_Recv(PIPE_MSG_CONTEXT_T * ct, int *msgType, int *subType, void **data, int *size)
{
    if (ct->state
                    == PIPE_MSG_STATE_UNINIT|| ct->msgSendHandle == NULL || ct->msgRecvHandle == NULL)
    {
        LOGE("state error \n");
        return -1;
    }

    PIPE_MSG_HEAD_T head = { 0 };
    int ret = 0;

    pthread_mutex_lock(&ct->readLock);
    ret = FifoMsg_Read(ct->msgRecvHandle, &head, sizeof(PIPE_MSG_HEAD_T), 3000);
    if (ret < 0)
    {
//        LOGE("fifo read failed %d %s\n", -ret, strerror(-ret));
        pthread_mutex_unlock(&ct->readLock);
        return ret;
    }

//    LOGD("read ret %d\n", ret);
    if (head.startCode != PIPE_MSG_START_CODE)
    {
        LOGE("start code error\n");
        FifoMsg_Clear(ct->msgRecvHandle);
        pthread_mutex_unlock(&ct->readLock);
        return -2;
    }

    if (head.msgLen > 0)
    {
        *data = FIFO_MSG_MALLOC(head.msgLen);

        ret = FifoMsg_Read(ct->msgRecvHandle, (*data), head.msgLen, 1000);
        if (ret != head.msgLen)
        {
            LOGE("fifo read failed %d head msgLen %u\n", ret,head.msgLen);
            FIFO_MSG_FREE(*data);
            *data = NULL;
            *size = 0;
            pthread_mutex_unlock(&ct->readLock);
            return -2;
        }
    }
    *msgType = (int) head.msgType;
    *subType = (int) head.subType;
    *size = head.msgLen;

    pthread_mutex_unlock(&ct->readLock);
    return ret;
}

static void PipeMsg_SendState(PIPE_MSG_CONTEXT_T *ct, int fd)
{
    int len = 0;
    char buf[64] = { 0 };
    struct inotify_event *event = NULL;

    while ((len = read(fd, buf, sizeof(buf))) > 0)
    {
        event = (struct inotify_event *) buf;
        if (event->mask & IN_CLOSE)
        {
            ct->RecvHandle(PIPE_MSG_TYPE_STATE,PIPE_MSG_SUB_TYPE_DISCONNECT, NULL, 0);
        }
        else if (event->mask & IN_OPEN)
        {
            ct->RecvHandle(PIPE_MSG_TYPE_STATE, PIPE_MSG_SUB_TYPE_CONNECT, NULL, 0);
        }
    }
}

static void * PipeMsg_MonThread(void *data)
{
    PIPE_MSG_CONTEXT_T *ct = (PIPE_MSG_CONTEXT_T *) data;
    int fd = 0, wd = 0;
    char watchPath[FIFO_MSG_FILE_NAME_MAX] = { 0 };

    int return_value = 0;
    fd_set descriptors;
    struct timeval time_to_wait;

    fd = inotify_init();
    if (fd < 0)
    {
        LOGE("inotify_init failed\n");
        return NULL ;
    }
    snprintf(watchPath,sizeof(watchPath),"%s/%s",FIFO_MSG_FILE_PATH,ct->sendFifoName);
    wd = inotify_add_watch(fd, watchPath, IN_CLOSE | IN_OPEN);
    if (wd < 0)
    {
        LOGE("inotify_add_watch %s failed\n", watchPath);
        close(fd);
        return NULL ;
    }

    prctl(PR_SET_NAME, __func__);

    while (return_value >= 0 && ct->state == PIPE_MSG_STATE_INIT)
    {
        FD_ZERO(&descriptors);
        FD_SET(fd, &descriptors);
        time_to_wait.tv_sec = 1;
        time_to_wait.tv_usec = 0;
        return_value = select(fd + 1, &descriptors, NULL, NULL, &time_to_wait);
        if (return_value < 0)
        {
            LOGW("select failed %d %s\n", errno, strerror(errno));
            continue;
        }
        else if (!return_value)
        {
            /* Timeout */
            continue;
        }
        else if (FD_ISSET(fd, &descriptors))
        {
            PipeMsg_SendState(ct,fd);
        }
    }
    close(fd);

    return NULL ;
}

static void * PipeMsg_RecvThread(void *data)
{
    PIPE_MSG_CONTEXT_T *ct = (PIPE_MSG_CONTEXT_T *)data;
    void *msgData = NULL;
    int msgSize = 0, msgType = 0, subType = 0, ret = 0;
    LOGD("do state %d\n",ct->state);
    prctl(PR_SET_NAME,__FUNCTION__);
    while (ct->state == PIPE_MSG_STATE_INIT)
    {
        msgData = NULL;
        ret = PipeMsg_Recv(ct,&msgType, &subType, &msgData, &msgSize);
//        LOGW("recv cmd %d size %d\n",ret,msgSize);
        if (ret > 0 && ct->RecvHandle)
            ct->RecvHandle(msgType, subType, msgData, msgSize);
        else
        {
            usleep(10000);
//            if (ret == -2)
//                ct->RecvHandle(PIPE_MSG_TYPE_STATE, PIPE_MSG_SUB_TYPE_CONNECT, NULL, 0);
        }

        if (msgData)
            FIFO_MSG_FREE(msgData);
    }
    LOGD("done\n");
    return NULL;
}

int PipeMsg_Init(char *pipeName, int mode, PipeRecvHandleF RecvHandle, void **pipeHandle)
{
    int ret = 0;
    PIPE_MSG_CONTEXT_T *ct = NULL;

    if (pipeHandle == NULL || *pipeHandle != NULL)
        return -1;

    *pipeHandle = FIFO_MSG_MALLOC(sizeof(PIPE_MSG_CONTEXT_T));
    ct = (PIPE_MSG_CONTEXT_T *)*pipeHandle;
    if (ct == NULL)
        return -1;

    memset(ct,0,sizeof(PIPE_MSG_CONTEXT_T));
    pthread_mutex_init(&ct->readLock,NULL);
    pthread_mutex_init(&ct->writeLock,NULL);

    if (mode == 0)
    {
        snprintf(ct->sendFifoName,sizeof(ct->sendFifoName),"%s_a",pipeName);
        snprintf(ct->recvFifoName,sizeof(ct->recvFifoName),"%s_b",pipeName);
    }
    else
    {
        snprintf(ct->sendFifoName,sizeof(ct->sendFifoName),"%s_b",pipeName);
        snprintf(ct->recvFifoName,sizeof(ct->recvFifoName),"%s_a",pipeName);
    }

    ct->mode = mode;

    ret = FifoMsg_Open(&ct->msgSendHandle, ct->sendFifoName, O_CREAT | O_RDWR | O_NONBLOCK);
    if (ret < 0)
    {
        LOGE("fifo open failed %s %s\n",ct->sendFifoName,strerror(-ret));
        pthread_mutex_destroy(&ct->readLock);
        pthread_mutex_destroy(&ct->writeLock);
        FIFO_MSG_FREE(*pipeHandle);
        *pipeHandle = NULL;
        return -2;
    }

    ret = FifoMsg_Open(&ct->msgRecvHandle, ct->recvFifoName, O_CREAT | O_RDWR | O_NONBLOCK);
    if (ret < 0)
    {
        LOGE("fifo open failed %s %s\n",ct->recvFifoName,strerror(-ret));
        FifoMsg_Close(&ct->msgSendHandle);
        pthread_mutex_destroy(&ct->readLock);
        pthread_mutex_destroy(&ct->writeLock);
        FIFO_MSG_FREE(*pipeHandle);
        *pipeHandle = NULL;
        return -3;
    }

    ct->RecvHandle = RecvHandle;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 64);
    ct->state = PIPE_MSG_STATE_INIT;

    if (pthread_create(&ct->msgPth, &attr, PipeMsg_RecvThread, (void * ) ct) != 0)
    {
        LOGE("create msg thread failed %s\n",strerror(errno));
        pthread_mutex_destroy(&ct->readLock);
        pthread_mutex_destroy(&ct->writeLock);
        FIFO_MSG_FREE(*pipeHandle);
        *pipeHandle = NULL;
        return -errno;
    }

    if (pthread_create(&ct->monPth, &attr, PipeMsg_MonThread, (void * ) ct) != 0)
    {
        LOGE("create mon thread failed %s\n",strerror(errno));
        pthread_mutex_destroy(&ct->readLock);
        pthread_mutex_destroy(&ct->writeLock);
        FIFO_MSG_FREE(*pipeHandle);
        *pipeHandle = NULL;
        return -errno;
    }

    return 0;
}

int PipeMsg_Uninit(void *pipeHandle)
{
    PIPE_MSG_CONTEXT_T *ct = (PIPE_MSG_CONTEXT_T *)pipeHandle;
    if (pipeHandle == NULL || ct->state == PIPE_MSG_STATE_UNINIT)
    {
        return -1;
    }

    ct->state = PIPE_MSG_STATE_UNINIT;
    pthread_join(ct->msgPth,NULL);
    pthread_join(ct->monPth,NULL);

    FifoMsg_Close(&ct->msgSendHandle);
    FifoMsg_Close(&ct->msgRecvHandle);
    pthread_mutex_destroy(&ct->readLock);
    pthread_mutex_destroy(&ct->writeLock);

    FIFO_MSG_FREE(pipeHandle);
    return 0;
}

int PipeMsg_Send(void *pipeHandle, int type, int subType, void *data, int size)
{
    PIPE_MSG_CONTEXT_T * ct = (PIPE_MSG_CONTEXT_T *)pipeHandle;
    if (pipeHandle == NULL || ct->state == PIPE_MSG_STATE_UNINIT || ct->msgSendHandle == NULL
                    || ct->msgRecvHandle == NULL)
    {
        return -1;
    }
    int ret = 0;

    PIPE_MSG_HEAD_T head;
    head.startCode = PIPE_MSG_START_CODE;
    head.msgLen = size;
    head.msgToken = 0xbede0892;
    head.msgType = type;
    head.subType = subType;

    pthread_mutex_lock(&ct->writeLock);

    ret = FifoMsg_Write(ct->msgSendHandle,&head,sizeof(PIPE_MSG_HEAD_T));
    if (ret > 0)
    {
        FifoMsg_Sync(ct->msgSendHandle);
        if (size > 0 && data != NULL)
        {
            ret = FifoMsg_Write(ct->msgSendHandle, data, size);
            if (ret > 0)
                FifoMsg_Sync(ct->msgSendHandle);
            else
                FifoMsg_Clear(ct->msgSendHandle);
        }
    }
    else
        FifoMsg_Clear(ct->msgSendHandle);

    pthread_mutex_unlock(&ct->writeLock);
    return ret;
}

void PipeMsg_Clear(void *pipeHandle)
{
    PIPE_MSG_CONTEXT_T * ct = (PIPE_MSG_CONTEXT_T *)pipeHandle;
    if (ct->state
                    == PIPE_MSG_STATE_UNINIT|| ct->msgSendHandle == NULL || ct->msgRecvHandle == NULL)
    {
        LOGE("state error \n");
        return;
    }

    pthread_mutex_lock(&ct->readLock);
    FifoMsg_Clear(ct->msgRecvHandle);
    pthread_mutex_unlock(&ct->readLock);

    pthread_mutex_lock(&ct->writeLock);
    FifoMsg_Clear(ct->msgSendHandle);
    pthread_mutex_unlock(&ct->writeLock);
    return;
}
#endif

/*
 * for testing
 */
//
//static void *ReadThread(void *data)
//{
//    void *fileData = NULL;
//    int fileSize = 0, ret = 0, msgType = 0;
//    ret = SmartMsg_Recv(&msgType,&fileData,&fileSize);
//    if (ret < 0)
//    {
//        LOGE("SmartMsg_Recv failed \n");
//    }
//    else
//    {
//        LOGD("recv data size %d\n",fileSize);
//        FILE * fp = fopen((char *)data,"wb");
//        fwrite(fileData,1,fileSize,fp);
//        fclose(fp);
//    }
//    if (fileData)
//        free(fileData);
//    LOGD("recv done\n");
//    return NULL;
//}
//
//static void *WriteThread(void *data)
//{
//    char * buf = NULL;
//    int ret = 0;
//    long int fileSize = 0;
//    FILE * fp = fopen((char *)data,"rb");
//    if (fp == NULL)
//    {
//        LOGE("open file %s %s\n",(char *)data, strerror(errno));
//        return NULL;
//    }
//
//    fseek(fp,0,SEEK_END);
//    fileSize = ftell(fp);
//    fseek(fp,0,SEEK_SET);
//    buf = malloc(fileSize);
//    ret = fread(buf,1,fileSize,fp);
//    fclose(fp);
//
//    LOGD("try to send size %d file size %ld after 3 sec\n",ret,fileSize);
//    sleep(3);
//    LOGD("sending\n");
//    ret = SmartMsg_Send(SMART_MSG_TYPE_SMART,buf,ret);
//    free(buf);
//    LOGD("send done\n");
//    return NULL;
//}
//
//int main(int argc, char **argv)
//{
//    int mode = atoi(argv[1]);
//    int ret = 0;
//    char *rd = strdup(argv[2]);
//    char *wr = strdup(argv[3]);
//    pthread_t readPth, writePth;
//
//
//    if ((ret = SmartMsg_Init(mode)) < 0)
//    {
//        LOGE("msg init failed %s\n",strerror(errno));
//        return 0;
//    }
//    pthread_create(&readPth,NULL,ReadThread,wr);
//    pthread_create(&writePth,NULL,WriteThread,rd);
//
//    pthread_join(readPth,NULL);
//    pthread_join(writePth,NULL);
//    SmartMsg_Uninit();
//    free(rd);
//    free(wr);
//    LOGD("main exit\n");
//    return 0;
//}
