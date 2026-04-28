#ifndef _REST_COMM_LIST_
#define _REST_COMM_LIST_

#ifdef __cplusplus
extern "C"{
#endif

#include"libcommon_api.h"

#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include<pthread.h>
#include"cjson.h"

typedef struct
{
	COMMON_DLIST_T listHdl;
	pthread_mutex_t	lock;
	pthread_cond_t	cnd;
}REST_LIST_HDL_T;

typedef enum
{
	TASK_TYPE_FUNC_CALL
}REST_LIST_TASK_TYPE_E;

typedef struct
{
	REST_LIST_TASK_TYPE_E	taskType;

	char			method[32];
	int				local;
	char*  			uri;
	char*			condition;
	Common_cJSON_T* param;
}REST_LIST_TASK_NODE_T;

int RestList_Init();
int RestList_Destroy();

int RestList_AddTask(REST_LIST_TASK_NODE_T* task);

#ifdef __cplusplus
};
#endif
#endif
