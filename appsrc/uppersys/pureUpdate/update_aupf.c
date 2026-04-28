//UPGRADE "Ants update packate file"

#include "flashrw.h"
#include "update_aupf.h"
#include "update_common.h"

#define UPDATE_BUFSIZE (10 * 1024)

static AupfUpdateStatusMgr_T s_stAupfUpdateStatus;

static int initAupfUpdateStatus(UpdateSvrMgr* pstUpdateSvrMgr, AupfUpdateStatusMgr_T *pstAupfUpdateStatus)
{
    int lRet = -1;

    if (pstAupfUpdateStatus->m_pMutex)
    {
        (void)Common_Lock_Destroy(&pstAupfUpdateStatus->m_pMutex);
        pstAupfUpdateStatus->m_pMutex = NULL;
    }
    memset((void *)pstAupfUpdateStatus, 0x00, sizeof(*pstAupfUpdateStatus));

    lRet = Common_Lock_Create(&pstAupfUpdateStatus->m_pMutex, "AupfUpdate_lock");
    if (0 != lRet)
    {
        LOGE("Creat Aupf update lock failed.\n");
        return -1;
    }

    pstAupfUpdateStatus->m_hUpgradeFile = pstUpdateSvrMgr->m_hUpgradeFile;
    pstAupfUpdateStatus->m_dwBoardType  = pstUpdateSvrMgr->m_dwBoardType;

    return lRet;
}

static int unInitAupfUpdateStatus(AupfUpdateStatusMgr_T *pstAupfUpdateStatus)
{
    if (pstAupfUpdateStatus->m_pPackageOffsetsTable)
    {
        free(pstAupfUpdateStatus->m_pPackageOffsetsTable);
        pstAupfUpdateStatus->m_pPackageOffsetsTable = NULL;
    }

    if (pstAupfUpdateStatus->m_pUpdateBuff)
    {
        free(pstAupfUpdateStatus->m_pUpdateBuff);
        pstAupfUpdateStatus->m_pUpdateBuff = NULL;
    }

    if (pstAupfUpdateStatus->m_pMutex)
    {
        (void)Common_Lock_Destroy(&pstAupfUpdateStatus->m_pMutex);
        pstAupfUpdateStatus->m_pMutex = NULL;
    }

    memset((void *)pstAupfUpdateStatus, 0x00, sizeof(*pstAupfUpdateStatus));
    return 0;
}

static AupfUpdateStatusMgr_T *getAupfStatusHandle()
{
    return &s_stAupfUpdateStatus;
}

static int checkBoardType(AupfUpdateStatusMgr_T* pstAupfUpdateStatus, UpdateFileHeader_T *pstFileheader)
{
    int i = 0;
    unsigned int BoardType  = 0;
    unsigned int *pIntArray = NULL;

    //获取板型
    pIntArray = (unsigned int *)(pstAupfUpdateStatus->m_pUpdateBuff + pstFileheader->uSupportBoardTypesTable_Offset);
    if(pstAupfUpdateStatus->m_dwBoardType != 0)
    {
        for(i = 0; i < pstFileheader->uSupportBoardNum; i++)
        {
            BoardType = pIntArray[i];
            LOGE("[%d]boardtype = 0x%x\n",i,BoardType);
            if(BoardType == pstAupfUpdateStatus->m_dwBoardType)
            {
                break;
            }
        }

        if(i == pstFileheader->uSupportBoardNum)
        {
            LOGE("Updata package boardtype mismatch device [0x%x]\n", pstAupfUpdateStatus->m_dwBoardType);
            return -1;
        }
    }

    return 0;
}

static int checkAupfBaseInfo(int lDataLen, AupfUpdateStatusMgr_T* pstAupfUpdateStatus)
{
    int lRet = -1;
    UpdateFileHeader_T *pstFileheader = NULL;

    // 此处pFileheader不可能为NULL.
    pstFileheader = (UpdateFileHeader_T *)pstAupfUpdateStatus->m_pUpdateBuff;

    do
    {
        //魔数校验
        if(pstFileheader->uMagicNumber != UPDATE_FILE_HEADER_MAGIC)
        {
            LOGE("Updata packed err : magic = 0x%x\n", pstFileheader->uMagicNumber);
            break;
        }

        //包个数校验
        if(pstFileheader->uPackageNum < 1 || pstFileheader->uPackageNum > 32)
        {
            LOGE("Updata package num err : num = %d\n", pstFileheader->uPackageNum);
            break;
        }

        //包类型掩码校验
        if(0 == pstFileheader->uPackageTypeMark)
        {
            LOGE("Updata package type mark err : mark = 0x%x\n", pstFileheader->uPackageTypeMark);
            break;
        }

        //check board type
        if(pstFileheader->uSupportBoardNum < 1 || pstFileheader->uSupportBoardNum > 256)
        {
            LOGE("Updata package boardnum  err : num = %d\n", pstFileheader->uSupportBoardNum);
            break;
        }

        //确保支持的板型在文件中
        if(lDataLen < pstFileheader->uSupportBoardTypesTable_Offset + sizeof(unsigned int) * pstFileheader->uSupportBoardNum)
        {
            LOGE("Updata packed err : file too short = %d\n", lDataLen);
            break;
        }

        //确保升级包索引在文件中
        if(lDataLen < pstFileheader->uPackageOffsetsTable_Offset + sizeof(unsigned int) * pstFileheader->uPackageNum)
        {
            LOGE("Updata packed err : file too short = %d\n", lDataLen);
            break;
        }

        //板型校验
        lRet = checkBoardType(pstAupfUpdateStatus, pstFileheader);
        if (0 != lRet)
        {
            LOGE("Check board type error.\n");
            break;
        }

        LOGD("Ants update package file BASE INFO CHECK SUCCESSFUL.\n");
        return 0;
    }while(0);

    return -1;
}

