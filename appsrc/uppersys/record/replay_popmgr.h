#ifndef _RECORD_POPMGR_H_
#define	_RECORD_POPMGR_H_

#include "CDiskPopM.h"

#define	MAX_CPOPM_NUM 32

class CPopMgr
{
public:
	static CPopMgr*			GetCPopMgrItem();
	static void 			DelCPopMgrItem();
	U32					Init();
	U32					UnInit();
	CDiskPopM*				GetOneCDiskPopM();
	U32					ReleaseCDiskPopM(CDiskPopM *pHandle);

private:
	CPopMgr();
	~CPopMgr();

	static CPopMgr*			theCPopManager;

	U32					bInit;
	U32					dwCDiskPopMNum;
	U32					bUsedPopM[MAX_CPOPM_NUM];
	CDiskPopM				*pCpopMHandle[MAX_CPOPM_NUM];
};

#endif

