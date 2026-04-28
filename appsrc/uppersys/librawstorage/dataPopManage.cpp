#include "dataPopManage.h"


CDataPopManage* CDataPopManage::theReaderManage = NULL;


CDataPopManage::CDataPopManage()
{
}

CDataPopManage::~CDataPopManage()
{
	for(int i = 0; i < PSS_MAX_DATAPOPER; i++)
	{
		m_bIdle[i] = HANDLE_IDLE;
	}

	for(int i = 0; i < PSS_MAX_DATAPOPER; i++)
	{
		if(NULL !=  m_data_reader[i])
		{
			DEL_POINTER(m_data_reader[i]);
			m_data_reader[i] = NULL;
		}
	}

	m_userMutex.Close();
}

CDataPopManage* CDataPopManage::GetMgrItem()
{
	if (NULL == theReaderManage)
	{
		theReaderManage = new CDataPopManage();

		if ( NULL == theReaderManage )
		{
			return NULL;
		}

		theReaderManage->InitParam();
	}
	return theReaderManage;
}

void CDataPopManage::DelMgrItem()
{
	DEL_POINTER(theReaderManage);
}

void CDataPopManage::InitParam()
{
	m_userMutex.Create();
	
	for(int i = 0; i < PSS_MAX_DATAPOPER; i++)
	{
		m_bIdle[i] = HANDLE_IDLE;
	}

	for(int i = 0; i < PSS_MAX_DATAPOPER; i++)
	{
		m_data_reader[i] = NULL;
	}
}

int CDataPopManage::DataPopCreate(int &nIdx, int nDevIdx, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime,
		ANTS_AVI_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext, BOOL bNeedSync)
{
	int nHandle = -1;

	m_userMutex.Wait();
	
	for(int i = 0; i < PSS_MAX_DATAPOPER; i++)
	{
		if(HANDLE_IDLE == m_bIdle[i])
		{
			m_bIdle[i] = HANDLE_BUSY;
			
			nHandle = i;
			break;
		}
	}

	m_userMutex.Release();
		
	if(-1 == nHandle)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_no_handle;
	}

	CDataPop *reader = new CDataPop();
	if(NULL == reader)
	{
		return avis_ret_failed;
	}
	nIdx = nHandle;
	m_data_reader[nIdx] = reader;
	
	int nRet = m_data_reader[nIdx]->DataPopCreate(nIdx, nRefChan, tStartSeekTime, tEndSeekTime, cbReplayCallback, bDynPop, pContext, bNeedSync);

	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	
	return avis_ret_ok;
}

int CDataPopManage::DataPopChanAdd(int nIdx, int iChannel)
{

	return avis_ret_ok;
}

int CDataPopManage::DataPopChanDel(int nIdx, int iChannel)
{

	return avis_ret_ok;
}

int CDataPopManage::DataPopRecTypeAdd(int nIdx, unsigned int iRecType)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopRecTypeAdd(iRecType);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopRecTypeDel(int nIdx, unsigned int iRecType)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopRecTypeDel(iRecType);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopTimeSeek(int nIdx, time_t SeekTime)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	
	int nRet = m_data_reader[nIdx]->DataPopTimeSeek(SeekTime);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopSetDirect(int nIdx, BOOL bSequential)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopSetDirect(bSequential);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopSetKeyFrame(int nIdx, BOOL bKeyFrame, int iSpeed)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopSetKeyFrame(bKeyFrame, iSpeed);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopGetDataSize(int nIdx, unsigned long long *pSize)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopGetDataSize(pSize);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopStart(int nIdx)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopStart();
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

int CDataPopManage::DataPopStop(int nIdx)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	int nRet = m_data_reader[nIdx]->DataPopStop();
	if(avis_ret_ok != nRet)
	{
		return nRet;
	}
	
	return avis_ret_ok;
}

int CDataPopManage::DataPopRelease(int nIdx)
{
	if(NULL == m_data_reader[nIdx])
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}

	int nRet = m_data_reader[nIdx]->DataPopRelease();
	if(avis_ret_ok != nRet)
	{
		return nRet;
	}

	DEL_POINTER(m_data_reader[nIdx]);

	m_bIdle[nIdx] = HANDLE_IDLE;
		
	return avis_ret_ok;
}

