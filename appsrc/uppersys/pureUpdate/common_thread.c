#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcommon_struct.h"
#include "libcommon_api.h"

typedef struct _tagModule_ThreadObject 
{
	Common_Thread_T hThreadHandle;
	S8 *szThreadName;
	Common_Thread_def fxn;
	void *pUserData;
	S32 bDetach;
	S32 bExit;
	S32 bReady;
	//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
	HANDLE hWin32handle;
	DWORD dwWin32ThreadID;
#else
	pthread_t hLinuxHandleID;
#endif

	struct _tagModule_ThreadObject *pPrev;
	struct _tagModule_ThreadObject *pNext;
}Module_ThreadObject_T;
static Module_ThreadObject_T *g_pThreadObjectList = NULL;
static Common_Lock_T g_hThreadObjectListLock = NULL;

typedef struct _tagModule_LockObject 
{
	Common_Lock_T hLockHandle;
	S8 *szLockName;
	//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
	HANDLE hWin32Mutex;
#else
    pthread_mutex_t hLinux_Mutex;
#endif
}Module_LockObject_T;

typedef struct _tagModule_RwLockObject 
{
	Common_RWLock_T hLockHandle;
	S8 *szLockName;
	//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
	HANDLE hWin32Mutex;
#else
	pthread_rwlock_t hLinux_Mutex;
#endif
}Module_RwLockObject_T;





#if defined(WIN32) || defined(_WIN32_WCE)
static DWORD Win32_thread_Routine(LPVOID lpThreadParameter)
#else
static void *linux_thread_Routine(void *lpThreadParameter)
#endif
{
	Module_ThreadObject_T *pObject = NULL;
	Module_ThreadObject_T *pObject_del = NULL,*pObject_del_Head = NULL;
	Module_ThreadObject_T *pThreadHandle = (Module_ThreadObject_T *)lpThreadParameter;
	if (pThreadHandle != NULL)
	{
		if(pThreadHandle->fxn != NULL)
		{
		    if (pThreadHandle->szThreadName && strlen(pThreadHandle->szThreadName) > 0)
            {
		        int nameOffset = strlen(pThreadHandle->szThreadName);
                nameOffset = (nameOffset - 15 > 0) ? (nameOffset - 15) : 0;
#ifdef WIN32
#else
                prctl(PR_SET_NAME, pThreadHandle->szThreadName + nameOffset);
#endif
            }
			pThreadHandle->fxn(pThreadHandle->hThreadHandle,pThreadHandle->pUserData);
		}
	}
	if (!pThreadHandle->bReady)
	{
		pThreadHandle->bExit = 1;
		return 0;
	}

	if(0 != Common_TryLock(g_hThreadObjectListLock))
	{
		pThreadHandle->bExit = 1;
		return 0;
	}
	
	pObject = g_pThreadObjectList;
	while(pObject != NULL)
	{
		if (pObject == pThreadHandle)
		{// 自己，先不销毁，等别的线程来销毁
			pObject = pObject->pNext;
			continue;
		}
		if (pObject->bDetach && pObject->bExit && pObject->bReady)
		{

			pObject_del = pObject;
			pObject = pObject->pNext;
			if (pObject_del->pPrev == NULL)
			{
				g_pThreadObjectList = pObject_del->pNext;
				if (g_pThreadObjectList != NULL)
				{
					g_pThreadObjectList->pPrev = NULL;
				}
			}
			else if (pObject_del->pNext == NULL)
			{
				pObject_del->pPrev->pNext = NULL;
			}
			else 
			{
				pObject_del->pPrev->pNext = pObject_del->pNext;
				pObject_del->pNext->pPrev = pObject_del->pPrev;
			}
			pObject_del->pNext = pObject_del_Head;
			pObject_del_Head = pObject_del;

			continue;
		}
		pObject = pObject->pNext;
	}

	Common_UnLock(g_hThreadObjectListLock);
	
	pThreadHandle->bExit = 1;

	pObject = pObject_del_Head;
	while(pObject != NULL)
	{
		pObject_del = pObject;
		pObject = pObject->pNext;
#if defined(WIN32) 
		CloseHandle(pObject_del->hWin32handle);
		pObject_del->hWin32handle = NULL;
		pObject_del->dwWin32ThreadID = -1;

#else

		

		pthread_join(pObject_del->hLinuxHandleID,NULL);	
		

		pObject_del->hLinuxHandleID = -1;
#endif
		Common_Free(pObject_del->szThreadName,__FUNCTION__,__LINE__);
		Common_Free(pObject_del,__FUNCTION__,__LINE__);

	}



	return 0;
}

