#include "ovfs_comm_tool.h"

#define HW_ADDR_BUF_LEN 32
#define IPV4_ADDR_BUF_LEN 32

void pHx(unsigned char* p,int len)
{
    printf("Hex: ");
    for(int i=0; i<len; i++)
    {
        printf("%02X:",p[i]);
    }
    printf("\b\n");
}


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
static void parseRoutes(char *devName, struct nlmsghdr *nlHdr, struct route_info *rtInfo, char * ifname,char *gateway)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;
    //struct in_addr dst;
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

    //dst.s_addr = rtInfo->dstAddr;
    //printf("if:%s\n", rtInfo->ifName);
    sprintf(ifname, rtInfo->ifName);
    gate.s_addr = rtInfo->gateWay;
    sprintf(gateway, (char *)inet_ntoa(gate));
    //printf("gw:%s\n", gateway);
    gate.s_addr = rtInfo->srcAddr;
    //printf("src:%s\n", (char *)inet_ntoa(gate));
    gate.s_addr = rtInfo->dstAddr;
    //printf("dst:%s\n", (char *)inet_ntoa(gate));

    return;
}


int NetWorkTool_GetGateway(char *devName,char *gateway)
{
    if(NULL == gateway)
        return -1;

    struct nlmsghdr *nlMsg;
    struct route_info *rtInfo;
    char *msgBuf = NULL;
    char ifname[16] = {0};
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
        free(msgBuf);
        return -1;
    }

    if((len = ReadNlSock(sock, msgBuf, 4096, msgSeq, getpid())) < 0)
    {
        free(msgBuf);
        return -1;
    }

    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

	int nRet = -1;
	char acBuf[128];
    for(; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len))
    {
    	memset(acBuf, 0, sizeof(acBuf));
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(devName, nlMsg, rtInfo, ifname, acBuf);

        if((0 == strcmp(devName,ifname)) && (0 != strcmp(acBuf,"0.0.0.0")))
        {
        	strcpy(gateway,acBuf);
            LOGD("devName=%s,ifname=%s,gateway=%s\n",devName,ifname,gateway);
			nRet = 0;
            break;
        }
    }
    //LOGW("devName=%s,ifname=%s,gateway=%s\n",devName,ifname,gateway);

    free(rtInfo);
    free(msgBuf);
    close(sock);

    return nRet;

}
/*****************************************************************************
*  Name        : int NetWorkTool_GetIFFLAGS(char *NetDev )
*  Description : Get net interface IFFLAGS
*  Params      : NetDev
*  Returns     : 0--OK
*  Author/date : Turkey
*****************************************************************************/
int NetWorkTool_GetIFFlags( char *ifname )
{
    int fd = -1;
    int InterfaceFlags;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        LOGE("Cannot get control socket\n");
        close(fd);
        return -1;
    }
    else if( 0!=(ioctl(fd, SIOCGIFFLAGS, (char*)&ifr)) )
    {
        LOGE("Cannot get Network Interface Flags!\n");
        close(fd);
        return -1;
    }

    InterfaceFlags = ifr.ifr_flags;

    printf("<");
    if ( InterfaceFlags & IFF_UP)            printf("Network %s is UP, ", ifname);
    if ( InterfaceFlags & IFF_BROADCAST)     printf("Network %s is BCAST, ", ifname);
    if ( InterfaceFlags & IFF_MULTICAST)     printf("Network %s is MCAST, ", ifname);
    if ( InterfaceFlags & IFF_LOOPBACK)      printf("Network %s is LOOP, ", ifname);
    if ( InterfaceFlags & IFF_POINTOPOINT)   printf("Network %s is P2P, ", ifname);
    printf(">\n");
    close(fd);
    return  0;
}


