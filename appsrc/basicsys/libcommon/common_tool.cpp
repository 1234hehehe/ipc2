#include "libcommon_struct.h"
#include "libcommon_api.h"

typedef struct _tagMemoryNode
{
	void *pPtr;
	U32 nSize;
	struct _tagMemoryNode *pNext;
	struct _tagMemoryNode *pPrev;
}MemoryNode_T;
typedef struct _tagMemoryMgr
{
	S8 *pDes;
	S32 nLine;
	S64 nSize;
	S64 nMallocCount;
	S64 nFreeCount;
	U32 nException;// 0- normal,> 0异常次数
	MemoryNode_T *pNodeHead;
	S64 nNodeCount;
	struct _tagMemoryMgr *pNext;
	struct _tagMemoryMgr *pPrev;
}MemoryMgr_T;

typedef struct _tagMemoryExceptNode
{
	void *pPtr;
	S8 *pDes;
	S32 nLine;
	struct _tagMemoryExceptNode *pNext;
	struct _tagMemoryExceptNode *pPrev;
}MemoryExceptNode_T;
static Common_Lock_T g_hMemoryLock = NULL;
static MemoryMgr_T *g_pMemoryHead = NULL;
static MemoryExceptNode_T *g_pMemoryFreeException = NULL;
static S32 g_bMemoryDebug = 0;
static Common_Thread_T g_hMemoryThread = NULL;
static S32 g_bMemoryThreadExit = 0;
static S32    g_nMemoryIntervalSec = 60;
static S8     *g_szMemoryFilePath = NULL;


static void *(*g_fMalloc_fn)(U32 sz) = NULL;
static void (*g_fFree_fn)(void *ptr) = NULL;
#ifdef WIN32
void Common_Trace_Return(Common_Trace_T *lpTrace,S32 nRetIdx,S8 **szFName,S8 **szSName,U32 *lpSAddr)
{
	return;
}
void Common_Trace_RetFree(Common_Trace_T *lpTrace)
{
	return;
}
#endif

