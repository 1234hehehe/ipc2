#include <stdio.h>
#include "libcommon_api.h"
#include "libmodule_struct.h"
#include "libmodule_api.h"
#include "module_subscribe.h"
#include "module_register.h"
#include "module_stream.h"
#include "module_uriproxy.h"
#include "module_memory.h"
#include "lib_encry.h"
#include "libcrypto_api.h"
#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
#include <getopt.h>
#include <sys/vfs.h>
#include <sys/mman.h>
#endif

#ifdef WIN32
#define FACTORY_PATH "d:/"
#define CFG_PATH "d:/cfgfiles"
#define TEMP_PATH "d:/tmp"
#define DEFAULT_PATH "d:/default"
#define CUSTOM_PATH "d:/custom"
#else
#define CFG_PATH "/usr/etc/cfgfiles"
#define FACTORY_PATH "/usr/etc"
#define TEMP_PATH "/tmp/modules/tempdata"
#define DEFAULT_PATH "/update/res/default"
#define DEFAULT_PATH_AUTO "/tmp/auto/res/default"
#define CUSTOM_PATH "/update/res/custom"
#define DEFAULT_PATH_CUSTOM "/usr/etc/default"

#endif

static S32 staticModule_CallCoreFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut);
static S32 static_Module_CallFunctions_ResultFilter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct *pOutParams);
static S32 static_Module_CallResponce(ModuleHandle_T hModuleHandle,LibModuleClientInfo_T *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct *pOutParams);
S32 extern_Module_CallFunctions_MsgRouter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams);
S32 Module_FactoryInfo_Read(cJSON_Struct** pInfo);
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static Common_Log_T g_tModuleLog_xxxx = NULL;
Common_Log_T Module_Log_GetDefaultHandle()
{
	return g_tModuleLog_xxxx;
}
 void Module_Log_SetDefaultHandle(Common_Log_T hLog)
{
	if (g_tModuleLog_xxxx != NULL)
	{
		Common_Log_Destroy(&g_tModuleLog_xxxx);
	}
	g_tModuleLog_xxxx = hLog;
}
#ifdef WIN32
BOOL APIENTRY DllMain( HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
		{

			static S32 bNetLoad = 0;
			if (!bNetLoad)
			{
				WORD wVersionRequested;
				WSADATA wsaData;
				wVersionRequested = MAKEWORD( 2, 2 );

				S32 err = WSAStartup( wVersionRequested, &wsaData );
				if ( err != 0 ) {
					/* Tell the user that we could not find a usable */
					/* WinSock DLL.                                  */
					return FALSE;
				}
			}

			break;
		}
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif
static S8 g_szSerialNumber[32] = {0};
static S8 g_szSerialUUID[129]={0};
static S8 g_szSerialUUID_CPU[129]={0};
static S8 g_szSerialUUID_SW[129]={0};
static S32 g_bBadBoy = 0;

#ifdef PLATFORM_MS316
typedef struct{
	unsigned int VerChk_Version;
	unsigned long long udid;
	unsigned int VerChk_Size;
} __attribute__ ((__packed__)) MSYS_UDID_INFO;
#define MSYS_IOCTL_MAGIC             'S'
#define IOCTL_MSYS_GET_UDID                 _IO(MSYS_IOCTL_MAGIC, 0x32)

static S8* MStar_GetUDID()
{
	int fd,ret;
	MSYS_UDID_INFO tudid;
	U8 *pU8Bytes = (U8 *)(&tudid.udid);
	if(g_szSerialUUID_CPU[0] != 0)
	{
		return g_szSerialUUID_CPU;
	}
	fd = open("/dev/msys",O_RDWR);
	if(fd < 0)
	{
		printf("open /dev/msys failed \n");
		return NULL;
	}
	memset(&tudid,0,sizeof(tudid));
	tudid.VerChk_Version = 0x4d530000|0x0100;
	tudid.VerChk_Size = sizeof(MSYS_UDID_INFO);
	ret = ioctl(fd,IOCTL_MSYS_GET_UDID,&tudid);
	if(ret)
	{
		close(fd);
		printf("open /dev/msys IOCTL_MSYS_GET_UDID failed <%d>\n",ret);
		return NULL;
	}

	close(fd);
	sprintf(g_szSerialUUID_CPU,"%02x%02x%02x%02x%02x%02x%02x%02x",pU8Bytes[7],pU8Bytes[6],pU8Bytes[5],pU8Bytes[4],pU8Bytes[3],pU8Bytes[2],pU8Bytes[1],pU8Bytes[0]);
	printf("mstar uuid [%s]\n",g_szSerialUUID_CPU);
	return g_szSerialUUID_CPU;
}
#endif


#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || defined PLATFORM_JZT40 || defined PLATFORM_JZT41

static char *JZT_GetUUID()
{
	if(g_szSerialUUID_CPU[0] != 0)
	{
		return g_szSerialUUID_CPU;
	}

	int fdIav = open("/tmp/boarduuid.txt", O_RDONLY);
	if( fdIav < 0 )
	{
		printf("not found /tmp/boarduuid.txt\n");
		return NULL;
	}

	read(fdIav,g_szSerialUUID_CPU,sizeof(g_szSerialUUID_CPU));

	int len = strlen(g_szSerialUUID_CPU);
	if(len > 0 )
		g_szSerialUUID_CPU[len-1] = 0;

	printf("JZT uuid :%s\n",g_szSerialUUID_CPU);

	return g_szSerialUUID_CPU;
}

#endif


static S8 *Hisi_GetUUID();
static S8 * Module_GetUUID()
{
	S8 *pRetString = NULL,*szCpuUUID = NULL,*szSwUUID = NULL;
	cJSON_Struct *pInfo = NULL;
	S8 *szLicence = NULL;
	int bHasVersion = 0,bHasSvrVersion = 0;

	if(g_szSerialUUID[0] != 0)
	{
		return g_szSerialUUID;
	}

#ifdef PLATFORM_MS316
	szCpuUUID = MStar_GetUDID();

#elif defined PLATFORM_JZT30
	szCpuUUID = JZT_GetUUID();
#elif defined PLATFORM_JZT32
	szCpuUUID = JZT_GetUUID();
#elif defined PLATFORM_JZT33
	szCpuUUID = JZT_GetUUID();
#elif defined PLATFORM_JZT40
	szCpuUUID = JZT_GetUUID();
#elif defined PLATFORM_JZT41
	szCpuUUID = JZT_GetUUID();
#else
	szCpuUUID = Hisi_GetUUID();
#endif

	// if(pRetString == NULL)
	{
		Module_FactoryInfo_Read(&pInfo);
		if(pInfo != NULL)
		{
		/*
		{
			"Licence":""
		}
		*/
		char *LicVersion = NULL;
			Common_Json_GetAttrValue(pInfo,-1,"Version",NULL,&LicVersion,NULL,NULL);
			if(LicVersion != NULL)
			{
				bHasVersion = 1;
			}
			Common_Json_GetAttrValue(pInfo,-1,"Licence",NULL,&szLicence,NULL,NULL);
			if(szLicence != NULL)
			{
				cJSON_Struct *pOutJson = NULL;
				S8 *szNewUUID = NULL,*szNewHardware = NULL,*szSerialNumber = NULL,*szCertificate=NULL,*szTime = NULL;
				if(0 == ovfs_Licence_dec(szLicence,&szNewHardware,&szNewUUID,&szSerialNumber,&szCertificate,&szTime,(void **)&pOutJson))
				{
					int bBadBoy = 0;
					if (szNewUUID != NULL)
					{
						char *szStringValue = NULL;

						snprintf(g_szSerialUUID_SW,sizeof(g_szSerialUUID_SW) - 1,"%s",szNewUUID);
						szSwUUID = g_szSerialUUID_SW;
						printf("sw uuid :<%s>\n",g_szSerialUUID_SW);

						Common_Json_GetAttrValue(pOutJson, -1, "SvrVersion", NULL,&szStringValue,NULL,NULL);
						if(szStringValue != NULL)
						{
							bHasSvrVersion = 1;
						}
						//printf("szStringValue = %s \n",szStringValue);
						if(szStringValue == NULL || (LicVersion == NULL) || (szStringValue != NULL && szCpuUUID == NULL))
						{

						}
					}

					Common_Free(szNewUUID,__FUNCTION__,__LINE__);
					Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
					Common_Free(szCertificate,__FUNCTION__,__LINE__);
					Common_Free(szTime,__FUNCTION__,__LINE__);
					Common_Free(szNewHardware,__FUNCTION__,__LINE__);

				}
				if (pOutJson != NULL)
				{
					Common_Json_Delete(pOutJson);
					pOutJson = NULL;
				}


			}
			Common_Json_Delete(pInfo);
			pInfo = NULL;
		}

	}
	if(szCpuUUID == NULL && szSwUUID == NULL)
	{
		pRetString = NULL;
	}
	else if(szSwUUID == NULL)
	{
		strcpy(g_szSerialUUID,szCpuUUID);
		pRetString = g_szSerialUUID;
	}
	else if(szCpuUUID == NULL)
	{
		strcpy(g_szSerialUUID,szSwUUID);
		pRetString = g_szSerialUUID;
	}
	else if(0 == strcmp(szCpuUUID,szSwUUID))
	{// UUID一致
		strcpy(g_szSerialUUID,szSwUUID);
		pRetString = g_szSerialUUID;
	}
	else
	{// szSwUUID != NULL && szCpuUUID != NULL
		if(bHasVersion)
		{// 授权时为新固件 以CPU UUID为主
			strcpy(g_szSerialUUID,szCpuUUID);
			pRetString = g_szSerialUUID;
		}
		else
		{// 授权时为老固件,以授权信息为主
			strcpy(g_szSerialUUID,szSwUUID);
			pRetString = g_szSerialUUID;
		}

	}
	if(pRetString != NULL)
	{
		printf("uuid :<%s>\n",pRetString);
	}

	return pRetString;
}
#define MAX_MTDBLOCK	6


static int BUFSIZE = 10240;
static S32 Module_Flash_ReadMtd(int iMtdBlockNum, char **pImageReadBuffer, int *iReadSize)
{
	int i,nRealSize,nWriteBlockSize;
	int ret = -1;
	int size;
	char device[32];
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	int nSleepCnt = 0,bNeedErase = 1,bIsBad = 0;
	loff_t offs;
	off_t seek;
	S32 bNandFlash = 0;
	S32 dev_fd = -1;
	S8 *pBuffer = NULL;
	S32 nNeedSize = 0;
    //printf("[%s.%d] here  bn = %d,iwritesize = %d,istartpos = %d,ierasesize = %d\n",__FUNCTION__,__LINE__,iMtdBlockNum,iWriteSize,iStartPos,iEraseSize);
	if(iMtdBlockNum < 0 || iMtdBlockNum >= MAX_MTDBLOCK)
	{
		printf("Param err : iMtdBlockNum = %d\n", iMtdBlockNum);
		return -1;
	}

	bNandFlash = 0;

	sprintf(device, "/dev/mtd%d", iMtdBlockNum);
	dev_fd = open(device, O_SYNC | O_RDWR);
	if(dev_fd < 0)
	{
		printf("Device open err : device = %s, ret = %d", device, dev_fd);
		return -1;
	}
	ret = ioctl(dev_fd, MEMGETINFO, &mtd);
	if(ret < 0)
	{
		printf("This doesn't seem to be a valid MTD flash device :%s ret = %d\n", device, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}
	mtd.writesize = mtd.size;
	if(pImageReadBuffer == NULL)
	{
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		if(iReadSize)
		{
			*iReadSize = mtd.size;
			return 0;
		}
		return -1;
	}

	if(mtd.type == MTD_NANDFLASH)
	{
		bNandFlash = 1;
	}
	else if(mtd.type != MTD_NORFLASH)
	{
		printf("This doesn't seem to be a valid MTD flash device :%s type = %d ret = %d\n", device,mtd.type, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}


	nNeedSize = mtd.size;

	pBuffer = (S8 *)Common_Malloc(nNeedSize,0,__FUNCTION__,__LINE__);
	if(pBuffer == NULL)
	{
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}



	S32 verifysize,totalverifysize;
	verifysize = 0;
	/******************** verify ********************/
	ret = lseek(dev_fd, 0, SEEK_SET);
	if(ret < 0)
	{
		printf("[%s.%d]While seeking to start of %s: %d\n", __FUNCTION__,__LINE__, device, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		Common_Free(pBuffer,__FUNCTION__,__LINE__);
			pBuffer = NULL;
		return -1;
	}
	size = mtd.size;
	verifysize = 0;
	seek = 0;
	while(size)
	{

		nRealSize = mtd.writesize;
		if(bNandFlash)
	 	{
	 	 // check nand bad block
		 	offs = seek;
		    ret = ioctl(dev_fd, MEMGETBADBLOCK, &offs);
			if (ret < 0)
		    {
				perror("ioctl(MEMGETBADBLOCK)\n");
				if(dev_fd > 0)
				{
					close(dev_fd);
					dev_fd = -1;
				}
				Common_Free(pBuffer,__FUNCTION__,__LINE__);
				pBuffer = NULL;
				return -1;
			}
			if (ret == 1)
			{

					fprintf (stderr, "Bad block  "
							"from %x will be skipped\n",
							(int) offs);

				seek += mtd.writesize;
				continue;
			}

	 	}
		ret = read(dev_fd, pBuffer + verifysize, mtd.writesize);
		if(ret != nRealSize)
		{
			if(ret < 0)
				printf("[%s.%d]While reading data from %s\n", __FUNCTION__,__LINE__, device);
			else
				printf("[%s.%d]Short read count returned while reading from %s\n", __FUNCTION__,__LINE__, device);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			Common_Free(pBuffer,__FUNCTION__,__LINE__);
			pBuffer = NULL;
			return -1;
		}

		seek += mtd.writesize;
		verifysize += mtd.writesize;
		size -= mtd.writesize;
	}
	if(dev_fd > 0)
	{
		close(dev_fd);
		dev_fd = -1;
	}
	*pImageReadBuffer = pBuffer;
	*iReadSize = mtd.size;
	//Common_Free(pBuffer,__FUNCTION__,__LINE__);
	//pBuffer = NULL;
	return 0;
}

static S32 Module_Flash_WriteMtd(int iMtdBlockNum, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize)
{
	int i,nRealSize,nWriteBlockSize;
	int ret = -1;
	int size;
	char device[32];
	char *src, *dest;//[BUFSIZE];
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	int nSleepCnt = 0,bNeedErase = 1,bIsBad = 0;
	loff_t offs;
	off_t seek;
	S32 bNandFlash = 0;
	S32 dev_fd = -1;
    //printf("[%s.%d] here  bn = %d,iwritesize = %d,istartpos = %d,ierasesize = %d\n",__FUNCTION__,__LINE__,iMtdBlockNum,iWriteSize,iStartPos,iEraseSize);
	if(iMtdBlockNum < 0 || iMtdBlockNum >= MAX_MTDBLOCK)
	{
		printf("Param err : iMtdBlockNum = %d\n", iMtdBlockNum);
		return -1;
	}

	bNandFlash = 0;

	sprintf(device, "/dev/mtd%d", iMtdBlockNum);
	dev_fd = open(device, O_SYNC | O_RDWR);
	if(dev_fd < 0)
	{
		printf("Device open err : device = %s, ret = %d", device, dev_fd);
		return -1;
	}
	ret = ioctl(dev_fd, MEMGETINFO, &mtd);
	if(ret < 0)
	{
		printf("This doesn't seem to be a valid MTD flash device :%s ret = %d\n", device, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}

	if(mtd.type == MTD_NANDFLASH)
	{
		bNandFlash = 1;
	}
	else if(mtd.type != MTD_NORFLASH)
	{
		printf("This doesn't seem to be a valid MTD flash device :%s type = %d ret = %d\n", device,mtd.type, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}
	if(iWriteSize > mtd.size)
	{
		printf("Size %d won't fit into %s : device size = %d\n", iWriteSize, device, mtd.size);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}
	if(iStartPos % mtd.erasesize != 0)
	{
		printf("Start pos %d won't fit into %s : device size = %d\n", iStartPos, device, mtd.size);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}
	if(iEraseSize % mtd.erasesize != 0)
	{
		printf("Erase pos %d won't fit into %s : device size = %d\n", iStartPos, device, mtd.size);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return -1;
	}
	/******************** erase ********************/
	S32 blocks = 0;
	erase.start = iStartPos;
	if(iEraseSize == 0)
		erase.length = mtd.size - iStartPos;
	else
		erase.length = iEraseSize;
	blocks = erase.length / mtd.erasesize;
	erase.length = mtd.erasesize;
	for(i = 1; i <= blocks; i++)
	{
		bNeedErase = 1;
	 	if(bNandFlash)
	 	{
	 	 // check nand bad block
		 	offs = erase.start;
		    ret = ioctl(dev_fd, MEMGETBADBLOCK, &offs);
			if (ret < 0)
		    {
				perror("ioctl(MEMGETBADBLOCK)\n");
				if(dev_fd > 0)
				{
					close(dev_fd);
					dev_fd = -1;
				}
				return -1;
			}
			if (ret == 1)
			{

					fprintf (stderr, "Bad block at %x block(s) "
							"from %x will be skipped\n",
							i,(int) offs);

				bNeedErase = 0;
			}

	 	}
		if(bNeedErase && ioctl(dev_fd, MEMERASE, &erase) < 0)
		{

			printf("[%s.%d]While erasing blocks 0x%.8x-0x%.8x on %s: %m\n", __FUNCTION__,__LINE__, (unsigned int)erase.start, (unsigned int)(erase.start + erase.length), device);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			return -1;
		}
		erase.start += mtd.erasesize;
		nSleepCnt += mtd.erasesize;
		if(nSleepCnt > 512 * 1024)
		{
			nSleepCnt =0;
			usleep(10000);
		}
	}
	S32 writesize,totalsize;
	writesize = 0;
	totalsize = iWriteSize;
    BUFSIZE = mtd.erasesize;

	if(pImageBuffer == NULL && iWriteSize == 0)
	{
		printf("No data to Write");
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		return 0;
	}
	nWriteBlockSize = mtd.writesize;
	if(!bNandFlash)
	{
		nWriteBlockSize = 1024 * 1024 /mtd.writesize * mtd.writesize;
	}
	dest = (char *)malloc(nWriteBlockSize);
	if(dest == NULL)
	{
		close(dev_fd);
		dev_fd = -1;
		return -1;
	}

	/******************** write ********************/
	i = BUFSIZE;
	src = pImageBuffer;
	size = iWriteSize;
	totalsize = iWriteSize;
	if(iStartPos != 0)
	{
		ret = lseek(dev_fd, iStartPos, SEEK_SET);
		if(ret < 0)
		{
			printf("[%s.%d]While seeking to start of %s: %d\n", __FUNCTION__,__LINE__, device, ret);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			Common_Free(dest,__FUNCTION__,__LINE__);
			dest = NULL;
			return -1;
		}
	}
	seek = iStartPos;
	while(size > 0)
	{
		if(seek >= mtd.size)
		{
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			Common_Free(dest,__FUNCTION__,__LINE__);
				dest = NULL;
			return -1;
		}
		nRealSize = nWriteBlockSize;
		if (size < nWriteBlockSize)
		{
            memset(dest + size, 0xff, nWriteBlockSize - size);
            memcpy(dest, src, size);
			nRealSize = size;
			nWriteBlockSize = (nRealSize + mtd.writesize - 1)/ mtd.writesize * mtd.writesize;

		}

		bIsBad = 0;
		if(bNandFlash)
	 	{
	 	 // check nand bad block
		 	offs = seek;
		    ret = ioctl(dev_fd, MEMGETBADBLOCK, &offs);
			if (ret < 0)
		    {
				perror("ioctl(MEMGETBADBLOCK)\n");
				if(dev_fd > 0)
				{
					close(dev_fd);
					dev_fd = -1;
				}
				Common_Free(dest,__FUNCTION__,__LINE__);
				dest = NULL;
				return -1;
			}
			if (ret == 1)
			{

					fprintf (stderr, "Bad block  "
							"from %x will be skipped\n",
							(int) offs);

				bIsBad = 1;
			}

	 	}
		if(bIsBad)
		{
			seek += nWriteBlockSize;
			continue;
		}
		if(lseek(dev_fd, seek, SEEK_SET) != seek)
		{
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			Common_Free(dest,__FUNCTION__,__LINE__);
				dest = NULL;
			return -1;
		}

		ret = write(dev_fd, (nRealSize != nWriteBlockSize)?dest:src, nWriteBlockSize);
		if(nWriteBlockSize != ret)
		{
			if(ret < 0)
			{
				printf("[%s.%d]While writing data to 0x%08x-0x%08x on %s\n", __FUNCTION__,__LINE__,writesize, writesize + nWriteBlockSize, device);
			}
			else
			{
				printf("[%s.%d]Short write count returned while writing to x%08x-0x%08x on %s: %d/%d bytes written to flash\n", __FUNCTION__,__LINE__, writesize, writesize + nWriteBlockSize, device, writesize + ret, iWriteSize);
			}
			// set bad block for nand flash
			if(bNandFlash)
	 	    {
	 	    	loff_t bad_addr = seek;
				fprintf (stdout,"Marking block at %08lx bad\n", (long)bad_addr);
				if (ioctl(dev_fd, MEMSETBADBLOCK, &bad_addr)) {
						fprintf (stdout,"MEMSETBADBLOCK");
					/* But continue anyway */
				}
				seek += nWriteBlockSize;
	 	    	continue;
			}
			else
			{
				if(dev_fd > 0)
				{
					close(dev_fd);
					dev_fd = -1;
				}
				Common_Free(dest,__FUNCTION__,__LINE__);
				dest = NULL;
				return -1;
			}
		}
        seek += nWriteBlockSize;
		writesize += nWriteBlockSize;
		size -= nWriteBlockSize;
		src += nWriteBlockSize;
		nSleepCnt += nWriteBlockSize;
		if(nSleepCnt >= 512 * 1024)
		{
			nSleepCnt =0;
			usleep(10000);
		}
	}
	S32 verifysize,totalverifysize;
	verifysize = 0;
	totalverifysize = iWriteSize;
	/******************** verify ********************/
	ret = lseek(dev_fd, iStartPos, SEEK_SET);
	if(ret < 0)
	{
		printf("[%s.%d]While seeking to start of %s: %d\n", __FUNCTION__,__LINE__, device, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		Common_Free(dest,__FUNCTION__,__LINE__);
			dest = NULL;
		return -1;
	}
	src = pImageBuffer;
	size = iWriteSize;
	i = mtd.writesize;
	verifysize = 0;
	totalverifysize = iWriteSize;
	seek = iStartPos;
	while(size)
	{
		if(size < mtd.writesize)
			i = size;

		nRealSize = mtd.writesize;
		if(bNandFlash)
	 	{
	 	 // check nand bad block
		 	offs = seek;
		    ret = ioctl(dev_fd, MEMGETBADBLOCK, &offs);
			if (ret < 0)
		    {
				perror("ioctl(MEMGETBADBLOCK)\n");
				if(dev_fd > 0)
				{
					close(dev_fd);
					dev_fd = -1;
				}
				Common_Free(dest,__FUNCTION__,__LINE__);
				dest = NULL;
				return -1;
			}
			if (ret == 1)
			{

					fprintf (stderr, "Bad block  "
							"from %x will be skipped\n",
							(int) offs);

				seek += mtd.writesize;
				continue;
			}

	 	}
		ret = read(dev_fd, dest, nRealSize);
		if(ret != nRealSize)
		{
			if(ret < 0)
				printf("[%s.%d]While reading data from %s\n", __FUNCTION__,__LINE__, device);
			else
				printf("[%s.%d]Short read count returned while reading from %s\n", __FUNCTION__,__LINE__, device);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			Common_Free(dest,__FUNCTION__,__LINE__);
			dest = NULL;
			return -1;
		}
		if(memcmp(src, dest, i))
		{
			printf("[%s.%d]File does not seem to match flash data. First mismatch at 0x%08x-0x%08x\n", __FUNCTION__,__LINE__, verifysize, verifysize + i);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			Common_Free(dest,__FUNCTION__,__LINE__);
			dest = NULL;
			return -1;
		}
		seek += mtd.writesize;
		verifysize += i;
		size -= i;
		src += i;
	}
	if(dev_fd > 0)
	{
		close(dev_fd);
		dev_fd = -1;
	}
	Common_Free(dest,__FUNCTION__,__LINE__);
	dest = NULL;
	return 0;
}


S32 Module_FactoryInfo_Read(cJSON_Struct** pInfo)
{
	 int  iMtdBlockNum =-1;
   char cmd[128];
   S8	buf[64];
   S8 *pStr = NULL;
   S32 nLen = 0,nRet = 0;

   if(pInfo == NULL)
   	{
   		return -1;
   	}

   if(Common_File_IsExist("/proc/mtd"))
	{
	   snprintf(cmd,sizeof(cmd),"cat /proc/mtd | grep \"%s\" | awk \'{print $1}\'","factory");

	   if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
	   {
		   if (0 == Common_StrnCmp(buf,"mtd",strlen("mtd")))
		   {
			   iMtdBlockNum = atoi(buf+strlen("mtd"));
		   }
	   }

	   nRet =  Module_Flash_ReadMtd(iMtdBlockNum,&pStr,&nLen);
	   if(pStr != NULL)
	   {
	   	 *pInfo = Common_Json_Parse(pStr,NULL,NULL);
	   	 Common_Free(pStr,__FUNCTION__,__LINE__);
	   }
   }
   else
	{
		nRet  -1;
		snprintf(cmd,sizeof(cmd),"cat /proc/mounts | grep \"update\"");
		 if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
	   {
		   if (NULL != strstr(buf,"/mmcblk0"))
		   {
			  // emmc : /dev/mmcblk0p2
			   struct statfs diskInfo;
				if(0 != statfs("/dev/mmcblk0p2",&diskInfo))
				{
					return -1;
				}
			    unsigned long long totalBlocks = diskInfo.f_bsize;
			    unsigned long long totalDisk = diskInfo.f_bavail*totalBlocks;
				// printf("totalDisk = %lld /%lld/%d\n",totalDisk,totalBlocks,diskInfo.f_bavail);
				nLen = 1024 * 1024;
				if(nLen > totalDisk)
				{
					nLen = totalDisk;
				}
				pStr = (S8 *)Common_Malloc(nLen,0,NULL,0);
				if(pStr == NULL)
				{
					return -1;
				}
				int nReadLen;
			   int fd = open("/dev/mmcblk0p2",O_RDWR);
			   if(fd > 0)
			   {
			   		nRet = read(fd,pStr,nLen);
					//printf("nReadLen == %d \n",nRet);
					if(nRet > 0)
					{
						*pInfo = Common_Json_Parse(pStr,NULL,NULL);
			   	 		Common_Free(pStr,__FUNCTION__,__LINE__);
					}

				   close(fd);
			   }
		   }
	   }
	}
   return nRet;
}
S32 Module_FactoryInfo_Write(cJSON_Struct* pInfo)
{
   int  iMtdBlockNum =-1;
   char cmd[128];
   S8	buf[64];
   S8 *pStr = NULL;
   S32 nLen = 0,nRet = 0;


	if(Common_File_IsExist("/proc/mtd"))
	{

	   snprintf(cmd,sizeof(cmd),"cat /proc/mtd | grep \"%s\" | awk \'{print $1}\'","factory");

	   if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
	   {
		   if (0 == Common_StrnCmp(buf,"mtd",strlen("mtd")))
		   {
			   iMtdBlockNum = atoi(buf+strlen("mtd"));
		   }
	   }
	   if(pInfo != NULL)
	   {
	   	pStr = Common_Json_Print(pInfo,&nLen);
		if(pStr != NULL)
		{
			nLen++;
		}
	   }
	   nRet =  Module_Flash_WriteMtd(iMtdBlockNum,pStr,nLen,0,0);
	   if(pStr != NULL)
	   {
	   	Common_Free(pStr,__FUNCTION__,__LINE__);
	   }
	}
	else
	{
		nRet  -1;
		snprintf(cmd,sizeof(cmd),"cat /proc/mounts | grep \"update\"");
		 if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
	   {
		   if (NULL != strstr(buf,"/mmcblk0"))
		   {
			   // emmc : /dev/mmcblk0p2
			int fd = open("/dev/mmcblk0p2",O_RDWR);
			if(fd > 0)
			{
				if(pInfo != NULL)
			   {
			   	pStr = Common_Json_Print(pInfo,&nLen);
				if(pStr != NULL)
				{
					nLen++;
					write(fd,pStr,nLen);
					Common_Free(pStr,__FUNCTION__,__LINE__);
					nRet = 0;
				}
			   }
				close(fd);
			}
		   }
	   }
	}
   return nRet;

}

static S8 *Hisi_GetUUID()
{
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || defined PLATFORM_JZT40 || defined PLATFORM_MS316
	return NULL;
#else
	char *pRetString = NULL,*szHisUUID = NULL;
	// 有校 id 才开始检查uuid

		if(g_szSerialUUID_CPU[0] != 0)
		{
			return g_szSerialUUID_CPU;
		}
			{
		// 读海思DIE ID
				void *pHiDieID_viraddr = NULL; /* volatile */

				off_t pageSize = 0;
				size_t nDieID_actualLen = 0;
				off_t nDieID_actualOffset = 0,nDieID_offset = 0,nDieID_Size = 0;
#ifdef PLATFORM_HI3516CV500
				nDieID_offset = 0x12020000+0x400;
				nDieID_Size = 24;

#endif
#ifdef PLATFORM_HI3516EV200
				nDieID_offset = 0x12020000+0x400;
				nDieID_Size = 24;

#endif
				if(nDieID_offset != 0)
				{
					int fd = -1;
					fd = open("/dev/mem", O_RDWR | O_SYNC);
					if(fd >= 0)
					{
						pageSize = getpagesize();
						nDieID_actualOffset = nDieID_offset & ~(pageSize - 1);		   // 找到对应的段起始
						nDieID_actualLen	= nDieID_Size + (nDieID_offset - nDieID_actualOffset);	 // 判断实际需要 mmap 的长度

						pHiDieID_viraddr = mmap(NULL,nDieID_actualLen,PROT_READ,MAP_SHARED,fd,nDieID_actualOffset);

						if(pHiDieID_viraddr)
						{
							unsigned int *pByte = (unsigned int *)((unsigned char *)pHiDieID_viraddr + nDieID_offset - nDieID_actualOffset);
							int i,pos = 0,bZero = 1;
							//printf("[%s.%d]herepByte = %p nDieID_Size = %d / %d pageSize = %d nDieID_actualLen = %d pHiDieID_viraddr = %p\n",__FUNCTION__,__LINE__,pByte,nDieID_Size,nDieID_offset - nDieID_actualOffset,pageSize,nDieID_actualLen ,pHiDieID_viraddr);
							for(i = 0; i < nDieID_Size / 4;i++)
							{
								if(bZero == 1 && pByte[i])
								{
									bZero = 0;
								}
								pos += sprintf(g_szSerialUUID_CPU + pos, "%08x",pByte[i]);
							}
							g_szSerialUUID_CPU[pos] = 0;
							printf("his uuid :<%s>\n",g_szSerialUUID_CPU);
							if(bZero)
							{
								pos = 0;
								g_szSerialUUID_CPU[pos] = 0;
							}
							else
							{
								pRetString = g_szSerialUUID_CPU;
								szHisUUID = g_szSerialUUID_CPU;
								printf("his uuid [%s]\n",g_szSerialUUID_CPU);
							}


						}
					}
					if (fd)
					{
						   close(fd);
						   fd = -1;
					}
					if (pHiDieID_viraddr)
					{
						munmap(pHiDieID_viraddr, nDieID_actualLen);
						pHiDieID_viraddr = NULL;
					}
				}
			}
	return pRetString;
#endif
}

static Common_Lock_T g_AuthBurnLock = NULL;
S32 Module_VersionAuth_burn(S8 *szAuthInfo)
{
	cJSON_Struct *pInfo = NULL,*pJsonBurnInfo = NULL;
	S8 *szNewUUID = NULL,*szNewHardware = NULL,*szSerialNumber = NULL,*szCertificate=NULL,*szTime = NULL;
	S32 nBadBoy = 0,nMainLevel = 0;
	S8 *strTmp = NULL;
	if(szAuthInfo == NULL)
	{
		return -1;
	}
	if(g_AuthBurnLock == NULL)
	{
		Common_Lock_Create(&g_AuthBurnLock,"AuthBurnLock");
	}
	Common_Lock(g_AuthBurnLock);

	if(0 != ovfs_Licence_dec(szAuthInfo,&szNewHardware,&szNewUUID,&szSerialNumber,&szCertificate,&szTime,&pJsonBurnInfo))
	{
		Common_UnLock(g_AuthBurnLock);
		Common_Free(szNewUUID,__FUNCTION__,__LINE__);
		Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
		Common_Free(szCertificate,__FUNCTION__,__LINE__);
		Common_Free(szTime,__FUNCTION__,__LINE__);
		return -1;
	}
	if (/*szSerialNumber == NULL ||
		szCertificate == NULL || */
		szNewUUID == NULL ||
		szNewHardware == NULL ||
		szTime == NULL ||
		pJsonBurnInfo == NULL)
	{
		Common_UnLock(g_AuthBurnLock);
		Common_Free(szNewHardware,__FUNCTION__,__LINE__);
		Common_Free(szNewUUID,__FUNCTION__,__LINE__);
		Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
		Common_Free(szCertificate,__FUNCTION__,__LINE__);
		Common_Free(szTime,__FUNCTION__,__LINE__);
		Common_Json_Delete(pJsonBurnInfo);
		return -1;
	}
	if (Module_GetSerialNumber(1) != NULL)
	{// UUID 存在时才采用UUID方式 ，否则使用非uuid方式
		printf("###[%s:%d]\n", __FUNCTION__, __LINE__);
		if(szSerialNumber == NULL || szCertificate == NULL ||szNewUUID == NULL|| 0 != Common_StrCmp(szNewUUID,Module_GetSerialNumber(1)))
		{
			Common_UnLock(g_AuthBurnLock);
			Common_Free(szNewHardware,__FUNCTION__,__LINE__);
			Common_Free(szNewUUID,__FUNCTION__,__LINE__);
			Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
			Common_Free(szCertificate,__FUNCTION__,__LINE__);
			Common_Free(szTime,__FUNCTION__,__LINE__);
			Common_Json_Delete(pJsonBurnInfo);
			return -1;
		}
	}
		strTmp = NULL;
		Common_Json_GetAttrValue(pJsonBurnInfo,-1,"BadBoy",NULL,&strTmp,NULL,NULL);
		if (strTmp != NULL)
		{
			nBadBoy = atoi(strTmp);
		}
		strTmp = NULL;
		Common_Json_GetAttrValue(pJsonBurnInfo,-1,"MainLevel",NULL,&strTmp,NULL,NULL);
		if (strTmp != NULL)
		{
			nMainLevel = atoi(strTmp);
		}
		Common_Json_Delete(pJsonBurnInfo);
		pJsonBurnInfo = NULL;
		if (szSerialNumber != NULL && szCertificate != NULL)
		{
			// 烧 授权
			// 获取一下看是否需要烧录
			if(Module_GetSerialNumber(0) != NULL)
			{
				if(0 == Common_StrCmp(szSerialNumber,Module_GetSerialNumber(0)))
				{
					if (!nBadBoy)
					{
						Common_UnLock(g_AuthBurnLock);
						Common_Free(szNewHardware,__FUNCTION__,__LINE__);
						Common_Free(szNewUUID,__FUNCTION__,__LINE__);
						Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
						Common_Free(szCertificate,__FUNCTION__,__LINE__);
						Common_Free(szTime,__FUNCTION__,__LINE__);
						return 0;
					}

				}
				else
				{// 序列号与要烧录的不同? 重新烧录更换? 还是认为是盗版
					if (nBadBoy)
					{
					}
					else
					{
#if 0
						// 破坏掉，需要重新授权
						pInfo = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
						if(pInfo == NULL)
						{
							Common_Lock(g_AuthBurnLock);
							Common_Free(szNewHardware,__FUNCTION__,__LINE__);
							Common_Free(szNewUUID,__FUNCTION__,__LINE__);
							Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
							Common_Free(szCertificate,__FUNCTION__,__LINE__);
							Common_Free(szTime,__FUNCTION__,__LINE__);
							return -1;
						}
						Common_Json_SetAttrValue(pInfo,-1,"Licence",Common_Json_Type_String,"xxx",0,0);
						Module_FactoryInfo_Write(pInfo);
						Common_Json_Delete(pInfo);
						pInfo = NULL;
						return -1;
#else
						// 不更换
						Common_UnLock(g_AuthBurnLock);
						Common_Free(szNewHardware,__FUNCTION__,__LINE__);
						Common_Free(szNewUUID,__FUNCTION__,__LINE__);
						Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
						Common_Free(szCertificate,__FUNCTION__,__LINE__);
						Common_Free(szTime,__FUNCTION__,__LINE__);
						return -1;
#endif
					}
				}
			}
		}
		else
		{
			// 烧 uuid
			// 获取一下看是否需要烧录
			if(Module_GetSerialNumber(1) != NULL)
			{
				// 不更换
				Common_UnLock(g_AuthBurnLock);
				Common_Free(szNewHardware,__FUNCTION__,__LINE__);
				Common_Free(szNewUUID,__FUNCTION__,__LINE__);
				Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
				Common_Free(szCertificate,__FUNCTION__,__LINE__);
				Common_Free(szTime,__FUNCTION__,__LINE__);
				return -1;

			}
		}

	pInfo = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pInfo == NULL)
	{
		Common_UnLock(g_AuthBurnLock);
		Common_Free(szNewHardware,__FUNCTION__,__LINE__);
		Common_Free(szNewUUID,__FUNCTION__,__LINE__);
		Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
		Common_Free(szCertificate,__FUNCTION__,__LINE__);
		Common_Free(szTime,__FUNCTION__,__LINE__);
		return -1;
	}
	Common_Json_SetAttrValue(pInfo,-1,"Licence",Common_Json_Type_String,szAuthInfo,0,0);
    printf("----->g_szSerialUUID_CPU:[%s]\n",g_szSerialUUID_CPU);
    if(g_szSerialUUID_CPU[0] != 0)
    {
	    Common_Json_SetAttrValue(pInfo,-1,"Version",Common_Json_Type_String,"2.0",0,0);
    }

	Module_FactoryInfo_Write(pInfo);
	Common_Json_Delete(pInfo);
	pInfo = NULL;

	if (szSerialNumber != NULL && szCertificate != NULL)
	{
		g_szSerialNumber[0] = 0;
		// 获取一下看是否ok
		if(Module_GetSerialNumber(0) == NULL)
		{
			Common_UnLock(g_AuthBurnLock);
			Common_Free(szNewHardware,__FUNCTION__,__LINE__);
			Common_Free(szNewUUID,__FUNCTION__,__LINE__);
			Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
			Common_Free(szCertificate,__FUNCTION__,__LINE__);
			Common_Free(szTime,__FUNCTION__,__LINE__);
			return nBadBoy?0:-1;
		}
		else if(0 != Common_StrCmp(szSerialNumber,Module_GetSerialNumber(0)))
		{
			Common_UnLock(g_AuthBurnLock);
			Common_Free(szNewHardware,__FUNCTION__,__LINE__);
			Common_Free(szNewUUID,__FUNCTION__,__LINE__);
			Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
			Common_Free(szCertificate,__FUNCTION__,__LINE__);
			Common_Free(szTime,__FUNCTION__,__LINE__);
			return -1;
		}
	}
	else if (szNewUUID != NULL)
	{
		g_szSerialUUID[0] = 0;
		// 获取一下看是否ok
		if(Module_GetSerialNumber(1) == NULL)
		{
			Common_UnLock(g_AuthBurnLock);
			Common_Free(szNewHardware,__FUNCTION__,__LINE__);
			Common_Free(szNewUUID,__FUNCTION__,__LINE__);
			Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
			Common_Free(szCertificate,__FUNCTION__,__LINE__);
			Common_Free(szTime,__FUNCTION__,__LINE__);
			return -1;
		}
		else if(0 != Common_StrCmp(szNewUUID,Module_GetSerialNumber(1)))
		{
			Common_UnLock(g_AuthBurnLock);
			Common_Free(szNewHardware,__FUNCTION__,__LINE__);
			Common_Free(szNewUUID,__FUNCTION__,__LINE__);
			Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
			Common_Free(szCertificate,__FUNCTION__,__LINE__);
			Common_Free(szTime,__FUNCTION__,__LINE__);
			return -1;
		}
	}
	Common_UnLock(g_AuthBurnLock);
	Common_Free(szNewHardware,__FUNCTION__,__LINE__);
	Common_Free(szNewUUID,__FUNCTION__,__LINE__);
	Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
	Common_Free(szCertificate,__FUNCTION__,__LINE__);
	Common_Free(szTime,__FUNCTION__,__LINE__);

	return nBadBoy?-1:0;

}
S8 *Module_VersionAuth_GetMethod()
{
#if defined(PLATFORM_MS316) || defined(PLATFORM_HI3516CV500) || defined(PLATFORM_HI3516EV200) || defined(PLATFORM_JZT30) || defined(PLATFORM_JZT32) || defined(PLATFORM_JZT33) || defined(PLATFORM_JZT40) || defined(PLATFORM_JZT41)
	static S8 *szMethod = "UUID";
#else
	static S8 *szMethod = "LEVEL0";
#endif
	return szMethod;
}
S32 Module_VersionAuth_Erase()
{
	cJSON_Struct *pInfo = NULL,*pJsonBurnInfo = NULL;
	S8 *szNewUUID = NULL,*szNewHardware = NULL,*szSerialNumber = NULL,*szCertificate=NULL,*szTime = NULL;
	S32 nBadBoy = 0,nMainLevel = 0;
	S8 *strTmp = NULL;

	if(g_AuthBurnLock == NULL)
	{
		Common_Lock_Create(&g_AuthBurnLock,"AuthBurnLock");
	}



	// 获取一下看是否需要烧录

	pInfo = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pInfo == NULL)
	{

		return -1;
	}
	Common_Json_SetAttrValue(pInfo,-1,"Licence",Common_Json_Type_String,"xxx",0,0);
	Common_Lock(g_AuthBurnLock);
	Module_FactoryInfo_Write(pInfo);
	Common_Json_Delete(pInfo);
	pInfo = NULL;

	g_szSerialNumber[0] = 0;
    Common_UnLock(g_AuthBurnLock);
	// 获取一下看是否ok
	if(Module_GetSerialNumber(0) == NULL)
	{


		return 0;
	}
	else
	{
		return -1;
	}


}

S8* Module_GetSerialNumber(S32 nIndex /*= 0 */)
{
	S8 *pRetString = NULL;
#ifdef WIN32
#else
	if(nIndex == 0)
	{
		U8 szInfo[20] = {0};
		U8 szLot[8];
		S32 i = 0;
		if (g_szSerialNumber[0] != 0)
		{
			return g_szSerialNumber;
		}
		if(0 == GetUserZoneInf(2, (unsigned char*)szInfo, 16))
		{
			GetLot((unsigned char*)szLot);

			sprintf(g_szSerialNumber,"%02x%02x",szInfo[10],szInfo[11]);
			sprintf(g_szSerialNumber + 4,"%02x%02x%02x%02x%02x%02x%02x%02x",szLot[0],szLot[1],szLot[2],szLot[3],szLot[4],szLot[5],szLot[6],szLot[7]);
			pRetString = g_szSerialNumber;
		}
		else
		{// 检查烧录文件
			S8 *szUUID = Module_GetUUID();
			if (szUUID != NULL)
			{

			// 有校 id 才开始检查uuid
				cJSON_Struct *pInfo = NULL;
				S8 *szLicence = NULL;
				Module_FactoryInfo_Read(&pInfo);
				if(pInfo != NULL)
				{
				/*
				{
					"Licence":""
				}
				*/
					Common_Json_GetAttrValue(pInfo,-1,"Licence",NULL,&szLicence,NULL,NULL);
					if(szLicence != NULL)
					{
						cJSON_Struct *pOutJson = NULL;
						S8 *szNewUUID = NULL,*szNewHardware = NULL,*szSerialNumber = NULL,*szCertificate=NULL,*szTime = NULL;
						if(0 == ovfs_Licence_dec(szLicence,&szNewHardware,&szNewUUID,&szSerialNumber,&szCertificate,&szTime,(void **)&pOutJson))
						{
							int bBadBoy = 0;
							if (pOutJson != NULL)
							{
								Common_Json_GetAttrValue(pOutJson,-1,"BadBoy",NULL,NULL,&bBadBoy,NULL);
							}
							g_bBadBoy = bBadBoy;
							if (szSerialNumber != NULL)
							{
								if(0 == Common_StrCmp(szNewUUID,szUUID) && (!bBadBoy))
								{
									strcpy(g_szSerialNumber,szSerialNumber);

									pRetString = g_szSerialNumber;

								}
							}

							Common_Free(szNewUUID,__FUNCTION__,__LINE__);
							Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
							Common_Free(szCertificate,__FUNCTION__,__LINE__);
							Common_Free(szTime,__FUNCTION__,__LINE__);
							Common_Free(szNewHardware,__FUNCTION__,__LINE__);

						}
						if (pOutJson != NULL)
						{
							Common_Json_Delete(pOutJson);
							pOutJson = NULL;
						}


					}
					Common_Json_Delete(pInfo);
					pInfo = NULL;
				}

		    }
		}

	}
	else if(nIndex == 1)
	{
		pRetString = Module_GetUUID();
		if(pRetString != NULL)
		{
			// 加密
		}
	}
#endif
	return pRetString;
}


static S32 ModuleOpenSocket(S32 bUnixLocal,char *pszDomainName,S32 nPort)
{
	struct sockaddr_in addr;
	S32 nRet;
	S32 nListenSocket = -1;
	MODULE_LOGD("unix=%d  [%s] nport = %d\n",bUnixLocal,pszDomainName,nPort);
	if (bUnixLocal)
	{
		if (pszDomainName == NULL)
		{
			return -1;
		}
#ifdef WIN32
		return -1;
#else
		// create socket
		struct sockaddr_un srv_addr;
		int addrlen;
		nListenSocket = socket(PF_UNIX, SOCK_STREAM, 0);
		if (nListenSocket == -1)
		{
			return -1;
		}
		srv_addr.sun_family=AF_UNIX;
		strncpy(srv_addr.sun_path,pszDomainName,sizeof(srv_addr.sun_path)-1);
		srv_addr.sun_path[0] = 0;
		addrlen =strlen(pszDomainName)   + offsetof(struct sockaddr_un, sun_path);
		if(bind(nListenSocket,(struct sockaddr *)&srv_addr,addrlen) == -1)
		{
			closeSocket(nListenSocket);
			nListenSocket = -1;
			MODULE_LOGE("Bind local unix failed [%s]\n",pszDomainName);
			return -1;
		}
#endif
	}
	else
	{


		// create socket
		nListenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (nListenSocket == -1)
		{
			return -1;
		}

		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		addr.sin_port = htons(nPort);
#if 1
		S32 val = 1;
		if(setsockopt(nListenSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
		{
			MODULE_LOGE("reuse failed port = %d! [%s]\n",nPort,strerror(GetLastError()));
			closeSocket(nListenSocket);
			nListenSocket = -1;
			return(-1);
		}
#endif
		if (bind(nListenSocket, (struct sockaddr*)&addr, sizeof addr) != 0)
		{
			MODULE_LOGE(" bind <%d> failed\n",nPort);
			closeSocket(nListenSocket);
			nListenSocket = -1;
			return -1;
		}
	}
#if defined(__WIN32__) || defined(_WIN32)
	unsigned long arg = 1;
	nRet =  ioctlsocket(nListenSocket, FIONBIO, &arg);
	if (nRet)
	{
		closeSocket(nListenSocket);
		nListenSocket = -1;
		return -1;
	}

#else
	S32 curFlags = fcntl(nListenSocket, F_GETFL, 0);
	nRet =  fcntl(nListenSocket, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(nListenSocket);
		nListenSocket = -1;
		return -1;
	}

#endif

	//开始监听
	if (listen(nListenSocket, 20) < 0)
	{
		closeSocket(nListenSocket);
		nListenSocket = -1;
		return -1;
	}
	return nListenSocket;
}

static S32 static_Require(S32 bUseUnixDomain,char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	struct sockaddr_in addr;
	S32 nRet;
	S32 selectResult;
	S32 nSocket = -1;
	S32 bTryConnect = 0,bWaitResult = 0;
	S32 nSendHeadPos = 0, nSendDataPos = 0,nSendDataSize;
	ModulePacketHeader_T tHeader;
	char *pSendBuffer = NULL;
	char *pRecvBuffer = NULL;
	S32 nRecvNeedSize = 0;
	S32 nRecvPos = 0,nRecvHeadPos = 0;
	struct timeval tv_timeToDelay;
	fd_set readSet,writeSet,exceptSet;
	S32 nResult = -1;
    char *pIFName = NULL;
#if 0
	if (pInParams != NULL)
	{
		S8 *pStrValue = NULL;
		MODULE_LOGD("InParam<%s>",pStrValue = Common_Json_Print(pInParams,NULL));
		Common_Free(pStrValue,__FUNCTION__,__LINE__);
	}
#endif

    Common_Json_GetAttrValueStr(pInParams, "Header/RemoteServerInfo/BindIF", &pIFName);
#ifdef WIN32
	if (bUseUnixDomain)
	{
		return -1;
	}
	if (pDomain == NULL || (nPort < 0 || nPort >= 0x0FFFF))
	{
		return -1;
	}
#else
	if (bUseUnixDomain && pDomain == NULL )
	{
		return -1;
	}
	errno = 0;

	if (bUseUnixDomain)
	{
		// create socket
		struct sockaddr_un srv_addr;
		int addrlen;
		nSocket = socket(PF_UNIX, SOCK_STREAM, 0);
		if (nSocket == -1)
		{
			return -1;
		}
		srv_addr.sun_family=AF_UNIX;
		strncpy(srv_addr.sun_path,pDomain,sizeof(srv_addr.sun_path)-1);
		srv_addr.sun_path[0] = 0;
		addrlen =strlen(pDomain)   + offsetof(struct sockaddr_un, sun_path);

		if(connect(nSocket,(struct sockaddr *)&srv_addr,addrlen) == -1)
		{
			MODULE_LOGD("Here[%s][%s]\n",pDomain,strerror(errno));
			closeSocket(nSocket);
			nSocket = -1;

			return -1;
		}

	}
	else if(nPort < 0 || nPort >= 0x0FFFF)
	{
		return -1;
	}
#endif
	if(!bUseUnixDomain)
	{


		// create socket
		nSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (nSocket == -1)
		{
			return -1;
		}
#ifdef WIN32
		unsigned long arg = 1;
		nRet =  ioctlsocket(nSocket, FIONBIO, &arg);
		if (nRet)
		{
			closeSocket(nSocket);
			nSocket = -1;
			return -1;
		}
		tcp_keepalive tkeepalive;
		DWORD dwBytesReturned;

		tkeepalive.onoff = 1;
		tkeepalive.keepalivetime = 60000;
		tkeepalive.keepaliveinterval = 15;
		if(WSAIoctl(nSocket,SIO_KEEPALIVE_VALS,&tkeepalive,sizeof(tkeepalive),NULL,0,&dwBytesReturned,NULL,NULL))
		{
			MODULE_LOGD("WSAIoctl failed\n");
		}
#else


		S32 curFlags = fcntl(nSocket, F_GETFL, 0);
		nRet =  fcntl(nSocket, F_SETFL, curFlags|O_NONBLOCK);
		if (nRet < 0 )
		{
			closeSocket(nSocket);
			nSocket = -1;
			return -1;
		}


		S32 keepalive = 1; // 开启keepalive属性
		S32 keepidle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
		S32 keepinterval = 15; // 探测时发包的时间间隔为5 秒
		S32 keepcount = 4; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
		setsockopt(nSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
		setsockopt(nSocket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
		setsockopt(nSocket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
		setsockopt(nSocket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

        if(pIFName)
        {
            struct ifreq ifr = {};
            strcpy(ifr.ifr_name, pIFName);
            setsockopt(nSocket, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));
        }

#endif


		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(pDomain);
		addr.sin_port = htons(nPort);

		if (connect(nSocket,(struct sockaddr *)&addr,sizeof(addr))!= 0)
		{

			S32 err = GetLastError();
			if(err == EINPROGRESS || err == EWOULDBLOCK)
			{
				bTryConnect = 1;
			}
			else
			{
				closeSocket(nSocket);
				nSocket = -1;
				return -1;
			}


		}



	}
	//

	tv_timeToDelay.tv_sec = nTimeOut/1000;
	tv_timeToDelay.tv_usec = (nTimeOut%1000) * 1000;
	while(1)
	{
		if (bTryConnect)
		{// 正在连接中
			FD_ZERO(&writeSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&writeSet);
			FD_SET(nSocket,&exceptSet);

			selectResult = select(nSocket + 1, NULL, &writeSet, &exceptSet, &tv_timeToDelay);

			if (selectResult < 0)
			{//错误

				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			if (selectResult == 0)
			{//超时

				break;
			}

			if (FD_ISSET(nSocket,&writeSet))
			{
				S32 nError,nRet;
				socklen_t nsize = sizeof(S32);

				nRet = getsockopt(nSocket, SOL_SOCKET, SO_ERROR, (char *)&nError, &nsize);

				if(nRet == 0 && nError == 0)
				{
					bTryConnect = 0;
					continue;
				}
				else
				{
					closeSocket(nSocket);
					nSocket = -1;
					break;
				}


			}

			if (FD_ISSET(nSocket,&exceptSet))
			{
				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			continue;
		}

		if (bWaitResult)
		{

			FD_ZERO(&readSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&readSet);
			FD_SET(nSocket,&exceptSet);


			S32 selectResult = select(nSocket + 1, &readSet,NULL, &exceptSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误
				MODULE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

				continue;
			}
			if (selectResult == 0)
			{//超时
				break;
			}
			if (FD_ISSET(nSocket,&exceptSet))
			{
				closeSocket(nSocket);
				nSocket = -1;
				break;

			}
			if (FD_ISSET(nSocket,&readSet))
			{
				S32 nRet;
				if (pRecvBuffer != NULL)
				{
					nRet = recv(nSocket,pRecvBuffer + nRecvPos,nRecvNeedSize - nRecvPos,0);
					if (nRet == 0)
					{
						closeSocket(nSocket);
						nSocket = -1;
						break;
					}
					else if (nRet < 0)
					{
						S32 errorNo = GetLastError();
						if (errorNo == EWOULDBLOCK ||
							errorNo == EINTR||
							errorNo == EAGAIN ||
							errorNo == ETIMEDOUT)
						{

							continue;
						}
						else
						{
							MODULE_ERROR("%s \n",strerror(errorNo));
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
					}
					nRecvPos += nRet;
					if (nRecvPos == nRecvNeedSize)
					{// 完整包
						cJSON_Struct *pRecvJson = NULL;
						// 处理包
						if (pInParams != NULL)
						{
							MODULE_LOGD("OutParam<%s>",pRecvBuffer + tHeader.uExternHeaderLen);
						}
						pRecvJson = Common_Json_Parse(pRecvBuffer + tHeader.uExternHeaderLen,NULL,NULL);
						if (pRecvJson != NULL)
						{
							if (pOutParams)
							{
								*pOutParams = pRecvJson;
								pRecvJson = NULL;
							}
							Common_Json_Delete(pRecvJson);
							pRecvJson = NULL;
							nResult = 0;
						}
						else
						{
							//
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
						break;

					}
				}
				else
				{
					nRet = recv(nSocket,((char *)&tHeader) + nRecvHeadPos,sizeof(ModulePacketHeader_T) - nRecvHeadPos,0);
					if (nRet == 0)
					{
						break;
					}
					else if (nRet < 0)
					{
						S32 errorNo = GetLastError();
						if (errorNo == EWOULDBLOCK ||
							errorNo == EINTR||
							errorNo == EAGAIN ||
							errorNo == ETIMEDOUT)
						{

							continue;
						}
						else
						{
							MODULE_ERROR("%s \n",strerror(errorNo));
							break;
						}
					}
					nRecvHeadPos += nRet;
					if (nRecvHeadPos == sizeof(ModulePacketHeader_T))
					{// 完整头
						// 处理包
						if (tHeader.uStartCode != LIBMODULE_PACKET_STARTCODE ||
							tHeader.nDataLen <= 0)
						{
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
						pRecvBuffer = (char *)Common_Malloc(tHeader.nDataLen + tHeader.uExternHeaderLen + 1,0,__FUNCTION__,__LINE__);
						if (pRecvBuffer == NULL)
						{
							closeSocket(nSocket);
							nSocket = -1;
							break;
						}
						nRecvNeedSize = tHeader.nDataLen + tHeader.uExternHeaderLen;
						nRecvPos = 0;

					}
				}

			}
		}
		else
		{
			FD_ZERO(&writeSet);
			FD_ZERO(&exceptSet);


			FD_SET(nSocket,&writeSet);
			FD_SET(nSocket,&exceptSet);

			S32 selectResult = select(nSocket + 1, NULL,&writeSet, &exceptSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误
				MODULE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

				continue;
			}
			if (selectResult == 0)
			{//超时
				break;
			}
			if (FD_ISSET(nSocket,&exceptSet))
			{
				break;
			}
			if (FD_ISSET(nSocket,&writeSet))
			{
				// send
				if (pSendBuffer == NULL)
				{

					pSendBuffer = Common_Json_Print(pInParams,&nSendDataSize);
					if (pSendBuffer == NULL)
					{
						break;
					}
					nSendDataSize++;
					nSendHeadPos = 0;
					nSendDataPos = 0;
					memset(&tHeader,0,sizeof(tHeader));
					tHeader.uStartCode = LIBMODULE_PACKET_STARTCODE;
					tHeader.nDataLen = nSendDataSize;


				}
				if (nSendHeadPos != sizeof(tHeader))
				{
					nRet = send(nSocket,((char *)&tHeader) + nSendHeadPos,sizeof(tHeader) - nSendHeadPos,MSG_NOSIGNAL);
					if (nRet < 0)
					{
						break;
					}
					nSendHeadPos += nRet;
				}
				if (nSendHeadPos == sizeof(tHeader))
				{
					nRet = send(nSocket,pSendBuffer + nSendDataPos,nSendDataSize - nSendDataPos,MSG_NOSIGNAL);
					if (nRet < 0)
					{
						break;
					}
					nSendDataPos += nRet;
					if (nSendDataPos == nSendDataSize)
					{
						bWaitResult = 1;
					}
				}

			}
		}
	}
	if (pSendBuffer)
	{
		Common_Free(pSendBuffer,__FUNCTION__,__LINE__);
		pSendBuffer = NULL;
	}
	if (pRecvBuffer)
	{
		Common_Free(pRecvBuffer,__FUNCTION__,__LINE__);
		pRecvBuffer = NULL;
	}
	if (nSocket != -1)
	{
		closeSocket(nSocket);
		nSocket = -1;
	}


	return nResult;

}

static S32 static_Module_ReportModuleList(ModuleHandle_T hModuleHandle,U32 *pRegRefreshFlag)
{
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S32 bUnix = 0,i;
	U32 dwRegRefreshFlag;
	struct _tag_TmpList
	{
		S32 bUnix;
		S8 *pszDomain;
		S32 nPort;

	} *pTmpList = NULL;
	if (pModuleMgr == NULL)
	{
		return 0;
	}
	if (!pModuleMgr->bManager)
	{
		return 0;
	}

		LibModuleRegInfo_T * p = NULL;
		cJSON_Struct *pArrayJson;
		S32 nWhich = 0;
		pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Module",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Module/Report",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Module/Report/CoreId",Common_Json_Type_Number,0,pModuleMgr->uModuleID,0);
		Common_Json_SetAttrValue(pInParam,-1,"Module/Report/CoreDomain",Common_Json_Type_String,pModuleMgr->pszCoreDomain,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Module/Report/RegRefreshFlag",Common_Json_Type_Number,NULL,pModuleMgr->dwRegRefreshFlag,0);

		pArrayJson = Common_Json_SetAttrValue(pInParam,-1,"Module/Report/Lists",Common_Json_Type_Array,0,0,0);
		Common_Lock(pModuleMgr->hRegModuleLock);
		if (pModuleMgr->nRegModuleNum > 0)
		{
			pTmpList = (struct _tag_TmpList *)Common_Malloc(sizeof(struct _tag_TmpList) * pModuleMgr->nRegModuleNum,0,__FUNCTION__,__LINE__);
			if (pTmpList == NULL)
			{
				Common_UnLock(pModuleMgr->hRegModuleLock);
				return -1;
			}
		}
	    p = pModuleMgr->pModuleRegInfoHead;

		while(p != NULL)
		{
				Common_Json_SetAttrValue(pArrayJson,nWhich,"Online",Common_Json_Type_Number,NULL,p->bOnline,0);
				Common_Json_SetAttrValue(pArrayJson,nWhich,"ModuleId",Common_Json_Type_Number,0,p->uModuleID,0);
				Common_Json_SetAttrValue(pArrayJson,nWhich,"ModuleName",Common_Json_Type_String,p->pszModuleName,0,0);
				Common_Json_SetAttrValue(pArrayJson,nWhich,"ModuleMark",Common_Json_Type_Number,NULL,p->nModuleMark,0);

				if (p->pszMac != NULL)
				{
					Common_Json_SetAttrValue(pArrayJson,nWhich,"Mac",Common_Json_Type_String,p->pszMac,0,0);
				}
				if (p->pszSerialNumber != NULL)
				{
					Common_Json_SetAttrValue(pArrayJson,nWhich,"SerialNumber",Common_Json_Type_String,p->pszSerialNumber,0,0);
				}
				if (p->pszIpv4 != NULL)
				{
					Common_Json_SetAttrValue(pArrayJson,nWhich,"RemoteDomain",Common_Json_Type_String,p->pszIpv4,0,0);
					Common_Json_SetAttrValue(pArrayJson,nWhich,"ServerPort",Common_Json_Type_Number,0,p->nPort,0);
				}

				if (p->pszDomain != NULL)
				{
					Common_Json_SetAttrValue(pArrayJson,nWhich,"LocalDomain",Common_Json_Type_String,p->pszDomain,0,0);
				}
				bUnix = 0;
				if (pModuleMgr->nUnixSvrSocket != -1)
				{
					if (p->pszSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
					{
						bUnix = 1;
					}
					else if (p->pszSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
					{
						if(0 == stricmp(p->pszSerialNumber,pModuleMgr->pszSerialNumber))
						{
							bUnix = 1;
						}
					}
				}
				pTmpList[nWhich].pszDomain = NULL;
				if (bUnix && p->pszDomain != NULL)
				{
					pTmpList[nWhich].pszDomain = Common_StrDup(p->pszDomain,__FUNCTION__,__LINE__);
				}
				else if(p->pszIpv4 != NULL)
				{
					pTmpList[nWhich].pszDomain = Common_StrDup(p->pszIpv4,__FUNCTION__,__LINE__);
					bUnix = 0;
				}
				else
				{
					bUnix = -1;
				}
				pTmpList[nWhich].bUnix = bUnix;
				pTmpList[nWhich].nPort = p->nPort;

				nWhich++;

			p = p->pNext;
		}
		dwRegRefreshFlag = pModuleMgr->dwRegRefreshFlag;
		Common_UnLock(pModuleMgr->hRegModuleLock);
		for (i = 0; i < nWhich;i++)
		{
			if (pTmpList[i].bUnix != -1)
			{
				static_Require(pTmpList[i].bUnix,pTmpList[i].pszDomain,pTmpList[i].nPort,pInParam,NULL,3000);
			}

			Common_Free(pTmpList[i].pszDomain,__FUNCTION__,__LINE__);
		}
		Common_Free(pTmpList,__FUNCTION__,__LINE__);
		if (pRegRefreshFlag)
		{
			*pRegRefreshFlag = dwRegRefreshFlag;
		}
	   Common_Json_Delete(pInParam);
	   pInParam = NULL;


	return 0;
}

static S32 static_Module_DeleteRegModule(ModuleHandle_T hModuleHandle,LibModuleRegInfo_T *pDel)
{
	if (pDel->pszModuleName != NULL)
	{
		Common_Free(pDel->pszModuleName,__FUNCTION__,__LINE__);
		pDel->pszModuleName = NULL;
	}
	if (pDel->pszDomain != NULL)
	{
		Common_Free(pDel->pszDomain,__FUNCTION__,__LINE__);
		pDel->pszDomain = NULL;
	}
	if (pDel->pszMac != NULL)
	{
		Common_Free(pDel->pszMac,__FUNCTION__,__LINE__);
		pDel->pszMac = NULL;
	}
	if (pDel->pszSerialNumber != NULL)
	{
		Common_Free(pDel->pszSerialNumber,__FUNCTION__,__LINE__);
		pDel->pszSerialNumber = NULL;
	}
	if (pDel->pszIpv4 != NULL)
	{
		Common_Free(pDel->pszIpv4,__FUNCTION__,__LINE__);
		pDel->pszIpv4 = NULL;
	}
	Common_Free(pDel,__FUNCTION__,__LINE__);
	return 0;
}

static S32 staticModule_Broadcast_Require_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S8 *pUri = NULL;
	Module_CallFunctions_Def fCallFunction = NULL;
	void *pUserData = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"/Header/Uri",NULL,&pUri,NULL,NULL);
	if (0 != Common_StrniCmp(pUri,"/Broadcast/",11))
	{
		return -1;
	}
	{
		S8 szAddUri[256];
		sprintf(szAddUri,"/%s/%s",pModuleMgr->pszModuleName,pUri+strlen("/Broadcast/"));
		Common_Json_SetAttrValue(pInParams,-1,"/Header/Uri",Common_Json_Type_String,szAddUri,0,0);
	}
	if (0 == Module_Memory_Require_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}

	pUserData = pModuleMgr->pCallUserData;
	fCallFunction = pModuleMgr->fCallFunction;
	if (fCallFunction != NULL)
	{
		fCallFunction(hModuleHandle,pInParams,NULL,pUserData);
	}

	return 0;
}
static S32 static_Module_CallFunctions_Filter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{// 返回 0表示已经处理
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pResponce = NULL;
	S32 nIntValue = 0;
	char *pStringValue = NULL,*pModuleName = NULL;
	S32 nRet = -1;

	if (pModuleMgr == NULL || pInParams == NULL)
	{
		return 0;
	}
	// MODULE_LOGD("<%s>\n",pStringValue=Common_Json_Print(pInParams,NULL));
	// Common_Free(pStringValue,__FUNCTION__,__LINE__);

	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}
	if (0 == staticModule_Broadcast_Require_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}

	if (0 == Module_Register_Heart_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}
	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}
	if (0 == Module_Register_Filter(hModuleHandle,pClientInfo,pInParams,pOutParams))
	{
		return 0;
	}


	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}
	if (0 == Module_Register_Report_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}
	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}
	if (0 == Module_UriProxy_Require_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}
	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}

	if (0 == Module_Subscribe_Require_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}
	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}

	if (0 == Module_StreamQueue_Require_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}

	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}
	if (0 == Module_Memory_Require_Filter(hModuleHandle,pInParams,pOutParams))
	{
		return 0;
	}
	if (pOutParams != NULL)
	{
		Common_Json_Delete(*pOutParams);
		*pOutParams = NULL;
	}

		pStringValue = NULL;
		Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pStringValue,NULL,NULL);
		if (pStringValue != NULL)
		{
			// 根目录处理
			if (0 == strcmp(pStringValue,"/"))
			{
				LibModuleRegInfo_T *p;
				cJSON_Struct *pArray;
				if (pModuleMgr->bManager)
				{
					S32 nWhich = 0;
					char szUri[128];
					pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pResponce,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pResponce,-1,"Header/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_SUCC,0);
					Common_Json_SetAttrValue(pResponce,-1,"Data",Common_Json_Type_Object,NULL,0,0);
					pArray = Common_Json_SetAttrValue(pResponce,-1,"Data/ResList",Common_Json_Type_Array,NULL,0,0);
					sprintf(szUri,"/%s",pModuleMgr->pszModuleName);
					Common_Json_SetAttrValue(pArray,nWhich,"Uri",Common_Json_Type_String,szUri,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"Lable",Common_Json_Type_String,pModuleMgr->pszModuleName,0,0);
					Common_Json_SetAttrValue(pArray,nWhich,"Method",Common_Json_Type_String,"Get",0,0);
					sprintf(szUri,"Access to Module[%s].",pModuleMgr->pszModuleName);
					Common_Json_SetAttrValue(pArray,nWhich,"Describe",Common_Json_Type_String,szUri,0,0);
					nWhich++;
					Common_Lock(pModuleMgr->hRegModuleLock);
					p = pModuleMgr->pModuleRegInfoHead;
					while (p != NULL)
					{
						sprintf(szUri,"/%s",p->pszModuleName);
						Common_Json_SetAttrValue(pArray,nWhich,"Uri",Common_Json_Type_String,szUri,0,0);
						Common_Json_SetAttrValue(pArray,nWhich,"Lable",Common_Json_Type_String,p->pszModuleName,0,0);
						Common_Json_SetAttrValue(pArray,nWhich,"Method",Common_Json_Type_String,"Get",0,0);
						sprintf(szUri,"Access to Module[%s].",p->pszModuleName);
						Common_Json_SetAttrValue(pArray,nWhich,"Describe",Common_Json_Type_String,szUri,0,0);
						Common_Json_SetAttrValue(pArray,nWhich,"Status",Common_Json_Type_String,p->bOnline?"Online":"Offline",0,0);
						nWhich++;
						p = p->pNext;
					}
					Common_UnLock(pModuleMgr->hRegModuleLock);
					if (pOutParams)
					{
						*pOutParams = pResponce;
						pResponce = NULL;
					}
					if (pResponce)
					{
						Common_Json_Delete(pResponce);
						pResponce = NULL;
					}
				}
				return 0;
			}

		}
		else
		{
			return 0;
		}
	return -1;
}
static S32 static_Module_CallFunctions_ReportRessInvalid(ModuleHandle_T hModuleHandle,LibModuleResourceInfo_T *pInfo)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S32 bDelete = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	if (0 == Common_StriCmp(pModuleMgr->pszModuleName,pInfo->pOwerModule))
	{

		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,pInfo->pResUri,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Delete",0,0);
	}
	else
	{
		S8 szUri[64];
		sprintf(szUri,"/%s/Notify/InvalidResource",pModuleMgr->pszModuleName);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,szUri,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Get",0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pInParam,-1,"Data/Uri",Common_Json_Type_String,pInfo->pResUri,0,0);
	}
	if (pModuleMgr->fCallFunction != NULL)
	{
		pModuleMgr->fCallFunction(hModuleHandle,pInParam,&pOutParam,pModuleMgr->pCallUserData);
	}
	if (pOutParam != NULL)
	{
		Common_Json_Delete(pOutParam);
	}
	return 0;

}

static S32 static_Module_CallFunctions_ResultFilter(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct *pOutParams)
{// 检查post资源
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char *pStringValue,*pRessUri = NULL,*pSrcValue = NULL,*pModuleName = NULL;
	S32 i,bOwer = 0;
	if (pModuleMgr == NULL || pInParams == NULL || pOutParams == NULL)
	{
		return 0;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Method",NULL,&pStringValue,NULL,NULL);
	if (pStringValue == NULL)
	{
		return 0;
	}
	if (Common_StriCmp(pStringValue,"Post"))
	{
		return 0;
	}
	pSrcValue = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pSrcValue,NULL,NULL);
	if (pSrcValue == NULL)
	{
		return 0;
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Data/Uri",NULL,&pStringValue,NULL,NULL);
	if (pStringValue == NULL)
	{
		return 0;
	}
	pRessUri = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
	if (pRessUri == NULL)
	{
		return 0;
	}
	LibModuleResourceInfo_T *pInfo = (LibModuleResourceInfo_T *)Common_Malloc(sizeof(LibModuleResourceInfo_T),0,__FUNCTION__,__LINE__);
	if (pInfo == NULL)
	{
		Common_Free(pRessUri,__FUNCTION__,__LINE__);
		pRessUri = NULL;
		return 0;
	}
	memset(pInfo,0,sizeof(LibModuleResourceInfo_T));
	pInfo->pResUri = pRessUri;
	Common_UriOneParse(pSrcValue,NULL,&pModuleName,NULL);
	pInfo->pUsedModule = pModuleName;
	Common_UriOneParse(pRessUri,NULL,&pModuleName,NULL);
	pInfo->pOwerModule = pModuleName;
	if (pInfo->pUsedModule == NULL || pInfo->pOwerModule == NULL)
	{
		Common_Free(pInfo->pUsedModule,__FUNCTION__,__LINE__);
		Common_Free(pInfo->pOwerModule,__FUNCTION__,__LINE__);
		Common_Free(pInfo->pResUri,__FUNCTION__,__LINE__);
		Common_Free(pInfo,__FUNCTION__,__LINE__);
		return 0;
	}
	if (0 == Common_StriCmp(pInfo->pOwerModule,pModuleMgr->pszModuleName))
	{
		bOwer = 1;

	}
	else if (0 == Common_StriCmp(pInfo->pUsedModule,pModuleMgr->pszModuleName))
	{
		bOwer = 0;
	}
	else
	{
		Common_Free(pInfo->pUsedModule,__FUNCTION__,__LINE__);
		Common_Free(pInfo->pOwerModule,__FUNCTION__,__LINE__);
		Common_Free(pInfo->pResUri,__FUNCTION__,__LINE__);
		Common_Free(pInfo,__FUNCTION__,__LINE__);
		return 0;
	}
	Common_Lock(pModuleMgr->hModuleRessLock);
	if (bOwer)
	{
		if (pModuleMgr->nOwnRessNum < LIBMODULE_MAX_RESOURCE_NUM)
		{

			for (i = 0; i < LIBMODULE_MAX_RESOURCE_NUM;i++)
			{
				if(pModuleMgr->pOwnResourceList[i] == NULL)
				{
					pModuleMgr->pOwnResourceList[i] = pInfo;
					pInfo->pNext = pModuleMgr->pOwnRessHead;
					if (pModuleMgr->pOwnRessHead != NULL)
					{
						pModuleMgr->pOwnRessHead->pPrev = pInfo;
						pModuleMgr->pOwnRessHead = pInfo;
					}
					pModuleMgr->nOwnRessNum++;
					pInfo = NULL;
					break;
				}
			}
		}
	}
	else
	{
		if (pModuleMgr->nUsedRessNum < LIBMODULE_MAX_RESOURCE_NUM)
		{

			for (i = 0; i < LIBMODULE_MAX_RESOURCE_NUM;i++)
			{
				if(pModuleMgr->pUsedResourceList[i] == NULL)
				{
					pModuleMgr->pUsedResourceList[i] = pInfo;
					pInfo->pNext = pModuleMgr->pUsedRessHead;
					if (pModuleMgr->pUsedRessHead != NULL)
					{
						pModuleMgr->pUsedRessHead->pPrev = pInfo;
						pModuleMgr->pUsedRessHead = pInfo;
					}
					pModuleMgr->nUsedRessNum++;
					pInfo = NULL;
					break;
				}
			}
		}
	}

	Common_UnLock(pModuleMgr->hModuleRessLock);
	if (pInfo != NULL)
	{
		Common_Free(pInfo->pUsedModule,__FUNCTION__,__LINE__);
		Common_Free(pInfo->pOwerModule,__FUNCTION__,__LINE__);
		Common_Free(pInfo->pResUri,__FUNCTION__,__LINE__);
		Common_Free(pInfo,__FUNCTION__,__LINE__);
	}

	return 0;
}
static S32 static_Module_CallFunctions_AfterFilter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{// 返回 0表示已经处理
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char *pStringValue;
	if (pModuleMgr == NULL || pInParams == NULL)
	{
		return 0;
	}
   // 可以处理自动转发
	if (!pModuleMgr->bManager)
	{
		return -1;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		char *pModuleDes = NULL;
		Common_UriOneParse(pStringValue,NULL,&pModuleDes,NULL);
		if (pModuleDes != NULL)
		{
			LibModuleRegInfo_T *pNode;
			pNode = pModuleMgr->pModuleRegInfoHead;
			MODULE_LOGD("Here\n");
			while(pNode != NULL)
			{
				MODULE_LOGD("[%s]->[%s]\n",pModuleDes,pNode->pszModuleName);
				if (0 == stricmp(pModuleDes,pNode->pszModuleName))
				{
					Common_Free(pModuleDes,__FUNCTION__,__LINE__);
					pModuleDes = NULL;
					return Module_CallFunctions(hModuleHandle,pInParams,pOutParams,60000);
					break;
				}
				pNode = pNode->pNext;
			}
			if (pNode == NULL)
			{
				cJSON_Struct *pResponce;
				// 未找到
				MODULE_LOGD("Here\n");
				pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pResponce,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pResponce,-1,"Header/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_NOTFOUND,0);
				if (pOutParams)
				{
					*pOutParams = pResponce;
					pResponce = NULL;
				}
				if (pResponce)
				{
					Common_Json_Delete(pResponce);
					pResponce = NULL;
				}
			}

			Common_Free(pModuleDes,__FUNCTION__,__LINE__);
			pModuleDes = NULL;
			return 0;
		}
	}

	return -1;
}

static S32 static_Http_Client(LibModuleClientInfo_T *pClientInfo)
{
	S32 nRet = 0;
	int nCrlf = 0,nNextPos = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	if (!pClientInfo->bHttp)
	{
		if (pClientInfo->nSocket != -1)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
		}
		pClientInfo->bNeedDelete = 1;
		return -1;
	}
	if (0 != Common_Http_Recv(pClientInfo->nSocket,&pClientInfo->tHttpContext))
	{
		if (pClientInfo->nSocket != -1)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
		}
		pClientInfo->bNeedDelete = 1;
		return -1;
	}
	if (pClientInfo->tHttpContext.nHttpStep != 3)
	{// 未完成,继续 读数据
		return 0;
	}
	// 处理命令
	if(0 != Common_Http_Http2Json(&pClientInfo->tHttpContext,&pInParam))
	{
		pClientInfo->bNeedDelete = 1;
		Common_Json_Delete(pInParam);
		pInParam = NULL;
		return -1;
	}
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)pClientInfo->pModuleMgr;
	if (pModuleMgr != NULL)
	{
		char *pSendStr = NULL;
		S32 nStrlen = 0,nSendLen = 0;
		S32 bDo = 0;
		S8 *pIPString = NULL;
		cJSON_Struct *pClientJson = NULL;
		if (!pClientInfo->bUnixSocket)
		{
			Common_GetRemoteIP(pClientInfo->nSocket,&pIPString,NULL,NULL);
		}
		if (pIPString != NULL)
		{
			pClientJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			if (pClientJson != NULL)
			{
				Common_Json_SetAttrValue(pClientJson,-1,"RemoteDomain",Common_Json_Type_String,pIPString,0,0);
			}
			Common_Free(pIPString,__FUNCTION__,__LINE__);
		}

		extern_Module_CallFunctions_MsgRouter(pModuleMgr->hModuleHandle,pClientJson,pInParam,&pOutParam);


		Common_Json_Delete(pClientJson);
		pClientJson = NULL;


	}

	// 应答
	if (pOutParam != NULL)
	{
		CommonHttpContext_T *pSendHttpContext = (CommonHttpContext_T *)Common_Malloc(sizeof(CommonHttpContext_T),0,__FUNCTION__,__LINE__);
		if (pSendHttpContext != NULL)
		{
			Common_Http_Init(pSendHttpContext);
			if(0 == Common_Http_Json2Http(pOutParam,pSendHttpContext))
			{

				do
				{
					nRet = Common_Http_Send(pClientInfo->nSocket,pSendHttpContext);
					if (nRet != 0)
					{
						pClientInfo->bNeedDelete = 1;
						break;
					}
					if (pSendHttpContext->nHttpStep == 3)
					{
						break;
					}
				} while (1);
				if (!pSendHttpContext->bKeepAlive)
				{
					pClientInfo->bNeedDelete = 1;
				}

			}
			else
			{
				pClientInfo->bNeedDelete = 1;
			}
			Common_Http_Free(pSendHttpContext);
			Common_Free(pSendHttpContext,__FUNCTION__,__LINE__);
			pSendHttpContext = NULL;
		}

	}

	// 关闭
	if (!pClientInfo->tHttpContext.bKeepAlive)
	{
		pClientInfo->bNeedDelete = 1;
	}
	Common_Json_Delete(pInParam);
	pInParam = NULL;
	Common_Json_Delete(pOutParam);
	pOutParam = NULL;



	return 0;
}

S32 extern_Module_CallFunctions_MsgRouter(ModuleHandle_T hModuleHandle,cJSON_Struct *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	cJSON_Struct *pRecvJson = pInParams,*pResultJson=NULL;
	S32 bDo = 0;
	if(0 == static_Module_CallFunctions_Filter(pModuleMgr,pClientInfo,pRecvJson,&pResultJson,pModuleMgr->pCallUserData))
	{
		bDo = 1;
	}
	else
	{
		if (pResultJson != NULL)
		{
			Common_Json_Delete(pResultJson);
			pResultJson = NULL;
		}
		if (pModuleMgr->fCallFunction != NULL && 0 == pModuleMgr->fCallFunction(pModuleMgr,pRecvJson,&pResultJson,pModuleMgr->pCallUserData))
		{
			bDo = 1;

		}
		else
		{
			if (pResultJson != NULL)
			{
				Common_Json_Delete(pResultJson);
				pResultJson = NULL;
			}
			if(0 == static_Module_CallFunctions_AfterFilter(pModuleMgr,pClientInfo,pRecvJson,&pResultJson,pModuleMgr->pCallUserData))
			{
				bDo = 1;
			}
			else
			{
				if (pResultJson != NULL)
				{
					Common_Json_Delete(pResultJson);
					pResultJson = NULL;
				}

				pResultJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pResultJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
				Common_Json_SetAttrValue(pResultJson,-1,"Header/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_UNKNOW,0);

			}
		}
	}
	if (pOutParams != NULL)
	{
		*pOutParams = pResultJson;
	}
	else if (pResultJson != NULL)
	{
		Common_Json_Delete(pResultJson);
		pResultJson = NULL;
	}
	return bDo?0:-1;
}

static S32 static_Module_Thread_Client(Common_Thread_T hThreadHandle,void *pUserData)
{
	LibModuleClientInfo_T *pClientInfo = (LibModuleClientInfo_T *)pUserData;
	struct timeval tv_timeToDelay;
	fd_set tReadSet,tExceptionSet;
	S32 nMaxNumSocket = 0;
	if (pClientInfo == NULL)
	{
		return -1;
	}
	while (!pClientInfo->bClientThreadExit)
	{
		if (pClientInfo->bNeedDelete)
		{
			if (pClientInfo->nSocket != -1)
			{
				closeSocket(pClientInfo->nSocket);
				pClientInfo->nSocket = -1;
			}
			break;
		}
		// add sockets
		FD_ZERO(&tReadSet);
		FD_ZERO(&tExceptionSet);
		if (pClientInfo->nSocket != -1)
		{
			FD_SET(pClientInfo->nSocket,&tReadSet);

			FD_SET(pClientInfo->nSocket,&tExceptionSet);
			if (pClientInfo->nSocket + 1 > nMaxNumSocket)
			{
				nMaxNumSocket = pClientInfo->nSocket + 1;
			}
		}

		tv_timeToDelay.tv_sec = 0;
		tv_timeToDelay.tv_usec = 10000;

		S32 selectResult = select(nMaxNumSocket, &tReadSet,NULL, &tExceptionSet, &tv_timeToDelay);
		if (selectResult < 0)
		{//错误
			MODULE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

			continue;
		}
		if (selectResult == 0)
		{//超时
			continue;
		}
		if (FD_ISSET(pClientInfo->nSocket,&tReadSet))
		{
			S32 nRet;
			if (pClientInfo->bHttp)
			{
				static_Http_Client(pClientInfo);
				continue;
			}
			if (pClientInfo->pRecvBuffer != NULL)
			{
				nRet = recv(pClientInfo->nSocket,pClientInfo->pRecvBuffer + pClientInfo->nRecvLen,pClientInfo->nNeedLen - pClientInfo->nRecvLen,0);
				if (nRet == 0)
				{
					closeSocket(pClientInfo->nSocket);
					pClientInfo->nSocket = -1;
					pClientInfo->bNeedDelete = 1;

					return -1;
				}
				else if (nRet < 0)
				{
					S32 errorNo = GetLastError();
					if (errorNo == EWOULDBLOCK ||
						errorNo == EINTR||
						errorNo == EAGAIN ||
						errorNo == ETIMEDOUT)
					{

						continue;
					}
					else
					{
						MODULE_ERROR("%s \n",strerror(errorNo));
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						return -1;
					}
				}
				pClientInfo->nRecvLen += nRet;
				if (pClientInfo->nRecvLen == pClientInfo->nNeedLen)
				{// 完整包
					cJSON_Struct *pRecvJson,*pResultJson=NULL;
					// 处理包
					pRecvJson = Common_Json_Parse(pClientInfo->pRecvBuffer + pClientInfo->tHeader.uExternHeaderLen,NULL,NULL);
					if (pRecvJson != NULL)
					{
						LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)pClientInfo->pModuleMgr;
						if (pModuleMgr != NULL)
						{
							char *pSendStr = NULL;
							cJSON_Struct *pClientJson = NULL;
							S32 nStrlen = 0,nSendLen = 0;
							S32 bDo = 0;
							S8 *pIPString = NULL;
							// 获取当前IP
							if (!pClientInfo->bUnixSocket)
							{
								Common_GetRemoteIP(pClientInfo->nSocket,&pIPString,NULL,NULL);
							}
							if (pIPString != NULL)
							{
								pClientJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
								if (pClientJson != NULL)
								{
									Common_Json_SetAttrValue(pClientJson,-1,"RemoteDomain",Common_Json_Type_String,pIPString,0,0);
								}
								Common_Free(pIPString,__FUNCTION__,__LINE__);
							}

						//
							extern_Module_CallFunctions_MsgRouter(pModuleMgr->hModuleHandle,pClientJson,pRecvJson,&pResultJson);
							if (pResultJson != NULL)
							{
								static_Module_CallResponce(pModuleMgr,pClientInfo,pRecvJson,pResultJson);
							}
							Common_Json_Delete(pClientJson);
						}
						Common_Json_Delete(pResultJson);
						Common_Json_Delete(pRecvJson);
						pRecvJson = NULL;
						if (pClientInfo->nSocket != -1)
						{
							closeSocket(pClientInfo->nSocket);
							pClientInfo->nSocket = -1;
						}

						pClientInfo->bNeedDelete = 1;
						return 0;
					}
					else
					{
						//
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						return -1;
					}
					// 处理完后，清除缓冲
					Common_Free(pClientInfo->pRecvBuffer,__FUNCTION__,__LINE__);
					pClientInfo->pRecvBuffer = NULL;
					pClientInfo->nNeedLen = 0;
					pClientInfo->nRecvLen = 0;

				}
			}
			else
			{
				// 检测是不是HTTP
				if (!pClientInfo->bHeaderChecked)
				{
					U32 uStartCode = 0;
					nRet = recv(pClientInfo->nSocket,(char *)&uStartCode,4,MSG_PEEK);
					if (nRet == 0)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
					else if (nRet < 0)
					{
						S32 errorNo = GetLastError();
						if (errorNo == EWOULDBLOCK ||
							errorNo == EINTR||
							errorNo == EAGAIN ||
							errorNo == ETIMEDOUT)
						{

							continue;
						}
						else
						{
							closeSocket(pClientInfo->nSocket);
							pClientInfo->nSocket = -1;
							pClientInfo->bNeedDelete = 1;
							break;
						}
					}
					if (nRet != 4)
					{
						continue;
					}
					if (uStartCode != LIBMODULE_PACKET_STARTCODE)
					{
						// 可能是http
						pClientInfo->bHttp = 1;
						Common_Http_Init(&pClientInfo->tHttpContext);

					}
					pClientInfo->bHeaderChecked = 1;

				}
				if (pClientInfo->bHttp)
				{
					static_Http_Client(pClientInfo);
					continue;
				}
				// 私有方式
				nRet = recv(pClientInfo->nSocket,((char *)&pClientInfo->tHeader) + pClientInfo->nHeaderLen,sizeof(ModulePacketHeader_T) - pClientInfo->nHeaderLen,0);
				if (nRet == 0)
				{
					closeSocket(pClientInfo->nSocket);
					pClientInfo->nSocket = -1;
					pClientInfo->bNeedDelete = 1;
					return -1;
				}
				else if (nRet < 0)
				{
					S32 errorNo = GetLastError();
					if (errorNo == EWOULDBLOCK ||
						errorNo == EINTR||
						errorNo == EAGAIN ||
						errorNo == ETIMEDOUT)
					{

						continue;
					}
					else
					{
						MODULE_ERROR("%s \n",strerror(errorNo));
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						return -1;
					}
				}
				pClientInfo->nHeaderLen += nRet;
				if (pClientInfo->nHeaderLen == sizeof(ModulePacketHeader_T))
				{// 完整头
					// 处理包
					if (pClientInfo->tHeader.uStartCode != LIBMODULE_PACKET_STARTCODE ||
						pClientInfo->tHeader.nDataLen <= 0)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						return -1;
					}
					pClientInfo->pRecvBuffer = (char *)Common_Malloc(pClientInfo->tHeader.nDataLen + pClientInfo->tHeader.uExternHeaderLen + 1,0,__FUNCTION__,__LINE__);
					if (pClientInfo->pRecvBuffer == NULL)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						return -1;
					}
					pClientInfo->nNeedLen = pClientInfo->tHeader.nDataLen + pClientInfo->tHeader.uExternHeaderLen;
					pClientInfo->nRecvLen = 0;
					// 处理完后，清除缓冲
					//memset(&pClientInfo->tHeader,0,sizeof(ModulePacketHeader_T));
					pClientInfo->nHeaderLen = 0;

				}
			}

		}
		if (FD_ISSET(pClientInfo->nSocket,&tExceptionSet))
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;
			return -1;
		}

	}
	return 0;
}

static S32 static_Module_CallResponce(ModuleHandle_T hModuleHandle,LibModuleClientInfo_T *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct *pOutParams)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S8 *pSendStr;
	S32 nStrlen = 0,nSendLen = 0;
	struct timeval tv_timeToDelay;
	fd_set tReadSet,tWriteSet,tExceptionSet;
	S32 nMaxNumSocket = 0;
	if (pOutParams != NULL)
	{
		pSendStr = Common_Json_Print(pOutParams,&nStrlen);
		if (pSendStr == NULL)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;
			return -1;

		}
		MODULE_LOGD("Responce:<%s>\n",pSendStr);

	}
	else
	{
		// 无结果 的自动处理
		pSendStr = (char *)Common_Malloc(512,0,__FUNCTION__,__LINE__);
		if (pSendStr)
		{
			nStrlen = sprintf(pSendStr,"{\"Header\":{\"Code\":0}}");
		}
	}
	if (pSendStr == NULL)
	{
		pClientInfo->bNeedDelete = 1;
		return -1;
	}
	if (pClientInfo->nSocket == -1 || pClientInfo->bNeedDelete)
	{
		Common_Free(pSendStr,__FUNCTION__,__LINE__);
		return -1;
	}

		pClientInfo->tHeader.nDataLen = nStrlen + 1;
		pClientInfo->tHeader.uExternHeaderLen = 0;
		nSendLen = send(pClientInfo->nSocket,(char *)&pClientInfo->tHeader,sizeof(ModulePacketHeader_T),MSG_NOSIGNAL);
		if (nSendLen < 0)
		{
			closeSocket(pClientInfo->nSocket);
			pClientInfo->nSocket = -1;
			pClientInfo->bNeedDelete = 1;

		}
		else if (nSendLen != sizeof(ModulePacketHeader_T))
		{
			// 需要放缓冲，稍后再发
			pClientInfo->nHeaderLen = nSendLen;
			pClientInfo->pSendBuffer = pSendStr;
			pClientInfo->nNeedSendLen = nStrlen + 1;
			pClientInfo->nWriteFlag = 1;
			pSendStr = NULL;
		}
		else
		{
			pClientInfo->nHeaderLen = nSendLen;
			nSendLen = send(pClientInfo->nSocket,pSendStr,nStrlen + 1,MSG_NOSIGNAL);
			if (nSendLen < 0)
			{
				closeSocket(pClientInfo->nSocket);
				pClientInfo->nSocket = -1;
				pClientInfo->bNeedDelete = 1;
			}
			else if (nSendLen != nStrlen + 1)
			{
				pClientInfo->pSendBuffer = pSendStr;
				pClientInfo->nNeedSendLen = nStrlen + 1;
				pClientInfo->nSendLen = nSendLen;
				pClientInfo->nWriteFlag = 1;
				pSendStr = NULL;
			}
		}
		if (pSendStr != NULL)
		{
			Common_Free(pSendStr,__FUNCTION__,__LINE__);
			// 成功
			//static_Module_CallFunctions_ResultFilter(hModuleHandle,pInParams,pOutParams);
			return 0;

		}
		if (pClientInfo->nSocket == -1)
		{
			return -1;
		}
		while(!pClientInfo->bClientThreadExit)
		{
			FD_ZERO(&tReadSet);
			FD_ZERO(&tWriteSet);
			FD_ZERO(&tExceptionSet);
			if (pClientInfo->bNeedDelete)
			{
				break;
			}


			FD_SET(pClientInfo->nSocket,&tWriteSet);


			FD_SET(pClientInfo->nSocket,&tExceptionSet);
			if (pClientInfo->nSocket + 1 > nMaxNumSocket)
			{
				nMaxNumSocket = pClientInfo->nSocket + 1;
			}

			tv_timeToDelay.tv_sec = 0;
			tv_timeToDelay.tv_usec = 100000;

			S32 selectResult = select(nMaxNumSocket, NULL,&tWriteSet, &tExceptionSet, &tv_timeToDelay);
			if (selectResult < 0)
			{//错误
				LOGE("select   err[%d,%s]\n",GetLastError(),strerror(GetLastError()));
				pClientInfo->bNeedDelete = 1;
				break;;
			}
			if (selectResult == 0)
			{//超时
				continue;
			}
			if (FD_ISSET(pClientInfo->nSocket,&tExceptionSet))
			{
				closeSocket(pClientInfo->nSocket);
				pClientInfo->nSocket = -1;
				pClientInfo->bNeedDelete = 1;
				break;
			}
			else if (FD_ISSET(pClientInfo->nSocket,&tWriteSet))
			{
				S32 nSendLen;
			 //
				if (pClientInfo->nHeaderLen != sizeof(ModulePacketHeader_T))
				{
					nSendLen = send(pClientInfo->nSocket,((char *)&pClientInfo->tHeader) + pClientInfo->nHeaderLen,sizeof(ModulePacketHeader_T)-pClientInfo->nHeaderLen,MSG_NOSIGNAL);
					if (nSendLen < 0)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;

					}
					else if (nSendLen != sizeof(ModulePacketHeader_T))
					{
						// 需要放缓冲，稍后再发
						pClientInfo->nHeaderLen += nSendLen;
					}

				}
				else if (pClientInfo->pSendBuffer != NULL)
				{
					nSendLen = send(pClientInfo->nSocket,pClientInfo->pSendBuffer + pClientInfo->nSendLen,pClientInfo->nNeedSendLen - pClientInfo->nSendLen,MSG_NOSIGNAL);
					if (nSendLen < 0)
					{
						closeSocket(pClientInfo->nSocket);
						pClientInfo->nSocket = -1;
						pClientInfo->bNeedDelete = 1;
						break;
					}
					else
					{
						pClientInfo->nSendLen += nSendLen;
						if (pClientInfo->nSendLen == pClientInfo->nNeedSendLen)
						{
							Common_Free(pClientInfo->pSendBuffer,__FUNCTION__,__LINE__);
							pClientInfo->pSendBuffer = NULL;
							pClientInfo->nSendLen = 0;
							pClientInfo->nNeedSendLen = 0;
							pClientInfo->nHeaderLen = 0;
							pClientInfo->nWriteFlag = 0;
							pClientInfo->bNeedDelete = 1;
							//static_Module_CallFunctions_ResultFilter(hModuleHandle,pInParams,pOutParams);
							return 0;
						}
					}
				}

			}
		};
		return -1;


}