int NetWorkTool_GetMacAddr(char *ifname, char *mac)
{
    int fd, rtn;
    struct ifreq ifr;

    if( !ifname || !mac )
    {
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

    if ( (rtn = ioctl(fd, SIOCGIFHWADDR, &ifr) ) == 0 )
    {
        //memcpy(    mac, (unsigned char *)ifr.ifr_hwaddr.sa_data, 6);
        //pHx((unsigned char*)ifr.ifr_hwaddr.sa_data,sizeof(ifr.ifr_hwaddr.sa_data));
        snprintf(mac,HW_ADDR_BUF_LEN, "%02X:%02X:%02X:%02X:%02X:%02X", //以太网MAC地址的长度是48位
                 (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    }
    else
        LOGE("ioctl %s\n", strerror(errno));
    close(fd);
    return rtn;
}

int NetWorkTool_SetMac(char *if_name, char *mac)
{
    char system_cmd[128];
    int ret = 0;

    if (NULL == if_name || NULL == mac)
    {
        LOGE("if_name=%d,mac=%d\n",if_name,mac);
        return -1;
    }

    snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s down", if_name);
    ret = Common_System(system_cmd);
    if (0 != ret)
    {
        return -1;
    }
	Common_Sleep(1, 0);
    snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s hw ether %s", if_name, mac);
    ret = Common_System(system_cmd);
    if (0 != ret)
    {
        return -1;
    }
    snprintf(system_cmd, sizeof(system_cmd), "ifconfig %s up", if_name);
    ret = Common_System(system_cmd);
    if (0 != ret)
    {
        return -1;
    }
	Common_Sleep(3, 0);

    return ret;
}


int NetWorkTool_SetMacAddr(char *ifname, char *mac)
{
    int fd, rtn;
    struct ifreq ifr;

    if( !ifname || !mac )
    {
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }
    ifr.ifr_addr.sa_family = ARPHRD_ETHER;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );
    memcpy((unsigned char *)ifr.ifr_hwaddr.sa_data, mac, 6);

    if ( (rtn = ioctl(fd, SIOCSIFHWADDR, &ifr) ) != 0 )
    {
        LOGE("SIOCSIFHWADDR %s\n", strerror(errno));
    }
    close(fd);
    return rtn;
}

int NetWorkTool_GetIPAddr(char *ifname, char *ipv4)
{
    int fd,rtn;
    struct ifreq ifr;

    if( !ifname || !ipv4 )
    {
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

    if ((rtn = ioctl(fd, SIOCGIFADDR, &ifr)) == 0)
    {
        struct sockaddr_in *myaddr;
        myaddr = (struct sockaddr_in *) &ifr.ifr_addr;
        memcpy(ipv4, inet_ntoa(myaddr->sin_addr), IPV4_ADDR_BUF_LEN);
    }
    else
    {
        //LOGE("ret=%d(%s)\n", rtn, strerror(errno));
    }
    close(fd);
    return rtn;
}


int NetWorkTool_SetIPAddr(char *ifname, char *ipv4)
{
    int fd, rtn;
    struct ifreq ifr;

    if( !ifname || !ipv4 )
    {
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

    struct sockaddr_in *addr;
    addr = (struct sockaddr_in *) & (ifr.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ipv4);
    if ((rtn = ioctl(fd, SIOCSIFADDR, &ifr)) < 0)
    {
        LOGE("ret=%d(%s)\n", rtn, strerror(errno));
    }
    close(fd);
    return rtn;
}

int NetWorkTool_GetMaskAddr(char *ifname, char *netmask)
{
    int fd,rtn;
    struct ifreq ifr;

    if( !ifname || !netmask )
    {
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

    if ((rtn = ioctl(fd, SIOCGIFNETMASK, &ifr)) >= 0)
    {
        struct sockaddr_in *myaddr;
        myaddr = (struct sockaddr_in *) &ifr.ifr_netmask;
        memcpy(netmask, inet_ntoa(myaddr->sin_addr), IPV4_ADDR_BUF_LEN);
    }
    else
    {
        LOGE("ret=%d(%s)\n", rtn, strerror(errno));
    }

    close(fd);
    return rtn;
}


int NetWorkTool_SetMaskAddr(char *ifname, char *netmask)
{
    int fd, rtn;
    struct ifreq ifr;

    if( !ifname || !netmask )
    {
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

    struct sockaddr_in *addr;
    addr = (struct sockaddr_in *) & (ifr.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(netmask);
    if ((rtn = ioctl(fd, SIOCSIFNETMASK, &ifr)) < 0)
    {
        LOGE("ret=%d(%s)\n", rtn, strerror(errno));
    }

    close(fd);
    return rtn;
}

/* μ??±??úμ?acμ?·o?pμ?· */
int NetWorkTool_MacIP ( const char *device,char *mac,char *ip )
{
    int sockfd;
    struct ifreq req;
    struct sockaddr_in * sin;

    if ( ( sockfd = socket ( PF_INET,SOCK_DGRAM,0 ) ) ==-1 )
    {
        fprintf ( stderr,"Sock Error:%s\n\a",strerror ( errno ) );
        return ( -1 );
    }

    memset ( &req,0,sizeof ( req ) );
    strcpy ( req.ifr_name,device );
    if ( ioctl ( sockfd,SIOCGIFHWADDR, ( char * ) &req ) ==-1 )
    {
        fprintf ( stderr,"ioctl SIOCGIFHWADDR:%s\n\a",strerror ( errno ) );
        close ( sockfd );
        return ( -1 );
    }
    memcpy ( mac,req.ifr_hwaddr.sa_data,6 );

    req.ifr_addr.sa_family = PF_INET;
    if ( ioctl ( sockfd,SIOCGIFADDR, ( char * ) &req ) ==-1 )
    {
        fprintf ( stderr,"ioctl SIOCGIFADDR:%s\n\a",strerror ( errno ) );
        close ( sockfd );
        return ( -1 );
    }
    sin = ( struct sockaddr_in * ) &req.ifr_addr;
    memcpy ( ip, ( char * ) &sin->sin_addr,4 );
    close(sockfd);
    return ( 0 );
}


int NetWorkTool_GetIfAddr(char *ifname, char *ip, char *netmask,int *family)
{
    struct ifaddrs * ifap0=NULL,*ifap=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifap0);
    ifap=ifap0;
	*family = -1;

    while (ifap!=NULL)
    {
        if(ifap->ifa_addr == NULL)
        {
            ifap=ifap->ifa_next;
            continue;
        }
        if (ifap->ifa_addr->sa_family==AF_INET)  // check it is IP4
        {
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifap->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            tmpAddrPtr=&((struct sockaddr_in *)ifap->ifa_netmask)->sin_addr;
            char netmaskBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, netmaskBuffer, INET_ADDRSTRLEN);
            if(strcmp(addressBuffer,"127.0.0.1")!=0)
            {
                //printf("%s IPv4: %s\n", ifap->ifa_name, addressBuffer);
                //printf("%s NetMask: %s\n", ifap->ifa_name, netmaskBuffer);
                if(strcmp(ifname,ifap->ifa_name)==0)
                {
                    *family = 1;
                    strncpy(ip,addressBuffer,INET_ADDRSTRLEN);
                    strncpy(netmask,netmaskBuffer,INET_ADDRSTRLEN);
                }
            }
        }
        else if (ifap->ifa_addr->sa_family==AF_INET6)  // check it is IP6
        {
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifap->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            tmpAddrPtr=&((struct sockaddr_in *)ifap->ifa_netmask)->sin_addr;
            char netmaskBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, netmaskBuffer, INET6_ADDRSTRLEN);
            if(strcmp(addressBuffer,"::")!=0)
            {
                printf("%s IPv6: %s\n", ifap->ifa_name, addressBuffer);
                printf("%s NetMask: %s\n", ifap->ifa_name, netmaskBuffer);
                if(strcmp(ifname,ifap->ifa_name)==0)
                {
                    *family = 0;
                    strncpy(ip,addressBuffer,INET6_ADDRSTRLEN);
                    strncpy(netmask,netmaskBuffer,INET6_ADDRSTRLEN);
                }
            }
        }
        ifap=ifap->ifa_next;
    }
    if (ifap0)
    {
        freeifaddrs(ifap0);
        ifap0 = NULL;
    }
    return 0;
}

//获取DHCP的地址，将获取出来的ip,netmask,getway回传出去.
int   NetWorkTool_Get_Dhcpc_Result(const char *if_name,char *ip,char *netmask,char *gateway)
{
    int ret = 0;
    char cmd[128]= {0},buf[64]= {0},conflie[128] = {0};
    if((NULL == ip) || (NULL == netmask) || (NULL == gateway))
    {
        return -1;
    }

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc_ip_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return -1;
	}

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc_resolv_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return -1;
	}

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc_route_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return -1;
	}

    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc_ip_%s.conf | awk \'{print $2}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(ip, strlen(buf), "%s", buf);
        LOGD("Get ip={%s}\n",ip);
    }
    else
        ret = -1;


    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc_ip_%s.conf | awk \'{print $4}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(netmask, strlen(buf), "%s", buf);
        LOGD("Get netmask={%s}\n",netmask);
    }
    else
        ret = -1;

    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc_route_%s.conf | awk \'{print $2}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(gateway, strlen(buf), "%s", buf);
        LOGD("Get gateway={%s}\n",gateway);
    }
    else
        ret = -1;

    return ret;
}

