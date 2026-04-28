#include "writerManage.h"
#include "globalObj.h"

CWriterManage* CWriterManage::theWriterManage = NULL;

CWriterManage::CWriterManage()
{
	
}

CWriterManage::~CWriterManage()
{
	
}

CWriterManage*CWriterManage::GetMgrItem()
{
	if (NULL == theWriterManage)
	{
		theWriterManage = new CWriterManage();

		if ( NULL == theWriterManage )
		{
			return NULL;
		}
		
		theWriterManage->InitParam();
	}
	return theWriterManage;
}

void CWriterManage::InitParam()
{
	m_cur_rectype = AVIS_RECORDTYPE_TIMER;

	m_start_rec = FALSE;
	m_frame_width = 0;
	m_frame_height = 0;
	memset(&m_recindex_info, 0, sizeof(m_recindex_info));
	m_need_updateRecIndex = FALSE;
	m_cur_date = 0;
	m_codec_id = AntsIPCVideoCodecID_H264_hisi;

	m_recfile_fd = NULL;

	m_nChannel = 0;

	m_pWriteBuffer = NULL;
	m_iFirstWriteForOpen = 0;
	m_dwWriteBufferSize = 0;
}

void CWriterManage::DelMgrItem()
{
	if(NULL !=  theWriterManage)
		theWriterManage->StopRec();
	
	DEL_POINTER(theWriterManage);
}

int CWriterManage::write_file(void *pBuffer, DWORD dwBuffsize)
{
	if( !IsFileOen(m_recfile_fd) || NULL == m_pWriteBuffer)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	S64 bytesWriten = 0;
	S64 dwTotalSize = 0;
	unsigned int dwRemainSize = 0;
	unsigned char *pBufferTemp = (unsigned char*)pBuffer;

	S64 iOffset = 0;

	if (m_dwWriteBufferSize & 3)
	{
		m_dwWriteBufferSize = m_dwWriteBufferSize + (4 - (m_dwWriteBufferSize&3));
	}

	//AVI_DBG("=====fd.fd:%d  startAddr:%lld filesize:%lld  datalen:%d io_direct:%d\n",m_recfile_fd->fd, m_recfile_fd->startAddr, m_recfile_fd->filesize, dwBuffsize, m_recfile_fd->m_io_direct);

	//每次写64K
	{
		//64K缓存中剩下的空间
		dwRemainSize = PER_WRITESIZE - m_dwWriteBufferSize;
		if(dwRemainSize >= 0)
		{
			if(dwBuffsize < dwRemainSize)
			{
				//printf("%s %s %d==============dwBuffsize+dwRemainSize:%d\n",__FILE__,__FUNCTION__,__LINE__,m_dwWriteBufferSize);
				memcpy(m_pWriteBuffer+m_dwWriteBufferSize, pBufferTemp, dwBuffsize);
				m_dwWriteBufferSize += dwBuffsize;
				return 0;
			}
			else
			{	
				S64 pos = ANTS_AVI_SEEK(m_recfile_fd, 0, SEEK_END);
				while(1)
				{
					if(dwRemainSize > 0)
					{
						memcpy(m_pWriteBuffer+m_dwWriteBufferSize, pBufferTemp, dwRemainSize);
					}

					for(int i=0; i<PSS_MAX_DATAPOPER; i++)
					{
						if(g_antsAviLibInfo.antsReaderManage->m_bIdle[i] == HANDLE_BUSY && NULL != g_antsAviLibInfo.antsReaderManage->m_data_reader[i])
						{
							//当前写的地方正好在读
							if(m_recfile_fd->startAddr+pos+dwTotalSize <= g_antsAviLibInfo.antsReaderManage->m_data_reader[i]->m_CurReadPosInSDCard && 
								m_recfile_fd->startAddr+pos+dwTotalSize+PER_WRITESIZE >= g_antsAviLibInfo.antsReaderManage->m_data_reader[i]->m_CurReadPosInSDCard)
							{
								AVI_DBG("pos:%lld dwTotalSize:%lld  g_antsAviLibInfo.antsReaderManage->m_data_reader[i]->m_CurReadPosInSDCard=%lld",pos,dwTotalSize,g_antsAviLibInfo.antsReaderManage->m_data_reader[i]->m_CurReadPosInSDCard);
								g_antsAviLibInfo.antsReaderManage->m_data_reader[i]->DataPopStop();
								g_antsAviLibInfo.antsReaderManage->m_data_reader[i]->DataPopRelease();
							}
						}
					}
					unsigned int tBegin = 0,tEnd = 0;	
					tBegin = get_cur_time_ms();
					bytesWriten = ANTS_AVI_WRITE(m_recfile_fd, m_pWriteBuffer, PER_WRITESIZE);
					if(bytesWriten <= 0)
					{
						AVI_ERR("Write RecFile Failed!\n");
						bytesWriten = 0;
					}
					tEnd = get_cur_time_ms();
					if(tEnd-tBegin > 1000)
					{
						AVI_ERR("Write RecFile cost %ums!\n",tEnd-tBegin);
					}
					dwTotalSize += bytesWriten;

					//sleep_ms(1);

					m_dwWriteBufferSize = 0;

					iOffset += dwRemainSize;
					pBufferTemp = (unsigned char*)pBuffer+iOffset;
					
					dwBuffsize -= dwRemainSize;

					//剩余的不够64K
					if(dwBuffsize < PER_WRITESIZE)
					{
						break;
					}

					dwRemainSize = PER_WRITESIZE;
				}

				if(dwBuffsize > 0)
				{
					memcpy(m_pWriteBuffer, pBufferTemp, dwBuffsize);
					m_dwWriteBufferSize = dwBuffsize;
				}

				//AVI_DBG("fd.fd:%d  startAddr:%lld filesize:%lld  %lldKB %lldMB  ReMainDataSize:%u dwTotalWriteSize:%lld\n",fd->fd, fd->startAddr, fd->filesize, fd->filesize>>10,fd->filesize>>20, dwBuffsize, dwTotalSize);
			}
		}
	}

	return dwTotalSize;
}

