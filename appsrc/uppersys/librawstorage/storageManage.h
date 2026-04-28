#ifndef _STORAGE_MANAGE_H_
#define _STORAGE_MANAGE_H_

#include "ants_avi_common.h"
#include "mutex.h"


class CStorageManage
{
public:
	static CStorageManage*	GetMgrItem();
	static void				DelMgrItem();

	CStorageManage();
	~CStorageManage();
		
	int GetSDCardHead(avis_SDCardHead *pSDCardhead);
	int SetSDCardHead(avis_SDCardHead *pSDCardhead);
	int SaveSDCardHead();
	int AddRecPartition(PartitionInfo *pInfo);
	int GetDiskStatus(char *pPath, unsigned int *lpdwAllSpace, unsigned int *lpdwFreeSpace);
	int FormatDisk(char *pPath);
	S64 GetPartionSize();

	S64 m_SDCardTotalSize;
	CMutex m_headMutex;

	S64 m_SDCardHeadMaxSize;

	S64 m_RecFileIndexStartAddr;
	S64 m_RecFileIndexMaxSize;

	S64 m_RecFileStartAddr;
	S64 m_RecFileMaxSize;

	BOOL m_bSDCardInit;

	avis_SDCardHead *m_ptSDCardHead;
	BOOL m_bHeadChanged;
private:
	int InitSDCardHead();

	int InitParam();
	
	static CStorageManage*	thestorageManage;

	pthread_t m_threadId;	

	avis_fileHanedle *m_fd;
};


#endif

