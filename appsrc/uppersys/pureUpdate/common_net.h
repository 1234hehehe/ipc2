#ifndef __COMMON_NET_H__
#define __COMMON_NET_H__

#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include "libcommon_api.h"

typedef struct
{
    int sockfd;
    char ipAddr[32];
    char devName[32];
} UTILS_SOCKET_NODE_T;


#define TCP		0
#define UDP		1


int MultiAddrCheck(COMMON_DLIST_T sockList);
int MultiAddrCreateAndListen(COMMON_DLIST_T sockList,int type, int port);
int MultiAddrSelect(COMMON_DLIST_T sockList, int *sockfd, char *devName);
S32 CreateAndListenHttp();

#endif
