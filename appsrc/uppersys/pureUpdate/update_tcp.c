#include "libcommon_api.h"
#include "update_struct.h"
#include "common_net.h"

static void TcpSockFree(void *data)
{
	UTILS_SOCKET_NODE_T *node = (UTILS_SOCKET_NODE_T *)data;

    if (node != NULL)
    {
    	if (node->sockfd > 0)
			close(node->sockfd);
    }
    free(node);
}

static S32 static_Module_CallResponce(UpdateClientInfo_T *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct *pOutParams)
{
	S8 *pSendStr;
	S32 nStrlen = 0,nSendLen = 0;
	struct timeval tv_timeToDelay;
	fd_set tReadSet,tWriteSet,tExceptionSet;
	S32 nMaxNumSocket = 0;
	if (pOutParams != NULL)
	{
		pSendStr = Common_Json_Print(pOutParams,&nStrlen);
		if (pSendStr == NULL)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;
			return -1;

		}

	}
	else
	{
		// 无结果 的自动处理
		pSendStr = (char *)Common_Malloc(512,0,__FUNCTION__,__LINE__);
		if (pSendStr)
		{
			nStrlen = sprintf(pSendStr,"{\"Header\":{\"Code\":0}}");
		}
	}
	if (pSendStr == NULL)
	{
		return -1;
	}
	if (pClientInfo->nSocket == -1 || pClientInfo->bNeedDelete)
	{
		Common_Free(pSendStr,__FUNCTION__,__LINE__);
		return -1;
	}

	pClientInfo->tHeader.nDataLen = nStrlen + 1;
	pClientInfo->tHeader.uExternHeaderLen = 0;
	nSendLen = send(pClientInfo->nSocket,(char *)&pClientInfo->tHeader,sizeof(PacketHeader_T),MSG_NOSIGNAL);
	if (nSendLen < 0)
	{
		closeSocket(pClientInfo->nSocket);
		pClientInfo->nSocket = -1;
		pClientInfo->bNeedDelete = 1;

	}
	else if (nSendLen != sizeof(PacketHeader_T))
	{
		// 需要放缓冲，稍后再发
		pClientInfo->nHeaderLen = nSendLen;
		pClientInfo->pSendBuffer = pSendStr;
		pClientInfo->nNeedSendLen = nStrlen + 1;
		pSendStr = NULL;
	}
	else
	{
		nSendLen = send(pClientInfo->nSocket,pSendStr,nStrlen + 1,MSG_NOSIGNAL);
		if (nSendLen < 0)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;
		}
		else if (nSendLen != nStrlen + 1)
		{
			pClientInfo->pSendBuffer = pSendStr;
			pClientInfo->nNeedSendLen = nStrlen + 1;
			pClientInfo->nSendLen = nSendLen;
			pSendStr = NULL;
		}
	}
	if (pSendStr != NULL)
	{
		Common_Free(pSendStr,__FUNCTION__,__LINE__);
		// 成功
		return 0;

	}
	if (pClientInfo->nSocket == -1)
	{
		return -1;
	}
	while(1)
	{
		FD_ZERO(&tReadSet);
		FD_ZERO(&tWriteSet);
		FD_ZERO(&tExceptionSet);


		FD_SET(pClientInfo->nSocket,&tWriteSet);


		FD_SET(pClientInfo->nSocket,&tExceptionSet);
		if (pClientInfo->nSocket + 1 > nMaxNumSocket)
		{
			nMaxNumSocket = pClientInfo->nSocket + 1;
		}

		tv_timeToDelay.tv_sec = 0;
		tv_timeToDelay.tv_usec = 100000;

		S32 selectResult = select(nMaxNumSocket, NULL,&tWriteSet, &tExceptionSet, &tv_timeToDelay);
		if (selectResult < 0)
		{//错误

			continue;
		}
		if (selectResult == 0)
		{//超时
			continue;
		}
		if (FD_ISSET(pClientInfo->nSocket,&tExceptionSet))
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;
			break;
		}
		else if (FD_ISSET(pClientInfo->nSocket,&tWriteSet))
		{
			S32 nSendLen;
			//
			if (pClientInfo->nHeaderLen != sizeof(PacketHeader_T))
			{
				nSendLen = send(pClientInfo->nSocket,((char *)&pClientInfo->tHeader) + pClientInfo->nHeaderLen,sizeof(PacketHeader_T)-pClientInfo->nHeaderLen,MSG_NOSIGNAL);
				if (nSendLen < 0)
				{
					closeSocket(pClientInfo->nSocket);
					pClientInfo->nSocket = -1;
					pClientInfo->bNeedDelete = 1;
					break;

				}
				else if (nSendLen != sizeof(PacketHeader_T))
				{
					// 需要放缓冲，稍后再发
					pClientInfo->nHeaderLen += nSendLen;
				}

			}
			else if (pClientInfo->pSendBuffer != NULL)
			{
				nSendLen = send(pClientInfo->nSocket,pClientInfo->pSendBuffer + pClientInfo->nSendLen,pClientInfo->nNeedSendLen - pClientInfo->nSendLen,MSG_NOSIGNAL);
				if (nSendLen < 0)
				{
					closeSocket(pClientInfo->nSocket);
					pClientInfo->nSocket = -1;
					pClientInfo->bNeedDelete = 1;
					break;
				}
				else
				{
					pClientInfo->nSendLen += nSendLen;
					if (pClientInfo->nSendLen == pClientInfo->nNeedSendLen)
					{
						Common_Free(pClientInfo->pSendBuffer,__FUNCTION__,__LINE__);
						pClientInfo->pSendBuffer = NULL;
						pClientInfo->nSendLen = 0;
						pClientInfo->nNeedSendLen = 0;
						pClientInfo->nHeaderLen = 0;
						pClientInfo->bNeedDelete = 1;
						return 0;
					}
				}
			}

		}
	};
	return -1;


}









