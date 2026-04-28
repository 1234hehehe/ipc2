#include "storageManage.h"
#include "globalObj.h"
#include <syslog.h>
#include <sys/syscall.h>
#include <linux/fs.h>  // 包含 BLKGETSIZE64 的定义

CStorageManage* CStorageManage::thestorageManage = NULL;

extern char g_devPath[128];
char str_SDLogCfg[64] = "/tmp/sd_abnornal.log";
int g_sdcard_errno = 0;

static void *StorageManageThread(void *lpParam)
{
	CStorageManage *pStorageManage = (CStorageManage*)lpParam;
	avis_SDCardHead *ptSDCardHead = pStorageManage->m_ptSDCardHead;

	int iCount = 8;

	AVI_DBG("pid=%ld enter\n", syscall(SYS_gettid));
    syslog(LOG_NOTICE, "[%s:%d] pid=%ld enter\n", __FUNCTION__, __LINE__, syscall(SYS_gettid));

	while(1)
	{
		if((NULL != pStorageManage) && (pStorageManage->m_bSDCardInit) && (0 == g_sdcard_errno))
		{
			g_antsAviLibInfo.antsRecIndexManage->SyncRecIndexNode();

			//录像文件空间小于SD卡大小的100M
			if(ptSDCardHead->tRecFileAddr.filesize >= pStorageManage->m_RecFileMaxSize - 100*MBYTE_UNIT)
			{
				AVI_DBG("remain rec file is less than 100M  tRecFile.filesize=%lld  m_RecFileMaxSize=%lld\n",ptSDCardHead->tRecFileAddr.filesize,pStorageManage->m_RecFileMaxSize);

				if(NULL != g_antsAviLibInfo.antsRecIndexManage)
				{
					while(iCount-- > 0)
					{
						g_antsAviLibInfo.antsRecIndexManage->DeleteOneRecIndexNode();
					}
					iCount = 5;
				}
			}

			//录像索引文件空间不足8个位置
			if(ptSDCardHead->tRecFileIndexAddr.filesize >= pStorageManage->m_RecFileIndexMaxSize - 5*REC_INDEX_NODE_LEN)
			{
				AVI_DBG("rec index filesize=%lld remain is less than 5*64Byte\n",ptSDCardHead->tRecFileIndexAddr.filesize);

				if(NULL != g_antsAviLibInfo.antsRecIndexManage)
				{
					while(iCount-- > 0)
					{
						g_antsAviLibInfo.antsRecIndexManage->DeleteOneRecIndexNode();
					}
					iCount = 5;
				}
			}

			//每4秒更新一次索引
			if(NULL !=  g_antsAviLibInfo.antsWriterManage && g_antsAviLibInfo.antsWriterManage->bIsWritting)
			{
				g_antsAviLibInfo.antsWriterManage->bIsWritting = FALSE;
				g_antsAviLibInfo.antsWriterManage->UpdateRecIndex(FALSE);
			}

			//每4秒更新一次SD卡头
			if(TRUE == pStorageManage->m_bHeadChanged)
			{
				pStorageManage->SaveSDCardHead();
				pStorageManage->m_bHeadChanged = FALSE;
			}

		}

		sleep_ms(4000);
	}

	return NULL;
}


CStorageManage::CStorageManage()
{

}

CStorageManage::~CStorageManage()
{
	m_headMutex.Close();

	if(NULL != m_ptSDCardHead)
	{
		free(m_ptSDCardHead);
	}

	if(NULL != m_fd)
	{
		ANTS_AVI_CLOSE(m_fd);
		DEL_POINTER(m_fd);
	}
}

CStorageManage* CStorageManage::GetMgrItem()
{
	if (NULL == thestorageManage)
	{
		thestorageManage = new CStorageManage();

		if ( NULL == thestorageManage )
		{
			return NULL;
		}

		thestorageManage->InitParam();
	}
	return thestorageManage;
}

void CStorageManage::DelMgrItem()
{
	DEL_POINTER(thestorageManage);
}

