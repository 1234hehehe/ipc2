#ifndef _RECORD_QUERY_H_
#define _RECORD_QUERY_H_

#include "libcommon_api.h"
#include "record_common.h"

using namespace cv_soft;

typedef struct
{
	//S8								sFileName[100];
	Common_Time_T					StartTime;
	Common_Time_T					StopTime;
	U32								FileSize;
	U32								FileType;
	U32								key;
	U32								blocked;
	void*							pPreSection;
	void*							pNextSection;
}CV_RECORD_SEGNODE,*LPCV_RECORD_SEGNODE;

class CQuery{
public:
						CQuery(CV_RECORD_QUESTPARA QueryInfo);
						~CQuery();

	S32				FindNextSeg(LPCV_RECORD_SEGDATA pSegInfo);
	S64				GetDataSize(CV_RECORD_QUESTPARA QueryInfo);
	S64 				GetIFrameCount(CV_RECORD_QUESTPARA QueryInfo);

private:
	S32				lchannel;
	U32				dwSegCnt;
	LPCV_RECORD_SEGNODE		pRecordSectionHead;
	LPCV_RECORD_SEGNODE		pRecordSectionTail;
};

#endif