static char * strncpy_Prt(char *szDst,const char *szSrc,size_t nDstSize,size_t nSrcSize)
{
	size_t i = 0,j = 0;
	char c,c_last = 0;
	for (i = 0; i < nSrcSize && j < nDstSize; i++)
	{
		c = szSrc[i];
		if (c == '\r' || c == '\n' || c == '\t')
		{
			c =' ';
		}
		else if (c == 0 || c < 32 || c > 126)
		{
			break;
		}
		if (c == ' ' && (c_last == ' ' || c_last == ';' || c_last == ':' || c_last == ',' || c_last == '{' || c_last == '}'||c_last == '\"'))
		{

			continue;
		}

		c_last = c;
		szDst[j] = c;
		j++;
	}
	if (j < nDstSize)
	{
		szDst[j] = 0;
	}


	return szDst;
}
void Common_InitHooks(void *(*malloc_fn)(U32 sz),void (*free_fn)(void *ptr))
{
	g_fMalloc_fn = malloc_fn;
	g_fFree_fn = free_fn;
}
#if 0//def _DEBUG
extern "C" void * memset( void * _Dst, int _Val,size_t _Size)
{
	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	// check
	S8 *pDst = (S8 *)_Dst;
	S32 i;
	Common_Lock(g_hMemoryLock);
	if (g_bMemoryDebug)
	{
		pMgr = g_pMemoryHead;
		while(pMgr != NULL)
		{
			
				pNode = pMgr->pNodeHead;
				while(pNode != NULL)
				{
					if ((pNode->pPtr > pDst && pDst + _Size > pNode->pPtr) ||
						(pDst < (S8 *)pNode->pPtr + pNode->nSize && pDst + _Size > (S8 *)pNode->pPtr + pNode->nSize))
					{// 异常
						S8 szJsonBuff[65];
						szJsonBuff[0] = 0;
						strncpy_Prt(szJsonBuff,(S8 *)pNode->pPtr,64,pNode->nSize);
						printf("memset!!!!!!!!!!!!!!!!!!!!!!!!!!!<%p-%d> <%p,%d>$$$$$$$$$$$$$$$$$$$$$$$$$$$$<%s>\n",pNode->pPtr,pNode->nSize,pDst,_Size,szJsonBuff);
						break;
					}
					pNode = pNode->pNext;
				}
			

			pMgr = pMgr->pNext;
		}
	
	}
	Common_UnLock(g_hMemoryLock);
	for (i = 0; i < _Size;i++)
	{
		pDst[i] = _Val;
	}
	return _Dst;

}
extern "C" void *  memcpy(void * _Dst, const void * _Src, size_t _Size)
{
	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	// check
	S8 *pDst = (S8 *)_Dst;
	S8 *pSrc = (S8 *)_Src;
	S32 i;
	Common_Lock(g_hMemoryLock);
	if (g_bMemoryDebug)
	{
		pMgr = g_pMemoryHead;
		while(pMgr != NULL)
		{

			pNode = pMgr->pNodeHead;
			while(pNode != NULL)
			{
				if ((pNode->pPtr > pDst && pDst + _Size > pNode->pPtr) ||
					(pDst < (S8 *)pNode->pPtr + pNode->nSize && pDst + _Size > (S8 *)pNode->pPtr + pNode->nSize))
				{// 异常
					S8 szJsonBuff[65];
					szJsonBuff[0] = 0;
					strncpy_Prt(szJsonBuff,(S8 *)pNode->pPtr,64,pNode->nSize);
					printf("memcpy!!!!!!!!!!!!!!!!!!!!!!!!!!!<%p-%d> <%p,%d>$$$$$$$$$$$$$$$$$$$$$$$$$$$$<%s>\n",pNode->pPtr,pNode->nSize,pDst,_Size,szJsonBuff);
					break;
				}
				pNode = pNode->pNext;
			}


			pMgr = pMgr->pNext;
		}

	}
	Common_UnLock(g_hMemoryLock);
	for (i = 0; i < _Size;i++)
	{
		pDst[i] = pSrc[i];
	}
	return _Dst;

}
extern "C" char * strcpy(char *pDst,const char * pSrc)
{
	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	// check

	S32 i;
	S32 _Size = strlen(pSrc) + 1;
	
	Common_Lock(g_hMemoryLock);
	if (g_bMemoryDebug)
	{
		pMgr = g_pMemoryHead;
		while(pMgr != NULL)
		{

			pNode = pMgr->pNodeHead;
			while(pNode != NULL)
			{
				if ((pNode->pPtr > pDst && pDst + _Size > pNode->pPtr) ||
					(pDst < (S8 *)pNode->pPtr + pNode->nSize && pDst + _Size > (S8 *)pNode->pPtr + pNode->nSize))
				{// 异常
					S8 szJsonBuff[65];
					szJsonBuff[0] = 0;
					strncpy_Prt(szJsonBuff,(S8 *)pNode->pPtr,64,pNode->nSize);
					printf("strcpy!!!!!!!!!!!!!!!!!!!!!!!!!!!<%p-%d> <%p,%d>$$$$$$$$$$$$$$$$$$$$$$$$$$$$<%s>\n",pNode->pPtr,pNode->nSize,pDst,_Size,szJsonBuff);
					break;
				}
				pNode = pNode->pNext;
			}


			pMgr = pMgr->pNext;
		}

	}
	Common_UnLock(g_hMemoryLock);
	for (i = 0; i < _Size;i++)
	{
		pDst[i] = pSrc[i];
	}
	return pDst;
}
#endif

