#include "libcommon_struct.h"
#include "libcommon_api.h"
S32 Common_Socket_Open(S32 domain, S32 type, S32 protocol)
{
	S32 fd = socket(domain, type, protocol);
	if (fd < 0)
	{
		LOGE("socket fail! %s\n",strerror(errno));
		return -1;
	}

	return fd;
}
S32 Common_Socket_Close(S32 socket)
{
	S32 ret = 0;

	if (socket < 2)
	{
		LOGE("socket fd < 2\n");
		return -1;
	}
#ifdef WIN32
	ret =closesocket(socket);
#else
	ret = close(socket);
#endif
	if (ret != 0)
	{
		LOGE("close failed %s\n",strerror(errno));
	}

	return ret;
}
U32 Common_Socket_IsValid(S32 socket_fd)  //检查句柄是否合法
{
	if (socket_fd < 2)
		return 0;

	char file_path[64] = { 0 };
#ifdef WIN32
	return 1;
#else
	snprintf(file_path,sizeof(file_path)-1,"/proc/%d/fd/%d",getpid(),socket_fd);
	if (access(file_path,F_OK) != 0)
		return 0;
#endif
	return 1;
}

S32 Common_File_Open(S8 *file_path, S32 flags, U32 mode)
{
	S32 fd = open(file_path, flags, mode);
	if (fd < 0)
	{
		LOGE("open %s, flags = 0x%x, mode = 0x%x fail! %s\n", file_path, flags, mode,strerror(errno));
		return -1;
	}

	return fd;
}
S32 Common_File_Close(S32 fd)
{
	int ret = 0;
	if (fd < 2)
	{
		LOGE("socket fd < 2\n");
		return -1;
	}

	ret = close(fd);
	if (ret != 0)
	{
		LOGE("close failed %s\n",strerror(errno));
	}
	return ret;
}
S32 Common_File_IsValid(S32 fd)    //检查句柄是否合法
{

	if (fd < 2)
		return 0;

	char file_path[64] = { 0 };
#ifdef WIN32
#else
	snprintf(file_path,sizeof(file_path)-1,"/proc/%d/fd/%d",getpid(),fd);
	if (access(file_path,F_OK) != 0)
		return 0;
#endif
	return 1;
}
S32 Common_File_Read(S32 fd,void *Buff,U32 nBytes)
{
	return read(fd,Buff,nBytes);
}
S32 Common_File_Write(S32 fd,void *Buff,U32 nBytes)
{
	return write(fd,Buff,nBytes);
}
S64 Common_File_Seek(S32 fd,S64 offset,S32 origin)
{
#ifdef WIN32
	return _lseeki64(fd,offset,offset);
#else
	return lseek(fd,offset,origin);
#endif
}

FILE* Common_File_fOpen(S8 *file_path, S8 *mode)
{
	FILE * fp = NULL;

	if (file_path == NULL || mode == NULL)
	{
		LOGE("param error\n");
		return NULL;
	}

	fp = fopen(file_path,mode);
	if (fp == NULL)
	{
		LOGE("fopen failed %s\n",strerror(errno));
	}

	return fp;
}
S32 Common_File_fClose(FILE* fd)
{
	int ret = 0;
	if (fd == NULL)
	{
		LOGE("param error\n");
		return -1;
	}

	ret = fclose(fd);

	if (ret != 0)
	{
		LOGE("fclose failed %s\n",strerror(errno));
	}

	return ret;
}
U32 Common_File_fIsValid(FILE* fd) //检查句柄是否合法
{
	int sfd = 0;
	char file_path[64] = { 0 };

	if (fd == NULL)
	{
		LOGE("param error\n");
		return 0;
	}
#ifdef WIN32
#else
	sfd = fileno(fd);
	if (sfd < 2)
	{
		return 0;
	}

	snprintf(file_path,sizeof(file_path)-1,"/proc/%d/fd/%d",getpid(),sfd);
	if (access(file_path,F_OK) != 0)
		return 0;
#endif
	return 1;
}
S32 Common_File_fRead(void *Buff,U32 SizePer,U32 nCount,FILE *fd)
{
	return fread(Buff,SizePer,nCount,fd);
}
S32 Common_File_fWrite(void *Buff,U32 SizePer,U32 nCount,FILE *fd)
{
	return fwrite(Buff,SizePer,nCount,fd);
}
S64 Common_File_fSeek(FILE * fd,S64 offset,S32 origin)
{
	return fseek(fd,offset,origin);
}
S64 Common_File_fTell(FILE * fd)
{
	return ftell(fd);
}
S32 Common_File_fEof(FILE * fd)
{
	return feof(fd);
}
S32 Common_File_fFlush(FILE * fd)
{
	return fflush(fd);
}
#ifdef WIN32
#define F_OK 00
#define R_OK 04
#define W_OK 02
#define X_OK 00
#endif
U32 Common_File_IsExist(S8 *pathname) //文件是否存在
{
	return (access(pathname, F_OK) == 0) ? 1 : 0;
}

