#include "CDiskPopM.h"

extern RECORD_CONTEXT_T m_gRecordInfo;
using namespace cv_soft;

CDiskPopM::CDiskPopM()
{
	S32 i;
	dev = NULL;
	for(i = 0; i < CV_MAX_LOCAL_CH_NUM; i++)
	{
		m_nBindCh[i] = 0;
		ChCnt[i] = 0;
	}

	for(i = 0; i < CV_MAX_LOCAL_CH_NUM * 2; i++)
	{
		lChHandle[i] = -1;
	}

	for(i = 0; i < MAX_RECORD_TYPE; i++)
	{
		m_RecordType[i] = 0;
	}
	nDevNo = -1;
	tCurPopTime = 0;
	m_bDirect = 0;

	m_CallBack = NULL;
	m_EndCallBack = NULL;
	m_tStartSeekTime = 0;
	m_tEndSeekTime = 0;
}

CDiskPopM::~CDiskPopM()
{
	Release();
}
time_t  CDiskPopM::GetCurFrameTime()
{
	return tCurPopTime;
}

S32 CDiskPopM::CallBack(void* hDataPopper, S32 iChannel,S32 bIndexFrame, U16 wFrameType, time_t FrameTime, U8 *pBuf, U32 dwSize, U32 EventID,void *UserContext)
{
	CDiskPopM *Handle = (CDiskPopM *)UserContext;
	CvFrameHeader tmp;
	//S32   lChannel;
	U32 dwRealSize;
	int nRet = -1;

	//lChannel = iChannel + CV_MAX_LOCAL_CH_NUM;
	if(wFrameType == 0)
	{
		if(EventID == DATAPOP_EVENT_TIME_OVER)
		{
			if(Handle->m_CallBack != NULL)
			{
				CvFrameHeader struHeader;
				memset(&struHeader, 0, sizeof(struHeader));
				struHeader.uiStartId = FRAME_STARTCODE;
				struHeader.uiFrameType = CvPktIFrames;
				nRet = Handle->m_CallBack(Handle->lChHandle[iChannel],iChannel,0,bIndexFrame, (U8 *)&struHeader, sizeof(struHeader), &Handle->bExit, Handle->lpUser);
			}
			
			Handle->bPopEnd = 1;
			LOGD("Pop end\n");

			if(Handle->m_EndCallBack != NULL)
			{

				Handle->m_EndCallBack(Handle->lChHandle[iChannel], iChannel);

				if (Handle->ChCnt[iChannel] == 3 || Handle->ChCnt[iChannel] == 2)
				{
				    Handle->m_EndCallBack(Handle->lChHandle[iChannel], iChannel);
				}
			}
		}
		else if(EventID == DATAPOP_EVENT_TIME_JUMP)
		{
#if (defined PLATFORM_JZT32)
			LOGD("Module_StreamQueue_ClearData begin\n");
			Module_StreamQueue_ClearData(m_gRecordInfo.ModeHandle, Handle->lChHandle[iChannel]);
			LOGD("Module_StreamQueue_ClearData end\n");
#endif
		}
		return 0;
	}
	
	if(FrameTime < Handle->m_tStartSeekTime)
	{
		LOGD("FrameTime=[%d], Handle->m_tStartSeekTime=[%d]\n", FrameTime, Handle->m_tStartSeekTime);
		return -1;
	}

    if(FrameTime > Handle->m_tEndSeekTime)
    {
    	LOGD("FrameTime=[%d], Handle->m_tStartSeekTime=[%d]\n", FrameTime, Handle->m_tEndSeekTime);
		return -1;
    }
	
	Handle->tCurPopTime = FrameTime;
	if(0 == Handle->m_nBindCh[iChannel])
	{
		LOGD("callback ch = %d, wFrameType = %d here\n", iChannel, wFrameType);
		return -1;
	}
	if(pBuf == NULL || dwSize <= sizeof(CvFrameHeader))
	{
		LOGD("callback ch = %d, wFrameType = %d here\n", iChannel, wFrameType);
		return -1;
	}
	memcpy((S8*)&tmp, (S8*)pBuf, sizeof(CvFrameHeader));
	if(tmp.uiStartId != FRAME_STARTCODE &&
	   tmp.uiStartId != MOTION_STARTCODE)
	{
		LOGD("tmp.uiStartId=[0x%x]\n",tmp.uiStartId);
		return -1;
	}
	dwRealSize = tmp.uiFrameLen + sizeof(CvFrameHeader);
	if(tmp.uiFrameType != CvPktAudioFrames)
	{
		if((dwRealSize > dwSize) || (dwSize - dwRealSize > 3))
		{
			LOGE("Frame err : Type = %d, dwSize= %d, dwRealSize = %d\n", tmp.uiFrameType, dwSize, dwRealSize);
			return -1;
		}
	}
	else
	{
	//	printf("[%x] uiFrameType = %d\n",tmp.uiStartId,tmp.uiFrameType);
		dwRealSize = dwSize;
	}

	if(tmp.uiFrameType == CvPktAudioFrames && access("/tmp/noaudio",F_OK) == 0)
	{
		return 0;
	}

	if(Handle->m_CallBack != NULL)
	{
		if(tmp.uiFrameType == CvPktIFrames || tmp.uiFrameType == CvPktPFrames)
		{
			if(Handle->FrameNo_Main[iChannel] > tmp.uiFrameNo)
			{
				LOGW("uiFrameNo:[%d,%d],s32StreamType:%d\n",Handle->FrameNo_Main[iChannel],tmp.uiFrameNo,Handle->s32StreamType);
				//return 0;
			}
			if(Handle->s32StreamType == 0 || (Handle->s32StreamType & 0x1))
			{
		   		nRet = Handle->m_CallBack(Handle->lChHandle[iChannel],iChannel,0,bIndexFrame, pBuf, dwRealSize, &Handle->bExit, Handle->lpUser);
			}
			Handle->u64PopSize += dwSize;
			Handle->FrameNo_Main[iChannel] = tmp.uiFrameNo;
		}
		else if(tmp.uiFrameType == CvPktSubIFrames || tmp.uiFrameType == CvPktSubPFrames)
		{
			if(Handle->FrameNo_Aux[iChannel] > tmp.uiFrameNo)
			{
				LOGW("uiFrameNo:[%d,%d],s32StreamType:%d\n",Handle->FrameNo_Aux[iChannel],tmp.uiFrameNo,Handle->s32StreamType);
				//return 0;
			}
			if(Handle->s32StreamType == 0 || (Handle->s32StreamType & 0x2))
			{
		   		nRet = Handle->m_CallBack(Handle->lChHandle[iChannel],iChannel,0,bIndexFrame, pBuf, dwRealSize, &Handle->bExit, Handle->lpUser);
			}
			Handle->u64PopSize += dwSize;
			Handle->FrameNo_Aux[iChannel] = tmp.uiFrameNo;
		}
		else if(tmp.uiFrameType == CvPktAudioFrames)
		{
			if(Handle->FrameNo_Audio[iChannel] > tmp.uiFrameNo)
			{
				LOGW("uiFrameNo:[%d,%d],s32StreamType:%d\n",Handle->FrameNo_Aux[iChannel],tmp.uiFrameNo,Handle->s32StreamType);
				//return 0;
			}
			if(Handle->s32StreamType == 0 || (Handle->s32StreamType & 0x4))
			{
		   		nRet = Handle->m_CallBack(Handle->lChHandle[iChannel],iChannel,0,bIndexFrame, pBuf, dwRealSize, &Handle->bExit, Handle->lpUser);
			}
			Handle->u64PopSize += dwSize;
			Handle->FrameNo_Audio[iChannel] = tmp.uiFrameNo;
		}
		else
		{
			if(Handle->FrameNo_Smart[iChannel] > tmp.uiFrameNo)
			{
				LOGW("uiFrameNo:[%d,%d],s32StreamType:%d\n",Handle->FrameNo_Smart[iChannel],tmp.uiFrameNo,Handle->s32StreamType);
				//return 0;
			}
			nRet = Handle->m_CallBack(Handle->lChHandle[iChannel],iChannel,0,bIndexFrame, pBuf, dwRealSize, &Handle->bExit, Handle->lpUser);
			Handle->u64PopSize += dwSize;
			Handle->FrameNo_Smart[iChannel] = tmp.uiFrameNo;
		}
	}
	else
	{
		LOGE("Handle->m_CallBack is null\n");
		nRet = -1;
	}

	return nRet;
}

