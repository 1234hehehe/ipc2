/*
 * common_signalcatch.c
 *
 *  Created on: 2017年7月7日
 *      Author: eric
 */
#ifdef WIN32
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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

#include "libcommon_api.h"

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
#elif (defined (__mips__))
# define REGFORMAT   "%llx"
#endif

static void Common_PrintReg(const ucontext_t *uc, FILE *errFp)
{
#if (defined (__x86_64__)) || (defined (__i386__))
    S32 i;
    for (i = 0; i < NGREG; i++)
    {
        LOGE("reg[%02d]: 0x"REGFORMAT"\n", i, uc->uc_mcontext.gregs[i]);
        fprintf(errFp,"reg[%02d]: 0x"REGFORMAT"\n", i, uc->uc_mcontext.gregs[i]);
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

    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 0, uc->uc_mcontext.arm_r0);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 1, uc->uc_mcontext.arm_r1);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 2, uc->uc_mcontext.arm_r2);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 3, uc->uc_mcontext.arm_r3);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 4, uc->uc_mcontext.arm_r4);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 5, uc->uc_mcontext.arm_r5);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 6, uc->uc_mcontext.arm_r6);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 7, uc->uc_mcontext.arm_r7);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 8, uc->uc_mcontext.arm_r8);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 9, uc->uc_mcontext.arm_r9);
    if (errFp)
    fprintf(errFp,"reg[%02d]     = 0x"REGFORMAT"\n", 10, uc->uc_mcontext.arm_r10);
    if (errFp)
    fprintf(errFp,"FP        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_fp);
    if (errFp)
    fprintf(errFp,"IP        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_ip);
    if (errFp)
    fprintf(errFp,"SP        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_sp);
    if (errFp)
    fprintf(errFp,"LR        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_lr);
    if (errFp)
    fprintf(errFp,"PC        = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_pc);
    if (errFp)
    fprintf(errFp,"CPSR      = 0x"REGFORMAT"\n", uc->uc_mcontext.arm_cpsr);
    if (errFp)
    fprintf(errFp,"Fault Address = 0x"REGFORMAT"\n", uc->uc_mcontext.fault_address);
    if (errFp)
    fprintf(errFp,"Trap no       = 0x"REGFORMAT"\n", uc->uc_mcontext.trap_no);
    if (errFp)
    fprintf(errFp,"Err Code  = 0x"REGFORMAT"\n", uc->uc_mcontext.error_code);
    if (errFp)
    fprintf(errFp,"Old Mask  = 0x"REGFORMAT"\n", uc->uc_mcontext.oldmask);
#elif (defined (__mips__))
    S32 i;
    for (i = 0; i < NGREG; i++)
    {
        LOGE("reg[%02d]: 0x"REGFORMAT"\n", i, uc->uc_mcontext.gregs[i]);
        if (errFp)
          fprintf(errFp,"reg[%02d]: 0x"REGFORMAT"\n", i, uc->uc_mcontext.gregs[i]);
    }
    LOGE("SP        = 0x"REGFORMAT"\n", uc->uc_mcontext.gregs[29]);
    LOGE("LR        = 0x"REGFORMAT"\n", uc->uc_mcontext.gregs[31]);
    LOGE("PC        = 0x"REGFORMAT"\n", uc->uc_mcontext.pc);
    if (errFp)
      fprintf(errFp,"SP        = 0x"REGFORMAT"\n", uc->uc_mcontext.gregs[29]);
    if (errFp)
      fprintf(errFp,"LR        = 0x"REGFORMAT"\n", uc->uc_mcontext.gregs[31]);
    if (errFp)
      fprintf(errFp,"PC        = 0x"REGFORMAT"\n", uc->uc_mcontext.pc);
    
#endif
}

static void Common_PrintCallLink(const ucontext_t *uc, FILE *errFp)
{
    S32 i = 0;
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
#elif (defined __mips__)
    const void ** frame_pointer = (const void **)(uc->uc_mcontext.gregs[32]);
    const void *return_address = (const void *)(uc->uc_mcontext.pc);
#else
    const void **frame_pointer = (const void **)uc->uc_mcontext.regs[0];
    const void *return_address = (const void *)uc->uc_mcontext.regs[0];
#endif

    S8 buf[512] = { 0 };

    FILE *cfp = fopen("/proc/self/maps", "r");
    while (fgets(buf, sizeof(buf) - 1, cfp) != NULL )
    {
        LOGE("%s", buf);
        if (errFp)
        {
            fprintf(errFp,"%s", buf);
            fflush(errFp);
        }
    }
    fclose(cfp);

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
        S32 status;
        S8 *tmp = __cxa_demangle(sname, NULL, 0, &status);
        if (status == 0 && tmp)
        {
            sname = tmp;
        }
#endif
        /* No: return address <sym-name + offset> (filename) */

        LOGE("%02d: %p <%s + %p> %s \n", i, return_address, sname,
             dl_info.dli_saddr, dl_info.dli_fname);

        if (errFp)
        {
            fprintf(errFp,"%02d: %p <%s + %p> %s \n", i, return_address, sname,
                    dl_info.dli_saddr, dl_info.dli_fname);
            fflush(errFp);
        }
        i++;
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
#elif (defined (__mips__))
        if (frame_pointer != NULL)
          {
            return_address = frame_pointer[-1];
            frame_pointer = (const void **)(frame_pointer[-3]);
          }
        else if (return_address != (const void *)uc->uc_mcontext.gregs[31])
          {
            return_address = (const void *)uc->uc_mcontext.gregs[31];
          }
        else
          break;
#endif
    }
    LOGE("Stack trace end.\n");
}

static int Common_PrintThreadName(FILE *errFp)
{
  char buf[128] = { 0 };
  char tmp[128] = { 0 };
  struct dirent *ptr = NULL;
  snprintf(buf,sizeof(buf),"%s","/proc/self/task");
  DIR *sp_dp = opendir(buf);

  if (sp_dp == NULL)
      return 0;

  while((ptr=readdir(sp_dp))!=NULL)
  {
    if(NULL != strstr(ptr->d_name, "."))
    {
        continue;
    }
    snprintf(tmp,sizeof(tmp),"%s/%s/comm",buf,ptr->d_name);
    char threadName[32] = { 0 };
    FILE * fp = fopen(tmp,"rb");
    if (fp != NULL)
    {
        fread(threadName,1,sizeof(threadName),fp);
        fclose(fp);
        if (errFp)
            fprintf(errFp, "thread %s %s",ptr->d_name,threadName);

        LOGE("thread %s %s",ptr->d_name,threadName);
    }
  }

  if (sp_dp)
      closedir(sp_dp);
  return 0;
}


static void Common_SigHandler(int signo, siginfo_t *info, void *context)
{
    LOGE("info.si_signo = %d\n", signo);
    if (signo == SIGPIPE)
        return;

    static FILE *errFp = NULL;
    if (errFp == NULL)
        errFp = fopen("/usr/etc/error.log","wb");
    else
        return;

    if (info)
    {
        time_t tt = time(NULL);
        if (errFp)
            fprintf(errFp, "%s info.si_signo = %d\n",ctime(&tt),signo);

        LOGE("info.si_errno = %d\n", info->si_errno);
        if (errFp)
            fprintf(errFp, "info.si_errno = %d\n", info->si_errno);

        LOGE("info.si_code  = %d (%s)\n", info->si_code, (info->si_code == SEGV_MAPERR) ? "SEGV_MAPERR" : "SEGV_ACCERR");
        if (errFp)
            fprintf(errFp, "info.si_code  = %d (%s)\n", info->si_code,
                            (info->si_code == SEGV_MAPERR) ? "SEGV_MAPERR" : "SEGV_ACCERR");

        LOGE("info.si_addr  = %p\n", info->si_addr);
        if (errFp)
            fprintf(errFp, "info.si_addr  = %p\n", info->si_addr);

        Common_PrintThreadName(errFp);
        fflush(errFp);
    }

    if (context)
    {
        const ucontext_t *uc = (const ucontext_t *) context;
        Common_PrintReg(uc,errFp);

        if (errFp)
            fflush(errFp);

        Common_PrintCallLink(uc,errFp);

        if (errFp)
            fflush(errFp);
    }

    if (errFp)
        fflush(errFp);

    usleep(1000000);

    if (errFp)
        fclose(errFp);
    _exit(0);
}

S32 Common_RegistSigHandle(int sigNum)
{
    struct sigaction sa;
    sigset_t waitset;
    memset(&sa, 0, sizeof(struct sigaction));

    sa.sa_sigaction = Common_SigHandler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(sigNum, &sa, NULL) < 0)
    {
        LOGE("register signal handle failed %s\n", strerror(errno));
        return -1;
    }
    sigemptyset(&waitset);
    sigaddset(&waitset, sigNum);
    pthread_sigmask(SIG_UNBLOCK, &waitset, NULL);
    return 0;
}

#endif