int CStorageManage::InitParam()
{
	m_headMutex.Create();

	posix_memalign((void**)&m_ptSDCardHead, DMA_ADDR_ALIGN, sizeof(avis_SDCardHead));
	if(NULL == m_ptSDCardHead)
	{
		AVI_ERR("m_ptSDCardHead error!!!\n\n\n");
		return avis_ret_failed;
	}
	
	memset(m_ptSDCardHead, 0, sizeof(avis_SDCardHead));

	m_fd = new avis_fileHanedle;
	if(NULL == m_fd)
	{
		AVI_ERR("m_ptSDCardHead error!!!\n\n\n");

		if(NULL != m_ptSDCardHead)
		{
			free(m_ptSDCardHead);
		}
		return avis_ret_failed;
	}

	m_SDCardTotalSize = 0;

	m_SDCardHeadMaxSize = 4*KBYTE_UNIT;

	m_RecFileIndexStartAddr = m_SDCardHeadMaxSize;
	m_RecFileIndexMaxSize = 200*MBYTE_UNIT;
	
	m_RecFileStartAddr = m_RecFileIndexStartAddr + m_RecFileIndexMaxSize;

	m_bSDCardInit = FALSE;

	m_bHeadChanged = FALSE;
	
	return avis_ret_ok;
}

int CStorageManage::InitSDCardHead()
{
	if(NULL == m_fd || NULL == m_ptSDCardHead)
	{
		AVI_ERR("m_fd is NULL!\n");
		return avis_ret_failed;
	}

	AVI_DBG("m_SDCardTotalSize=%lld m_SDCardHeadMaxSize=%lld  m_RecFileIndexMaxSize=%lld m_RecFileMaxSize=%lld\n",
		m_SDCardTotalSize,m_SDCardHeadMaxSize,m_RecFileIndexMaxSize,m_RecFileMaxSize);

	m_fd->startAddr = 0;
	m_fd->filesize = m_SDCardTotalSize;

#ifdef ByteAlignmentUseIO
	ANTS_AVI_OPEN(m_fd,io_mode_rw,TRUE);
#else
	ANTS_AVI_OPEN(m_fd,io_mode_rw,FALSE);
#endif

	memset(m_ptSDCardHead, 0, sizeof(avis_SDCardHead));

	ANTS_AVI_SEEK(m_fd, 0, SEEK_SET);
	int byteRead = ANTS_AVI_READ(m_fd, (char *)m_ptSDCardHead, sizeof(avis_SDCardHead));
	AVI_DBG("read sd card head. ret=[%d]\n",byteRead);
	if(byteRead < 0)
	{
		g_sdcard_errno = -3;
		AVI_ERR("g_sdcard_errno=[%d]\n",g_sdcard_errno);

		char acLogBuf[256] = {0};
		sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '666 g_sdcard_errno=[%d]' %s && echo '666 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
		system(acLogBuf);
	}
	else
	{
		if(memcmp(m_ptSDCardHead->startMark, "antsRecStart", strlen("antsRecStart")) == 0 && memcmp(m_ptSDCardHead->endMark, "antsRecEnd", strlen("antsRecEnd")) == 0)
		{
			AVI_ERR("m_bHeadNormal=[%d]\n",g_sdcard_errno);
		}
		else
		{
			S64 nSDCard_total_size = GetPartionSize();
			AVI_DBG("check xubiaoka size=[%lld],SDCard real size=[%lld]\n",nSDCard_total_size,m_SDCardTotalSize);
			if(nSDCard_total_size == m_SDCardTotalSize)
			{
				AVI_ERR("m_bHeadNormal=[%d]\n",g_sdcard_errno);
			}
			else
			{
				g_sdcard_errno = -4;
				AVI_ERR("m_bHeadNormal=[%d]\n",g_sdcard_errno);

				char acLogBuf[256] = {0};
				sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '777 g_sdcard_errno=[%d]' %s && echo '777 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
				system(acLogBuf);
			}
		}
	}
	
	if(g_sdcard_errno == 0)
	{
		sprintf(m_ptSDCardHead->startMark,"antsRecStart");
		sprintf(m_ptSDCardHead->endMark,"antsRecEnd");
	}
	else
	{
		m_ptSDCardHead->tRecFileIndexAddr.startAddr = m_RecFileIndexStartAddr;
		m_ptSDCardHead->tRecFileIndexAddr.filesize = 0;

		m_ptSDCardHead->tRecFileAddr.startAddr = m_RecFileStartAddr;
		m_ptSDCardHead->tRecFileAddr.filesize = 0;
	}
	
	m_ptSDCardHead->llSDCardSize = m_SDCardTotalSize;
	m_ptSDCardHead->llheadSize = m_SDCardHeadMaxSize;
	m_ptSDCardHead->llRecIndexSize = m_RecFileIndexMaxSize;
	m_ptSDCardHead->llRecFileSize = m_RecFileMaxSize;
	
	if(m_ptSDCardHead->tRecFileIndexAddr.startAddr < m_RecFileIndexStartAddr || m_ptSDCardHead->tRecFileIndexAddr.startAddr > m_RecFileIndexStartAddr+m_RecFileIndexMaxSize
		|| m_ptSDCardHead->tRecFileIndexAddr.filesize < 0 || m_ptSDCardHead->tRecFileIndexAddr.filesize > m_RecFileIndexMaxSize)
	{
		AVI_DBG("===========tRecFileIndexAddr head is bad!!!!!!\n\n");
		m_ptSDCardHead->tRecFileIndexAddr.startAddr = m_RecFileIndexStartAddr;
		m_ptSDCardHead->tRecFileIndexAddr.filesize = m_RecFileIndexMaxSize;
	}

	if(m_ptSDCardHead->tRecFileAddr.startAddr < m_RecFileStartAddr || m_ptSDCardHead->tRecFileAddr.startAddr > m_RecFileStartAddr+m_RecFileMaxSize
		|| m_ptSDCardHead->tRecFileAddr.filesize < 0 || m_ptSDCardHead->tRecFileAddr.filesize > m_RecFileMaxSize)
	{
		AVI_DBG("===========tRecFileAddr head is bad!!!!!!\n\n");
		m_ptSDCardHead->tRecFileAddr.startAddr = m_RecFileStartAddr;
		m_ptSDCardHead->tRecFileAddr.filesize = 0;
	}

	AVI_DBG("m_ptSDCardHead->startMark=%s  m_ptSDCardHead->endMark =%s\n"
			  "m_ptSDCardHead->tRecFileIndexAddr=[%lld - %lld]\n"
			  "m_ptSDCardHead->tRecFileAddr=[%lld - %lld]\n"
			  "m_SDCardTotalSize=%lld  m_SDCardHeadMaxSize=%lld  m_RecFileIndexMaxSize=%lld m_RecFileMaxSize=%lld  byteRead=%d\n",
				m_ptSDCardHead->startMark, m_ptSDCardHead->endMark,
				m_ptSDCardHead->tRecFileIndexAddr.startAddr, m_ptSDCardHead->tRecFileIndexAddr.filesize,
				m_ptSDCardHead->tRecFileAddr.startAddr, m_ptSDCardHead->tRecFileAddr.filesize,
				m_SDCardTotalSize, m_SDCardHeadMaxSize, m_RecFileIndexMaxSize, m_RecFileMaxSize,byteRead);

	return avis_ret_ok;
}

