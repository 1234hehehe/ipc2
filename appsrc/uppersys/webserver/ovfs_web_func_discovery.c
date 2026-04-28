
#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <ifaddrs.h>
#include "ovfs_web_func.h"
#include "mxml.h"
#include "libmodule_api.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */
#endif
#include <linux/filter.h>

#if (OVFS_WEB_DISCOVERY == 1)

static unsigned char *web_common_getu32(unsigned char *lpBegin, unsigned char *lpEnd, unsigned int *lpVal);
static int web_discovery_get_headertype(const char *inbuffer, unsigned int insize);
static int web_discovery_server_request(char *devName, unsigned int stxcode, unsigned char *inbuffer, unsigned int insize);
static int web_discovery_generate_response(char *devName, int type, const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize);

static int web_discovery_response_discovery(char *devName, const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize);
static int web_discovery_response_modifyIP(char *devName, const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize);
static int web_discovery_response_setUUID(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize);
static int web_discovery_response_modifyMAC(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize);
static int web_discovery_response_customcmd(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize);
static int web_discovery_response_testtool(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize, int *bModifyIp);

static int web_discovery_compose_head(char *composeBuffer, unsigned bufferSize, int headerType);
static int web_discovery_compose_end(char *composeBuffer, unsigned bufferSize);

static int web_discovery_sendto_broadcaset(char *devName, int port, unsigned int u32stxcode, unsigned char *outbuf,
        unsigned int outsize);

static int QueryNetDevName(int devIndex, char *devName);
static int web_calc_checksum(unsigned char *buffer, unsigned int size);

static int web_discovery_compose_version(char *composebuff, unsigned int buffsize);
static int web_discovery_compose_network(char *devName, char *composebuff, unsigned int buffsize);
static int web_discovery_compose_service(char *composebuff, unsigned int buffsize);
static int ovfs_web_discovery_set_network(char *devName, char *s_ip, char *s_netmask, char *s_gateway, char *s_mac);

static int web_discovery_compose_port(char *composebuff, unsigned int buffsize);
static int ovfs_web_get_port_all(OVFS_PORT_ALL_T *p_server_port);

typedef struct
{
    int sockfd;
    char ipAddr[32];
    char devName[32];
} UTILS_UDP_SOCK_NODE_T;

static void UdpSockFree(void *data)
{
    if (data != NULL)
    {
        UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)data;
        if (node->sockfd != 0)
        {
            close(node->sockfd);
            node->sockfd = 0;
        }
        free(data);
    }
}


/* static int UdpSockDestroy(void *a, void *b) */
/* { */
/*     if (a) */
/*     { */
/*         UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)a; */
/*         if (node->sockfd != 0) */
/*             close(node->sockfd); */
/*         return 0; */
/*     } */

/*     return -1; */
/* } */

static int UdpSockSearchByName(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;
    UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)a;
    if (strcmp(node->devName, (char *)b) == 0)
        return 0;

    return -1;
}

static int UdpCreateSocket(int port, char *dev, char *ip)
{
    int sockfd = 0;
    unsigned int socklen = 0;
    struct sockaddr_in peeraddr = {};

    /* ���� socket ����UDPͨѶ */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        LOGE("socket creating err in udptalk\n");
        return -1;
    }

    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval)) != 0)
    {
        close(sockfd);
        LOGE("setsockopt failed %s\n", strerror(errno));
        return -1;
    }

    int reuseaddr = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    struct ifreq ifr = {};
    strcpy(ifr.ifr_name, dev);
    setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

    socklen = sizeof(struct sockaddr_in);
    memset(&peeraddr, 0, socklen);
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(port);
    /* peeraddr.sin_addr.s_addr = inet_addr(ip); */
    /* ���Լ��Ķ˿ں�IP��Ϣ��socket�� */
    if (bind(sockfd, (struct sockaddr *) &peeraddr,
             sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("Bind error %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    return sockfd;
}

static int MultiAddrCreateAndListen(COMMON_DLIST_T sockList, int port)
{
    struct ifaddrs *ifAddrStruct = NULL, *ifAddrStructSrc = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStructSrc);
    ifAddrStruct = ifAddrStructSrc;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            if (strcmp(addressBuffer, "127.0.0.1") == 0)
            {
                ifAddrStruct = ifAddrStruct->ifa_next;
                continue;
            }
            LOGD("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);

            UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)malloc(
                                              sizeof(UTILS_UDP_SOCK_NODE_T));
            node->sockfd = 0;
            snprintf(node->ipAddr, sizeof(node->ipAddr), "%s", addressBuffer);
            snprintf(node->devName, sizeof(node->devName), "%s",
                     ifAddrStruct->ifa_name);
            node->sockfd = UdpCreateSocket(port, ifAddrStruct->ifa_name, node->ipAddr);
            if (node->sockfd < 0)
                free(node);
            else
                Common_DList_InsertTail(sockList, node, sizeof(UTILS_UDP_SOCK_NODE_T));
        }
        // else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
        // {   // check it is IP6
        //     // is a valid IP6 Address
        //     tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        //     char addressBuffer[INET6_ADDRSTRLEN];
        //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        //     printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        // }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    freeifaddrs(ifAddrStructSrc);
    return 0;
}

static int MultiAddrCheck(COMMON_DLIST_T sockList)
{
    struct ifaddrs *ifAddrStruct = NULL, *ifAddrStructSrc = NULL;
    void *tmpAddrPtr = NULL;
    int cnt = 0;
    getifaddrs(&ifAddrStructSrc);
    ifAddrStruct = ifAddrStructSrc;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            // LOGD("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            if (strcmp(addressBuffer, "127.0.0.1") == 0)
            {
                ifAddrStruct = ifAddrStruct->ifa_next;
                continue;
            }

            if (Common_DList_Search(sockList, ifAddrStruct->ifa_name,
                                    UdpSockSearchByName) == NULL)
            {
                freeifaddrs(ifAddrStructSrc);
                return 1;
            }
            cnt++;
        }
        // else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
        // {   // check it is IP6
        //     // is a valid IP6 Address
        //     tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        //     char addressBuffer[INET6_ADDRSTRLEN];
        //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        //     printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        // }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    freeifaddrs(ifAddrStructSrc);
    if (cnt != Common_DList_GetCount(sockList))
        return 1;

    return 0;
}

static int MultiAddrSelect(COMMON_DLIST_T sockList, int *sockfd, char *devName)
{
    fd_set rfds;
    int retSelect = 0;
    struct timeval timeout = { 0, 0 };
    FD_ZERO(&rfds);
    int i = 0, cnt = 0, lsock = 0;
    cnt = Common_DList_GetCount(sockList);
    for (i = 0; i < cnt; i++)
    {
        UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *) Common_DList_GetNode(
                                          sockList, i);

        if (node == NULL)
            continue;
        FD_SET(node->sockfd, &rfds);
        lsock = node->sockfd;
    }
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    retSelect = select(lsock + 1, &rfds, NULL, NULL, &timeout);
    if (retSelect > 0)
    {
        for (i = 0; i < cnt; i++)
        {
            UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *) Common_DList_GetNode(
                                              sockList, i);

            if (node == NULL)
                continue;
            if (FD_ISSET(node->sockfd, &rfds))
            {
                *sockfd = node->sockfd;
                strcpy(devName, node->devName);
                break;
            }
        }
    }
    return retSelect;
}

int ovfs_web_discovery_init(COMMON_DLIST_T *sockList, int port)
{

    int ret = 0;
    Common_DList_Init(sockList, UdpSockFree);
    //LOGW("sockList:[%p] [%d]\n",sockList,Common_DList_GetCount(*sockList));
    ret = MultiAddrCreateAndListen(*sockList, port);
    return ret;
}

/* int ovfs_web_discovery_init(int port) */
/* { */
/*     int ret = 0; */
/*     int optval = 0; */
/*     struct sockaddr_in addr; */

/*     g_ovfs_web->discovery.sockfd_dis = socket(AF_INET, SOCK_DGRAM, 0); */
/*     if (g_ovfs_web->discovery.sockfd_dis == INVALID_SOCKET) */
/*     { */
/*         LOGE("Socket create Failed!\n"); */
/*     } */

/*     if (g_ovfs_web->discovery.sockfd_dis != INVALID_SOCKET) */
/*     { */
/*         optval= 1; */
/*      if(setsockopt(g_ovfs_web->discovery.sockfd_dis, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval)) == SOCKET_ERROR) */
/*      { */
/*          LOGE("Set UDP Socket Opt(SO_BROADCAST) Error!\n") ; */
/*          closesocket(g_ovfs_web->discovery.sockfd_dis) ; */
/*          g_ovfs_web->discovery.sockfd_dis = INVALID_SOCKET ; */
/*      } */
/*  } */

/*  if (g_ovfs_web->discovery.sockfd_dis != INVALID_SOCKET) */
/*  { */
/*      optval = 1; */
/*      if(setsockopt(g_ovfs_web->discovery.sockfd_dis, SOL_SOCKET,  SO_REUSEADDR, (char *)&optval, sizeof(optval)) == SOCKET_ERROR) */
/*      { */
/*          LOGE("Set UDP Socket Opt(SO_REUSEADDR) Error!\n") ; */
/*          closesocket(g_ovfs_web->discovery.sockfd_dis) ; */
/*          g_ovfs_web->discovery.sockfd_dis = INVALID_SOCKET ; */
/*      } */
/*  } */

