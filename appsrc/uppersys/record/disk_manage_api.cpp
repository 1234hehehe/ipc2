#include <unistd.h>
#include <sys/syscall.h>
#include <sys/statfs.h> 
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include "disk_manage_api.h"
#include "ipcstorage.h"

using namespace cv_soft;

#define MAX_DISKNUM_EX	4

#define EXT4_SUPER_MAGIC      0xEF53

#define BLKGETSIZE _IO(0x12,96)
#define BLKSSZGET _IO(0x12, 104)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)
#define HDIO_GETGEO     0x0301  /* get device geometry */
struct hd_geometry {
	unsigned char heads;
	unsigned char sectors;
	unsigned short cylinders;
	unsigned long start;
};

static int g_nSDCount = 0;
static PartitionInfo g_tPartInfo[MAX_DISKNUM_EX];
static int g_bDiskFull = 0;
static fAlarmUploadCallback	EventUpload = NULL;
extern char str_SDLogCfg[64];
extern int g_sdcard_errno;

static S32 record_diskcheck_thread(Common_Thread_T pThHandle, void* para)
{
	U32 totalspace = 0, freespace = 0;
	U32 FullAlarm = 0;
    LOGD("#################### thread PID = %d enter\n", getpid());
    
    while (1)
    {
		if(g_nSDCount > 0)
		{
			for(S32 i = 0; i < g_nSDCount; i ++)
			{
				S8 path[128];
				sprintf(path, "/dev/mmcblk%dp2", i);
				ANTS_AviFile_GetDiskStatus(path, &totalspace, &freespace);
				if(freespace < 100)
				{
					FullAlarm = 1;
				}
			}
		}
		#if 0
		if(EventUpload != NULL)
		{
			EventUpload(0, g_nSDCount);
			EventUpload(1, FullAlarm);
		}
		if(FullAlarm)
		{
			g_bDiskFull = 1;
		}
		else
		{
			g_bDiskFull = 0;
		}
		#endif
		FullAlarm = 0;

		Common_Sleep(3, 0);
    }

    LOGD("#################### thread PID = %d exit\n", getpid());
    return 0;
}

int cv_diskm_init(int dwRecycle, int dwPreserveMode, int dwPreserveTime)
{
	int bDoFileInit = 0;
	char spath[260];
	struct statfs tfs;
	int statfsret = 0;
	#if 0
	statfsret = statfs("/dev/mmcblk0",&tfs);
	if(statfsret)
	{
		LOGE("statfs error.\n");
		return -1;
	}
	
	LOGD("fs: type=0x%x block=%d avl=%ld\n",tfs.f_type, tfs.f_bsize, tfs.f_bavail);
	if (EXT4_SUPER_MAGIC != tfs.f_type)
	{
		LOGW("This fs is not ext fs.\n");
	}
	else
	{
		LOGD("This fs is ext fs.\n");
	}
	#endif
	
	int dev_fd = open("/dev/mmcblk0p2", O_RDWR);
	if (dev_fd < 0)
	{
		LOGE("Open failed. errno=[%d]\n",errno);
		g_sdcard_errno = -3;
		LOGE("g_sdcard_errno=[%d]\n",g_sdcard_errno);

		char acLogBuf[256] = {0};
		sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '333 g_sdcard_errno=[%d]' %s && echo '333 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
		system(acLogBuf);
		
		return -1;
	}
	
	unsigned long longsectors;
	unsigned long long v64;
	if (ioctl(dev_fd, BLKGETSIZE64, &v64) == 0)
	{
		/* Got bytes, convert to 512 byte sectors */
		v64 >>= 9;
		if (v64 != (unsigned long)v64)
		{
			v64 = (unsigned long)-1L;
		}
		else
		{
			LOGD("BLKGETSIZE64 v64=%llu.\n", v64);
		}
		longsectors = (unsigned long)v64;
	}
	else if (ioctl(dev_fd, BLKGETSIZE, &longsectors) == 0)
	{
		LOGD("BLKGETSIZE longsectors=%lu.\n", longsectors);
	}
	else
	{
		LOGE("failed.\n");
		longsectors = 0;
	}
	
	int value;
	if (ioctl(dev_fd, BLKSSZGET, &value) == 0)
	{
		LOGD("BLKSSZGET value=%d.\n", value);
	}
	else
	{
		LOGE("failed.\n");
	}
	
	if (dev_fd >= 0)
	{
		close(dev_fd);
		dev_fd = -1;
	}
	
	snprintf(spath,sizeof(spath),"/dev/mmcblk0p2");
	
	PartitionInfo tPartInfo;
	if(!bDoFileInit)
	{
		int ret;
		bDoFileInit = 1;
		ret = ANTS_AviFile_Init(NULL);
		if(ret)
		{
			LOGE("ANTS_AviFile_Init Failed\n");
		}
	}
	
	memset(&tPartInfo,0,sizeof(tPartInfo));
	snprintf(tPartInfo.acPath,sizeof(tPartInfo.acPath),"%s",spath);
	tPartInfo.ePriority = eRecPriority_Highest;
	tPartInfo.nPartitionTotalSpace = v64 * value;
	tPartInfo.nRecCanUseSpace  = tPartInfo.nPartitionTotalSpace;
	if(dwRecycle == 0)
		tPartInfo.nRecMaxSaveHour = 0xFFFFFFFF;
	else
	{
		if(dwPreserveMode == 0)
			tPartInfo.nRecMaxSaveHour = 0;
		else
			tPartInfo.nRecMaxSaveHour = dwPreserveTime;
	}

	if(ANTS_AviFile_AddRecPartition(&tPartInfo))
	{
		LOGE("ANTS_AviFile_AddRecPartition failed \n");
	}
	g_tPartInfo[g_nSDCount] = tPartInfo;
	g_nSDCount++;

//	Common_Thread_T ThreadHandle = NULL;
//	Common_Thread_Create(&ThreadHandle, "record_diskcheck_thread", 1024*256, COMMON_THREAD_CREATEFLAG_NORMAL, record_diskcheck_thread, NULL);

	return 0;
}

