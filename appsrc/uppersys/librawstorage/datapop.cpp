#include "datapop.h"
#include "globalObj.h"
#include <syslog.h>
#include <sys/syscall.h>

static S64 fast_find_node(unsigned char *buf, S64 scan_size, S64 *pos)
{
	for (S64 i = 0; i < scan_size - 3; i++)
	{
		if ((buf[i] == 0x00) && (buf[i+1] == 0x00) && (buf[i+2] == 0x01) && (buf[i+3] == 0xAB))
		{
			*pos = i;
			return avis_ret_ok;
		}
	}
	return avis_ret_failed;
}

static void *ReadThread(void *lpParam)
{
	CDataPop *datapop = (CDataPop *)lpParam;
	int nRet = 0;

	AVI_DBG("pid=%ld enter\n", syscall(SYS_gettid));
    syslog(LOG_NOTICE, "[%s:%d] pid=%ld enter\n", __FUNCTION__, __LINE__, syscall(SYS_gettid));

	if(NULL == datapop)
	{
		return NULL;
	}

	ANTS_AVI_CLOSE(datapop->m_readfd);
	DEL_POINTER(datapop->m_readfd);

	//打开录像文件			
	datapop->m_readfd = new avis_fileHanedle;
	avis_fileHanedle *fd = datapop->m_readfd;
	if(NULL == fd)
	{
		return NULL;
	}

	fd->fd = 0;
	fd->startAddr = g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr;
	fd->filesize = 0;

	nRet = ANTS_AVI_OPEN(fd, io_mode_r, FALSE);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return NULL;
	}

	ANTS_AVI_SEEK(fd, 0, SEEK_SET);

	S64 nByteRead = 0;

	unsigned char *pBuffer = NULL;
	AntsIPCFrameHeader_T *pFrameHead = NULL;
	time_t startTime=0, endTime=0;

	time_t curFrameTime = 0;

	//avis_address addr;
	unsigned int dwCurRecType = 0;
	frame_read_mod_e eCurReadMode = READ_ALL_DATA;
	int iCurSpeed = 0;

	unsigned int dwNeedDropIFrameNum = 0;

	BOOL bFirstPop = TRUE;

	avis_recfileindexlist *reclist=NULL;
	avis_recfileindexlist *head=NULL;
	int reclistCount=0;

	avis_iFrameindexlist *iFramelist = NULL;
	avis_iFrameindexlist *iFrameHead = NULL;
	int iFrameCount = 0;

	BOOL bFirstReadData = TRUE;
	BOOL bTimeOver = TRUE;
	BOOL needSwitchFile = TRUE;
	int iFileCount = 0;
#ifdef JZT31N	
	BOOL MemBig2Small = FALSE;
	S64 dwMaxReadBufferSize = MAX_READERBUFFERSIZE;
	unsigned char  *pTmpReadBuffer = NULL;	
