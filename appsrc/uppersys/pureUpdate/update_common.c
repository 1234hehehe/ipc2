#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
#include <getopt.h>
#include <sys/mount.h>
#include <string.h>
#include <stdio.h>

#include "update_common.h"
#include "libcrypto_api.h"
#include "common_net.h"
#include "lib_encry.h"
#ifdef MAX_MTDBLOCK
#undef MAX_MTDBLOCK
#endif
#define MAX_MTDBLOCK	6
#define MOUNT_PARTITIONS_FILE   "/proc/mounts"

static S8 g_acSerialNumber[32 + 1] = {0};
static S8 g_acSerialUUID[64 + 1] = {0};
static S8 g_acHardVer[32 + 1] = {0};
static int BUFSIZE = 10240;

//static int BUFSIZE = 10240;
static S32 ReadFlashMtd(int iMtdBlockNum, char **pImageReadBuffer, int *iReadSize)
{
	int nRealSize;
	int ret = -1;
	int size;
	char device[32];
	struct mtd_info_user mtd;


	loff_t offs;
	off_t seek;
	S32 bNandFlash = 0;
	S32 dev_fd = -1;
	S8 *pBuffer = NULL;
	S32 nNeedSize = 0;
    //printf("[%s.%d] here  bn = %d,iwritesize = %d,istartpos = %d,ierasesize = %d\n",__FUNCTION__,__LINE__,iMtdBlockNum,iWriteSize,iStartPos,iEraseSize);
	if(iMtdBlockNum < 0 || iMtdBlockNum >= MAX_MTDBLOCK)
	{
		LOGE("Param err : iMtdBlockNum = %d\n", iMtdBlockNum);
		return -1;
	}

	bNandFlash = 0;

	sprintf(device, "/dev/mtd%d", iMtdBlockNum);
	dev_fd = open(device, O_SYNC | O_RDWR);
	if(dev_fd < 0)
	{
		LOGE("Device open err : device = %s, ret = %d", device, dev_fd);
		return -1;
	}
	ret = ioctl(dev_fd, MEMGETINFO, &mtd);
	if(ret < 0)
	{
		LOGE("This doesn't seem to be a valid MTD flash device :%s ret = %d\n", device, ret);
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
		LOGE("This doesn't seem to be a valid MTD flash device :%s type = %d ret = %d\n", device,mtd.type, ret);
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



	S32 verifysize;
	verifysize = 0;
	/******************** verify ********************/
	ret = lseek(dev_fd, 0, SEEK_SET);
	if(ret < 0)
	{
		LOGE("[%s.%d]While seeking to start of %s: %d\n", __FUNCTION__,__LINE__, device, ret);
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
				LOGE("[%s.%d]While reading data from %s\n", __FUNCTION__,__LINE__, device);
			else
				LOGE("[%s.%d]Short read count returned while reading from %s\n", __FUNCTION__,__LINE__, device);

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

static S32 ReadFactoryInfo(cJSON_Struct** pInfo)
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

    //可以直接读取 /proc/mtd文件解析分区表
    snprintf(cmd,sizeof(cmd),"cat /proc/mtd | grep \"%s\" | awk \'{print $1}\'","factory");
    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        if (0 == Common_StrnCmp(buf, "mtd", strlen("mtd")))
        {
            iMtdBlockNum = atoi(buf + strlen("mtd"));
        }
    }

    nRet = ReadFlashMtd(iMtdBlockNum, &pStr, &nLen);
    if(pStr != NULL)
    {
        *pInfo = Common_Json_Parse(pStr, NULL, NULL);
        Common_Free(pStr,__FUNCTION__,__LINE__);
    }

    return nRet;
}

static void GetHardVerUUIDandSN()
{
    S8 *szTime         = NULL;
    S8 *szLicence      = NULL;
    S8 *szNewUUID      = NULL;
    S8 *szNewHardware  = NULL;
    S8 *szSerialNumber = NULL;
    S8 *szCertificate  = NULL;

    cJSON_Struct *pInfo    = NULL;
    cJSON_Struct *pOutJson = NULL;

    (void)ReadFactoryInfo(&pInfo);

    if(pInfo != NULL)
    {
        Common_Json_GetAttrValue(pInfo,-1,"Licence",NULL,&szLicence,NULL,NULL);
        if(szLicence != NULL)
        {
            if(0 == ovfs_Licence_dec(szLicence, &szNewHardware,
                                      &szNewUUID, &szSerialNumber,
                                      &szCertificate, &szTime, (void **)&pOutJson))
            {
                if (szNewUUID != NULL)
                {
                    snprintf(g_acSerialUUID, sizeof(g_acSerialUUID) - 1, "%s", szNewUUID);
                }

                if (szSerialNumber != NULL)
                {
                    snprintf(g_acSerialNumber, sizeof(g_acSerialNumber) - 1, "%s", szSerialNumber);
                }

                if (szNewHardware != NULL)
                {
                    snprintf(g_acHardVer, sizeof(g_acHardVer) - 1, "%s", szNewHardware);
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

	LOGD("g_acSerialNumber = %s\n", g_acSerialNumber);
	if(g_acSerialNumber[0] == 0 && g_acSerialNumber[1] == 0 && g_acSerialNumber[2] == 0 && g_acSerialNumber[3] == 0)
	{
		U8 szInfo[20] = {0};
		U8 szLot[8];
		if(0 == GetUserZoneInf(2, (unsigned char*)szInfo, 16))
		{
			GetLot((unsigned char*)szLot);

			sprintf(g_acSerialNumber,"%02x%02x",szInfo[10],szInfo[11]);
			sprintf(g_acSerialNumber + 4,"%02x%02x%02x%02x%02x%02x%02x%02x",szLot[0],szLot[1],szLot[2],szLot[3],szLot[4],szLot[5],szLot[6],szLot[7]);
		}
	}
}

S8 *update_common_GetUUID()
{
    if(g_acSerialUUID[0] != 0)
    {
        return g_acSerialUUID;
    }

    //主动获取
    GetHardVerUUIDandSN();

    if(g_acSerialUUID[0] != 0)
    {
        return g_acSerialUUID;
    }

    return NULL;
}

S8 *update_common_GetSerialNumber()
{
    if(g_acSerialNumber[0] != 0)
    {
        return g_acSerialNumber;
    }

    //主动获取
    GetHardVerUUIDandSN();

    if(g_acSerialNumber[0] != 0)
    {
        return g_acSerialNumber;
    }

    return NULL;
}

S8 *update_common_GetHardwareVersion()
{
    if(g_acHardVer[0] != 0)
    {
        return g_acHardVer;
    }

    //主动获取
    GetHardVerUUIDandSN();

    if(g_acHardVer[0] != 0)
    {
        return g_acHardVer;
    }

    return NULL;
}

//通过解析"/proc/mounts"文件，找并卸载已经挂载的分区.
S32 udpate_common_UmountPartition()
{
    S32 i    =  0;
    S32 lRet = -1;

    // kill all ovfs process
    if (access("/root/killtwo.sh", F_OK) == 0)
    {
        Common_System("/root/killtwo.sh");
    }
    else
    {
        Common_System("pkill -9 ovfs_");
        Common_System("pkill -9 auto");
        Common_System("pkill -9 nginx");
        Common_System("pkill -9 sysinit");
        Common_System("killall -9 appinstall");
    }

    if (access("/tmp/updating_restore", F_OK) == 0)
    {
        LOGW("updating_restore!\n");
        Common_System("rm /usr/etc/cfgfiles/*;touch /usr/etc/restore_other");
    }

	Common_System("umount /root/nginx/conf");
	Common_System("umount /root/nginx/logs");
	Common_System("umount /root/nginx/fastcgi_temp");
	Common_System("umount /tmp/nginx/proxy_temp");

    Common_Sleep(1, 0);
    Common_System("sysctl -w vm.drop_caches=3;sync");

    //是否发生错误
    S32 lUmountErr = 0;

    //记录分区是否已经成功卸载
    static int s_lUmountSuccess = 0;

    FILE *fp = NULL;

    S8 acLine[256] = {0};
    memset((void *)acLine, 0x00, sizeof(acLine));

    S8 *paPartList[4] = {"/update", "/root", "/usr/etc", "/smart"};

    if (1 == s_lUmountSuccess)
    {
        return 0;
    }

    fp = fopen(MOUNT_PARTITIONS_FILE, "r");
    if (NULL == fp)
    {
        LOGE("fopen %s file failed.\n", MOUNT_PARTITIONS_FILE);
        return -1;
    }

    while (fgets(acLine, sizeof(acLine), fp))
    {
        if (acLine[0] != 0)
        {
            for (i = 0; i < 4; i++)
            {
                if (strstr(acLine, paPartList[i]))
                {
                	char *szCheckUbi = NULL;
			LOGD("umount2 %s.<%s>\n", paPartList[i],acLine);

                    lRet = umount2(paPartList[i], MNT_FORCE);
                    if (lRet < 0)
                    {
                        //卸载失败
                        lUmountErr = 1;

                        // 如果umount失败,是否需要继续升级.
                        LOGE("umount failed lRet = %d %d<%s>\n", lRet, GetLastError(), strerror(GetLastError()));
                    }

                    //卸载ubi 卷
                    szCheckUbi = strstr(acLine,"/dev/ubi");
                    if(szCheckUbi != NULL)
                    {
                        int nUBI_No = -1;
                        nUBI_No = atoi(szCheckUbi + strlen("/dev/ubi"));
                        if(nUBI_No > 0)
                        {
                            char szcmd[64];
                            sprintf(szcmd,"ubidetach -d %d /dev/ubi_ctrl",nUBI_No);
                            Common_System(szcmd);
			    }

                    }
                }

            }
        }

        memset((void *)acLine, 0x00, sizeof(acLine));
    }

    //分区卸载成功
    if (0 == lUmountErr)
    {
        s_lUmountSuccess = 1;
    }

    fclose(fp);
    return 0;
}

S32 udpate_common_Flash_WriteMtd(int iMtdBlockNum, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize)
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
    LOGW("mtd size:[%d] erasesize:[%d]\n",mtd.size,mtd.erasesize);
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

