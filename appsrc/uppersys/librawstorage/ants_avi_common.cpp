#include "ants_avi_common.h"
#include "globalObj.h"

char g_devPath[128]; 

extern AntsAviLibInfo g_antsAviLibInfo;
extern char str_SDLogCfg[64];
extern int g_sdcard_errno;

int ANTSAVI_Pthread_Create(pthread_t * threadid, pthread_attr_t * attr, void*(*routine)(void*), void* param)
{
    int ret = 0;
    pthread_t id = 0;
    pthread_attr_t defattr;
    if (attr == NULL)
    {
        attr = &defattr;
        pthread_attr_init(attr);
    }
    
    pthread_attr_setstacksize(attr, PTHREAD_STACK_MIN * 64);
    
	ret = pthread_create(&id, attr, routine, param);

    pthread_attr_destroy(attr);

    if (threadid)
    {
        *threadid = id;
    }
    
    return ret;
}

S64 safe_write(int fd, void *buf, S64 count)
{
	if(access("/dev/mmcblk0p2",F_OK) != 0)
	{
		exit(0);
	}

    S64 n;
	char *p=(char*)buf;
	S64 iWrite = count;
	S64 bytesWriteen = 0;
	static int nIOErrorCount = 0;
	
    do
    {
        n = write(fd, p, iWrite);
		if (n < 0)
		{
			AVI_DBG("==============fd:%d bytesWriten:%lld errno:%d strerror:%s  buf:%x count=%lld, nIOErrorCount=%d\n",fd,n,errno,strerror(errno),*(int*)buf, count, nIOErrorCount);	
			if(errno==EINTR)
			{
				nIOErrorCount = 0;
				usleep(1000);
            	continue;
			}
			else if(errno==5)
			{
				nIOErrorCount++;
				if(nIOErrorCount >= 100)
				{
					g_sdcard_errno = -3;
					AVI_ERR("g_sdcard_errno=[%d]\n",g_sdcard_errno);
					
					char acLogBuf[256] = {0};
					sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '111 g_sdcard_errno=[%d]' %s && echo '111 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
					system(acLogBuf);
				}
				break;
			}
			else
			{
				nIOErrorCount = 0;
				break;
				//exit(0);
			}
		}
		else if(n!=iWrite)
		{
			nIOErrorCount = 0;
			AVI_DBG("==============fd:%d bytesWriten:%lld iWrite=%lld count=%lld\n",fd,n, iWrite,count);
			iWrite = count-n;
			bytesWriteen +=n;

			p = (char*)buf+bytesWriteen;
			sleep_ms(1);
		}
        else
        {
        	nIOErrorCount = 0;
			bytesWriteen +=n;
            break;
        }
    } while (1);

    return bytesWriteen;
}

S64 safe_read(int fd, void *buf, S64 count)
{
	if(access("/dev/mmcblk0p2",F_OK) != 0)
	{
		exit(0);
	}

    S64 n;
	S64 iRead = count;
	char *p=(char*)buf;
	S64 bytesRead = 0;
	static int nIOErrorCount = 0;
	
    do
    {
        n = read(fd, p, iRead);
        if (n < 0)
        {
			printf("==============fd:%d bytesRead:%lld errno:%d strerror:%s  buf:%x,nIOErrorCount=%d\n",fd,n,errno,strerror(errno),*(int*)buf,nIOErrorCount);
			if(errno == EINTR)
			{
				nIOErrorCount = 0;
            	usleep(1000);
            	continue;
			}
			else if(errno==5)
			{
				nIOErrorCount++;
				if(nIOErrorCount >= 100)
				{
					g_sdcard_errno = -3;
					AVI_ERR("g_sdcard_errno=[%d]\n",g_sdcard_errno);
					
					char acLogBuf[256] = {0};
					sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '222 g_sdcard_errno=[%d]' %s && echo '222 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
					system(acLogBuf);
				}
				break;
			}
			else
			{
				nIOErrorCount = 0;
				break;
				//exit(0);
			}
        }
/*		else if(n != iRead)
		{
			printf("==============fd:%d bytesRead:%lld  errno:%d strerror:%s iRead=%lld count=%lld\n",fd,n, errno,strerror(errno), iRead,count);
			iRead -= n;
			bytesRead += n;
			
			p = (char *)buf + bytesRead;
			
			sleep_ms(1);
		}*/
        else
        {
        	nIOErrorCount = 0;
			bytesRead += n;
            break;
        }
    } while (1);

    return bytesRead;
}