static S32 static_HandleListenInComming(LibModuleInfo_T *pModuleMgr,S32 nListenSocket,S32 bUseUnixLocal)
{
	S32 nRet;
	S32 nIdx;
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	S32 clientSocket = accept(nListenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientSocket >= 0)
	{
#if defined(__WIN32__) || defined(_WIN32)
		unsigned long arg = 1;
		nRet =  ioctlsocket(clientSocket, FIONBIO, &arg);
		if (nRet)
		{
			closeSocket(clientSocket);
			return -1;
		}

#else
		S32 curFlags = fcntl(clientSocket, F_GETFL, 0);
		nRet =  fcntl(clientSocket, F_SETFL, curFlags|O_NONBLOCK);
		if (nRet < 0 )
		{
			MODULE_LOGD("Here\n");
			closeSocket(clientSocket);
			return -1;
		}

#endif
		for (nIdx = 0; nIdx < LIBMODULE_MAX_CLIENT_NUM; nIdx++)
		{
			if(pModuleMgr->pClientModuleInfo[nIdx] == NULL)
			{
				LibModuleClientInfo_T *pClientModuleInfo = NULL;
				pClientModuleInfo = (LibModuleClientInfo_T *)Common_Malloc(sizeof(LibModuleClientInfo_T),0,__FUNCTION__,__LINE__) ;
				if (pClientModuleInfo == NULL)
				{
					closeSocket(clientSocket);
					return -1;
				}
				memset(pClientModuleInfo,0,sizeof(LibModuleClientInfo_T));
				pModuleMgr->uStaticCount++;
				pClientModuleInfo->uModuleID = ((nIdx + 1) << 16) |(pModuleMgr->uStaticCount & 0xFFFF);
				pClientModuleInfo->nSocket = clientSocket;
				pClientModuleInfo->pModuleMgr = pModuleMgr;
				pClientModuleInfo->bUnixSocket = bUseUnixLocal;
				MODULE_LOGD("Here\n");
				{

					S8 szTmp[128];
					sprintf(szTmp,"Module_ClientThread_%x_%d",pClientModuleInfo->uModuleID,clientSocket);
					if(Common_Thread_Create(&pClientModuleInfo->hClientThread,szTmp,0,0,static_Module_Thread_Client,pClientModuleInfo))
					{
						closeSocket(clientSocket);
						Common_Free(pClientModuleInfo,__FUNCTION__,__LINE__);
						MODULE_LOGD("Here\n");
						return -1;
					}
				}
				pClientModuleInfo->pNext = pModuleMgr->pClientInfoHead;
				if (pModuleMgr->pClientInfoHead != NULL)
				{
					pModuleMgr->pClientInfoHead->pPrev = pClientModuleInfo;
				}
				pModuleMgr->pClientInfoHead = pClientModuleInfo;
				pModuleMgr->nClientModuleNum++;
				break;
			}
		}
		// 创建线程
	}
	return 0;
}
static S32 static_Module_Thread_Listen(Common_Thread_T hThreadHandle,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)pUserData;
	struct timeval tv_timeToDelay;
	fd_set tReadSet,tExceptionSet;
	S32 nMaxNumSocket = 0;
	LibModuleClientInfo_T *pClientInfo;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	while (!pModuleMgr->bListenThreadExit)
	{
		// add sockets
		FD_ZERO(&tReadSet);
		FD_ZERO(&tExceptionSet);
		if (pModuleMgr->nServerSocket != -1)
		{
			FD_SET(pModuleMgr->nServerSocket,&tReadSet);
			FD_SET(pModuleMgr->nServerSocket,&tExceptionSet);
			if (pModuleMgr->nServerSocket > nMaxNumSocket)
			{
				nMaxNumSocket = pModuleMgr->nServerSocket;
			}
		}
		if (pModuleMgr->nUnixSvrSocket != -1)
		{
			FD_SET(pModuleMgr->nUnixSvrSocket,&tReadSet);
			FD_SET(pModuleMgr->nUnixSvrSocket,&tExceptionSet);
			if (pModuleMgr->nUnixSvrSocket > nMaxNumSocket)
			{
				nMaxNumSocket = pModuleMgr->nUnixSvrSocket;
			}
		}
		// 检查是否需要删除客户端
		pClientInfo = pModuleMgr->pClientInfoHead;
		while (pClientInfo != NULL)
		{
			if (pClientInfo->bNeedDelete || pClientInfo->nSocket == -1)
			{
				LibModuleClientInfo_T *pClientInfo_del;
				S32 nIdx;

				//
				pClientInfo->bClientThreadExit = 1;
				if (pClientInfo->pPrev == NULL)
				{
					pModuleMgr->pClientInfoHead = pClientInfo->pNext;
					if (pModuleMgr->pClientInfoHead != NULL)
					{
						pModuleMgr->pClientInfoHead->pPrev = NULL;
					}

				}
				else
				{
					pClientInfo->pPrev->pNext = pClientInfo->pNext;
					if (pClientInfo->pNext != NULL)
					{
						pClientInfo->pNext->pPrev = pClientInfo->pPrev;
					}
				}
				pClientInfo_del = pClientInfo;

				pClientInfo = pClientInfo->pNext;

				nIdx = (pClientInfo_del->uModuleID >> 16) -1;
				pModuleMgr->pClientModuleInfo[nIdx] = NULL;
				if (pClientInfo_del->hClientThread)
				{
					Common_Thread_Destroy(&pClientInfo_del->hClientThread);
				}
				if (pClientInfo_del->pRecvBuffer != NULL)
				{
					Common_Free(pClientInfo_del->pRecvBuffer,__FUNCTION__,__LINE__);
					pClientInfo_del->pRecvBuffer = NULL;
				}
				if (pClientInfo_del->pSendBuffer != NULL)
				{
					Common_Free(pClientInfo_del->pSendBuffer,__FUNCTION__,__LINE__);
					pClientInfo_del->pSendBuffer = NULL;
				}
				if (pClientInfo_del->pszModuleName)
				{
					Common_Free(pClientInfo_del->pszModuleName,__FUNCTION__,__LINE__);
					pClientInfo_del->pszModuleName = NULL;
				}
				if (pClientInfo_del->nSocket != -1)
				{
					closeSocket(pClientInfo_del->nSocket);
					pClientInfo_del->nSocket = -1;
				}
				Common_Http_Free(&pClientInfo_del->tHttpContext);
				Common_Free(pClientInfo_del,__FUNCTION__,__LINE__);

				continue;

			}
			pClientInfo = pClientInfo->pNext;
		}
		tv_timeToDelay.tv_sec = 1;
		tv_timeToDelay.tv_usec = 0;

		S32 selectResult = select(nMaxNumSocket + 1, &tReadSet, NULL, &tExceptionSet, &tv_timeToDelay);
		if (selectResult < 0)
		{//错误
			MODULE_ERROR("[%s.%d]select   err[%d,%s]\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));

			continue;
		}
		if (selectResult == 0)
		{//超时
			continue;
		}
		if (pModuleMgr->nServerSocket != -1)
		{
			if (FD_ISSET(pModuleMgr->nServerSocket,&tReadSet))
			{
				MODULE_LOGD("Here\n");
				static_HandleListenInComming(pModuleMgr,pModuleMgr->nServerSocket,0);
			}

			if (FD_ISSET(pModuleMgr->nServerSocket,&tExceptionSet))
			{
			}
		}

		if (pModuleMgr->nUnixSvrSocket != -1)
		{
			if (FD_ISSET(pModuleMgr->nUnixSvrSocket,&tReadSet))
			{
				MODULE_LOGD("Here\n");
				static_HandleListenInComming(pModuleMgr,pModuleMgr->nUnixSvrSocket,1);
			}

			if (FD_ISSET(pModuleMgr->nUnixSvrSocket,&tExceptionSet))
			{
			}
		}

	}
	return 0;
}

