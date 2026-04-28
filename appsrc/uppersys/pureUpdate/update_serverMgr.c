#include "update_serverMgr.h"
#include "update_server.h"
#include "update_common.h"
#include "update_aupf.h"
#include "update_rpu.h"
#include "flashrw.h"

//升级文件格式
typedef enum TAG_UPDATE_FIRE_FORMAT{
    /** Unknown file format */
    UPDATE_FIRE_FORMAT_E_UNKNOWN = 0,

    /** Ants Raw Partition Update */
    UPDATE_FIRE_FORMAT_E_ARPU = 1,

    /** Ants update Package File */
    UPDATE_FIRE_FORMAT_E_AUPF = 2
}UPDATE_FIRE_FORMAT;

static void UpdataSvrMgr_Init(UpdateSvrMgr* phUpdateSvrMgr);

static UpdateSvrMgr* g_pstUpdataManager = NULL;

UpdateSvrMgr* updateServer_getUpdateItem()
{
	if (NULL == g_pstUpdataManager)
	{
		g_pstUpdataManager = (UpdateSvrMgr*)malloc(sizeof(UpdateSvrMgr));

		if (NULL == g_pstUpdataManager)
		{
			LOGE("GetMgrItem err");
		}
        else
        {
            UpdataSvrMgr_Init(g_pstUpdataManager);
        }
	}

    return g_pstUpdataManager;
}

static void SetBoardType(DWORD type)
{
	g_pstUpdataManager->m_dwBoardType = type;
}

static void SetSoftwareVersion(DWORD version)
{
	g_pstUpdataManager->m_dwSoftwareVersion = version;
}

static LONG CreateTask(char *pBuffer)
{
    LOGE("The API is stub stub stub ...\n");
    return -1;
}

//AUPF(Ants Update Package File) or ARPU(Ants Raw Part Update File)
static S32 checkFileFormat(S8 *pFileName, UpdateSvrMgr* pstUpdateMgrHandle)
{
    S32 lRet = 0;

    pFileName = pFileName;

    if (NULL == pstUpdateMgrHandle->m_hUpgradeFile)
    {
        LOGE("File handle is NULL.\n");
        return -1;
    }

	unsigned int stRpuFileHead;
    //RawPartUpgradeFileHeader_T stRpuFileHead;
    memset((void *)&stRpuFileHead, 0x00, sizeof(stRpuFileHead));

    lRet = fread((S8 *)&stRpuFileHead, 1, sizeof(stRpuFileHead), pstUpdateMgrHandle->m_hUpgradeFile);
    if (lRet < sizeof(stRpuFileHead))
    {
        LOGE("Read data so shot.\n");
        return -1;
    }

    //读写位置复原
    fseek(pstUpdateMgrHandle->m_hUpgradeFile, 0, SEEK_SET);

	if (stRpuFileHead == ARPUHEADERMAGIC)
    //if (stRpuFileHead.uMagicNumber == ARPUHEADERMAGIC)
    {
        LOGD("It is Raw Partition Update Formate.\n");
        return UPDATE_FIRE_FORMAT_E_ARPU; // ARPU
    }
    else
    {
        LOGD("It is Ants Update Package File.\n");
        return UPDATE_FIRE_FORMAT_E_AUPF; // AUPF
    }

    return -1;
}

static LONG CreateTaskFile(S8 *pFileName)
{
	S32 lRet;

	if(pFileName == NULL)
	{
		LOGE("FileName = NULL\n");
		return -1;
	}

    UpdateSvrMgr* phUpdateServerMgrItem = updateServer_getUpdateItem();
    if (NULL == phUpdateServerMgrItem)
    {
        LOGE("Update server mgr is NULL.\n");
        return -1;
    }

    //如果正在升级,直接返回错误.
    if (phUpdateServerMgrItem->m_bStartTask)
    {
        LOGE("The upgrade task HAS BEGUN.\n");
        return -1;
    }

    LOGI("FileName = %s\n", pFileName);
	if(phUpdateServerMgrItem->m_hUpgradeFile != NULL)
	{
		fclose(phUpdateServerMgrItem->m_hUpgradeFile);
		phUpdateServerMgrItem->m_hUpgradeFile = NULL;
	}

	phUpdateServerMgrItem->m_hUpgradeFile = fopen(pFileName, "rb");
	if(NULL == phUpdateServerMgrItem->m_hUpgradeFile)
	{
		LOGE("open failed FileName = %s\n",pFileName);
		return -1;
	}

    //保存升级文件名称(包含路径)
    strcpy(phUpdateServerMgrItem->m_acUpgradeFileName, pFileName);
	phUpdateServerMgrItem->m_lUpgradeFileFormatCheckOK = 0;

    lRet = Common_Thread_Create(&phUpdateServerMgrItem->m_UpdataThreadId, 
                                "DoUpdate", 
                                0, 
                                0, 
                                phUpdateServerMgrItem->UpdataThread, (void*)phUpdateServerMgrItem);
	if(lRet != 0)
	{
		LOGE("UpdataThread create err : ret = %d\n", lRet);

        fclose(phUpdateServerMgrItem->m_hUpgradeFile);
		phUpdateServerMgrItem->m_hUpgradeFile = NULL;
		return -1;
	}

	phUpdateServerMgrItem->m_bStartTask = TRUE;
	phUpdateServerMgrItem->SetStatusFlag(TRUE);
	phUpdateServerMgrItem->m_lHandleCnt --;

    if(phUpdateServerMgrItem->m_lHandleCnt < 0)
    {
		phUpdateServerMgrItem->m_lHandleCnt = 0x7FFFFFFF;
    }

	LOGD("update handle = %x\n", phUpdateServerMgrItem->m_lHandleCnt);
	return phUpdateServerMgrItem->m_lHandleCnt;
}

