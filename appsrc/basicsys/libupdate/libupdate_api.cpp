#include <stdio.h>
#include "libcommon_api.h"
#include "libupdate_api.h"
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#include <Mstcpip.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include <direct.h>

#endif

#define UPDATE_MAKEFOURCC(ch0, ch1, ch2, ch3) ((U32)(U8)(ch0) | ((U32)(U8)(ch1) << 8) | ((U32)(U8)(ch2) << 16) | ((U32)(U8)(ch3) << 24 ))
#define UPDATE_PACKET_STARTCODE UPDATE_MAKEFOURCC('o','n','v','s')



typedef struct _tagPacketHeader
{
	U32 uStartCode;// 数据同步码 
	S32 nDataLen;// 数据长度.
	U32 uDataFormat:8;// 0- JSON
	U32 uExternHeaderLen:8;// 扩展头长度
	U32 uSeq:15;
	U32 bResp:1; // 0-请求 ,1-应答
	U32 uMsgId:16;// 整包消息编号，便于识别对应包
	U32 uRes:16;
}PacketHeader_T;

typedef struct _tagUDPPacketExtHeader
{
	U32 uType:8;// 1- udp
	U32 uRes:24;
	U32 uTotal;// 总长度
	U32 uTotalPkt:16;// 总包个数
	U32 uPktIdx:16; // 当前包索引 0- 开始
}UDPPacketExtHeader_T;

#define UPDATE_DEBUG printf
#define UPDATE_INFO printf
#define UPDATE_ERROR printf


//////////////////////////////////////////////////////////////////////////



