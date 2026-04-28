#ifndef _GLOBALOBJ_H_
#define _GLOBALOBJ_H_

#include "ants_avi_common.h"

#include "writerManage.h"
#include "dataPopManage.h"
#include "storageManage.h"
#include "recIndexManage.h"


class CWriterManage;
class CDataPopManage;
class CStorageManage;
class CRecIndexManage;

class AntsAviLibInfo
{
public:
	AntsAviLibInfo();
	~AntsAviLibInfo();
			
	CWriterManage *antsWriterManage;
	CDataPopManage *antsReaderManage;
	CStorageManage *antsStorageManage;
	CRecIndexManage *antsRecIndexManage;

	int Initialize();
	int DeInit();
	BOOL IsInited();

private:
	BOOL m_bInit;
};

extern AntsAviLibInfo g_antsAviLibInfo;



#endif

