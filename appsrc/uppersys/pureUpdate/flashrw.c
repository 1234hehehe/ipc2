#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
#include <getopt.h>
#endif
#include "flashrw.h"
#include "libcommon_api.h"

static int dev_fd = -1;
static int status = NoWriteNow;
static unsigned int blocks = 1, eraseblock = 0;
static unsigned int totalsize = 1, writesize = 0;
static unsigned int totalverifysize = 1, verifysize = 0;
static int g_bNandFlash = 0;

int Ovfs_update_Flash_WriteMtd_ByName(char *pMtdName, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize)
{
    int  iMtdBlockNum =-1;
	char cmd[128];
	S8   buf[64];
	
	if (pMtdName == NULL)
	{
        return -1;
	}

	snprintf(cmd,sizeof(cmd),"cat /proc/mtd | grep \"%s\" | awk \'{print $1}\'",pMtdName);

    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        if (0 == Common_StrnCmp(buf,"mtd",strlen("mtd")))
        {
            iMtdBlockNum = atoi(buf+strlen("mtd"));
		}
	}

	return Ovfs_update_Flash_WriteMtd(iMtdBlockNum,pImageBuffer,iWriteSize,iStartPos,iEraseSize);
}

int Ovfs_update_Flash_WriteFileMtd_ByName(char *pMtdName, FILE *UpFile, int iWriteSize, int iStartPos, int iEraseSize)
{
    int iMtdBlockNum =-1;
	char cmd[128];
	S8   buf[64];
	
	if (pMtdName == NULL)
	{
        return -1;
	}

	snprintf(cmd,sizeof(cmd),"cat /proc/mtd | grep \"%s\" | awk \'{print $1}\'",pMtdName);

    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        if (0 == Common_StrnCmp(buf,"mtd",strlen("mtd")))
        {
            iMtdBlockNum = atoi(buf+strlen("mtd"));
		}
	}
	
	return Ovfs_update_Flash_WriteFileMtd(iMtdBlockNum,UpFile,iWriteSize,iStartPos,iEraseSize);
}