void CDiskPopM::SetDevNo(S32 nPopDevNo)
{
	nDevNo = nPopDevNo;
}

S32 CDiskPopM::GetDevNo()
{
	return nDevNo;
}

S32 CDiskPopM::SetAttr(S32 DiskGroupMask, S32 nRefChan, time_t tStartSeekTime, time_t tEndSeekTime, fHistStreamCallBackV2 PopCallBack, S32 StreamType, void* lpUserContext, S32 bDynPop)
{
	S32 ret = 0;
	S32 i;
	time_t tCurrStartTime, tCurrEndTime;
	if(tStartSeekTime > tEndSeekTime)
	{
		tCurrStartTime = tEndSeekTime;
		tCurrEndTime = tStartSeekTime;
		m_bDirect = 1;
	}
	else
	{
		tCurrStartTime = tStartSeekTime;
		tCurrEndTime = tEndSeekTime;
		m_bDirect = 0;
	}

	m_tStartSeekTime = tCurrStartTime;
	m_tEndSeekTime = tCurrEndTime;

	if( NULL != dev)
	{
		ANTS_AviFile_DataPopStop(dev);
		ANTS_AviFile_DataPopRelease(dev);
		dev = NULL;
	}
	if(ANTS_AviFile_DataPopCreate(&dev,nRefChan,((tCurrStartTime < 10) ? 0 : (tCurrStartTime - 10)),tCurrEndTime+10,CallBack, bDynPop,this))
	{
		LOGE("ANTS_AviFile_DataPopCreate failed\n");
	}
	if(NULL == dev)
	{
		LOGE("PopM create failed\n");
		ret = -1;
	}
	else
	{
		LOGD("Create OK:dev = %p\n", dev);

		lpUser = lpUserContext;
		m_CallBack = PopCallBack;
		u64PopSize = 0;
		s32StreamType = StreamType;
		bExit = 0;
		bPopEnd = 0;
		tCurPopTime = tStartSeekTime;

		for(i = 0; i < CV_MAX_LOCAL_CH_NUM; i ++)
		{
			if(1 == m_nBindCh[i])
			{
				LOGD("Add ch %d\n", i);
				ret = ANTS_AviFile_DataPopChanAdd(dev,i);
				u64PopSize = 0;
			}
			FrameNo_Main[i] = 0;
			FrameNo_Aux[i] = 0;
			FrameNo_Audio[i] = 0;
			FrameNo_Smart[i] = 0;
		}

		for(i = 0; i < MAX_RECORD_TYPE; i ++)
		{
			if(0 != m_RecordType[i])
			{
				LOGE("添加录像类型%d\n", i);
				ret = ANTS_AviFile_DataPopRecTypeAdd(dev, m_RecordType[i]);
				if(ret != 0)
				{
					LOGE("RecType = %d add err: errcode = %p\n", m_RecordType[i], ret);
					return -1;
				}
			}
		}

	}

	return (S32)ret;
}