static BOOL DestroyTask(LONG lHandle)
{
    UpdateSvrMgr* phUpdateServerMgrItem = updateServer_getUpdateItem();
    if (NULL == phUpdateServerMgrItem)
    {
        LOGE("Update server mgr is NULL.\n");
        return FALSE;
    }

	if(lHandle != phUpdateServerMgrItem->m_lHandleCnt)
	{
		LOGE("Handle is not valid");
		return FALSE;
	}

    if(!phUpdateServerMgrItem->m_bStartTask)
	{
		LOGE("Handle is not exist");
		return FALSE;
	}
	Common_Thread_Destroy(&phUpdateServerMgrItem->m_UpdataThreadId);

    if(phUpdateServerMgrItem->m_hUpgradeFile != NULL)
    {
		fclose(phUpdateServerMgrItem->m_hUpgradeFile);
    }
	phUpdateServerMgrItem->m_hUpgradeFile = NULL;

	phUpdateServerMgrItem->m_bStartTask = FALSE;
	phUpdateServerMgrItem->m_bWriteChannelLogo = FALSE;
	return TRUE;
}

extern int CreateAndListenHttp();

static int DoUpdate()
{
    int lRet = -1;

    UpdateSvrMgr* phUpdateServerMgrItem = updateServer_getUpdateItem();

    //检查打包格式,选择升级方法(AUPF/ARPU)
    phUpdateServerMgrItem->m_lUpgradeFileFormat = checkFileFormat(phUpdateServerMgrItem->m_acUpgradeFileName, phUpdateServerMgrItem);
    if (-1 == phUpdateServerMgrItem->m_lUpgradeFileFormat)
    {
        fclose(phUpdateServerMgrItem->m_hUpgradeFile);
		phUpdateServerMgrItem->m_hUpgradeFile = NULL;
		phUpdateServerMgrItem->m_lUpgradeFileFormatCheckOK = 1;

        LOGE("Check update file error.\n");
        return -1;
    }

    system("pkill -9 webserver ;/root/nginx/sbin/nginx -p /root/nginx -s quit;pkill -9 nginx;");
    CreateAndListenHttp();

    if (UPDATE_FIRE_FORMAT_E_ARPU == phUpdateServerMgrItem->m_lUpgradeFileFormat)
    {
		updateRpu_Begin();
		phUpdateServerMgrItem->m_lUpgradeFileFormatCheckOK = 1;
        lRet = updateRpu_Upgrade(phUpdateServerMgrItem, phUpdateServerMgrItem->m_acUpgradeFileName); //Ante Raw Partition Upgrade
    }
    else if (UPDATE_FIRE_FORMAT_E_AUPF == phUpdateServerMgrItem->m_lUpgradeFileFormat)
    {
		phUpdateServerMgrItem->m_lUpgradeFileFormatCheckOK = 1;
        lRet = updateAupf_Upgrade(phUpdateServerMgrItem, phUpdateServerMgrItem->m_acUpgradeFileName); //Ants Update Package File
    }
    else
    {
        fclose(phUpdateServerMgrItem->m_hUpgradeFile);
		phUpdateServerMgrItem->m_hUpgradeFile = NULL;
		phUpdateServerMgrItem->m_lUpgradeFileFormatCheckOK = 1;

        LOGE("UNKNOWN upgrade file format.\n");
        return -1;
    }

    fclose(phUpdateServerMgrItem->m_hUpgradeFile);
    phUpdateServerMgrItem->m_hUpgradeFile = NULL;
	return lRet;
}

