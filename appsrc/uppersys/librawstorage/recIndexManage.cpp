#include "recIndexManage.h"
#include "globalObj.h"

#define READ_RECINDEX_PERSIZE  64*KBYTE_UNIT

CRecIndexManage*	CRecIndexManage::theRecIndexList = NULL;

CRecIndexManage*CRecIndexManage::GetMgrItem()
{
	if (NULL == theRecIndexList)
	{
		theRecIndexList = new CRecIndexManage();

		if ( NULL == theRecIndexList )
		{
			return NULL;
		}
	}
	return theRecIndexList;
}

void CRecIndexManage::DelMgrItem()
{
	DEL_POINTER(theRecIndexList);
}

CRecIndexManage::CRecIndexManage()
{
	m_lpDateList = new antsavi_recdate_list_t;
	if(NULL !=  m_lpDateList)
	{
		m_lpDateList->iNodeCount=0;
		m_lpDateList->head=NULL;
		m_lpDateList->tail=NULL;
	}

	m_recindex_fd = new avis_fileHanedle;

	m_delRecIndex_fd = new avis_fileHanedle;

	memset(&m_recDateIndexInfo, 0, sizeof(m_recDateIndexInfo));

	m_ListMutex.Create();

	m_indexInfoMutex.Create();
}

CRecIndexManage::~CRecIndexManage( )
{
	if(NULL != m_recindex_fd)
	{
		ANTS_AVI_CLOSE(m_recindex_fd);
		DEL_POINTER(m_recindex_fd);
	}

	if(NULL != m_delRecIndex_fd)
	{
		ANTS_AVI_CLOSE(m_delRecIndex_fd);
		DEL_POINTER(m_delRecIndex_fd);
	}

	if(NULL != m_lpDateList)
	{
		antsavi_recdate_node_t *temp1=NULL;
		antsavi_recdate_node_t *temp2=NULL;
		antsavi_recDateInfo *node=NULL;

		temp1 = m_lpDateList->head;
		temp2 = m_lpDateList->head;

		while(temp1!=NULL){
			temp2 = temp1->next ;
			node = temp1->element;
			DEL_POINTER(node);
			DEL_POINTER(temp1);
			m_lpDateList->iNodeCount--;
			temp1=temp2;
		}

		m_lpDateList->head = NULL;
		m_lpDateList->tail = NULL;
		m_lpDateList->iNodeCount = 0;
	}

	m_ListMutex.Close();

	m_indexInfoMutex.Close();

	DEL_POINTER(m_lpDateList);
}

int CRecIndexManage::PrintRecDateList()
{
	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pNodeInfo = NULL;
	node = m_lpDateList->head;

	if(NULL == node)
	{
		return avis_ret_ok;
	}

	char temp[20];
	int i=0;

	AVI_DBG("========================the RecDateIndex List nodecount[%d]=================", m_lpDateList->iNodeCount);

	while(NULL !=  node)
	{
		pNodeInfo = node->element;

		if(!node->element)
			break;
		GetStrFromTime(pNodeInfo->tDate, temp);

		AVI_DBG("node[%d] startAddr[%llu] filesize[%lld] date[%s]", i,pNodeInfo->startAddr, pNodeInfo->file_size, temp);

		i++;
		node = node->next;
	}
	printf("\n");

	return avis_ret_ok;
}