static int checkAupfPackageMd5(AupfUpdateStatusMgr_T* pstUpdateStatusMgr, UpdatePackageHeader_T *pstPackageHeader)
{
    int lReadLen = 0;
    Common_Md5_T md5 = NULL;
    unsigned int md5Result[4] = {0};

    if(UpdateCheckType_MD5 == pstPackageHeader->uPackageCheckType)
    {
        int readsize = 0;
        int LeftSize = 0;

        if (pstPackageHeader->uPackageLength > 0)
        {
            Common_Md5_Create(&md5);
            LeftSize = pstPackageHeader->uPackageLength;
            do
            {
                readsize = LeftSize > UPDATE_BUFSIZE ? UPDATE_BUFSIZE : LeftSize;
                //LOGI("read file %d/%d/%d\n",readsize,LeftSize,pstPackageHeader->uPackageLength);

                lReadLen = fread(pstUpdateStatusMgr->m_pUpdateBuff, 1, readsize, pstUpdateStatusMgr->m_hUpgradeFile);
                if(lReadLen <= 0)
                {
                    LOGE("read file failed ret = %d(%d)\n",lReadLen, ferror(pstUpdateStatusMgr->m_hUpgradeFile));
                    return -1;
                }
                LeftSize -= lReadLen;

                Common_Md5_Append(md5, ((unsigned char *)pstUpdateStatusMgr->m_pUpdateBuff), lReadLen);
            }while(LeftSize > 0);
            Common_Md5_Finish(md5, (unsigned char *)md5Result, NULL);
            Common_Md5_Destroy(&md5);

            if(md5Result[0] != pstPackageHeader->uPackageCheckValue[0] ||
               md5Result[1] != pstPackageHeader->uPackageCheckValue[1] ||
               md5Result[2] != pstPackageHeader->uPackageCheckValue[2] ||
               md5Result[3] != pstPackageHeader->uPackageCheckValue[3])
            {
                LOGE("Updata package check failed %x%x%x%x ->%x%x%x%x\n", md5Result[0],
                                                                          md5Result[1],
                                                                          md5Result[2],
                                                                          md5Result[3],
                                                                          pstPackageHeader->uPackageCheckValue[0],
                                                                          pstPackageHeader->uPackageCheckValue[1],
                                                                          pstPackageHeader->uPackageCheckValue[2],
                                                                          pstPackageHeader->uPackageCheckValue[3]);
                return -1;
            }
        }
    }

    return 0;
}

static int checkAupfForPerPackage(AupfUpdateStatusMgr_T* pstUpdateStatusMgr, UpdatePackageHeader_T *pstPackageHeader)
{
    int lRet = -1;

    do
    {
        if(pstPackageHeader->uMagicNumber != UPDATE_PACKAGE_HEADER_MAGIC)
        {
            LOGE("package magic err 0x%X\n",pstPackageHeader->uMagicNumber);
            break;
        }

        if(pstPackageHeader->uPackageLength > 512 * 1024 * 1024)
        {
            LOGE("Updata package len too long %d    invalid\n", pstPackageHeader->uPackageLength);
            break;
        }

        if(UpdateType_File == pstPackageHeader->uPackageType && pstPackageHeader->uExtPackHeaderLengh > pstPackageHeader->uPackageLength)
        {
            LOGE("Updata package file path len too long %d  invalid\n", pstPackageHeader->uExtPackHeaderLengh);
            break;
        }

        if(UpdateType_Tar == pstPackageHeader->uPackageType && pstPackageHeader->uExtPackHeaderLengh > pstPackageHeader->uPackageLength)
        {
            LOGE("Updata package file path len too long %d  invalid\n", pstPackageHeader->uExtPackHeaderLengh);
            break;
        }

        lRet = checkAupfPackageMd5(pstUpdateStatusMgr, pstPackageHeader);
        if(0 != lRet)
        {
            LOGE("Package MD5 CHECK FAILED.\n");
            break;
        }

        LOGD("Package chekc success.\n");
        return 0;
    }while(0);

    return -1;
}