static BOOL GetTaskStatus(LONG lHandle, LONG *plUpdataStatus, LONG *plUpdataProgress)
{
    int lRet = 0; 

    UpdateSvrMgr* phUpdateServerMgrItem = updateServer_getUpdateItem();
    if (NULL == phUpdateServerMgrItem)
    {
        LOGE("Update server mgr is NULL.\n");
        return FALSE;
    }

	if(lHandle != phUpdateServerMgrItem->m_lHandleCnt)
	{
		LOGE("Handle is not valid.\n");
		return FALSE;
	}

    if(!phUpdateServerMgrItem->m_bStartTask)
	{
		LOGE("Handle is not exist.\n");
		return FALSE;
	}

	if (!phUpdateServerMgrItem->m_bUpdataOK)
	{
	    LOGE("Upgrade failed.\n");

	    // 升级失败
		*plUpdataStatus   = UPDATE_STATUS_E_FAILED; // -1;
		*plUpdataProgress = 0;
        return TRUE;
	}
	if(!phUpdateServerMgrItem->m_lUpgradeFileFormatCheckOK)
	{
		*plUpdataStatus = UPDATE_STATUS_E_UPDATING;
		*plUpdataProgress = 0;
		return TRUE;
	}

    if (UPDATE_FIRE_FORMAT_E_AUPF == phUpdateServerMgrItem->m_lUpgradeFileFormat)
    {
        lRet = updateAupf_GetUpgradeStatus(plUpdataStatus, plUpdataProgress);
    }
    else if (UPDATE_FIRE_FORMAT_E_ARPU == phUpdateServerMgrItem->m_lUpgradeFileFormat)
    {
        lRet = updateRpu_GetUpgradeStatus(plUpdataStatus, plUpdataProgress);
    }
    else if (-1 == phUpdateServerMgrItem->m_lUpgradeFileFormat)
    {
        LOGE("Update file format error.\n");
        return FALSE;
    }
    else
    {
		*plUpdataStatus = UPDATE_STATUS_E_UPDATING;
		*plUpdataProgress = 0;
    }

    LOGD("Process: %%%d.\n", *plUpdataProgress);

    if (0 != lRet)
    {
        LOGE("Get update status failed.\n");
        return FALSE;
    }

	return TRUE;
}

static void SetStatusFlag(int bSucc)
{
    UpdateSvrMgr* phUpdateServerMgrItem = updateServer_getUpdateItem();
    if (NULL == phUpdateServerMgrItem)
    {
        LOGE("Update server mgr is NULL.\n");
    }

	phUpdateServerMgrItem->m_bUpdataOK = bSucc;
}

#if 0
static void UpdataSvrMgr_Uninit()
{
    UpdateSvrMgr* phUpdateServerMgrItem = updateServer_getUpdateItem();
    if (NULL == phUpdateServerMgrItem)
    {
        LOGE("Update server mgr is NULL.\n");
    }

	if(phUpdateServerMgrItem->m_bStartTask)
	{
		Common_Thread_Destroy(&phUpdateServerMgrItem->m_UpdataThreadId);
		phUpdateServerMgrItem->m_bStartTask = FALSE;
	}

	if(phUpdateServerMgrItem->m_hUpgradeFile != NULL)
	{
		fclose(phUpdateServerMgrItem->m_hUpgradeFile);
		phUpdateServerMgrItem->m_hUpgradeFile = NULL;
	}
}
#endif

static int UpdataThread(Common_Thread_T hThread,void *pParam)
{
    int lRet = -1;
	UpdateSvrMgr *handle = (UpdateSvrMgr *)(pParam);

    if(NULL == handle)
	{
		LOGI("[%s.%d]\n",__FUNCTION__,__LINE__);
		return -1;
    }

    lRet = handle->DoUpdate();
    if(lRet < 0)
    {
        handle->SetStatusFlag(FALSE);
    }
    else
    {
        handle->SetStatusFlag(TRUE);
    }

    LOGD("Exit: [%s.%d]\n",__FUNCTION__,__LINE__);
	return lRet;
}

static void UpdataSvrMgr_Init(UpdateSvrMgr* phUpdateSvrMgr)
{
	phUpdateSvrMgr->m_lHandleCnt = 0x7FFFFFFF;
	phUpdateSvrMgr->m_bStartTask = FALSE;
	phUpdateSvrMgr->m_bWriteChannelLogo = FALSE;
	phUpdateSvrMgr->m_dwBoardType = 0;
	phUpdateSvrMgr->m_dwSoftwareVersion = 0;
	phUpdateSvrMgr->m_hUpgradeFile = NULL;
	phUpdateSvrMgr->m_bUpdataOK = FALSE;
	phUpdateSvrMgr->m_UpdataThreadId = NULL;
    phUpdateSvrMgr->m_lUpgradeFileFormat = 0;
	phUpdateSvrMgr->m_lUpgradeFileFormatCheckOK = 0;
    memset(phUpdateSvrMgr->m_acUpgradeFileName, 0x00, sizeof(phUpdateSvrMgr->m_acUpgradeFileName));

	phUpdateSvrMgr->SetBoardType = SetBoardType;
	phUpdateSvrMgr->SetSoftwareVersion = SetSoftwareVersion;
 	phUpdateSvrMgr->CreateTask = CreateTask;
 	phUpdateSvrMgr->CreateTaskFile = CreateTaskFile;
 	phUpdateSvrMgr->DestroyTask = DestroyTask;
 	phUpdateSvrMgr->GetTaskStatus = GetTaskStatus;
	phUpdateSvrMgr->DoUpdate = DoUpdate;
	phUpdateSvrMgr->SetStatusFlag = SetStatusFlag;
	phUpdateSvrMgr->UpdataThread  = UpdataThread;
}


