#ifndef _RECINDEX_MANAGE_H
#define _RECINDEX_MANAGE_H

#include "ants_avi_common.h"
#include "mutex.h"

//录像索引链表
typedef struct _antsavi_nodeInfo  //112字节
{
	S64 startAddr;
	int file_size;							// 文件的大小
	int file_type;							// 文件类型
	time_t begin_sec;								
	time_t end_sec;
	int use_size;  //文件占用的大小
	int iIndexNum;
	int iFrameOffset[IFRAMEOFFSETNUM];
}antsavi_nodeInfo;


typedef struct _antsavi_recindex_node
{
	antsavi_nodeInfo *element;
	struct _antsavi_recindex_node *pre;
	struct _antsavi_recindex_node *next;
}antsavi_recindex_node_t;


typedef struct _antsavi_recindex_list
{
	int iNodeCount;
	antsavi_recindex_node_t *head;
	antsavi_recindex_node_t *tail;
}antsavi_recindex_list_t;

//日期索引
typedef struct //20字节
{
	S64 startAddr;
	S64 file_size;	// 文件的大小
	time_t tDate;
}antsavi_recDateInfo;

typedef struct _antsavi_recdate_node
{
	antsavi_recDateInfo *element;
	struct _antsavi_recdate_node *pre;
	struct _antsavi_recdate_node *next;
}antsavi_recdate_node_t;


typedef struct _antsavi_recdate_list
{
	int iNodeCount;
	antsavi_recdate_node_t *head;
	antsavi_recdate_node_t *tail;
}antsavi_recdate_list_t;


class CRecIndexManage
{
public:

	static CRecIndexManage* GetMgrItem();
	static void	DelMgrItem();
	
	CRecIndexManage();
	~CRecIndexManage();

	int AddOneRecIndexNodeToDateList(RecIndexInfo *pIndexInfo, BOOL bWriteFile, S64 recIndexAddr=-1);
	int UpdateOneRecIndexNode(RecIndexInfo *pIndexInfo);
	int DeleteOneRecIndexNode();

	int SyncRecIndexNode();

	//回放时查找I帧数量
	int GetIFrameCountInFile(time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pCount);

	//回放时查找非I帧链表
	int FindInfFileForList(time_t tBeginTime, time_t tEndTime, int iRecType, avis_recfileindexlist **pRecSegHead, int *pRecSegCount, BOOL bIFrame=FALSE);
	int ReleaseList(avis_recfileindexlist *pRecSegHead);

	//回放时查找I帧链表
	int FindIFrameInFileForList(time_t tBeginTime, time_t tEndTime, int iRecType, avis_iFrameindexlist **pRecSegHead, int *pRecSegCount);
	int ReleaseIFrameList(avis_iFrameindexlist *pRecSegHead);

	int GetDataSize(time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pSize);

	int HasRecSegList(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType);
	int GetRecSegList(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG **pRecSegHead, int *pRecSegCount);
	int ReleaseRecSegList(PSS_REGSEG *pRecSegHead);

	int GetRecSegListEx(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG_EX **pRecSegHead, int *pRecSegCount);
	int ReleaseRecSegListEx(PSS_REGSEG_EX *pRecSegHead);

	int GetMonthHasRecDays(time_t begin_time, int numOfDays, DWORD &HasMask);

	int RecoverFromFile();

	int FormatDisk();
	
private:
	int PrintRecDateList();

	int UpdateLastRecIndexToFile(RecIndexInfo *pIndexInfo); //更新最后一个索引
	int InsertRecIndexToFile(RecIndexInfo *pIndexInfo);  //在最后插入一个索引

	int AddOneRecDateIndexNode(antsavi_recDateInfo *pIndexInfo);
	int UpdateOneRecDateIndexNode(antsavi_recDateInfo *pIndexInfo);

	static CRecIndexManage*	theRecIndexList;

	antsavi_recdate_list_t *m_lpDateList;	
//	antsavi_recindex_list_t *m_lpMediaList;

	antsavi_recDateInfo m_recDateIndexInfo;//记录当前正在写的日期索引
	
	avis_fileHanedle *m_recindex_fd; //打开录像索引

	avis_fileHanedle *m_delRecIndex_fd; //删除录像索引

	CMutex m_ListMutex; //日期链表锁

	CMutex m_indexInfoMutex; //日期索引和录像索引在删除和添加的时候可能同时被访问，可能出错
};

#endif

