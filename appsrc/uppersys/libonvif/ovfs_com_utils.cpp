
/*
 * ovfs_com_utils.c
 *
 *  Created on: 2017
 *      Author: eric
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <signal.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>

#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/un.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include <ifaddrs.h>


#if (!defined LINUX_MSTAR316) && (!defined LINUX_INGENIC)
// #define CURL_DISABLE_TYPECHECK
// #include <curl.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libcommon_api.h>

#include "ovfs_com_utils.h"
#include "ovfs_onvif.h"
#include "ovfs_com_pthread_pool.h"

typedef struct
{
    int maxThreads;
    int timeOut;
    int stackSize;
    pthread_pool_t *tpoolHandle;
    UTILS_ASYNC_TASK_CALLBACK_F CallBack;
} UTILS_ASYNC_TASK_CONTEXT_T;

typedef struct
{
    int taskId;
    int delay;
    void *taskData;
    int taskSize;
    UTILS_ASYNC_TASK_CALLBACK_F CallBack;
} UTILS_ASYNC_TASK_T;

struct arpMsg
{
    struct ethhdr ethhdr; /* Ethernet header */
    u_short htype; /* hardware type (must be ARPHRD_ETHER) */
    u_short ptype; /* protocol type (must be ETH_P_IP) */
    u_char hlen; /* hardware address length (must be 6) */
    u_char plen; /* protocol address length (must be 4) */
    u_short operation; /* ARP opcode */
    u_char sHaddr[6]; /* sender's hardware address */
    u_char sInaddr[4]; /* sender's IP address */
    u_char tHaddr[6]; /* target's hardware address */
    u_char tInaddr[4]; /* target's IP address */
    u_char pad[18]; /* pad for min. Ethernet payload (60 bytes) */
};

void Utils_Sleep(unsigned long long msec)
{
    struct timespec ts;
    int err;

    if (msec == 0)
        return;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do
    {
        err = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts);
    }
    while (err < 0 && errno == EINTR);
}

long long int Utils_GetMs(void)
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (long long int) ((long long int) tp.tv_sec * 1000L +
                            (long long int) tp.tv_nsec / 1000000L);
}

int Utils_FileMonitor(char *path)
{
    int fd = 0, wd = 0, len = 0, nread = 0;
    char buf[64] = { 0 };
    struct inotify_event *event = NULL;

    int return_value = 0;
    fd_set descriptors;
    struct timeval time_to_wait;

    fd = inotify_init();
    if (fd < 0)
    {
        LOGE("inotify_init failed\n");
        return -1;
    }

    wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_MODIFY);
    if (wd < 0)
    {
        LOGE("inotify_add_watch %s failed\n", path);
        close(fd);
        return -1;
    }

    while (return_value >= 0)
    {
        FD_ZERO(&descriptors);
        FD_SET(fd, &descriptors);
        time_to_wait.tv_sec = 3;
        time_to_wait.tv_usec = 0;
        return_value = select(fd + 1, &descriptors, NULL, NULL, &time_to_wait);
        if (return_value < 0)
        {
            break;
        }
        else if (!return_value)
        {
            /* Timeout */
        }
        else if (FD_ISSET(fd, &descriptors))
        {
            while ((len = read(fd, buf, sizeof(buf) - 1)) > 0)
            {
                nread = 0;
                event = (struct inotify_event *) &buf[nread];
                if ((event->mask & IN_CLOSE_WRITE) || (event->mask & IN_MODIFY))
                {
                    close(fd);
                    return 0;
                }
                nread = nread + sizeof(struct inotify_event) + event->len;
                len = len - sizeof(struct inotify_event) - event->len;
            }
        }
    }
    close(fd);
    return -1;
}

int Utils_AsyncTaskInit(void **UtilsAsyncHandle,
                        UTILS_ASYNC_TASK_CALLBACK_F CallBack)
{
    if (UtilsAsyncHandle == NULL || *UtilsAsyncHandle != NULL || CallBack == NULL)
    {
        LOGE("param error\n");
        return -1;
    }

    UTILS_ASYNC_TASK_CONTEXT_T *ct = NULL;
    ct = (UTILS_ASYNC_TASK_CONTEXT_T *)ONVIF_MALLOC(sizeof(
                UTILS_ASYNC_TASK_CONTEXT_T));
    if (ct == NULL)
    {
        LOGE("malloc failed\n");
        return -1;
    }

    memset(ct, 0, sizeof(UTILS_ASYNC_TASK_CONTEXT_T));
    ct->maxThreads = 8;
    ct->timeOut = 0;
    ct->stackSize = PTHREAD_STACK_MIN * 8;
    ct->CallBack = CallBack;
    ct->tpoolHandle = thread_pool_create(ct->maxThreads, ct->timeOut,
                                         ct->stackSize);

    *UtilsAsyncHandle = ct;
    return 0;
}

int Utils_AsyncTaskUninit(void **UtilsAsyncHandle)
{
    if (UtilsAsyncHandle == NULL || *UtilsAsyncHandle == NULL)
    {
        LOGE("param error\n");
        return -1;
    }

    UTILS_ASYNC_TASK_CONTEXT_T *ct = (UTILS_ASYNC_TASK_CONTEXT_T *)
                                     *UtilsAsyncHandle;
    pthread_pool_destroy(ct->tpoolHandle);
    ONVIF_FREE(ct);

    *UtilsAsyncHandle = NULL;
    return 0;
}

static void Utils_AsyncTaskFreeCb(void *data)
{
    UTILS_ASYNC_TASK_T *task = (UTILS_ASYNC_TASK_T *)data;
    if (task != NULL)
    {
        if (task->taskData)
        {
            ONVIF_FREE(task->taskData);
            task->taskData = NULL;
        }

        ONVIF_FREE(task);
        task = NULL;
    }
}