#endif	
/*	int fdDebug = open("/mnt/write.i8",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	char temp[28] = {0x00,0x00,0x01,0xAA,0x03,0x00,0x00,0x00,0x19,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0};

	safe_write(fdDebug, temp, 28);
*/	
	while(1)
	{
		if(datapop->m_epop_state == epop_state_destroy)
		{
			AVI_DBG("epop_state_destroy\n");
			break;
		}
		
		if(datapop->m_epop_state != epop_state_start)
		{
			sleep_ms(500);
			continue;
		}

		bTimeOver = FALSE;
		
		if(datapop->m_bNeedReLocate)
		{
			if(NULL != datapop->m_playCallback)
			{
				datapop->m_playCallback(datapop->m_hParenHandle, 0, FALSE, 0, 0, NULL,0, DATAPOP_EVENT_TIME_JUMP, datapop->m_pUserData);
			}
			
			dwCurRecType = datapop->m_rec_type_mask;
			eCurReadMode = datapop->m_read_mod; 

			AVI_DBG("iCurSpeed=%d   datapop->m_iCurSpeed=%d  dwCurRecType=%d\n",iCurSpeed, datapop->m_iCurSpeed,dwCurRecType);

			//如果时间没有重新设置，那就是快进了
			if(iCurSpeed != datapop->m_iCurSpeed)
			{					
				startTime = curFrameTime;
			}
			else
			{
				startTime = datapop->m_seek_start_time;
				endTime = datapop->m_seek_end_time;
			}

			iCurSpeed = datapop->m_iCurSpeed;
			//当快放到8倍速以上，需要丢掉I帧
			//8倍速的情况，不丢I帧   16倍速的情况，隔1帧丢1帧

			if(iCurSpeed == 3)
				dwNeedDropIFrameNum = -1;
			else if(iCurSpeed == 4)
				dwNeedDropIFrameNum = 2;
			
			{
				char temp1[20], temp2[20];
				GetStrFromTime(startTime, temp1);
				GetStrFromTime(endTime, temp2);

				AVI_DBG("seek time [%s -- %s]\n",temp1, temp2);
				
				g_antsAviLibInfo.antsRecIndexManage->ReleaseList(reclist);
				reclist = NULL;
				reclistCount = 0;

				g_antsAviLibInfo.antsRecIndexManage->ReleaseIFrameList(iFramelist);
				iFramelist = NULL;
				iFrameCount = 0;
				
				iFileCount = 0;

				if(iCurSpeed < 3)
				{
					g_antsAviLibInfo.antsRecIndexManage->FindInfFileForList(startTime, endTime, dwCurRecType, &reclist, &reclistCount, iCurSpeed>=3?TRUE:FALSE);

					head = reclist;
				}
				else
				{
					g_antsAviLibInfo.antsRecIndexManage->FindIFrameInFileForList(startTime, endTime, dwCurRecType, &iFramelist, &reclistCount);

					iFrameHead = iFramelist;

					bFirstReadData = TRUE;
				}

				needSwitchFile = TRUE;
			}

			datapop->m_bNeedReLocate = FALSE;
		}

		if(TRUE == needSwitchFile) //切换文件，并清空缓冲
		{
			needSwitchFile = FALSE;

			bFirstPop = TRUE;

			if(iCurSpeed < 3)
			{
				if(head ==NULL || iFileCount >= reclistCount)
				{
					AVI_DBG("find file is over!  iFrameHead=%p  iFileCount=%d reclistCount=%d \n", head, iFileCount ,reclistCount);
					datapop->DataPopStop();
					
					if(bTimeOver == FALSE)
					{
						if(NULL != datapop->m_playCallback)
						{
							AVI_DBG("DATAPOP_EVENT_TIME_OVER\n");
							datapop->m_playCallback(datapop->m_hParenHandle, 0, FALSE, 0, 0, NULL,0, DATAPOP_EVENT_TIME_OVER, datapop->m_pUserData);
							sleep_ms(100);
						}
						bTimeOver = TRUE;
					}
					continue;
				}

				fd->startAddr = head->startAddr;
				fd->filesize = head->filesize;
				ANTS_AVI_SEEK(fd, 0, SEEK_SET);

				head = head->ptNext;
				iFileCount++;
			}
			else  //8倍速以上
			{
				if(iFrameHead == NULL || iFileCount >= reclistCount || iFrameCount >= IFRAMEOFFSETNUM)
				{
					AVI_DBG("find file is over!  iFrameHead=%p  iFileCount=%d reclistCount=%d  iFrameCount=%d\n", iFrameHead, iFileCount ,reclistCount,iFrameCount);
					datapop->DataPopStop();
					
					if(bTimeOver == FALSE)
					{
						if(NULL != datapop->m_playCallback)
						{
							AVI_DBG("DATAPOP_EVENT_TIME_OVER\n");
							datapop->m_playCallback(datapop->m_hParenHandle, 0, FALSE, 0, 0, NULL,0, DATAPOP_EVENT_TIME_OVER, datapop->m_pUserData);
							sleep_ms(100);
						}
						bTimeOver = TRUE;
					}
					continue;
				}

				if(iFileCount == 0 && bFirstReadData) //第一次找符合条件的I帧,否则会读取无效的数据导致切换时候很慢
				{
					if(startTime - iFrameHead->recIndexInfo.begin_sec > 0)
					{
						iFrameCount = startTime - iFrameHead->recIndexInfo.begin_sec;
					}
					
					iFrameCount = iFrameCount-2;

					if(iFrameCount < 0)
						iFrameCount = 0;
					if(iFrameCount >= IFRAMEOFFSETNUM)
					{
						AVI_ERR("iFrameCount = %d\n",iFrameCount);
						iFileCount = 0;
					}

					bFirstReadData = FALSE;
				}
				
				if(iFrameCount >= IFRAMEOFFSETNUM)
				{
					needSwitchFile = TRUE;
					continue;
				}

				fd->startAddr = iFrameHead->recIndexInfo.startAddr + iFrameHead->recIndexInfo.iFrameOffset[iFrameCount];

				if(iFrameCount == IFRAMEOFFSETNUM-1)
				{
					fd->filesize = iFrameHead->recIndexInfo.use_size - iFrameHead->recIndexInfo.iFrameOffset[iFrameCount];
				}
				else
				{
					fd->filesize = iFrameHead->recIndexInfo.iFrameOffset[iFrameCount+1] - iFrameHead->recIndexInfo.iFrameOffset[iFrameCount];

					if(fd->filesize == 0)
					{
						fd->filesize = iFrameHead->recIndexInfo.file_size - iFrameHead->recIndexInfo.iFrameOffset[iFrameCount];
					}
				}
				
				ANTS_AVI_SEEK(fd, 0, SEEK_SET);

				iFrameCount ++;

				//AVI_DBG("======iFrameCount=%d   fd->startAddr=%lld  fd->filesize=%lld\n",iFrameCount,fd->startAddr,fd->filesize);
				
				if(iFrameCount == IFRAMEOFFSETNUM)
				{
					iFrameHead = iFrameHead->ptNext;
					iFileCount++;

					iFrameCount = 0;
				}
			}

			//AVI_DBG("fd->startAddr:%lld  filesize:%lld\n",fd->startAddr,fd->filesize);

			

			datapop->m_dwReadPos = 0;
			datapop->m_dwReadBufferSize = 0;
		}
		
#ifdef JZT31N
		{			
			memmove(datapop->m_pReadBuffer, datapop->m_pReadBuffer + datapop->m_dwReadPos, datapop->m_dwReadBufferSize);
			datapop->m_dwReadPos = 0;		
			
			S64 curPos = ANTS_AVI_SEEK(fd, 0, SEEK_CUR);

			datapop->m_CurReadPosInSDCard = curPos + fd->startAddr;
			//AVI_DBG("curPos:%lld   g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize=%lld\n",fd->startAddr+curPos,g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize);
			if(curPos > fd->filesize || fd->startAddr+curPos+800*KBYTE_UNIT >= g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
			{
				AVI_DBG("read over curPos:%lld  fd->filesize=%lld  datapop->m_dwReadBufferSize=%lld\n",curPos,fd->filesize,datapop->m_dwReadBufferSize);
				//sleep_ms(1000);
				needSwitchFile = TRUE;
				continue;
			}
			
			nByteRead = ANTS_AVI_READ(fd, datapop->m_pReadBuffer+ datapop->m_dwReadPos + datapop->m_dwReadBufferSize, dwMaxReadBufferSize - datapop->m_dwReadPos - datapop->m_dwReadBufferSize);

			datapop->m_dwReadBufferSize += nByteRead; 
		}
		
		MemBig2Small = FALSE;
#else
		if(datapop->m_dwReadPos + datapop->m_dwReadBufferSize + PER_READSIZE <= MAX_READERBUFFERSIZE)
		{
			S64 curPos = ANTS_AVI_SEEK(fd, 0, SEEK_CUR);

			datapop->m_CurReadPosInSDCard = curPos + fd->startAddr;
			//AVI_DBG("curPos:%lld   g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize=%lld\n",fd->startAddr+curPos,g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize);
			if(curPos > fd->filesize || fd->startAddr+curPos+800*KBYTE_UNIT >= g_antsAviLibInfo.antsStorageManage->m_RecFileStartAddr+g_antsAviLibInfo.antsStorageManage->m_RecFileMaxSize)
			{
				//AVI_DBG("read over curPos:%lld  fd->filesize=%lld  datapop->m_dwReadBufferSize=%lld\n",curPos,fd->filesize,datapop->m_dwReadBufferSize);
				//sleep_ms(1000);
				needSwitchFile = TRUE;
				continue;
			}
			nByteRead = ANTS_AVI_READ(fd, datapop->m_pReadBuffer+ datapop->m_dwReadPos + datapop->m_dwReadBufferSize, PER_READSIZE);

			if(nByteRead != PER_READSIZE)
			{
				AVI_ERR("read error  fd->fd=%d startAddr[%lld] filesize[%lld] m_dwReadPos[%lld] m_dwReadBufferSize[%lld] nByteRead=%lld\n",
					fd->fd, fd->startAddr, fd->filesize ,datapop->m_dwReadPos, datapop->m_dwReadBufferSize,nByteRead);
				//break;
			}

			datapop->m_dwReadBufferSize += nByteRead;

			//AVI_DBG("read curFilePos:%lld  datapop->m_dwReadPos=%lld m_dwReadBufferSize=%lld\n",curPos,datapop->m_dwReadPos,datapop->m_dwReadBufferSize);
		}
		else
		{
/*			//512字节对齐
			S64 pos = 0;

		#ifdef ByteAlignmentUseIO
			int byteAlign = 512;
		#else
			int byteAlign = 4;
		#endif

			if(datapop->m_dwReadBufferSize%byteAlign != 0)
			{
				pos = byteAlign - datapop->m_dwReadBufferSize%byteAlign;
			}
*/
			//AVI_DBG("move datapop->m_dwReadPos[%lld] datapop->m_dwReadBufferSize[%lld]",datapop->m_dwReadPos,datapop->m_dwReadBufferSize);
			
			memmove(datapop->m_pReadBuffer, datapop->m_pReadBuffer + datapop->m_dwReadPos, datapop->m_dwReadBufferSize);
			datapop->m_dwReadPos = 0;

			//AVI_DBG("move datapop->m_dwReadPos[%lld] m_dwReadBufferSize[%lld] pos[%d]\n",datapop->m_dwReadPos,datapop->m_dwReadBufferSize,pos);

			continue;
		}
#endif

		//读取完缓冲中所有的帧
		while(1)
		{
			S64 pos = 0;
			pFrameHead = NULL;
			S64 FrameLen = 0;

			if(datapop->m_epop_state == epop_state_destroy)
			{
				AVI_DBG("epop_state_destroy\n");
				break;
			}

			if(datapop->m_bNeedReLocate)
			{
				break;
			}

			pBuffer = datapop->m_pReadBuffer + datapop->m_dwReadPos;

			if(datapop->m_dwReadBufferSize < FRAME_HEAD_LEN)
			{
				//缓冲不够一个帧头了
				//AVI_DBG("==============datapop->m_dwReadBufferSize:%lld  break\n",datapop->m_dwReadBufferSize);
				break;
			}
			
			if(avis_ret_ok == fast_find_node(pBuffer, datapop->m_dwReadBufferSize, &pos))
			{
				datapop->m_dwReadPos += pos;
				datapop->m_dwReadBufferSize -= pos;
				pBuffer = datapop->m_pReadBuffer + datapop->m_dwReadPos;

				pFrameHead = (AntsIPCFrameHeader_T *)pBuffer;
				FrameLen = pFrameHead->uiFrameLen;
#ifdef JZT31N
				//帧长度 > 当前缓冲区大小，小内存变大内存
				if(FrameLen + FRAME_HEAD_LEN > dwMaxReadBufferSize)
				{			
					dwMaxReadBufferSize = ((FrameLen + FRAME_HEAD_LEN) / DMA_ADDR_ALIGN + 1) * DMA_ADDR_ALIGN;
					AVI_DBG("realloc bigger memory. FrameLen + FRAME_HEAD_LEN=[%lld].dwMaxReadBufferSize=[%u]",FrameLen + FRAME_HEAD_LEN,dwMaxReadBufferSize);
					posix_memalign((void**)&pTmpReadBuffer, DMA_ADDR_ALIGN, dwMaxReadBufferSize);
				
					if(NULL != pTmpReadBuffer)
					{
						if(NULL !=  datapop->m_pReadBuffer)
						{
							memmove(pTmpReadBuffer, datapop->m_pReadBuffer + datapop->m_dwReadPos, datapop->m_dwReadBufferSize);
							datapop->m_dwReadPos= 0;

							free(datapop->m_pReadBuffer);
							datapop->m_pReadBuffer = NULL;
						}

						datapop->m_pReadBuffer = pTmpReadBuffer;
					}				

					break;
				}
#endif
				if(FrameLen + FRAME_HEAD_LEN > datapop->m_dwReadBufferSize)
				{
					//缓冲不够一帧
					//AVI_DBG("==============datapop->m_dwReadBufferSize:%lld  FrameLen + FRAME_HEAD_LEN:%d  break\n",datapop->m_dwReadBufferSize,FrameLen + FRAME_HEAD_LEN);
					break;
				}
			}
			else
			{
				//如果找不到帧头，则重新定位文件
				//AVI_PrintBuffer(pBuffer, 516);
				//datapop->m_bNeedReLocate = TRUE;
				AVI_DBG("not find frame head, data_len = %lld ==================\n", datapop->m_dwReadBufferSize);
				//AVI_PrintBuffer(pBuffer, 600);
				//sleep_ms(2000);
				break;
			}

			if(NULL !=  pFrameHead)
			{
				BOOL need_drop = FALSE;

				BOOL bIFrame = is_i_frame(pFrameHead->uiFrameType);

				if(bFirstPop == TRUE && !bIFrame)
				{
					need_drop = TRUE;
				}
				else if(0 == (datapop->m_rec_type_mask & dwCurRecType))
				{
					// 录像类型不一致
					need_drop = TRUE;
				}
				else if (((eCurReadMode == READ_ONLY_I) && !bIFrame)
					|| ((eCurReadMode == READ_ONLY_VIDEO) && (!is_video_frame(pFrameHead->uiFrameType) || !is_smart_frame(pFrameHead->uiFrameType)))
					|| ((eCurReadMode == READ_ONLY_AUDIO) && !is_audio_frame(pFrameHead->uiFrameType)))
				{
					// 帧类型不一致
					need_drop = TRUE;
				}
				else if(READ_ONLY_I == eCurReadMode && bIFrame && iCurSpeed >= 3) //快放
				{
					if(dwNeedDropIFrameNum > 0)
					{
						dwNeedDropIFrameNum--;
					}
					if(0 == dwNeedDropIFrameNum)
					{
						need_drop = TRUE;
						dwNeedDropIFrameNum = 6 -iCurSpeed;
					}
				}

				if(pFrameHead->uiFrameTime < (U32)startTime || pFrameHead->uiFrameTime > (U32)endTime)
				{
					need_drop = TRUE;
				}

				//一帧 I8 数据除了开头有I8头, 中间还包含一个I8头..
				if(avis_ret_ok == fast_find_node((unsigned char*)pFrameHead+FRAME_HEAD_LEN, FrameLen, &pos))
				{
					need_drop = TRUE;
				}
				

				//最后一帧不管怎么样都不能丢
				if(bIFrame && (pFrameHead->uiFrameTime == (U32)endTime || pFrameHead->uiFrameTime+1 ==  (U32)endTime  
					|| pFrameHead->uiFrameTime+2 ==  (U32)endTime || pFrameHead->uiFrameTime+3 ==  (U32)endTime || pFrameHead->uiFrameTime+4 ==  (U32)endTime))
				{
					need_drop = FALSE;
				}

				if(need_drop)
				{
					datapop->m_dwReadPos = datapop->m_dwReadPos + FRAME_HEAD_LEN + FrameLen;
					datapop->m_dwReadBufferSize = datapop->m_dwReadBufferSize - FRAME_HEAD_LEN - FrameLen;

					//AVI_DBG("need_drop FRAME_HEAD_LEN:%d FrameLen:%d\n",FRAME_HEAD_LEN,FrameLen);
				}
				else
				{					
					if(NULL != datapop->m_playCallback)
					{
						//AVI_DBG("pFrameHead=%d  m_pReadBuffer=%d m_dwReadPos=%d\n",(int)pFrameHead, (int)datapop->m_pReadBuffer, datapop->m_dwReadPos);
						if((int)pFrameHead & 3)
						{
							memmove(datapop->m_pReadBuffer, pFrameHead, FRAME_HEAD_LEN+FrameLen);
							pFrameHead = (AntsIPCFrameHeader_T *)datapop->m_pReadBuffer;
						}
						nRet = datapop->m_playCallback(datapop->m_hParenHandle, 0, bIFrame, pFrameHead->uiFrameType, pFrameHead->uiFrameTime, (unsigned char*)pFrameHead,FRAME_HEAD_LEN+FrameLen, DATAPOP_EVENT_NONE, datapop->m_pUserData);					
						if(nRet == 0)
						{
							bFirstPop = FALSE;
						}
					}
					
					if(iCurSpeed >= 3) //8倍速以上，弹出一个I帧后，就马上切换文件
					{
						needSwitchFile = TRUE;
					}

					if(is_i_frame(pFrameHead->uiFrameType))
					{
						curFrameTime = pFrameHead->uiFrameTime;
						//GetStrFromTime(curFrameTime, temp);
						//AVI_DBG("pFrameHead->uiFrameTime %s  pFrameHead->uiFrameType=%d\n",temp,pFrameHead->uiFrameType);
					}
					
					datapop->m_dwReadPos = datapop->m_dwReadPos + FRAME_HEAD_LEN + FrameLen;
					datapop->m_dwReadBufferSize = datapop->m_dwReadBufferSize - FRAME_HEAD_LEN - FrameLen;
				}
#ifdef JZT31N
				if (dwMaxReadBufferSize > MAX_READERBUFFERSIZE)
				{
					MemBig2Small = TRUE;
				}
#endif
				//AVI_DBG(" datapop->m_dwReadPos=%lld m_dwReadBufferSize=%lld\n",datapop->m_dwReadPos,datapop->m_dwReadBufferSize);
			}
		}
#ifdef JZT31N
		//当前缓冲区 > 512K, 大内存恢复小内存
		if (TRUE == MemBig2Small)
		{
			dwMaxReadBufferSize = MAX_READERBUFFERSIZE;
			AVI_DBG("realloc smaller memory. dwMaxReadBufferSize=[%u]",dwMaxReadBufferSize);

			posix_memalign((void**)&pTmpReadBuffer, DMA_ADDR_ALIGN, dwMaxReadBufferSize);
					
			if(NULL != pTmpReadBuffer)
			{
				if(NULL !=  datapop->m_pReadBuffer)
				{
					memmove(pTmpReadBuffer, datapop->m_pReadBuffer + datapop->m_dwReadPos, datapop->m_dwReadBufferSize);
					datapop->m_dwReadPos= 0;

					free(datapop->m_pReadBuffer);
					datapop->m_pReadBuffer = NULL;
				}
				
				datapop->m_pReadBuffer = pTmpReadBuffer;
			}
		}
#endif

		if(!datapop->m_bNeedReLocate)
		{
			int sleepTime = 0;
			switch(datapop->m_iRealSpeed)
			{
				case 0:
				case 1:
				case 2:sleepTime = 20;break;
				case 3:sleepTime = 10;break;
				case 4:sleepTime = 10;break;
				default:sleepTime = 10;break;
			}
			sleep_ms(sleepTime);	
		}
	}

	if(bTimeOver == FALSE)
	{
		if(NULL != datapop->m_playCallback)
		{
			AVI_DBG("DATAPOP_EVENT_TIME_OVER\n");
			datapop->m_playCallback(datapop->m_hParenHandle, 0, FALSE, 0, 0, NULL,0, DATAPOP_EVENT_TIME_OVER, datapop->m_pUserData);
			sleep_ms(100);
		}
		bTimeOver = TRUE;
	}

	g_antsAviLibInfo.antsRecIndexManage->ReleaseList(reclist);	
	reclist = NULL;
	head = NULL;

	g_antsAviLibInfo.antsRecIndexManage->ReleaseIFrameList(iFramelist);
	iFramelist = NULL;
	iFrameHead = 0;

	return NULL;
}