static S32 static_Http_Client(UpdateClientInfo_T *pClientInfo)
{
	S32 nRet = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	if (!pClientInfo->bHttp)
	{
		if (pClientInfo->nSocket != -1)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
		}
		pClientInfo->bNeedDelete = 1;
		return -1;
	}
	if (0 != Common_Http_Recv(pClientInfo->nSocket,&pClientInfo->tHttpContext))
	{
		if (pClientInfo->nSocket != -1)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
		}
		pClientInfo->bNeedDelete = 1;
		return -1;
	}
	if (pClientInfo->tHttpContext.nHttpStep != 3)
	{// 未完成,继续 读数据
		return 0;
	}
	// 处理命令
	if(0 != Common_Http_Http2Json(&pClientInfo->tHttpContext,&pInParam))
	{
		pClientInfo->bNeedDelete = 1;
		Common_Json_Delete(pInParam);
		pInParam = NULL;
		return -1;
	}
	UpdateMgr_T *pModuleMgr = (UpdateMgr_T *)pClientInfo->pMgr;
	if (pModuleMgr != NULL)
	{
		if (pModuleMgr->fCallback != NULL && 0 == pModuleMgr->fCallback(NULL,pClientInfo,pInParam,&pOutParam,pModuleMgr->pCallbackUser))
		{
		}
		else
		{
			pOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,-6,0);
		}
	}
	if (pOutParam == NULL)
	{
		pOutParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pOutParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pOutParam,-1,"Header/Code",Common_Json_Type_Number,NULL,Common_ResResponceStatusCode_BadRequest,0);
	}

	// 应答
	if (pOutParam != NULL)
	{
		CommonHttpContext_T *pSendHttpContext = (CommonHttpContext_T *)Common_Malloc(sizeof(CommonHttpContext_T),0,__FUNCTION__,__LINE__);
		if (pSendHttpContext != NULL)
		{
			Common_Http_Init(pSendHttpContext);
			if(0 == Common_Http_Json2Http(pOutParam,pSendHttpContext))
			{

				do
				{
					nRet = Common_Http_Send(pClientInfo->nSocket,pSendHttpContext);
					if (nRet != 0)
					{
						pClientInfo->bNeedDelete = 1;
						break;
					}
					if (pSendHttpContext->nHttpStep == 3)
					{
						break;
					}
				} while (1);
				if (!pSendHttpContext->bKeepAlive)
				{
					pClientInfo->bNeedDelete = 1;
				}

			}
			else
			{
				pClientInfo->bNeedDelete = 1;
			}
			Common_Http_Free(pSendHttpContext);
			Common_Free(pSendHttpContext,__FUNCTION__,__LINE__);
			pSendHttpContext = NULL;
		}

	}

	// 关闭
	if (!pClientInfo->tHttpContext.bKeepAlive)
	{
		pClientInfo->bNeedDelete = 1;
	}
	Common_Json_Delete(pInParam);
	pInParam = NULL;
	Common_Json_Delete(pOutParam);
	pOutParam = NULL;



	return 0;
}

