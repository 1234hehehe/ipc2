#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>

#include"ovfs_rest_comm_list.h"
#include"ovfs_rest_common.h"                                                  
#include"ovfs_rest_comm_cfg.h"

REST_LIST_HDL_T s_list_hdl;
static int      s_list_exit = 0;
Common_Thread_T s_list_thread = NULL;

static void FreeListNode(void* data)
{
    if(data == NULL)
        return;
    
    REST_LIST_TASK_NODE_T* node = (REST_LIST_TASK_NODE_T*)data;
    
    if(node->uri)
    {
        Common_Free(node->uri,__FUNCTION__,__LINE__);
        node->uri = NULL;
    }

    if(node->condition)
    {
        Common_Free(node->condition,__FUNCTION__,__LINE__);
        node->condition = NULL;
    }

    if(node->param)
    {
        Common_cJSON_Delete(node->param);
        node->param = NULL;
    }
        
    Common_Free(data,__FUNCTION__,__LINE__);
    return;
}

static int ListCompare(void* a, void* b)
{
    return memcmp(a,b,sizeof(REST_LIST_TASK_NODE_T));
}

static REST_LIST_TASK_NODE_T* GetTask(void)
{
    pthread_mutex_lock(&s_list_hdl.lock);
    REST_LIST_TASK_NODE_T* node = NULL;
    while(s_list_exit == 0)
    {
        node = (REST_LIST_TASK_NODE_T*)Common_DList_GetFirst(s_list_hdl.listHdl);
        if(node == NULL)
            pthread_cond_wait(&s_list_hdl.cnd,&s_list_hdl.lock);
        else
            break;
    }
    pthread_mutex_unlock(&s_list_hdl.lock);
    return node;
}

static int FreeTask(REST_LIST_TASK_NODE_T* task)
{   
    return Common_DList_Delete(s_list_hdl.listHdl,(void*)task,ListCompare);
}

static int GetTaskThread(Common_Thread_T hThreadHandle,void *pUserData)
{
    REST_LIST_TASK_NODE_T* node = NULL;
    while(s_list_exit == 0)
    {
        node = GetTask();
        if( s_list_exit || node == NULL)
            continue;
        
        if(node->taskType == TASK_TYPE_FUNC_CALL)
        {   
            char* out = NULL;
            LOGW("FunCall method=%s condition=%s param=%s\n",node->method,node->uri,node->condition,out = Common_cJSON_Print(node->param,NULL));
            if(out)
                Common_Free(out,__FUNCTION__,__LINE__);
            
            if(node->local == 0)
                RestComm_FuncCall(node->method,node->uri,node->condition,node->param,NULL);
            else
            {
                Common_cJSON_T* outParam = Common_cJSON_CreateObject();
                if(strcmp(node->method,"get")== 0)
                    RestComm_GetMyself(node->uri,node->condition,node->param,outParam);
                else if(strcmp(node->method,"put")== 0)             
                    RestComm_PutMyself(node->uri,node->condition,node->param,outParam);

                LOGW("method:%s outParm=%s\n",node->method,out = Common_cJSON_Print(outParam,NULL));
                if(out)
                    Common_Free(out,__FUNCTION__,__LINE__);
                Common_cJSON_Delete(outParam);
            }
        }
        else
            LOGE("unknow task type.\n");

        FreeTask(node);
        node = NULL;
    }
    return 0;
}

#if 0
static int TestThread(Common_Thread_T hThreadHandle,void *pUserData)
{
    Common_Sleep(2,0);

    while(1)
    {
        Common_Sleep(2,0);

        REST_LIST_TASK_NODE_T* p = (REST_LIST_TASK_NODE_T*)Common_Malloc(sizeof(REST_LIST_TASK_NODE_T),0,__FUNCTION__,__LINE__);
        memset(p,0,sizeof(REST_LIST_TASK_NODE_T));
        strcpy(p->method,"get");
        p->local = 1;
        p->uri = Common_StrDup("/BoardSystem/Video/Attribute/Device0/Channel0/Stream0",__FUNCTION__,__LINE__);
        RestList_AddTask(p);
    }
    return 0;
}
#endif

int RestList_Init()
{
    pthread_mutex_init(&s_list_hdl.lock,NULL);
    pthread_cond_init(&s_list_hdl.cnd,NULL);
    s_list_hdl.listHdl = NULL;
    s_list_thread = NULL;

    int ret = Common_DList_Init(&s_list_hdl.listHdl,FreeListNode);
    if(ret != 0)
    {
        LOGE("dlist init fail!\n");
        return -1;
    }

    ret = Common_Thread_Create(&s_list_thread,"GetTaskThread",0,0,GetTaskThread,NULL);
    if(ret != 0)
    {
        LOGE("create task thread fail! ret=%d\n",ret);
        return -1;
    }
    #if 0
    Common_Thread_T xx = NULL;

    ret = Common_Thread_Create(&xx,"TestThread",0,0,TestThread,NULL);
    if(ret != 0)
    {
        LOGE("create task thread fail! ret=%d\n",ret);
        return -1;
    }
    #endif
    return 0;
}
    
int RestList_Destroy()
{
    s_list_exit = 1;
    pthread_cond_signal(&s_list_hdl.cnd);
    Common_Thread_Destroy(&s_list_thread);
    Common_DList_Uninit(&s_list_hdl.listHdl);
    pthread_mutex_destroy(&s_list_hdl.lock);
    pthread_cond_destroy(&s_list_hdl.cnd);
    return 0;
}

int RestList_AddTask(REST_LIST_TASK_NODE_T* task)
{
    if(task == NULL)
        return -1;

    LOGI("add task. type=%d \n",task->taskType);
    
    pthread_mutex_lock(&s_list_hdl.lock);
    int ret = Common_DList_InsertTail(s_list_hdl.listHdl,task,sizeof(REST_LIST_TASK_NODE_T));
    if(ret != 0)
    {
        LOGE("inser fail. ret=%d\n",ret);
        pthread_mutex_unlock(&s_list_hdl.lock);
        return -1;
    }
    pthread_cond_signal(&s_list_hdl.cnd);
    pthread_mutex_unlock(&s_list_hdl.lock);
    return 0;
}

