#ifndef OVFS_WEB_ALIST_H
#define OVFS_WEB_ALIST_H


#ifndef OVFS_MAXALLOCSIZE
#define OVFS_MAXALLOCSIZE  (0)
#endif

#ifndef WEB_CANARY
# define WEB_CANARY (0xC0DE)
#endif

typedef struct ALIST_S{
	void *memlist;
	pthread_mutex_t mutex;
}ALIST_T;

/* ALIST
 */
int common_alist_init(ALIST_T *list);
void* common_alist_calloc(ALIST_T *list, size_t n);
char* common_alist_strdup(ALIST_T *list, const char *s);
void common_alist_dealloc(ALIST_T *list, void *p);
int common_alist_uninit(ALIST_T *list);
void common_alist_echo(ALIST_T *list);


#endif /* common_alist.h */