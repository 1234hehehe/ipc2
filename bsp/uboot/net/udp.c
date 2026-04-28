/*
 * DNS support driver
 *
 * Copyright (c) 2008 Pieter Voorthuijsen <pieter.voorthuijsen@prodrive.nl>
 * Copyright (c) 2009 Robin Getz <rgetz@blackfin.uclinux.org>
 *
 * This is a simple DNS implementation for U-Boot. It will use the first IP
 * in the DNS response as NetServerIP. This can then be used for any other
 * network related activities.
 *
 * The packet handling is partly based on TADNS, original copyrights
 * follow below.
 *
 */

/*
 * Copyright (c) 2004-2005 Sergey Lyubka <valenok@gmail.com>
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Sergey Lyubka wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <u-boot/md5.h>

#include "udp.h"

DECLARE_GLOBAL_DATA_PTR;
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define HTTP_STATE_STATUSLINE 0
#define HTTP_STATE_HEADERS    1
#define HTTP_STATE_DATA       2
#define HTTP_STATE_CLOSE      3

#define HTTPFLAG_NONE          0
#define HTTPFLAG_REQ_GET       1
#define HTTPFLAG_REQ_PUT       2
#define HTTPFLAG_REQ_POST      3
#define HTTPFLAG_REQ_DEL       4
#define HTTPFLAG_RESPONCE      5

#define GET_UPDATE                     1
#define GET_DISCOVERY                  2
#define GET_TRANSMODE                  3
#define PUT_NETCONFIG                  4
#define POST_UPDATE_START              5
#define DEL_UPDATE                     6
#define GET_UPDATE_INFO                7
#define PUT_UPDATE_PREPARE             8
#define PUT_UPDATE_REBOOT              9

#define DATATYPE_NOJSON              0
#define DATATYPE_JSON                1

#define ISO_nl       0x0a
#define ISO_cr       0x0d
#define ISO_space    0x20

//#define TFTP_LOAD_ADDR  0x82000000
#define UPDATEFILEHEADERMAGIC 0x41555046  //"AUPF"
#define UPDATEPACKAGEHEADERMAGIC 0x5350502A // "SPP*"

#define UPDATEFILEHEADERRPUMAGIC 0x55505241//"URPA"

typedef struct
{
    int state;
    int httpflag;
    int action;
    int datalen;
    int datatype;
} http_info_t;

typedef struct
{
    int nPartial; // 0-Normal,1-准备升级,2-传输数据,3-校验数据,4-擦除flash,5-写数据,6-更新配置,7-升级完成
    int nProgress;
    int nPartProgress;
    int nStatus; // 0-正常,-1-失败，-2-取消、中断
} update_status_t;

typedef struct
{
    int   nStatus;
    int   nTransMode;
    char  FileName[256];
    int   nFileSize;
    char  nMd5sum[64];
    int   nUpdatID;
    update_status_t nUpdateStatus;
} update_info_t;

#pragma pack(push,1)

typedef enum
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
    UpdateType_PartName = 20, //  通过分区名升级
	UpdateType_PartIndex , //  0-无效分区，1-第一分区,2-第二分区
	UpdateType_Flash , // 升级整个flash
    UpdateType_Butt,
} UpdateType_T;

typedef enum
{
    UpdateCheckType_MD5 = 0,
    UpdateCheckType_BUTT
} UpdateCheckType_T;

typedef struct
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
} UpdateFileHeader_T;

typedef struct
{
    unsigned int uMagicNumber;// "SPP*" single part package
    unsigned int uPackageType;
    unsigned int uVersion;// 0- ignore
    unsigned int uRequireVersion;// 0- ignore
    unsigned int uPackageLength;// not include header
    unsigned int uDependPackage;// package type bits mark
    unsigned int uPackageCheckType;// 0- MD5
    unsigned int uPackageCheckValue[4];
    unsigned int uExtPackHeaderLengh; // 4??
	union{
	unsigned int uRes[2];
	unsigned int uPartIndx;
	};
} UpdatePackageHeader_T;

#if 1 //ARPU

#define MAX_MERGE_BOARDID_COUNT 16
#define MAX_MERGE_PART_COUNT 16

typedef struct
{
    unsigned int fileStartKB;  //éy?????t′ò°üμ??·
    unsigned int fileSizeKB;   //éy?????t′óD?(kB)
    unsigned int partStartKB;  //éy?????tFlashμ??·
    unsigned int partSizeKB;   //2á3y′óD?(kB)
    char partChecksum[16];
} RawPartUpgradeFilePart_T;

typedef struct RawPartUpgradeFileHeader_T
{
    unsigned int uMagicNumber;// "ARPU"-ants raw partition update.
    unsigned int headSize; // header3¤?è,°üà¨magic number?ú?ú.
    unsigned char headChecksum[16]; // ????D￡?é?μ
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
#endif
#pragma pack(pop)

static http_info_t http_info;
static update_info_t update_info;
static unsigned int m_dwBoardType = 0;
static int bready = 0;


static int
FindStr(char *data, char *str, char *pvalue)
{
    char *tmp = NULL;
    int pos = 0, end = 0;

    if (NULL == data || NULL == str)
    {
        return -1;
    }

    tmp = strstr(data, str);
    if (tmp != NULL)
    {
        pos = tmp + strlen(str) - data;
        tmp = strchr(data + pos, '\"');
        if (tmp != NULL)
        {
            pos = tmp - data + 1;
            tmp = strchr(data + pos, '\"');
            if (tmp != NULL)
            {
                end = tmp - data;
                if (end > pos)
                {
                    if (pvalue != NULL)
                    {
                        strncpy(pvalue, data + pos, end - pos);
                        pvalue[end - pos] = 0;
                    }

                    return 0;
                }
            }
        }
    }

    return -1;
}

static int
FindNum(char *data, char *str, long long *pvalue)
{
    char *tmp = NULL;
    int pos = 0;
    char c;

    if (NULL == data || NULL == str)
    {
        return -1;
    }

    tmp = strstr(data, str);
    if (tmp != NULL)
    {
        pos = tmp + strlen(str) - data;
        tmp = strchr(data + pos, ',');
        if (tmp == NULL)
        {
            tmp = strchr(data + pos, '}');
        }

        if (tmp != NULL)
        {
            c = data[pos];
            while (c == ISO_space)
            {
                pos++;
                c = data[pos];
            }

            if (NULL != pvalue)
            {
                *pvalue = simple_strtol(data + pos, NULL, 10);
                return 0;
            }
        }
    }

    return -1;
}

static int
get_part_sizeandaddr(const char* part, unsigned long long* size, unsigned long long* addr)
{
    const char* bootargs = getenv("bootargs");
    char str_mtd[256] = "";
    char* p = NULL;
    char* q = NULL;
    int partcount = 0;
    unsigned long long partaddr = 0;
    unsigned long long partsize = 0;

    p = strstr(bootargs, "mtdparts");
    if (p == NULL)
    {
    		p = strstr(bootargs, "blkdevparts");
    		if(p == NULL)
        	return -1;
    }

    q = strchr(p, ' ');
    if (q != NULL)
    {
        strncpy(str_mtd, p, MIN(q-p, sizeof(str_mtd) - 1));
    }
	else
	{
		strncpy(str_mtd, p, MIN(strlen(p)+1, sizeof(str_mtd) - 1));
	}
	str_mtd[255] = 0;

    // "mtdparts=hi_sfc:320K(uboot),2752K(kernel),3M(rom),8960K(app),640K(config),640K(log)"
    p = strstr(str_mtd, "hi_sfc:");
    if (NULL == p)
    {
    		p = strstr(str_mtd, "hinand:");
    		if(NULL == p)
    		{
					p = strstr(str_mtd, "mmcblk0:");
					if(NULL == p)
						return -1;
					else
						p+=8;
    		}
    		else
    		{
    				p+=7;
    		}        
    }
    else
    {
        p += 7;
    }

    for (; ;)
    {
        // Get part size
        partsize = simple_strtoull(p, NULL, 10) * 1024;
        
        p = strchr(p, '(');
        if (p && (*(p-1) == 'M' || *(p-1) == 'm'))
        {
            partsize *= 1024; // MB to KB
            printf("MB to KB partsize=%d\n",partsize);
        }

        // Get part name
        p++;
        char* partname = p;
        p = strchr(p, ')');
        if (p)
        {
            p[0] = '\0';
            if (strcmp(partname, part) == 0)
            {
                *addr = partaddr;
                *size = partsize;               
                return 0;
            }
            p[0] = ')';
        }
        partcount++;
        partaddr += partsize;

        p = strchr(p, ',');
        if (NULL == p)
            return -1;
        else
            p++;
    }
    printf("return size:%llu addr=%#llx\n",size,addr);
}

static void
UdpSend(uchar *ether, IPaddr_t dest, int dport, int sport, char *data, int len)
{
    volatile uchar *pkt;
    int i = 0;

    pkt = (uchar *)(NetTxPacket + NetEthHdrSize() + IP_UDP_HDR_SIZE);
    memcpy((void*)pkt, (const char*)data, len);

    //printf("UdpSendPort = %d, NetUdpListenPort = %d data = %s len = %d\n",dport,sport,data,len);
   // for (i = 0; i < 6; i++)
   // {
   //     printf("ether[%d] = %x\n",i,ether[i]);
   // }

    NetSendUDPPacket(ether, dest, dport, sport, len);
}

static void
UdpTimeout(void)
{
	//puts("Timeout\n");
	//net_state = NETLOOP_RESTART;
	printf("bready = %d\n", bready);
	if (bready == 0)
	{
		printf("Timeout\n");
		//eth_halt();
		net_state_ex = NETLOOP_FAIL;
	}
}

static int 
send_progress(uchar *ether, IPaddr_t dest, int dport, int sport)
{
    char json_data[512];
    char buf[1024];
	int json_len = 0;
	
    snprintf(json_data, sizeof(json_data), "{\"Partial\":%d,\"Progress\":%d,\"PartProgress\":%d,\"Status\":%d}", update_info.nUpdateStatus.nPartial, update_info.nUpdateStatus.nProgress,
		                                                                                                         update_info.nUpdateStatus.nPartProgress, update_info.nUpdateStatus.nStatus);
    json_len = strlen(json_data) + 1;
    snprintf(buf, 1024 , "Put /Update/%d/Status HTTP/1.1\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s", update_info.nUpdatID, json_len, json_data);
    printf("%s\n", buf);

	UdpSend(ether, dest, dport, sport, buf, strlen(buf) + 1);

	return 0;
}	

#if 1
static int
do_updaterpu_sf(char *pBuffer, uchar *ether, IPaddr_t dest, int dport, int sport)
{
	int i;
    RawPartUpgradeFileHeader_T *rpuFileHeader;
	const char* bootargs = getenv("bootargs");
    int bNandFlash=(NULL != strstr(bootargs, "hinand:"));
    int bEmmc=(NULL != strstr(bootargs, "mmcblk0:"));
    char tmpChecksum[16] = {0};

    rpuFileHeader = (RawPartUpgradeFileHeader_T *)pBuffer;
    if(rpuFileHeader == NULL)
    {
        printf("Updata packed pbuff = NULL\n");
        return -1;
    }

	//文件头校验
    memcpy(tmpChecksum, rpuFileHeader->headChecksum, sizeof(rpuFileHeader->headChecksum));
    memset(rpuFileHeader->headChecksum, 0, sizeof(rpuFileHeader->headChecksum));
    md5(pBuffer, rpuFileHeader->headSize, rpuFileHeader->headChecksum);
    if (0 != memcmp(tmpChecksum, rpuFileHeader->headChecksum, sizeof(rpuFileHeader->headChecksum)))
    {
        printf("Check RPUF header failed.\n");
        return -1;
    }
/*
    lRet = checkRpuPartData(pBuffer);//分段校验
    if (0 != lRet)
    {
        print("Check RPUF part data failed.\n");
        return -1;
    }
*/
    if(m_dwBoardType != 0)
    {
        // 是否支持板型
        for(i = 0; i < MAX_MERGE_BOARDID_COUNT; i++)
        {
            if(rpuFileHeader->boardid[i] == m_dwBoardType)
            {
                break;
            }
        }
        if(i == MAX_MERGE_BOARDID_COUNT)
        {
            printf("Updata package boardtype mismatch device\n");
            return -1;
        }
    }
    for (i = 0; i < rpuFileHeader->partCount; i++)
	{
		char strcmd[64];
        int readOffset = rpuFileHeader->headSize + rpuFileHeader->partInfo[i].fileStartKB * 1024;
		if(bEmmc == 0)
		{
			if(bNandFlash)
			{
				sprintf(strcmd, "nand erase %x %x", rpuFileHeader->partInfo[i].partStartKB * 1024, rpuFileHeader->partInfo[i].partSizeKB * 1024);
				printf("strcmd=%s\n", strcmd);
				run_command(strcmd, 0);

				sprintf(strcmd, "nand write %x %x %x", pBuffer + readOffset, rpuFileHeader->partInfo[i].partStartKB * 1024, rpuFileHeader->partInfo[i].fileSizeKB * 1024);
				printf("strcmd=%s\n", strcmd);
				run_command(strcmd, 0);
			}
			else
			{
				run_command("sf probe 0", 0);
				sprintf(strcmd, "sf erase %x %x", rpuFileHeader->partInfo[i].partStartKB * 1024, rpuFileHeader->partInfo[i].partSizeKB * 1024);
				printf("strcmd=%s\n", strcmd);
				run_command(strcmd, 0);

				sprintf(strcmd, "sf write %x %x %x", pBuffer + readOffset, rpuFileHeader->partInfo[i].partStartKB * 1024, rpuFileHeader->partInfo[i].fileSizeKB * 1024);
				printf("strcmd=%s\n", strcmd);
				run_command(strcmd, 0);
			}
		}
		else
		{
				sprintf(strcmd, "mmc erase %x %x", rpuFileHeader->partInfo[i].partStartKB * 2, rpuFileHeader->partInfo[i].partSizeKB * 2);
				printf("strcmd=%s\n", strcmd);
				run_command(strcmd, 0);

				sprintf(strcmd, "mmc write 0 %x %x %x", pBuffer + readOffset, rpuFileHeader->partInfo[i].partStartKB * 2, rpuFileHeader->partInfo[i].fileSizeKB * 2);
				printf("strcmd=%s\n", strcmd);
				run_command(strcmd, 0);
		}

		update_info.nUpdateStatus.nPartial = 5;
		update_info.nUpdateStatus.nProgress = 50 + (i + 1) * 50 / rpuFileHeader->partCount;
		update_info.nUpdateStatus.nPartProgress = 100;
		send_progress(ether, dest, dport, sport);			
	}
	printf("update rpu ok (sf)\n");

    update_info.nUpdateStatus.nPartial = 7;
	update_info.nUpdateStatus.nProgress = 100;
    update_info.nUpdateStatus.nPartProgress = 100;
	return 0;
}
#endif
static int
do_update_sf(char *pBuffer, uchar *ether, IPaddr_t dest, int dport, int sport)
{
    UpdateFileHeader_T *pFileheader;
    UpdatePackageHeader_T *pPackage;
    int nPackNum = 0, i = 0;
    unsigned int BoardType, *pIntArray;
    unsigned int m_md5_result[4];
    unsigned long long m_uTotalUpdateSize = 0;
    unsigned long long m_uCurrUpdatedSize = 0;
    int nMtdIndex = -1, ret = -1;
    unsigned long long partsize = 0;
    unsigned long long partaddr = 0;
	char *pMtdName = NULL;
	unsigned long long nMtdPackOffset = 0;
	const char* bootargs = getenv("bootargs");
    int bNandFlash=(NULL != strstr(bootargs, "hinand:"));
    int bEmmc=(NULL != strstr(bootargs, "mmcblk0:"));
    
    pFileheader = (UpdateFileHeader_T *)pBuffer;
    if(pFileheader == NULL)
    {
        printf("Updata packed pbuff = NULL\n");
        return -1;
    }

    if(pFileheader->uMagicNumber != UPDATEFILEHEADERMAGIC)
    {
        printf("Updata packed err : magic = 0x%x", pFileheader->uMagicNumber);
        return -1;
    }

    if(pFileheader->uPackageNum < 1 || pFileheader->uPackageNum > 32)
    {
        printf("Updata package num err : num = %d", pFileheader->uPackageNum);
        return -1;
    }

    if(pFileheader->uPackageTypeMark == 0)
    {
        printf("Updata package type mark err : mark = 0x%x", pFileheader->uPackageTypeMark);
        return -1;
    }

    if(pFileheader->uSupportBoardNum < 1 || pFileheader->uSupportBoardNum > 256)
    {
        printf("Updata package boardnum  err : num = %d", pFileheader->uSupportBoardNum);
        return -1;
    }

    pIntArray = (unsigned int *)(pBuffer + pFileheader->uSupportBoardTypesTable_Offset);
    if(m_dwBoardType != 0)
    {
        // 是否支持板型
        for(i = 0; i < pFileheader->uSupportBoardNum; i++)
        {
            BoardType = pIntArray[i];
            if(BoardType == m_dwBoardType)
            {
                break;
            }
        }
        if(i == pFileheader->uSupportBoardNum)
        {
            printf("Updata package boardtype mismatch device\n");
            return -1;
        }
    }

    pIntArray = (unsigned int *)(pBuffer + pFileheader->uPackageOffsetsTable_Offset);
    for(nPackNum = 0; nPackNum < pFileheader->uPackageNum; nPackNum++)
    {
        pPackage = (UpdatePackageHeader_T *)(pBuffer + pIntArray[nPackNum]);
        if(pPackage->uMagicNumber != UPDATEPACKAGEHEADERMAGIC)
        {
            printf("Updata package %d  invalid", nPackNum);
            return -1;
        }

        if(pPackage->uPackageLength > 2 * 1024 * 1024 * 1024)
        {
            printf("Updata package len too long %d  invalid", pPackage->uPackageLength);
            return -1;
        }

        if(pPackage->uPackageType == UpdateType_File && pPackage->uExtPackHeaderLengh > pPackage->uPackageLength)
        {
            printf("Updata package file path len too long %d	invalid", pPackage->uExtPackHeaderLengh);
            return -1;
        }

        if(pPackage->uPackageType == UpdateType_Tar && pPackage->uExtPackHeaderLengh > pPackage->uPackageLength)
        {
            printf("Updata package file path len too long %d	invalid", pPackage->uExtPackHeaderLengh);
            return -1;
        }

        if(pPackage->uPackageCheckType == UpdateCheckType_MD5)
        {
        	if(pPackage->uPackageLength > 0)
        	{
	            md5(((unsigned char *)pPackage + sizeof(UpdatePackageHeader_T)),pPackage->uPackageLength,(unsigned char *)m_md5_result);
	            if(m_md5_result[0] != pPackage->uPackageCheckValue[0] ||
	               m_md5_result[1] != pPackage->uPackageCheckValue[1] ||
	               m_md5_result[2] != pPackage->uPackageCheckValue[2] ||
	               m_md5_result[3] != pPackage->uPackageCheckValue[3])
	            {
	                printf("Updata package [%d]  check failed %x%x%x%x", nPackNum,m_md5_result[0],m_md5_result[1],m_md5_result[2],m_md5_result[3]);
	                return -1;
	            }
        	}
        }

        m_uTotalUpdateSize += pPackage->uPackageLength;
    }

    if (m_uTotalUpdateSize > update_info.nFileSize)
    {
        return -1;
    }

    pIntArray = (unsigned int *)(pBuffer + pFileheader->uPackageOffsetsTable_Offset);
    for(nPackNum = 0; nPackNum < pFileheader->uPackageNum; nPackNum++)
    {
        pPackage = (UpdatePackageHeader_T *)(pBuffer + pIntArray[nPackNum]);
        if(pPackage->uMagicNumber != UPDATEPACKAGEHEADERMAGIC)
        {
            printf("Updata package %d  invalid", nPackNum);
            return -1;
        }
		pMtdName = NULL;
		nMtdIndex = -1;
		nMtdPackOffset = 0;

        printf("package [%d] type = %d\n",nPackNum,pPackage->uPackageType);
        switch(pPackage->uPackageType)
        {
            case UpdateType_Uboot:
                {
                    nMtdIndex = 0;
                    break;
                }
            case UpdateType_Kernel:
                {
                    nMtdIndex = 3;
                  
                    break;
                }
            case UpdateType_Rfs:
                {
                    nMtdIndex = -1;
                   
                    break;
                }
            case UpdateType_App:
                {
                    nMtdIndex = 4;
                    
                    break;
                }
            case UpdateType_Config:
                {
                    nMtdIndex = 2;
                    
                    break;
                }
			case UpdateType_PartIndex:
				{
					nMtdIndex = pPackage->uPartIndx;
					nMtdIndex--;
					break;
				}
			case UpdateType_PartName:
				{
					nMtdIndex = -1;
					
					if (pPackage->uExtPackHeaderLengh > 0)
					{
						pMtdName = ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T);
						nMtdPackOffset = pPackage->uExtPackHeaderLengh;
					}
					
					break;
				}
            default:
                {
                    nMtdIndex = -1;
                    break;
                }
        }
		
        m_uCurrUpdatedSize += pPackage->uPackageLength;
        ret = -1;

        if (0 == nMtdIndex)
        {
            ret = get_part_sizeandaddr("uboot", &partsize, &partaddr);
            printf("update uboot!!!!\n");
        }
        else if (1 == nMtdIndex)
        {
            // ret = get_part_sizeandaddr("factory", &partsize, &partaddr);
            ret = -1;
            printf("update factory!!!!\n");
        }
        else if (2 == nMtdIndex)
        {
            ret = get_part_sizeandaddr("config", &partsize, &partaddr);
            printf("update config parsize=%llu partaddr=%d !!!!\n",partsize,partaddr);
        }
        else if (3 == nMtdIndex)
        {
            ret = get_part_sizeandaddr("kernel", &partsize, &partaddr);
            printf("update kernel parsize=%llu partaddr=%d !!!!\n",partsize,partaddr);
        }
        else if (4 == nMtdIndex)
        {
            ret = get_part_sizeandaddr("custom", &partsize, &partaddr);
            printf("update custom parsize=%llu partaddr=%d !!!!\n",partsize,partaddr);
        }
		else if (5 == nMtdIndex)
        {
            ret = get_part_sizeandaddr("smart", &partsize, &partaddr);
            printf("update smart parsize=%llu partaddr=%d !!!!\n",partsize,partaddr);
        }
		else if(pMtdName != NULL)
		{
			if(0 == strcmp(pMtdName,"factory"))
			{
				ret = -1;
			}
			else
			{
			ret = get_part_sizeandaddr(pMtdName, &partsize, &partaddr);
			}
			printf("update %s!!!!\n",pMtdName);
		}

        if (0 == ret && pPackage->uPackageLength - nMtdPackOffset > 0)
        {
            char strcmd[64] = "";
            
						if(bEmmc == 0)
						{
							if(bNandFlash)
							{
								sprintf(strcmd, "nand erase %llx %llx", partaddr, partsize);
							}
							else
							{
								run_command("sf probe 0", 0);
								sprintf(strcmd, "sf erase %llx %llx", partaddr, partsize);
							}
							printf("strcmd=%s\n", strcmd);
							run_command(strcmd, 0);
						}// emmc not erase
						
            update_info.nUpdateStatus.nPartial = 4;
			update_info.nUpdateStatus.nProgress = 50 + m_uCurrUpdatedSize * 25 / m_uTotalUpdateSize;
		    update_info.nUpdateStatus.nPartProgress = 100;
		    send_progress(ether, dest, dport, sport);
		    
			if(bEmmc == 0)
			{
				if(bNandFlash)
				{
					if(nMtdIndex == 0 || nMtdIndex == 3)
					{
					// uboot or kernel
					//sprintf(strcmd, "nand write %x %x %x", ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + nMtdPackOffset, partaddr, pPackage->uPackageLength - nMtdPackOffset);
					sprintf(strcmd, "nand write %x %llx %llx", ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + nMtdPackOffset, partaddr, partsize);
					}
					else
					{
						// others
						sprintf(strcmd, "nand write.yaffs %x %llx %x", ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + nMtdPackOffset, partaddr, pPackage->uPackageLength - nMtdPackOffset);
					}
				}
				else
				{
				sprintf(strcmd, "sf write %x %llx %x", ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + nMtdPackOffset, partaddr, pPackage->uPackageLength - nMtdPackOffset);
				}
			}
			else // emmc
			{
					if(nMtdIndex == 0 || nMtdIndex == 3)
					{		
						//sprintf(strcmd, "mmc write 0 42000000 %x %x", partaddr/512, partsize/512);		
						sprintf(strcmd, "mmc write 0 %x %llx %llx", ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + nMtdPackOffset, partaddr/512, partsize/512);
					}
					else
					{
						// others
						//sprintf(strcmd, "mmc write.ext4sp 0 42000000 %x %x", partaddr/512, partsize/512);			
						sprintf(strcmd, "mmc write.ext4sp 0 %x %llx %llx", ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + nMtdPackOffset, partaddr/512, partsize/512);
					}				
			}			
            printf("strcmd=%s\n", strcmd);
            run_command(strcmd, 0);

            update_info.nUpdateStatus.nPartial = 5;
			update_info.nUpdateStatus.nProgress = 50 + m_uCurrUpdatedSize * 50 / m_uTotalUpdateSize;
		    update_info.nUpdateStatus.nPartProgress = 100;
		    send_progress(ether, dest, dport, sport);			
        }
    }

    update_info.nUpdateStatus.nPartial = 7;
	update_info.nUpdateStatus.nProgress = 100;
    update_info.nUpdateStatus.nPartProgress = 100;
	
    return 0;
}

