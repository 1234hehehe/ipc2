#include "libcommon_struct.h"
#include "libcommon_api.h"
#include "common_net.h"

/*****************************************************************************
*  Name        : int Common_GetNetLinkStatus(char *ifname)
*  Description : Get net interface IFFLAGS
*  Params      : ifname	eg. eth0/eht1/wlan0
*  Returns     : fd
*  Author/date : Turkey /2019.9.4
*****************************************************************************/
int Common_GetNetLinkStatus(char *ifname)
{
    int fd = -1;
    int InterfaceFlags;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        printf("Cannot get control socket\n");
        close(fd);
        return -1;
    }
    else if( 0!=(ioctl(fd, SIOCGIFFLAGS, (char*)&ifr)) )
    {
       printf("Cannot get Network Interface Flags!\n");
       close(fd);
       return -2;
    }

    InterfaceFlags = ifr.ifr_flags;

    printf("<");
    if ( InterfaceFlags & IFF_UP)            printf("Network %s is UP, ", ifname);
    if ( InterfaceFlags & IFF_BROADCAST)     printf("Network %s is BCAST, ", ifname);
    if ( InterfaceFlags & IFF_MULTICAST)      printf("Network %s is MCAST, ", ifname);
    if ( InterfaceFlags & IFF_LOOPBACK)       printf("Network %s is LOOP, ", ifname);
    if ( InterfaceFlags & IFF_POINTOPOINT)   printf("Network %s is P2P, ", ifname);
    printf(">\n");

    if(!(InterfaceFlags & IFF_UP))
    {
        printf("%s is down\n", ifname);
        close(fd);
        return -1;
    }
    //close(fd);
    return  fd;

}


int Common_GetNetIP(int nSocket,char *ifname,char **pIPString)
{
	struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
	LOGD("ifname:%s\n",ifname);
    int ret = ioctl(nSocket,   SIOCGIFADDR,   &ifr);
    if (ret < 0)
    {
        printf("ioctl ip address error!\n");
        return -1;
    }
 	if(NULL == *pIPString)
		*pIPString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
	(*pIPString)[64] = 0;
    strcpy(*pIPString, inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));
	LOGD("GetIP:%s\n",*pIPString);
	return 0;
}

int Common_GetNetMAC(int nSocket,char *ifname,char **pMACString)
{
	struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    int ret = ioctl(nSocket,   SIOCGIFHWADDR,   &ifr);
    if (ret < 0)
    {
        printf("ioctl hwaddr error!\n");
        return -1;
    }

 	if(NULL == *pMACString)
		*pMACString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
 	(*pMACString)[64] = 0;
 	//strcpy(*pMACString, ether_ntoa((struct ether_addr *)ifr.ifr_hwaddr.sa_data));
	sprintf(*pMACString,"%02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned char)ifr.ifr_hwaddr.sa_data[0],
			(unsigned char)ifr.ifr_hwaddr.sa_data[1],
			(unsigned char)ifr.ifr_hwaddr.sa_data[2],
			(unsigned char)ifr.ifr_hwaddr.sa_data[3],
			(unsigned char)ifr.ifr_hwaddr.sa_data[4],
			(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	LOGD("GetMAC:%s\n",*pMACString);
	return 0;
}

int Common_GetNetMask(int nSocket,char *ifname,char **pMaskString)
{
	struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    int ret = ioctl(nSocket,   SIOCGIFNETMASK,   &ifr);
    if (ret < 0)
    {
        printf("ioctl net mask error!\n");
        return -1;
    }

 	if(NULL == *pMaskString)
		*pMaskString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
	(*pMaskString)[64] = 0;
    strcpy(*pMaskString, inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_netmask))->sin_addr));
	LOGD("GetMask:%s\n",*pMaskString);
	return 0;
}

struct route_info{
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
//收到内核的应答
        if((readLen = recv(sockFd, bufPtr, bufSize - msgLen, 0)) < 0)
        {
            perror("SOCK READ: ");
            return -1;
        }
        nlHdr = (struct nlmsghdr *)bufPtr;

//检查header是否有效
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


//分析返回的路由信息
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


int Common_GetGateway(char *devName,char **gateway)
{
    struct nlmsghdr *nlMsg;
    struct route_info *rtInfo;
    char *msgBuf = NULL;
    int sock, len, msgSeq = 0;

	if(NULL == *gateway)
			*gateway = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
	(*gateway)[64] = 0;

    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    {
        printf("Socket Creation: ");
        free(msgBuf);
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
        free(msgBuf);
		close(sock);
        return -1;
    }

    if((len = ReadNlSock(sock, msgBuf, 4096, msgSeq, getpid())) < 0)
    {
        free(msgBuf);
		close(sock);
        return -1;
    }

    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

    for(; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len))
    {
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(devName, nlMsg, rtInfo, *gateway);
    }