S32 CDiskPopM::SetPopEndCallback(fPopEndCallBack cbEndCallback)
{
	m_EndCallBack = cbEndCallback;

	return 0;
}

S32 CDiskPopM::AddCh(S32 nChannel, S32 nStatus)
{
	S32 ret = 0;
    S32 lChannel;

	if(NULL == dev)
	{
		LOGE("PopM dev is NULL\n");
		return -1;
	}

	if(CV_MAX_LOCAL_CH_NUM * 2 <= nChannel ||nChannel < 0 )
	{
		return -1;
	}

	lChannel = nChannel < CV_MAX_LOCAL_CH_NUM ? nChannel : nChannel - CV_MAX_LOCAL_CH_NUM;

	if (ChCnt[lChannel] == 0)
	{
        if (nChannel < CV_MAX_LOCAL_CH_NUM)
        {
            ChCnt[lChannel] = 1;
		}
		else
		{
            ChCnt[lChannel] = 2;
		}
	}

	if (ChCnt[lChannel] == 1 && nChannel >= CV_MAX_LOCAL_CH_NUM && nStatus == 0)
	{
        ChCnt[lChannel] = 3;
	}

	if (ChCnt[lChannel] == 2 && nChannel < CV_MAX_LOCAL_CH_NUM && nStatus == 0)
	{
        ChCnt[lChannel] = 3;
	}

	if (ChCnt[lChannel] == 1 && nChannel >= CV_MAX_LOCAL_CH_NUM && nStatus == 1)
	{
        ChCnt[lChannel] = 2;
		lChHandle[lChannel] = -1;
	}

	if (ChCnt[lChannel] == 2 && nChannel < CV_MAX_LOCAL_CH_NUM && nStatus == 1)
	{
        ChCnt[lChannel] = 1;
		lChHandle[lChannel + CV_MAX_LOCAL_CH_NUM] = -1;
	}

	m_nBindCh[lChannel] = 1;
	if ((ChCnt[lChannel] == 1 || ChCnt[lChannel] == 2) && nStatus == 0)
	{
		ret = ANTS_AviFile_DataPopStop(dev);
		ret = ANTS_AviFile_DataPopChanAdd(dev, lChannel);
		if(ret != 0)
		{
			LOGE("ANTS_AviFile_DataPopChanAdd failed,ret:%d\n",ret);
			return ret;
		}
		ret = ANTS_AviFile_DataPopTimeSeek(dev, tCurPopTime);
	}
	return ret;
}

