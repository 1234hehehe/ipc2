/*
 * ants_ds.c
 *
 *  Created on: 2016年3月30日
 *      Author: eric
 */
#include "libcommon_api.h"
#include "libcommon_struct.h"
#include "queue.h"


#ifndef CIRCLEQ_ENTRY
#define	CIRCLEQ_ENTRY(type)						\
struct {								\
	struct type *cqe_next;		/* next element */		\
	struct type *cqe_prev;		/* previous element */		\
}
#endif

#ifndef CIRCLEQ_HEAD
#define	CIRCLEQ_HEAD(name, type)					\
struct name {								\
	struct type *cqh_first;		/* first element */		\
	struct type *cqh_last;		/* last element */		\
}
#endif

/**
 * 双链表节点定义
 */
typedef struct _DLIST_NODE_T
{
    void * data;
    U32 size;
    S32 refs;
    TAILQ_ENTRY(_DLIST_NODE_T) ptr;
} DLIST_NODE_T;

/**
 * 循环双链表节点定义
 */
typedef struct _CLIST_NODE_T
{
    void * data;
    CIRCLEQ_ENTRY(_CLIST_NODE_T) ptr;
} CLIST_NODE_T;

/**
 * 双链表头节点
 */
typedef TAILQ_HEAD(dlisth,_DLIST_NODE_T) COMMON_DLIST_TEADER_T;


/**
 * 循环双链表头节点
 */
typedef CIRCLEQ_HEAD(clisth,_CLIST_NODE_T)
CLIST_HEADER_T;

/**
 * 双链表handle
 */
typedef struct
{
    S32 state;
    COMMON_DLIST_TEADER_T header;
    S32 count;
    Common_RWLock_T mutex;
	COMMON_LIST_NODE_FREE_F nodeFree;
} DLIST_CONTEXT_T;

/*
 * 名称:    CreateDlistNode
 *              创建一个双链表节点
 * 参数:
 * 返回:
 */
static DLIST_NODE_T * CreateDlistNode(void *data, U32 size)
{
    DLIST_NODE_T *node = NULL;
    node = (DLIST_NODE_T *) Common_Malloc(sizeof(DLIST_NODE_T),0,__FUNCTION__,__LINE__);
    if (node == NULL)
    {
        return NULL;
    }
    node->refs = 0;
    node->data = data;
    node->size = size;
    return node;
}

/*
 * 名称:    DestroyDlistNode
 *              销毁一个双链表节点，并释放节点的数据指针
 * 参数:
 * 返回:
 */
static void DestroyDlistNode(DLIST_CONTEXT_T * dh, DLIST_NODE_T *np)
{
    if (np == NULL)
    {
        return;
    }
	if (np->data)
    {
        dh->nodeFree(np->data);
    }

    Common_Free(np,__FUNCTION__,__LINE__);
}

S32 Common_DList_Init(COMMON_DLIST_T*dlistHandle,COMMON_LIST_NODE_FREE_F nodeFree)
{
    DLIST_CONTEXT_T * dh = NULL;

    if (nodeFree == NULL || dlistHandle == NULL)
        return -1;

     dh = (DLIST_CONTEXT_T *)Common_Malloc(sizeof(DLIST_CONTEXT_T),0,__FUNCTION__,__LINE__);
	 if (dh == NULL)
	 {
		 return -1;
	 }
	 memset(dh,0,sizeof(DLIST_CONTEXT_T));
	*dlistHandle = (COMMON_DLIST_T )dh;
   

    TAILQ_INIT(&dh->header);
    dh->nodeFree = nodeFree;
	Common_RWLock_Create(&dh->mutex,NULL);
    

    Common_RWLock_WLock(dh->mutex);
    dh->state = 1;
    Common_RWLock_UnLock(dh->mutex);

    return 0;
}

S32 Common_DList_Uninit(COMMON_DLIST_T*dlistHandle)
{
    DLIST_CONTEXT_T * dh = (DLIST_CONTEXT_T *) (*dlistHandle);
    DLIST_NODE_T *np = NULL;

    if (dh == NULL || dh->state != 1)
    {
        return -1;
    }

    Common_RWLock_WLock(dh->mutex);
    dh->state = 0;
    while((np = TAILQ_FIRST(&dh->header)))
    {
        TAILQ_REMOVE(&dh->header, np, ptr);
        DestroyDlistNode(dh,np);
    }
     Common_RWLock_UnLock(dh->mutex);
     Common_RWLock_Destroy(&dh->mutex);

    Common_Free(dh,__FUNCTION__,__LINE__);
    *dlistHandle = NULL;
    return 0;
}

S32 Common_DList_InsertTail(COMMON_DLIST_T dlistHandle, void *data, U32 size)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *node = NULL;
    if (dlistHandle == NULL || data == NULL || dh->state == 0)
    {
        return -1;
    }

    node = CreateDlistNode(data, size);
    if (node == NULL)
    {
        return -1;
    }

    Common_RWLock_WLock(dh->mutex);
    TAILQ_INSERT_TAIL(&dh->header, node, ptr);
    dh->count++;
    Common_RWLock_UnLock(dh->mutex);
    return 0;
}