    free(rtInfo);
    free(msgBuf);
    close(sock);
    return 0;

}

int Common_GetNetworkInfo(char *ifname,int bIpv6,char **pIpAddress,char **pNetmask,char **pGateway,char **pMacString)
{
    int   fd;
    int ret = 0;
    if(NULL == ifname)
    {
        LOGE("ifname is null\n");
        return -1;
    }

    if(0 < (fd = Common_GetNetLinkStatus(ifname)))//如果存在网络设备，并且up状态。
    {
        int nCode = -1;
        cJSON_Struct *pInParam  = NULL;
        cJSON_Struct *pOutParam = NULL;

        pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Network/NetAttr/Eth/0",0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"get",0,0);

        Update_Tcp_Require("127.0.0.1", 10009, pInParam, &pOutParam, 3000);
        if (pOutParam != NULL)
    	{
            LOGD("pOutParam IS NOT NULL!\n");

            char *ptr = NULL;
            ptr = Common_Json_Print(pOutParam, NULL);
            LOGI("%s\n",ptr);
		    Common_Free(ptr, __FUNCTION__,__LINE__);

    		Common_Json_GetAttrValue(pOutParam,-1,"Header/Code",NULL,NULL,&nCode,NULL);
            LOGD("nCode:[%d]\n",nCode);
    		if (!(nCode == 0 || nCode == 200))
    		{
    			ret = -1;
    		}

            if(ret == 0)
            {
                *pIpAddress = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
                *pNetmask = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
                *pGateway = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
                *pMacString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
                char *str_tmp = NULL;
                Common_Json_GetAttrValue(pOutParam,-1,"Data/IpAddrV4",NULL,&str_tmp,NULL,NULL);
                *pIpAddress = Common_StrDup(str_tmp, __FUNCTION__, __LINE__);
                Common_Json_GetAttrValue(pOutParam,-1,"Data/IpMaskV4",NULL,&str_tmp,NULL,NULL);
                *pNetmask = Common_StrDup(str_tmp, __FUNCTION__, __LINE__);
                Common_Json_GetAttrValue(pOutParam,-1,"Data/GatewayV4",NULL,&str_tmp,NULL,NULL);
                *pGateway = Common_StrDup(str_tmp, __FUNCTION__, __LINE__);
                Common_Json_GetAttrValue(pOutParam,-1,"Data/MacAddr",NULL,&str_tmp,NULL,NULL);
                *pMacString = Common_StrDup(str_tmp, __FUNCTION__, __LINE__);


            }
    	}

        if(pInParam)
        {
            Common_Json_Delete(pInParam);
            pInParam = NULL;
        }

        if(pOutParam)
        {
            Common_Json_Delete(pOutParam);
            pOutParam = NULL;
        }

    }
    else
	{
		ret = 1;
	}
    LOGD("pIpAddress:[%s] pNetmask:[%s] pGateway:[%s] pMacString:[%s]\n",
                        *pIpAddress,*pNetmask,*pGateway,*pMacString);

    return ret;
}
int Common_GetLocalNetInfo(char *ifname,int bIpv6,char **pIpAddress,char **pNetmask,char **pGateway,char **pMacString)
{
	int   fd;
	int	  ret = 0;
	if(NULL == ifname)
	{
		LOGE("ifname is null\n");
		return -1;
	}

	if(0 < (fd = Common_GetNetLinkStatus(ifname)))//如果存在网络设备，并且up状态。
	{
		if(NULL != pIpAddress)
		{
			if(Common_GetNetIP(fd,ifname,pIpAddress) < 0)
			{
				LOGE("Common_GetNetIP failed\n");
				close(fd);
				return -2;
			}
		}
		if(NULL != pMacString)
		{
			if(Common_GetNetMAC(fd,ifname,pMacString) < 0)
			{
				LOGE("Common_GetNetMAC failed\n");
				close(fd);
				return -3;
			}
		}
		if(NULL != pNetmask)
		{
			if(Common_GetNetMask(fd,ifname,pNetmask) < 0)
			{
				LOGE("Common_GetNetMask failed\n");
				close(fd);
				return -4;
			}
		}
		close(fd);
	}
	else
	{
		ret = 1;
	}
	if (pGateway != NULL)
	{
		if(Common_GetGateway(ifname,pGateway) < 0)
		{
			LOGE("Common_GetGateway failed\n");
			return -5;
		}
	}
	return ret;
}

