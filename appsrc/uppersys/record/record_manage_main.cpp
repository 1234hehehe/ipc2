/*
 * cv_record_manage_main.c
 *
 *  Created on: 2016.12.9
 *      Author: jiangtaixu
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef WIN32
#include <Windows.h>

#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <ctype.h>
#include <map>
#include <ucontext.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/fs.h>      // 定义 BLKGETSIZE64
#include <sys/ioctl.h>     // 提供 ioctl 函数
#include <sys/inotify.h>
#include <sys/statfs.h>
#endif
#include <libcommon_api.h>
#include <libmodule_api.h>
#include <common_media_struct.h>
#include "record_common.h"

#include "cfg_manage_api.h"
#include "record_api.h"
#include "disk_manage_api.h"
#include "replay_api.h"
#include <math.h>

using namespace cv_soft;
using namespace std;

#define MAX_QUERY_COUNT		1
#define MAX_SUBCRIBESRC 32

typedef map<S32, U32>	ReplayHandleMap;

RECORD_CONTEXT_T m_gRecordInfo;
S32 m_bEnAudio = 1;
S32 m_bInit = 0;
S32 m_bPrintDbg = 0;
int m_iQueryCnt = 0;
S32 uNoDiskRecvID[MAX_SUBCRIBESRC] = {0};
S32 uDiskFullRecvID[MAX_SUBCRIBESRC] = {0};
extern char str_SDLogCfg[64];
extern int g_sdcard_errno;

static ReplayHandleMap CV_ReplayHandle;
static Common_RWLock_T StreamLock;

static pthread_t g_pThreadTestDisk;
static int g_bTestDisk = 0;
static int g_minDiskBitrate = 0; // KB/s
static int g_diskBitrate = 0; // KB/s


#if 0
#ifndef WIN32
/* 纯C环境下，定义宏NO_CPP_DEMANGLE */
#if (!defined(__cplusplus)) && (!defined(NO_CPP_DEMANGLE))
# define NO_CPP_DEMANGLE
#endif

#ifndef NO_CPP_DEMANGLE
# include <cxxabi.h>
# ifdef __cplusplus
    using __cxxabiv1::__cxa_demangle;
# endif
#endif

#if (defined (__x86_64__))
# define REGFORMAT   "%016lx"
#elif (defined (__i386__))
# define REGFORMAT   "%08x"
#elif (defined (__arm__))
# define REGFORMAT   "%lx"
#endif