S32 Common_DList_Delete(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *np = NULL;
    S32 ret = -1;
    if (dlistHandle == NULL || cb == NULL || dh->state == 0)
    {
        return -1;
    }

    Common_RWLock_WLock(dh->mutex);
    TAILQ_FOREACH(np,&dh->header,ptr)
    {
        if (np->data == NULL)
        {
            TAILQ_REMOVE(&dh->header, np, ptr);
            DestroyDlistNode(dh,np);
            dh->count--;
            continue;
        }

        if (cb(np->data, data) == 0)
        {
            TAILQ_REMOVE(&dh->header, np, ptr);
            DestroyDlistNode(dh,np);
            dh->count--;
            ret = 0;
            break;
        }
    }
    Common_RWLock_UnLock(dh->mutex);
    return ret;
}

S32 Common_DList_DeleteAll(COMMON_DLIST_T dlistHandle)
{
    DLIST_CONTEXT_T * dh = (DLIST_CONTEXT_T *) (dlistHandle);
    DLIST_NODE_T *np = NULL;

    if (dh == NULL || dh->state == 0)
    {
        return -1;
    }

    Common_RWLock_WLock(dh->mutex);
    while((np = TAILQ_FIRST(&dh->header)))
    {
        TAILQ_REMOVE(&dh->header, np, ptr);
        DestroyDlistNode(dh,np);
    }
    dh->count = 0;
    Common_RWLock_UnLock(dh->mutex);

    return 0;
}

void * Common_DList_Search(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb)
{
    DLIST_CONTEXT_T *dh =  (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *np = NULL;
    if (dlistHandle == NULL || cb == NULL || dh->state == 0)
    {
        return NULL;
    }

    Common_RWLock_RLock(dh->mutex);
    TAILQ_FOREACH(np,&dh->header,ptr)
    {
        if (cb(np->data, data) == 0)
        {
            Common_RWLock_UnLock(dh->mutex);
            return np->data;
        }
    }
    Common_RWLock_UnLock(dh->mutex);
    return NULL;
}

S32 Common_DList_GetCount(COMMON_DLIST_T dlistHandle)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    S32 count = 0;

    if (dlistHandle == NULL || dh->state == 0)
    {
        return -1;
    }

   Common_RWLock_RLock(dh->mutex);
    count = dh->count;
    Common_RWLock_UnLock(dh->mutex);

    return count;
}

void* Common_DList_GetFirst(COMMON_DLIST_T dlistHandle)
{
    DLIST_NODE_T *nd = NULL;
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;

    if (dlistHandle == NULL || dh->state == 0)
    {
        return NULL;
    }

    Common_RWLock_RLock(dh->mutex);
    nd = TAILQ_FIRST(&dh->header);
    Common_RWLock_UnLock(dh->mutex);

    if (nd == NULL)
        return NULL;

    return nd->data;
}

void * Common_DList_GetNode(COMMON_DLIST_T dlistHandle, S32 idx)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *nd = NULL;
    S32 i = 0, count = 0;

    if (dlistHandle == NULL || idx < 0 || dh->state == 0)
    {
        return NULL;
    }

    Common_RWLock_RLock(dh->mutex);
    count = dh->count;
    if (idx >= count)
    {
       Common_RWLock_UnLock(dh->mutex);
        return NULL;
    }

    nd = TAILQ_FIRST(&dh->header);
    while (i < idx && nd != NULL)
    {
        nd = TAILQ_NEXT(nd, ptr);
        i++;
    }
   Common_RWLock_UnLock(dh->mutex);

    if (nd != NULL)
    {
        return nd->data;
    }

    return NULL;
}

void * Common_DList_LockNode(COMMON_DLIST_T dlistHandle, void **nodeHandle)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T **nd = (DLIST_NODE_T **)nodeHandle;
    DLIST_NODE_T *curNode = NULL;
    void *ret = NULL;
    if (dlistHandle == NULL || nodeHandle == NULL || dh->state == 0)
    {
        return NULL;
    }

     Common_RWLock_RLock(dh->mutex);
    if (*nd == NULL)
    {
        *nd = TAILQ_LAST(&dh->header,dlisth);
        if (*nd && (*nd)->data)
        {
            ret = (*nd)->data;
            if (ret != NULL)
                (*nd)->refs++;
        }
        else
        {
            ret = NULL;
        }
    }
    else
    {
        curNode = *nd;
        *nd = TAILQ_NEXT(*nd, ptr);
        if (*nd && (*nd)->data)
        {
            ret = (*nd)->data;
            if (ret != NULL)
                (*nd)->refs++;
        }
        else
        {
            ret = NULL;
            *nd = curNode;
        }
    }
     Common_RWLock_UnLock(dh->mutex);
    return ret;
}