static int checkAupfPackages(AupfUpdateStatusMgr_T* pstAupfUpdateStatus, UpdateFileHeader_T *pstFileheader)
{
    int lRet = -1;
    int lReadLen = 0;
    int nPackNum = 0;
    unsigned int *pIntArray = NULL;

    UpdatePackageHeader_T stCurrPackageHeader;
    UpdatePackageHeader_T *pstPackageHeader = NULL;

    AupfUpdateStatusMgr_T* pstUpdateStatusMgrTmp = pstAupfUpdateStatus;

    if(pstUpdateStatusMgrTmp->m_pPackageOffsetsTable != NULL)
    {
        free(pstUpdateStatusMgrTmp->m_pPackageOffsetsTable);
        pstUpdateStatusMgrTmp->m_pPackageOffsetsTable = NULL;
    }

    pstUpdateStatusMgrTmp->m_uPackageOffsetsNum   = pstFileheader->uPackageNum;
    pstUpdateStatusMgrTmp->m_pPackageOffsetsTable = (unsigned int*)malloc(sizeof(unsigned int) * pstUpdateStatusMgrTmp->m_uPackageOffsetsNum);
    if(NULL == pstUpdateStatusMgrTmp->m_pPackageOffsetsTable)
    {
        LOGE("new packageoffsettable failed.\n");
        return -1;
    }

    pstUpdateStatusMgrTmp->m_uTotalUpdateSize = 0;
    pstUpdateStatusMgrTmp->m_uCurrUpdatedSize = 0;
    pstUpdateStatusMgrTmp->m_uUpdatedSize  = 0;
    pstUpdateStatusMgrTmp->m_bNoFlashWrite = 0;

    // check package md5
    pIntArray = (unsigned int *)(pstUpdateStatusMgrTmp->m_pUpdateBuff + pstFileheader->uPackageOffsetsTable_Offset);
    for(nPackNum = 0; nPackNum < pstUpdateStatusMgrTmp->m_uPackageOffsetsNum; nPackNum++)
    {
        pstUpdateStatusMgrTmp->m_pPackageOffsetsTable[nPackNum] = pIntArray[nPackNum];
    }
    pstUpdateStatusMgrTmp->m_bClearConfig = pstFileheader->uRequireClearConfig;

    // after pstFileheader invalid
    for(nPackNum = 0; nPackNum < pstUpdateStatusMgrTmp->m_uPackageOffsetsNum; nPackNum++)
    {
        LOGD("[%d/%d] offset = 0x%x\n", nPackNum, pstUpdateStatusMgrTmp->m_uPackageOffsetsNum, pstUpdateStatusMgrTmp->m_pPackageOffsetsTable[nPackNum]);
        fseek(pstUpdateStatusMgrTmp->m_hUpgradeFile, pstUpdateStatusMgrTmp->m_pPackageOffsetsTable[nPackNum], SEEK_SET);

        memset((void *)&stCurrPackageHeader, 0x00, sizeof(stCurrPackageHeader));
        lReadLen = fread(&stCurrPackageHeader, 1, sizeof(UpdatePackageHeader_T), pstUpdateStatusMgrTmp->m_hUpgradeFile);
        if(lReadLen != sizeof(stCurrPackageHeader))
        {
            LOGE("read failed ret = %d (%d)\n", lReadLen, ferror(pstUpdateStatusMgrTmp->m_hUpgradeFile));

            free(pstUpdateStatusMgrTmp->m_pPackageOffsetsTable);
            pstUpdateStatusMgrTmp->m_pPackageOffsetsTable = NULL;
            return -1;
        }

        pstPackageHeader = (UpdatePackageHeader_T *)(&stCurrPackageHeader);
        lRet = checkAupfForPerPackage(pstUpdateStatusMgrTmp, pstPackageHeader);
        if (0 != lRet)
        {
            LOGE("Check package(%d/%d) failed.\n", nPackNum, pstUpdateStatusMgrTmp->m_uPackageOffsetsNum);

            free(pstUpdateStatusMgrTmp->m_pPackageOffsetsTable);
            pstUpdateStatusMgrTmp->m_pPackageOffsetsTable = NULL;
            return -1;
        }

        //更新升级数据大小
        pstUpdateStatusMgrTmp->m_uTotalUpdateSize += pstPackageHeader->uPackageLength;
    }

    return 0;
}

static int checkAupfFile(AupfUpdateStatusMgr_T* pstAupfUpdateStatus, S8 *pFileName)
{
    //int i        =  0;
    int lRet     = -1;
    int lReadLen =  0;
    //int nPackNum =  0;

    //unsigned int BoardType  = 0;
    //unsigned int *pIntArray = NULL;

    UpdateFileHeader_T *pstFileheader       = NULL;
    //UpdatePackageHeader_T *pstPackageHeader = NULL;
    //UpdatePackageHeader_T stCurrPackageHeader;

#if 0
    if(pstUpdateSvrMgr->m_bStartTask)
    {
        LOGE("The upgrade task HAS BEGUN.\n");
        return -1;
    }
#endif

    if(NULL == pstAupfUpdateStatus->m_pUpdateBuff)
    {
        pstAupfUpdateStatus->m_pUpdateBuff = (unsigned char*)malloc(sizeof(unsigned char) * UPDATE_BUFSIZE);
    }

    if(NULL == pstAupfUpdateStatus->m_pUpdateBuff)
    {
        LOGE("Update buffer is NULL.\n");
        return -1;
    }
    memset((void *)pstAupfUpdateStatus->m_pUpdateBuff, 0x00, UPDATE_BUFSIZE);

    /* fread
    * 函数功能: 从一个流中读取数据
    * 函数原型: size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    * 参    数: ptr 接收数据地址
    *           size 单个元素大小
    *           nmemb 元素个数
    *           stream 提供数据的文件指针
    * 返回值: 成功读取到的元素个数,返回值小于nmemb说明遇到了结束符.
    */
    lReadLen = fread(pstAupfUpdateStatus->m_pUpdateBuff, 1, UPDATE_BUFSIZE, pstAupfUpdateStatus->m_hUpgradeFile);

    //至少要读取到整个文件头
    if(lReadLen < sizeof(UpdateFileHeader_T))
    {
        free(pstAupfUpdateStatus->m_pUpdateBuff);
        pstAupfUpdateStatus->m_pUpdateBuff = NULL;

        LOGE("Updata packed err : file too short = %d\n", lReadLen);
        return -1;
    }

    // 此处pFileheader不可能为NULL.
    pstFileheader = (UpdateFileHeader_T *)pstAupfUpdateStatus->m_pUpdateBuff;

    lRet = checkAupfBaseInfo(lReadLen, pstAupfUpdateStatus);
    if (0 != lRet)
    {
        free(pstAupfUpdateStatus->m_pUpdateBuff);
        pstAupfUpdateStatus->m_pUpdateBuff = NULL;

        return -1;
    }

    if(0 != Common_Lock(pstAupfUpdateStatus->m_pMutex))
    {
        free(pstAupfUpdateStatus->m_pUpdateBuff);
        pstAupfUpdateStatus->m_pUpdateBuff = NULL;

        LOGE("Updata task create now\n");
        return -1;
    }
    //printf("[%d/%s]pthread_mutex_trylock over\n",__LINE__,__FUNCTION__);
#if 0
    if(pstUpdateSvrMgr->m_bStartTask)
    {
        free(pstUpdateSvrMgr->m_pUpdateBuff);
        pstUpdateSvrMgr->m_pUpdateBuff = NULL;

        Common_UnLock(pstUpdateSvrMgr->m_pMutex);
        return -1;
    }
#endif

    lRet = checkAupfPackages(pstAupfUpdateStatus, pstFileheader);
    if (0 != lRet)
    {
        LOGE("Ants update package file PACKAGE CHECK FAILE.\n");

        free(pstAupfUpdateStatus->m_pUpdateBuff);
        pstAupfUpdateStatus->m_pUpdateBuff = NULL;

        Common_UnLock(pstAupfUpdateStatus->m_pMutex);
        return -1;
    }

    //printf("UpdataThread Start\n");
    {
        char *pstr;
        strcpy(pstAupfUpdateStatus->m_pTarFileName, pFileName);
        pstr= strrchr(pstAupfUpdateStatus->m_pTarFileName,'/');

        if(pstr != NULL && pstr != pstAupfUpdateStatus->m_pTarFileName)
        {
            pstAupfUpdateStatus->m_pTarFileName[pstr - pstAupfUpdateStatus->m_pTarFileName] = 0;
            strcat(pstAupfUpdateStatus->m_pTarFileName,"/tarfileupdate.tar.gz");
        }
        else
        {
            strcpy(pstAupfUpdateStatus->m_pTarFileName,"/root/tarfileupdate.tar.gz");
        }

        LOGD("TAR:%s\n", pstAupfUpdateStatus->m_pTarFileName);
    }

    pstAupfUpdateStatus->m_bFileUp = 1;
    pstAupfUpdateStatus->m_pUpdateByBuffer = NULL;
    Common_UnLock(pstAupfUpdateStatus->m_pMutex);
    return 0;
}