int Utils_AsyncTaskClearWaitQueue(void *UtilsAsyncHandle)
{
    if (UtilsAsyncHandle == NULL)
    {
        LOGE("param error\n");
        return -1;
    }

    UTILS_ASYNC_TASK_CONTEXT_T *ct = (UTILS_ASYNC_TASK_CONTEXT_T *)
                                     UtilsAsyncHandle;
    if (pthread_pool_qlen(ct->tpoolHandle) > 0)
        pthread_pool_clear_wait_queue(ct->tpoolHandle, Utils_AsyncTaskFreeCb);

    return 0;
}

static void AsyncTaskDoThread(void *data)
{
    UTILS_ASYNC_TASK_T *task = (UTILS_ASYNC_TASK_T *) data;
    if (task == NULL)
        return;

    if (task->delay > 0)
        Common_Sleep(task->delay / 1000, task->delay % 1000);

    task->CallBack(task->taskId, task->taskData, task->taskSize);

    if (task->taskData)
        ONVIF_FREE(task->taskData);

    ONVIF_FREE(data);
}


int Utils_AsyncTaskAdd(void *UtilsAsyncHandle, int taskId, int delay,
                       void *data, int dataSize)
{
    if (UtilsAsyncHandle == NULL || (data != NULL && dataSize <= 0))
    {
        LOGE("param error\n");
        return -1;
    }

    // LOGD("%p add async task %d\n", UtilsAsyncHandle, taskId);
    UTILS_ASYNC_TASK_CONTEXT_T *ct = (UTILS_ASYNC_TASK_CONTEXT_T *)
                                     UtilsAsyncHandle;
    if (pthread_pool_qlen(ct->tpoolHandle) > 16 * 2 )
    {
        LOGW("task is full drop this task\n");
        return -1;
    }

    UTILS_ASYNC_TASK_T *task = (UTILS_ASYNC_TASK_T *)ONVIF_MALLOC(sizeof(
                                   UTILS_ASYNC_TASK_T));
    task->delay = delay;
    task->taskId = taskId;
    task->taskSize = dataSize;
    task->CallBack = ct->CallBack;

    if (data != NULL)
    {
        task->taskData = ONVIF_MALLOC(task->taskSize);
        memcpy(task->taskData, data, dataSize);
    }
    else
    {
        task->taskData = NULL;
        task->taskSize = 0;
    }

    pthread_pool_add(ct->tpoolHandle, AsyncTaskDoThread, (void *) task);
    return 0;
}

typedef struct
{
    int state;
    int recvMax;
    int timeOut;
    int maxBufLen;
    int currentClient;
    int sockListen;
    int epollfd;
    pthread_t serverPth;
    UTILS_UDS_CALLBACK_F cb;
    void *userData;
    pthread_pool_t *recvThreadPoolHandle;
} UTILS_UDS_SERVER_CONTEXT_T;


typedef struct
{
    UTILS_UDS_SERVER_CONTEXT_T *handle;
    int sockfd;
} UTILS_UDS_PARAM_T;