static void PrintReg(const ucontext_t *uc)
{
#if (defined (__x86_64__)) || (defined (__i386__))
    int i;
    for (i = 0; i < NGREG; i++)
    {
        LOGE("reg[%02d]: 0x"REGFORMAT"\n", i, uc->uc_mcontext.gregs[i]);
    }
#elif (defined (__arm__))
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 0, uc->uc_mcontext.arm_r0);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 1, uc->uc_mcontext.arm_r1);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 2, uc->uc_mcontext.arm_r2);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 3, uc->uc_mcontext.arm_r3);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 4, uc->uc_mcontext.arm_r4);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 5, uc->uc_mcontext.arm_r5);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 6, uc->uc_mcontext.arm_r6);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 7, uc->uc_mcontext.arm_r7);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 8, uc->uc_mcontext.arm_r8);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 9, uc->uc_mcontext.arm_r9);
    LOGE("reg[%02d]     = 0x"REGFORMAT"\n", 10, uc->uc_mcontext.arm_r10);
    LOGE("FP        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_fp);
    LOGE("IP        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_ip);
    LOGE("SP        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_sp);
    LOGE("LR        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_lr);
    LOGE("PC        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_pc);
    LOGE("CPSR      = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_cpsr);
    LOGE("Fault Address = 0x"REGFORMAT"\n", uc->uc_mcontext.fault_address);
    LOGE("Trap no       = 0x"REGFORMAT"\n", uc->uc_mcontext.trap_no);
    LOGE("Err Code  = 0x"REGFORMAT"\n", uc->uc_mcontext.error_code);
    LOGE("Old Mask  = 0x"REGFORMAT"\n", uc->uc_mcontext.oldmask);
#endif
}

static void PrintCallLink(const ucontext_t *uc)
{
    int i = 0;
    Dl_info dl_info;

#if (defined (__i386__))
    const void **frame_pointer = (const void **)uc->uc_mcontext.gregs[REG_EBP];
    const void *return_address = (const void *)uc->uc_mcontext.gregs[REG_EIP];
#elif (defined (__x86_64__))
    const void **frame_pointer = (const void **)uc->uc_mcontext.gregs[REG_RBP];
    const void *return_address = (const void *)uc->uc_mcontext.gregs[REG_RIP];
#elif (defined (__arm__))
    /* sigcontext_t on ARM:
     unsigned long trap_no;
     unsigned long error_code;
     unsigned long oldmask;
     unsigned long arm_r0;
     ...
     unsigned long arm_r10;
     unsigned long arm_fp;
     unsigned long arm_ip;
     unsigned long arm_sp;
     unsigned long arm_lr;
     unsigned long arm_pc;
     unsigned long arm_cpsr;
     unsigned long fault_address;
     */
    const void **frame_pointer = (const void **)uc->uc_mcontext.arm_fp;
    const void *return_address = (const void *)uc->uc_mcontext.arm_pc;
#else
	const void **frame_pointer = NULL;
	const void *return_address = NULL;

#endif
#if 1
    pid_t pids = getpid();
    char cmd[256] = { 0 };
    char buf[256] = { 0 };

    snprintf(cmd, sizeof(cmd) - 1, "cat /proc/%d/maps", pids);
    FILE *cfp = popen(cmd, "r");
    while (fgets(buf, sizeof(buf) - 1, cfp) != NULL )
    {
        LOGE("%s", buf);
    }
    fclose(cfp);
#endif
    LOGE("Stack trace begin:\n");
    while (return_address && i < 16)
    {
        memset(&dl_info, 0, sizeof(Dl_info));
        if (!dladdr((void *) return_address, &dl_info))
        {
            break;
        }
	const char *sname = dl_info.dli_sname;
#if (!defined NO_CPP_DEMANGLE)
        int status;
        char *tmp = __cxa_demangle(sname, NULL, 0, &status);
        if (status == 0 && tmp)
        {
            sname = tmp;
        }
#endif
        /* No: return address <sym-name + offset> (filename) */

        LOGE("%02d: %p <%s + %lu> %s \n", i++, return_address, dl_info.dli_sname,
                        (unsigned long )return_address - (unsigned long )dl_info.dli_saddr, dl_info.dli_fname);

#if (!defined NO_CPP_DEMANGLE)
        if (tmp) free(tmp);
#endif
        if ((dl_info.dli_sname && !strcmp(dl_info.dli_sname, "main")))
        {
            break;
        }
#if (defined (__x86_64__)) || (defined (__i386__))
        return_address = frame_pointer[1];
        frame_pointer = (const void **)frame_pointer[0];
#elif (defined (__arm__))
        if (frame_pointer != NULL)
        {
            return_address = frame_pointer[-1];
            frame_pointer = (const void **)(frame_pointer[-3]);
        }
        else if (return_address != (const void *)uc->uc_mcontext.arm_lr)
        {
            return_address = (const void *)uc->uc_mcontext.arm_lr;
        }
        else
        break;
#endif
    }
    LOGE("Stack trace end.\n");
}

static void SigHandler(int signo, siginfo_t *info, void *context)
{
    LOGE("info.si_signo = %d\n", signo);
    if (signo == SIGPIPE || signo == SIGABRT)
        return;

    if (info)
    {
        LOGE("info.si_errno = %d\n", info->si_errno);
        LOGE("info.si_code  = %d (%s)\n", info->si_code, (info->si_code == SEGV_MAPERR) ? "SEGV_MAPERR" : "SEGV_ACCERR");
        LOGE("info.si_addr  = %p\n", info->si_addr);
    }

    if (context)
    {
        const ucontext_t *uc = (const ucontext_t *) context;
        PrintReg(uc);
        PrintCallLink(uc);

    }

    _exit(0);
}
#endif
static int RegistSigHandle(int sigNum)
{
#ifdef WIN32
#else
    struct sigaction sa;
    sigset_t waitset;
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_sigaction = SigHandler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(sigNum, &sa, NULL) < 0)
    {
        LOGE("register signal handle failed %s\n", strerror(errno));
        return -1;
    }

    sigemptyset(&waitset);
    sigaddset(&waitset, sigNum);
    pthread_sigmask(SIG_UNBLOCK, &waitset, NULL);
#endif
    LOGI("regist signal %d\n", sigNum);
    return 0;
}
#endif
S32 atoul(S8 *str, S32 * pulValue)
{
    S32 ulResult=0;

    while (*str)
    {
        if (isdigit((int)*str))
        {
            /*最大支持到0xFFFFFFFF(4294967295),
               X * 10 + (*str)-48 <= 4294967295
               所以， X = 429496729 */
            if ((ulResult<429496729) || ((ulResult==429496729) && (*str<'6')))
            {
                ulResult = ulResult*10 + (*str)-48;
            }
            else
            {
                *pulValue = ulResult;
                return -1;
            }
        }
        else
        {
            *pulValue=ulResult;
            return -1;
        }
        str++;
    }
    *pulValue=ulResult;
    return 0;
}
int atotime(char *str, Common_Time_T * pulValue)
{
	S32 lResult[6] = {0};
    	S32 ulResult = 0;
	S32 i = 0;
	S32 strsize = strlen(str);

	if(str == NULL || pulValue == NULL)
		return -1;
   	 while (*str)
	{
		if(i >= strsize)
			break;
		if (isdigit((int)*str))
		{
			if(i > 2 && ((i % 2) == 0))
				ulResult = 0;
		    	ulResult = ulResult*10 + (*str)-48;
		}
		else
		{
			return -1;
		}
		if(i < 4)
			lResult[0] = ulResult;
		else if(i < 6)
			lResult[1] = ulResult;
		else if(i < 8)
			lResult[2] = ulResult;
		else if(i < 10)
			lResult[3] = ulResult;
		else if(i < 12)
			lResult[4] = ulResult;
		else if(i < 14)
			lResult[5] = ulResult;
		else
			break;
		i ++;
		str++;
	}
	pulValue->year = lResult[0];
	if((strsize > 4) && (lResult[1] < 1 || lResult[1] > 12))
		return -1;
	pulValue->month = lResult[1];
	if((strsize > 6) && (lResult[2] < 1 || lResult[2] > 31))
		return -1;
	pulValue->day = lResult[2];
	if((strsize > 8) && (lResult[3] < 0 || lResult[3] > 23))
		return -1;
	pulValue->hour = lResult[3];
	if((strsize > 10) && (lResult[4] < 0 || lResult[4] > 59))
		return -1;
	pulValue->min = lResult[4];
	if((strsize > 12) && (lResult[5] < 0 || lResult[5] > 59))
		return -1;
	pulValue->sec = lResult[5];
    return 0;
}

// 检查设备是否存在且可访问
int check_device_exists(const char *dev_path)
{
	if(access(dev_path,F_OK) != 0)
	{
		LOGE("%s is not exist\n", dev_path);
        return -1; // 设备不存在
	}

    struct stat st;
    if (stat(dev_path, &st) == -1)
	{
		LOGE("%s stat failed\n", dev_path);
        return -1; // 设备不存在
    }

    if (!S_ISBLK(st.st_mode))
	{
        LOGE("%s is not block device\n", dev_path);
        return -2; // 不是块设备
    }

	LOGD("%s exist\n", dev_path);
    return 0;
}

// 检测设备是否可读写
int check_device_io(const char *dev_path)
{
    int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
    if (fd == -1)
	{
		LOGE("%s open failed\n", dev_path);
        return -1;
    }

    // 尝试获取设备大小（触发I/O操作）
    unsigned long long size = 0;
    if (ioctl(fd, BLKGETSIZE64, &size) == -1)
	{
		LOGE("%s ioctl failed\n", dev_path);
        close(fd);
        return -2;
    }

    LOGD("%s size: %llu bytes\n", dev_path, size);
    close(fd);
    return 0;
}

// 检查分区表有效性
int check_partition_table(const char *dev_path)
{
	int ret = 0;
	char res[256];
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "fdisk -l %s 2>&1 | grep -iE \"input/output error|can't open|unable to read\" > /dev/null; echo $?", dev_path);
	memset(res, 0, sizeof(res));
	ret = Common_Exe_Cmd(cmd, 1000, res, sizeof(res));
	if(ret != 0)
	{
		LOGE("%s fdisk failed\n", dev_path);
		return -1;
	}

	ret = atoi(res);
	if(ret == 0)
	{
		LOGE("%s fdisk failed\n", dev_path);
		return -1;
	}
	else
	{
		LOGD("%s check partition succ\n", dev_path);
		return 0;
	}
}

int check_sd_node_status()
{
	char acSDPath[16] = "/dev/mmcblk0";
	char acSDPartion2[16] = "/dev/mmcblk0p2";

	if(check_device_exists(acSDPath) != 0)
	{
		//Common_System("ls -lh /dev/mmcblk*; fdisk -l /dev/mmcblk*\n");
		g_sdcard_errno = -5;
		LOGE("g_sdcard_errno=[%d]\n",g_sdcard_errno);

		char acLogBuf[256] = {0};
		sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '444 g_sdcard_errno=[%d]' %s && echo '444 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
		system(acLogBuf);

		return -1;
	}

	if((check_device_exists(acSDPartion2) != 0) || (check_device_io(acSDPartion2) != 0) || (check_partition_table(acSDPartion2) != 0))
	{
		//Common_System("ls -lh /dev/mmcblk*; fdisk -l /dev/mmcblk*\n");
		g_sdcard_errno = -5;
		LOGE("g_sdcard_errno=[%d]\n",g_sdcard_errno);

		char acLogBuf[256] = {0};
		sprintf(acLogBuf, "[ ! -e %s ] || ! grep -qF '555 g_sdcard_errno=[%d]' %s && echo '555 g_sdcard_errno=[%d]' >> %s",str_SDLogCfg,g_sdcard_errno,str_SDLogCfg,g_sdcard_errno,str_SDLogCfg);
		system(acLogBuf);

		return -1;
	}

	return 0;
}

static void *TestDisk(void* param)
{
	prctl(PR_SET_NAME,__func__);

	unsigned int len = 500*1024;
	char *pBuffer = (char*)malloc(len);
	unsigned int tBegin = 0,tEnd = 0;
	struct timeval tv;

	while(g_bTestDisk == 1)
	{
		gettimeofday(&tv, NULL);
		tBegin = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	    FILE *fp = fopen("/tmp/mmc/mmc1/test.bin","wb+");
		if(len != fwrite(pBuffer,1,len,fp))
		{
			LOGE("write error!\n");
		}
		fseek(fp,0,SEEK_SET);
		if(len != fread(pBuffer,1,len,fp))
		{
			LOGE("read error!\n");
		}
	    fflush(fp);
	    fclose(fp);

		gettimeofday(&tv, NULL);
		tEnd = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		if(tEnd>tBegin)
			g_diskBitrate = (g_diskBitrate + 2*500*1024/(tEnd-tBegin))/2;

		if(g_diskBitrate < g_minDiskBitrate || g_minDiskBitrate ==0)
		{
			g_minDiskBitrate = g_diskBitrate;
		}

		LOGW("g_diskBitrate=%d g_minDiskBitrate=%d time=%d\n", g_diskBitrate, g_minDiskBitrate, tEnd-tBegin);
		usleep(10000);
	}

	if(pBuffer != NULL)
	{
		free(pBuffer);
		pBuffer = NULL;
	}

	return NULL;
}

static int GetSDCardStatus(cJSON_Struct *pOut)
{

#if (defined PLATFORM_HI3516EV200)
	unsigned int iValue = 0;
	HI_MPI_SYS_GetReg(0x10010024, &iValue);
	int iDetect = (iValue>>18) & 0x01;
#elif (defined PLATFORM_HI3516CV500)
	unsigned int iValue = 0;
	HI_MPI_SYS_GetReg(0x100f0050, &iValue);
	int iDetect = 0;
	LOGE("iValue=%d\n", iValue);
	if((iValue & 0x01) == 0)
	{
		iDetect = 1;
	}
#elif (defined PLATFORM_JZT30) || (defined PLATFORM_JZT32) || (defined PLATFORM_JZT40) || (defined PLATFORM_JZT41)
	int iDetect = 1;
#else
	int iDetect = 0;
#endif

	struct stat statFile;
	int iErrorCode = 0;

#if (defined PLATFORM_JZT30) || (defined PLATFORM_JZT32) || (defined PLATFORM_JZT40) || (defined PLATFORM_JZT41)
	if(stat("/dev/mmcblk0", &statFile) != 0)
	{
		Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "No SD", 0, 0);
		iErrorCode = -1;
	}
	else
#else
	if(0 == iDetect)
	{
		Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "No SD", 0, 0);
		iErrorCode = -1;
	}
	else if(1 == iDetect && stat("/dev/mmcblk0", &statFile) != 0)
	{
		Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD is detect but abnormal", 0, 0);
		iErrorCode = -2;
	}
	else if(stat("/dev/mmcblk0", &statFile) == 0)
#endif
	{
		if(g_sdcard_errno != 0)
		{
			if(g_sdcard_errno == -3)
			{
				Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD read or write abnormal", 0, 0);
				iErrorCode = g_sdcard_errno;
			}
			else if(g_sdcard_errno == -4)
			{
				Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD Capacity Fraud", 0, 0);
				iErrorCode = g_sdcard_errno;
			}
			else if(g_sdcard_errno == -5)
			{
				Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD node info abnormal", 0, 0);
				iErrorCode = g_sdcard_errno;
			}
			else if(g_sdcard_errno == -6)
			{
				Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD Capacity less than 8GB or greater than 512GB", 0, 0);
				iErrorCode = g_sdcard_errno;
			}
		}
		else
		{
			U32 SchedMode, RecordMask, RecordType;
			cv_record_get_ch_state(0, &RecordType, &RecordMask, &SchedMode);
			U32 SheduleStatus = 0;
			cv_record_get_shedule_status(&SheduleStatus);
			if((RecordType != 0) && (SheduleStatus == TRUE))
			{
				Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD is Recording", 0, 0);
				iErrorCode = 1;
			}
			else
			{
				Common_Json_SetAttrValue(pOut, -1, "Data/Decribe", Common_Json_Type_String, "SD is normal", 0, 0);
				iErrorCode = 0;
			}
		}
	}

	Common_Json_SetAttrValue(pOut, -1, "Data/Status", Common_Json_Type_Number, NULL, iErrorCode, 0);

    return iErrorCode;
}

static int SendSDCardStatus()
{
    int ret = -1;
    int status = 0;
    cJSON_Struct *pInData = NULL,*pOutData = NULL;

    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValueObj(pInData, "Header");
    Common_Json_SetAttrValueStr(pInData, "Header/Uri", "/WebServer/DeviceStatus");
    Common_Json_SetAttrValueStr(pInData, "Header/Method", "put");
    Common_Json_SetAttrValueObj(pInData, "Data");

    status = GetSDCardStatus(NULL);
    Common_Json_SetAttrValueInt(pInData, "Data/SDCardStatus", status);

    Module_CallFunctions(m_gRecordInfo.ModeHandle,pInData,&pOutData,3000);
    Common_Json_GetAttrValueInt(pOutData, "Header/Code", &ret);

    Common_Json_Delete(pInData);
    Common_Json_Delete(pOutData);

    return ret;
}

int ReceiveCmd(ModuleHandle_T hModuleHandle, cJSON_Struct *pInParams, cJSON_Struct **pOutParams, void *pUserData)
{
	S8 *str;
	S32 errcode = MODULE_ERROR_TYPE_SUCC;
	U32 channel;
	cv_device_capability *psyscap;
	cJSON_Struct	*pOut = NULL;
	cJSON_Struct 	*pNode, *pNode1,*pChild = NULL;
	//if(!m_bInit)
	//	return -1;
	if(pInParams == NULL || pOutParams == NULL)
		return -1;

	psyscap = cv_cfgm_get_dev_cap();
	Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &str, NULL, NULL);

	pOut = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	*pOutParams = pOut;
	Common_Json_SetAttrValue(pOut, -1, "Header", Common_Json_Type_Object, NULL, 0, 0);
	Common_Json_SetAttrValue(pOut, -1, "Header/Uri", Common_Json_Type_String, str, 0, 0);
	pChild = Common_Json_SetAttrValue(pOut, -1, "Data", Common_Json_Type_Object, NULL, 0, 0);

	if((m_bPrintDbg&0x01) && pInParams)
	{
		OVFS_PRINT_JSON(pInParams);
	}
	if(Common_StriCmp(str, (S8*)"/Record") == 0)
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);

			Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String, "/Record/RecordConfig", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "RecordConfig", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 1, "/Uri", Common_Json_Type_String, "/Record/RecordStatus", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "/Label", Common_Json_Type_String, "RecordStatus", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 1, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 2, "/Uri", Common_Json_Type_String, "/Record/Functions", 0, 0);
			Common_Json_SetAttrValue(pNode, 2, "/Label", Common_Json_Type_String, "Functions", 0, 0);
			Common_Json_SetAttrValue(pNode, 2, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 2, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 3, "/Uri", Common_Json_Type_String, "/Record/Replay", 0, 0);
			Common_Json_SetAttrValue(pNode, 3, "/Label", Common_Json_Type_String, "Replay", 0, 0);
			Common_Json_SetAttrValue(pNode, 3, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 3, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 4, "/Uri", Common_Json_Type_String, "/Record/DiskManage", 0, 0);
			Common_Json_SetAttrValue(pNode, 4, "/Label", Common_Json_Type_String, "DiskManage", 0, 0);
			Common_Json_SetAttrValue(pNode, 4, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 4, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 5, "/Uri", Common_Json_Type_String, "/Record/printDebug", 0, 0);
			Common_Json_SetAttrValue(pNode, 5, "/Label", Common_Json_Type_String, "printDebug", 0, 0);
			Common_Json_SetAttrValue(pNode, 5, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 5, NULL, Common_Json_Type_String, "Get or Put", 0, 0);

			Common_Json_SetAttrValue(pNode, 6, "/Uri", Common_Json_Type_String, "/Record/Status", 0, 0);
			Common_Json_SetAttrValue(pNode, 6, "/Label", Common_Json_Type_String, "Status", 0, 0);
			Common_Json_SetAttrValue(pNode, 6, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 6, NULL, Common_Json_Type_String, "Get", 0, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/printDebug") == 0)
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "0x02:printWframe 0x04:printRframe 0x08:printWMKV 0x10:printCostSec 0x20:printIndex 0x40:printCodec 0x80:printPlay", 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/printDebug", Common_Json_Type_Number, NULL, m_bPrintDbg, 0);
		}
		else if(Common_StriCmp(str, (S8*)"Put") == 0)
		{
			Common_Json_GetAttrValue(pInParams, -1, "Data/printDebug", NULL, NULL, &m_bPrintDbg, 0);
/*			if(!m_bPrintDbg)
				ANTS_AviFile_SetDbgPrint(0);
			else if(m_bPrintDbg != 1)
				ANTS_AviFile_SetDbgPrint(m_bPrintDbg);*/
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/RecordConfig") == 0)
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String, "/Record/RecordConfig/PreserveRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "PreserveRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
			Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
			for(U32 i = 0; i < psyscap->video_vi_num; i ++)
			{
				S8 strtmp[128];

				sprintf(strtmp, "/Record/RecordConfig/Device%d", i);
				Common_Json_SetAttrValue(pNode, i + 1, "/Uri", Common_Json_Type_String, strtmp, 0, 0);
				sprintf(strtmp, "Device%d", i);
				Common_Json_SetAttrValue(pNode, i + 1, "/Label", Common_Json_Type_String, strtmp, 0, 0);
				Common_Json_SetAttrValue(pNode, i + 1, "/Describe", Common_Json_Type_String, "None", 0, 0);
				pNode1 = Common_Json_SetAttrValue(pNode, i + 1, "/Method", Common_Json_Type_Array, NULL, 0, 0);
				Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
			}
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/RecordConfig/PreserveRecord") == 0)
	{
		S32 RecycleRecord;
		PRESERVERECORDRULE	PreserveRecord;

		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			cv_cfgm_get_diskconfig(&RecycleRecord, &PreserveRecord);
			Common_Json_SetAttrValue(pOut, -1, "Data/RecycleRecord", Common_Json_Type_Number, NULL, RecycleRecord, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/PreserveMode", Common_Json_Type_Number, NULL, PreserveRecord.PreserveMode, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/PreservePercent", Common_Json_Type_Number, NULL, PreserveRecord.PreservePercent, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/PreserveTime", Common_Json_Type_Number, NULL, PreserveRecord.PreserveTime, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/PreserveVolume", Common_Json_Type_Number, NULL, PreserveRecord.PreserveVolume, 0);
		}
		else if(Common_StriCmp(str, (S8*)"Put") == 0)
		{
			Common_Json_GetAttrValue(pInParams, -1, "Data/RecycleRecord", NULL, NULL, &RecycleRecord, 0);
			Common_Json_GetAttrValue(pInParams, -1, "Data/PreserveMode", NULL, NULL, (S32*)&PreserveRecord.PreserveMode, 0);
			Common_Json_GetAttrValue(pInParams, -1, "Data/PreservePercent", NULL, NULL, (S32*)&PreserveRecord.PreservePercent, 0);
			Common_Json_GetAttrValue(pInParams, -1, "Data/PreserveTime", NULL, NULL, (S32*)&PreserveRecord.PreserveTime, 0);
			Common_Json_GetAttrValue(pInParams, -1, "Data/PreserveVolume", NULL, NULL, (S32*)&PreserveRecord.PreserveVolume, 0);
			if(RecycleRecord != 0)
			{
				if(PreserveRecord.PreserveMode != 1 && PreserveRecord.PreserveMode != 2 && PreserveRecord.PreserveMode != 4)
					errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
				if(PreserveRecord.PreserveTime < 1)
					errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
/*
				if(PreserveRecord.PreserveVolume < 1)
					errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
				if(PreserveRecord.PreservePercent < 1 || PreserveRecord.PreservePercent > 100)
					errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
*/
			}
			if(0 == cv_diskm_setcfg(RecycleRecord, PreserveRecord.PreserveMode, PreserveRecord.PreserveTime))
			{
				cv_cfgm_set_diskconfig(RecycleRecord, PreserveRecord);
			}
			else
			{
				errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
			}
			if(errcode == MODULE_ERROR_TYPE_SUCC)
			{
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			}
			else
			{
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, errcode, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			}
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StrniCmp(str, (S8*)"/Record/RecordConfig/Device", 27) == 0)
	{
		RECORD_FUNCTION sRecFunc = {0};
		cv_cfgm_get_record_func(&sRecFunc);
		for(U32 i = 0; i < psyscap->video_vi_num; i ++)
		{
			S8 strchn[128];
			sprintf(strchn, "/Record/RecordConfig/Device%d", i);
			if(Common_StriCmp(str, strchn) == 0)
			{
				Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
				if(Common_StriCmp(str, (S8*)"Get") == 0)
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

					pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);
					for(U32 j = 0; j < psyscap->video_vi_chn[i]; j ++)
					{
						S8 strtmp[128];

						sprintf(strtmp, "/Record/RecordConfig/Device%d/Channel%d", i, j);
						Common_Json_SetAttrValue(pNode, j, "/Uri", Common_Json_Type_String, strtmp, 0, 0);
						sprintf(strtmp, "Channel%d", j);
						Common_Json_SetAttrValue(pNode, j, "/Label", Common_Json_Type_String, strtmp, 0, 0);
						Common_Json_SetAttrValue(pNode, j, "/Describe", Common_Json_Type_String, "None", 0, 0);
						pNode1 = Common_Json_SetAttrValue(pNode, j, "/Method", Common_Json_Type_Array, NULL, 0, 0);
						Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
						Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);
					}
				}
				else
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
					return -1;
				}
			}
			else
			{
				for(U32 k = 0; k < psyscap->video_vi_chn[i]; k ++)
				{
					sprintf(strchn, "/Record/RecordConfig/Device%d/Channel%d", i, k);

					if(Common_StriCmp(str, strchn) == 0)
					{
						U32 RecordMode, ManualStreamMask;
						PRERECORDRULE PreRecord;
						cv_record_arming_day dayplan[7];
						channel = cv_cfgm_get_channel(i, k);

						cv_cfgm_get_record_mode(channel, &RecordMode);
						cv_cfgm_get_manual_mask(channel, &ManualStreamMask);
						cv_cfgm_get_prerecord(channel, &PreRecord);
#ifdef JZT31N
						//报警录像&&延迟时间小于120秒,设置为120秒
						if(PreRecord.DelayRecordTime < 120)
						{
							PreRecord.DelayRecordTime = 120;
						}
#endif
						for(U32 j = 0; j < 7; j ++)
						{
							cv_cfgm_get_plan_day(channel, j, &dayplan[j]);
						}
						Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
						if(Common_StriCmp(str, (S8*)"Get") == 0)
						{
							Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
							Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/RecordMode", Common_Json_Type_Number, NULL, RecordMode, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/ManualStreamMask", Common_Json_Type_Number, NULL, ManualStreamMask, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/PreRecordMode", Common_Json_Type_Number, NULL, PreRecord.PreRecordMode, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/PreRecordTime", Common_Json_Type_Number, NULL, PreRecord.PreRecordTime, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/PreRecordSpace", Common_Json_Type_Number, NULL, PreRecord.PreRecordMem, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/PreRecordDelayTime", Common_Json_Type_Number, NULL, PreRecord.DelayRecordTime, 0);
							for(U32 j = 0; j < 7; j ++)
							{
								S8 strplan[128];

								sprintf(strplan, "Data/Week%d", j);
								Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Object, "ovfs", 0, 0);

								sprintf(strplan, "Data/Week%d/AlldayEnable", j);
								Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Number, NULL, dayplan[j].bAlldayRec, 0);
								for(U32 n = 0; n < CV_MAX_ARMING_TIME_QUANTUM; n ++)
								{
									sprintf(strplan, "Data/Week%d/Sched%d", j, n);
									Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Object, "ovfs", 0, 0);

									sprintf(strplan, "Data/Week%d/Sched%d/StartTime", j, n);
									Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Number, NULL, dayplan[j].arming[n].time.start_hour_min, 0);
									sprintf(strplan, "Data/Week%d/Sched%d/StopTime", j, n);
									Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Number, NULL, dayplan[j].arming[n].time.stop_hour_min, 0);
									sprintf(strplan, "Data/Week%d/Sched%d/SchedMode", j, n);
									Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Number, NULL, dayplan[j].arming[n].arming_type, 0);
									sprintf(strplan, "Data/Week%d/Sched%d/StreamMask", j, n);
									Common_Json_SetAttrValue(pOut, -1, strplan, Common_Json_Type_Number, NULL, dayplan[j].arming[n].recordmask, 0);
								}
							}
						}
						else if(Common_StriCmp(str, (S8*)"Put") == 0)
						{
							Common_Json_GetAttrValue(pInParams, -1, "Data/RecordMode", NULL, NULL, (S32*)&RecordMode, 0);
							if(RecordMode > 2)
							{
								errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
							}
							Common_Json_GetAttrValue(pInParams, -1, "Data/ManualStreamMask", NULL, NULL, (S32*)&ManualStreamMask, 0);
							if(ManualStreamMask > 0x0f)
							{
								errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
							}
							Common_Json_GetAttrValue(pInParams, -1, "Data/PreRecordMode", NULL, NULL, (S32*)&PreRecord.PreRecordMode, 0);
							if(PreRecord.PreRecordMode> 1||PreRecord.PreRecordMode < 0)
							{
								PreRecord.PreRecordMode = 0;
							}
							Common_Json_GetAttrValue(pInParams, -1, "Data/PreRecordTime", NULL, NULL, (S32*)&PreRecord.PreRecordTime, 0);
							if(PreRecord.PreRecordTime > 60)
							{
								PreRecord.PreRecordTime = 60;
							}
							Common_Json_GetAttrValue(pInParams, -1, "Data/PreRecordSpace", NULL, NULL, (S32*)&PreRecord.PreRecordMem, 0);
							if(PreRecord.PreRecordMem > (5 << 20))
							{
								errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
							}
							Common_Json_GetAttrValue(pInParams, -1, "Data/PreRecordDelayTime", NULL, NULL, (S32*)&PreRecord.DelayRecordTime, 0);
							if(PreRecord.DelayRecordTime > 600)
							{
								errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
							}
							for(U32 j = 0; j < 7; j ++)
							{
								S8 strplan[128];

								sprintf(strplan, "Data/Week%d/AlldayEnable", j);
								Common_Json_GetAttrValue(pInParams, -1, strplan, NULL, NULL, (S32*)&dayplan[j].bAlldayRec, 0);
								for(U32 n = 0; n < CV_MAX_ARMING_TIME_QUANTUM; n ++)
								{
									if(dayplan[j].bAlldayRec)
									{
										if(0 == n)
										{
											dayplan[j].arming[n].time.start_hour_min = 0;
											dayplan[j].arming[n].time.stop_hour_min = 2359;
										}
										else
										{
											dayplan[j].arming[n].time.start_hour_min = 0;
											dayplan[j].arming[n].time.stop_hour_min = 0;
										}
									}
									else
									{
										sprintf(strplan, "Data/Week%d/Sched%d/StartTime", j, n);
										Common_Json_GetAttrValue(pInParams, -1, strplan, NULL, NULL, (S32*)&dayplan[j].arming[n].time.start_hour_min, 0);
										sprintf(strplan, "Data/Week%d/Sched%d/StopTime", j, n);
										Common_Json_GetAttrValue(pInParams, -1, strplan, NULL, NULL, (S32*)&dayplan[j].arming[n].time.stop_hour_min, 0);
									}
									sprintf(strplan, "Data/Week%d/Sched%d/SchedMode", j, n);
									Common_Json_GetAttrValue(pInParams, -1, strplan, NULL, NULL, &dayplan[j].arming[n].arming_type, 0);
									sprintf(strplan, "Data/Week%d/Sched%d/StreamMask", j, n);
									Common_Json_GetAttrValue(pInParams, -1, strplan, NULL, NULL, &dayplan[j].arming[n].recordmask, 0);
									if((dayplan[j].arming[n].time.start_hour_min / 100) > 23)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
									if((dayplan[j].arming[n].time.start_hour_min % 100) > 59)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
									if((dayplan[j].arming[n].time.stop_hour_min / 100) > 23)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
									if((dayplan[j].arming[n].time.stop_hour_min % 100) > 59)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
									if(dayplan[j].arming[n].time.stop_hour_min < dayplan[j].arming[n].time.start_hour_min)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
									if(dayplan[j].arming[n].arming_type < 1 || dayplan[j].arming[n].arming_type > 3)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
									if(dayplan[j].arming[n].recordmask < 1 || dayplan[j].arming[n].recordmask > 0xf)
									{
										errcode = MODULE_ERROR_TYPE_INVALIDPARAM;
									}
								}
							}

							if(errcode == MODULE_ERROR_TYPE_SUCC)
							{
#ifdef JZT31N
								//报警录像&&延迟时间小于120秒,设置为120秒
								if(PreRecord.DelayRecordTime < 120)
								{
									PreRecord.DelayRecordTime = 120;
								}
#endif
								cv_record_set_record_mode(channel, RecordMode);
								cv_cfgm_set_record_mode(channel, RecordMode);
								cv_record_set_record_manualmask(channel, ManualStreamMask);
								cv_cfgm_set_manual_mask(channel, ManualStreamMask);
								cv_record_set_pre_rec_mode(channel, PreRecord.PreRecordMode);
								if(sRecFunc.PreRecord)
								{
									cv_record_set_pre_rec_sec(channel, PreRecord.PreRecordTime);
								}
								else
								{
									cv_record_set_pre_rec_sec(channel, 0);
								}
								cv_record_set_pre_rec_mem(channel, PreRecord.PreRecordMem);
								cv_record_set_delay_rec_sec(channel, PreRecord.DelayRecordTime);
								cv_cfgm_set_prerecord(channel, PreRecord);
								for(U32 j = 0; j < 7; j ++)
								{
									cv_record_set_rec_plan_day(channel, j, &dayplan[j]);
									cv_cfgm_set_plan_day(channel, j, dayplan[j]);
								}
								cv_record_saveconfig();
								Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
								Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

                                SendSDCardStatus();
							}
							else
							{
								Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, errcode, 0);
								Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
							}
						}
						else
						{
							Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
							Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
							return -1;
						}
						break;
					}
				}
			}
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/RecordStatus") == 0)
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);
			for(U32 i = 0; i < psyscap->video_vi_num; i ++)
			{
				S8 strtmp[128];

				sprintf(strtmp, "/Record/RecordStatus/Device%d", i);
				Common_Json_SetAttrValue(pNode, i, "/Uri", Common_Json_Type_String, strtmp, 0, 0);
				sprintf(strtmp, "Device%d", i);
				Common_Json_SetAttrValue(pNode, i, "/Label", Common_Json_Type_String, strtmp, 0, 0);
				Common_Json_SetAttrValue(pNode, i, "/Describe", Common_Json_Type_String, "None", 0, 0);
				pNode1 = Common_Json_SetAttrValue(pNode, i, "/Method", Common_Json_Type_Array, NULL, 0, 0);
				Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
			}
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StrniCmp(str, (S8*)"/Record/RecordStatus/Device", 27) == 0)
	{
		for(U32 i = 0; i < psyscap->video_vi_num; i ++)
		{
			S8 strchn[128];
			sprintf(strchn, "/Record/RecordStatus/Device%d", i);
			if(Common_StriCmp(str, strchn) == 0)
			{
				Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
				if(Common_StriCmp(str, (S8*)"Get") == 0)
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

					pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);
					for(U32 j = 0; j < psyscap->video_vi_chn[i]; j ++)
					{
						S8 strtmp[128];

						sprintf(strtmp, "/Record/RecordStatus/Device%d/Channel%d", i, j);
						Common_Json_SetAttrValue(pNode, j, "/Uri", Common_Json_Type_String, strtmp, 0, 0);
						sprintf(strtmp, "Channel%d", j);
						Common_Json_SetAttrValue(pNode, j, "/Label", Common_Json_Type_String, strtmp, 0, 0);
						Common_Json_SetAttrValue(pNode, j, "/Describe", Common_Json_Type_String, "None", 0, 0);
						pNode1 = Common_Json_SetAttrValue(pNode, j, "/Method", Common_Json_Type_Array, NULL, 0, 0);
						Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
					}
				}
				else
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
					return -1;
				}
			}
			else
			{
				for(U32 j = 0; j < psyscap->video_vi_chn[i]; j ++)
				{
					sprintf(strchn, "/Record/RecordStatus/Device%d/Channel%d", i, j);

					if(Common_StriCmp(str, strchn) == 0)
					{
						channel = cv_cfgm_get_channel(i, j);

						Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
						if(Common_StriCmp(str, (S8*)"Get") == 0)
						{
							U32 RecordMode, SchedMode, RecordMask, RecordType;

							Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
							Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

							cv_cfgm_get_recplan(channel, &RecordMode);
							cv_record_get_ch_state(channel, &RecordType, &RecordMask, &SchedMode);
							Common_Json_SetAttrValue(pOut, -1, "Data/RecordMode", Common_Json_Type_Number, NULL, RecordMode, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/SchedMode", Common_Json_Type_Number, NULL, SchedMode, 0);
							Common_Json_SetAttrValue(pOut, -1, "Data/RecordType", Common_Json_Type_Object, "ovfs", 0, 0);
							for(U32 k = 0; k < psyscap->video_stream_num[channel]; k ++)
							{
								S8 strstream[128];
								sprintf(strstream, "Data/RecordType/Stream%d", k);
								if((RecordMask & (0x1 << k)) != 0)
									Common_Json_SetAttrValue(pOut, -1, strstream, Common_Json_Type_Number, NULL, RecordType, 0);
								else
									Common_Json_SetAttrValue(pOut, -1, strstream, Common_Json_Type_Number, NULL, 0, 0);
							}
						}
					}
				}
			}
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Functions") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);

			Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String, "/Record/Functions/StartAlarmRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "StartAlarmRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);

			Common_Json_SetAttrValue(pNode, 1, "/Uri", Common_Json_Type_String, "/Record/Functions/StopAlarmRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "/Label", Common_Json_Type_String, "StopAlarmRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 1, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Functions/StartAlarmRecord") == 0 && (g_sdcard_errno == 0))
	{
		if(!m_bInit)
			return -1;
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Put") == 0)
		{
			U32 alarmtype, alarmSrc;
			cv_event_id linkevent;

			Common_Json_GetAttrValue(pInParams, -1, "Data/AlarmType", NULL, NULL, (S32*)&alarmtype, NULL);
			Common_Json_GetAttrValue(pInParams, -1, "Data/AlarmSrc", NULL, NULL, (S32*)&alarmSrc, NULL);
			if(0 == cv_cfgm_change_event(&linkevent, alarmtype, alarmSrc))
			{
				pNode = Common_Json_GetItem(pInParams, -1, "Data/LinkChannel");
				U32 ChannelNum = Common_Json_Size(pNode);
				for(U32 i = 0; i < ChannelNum; i ++)
				{
					U32 videv, vichn;
					Common_Json_GetAttrValue(pNode, i, "Device", NULL, NULL, (S32*)&videv, NULL);
					Common_Json_GetAttrValue(pNode, i, "Channel", NULL, NULL, (S32*)&vichn, NULL);
					cv_record_set_linkage(cv_cfgm_get_channel(videv, vichn), &linkevent, 1);
					LOGW("StartAlarmRecord,videv %d vichn %d alarmtype %d alarmSrc %d\n",videv,vichn,alarmtype,alarmSrc);
				}
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			}
			else
			{
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
				return -1;
			}
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Functions/StopAlarmRecord") == 0 && (g_sdcard_errno == 0))
	{
		if(!m_bInit)
			return -1;
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Put") == 0)
		{
			U32 alarmtype, alarmSrc;
			cv_event_id linkevent;

			Common_Json_GetAttrValue(pInParams, -1, "Data/AlarmType", NULL, NULL, (S32*)&alarmtype, NULL);
			Common_Json_GetAttrValue(pInParams, -1, "Data/AlarmSrc", NULL, NULL, (S32*)&alarmSrc, NULL);
			if(0 == cv_cfgm_change_event(&linkevent, alarmtype, alarmSrc))
			{
				pNode = Common_Json_GetItem(pInParams, -1, "Data/LinkChannel");
				U32 ChannelNum = Common_Json_Size(pNode);
				for(U32 i = 0; i < ChannelNum; i ++)
				{
					U32 videv, vichn;
					Common_Json_GetAttrValue(pNode, i, "Device", NULL, NULL, (S32*)&videv, NULL);
					Common_Json_GetAttrValue(pNode, i, "Channel", NULL, NULL, (S32*)&vichn, NULL);
					cv_record_set_linkage(cv_cfgm_get_channel(videv, vichn), &linkevent, 0);
					LOGW("StopAlarmRecord,videv %d vichn %d alarmtype %d alarmSrc %d\n",videv,vichn,alarmtype,alarmSrc);
				}
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			}
			else
			{
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
				return -1;
			}
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Replay") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);

			Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String, "/Record/Replay/RecordList", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "RecordList", 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 1, "/Uri", Common_Json_Type_String, "/Record/Replay/RecordListByMonthly", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "/Label", Common_Json_Type_String, "RecordListByMonthly", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 1, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 2, "/Uri", Common_Json_Type_String, "/Record/Replay/HistoryStream", 0, 0);
			Common_Json_SetAttrValue(pNode, 2, "/Label", Common_Json_Type_String, "HistoryStream", 0, 0);
			Common_Json_SetAttrValue(pNode, 2, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 2, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

			Common_Json_SetAttrValue(pNode, 3, "/Uri", Common_Json_Type_String, "/Record/Replay/LockRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 3, "/Label", Common_Json_Type_String, "LockRecord", 0, 0);
			Common_Json_SetAttrValue(pNode, 3, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, 3, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Replay/RecordList") == 0 && (g_sdcard_errno == 0))
	{
		if(!m_bInit)
			return -1;

		{
			char *pData = Common_Json_Print(pInParams, NULL);
			if(pData)
			{
				LOGI("RecordList req:\n");
				printf("%s\n", pData);
				Common_Free(pData, NULL, 0);
			}
		}

		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			int iRet = -1;
			S32 dev, chn;
			CV_RECORD_QUESTPARA param;
			Common_Json_GetAttrValue(pInParams, -1, "Data/Device", NULL, NULL, &dev, 0);
			Common_Json_GetAttrValue(pInParams, -1, "Data/Channel", NULL, NULL, &chn, 0);
			param.Channel = cv_cfgm_get_channel(dev, chn);
			Common_Json_GetAttrValue(pInParams, -1, "Data/StreamMask", NULL, NULL, (S32*)&param.StreamMask, NULL);
			Common_Json_GetAttrValue(pInParams, -1, "Data/QueryMode", NULL, NULL, (S32*)&param.QueryMode, NULL);
			S8 *strtime;
			Common_Json_GetAttrValue(pInParams, -1, "Data/StartTime", NULL, &strtime, NULL, NULL);
			if(strtime == NULL)
			{
				LOGE("error\n");
				return -1;
			}
			iRet = atotime(strtime, &param.StartTime);
			if(iRet != 0)
			{
				LOGE("error[StartTime:%s]\n",strtime);
				return -1;
			}
			Common_Json_GetAttrValue(pInParams, -1, "Data/StopTime", NULL, &strtime, NULL, NULL);
			if(strtime == NULL)
			{
				LOGE("error\n");
				return -1;
			}
			iRet= atotime(strtime, &param.StopTime);
			if(iRet != 0)
			{
				LOGE("error[StopTime:%s]\n",strtime);
				return -1;
			}
			U32 handle;
			S64 DataSize = 0;
			S32 iHDataSize = 0,iLDataSize = 0;;

			if(m_iQueryCnt >= MAX_QUERY_COUNT)
			{
				LOGE("Max query count is %d\n",m_iQueryCnt);
				return -1;
			}
			m_iQueryCnt++;

			if(CV_SUCCESS != cv_replay_querycreate(&handle, param))
			{
				if(m_iQueryCnt > 0)
					m_iQueryCnt--;
				LOGE("error,m_iQueryCnt[%d]\n",m_iQueryCnt);
				return -1;
			}

			cv_replay_getdatasize(handle, param, &DataSize);
			iHDataSize = (DataSize>>32)&0xffffffff;
			iLDataSize = DataSize&0xffffffff;
			Common_Json_SetAttrValue(pChild, -1, "DataSizeH", Common_Json_Type_Number, NULL, iHDataSize, 0);
			Common_Json_SetAttrValue(pChild, -1, "DataSizeL", Common_Json_Type_Number, NULL, iLDataSize, 0);
			DataSize = 0;
			cv_replay_getiframecount(handle, param, &DataSize);
			iHDataSize = (DataSize>>32)&0xffffffff;
			iLDataSize = DataSize&0xffffffff;
			Common_Json_SetAttrValue(pChild, -1, "IFrameCountH", Common_Json_Type_Number, NULL, iHDataSize, 0);
			Common_Json_SetAttrValue(pChild, -1, "IFrameCountL", Common_Json_Type_Number, NULL, iLDataSize, 0);
			pNode = Common_Json_SetAttrValue(pChild, -1, "Segments", Common_Json_Type_Array, NULL, 0, 0);

			S32 tzoffset = Common_GetTimeDiff()/60;
			S32 i = 0, ret = 0;
			do
			{
				CV_RECORD_SEGDATA	data;
				ret = cv_replay_querynext(handle, &data);
				if(ret == CV_SUCCESS)
				{
					S8 strtmp[128];
					sprintf(strtmp, "%04d%02d%02d%02d%02d%02d", data.StartTime.year, data.StartTime.month, data.StartTime.day, data.StartTime.hour, data.StartTime.min, data.StartTime.sec);
					Common_Json_SetAttrValue(pNode, i, "starttime", Common_Json_Type_String, strtmp, 0, 0);
					sprintf(strtmp, "%04d%02d%02d%02d%02d%02d", data.StopTime.year, data.StopTime.month, data.StopTime.day, data.StopTime.hour, data.StopTime.min, data.StopTime.sec);
					Common_Json_SetAttrValue(pNode, i, "stoptime", Common_Json_Type_String, strtmp, 0, 0);
					Common_Json_SetAttrValue(pNode, i, "TZOffset", Common_Json_Type_Number, NULL, tzoffset, 0);
					Common_Json_SetAttrValue(pNode, i, "key", Common_Json_Type_Number, NULL, data.key, 0);
					Common_Json_SetAttrValue(pNode, i, "streamtype", Common_Json_Type_Number, NULL, 0, 0);
					Common_Json_SetAttrValue(pNode, i, "DataType", Common_Json_Type_Number, NULL, data.FileType, 0);
					Common_Json_SetAttrValue(pNode, i, "IsLocked", Common_Json_Type_Number, NULL, data.blocked, 0);
					Common_Json_SetAttrValue(pNode, i, "DataSize", Common_Json_Type_Number, NULL, data.FileSize, 0);
					i ++;
				}
			}while(ret == CV_SUCCESS);
			cv_replay_queryclose(handle);
			if(m_iQueryCnt > 0)
				m_iQueryCnt--;
			if((m_bPrintDbg&0x01))
				LOGD("Query succ!m_iQueryCnt[%d][%d]\n",m_iQueryCnt,i);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}

		{
			char *pData = Common_Json_Print(pOut, NULL);
			if(pData)
			{
				LOGI("RecordList res:\n");
				printf("%s\n", pData);
				Common_Free(pData, NULL, 0);
			}
		}

	}
	else if(Common_StriCmp(str, (S8*)"/Record/Replay/RecordListByMonthly") == 0 && (g_sdcard_errno == 0))
	{
		if(!m_bInit)
			return -1;
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			int iRet = -1;
			S8 *strtime = NULL;
			Common_Time_T SerchTime;
			Common_Json_GetAttrValue(pInParams, -1, "Data/Month", NULL, &strtime, NULL, NULL);
			if(strtime == NULL)
			{
				LOGE("error\n");
				return -1;
			}
			iRet = atotime(strtime, &SerchTime);
			if(iRet != 0)
			{
				LOGE("error[Month:%s]\n",strtime);
				return -1;
			}
			U32 result = 0;
			if(CV_SUCCESS != cv_replay_querybymonthly(SerchTime.year, SerchTime.month, &result))
			{
				return -1;
			}

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/Day", Common_Json_Type_Array, NULL, 0, 0);
			S32 day = 0;
			for(S32 i = 0; i < 31; i ++)
			{
				if(result & (0x01 << i))
				{
					Common_Json_SetAttrValue(pNode, day, "Day", Common_Json_Type_Number, NULL, i + 1, 0);
					day ++;
				}
			}
			Common_Json_SetAttrValue(pOut, -1, "DayNum", Common_Json_Type_Number, NULL, day, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Replay/HistoryStream") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/AddressString", Common_Json_Type_String, (S8*)"/Record/Replay/HistoryStream", 0, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}

	}
	else if(Common_StriCmp(str, (S8*)"/Record/Replay/LockRecord") == 0 && (g_sdcard_errno == 0))
	{
		if(!m_bInit)
			return -1;
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Put") == 0)
		{
			S32 iRet;
			S8 *strtime = NULL;
			time_t tBeginTime,tEndTime;
			Common_Time_T t_start,t_end;
			int dev = 0,chn = 0,bLock = 0,iChannel = 0;

			Common_Json_GetAttrValue(pInParams, -1, "Data/Device", NULL, NULL, &dev, 0);
			Common_Json_GetAttrValue(pInParams, -1, "Data/Channel", NULL, NULL, &chn, 0);
			iChannel = cv_cfgm_get_channel(dev, chn);
			Common_Json_GetAttrValue(pInParams, -1, "Data/StartTime", NULL, &strtime, NULL, NULL);
			if(strtime == NULL)
			{
				LOGE("error\n");
				return -1;
			}
			iRet = atotime(strtime, &t_start);
			if(iRet != 0)
			{
				LOGE("error[StartTime:%s]\n",strtime);
				return -1;
			}
			Common_Json_GetAttrValue(pInParams, -1, "Data/StopTime", NULL, &strtime, NULL, NULL);
			if(strtime == NULL)
			{
				LOGE("error\n");
				return -1;
			}
			iRet = atotime(strtime, &t_end);
			if(iRet != 0)
			{
				LOGE("error[StopTime:%s]\n",strtime);
				return -1;
			}
			//LOGE("Lock recfile[%04d-%02d-%02d %02d:%02d:%02d   -  %04d-%02d-%02d %02d:%02d:%02d]\n",t_start.year,t_start.month,t_start.day,t_start.hour,t_start.min,t_start.sec,t_end.year,t_end.month,t_end.day,t_end.hour,t_end.min,t_end.sec);
			Common_Json_GetAttrValue(pInParams, -1, "Data/LockFlag", NULL, NULL, &bLock, NULL);
			Common_CommonGm2LinuxTime(&t_start, &tBeginTime);
			Common_CommonGm2LinuxTime(&t_end, &tEndTime);
			iRet = cv_diskm_lockrecord(iChannel, tBeginTime, tEndTime, bLock);
			if(iRet != 0)
			{
				LOGE("error[%d]\n",iRet);
				return -1;
			}
			LOGW("succ\n");
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/DiskManage") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);

			U32 DiskNum = cv_diskm_getdisknum();
			U32 i = 0;
			for(i = 0; i < DiskNum; i ++)
			{
				S8 strtmp[128];
				sprintf(strtmp, "/Record/DiskManage/Disk%d", i);
				Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String, strtmp, 0, 0);
				sprintf(strtmp, "Disk%d", i);
				Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, strtmp, 0, 0);
				Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None", 0, 0);
				pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
				Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
			}

			Common_Json_SetAttrValue(pNode, i, "/Uri", Common_Json_Type_String, "/Record/DiskManage/SubScribe", 0, 0);
			Common_Json_SetAttrValue(pNode, i, "/Label", Common_Json_Type_String, "SubScribe", 0, 0);
			Common_Json_SetAttrValue(pNode, i, "/Describe", Common_Json_Type_String, "None", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, i, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
			i++;
			Common_Json_SetAttrValue(pNode, i, "/Uri", Common_Json_Type_String, "/Record/DiskManage/FSInfo", 0, 0);
			Common_Json_SetAttrValue(pNode, i, "/Label", Common_Json_Type_String, "FSInfo", 0, 0);
			Common_Json_SetAttrValue(pNode, i, "/Describe", Common_Json_Type_String, "FSInfo", 0, 0);
			pNode1 = Common_Json_SetAttrValue(pNode, i, "/Method", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StrniCmp(str, (S8*)"/Record/DiskManage/Disk", 23) == 0 && (g_sdcard_errno == 0))
	{
		U32 DiskNum = cv_diskm_getdisknum();
		for(U32 i = 0; i < DiskNum; i ++)
		{
			S8 strchn[128] = {0};
			S8 strchn1[128] = {0};
			S8 strchn2[128] = {0};
			sprintf(strchn, "/Record/DiskManage/Disk%d", i);
			sprintf(strchn1, "/Record/DiskManage/Disk%d/Attribute", i);
			sprintf(strchn2, "/Record/DiskManage/Disk%d/Format", i);
			if(Common_StriCmp(str, strchn) == 0)
			{
				Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
				if(Common_StriCmp(str, (S8*)"Get") == 0)
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

					pNode = Common_Json_SetAttrValue(pOut, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);

					Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String, strchn1, 0, 0);
					Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "Attribute", 0, 0);
					Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None", 0, 0);
					pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array, NULL, 0, 0);
					Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);

					Common_Json_SetAttrValue(pNode, 1, "/Uri", Common_Json_Type_String, strchn2, 0, 0);
					Common_Json_SetAttrValue(pNode, 1, "/Label", Common_Json_Type_String, "Format", 0, 0);
					Common_Json_SetAttrValue(pNode, 1, "/Describe", Common_Json_Type_String, "None", 0, 0);
					pNode1 = Common_Json_SetAttrValue(pNode, 1, "/Method", Common_Json_Type_Array, NULL, 0, 0);
					Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Put", 0, 0);
				}
				else
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
					return -1;
				}
			}
			else if(Common_StriCmp(str, strchn1) == 0)
			{
				Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
				if(Common_StriCmp(str, (S8*)"Get") == 0)
				{

                    RECORD_CUSTOM sRecCustom = {0};
                    cv_cfgm_get_record_custom(&sRecCustom);

					S8 nodepath[128] = {0};
					S32 partitionnum = 0;
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

					cv_diskm_getdisknode(i, nodepath);
					Common_Json_SetAttrValue(pOut, -1, "Data/NodePath", Common_Json_Type_String, nodepath, 0, 0);

					cv_diskm_getdiskPartitionNum(i, &partitionnum);
					Common_Json_SetAttrValue(pOut, -1, "Data/PartitionNum", Common_Json_Type_Number, NULL, partitionnum, 0);
					pNode = Common_Json_SetAttrValue(pOut, -1, "Data/Partition", Common_Json_Type_Array, NULL, 0, 0);
					for(S32 j = 0; j < partitionnum; j ++)
					{
						strPartitionAttr attr = {0};
						cv_diskm_getdiskPartition(i, j, &attr);
						Common_Json_SetAttrValue(pNode, j, "NodePath", Common_Json_Type_String, attr.nodepath, 0, 0);
						Common_Json_SetAttrValue(pNode, j, "MountPath", Common_Json_Type_String, attr.mountpath, 0, 0);
						Common_Json_SetAttrValue(pNode, j, "PartitionType", Common_Json_Type_String, attr.describe, 0, 0);
						S8 strspace[128];
						sprintf(strspace, "%dMB", attr.totalspace);
						Common_Json_SetAttrValue(pNode, j, "TotalSpace", Common_Json_Type_String, strspace, 0, 0);
						sprintf(strspace, "%dMB", attr.freespace);
						Common_Json_SetAttrValue(pNode, j, "FreeSpace", Common_Json_Type_String, strspace, 0, 0);


                        if(Common_StriCmp(attr.describe, (char *)"Record") == 0)
                        {
                            if(sRecCustom.SDCapacity>0)
                            {
                                sprintf(strspace, "%dMB", sRecCustom.SDCapacity);
                                Common_Json_SetAttrValue(pNode, j, "TotalSpace", Common_Json_Type_String, strspace, 0, 0);
                            }

                            if(strlen(sRecCustom.SDRecordableDays)>0)
                            {
                                Common_Json_SetAttrValue(pNode, j, "SDRecordableDays", Common_Json_Type_String, sRecCustom.SDRecordableDays, 0, 0);
                            }
                            else
                            {
                                int max_size = 12000;
                                int min_size = 5000;
#if 0
                                cv_record_arming_day dayplan;
                                cv_cfgm_get_plan_day(0, 0, &dayplan);
                                if(dayplan.arming[0].arming_type == 2)
                                {
                                    max_size = 1000;
                                    min_size = 1000;
                                }
                                else if(dayplan.arming[0].arming_type == 3)
                                {
                                    max_size = 4000;
                                    min_size = 3000;
                                }
#endif
                                int min_day = round(attr.totalspace*1.0/max_size);
                                if(min_day == 0)min_day = 1;

                                int max_day = round(attr.totalspace*1.0/min_size);
                                if(max_day == 0)max_day = 1;

                                if(min_day == max_day)
                                {

                                    snprintf(strspace,sizeof(strspace),"%d",min_day);
                                }
                                else
                                {
                                    snprintf(strspace,sizeof(strspace),"%d-%d",min_day,max_day);
                                }

                                Common_Json_SetAttrValue(pNode, j, "SDRecordableDays", Common_Json_Type_String, strspace, 0, 0);
                            }
                        }
					}
				}
				else
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
					return -1;
				}
			}
			else if(Common_StriCmp(str, strchn2) == 0)
			{
				S32 iPartNo = 1;
				if(!m_bInit)
					return -1;
				Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Data/PartitionId", NULL, NULL, &iPartNo, NULL);
				if(Common_StriCmp(str, (S8*)"Put") == 0)
				{
					S32 iRet = -1;
					cv_record_stop_work(0);
					iRet = cv_diskm_format(i,iPartNo);
					cv_record_resume_work();
					if(0 == iRet)
					{
						Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
						Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
					}
					else
					{
						Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
						Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
						return -1;
					}
				}
				else
				{
					Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
					Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
					return -1;
				}
			}
			else
			{
				Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
				Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
				return -1;
			}
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/DiskManage/SubScribe") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			pNode = Common_Json_SetAttrValue(pOut, -1, "Data/SubScribeAddress", Common_Json_Type_Array, NULL, 0, 0);
			Common_Json_SetAttrValue(pNode, 0, "AddressString", Common_Json_Type_String, (S8*)"/Record/DiskManage/SubScribe/NoDisk", 0, 0);
			Common_Json_SetAttrValue(pNode, 1, "AddressString", Common_Json_Type_String, (S8*)"/Record/DiskManage/SubScribe/DiskFull", 0, 0);
		}
		else
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/DiskManage/FSInfo") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Data/FSSupport", Common_Json_Type_String, "MKV", 0, 0);
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/Status") == 0)
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);

            GetSDCardStatus(pOut);
		}
	}
	else if(Common_StriCmp(str, (S8*)"/Record/DiskTest") == 0 && (g_sdcard_errno == 0))
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &str, NULL, NULL);
		if(Common_StriCmp(str, (S8*)"Get") == 0)
		{
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);

			char temp[32] = "";
			snprintf(temp, sizeof(temp), "Avg:%dKB/s Min:%dKB/s", g_diskBitrate, g_minDiskBitrate);
			Common_Json_SetAttrValue(pOut, -1, "Data/Bitrate", Common_Json_Type_String, temp, 0, 0);
		}
		else
		{
			S8 *temp = NULL;
			Common_Json_GetAttrValue(pInParams, -1, "Data/Type", NULL, &temp, NULL, NULL);
			LOGI("temp=%s\n", temp);
			if(Common_StriCmp(temp, (S8*)"Start") == 0)
			{
				g_bTestDisk = 1;
				pthread_attr_t attr;
			    pthread_attr_init(&attr);
			    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
				pthread_create(&g_pThreadTestDisk,&attr,TestDisk,NULL);
			}
			else
			{
				g_minDiskBitrate = 0;
				g_bTestDisk = 0;
				pthread_join(g_pThreadTestDisk, NULL);
			}
			Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
			Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "SUCCESS", 0, 0);
		}
	}
	else
	{
		Common_Json_SetAttrValue(pOut, -1, "Header/Code", Common_Json_Type_Number, NULL, -1, 0);
		Common_Json_SetAttrValue(pOut, -1, "Header/Decribe", Common_Json_Type_String, "ERROR", 0, 0);
		return -1;
	}
	return 0;
}

