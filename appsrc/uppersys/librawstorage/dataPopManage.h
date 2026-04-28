#ifndef _READER_MANAGE_H_
#define _READER_MANAGE_H_

#include "ants_avi_common.h"
#include "datapop.h"

#define MAX_AVIS_HANDLE_CNT	(1024)


#define HANDLE_BUSY	0
#define HANDLE_IDLE	1
#define HANDLE_WAIT	2

class CDataPopManage
{
public:
	static CDataPopManage*	GetMgrItem();
	static void				DelMgrItem();

	CDataPopManage();
	~CDataPopManage();

	int DataPopCreate(int &nIdx, int nDevIdx, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime,
		ANTS_AVI_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext, BOOL bNeedSync);
	int DataPopChanAdd(int nIdx, int iChannel);
	int DataPopChanDel(int nIdx, int iChannel);
	int DataPopRecTypeAdd(int nIdx, unsigned int iRecType);
	int DataPopRecTypeDel(int nIdx, unsigned int iRecType);
	int DataPopTimeSeek(int nIdx, time_t SeekTime);
	int DataPopSetDirect(int nIdx, BOOL bSequential);
	int DataPopSetKeyFrame(int nIdx, BOOL bKeyFrame, int iSpeed);
	int DataPopGetDataSize(int nIdx, unsigned long long *pSize);
	int DataPopStart(int nIdx);
	int DataPopStop(int nIdx);
	int DataPopRelease(int nIdx);

	CDataPop *m_data_reader[PSS_MAX_DATAPOPER];
	char m_bIdle[PSS_MAX_DATAPOPER];
	
private:
	static CDataPopManage*	theReaderManage;

	void InitParam();

	CMutex m_userMutex;
};


#endif

