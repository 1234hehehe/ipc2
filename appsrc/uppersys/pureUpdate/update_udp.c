#include "libcommon_api.h"
#include "libupdate_api.h"
#include "update_struct.h"
#include "common_net.h"

#define UPDATE_UDP_BUFFSIZE  1600

static void UdpSockFree(void *data)
{
	UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *)data;

    if (node != NULL)
    {
    	if (node->sockfd > 0)
			close(node->sockfd);
    }
    free(node);
}

int deal_udp_data(int socketfd,char *devName,UpdateMgr_T *pMgr,U32 uFromIpv4,U16 uFromPort,S8 *pData,S32 nSize)
{
	PacketHeader_T *pHeader = (PacketHeader_T *)pData;
	UDPPacketExtHeader_T *pExtHeader  = (UDPPacketExtHeader_T *)(pHeader + 1);
	UpdateClientInfo_T *pInfo;
	S32 i,nFreeIndex = -1,nSec = 0,bNewPkt = 1;
	cJSON_Struct *pRecvJson = NULL,*pResultJson=NULL;
	if (pHeader->uStartCode != UPDATE_PACKET_STARTCODE)
	{
		return -1;
	}
	Common_GetSystemCount(&nSec,NULL);
	for (i = 0; i < UPDATE_UDP_CLIENT_NUM; i++)
	{
		if( NULL == pMgr->pClientInfo_Udp[i])
		{
			if (nFreeIndex == -1)
			{
				nFreeIndex = i;
			}
			continue;
		}
		if (pMgr->pClientInfo_Udp[i]->uClientIpv4 == uFromIpv4 &&
			pMgr->pClientInfo_Udp[i]->uClientPort == uFromPort)
		{
			if (pHeader->uExternHeaderLen == 0)
			{// 释放上一次未完成的包
				Common_Free(pMgr->pClientInfo_Udp[i]->pRecvBuffer,__FUNCTION__,__LINE__);
				Common_Free(pMgr->pClientInfo_Udp[i],__FUNCTION__,__LINE__);
				pMgr->pClientInfo_Udp[i] = NULL;
				pMgr->nClientInfoCount--;
				if (nFreeIndex == -1)
				{
					nFreeIndex = i;
				}
				continue;
			}

			pInfo = pMgr->pClientInfo_Udp[i];
			//nCurrIndex = i;
			U32 uNextSeq = pInfo->tHeader.uSeq + 1;
			uNextSeq &= 0x7FFF;
			if (pHeader->uSeq != uNextSeq)
			{// 丢包
				Common_Free(pMgr->pClientInfo_Udp[i]->pRecvBuffer,__FUNCTION__,__LINE__);
				Common_Free(pMgr->pClientInfo_Udp[i],__FUNCTION__,__LINE__);
				pMgr->pClientInfo_Udp[i] = NULL;
				if (nFreeIndex == -1)
				{
					nFreeIndex = i;
				}
				continue;
			}
			if (pExtHeader->uPktIdx + 1 == pExtHeader->uTotalPkt)
			{
				// 完整包
				pRecvJson = Common_Json_Parse(pInfo->pRecvBuffer,NULL,NULL);

				// 回调
				// 应答
				// 释放
				Common_Free(pMgr->pClientInfo_Udp[i]->pRecvBuffer,__FUNCTION__,__LINE__);
				Common_Free(pMgr->pClientInfo_Udp[i],__FUNCTION__,__LINE__);
				pMgr->pClientInfo_Udp[i] = NULL;
				pMgr->nClientInfoCount--;
				if (nFreeIndex == -1)
				{
					nFreeIndex = i;
				}
				bNewPkt = 0;
				continue;
			}
			else
			{
				memcpy(pInfo->pRecvBuffer + pInfo->nRecvLen,pData + pHeader->uExternHeaderLen + sizeof(PacketHeader_T),pHeader->nDataLen);
				pInfo->nRecvLen += pHeader->nDataLen;
				pInfo->uLastTime = nSec;
				bNewPkt = 0;
			}
			continue;
		}
		else if (pMgr->pClientInfo_Udp[i]->uLastTime + 60 < nSec)
		{
			// 释放
			Common_Free(pMgr->pClientInfo_Udp[i]->pRecvBuffer,__FUNCTION__,__LINE__);
			Common_Free(pMgr->pClientInfo_Udp[i],__FUNCTION__,__LINE__);
			pMgr->pClientInfo_Udp[i] = NULL;
			pMgr->nClientInfoCount--;
			if (nFreeIndex == -1)
			{
				nFreeIndex = i;
			}
		}
	}
	if (bNewPkt)
	{
		if (pHeader->uExternHeaderLen == 0)
		{// 单包
			UpdateClientInfo_T tInfo;
			memset(&tInfo,0,sizeof(UpdateClientInfo_T));
			tInfo.pMgr = pMgr;
			//tInfo.ifr_name = devName;
			pRecvJson = Common_Json_Parse(pData + pHeader->uExternHeaderLen + sizeof(PacketHeader_T),NULL,NULL);
			// 回调
			if (pMgr->fCallback != NULL &&
				0 == pMgr->fCallback(devName,&tInfo,pRecvJson,&pResultJson,pMgr->pCallbackUser))
			{
			}
		}
		else
		{
			// 多包，需要保存
			if (pExtHeader->uType != 1)
			{// 非udp ,不处理

			}
			else if (pExtHeader->uPktIdx != 0)
			{// 非首包

			}
			else if(nFreeIndex != -1)
			{
				pInfo = (UpdateClientInfo_T *)Common_Malloc(sizeof(UpdateClientInfo_T),0,__FUNCTION__,__LINE__);
				if (pInfo != NULL)
				{
					memset(pInfo,0,sizeof(UpdateClientInfo_T));
					pInfo->pMgr = pMgr;
					pInfo->pRecvBuffer = (S8 *)Common_Malloc(pExtHeader->uTotal,0,__FUNCTION__,__LINE__);
					if(pInfo->pRecvBuffer != NULL)
					{
						pInfo->nNeedLen = pExtHeader->uTotal;
						pInfo->tHeader = *pHeader;
						pInfo->uClientIpv4 = uFromIpv4;
						pInfo->uClientPort = uFromPort;
						pInfo->uLastTime = nSec;
						pInfo->tExtHeader = *pExtHeader;
						pMgr->pClientInfo_Udp[nFreeIndex] = pInfo;
						pMgr->nClientInfoCount++;
					}
					else
					{
						Common_Free(pInfo,__FUNCTION__,__LINE__);
						pInfo = NULL;
					}
				}
			}
		}
	}
	if (pResultJson != NULL)
	{
		S8 *pSendStr = NULL;
		S32 nSendLen = 0;
		S32 bBroadcast = 0;
		S8 *pSendType = NULL;
		Common_Json_GetAttrValue(pResultJson,-1,"/Header/SendType",NULL,&pSendType,NULL,NULL);
		if (pSendType != NULL && 0 == Common_StriCmp(pSendType,"Broadcast"))
		{
			bBroadcast = 1;
		}
		pSendStr = Common_Json_Print(pResultJson,&nSendLen);
		if (pSendStr != NULL)
		{
			S8 szIPv4[20];
			U32 uSeq = 0;

			if (bBroadcast)
			{
				if (!pMgr->bBroadcast)
				{
					S32  bBroadcast=TRUE;
                    S32 lRet = -1;
					lRet = setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST,(const char*)&bBroadcast,sizeof(S32));
                    if (lRet <= 0)
                    {
                        LOGE("setsockopt =%d.\n", lRet);
                        perror("setsockopt:");
                    }

                    pMgr->bBroadcast = 1;
				}

				sprintf(szIPv4,"255.255.255.255");
			}
			else
			{
				sprintf(szIPv4,"%d.%d.%d.%d",((uFromIpv4 >> 0) & 0xFF),((uFromIpv4 >> 8) & 0xFF),((uFromIpv4 >> 16) & 0xFF),((uFromIpv4 >> 24) & 0xFF));
			}
			(void)Update_udp_SendPkt(socketfd,szIPv4,ntohs(uFromPort),1,0,&uSeq,pSendStr,nSendLen+1);

		    Common_Free(pSendStr,__FUNCTION__,__LINE__);
			pSendStr = NULL;

            S8 *pUri = NULL;
            cJSON_Struct *pJsonOut = NULL;
            UpdateClientInfo_T tInfo;
            memset(&tInfo,0,sizeof(UpdateClientInfo_T));
            tInfo.pMgr = pMgr;

            if(Common_Json_GetAttrValueStr(pRecvJson, "/Header/Uri", &pUri))
            {
                if(pMgr->fCallback != NULL && 0 == Common_StriCmp(pUri,"/Update/SetMac"))
                {
                    pMgr->fCallback(devName,&tInfo,pRecvJson,&pJsonOut,pMgr->pCallbackUser);
                    Common_Json_Delete(pJsonOut);
                    pJsonOut = NULL;
                }
            }
		}
		Common_Json_Delete(pResultJson);
	}
	if (pRecvJson != NULL)
	{
		Common_Json_Delete(pRecvJson);
	}
	return 0;
}
int deal_udp_http_data(int socketfd,char *devName,UpdateMgr_T *pMgr,U32 uFromIpv4,U16 uFromPort,S8 *pData,S32 nSize)
{
    S32 lRet = -1;

	UpdateClientInfo_T *pInfo;

	cJSON_Struct *pRecvJson = NULL,*pResultJson=NULL;
	CommonHttpContext_T tHttpContext;
	Common_Http_Init(&tHttpContext);
	if(Common_Http_Parse(pData,nSize,&tHttpContext) <= 0)
	{
		Common_Http_Free(&tHttpContext);
		return -1;
	}
	if(Common_Http_Http2Json(&tHttpContext,&pRecvJson) != 0)
	{
		Common_Http_Free(&tHttpContext);
		Common_Json_Delete(pRecvJson);
		return -2;
	}
	if (pRecvJson == NULL)
	{
		Common_Http_Free(&tHttpContext);
		return -3;
	}
	Common_Http_Free(&tHttpContext);
	pInfo = (UpdateClientInfo_T *)Common_Malloc(sizeof(UpdateClientInfo_T),0,__FUNCTION__,__LINE__) ;
	if (pInfo == NULL)
	{
		Common_Json_Delete(pRecvJson);
		return -4;
	}
	memset(pInfo,0,sizeof(UpdateClientInfo_T));
	pInfo->pMgr = pMgr;
	//pInfo->ifr_name = devName;
	if (pMgr->fCallback != NULL &&
		0 == pMgr->fCallback(devName,pInfo,pRecvJson,&pResultJson,pMgr->pCallbackUser))
	{
	}

    Common_Free(pInfo,__FUNCTION__,__LINE__);
	pInfo = NULL;
	if (pResultJson != NULL)
	{
		S8 *pSendStr = NULL;
		S32 nSendLen = 0;
		S32 bBroadcast = 0;
		S8 *pSendType = NULL;

        Common_Http_Init(&tHttpContext);
		if(Common_Http_Json2Http(pResultJson,&tHttpContext) == 0)
		{
			pSendStr = Common_Http_Http2String(&tHttpContext,&nSendLen);
		}

        Common_Json_GetAttrValue(pResultJson,-1,"/Header/SendType",NULL,&pSendType,NULL,NULL);
		if (pSendType != NULL && 0 == Common_StriCmp(pSendType,"Broadcast"))
		{
			bBroadcast = 1;
		}

        if (pSendStr != NULL)
		{
			S8 szIPv4[20];
			struct sockaddr_in addr;
			// 1.创建socketfd
        	//int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

			// 2.配置socketfd
			//struct ifreq ifr;
			//sprintf(ifr.ifr_ifrn.ifrn_name, "%s", if_name);
			//setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

			LOGD("bBroadcast=%d\n",bBroadcast);
			if (bBroadcast)
			{
				int ret;
				int optval = 1;
				ret = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &optval,sizeof(int));
				if(ret!=0)
				{
					printf("setsockopt SO_BROADCAST error:%d, %s\n", errno, strerror(errno));
					//close(sockfd);
					return -5;
				}
				ret = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(int));
				if(ret!=0)
				{
					//close(sockfd);
					return -6;
				}

				sprintf(szIPv4,"255.255.255.255");
			}
			else
			{
				sprintf(szIPv4,"%d.%d.%d.%d",((uFromIpv4 >> 0) & 0xFF),((uFromIpv4 >> 8) & 0xFF),((uFromIpv4 >> 16) & 0xFF),((uFromIpv4 >> 24) & 0xFF));
			}

			// 3.发送数据
			memset(&addr,0,sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(szIPv4);
			addr.sin_port = uFromPort;
			LOGD("sendto socketfd=%d,szIPv4=%s\n",socketfd,szIPv4);
			lRet = sendto(socketfd,(const char *)pSendStr,nSendLen,0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
            if (lRet <= 0)
            {
                LOGE("nRet = %d %d<%s>\n", lRet, GetLastError(), strerror(GetLastError()));
				return -7;
            }
			// 4.关闭socketfd
        	//close(sockfd);

		}
		Common_Http_Free(&tHttpContext);
		Common_Json_Delete(pResultJson);
	}

    S8 *pUri = NULL;
    cJSON_Struct *pJsonOut = NULL;

    if(Common_Json_GetAttrValueStr(pRecvJson, "/Header/Uri", &pUri))
    {
        if(pMgr->fCallback != NULL && 0 == Common_StriCmp(pUri,"/Update/SetMac"))
        {
            pMgr->fCallback(devName,pInfo,pRecvJson,&pJsonOut,pMgr->pCallbackUser);
            Common_Json_Delete(pJsonOut);
            pJsonOut = NULL;
        }
    }
	if (pRecvJson != NULL)
	{
		Common_Json_Delete(pRecvJson);
	}
	return 0;
}

