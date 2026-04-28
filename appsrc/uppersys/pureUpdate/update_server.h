#ifndef __UPDATE_SERVER_H__
#define __UPDATE_SERVER_H__

#define UPGRADE_STATE_IDLE   0  //升级未开始
#define UPGRADE_STATE_FINISH 1  //升级完成
#define UPGRADE_STATE_ING    2  //升级中
#define UPGRADE_STATE_FAIL   3  //升级失败

// UpdateServer_SetBoardType
// dwBoardType：16进制板型如0x0134;    0-不检测板型
void UpdateServer_SetBoardType(unsigned int dwBoardType);


//UpdateServer_Upgrade
// 返回句柄: <0 失败
// 会检测当前目录下boardtype.txt文件指示当前的板型,内容如: #0134#
//   
int	UpdateServer_Upgrade(int lUserID, char *pBuffer, int bFile);
//UpdateServer_GetUpgradeState
//返回状态: <0 失败
// lStatus : -1升级失败,0-升级完成,1-开始升级,2-升级中

int	UpdateServer_GetUpgradeState(int lUpgradeHandle);

//UpdateServer_GetUpgradeProgress
// 返回进度 : <0 失败
int	UpdateServer_GetUpgradeProgress(int lUpgradeHandle);

// UpdateServer_CloseUpgradeHandle
// 返回：TRUE，FALSE
int	UpdateServer_CloseUpgradeHandle(int lUpgradeHandle); 
#endif

