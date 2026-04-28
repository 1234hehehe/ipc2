// ipcstorage.cpp : 定义 DLL 应用程序的导出函数。
//

#include "ipcstorage.h"
#include "ants_avi_common.h"
#include "globalObj.h"

AntsAviLibInfo g_antsAviLibInfo;

/*!
* \brief
* 存储系统的初始化.
* \param sDeviceSerial 当前未使用，可设置为空
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_Init(const char *sDeviceSerial)
{
	if (g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_ok;
	}

	AVI_DBG("ANTS_AviFile_Init\n");
	
	return g_antsAviLibInfo.Initialize();
}


/*!
* \brief
* 存储系统的反初始化.
* \param sDeviceSerial 当前未使用，可设置为空
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_UnInit(const char *sDeviceSerial)
{
	return g_antsAviLibInfo.DeInit();
}


/*!
* \brief 添加录像分区，如果相同路径的录像分区已经被添加了，则该接口将更新分区录像参数
* \param pInfo 欲添加的分区信息
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_AddRecPartition(PartitionInfo *pInfo)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}

	if(NULL == pInfo)
	{
		return avis_ret_failed;
	}

	int iRet = avis_ret_failed;

	iRet = g_antsAviLibInfo.antsStorageManage->AddRecPartition(pInfo);
	
	return iRet;
}



/*!
* \brief 删除录像分区
* \param pInfo 欲删除的分区信息，注意分区路径必须和添加时的保存一致
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DelRecPartition(char *pPath)
{

	return avis_ret_ok;
}


/*!
* \brief 获取分区录像已使用空间
* \param pInfo 分区路径，必须和添加时的保存一致
* \param pRecUsedSpace 输出参数，录像已使用的空间（字节）
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetRecUsedSpace(char *pPath, unsigned long long *pRecUsedSpace)
{
	return avis_ret_ok;
}

/*!
* \brief 获取录像分区的总大小和使用的大小
* \param pInfo 分区路径，必须和添加时的保存一致
* \param lpdwAllSpace 输出参数，录像空间的大小（M字节）
* \param lpdwFreeSpace 输出参数，录像已使用的空间（M字节）
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetDiskStatus(char *pPath, unsigned int *lpdwAllSpace, unsigned int *lpdwFreeSpace)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}

	if(NULL == pPath || NULL == lpdwAllSpace || NULL == lpdwFreeSpace)
	{
		return avis_ret_failed;
	}
	
	return g_antsAviLibInfo.antsStorageManage->GetDiskStatus(pPath, lpdwAllSpace,lpdwFreeSpace);
}


/*!
* \brief 格式化录像分区
* \param pInfo 分区路径，必须和添加时的保存一致
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_FormatDisk(char *pPath)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}

	if(NULL == pPath)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_failed;
	}
	
	return g_antsAviLibInfo.antsStorageManage->FormatDisk(pPath);
}


/*!
* \brief 将数据写入录像通道中
* \param iChannel 录像通道号
* \param pFrameHead 帧头数据指针
* \param nHeadLen 帧头数据长度
* \param pFrameData 帧数据指针，不包含帧头
* \param nDataLen 帧数据长度，不包含帧头
* \param tFrameTime 写入帧的绝对时间簇，精确度为秒
* \param tFrameTimeInUs 写入帧的绝对时间簇，秒内的精度(微秒)
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataInput(int iChannel, char *pBuffer, int nBufferLen, char *pFrameData, int nDataLen, DWORD dwCurTimeSec, DWORD dwCurTimeUsec)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		//OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
/*	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL)
	{
		AVI_ERR("iChannel=%d\n",iChannel);
		return avis_ret_invalid_para;
	}
*/
	if(NULL == pBuffer || nBufferLen <= FRAME_HEAD_LEN)
	{
		//AVI_ERR("pBuffer=%p len=%d\n",pBuffer, nBufferLen);
		return avis_ret_invalid_para;
	}
	
	int nRet = g_antsAviLibInfo.antsWriterManage->SetRecFrame(iChannel, pBuffer, nBufferLen, dwCurTimeSec, dwCurTimeUsec);
	if(avis_ret_ok != nRet)
	{
		//OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}


/*!
* \brief 以指定的录像类型开始录像，如果该通道的录像已经开启了，则调用该接口表示设置新的录像类型
* \param pRecDev 输出参数，返回录像设备句柄，相同的pDeviceId值会返回相同的设备句柄值
* \param iChannel 录像通道号
* \param iRecType 录像类型
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_RecStart(int iChannel, int iRecType)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	
	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL || iRecType <= ANTS_RECORDTYPE_BEGIN || iRecType >= ANTS_RECORDTYPE_END)
	{
		AVI_ERR("iChannel=%d  iRecType=%d\n",iChannel, iRecType);
		return avis_ret_invalid_para;
	}
	
	return g_antsAviLibInfo.antsWriterManage->StartRec(trans_to_rectype_mask(iRecType));
}

/*!
* \brief 停止指定录像通道的录像
* \param iChannel 录像通道号
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_RecStop(int iChannel)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	
	return g_antsAviLibInfo.antsWriterManage->StopRec();
}

/*!
* \brief 获取指定时间段通道的录像的I帧数量
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间
* \param iRecType 录像类型
* \param pCount 保存返回获取录像的I帧数量
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetIFrameCount(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pCount)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		//OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}

	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL || tBeginTime >= tEndTime)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(STRIDE_EXCEED_1_DAY(tEndTime, tBeginTime))
	{
		// 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int iTmpType = 0;
	if(iRecType == 0)
		iTmpType = AVIS_RECORDTYPE_ALL;
	else if(iRecType == 1)
		iTmpType = AVIS_RECORDTYPE_TIMER;
	else
		iTmpType = AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | \
			AVIS_RECORDTYPE_MANUAL | AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE;
	int nRet = g_antsAviLibInfo.antsRecIndexManage->GetIFrameCountInFile(tBeginTime, tEndTime, iTmpType, pCount);
	if(avis_ret_ok != nRet)
	{
		//OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

/*!
* \brief 获取指定时间段通道的录像大小
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间
* \param iRecType 录像类型
* \param pSize 保存返回获取录像数据大小
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetDataSize(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, unsigned long long *pSize)
{
	if (!g_antsAviLibInfo.IsInited())
	{
		//OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}

	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL || tBeginTime >= tEndTime)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(STRIDE_EXCEED_1_DAY(tEndTime, tBeginTime))
	{
		// 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int iTmpType = 0;
	if(iRecType == 0)
		iTmpType = AVIS_RECORDTYPE_ALL;
	else if(iRecType == 1)
		iTmpType = AVIS_RECORDTYPE_TIMER;
	else
		iTmpType = AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | \
			AVIS_RECORDTYPE_MANUAL | AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE;
	int nRet = g_antsAviLibInfo.antsRecIndexManage->GetDataSize(tBeginTime, tEndTime, iTmpType, pSize);
	if(avis_ret_ok != nRet)
	{
		//OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

/*!
* \brief 获得指定通道在指定时间段内指定录像类型的录像片断
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间
* \param iRecType 录像类型
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \param pRecSegCount 保存返回获取片断的个数
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetRecSegList(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG **pRecSegHead, int *pRecSegCount)
{
	//AVI_DBG("enter ANTS_AviFile_GetRecSegList, channel = %d, begintime = %ld, endtime=%ld, type=%d\n", iChannel, (long)tBeginTime, (long)tEndTime, iRecType);
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL || tBeginTime >= tEndTime)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(STRIDE_EXCEED_1_DAY(tEndTime, tBeginTime))
	{
		// 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(NULL == pRecSegHead || NULL == pRecSegCount)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int iTmpType = 0;
	if(iRecType == 0)
		iTmpType = AVIS_RECORDTYPE_ALL;
	else if(iRecType == 1)
		iTmpType = AVIS_RECORDTYPE_TIMER;
	else
		iTmpType = AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | \
			AVIS_RECORDTYPE_MANUAL | AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE;
	int nRet = g_antsAviLibInfo.antsRecIndexManage->GetRecSegList(iChannel, tBeginTime, tEndTime, iTmpType, pRecSegHead, pRecSegCount);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}

// 	avis_time_info begin_info, end_info;
// 	CGlobalObject::Instance().gmtime_utc(tBeginTime, &begin_info);
// 	CGlobalObject::Instance().gmtime_utc(tEndTime, &end_info);
// 	AVI_DBG("pRecSegCount = %d, begin_time = %04d-%02d-%02d %02d:%02d:%02d, end_time = %04d-%02d-%02d %02d:%02d:%02d\n", *pRecSegCount,
// 		begin_info.wYear, begin_info.wMonth, begin_info.wDay, begin_info.wHour, begin_info.wMinute, begin_info.wSecond,
// 		end_info.wYear, end_info.wMonth, end_info.wDay, end_info.wHour, end_info.wMinute, end_info.wSecond);

	//int seg_idx = 0;
	PSS_REGSEG *seg_first = *pRecSegHead;
	while(NULL != seg_first)
	{
 		//char acBeginTime[32] = {0}, acEndTime[32] = {0};
 		//avis_time_info begin_info, end_info;
 		//CGlobalObject::Instance().gmtime_utc(seg_first->tBeginTime, &begin_info);
 		//CGlobalObject::Instance().gmtime_utc(seg_first->tEndTime, &end_info);
 		//sprintf_s(acBeginTime, sizeof(acBeginTime), "%02d:%02d:%02d", begin_info.wHour, begin_info.wMinute, begin_info.wSecond);
 		//sprintf_s(acEndTime, sizeof(acEndTime), "%02d:%02d:%02d", end_info.wHour, end_info.wMinute, end_info.wSecond);
 		//printf("[ch%02d]: segment %02d: type = %d, begint_time = %s, end_time = %s \n", iChannel+1, ++seg_idx, seg_first->iRecType, acBeginTime, acEndTime);

		// 转换成外部使用的录像类型
		seg_first->iRecType = trans_from_rectype_mask(seg_first->iRecType);
		seg_first = seg_first->ptNext;
	}
	return avis_ret_ok;
}


/*!
* \brief 释放录像片断信息列表所占内存
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_ReleaseRecSegList(PSS_REGSEG *pRecSegHead)
{
	//AVI_DBG("enter ANTS_AviFile_ReleaseRecSegList \n");
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	if(NULL == pRecSegHead)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int nRet = g_antsAviLibInfo.antsRecIndexManage->ReleaseRecSegList(pRecSegHead);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}


/*!
* \brief 获得指定通道在指定时间段内指定录像类型的录像片断
* \param iChannel 录像通道号
* \param tBeginTime 起始时间
* \param tEndTime 结束时间
* \param iRecType 录像类型
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \param pRecSegCount 保存返回获取片断的个数
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetRecSegListEx(int iChannel, time_t tBeginTime, time_t tEndTime, int iRecType, PSS_REGSEG_EX **pRecSegHead, int *pRecSegCount)
{
	//AVI_DBG("enter ANTS_AviFile_GetRecSegListEx, channel = %d, begintime = %ld, endtime=%ld, type=%d\n", iChannel, (long)tBeginTime, (long)tEndTime, iRecType);
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL || tBeginTime >= tEndTime)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(STRIDE_EXCEED_1_DAY(tEndTime, tBeginTime))
	{
		// 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(NULL == pRecSegHead || NULL == pRecSegCount)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int iTmpType = 0;
	if(iRecType == 0)
		iTmpType = AVIS_RECORDTYPE_ALL;
	else if(iRecType == 1)
		iTmpType = AVIS_RECORDTYPE_TIMER;
	else
		iTmpType = AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | \
			AVIS_RECORDTYPE_MANUAL | AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE;
	int nRet = g_antsAviLibInfo.antsRecIndexManage->GetRecSegListEx(iChannel, tBeginTime, tEndTime, iTmpType, pRecSegHead, pRecSegCount);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}


	PSS_REGSEG_EX *seg_first = *pRecSegHead;
	while(NULL != seg_first)
	{
		// 转换成外部使用的录像类型
		seg_first->iRecType = trans_from_rectype_mask(seg_first->iRecType);
		seg_first = seg_first->ptNext;
	}
	return avis_ret_ok;
}

/*!
* \brief 释放录像片断信息列表所占内存
* \param pRecSegHead 存放录像片段的缓冲区头指针
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_ReleaseRecSegListEx(PSS_REGSEG_EX *pRecSegHead)
{
	//AVI_DBG("enter ANTS_AviFile_ReleaseRecSegListEx \n");
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	if(NULL == pRecSegHead)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int nRet = g_antsAviLibInfo.antsRecIndexManage->ReleaseRecSegListEx(pRecSegHead);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

/*!
* \brief 创建数据弹出器
* \param nRefChan 参考通道(可以不设置，值为-1表示不设置)
* \param tStartSeekTime 初始弹出数据的时间
* \param tEndSeekTime 弹出数据的结束时间。当此时间为0时，数据进行动态弹出。如果结束时间是一个未来时间，则到达当前时间时，通过回调接口告之该事件。
* \param cbReplayCallback 设定数据弹出的回调函数接口，弹出的数据按弹出器句柄、通道、帧类型、时间簇、事件类型（跳到新的时间片/查询结束/当前数据查询完毕等）等参数形式送回放线程
* \param bDynPop 是否动态定位弹出，即播放完毕后，是否主动重新定位继续弹出。当用于回放时，设置为TRUE，用于备份时，设置为FALSE
* \param pContext 上下文，上层软件可以传入此参数，在弹出回调中会给出该参数
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopCreate(HANDLE *pDataPopper, int nRefChan, time_t tStartSeekTime, time_t tEndSeekTime, ANTS_AVI_DATAPOP_CALLBACK cbReplayCallback, BOOL bDynPop, void *pContext)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopCreate, channel = %d, begintime = %ld, endtime=%ld\n", nRefChan, (long)tStartSeekTime, (long)tEndSeekTime);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	if(NULL == pDataPopper)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if((-1 != nRefChan && (nRefChan >= PSS_MAX_CHANNEL || nRefChan < 0)) || (tStartSeekTime > tEndSeekTime && 0 != tEndSeekTime) || NULL == cbReplayCallback)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	if(0 == tEndSeekTime)
	{
		bDynPop = TRUE;
	}
	else
	{
		if(STRIDE_EXCEED_1_DAY(tEndSeekTime, tStartSeekTime))
		{
			// 起始时间和结束时间之间允许跨天, 但二者之差不得超过24小时
			OUTPUT_FUNC_LINE;
			return avis_ret_invalid_para;
		}
	}
	int nDevIdx = 0;
	BOOL need_sync = true;
	int nIdx = -1;
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopCreate(nIdx, nDevIdx, nRefChan, tStartSeekTime, tEndSeekTime, cbReplayCallback, bDynPop, pContext, need_sync);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	*pDataPopper = (HANDLE)(nIdx + 1);
	return avis_ret_ok;
}



/*!
* \brief 添加弹出数据的通道
* \param hDataPopper 数据弹出器的句柄
* \param iChannel 通道号
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopChanAdd(HANDLE hDataPopper, int iChannel)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopChanAdd, channel = %d\n", iChannel);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopChanAdd(nIndex, iChannel);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 移除弹出数据的通道
* \param hDataPopper 数据弹出器的句柄
* \param iChannel 通道号
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopChanDel(HANDLE hDataPopper, int iChannel)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopChanDel, channel = %d\n", iChannel);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	if(iChannel < 0 || iChannel >= PSS_MAX_CHANNEL)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopChanDel(nIndex, iChannel);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 添加弹出数据的录像类型
* \param hDataPopper 数据弹出器的句柄
* \param iRecType 录像类型
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopRecTypeAdd(HANDLE hDataPopper, unsigned int iRecType)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopRecTypeAdd, type = %d\n", iRecType);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int iTmpType = 0;
	if(iRecType == 0)
		iTmpType = AVIS_RECORDTYPE_ALL;
	else if(iRecType == 1)
		iTmpType = AVIS_RECORDTYPE_TIMER;
	else
		iTmpType = AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | \
			AVIS_RECORDTYPE_MANUAL | AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE;
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopRecTypeAdd(nIndex, iTmpType);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 移除弹出数据的录像类型
* \param hDataPopper 数据弹出器的句柄
* \param iRecType 录像类型
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopRecTypeDel(HANDLE hDataPopper, unsigned int iRecType)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopRecTypeDel, channel = %d\n", iChannel);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int iTmpType = 0;
	if(iRecType == 0)
		iTmpType = AVIS_RECORDTYPE_ALL;
	else if(iRecType == 1)
		iTmpType = AVIS_RECORDTYPE_TIMER;
	else
		iTmpType = AVIS_RECORDTYPE_MOTION | AVIS_RECORDTYPE_ALARM | AVIS_RECORDTYPE_MOTIONORALARM | AVIS_RECORDTYPE_MOTIONANDALARM | AVIS_RECORDTYPE_COMMAND | \
			AVIS_RECORDTYPE_MANUAL | AVIS_RECORDTYPE_IVSDETECT | AVIS_RECORDTYPE_FACEDETECT | AVIS_RECORDTYPE_FIREDETECT | AVIS_RECORDTYPE_VIDEODIAGNOSE;
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopRecTypeDel(nIndex, iTmpType);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 数据弹出器定位(设定了参考通道，按参考通道定位，未设参考通道，定位为时序上第一个满足条件的通道)
* \param hDataPopper 数据弹出器的句柄
* \param SeekTime 设置的弹出数据的时间
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopTimeSeek(HANDLE hDataPopper, time_t SeekTime)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopTimeSeek, channel = %d\n", iChannel);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}

	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopTimeSeek(nIndex, SeekTime);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 开始数据弹出
* \param hDataPopper 数据弹出器的句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopStart(HANDLE hDataPopper)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopTimeSeek, channel = %d\n", iChannel);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		AVI_ERR("hDataPopper %d  nIndex %d\n",(int)hDataPopper, nIndex);
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopStart(nIndex);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 停止数据弹出
* \param hDataPopper 数据弹出器的句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopStop(HANDLE hDataPopper)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopStop \n");
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		AVI_ERR("hDataPopper %d  nIndex %d\n",(int)hDataPopper, nIndex);
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopStop(nIndex);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}



/*!
* \brief 释放数据弹出器
* \param hDataPopper 数据弹出器的句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopRelease(HANDLE hDataPopper)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopRelease \n");
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		AVI_ERR("hDataPopper %d  nIndex %d\n",(int)hDataPopper, nIndex);
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopRelease(nIndex);
	if(avis_ret_ok != nRet)
	{
		return nRet;
	}
	return avis_ret_ok;
}


/*!
* \brief 数据弹出时是否只弹出关键帧
* \param hDataPopper 数据弹出器的句柄
* \param bKeyFrame 是否只弹出关键帧，如果为TRUE则只弹出关键帧，如果为FALSE则弹出所有帧。
* \param iSpeed 参数为-4 ~ 4 -1表示1/2倍速 1表示2倍速 0表示正常速度
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopSetKeyFrame(HANDLE hDataPopper, BOOL bKeyFrame, int iSpeed)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopSetKeyFrame, bkeyframe = %d, iSpeed = %ld \n", (int)bKeyFrame, iSpeed);
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	if(iSpeed < -4 || iSpeed > 4)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopSetKeyFrame(nIndex, bKeyFrame, iSpeed);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}


/*!
* \brief 获取弹出器备份数据的总容量(单位：字节)
* \param hDataPopper 数据弹出器的句柄
* \param pSize 总容量
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopGetDataSize(HANDLE hDataPopper, unsigned long long *pSize)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopGetDataSize\n");
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	if(NULL == pSize)
	{
		return avis_ret_failed;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopGetDataSize(nIndex, pSize);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}	
	//AVI_DBG("leave ANTS_AviFile_DataPopGetDataSize, size = %d MB\n", *pSize > MBYTE_UNIT ? int(*pSize/ MBYTE_UNIT) : int(*pSize));
	return avis_ret_ok;
}



/*!
* \brief 设置数据弹出方向
* \param hDataPopper 数据弹出器的句柄
* \param bKeyFrame 数据弹出方向，如果为TRUE则向后弹出，如果为FALSE则向前弹出（暂未实现）
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DataPopSetDirect(HANDLE hDataPopper, BOOL bSequential)
{
	//AVI_DBG("enter ANTS_AviFile_DataPopSetDirect, bSequential = %d\n", (int)bSequential);
	if (!g_antsAviLibInfo.IsInited())
	{
		return avis_ret_sdk_no_init;
	}
	int nIndex = (int)hDataPopper - 1;
	if(INVALID_HANDLE_VALUE == hDataPopper || nIndex < 0 || nIndex >= PSS_MAX_DATAPOPER)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_handle;
	}
	int nRet = g_antsAviLibInfo.antsReaderManage->DataPopSetDirect(nIndex, bSequential);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	return avis_ret_ok;
}

/*!
* \brief
* 设置磁盘分组的满回调
* \param cbDiskGroupFullCallback 回调函数句柄
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SetDiskGroupFullCallback(ANTS_DATAINPUT_DISKFULL_CALLBACK cbDiskGroupFullCallback)
{
	//AVI_DBG("enter ANTS_AviFile_SetDiskGroupFullCallback\n");
	
	return avis_ret_ok;
}

/*!
* \brief 获取是否有指定天数(一般用于一个月)的录像数据
* \param tBeginTime 指定时间段的开始时间
* \param numOfdays 从起始天开始的天数(24小时)
* \param pResult 返回结果，采用位与方式bit0-bit30分别对应31天
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，缺省时表示不需要区分设备
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_JudgeIFHaveDataByDays(time_t tBeginTime, unsigned int numOfdays, unsigned int *pResult)
{
	//AVI_DBG("ANTS_AviFile_JudgeIFHaveDataByDays enter\n");
	if (!g_antsAviLibInfo.IsInited())
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_sdk_no_init;
	}
	if(numOfdays < 0 || numOfdays > 31 || NULL == pResult)
	{
		OUTPUT_FUNC_LINE;
		return avis_ret_invalid_para;
	}
	
	DWORD dwRecMask = 0;
	int nRet = g_antsAviLibInfo.antsRecIndexManage->GetMonthHasRecDays(tBeginTime, numOfdays, dwRecMask);
	if(avis_ret_ok != nRet)
	{
		OUTPUT_FUNC_LINE;
		return nRet;
	}
	*pResult = dwRecMask;

	//for(int i = 0; i < 32; i++)
	//{
	//	if(dwRecMask & (1 << i))
	//	{
	//		printf("%d, ", i+1);
	//	}
	//}
	//printf("\n ANTS_AviFile_JudgeIFHaveDataByDays \n");
	return avis_ret_ok;
}


/*!
* \brief 将帧数据保存成avi文件
* \param pSaveHandle 输出参数，返回保存句柄
* \param pFullName 完整的文件路径
当pFullName是以".avi"结尾的合法文件名时，接口只负责存储录像数据到指定名称的avi文件中，何时切换录像文件由上层决定；
否则当pFullName是合法的路径名时，接口会在该路径下存储avi录像文件并自动切换文件
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveStart(HANDLE *pSaveHandle, char *pFullName)
{
	
	return avis_ret_ok;
}


/*!
* \brief 将帧数据保存成指定名称的avi文件
* \param pSaveHandle 输出参数，返回保存句柄
* \param pFullName 完整的文件路，无论输入的文件名是否以".avi"结尾，一律将avi格式的数据存储到指定名称的文件中，何时切换录像文件由上层决定
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveStart_ByName(HANDLE *pSaveHandle, char *pFullName)
{
	
	return avis_ret_ok;
}


/*!
* \brief 将数据写入avi文件中
* \param hSaveHandle 保存句柄，由ANTS_AviFile_SaveStart接口返回
* \param pFrameHead 帧头数据指针
* \param nHeadLen 帧头数据长度
* \param pFrameData 帧数据指针，不包含帧头
* \param nDataLen 帧数据长度，不包含帧头
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveInput(HANDLE hSaveHandle, char *pFrameHead, int nHeadLen, char *pFrameData, int nDataLen)
{
	
	return avis_ret_ok;
}

/*!
* \brief 停止指定录像通道的录像
* \param hSaveHandle 保存句柄，由ANTS_AviFile_SaveStart接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_SaveStop(HANDLE hSaveHandle)
{
	
	return avis_ret_ok;
}


/*!
* \brief 开始查询指定通道在指定日期的录像文件
* \param pQuery 查询句柄，输出参数
* \param iChannel 录像通道号，从0开始，-1表示查询所有通道的录像文件
* \param tDate 录像查询日期，-1表示查询所有日期的录像文件
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回，
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_QueryRecFilesStart(OUT HANDLE *pQuery, IN int iChannel, IN time_t tDate)
{
	
	return avis_ret_ok;
}


/*!
* \brief 获取下一条录像文件信息
* \param hQuery 查询句柄，通过ANTS_AviFile_QueryRecFilesStart接口返回
* \param pInfo 查询到的录像文件信息
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_GetNextRecFile(IN HANDLE hQuery, OUT RecFileInfo *pInfo)
{
	
	return avis_ret_ok;
}


/*!
* \brief 停止查询录像文件
* \param hQuery 查询句柄，通过ANTS_AviFile_QueryRecFilesStart接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_QueryRecFilesStop(IN HANDLE hQuery)
{
	
	return avis_ret_ok;
}

/*!
* \brief 删除指定名称的录像文件
* \param pFullPath 录像文件的完整路径
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DelAviFileByName(IN char *pFullPath)
{
	
	return avis_ret_ok;
}

/*!
* \brief 删除指定条件的所有录像文件
* \param iChannel 录像通道号，从0开始，-1表示删除所有通道的录像文件
* \param tDate 录像查询日期，-1表示删除所有日期的录像文件
* \param hRecDev 设备句柄，由ANTS_AviFile_CreateDevHandle接口返回
* \return 成功返回0，失败的错误码请参考ants_avis_ret_code
*/
AAS_API int AAS_CALL ANTS_AviFile_DelAviFiles(IN int iChannel, IN time_t tDate)
{

	return avis_ret_ok;
}