int CWriterManage::StartRec(int type)
{
	if(m_start_rec == TRUE)
	{
		return avis_ret_ok;
	}
	
	int nRet = 0;
	m_start_rec = TRUE;
	m_cur_rectype = type;
	bIsWritting = FALSE;

	if(NULL !=  m_pWriteBuffer)
	{
		free (m_pWriteBuffer);
		m_pWriteBuffer = NULL;
	}

	posix_memalign((void**)&m_pWriteBuffer, DMA_ADDR_ALIGN, PER_WRITESIZE);
	if(NULL == m_pWriteBuffer)
	{
		AVI_ERR("m_pWriteBuffer error!!!\n\n\n");
		return avis_ret_failed;
	}

	memset(m_pWriteBuffer, 0, PER_WRITESIZE);
	m_dwWriteBufferSize = 0;
	//AVI_DBG("======m_pWriteBuffer:%x \n",(int)m_pWriteBuffer);

	if(NULL !=  m_recfile_fd)
	{
		ANTS_AVI_CLOSE(m_recfile_fd);
		DEL_POINTER(m_recfile_fd);
		m_recfile_fd = NULL;
	}

	m_recfile_fd = new avis_fileHanedle;

	if(NULL == m_recfile_fd)
	{
		AVI_ERR("m_recfile_fd is null!!!\n");
				
		return avis_ret_failed;
	}

	//获取当前写录像位置
	if(avis_ret_failed == GetRecFileName())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	//打开录像文件
#ifdef ByteAlignmentUseIO
	nRet = ANTS_AVI_OPEN(m_recfile_fd, io_mode_w, TRUE);
#else
	nRet = ANTS_AVI_OPEN(m_recfile_fd, io_mode_w, FALSE);
#endif

	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

#if (WRITE_DBG==1)
	m_fdDebug = open("/mnt/write.i8",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
#endif

	m_iFirstWriteForOpen = 0;
	
	return avis_ret_ok;
}

int CWriterManage::StopRec()
{
	if(m_start_rec == FALSE)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_ok;
	}
	
	m_recindex_info.startAddr = m_recfile_fd->startAddr;
	m_recindex_info.file_size = m_recfile_fd->filesize;
	WriteRemainData();
	UpdateRecIndex(FALSE);

	m_start_rec = FALSE;

	if(NULL != m_recfile_fd)// && IsFileOen(m_recfile_fd))
	{
		ANTS_AVI_CLOSE(m_recfile_fd);
		DEL_POINTER(m_recfile_fd);
	}

	if(NULL !=  m_pWriteBuffer)
	{
		free (m_pWriteBuffer);
		m_pWriteBuffer = NULL;
	}

	InitParam();
	
	return avis_ret_ok;
}

