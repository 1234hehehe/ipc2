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

#include "jmutex.h"

namespace jthread
{

JMutex::JMutex()
{
	initialized = false;
}

JMutex::~JMutex()
{
#if defined(WIN32) || defined(_WIN32_WCE)
	if (initialized)
#ifdef JTHREAD_CONFIG_JMUTEXCRITICALSECTION
		DeleteCriticalSection(&mutex);
#else
		CloseHandle(mutex);
#endif // JTHREAD_CONFIG_JMUTEXCRITICALSECTION
#else
	if (initialized)
		pthread_mutex_destroy(&mutex);
#endif
}

int JMutex::Init()
{
#if defined(WIN32) || defined(_WIN32_WCE)
	if (initialized)
		return ERR_JMUTEX_ALREADYINIT;
#ifdef JTHREAD_CONFIG_JMUTEXCRITICALSECTION
	InitializeCriticalSection(&mutex);
#else
	mutex = CreateMutex(NULL,FALSE,NULL);
	if (mutex == NULL)
		return ERR_JMUTEX_CANTCREATEMUTEX;
#endif // JTHREAD_CONFIG_JMUTEXCRITICALSECTION
#else
	if (initialized)
		return ERR_JMUTEX_ALREADYINIT;
    pthread_mutexattr_t attr; 
    pthread_mutexattr_init(&attr); 
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&mutex,&attr);
#endif
	initialized = true;
	return 0;
}

int JMutex::Lock()
{
	if (!initialized)
		return ERR_JMUTEX_NOTINIT;
#if defined(WIN32) || defined(_WIN32_WCE)
#ifdef JTHREAD_CONFIG_JMUTEXCRITICALSECTION
	EnterCriticalSection(&mutex);
#else
	WaitForSingleObject(mutex,INFINITE);
#endif // JTHREAD_CONFIG_JMUTEXCRITICALSECTION
#else		
	pthread_mutex_lock(&mutex);
#endif
	return 0;
}

int JMutex::Unlock()
{
	if (!initialized)
		return ERR_JMUTEX_NOTINIT;
#if defined(WIN32) || defined(_WIN32_WCE)
#ifdef JTHREAD_CONFIG_JMUTEXCRITICALSECTION
	LeaveCriticalSection(&mutex);
#else
	ReleaseMutex(mutex);
#endif // JTHREAD_CONFIG_JMUTEXCRITICALSECTION
#else
	pthread_mutex_unlock(&mutex);
#endif
	return 0;
}

CAntsSemaphore::CAntsSemaphore(int lInitialCount ,int lMaxCount ,char *pstrName )
{
#ifndef WIN32
    sem_init(&sem, 0, lInitialCount);
    m_hSem = &sem;
#else
    m_hSem = CreateSemaphore(NULL,lInitialCount,lMaxCount,(LPCSTR)pstrName);
#endif
}
CAntsSemaphore::~CAntsSemaphore()
{
    if (m_hSem == NULL)
    {
        return;
    }
#ifndef WIN32
    sem_destroy(&sem);
#else
    CloseHandle(m_hSem);
#endif
    m_hSem = NULL;
}
int CAntsSemaphore::Post()
{
    if (m_hSem == NULL)
    {
        return 0;
    }
#ifndef WIN32
    sem_post(&sem);

#else
    if(!ReleaseSemaphore((HANDLE)m_hSem,1,NULL))
    {
        return 0;
    }
#endif
    return 1;
}
int CAntsSemaphore::Pend()
{
    if(m_hSem == NULL)
        return 0;
#ifndef WIN32
    sem_wait(&sem);
    return 1;
#else
    DWORD Ret;
    Ret = WaitForSingleObject((HANDLE)m_hSem,	INFINITE);
    if(Ret == WAIT_OBJECT_0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

int CAntsSemaphore::Pend(unsigned int dwMilliseconds)
{
    if(m_hSem == NULL)
        return 0;
#ifndef WIN32
    do{


        if(!sem_trywait(&sem))
        {
            //³É¹¦
            return 1;
        }
        if (dwMilliseconds == 0)
        {
            break;
        }
    

        struct timespec req,rem;

        req.tv_sec = 0;
        req.tv_nsec = 1000000;
        nanosleep(&req,&rem);
        dwMilliseconds--;
    }while(dwMilliseconds);

    return 0;
#else
    DWORD Ret;
    Ret = WaitForSingleObject((HANDLE)m_hSem,	dwMilliseconds);
    if(Ret == WAIT_OBJECT_0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

void CAntsSemaphore::Set(int lInitialCount ,int lMaxCount ,char *pstrName)
{
#ifndef WIN32
#else
    if(m_hSem != NULL)
        return ;
    m_hSem = CreateSemaphore(NULL,lInitialCount,lMaxCount,(LPCSTR)pstrName);
#endif

}

} // end namespace