int CStorageManage::GetSDCardHead(avis_SDCardHead *pSDCardhead)
{
	if(NULL == pSDCardhead)
	{
		return avis_ret_failed;
	}

	if(!m_bSDCardInit)
	{
		return avis_ret_failed;
	}

	memcpy(pSDCardhead, m_ptSDCardHead, sizeof(avis_SDCardHead));

/*	AVI_DBG("m_ptSDCardHead->startMark=%s  m_ptSDCardHead->endMark =%s\n"
				"m_ptSDCardHead->tRecFileIndexAddr=[%lld - %lld]\n"
				"m_ptSDCardHead->tRecFileAddr=[%lld - %lld]  \n",
				m_ptSDCardHead->startMark, m_ptSDCardHead->endMark,
				m_ptSDCardHead->tRecFileIndexAddr.startAddr,m_ptSDCardHead->tRecFileIndexAddr.filesize,
				m_ptSDCardHead->tRecFileAddr.startAddr,m_ptSDCardHead->tRecFileAddr.filesize);
*/
	return avis_ret_ok;
}

int CStorageManage::SetSDCardHead(avis_SDCardHead *pSDCardhead)
{
	if(NULL == pSDCardhead)
	{
		return avis_ret_failed;
	}

	if(!m_bSDCardInit)
	{
		return avis_ret_failed;
	}

	if(pSDCardhead->tRecFileIndexAddr.startAddr < m_RecFileIndexStartAddr || pSDCardhead->tRecFileIndexAddr.startAddr > m_RecFileIndexStartAddr+m_RecFileIndexMaxSize
		|| pSDCardhead->tRecFileIndexAddr.filesize < 0 || pSDCardhead->tRecFileIndexAddr.filesize > m_RecFileIndexMaxSize)
	{
		AVI_ERR("pSDCardhead->tRecFileIndexAddr.startAddr=%lld  m_RecFileIndexStartAddr=%lld pSDCardhead->tRecFileIndexAddr.filesize=%lld m_RecFileIndexMaxSize=%lld\n",
			pSDCardhead->tRecFileIndexAddr.startAddr,m_RecFileIndexStartAddr,pSDCardhead->tRecFileIndexAddr.filesize,m_RecFileIndexMaxSize);

		AVI_DBG("m_ptSDCardHead->tRecFileIndexAddr.startAddr=%lld filesize=%lld\n",m_ptSDCardHead->tRecFileIndexAddr.startAddr, m_ptSDCardHead->tRecFileIndexAddr.filesize);
		return avis_ret_failed;
	}

	if(pSDCardhead->tRecFileAddr.startAddr < m_RecFileStartAddr || pSDCardhead->tRecFileAddr.startAddr > m_RecFileStartAddr+m_RecFileMaxSize
		|| pSDCardhead->tRecFileAddr.filesize < 0 || pSDCardhead->tRecFileAddr.filesize > m_RecFileMaxSize)
	{
		AVI_ERR("pSDCardhead->tRecFileAddr.startAddr=%lld  m_RecFileStartAddr=%lld pSDCardhead->tRecFileAddr.filesize=%lld m_RecFileMaxSize=%lld\n",
			pSDCardhead->tRecFileAddr.startAddr,m_RecFileStartAddr,pSDCardhead->tRecFileAddr.filesize,m_RecFileMaxSize);

		AVI_DBG("m_ptSDCardHead->tRecFileAddr.startAddr=%lld filesize=%lld\n",m_ptSDCardHead->tRecFileAddr.startAddr, m_ptSDCardHead->tRecFileAddr.filesize);
		return avis_ret_failed;
	}

	/*
	//录像检索块地址和大小
	m_ptSDCardHead->tRecFileIndexAddr.startAddr = pSDCardhead->tRecFileIndexAddr.startAddr;
	m_ptSDCardHead->tRecFileIndexAddr.filesize 	= pSDCardhead->tRecFileIndexAddr.filesize;

	//录像文件地址和大小
	m_ptSDCardHead->tRecFileAddr.startAddr 	= pSDCardhead->tRecFileAddr.startAddr;
	m_ptSDCardHead->tRecFileAddr.filesize 	= pSDCardhead->tRecFileAddr.filesize;
	*/
	memcpy(m_ptSDCardHead, pSDCardhead, sizeof(avis_SDCardHead));
	
	m_bHeadChanged = TRUE;

#if 0
	AVI_DBG("m_ptSDCardHead->startMark=%s  m_ptSDCardHead->endMark =%s\n"
			  "m_ptSDCardHead->tRecFileIndexAddr=[%lld - %lld]\n"
			  "m_ptSDCardHead->tRecFileAddr=[%lld - %lld]\n"
			  "m_SDCardTotalSize=%lld  m_SDCardHeadMaxSize=%lld  m_RecFileIndexMaxSize=%lld m_RecFileMaxSize=%lld \n",
				m_ptSDCardHead->startMark, m_ptSDCardHead->endMark,
				m_ptSDCardHead->tRecFileIndexAddr.startAddr, m_ptSDCardHead->tRecFileIndexAddr.filesize,
				m_ptSDCardHead->tRecFileAddr.startAddr, m_ptSDCardHead->tRecFileAddr.filesize,
				m_SDCardTotalSize, m_SDCardHeadMaxSize, m_RecFileIndexMaxSize, m_RecFileMaxSize);
#endif

	return avis_ret_ok;
}

