/*
 * ovfs_network_mgr.h
 *
 *  Created on: 2017??7??20??
 *      Author: eric
 */

#ifndef OVFS_ONVIF_MGR_H_
#define OVFS_ONVIF_MGR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "ovfs_onvif.h"


int OnvifMgr_Init(MQ_HANDLE_H mqHandle);
int OnvifMgr_Uninit();
int OnvifMgr_Start();
int OnvifMgr_Stop();
int OnvifMgr_Restart();
int OnvifMgr_GetCfg();
int OnvifMgr_GetNetInfo(char *ifname, char *hwaddr, char *ipv4,
                        char *netmask);
int OnvifMgr_EventNotify();
int OnvifMgr_EventCheck();
int OnvifMgr_FixedIp();
int OnvifEvent_SendEvent(int isHappen);
int UdpMultiServerCallBack(char *devName, char *remoteIp, char *buf,int len, char **outBuf,int *outSize);
#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* OVFS_ONVIF_MGR_H_ */