/*  memset(&addr, 0, sizeof(struct sockaddr_in)); */
/*  addr.sin_family = AF_INET; */
/*  addr.sin_port = htons(port); */
/*  addr.sin_addr.s_addr = htonl(INADDR_ANY); */

/*  if (g_ovfs_web->discovery.sockfd_dis != INVALID_SOCKET) */
/*  { */
/*         if (bind(g_ovfs_web->discovery.sockfd_dis, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == SOCKET_ERROR) */
/*         { */
/*             LOGE("Bind Socket Error!\n"); */
/*         } */
/*  } */

/*     if (g_ovfs_web->discovery.sockfd_dis == INVALID_SOCKET) */
/*     { */
/*         ret = -1; */
/*     } */

/*     return ret; */
/* } */

int Fxn_Web_Discovery(Common_Thread_T hThreadHandle, void *pUserData)
{
    int result = 0;
    int recvlen = 0;
    unsigned char *lpBuffer = NULL;
    unsigned char *lpBegin = NULL;
    unsigned char *lpEnd = NULL;
    unsigned int u32StxCode;
    unsigned int u32LoadLen;
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(struct sockaddr_in);

    lpBuffer = Common_Calloc(1, 4096, __FUNCTION__, __LINE__);

    unsigned long long int timeA = 0, timeB = 0;

    timeA = timeB = Common_GetSystemCount64();

    while(1)
    {
        timeB = Common_GetSystemCount64();

        if (timeB - timeA > 1000LLU)
        {
            timeA = timeB;
            if (MultiAddrCheck(g_ovfs_web->discovery.sockList) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_Uninit(&g_ovfs_web->discovery.sockList);
                ovfs_web_discovery_init(&g_ovfs_web->discovery.sockList, g_ovfs_web->discovery.port);
                continue;
            }
        }

        memset(lpBuffer, 0, 4096);
        int sockfd = 0;
        char devName[32] = {};
        result = MultiAddrSelect(g_ovfs_web->discovery.sockList, &sockfd, devName);
        if (result < 0)
        {
            Common_DList_Uninit(&g_ovfs_web->discovery.sockList);
            ovfs_web_discovery_init(&g_ovfs_web->discovery.sockList, g_ovfs_web->discovery.port);
            LOGE("Select Error!\n");
            continue;
        }
        else if (result == 0)
        {
            continue;
        }
        else
        {
            recvlen = recvfrom(sockfd, (char *)lpBuffer, 4096, 0, (struct sockaddr *)&remote_addr,
                               &remote_addr_len);
            if (recvlen <= 8)
            {
                LOGE("Recv too short!\n");
                continue;
            }

            /* char ipAddr[32] = { 0 }; */
            /* snprintf(ipAddr, sizeof(ipAddr), "%s", inet_ntoa(remote_addr.sin_addr)); */
            /* LOGD("recv discovery from %s\n",ipAddr); */

            lpBegin = lpBuffer;
            lpEnd = lpBegin + recvlen;
            lpBegin = web_common_getu32(lpBegin, lpEnd, &u32StxCode);
            lpBegin = web_common_getu32(lpBegin, lpEnd, &u32LoadLen);

            //  LOGW("Socket[%d] StxCode[0x%x] LoadLen[%d]\r\n", g_ovfs_web->discovery.sockfd_dis, u32StxCode, u32LoadLen);

            switch (u32StxCode)
            {
            case StxCode_V1:
            {
                //      LOGW("V1\n");
            }
            break;
            case StxCode_V2:
            {
                web_discovery_server_request(devName, StxCode_V2, lpBegin, u32LoadLen);
            }
            break;
            case StxCode_V3:
            {
                //          LOGW("V3\n");
            }
            break;
            default:
            {
                //           LOGW("Default\n");
            }
            }
        }
    }

    Common_DList_Uninit(&g_ovfs_web->discovery.sockList);
    Common_Free(lpBuffer, __FUNCTION__, __LINE__);

    return 0;
}

static unsigned char *web_common_getu32(unsigned char *lpBegin, unsigned char *lpEnd, unsigned int *lpVal)
{
    if(lpBegin + 4 > lpEnd)
    {
        return NULL ;
    }

    /* Properly reconstruct the 32-bit value from bytes to avoid type aliasing issues */
    *lpVal = ((unsigned int)lpBegin[0] << 0) |
             ((unsigned int)lpBegin[1] << 8) |
             ((unsigned int)lpBegin[2] << 16) |
             ((unsigned int)lpBegin[3] << 24);

    return (lpBegin + 4 ) ;
}

static int web_discovery_server_request(char *devName, unsigned int stxcode, unsigned char *inbuffer, unsigned int insize)
{
    int ret = 0;
    int headertype = 0;
    unsigned char *outbuffer = NULL;
    unsigned int outsize = 1500;

	if (access("/tmp/updating", F_OK) == 0)
	{
		//is updating
		return 0;
	}

    headertype = web_discovery_get_headertype((const char *)inbuffer, insize);
    if (0 == headertype)
    {
        ret = -1;
    }
    else
    {
        outbuffer = Common_Calloc(1, 1500, __FUNCTION__, __LINE__);
        // ������Ӧ
        ret = web_discovery_generate_response(devName, headertype, (const char *)inbuffer, insize, (char *)outbuffer,
                                              &outsize);
        if ((0 == ret) && (outsize > 0))
        {
            // ����
            web_discovery_sendto_broadcaset(devName, g_ovfs_web->discovery.port, stxcode, outbuffer, outsize);
        }

        Common_Free(outbuffer, __FUNCTION__, __LINE__);
    }
    return ret;
}

static int web_discovery_get_headertype(const char *inbuffer, unsigned int insize)
{
    int headertype = 0;
    const char *value;
    char *mxmlString = NULL;
    mxml_node_t *tree = NULL;
    mxml_node_t *node_1 = NULL;
    mxml_node_t *node_2 = NULL;
    mxmlString = calloc(1, insize + 64);
    /* LOGE("inbuffer size is %d %d\n",strlen(inbuffer), insize); */
    sprintf(mxmlString, "<?xml version=\"1.0\" encoding=\"big5\" ?>%s", inbuffer);

    //LOGD("web_discovery_get_headertype inbuffer=%s\n",mxmlString);
    tree = mxmlLoadString(NULL, NULL, mxmlString);

    node_1 = mxmlFindElement(tree, tree, "Result", NULL, NULL, MXML_DESCEND_ALL);
    //LOGW("node_1:%p\n",node_1);

    if (node_1 == NULL)
    {
        //mxmlSaveString(tree, buff, sizeof(buff), MXML_TEXT_CALLBACK);
        //LOGW("Tmp:%s\n", buff);

        node_2 = mxmlFindElement(tree, tree, "HEADER", NULL, NULL, MXML_DESCEND_ALL);
        if (node_2)
        {
            value = mxmlElementGetAttr(node_2, "TYPE");
            headertype = atoi(value);
            //LOGW("Type:%d\n", headertype);
        }
    }

    if (headertype == Command_Discovery)
    {
        if (insize < REQUEST_DISCOVERY_PACKET_LEN_MIN || insize > REQUEST_DISCOVERY_PACKET_LEN_MAX)
        {
            LOGE("Discovery packet length is out of range(%d-%d).\n",
                 REQUEST_DISCOVERY_PACKET_LEN_MIN, REQUEST_DISCOVERY_PACKET_LEN_MAX);
            headertype = 0;
        }
    }
    else if (headertype == Command_ModifyIPAddr)
    {
        if (insize < REQUEST_MODIFYIP_PACKET_LEN_MIN || insize > REQUEST_MODIFYIP_PACKET_LEN_MAX)
        {
            LOGE("ModifyIp packet length is out of range(%d-%d).\n",
                 REQUEST_MODIFYIP_PACKET_LEN_MIN, REQUEST_MODIFYIP_PACKET_LEN_MAX);
            headertype = 0;
        }
    }
    mxmlDelete(tree);
    free(mxmlString);
    return headertype;
}