static S32 static_Module_Register(LibModuleInfo_T *pModuleMgr)
{
	S32 nCode = -1;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	if (pModuleMgr->bRegister)
	{
		return 0;
	}
	MODULE_LOGD("start register\n");
	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Register",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Register/SystemName",Common_Json_Type_String,pModuleMgr->pszSystemName,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Register/ModuleName",Common_Json_Type_String,pModuleMgr->pszModuleName,0,0);
	if (pModuleMgr->nUnixSvrSocket != -1)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Module/Register/UnixSocketValid",Common_Json_Type_Number,NULL,1,0);
		Common_Json_SetAttrValue(pInParam,-1,"Module/Register/LocalDomain",Common_Json_Type_String,pModuleMgr->pszLocalDomain,0,0);
	}
	if (pModuleMgr->nServerSocket != -1)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Module/Register/ServerPort",Common_Json_Type_Number,0,pModuleMgr->nServerPort,0);
	}
	if (pModuleMgr->pszMac != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Module/Register/Mac",Common_Json_Type_String,pModuleMgr->pszMac,0,0);
	}
	if (pModuleMgr->pszSerialNumber != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"Module/Register/SerialNumber",Common_Json_Type_String,pModuleMgr->pszSerialNumber,0,0);
	}
	Common_Json_SetAttrValue(pInParam,-1,"Module/Register/ModuleMark",Common_Json_Type_Number,0,pModuleMgr->nModuleMark,0);
	if(0 == staticModule_CallCoreFunctions(pModuleMgr,pInParam,&pOutParam,3000))
	{
		Common_Json_GetAttrValue(pOutParam,-1,"/Module/Register/Code",NULL,NULL,&nCode,NULL);
		if (nCode == 0)
		{
			char *pStringValue = NULL;
			S32 nModuleId = 0;
			Common_Json_GetAttrValue(pOutParam,-1,"/Module/Register/ModuleId",NULL,NULL,&nModuleId,NULL);
			pModuleMgr->uModuleID = (U32)nModuleId;
			Common_Json_GetAttrValue(pOutParam,-1,"/Module/Register/CoreId",NULL,NULL,&nModuleId,NULL);
			pModuleMgr->uCoreID = (U32)nModuleId;
			pModuleMgr->bRegister = 1;
			MODULE_LOGD("register succ\n");
			pStringValue = NULL;
			Common_Json_GetAttrValue(pOutParam,-1,"/Module/Register/CoreDomain",NULL,&pStringValue,&nCode,NULL);
			if (pStringValue)
			{
				if(pModuleMgr->pszCoreDomain != NULL)
				{
					if(0 != Common_StrCmp(pModuleMgr->pszCoreDomain, pStringValue))
					{
						Common_Free(pModuleMgr->pszCoreDomain,__FUNCTION__,__LINE__);
						pModuleMgr->pszCoreDomain = NULL;
					}

				}
				if(pModuleMgr->pszCoreDomain == NULL)
				{
					pModuleMgr->pszCoreDomain = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
			}
			pStringValue = NULL;
			Common_Json_GetAttrValue(pOutParam,-1,"/Module/Register/CoreMac",NULL,&pStringValue,&nCode,NULL);
			if (pStringValue)
			{
				if(pModuleMgr->pszCoreMac != NULL)
				{
					if(0 != Common_StrCmp(pModuleMgr->pszCoreMac, pStringValue))
					{
						Common_Free(pModuleMgr->pszCoreMac,__FUNCTION__,__LINE__);
						pModuleMgr->pszCoreMac = NULL;
					}

				}
				if(pModuleMgr->pszCoreMac == NULL)
				{
					pModuleMgr->pszCoreMac = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
			}
			pStringValue = NULL;
			Common_Json_GetAttrValue(pOutParam,-1,"/Module/Register/CoreSerialNumber",NULL,&pStringValue,&nCode,NULL);
			if (pStringValue)
			{
				if(pModuleMgr->pszCoreSerialNumber != NULL)
				{
					if(0 != Common_StrCmp(pModuleMgr->pszCoreSerialNumber, pStringValue))
					{
						Common_Free(pModuleMgr->pszCoreSerialNumber,__FUNCTION__,__LINE__);
						pModuleMgr->pszCoreSerialNumber = NULL;
					}

				}
				if(pModuleMgr->pszCoreSerialNumber == NULL)
				{
					pModuleMgr->pszCoreSerialNumber = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
				}
			}


		}

	}
	if (pOutParam != NULL)
	{
		char *pstr = Common_Json_Print(pOutParam,NULL);
		MODULE_LOGD(pstr);
		Common_Free(pstr,__FUNCTION__,__LINE__);
		Common_Json_Delete(pOutParam);
		pOutParam = NULL;
	}
	Common_Json_Delete(pInParam);

	return nCode;
}
static S32 static_Module_Heart(LibModuleInfo_T *pModuleMgr)
{
	S32 nSec = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
	if (!pModuleMgr->bRegister)
	{
		return 0;
	}
	Common_GetSystemCount(&nSec,NULL);
	if (pModuleMgr->nLastHeartTime + LIBMODULE_HEART_INV > nSec)
	{
		return 0;
	}
	pModuleMgr->nLastHeartTime = nSec;
	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Heart",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Heart/SystemName",Common_Json_Type_String,pModuleMgr->pszSystemName,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Heart/ModuleName",Common_Json_Type_String,pModuleMgr->pszModuleName,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Heart/ModuleId",Common_Json_Type_Number,0,pModuleMgr->uModuleID,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Heart/RegRefreshFlag",Common_Json_Type_Number,0,pModuleMgr->dwRegRefreshFlag,0);
	Common_Json_SetAttrValue(pInParam,-1,"Module/Heart/ModlueMark",Common_Json_Type_Number,0,pModuleMgr->nModuleMark,0);

	if(0 == staticModule_CallCoreFunctions(pModuleMgr,pInParam,&pOutParam,3000))
	{
		S32 nCode = -1;
		Common_Json_GetAttrValue(pOutParam,-1,"Module/Heart/Code",NULL,NULL,&nCode,NULL);
		if (nCode != 0)
		{
			pModuleMgr->nHeartFailCnt++;
		}
	}
	else
	{
		pModuleMgr->nHeartFailCnt++;

	}
	if (pModuleMgr->nHeartFailCnt >= 3)
	{
		pModuleMgr->bRegister = 0;
		pModuleMgr->nHeartFailCnt = 0;
	}
	if (pOutParam != NULL)
	{
#if 0
		char *pstr = Common_Json_Print(pOutParam,NULL);
		MODULE_LOGD(pstr);
		Common_Free(pstr,__FUNCTION__,__LINE__);
#endif
		Common_Json_Delete(pOutParam);
		pOutParam = NULL;
	}
	Common_Json_Delete(pInParam);
	return 0;
}
extern S32 Module_StreamQueue_Test(S8 *szName);
extern void StreamQueue_CheckValid(LibModuleInfo_T *pModuleMgr);
static S32 static_Module_Thread_Heart(Common_Thread_T hThreadHandle,void *pUserData)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)pUserData;
	U32 dwRegRefreshFlag = 0,dwProxyFlag = 0;
	S32 dwTestCnt = 0,dwTestQueue = 0;
	if (pModuleMgr == NULL)
	{
		return -1;
	}
	while (!pModuleMgr->bHeartThreadExit)
	{
		Common_Sleep(1,0);
		if(dwTestCnt > 10)
		{
			dwTestCnt = 0;
			Module_StreamQueue_Test(pModuleMgr->pszModuleName);
		}
		dwTestCnt++;
		if (pModuleMgr->bManager)
		{
			S32 bReport = 0;
			bReport = Module_Register_Offline_Check((ModuleHandle_T)pModuleMgr);
			if(bReport ||
			   dwRegRefreshFlag != pModuleMgr->dwRegRefreshFlag)
			{

				static_Module_ReportModuleList(pModuleMgr,&dwRegRefreshFlag);
				Module_Register_Save((ModuleHandle_T)pModuleMgr);
			}
			if (pModuleMgr->bNeedReportProxy)
			{
				Module_UriProxy_Update((ModuleHandle_T)pModuleMgr);
			}



		}
		else
		{
			static_Module_Register(pModuleMgr);
			static_Module_Heart(pModuleMgr);

		}
		Module_Subscribe_CheckSubmit((ModuleHandle_T)pModuleMgr);
		Module_Subscribe_do_Offline(pModuleMgr->hModuleHandle);
		dwTestQueue++;
		if(dwTestQueue > 10)
		{
			dwTestQueue= 0;
			StreamQueue_CheckValid(pModuleMgr);
		}


	}
	return 0;
}
/*
SystemName:,
ModuleName:,
RemoteSvrDomain:,
RemoteSvrPort:,
LocalSvrDomain:,
LocalSvrPort:,
SvrType:
*/

