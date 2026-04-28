#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
#include <getopt.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "antsmid_md5.h"
#include "update_rpu.h"
#include "update_serverMgr.h"
#include "update_encry.h"
#include "update_common.h"

#define CONFIG_WRITEFILE 0

#ifndef MIN2
#define MIN2(a, b) ((a) < (b) ? (a) : (b))
#endif

enum {
    CHECK_AUTHEN_UNKNOWN = 0, 
    CHECK_AUTHEN_NORMAL  = 1, 
    CHECK_AUTHEN_DEMO    = 2, 
    CHECK_AUTHEN_MISS    = 3, 
    CHECK_AUTHEN_WARNING = 4
};

struct tag_RpuUpgradeStatus_T{
        int lStatus; //升级状态
        unsigned int ulUpgradeProgress; //升级进度
}stRpuUpgradeStatus = {UPDATE_STATUS_E_IDLE, 0};

static ProgramStatus_T s_programStatus = {0};
static const char *s_statusPreparing   = "Preparing";
static const char *s_statusErasing     = "Erasing";
static const char *s_statusWriting     = "Writing";
static const char *s_statusExtracting  = "Extracting";

static void setUpgradeStatus(int lStatus, int lProgress)
{
    stRpuUpgradeStatus.lStatus = lStatus;
    stRpuUpgradeStatus.ulUpgradeProgress = (unsigned int)lProgress;
}

static int getUserZoneInfA(unsigned char *id16byte)
{
    static int checkAuthen = -1;
    static unsigned char id[32];
    int ret = 0;

    if (checkAuthen < 0)
    {
        ret = updateEncry_GetUserZoneInf(2, id, 16);
        if (ret == 0)
        {
            checkAuthen = 0;
        }
    }

    if (0 == ret && id16byte)
    {
        memcpy(id16byte, id, 16);
    }
    return ret;
}

