#ifndef _REPLAY_API_H_
#define _REPLAY_API_H_

#include "libcommon_api.h"
#include "record_common.h"
#include "replay_popmgr.h"
#include "replay_query.h"

namespace cv_soft {

CV_ERR cv_replay_querybymonthly(S32 year, S32 month, U32 *pResult);
CV_ERR cv_replay_querycreate(U32 *phandle, CV_RECORD_QUESTPARA param);
CV_ERR cv_replay_querynext(U32 handle, LPCV_RECORD_SEGDATA pSegData);
CV_ERR cv_replay_getdatasize(U32 handle,CV_RECORD_QUESTPARA param,S64 *pDataSize);
CV_ERR cv_replay_getiframecount(U32 handle,CV_RECORD_QUESTPARA param,S64 *pCount);
CV_ERR cv_replay_queryclose(U32 handle);
CV_ERR cv_replay_create(U32 *phandle, S32 lChannelNum, U32 *uChannel, time_t tStartTime, time_t tEndTime, fHistStreamCallBackV2 PopCallBack, S32 StreamFd, S32 StreamType, S32 lRecordType, void* lpUser);
CV_ERR cv_replay_start(U32 handle);
CV_ERR cv_replay_stop(U32 handle);
CV_ERR cv_replay_seek(U32 handle, time_t tSeekTime);
CV_ERR cv_replay_setdir(U32 handle, U32 dir);
CV_ERR cv_replay_getpopdatesize(U32 handle,S64 *pDataSize);
CV_ERR cv_replay_setpoptype(U32 handle, U32 onlykeyframe);
CV_ERR cv_replay_release(U32 handle);

}
#endif