S32 Common_DList_UnlockNode(COMMON_DLIST_T dlistHandle,void *data)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T * nd = NULL;

    if (dlistHandle == NULL || data == NULL || dh->state == 0)
    {
        return -1;
    }
     Common_RWLock_RLock(dh->mutex);
    TAILQ_FOREACH(nd,&dh->header,ptr)
    {
        if (nd->data == ((DLIST_NODE_T *)data)->data)
        {
            nd->refs--;
            break;
        }
    }
     Common_RWLock_UnLock(dh->mutex);
    return 0;
}

S32 Common_DList_DeleteWithCk(COMMON_DLIST_T dlistHandle)
{
    DLIST_NODE_T *nd = NULL;
    DLIST_CONTEXT_T *dh =  (DLIST_CONTEXT_T *) dlistHandle;
    S32 cnt = 0, ret = 0;
    
    if (dlistHandle == NULL || dh->state == 0)
    {
        return -1;
    }

     Common_RWLock_WLock(dh->mutex);
    cnt = dh->count;
    TAILQ_FOREACH(nd,&dh->header,ptr)
    {
        if (nd->refs > 0)
            continue;
        TAILQ_REMOVE(&dh->header, nd, ptr);
        DestroyDlistNode(dh,nd);
        dh->count--;
        break;
    }
    if (cnt == dh->count)
        ret = -1;
     Common_RWLock_UnLock(dh->mutex);
    return ret;
}

S32 ants_dlist_for_each(COMMON_DLIST_T dlistHandle, COMMON_LIST_FOR_EACH_F cb)
{
    DLIST_CONTEXT_T *dh =  (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *np = NULL;
    S32 refNum = 0;
    if (dlistHandle == NULL  || dh->state == 0)// || cb == NULL)
    {
        return -1;
    }

     Common_RWLock_RLock(dh->mutex);
    TAILQ_FOREACH(np,&dh->header,ptr)
    {
        if (np->refs != 0)
        {
            refNum++;
        }
        if (cb && cb(np->data) != 0)
            break;
    }
    LOGI("shmqueuelist count = %d  total ref num = %d\n",dh->count ,refNum);
    Common_RWLock_UnLock(dh->mutex);
    return 0;
}

void * Common_DList_Detach(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb)
{
    DLIST_CONTEXT_T *dh =  (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *np = NULL;
    void *npData = NULL;
    if (dlistHandle == NULL || cb == NULL || dh->state == 0)
    {
        return NULL;
    }

    Common_RWLock_WLock(dh->mutex);
    TAILQ_FOREACH(np,&dh->header,ptr)
    {
        if (cb(np->data, data) == 0)
        {
            TAILQ_REMOVE(&dh->header, np, ptr);
            npData = np->data;
            Common_Free(np,__FUNCTION__,__LINE__);
            np = NULL;
            dh->count--;
            break;
        }
    }
    Common_RWLock_UnLock(dh->mutex);
    return npData;
}

S32 Common_DList_Separate(COMMON_DLIST_T dlistHandleSrc, COMMON_DLIST_T dlistHandleDst, void *data,COMMON_LIST_COMPARE_F cb)
{
    DLIST_CONTEXT_T *dhSrc = (DLIST_CONTEXT_T *) dlistHandleSrc;
    DLIST_CONTEXT_T *dhDst = (DLIST_CONTEXT_T *) dlistHandleDst;
    DLIST_NODE_T *np = NULL, *tmp = NULL;

    if (dlistHandleSrc == NULL || dlistHandleDst == NULL || dhSrc->state == 0 || dhDst->state == 0)
    {
        return -1;
    }

    Common_RWLock_WLock(dhSrc->mutex);
    Common_RWLock_WLock(dhDst->mutex);
    np = TAILQ_FIRST(&dhSrc->header);
    while(np != NULL)
    {
        tmp = np->ptr.tqe_next;

        if (cb == NULL || (cb != NULL && cb(np->data,data) == 0))
        {
            TAILQ_REMOVE(&dhSrc->header, np, ptr);
            TAILQ_INSERT_TAIL(&dhDst->header, np, ptr);
            dhDst->count++;
            dhSrc->count--;
        }
        np = tmp;
    }

    Common_RWLock_UnLock(dhDst->mutex);
    Common_RWLock_UnLock(dhSrc->mutex);
    return 0;
}


S32 Common_DList_DeleteMulti(COMMON_DLIST_T dlistHandle, void *data, COMMON_LIST_COMPARE_F cb)
{
    DLIST_CONTEXT_T *dh = (DLIST_CONTEXT_T *) dlistHandle;
    DLIST_NODE_T *np = NULL, *tmp = NULL;
    S32 ret = -1;
    if (dlistHandle == NULL || cb == NULL || dh->state == 0)
    {
        return -1;
    }

    Common_RWLock_WLock(dh->mutex);
    np = TAILQ_FIRST(&dh->header);
    while(np != NULL)
    {
        tmp = np->ptr.tqe_next;
        if (cb(np->data, data) == 0)
        {
            TAILQ_REMOVE(&dh->header, np, ptr);
            DestroyDlistNode(dh,np);
            dh->count--;
            ret++;
        }
        np = tmp;
    }
    Common_RWLock_UnLock(dh->mutex);
    return ret;
}