static void static_addMemory(void *p,S32 nSize,S8 *pDes,S32 nLine)
{
	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	S32 bFound = 0;
	if (g_hMemoryLock == NULL)
	{
		Common_Lock_Create(&g_hMemoryLock,"CommonMemoryMgr");
	}
	Common_Lock(g_hMemoryLock);
	if (!g_bMemoryDebug)
	{
		Common_UnLock(g_hMemoryLock);
		return;
	}
	// 是否查找内存是否分配
	pMgr = g_pMemoryHead;
	while(pMgr != NULL)
	{
		if (pMgr->nLine == nLine)
		{
			if (pDes == NULL && pMgr->pDes == NULL)
			{// ok found
				bFound = 1;
			}
			else if (pDes == NULL || pMgr->pDes == NULL)
			{
			}
			else if ( 0 == Common_StrCmp(pDes,pMgr->pDes))
			{// ok found
				bFound = 1;
			}
		}
		if (bFound)
		{
			pNode = pMgr->pNodeHead;
			while(pNode != NULL)
			{
				if ((pNode->pPtr >= p && pNode->pPtr < (S8 *)p + nSize) ||
					(pNode->pPtr <= p && p < (S8 *)pNode->pPtr + pNode->nSize))
				{// 异常
					pMgr->nException++;
					// bException = 1;
					break;
				}
				pNode = pNode->pNext;
			}
			break;
		}
		
		pMgr = pMgr->pNext;
	}
	// 新内存
	
	pNode = (MemoryNode_T *)malloc(sizeof(MemoryNode_T));
	if (pNode != NULL)
	{
		memset(pNode,0,sizeof(MemoryNode_T));
		pNode->pPtr = p;
		pNode->nSize = nSize;
	}
	if (pMgr == NULL)
	{
		pMgr = (MemoryMgr_T *)malloc(sizeof(MemoryMgr_T));
		if (pMgr != NULL)
		{
			memset(pMgr,0,sizeof(MemoryMgr_T));
			if (pDes != NULL)
			{
				int nlen = strlen(pDes);
				if (nlen > 64)
				{
					nlen = 64;
				}
				if (nlen > 0)
				{
					pMgr->pDes = (S8 *)malloc(nlen + 1);
					if (pMgr->pDes != NULL)
					{
						strncpy(pMgr->pDes,pDes,nlen);
						pMgr->pDes[nlen] = 0;
					}
				}
				// pMgr->pDes = strdup(pDes);
			}
			pMgr->nLine = nLine;
			pMgr->nMallocCount++;
			pMgr->nSize += nSize;
			pMgr->pNodeHead = pNode;
			pMgr->nNodeCount++;
			pMgr->pNext = g_pMemoryHead;
			if (g_pMemoryHead != NULL)
			{
				g_pMemoryHead->pPrev = pMgr;
			}
			g_pMemoryHead = pMgr;
			
		}
	}
	else
	{
		pMgr->nMallocCount++;
		pMgr->nSize += nSize;
		pNode->pNext = pMgr->pNodeHead;
		
		if (pMgr->pNodeHead != NULL)
		{
			pMgr->pNodeHead->pPrev = pNode;
		}
		pMgr->pNodeHead = pNode;
		pMgr->nNodeCount++;
	}
	Common_UnLock(g_hMemoryLock);
}



static void static_freeMemory(void *p,S8 *pDes,S32 nLine)
{

	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	MemoryExceptNode_T *pFreeNode = NULL;
	int bFound = 0;
	if (g_hMemoryLock == NULL)
	{
		Common_Lock_Create(&g_hMemoryLock,"CommonMemoryMgr");
	}
	Common_Lock(g_hMemoryLock);
	if (!g_bMemoryDebug)
	{
		Common_UnLock(g_hMemoryLock);
		return;
	}
	// 是否查找内存是否分配
	pMgr = g_pMemoryHead;
	while(pMgr != NULL)
	{
		
		pNode = pMgr->pNodeHead;
		while(pNode != NULL)
		{
			if (p == pNode->pPtr)
			{
				bFound = 1;
				break;
			}
			pNode = pNode->pNext;
		}
		if (bFound)
		{
			break;
		}
		
		pMgr = pMgr->pNext;
	}
	if (bFound)
	{
		pMgr->nFreeCount++;
		pMgr->nSize -= pNode->nSize;
		if (pNode->pPrev == NULL)
		{
			pMgr->pNodeHead = pNode->pNext;
			if (pMgr->pNodeHead != NULL)
			{
				pMgr->pNodeHead->pPrev = NULL;
			}
		}
		else 
		{
			pNode->pPrev->pNext = pNode->pNext;
		}
		if (pNode->pNext != NULL)
		{
			pNode->pNext->pPrev = pNode->pPrev;
		}
		pMgr->nNodeCount--;
		free(pNode);
		if (pMgr->nNodeCount <= 0)
		{
			if (pMgr->pDes != NULL)
			{
				free(pMgr->pDes);
			}
			if (pMgr->pPrev == NULL)
			{
				g_pMemoryHead = pMgr->pNext;
				if (g_pMemoryHead != NULL)
				{
					g_pMemoryHead->pPrev = NULL;
				}
			}
			else
			{
				pMgr->pPrev->pNext = pMgr->pNext;
			}
			if (pMgr->pNext != NULL)
			{
				pMgr->pNext->pPrev = pMgr->pPrev;
			}
			free(pMgr);
			
		}
	}
	else
	{
		pFreeNode = (MemoryExceptNode_T *)malloc(sizeof(MemoryExceptNode_T));
		if (pFreeNode != NULL)
		{
			memset(pFreeNode,0,sizeof(MemoryExceptNode_T));
			if (pDes != NULL)
			{
				pFreeNode->pDes = strdup(pDes);
			}
			pFreeNode->nLine = nLine;
			pFreeNode->pPtr = p;
			pFreeNode->pNext = g_pMemoryFreeException;
			if (g_pMemoryFreeException != NULL)
			{
				g_pMemoryFreeException->pPrev = pFreeNode;
			}
			g_pMemoryFreeException = pFreeNode;
			
		}
	}
	Common_UnLock(g_hMemoryLock);
}