// 1 - Normal ; 2 - demo ; 3 - miss
// id[16], sn[8]
int check_Authen(unsigned char *id, unsigned char *sn)
{
    static int s_checkAuthen = -1;
    static unsigned char s_snAuthen[8]  = {0};
    static unsigned char s_idAuthen[16] = {0};

    if (s_checkAuthen == -1)
    {
        // TODO:
        //Custom_Init();

		S8* pSerial = NULL;
        int *p = (int*)s_snAuthen;
        memset(s_idAuthen, 0, sizeof(s_idAuthen));
        memset(s_snAuthen, 0, sizeof(s_snAuthen));

        FILE *fs = fopen("/root/demo", "rb");
        if(fs != NULL)
        {
            s_checkAuthen = CHECK_AUTHEN_DEMO;
            fread(s_idAuthen, 1, sizeof(s_idAuthen), fs);
            fclose(fs);
        }
        else if(0 == getUserZoneInfA(s_idAuthen) && 0 == updateEncry_GetLot(s_snAuthen))
        {
            if ((0 == s_idAuthen[6] || 0 == s_idAuthen[7] || 0 == s_idAuthen[10] || 0 == s_idAuthen[11]) || 
                (0 == s_snAuthen[5] && 0 == s_snAuthen[6] && 0 == s_snAuthen[7]))
            {
                // 加密芯片存在但是有异常,随机产生序列号
                s_checkAuthen = CHECK_AUTHEN_WARNING;
                LOGW("Warning Id=0x%02x%02x-%02x%02x sn=%02x%02x%02x%02x-%02x%02x%02x%02x\n", 
                              s_idAuthen[6], s_idAuthen[7], s_idAuthen[10], s_idAuthen[11], 
                              s_snAuthen[0], s_snAuthen[1], s_snAuthen[2], s_snAuthen[3], 
                              s_snAuthen[4], s_snAuthen[5], s_snAuthen[6], s_snAuthen[7]);
            }
            else 
            {
                s_checkAuthen = CHECK_AUTHEN_NORMAL;
                LOGI("Normal Id=0x%02x%02x-%02x%02x sn=%02x%02x%02x%02x-%02x%02x%02x%02x\n", 
                             s_idAuthen[6], s_idAuthen[7], s_idAuthen[10], s_idAuthen[11], 
                             s_snAuthen[0], s_snAuthen[1], s_snAuthen[2], s_snAuthen[3], 
                             s_snAuthen[4], s_snAuthen[5], s_snAuthen[6], s_snAuthen[7]);
            }
        }
		else if(NULL != (pSerial = update_common_GetSerialNumber()))
		{
			LOGW("pSerial=%s\n", pSerial);
			S32 tmp[2];
            if (2 == sscanf(pSerial, "%02x%02x", &tmp[0], &tmp[1]) || 2 == sscanf(pSerial, "%02X%02X", &tmp[0], &tmp[1]))
            {
            	s_idAuthen[10] = tmp[0]&0xff;
            	s_idAuthen[11] = tmp[1]&0xff;
                s_checkAuthen = CHECK_AUTHEN_NORMAL;
            }
			else
			{
				s_checkAuthen = CHECK_AUTHEN_MISS;
			}
		}
        else
        {
            s_checkAuthen = CHECK_AUTHEN_MISS;
        }

        if (s_checkAuthen == CHECK_AUTHEN_WARNING || s_checkAuthen == CHECK_AUTHEN_MISS)
        {
            // TODO:
            LOGW("Unknown hardware.\n");
        }

        if (s_checkAuthen != CHECK_AUTHEN_NORMAL)
        {
            struct timeval timeP0;
            gettimeofday(&timeP0, NULL);

            FILE* fp = fopen("/usr/etc/auth/sn", "rb");
            if (NULL == fp)
            {
                p[0] = ((timeP0.tv_usec * timeP0.tv_sec) >> 2) & (~3);
                p[1] = 0x00aeacaa;
            }
            else
            {
                fread(s_snAuthen, 1, sizeof(s_snAuthen), fp);
                fclose(fp);
            }

            LOGI("Abnormal Id=0x%02x%02x-%02x%02x sn=%02x%02x%02x%02x-%02x%02x%02x%02x\n", 
                            s_idAuthen[6], s_idAuthen[7], s_idAuthen[10], s_idAuthen[11], 
                            s_snAuthen[0], s_snAuthen[1], s_snAuthen[2], s_snAuthen[3], 
                            s_snAuthen[4], s_snAuthen[5], s_snAuthen[6], s_snAuthen[7]);
        }

        if ((0 == s_idAuthen[10]) || (0 == s_idAuthen[11]))
        {
            s_checkAuthen = CHECK_AUTHEN_UNKNOWN;
            LOGW("Unknown authen id.\n");
        }
    }

    if (id)
    {
        memcpy(id, s_idAuthen, sizeof(s_idAuthen));
    }

    if (sn)
    {
        memcpy(sn, s_snAuthen, sizeof(s_snAuthen));
    }
    
    return s_checkAuthen;
}

const unsigned char *getAuthenId()
{
    static unsigned char id[16];
    check_Authen(id, NULL);
    return id;
}

static void PrintProgress()
{
    if (strlen(s_programStatus.status) > 0 && (s_programStatus.erasingSize > 0 || s_programStatus.writingSize > 0))
    {
        int progress = MIN2(100, ((s_programStatus.erasedSize + s_programStatus.writtenSize) * 1.0 / (s_programStatus.erasingSize + s_programStatus.writingSize))*100);

        setUpgradeStatus(UPDATE_STATUS_E_UPDATING, progress);
#if DEBUG
        LOGD("\rtotal: %%%d ((%x+%x)/(%x+%x)) status: %-20s", progress,
                                                              s_programStatus.erasedSize,
                                                              s_programStatus.writtenSize,
                                                              s_programStatus.erasingSize,
                                                              s_programStatus.writingSize,
                                                              s_programStatus.status);
        fflush(stdout);
#endif
    }
}

