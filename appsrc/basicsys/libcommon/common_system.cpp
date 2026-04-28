

#include"libcommon_struct.h"
#include "libcommon_api.h"
#ifndef WIN32
#include <sys/wait.h>
#define SYSTEM_TIME_OUT         (10*60*100)   //10ms in unit
#define MAX_SYSTEM_CMD_NUM      64
#define MAX_SYSTEM_CMD_LENGTH   1024

#define     EC_INIT                                -1001
#define     EC_INIT_STR                        "Interface need init first or already init"
#define     EC_MEM                               -1002
#define     EC_MEM_STR                       "Alloc memory failed"
#define     EC_FORMAL                        -1003
#define     EC_FORMAL_STR                "Formal error , receive a data with error format in IPC"
#define     EC_SRC_NO_CON              -1004
#define     EC_SRC_NO_CON_STR      "Local socket is not connected to server"
#define     EC_DST_NO_CON               -1005
#define     EC_DST_NO_CON_STR       "Remote socket is not connected to server"
#define     EC_APP_BUSY                      -1006
#define     EC_APP_BUSY_STR             "App is busy , MQ will drop new coming data"
#define     EC_LIST_FULL                     -1007
#define     EC_LIST_FULL_STR             "MQ list is full"
#define     EC_SHM_SEQ                     -1008
#define     EC_SHM_SEQ_STR             "Share memory queue seq error"
#define     EC_OP_FAILED                   -1009
#define     EC_OP_FAILED_STR           "Operate failed"
#define     EC_SHM_DESTROY             -1010
#define     EC_SHM_DESTROY_STR     "Share memory queue destroyed"

#define Utils_SetThreadName() prctl(PR_SET_NAME,__func__)

#define SUCCESS 0
#define FAILURE (-1)
#define MYSYS_PATH "/root/bin/sysinit"
#define SYS_TIMEOUT (-2)
#define SYS_SHARE_ID 100
#define SYS_SEM_ID     101
typedef union{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
}MYSYS_SEMUM_U;
typedef enum  {
	SYSTEM_EMPTY,				/* system command not ready */
	SYSTEM_READY,				/* system command ready */
	SYSTEM_EXECUTING,           /* system is executing */
	SYSTEM_FINISHED             /* system executed OK or time out*/
}MYSYS_STATUS_T;

typedef struct  {
	int avail;                          /* systemd index available */
	int ret;                            /* systemd return value */
	int timeOut;                        /* systemd executing command timeOut */
	MYSYS_STATUS_T status;              /* status of system command */
	char cmd[MAX_SYSTEM_CMD_LENGTH];                      /* command string */
}MYSYS_SHM_INFO_T;

typedef struct{
	MYSYS_SHM_INFO_T *shm;              /* share memory handle */
	int shmId;							/* share memory flag id */
	int semId;							/* semaphore flag id */
}MYSYS_SHARE_INFO_T;


//static int s_mySys_inited = 0;
static int s_mySys_exitServer = 0;

static void Rsleep (unsigned int ntime)
{
    fd_set rfds;
    struct timeval tv; 
    //int retval;  /* Watch stdin  (fd 0) to see when it has input. */
    
    FD_ZERO (&rfds);
    FD_SET (0, &rfds);  /* Wait up to five seconds. */
    tv.tv_sec = ntime / 1000000;
    tv.tv_usec = ntime % 1000000;
    select (0, &rfds, NULL, NULL, &tv);  /* Don't rely on the value of tv now! */
    
    return;
}

/**
*  \brief  Semaphore P OP.
*  \param  semaphore id
*  \return 
*/
static int SystemP(int semId)
{
    struct sembuf sops={0, -1, SEM_UNDO};
    return (semop(semId, &sops, 1));
}

/**
*  \brief Semaphore V OP.
*  \param semaphore id
*  \return 
*/
static int SystemV(int semId)
{
    struct sembuf sops={0, +1, SEM_UNDO};
    return (semop(semId, &sops, 1));
}

