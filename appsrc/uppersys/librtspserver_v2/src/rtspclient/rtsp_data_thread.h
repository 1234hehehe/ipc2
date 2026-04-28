#ifndef _RTSP_DATA_THREAD_H_
#define _RTSP_DATA_THREAD_H_
#include <jthread/jthread.h>
#include <rtpsession.h>


#include "rtsp_common.h"
#include "rtsp_msg.h"


using namespace jthread;
using namespace jrtplib;
class CClientPacketRecvThread:public JThread
{
public:
    CClientPacketRecvThread();
    ~CClientPacketRecvThread();
    void SetCallback(void *(*ThreadCallback)());
private:
    void *Thread();
    void *(*m_ThreadCallback)();


};
#endif