u_int32_t raddr;
u_int32_t laddr;
/*参数说明 目标IP地址，本机IP地址，本机mac地址，网卡类型*/
static int Arpping(char *destAddr, char *localAddr,/*u_int32_t destAddr, u_int32_t localAddr,*/ unsigned char *macStr,
                   char *interface, unsigned long long int checkTimeout)
{
    LOGW("-----1 raddr:[%s] laddr:[%s]]\n",destAddr,localAddr);

    raddr = inet_addr(destAddr);
    laddr = inet_addr(localAddr);

    unsigned long long int timeout = checkTimeout;
    unsigned long long int prevTimeA = 0LLU, lastTimeB = 0LLU;
    int s; /* socket */
    int rv = -1; /* return value */
    struct sockaddr addr; /* for interface name */
    struct arpMsg arp;
    fd_set fdset;
    struct timeval tm;

    unsigned char mac[6] = { 0 };
//    LOGE("mac str %s\n",macStr);
    sscanf((const char *) macStr, "%02x:%02x:%02x:%02x:%02x:%02x",
           (unsigned int *) &mac[0],
           (unsigned int *) &mac[1],
           (unsigned int *) &mac[2],
           (unsigned int *) &mac[3],
           (unsigned int *) &mac[4],
           (unsigned int *) &mac[5]);
//    LOGE("mac %02x %02x %02x %02x %02x %02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    /*socket发送一个arp包*/
    if ((s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1)
    {
        LOGE("Could not open raw socket/n");
        return -1;
    }

    /*设置套接口类型为广播，把这个arp包是广播到这个局域网*/
    // if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1)
    // {
    //     LOGE("Could not setsocketopt on raw socket/n");
    //     close(s);
    //     return -1;
    // }

    /* 对arp设置，这里按照arp包的封装格式赋值即可，详见http://blog.csdn.net/wanxiao009/archive/2010/05/21/5613581.aspx */
    memset(&arp, 0, sizeof(arp));
    memset(arp.ethhdr.h_dest, 0xff, 6); /* MAC DA */
    memcpy(arp.ethhdr.h_source, mac, 6); /* MAC SA */
    arp.ethhdr.h_proto = htons(ETH_P_ARP); /* protocol type (Ethernet) */
    arp.htype = htons(ARPHRD_ETHER); /* hardware type */
    arp.ptype = htons(ETH_P_IP); /* protocol type (ARP message) */
    arp.hlen = 6; /* hardware address length */
    arp.plen = 4; /* protocol address length */
    arp.operation = htons(ARPOP_REQUEST); /* ARP op code */
    memcpy(arp.sInaddr, &laddr, sizeof(u_int)); /* source IP address */

//    *((u_int *) arp.sInaddr) = localAddr;
    memcpy(arp.sHaddr, mac, 6); /* source hardware address */
    memset(arp.sInaddr, 0 , 4);
//    *((u_int *) arp.tInaddr) = destAddr;      /* target IP address */
    memcpy(arp.tInaddr, &raddr, sizeof(u_int)); /* target IP address */
    LOGW("-----2 raddr:[%x] laddr:[%x]]\n",raddr,laddr);


    memset(arp.tHaddr, 0xff, 6);

    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sa_data, interface);
    /*发送arp请求*/

    if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
        rv = 0;

    /* 利用select函数进行多路等待*/
    tm.tv_usec = 0;
    prevTimeA = Common_GetSystemCount64();

    while (timeout > 0LLU)
    {
        FD_ZERO(&fdset);
        FD_SET(s, &fdset);
        tm.tv_sec = (time_t)(timeout / 1000LLU);
        tm.tv_usec = timeout % 1000LLU * 1000LLU;
        // LOGE("timeout %llu \n",timeout);
        if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0)
        {
            LOGE("Error on ARPING request: %s/n", strerror(errno));
            if (errno != EINTR)
                rv = 0;
        }
        else if (FD_ISSET(s, &fdset))
        {
            if (recv(s, &arp, sizeof(arp), 0) < 0)
            {
                LOGE("recv no ok !\n");
                rv = 0;
                break;
            }
            // LOGE("arp.sInaddr %d.%d.%d.%d arp.sHaddr %02X.%02X.%02X.%02X.%02X.%02X\n",
            //      arp.sInaddr[0],
            //      arp.sInaddr[1],
            //      arp.sInaddr[2],
            //      arp.sInaddr[3],
            //      arp.sHaddr[0],
            //      arp.sHaddr[1],
            //      arp.sHaddr[2],
            //      arp.sHaddr[3],
            //      arp.sHaddr[4],
            //      arp.sHaddr[5]);
            /*
            int i_tmp = memcmp(arp.sInaddr, &raddr, sizeof(u_int));
            if(*((u_int *) arp.sInaddr) == raddr)
            LOGD("sInaddr == destAddr!\n");
            */
            //LOGW("-recv ok sInaddr:[%x][%x][%x][%x] destAddr:[%x]!\n",arp.sInaddr[0],arp.sInaddr[1],arp.sInaddr[2],arp.sInaddr[3], raddr);

            /*如果条件 htons(ARPOP_REPLY) bcmp(arp.tHaddr, mac, 6) == 0 *((u_int *) arp.sInaddr) == yiaddr 三者都为真，则ARP应答有效,说明这个地址是已近存在的*/
            if (arp.operation == htons(ARPOP_REPLY)/* &&
             bcmp(arp.tHaddr, mac, 6) == 0 */ && *((u_int *) arp.sInaddr) == raddr/*memcmp(arp.sInaddr, &destAddr, sizeof(u_int)) == 0*/)
            {
//                *((u_int *) arp.sInaddr) == destAddr

                rv = 1;
                break;
            }
        }
        lastTimeB = Common_GetSystemCount64();
        if (lastTimeB - prevTimeA > checkTimeout)
            break;
        timeout = checkTimeout - (lastTimeB - prevTimeA);

    }
    close(s);
    LOGW("rv:[%d]\n",rv);
    return rv;
}



static void UdsAcceptConn(void *handle, int srvfd)
{
    UTILS_UDS_SERVER_CONTEXT_T *ct = (UTILS_UDS_SERVER_CONTEXT_T *)handle;

    struct sockaddr_un sin;
    socklen_t len = sizeof(struct sockaddr_un);
    bzero(&sin, len);

    int confd = accept(srvfd, (struct sockaddr *)&sin, &len);

    if (confd < 0)
    {
        LOGE("bad accept %s\n", strerror(errno));
        return;
    }
    else
    {
        // LOGE("Accept Connection: %d\n", confd);
    }
    //将新建立的连接添加到EPOLL的监听中
    struct epoll_event event;
    event.data.fd = confd;
    event.events =  EPOLLIN | EPOLLET;
    epoll_ctl(ct->epollfd, EPOLL_CTL_ADD, confd, &event);
}

static void UdsRecvData(void *data)
{
    UTILS_UDS_PARAM_T *param = (UTILS_UDS_PARAM_T *)data;
    prctl(PR_SET_NAME, __func__);
    int ret = 0;
    char *buf = (char *)ONVIF_MALLOC(param->handle->maxBufLen);
    memset(buf, 0, param->handle->maxBufLen);
    while(1)
    {
        ret = recv(param->sockfd, buf, param->handle->maxBufLen, 0);
        if (ret == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            continue;
        }
        else if (ret < 0)
        {
            LOGE("recv faled %s\n", strerror(errno));
        }

        break;
    }

    if (ret > 0)
    {
        if (param->handle->cb != NULL)
        {
            param->handle->cb(param->handle, param->sockfd, buf, ret, param->handle->userData);
        }
    }
    close(param->sockfd);
    ONVIF_FREE(buf);
    ONVIF_FREE(data);
}

