#ifndef _WRITE_MANAGE_H_
#define _WRITE_MANAGE_H_

#include "ants_avi_common.h"

#define WRITE_DBG 0

class CWriterManage
{
public:

	static CWriterManage*	GetMgrItem();
	static void				DelMgrItem();

	CWriterManage();
	~CWriterManage();

	int SetRecFrame(int iChannel, char *pBuffer, int nBufferLen, DWORD dwCurTimeSec, DWORD dwCurTimeUsec);
	int StartRec(int type);
	int StopRec();

	int UpdateRecIndex(BOOL bneedInsertInfo);

	BOOL bIsWritting;

private:

	void InitParam();
	int WriteRemainData();
	int GetRecFileName();

	int WriteAviFile(char *buf, int buf_len);
	int write_file(void *data_buf, DWORD data_len);
	
	static CWriterManage*	theWriterManage;

	BOOL m_start_rec;
	int m_frame_width;
	int m_frame_height;
	int m_codec_id;
	time_t m_cur_date;
	RecIndexInfo m_recindex_info;
	BOOL m_need_updateRecIndex;

	int m_nChannel;

	avis_fileHanedle *m_recfile_fd; //打开录像文件

#if (WRITE_DBG==1)
	int m_fdDebug;
#endif

	int m_cur_rectype;

	int m_iFirstWriteForOpen;

	unsigned char *m_pWriteBuffer;
	int    m_dwWriteBufferSize;

};


#endif