static int sem(key_t key)
{
    MYSYS_SEMUM_U  lcsem;
    int semId;
    
    lcsem.val=0;
    semId = semget(key, 1, IPC_CREAT|0666);
    if (-1 == semId) 
    {
        LOGE("create semaphore error.");
        return -1;
    }
    
    /* init semaphore */
    semctl(semId, 0, SETVAL, lcsem);
    return semId;
}

static int SystemSemIn(MYSYS_SHARE_INFO_T* shareInfo)
{
    key_t key;
    int lcShmId,lcSemId;
    void *lcShm;
    
    key   = ftok(MYSYS_PATH, SYS_SHARE_ID);
    lcShmId = shmget(key, MAX_SYSTEM_CMD_NUM*sizeof(MYSYS_SHM_INFO_T), /*IPC_CREAT|*/0666); /* create memory */
    if(-1 == lcShmId) 
    {
        LOGE("create shared momery error.");
        return -1;
    }
    
    lcShm = shmat(lcShmId, 0, 0);               /* attached memory */
    if(-1 == (long)lcShm) 
    {  
        LOGE("attach shared momery error.");
        return -1;
    }
    
    key   = ftok(MYSYS_PATH, SYS_SEM_ID);
    lcSemId = semget(key, 1, /*IPC_CREAT|*/0666);
    if (-1 == lcSemId)
    {
        LOGE("create semaphore error.");
        return -1;
    }
    SystemP(lcSemId);                                  /* semaphore p. op */
    
    shareInfo->shm      = (MYSYS_SHM_INFO_T *)lcShm;
    shareInfo->shmId    = lcShmId;
    shareInfo->semId    = lcSemId;
    
    return SUCCESS;
}

static int SystemSemOut(MYSYS_SHARE_INFO_T shareInfo)
{
    void *shm;
    int semId;

    shm     = shareInfo.shm;
    semId   = shareInfo.semId;
    
    SystemV(semId);
    shmdt(shm);
    
    return SUCCESS;
}

static int SystemInit(MYSYS_SHARE_INFO_T* shareInfo)
{
    key_t key;
    int lcShmId,lcSemId;
    void *lcShm;
    int i;
    MYSYS_SHM_INFO_T *pshm;

    key   = ftok(MYSYS_PATH, SYS_SHARE_ID);
    lcShmId = shmget(key, MAX_SYSTEM_CMD_NUM*sizeof(MYSYS_SHM_INFO_T), IPC_CREAT|0666); /* create memory */
    if(-1 == lcShmId) 
    {
        LOGE("create shared momery error.");
        return -1;
    }
   
    lcShm = shmat(lcShmId, 0, 0);               /* attached memory */
    if(-1 == (long)lcShm) 
    {
        LOGI("attach shared momery error.");
        return -1;
    }
    
    key   = ftok(MYSYS_PATH, SYS_SEM_ID);
    lcSemId = sem(key);
    if (-1 == lcSemId)
    {
        LOGE("create semaphore error.");
        return -1;
    }

    SystemV(lcSemId);                                  /* semaphore p. op */

    shareInfo->shm      = (MYSYS_SHM_INFO_T *)lcShm;
    shareInfo->shmId    = lcShmId;
    shareInfo->semId    = lcSemId;

    for(i=0;i<MAX_SYSTEM_CMD_NUM;i++)
    {
        pshm = (MYSYS_SHM_INFO_T *)lcShm + i;
        pshm->avail = 1;
        pshm->status = SYSTEM_EMPTY;
    }

    return SUCCESS;
}