S32 HistoryStreamPop(S32 StreamFd, U32 dwChannel,U32 dwStreamIdx,U32 bIFrame, U8 *byBuffer, U32 dwSize, U32 *Exit, void* lpUser)
{
	ReplayHandleMap::iterator	iter;
	S32 lRet;

	Common_RWLock_RLock(StreamLock);

	iter = CV_ReplayHandle.find(StreamFd);
	if(iter == CV_ReplayHandle.end())
	{
		LOGE("lFindHandle err[StreamFd:%d]\n",StreamFd);
		Common_RWLock_UnLock(StreamLock);
		return -1;
	}

	Common_RWLock_UnLock(StreamLock);

	do
	{
		if((m_bPrintDbg&0x01))
		{
			Ovfs_FrameHeader_T *head = (Ovfs_FrameHeader_T *)byBuffer;
			if(head && (head->uiFrameType == CvPktIFrames ||head->uiFrameType == CvPktPFrames ||head->uiFrameType == CvPktAudioFrames ||head->uiFrameType == CvPktSmartIFrames ||head->uiFrameType == CvPktSmartPFrames))
			{
				S8 szTime[32] = {0};
				Common_Time_T t_time;
				Common_Linux2CommonGmTime(head->uiFrameTime, &t_time);
				sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
				#if 0
				static U64 u64LstTime = 0,iLstFrameMSec = 0;
				U64 u64CurTime = 0,iCurFrameMSec = 0;
				S64 delayMs = 0;

				u64CurTime = Common_GetSystemCount64();
				iCurFrameMSec = head->uiFrameTime*1000 + head->uiFrameTickCount;
				if(head->uiFrameType == CvPktIFrames)
				{
					if(iLstFrameMSec != 0)
					{
						delayMs = iCurFrameMSec-iLstFrameMSec - (u64CurTime-u64LstTime);
						if(delayMs > 0)
						{
							Common_Sleep(0, delayMs*1000);
						}
					}
					LOGW("[%d.%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d\n",head->uiFrameTime,head->uiFrameTickCount,szTime,head->uiFrameNo,head->uiFrameType,head->uiFrameLen);
					LOGE("delayMs %lld iCurFrameMSec-iLstFrameMSec=%llu u64CurTime-u64LstTime=%llu \n",delayMs,iCurFrameMSec-iLstFrameMSec,u64CurTime-u64LstTime);
					u64LstTime = u64CurTime;
					iLstFrameMSec = iCurFrameMSec;
				}
				#endif
				#if 1
				static U32 uiLstFrameNo = 0;
				//if(uiLstFrameNo&&head->uiFrameNo-uiLstFrameNo != 1)
				{
					LOGW("[%d.%06d-%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d,uiLstFrameNo:%d\n",head->uiFrameTime,head->uiFrameTickCount,head->dwTimeStamp,szTime,head->uiFrameNo,head->uiFrameType,head->uiFrameLen,uiLstFrameNo);
				}
				uiLstFrameNo = head->uiFrameNo;
				#endif

				//LOGE("[%d.%d]uiFrameTime:%s,uiFrameNo:%d,uiFrameType:%d,uiFrameLen:%d\n",head->uiFrameTime,head->uiFrameTickCount,szTime,head->uiFrameNo,head->uiFrameType,head->uiFrameLen);
			}
		}

		lRet = Module_StreamQueue_WriteData(m_gRecordInfo.ModeHandle, StreamFd, 0, NULL, byBuffer, dwSize, NULL, 0);
		if(lRet!= MODULE_ERROR_TYPE_SUCC)
		{
			Common_Sleep(0, 10000);
		}
#if 0
		{
			Ovfs_FrameHeader_T *head = (Ovfs_FrameHeader_T *)byBuffer;
			if(head && (head->uiFrameType == CvPktIFrames ||head->uiFrameType == CvPktPFrames ||head->uiFrameType == CvPktAudioFrames ||head->uiFrameType == CvPktSmartIFrames ||head->uiFrameType == CvPktSmartPFrames))
			{
				static unsigned int nWriteCount = 0;
				//if(head->uiFrameNo == 0 || nWriteCount % 25 == 0)
					LOGW("Module_StreamQueue_WriteData lRet=[%d]. uiFrameType=[%u], uiFrameNo=[%u], uiFrameLen=[%u]\n", lRet, head->uiFrameType, head->uiFrameNo, head->uiFrameLen);
				nWriteCount++;
			}
		}
#endif
	}while(lRet != MODULE_ERROR_TYPE_SUCC && *Exit == 0);

	return CV_SUCCESS;
}