S32 Common_Thread_Create(Common_Thread_T *pThreadHandle,const S8 *szThreadName,U32 uStackSize,S32 CreateFLAG,Common_Thread_def fxn,void *pUserData)
{
	Module_ThreadObject_T *pObject = NULL,*pCurrObject = NULL;
	int bDetach = 0;
	if (pThreadHandle == NULL)
	{
		return -1;
	}
	if (*pThreadHandle != NULL)
	{
		return -1;
	}
	if (g_hThreadObjectListLock == NULL)
	{
		Common_Lock_Create(&g_hThreadObjectListLock,"Common_ThreadCreateLock");
	}
	
	Common_Lock(g_hThreadObjectListLock);
	
	pObject = (Module_ThreadObject_T *)Common_Malloc(sizeof(Module_ThreadObject_T),0,__FUNCTION__,__LINE__) ;
	if (pObject == NULL)
	{
		
		Common_UnLock(g_hThreadObjectListLock);
		
		return -1;
	}
	memset(pObject,0,sizeof(Module_ThreadObject_T));
	pObject->hThreadHandle = (Common_Thread_T)pObject;
	if(szThreadName != NULL)
	{
		pObject->szThreadName = Common_StrDup((S8 *)szThreadName,__FUNCTION__,__LINE__);
	}
	pObject->fxn = fxn;
	pObject->pUserData = pUserData;
	if (CreateFLAG & (1 << 1))
	{
		bDetach = 1;
	}
	pObject->bDetach = bDetach;
	
#if defined(WIN32) 
	HANDLE threadhandle;
	S32 nThreadID = -1;
	threadhandle = CreateThread(NULL,0,(PTHREAD_START_ROUTINE)Win32_thread_Routine,pObject,0,(LPDWORD)&nThreadID);
	if (threadhandle == NULL)
	{
		if (pObject->szThreadName)
		{
			Common_Free(pObject->szThreadName,__FUNCTION__,__LINE__);
			pObject->szThreadName = NULL;
		}
		Common_Free(pObject,__FUNCTION__,__LINE__);
		Common_UnLock(g_hThreadObjectListLock);
		return -1;
	}
	pObject->hWin32handle = threadhandle;
	pObject->dwWin32ThreadID = nThreadID;
#else
	pthread_t nThreadID = -1;
	S32 status;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
#if 0
	if (bDetach)
	{
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	}
#endif
	
	status = pthread_create(&nThreadID,&attr,linux_thread_Routine,pObject);	
	pthread_attr_destroy(&attr);
	if (status != 0)
	{
		if (pObject->szThreadName)
		{
			Common_Free(pObject->szThreadName,__FUNCTION__,__LINE__);
			pObject->szThreadName = NULL;
		}
		Common_Free(pObject,__FUNCTION__,__LINE__);
		
		Common_UnLock(g_hThreadObjectListLock);
		
		return -1;
	}
	pObject->hLinuxHandleID = nThreadID;
#endif
	//if (!bDetach)
	{
		*pThreadHandle = pObject;
	}
	pCurrObject = pObject;
	
	pObject->pNext = g_pThreadObjectList;
	if (g_pThreadObjectList != NULL)
	{
		g_pThreadObjectList->pPrev = pObject;
	}
	
	g_pThreadObjectList = pObject;
	Module_ThreadObject_T *pObject_del = NULL,*pObject_del_Head = NULL;
	pObject = g_pThreadObjectList;
	while(pObject != NULL)
	{
		if (pObject->bDetach && pObject->bExit && pObject != pCurrObject && pObject->bReady)
		{
		
			pObject_del = pObject;
			pObject = pObject->pNext;
			if (pObject_del->pPrev == NULL)
			{
				g_pThreadObjectList = pObject_del->pNext;
				if (g_pThreadObjectList != NULL)
				{
					g_pThreadObjectList->pPrev = NULL;
				}
			}
			else if (pObject_del->pNext == NULL)
			{
				pObject_del->pPrev->pNext = NULL;
			}
			else 
			{
				pObject_del->pPrev->pNext = pObject_del->pNext;
				pObject_del->pNext->pPrev = pObject_del->pPrev;
			}

			pObject_del->pNext = pObject_del_Head;
			pObject_del_Head = pObject_del;

			continue;
		}
		pObject = pObject->pNext;
	}
	pCurrObject->bReady = 1;
	
	Common_UnLock(g_hThreadObjectListLock);
	
	pObject = pObject_del_Head;
	while(pObject != NULL)
	{
		pObject_del = pObject;
		pObject = pObject->pNext;
#if defined(WIN32) 
		CloseHandle(pObject_del->hWin32handle);
		pObject_del->hWin32handle = NULL;
		pObject_del->dwWin32ThreadID = -1;

#else

		

		pthread_join(pObject_del->hLinuxHandleID,NULL);	
		

		pObject_del->hLinuxHandleID = -1;
#endif
		Common_Free(pObject_del->szThreadName,__FUNCTION__,__LINE__);
		Common_Free(pObject_del,__FUNCTION__,__LINE__);

	}

	
	return 0; 
}
Common_Thread_T Common_Thread_Self()
{
	Common_Thread_T hThread = NULL;
	Module_ThreadObject_T *pObject = NULL;
#ifdef WIN32
	DWORD dwWin32ID = GetCurrentThreadId();
#else
	pthread_t tlinux = pthread_self();
#endif
	
	Common_Lock(g_hThreadObjectListLock);
	
	pObject = g_pThreadObjectList;
	while(pObject != NULL)
	{
		if ((!pObject->bExit) && (pObject->bReady))
		{
		
		
#ifdef WIN32
			if (pObject->dwWin32ThreadID == dwWin32ID)
#else
			if (pObject->hLinuxHandleID == tlinux)
#endif
			{
				hThread = pObject->hThreadHandle;
				break;
			}
		}
		pObject = pObject->pNext;
	}

	Common_UnLock(g_hThreadObjectListLock);
	
	return hThread;
}

