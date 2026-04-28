#include "libcommon_api.h"
#include "libcommon_struct.h"
static int FindCRLF(char *pStr,int nMaxLen)
{
	int i = 0;
	char c;
	c = pStr[i];
	while (c != '\0')
	{
		if (c == '\r' || c == '\n')
		{
			return i;
		}
		i++;
		if (nMaxLen >= 0 && i >= nMaxLen)
		{
			break;;
		}
		c = pStr[i];
	}
	return -1;

}
S32 Common_Http_Parse(S8 *pRecvBuffer,S32 nDataLen,CommonHttpContext_T *pHttpContext)
{
	S32 nRet = 0;
	int nCrlf = 0,nNextPos = 0;
	S8 *pNewLine = NULL;
	S32 nLeftSize = 0,nUseSize = 0;
	if (pRecvBuffer == NULL || nDataLen <= 0)
	{ 
		return nUseSize;
	}
	if (pHttpContext->pHttpLine == NULL)
	{
		pHttpContext->pHttpLine = (S8 *)Common_Malloc(HTTP_RECV_BUFFER_SIZE,0,__FUNCTION__,__LINE__);
		if (pHttpContext->pHttpLine == NULL)
		{

			return -1;
		}
		pHttpContext->nHttpLineLen = HTTP_RECV_BUFFER_SIZE - 1;
		pHttpContext->pHttpLine[pHttpContext->nHttpLineLen] = 0;
		pHttpContext->nHttpLinePos = 0;
	}
	if (pHttpContext->nHttpStep == 3)
	{// 
		if (pHttpContext->bContextMalloc)
		{
			Common_Free(pHttpContext->pContext,__FUNCTION__,__LINE__);
			pHttpContext->pContext = NULL;

			pHttpContext->bContextMalloc = 0;
			pHttpContext->nHttpLinePos = 0;
			pHttpContext->nDealLinePos = 0;
		}
		else
		{

		}
		pHttpContext->nHttpStep = 0;
		pHttpContext->bKeepAlive = 0;

		pHttpContext->nContextNeedLen = 0;
		pHttpContext->nContextLen = 0;
	}
	if(pHttpContext->nHttpStep == 0 || pHttpContext->nHttpStep == 1)
	{
		//
		if (pHttpContext->nHttpLinePos >= pHttpContext->nHttpLineLen)
		{
			if (pHttpContext->nDealLinePos == 0)
			{// 满缓冲，且未处理过，说明是有问题的
				return -1;
			}
			memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + pHttpContext->nDealLinePos,pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos);

			pHttpContext->nHttpLinePos -= pHttpContext->nDealLinePos;
			pHttpContext->nDealLinePos = 0;
		}
		//recv
		//nRet = recv(nSocket,pHttpContext->pHttpLine + pHttpContext->nHttpLinePos,pHttpContext->nHttpLineLen - pHttpContext->nHttpLinePos,0);
		nRet = pHttpContext->nHttpLineLen - pHttpContext->nHttpLinePos;
		if (nRet > nDataLen - nUseSize)
		{
			nRet = nDataLen - nUseSize;
		}
		memcpy(pHttpContext->pHttpLine + pHttpContext->nHttpLinePos,pRecvBuffer + nUseSize,nRet);
		nUseSize += nRet;
		
		pHttpContext->nHttpLinePos += nRet;
		pHttpContext->pHttpLine[pHttpContext->nHttpLinePos] = 0;
	}
	else if (pHttpContext->nHttpStep == 2)
	{// 正文
		//recv
		nRet = pHttpContext->nContextNeedLen - pHttpContext->nContextLen;
		if (nRet > nDataLen - nUseSize)
		{
			nRet = nDataLen - nUseSize;
		}
		memcpy(pHttpContext->pContext + pHttpContext->nContextLen,pRecvBuffer + nUseSize,nRet);
		nUseSize += nRet;

		
		pHttpContext->nContextLen += nRet;
		pHttpContext->pContext[pHttpContext->nContextLen] = 0;
		if (!pHttpContext->bContextMalloc)
		{
			pHttpContext->nHttpLinePos += nRet;
			pHttpContext->nDealLinePos += nRet;
		}

		if (pHttpContext->nContextLen == pHttpContext->nContextNeedLen)
		{
			pHttpContext->nHttpStep = 3;
			return nUseSize;
		}
	}
	if (pHttpContext->nHttpStep == 0)
	{// GET PUT POST DELETE

		pNewLine = pHttpContext->pHttpLine + pHttpContext->nDealLinePos;
		nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
		if (nLeftSize < 7)
		{
			return nUseSize;
		}
		if (0 == Common_StrniCmp(pNewLine,(S8 *)"GET ",4))
		{
			pHttpContext->nHttpMethod = 0;
			nNextPos = 4;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"PUT ",4))
		{
			pHttpContext->nHttpMethod = 1;
			nNextPos = 4;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"POST ",5))
		{
			pHttpContext->nHttpMethod = 2;
			nNextPos = 5;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"DELETE ",7))
		{
			pHttpContext->nHttpMethod = 3;
			nNextPos = 7;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"HTTP/",5))
		{
			pHttpContext->bResponce = 1;
			nNextPos = 0;

		}
		else
		{

			return -1;
		}

		nCrlf = FindCRLF(pNewLine,nLeftSize);
		if (nCrlf < 0)
		{
			if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
			{// 一行太长!

				return -1;
			}
			return nUseSize; 
		}

		if (pNewLine[nCrlf] != '\r')
		{

			return -1;
		}
		if (nLeftSize == nCrlf + 1)
		{
			if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
			{// 一行太长!

				return -1;
			}
			return nUseSize;
		}
		if(pNewLine[nCrlf + 1] != '\n')
		{

			return -1;
		}
		// 找到一行!处理之
		// 请求行
		char *pstrTmp = NULL;
		S32 len;
		if (pHttpContext->bResponce)
		{
			pstrTmp = strchr(pHttpContext->pHttpLine + nNextPos,' ');
			len = pstrTmp - (pHttpContext->pHttpLine + nNextPos);
			pHttpContext->pHttpVer = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			// 错误码
			nNextPos += len + 1;
			pstrTmp = strchr(pHttpContext->pHttpLine + nNextPos,' ');
			len = pstrTmp - (pHttpContext->pHttpLine + nNextPos);
			pHttpContext->pHttpCode = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			// 错误描述
			nNextPos += len + 1;
			len = nCrlf - nNextPos;
			pHttpContext->pCodeDescribe = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			if (pHttpContext->pHttpCode != NULL)
			{
				pHttpContext->nHttpCode = atoi(pHttpContext->pHttpCode);
				if (pHttpContext->nHttpCode == 0 && pHttpContext->pHttpCode[0] != '0' && pHttpContext->pHttpCode[1] != '\0')
				{
					pHttpContext->nHttpCode = -1;
				}
			}
			else
			{
				pHttpContext->nHttpCode = -1;
			}
		}
		else
		{
			pHttpContext->pHttpMethod = Common_StrnDup(pNewLine,nNextPos - 1,__FUNCTION__,__LINE__);
			//URI
			pstrTmp = strchr(pHttpContext->pHttpLine + nNextPos,' ');
			len = pstrTmp - (pHttpContext->pHttpLine + nNextPos);
			if (len <= 0)
			{
				return -1;
			}
			pHttpContext->pHttpUri = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			// http ver
			nNextPos += len + 1;
			len = nCrlf - nNextPos;
			pHttpContext->pHttpVer = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
		}
		// 删除处理的行
		pHttpContext->nDealLinePos = nCrlf + 2;
		//memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + nCrlf + 2,pHttpContext->nHttpLinePos - nCrlf - 2);
		//pHttpContext->nHttpLinePos -= (nCrlf + 2);
		pHttpContext->nHttpStep = 1;





	}
	if (pHttpContext->nHttpStep == 1)
	{// 请求头处理

		do
		{
			pNewLine = pHttpContext->pHttpLine + pHttpContext->nDealLinePos;
			nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
			nCrlf = FindCRLF(pNewLine,nLeftSize);
			if (nCrlf < 0)
			{
				if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
				{// 一行太长!

					return -1;
				}
				return nUseSize; 
			}

			if (pNewLine[nCrlf] != '\r')
			{

				return -1;
			}
			if (nLeftSize == nCrlf + 1)
			{
				if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
				{// 一行太长!

					return -1;
				}
				return nUseSize;
			}
			if(pNewLine[nCrlf + 1] != '\n')
			{

				return -1;
			}
			if (nCrlf == 0)
			{// 头部结束，开始数据部分
				pHttpContext->nDealLinePos += nCrlf + 2;
				nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
				if (nLeftSize - nCrlf - 2 > 0)
				{
					//pHttpContext->nDealLinePos += nCrlf + 2;
					//nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
					//memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + nCrlf + 2,pHttpContext->nHttpLinePos - nCrlf - 2);
				}
				//pHttpContext->nHttpLinePos -= (nCrlf + 2);
				pHttpContext->nHttpStep = 2;
				if (pHttpContext->nContextNeedLen > 0)
				{
					if (pHttpContext->nHttpLineLen - pHttpContext->nDealLinePos >= pHttpContext->nContextNeedLen)
					{
						pHttpContext->pContext = pHttpContext->pHttpLine + pHttpContext->nDealLinePos;
						pHttpContext->nContextLen = nLeftSize;
						pHttpContext->bContextMalloc = 0;
						pHttpContext->nHttpStep = 3;
						pHttpContext->nDealLinePos += nLeftSize;
					}
					else
					{
						pHttpContext->pContext = (S8 *)Common_Malloc(pHttpContext->nContextNeedLen + 1,0,__FUNCTION__,__LINE__);
						if (pHttpContext->pContext == NULL)
						{
							return -1;
						}
						pHttpContext->bContextMalloc = 1;
						pHttpContext->pContext[pHttpContext->nContextNeedLen] = 0;
						if (pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos > 0)
						{
							memcpy(pHttpContext->pContext,pHttpContext->pHttpLine + pHttpContext->nDealLinePos,nLeftSize);
							pHttpContext->nContextLen = nLeftSize;
							pHttpContext->nDealLinePos += nLeftSize;
							if (pHttpContext->nContextLen < pHttpContext->nContextNeedLen )
							{
								if (nUseSize < nDataLen)
								{
									S32 nCopyLen = pHttpContext->nContextNeedLen - pHttpContext->nContextLen ;
									if (nCopyLen > nDataLen - nUseSize)
									{
										nCopyLen = nDataLen - nUseSize;
									}
									memcpy(pHttpContext->pContext + pHttpContext->nContextLen,pRecvBuffer + nUseSize,nCopyLen);
									nUseSize += nCopyLen;
									pHttpContext->nContextLen += nCopyLen;
									if (!pHttpContext->bContextMalloc)
									{
										pHttpContext->nHttpLinePos += nCopyLen;
										pHttpContext->nDealLinePos += nCopyLen;
									}

									if (pHttpContext->nContextLen == pHttpContext->nContextNeedLen)
									{
										pHttpContext->nHttpStep = 3;
									}
								}
							}
						}

					}

				}
				else if(nLeftSize > 0)
				{
					return -1;
				}
				else
				{
					pHttpContext->nHttpStep = 3;
				}
				break;
			}
			// 找到一行!处理之
			// 请求行
			char *pstrTmp = NULL,*pName = NULL,*pValue = NULL;
			//键值对
			nNextPos = 0;
			// 键
			pstrTmp = strchr(pNewLine + nNextPos,':');
			S32 len = pstrTmp - (pNewLine + nNextPos);
			if (len <= 0)
			{

				return -1;
			}
			pName = Common_StrnDup(pNewLine,len,__FUNCTION__,__LINE__);
			if (pName == NULL)
			{//

				return -1;
			}
			// 值
			nNextPos = len + 2;
			len = nCrlf - nNextPos;
			pValue = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			if (pValue == NULL)
			{//

				if (pName != NULL)
				{
					Common_Free(pName,__FUNCTION__,__LINE__);
					pName = NULL;
				}
				return -1;
			}
			// 检查长度
			if (0 == Common_StriCmp(pName,(S8 *)"Content-Length"))
			{
				if (pValue != NULL)
				{
					pHttpContext->nContextNeedLen = atoi(pValue);


				}

			}
			else if (0 == Common_StriCmp(pName,(S8 *)"Connection") &&
               0 == Common_StriCmp(pValue,(S8 *)"keep-alive"))
			{

				pHttpContext->bKeepAlive = 1;

			}
			if (pHttpContext->nKeyValueCount < 64)
			{
				pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = pName;
				pName = NULL;
				pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = pValue;
				pValue = NULL;
				pHttpContext->nKeyValueCount++;
			}






			if (pName != NULL)
			{
				Common_Free(pName,__FUNCTION__,__LINE__);
				pName = NULL;
			}
			if (pValue != NULL)
			{
				Common_Free(pValue,__FUNCTION__,__LINE__);
				pValue = NULL;
			}

			// 删除处理的行
			pHttpContext->nDealLinePos += nCrlf + 2;

			//memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + nCrlf + 2,pHttpContext->nHttpLinePos - nCrlf - 2);
			//pHttpContext->nHttpLinePos -= (nCrlf + 2);

		}while (pHttpContext->nHttpLinePos > 0);

	}



	return nUseSize;
}

