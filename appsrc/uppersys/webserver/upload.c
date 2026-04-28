//#include "upload.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include "AntsWebCommon.h"

extern int JsonOper_MergeObj(Common_cJSON_T* dst,Common_cJSON_T* src,int type);

int upload_file_save_as(int content_length, Webs *wp)
{
    int ret = 0;
    unsigned int contentLength = (unsigned int)content_length; /* uploaded file content length */

    if(contentLength <= 0)
    {
        ret = WEB_CODE_InvalidArg;
    }

    char *boundary = NULL;
    unsigned int boundaryLen = 0;
    if (0 == ret)
    {
        char *tempBound = NULL;
        char *tempStr = FCGX_GetParam("CONTENT_TYPE", wp->req->envp);
        if (tempStr == NULL || strncmp(tempStr, "multipart/form-data;", strlen("multipart/form-data;")) != 0)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else if ((tempBound = strstr(tempStr, "boundary=")) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else if ((boundary = strdup(tempBound + 9)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            boundaryLen = strlen(boundary);
            PRINT_DBG("boundaryLen=%d\n", boundaryLen);
        }
    }

    char *streamBuff = NULL;
    unsigned int streamBuffSize = 0;
    unsigned int streamBuffUsed = 0;
    if (0 == ret)
    {
        //! 注意:streamBuff的末尾4字节是额外申请的,总是为0,所以可以对streamBuff进行字符串处理而不会越界.
        streamBuffSize = 64*1024;
        if ((streamBuff = calloc(streamBuffSize + 4, 1)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }
    
    cJSON_Struct *param = NULL;
    if (0 == ret)
    {
        param = Common_cJSON_CreateObject();
        if (param == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

/**************待处理内容示例*******************************
-----------------------------7e1a82a90b88
Content-Disposition: form-data; name="param"

{"Username":"admin","PasswordDigest":"AARk3P/3MLvGZmZ9JQWqeRZeaqc=","Created":"2017-09-30T07:11:02.464Z","Nonce":"MmpkYTRuTTRCZDRyKzA2VE94ZnNRcyttMXNnPQ=="}
-----------------------------7e1a82a90b88
Content-Disposition: form-data; name="Username"

admin
-----------------------------7e1a82a90b88
Content-Disposition: form-data; name="selfile"; filename="Release.update"
Content-Type: application/x-zip-compressed

....
-----------------------------7e1a82a90b88--
***********************************************************/

    int fdUpload = -1;
    unsigned int readSize;
    unsigned int streamCounter;
    unsigned int fileCounter = 0;
	int file_count = 0;
    char *segmentName = NULL;
    char *segmentBuff = NULL;
    unsigned int segmentBuffLen = 0;
    int tempret = ret;
    for (streamCounter = 0 ; 0 == tempret && streamCounter < contentLength; streamCounter += readSize)
    {
        // 读取一部分数据到streamBuff中.
        if (0 == tempret)
        {
            readSize = MIN(streamBuffSize - streamBuffUsed, contentLength - streamCounter);
            //PRINT_DBG("Reading=%d,used=%d,size=%d,content=%d/%d\n", readSize, streamBuffUsed, streamBuffSize, streamCounter, contentLength);
            if(FCGX_GetStr(streamBuff + streamBuffUsed, readSize, wp->req->in) != (size_t)readSize)
    		{
                LOGE("read error %d\n", readSize);
                tempret = WEB_CODE_FileNotAccess;
            }
            else
            {
                //PrintBufferWrap(streamBuff, readSize, 32);
                streamBuffUsed += readSize;
                streamBuff[streamBuffUsed] = '\0';
            }
        }
#if 1
        if (0 == tempret)
        {
            char *lastPointer = NULL;
            char *buffPointer = NULL;
            char *nextBoundary = NULL;
            for (buffPointer = streamBuff; tempret == 0 && streamBuff + streamBuffUsed - buffPointer > boundaryLen; )
            {
                if (buffPointer <= lastPointer)
                {
                    PRINT_DBG("Can not process this. buffPointer=%p,streamBuff=%p,streamCounter=%d\n", buffPointer,streamBuff,streamCounter);
                    tempret = WEB_CODE_InvalidArg;
                    break;
                }
                lastPointer = buffPointer;
                
                nextBoundary = memmem(buffPointer, streamBuff + streamBuffUsed - buffPointer, boundary, boundaryLen);
                //PRINT_DBG("streamBuff=%p,buffPointer=%p,nextb=%p\n", streamBuff, buffPointer, nextBoundary);
                
                if (segmentName)
                {
                    unsigned int copyLen = nextBoundary ? nextBoundary - buffPointer : streamBuff + streamBuffUsed - buffPointer - boundaryLen;
                    // nextBoundary这行(不是首行boundary)之前的一个换行符不属于当前段内容.可以少拷贝(下次可以再拷贝),但不能多拷贝(会导致上传文件内容错误).
                    if (nextBoundary)
                    {
                        // 找出boundary前的最后一个换行符,那里是当前段的结束.
                        char *temp;
                        for (temp = nextBoundary - 2; \
                            temp > buffPointer && *temp != '\0' && (*temp != '\r' || *(temp+1) != '\n'); \
                            temp--);
                        if (*temp != '\0')
                        {
                            copyLen = temp - buffPointer;
                        }
                    }
                    else if (copyLen >= 2)//(nextBoundary && nextBoundary != buffPointer)
                    {
                        copyLen -= 2;
                    }
                    //PRINT_DBG("copyLen=%d\n", copyLen);
                    if (copyLen <= 0)
                    {
                        break;
                    }
                    
                    // 继续对segmentName所在段进行处理.
                    
                    if (strcmp(segmentName, "param") == 0 || strcmp(segmentName, "Username") == 0 || strcmp(segmentName, "PasswordDigest") == 0 || 
                        strcmp(segmentName, "Created") == 0 || strcmp(segmentName, "Nonce") == 0 || strcmp(segmentName, "uploadType") == 0)
                    {
                        if (copyLen > 0)
                        {
                            char *temp = realloc(segmentBuff, segmentBuffLen + copyLen + 1);
                            if (temp == NULL)
                            {
                                tempret = WEB_CODE_LackingMem;
                                free(segmentBuff);
                                segmentBuff = NULL;
                                segmentBuffLen = 0;
                            }
                            else
                            {
                                memcpy(temp + segmentBuffLen, buffPointer, copyLen);
                                temp[segmentBuffLen+copyLen] = '\0';
                                segmentBuff = temp;
                                segmentBuffLen += copyLen;
                            }
                            buffPointer += copyLen;
                        }

                        // 如果当前段的内容是完整的,就可以解析处理.
                        if (nextBoundary)
                        {
                            /************ param段内容 ************************************
                            *Content-Disposition: form-data; name="param"
                            *
                            *{"Username":"admin","PasswordDigest":"AARk3P/3MLvGZmZ9JQWqeRZeaqc=","Created":"2017-09-30T07:11:02.464Z","Nonce":"MmpkYTRuTTRCZDRyKzA2VE94ZnNRcyttMXNnPQ=="}
                            *************************************************************/
                            if (strcmp(segmentName, "param") == 0)
                            {
                                cJSON_Struct * paramSegment = Common_Json_Parse(segmentBuff, NULL, NULL);
                                if (paramSegment)
                                {
                                    PRINT_DBG("paramSegment\n");
                                    //Common_Json_StandardPrint(paramSegment, NULL, NULL, NULL);
                                    JsonOper_MergeObj(param, paramSegment, 0);
                                    Common_Json_Delete(paramSegment);
                                    paramSegment = NULL;
                                }
                            }
                            /************ Username段内容 ************************************
                            *Content-Disposition: form-data; name="Username"
                            *
                            *admin
                            *************************************************************/
                            else
                            {
                                PRINT_DBG("segmentName=%s,segmentBuff=%s\n", segmentName, segmentBuff);
                                Common_Json_SetAttrValueStr(param, segmentName, segmentBuff);
                            }
                        }
                    }
                    else if (strncmp(segmentName, "selfile",7) == 0)
                    {
//						LOGW("fd:%d   writeBytes:%d\n",fdUpload,copyLen);
						// 写入数据到文件中.
                        if (fdUpload >= 0)
                        {
                            write(fdUpload, buffPointer, copyLen);
                            fileCounter += copyLen;
                        }
                        
                        buffPointer += copyLen;

                        // 如果当前段完整的,就可以关闭上传的文件.
                        if (nextBoundary)
                        {
                            PRINT_DBG("Close file. fileCounter=%d.\n", fileCounter);
                            close(fdUpload);
                            fdUpload = -1;
                        }
                    }
                    else
                    {
                        buffPointer += copyLen;
                    }

                    // 遇到下个段,结束当前段.否则当前的streamBuff就拷贝完成,可以break去读下一部分数据.
                    if (nextBoundary)
                    {
                        free(segmentBuff);
                        segmentBuff = NULL;
                        segmentBuffLen = 0;
                        
                        free(segmentName);
                        segmentName = NULL;
                    }
                    else
                    {
                        break;
                    }
                }

                if (segmentName == NULL)
                {
                    const char *delimiter = "\r\n\r\n";
                    char *segmentStart = NULL;
                    if (nextBoundary == NULL)
                    {
                        // 没有找到boundary.理论上不可能.但还是要做防错处理.
                        buffPointer = streamBuff + streamBuffUsed - boundaryLen;
                    }
                    else if (nextBoundary[boundaryLen] == '-' && nextBoundary[boundaryLen+1] == '-' &&
                        nextBoundary[boundaryLen+2] == '\r' && nextBoundary[boundaryLen+3] == '\n')
                    {
                        // 表单结束.
                        //LOGD("Form completed.\n");
                        buffPointer = nextBoundary + boundaryLen + 4;
                        PRINT_DBG("Form completed.\n");
                    }
                    else if ((segmentStart = strstr(buffPointer, delimiter)) == NULL)
                    {
                        // 找不到段正文的分隔符.
                        buffPointer = nextBoundary;
                        PRINT_DBG("Can not find delimiter.\n");
                    }
                    else
                    {
                        buffPointer = nextBoundary + boundaryLen;
                        // boundary的下一行就是name信息.
                        // 举例,Content-Disposition: form-data; name="param"
                        char *nextLine = buffPointer + 2;
                        const char *formLineHead = "Content-Disposition: form-data; name=\"";
                        const int formLineHeadLen = strlen(formLineHead);
                        char *namePos = nextLine + formLineHeadLen;
                        char *nameEnd = NULL;
                        char *fileNamePos = NULL;
                        char *fileNameEnd = NULL;
                        if (buffPointer[0] != '\r' || buffPointer[1] != '\n')
                        {
                            PRINT_DBG("Error\n");
                            // 错误的格式.boundary必须是换行.
                            tempret = WEB_CODE_InvalidArg;
                        }
                        else if (strncmp(nextLine, formLineHead, formLineHeadLen) != 0)
                        {
                            PRINT_DBG("Error\n");
                            // 错误的格式.boundary下一行内容是固定的.
                            tempret = WEB_CODE_InvalidArg;
                        }
                        else if ((nameEnd = strchr(namePos, '"')) == NULL)
                        {
                            PRINT_DBG("Error\n");
                            // 错误的格式.没有匹配到name字符串结尾.
                            tempret = WEB_CODE_InvalidArg;
                        }
                        else if ((segmentName = strndup(namePos, nameEnd - namePos)) == NULL)
                        {
                            PRINT_DBG("Error\n");
                            // 内存分配失败.
                            tempret = WEB_CODE_LackingMem;
                        }
                        else if (strncmp(segmentName, "selfile",7) == 0 && 
                            ( (fileNamePos = strstr(nextLine, "filename=\"")) == NULL || 
                            (fileNameEnd = strchr(fileNamePos + 10, '"')) == NULL || fileNameEnd < fileNamePos + 10 + 4) )
                        {
                            PRINT_DBG("Error\n");
                            // 错误的格式.没有匹配到filename.
                            tempret = WEB_CODE_InvalidArg;
                        }
                        else
                        {
                            LOGD("segmentName=%s\n", segmentName);
							 
                            if (strncmp(segmentName, "selfile",7) == 0)
                            {
                            	
                                fileNamePos += 10;
                                char fileName[256] = "/dev/";//ovfs_ipc.update
                                if(0){
									memcpy(fileName,"/dev/ovfs_ipc.update",slen("/dev/ovfs_ipc.update"));
								}else{
	                                char tmp[256] = {0};//long filename maybe overflow
									LOGD("%p %p len:%d\n",fileNamePos,fileNameEnd,fileNameEnd - fileNamePos);
									memcpy(tmp,fileNamePos, fileNameEnd - fileNamePos);
									LOGD("tmp:%s\n",tmp);
	                              	//some version IE send not filename but full filepath.we should deal with it
	                                char* pos = NULL;
									if(((pos = strrchr(tmp, '\\')) != NULL) || ((pos = strrchr(tmp, '/')) != NULL)){
										pos++;	
										LOGD("basename:%s\n",pos);
										
										memcpy(fileName + 5, pos, slen(pos));
										fileName[5 + slen(pos)] = '\0';

									}else{
										LOGD("no basename:\n");
										memcpy(fileName + 5, tmp, slen(tmp));
										fileName[5 + slen(tmp)] = '\0';
									}
									
                                }

								LOGD("filename:%s\n",fileName);
								
                                struct stat tmpStat;
                                if (stat(fileName, &tmpStat) == 0)
                                {
                                   // tempret = WEB_CODE_TaskExist;
                                    PRINT_DBG("%s exist!remove it\n",fileName);
									remove(fileName);
                                }

								if ((fdUpload = open(fileName, O_CREAT | O_RDWR | O_TRUNC,S_IRWXU|S_IRUSR|S_IXUSR|S_IROTH|S_IXOTH)) < 0)
                                {
                                    tempret = WEB_CODE_FileNotAccess;
                                    PRINT_DBG("Error\n");
                                }
                                else
                                {
                                    PRINT_DBG("Open file %s.\n", fileName);
                                    wp->upload_filename[file_count++] = sclone(fileName);
                                }
                            }
                        }

                        buffPointer = segmentStart + 4;
                    }
                }

            }
            
            //! 剩余最后未处理完的数据移动到起始地址.
            memmove(streamBuff, buffPointer, streamBuff + streamBuffUsed - buffPointer);
            streamBuffUsed = streamBuff + streamBuffUsed - buffPointer;
        }
#endif
    }
    ret = tempret;

    if (param)
    {
        PRINT_DBG("upload param=\n");
        Common_Json_StandardPrint(param, NULL, NULL, NULL);

        char *param_str = Common_Json_PrintUnformatted(param, NULL);
        char body[512] = {0};
        snprintf(body,sizeof(body),"{\"Ch\":1,\"Type\":0,\"Data\":%s}",param_str);
        wp->query = strdup(body);
        LOGW("query:[%s]\n",wp->query);
        free(param_str);

        char *valueStr;
        if (Common_Json_GetAttrValueStr(param, "Username", &valueStr))
        {
            if (wp->username)
            {
                free(wp->username);
                wp->username = NULL;
            }
            wp->username = sclone(valueStr);
        }
        if (Common_Json_GetAttrValueStr(param, "PasswordDigest", &valueStr))
        {
            if (wp->wsse_passwordDigest)
            {
                free(wp->wsse_passwordDigest);
                wp->wsse_passwordDigest = NULL;
            }
            wp->wsse_passwordDigest = sclone(valueStr);
        }
        if (Common_Json_GetAttrValueStr(param, "Created", &valueStr))
        {
            if (wp->wsse_created)
            {
                free(wp->wsse_created);
                wp->wsse_created = NULL;
            }
            wp->wsse_created = sclone(valueStr);
        }
        if (Common_Json_GetAttrValueStr(param, "Nonce", &valueStr))
        {
            if (wp->wsse_nonce)
            {
                free(wp->wsse_nonce);
                wp->wsse_nonce = NULL;
            }
            wp->wsse_nonce = sclone(valueStr);
        }
        if (Common_Json_GetAttrValueStr(param, "uploadType", &valueStr))
        {
            if (wp->uploadType)
            {
                free(wp->uploadType);
                wp->uploadType = NULL;
            }
            wp->uploadType = sclone(valueStr);
        }
        
        Common_Json_Delete(param);
        param = NULL;
    }

    if (streamBuff)
    {
        free(streamBuff);
        streamBuff = NULL;
        streamBuffSize = 0;
        streamBuffUsed = 0;
    }
    
    if (segmentName)
    {
        free(segmentName);
        segmentName = NULL;
    }
    
    if (segmentBuff)
    {
        free(segmentBuff);
        segmentBuff = NULL;
        segmentBuffLen = 0;
    }
    
    if (boundary)
    {
        free(boundary);
        boundary = NULL;
    }
    
    return ret;
}