static void *UdsServerThread(void *data)
{
    UTILS_UDS_SERVER_CONTEXT_T *ct = (UTILS_UDS_SERVER_CONTEXT_T *)data;
    struct epoll_event eventList[10];
    struct epoll_event event;
    prctl(PR_SET_NAME, __func__);

    ct->epollfd = epoll_create(10);
    event.events = EPOLLIN;
    event.data.fd = ct->sockListen;

    //add Event
    if(epoll_ctl(ct->epollfd, EPOLL_CTL_ADD, ct->sockListen, &event) < 0)
    {
        LOGE("epoll add fail : fd = %d\n", ct->sockListen);
        return NULL;
    }

    //epoll
    while(1)
    {
        if (ct->state != 1)
        {
            LOGD("quit thread state %d\n", ct->state);
            break;
        }

        int ret = epoll_wait(ct->epollfd, eventList, 10, ct->timeOut);
        if(ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;

            LOGE("epoll error %s\n", strerror(errno));
            break;
        }
        else if(ret == 0)
        {
            // LOGD("timeOut\n");
            continue;
        }

        int i = 0;
        for(i = 0; i < ret; i++)
        {
            if ((eventList[i].events & EPOLLERR) ||
                    (eventList[i].events & EPOLLHUP) ||
                    !(eventList[i].events & EPOLLIN))
            {
                // LOGE("close socket %d %x\n", eventList[i].data.fd, eventList[i].events);
                close(eventList[i].data.fd);
                continue;
            }

            if (eventList[i].data.fd == ct->sockListen)
            {
                UdsAcceptConn(ct, ct->sockListen);
            }
            else if (eventList[i].data.fd > 0)
            {
                // LOGE("recv %d\n", eventList[i].data.fd);
                epoll_ctl(ct->epollfd, EPOLL_CTL_DEL, eventList[i].data.fd, eventList);
                UTILS_UDS_PARAM_T *param = (UTILS_UDS_PARAM_T *)ONVIF_MALLOC(sizeof(UTILS_UDS_PARAM_T));
                param->handle = ct;
                param->sockfd = eventList[i].data.fd;
                // UdsRecvData(param);
                pthread_pool_add(ct->recvThreadPoolHandle, UdsRecvData, param);
                // UdsRecvData(ct, eventList[i].data.fd);
                // close(eventList[i].data.fd);
            }
        }
    }

    close(ct->epollfd);
    LOGW("Uds server thread exit\n");
    return NULL;
}