S32 CDiskPopM::AddChannelHandle(S32 nChannel, S32 lChannelHandle)
{
	if(NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}

	if(CV_MAX_LOCAL_CH_NUM * 2 <= nChannel || nChannel < 0)
	{
		LOGE("nChannel : %d\n",nChannel);
		return -1;
	}

	lChHandle[nChannel] = lChannelHandle;

	return 0;
}

S32 CDiskPopM::DelCh(S32 nChannel)
{
	S32 ret = 0;
    S32 lChannel;

	if(NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}

	if(CV_MAX_LOCAL_CH_NUM * 2<= nChannel || nChannel < 0)
	{
		LOGE("nChannel : %d\n",nChannel);
		return -1;
	}
	LOGD("Del ch %d",nChannel);
	lChannel = nChannel < CV_MAX_LOCAL_CH_NUM ? nChannel : nChannel - CV_MAX_LOCAL_CH_NUM;

	if(ChCnt[lChannel] == 3)
	{
        if (nChannel < CV_MAX_LOCAL_CH_NUM)
        {
            ChCnt[lChannel] = 2;
		}
		else
		{
            ChCnt[lChannel] = 1;
		}
	}

	if (ChCnt[lChannel] == 1 && nChannel < CV_MAX_LOCAL_CH_NUM)
	{
        ChCnt[lChannel] = 0;
	}

	if (ChCnt[lChannel] == 2 && nChannel >= CV_MAX_LOCAL_CH_NUM)
	{
        ChCnt[lChannel] = 0;
	}

	lChHandle[nChannel] = -1;

	if (ChCnt[lChannel] == 0)
	{
	    m_nBindCh[lChannel] = 0;
		ret = ANTS_AviFile_DataPopChanDel(dev, lChannel);
	}
	return (S32)ret;
}