CDataPop::CDataPop()
{
	InitParam();
}

CDataPop::~CDataPop()
{
	DataPopRelease();
}

int CDataPop::InitParam()
{
	m_seek_start_time = 0;
	m_seek_end_time = 0;
	m_playCallback = NULL;
	m_pUserData = NULL;

	m_rec_type_mask = 0;
	m_epop_state = epop_state_destroy;
	m_hParenHandle = INVALID_HANDLE_VALUE;
	m_readDir = false;
	m_dyn_pop = true;

	m_threadId = 0;

	m_readfd = NULL;
	m_pReadBuffer = NULL;
	m_dwReadBufferSize = 0;
	m_dwReadPos = 0;
	
	m_pUserData = NULL;

	m_iCurSpeed = 0;
	m_iRealSpeed = 0;
	m_read_mod = READ_ALL_DATA;

	m_CurReadPosInSDCard = 0;
	
	return avis_ret_ok;
}

int CDataPop::DataPopCreate(int nIdx, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime,
	ANTS_AVI_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext, BOOL bNeedSync)
{
	m_seek_start_time = tStartSeekTime;
	m_seek_end_time = tEndSeekTime;
	m_playCallback = cbReplayCallback;
	m_pUserData = pContext;
	m_dyn_pop = (TRUE == bDynPop ? true : false);

	m_hParenHandle = (HANDLE)nIdx;

	char temp1[20], temp2[20];
	GetStrFromTime(m_seek_start_time, temp1);
	GetStrFromTime(m_seek_end_time, temp2);
	AVI_DBG("DataPopCreate time [%s -- %s]\n",temp1, temp2);

	posix_memalign((void**)&m_pReadBuffer, DMA_ADDR_ALIGN, MAX_READERBUFFERSIZE);
	if(NULL == m_pReadBuffer)
	{
		AVI_ERR("error\n\n\n");
		return avis_ret_failed;
	}

	memset(m_pReadBuffer, 0, MAX_READERBUFFERSIZE);
	m_dwReadBufferSize = 0;
	//AVI_DBG("======m_pReadBuffer:%x \n",(int)m_pReadBuffer);

	m_bNeedReLocate = TRUE;

	m_epop_state = epop_state_create;

	int iRet = ANTSAVI_Pthread_Create(&m_threadId, NULL, ReadThread, (void*)this);
    if(iRet!=0)  
    {  
        AVI_ERR("=====Create pthread error!\n");  

		m_bNeedReLocate = FALSE;
		m_epop_state = epop_state_destroy;

		if(NULL !=  m_pReadBuffer)
		{
			free(m_pReadBuffer);
			m_pReadBuffer = NULL;
		}
		
    	return avis_ret_failed;  
    }

	return avis_ret_ok;
}