S32 Common_Thread_Detach(Common_Thread_T hThreadHandle)
{
	Module_ThreadObject_T *pObject;
	if (hThreadHandle == NULL)
	{
		return -1;
	}
	
	Common_Lock(g_hThreadObjectListLock);
	
	pObject = g_pThreadObjectList;
	while(pObject != NULL)
	{
		if (pObject->hThreadHandle == hThreadHandle)
		{
			pObject->bDetach = 1;
			break;
		}
		pObject = pObject->pNext;
	}
	
	Common_UnLock(g_hThreadObjectListLock);
	
	return 0;
}
S32 Common_Thread_Yield(Common_Thread_T hThreadHandle)
{
	return 0;
}
S32 Common_Thread_Join(Common_Thread_T *pThreadHandle,S32 nRetValue)
{
	return 0;
}
S32 Common_Thread_Cancel(Common_Thread_T *pThreadHandle,S32 nRetValue)
{
	return 0;
}
S32 Common_Thread_Exit(Common_Thread_T *pThreadHandle,S32 nRetValue)
{
	return 0;
}
S32 Common_Thread_Destroy(Common_Thread_T *pThreadHandle)
{
	Module_ThreadObject_T *pObject;
	if (pThreadHandle == NULL || *pThreadHandle == NULL)
	{
		return -1;
	}
	
	Common_Lock(g_hThreadObjectListLock);

	pObject = g_pThreadObjectList;
	while(pObject != NULL)
	{
		if (pObject->hThreadHandle == *pThreadHandle)
		{
			break;
		}
		pObject = pObject->pNext;
	}
	if (pObject == NULL || pObject->bDetach)
	{
		Common_UnLock(g_hThreadObjectListLock);

		*pThreadHandle = NULL;
		return 0;
	}
	*pThreadHandle = NULL;
	if (pObject->hThreadHandle == Common_Thread_Self())
	{// 自己线程内调自己，则detach;
		Common_Thread_Detach(pObject->hThreadHandle);
		Common_UnLock(g_hThreadObjectListLock);
		return 0;
	}
#if defined(WIN32)
	HANDLE hWin32handle = pObject->hWin32handle;
	pObject->hWin32handle = NULL;
	pObject->dwWin32ThreadID = -1;
#else
	pthread_t hLinuxHandleID = pObject->hLinuxHandleID;
	pObject->hLinuxHandleID = -1;
#endif

	if (pObject->pPrev == NULL)
	{
		g_pThreadObjectList = pObject->pNext;
		if (g_pThreadObjectList != NULL)
		{
			g_pThreadObjectList->pPrev = NULL;
		}
	}
	else if (pObject->pNext == NULL)
	{
		pObject->pPrev->pNext = NULL;
	}
	else 
	{
		pObject->pPrev->pNext = pObject->pNext;
		pObject->pNext->pPrev = pObject->pPrev;
	}

	Common_UnLock(g_hThreadObjectListLock);

#if defined(WIN32) 
	WaitForSingleObject(hWin32handle,INFINITE);
	CloseHandle(hWin32handle);

#else
	pthread_join(hLinuxHandleID,NULL);	
#endif
	if (pObject->szThreadName)
	{
		Common_Free(pObject->szThreadName,__FUNCTION__,__LINE__);
		pObject->szThreadName = NULL;
	}
	Common_Free(pObject,__FUNCTION__,__LINE__);


	return 0;
}