int NetWorkTool_Get_Dhcpc6_Result(const char *if_name,char *ip,int *prefixlen,char *gateway)
{
    int ret = 0;
    char cmd[128]= {0},buf[64]= {0},conflie[128] = {0};
    if((NULL == ip) || (NULL == prefixlen) || (NULL == gateway))
    {
        return -1;
    }

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc6_ip_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return -1;
	}

	snprintf(conflie,sizeof(conflie),"/tmp/udhcpc6_resolv_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return -1;
	}

	/*snprintf(conflie,sizeof(conflie),"/tmp/udhcpc6_route_%s.conf", if_name);
	if(0 != access(conflie, 0))
	{
		return -1;
	}*/

    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc6_ip_%s.conf | awk \'{print $2}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(ip, strlen(buf), "%s", buf);
        LOGD("Get ip={%s}\n",ip);
    }
    else
        ret = -1;


    snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc6_ip_%s.conf | awk \'{print $3}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        LOGD("buf:[%s]\n",buf);
        *prefixlen = atoi(buf);
        LOGD("Get prefixlen={%d}\n",*prefixlen);
    }
    else
        ret = -1;

    /*snprintf(cmd,sizeof(cmd),"cat /tmp/udhcpc6_route_%s.conf | awk \'{print $2}\'", if_name);
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        snprintf(gateway, strlen(buf), "%s", buf);
        LOGD("Get gateway={%s}\n",gateway);
    }
    else
        ret = -1;*/

    return ret;
}

int NetWorkTool_IfUpDown(char *ifname, int flag)
{
    int fd, rtn;
    struct ifreq ifr;

    if (!ifname)
    {
        return -1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        LOGE("socket %s\n", strerror(errno));
        return -1;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ - 1 );

    if ( (rtn = ioctl(fd, SIOCGIFFLAGS, &ifr) ) == 0 )
    {
        if ( flag == DOWN )
            ifr.ifr_flags &= ~IFF_UP;
        else if ( flag == UP )
            ifr.ifr_flags |= IFF_UP;

    }

    if ( (rtn = ioctl(fd, SIOCSIFFLAGS, &ifr) ) != 0)
    {
        LOGE("SIOCSIFFLAGS %s\n", strerror(errno));
    }

    close(fd);

    return rtn;
}

int NetWorkTool_SetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask)
{
    int ret = 0;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        ret = -1;
    }
    else
    {
        strncpy(ifr.ifr_name, ifname, 16);

        if (hwaddr)
        {
//            sscanf(hwaddr,"%02d:%02d:%02d:%02d:%02d:%02d",
//                &ifr.ifr_hwaddr.sa_data[0],
//                &ifr.ifr_hwaddr.sa_data[1],
//                &ifr.ifr_hwaddr.sa_data[2],
//                &ifr.ifr_hwaddr.sa_data[3],
//                &ifr.ifr_hwaddr.sa_data[4],
//                &ifr.ifr_hwaddr.sa_data[5]);
//            if (ioctl(skfd, SIOCSIFHWADDR, &ifr) < 0)
//            {
//                ret = errno;
//                LOGE("ret=%d(%s)\n", ret, strerror(ret));
//                ret = -1;
//            }
        }

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
    }

    if (skfd >= 0)
    {
        close(skfd);
        skfd = -1;
    }

    return ret;
}