S32 Common_Http_Recv(S32 nSocket,CommonHttpContext_T *pHttpContext)
{
	S32 nRet = 0;
	int nCrlf = 0,nNextPos = 0;
	S8 *pNewLine = NULL;
	S32 nLeftSize = 0;
	if (pHttpContext->pHttpLine == NULL)
	{
		pHttpContext->pHttpLine = (S8 *)Common_Malloc(HTTP_RECV_BUFFER_SIZE,0,__FUNCTION__,__LINE__);
		if (pHttpContext->pHttpLine == NULL)
		{

			return -1;
		}
		pHttpContext->nHttpLineLen = HTTP_RECV_BUFFER_SIZE - 1;
		pHttpContext->pHttpLine[pHttpContext->nHttpLineLen] = 0;
		pHttpContext->nHttpLinePos = 0;
	}
	if (pHttpContext->nHttpStep == 3)
	{// 
		if (pHttpContext->bContextMalloc)
		{
			Common_Free(pHttpContext->pContext,__FUNCTION__,__LINE__);
			pHttpContext->pContext = NULL;

			pHttpContext->bContextMalloc = 0;
			pHttpContext->nHttpLinePos = 0;
			pHttpContext->nDealLinePos = 0;
		}
		else
		{

		}
		pHttpContext->nHttpStep = 0;
		pHttpContext->bKeepAlive = 0;

		pHttpContext->nContextNeedLen = 0;
		pHttpContext->nContextLen = 0;
	}
	if(pHttpContext->nHttpStep == 0 || pHttpContext->nHttpStep == 1)
	{
		//
		if (pHttpContext->nHttpLinePos >= pHttpContext->nHttpLineLen)
		{
			if (pHttpContext->nDealLinePos == 0)
			{// 满缓冲，且未处理过，说明是有问题的
				return -1;
			}
			memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + pHttpContext->nDealLinePos,pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos);

			pHttpContext->nHttpLinePos -= pHttpContext->nDealLinePos;
			pHttpContext->nDealLinePos = 0;
		}
		//recv
		nRet = recv(nSocket,pHttpContext->pHttpLine + pHttpContext->nHttpLinePos,pHttpContext->nHttpLineLen - pHttpContext->nHttpLinePos,0);
		if (nRet == 0)
		{

			return -1;
		}
		else if (nRet < 0)
		{
			S32 errorNo = GetLastError();
			if (errorNo == EWOULDBLOCK ||
				errorNo == EINTR||
				errorNo == EAGAIN ||
				errorNo == ETIMEDOUT)
			{

				return 0;
			}
			else
			{

				return -1;
			}
		}
		pHttpContext->nHttpLinePos += nRet;
		pHttpContext->pHttpLine[pHttpContext->nHttpLinePos] = 0;
	}
	else if (pHttpContext->nHttpStep == 2)
	{// 正文
		//recv
		nRet = recv(nSocket,pHttpContext->pContext + pHttpContext->nContextLen,pHttpContext->nContextNeedLen - pHttpContext->nContextLen,0);
		if (nRet == 0)
		{

			return -1;
		}
		else if (nRet < 0)
		{
			S32 errorNo = GetLastError();
			if (errorNo == EWOULDBLOCK ||
				errorNo == EINTR||
				errorNo == EAGAIN ||
				errorNo == ETIMEDOUT)
			{

				return 0;
			}
			else
			{

				return -1;
			}
		}
		pHttpContext->nContextLen += nRet;
		pHttpContext->pContext[pHttpContext->nContextLen] = 0;
		if (!pHttpContext->bContextMalloc)
		{
			pHttpContext->nHttpLinePos += nRet;
			pHttpContext->nDealLinePos += nRet;
		}

		if (pHttpContext->nContextLen == pHttpContext->nContextNeedLen)
		{
			pHttpContext->nHttpStep = 3;
			return 0;
		}
	}
	if (pHttpContext->nHttpStep == 0)
	{// GET PUT POST DELETE

		pNewLine = pHttpContext->pHttpLine + pHttpContext->nDealLinePos;
		nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
		if (nLeftSize < 7)
		{
			return 0;
		}
		if (0 == Common_StrniCmp(pNewLine,(S8 *)"GET ",4))
		{
			pHttpContext->nHttpMethod = 0;
			nNextPos = 4;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"PUT ",4))
		{
			pHttpContext->nHttpMethod = 1;
			nNextPos = 4;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"POST ",5))
		{
			pHttpContext->nHttpMethod = 2;
			nNextPos = 5;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"DELETE ",7))
		{
			pHttpContext->nHttpMethod = 3;
			nNextPos = 7;

		}
		else if (0 == Common_StrniCmp(pNewLine,(S8 *)"HTTP/",5))
		{
			pHttpContext->bResponce = 1;
			nNextPos = 0;

		}
		else
		{

			return -1;
		}

		nCrlf = FindCRLF(pNewLine,nLeftSize);
		if (nCrlf < 0)
		{
			if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
			{// 一行太长!

				return -1;
			}
			return 0; 
		}

		if (pNewLine[nCrlf] != '\r')
		{

			return -1;
		}
		if (nLeftSize == nCrlf + 1)
		{
			if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
			{// 一行太长!

				return -1;
			}
			return 0;
		}
		if(pNewLine[nCrlf + 1] != '\n')
		{

			return -1;
		}
		// 找到一行!处理之
		// 请求行
		char *pstrTmp = NULL;
		S32 len;
		if (pHttpContext->bResponce)
		{
			pstrTmp = strchr(pHttpContext->pHttpLine + nNextPos,' ');
			len = pstrTmp - (pHttpContext->pHttpLine + nNextPos);
			pHttpContext->pHttpVer = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			// 错误码
			nNextPos += len + 1;
			pstrTmp = strchr(pHttpContext->pHttpLine + nNextPos,' ');
			len = pstrTmp - (pHttpContext->pHttpLine + nNextPos);
			pHttpContext->pHttpCode = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			// 错误描述
			nNextPos += len + 1;
			len = nCrlf - nNextPos;
			pHttpContext->pCodeDescribe = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			if (pHttpContext->pHttpCode != NULL)
			{
				pHttpContext->nHttpCode = atoi(pHttpContext->pHttpCode);
			}
		}
		else
		{
			pHttpContext->pHttpMethod = Common_StrnDup(pNewLine,nNextPos - 1,__FUNCTION__,__LINE__);
			//URI
			pstrTmp = strchr(pHttpContext->pHttpLine + nNextPos,' ');
			len = pstrTmp - (pHttpContext->pHttpLine + nNextPos);
			if (len <= 0)
			{
				return -1;
			}
			pHttpContext->pHttpUri = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			// http ver
			nNextPos += len + 1;
			len = nCrlf - nNextPos;
			pHttpContext->pHttpVer = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
		}
		// 删除处理的行
		pHttpContext->nDealLinePos = nCrlf + 2;
		//memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + nCrlf + 2,pHttpContext->nHttpLinePos - nCrlf - 2);
		//pHttpContext->nHttpLinePos -= (nCrlf + 2);
		pHttpContext->nHttpStep = 1;





	}
	if (pHttpContext->nHttpStep == 1)
	{// 请求头处理

		do
		{
			pNewLine = pHttpContext->pHttpLine + pHttpContext->nDealLinePos;
			nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
			nCrlf = FindCRLF(pNewLine,nLeftSize);
			if (nCrlf < 0)
			{
				if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
				{// 一行太长!

					return -1;
				}
				return 0; 
			}

			if (pNewLine[nCrlf] != '\r')
			{

				return -1;
			}
			if (nLeftSize == nCrlf + 1)
			{
				if (pHttpContext->nHttpLinePos == pHttpContext->nHttpLineLen)
				{// 一行太长!

					return -1;
				}
				return 0;
			}
			if(pNewLine[nCrlf + 1] != '\n')
			{

				return -1;
			}
			if (nCrlf == 0)
			{// 头部结束，开始数据部分
				pHttpContext->nDealLinePos += nCrlf + 2;
				nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
				if (nLeftSize - nCrlf - 2 > 0)
				{
					//pHttpContext->nDealLinePos += nCrlf + 2;
					//nLeftSize = pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos;
					//memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + nCrlf + 2,pHttpContext->nHttpLinePos - nCrlf - 2);
				}
				//pHttpContext->nHttpLinePos -= (nCrlf + 2);
				pHttpContext->nHttpStep = 2;
				if (pHttpContext->nContextNeedLen > 0)
				{
					if (pHttpContext->nHttpLineLen - pHttpContext->nDealLinePos >= pHttpContext->nContextNeedLen)
					{
						pHttpContext->pContext = pHttpContext->pHttpLine + pHttpContext->nDealLinePos;
						pHttpContext->nContextLen = nLeftSize;
						pHttpContext->bContextMalloc = 0;
						pHttpContext->nHttpStep = 3;
						pHttpContext->nDealLinePos += nLeftSize;
					}
					else
					{
						pHttpContext->pContext = (S8 *)Common_Malloc(pHttpContext->nContextNeedLen + 1,0,__FUNCTION__,__LINE__);
						if (pHttpContext->pContext == NULL)
						{
							return -1;
						}
						pHttpContext->bContextMalloc = 1;
						pHttpContext->pContext[pHttpContext->nContextNeedLen] = 0;
						if (pHttpContext->nHttpLinePos - pHttpContext->nDealLinePos > 0)
						{
							memcpy(pHttpContext->pContext,pHttpContext->pHttpLine + pHttpContext->nDealLinePos,nLeftSize);
							pHttpContext->nContextLen = nLeftSize;
							pHttpContext->nDealLinePos += nLeftSize;
						}

					}

				}
				else if(nLeftSize > 0)
				{
					return -1;
				}
				else
				{
					pHttpContext->nHttpStep = 3;
				}
				break;
			}
			// 找到一行!处理之
			// 请求行
			char *pstrTmp = NULL,*pName = NULL,*pValue = NULL;
			//键值对
			nNextPos = 0;
			// 键
			pstrTmp = strchr(pNewLine + nNextPos,':');
			S32 len = pstrTmp - (pNewLine + nNextPos);
			if (len <= 0)
			{

				return -1;
			}
			pName = Common_StrnDup(pNewLine,len,__FUNCTION__,__LINE__);
			if (pName == NULL)
			{//

				return -1;
			}
			// 值
			nNextPos = len + 2;
			len = nCrlf - nNextPos;
			pValue = Common_StrnDup(pNewLine + nNextPos,len,__FUNCTION__,__LINE__);
			if (pValue == NULL)
			{//

				if (pName != NULL)
				{
					Common_Free(pName,__FUNCTION__,__LINE__);
					pName = NULL;
				}
				return -1;
			}
			// 检查长度
			if (0 == Common_StriCmp(pName,(S8 *)"Content-Length"))
			{
				if (pValue != NULL)
				{
					pHttpContext->nContextNeedLen = atoi(pValue);


				}

			}
			else if (0 == Common_StriCmp(pName,(S8 *)"Connection") &&
               0 == Common_StriCmp(pValue,(S8 *)"keep-alive"))
			{

				pHttpContext->bKeepAlive = 1;

			}
			if (pHttpContext->nKeyValueCount < 64)
			{
				pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = pName;
				pName = NULL;
				pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = pValue;
				pValue = NULL;
				pHttpContext->nKeyValueCount++;
			}

			




			if (pName != NULL)
			{
				Common_Free(pName,__FUNCTION__,__LINE__);
				pName = NULL;
			}
			if (pValue != NULL)
			{
				Common_Free(pValue,__FUNCTION__,__LINE__);
				pValue = NULL;
			}

			// 删除处理的行
			pHttpContext->nDealLinePos += nCrlf + 2;

			//memmove(pHttpContext->pHttpLine,pHttpContext->pHttpLine + nCrlf + 2,pHttpContext->nHttpLinePos - nCrlf - 2);
			//pHttpContext->nHttpLinePos -= (nCrlf + 2);

		}while (pHttpContext->nHttpLinePos > 0);

	}



	return 0;
}

