#ifndef _MUTEX_H
#define _MUTEX_H

class CMutex  
{
public:
	CMutex()
	{
		m_bInit = false;
	}

	~CMutex()
	{
		m_bInit = false;
	}

	int Create()
	{
		if (m_bInit)
		{
			return avis_ret_has_create;
		}

		pthread_mutex_init (&m_hMutex, 0);
		m_bInit = true;
		return avis_ret_ok;
	}

	int Close()
	{
		if (!m_bInit)
		{
			return avis_ret_not_create;
		}

		pthread_mutex_destroy(&m_hMutex);
		m_bInit = false;
		return avis_ret_ok;
	}

	int Wait()
	{
		if (!m_bInit)
		{
			return avis_ret_not_create;
		}

		pthread_mutex_lock (&m_hMutex);
		return avis_ret_ok;
	}

	int TryWait()
	{
		if (!m_bInit)
		{
			return avis_ret_not_create;
		}

		pthread_mutex_trylock(&m_hMutex);
		return avis_ret_ok;
	}

	int Release()
	{
		if (!m_bInit)
		{
			return avis_ret_not_create;
		}
		pthread_mutex_unlock (&m_hMutex);
		return avis_ret_ok;
	}

private:
	BOOL m_bInit;	
	pthread_mutex_t m_hMutex;
};

#endif