static int web_discovery_generate_response(char *devName, int type, const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize)
{
    int ret = 0;

    if (type == Command_Discovery)
    {
        ret = web_discovery_response_discovery(devName, inbuff, insize, outbuff, outsize);
    }
    else if (type == Command_ModifyIPAddr)
    {
        LOGD("inbuffer:[%s]\n", inbuff);
        ret = web_discovery_response_modifyIP(devName, inbuff, insize, outbuff, outsize);
        if(ret == 0)
        {
            //      ret = web_discovery_response_discovery(inbuff, insize, outbuff, outsize);
        }
    }
    else if(type == ANTS_WRITEUMEYEUUID_V2)
    {
        ret = web_discovery_response_setUUID(inbuff, insize, outbuff, outsize);
        if(ret == 0)
        {
            //      ret = web_discovery_response_discovery(inbuff, insize, outbuff, outsize);
        }
    }
    else if(type == Command_ModifyMac)
    {
        ret = web_discovery_response_modifyMAC(inbuff, insize, outbuff, outsize);
    }
    else if (type == 0x3030) //== Command_Custom)
    {
        ret = web_discovery_response_customcmd(inbuff, insize, outbuff, outsize);
    }
    else if (type == Command_TestTool_GetTestResult)
    {
        int bModifyIp = 0;
        ret = web_discovery_response_testtool(inbuff, insize, outbuff, outsize, &bModifyIp);
        if(ret == 0 && bModifyIp)
        {
            ret = web_discovery_response_discovery(devName, inbuff, insize, outbuff, outsize);
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}

unsigned long long int discoveryUpdateTime = 0;

char discoveryXml[1500] = {0};

static int web_discovery_response_discovery(char *devName, const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize)
{
    int ret = 0;
    int k = 0;
    char *result = NULL;
    int result_size = 1500;

    unsigned long long int timeA = 0;

    timeA = Common_GetSystemCount64();

    if (timeA - discoveryUpdateTime > 1000LLU)
    {
        discoveryUpdateTime = timeA;
        //LOGD("-----get from url\n");

        result = Common_Calloc(1, result_size, __FUNCTION__, __LINE__);
        memset(result, 0, sizeof(char)*result_size);

        // 1.Header
        k += web_discovery_compose_head(result + k, result_size - k, Command_Discovery);
        // 2.sn
        k += web_discovery_compose_version(result + k, result_size - k);
        // 3.FactoryInfo (DeviceType, Sensor, HardWare, Customer, UUid, DeviceName, admininfo)

        // 4.Version (version, buildversion)

        // 5.Net (mac, ipv4, netmask, gateway, dns1, dns2)
        k += web_discovery_compose_network(devName, result + k, result_size - k);
        // 6.Service
        k += web_discovery_compose_service(result + k, result_size - k);

        // 7.Port (HTTP, HTTPS, RTSP, ONVIF)
        k += web_discovery_compose_port(result + k, result_size - k);

        // 8.End
        k += web_discovery_compose_end(result + k, result_size - k);

        if (k > (int)*outsize)
        {
            ret = -1;
        }
        else
        {
            //LOGD("-----memcpy\n");
            memcpy(outbuff, result, k);
            memset(discoveryXml, 0, sizeof(discoveryXml));
            memcpy(discoveryXml, result, k);
            *outsize = (unsigned int)k;
            //  LOGD("\n\n%s\n\n",result);
        }

        Common_Free(result, __FUNCTION__, __LINE__);
    }
    else
    {
        *outsize = (unsigned int)strlen(discoveryXml);
        LOGD("-----get from cache [%d]\n",*outsize);
        memcpy(outbuff, discoveryXml, *outsize);
    }

    return ret;
}

static int web_discovery_response_modifyIP(char *devName, const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize)
{
    int ret = 0;
    int k = 0;
    char *result = NULL;
    int result_size = 1500;

    char *s_ip = NULL;
    char *s_netmask = NULL;
    char *s_gateway = NULL;
    char *s_mac = NULL;
    mxml_node_t *tree = NULL;
    mxml_node_t *node_1 = NULL;
    mxml_node_t *node_2 = NULL;
    //mxml_node_t *node_3 = NULL;
    //mxml_node_t *node_4 = NULL;
    char mxmlString[1024] = {0};

    sprintf(mxmlString, "<?xml version=\"1.0\" encoding=\"big5\" ?>%s", inbuff);
    tree = mxmlLoadString(NULL, NULL, mxmlString);

    node_1 = mxmlFindElement(tree, tree, "PARAMETERS", NULL, NULL, MXML_DESCEND_ALL);
    if (node_1)
    {
        node_2 = mxmlFindElement(node_1, tree, "LocalIp", NULL, NULL, MXML_DESCEND_ALL);
        if(node_2)
        {
            s_ip = mxmlGetText(node_2,NULL);//node_2->child->value.text.string;
            //   LOGD("---->localIP:%s\n",s_ip);
        }
        node_2 = mxmlFindElement(node_1, tree, "NetMask", NULL, NULL, MXML_DESCEND_ALL);
        if(node_2)
        {
            s_netmask = mxmlGetText(node_2,NULL);//node_2->child->value.text.string;
            //      LOGD("---->localIP:%s\n",s_ip);
        }
        node_2 = mxmlFindElement(node_1, tree, "GateWay", NULL, NULL, MXML_DESCEND_ALL);
        if(node_2)
        {
            s_gateway = mxmlGetText(node_2,NULL);//node_2->child->value.text.string;
            //     LOGD("---->localIP:%s\n",s_ip);
        }
        node_2 = mxmlFindElement(node_1, tree, "MacAddress", NULL, NULL, MXML_DESCEND_ALL);

        if(node_2)
        {
            s_mac = mxmlGetText(node_2,NULL);//node_2->child->value.text.string;
            //     LOGD("---->localIP:%s\n",s_ip);
        }
        ret = ovfs_web_discovery_set_network(devName, s_ip, s_netmask, s_gateway, s_mac);
        //   LOGD("---->SET IP!:%d\n",ret);
    }

    if(ret == 0)
    {
        //LOGW("\n");
        result = Common_Calloc(1, result_size, __FUNCTION__, __LINE__);
        memset(result, 0, sizeof(char)*result_size);
        // 1.Header
        k += web_discovery_compose_head(result + k, result_size - k, Command_ModifyIPAddr);
        // 2.sn
        k += web_discovery_compose_version(result + k, result_size - k);
        // 3.FactoryInfo (DeviceType, Sensor, HardWare, Customer, UUid, DeviceName, admininfo)

        // 4.Version (version, buildversion)

        // 5.Net (mac, ipv4, netmask, gateway, dns1, dns2)
        k += web_discovery_compose_network(devName, result + k, result_size - k);
        // 6.Service
        k += web_discovery_compose_service(result + k, result_size - k);

        // 7.Port (HTTP, HTTPS, RTSP, ONVIF)
        k += web_discovery_compose_port(result + k, result_size - k);

        // 8.End
        k += web_discovery_compose_end(result + k, result_size - k);

        if (k > (int)*outsize)
        {
            ret = -1;
        }
        else
        {
            memcpy(outbuff, result, k);
            *outsize = (unsigned int)k;
        }

        Common_Free(result, __FUNCTION__, __LINE__);
    }

    mxmlDelete(tree);

    return ret;
}

static int web_discovery_response_setUUID(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize)
{
    int ret = 0;

    LOGW("\n");

    return ret;
}

static int web_discovery_response_modifyMAC(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize)
{
    int ret = 0;

    LOGW("\n");

    return ret;
}

static int web_discovery_response_customcmd(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize)
{
    int ret = 0;

    LOGW("\n");

    return ret;
}

static int web_discovery_response_testtool(const char *inbuff, unsigned int insize, char *outbuff,
        unsigned int *outsize, int *bModifyIp)
{
    int ret = 0;

    LOGW("\n");

    return ret;
}


// Compose XML
static int web_discovery_compose_head(char *composeBuffer, unsigned bufferSize, int headerType)
{
    int k = 0;
    k += snprintf(composeBuffer + k, bufferSize - k, "<MESSAGE VERSION=\"1\">\n");
    k += snprintf(composeBuffer + k, bufferSize - k, "<HEADER TYPE=\"%d\"/>\n", headerType);
    k += snprintf(composeBuffer + k, bufferSize - k, "<Result CODE=\"0\"/>\n");
    k += snprintf(composeBuffer + k, bufferSize - k, "<PARAMETERS>\n");

    return k;
}

static int web_discovery_compose_end(char *composeBuffer, unsigned bufferSize)
{
    int k = 0;

    k += snprintf(composeBuffer + k, bufferSize - k, "</PARAMETERS>\n");
    k += snprintf(composeBuffer + k, bufferSize - k, "</MESSAGE>\n");

    return k;
}

static int web_discovery_sendto_broadcaset(char *devName, int port, unsigned int u32stxcode, unsigned char *outbuf,
        unsigned int outsize)
{
    int ret = 0;
    int iloop = 0;
    int optval = 0;
    int net_count = 0;
    struct ifreq ifr;
    int sockfd = INVALID_SOCKET;
    char if_name[32] = {0};
    struct sockaddr_in remote_addr;
    OVFS_WEB_DISCOVERY_PACKET_T *discovery_data = NULL;

    //LOGW("\n");

    discovery_data = Common_Calloc(1, sizeof(OVFS_WEB_DISCOVERY_PACKET_T), __FUNCTION__, __LINE__);
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr.s_addr = INADDR_BROADCAST;
    // ������ѯ
    net_count = QueryNetDevName(0, NULL);
    for (iloop = 0; iloop < net_count; iloop ++)
    {
        sockfd = INVALID_SOCKET;
        QueryNetDevName(iloop, if_name);
        if (strcmp(if_name, devName) != 0)
            continue;
        //LOGW("IF_NAME:%s\n", if_name);
        // 1.����socketfd
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        // 2.����socketfd
        optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval));
        optval = 1;
        setsockopt(sockfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&optval, sizeof(optval));
        sprintf(ifr.ifr_ifrn.ifrn_name, "%s", if_name);
        setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));
        // 3.��������
        memset(discovery_data, 0, sizeof(OVFS_WEB_DISCOVERY_PACKET_T));
        memcpy(discovery_data->u8Content, outbuf, outsize);

        discovery_data->u32StxCode = u32stxcode;
        discovery_data->u32EtxCode = EtxCode_V1;
        discovery_data->u32LoadLen = outsize;
        discovery_data->u32CheckSum = (unsigned int)web_calc_checksum((unsigned char *)discovery_data,
                                      sizeof(OVFS_WEB_DISCOVERY_PACKET_T) - sizeof(unsigned int));
        sendto(sockfd, (char *)discovery_data, sizeof(OVFS_WEB_DISCOVERY_PACKET_T), 0,
               (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
        // 4.�ر�socketfd
        closesocket(sockfd);
        sockfd = INVALID_SOCKET;
    }
    Common_Free(discovery_data, __FUNCTION__, __LINE__);
    discovery_data = NULL;

    return ret;
}