void MySys_ServerStart()
{
    int index;
    MYSYS_SHARE_INFO_T shareInfo;
    MYSYS_SHM_INFO_T   *pshminfo;

    SystemInit(&shareInfo);
    //s_mySys_inited = 1;
    Utils_SetThreadName();

    while(s_mySys_exitServer == 0)
    {
        SystemSemIn(&shareInfo);

        for (index = 0; index < MAX_SYSTEM_CMD_NUM; index++)
        {
            pshminfo = (MYSYS_SHM_INFO_T *) (shareInfo.shm) + index;

            if (SYSTEM_READY == pshminfo->status)  //command ready, fork a child process to execute it.
            {
                pid_t pidparent;
                int statusparent;

                pshminfo->status = SYSTEM_EXECUTING;
                SystemSemOut(shareInfo);

                if (0 == (pidparent = fork()))
                {
                    pid_t pid;
                    int cmdindex;
                    int status;
                    int ret = 0;
                    int i;
                    MYSYS_SHARE_INFO_T shareinfochld;
                    MYSYS_SHM_INFO_T *pshminfochld;

                    cmdindex = index;

                    if (0 == (pid = fork()))
                    {
                        char cmdstr[MAX_SYSTEM_CMD_LENGTH];
                        MYSYS_SHARE_INFO_T shareinfograndchld;
                        MYSYS_SHM_INFO_T *pshminfograndchld;
                        int grandindex = cmdindex;

                        SystemSemIn(&shareinfograndchld);
                        pshminfograndchld = (MYSYS_SHM_INFO_T *) (shareinfograndchld.shm) + grandindex;
                        snprintf(cmdstr, sizeof(cmdstr), "%s", pshminfograndchld->cmd);
                        SystemSemOut(shareinfograndchld);

                        //ret=system(cmdstr);
                        ret = execl("/bin/sh", "sh", "-c", cmdstr, (char *) 0);
                        if (-1 != ret)
                            ret = 0;

                        SystemSemIn(&shareinfograndchld);
                        pshminfograndchld = (MYSYS_SHM_INFO_T *) (shareinfograndchld.shm) + grandindex;
                        pshminfograndchld->ret = ret;
                        SystemSemOut(shareinfograndchld);
                        _exit(0);
                    }

                    i = 0;
                    while (0 == waitpid(pid, &status, WNOHANG))  //check if the the time is out
                    {

                        if (i++ < SYSTEM_TIME_OUT)
                        {
                            Rsleep(10000); //Here, this sleep period cannot be too long, otherwise, sysinit cannot return immediately.
                        }
                        else
                        {
                            printf("\n[sysinit]command index %d executing time out!\n", cmdindex);
                            kill(pid, SIGKILL);
                            waitpid(pid, &status, WNOHANG);

                            SystemSemIn(&shareinfochld);
                            pshminfochld = (MYSYS_SHM_INFO_T *) (shareinfochld.shm) + cmdindex;
                            pshminfochld->timeOut = 1;
                            SystemSemOut(shareinfochld);
                            break;
                        }
                    }

                    SystemSemIn(&shareinfochld);
                    pshminfochld = (MYSYS_SHM_INFO_T*) (shareinfochld.shm) + cmdindex;
                    pshminfochld->ret = ret;
                    pshminfochld->status = SYSTEM_FINISHED;
                    SystemSemOut(shareinfochld);

                    _exit(0);

                } //childprocess over
                waitpid(pidparent, &statusparent, WNOHANG);

                break;
            }
        }

        if (MAX_SYSTEM_CMD_NUM == index)
        {
            SystemSemOut(shareInfo);
        }
        waitpid(-1, NULL, WNOHANG);
        Rsleep(100000);    //10ms}
    }

    return;
}


