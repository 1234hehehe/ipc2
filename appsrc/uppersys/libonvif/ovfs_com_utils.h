/*
 * ovfs_com_utils.h
 *
 *  Created on: 
 *      Author: eric
 */

#ifndef OVFS_COM_UTILS_H_
#define OVFS_COM_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "ovfs_onvif.h"
typedef void (*UTILS_ASYNC_TASK_CALLBACK_F)(int, void *, int);

int Utils_Exception_RegistSigHandle(int sigNum);
void Utils_Sleep(unsigned long long msec);
long long int Utils_GetMs(void);
int Utils_FileMonitor(char *path);


int Utils_AsyncTaskInit(void **UtilsAsyncHandle,
                        UTILS_ASYNC_TASK_CALLBACK_F CallBack);
int Utils_AsyncTaskUninit(void **UtilsAsyncHandle);
int Utils_AsyncTaskAdd(void *UtilsAsyncHandle, int taskId, int delay,
                       void *data, int dataSize);


typedef void (* UTILS_UDS_CALLBACK_F)(void *handle, int conHandle, char *buf,
                                      int len, void *userData);
int Utils_UdsServerStart(char *addr, int timeOut, int maxBufLen,
                         UTILS_UDS_CALLBACK_F cb, void *userData, void **handle);
int Utils_UdsServerStop(void **handle);
int Utils_UdsServerSend(void *handle, int conHandle, char *buf, int len);

typedef int (* UTILS_UDP_MULTI_CALLBACK_F)(void *handle, int conHandle, char *devName,
        char *buf, int len, char *remoteIp, void *userData);
int Utils_UdpMultiServerStart(char *addr, int port, int timeOut, int maxBufLen,
                              UTILS_UDP_MULTI_CALLBACK_F cb, void *userData, void **handle);
int Utils_UdpMultiServerStop(void **handle);

int Utils_SendtoMulticast(char *s, int len, char *devName, char *devIp);

char *ovfs_onvif_BASE64Encode(const char *data, int data_len);
char *ovfs_onvif_BASE64Decode(const char *data, int data_len, int *dec_len);
unsigned int ovfs_onvif_Packbits(unsigned char *src, unsigned char *dst,
                                 unsigned int n);
unsigned int ovfs_onvif_UnPackbits(unsigned char *outp, unsigned char *inp,
                                   unsigned int outlen, unsigned int inlen);
int Utils_GetNetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask);
int Utils_SetNetDevInfo(char *ifname, char *hwaddr, char *ipv4, char *netmask);

int Utils_ArpingCheckIp(char *remoteIpaddr, char *localIpaddr, char *mac, char *dev,
                        unsigned long long int timeout);
int Utils_PickDecNumber(const char* str);

/*获取本地某个tcp端口的连接数量*/
int Utils_CheckTcpPortIsConnected(int port, int *connectedNum);

int Utils_GetNetMaskBit(char *netmask);
#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* OVFS_COM_EXCEPTION_H_ */