S32 ReplayOpen(ModuleHandle_T hModuleHandle,S32 *pSourceFd,S32 nRemoteFd,S8 *szUri,cJSON_Struct *pOpenParams,cJSON_Struct **pStreamInfo,void **pUserDataForRemoteFd,void *pUserData)
{
	LOGD("m_bPrintDbg %d\n",m_bPrintDbg);
	if(Common_StriCmp(szUri, (S8*)"/Record/Replay/HistoryStream") != 0)
		return -1;
	if(pOpenParams == NULL)
	{
		LOGE("param err\n");
		return -1;
	}
	
	//if((m_bPrintDbg&0x01) && pOpenParams)
	{
		LOGI("szUri=[%s]\n",szUri);
		OVFS_PRINT_JSON(pOpenParams);
	}

    int maxStreamNum = cv_cfgm_get_historystream_maxnum();
    if(maxStreamNum != 0 && (int)CV_ReplayHandle.size() >= maxStreamNum)
    {
        LOGE("record has max connect[%d]!\n",maxStreamNum);
        return -2;
    }
	S32 ChannelNum;
	U32 Channel[CV_MAX_LOCAL_CH_NUM];
	cJSON_Struct *pNode = Common_Json_GetItem(pOpenParams, -1, "ReplayCh");
	ChannelNum = Common_Json_Size(pNode);
	for(S32 i = 0; i < ChannelNum; i ++)
	{
		S32 videv, vichn;
		Common_Json_GetAttrValue(pNode, i, "Device", NULL, NULL, &videv, NULL);
		Common_Json_GetAttrValue(pNode, i, "Channel", NULL, NULL, &vichn, NULL);
		Channel[i] = cv_cfgm_get_channel(videv, vichn);
		if(Channel[i] < 0)
		{
			OVFS_PRINT_JSON(pOpenParams);
			return -1;
		}
	}
	S8 *str;
	int iRet = -1;
	Common_Time_T commontime;
	time_t tStartTime, tEndTime;
	Common_Json_GetAttrValue(pOpenParams, -1, "StartTime", NULL, &str, NULL, NULL);
	if(str == NULL)
	{
		LOGE("error\n");
		return -1;
	}
	//LOGD("StartTime = %s\n", str);
	iRet = atotime(str, &commontime);
	if(iRet != 0)
	{
		LOGE("error[StartTime:%s]\n",str);
		return -1;
	}
	Common_CommonGm2LinuxTime(&commontime, &tStartTime);
	Common_Json_GetAttrValue(pOpenParams, -1, "EndTime", NULL, &str, NULL, NULL);
	if(str == NULL)
	{
		LOGE("error\n");
		return -1;
	}
	//LOGD("EndTime = %s\n", str);
	iRet = atotime(str, &commontime);
	if(iRet != 0)
	{
		LOGE("error[EndTime:%s]\n",str);
		return -1;
	}
	Common_CommonGm2LinuxTime(&commontime, &tEndTime);
	S32 StreamType;
	Common_Json_GetAttrValue(pOpenParams, -1, "StreamType", NULL, NULL, &StreamType, NULL);
	S32 lRecordType = 0;
	Common_Json_GetAttrValue(pOpenParams, -1, "Mode", NULL, NULL, &lRecordType, NULL);

	cJSON_Struct *pInparam = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	Common_Json_SetAttrValue(pInparam, -1, "QueueType", Common_Json_Type_Number, NULL, 1, 0);
	Common_Json_SetAttrValue(pInparam, -1, "Mode", Common_Json_Type_Number, NULL, 1, 0);
	Common_Json_SetAttrValue(pInparam, -1, "MaxMemSize", Common_Json_Type_Number, NULL, 2*1024*1024, 0);
	Common_Json_SetAttrValue(pInparam, -1, "MaxMemNum", Common_Json_Type_Number, NULL, 10, 0);
	Common_Json_SetAttrValue(pInparam, -1, "HasEliminated", Common_Json_Type_Number, NULL, 0, 0);
	S32 fd = Module_StreamQueue_Create(m_gRecordInfo.ModeHandle, NULL, pInparam, NULL, NULL, NULL);
	Common_Json_Delete(pInparam);
	if(fd <= 0)
	{
		LOGE("Module_StreamQueue_Create failed,fd:%d\n",fd);
		return -1;
	}
	*pSourceFd = fd;

	U32 Handle;
	S32 ret = cv_replay_create(&Handle, ChannelNum, Channel, tStartTime, tEndTime, HistoryStreamPop, fd, StreamType, lRecordType, NULL);
	if(ret != CV_SUCCESS)
	{
		LOGE("fd:%d,ret:%d\n",ret);
		Module_StreamQueue_Destroy(m_gRecordInfo.ModeHandle, fd);
		return -1;
	}
	S64 s64DataSize = 0;
	S32 iHDataSize = 0,iLDataSize = 0;

	cv_replay_getpopdatesize(Handle,&s64DataSize);
	iHDataSize = (s64DataSize>>32)&0xffffffff;
	iLDataSize = s64DataSize&0xffffffff;
	if(pStreamInfo && (NULL == *pStreamInfo))
	{
		*pStreamInfo = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
		Common_Json_SetAttrValue(*pStreamInfo, -1, "PopDataSizeH", Common_Json_Type_Number, NULL, iHDataSize, 0);
		Common_Json_SetAttrValue(*pStreamInfo, -1, "PopDataSizeL", Common_Json_Type_Number, NULL, iLDataSize, 0);
		if((m_bPrintDbg&0x01))
		{
			OVFS_PRINT_JSON(*pStreamInfo);
		}
	}

	CV_ReplayHandle.insert(pair<S32, U32>(fd, Handle));
	LOGI("succ!fd:%d,Handle:%u\n",fd,Handle);
	return CV_SUCCESS;
}