int cv_diskm_setcfg(int dwRecycle, int dwPreserveMode, int dwPreserveTime)
{
	for(int i = 0; i < g_nSDCount; i ++)
	{
		if(dwRecycle == 0)
			g_tPartInfo[i].nRecMaxSaveHour = 0xFFFFFFFF;
		else
		{
			if(dwPreserveMode == 0)
				g_tPartInfo[i].nRecMaxSaveHour = 0;
			else
				g_tPartInfo[i].nRecMaxSaveHour = dwPreserveTime;
		}

		if(ANTS_AviFile_AddRecPartition(&g_tPartInfo[i]))
		{
			LOGE("ANTS_AviFile_AddRecPartition part %d failed \n", i);
			return -1;
		}
	}

	return 0;
}

void cv_diskm_setalarmcallback(fAlarmUploadCallback	callback)
{
	EventUpload = callback;
}

int cv_diskm_getdisknum()
{
	return g_nSDCount;
}

int cv_diskm_getrecorddiskfull()
{
	return g_bDiskFull;
}

int cv_diskm_getdisknode(S32 diskno, S8 *path)
{
	if(diskno < g_nSDCount)
	{
		sprintf(path, "/dev/mmcblk%d", diskno);
	}
	return 0;
}

int cv_diskm_getdiskPartitionNum(S32 diskno, S32 *num)
{
	if(diskno < g_nSDCount)
	{
		*num = 2;
	}
	return 0;
}

int cv_diskm_getdiskPartition(S32 diskno, S32 partno, strPartitionAttr *attr)
{
	sprintf(attr->nodepath, "/dev/mmcblk%dp%d", diskno, partno + 1);
	if(partno == 0)
	{
		struct statfs tfs;
		sprintf(attr->mountpath, "/tmp/mmc/mmc%d", diskno + 1);
		sprintf(attr->describe, "Normal");
		if(0 == statfs(attr->mountpath,&tfs))
		{
			U64 totalsize = (((long long)tfs.f_bsize * (long long)tfs.f_blocks) / (long long) 1048576);
			U64 freesize = (((long long)tfs.f_bsize * (long long)tfs.f_bfree) / (long long) 1048576);
			attr->freespace = (U32)freesize;
			attr->totalspace = (U32)totalsize;
		}
	}
	else
	{
		sprintf(attr->mountpath, "NULL");
		sprintf(attr->describe, "Record");
		ANTS_AviFile_GetDiskStatus(attr->nodepath, &attr->totalspace, &attr->freespace);
	}
	return 0;
}

int cv_diskm_format(S32 diskno,S32 partno)
{
	S8 path[128];
	sprintf(path, "/dev/mmcblk%dp%d", diskno,partno+1);
	if(0 == partno)
	{
		S32 iRet = 0;
		S8 szCmd[128] = {0};
		LOGW("begin format ......\n");
		sprintf(szCmd,"umount %s",path);
		iRet = Common_System(szCmd);
		if(iRet != 0)
		{
			LOGE("%s failed!\n",szCmd);
			return iRet;
		}
		else
		{
			LOGW("%s finish!\n",szCmd);
		}
		sprintf(szCmd,"mkfs.vfat %s",path);
		iRet = Common_System(szCmd);
		if(iRet != 0)
		{
			LOGE("%s failed!\n",szCmd);
			return iRet;
		}
		else
		{
			LOGW("%s finish!\n",szCmd);
		}
		sprintf(szCmd,"mount -t vfat %s /tmp/mmc/mmc1",path);
		iRet = Common_System(szCmd);
		if(iRet != 0)
		{
			LOGE("%s failed!\n",szCmd);
			return iRet;
		}
		else
		{
			LOGW("%s finish!\n",szCmd);
		}
		return iRet;
	}
	return ANTS_AviFile_FormatDisk(path);
}

int cv_diskm_serchrecordbyday(time_t tBeginTime, U32 numOfdays, U32 *pResult)
{
	return ANTS_AviFile_JudgeIFHaveDataByDays(tBeginTime, numOfdays, pResult);
}

int cv_diskm_lockrecord(int iChannel, time_t tBeginTime, time_t tEndTime, int bLock)
{
#ifdef USE_LOCK
	return ANTS_AviFile_LockRecFile(iChannel,tBeginTime,tEndTime,bLock);
#else
	return 0;
#endif
}