static int QueryNetDevName(int devIndex, char *devName)
{
    int ret = 0;

    static int s_netDevCount = 0;
    static char s_netDevName[256] = "";

    if (devIndex == 0 && devName == NULL)
    {
        FILE *fp = fopen("/proc/net/dev", "r");
        if (fp == NULL)
        {
            ret = -1;
        }

        if (0 == ret)
        {
            char buf[512];
            // Eat first two lines.
            fgets(buf, sizeof(buf), fp);
            fgets(buf, sizeof(buf), fp);

            // Read all net devices name.
            int netDevCount = 0;
            char *netDevName = s_netDevName;
            int maxsize = sizeof(s_netDevName);
            int size = 0;
            int index = 0;
            for (; fgets(buf, sizeof(buf), fp); index++)
            {
                char *pstart = buf;
                char *pstop = buf + sizeof(buf);
                while (*pstart == ' ' && pstart < pstop)
                {
                    pstart++;
                }
                pstop = pstart;
                while (*pstop && pstop < buf + sizeof(buf) && (*pstop != ':' && *pstop != ' '))
                {
                    pstop++;
                }
                if (strncmp(pstart, "eth", 3) == 0 || strncmp(pstart, "wlan", 4) == 0)
                {
                    netDevCount++;
                    snprintf(netDevName, MIN2(maxsize - size, pstop - pstart + 1), "%s", pstart);
                    size += strlen(netDevName) + 1;
                    netDevName += strlen(netDevName) + 1;
                }
            }

            s_netDevCount = netDevCount;

            // Query net dev count.
            ret = s_netDevCount;
        }

        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
    }
    else if (devIndex >= 0 && devIndex < s_netDevCount && devName)
    {
        char *pstart = s_netDevName;
        int i;
        for (i = 0; i < devIndex; i++)
        {
            pstart += strlen(pstart) + 1;
        }
        snprintf(devName, 16, "%s", pstart);
    }
    else
    {
        ret = -1;
    }

    return ret;
}

static int web_calc_checksum(unsigned char *buffer, unsigned int size)
{
    unsigned char i ;
    unsigned char crc = 0 ;
    unsigned char *lpPtr = buffer ;

    while(size-- != 0)
    {
        for(i = 0x80; i != 0; i /= 2)
        {
            if((crc & 0x8000) != 0)
            {
                /* ��ʽCRC����2����CRC */
                crc *= 2;
                crc ^= 0x1021;
            }
            else
            {
                crc *= 2;
            }

            if((*lpPtr & i) != 0)
            {
                crc ^= 0x1021; /* �ټ��ϱ�λ��CRC */
            }
        }

        lpPtr++;
    }

    return(crc) ;
}

static int web_discovery_compose_version(char *composebuff, unsigned int buffsize)
{
    int k = 0;
    int ret = 0;
    char *devicetype = NULL;
    char *devicename = NULL;
    char *hardware = NULL;
    //char *software = NULL;
    char *ver = NULL;
    char *build = NULL;
    char *vendor = NULL;
    char *str_serial = NULL;
    char *str_sensor = NULL;
    char version[32] = {0};
    int ver_major = 0;
    int ver_minor = 0;
    char build_data[32] = {0};
    int year = 2017;
    int month = 6;
    int day = 28;
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *header = NULL;

    if (0 == ret)
    {
        if ((header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Version");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (ret == 0)
        {
            //Common_Json_StandardPrint(pResult, NULL, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "DeviceType", NULL, &devicetype, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "DeviceName", NULL, &devicename, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "Version", NULL, &ver, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "HardVersion", NULL, &hardware, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "BuildDate", NULL, &build, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "SerialNumber", NULL, &str_serial, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "SensorModel", NULL, &str_sensor, NULL, NULL);
            Common_Json_GetAttrValue(pResult, -1, "Vendor", NULL, &vendor, NULL, NULL);
            if (devicetype)
            {
                char buf[5] = {0};
                strncpy(buf, str_serial, 4);
                //  LOGD("-----------[%s]\n",buf);
                k += snprintf(composebuff + k, buffsize - k, "<DeviceType>%s-%s-1CH</DeviceType>\n", devicetype, buf);
            }
            if (devicename && slen(devicename)>0)
            {
                k += snprintf(composebuff + k, buffsize - k, "<DeviceName>%s</DeviceName>\n", devicename);
            }
            if (str_serial)
            {
                k += snprintf(composebuff + k, buffsize - k, "<SerialNo>%s</SerialNo>\n", str_serial);
            }
            if (str_sensor)
            {
                k += snprintf(composebuff + k, buffsize - k, "<Sensor>%s</Sensor>\n", str_sensor);
            }
            if (hardware && ver)
            {
                k += snprintf(composebuff + k, buffsize - k, "<OvfsVersion swVersion=\"%s\" HwVersion=\"%s\" />\n", ver,
                              hardware);
            }
            if (vendor)
            {
                k += snprintf(composebuff + k, buffsize - k, "<Vendor>%s</Vendor>\n", vendor);
            }
#ifdef WITH_FACE_RECOGNIZE_AND_COMPARE
            k += snprintf(composebuff + k, buffsize - k, "<SupportFaceRecg>1</SupportFaceRecg>\n");
#endif

            if (ver && build)
            {
                sscanf(ver, "V%d.%d", &ver_major, &ver_minor);
                sscanf(build, "%04d%02d%02d", &year, &month, &day);
                snprintf(version, sizeof(version), "%d", (ver_major << 16) | (ver_minor));
                snprintf(build_data, sizeof(build_data), "%d",
                         ((year & 0xffff) << 16) | ((month & 0xff) << 8) | (day & 0xff));
                //hard code version
                k += snprintf(composebuff + k, buffsize - k, "<Version Version=\"%s\" Date=\"%s\"/>\n", version, build_data);
            }

        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }
    k += snprintf(composebuff + k, buffsize - k, "<Programs OldPrograms=\"2\"/>\n");

    if (header)
    {
        Common_Json_Delete(header);
        header = NULL;
    }

    return k;
}
/*char hwaddr[32] ipv4[32] netmask[32]*/
static int GetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask)
{
    int ret = 0;
    if (ifname == NULL)
    {
        LOGW("ifname is null\n");
        return -1;
    }

    struct ifreq ifr;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        ret = -1;
    }
    else
    {
        strncpy(ifr.ifr_name, ifname, 16);
        if (hwaddr != NULL)
        {
            if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0)
            {
                snprintf(hwaddr, 32, "%02X:%02X:%02X:%02X:%02X:%02X",
                         (unsigned char) ifr.ifr_hwaddr.sa_data[0],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[1],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[2],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[3],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[4],
                         (unsigned char) ifr.ifr_hwaddr.sa_data[5]);
            }
            else
            {
                ret = errno;
                LOGE("ret=%d(%s)\n", ret, strerror(ret));
                ret = -1;
            }
        }

        if (ipv4 != NULL)
        {
            if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0)
            {
                struct sockaddr_in *myaddr;
                myaddr = (struct sockaddr_in *) &ifr.ifr_addr;
                memcpy(ipv4, inet_ntoa(myaddr->sin_addr), 32);/*ip len is 32*/
            }
            else
            {
                ret = errno;
                LOGE("ret=%d(%s)\n", ret, strerror(ret));
                ret = -1;
            }
        }

        if (netmask != NULL)
        {
            if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0)
            {
                struct sockaddr_in *myaddr;
                myaddr = (struct sockaddr_in *) &ifr.ifr_netmask;
                memcpy(netmask, inet_ntoa(myaddr->sin_addr), 32);/*netmask len is 32*/
            }
            else
            {
                ret = errno;
                LOGE("ret=%d(%s)\n", ret, strerror(ret));
                ret = -1;
            }
        }
    }

    if (skfd >= 0)
    {
        close(skfd);
        skfd = -1;
    }

    return ret;
}

static int SetNetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask)
{
    int ret = 0;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        LOGE("create socket failed %s\n", strerror(errno));
        return -1;
    }
    strncpy(ifr.ifr_name, ifname, 16);

    if (ipv4)
    {
        struct sockaddr_in *addr;
        addr = (struct sockaddr_in *) & (ifr.ifr_addr);
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr(ipv4);
        if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0)
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }

    if (netmask)
    {
        struct sockaddr_in *addr;
        addr = (struct sockaddr_in *) & (ifr.ifr_addr);
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr(netmask);
        if (ioctl(skfd, SIOCSIFNETMASK, &ifr) < 0)
        {
            ret = errno;
            LOGE("ret=%d(%s)\n", ret, strerror(ret));
            ret = -1;
        }
    }

    if (skfd >= 0)
    {
        close(skfd);
        skfd = -1;
    }

    return ret;
}

