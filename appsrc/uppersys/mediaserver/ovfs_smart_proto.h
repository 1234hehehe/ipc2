/**
 *   \file ovfs_smart_proto.h
 *   \brief OVFS HTTP 人脸信息应用协议
 *   \date 2019-07-24
 *   \author eric
 *
 *  Detailed description
 *
 */
#ifndef OVFS_SMART_PROTO_H_
#define OVFS_SMART_PROTO_H_

#include <libcommon_api.h>
#include <cjson.h>

#if (defined WITH_CURL)
int SmartProto_Init(MQ_HANDLE_H mqHandle);

int SmartProto_Start();

int SmartProto_Stop();

int SmartProto_Restart();

int SmartProto_CheckServer(char *uri, char **resonpseStr);
#endif

#endif