static void staticDealSIGPIPE(int sig)
{
#ifndef WIN32
	signal(SIGPIPE, staticDealSIGPIPE);
#endif
}

S32 Module_Init(ModuleHandle_T *pModuleHandle,cJSON_Struct *pInitConfig,cJSON_Struct **pOutConfig,Module_CallFunctions_Def fxn,void *pUserData)
{
	LibModuleInfo_T *pModule = NULL;
	char *pSystemName =NULL,*pModuleName=NULL,*pRemoteSvrDomain=NULL,*pLocalSvrDomain=NULL;
	S32 nSvrType = 0,nLocalSvrPort = -1,nRemoteSvrPort = -1;

	char *pStringValue;
	S32 nIntValue;
	S32 nRet = -1;
	S32 bUseLocal = 0;
	if (pModuleHandle == NULL || pInitConfig == NULL || fxn == NULL)
	{
		return -1;
	}
	if (pOutConfig != NULL && *pOutConfig != NULL)
	{
		return -1;
	}
	if(Common_InstanceIsRunning())
	{
		return MODULE_ERROR_TYPE_RUNNING;
	}
#ifndef WIN32
	signal(SIGPIPE, staticDealSIGPIPE);
#endif
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"SystemName",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pSystemName = pStringValue;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"ModuleName",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pModuleName = pStringValue;
	}
	if(pModuleName != NULL)
	{
		if(pModuleName[0]>= 'a' && pModuleName[0] <= 'z')
		{
			pModuleName[0] += 'A' - 'a';
		}
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"RemoteDomain",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pRemoteSvrDomain = pStringValue;
	}
	nIntValue = -1;
	Common_Json_GetAttrValue(pInitConfig,-1,"RemotePort",NULL,&pStringValue,&nIntValue,NULL);
	if (nIntValue != -1)
	{
		nRemoteSvrPort = nIntValue;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"LocalDomain",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pLocalSvrDomain = pStringValue;
	}
	nIntValue = -1;
	Common_Json_GetAttrValue(pInitConfig,-1,"LocalPort",NULL,&pStringValue,&nIntValue,NULL);
	if (nIntValue != -1)
	{
		nLocalSvrPort = nIntValue;
		bUseLocal = 1;
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(pInitConfig,-1,"SvrType",NULL,&pStringValue,&nIntValue,NULL);
	if (nIntValue != -1)
	{
		nSvrType = nIntValue;
	}
	{
		cJSON_Struct *pFactoryJson = NULL;
		Module_LoadConfigByType(NULL,Module_ConfigType_Debug,&pFactoryJson);
		if (pFactoryJson != NULL)
		{
			S32 bEnable = 0,nSec = 10;
			Common_Json_GetAttrValue(pFactoryJson,-1,"/Debug/Mem/Enable",NULL,NULL,&bEnable,NULL);
			Common_Json_GetAttrValue(pFactoryJson,-1,"/Debug/Mem/IntervalSec",NULL,NULL,&nSec,NULL);
			if (nSec < 10)
			{
				nSec = 10;
			}
			else if (nSec > 60 * 60)
			{
				nSec = 60 * 60;
			}
			S8 szPath[128];
			Common_File_MkDir("/tmp/debug");
			sprintf(szPath,"/tmp/debug/%s.mem",pModuleName?pModuleName:"Comm");
			if (bEnable)
			{
				Common_Memory_StartDebug(1,nSec,szPath);
			}
			else
			{
				Common_Memory_StopDebug();
			}


			Common_Json_Delete_ex(pFactoryJson,__FUNCTION__,__LINE__);
		}
	}


	pModule = (LibModuleInfo_T *)new LibModuleInfo_T;
	if (pModule == NULL)
	{
		return -1;
	}
	{
		S8 szLog[128];
		sprintf(szLog,"Module<%s>",pModuleName);
		MODULE_LOG_INIT(szLog,COMMON_LOG_LV_HIGH);
	}


	memset(pModule,0,sizeof(LibModuleInfo_T));
	pModule->hModuleHandle = (ModuleHandle_T)pModule;
	S32 nSec = 0,nMSec = 0;
	Common_GetSystemCount(&nSec,&nMSec);
	if (0 == Common_StriCmp("core",pModuleName))
	{

		pModule->bManager = 1;

		pModule->uModuleID = (nSec << 8);
		pModule->uModuleID &= 0x7FFFFFFF;
	}
	pModule->nModuleMark = nSec * 100 + nMSec / 10;
	pModule->nServerSocket = -1;
	// 打开本地服务
	if(bUseLocal)
	{
		pModule->nServerSocket = ModuleOpenSocket(0,pLocalSvrDomain,nLocalSvrPort);
		if (pModule->nServerSocket != -1)
		{
			pModule->nServerPort = nLocalSvrPort;
		}
		else
		{
			MODULE_LOGE("tcp socket failed\n");
		}
	}

	pModule->nUnixSvrSocket = 1;
	// 在同一设备内优先使用linux 本地socket
	{
		char szLocalDomain[65];
		sprintf(szLocalDomain,"@%s",pModuleName);
		pModule->nUnixSvrSocket = ModuleOpenSocket(1,szLocalDomain,-1);
		if (pModule->nUnixSvrSocket != -1)
		{
			pModule->pszLocalDomain = Common_StrDup(szLocalDomain,__FUNCTION__,__LINE__);
		}
		else
		{
			MODULE_LOGE("Unix Local socket failed\n");
		}
	}

		if (pModule->nUnixSvrSocket == -1  && pModule->nServerSocket == -1)
		{
			Module_Unint((ModuleHandle_T *)&pModule);
			return -1;
		}



	if (pSystemName != NULL)
	{
		pModule->pszSystemName = Common_StrDup(pSystemName,__FUNCTION__,__LINE__);
	}
	if (pModuleName != NULL)
	{
		pModule->pszModuleName = Common_StrDup(pModuleName,__FUNCTION__,__LINE__);
	}
	if (pRemoteSvrDomain != NULL)
	{
		pModule->pszCoreIp = Common_StrDup(pRemoteSvrDomain,__FUNCTION__,__LINE__);
	}
	pModule->pszCoreDomain = Common_StrDup("@Core",__FUNCTION__,__LINE__);

	pModule->nCorePort = nRemoteSvrPort;
	Common_GetLocalNetInfo("eth0",0,NULL,NULL,NULL,&pModule->pszMac);
	S8 *pSerial = Module_GetSerialNumber(0);
	if (pSerial != NULL)
	{
		pModule->pszSerialNumber = Common_StrDup(pSerial,__FUNCTION__,__LINE__);
	}
	else if(pModule->pszMac != NULL)
	{
		U32 nMac[6];
		S8 szSerial[21],szSerial_read[21];
		int fd = -1,bWrite = 1;
		FILE *pf = NULL;
		sscanf(pModule->pszMac,"%02x:%02x:%02x:%02x:%02x:%02x",nMac,nMac+1,nMac+2,nMac+3,nMac+4,nMac+5);
		snprintf(szSerial,20,"00000000%02x%02x%02x%02x%02x%02x",nMac[0],nMac[1],nMac[2],nMac[3],nMac[4],nMac[5]);
		szSerial[20] = 0;
		memset(szSerial_read,0,21);
		do
		{
			 fd = open("/tmp/modules/uuid.lck", O_RDWR | O_CREAT | O_EXCL, 0444);
			 if(fd < 0)
			 {
			 	Common_Sleep(1, 0);
				continue;
			 }
			 pf = fopen("/tmp/modules/ovfs_uuid","ab+");
			 if(pf != NULL)
			 {

			 	// 读内容
			 	if(21 == fread(szSerial_read,1,21,pf))
			 	{
			 		//
			 		if(szSerial_read[20] == 0)
			 		{
			 			bWrite = 0;
			 		}
			 	}
				if(bWrite)
				{
					fwrite(szSerial,1,21,pf);
				}
				else
				{
					strcpy(szSerial,szSerial_read);
				}
				fclose(pf);

			 }


			close(fd);
			unlink( "/tmp/modules/uuid.lck");
			break;
		}while(1);
		pModule->pszSerialNumber = Common_StrDup(szSerial,__FUNCTION__,__LINE__);
	}



	pModule->nServerType = nSvrType;
	pModule->fCallFunction = fxn;
	pModule->pCallUserData = pUserData;
	Module_Register_Init((ModuleHandle_T)pModule);
	Module_StreamQueue_Init(pModule->hModuleHandle);
	Module_UriProxy_Init(pModule->hModuleHandle);
	Module_Subscribe_Init((ModuleHandle_T)pModule);

	////连接服务器
	// 注册
	nRet = 0;
	while(!pModule->bManager)
	{
		nRet = static_Module_Register(pModule);
		if(nRet == 0 || nRet == MODULE_ERROR_TYPE_RUNNING ||
			nRet == MODULE_ERROR_TYPE_LIMITED)
		{
			break;
		}
		Common_Sleep(1,0);

	}
	MODULE_LOGD("reg ok\n");
	if (nRet != 0)
	{
		Module_Unint((ModuleHandle_T *)&pModule);
		return -1;
	}

	// 开启本地服务监听线程
	{
		S8 szThreadName[128];
		sprintf(szThreadName,"Module_%s_ListenThread",pModuleName);
		if(Common_Thread_Create(&pModule->hListenThread,szThreadName,0,0,static_Module_Thread_Listen,pModule))
		{
			Module_Unint((ModuleHandle_T *)&pModule);
			return -1;
		}
	}

	// 对core 来说，是模块管理,上线，掉线广播发布
    // 对非core来说，是发送心跳，注册模块线程
	{
    	S8 szThreadName[128];
		sprintf(szThreadName,"Module_%s_HeartThread",pModuleName);
		if (Common_Thread_Create(&pModule->hHeartThread,szThreadName,0,0,static_Module_Thread_Heart,pModule))
		{
			Module_Unint((ModuleHandle_T *)&pModule);
			return -1;
		}
	}
	*pModuleHandle = (ModuleHandle_T)pModule;

	return nRet;
}
S32 Module_Init_Ex(ModuleHandle_T *pModuleHandle,cJSON_Struct *pInitConfig,cJSON_Struct **pOutConfig,Module_CallFunctions_Def fxn,void *pUserData)
{
	LibModuleInfo_T *pModule = NULL;
	char *pSystemName =NULL,*pModuleName=NULL,*pRemoteSvrDomain=NULL,*pLocalSvrDomain=NULL;
	S32 nSvrType = 0,nLocalSvrPort = -1,nRemoteSvrPort = -1;

	char *pStringValue;
	S32 nIntValue;
	S32 nRet = -1;
	S32 bUseLocal = 0;
	if (pModuleHandle == NULL || pInitConfig == NULL || fxn == NULL)
	{
		return -1;
	}
	if (pOutConfig != NULL && *pOutConfig != NULL)
	{
		return -1;
	}
#ifndef WIN32
	signal(SIGPIPE, staticDealSIGPIPE);
#endif
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"SystemName",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pSystemName = pStringValue;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"ModuleName",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pModuleName = pStringValue;
	}
	if(pModuleName != NULL)
	{
		if(pModuleName[0]>= 'a' && pModuleName[0] <= 'z')
		{
			pModuleName[0] += 'A' - 'a';
		}
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"RemoteDomain",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pRemoteSvrDomain = pStringValue;
	}
	nIntValue = -1;
	Common_Json_GetAttrValue(pInitConfig,-1,"RemotePort",NULL,&pStringValue,&nIntValue,NULL);
	if (nIntValue != -1)
	{
		nRemoteSvrPort = nIntValue;
	}
	pStringValue = NULL;
	Common_Json_GetAttrValue(pInitConfig,-1,"LocalDomain",NULL,&pStringValue,NULL,NULL);
	if (pStringValue != NULL)
	{
		pLocalSvrDomain = pStringValue;
	}
	nIntValue = -1;
	Common_Json_GetAttrValue(pInitConfig,-1,"LocalPort",NULL,&pStringValue,&nIntValue,NULL);
	if (nIntValue != -1)
	{
		nLocalSvrPort = nIntValue;
		bUseLocal = 1;
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(pInitConfig,-1,"SvrType",NULL,&pStringValue,&nIntValue,NULL);
	if (nIntValue != -1)
	{
		nSvrType = nIntValue;
	}
	{
		cJSON_Struct *pFactoryJson = NULL;
		Module_LoadConfigByType(NULL,Module_ConfigType_Debug,&pFactoryJson);
		if (pFactoryJson != NULL)
		{
			S32 bEnable = 0,nSec = 10;
			Common_Json_GetAttrValue(pFactoryJson,-1,"/Debug/Mem/Enable",NULL,NULL,&bEnable,NULL);
			Common_Json_GetAttrValue(pFactoryJson,-1,"/Debug/Mem/IntervalSec",NULL,NULL,&nSec,NULL);
			if (nSec < 10)
			{
				nSec = 10;
			}
			else if (nSec > 60 * 60)
			{
				nSec = 60 * 60;
			}
			S8 szPath[128];
			Common_File_MkDir("/tmp/debug");
			sprintf(szPath,"/tmp/debug/%s.mem",pModuleName?pModuleName:"Comm");
			if (bEnable)
			{
				Common_Memory_StartDebug(1,nSec,szPath);
			}
			else
			{
				Common_Memory_StopDebug();
			}


			Common_Json_Delete_ex(pFactoryJson,__FUNCTION__,__LINE__);
		}
	}


	pModule = (LibModuleInfo_T *)new LibModuleInfo_T;
	if (pModule == NULL)
	{
		return -1;
	}
	{
		S8 szLog[128];
		sprintf(szLog,"Module<%s>",pModuleName);
		MODULE_LOG_INIT(szLog,COMMON_LOG_LV_HIGH);
	}


	memset(pModule,0,sizeof(LibModuleInfo_T));
	pModule->hModuleHandle = (ModuleHandle_T)pModule;
	S32 nSec = 0,nMSec = 0;
	Common_GetSystemCount(&nSec,&nMSec);
	if (0 == Common_StriCmp("core",pModuleName))
	{

		pModule->bManager = 1;

		pModule->uModuleID = (nSec << 8);
		pModule->uModuleID &= 0x7FFFFFFF;
	}
	pModule->nModuleMark = nSec * 100 + nMSec / 10;
	pModule->nServerSocket = -1;
	// 打开本地服务
	if(bUseLocal)
	{
		pModule->nServerSocket = ModuleOpenSocket(0,pLocalSvrDomain,nLocalSvrPort);
		if (pModule->nServerSocket != -1)
		{
			pModule->nServerPort = nLocalSvrPort;
		}
		else
		{
			MODULE_LOGE("tcp socket failed\n");
		}
	}

	pModule->nUnixSvrSocket = 1;
	// 在同一设备内优先使用linux 本地socket
	{
		char szLocalDomain[65];
		sprintf(szLocalDomain,"@%s",pModuleName);
		pModule->nUnixSvrSocket = ModuleOpenSocket(1,szLocalDomain,-1);
		if (pModule->nUnixSvrSocket != -1)
		{
			pModule->pszLocalDomain = Common_StrDup(szLocalDomain,__FUNCTION__,__LINE__);
		}
		else
		{
			MODULE_LOGE("Unix Local socket failed\n");
		}
	}

		if (pModule->nUnixSvrSocket == -1  && pModule->nServerSocket == -1)
		{
			Module_Unint((ModuleHandle_T *)&pModule);
			return -1;
		}



	if (pSystemName != NULL)
	{
		pModule->pszSystemName = Common_StrDup(pSystemName,__FUNCTION__,__LINE__);
	}
	if (pModuleName != NULL)
	{
		pModule->pszModuleName = Common_StrDup(pModuleName,__FUNCTION__,__LINE__);
	}
	if (pRemoteSvrDomain != NULL)
	{
		pModule->pszCoreIp = Common_StrDup(pRemoteSvrDomain,__FUNCTION__,__LINE__);
	}
	pModule->pszCoreDomain = Common_StrDup("@Core",__FUNCTION__,__LINE__);

	pModule->nCorePort = nRemoteSvrPort;
	Common_GetLocalNetInfo("eth0",0,NULL,NULL,NULL,&pModule->pszMac);
	S8 *pSerial = Module_GetSerialNumber(0);
	if (pSerial != NULL)
	{
		pModule->pszSerialNumber = Common_StrDup(pSerial,__FUNCTION__,__LINE__);
	}
	else if(pModule->pszMac != NULL)
	{
		U32 nMac[6];
		S8 szSerial[21],szSerial_read[21];
		int fd = -1,bWrite = 1;
		FILE *pf = NULL;
		sscanf(pModule->pszMac,"%02x:%02x:%02x:%02x:%02x:%02x",nMac,nMac+1,nMac+2,nMac+3,nMac+4,nMac+5);
		snprintf(szSerial,20,"00000000%02x%02x%02x%02x%02x%02x",nMac[0],nMac[1],nMac[2],nMac[3],nMac[4],nMac[5]);
		szSerial[20] = 0;
		memset(szSerial_read,0,21);
		do
		{
			fd = open("/tmp/modules/uuid.lck", O_RDWR | O_CREAT | O_EXCL, 0444);
			if(fd < 0)
			{
				Common_Sleep(1, 0);
				continue;
			}
			pf = fopen("/tmp/modules/ovfs_uuid","ab+");
			if(pf != NULL)
			{
				// 读内容
				if(21 == fread(szSerial_read,1,21,pf))
				{
					//
					if(szSerial_read[20] == 0)
					{
						bWrite = 0;
					}
				}
				if(bWrite)
				{
					fwrite(szSerial,1,21,pf);
				}
				else
				{
					strcpy(szSerial,szSerial_read);
				}
				fclose(pf);
			}
			close(fd);
			unlink( "/tmp/modules/uuid.lck");
			break;
		}while(1);
		pModule->pszSerialNumber = Common_StrDup(szSerial,__FUNCTION__,__LINE__);
	}

	pModule->nServerType = nSvrType;
	pModule->fCallFunction = fxn;
	pModule->pCallUserData = pUserData;
	Module_Register_Init((ModuleHandle_T)pModule);
	Module_StreamQueue_Init(pModule->hModuleHandle);
	Module_UriProxy_Init(pModule->hModuleHandle);
	Module_Subscribe_Init((ModuleHandle_T)pModule);

	////连接服务器
	// 注册
	nRet = 0;
	while(!pModule->bManager)
	{
		nRet = static_Module_Register(pModule);
		if(nRet == 0 || nRet == MODULE_ERROR_TYPE_RUNNING ||
			nRet == MODULE_ERROR_TYPE_LIMITED)
		{
			break;
		}
		Common_Sleep(1,0);
	}
	MODULE_LOGD("reg ok\n");
	if (nRet != 0)
	{
		Module_Unint((ModuleHandle_T *)&pModule);
		return -1;
	}

	// 开启本地服务监听线程
	{
		S8 szThreadName[128];
		sprintf(szThreadName,"Module_%s_ListenThread",pModuleName);
		if(Common_Thread_Create(&pModule->hListenThread,szThreadName,0,0,static_Module_Thread_Listen,pModule))
		{
			Module_Unint((ModuleHandle_T *)&pModule);
			return -1;
		}
	}

	// 对core 来说，是模块管理,上线，掉线广播发布
    // 对非core来说，是发送心跳，注册模块线程
	{
    	S8 szThreadName[128];
		sprintf(szThreadName,"Module_%s_HeartThread",pModuleName);
		if (Common_Thread_Create(&pModule->hHeartThread,szThreadName,0,0,static_Module_Thread_Heart,pModule))
		{
			Module_Unint((ModuleHandle_T *)&pModule);
			return -1;
		}
	}
	*pModuleHandle = (ModuleHandle_T)pModule;

	return nRet;
}