struct route_info
{
    u_int dstAddr;
    u_int srcAddr;
    u_int gateWay;
    char ifName[IF_NAMESIZE];
};

static int ReadNlSock(int sockFd, char *bufPtr, int bufSize, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do
    {
//�յ��ں˵�Ӧ��
        if((readLen = recv(sockFd, bufPtr, bufSize - msgLen, 0)) < 0)
        {
            perror("SOCK READ: ");
            return -1;
        }
        nlHdr = (struct nlmsghdr *)bufPtr;

//���header�Ƿ���Ч
        if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
        {
            perror("Error in recieved packet");
            return -1;
        }

        if(nlHdr->nlmsg_type == NLMSG_DONE)
        {
            break;
        }
        else
        {
            bufPtr += readLen;
            msgLen += readLen;
        }
        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
        {
            break;
        }

    }
    while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

    return msgLen;
}


//�������ص�·����Ϣ
static void parseRoutes(char *devName, struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *gateway)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;
    struct in_addr dst;
    struct in_addr gate;

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

// If the route is not for AF_INET or does not belong to main routing table
//then return.
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
    {
        return;
    }

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);

    for(; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen))
    {
        switch(rtAttr->rta_type)
        {
        case RTA_OIF:
            {
                int oif;
                memcpy(&oif, RTA_DATA(rtAttr), sizeof(int));
                if_indextoname(oif, rtInfo->ifName);
            }
            break;
        case RTA_GATEWAY:
            memcpy(&rtInfo->gateWay, RTA_DATA(rtAttr), sizeof(u_int));
            break;
        case RTA_PREFSRC:
            memcpy(&rtInfo->srcAddr, RTA_DATA(rtAttr), sizeof(u_int));
            break;
        case RTA_DST:
            memcpy(&rtInfo->dstAddr, RTA_DATA(rtAttr), sizeof(u_int));
            break;
        }
    }

    dst.s_addr = rtInfo->dstAddr;
    if(!strcmp((char *)inet_ntoa(dst), "0.0.0.0") && strcmp(rtInfo->ifName, devName) == 0)
    {
        /* printf("if:%s\n", rtInfo->ifName); */
        gate.s_addr = rtInfo->gateWay;
        sprintf(gateway, (char *)inet_ntoa(gate));
        /* printf("gw%s\n", gateway); */
        gate.s_addr = rtInfo->srcAddr;
        /* printf("src:%s\n", (char *)inet_ntoa(gate)); */
        gate.s_addr = rtInfo->dstAddr;
        /* printf("dst:%s\n", (char *)inet_ntoa(gate)); */
    }

    return;
}

static int GetGateWay(char *devName, char *gateway)
{
    struct nlmsghdr *nlMsg;
    struct route_info *rtInfo;
    char *msgBuf = NULL;
    int sock, len, msgSeq = 0;


    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    {
        perror("Socket Creation: ");
        return -1;
    }

    msgBuf = (char *)calloc(1, 4096);

    nlMsg = (struct nlmsghdr *)msgBuf;
    /* rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg); */
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0)
    {
        return -1;
    }

    if((len = ReadNlSock(sock, msgBuf, 4096, msgSeq, getpid())) < 0)
    {
        free(msgBuf);
        return -1;
    }

    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

    for(; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len))
    {
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(devName, nlMsg, rtInfo, gateway);
    }

    free(rtInfo);
    free(msgBuf);
    close(sock);
    return 0;
}

static int web_discovery_compose_network(char *devName, char *composebuff, unsigned int buffsize)
{
    int k = 0;
    char *ifname = NULL;
    char ipV4[32] = {};
    char ipMaskV4[32] = {};
    char GatewayV4[32] = {};
    char mac[32] = {};

    if (devName && strncmp("eth0", devName, 4) == 0)
    {
        ifname = "Eth0";
    }
    else if (devName && strncmp("wlan0", devName, 5) == 0)
    {
        ifname = "WIFI";
    }
    else
    {
        ifname = "Eth0";
    }

    GetDevInfo(devName, mac, ipV4, ipMaskV4);
    GetGateWay(devName, GatewayV4);
    /* LOGW("%s ip %s mask %s mac %s gatewayv4 %s\n", */
    /*      devName, ipV4, ipMaskV4, mac, GatewayV4); */
    k += snprintf(composebuff + k, buffsize - k, "<%s>\n", ifname);

    k += snprintf(composebuff + k, buffsize - k, "<MacAddress>%s</MacAddress>\n", mac);
    k += snprintf(composebuff + k, buffsize - k, "<LocalIp>%s</LocalIp>\n", ipV4);
    k += snprintf(composebuff + k, buffsize - k, "<NetMask>%s</NetMask>\n", ipMaskV4);
    k += snprintf(composebuff + k, buffsize - k, "<GateWay>%s</GateWay>\n", GatewayV4);
    k += snprintf(composebuff + k, buffsize - k, "<Dns1>8.8.8.8</Dns1>\n");
    k += snprintf(composebuff + k, buffsize - k, "<Dns2>4.4.4.4</Dns2>\n");

    k += snprintf(composebuff + k, buffsize - k, "</%s>\n", ifname);

//    closesocket(skfd);

    return k;
}

static int web_discovery_compose_service(char *composebuff, unsigned int buffsize)
{
    int k = 0;
    unsigned char tmp[4] = {0};
    unsigned int services = 0;

#if 0
    FILE *fp = fopen("/dev/servicestatus", "r");
    if (fp)
    {
        fscanf(fp, "%d", &services);
        fclose(fp);
        fp = NULL;
    }
#endif

    if(g_ovfs_web->enable_T8S)
    {
        services = 255 | (1 << 17) | (1 << 16);

    }
    else
    {

        services = 255 | (1 << 15) | (1 << 14);

    }

    if(g_ovfs_web->discovery.support_new_upgrade)
    {
        services |= (1 << 13);// support new upgrade api
    }

    tmp[0] = (unsigned int)(services & 0xff);
    tmp[1] = (unsigned int)((services >> 8) & 0xff);
    tmp[2] = (unsigned int)((services >> 16) & 0xff);
    tmp[3] = (unsigned int)((services >> 24) & 0xff);


    //k += snprintf(composeBuffer + k, bufferSize - k, "<ServiceAbility Attribute=\"%d\" Attribute2=\"%d\" Attribute3=\"%d\" Attribute4=\"%d\"/>\n");
    k += snprintf(composebuff + k, buffsize - k, "<ServiceAbility Attribute=\"%d\" ", tmp[0]);

    if (tmp[1])
    {
        k += snprintf(composebuff + k, buffsize - k, "Attribute2=\"%d\" ", tmp[1]);
    }
    if (tmp[2])
    {
        k += snprintf(composebuff + k, buffsize - k, "Attribute3=\"%d\" ", tmp[2]);
    }
    if (tmp[3])
    {
        k += snprintf(composebuff + k, buffsize - k, "Attribute4=\"%d\" ", tmp[3]);
    }

    k += snprintf(composebuff + k, buffsize - k, "/>\n");

    return k;
}

static int web_discovery_compose_port(char *composebuff, unsigned int buffsize)
{
    int k = 0;
    OVFS_PORT_ALL_T service_port;

    memset(&service_port, 0, sizeof(OVFS_PORT_ALL_T));

    ovfs_web_get_port_all(&service_port);
    k += snprintf(composebuff + k, buffsize - k, "<PrivatePort>5050</PrivatePort>\n");
    int port_t;
    if (service_port.port_http > 0)
    {
        port_t = service_port.port_http;

        if(g_ovfs_web->enbale_http_redirect_to_https || (g_ovfs_web->enable_http == 0))
        {
            if (service_port.port_https > 0)
            {
                port_t = service_port.port_https;
            }
        }
        //LOGE("\n port_t:%d \n",port_t);
        k += snprintf(composebuff + k, buffsize - k, "<HttpPort>%d</HttpPort>\n", port_t);
    }
    if (service_port.port_https > 0)
    {
        k += snprintf(composebuff + k, buffsize - k, "<HttpsPort>%d</HttpsPort>\n", service_port.port_https);
    }
    if (port_t > 0)
    {
        k += snprintf(composebuff + k, buffsize - k, "<OnvifPort>%d</OnvifPort>\n", port_t);
    }
    if (service_port.port_rtsp > 0)
    {
        k += snprintf(composebuff + k, buffsize - k, "<RtspPort>%d</RtspPort>\n", service_port.port_rtsp);
    }
    if (service_port.port_rtmp > 0)
    {
        k += snprintf(composebuff + k, buffsize - k, "<RtmpPort>%d</RtmpPort>\n", service_port.port_rtmp);
    }
    //add ipctype ,nvr auto login mark
    k += snprintf(composebuff + k, buffsize - k, "<IpcType>Ovfs</IpcType>\n");
    //k += snprintf(composebuff + k, buffsize - k, "<SubType>NULL</SubType>\n");

    char tempbuf[32]={0};
    sprintf(tempbuf,"%s",g_ovfs_web->DeviceTypeString + 4);
    supper(tempbuf);
    k += snprintf ( composebuff + k, buffsize - k, "<SubType>%s</SubType>\n" ,tempbuf);

    return k;
}