S32 ReplayCtrl(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,cJSON_Struct *pControlParams,cJSON_Struct **pOutParams,void *pUserDataForRemoteFd,void *pUserData)
{
	ReplayHandleMap::iterator	iter;
	U32	handle;

	//if((m_bPrintDbg&0x01) && pControlParams)
	{
		LOGI("nSourceFd=[%d]\n",nSourceFd);
		OVFS_PRINT_JSON(pControlParams);
	}
	Common_RWLock_RLock(StreamLock);

	iter = CV_ReplayHandle.find(nSourceFd);
	if(iter == CV_ReplayHandle.end())
	{
		LOGE("lFindHandle err\n");
		Common_RWLock_UnLock(StreamLock);
		return -1;
	}
	handle = iter->second;

	Common_RWLock_UnLock(StreamLock);

	S8* str;
	Common_Json_GetAttrValue(pControlParams, -1, "operate", NULL, &str, NULL, NULL);
	if(Common_StriCmp(str, (S8 *)"start") == 0)
	{
		S64 s64DataSize = 0;
		S32 iHDataSize = 0,iLDataSize = 0;

		cv_replay_getpopdatesize(handle,&s64DataSize);
		iHDataSize = (s64DataSize>>32)&0xffffffff;
		iLDataSize = s64DataSize&0xffffffff;
		if(pOutParams && (NULL == *pOutParams))
		{
			*pOutParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
			Common_Json_SetAttrValue(*pOutParams, -1, "PopDataSizeH", Common_Json_Type_Number, NULL, iHDataSize, 0);
			Common_Json_SetAttrValue(*pOutParams, -1, "PopDataSizeL", Common_Json_Type_Number, NULL, iLDataSize, 0);
			if(m_bPrintDbg)
			{
				OVFS_PRINT_JSON(*pOutParams);
			}
		}

		if(CV_SUCCESS != cv_replay_start(handle))
		{
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8 *)"stop") == 0)
	{
		if(CV_SUCCESS != cv_replay_stop(handle))
		{
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8 *)"seek")== 0)
	{
		Common_Json_GetAttrValue(pControlParams, -1, "param", NULL, &str, NULL, NULL);
		Common_Time_T time;
		time_t tSeekTime;
		int iRet = -1;
		if(str == NULL)
		{
			LOGE("error\n");
			return -1;
		}
		iRet = atotime(str, &time);
		if(iRet != 0)
		{
			LOGE("error[str:%s]\n",str);
			return -1;
		}
		Common_CommonGm2LinuxTime(&time, &tSeekTime);
		if(CV_SUCCESS != cv_replay_seek(handle, tSeekTime))
		{
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8 *)"setdir")== 0)
	{
		S32 dir;
		Common_Json_GetAttrValue(pControlParams, -1, "param", NULL, NULL, &dir, NULL);
		if(CV_SUCCESS != cv_replay_setdir(handle, dir))
		{
			return -1;
		}
	}
	else if(Common_StriCmp(str, (S8 *)"forcekey")== 0)
	{
		S32 forceI;
		Common_Json_GetAttrValue(pControlParams, -1, "param", NULL, NULL, &forceI, NULL);
		if(CV_SUCCESS != cv_replay_setpoptype(handle, forceI))
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
	return CV_SUCCESS;
}