static int getFlashPartIndexByName(char *pMtdName)
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

	return iMtdBlockNum;
}

static int upgradeReplaceTargz(S8 *szUpdateDir /* "/dev/update_bin/" */,
                                           S8 *szOrgTargz /*"/update/bin.tar.gz"*/,
                                           S8 *szOrgRunDir /*"/var/untar/"*/,
                                           S8 *szOrgSubDir /* bin */,
                                           S8 *szOrgFlashDir /* "/update/bin/"*/)
{
	S32 nRet = -1;
	S8 szTmpString[512];
	if(0 == Common_StrCmp(szOrgSubDir,"ko"))
	{
		if(!Common_File_IsExist(szOrgTargz))
		{
			return nRet;
		}
		if(!Common_File_IsExist(szUpdateDir))
		{
			return nRet;
		}
		sprintf(szTmpString,"tar xf %s -C %s/",szOrgTargz,szOrgRunDir);
		if(0 != Common_System(szTmpString))
		{
			return nRet;
		}
	}
	if(Common_File_IsExist(szUpdateDir))
	{
		if(Common_File_IsExist(szOrgTargz))
		{
			if(Common_File_IsExist(szOrgRunDir) || Common_File_IsExist(szOrgFlashDir))
			{
				// "cp -a /dev/update_bin/* /var/untar/bin/"
				sprintf(szTmpString,"cp -a %s/* %s/%s/;cp -a %s/* %s/%s/;rm -rf %s",szUpdateDir,szOrgRunDir,szOrgSubDir,szOrgFlashDir,szOrgRunDir,szOrgSubDir,szOrgFlashDir);
				if(0 == Common_System(szTmpString))
				{
					Common_System("rm /dev/update_tmp.tar.gz;rm /dev/update_org.tar.gz");
					// "tar czvf /dev/update_bin.tar.gz bin -C /var/untar/"
					sprintf(szTmpString,"tar czf /dev/update_tmp.tar.gz %s -C %s/",szOrgSubDir,szOrgRunDir);
					if(0 == Common_System(szTmpString))
					{
						// "mv /update/bin.tar.gz /dev/update_org.tar.gz"
						sprintf(szTmpString,"mv %s /dev/update_org.tar.gz",szOrgTargz);
						if(0 == Common_System(szTmpString))
						{
							// "mv /dev/update_bin.tar.gz /update/bin.tar.gz"
							sprintf(szTmpString,"mv /dev/update_tmp.tar.gz %s",szOrgTargz);
							if(0 == Common_System(szTmpString))
							{
								if(Common_File_IsExist("/dev/update_tmp.tar.gz"))
								{// 失败
									sprintf(szTmpString,"rm %s",szOrgTargz);
									Common_System(szTmpString);
									// "mv /dev/update_org.tar.gz.bak /update/bin.tar.gz "
									sprintf(szTmpString,"mv /dev/update_org.tar.gz %s",szOrgTargz);
									if(0 != Common_System(szTmpString))
									{
										LOGE("[%s.%d]mv update failed\n",__FUNCTION__,__LINE__);
									}
								}
								else
								{
									nRet = 0;
								}
							}
						}
					}
				}
			}
			Common_System("rm /dev/update_tmp.tar.gz;rm /dev/update_org.tar.gz");
		}
		else if(Common_File_IsExist(szOrgFlashDir))
		{
			// "cp -a /dev/update_bin/* /update/bin/"
			sprintf(szTmpString,"cp -a %s/* %s/",szUpdateDir,szOrgFlashDir);
			if(0 == Common_System("cp -a /dev/update_bin/* /update/bin/"))
			{
				nRet = 0;
			}
		}
	}
	return 0;
}

