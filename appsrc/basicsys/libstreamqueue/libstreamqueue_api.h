/*
 * libstreamqueue_api.h
 *
 *  Created on: 2016年6月6日
 *      Author: eric
 */

#ifndef LIBSTREAMQUEUE_API_H_
#define LIBSTREAMQUEUE_API_H_

#ifdef WIN32
#define LIBSTREAMQUEUE_API __declspec(dllexport)
#else
#define LIBSTREAMQUEUE_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define STREAM_QUEUE_OPEN_FLAG_CREATE                   (1 << 0)
#define STREAM_QUEUE_OPEN_FLAG_WRITE                    (1 << 1)
#define STREAM_QUEUE_OPEN_FLAG_READ                     (1 << 2)

// 淘汰机制 :
#define STREAM_QUEUE_CREATE_FLAG_ELIMINATED_SMALL       (1 << 16) // 有淘汰机制,最少内存方式

/* EPOLL 事件*/
#define STREAM_QUEUE_EPOLL_EVENT_READ                   1
#define STREAM_QUEUE_EPOLL_EVENT_WRITE                  2
#define STREAM_QUEUE_EPOLL_EVENT_RW                     3

/*EPOLL 控制*/
#define STREAM_QUEUE_EPOLL_CTL_ADD                      1
#define STREAM_QUEUE_EPOLL_CTL_RM                       2
#define STREAM_QUEUE_EPOLL_CTL_MDD                      3

typedef struct
{
    int streamQueueHandle;
    void *userData;
    int event; // 参考定义 STREAM_QUEUE_EPOLL_EVENT_*
} STREAM_QUEUE_EPOLL_EVENT_T;

/*
 * 名称:    StreamQueue_Open
 *              初始化打开流队列
 * 参数:
 *              queueName  - 队列名称,最大32-1字节
 *              openFlag   - 打开模式,相关模式参考上面的宏定义
 *              maxMemSize - 内存最大大小
 *              maxMemNum  - 内存最大个数
 * 返回:
 *              > 0  -   成功, 返回fd
 *              < 0  -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_Open(char *queueName,int openFlag,int maxMemSize,int maxNodeNum);

/*
 * 名称:    StreamQueue_Close
 *              销毁流队列
 * 参数:
 *              streamQueueHandle - 流队列handle
 * 返回:
 *              <0 - 失败
 *              =0 - 成功
 */
LIBSTREAMQUEUE_API int StreamQueue_Close(int streamQueueHandle);

/*
 * 名称:    StreamQueue_WriteData
 *              插入新数据到流队列尾部
 * 参数:
 *              streamQueueHandle - 流队列handle
 *              userHeader - 用户自定义数据头
 *              userSize  - 用户自定义数据大小
 *              head_data - 数据头 , 可以为NULL
 *              head_size - 数据头大小,可以为0,当head_data = NULL
 *              data - 数据体
 *              dataSize - 数据体大小
 *              tag - 数据标识
 * 返回:
 *              = 0   -   成功
 *              < 0   -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_WriteData(int streamQueueHandle, void *userData, unsigned int userSize, void *headData,
                unsigned int headSize, void *data, unsigned int dataSize, int tag);

/*
 * 名称:    StreamQueue_ReadData
 *              从流队列中获得数据
 * 参数:
 *              streamQueueHandle - 流队列handle
 *              userHeader - 用户自定义数据头
 *              userSize  - 用户自定义数据大小
 *              data - 数据体
 *              size - 数据体大小
 *              timeout - 超时毫秒
 *
 * 返回:
 *              = 0   -   成功
 *              < 0   -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_ReadData(int streamQueueHandle, void **userData, unsigned int *userSize, void **data, unsigned int *size, unsigned int timeout);

/*
 * 名称:    StreamQueue_ReleaseData
 *              释放上次获得的流节点
 * 参数:
 *              streamQueueHandle - handle
 * 返回:
 *              = 0   -   成功
 *              < 0   -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_ReleaseData(int streamQueueHandle);

/*
 * 名称:    StreamQueue_ClearData
 *              释放流的所有节点,淘汰机制1适用
 * 参数:
 *              streamQueueHandle - handle
 * 返回:
 *              = 0   -   成功
 *              < 0   -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_ClearData(int streamQueueHandle);

/*
 * 名称:    StreamQueue_Control
 *              控制流接口，用来偏移节点到指定tag的节点，需要release已经read的节点后操作
 * 参数:
 *              streamQueueHandle - handle
 *              tag - 节点数据标识，写入节点时的tag
 * 返回:
 *              = 0   -   成功
 *              < 0   -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_Control(int streamQueueHandle, int tag);

/*
 * 名称:    StreamQueue_EpollCreate
 *              创建一个epollHandle
 * 参数:
 * 返回:
 *              >0    -   成功, 返回epollHandle
 *              <0    -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_EpollCreate();

/*
 * 名称:    StreamQueue_EpollDestroy
 *              销毁一个epollHandle
 * 参数:
 *              epollHandle  - epollHandle
 * 返回:
 *              =0    -   成功
 *              <0    -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_EpollDestroy(int epollHandle);

/*
 * 名称:    StreamQueue_EpollCtl
 *              epollHanlde控制接口
 * 参数:
 *              epollHandle          - epollHanlde
 *              ctl                  - 控制类型参考定义 STREMA_QUEUE_EPOLL_CTL_*
 *              events               -
 * 返回:
 *              != NULL   -   成功, 返回有新数据加入的streamQueueHandle
 *              == NULL   -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_EpollCtl(int epollHandle, int ctl, STREAM_QUEUE_EPOLL_EVENT_T *events);

/*
 * 名称:    StreamQueue_EpollWait
 *               等待epollHandle上的事件
 * 参数:
 *              epollHandle          - epollHandle
 *              events               - 事件指针
 *              maxEvents            - 事件最大个数
 *              timeout              - 超时，毫秒数 =0 表示立刻返回结果, -1 表示阻塞
 * 返回:
 *              >0    -   成功, 返回事件个数
 *              =0    -   超时
 *              <0    -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_EpollWait(int epollHandle, STREAM_QUEUE_EPOLL_EVENT_T *events, int maxEvents, int timeout);

/*
 * 名称:    StreamQueue_GetRestCnt
 *               获取队列剩余节点数
 * 参数:
 *              streamQueueHandle - handle
 * 返回:
 *              >0    -   成功, 返回个数
 *              <0    -   失败 , 查看错误码
 */
LIBSTREAMQUEUE_API int StreamQueue_GetRestCnt(int streamQueueHandle);
#ifdef __cplusplus
}
#endif
#endif /* LIBSTREAMQUEUE_API_H_ */