S32 Module_Unint(ModuleHandle_T *pModuleHandle)
{
	return 0;
}

static S32 staticModule_CallCoreFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S32 bUnix = 0;
	S32 nRet = -1;
	if (pModuleMgr->bManager)
	{
		S8 *pUri = NULL,*pModuleName = NULL;
		LibModuleRegInfo_T *p = NULL;
		char *pszDomain = NULL;
		S32 nPort = -1;
		if (NULL == Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pUri,NULL,NULL))
		{
			return -1;
		}
		if (pUri == NULL)
		{
			return -1;
		}
		Common_UriOneParse(pUri,NULL,&pModuleName,NULL);
		if (pModuleName == NULL)
		{
			return -1;
		}
		if (0 == Common_StriCmp(pModuleName,pModuleMgr->pszModuleName))
		{
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return -1;
		}
		Common_Lock(pModuleMgr->hRegModuleLock);
		p = pModuleMgr->pModuleRegInfoHead;
		while (p != NULL)
		{

			if (!p->bOnline)
			{
				p = p->pNext;
				continue;
			}

			if (0 == stricmp(pModuleName,p->pszModuleName))
			{
				if (pModuleMgr->nUnixSvrSocket != -1)
				{

					if (p->pszSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
					{
						bUnix = 1;
					}
					else if (p->pszSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
					{
						if(0 == stricmp(p->pszSerialNumber,pModuleMgr->pszSerialNumber))
						{
							bUnix = 1;
						}
					}
				}

				// 同一设备内
				if (bUnix && p->pszDomain != NULL)
				{
					pszDomain = Common_StrDup(p->pszDomain,__FUNCTION__,__LINE__);
					nPort = -1;
				}
				else if(p->pszIpv4 != NULL)
				{
					pszDomain = Common_StrDup(p->pszIpv4,__FUNCTION__,__LINE__);
					nPort = p->nPort;
					bUnix = 0;
				}
				else
				{// 不能点对点访问，则交给管理者
					p = NULL;
					pszDomain = NULL;
				}

				break;
			}

			p = p->pNext;
		}
		Common_UnLock(pModuleMgr->hRegModuleLock);
		if (p == NULL)
		{
			// 没找到，则发给管理者
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return MODULE_ERROR_TYPE_INVALIDPARAM;
		}
		if (pszDomain != NULL)
		{
			nRet = static_Require(bUnix,pszDomain,nPort,pInParams,pOutParams,3000);
		}

		Common_Free(pszDomain,__FUNCTION__,__LINE__);
		Common_Free(pModuleName,__FUNCTION__,__LINE__);
		pModuleName = NULL;

		return nRet;
	}
	if (pModuleMgr->nUnixSvrSocket != -1)
	{

		if (pModuleMgr->pszCoreSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
		{
#ifdef WIN32
#else
		bUnix = 1;
#endif
		}
		else if (pModuleMgr->pszCoreSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
		{
			if(0 == stricmp(pModuleMgr->pszCoreSerialNumber,pModuleMgr->pszSerialNumber))
			{
#ifdef WIN32
#else
				bUnix = 1;
#endif
			}
		}
	}
	if (pModuleMgr->nServerSocket == -1)
	{
		bUnix = 1;
	}
	if (pModuleMgr->pszCoreIp == NULL)
	{
		bUnix = 1;
	}
	if (bUnix && pModuleMgr->pszCoreDomain != NULL)
	{
		nRet = static_Require(1,pModuleMgr->pszCoreDomain,0,pInParams,pOutParams,3000);
	}
	else if(pModuleMgr->pszCoreIp != NULL)
	{
		nRet = static_Require(0,pModuleMgr->pszCoreIp,pModuleMgr->nCorePort,pInParams,pOutParams,3000);
	}
	return nRet ;
}

static S32 staticModule_Core_Broadcast_OtherCallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S32 bUnix = 0;
	S32 nRet = -1;

	S8 *pUri = NULL,*pModuleName = NULL, *pOldUri = NULL;
	LibModuleRegInfo_T *p = NULL,*p1 = NULL;
	char *pszDomain = NULL;
	S32 nPort = -1;
	S8 *pAddUri = NULL;
	if (!pModuleMgr->bManager)
	{
		return -1;
	}
	if (NULL == Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pUri,NULL,NULL))
	{
		return -1;
	}
	if (pUri == NULL)
	{
		return -1;
	}
	pAddUri = strstr(pUri,"/Broadcast/");
	if (pAddUri == NULL)
	{
		return -1;
	}
	pOldUri = Common_StrDup(pUri,__FUNCTION__,__LINE__);
	if (pOldUri == NULL)
	{
		return 0;
	}
	pAddUri = strstr(pOldUri,"/Broadcast/");
	pAddUri += strlen("/Broadcast/");

	Common_Lock(pModuleMgr->hRegModuleLock);
	p = pModuleMgr->pModuleRegInfoHead;
	while (p != NULL)
	{

		p1 = p->pNext;
		if (!p->bOnline)
		{
			p = p->pNext;
			continue;
		}
		if (0 == Common_StriCmp(p->pszModuleName,pModuleMgr->pszModuleName))
		{
			p = p->pNext;
			continue;
		}


			if (pModuleMgr->nUnixSvrSocket != -1)
			{

				if (p->pszSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
				{
					bUnix = 1;
				}
				else if (p->pszSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
				{
					if(0 == stricmp(p->pszSerialNumber,pModuleMgr->pszSerialNumber))
					{
						bUnix = 1;
					}
				}
			}

			// 同一设备内
			if (bUnix && p->pszDomain != NULL)
			{
				pszDomain = Common_StrDup(p->pszDomain,__FUNCTION__,__LINE__);
				nPort = -1;
			}
			else if(p->pszIpv4 != NULL)
			{
				pszDomain = Common_StrDup(p->pszIpv4,__FUNCTION__,__LINE__);
				nPort = p->nPort;
				bUnix = 0;
			}
			else
			{// 不能点对点访问，则交给管理者
				p = NULL;
				pszDomain = NULL;
			}

			if (pszDomain != NULL)
			{
				Common_UnLock(pModuleMgr->hRegModuleLock);
				S8 szTmp[256]={0};
				snprintf(szTmp,255,"/%s/%s",p->pszModuleName,pAddUri);
				szTmp[255] = 0;
				Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,szTmp,0,0);
				static_Require(bUnix,pszDomain,nPort,pInParams,NULL,3000);
				Common_Lock(pModuleMgr->hRegModuleLock);
			}
			Common_Free(pszDomain,__FUNCTION__,__LINE__);
			pszDomain = NULL;



		p = p1;
	}
	Common_UnLock(pModuleMgr->hRegModuleLock);
	if (pOldUri != NULL)
	{
		Common_Json_SetAttrValue(pInParams,-1,"Header/Uri",Common_Json_Type_String,pOldUri,0,0);
		Common_Free(pOldUri,__FUNCTION__,__LINE__);
	}


	Common_Free(pszDomain,__FUNCTION__,__LINE__);


	return 0;


}

static S32 staticModule_Core_to_OtherCallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	S32 bUnix = 0;
	S32 nRet = -1;

		S8 *pUri = NULL,*pModuleName = NULL;
		LibModuleRegInfo_T *p = NULL;
		char *pszDomain = NULL;
		S32 nPort = -1;
		if (!pModuleMgr->bManager)
		{
			return -1;
		}
		if (0 == staticModule_Core_Broadcast_OtherCallFunctions(hModuleHandle,pInParams,pOutParams,nTimeOut))
		{
			return 0;
		}
		if (NULL == Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pUri,NULL,NULL))
		{
			return -1;
		}
		if (pUri == NULL)
		{
			return -1;
		}
		Common_UriOneParse(pUri,NULL,&pModuleName,NULL);
		if (pModuleName == NULL)
		{
			return -1;
		}
		if (0 == Common_StriCmp(pModuleName,pModuleMgr->pszModuleName))
		{
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return -1;
		}
		Common_Lock(pModuleMgr->hRegModuleLock);
		p = pModuleMgr->pModuleRegInfoHead;
		while (p != NULL)
		{

			if (!p->bOnline)
			{
				p = p->pNext;
				continue;
			}

			if (0 == stricmp(pModuleName,p->pszModuleName))
			{
				if (pModuleMgr->nUnixSvrSocket != -1)
				{

					if (p->pszSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
					{
						bUnix = 1;
					}
					else if (p->pszSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
					{
						if(0 == stricmp(p->pszSerialNumber,pModuleMgr->pszSerialNumber))
						{
							bUnix = 1;
						}
					}
				}

				// 同一设备内
				if (bUnix && p->pszDomain != NULL)
				{
					pszDomain = Common_StrDup(p->pszDomain,__FUNCTION__,__LINE__);
					nPort = -1;
				}
				else if(p->pszIpv4 != NULL)
				{
					pszDomain = Common_StrDup(p->pszIpv4,__FUNCTION__,__LINE__);
					nPort = p->nPort;
					bUnix = 0;
				}
				else
				{// 不能点对点访问，则交给管理者
					p = NULL;
					pszDomain = NULL;
				}

				break;
			}

			p = p->pNext;
		}
		Common_UnLock(pModuleMgr->hRegModuleLock);
		if (p == NULL)
		{
			// 没找到，则发给管理者
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return MODULE_ERROR_TYPE_INVALIDPARAM;
		}
		if (pszDomain != NULL)
		{
			nRet = static_Require(bUnix,pszDomain,nPort,pInParams,pOutParams,nTimeOut);
		}

		Common_Free(pszDomain,__FUNCTION__,__LINE__);
		Common_Free(pModuleName,__FUNCTION__,__LINE__);
		pModuleName = NULL;

		return nRet;


}


S32 Module_CallFunctions(ModuleHandle_T hModuleHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	char *pUri = NULL,*pModuleName = NULL;
	LibModuleRegInfo_T *p;
	S32 nRet = -1;
	char *pszDomain = NULL;
	S32 nPort = -1;
	S32 bUnix = 0;
	if (pModuleMgr == NULL || pInParams == NULL)
	{
		return -1;
	}
	if (pOutParams != NULL && *pOutParams != NULL)
	{
		return -1;
	}
	if (nTimeOut < 3000)
	{
		nTimeOut = 3000;
	}
	if (NULL == Common_Json_GetAttrValue(pInParams,-1,"Header/Uri",NULL,&pUri,NULL,NULL))
	{
		return -1;
	}
	if (pUri == NULL)
	{
		return -1;
	}
	// 加入源
	{
		S8 *szFromUri = NULL;
		char szUri[128];
		sprintf(szUri,"/%s",pModuleMgr->pszModuleName);
		Common_Json_GetAttrValue(pInParams,-1,"Header/From/Uri",NULL,&szFromUri,NULL,NULL);
		if (szFromUri == NULL)
		{
			Common_Json_SetAttrValue(pInParams,-1,"Header/From",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pInParams,-1,"Header/From/Uri",Common_Json_Type_String,szUri,0,0);
		}
		else if(0 != Common_StriCmp(szFromUri,szUri))
		{// 同模块
			Common_Json_SetAttrValue(pInParams,-1,"Header/From/ForwarderUri",Common_Json_Type_String,szUri,0,0);
		}

	}



	Common_UriOneParse(pUri,NULL,&pModuleName,NULL);

	if (0 == strcmp(pUri,"/"))
	{
		if (pModuleMgr->bManager)
		{//
			cJSON_Struct *pArray;
			cJSON_Struct *pResponce = NULL;
			S32 nWhich = 0;
			char szUri[128];
			pResponce = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResponce,-1,"Header/Code",Common_Json_Type_Number,NULL,MODULE_ERROR_TYPE_SUCC,0);
			pArray = Common_Json_SetAttrValue(pResponce,-1,"Ress",Common_Json_Type_Array,NULL,0,0);
			p = pModuleMgr->pModuleRegInfoHead;
			while (p != NULL)
			{
				sprintf(szUri,"/%s",p->pszModuleName);
				Common_Json_SetAttrValue(pArray,nWhich,"Uri",Common_Json_Type_String,szUri,0,0);
				Common_Json_SetAttrValue(pArray,nWhich,"Lable",Common_Json_Type_String,p->pszModuleName,0,0);
				Common_Json_SetAttrValue(pArray,nWhich,"Method",Common_Json_Type_String,"Get",0,0);
				sprintf(szUri,"Access to Module[%s].",p->pszModuleName);
				Common_Json_SetAttrValue(pArray,nWhich,"Describe",Common_Json_Type_String,szUri,0,0);
				nWhich++;
				p = p->pNext;
			}
			if (pOutParams)
			{
				*pOutParams = pResponce;
				pResponce = NULL;
			}
			if (pResponce)
			{
				Common_Json_Delete(pResponce);
				pResponce = NULL;
			}
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return 0;
		}
		else
		{
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return  staticModule_CallCoreFunctions(hModuleHandle,pInParams,pOutParams,nTimeOut);
		}
	}
#if 0
	// 直接管理者转发
	nRet =  staticModule_CallCoreFunctions(hModuleHandle,pInParams,pOutParams,nTimeOut);
	return nRet;
#endif
	if (pModuleName == NULL)
	{
		return MODULE_ERROR_TYPE_INVALIDPARAM;
	}
	if (0 == stricmp(pModuleName,pModuleMgr->pszModuleName))
	{// 调用自己, 直接到回调里
		S32 nRet = extern_Module_CallFunctions_MsgRouter(hModuleHandle,NULL,pInParams,pOutParams);
		Common_Free(pModuleName,__FUNCTION__,__LINE__);
		pModuleName = NULL;
		return nRet;
	}
	if (pModuleMgr->bManager)
	{
		Common_Free(pModuleName,__FUNCTION__,__LINE__);
		pModuleName = NULL;
		return staticModule_Core_to_OtherCallFunctions(hModuleHandle,pInParams,pOutParams,nTimeOut);
	}
	Common_Lock(pModuleMgr->hRegModuleLock);
	p = pModuleMgr->pModuleRegInfoHead;
	while (p != NULL)
	{

		if (!p->bOnline)
		{
			p = p->pNext;
			continue;
		}
		if (0 == stricmp(pModuleName,p->pszModuleName))
		{
			if (pModuleMgr->nUnixSvrSocket != -1)
			{
				if (p->pszSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
				{
					if (p->pszDomain != NULL)
					{
						bUnix = 1;
					}

				}
				else if (p->pszSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
				{
					if(0 == stricmp(p->pszSerialNumber,pModuleMgr->pszSerialNumber))
					{
						if (p->pszDomain != NULL)
						{
							bUnix = 1;
						}

					}
				}

			}

				// 同一设备内
			if (bUnix && p->pszDomain != NULL)
			{
				pszDomain = Common_StrDup(p->pszDomain,__FUNCTION__,__LINE__);
				nPort = -1;
			}
			else if(p->pszIpv4 != NULL)
			{
				pszDomain = Common_StrDup(p->pszIpv4,__FUNCTION__,__LINE__);
				nPort = p->nPort;
				bUnix = 0;
			}
			else
			{// 不能点对点访问，则交给管理者
				p = NULL;
			}

			break;
		}

		p = p->pNext;
	}
	Common_UnLock(pModuleMgr->hRegModuleLock);
	if (p == NULL)
	{
		// 没找到，则发给管理者
		if (pModuleMgr->bManager)
		{
			Common_Free(pModuleName,__FUNCTION__,__LINE__);
			pModuleName = NULL;
			return MODULE_ERROR_TYPE_INVALIDPARAM;
		}
		if (pModuleMgr->nUnixSvrSocket != -1)
		{
			if (pModuleMgr->pszCoreSerialNumber == NULL && pModuleMgr->pszSerialNumber == NULL)
			{
				bUnix = 1;
			}
			else if (pModuleMgr->pszCoreSerialNumber != NULL && pModuleMgr->pszSerialNumber != NULL)
			{
				if(0 == stricmp(pModuleMgr->pszCoreSerialNumber,pModuleMgr->pszSerialNumber))
				{

					bUnix = 1;
				}
			}
		}
		if (pModuleMgr->nServerSocket == -1)
		{
			bUnix = 1;
		}
		if (bUnix && pModuleMgr->pszCoreDomain != NULL)
		{
			pszDomain = Common_StrDup(pModuleMgr->pszCoreDomain,__FUNCTION__,__LINE__);
			nPort = -1;
		}
		else if(pModuleMgr->pszCoreIp !=NULL)
		{

			pszDomain = Common_StrDup(pModuleMgr->pszCoreIp,__FUNCTION__,__LINE__);
			nPort = pModuleMgr->nCorePort;
			bUnix = 0;
		}
		else
		{

			pszDomain = NULL;
		}

	}

	if (pszDomain != NULL)
	{

		nRet = static_Require(bUnix,pszDomain,nPort,pInParams,pOutParams,nTimeOut);
	}



	Common_Free(pszDomain,__FUNCTION__,__LINE__);
	Common_Free(pModuleName,__FUNCTION__,__LINE__);
	pModuleName = NULL;
	//
	return nRet;
}

static S32 static_LoadConfigByPath(const S8 *pcPath, cJSON_Struct **ppConfig)
{
	S32 nStrLen       = 0;
	S8 *pConfigString = NULL;

	FILE *fp = NULL;

    fp = fopen(pcPath, "rb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);

        if (nStrLen > 0)
		{
			pConfigString = (S8 *)Common_Malloc(nStrLen, 0, __FUNCTION__, __LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString, 1, (U32)nStrLen, fp))
				{
					*ppConfig = Common_Json_Parse(pConfigString, NULL, NULL);
				}
			}
		}

        fclose(fp);
	}

    if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}

    if (NULL == *ppConfig)
	{
		return -1;
	}

    return 0;
}