int Utils_UdsServerStart(char *addr, int timeOut, int maxBufLen,
                         UTILS_UDS_CALLBACK_F cb, void *userData, void **handle)
{
    struct sockaddr_un server_addr;

    if (addr == NULL || timeOut < 1 || cb == NULL || handle == NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }

    *handle = ONVIF_MALLOC(sizeof(UTILS_UDS_SERVER_CONTEXT_T));
    UTILS_UDS_SERVER_CONTEXT_T *ct = (UTILS_UDS_SERVER_CONTEXT_T *)(*handle);

    if (ct->state == 1)
        return -1;

    if((ct->sockListen = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        LOGE("socket error %s\n", strerror(errno));
        ONVIF_FREE(*handle);
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    snprintf(server_addr.sun_path, sizeof(server_addr.sun_path) - 1, "%s", addr);

    //bind
    if(bind(ct->sockListen, (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0)
    {
        LOGE("bind error %s\n", strerror(errno));
        ONVIF_FREE(*handle);
        return -1;
    }

    //listen
    if(listen(ct->sockListen, 8) < 0)
    {
        LOGE("listen error %s\n", strerror(errno));
        ONVIF_FREE(*handle);
        return -1;
    }

    ct->timeOut = timeOut;
    ct->cb = cb;
    ct->state = 1;
    ct->userData = userData;
    ct->maxBufLen = maxBufLen;
    ct->recvThreadPoolHandle = thread_pool_create(4, 0, PTHREAD_STACK_MIN * 4);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 32);
    /* pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); */
    pthread_create(&ct->serverPth, &attr, UdsServerThread, ct);
    return 0;
}

int Utils_UdsServerStop(void **handle)
{
    UTILS_UDS_SERVER_CONTEXT_T *ct = *(UTILS_UDS_SERVER_CONTEXT_T **)handle;
    if (ct->state == 0)
        return -1;

    ct->state = 0;
    pthread_pool_destroy(ct->recvThreadPoolHandle);
    pthread_join(ct->serverPth, NULL);
    close(ct->sockListen);
    memset(ct, 0, sizeof(UTILS_UDS_SERVER_CONTEXT_T));
    return 0;
}

int Utils_UdsServerSend(void *handle, int conHandle, char *buf, int len)
{
    int remain = len, sent = 0, offset = 0;
    UTILS_UDS_SERVER_CONTEXT_T *ct = (UTILS_UDS_SERVER_CONTEXT_T *)handle;
    if (ct->state == 0)
        return -1;

    while(remain)
    {
        sent = write(conHandle, buf + offset, remain);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            return -1;
        }
        offset = offset + sent;
        remain = remain - sent;
    }
    return 0;
}

typedef struct
{
    int sockfd;
    char ipAddr[32];
    char devName[32];
} UTILS_UDP_SOCK_NODE_T;

typedef struct
{
    char addr[32];
    void *msgHandle;
    int port;
    int timeOut;
    pthread_t serverPth;
    int state;
    char *buf;
    int bufLen;
    UTILS_UDP_MULTI_CALLBACK_F cb;
    void *userData;
    COMMON_DLIST_T sockDevList;
} UTILS_UDP_MULTI_SERVER_CONTEXT_T;

static int UdpCreateSocket(UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct, char *ip, char *dev)
{
    struct sockaddr_in peeraddr;
    int sockfd = 0;
    unsigned int socklen = 0;
    struct ip_mreq mreq;

    /* 创建 socket 用于UDP通讯 */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        LOGE("socket creating err in udptalk\n");
        return -1;
    }

    /* 设置要加入组播的地址 */
    bzero(&mreq, sizeof(struct ip_mreq));
    // group = gethostbyname(ct->addr);
    // bcopy((void *) group->h_addr, (void *) &ia, group->h_length);

    /* 设置组地址 */
    // bcopy(&ia, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));
    mreq.imr_multiaddr.s_addr = inet_addr(ct->addr);
    /* 设置发送组播消息的源主机的地址信息 */
    // mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_interface.s_addr = inet_addr(ip);

    /* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                   sizeof(struct ip_mreq)) == -1)
    {
        LOGE("setsockopt failed %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    unsigned char loop = 0;
    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    int reuseaddr = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    struct ifreq ifr = {};
    strcpy(ifr.ifr_name, dev);
    setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

    int ttl = 255;
    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl));

    socklen = sizeof(struct sockaddr_in);
    memset(&peeraddr, 0, socklen);
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(ct->port);

    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(sockfd, (struct sockaddr *) &peeraddr,
             sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("Bind error %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    return sockfd;
}

int Utils_SendtoMulticast(char *s, int len, char *devName, char *devIp)
{
    int ret = 0;
    int sending, sendpos = 0;
    int sockfd;
    struct sockaddr_in server_addr = {};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        ret = -1;
    }

    if (0 == ret)
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(ONVIF_MULTIADDR);
        server_addr.sin_port = htons(ONVIF_MULTIPORT);

        unsigned char loop = 0;
        setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

        int reuseaddr = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

        struct ifreq ifr = {};
        strcpy(ifr.ifr_name, devName);
        setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

        int ttl = 255;
        setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl));

        do
        {
            sending = sendto(sockfd, s + sendpos, len - sendpos, 0,
                             (struct sockaddr *)&server_addr, sizeof(server_addr));
            if(sending < 0)
            {
                LOGE("%s\n", strerror(errno));
                ret = -1;
                break;
            }
            else
            {
                sendpos += sending;
            }
        }
        while(sendpos < len);

    }

    if (sockfd >= 0)
    {
        close(sockfd);
    }

    return ret;
}

static void UdpSockFree(void *data)
{
    if (data != NULL)
    {
        UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)data;
        if (node->sockfd != 0)
        {
            close(node->sockfd);
            node->sockfd = 0;
        }
    }

    if (data)
        ONVIF_FREE(data);
}

static int UdpSockSearchByName(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;
    UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)a;
    if (strcmp(node->devName, (char *)b) == 0)
        return 0;

    return -1;
}

static int MultiAddrCreateAndListen(UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct)
{

    struct ifaddrs *ifAddrStruct = NULL, *ifAddrStructSrc = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStructSrc);
    ifAddrStruct = ifAddrStructSrc;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            LOGD("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            if (strcmp(addressBuffer, "127.0.0.1") == 0)
            {
                ifAddrStruct = ifAddrStruct->ifa_next;
                continue;
            }
            UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)ONVIF_MALLOC(
                                              sizeof(UTILS_UDP_SOCK_NODE_T));
            node->sockfd = 0;
            snprintf(node->ipAddr, sizeof(node->ipAddr), "%s", addressBuffer);
            snprintf(node->devName, sizeof(node->devName), "%s", ifAddrStruct->ifa_name);
            node->sockfd = UdpCreateSocket(ct, addressBuffer, ifAddrStruct->ifa_name);
            if (node->sockfd < 0)
                ONVIF_FREE(node);
            else
                Common_DList_InsertTail(ct->sockDevList, node, sizeof(UTILS_UDP_SOCK_NODE_T));
        }
        // else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
        // {   // check it is IP6
        //     // is a valid IP6 Address
        //     tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        //     char addressBuffer[INET6_ADDRSTRLEN];
        //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        //     printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        // }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    freeifaddrs(ifAddrStructSrc);
    return 0;
}

static int MultiAddrCheck(UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct)
{

    struct ifaddrs *ifAddrStruct = NULL, *ifAddrStructSrc = NULL;
    void *tmpAddrPtr = NULL;
    int cnt = 0;
    getifaddrs(&ifAddrStructSrc);
    ifAddrStruct = ifAddrStructSrc;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            // LOGD("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            if (strcmp(addressBuffer, "127.0.0.1") == 0)
            {
                ifAddrStruct = ifAddrStruct->ifa_next;
                continue;
            }

            if (Common_DList_Search(ct->sockDevList, ifAddrStruct->ifa_name,
                                    UdpSockSearchByName) == NULL)
            {
                freeifaddrs(ifAddrStructSrc);
                return 1;
            }
            cnt++;
        }
        // else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
        // {   // check it is IP6
        //     // is a valid IP6 Address
        //     tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        //     char addressBuffer[INET6_ADDRSTRLEN];
        //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        //     printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        // }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    freeifaddrs(ifAddrStructSrc);

    if (cnt != Common_DList_GetCount(ct->sockDevList))
        return 1;

    return 0;
}

static int MultiAddrSelect(UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct, int *sockfd, char *devName)
{
    fd_set rfds;
    int retSelect = 0;
    struct timeval timeout = { 0, 0 };
    bzero(ct->buf, ct->bufLen);
    FD_ZERO(&rfds);
    int i = 0, cnt = 0, lsock = 0;
    cnt = Common_DList_GetCount(ct->sockDevList);
    for (i = 0; i < cnt; i++)
    {
        UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *) Common_DList_GetNode(
                                          ct->sockDevList, i);

        if (node == NULL)
            continue;
        FD_SET(node->sockfd, &rfds);
        lsock = node->sockfd;
    }
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    retSelect = select(lsock + 1, &rfds, NULL, NULL, &timeout);
    if (retSelect > 0)
    {
        for (i = 0; i < cnt; i++)
        {
            UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *) Common_DList_GetNode(
                                              ct->sockDevList, i);

            if (node == NULL)
                continue;
            if (FD_ISSET(node->sockfd, &rfds))
            {
                *sockfd = node->sockfd;
                strcpy(devName, node->devName);
                break;
            }
        }
    }
    return retSelect;
}

static void *UdpMulticastServerThread(void *data)
{
    UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct = (UTILS_UDP_MULTI_SERVER_CONTEXT_T *)
                                           data;

    struct sockaddr_in peeraddr = {};
    int n = 0, ret = 0;
    unsigned int socklen = sizeof(peeraddr);

    prctl(PR_SET_NAME, __func__);

udp_multi_server:
    if (ct == NULL || ct->state == 0)
    {
        LOGE("mgr state error \n");
        return NULL;
    }

    ret = MultiAddrCreateAndListen(ct);
    if (ret < 0)
    {
        Common_Sleep(1, 0);
        goto udp_multi_server;
    }

    unsigned long long int timeA = 0, timeB = 0;

    timeA = timeB = Common_GetSystemCount64();
    /* 循环接收网络上来的组播消息 */
    while (ct != NULL && ct->state == 1)
    {
        int retSelect = 0, sockfd = 0;
        char devName[32] = {};
        timeB = Common_GetSystemCount64();

        if (timeB - timeA > 1000LLU)
        {
            timeA = timeB;
            if (MultiAddrCheck(ct) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_DeleteAll(ct->sockDevList);
                goto udp_multi_server;
            }
        }

        retSelect = MultiAddrSelect(ct, &sockfd, devName);

        if (ct->state != 1)
        {
            break;
        }

        if (retSelect == 0)
        {
            continue;
        }
        else if (retSelect < 0)
        {
            LOGE("select error %s\n", strerror(errno));
            Common_DList_DeleteAll(ct->sockDevList);
            goto udp_multi_server;
        }
        else if (retSelect > 0)
        {
            n = recvfrom(sockfd, ct->buf, ct->bufLen, 0,
                         (struct sockaddr *) &peeraddr, &socklen);

            if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                LOGE("recvfrom err %s\n", strerror(errno));
                Common_DList_DeleteAll(ct->sockDevList);
                Common_Sleep(1, 0);
                goto udp_multi_server;
            }

            /* 成功接收到数据报 */
            if (n > ct->bufLen)
            {
                LOGE("recv buffer len was too big %d , buffer len is %d\n",
                     n, ct->bufLen);
                continue;
            }

            if (ct->cb == NULL)
            {
                LOGE("callback function is null\n");
                Common_DList_DeleteAll(ct->sockDevList);
                Common_Sleep(1, 0);
                goto udp_multi_server;
            }

            ct->buf[n] = 0;

            char ipAddr[32] = { 0 };
            snprintf(ipAddr, sizeof(ipAddr), "%s", inet_ntoa(peeraddr.sin_addr));

            ret = ct->cb((void *)ct, sockfd, devName, ct->buf, ct->bufLen,
                         ipAddr, ct->userData);
            if (ret == 0)
            {
                // LOGD("recv from %s port %d dev name %s\n", inet_ntoa(peeraddr.sin_addr),
                // peeraddr.sin_port, devName);
                //Utils_SendtoMulticast(ct, ct->buf, strlen(ct->buf), devName, ipAddr);
                if (peeraddr.sin_port != 0)
                {
                    n = sendto(sockfd, ct->buf, strlen(ct->buf), 0, (struct sockaddr *) &peeraddr,
                               socklen);
                }
            }
            /* LOGD("send rsp len %d to %x port %d\n",n,peeraddr.sin_addr.s_addr,peeraddr.sin_port); */
        }
    }

    if (ct != NULL)
        Common_DList_DeleteAll(ct->sockDevList);
    LOGW("discovery thread exit \n");
    return NULL;
}