static S32 static_Module_Thread_Client(Common_Thread_T hThreadHandle,void *pUserData)
{
	UpdateClientInfo_T *pClientInfo = (UpdateClientInfo_T *)pUserData;
	struct timeval tv_timeToDelay;
	fd_set tReadSet,tExceptionSet;
	S32 nMaxNumSocket = 0;
	if (pClientInfo == NULL)
	{
		return -1;
	}
	while (1)
	{
		if (pClientInfo->bNeedDelete)
		{
			if (pClientInfo->nSocket != -1)
			{
				closeSocket(pClientInfo->nSocket);
				pClientInfo->nSocket = -1;
			}
			break;
		}
		// add sockets
		FD_ZERO(&tReadSet);
		FD_ZERO(&tExceptionSet);
		if (pClientInfo->nSocket != -1)
		{
			FD_SET(pClientInfo->nSocket,&tReadSet);

			FD_SET(pClientInfo->nSocket,&tExceptionSet);
			if (pClientInfo->nSocket + 1 > nMaxNumSocket)
			{
				nMaxNumSocket = pClientInfo->nSocket + 1;
			}
		}

		tv_timeToDelay.tv_sec = 0;
		tv_timeToDelay.tv_usec = 10000;

		S32 selectResult = select(nMaxNumSocket, &tReadSet,NULL, &tExceptionSet, &tv_timeToDelay);
		if (selectResult < 0)
		{//错误

			continue;
		}
		if (selectResult == 0)
		{//超时
			continue;
		}
		if (FD_ISSET(pClientInfo->nSocket,&tReadSet))
		{
			S32 nRet;
			if (pClientInfo->bHttp)
			{
				static_Http_Client(pClientInfo);
				continue;
			}
			if (pClientInfo->pRecvBuffer != NULL)
			{
				nRet = recv(pClientInfo->nSocket,pClientInfo->pRecvBuffer + pClientInfo->nRecvLen,pClientInfo->nNeedLen - pClientInfo->nRecvLen,0);
				if (nRet == 0)
				{
					closeSocket(pClientInfo->nSocket);
					pClientInfo->nSocket = -1;
					pClientInfo->bNeedDelete = 1;

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
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
				}
				pClientInfo->nRecvLen += nRet;
				if (pClientInfo->nRecvLen == pClientInfo->nNeedLen)
				{// 完整包
					cJSON_Struct *pRecvJson,*pResultJson=NULL;
					// 处理包
					pRecvJson = Common_Json_Parse(pClientInfo->pRecvBuffer + pClientInfo->tHeader.uExternHeaderLen,NULL,NULL);
					if (pRecvJson != NULL)
					{
						UpdateMgr_T *pModuleMgr = (UpdateMgr_T *)pClientInfo->pMgr;
						if (pModuleMgr != NULL)
						{
						    if (pModuleMgr->fCallback != NULL && 0 == pModuleMgr->fCallback(NULL,pClientInfo,pRecvJson,&pResultJson,pModuleMgr->pCallbackUser))
							{
								static_Module_CallResponce(pClientInfo,pRecvJson,pResultJson);
							}
							else
							{
								pResultJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
								Common_Json_SetAttrValue(pResultJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
								Common_Json_SetAttrValue(pResultJson,-1,"Header/Code",Common_Json_Type_Number,NULL,-6,0);
								static_Module_CallResponce(pClientInfo,pRecvJson,pResultJson);
							}
						}
						Common_Json_Delete(pResultJson);
						Common_Json_Delete(pRecvJson);
						pRecvJson = NULL;
						if (pClientInfo->nSocket != -1)
						{
							closeSocket(pClientInfo->nSocket);
							pClientInfo->nSocket = -1;
						}

						pClientInfo->bNeedDelete = 1;
						break;
					}
					else
					{
						//
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
					// 处理完后，清除缓冲
					Common_Free(pClientInfo->pRecvBuffer,__FUNCTION__,__LINE__);
					pClientInfo->pRecvBuffer = NULL;
					pClientInfo->nNeedLen = 0;
					pClientInfo->nRecvLen = 0;

				}
			}
			else
			{
				// 检测是不是HTTP
				if (!pClientInfo->bHeaderChecked)
				{
					U32 uStartCode = 0;
					nRet = recv(pClientInfo->nSocket,(char *)&uStartCode,4,MSG_PEEK);
					if (nRet == 0)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
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
							closeSocket(pClientInfo->nSocket);
							pClientInfo->nSocket = -1;
							pClientInfo->bNeedDelete = 1;
							break;
						}
					}
					if (nRet != 4)
					{
						continue;
					}
					if (uStartCode != UPDATE_PACKET_STARTCODE)
					{
						// 可能是http
						pClientInfo->bHttp = 1;
						Common_Http_Init(&pClientInfo->tHttpContext);

					}
					pClientInfo->bHeaderChecked = 1;

				}
				if (pClientInfo->bHttp)
				{
					static_Http_Client(pClientInfo);
					continue;
				}
				// 私有方式
				nRet = recv(pClientInfo->nSocket,((char *)&pClientInfo->tHeader) + pClientInfo->nHeaderLen,sizeof(PacketHeader_T) - pClientInfo->nHeaderLen,0);
				if (nRet == 0)
				{
					closeSocket(pClientInfo->nSocket);
					pClientInfo->nSocket = -1;
					pClientInfo->bNeedDelete = 1;
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
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
				}
				pClientInfo->nHeaderLen += nRet;
				if (pClientInfo->nHeaderLen == sizeof(PacketHeader_T))
				{// 完整头
					// 处理包
					if (pClientInfo->tHeader.uStartCode != UPDATE_PACKET_STARTCODE ||
						pClientInfo->tHeader.nDataLen <= 0)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
					pClientInfo->pRecvBuffer = (char *)Common_Malloc(pClientInfo->tHeader.nDataLen + pClientInfo->tHeader.uExternHeaderLen + 1,0,__FUNCTION__,__LINE__);
					if (pClientInfo->pRecvBuffer == NULL)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
					pClientInfo->nNeedLen = pClientInfo->tHeader.nDataLen + pClientInfo->tHeader.uExternHeaderLen;
					pClientInfo->nRecvLen = 0;
					// 处理完后，清除缓冲
					//memset(&pClientInfo->tHeader,0,sizeof(ModulePacketHeader_T));
					pClientInfo->nHeaderLen = 0;

				}
			}

		}
		if (FD_ISSET(pClientInfo->nSocket,&tExceptionSet))
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;
			break;
		}

	}
	if (pClientInfo->nSocket > 0)
	{
		closeSocket(pClientInfo->nSocket);
		pClientInfo->nSocket = -1;
	}
	Common_Http_Free(&pClientInfo->tHttpContext);
	Common_Free(pClientInfo->pRecvBuffer,__FUNCTION__,__LINE__);
	pClientInfo->pRecvBuffer = NULL;
	Common_Free(pClientInfo->pSendBuffer,__FUNCTION__,__LINE__);
	pClientInfo->pSendBuffer = NULL;
	Common_Free(pClientInfo,__FUNCTION__,__LINE__);

	return 0;
}