int MySys_Cmd(const char *command)
{
    if(command == NULL)
    {
        LOGE("mysystem not init or command is NULL!\n");
        return -1;
    }
	if (strlen(command) > MAX_SYSTEM_CMD_LENGTH - 1)
	{
		LOGE("mysystem command too long <%s>!\n",command);
		return -1;
	}

    int ret;
    int index = MAX_SYSTEM_CMD_NUM, i;
    MYSYS_SHARE_INFO_T shareInfo;
    MYSYS_SHM_INFO_T *pchldshminfo;

    ret = SystemSemIn(&shareInfo);
    if(ret < 0)
   {
   	LOGE("sys cmd sem fail. cmd=%s\n",command);
	return -1;
   }

    for(i=0; i<MAX_SYSTEM_CMD_NUM; i++)
    {
    	pchldshminfo = (MYSYS_SHM_INFO_T *)(shareInfo.shm) + i;
    	if(1 == pchldshminfo->avail)
    	{      
    		index = i;
    		pchldshminfo->avail = 0;
    		pchldshminfo->timeOut = 0;
    		snprintf(pchldshminfo->cmd,sizeof(pchldshminfo->cmd),"%s", command);
    		pchldshminfo->status = SYSTEM_READY;
    		break;
    	}
    }
    SystemSemOut(shareInfo);

    if(index == MAX_SYSTEM_CMD_NUM)
    {
    	return -1;
    }

    while(1)
    {
    	SystemSemIn(&shareInfo);
    	pchldshminfo = (MYSYS_SHM_INFO_T *)shareInfo.shm + index;
    	if(SYSTEM_FINISHED == pchldshminfo->status)
    	{
    		pchldshminfo->avail = 1;

    		if(1 == pchldshminfo->timeOut)
    			ret = SYS_TIMEOUT;
    		else
    			ret = pchldshminfo->ret;
            
    		pchldshminfo->status = SYSTEM_EMPTY;
    		SystemSemOut(shareInfo);
    		break;
    	}
    	SystemSemOut(shareInfo);
    	Rsleep(10000);
    }

	return ret;
}

int MySys_Exit(void)
{
    s_mySys_exitServer = 1;
    return 0;
}

//执行程序program, pipe_fd: 管道句柄，progress_pid: 新进程的pid
static S32 popen_ex(const S8 *program, const S8 *type, S32 *pipe_fd, S32 *progress_pid)
{
	*pipe_fd = *progress_pid = -1;
	S32 pdes[2], pid;
	if (((*type != 'r') && (*type != 'w')) || type[1])
	{
		LOGE("Type Must 'r' Or 'w' for popen_ex\n");
		return -EINVAL;
	}

	if (pipe(pdes) < 0)
	{
		LOGE("Create Pipe Failed\n");
		return EC_OP_FAILED;
	}

	switch (pid = vfork())
	{
	case -1:
		LOGE("vfork Failed\n");
		(void) close(pdes[0]);
		(void) close(pdes[1]);
		return EC_OP_FAILED;

	case 0:
		if (*type == 'r')
		{
			if (pdes[1] != fileno(stdout))
			{
				(void) dup2(pdes[1], fileno(stdout));
				(void) close(pdes[1]);
			}
			(void) close(pdes[0]);
		}
		else
		{
			if (pdes[0] != fileno(stdin))
			{
				(void) dup2(pdes[0], fileno(stdin));
				(void) close(pdes[0]);
			}
			(void) close(pdes[1]);
		}

		execl("/bin/sh", "sh", "-c", program, NULL);
		_exit(127);
	}

	if (*type == 'r')
	{
		*pipe_fd = pdes[0];
		(void) close(pdes[1]);
	}
	else
	{
		*pipe_fd = pdes[1];
		(void) close(pdes[0]);
	}

	*progress_pid = pid;
	return 0;
}
//关闭管道，杀死子进程(一定要收尸防止僵尸进程的产生)
static S32 pclose_ex(S32 pipe_fd, S32 progress_pid)
{
	(void) close(pipe_fd);
	sigset_t omask, nmask;
	//union wait pstat;
	int pstat;
	int pid;

	sigemptyset(&nmask);
	sigaddset(&nmask, SIGINT);
	sigaddset(&nmask, SIGQUIT);
	sigaddset(&nmask, SIGHUP);
	(void) sigprocmask(SIG_BLOCK, &nmask, &omask);

	do
	{
		//注意:不论新进程结束与否，在此时我们都希望能尽快的结束掉新开的进程，因此
		//这里优先向进程发送一个SIGTERM信号，尽量让新开的进程优雅的结束掉(当然如果
		//当前进程已经结束了那就皆大欢喜)
		kill(progress_pid, SIGTERM);
		pid = waitpid(progress_pid, (int *) &pstat, 0);
	} while (pid == -1 && errno == EINTR);

	(void) sigprocmask(SIG_SETMASK, &omask, NULL);
	return (pid == -1 ? EC_OP_FAILED : 0);
}

