/*
 * ovfs_com_utils.h
 *
 *  Created on: 2017年2月23日
 *      Author: eric
 */

#ifndef OVFS_COM_UTILS_H_
#define OVFS_COM_UTILS_H_

int Utils_Exception_RegistSigHandle(int sigNum);
void Utils_Sleep(unsigned long long msec);
long long int Utils_GetMs(void);
int Utils_FileMonitor(char *path);

#endif /* OVFS_COM_EXCEPTION_H_ */