S64 ANTS_AVI_SEEK(avis_fileHanedle *fd, S64 offset, int where)
{
	if(NULL == fd || fd->fd <= 0)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	
	if(where == SEEK_SET)
	{
		if(offset < 0 || offset > fd->filesize)
			return avis_ret_failed;
		
		return lseek64(fd->fd, offset+fd->startAddr, SEEK_SET) - fd->startAddr;
	}
	else if(where == SEEK_CUR)
	{
		return  lseek64(fd->fd, offset, SEEK_CUR) - fd->startAddr;
	}
	else if(where == SEEK_END)
	{
		if(offset > 0)
		{
			OUTPUT_FUNC_LINE;
			return avis_ret_failed;
		}
		
		return lseek64(fd->fd, fd->startAddr + fd->filesize + offset, SEEK_SET) - fd->startAddr;
	}

	return avis_ret_failed;
}

int ANTS_AVI_OPEN(avis_fileHanedle *fd, file_io_mode io_mode, BOOL io_direct) 
{
	if(NULL == fd)
	{
		AVI_ERR("fd is NULL!!!!!\n");
		return avis_ret_failed;
	}

	if(g_devPath[0] == '\0')
	{
		AVI_ERR("g_devPath is NULL!!!!!\n");
		return avis_ret_failed;
	}

	if(fd->fd > 0)
	{
		ANTS_AVI_CLOSE(fd);
	}
	
	int oflag = 0;
	if(io_mode_r == io_mode)
	{
		oflag = O_RDONLY;
	}
	else if(io_mode_w == io_mode)
	{
		oflag = O_WRONLY | O_CREAT;
	}
	else
	{
		oflag = O_RDWR | O_CREAT;
	}
	if(io_direct)
	{
		oflag |= O_DIRECT;
	}

	oflag |= O_LARGEFILE;
	
	fd->fd = open(g_devPath, oflag, S_IRUSR | S_IWUSR);
	if(fd->startAddr >= 0)
	{
		ANTS_AVI_SEEK(fd, 0, SEEK_END);
	}
	
	if(fd->fd <= 0)
	{
		AVI_ERR("open fd->fd: %d startAddr: %lld failed, io_mode = %d\n", fd->fd, fd->startAddr,io_mode);
		return avis_ret_failed;
	}
	else
	{
		AVI_DBG("==============openfile startAddr:%lld  fd:%d  io_mode = %d  io_direct:%d success!!!!!\n", fd->startAddr ,fd->fd, io_mode, io_direct);
	}

	fd->m_io_mode = io_mode;
	
	return avis_ret_ok;
}

S64 ANTS_AVI_WRITE(avis_fileHanedle *fd, void *buff, S64 count)
{
	S64 byteswriten=0;
	S64 pos = 0;
	if(NULL == fd || fd->fd <= 0 || fd->m_io_mode == io_mode_r)
	{
		AVI_ERR("====\n");
		return avis_ret_failed;
	}
	
	pos = ANTS_AVI_SEEK(fd, 0, SEEK_CUR);
	
	byteswriten = safe_write(fd->fd, buff, count);

	if(byteswriten != count)// || count != PER_WRITESIZE)
	{
		AVI_DBG("=======write startAddr:%lld filesize:%lld  fd:%d  byteswriten:%lld  count:%lld\n", fd->startAddr, fd->filesize, fd->fd, byteswriten, count);
	}
	//AVI_DBG("pos:%lld  filesize:%lld\n\n",pos, fd->filesize);
	if(pos == fd->filesize)
	{
		fd->filesize += count;
	}
	else
	{
		//AVI_DBG("here\n");
	}
		
	return byteswriten;
}


S64 ANTS_AVI_READ(avis_fileHanedle *fd, void *buff, S64 count)
{
	S64 bytesread=0;		

	if(NULL == fd || fd->fd <=0 || fd->m_io_mode == io_mode_w)
		return avis_ret_failed;
	
	bytesread = safe_read(fd->fd, buff, count);
	return bytesread;
}

int ANTS_AVI_CLOSE(avis_fileHanedle *fd)
{
	if(NULL == fd || fd->fd <=0)
		return avis_ret_failed;
	
	if(fd->fd > 0)
	{
		close(fd->fd);
		fd->fd = -1;
	}
	return avis_ret_ok;
}

