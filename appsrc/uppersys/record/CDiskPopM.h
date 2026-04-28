#ifndef	_CDISKPOPM_H_
#define	_CDISKPOPM_H_

#include <libcommon_api.h>
#include <libmodule_api.h>
#include "record_common.h"
#include "ipcstorage.h"

using namespace cv_soft;

typedef struct
{
	U32 bExit;
	ModuleHandle_T ModeHandle;
}RECORD_CONTEXT_T;

#define MAX_RECORD_TYPE 12

typedef void(*fPopEndCallBack )(S32 lHandle, S32 lChannel);
typedef S32 (*fHistStreamCallBackV2 )(S32 lRemotePlayHandle, U32 dwChannel,U32 dwStreamIdx,U32 bIFrame, U8 *byBuffer, U32 dwSize, U32 *Exit, void* lpUser);

class CDiskPopM{
public:
		CDiskPopM();
		~CDiskPopM();

	void		SetDevNo(S32 nPopDevNo);
	S32			GetDevNo();
	S32			SetAttr(S32 DiskGroupMask, S32 nRefChan, time_t tStartSeekTime, time_t tEndSeekTime, fHistStreamCallBackV2 PopCallBack, S32 StreamType, void* lpUserContext, S32 bDynPop);
	
	S32			SetPopEndCallback(fPopEndCallBack cbEndCallback);
	S32			AddCh(S32 nChannel, S32 nStatus);
	S32			AddChannelHandle(S32 nChannel, S32 lChannelHandle);
	S32			DelCh(S32 nChannel);
	S32			DPM_AddRecType( S32 RecType);
	S64			GetSize();
	S32			Release();
	S32			DPM_DelRecType(S32 RecType);
	S32			Seek(time_t SeekTime);
	S32			StartPop(U32 bGetSize);
	S32			StopPop();
	S32			SetPopDataType(S32 nDataType,U32 PopKeyInterval);
	S32			SetPopDataDirection(U32 bFront);
	S32			GetTotalSize(U64 *u64Size, U32 bRealGet = FALSE);
	S32			GetPopSize(U64 *u64Size);
	time_t      GetCurFrameTime();

	S32			bPopEnd;
	S32			tCurPopTime;
	S32         m_bDirect;
	
	time_t m_tStartSeekTime;
	time_t m_tEndSeekTime;

	static S32	CallBack(void* hDataPopper, S32 iChannel,S32 bIndexFrame, U16 wFrameType, time_t FrameTime, U8 *pBuf, U32 dwSize, U32 EventID,void *UserContext);
private:
	S32			nDevNo;
	void*		dev;
	S8			m_nBindCh[CV_MAX_LOCAL_CH_NUM];
	S32			m_RecordType[MAX_RECORD_TYPE];
	S32		lChHandle[CV_MAX_LOCAL_CH_NUM * 2];
	S32		ChCnt[CV_MAX_LOCAL_CH_NUM];
	U32		FrameNo_Main[CV_MAX_LOCAL_CH_NUM];
	U32		FrameNo_Aux[CV_MAX_LOCAL_CH_NUM];
	U32		FrameNo_Audio[CV_MAX_LOCAL_CH_NUM];
	U32		FrameNo_Smart[CV_MAX_LOCAL_CH_NUM];
	U64		u64PopSize;
	U64		u64TotalSize;

	fHistStreamCallBackV2 m_CallBack;
	fPopEndCallBack		m_EndCallBack;
	void*		lpUser;
	S32		s32StreamType;
	U32		bExit;
};

#endif

