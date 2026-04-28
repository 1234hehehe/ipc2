#ifndef _DISK_MANAGE_API_H_
#define _DISK_MANAGE_API_H_

#include <sys/time.h>
#include <libcommon_api.h>
#include "record_common.h"

using namespace cv_soft;

typedef struct
{
	S8		nodepath[128];
	S8		mountpath[128];
	S8		describe[128];
	U32		totalspace;
	U32		freespace;
}strPartitionAttr;

typedef void(*fAlarmUploadCallback)(S32 AlarmType, S32 AlarmStatus);

int cv_diskm_init(int dwRecycle, int dwPreserveMode, int dwPreserveTime);
int cv_diskm_setcfg(int dwRecycle, int dwPreserveMode, int dwPreserveTime);
void cv_diskm_setalarmcallback(fAlarmUploadCallback	callback);
int cv_diskm_getdisknum();
int cv_diskm_getrecorddiskfull();
int cv_diskm_getdisknode(S32 diskno, S8 *path);
int cv_diskm_getdiskPartitionNum(S32 diskno, S32 *num);
int cv_diskm_getdiskPartition(S32 diskno, S32 partno, strPartitionAttr *attr);
int cv_diskm_format(S32 diskno,S32 partno);
int cv_diskm_serchrecordbyday(time_t tBeginTime, U32 numOfdays, U32 *pResult);
int cv_diskm_lockrecord(int iChannel, time_t tBeginTime, time_t tEndTime, int bLock);

#endif	//#ifndef _COMM_SYS_H_