int CRecIndexManage::RecoverFromFile()
{
	if(NULL == m_recindex_fd)
	{
		return avis_ret_failed;
	}

	memset(&m_recDateIndexInfo, 0, sizeof(m_recDateIndexInfo));

	avis_time_info tTime;
	memset(&tTime, 0, sizeof(tTime));
	tTime.wYear = 2000;
	tTime.wMonth = 13;
	tTime.wDay = 31;
	mktime_utc(&tTime , &m_recDateIndexInfo.tDate);

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();

	avis_SDCardHead tSDCardHead;
	memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	//打开录像索引文件
	m_recindex_fd->fd = 0;
	m_recindex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_recindex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	int nRet = ANTS_AVI_OPEN(m_recindex_fd, io_mode_rw, FALSE);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();
		return avis_ret_failed;
	}

	m_delRecIndex_fd->fd = 0;
	m_delRecIndex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_delRecIndex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	nRet = ANTS_AVI_OPEN(m_delRecIndex_fd, io_mode_rw, FALSE);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();
		return avis_ret_failed;
	}



	//判断文件索引区域是否损坏
	S64 offset = m_recindex_fd->startAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
	if(offset % REC_INDEX_NODE_LEN != 0)
	{
		OUTPUT_FUNC_LINE;
		m_recindex_fd->startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
		m_recindex_fd->filesize = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
	}

	ANTS_AVI_SEEK(m_recindex_fd, 0, SEEK_SET);

	avis_address recIndexAddr;
	avis_address recFileAddr;

	recIndexAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
	recIndexAddr.filesize = 0;

	recFileAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr;
	recFileAddr.filesize = 0;

	int iFindFirstValid = 0;
	S64 iReadFileSize = 0;

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	RecIndexInfo *pIndexNode = NULL;

	RecIndexInfo IndexNodeTemp;
	memset(&IndexNodeTemp, 0, sizeof(RecIndexInfo));

	int i=0;

	//记录索引出错的位置，如果是最后几个或者一个出错，应当去掉
	avis_address addr_error;
	memset(&addr_error, 0, sizeof(avis_address));

	char chStart[8]={0};
	char chEnd[8]={0};
	memset(chStart, 0, sizeof(chStart));
	memset(chEnd, 0, sizeof(chEnd));
	snprintf(chStart, sizeof(chStart), "recInS");
	snprintf(chEnd, sizeof(chEnd), "recInE");
	while (m_recindex_fd->filesize > 0)
	{
		S64 iRemainSize = m_recindex_fd->filesize-iReadFileSize;

		if(m_recindex_fd->startAddr + iReadFileSize >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //日期索引超过文件大小
		{
			offset = m_recindex_fd->startAddr + iReadFileSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
			lseek64(m_recindex_fd->fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset, SEEK_SET);
		}
		else
		{
			offset = m_recindex_fd->startAddr + iReadFileSize;
			lseek64(m_recindex_fd->fd, offset, SEEK_SET);
		}

		S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

		if(m_recindex_fd->startAddr + iReadFileSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
			m_recindex_fd->startAddr + iReadFileSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
		{
			iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - m_recindex_fd->startAddr - iReadFileSize;
		}

		AVI_DBG("iRemainSize=%lld iCanRead = %lld  m_recindex_fd->startAddr=%lld filesize=%lld\n",iRemainSize,iCanRead,m_recindex_fd->startAddr, m_recindex_fd->filesize);

		memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
		S64 iByteRead = ANTS_AVI_READ(m_recindex_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

		//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
		for(i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
		{
			pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

			//AVI_DBG("[%d] time[%d-%d] addr=%lld size=%d usesize=%d\n",i,(int)pIndexNode->begin_sec,(int)pIndexNode->end_sec,pIndexNode->startAddr,pIndexNode->file_size,pIndexNode->use_size);
			#if 0
			int iCount = 0;
			char szBegin[32] = {0},szEnd[32] = {0};
			GetStrFromTime(pIndexNode->begin_sec, szBegin);
			GetStrFromTime(pIndexNode->end_sec, szEnd);
			AVI_WAR("[%d]pIndexNode:%d[%s]=>%d[%s],startAddr:%lld,file_size:%d,use_size:%d\n",iCount++,pIndexNode->begin_sec,szBegin,pIndexNode->end_sec,szEnd,pIndexNode->startAddr,pIndexNode->file_size,pIndexNode->use_size);
			#endif
			if(strncmp(pIndexNode->startMask, chStart, sizeof(pIndexNode->startMask))==0 && strncmp(pIndexNode->endMask, chEnd, sizeof(pIndexNode->endMask))==0 )
			{
				if(pIndexNode->file_size >= 0) //有效索引
				{
					memcpy(&IndexNodeTemp, pIndexNode, sizeof(RecIndexInfo));

					AddOneRecIndexNodeToDateList(pIndexNode, FALSE, m_recindex_fd->startAddr+iReadFileSize);
					//根据日期索引来求录像索引的起始地址和大小
					if(iFindFirstValid == 0)
					{
						iFindFirstValid = 1;
						recIndexAddr.startAddr = m_recindex_fd->startAddr+iReadFileSize;
						recFileAddr.startAddr = pIndexNode->startAddr;

						AVI_DBG("first pIndexNode->startAddr=%lld file_size=%d use_size=%d\n",pIndexNode->startAddr, pIndexNode->file_size, pIndexNode->use_size);

						AVI_DBG("m_recindex_fd->startAddr=%lld iReadFileSize=%lld\n",m_recindex_fd->startAddr,iReadFileSize);
					}

					//recFileAddr.filesize += pIndexNode->use_size;
				}
				else
				{
					if(m_recDateIndexInfo.file_size != 0)
					{
						m_recDateIndexInfo.file_size += REC_INDEX_NODE_LEN;
						UpdateOneRecDateIndexNode(&m_recDateIndexInfo);
					}
					AVI_DBG("pIndexNode.startAddr=%lld  pIndexNode.file_size=%d\n",pIndexNode->startAddr,pIndexNode->file_size);
				}
				memset(&addr_error, 0, sizeof(addr_error));
			}
			else
			{
				//AVI_DBG("pIndexNode.startMask error i=%d  iByteRead/sizeof(RecIndexInfo)= %lld | %d = %lld!\n", i, iByteRead,sizeof(RecIndexInfo), iByteRead/sizeof(RecIndexInfo));
				//AVI_DBG("pIndexNode->startMask = %s  endMask= %s\n", pIndexNode->startMask, pIndexNode->endMask);
				//AVI_PrintBuffer(pIndexNode->startMask, sizeof(pIndexNode->startMask));
				//AVI_PrintBuffer(pIndexNode->endMask, sizeof(pIndexNode->endMask));
				if(m_recDateIndexInfo.file_size != 0)
				{
					m_recDateIndexInfo.file_size += REC_INDEX_NODE_LEN;
					UpdateOneRecDateIndexNode(&m_recDateIndexInfo);
				}
				AVI_DBG("addr_error.startAddr = %lld  filesize=%lld m_recindex_fd->startAddr=%lld  iReadFileSize=%lld\n", addr_error.startAddr, addr_error.filesize, m_recindex_fd->startAddr, iReadFileSize);

				if(addr_error.startAddr + addr_error.filesize == m_recindex_fd->startAddr+iReadFileSize)
				{
					addr_error.filesize += REC_INDEX_NODE_LEN;
				}
				else if(addr_error.startAddr == 0 && addr_error.filesize == 0)
				{
					addr_error.startAddr = m_recindex_fd->startAddr+iReadFileSize;
					addr_error.filesize = REC_INDEX_NODE_LEN;
				}
			}

			iReadFileSize += REC_INDEX_NODE_LEN;
		}

		if(iReadFileSize >= m_recindex_fd->filesize)
		{
			AVI_DBG("iReadFileSize:%lld,filesize:%lld!!!\n",iReadFileSize,m_recindex_fd->filesize);
			break;
		}

		if(addr_error.filesize >= READ_RECINDEX_PERSIZE)
		{
			iFindFirstValid = 0;
			AVI_ERR("there is no match data in 64K data!!!\n");
			break;
		}

		//若检测64K数据都没有正确的索引，那么则视为SD卡损坏
		if(iFindFirstValid == 0)
		{
			AVI_ERR("there is no match data in 64K data!!!\n");
			break;
		}
	}

	if(strncmp(IndexNodeTemp.startMask, chStart, sizeof(IndexNodeTemp.startMask))==0 && strncmp(IndexNodeTemp.endMask, chEnd, sizeof(IndexNodeTemp.endMask))==0 &&
		IndexNodeTemp.startAddr > 0 &&IndexNodeTemp.file_size >= 0)
	{
		AVI_DBG("last pIndexNode->startAddr=%lld file_size=%d use_size=%d\n",IndexNodeTemp.startAddr, IndexNodeTemp.file_size, IndexNodeTemp.use_size);

		if(IndexNodeTemp.startAddr < recFileAddr.startAddr)
		{
			AVI_DBG("the last node addr is less than the first!!!!\n");
			recFileAddr.filesize = g_antsAviLibInfo.antsStorageManage->m_SDCardTotalSize - recFileAddr.startAddr + IndexNodeTemp.startAddr + IndexNodeTemp.use_size - g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr;
		}
		else
		{
			recFileAddr.filesize = IndexNodeTemp.startAddr + IndexNodeTemp.use_size - recFileAddr.startAddr;
		}
	}

	AVI_DBG("addr_error[%lld  %lld]  recIndexAddr[%lld  %lld]\n", addr_error.startAddr, addr_error.filesize, recIndexAddr.startAddr, iReadFileSize);
	if(addr_error.startAddr == recIndexAddr.startAddr)
	{
		recIndexAddr.filesize = iReadFileSize - addr_error.filesize;
	}
	else
	{
		recIndexAddr.filesize = iReadFileSize;
	}

	memcpy(&tSDCardHead.tRecFileIndexAddr, &recIndexAddr, sizeof(avis_address));
	memcpy(&tSDCardHead.tRecFileAddr, &recFileAddr, sizeof(avis_address));
	int iRet = g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);

	if(avis_ret_failed == iRet || iFindFirstValid == 0 || recIndexAddr.filesize == 0)
	{
		recIndexAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
		recIndexAddr.filesize = 0;

		recFileAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr;
		recFileAddr.filesize = 0;

		memcpy(&tSDCardHead.tRecFileIndexAddr, &recIndexAddr, sizeof(avis_address));
		memcpy(&tSDCardHead.tRecFileAddr, &recFileAddr, sizeof(avis_address));
		g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);

		if(NULL != m_lpDateList)
		{
			antsavi_recdate_node_t *temp1=NULL;
			antsavi_recdate_node_t *temp2=NULL;
			antsavi_recDateInfo *node=NULL;

			temp1 = m_lpDateList->head;
			temp2 = m_lpDateList->head;

			while(temp1!=NULL){
				temp2 = temp1->next ;
				node = temp1->element;
				DEL_POINTER(node);
				DEL_POINTER(temp1);
				m_lpDateList->iNodeCount--;
				temp1=temp2;
			}
		}
		m_lpDateList->head = NULL;
		m_lpDateList->tail = NULL;
		m_lpDateList->iNodeCount = 0;

	}

	m_recindex_fd->startAddr = recIndexAddr.startAddr;
	m_recindex_fd->filesize = recIndexAddr.filesize;

	AVI_DBG("recover ok !!! tSDCardHead.startMark=%s  tSDCardHead.endMark =%s\n"
				"tSDCardHead.tRecFileIndexAddr=[%lld - %lld]\n"
				"tSDCardHead.tRecFileAddr=[%lld - %lld] \n",
				tSDCardHead.startMark, tSDCardHead.endMark,
				tSDCardHead.tRecFileIndexAddr.startAddr,tSDCardHead.tRecFileIndexAddr.filesize,
				tSDCardHead.tRecFileAddr.startAddr,tSDCardHead.tRecFileAddr.filesize);

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	//查找最新的日期索引赋给  m_recDateIndexInfo
	if(NULL != m_lpDateList && NULL != m_lpDateList->tail && 0 != m_lpDateList->iNodeCount)
	{
		antsavi_recDateInfo *pNode = m_lpDateList->tail->element;

		memset(&m_recDateIndexInfo, 0, sizeof(m_recDateIndexInfo));

		if(NULL != pNode)
		{
			m_recDateIndexInfo.startAddr = pNode->startAddr;
			m_recDateIndexInfo.file_size = pNode->file_size;
			m_recDateIndexInfo.tDate = pNode->tDate;
		}
	}
	//PrintRecDateList();

	DEL_ARR_POINTER(pBuffer);

	return avis_ret_ok;
}

int CRecIndexManage::GetIFrameCountInFile(time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pCount)
{
	if(NULL == m_lpDateList)
	{
		*pCount = 0;
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pDateNode = NULL;

	RecIndexInfo *pIndexNode = NULL;
	unsigned long long nCount = 0;

	//m_ListMutex.Wait();

	node = m_lpDateList->head;

	if(NULL == node)
	{
		*pCount = 0;
		//m_ListMutex.Release();
		return avis_ret_ok;
	}

	avis_fileHanedle read_fd;
	read_fd.fd = 0;
	read_fd.startAddr = 0;
	read_fd.filesize = 0;

	ANTS_AVI_OPEN(&read_fd, io_mode_r, FALSE);

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		*pCount = 0;
		OUTPUT_FUNC_LINE;
		ANTS_AVI_CLOSE(&read_fd);
		return avis_ret_failed;
	}

	S64 iReadSize = 0;

	char startMask[8] , endMask[8];

	snprintf(startMask, sizeof(startMask), "recInS");
	snprintf(endMask, sizeof(endMask), "recInE");


	//遍历日期索引链表
	while(NULL !=  node)
	{
		pDateNode = node->element;
		if(!node->element)
			break;
		if(!compareYMD(pDateNode->tDate, tBeginTime) && !compareYMD(pDateNode->tDate, tEndTime))
		{
			node = node->next;
			continue;
		}

		avis_SDCardHead tSDCardHead;
		memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
		g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);
		if(!(((pDateNode->startAddr >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize)) ||
			((pDateNode->startAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize))))
		{
			node = node->next;
			continue;
		}

		read_fd.startAddr = pDateNode->startAddr;
		read_fd.filesize = pDateNode->file_size;

		iReadSize = 0;

		//AVI_DBG("time[%d %d] read_fd  startAddr=%lld filesize=%lld\n",tBeginTime, tEndTime,read_fd.startAddr, read_fd.filesize);

		while(iReadSize < read_fd.filesize)
		{
			S64 iRemainSize = read_fd.filesize-iReadSize;

			if(read_fd.startAddr + iReadSize  >=
				g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
			{
				S64 offset = read_fd.startAddr + iReadSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
				lseek64(read_fd.fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset, SEEK_SET);
			}
			else
			{
				ANTS_AVI_SEEK(&read_fd, iReadSize, SEEK_SET);
			}

			S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

			if(read_fd.startAddr + iReadSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
				read_fd.startAddr + iReadSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - read_fd.startAddr - iReadSize;
			}

			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			S64 iByteRead = ANTS_AVI_READ(&read_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

			//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
			for(S64 i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
			{
				pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

				if(strncmp(pIndexNode->startMask, startMask, sizeof(startMask))!=0 || strncmp(pIndexNode->endMask, endMask, sizeof(endMask))!=0)
				{
					continue;
				}

				if(pIndexNode->begin_sec > tEndTime || pIndexNode->end_sec < tBeginTime || pIndexNode->begin_sec > pIndexNode->end_sec)
				{
					continue;
				}

				if(0 == (pIndexNode->file_type & iRecType))
				{
					continue;
				}

				if(pIndexNode->file_size <= 0)
				{
					continue;
				}

				//char temp1[20], temp2[20];
				//GetStrFromTime(pIndexNode->begin_sec, temp1);
				//GetStrFromTime(pIndexNode->end_sec, temp2);

				//AVI_DBG("pIndexNode time[%s-%s] startAddr=%lld file_size=%d use_size=%d\n",temp1, temp2, pIndexNode->startAddr,pIndexNode->file_size, pIndexNode->use_size);

				{
					int iTimeOffset = 0;

					if(tBeginTime - pIndexNode->begin_sec > 0)
					{
						iTimeOffset = tBeginTime - pIndexNode->begin_sec;
					}

					if(iTimeOffset > 0)//往前多读1S
					{
						iTimeOffset = iTimeOffset-1;
					}

					for(int i=iTimeOffset; i<IFRAMEOFFSETNUM; i++)
					{
						if(i>0)
						{
							//AVI_DBG("[%llu]->%d\n",nCount,pIndexNode->iFrameOffset[i]);
							if(pIndexNode->iFrameOffset[i] == pIndexNode->iFrameOffset[i-1])
							{
								continue;
							}
							nCount++;
						}
						else
						{
							nCount++;
						}
					}
				}
			}

			iReadSize += iByteRead;
		}

		node = node->next;
	}

	ANTS_AVI_CLOSE(&read_fd);
	DEL_ARR_POINTER(pBuffer);

	*pCount = nCount;
	AVI_DBG("========IFrameCount:%llu\n",(*pCount));

	//m_ListMutex.Release();

	return avis_ret_ok;
}

int CRecIndexManage::FindInfFileForList(time_t tBeginTime, time_t tEndTime, int iRecType, avis_recfileindexlist **pRecSegHead, int *pRecSegCount, BOOL bIFrame)
{
	if(NULL == m_lpDateList)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pDateNode = NULL;

	RecIndexInfo *pIndexNode = NULL;

	int seg_cnt = 0;
	avis_recfileindexlist *seg_first = NULL;
	avis_recfileindexlist *seg_head = NULL;

	//m_ListMutex.Wait();

	node = m_lpDateList->head;

	if(NULL == node)
	{
		*pRecSegCount = 0;
		//m_ListMutex.Release();
		return avis_ret_ok;
	}

	avis_fileHanedle read_fd;
	read_fd.fd = 0;
	read_fd.startAddr = 0;
	read_fd.filesize = 0;

	ANTS_AVI_OPEN(&read_fd, io_mode_r, FALSE);

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	S64 iReadSize = 0;

	char startMask[8] , endMask[8];

	snprintf(startMask, sizeof(startMask), "recInS");
	snprintf(endMask, sizeof(endMask), "recInE");


	//遍历日期索引链表
	while(NULL !=  node)
	{
		pDateNode = node->element;
		if(!node->element)
			break;
		if(compareYMD_Ex(pDateNode->tDate, tBeginTime)<0 || compareYMD_Ex(pDateNode->tDate, tEndTime) > 0)
		{
			node = node->next;
			continue;
		}

		avis_SDCardHead tSDCardHead;
		memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
		g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);
		if(!(((pDateNode->startAddr >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize)) ||
			((pDateNode->startAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize))))
		{
			node = node->next;
			continue;
		}

		read_fd.startAddr = pDateNode->startAddr;
		read_fd.filesize = pDateNode->file_size;

		iReadSize = 0;

		//AVI_DBG("time[%d %d] read_fd  startAddr=%lld filesize=%lld\n",tBeginTime, tEndTime,read_fd.startAddr, read_fd.filesize);

		while(iReadSize < read_fd.filesize)
		{
			S64 iRemainSize = read_fd.filesize-iReadSize;

			if(read_fd.startAddr + iReadSize  >=
				g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
			{
				S64 offset = read_fd.startAddr + iReadSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
				lseek64(read_fd.fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset, SEEK_SET);
			}
			else
			{
				ANTS_AVI_SEEK(&read_fd, iReadSize, SEEK_SET);
			}

			S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

			if(read_fd.startAddr + iReadSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
				read_fd.startAddr + iReadSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - read_fd.startAddr - iReadSize;
			}

			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			S64 iByteRead = ANTS_AVI_READ(&read_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

			//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
			for(S64 i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
			{
				pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

				if(strncmp(pIndexNode->startMask, startMask, sizeof(startMask))!=0 || strncmp(pIndexNode->endMask, endMask, sizeof(endMask))!=0)
				{
					continue;
				}

				if(pIndexNode->begin_sec > tEndTime || pIndexNode->end_sec < tBeginTime || pIndexNode->begin_sec > pIndexNode->end_sec)
				{
					continue;
				}

				if(0 == (pIndexNode->file_type & iRecType))
				{
					continue;
				}

				if(pIndexNode->file_size <= 0)
				{
					continue;
				}

				//char temp1[20], temp2[20];
				//GetStrFromTime(pIndexNode->begin_sec, temp1);
				//GetStrFromTime(pIndexNode->end_sec, temp2);

				//AVI_DBG("pIndexNode time[%s-%s] startAddr=%lld file_size=%d use_size=%d\n",temp1, temp2, pIndexNode->startAddr,pIndexNode->file_size, pIndexNode->use_size);

				if(bIFrame == FALSE)  //正常播放的时候
				{
					if(NULL == seg_first)
					{
						avis_recfileindexlist *seg_node = new avis_recfileindexlist;
						memset(seg_node, 0, sizeof(avis_recfileindexlist));

						seg_node->begin_sec = pIndexNode->begin_sec;
						seg_node->end_sec	= pIndexNode->end_sec;

						//查询起始时间 <= 索引节点的起始时间
						if(tBeginTime <= pIndexNode->begin_sec)
						{
							seg_node->startAddr = pIndexNode->startAddr;
							seg_node->filesize = pIndexNode->use_size;
						}
						else
						{
							int iTimeOffset = tBeginTime - pIndexNode->begin_sec;

							if(iTimeOffset > 0) //往前多读1S
							{
								seg_node->startAddr = pIndexNode->startAddr + pIndexNode->iFrameOffset[iTimeOffset-1];
								seg_node->filesize = pIndexNode->use_size - pIndexNode->iFrameOffset[iTimeOffset-1];
							}
						}

						seg_node->ptNext = NULL;

						seg_first = seg_node;
						seg_head = seg_first;

						seg_cnt++;
					}
					else
					{
						if(seg_first->startAddr + seg_first->filesize == pIndexNode->startAddr && (seg_first->end_sec == pIndexNode->begin_sec || seg_first->end_sec+1 == pIndexNode->begin_sec))
						{
							seg_first->end_sec	= pIndexNode->end_sec;

							if(pIndexNode->startAddr + pIndexNode->use_size == g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
								seg_first->filesize += pIndexNode->file_size;
							else
								seg_first->filesize += pIndexNode->use_size;
						}
						else //sd卡满的时候,或者地址不连续的时候 重新建一个索引
						{
							avis_recfileindexlist *seg_node = new avis_recfileindexlist;
							memset(seg_node, 0, sizeof(avis_recfileindexlist));

							seg_node->filesize = 0;
							seg_node->begin_sec = pIndexNode->begin_sec;
							seg_node->end_sec	= pIndexNode->end_sec;

							//查询起始时间 <= 索引节点的起始时间
							if(tBeginTime <= pIndexNode->begin_sec)
							{
								seg_node->startAddr = pIndexNode->startAddr;
								seg_node->filesize = pIndexNode->use_size;
							}
							else
							{
								int iTimeOffset = tBeginTime - pIndexNode->begin_sec;

								if(iTimeOffset > 0) //往前多读1S
								{
									seg_node->startAddr = pIndexNode->startAddr + pIndexNode->iFrameOffset[iTimeOffset-1];
									seg_node->filesize = pIndexNode->use_size - pIndexNode->iFrameOffset[iTimeOffset-1];
								}
							}

							seg_node->ptNext = NULL;

							seg_first->ptNext = seg_node;
							seg_first = seg_node;

							seg_cnt++;
						}
					}
				}
				else //只读I帧
				{
					int iTimeOffset = 0;

					if(tBeginTime - pIndexNode->begin_sec > 0)
					{
						iTimeOffset = tBeginTime - pIndexNode->begin_sec;
					}

					if(iTimeOffset > 0)//往前多读1S
					{
						iTimeOffset = iTimeOffset-1;
					}

					for(int i=iTimeOffset; i<IFRAMEOFFSETNUM; i++)
					{
						if(i>0)
						{
							if(pIndexNode->iFrameOffset[i] == pIndexNode->iFrameOffset[i-1])
							{
								continue;
							}
						}

						avis_recfileindexlist *seg_node = new avis_recfileindexlist;
						memset(seg_node, 0, sizeof(avis_recfileindexlist));

						seg_node->startAddr = pIndexNode->startAddr + pIndexNode->iFrameOffset[i];
						seg_node->begin_sec = pIndexNode->begin_sec;
						seg_node->end_sec	= pIndexNode->end_sec;

						if(i == IFRAMEOFFSETNUM-1)
						{
							seg_node->filesize = pIndexNode->use_size - pIndexNode->iFrameOffset[i];
						}
						else
						{
							seg_node->filesize = pIndexNode->iFrameOffset[i+1] - pIndexNode->iFrameOffset[i];

							if(seg_node->filesize == 0)
							{
								seg_node->filesize = pIndexNode->file_size - pIndexNode->iFrameOffset[i];
							}
						}

						seg_node->ptNext = NULL;

						if(NULL == seg_first)
						{
							seg_first = seg_node;
							seg_head = seg_first;

							seg_cnt++;
						}
						else
						{
							seg_first->ptNext = seg_node;
							seg_first = seg_node;

							seg_cnt++;
						}
					}
				}
			}

			iReadSize += iByteRead;
		}

		node = node->next;
	}

	ANTS_AVI_CLOSE(&read_fd);
	DEL_ARR_POINTER(pBuffer);


	AVI_DBG("========seg_cnt:%d\n",seg_cnt);

	avis_recfileindexlist *seg_node = seg_head;
	avis_recfileindexlist *seg_next = NULL;
	avis_recfileindexlist temp;

/*	char temp1[20], temp2[20];
	int i=0;
	while(NULL != seg_node)
	{
		GetStrFromTime(seg_node->begin_sec, temp1);
		GetStrFromTime(seg_node->end_sec, temp2);
		AVI_DBG("seg_node[%d] startAddr[%lld]  filesize[%lld] time=%s-%s\n", i, seg_node->startAddr, seg_node->filesize, temp1, temp2);
		seg_node = seg_node->ptNext;
		i++;
	}
*/
	for(seg_node=seg_head; seg_node!=NULL; seg_node=seg_node->ptNext)
	{
		for(seg_next=seg_node->ptNext;seg_next!=NULL;seg_next=seg_next->ptNext)
		{
			if(seg_node->begin_sec > seg_next->begin_sec)
			{
				temp.startAddr = seg_node->startAddr;
				temp.filesize = seg_node->filesize;
				temp.begin_sec = seg_node->begin_sec;
				temp.end_sec = seg_node->end_sec;

				seg_node->startAddr = seg_next->startAddr;
				seg_node->filesize = seg_next->filesize;
				seg_node->begin_sec = seg_next->begin_sec;
				seg_node->end_sec = seg_next->end_sec;

				seg_next->startAddr = temp.startAddr;
				seg_next->filesize = temp.filesize;
				seg_next->begin_sec = temp.begin_sec;
				seg_next->end_sec = temp.end_sec;
			}
		}
	}

	seg_node = seg_head;

/*
	i=0;
	while(NULL != seg_node)
	{
		GetStrFromTime(seg_node->begin_sec, temp1);
		GetStrFromTime(seg_node->end_sec, temp2);
		AVI_DBG("seg_node[%d] startAddr[%lld]  filesize[%lld] time=%s-%s\n", i, seg_node->startAddr, seg_node->filesize, temp1, temp2);
		seg_node = seg_node->ptNext;
		i++;
	}
*/
	*pRecSegHead = seg_head;
	*pRecSegCount = seg_cnt;

	//m_ListMutex.Release();

	return avis_ret_ok;
}

int CRecIndexManage::ReleaseList(avis_recfileindexlist *pRecSegHead)
{
	avis_recfileindexlist *seg_first = pRecSegHead;
	while(NULL != seg_first)
	{
		avis_recfileindexlist *seg_node = seg_first->ptNext;
		delete seg_first;
		seg_first = seg_node;
	}

	pRecSegHead = NULL;
	return avis_ret_ok;
}

int CRecIndexManage::FindIFrameInFileForList(time_t tBeginTime, time_t tEndTime, int iRecType, avis_iFrameindexlist **pRecSegHead, int *pRecSegCount)
{
	if(NULL == m_lpDateList)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pDateNode = NULL;

	RecIndexInfo *pIndexNode = NULL;

	int seg_cnt = 0;
	avis_iFrameindexlist *seg_first = NULL;
	avis_iFrameindexlist *seg_head = NULL;

	//m_ListMutex.Wait();

	node = m_lpDateList->head;

	if(NULL == node)
	{
		*pRecSegCount = 0;
		//m_ListMutex.Release();
		return avis_ret_ok;
	}

	avis_fileHanedle read_fd;
	read_fd.fd = 0;
	read_fd.startAddr = 0;
	read_fd.filesize = 0;

	ANTS_AVI_OPEN(&read_fd, io_mode_r, FALSE);

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	S64 iReadSize = 0;

	char startMask[8] , endMask[8];

	snprintf(startMask, sizeof(startMask), "recInS");
	snprintf(endMask, sizeof(endMask), "recInE");


	//遍历日期索引链表
	while(NULL !=  node)
	{
		pDateNode = node->element;
		if(!node->element)
			break;
		if(!compareYMD(pDateNode->tDate, tBeginTime) && !compareYMD(pDateNode->tDate, tEndTime))
		{
			node = node->next;
			continue;
		}

		avis_SDCardHead tSDCardHead;
		memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
		g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);
		if(!(((pDateNode->startAddr >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize)) ||
			((pDateNode->startAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize))))
		{
			node = node->next;
			continue;
		}

		read_fd.startAddr = pDateNode->startAddr;
		read_fd.filesize = pDateNode->file_size;

		iReadSize = 0;

		//AVI_DBG("time[%d %d] read_fd  startAddr=%lld filesize=%lld\n",tBeginTime, tEndTime,read_fd.startAddr, read_fd.filesize);

		while(iReadSize < read_fd.filesize)
		{
			S64 iRemainSize = read_fd.filesize-iReadSize;

			if(read_fd.startAddr + iReadSize  >=
				g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
			{
				S64 offset = read_fd.startAddr + iReadSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
				lseek64(read_fd.fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset, SEEK_SET);
			}
			else
			{
				ANTS_AVI_SEEK(&read_fd, iReadSize, SEEK_SET);
			}

			S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

			if(read_fd.startAddr + iReadSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
				read_fd.startAddr + iReadSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - read_fd.startAddr - iReadSize;
			}

			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			S64 iByteRead = ANTS_AVI_READ(&read_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

			//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
			for(S64 i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
			{
				pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

				if(strncmp(pIndexNode->startMask, startMask, sizeof(startMask))!=0 || strncmp(pIndexNode->endMask, endMask, sizeof(endMask))!=0)
				{
					continue;
				}

				if(pIndexNode->begin_sec > tEndTime || pIndexNode->end_sec < tBeginTime || pIndexNode->begin_sec > pIndexNode->end_sec)
				{
					continue;
				}

				if(0 == (pIndexNode->file_type & iRecType))
				{
					continue;
				}

				if(pIndexNode->file_size <= 0)
				{
					continue;
				}

				//char temp1[20], temp2[20];
				//GetStrFromTime(pIndexNode->begin_sec, temp1);
				//GetStrFromTime(pIndexNode->end_sec, temp2);

				//AVI_DBG("pIndexNode time[%s-%s] startAddr=%lld file_size=%d use_size=%d\n",temp1, temp2, pIndexNode->startAddr,pIndexNode->file_size, pIndexNode->use_size);

				 //只读I帧
				avis_iFrameindexlist *seg_node = new avis_iFrameindexlist;
				memset(seg_node, 0, sizeof(avis_iFrameindexlist));

				memcpy(&seg_node->recIndexInfo, pIndexNode, sizeof(RecIndexInfo));
				seg_node->ptNext = NULL;

				if(NULL == seg_first)
				{
					seg_first = seg_node;
					seg_head = seg_first;
				}
				else
				{
					seg_first->ptNext = seg_node;
					seg_first = seg_node;
				}

				seg_cnt++;

			}

			iReadSize += iByteRead;
		}

		node = node->next;
	}

	ANTS_AVI_CLOSE(&read_fd);
	DEL_ARR_POINTER(pBuffer);


	AVI_DBG("========seg_cnt:%d\n",seg_cnt);
	//int i=0;
	avis_iFrameindexlist *seg_node = seg_head;
	avis_iFrameindexlist *seg_next = NULL;
	RecIndexInfo temp;

/*	char temp1[20], temp2[20];

	while(NULL != seg_node)
	{
		GetStrFromTime(seg_node->recIndexInfo.begin_sec, temp1);
		GetStrFromTime(seg_node->recIndexInfo.end_sec, temp2);
		AVI_DBG("seg_node[%d] startAddr[%lld]  filesize[%d] time=%s-%s\n", i, seg_node->recIndexInfo.startAddr, seg_node->recIndexInfo.file_size , temp1, temp2);
		seg_node = seg_node->ptNext;
		i++;
	}
*/
	for(seg_node=seg_head; seg_node!=NULL; seg_node=seg_node->ptNext)
	{
		for(seg_next=seg_node->ptNext;seg_next!=NULL;seg_next=seg_next->ptNext)
		{
			if(seg_node->recIndexInfo.begin_sec > seg_next->recIndexInfo.begin_sec)
			{
				memcpy(&temp, &seg_node->recIndexInfo, sizeof(RecIndexInfo));
				memcpy(&seg_node->recIndexInfo, &seg_next->recIndexInfo, sizeof(RecIndexInfo));
				memcpy(&seg_next->recIndexInfo, &temp, sizeof(RecIndexInfo));
			}
		}
	}

	seg_node = seg_head;
/*	i=0;

	while(NULL != seg_node)
	{
		GetStrFromTime(seg_node->recIndexInfo.begin_sec, temp1);
		GetStrFromTime(seg_node->recIndexInfo.end_sec, temp2);
		AVI_DBG("seg_node[%d] startAddr[%lld]  filesize[%d] time=%s-%s\n", i, seg_node->recIndexInfo.startAddr, seg_node->recIndexInfo.file_size, temp1, temp2);
		seg_node = seg_node->ptNext;
		i++;
	}
*/
	*pRecSegHead = seg_head;
	*pRecSegCount = seg_cnt;

	//m_ListMutex.Release();

	return avis_ret_ok;
}

int CRecIndexManage::ReleaseIFrameList(avis_iFrameindexlist *pRecSegHead)
{
	avis_iFrameindexlist *seg_first = pRecSegHead;
	while(NULL != seg_first)
	{
		avis_iFrameindexlist *seg_node = seg_first->ptNext;
		delete seg_first;
		seg_first = seg_node;
	}

	pRecSegHead = NULL;
	return avis_ret_ok;
}

int CRecIndexManage::GetDataSize(time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pSize)
{
	if(NULL == m_lpDateList)
	{
		*pSize = 0;
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pDateNode = NULL;

	RecIndexInfo *pIndexNode = NULL;

	m_ListMutex.Wait();

	node = m_lpDateList->head;

	if(NULL == node)
	{
		*pSize = 0;
		m_ListMutex.Release();
		return avis_ret_ok;
	}
	char startMask[8] , endMask[8];

	snprintf(startMask, sizeof(startMask), "recInS");
	snprintf(endMask, sizeof(endMask), "recInE");
	avis_fileHanedle read_fd;
	read_fd.fd = 0;
	read_fd.startAddr = 0;
	read_fd.filesize = 0;

	ANTS_AVI_OPEN(&read_fd, io_mode_r, FALSE);

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		*pSize = 0;
		OUTPUT_FUNC_LINE;
		ANTS_AVI_CLOSE(&read_fd);
		m_ListMutex.Release();
		return avis_ret_failed;
	}

	S64 iReadSize = 0;

	unsigned long long size = 0;

	while(NULL !=  node)
	{
		pDateNode = node->element;
		if(!node->element)
			break;
		if(compareYMD_Ex(pDateNode->tDate, tBeginTime) < 0 || compareYMD_Ex(pDateNode->tDate, tEndTime) > 0)
		//if(pDateNode->tDate > tEndTime || pDateNode->tDate < tBeginTime)
		{
			node = node->next;
			continue;
		}

		avis_SDCardHead tSDCardHead;
		memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
		g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);
		if(!(((pDateNode->startAddr >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize)) ||
			((pDateNode->startAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize))))
		{
			node = node->next;
			continue;
		}

		read_fd.startAddr = pDateNode->startAddr;
		read_fd.filesize = pDateNode->file_size;


		iReadSize = 0;

		while(iReadSize < read_fd.filesize)
		{
			S64 iRemainSize = read_fd.filesize-iReadSize;

			if(read_fd.startAddr + iReadSize  >=
				g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
			{
				S64 offset = read_fd.startAddr + iReadSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
				lseek64(read_fd.fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset, SEEK_SET);
			}
			else
			{
				ANTS_AVI_SEEK(&read_fd, iReadSize, SEEK_SET);
			}

			S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

			if(read_fd.startAddr + iReadSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
				read_fd.startAddr + iReadSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - read_fd.startAddr - iReadSize;
			}

			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			S64 iByteRead = ANTS_AVI_READ(&read_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

			//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
			for(S64 i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
			{
				pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

				if(strncmp(pIndexNode->startMask, startMask, sizeof(startMask))!=0 || strncmp(pIndexNode->endMask, endMask, sizeof(endMask))!=0)
				{
					continue;
				}

				if(pIndexNode->begin_sec > tEndTime || pIndexNode->end_sec < tBeginTime || pIndexNode->begin_sec > pIndexNode->end_sec)
				{
					continue;
				}

				if(0 == (pIndexNode->file_type & iRecType))
				{
					continue;
				}

				if(pIndexNode->file_size <= 0)
				{
					continue;
				}

				if(tBeginTime >= pIndexNode->begin_sec && tEndTime <= pIndexNode->end_sec)
				{
					int iTimeOffset1 = tEndTime - pIndexNode->begin_sec;
					int iTimeOffset2 = tBeginTime - pIndexNode->begin_sec;

					if(iTimeOffset1 < IFRAMEOFFSETNUM)
					{
						iTimeOffset1 += 1;
					}
					size += (pIndexNode->iFrameOffset[iTimeOffset2] - pIndexNode->iFrameOffset[iTimeOffset1]);
				}
				else if(tBeginTime >= pIndexNode->begin_sec && tEndTime > pIndexNode->end_sec)
				{
					int iTimeOffset = tBeginTime - pIndexNode->begin_sec;
					size += (pIndexNode->file_size - pIndexNode->iFrameOffset[iTimeOffset]);
				}
				else if(tBeginTime < pIndexNode->begin_sec)
				{
					int iTimeOffset = tEndTime - pIndexNode->begin_sec;
					if(iTimeOffset >= IFRAMEOFFSETNUM)
					{
						iTimeOffset = IFRAMEOFFSETNUM-1;
					}
					size += pIndexNode->iFrameOffset[iTimeOffset];
				}
			}

			iReadSize += iByteRead;
		}

		node = node->next;
	}

	ANTS_AVI_CLOSE(&read_fd);
	DEL_ARR_POINTER(pBuffer);

	*pSize = size;
	AVI_WAR("[%d-%d] iRecType:%d,iDataSize:%llu\n",tBeginTime,tEndTime,iRecType,size);

	m_ListMutex.Release();

	return avis_ret_ok;
}


int CRecIndexManage::UpdateLastRecIndexToFile(RecIndexInfo *pIndexInfo) //更新最后一个索引
{
	//AVI_DBG("========= m_recindex_fd->startAddr=%lld filesize=%lld\n",m_recindex_fd->startAddr, m_recindex_fd->filesize);
	// 从后往前找比较快
	int find_ok = 0;
	RecIndexInfo node_info;
	S64 offset=0;

	if(0 == pIndexInfo->startAddr)
	{
		return avis_ret_failed;
	}

	avis_SDCardHead tSDCardHead;
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	m_indexInfoMutex.Wait();
	m_recindex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_recindex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	if(m_recindex_fd->startAddr + m_recindex_fd->filesize  > g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
	{
		offset = m_recindex_fd->startAddr + m_recindex_fd->filesize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
		lseek64(m_recindex_fd->fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset -REC_INDEX_NODE_LEN, SEEK_SET);
	}
	else
	{
		ANTS_AVI_SEEK(m_recindex_fd, -REC_INDEX_NODE_LEN, SEEK_END);
	}

	S64 rd_size = ANTS_AVI_READ(m_recindex_fd, &node_info, REC_INDEX_NODE_LEN);
	if (rd_size != REC_INDEX_NODE_LEN)
	{
		OUTPUT_FUNC_LINE;
		m_indexInfoMutex.Release();
		g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();
		return avis_ret_failed;
	}
	if (pIndexInfo->startAddr == node_info.startAddr)
	{
		find_ok = 1;
	}

	BOOL update_ok = FALSE;
	if (find_ok)
	{
		if(m_recindex_fd->startAddr + m_recindex_fd->filesize  > g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
		{
			offset = m_recindex_fd->startAddr + m_recindex_fd->filesize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
			lseek64(m_recindex_fd->fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset -REC_INDEX_NODE_LEN, SEEK_SET);
		}
		else
		{
			ANTS_AVI_SEEK(m_recindex_fd, -REC_INDEX_NODE_LEN, SEEK_END);
		}

		if (ANTS_AVI_WRITE(m_recindex_fd, pIndexInfo, REC_INDEX_NODE_LEN) == REC_INDEX_NODE_LEN)
		{
			update_ok = true;
			//AVI_DBG("update pos=%lld m_recindex_info.startAddr %lld, m_recindex_info.file_size = %d, success\n",pos, pIndexInfo->startAddr, pIndexInfo->file_size);
		}
		else
		{
			AVI_ERR("%lld, update fail!\n", pIndexInfo->startAddr);
		}
	}
	else
	{
		AVI_ERR("not find the file pIndexInfo.startAddr: %lld, node_info.startAddr = %lld\n", pIndexInfo->startAddr, node_info.startAddr);
	}

	m_indexInfoMutex.Release();
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	return update_ok ? avis_ret_ok : avis_ret_failed;
}

int CRecIndexManage::InsertRecIndexToFile(RecIndexInfo *pIndexInfo)  //在最后插入一个索引
{
	avis_SDCardHead tSDCardHead;
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	m_recindex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_recindex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	//AVI_DBG("=========InsertRecIndexToFile  m_recindex_fd->startAddr=%lld filesize=%lld\n",m_recindex_fd->startAddr, m_recindex_fd->filesize);

	S64 offset = 0;

	if(m_recindex_fd->startAddr + m_recindex_fd->filesize  >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
	{
		offset = m_recindex_fd->startAddr + m_recindex_fd->filesize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
		lseek64(m_recindex_fd->fd, g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + offset, SEEK_SET);

		m_recindex_fd->filesize += REC_INDEX_NODE_LEN;
		AVI_DBG("===== m_recindex_fd startAddr=%lld filesize=%lld\n",m_recindex_fd->startAddr,m_recindex_fd->filesize);
	}
	else
	{
		ANTS_AVI_SEEK(m_recindex_fd, 0, SEEK_END);
	}

	if (ANTS_AVI_WRITE(m_recindex_fd, pIndexInfo, REC_INDEX_NODE_LEN) != REC_INDEX_NODE_LEN)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	//AVI_DBG("m_recindex_fd->startAddr:%lld filesize:%lld\n", m_recindex_fd->startAddr, m_recindex_fd->filesize);

	tSDCardHead.tRecFileIndexAddr.filesize = m_recindex_fd->filesize;

	int iRet = g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);
	if(iRet == avis_ret_failed)
	{
		AVI_ERR("error !!\n");
	}

	return avis_ret_ok;
}


int CRecIndexManage::AddOneRecDateIndexNode(antsavi_recDateInfo *pIndexInfo)
{
	if(NULL == m_lpDateList)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	if(NULL == pIndexInfo)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	m_ListMutex.Wait();

	antsavi_recdate_node_t *node = new antsavi_recdate_node_t;
	if(NULL == node)
	{
		OUTPUT_FUNC_LINE;
		m_ListMutex.Release();
		return avis_ret_failed;
	}

	antsavi_recDateInfo *pNodeInfo = new antsavi_recDateInfo;
	if(NULL == pNodeInfo)
	{
		DEL_POINTER(node);
		m_ListMutex.Release();
		return avis_ret_failed;
	}

	pNodeInfo->startAddr = pIndexInfo->startAddr;
	pNodeInfo->file_size = pIndexInfo->file_size;
	pNodeInfo->tDate = pIndexInfo->tDate;

	node->pre=NULL;
	node->next=NULL;
	node->element = pNodeInfo;

	if(m_lpDateList->head==NULL && m_lpDateList->tail==NULL)
	{
		m_lpDateList->head=node;
		m_lpDateList->tail=node;

		m_lpDateList->iNodeCount++;

		//PrintRecDateList();

		m_ListMutex.Release();
		return avis_ret_ok;
	}

	m_lpDateList->tail->next = node;
	node->pre = m_lpDateList->tail;
	m_lpDateList->tail=node;
	m_lpDateList->iNodeCount++;

	//PrintRecDateList();

	m_ListMutex.Release();
	return avis_ret_ok;
}

int CRecIndexManage::UpdateOneRecDateIndexNode(antsavi_recDateInfo *pIndexInfo)
{
	if(NULL == m_lpDateList)
	{
		return avis_ret_failed;
	}

	if(NULL == pIndexInfo)
	{
		return avis_ret_failed;
	}

	m_ListMutex.Wait();

	//从最后一个结点查找最快
	antsavi_recdate_node_t *node = NULL;
	node = m_lpDateList->tail;

	if(NULL == node)
	{
		m_ListMutex.Release();
		return avis_ret_failed;
	}

	int find_ok = 0;

	while(NULL !=  node)
	{
		if(!node->element)
			break;
		if(node->element->startAddr == pIndexInfo->startAddr && node->element->tDate == pIndexInfo->tDate)
		{
			find_ok = 1;
			break;
		}

		node = node->pre;
	}

	if(find_ok == 1)
	{
		if(node->element->file_size != pIndexInfo->file_size)
		{
			node->element->file_size = pIndexInfo->file_size;
		}
		m_ListMutex.Release();
		return avis_ret_ok;
	}
	else
	{
		AVI_ERR("not find node start=%lld  pIndexInfo->tDate=%d\n",pIndexInfo->startAddr,(int)pIndexInfo->tDate);
	}

	m_ListMutex.Release();
	return avis_ret_failed;
}

int CRecIndexManage::AddOneRecIndexNodeToDateList(RecIndexInfo *pIndexInfo, BOOL bWriteFile, S64 recIndexAddr)  //先写录像索引，再写日期索引，最后写SD卡头
{
	if(NULL == pIndexInfo)
	{
		return avis_ret_failed;
	}

	static int sRecIndexNum = 0;
	pIndexInfo->iIndexNum = sRecIndexNum;
	sRecIndexNum++;

	if(bWriteFile)
	{
		g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();
	}
	
	m_indexInfoMutex.Wait();

	int iRet = avis_ret_ok;

	if(bWriteFile)
	{
		iRet = InsertRecIndexToFile(pIndexInfo);
	}

	if(avis_ret_ok == iRet)
	{
		avis_time_info tTime1,tTime2;
//		gmtime_utc(pIndexInfo->begin_sec, &tTime1);
//		gmtime_utc(m_recDateIndexInfo.tDate, &tTime2);
		localtime_utc(pIndexInfo->begin_sec, &tTime1);
		localtime_utc(m_recDateIndexInfo.tDate, &tTime2);

		if(tTime1.wYear==tTime2.wYear && tTime1.wMonth==tTime2.wMonth && tTime1.wDay==tTime2.wDay)
		{
			m_recDateIndexInfo.file_size += REC_INDEX_NODE_LEN;

			UpdateOneRecDateIndexNode(&m_recDateIndexInfo);
		}
		else
		{
			AVI_DBG("pIndexInfo[%04d-%02d-%02d] ,m_recDateIndexInfo[%04d-%02d-%02d] m_recDateIndexInfo.startAddr=%lld recIndexAddr=%lld\n",
				tTime1.wYear, tTime1.wMonth, tTime1.wDay, tTime2.wYear, tTime2.wMonth, tTime2.wDay,m_recDateIndexInfo.startAddr,recIndexAddr);

			memset(&m_recDateIndexInfo, 0, sizeof(m_recDateIndexInfo));

			avis_time_info tTime;
			memset(&tTime, 0, sizeof(tTime));
			tTime.wYear = tTime1.wYear;
			tTime.wMonth = tTime1.wMonth;
			tTime.wDay = tTime1.wDay;
			mktime_utc(&tTime , &m_recDateIndexInfo.tDate);

			char temp[20];
			GetStrFromTime(m_recDateIndexInfo.tDate, temp);


			if(recIndexAddr != -1)
			{
				m_recDateIndexInfo.startAddr = recIndexAddr;
			}
			else
			{
				m_recDateIndexInfo.startAddr = m_recindex_fd->startAddr + m_recindex_fd->filesize - REC_INDEX_NODE_LEN;
			}

			AVI_DBG("m_recDateIndexInfo.tDate = %s startAddr=%lld recIndexAddr=%lld\n",temp,m_recDateIndexInfo.startAddr,recIndexAddr);

			if(m_recDateIndexInfo.startAddr >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				m_recDateIndexInfo.startAddr -= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
			}

			m_recDateIndexInfo.file_size = REC_INDEX_NODE_LEN;

			AddOneRecDateIndexNode(&m_recDateIndexInfo);

			//PrintRecDateList();
		}
	}

	m_indexInfoMutex.Release();

	if(bWriteFile)
	{
		g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();
	}

	return avis_ret_ok;
}

int CRecIndexManage::UpdateOneRecIndexNode(RecIndexInfo *pIndexInfo)
{
	if(NULL == pIndexInfo)
	{
		return avis_ret_failed;
	}
	
	UpdateLastRecIndexToFile(pIndexInfo);
	
	return avis_ret_failed;
}


int CRecIndexManage::DeleteOneRecIndexNode()
{
	//AVI_DBG("DeleteOneRecIndexNode\n");
	if(NULL == m_lpDateList || NULL == m_delRecIndex_fd)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();

	m_indexInfoMutex.Wait();

	m_ListMutex.Wait();

	avis_SDCardHead tSDCardHead;
	memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	m_delRecIndex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_delRecIndex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	//AVI_DBG("m_recindex_fd startAddr=%lld size=%lld\n",m_recindex_fd->startAddr, m_recindex_fd->filesize);

	char ch[512];
	memset(ch, 0, sizeof(ch));

	BOOL bErrorIndex = FALSE;

	char chStart[8]={0};
	char chEnd[8]={0};
	memset(chStart, 0, sizeof(chStart));
	memset(chEnd, 0, sizeof(chEnd));
	snprintf(chStart, sizeof(chStart), "recInS");
	snprintf(chEnd, sizeof(chEnd), "recInE");

	RecIndexInfo recIndexInfo;
	memset(&recIndexInfo, 0, REC_INDEX_NODE_LEN);

	ANTS_AVI_SEEK(m_delRecIndex_fd, 0, SEEK_SET);
	S64 iByte = ANTS_AVI_READ(m_delRecIndex_fd, &recIndexInfo, REC_INDEX_NODE_LEN);
	if(iByte != REC_INDEX_NODE_LEN)
	{
		bErrorIndex = TRUE;
		AVI_ERR("======read error!!!!\n\n");
	}
	else if(strncmp(recIndexInfo.startMask, chStart, sizeof(recIndexInfo.startMask))!=0 || strncmp(recIndexInfo.endMask, chEnd, sizeof(recIndexInfo.endMask))!=0 ) //索引错误
	{
		bErrorIndex = TRUE;
		AVI_ERR("======error rec index\n");

		AVI_PrintBuffer(&recIndexInfo, REC_INDEX_NODE_LEN);
	}
	else if(recIndexInfo.startAddr < g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr || recIndexInfo.startAddr > g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
	{
		bErrorIndex = TRUE;
		AVI_ERR("======error rec index recIndexInfo.startAddr=%lld filesize=%d\n", recIndexInfo.startAddr,recIndexInfo.file_size);
	}

	//更新日期索引
	antsavi_recdate_node_t *pHead = m_lpDateList->head;
	antsavi_recDateInfo *pDateInfo = NULL;
	if(NULL != pHead)
	{
		pDateInfo = pHead->element;
	}
	if(NULL != pHead && NULL != pDateInfo && pDateInfo->startAddr == m_delRecIndex_fd->startAddr)
	{
		if(m_recDateIndexInfo.startAddr == pDateInfo->startAddr && m_recDateIndexInfo.tDate == pDateInfo->tDate)
		{
			m_recDateIndexInfo.startAddr += REC_INDEX_NODE_LEN;
			m_recDateIndexInfo.file_size -= REC_INDEX_NODE_LEN;

			if(m_recDateIndexInfo.startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				m_recDateIndexInfo.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
			}
		}

		pDateInfo->startAddr += REC_INDEX_NODE_LEN;
		pDateInfo->file_size -= REC_INDEX_NODE_LEN;

		if(pDateInfo->startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
		{
			pDateInfo->startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
		}

		if(pDateInfo->file_size == 0)
		{
			m_lpDateList->head = m_lpDateList->head->next;
			if(!m_lpDateList->head)
				m_lpDateList->tail = NULL;
			DEL_POINTER(pDateInfo);
			DEL_POINTER(pHead);
			m_lpDateList->iNodeCount --;
		}

		//PrintRecDateList();
	}
	else
	{
		PrintRecDateList();
		AVI_ERR("pHead=%p  pDateInfo=%p  m_delRecIndex_fd->startAddr=%lld filesize=%lld!!!!!!!!!\n",pHead,pDateInfo,m_delRecIndex_fd->startAddr,m_delRecIndex_fd->filesize);

		AVI_PrintBuffer(&recIndexInfo, REC_INDEX_NODE_LEN);
	}

	//更新录像索引
	ANTS_AVI_SEEK(m_delRecIndex_fd, 0, SEEK_SET);
	ANTS_AVI_WRITE(m_delRecIndex_fd, ch, REC_INDEX_NODE_LEN);

	m_delRecIndex_fd->startAddr += REC_INDEX_NODE_LEN;
	m_delRecIndex_fd->filesize -= REC_INDEX_NODE_LEN;


	tSDCardHead.tRecFileIndexAddr.startAddr = m_delRecIndex_fd->startAddr;
	if(tSDCardHead.tRecFileIndexAddr.startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
	{
		tSDCardHead.tRecFileIndexAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;

		m_delRecIndex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	}
	tSDCardHead.tRecFileIndexAddr.filesize = m_delRecIndex_fd->filesize;

	if(bErrorIndex == FALSE)  //索引是正确的，才能更新录像文件地址
	{
		//更新录像文件地址
		tSDCardHead.tRecFileAddr.startAddr = recIndexInfo.startAddr + recIndexInfo.use_size;
		if(tSDCardHead.tRecFileAddr.startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
		{
			tSDCardHead.tRecFileAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr;
		}
		tSDCardHead.tRecFileAddr.filesize -= recIndexInfo.use_size;
	}

	int iRet = g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);
	if(iRet == avis_ret_failed)
	{
		AVI_ERR("error !!  recIndexInfo.startAddr=%lld filesize=%d usesize=%d\n",recIndexInfo.startAddr, recIndexInfo.file_size, recIndexInfo.use_size);
	}

	m_ListMutex.Release();
	m_indexInfoMutex.Release();
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	return avis_ret_ok;
}


int CRecIndexManage::SyncRecIndexNode()
{
	//AVI_DBG("DeleteOneRecIndexNode\n");
	if(NULL == m_lpDateList || NULL == m_delRecIndex_fd)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	static BOOL bRecIndexIsSync = FALSE;

	if(bRecIndexIsSync == TRUE)
	{
		return avis_ret_ok;
	}

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();

	m_indexInfoMutex.Wait();

	m_ListMutex.Wait();

	avis_SDCardHead tSDCardHead;
	memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	m_delRecIndex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_delRecIndex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	char chStart[8]={0};
	char chEnd[8]={0};
	snprintf(chStart, sizeof(chStart), "recInS");
	snprintf(chEnd, sizeof(chEnd), "recInE");

	char *pBuffer = NULL;
	S64 sCanReadSize = 0;

	do
	{
		if(m_delRecIndex_fd->filesize <= 0)
		{
			bRecIndexIsSync = TRUE;
			break;
		}

		pBuffer = new char[READ_RECINDEX_PERSIZE];
		if(NULL == pBuffer)
		{
			OUTPUT_FUNC_LINE;
			break;
		}
		memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);

		sCanReadSize = m_delRecIndex_fd->filesize > READ_RECINDEX_PERSIZE ? READ_RECINDEX_PERSIZE : m_delRecIndex_fd->filesize;

		ANTS_AVI_SEEK(m_delRecIndex_fd, 0, SEEK_SET);
		S64 iByte = ANTS_AVI_READ(m_delRecIndex_fd, pBuffer, sCanReadSize);
		if(iByte != sCanReadSize)
		{
			AVI_ERR("======read error!!!!\n\n");
			break;
		}

		int i=0;
		RecIndexInfo *pIndexNode = NULL;

		for(i=0; i<sCanReadSize/REC_INDEX_NODE_LEN; i++)
		{
			pIndexNode = (RecIndexInfo*)(pBuffer+i*REC_INDEX_NODE_LEN);

			if(pIndexNode == NULL)
			{
				break;
			}

			if(strncmp(pIndexNode->startMask, chStart, sizeof(pIndexNode->startMask))!=0 || strncmp(pIndexNode->endMask, chEnd, sizeof(pIndexNode->endMask))!=0 ) //索引错误
			{
				AVI_ERR("======error rec index\n");
				AVI_PrintBuffer(pIndexNode->startMask, sizeof(pIndexNode->startMask));
				AVI_PrintBuffer(pIndexNode->endMask, sizeof(pIndexNode->endMask));
			}
			else if(pIndexNode->startAddr < g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr || pIndexNode->startAddr > g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
			{
				AVI_ERR("======error rec index recIndexInfo.startAddr=%lld filesize=%d\n", pIndexNode->startAddr,pIndexNode->file_size);
			}
			else
			{
				avis_fileHanedle avisRecFd;
				AntsIPCFrameHeader_T tFrameHead;
				memset(&tFrameHead, 0, sizeof(AntsIPCFrameHeader_T));
				avisRecFd.fd = 0;
				avisRecFd.startAddr = pIndexNode->startAddr;
				avisRecFd.filesize = pIndexNode->file_size;

				if(avis_ret_ok != ANTS_AVI_OPEN(&avisRecFd, io_mode_r, FALSE))
				{
					OUTPUT_FUNC_LINE;
					break;
				}

				ANTS_AVI_SEEK(&avisRecFd, 0, SEEK_SET);
				if(sizeof(AntsIPCFrameHeader_T) == ANTS_AVI_READ(&avisRecFd, &tFrameHead, sizeof(AntsIPCFrameHeader_T)))
				{
					time_t cur_sec = tFrameHead.uiFrameTime;
					if(cur_sec >= pIndexNode->begin_sec && cur_sec <= pIndexNode->end_sec)
					{
						//索引和录像文件的时间戳是对的
						AVI_DBG("=======Sync Success!\n\n");
						bRecIndexIsSync = TRUE;
						ANTS_AVI_CLOSE(&avisRecFd);
						break;
					}
					else
					{
						AVI_DBG("pIndexNode[%u %u]  cur_sec=%u\n",(unsigned int)pIndexNode->begin_sec, (unsigned int)pIndexNode->end_sec, (unsigned int)cur_sec);
					}
				}

				ANTS_AVI_CLOSE(&avisRecFd);
			}

			//更新日期索引
			antsavi_recdate_node_t *pHead = m_lpDateList->head;
			antsavi_recDateInfo *pDateInfo = NULL;
			if(NULL != pHead)
			{
				pDateInfo = pHead->element;
			}
			if(NULL != pHead && NULL != pDateInfo && pDateInfo->startAddr == m_delRecIndex_fd->startAddr+i*REC_INDEX_NODE_LEN)
			{
				if(m_recDateIndexInfo.startAddr == pDateInfo->startAddr && m_recDateIndexInfo.tDate == pDateInfo->tDate)
				{
					m_recDateIndexInfo.startAddr += REC_INDEX_NODE_LEN;
					m_recDateIndexInfo.file_size -= REC_INDEX_NODE_LEN;

					if(m_recDateIndexInfo.startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
					{
						m_recDateIndexInfo.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
					}
				}

				pDateInfo->startAddr += REC_INDEX_NODE_LEN;
				pDateInfo->file_size -= REC_INDEX_NODE_LEN;

				if(pDateInfo->startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
				{
					pDateInfo->startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
				}

				if(pDateInfo->file_size == 0)
				{
					m_lpDateList->head = m_lpDateList->head->next;
					if(!m_lpDateList->head)
						m_lpDateList->tail = NULL;
					DEL_POINTER(pDateInfo);
					DEL_POINTER(pHead);
					m_lpDateList->iNodeCount --;
				}

				//PrintRecDateList();
			}
		}

		//有i个索引没有对应的录像，更新录像索引
		if(i > 0)
		{
			AVI_DBG("\n\n======================there has %d invalid recIndex\n\n", i);

			//查找最新的日期索引赋给  m_recDateIndexInfo
			if(NULL != m_lpDateList && NULL != m_lpDateList->tail && 0 != m_lpDateList->iNodeCount)
			{
				antsavi_recDateInfo *pNode = m_lpDateList->tail->element;
				if(NULL != pNode)
				{
					m_recDateIndexInfo.tDate = pNode->tDate;
				}
			}
			else
			{
				avis_time_info tTime;
				memset(&tTime, 0, sizeof(tTime));
				tTime.wYear = 2000;
				tTime.wMonth = 13;
				tTime.wDay = 31;
				mktime_utc(&tTime , &m_recDateIndexInfo.tDate);
			}
			
			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			ANTS_AVI_SEEK(m_delRecIndex_fd, 0, SEEK_SET);
			ANTS_AVI_WRITE(m_delRecIndex_fd, pBuffer, i*REC_INDEX_NODE_LEN);

			m_delRecIndex_fd->startAddr += i*REC_INDEX_NODE_LEN;
			m_delRecIndex_fd->filesize -= i*REC_INDEX_NODE_LEN;

			tSDCardHead.tRecFileIndexAddr.startAddr = m_delRecIndex_fd->startAddr;
			if(tSDCardHead.tRecFileIndexAddr.startAddr == g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				tSDCardHead.tRecFileIndexAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;

				m_delRecIndex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
			}
			tSDCardHead.tRecFileIndexAddr.filesize = m_delRecIndex_fd->filesize;

			int iRet = g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);
			if(iRet == avis_ret_failed)
			{
				OUTPUT_FUNC_LINE;
			}
		}
	}while(0);

	DEL_ARR_POINTER(pBuffer);

	m_ListMutex.Release();
	m_indexInfoMutex.Release();
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	return avis_ret_ok;
}

int CRecIndexManage::FormatDisk()
{
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();

	m_indexInfoMutex.Wait();

	m_ListMutex.Wait();

	if(NULL != m_lpDateList)
	{
		antsavi_recdate_node_t *temp1=NULL;
		antsavi_recdate_node_t *temp2=NULL;
		antsavi_recDateInfo *node=NULL;

		temp1 = m_lpDateList->head;
		temp2 = m_lpDateList->head;

		while(temp1!=NULL){
			temp2 = temp1->next ;
			node = temp1->element;
			DEL_POINTER(node);
			DEL_POINTER(temp1);
			m_lpDateList->iNodeCount--;
			temp1=temp2;
		}

		m_lpDateList->head = NULL;
		m_lpDateList->tail = NULL;
		m_lpDateList->iNodeCount = 0;
	}

	avis_SDCardHead tSDCardHead;
	memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	m_recindex_fd->startAddr = tSDCardHead.tRecFileIndexAddr.startAddr;
	m_recindex_fd->filesize = tSDCardHead.tRecFileIndexAddr.filesize;

	//先清除SD卡头
	tSDCardHead.tRecFileIndexAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr;
	tSDCardHead.tRecFileIndexAddr.filesize = 0;

	tSDCardHead.tRecFileAddr.startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr;
	tSDCardHead.tRecFileAddr.filesize = 0;

	tSDCardHead.llSDCardSize = g_antsAviLibInfo.antsStorageManage->m_SDCardTotalSize;
	tSDCardHead.llheadSize = g_antsAviLibInfo.antsStorageManage->m_SDCardHeadMaxSize;
	tSDCardHead.llRecIndexSize = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
	tSDCardHead.llRecFileSize = g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize;
	
	memset(tSDCardHead.startMark, 0, sizeof(tSDCardHead.startMark));
	memset(tSDCardHead.endMark, 0, sizeof(tSDCardHead.endMark));
	
	int iRet = g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);
	if(iRet == avis_ret_failed)
	{
		AVI_ERR("\n\n=========err here!!!!!!!!!!!\n");
	}

	ANTS_AVI_SEEK(m_recindex_fd, 0, SEEK_SET);

	char *ch = new char[1024*1024];

	if(NULL != ch)
	{
		memset(ch, 0, sizeof(1024*1024));
		S64 iWriteByte = 0;

		while(m_recindex_fd->filesize > 0)
		{
			iWriteByte = m_recindex_fd->filesize >=1024*1024?1024*1024:m_recindex_fd->filesize;

			AVI_DBG("================================m_recindex_fd->filesize=%lld  iWriteByte=%lld\n",m_recindex_fd->filesize,iWriteByte);

			ANTS_AVI_WRITE(m_recindex_fd, ch, iWriteByte);
			m_recindex_fd->filesize -= iWriteByte;
		}

		DEL_ARR_POINTER(ch);
	}

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	g_antsAviLibInfo.antsStorageManage->SaveSDCardHead();

	m_indexInfoMutex.Release();
	m_ListMutex.Release();

	return avis_ret_ok;
}

int CRecIndexManage::GetRecSegList(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG **pRecSegHead, int *pRecSegCount)
{
	if(NULL == m_lpDateList)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pDateNode = NULL;

	RecIndexInfo *pIndexNode = NULL;

	int seg_cnt = 0;
	PSS_REGSEG *seg_first = NULL;
	PSS_REGSEG *seg_head = NULL;

	m_ListMutex.Wait();

	node = m_lpDateList->head;

	if(NULL == node)
	{
		AVI_ERR("no data in m_lpDateList\n");
		*pRecSegCount = 0;
		m_ListMutex.Release();
		return avis_ret_ok;
	}

	avis_fileHanedle read_fd;
	read_fd.fd = 0;
	read_fd.startAddr = 0;
	read_fd.filesize = 0;

	ANTS_AVI_OPEN(&read_fd, io_mode_r, FALSE);

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	S64 iReadSize = 0;

	char startMask[8] , endMask[8];

	memset(startMask, 0, sizeof(startMask));
	memset(endMask, 0, sizeof(endMask));

	snprintf(startMask, sizeof(startMask), "recInS");
	snprintf(endMask, sizeof(endMask), "recInE");

//	PrintRecDateList();

	char temp1[20], temp2[20];
	GetStrFromTime(tBeginTime, temp1);
	GetStrFromTime(tEndTime, temp2);

	AVI_DBG("====iRecType=0x%x  time [%s -- %s]\n",iRecType,temp1, temp2);

	while(NULL !=  node)
	{
		pDateNode = node->element;
		if(!node->element)
			break;
		//if(pDateNode->tDate > tEndTime || pDateNode->tDate < tBeginTime)
		if(compareYMD_Ex(pDateNode->tDate, tBeginTime) < 0 || compareYMD_Ex(pDateNode->tDate, tEndTime) > 0)
		//if(!compareYMD(pDateNode->tDate, tBeginTime) && !compareYMD(pDateNode->tDate, tEndTime))
		{
			node = node->next;
			continue;
		}

		avis_SDCardHead tSDCardHead;
		memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
		g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);
		if(!(((pDateNode->startAddr >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize)) ||
			((pDateNode->startAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize))))
		{
			node = node->next;
			continue;
		}
		
		//AVI_DBG("pDateNode->startAddr=%lld  file_size=%lld\n",pDateNode->startAddr,pDateNode->file_size);

		read_fd.startAddr = pDateNode->startAddr;
		read_fd.filesize = pDateNode->file_size;

		iReadSize = 0;

		while(iReadSize < read_fd.filesize)
		{
			S64 iRemainSize = read_fd.filesize-iReadSize;

			//AVI_DBG("read_fd.startAddr=%lld  iReadSize=%d",read_fd.startAddr,iReadSize);

			if(read_fd.startAddr + iReadSize  >=
				g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
			{
				S64 offset = read_fd.startAddr + iReadSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
				lseek64(read_fd.fd, offset, SEEK_SET);
			}
			else
			{
				ANTS_AVI_SEEK(&read_fd, iReadSize, SEEK_SET);
			}

			S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

			if(read_fd.startAddr + iReadSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
				read_fd.startAddr + iReadSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - read_fd.startAddr - iReadSize;
			}

			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			S64 iByteRead = ANTS_AVI_READ(&read_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

			//AVI_DBG("iByteRead=%d  seg_cnt=%d\n",iByteRead,seg_cnt);

			//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
			for(S64 i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
			{
				pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

				#if 0
				int iCount = 0;
				char szBegin[32] = {0},szEnd[32] = {0};
				GetStrFromTime(pIndexNode->begin_sec, szBegin);
				GetStrFromTime(pIndexNode->end_sec, szEnd);
				AVI_WAR("[%d]pIndexNode:%d[%s]=>%d[%s],startAddr:%lld,file_size:%d,use_size:%d\n",iCount++,pIndexNode->begin_sec,szBegin,pIndexNode->end_sec,szEnd,pIndexNode->startAddr,pIndexNode->file_size,pIndexNode->use_size);
				#endif
				if(strncmp(pIndexNode->startMask, startMask, sizeof(startMask))!=0 || strncmp(pIndexNode->endMask, endMask, sizeof(endMask))!=0)
				{
					continue;
				}

				if(pIndexNode->begin_sec > tEndTime || pIndexNode->end_sec < tBeginTime || pIndexNode->begin_sec > pIndexNode->end_sec)
				{
					continue;
				}

				if(pIndexNode->begin_sec == pIndexNode->end_sec)
				{
					continue;
				}

				if(0 == (pIndexNode->file_type & iRecType))
				{
					continue;
				}

				if(pIndexNode->file_size <= 0)
				{
					continue;
				}
				if(NULL == seg_first)
				{
					PSS_REGSEG *seg_node = new PSS_REGSEG;

					seg_node->tBeginTime =  pIndexNode->begin_sec < tBeginTime ? tBeginTime : pIndexNode->begin_sec;
					seg_node->tEndTime =  pIndexNode->end_sec > tEndTime ? tEndTime : pIndexNode->end_sec;

					seg_node->iRecType = pIndexNode->file_type;
					seg_node->ptNext = NULL;

					seg_first = seg_node;
					seg_head = seg_first;

					seg_cnt++;
				}
				else
				{
					//如果上一个结点的endTime和当前查询结点的开始时间相差1s
					if((seg_first->tEndTime == pIndexNode->begin_sec || seg_first->tEndTime+1 == pIndexNode->begin_sec)  && seg_first->iRecType == pIndexNode->file_type)
					{
						seg_first->tEndTime = pIndexNode->end_sec > tEndTime ? tEndTime : pIndexNode->end_sec;
					}
					else
					{
						PSS_REGSEG *seg_node = new PSS_REGSEG;

						seg_node->tBeginTime =  pIndexNode->begin_sec < tBeginTime ? tBeginTime : pIndexNode->begin_sec;
						seg_node->tEndTime =  pIndexNode->end_sec > tEndTime ? tEndTime : pIndexNode->end_sec;

						seg_node->iRecType = pIndexNode->file_type;
						seg_node->ptNext = NULL;

						seg_first->ptNext = seg_node;
						seg_first = seg_node;

						seg_cnt++;
					}
				}
			}

			iReadSize += iByteRead;
		}

		//AVI_DBG("read_fd.startAddr=%lld  iReadSize=%d",read_fd.startAddr,iReadSize);

		node = node->next;
	}


	ANTS_AVI_CLOSE(&read_fd);
	DEL_ARR_POINTER(pBuffer);
#if 0
	PSS_REGSEG *seg_node = seg_head;
#if 0		//屏蔽掉这里，可能会存在同一时间多个录像片断(时间同步引起)
	while(seg_node)
	{
		PSS_REGSEG *tmp_node = seg_head;
		while(tmp_node)
		{
			if(tmp_node != seg_node)
			{
				if(seg_node->tEndTime < tmp_node->tBeginTime || seg_node->tBeginTime > tmp_node->tEndTime)
				{
				}
				else if(seg_node->tBeginTime < tmp_node->tBeginTime ||(seg_node->tEndTime > tmp_node->tBeginTime &&seg_node->tEndTime < tmp_node->tEndTime))
				{
					seg_node->tEndTime = tmp_node->tEndTime;
					seg_node->ptNext = tmp_node->ptNext;
					delete tmp_node;
					if(seg_cnt > 0)	seg_cnt--;
				}
				else if(seg_node->tBeginTime >= tmp_node->tBeginTime ||seg_node->tEndTime <= tmp_node->tEndTime)
				{
					seg_node->tBeginTime = tmp_node->tBeginTime;
					seg_node->tEndTime = tmp_node->tEndTime;
					seg_node->ptNext = tmp_node->ptNext;
					delete tmp_node;
					if(seg_cnt > 0)	seg_cnt--;
				}
				else if(seg_node->tBeginTime <= tmp_node->tBeginTime ||seg_node->tEndTime >= tmp_node->tEndTime)
				{
					seg_node->ptNext = tmp_node->ptNext;
					delete tmp_node;
					if(seg_cnt > 0)	seg_cnt--;
				}
				else if((seg_node->tBeginTime > tmp_node->tBeginTime && seg_node->tBeginTime < tmp_node->tEndTime)||seg_node->tEndTime > tmp_node->tEndTime)
				{
					seg_node->tBeginTime = tmp_node->tBeginTime;
					seg_node->ptNext = tmp_node->ptNext;
					delete tmp_node;
					if(seg_cnt > 0)	seg_cnt--;
				}
			}
			tmp_node = tmp_node->ptNext;
		}
		seg_node = seg_node->ptNext;
	}
#endif
	char szBegin[32] = {0},szEnd[32] = {0};
	GetStrFromTime(pIndexNode->begin_sec, szBegin);
	GetStrFromTime(pIndexNode->end_sec, szEnd);
	seg_node = seg_head;
	while(seg_node)
	{
		char szBegin[32] = {0},szEnd[32] = {0};
		GetStrFromTime(seg_node->tBeginTime, szBegin);
		GetStrFromTime(seg_node->tEndTime, szEnd);
		AVI_WAR("[%s-%s] iRecType:%d\n",szBegin,szEnd,seg_node->iRecType);
		seg_node = seg_node->ptNext;
	}
#endif
	*pRecSegHead = seg_head;
	*pRecSegCount = seg_cnt;

	m_ListMutex.Release();

	return avis_ret_ok;
}

int CRecIndexManage::HasRecSegList(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType)
{
	if(NULL == m_lpDateList)
	{
		OUTPUT_FUNC_LINE;
		return 0;
	}

	antsavi_recdate_node_t *node = NULL;
	antsavi_recDateInfo *pDateNode = NULL;

	RecIndexInfo *pIndexNode = NULL;

	m_ListMutex.Wait();

	node = m_lpDateList->head;

	if(NULL == node)
	{
		m_ListMutex.Release();
		return 0;
	}

	avis_fileHanedle read_fd;
	read_fd.fd = 0;
	read_fd.startAddr = 0;
	read_fd.filesize = 0;

	ANTS_AVI_OPEN(&read_fd, io_mode_r, FALSE);

	char *pBuffer = new char[READ_RECINDEX_PERSIZE];
	if(NULL == pBuffer)
	{
		OUTPUT_FUNC_LINE;
		ANTS_AVI_CLOSE(&read_fd);
		m_ListMutex.Release();
		return 0;
	}

	S64 iReadSize = 0;

	char startMask[8] , endMask[8];

	memset(startMask, 0, sizeof(startMask));
	memset(endMask, 0, sizeof(endMask));

	snprintf(startMask, sizeof(startMask), "recInS");
	snprintf(endMask, sizeof(endMask), "recInE");

//	PrintRecDateList();

	char temp1[20], temp2[20];

	GetStrFromTime(tBeginTime, temp1);
	GetStrFromTime(tEndTime, temp2);

	AVI_DBG("====iRecType=0x%x  time [%s -- %s]\n",iRecType,temp1, temp2);

	while(NULL !=  node)
	{
		pDateNode = node->element;
		if(!node->element)
			break;
		//if(pDateNode->tDate > tEndTime || pDateNode->tDate < tBeginTime)
		if(compareYMD_Ex(pDateNode->tDate, tBeginTime) < 0 || compareYMD_Ex(pDateNode->tDate, tEndTime) > 0)
		//if(!compareYMD(pDateNode->tDate, tBeginTime) && !compareYMD(pDateNode->tDate, tEndTime))
		{
			node = node->next;
			continue;
		}

		avis_SDCardHead tSDCardHead;
		memset(&tSDCardHead, 0, sizeof(avis_SDCardHead));
		g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);
		if(!(((pDateNode->startAddr >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize)) ||
			((pDateNode->startAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize >= tSDCardHead.tRecFileIndexAddr.startAddr) && (pDateNode->startAddr + pDateNode->file_size + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize <= tSDCardHead.tRecFileIndexAddr.startAddr + tSDCardHead.tRecFileIndexAddr.filesize))))
		{
			node = node->next;
			continue;
		}

		//AVI_DBG("pDateNode->startAddr=%lld  file_size=%lld\n",pDateNode->startAddr,pDateNode->file_size);

		read_fd.startAddr = pDateNode->startAddr;
		read_fd.filesize = pDateNode->file_size;

		iReadSize = 0;

		while(iReadSize < read_fd.filesize)
		{
			S64 iRemainSize = read_fd.filesize-iReadSize;

			//AVI_DBG("read_fd.startAddr=%lld  iReadSize=%d",read_fd.startAddr,iReadSize);

			if(read_fd.startAddr + iReadSize  >=
				g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize) //录像索引超过文件大小
			{
				S64 offset = read_fd.startAddr + iReadSize - g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize;
				lseek64(read_fd.fd, offset, SEEK_SET);
			}
			else
			{
				ANTS_AVI_SEEK(&read_fd, iReadSize, SEEK_SET);
			}

			S64 iCanRead = iRemainSize>READ_RECINDEX_PERSIZE?READ_RECINDEX_PERSIZE:iRemainSize;

			if(read_fd.startAddr + iReadSize < g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize &&
				read_fd.startAddr + iReadSize + iCanRead >= g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize)
			{
				iCanRead = g_antsAviLibInfo.antsStorageManage->m_RecFileIndexStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileIndexMaxSize - read_fd.startAddr - iReadSize;
			}

			memset(pBuffer, 0, sizeof(char)*READ_RECINDEX_PERSIZE);
			S64 iByteRead = ANTS_AVI_READ(&read_fd, pBuffer, iCanRead); //每次最多读READ_RECINDEX_PERSIZE字节

			//AVI_DBG("iByteRead=%d  seg_cnt=%d\n",iByteRead,seg_cnt);

			//在READ_RECINDEX_PERSIZE字节中查找符合的索引结点
			for(S64 i=0; i<iByteRead/REC_INDEX_NODE_LEN; i++)
			{
				pIndexNode = (RecIndexInfo*)(pBuffer + i*REC_INDEX_NODE_LEN);

				#if 0
				int iCount = 0;
				char szBegin[32] = {0},szEnd[32] = {0};
				GetStrFromTime(pIndexNode->begin_sec, szBegin);
				GetStrFromTime(pIndexNode->end_sec, szEnd);
				AVI_WAR("[%d]pIndexNode:%d[%s]=>%d[%s],startAddr:%lld,file_size:%d,use_size:%d\n",iCount++,pIndexNode->begin_sec,szBegin,pIndexNode->end_sec,szEnd,pIndexNode->startAddr,pIndexNode->file_size,pIndexNode->use_size);
				#endif
				if(strncmp(pIndexNode->startMask, startMask, sizeof(startMask))!=0 || strncmp(pIndexNode->endMask, endMask, sizeof(endMask))!=0)
				{
					continue;
				}
				
				if(pIndexNode->begin_sec > tEndTime || pIndexNode->end_sec < tBeginTime || pIndexNode->begin_sec > pIndexNode->end_sec)
				{
					continue;
				}

				if(pIndexNode->begin_sec == pIndexNode->end_sec)
				{
					continue;
				}

				if(0 == (pIndexNode->file_type & iRecType))
				{
					continue;
				}

				if(pIndexNode->file_size <= 0)
				{
					continue;
				}

				ANTS_AVI_CLOSE(&read_fd);
				DEL_ARR_POINTER(pBuffer);

				m_ListMutex.Release();

				return 1;
			}

			iReadSize += iByteRead;
		}

		//AVI_DBG("read_fd.startAddr=%lld  iReadSize=%d",read_fd.startAddr,iReadSize);

		node = node->next;
	}


	ANTS_AVI_CLOSE(&read_fd);
	DEL_ARR_POINTER(pBuffer);

	m_ListMutex.Release();

	return 0;
}

int CRecIndexManage::ReleaseRecSegList(PSS_REGSEG *pRecSegHead)
{
	PSS_REGSEG *seg_first = pRecSegHead;
	//int i=1;
	while(NULL != seg_first)
	{
		//char temp1[20], temp2[20];
		//GetStrFromTime(seg_first->tBeginTime, temp1);
		//GetStrFromTime(seg_first->tEndTime, temp2);
		//AVI_DBG("====node [%d] type=%d  time [%s -- %s]\n",i,seg_first->iRecType,temp1, temp2);
		//i++;

		PSS_REGSEG *seg_node = seg_first->ptNext;
		DEL_POINTER(seg_first);
		seg_first = seg_node;
	}

	return avis_ret_ok;
}


int CRecIndexManage::GetMonthHasRecDays(time_t begin_time, int numOfDays, DWORD &HasMask)
{
    int nDaySec = (24*3600);
	if(NULL == m_lpDateList)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	int nHasRec = 0;

	for(int nDay = 0; nDay < numOfDays; nDay++)
	{
		nHasRec = HasRecSegList(0, begin_time + nDay * nDaySec, begin_time + (nDay + 1) * nDaySec, AVIS_RECORDTYPE_ALL);		
		if(nHasRec)
		{
			HasMask |= (1 << nDay);
		}
	}
	
	return avis_ret_ok;
}



int CRecIndexManage::GetRecSegListEx(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG_EX **pRecSegHead, int *pRecSegCount)
{
	return avis_ret_ok;
}


int CRecIndexManage::ReleaseRecSegListEx(PSS_REGSEG_EX *pRecSegHead)
{
	PSS_REGSEG_EX *seg_first = pRecSegHead;
	while(NULL != seg_first)
	{
		PSS_REGSEG_EX *seg_node = seg_first->ptNext;
		delete seg_first;
		seg_first = seg_node;
	}

	return avis_ret_ok;
}