static S32 static_Memory_Thread_fxn(Common_Thread_T hThreadHandle,void *pUserData)
{
	S32 bPrint = 0;
	S32 nCount = 0,nIntervalSec = 0;
	S8 *szFilePath = NULL;
	while(!g_bMemoryThreadExit)
	{
		Common_Lock(g_hMemoryLock);
		if (nIntervalSec != g_nMemoryIntervalSec)
		{
			nIntervalSec = g_nMemoryIntervalSec;
			nCount = 0;
		}
		if (g_szMemoryFilePath == NULL && szFilePath == NULL)
		{
		}
		else if (g_szMemoryFilePath == NULL || szFilePath == NULL)
		{
			free(szFilePath);
			szFilePath = NULL;
			if (g_szMemoryFilePath != NULL)
			{
				szFilePath = strdup(g_szMemoryFilePath);
			}
		}
		else if (0 != strcmp(g_szMemoryFilePath,szFilePath))
		{
			free(szFilePath);
			szFilePath = strdup(g_szMemoryFilePath);
		}
		Common_UnLock(g_hMemoryLock);
		if (nCount <= 0)
		{
			nCount = nIntervalSec;
			bPrint = 1;
		}
		if (bPrint)
		{
			bPrint = 0;
			Common_Memory_Print(szFilePath);
		}
		nCount--;
		
		Common_Sleep(1,0);

	}
	return 0;
}
S32 Common_Memory_IsDebugging()
{
	return g_bMemoryDebug;
}