static int upgradeTarFile(AupfUpdateStatusMgr_T * phUpdateStatusMgr, UpdatePackageHeader_T *pPackage)
{
    int lRet     = -1;
    int WriteLen = 0;

    char *pBuf      = NULL;
    char *pFileName = NULL;

    unsigned char *pBuffer = NULL;

    phUpdateStatusMgr->m_bNoFlashWrite  = 1;
    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_BEGIN;
    phUpdateStatusMgr->m_lNoFlashRatio  = 0;

    // pPackage->uRes[0]:file dst path string length + 1
    if(pPackage->uExtPackHeaderLengh > pPackage->uPackageLength)
    {
        return -1;
    }

    //LOGD("m_bFileUp = %d pkglen = %d\n",m_bFileUp,pPackage->uPackageLength);
    if(phUpdateStatusMgr->m_bFileUp)
    {
        pBuf = (char*)malloc(sizeof(char)*(256 + 64 * 1024));//[pPackage->uPackageLength];
        if(pBuf != NULL)
        {
            lRet = fread(pBuf, 1, pPackage->uExtPackHeaderLengh/*pPackage->uPackageLength*/, phUpdateStatusMgr->m_hUpgradeFile);
            if(lRet == pPackage->uExtPackHeaderLengh/*pPackage->uPackageLength*/)
            {
                pFileName = pBuf;
                WriteLen  = pPackage->uPackageLength - pPackage->uExtPackHeaderLengh;
                pBuffer   = (unsigned char *)pBuf + pPackage->uExtPackHeaderLengh;
                if(pPackage->uPackageType == UpdateType_Tar)
                {
                    pFileName = phUpdateStatusMgr->m_pTarFileName;
                }
            }
        }
        else
        {
            LOGE("Malloc memory failed.\n");
            return -1;
        }
    }
    else
    {
        pFileName = (( char *)pPackage) + sizeof(UpdatePackageHeader_T);
        pBuffer   = (unsigned char *)pFileName + pPackage->uExtPackHeaderLengh;
        WriteLen  = pPackage->uPackageLength - pPackage->uExtPackHeaderLengh;
        if(UpdateType_Tar == pPackage->uPackageType)
        {
            pFileName = phUpdateStatusMgr->m_pTarFileName;
        }
        //LOGD("[%s] len = %d,type = %d\n",pFileName,WriteLen,pPackage->uPackageType);
    }

    phUpdateStatusMgr->m_lNoFlashRatio  = 0;
    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING;

    if(pFileName != NULL)
    {
        FILE *fs = fopen(pFileName, "wb+");
        if(fs != NULL)
        {
            if(phUpdateStatusMgr->m_bFileUp)
            {
                int nReadSize = 0,nRRet,nWret;
                lRet = 0;
                while(lRet < WriteLen)
                {
                    nReadSize = 64 * 1024;
                    if(WriteLen - lRet < nReadSize)
                    {
                        nReadSize = WriteLen - lRet;
                    }

                    nRRet = fread(pBuf + 256, 1, nReadSize, phUpdateStatusMgr->m_hUpgradeFile);
                    if(nRRet == nReadSize)
                    {
                        nWret = fwrite(pBuf + 256, 1, nReadSize, fs);
                        if(nWret == nReadSize)
                        {
                            lRet += nReadSize;
                        }
                        else
                        {
                            LOGE("write faild %d - > %d  /%d/%d\n",nReadSize,nRRet,WriteLen,lRet);
                            return -1;
                        }
                    }
                    else
                    {
                        LOGE("read faild %d - > %d /%d/%d\n",nReadSize,nRRet,WriteLen,lRet);
                        return -1;
                    }
                }
            }
            else
            {
                lRet = fwrite(pBuffer, 1, WriteLen, fs);
            }
            fclose(fs);

            if(lRet != WriteLen)
            {
                LOGE("Channel logo /tar write failed : %d, %d", lRet, WriteLen);
                if(UpdateType_Tar == pPackage->uPackageType)
                {
                    char strtmp[256];
                    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING_PLUS1;
                    phUpdateStatusMgr->m_lNoFlashRatio  = 0;
                    sprintf(strtmp, "rm -fr %s", pFileName);
                    Common_System(strtmp);
                }
            }
            else
            {
                if(UpdateType_Tar == pPackage->uPackageType)
                {
                    char strtmp[256];
                    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING;
                    phUpdateStatusMgr->m_lNoFlashRatio  = 50;
                    Common_System("rm /update/update_after /update/update_pre /dev/update_after /dev/update_pre;mkdir -p /dev/deal_update_tmp;");
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || defined PLATFORM_JZT40 || defined PLATFORM_JZT41
					sprintf(strtmp,"gunzip -f %s",pFileName);
					Common_System(strtmp);

					char tarfinename[128];
					if(strlen(pFileName) < 128)
					{
						strncpy(tarfinename, pFileName, strlen(pFileName) - 3);
						tarfinename[strlen(pFileName) - 3] = 0;
						LOGW("JXT31 : tarfile : %s len = %d\n", tarfinename, strlen(pFileName));

	                    sprintf(strtmp,"cd /;tar -xvf %s dev/update_pre",tarfinename);
	                    Common_System(strtmp);
	                    sprintf(strtmp,"cd /;tar -xvf %s dev/update_after",tarfinename);
	                    Common_System(strtmp);
	                    sprintf(strtmp,"cd /dev/;tar -xvf %s update/update_pre;mv /dev/update/update_pre /dev/",tarfinename);
	                    Common_System(strtmp);
	                    sprintf(strtmp,"cd /dev/;tar -xvf %s update/update_after;mv /dev/update/update_after /dev/",tarfinename);
	                    Common_System(strtmp);

	                    sprintf(strtmp,"chmod 777 /dev/update_pre;/dev/update_pre;rm -rf /dev/update_pre");
	                    Common_System(strtmp);

	                    // tar -xzf filename -C /
	                    sprintf(strtmp,"tar -xvf %s -C /",tarfinename);
	                    Common_System(strtmp);
						phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING_PLUS1;
						phUpdateStatusMgr->m_lNoFlashRatio	= 0;
						sprintf(strtmp,"rm -fr %s %s",pFileName, tarfinename);
						Common_System(strtmp);
					}
#else//ifdef DPLATFORM_JZT30
                    // tar xf filename /update/update_pre
                    sprintf(strtmp,"tar -xvzf %s dev/update_pre -C /",pFileName);
                    Common_System(strtmp);
                    sprintf(strtmp,"tar -xvzf %s dev/update_after -C /",pFileName);
                    Common_System(strtmp);
                    sprintf(strtmp,"tar -xvzf %s update/update_pre -C /dev/deal_update_tmp/;mv /dev/deal_update_tmp/update/update_pre /dev/",pFileName);
                    Common_System(strtmp);
                    sprintf(strtmp,"tar -xvzf %s update/update_after -C /dev/deal_update_tmp/;mv /dev/deal_update_tmp/update/update_after /dev/",pFileName);
                    Common_System(strtmp);

                    sprintf(strtmp,"chmod 777 /dev/update_pre;/dev/update_pre;rm -rf /dev/update_pre");
                    Common_System(strtmp);

                    // tar -xzf filename -C /
                    sprintf(strtmp,"tar -xvzf %s -C /",pFileName);
                    Common_System(strtmp);
                    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING_PLUS1;
                    phUpdateStatusMgr->m_lNoFlashRatio  = 0;
                    sprintf(strtmp,"rm -fr %s",pFileName);
                    Common_System(strtmp);

                    // check /update/bin and /update/bin.tar.gz
                    upgradeReplaceTargz("/dev/update_bin/","/update/bin.tar.gz","/var/untar/","bin","/update/bin/");
                    upgradeReplaceTargz("/dev/update_lib/","/update/lib.tar.gz","/var/untar/","lib","/update/lib/");
                    upgradeReplaceTargz("/dev/update_ko/","/update/ko.tar.gz","/var/untar/","ko","/update/ko/");
#endif//ifdef DPLATFORM_JZT30

                    // run /update/update_after
                    Common_System("chmod 777 /dev/update_after;/dev/update_after;rm /dev/update_after /dev/update_pre");
                }
            }
        }
        else
        {
            LOGE("open file failed [%s]\n",pFileName);
        }
    }

    Common_System("sync");
    LOGD("%d ---- %d \n",pPackage->uPackageType, UpdateType_Tar);

    if(pBuf != NULL)
    {
        free(pBuf);
        pBuf = NULL;
    }

    return 0;
}