static int
do_update_all(uchar *ether, IPaddr_t dest, int dport, int sport)
{
    //extern ulong upload_size;
    int i = 0, ret = -1;
    long long fileSize = 0;
    u8 output[16];
    char nMd5sum[64];

    if (0 == update_info.nStatus)
    {
        return 0;
    }

    if (0 == update_info.nUpdateStatus.nPartial)
    {
        update_info.nUpdateStatus.nPartial = 1;

        //load_addr = TFTP_LOAD_ADDR;
				getenv_hex("fileaddr",&load_addr);
        memset((void *)load_addr, 0xff, update_info.nFileSize);
        copy_filename(BootFile, update_info.FileName, sizeof(BootFile));
        //upload_size = 0;

        update_info.nUpdateStatus.nPartial = 2;

		send_progress(ether, dest, dport, sport);
    }

    if (2 == update_info.nUpdateStatus.nPartial)
    {
        fileSize = NetLoop(TFTPGET);
        getenv_hex("filesize",&fileSize);
        getenv_hex("fileaddr",&load_addr);
        net_state = NETLOOP_CONTINUE;

        eth_halt();
#ifdef CONFIG_NET_MULTI
        eth_set_current();
#endif
        eth_init(gd->bd);
        //eth_init();

		update_info.nUpdateStatus.nProgress = 50;
		update_info.nUpdateStatus.nPartProgress = 100;
		send_progress(ether, dest, dport, sport);
		
        update_info.nUpdateStatus.nPartial = 3;
    }

    if (3 == update_info.nUpdateStatus.nPartial)
    {
        if (update_info.nFileSize != fileSize)
        {
            printf("file size %d \n", fileSize);
            return -1;
        }

        flush_cache(load_addr, fileSize);

        md5((unsigned char *)load_addr, fileSize, output);

        for (i = 0; i < 16; i++)
        {
            printf("%02x", output[i]);
            sprintf(nMd5sum + i * 2, "%02x", output[i]);
        }

        printf("\n");

        if (0 != strncasecmp(nMd5sum, update_info.nMd5sum, 32))
        {
            printf("update_info.nMd5sum = %s\n", update_info.nMd5sum);
            return -1;
        }

		update_info.nUpdateStatus.nPartProgress = 100;
		send_progress(ether, dest, dport, sport);		
    }
	
	int magic = *((int *)load_addr);
	if(magic == UPDATEFILEHEADERMAGIC)
		ret = do_update_sf((char *)load_addr, ether, dest, dport, sport);
	else if(magic == UPDATEFILEHEADERRPUMAGIC)
		ret = do_updaterpu_sf((char *)load_addr, ether, dest, dport, sport);
	else
	{
		printf("file head failed : magic = 0x%08x\n", magic);
		ret = -1;
	}

    return ret;
}

