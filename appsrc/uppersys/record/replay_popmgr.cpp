#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "replay_popmgr.h"

CPopMgr* CPopMgr::theCPopManager = NULL;

CPopMgr *CPopMgr::GetCPopMgrItem()
{
	if(NULL == theCPopManager)
	{
		theCPopManager = new CPopMgr();

		if(NULL == theCPopManager)
		{
			LOGE("theAlarmOutManager err\n");
			return NULL;
		}

		theCPopManager->Init();
	}
	return theCPopManager;
}

void CPopMgr::DelCPopMgrItem()
{
	if (NULL != theCPopManager)
	{
		delete theCPopManager;
		theCPopManager = NULL;
	}
}

U32 CPopMgr::Init()
{
	if(bInit)
	{
		return TRUE;
	}

	for(S32 i = 0; i < MAX_CPOPM_NUM; i ++)
	{
		pCpopMHandle[i] = new CDiskPopM;
		pCpopMHandle[i]->SetDevNo(i);
	}
	bInit = TRUE;

	return TRUE;
}

U32 CPopMgr::UnInit()
{
	U32 i;

	for(i = 0; i < MAX_CPOPM_NUM; i ++)
	{
		if(pCpopMHandle[i] != NULL)
		{
			delete pCpopMHandle[i];
		}
	}

	bInit = FALSE;

	return TRUE;
}

CDiskPopM *CPopMgr::GetOneCDiskPopM()
{
	S32 i;

	if(dwCDiskPopMNum >= MAX_CPOPM_NUM)
	{
		LOGE("DiskPop resource is less\n");
		return NULL;
	}
	for(i = 0; i < MAX_CPOPM_NUM; i ++)
	{
		if(!bUsedPopM[i])
		{
			bUsedPopM[i] = TRUE;
			dwCDiskPopMNum ++;
			LOGD("create %d,dwCDiskPopMNum:%d\n",i,dwCDiskPopMNum);
			return pCpopMHandle[i];
		}
	}

	return NULL;
}

U32 CPopMgr::ReleaseCDiskPopM(CDiskPopM *pHandle)
{
	S32 i;

	if(pHandle == NULL)
	{
		LOGE("Handle is NULL\n");
		return FALSE;
	}

	i = pHandle->GetDevNo();
	if(i < 0 || i >= MAX_CPOPM_NUM)
	{
		LOGE("Handle err : No = %d\n", i);
		return FALSE;
	}
	if(bUsedPopM[i])
	{
		LOGD("delete %d,dwCDiskPopMNum:%d\n",i,dwCDiskPopMNum);
		pHandle->Release();
		bUsedPopM[i] = FALSE;
		dwCDiskPopMNum --;
	}

	return TRUE;
}

CPopMgr::CPopMgr()
{
	bInit = FALSE;
	dwCDiskPopMNum = 0;
	for(S32 i = 0; i < MAX_CPOPM_NUM; i ++)
	{
		pCpopMHandle[i] = 0;
		bUsedPopM[i] = FALSE;
	}
}

CPopMgr::~CPopMgr()
{
	if(bInit)
	{
		UnInit();
		bInit = FALSE;
	}
}