int NetWorkTool_GetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask)
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
                //pHx((unsigned char*)ifr.ifr_hwaddr.sa_data,sizeof(ifr.ifr_hwaddr.sa_data));
                snprintf(hwaddr,HW_ADDR_BUF_LEN, "%02X:%02X:%02X:%02X:%02X:%02X", //以太网MAC地址的长度是48位
                         (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                         (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                         (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                         (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                         (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                         (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
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
                memcpy(ipv4, inet_ntoa(myaddr->sin_addr), IPV4_ADDR_BUF_LEN);
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
                memcpy(netmask, inet_ntoa(myaddr->sin_addr), IPV4_ADDR_BUF_LEN);
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
    //LOGD("hwaddr[%p]=%s,hwaddr[0]=%02X\n",hwaddr,hwaddr,*hwaddr);
    return ret;
}

int NetWorkTool_SetDevInfoV6(char *ifname, char *ipv6, int prefixlen)
{
    int ret = 0;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    struct in6_ifreq ifr6;
    memset(&ifr6, 0, sizeof(struct in6_ifreq));

    int skfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        ret = -1;
    }
    else
    {
        strncpy(ifr.ifr_name, ifname, 16);
        ioctl(skfd, SIOCGIFINDEX, &ifr);
        ifr6.ifr6_ifindex = ifr.ifr_ifindex;
        ifr6.ifr6_prefixlen = prefixlen;
        if (ipv6)
        {
            inet_pton(AF_INET6, ipv6, &(ifr6.ifr6_addr));
            if (ioctl(skfd, SIOCSIFADDR, &ifr6) < 0)
            {
                printf("ret=%d(%s)\n", ret, strerror(ret));
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


int NetWorkTool_GetDevInfoV6(char *ifname, char *ipv6, int *prefixlen)
{
    FILE *fp;
    int ret = -1;
    char buf[256];
    char cmd[128];

    memset(buf,0,sizeof(buf));
    snprintf(cmd, sizeof(cmd), "ip -6 addr show %s | grep 'global' | awk '{print $2}'", ifname);
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return -1;
    }

    if(fgets(buf, sizeof(buf), fp) != NULL)
    {
        sscanf(buf,"%[^/]/%d",ipv6, prefixlen);
        LOGD("buf:[%s] ipv6:[%s] prefixlen:[%d]\n",buf,ipv6, *prefixlen);
    }

    if(strlen(buf) > 0)
        ret = 0;
    pclose(fp);
    return ret;
}

int NetWorkTool_GetSubNet(char *subnet, char *ipv4, char *netmask)
{
    int sub,ip,mask;
    if (subnet != NULL)
    {
        if ((ipv4 != NULL) && (netmask != NULL))
        {
            inet_pton(AF_INET,ipv4,(void *)&ip);
            inet_pton(AF_INET,netmask,(void *)&mask);
            sub = ip & mask;
            inet_ntop(AF_INET,(void *)&sub,subnet,16);
        }
    }
    return 0;
}

int NetWorkTool_GetDefaultRoute(char *gateWay)
{
    FILE *fp;
    int ret = -1;
    char buf[512];
    char cmd[128];

    memset(buf,0,sizeof(buf));
    snprintf(cmd, sizeof(cmd), "route | grep 'default' | awk '{print $2}'");
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return -1;
    }

    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        sscanf(buf,"%s\n",gateWay);
    }
    if(strlen(buf) > 0)
        ret = 0;
    pclose(fp);
    return ret;
}

int NetWorkTool_GetDefaultRouteV2(char *ethName, char *gateWay)
{
    FILE *fp;
    int ret = -1;
    char buf[512];
    char cmd[128];

    memset(buf,0,sizeof(buf));
    snprintf(cmd, sizeof(cmd), "ip route | grep 'default' | grep '%s' | awk '{print $3}'", ethName);
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return -1;
    }

    if(fgets(buf, sizeof(buf), fp) != NULL)
    {
        sscanf(buf,"%s\n",gateWay);
    }
    if(strlen(buf) > 0)
        ret = 0;
    pclose(fp);
    return ret;
}

int NetWorkTool_AddDefaultRoute(char *ethName, char *gateWay)
{
    struct rtentry route; /* route item struct */
    struct sockaddr_in *addr;
    int skfd = 0;

    /* clear route struct by 0 */
    memset((char *) &route, 0x00, sizeof(route));

    /* default target is net (host)*/
    route.rt_flags = RTF_UP;

    ((struct sockaddr_in *) &route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *) &route.rt_genmask)->sin_family = AF_INET;

    addr = (struct sockaddr_in *) &route.rt_gateway;
    addr->sin_family = AF_INET;
    /* addr->sin_addr.s_addr = inet_addr(gateWay); */
    route.rt_flags |= RTF_GATEWAY;
    route.rt_dev = ethName;
    /* create a socket */
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        LOGE("socket error %s\n", strerror(errno));
        return -1;
    }

    /*clear last router*/
    if (ioctl(skfd, SIOCDELRT, &route) < 0)
    {
        LOGE("SIOCDELRT %s\n", strerror(errno));
//        close(skfd);
//        return -1;
    }

    /* clear route struct by 0 */
    memset((char *) &route, 0x00, sizeof(route));

    /* default target is net (host)*/
    route.rt_flags = RTF_UP;

    ((struct sockaddr_in *) &route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *) &route.rt_genmask)->sin_family = AF_INET;

    addr = (struct sockaddr_in *) &route.rt_gateway;
    addr->sin_family = AF_INET;
    /* addr->sin_addr.s_addr = inet_addr(gateWay); */
    route.rt_flags |= RTF_GATEWAY;
    route.rt_dev = ethName;

    /* add a route item */
    if (ioctl(skfd, SIOCADDRT, &route) < 0)
    {
        LOGE("SIOCADDRT %s\n", strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);

    return 0;
}

int NetWorkTool_DelDefaultRoute(char *ethName, char *gateWay)
{
    struct rtentry route; /* route item struct */
    struct sockaddr_in *addr;
    int skfd = 0;

    /* clear route struct by 0 */
    memset((char *) &route, 0x00, sizeof(route));

    /* default target is net (host)*/
    route.rt_flags = RTF_UP;

    ((struct sockaddr_in *) &route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *) &route.rt_genmask)->sin_family = AF_INET;

    addr = (struct sockaddr_in *) &route.rt_gateway;
    addr->sin_family = AF_INET;
    /* addr->sin_addr.s_addr = inet_addr(gateWay); */
    route.rt_flags |= RTF_GATEWAY;
    route.rt_dev = ethName;
    /* create a socket */
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        LOGE("socket error %s\n", strerror(errno));
        return -1;
    }

    /*clear last router*/
    if (ioctl(skfd, SIOCDELRT, &route) < 0)
    {
        LOGE("SIOCDELRT %s\n", strerror(errno));
//        close(skfd);
//        return -1;
    }

    /* clear route struct by 0 */
    memset((char *) &route, 0x00, sizeof(route));

    /* default target is net (host)*/
    route.rt_flags = RTF_UP;

    ((struct sockaddr_in *) &route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *) &route.rt_genmask)->sin_family = AF_INET;

    addr = (struct sockaddr_in *) &route.rt_gateway;
    addr->sin_family = AF_INET;
    /* addr->sin_addr.s_addr = inet_addr(gateWay); */
    route.rt_flags |= RTF_GATEWAY;
    route.rt_dev = ethName;

    /* add a route item */
    if (ioctl(skfd, SIOCADDRT, &route) < 0)
    {
        LOGE("SIOCADDRT %s\n", strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);

    return 0;
}

int NetWorkTool_UpdateDefaultRoute(char *ifname)
{

    LOGW("First is %s\n",ifname);

    return 0;
}

int NetWorkTool_GetDefaultRouteV6(char *ethName, char *gateWay)
{
    FILE *fp;
    int ret = -1;
    char buf[512];
    char cmd[128];

    memset(buf,0,sizeof(buf));
    snprintf(cmd, sizeof(cmd), "ip -6 route | grep 'default' | grep '%s' | awk '{print $3}'", ethName);
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return -1;
    }

    if(fgets(buf, sizeof(buf), fp) != NULL)
    {
        sscanf(buf,"%s\n",gateWay);

        LOGD("buf:[%s] gateWay:[%s]\n",buf,gateWay);
    }

    if(strlen(buf) > 0)
        ret = 0;
    pclose(fp);
    return ret;
}