static int
parse_statusline(uchar *data, int len)
{
    printf("parse_statusline len = %d\n", len);
    int left_len = -1;
    int pos = 0;
    char *tmp = NULL;

    if (len < 7 || NULL == data)
    {
        http_info.httpflag = HTTPFLAG_NONE;
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }

    if (0 == strncasecmp((char *)data, "GET ", strlen("GET ")))
    {
        http_info.httpflag = HTTPFLAG_REQ_GET;
        pos += strlen("GET ");
    }
    else if (0 == strncasecmp((char *)data, "PUT ", strlen("PUT ")))
    {
        http_info.httpflag = HTTPFLAG_REQ_PUT;
        pos += strlen("PUT ");
    }
    else if (0 == strncasecmp((char *)data, "POST ", strlen("POST ")))
    {
        http_info.httpflag = HTTPFLAG_REQ_POST;
        pos += strlen("POST ");
    }
    else if (0 == strncasecmp((char *)data, "DELETE ", strlen("DELETE ")))
    {
        http_info.httpflag = HTTPFLAG_REQ_DEL;
        pos += strlen("DELETE ");
    }
    else if (0 == strncasecmp((char *)data, "HTTP/", strlen("HTTP/")))
    {
        http_info.httpflag = HTTPFLAG_RESPONCE;
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }
    else
    {
        http_info.httpflag = HTTPFLAG_NONE;
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }

    tmp = strchr((char *)(data + pos), ' ');
    if (NULL == tmp)
    {
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }

    left_len = tmp - ((char *)(data + pos));
    if (left_len <= 0)
    {
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }
    printf("left_len = %d!!!!!!!!!\n",left_len);
    if (0 == strncasecmp((char *)(data + pos), "/Update", left_len) &&
        http_info.httpflag == HTTPFLAG_REQ_GET)
    {
        http_info.action = GET_UPDATE;
        printf("/Update\n");
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/Discovery", left_len) &&
             http_info.httpflag == HTTPFLAG_REQ_GET)
    {
        http_info.action = GET_DISCOVERY;
        printf("/Update/Discovery\n");
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/TransMode", left_len) &&
             http_info.httpflag == HTTPFLAG_REQ_GET)
    {
        http_info.action = GET_TRANSMODE;
        printf("/Update/TransMode\n");
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/NetConfig", left_len) &&
             http_info.httpflag == HTTPFLAG_REQ_PUT)
    {
        http_info.action = PUT_NETCONFIG;
        printf("/Update/NetConfig\n");
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/Start", left_len) &&
             http_info.httpflag == HTTPFLAG_REQ_POST)
    {
        http_info.action = POST_UPDATE_START;
        printf("/Update/Start\n");
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/Updating/", strlen("/Update/Updating/")) &&
             http_info.httpflag == HTTPFLAG_REQ_DEL)
    {
        http_info.action = DEL_UPDATE;
        printf("/Update/Updating/ DEL %d\n",
               simple_strtol((char *)(data + pos + strlen("/Update/Updating/")), NULL, 10));
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/Updating/", strlen("/Update/Updating/")) &&
             http_info.httpflag == HTTPFLAG_REQ_GET)
    {
        http_info.action = GET_UPDATE_INFO;
        printf("/Update/Updating/ GET %d\n",
               simple_strtol((char *)(data + pos + strlen("/Update/Updating/")), NULL, 10));
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/Prepare", left_len) &&
             http_info.httpflag == HTTPFLAG_REQ_PUT)
    {
        http_info.action = PUT_UPDATE_PREPARE;
        printf("/Update/Prepare\n");
    }
    else if (0 == strncasecmp((char *)(data + pos), "/Update/Reboot", left_len) &&
             http_info.httpflag == HTTPFLAG_REQ_PUT)
    {
        http_info.action = PUT_UPDATE_REBOOT;
        printf("/Update/Reboot\n");
    }
    else
    {
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }

    tmp = strstr((char *)(data + pos), "\r\n");
    if (NULL == tmp)
    {
        http_info.state = HTTP_STATE_DATA;

        return 0;
    }

    left_len = len - (tmp + strlen("\r\n") - (char *)data);
    printf("left_len1 = %d!!!!!!!!!\n",left_len);
    if (left_len > 0)
    {
        http_info.state = HTTP_STATE_HEADERS;
    }
    else
    {
        http_info.state = HTTP_STATE_DATA;
    }

    return left_len;
}