void Common_Memory_StartDebug(S32 bAutoPrint,S32 nIntervalSec,S8 *pFilePath)
{
	if (g_hMemoryLock == NULL)
	{
		Common_Lock_Create(&g_hMemoryLock,"CommonMemoryMgr");
	}

	Common_Lock(g_hMemoryLock);
	g_bMemoryDebug = 1;
	if (bAutoPrint)
	{
		if (nIntervalSec < 3)
		{
			nIntervalSec = 3;
		}
		if (nIntervalSec > 30 * 24 * 60 * 60)
		{
			nIntervalSec = 1 * 60 * 60;
		}
		g_nMemoryIntervalSec = nIntervalSec;
		if (pFilePath == NULL)
		{
			if (g_szMemoryFilePath != NULL)
			{
				free(g_szMemoryFilePath);
				g_szMemoryFilePath = NULL;
			}
		}
		else
		{
			if (g_szMemoryFilePath != NULL)
			{
				if(0 != strcmp(g_szMemoryFilePath,pFilePath))
				{
					free(g_szMemoryFilePath);
					g_szMemoryFilePath = NULL;
					g_szMemoryFilePath = strdup(pFilePath);
				}
			}
			else
			{
				g_szMemoryFilePath = strdup(pFilePath);
			}
		}
		if (g_hMemoryThread == NULL)
		{// 开线程
			g_bMemoryThreadExit = 0;
			Common_Thread_Create(&g_hMemoryThread,"MemoryDebug",0,0,static_Memory_Thread_fxn,NULL);
		}
	}
	else
	{
		if (g_hMemoryThread != NULL)
		{
			// 关线程
			g_bMemoryThreadExit = 1;
			Common_Thread_Detach(g_hMemoryThread);
			Common_Thread_Destroy(&g_hMemoryThread);
		}
	}
	Common_UnLock(g_hMemoryLock);
}
void Common_Memory_StopDebug()
{
	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	MemoryExceptNode_T *pFreeNode = NULL;
	void *pDelete = NULL;
	if (g_hMemoryLock == NULL)
	{
		Common_Lock_Create(&g_hMemoryLock,"CommonMemoryMgr");
	}
	
	
	Common_Lock(g_hMemoryLock);
	g_bMemoryDebug = 0;
	g_bMemoryThreadExit = 1;
	Common_Thread_Detach(g_hMemoryThread);
	Common_Thread_Destroy(&g_hMemoryThread);
	// 释放内存
	pMgr = g_pMemoryHead;
	while(pMgr != NULL)
	{
		if (pMgr->pDes != NULL)
		{
			free(pMgr->pDes);
			pMgr->pDes = NULL;
		}
		pNode = pMgr->pNodeHead;
		while(pNode != NULL)
		{
			pDelete = pNode;
			pNode = pNode->pNext;
			free(pDelete);
		}
		pMgr->pNodeHead = NULL;
		pDelete = pMgr;
		pMgr = pMgr->pNext;
		free(pDelete);
	}
	g_pMemoryHead = NULL;

	pFreeNode = g_pMemoryFreeException;
	while(pFreeNode != NULL)
	{
		if (pFreeNode->pDes != NULL)
		{
			free(pFreeNode->pDes);
			pFreeNode->pDes = NULL;
		}
		
		pDelete = pFreeNode;
		pFreeNode = pFreeNode->pNext;
		free(pDelete);
	}
	g_pMemoryFreeException = NULL;
	Common_UnLock(g_hMemoryLock);
}


void Common_Memory_Print(S8 *pFilePath)
{
	MemoryMgr_T *pMgr = NULL;
	MemoryNode_T *pNode = NULL;
	MemoryExceptNode_T *pFreeNode = NULL;
	FILE *pf = NULL,*pOut = NULL;
	int nCount = 0,nTotalMem = 0;
	S8 szJsonBuff[65];
	if (g_hMemoryLock == NULL)
	{
		Common_Lock_Create(&g_hMemoryLock,"CommonMemoryMgr");
	}
	szJsonBuff[0] = 0;
	szJsonBuff[64] = 0;
	

	Common_Lock(g_hMemoryLock);
	if (pFilePath != NULL)
	{
		pf = fopen(pFilePath,"w+");
		pOut = pf;
	}
	else if (g_szMemoryFilePath != NULL)
	{
		pf = fopen(g_szMemoryFilePath,"w+");
		pOut = pf;
	}
	if (pOut == NULL)
	{
		pOut = stdout;
	}
	if (pOut == NULL)
	{
		Common_UnLock(g_hMemoryLock);
		return;
	}
	// 是否查找内存是否分配
	pMgr = g_pMemoryHead;
	while(pMgr != NULL)
	{
		nCount = 0;
/*		bJsonMalloc = 0; 
		if (pMgr->pDes != NULL&& 
			(0 == strcmp(pMgr->pDes,"Common_cJSON_malloc") || 
			 0 == strcmp(pMgr->pDes,"Common_cJSON_strdup")))
		{
			bJsonMalloc = 1;
		}
*/
		fprintf(pOut,"\n[%s.%d]>Malloc[%lld] Free[%lld] Size[%lld] Exception[%u] Hold[%lld]:\n",pMgr->pDes?pMgr->pDes:"null",pMgr->nLine,pMgr->nMallocCount,pMgr->nFreeCount,pMgr->nSize,pMgr->nException,pMgr->nNodeCount);
		pNode = pMgr->pNodeHead;
		while(pNode != NULL)
		{
			if (nCount == 0)
			{
				fprintf(pOut,"\t");
			}
			nCount++;
			//if (bJsonMalloc)
			{
				strncpy_Prt(szJsonBuff,(S8 *)pNode->pPtr,64,pNode->nSize);
			}
			fprintf(pOut,"[%p,%08x:%s]",pNode->pPtr,pNode->nSize,szJsonBuff);
			if (nCount > 16)
			{
				nCount = 0;
				fprintf(pOut,"\n");
			}
			pNode = pNode->pNext;
		}
		nTotalMem += pMgr->nSize;

		pMgr = pMgr->pNext;
	}
	// free exception
	pFreeNode = g_pMemoryFreeException;
	if (pFreeNode != NULL)
	{
		fprintf(pOut,"\nFree Memory Exception:\n");

	}
	nCount = 0;

	while(pFreeNode != NULL)
	{

		if (nCount == 0)
		{
			fprintf(pOut,"\t");
		}
		nCount++;
		fprintf(pOut,"[%s.%d.%p]",pFreeNode->pDes?pFreeNode->pDes:"null",pFreeNode->nLine,pFreeNode->pPtr);
		if (nCount > 16)
		{
			nCount = 0;
			fprintf(pOut,"\n");
		}

		pFreeNode = pFreeNode->pNext;
	}
	Common_UnLock(g_hMemoryLock);

	fprintf(pOut,"\nTotal Mem Size:%d\n",nTotalMem);
	if (pf != NULL)
	{
		fclose(pf);
	}
}