BOOL is_i_frame(int frame_type)
{
	if(AntsIPCFrameType_IFrames == frame_type || AntsIPCFrameType_SubIFrames == frame_type || AntsIPCFrameType_ThirdIFrames == frame_type)
	{
		return true;
	}
	return FALSE;
}

BOOL is_video_frame(int frame_type)
{
	if(AntsIPCFrameType_IFrames == frame_type || AntsIPCFrameType_PFrames == frame_type || AntsIPCFrameType_BBPFrames == frame_type || AntsIPCFrameType_BBIFrames == frame_type
		|| AntsIPCFrameType_BPFrames == frame_type || AntsIPCFrameType_SFrames == frame_type || AntsIPCFrameType_SubIFrames == frame_type
		|| AntsIPCFrameType_SubPFrames == frame_type || AntsIPCFrameType_SubBBPFrames == frame_type || AntsIPCFrameType_ThirdIFrames == frame_type
		|| AntsIPCFrameType_ThirdPFrames == frame_type || AntsIPCFrameType_ThirdBBPFrames == frame_type)
	{
		return true;
	}
	return FALSE;
}

BOOL is_audio_frame(int frame_type)
{
	if(AntsIPCFrameType_AudioFrames == frame_type)
	{
		return true;
	}
	return FALSE;
}

BOOL is_smart_frame(int frame_type)
{
	if(Antsmid_FrameType_SmartIFrames == frame_type || Antsmid_FrameType_SmartPFrames == frame_type)
	{
		return true;
	}
	return FALSE;
}


void sleep_ms(int ms)
{
	struct timeval timeout; 
	timeout.tv_sec = ms / 1000; 
	timeout.tv_usec = (ms % 1000) * 1000;
	select(0, NULL, NULL, NULL, &timeout); 
}

int trans_to_rectype_mask(int rectype)
{
	int recmask = AVIS_RECORDTYPE_TIMER;
	if(rectype > ANTS_RECORDTYPE_BEGIN && rectype < ANTS_RECORDTYPE_END)
	{
		if(ANTS_RECORDTYPE_ALL == rectype)
		{
			recmask = AVIS_RECORDTYPE_ALL;
		}
		else
		{
			recmask = (1 << (rectype - 1));
		}
	}
	return recmask;
}

int trans_from_rectype_mask(int recmask)
{
	int rectype = ANTS_RECORDTYPE_TIMER;
	if(0 == (recmask & (~AVIS_RECORDTYPE_ALL)))
	{
		for(int i = 0; i < 7; i++)
		{
			if(recmask & (1 << i))
			{
				rectype = (i + 1);
				break;
			}
		}

		if(recmask & (1 << (ANTS_RECORDTYPE_IVSDETECT-1)))
		{
			rectype = ANTS_RECORDTYPE_IVSDETECT;
		}
		else if(recmask & (1 << (ANTS_RECORDTYPE_FACE_DETECT-1)))
		{
			rectype = ANTS_RECORDTYPE_FACE_DETECT;
		}
		else if(recmask & (1 << (ANTS_RECORDTYPE_FIRE_DETECT-1)))
		{
			rectype = ANTS_RECORDTYPE_FIRE_DETECT;
		}
		else if(recmask & (1 << (ANTS_RECORDTYPE_VIDEODIAGNOSE-1)))
		{
			rectype = ANTS_RECORDTYPE_VIDEODIAGNOSE;
		}
	}
	return rectype;
}

BOOL IsFileOen(avis_fileHanedle *fd)
{
	if(NULL !=  fd && fd->fd > 0)
		return true;

	return FALSE;
}

int mktime_utc(avis_time_info *pTimeInfo, time_t *dwTime)
{
	if(NULL == pTimeInfo)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}

	struct tm tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	tmTime.tm_year = pTimeInfo->wYear - 1900;
	tmTime.tm_mon = pTimeInfo->wMonth - 1;
	tmTime.tm_mday = pTimeInfo->wDay;
	tmTime.tm_hour = pTimeInfo->wHour;
	tmTime.tm_min = pTimeInfo->wMinute;
	tmTime.tm_sec = pTimeInfo->wSecond;
	tmTime.tm_isdst = -1;

	time_t result = mktime(&tmTime);
	if((time_t)-1 == result)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	*dwTime = result;
	
	return avis_ret_ok;
}