static int
parse_headers(uchar *data, int len)
{
    printf("parse_headers len = %d \n", len);
    int pos = 0;
    int left_len = 0;
    char *tmp = NULL;

    if (NULL == data)
    {
        http_info.state = HTTP_STATE_CLOSE;

        return 0;
    }

    if (len < strlen("\r\n"))
    {
        http_info.state = HTTP_STATE_DATA;

        return 0;
    }

    if (0 == strncmp((char *)data, "\r\n", strlen("\r\n")))
    {
        http_info.datalen = 0;
        http_info.datatype = DATATYPE_NOJSON;
    }
    else
    {
        tmp = strstr((char *)data, "\r\n\r\n");
        if (NULL == tmp)
        {
            left_len = len;
        }
        else
        {
            left_len = tmp + strlen("\r\n\r\n") - (char *)data;
        }


		while (1)
		{
			if (pos >= left_len)
			{
				break;
			}

			if (0 == strncasecmp((char *)(data + pos), "Content-Length: ", strlen("Content-Length: ")))
			{
				http_info.datalen = simple_strtol((char *)(data + pos + strlen("Content-Length: ")), NULL, 10);
				printf("http_info.datalen = %d\n", http_info.datalen);
			}
			else if (0 == strncasecmp((char *)(data + pos), "Content-Type: application/json;", strlen("Content-Length: application/json;")))
			{
				http_info.datatype = DATATYPE_JSON;
			}
			else if (0 == strncasecmp((char *)(data + pos), "UbootMode: ", strlen("UbootMode: ")))
			{
				int iEnableUboot  = simple_strtol((char *)(data + pos + strlen("UbootMode: ")), NULL, 10);
				if(iEnableUboot)
				{
					printf("discovery bready = %d\n", bready);
					bready = 1;
				}
			}

			tmp = strstr((char *)(data + pos), "\r\n");
			if (NULL == tmp)
			{
				http_info.state = HTTP_STATE_DATA;

				return 0;
			}
			else
			{
				pos = tmp + strlen("\r\n") - (char *)data;
			}
		}
	}

    http_info.state = HTTP_STATE_DATA;
    left_len = len - left_len;

    return left_len;
}