S8 *Common_Http_Http2String(CommonHttpContext_T *pHttpContext,S32 *lpSize)
{
	S32 i;
	if (pHttpContext == NULL)
	{
		return NULL;
	}
	if (pHttpContext->pSendBuffer != NULL)
	{// 
		Common_Free(pHttpContext->pSendBuffer,__FUNCTION__,__LINE__);
		pHttpContext->pSendBuffer = NULL;
		pHttpContext->nSendSize = 0;
		pHttpContext->nSendPos = 0;
		pHttpContext->nSendBuffSize = 0;
	}
		// 计算大小
		pHttpContext->pSendBuffer = (S8 *)Common_Malloc(pHttpContext->nContextLen + 1024 + 1,0,__FUNCTION__,__LINE__);
		if (pHttpContext->pSendBuffer == NULL)
		{
			return NULL;
		}
		pHttpContext->nSendBuffSize = pHttpContext->nContextLen + 1024;
		// 头
		if (pHttpContext->bResponce)
		{
			// 请求行
			pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
				pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
				"%s %s %s\r\n",
				pHttpContext->pHttpVer?pHttpContext->pHttpVer:"",
				pHttpContext->pHttpCode?pHttpContext->pHttpCode:"",
				pHttpContext->pCodeDescribe?pHttpContext->pCodeDescribe:"");
		}
		else
		{
			pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
				pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
				"%s %s %s\r\n",
				pHttpContext->pHttpMethod?pHttpContext->pHttpMethod:"",
				pHttpContext->pHttpUri?pHttpContext->pHttpUri:"",
				pHttpContext->pHttpVer?pHttpContext->pHttpVer:"");
		}
		for (i = 0; i < pHttpContext->nKeyValueCount;i++)
		{
			if (pHttpContext->tKeyValue[i].pKey != NULL && 
				pHttpContext->tKeyValue[i].pValue != NULL)
			{
				pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
					pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
					"%s: %s\r\n",
					pHttpContext->tKeyValue[i].pKey,
					pHttpContext->tKeyValue[i].pValue);
			}

		}
		// 请求头完毕
		pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
			pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
			"\r\n");
		if (pHttpContext->nContextLen > 0)
		{
			memcpy(pHttpContext->pSendBuffer + pHttpContext->nSendSize,pHttpContext->pContext,pHttpContext->nContextLen);
			pHttpContext->nSendSize += pHttpContext->nContextLen;
		}
	
	if (lpSize != NULL)
	{
		*lpSize = pHttpContext->nSendSize;
	}
	
	
	return pHttpContext->pSendBuffer;
}