int CStorageManage::SaveSDCardHead()
{
	//AVI_DBG("==============SaveSDCardHead\n");

	if (!IsFileOen(m_fd))
	{
		AVI_ERR("file not Open\n");
		return avis_ret_failed;
	}

	m_headMutex.Wait();

	ANTS_AVI_SEEK(m_fd, 0, SEEK_SET);
	int byteWritern = ANTS_AVI_WRITE(m_fd, m_ptSDCardHead, sizeof(avis_SDCardHead));
	
	if(byteWritern != sizeof(avis_SDCardHead))
	{
		AVI_ERR("write error!!!\n");
	}

	m_headMutex.Release();

#if 0
	AVI_DBG("m_ptSDCardHead->startMark=%s  m_ptSDCardHead->endMark =%s\n"
			  "m_ptSDCardHead->tRecFileIndexAddr=[%lld - %lld]\n"
			  "m_ptSDCardHead->tRecFileAddr=[%lld - %lld]\n"
			  "m_SDCardTotalSize=%lld  m_SDCardHeadMaxSize=%lld   m_RecFileIndexMaxSize=%lld m_RecFileMaxSize=%lld  byteWritern=%d\n",
				m_ptSDCardHead->startMark, m_ptSDCardHead->endMark,
				m_ptSDCardHead->tRecFileIndexAddr.startAddr, m_ptSDCardHead->tRecFileIndexAddr.filesize,
				m_ptSDCardHead->tRecFileAddr.startAddr, m_ptSDCardHead->tRecFileAddr.filesize,
				m_SDCardTotalSize, m_SDCardHeadMaxSize, m_RecFileIndexMaxSize, m_RecFileMaxSize,byteWritern);
#endif

	return avis_ret_ok;
}