S32  Common_SystemServerStart()
{
	//MySys_ServerStart();
	return 0;
}

S32  Common_SystemServerStop()
{
	return 0;//MySys_Exit();
}

S32  Common_System(const S8 *cmd)
{
#if 0
	FILE *pf;
	pf = popen(cmd,"r");
	if (pf == NULL)
	{
		return -1;
	}
	 pclose(pf);
	return 0;
#else

	return MySys_Cmd(cmd);
#endif
}
S32  Common_Exe_Cmd(const S8 *cmd, U32 timeout_ms, S8 *buf, U32 buf_size)
{
	if ((cmd == NULL) || (buf == NULL) || (buf_size == 0))
	{
		LOGE("Invalid Param\n");
		return -EINVAL;
	}

	memset(buf, 0, buf_size);
	S32 pipe_fd, progress_pid;
	if (popen_ex(cmd, "r", &pipe_fd, &progress_pid) != 0)
	{
		LOGE("popen_ex function Failed\n");
		return -1;
	}

	U32 execute_ok = 0;
	struct timeval timeout;
	fd_set rd_set;
	FD_ZERO(&rd_set);
	FD_SET(pipe_fd, &rd_set);
	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;

	int ret = select(pipe_fd + 1, &rd_set, NULL, NULL, &timeout);
	if (ret < 0)
	{
		LOGE("select Function Error\n");
	}
	else if (ret == 0)
	{
		LOGE("select Function Timeout\n");
	}
	else
	{
		if (FD_ISSET(pipe_fd, &rd_set))
		{
			S32 real_read_size = read(pipe_fd, buf, buf_size);
			if (real_read_size == (S32) buf_size)
			{
				LOGW("CMD[%s] Output Info Size May Be Overflow, Store Buffer Size = %u\n", cmd, buf_size);
			}
			buf[buf_size - 1] = '\0';        //防止越界
			execute_ok = 1;
		}
	}

	pclose_ex(pipe_fd, progress_pid);
	return execute_ok ? 0 : -1;
}


#define CMD_STACK_SIZE (8 * 1024)    /* Stack size for cloned child */

typedef struct {
    char cmd_str[128];
    char cmd_fifo[64];
    char *stack;
    int cmd_fd;
    pid_t cmd_pid;
} cmd_arg_t;

static int child_func(void *data)
{
    cmd_arg_t *arg = (cmd_arg_t *)data;
    if (arg == NULL)
        return 0;

    int fd = open(arg->cmd_fifo, O_RDWR | O_NONBLOCK, 0666);

    dup2(fd,fileno(stdout));

    execl("/bin/sh", "sh", "-c", (char *)arg->cmd_str,(char *) 0);
    return 0;           /* Child terminates now */
}

void *Common_Exe_Popen(const S8 *cmd)
{
    char *stack = NULL;                    /* Start of stack buffer */
    char *stackTop = NULL;                 /* End of stack buffer */
    cmd_arg_t *arg = NULL;

    stack = (char *)malloc(CMD_STACK_SIZE);
    if (stack == NULL)
        return NULL;
    stackTop = stack + CMD_STACK_SIZE;  /* Assume stack grows downward */
    arg = (cmd_arg_t *) malloc(sizeof(cmd_arg_t));
    if (arg == NULL)
    {
        free(stack);
        return NULL;
    }

    memset(arg,0,sizeof(cmd_arg_t));
    arg->stack = stack;
    snprintf(arg->cmd_fifo,sizeof(arg->cmd_fifo),"%d",getpid());
    snprintf(arg->cmd_str,sizeof(arg->cmd_str),"%s",cmd);

    mkfifo(arg->cmd_fifo,S_IRWXU | S_IRWXG | S_IRWXO);

    arg->cmd_pid = clone(child_func, stackTop,  SIGCHLD  , arg);
    if (arg->cmd_pid == -1) {
        free(stack);
        free(arg);
        return NULL;
    }

    arg->cmd_fd = open(arg->cmd_fifo, O_RDWR | O_NONBLOCK, 0666);
    if (arg->cmd_fd < 0){
        free(stack);
        free(arg);
        return NULL;
    }

    return (void *)arg;
}