int Common_GetLocalIP(int nSocket,char **pIPString,int *nPort,int *bIPv6)
{
	//struct sockaddr_in addr;
	struct sockaddr_storage addr;
	char *pIpstr = NULL;
	socklen_t addrlen;
	int nRet;
	addrlen = sizeof(addr);

	if (nSocket < 0)
	{

		return -1;
	}
	nRet = getsockname(nSocket,(struct sockaddr *)&addr,&addrlen);
	if (nRet)
	{

		return -1;
	}

	pIpstr = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
	if (pIpstr == NULL)
	{
		return -1;
	}
	pIpstr[64] = 0;
	if (addr.ss_family == AF_INET)
	{
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		if (nPort)
		{
			*nPort = ntohs(s->sin_port);
		}
		inet_ntop(AF_INET, &s->sin_addr, pIpstr, 64);
		if (bIPv6)
		{
			*bIPv6 = 0;
		}
	}
	else
	{
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		if (nPort)
		{
			*nPort = ntohs(s->sin6_port);
		}
		inet_ntop(AF_INET6, &s->sin6_addr, pIpstr, 64);
		if (bIPv6)
		{
			*bIPv6 = 1;
		}

	}
	if (pIPString != NULL)
	{
		*pIPString = pIpstr;
		pIpstr = NULL;

	}
	if (pIpstr != NULL)
	{
		Common_Free(pIpstr,__FUNCTION__,__LINE__);
		pIpstr = NULL;
	}

	return 0;
}

U32 Common_IpAddr_IsValid(S8 *pIpv4_v6,S32 bIpv6)
{
	if(pIpv4_v6 == NULL)
	{
		return 0;
	}
	if (bIpv6)
	{
		struct in6_addr addr;
		if (0 == inet_pton(AF_INET6, pIpv4_v6, &addr))
		{
			return 0;
		}
		else
		{
			return 1;
		}

	}
	else
	{
		struct in_addr addr;
		if (0 == inet_pton(AF_INET, pIpv4_v6, &addr))
		{
			return 0;
		}
		else
		{
			return 1;
		}

	}
	return 0;
}


int Common_GetRemoteIP(int nSocket,char **pIPString,int *nPort,int *bIPv6)
{
	//struct sockaddr_in addr;
	struct sockaddr_storage addr;
	char *pIpstr = NULL;
	socklen_t addrlen;
	int nRet;
	addrlen = sizeof(addr);

	if (nSocket < 0)
	{

		return -1;
	}
	nRet = getpeername(nSocket,(struct sockaddr *)&addr,&addrlen);
	if (nRet)
	{

		return -1;
	}

	pIpstr = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
	if (pIpstr == NULL)
	{
		return -1;
	}
	pIpstr[64] = 0;
	if (addr.ss_family == AF_INET)
	{
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		if (nPort)
		{
			*nPort = ntohs(s->sin_port);
		}
		inet_ntop(AF_INET, &s->sin_addr, pIpstr, 64);
		if (bIPv6)
		{
			*bIPv6 = 0;
		}
	}
	else
	{
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		if (nPort)
		{
			*nPort = ntohs(s->sin6_port);
		}
		inet_ntop(AF_INET6, &s->sin6_addr, pIpstr, 64);
		if (bIPv6)
		{
			*bIPv6 = 1;
		}

	}
	if (pIPString != NULL)
	{
		*pIPString = pIpstr;
		pIpstr = NULL;

	}
	if (pIpstr != NULL)
	{
		Common_Free(pIpstr,__FUNCTION__,__LINE__);
		pIpstr = NULL;
	}

	return 0;
}


