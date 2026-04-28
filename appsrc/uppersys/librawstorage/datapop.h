#ifndef _READER_H_
#define _READER_H_

#include "ants_avi_common.h"
#include "mutex.h"

typedef enum edata_pop_state
{
	epop_state_create,
	epop_state_start,
	epop_state_stop,
	epop_state_destroy,
}edata_pop_state;

typedef enum frame_read_mod_e
{
	READ_ONLY_I	 = 1,
	READ_ONLY_VIDEO = 2,
	READ_ALL_DATA = 3,
	READ_ONLY_AUDIO = 4,
}frame_read_mod_e;

class CDataPop
{
public:
	CDataPop();
	~CDataPop();
	
	int DataPopCreate(int nIdx, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime, ANTS_AVI_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext, BOOL bNeedSync);

	int DataPopChanAdd(int iChannel);
	int DataPopChanDel(int iChannel);
	int DataPopRecTypeAdd(unsigned int iRecType);
	int DataPopRecTypeDel(unsigned int iRecType);
	int DataPopTimeSeek(time_t SeekTime);
	int DataPopSetDirect(BOOL bSequential);
	int DataPopSetKeyFrame(BOOL bKeyFrame, int iSpeed);
	int DataPopGetDataSize(unsigned long long *pSize);
	int DataPopStart();
	int DataPopStop();
	int DataPopRelease();

private:
	int InitParam();

public:
	avis_fileHanedle *m_readfd;

	time_t m_seek_start_time;
	time_t m_seek_end_time;
	ANTS_AVI_DATAPOP_CALLBACK m_playCallback;
	void *m_pUserData;
	HANDLE m_hParenHandle;

	unsigned char  *m_pReadBuffer;
	S64   m_dwReadBufferSize; //缓冲中数据的大小
	S64   m_dwReadPos; //缓冲中起始位置

	edata_pop_state m_epop_state;
	BOOL m_readDir;

	frame_read_mod_e m_read_mod;
	int m_iRealSpeed;
	int m_iCurSpeed;

	unsigned int m_rec_type_mask;

	BOOL m_bNeedReLocate;

	S64 m_CurReadPosInSDCard;

private:
	pthread_t m_threadId;
	BOOL m_dyn_pop;	// 是否动态弹出，回放为true，备份为false

};

#endif
