#include "replay_query.h"
#include "ipcstorage.h"

using namespace cv_soft;

CQuery::CQuery(CV_RECORD_QUESTPARA QueryInfo)
{
	S32 lRet, i, j;
	S32 lRecordType;
	S32 lRecSegCount;
	PSS_REGSEG *pRecSegHead = NULL, *pRecSegNode = NULL;
	LPCV_RECORD_SEGNODE		pRecordSectionNode, pRecordSectiontmp, pRecordSectiontmp1;
	time_t starttime, stoptime;

	S32 lAllRecordType[MAX_RECORDTYPE] = 
		{
			CV_RECTYPE_TIMER,
			CV_RECTYPE_MOTION,
			CV_RECTYPE_ALARM,
			CV_RECTYPE_MOTIONORALARM,
			CV_RECTYPE_MOTIONANDALARM,
			CV_RECTYPE_COMMAND,
			CV_RECTYPE_MANUAL,
			CV_RECTYPE_IVSDETECT,
			CV_RECTYPE_FACE_DETECT,
			CV_RECTYPE_FIRE_DETECT,
			CV_RECTYPE_VIDEODIAGNOSE
		};

	//S32 lRecordCount[MAX_RECORDTYPE] = {0};
	pRecordSectionHead = NULL;
	pRecordSectionTail = NULL;
	lchannel = QueryInfo.Channel;
	dwSegCnt = 0;

	LOGD("StartTime [%04d-%02d-%02d %02d:%02d:%02d]\n", QueryInfo.StartTime.year, QueryInfo.StartTime.month, QueryInfo.StartTime.day, 
		QueryInfo.StartTime.hour, QueryInfo.StartTime.min, QueryInfo.StartTime.sec);
	LOGD("StopTime  [%04d-%02d-%02d %02d:%02d:%02d]\n", QueryInfo.StopTime.year, QueryInfo.StopTime.month, QueryInfo.StopTime.day, 
		QueryInfo.StopTime.hour, QueryInfo.StopTime.min, QueryInfo.StopTime.sec);

	Common_CommonGm2LinuxTime(&QueryInfo.StartTime, &starttime);
	Common_CommonGm2LinuxTime(&QueryInfo.StopTime, &stoptime);
	
	//if(QueryInfo.QueryMode == 0)
	{
		lRecordType = QueryInfo.QueryMode;
		lRecSegCount = 0;
		lRet = ANTS_AviFile_GetRecSegList(QueryInfo.Channel, (starttime < 300) ? 0 : (starttime - 300), stoptime + 300, lRecordType, &pRecSegHead, &lRecSegCount);
		if(lRet != 0)
		{
			LOGE("Get RecList Err : errcode = 0x%x\n", lRet);
			LOGE("%d, %d, %d, %d, %p, %p\n", QueryInfo.Channel, starttime, stoptime, lRecordType, &pRecSegHead, &lRecSegCount);
			return;
		}
		LOGD("segcnt = %d\n", lRecSegCount);
		dwSegCnt = lRecSegCount;
		pRecSegNode = pRecSegHead;
		for(j = 0; (j < lRecSegCount) && (pRecSegNode != NULL); j ++)
		{
			time_t time1;
			if(pRecSegNode->tEndTime < starttime || pRecSegNode->tBeginTime > stoptime)
			{
				pRecSegNode = pRecSegNode->ptNext;
				continue;
			}
			pRecordSectionNode = new CV_RECORD_SEGNODE;
			if(pRecordSectionNode == NULL)
			{
				LOGE("ch_%d Seg_%d findrecord failed: RECORDSECTION malloc failed\n", lchannel, j);
				continue;
			}
			//LOGD("iRecType = %d\n", pRecSegNode->iRecType);
			pRecordSectionNode->FileType = pRecSegNode->iRecType;
			pRecordSectionNode->key = 0;
			pRecordSectionNode->blocked = 0;
			pRecordSectionNode->FileSize = 0;
			if(pRecSegNode->tBeginTime < starttime)
				Common_Linux2CommonGmTime(starttime, &pRecordSectionNode->StartTime);
			else
				Common_Linux2CommonGmTime(pRecSegNode->tBeginTime, &pRecordSectionNode->StartTime);
			if(pRecSegNode->tEndTime > stoptime)
				Common_Linux2CommonGmTime(stoptime, &pRecordSectionNode->StopTime);
			else
				Common_Linux2CommonGmTime(pRecSegNode->tEndTime, &pRecordSectionNode->StopTime);
			pRecordSectionNode->pNextSection = NULL;
			pRecordSectionNode->pPreSection = NULL;
			time1 = pRecSegNode->tBeginTime;
			pRecSegNode = pRecSegNode->ptNext;
			if(pRecordSectionHead == NULL)
			{
				pRecordSectionHead = pRecordSectionNode;
				pRecordSectionTail = pRecordSectionHead;
			}
			else
			{
				pRecordSectiontmp = pRecordSectionTail;
				do
				{
					time_t time2;
					Common_CommonGm2LinuxTime(&pRecordSectiontmp->StartTime, &time2);
					if(time1 > time2)
					{
						pRecordSectiontmp1 = (LPCV_RECORD_SEGNODE)pRecordSectiontmp->pNextSection;
						if(pRecordSectiontmp1 != NULL)
						{
							pRecordSectionNode->pNextSection = (void*)pRecordSectiontmp1;
							pRecordSectiontmp1->pPreSection = (void*)pRecordSectionNode;
						}
						else
						{
							pRecordSectionTail = pRecordSectionNode;
						}
						pRecordSectionNode->pPreSection = (LPVOID)pRecordSectiontmp;
						pRecordSectiontmp->pNextSection = (LPVOID)pRecordSectionNode;
						break;
					}
					else
					{
						if(pRecordSectiontmp->pPreSection != NULL)
						{
							pRecordSectiontmp = (LPCV_RECORD_SEGNODE)pRecordSectiontmp->pPreSection;
						}
						else
						{
							pRecordSectiontmp->pPreSection = (LPVOID)pRecordSectionNode;
							pRecordSectionNode->pNextSection = (LPVOID)pRecordSectiontmp;
							pRecordSectionNode->pPreSection = NULL;
							pRecordSectionHead = pRecordSectionNode;
							break;
						}
					}
				}				while(pRecordSectiontmp != NULL);
			}
		}
		#if 0
		for(i = 0;i < MAX_RECORDTYPE;i++)
		{
			if(lRecordCount[i] > 0)
			{
				LOGD("%d type %d segcnt = %d\n", i,lAllRecordType[i], lRecordCount[i]);
			}
		}
		#endif
		if(pRecSegHead != NULL)
		{
			lRet = ANTS_AviFile_ReleaseRecSegList(pRecSegHead);
			if(lRet != 0)
			{
				LOGE("Release RecList Err : errcode = 0x%x\n", lRet);
			}
			pRecSegHead = NULL;
		}
		return;
	}
	/*
	for(lRecSegCount = 0, i = 0; i < MAX_RECORDTYPE; i ++)
	{
		if(QueryInfo.QueryMode != 0)
		{
			if((QueryInfo.QueryMode & (0x1 << i)) == 0)
				continue;
		}

		lRecordType = lAllRecordType[i];
		lRet = ANTS_AviFile_GetRecSegList(QueryInfo.Channel, starttime, stoptime, lRecordType, &pRecSegHead, &lRecSegCount);
		if(lRet != 0)
		{
			LOGE("Get RecList Err : errcode = 0x%x\n", lRet);
			LOGE("%d, %d, %d, %d, %p, %p\n", QueryInfo.Channel, starttime, stoptime, lRecordType, &pRecSegHead, &lRecSegCount);
			continue;
		}
		if(lRecSegCount>0)
		{
			LOGD("type %d segcnt = %d\n", lAllRecordType[i], lRecSegCount);
		}
		dwSegCnt += lRecSegCount;
		pRecSegNode = pRecSegHead;
		for(j = 0; (j < lRecSegCount) && (pRecSegNode != NULL); j ++)
		{
			time_t time1;
			pRecordSectionNode = new CV_RECORD_SEGNODE;
			if(pRecordSectionNode == NULL)
			{
				LOGE("ch_%d type_%d Seg_%d findrecord failed: RECORDSECTION malloc failed\n", lchannel, lAllRecordType[i], j);
				continue;
			}
#ifdef PRINT_TIMEVAL
				Common_Time_T t_time;
				S8 szBegin[20] = {0},szEnd[20] = {0};
				
				Common_Linux2CommonGmTime(pRecSegNode->tBeginTime, &t_time);
				sprintf(szBegin,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
				Common_Linux2CommonGmTime(pRecSegNode->tEndTime, &t_time);
				sprintf(szEnd,"%04d-%02d-%02d %02d:%02d:%02d",t_time.year,t_time.month,t_time.day,t_time.hour,t_time.min,t_time.sec);
				LOGW("[%s-%s]\n",szBegin,szEnd);
#endif
			//pRecordSectionNode->FileType = (1<<(lRecordType-1));//i + 1;
			pRecordSectionNode->FileType = pRecSegNode->iRecType;
			pRecordSectionNode->key = 0;
			pRecordSectionNode->blocked = 0;
			pRecordSectionNode->FileSize = 0;
			Common_Linux2CommonGmTime(pRecSegNode->tBeginTime, &pRecordSectionNode->StartTime);
			Common_Linux2CommonGmTime(pRecSegNode->tEndTime, &pRecordSectionNode->StopTime);
			pRecordSectionNode->pNextSection = NULL;
			pRecordSectionNode->pPreSection = NULL;
			time1 = pRecSegNode->tBeginTime;
			pRecSegNode = pRecSegNode->ptNext;
			if(pRecordSectionHead == NULL)
			{
				pRecordSectionHead = pRecordSectionNode;
				pRecordSectionTail = pRecordSectionHead;
			}
			else
			{
			#if 0
				pRecordSectiontmp = pRecordSectionHead;
				do
				{
					time_t time1, time2;
					Common_Common2LinuxTime(&pRecordSectionNode->StartTime, &time1);
					Common_Common2LinuxTime(&pRecordSectiontmp->StartTime, &time2);
					if(time1 < time2)
					{
						pRecordSectiontmp1 = (LPCV_RECORD_SEGNODE)pRecordSectiontmp->pPreSection;
						if(pRecordSectiontmp1 != NULL)
						{
							pRecordSectiontmp1->pNextSection = (void*)pRecordSectionNode;
							pRecordSectionNode->pPreSection = (void*)pRecordSectiontmp1;
						}
						else
						{
							pRecordSectionHead = pRecordSectionNode;
						}
						pRecordSectionNode->pNextSection = (LPVOID)pRecordSectiontmp;
						pRecordSectiontmp->pPreSection = (LPVOID)pRecordSectionNode;
						break;
					}
					else
					{
						if(pRecordSectiontmp->pNextSection != NULL)
							pRecordSectiontmp = (LPCV_RECORD_SEGNODE)pRecordSectiontmp->pNextSection;
						else
						{
							pRecordSectiontmp->pNextSection = (LPVOID)pRecordSectionNode;
							pRecordSectionNode->pPreSection = (LPVOID)pRecordSectiontmp;
							pRecordSectionNode->pNextSection = NULL;
							pRecordSectionTail = pRecordSectionNode;
							break;
						}
					}
				}
				while(pRecordSectiontmp != NULL);
			#else
				pRecordSectiontmp = pRecordSectionTail;
				do
				{
					time_t time2;
					Common_CommonGm2LinuxTime(&pRecordSectiontmp->StartTime, &time2);
					if(time1 > time2)
					{
						pRecordSectiontmp1 = (LPCV_RECORD_SEGNODE)pRecordSectiontmp->pNextSection;
						if(pRecordSectiontmp1 != NULL)
						{
							pRecordSectionNode->pNextSection = (void*)pRecordSectiontmp1;
							pRecordSectiontmp1->pPreSection = (void*)pRecordSectionNode;
						}
						else
						{
							pRecordSectionTail = pRecordSectionNode;
						}
						pRecordSectionNode->pPreSection = (LPVOID)pRecordSectiontmp;
						pRecordSectiontmp->pNextSection = (LPVOID)pRecordSectionNode;
						break;
					}
					else
					{
						if(pRecordSectiontmp->pPreSection != NULL)
						{
							pRecordSectiontmp = (LPCV_RECORD_SEGNODE)pRecordSectiontmp->pPreSection;
						}
						else
						{
							pRecordSectiontmp->pPreSection = (LPVOID)pRecordSectionNode;
							pRecordSectionNode->pNextSection = (LPVOID)pRecordSectiontmp;
							pRecordSectionNode->pPreSection = NULL;
							pRecordSectionHead = pRecordSectionNode;
							break;
						}
					}
				}				while(pRecordSectiontmp != NULL);
			#endif
			}
		}
		if(pRecSegHead != NULL)
		{
			lRet = ANTS_AviFile_ReleaseRecSegList(pRecSegHead);
			if(lRet != 0)
			{
				LOGE("Release RecList Err : errcode = 0x%x\n", lRet);
			}
			pRecSegHead = NULL;
		}
	}
	*/
}