S32 Common_Http_Send(S32 nSocket,CommonHttpContext_T *pHttpContext)
{
	S32 i;
	if (pHttpContext == NULL)
	{
		return -1;
	}
	if (pHttpContext->pSendBuffer == NULL)
	{// 
		// 计算大小
		pHttpContext->pSendBuffer = (S8 *)Common_Malloc(pHttpContext->nContextLen + 1024 + 1,0,__FUNCTION__,__LINE__);
		if (pHttpContext->pSendBuffer == NULL)
		{
			return -1;
		}
		pHttpContext->nSendBuffSize = pHttpContext->nContextLen + 1024;
		// 头
		if (pHttpContext->bResponce)
		{
			// 请求行
			pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
				pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
				"%s %s %s\r\n",
				pHttpContext->pHttpVer?pHttpContext->pHttpVer:"",
				pHttpContext->pHttpCode?pHttpContext->pHttpCode:"",
				pHttpContext->pCodeDescribe?pHttpContext->pCodeDescribe:"");
		}
		else
		{
			pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
				pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
				"%s %s %s\r\n",
				pHttpContext->pHttpMethod?pHttpContext->pHttpMethod:"",
				pHttpContext->pHttpUri?pHttpContext->pHttpUri:"",
				pHttpContext->pHttpVer?pHttpContext->pHttpVer:"");
		}
		for (i = 0; i < pHttpContext->nKeyValueCount;i++)
		{
			if (pHttpContext->tKeyValue[i].pKey != NULL && 
				pHttpContext->tKeyValue[i].pValue != NULL)
			{
				pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
					pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
					"%s: %s\r\n",
					pHttpContext->tKeyValue[i].pKey,
					pHttpContext->tKeyValue[i].pValue);
			}

		}
		// 请求头完毕
		pHttpContext->nSendSize += snprintf(pHttpContext->pSendBuffer + pHttpContext->nSendSize,
			pHttpContext->nSendBuffSize - pHttpContext->nSendSize,
			"\r\n");
		if (pHttpContext->nContextLen > 0)
		{
			memcpy(pHttpContext->pSendBuffer + pHttpContext->nSendSize,pHttpContext->pContext,pHttpContext->nContextLen);
			pHttpContext->nSendSize += pHttpContext->nContextLen;
		}
	}
	if (pHttpContext->nSendSize > pHttpContext->nSendPos)
	{
		S32 nRet;
		nRet = send(nSocket,pHttpContext->pSendBuffer + pHttpContext->nSendPos,pHttpContext->nSendSize - pHttpContext->nSendPos,0);
		if (nRet < 0)
		{
			return -1;
		}
		pHttpContext->nSendPos += nRet;
		if (pHttpContext->nSendPos >= pHttpContext->nSendSize)
		{
			pHttpContext->nHttpStep = 3;
		}
	}
	return 0;
}