int gmtime_utc(time_t dwTime, avis_time_info *pTimeInfo)
{
	if(NULL == pTimeInfo)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(dwTime < 0)
	{
		AVI_WAR("force dwTime to zero, dwTime = %d\n", (int)dwTime);
		dwTime = 0;
	}

	tm *ptmTime = gmtime((time_t *)&dwTime);
	if(NULL == ptmTime)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	pTimeInfo->wYear = ptmTime->tm_year + 1900;
	pTimeInfo->wMonth = ptmTime->tm_mon + 1;
	pTimeInfo->wDay = ptmTime->tm_mday;
	pTimeInfo->wHour = ptmTime->tm_hour;
	pTimeInfo->wMinute = ptmTime->tm_min;
	pTimeInfo->wSecond = ptmTime->tm_sec;
	return avis_ret_ok;
}

int localtime_utc(time_t dwTime, avis_time_info *pTimeInfo)
{
	if(NULL == pTimeInfo)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(dwTime < 0)
	{
		AVI_WAR("force dwTime to zero, dwTime = %d\n", (int)dwTime);
		dwTime = 0;
	}

	struct tm tmTime;
	localtime_r((time_t *)&dwTime, &tmTime);
	pTimeInfo->wYear = tmTime.tm_year + 1900;
	pTimeInfo->wMonth = tmTime.tm_mon + 1;
	pTimeInfo->wDay = tmTime.tm_mday;
	pTimeInfo->wHour = tmTime.tm_hour;
	pTimeInfo->wMinute = tmTime.tm_min;
	pTimeInfo->wSecond = tmTime.tm_sec;
	return avis_ret_ok;
}

//比较年月日
BOOL compareYMD(time_t time1, time_t time2)
{
	avis_time_info info1,info2;
//	gmtime_utc(time1, &info1);
//	gmtime_utc(time2, &info2);
	localtime_utc(time1, &info1);
	localtime_utc(time2, &info2);

	if(info1.wYear==info2.wYear && info1.wMonth==info2.wMonth && info1.wDay==info2.wDay)
	{
		return TRUE;
	}

	return FALSE;
}

//比较年月日
int compareYMD_Ex(time_t time1, time_t time2)
{
	S32 t1 = 0,t2 = 0;
	avis_time_info info1,info2;
//	gmtime_utc(time1, &info1);
//	gmtime_utc(time2, &info2);
	localtime_utc(time1, &info1);
	localtime_utc(time2, &info2);
	t1 = info1.wYear*10000+info1.wMonth*100+info1.wDay;
	t2 = info2.wYear*10000+info2.wMonth*100+info2.wDay;

	if(t1 > t2)
		return 1;
	else if(t1 == t2)
		return 0;
	else
		return -1;
}


BOOL GetStrFromTime(time_t iTimeStamp, char *pszTime) 
{ 
   struct tm *pTmp = localtime(&iTimeStamp); 
   if (NULL == pTmp) 
   { 
      return FALSE; 
   } 
   sprintf(pszTime, "%4d-%02d-%02d %02d:%02d:%02d", pTmp->tm_year + 1900, pTmp->tm_mon + 1, pTmp->tm_mday, pTmp->tm_hour, pTmp->tm_min, pTmp->tm_sec); 
   return TRUE; 
} 


//指定YYYYMMDDHH24MISS型的时间，格式化为time_t型的时间
time_t FormatTime2(char * szTime)
{
    struct tm tm1;
    time_t time1;

    sscanf(szTime, "%4d%2d%2d%2d%2d%2d",     
                 &tm1.tm_year, 
                 &tm1.tm_mon, 
                 &tm1.tm_mday, 
                 &tm1.tm_hour, 
                 &tm1.tm_min,
                 &tm1.tm_sec);
           
    tm1.tm_year -= 1900;
      tm1.tm_mon --;

    tm1.tm_isdst=-1;

    time1 = mktime(&tm1);
    return time1;
}


unsigned int get_cur_time_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


void AVI_PrintBuffer(void *buffer, int len)
{
    const unsigned char* p = (const unsigned char*)buffer;
    int i, j;
    
    printf("buffer=%p len=0x%x\n", buffer, len);
    for (i = 0; i < len; i+= 16)
    {
        printf("0x%04x:", i);
        for (j = i; j < (i + 16) && j < len; j++)
        {
            printf(" %02x", p[j]);
        }
        if (j < i + 16)
        {
            for ( ; j < i + 16; j++)
            {
                printf("   ");
            }
        }
        
        printf(" | ");
        for (j = i; j < (i + 16) && j < len; j++)
        {
            printf("%c", (p[j] >= 0x20 && p[j] < 0x80) ? p[j] : '.');
        }
        printf("\n");
    }
    printf("\n");
}