int CStorageManage::GetDiskStatus(char *pPath, unsigned int *lpdwAllSpace, unsigned int *lpdwFreeSpace)
{
	if(NULL == pPath || NULL == lpdwAllSpace || NULL == lpdwFreeSpace)
	{
		return avis_ret_failed;
	}

	m_headMutex.Wait();

	if(strncmp(pPath, g_devPath, sizeof(g_devPath)) == 0)
	{
		*lpdwAllSpace = m_SDCardTotalSize/1000/1000;
		*lpdwFreeSpace = (m_RecFileMaxSize - m_ptSDCardHead->tRecFileAddr.filesize + m_RecFileIndexMaxSize - m_ptSDCardHead->tRecFileIndexAddr.filesize)/1000/1000;
		m_headMutex.Release();
		return avis_ret_ok;
	}

	m_headMutex.Release();
	return avis_ret_failed;
}

int CStorageManage::FormatDisk(char *pPath)
{
	if(NULL == pPath)
	{
		return avis_ret_failed;
	}

	if(strncmp(pPath, g_devPath, sizeof(g_devPath)) == 0)
	{
		g_antsAviLibInfo.antsRecIndexManage->FormatDisk();
	}

	return avis_ret_ok;
}

int CStorageManage::AddRecPartition(PartitionInfo *pInfo)
{
	AVI_DBG("path:%s pInfo->nPartitionTotalSpace=%llu\n",pInfo->acPath, pInfo->nPartitionTotalSpace);

	if(NULL == pInfo)
	{
		return avis_ret_failed;
	}

	m_SDCardTotalSize = pInfo->nPartitionTotalSpace;
	//m_SDCardTotalSize = 500*MBYTE_UNIT;
	if(m_SDCardTotalSize < (S64)4 * (S64)GBYTE_UNIT || m_SDCardTotalSize > (S64)512 * (S64)GBYTE_UNIT)
	{
		g_sdcard_errno = -6;
		AVI_ERR("g_sdcard_errno=[%d]\n",g_sdcard_errno);
		
		char acLogBuf[256] = {0};
		sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '999 g_sdcard_errno=[%d]' %s && echo '999 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
		system(acLogBuf);
	}

	memset(g_devPath, 0, sizeof(g_devPath));
	snprintf(g_devPath, sizeof(g_devPath), "%s", pInfo->acPath);
	//snprintf(g_devPath, sizeof(g_devPath), "%s", "/mnt/write.txt");

	struct statfs tfs;
	if(!statfs(g_devPath, &tfs))
	{
		AVI_DBG("type = %x sid=%x-%x blocks = %lu blocksize= %d free = %lu avail=%lu,files=%lu,freenode=%lu filenamelen = %d\n",
			tfs.f_type,tfs.f_fsid.__val[0],tfs.f_fsid.__val[1],tfs.f_blocks , tfs.f_bsize,tfs.f_bfree,tfs.f_bavail,tfs.f_files,tfs.f_ffree,tfs.f_namelen);

		m_SDCardHeadMaxSize = tfs.f_bsize;
	}
	else
	{
		AVI_ERR("statfs g_devPath=%s error!!\n",g_devPath);
		return avis_ret_failed;
	}

	//1G中用5M来存索引
	int i = m_SDCardTotalSize/GBYTE_UNIT;
	if(i<=0)
		i=1;

	m_RecFileIndexStartAddr = m_SDCardHeadMaxSize;
	m_RecFileIndexMaxSize = (S64)i*5*MBYTE_UNIT;

	m_RecFileStartAddr = m_RecFileIndexStartAddr + m_RecFileIndexMaxSize;
	m_RecFileMaxSize = m_SDCardTotalSize - m_RecFileStartAddr;

	InitSDCardHead();

	m_bSDCardInit = TRUE;

	int iRet = g_antsAviLibInfo.antsRecIndexManage->RecoverFromFile();
	if(iRet!=0)
    {
        AVI_ERR("=====RecoverFromFile error!\n");
    	return avis_ret_failed;
    }

	iRet = ANTSAVI_Pthread_Create(&m_threadId, NULL, StorageManageThread, (void*)this);
    if(iRet!=0)
    {
        AVI_ERR("=====Create pthread error!\n");
    	return avis_ret_failed;
    }

	return avis_ret_ok;
}