S32 Common_Http_Http2Json(CommonHttpContext_T *pHttpContext,cJSON_Struct **pJson)
{
	cJSON_Struct *pHttpJson = NULL,*pSubItem = NULL;
	S32 i;
	// S32 bJson = 0;
	pHttpJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pHttpJson == NULL || pJson == NULL || *pJson != NULL)
	{
		return -1;
	}
	// header
	pSubItem = Common_Json_SetAttrValue(pHttpJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	if (pHttpContext->bResponce)
	{
		Common_Json_SetAttrValue(pHttpJson,-1,"Header/Code",Common_Json_Type_Number,NULL,pHttpContext->nHttpCode,0);
		Common_Json_SetAttrValue(pHttpJson,-1,"Header/Describe",Common_Json_Type_String,pHttpContext->pCodeDescribe?pHttpContext->pCodeDescribe:"",0,0);
	}
	else
	{
		Common_Json_SetAttrValue(pHttpJson,-1,"Header/Method",Common_Json_Type_String,pHttpContext->pHttpMethod?pHttpContext->pHttpMethod:"",0,0);
		Common_Json_SetAttrValue(pHttpJson,-1,"Header/Uri",Common_Json_Type_String,pHttpContext->pHttpUri?pHttpContext->pHttpUri:"",0,0);
	}
	Common_Json_SetAttrValue(pHttpJson,-1,"Header/HttpVer",Common_Json_Type_String,pHttpContext->pHttpVer?pHttpContext->pHttpVer:"",0,0);
	for (i = 0; i < pHttpContext->nKeyValueCount;i++)
	{
      if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"Content-Length"))
		{
		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"Connection"))
		{
			Common_Json_SetAttrValue(pHttpJson,-1,"Header/Connection",Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);
		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"Content-Type"))
		{
			if (strstr(pHttpContext->tKeyValue[i].pValue,"json"))
			{
				// bJson = 1;
			}
		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"From"))
		{
			cJSON_Struct *pFromJson = NULL;
			pFromJson = Common_Json_GetItem(pHttpJson,-1,"Header/From");
			if (pFromJson == NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Header/From",Common_Json_Type_Object,NULL,0,0);
			}
			Common_Json_SetAttrValue(pHttpJson,-1,"Header/From/Email",Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);
			
		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"Referer"))
		{
			cJSON_Struct *pFromJson = NULL;
			pFromJson = Common_Json_GetItem(pHttpJson,-1,"Header/From");
			if (pFromJson == NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Header/From",Common_Json_Type_Object,NULL,0,0);
			}
			Common_Json_SetAttrValue(pHttpJson,-1,"Header/From/Uri",Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);

		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"Forwarder"))
		{
			cJSON_Struct *pFromJson = NULL;
			pFromJson = Common_Json_GetItem(pHttpJson,-1,"Header/From");
			if (pFromJson == NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Header/From",Common_Json_Type_Object,NULL,0,0);
			}
			Common_Json_SetAttrValue(pHttpJson,-1,"Header/From/ForwarderUri",Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);

		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"RedirectUri"))
		{
			cJSON_Struct *pFromJson = NULL;
			pFromJson = Common_Json_GetItem(pHttpJson,-1,"Header/From");
			if (pFromJson == NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Header/From",Common_Json_Type_Object,NULL,0,0);
			}
			Common_Json_SetAttrValue(pHttpJson,-1,"Header/From/RedirectUri",Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);

		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"/OrginUri"))
		{
			cJSON_Struct *pFromJson = NULL;
			pFromJson = Common_Json_GetItem(pHttpJson,-1,"Header/From");
			if (pFromJson == NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Header/From",Common_Json_Type_Object,NULL,0,0);
			}
			Common_Json_SetAttrValue(pHttpJson,-1,"Header/From/OrginUri",Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);

		}
      else if (0 == Common_StriCmp(pHttpContext->tKeyValue[i].pKey,(S8 *)"Authorization"))
		{
			if (pHttpContext->tKeyValue[i].pValue != NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth",Common_Json_Type_Object,NULL,0,0);
				if (0 == Common_StrniCmp(pHttpContext->tKeyValue[i].pValue,(S8 *)"Basic ",6))
				{// base64
					U32 len = 0;
					S8 *pUserName = NULL,*pPass = NULL;
					Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Method",Common_Json_Type_Number,NULL,1,0);
					pUserName = Common_Base64_Decode(pHttpContext->tKeyValue[i].pValue + strlen("Basic "),strlen(pHttpContext->tKeyValue[i].pValue) - strlen("Basic "),&len);
					if (pUserName != NULL)
					{
						pPass = strchr(pUserName,':');
						if (pPass != NULL)
						{
							pPass[0] = 0;
							pPass++;
							Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/UserName",Common_Json_Type_String,pUserName,0,0);
							Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Password",Common_Json_Type_String,pPass,0,0);
						}
						else
						{
							Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/UserName",Common_Json_Type_String,pUserName,0,0);
						}
						Common_Free(pUserName,__FUNCTION__,__LINE__);
						pUserName = NULL;
					}
				}
				else if (0 == Common_StrniCmp(pHttpContext->tKeyValue[i].pValue,(S8 *)"Digest ",7))
				{
					// 取username
					S8 *pStr,*pStr1,*pValue,*pOrgString;
					Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Method",Common_Json_Type_Number,NULL,2,0);

					Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest",Common_Json_Type_Object,NULL,0,0);
					pOrgString = Common_StrDup(pHttpContext->tKeyValue[i].pValue,__FUNCTION__,__LINE__);
					if (pOrgString != NULL)
					{
						pStr = strstr(pOrgString,"username=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("username=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/UserName",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"realm=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("realm=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Realm",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"nonce=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("nonce=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Nonce",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"uri=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("uri=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Uri",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"response=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("response=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Response",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"opaque=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("opaque=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Opaque",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"nc=");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("nc=");
							if (pValue[0] == '\"')
							{
								pValue++;
								pStr1 = strchr(pValue,'\"');
								if (pStr1 != NULL)
								{
									pStr1[0] = 0;
									Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Nc",Common_Json_Type_String,pValue,0,0);
									pStr1[0] = '\"';
								}
								
							}
							else
							{
								pStr1 = strchr(pValue,',');
								if (pStr1 != NULL)
								{
									pStr1[0] = 0;
								}
								
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Nc",Common_Json_Type_String,pValue,0,0);
								if (pStr1 != NULL)
								{
									pStr1[0] = ',';
								}
								
							}
							
						}
						pStr = strstr(pOrgString,"cnonce=\"");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("cnonce=\"");
							pStr1 = strchr(pValue,'\"');
							if (pStr1 != NULL)
							{
								pStr1[0] = 0;
								Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Cnonce",Common_Json_Type_String,pValue,0,0);
								pStr1[0] = '\"';
							}
						}
						pStr = strstr(pOrgString,"qop=");
						if (pStr != NULL)
						{
							pValue = pStr + strlen("qop=");
							if (pValue[0] == '\"')
							{
								pValue++;
								pStr1 = strchr(pValue,'\"');
								if (pStr1 != NULL)
								{
									pStr1[0] = 0;
									Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Qop",Common_Json_Type_String,pValue,0,0);
									pStr1[0] = '\"';
								}
							}
							else
							{
								pStr1 = strchr(pValue,',');
								if (pStr1 != NULL)
								{
									pStr1[0] = 0;
								}
									Common_Json_SetAttrValue(pHttpJson,-1,"Header/Auth/Digest/Qop",Common_Json_Type_String,pValue,0,0);
								if(pStr1 != NULL)
								{
									pStr1[0] = ',';
								}
							}
							
							
						}
						Common_Free(pOrgString,__FUNCTION__,__LINE__);
					}
					
				}
			}
		}
		else
		{
			// 直接加到头部
			if (pHttpContext->tKeyValue[i].pKey != NULL && 
				pHttpContext->tKeyValue[i].pValue != NULL)
			{
				Common_Json_SetAttrValue(pSubItem,-1,pHttpContext->tKeyValue[i].pKey,Common_Json_Type_String,pHttpContext->tKeyValue[i].pValue,0,0);
			}
		}
	}
	// 数据部分
	if (pHttpContext->pContext != NULL)
	{
		cJSON_Struct *pDataJson = NULL;

		pDataJson = Common_Json_Parse(pHttpContext->pContext,NULL,NULL);
		if (pDataJson != NULL)
		{
			Common_Json_AddItem(pHttpJson,-1,"Data",pDataJson);
		}
		else
		{
			S8 *pOtherData = NULL;
			U32 Len = 0;
			pOtherData = Common_Base64_Encode(pHttpContext->pContext,pHttpContext->nContextLen,&Len);
			if (pOtherData != NULL)
			{
				Common_Json_SetAttrValue(pHttpJson,-1,"Data",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pHttpJson,-1,"Data/NoJsonData",Common_Json_Type_String,pOtherData,0,0);
				Common_Free(pOtherData,__FUNCTION__,__LINE__);
				pOtherData = NULL;
			}
		}

	}
	*pJson = pHttpJson;

	return 0;
}