S32 Common_Lock_Create(Common_Lock_T *pLock,const S8 *szLockName)
{
	Module_LockObject_T *pObject = NULL;
	if (pLock == NULL)
	{
		return -1;
	}
	if (*pLock != NULL)
	{
		return -1;
	}
	//pObject = new Module_LockObject_T;
	pObject = (Module_LockObject_T *)malloc(sizeof(Module_LockObject_T));
	if (pObject == NULL)
	{
		return -1;
	}
	memset(pObject,0,sizeof(Module_LockObject_T));
	pObject->hLockHandle = (Common_Lock_T)pObject;
	if(szLockName != NULL)
	{
		S8 szDes[128];
		sprintf(szDes,"%s_%s",__FUNCTION__,szLockName?szLockName:"");
		pObject->szLockName = Common_StrDup((S8 *)szLockName,szDes,__LINE__);
	}
#if defined(WIN32)
	
	pObject->hWin32Mutex = CreateMutex(NULL,FALSE,NULL);
	if (pObject->hWin32Mutex == NULL)
	{
		if (pObject->szLockName)
		{
			Common_Free(pObject->szLockName,pObject->szLockName,__LINE__);
			pObject->szLockName = NULL;
		}
		//delete pObject;
		free(pObject);
        pObject = NULL;
		return -1;
	}
#else
	
	pthread_mutexattr_t attr; 
	pthread_mutexattr_init(&attr); 
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&pObject->hLinux_Mutex,&attr);
	pthread_mutexattr_destroy(&attr);

	