static S32 static_Module_Thread_Udp_Recv(Common_Thread_T hThreadHandle,void *pUserData)
{
	UpdateMgr_T *pMgr = (UpdateMgr_T *)pUserData;
	struct sockaddr_in srcaddr;
	S8 *pRecvBuffer = NULL;
	if (pMgr == NULL)
	{
		return -1;
	}

    if (pRecvBuffer == NULL)
	{
		pRecvBuffer = (S8 *)Common_Malloc(UPDATE_UDP_BUFFSIZE,0,__FUNCTION__,__LINE__);
	}

	unsigned long long int timeA = 0, timeB = 0;
    timeA = timeB = Common_GetSystemCount64();

    while (1)
	{
        timeB = Common_GetSystemCount64();

        if (timeB - timeA > 1000LLU)
        {
            timeA = timeB;
            if (MultiAddrCheck(pMgr->socketUdpList) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_DeleteAll(pMgr->socketUdpList);
				MultiAddrCreateAndListen(pMgr->socketUdpList,UDP,pMgr->nListenPort_Udp);
                continue;
            }
        }


        int sockfd = -1;
        char devName[32] = {};
        S32 selectResult = MultiAddrSelect(pMgr->socketUdpList, &sockfd, devName);
		if (selectResult < 0)
		{
		    //错误
			Common_Sleep(1,0);
			continue;
		}

        if (selectResult == 0)
		{
		    //超时
			continue;
		}

        if (sockfd != -1)
		{
			S32 nRet;
			S32 bHttp = 0;
			socklen_t fromlen = 0;

			if (pRecvBuffer == NULL)
			{
				pRecvBuffer = (S8 *)Common_Malloc(UPDATE_UDP_BUFFSIZE,0,__FUNCTION__,__LINE__);
			}

			if (pRecvBuffer == NULL)
			{
				Common_Sleep(1,0);
				continue;
			}

			bHttp = 0;
			fromlen = sizeof(struct sockaddr_in);
			nRet = recvfrom(sockfd,pRecvBuffer,UPDATE_UDP_BUFFSIZE,0,(struct sockaddr *)&srcaddr,&fromlen);
			if (nRet < 0)
			{
				LOGE("nRet = %d %d<%s>\n", nRet, GetLastError(), strerror(GetLastError()));
				Common_Sleep(1,0);
				continue;
			}
			else if (nRet == 0)
			{
				LOGW("recvfrom return 0!\n");
				Common_Sleep(1,0);
				continue;
			}
			if (nRet >= 4)
			{
			//	LOGD("sockfd=%d,devName=%s,recv %d bytes\n",sockfd,devName,nRet);
				if (*((U32 *)pRecvBuffer) != UPDATE_PACKET_STARTCODE)
				{
					// 可能是http
					bHttp = 1;
				}
			}

			if (bHttp)
			{
				deal_udp_http_data(sockfd,devName,pMgr,srcaddr.sin_addr.s_addr,srcaddr.sin_port,pRecvBuffer,nRet);
			}
			else
			{
				deal_udp_data(sockfd,devName,pMgr,srcaddr.sin_addr.s_addr,srcaddr.sin_port,pRecvBuffer,nRet);
			}
		}
	}
	Common_DList_DeleteAll(pMgr->socketUdpList);
    if (pRecvBuffer != NULL)
	{
		Common_Free(pRecvBuffer,__FUNCTION__,__LINE__);
		pRecvBuffer = NULL;
	}
	return 0;
}

S32 Update_Udp_Start_V2(UpdateMgr_T *pMgr)
{
	if (pMgr->nListenPort_Udp <= 0 ||
		pMgr->nListenPort_Udp >= 0xFFFF)
	{
		return -1;
	}
	Common_DList_Init(&pMgr->socketUdpList, UdpSockFree);
	MultiAddrCreateAndListen(pMgr->socketUdpList,UDP,pMgr->nListenPort_Udp);
	// 开启本地服务监听线程
	if(Common_Thread_Create(&pMgr->hThread_Udp,__FUNCTION__,0,COMMON_THREAD_CREATEFLAG_DETACH,static_Module_Thread_Udp_Recv,pMgr))
	{

		return -1;
	}
	return 0;
}

