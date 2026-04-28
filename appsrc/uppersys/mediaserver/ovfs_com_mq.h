/*
 * ovfs_com_mq.h
 *
 *  Created on: 2016年4月8日
 *      Author: eric
 */

#ifndef OVFS_COM_MQ_H_
#define OVFS_COM_MQ_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** min length of MQ (message queue) , also is the default value*/
#define OVFS_MQ_QUEUE_MIN_LEN                              100

/** if length of MQ larger than this , app will be set busy state */
#define OVFS_MQ_QUEUE_WARN_MAX                          80

/**  if app state was busy , length of MQ little than this , app will be set idle state*/
#define OVFS_MQ_QUEUE_WARN_MIN                          50

/*
 * 消息队列接口
 */

/*
 * 消息队列handle
 */
typedef void * MQ_HANDLE_H;

/*
 * 名称:    MQ_DATA_F
 *               消息队列Mq_MsgLoop回调函数,用来处理进程间通信(IPC)数据类型
 * 参数:
 *              id - app id , 数据发送者的app id
 *              ipcDataType - IPC 数据类型和事件, 请查看ovfs_ipc.h
 *              ipcDataToken - IPC 接收数据的token
 *              data -  IPC接收到的数据
 *              size -  IPC接收到的数据大小
 * 返回:
 */
typedef void (*MQ_DATA_F)(unsigned int id, unsigned int ipcDataType, unsigned int ipcDataToken, void *data, unsigned int size);

/*
 * 名称:    MQ_TIMER_F
 *               消息队列Mq_MsgLoop回调函数,用来处理定时器事件类型
 * 参数:
 *              id - timer id 定时器id
 * 返回:
 */
typedef void (*MQ_TIMER_F)(unsigned int id);

/*
 * 名称:    MQ_MSG_F
 *               消息队列Mq_MsgLoop回调函数,用来处理程序内部消息类型
 * 参数:
 *              id - 内部消息 id
 *              data -  内部消息数据
 *              size -  数据大小
 * 返回:
 */
typedef void (*MQ_MSG_F)(unsigned int id, void *data, unsigned int size);

/*
 * 名称:    MQ_REQ_F
 *               消息队列Mq_MsgLoop回调函数,用来处理程序内部线程间请求消息类型
 * 参数:
 *              id - 请求id
 *              reqInData - 请求参数
 *              reqInSize -  请求参数大小
 *              reqOutData -  请求输出
 *              reqOutSize -  请求输出大小
 * 返回:
 */
typedef void (*MQ_REQ_F)(unsigned int id, void *reqInData, unsigned int reqInSize, int *reqRet, void *reqOutData, unsigned int reqOutSize);

/*
 * 名称:    Mq_Init
 *               初始化消息队列
 * 参数:
 *              mqHandle - 消息队列handle , 将会初始化及分配内存
 *              timeout - 消息队列超时时间 , 消息队列循环读取超时时间,一般设置为0
 * 返回:
 *              0     - 成功
 *              <0   - 失败
 */
int Mq_Init(MQ_HANDLE_H *mqHandle, unsigned int timeout);

/*
 * 名称:    Mq_SetMaxLength
 *               设置消息队列最大长度, 消息队列默认值为MQ_QUEUE_MIN_LEN
 * 参数:
 *              mqHandle - 消息队列handle
 *              length - >= MQ_QUEUE_MIN_LEN ,消息队列最大长度必須大於最小值MQ_QUEUE_MIN_LEN
 * 返回:
 *              0     - 成功
 *              <0   - 失败
 */
int Mq_SetMaxLength(MQ_HANDLE_H mqHandle, int length);

/*
 * 名称:    Mq_GetMaxLength
 *               获得消息队列最大长度, 消息队列默认值为MQ_QUEUE_MIN_LEN
 * 参数:
 *              mqHandle - 消息队列handle
 * 返回:
 *              >0     - 成功 , 返回当前MQ最大值
 *              <0   - 失败
 */
int Mq_GetMaxLength(MQ_HANDLE_H mqHandle);

/*
 * 名称:    MqGetLength
 *               获得消息队列当前长度
 * 参数:
 *              mqHandle - 消息队列handle
 * 返回:
 *              >0     - 成功 , 返回当前MQ长度
 *              <0   - 失败
 */
int MqGetLength(MQ_HANDLE_H mqHandle);

/*
 * 名称:    Mq_Uninit
 *               销毁当前MQ,删除所有消息队列的节点并且释放节点内存
 * 参数:
 *              mq_handle - 消息队列handle
 * 返回:
 */