#endif
	*pLock = pObject;
	return 0;
}
S32 Common_TryLock(Common_Lock_T hLock)
{
	S32 nRet = -1;
	Module_LockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_LockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(pObject->hWin32Mutex,0);
	if (dwRet == WAIT_OBJECT_0)
	{
		nRet = 0;
	}
	else if (dwRet == WAIT_TIMEOUT)
	{
		nRet = -2;
	}
	else if (dwRet == WAIT_ABANDONED)
	{
	}
	
#else	
	
		nRet = pthread_mutex_trylock(&pObject->hLinux_Mutex);
		
#endif
	return nRet;
}

S32 Common_Lock(Common_Lock_T hLock)
{
	S32 nRet = -1;
	Module_LockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_LockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(pObject->hWin32Mutex,INFINITE);
	if (dwRet == WAIT_OBJECT_0)
	{
		nRet = 0;
	}
	else if (dwRet == WAIT_TIMEOUT)
	{
		nRet = -2;
	}
	else if (dwRet == WAIT_ABANDONED)
	{
	}
#else		
	nRet = pthread_mutex_lock(&pObject->hLinux_Mutex);
#endif
return nRet;
}
S32 Common_UnLock(Common_Lock_T hLock)
{
	S32 nRet = -1;
	Module_LockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_LockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	if(ReleaseMutex(pObject->hWin32Mutex))
	{
		nRet = 0;
	}

#else		
	nRet = pthread_mutex_unlock(&pObject->hLinux_Mutex);
#endif
	return nRet;
}
S32 Common_Lock_Destroy(Common_Lock_T *pLock)
{
	Module_LockObject_T *pObject;
	if (pLock == NULL || *pLock == NULL)
	{
		return -1;
	}
	pObject = (Module_LockObject_T *)(*pLock);
	if (pObject->hLockHandle != *pLock)
	{
		return -1;
	}
	*pLock = NULL;
#ifdef WIN32
	CloseHandle(pObject->hWin32Mutex);
	pObject->hWin32Mutex = NULL;
#else
	pthread_mutex_destroy(&pObject->hLinux_Mutex);

#endif
	if (pObject->szLockName)
	{
		Common_Free(pObject->szLockName,pObject->szLockName,__LINE__);
		pObject->szLockName = NULL;
	}
	//delete pObject;
	free(pObject);
    pObject = NULL;
	return 0;
}