static S32 static_SaveConfigByPath(const S8 *pcPath, cJSON_Struct *pConfig)
{
    S32 nRet    = -1;
	S32 nStrLen =  0;

    FILE *fp    = NULL;
	S8 *pConfigString = NULL;

    fp = fopen(pcPath, "wb+");
	if (fp != NULL)
	{
		pConfigString = Common_Json_Print(pConfig, &nStrLen);
		if (pConfigString && nStrLen > 0)
		{
			if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
			{
				nRet = 0;
			}
		}

        fclose(fp);
	}

    if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}

    return nRet;
}

S32 Module_LoadConfig(ModuleHandle_T hModuleHandle,cJSON_Struct **pConfig)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pConfig == NULL || *pConfig != NULL)
	{
		return -1;
	}
	sprintf(szPath,"%s/%s.json",CFG_PATH,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"rb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		if (nStrLen > 0)
		{
			pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString,1,(U32)nStrLen,fp))
				{
					*pConfig = Common_Json_Parse(pConfigString,NULL,NULL);
				}
			}

		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	if (*pConfig == NULL)
	{
		return -1;
	}
	return 0;
}

S32 Module_SaveConfig(ModuleHandle_T hModuleHandle,cJSON_Struct *pConfig)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	S32 nRet = -1;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pConfig == NULL)
	{
		return -1;
	}
	if (0 != Common_File_MkDir(CFG_PATH))
	{
		return -1;
	}

	sprintf(szPath,"%s/%s.json",CFG_PATH,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"wb+");
		if (fp != NULL)
		{
			pConfigString = Common_Json_Print(pConfig,&nStrLen);
			if (pConfigString && nStrLen > 0)
			{
				if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
				{
					nRet = 0;
				}
			}
			fclose(fp);
		}
		if (pConfigString != NULL)
		{
			Common_Free(pConfigString,__FUNCTION__,__LINE__);
			pConfigString = NULL;
		}
		return nRet;
}

