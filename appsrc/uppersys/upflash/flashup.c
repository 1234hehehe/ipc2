#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
#include <getopt.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#define PRINT_DBG(x...) printf("[%s:%d] ", __PRETTY_FUNCTION__, __LINE__); printf(x)

#define CONFIG_WRITEFILE 0

typedef unsigned long int ANTSMID_UINT4;
/* MD5 context. */
typedef struct {
    ANTSMID_UINT4 state[4];                                   /* state (ABCD) */
    ANTSMID_UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];                         /* input buffer */
} ANTSMID_MD5_CTX;
#ifdef __cplusplus
extern "C"{
#endif
void Antsmid_MD5Init (ANTSMID_MD5_CTX *);
void Antsmid_MD5Update(ANTSMID_MD5_CTX *, unsigned char *, unsigned int);
void Antsmid_MD5Final (unsigned char [16], ANTSMID_MD5_CTX *);
#ifdef __cplusplus
}
#endif

#ifndef MIN2
#define MIN2(a,b) ((a) < (b) ? (a) : (b))
#endif


#define UPDATEFILEHEADERMAGIC 0x41555046  //"AUPF"
#define UPDATEPACKAGEHEADERMAGIC 0x5350502A // "SPP*"
#define ARPUHEADERMAGIC ('A' | 'R'<<8 | 'P'<<16 | 'U'<<24) //0x41525055  //"A"R"P"U"

#define VERSION_MAJOR 1

typedef enum _tagUpdateType
{
    UpdateType_Nothing = 0,
    UpdateType_Uboot,
    UpdateType_Kernel,
    UpdateType_Rfs,
    UpdateType_App,
    UpdateType_Config,
    UpdateType_Logo,
    UpdateType_ChLogo,
    UpdateType_File,
    UpdateType_Tar,
	UpdateType_FileEx = 10, // 文件操作,删除/
	UpdateType_PartName = 20, //  通过分区名升级方式
	UpdateType_PartIndex , //  通过分区索引升级方式,0- 无效分区,1-第一分区,2-第二分区
	UpdateType_Flash, // 升级整个flash
    UpdateType_Butt,
}UpdateType_T;

typedef struct _tagUpdateFileHeader
{
    unsigned int uMagicNumber;// "AUPF" -ants update package file
    unsigned int uPackageTypeMark;// package type bits mark
    unsigned int uVersion;// 0- ignore
    unsigned int uRequireVersion;// 0- ignore
    unsigned int uRequireClearConfig;
    unsigned int uSupportBoardNum;// 0- do nothing
    unsigned int uSupportBoardTypesTable_Offset;// 1-N array,,from file start,include file header:0- do nothing
    unsigned int uPackageNum; // [1-32]
    unsigned int uPackageOffsetsTable_Offset;// 1- 32 array ,from file start,include file header
}UpdateFileHeader_T;

typedef struct _tagUpdatePackageHeader
{
    unsigned int uMagicNumber;// "SPP*" single part package 
    unsigned int uPackageType;
    unsigned int uVersion;// 0- ignore
    unsigned int uRequireVersion;// 0- ignore
    unsigned int uPackageLength;// not include header
    unsigned int uDependPackage;// package type bits mark
    unsigned int uPackageCheckType;// 0- MD5
    unsigned int uPackageCheckValue[4];
	unsigned int uExtPackHeaderLengh; // 4对齐
	union{
		unsigned int uRes[2];
		unsigned int uPartIndx;
	};
}UpdatePackageHeader_T;

#define MAX_MERGE_BOARDID_COUNT 16
#define MAX_MERGE_PART_COUNT 16
typedef struct
{
unsigned int fileStartKB;
unsigned int fileSizeKB;
unsigned int partStartKB;
unsigned int partSizeKB;
char partChecksum[16];
} RawPartUpgradeFilePart_T;
typedef struct RawPartUpgradeFileHeader_T
{
    unsigned int uMagicNumber;// "ARPU"-ants raw partition update.
    unsigned int headSize; // header长度,包括magic number在内.
    unsigned char headChecksum[16]; // 计算校验值
    union
    {
      struct
      {
        char versionMajor;
        char versionMinor;
        char versionRev;
        char versionBuild;
      } verPart;
      int verValue;
    };
    int boardid[MAX_MERGE_BOARDID_COUNT];

    char res2[3];
    char partCount;
    RawPartUpgradeFilePart_T partInfo[MAX_MERGE_PART_COUNT];

    char res3[416];
} RawPartUpgradeFileHeader_T;

typedef struct 
{
    unsigned int erasingSize; // 需要擦除的大小
    unsigned int erasedSize; // 已经擦除的大小
    unsigned int writingSize; // 需要写入的大小
    unsigned int writtenSize; // 已经写入的大小
    unsigned int curPosition; // 当前操作Flash地址
    char status[16]; // 当前操作状态
} ProgramStatus_T;

static ProgramStatus_T s_programStatus = {0};
static const char *s_statusPreparing = "Preparing";
static const char *s_statusErasing = "Erasing";
static const char *s_statusWriting = "Writing";
static const char *s_statusExtracting = "Extracting";


static char s_logFile[256] = "/dev/tmp.log";
static FILE* s_logFileFp = NULL;

extern int GetUserZoneInf(unsigned char ucUZId, unsigned char *ucpData, unsigned char ucDataCount);
extern int GetLot(unsigned char *ucpData);

//extern FILE *mypopen(const char *cmdstring, const char *type);
//extern int mypclose(FILE *fp);

