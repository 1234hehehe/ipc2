#include "globalObj.h"

AntsAviLibInfo::AntsAviLibInfo()
{
	antsWriterManage = NULL;
	antsReaderManage = NULL;
	antsStorageManage = NULL;
	antsRecIndexManage = NULL;
	m_bInit = FALSE;
}

AntsAviLibInfo::~AntsAviLibInfo()
{
	
}

int AntsAviLibInfo::Initialize()
{
	antsStorageManage = CStorageManage::GetMgrItem();
	antsRecIndexManage = CRecIndexManage::GetMgrItem();

	antsWriterManage = CWriterManage::GetMgrItem();
	antsReaderManage = CDataPopManage::GetMgrItem();

	m_bInit = TRUE;
	
	return avis_ret_ok;
}

int AntsAviLibInfo::DeInit()
{
	m_bInit = FALSE;
	CWriterManage::DelMgrItem();
	CDataPopManage::DelMgrItem();
	CStorageManage::DelMgrItem();
	CRecIndexManage::DelMgrItem();

	return avis_ret_ok;
}

BOOL AntsAviLibInfo::IsInited()
{
	return m_bInit;
}