int CDataPop::DataPopChanAdd(int iChannel)
{
	return avis_ret_ok;	
}


int CDataPop::DataPopChanDel(int iChannel)
{
	return avis_ret_ok;	
}


int CDataPop::DataPopRecTypeAdd(unsigned int iRecType)
{
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}
	m_rec_type_mask |= iRecType;

	m_bNeedReLocate = TRUE;

	AVI_DBG("m_rec_type_mask = 0x%x\n",m_rec_type_mask);
	
	return avis_ret_ok;
}


int CDataPop::DataPopRecTypeDel(unsigned int iRecType)
{
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}
	
	m_rec_type_mask &= ~iRecType;

	m_bNeedReLocate = TRUE;
	
	return avis_ret_ok;	
}


int CDataPop::DataPopTimeSeek(time_t SeekTime)
{
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}

	if(epop_state_stop == m_epop_state)
	{
		// 跳到指定的时间点
		m_seek_start_time = SeekTime;

		char temp[20];
		GetStrFromTime(m_seek_start_time, temp);
		AVI_DBG("m_seek_start_time = %s\n",temp);

		m_bNeedReLocate = TRUE;
	}
	return avis_ret_ok;
}


int CDataPop::DataPopSetDirect(BOOL bSequential)
{
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}
	m_readDir = 0 != bSequential ? true : false;
	return avis_ret_ok;
}