static int upgradeAllFlash(AupfUpdateStatusMgr_T * phUpdateStatusMgr, UpdatePackageHeader_T *pPackage)
{
    int lRet = -1;

    FILE *pout = fopen("/dev/update.flash","wb+");
    phUpdateStatusMgr->m_bNoFlashWrite  = 1;
    phUpdateStatusMgr->m_lNoFlashRatio  = 0;
    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_BEGIN;

    if (pout != NULL)
    {
        if(phUpdateStatusMgr->m_bFileUp)
        {
            int nReadSize = 0,nRRet,nWret;
            char *pBuf;
            lRet = 0;
            pBuf = (char *)Common_Malloc(64 * 1024,0,__FUNCTION__,__LINE__);
            if (pBuf != NULL)
            {
                while(lRet < pPackage->uPackageLength)
                {
                    nReadSize = 64 * 1024;
                    if(pPackage->uPackageLength - lRet < nReadSize)
                    {
                        nReadSize = pPackage->uPackageLength - lRet;
                    }
                    nRRet = fread(pBuf, 1, nReadSize, phUpdateStatusMgr->m_hUpgradeFile);
                    if(nRRet == nReadSize)
                    {
                        nWret = fwrite(pBuf, 1, nReadSize, pout);
                        if(nWret == nReadSize)
                        {
                            lRet += nReadSize;
                        }
                        else
                        {
                            LOGE("write faild %d - > %d  /%d/%d\n",nReadSize,nRRet,pPackage->uPackageLength,lRet);
                            break;
                        }
                    }
                    else
                    {
                        LOGE("read faild %d - > %d /%d/%d\n",nReadSize,nRRet,pPackage->uPackageLength,lRet);
                        break;
                    }
                }
            }
        }
        else
        {
            //lRet = fwrite(pBuffer, 1, pPackage->uPackageLength, pout);
            LOGE("FileUp is FALSE and pBuffer is NULL.\n");
            lRet = -1;
        }

        fclose(pout);
        pout = NULL;
        phUpdateStatusMgr->m_lNoFlashRatio  = 0;
        phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING_PLUS1;

        if (lRet == pPackage->uPackageLength)
        {// 完整
            Common_System("upflash /dev/update.flash;rm /dev/update.flash;");
        }
        else
        {
             Common_System("rm /dev/update.flash;");
        }
    }

    return 0;
}