static int safe_read(int fd, void *buf, size_t count)
{
    int n;

    do
    {
        n = read(fd, buf, count);
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

void PrintBuffer(void *buffer, int len)
{
    unsigned char* p = (unsigned char*)buffer;
    int i;
    
    printf("[yul][%s:%d] buffer=%p len=0x%x\n", __FUNCTION__, __LINE__, buffer, len);
    for (i = 0; i < len; i += 16)
    {
        int j;
        printf("0x%04x:", i);
        for (j = 0; j < 16 && i + j < len; j++)
        {
            printf(" %02x", p[i + j]);
        }
        if (j < 16)
        {
            for ( ; j < 16; j++)
            {
                printf("   ");
            }
        }
        printf(" | ");
        for (j = 0; j < 16; j++)
        {
            char c = p[i + j];
            printf("%c", c >= 0x20 && c <= 0x7f ? c : '.');
        }
        printf("\n");
    }
    printf("\n");
}



char g_sensorModel[16] = "";
char g_platform[16] = "HI3518A";
char g_customer[32] = "";
char g_hardware[64] = "FSAN_HI3518A_D1306";
char g_isofdome[4] = "n";
char g_isofir[4] = "y";

const char g_hardware_AEVISION_HI3518C_D1405_1XX1[] = "AEVISION_HI3518C_D1405_1XX1";
const char g_hardware_AEVISION_HI3518C_D1405_1XX18M[] = "AEVISION_HI3518C_D1405_1XX18M";
const char g_hardware_AEVISION_HI3518C_D1406_JUAN8M[] = "AEVISION_HI3518C_D1406_JUAN8M";
const char g_hardware_AEVISION_HI3516C_D1405_2XX1[] = "AEVISION_HI3516C_D1405_2XX1";
const char g_hardware_AEVISION_HI3516C_D1405_2XX18M[] = "AEVISION_HI3516C_D1405_2XX18M";
const char g_hardware_AEVISION_HI3516C_D1411_2XX1[] = "AEVISION_HI3516C_D1411_2XX1";
const char g_hardware_AEVISION_HI3516C_D1411_2XX18M[] = "AEVISION_HI3516C_D1411_2XX18M";
const char g_hardware_AEVISION_HI3518E_D1408[] = "AEVISION_HI3518E_D1408";

const char g_hardware_FSAN_HI3518C_D1402_JUAN[] = "FSAN_HI3518C_D1402_JUAN";
const char g_sensorModel_OV9712P[] = "OV9712P";

static void Custom_Init()
{
    static char s_inited = 0;
    
    if (s_inited == 0)
    {
        s_inited = 1;
        
        const char* filename = "/root/res_xml/factoryInfo.xml";
        FILE* fp = fopen(filename, "r");
        
        if (NULL != fp)
        {
            //Line this: <SensorModel content="IMX122"/>
            char buffer[256] = "";
            char* p = NULL;
            unsigned char findcount = 0;
            struct 
            {
                const char* labelstr;
                char* resultstr;
                char found;
                unsigned char resultsize;
            } findlabel[] =
            {
                {"SensorModel content=\"", g_sensorModel, 0, sizeof(g_sensorModel)},
                {"Customer content=\"", g_customer, 0, sizeof(g_customer)},
                {"IsofDome content=\"", g_isofdome, 0, sizeof(g_isofdome)},
                {"IsofIr content=\"", g_isofir, 0, sizeof(g_isofir)},
                {"Hardware content=\"", g_hardware, 0, sizeof(g_hardware)},
                {"Platform content=\"", g_platform, 0, sizeof(g_platform)},
            };
            unsigned int i;
            while (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                for (i = 0; i < sizeof(findlabel)/sizeof(findlabel[0]); i++)
                {
                    if (0 == findlabel[i].found && (p = strstr(buffer, findlabel[i].labelstr)))
                    {
                        p += strlen(findlabel[i].labelstr);
                        char* q = strchr(p, '"');
                        if (NULL != q && (q > p))
                        {
                            //snprintf(result[i], q - p + 1, "%s", p);
                            snprintf(findlabel[i].resultstr, MIN2(q - p + 1, findlabel[i].resultsize), "%s", p);
                        }
                        findlabel[i].found = 1;
                        findcount++;
                        
                        if (findcount == sizeof(findlabel)/sizeof(findlabel[0]))
                        {
                            break;
                        }
                    }
                }
            }

            for (i = 0; i < sizeof(findlabel)/sizeof(findlabel[0]); i++)
            {
                 printf("[hal][%s:%d] %s%s\"\n", __FUNCTION__, __LINE__, findlabel[i].labelstr, findlabel[i].resultstr);
            }
        }
        
        if (fp)
        {
            fclose(fp);
            fp = NULL;
        }
    }
    
}

enum {CHECK_AUTHEN_UNKNOWN=0, CHECK_AUTHEN_NORMAL=1, CHECK_AUTHEN_DEMO=2, CHECK_AUTHEN_MISS=3, CHECK_AUTHEN_WARNING=4,};

static int GetUserZoneInfA(unsigned char *id16byte)
{
    static int checkAuthen = -1;
    static unsigned char id[32];
    int ret = 0;
    if (checkAuthen < 0)
    {
        ret = GetUserZoneInf(2, id, 16);
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
int Check_Authen(unsigned char *id, unsigned char *sn)
{
    static int s_checkAuthen = -1;
    static unsigned char s_idAuthen[16];
    static unsigned char s_snAuthen[8];
    
    if (s_checkAuthen == -1)
    {
        Custom_Init();
        
        int *p = (int*)s_snAuthen;

        memset(s_idAuthen, 0, sizeof(s_idAuthen));
        memset(s_snAuthen, 0, sizeof(s_snAuthen));
        
        FILE *fs = fopen("/root/demo", "rb");
        if(fs != NULL)
        {
            // Demo
            s_checkAuthen = CHECK_AUTHEN_DEMO;
            fread(s_idAuthen, 1, sizeof(s_idAuthen), fs);
            fclose(fs);
        }
        else if(0 == GetUserZoneInfA(s_idAuthen) && 0 == GetLot(s_snAuthen))
        {
            if ((0 == s_idAuthen[6] || 0 == s_idAuthen[7] || 0 == s_idAuthen[10] || 0 == s_idAuthen[11]) || 
                (0 == s_snAuthen[5] && 0 == s_snAuthen[6] && 0 == s_snAuthen[7]))
            {
                // 加密芯片存在但是有异常,随机产生序列号
                s_checkAuthen = CHECK_AUTHEN_WARNING;
                PRINT_DBG("Warning Id=0x%02x%02x-%02x%02x sn=%02x%02x%02x%02x-%02x%02x%02x%02x\n", s_idAuthen[6], s_idAuthen[7], s_idAuthen[10], s_idAuthen[11], 
                    s_snAuthen[0], s_snAuthen[1], s_snAuthen[2], s_snAuthen[3], s_snAuthen[4], s_snAuthen[5], s_snAuthen[6], s_snAuthen[7]);
            }
            else 
            {
                s_checkAuthen = CHECK_AUTHEN_NORMAL;
                PRINT_DBG("Normal Id=0x%02x%02x-%02x%02x sn=%02x%02x%02x%02x-%02x%02x%02x%02x\n", s_idAuthen[6], s_idAuthen[7], s_idAuthen[10], s_idAuthen[11], 
                    s_snAuthen[0], s_snAuthen[1], s_snAuthen[2], s_snAuthen[3], s_snAuthen[4], s_snAuthen[5], s_snAuthen[6], s_snAuthen[7]);
            }
        }
        else
        {
            s_checkAuthen = CHECK_AUTHEN_MISS;
        }

        if (s_checkAuthen == CHECK_AUTHEN_WARNING || s_checkAuthen == CHECK_AUTHEN_MISS)
        {
            if ((strcmp(g_hardware, g_hardware_AEVISION_HI3518E_D1408) == 0))
            {
                s_idAuthen[6] = 0x1;
                s_idAuthen[7] = 0x8;
                s_idAuthen[10] = 0x1;
                s_idAuthen[11] = 0x65;
            }
            else if ((strcmp(g_hardware, g_hardware_AEVISION_HI3518C_D1405_1XX18M) == 0) || 
                (strcmp(g_hardware, g_hardware_AEVISION_HI3518C_D1405_1XX1) == 0) ||
                (strcmp(g_hardware, g_hardware_AEVISION_HI3518C_D1406_JUAN8M) == 0))
            {
                s_idAuthen[6] = 0x1;
                s_idAuthen[7] = 0x8;
                s_idAuthen[10] = 0x1;
                s_idAuthen[11] = 0x62;
            }
			else if ((strcmp(g_hardware, g_hardware_FSAN_HI3518C_D1402_JUAN) == 0) && (strcmp(g_sensorModel, g_sensorModel_OV9712P) == 0))
			{
				s_idAuthen[6] = 0x1;
                s_idAuthen[7] = 0x8;
                s_idAuthen[10] = 0x2;
                s_idAuthen[11] = 0x62;
				s_checkAuthen = CHECK_AUTHEN_NORMAL;
			}
            else
            {
                PRINT_DBG("Unknown hardware.\n");
            }
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
            PRINT_DBG("Abnormal Id=0x%02x%02x-%02x%02x sn=%02x%02x%02x%02x-%02x%02x%02x%02x\n", s_idAuthen[6], s_idAuthen[7], s_idAuthen[10], s_idAuthen[11], 
                s_snAuthen[0], s_snAuthen[1], s_snAuthen[2], s_snAuthen[3], s_snAuthen[4], s_snAuthen[5], s_snAuthen[6], s_snAuthen[7]);
        }
        if (s_idAuthen[10] != 0 && s_idAuthen[11] != 0)
        {
        }
        else
        {
            s_checkAuthen = CHECK_AUTHEN_UNKNOWN;
            PRINT_DBG("Unknown authen id.\n");
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

const unsigned char *GetAuthenId()
{
    static unsigned char id[16];
    Check_Authen(id, NULL);
    return id;
}
const unsigned char *GetAuthenSn()
{
    static unsigned char sn[16];
    Check_Authen(NULL, sn);
    return sn;
}

static void InitProgress(unsigned int erasingSize, unsigned int writingSize)
{
    if (erasingSize > 0)
    {
        s_programStatus.erasingSize = erasingSize;
    }
    if (writingSize > 0)
    {
        s_programStatus.writingSize = writingSize;
    }
    if (NULL == s_logFileFp)
    {
        s_logFileFp = fopen(s_logFile, "w");
    }
}

static void SetProgress(const char *status, unsigned int value)
{
    if (status && strlen(status) > 0)
    {
        if (strcmp(status, s_statusPreparing) == 0)
        {
            snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
        }
        else if (strcmp(status, s_statusErasing) == 0)
        {
            snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
            s_programStatus.erasedSize += value;
        }
        else if (strcmp(status, s_statusWriting) == 0)
        {
            snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
            s_programStatus.writtenSize += value;
        }
        else if (strcmp(status, s_statusExtracting) == 0)
        {
            snprintf(s_programStatus.status, sizeof(s_programStatus.status), "%s", status);
            s_programStatus.writtenSize += value;
        }
        else
        {
        }
    }
}

int QueryProgress()
{
    int progress = 0;
    if (strlen(s_programStatus.status) > 0 && (s_programStatus.erasingSize > 0 || s_programStatus.writingSize > 0))
    {
        progress = MIN2(100, (s_programStatus.erasedSize + s_programStatus.writtenSize) * 100 / (s_programStatus.erasingSize + s_programStatus.writingSize));
    }
    return progress;
}

static void PrintProgress()
{
//    printf("\rtotal: %%%d ((%x+%x)/(%x+%x)) status: %-20s", 
//            progress, s_programStatus.erasedSize, s_programStatus.writtenSize, s_programStatus.erasingSize, s_programStatus.writingSize, s_programStatus.status);
    if (strlen(s_programStatus.status) > 0 && (s_programStatus.erasingSize > 0 || s_programStatus.writingSize > 0))
    {
        int progress = MIN2(100, ((s_programStatus.erasedSize + s_programStatus.writtenSize) * 1.0 / (s_programStatus.erasingSize + s_programStatus.writingSize))*100);
        printf("\rtotal: %%%d ((%x+%x)/(%x+%x)) status: %-20s", 
            progress, s_programStatus.erasedSize, s_programStatus.writtenSize, s_programStatus.erasingSize, s_programStatus.writingSize, s_programStatus.status);
        fflush(stdout);
		if (NULL != s_logFileFp)
		{
			fseek(s_logFileFp, 0, SEEK_SET);
			fprintf(s_logFileFp, "\rtotal: %%%d ((%x+%x)/(%x+%x)) status: %-20s", 
				progress, s_programStatus.erasedSize, s_programStatus.writtenSize, s_programStatus.erasingSize, s_programStatus.writingSize, s_programStatus.status);
			fflush(s_logFileFp);
		}
    }
}

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
                printf("Open device [%s] failed. errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                ret = -1;
            }
            else
            {
                printf("mtd%d info: type=%d(%s) size=%d esize=%d wsize=%d osize=%d\n", count, mtd[count].type, 
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

static int GetMtdInfoByPos(int flashPosition, struct mtd_info_user *mtdinfo, int *mtdIndex, int *inOffset)
{
    int ret = 0;
    int count = 0;
    struct mtd_info_user * mtds = NULL;
    GetMtdFlashInfo(&count, &mtds);

    if (flashPosition <0)
    {
        ret = -1;
    }
    
    int i;
    int offset = 0;
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

static int GetMtdIndexByName(char *szMtdName)
{
/*
dev:    size   erasesize  name
mtd0: 00100000 00020000 "uboot"
mtd1: 00100000 00020000 "factory"
mtd2: 00300000 00020000 "config"
mtd3: 00500000 00020000 "kernel"
mtd4: 02000000 00020000 "custom"
mtd5: 05600000 00020000 "smart"
*/
    int ret = 0;
    FILE *fp = NULL;
    if (0 == ret)
    {
		char mtdDevice[] = "/proc/mtd";
	    fp = fopen(mtdDevice, "r");
	    if(fp == NULL)
	    {
	        ret = -1;
	        printf("Open device [%s] failed. errno=%d(%s)\n", mtdDevice, errno, strerror(errno));
	    }
    }
    
	int mtdIndex = -1;
    if (0 == ret)
    {
		char buffer[128];
		int index;
		char name[64];
		for (; ; )
		{
			if (fgets(buffer, sizeof(buffer), fp) == NULL)
			{
				break;
			}
			else
			{
				//printf("buffer=[%s]\n", buffer);
				if (sscanf(buffer, "mtd%d: %*s %*s %s", &index, name) == 2)
				{
					// end of name is ".
					name[strlen(name)-1] = '\0';
					// name[0] is ".
					char *pname = name + 1;
					//printf("index=%d name=%s\n", index, pname);
					if (strcmp(pname, szMtdName) == 0)
					{
						mtdIndex = index;
						break;
					}
				}
			}
		}
    }

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}
    
    return mtdIndex;
}

static int GetMtdInfoByIndex(int mtdIndex, struct mtd_info_user *mtdinfo, int *startPos)
{
    int ret = 0;
    int count = 0;
    struct mtd_info_user * mtds = NULL;
    GetMtdFlashInfo(&count, &mtds);

    if (mtdIndex < 0 || mtdIndex >= count)
    {
        ret = -1;
    }

    if (0 == ret)
    {
        if (mtdinfo)
        {
            memcpy(mtdinfo, &mtds[mtdIndex], sizeof(*mtdinfo));
        }
        if (startPos)
        {
            int i;
            int offset = 0;
            for (i = 0; i < mtdIndex; i++)
            {
                offset += mtds[i].size;
            }
            *startPos = offset;
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

static int FlashErase(int position, int length)
{
    int ret = 0;
    
    struct mtd_info_user mtdinfo;
    int mtdIndex = 0;
    int inOffset = 0;
    if (position < 0 || length <= 0)
    {
        ret = -1;
    }

    int devFd = -1;
    
    int handleLength = 0;
    while (0 == ret && handleLength < length)
    {
        if (0 == ret)
        {
            if (GetMtdInfoByPos(position + handleLength, &mtdinfo, &mtdIndex, &inOffset) < 0)
            {
                ret = -1;
                printf("Not find such mtd info at pos=0x%x\n", position);
            }
            else
            {
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
                printf("Open [%s] failed.errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                ret = -1;
            }
        }

        if (0 == ret)
        {
            // Erase blocks.
            int eraseStart = inOffset;
            int eraseLength = MIN2((int)mtdinfo.size - inOffset, length - handleLength);
            eraseLength = (eraseLength + mtdinfo.erasesize - 1) / mtdinfo.erasesize * mtdinfo.erasesize;
            int tmpCount = 0;
            for (; tmpCount < eraseLength; tmpCount += mtdinfo.erasesize)
            {
                struct erase_info_user erase;
                erase.start = eraseStart + tmpCount;
                erase.length = mtdinfo.erasesize;
                
                erase.start = next_good_eraseblock(devFd, &mtdinfo, erase.start);
#if (CONFIG_WRITEFILE==0)
                if (ioctl(devFd, MEMERASE, &erase) < 0)
#endif
                {
                    ret = errno;
                    printf("Erase [%s][0x%x] failed. errno=%d(%s)\n", mtdDevice, erase.start, ret, strerror(ret));
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

static int FlashCopy(int position, int fd, int length)
{
    int ret = 0;

    struct mtd_info_user mtdinfo;
    int mtdIndex = 0;
    int inOffset = 0; // internal offset of this mtd.
    int devFdSeek = 0;
    if (position < 0 || fd < 0 || length < 0)
    {
        ret = -1;
    }

    int devFd = -1;
    
    int handleLength = 0;
    while (0 == ret && handleLength < length)
    {
        if (0 == ret)
        {
            if (GetMtdInfoByPos(position + handleLength, &mtdinfo, &mtdIndex, &inOffset) < 0)
            {
                ret = -1;
                printf("Not find such mtd info at pos=0x%x\n", position);
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
                printf("Open [%s] failed.errno=%d(%s)\n", mtdDevice, ret, strerror(ret));
                ret = -1;
            }
        }

        if (0 == ret)
        {
            // Copy data.
            int bufferLen = mtdinfo.erasesize;
            char *buffer = (char*)malloc(bufferLen);
            if (buffer == NULL)
            {
                ret = -1;
                printf("malloc failed size=0x%x.\n", bufferLen);
            }

            devFdSeek = inOffset;
            while (0 == ret && handleLength < length && devFdSeek < (int)mtdinfo.size)
            {
                unsigned int blockBasePos = devFdSeek / mtdinfo.erasesize * mtdinfo.erasesize;
                unsigned int blockBasePosNew = next_good_eraseblock(devFd, &mtdinfo, blockBasePos);
                if (blockBasePosNew != blockBasePos)
                {
                    devFdSeek = devFdSeek - blockBasePos + blockBasePosNew;
                }
                if (devFdSeek >= mtdinfo.size)
                {
                    ret = -1;
                    printf("No space enough.\n");
                    break;
                }
                else if (lseek(devFd, devFdSeek, SEEK_SET) < 0)
                {
                    ret = -1;
                    printf("Seek failed. mtdIndex=%d. devFdSeek=0x%x.\n", mtdIndex, devFdSeek);
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
                        printf("Write failed(%d). mtdIndex=%d. devFdSeek=0x%x. writeLen=0x%x.\n", retA, mtdIndex, devFdSeek, readLen);
                        printf("%s\n", strerror(retA));
                        break;
                    }
                    else
                    {
                        handleLength += readLen;

                        //PRINT_DBG("Writing pos=%x len=%x\n", devFdSeek, readLen);
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

static int UpdateFileToFlash(int fd, unsigned int writingSize, unsigned int flashAddr, unsigned int erasingSize)
{
    int ret = 0;

    if (ret >= 0)
    {
        if (erasingSize > 0)
        {
            ret = FlashErase(flashAddr, erasingSize);
        }
    }

    if (ret >= 0)
    {
        if (writingSize > 0)
        {
            ret = FlashCopy(flashAddr, fd, writingSize);
        }
    }

    return ret < 0 ? ret : 0;
}

static int ExtractArchiveFile(int fileFd, unsigned int writingSize)
{
    int retA = 0;
    int partDataSize = writingSize;
    
    int tmpFd = -1;
    const char *tmpPath = "/dev/tmp.tgz";
    if (0 == retA)
    {
        tmpFd = open(tmpPath, O_CREAT | O_RDWR);
        if (tmpFd < 0)
        {
            retA = -1;
            printf("Create file [%s] failed.\n", tmpPath);
        }
    }
    
    if (0 == retA)
    {
        int readLen = 0;
        char buffer[4096];

        SetProgress("Extracting", 0);
        PrintProgress();

        for (readLen = 0; readLen < partDataSize; readLen += sizeof(buffer))
        {
            int readsize = safe_read(fileFd, buffer, sizeof(buffer));
            if (readsize > 0)
            {
                int writtensize = safe_write(tmpFd, buffer, readsize);
                if (writtensize < 0)
                {
                    retA = errno;
                    printf("Write failed. errno=%d(%s)\n", retA, strerror(retA));
                    retA = -1;
                    break;
                }
                if (readLen + readsize >= partDataSize)
                {
                    //PRINT_DBG("readLen=%x\n", readLen);
                    readLen = partDataSize;
                    break;
                }
            }
            else
            {
                if (readsize < 0)
                {
                    retA = errno;
                    printf("Write failed. errno=%d(%s)\n", retA, strerror(retA));
                    retA = -1;
                }
                break;
            }
        }

        close(tmpFd);
        tmpFd = -1;
        
        char cmdline[128] = "";

        SetProgress("Extracting", partDataSize/3);
        PrintProgress();

        snprintf(cmdline, sizeof(cmdline), "tar -xzf %s -C /", tmpPath);
        system(cmdline);

        SetProgress("Extracting", partDataSize - partDataSize/3);
        PrintProgress();
        
        unlink(tmpPath);
    }
    
    return retA < 0 ? retA : partDataSize;
}
/*
static int WriteFlashPartition(int partIndex, int fileFd, int bFileFormatInFlash)
{
    int retA = 0;
    int readOffset = 0;
    int partBlockSize = 0;
    int partTotalSize = 0;
    int partDataSize = 0;
    int writtenSize = 0;
    char mtdDevice[32];
    char *dataBlock = NULL;
    char *idleBlock = NULL;
    
    struct mtd_info_user mtd;
    struct erase_info_user erase;

    snprintf(mtdDevice, sizeof(mtdDevice), "/dev/mtd%d", partIndex);
    // Open mtd device
    int devFd = -1;
    devFd = open(mtdDevice, O_SYNC | O_RDWR);
    if(devFd < 0)
    {
        retA = errno;
        printf("Open device [%s] failed. errno=%d(%s)\n", mtdDevice, retA, strerror(retA));
        retA = -1;
    }
    
    unsigned int packageType;
    if (0 == bFileFormatInFlash)
    {
        UpdatePackageHeader_T packHead;
        if (sizeof(packHead) != safe_read(fileFd, &packHead, sizeof(packHead)))
        {
            retA = -1;
            printf("Read pack head failed.\n");
        }
        else
        {
            partDataSize = packHead.uPackageLength;
            packageType = packHead.uPackageType;
        }
    }
    
    // Query mtd info
    if (0 == retA)
    {
        if (ioctl(devFd, MEMGETINFO, &mtd) < 0)
        {
            retA = errno;
            printf("Open device [%s] failed. errno=%d(%s)\n", mtdDevice, retA, strerror(retA));
            retA = -1;
        }
        else
        {
            partBlockSize = mtd.erasesize;
            partTotalSize = mtd.size;
            
            if (1 == bFileFormatInFlash)
            {
                partDataSize = partTotalSize;//! 如果是升级包,从文件中得到该值.
            }
            else if (partDataSize > partTotalSize)
            {
                retA = -1;
                printf("Too much data (%d) than partition size (%d). Stop.\n", partDataSize, partTotalSize);
            }
        }
    }
    
    // 
    if (0 == retA)
    {
        dataBlock = realloc(dataBlock, partBlockSize);
        if (dataBlock == NULL)
        {
            retA = -1;
        }
    }
    if (0 == retA)
    {
        idleBlock = realloc(idleBlock, partBlockSize);
        if (idleBlock == NULL)
        {
            retA = -1;
        }
        else
        {
            memset(idleBlock, 0xff, partBlockSize);
        }
    }
    
    // Erase all blocks of this partition
    if (0 == retA)
    {
        erase.start = 0;
        erase.length = partBlockSize;
        SetProgress(0, partIndex, 0, "Erasing");
        PrintProgress();
        for ( ; erase.start + erase.length <= partTotalSize; erase.start += erase.length)
        {
            SetProgress(0, -1, erase.start, "Erasing");
            PrintProgress();
            
            if (ioctl(devFd, MEMERASE, &erase) < 0)
            {
                retA = errno;
                printf("Erase [%s][%x] failed. errno=%d(%s)\n", mtdDevice, erase.start, retA, strerror(retA));
                retA = 0; // Keep go on if erase failed.
            }
        }
        SetProgress(0, -1, erase.start, NULL);
        PrintProgress();
    }
    
    if (0 == retA)
    {
        int readingSize = partBlockSize;
        SetProgress(0, partIndex, 0, "Writing");
        PrintProgress();
        for (readOffset = 0; readOffset < partDataSize; readOffset += readingSize)
        {
            SetProgress(0, partIndex, readOffset, NULL);
            PrintProgress();
            // Read data of one block-size from file.
            if (readOffset + partBlockSize > partDataSize)
            {
                readingSize = partDataSize - readOffset;
                memset(dataBlock, 0xff, partBlockSize);
            }
            safe_read(fileFd, dataBlock, readingSize);
            
            // Write blocks if there is data need be written.
            if (memcmp(dataBlock, idleBlock, partBlockSize) != 0)
            {
                safe_write(devFd, dataBlock, partBlockSize);
            }
            else
            {
                lseek(devFd, partBlockSize, SEEK_CUR);
            }
            writtenSize += readingSize;
        }
        SetProgress(0, partIndex, readOffset, NULL);
        PrintProgress();
        //printf("\rWriting: %%%d (%08x / %08x)\n", readOffset * 100 / partDataSize, readOffset, partDataSize);
    }
    
    if (devFd >= 0)
    {
        close(devFd);
        devFd = -1;
    }
    
    if (dataBlock)
    {
        free(dataBlock);
        dataBlock = NULL;
    }
    if (idleBlock)
    {
        free(idleBlock);
        idleBlock = NULL;
    }
    
    return writtenSize;
}
*/
static int CheckPackHead(UpdatePackageHeader_T *pPackage)
{
    int ret = 0;
    if (NULL == pPackage)
    {
        return -1;
    }

    if (0 == ret)
    {
        if (pPackage->uMagicNumber != UPDATEPACKAGEHEADERMAGIC || 
			UpdateType_Nothing >= pPackage->uPackageType || 
			UpdateType_Butt <= pPackage->uPackageType)
        {
            ret = -1;
            printf("Invalid pack head.\n");
        }
    }
    
    if (0 == ret)
    {/*
        if (pPackage->uPackageCheckType == 0)
        {
            ANTSMID_MD5_CTX m_md5;
            unsigned int m_md5_result[4];
            Antsmid_MD5Init(&m_md5);
            Antsmid_MD5Update(&m_md5,((unsigned char *)pPackage + sizeof(UpdatePackageHeader_T)),pPackage->uPackageLength);
            Antsmid_MD5Final((unsigned char *)m_md5_result,&m_md5);
            if(m_md5_result[0] != pPackage->uPackageCheckValue[0] ||
               m_md5_result[1] != pPackage->uPackageCheckValue[1] ||
               m_md5_result[2] != pPackage->uPackageCheckValue[2] ||
               m_md5_result[3] != pPackage->uPackageCheckValue[3])
            {
                ret = -1;
                printf("Check pack head failed.\n");
            }
        }*/
    }
    
    return ret;
}

int JudgeAllFlashFile(const char *filePath)
{
    int ret = 0;
    
    unsigned int fileSize = 0;
    if (0 == ret)
    {
        struct stat fileStat;
        if (0 == stat(filePath, &fileStat))
        {
            fileSize = (unsigned int)fileStat.st_size;
        }
        else
        {
            ret = errno;
            printf("Query file stat [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }

    if (0 == ret)
    {
        if (fileSize >= 0x400000 && (fileSize & (fileSize-1)) == 0)
        {
            InitProgress(fileSize, fileSize);
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}

typedef struct
{
    UpdateFileHeader_T fileHeader;
    unsigned int packOffset[16];
    UpdatePackageHeader_T packHeader[16];
	unsigned int packPartIdxByName[16];
    unsigned int packCount;
} UpdateParam_T;

int JudgeUpdateFile(const char* filePath, UpdateParam_T *updateParam)
{
    int ret = 0;
    
    int fileFd = -1;
    if (0 == ret)
    {
        fileFd = open(filePath, O_RDONLY);
        if (fileFd < 0)
        {
            ret = errno;
            printf("Open file [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    int fileSize = 0;
    if (0 == ret)
    {
        struct stat fileStat;
        if (0 == stat(filePath, &fileStat))
        {
            fileSize = fileStat.st_size;
        }
        else
        {
            ret = errno;
            printf("Query file stat [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    int readOffset = 0;
    if (0 == ret)
    {
        UpdateFileHeader_T fileHead;
        int partOffsetInFile[32] = {0};
        
        readOffset += safe_read(fileFd, &fileHead, sizeof(fileHead));
        if (fileHead.uMagicNumber == UPDATEFILEHEADERMAGIC && fileHead.uPackageTypeMark != 0 && 
            fileHead.uSupportBoardNum > 0 && fileHead.uSupportBoardNum < 256 &&
            fileHead.uPackageNum > 0 && fileHead.uPackageNum < 33)
        {
            lseek(fileFd, fileHead.uSupportBoardNum*sizeof(int), SEEK_CUR);
            readOffset += fileHead.uSupportBoardNum*sizeof(int);

            readOffset += read(fileFd, partOffsetInFile, fileHead.uPackageNum*sizeof(int));

            unsigned int erasingSize = 0;
            unsigned int writingSize = 0;
            
            int count = 0;
            struct mtd_info_user * mtds = NULL;
            GetMtdFlashInfo(&count, &mtds);
            
            memcpy(&updateParam->fileHeader, &fileHead, sizeof(fileHead));
            updateParam->packCount = MIN2(fileHead.uPackageNum, sizeof(updateParam->packHeader)/sizeof(updateParam->packHeader[0]));
            
            unsigned int i;
            for (i = 0; i < fileHead.uPackageNum; i++)
            {
                if (partOffsetInFile[i] < fileSize && (i == 0 || partOffsetInFile[i] > partOffsetInFile[i-1]))
                {
                    lseek(fileFd, partOffsetInFile[i], SEEK_SET);
                    UpdatePackageHeader_T packHead;
                    if (sizeof(packHead) != safe_read(fileFd, &packHead, sizeof(packHead)))
                    {
                        ret = -1;
                        printf("Read pack head failed.\n");
                        break;
                    }
                    else if (CheckPackHead(&packHead) <0)
                    {
                        ret = -1;
                    }
                    else if (packHead.uPackageType == UpdateType_File || packHead.uPackageType == UpdateType_Tar)
                    {
                        writingSize += packHead.uPackageLength - packHead.uExtPackHeaderLengh;
                    }
                    else if (packHead.uPackageType >= UpdateType_Uboot && packHead.uPackageType <= UpdateType_ChLogo && (int)packHead.uPackageType <= count)
                    {
                        erasingSize += mtds[packHead.uPackageType-1].size;
                        writingSize += packHead.uPackageLength - packHead.uExtPackHeaderLengh;
                    }
					else if (packHead.uPackageType == UpdateType_PartIndex)
					{
						int nPartidx = packHead.uPartIndx - 1;
						erasingSize += mtds[nPartidx].size;
						writingSize += packHead.uPackageLength - packHead.uExtPackHeaderLengh;
					}
					else if (packHead.uPackageType == UpdateType_PartName)
					{
						char szPartName[256]="";
						if(packHead.uExtPackHeaderLengh == 0 || packHead.uExtPackHeaderLengh != safe_read(fileFd, szPartName, packHead.uExtPackHeaderLengh))
						{
							ret = -1;
							printf("Read pack ExtPack failed.\n");
							break;
						}
						int nPartidx = GetMtdIndexByName(szPartName);
						if (nPartidx < 0 )
						{
							ret = -1;
							printf("nPartIdx = %d byName[%s]\n",nPartidx,szPartName);
							break;
						}
						erasingSize += mtds[nPartidx].size;
						writingSize += packHead.uPackageLength - packHead.uExtPackHeaderLengh;
						updateParam->packPartIdxByName[i] = nPartidx;
					}
					else if (packHead.uPackageType == UpdateType_Flash)
					{
						erasingSize += packHead.uPackageLength - packHead.uExtPackHeaderLengh;
						writingSize += packHead.uPackageLength - packHead.uExtPackHeaderLengh;
					}
                    
                    updateParam->packOffset[i] = partOffsetInFile[i];
                    memcpy(&updateParam->packHeader[i], &packHead, sizeof(packHead));
                }
                else
                {
                    ret = 1;
                    break;
                }
            }

            InitProgress(erasingSize, writingSize);
        }
        else
        {
            ret = -1;
        }
    }

    if (fileFd >= 0)
    {
        close(fileFd);
        fileFd = -1;
    }
    
    return ret;
}

int UpgradeUpdateFile(const char *filePath, UpdateParam_T *updateParam)
{
    int ret = 0;
    
    int fileFd = -1;
    if (0 == ret)
    {
        fileFd = open(filePath, O_RDONLY);
        if (fileFd < 0)
        {
            ret = errno;
            printf("Open file [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    int count = 0;
    struct mtd_info_user * mtds = NULL;
    GetMtdFlashInfo(&count, &mtds);

    unsigned int fileSize = 0;
    if (0 == ret)
    {
        struct stat fileStat;
        if (0 == stat(filePath, &fileStat))
        {
            fileSize = (unsigned int)fileStat.st_size;
        }
        else
        {
            ret = errno;
            printf("Query file stat [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    if (0 == ret)
    {
        printf("File format is Upgrade Package packCount=%d\n", updateParam->packCount);

        unsigned int i;
        for (i = 0; i < updateParam->packCount; i++)
        {
            lseek(fileFd, updateParam->packOffset[i] + sizeof(updateParam->packHeader[i]) + updateParam->packHeader[i].uExtPackHeaderLengh, SEEK_SET);
            if (updateParam->packHeader[i].uPackageType == UpdateType_File || updateParam->packHeader[i].uPackageType == UpdateType_Tar)
            {
                ExtractArchiveFile(fileFd, updateParam->packHeader[i].uPackageLength - updateParam->packHeader[i].uExtPackHeaderLengh);
            }
            else
            {
                int mtdIndex = updateParam->packHeader[i].uPackageType-1;
                int startPos = 0;
				if (updateParam->packHeader[i].uPackageType == UpdateType_PartIndex)
				{
					mtdIndex = updateParam->packHeader[i].uPartIndx - 1;
				}
				else if (updateParam->packHeader[i].uPackageType == UpdateType_PartName)
				{
					mtdIndex = updateParam->packPartIdxByName[i];
				}
				if (updateParam->packHeader[i].uPackageType == UpdateType_Flash)
				{
					unsigned int offset = 0;
					unsigned int eachSize = mtds->erasesize;
					fileSize = updateParam->packHeader[i].uPackageLength;
					while (offset < fileSize)
					{
						if (UpdateFileToFlash(fileFd, eachSize, offset, eachSize) < 0)
						{
							ret = -1;
							break;
						}
						else
						{
							offset += eachSize;
						}
					}
				}
				else
				{
					GetMtdInfoByIndex(mtdIndex, NULL, &startPos);
					//PRINT_DBG("[%x %x %x %x]\n", updateParam->packOffset[i] + sizeof(updateParam->packHeader[i]) + updateParam->packHeader[i].uRes[0], 
					//    updateParam->packHeader[i].uPackageLength, startPos, mtds[mtdIndex].size);
					if (UpdateFileToFlash(fileFd, updateParam->packHeader[i].uPackageLength - updateParam->packHeader[i].uExtPackHeaderLengh, startPos, mtds[mtdIndex].size) < 0)
					{
						ret = -1;
						break;
					}
				}
            }
        }
    }
    
    if (fileFd >= 0)
    {
        close(fileFd);
        fileFd = -1;
    }
    
    if (s_logFileFp)
    {
        fclose(s_logFileFp);
        s_logFileFp = NULL;
    }
    
    return ret;
}

int JudgeRawPartUpgradeFileHeader(const char *filePath, RawPartUpgradeFileHeader_T *fileHeader)
{
    int ret = 0;

    int fileFd = -1;
    if (0 == ret)
    {
        fileFd = open(filePath, O_RDONLY);
        if (fileFd < 0)
        {
            ret = errno;
            printf("Open file [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    RawPartUpgradeFileHeader_T rpuFileHeader;
    if (0 == ret)
    {
        int readlen = read(fileFd, &rpuFileHeader, sizeof(rpuFileHeader));
        if (readlen < (int)sizeof(rpuFileHeader))
        {
            //printf("Read file [%s] readlen=%d not enough.\n", filePath, readlen);
            ret = -1;
        }
    }

    if (0 == ret)
    {
        if (rpuFileHeader.uMagicNumber != ARPUHEADERMAGIC)
        {
            ret = -1;
        }
    }
    
    //PrintBuffer(&rpuFileHeader, sizeof(rpuFileHeader));
#if 1
    if (0 == ret)
    {
        char tmpChecksum[16];
        
        // 校验头部
        memcpy(tmpChecksum, rpuFileHeader.headChecksum, sizeof(rpuFileHeader.headChecksum));
        memset(rpuFileHeader.headChecksum, 0, sizeof(rpuFileHeader.headChecksum));

        // Compute file MD5.
        ANTSMID_MD5_CTX md5;
        Antsmid_MD5Init(&md5);
        Antsmid_MD5Update(&md5, (unsigned char *)&rpuFileHeader, rpuFileHeader.headSize);
        Antsmid_MD5Final(rpuFileHeader.headChecksum, &md5);
        
        if (memcmp(tmpChecksum, rpuFileHeader.headChecksum, sizeof(rpuFileHeader.headChecksum) != 0))
        {
            printf("Head checksum is invalid.\n");
            ret = 1;
        }
    }

    if (0 == ret)
    {
        unsigned char tmpChecksum[16];
        char tmpbuffer[65536];
        int readlen = 0;
        int i;
        for (i = 0; i < rpuFileHeader.partCount; i++)
        {
            off_t offset = lseek(fileFd, rpuFileHeader.headSize + rpuFileHeader.partInfo[i].fileStartKB*1024, SEEK_SET);
            if (offset == (off_t)(-1))
            {
                printf("Seek part [%d]=[%d] out of range.\n", i, rpuFileHeader.headSize + rpuFileHeader.partInfo[i].fileStartKB*1024);
                ret = -1;
                break;
            }
            
            ANTSMID_MD5_CTX md5;
            Antsmid_MD5Init(&md5);
            unsigned int j;
            for (j = 0; j < rpuFileHeader.partInfo[i].fileSizeKB*1024; j += readlen)
            {
                readlen = rpuFileHeader.partInfo[i].fileSizeKB*1024 - j;
                readlen = readlen < (int)sizeof(tmpbuffer) ? readlen : (int)sizeof(tmpbuffer);
                readlen = safe_read(fileFd, tmpbuffer, readlen);
                Antsmid_MD5Update(&md5, (unsigned char*)tmpbuffer, readlen);
            }
            Antsmid_MD5Final(tmpChecksum, &md5);
            if (memcmp(tmpChecksum, rpuFileHeader.partInfo[i].partChecksum, sizeof(rpuFileHeader.partInfo[i].partChecksum)) != 0)
            {
                printf("Part[%d] checksum is invalid.\n", i);
                ret = 1;
                break;
            }
        }
    }
#endif

    if (0 == ret)
    {
        const unsigned char *id = GetAuthenId();
        int * idtable = rpuFileHeader.boardid;
        int idcount = sizeof(rpuFileHeader.boardid)/sizeof(rpuFileHeader.boardid[0]);
        int i;
        for (i = 0; i < idcount; i++)
        {
            if (idtable[i] == (id[10]<<8 | id[11]))
            {
                break;
            }
        }
        if (i == idcount)
        {
            printf("Id[%04x] is not effective for upgrading. No need to upgrade.\n", (id[10]<<8 | id[11]));
            ret = 1;
        }
    }

    if (0 == ret)
    {
        unsigned int erasingSize = 0;
        unsigned int writingSize = 0;
        int i;
        for (i = 0; i < rpuFileHeader.partCount; i++)
        {
            erasingSize += rpuFileHeader.partInfo[i].partSizeKB*1024;
            writingSize += rpuFileHeader.partInfo[i].fileSizeKB*1024;
        }
        InitProgress(erasingSize, writingSize);
    }
    
    if (0 == ret && fileHeader)
    {
        memcpy(fileHeader, &rpuFileHeader, sizeof(rpuFileHeader));
    }
    
    if (fileFd >= 0)
    {
        close(fileFd);
        fileFd = -1;
    }
    
    return ret;
}

int UpgradeAllFlashFile(const char *filePath)
{
    int ret = 0;

    int fileFd = -1;
    if (0 == ret)
    {
        fileFd = open(filePath, O_RDONLY);
        if (fileFd < 0)
        {
            ret = errno;
            printf("Open file [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    unsigned int fileSize = 0;
    if (0 == ret)
    {
        struct stat fileStat;
        if (0 == stat(filePath, &fileStat))
        {
            fileSize = (unsigned int)fileStat.st_size;
        }
        else
        {
            ret = errno;
            printf("Query file stat [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }

    if (0 == ret)
    {
        unsigned int offset = 0;
        unsigned int eachSize = 0x100000;
        while (offset < fileSize)
        {
            if (UpdateFileToFlash(fileFd, eachSize, offset, eachSize) < 0)
            {
                ret = -1;
                break;
            }
            else
            {
                offset += eachSize;
            }
        }
    }
    
    return ret;
}

int UpgradeRawPartUpgradeFile(const char *filePath, RawPartUpgradeFileHeader_T *fileHeader)
{
    int ret = 0;

    int fileFd = -1;
    if (0 == ret)
    {
        fileFd = open(filePath, O_RDONLY);
        if (fileFd < 0)
        {
            ret = errno;
            printf("Open file [%s] failed. errno=%d(%s)\n", filePath, ret, strerror(ret));
            ret = -1;
        }
    }
    
    int readOffset = 0;
    int i;
    for (i = 0; i < fileHeader->partCount; i++)
    {
        readOffset = fileHeader->headSize + fileHeader->partInfo[i].fileStartKB*1024;
        lseek(fileFd, readOffset, SEEK_SET);

        if (UpdateFileToFlash(fileFd, fileHeader->partInfo[i].fileSizeKB*1024, fileHeader->partInfo[i].partStartKB*1024, fileHeader->partInfo[i].partSizeKB*1024) < 0)
        {
            ret = -1;
            break;
        }
    }

    return ret;
}

const unsigned char* BuildDataTime()
{
    static unsigned char BuildDataTimeInfo[6] = {0};

    if (BuildDataTimeInfo[0] == 0)
    {
        char *szMonthDesc[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        char dateMonthStr[8];
        int dateYear = 2015;
        int dateMonth = 1;
        int dateDay = 1;
        int timeHour = 0;
        int timeMin = 0;
        int timeSec = 0;
        
        sscanf(__DATE__, "%3s %d %d", dateMonthStr, &dateDay, &dateYear);
        sscanf(__TIME__, "%d:%d:%d", &timeHour, &timeMin, &timeSec);

        int i;
        for (i = 0; i < sizeof(szMonthDesc)/sizeof(szMonthDesc[0]); i++)
        {
            if (strncasecmp(szMonthDesc[i], dateMonthStr, 3) == 0)
            {
                dateMonth = i + 1;
                break;
            }
        }

        BuildDataTimeInfo[0] = (unsigned char)(dateYear - 2010);
        BuildDataTimeInfo[1] = (unsigned char)(dateMonth - 1);
        BuildDataTimeInfo[2] = (unsigned char)(dateDay - 1);
        BuildDataTimeInfo[3] = (unsigned char)timeHour;
        BuildDataTimeInfo[4] = (unsigned char)timeMin;
        BuildDataTimeInfo[5] = (unsigned char)timeSec;
    }
    
    return BuildDataTimeInfo;
}

const unsigned char* BuildVersionInfo()
{
    static unsigned char BuildVersionInfo[4] = {0};
    if (BuildVersionInfo[0] == 0)
    {
        const unsigned char *datetimeInfo = BuildDataTime();

        struct tm tmpTime;
        tmpTime.tm_year = 2010 - 1900;
        tmpTime.tm_mon = 0;
        tmpTime.tm_mday = 1;
        tmpTime.tm_hour = 0;
        tmpTime.tm_min = 0;
        tmpTime.tm_sec = 0;
        time_t tmpBase = mktime(&tmpTime);
        
        tmpTime.tm_year = datetimeInfo[0] + 2010 - 1900;
        tmpTime.tm_mon = datetimeInfo[1];
        tmpTime.tm_mday = datetimeInfo[2] + 1;
        tmpTime.tm_hour = datetimeInfo[3];
        tmpTime.tm_min = datetimeInfo[4];
        tmpTime.tm_sec = datetimeInfo[5];
        time_t tmpNow = mktime(&tmpTime);

        unsigned int tmp = (tmpNow - tmpBase) / 60;
        BuildVersionInfo[0] = (unsigned char)VERSION_MAJOR;
        BuildVersionInfo[1] = (unsigned char)((tmp >> 16) & 0xff);
        BuildVersionInfo[2] = (unsigned char)((tmp >> 8) & 0xff);
        BuildVersionInfo[3] = (unsigned char)(tmp & 0xff);
    }

    return BuildVersionInfo;
}

void PrintHelp(const char* appName)
{
    const unsigned char * verInfo = BuildVersionInfo();
    printf("Version: V%d.%d.%d.%d (%s %s)\n", verInfo[0], verInfo[1], verInfo[2], verInfo[3], __DATE__, __TIME__);
    printf("Copyright (C) 2010-2014 Wuhan ANTS Ltd.\n");
    printf("Brief: Upgrade flash with specified file.\n");
    printf("Usage: %s filename [logfile]\n", appName);
}

void handlesignal(int signo)
{
    printf("catch signal %d\n", signo);
    if (signo == SIGUSR1)
    {
    }
}

extern int ExHand_Init(const char* printFile, const int fileCount);
int main(const int argc, const char* argv[])
{
    int ret = 0;
    if (argc < 2)
    {
        PrintHelp(argv[0]);
        return -1;
    }
    
    signal(SIGUSR1, handlesignal);
    if (argc >= 3)
    {
        snprintf(s_logFile, sizeof(s_logFile), "%s", argv[2]);
    }

    //ExHand_Init("/usr/etc/flashup.log", 4);
    
    RawPartUpgradeFileHeader_T rpuFileHeader;
    ret = JudgeRawPartUpgradeFileHeader(argv[1], &rpuFileHeader);
    if (ret > 0)
    {
        printf("Invliad RawPartUpgradeFile.\n");
    }
    else if (0 == ret)
    {
        printf("File format: RawPartUpgradeFile.\n");
        return UpgradeRawPartUpgradeFile(argv[1], &rpuFileHeader);
    }

    UpdateParam_T updateParam;
    memset(&updateParam, 0, sizeof(updateParam));
    ret = JudgeUpdateFile(argv[1], &updateParam);
    if (0 == ret)
    {
        printf("File format: Update File.\n");
        return UpgradeUpdateFile(argv[1], &updateParam);
    }
    else if (ret > 0)
    {
        printf("Invliad Update File.\n");
    }

    ret = JudgeAllFlashFile(argv[1]);
    if (0 == ret)
    {
        printf("File format: Flash Image File.\n");
        UpgradeAllFlashFile(argv[1]);
    }
    else if (ret > 0)
    {
        printf("Invliad Flash File.\n");
    }
    else
    {
        printf("Unknown File Format.\n");
    }
    
    return 0;
}