int Utils_UdpMultiServerStart(char *addr, int port, int timeOut, int maxBufLen,
                              UTILS_UDP_MULTI_CALLBACK_F cb, void *userData, void **handle)
{
    if (port <= 10 || timeOut < 10 || maxBufLen < 1 || cb == NULL || handle == NULL)
    {
        LOGE("parameters error\n");
    }

    *handle = ONVIF_MALLOC(sizeof(UTILS_UDP_MULTI_SERVER_CONTEXT_T));
    UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct = *(UTILS_UDP_MULTI_SERVER_CONTEXT_T **)
                                           handle;
    ct->port = port;
    snprintf(ct->addr, sizeof(ct->addr), "%s", addr);
    ct->timeOut = timeOut;
    ct->bufLen = maxBufLen;
    ct->buf = (char *)ONVIF_MALLOC(maxBufLen);
    ct->state = 1;
    ct->userData = userData;
    ct->cb = cb;
    ct->sockDevList = NULL;

    Common_DList_Init(&ct->sockDevList, UdpSockFree);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 32);
    /* pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); */
    pthread_create(&ct->serverPth, &attr, UdpMulticastServerThread, ct);

    return 0;
}

int Utils_UdpMultiServerStop(void **handle)
{
    UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct = *(UTILS_UDP_MULTI_SERVER_CONTEXT_T **)
                                           handle;
    if (ct->state == 0)
        return -1;

    ct->state = 0;
    pthread_join(ct->serverPth, NULL);
    Common_DList_Uninit(&ct->sockDevList);

    if (ct->buf)
    {
        ONVIF_FREE(ct->buf);
        ct->buf = NULL;
    }
    ONVIF_FREE(*handle);
    *handle = NULL;
    return 0;
}

int Utils_UdpMultiServerSend(void *handle, int conHandle, char *buf, int len)
{
    int remain = len, sent = 0, offset = 0;
    UTILS_UDP_MULTI_SERVER_CONTEXT_T *ct = (UTILS_UDP_MULTI_SERVER_CONTEXT_T *)
                                           handle;
    if (ct == NULL)
        return -1;

    if (ct->state == 0)
        return -1;

    while(remain && ct->state == 1)
    {
        sent = write(conHandle, buf + offset, remain);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            return -1;
        }
        offset = offset + sent;
        remain = remain - sent;
    }
    return 0;
}

static char gfxBASE64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

