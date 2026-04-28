#ifndef OVFS_WS_MGR_H_
#define OVFS_WS_MGR_H_

#include <libcommon_api.h>
#include <cjson.h>

int WsMgr_Init(MQ_HANDLE_H mqHandle);
int WsMgr_Start();
int WsMgr_Stop();
int WsMgr_Restart();
int WsMgr_SendAlarm(void *data);

#endif