S64 CStorageManage::GetPartionSize()
{
	int nRet = 0;
	S64 nSDCard_wright_size = 0;
	S64 nSDCard_total_size = 0;
	S64 nSDCard_seek_size = 0;
	
	int buffer_size = 512;
	int compare_size = 512;
	unsigned char* read_buffer = NULL;
	unsigned char* write_buffer = NULL;
	
	posix_memalign((void**)&read_buffer, 512, buffer_size);
	posix_memalign((void**)&write_buffer, 512, buffer_size);
	if(read_buffer == NULL || write_buffer == NULL)
	{
		printf("posix_memalign read_buffer or write_buffer failed\n");
		if(read_buffer)
			free(read_buffer);
		if(write_buffer)
			free(write_buffer);
		return -1;
	}
	
	for(int i = 0; i < buffer_size; i++)
	{
		write_buffer[i] = (unsigned char)i % 128;
	}
	
	int oflag = O_RDWR | O_CREAT | O_DIRECT | O_LARGEFILE;
	int fd = open(g_devPath, oflag, S_IRUSR | S_IWUSR);
	if(fd < 0)
	{
		printf("open file[%s] failed.%d,%s\n",g_devPath,errno,strerror(errno));
		if(read_buffer)
			free(read_buffer);
		if(write_buffer)
			free(write_buffer);
		return -1;
	}
	//printf("[%s] open succ. fd=[%d]\n",pPartionPath,fd);
	
	// 获取设备总大小
    ioctl(fd, BLKGETSIZE64, &nSDCard_total_size);
	//printf("[%s] get size succ. SDCardSize=[%lld]\n",pPartionPath,nSDCard_total_size);
	
	for(int i = 1; i <= nSDCard_total_size/GBYTE_UNIT + 1; i++)
	{
		if(i < nSDCard_total_size/GBYTE_UNIT + 1)
		{
			nSDCard_seek_size = (long long)i * GBYTE_UNIT - buffer_size;
			compare_size = buffer_size;
		}
		else
		{
			if((long long)(i - 1) * GBYTE_UNIT + buffer_size > nSDCard_total_size)
			{
				nSDCard_seek_size = (long long)(i - 1) * GBYTE_UNIT;
				compare_size = nSDCard_total_size - (long long)(i - 1) * GBYTE_UNIT;
			}
			else if((long long)(i - 1) * GBYTE_UNIT + buffer_size <= nSDCard_total_size)
			{
				nSDCard_seek_size = nSDCard_total_size/buffer_size*buffer_size - buffer_size;	//512BYTE对齐
				compare_size = buffer_size;
			}
		}
		
		lseek64(fd, nSDCard_seek_size, SEEK_SET);
		
		nRet = write(fd, write_buffer, buffer_size);
		if(nRet <= 0)
		{
			printf("write. nRet=[%d].%d,%s\n",nRet,errno,strerror(errno));
			break;
		}
		
		fsync(fd);
		
		lseek64(fd, nSDCard_seek_size, SEEK_SET);
		
		memset(read_buffer, 0, buffer_size);
		nRet = read(fd, read_buffer, buffer_size);
		if(nRet <= 0)
		{
			printf("read. nRet=[%d].%d,%s\n",nRet,errno,strerror(errno));
			break;
		}
		
		nRet = memcmp(write_buffer, read_buffer, compare_size);
		if(nRet == 0)
		{
			if(i < nSDCard_total_size/GBYTE_UNIT + 1)
			{
				nSDCard_wright_size += GBYTE_UNIT;
			}
			else
			{
				nSDCard_wright_size += nSDCard_total_size - (long long)(i - 1) * GBYTE_UNIT;
			}
		}
		else
		{
			break;
		}
	}
	
	close(fd);
	
	if(read_buffer)
		free(read_buffer);
	if(write_buffer)
		free(write_buffer);
	
	return nSDCard_wright_size;
}


