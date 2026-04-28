/*
 * ovfs_com_utils.c
 *
 *  Created on: 2017年2月23日
 *      Author: eric
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <signal.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>

#include <libcommon_api.h>
#include "ovfs_media.h"

#ifdef ERIC_DEBUG
void* __real_malloc(size_t size);
void __real_free(void *ptr);
void* __real_calloc(size_t nmemb, size_t size);
void* __real_realloc(void *ptr, size_t size);
char* __real_strdup(const char *s);

void* __wrap_malloc(size_t size)
{
    void * ptr = NULL;
    int * ret = __builtin_return_address(0);
    ptr = __real_malloc(size);
    fprintf(stderr,"tid %ld malloc %p %d <%p>\n", syscall(SYS_gettid), ptr, size, ret);
    fflush(stderr);
    return ptr;
}

void* __wrap_calloc(size_t nmemb, size_t size)
{
    void * ptr = NULL;
    int * ret = __builtin_return_address(0);
    ptr = __real_calloc(nmemb,size);
    fprintf(stderr, "tid %ld calloc %p %d <%p>\n", syscall(SYS_gettid), ptr, nmemb*size, ret);

    fflush(stderr);
    return ptr;
}

void __wrap_free(void *ptr)
{
    int * ret = __builtin_return_address(0);

    fprintf(stderr, "tid %ld free %p <%p>\n", syscall(SYS_gettid), ptr, ret);
    fflush(stderr);
    __real_free(ptr);
}

void* __wrap_realloc(void *ptr, size_t size)
{
    void * reptr = NULL;
    int * ret = __builtin_return_address(0);
    reptr = __real_realloc(ptr,size);
    fprintf(stderr,"tid %ld realloc %p %d <%p>\n", syscall(SYS_gettid), reptr, size, ret);
    fflush(stderr);
    return ptr;
}

char* __wrap_strdup(const char *s)
{
    char * ptr = NULL;
    int * ret = __builtin_return_address(0);
    int size = strlen(s);
    ptr = __real_strdup(s);
    fprintf(stderr,"tid %ld strdup %p %d <%p>\n", syscall(SYS_gettid), ptr, size, ret);
    fflush(stderr);
    return ptr;
}

#endif
void Utils_Sleep(unsigned long long msec)
{
    struct timespec ts;
    int err;

    if (msec == 0)
        return;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do
    {
        err = clock_nanosleep(CLOCK_MONOTONIC,0,&ts, &ts);
    } while (err < 0 && errno == EINTR);
}

long long int Utils_GetMs(void)
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (long long int) ((long long int) tp.tv_sec * 1000L + (long long int) tp.tv_nsec / 1000000L);
}

int Utils_FileMonitor(char *path)
{
    int fd = 0, wd = 0, len = 0, nread = 0;
    char buf[64] = { 0 };
    struct inotify_event *event = NULL;

    int return_value = 0;
    fd_set descriptors;
    struct timeval time_to_wait;

    fd = inotify_init();
    if (fd < 0)
    {
        LOGE("inotify_init failed\n");
        return -1;
    }

    wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_MODIFY);
    if (wd < 0)
    {
        LOGE("inotify_add_watch %s failed\n", path);
        close(fd);
        return -1;
    }

    while (return_value >= 0)
    {
        FD_ZERO(&descriptors);
        FD_SET(fd, &descriptors);
        time_to_wait.tv_sec = 3;
        time_to_wait.tv_usec = 0;
        return_value = select(fd + 1, &descriptors, NULL, NULL, &time_to_wait);
        if (return_value < 0)
        {
            break;
        }
        else if (!return_value)
        {
            /* Timeout */
        }
        else if (FD_ISSET(fd, &descriptors))
        {
            while ((len = read(fd, buf, sizeof(buf) - 1)) > 0)
            {
                nread = 0;
                event = (struct inotify_event *) &buf[nread];
                if (event->mask & IN_CLOSE_WRITE)
                {
                    close(fd);
                    return 0;
                }
                nread = nread + sizeof(struct inotify_event) + event->len;
                len = len - sizeof(struct inotify_event) - event->len;
            }
        }
    }
    close(fd);
    return -1;
}