static S32 static_HandleListenInComming(UpdateMgr_T *pModuleMgr,int socketfd)
{
	S32 nRet;
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	S32 clientSocket = accept(socketfd, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientSocket >= 0)
	{
#if defined(__WIN32__) || defined(_WIN32)
		unsigned long arg = 1;
		nRet =  ioctlsocket(clientSocket, FIONBIO, &arg);
		if (nRet)
		{
			closeSocket(clientSocket);
			return -1;
		}

#else
		S32 curFlags = fcntl(clientSocket, F_GETFL, 0);
		nRet =  fcntl(clientSocket, F_SETFL, curFlags|O_NONBLOCK);
		if (nRet < 0 )
		{
			closeSocket(clientSocket);
			return -1;
		}

#endif

			UpdateClientInfo_T *pClientModuleInfo = NULL;
			pClientModuleInfo = (UpdateClientInfo_T *)Common_Malloc(sizeof(UpdateClientInfo_T),0,__FUNCTION__,__LINE__) ;
			if (pClientModuleInfo == NULL)
			{
				closeSocket(clientSocket);
				return -1;
			}
			memset(pClientModuleInfo,0,sizeof(UpdateClientInfo_T));
			pClientModuleInfo->pMgr = pModuleMgr;
			pClientModuleInfo->nSocket = clientSocket;
			pClientModuleInfo->bTcp = 1;
			if(Common_Thread_Create(&pClientModuleInfo->hThread,__FUNCTION__,0,COMMON_THREAD_CREATEFLAG_DETACH,static_Module_Thread_Client,pClientModuleInfo))
			{
				closeSocket(clientSocket);
				Common_Free(pClientModuleInfo,__FUNCTION__,__LINE__);
				return -1;
			}


	}
	return 0;
}