S32 UpdateOpenSocket(S32 bTcp,S32 nPort,char *dev,char *ip)
{
	struct sockaddr_in addr;
	S32 nRet;
	S32 nListenSocket = -1;

		// create socket
	    nListenSocket = socket(AF_INET, bTcp?SOCK_DGRAM:SOCK_STREAM, 0);
		if (nListenSocket == -1)
		{
			perror("error:");
			return -1;
		}

		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = bTcp?INADDR_ANY:inet_addr("127.0.0.1");
		addr.sin_port = htons(nPort);

		if(bTcp == UDP)
		{
			S32 val = 1;
			if(setsockopt(nListenSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
			{
				perror("error:");
				closeSocket(nListenSocket);
				nListenSocket = -1;
				return(-1);
			}

			if(setsockopt(nListenSocket,SOL_SOCKET,SO_BROADCAST,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
			{
				perror("error:");
				closeSocket(nListenSocket);
				nListenSocket = -1;
				return(-1);
			}
		}

        if(dev)
        {
            LOGW("bind device[%s] bTcp:[%d]\n",dev,bTcp);
    		// 绑定网卡
    		struct ifreq ifr = {};
    		strcpy(ifr.ifr_name, dev);
    		setsockopt(nListenSocket, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));
        }

		if (bind(nListenSocket, (struct sockaddr*)&addr, sizeof addr) != 0)
		{
			perror("error:");
			closeSocket(nListenSocket);
			nListenSocket = -1;
			return -1;
		}

		S32 curFlags = fcntl(nListenSocket, F_GETFL, 0);
		nRet =	fcntl(nListenSocket, F_SETFL, curFlags|O_NONBLOCK);
		if (nRet < 0 )
		{
			perror("error:");
			closeSocket(nListenSocket);
			nListenSocket = -1;
			return -1;
		}


		if(bTcp == TCP)
		{
			//开始监听
			if (listen(nListenSocket, 20) < 0)
			{
				perror("error:");
				closeSocket(nListenSocket);
				nListenSocket = -1;
				return -1;
			}
		}
	return nListenSocket;
}

static int UdpSockSearchByName(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;
    UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *)a;
    if (strcmp(node->devName, (char *)b) == 0)
        return 0;

    return -1;
}


int MultiAddrCreateAndListen(COMMON_DLIST_T sockList,int type, int port)
{

    struct ifaddrs *ifAddrStruct = NULL,*pifAddrSrc = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
	pifAddrSrc = ifAddrStruct;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *)malloc(sizeof(UTILS_SOCKET_NODE_T));
            node->sockfd = 0;
            snprintf(node->ipAddr, sizeof(node->ipAddr), "%s", addressBuffer);
            snprintf(node->devName, sizeof(node->devName), "%s",ifAddrStruct->ifa_name);
            node->sockfd = UpdateOpenSocket(type,port, ifAddrStruct->ifa_name, node->ipAddr);
			LOGD("%s sockfd=%d,type=%d,port=%d,IPV4 Address %s\n", ifAddrStruct->ifa_name,node->sockfd,type,port, addressBuffer);
            if (node->sockfd < 0)
                free(node);
            else
                Common_DList_InsertTail(sockList, node, sizeof(UTILS_SOCKET_NODE_T));
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
	freeifaddrs(pifAddrSrc);
    return 0;
}

int MultiAddrCheck(COMMON_DLIST_T sockList)
{
    struct ifaddrs *ifAddrStruct = NULL,*pifAddrSrc = NULL;
    void *tmpAddrPtr = NULL;
    int cnt = 0;
    getifaddrs(&ifAddrStruct);
	pifAddrSrc = ifAddrStruct;

    while (ifAddrStruct != NULL)
    {
        if ((NULL != ifAddrStruct->ifa_addr) && (ifAddrStruct->ifa_addr->sa_family == AF_INET))
        {
            // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //LOGD("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);

            if (Common_DList_Search(sockList, ifAddrStruct->ifa_name,UdpSockSearchByName) == NULL)
            {
            	freeifaddrs(pifAddrSrc);
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

	freeifaddrs(pifAddrSrc);
    if (cnt != Common_DList_GetCount(sockList))
        return 1;

    return 0;
}

int MultiAddrSelect(COMMON_DLIST_T sockList, int *sockfd, char *devName)
{
    fd_set rfds;
    int retSelect = 0;
    struct timeval timeout = { 0, 0 };
    FD_ZERO(&rfds);
    int i = 0, cnt = 0, lsock = 0;
    cnt = Common_DList_GetCount(sockList);
    for (i = 0; i < cnt; i++)
    {
        UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *) Common_DList_GetNode(sockList, i);

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
            UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *) Common_DList_GetNode(sockList, i);

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



#define UPDATE_HTTP_GET_PROCESS_RSP_STR "HTTP/1.1 200\r\nServer: nginx\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\nConnection: keep-alive\r\nCache-Control: no-cache\r\nPragma: no-cache\r\nExpires: -1\r\nStrict-Transport-Security: max-age=31536000\r\nX-Frame-Options: SAMEORIGIN\r\nX-Content-Type-Options: nosniff\r\nX-XSS-Protection: 1; mode=block\r\n\r\n\
%x\r\n\
%s\r\n\
0\r\n\r\n"

#define UPDATE_HTTP_GET_PROCESS_JSON_STR "{\"Result\":%d,\"Data\":{\"Progress\":%d}}"

typedef struct
{
    int httpState;
    int httpPort;
    pthread_t handlePth;
    COMMON_DLIST_T sockList;
}UPDATE_COMMON_HTTP_T;

/*extern this function from pure_update_main.c*/
extern int GetUpdateUpdatingStatus(int *partial, int *progress, int *status);
extern int GetHttpPort(int *port);

UPDATE_COMMON_HTTP_T s_http_ct;

static void SockNodeFree(void *data)
{
    if (data != NULL)
    {
        UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *)data;
        if (node->sockfd)
        {
            close(node->sockfd);
            node->sockfd = 0;
        }
    }
    free(data);
}

static int HttpMsgHandle(int sock, char *buf, int len)
{
    /* LOGE("buf=@%s@\n",buf); */
    int partial = -1, progress = -1, status = -1;
    int ret = 0;
    char *rspStr = (char *)malloc(512);
    char *jsonStr = (char *)malloc(128);
    if (strstr(buf, "GetProgress") != NULL ||
        strstr(buf, "frmUpgradeProgress") != NULL)
    {
        ret = GetUpdateUpdatingStatus(&partial, &progress, &status);
        LOGW("partial %d, progress %d status %d\n", partial, progress, status);
        if (ret == 0)
        {
            if (partial == 7)
                progress = 100;
            else if (status == -1)
                progress = -1;
        }
        snprintf(jsonStr, 128, UPDATE_HTTP_GET_PROCESS_JSON_STR, ret, progress);
        snprintf(rspStr, 512, UPDATE_HTTP_GET_PROCESS_RSP_STR, (unsigned int)strlen(jsonStr),
                 jsonStr);

        ret = write(sock,rspStr, strlen(rspStr));
        /* LOGW("sock rsp ret %s\n",rspStr); */
    }

	free(rspStr);
	rspStr = NULL;

	free(jsonStr);
	jsonStr = NULL;

    return 0;
}

static void *HttpMsgHandleThread(void *data)
{
#define HTTP_RECV_BUF 1024
    UPDATE_COMMON_HTTP_T *ct = (UPDATE_COMMON_HTTP_T *)data;
    int  listenfd = 0, connfd = 0;
    struct sockaddr_in  servaddr = {};
    char buff[HTTP_RECV_BUF + 4] = {};
    int  n = 0, port = 0;

http_multi_server:
    if (ct == NULL || ct->httpState == 0)
    {
        LOGE("http state error \n");
        return NULL;
    }

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    GetHttpPort(&port);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (port != 0)
        servaddr.sin_port = htons(port);
    else
        servaddr.sin_port = htons(80);

    const int on=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        LOGE("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(listenfd);
        sleep(1);
        goto http_multi_server;
    }

    if( listen(listenfd, 10) == -1)
    {
        LOGE("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(listenfd);
        goto http_multi_server;
    }

    LOGW("create and listen port %d\n", port);

    while(ct != NULL && ct->httpState == 1)
    {
        fd_set rfds;
        int retSelect = 0;
        struct timeval timeout = { 0, 0 };
        FD_ZERO(&rfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_SET(listenfd, &rfds);
        retSelect = select(listenfd + 1, &rfds, NULL, NULL, &timeout);
        if (retSelect > 0)
        {
            if (FD_ISSET(listenfd, &rfds))
            {
                if( (connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1)
                {
                    LOGE("accept socket error: %s(errno: %d)", strerror(errno), errno);
                    continue;
                }
                n = recv(connfd, buff, HTTP_RECV_BUF, 0);
                if(n>=0 && n < sizeof(buff))
                {
                    buff[n] = 0;
                    HttpMsgHandle(connfd, buff, n);
                }
                close(connfd);
            }
        }
        else if (retSelect < 0)
        {
            close(listenfd);
            goto http_multi_server;
        }
    }

    close(listenfd);
    return 0;
}

S32 CreateAndListenHttp()
{
    if (s_http_ct.httpState == 1)
    {
        LOGE("state error\n");
        return -1;
    }

    Common_DList_Init(&s_http_ct.sockList, SockNodeFree);
    pthread_create(&s_http_ct.handlePth, NULL, HttpMsgHandleThread, (void *)&s_http_ct);

    s_http_ct.httpState = 1;
    return 0;
}


S32 DestroyHttp()
{
    if (s_http_ct.httpState == 0)
    {
        LOGE("state error\n");
        return -1;
    }
    Common_DList_Uninit(&s_http_ct.sockList);
    s_http_ct.httpState = 0;
    return 0;
}