S32 Common_Http_Init(CommonHttpContext_T *pHttpContext)
{
	if (pHttpContext == NULL)
	{
		return 0;
	}
	memset(pHttpContext,0,sizeof(CommonHttpContext_T));
	return 0;
}


S32 Common_Http_Free(CommonHttpContext_T *pHttpContext)
{
    int i = 0;

	if (pHttpContext == NULL)
	{
		return 0;
	}
	if (pHttpContext->pHttpLine != NULL)
	{
		Common_Free(pHttpContext->pHttpLine,__FUNCTION__,__LINE__);
		pHttpContext->pHttpLine = NULL;
	}
	if (pHttpContext->pHttpUri != NULL)
	{
		Common_Free(pHttpContext->pHttpUri,__FUNCTION__,__LINE__);
		pHttpContext->pHttpUri = NULL;
	}
	if (pHttpContext->pHttpMethod != NULL)
	{
		Common_Free(pHttpContext->pHttpMethod,__FUNCTION__,__LINE__);
		pHttpContext->pHttpMethod = NULL;
	}
	if (pHttpContext->pHttpVer != NULL)
	{
		Common_Free(pHttpContext->pHttpVer,__FUNCTION__,__LINE__);
		pHttpContext->pHttpVer = NULL;
	}
	if (pHttpContext->pHttpCode != NULL)
	{
		Common_Free(pHttpContext->pHttpCode,__FUNCTION__,__LINE__);
		pHttpContext->pHttpCode = NULL;
	}
	if (pHttpContext->pCodeDescribe != NULL)
	{
		Common_Free(pHttpContext->pCodeDescribe,__FUNCTION__,__LINE__);
		pHttpContext->pCodeDescribe = NULL;
	}

	for (i = 0; i < pHttpContext->nKeyValueCount;i++)
	{
		if (pHttpContext->tKeyValue[i].pKey != NULL)
		{
			Common_Free(pHttpContext->tKeyValue[i].pKey,__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[i].pKey = NULL;
		}
		if (pHttpContext->tKeyValue[i].pValue != NULL)
		{
			Common_Free(pHttpContext->tKeyValue[i].pValue,__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[i].pValue = NULL;
		}
	}
	if (pHttpContext->bContextMalloc)
	{
		if (pHttpContext->pContext != NULL)
		{
			Common_Free(pHttpContext->pContext,__FUNCTION__,__LINE__);
			pHttpContext->pContext = NULL;
		}
	}
	if (pHttpContext->pSendBuffer != NULL)
	{
		Common_Free(pHttpContext->pSendBuffer,__FUNCTION__,__LINE__);
		pHttpContext->pSendBuffer = NULL;

	}
	memset(pHttpContext,0,sizeof(CommonHttpContext_T));
	return 0;
}

S32 Common_Http_Json2Http(cJSON_Struct *pJson,CommonHttpContext_T *pHttpContext)
{
	cJSON_Struct *pHttpJson = NULL;
	S8 *pStringValue = NULL;
	S32 nIntValue = 0;
	F64 fFloatValue = 0;
	S8 szString[256];
	S8 *pErrorDes = NULL;
	if (pJson == NULL || pHttpContext == NULL)
	{
		return -1;
	}
	// 头部
	pHttpJson = Common_Json_GetItem(pJson,-1,"/Header");
	if (pHttpJson == NULL)
	{
		return -1;
	}
	nIntValue = 0;
	if(Common_Json_GetAttrValue(pJson,-1,"/Header/Code",NULL,NULL,&nIntValue,NULL))
	{
		// 
		pHttpContext->bResponce = 1;
		if (nIntValue == COMMON_ERROR_TYPE_SUCC)
		{
			nIntValue = Common_ResResponceStatusCode_Ok;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_PARTIAL_SUCCESS)
		{
			nIntValue = Common_ResResponceStatusCode_PartialContent;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_INVALIDPARAM)
		{
			nIntValue = Common_ResResponceStatusCode_BadRequest;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_UNINITED)
		{
			nIntValue = Common_ResResponceStatusCode_InternalServerError;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_TIMEOUT)
		{
			nIntValue = Common_ResResponceStatusCode_RequestTimeOut;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_BUSY)
		{
			nIntValue = Common_ResResponceStatusCode_Processing;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_AGAIN)
		{
			nIntValue = Common_ResResponceStatusCode_ServerUnavailable;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_LIMITED)
		{
			nIntValue = Common_ResResponceStatusCode_Forbidden;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_MISMATCH)
		{
			nIntValue = Common_ResResponceStatusCode_NotFound;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_NOTFOUND)
		{
			nIntValue = Common_ResResponceStatusCode_NotFound;
		}
		else if (nIntValue == COMMON_ERROR_TYPE_UNKWON)
		{
			nIntValue = Common_ResResponceStatusCode_BadRequest;
		}
		else
		{
			
			 pErrorDes = Common_FindErrString(nIntValue);
			 if (pErrorDes != NULL && 0 == Common_StriCmp(pErrorDes,(S8 *)"Unassigned"))
			 {
				 nIntValue = Common_ResResponceStatusCode_BadRequest;
			 }
			 
		}
		sprintf(szString,"%d",nIntValue);
		pHttpContext->nHttpCode = nIntValue;
		pHttpContext->pHttpCode = Common_StrDup(szString,__FUNCTION__,__LINE__);
		pStringValue = NULL;
		if(Common_Json_GetAttrValue(pJson,-1,"/Header/Describe",NULL,&pStringValue,NULL,NULL))
		{
			if (pStringValue != NULL)
			{
				pHttpContext->pCodeDescribe = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
			}
			else
			{
          pHttpContext->pCodeDescribe = Common_StrDup((S8 *)"Unassigned",__FUNCTION__,__LINE__);
			}
			
		}
		else
		{
			pErrorDes = Common_FindErrString(nIntValue);
		
			if(pErrorDes != NULL)
			{
				pHttpContext->pCodeDescribe = Common_StrDup(pErrorDes,__FUNCTION__,__LINE__);
			}
			else
			{

          pHttpContext->pCodeDescribe = Common_StrDup((S8 *)"Unassigned",__FUNCTION__,__LINE__);
			}
		}
	}
	
	if(Common_Json_GetAttrValue(pJson,-1,"/Header/Uri",NULL,&pStringValue,NULL,NULL))
	{
		pHttpContext->bResponce = 0;
		pHttpContext->pHttpUri = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
	}
	if(Common_Json_GetAttrValue(pJson,-1,"/Header/Method",NULL,&pStringValue,NULL,NULL))
	{
		pHttpContext->bResponce = 0;

		if (0 == Common_StriCmp(pStringValue,(S8 *)"Get"))
		{
			pHttpContext->nHttpMethod = 0;
			pHttpContext->pHttpMethod = Common_StrDup((S8 *)"GET",__FUNCTION__,__LINE__);
		}
		else if (0 == Common_StriCmp(pStringValue,(S8 *)"Put"))
		{
			pHttpContext->nHttpMethod = 1;
			pHttpContext->pHttpMethod = Common_StrDup((S8 *)"PUT",__FUNCTION__,__LINE__);
		}
		else if (0 == Common_StriCmp(pStringValue,(S8 *)"Post"))
		{
			pHttpContext->nHttpMethod = 2;
			pHttpContext->pHttpMethod = Common_StrDup((S8 *)"POST",__FUNCTION__,__LINE__);
		}
		else if (0 == Common_StriCmp(pStringValue,(S8 *)"Delete"))
		{
			pHttpContext->nHttpMethod = 3;
			pHttpContext->pHttpMethod = Common_StrDup((S8 *)"DELETE",__FUNCTION__,__LINE__);
		}
		else
		{
			return -1;
		}
	}
	// http ver
	pStringValue = NULL;
	if(Common_Json_GetAttrValue(pJson,-1,"/Header/HttpVer",NULL,&pStringValue,NULL,NULL))
	{
		pHttpContext->pHttpVer = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);

	}
	// auth
	pStringValue = NULL;
	if(Common_Json_GetAttrValue(pJson,-1,"/Header/Auth",NULL,&pStringValue,NULL,NULL))
	{
		if (Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Method",NULL,NULL,&nIntValue,NULL))
		{
			char *pUserName = NULL;
			Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/UserName",NULL,&pUserName,NULL,NULL);
			if (nIntValue == 1)
			{// base64
				char *pBase64 = NULL;
				int nLen;
				S8 *pPassword = NULL;
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Password",NULL,&pPassword,NULL,NULL);
				nLen = sprintf(szString,"%s:%s",pUserName?pUserName:"",pPassword?pPassword:"");
				pBase64 = Common_Base64_Encode(szString,nLen,NULL);
				if (pBase64 != NULL)
				{
					char *pszString = NULL;
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Authorization",__FUNCTION__,__LINE__);
					pszString = (S8 *)Common_Malloc(nLen + 1 + 6,0,__FUNCTION__,__LINE__);
					if (pszString != NULL)
					{
						sprintf(pszString,"Basic %s",pBase64);
						pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = pszString;
						Common_Free(pszString,__FUNCTION__,__LINE__);
					}
					pHttpContext->nKeyValueCount++;

					Common_Free(pBase64,__FUNCTION__,__LINE__);
					pBase64 = NULL;
				}
			}
			else if (nIntValue == 2)
			{// digest
				S8 *szRealm = NULL,*szQop = NULL,*szNonce = NULL,*szOpaque = NULL,
					*szCnonce=NULL,*szAuthUri=NULL,*szResponse=NULL,*szNc = NULL;
				S32 nLen = 0;
				S8 *pDigestString;
				
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Realm",NULL,&szRealm,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Qop",NULL,&szQop,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Nonce",NULL,&szNonce,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Opaque",NULL,&szOpaque,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Cnonce",NULL,&szCnonce,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Uri",NULL,&szAuthUri,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Resopnse",NULL,&szResponse,NULL,NULL);
				Common_Json_GetAttrValue(pJson,-1,"/Header/Auth/Digest/Nc",NULL,&szNc,NULL,NULL);
				pDigestString = (S8 *)Common_Malloc(1024 + 1,0,__FUNCTION__,__LINE__);
				if (pDigestString != NULL)
				{
					pDigestString[1024] = 0;
					nLen += sprintf(pDigestString + nLen,"Digest ");
					if (pUserName != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"username=\"%s\"",pUserName);
					}
					if (szRealm != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"realm=\"%s\"",szRealm);
					}
					if (szNonce != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"nonce=\"%s\"",szNonce);
					}
					if (szAuthUri != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"uri=\"%s\"",szAuthUri);
					}
					if (szResponse != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"response=\"%s\"",szResponse);
					}
					if (szOpaque != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"opaque=\"%s\"",szOpaque);
					}
					if (szNc != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"nc=\"%s\"",szNc);
					}
					if (szCnonce != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"cnonce=\"%s\"",szCnonce);
					}
					if (szQop != NULL)
					{
						nLen += snprintf(pDigestString + nLen,1024 - nLen,"qop=\"%s\"",szQop);
					}
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Authorization",__FUNCTION__,__LINE__);
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pDigestString,__FUNCTION__,__LINE__);
					
					pHttpContext->nKeyValueCount++;
					Common_Free(pDigestString,__FUNCTION__,__LINE__);
					pDigestString = NULL;
				}


			}
			else if (nIntValue == 3)
			{// username-token
			}
		}
	}
	// From
	pStringValue = NULL;
	if(Common_Json_GetAttrValue(pJson,-1,"/Header/From",NULL,&pStringValue,NULL,NULL))
	{
		S8 *pEmail = NULL,*pReferer = NULL,*pForwarder = NULL,*pOrginUri = NULL,*pRedirectUri = NULL;
		Common_Json_GetAttrValue(pHttpJson,-1,"Header/From/Email",NULL,&pEmail,NULL,NULL);
		Common_Json_GetAttrValue(pHttpJson,-1,"Header/From/Uri",NULL,&pReferer,NULL,NULL);
		Common_Json_GetAttrValue(pHttpJson,-1,"Header/From/ForwarderUri",NULL,&pForwarder,NULL,NULL);
		Common_Json_GetAttrValue(pHttpJson,-1,"Header/From/OrginUri",NULL,&pOrginUri,NULL,NULL);
		Common_Json_GetAttrValue(pHttpJson,-1,"Header/From/RedirectUri",NULL,&pRedirectUri,NULL,NULL);
		if (pEmail != NULL)
		{
        pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"From",__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pEmail,__FUNCTION__,__LINE__);
			pHttpContext->nKeyValueCount++;
		}
		if (pReferer != NULL)
		{
        pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Referer",__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pReferer,__FUNCTION__,__LINE__);
			pHttpContext->nKeyValueCount++;
		}
		if (pForwarder != NULL)
		{
        pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Forwarder",__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pForwarder,__FUNCTION__,__LINE__);
			pHttpContext->nKeyValueCount++;
		}
		if (pOrginUri != NULL)
		{
        pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"OrginUri",__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pOrginUri,__FUNCTION__,__LINE__);
			pHttpContext->nKeyValueCount++;
		}
		if (pRedirectUri != NULL)
		{
        pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"RedirectUri",__FUNCTION__,__LINE__);
			pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pRedirectUri,__FUNCTION__,__LINE__);
			pHttpContext->nKeyValueCount++;
		}

	}
	// 其他的头
	cJSON_Struct *pHeadFirst = Common_Json_GetFirstChild(pHttpJson);
	S8 *pName = NULL,*pValue = NULL;
	S32 bHasType = 0;
	S32 nType = -1;
	while(pHeadFirst != NULL)
	{

		if(0 == Common_Json_GetAttr(pHeadFirst,NULL,&pName,&nType,&pValue,&nIntValue,&fFloatValue))
		{
			if (pName != NULL)
			{
				// 过滤掉已经处理的
          if (0 == Common_StriCmp(pName,(S8 *)"Uri") || 
              0 == Common_StriCmp(pName,(S8 *)"HttpVer") || 
              0 == Common_StriCmp(pName,(S8 *)"Describe") || 
              0 == Common_StriCmp(pName,(S8 *)"Code")|| 
              0 == Common_StriCmp(pName,(S8 *)"Method")|| 
              0 == Common_StriCmp(pName,(S8 *)"Auth")||
              0 == Common_StriCmp(pName,(S8 *)"Content-Length") ||
              0 == Common_StriCmp(pName,(S8 *)"From"))
				{
				}
          else if (0 == Common_StriCmp(pName,(S8 *)"Content-Type"))
				{
					bHasType = 1;
				}
				else if (nType == Common_Json_Type_String)
				{
            if (0 == Common_StriCmp(pName,(S8 *)"Connection"))
					{
						
						if (pValue != NULL)
						{
                if (0 == Common_StriCmp(pValue,(S8 *)"Close"))
							{
								pHttpContext->bKeepAlive = 0;
							}
							else
							{
								pHttpContext->bKeepAlive = 1;
							}
						}
						// bHasConn = 1;
					}
					
					if (pValue != NULL)
					{
						pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup(pName,__FUNCTION__,__LINE__);
						pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(pValue,__FUNCTION__,__LINE__);
						pHttpContext->nKeyValueCount++;
					}
				}
				else if (nType == Common_Json_Type_Number)
				{
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup(pName,__FUNCTION__,__LINE__);
					sprintf(szString,"%d",nIntValue);
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(szString,__FUNCTION__,__LINE__);
					pHttpContext->nKeyValueCount++;
				}
				else if (nType == Common_Json_Type_Double)
				{
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup(pName,__FUNCTION__,__LINE__);
					sprintf(szString,"%f",fFloatValue);
					pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(szString,__FUNCTION__,__LINE__);
					pHttpContext->nKeyValueCount++;
				}
			}

		}
		pHeadFirst = Common_Json_GetNext(pHeadFirst);
	}
	// 数据
	pHttpJson = Common_Json_GetItem(pJson,-1,"/Data");
	if (pHttpJson != NULL)
	{
		pStringValue = NULL;
		Common_Json_GetAttrValue(pJson,-1,"/Data/NoJsonData",NULL,&pStringValue,NULL,NULL);
		if (pStringValue != NULL)
		{
			pHttpContext->pContext =Common_Base64_Decode(pStringValue,strlen(pStringValue),(U32 *)&pHttpContext->nContextLen);
			if (!bHasType)
			{
          pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Content-Type",__FUNCTION__,__LINE__);
				sprintf(szString,"%d",pHttpContext->nContextLen);
				pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup((S8 *)"text/plain; charset=UTF-8",__FUNCTION__,__LINE__);
				pHttpContext->nKeyValueCount++;
			}
		}
		else
		{
			pHttpContext->pContext = Common_Json_PrintUnformatted(pHttpJson,&pHttpContext->nContextLen);
			if (!bHasType)
			{
          pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Content-Type",__FUNCTION__,__LINE__);
				sprintf(szString,"%d",pHttpContext->nContextLen);
				pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup((S8 *)"application/json; charset=UTF-8",__FUNCTION__,__LINE__);
				pHttpContext->nKeyValueCount++;
			}
		}
		if (pHttpContext->pContext != NULL)
		{
			pHttpContext->bContextMalloc = 1;
		}
	}
	// 补充字段
	if (pHttpContext->pHttpVer == NULL)
	{
      pHttpContext->pHttpVer = Common_StrDup((S8 *)"HTTP/1.1",__FUNCTION__,__LINE__);
	}
	// 长度字段

	pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pKey = Common_StrDup((S8 *)"Content-Length",__FUNCTION__,__LINE__);
	sprintf(szString,"%d",pHttpContext->nContextLen);
	pHttpContext->tKeyValue[pHttpContext->nKeyValueCount].pValue = Common_StrDup(szString,__FUNCTION__,__LINE__);
	pHttpContext->nKeyValueCount++;

	



	return 0;
}