int CWriterManage::SetRecFrame(int iChannel, char *pBuffer, int nBufferLen, DWORD dwCurTimeSec, DWORD dwCurTimeUsec)
{
	m_nChannel = iChannel;
	if(!m_start_rec)
	{
		AVI_ERR("channel %d: record has not stared\n", iChannel);
		return avis_ret_not_start;
	}

	AntsIPCFrameHeader_T *pFrameHead = (AntsIPCFrameHeader_T *)pBuffer;
	
	//AVI_DBG("=========sizeof(AntsIPCFrameHeader_T):%d  nDataLen:%d\n",sizeof(AntsIPCFrameHeader_T),nBufferLen);
	if(is_audio_frame(pFrameHead->uiFrameType))
	{
		// 音频帧是8个帧在一起传过来的
		char *pHead = (char *)pFrameHead;
		AntsIPCFrameHeader_T *cMyHead = (AntsIPCFrameHeader_T *)pHead;
		int nLen = cMyHead->uiFrameLen;

		int nIdx = 0;
		int audio_cnt =  nBufferLen / (FRAME_HEAD_LEN + nLen);
		while(nIdx < audio_cnt)
		{
			cMyHead = (AntsIPCFrameHeader_T *)pHead;
			nLen = cMyHead->uiFrameLen;
			//cMyHead->uiFrameTime = dwCurTimeSec;
			//cMyHead->uiFrameTickCount = dwCurTimeUsec + nIdx * 40000;
			if(cMyHead->uiFrameTickCount >= 1000000)
			{
				cMyHead->uiFrameTickCount -= 1000000;
				cMyHead->uiFrameTime += 1;
			}
			WriteAviFile(pHead, FRAME_HEAD_LEN+nLen);
			if(nIdx == 0)
			{
				pHead = pHead + FRAME_HEAD_LEN + nLen;
			}
			else
			{
				pHead += FRAME_HEAD_LEN + nLen;
			}
			nIdx++;
		}
	}
	else if(is_video_frame(pFrameHead->uiFrameType) || is_smart_frame(pFrameHead->uiFrameType))
	{	
		// 录像时间以系统时间为准
		//pFrameHead->uiFrameTime = dwCurTimeSec;
		//pFrameHead->uiFrameTickCount = dwCurTimeUsec;
		WriteAviFile(pBuffer, nBufferLen);
	}
	else
	{
		AVI_WAR("unexpected frame, type = %d, len = %d\n", pFrameHead->uiFrameType, nBufferLen);
	}
	return avis_ret_ok;
}