S32 ReplayClose(ModuleHandle_T hModuleHandle,S32 nSourceFd,S32 nRemoteFd,cJSON_Struct *pCloseParams,cJSON_Struct **pOutParams,void *pUserDataForRemoteFd,void *pUserData)
{
	ReplayHandleMap::iterator	iter;
	U32	handle;
	
	//if((m_bPrintDbg&0x01) && pCloseParams)
	{
		LOGI("nSourceFd=[%d]\n",nSourceFd);
		OVFS_PRINT_JSON(pCloseParams);
	}
	Common_RWLock_WLock(StreamLock);

	iter = CV_ReplayHandle.find(nSourceFd);
	if(iter == CV_ReplayHandle.end())
	{
		LOGE("lFindHandle err\n");
		Common_RWLock_UnLock(StreamLock);
		return -1;
	}
	handle = iter->second;

	CV_ReplayHandle.erase(iter);

	Common_RWLock_UnLock(StreamLock);

	cv_replay_stop(handle);
	cv_replay_release(handle);
	Module_StreamQueue_Destroy(m_gRecordInfo.ModeHandle, nSourceFd);
	LOGD("succ!\n");
	return CV_SUCCESS;
}

S32 RecvSubscribeData(ModuleHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
	if(pEventInfo)
	{
		U32 channel,mask;
		S32 bAudioEn = 1;
		cv_device_capability *psyscap = NULL;
		psyscap = cv_cfgm_get_dev_cap();
		if((m_bPrintDbg&0x01))
		{
			OVFS_PRINT_JSON(pEventInfo);
		}
		Common_Json_GetAttrValue(pEventInfo, -1, "AudioEnable", NULL, NULL, &bAudioEn, NULL);
		if(m_bEnAudio == bAudioEn)
			return 0;
		else
			m_bEnAudio = bAudioEn;
		for(U32 i = 0; i < psyscap->video_vi_num; i ++){
			for(U32 k = 0; k < psyscap->video_vi_chn[i]; k ++)
			{
				channel = cv_cfgm_get_channel(i, k);
				cv_record_get_record_mode(channel,NULL,&mask,NULL);
				if(bAudioEn)
					cv_record_set_record_manualmask(channel, mask|0x04);
				else
					cv_record_set_record_manualmask(channel, mask&0xfffffffb);
				for(U32 j = 0; j < 7; j ++)
				{
					cv_record_arming_day dayplan[7];
					cv_cfgm_get_plan_day(channel, j, &dayplan[j]);
					for(U32 n = 0; n < CV_MAX_ARMING_TIME_QUANTUM; n ++)
					{
						if(bAudioEn)
							dayplan[j].arming[n].recordmask = dayplan[j].arming[n].recordmask|0x04;
						else
							dayplan[j].arming[n].recordmask = dayplan[j].arming[n].recordmask&0xfffffffb;
					}
					cv_record_set_rec_plan_day(channel, j, &dayplan[j]);
					cv_cfgm_set_plan_day(channel, j, dayplan[j]);
				}
				cv_record_saveconfig();
			}
		}
	}
	return 0;
}