static int ovfs_web_discovery_set_network(char *devName, char *s_ip, char *s_netmask,
        char *s_gateway,
        char *s_mac)
{
    int ret = 0;
    cJSON_Struct *header = NULL;
    cJSON_Struct *lowerData = NULL;
    char mac[32] = {};

    GetDevInfo(devName, mac, NULL, NULL);
    LOGD("mac:%s\n", mac);
    LOGW("\n%s \n%s\n", mac, s_mac);
    if(0 != sncaselesscmp(mac, s_mac, slen(s_mac)))
        return WEB_CODE_InternalMistake;

    /*eth0 device*/
    if (strcmp(devName, "eth") != 0)
    {

        header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        lowerData =  Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

        Common_Json_SetAttrValue(lowerData, -1, "EthName", Common_Json_Type_String, devName, 0, 0);
        if(s_ip)
            Common_Json_SetAttrValue(lowerData, -1, "EnableDhcp", Common_Json_Type_Number, NULL, 0, 0);
            Common_Json_SetAttrValue(lowerData, -1, "IpAddrV4", Common_Json_Type_String, s_ip, 0, 0);

        if(s_netmask)
            Common_Json_SetAttrValue(lowerData, -1, "IpMaskV4", Common_Json_Type_String,
                                     s_netmask, 0, 0);

        if(s_gateway)
            Common_Json_SetAttrValue(lowerData, -1, "GatewayV4", Common_Json_Type_String,
                                     s_gateway, 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetAttr/Eth/0");
        //Ovfs_Web_RestMethodA(header, NULL, lowerData, 0);

        cJSON_Struct *pInParams = NULL;
        if (0 == ret)
        {
            if ((pInParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
            else
            {
                Common_Json_AddItem(pInParams, -1, "Header", header);
                Common_Json_AddItem(pInParams, -1, "Data", lowerData);
            }

        }
        if(ret == 0)
        {
            int TimeOut = 60000;
            cJSON_Struct *pOutResults = NULL;
            if(s_networkfuncCb.config != NULL)
            {
                LOGW("s_networkfuncCb.config!\n");
                s_networkfuncCb.config((void *)pInParams, (void **)&pOutResults);
            }
            else
            {
                ret = Module_CallFunctions((ModuleHandle_T)g_AccessHandle,pInParams,&pOutResults,TimeOut);
            }
            //LOGD("%s\n",Common_Json_Print(pInParams,NULL));
            //char *str = Common_cJSON_Print(pOutResults, NULL);
            //printf("---->ret:%d|result:%s\n",ret,str);
            if (pOutResults)
            {
                Common_Json_Delete(pOutResults);
                pOutResults = NULL;
            }

        }

        /*why do it?*/
        /*if((ret == 0) && s_ip)
        {
            SetNetDevInfo(devName, NULL, s_ip, NULL);
        }*/

        if(pInParams)
        {
            Common_Json_Delete(pInParams);
            pInParams = NULL;
        }

        if(ret != 0)
        {
            Common_Json_Delete(header);
            header = NULL;
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }

    }
    /*wlan0 device*/
    else if (strcmp(devName, "wlan") != 0)
    {
        header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        lowerData =  Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetAttr/Wifi/APList");
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        if (ret != 0)
        {
            Common_Json_Delete(header);
            Common_Json_Delete(lowerData);
            return ret;
        }

        cJSON_Struct *arryList = Common_Json_GetAttrValue(lowerData, -1, "ResList", NULL, NULL, NULL, NULL);

        cJSON_Struct *arry = Common_Json_GetAttrValue(arryList, 0, NULL, NULL, NULL, NULL, NULL);
        Common_Json_Delete(header);
        header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

        Common_Json_SetAttrValue(arry, -1, "IP", Common_Json_Type_String, s_ip, 0, 0);
        Common_Json_SetAttrValue(arry, -1, "NetMask", Common_Json_Type_String, s_netmask, 0, 0);
        Common_Json_SetAttrValue(arry, -1, "GateWay",  Common_Json_Type_String, s_gateway, 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/NetAttr/Wifi/AddAp");
        Ovfs_Web_RestMethodA(header, arry, NULL, 0);
        Common_Json_Delete(header);
        Common_Json_Delete(lowerData);

    }

    return ret;
}

static int ovfs_web_get_port_all(OVFS_PORT_ALL_T * p_server_port)
{
    int ret = 0;

    if (p_server_port == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }
    else
    {
        p_server_port->port_http = g_ovfs_web->httpport;
        p_server_port->port_https = g_ovfs_web->httpsport;
    }

    cJSON_Struct *header = NULL;
    if (0 == ret)
    {
        if ((header = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    cJSON_Struct *pResult = NULL;
    if (0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Onvif/Port");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(pResult, -1, "Port", NULL, NULL, &p_server_port->port_onvif, NULL);
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(pResult, -1, "Rtsp.RtspPort", NULL, NULL, &p_server_port->port_rtsp, NULL);
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(pResult, -1, "Rtmp.RtmpPort", NULL, NULL, &p_server_port->port_rtmp, NULL);
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (header)
    {
        Common_Json_Delete(header);
        header = NULL;
    }

    return ret;
}
#endif

static int OnvifUdpCreateSocket(ONVIF_UDP_CONTEXT_T *ct, char *ip, char *dev)
{
    struct sockaddr_in peeraddr;
    int sockfd = 0;
    unsigned int socklen = 0;
    struct ip_mreq mreq;

    /* ���� socket ����UDPͨѶ */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        LOGE("socket creating err in udptalk\n");
        return -1;
    }

    /* ����Ҫ�����鲥�ĵ�ַ */
    bzero(&mreq, sizeof(struct ip_mreq));
    // group = gethostbyname(ct->addr);
    // bcopy((void *) group->h_addr, (void *) &ia, group->h_length);

    /* �������ַ */
    // bcopy(&ia, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));
    mreq.imr_multiaddr.s_addr = inet_addr(ct->addr);
    /* ���÷����鲥��Ϣ��Դ�����ĵ�ַ��Ϣ */
    // mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_interface.s_addr = inet_addr(ip);

    /* �ѱ��������鲥��ַ��������������Ϊ�鲥��Ա��ֻ�м���������յ��鲥��Ϣ */
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                   sizeof(struct ip_mreq)) == -1)
    {
        LOGE("setsockopt failed %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    unsigned char loop = 0;
    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    int reuseaddr = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    struct ifreq ifr = {};
    strcpy(ifr.ifr_name, dev);
    setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

    int ttl = 255;
    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl));

    socklen = sizeof(struct sockaddr_in);
    memset(&peeraddr, 0, socklen);
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(ct->port);

    /* ���Լ��Ķ˿ں�IP��Ϣ��socket�� */
    if (bind(sockfd, (struct sockaddr *) &peeraddr,
             sizeof(struct sockaddr_in)) == -1)
    {
        LOGE("Bind error %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    return sockfd;
}

static int OnvifMultiAddrCreateAndListen(ONVIF_UDP_CONTEXT_T *ct)
{

    struct ifaddrs *ifAddrStruct = NULL, *ifAddrStructSrc = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStructSrc);
    ifAddrStruct = ifAddrStructSrc;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            LOGD("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            if (strcmp(addressBuffer, "127.0.0.1") == 0)
            {
                ifAddrStruct = ifAddrStruct->ifa_next;
                continue;
            }
            UTILS_UDP_SOCK_NODE_T *node = (UTILS_UDP_SOCK_NODE_T *)Common_Calloc(1, sizeof(UTILS_UDP_SOCK_NODE_T), __FUNCTION__, __LINE__);
            node->sockfd = 0;
            snprintf(node->ipAddr, sizeof(node->ipAddr), "%s", addressBuffer);
            snprintf(node->devName, sizeof(node->devName), "%s", ifAddrStruct->ifa_name);
            node->sockfd = OnvifUdpCreateSocket(ct, addressBuffer, ifAddrStruct->ifa_name);
            if (node->sockfd < 0)
                Common_Free(node,__func__,__LINE__);
            else
                Common_DList_InsertTail(ct->sockDevList, node, sizeof(UTILS_UDP_SOCK_NODE_T));
        }
        // else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
        // {   // check it is IP6
        //     // is a valid IP6 Address
        //     tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        //     char addressBuffer[INET6_ADDRSTRLEN];
        //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        //     printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        // }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    freeifaddrs(ifAddrStructSrc);
    return 0;
}

void *OnvifUdpThread(void *data)
{
    ONVIF_UDP_CONTEXT_T *ct = (ONVIF_UDP_CONTEXT_T *)data;

    Common_DList_Init(&ct->sockDevList, UdpSockFree);

    struct sockaddr_in peeraddr = {};
    int n = 0, ret = 0;
    unsigned int socklen = sizeof(peeraddr);

    prctl(PR_SET_NAME, __func__);

udp_multi_server:
    if (ct == NULL)
    {
        LOGE("onvif ct error \n");
        return NULL;
    }

    ret = OnvifMultiAddrCreateAndListen(ct);
    if (ret < 0)
    {
        Common_Sleep(1, 0);
        goto udp_multi_server;
    }

    unsigned long long int timeA = 0, timeB = 0;

    timeA = timeB = Common_GetSystemCount64();
    /* ѭ�����������������鲥��Ϣ */
    while (ct != NULL)
    {
        int retSelect = 0, sockfd = 0;
        char devName[32] = {};
        timeB = Common_GetSystemCount64();

        if (timeB - timeA > 1000LLU)
        {
            timeA = timeB;

            if (MultiAddrCheck(ct->sockDevList) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_DeleteAll(ct->sockDevList);
                goto udp_multi_server;
            }
        }

        retSelect = MultiAddrSelect(ct->sockDevList, &sockfd, devName);

        if (retSelect == 0)
        {
            continue;
        }
        else if (retSelect < 0)
        {
            LOGE("select error %s\n", strerror(errno));
            Common_DList_DeleteAll(ct->sockDevList);
            goto udp_multi_server;
        }
        else if (retSelect > 0)
        {
            bzero(ct->buf, ct->bufLen);
            n = recvfrom(sockfd, ct->buf, ct->bufLen, 0,
                         (struct sockaddr *) &peeraddr, &socklen);

            if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                LOGE("recvfrom err %s\n", strerror(errno));
                Common_DList_DeleteAll(ct->sockDevList);
                Common_Sleep(1, 0);
                goto udp_multi_server;
            }

            /* �ɹ����յ����ݱ� */
            if (n > ct->bufLen)
            {
                LOGE("recv buffer len was too big %d , buffer len is %d\n",
                     n, ct->bufLen);
                continue;
            }

            if (ct->cb == NULL)
            {
                LOGE("callback function is null\n");
                Common_DList_DeleteAll(ct->sockDevList);
                Common_Sleep(1, 0);
                goto udp_multi_server;
            }

            ct->buf[n] = 0;

            char ipAddr[32] = { 0 };
            snprintf(ipAddr, sizeof(ipAddr), "%s", inet_ntoa(peeraddr.sin_addr));
//LOGW("onvif discovery:\n");
//printf("[%s]\n\n",ct->buf);

            THIRD_UDP_USERDATA userdata = {0};
            userdata.UseMask = 0xf;
            Common_Strncpy(userdata.DevName, devName, strlen(devName));
            Common_Strncpy(userdata.RemoteIp, ipAddr, strlen(ipAddr));
            //LOGD("userdata:[%s][%s][%d][%d]\n",userdata.DevName,userdata.RemoteIp,slen(ct->buf),ct->bufLen);
            char *outBuf = NULL;
            int outSize = 0;
            ret = ct->cb((void *)&userdata, ct->buf,ct->bufLen,&outBuf,&outSize);
//LOGW("ret:[%d] outSize:[%d]\n",ret,outSize);
//            printf("[%s]\n\n",outBuf);

            if (ret == 0 && (access("/usr/etc/OnvifDiscovery_No_Response", F_OK)!=0))
            {
                // LOGD("recv from %s port %d dev name %s\n", inet_ntoa(peeraddr.sin_addr),
                // peeraddr.sin_port, devName);
                //Utils_SendtoMulticast(ct, ct->buf, strlen(ct->buf), devName, ipAddr);
                if (peeraddr.sin_port != 0)
                {
                    //LOGD("send to\n");
                    n = sendto(sockfd, outBuf, outSize, 0, (struct sockaddr *) &peeraddr,
                               socklen);
                }
            }

            free(outBuf);
            /* LOGD("send rsp len %d to %x port %d\n",n,peeraddr.sin_addr.s_addr,peeraddr.sin_port); */
        }
    }

    if (ct != NULL)
    {
        Common_DList_DeleteAll(ct->sockDevList);
        free(ct->buf);
        free(ct);
    }
    LOGW("discovery thread exit \n");
    return NULL;
}

static int HikUdpCreateSocket(HK_Ether_Cap *pEthCap,char *IfName, int ListenPort)
{
	int sockfd = 0;

	if (NULL == pEthCap || NULL == IfName || 0 == ListenPort)
	{
		return -1;
	}

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ListenPort));
	if (sockfd < 0)
	{
		return -1;
	}

	/* Set the network card in promiscuos mode */
	struct ifreq ethreq = {{{0}}};
	memset(&ethreq,0,sizeof(struct ifreq));
	memcpy(ethreq.ifr_name, IfName, IFNAMSIZ);
	if (ioctl(sockfd,SIOCGIFFLAGS,&ethreq)==-1)
	{
		perror("SIOCGIFFLAGS:");
		close(sockfd);
		return -1;
	}

	ethreq.ifr_flags|=IFF_PROMISC;
	if (ioctl(sockfd,SIOCSIFFLAGS,&ethreq)==-1)
	{
        perror("SIOCSIFFLAGS:");
		close(sockfd);
		return -1;
	}

	/* Attach the filter to the socket */
	struct sock_fprog Filter = {0};
	// tcpdump -dd ether proto 0x8033
	// 0x00000200ָ�����������ݳ���Ϊ512�ֽ�
	struct sock_filter BPF_code[]= {
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 0, 1, 0x00008033 },
		{ 0x6, 0, 0, 0x00000200 },
		{ 0x6, 0, 0, 0x00000000 }
	};
	//init filter settings
	Filter.len = 4;
	Filter.filter = BPF_code;
	if(setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER, &Filter, sizeof(Filter))<0)
	{
        perror("SO_ATTACH_FILTER:");
		close(sockfd);
		return -1;
	}

	struct ifreq ifr = {{{0}}};
    strncpy (ifr.ifr_name, IfName, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1)
	{
        perror("SIOCGIFINDEX\n");
		close(sockfd);
        return (-1);
    }
    int ifindex = ifr.ifr_ifindex;

	memset(&ifr,0,sizeof(struct ifreq));
    strncpy (ifr.ifr_name, "wlan0", sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1)
	{
        perror("SIOCGIFINDEX\n");
		//close(sockfd);
        //return (-1);
    }
    pEthCap->wlanidx = ifr.ifr_ifindex;;

	//set default value
	pEthCap->device = IfName;
	pEthCap->ifindex = ifindex;
	pEthCap->fd = sockfd;
	pEthCap->buffer = (u_int8_t *)malloc(1024);
	pEthCap->buf_len = 1024;

	LOGD("call, pEthCap->device=[%s],pEthCap->ifindex=[%d],pEthCap->fd=[%d]\n",pEthCap->device,pEthCap->ifindex,pEthCap->fd);
    return 0;
}

void *HikUdpThread(void *data)
{
    pthread_detach(pthread_self());

	int Ret = 0;
	int Recvlen = 0;
	int OutSize = 0;
	char *OutBuffer = NULL;
	char Ifname[] = "eth0";
	int ListenPort = 0x8033;
	fTHIRD_PROTOCOL_DealUdpPkg cb;
	HK_Ether_Cap sadpInst;
	struct sockaddr_ll sa;

	if (data == NULL)
    {
		LOGE("invalid param\n");

        return NULL;
    }

	LOGD("Hik Discovery thread run\n");

	cb = (fTHIRD_PROTOCOL_DealUdpPkg)data;

	Ret = HikUdpCreateSocket(&sadpInst, Ifname, ListenPort);

	if (Ret < 0)
	{
		LOGE("HikUdpCreateSocket error\n");

		return NULL;
	}

	memset(&sa, 0, sizeof(struct sockaddr_ll));
	sa.sll_family = AF_PACKET;
	sa.sll_ifindex = sadpInst.ifindex;
	sa.sll_protocol = htons(ETH_P_ALL);
    LOGE("Hik Discovery thread ifindex[%d][%d]\n",sadpInst.ifindex,sadpInst.wlanidx);
	while (g_ovfs_web->support_HK && g_ovfs_web->enable_HK)
	{
		Recvlen = -1;
		memset(sadpInst.buffer, 0, sadpInst.buf_len);

		Recvlen = recvfrom(sadpInst.fd, sadpInst.buffer, sadpInst.buf_len, 0, NULL, NULL);

		if (Recvlen < 0)
		{
			LOGE("Hik Discovery thread exit\n");
			close(sadpInst.fd);
			free(sadpInst.buffer);
			return NULL;
		}
		else if (0 == Recvlen)
		{
			continue;
		}

		OutBuffer = NULL;
		OutSize = 0;
		Ret = cb(NULL, sadpInst.buffer, Recvlen, &OutBuffer, &OutSize);

		if (Ret < 0 || NULL == OutBuffer || 0 == OutSize)
		{
			continue;
		}

        sa.sll_ifindex = sadpInst.ifindex;
		sendto(sadpInst.fd, OutBuffer, OutSize, 0, (struct sockaddr *)&sa, sizeof(sa));
        if(sadpInst.wlanidx>0)
        {
            sa.sll_ifindex = sadpInst.wlanidx;
		    sendto(sadpInst.fd, OutBuffer, OutSize, 0, (struct sockaddr *)&sa, sizeof(sa));
        }

		free(OutBuffer);
	}

    LOGD("Hik Discovery thread exit\n");
	close(sadpInst.fd);
	free(sadpInst.buffer);

	return NULL;
}
#if 0
static int tst_discovery_sendto_broadcaset(char *devName, int remoteIp,int port, unsigned char *outbuf, unsigned int outsize)
{
    int ret = 0;
    int iloop = 0;
    int optval = 0;
    int net_count = 0;
    struct ifreq ifr;
    int sockfd = INVALID_SOCKET;
    char if_name[32] = {0};
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr.s_addr = remoteIp;
    // ������ѯ
    net_count = QueryNetDevName(0, NULL);
    for (iloop = 0; iloop < net_count; iloop ++)
    {
        sockfd = INVALID_SOCKET;
        QueryNetDevName(iloop, if_name);
        if (strcmp(if_name, devName) != 0)
            continue;
        //LOGW("IF_NAME:%s\n", if_name);
        // 1.����socketfd
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        // 2.����socketfd
        optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval));
        optval = 1;
        setsockopt(sockfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&optval, sizeof(optval));
        sprintf(ifr.ifr_ifrn.ifrn_name, "%s", if_name);
        setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));
        // 3.��������

        sendto(sockfd, (char *)outbuf, outsize, 0,
               (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
        // 4.�ر�socketfd
        closesocket(sockfd);
        sockfd = INVALID_SOCKET;
    }

    return ret;
}