static int
parse_data(uchar *data, int len, char *buf, int *send_len, int *reboot)
{
	printf("parse_data len = %d \n", len);
	char json_data[2048] = {0};
	int json_len = 0, tmp_num = 0;
	char tmp[64];
	IPaddr_t tmpip = 0;

	if (NULL == data || len < 0)
	{
		http_info.state = HTTP_STATE_CLOSE;
		return 0;
	}

	if (send_len != NULL && buf != NULL)
	{
		//strncpy(buf, "shushi", strlen("shushi") + 1);
		//*send_len = strlen("shushi") + 1;

		if (GET_UPDATE == http_info.action)
		{
			snprintf(json_data, sizeof(json_data), "{\"ResList\":[{\"Uri\":\"/Update/Discovery\",\"Label\":\"Discovery\",\"Describe\":\"Broadcast to discovery devices.\"},"
			         "{\"Uri\":\"/Update/NetConfig\",\"Label\":\"NetConfig\",\"Describe\":\"Temporarily modify network information\"},"
			         "{\"Uri\":\"/Update/TransMode\",\"Label\":\"TransMode\",\"Describe\":\"To get TransMode bit mask:b0-Rest,b1-tftpA[active],b2-tftpP[passive],b3-ftpA,b4-ftpP,b5-privA,bit6-privB,bit7-local file\"},"
			         "{\"Uri\":\"/Update/Updating/<Id>\",\"Label\":\"Updating\",\"Describe\":\"Delete:Destory update;Get:get updating status.Put: upload data.\"}]}");
			json_len = strlen(json_data) + 1;
			snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s", json_len, json_data);
			*send_len = strlen(buf) + 1;
		}
		else if (GET_DISCOVERY == http_info.action)
		{
			if(bready)
			{
				int nPos = 0;
				char *pEnvString = NULL;
				nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, "{");
				nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, "\"System\":\"uboot\"");
				pEnvString = getenv("ipaddr");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Ipv4\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("netmask");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Netmask\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("gatewayip");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Gateway\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("ethaddr");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Mac\":\"%s\"",pEnvString);
				}
				nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"UpdatePort\":%d",NetUdpListenPort);
				nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"TranMask\":%d",1 << 1);
				pEnvString = getenv("httpport");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"HttpPort\":%d",simple_strtol(pEnvString,NULL,10));
				}
				pEnvString = getenv("httpsport");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"HttpsPort\":%d",simple_strtol(pEnvString,NULL,10));
				}
				pEnvString = getenv("onvifport");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"OnvifPort\":%d",simple_strtol(pEnvString,NULL,10));
				}
				pEnvString = getenv("rtspenable");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"RtspEnable\":%d",simple_strtol(pEnvString,NULL,10));
				}
				pEnvString = getenv("rtspport");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"RtspPort\":%d",simple_strtol(pEnvString,NULL,10));
				}
				pEnvString = getenv("devicename");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"DeviceName\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("productname");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"ProductName\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("devicetype");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"DeviceType\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("devicemodel");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"DeviceModel\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("serialnumber");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"SerialNumber\":\"%s\"",pEnvString);
				}
				 pEnvString = getenv("deviceuuid");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"UUID\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("devicestatus");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Status\":\"%s\"",pEnvString);
				}
				 pEnvString = getenv("contry");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Country\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("city");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"City\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("web");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Web\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("tel");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Tel\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("copyright");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Copyright\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("manufacturer");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Manufacturer\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("brand");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Brand\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("customer");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Customer\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("sensormodel");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"SensorModel\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("isofdome");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"IsOfDome\":%d",simple_strtol(pEnvString,NULL,10));
				}
				pEnvString = getenv("version");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Version\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("hardversion");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"HardVersion\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("builddate");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"BuildDate\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("productdate");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"ProductDate\":\"%s\"",pEnvString);
				}
				pEnvString = getenv("hardware");
				if(pEnvString != NULL)
				{
					nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Hardware\":\"%s\"",pEnvString);
				}
				nPos += snprintf(json_data + nPos , sizeof(json_data)- nPos, "}");
				json_len = strlen(json_data) + 1;
				snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s", json_len, json_data);
				*send_len = strlen(buf) + 1;
		  }
		}
		else if (GET_TRANSMODE == http_info.action)
		{
			snprintf(json_data, sizeof(json_data), "{\"TransModeMask\":%d}", 1 << 1);
			json_len = strlen(json_data) + 1;
			snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s", json_len, json_data);
			*send_len = strlen(buf) + 1;
		}
        else if (PUT_NETCONFIG == http_info.action)
        {
            if (http_info.datalen > 0 && len > 0)
            {
                if (0 == FindStr((char *)data, "\"Mac\":", tmp))
                {
                	int nPos = 0;
							    char *pEnvString = NULL;
                    printf("MAC = %s\n", tmp);
                    if (0 == strncasecmp(tmp, getenv("ethaddr"), strlen(tmp)))
                    {
                        if (0 == FindStr((char *)data, "\"Ipv4\":", tmp))
                        {
                            printf("Ipv4 = %s\n", tmp);
                            tmpip = string_to_ip(tmp);
                            if (0 != tmpip && NetOurIP != tmpip)
                            {
                                NetOurIP = tmpip;
                                setenv("ipaddr", tmp);
                            }
                        }

                        if (0 == FindStr((char *)data, "\"Netmask\":", tmp))
                        {
                            printf("Netmask = %s\n", tmp);
                            tmpip = string_to_ip(tmp);
                            if (0 != tmpip && NetOurSubnetMask != tmpip)
                            {
                                NetOurSubnetMask = tmpip;
                                setenv("netmask", tmp);
                            }
                        }
                        if (0 == FindStr((char *)data, "\"Gateway\":", tmp))
                        {
                            printf("Gateway = %s\n", tmp);
                            tmpip = string_to_ip(tmp);
                            if (0 != tmpip && NetOurGatewayIP != tmpip)
                            {
                                NetOurGatewayIP = tmpip;
                                setenv("gatewayip", tmp);
                            }
                        }
                        
							        	nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, "{");
							        	nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, "\"System\":\"uboot\"");
							        	pEnvString = getenv("ipaddr");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Ipv4\":\"%s\"",pEnvString);
							          }
							          pEnvString = getenv("netmask");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Netmask\":\"%s\"",pEnvString);
							          }
							        	pEnvString = getenv("gatewayip");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Gateway\":\"%s\"",pEnvString);
							          }
							          pEnvString = getenv("ethaddr");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Mac\":\"%s\"",pEnvString);
							          }
							          
							        
							          pEnvString = getenv("serialnumber");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"SerialNumber\":\"%s\"",pEnvString);
							          }
							          pEnvString = getenv("deviceuuid");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"UUID\":\"%s\"",pEnvString);
							          }
							          pEnvString = getenv("devicestatus");
							        	if(pEnvString != NULL)
							        	{
							        		nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, ",\"Status\":\"%s\"",pEnvString);
							          }
							           
							        
							        	  nPos += snprintf(json_data + nPos, sizeof(json_data) - nPos, "}");

                        json_len = strlen(json_data) + 1;
                        snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s", json_len, json_data);
                        *send_len = strlen(buf) + 1;
                    }
                }
            }
        }
        else if (POST_UPDATE_START == http_info.action)
        {
            if (http_info.datalen > 0 && len > 0 && 0 == update_info.nStatus)
            {
                memset(&update_info, 0, sizeof(update_info));

                if (0 == FindNum((char *)data, "\"TranMask\":", &tmp_num))
                {
                    printf("TranMask = %d\n", tmp_num);
                    if (tmp_num == 1 << 1)
                    {
                        if (0 == FindNum((char *)data, "\"FileSize\":", &tmp_num))
                        {
                            printf("FileSize = %d\n", tmp_num);
                            update_info.nFileSize = tmp_num;
                        }

                        if (0 == FindNum((char *)data, "\"TftpPort\":", &tmp_num))
                        {
                            printf("TftpPort = %d\n", tmp_num);
                            snprintf(tmp, sizeof(tmp), "%d", tmp_num);
                            setenv("tftpdstp", tmp);
                        }

                        if (0 == FindStr((char *)data, "\"TftpServer\":", tmp))
                        {
                            printf("TftpServer = %s\n", tmp);
                            setenv("serverip", tmp);
                            NetServerIP = string_to_ip(getenv("serverip"));
                        }

                        if (0 == FindStr((char *)data, "\"Md5sum\":", tmp))
                        {
                            printf("Md5sum = %s\n", tmp);
                            strcpy(update_info.nMd5sum, tmp);
                        }

                        if (0 == FindStr((char *)data, "\"FilePath\":", tmp))
                        {
                            printf("FilePath = %s\n", tmp);
                            strcpy(update_info.FileName, tmp);
                        }

                        update_info.nUpdatID = get_timer(0);

                        snprintf(json_data, sizeof(json_data), "{\"Uri\":\"/Update/Updating/%d\",\"Timeout\":\"%d\"}",
                                 update_info.nUpdatID, 60);
                        json_len = strlen(json_data) + 1;
                        snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s", json_len, json_data);
                        *send_len = strlen(buf) + 1;

                        update_info.nStatus = 1;
                    }
                }
            }
        }
        else if (PUT_UPDATE_PREPARE == http_info.action)
        {
            if (http_info.datalen > 0 && len > 0)
            {
                if (0 == FindStr((char *)data, "\"Mac\":", tmp))
                {
                    printf("MAC = %s\n", tmp);
                    if (0 == strncasecmp(tmp, getenv("ethaddr"), strlen(tmp)))
                    {
                        bready = 1;
                        snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: 0");
                        *send_len = strlen(buf) + 1;
                    }
                }
            }
            else
            {
                bready = 1;
                snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: 0");
                *send_len = strlen(buf) + 1;
            }
        }
        else if (PUT_UPDATE_REBOOT == http_info.action)
        {
            if (http_info.datalen > 0 && len > 0)
            {
                if (0 == FindStr((char *)data, "\"Mac\":", tmp))
                {
                    printf("MAC = %s\n", tmp);
                    if (0 == strncasecmp(tmp, getenv("ethaddr"), strlen(tmp)))
                    {
                        snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: 0");
                        *send_len = strlen(buf) + 1;

						if (NULL != reboot)
						{
                            *reboot = 1;
						}
                    }
                }
            }
            else
            {
                snprintf(buf, 2048 , "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: 0");
                *send_len = strlen(buf) + 1;

				if (NULL != reboot)
				{
                    *reboot = 1;
				}				
            }
        }
    }

    http_info.state = HTTP_STATE_CLOSE;
    return 0;
}