CQuery::~CQuery()
{
	LPCV_RECORD_SEGNODE		pRecordSectionTmp;

	while(pRecordSectionHead != NULL)
	{
		pRecordSectionTmp = pRecordSectionHead;
		pRecordSectionHead = (LPCV_RECORD_SEGNODE)pRecordSectionTmp->pNextSection;
		delete pRecordSectionTmp;
	}
}

S32 CQuery::FindNextSeg(LPCV_RECORD_SEGDATA pSegInfo)
{
	LPCV_RECORD_SEGNODE		pRecordSectionTmp;

	if(pSegInfo == NULL)
	{
		LOGE("pSegInfo is NULL\n");
		return CV_ERR_REC_INVALID_PARA;
	}
	if(dwSegCnt == 0)
	{
		LOGE("FindFile no file exist\n");
		return CV_ERR_REC_INVALID_PARA;
	}
	if(pRecordSectionHead == NULL)
	{
		//LOGE("FindFile End\n");
		return CV_ERR_REC_INVALID_PARA;
	}

	memcpy((void*)pSegInfo, (void*)pRecordSectionHead, sizeof(CV_RECORD_SEGDATA));
	pRecordSectionTmp = pRecordSectionHead;
	pRecordSectionHead = (LPCV_RECORD_SEGNODE)pRecordSectionTmp->pNextSection;
	if(pRecordSectionHead != NULL)
		pRecordSectionHead->pPreSection = NULL;
	delete pRecordSectionTmp;

	return CV_SUCCESS;
}