void Mq_Uninit(MQ_HANDLE_H *mqHandle);

/*
 * 名称:    Mq_DelTimer
 *               删除消息队列中,一个定时器的节点
 * 参数:
 *              mqHandle - 消息队列handle
 *              id  -  定时器id , 用来比较队列中的定时器节点,当匹配成功后,将删除此节点
 * 返回:
 */
void Mq_DelTimer(MQ_HANDLE_H mqHandle, unsigned int id);

/*
 * 名称:    Mq_PostIpcData
 *               将从socket接收到的IPC数据,插入队列中
 * 参数:
 *              mqHandle - 消息队列handle
 *              ipcType - IPC数据事件类型
 *              appid     -  IPC数据发送者appid
 *              token     - IPC数据token
 *              data       - IPC数据
 *              size        - IPC数据大小
 * 返回:
 *              =0     -  成功
 *              <0     -  失败
 */
int Mq_PostIpcData(MQ_HANDLE_H mqHandle, unsigned char ipcType, unsigned int appid, unsigned int token, void *data, unsigned int size);

/*
 * 名称:    Mq_PostIpcEvent
 *               将从socket接收到的IPC事件,插入队列中
 * 参数:
 *              mqHandle - 消息队列handle
 *              ipcType - IPC数据事件类型
 *              appid     -  IPC数据发送者appid
 *              data       - IPC数据
 *              size        - IPC数据大小
 * 返回:
 *              =0     -  成功
 *              <0     -  失败
 */
int Mq_PostIpcEvent(MQ_HANDLE_H mqHandle, unsigned char ipcType, unsigned int appid, void *data, unsigned int size);

/*
 * 名称:    Mq_PostTimer
 *               将已经到时的定时器事件插入到MQ队列中
 * 参数:
 *              mqHandle - 消息队列handle
 *              id - timer id
 * 返回:
 *              =0     -  成功
 *              <0     -  失败
 */
int Mq_PostTimer(MQ_HANDLE_H mqHandle, unsigned int id);

/*
 * 名称:    Mq_PostMsg
 *               将进程内部的事件数据消息插入到MQ队列中
 * 参数:
 *              mqHandle - 消息队列handle
 *              id - 消息事件id
 *              data - 消息事件数据
 *              size -  消息事件数据大小
 * 返回:
 *              =0     -  成功
 *              <0     -  失败
 */
int Mq_PostMsg(MQ_HANDLE_H mqHandle, unsigned int id, void *data, unsigned int size);

/*
 * 名称:    Mq_Request
 *              线程间同步请求接口
 * 参数:
 *              mqHandle - 消息队列handle
 *              id - 消息事件id
 *              reqInData - 请求参数
 *              reqInSize -  请求参数大小
 *              reqOutData - 应答参数
 *              reqOutSize -  应答参数大小
 * 返回:
 *              =0     -  成功
 *              <0     -  失败
 */
int Mq_Request(MQ_HANDLE_H mqHandle, unsigned int id, void *reqInData, unsigned int reqInSize, int *reqRet, void *reqOutData, unsigned int reqOutSize);

/*
 * 名称:    Mq_CheckState
 *               检查当期MQ的状态
 * 参数:
 *              mqHandle - 消息队列handle
 * 返回:
 *              =0     -  空闲
 *              =1     -  繁忙
 *              <0     -  失败
 */
int Mq_CheckState(MQ_HANDLE_H mqHandle);

/*
 * 名称:    Mq_MsgLoop
 *               消息队列循环, 循环从队列中取出数据节点,根据数据类型,调用对应的回调函数 ,处理完成后释放节点
 * 参数:
 *              mqHandle - 消息队列handle
 *              DataCallback - IPC接收到的消息处理回调
 *              TimerCallback - 某个定时器到时的处理回调
 *              MsgCallback - 进程内部消息的处理回调
 *              ReqCallback - 进程内部同步请求消息的处理回调
 * 返回:
 */
void Mq_MsgLoop(MQ_HANDLE_H mqHandle, MQ_DATA_F DataCallback, MQ_TIMER_F TimerCallback,
                MQ_MSG_F MsgCallback, MQ_REQ_F ReqCallback);

/*
 * 名称:    Mq_QuitLoop
 *               退出消息队列循环
 * 参数:
 *              mqHandle - 消息队列handle
 * 返回:
 */
void Mq_QuitLoop(MQ_HANDLE_H mqHandle);

#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif /* OVFS_COM_MQ_H_ */