S32 Common_Exe_Pread(void *popen_stream, void *buf, U32 nbyte, U32 timeout_ms)
{
    cmd_arg_t *arg = (cmd_arg_t *)popen_stream;
    struct timeval timeout;

    if (arg == NULL || arg->cmd_pid == 0 || arg->cmd_fd <= 0)
        return 0;

    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(arg->cmd_fd, &rd_set);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(arg->cmd_fd + 1, &rd_set, NULL, NULL, &timeout);
    if (ret < 0)
    {
        LOGE("select Function Error\n");
    }
    else if (ret == 0)
    {
        LOGW("select Function Timeout\n");
    }
    else
    {
        ret = read(arg->cmd_fd,buf,nbyte - 1);
        if (ret > 0)
            *((char *)buf + ret) = 0;
    }
    
    return ret;
}

S32 Common_Exe_Pclose(void *popen_stream)
{
    cmd_arg_t *arg = (cmd_arg_t *)popen_stream;
    if (arg == NULL || arg->cmd_pid == 0 || arg->cmd_fd <= 0)
        return -1;
    
    int statt = 0;

    kill(arg->cmd_pid,9);
    waitpid(arg->cmd_pid, (int *) &statt, 0);

    close(arg->cmd_fd);
    remove((const char *)arg->cmd_fifo);
    free(arg->stack);
    free(arg);
    return 0;
}

S8 * Common_Exe_Pgets(S8 *s, U32 size, U32 timeout_ms, void *popen_stream)
{
    cmd_arg_t *arg = (cmd_arg_t *)popen_stream;
    if (arg == NULL || arg->cmd_pid == 0 || arg->cmd_fd <= 0)
        return NULL;
    struct timeval timeout;
    fd_set rd_set;
    
    FD_ZERO(&rd_set);
    FD_SET(arg->cmd_fd, &rd_set);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(arg->cmd_fd + 1, &rd_set, NULL, NULL, &timeout);
    if (ret <= 0)
    {
//		LOGW("select Function Error %d\n",ret);
        return NULL;
    }
    
    char c = 0;
    unsigned int csize = 0;
    do
    {
        ret = read(arg->cmd_fd,&c,1);
        if (ret != 1)
        {
            s[csize] = 0;
            return s;
        }
        
        s[csize] = c;
        csize += ret;
        
        if (csize >= size - 1)
        {
            s[csize] = 0;
            return s;
        }
        
        if (c == '\n')
        {
            s[csize] = 0;
            return s;
        }
    }while (ret == 1);
    
    return NULL;
}

S32  Common_Exe_Pcmd(const S8 *cmd, U32 timeout_ms, S8 *buf, U32 buf_size)
{
    int ret = 0;
    void * handle = Common_Exe_Popen(cmd);
    if (handle == NULL)
        return -1;

    ret = Common_Exe_Pread(handle,buf,buf_size,timeout_ms);
    Common_Exe_Pclose(handle);

    return ret;
}
#else
S32  Common_System(const S8 *cmd)
{
	return system(cmd);
}
S32  Common_Exe_Cmd(const S8 *cmd, U32 timeout_ms, S8 *buf, U32 buf_size)
{    
	S8 szBuff[256];
	FILE *fPipe = NULL;
	if (cmd == NULL)
	{
		return -1;
	}
	

	fPipe = popen(cmd, "r");
	if (fPipe == NULL)
	{
		LOGE("popen_ex function Failed\n");
		return -1;
	}


	memset(szBuff,0,sizeof(szBuff));

	fread(buf,1,buf_size,fPipe);


	return pclose(fPipe);
}
S32  Common_SystemServerStart()
{
	return 0;
}
S32  Common_SystemServerStop()
{
	return 0;
}
#endif