static S32 static_Module_Thread_Listen(Common_Thread_T hThreadHandle,void *pUserData)
{
	UpdateMgr_T *pMgr = (UpdateMgr_T *)pUserData;

	if (pMgr == NULL)
	{
        LOGE("pMgr == null\n");
		return -1;
	}

	//unsigned long long int timeA = 0, timeB = 0;
    //timeA = timeB = Common_GetSystemCount64();

	while (!pMgr->bListenThreadExit)
	{
        /*timeB = Common_GetSystemCount64();

        if (timeB - timeA > 1000LLU)
        {
            timeA = timeB;
            if (MultiAddrCheck(pMgr->socketTcpList) > 0)
            {
                LOGD("network dev changed \n");
                Common_DList_DeleteAll(pMgr->socketTcpList);
				MultiAddrCreateAndListen(pMgr->socketTcpList,TCP,pMgr->nListenPort_Tcp);
                continue;
            }
        }

        int sockfd = -1;
        char devName[32] = {};
        S32 selectResult = MultiAddrSelect(pMgr->socketTcpList, &sockfd, devName);*/

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(pMgr->nSocketTcp, &rfds);

        struct timeval timeout = { 0, 0 };
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        S32 selectResult = select(pMgr->nSocketTcp + 1, &rfds, NULL, NULL, &timeout);

		if (selectResult < 0)
		{//错误
			Common_Sleep(1,0);
			continue;
		}
		else if (selectResult == 0)
		{//超时
			continue;
		}
        else //(selectResult > 0)
        {
            if (FD_ISSET(pMgr->nSocketTcp, &rfds))
            {
                static_HandleListenInComming(pMgr,pMgr->nSocketTcp);
            }
        }
	}
	//Common_DList_DeleteAll(pMgr->socketTcpList);
	closeSocket(pMgr->nSocketTcp);
    LOGW("thread exit\n");
	return 0;
}


S32 Update_Tcp_Start(UpdateMgr_T *pMgr)
{
	if (pMgr->nListenPort_Tcp <= 0 ||
		pMgr->nListenPort_Tcp >= 0xFFFF)
	{
		return -1;
	}
	//Common_DList_Init(&pMgr->socketTcpList, TcpSockFree);
	//MultiAddrCreateAndListen(pMgr->socketTcpList,TCP,pMgr->nListenPort_Tcp);
    pMgr->nSocketTcp = UpdateOpenSocket(TCP, pMgr->nListenPort_Tcp, NULL, NULL);
	// 开启本地服务监听线程
	if(Common_Thread_Create(&pMgr->hThread_Tcp,__FUNCTION__,0,COMMON_THREAD_CREATEFLAG_DETACH,static_Module_Thread_Listen,pMgr))
	{

		return -1;
	}
	return 0;
}

S32 Update_Tcp_Stop()
{
	return 0;
}

S32 Update_Require(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
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
		UPDATE_DEBUG("InParam<%s>",pStrValue = Common_Json_Print(pInParams,NULL));
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
						if (pInParams != NULL)
						{
							UPDATE_DEBUG("OutParam<%s>",pRecvBuffer + tHeader.uExternHeaderLen);
						}
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

					pSendBuffer = Common_Json_Print(pInParams,&nSendDataSize);
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