S32 Common_File_MkDir(S8 *pathname)
{

		char   DirName[256];  
		int   i,len;
		if (pathname == NULL)
		{
			return -1;
		}
		strcpy(DirName,   pathname);  
		len   =   strlen(DirName); 

		if(DirName[len-1]!='/' && DirName[len - 1] != '\\')  
		{
			strcat(DirName,   "/");  
		}

		len   =   strlen(DirName);  

		for(i=1;   i<len;   i++)  
		{  
			if(DirName[i]=='/' || DirName[i]=='\\')  
			{  
				DirName[i]   =   0;  
#ifdef WIN32
				if(_access(DirName,NULL)!=0   )  
#else
				if(   access(DirName,   0)!=0   )  
#endif
				{  
#ifdef WIN32
					if(_mkdir(DirName)==-1)  
#else
					if(mkdir(DirName,0755)==-1)  
#endif
					{   
						perror("mkdir   error");   
						return   -1;   
					}  
				}  
				DirName[i]   =   '/';  
			}  
		}  

		return   0;  

}


char * Common_GetSelfExeName(char* exeName, int len)
{
	if (exeName == NULL || len <= 0)
	{
		return NULL;
	}

	char* path_end = NULL;
	char path[512] = { 0 };
#ifdef WIN32
	GetModuleFileName(NULL,path,512);
	path_end = strrchr(path, '\\');
	if (path_end == NULL)
		return NULL;

	++path_end;
	snprintf(exeName, len - 1, "%s", path_end);
#else
	if (readlink("/proc/self/exe", path, PATH_MAX) <= 0)
		return NULL;

	path_end = strrchr(path, '/');
	if (path_end == NULL)
		return NULL;

	++path_end;
	snprintf(exeName, len - 1, "%s", path_end);
#endif
	return exeName;
}

S32 Common_File_Lock(int lockFd, int type, int whence, int start, int len, int timeout)
{
	S32 nRet = -1;
#ifdef WIN32
	
#else
	struct flock lock;
	S32 ftype;
	if (type == COMMON_FILE_LOCK_READ)
	{
		ftype = F_RDLCK;
	}
	else if (type == COMMON_FILE_LOCK_WRITE)
	{
		ftype = F_WRLCK;
	}
	else if (type == COMMON_FILE_UNLOCK)
	{
		ftype = F_UNLCK;
	}
	else
	{
		return -1;
	}
	
	lock.l_type = (short int) ftype; //F_RDLCK F_WRLCK  F_UNLCK
	lock.l_whence = whence; // SEEK_SET = 0 ,SEEK_CUR = 1, SEEK_END= 2;
	lock.l_start = start;
	lock.l_len = len; //此三个成员设为0值表示对整个文件加锁
	while ((nRet = fcntl(lockFd, F_SETLKW, &lock)) != 0)
		LOGE("lock failed %s\n",strerror(errno));
#endif
	return nRet;
}


S32 Common_InstanceIsRunning()
{
	S8 strTmp[128];
	S8 szProcName[128];
	
	if(NULL == Common_GetSelfExeName(szProcName,128))
	{
		return 0;
	}
#ifdef WIN32
	sprintf(strTmp,"Common_instances_%s.run",szProcName);
HANDLE hMutex = CreateMutex(NULL,FALSE,strTmp);
	if (hMutex == NULL)
	{
		return 0;
	}
	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		//CloseHandle(hMutex);
		return 1;
	}
#else
	
	int fd ;
	if (0 != Common_File_MkDir((S8 *)"/tmp/modules/instances/"))
	{
		return 0;
	}
	sprintf(strTmp,"/tmp/modules/instances/%s.run",szProcName);
	fd = open(strTmp, O_RDWR | O_CREAT, 0666);
	if (fd < 0)
	{
		return 0;
	}
	struct flock fl;
	fl.l_type = F_WRLCK; // 写文件锁定
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	int ret = fcntl(fd, F_SETLK, &fl);
	if (ret < 0)
	{
		if (errno == EACCES || errno == EAGAIN)
		{
			printf("%s already locked, error: %s\n", strTmp, strerror(errno));
			close(fd);
			return 1;
		}
		close(fd);
		return 0;
	}


	
#endif
	return 0;
}