S32 Common_RWLock_Create(Common_RWLock_T *pLock,const S8 *szLockName)
{
	Module_RwLockObject_T *pObject = NULL;
	if (pLock == NULL)
	{
		return -1;
	}
	if (*pLock != NULL)
	{
		return -1;
	}
	//pObject = new Module_RwLockObject_T;
	pObject = (Module_RwLockObject_T *)malloc(sizeof(Module_RwLockObject_T));
	if (pObject == NULL)
	{
		return -1;
	}
	memset(pObject,0,sizeof(Module_RwLockObject_T));
	pObject->hLockHandle = (Common_RWLock_T)pObject;
	if(szLockName != NULL)
	{
		pObject->szLockName = Common_StrDup((S8 *)szLockName,__FUNCTION__,__LINE__);
	}
#if defined(WIN32)

	pObject->hWin32Mutex = CreateMutex(NULL,FALSE,NULL);
	if (pObject->hWin32Mutex == NULL)
	{
		if (pObject->szLockName)
		{
			Common_Free(pObject->szLockName,__FUNCTION__,__LINE__);
			pObject->szLockName = NULL;
		}
		//delete pObject;
		free(pObject);
        pObject = NULL;
		return -1;
	}
#else

	pthread_rwlockattr_t attr; 
	pthread_rwlockattr_init(&attr); 
	pthread_rwlockattr_setkind_np(&attr,PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(&pObject->hLinux_Mutex,&attr);
	pthread_rwlockattr_destroy(&attr);


#endif
	*pLock = pObject;
	return 0;
}
S32 Common_RWLock_TryWLock(Common_RWLock_T hLock)
{
	S32 nRet = -1;
	Module_RwLockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_RwLockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(pObject->hWin32Mutex,0);
	if (dwRet == WAIT_OBJECT_0)
	{
		nRet = 0;
	}
	else if (dwRet == WAIT_TIMEOUT)
	{
		nRet = -2;
	}
	else if (dwRet == WAIT_ABANDONED)
	{
	}

#else	

	nRet = pthread_rwlock_trywrlock(&pObject->hLinux_Mutex);

#endif
	return nRet;
}
S32 Common_RWLock_TryRLock(Common_RWLock_T hLock)
{
	S32 nRet = -1;
	Module_RwLockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_RwLockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(pObject->hWin32Mutex,0);
	if (dwRet == WAIT_OBJECT_0)
	{
		nRet = 0;
	}
	else if (dwRet == WAIT_TIMEOUT)
	{
		nRet = -2;
	}
	else if (dwRet == WAIT_ABANDONED)
	{
	}

#else	

	nRet = pthread_rwlock_tryrdlock(&pObject->hLinux_Mutex);

#endif
	return nRet;
}

S32 Common_RWLock_RLock(Common_RWLock_T hLock)
{
	S32 nRet = -1;
	Module_RwLockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_RwLockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(pObject->hWin32Mutex,INFINITE);
	if (dwRet == WAIT_OBJECT_0)
	{
		nRet = 0;
	}
	else if (dwRet == WAIT_TIMEOUT)
	{
		nRet = -2;
	}
	else if (dwRet == WAIT_ABANDONED)
	{
	}
#else		
	nRet = pthread_rwlock_rdlock(&pObject->hLinux_Mutex);
#endif
	return nRet;
}
S32 Common_RWLock_WLock(Common_RWLock_T hLock)
{
	S32 nRet = -1;
	Module_RwLockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_RwLockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	DWORD dwRet = WaitForSingleObject(pObject->hWin32Mutex,INFINITE);
	if (dwRet == WAIT_OBJECT_0)
	{
		nRet = 0;
	}
	else if (dwRet == WAIT_TIMEOUT)
	{
		nRet = -2;
	}
	else if (dwRet == WAIT_ABANDONED)
	{
	}
#else		
	nRet = pthread_rwlock_wrlock(&pObject->hLinux_Mutex);
#endif
	return nRet;
}
S32 Common_RWLock_UnLock(Common_RWLock_T hLock)
{
	S32 nRet = -1;
	Module_RwLockObject_T *pObject;
	if (hLock == NULL)
	{
		return -1;
	}
	pObject = (Module_RwLockObject_T *)hLock;
	if (pObject->hLockHandle != hLock)
	{
		return -1;
	}
#ifdef WIN32
	if(ReleaseMutex(pObject->hWin32Mutex))
	{
		nRet = 0;
	}

#else		
	nRet = pthread_rwlock_unlock(&pObject->hLinux_Mutex);
#endif
	return nRet;
}
S32 Common_RWLock_Destroy(Common_RWLock_T *pLock)
{
	Module_RwLockObject_T *pObject;
	if (pLock == NULL || *pLock == NULL)
	{
		return -1;
	}
	pObject = (Module_RwLockObject_T *)(*pLock);
	if (pObject->hLockHandle != *pLock)
	{
		return -1;
	}
	*pLock = NULL;
#ifdef WIN32
	CloseHandle(pObject->hWin32Mutex);
	pObject->hWin32Mutex = NULL;
#else
	pthread_rwlock_destroy(&pObject->hLinux_Mutex);

#endif
	if (pObject->szLockName)
	{
		Common_Free(pObject->szLockName,__FUNCTION__,__LINE__);
		pObject->szLockName = NULL;
	}
	//delete pObject;
	free(pObject);
    pObject = NULL;
	return 0;
}

typedef struct _tagCommon_SemObject 
{
	Common_Lock_T hLockHandle;
	S8 *szLockName;
	//////////////////////////////////////////////////////////////////////////
#ifndef WIN32
	sem_t   sem;
#endif
	void *hSem;

}Common_SemObject_T;


S32 Common_Sem_Create(Common_Sem_T *phSem,S32 nInitCount,S32 nMaxCount,const S8 *szSemName)
{
	Common_SemObject_T *pObject;
	if (phSem == NULL)
	{
		return -1;
	}
	pObject = (Common_SemObject_T *)Common_Malloc(sizeof(Common_SemObject_T),0,__FUNCTION__,__LINE__);
	if (pObject == NULL)
	{
		return -1;
	}
	memset(pObject,0,sizeof(Common_SemObject_T));
#ifndef WIN32
	sem_init(&pObject->sem, 0, nInitCount);
	pObject->hSem = &pObject->sem;
#else
	pObject->hSem = CreateSemaphore(NULL,nInitCount,nMaxCount,(LPCSTR)szSemName);
	if (pObject->hSem == NULL)
	{
		Common_Free(pObject->hSem,__FUNCTION__,__LINE__);
		return -1;
	}
#endif
	*phSem = (Common_Sem_T *)pObject;
	return 0;
}
S32 Common_Sem_Post(Common_Sem_T hSem)
{
	Common_SemObject_T *pObject;
	if (hSem == NULL)
	{
		return -1;
	}
	pObject = (Common_SemObject_T *)(hSem);
#ifndef WIN32
	sem_post(&pObject->sem);

#else
	if(!ReleaseSemaphore((HANDLE)pObject->hSem,1,NULL))
	{
		return -1;
	}
#endif
	return 0;
}
S32 Common_Sem_Pend(Common_Sem_T hSem)
{
	Common_SemObject_T *pObject;
	if (hSem == NULL)
	{
		return -1;
	}
	pObject = (Common_SemObject_T *)(hSem);
#ifndef WIN32
	sem_wait(&pObject->sem);
#else
	DWORD Ret;
	Ret = WaitForSingleObject((HANDLE)pObject->hSem,	INFINITE);
	if(Ret == WAIT_OBJECT_0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
#endif
	return 0;
}
S32 Common_Sem_TryPend(Common_Sem_T hSem,U32 nMSec)
{

	Common_SemObject_T *pObject;
	if (hSem == NULL)
	{
		return -1;
	}
	pObject = (Common_SemObject_T *)(hSem);
#ifndef WIN32
	struct timespec ts;
	S64 llStartMSec,llCurrMSec;
	clock_gettime(CLOCK_MONOTONIC,&ts);
	llStartMSec = ts.tv_sec;
	llStartMSec = llStartMSec * 1000 + ts.tv_nsec/1000000;

	do{
		if(!sem_trywait(&pObject->sem))
		{
			//成功
			return 0;
		}
		clock_gettime(CLOCK_MONOTONIC,&ts);
		llCurrMSec = ts.tv_sec;
		llCurrMSec = llCurrMSec * 1000 + ts.tv_nsec/1000000;
		if (llCurrMSec - llStartMSec >= nMSec)
		{
			break;
		}
		struct timespec req,rem;
		req.tv_sec = 0;
		req.tv_nsec = 10000000;
		nanosleep(&req,&rem);
	}while(1);
	return COMMON_ERROR_TYPE_TIMEOUT;

#else
	DWORD Ret;
	Ret = WaitForSingleObject((HANDLE)pObject->hSem,	nMSec);
	if(Ret == WAIT_OBJECT_0)
	{
		return 0;
	}
	else if (Ret == WAIT_TIMEOUT)
	{
		return COMMON_ERROR_TYPE_TIMEOUT;
	}
	else
	{
		return -1;
	}
#endif
	return -1;
}
S32 Common_Sem_Destroy(Common_Sem_T *phSem)
{
	Common_SemObject_T *pObject;
	if (phSem == NULL || *phSem == NULL)
	{
		return -1;
	}
	pObject = (Common_SemObject_T *)(*phSem);
#ifndef WIN32
	sem_destroy(&pObject->sem);
#else
	CloseHandle(pObject->hSem);
#endif
	Common_Free(pObject,__FUNCTION__,__LINE__);
	*phSem = NULL;
	return 0;
}

typedef struct _tagModule_CondObject 
{
	Common_Cond_T hCondHandle;
	S8 *szLockName;
	Common_Lock_T hLock;
	//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#else
	pthread_cond_t hLinuxCond;
#endif
}Module_CondObject_T;

S32 Common_Cond_Create(Common_Cond_T *pCond,const S8 *szCondName)
{
	Module_CondObject_T *pObject;
	if (pCond == NULL)
	{
		return -1;
	}
	pObject = (Module_CondObject_T *)Common_Malloc(sizeof(Module_CondObject_T),0,__FUNCTION__,__LINE__);
	if (pObject == NULL)
	{
		return -1;
	}
	memset(pObject,0,sizeof(Module_CondObject_T));
#ifndef WIN32
	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);
	pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
	pthread_cond_init(&pObject->hLinuxCond, &condattr);
	pthread_condattr_destroy(&condattr);
#else
	
#endif
	*pCond = (Module_CondObject_T *)pObject;
	return 0;

}
S32 Common_Cond_TryWait(Common_Cond_T hCond,Common_Lock_T hLock,U32 nMSec)
{
	S32 nRet = -1;
	Module_LockObject_T *pLock = (Module_LockObject_T *)hLock;
	Module_CondObject_T *pObject = (Module_CondObject_T *)hCond;
#ifdef WIN32
#else
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	tv.tv_sec = tv.tv_sec + nMSec / 1000;
	tv.tv_nsec = tv.tv_nsec + (nMSec % 1000) * 1000L * 1000L;
	if (tv.tv_nsec > 1000000000L)
	{
		tv.tv_nsec = tv.tv_nsec % 1000000000L;
		tv.tv_sec++;
	}
	nRet = pthread_cond_timedwait(&pObject->hLinuxCond,&pLock->hLinux_Mutex,&tv);
#endif
	return nRet;
}
S32 Common_Cond_Wait(Common_Cond_T hCond,Common_Lock_T hLock)
{
	S32 nRet = -1;
	Module_LockObject_T *pLock = (Module_LockObject_T *)hLock;
	Module_CondObject_T *pObject = (Module_CondObject_T *)hCond;
#ifdef WIN32
#else
	nRet = pthread_cond_wait(&pObject->hLinuxCond,&pLock->hLinux_Mutex);
#endif
	return nRet;
}
S32 Common_Cond_Broadcast(Common_Cond_T hCond)
{
	S32 nRet = -1;
	Module_CondObject_T *pObject = (Module_CondObject_T *)hCond;
#ifdef WIN32
#else
	nRet = pthread_cond_broadcast(&pObject->hLinuxCond);
#endif
	return nRet;
}
S32 Common_Cond_Signal(Common_Cond_T hCond)
{
	S32 nRet = -1;
	Module_CondObject_T *pObject = (Module_CondObject_T *)hCond;
#ifdef WIN32
#else
	nRet = pthread_cond_signal(&pObject->hLinuxCond);
#endif
	return nRet;
}
S32 Common_Cond_Destroy(Common_Cond_T *pCond)
{
	Module_CondObject_T *pObject;
	if (pCond == NULL || *pCond == NULL)
	{
		return -1;
	}
	pObject = (Module_CondObject_T *)(*pCond);
#ifdef WIN32
#else
	pthread_cond_destroy(&pObject->hLinuxCond);
#endif
	Common_Free(pObject,__FUNCTION__,__LINE__);
	*pCond = NULL;
	return 0;
}