S32 DiskAlarmSubscribe(ModuleHandle_T hModuleHandle, S32 nType,S32 nRecvID, S8 *szSubscribeUri, cJSON_Struct **pQueryEventInfo, void *pUserData)
{
	if(nType == 0)
	{
		if(Common_StriCmp(szSubscribeUri, (S8*)"/Record/DiskManage/SubScribe/NoDisk") == 0)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uNoDiskRecvID[i] <= 0)
				{
					uNoDiskRecvID[i] = nRecvID;
					break;
				}
			}
		}
		else if(Common_StriCmp(szSubscribeUri, (S8*)"/Record/DiskManage/SubScribe/DiskFull") == 0)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uDiskFullRecvID[i] <= 0)
					uDiskFullRecvID[i] = nRecvID;
			}
		}
		else
		{
			return -1;
		}
	}
	else if(nType == 1)
	{
		if(Common_StriCmp(szSubscribeUri, (S8*)"/Record/DiskManage/SubScribe/NoDisk") == 0)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uNoDiskRecvID[i] == nRecvID)
					uNoDiskRecvID[i] = 0;
			}
		}
		else if(Common_StriCmp(szSubscribeUri, (S8*)"/Record/DiskManage/SubScribe/DiskFull") == 0)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uDiskFullRecvID[i] == nRecvID)
					uDiskFullRecvID[i] = 0;
			}
		}
	}
	else if(nType == 2)
	{
		if(Common_StriCmp(szSubscribeUri, (S8*)"/Record/DiskManage/SubScribe/NoDisk") == 0)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uNoDiskRecvID[i] == nRecvID)
				{
					cJSON_Struct *pOut = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
					if(pOut == NULL)
					{
						return -1;
					}
					cJSON_Struct *pNode = Common_Json_SetAttrValue(pOut, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
					Common_Json_SetAttrValue(pNode, 0, "AlarmName", Common_Json_Type_String, (S8*)"NoDisk", 0, 0);
					Common_Json_SetAttrValue(pNode, 0, "IsHappent", Common_Json_Type_Number, NULL, (cv_diskm_getdisknum() <= 0), 0);
					*pQueryEventInfo = pOut;
					break;
				}
			}
		}
		else if(Common_StriCmp(szSubscribeUri, (S8*)"/Record/DiskManage/SubScribe/DiskFull") == 0)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uDiskFullRecvID[i] == nRecvID)
				{
					cJSON_Struct *pOut = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
					if(pOut == NULL)
					{
						return -1;
					}
					cJSON_Struct *pNode = Common_Json_SetAttrValue(pOut, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
					Common_Json_SetAttrValue(pNode, 0, "AlarmName", Common_Json_Type_String, (S8*)"DiskFull", 0, 0);
					Common_Json_SetAttrValue(pNode, 0, "IsHappent", Common_Json_Type_Number, NULL, cv_diskm_getrecorddiskfull(), 0);
					*pQueryEventInfo = pOut;
					break;
				}
			}
		}
	}
	else
	{
		return -1;
	}

	return CV_SUCCESS;
}