int Ovfs_update_Flash_WriteMtd(int iMtdBlockNum, char *pImageBuffer, int iWriteSize, int iStartPos, int iEraseSize)
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
    //printf("[%s.%d] here  bn = %d,iwritesize = %d,istartpos = %d,ierasesize = %d\n",__FUNCTION__,__LINE__,iMtdBlockNum,iWriteSize,iStartPos,iEraseSize);
	if(iMtdBlockNum < 0 || iMtdBlockNum >= MAX_MTDBLOCK)
	{
		LOGE("Param err : iMtdBlockNum = %d\n", iMtdBlockNum);
		return -1;
	}

    if(dev_fd > 0)
	{
		LOGE("Writing now!\n");
		return -1;
	}
	g_bNandFlash = 0;
	status = StartNow;
    totalsize = iWriteSize;

	sprintf(device, "/dev/mtd%d", iMtdBlockNum);
	dev_fd = open(device, O_SYNC | O_RDWR);
	if(dev_fd < 0)
	{
		LOGE("Device open err : device = %s, ret = %d.\n", device, dev_fd);
		status = NoWriteNow;
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
		status = NoWriteNow;
		return -1;
	}

	if(MTD_NANDFLASH == mtd.type)
	{
		g_bNandFlash = 1;
	}
	else if(mtd.type != MTD_NORFLASH)
	{
		LOGE("This doesn't seem to be a valid MTD flash device :%s type = %d ret = %d\n", device,mtd.type, ret);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		status = NoWriteNow;
		return -1;
	}

    //不支持跨分区
	if(iWriteSize > mtd.size)
	{
		LOGE("Size %d won't fit into %s : device size = %d\n", iWriteSize, device, mtd.size);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		status = NoWriteNow;
		return -1;
	}

    //数据起始是block整数倍
    if(iStartPos % mtd.erasesize != 0)
	{
		LOGE("Start pos %d won't fit into %s : device size = %d\n", iStartPos, device, mtd.size);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		status = NoWriteNow;
		return -1;
	}

    //擦除以block为单位
    if(iEraseSize % mtd.erasesize != 0)
	{
		LOGE("Erase pos %d won't fit into %s : device size = %d\n", iStartPos, device, mtd.size);
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		status = NoWriteNow;
		return -1;
	}

    /******************** erase ********************/
	eraseblock = 0;
	status = EraseNow;
	erase.start = iStartPos;

    //擦除区域大小
	if(0 == iEraseSize)
    {
		erase.length = mtd.size - iStartPos;
    }
	else
    {
    	erase.length = iEraseSize;
    }

	blocks = erase.length / mtd.erasesize;
	erase.length = mtd.erasesize;

    for(i = 1; i <= blocks; i++)
	{
		bNeedErase = 1;
	 	if(mtd.type == MTD_NANDFLASH)
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
				status = NoWriteNow;
				return -1;
			}

			if (ret == 1) 
			{
				LOGE("Bad block at %x block(s) from %x will be skipped\n", i, (int)offs);
				bNeedErase = 0;
			}
	 	}

        if(bNeedErase && ioctl(dev_fd, MEMERASE, &erase) < 0)
		{
			LOGE("While erasing blocks 0x%.8x-0x%.8x on %s: %m\n", (unsigned int)erase.start, 
                                                                   (unsigned int)(erase.start + erase.length), 
                                                                   device);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}

            status = NoWriteNow;
			return -1;
		}

        erase.start += mtd.erasesize;
		eraseblock = i;
		nSleepCnt += mtd.erasesize;
		if(nSleepCnt > 512 * 1024)
		{
			nSleepCnt =0;
			usleep(10000);
		}
	}

    writesize = 0;
	//totalsize = iWriteSize;
	status = WriteNow;

	if(pImageBuffer == NULL && iWriteSize == 0)
	{
		LOGE("No data to Write.\n");
		if(dev_fd > 0)
		{
			close(dev_fd);
			dev_fd = -1;
		}
		status = NoWriteNow;
		
		return 0;
	}

    nWriteBlockSize = mtd.writesize;
	if(!g_bNandFlash)
	{
		nWriteBlockSize = 1024 * 1024 /mtd.writesize * mtd.writesize; 
	}

    dest = (char *)malloc(nWriteBlockSize);
	if(dest == NULL)
	{
		close(dev_fd);
		dev_fd = -1;
		status = NoWriteNow;
		return -1;
	}

	/******************** write ********************/
	i = mtd.erasesize;
	src  = pImageBuffer;
	size = iWriteSize;
	//totalsize = iWriteSize;
	if(iStartPos != 0)
	{
		ret = lseek(dev_fd, iStartPos, SEEK_SET);
		if(ret < 0)
		{
			LOGE("While seeking to start of %s: %d\n", device, ret);
			if(dev_fd > 0)
			{
				close(dev_fd);
				dev_fd = -1;
			}
			status = NoWriteNow;
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
			status = NoWriteNow;
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
		if(mtd.type == MTD_NANDFLASH)
	 	{
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
				status = NoWriteNow;
				return -1;
			}
			if (ret == 1) 
			{
				LOGE("Bad block from %x will be skipped.\n", (int)offs);
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
			status = NoWriteNow;
			return -1;
		}

		ret = write(dev_fd, (nRealSize != nWriteBlockSize)?dest:src, nWriteBlockSize);
		if(nWriteBlockSize != ret)
		{
			if(ret < 0)
			{
				LOGE("While writing data to 0x%08x-0x%08x on %s\n", writesize, writesize + nWriteBlockSize, device);
			}
			else
			{
				LOGE("Short write count returned while writing to x%08x-0x%08x on %s: %d/%d bytes written to flash\n", 
                                            writesize, writesize + nWriteBlockSize, device, writesize + ret, iWriteSize);
			}

            // set bad block for nand flash
			if(mtd.type == MTD_NANDFLASH)
	 	    {
	 	    	loff_t bad_addr = seek;
				LOGE("Marking block at %08lx bad\n", (long)bad_addr);

				if(iMtdBlockNum == 0 && bad_addr == 0)
				{
					if(dev_fd > 0)
					{
						close(dev_fd);
						dev_fd = -1;
					}
					Common_Free(dest,__FUNCTION__,__LINE__);
						dest = NULL;
					status = NoWriteNow;
					return -1;
				}

                if (ioctl(dev_fd, MEMSETBADBLOCK, &bad_addr)) 
                {
					LOGE("MEMSETBADBLOCK");
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

                status = NoWriteNow;
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

    verifysize = 0;
	totalverifysize = iWriteSize;
	status = VerifyNow;
	if(dev_fd > 0)
	{
		close(dev_fd);
		dev_fd = -1;
	}

    status = NoWriteNow;
	Common_Free(dest,__FUNCTION__,__LINE__);
	dest = NULL;

    //数据清零
    eraseblock = 0;
    writesize  = 0;
	return 0;
}

static unsigned int next_good_eraseblock(int fd, struct mtd_info_user *meminfo,
		unsigned int block_offset)
{
	if (meminfo->type == MTD_NANDFLASH)
	{
		while (1) {
			loff_t offs;

			if (block_offset >= meminfo->size) {
				LOGE("Not enough space in MTD device");
				return block_offset; /* let the caller exit */
			}
			offs = block_offset;
			if (ioctl(fd, MEMGETBADBLOCK, &offs) == 0)
				return block_offset;

            /* ioctl returned 1 => "bad block" */
			LOGD("Skipping bad block at 0x%08x\n", block_offset);
			block_offset += meminfo->erasesize;
		}
	}
	else
	{
		return block_offset;
	}
}

int Ovfs_update_Flash_WriteFileMtd(int iMtdBlockNum, FILE *UpFile, int iWriteSize, int iStartPos, int iEraseSize)
{
	int i,nRealSize,nWriteBlockSize;
	int ret = 0; // 过程控制变量,0-前述步骤正常完成,负数-前述步骤出错(后续步骤不应进行),正数-某个步骤出现属于正常情况的中断(后续步骤不继续).
	int size;
	char device[32];
 	char *dest = NULL;//[BUFSIZE];
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	int nSleepCnt = 0,bNeedErase = 1;

	off_t seek;
	int bRead = 0;

	if (0 == ret)
	{
		if(iMtdBlockNum < 0 || iMtdBlockNum >= MAX_MTDBLOCK)
		{
			LOGE("Param err : iMtdBlockNum = %d\n", iMtdBlockNum);
			ret = -1;
		}
	}
	
	if (0 == ret)
	{
    	if(dev_fd > 0)
    	{
    		LOGE("Writing busy now!\n");
    		ret = 1;
    	}
	}
    
	g_bNandFlash = 0;
	status = StartNow;
    totalsize = iWriteSize;

    if (0 == ret)
    {
    	sprintf(device, "/dev/mtd%d", iMtdBlockNum);
    	dev_fd = open(device, O_SYNC | O_RDWR);
    	if(dev_fd < 0)
    	{
    		LOGE("Device open err : device = %s, ret = %d.\n", device, dev_fd);
    		status = NoWriteNow;
    		ret = -1;
    	}
    }

    if (0 == ret)
    {
    	if(ioctl(dev_fd, MEMGETINFO, &mtd) < 0)
    	{
    		LOGE("This doesn't seem to be a valid MTD flash device :%s ret = %d.\n", device, ret);
    		status = NoWriteNow;
    		ret = -1;
    	}
    }

    if (0 == ret)
    {
		if(mtd.type == MTD_NANDFLASH)
		{
			g_bNandFlash = 1;
		}
		else if(mtd.type != MTD_NORFLASH)
		{
			LOGE("This doesn't seem to be a valid MTD flash device :%s type = %d ret = %d\n", device,mtd.type, ret);
			status = NoWriteNow;
			ret = -1;
		}
    }

    if (0 == ret)
    {
    	if(iWriteSize > mtd.size)
    	{
    		LOGE("Size %d won't fit into %s : device size = %d\n", iWriteSize, device, mtd.size);
    		status = NoWriteNow;
    		ret = -1;
    	}
    }
    
    if (0 == ret)
    {
    	if(iStartPos % mtd.erasesize != 0)
    	{
    		LOGE("Start pos %d won't fit into %s : device size = %d\n", iStartPos, device, mtd.size);
    		status = NoWriteNow;
    		ret = -1;
    	}
    }

    if (0 == ret)
    {
    	if(iEraseSize % mtd.erasesize != 0)
    	{
    		LOGE("Erase pos %d won't fit into %s : device size = %d\n", iStartPos, device, mtd.size);
    		status = NoWriteNow;
    		ret = -1;
    	}
    }
    
	/******************** erase ********************/
    if (0 == ret)
    {
    	eraseblock = 0;
    	status = EraseNow;
    	erase.start = iStartPos;
    	if(iEraseSize == 0)
	    {
    		erase.length = mtd.size - iStartPos;
        }
    	else
	    {
    		erase.length = iEraseSize;
	    }

        blocks = erase.length / mtd.erasesize;
    	erase.length = mtd.erasesize;
    	//LOGD("mtd.erasesize = %d \n", mtd.erasesize);
    	//LOGD("here erase pos = %d len = %d\n", erase.start, erase.length);

        for(i = 0; i < blocks && erase.start < mtd.size; i++)
    	{
    		//LOGD("erase block = %d\n", i);
    		bNeedErase = 1;

            // check nand bad block
    		erase.start = next_good_eraseblock(dev_fd, &mtd, erase.start);
            if (erase.start >= mtd.size)
            {
                break;
            }
    		else if(bNeedErase && ioctl(dev_fd, MEMERASE, &erase) < 0)
    		{
    			//printf("While erasing blocks 0x%08x-0x%08x on %s\n", (unsigned int)erase.start, (unsigned int)(erase.start + erase.length), device);
                LOGE("Erase [%s][0x%x] failed. errno=%d(%s)\n", device, erase.start, errno, strerror(errno));
    		}

            erase.start += mtd.erasesize;
    		eraseblock = i+1;
    		nSleepCnt += mtd.erasesize;
    		if(nSleepCnt > 512 * 1024)
    		{
    			nSleepCnt =0;
    			usleep(10000);
    		}
    	}
    	writesize = 0;
    	//totalsize = iWriteSize;
    	status = WriteNow;
    	nWriteBlockSize = mtd.writesize;
    	if(!g_bNandFlash)
    	{
    		nWriteBlockSize = 1024 * 1024 /mtd.writesize * mtd.writesize; 
    	}

    	if(NULL == UpFile && 0 == iWriteSize)
    	{
    		LOGE("No data to Write\n");
    		if(dev_fd > 0)
    		{
    			close(dev_fd);
    			dev_fd = -1;
    		}

            status = NoWriteNow;
    		ret = 1;
    	}
    }

	//LOGD("mtd.erasesize = %d \n", mtd.erasesize);

    if (0 == ret)
    {
    	if((dest = (char *)malloc(nWriteBlockSize)) == NULL)
    	{
    		status = NoWriteNow;
    		return -1;
    	}
    }

	/******************** write ********************/
    if (0 == ret)
    {
    	size = iWriteSize;
    	//totalsize = iWriteSize;
    	seek = iStartPos;
    	bRead = 1;
    	//LOGD("write size = %d\n", size);

        while(size > 0)
    	{
            // 请注意:next_good_eraseblock的固有缺陷,坏块前的一个writesize区域,会被识别为坏块,因此传入地址应该erasesize对齐.
    		unsigned int blockBasePos = seek / mtd.erasesize * mtd.erasesize;
    		unsigned int newPos = next_good_eraseblock(dev_fd, &mtd, blockBasePos);
            if (newPos != blockBasePos)
            {
                LOGW("skip %x to %x\n", blockBasePos, newPos);
                seek = seek - blockBasePos + newPos;
            }
    		
    	    if(seek >= mtd.size)
    		{
    			status = NoWriteNow;
    			ret = -1;
                break;
    		}
    		
    	    nRealSize = nWriteBlockSize;
    		
    		if(lseek(dev_fd, seek, SEEK_SET) != seek)
    		{
    			status = NoWriteNow;
    			ret = -1;
                break;
    		}

            //printf("[%s.%d] here\n",__FUNCTION__,__LINE__);
    		// for循环,完成一个eraseblock的写入.
    		int dataCounter;
            for (dataCounter = 0; dataCounter < mtd.erasesize && size > 0; dataCounter += nWriteBlockSize)
            {
        		if(bRead)
        		{
        			if (size < nWriteBlockSize)
        			{
        				memset(dest, 0xff, nWriteBlockSize);
        				nRealSize = size;
        				nWriteBlockSize = (nRealSize + mtd.writesize - 1)/ mtd.writesize * mtd.writesize;
        			}
        			//printf("[%s.%d] read nRealSize = %d\n",__FUNCTION__,__LINE__,nRealSize);
        			if(fread(dest, 1, nRealSize, UpFile) != nRealSize)
        			{
        				LOGE("ReadFile err. ReadPos=%x WritePos=%x\n", ftell(UpFile), lseek(dev_fd, 0, SEEK_CUR));
        				ret = -1;
                        break;
        			}
        		}

                //printf("[%s.%d] here\n",__FUNCTION__,__LINE__);
        		if(write(dev_fd, dest, nWriteBlockSize) <= 0)
        		{
        			printf("Failed to write to 0x%08x-0x%08x on %s\n", writesize, writesize + nWriteBlockSize, device);
        			status = NoWriteNow;
        			ret = -1;
                    break;
        		}

                //printf("[%s.%d] here\n",__FUNCTION__,__LINE__);
                bRead = 1;
        		seek += nWriteBlockSize;
        		writesize += nWriteBlockSize;
        		size -= nWriteBlockSize;
        		nSleepCnt += nWriteBlockSize;
        		if(nSleepCnt > 512 * 1024)
        		{
        			nSleepCnt =0;
        			usleep(10000);
        		}
            }

            if (ret < 0)
            {
                break;
            }
    		//LOGD("here size = %d writesize = %d\n", size, writesize);
    	}
    }

    if (dest)
    {
        Common_Free(dest,__FUNCTION__,__LINE__);
        dest = NULL;
    }
    
    if (ret <= 0)
    {
    	if(dev_fd > 0)
    	{
    		close(dev_fd);
    		dev_fd = -1;
    	}
    }

    //数据清零
    eraseblock = 0;
    writesize  = 0;
	return 0;
}

int Ovfs_update_Flash_GetStatus(int *pStatus, int *pRatio)
{
	int nRatio;
	*pStatus = status;

    if(g_bNandFlash)
	{
		if(status == NoWriteNow)
		{
			*pRatio = 100;
		}
		else
		{
			nRatio = (writesize * 100) / totalsize;
			if(nRatio < 30)
			{
				*pStatus = EraseNow;
				*pRatio = nRatio * 100 / 30;
			}
			else if(nRatio < 100)
			{
				*pStatus = WriteNow;
				*pRatio = (nRatio - 30) * 100 / 70;
			}
			else 
			{
				*pStatus = VerifyNow;
				*pRatio = 0;
			}
		}
		return 0;
	}

    if(status == NoWriteNow)
	{
		*pRatio = 100;
	}
	else if(status == EraseNow)
	{
		if(blocks == 0)
		{
			LOGE("Erase err : blocks is null\n");
			return -1;
		}

        *pRatio = (eraseblock * 100) / blocks;
	}
	else if(status == WriteNow)
	{
		if(totalsize == 0)
		{
			LOGE("Write err : totalsize is null\n");
			return -1;
		}

        *pRatio = (writesize * 100) / totalsize;
	}
	else if(status == VerifyNow)
	{
		*pRatio = 0;
	}
	else
	{
		*pRatio = 0;
	}

	return 0;
}

unsigned int Ovfs_update_Flash_GetEWStatus()
{
    /**
     * (1)假设erase大小为totalsize;
     * (2)完成擦除相当于只完成一半的升级操作;
     * (3)分区升级完成即eraseblock==blocks且writesize==totalsize.
     */
    unsigned long long tmp  = (unsigned long long)eraseblock * totalsize;
    unsigned long long tmp1 = (unsigned long long)writesize  * totalsize;

    unsigned int size = (unsigned int)((tmp / blocks + tmp1 / totalsize) / 2);

    LOGD("eraseblock=%d, blocks=%d, writesize=%d, totalsize=%d, size=%d.\n", eraseblock, 
                                                                             blocks, 
                                                                             writesize, 
                                                                             totalsize,
                                                                             size);
    return size;
}

