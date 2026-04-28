/**
 * @file libpipemsg_api.h
 * @date create on: 2017骞�6鏈�23鏃�
 * @author eric
 * @brief 
 * 
 * @defgroup
 * @{
 *  @note
 *
 */
#ifndef LIBPIPEMSG_API_H_
#define LIBPIPEMSG_API_H_

#ifdef WIN32
#else
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef NULL
#define NULL             (void *)0
#endif

/*
#ifndef __LIBCOMMON_API_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <time.h>
#include <stdio.h>

#define NONE             "\e[0m"
#define RED              "\e[0;31m"
#define YELLOW           "\e[0;33m"
#define CYAN             "\e[0;36m"

#define LOGE(x,...)  do {\
    char pname[32]; \
    char timestr[32]; \
    time_t lt;\
    struct tm lm;\
    time(&lt);\
    localtime_r(&lt,&lm);\
    strftime(timestr,30,"%Y-%m-%d %H:%M:%S",&lm);\
    prctl(PR_GET_NAME,pname); \
    printf(RED "[%s] [%s] [%ld] [ERROR] [%s:%d] "x, pname,timestr,syscall(SYS_gettid),__FUNCTION__, __LINE__, ##__VA_ARGS__);\
    printf(NONE);\
    }while(0)

#define LOGD(x,...)  do {\
    char pname[32]; \
    char timestr[32]; \
    time_t lt;\
    struct tm lm;\
    time(&lt);\
    localtime_r(&lt,&lm);\
    strftime(timestr,30,"%Y-%m-%d %H:%M:%S",&lm);\
    prctl(PR_GET_NAME,pname); \
    printf(CYAN "[%s] [%s] [%ld] [DEBUG] [%s:%d] "x, pname,timestr,syscall(SYS_gettid),__FUNCTION__, __LINE__, ##__VA_ARGS__);\
    printf(NONE);\
    }while(0)

#define LOGI(x,...)  do {\
    char pname[32]; \
    char timestr[32]; \
    time_t lt;\
    struct tm lm;\
    time(&lt);\
    localtime_r(&lt,&lm);\
    strftime(timestr,30,"%Y-%m-%d %H:%M:%S",&lm);\
    prctl(PR_GET_NAME,pname); \
    printf(NONE "[%s] [%s] [%ld] [INFO ] [%s:%d] "x, pname,timestr,syscall(SYS_gettid),__FUNCTION__, __LINE__, ##__VA_ARGS__);\
    printf(NONE);\
    }while(0)

#define LOGW(x,...)  do {\
    char pname[32]; \
    char timestr[32]; \
    time_t lt;\
    struct tm lm;\
    time(&lt);\
    localtime_r(&lt,&lm);\
    strftime(timestr,30,"%Y-%m-%d %H:%M:%S",&lm);\
    prctl(PR_GET_NAME,pname); \
    printf(YELLOW "[%s] [%s] [%ld] [WARN ] [%s:%d] "x, pname,timestr,syscall(SYS_gettid),__FUNCTION__, __LINE__, ##__VA_ARGS__);\
    printf(NONE);\
    }while(0)

#endif
*/

enum
{
    PIPE_MSG_TYPE_STATE, //remote connect or disconnect
    PIPE_MSG_TYPE_CMD,
    PIPE_MSG_TYPE_DATA,
};

enum
{
    PIPE_MSG_SUB_TYPE_CONNECT = 0, //remote connect
    PIPE_MSG_SUB_TYPE_DISCONNECT,  //remote disconnect
};


typedef void (*PipeRecvHandleF)(int type, int subType, void *data, int size);

int PipeMsg_Init(char *pipeName, int mode, PipeRecvHandleF RecvHandle, void **pipeHandle);
int PipeMsg_Uninit(void *pipeHandle);
int PipeMsg_Send(void *pipeHandle, int type, int subType, void *data, int size);
void PipeMsg_Clear(void *pipeHandle);

#ifdef __cplusplus
}
#endif

#endif /*WIN32*/

#endif /* LIBPIPEMSG_API_H_ */
/**
 * @}
 */