S64 CQuery::GetDataSize(CV_RECORD_QUESTPARA QueryInfo)
{
	S32 lRecordType = 0;
	U64 TotalSize = 0,DataSize = 0;
	time_t starttime = 0, stoptime = 0;

	S32 lAllRecordType[MAX_RECORDTYPE] = 
		{
			CV_RECTYPE_TIMER,
			CV_RECTYPE_MOTION,
			CV_RECTYPE_ALARM,
			CV_RECTYPE_MOTIONORALARM,
			CV_RECTYPE_MOTIONANDALARM,
			CV_RECTYPE_COMMAND,
			CV_RECTYPE_MANUAL,
			CV_RECTYPE_IVSDETECT,
			CV_RECTYPE_FACE_DETECT,
			CV_RECTYPE_FIRE_DETECT,
			CV_RECTYPE_VIDEODIAGNOSE
		};
	Common_CommonGm2LinuxTime(&QueryInfo.StartTime, &starttime);
	Common_CommonGm2LinuxTime(&QueryInfo.StopTime, &stoptime);
	//if(QueryInfo.QueryMode == 0)
	{
		lRecordType = QueryInfo.QueryMode;
		ANTS_AviFile_GetDataSize(QueryInfo.Channel, starttime, stoptime, lRecordType, &TotalSize);
	}
	/*
	else
	{
		lRecordType = QueryInfo.QueryMode;
		ANTS_AviFile_GetDataSize(QueryInfo.Channel, starttime, stoptime, lRecordType, &DataSize);
		TotalSize += DataSize;
	}
	*/
	//TotalSize >>= 10;
	return TotalSize;
}