U32 Common_Rand32()
{
	S32 nSec,nMsec;
	U64 msec;
	Common_GetSystemCount(&nSec,&nMsec);
	msec = nSec * 1000 + nMsec;
#ifdef WIN32
	return (U32) (rand() + msec);
#else
	return (U32) (random() + msec);
#endif
}

U16 Common_Rand16()
{
	S32 nSec,nMsec;
	U64 msec;
	Common_GetSystemCount(&nSec,&nMsec);
	msec = nSec * 1000 + nMsec;
#ifdef WIN32
	return (U16) (rand() + msec);
#else
	return (U16) (random() + msec);
#endif
}



void * Common_Malloc(S32 nSize,S32 nAlignBytes,const S8 *pDes,S32 nLine)
{
	void *p = NULL;
	if (nSize <= 0)
	{
		return NULL;
	}

	// nSize = ((nSize + 1)/2) * 2;
	if (g_fMalloc_fn != NULL)
	{
		p = g_fMalloc_fn(nSize);
	}
	else
	{
		p = malloc(nSize);
	}
	if (p != NULL && nSize > 0)
	{
		((U8 *)p)[0] = 0;
	}
	if (p != NULL && g_bMemoryDebug)
	{
		static_addMemory(p,nSize,(S8 *)pDes,nLine);
	}
	return p;
}

void * Common_Calloc(size_t n, size_t size,const S8 *pDes,S32 nLine)
{
	void *p = NULL;
	S32 nSize = n * size;
	if (nSize <= 0)
	{
		return NULL;
	}
	// nSize = ((nSize + 1)/2) * 2;
	if (g_fMalloc_fn != NULL)
	{
		p = g_fMalloc_fn(nSize);
	}
	else
	{
		p = malloc(nSize);
	}
	if (p != NULL)
	{
		memset(p,0,nSize);
	}
	if (p != NULL && g_bMemoryDebug)
	{
		static_addMemory(p,nSize,(S8 *)pDes,nLine);
	}
	return p;
	
}

void *Common_Realloc(void *mem_address, size_t newsize,const S8 *pDes,S32 nLine)
{
    void *p = NULL;
    // int nsize = ((newsize + 1)/2) * 2;
    p = realloc(mem_address,newsize);
    if (p != NULL && g_bMemoryDebug)
    {

        static_freeMemory(mem_address,(S8 *)pDes,nLine);

        static_addMemory(p,newsize,(S8 *)pDes,nLine);
    }
    return p;
}


