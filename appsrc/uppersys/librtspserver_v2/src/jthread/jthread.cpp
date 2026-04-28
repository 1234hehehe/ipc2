/*

    This file is a part of the JThread package, which contains some object-
    oriented thread wrappers for different thread implementations.

    Copyright (c) 2000-2011  Jori Liesenborgs (jori.liesenborgs@gmail.com)

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

*/

#include "jthread.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include "jmutexautolock.h"

#ifndef _WIN32_WCE
	#include <process.h>
#endif // _WIN32_WCE

#else
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#endif

namespace jthread
{

JThread::JThread()
{
	retval = NULL;
	mutexinit = false;
	running = false;
}

JThread::~JThread()
{
	Kill();
}

int JThread::Start(int size)
{
#if defined(WIN32) || defined(_WIN32_WCE)
#else
	int status;
#endif
	if (!mutexinit)
	{
		if (!runningmutex.IsInitialized())
		{
			if (runningmutex.Init() < 0)
				return ERR_JTHREAD_CANTINITMUTEX;
		}
		if (!continuemutex.IsInitialized())
		{
			if (continuemutex.Init() < 0)
				return ERR_JTHREAD_CANTINITMUTEX;
		}
		if (!continuemutex2.IsInitialized())
		{
			if (continuemutex2.Init() < 0)
				return ERR_JTHREAD_CANTINITMUTEX;
		}
		mutexinit = true;
	}
	
	runningmutex.Lock();
	if (running)
	{
		runningmutex.Unlock();
		return ERR_JTHREAD_ALREADYRUNNING;
	}
	runningmutex.Unlock();
	
	continuemutex.Lock();
#if defined(WIN32) || defined(_WIN32_WCE)
#ifndef _WIN32_WCE
	threadhandle = (HANDLE)_beginthreadex(NULL,0,TheThread,this,0,&threadid);
#else
	threadhandle = CreateThread(NULL,0,TheThread,this,0,&threadid);
#endif // _WIN32_WCE
	if (threadhandle == NULL)
	{
		continuemutex.Unlock();
		return ERR_JTHREAD_CANTSTARTTHREAD;
	}
#else
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	size_t stacksize = PTHREAD_STACK_MIN * 64; // 16KB
	if (size > 0)
	{
        stacksize = size;
	}
    
    pthread_attr_setstacksize(&attr,stacksize);
	status = pthread_create(&threadid,&attr,TheThread,this);	
	pthread_attr_destroy(&attr);
	if (status != 0)
	{
		
		continuemutex.Unlock();
		

		return ERR_JTHREAD_CANTSTARTTHREAD;
	}
#endif
	
	/* Wait until 'running' is set */
	
	runningmutex.Lock();			
	while (!running)
	{
		runningmutex.Unlock();
#if defined(WIN32) || defined(_WIN32_WCE)
		Sleep(1);
#else	
#if 0
		struct timespec req,rem;

		req.tv_sec = 0;
		req.tv_nsec = 10000000;
		nanosleep(&req,&rem);
#endif
        struct timeval tv;
        fd_set			rfds;
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 10000;		
        select(0, &rfds, NULL, NULL, &tv);		
#endif
		runningmutex.Lock();
	}
	runningmutex.Unlock();
	
	continuemutex.Unlock();
	
	continuemutex2.Lock();
	continuemutex2.Unlock();
		
	return 0;
}

int JThread::Kill()
{
	runningmutex.Lock();			
	if (!running)
	{
		runningmutex.Unlock();
		return ERR_JTHREAD_NOTRUNNING;
	}
#if defined(WIN32) || defined(_WIN32_WCE)
	TerminateThread(threadhandle,0);
	CloseHandle(threadhandle);
#elif defined(ON_ANDROID)

#else
	pthread_cancel(threadid);
#endif
	running = false;
	runningmutex.Unlock();
	return 0;
}

bool JThread::IsRunning()
{
	bool r;
	
	runningmutex.Lock();			
	r = running;
	runningmutex.Unlock();
	return r;
}

void *JThread::GetReturnValue()
{
	void *val;
#if defined(WIN32) || defined(_WIN32_WCE)
	JMutexAutoLock autolock(runningmutex);
	if (running)
		val = NULL;
	else
		val = retval;
#else
	runningmutex.Lock();
	if (running)
		val = NULL;
	else
		val = retval;
	runningmutex.Unlock();
#endif
	return val;
}

#if defined(WIN32) || defined(_WIN32_WCE)
#ifndef _WIN32_WCE
UINT __stdcall JThread::TheThread(void *param)
#else
DWORD WINAPI JThread::TheThread(void *param)
#endif // _WIN32_WCE
#else
void *JThread::TheThread(void *param)
#endif
{
	JThread *jthread;
	void *ret;
	
	jthread = (JThread *)param;
	
	jthread->continuemutex2.Lock();
	jthread->runningmutex.Lock();
	jthread->running = true;
	jthread->runningmutex.Unlock();
	
	jthread->continuemutex.Lock();
	jthread->continuemutex.Unlock();
	
	ret = jthread->Thread();
	
	jthread->runningmutex.Lock();
	jthread->running = false;
	jthread->retval = ret;
#if defined(WIN32) || defined(_WIN32_WCE)
	CloseHandle(jthread->threadhandle);
#endif
	jthread->runningmutex.Unlock();
#if defined(WIN32) || defined(_WIN32_WCE)
	return 0;		
#else
	return NULL;
#endif
}

void JThread::ThreadStarted()
{
	continuemutex2.Unlock();
}

} // end namespace