int NetWorkTool_SetDefaultRouteV6(char *ethName, char *gateWay)
{
    int ret = -1;
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "ip -6 route del default dev %s;\
         ip -6 route add default via %s dev %s", ethName, gateWay, ethName);

    Common_System(cmd);

    return 0;
}

#define IPVERSION 4
#define ICMP_DATA_LEN 56

int nsent = 0;//发送的ICMP消息序号
int nrecv = 0;
char sendbuf[1024];          // 用来存放将要发送的ip数据包
struct sockaddr_in sockaddr, recvsock;

// 校验和生成
ushort checksum(unsigned char *buf, int len)
{
    unsigned int sum=0;
    unsigned short *cbuf;

    cbuf=(unsigned short *)buf;

    while(len>1)
    {
        sum+=*cbuf++;
        len-=2;
    }

    if(len)
        sum+=*(unsigned char *)cbuf;

    sum=(sum>>16)+(sum & 0xffff);
    sum+=(sum>>16);

    return ~sum;
}

// 计算时间差
float timesubtract(struct timeval *begin, struct timeval *end)
{
    int n;// 先计算两个时间点相差多少微秒
    n = ( end->tv_sec - begin->tv_sec ) * 1000000
        + ( end->tv_usec - begin->tv_usec );
    // 转化为毫秒返回
    return (float) (n / 1000);
}

unsigned long long monotonic_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