static int upgradeAupfFile(AupfUpdateStatusMgr_T * phUpdateStatusMgr)
{
	unsigned char *pBuffer = NULL;
	int nPackageNum = 0;
	int lRet = 0;
	int nMtdIndex = -1;
	S8 *pMtdName = NULL;
	int nMtdPackLen = 0;
	UpdatePackageHeader_T *pPackage,CurrPackage;

	LOGD("[%s.%d] m_uPackageOffsetsNum = %d\n", __FUNCTION__, __LINE__, phUpdateStatusMgr->m_uPackageOffsetsNum);
    for(nPackageNum = 0; nPackageNum < phUpdateStatusMgr->m_uPackageOffsetsNum; nPackageNum++)
    {
        if(phUpdateStatusMgr->m_bFileUp)
        {
            fseek(phUpdateStatusMgr->m_hUpgradeFile, phUpdateStatusMgr->m_pPackageOffsetsTable[nPackageNum], SEEK_SET);
            lRet = fread(&CurrPackage, 1, sizeof(CurrPackage), phUpdateStatusMgr->m_hUpgradeFile);
            if(lRet != sizeof(CurrPackage))
            {
                LOGE("[%d/%d]Read file failed lRet = %d %d\n",nPackageNum, phUpdateStatusMgr->m_uPackageOffsetsNum,lRet,sizeof(CurrPackage));
                return -1;
            }
            pPackage = &CurrPackage;
        }
        else
        {
            pPackage = (UpdatePackageHeader_T *)(phUpdateStatusMgr->m_pUpdateByBuffer + phUpdateStatusMgr->m_pPackageOffsetsTable[nPackageNum]);
        }

        if(pPackage->uMagicNumber != UPDATE_PACKAGE_HEADER_MAGIC)
        {
            LOGE("uMagicNumber = %x\n",pPackage->uMagicNumber);
            return -1;
        }

        //更新当前升级数据包大小
        phUpdateStatusMgr->m_uCurrUpdatedSize = pPackage->uPackageLength;

        LOGD("package [%d] type = %d\n", nPackageNum, pPackage->uPackageType);
        switch(pPackage->uPackageType)
        {// uboot,factory config kernel custom
            case UpdateType_Uboot:
                {
                    nMtdIndex = 0;
                    nMtdPackLen = pPackage->uPackageLength;
                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_Kernel:
                {
                    nMtdIndex = 3;
                    nMtdPackLen = pPackage->uPackageLength;

                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_Rfs:
                {
                    nMtdIndex = -1;
                    nMtdPackLen = pPackage->uPackageLength;

                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_App:
                {
                    nMtdIndex = 4;
                    nMtdPackLen = pPackage->uPackageLength;

                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_Config:
                {
                    nMtdIndex = 2;
                    nMtdPackLen = pPackage->uPackageLength;

                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_Logo:
                {
                    nMtdIndex = -1;
                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_ChLogo:
                {

                    nMtdIndex = -1;
                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_File:
            case UpdateType_Tar:
                {
                    (void)upgradeTarFile(phUpdateStatusMgr, pPackage);
                    phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING_PLUS2;
                    nMtdIndex = -1;
                    break;
                }

            case UpdateType_Flash:
                {
                    (void)upgradeAllFlash(phUpdateStatusMgr, pPackage);
                     phUpdateStatusMgr->m_lNoFlashStatus = UPDATE_STATUS_E_UPDATING_PLUS2;
                     nMtdIndex = -1;//
                    break;
                }
            case UpdateType_PartIndex:
                {
                    nMtdIndex = pPackage->uPartIndx;
                    nMtdIndex--;
                    if(nMtdIndex == getFlashPartIndexByName("factory"))
                    {
                        nMtdIndex = -1;
                    }

                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            case UpdateType_PartName:
                {
                    nMtdIndex = -1;

                    if (pPackage->uExtPackHeaderLengh > 0)
                    {
                        pMtdName = (char *)Common_Malloc(pPackage->uExtPackHeaderLengh + 1,0,__FUNCTION__,__LINE__);//[pPackage->uPackageLength];
                        if(pMtdName != NULL)
                        {
                            memset(pMtdName,0,pPackage->uExtPackHeaderLengh + 1);
                            if(phUpdateStatusMgr->m_bFileUp)
                            {
                                lRet = fread(pMtdName, 1, pPackage->uExtPackHeaderLengh/*pPackage->uPackageLength*/, phUpdateStatusMgr->m_hUpgradeFile);
                            }
                            else
                            {
                                memcpy(pMtdName, (( char *)pPackage) + sizeof(UpdatePackageHeader_T),pPackage->uExtPackHeaderLengh);
                                lRet = pPackage->uExtPackHeaderLengh;
                                pBuffer = (( unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T) + pPackage->uExtPackHeaderLengh;
                            }

                            if(lRet == pPackage->uExtPackHeaderLengh/*pPackage->uPackageLength*/)
                            {
                                nMtdPackLen = pPackage->uPackageLength - pPackage->uExtPackHeaderLengh;
                            }

                            if(0 == Common_StriCmp(pMtdName,"factory"))
                            {
                                Common_Free(pMtdName,__FUNCTION__,__LINE__);
                                pMtdName = NULL;
                            }
                        }
                    }

                    phUpdateStatusMgr->m_bNoFlashWrite = 0;
                    break;
                }
            default:
                {
                    nMtdIndex = -1;
                    break;
                }
        }

        if(nMtdIndex != -1)
        {
            //卸载分区
            (void)udpate_common_UmountPartition();

            if(phUpdateStatusMgr->m_bFileUp)
            {
                //fseek(m_hUpgradeFile,sizeof(UpdatePackageHeader_T),SEEK_CUR);
                lRet = Ovfs_update_Flash_WriteFileMtd(nMtdIndex, phUpdateStatusMgr->m_hUpgradeFile, pPackage->uPackageLength, 0, 0);
            }
            else
            {
                pBuffer = ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T);
                lRet = Ovfs_update_Flash_WriteMtd(nMtdIndex, (char *)pBuffer, pPackage->uPackageLength, 0, 0);
            }

            nMtdIndex = -1;
            if(lRet != 0)
            {
                LOGE("Write mtd err : UpdataType = %d", pPackage->uPackageType);
                return -1;
            }
        }
        else if(pMtdName != NULL)
        {
            //卸载分区
            (void)udpate_common_UmountPartition();

            if(phUpdateStatusMgr->m_bFileUp)
            {
                //fseek(m_hUpgradeFile,sizeof(UpdatePackageHeader_T),SEEK_CUR);
                lRet = Ovfs_update_Flash_WriteFileMtd_ByName(pMtdName, phUpdateStatusMgr->m_hUpgradeFile, nMtdPackLen, 0, 0);
                LOGD("#Flash_WriteFileMtd (%s -> %d)\n", pMtdName, pPackage->uPackageLength);
            }
            else
            {
                pBuffer = ((unsigned char *)pPackage) + sizeof(UpdatePackageHeader_T);
                lRet = Ovfs_update_Flash_WriteMtd_ByName(pMtdName, (char *)pBuffer, nMtdPackLen, 0, 0);
                LOGD("#Flash_WriteMtd (%s-> %d)\n", pMtdName, pPackage->uPackageLength);
            }

            Common_Free(pMtdName,__FUNCTION__,__LINE__);
            pMtdName = NULL;

            if(lRet != 0)
            {
                LOGE("Write mtd err : UpdataType = %d", pPackage->uPackageType);

                return -1;
            }
        }

        //更新已升级的数据
        phUpdateStatusMgr->m_uUpdatedSize += pPackage->uPackageLength;
    }

    if(phUpdateStatusMgr->m_bClearConfig)
    {
        Common_System("rm -rf /usr/etc/cfgfiles/*");
    }

    return 0;
}

int updateAupf_Upgrade(UpdateSvrMgr* pstUpdateSvrMgr, S8 *pcFileName)
{
    int lRet = -1;

    AupfUpdateStatusMgr_T *pstAupfUpdateStatusMgr = getAupfStatusHandle();

    // 升级状态初始化
    lRet = initAupfUpdateStatus(pstUpdateSvrMgr, pstAupfUpdateStatusMgr);
    if (0 != lRet)
    {
        LOGE("Init aupf update status failed.\n");
        return -1;
    }

    //镜像校验
    lRet = checkAupfFile(pstAupfUpdateStatusMgr, pcFileName);
    if (0 != lRet)
    {
        LOGE("Check ants update package file error.\n");

        (void)unInitAupfUpdateStatus(pstAupfUpdateStatusMgr);
        return -1;
    }

    //镜像升级
    lRet = upgradeAupfFile(pstAupfUpdateStatusMgr);
    if (0 != lRet)
    {
        LOGE("Upgrade ants update package file error.\n");

        (void)unInitAupfUpdateStatus(pstAupfUpdateStatusMgr);
        return -1;
    }

    return lRet;  /* 升级成功会reboot,不用uninit释放资源. */
}

int updateAupf_GetUpgradeStatus(int *plStatus, int *plUpgradeProgress)
{
	//LONG lRet    = 0;
    LONG lRatio  = 0;
    LONG XRatio  = 0;
    LONG lStatus = 0;

    AupfUpdateStatusMgr_T * pstAupfUpdateStatusMgr = getAupfStatusHandle();

    if ((NULL == plStatus) || (NULL == plUpgradeProgress))
    {
        LOGE("Input parameters BUFFER IS NULL.\n");
        return -1;
    }

    if(pstAupfUpdateStatusMgr->m_uTotalUpdateSize == 0)
    {
        // 开始升级
        *plStatus = UPDATE_STATUS_E_BEGIN; // 1;
        *plUpgradeProgress = 0;
        return 0;
    }

    if(pstAupfUpdateStatusMgr->m_uUpdatedSize == pstAupfUpdateStatusMgr->m_uTotalUpdateSize)
    {
        // 升级完成
        *plStatus = UPDATE_STATUS_E_FINISH; // 0;
        *plUpgradeProgress = 100;
        return 0;
    }

    if(pstAupfUpdateStatusMgr->m_bNoFlashWrite)
    {
        //lRet    = 0;
        lStatus = pstAupfUpdateStatusMgr->m_lNoFlashStatus;
        lRatio  = pstAupfUpdateStatusMgr->m_lNoFlashRatio;

        if(UPDATE_STATUS_E_FAILED == lStatus)
        {
            XRatio = 0;
        }
        else if(UPDATE_STATUS_E_FINISH == lStatus)
        {
            XRatio = 100;
        }
        else
        {
            if(UPDATE_STATUS_E_UPDATING == lStatus)
            {
                XRatio = (lRatio * 3) / 10;
                XRatio = (XRatio > 1 ? XRatio : 1);
            }
            else if(UPDATE_STATUS_E_UPDATING_PLUS1 == lStatus)
            {
                XRatio = (lRatio * 4) / 10;
                XRatio = (XRatio > 1 ? XRatio : 1);
                XRatio += 30;
            }
            else if(UPDATE_STATUS_E_UPDATING_PLUS2 == lStatus)
            {
                XRatio = (lRatio * 3) / 10;
                XRatio = (XRatio > 1 ? XRatio : 1);
                XRatio += 70;
            }
            else
            {
                XRatio = 0;
            }
        }

        lRatio = (pstAupfUpdateStatusMgr->m_uUpdatedSize * 100
            + pstAupfUpdateStatusMgr->m_uCurrUpdatedSize * XRatio) / pstAupfUpdateStatusMgr->m_uTotalUpdateSize;
    }
    else
    {
        lRatio = ((pstAupfUpdateStatusMgr->m_uUpdatedSize + Ovfs_update_Flash_GetEWStatus()) * 100)
                    / pstAupfUpdateStatusMgr->m_uTotalUpdateSize;
        LOGD("UpdatedSize=%d, TotalUpdateSize=%d.\n", pstAupfUpdateStatusMgr->m_uUpdatedSize,
                                                      pstAupfUpdateStatusMgr->m_uTotalUpdateSize);
    }

    /**
     * (1)升级进度计算有误差,
     * (2)升级完成(100%)条件: 已写入数据量等于总的升级包大小
     */
    if(100 == lRatio)
    {
        lRatio = 99;
    }
    lStatus = UPDATE_STATUS_E_UPDATING; // 2; // 升级过程中

    *plStatus          = lStatus;
    *plUpgradeProgress = lRatio;
    return 0;
}
