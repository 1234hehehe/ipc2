#include "rtsp_data_thread.h"

#include "rtspclient.h"

#ifndef RTSP_NO_CLIENT

extern void *ANTS_RTSPClientPacketRecvThread();
CClientPacketRecvThread::CClientPacketRecvThread()
{
    m_ThreadCallback = NULL;
}

CClientPacketRecvThread::~CClientPacketRecvThread()
{
    
}

void CClientPacketRecvThread::SetCallback(void *(*ThreadCallback)())
{
    m_ThreadCallback = ThreadCallback;
}

void *CClientPacketRecvThread::Thread()
{
    ThreadStarted();
    if (m_ThreadCallback != NULL)
    {
        return m_ThreadCallback();
    }
    return NULL;
}

#endif