/*
Timestamp or Timestamp Reply Message

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |      Code     |          Checksum             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Identifier          |        Sequence Number        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Originate Timestamp                                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Receive Timestamp                                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Transmit Timestamp                                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

// 发送ping数据包
int packping(int sendsqe)
{
    struct icmp *icmp_hdr;  // icmp头部指针

    icmp_hdr = (struct icmp *)sendbuf;
    icmp_hdr->icmp_type = ICMP_ECHO;                     // 类型
    icmp_hdr->icmp_code = 0;                                    // 代码
    icmp_hdr->icmp_hun.ih_idseq.icd_id = getpid();      // 标示符
    icmp_hdr->icmp_hun.ih_idseq.icd_seq = sendsqe; // 序列号
    memset(icmp_hdr->icmp_data, 0, ICMP_DATA_LEN);
    //gettimeofday((struct timeval *)icmp_hdr->icmp_data, NULL);
    *(uint32_t*)&icmp_hdr->icmp_data = monotonic_us();

    int icmp_total_len = 8 + ICMP_DATA_LEN;

    icmp_hdr->icmp_cksum = 0;
    icmp_hdr->icmp_cksum = checksum((unsigned char *)(sendbuf),icmp_total_len); // 校验和

    return icmp_total_len;
}

int decodepack(char *buf, int len)
{
    struct iphdr *ip_hdr;
    struct icmp *icmp_hdr;
    int iph_lenthg;
//    float rtt; // 往返时间
    struct timeval end; // 接收报文时的时间戳

    ip_hdr = (struct iphdr *)buf;
    // ip头部长度
    iph_lenthg = ip_hdr->ihl<<2;

    icmp_hdr = (struct icmp *)(buf + iph_lenthg);

    // icmp报文的长度
    len -= iph_lenthg;
    if(len < 8)
    {
        fprintf(stderr, "Icmp package length less 8 bytes , error!\n");
        return -1;
    }

    // 确认是本机发出的icmp报文的响应
    if(icmp_hdr->icmp_type != ICMP_ECHOREPLY || icmp_hdr->icmp_hun.ih_idseq.icd_id != getpid())
    {
        fprintf(stderr, "Don't send to us!");
        return -1;
    }
    gettimeofday(&end, NULL);
	timesubtract((struct timeval *)&icmp_hdr->icmp_data, &end);
    //rtt = timesubtract((struct timeval *)&icmp_hdr->icmp_data, &end);
    //printf("Received %d bytes from %s, ttl = %d, rtt = %f ms, icmpseq = %d \n", len, inet_ntoa(recvsock.sin_addr),ip_hdr->ttl, rtt, icmp_hdr->icmp_seq);

    nrecv++;
    return 0;
}

//获取端口连通情况
int NetWorkTool_CheckConnect(char *if_name,char *ip)
{
    int sockaddr_len = sizeof(struct sockaddr);
    struct hostent *host;
    int sockfd=0;

    memset(&sockaddr, 0, sizeof(struct sockaddr));
    if((sockaddr.sin_addr.s_addr = inet_addr(ip)) == INADDR_NONE)
    {
        // 说明输入的主机名不是点分十进制,采用域名方式解析
        if((host = gethostbyname(ip)) == NULL)
        {
            fprintf(stderr, "ping %s , 未知的名称!\n", ip);
            return -1;
        }
        sockaddr.sin_addr = *(struct in_addr *)(host->h_addr);
    }
    sockaddr.sin_family = PF_INET;

    // 创建原始套接字 SOCK_RAW 协议类型 IPPROTO_ICMP
    if((sockfd = socket(PF_INET,SOCK_RAW,IPPROTO_ICMP)) == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    struct ifreq interface;
    if(if_name != NULL)
    {
        strncpy(interface.ifr_ifrn.ifrn_name, if_name, IFNAMSIZ);
        if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface)) < 0)
        {
            perror("SO_BINDTODEVICE failed");
            /* Deal with error... */
        }
    }


    // 发包操作
    //printf("%s PINGing %s %d data send.\n", if_name,ip, ICMP_DATA_LEN);
    int i = 1,ret = 0;
    int ping_time = 3;
    nsent = 0;//发送的ICMP消息序号
    nrecv = 0;
    int recvDataLen = 0;
    int sendDatalen = 0;
    char recvbuf[1024];

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd,&rfds);

    struct timeval tv; /* 申明一个时间变量来保存时间 */
    tv.tv_sec = 1;
    tv.tv_usec = 500; /* 设置select等待的最大时间为1秒加500微秒 */

    while(ping_time--)
    {
        int packlen = packping(i++);
        if((sendDatalen = sendto(sockfd, sendbuf, packlen,0, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) < 0)
        {
            fprintf(stderr, "send ping package %d error, %s\n", i, strerror(errno));
            continue ;
        }
        else
            nsent++;

        ret = select(sockfd+1, &rfds, NULL, NULL, &tv);
        if(ret < 0)
        {
            perror("select");
        }
        else if(ret == 0)
        {
            //printf("Request Timed Out\n");
            continue;
        }
        else
        {
            if (FD_ISSET(sockfd,&rfds))
            {
                if((recvDataLen = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&recvsock, (socklen_t *)&sockaddr_len)) == -1)
                {
                    //经socket接收数据,如果正确接收返回接收到的字节数，失败返回0.
                    if(errno==EINTR)  //EINTR表示信号中断
                        continue;
                    fprintf(stderr, "recvmsg error, %s\n", strerror(errno));
                    continue;
                }
            }
        }

        decodepack(recvbuf, recvDataLen);

    }
    if(nrecv > 0)
    {
        printf("\e[1;32m =====%s %s connect ok!===== \e[0m\n",ip,if_name);
		close(sockfd);
        return 1;
    }
    else
    {
        printf("\e[1;33m =====%s not connect to %s!===== \e[0m\n ",if_name,ip);
		close(sockfd);
        return 0;
    }
}