S32 Module_LoadTempData(ModuleHandle_T hModuleHandle,cJSON_Struct **pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pTempJson == NULL || *pTempJson != NULL)
	{
		return -1;
	}
	sprintf(szPath,"%s/%s.json",TEMP_PATH,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"rb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		if (nStrLen > 0)
		{
			pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString,1,(U32)nStrLen,fp))
				{
					*pTempJson = Common_Json_Parse(pConfigString,NULL,NULL);
				}
			}

		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	if (*pTempJson == NULL)
	{
		return -1;
	}
	return 0;
}
S32 Module_SaveTempData(ModuleHandle_T hModuleHandle,cJSON_Struct *pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	S32 nRet = -1;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pTempJson == NULL)
	{
		return -1;
	}
	if (0 != Common_File_MkDir(TEMP_PATH))
	{
		return -1;
	}
	sprintf(szPath,"%s/%s.json",TEMP_PATH,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"wb+");
	if (fp != NULL)
	{
		pConfigString = Common_Json_Print(pTempJson,&nStrLen);
		if (pConfigString && nStrLen > 0)
		{
			if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
			{
				nRet = 0;
			}
		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	return nRet;
}

S32 Module_LoadDefault(ModuleHandle_T hModuleHandle,cJSON_Struct **pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pTempJson == NULL || *pTempJson != NULL)
	{
		return -1;
	}

    cJSON_Struct *pDefaultCustomConfig = NULL;
    snprintf(szPath,sizeof(szPath),"%s/%s.json",DEFAULT_PATH_CUSTOM,pModuleMgr->pszModuleName);
    static_LoadConfigByPath(szPath, &pDefaultCustomConfig);
    if(pDefaultCustomConfig)
    {
        *pTempJson = pDefaultCustomConfig;
        return 0;
    }

	sprintf(szPath,"%s/%s.json",DEFAULT_PATH_AUTO,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"rb");
	if (fp == NULL)
	{
		sprintf(szPath,"%s/%s.json",DEFAULT_PATH,pModuleMgr->pszModuleName);
		fp = fopen(szPath,"rb");
	}

	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		if (nStrLen > 0)
		{
			pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString,1,(U32)nStrLen,fp))
				{
					*pTempJson = Common_Json_Parse(pConfigString,NULL,NULL);
				}
			}

		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	if (*pTempJson == NULL)
	{
		return -1;
	}
	return 0;
}