void UploadEvent(S32 AlarmType, S32 AlarmStatus)
{
	static int bLstDiskFull = 0;
	static int bLstNoSD = 0;
	if(AlarmType == 0)
	{
		int bNoSD = (cv_diskm_getdisknum() <=0);
		if(bNoSD != bLstNoSD)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uNoDiskRecvID[i] != 0)
				{
					cJSON_Struct *pOut = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
					if(pOut != NULL)
					{
						cJSON_Struct *pNode = Common_Json_SetAttrValue(pOut, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
						Common_Json_SetAttrValue(pNode, 0, "AlarmName", Common_Json_Type_String, (S8*)"NoDisk", 0, 0);
						Common_Json_SetAttrValue(pNode, 0, "IsHappent", Common_Json_Type_Number, NULL, bNoSD, 0);
						Module_SendEvent(m_gRecordInfo.ModeHandle, uNoDiskRecvID[i], pOut, NULL, 100);
						Common_Json_Delete(pOut);
					}
				}
			}
			bLstNoSD = bNoSD;
		}
	}
	else if(AlarmType == 1)
	{
		int bDiskFull = cv_diskm_getrecorddiskfull();
		if(bDiskFull != bLstDiskFull)
		{
			for(S32 i = 0; i < MAX_SUBCRIBESRC; i ++)
			{
				if(uDiskFullRecvID[i] != 0)
				{
					cJSON_Struct *pOut = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
					if(pOut != NULL)
					{
						cJSON_Struct *pNode = Common_Json_SetAttrValue(pOut, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
						Common_Json_SetAttrValue(pNode, 0, "AlarmName", Common_Json_Type_String, (S8*)"DiskFull", 0, 0);
						Common_Json_SetAttrValue(pNode, 0, "IsHappent", Common_Json_Type_Number, NULL, bDiskFull, 0);
						Module_SendEvent(m_gRecordInfo.ModeHandle, uDiskFullRecvID[i], pOut, NULL, 100);
						Common_Json_Delete(pOut);
					}
				}
			}
			bLstDiskFull = bDiskFull;
		}
	}
}

int main()
{
	cJSON_Struct *pConfig;
	S32 iRetval = 0,iSDNum = 0,iCount = 0,iPlayAudio = 0;

    //signal(SIGINT,SignalHandle);
	Common_RegistSigHandle(SIGSEGV);
	//RegistSigHandle(SIGSEGV);

    LOG_INIT((S8*)"Record", COMMON_LOG_LV_HIGH);

	pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
	if (pConfig != NULL)
	{
		Common_Json_SetAttrValue(pConfig,-1,"SystemName",Common_Json_Type_String,"ovfs",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"ModuleName",Common_Json_Type_String,"Record",0,0);
	}
	iRetval = Module_Init(&m_gRecordInfo.ModeHandle, pConfig, NULL, ReceiveCmd, NULL);
	Common_Json_Delete(pConfig);
	if(iRetval != MODULE_ERROR_TYPE_SUCC)
	{
		LOGE("Module init %d\n", iRetval);
		goto exit_log;
	}

	check_sd_node_status();
	cv_record_init((void*)m_gRecordInfo.ModeHandle);
	if(g_sdcard_errno == 0)
	{
		iSDNum = cv_diskm_getdisknum();
		while(iSDNum <= 0)
		{
			iCount++;
			if(iCount >= 120)
			{
				iCount = 0;
				LOGE("No SD!\n");
			}
			Common_Sleep(1, 0);
		}

		cv_record_resume_work();
		Common_RWLock_Create(&StreamLock, (S8*)"Stream");
		Module_StreamQueue_Callbacks strcallback;
		strcallback.fOpen = ReplayOpen;
		strcallback.fClose = ReplayClose;
		strcallback.fControl = ReplayCtrl;
		Module_StreamQueue_SetGlobalCallback(m_gRecordInfo.ModeHandle, &strcallback, NULL);

		Module_RegisterSubscribe(m_gRecordInfo.ModeHandle, (S8*)"/Record/DiskManage/SubScribe/DiskFull", DiskAlarmSubscribe, NULL);
		Module_RegisterSubscribe(m_gRecordInfo.ModeHandle, (S8*)"/Record/DiskManage/SubScribe/NoDisk", DiskAlarmSubscribe, NULL);
		Module_SubscribeEvent(m_gRecordInfo.ModeHandle, (S8*)"/BoardSys/Subscribe/Audio/AudioEnable", RecvSubscribeData, NULL);
		cv_diskm_setalarmcallback(UploadEvent);
		m_bInit = 1;
	}

    SendSDCardStatus();

	while(!m_gRecordInfo.bExit)
	{
		if(iPlayAudio == 0)
		{
			Common_Sleep(10, 0);
			if((m_bInit == 1) && (g_sdcard_errno == 0))
			{
				cv_cfgm_play_audio((char *)"/update/soundFile/sd_start_record");
			}
			iPlayAudio = 1;
		}
		Common_Sleep(1, 0);
	}

	Module_Unint(&m_gRecordInfo.ModeHandle);

exit_log:
    LOG_UNINIT();
    return 0;
}

