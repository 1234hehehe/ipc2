#include "libcommon_struct.h"
#include "libcommon_api.h"
#ifdef WIN32
#pragma comment(lib,"Iphlpapi.lib")
#endif
int Common_GetLocalNetInfo(char *ifname,int bIpv6,char **pIpAddress,char **pNetmask,char **pGateway,char **pMacString)  
{  
	int nRet = -1;
#ifdef WIN32 
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	/* variables used to print DHCP time info */
	struct tm newtime;
	char buffer[32];    
	errno_t error;


	pAdapterInfo = (IP_ADAPTER_INFO *) Common_Malloc( sizeof(IP_ADAPTER_INFO),0,__FUNCTION__,__LINE__);
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen);
	if (dwRetVal == ERROR_BUFFER_OVERFLOW) 
	{
		Common_Free(pAdapterInfo,__FUNCTION__,__LINE__);
		pAdapterInfo = (IP_ADAPTER_INFO *) Common_Malloc(ulOutBufLen,0,__FUNCTION__,__LINE__); 
		if (pAdapterInfo == NULL)
		{
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return -1;
		}
		dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen);
	}
		if (dwRetVal == NO_ERROR) 
		{
			pAdapter = pAdapterInfo;
			while (pAdapter)
			{
				if (NULL != strstr(pAdapter->Description,"Loopback") ||
					NULL != strstr(pAdapter->Description,"Virtual") ||
					NULL != strstr(pAdapter->Description,"Host-Only") ||
					NULL != strstr(pAdapter->GatewayList.IpAddress.String,"0.0.0.0"))
				{
					pAdapter=pAdapter->Next;
					continue;
				}
				IP_ADDR_STRING *lpCurAddress = NULL;
				lpCurAddress = &pAdapter->IpAddressList;
				while(lpCurAddress)
				{
					if (strcmp(lpCurAddress->IpAddress.String,"0.0.0.0") == 0)
					{
					}
					else
					{
						int ip1,ip2,ip3,ip4;
						ip1=ip2=ip3=ip4=0;
						if(4 == sscanf(lpCurAddress->IpAddress.String,"%d.%d.%d.%d",&ip1,&ip2,&ip3,&ip4))
						{
							if (ip4 < 2 || ip4 >= 255 || ip1 >= 255 || ip2 >= 255 || ip3 >= 255 || ip1 == 0)
							{
								
							}
							else
							{
								break;
							}
						}
						
					}
					lpCurAddress = lpCurAddress->Next;
				}
				S8 *pMac;
				if (pMacString != NULL)
				{
					S32 nMacPos = 0;
					pMac = (S8 *) Common_Malloc( pAdapter->AddressLength * 3 ,0,__FUNCTION__,__LINE__);
					if (pMac != NULL)
					{
						for (UINT i = 0; i < pAdapter->AddressLength; i++) 
						{
							if (i == (pAdapter->AddressLength - 1))
							{
								nMacPos += sprintf(pMac + nMacPos,"%02x",(int)pAdapter->Address[i]);
							}
							else
							{
								nMacPos += sprintf(pMac + nMacPos,"%02x:",(int)pAdapter->Address[i]);
							}
						}
						*pMacString = pMac;
					}
				}
				if (pIpAddress)
				{
					*pIpAddress = Common_StrDup(pAdapter->IpAddressList.IpAddress.String,__FUNCTION__,__LINE__);
				}
				if (pNetmask)
				{
					*pNetmask = Common_StrDup(pAdapter->IpAddressList.IpMask.String,__FUNCTION__,__LINE__);
				}
				if (pGateway)
				{
					*pGateway = Common_StrDup(pAdapter->GatewayList.IpAddress.String,__FUNCTION__,__LINE__);
				}
				
				break;
			}
		}
	
	
		if (pAdapterInfo)
			Common_Free(pAdapterInfo,__FUNCTION__,__LINE__);
		

#else
	int   fd;    
	struct   ifreq   ifr;     
	struct   sockaddr_in*   sin; 
	char *pString = NULL;
	fd   =   socket(AF_INET,   SOCK_STREAM,   0);   
	if(fd < 0)
	{	
		return -1;
	}
	memset(&ifr,   0x00,   sizeof(ifr));     
	strcpy(ifr.ifr_name,   ifname);   
	if (pIpAddress != NULL)
	{
		nRet = ioctl(fd,   SIOCGIFADDR,   &ifr);  
		if(nRet == 0)
		{
			sin   =   (struct   sockaddr_in*   )&ifr.ifr_addr;  
			pString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
			if (pString != NULL)
			{
				pString[64] = 0;
				inet_ntop(AF_INET, &sin->sin_addr, pString, 64);
				*pIpAddress = pString;
				pString = NULL;
			}
			
		}
	}
	if (pNetmask != NULL)
	{
		if( (ioctl( fd, SIOCGIFNETMASK, &ifr ) ) < 0 )     
		{    
			return -1;    
		}    

		sin = ( struct sockaddr_in * )&( ifr.ifr_netmask );  
		pString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
		if (pString != NULL)
		{
			pString[64] = 0;
			inet_ntop(AF_INET, &sin->sin_addr, pString, 64);
			*pNetmask = pString;
			pString = NULL;
		}

	}
	if (pMacString)
	{
		if( (ioctl( fd, SIOCGIFHWADDR, &ifr)) < 0)    
		{    
	        return -1;    
	    }    
		pString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
		if (pString != NULL)
		{
			sprintf(pString,"%02x:%02x:%02x:%02x:%02x:%02x",    
			(unsigned char)ifr.ifr_hwaddr.sa_data[0],    
			(unsigned char)ifr.ifr_hwaddr.sa_data[1],    
			(unsigned char)ifr.ifr_hwaddr.sa_data[2],    
			(unsigned char)ifr.ifr_hwaddr.sa_data[3],    
			(unsigned char)ifr.ifr_hwaddr.sa_data[4],    
			(unsigned char)ifr.ifr_hwaddr.sa_data[5]); 
			*pMacString = pString;
			pString = NULL;
		}
		
		   
		

	}
	
	close(fd);    
	if (pGateway != NULL)
	{
		FILE *fp;    
		char buf[512];    
		char cmd[128];    
		char *tmp;    
		pString = (char *)Common_Malloc(65,0,__FUNCTION__,__LINE__);
		if (pString != NULL)
		{
			pString[64] = 0;
			strcpy(cmd, "ip route");    
			fp = popen(cmd, "r");    
			if(NULL != fp)    
			{    
				while(fgets(buf, sizeof(buf), fp) != NULL)    
				{    
					tmp =buf;    
					while(*tmp && isspace(*tmp))    
						++ tmp;    
					if(strncmp(tmp, "default", strlen("default")) == 0)    
						break;    
				}    
				sscanf(buf, "%*s%*s%s", pString); 
				*pGateway = pString;
				pString = NULL;
				pclose(fp);    
			}    
			if (pString)
			{
				Common_Free(pString,__FUNCTION__,__LINE__);
				pString = NULL;
			}
		}

	}
	nRet = 0;
#endif
	return nRet;
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