//获得公网IP
bool NetWorkTool_GetPublicIP(char *url)
{
    struct sockaddr_in pin;
    struct hostent *nlp_host;
    int sd = 0;
    int len = 0;
    char buf[512] = { 0 };
    char myurl[64] = {0};
    char host[64] = { 0 };
    char GET[64] =  {0};
    char header[240] =  {0};
    char *pHost = 0;
    char publicIP[30];

    ///get the host name and the relative address from url name!!!
    strcpy(myurl, url);
    for (pHost = myurl; *pHost != '/' && *pHost != '\0'; ++pHost) ;
    if ((unsigned int)(pHost - myurl) == strlen(myurl))
        strcpy(GET, "/");
    else
        strcpy(GET, pHost);
    *pHost = '\0';
    strcpy(host, myurl);


    ///setting socket param
    if ((nlp_host = gethostbyname(host)) == 0)
    {
        perror("error get host\n");
        return false;
    }

    bzero(&pin, sizeof(pin));
    pin.sin_family = AF_INET;
    pin.sin_addr.s_addr = htonl(INADDR_ANY);
    pin.sin_addr.s_addr = ((struct in_addr *)(nlp_host->h_addr))->s_addr;
    pin.sin_port = htons(80);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error opening socket!!!\n");
        return false;
    }

    ///together the request info that will be sent to web server
    ///Note: the blank and enter key byte is necessary,please remember!!!
    strcat(header, "GET");
    strcat(header, " ");
    strcat(header, GET);
    strcat(header, " ");
    strcat(header, "HTTP/1.1\r\n");
    strcat(header, "HOST:");
    strcat(header, host);
    strcat(header, "\r\n");
    strcat(header, "ACCEPT:*/*");
    strcat(header, "\r\nConnection: close\r\n\r\n\r\n");

    ///connect to the webserver,send the header,and receive the web sourcecode
    if (connect(sd, (void *)&pin, sizeof(pin)) == -1)
//    if (connect(sd, (sockaddr *)&pin, sizeof(pin)) == -1)
    {
        close(sd);
        printf("error connect to socket\n");
        return false;
    }

    if (send(sd, header, strlen(header), 0) == -1)
    {
        close(sd);
        perror("error in send \n");
        return false;
    }

    ///send the message and wait the response!!!
    len = recv(sd, buf, 512, 0);
    if (len < 0)
    {
        close(sd);
        printf("receive data error!!!\n");
        return false;
    }
    else
    {
        //printf("buf=%s\n",strstr(buf,"close")+9);
        memset(publicIP,0,sizeof(publicIP));
//buf中存储的数据并不只是公网IP，还有其它数据，需要将公网IP解析出来
//不同的环境中，数据格式可能不同，具体情况具体分析
        sscanf(strstr(buf,"close")+9,"%*[^\n]\n%[^\n]",publicIP);
        printf("\e[1;32m publicIP = %s \e[0m\n",publicIP);

        close(sd);
        return true;
    }
}


