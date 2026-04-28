#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <map>

#include "replay_api.h"
#include "disk_manage_api.h"

using namespace cv_soft;
using namespace std;

static pthread_mutex_t s_replay_init_lock = PTHREAD_MUTEX_INITIALIZER;

typedef map<S32, CQuery *>	QueryMap;
typedef map<S32, CDiskPopM *>	ReplayMap;

static S32 sFindFileCnt = 0x7FFFFFFF;
static QueryMap CV_QueryInfo;

static S32 sReplayCnt = 0x7FFFFFFF;
static ReplayMap CV_ReplayInfo;

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_querybymonthly(S32 year, S32 month, U32 *pResult)
{
	U32 day[2][12] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
					  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
	time_t starttime;
	U32 daynum;
	Common_Time_T struCommon_time;
	memset(&struCommon_time, 0, sizeof(Common_Time_T));	
	struCommon_time.year = year;
	struCommon_time.month = month;
	struCommon_time.day = 1;
	Common_Common2LinuxTime(&struCommon_time, &starttime);	
	if((year % 4) != 0)
	{
		daynum = day[0][month-1];
	}
	else
	{
		if((year % 100) == 0 && (year % 400) != 0)
		{
			daynum = day[0][month-1];
		}
		else
		{
			daynum = day[1][month-1];
		}
	}
	cv_diskm_serchrecordbyday(starttime, daynum, pResult);

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_querycreate(U32 *phandle, CV_RECORD_QUESTPARA param)
{
	CQuery	*pCQueryHandle;

	if(phandle == NULL)
	{
		LOGE("param is %p, phandle = %p\n", param, phandle);
		return CV_ERR_REC_INVALID_PARA;
	}

	if(param.Channel < 0)
	{
		return CV_ERR_REC_INVALID_PARA;
	}

	pCQueryHandle = new CQuery(param);

	pthread_mutex_lock(&s_replay_init_lock);

	sFindFileCnt --;
	if(sFindFileCnt < 0)
		sFindFileCnt = 0x7FFFFFFF;
	CV_QueryInfo.insert(pair<S32, CQuery *>(sFindFileCnt, pCQueryHandle));
	*phandle = (U32)sFindFileCnt;

	pthread_mutex_unlock(&s_replay_init_lock);

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_querynext(U32 handle, LPCV_RECORD_SEGDATA pSegData)
{
	S32 lRet;
	QueryMap::iterator	iter;

	CQuery				*pCFindFileHandle;

	if(pSegData == NULL)
	{
		LOGE("pFindCond null\n");
		return CV_ERR_REC_INVALID_PARA;
	}

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_QueryInfo.find(handle);
	if(iter == CV_QueryInfo.end())
	{
		LOGE("lFindHandle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pCFindFileHandle = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);

	lRet = pCFindFileHandle->FindNextSeg(pSegData);
	if(CV_SUCCESS != lRet)
	{
		//LOGE("Type err : %d\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}
	
	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_getdatasize(U32 handle,CV_RECORD_QUESTPARA param,S64 *pDataSize)
{
	QueryMap::iterator	iter;

	CQuery				*pCFindFileHandle;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_QueryInfo.find(handle);
	if(iter == CV_QueryInfo.end())
	{
		LOGE("lFindHandle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pCFindFileHandle = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	if(pDataSize)
		*pDataSize = pCFindFileHandle->GetDataSize(param);

	return CV_SUCCESS;

}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_getiframecount(U32 handle,CV_RECORD_QUESTPARA param,S64 *pCount)
{
	QueryMap::iterator	iter;

	CQuery				*pCFindFileHandle;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_QueryInfo.find(handle);
	if(iter == CV_QueryInfo.end())
	{
		LOGE("lFindHandle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pCFindFileHandle = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	if(pCount)
		*pCount = pCFindFileHandle->GetIFrameCount(param);

	return CV_SUCCESS;

}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_queryclose(U32 handle)
{
	QueryMap::iterator	iter;
	CQuery				*pCFindFileHandle;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_QueryInfo.find(handle);
	if(iter == CV_QueryInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pCFindFileHandle = iter->second;
	CV_QueryInfo.erase(iter);

	pthread_mutex_unlock(&s_replay_init_lock);
	
	delete pCFindFileHandle;

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_create(U32 *phandle, S32 lChannelNum, U32 *uChannel, time_t tStartTime, time_t tEndTime, fHistStreamCallBackV2 PopCallBack, S32 StreamFd, S32 StreamType, S32 lRecordType, void* lpUser)
{
	CDiskPopM*					pStreamHandel;
	S32							i, lRet;
	
	pStreamHandel = CPopMgr::GetCPopMgrItem()->GetOneCDiskPopM();
	if(pStreamHandel == NULL)
	{
		LOGE("No resource\n");
		return CV_ERR_REC_MALLOC_FAIL;
	}

	lRet = pStreamHandel->SetAttr(DISKGROUP_NORMAL, 0xFFFFFFFF, tStartTime, tEndTime, PopCallBack, StreamType, lpUser, FALSE);
	if(lRet < 0)
	{
		LOGE("Set DiskPop Attr err\n");
		CPopMgr::GetCPopMgrItem()->ReleaseCDiskPopM(pStreamHandel);
		pStreamHandel = NULL;
		return CV_ERR_REC_OPERATE_FAIL;
	}
	
	//for(i = 0; i < MAX_RECORDTYPE; i ++)
	{
		lRet = pStreamHandel->DPM_AddRecType(lRecordType);
		if(lRet < 0)
		{
			LOGD("Add DiskPop RecType err:i = %d", i);
			CPopMgr::GetCPopMgrItem()->ReleaseCDiskPopM(pStreamHandel);
			pStreamHandel = NULL;
			return CV_ERR_REC_OPERATE_FAIL;
		}
	}
	for(i = 0; i < lChannelNum; i ++)
	{
		lRet = pStreamHandel->AddCh(uChannel[i], 0);
		if(lRet < 0)
		{
			LOGE("Add DiskPop Ch err\n");
			CPopMgr::GetCPopMgrItem()->ReleaseCDiskPopM(pStreamHandel);
			pStreamHandel = NULL;
			return CV_ERR_REC_OPERATE_FAIL;
		}
		pStreamHandel->AddChannelHandle(uChannel[i], StreamFd);
	}
	
	pthread_mutex_lock(&s_replay_init_lock);
	
	sReplayCnt --;
	if(sReplayCnt < 0)
		sReplayCnt = 0x7FFFFFFF;
	CV_ReplayInfo.insert(pair<S32, CDiskPopM *>(sReplayCnt, pStreamHandel));
	*phandle = (U32)sReplayCnt;

	pthread_mutex_unlock(&s_replay_init_lock);

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_start(U32 handle)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;
	S32							lRet;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pStreamHandel = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	lRet = pStreamHandel->StartPop(FALSE);
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_stop(U32 handle)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;
	S32							lRet;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pStreamHandel = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	lRet = pStreamHandel->StopPop();
	if(lRet !=0)
	{
		LOGE("stop replay err : ret = 0x%x\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}
	
	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_seek(U32 handle, time_t tSeekTime)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;
	S32							lRet;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pStreamHandel = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	lRet = pStreamHandel->StopPop();
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
		
		return CV_ERR_REC_OPERATE_FAIL;
	}
	lRet = pStreamHandel->Seek(tSeekTime);
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}
	lRet = pStreamHandel->StartPop(FALSE);
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_setdir(U32 handle, U32 dir)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;
	S32							lRet;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");

		pthread_mutex_unlock(&s_replay_init_lock);
		
		return CV_ERR_REC_INVALID_PARA;
	}

	pStreamHandel = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	lRet = pStreamHandel->StopPop();
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}
	lRet = pStreamHandel->SetPopDataDirection(dir);
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
	}
	lRet = pStreamHandel->StartPop(FALSE);
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
		return CV_ERR_REC_OPERATE_FAIL;
	}

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_getpopdatesize(U32 handle,S64 *pDataSize)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pStreamHandel = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	if(pDataSize)
		*pDataSize = pStreamHandel->GetSize();
		
	return CV_SUCCESS;

}


FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_setpoptype(U32 handle, U32 onlykeyframe)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;
	S32							lRet;

	pthread_mutex_lock(&s_replay_init_lock);

	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}

	pStreamHandel = iter->second;

	pthread_mutex_unlock(&s_replay_init_lock);
	
	lRet = pStreamHandel->SetPopDataType(onlykeyframe, 0);
	if(lRet !=0)
	{
		LOGE("start replay err : ret = 0x%x\n", lRet);
	}

	return CV_SUCCESS;
}

FOX_HELPER_DLL_EXPORT CV_ERR cv_soft::cv_replay_release(U32 handle)
{
	ReplayMap::iterator			iter;
	CDiskPopM*					pStreamHandel;

	pthread_mutex_lock(&s_replay_init_lock);
	iter = CV_ReplayInfo.find(handle);
	if(iter == CV_ReplayInfo.end())
	{
		LOGE("Handle err\n");
		pthread_mutex_unlock(&s_replay_init_lock);
		return CV_ERR_REC_INVALID_PARA;
	}
	
	pStreamHandel = iter->second;

	CV_ReplayInfo.erase(iter);

	pthread_mutex_unlock(&s_replay_init_lock);
		
	CPopMgr::GetCPopMgrItem()->ReleaseCDiskPopM(pStreamHandel);
	pStreamHandel = NULL;
		
	return CV_SUCCESS;
}