char *ovfs_onvif_BASE64Encode(const char *data, int data_len)
{
    //int data_len = strlen(data);
    int prepare = 0;
    int ret_len;
    int temp = 0;
    char *ret = NULL;
    char *f = NULL;
    int tmp = 0;
    char changed[4];
    int i = 0;

    if(data == NULL || data_len <= 0)
        return NULL;

    ret_len = data_len / 3;
    temp = data_len % 3;

    if (temp > 0)
        ret_len += 1;

    ret_len = ret_len * 4 + 1;

    ret = (char *)ONVIF_MALLOC(ret_len);
    if ( ret == NULL)
        return NULL;

    memset(ret, 0, ret_len);
    f = ret;

    while (tmp < data_len)
    {
        temp = 0;
        prepare = 0;
        memset(changed, '\0', 4);

        while (temp < 3)
        {
            if (tmp >= data_len)
                break;

            prepare = ((prepare << 8) | (data[tmp] & 0xFF));
            tmp++;
            temp++;
        }

        prepare = (prepare << ((3 - temp) * 8));

        for (i = 0; i < 4 ; i++ )
        {
            if (temp < i)
                changed[i] = 0x40;
            else
                changed[i] = (prepare >> ((3 - i) * 6)) & 0x3F;

            *f = gfxBASE64[(int)changed[i]];
            f++;
        }
    }

    *f = '\0';
    return ret;
}

static char FindBASE64(char ch)
{
    char *ptr = (char *)strrchr(gfxBASE64,
                                ch); //the last position (the only) in base[]

    return (ptr - gfxBASE64);
}

char *ovfs_onvif_BASE64Decode(const char *data, int data_len, int *dec_len)
{
    int ret_len = (data_len / 4) * 3;
    int equal_count = 0;
    char *ret = NULL;
    char *f = NULL;
    int tmp = 0;
    int temp = 0;
    int prepare = 0;
    int i = 0;

    if(dec_len != NULL)
        *dec_len = 0;

    if(data == NULL || data_len <= 0)
        return NULL;

    if (*(data + data_len - 1) == '=')
        equal_count += 1;

    if (*(data + data_len - 2) == '=')
        equal_count += 1;

    if (*(data + data_len - 3) == '=')
        equal_count += 1;

    if(dec_len != NULL)
        *dec_len = (int)(ret_len - equal_count);

    switch (equal_count)
    {
    case 0:
        ret_len += 4;//3 + 1 [1 for NULL]
        break;
    case 1:
        ret_len += 4;//Ceil((6*3)/8)+1
        break;
    case 2:
        ret_len += 3;//Ceil((6*2)/8)+1
        break;
    case 3:
        ret_len += 2;//Ceil((6*1)/8)+1
        break;
    }

    ret = (char *)ONVIF_MALLOC(ret_len);
    if (ret == NULL)
        return NULL;

    memset(ret, 0, ret_len);
    f = ret;

    while (tmp < (data_len - equal_count))
    {
        temp = 0;
        prepare = 0;

        while (temp < 4)
        {
            if (tmp >= (data_len - equal_count))
                break;

            prepare = (prepare << 6) | (FindBASE64(data[tmp]));
            temp++;
            tmp++;
        }

        prepare = prepare << ((4 - temp) * 6);

        for (i = 0; i < 3 ; i++ )
        {
            if (i == temp)
                break;

            *f = (char)((prepare >> ((2 - i) * 8)) & 0xFF);
            f++;
        }
    }

    *f = '\0';
    return ret;
}

unsigned int ovfs_onvif_Packbits(unsigned char *src, unsigned char *dst,
                                 unsigned int n)
{
    unsigned char *p, *q, *run, *dataend;
    int count, maxrun;

    dataend = src + n;
    for( run = src, q = dst; n > 0; run = p, n -= count )
    {
        // A run cannot be longer than 128 bytes.
        maxrun = n < 128 ? n : 128;
        if(run <= (dataend - 3) && run[1] == run[0] && run[2] == run[0])
        {
            // 'run' points to at least three duplicated values.
            // Step forward until run length limit, end of input,
            // or a non matching byte:
            for( p = run + 3; p < (run + maxrun) && *p == run[0]; )
                ++p;
            count = p - run;

            // replace this run in output with two bytes:
            *q++ = 1 + 256 - count; /* flag byte, which encodes count (129..254) */
            *q++ = run[0];      /* byte value that is duplicated */
        }
        else
        {
            // If the input doesn't begin with at least 3 duplicated values,
            // then copy the input block, up to the run length limit,
            // end of input, or until we see three duplicated values:
            for( p = run; p < (run + maxrun); )
                if(p <= (dataend - 3) && p[1] == p[0] && p[2] == p[0])
                    break; // 3 bytes repeated end verbatim run
                else
                    ++p;
            count = p - run;
            *q++ = count - 1;      /* flag byte, which encodes count (0..127) */
            memcpy(q, run, count); /* followed by the bytes in the run */
            q += count;
        }
    }

    return q - dst;
}

unsigned int ovfs_onvif_UnPackbits(unsigned char *outp, unsigned char *inp,
                                   unsigned int outlen, unsigned int inlen)
{
    unsigned int i, len;
    int val;

    /* i counts output bytes; outlen = expected output size */
    for(i = 0; inlen > 1 && i < outlen;)
    {
        /* get flag byte */
        len = *inp++;
        --inlen;

        if(len == 128) /* ignore this flag value */
            ; // warn_msg("RLE flag byte=128 ignored");
        else
        {
            if(len > 128)
            {
                len = 1 + 256 - len;

                /* get value to repeat */
                val = *inp++;
                --inlen;

                if((i + len) <= outlen)
                    memset(outp, val, len);
                else
                {
                    memset(outp, val, outlen - i); // fill enough to complete row
                    printf("unpacked RLE data would overflow row (run)\n");
                    len = 0; // effectively ignore this run, probably corrupt flag byte
                }
            }
            else
            {
                ++len;
                if((i + len) <= outlen)
                {
                    if(len > inlen)
                        break; // abort - ran out of input data
                    /* copy verbatim run */
                    memcpy(outp, inp, len);
                    inp += len;
                    inlen -= len;
                }
                else
                {
                    memcpy(outp, inp, outlen - i); // copy enough to complete row
                    printf("unpacked RLE data would overflow row (copy)\n");
                    len = 0; // effectively ignore
                }
            }
            outp += len;
            i += len;
        }
    }

    /*
        if(i < outlen)
            printf("not enough RLE data for row\n");
    */

    return i;
}