/*
static unsigned short calc_cksum(char *buff, int len)
{
    int blen = len;
    unsigned short *mid = (unsigned short *)buff;
    unsigned short te = 0;
    unsigned int sum = 0;

    while(blen > 1)
    {
        sum += *mid++;
        blen -= 2;
    }
    //数据长度为奇数比如65 上面的while是按16计算的 最后就会剩下一字节不能计算
    if(blen == 1)
    {
        //将多出的一字节放入short类型的高位 低8位置0 加入到sum中
        te = *(unsigned char *)mid;
        te  = (te << 8) & 0xff;
        sum += te;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return (unsigned short)(~sum);
}


static void icmp_packet(char *buff, int len, int id, int seq)
{
    struct timeval *tval = NULL;
    struct icmp *icmp = (struct icmp *)buff;

    icmp->icmp_type = 8; //ECHO REQUEST
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;  //first set zero
    icmp->icmp_id = id & 0xffff;
    icmp->icmp_seq = seq;

    tval = (struct timeval *)icmp->icmp_data;
    gettimeofday(tval, NULL); //获得传输时间作为数据

    //计算校验和
    icmp->icmp_cksum = calc_cksum(buff, len);
    return;
}

static int parse_packet(char *buff, int len)
{
    struct timeval *val;
    struct timeval nv;
    struct icmp *icmp;
    struct iphdr *iphead = (struct iphdr *)buff;
    struct in_addr addr;
    addr.s_addr = iphead->saddr;

    printf("comefrom ip=%s  ",inet_ntoa(addr));
    //跳过ip头
    icmp = (struct icmp *)(buff + sizeof(struct iphdr));

    //看传输回的包校验和是否正确
    if(calc_cksum((char *)icmp, len - sizeof(sizeof(struct iphdr))) > 1)
    {
        return -1;
    }
    gettimeofday(&nv, NULL);
    val = (struct timeval *)icmp->icmp_data;

     printf("type=%d  seq=%d id=%d pid=%d usec=%ld \n",
            icmp->icmp_type,icmp->icmp_seq,icmp->icmp_id,(getpid()&0xffff),
            nv.tv_usec - val->tv_usec);
    return 0;

}


static int ping_address(char *srcDev, uint32_t dstIp, int timeout, int cnt)
{
    int skfd;
    struct sockaddr_in addr = {0};
    struct sockaddr_in saddr = {0};
    char buff[64] = {0};
    char recvbuff[512] = {0};
    int ret = 0, isOk = -1;
    int addrlen = 0;
    int count = cnt;
    int i = 1;

    skfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(skfd < 0)
    {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dstIp;//inet_addr(dstIp);

    struct ifreq ifr = {};
    strcpy(ifr.ifr_name, srcDev);
    setsockopt(skfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

    //每一秒发送一次 共发送count次
    while(count > 0)
    {
        //序列号seq 从1 开始传输  buff的大小为64
        memset(buff, 0, sizeof(buff));
        icmp_packet(buff, 64, getpid(), i);
        i++;
        count --;

        //将数据发送出去
        ret = sendto(skfd, buff, 64, 0, (struct sockaddr *)&addr, sizeof(addr));
        if(ret <= 0)
        {
            break;
        }

        //接收echo replay
        memset(recvbuff, 0, sizeof(recvbuff));
        memset(&saddr, 0, sizeof(saddr));
        addrlen = sizeof(saddr);

        fd_set descriptors;
        struct timeval time_to_wait;

        FD_ZERO(&descriptors);
        FD_SET(skfd, &descriptors);
        time_to_wait.tv_sec = timeout / 1000;
        time_to_wait.tv_usec = timeout % 1000 * 1000;
        int return_value = select(skfd + 1, &descriptors, NULL, NULL, &time_to_wait);
        if (return_value < 0)
        {
            continue;
        }
        else if (!return_value)
        {
            continue;
        }

        ret = recvfrom(skfd, recvbuff, sizeof(recvbuff), 0, (struct sockaddr *)&saddr,
                       (socklen_t *)&addrlen);
        if(ret <= 0)
        {
            continue;
        }
        if (parse_packet(recvbuff, ret) == 0)
            isOk = 0;
    }

    close(skfd);
    return isOk;
}
*/
/*this strcut was copy from busybox-1.29.3/networking/udhcp/dhcpd.h */
typedef uint32_t leasetime_t;
typedef struct dyn_lease
{
    /* Unix time when lease expires. Kept in memory in host order.
     * When written to file, converted to network order
     * and adjusted (current time subtracted) */
    leasetime_t expires;
    /* "nip": IP in network order */
    uint32_t lease_nip;
    /* We use lease_mac[6], since e.g. ARP probing uses
     * only 6 first bytes anyway. We check received dhcp packets
     * that their hlen == 6 and thus chaddr has only 6 significant bytes
     * (dhcp packet has chaddr[16], not [6])
     */
    uint8_t lease_mac[6];
    char hostname[20];
    uint8_t pad[2];
    /* total size is a multiply of 4 */
} UDHCPD_LEASE_T;

static int load_udhcpd_lease(char *leaseFile, UDHCPD_LEASE_T **lease, int *leaseCnt)
{
    int fileSize = 0, leaseSize = 0, ret = 0;
    FILE *fp = fopen(leaseFile, "rb");

    if (fp == NULL)
        return -1;

    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);

    /*there is a uint64 header in lease file , please check the dhcpd.c in busybox for detail*/
    leaseSize = fileSize - 8;

    fseek(fp, 8, SEEK_SET);

    if (leaseSize % sizeof(UDHCPD_LEASE_T) != 0)
    {
        printf("lease file size not match\n");
        fclose(fp);
        return -1;
    }

    *leaseCnt = leaseSize / sizeof(UDHCPD_LEASE_T);
    *lease = (UDHCPD_LEASE_T *)malloc(*leaseCnt * sizeof(UDHCPD_LEASE_T));
    ret = fread(*lease, 1, leaseSize, fp);
    if (ret != leaseSize)
    {
        printf("load lease file size not match\n");
        free(*lease);
        *lease = NULL;
        *leaseCnt = 0;
        fclose(fp);
        return -1;
    }


    fclose(fp);
    return 0;
}

int NetWorkTool_GetIpaddrByMacFromUdhcpdLease(char *leaseFile, char *mac, char *ip)
{
    UDHCPD_LEASE_T *lease = NULL;
    int leaseCnt = 0, i = 0, ret = 0;
    if (access(leaseFile, F_OK) != 0)
        return -1;

    if (load_udhcpd_lease(leaseFile, &lease, &leaseCnt) < 0)
    {
        LOGE("load udhcpd lease filed failed\n");
        return -1;
    }


    for (i = 0; i < leaseCnt; i++)
    {
        char macStr[18] = {};
        snprintf(macStr,sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                 lease[i].lease_mac[0],
                 lease[i].lease_mac[1],
                 lease[i].lease_mac[2],
                 lease[i].lease_mac[3],
                 lease[i].lease_mac[4],
                 lease[i].lease_mac[5]);
        /* LOGE("macStr %s\n",macStr); */
        if (strncmp(mac, macStr, strlen(macStr)) == 0)
        {
            struct in_addr in = {};
            in.s_addr = lease[i].lease_nip;
            strcpy(ip,inet_ntoa(in));
            ret = 1;
            /* LOGE("done\n"); */
            break;
        }
    }


    /* for (i = 0; i < leaseCnt; i++) */
    /* { */
    /*     struct in_addr a; */
    /*     a.s_addr = lease[i].lease_nip; */
    /*     /\* printf("check ip %s\n", inet_ntoa(a)); *\/ */
    /*     /\* printf("%x\n",lease[i].lease_nip); *\/ */
    /*     ping_address(wifiDevName, lease[i].lease_nip, 200, 1); */
    /* } */


    free(lease);
    return ret;
}