static int tst_discovery_server_request(char *devName, int remoteIp, unsigned char *inbuffer, unsigned int insize)
{
    int ret = 0;
    //int headertype = 0;
    unsigned char *outbuffer = NULL;
    unsigned int outsize = 1500;

	if (access("/tmp/updating", F_OK) == 0)
	{
		//is updating
		return 0;
	}

    outbuffer = Common_Calloc(1, 1500, __FUNCTION__, __LINE__);
    // ������Ӧ

    ret = tst_discovery_deal_request(devName, (const char *)inbuffer, insize, (char *)outbuffer, &outsize);

    if ((0 == ret) && (outsize > 0))
    {
        // ����
        if(g_ovfs_web->debugPrint == 2)
        {
            LOGW("send outbuffer:[%d][%s]\n",outsize,outbuffer);
        }

        tst_discovery_sendto_broadcaset(devName, remoteIp, 3002, outbuffer, outsize);
        tst_discovery_sendto_broadcaset(devName, INADDR_BROADCAST, 3002, outbuffer, outsize);
    }

    Common_Free(outbuffer, __FUNCTION__, __LINE__);

    return ret;
}

void *Fxn_TST_Discovery()
{
    pthread_detach(pthread_self());

    int result = 0;
    int recvlen = 0;
    int listenPort = 3001;
    unsigned char *lpBuffer = NULL;
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(struct sockaddr_in);
    COMMON_DLIST_T sockList;

    lpBuffer = Common_Calloc(1, 4096, __FUNCTION__, __LINE__);

    //unsigned long long int timeA = 0, timeB = 0;

    //timeA = timeB = Common_GetSystemCount64();

    ovfs_web_discovery_init(&sockList, listenPort);
    LOGD("sockList:[%p] [%d]\n",sockList,Common_DList_GetCount(sockList));

    while(g_ovfs_web->enable_TST)
    {
        //timeB = Common_GetSystemCount64();

        //if (timeB - timeA > 1000LLU)
        {
            //timeA = timeB;
            if (MultiAddrCheck(sockList) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_Uninit(&sockList);
                ovfs_web_discovery_init(&sockList, listenPort);
                continue;
            }
        }

        memset(lpBuffer, 0, 4096);
        int sockfd = 0;
        char devName[32] = {};
        result = MultiAddrSelect(sockList, &sockfd, devName);
        if (result < 0)
        {
            Common_DList_Uninit(&sockList);
            ovfs_web_discovery_init(&sockList, listenPort);
            LOGE("Select Error!\n");
            continue;
        }
        else if (result == 0)
        {
            continue;
        }
        else
        {
            recvlen = recvfrom(sockfd, (char *)lpBuffer, 4096, 0, (struct sockaddr *)&remote_addr,
                               &remote_addr_len);
            if (recvlen <= 8)
            {
                LOGE("Recv too short!\n");
                continue;
            }

             char ipAddr[32] = { 0 };
             snprintf(ipAddr, sizeof(ipAddr), "%s", inet_ntoa(remote_addr.sin_addr));
             LOGD("recv tst discovery from %s\n",ipAddr);

            if(g_ovfs_web->debugPrint == 2)
            {
                LOGW("lpBuffer:[%s]\n",lpBuffer);
            }

            tst_discovery_server_request(devName, remote_addr.sin_addr.s_addr, lpBuffer, recvlen);
        }
    }

    Common_DList_Uninit(&sockList);
    Common_Free(lpBuffer, __FUNCTION__, __LINE__);
    LOGD("TST Discovery thread exit\n");

    return NULL;
}
#endif
void *AudioBroadcastThread(void *data)
{
    pthread_detach(pthread_self());

    int i = 0;
    int talkhandle = 0;
    int timeout_count = 0;
    ONVIF_UDP_CONTEXT_T *ct = calloc(1,sizeof(ONVIF_UDP_CONTEXT_T));

    if (ct == NULL)
    {
        LOGE("ct error \n");
        return NULL;
    }

    snprintf(ct->addr, sizeof(ct->addr), g_ovfs_web->broadcast.ip);
    ct->port = g_ovfs_web->broadcast.port;
    ct->bufLen = 1024 * 2;
    ct->buf = calloc(1,ct->bufLen);

    Common_DList_Init(&ct->sockDevList, UdpSockFree);

    struct sockaddr_in peeraddr = {};
    int n = 0, ret = 0;
    unsigned int socklen = sizeof(peeraddr);

    prctl(PR_SET_NAME, __func__);

    talkhandle = talk_stream_open();

    if (talkhandle < 0)
    {
        LOGE("talk_stream_open error [%d]\n",talkhandle);
        return NULL;
    }

udp_multi_server:

    ret = OnvifMultiAddrCreateAndListen(ct);
    if (ret < 0)
    {
        Common_Sleep(1, 0);
        timeout_count++;
        goto udp_multi_server;
    }

    unsigned long long int timeA = 0, timeB = 0;

    timeA = timeB = Common_GetSystemCount64();
    /* ѭ�����������������鲥��Ϣ */
    while (g_ovfs_web->broadcast.status)
    {
        if(timeout_count >= 10)
        {
            LOGW("timeout! [%d]\n",timeout_count);
            g_ovfs_web->broadcast.status = 0;
            break;
        }
        int retSelect = 0, sockfd = 0;
        char devName[32] = {};
        timeB = Common_GetSystemCount64();

        if (timeB - timeA > 1000LLU)
        {
            timeA = timeB;

            if (MultiAddrCheck(ct->sockDevList) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_DeleteAll(ct->sockDevList);
                goto udp_multi_server;
            }
        }

        retSelect = MultiAddrSelect(ct->sockDevList, &sockfd, devName);

        if (retSelect == 0)
        {
            timeout_count++;
            continue;
        }
        else if (retSelect < 0)
        {
            LOGE("select error %s\n", strerror(errno));
            Common_DList_DeleteAll(ct->sockDevList);
            timeout_count++;
            goto udp_multi_server;
        }
        else if (retSelect > 0)
        {
            bzero(ct->buf, ct->bufLen);
            n = recvfrom(sockfd, ct->buf, ct->bufLen, 0,
                         (struct sockaddr *) &peeraddr, &socklen);

            if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                LOGE("recvfrom err %s\n", strerror(errno));
                Common_DList_DeleteAll(ct->sockDevList);
                Common_Sleep(1, 0);
                timeout_count++;
                goto udp_multi_server;
            }

            /* �ɹ����յ����ݱ� */
            if (n > ct->bufLen)
            {
                LOGE("recv buffer len was too big %d , buffer len is %d\n",
                     n, ct->bufLen);
                continue;
            }

            //printf("[%d]\n",n);

            if(n < sizeof(Broadcast_header))            {

                LOGE("recv buffer len was too small, buffer len is %d\n",
                     n);
                continue;
            }

            if(timeout_count)
            {
                timeout_count = 0;
            }

            Broadcast_header *header = (Broadcast_header *)ct->buf;
            //LOGD("code:[%d] sn:[%s] version:[%d] frameCount:[%d]\n",header->code,header->sn,header->version,header->frameCount);
            if(header->code != 20230422)
            {
                LOGE("code:[%d]\n",header->code);
                continue;
            }

            if(Common_StrnCmp(header->sn, g_ovfs_web->broadcast.sn, sizeof(g_ovfs_web->broadcast.sn)))
            {
                LOGE("sn:[%s][%s]\n",header->sn,g_ovfs_web->broadcast.sn);
                continue;
            }

            int offset = sizeof(Broadcast_header);
            for(i=0; i<header->frameCount; i++)
            {
                //Ovfs_FrameHeader_T *ants_header = (Ovfs_FrameHeader_T *)(ct->buf+offset);
                //LOGD("uiFrameType:[%d] uiFrameLen:[%d] cCodecId:[%d]\n",ants_header->uiFrameType,ants_header->uiFrameLen,ants_header->uMedia.struAudioHeader.cCodecId);
                talk_stream_write(talkhandle, ct->buf+offset + sizeof(Ovfs_FrameHeader_T), 320);
                offset += 356;
            }
        }
    }

    talk_stream_close(talkhandle);

    if (ct != NULL)
    {
        Common_DList_Uninit(&ct->sockDevList);
        free(ct->buf);
        free(ct);
    }
    LOGW("AudioBroadcastThread exit \n");
    return NULL;
}