static void
UdpHandler(uchar *pkt, unsigned dest, unsigned src, unsigned len)
{

}

static void
UdpHandlerEx(uchar *pkt, uchar *src_mac, IPaddr_t src_ip, IPaddr_t dst_ip, unsigned dest, unsigned src, unsigned len)
{
    char ip[16];
    int i = 0, ret = -1, reboot = 0;
    int left_len = 0, send_len = 0;
    char buf[2048];
    uchar srcmac[6];

    if (dest != NetUdpListenPort)
    {
        return;
    }

    pkt[len] = 0;
    printf("len = %d dest = %d src = %d, ip(%pI4->%pI4)\n", len, dest, src, &src_ip, &dst_ip);
#if 0
    ip_to_string(NetReadIP(&src_ip),ip);
    printf("ip = %s\n", ip);
    for (i = 0; i < 6; i++)
    {
        printf("src_mac[%d] = %x\n",i,src_mac[i]);
    }
#endif
    memcpy(srcmac, src_mac, 6);

    memset(&http_info, 0, sizeof(http_info));
    http_info.state = HTTP_STATE_STATUSLINE;

    if (http_info.state == HTTP_STATE_STATUSLINE)
    {
        left_len = parse_statusline(pkt, len);
    }

    if (http_info.state == HTTP_STATE_HEADERS)
    {
        left_len = parse_headers(pkt + (len - left_len), left_len);
    }

	if (http_info.state == HTTP_STATE_DATA)
	{
		left_len = parse_data(pkt + (len - left_len), left_len, buf, &send_len, &reboot);

		if (0 == left_len && send_len > 0)
		{
			if (dst_ip == 0xFFFFFFFF)
			{
				IPaddr_t zerotmp;
				zerotmp = 0;
				UdpSend(NetBcastAddr, zerotmp, src, dest, buf, send_len);
			}
			else
			{
				UdpSend(srcmac, src_ip, src, dest, buf, send_len);
			}
		}

		printf("reboot = %d\n", reboot);
		if (1 == reboot && update_info.nStatus != 2)
		{
		  printf("run_command reset\n");
			run_command("reset", 0);        
		}
	}

	if (update_info.nStatus == 1)
	{
		update_info.nStatus = 2;
		run_command("watchdog 0", 0);        
		if (dst_ip == 0xFFFFFFFF)
		{
			IPaddr_t tmpaddr;
			tmpaddr = 0;
			ret = do_update_all(NetBcastAddr, tmpaddr, src, dest);
		}
		else
		{
			ret = do_update_all(srcmac, src_ip, src, dest);
		}
		printf("do_update_all ret = %d\n", ret);

		if (-1 == ret)
		{
			update_info.nUpdateStatus.nStatus = -1;
		}
		else
		{
			update_info.nUpdateStatus.nStatus = 0;
		}

		if (dst_ip == 0xFFFFFFFF)
		{
			IPaddr_t tmpaddr;
			tmpaddr = 0;	
			send_progress(NetBcastAddr, tmpaddr, src, dest);
		}
		else
		{
			send_progress(srcmac, src_ip, src, dest);
		}

		update_info.nStatus = 0;
		printf("%pI4 http_info.state = %d\n", &src_ip, http_info.state);
		mdelay(3000);
		printf("sleep ok reset\n");
		run_command("reset", 0);
	}
	if (http_info.state == HTTP_STATE_CLOSE)
	{
		return;
	}
	return;
}

void
UdpStart(void)
{
	char *szEnvSaved = NULL;

	memset(&http_info, 0, sizeof(http_info));
	http_info.state = HTTP_STATE_STATUSLINE;

	memset(&update_info, 0, sizeof(update_info));
	bready = 0;
#if 0
	szEnvSaved = getenv("envstatus");
	if(szEnvSaved == NULL || 0 != strncasecmp(szEnvSaved,"saved",strlen("saved")))
	{
		setenv("envstatus","saved");
		//env_crc_update();
		saveenv();
	}
#endif
	net_state_ex = NETLOOP_CONTINUE;
	NetSetTimeout_ex(UDP_TIMEOUT, UdpTimeout);
	//net_set_udp_handler(UdpHandler);
	net_set_udp_handler_ex(UdpHandlerEx);
}
