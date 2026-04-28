#include"libcommon_struct.h"
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
		printf("[sysinit.error]create semaphore error.\n");
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
		printf("[sysinit.error]create shared momery error.\n");
		return -1;
	}

	lcShm = shmat(lcShmId, 0, 0);               /* attached memory */
	if(-1 == (int)lcShm) 
	{  
		printf("[sysinit.error]attach shared momery error.\n");
		return -1;
	}

	key   = ftok(MYSYS_PATH, SYS_SEM_ID);
	lcSemId = semget(key, 1, /*IPC_CREAT|*/0666);
	if (-1 == lcSemId)
	{
		printf("[sysinit.error]create semaphore error.\n");
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
		printf("[sysinit.error]create shared momery error.\n");
		return -1;
	}

	lcShm = shmat(lcShmId, 0, 0);               /* attached memory */
	if(-1 == (int)lcShm) 
	{
		printf("[sysinit.info]attach shared momery error.\n");
		return -1;
	}

	key   = ftok(MYSYS_PATH, SYS_SEM_ID);
	lcSemId = sem(key);
	if (-1 == lcSemId)
	{
		printf("[sysinit.error]create semaphore error.\n");
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

int main(int argc,char *argv[])
{
	int index;
	MYSYS_SHARE_INFO_T shareInfo;
	MYSYS_SHM_INFO_T   *pshminfo;

	if(SystemInit(&shareInfo))
	{
		printf("[sysinit.error]init failed.\n");
		return -1;
	}

	while(1)
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

						printf("[sysinit.info][%d]run <%s> .\n",index,cmdstr);

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

	return 0;
}