S64 CQuery::GetIFrameCount(CV_RECORD_QUESTPARA QueryInfo)
{
	S32 lRecordType = 0;
	U64 nTotalCount = 0,nCount = 0;
	time_t starttime = 0, stoptime = 0;

	S32 lAllRecordType[MAX_RECORDTYPE] = 
		{
			CV_RECTYPE_TIMER,
			CV_RECTYPE_MOTION,
			CV_RECTYPE_ALARM,
			CV_RECTYPE_MOTIONORALARM,
			CV_RECTYPE_MOTIONANDALARM,
			CV_RECTYPE_COMMAND,
			CV_RECTYPE_MANUAL,
			CV_RECTYPE_IVSDETECT,
			CV_RECTYPE_FACE_DETECT,
			CV_RECTYPE_FIRE_DETECT,
			CV_RECTYPE_VIDEODIAGNOSE
		};
	Common_CommonGm2LinuxTime(&QueryInfo.StartTime, &starttime);
	Common_CommonGm2LinuxTime(&QueryInfo.StopTime, &stoptime);
	//if(QueryInfo.QueryMode == 0)
	{
		lRecordType = QueryInfo.QueryMode;
		ANTS_AviFile_GetIFrameCount(QueryInfo.Channel, starttime, stoptime, lRecordType, &nTotalCount);
	}
	/*
	else
	{
		lRecordType = QueryInfo.QueryMode;
		ANTS_AviFile_GetIFrameCount(QueryInfo.Channel, starttime, stoptime, lRecordType, &nCount);
		nTotalCount += nCount;
	}
	*/
	return nTotalCount;
}

