#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/**********/ 

#ifdef WIN32
#include <Windows.h>

#else
#include <dlfcn.h>
#include <signal.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#endif
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "access_struct.h"
#include "access_rest.h"
#include "libaccess_sdk.h"

/*
/Access/UserCfg   用户信息列表 Get/Put
/Access/BindUser  绑定用户 Get/Put 
/Access/Login     登陆 Post
/Access/KeepAlive 更新 Put
/Access/Logout    注销 Delete
/Access/Online     在线用户状态
/Access/Subscribe/UserCfg[?Auth=xx&AuthTime=xx]
*/
extern int g_iCfgChange;
static S32 g_bInited = 0;
static Common_Lock_T g_tRestLock;
//static OVFS_COMMON_LIST_T g_subscribeList;
static ModuleHandle_T g_hModuleHandle = NULL;
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
#elif (defined (__arm__) )
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
    if (signo == SIGPIPE)
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
static int AnalyzeDataAndMakeResult(cJSON_Struct *dataJson,cJSON_Struct **pOutData)
{
	int iRet = -1;
	char *pUri = NULL,*pMethod = NULL;
	cJSON_Struct *pData = NULL,*pChild = NULL;
	if(!dataJson||!pOutData)
	{
		return -1;
	}
	pChild = Common_Json_GetAttrValue(dataJson, -1, "Header/Method", NULL, &pMethod, NULL, NULL);
	if(!pChild)
	{
		return iRet;
	}
	pChild = Common_Json_GetAttrValue(dataJson, -1, "Header/Uri", NULL, &pUri, NULL, NULL);
	if(!pChild)
	{
		return iRet; 
	}
	pData = Common_Json_GetItem(dataJson,-1,"Data");
	if(pMethod)
	{
		Common_Lock(g_tRestLock);
		if(0 == Common_StriCmp(pMethod,(S8*)"get"))
		{
			iRet = get_access_res(pUri,pData,pOutData);
		}
		else if (0 == Common_StriCmp(pMethod,(S8*)"put"))
		{
			iRet = put_access_res(pUri,pData,pOutData);
		}
		else if(0 == Common_StriCmp(pMethod,(S8*)"post"))
		{
			iRet = post_access_res(pUri,pData,pOutData);
		}
		else if (0 == Common_StriCmp(pMethod,(S8*)"delete"))
		{
			iRet = delete_access_res(pUri,pData,pOutData);
		}
		Common_UnLock(g_tRestLock);
	}
	return iRet;
}

static S32 static_Access_CallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
	if (pInParams)
	{
		S32 bPrintDbg = access_get_debug();
		if(bPrintDbg)
		{
			ovfs_print_json(pInParams);
		}
		AnalyzeDataAndMakeResult(pInParams,pOutParams);
	}
	return 0;
}

S32 libaccess_sdk_init()
{
	cJSON_Struct *pConfig = NULL;
	S8 *pModuleName = (S8*)"Access";
	//cJSON_Struct *pOutParams = NULL;
	S32 nRet;

	//Common_System("/root/nginx/sbin/nginx -p /root/nginx -s quit");

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig)
	{
		Common_Json_SetAttrValue(pConfig,-1,"SystemName",Common_Json_Type_String,"ovfs",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"ModuleName",Common_Json_Type_String,pModuleName,0,0);
#ifdef WIN32
		Common_Json_SetAttrValue(pConfig,-1,"RemoteDomain",Common_Json_Type_String,"127.0.0.1",11,0);
		Common_Json_SetAttrValue(pConfig,-1,"RemotePort",Common_Json_Type_Number,NULL,100,0);
		Common_Json_SetAttrValue(pConfig,-1,"LocalPort",Common_Json_Type_Number,NULL,101,0);
#endif
	}
	nRet = Module_Init_Ex(&g_hModuleHandle,pConfig,NULL,static_Access_CallFunctions,NULL);
	LOGI("Module init %d\n",nRet);
	Common_Json_Delete(pConfig);
	if (nRet < 0)
	{
		if (nRet == MODULE_ERROR_TYPE_RUNNING)
		{
			printf("Module[access] is Running,exit!\n");
		}
		return -1;
	}

	nRet = request_channel_ability(g_hModuleHandle);
	if(0 != nRet)
	{
		LOGW("======request channel ability failed======\n");
		Common_Sleep(1,0);
	}
	else
	{
		g_bInited = 1;
	}

	// 1. Load cfg
	nRet = access_loadCfg(g_hModuleHandle);
	if(nRet < 0)
	{
		do{
			nRet = access_CreateDefCfg(g_hModuleHandle);
			if(0 != nRet)
				Common_Sleep(1,0);
		}while(nRet != 0);
	}
	// 2. 初始化资源并注册订阅
	init_access_res(g_hModuleHandle);
	// 3.准备好，可以处理订阅及其他相关操作
	Common_Lock_Create(&g_tRestLock,NULL);

	while(!g_bInited)
	{
		Common_Lock(g_tRestLock);
		nRet = request_channel_ability(g_hModuleHandle);
		if(0 != nRet)
		{
			LOGW("======Retry request channel ability======\n");
		}
		else
		{
			g_bInited = 1;
			g_iCfgChange = OVFS_CFG_FLAG|0x01;
		}
		Common_UnLock(g_tRestLock);

		Common_Sleep(1,0);
	}
	
	return 0;
}


S32 libaccess_sdk_uninit()
{
	return 0;
}