static int safe_read(FILE * fd, void *buf, size_t count)
{
    int n;

    do
    {
        //n = read(fd, buf, count);
        n = fread(buf, 1, count, fd);
        if (n < 0 && errno == EINTR)
        {
            usleep(1000);
            continue;
        }
        else
        {
            break;
        }
    } while (1);

    return n;
}

static int safe_write(int fd, const void *buf, int count)
{
    int n;

    do
    {
        n = write(fd, buf, count);
        if (n < 0 && errno == EINTR)
        {
            usleep(1000);
            continue;
        }
        else
        {
            break;
        }
    } while (1);

    return n;
}

//通过查询/dev/mtd%d返回Flash分区信息.
static int GetMtdFlashInfo(int *partCount, struct mtd_info_user **mtdinfo)
{
    static int count = 0;
    static struct mtd_info_user mtd[32] = {{0}};

    // Query mtd info
    if (0 == count)
    {
        int ret;
        unsigned int i;
        for (i = 0; i < sizeof(mtd)/sizeof(mtd[0]); i++)
        {
            char mtdDevice[16];
            snprintf(mtdDevice, sizeof(mtdDevice), "/dev/mtd%d", i);
            
            int devFd = -1;
            devFd = open(mtdDevice, O_SYNC | O_RDONLY);
            if(devFd < 0)
            {
                ret = errno;
                //printf("Open device [%s] failed. errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                break;
            }
            
            if (ioctl(devFd, MEMGETINFO, &mtd[count]) < 0)
            {
                ret = errno;
                LOGE("Open device [%s] failed. errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                ret = -1;
            }
            else
            {
                LOGD("mtd%d info: type=%d(%s) size=%d esize=%d wsize=%d osize=%d\n", count, mtd[count].type, 
                      mtd[count].type == MTD_NANDFLASH ? "NANDFLASH" : (mtd[count].type == MTD_NANDFLASH ? "MTD_NORFLASH" : "?"), 
                      mtd[count].size, mtd[count].erasesize, mtd[count].writesize, mtd[count].oobsize);
                if (MTD_NORFLASH == mtd[count].type || MTD_NANDFLASH == mtd[count].type)
                {
                    count++;
                }
            }

            if (devFd >= 0)
            {
                close(devFd);
                devFd = -1;
            }
        }
    }

    if (partCount)
    {
        *partCount = count;
    }

    if (mtdinfo)
    {
        *mtdinfo = mtd;
    }

    return count;
}

//通过flashPosition找到对应分区号,对应分区Flash地址以及分区信息.
static int GetMtdInfoByPos(int flashPosition, struct mtd_info_user *mtdinfo, int *mtdIndex, int *inOffset)
{
    int i;
    int ret    = 0;
    int count  = 0;
    int offset = 0;

    struct mtd_info_user * mtds = NULL;
    GetMtdFlashInfo(&count, &mtds);

    if (flashPosition < 0)
    {
        ret = -1;
    }

    if (0 == ret)
    {
        for (i = 0; i < count; i++)
        {
            if (offset <= flashPosition && flashPosition < offset + (int)mtds[i].size)
            {
                break;
            }

            offset += mtds[i].size;
        }

        if (i == count)
        {
            ret = -1;
        }
    }

    if (0 == ret)
    {
        if (mtdinfo)
        {
            memcpy(mtdinfo, &mtds[i], sizeof(mtds[i]));
        }

        if (mtdIndex)
        {
            *mtdIndex = i;
        }

        if (inOffset)
        {
            *inOffset = flashPosition - offset;
        }
    }
    
    return ret;
}

static unsigned int next_good_eraseblock(int fd, struct mtd_info_user *meminfo,
		unsigned int block_offset)
{
	if (meminfo->type == MTD_NANDFLASH)
	{
		while (1) {
			loff_t offs;

			if (block_offset >= meminfo->size) {
				printf("Not enough space in MTD device");
				return block_offset; /* let the caller exit */
			}
			offs = block_offset;
			if (ioctl(fd, MEMGETBADBLOCK, &offs) == 0)
				return block_offset;
			/* ioctl returned 1 => "bad block" */
			printf("Skipping bad block at 0x%08x\n", block_offset);
			block_offset += meminfo->erasesize;
		}
	}
	else
	{
		return block_offset;
	}
}

static void SetProgress(const char *status, unsigned int value)
{
    if (status && strlen(status) > 0)
    {
        do
        {
            if (0 == strcmp(status, s_statusPreparing))
            {
                //snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
                break;
            }

            if (0 == strcmp(status, s_statusErasing))
            {
                //snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
                s_programStatus.erasedSize += value;
                break;
            }

            if (0 == strcmp(status, s_statusWriting))
            {
                //snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
                s_programStatus.writtenSize += value;
                break;
            }

            if (0 == strcmp(status, s_statusExtracting))
            {
                //snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
                s_programStatus.writtenSize += value;
                break;
            }
            else
            {
                LOGI("Unknown status(%s).\n", status);
                return ;
            }
        }while (0);

        snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
    }
}

static int FlashErase(int position, int length)
{
    int ret   = 0;
    int devFd = -1;
    int handleLength = 0;

    struct mtd_info_user mtdinfo;
    int mtdIndex = 0;
    int inOffset = 0;
    if (position < 0 || length <= 0)
    {
        ret = -1;
    }

    while (0 == ret && handleLength < length)
    {
        if (0 == ret)
        {
            if (GetMtdInfoByPos(position + handleLength, &mtdinfo, &mtdIndex, &inOffset) < 0)
            {
                ret = -1;
                LOGE("Not find such mtd info at pos=0x%x\n", position);
            }
            else
            {
                // 这是干嘛?
                inOffset = inOffset / mtdinfo.erasesize * mtdinfo.erasesize;
            }
        }

        char mtdDevice[32];
        if (0 == ret)
        {
            snprintf(mtdDevice, sizeof(mtdDevice), "/dev/mtd%d", mtdIndex);
            if ((devFd = open(mtdDevice, O_SYNC | O_RDWR)) < 0)
            {
                ret = errno;
                LOGE("Open [%s] failed.errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                ret = -1;
            }
        }

        if (0 == ret)
        {
            // Erase blocks.
            int tmpCount = 0;
            int eraseStart  = inOffset;
            int eraseLength = MIN2((int)mtdinfo.size - inOffset, length - handleLength);
            eraseLength = (eraseLength + mtdinfo.erasesize - 1) / mtdinfo.erasesize * mtdinfo.erasesize;

            for (tmpCount = 0; tmpCount < eraseLength; tmpCount += mtdinfo.erasesize)
            {
                struct erase_info_user erase;
                erase.start  = eraseStart + tmpCount;
                erase.length = mtdinfo.erasesize;

                erase.start = next_good_eraseblock(devFd, &mtdinfo, erase.start);
#if (0 == CONFIG_WRITEFILE)
                if (ioctl(devFd, MEMERASE, &erase) < 0)
#endif
                {
                    ret = errno;
                    LOGE("Erase [%s][0x%x] failed. errno=%d(%s)\n", mtdDevice, erase.start, ret, strerror(ret));
                    ret = 0; // Keep going on if erase failed.
                }

                handleLength += erase.length;
                SetProgress("Erasing", erase.length);
                PrintProgress();
            }
        }

        if (devFd >= 0)
        {
            close(devFd);
            devFd = -1;
        }
    }

    return ret;
}

static int FlashCopy(int position, FILE *fd, int length)
{
    int ret = 0;
    int devFd = -1;
    int handleLength = 0;

    struct mtd_info_user mtdinfo;
    int mtdIndex = 0;
    int inOffset = 0; // internal offset of this mtd.
    int devFdSeek = 0;
    if (position < 0 || NULL == fd || length < 0)
    {
        ret = -1;
    }

    while (0 == ret && handleLength < length)
    {
        if (0 == ret)
        {
            if (GetMtdInfoByPos(position + handleLength, &mtdinfo, &mtdIndex, &inOffset) < 0)
            {
                ret = -1;
                LOGE("Not find such mtd info at pos=0x%x\n", position);
            }
            else
            {
                inOffset = inOffset / mtdinfo.erasesize * mtdinfo.erasesize;
            }
        }

        if (0 == ret)
        {
            char mtdDevice[32];
            snprintf(mtdDevice, sizeof(mtdDevice), "/dev/mtd%d", mtdIndex);
            if ((devFd = open(mtdDevice, O_SYNC | O_RDWR)) < 0)
            {
                ret = errno;
                LOGE("Open [%s] failed.errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                ret = -1;
            }
        }

        if (0 == ret)
        {
            // Copy data.
            int bufferLen = mtdinfo.erasesize;
            char *buffer = (char *)malloc(bufferLen);
            if (buffer == NULL)
            {
                ret = -1;
                LOGE("malloc failed size=0x%x.\n", bufferLen);
            }

            devFdSeek = inOffset;
            while (0 == ret && handleLength < length && devFdSeek < (int)mtdinfo.size)
            {
                unsigned int blockBasePos    = devFdSeek / mtdinfo.erasesize * mtdinfo.erasesize;
                unsigned int blockBasePosNew = next_good_eraseblock(devFd, &mtdinfo, blockBasePos);
                if (blockBasePosNew != blockBasePos)
                {
                    devFdSeek = devFdSeek - blockBasePos + blockBasePosNew;
                }

                if (devFdSeek >= mtdinfo.size)
                {
                    ret = -1;
                    LOGE("No space enough.\n");
                    break;
                }
                else if (lseek(devFd, devFdSeek, SEEK_SET) < 0)
                {
                    ret = -1;
                    LOGE("Seek failed. mtdIndex=%d. devFdSeek=0x%x.\n", mtdIndex, devFdSeek);
                    break;
                }

                int readLen = (int)mtdinfo.size - devFdSeek < length - handleLength ? (int)mtdinfo.size - devFdSeek : length - handleLength;
                readLen = readLen < bufferLen ? readLen : bufferLen;
                if ((readLen = safe_read(fd, buffer, readLen)) <= 0)
                {
                    ret = -1;
                    break;
                }

                if (0 == ret)
                {
					if (mtdinfo.type == MTD_NANDFLASH)
					{
						if (readLen < mtdinfo.erasesize)
						{
                            int elsePageSize = readLen % mtdinfo.writesize;
                            if (elsePageSize > 0)
                            {
							    memset(buffer + readLen, 0xff, mtdinfo.writesize - elsePageSize);
							    readLen = readLen - elsePageSize + mtdinfo.writesize;
                            }
						}
					}
#if (CONFIG_WRITEFILE==0)
                    if (safe_write(devFd, buffer, readLen) <= 0)
#else
					if (0)
#endif
                    {
                        int retA = errno;
                        ret = -1;
                        LOGE("Write failed(%d). mtdIndex=%d. devFdSeek=0x%x. writeLen=0x%x.\n", retA, mtdIndex, devFdSeek, readLen);
                        LOGE("%s\n", strerror(retA));
                        break;
                    }
                    else
                    {
                        handleLength += readLen;

                        //printf("Writing pos=%x len=%x\n", devFdSeek, readLen);
                        devFdSeek += readLen;
                        SetProgress("Writing", readLen);
                        PrintProgress();
                        if (devFdSeek >= (int)mtdinfo.size)
                        {
                            break;
                        }
                    }
                }
            }

            if (buffer)
            {
                free(buffer);
                buffer = NULL;
            }
        }

        if (devFd >= 0)
        {
            close(devFd);
            devFd = -1;
        }
    }

    return ret < 0 ? ret : handleLength;
}

static int UpdateFileToFlash(FILE *fd/*int fd*/, unsigned int writingSize, unsigned int flashAddr, unsigned int erasingSize)
{
    int ret = 0;

    if (ret >= 0)
    {
        if (erasingSize > 0)
        {
			LOGD("flashAddr = %d, erasingSize = %d\n", flashAddr, erasingSize);
            ret = FlashErase(flashAddr, erasingSize);
        }
    }

    if (ret >= 0)
    {
        if (writingSize > 0)
        {
			LOGD("flashAddr = %d, writingSize = %d\n", flashAddr, writingSize);
            ret = FlashCopy(flashAddr, fd, writingSize);
        }
    }

    return ret < 0 ? ret : 0;
}

//文件头校验
static S32 checkRpuFileHeader(UpdateSvrMgr* pstUpdateSvrMgr, RawPartUpgradeFileHeader_T *pstRpuFileHeader)
{
    //int lRet = -1;
    char tmpChecksum[16] = {0};

    // 校验头部
    memcpy(tmpChecksum, pstRpuFileHeader->headChecksum, sizeof(pstRpuFileHeader->headChecksum));
    memset(pstRpuFileHeader->headChecksum, 0, sizeof(pstRpuFileHeader->headChecksum));

#if 1
    Common_Md5_Simply((unsigned char *)pstRpuFileHeader, pstRpuFileHeader->headSize, pstRpuFileHeader->headChecksum, NULL);
#else
    // Compute file MD5.
    ANTSMID_MD5_CTX md5;
    Antsmid_MD5Init(&md5);
    Antsmid_MD5Update(&md5, (unsigned char *)&rpuFileHeader, rpuFileHeader.headSize);
    Antsmid_MD5Final(rpuFileHeader.headChecksum, &md5);
#endif
    if (0 != memcmp(tmpChecksum, pstRpuFileHeader->headChecksum, sizeof(pstRpuFileHeader->headChecksum)))
    {
        LOGE("Header checksum is invalid.\n");
        return -1;
    }

    return 0;
}

//分段数据校验
static S32 checkRpuPartData(UpdateSvrMgr* pstUpdateSvrMgr, RawPartUpgradeFileHeader_T *pstRpuFileHeader)
{
#define BUFFER_LEN 65536    
    int lRet = 0;
    unsigned char tmpChecksum[16] = {0};
    char *tmpBuffer = NULL;
    int readlen = 0;
    int i;
    int offset;
    unsigned int j = 0;

    tmpBuffer = (char *)malloc(sizeof(char) * BUFFER_LEN);
    if(NULL == tmpBuffer)
    {
        LOGE("Malloc memory error.\n");
        return -1;
    }

    for (i = 0; i < pstRpuFileHeader->partCount; i++)
    {
        offset = fseek(pstUpdateSvrMgr->m_hUpgradeFile, pstRpuFileHeader->headSize + pstRpuFileHeader->partInfo[i].fileStartKB * 1024, SEEK_SET);
        if (offset == (int)(-1))
        {
            LOGE("Seek part [%d]=[%d] out of range.\n", i, pstRpuFileHeader->headSize + pstRpuFileHeader->partInfo[i].fileStartKB * 1024);
            lRet = -1;
            break;
        }

        //ANTSMID_MD5_CTX md5;
        //Antsmid_MD5Init(&md5);
        Common_Md5_T md5;
        Common_Md5_Create(&md5);

        for (j = 0; j < pstRpuFileHeader->partInfo[i].fileSizeKB * 1024; j += readlen)
        {
            readlen = pstRpuFileHeader->partInfo[i].fileSizeKB * 1024 - j;
            readlen = readlen < BUFFER_LEN ? readlen : BUFFER_LEN;

            memset((void *)tmpBuffer, 0x00, BUFFER_LEN);
            readlen = fread(tmpBuffer, 1, readlen, pstUpdateSvrMgr->m_hUpgradeFile);
            if (0 == readlen)
            {
                LOGE("Read data error.\n");
                Common_Md5_Destroy(&md5);

                free(tmpBuffer);
                tmpBuffer = NULL;
                return -1;
            }

            Common_Md5_Append(md5, (unsigned char*)tmpBuffer, readlen);
            //Antsmid_MD5Update(&md5, (unsigned char*)tmpbuffer, readlen);
        }

        //Antsmid_MD5Final(tmpChecksum, &md5);
        Common_Md5_Finish(md5, (unsigned char *)tmpChecksum, NULL);
        Common_Md5_Destroy(&md5);

        if (memcmp(tmpChecksum, pstRpuFileHeader->partInfo[i].partChecksum, sizeof(pstRpuFileHeader->partInfo[i].partChecksum)) != 0)
        {
            LOGE("Part[%d] checksum is invalid.\n", i);
            lRet = -1;
            break;
        }
    }

    free(tmpBuffer);
    tmpBuffer = NULL;
    return lRet;
}

//BoardI校验
static S32 checkBoardID(UpdateSvrMgr* pstUpdateSvrMgr, RawPartUpgradeFileHeader_T *pstRpuFileHeader)
{
    int i = 0;
    int lRet = 0;
    const unsigned char *id = getAuthenId();
    int *idtable = pstRpuFileHeader->boardid;
    int idcount  = sizeof(pstRpuFileHeader->boardid)/sizeof(pstRpuFileHeader->boardid[0]);

    for (i = 0; i < idcount; i++)
    {
        if (idtable[i] == (id[10] << 8 | id[11]))
        {
            break;
        }
    }

    if (i == idcount)
    {
        LOGE("Id[%04x] is not effective for upgrading. No need to upgrade.\n", (id[10]<<8 | id[11]));
        lRet = -1;
    }

    return lRet;
}

static void initProgress(RawPartUpgradeFileHeader_T *pstRpuFileHeader)
{
    int i;
    unsigned int erasingSize = 0;
    unsigned int writingSize = 0;

    for (i = 0; i < pstRpuFileHeader->partCount; i++)
    {
        erasingSize += pstRpuFileHeader->partInfo[i].partSizeKB*1024;
        writingSize += pstRpuFileHeader->partInfo[i].fileSizeKB*1024;
    }

    if (erasingSize > 0)
    {
        s_programStatus.erasingSize = erasingSize;
    }

    if (writingSize > 0)
    {
        s_programStatus.writingSize = writingSize;
    }

    stRpuUpgradeStatus.lStatus = UPDATE_STATUS_E_BEGIN;
    stRpuUpgradeStatus.ulUpgradeProgress = 0;
}

static S32 checkRpuUpgradeFile(UpdateSvrMgr* pstUpdateSvrMgr, RawPartUpgradeFileHeader_T* pstRpuFileHeader)
{
    int lRet = -1;
    RawPartUpgradeFileHeader_T rpuFileHeader;

    if ((NULL == pstUpdateSvrMgr) || (NULL == pstUpdateSvrMgr->m_hUpgradeFile))
    {
        LOGE("UpdateSvrMgr or UpdateFile handle is NULL.\n");
        return -1;
    }

    memset((void *)&rpuFileHeader, 0x00, sizeof(rpuFileHeader));
    lRet = fread(&rpuFileHeader, 1, sizeof(rpuFileHeader), pstUpdateSvrMgr->m_hUpgradeFile);
    if (lRet < sizeof(rpuFileHeader))
    {
        LOGE("Read the rpuFileHeader's datas SO SHORT.\n");
        return -1;
    }
    lRet = 0;

    //包头+Part段数据以及BoardID校验
    lRet = checkRpuFileHeader(pstUpdateSvrMgr, &rpuFileHeader);
    if (0 != lRet)
    {
        LOGE("Check RPUF header failed.\n");
        return -1;
    }

    lRet = checkRpuPartData(pstUpdateSvrMgr, &rpuFileHeader);
    if (0 != lRet)
    {
        LOGE("Check RPUF part data failed.\n");
        return -1;
    }

    lRet = checkBoardID(pstUpdateSvrMgr, &rpuFileHeader);
    if (0 != lRet)
    {
        LOGE("Check BOARD ID failed.\n");
        return -1;
    }

    //初始化升级进度信息
    //initProgress(&rpuFileHeader);

    memcpy(pstRpuFileHeader, &rpuFileHeader, sizeof(*pstRpuFileHeader));
    return 0;
}

int upgradeRpuFile(UpdateSvrMgr* pstUpdateSvrMgr, RawPartUpgradeFileHeader_T *fileHeader)
{
    int i;
    int lRet = -1;
    int readOffset = 0;

    if ((NULL == pstUpdateSvrMgr) || (NULL == pstUpdateSvrMgr->m_hUpgradeFile))
    {
        LOGE("UpdateSvrMgr or UpdateFile handle is NULL.\n");
        return -1;
    }

    LOGD("The upgrade file include %d parts.\n", fileHeader->partCount);
    for (i = 0; i < fileHeader->partCount; i++)
    {
        LOGD("Update part %dst:\n", i);
        readOffset = fileHeader->headSize + fileHeader->partInfo[i].fileStartKB * 1024;
        fseek(pstUpdateSvrMgr->m_hUpgradeFile, readOffset, SEEK_SET);

		Wtdg_Stop();
		LOGD("stop wtd\n", i);
        lRet = UpdateFileToFlash(pstUpdateSvrMgr->m_hUpgradeFile,
                                 fileHeader->partInfo[i].fileSizeKB  * 1024, 
                                 fileHeader->partInfo[i].partStartKB * 1024, 
                                 fileHeader->partInfo[i].partSizeKB  * 1024);
        if (lRet < 0)
        {
			LOGE("Update part %dst err\n", i);
            lRet = -1;
			Wtdg_Start();
			Wtdg_SetTime(120);
            break;
        }
		LOGD("Update part %dst OK\n", i);
		Common_Sleep(0, 1000000);
		LOGD("Sleep 1s restart wtd\n", i);
		Wtdg_Start();
		Wtdg_SetTime(120);
    }

    return lRet;
}

int updateRpu_Begin()
{
    setUpgradeStatus(UPDATE_STATUS_E_BEGIN, 0);
	return 0;
}

int updateRpu_Upgrade(UpdateSvrMgr* pstUpdateSvrMgr, S8 *pcFileName)
{
    int lRet = -1;
    RawPartUpgradeFileHeader_T stRpuFileHeader;

    pcFileName = pcFileName;

    setUpgradeStatus(UPDATE_STATUS_E_BEGIN, 0);

    // 检验升级文件
    memset((void *)&stRpuFileHeader, 0x00, sizeof(stRpuFileHeader));    
    lRet = checkRpuUpgradeFile(pstUpdateSvrMgr, &stRpuFileHeader);
    if (0 != lRet)
    {
        // 升级失败,升级进度为0.
        setUpgradeStatus(UPDATE_STATUS_E_FAILED, 0);

        LOGE("Check rpu upgrade file failed.\n");
        return -1;
    }

    //初始化升级进度信息
    initProgress(&stRpuFileHeader);

    //卸载分区
    (void)udpate_common_UmountPartition();

    // 升级镜像
    lRet = upgradeRpuFile(pstUpdateSvrMgr, &stRpuFileHeader);
    if (0 != lRet)
    {
        // 升级失败,升级进度为0.
        setUpgradeStatus(UPDATE_STATUS_E_FAILED, 0);

        LOGE("Upgrade Rpu file failed.\n");
        return -1;
    }

    // 升级完成,升级进度为100%.
    setUpgradeStatus(UPDATE_STATUS_E_FINISH, 100);
    return 0;
}

int updateRpu_GetUpgradeStatus(int *plStatus, int *plUpgradeProgress)
{
    if ((NULL == plStatus) || (NULL == plUpgradeProgress))
    {
        LOGE("Input parameters BUFFER IS NULL.\n");
        return -1;
    }

    *plStatus = stRpuUpgradeStatus.lStatus;
    *plUpgradeProgress = (int)stRpuUpgradeStatus.ulUpgradeProgress;
    return 0;
}

