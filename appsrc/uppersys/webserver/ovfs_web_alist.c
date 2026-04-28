#include "ovfs_web_func.h"

/*  ALIST
 */
int common_alist_init(ALIST_T *list)
{
	int iRet = 0;

	if (list)
	{
		list->memlist = NULL;
		pthread_mutex_init(&(list->mutex), NULL);
	}
	else
	{
		LOGE("Param Err!\n");
		iRet = -1;
	}

	return iRet;
}

void* common_alist_calloc(ALIST_T *list, size_t n)
{
	char *p;

	if (NULL == list)
		return NULL;
	if (OVFS_MAXALLOCSIZE > 0 && n > OVFS_MAXALLOCSIZE)
		return NULL;
	if (n + sizeof(short) < n)
  		return NULL;
	n += sizeof(short);
	if (n + ((-(long)n) & (sizeof(void*)-1)) + sizeof(void*) + sizeof(size_t) < n)
  		return NULL;
	n += (-(long)n) & (sizeof(void*)-1); /* align at 4-, 8- or 16-byte boundary by rounding up */	
	p = (char*)calloc(1, n + sizeof(void*) + sizeof(size_t));
	if (!p)
	{ 
  		return NULL;
	}
	/* set a canary word to detect memory overruns and data corruption */
	*(unsigned short*)(p + n - sizeof(unsigned short)) = (unsigned short)WEB_CANARY;
	/* keep chain of alloced cells for destruction */
	pthread_mutex_lock(&list->mutex);
	*(void**)(p + n) = list->memlist;
	*(size_t*)(p + n + sizeof(void*)) = n;
	list->memlist = p + n;
	pthread_mutex_unlock(&list->mutex);
	
	return p;
}

char* common_alist_strdup(ALIST_T *list, const char *s)
{
	char *t = NULL;

	if (s)
	{ 
		size_t n = strlen(s) + 1;
		if (n > 0)
		{ t = (char*)common_alist_calloc(list, n);
		  if (t)
		  { memcpy((void*)t, (const void*)s, n);
		    t[n - 1] = '\0';
		  }
		}
	}

  return t;
}

void common_alist_dealloc(ALIST_T *list, void *p)
{
	if (list)
	{
		pthread_mutex_lock(&list->mutex);
		if (p)
		{
			char **q;
			for (q = (char **)(void *)&list->memlist; *q; q = *(char***)q)
			{
				if (*(unsigned short*)(char*)(*q - sizeof(unsigned short)) != (unsigned short)WEB_CANARY)
				{
					LOGE("MOE");
					return;
				}
				if (p == (void*)(*q - *(size_t*)(*q + sizeof(void*))))
				{
					*q = **(char***)q;
					free(p);
					pthread_mutex_unlock(&list->mutex);
					return;
				}
			}
		}
		else
		{
			char *q;
			while (list->memlist)
			{
				q = (char*)list->memlist;
				if (*(unsigned short*)(char*)(q - sizeof(unsigned short)) != (unsigned short)WEB_CANARY)
				{
					LOGE("MOE");
					pthread_mutex_unlock(&list->mutex);
					return;
				}
				list->memlist = *(void**)q;
				q -= *(size_t*)(q + sizeof(void*));
				free(q);
			}
		}
		pthread_mutex_unlock(&list->mutex);
	}
	else
	{
		LOGE("Param Err!\n");
	}

	return;
}

int common_alist_uninit(ALIST_T *list)
{
	int iRet = 0;

	if (list)
	{
		common_alist_dealloc(list, NULL);
		pthread_mutex_destroy(&list->mutex);
	}
	else
	{
		LOGE("Param Err!\n");
		iRet = -1;
	}

	return iRet;
}

void common_alist_echo(ALIST_T *list)
{
	char *q;
	char *tmp;
	int index = 0;
	
	if (list)
	{
		pthread_mutex_lock(&list->mutex);
		tmp = (char *)list->memlist;
		while (tmp)
		{
			q = (char*)tmp;
			if (*(unsigned short*)(char*)(q - sizeof(unsigned short)) != (unsigned short)WEB_CANARY)
			{
				LOGE("MOE");
				pthread_mutex_unlock(&list->mutex);
				return;
			}
			tmp = (char *)(*(void**)q);
			q -= *(size_t*)(q + sizeof(void*));
			LOGE("[%d]%p\n", index, q);
			index ++;
		}
		pthread_mutex_unlock(&list->mutex);
	}

	return;
}