int Utils_GetNetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask)
{
    int ret = 0;
    if (ifname == NULL)
    {
        LOGW("ifname is null\n");
        return -1;
    }

    struct ifreq ifr;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        LOGE("create socket failed %s\n", strerror(errno));
        return -1;
    }


    strncpy(ifr.ifr_name, ifname, 16);
    if (hwaddr != NULL)
    {
        if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0)
        {
            snprintf(hwaddr, 32, "%02X:%02X:%02X:%02X:%02X:%02X",
                     (unsigned char) ifr.ifr_hwaddr.sa_data[0],
                     (unsigned char) ifr.ifr_hwaddr.sa_data[1],
                     (unsigned char) ifr.ifr_hwaddr.sa_data[2],
                     (unsigned char) ifr.ifr_hwaddr.sa_data[3],
                     (unsigned char) ifr.ifr_hwaddr.sa_data[4],
                     (unsigned char) ifr.ifr_hwaddr.sa_data[5]);
        }
        else
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }

    if (ipv4 != NULL)
    {
        if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0)
        {
            struct sockaddr_in *myaddr;
            myaddr = (struct sockaddr_in *) &ifr.ifr_addr;
            memcpy(ipv4, inet_ntoa(myaddr->sin_addr), 32);
        }
        else
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }

    if (netmask != NULL)
    {
        if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0)
        {
            struct sockaddr_in *myaddr;
            myaddr = (struct sockaddr_in *) &ifr.ifr_netmask;
            memcpy(netmask, inet_ntoa(myaddr->sin_addr), 32);
        }
        else
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }


    if (skfd >= 0)
    {
        close(skfd);
        skfd = -1;
    }

    return ret;
}

int Utils_SetNetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask)
{
    int ret = 0;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        LOGE("create socket failed %s\n", strerror(errno));
        return -1;
    }
    strncpy(ifr.ifr_name, ifname, 16);

    if (ipv4)
    {
        struct sockaddr_in *addr;
        addr = (struct sockaddr_in *) & (ifr.ifr_addr);
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr(ipv4);
        if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0)
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }

    if (netmask)
    {
        struct sockaddr_in *addr;
        addr = (struct sockaddr_in *) & (ifr.ifr_addr);
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr(netmask);
        if (ioctl(skfd, SIOCSIFNETMASK, &ifr) < 0)
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }

    if (skfd >= 0)
    {
        close(skfd);
        skfd = -1;
    }

    return ret;
}

int Utils_ArpingCheckIp(char *remoteIpaddr, char *localIpaddr, char *mac, char *dev,
                        unsigned long long int timeout)
{
    struct in_addr temp;
    //LOGD("remoteIpaddr:[%s] localIpaddr:[%s]]\n",remoteIpaddr,localIpaddr);
    //u_int32_t raddr = inet_addr(remoteIpaddr);
    //u_int32_t laddr = inet_addr(localIpaddr);
    //LOGD("raddr:[%x] laddr:[%x]]\n",raddr,laddr);
    if (remoteIpaddr == NULL || localIpaddr == NULL || mac == NULL ||
            dev == NULL)
    {
        LOGE("param error\n");
        return -1;
    }

    timeout = timeout + 1000LLU;

    if (Arpping(remoteIpaddr, localIpaddr, /*raddr, laddr,*/ (unsigned char *) mac, dev, timeout) == 1)
    {
        temp.s_addr = inet_addr(remoteIpaddr);//raddr;
        LOGE("%s belongs to someone\n", inet_ntoa(temp));
        return 1;
    }
    else
    {
        return 0;
    }

    return 0;
}

//从字符串中提取十进制数字
int Utils_PickDecNumber(const char* str)
{
    int value;

    if (! str)
    {
        return 0;
    }
    value = 0;
	while (*str != 0)
	{
	    if ((*str >= '0') && (*str <= '9'))
	    {
	        value = value*10 + (*str - '0');
	    }
		str++;
	}
    return value;
}

/*获取本地某个tcp端口的连接数量*/
int Utils_CheckTcpPortIsConnected(int port, int *connectedNum)
{
    if (port <= 0 || connectedNum == NULL)
        return -1;

    FILE *fp = fopen("/proc/net/tcp", "rb");
    if (fp == NULL)
        return -1;

    char lineBuf[256] = {};
    char *rp  = NULL;

    do
    {
        rp = fgets(lineBuf, sizeof(lineBuf), fp);
        if (rp == NULL)
            break;

        char ipStr[16] = {};
        char portStr[8] = {};
        int portInt = 0;

        sscanf(lineBuf, "%*[^:]: %15[^:]:%7[^ ]", ipStr, portStr);
        if (strlen(ipStr) == 0)
            continue;

        portInt = strtol(portStr, NULL, 16);
        if (port == portInt && strcmp(ipStr, "00000000") != 0)
            *connectedNum += 1;

    }
    while(rp != NULL);

    fclose(fp);
    return 0;
}

int Utils_GetNetMaskBit(char *netmask)
{
    if (netmask == NULL)
        return -1;

    in_addr_t ad = inet_network(netmask);
    if ((int)ad == -1)
        return -1;

    /*计算整数中有多少个 1*/
    int n = ad;
    n = (n & 0x55555555) + ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f);
    n = (n & 0x00ff00ff) + ((n >> 8) & 0x00ff00ff);
    n = (n & 0x0000ffff) + ((n >> 16) & 0x0000ffff);

    return n;
}