int CWriterManage::WriteRemainData()
{
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();

	avis_SDCardHead tSDCardHead;
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	char *temp = NULL;
	posix_memalign((void**)&temp, DMA_ADDR_ALIGN, PER_WRITESIZE);
	if(NULL == temp)
	{
		AVI_ERR("temp error!!!\n\n\n");
		return avis_ret_failed;
	}
	memset(temp, 0, PER_WRITESIZE);

	//AVI_DBG("WriteRemainData\n");

#ifdef ByteAlignmentUseIO
	unsigned int byteAlign = 512;
#else
	unsigned int byteAlign = 4;
#endif

	unsigned int iNeedFill = 0;
	if(m_dwWriteBufferSize > 0)
	{
		m_recindex_info.file_size += m_dwWriteBufferSize;
		m_recindex_info.use_size = m_recindex_info.file_size;
		
		if(m_dwWriteBufferSize%byteAlign != 0)
		{
			iNeedFill = byteAlign - m_dwWriteBufferSize%byteAlign;
			
			memcpy(m_pWriteBuffer+m_dwWriteBufferSize, temp, iNeedFill);

			m_dwWriteBufferSize += iNeedFill;
		}

		//AVI_DBG("m_dwWriteBufferSize=%d iNeedFill=%d\n",m_dwWriteBufferSize,iNeedFill);

		ANTS_AVI_SEEK(m_recfile_fd, 0, SEEK_END);
		ANTS_AVI_WRITE(m_recfile_fd, m_pWriteBuffer, m_dwWriteBufferSize);
		m_dwWriteBufferSize = 0;

		m_recindex_info.use_size += iNeedFill;
	}
	
	tSDCardHead.tRecFileAddr.filesize += m_recfile_fd->filesize;

	S64 extralsize = 0;

	//把剩余的1M分配给最后一个文件
	avis_address addr;
	addr.startAddr = tSDCardHead.tRecFileAddr.startAddr;
	addr.filesize = tSDCardHead.tRecFileAddr.filesize;
	if(addr.startAddr + addr.filesize + MBYTE_UNIT >= g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize
		&& addr.startAddr + addr.filesize <= g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
	{
		AVI_DBG("address is to the max addr file_addr = %lld file_size = %lld\n",m_recfile_fd->startAddr, m_recfile_fd->filesize);
		
		AVI_DBG("addr.startAddr=%lld addr.filesize=%lld\n",addr.startAddr, addr.filesize);

		extralsize = g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize - addr.startAddr - addr.filesize;

		AVI_DBG("m_recindex_info startAddr=%lld filesize=%d use_size=%d  extralsize=%lld\n",m_recindex_info.startAddr,m_recindex_info.file_size,m_recindex_info.use_size, extralsize);		

		m_recindex_info.use_size += extralsize;
		tSDCardHead.tRecFileAddr.filesize += extralsize;

		if(m_recindex_info.startAddr + m_recindex_info.use_size > g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
		{
			AVI_ERR("here\n");
		}

		ANTS_AVI_SEEK(m_recfile_fd, 0, SEEK_END);
		int iCount = 0;
		while(extralsize > 0)
		{
			S64 byte = ANTS_AVI_WRITE(m_recfile_fd, temp, extralsize>PER_WRITESIZE?PER_WRITESIZE:extralsize);
			ANTS_AVI_SEEK(m_recfile_fd, 0, SEEK_END);

			extralsize -= byte;

			iCount++;
			if(iCount == 3) //写三次64K
			{
				break;
			}
		}

		//m_recfile_fd->filesize += extralsize;

		AVI_DBG("m_fd startAddr=%lld filesize=%lld\n",m_recfile_fd->startAddr, m_recfile_fd->filesize);
		AVI_DBG("m_recindex_info.filesize=%d m_recindex_info.use_size=%d  extralsize=%lld\n",m_recindex_info.file_size,m_recindex_info.use_size, extralsize);
		AVI_DBG("tSDCardHead.tRecFileAddr.filesize=%lld\n",tSDCardHead.tRecFileAddr.filesize);
	}

	int iRet = g_antsAviLibInfo.antsStorageManage->SetSDCardHead(&tSDCardHead);
	if(iRet == avis_ret_failed)
	{
		AVI_ERR("m_dwWriteBufferSize=%d iNeedFill=%d  addr.startAddr=%lld size=%lld!!\n",m_dwWriteBufferSize,iNeedFill,addr.startAddr,addr.filesize);
	}

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	if(temp != NULL)
	{
		free(temp);
		temp = NULL;
	}

	//AVI_DBG("WriteRemainData___END\n");

	return avis_ret_ok;
}


int CWriterManage::WriteAviFile(char *buf, int buf_len)
{
	if(!IsFileOen(m_recfile_fd))
	{
		AVI_ERR("file not open!!!\n");
		return avis_ret_failed;
	}

	if(m_iFirstWriteForOpen == 0)
	{
		m_iFirstWriteForOpen = 1;
	}
	else
	{
		m_iFirstWriteForOpen = 2;
	}

	AntsIPCFrameHeader_T *pFrameHead =  (AntsIPCFrameHeader_T *)buf;

	time_t cur_sec = pFrameHead->uiFrameTime;

	static int byteWrite = 0;
	static unsigned int startWriteTime = 0;
	static unsigned int endWriteTime = 0;

	S64 cur_msec = (S64)pFrameHead->uiFrameTime*1000 + (S64)pFrameHead->uiFrameTickCount/1000;
	static S64 sLastTime = 0;
	
	BOOL bIsIFrame = is_i_frame(pFrameHead->uiFrameType);
	BOOL bIsVideoFrame = is_video_frame(pFrameHead->uiFrameType);

	//第一次写时等待一个I帧
/*	if(1 == m_iFirstWriteForOpen && !bIsIFrame)
	{
		m_iFirstWriteForOpen = 0;
		return avis_ret_ok;
	}
*/

	unsigned int iFrameNo = pFrameHead->uiFrameNo;
	static unsigned int sLastFrameNo = iFrameNo;
	
	//与上一帧间隔大于1s时
	if(bIsVideoFrame && (cur_msec - sLastTime > 10000))
	{		
		AVI_DBG("[%02d] Frame Interval > 1s, need to update recfile, now = %lld, last = %lld  chazhi:%lld  No:[%u -> %u]\n", m_nChannel, cur_msec, sLastTime,cur_msec-sLastTime,sLastFrameNo, iFrameNo);

		AVI_DBG("last write time [%u - %u]  writeByte:%d  cost: %d ms",startWriteTime, endWriteTime, byteWrite, endWriteTime-startWriteTime);
		m_need_updateRecIndex = true;
	}

	// 日期改变了，需要更新录像索引
	if(bIsVideoFrame && (m_cur_date != cur_sec / DAY_SEC_CNT))
	{
		AVI_DBG("[%02d] day changed, need to update recfile, now = %d, last = %d\n", m_nChannel, (int)(cur_sec / DAY_SEC_CNT), (int)m_cur_date);
		m_cur_date = cur_sec / DAY_SEC_CNT;
		
		m_need_updateRecIndex = true;
	}

	//录像时长超过20s，需要更新录像索引
	if(bIsVideoFrame && (cur_sec - m_recindex_info.begin_sec >= 20 || cur_sec < m_recindex_info.end_sec))
	{
		AVI_DBG("[%02d] file_size or time exceed, need to update recfile, cur_sec = %d, begin_sec = %d, end_sec = %d\n",
			m_nChannel,  (int)cur_sec, (int)m_recindex_info.begin_sec, (int)m_recindex_info.end_sec);
		m_need_updateRecIndex = true;
	}

	if(bIsVideoFrame && iFrameNo != sLastFrameNo + 1)
	{
		AVI_DBG("sLastFrameNo=%d -> iFrameNo=%d\n",sLastFrameNo, iFrameNo);
	}

	if(bIsVideoFrame)
	{
		sLastTime = cur_msec;
		sLastFrameNo = iFrameNo;
	}

	//文件写到末尾时，需要切换录像文件
	if(m_recfile_fd->startAddr + m_recfile_fd->filesize + MBYTE_UNIT >= g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
	{
		AVI_DBG("address is to the max addr, need to update recfile, file_addr = %lld file_size = %lld\n",m_recfile_fd->startAddr, m_recfile_fd->filesize);
		m_need_updateRecIndex = true;
	}
	
	if(bIsIFrame)
	{
		// 分辨率改变了，需要更新录像索引
		int frame_width = pFrameHead->uMedia.struVideoHeader.usWidth;
		int frame_height = pFrameHead->uMedia.struVideoHeader.usHeight;
		if(m_frame_width != frame_width || m_frame_height != frame_height)
		{
			AVI_DBG("[%02d] resolution changed, need to update recfile, now = (%d, %d), origin = (%d, %d)\n", m_nChannel, frame_width, frame_height, m_frame_width, m_frame_height);

			m_frame_width = frame_width;
			m_frame_height = frame_height;
			
			m_need_updateRecIndex = true;
		}
		//编码改变了，需要更新录像索引
		if(m_codec_id != pFrameHead->uMedia.struVideoHeader.cCodecId)
		{
			AVI_DBG("[%02d] codec id changed, need to update recfile, now = %d, last = %d\n", m_nChannel, pFrameHead->uMedia.struVideoHeader.cCodecId, m_codec_id);
			m_codec_id = pFrameHead->uMedia.struVideoHeader.cCodecId;
			
			m_need_updateRecIndex = true;
		}
	}

	startWriteTime = get_cur_time_ms();

	if(m_need_updateRecIndex)
	{
		// 更新索引文件
		if(m_iFirstWriteForOpen != 1) //第一次写时，没有写入索引文件，所以不用更新，只需要插入一个索引
		{
			WriteRemainData();

			UpdateRecIndex(FALSE);
		}
		
		m_need_updateRecIndex = FALSE;

		GetRecFileName();

		// 添加索引文件
		memset(&m_recindex_info, 0, sizeof(m_recindex_info));
		snprintf(m_recindex_info.startMask, sizeof(m_recindex_info.startMask), "recInS");
		m_recindex_info.startAddr = m_recfile_fd->startAddr;
		m_recindex_info.file_size = 0;
		m_recindex_info.use_size = 0;
		m_recindex_info.file_type = m_cur_rectype;
		m_recindex_info.begin_sec = cur_sec;
		m_recindex_info.end_sec = m_recindex_info.begin_sec;

		memset(m_recindex_info.iFrameOffset, 0, sizeof(m_recindex_info.iFrameOffset));
		
		snprintf(m_recindex_info.endMask,sizeof(m_recindex_info.endMask), "recInE");
		UpdateRecIndex(TRUE);
	}

	if(bIsIFrame)
	{
		int iTimeOffset = cur_sec - m_recindex_info.begin_sec;

		if( iTimeOffset >= 0 && iTimeOffset < IFRAMEOFFSETNUM)
		{
			if(m_recfile_fd->filesize + m_dwWriteBufferSize > m_recindex_info.iFrameOffset[iTimeOffset])
			{
				m_recindex_info.iFrameOffset[iTimeOffset] = m_recfile_fd->filesize + m_dwWriteBufferSize;
			}
		}
	}

	if(bIsVideoFrame)
	{
		m_recindex_info.end_sec = cur_sec;
	}

	byteWrite = write_file(buf, buf_len);

	endWriteTime = get_cur_time_ms();
	
	if(byteWrite > 0)
	{
		m_recindex_info.file_size += byteWrite;
		m_recindex_info.use_size = m_recindex_info.file_size;

		bIsWritting = TRUE;
	}

#if (WRITE_DBG==1)
	safe_write(m_fdDebug,buf, buf_len);
#endif
	
	return avis_ret_ok;
}


int CWriterManage::GetRecFileName()
{
	g_antsAviLibInfo.antsStorageManage->m_headMutex.Wait();

	avis_SDCardHead tSDCardHead;
	g_antsAviLibInfo.antsStorageManage->GetSDCardHead(&tSDCardHead);

	avis_address addr;
	addr.startAddr = tSDCardHead.tRecFileAddr.startAddr;
	addr.filesize = tSDCardHead.tRecFileAddr.filesize;

	if(addr.startAddr + addr.filesize >= g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr + g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
	{
		addr.startAddr = addr.startAddr + addr.filesize - g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize;
		addr.filesize = 0;
	}

	m_recfile_fd->startAddr = addr.startAddr + addr.filesize;
	m_recfile_fd->filesize = 0;

	g_antsAviLibInfo.antsStorageManage->m_headMutex.Release();

	return avis_ret_ok;
}


//=true insert  =FALSE update
int CWriterManage::UpdateRecIndex(BOOL bneedInsertInfo)  
{
	if(m_start_rec == FALSE)
	{
		return avis_ret_failed;
	}

	if(m_start_rec == FALSE)
	{
		return avis_ret_not_start;
	}
	
	int iRet = avis_ret_failed;
	if(bneedInsertInfo)
	{
		iRet = g_antsAviLibInfo.antsRecIndexManage->AddOneRecIndexNodeToDateList(&m_recindex_info, TRUE);
	}
	else
	{
		for(int i=0; i<IFRAMEOFFSETNUM-1; i++)
		{
			if(m_recindex_info.iFrameOffset[i] > m_recindex_info.iFrameOffset[i+1])
			{
				m_recindex_info.iFrameOffset[i+1] = m_recindex_info.iFrameOffset[i];
			}
		}
		
		iRet = g_antsAviLibInfo.antsRecIndexManage->UpdateOneRecIndexNode(&m_recindex_info);
	}

	return iRet;
}