S8*  Common_StrDup(S8 *pString,const S8 *pDes,S32 nLine)
{
    size_t len;
    S8* copy = NULL;
    if (pString == NULL)
    {
        return NULL;
    }
    // len = ((strlen(pString) + 1 + 2)/2)*2;
    len = strlen(pString) + 1;
    copy = (S8*)calloc(1, len);
    if (copy != NULL)
    {
        memcpy(copy,pString,len);
    }
    if (copy != NULL && g_bMemoryDebug)
    {
        static_addMemory(copy,len,(S8 *)pDes,nLine);
    }
	
    return copy;
}

S8  *Common_StrnDup(S8 *pString,S32 nMaxLen,const S8 *pDes,S32 nLine)
{
    size_t len;
    S8* copy = NULL;
    if (pString == NULL)
    {
        return NULL;
    }
    len = strlen(pString) + 1;
    if ((int)len > nMaxLen + 1)
    {
        len = nMaxLen + 1;
    }
    copy = (S8*)malloc(len);
    if (copy != NULL)
    {
        memcpy(copy,pString,len);
        copy[len - 1] = 0;
    }
    if (copy != NULL && g_bMemoryDebug)
    {
        static_addMemory(copy,len,(S8 *)pDes,nLine);
    }

    return copy;
}
void  Common_Free(void *pMem,const S8 *pDes,S32 nLine)
{
	if (pMem == NULL)
	{
		return;
	}
	if (g_bMemoryDebug)
	{
		static_freeMemory(pMem,(S8 *)pDes,nLine);
	}
	if (g_fFree_fn != NULL)
	{
		g_fFree_fn(pMem);
	}
	else
	{
		free(pMem);
	}
	
	return;
}


void *Common_Copy(void *pDst,void *pSrc,S32 nByteSize)
{
	if (pDst == NULL && pSrc == NULL)
	{
		return NULL;
	}
	else if (pDst == NULL || pSrc == NULL)
	{
		return NULL;
	}
	return memcpy(pDst,pSrc,nByteSize);
}
 S8* Common_Strcpy(S8 *pDst,S8 *pSrc)
 {
	 if (pDst == NULL && pSrc == NULL)
	 {
		 return NULL;
	 }
	 else if (pDst == NULL || pSrc == NULL)
	 {
		 return NULL;
	 }
	 return strcpy(pDst,pSrc);
 }
 void Common_Strncpy(S8 *pDst,S8 *pSrc,S32 n)
 {
	 if (pDst == NULL && pSrc == NULL)
	 {
		 return;
	 }
	 else if (pDst == NULL || pSrc == NULL)
	 {
		 return;
	 }
	 S32 nSrcLen;
	 nSrcLen = strlen(pSrc);
	 if (nSrcLen + 1 < n)
	 {
		 n = nSrcLen + 1;
	 }
	 memcpy(pDst,pSrc,n);
	 return ;
 }
 S32 Common_StrCmp(S8 *pDst,S8 *pSrc)
 {
	 if (pDst == NULL && pSrc == NULL)
	 {
		 return 0;
	 }
	 else if (pDst == NULL || pSrc == NULL)
	 {
		 return -1;
	 }
	 return strcmp(pDst,pSrc);
 }
 S32 Common_StriCmp(S8 *pDst,S8 *pSrc)
 {
	 if (pDst == NULL && pSrc == NULL)
	 {
		 return 0;
	 }
	 else if (pDst == NULL || pSrc == NULL)
	 {
		 return -1;
	 }
	 return stricmp(pDst,pSrc);
 }

 S32 Common_StrnCmp(S8 *pDst,S8 *pSrc,S32 nCount)
 {
	 if (pDst == NULL && pSrc == NULL)
	 {
		 return 0;
	 }
	 else if (pDst == NULL || pSrc == NULL)
	 {
		 return -1;
	 }
	return strncmp(pDst,pSrc,nCount);
 }

 S32 Common_StrniCmp(S8 *pDst,S8 *pSrc,S32 nCount)
 {
	 if (pDst == NULL && pSrc == NULL)
	 {
		 return 0;
	 }
	 else if (pDst == NULL || pSrc == NULL)
	 {
		 return -1;
	 }
	return strnicmp(pDst,pSrc,nCount);
 }