S32 CDiskPopM::Seek(time_t SeekTime)
{
	S32 ret = 0;

	if ( NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	
	if(SeekTime < m_tStartSeekTime || SeekTime > m_tEndSeekTime)
	{
		LOGE("SeekTime=[%d],m_tStartSeekTime=[%d],m_tEndSeekTime=[%d]\n",SeekTime,m_tStartSeekTime,m_tEndSeekTime);
		return -1;
	}
	
	ret = ANTS_AviFile_DataPopTimeSeek(dev, SeekTime);

	if(ret == 0)
		bPopEnd = 0;

	return (S32)ret;
}

S64 CDiskPopM::GetSize()
{
	S64 ret = 0;
	U64 nSize = 0;

	if ( NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	ret = ANTS_AviFile_DataPopGetDataSize(dev, &nSize);

	//nSize >>= 10;
	if (ret == 0)
	{
		ret = nSize;
	}
	else
	{
		LOGD("ret = %d\n",ret);
		ret = 0;
	}

	return ret;
}


S32 CDiskPopM::StartPop(U32 bGetSize)
{
	S32 ret = 0;

	if ( NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	bExit = 0;
	if(bGetSize)
		ANTS_AviFile_DataPopGetDataSize(dev, &u64TotalSize);
	ret = ANTS_AviFile_DataPopStart(dev);
	for(S32 i = 0; i < CV_MAX_LOCAL_CH_NUM; i ++)
	{
		FrameNo_Main[i] = 0;
		FrameNo_Aux[i] = 0;
		FrameNo_Audio[i] = 0;
		FrameNo_Smart[i] = 0;
	}

	return (S32)ret;
}

S32 CDiskPopM::DPM_AddRecType( S32 RecType)
{
	S32 ret = 0;
	S32 i   = 0;

	if(NULL == dev)
	{
		LOGE("Pop Dev is not exist\n");
		return -1;
	}
	/*
	if(RecType == 0)
	{
		LOGE("RecType = 0\n");
		return -1;
	}
	*/
	ret = ANTS_AviFile_DataPopRecTypeAdd(dev, RecType);
	if(ret != 0)
	{
		LOGE("RecType = %d add err: errcode = %p\n", RecType, ret);
		return -1;
	}

	for ( i = 0; i < MAX_RECORD_TYPE; i++)
	{
		if ( m_RecordType[i] == RecType)
		{
			break;
		}
	}

	if (i >= MAX_RECORD_TYPE)
	{
		//当前的录像类型没有添加到记录表内
		for ( i = 0; i < MAX_RECORD_TYPE; i++)
		{
			if ( m_RecordType[i] == 0 )
			{
				m_RecordType[i] = RecType;
				break;
			}
		}
	}

	return (S32)ret;
}


S32 CDiskPopM::DPM_DelRecType(S32 RecType)
{
	S32 ret = 0;
	S32 i   = 0;

	if(NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	ret = ANTS_AviFile_DataPopRecTypeDel(dev, RecType);
	for ( i = 0; i < MAX_RECORD_TYPE; i++)
	{
		if ( m_RecordType[i] == RecType)
		{
			m_RecordType[i] = 0;
			break;
		}
	}

	return (S32)ret;

}

S32 CDiskPopM::StopPop()
{
	S32 ret = 0;

	if(NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	bExit = 1;
	ret = ANTS_AviFile_DataPopStop(dev);
	for(S32 i = 0; i < CV_MAX_LOCAL_CH_NUM; i ++)
	{
		FrameNo_Main[i] = 0;
		FrameNo_Aux[i] = 0;
		FrameNo_Audio[i] = 0;
		FrameNo_Smart[i] = 0;
	}
	return (S32)ret;
}

S32 CDiskPopM::Release()
{
	S32 i, ret = 0;

	if(NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	bExit = 1;
	ret = ANTS_AviFile_DataPopStop(dev);
	ANTS_AviFile_DataPopRelease(dev);
	dev = NULL;
	for(i = 0; i < CV_MAX_LOCAL_CH_NUM; i++)
	{
		m_nBindCh[i] = 0;
		ChCnt[i] = 0;
	}

	for ( i = 0; i < CV_MAX_LOCAL_CH_NUM * 2; i++)
	{
		lChHandle[i] = -1;
	}
	for(i = 0; i < MAX_RECORD_TYPE; i++)
	{
		m_RecordType[i] = 0;
	}

	m_CallBack = NULL;
	m_EndCallBack = NULL;
	return (S32)ret;
}

S32 CDiskPopM::SetPopDataType(S32 nDataType,U32 PopKeyInterval)
{
	S32 ret = 0;

	if(NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	if(m_bDirect)
	{
		if(!nDataType)
			nDataType = 1;
	}
	if(nDataType != 0)
		ret = ANTS_AviFile_DataPopSetKeyFrame(dev, TRUE, 3);
	else
		ret = ANTS_AviFile_DataPopSetKeyFrame(dev, FALSE, 0);

	return (S32)ret;
}

S32 CDiskPopM::SetPopDataDirection(U32 bFront)
{
	S32 ret = 0;

	if ( NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	ret = ANTS_AviFile_DataPopSetDirect(dev,bFront);
	return (S32)ret;
}

S32 CDiskPopM::GetTotalSize(U64 *u64Size, U32 bRealGet)
{
	if ( NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}

	if(!bRealGet)
		*u64Size = u64TotalSize;
	else
	{
		U64 TmpSize = 0;
		ANTS_AviFile_DataPopGetDataSize(dev, &TmpSize);
		TmpSize >>= 10;
		*u64Size = TmpSize;
		u64TotalSize = *u64Size;
	}
	return 0;
}

S32 CDiskPopM::GetPopSize(U64 *u64Size)
{
	if ( NULL == dev)
	{
		LOGE("没有设置弹出器的属性\n");
		return -1;
	}
	*u64Size = u64PopSize;
	return 0;
}