S32 Module_SaveDefault(ModuleHandle_T hModuleHandle,cJSON_Struct *pConfig)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	S32 nRet = -1;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pConfig == NULL)
	{
		return -1;
	}
	if (0 != Common_File_MkDir(DEFAULT_PATH_CUSTOM))
	{
		return -1;
	}

	sprintf(szPath,"%s/%s.json",DEFAULT_PATH_CUSTOM,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"wb+");
		if (fp != NULL)
		{
			pConfigString = Common_Json_Print(pConfig,&nStrLen);
			if (pConfigString && nStrLen > 0)
			{
				if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
				{
					nRet = 0;
				}
			}
			fclose(fp);
		}
		if (pConfigString != NULL)
		{
			Common_Free(pConfigString,__FUNCTION__,__LINE__);
			pConfigString = NULL;
		}
		return nRet;
}

S32 Module_LoadCustom(ModuleHandle_T hModuleHandle,cJSON_Struct **pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pTempJson == NULL || *pTempJson != NULL)
	{
		return -1;
	}
	sprintf(szPath,"%s/%s.json",CUSTOM_PATH,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"rb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		if (nStrLen > 0)
		{
			pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString,1,(U32)nStrLen,fp))
				{
					*pTempJson = Common_Json_Parse(pConfigString,NULL,NULL);
				}
			}

		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	if (*pTempJson == NULL)
	{
		return -1;
	}
	return 0;
}

S32 Module_SaveCustom(ModuleHandle_T hModuleHandle,cJSON_Struct *pConfig)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	S32 nRet = -1;
	LibModuleInfo_T *pModuleMgr = (LibModuleInfo_T *)hModuleHandle;
	if (hModuleHandle == NULL || pConfig == NULL)
	{
		return -1;
	}
	if (0 != Common_File_MkDir(CUSTOM_PATH))
	{
		return -1;
	}

	sprintf(szPath,"%s/%s.json",CUSTOM_PATH,pModuleMgr->pszModuleName);
	fp = fopen(szPath,"wb+");
		if (fp != NULL)
		{
			pConfigString = Common_Json_Print(pConfig,&nStrLen);
			if (pConfigString && nStrLen > 0)
			{
				if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
				{
					nRet = 0;
				}
			}
			fclose(fp);
		}
		if (pConfigString != NULL)
		{
			Common_Free(pConfigString,__FUNCTION__,__LINE__);
			pConfigString = NULL;
		}
		return nRet;
}


S32 Module_LoadFactoryInfo(cJSON_Struct **pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	if (pTempJson == NULL || *pTempJson != NULL)
	{
		return -1;
	}

	sprintf(szPath,"%s/factoryinfo.json",FACTORY_PATH);
	fp = fopen(szPath,"rb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		if (nStrLen > 0)
		{
			pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString,1,(U32)nStrLen,fp))
				{
					*pTempJson = Common_Json_Parse(pConfigString,NULL,NULL);
				}
			}

		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	if (*pTempJson == NULL)
	{
		return -1;
	}
	return 0;
}

S32 Module_SaveFactoryInfo(cJSON_Struct *pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	S32 nRet = -1;
	if (pTempJson == NULL)
	{
		return -1;
	}
	if (0 != Common_File_MkDir(FACTORY_PATH))
	{
		return -1;
	}

	sprintf(szPath,"%s/factoryinfo.json",FACTORY_PATH);
	fp = fopen(szPath,"wb+");
	if (fp != NULL)
	{
		pConfigString = Common_Json_Print(pTempJson,&nStrLen);
		if (pConfigString && nStrLen > 0)
		{
			if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
			{
				nRet = 0;
			}
		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	return nRet;
}


S32 Module_LoadDebug(cJSON_Struct **pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	if (pTempJson == NULL || *pTempJson != NULL)
	{
		return -1;
	}

	sprintf(szPath,"%s/debug.json",FACTORY_PATH);
	fp = fopen(szPath,"rb");
	if (fp != NULL)
	{
		fseek(fp,0,SEEK_END);
		nStrLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		if (nStrLen > 0)
		{
			pConfigString = (char *)Common_Malloc(nStrLen,0,__FUNCTION__,__LINE__);
			if (pConfigString != NULL)
			{
				if(nStrLen == fread(pConfigString,1,(U32)nStrLen,fp))
				{
					*pTempJson = Common_Json_Parse(pConfigString,NULL,NULL);
				}
			}

		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	if (*pTempJson == NULL)
	{
		return -1;
	}
	return 0;
}

S32 Module_SaveDebug(cJSON_Struct *pTempJson)
{
	char *pConfigString = NULL;
	S32 nStrLen = 0;
	char szPath[128];
	FILE *fp;
	S32 nRet = -1;
	if (pTempJson == NULL)
	{
		return -1;
	}
	if (0 != Common_File_MkDir(FACTORY_PATH))
	{
		return -1;
	}

	sprintf(szPath,"%s/debug.json",FACTORY_PATH);
	fp = fopen(szPath,"wb+");
	if (fp != NULL)
	{
		pConfigString = Common_Json_Print(pTempJson,&nStrLen);
		if (pConfigString && nStrLen > 0)
		{
			if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
			{
				nRet = 0;
			}
		}
		fclose(fp);
	}
	if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}
	return nRet;
}
LIBMODULE_API S32 Module_LoadConfigByType(ModuleHandle_T hModuleHandle,Module_ConfigType_E nType,cJSON_Struct **pConfigJson)
{
	if (nType == 0)
	{
		return Module_LoadConfig(hModuleHandle,pConfigJson);
	}
	else if (nType == 1)
	{
		return Module_LoadDefault(hModuleHandle,pConfigJson);
	}
	else if (nType == 2)
	{
		return Module_LoadCustom(hModuleHandle,pConfigJson);
	}
	else if (nType == 3)
	{
		return Module_LoadTempData(hModuleHandle,pConfigJson);
	}
	else if (nType == 4)
	{
		return Module_LoadFactoryInfo(pConfigJson);
	}
	else if (nType == 5)
	{
		return Module_LoadDebug(pConfigJson);
	}
	return -1;
}
LIBMODULE_API S32 Module_SaveConfigByType(ModuleHandle_T hModuleHandle,Module_ConfigType_E nType,cJSON_Struct *pConfigJson)
{
	if (nType == 0)
	{
		return Module_SaveConfig(hModuleHandle,pConfigJson);
	}
	else if (nType == 1)
	{// load only
		//return -1;
	    return Module_SaveDefault(hModuleHandle,pConfigJson);
	}
	else if (nType == 2)
	{
		return Module_SaveCustom(hModuleHandle,pConfigJson);
	}
	else if (nType == 3)
	{
		return Module_SaveTempData(hModuleHandle,pConfigJson);
	}
	else if (nType == 4)
	{
		return Module_SaveFactoryInfo(pConfigJson);
	}
	else if (nType == 5)
	{
		return Module_SaveDebug(pConfigJson);
	}
	return -1;

}

/*****************************************************************************
 函 数 名  : Module_LoadConfigByPath
 功能描述  : 加载指定路径的配置文件
 输入参数  : ModuleHandle_T hModuleHandle 模块名
             const S8 *pcPath             配置文件路径及名称
 输出参数  : cJSON_Struct **ppConfigJson  配置文件
 返 回 值  : 0 加载成功; -1 加载失败
 调用函数  : static_LoadConfigByPath
 被调函数  :

 修改历史      :
  1.日    期   : 2019年6月28日
    作    者   : wangconglin
    修改内容   : 新生成函数

*****************************************************************************/
S32 Module_LoadConfigByPath(ModuleHandle_T hModuleHandle,
                                     const S8 *pcPath,
                                     cJSON_Struct **ppConfigJson)
{
    if ((NULL == pcPath) || (NULL == ppConfigJson) || (*ppConfigJson != NULL))
    {
        LOGE("Input parameters ERROR.\n");
        return -1;
    }

    hModuleHandle = hModuleHandle;

    return static_LoadConfigByPath(pcPath, ppConfigJson);
}

/*****************************************************************************
 函 数 名  : Module_SaveConfigByPath
 功能描述  : 保存配置到指定文件中
 输入参数  : ModuleHandle_T hModuleHandle 模块名
             const S8 *pcPath             配置文件路径及名称
             const cJSON_Struct *pConfigJson 配置文件
 输出参数  : 无
 返 回 值  : 0 保存成功; -1 保存失败
 调用函数  : static_SaveConfigByPath
 被调函数  :

 修改历史      :
  1.日    期   : 2019年6月28日
    作    者   : wangconglin
    修改内容   : 新生成函数

*****************************************************************************/
S32 Module_SaveConfigByPath(ModuleHandle_T hModuleHandle,
                                     const S8 *pcPath,
                                     cJSON_Struct *pConfigJson)
{
    if ((NULL == pcPath) || (NULL == pConfigJson))
    {
        LOGE("Input parameters ERROR.\n");
        return -1;
    }

    hModuleHandle = hModuleHandle;

    return static_SaveConfigByPath(pcPath, pConfigJson);
}

S32 Module_CallFunctions_Remote(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
{
    return static_Require(0, pDomain, nPort, pInParams, pOutParams, nTimeOut);
}