S32 Update_Tcp_Require(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	struct sockaddr_in addr;
	S32 nRet;
	S32 selectResult;
	S32 nSocket = -1;
	S32 bTryConnect = 0,bWaitResult = 0;
	S32 nSendHeadPos = 0, nSendDataPos = 0,nSendDataSize;
	PacketHeader_T tHeader;
	char *pSendBuffer = NULL;
	char *pRecvBuffer = NULL;
	S32 nRecvNeedSize = 0;
	S32 nRecvPos = 0,nRecvHeadPos = 0;
	struct timeval tv_timeToDelay;
	fd_set readSet,writeSet,exceptSet;
	S32 nResult = -1;
#if 0
	if (pInParams != NULL)
	{
		S8 *pStrValue = NULL;
		UPDATE_DEBUG("\n[TCP]InParam<%s>\n",pStrValue = Common_Json_PrintUnformatted(pInParams,NULL));
		Common_Free(pStrValue,__FUNCTION__,__LINE__);
	}
#endif
	if (pDomain == NULL || (nPort < 0 || nPort >= 0x0FFFF))
	{
		UPDATE_DEBUG("Here\n");
		return -1;
	}


	// create socket
	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (nSocket == -1)
	{
		return -1;
	}
#ifdef WIN32
	unsigned long arg = 1;
	nRet =  ioctlsocket(nSocket, FIONBIO, &arg);
	if (nRet)
	{
		closeSocket(nSocket);
		nSocket = -1;
		return -1;
	}
	tcp_keepalive tkeepalive;
	DWORD dwBytesReturned;

	tkeepalive.onoff = 1;
	tkeepalive.keepalivetime = 60000;
	tkeepalive.keepaliveinterval = 15;
	if(WSAIoctl(nSocket,SIO_KEEPALIVE_VALS,&tkeepalive,sizeof(tkeepalive),NULL,0,&dwBytesReturned,NULL,NULL))
	{
		UPDATE_DEBUG("WSAIoctl failed\n");
	}
#else
	S32 curFlags = fcntl(nSocket, F_GETFL, 0);
	nRet =  fcntl(nSocket, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(nSocket);
		nSocket = -1;
		return -1;
	}

	S32 keepalive = 1; // 开启keepalive属性
	S32 keepidle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
	S32 keepinterval = 15; // 探测时发包的时间间隔为5 秒
	S32 keepcount = 4; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	setsockopt(nSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(nSocket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
	setsockopt(nSocket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	setsockopt(nSocket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));
#endif


	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(pDomain);
	addr.sin_port = htons(nPort);
	if (connect(nSocket,(struct sockaddr *)&addr,sizeof(addr))!= 0)
	{
		S32 err = GetLastError();
		if(err == EINPROGRESS || err == EWOULDBLOCK)
		{
			bTryConnect = 1;
		}
		else
		{
			closeSocket(nSocket);
			nSocket = -1;
			return -1;
		}


	}


	//
	tv_timeToDelay.tv_sec = nTimeOut/1000;
	tv_timeToDelay.tv_usec = (nTimeOut%1000) * 1000;  
	while(1)
	{
		if (bTryConnect)
		{// 正在连接中
			FD_ZERO(&writeSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&writeSet);
			FD_SET(nSocket,&exceptSet);
			tv_timeToDelay.tv_sec = nTimeOut/1000;
			tv_timeToDelay.tv_usec = (nTimeOut%1000) * 1000;  
			selectResult = select(nSocket + 1, NULL, &writeSet, &exceptSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误

				UPDATE_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,nSocket,GetLastError(),strerror(GetLastError()));
				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			if (selectResult == 0)
			{//超时

				break;
			}

			if (FD_ISSET(nSocket,&writeSet))
			{
				S32 nError,nRet;
				socklen_t nsize = sizeof(S32);
				nRet = getsockopt(nSocket, SOL_SOCKET, SO_ERROR, (char *)&nError, &nsize);
				if(nRet == 0 && nError == 0)
				{
					bTryConnect = 0;
					continue;
				}
				else
				{
					closeSocket(nSocket);
					nSocket = -1;
					break;
				}


			}

			if (FD_ISSET(nSocket,&exceptSet))
			{
				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			continue;
		}
		if (bWaitResult)
		{

			FD_ZERO(&readSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&readSet);
			FD_SET(nSocket,&exceptSet);

			tv_timeToDelay.tv_sec = nTimeOut/1000;
			tv_timeToDelay.tv_usec = (nTimeOut%1000) * 1000;   
			S32 selectResult = select(nSocket + 1, &readSet,NULL, &exceptSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误
				UPDATE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

				continue;
			}
			if (selectResult == 0)
			{//超时
				break;
			}
			if (FD_ISSET(nSocket,&exceptSet))
			{
				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			if (FD_ISSET(nSocket,&readSet))
			{
				S32 nRet;
				if (pRecvBuffer != NULL)
				{
					nRet = recv(nSocket,pRecvBuffer + nRecvPos,nRecvNeedSize - nRecvPos,0);
					if (nRet == 0)
					{
						closeSocket(nSocket);
						nSocket = -1;
						break;
					}
					else if (nRet < 0)
					{
						S32 errorNo = GetLastError();
						if (errorNo == EWOULDBLOCK ||
							errorNo == EINTR||
							errorNo == EAGAIN ||
							errorNo == ETIMEDOUT)
						{

							continue;
						}
						else
						{
							UPDATE_ERROR("%s \n",strerror(errorNo));
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
					}
					nRecvPos += nRet;
					if (nRecvPos == nRecvNeedSize)
					{// 完整包
						cJSON_Struct *pRecvJson = NULL;
						// 处理包
					
						pRecvJson = Common_Json_Parse(pRecvBuffer + tHeader.uExternHeaderLen,NULL,NULL);
						if (pRecvJson != NULL)
						{
							if (pOutParams)
							{
								*pOutParams = pRecvJson;
								pRecvJson = NULL;
							}
							Common_Json_Delete(pRecvJson);
							pRecvJson = NULL;
							nResult = 0;
						}
						else
						{
							//
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
						break;

					}
				}
				else
				{
					nRet = recv(nSocket,((char *)&tHeader) + nRecvHeadPos,sizeof(PacketHeader_T) - nRecvHeadPos,0);
					if (nRet == 0)
					{
						break;
					}
					else if (nRet < 0)
					{
						S32 errorNo = GetLastError();
						if (errorNo == EWOULDBLOCK ||
							errorNo == EINTR||
							errorNo == EAGAIN ||
							errorNo == ETIMEDOUT)
						{

							continue;
						}
						else
						{
							UPDATE_ERROR("%s \n",strerror(errorNo));
							break;
						}
					}
					nRecvHeadPos += nRet;
					if (nRecvHeadPos == sizeof(PacketHeader_T))
					{// 完整头
						// 处理包
						if (tHeader.uStartCode != UPDATE_PACKET_STARTCODE ||
							tHeader.nDataLen <= 0)
						{
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
						pRecvBuffer = (char *)Common_Malloc(tHeader.nDataLen + tHeader.uExternHeaderLen + 1,0,__FUNCTION__,__LINE__);
						if (pRecvBuffer == NULL)
						{
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
						nRecvNeedSize = tHeader.nDataLen + tHeader.uExternHeaderLen;
						nRecvPos = 0;

					}
				}

			}
		}
		else
		{
			FD_ZERO(&writeSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&writeSet);
			FD_SET(nSocket,&exceptSet);
			tv_timeToDelay.tv_sec = nTimeOut/1000;
			tv_timeToDelay.tv_usec = (nTimeOut%1000) * 1000;   
			S32 selectResult = select(nSocket + 1, NULL,&writeSet, &exceptSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误
				UPDATE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

				continue;
			}
			if (selectResult == 0)
			{//超时
				break;
			}
			if (FD_ISSET(nSocket,&exceptSet))
			{
				break;
			}
			if (FD_ISSET(nSocket,&writeSet))
			{
				// send
				if (pSendBuffer == NULL)
				{

					pSendBuffer = Common_Json_PrintUnformatted(pInParams,&nSendDataSize);
					if (pSendBuffer == NULL)
					{
						break;
					}
					nSendDataSize++;
					nSendHeadPos = 0;
					nSendDataPos = 0;
					memset(&tHeader,0,sizeof(tHeader));
					tHeader.uStartCode = UPDATE_PACKET_STARTCODE;
					tHeader.nDataLen = nSendDataSize;


				}
				if (nSendHeadPos != sizeof(tHeader))
				{
					nRet = send(nSocket,((char *)&tHeader) + nSendHeadPos,sizeof(tHeader) - nSendHeadPos,MSG_NOSIGNAL);
					if (nRet < 0)
					{
						break;
					}
					nSendHeadPos += nRet;
				}
				if (nSendHeadPos == sizeof(tHeader))
				{
					nRet = send(nSocket,pSendBuffer + nSendDataPos,nSendDataSize - nSendDataPos,MSG_NOSIGNAL);
					if (nRet < 0)
					{
						break;
					}
					nSendDataPos += nRet;
					if (nSendDataPos == nSendDataSize)
					{
						bWaitResult = 1;
					}
				}

			}
		}
	}
	if (pSendBuffer)
	{
		Common_Free(pSendBuffer,__FUNCTION__,__LINE__);
		pSendBuffer = NULL;
	}
	if (pRecvBuffer)
	{
		Common_Free(pRecvBuffer,__FUNCTION__,__LINE__);
		pRecvBuffer = NULL;
	}
	if (nSocket != -1)
	{
		closeSocket(nSocket);
		nSocket = -1;
	}


	return nResult;

}
#define UPDATE_UDP_PKT_LEN   1400
#define UPDATE_UDP_BUFFSIZE 1600
S32 Update_udp_SendPkt(S32 nSocket,char *pToIpv4,S32 nToPort,U32 bResp,U16 uMsgId,U32 *pSeq,void *pData,S32 nLen)
{
	PacketHeader_T *pHeader;
	UDPPacketExtHeader_T *pExtHeader;
	S8 *pSendBuffer = NULL,*pDataBuff;
	S32 nRet = -1;
	struct sockaddr_in addr;
	pSendBuffer = (S8 *)Common_Malloc(UPDATE_UDP_PKT_LEN,0,__FUNCTION__,__LINE__);
	if (pSendBuffer == NULL)
	{
		return -1;
	}
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(pToIpv4);
	addr.sin_port = htons(nToPort);
	if (pSeq == NULL)
	{// 不带私有头
		if (nLen < 64)
		{
			memcpy(pSendBuffer,pData,nLen);
			memset(pSendBuffer + nLen,0,64 - nLen);
			nRet = sendto(nSocket,(const char *)pSendBuffer,64,0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
		}
		else
		{
			 nRet = sendto(nSocket,(const char *)pData,nLen,0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
		}
		
	}
	else
	{// 带私有头
		pHeader = (PacketHeader_T *)pSendBuffer;
		memset(pHeader,0,sizeof(PacketHeader_T));
		pHeader->uStartCode = UPDATE_PACKET_STARTCODE;
		pHeader->uMsgId = uMsgId;
		pHeader->bResp = bResp;
		if (sizeof(PacketHeader_T) + nLen <= UPDATE_UDP_PKT_LEN)
		{// 单片
			pHeader->uSeq = *pSeq; 
			(*pSeq) ++;
			if (*pSeq > 0x7FFF)
			{
				*pSeq = 0;
			}
			pHeader->nDataLen = nLen;
			memcpy(pSendBuffer + sizeof(PacketHeader_T),pData,nLen);
			nRet = sendto(nSocket,(const char *)pSendBuffer,nLen + sizeof(PacketHeader_T),0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
			//printf("[%s.%d]nLen = %d nRet = %d %d<%s>\n",__FUNCTION__,__LINE__,nLen,nRet,GetLastError(),strerror(GetLastError()));

		}
		else
		{
			// 多片
			S32 nTotalSendSize = sizeof(PacketHeader_T) + sizeof(UDPPacketExtHeader_T) + nLen;
			S32 nSplitNum = (nTotalSendSize + UPDATE_UDP_PKT_LEN -1) / UPDATE_UDP_PKT_LEN;
			S32 i,pPos,nSplitLen,nPktLen,nFirstPktLen;
			nPktLen = UPDATE_UDP_PKT_LEN - sizeof(PacketHeader_T) - sizeof(UDPPacketExtHeader_T);
			nSplitNum = (nLen  + nPktLen - 1) / nPktLen;
			pHeader->uExternHeaderLen = sizeof(UDPPacketExtHeader_T);
			pExtHeader = (UDPPacketExtHeader_T *)(pHeader + 1);
			pDataBuff = (S8 *)(pExtHeader + 1);
			memset(pExtHeader,0,sizeof(UDPPacketExtHeader_T));
			pExtHeader->uType = 1;
			pExtHeader->uTotal = nLen;

			pExtHeader->uTotalPkt = nSplitNum;
			for (i = 0;i < nSplitNum;i++)
			{
				pHeader->uSeq = *pSeq;
				(*pSeq) ++;
				if (*pSeq > 0x7FFF)
				{
					*pSeq = 0;
				}
				if (i == nSplitNum - 1)
				{
					pHeader->nDataLen = nLen - nPktLen * i;
				}
				else
				{
					pHeader->nDataLen = nPktLen;
				}
				memcpy(pDataBuff,(S8 *)pData + i * nPktLen,pHeader->nDataLen);

				nRet = sendto(nSocket,(const char *)pSendBuffer,pHeader->nDataLen + sizeof(PacketHeader_T) + sizeof(UDPPacketExtHeader_T),0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
				//printf("[%s.%d]nLen = %d nRet = %d %d<%s>\n",__FUNCTION__,__LINE__,nLen,nRet,GetLastError(),strerror(GetLastError()));
				pExtHeader->uPktIdx++;


			}
		}
	}

	
	Common_Free(pSendBuffer,__FUNCTION__,__LINE__);
	return 0;
}





S32 Update_Udp_Require(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	struct sockaddr_in addr;
	S32 nRet;
	S32 selectResult;
	S32 nSocket = -1;
	S32 bTryConnect = 0,bWaitResult = 0;
	S32 nSendHeadPos = 0, nSendDataPos = 0,nSendDataSize;
	PacketHeader_T *pHeader;
	char *pSendBuffer = NULL;
	char *pRecvBuffer = NULL,*pPktBuffer = NULL;
	S32 nRecvNeedSize = 0;
	S32 nRecvPos = 0,nRecvHeadPos = 0;
	struct timeval tv_timeToDelay;
	fd_set readSet,writeSet,exceptSet;
	S32 nResult = -1;
	struct sockaddr_in srcaddr;
	static U16 staticMsgId = 0;
	U16 uMsgId = staticMsgId++;
	U32 uLastSeq,uPktIdx;
	cJSON_Struct *pOutJson = NULL;
	U32 nSeq = 0;
#if 0
	if (pInParams != NULL)
	{
		S8 *pStrValue = NULL;
		UPDATE_DEBUG("\n[UDP]InParam<%s>\n",pStrValue = Common_Json_PrintUnformatted(pInParams,NULL));
		Common_Free(pStrValue,__FUNCTION__,__LINE__);
	}
#endif
	if (pDomain == NULL || (nPort < 0 || nPort >= 0x0FFFF))
	{
		UPDATE_DEBUG("Here\n");
		return -1;
	}


	// create socket
	nSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSocket == -1)
	{
		return -1;
	}



	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(pDomain);
	addr.sin_port = htons(nPort);

	S8 *pSendType = NULL;
	int bBroadcast = 0,bHttp = 0;
	S8 szIPv4[20];
	Common_Json_GetAttrValue(pInParams,-1,"/Header/SendType",NULL,&pSendType,NULL,NULL);
	Common_Json_GetAttrValue(pInParams,-1,"/Header/HttpOverUdp",NULL,NULL,&bHttp,NULL);
	if (pSendType != NULL && 0 == Common_StriCmp(pSendType,"Broadcast"))
	{
		bBroadcast = 1;
	}
	if (bHttp)
	{
		CommonHttpContext_T tHttpContext;
		S32 nHttpSize = 0;
		Common_Http_Init(&tHttpContext);
		Common_Http_Json2Http(pInParams,&tHttpContext);
		pSendBuffer = Common_Http_Http2String(&tHttpContext,&nHttpSize);
		tHttpContext.pSendBuffer = NULL;
		Common_Http_Free(&tHttpContext);
	}
	else
	{
		pSendBuffer = Common_Json_PrintUnformatted(pInParams,&nSendDataSize);
	}
	

		
		if (pSendBuffer == NULL)
		{
			closeSocket(nSocket);
			return -1;
		}

		if (bBroadcast)
		{

			S32  bBroadcast1=TRUE; 
			setsockopt(nSocket,SOL_SOCKET,SO_BROADCAST,(const char*)&bBroadcast1,sizeof(S32));
			sprintf(szIPv4,"255.255.255.255");
		}
		else
		{
			strcpy(szIPv4,pDomain);
		}

		if(Update_udp_SendPkt(nSocket,szIPv4,nPort,0,uMsgId,bHttp?NULL:(&nSeq),pSendBuffer,nSendDataSize + 1))
		{
			Common_Free(pSendBuffer,__FUNCTION__,__LINE__);
			closeSocket(nSocket);
			return -1;
		}


	//
	
	while(1)
	{
	
	

			FD_ZERO(&readSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&readSet);
			FD_SET(nSocket,&exceptSet);
			tv_timeToDelay.tv_sec = nTimeOut/1000;
			tv_timeToDelay.tv_usec = (nTimeOut%1000) * 1000;  


			S32 selectResult = select(nSocket + 1, &readSet,NULL, &exceptSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误
				UPDATE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

				continue;
			}
			if (selectResult == 0)
			{//超时
				break;
			}
			if (FD_ISSET(nSocket,&exceptSet))
			{
				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			if (FD_ISSET(nSocket,&readSet))
			{
				
				S32 nRet;
#if (defined(WIN32) )
				int fromlen = 0;
				unsigned long len = 0;
#else
				socklen_t fromlen = 0;
				size_t len = 0;
#endif
				// ioctlsocket(nSocket,FIONREAD,&len);
				if (pPktBuffer == NULL)
				{
					pPktBuffer = (S8 *)Common_Malloc(UPDATE_UDP_BUFFSIZE + 1,0,__FUNCTION__,__LINE__);
				}
				if (pPktBuffer == NULL)
				{
					Common_Sleep(1,0);
					continue;
				}


				//while(len > 0)
				do
				{
					fromlen = sizeof(struct sockaddr_in);

					nRet = recvfrom(nSocket,pPktBuffer,UPDATE_UDP_BUFFSIZE,0,(struct sockaddr *)&srcaddr,&fromlen);
					if (nRet < 0)
					{
						break;
					}
					else if (nRet == 0)
					{
						break;
					}
					if (srcaddr.sin_addr.s_addr == inet_addr(pDomain) &&
						srcaddr.sin_port == htons(nPort))
					{
						pHeader = (PacketHeader_T *)pPktBuffer;
						if (pHeader->uStartCode != UPDATE_PACKET_STARTCODE)
						{
							CommonHttpContext_T tHttpContext;
							Common_Http_Init(&tHttpContext);
							if(Common_Http_Parse(pPktBuffer,nRet,&tHttpContext) <= 0)
							{
								Common_Http_Free(&tHttpContext);
								break;
							}
							if(Common_Http_Http2Json(&tHttpContext,&pOutJson) != 0)
							{
								Common_Http_Free(&tHttpContext);
								Common_Json_Delete(pOutJson);
								pOutJson = NULL;
								break;
							}
							if (pOutJson == NULL)
							{
								Common_Http_Free(&tHttpContext);
								break;
							}
							Common_Http_Free(&tHttpContext);
							nResult = 0;

						}
						else if (pHeader->uStartCode == UPDATE_PACKET_STARTCODE &&
							pHeader->uMsgId == uMsgId &&
							pHeader->bResp)
						{
							if (pHeader->uExternHeaderLen == 0)
							{// 完整包
								pPktBuffer[sizeof(PacketHeader_T) + pHeader->nDataLen] = 0;
								pOutJson = Common_Json_Parse(pPktBuffer + sizeof(PacketHeader_T),NULL,NULL);
								nResult = 0;
								break;
							}
							else
							{
								// 分片的包
								UDPPacketExtHeader_T *pExtHeader = (UDPPacketExtHeader_T *)(pHeader + 1);
								if (pExtHeader->uType == 1)
								{
									if (pExtHeader->uPktIdx == 0)
									{// 第一包
										if (pRecvBuffer != NULL)
										{//有错!
											break;
											
										}
										pRecvBuffer = (S8 *)Common_Malloc(pExtHeader->uTotal,0,__FUNCTION__,__LINE__);
										if (pRecvBuffer != NULL)
										{
											nRecvNeedSize = pExtHeader->uTotal;
											uPktIdx = 0;
											uLastSeq = pHeader->uSeq;
											nRecvPos = 0;
											memcpy(pRecvBuffer + nRecvPos,pPktBuffer + sizeof(PacketHeader_T) + sizeof(UDPPacketExtHeader_T),pHeader->nDataLen);
											nRecvPos += pHeader->nDataLen;
										}
									}
									else
									{
										U32 nNextSeq = uLastSeq + 1,nNextIdx = uPktIdx + 1;
										if (nNextSeq > 0x7FFF)
										{
											nNextSeq = 0;
										}
										if (nNextIdx > 0xFFFF)
										{
											nNextIdx = 0;
										}
										if (nNextSeq == pHeader->uSeq &&
											nNextIdx == pExtHeader->uPktIdx)
										{// ok
											memcpy(pRecvBuffer + nRecvPos,pPktBuffer + sizeof(PacketHeader_T) + sizeof(UDPPacketExtHeader_T),pHeader->nDataLen);
											nRecvPos += pHeader->nDataLen;
											if (pExtHeader->uTotalPkt == nNextIdx + 1)
											{// 最后一片
												if (nRecvPos == pExtHeader->uTotal)
												{//
													//完整包
													pRecvBuffer[nRecvPos] = 0;
													pOutJson = Common_Json_Parse(pRecvBuffer,NULL,NULL);
													nResult = 0;
													break;
												}
											}
										}
									}
								}
							}
						}
					}
					


					if (pOutJson != NULL)
					{
						break;
					}
					


					len = 0;
					//ioctlsocket(nSocket,FIONREAD,&len);
				}while(1);

			}

		
	}
	if (NULL != pOutParams)
	{
		*pOutParams = pOutJson;
		pOutJson = NULL;
	}
	else
	{
		// 失败
	}
	Common_Json_Delete(pOutJson);

	if (pPktBuffer != NULL)
	{
		Common_Free(pPktBuffer,__FUNCTION__,__LINE__);
		pPktBuffer = NULL;
	}
	if (pSendBuffer)
	{
		Common_Free(pSendBuffer,__FUNCTION__,__LINE__);
		pSendBuffer = NULL;
	}
	if (pRecvBuffer)
	{
		Common_Free(pRecvBuffer,__FUNCTION__,__LINE__);
		pRecvBuffer = NULL;
	}
	if (nSocket != -1)
	{
		closeSocket(nSocket);
		nSocket = -1;
	}


	return nResult;

}
#define UPDATE_CLIENT_NUM  4
typedef struct _tagUpdateClientMgr
{
	Common_Thread_T hThread;
	S32 bThreadExit;
	S8 *szDomain;
	S32 nPort;
	S32 nSocket;
	Update_Broadcast_Callback_Def fCallback;
	void *pCallbackUserData;
	S32 nTimes;
	S32 nInterval;
	S8 *pSendMsg;
	S32 nSendMsgLen;

}UpdateClientMgr_T;


static S32 staticUpdate_BroadcastCallback(Common_Thread_T hThreadHandle,void *pUserData)
{
	struct sockaddr_in addr;
	S32 nRet;
	S32 selectResult;
	S32 nSocket = -1;
	S32 bTryConnect = 0,bWaitResult = 0;
	S32 nSendHeadPos = 0, nSendDataPos = 0,nSendDataSize;
	PacketHeader_T *pHeader;
	char *pSendBuffer = NULL;
	char *pRecvBuffer = NULL,*pPktBuffer = NULL;
	S32 nRecvNeedSize = 0;
	S32 nRecvPos = 0,nRecvHeadPos = 0;
	struct timeval tv_timeToDelay;
	fd_set readSet,writeSet,exceptSet;
	S32 nResult = -1;
	struct sockaddr_in srcaddr;
	U32 uLastSeq,uPktIdx;
	cJSON_Struct *pOutJson = NULL;
	UpdateClientMgr_T *pMgr = (UpdateClientMgr_T *)pUserData;
	S32 nSec = 0,nLastSec = 0;
	S32 nTimeCount = 0,nInterval = 0;
	S8 szFromIPv4[20];
	U32 nSeq = 0;

	

	nSocket = pMgr->nSocket;
	

	nTimeCount = pMgr->nTimes;
	nInterval = pMgr->nInterval;
	if (nInterval < 1)
	{
		nInterval = 1;
	}
	//

	while(1)
	{
		if (pMgr->bThreadExit)
		{
			break;
		}
		if (pMgr->nTimes <= 0)
		{
			nTimeCount = 1;
		}
		Common_GetSystemCount(&nSec,NULL);
		//printf("nTimes = %d nTimeCount = %d nInterval = %d nSec = %d nLastSec = %d\n",pMgr->nTimes,nTimeCount,nInterval,nSec,nLastSec);
		if (pMgr->pSendMsg != NULL && nTimeCount > 0 && nLastSec + nInterval <= nSec)
		{
			// do
		//	printf("[Broadcast]Send \n");
			if (pMgr->pSendMsg[0] != '{')
			{
				struct sockaddr_in addr;
				memset(&addr,0,sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = inet_addr(pMgr->szDomain);
				addr.sin_port =  htons(pMgr->nPort);;
				sendto(nSocket,(const char *)pMgr->pSendMsg,pMgr->nSendMsgLen,0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
			}
			else
			{
				 Update_udp_SendPkt(nSocket,pMgr->szDomain,pMgr->nPort,0,0,&nSeq,pMgr->pSendMsg,pMgr->nSendMsgLen);
			}
			
			//printf("[Broadcast]Send end\n");
			nTimeCount--;
			nLastSec = nSec;
		}


		FD_ZERO(&readSet);
		FD_ZERO(&exceptSet);


		FD_SET(nSocket,&readSet);
		FD_SET(nSocket,&exceptSet);
		tv_timeToDelay.tv_sec = 1;
		tv_timeToDelay.tv_usec = 0;  

		S32 selectResult = select(nSocket + 1, &readSet,NULL, &exceptSet, &tv_timeToDelay);
		if (selectResult < 0)
		{//错误
			UPDATE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));
			continue;
		}
		if (selectResult == 0)
		{//超时
			
			continue;
		}
		if (FD_ISSET(nSocket,&exceptSet))
		{
			closeSocket(nSocket);
			nSocket = -1;
			break;

		}
		if (FD_ISSET(nSocket,&readSet))
		{

			S32 nRet;
#if (defined(WIN32) )
			int fromlen = 0;
			unsigned long len = 0;
#else
			socklen_t fromlen = 0;
			size_t len = 0;
#endif
			//ioctlsocket(nSocket,FIONREAD,&len);
			if (pPktBuffer == NULL)
			{
				pPktBuffer = (S8 *)Common_Malloc(UPDATE_UDP_BUFFSIZE,0,__FUNCTION__,__LINE__);
			}
			if (pPktBuffer == NULL)
			{
				printf("[%s.%d]\n",__FUNCTION__,__LINE__);
				Common_Sleep(1,0);
				continue;
			}

			//while(len > 0)
			while(1)
			{
				if (pMgr->bThreadExit)
				{
					break;
				}
				fromlen = sizeof(struct sockaddr_in);
				nRet = recvfrom(nSocket,pPktBuffer,UPDATE_UDP_BUFFSIZE,0,(struct sockaddr *)&srcaddr,&fromlen);
				if (nRet < 0)
				{
					break;
				}
				else if (nRet == 0)
				{
					break;
				}
				sprintf(szFromIPv4,"%d.%d.%d.%d",(srcaddr.sin_addr.s_addr >> 0) & 0xFF
					,(srcaddr.sin_addr.s_addr >> 8) & 0xFF
					,(srcaddr.sin_addr.s_addr >> 16) & 0xFF
					,(srcaddr.sin_addr.s_addr >> 24) & 0xFF);
				pPktBuffer[nRet] = 0;
				if (srcaddr.sin_port == htons(pMgr->nPort))
				{
					pHeader = (PacketHeader_T *)pPktBuffer;
					if (pHeader->uStartCode == UPDATE_PACKET_STARTCODE)
					{
						if (pHeader->bResp)
						{
							if (pHeader->uExternHeaderLen == 0)
							{// 完整包
								pPktBuffer[sizeof(PacketHeader_T) + pHeader->nDataLen] = 0;
								pOutJson = Common_Json_Parse(pPktBuffer + sizeof(PacketHeader_T),NULL,NULL);
								pMgr->fCallback(pMgr,szFromIPv4,pMgr->nPort,pOutJson,pMgr->pCallbackUserData);
								Common_Json_Delete(pOutJson);
								pOutJson = NULL;
							}
							else
							{
							}
						}
						
					}
					else
					{ 
						// http
						CommonHttpContext_T tHttpContext;
						Common_Http_Init(&tHttpContext);

						Common_Http_Parse(pPktBuffer,nRet,&tHttpContext);
						Common_Http_Http2Json(&tHttpContext,&pOutJson);
						if (pOutJson != NULL)
						{
							pMgr->fCallback(pMgr,szFromIPv4,pMgr->nPort,pOutJson,pMgr->pCallbackUserData);
							Common_Json_Delete(pOutJson);
							pOutJson = NULL;
						}
						Common_Http_Free(&tHttpContext);
					}
				}



		

				len = 0;
				//ioctlsocket(nSocket,FIONREAD,&len);
			}

		}


	}
	printf("[%s.%d]\n",__FUNCTION__,__LINE__);
	Common_Json_Delete(pOutJson);

	if (pPktBuffer != NULL)
	{
		Common_Free(pPktBuffer,__FUNCTION__,__LINE__);
		pPktBuffer = NULL;
	}
	if (pSendBuffer)
	{
		Common_Free(pSendBuffer,__FUNCTION__,__LINE__);
		pSendBuffer = NULL;
	}
	if (pRecvBuffer)
	{
		Common_Free(pRecvBuffer,__FUNCTION__,__LINE__);
		pRecvBuffer = NULL;
	}



	return nResult;

}


S32 Update_Udp_Start(UPDATE_BROADCAST_HANDLE *pHandle,char *szDomain,S32 nPort,cJSON_Struct *pInParams,S8 *pBuffer,S32 nBufferSize,S32 nTimes,S32 nIntervalSec,Update_Broadcast_Callback_Def fxn,void *pUserData)
{
	UpdateClientMgr_T *pMgr = NULL;
	struct sockaddr_in addr;
	int nSocket = -1,nRet = -1;
	char *szLocalIpv4 = NULL;
	if ( (pInParams == NULL && (pBuffer == NULL|| nBufferSize <= 0)) ||
		nPort <= 0 || nPort >= 0xFFFF ||
		fxn == NULL)
	{
		return -1;
	}
	pMgr = (UpdateClientMgr_T *)Common_Malloc(sizeof(UpdateClientMgr_T),0,__FUNCTION__,__LINE__);
	if (pMgr == NULL)
	{
		return -1;
	}

	
	memset(pMgr,0,sizeof(UpdateClientMgr_T));
	if (szDomain != NULL)
	{
		pMgr->szDomain = Common_StrDup(szDomain,__FUNCTION__,__LINE__);
	
	}
	else
	{
		pMgr->szDomain = Common_StrDup("255.255.255.255",__FUNCTION__,__LINE__);
		
	}
	if (pMgr->szDomain == NULL)
	{
		Common_Free(pMgr,__FUNCTION__,__LINE__);
		return -1;
	}
	pMgr->nPort = nPort;
	pMgr->nTimes = nTimes;
	pMgr->nInterval = nIntervalSec;
	pMgr->fCallback = fxn;
	pMgr->pCallbackUserData = pUserData;
	if (pInParams != NULL)
	{
		S32 bHttp = 0;
		Common_Json_GetAttrValue(pInParams,-1,"/Header/BindIpv4",NULL,&szLocalIpv4,NULL,NULL);
		Common_Json_GetAttrValue(pInParams,-1,"/Header/HttpOverUdp",NULL,NULL,&bHttp,NULL);
		if (bHttp)
		{
			CommonHttpContext_T tHttpContext;
			Common_Http_Init(&tHttpContext);
			Common_Http_Json2Http(pInParams,&tHttpContext);
			S32 nSendLen = 0;
			S8 *pSendStr = Common_Http_Http2String(&tHttpContext,&nSendLen);
			pMgr->pSendMsg = pSendStr;
			pMgr->nSendMsgLen = nSendLen;
			tHttpContext.pSendBuffer = NULL;
			Common_Http_Free(&tHttpContext);
		}
		else
		{
			pMgr->pSendMsg = Common_Json_PrintUnformatted(pInParams,&pMgr->nSendMsgLen);
			pMgr->nSendMsgLen  += 1;
		}
		if (pMgr->pSendMsg == NULL)
		{
			Common_Free(pMgr->szDomain,__FUNCTION__,__LINE__);
			Common_Free(pMgr,__FUNCTION__,__LINE__);
			return -1;
		}
		
	}
	else if(pBuffer != NULL)
	{
		S32 bHttp = 0;
		pInParams = Common_Json_Parse(pBuffer,NULL,NULL);
		if (pInParams != NULL)
		{
			Common_Json_GetAttrValue(pInParams,-1,"/Header/BindIpv4",NULL,&szLocalIpv4,NULL,NULL);
			Common_Json_GetAttrValue(pInParams,-1,"/Header/HttpOverUdp",NULL,NULL,&bHttp,NULL);
			if (bHttp)
			{
				CommonHttpContext_T tHttpContext;
				Common_Http_Init(&tHttpContext);
				Common_Http_Json2Http(pInParams,&tHttpContext);
				S32 nSendLen = 0;
				S8 *pSendStr = Common_Http_Http2String(&tHttpContext,&nSendLen);
				pMgr->pSendMsg = pSendStr;
				pMgr->nSendMsgLen = nSendLen;
				tHttpContext.pSendBuffer = NULL;
				Common_Http_Free(&tHttpContext);
			}
			else
			{
				pMgr->pSendMsg = Common_Json_PrintUnformatted(pInParams,&pMgr->nSendMsgLen);
				pMgr->nSendMsgLen  += 1;
			}
			Common_Json_Delete(pInParams);
			pInParams = NULL;
		}
		else
		{
			pMgr->pSendMsg = pBuffer;
			pMgr->nSendMsgLen  = nBufferSize;
		}
		if (pMgr->pSendMsg == NULL)
		{
			Common_Free(pMgr->szDomain,__FUNCTION__,__LINE__);
			Common_Free(pMgr,__FUNCTION__,__LINE__);
			return -1;
		}
		
	}
	

	// create socket
	nSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSocket == -1)
	{
		Common_Free(pMgr->szDomain,__FUNCTION__,__LINE__);
		Common_Free(pMgr->pSendMsg,__FUNCTION__,__LINE__);
		Common_Free(pMgr,__FUNCTION__,__LINE__);
		return -1;
	}

	memset(&addr,0,sizeof(addr));
	if (szLocalIpv4 != NULL)
	{
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(szLocalIpv4);
		addr.sin_port = 0;
		bind(nSocket,(struct sockaddr *)&addr,sizeof(struct sockaddr_in));
	}
	if (szDomain == NULL || 0 == Common_StrCmp(szDomain,"255.255.255.255"))
	{
		S32  bBroadcast=TRUE; 
		setsockopt(nSocket,SOL_SOCKET,SO_BROADCAST,(const char*)&bBroadcast,sizeof(S32));
	}

	
#if defined(__WIN32__) || defined(_WIN32)
	unsigned long arg = 1;
	nRet =  ioctlsocket(nSocket, FIONBIO, &arg);
	if (nRet)
	{
		Common_Free(pMgr->szDomain,__FUNCTION__,__LINE__);
		Common_Free(pMgr->pSendMsg,__FUNCTION__,__LINE__);
		Common_Free(pMgr,__FUNCTION__,__LINE__);
		closeSocket(nSocket);
		nSocket = -1;
		return -1;
	}

#else


	int curFlags = fcntl(nSocket, F_GETFL, 0);
	nRet =  fcntl(nSocket, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		Common_Free(pMgr->pSendMsg,__FUNCTION__,__LINE__);
		Common_Free(pMgr,__FUNCTION__,__LINE__);
		closeSocket(nSocket);
		nSocket = -1;
		return -1;
	}
#endif
	
	pMgr->nSocket = nSocket;
	if(Common_Thread_Create(&pMgr->hThread,"Update_udp_Broadcast",0,0,staticUpdate_BroadcastCallback,pMgr))
	{
		Common_Free(pMgr->szDomain,__FUNCTION__,__LINE__);
		Common_Free(pMgr->pSendMsg,__FUNCTION__,__LINE__);
		pMgr->pSendMsg = NULL;
		Common_Free(pMgr,__FUNCTION__,__LINE__);
		closeSocket(nSocket);
		nSocket = -1;
		return -1;
	}
	*pHandle = pMgr;


	return 0;
}
S32 Update_Udp_Send(UPDATE_BROADCAST_HANDLE *pHandle,cJSON_Struct *pInParams,S8 *pBuffer,S32 nBufferSize)
{
	UpdateClientMgr_T *pMgr = NULL;
	S8 *pSendStr = NULL;
	S32 nSendLen = 0,nRet = -1;
	if (pHandle == NULL)
	{
		return 0;
	}
	pMgr =(UpdateClientMgr_T *) (*pHandle);
	if (pMgr == NULL)
	{
		return 0;
	}
	if (pInParams != NULL)
	{
		S32 bHttp = 0;
		Common_Json_GetAttrValue(pInParams,-1,"/Header/HttpOverUdp",NULL,NULL,&bHttp,NULL);
		if (bHttp)
		{
			CommonHttpContext_T tHttpContext;
			Common_Http_Init(&tHttpContext);
			Common_Http_Json2Http(pInParams,&tHttpContext);
			
			pSendStr = Common_Http_Http2String(&tHttpContext,&nSendLen);
			
			tHttpContext.pSendBuffer = NULL;
			Common_Http_Free(&tHttpContext);
		}
		else
		{
			pSendStr = Common_Json_PrintUnformatted(pInParams,&nSendLen);
			nSendLen  += 1;
		}

	}
	else if(pBuffer != NULL)
	{
		S32 bHttp = 0;
		pInParams = Common_Json_Parse(pBuffer,NULL,NULL);
		if (pInParams != NULL)
		{
			Common_Json_GetAttrValue(pInParams,-1,"/Header/HttpOverUdp",NULL,NULL,&bHttp,NULL);
			if (bHttp)
			{
				CommonHttpContext_T tHttpContext;
				Common_Http_Init(&tHttpContext);
				Common_Http_Json2Http(pInParams,&tHttpContext);
				pSendStr = Common_Http_Http2String(&tHttpContext,&nSendLen);
				tHttpContext.pSendBuffer = NULL;
				Common_Http_Free(&tHttpContext);
			}
			else
			{
				pSendStr = Common_Json_PrintUnformatted(pInParams,&nSendLen);
				nSendLen  += 1;
			}
			Common_Json_Delete(pInParams);
			pInParams = NULL;
		}
		else
		{
			pSendStr = pBuffer;
			nSendLen  = nBufferSize;
		}


	}
	if (pSendStr != NULL)
	{
		// do
		//	printf("[Broadcast]Send \n");
		if (pSendStr[0] != '{')
		{
			struct sockaddr_in addr;
			memset(&addr,0,sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(pMgr->szDomain);
			addr.sin_port =  htons(pMgr->nPort);
			nRet = sendto(pMgr->nSocket,(const char *)pSendStr,nSendLen,0,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
		}
		else
		{
			U32 nSeq = 0;
			nRet = Update_udp_SendPkt(pMgr->nSocket,pMgr->szDomain,pMgr->nPort,0,0,&nSeq,pSendStr,nSendLen);
		}
	
	}
	return nRet;
}
S32 Update_Udp_Stop(UPDATE_BROADCAST_HANDLE *pHandle)
{
	UpdateClientMgr_T *pMgr = NULL;
	if (pHandle == NULL)
	{
		return 0;
	}
	pMgr =(UpdateClientMgr_T *) (*pHandle);
	if (pMgr == NULL)
	{
		return 0;
	}
	pMgr->bThreadExit = 1;
	Common_Thread_Destroy(&pMgr->hThread);
	pMgr->hThread = NULL;
	Common_Free(pMgr->pSendMsg,__FUNCTION__,__LINE__);
	pMgr->pSendMsg = NULL;
	Common_Free(pMgr->szDomain,__FUNCTION__,__LINE__);
	pMgr->szDomain = NULL;
	if (pMgr->nSocket > 0)
	{
		closeSocket(pMgr->nSocket);
		pMgr->nSocket = -1;
	}
	Common_Free(pMgr,__FUNCTION__,__LINE__);
	*pHandle = NULL;
	return 0;
}

S32 Update_Broadcast_Start(UPDATE_BROADCAST_HANDLE *pHandle,S32 nPort,cJSON_Struct *pInParams,S8 *pBuffer,S32 nBufferSize,S32 nTimes,S32 nIntervalSec,Update_Broadcast_Callback_Def fxn,void *pUserData)
{
	return Update_Udp_Start(pHandle,NULL,nPort,pInParams,pBuffer,nBufferSize,nTimes,nIntervalSec,fxn,pUserData);
}

S32 Update_Broadcast_Stop(UPDATE_BROADCAST_HANDLE *pHandle)
{

	return Update_Udp_Stop(pHandle);
}


