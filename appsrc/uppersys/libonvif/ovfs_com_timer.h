/*
 * ovfs_com_timer.h
 *
 *  Created on: 2016年4月9日
 *      Author: eric
 */

#ifndef OVFS_COM__TIMER_H_
#define OVFS_COM_TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "ovfs_com_mq.h"
/*
 * 名称:    Timer_Init
 *               初始化定时器,定时器在一个进程内只能有一个, 相关参数和句柄,存储于内部全局变量中
 * 参数:
 *              mq_handle - 消息队列handle, 必须已经初始化
 *              max_id - 定时器最大id
 * 返回:
 *              0     - 成功
 *              <0   - 失败
 */
int Timer_Init(unsigned int maxId, MQ_HANDLE_H mqHandle);

/*
 * 名称:    Timer_Uninit
 *               销毁定时器, 定时器在一个进程内只能有一个, 相关参数和句柄,存储于内部全局变量中
 * 参数:
 * 返回:
 */
void Timer_Uninit();

/*
 * 名称:    Timer_Start
 *               开启一个定时器
 * 参数:
 *              timer_id - timer id , 用户自定义 不能超过初始化时提供的最大值
 *              interval - 定时毫秒数
 * 返回:
 *              0     - 成功
 *              <0   - 失败
 */
int  Timer_Start(unsigned int timerId, unsigned int interval);

/*
 * 名称:    Timer_ReadLeft
 *               获得timer_id剩余的时间
 * 参数:
 *              timer_id - timer id , 用户自定义 不能超过初始化时提供的最大值
 * 返回:
 *              >=0   - timer_id 的剩余时间
 */
unsigned int Timer_ReadLeft(unsigned int timerId);

/*
 * 名称:    Timer_Stop
 *               停止timer_id这个定时
 * 参数:
 *              timer_id - timer id , 用户自定义 不能超过初始化时提供的最大值
 * 返回:
 *              =0   -  成功
 *              <0   - 失败
 */
int Timer_Stop(unsigned int timerId);

/*
 * 名称:    Timer_SignalMask
 *              设置线程信号,由于当前进程间通讯使用了自定义信号,
 *              所以在main函数创建其他线程以前,需要调用本函数.否则进程在收到自定义信号后,会退出
 * 参数:
 * 返回:
 */
void Timer_SignalMask();

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* OVFS_COM_TIMER_H_ */