int CDataPop::DataPopSetKeyFrame(BOOL bKeyFrame, int iSpeed)
{
	frame_read_mod_e read_mode = READ_ALL_DATA;
	if(bKeyFrame)
	{
		read_mode = READ_ONLY_I;
	}

	if(m_read_mod != read_mode)
	{
		m_bNeedReLocate = TRUE;
	}
	
	if(iSpeed < 0)
	{
		iSpeed = 0;
	}
	m_iRealSpeed = iSpeed;

	AVI_DBG("\n\n ====== m_iRealSpeed=%d===========\n",m_iRealSpeed);

	if(iSpeed < 3)
		iSpeed = 0;

	m_read_mod = read_mode;
	m_iCurSpeed = iSpeed;

	//AVI_DBG("bKeyFrame=%d iSpeed=%d\n",bKeyFrame,iSpeed);
		
	return avis_ret_ok;	
}


int CDataPop::DataPopGetDataSize(unsigned long long *pSize)
{
	return g_antsAviLibInfo.antsRecIndexManage->GetDataSize(m_seek_start_time, m_seek_end_time, m_rec_type_mask, pSize);
}


int CDataPop::DataPopStart()
{
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}
	
	if(0 == m_rec_type_mask)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	//AVI_DBG("=======DataPopStart\n");

	m_epop_state = epop_state_start;
	
	return avis_ret_ok;	
}


int CDataPop::DataPopStop()
{
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}

	//AVI_DBG("=======DataPopStop\n");

	m_epop_state = epop_state_stop;
	
	return avis_ret_ok;	
}


int CDataPop::DataPopRelease()	
{	
	if(epop_state_destroy == m_epop_state)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_not_create;
	}
	
	if(epop_state_start == m_epop_state)
	{
		DataPopStop();
	}

	m_epop_state = epop_state_destroy;

	if(m_threadId > 0)
	{
		// 停止数据弹出线程
		pthread_join(m_threadId, NULL);
		m_threadId = 0;
	}

	ANTS_AVI_CLOSE(m_readfd);
	DEL_POINTER(m_readfd);

	if(NULL !=  m_pReadBuffer)
	{
		free(m_pReadBuffer);
		m_pReadBuffer = NULL;
	}

	InitParam();
	
	return avis_ret_ok;	
}


