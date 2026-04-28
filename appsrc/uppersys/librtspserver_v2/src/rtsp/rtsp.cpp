#include "rtsp.h"

#include <stdio.h>

#include <time.h>
//#include "Base64.h"
#include "digcalc.h"

#include "rtsp_common.h"

//#include "rtspserver_new.h"
#ifdef WIN32
#include<Mstcpip.h>
#include <ws2tcpip.h>
#else
#include <syslog.h>
#include <sys/syscall.h>
#endif
//#define FORCE_OVER_TCP



static char sAndCheck[][6] = {"nbsp","lt","gt","amp","apos","quot","copy","reg"};



int Ants_Rtsp_ParseUrl(char *pUrl,RTSP_URL_T *pResult)
{
    int idx;
    char c;
    char tembuf[256];
    int pos;
    int step;
	int bIPv6 = 0,bIPv6Start = 0;
    char *pName = NULL,*pValue = NULL;
//[rtsp,urtsp]://[ip,dnsname][:port]/file?ctype=[video;audio;videoback;audioback;appxxx]&stype=[unicast,multicast]&ptype=[tcp,http,udp]&ip=[224.x.x.x]&port=x
// 兼容回放
// rtsp://192.168.25.84/recordstream?starttime=20140627T154000Z&endtime=20140627T154002Z
// rtsp://192.168.25.84/recording?ch=0&stream=0&start=20140627154000&stop=20140627154002
//  rtsp://192.168.25.84/recording_comb?ch=0&stream=0&start=20140627154000&stop=20140627154002 // 多通道回放
//  rtsp://192.168.25.84/living_comb01.264 // 多通道实时流
// ipv6 : rtsp://[::1]:554/ch01.264
    if (pUrl == NULL || pResult == NULL)
    {
        return -1;
    }
    memset(pResult,0,sizeof(RTSP_URL_T));
    idx = 0;
    pos = 0;
    step = 0;
	pValue = strstr(pUrl, "@");
    if (pValue != NULL)
    {
        idx += pValue - pUrl + 1;
		step = 1;
	}

    while(1)
    {
        c = pUrl[idx];
        if (c == '?')
        {
            pResult->bHasParam = 1;
        }
        if (step == 0)
        {
            //处理rtsp/urtsp
            if (c == ':')
            {
                if (pUrl[idx + 1] != '/' ||
                    pUrl[idx + 2] != '/')
                {

                    return -1;
                }

                tembuf[pos] = 0;
                if (pos == 0)
                {

                    return -1;
                }
                if (!strnicmp(tembuf,"rtsp",4))
                {
                    pResult->nProtoType = 0;
                }
                else if (!stricmp(tembuf,"urtsp"))
                {

                    return -2;//不支持的协议
                }
                else
                {
                    //不支持
printf("-------------------unsupported protocol\n");
                    return -2;//不支持的协议
                }
                pos = 0;
                idx += 3;
                step = 1;//处理ip/dnsname
                continue;
            }
        }
        else if (step == 1)
        {
            //处理ip/dnsname
			if (c == '[')
			{
				bIPv6 = 1;
				bIPv6Start = 1;
				pos = 0;
				idx ++;
				continue;
			}


			if (c == ']')
			{
				if (bIPv6Start)
				{
					bIPv6Start = 0;
					idx++;
					continue;
				}
				else
				{
					return -301;
				}
			}
			if (bIPv6Start)
			{
			}
            else if (c == ':' || c == '/')
            {

                tembuf[pos] = 0;
                if(inet_addr(tembuf) != INADDR_NONE)
                {
                    pResult->bIP = 1;
                }
                if (pos > ANTS_RTSP_MAX_DNSNAME_LEN)
                {

                    return -3;//太长的IP/DNSNAME
                }
                strcpy(pResult->pAddress,tembuf);
                if (c == ':')
                {
                    //带端口
                    step = 2;//处理端口

                }
                else
                {
                    step = 3;//处理流文件名
                }
                pos = 0;
                idx++;
                continue;
            }
        }
        else if (step == 2)
        {
            //处理端口
            if (c == '/')
            {
                tembuf[pos] = 0;
                pResult->nPort = atoi(tembuf);

                pos = 0;
                step = 3;//处理流文件名
                idx++;
                continue;
            }
            if (c < '0' || c > '9')
            {

                return -4;//错误的端口
            }
        }
        else if (step == 3)
        {
            //处理流文件名
           // if (c == '/'|| c == '?' || c == 0 || c == '&')
            if(c == '?' || c == 0 || c == '&')
            {
                tembuf[pos] = 0;
                if (c == '/')
                {

                    //  return -11;//错误的格式
                }
                if (pos > ANTS_RTSP_MAX_FILENAME_LEN)
                {

                    return -5;//太长的文件名
                }
                if (c == '&')
                {
					if (0 == strncmp(&pUrl[idx],"&amp;",5))
					{
						idx+=4;
					}

					pResult->bHasParam = 1;
                }
                if(c == '?')
                {
                    pResult->bHasParam = 1;
                }
                strcpy(pResult->pStreamFileName,tembuf);
                if (strcmp(pResult->pStreamFileName,"recording") == 0)
                {
                    pResult->bRecording = 1;
                }
                else if (strcmp(pResult->pStreamFileName,"recordstream") == 0)
                {
                    pResult->bRecording = 1;
                    pResult->nCustom = 1;
                }
				else if (strcmp(pResult->pStreamFileName,"recording_comb") == 0)
				{
                    pResult->bRecording = 1;
                    pResult->bAntsComb = 1;
				}
				else if (strnicmp(pResult->pStreamFileName,"living_comb",11) == 0)
				{
				    pResult->bAntsComb = 1;
				}
                pos = 0;
                step = 4;//处理后面参数，直到结束
                idx++;
                if (c == 0)
                {
                    break;
                }
                continue;
            }
        }
        else if (step == 4)
        {
            if (c == '&' || c == '/'|| c == 0)
            {

                tembuf[pos] = 0;



                if (!strnicmp(tembuf,"ctype=",6))
                {
                    if (!stricmp(tembuf+6,"video"))
                    {
                        pResult->ctype = 1;
                    }
                    else if (!stricmp(tembuf+6,"audio"))
                    {
                        pResult->ctype = 2;
                    }
                    else if (!stricmp(tembuf+6,"videoback"))
                    {
                        pResult->ctype = 3;
                    }
                    else if (!stricmp(tembuf+6,"audioback"))
                    {
                        pResult->ctype = 4;
                    }
                    else if (!strnicmp(tembuf+6,"app",3))
                    {
                        pResult->ctype = 5;
                        pResult->byAppPayloadType = atoi(tembuf+6+3);
                    }
                    else
                    {

                        return -6;//不支持的流类型
                    }
                }
                else if (!strnicmp(tembuf,"stype=",6))
                {
                    if (!stricmp(tembuf+6,"unicast"))
                    {
                        pResult->stype = 1;
                    }
                    else if (!stricmp(tembuf+6,"multicast"))
                    {
                        pResult->stype = 2;
                    }
                    else
                    {

                        return -7;//不支持的传输类型
                    }
                }
                else if (!strnicmp(tembuf,"ptype=",6))
                {
                    if (!stricmp(tembuf+6,"udp"))
                    {
                        pResult->ptype = 1;
                    }
                    else if (!stricmp(tembuf+6,"tcp"))
                    {
                        pResult->ptype = 2;
                    }
                    else if (!stricmp(tembuf+6,"http"))
                    {
                        pResult->ptype = 3;
                    }
                    else
                    {

                        return -8;//不支持的传输协议类型
                    }
                }
                else if (!strnicmp(tembuf,"ip=",3))
                {
                    pResult->destIPV[0] = inet_addr(tembuf+3);
                    pResult->destIPA[0] = pResult->destIPV[0];
                }
                else if (!strnicmp(tembuf,"port=",5))
                {
                    pResult->destPortV = atoi(tembuf+5);
                    if (pResult->destPortV <= 0 ||
                        pResult->destPortV >= 0xFFFF)
                    {

                        return -12;//端口错误
                    }
                    if(pResult->destPortV & 1)
                        pResult->destPortV = 0;
                }
                else if (!strnicmp(tembuf,"portv=",6))
                {
                    pResult->destPortV = atoi(tembuf+6);
                    if (pResult->destPortV <= 0 ||
                        pResult->destPortV >= 0xFFFF)
                    {

                        return -12;//端口错误
                    }
                    if(pResult->destPortV & 1)
                        pResult->destPortV = 0;
                }
                else if (!strnicmp(tembuf,"ipv=",4))
                {
                    pResult->destIPV[0] = inet_addr(tembuf+4);
                }
                else if (!strnicmp(tembuf,"portv=",6))
                {
                    pResult->destPortV = atoi(tembuf+6);
                    if (pResult->destPortV <= 0 ||
                        pResult->destPortV >= 0xFFFF)
                    {

                        return -12;//端口错误
                    }
                    if(pResult->destPortV & 1)
                        pResult->destPortV = 0;
                }
                else if (!strnicmp(tembuf,"ipa=",4))
                {
                    pResult->destIPA[0] = inet_addr(tembuf+4);
                }
                else if (!strnicmp(tembuf,"porta=",6))
                {
                    pResult->destPortA = atoi(tembuf+6);
                    if (pResult->destPortA <= 0 ||
                        pResult->destPortA >= 0xFFFF)
                    {

                        return -12;//端口错误
                    }
                    if(pResult->destPortA & 1)
                        pResult->destPortA = 0;
                }
                else if (!strnicmp(tembuf,"ttl=",4))
                {
                    pResult->nTTL = atoi(tembuf+4);
                    if (pResult->nTTL < 0 || pResult->nTTL > 255)
                    {
                        pResult->nTTL = 255;
                    }
                }
                else if (!strnicmp(tembuf,"starttime=",10))
                {
                    int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0;
                    sscanf(tembuf+10,"%04d%02d%02dT%02d%02d%02d",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec);
                    pResult->tStart.wYear = nYear;
                    pResult->tStart.byMon = nMon;
                    pResult->tStart.byDay = nDay;
                    pResult->tStart.byHour = nHour;
                    pResult->tStart.byMin = nMin;
                    pResult->tStart.bySec = nSec;
                    pResult->bTimeValid |= 1;
                }
                else if (!strnicmp(tembuf,"startime=",9))
                {
                    int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0;
                    sscanf(tembuf+9,"%04d%02d%02dT%02d%02d%02d",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec);
                    pResult->tStart.wYear = nYear;
                    pResult->tStart.byMon = nMon;
                    pResult->tStart.byDay = nDay;
                    pResult->tStart.byHour = nHour;
                    pResult->tStart.byMin = nMin;
                    pResult->tStart.bySec = nSec;
                    pResult->bTimeValid |= 1;
                }
                else if (!strnicmp(tembuf,"endtime=",8))
                {
                    int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0;
                    sscanf(tembuf+8,"%04d%02d%02dT%02d%02d%02d",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec);
                    pResult->tStop.wYear = nYear;
                    pResult->tStop.byMon = nMon;
                    pResult->tStop.byDay = nDay;
                    pResult->tStop.byHour = nHour;
                    pResult->tStop.byMin = nMin;
                    pResult->tStop.bySec = nSec;
                    pResult->bTimeValid |= 2;
                }
                else if (!strnicmp(tembuf,"start=",6))
                {
                    int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0;
                    sscanf(tembuf+6,"%04d%02d%02d%02d%02d%02d",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec);
                    pResult->tStart.wYear = nYear;
                    pResult->tStart.byMon = nMon;
                    pResult->tStart.byDay = nDay;
                    pResult->tStart.byHour = nHour;
                    pResult->tStart.byMin = nMin;
                    pResult->tStart.bySec = nSec;
                    pResult->bTimeValid |= 1;
                }
                else if (!strnicmp(tembuf,"stop=",5))
                {
                    int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0;
                    sscanf(tembuf+5,"%04d%02d%02d%02d%02d%02d",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec);
                    pResult->tStop.wYear = nYear;
                    pResult->tStop.byMon = nMon;
                    pResult->tStop.byDay = nDay;
                    pResult->tStop.byHour = nHour;
                    pResult->tStop.byMin = nMin;
                    pResult->tStop.bySec = nSec;
                    pResult->bTimeValid |= 2;

                }
                else if (!strnicmp(tembuf,"ch=",3))
                {
                    pResult->nCh = atoi(tembuf+3);
                }
                else if (!strnicmp(tembuf,"stream=",7))
                {
                    pResult->nStream = atoi(tembuf+7);
                }
                else if(pResult->pStreamFileName[0] != 0)
                {
                    //忽悠其他参数

                    pValue = strstr(tembuf,"=");
                    if (pValue == NULL)
                    {
                        if (tembuf[0] != 0)
                        {
                            snprintf(pResult->pStreamFileName,ANTS_RTSP_MAX_FILENAME_LEN - 1,"%s_%s",pResult->pStreamFileName,tembuf);
                        }

                    }
                    else
                    {
                        pName = Ants_strndup(tembuf,pValue - tembuf);
                        if(pName != NULL)
                        {
                            snprintf(pResult->pStreamFileName,ANTS_RTSP_MAX_FILENAME_LEN - 1,"%s_%s_%s",pResult->pStreamFileName,pName,pValue + 1);
							delete []pName;
							pName = NULL;
                        }
                    }
                }
                if (c == '&')
                {
                    int n;
                    if (0 == strncmp(&pUrl[idx],"&amp;",5))
                    {
                        idx+= 4;
                    }
                }
                pos = 0;
                idx++;
                if (c == 0)
                {
                    break;
                }
                continue;
            }
        }


        if (c == 0)
        {
            //结束
            break;
        }

        if (c == ' ' || c == '+')
        {
            //去掉空格
            idx++;
            continue;
        }


        tembuf[pos++] = c;
        idx++;
    }

    if (step != 4)
    {

        return -1;//不完整的URL
    }
	pResult->bIPv6 = bIPv6;
    //检查IP是否正确
    if (pResult->stype == 1)
    {
        //多播
        if (pResult->destIPV[0] != 0)
        {
            if (pResult->destIPV[0] < 0xE0000000 ||
                pResult->destIPV[0] > 0xEFFFFFFF)
            {

                return -9;//多播地址不对
            }

        }
		if (pResult->destPortA < 0 ||
			pResult->destPortA >= 65535)
		{

			return -10;//端口不对
		}

        if (pResult->destIPA[0] != 0)
        {
            if (pResult->destIPA[0] < 0xE0000000 ||
                pResult->destIPA[0] > 0xEFFFFFFF)
            {

                return -9;//多播地址不对
            }

        }
		if (pResult->destPortA < 0 ||
			pResult->destPortA >= 65535)
		{

			return -10;//端口不对
		}
        if (pResult->destIPV[0] == pResult->destIPA[0] && pResult->destPortA == pResult->destPortV && pResult->destPortV)
        {

            return -10;
        }

    }


    return 0;

}



int CClientSocket::m_nSessionCnt = 1;
unsigned int CClientSocket::m_dwRecordCreateCnt = 0;
CClientSocket::CClientSocket(CRtspServer *pServer,int clientSocket,int bIPv6, SSL *pSsl):
    m_nClientSocket(clientSocket),m_pNext(NULL),m_pPrev(NULL),
    m_nSessionID(0), m_nReadLen(0),m_nWriteLen(0),m_pMsg(NULL),m_pMsg_tail(NULL)
{
    time_t tim;
    int rand_num;
    m_bHasError = 0;
    m_nErrorCode = 0;
    m_pRtspServer = pServer;
    m_hRtpSess = -1;
    m_pRtpSess = NULL;
    m_bSetup = 0;
    m_bTransType = 0;
	m_bIPv6 = bIPv6;

    m_tLastRefreshTime = 0;

    m_bOverHTTP = 0;
    m_pOverHttpSessionCookie = NULL;
    m_pOutPutSocket = NULL;
	m_pInPutSocket = NULL;

    m_pDollarBuffer = NULL;
    m_bDollar = 0;
    m_nDollarLen = 0;
    m_nDollarPos = 0;
    m_bNeedClose = 0;
	m_nReason = 0;

    m_pSsl = pSsl;

    m_hWriteLock.Init();
    m_hMsgLock.Init();



	memset(m_dwIPAdress_VA,0,sizeof(m_dwIPAdress_VA));
	memset(m_nPort_VA,0,sizeof(m_nPort_VA));
	memset(m_nTTL_VA,255,sizeof(m_nTTL_VA));


	m_nInterleaved = 0;

    m_bPlay = 0;
    m_bStreamOpen = 0;
    m_pCurrUrl = NULL;
    m_pRequire = NULL;
    m_pVideoSession = NULL;
    m_pAudioSession = NULL;
    m_nRequireType = 0;

    m_nRequireVideoType = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
    m_nRequireAudioType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
	memset(m_dwIPAddress,0,sizeof(m_dwIPAddress));
	if(bIPv6)
	{
		GetIPv6((unsigned char *)m_dwIPAddress);

	}
	else
	{

		m_dwIPAddress[0] = GetIP();
	}
	RTSP_DEBUG("IPV6(%d) clientip [%x,%x,%x,%x]\n",bIPv6,m_dwIPAddress[0],m_dwIPAddress[1],m_dwIPAddress[2],m_dwIPAddress[3]);
    m_uPort = GetPort();
    m_bPlayReady = 0;
    // CreateRTPSessionTCP(int handle);

    if(m_nSessionID == 0)
    {
        time(&tim);
        srand(tim + m_nSessionCnt);
        rand_num = (int)(255.0*rand()/(RAND_MAX+1.0));
        rand_num &= 0xFF;
        m_nSessionCnt++;
        m_nSessionCnt &= 0xFF;

        //产生ID
        m_nSessionID = ((tim & 0x7FFF) << 16) | m_nSessionCnt |(rand_num << 8) ;
    }
    m_dwStreamDataProp = 0;
    m_pStreamDataUser = NULL;
    m_fStreamDataCallback = NULL;

    m_nWritePos = 0;
	m_nReadPos = 0;
    m_nSendPos = 0;
    m_bRecording = 0;
    m_bRateControl = 1;
    m_bUsing = 0;
    m_bWaitDelete = 0;
    m_bThreadFlag = 0;

    m_bCreateRecordStream = 0;
    m_bModeOpen = 0;
    pStreamUser = NULL;

    m_nTTL[0] = m_nTTL[1] = 255;
	memset(m_dwIPv4Multicast,0,sizeof(m_dwIPv4Multicast));
    m_dwIPv4Multicast[0][0] = 0x010101E0;
    m_dwIPv4Multicast[1][0] = 0x020101E0;
    m_nMulticastPort[0] = 5678;
    m_nMulticastPort[1] = 5680;
    m_nMulticastCnt[0] =
        m_nMulticastCnt[1] = 0;

    m_bSupportMulticast = 1;
    m_bPlayloadType[0] = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
    m_bPlayloadType[1] = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
    m_nPlayloadStreamType[0] = ANTS_RTSP_CALLBACKBYPE_STREAM_H264;
    m_nPlayloadStreamType[1] = ANTS_RTSP_CALLBACKBYPE_STREAM_G711U;
    m_nPlayloadClockRate[0] = 90000;
    m_nPlayloadClockRate[1] = 8000;
    sprintf(m_szPlayloadName[0],"H264");
    sprintf(m_szPlayloadName[1],"PCMU");
    m_StreamType = ANTS_RTSPSERVER_STREAMTYPE_ALL;

    memset(m_pAppMgr,0,sizeof(m_pAppMgr));

	m_pSPS = NULL;
    m_pPPS = NULL;
    m_pVPS = NULL;
    m_nSPSLen = 0;
    m_nPPSLen = 0;
    m_nVPSLen = 0;
	m_pVPS_un = NULL;
	m_nVPS_un_Len = 0;

    m_nCh = -1;
    m_nStream = -1;

    m_bTCP = 0;
    m_nScale = 0;
    m_bUsed = 0;
    m_bAntsComb = 0;

	m_pSendMsg = NULL;
	m_pSendMsg_tail = NULL;
    m_pUserName = NULL;
    m_pPassword = NULL;
    m_bAuthOK = 0;
    m_pNonce = NULL;
	m_bCheckAndSendUDP = 0;
	m_bPlaying_status = 0;
    //Create();
}


int CClientSocket::GetSocket()
{
    int nSock = -1;

    nSock = m_nClientSocket;

    return nSock;
}


CClientSocket::~CClientSocket()
{
    Destroy();
}


int CClientSocket::Create()
{
    int nRet = -1;

    m_bThreadExit = 0;
    nRet = Start(16 * 1024 * 64);//开始线程
    if (nRet)
    {
        RTSP_ERROR("start thread failed\n");
        Destroy();
        return -1;
    }

    return 0;
}


int CClientSocket::Destroy()
{
    CMessage *p,*pCurr;
	RTSP_SEND_MSG_T *pSend, *pSendCurr;
    int nVideoCnt = 0, nAudioCnt = 0, nSetup = 0, nRet = -1, nAppCnt = 0;

    RTSP_DEBUG("closing [%d][%d] m_bPlay = %d m_bSetup = %d m_bTransType = %d \n",m_nClientSocket,m_hRtpSess,m_bPlay,m_bSetup,m_bTransType);

#if 0
    m_bThreadExit = 1;

    if (m_bThreadFlag > 0)
    {
        while (1)
        {
            if (m_bThreadFlag == 2)
            {
                break;
            }

            Ants_RTSPServer_WaitTime(0,10000);
        }
    }
#endif

    if (m_bPlay)
    {
        if (m_bSetup & 3)
        {
            //播放
            if (m_bTransType == 3 ||
                m_bTransType == 4)
            {
                //tcp
                if(m_bSetup &1)
                {
                    m_pRtspServer->RemoveRtpDataClient(m_hRtpSess,m_dwIPAddress,m_uPort,m_nPort_VA[0],0,1,this);
                }
                if(m_bSetup &2)
                {
                    m_pRtspServer->RemoveRtpDataClient(m_hRtpSess,m_dwIPAddress,m_uPort,m_nPort_VA[1],1,1,this);
                }
            }
            else
            {
                if(m_bSetup &1)
                {
                    m_pRtspServer->RemoveRtpDataClient(m_hRtpSess,m_dwIPAdress_VA[0],m_nPort_VA[0],0,0,0,this);
                }
                if(m_bSetup &2)
                {
                    m_pRtspServer->RemoveRtpDataClient(m_hRtpSess,m_dwIPAdress_VA[1],m_nPort_VA[1],0,1,0,this);
                }
            }
        }

        for (int nAppIdx = 0; nAppIdx < RTSP_APP_SUPPORT_MAX_NUM; nAppIdx++)
        {
            if (m_bSetup & (1 << (4 + nAppIdx)))
            {
                RTSP_DEBUG("CClientSocket[%s.%d]\n",__FUNCTION__,__LINE__);
                CAppRtpSession *pAppSess = NULL;

                if (m_pRtpSess != NULL && m_pAppMgr[nAppIdx] != NULL)
                {
                    pAppSess = m_pRtpSess->GetAppSessionByType(m_pAppMgr[nAppIdx]->byAppPayloadType, m_pAppMgr[nAppIdx]->dwClockRate, m_pAppMgr[nAppIdx]->szAppName);
                }

                if (pAppSess == NULL)
                {
                    continue;
                }
                if (m_bTransType == 3 ||
                    m_bTransType == 4)
                {
                    pAppSess->DeleteDestination(m_dwIPAddress,m_uPort,1,m_nPort_VA[4 + nAppIdx],this);
                }
                else
                {
                    pAppSess->DeleteDestination(m_dwIPAdress_VA[4 + nAppIdx],m_nPort_VA[4 + nAppIdx],0,0,this);
                }

                if (pAppSess->GetDestinationCnt() == 0)
                {
                    if (m_pRtpSess != NULL)
                    {
                        //m_pRtpSess->DeleteAppStream(m_pAppMgr[nAppIdx]->byAppPayloadType);
                    }
                }
            }
        }
    }

    if (m_pVideoSession != NULL)
    {
        m_pVideoSession->Destroy();
        delete m_pVideoSession;
        m_pVideoSession = NULL;
    }

    if (m_pAudioSession != NULL)
    {
        m_pAudioSession->Destroy();
        delete m_pAudioSession;
        m_pAudioSession = NULL;
    }

    if (m_pRtpSess != NULL)
    {
        //if (m_bUsed == 1)
        //{
        m_pRtpSess->SetSetup(0);
         //   m_bUsed = 0;
        //}

        if (m_pRtpSess->GetVideo()!= NULL)
        {
            nVideoCnt = m_pRtpSess->GetVideo()->GetDestinationCnt();
        }

        if (m_pRtpSess->GetAudio()!= NULL)
        {
            nAudioCnt = m_pRtpSess->GetAudio()->GetDestinationCnt();
        }

        nAppCnt = m_pRtpSess->GetAppCnt();
        nSetup = m_pRtpSess->IsSetup();
        RTSP_DEBUG("\n***nVideoCnt [%d] nAudioCnt [%d] nAppCnt[%d] nSetup[%d]***\n", nVideoCnt, nAudioCnt, nAppCnt, nSetup);
    }

    if (m_nClientSocket > 0)
    {
        RTSP_DEBUG("\n*** Close [%d] ***\n",m_nClientSocket);
		shutdown(m_nClientSocket, SHUT_RDWR);
        closeSocket(m_nClientSocket);
        m_nClientSocket = -1;
    }

    p = m_pMsg;
    while(p != NULL)
    {
        pCurr = p;
        p = p->m_pNext;
        if (pCurr->m_pMessage != NULL)
		{
            delete [](pCurr->m_pMessage);
			pCurr->m_pMessage = NULL;
		}
        delete pCurr;
    }
    m_pMsg = NULL;
    m_pMsg_tail = NULL;

    pSend = m_pSendMsg;
    while(pSend != NULL)
    {
        pSendCurr = pSend;
        pSend = pSend->pNext;
		if (pSendCurr->pData != NULL)
		{
            delete [](pSendCurr->pData);
			pSendCurr->pData = NULL;
		}
        free(pSendCurr);
		pSendCurr = NULL;
    }
    m_pSendMsg = NULL;
    m_pSendMsg_tail = NULL;

    m_pRtspServer->DecWriteLen(m_nWriteLen);
    m_nWriteLen = 0;



    if (m_pOverHttpSessionCookie != NULL)
    {
		//printf("[%s.%d][%s]this = %p out = %p in = %p\n",__FUNCTION__,__LINE__,m_pOverHttpSessionCookie,this,m_pOutPutSocket,m_pInPutSocket);
		m_pRtspServer->GLock();
        CClientSocket *pDel = m_pOutPutSocket;
        m_pOutPutSocket = NULL;
		if (pDel != NULL)
		{
			if (pDel->m_pInPutSocket != NULL)
			{
				pDel->m_pInPutSocket = NULL;
			}
		}
		m_pRtspServer->GUnlock();
        if (pDel != NULL)
        {
            m_pRtspServer->RemoveClientSocket(pDel);
			pDel = NULL;
        }
		m_pRtspServer->GLock();
		pDel = m_pInPutSocket;
		m_pInPutSocket = NULL;
		if (pDel != NULL)
		{
			if (pDel->m_pOutPutSocket != NULL)
			{
				pDel->m_pOutPutSocket = NULL;
			}
		}
		m_pRtspServer->GUnlock();
		if (pDel != NULL)
		{
			m_pRtspServer->RemoveClientSocket(pDel);
			pDel = NULL;
		}
		//printf("[%s.%d][%s]this = %p finished\n",__FUNCTION__,__LINE__,m_pOverHttpSessionCookie,this);

        m_pRtspServer->RemoveHttpSessionCookie(m_pOverHttpSessionCookie);
        delete m_pOverHttpSessionCookie;
        m_pOverHttpSessionCookie = NULL;
		//printf("[%s.%d]this = %p finished\n",__FUNCTION__,__LINE__,this);
    }
    // 关掉内部创建的流
    RTSP_DEBUG("\n*** m_bCreateRecordStream [%d] nVideoCnt [%d] nAudioCnt [%d] nAppCnt[%d] nSetup[%d]***\n",m_bCreateRecordStream, nVideoCnt, nAudioCnt, nAppCnt, nSetup);
    if (m_bCreateRecordStream && nVideoCnt == 0 && nAudioCnt == 0 && nAppCnt == 0 && nSetup == 0)
    {
        nRet = m_pRtspServer->DestroyStream(m_hRtpSess);
        RTSP_DEBUG("\n*** DestroyStream [%d]***\n", nRet);
        m_bCreateRecordStream = 0;

        if(m_bStreamOpen && nRet == 0)
        {
#if 0
            Ants_RTSPV2_InParam_T tInParam;
            Ants_RTSPV2_OutParam_T tOutParam;
            memset(&tOutParam,0,sizeof(tOutParam));
            memset(&tInParam,0,sizeof(tInParam));
            tInParam.pUrl = m_pCurrUrl;
            tInParam.dwClientIP = m_dwIPAddress;
            tInParam.wRemotePort = m_uPort;
            tInParam.dwSessionID = m_nSessionID;
            tInParam.pRequire = m_pRequire;
            tInParam.bRecording = m_bRecording;
            tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
#endif

            // 开始调用停止播放
            if(m_bPlay)
            {
                m_bPlay = 0;
            }

            //开始关掉
            if (m_bModeOpen)
            {
                if (m_nRequireType != 0)
                {
                    m_pRtspServer->fClose(m_hRtpSess,pStreamUser,2,m_pCurrUrl);
                }
                else
                {
                    m_pRtspServer->fClose(m_hRtpSess,pStreamUser,m_bRecording,m_pCurrUrl);
                }
            }

            m_bModeOpen = 0;
            m_bStreamOpen = 0;
        }
    }
#if 1
	{
		Ants_RTSPV2_InParam_T tInParam;
		Ants_RTSPV2_OutParam_T tOutParam;
		memset(&tOutParam,0,sizeof(tOutParam));
		memset(&tInParam,0,sizeof(tInParam));
		tInParam.pUrl = m_pCurrUrl;
		tInParam.dwClientIP = m_dwIPAddress[0];
		tInParam.wRemotePort = m_uPort;
		tInParam.dwSessionID = m_nSessionID;
		tInParam.pRequire = m_pRequire;
		tInParam.bRecording = m_bRecording;
		tInParam.nReason = m_nReason;
		tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
		tInParam.bIPv6 = m_bIPv6;
		tInParam.dwClientIPv6[0] = m_dwIPAddress[0];
		tInParam.dwClientIPv6[1] = m_dwIPAddress[1];
		tInParam.dwClientIPv6[2] = m_dwIPAddress[2];
		tInParam.dwClientIPv6[3] = m_dwIPAddress[3];

		// 开始调用停止播放
		// if(m_bPlaying_status)
		 // 始终 停止 播放，否则在setup后 play 之前中止的话，导致不能无法通知应用连接已经断开
		{

				if (0 == m_pRtspServer->fStatusCallBack(m_nSessionID,ANTS_RTSPSERVER_CALLBACK_TYPE_STOPPING,&tInParam,NULL))
				{

				}
				m_bPlaying_status = 0;
		}
	}


#endif

    if (m_pCurrUrl)
    {
        free(m_pCurrUrl);
        m_pCurrUrl = NULL;
    }

    DeleteAppStream();
    if(m_pUserName != NULL)
    {
        free(m_pUserName);
        m_pUserName = NULL;
    }
    if(m_pPassword != NULL)
    {
        free(m_pPassword);
        m_pPassword = NULL;
    }
    if (m_pNonce != NULL)
    {
        free(m_pNonce);
        m_pNonce = NULL;
    }

    if (m_pSPS != NULL)
    {
        delete [] m_pSPS;
        m_pSPS = NULL;
    }
    if (m_pPPS != NULL)
    {
        delete [] m_pPPS;
        m_pPPS = NULL;
    }
    if (m_pVPS != NULL)
    {
        delete [] m_pVPS;
        m_pVPS = NULL;
    }

    if(m_pVPS_un != NULL)
	{
		free(m_pVPS_un);
		m_pVPS_un = NULL;
	}

    if (m_pDollarBuffer != NULL)
    {
        delete []m_pDollarBuffer;
        m_pDollarBuffer = NULL;
    }

    if(m_pRequire != NULL)
    {
        free(m_pRequire);
        m_pRequire = NULL;
    }

    if(m_pSsl != NULL)
    {
        SSL_free(m_pSsl);
        m_pSsl = NULL;
    }

    m_nSPSLen = 0;
    m_nPPSLen = 0;
	m_nVPSLen = 0;

    return 0;
}


void *CClientSocket::Thread()
{
    struct timeval tv_timeToDelay;
    unsigned int bTimeout = 0,dwTimeout = 0;
    unsigned dwSec,dwUSec;
    int nReadType = 0;
    int bAdpcm2G711u = 0;
    rtsp_stream_info nInfo;
    char *pBuffer = NULL;
    int dwBuffsize, nStreamType = 0;
    int bSleep = 0;

    ThreadStarted();

    m_bThreadFlag = 1;
#if !(defined(_WIN32) || defined(ON_ANDROID))
	syslog(LOG_NOTICE, "[%s:%d] pid=%d enter\n", __FUNCTION__, __LINE__, (unsigned int)syscall(SYS_gettid));
#endif
    while(!m_bThreadExit)
    {

        SendMsg();
        bSleep = 1;
#if 1
        if (m_bModeOpen && (m_bTCP == 1 || m_bTCP == 3/* || m_bTCP == 2 || m_bTransType == 0*/) && (m_bSetup & 3))
        {
            if (m_bTCP == 3 || m_bTCP == 2)
            {
                nStreamType = 2;
            }
            else
            {
                nStreamType = m_bRecording;
            }


            if (0 == m_pRtspServer->fRead(m_hRtpSess,pStreamUser,nStreamType,&pBuffer, &dwBuffsize, &nInfo, &nReadType, &bAdpcm2G711u))
            {
                if (nReadType == 1)
                {
                    //printf("[%s.%d]dwBuffsize = %d ch%d_%d[%d]\n",__FUNCTION__,__LINE__,dwBuffsize, m_nCh ,m_nStream,m_nClientSocket);
					 if (m_bAntsComb == 1)
					 {
						 m_pRtspServer->InputAntsCombData(m_hRtpSess, 0, nInfo.wCh + 1, nInfo.byStream, pBuffer, dwBuffsize);
					 }
					 else
					 {
						  m_pRtspServer->InputAntsData(m_hRtpSess, pBuffer, dwBuffsize, bAdpcm2G711u);
					 }
                    //printf("[%s.%d]dwBuffsize = %d ch%d_%d\n",__FUNCTION__,__LINE__,dwBuffsize, m_nCh , m_nStream);
                }
                else if (nReadType == 2)
                {
                    //m_pRtspServer->InputData();
                }

                m_pRtspServer->fRelease(m_hRtpSess,pStreamUser,nStreamType);
                bSleep = 0;
            }
		}


#endif
        if (IsSendUDP())
        {
            //printf("IsSendUDP!!!!!!!!!!!!!!!!!!!!\n");
            if (m_pRtpSess != NULL)
            {
                bSleep = m_pRtspServer->UDPData(m_pRtpSess, m_nSessionID);
            }
        }

        Fd_Set();
        tv_timeToDelay.tv_sec = 0;
        tv_timeToDelay.tv_usec = bSleep?10000:0;
        //SendResponse();
//printf("CClientSocket[%s.%d] m_nMaxNumSocket = %d [%d/%d] [%d]\n",__FUNCTION__,__LINE__, m_nMaxNumSocket, tv_timeToDelay.tv_sec, tv_timeToDelay.tv_usec, m_nClientSocket);
        int selectResult = select(m_nMaxNumSocket, &m_readSet,NULL/* &m_writeSet*/,NULL/* &m_exceptionSet*/, &tv_timeToDelay);
//printf("CClientSocket[%s.%d] selectResult = %d [%d]\n",__FUNCTION__,__LINE__, selectResult, m_nClientSocket);

        if (selectResult < 0)
        {
            //错误
            RTSP_ERROR("[%s.%d]select  %d err[%d,%s]\n",__FUNCTION__,__LINE__,m_nMaxNumSocket,GetLastError(),strerror(GetLastError()));
            Poll();
            continue;
        }
        if (selectResult == 0)
        {
            //超时
            Poll();
            bTimeout = 1;
            dwTimeout = 0;
            continue;
        }

        dwTimeout = 10000 - tv_timeToDelay.tv_usec;
//printf("CClientSocket[%s.%d]----------------->\n",__FUNCTION__,__LINE__);
        selectHandler();
//printf("CClientSocket[%s.%d]<-----------------\n",__FUNCTION__,__LINE__);
    }

#if 0
    printf("[%s.%d] m_pRtspServer->GLock[%d][%d]!!!!\n",__FUNCTION__,__LINE__,m_nClientSocket,m_hRtpSess);
    m_pRtspServer->GLock();
	printf("[%s.%d] m_pRtspServer->Lock[%d][%d]!!!!\n",__FUNCTION__,__LINE__,m_nClientSocket,m_hRtpSess);
    if (m_pRtpSess != NULL)
    {
		if (m_pRtpSess->GetCurSendSessionID() == m_nSessionID)
		{
		    printf("[%s.%d] m_pRtpSess->GetCurSendSessionID()[%d][%d]!!!!\n",__FUNCTION__,__LINE__,m_nClientSocket,m_hRtpSess);
			m_pRtpSess->SetCurSendSessionID(0);
		}

	}
	 m_pRtspServer->GUnlock();
    printf("[%s.%d] m_pRtspServer->GUnlock[%d][%d]!!!!\n",__FUNCTION__,__LINE__,m_nClientSocket,m_hRtpSess);
#endif

    SendMsg();
    m_bThreadFlag = 2;

    return NULL;
}

void CClientSocket::Fd_Set()
{
    int nSock = -1;

    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    FD_ZERO(&m_exceptionSet);

    nSock = GetSocket();
    if (nSock >= 0)
    {
        FD_SET(nSock,&m_readSet);
        FD_SET(nSock,&m_writeSet);
        FD_SET(nSock,&m_exceptionSet);
        if (nSock + 1 > m_nMaxNumSocket)
        {
            m_nMaxNumSocket = nSock + 1;
        }
    }

    if (m_pAudioSession != NULL)
    {
        nSock = m_pAudioSession->GetRTPSocket();
        if (nSock >= 0)
        {
            FD_SET(nSock,&m_readSet);
            if (nSock + 1 > m_nMaxNumSocket)
            {
                m_nMaxNumSocket = nSock + 1;
            }
        }

        nSock = m_pAudioSession->GetRTCPSocket();
        if (nSock >= 0)
        {
            FD_SET(nSock,&m_readSet);
            if (nSock + 1 > m_nMaxNumSocket)
            {
                m_nMaxNumSocket = nSock + 1;
            }


        }
    }

    if (m_pVideoSession != NULL)
    {
        nSock = m_pVideoSession->GetRTPSocket();
        if (nSock >= 0)
        {
            FD_SET(nSock,&m_readSet);
            if (nSock + 1 > m_nMaxNumSocket)
            {
                m_nMaxNumSocket = nSock + 1;
            }
        }

        nSock = m_pVideoSession->GetRTCPSocket();
        if (nSock >= 0)
        {
            FD_SET(nSock,&m_readSet);
            if (nSock + 1 > m_nMaxNumSocket)
            {
                m_nMaxNumSocket = nSock + 1;
            }
        }
    }
}


void CClientSocket::selectHandler()
{
    int nRet;
    int nSockTmp = -1;

    if (m_pAudioSession != NULL)
    {
        nSockTmp = m_pAudioSession->GetRTPSocket();
        if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
        {

            m_pAudioSession->RecvRTPPacket();

        }
        nSockTmp = m_pAudioSession->GetRTCPSocket();
        if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
        {

            m_pAudioSession->RecvRTCPPacket();

        }
    }

    if (m_pVideoSession != NULL)
    {
        nSockTmp = m_pVideoSession->GetRTPSocket();
        if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
        {

            m_pVideoSession->RecvRTPPacket();

        }
        nSockTmp = m_pVideoSession->GetRTCPSocket();
        if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
        {

            m_pVideoSession->RecvRTCPPacket();

        }
    }

    if (m_nClientSocket > 0)
    {
        if (FD_ISSET(m_nClientSocket, &m_readSet))
        {
        	m_hWriteLock.Lock();
            nRet = incomingRequestHandler();
		    m_hWriteLock.Unlock();
            if (nRet < 0)
            {
                //RTSP_DEBUG("Requesthandle faild .... close %d\n",nSocket);
                //m_pRtspServer->RemoveClientSocket(m_nClientSocket);
                SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
                //printf("m_nClientSocket = %d %d\n", m_nClientSocket, m_bNeedClose);
                return;
            }
        }
    }
#if 0
    if (FD_ISSET(nSocket,&m_writeSet))
    {
        pCurr->SendResponse();

    }
    if (FD_ISSET(nSocket,&m_exceptionSet))
    {
        RemoveClientSocket(nSocket);
        continue;
    }
#endif

}


int CClientSocket::incomingRequestHandler()
{

    int ReadBytes;
    int ret ;
    ReadBytes = ReadAndParse();
    if (ReadBytes < 0)
    {
        return -1;
    }
    ret = MessageHandle();
    if (ret < 0)
    {
        return -1;
    }

    // 查找客户端
    Poll();

    return 0;
}


int CClientSocket::CheckRecvHandler()
{
    int ReadBytes;
    int ret ;
    ReadBytes = ReadAndParse();
    if (ReadBytes < 0)
    {
        return -1;
    }
    ret = MessageHandle();
    if (ret < 0)
    {
        return -1;
    }

    // 查找客户端
    //Poll();

    return 0;
}


int CClientSocket::IsOwe(unsigned int uiIPv4[4],int nPort)
{
    if (0 != memcmp(uiIPv4,m_dwIPAddress,sizeof(m_dwIPAddress)) || nPort != m_uPort)
    {
        //
        return 0;
    }

    return 1;
}

unsigned int CClientSocket::GetLocalIP(char *szString,int size)
{

	struct sockaddr_in addr;
	int nRet;
	socklen_t addrlen;
	if (m_nClientSocket < 0)
	{

		return 0;
	}
	addrlen = sizeof(addr);
	nRet = getsockname(m_nClientSocket,(struct sockaddr *)&addr,&addrlen);
	if (nRet)
	{
		return 0;
	}
	if (szString)
	{
		inet_ntop(AF_INET,&addr.sin_addr,szString,size);
	}
	return addr.sin_addr.s_addr;
}

int CClientSocket::GetLocalIPv6(unsigned char *byIPv6/*[16]*/,char *szString,int size)
{
	struct sockaddr_in6 addr;

	socklen_t addrlen;
	int nRet;
	addrlen = sizeof(addr);
	if (!m_bIPv6)
	{
		unsigned int dwIp = GetLocalIP(szString,size);
		if (dwIp == 0 || dwIp == -1)
		{
			return -1;
		}
		if(byIPv6)
		memcpy(byIPv6,&dwIp,4);
		return 0;
	}

	if (m_nClientSocket < 0)
	{

		return 0;
	}
	nRet = getsockname(m_nClientSocket,(struct sockaddr *)&addr,&addrlen);
	if (nRet)
	{

		return -1;
	}
	if (szString)
	{
		inet_ntop(AF_INET6,&addr.sin6_addr,szString,size);
	}
	if(byIPv6!= NULL)
	{
		 memcpy(byIPv6,&addr.sin6_addr,16);
	}

	return 0;
}
unsigned int CClientSocket::GetIP(char *szString,int size)
{
    struct sockaddr_in addr;

    socklen_t addrlen;
    int nRet;
    addrlen = sizeof(addr);

    if (m_nClientSocket < 0)
    {

        return 0;
    }
    nRet = getpeername(m_nClientSocket,(struct sockaddr *)&addr,&addrlen);
    if (nRet)
    {

        return 0;
    }
	if (szString)
	{
		inet_ntop(AF_INET,&addr.sin_addr,szString,size);
	}
    return addr.sin_addr.s_addr;
}

int CClientSocket::GetIPv6(unsigned char *byIPv6/*[16]*/,char *szString,int size)
{
	struct sockaddr_in6 addr;

	socklen_t addrlen;
	int nRet;
	addrlen = sizeof(addr);
	if (!m_bIPv6)
	{
		unsigned int dwIp = GetIP(szString);
		if (dwIp == 0 || dwIp == -1)
		{
			return -1;
		}
		if (byIPv6 != NULL)
		{
			memcpy(byIPv6,&dwIp,4);
		}

		return 0;
	}

	if (m_nClientSocket < 0)
	{

		return 0;
	}
	nRet = getpeername(m_nClientSocket,(struct sockaddr *)&addr,&addrlen);
	if (nRet)
	{

		return -1;
	}
	if (byIPv6)
	{
		memcpy(byIPv6,&addr.sin6_addr,16);
	}
	if (szString)
	{
		inet_ntop(AF_INET6,&addr.sin6_addr,szString,size);
	}

	return 0;
}
unsigned int CClientSocket::GetPort()
{
    struct sockaddr_in addr;
    int nRet;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    if (m_nClientSocket < 0)
    {

        return 0;
    }
    nRet = getpeername(m_nClientSocket,(struct sockaddr *)&addr,&addrlen);
    if (nRet)
    {

        return 0;
    }

    return addr.sin_port;
}

int CClientSocket::CheckAlive(uint32_t Currtime)
{
    uint32_t nTimeOut = 0;

    //printf("m_bNeedClose = %d m_nClientSocket = %d!!\n", m_bNeedClose, m_nClientSocket);
    if (m_bNeedClose == 1)
    {
        return m_bNeedClose;
    }

    if (m_pRtspServer != NULL)
    {
        nTimeOut = m_pRtspServer->GetStreamTimeOut(m_hRtpSess);
    }
    if (nTimeOut == 0)
    {
        return 0;
    }
    if (m_tLastRefreshTime == 0)
    {
        //time(&m_tLastRefreshTime);
        Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
    }
    if (Currtime == 0)
    {
        //time(&Currtime);
        Ants_rtsp_GetSysRunTime(&Currtime,NULL);
    }
	//printf("m_tLastRefreshTime = %d nTimeOut = %d Currtime = %d!!\n", m_tLastRefreshTime, nTimeOut, Currtime);
    if (m_tLastRefreshTime + nTimeOut + 10 <= Currtime)
    {
		SetNeedClose(ANTS_RTSPSERVER_ERROR_NoAnyData);
    }
    return m_bNeedClose;
}
CMessage *CClientSocket::GetMessage()
{
    CMessage *p;
    if (m_pMsg == NULL)
    {
        return NULL;
    }
    p = m_pMsg;
    m_pMsg = m_pMsg->m_pNext;
    if (m_pMsg_tail == p)
    {
        m_pMsg_tail = m_pMsg_tail->m_pNext;
    }
    p->m_pNext = NULL;
    return p;

}
int CClientSocket::ReadAndParse()
{
    int Readbytes;
    char *pData;
    int nDataSize = 0,nLastPos = 0,nLastReadPos = 0;
    unsigned long len = 0;

    if (m_nClientSocket < 0 || m_bNeedClose == 1)
    {

        return -1;
    }
RTSP_DEBUG("\n!!!!!!!!!!!!!!!!!!!!m_nReadLen = %d,pos=%d ,len = %d nLastPos = %d\n",m_nReadLen,m_nReadPos,m_nReadLen - m_nReadPos,nLastPos);
    if (m_nReadLen == RTSP_BUFFER_RECV_SIZE)
    {
		if (m_nReadPos == 0)
		{
			//表示上一次,无法处理,全丢.
			RTSP_ERROR("\n!!!!!!!!!!!!!!!!!!!!m_nReadLen = %d pos = %d\n",m_nReadLen,m_nReadPos);
			SetNeedClose(ANTS_RTSPSERVER_ERROR_BufferTooSmall);
			m_nReadLen = 0;
			m_nReadPos = 0;
			return -1;
		}
		if (m_nReadLen > m_nReadPos)
		{
			memmove(m_pRequestBuffer,m_pRequestBuffer + m_nReadPos,m_nReadLen - m_nReadPos);
			m_nReadLen = m_nReadLen - m_nReadPos;
			m_nReadPos = 0;
		}
		else
		{
			m_nReadLen = 0;
			m_nReadPos = 0;
		}



    }

    //ioctlsocket(m_nClientSocket,FIONREAD,&len);
   // printf("len = %d \n",len);
	nLastReadPos = m_nReadLen;
        //Readbytes = recv(m_nClientSocket,(char *)m_pRequestBuffer + m_nReadLen,RTSP_BUFFER_RECV_SIZE - m_nReadLen,0);
        Readbytes = SSL_read(m_pSsl,(char *)m_pRequestBuffer + m_nReadLen,RTSP_BUFFER_RECV_SIZE - m_nReadLen);
#if 1
if(m_bTransType == 4){
    /* Readbytes 需要是4的整数倍 但实际却不一样能满足 试一下多次读取
    odt 测试工具中发过来的有换行符，需要去除后再来判断实际大小  如果实际大小刚好是4的倍数 则测试可以通过 否则会出现意外
    */

   int tempSize = 0;
   char *tempBuf = (char *)m_pRequestBuffer + m_nReadLen;
   for(int i = 0;i<Readbytes;i++)
   {
        if(tempBuf[i] == '\n' || tempBuf[i] == '\t')
        {

        } else{
            tempSize++;
        }
   }

    int retryCount = 0;
    while(tempSize % 4 != 0 && retryCount > 10){
        int temp = tempSize % 4;
       //int recvSize = recv(m_nClientSocket,(char *)m_pRequestBuffer + m_nReadLen+Readbytes,4-temp,0);
       int recvSize = SSL_read(m_pSsl,(char *)m_pRequestBuffer + m_nReadLen+Readbytes,4-temp);
       if(recvSize > 0){
            RTSP_DEBUG("+++++++++++++++Readbytes:%d m_nReadLen=%d extraRecv:%d  retryCount=%d\n",Readbytes,m_nReadLen,recvSize,retryCount);
            Readbytes += recvSize;
            tempSize += recvSize;
       }else{
            struct timespec req,rem;

            req.tv_sec = 0;
            req.tv_nsec = 100000;
            nanosleep(&req,&rem);
       }

       retryCount++;

    }

    unsigned int ResultSize = 0;
    unsigned char * pExtra = (unsigned char *)Ants_Rtsp_Base64Decode((char *)m_pRequestBuffer + m_nReadLen,Readbytes,&ResultSize);
     RTSP_DEBUG("**************decode Readbytes:%d ResultSize:%d\n",Readbytes,ResultSize);
     /*测试这里可以得到TEARDOWN  但到后面有概率被截断*/
    if(Readbytes >= 490 && Readbytes <= 496){
        pExtra[ResultSize] = 0;
        printf("%s\n",pExtra);
    }
    Readbytes = ResultSize;
    // = pExtra;
    memcpy((char *)m_pRequestBuffer + m_nReadLen,pExtra,Readbytes);
    free(pExtra);


}
#endif




        if (Readbytes == 0)
        {
            RTSP_DEBUG("[%s.%d]errno = %d[%s] sock=%d\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()),m_nClientSocket);
			SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketClose);
            return -1;
        }
        else if (Readbytes < 0)
        {
            int errorNo = GetLastError();

            if (errorNo == EWOULDBLOCK ||
                errorNo == EINTR||
                errorNo == EAGAIN ||
                errorNo == ETIMEDOUT)
            {
                //printf("!!errno = %d\n",errorNo);
               // break;
                return 0;
            }
            else
            {
                RTSP_DEBUG("[%s.%d]errno = %d %s\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo));
				SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
                return -1;
            }
        }
        m_nReadLen += Readbytes;
        m_pRequestBuffer[m_nReadLen] = 0;
        //printf(" [%s]\n",m_pRequestBuffer);
        if (m_nReadLen >= RTSP_BUFFER_RECV_SIZE)
        {
           // break;
        }
#if 0
		RTSP_DEBUG("[%s.%d] dollar = %d {\n\n",__FUNCTION__,__LINE__,m_bDollar);
		int x = 0;
		for (int n = 0;n< m_nReadLen - m_nReadPos;n++)
		{
			printf(" %02X",m_pRequestBuffer[m_nReadPos + n]);
			x++;
			if (x == 16)
			{
				x=0;
				printf("\n");
			}
		}

		RTSP_DEBUG("\n\n}\n",__FUNCTION__,__LINE__);

#endif



    if (m_nReadLen <= 0)
    {
        //RTSP_DEBUG("[%s.%d]m_nReadLen = %d\n",__FUNCTION__,__LINE__,m_nReadLen);
        return 0;
    }
    m_pRequestBuffer[m_nReadLen] = 0;
    //printf("Readbytes = %d m_nReadPos = %d nLastPos = %d nLastReadPos = %d<%s>\n",Readbytes,m_nReadPos,nLastPos,nLastReadPos,(char *)m_pRequestBuffer + nLastReadPos);
    pData = (char *)m_pRequestBuffer + m_nReadPos;
    nDataSize = m_nReadLen - m_nReadPos;

    if (pData[0] == '$' && nDataSize == 1)
    {
		RTSP_DEBUG("[%s.%d]$ new\n",__FUNCTION__,__LINE__);
        m_bDollar = 1;
        m_nDollarLen = 0;
        m_nDollarPos = 1;
        m_pDollarBuffer = NULL;
        m_pDollarBuffer0[0] = '$';

		m_nReadPos += nDataSize;

        nDataSize = 0;
    }

    if (m_bDollar)
    {
        CMessage *pMsg;
        if (m_pDollarBuffer != NULL)
        {
            if (m_nDollarLen > m_nDollarPos + nDataSize)
            {
                memcpy(m_pDollarBuffer + m_nDollarPos,m_pRequestBuffer,nDataSize);
                m_nDollarPos += nDataSize;
				m_nReadPos += nDataSize;
                nDataSize = 0;
            }
            else
            {
                memcpy(m_pDollarBuffer + m_nDollarPos,pData,m_nDollarLen - m_nDollarPos);
                pData += m_nDollarLen - m_nDollarPos;
                nDataSize -= m_nDollarLen - m_nDollarPos;
				m_nReadPos += m_nDollarLen - m_nDollarPos;
                pMsg = new CMessage;
                if (pMsg != NULL)
                {
                    pMsg->m_nMsgLen = m_nDollarLen;
                    pMsg->m_pMessage = m_pDollarBuffer;
                    pMsg->m_bAllocFlag = 1;
                    pMsg->m_nDollar_ch = m_pDollarBuffer[1];


                    pMsg->m_nLineCnt = 1;
                    pMsg->m_nSegCnt[0] = 1;
                    pMsg->m_nPos[0][0] = 0;
                    pMsg->m_nLen[0][0] =  m_nDollarLen;
                    pMsg->m_bDollar = 1;



                    if (m_pMsg_tail == NULL)
                    {
                        m_pMsg = m_pMsg_tail = pMsg;
                    }
                    else
                    {
                        m_pMsg_tail->m_pNext = pMsg;
                        m_pMsg_tail = pMsg;
                    }




                }
                m_nDollarPos = 0;
                m_nDollarLen = 0;
                m_bDollar = 0;
                m_pDollarBuffer = NULL;

            }
        }
        else
        {
            //
            if (m_nDollarPos >= 4)
            {
                //说明内存分配失败
                if (m_nDollarLen > m_nDollarPos + nDataSize)
                {
                    //memcpy(m_pDollarBuffer + m_nDollarPos,pData,nDataSize);
                    m_nDollarPos += nDataSize;
					m_nReadPos += nDataSize;
                    nDataSize = 0;
                }
                else
                {
                    //memcpy(m_pDollarBuffer + m_nDollarPos,pData,m_nDollarLen - m_nDollarPos);
                    pData += m_nDollarLen - m_nDollarPos;
                    nDataSize -= m_nDollarLen - m_nDollarPos;
					m_nReadPos += m_nDollarLen - m_nDollarPos;
                    m_nDollarPos = m_nDollarLen;

                    m_nDollarPos = 0;
                    m_nDollarLen = 0;
                    m_bDollar = 0;
                    m_pDollarBuffer = NULL;

                }
            }
            else
            {
                //说明长度尚未获取
                if (m_nDollarPos + nDataSize >= 4)
                {
                    //可以获取长度
                    int Len;
                    memcpy(m_pDollarBuffer0 + m_nDollarPos,pData,4 - m_nDollarPos);
                    pData += 4 - m_nDollarPos;
                    nDataSize -= 4 - m_nDollarPos;
					m_nReadPos += 4 - m_nDollarPos;
                    m_nDollarPos = 4;
                    Len = htons(*((short *)&m_pDollarBuffer0[2]));
                    m_nDollarLen = Len + 4;
                    m_pDollarBuffer = new char [m_nDollarLen];
                    if (m_pDollarBuffer != NULL)
                    {
                        memcpy(m_pDollarBuffer,m_pDollarBuffer0,4);
                    }

                    if (m_nDollarLen > m_nDollarPos + nDataSize)
                    {
                        if (m_pDollarBuffer != NULL)
                        {
                            memcpy(m_pDollarBuffer + m_nDollarPos,pData,nDataSize);
                        }
                        m_nDollarPos += nDataSize;
						m_nReadPos += nDataSize;
                        nDataSize = 0;
                    }
                    else
                    {
                        if (m_pDollarBuffer != NULL)
                        {
                            memcpy(m_pDollarBuffer + m_nDollarPos,pData,m_nDollarLen - m_nDollarPos);
                        }

                        pData += m_nDollarLen - m_nDollarPos;
                        nDataSize -= m_nDollarLen - m_nDollarPos;
						m_nReadPos += m_nDollarLen - m_nDollarPos;
                        m_nDollarPos = m_nDollarLen;
                        if (m_pDollarBuffer != NULL)
                        {
                            pMsg = new CMessage;
                            if (pMsg != NULL)
                            {
                                pMsg->m_nMsgLen = m_nDollarLen;
                                pMsg->m_pMessage = m_pDollarBuffer;
                                pMsg->m_bAllocFlag = 1;
                                pMsg->m_nDollar_ch = m_pDollarBuffer0[1];

                                pMsg->m_nLineCnt = 1;
                                pMsg->m_nSegCnt[0] = 1;
                                pMsg->m_nPos[0][0] = 0;
                                pMsg->m_nLen[0][0] =  m_nDollarLen;
                                pMsg->m_bDollar = 1;

                                if (m_pMsg_tail == NULL)
                                {
                                    m_pMsg = m_pMsg_tail = pMsg;
                                }
                                else
                                {
                                    m_pMsg_tail->m_pNext = pMsg;
                                    m_pMsg_tail = pMsg;
                                }

                            }

                        }

                        m_nDollarPos = 0;
                        m_nDollarLen = 0;
                        m_bDollar = 0;
                        m_pDollarBuffer = NULL;

                    }
                }
                else
                {
                    //仍然不能获取


                    memcpy(m_pDollarBuffer0 + m_nDollarPos,pData,nDataSize);
                    m_nDollarPos += nDataSize;
					m_nReadPos += nDataSize;
					nDataSize = 0;

                }
            }
        }
    }
    else
    {
        if (m_pDollarBuffer != NULL)
        {
            delete []m_pDollarBuffer;
            m_pDollarBuffer = NULL;
        }

    }



    //RTSP_DEBUG("\nC->:\n%s",(char *)m_pRequestBuffer);
    //RTSP_DEBUG("[%d]C->:%d\n%s",m_nClientSocket,m_nReadLen,m_pRequestBuffer);
    nLastPos = Parse((char *)pData,nDataSize);
	m_nReadPos += nLastPos;
	if(m_nReadPos >= m_nReadLen)
	{
		m_nReadLen = 0;
		m_nReadPos = 0;
	}


    //消息头
    //消息体




    return 0;
}


int CClientSocket::GetMulticastIPv4(unsigned int *uiIPv4,int *nPort,int *nTTL,int bAudio,int *bIPv6)
{
    unsigned int CurrIPv4;
    int CurrPort;
    int CurrTTL;

    CurrPort = m_nMulticastPort[bAudio != 0];
    CurrTTL = m_nTTL[bAudio != 0];
	if(m_bIPv6)
	{

	}
	else
	{
		 CurrIPv4 = m_dwIPv4Multicast[bAudio != 0][0];
		if(htonl(CurrIPv4) < 0xE0000000 || htonl(CurrIPv4) > 0xEFFFFFFF)
			return -1;
	}
    if(CurrTTL < 1)CurrTTL = 255;
    if(CurrTTL > 255)CurrTTL = 255;
    if (uiIPv4)
    {
		if (m_bIPv6)
		{
			memcpy(uiIPv4,m_dwIPv4Multicast[bAudio != 0],16);
		}
		else
		{
			*uiIPv4 = CurrIPv4;
		}
    }
    if (nPort)
    {
        *nPort = CurrPort;
    }
    if (nTTL)
    {
        *nTTL = CurrTTL;
    }
	if (bIPv6)
	{
		*bIPv6 = m_bIPv6;
	}
    return 0;
}


int CClientSocket::SetMulticastIPv4(unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio)
{
    int nRet;
    int nIdx = bAudio != 0;
    //224.0.0.0到239.255.255.255
    if(nPort & 1)
        return -1;
#if 0
    if (htonl(uiIPv4) < 0xE0000000 || htonl(uiIPv4) > 0xEFFFFFFF)
    {
        return -1;
    }
#endif
    nPort = nPort &0xFFFFFFFE;
    if (0 != memcmp(m_dwIPv4Multicast[nIdx], uiIPv4,16) || nPort != m_nMulticastPort[nIdx])
    {
        //更改地址
        //   DeleteDestination(m_dwIPv4Multicast[nIdx],m_nMulticastPort[nIdx],bAudio);
        m_nMulticastCnt[nIdx] = 0;
        memcpy(m_dwIPv4Multicast[nIdx],uiIPv4,16) ;
        m_nMulticastPort[nIdx] = nPort ;
    }
    //DeleteDestination(m_tIPv4Multicast[bAudio != 0],bAudio);
    //m_tIPv4Multicast[bAudio != 0].SetIP(ip);
    //m_tIPv4Multicast[bAudio != 0].SetPort(addr.GetPort());
    if (nTTL <= 0)
    {
        nTTL = 1;
    }
    if (nTTL > 255)
    {
        nTTL = 255;
    }
    m_nTTL[nIdx] = nTTL;
    if (nIdx == 0)
    {
        if (m_pRtpSess != NULL)
        {
            if (m_pRtpSess->GetVideo() != NULL)
            {
                m_pRtpSess->GetVideo()->SetMulticastTTL(nTTL);
            }
        }
    }
    else
    {
        if (m_pRtpSess != NULL)
        {
            if (m_pRtpSess->GetAudio() != NULL)
            {
                m_pRtpSess->GetAudio()->SetMulticastTTL(nTTL);
            }
        }
    }

    return 0;
}


int CClientSocket::SetSPS(char *pSPSData,int nSPSLen)
{
   // RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
    if (pSPSData == NULL || nSPSLen == 0)
    {
        if (m_pSPS != NULL)
        {
            delete []m_pSPS;
            m_pSPS = NULL;
            m_nSPSLen = 0;
        }

      // RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
        return NULL;
    }
   //RTSP_DEBUG("[%s.%d] this = %x\n",__FUNCTION__,__LINE__,this);
    if (m_pSPS != NULL)
    {
         if (nSPSLen + 4 != m_nSPSLen || 0 != memcmp(m_pSPS + 4,pSPSData,nSPSLen))
         {
             delete m_pSPS;
             m_pSPS = NULL;
             m_nSPSLen = 0;
            //RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
         }

    }
   // RTSP_DEBUG("[%s.%d] m_pSPS = %x\n",__FUNCTION__,__LINE__,m_pSPS);
    if (m_pSPS == NULL)
    {
        m_nSPSLen = 0;
        m_pSPS = new char [nSPSLen + 4];
        if (m_pSPS == NULL)
        {
         // RTSP_DEBUG("[%s.%d] \n",__FUNCTION__,__LINE__);
            return -1;
        }
        m_pSPS[0] = 0x00;
        m_pSPS[1] = 0x00;
        m_pSPS[2] = 0x00;
        m_pSPS[3] = 0x01;
        memcpy(m_pSPS + 4,pSPSData,nSPSLen);
        m_nSPSLen = nSPSLen + 4;
      //RTSP_DEBUG("[%s.%d] [%x]m_pSPS = %x m_nSPSLen = %d\n",__FUNCTION__,__LINE__,this,m_pSPS,m_nSPSLen);
    }

    return 0;
}

int CClientSocket::SetPPS(char *pPPSData,int nPPSLen)
{
    if (pPPSData == NULL || nPPSLen == 0)
    {
        if (m_pPPS != NULL)
        {
            delete []m_pPPS;
            m_pPPS = NULL;
            m_nPPSLen = 0;
        }
        return NULL;
    }

    if (m_pPPS != NULL)
    {
        if (nPPSLen + 4 != m_nPPSLen || 0 != memcmp(m_pPPS + 4,pPPSData,nPPSLen))
        {
            delete []m_pPPS;
            m_pPPS = NULL;
            m_nPPSLen = 0;
        }

    }
    if (m_pPPS == NULL)
    {
        m_nPPSLen = 0;
        m_pPPS = new char [nPPSLen + 4];
        if (m_pPPS == NULL)
        {
            return -1;
        }
        m_pPPS[0] = 0x00;
        m_pPPS[1] = 0x00;
        m_pPPS[2] = 0x00;
        m_pPPS[3] = 0x01;
        memcpy(m_pPPS + 4,pPPSData,nPPSLen);
        m_nPPSLen = nPPSLen + 4;
    }

    return 0;
}

int CClientSocket::SetVPS(char *pVPSData,int nVPSLen)
{
    if (pVPSData == NULL || nVPSLen == 0)
    {
        if (m_pVPS != NULL)
        {
            delete []m_pVPS;
            m_pVPS = NULL;
            m_nVPSLen = 0;
        }
        return NULL;
    }

    if (m_pVPS != NULL)
    {
        if (nVPSLen + 4 != m_nVPSLen || 0 != memcmp(m_pVPS + 4,pVPSData,nVPSLen))
        {
            delete []m_pVPS;
            m_pVPS = NULL;
            m_nVPSLen = 0;
        }

    }
    if (m_pVPS == NULL)
    {
        m_nVPSLen = 0;
        m_pVPS = new char [nVPSLen + 4];
        if (m_pVPS == NULL)
        {
            return -1;
        }
        m_pVPS[0] = 0x00;
        m_pVPS[1] = 0x00;
        m_pVPS[2] = 0x00;
        m_pVPS[3] = 0x01;
        memcpy(m_pVPS + 4,pVPSData,nVPSLen);
        m_nVPSLen = nVPSLen + 4;
		if(m_pVPS_un != NULL)
		{
			free(m_pVPS_un);
			m_pVPS_un = NULL;
		}
		m_nVPS_un_Len = 0;
		m_pVPS_un = new char [nVPSLen];
		if(m_pVPS_un != NULL)
		{
			int i;
			for(i = 0;i < nVPSLen;i++)
			{
				if(nVPSLen - i > 2 && pVPSData[i] == 0 && pVPSData[i + 1] == 0 && pVPSData[i + 2] == 3)
				{
					m_pVPS_un[m_nVPS_un_Len++] = pVPSData[i++];
					m_pVPS_un[m_nVPS_un_Len++] = pVPSData[i++];
				}
				else
				{
					m_pVPS_un[m_nVPS_un_Len++] = pVPSData[i];
				}
			}

		}
    }

    return 0;
}

char * CClientSocket::GetPPS_Base64()
{
    char *pReturn = NULL;

    if (m_pPPS == NULL || m_nPPSLen < 4)
    {
        return NULL;
    }

    pReturn =  Ants_Rtsp_Base64Encode(m_pPPS+4,m_nPPSLen -4);

    return pReturn;
}

char * CClientSocket::GetSPS_Base64()
{
    char *pReturn = NULL;
   // RTSP_DEBUG("[%s.%d] this = %x \n",__FUNCTION__,__LINE__,this);

    if (m_pSPS == NULL || m_nSPSLen < 4)
    {
     //   RTSP_DEBUG("[%s.%d]m_pSPS = %x m_nSPSLen = %d\n",__FUNCTION__,__LINE__,m_pSPS,m_nSPSLen);
        return NULL;
    }
    //RTSP_DEBUG("[%s.%d]m_pSPS = %x m_nSPSLen = %d\n",__FUNCTION__,__LINE__,m_pSPS,m_nSPSLen);
    pReturn = Ants_Rtsp_Base64Encode(m_pSPS+4,m_nSPSLen-4);

    return pReturn;
}

char * CClientSocket::GetVPS_Base64()
{
    char *pReturn = NULL;

    if (m_pVPS == NULL || m_nVPSLen < 4)
    {
        return NULL;
    }
    pReturn = Ants_Rtsp_Base64Encode(m_pVPS+4,m_nVPSLen-4);

    return pReturn;
}

int  CClientSocket::GetProfileLevelID()
{
    int nRet = -1,nValue;

    if (m_pVPS == NULL)
    {
        if (m_pSPS != NULL)
        {// 4 + 1,
            nValue = m_pSPS[5];
            nRet = nValue & 0xFF;
            nRet <<= 8;
            nValue = m_pSPS[6];
            nRet |= nValue & 0xFF;
            nRet <<= 8;
            nValue = m_pSPS[7];
            nRet |= nValue & 0xFF;
        }
    }

    return nRet;
}
int CClientSocket::H265_GetProfileId()
{
	int nRet = -1;
	unsigned int nValue;
	if(m_pVPS_un != NULL && m_nVPS_un_Len > 6)
	{
			nValue = ((unsigned char *)m_pVPS_un)[6];
			nRet = nValue & 0x1F;
	}
	return nRet;
}
int CClientSocket::H265_GetProfileSpace()
{
	int nRet = -1;
	unsigned int nValue;
	if(m_pVPS_un != NULL && m_nVPS_un_Len > 6)
	{// 4+ 2,
		nValue = ((unsigned char *)m_pVPS_un)[6];
		nRet = nValue >> 6;
	}
	return nRet;
}
int CClientSocket::H265_GetTierFlag()
{
	int nRet = -1;
	unsigned int nValue;
	if(m_pVPS_un != NULL && m_nVPS_un_Len > 6)
	{// 4+ 2,
		nValue = ((unsigned char *)m_pVPS_un)[6];
		nRet = (nValue >> 5) & 1;
	}
	return nRet;
}
int CClientSocket::H265_GetLevelId()
{
	int nRet = -1;
	unsigned int nValue;
	if(m_pVPS_un != NULL && m_nVPS_un_Len > 17)
	{// 4+ 2
		nValue = ((unsigned char *)m_pVPS_un)[17];
		nRet = nValue;
	}
	return nRet;
}
char* CClientSocket::H265_GetInteropConstraints()
{
	char *szRet = NULL;
	if(m_pVPS_un != NULL && m_nVPS_un_Len > 11 + 6)
	{// 4+ 2,
		unsigned char *pCons = (unsigned char *)&m_pVPS_un[11];
		sprintf(m_sInteropConstraints,"%02X%02X%02X%02X%02X%02X",pCons[0],pCons[1],pCons[2],pCons[3],pCons[4],pCons[5]);
		szRet = m_sInteropConstraints;
	}
	return szRet;
}

int CClientSocket::SetConfig(int nCmdType,void *pData,int nDataSize)
{
    if (nCmdType == 12)
    {
        // SPS
        SetSPS((char *)pData,nDataSize);
    }
    else if (nCmdType == 13)
    {
        // PPS
        SetPPS((char *)pData,nDataSize);
    }
    else if (nCmdType == 14)
    {
        // VPS
        SetVPS((char *)pData,nDataSize);
    }

    return 0;
}


int CClientSocket::SetStreamInfo(int bStreamType,int nVideoPayloadType,int nAudioPayloadType)
{
    m_bPlayloadType[0] = nVideoPayloadType;
    m_bPlayloadType[1] = nAudioPayloadType;
    if (nVideoPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_H264)
    {
        m_nPlayloadStreamType[0] = ANTS_RTSP_CALLBACKBYPE_STREAM_H264;
        sprintf(m_szPlayloadName[0],"H264");
    }
    else if (nVideoPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_H265)
    {
        m_nPlayloadStreamType[0] = ANTS_RTSP_CALLBACKBYPE_STREAM_H265;
        sprintf(m_szPlayloadName[0],"H265");
    }
    else if (nVideoPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG)
    {
        m_nPlayloadStreamType[0] = ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG;
        sprintf(m_szPlayloadName[0],"JPEG");
    }
    else if (nVideoPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB)
    {
        m_nPlayloadStreamType[0] = ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB;
        sprintf(m_szPlayloadName[0],"AntsComb");
    }
    if (nAudioPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G711A)
    {
        m_nPlayloadStreamType[1] = ANTS_RTSP_CALLBACKBYPE_STREAM_G711A;
        sprintf(m_szPlayloadName[1],"PCMA");
    }
    else if (nAudioPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G711U)
    {
        m_nPlayloadStreamType[1] = ANTS_RTSP_CALLBACKBYPE_STREAM_G711U;
        sprintf(m_szPlayloadName[1],"PCMU");
    }
    else if (nAudioPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_AAC)
    {
        m_nPlayloadStreamType[1] = ANTS_RTSP_CALLBACKBYPE_STREAM_AAC;
        sprintf(m_szPlayloadName[1],"mpeg4-generic");
    }

    m_StreamType = bStreamType;

    return 0;
}


int CClientSocket::SetNeedClose(int nReason)
{
	if (!m_bNeedClose)
	{
		m_bNeedClose = 1;
		m_nReason = nReason;
	}


    return 0;
}

int CClientSocket::AddAppStream(int nAppPayloadType,unsigned int dwClockRate,char *szAppName)
{
    int i,nFreeIdx = -1;
    char *pNewAppName;
    RTSP_APP_MGR_T *pMgr;
    int status = 0;
    if (szAppName == NULL)
    {
        return -1;
    }
    for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            if (m_pAppMgr[i]->byAppPayloadType == nAppPayloadType ||
                0 == strcmp(m_pAppMgr[i]->szAppName,szAppName ))
            {
                return -1;
            }
        }
        else if(nFreeIdx == -1)
        {
            nFreeIdx = i;

        }
    }
    if (nFreeIdx == -1)
    {
        return -1;
    }
    pMgr = new RTSP_APP_MGR_T;
    if (pMgr == NULL)
    {
        return -1;
    }
    memset(pMgr,0,sizeof(RTSP_APP_MGR_T));
    pNewAppName = strdup(szAppName);
    if (pNewAppName == NULL)
    {
        delete pMgr;
        return -1;
    }

    pMgr->byAppPayloadType = nAppPayloadType;
    pMgr->dwClockRate = dwClockRate;
    pMgr->szAppName = pNewAppName;
    m_pAppMgr[nFreeIdx] = pMgr;
    return 0;


}


int CClientSocket::DeleteAppStream()
{
    int i;
    RTSP_APP_MGR_T *pMgr = NULL;

    for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            pMgr = m_pAppMgr[i];
            m_pAppMgr[i] = NULL;
            if (pMgr->pSess != NULL)
            {
                pMgr->pSess->BYEDestroy(0,NULL,0);
                delete pMgr->pSess;
                pMgr->pSess = NULL;
            }

            if (pMgr->szAppName != NULL)
            {
                free(pMgr->szAppName);
                pMgr->szAppName = NULL;
            }
            delete pMgr;
        }
    }

    return 0;
}


int CClientSocket::GetAppIndexByPayloadType(int nAppPayloadType)
{
    for (int i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            if(m_pAppMgr[i]->byAppPayloadType == nAppPayloadType)
            {
                return i;
            }
        }
    }
    return -1;
}


int CClientSocket::Parse(char *pData,int nDataSize)
{
    //假定 N个完整消息
    int i;
    char c,c1;
    int nAndPos =-1;// 转义符 &


    CMessage tmpMsg,*pMsg;
    int crlf_cnt = 0;
    int Pos,Len,MsgPos,nLastDealPos = 0;

    if (nDataSize <= 0)
    {
        return 0;
    }
    RTSP_DEBUG("[%d]C->:%d\n%s",m_nClientSocket,nDataSize,pData);
    //请求命令

    crlf_cnt = 0;
    c = pData[0];
    Pos = 0;
    Len = 0;
    MsgPos = 0;

    int bq = 0;


    //////////////////////////////////////////////////////////////////////////
    //	memcpy(pData+nDataSize+1,pData,nDataSize/2);
    //	pData[nDataSize]=LF;
    //	nDataSize = nDataSize *3/2+1;
    //////////////////////////////////////////////////////////////////////////
    for (i = 1; i < nDataSize; i++)
    {

        c1 = pData[i];

        if (c == '$')
        {
			RTSP_DEBUG("[%s.%d]$ new\n",__FUNCTION__,__LINE__);
            //$[1Byte ChID][2bytes Length][data][without CRLF]
            int Len,CurrLen;
            if (nDataSize - i < 3)
            {
                //遇到错误
                m_bDollar = 1;
                m_nDollarLen = 0;
                m_nDollarPos = nDataSize - i + 1;
                m_pDollarBuffer = NULL;
                memcpy(m_pDollarBuffer0,&pData[i - 1],nDataSize - i + 1);
				nLastDealPos = nDataSize;

                return nLastDealPos;
            }
            Len = htons(*((short *)&pData[i + 1]));
            RTSP_DEBUG("[%s.%d]$ch = %d,len = %d ,sock = %d\n",__FUNCTION__,__LINE__,c1,Len,m_nClientSocket);



            CurrLen = Len;
            if (CurrLen > nDataSize - i - 3)
            {
                CurrLen = nDataSize - i - 3;
            }


            if (Len > nDataSize - i - 3)
            {
                //未完,遇到错误

                m_bDollar = 1;
                m_nDollarLen = Len + 4;
                m_nDollarPos = nDataSize - i + 1;
                m_pDollarBuffer = new char [m_nDollarLen];
                if (m_pDollarBuffer != NULL)
                {
                    memcpy(m_pDollarBuffer,&pData[i - 1],nDataSize - i + 1);
                }
				nLastDealPos = nDataSize;

                return nLastDealPos;
            }


            //
            pMsg = new CMessage;
            if (pMsg != NULL)
            {
                pMsg->m_nMsgLen = Len + 4;
                pMsg->m_pMessage = pData + i -1;


                pMsg->m_nLineCnt = 1;
                pMsg->m_nSegCnt[0] = 1;
                pMsg->m_nPos[0][0] = 0;
                pMsg->m_nLen[0][0] =  Len + 4;
                pMsg->m_bDollar = 1;



                if (m_pMsg_tail == NULL)
                {
                    m_pMsg = m_pMsg_tail = pMsg;
                }
                else
                {
                    m_pMsg_tail->m_pNext = pMsg;
                    m_pMsg_tail = pMsg;
                }




            }



            tmpMsg.Reset();



            i += Len + 4 - 1;
            c = pData[i];

			nLastDealPos = i;

            MsgPos = i;
            Pos = i;
            crlf_cnt = 0;

            continue;

        }
        else if (c == '&')
        {
            nAndPos = i;
            crlf_cnt = 0;
        }else if(c == '\"'){
            bq = !bq;
        }
		else if (c == ' ' || c == ';' || c == ':' || c == ',')
        {
            //当前消息,当前行,某字段结束
            int bCheck = 0,n,bSpace = 0;
            crlf_cnt = 0;

            if (c == ';' && nAndPos != -1)
            {
                // 检查转义
                if (i - nAndPos <= 5)
                {
                    for (n = 0; n < sizeof(sAndCheck)/sizeof(sAndCheck[0]); n++)
                    {
                        if(0 == strnicmp(pData + nAndPos,sAndCheck[n],strlen(sAndCheck[n])))
                        {
                            bCheck = 1;
                            if (n == 0)
                            {
                                bSpace = 1;
                            }
                            break;
                        }
                    }

                }

            }
            if (bCheck)
            {
                if (bSpace)
                {
                    // &nbsp;
                    if (MsgPos == nAndPos - 1)
                    {
                        MsgPos = i ;
                    }
                    if (Pos >= nAndPos - 1)
                    {
                        Pos = i ;
                    }

                    if (MsgPos == Pos )
                    {
                    }
                    else
                    {
                        tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,nAndPos-1-Pos);
                    }
                    Pos = i;//从非空格开始
                }
            }
			else if(c == ':')
			{
				if(c1 != ' ' && tmpMsg.m_nLineCnt>0)
				{
					tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,i-Pos);
					Pos = i;//从非空格开始
				}

			}
            else if((!bq) && c == ',')
			{
				if(c1 != ' ' && tmpMsg.m_nLineCnt>0)
				{
					tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,i-Pos);
					Pos = i;//从非空格开始
				}

			}
            else
            {


                if (MsgPos == i - 1)
                {
                    MsgPos = i;
                }
                if (Pos == i - 1)
                {
                    //去掉多个空格


                }
                else
                {
                    tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,i-1-Pos);
                }
                Pos = i;//从非空格开始
            }

        }
        else if (c == CR)
        {
            if(c1 == LF)
            {
                crlf_cnt++;
                if (crlf_cnt > 1)
                {
                    //消息结束,开始检查下一条消息

                    tmpMsg.m_nMsgLen = i - MsgPos + 1;
                    tmpMsg.m_pMessage = (char *)pData + MsgPos;
                    if (tmpMsg.m_bHasContext)
                    {
                        //
						if (tmpMsg.m_nContextLength > nDataSize - i - 1)
						{
							return -1;
						}

                        tmpMsg.m_pContext = (uint8_t *)pData + i + 1;
						if (tmpMsg.m_nContextLength < 0)
						{
							tmpMsg.m_nContextLength = nDataSize - i - 1;
						}
                       // tmpMsg.m_nContextLength = nDataSize - i - 1;
                       // i = nDataSize;
						i = i + 1 + tmpMsg.m_nContextLength;

                    }


                    pMsg = new CMessage;
                    if (pMsg != NULL)
                    {

                        if (!pMsg->CopyMessage(tmpMsg))
                        {

                            if (m_pMsg_tail == NULL)
                            {
                                m_pMsg = m_pMsg_tail = pMsg;
                            }
                            else
                            {
                                m_pMsg_tail->m_pNext = pMsg;
                                m_pMsg_tail = pMsg;
                            }


                        }
                        else
                        {
                            delete pMsg;
                        }
                    }
                    tmpMsg.Reset();

                    MsgPos = i;
					nLastDealPos = i;

                }
                else
                {
                    //当前消息,某一行结束,开始检查下一行
                    if (tmpMsg.m_nLineCnt < MSG_MAX_LINENUMS)
                    {
                        tmpMsg.IncSeg((uint8_t *)pData+Pos,Pos - MsgPos,i-1-Pos);
                        tmpMsg.IncLine();

                    }



                }

                Pos = i;


            }
            else
            {
                //格式不对,忽略
                if (c == CR || c == LF || c == ' ' || c == 0)
                {
                    if (MsgPos == i - 1)
                    {
                        MsgPos = i;
                    }
                    if (Pos == i - 1)
                    {
                        Pos = i;
                    }
                }
            }

        }
        else
        {
            if (crlf_cnt && c == LF)
            {
            }
            else
            {
                crlf_cnt = 0;
            }
            if (c == CR || c == LF || c == ' ' || c == 0)
            {
                if (MsgPos == i - 1)
                {
                    MsgPos = i;
                }
                if (Pos == i - 1)
                {
                    Pos = i;
                }
            }

        }

        c = c1;


    }

    if (MsgPos == 0)
    {
		if (m_pOutPutSocket != NULL)
		{

			tmpMsg.m_bHasContext = 1;
			tmpMsg.m_pContext = (uint8_t *)pData;
			tmpMsg.m_nContextLength = nDataSize;
			nLastDealPos = nDataSize;

			pMsg = new CMessage;
			if (pMsg != NULL)
			{

				if (!pMsg->CopyMessage(tmpMsg))
				{

					if (m_pMsg_tail == NULL)
					{
						m_pMsg = m_pMsg_tail = pMsg;
					}
					else
					{
						m_pMsg_tail->m_pNext = pMsg;
						m_pMsg_tail = pMsg;
					}


				}
				else
				{
					delete pMsg;
				}
			}
		}
    }

    return nLastDealPos;

}
void CClientSocket::PushRTPorRTCP(int ch,void *pData,int len)
{
    if (pData == NULL)
    {
        return ;
    }
    if (ch == m_nPort_VA[2] || ch == m_nPort_VA[2] + 1)
    {
        if (m_bTransType == 3 ||
            m_bTransType == 4)
        {
            //tcp

            if ((m_bSetup & 4) && m_pVideoSession)
            {
                if (m_pVideoSession != NULL)
                {
                    if(-1 == m_pVideoSession->DealRecvPacket((unsigned char *)pData,len))
                    {
                    }
                }



            }
        }
    }
    else if (ch == m_nPort_VA[3] || ch == m_nPort_VA[3] + 1)
    {
        if (m_bTransType == 3 ||
            m_bTransType == 4)
        {
            //tcp

            if ((m_bSetup & 8) && m_pAudioSession)
            {
                if(m_pAudioSession != NULL)
                {
                    m_pAudioSession->DealRecvPacket((unsigned char *)pData,len);
                }


            }

        }
    }





}
int CClientSocket::SendRTP_RTCPData(int nCh,const void *pData,size_t len)
{

    DOLLAR_HEAER_T dollar;
    int nRet;
    int nHeadLen = 0;
    char *pHead = NULL,*pDataNew = (char *)pData;
    int nDataLen = len;
    uint16_t wLen;
    int bNeedHead = 1;
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
	if (m_pOutPutSocket != NULL)
	{
		bOutPut = 1;
		if (!m_pOutPutSocket->m_bNeedClose)
		{
			pOutPut = m_pOutPutSocket;
			pOutPut->SetUsed(1);
		}
	}
	m_pRtspServer->GUnlock();

	if (pOutPut != NULL)
	{
		nRet = pOutPut->SendRTP_RTCPData(nCh,pData,len);
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
		return nRet;
	}
	if (bOutPut)
	{
		return -1;
	}



    //$[1bytes ch][2 length]
    dollar.cDollar = '$';
    dollar.nCH = nCh & 0xFF;
    wLen = htons(len);
    //  dollar.nLen = wLen;
    SendResponse();

    pHead = (char *)&dollar;
    pHead[2] = wLen & 0xFF;
    pHead[3] = (wLen >> 8)&0xFF;

    if(pDataNew[0] == '$' && pDataNew[1] == nCh)
    {
        wLen = pDataNew[2];
        wLen &= 0xFF;
        wLen <<= 8;
        wLen = wLen + (((int)(pDataNew[3])) & 0xFF);
        bNeedHead = 0;
        if(wLen + 4 < len)
        {
            if (pDataNew[wLen + 4] != '$')
            {
                bNeedHead = 1;
            }
        }

    }
    nHeadLen = 4;
    if (!bNeedHead)
    {
        nHeadLen = 0;
    }
    m_hWriteLock.Lock();

    if(len + nHeadLen > RTSP_BUFFER_SEND_SIZE - m_nWriteLen)
    {
        m_hWriteLock.Unlock();

        // printf("len = %d nHeadlen = %d nWritelen = %d nWpos = %d nSendPos = %d\n",len,nHeadLen,m_nWriteLen,m_nWritePos,m_nSendPos);
        return 0;

    }

    if (m_nWriteLen <= 0)
    {
        //
        if (nHeadLen > 0)
        {
            //int nSend = send(m_nClientSocket,(char *)pHead,nHeadLen,0);
            int nSend = SSL_write(m_pSsl,(char *)pHead,nHeadLen);
            if (nSend > 0)
            {
                nHeadLen -= nSend;
                pHead += nSend;
                if (nHeadLen <= 0)
                {
                    if(nDataLen > 0)
                    {
                        //int nSend = send(m_nClientSocket,(char *)pDataNew,nDataLen,0);
                        int nSend = SSL_write(m_pSsl,(char *)pDataNew,nDataLen);
                        if (nSend > 0)
                        {
                            nDataLen -= nSend;
                            pDataNew += nSend;

                        }
                        else
                        {
                            RTSP_DEBUG("ennor = %d [%s]\n",errno,strerror(errno));
                        }
                    }

                }
            }
        }
        else
        {
            if(nDataLen > 0)
            {
                //int nSend = send(m_nClientSocket,(char *)pDataNew,nDataLen,0);
                int nSend = SSL_write(m_pSsl,(char *)pDataNew,nDataLen);
                if (nSend > 0)
                {
                    nDataLen -= nSend;
                    pDataNew += nSend;

                }
                else
                {
                    RTSP_DEBUG("ennor = %d [%s]\n",errno,strerror(errno));
                }
            }

        }

    }
    else
    {
        // 缓冲有数据，丢包
        // m_hWriteLock.Unlock();
        // return 0;
    }
    if (nHeadLen == 0 && nDataLen == 0)
    {
        m_hWriteLock.Unlock();
        return len;
    }
    //RTSP_DEBUG("in buff headlen=%d %d errno = %d [%s] m_nWriteLen = %d\n",nHeadLen,len,errno,strerror(errno),m_nWriteLen);

    // RTSP_DEBUG("ch = %d,len = %d---< bufsize = %d\n",nCh,len,m_nWriteLen);
    if(m_nWritePos == RTSP_BUFFER_SEND_SIZE)
    {
        m_nWritePos = 0;
    }
    if (nHeadLen > 0)
    {
        if (m_nWritePos + nHeadLen > RTSP_BUFFER_SEND_SIZE)
        {
            memcpy(m_pResponseBuffer + m_nWritePos,pHead,RTSP_BUFFER_SEND_SIZE - m_nWritePos);

            memcpy(m_pResponseBuffer,pHead +RTSP_BUFFER_SEND_SIZE - m_nWritePos ,nHeadLen - (RTSP_BUFFER_SEND_SIZE - m_nWritePos));

            m_nWritePos = nHeadLen - (RTSP_BUFFER_SEND_SIZE - m_nWritePos);
        }
        else
        {
            memcpy(m_pResponseBuffer + m_nWritePos,pHead,nHeadLen);

            m_nWritePos += nHeadLen;
        }
    }

    m_nWriteLen += nHeadLen;

    if(m_nWritePos == RTSP_BUFFER_SEND_SIZE)
    {
        m_nWritePos = 0;
    }
    if (m_nWritePos + nDataLen > RTSP_BUFFER_SEND_SIZE)
    {
        memcpy(m_pResponseBuffer + m_nWritePos,pDataNew,RTSP_BUFFER_SEND_SIZE - m_nWritePos);

        memcpy(m_pResponseBuffer,(char *)pDataNew +RTSP_BUFFER_SEND_SIZE - m_nWritePos ,nDataLen - (RTSP_BUFFER_SEND_SIZE - m_nWritePos));

        m_nWritePos = nDataLen - (RTSP_BUFFER_SEND_SIZE - m_nWritePos);
    }
    else
    {
        memcpy(m_pResponseBuffer + m_nWritePos,(char *)pDataNew,nDataLen);

        m_nWritePos += nDataLen;
    }

    m_nWriteLen += nDataLen;
    m_pResponseBuffer[m_nWritePos] = 0;

    m_pRtspServer->IncWriteLen(nDataLen + nHeadLen);

    m_hWriteLock.Unlock();




    return len;

}


int CClientSocket::SendRTP_RTCPDataV2(int nCh,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,size_t len)
{

    DOLLAR_HEAER_T dollar;
    int nRet;
    int nHeadLen = 0;
    int nSend = 0;
    char *pHead = NULL,*pDataNew = (char *)pData;
    int nDataLen = 0;
    uint16_t wLen;
    int errorNo = 0;
    int bNeedHead = 1;
	struct timeval tv_timeToDelay;
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
    if (m_pOutPutSocket != NULL)
    {
		bOutPut = 1;
        if (!m_pOutPutSocket->m_bNeedClose)
        {
			pOutPut =  m_pOutPutSocket;
			pOutPut->SetUsed(1);
        }

    }
	m_pRtspServer->GUnlock();
	if (pOutPut != NULL)
	{
		nRet = pOutPut->SendRTP_RTCPDataV2(nCh,pRTPHeader,nRTPHeadLen,pAdd,nAddLen,pData,len);
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
		  return nRet;
	}

    if (m_bNeedClose == 1 || bOutPut)
    {
        return -1;
    }

    //$[1bytes ch][2 length]
    dollar.cDollar = '$';
    dollar.nCH = nCh & 0xFF;
    wLen = htons(len + nRTPHeadLen + nAddLen);
    //  dollar.nLen = wLen;

    pHead = (char *)&dollar;
    pHead[2] = wLen & 0xFF;
    pHead[3] = (wLen >> 8)&0xFF;


    if(pRTPHeader == NULL && pAdd == NULL && pDataNew[0] == '$' && pDataNew[1] == nCh)
    {
        wLen = pDataNew[2];
        wLen &= 0xFF;
        wLen <<= 8;
        wLen = wLen + (((int)(pDataNew[3])) & 0xFF);
        bNeedHead = 0;
        if(wLen + 4 < len)
        {
            if (pDataNew[wLen + 4] != '$')
            {
                bNeedHead = 1;
            }
        }

    }
    nHeadLen = 4;
    if (!bNeedHead)
    {
        nHeadLen = 0;
    }
    m_hWriteLock.Lock();

    while (nHeadLen > 0)
    {
        if (m_bThreadExit || m_bNeedClose)
        {
            break;
        }

        //
        //nSend = send(m_nClientSocket,(char *)pHead,nHeadLen,0);
        nSend = SSL_write(m_pSsl,(char *)pHead,nHeadLen);
        if (nSend > 0)
        {
            nHeadLen -= nSend;
            pHead += nSend;
        }
        else
        {
            errorNo = GetLastError();

            if(errorNo == EWOULDBLOCK ||
               errorNo == EINTR||
               errorNo == EAGAIN||
               errorNo == EINPROGRESS)
            {
				int selectResult;
				fd_set readSet,writeSet,exceptSet;
				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				FD_ZERO(&exceptSet);
				FD_SET(m_nClientSocket,&readSet);
				FD_SET(m_nClientSocket,&writeSet);
				tv_timeToDelay.tv_sec = 0;
				tv_timeToDelay.tv_usec = 10000;
				selectResult = select(m_nClientSocket + 1, &readSet, &writeSet, &exceptSet, &tv_timeToDelay);
				if (selectResult < 0)
				{//错误
					if( GetLastError() != EINTR)
					{
						m_hWriteLock.Unlock();
						SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						return -1;
					}
				}
				else
				{
					if (FD_ISSET(m_nClientSocket,&readSet))
					{
						CheckRecvHandler();
					}
					if (FD_ISSET(m_nClientSocket,&exceptSet))
					{
						m_hWriteLock.Unlock();
						SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						return -1;
					}

				}
                //RTSP_DEBUG("[%s.%d]errno = %d %s[%d] m_bNeedClose = %d m_bUsing = %d\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket, m_bNeedClose, m_bUsing);
				//CheckRecvHandler();
               // Ants_RTSPServer_WaitTime(0,10000);
                continue;
            }
            RTSP_DEBUG("[%s.%d]errno = %d %s[%d]\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket);
            m_hWriteLock.Unlock();
			SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
            return -1;
        }
    }

	nDataLen = nRTPHeadLen;

	while (nDataLen > 0)
    {
        if (m_bThreadExit || m_bNeedClose)
        {
            break;
        }

        //
        //nSend = send(m_nClientSocket,(char *)pRTPHeader,nDataLen,0);
        nSend = SSL_write(m_pSsl,(char *)pRTPHeader,nDataLen);
        if (nSend > 0)
        {
            nDataLen -= nSend;
            pRTPHeader += nSend;
        }
        else
        {
            errorNo = GetLastError();

            if(errorNo == EWOULDBLOCK ||
               errorNo == EINTR||
               errorNo == EAGAIN||
               errorNo == EINPROGRESS)
            {
				int selectResult;
				fd_set readSet,writeSet,exceptSet;
				 FD_ZERO(&readSet);
				 FD_ZERO(&writeSet);
				 FD_ZERO(&exceptSet);
				 FD_SET(m_nClientSocket,&readSet);
				 FD_SET(m_nClientSocket,&writeSet);
				 tv_timeToDelay.tv_sec = 0;
				 tv_timeToDelay.tv_usec = 10000;
				 selectResult = select(m_nClientSocket + 1, &readSet, &writeSet, &exceptSet, &tv_timeToDelay);
				 if (selectResult < 0)
				 {//错误
					 if( GetLastError() != EINTR)
					 {
						 m_hWriteLock.Unlock();
						 SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						 return -1;
					 }
				 }
				 else
				 {
					 if (FD_ISSET(m_nClientSocket,&readSet))
					 {
						CheckRecvHandler();
					 }
					 if (FD_ISSET(m_nClientSocket,&exceptSet))
					 {
						 m_hWriteLock.Unlock();
						 SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						 return -1;
					 }

				 }
                //RTSP_DEBUG("[%s.%d]errno = %d %s[%d] m_bNeedClose = %d m_bUsing = %d\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket, m_bNeedClose, m_bUsing);


               // Ants_RTSPServer_WaitTime(0,10000);
                continue;
            }
            RTSP_DEBUG("[%s.%d]errno = %d %s[%d]\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket);
            m_hWriteLock.Unlock();
           SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
            return -1;
        }
    }

	nDataLen = nAddLen;

	while (nDataLen > 0)
    {
        if (m_bThreadExit || m_bNeedClose)
        {
            break;
        }

        //
        //nSend = send(m_nClientSocket,(char *)pAdd,nDataLen,0);
        nSend = SSL_write(m_pSsl,(char *)pAdd,nDataLen);
        if (nSend > 0)
        {
            nDataLen -= nSend;
            pAdd += nSend;
        }
        else
        {
            errorNo = GetLastError();

            if(errorNo == EWOULDBLOCK ||
               errorNo == EINTR||
               errorNo == EAGAIN||
               errorNo == EINPROGRESS)
            {
				int selectResult;
				fd_set readSet,writeSet,exceptSet;
				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				FD_ZERO(&exceptSet);
				FD_SET(m_nClientSocket,&readSet);
				FD_SET(m_nClientSocket,&writeSet);
				tv_timeToDelay.tv_sec = 0;
				tv_timeToDelay.tv_usec = 10000;
				selectResult = select(m_nClientSocket + 1, &readSet, &writeSet, &exceptSet, &tv_timeToDelay);
				if (selectResult < 0)
				{//错误
					if( GetLastError() != EINTR)
					{
						m_hWriteLock.Unlock();
						SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						return -1;
					}
				}
				else
				{
					if (FD_ISSET(m_nClientSocket,&readSet))
					{
						CheckRecvHandler();
					}
					if (FD_ISSET(m_nClientSocket,&exceptSet))
					{
						m_hWriteLock.Unlock();
						SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						return -1;
					}

				}
                //RTSP_DEBUG("[%s.%d]errno = %d %s[%d] m_bNeedClose = %d m_bUsing = %d\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket, m_bNeedClose, m_bUsing);
				//CheckRecvHandler();
               // Ants_RTSPServer_WaitTime(0,10000);
                continue;
            }
            RTSP_DEBUG("[%s.%d]errno = %d %s[%d]\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket);
            m_hWriteLock.Unlock();
            m_bNeedClose = 1;
            return -1;
        }
    }

	nDataLen = len;

	while (nDataLen > 0)
    {
        if (m_bThreadExit || m_bNeedClose)
        {
            break;
        }

        //nSend = send(m_nClientSocket,(char *)pDataNew,nDataLen,0);
        nSend = SSL_write(m_pSsl,(char *)pDataNew,nDataLen);
        if (nSend > 0)
        {
            nDataLen -= nSend;
            pDataNew += nSend;
        }
        else
        {
            errorNo = GetLastError();

            if(errorNo == EWOULDBLOCK ||
               errorNo == EINTR||
               errorNo == EAGAIN||
               errorNo == EINPROGRESS)
            {
				int selectResult;
				fd_set readSet,writeSet,exceptSet;
				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				FD_ZERO(&exceptSet);
				FD_SET(m_nClientSocket,&readSet);
				FD_SET(m_nClientSocket,&writeSet);
				tv_timeToDelay.tv_sec = 0;
				tv_timeToDelay.tv_usec = 10000;
				selectResult = select(m_nClientSocket + 1, &readSet, &writeSet, &exceptSet, &tv_timeToDelay);
				if (selectResult < 0)
				{//错误
					if( GetLastError() != EINTR)
					{
						m_hWriteLock.Unlock();
						SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						return -1;
					}
				}
				else
				{
					if (FD_ISSET(m_nClientSocket,&readSet))
					{
						CheckRecvHandler();
					}
					if (FD_ISSET(m_nClientSocket,&exceptSet))
					{
						m_hWriteLock.Unlock();
						SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
						return -1;
					}

				}
                //RTSP_DEBUG("[%s.%d]errno = %d %s[%d] m_bNeedClose = %d m_bUsing = %d\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket, m_bNeedClose, m_bUsing);
				//CheckRecvHandler();
                //Ants_RTSPServer_WaitTime(0,10000);
                continue;
            }
            RTSP_DEBUG("[%s.%d]errno = %d %s[%d]\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket);
            m_hWriteLock.Unlock();
            SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
            return -1;
        }
    }

    m_hWriteLock.Unlock();

    return len + nRTPHeadLen + nAddLen;
}


int CClientSocket::SendData(const void *pData,size_t len)
{
    int nWrite = 0;
    int nRet;
    char *pDataNew = (char *)pData;
    int nDatalen = len;
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
	if (m_pOutPutSocket != NULL)
	{
		bOutPut = 1;
		if (!m_pOutPutSocket->m_bNeedClose)
		{
			pOutPut = m_pOutPutSocket;
			pOutPut->SetUsed(1);
		}
	}
	m_pRtspServer->GUnlock();

	if (pOutPut != NULL)
	{
		nRet =  pOutPut->SendData(pData,len);
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
		return nRet;
	}
	if (bOutPut)
	{
		return -1;
	}
    nRet = SendResponse();
    if (pData != NULL && len > 0)
    {
        //RTSP_DEBUG("\n[%d]S->:\n{%s}\n",m_nClientSocket,(char *)pData);
    }
    m_hWriteLock.Lock();


    if(nDatalen > RTSP_BUFFER_SEND_SIZE - m_nWriteLen)
    {


        if (nRet < 0 && m_nErrorCode != EWOULDBLOCK)
        {
            m_hWriteLock.Unlock();
            return -1;
        }
        else
        {
            m_hWriteLock.Unlock();
            return -1;
        }

    }
    if (m_nWriteLen <= 0)
    {
        // int nSend = send(m_nClientSocket,(char *)pDataNew,nDatalen,0);
        int nSend = SSL_write(m_pSsl,(char *)pDataNew,nDatalen);
        if (nSend > 0)
        {
            nDatalen -= nSend;
            pDataNew += nSend;
            if (nDatalen <= 0)
            {
                m_hWriteLock.Unlock();
                return len;
            }
        }
        else
        {
            RTSP_DEBUG("[%s.%d]errno = %d %s\n",__FUNCTION__,__LINE__,GetLastError(),strerror(GetLastError()));
        }
    }
    //RTSP_DEBUG("in buff %d err = %d[%s]\n",len,errno,strerror(errno));


    nWrite = nDatalen;
    if (nWrite > RTSP_BUFFER_SEND_SIZE - m_nWriteLen)
    {
        nWrite = RTSP_BUFFER_SEND_SIZE - m_nWriteLen;
    }
    if (nWrite != 0)
    {
        if (m_nWritePos + nWrite > RTSP_BUFFER_SEND_SIZE)
        {
            if(RTSP_BUFFER_SEND_SIZE - m_nWritePos > 0)
            {
                memcpy(m_pResponseBuffer + m_nWritePos,(char *)pDataNew,RTSP_BUFFER_SEND_SIZE - m_nWritePos);
            }


            memcpy(m_pResponseBuffer,(char *)pDataNew + RTSP_BUFFER_SEND_SIZE - m_nWritePos,nWrite - (RTSP_BUFFER_SEND_SIZE - m_nWritePos));
            m_nWritePos = nWrite - (RTSP_BUFFER_SEND_SIZE - m_nWritePos);
        }
        else
        {
            memcpy(m_pResponseBuffer + m_nWritePos,(char *)pDataNew,nWrite);
            m_nWritePos += nWrite;
        }

        m_nWriteLen += nWrite;
        m_pResponseBuffer[m_nWritePos] = 0;

        m_pRtspServer->IncWriteLen(nWrite);

    }
    else
    {
        m_pResponseBuffer[m_nWritePos] = 0;
    }

    m_hWriteLock.Unlock();

    SendResponse();


    return len;
}


int CClientSocket::SendDataV2(const void *pData,size_t len,int bType)
{
    int nWrite = 0;
    int nRet;
    char *pDataNew = (char *)pData;
    int nDatalen = len;
    int errorNo = 0;
	RTSP_SEND_MSG_T *pSendMsg = NULL, *pLast = NULL, *pDel = NULL, *pCur = NULL;
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
	if (m_pOutPutSocket != NULL)
	{
		bOutPut = 1;
		if (!m_pOutPutSocket->m_bNeedClose)
		{
			pOutPut = m_pOutPutSocket;
			pOutPut->SetUsed(1);
		}
	}
	m_pRtspServer->GUnlock();

	if (pOutPut != NULL)
	{
		nRet =  pOutPut->SendDataV2(pData,len,bType);
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
		return nRet;
	}


    if (pData != NULL && len > 0)
    {
        RTSP_DEBUG("\n[%d]S->:\n{%s}\n",m_nClientSocket,(char *)pData);
    }

	if (bOutPut)
    {
        if (pData != NULL)
        {
            free((char *)pData);
			pData = NULL;
		}
        return -1;
    }


	pSendMsg = (RTSP_SEND_MSG_T *)malloc(sizeof(RTSP_SEND_MSG_T));

	if (pSendMsg != NULL)
    {
        memset(pSendMsg, 0, sizeof(RTSP_SEND_MSG_T));
        pSendMsg->pData = (char *)pData;
		pSendMsg->len = len;
	    pSendMsg->bType = bType;
	    m_hMsgLock.Lock();
	    RTSP_DEBUG("\n---------add msg------\n");

		if (bType != 0)
		{
	        pCur = m_pSendMsg;
			pLast = NULL;

		    while (pCur != NULL)
		    {
		        if (pCur->bType == bType)
		        {
		            pDel = pCur;

					if (m_pSendMsg_tail == pDel)
	                {
                       m_pSendMsg_tail = pLast;
					}

		            if (pLast == NULL)
		            {
		                m_pSendMsg = pCur->pNext;
						pCur = m_pSendMsg;
					    pLast = NULL;
					}
					else
					{
		                pLast->pNext = pCur->pNext;
						pCur = pLast->pNext;
					}

					if (pDel->pData != NULL)
				    {
                        delete [](pDel->pData);
						pDel->pData = NULL;
					}
					free(pDel);
					pDel = NULL;
				    continue;
		        }

				pLast = pCur;
				pCur = pCur->pNext;
			}
		}

		if (m_pSendMsg_tail == NULL)
	    {
	        m_pSendMsg = m_pSendMsg_tail = pSendMsg;
	    }
	    else
	    {
	        m_pSendMsg_tail->pNext = pSendMsg;
	        m_pSendMsg_tail = pSendMsg;
	    }
        m_hMsgLock.Unlock();
    }

    return len;
}


int CClientSocket::SendMsg()
{
    int nWrite = 0;
    int nRet;
    char *pDataNew = NULL;
    int nDatalen = 0;
    int errorNo = 0;
	RTSP_SEND_MSG_T *pSendMsg = NULL, *pDel = NULL;
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
	if (m_pOutPutSocket != NULL)
	{
		bOutPut = 1;
		if (!m_pOutPutSocket->m_bNeedClose)
		{
			pOutPut = m_pOutPutSocket;
			pOutPut->SetUsed(1);
		}
	}
	m_pRtspServer->GUnlock();

    if (pOutPut != NULL)
    {
        nRet =  pOutPut->SendMsg();
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
		return nRet;
    }

	if (bOutPut)
    {
        return -1;
    }

	while (1)
	{
	    m_hMsgLock.Lock();
	    pSendMsg = m_pSendMsg;
        if (pSendMsg == NULL)
        {
 	        m_hMsgLock.Unlock();
            break;
		}

	    pDel = pSendMsg;
        pDataNew = pSendMsg->pData;
	    nDatalen = pSendMsg->len;
		pSendMsg = pSendMsg->pNext;
		m_pSendMsg = pSendMsg;
		if (pDel == m_pSendMsg_tail)
		{
            m_pSendMsg_tail = m_pSendMsg_tail->pNext;
		}
	    m_hMsgLock.Unlock();
	    RTSP_DEBUG("\SendMsg[%d]S->:\n{%s}\n",m_nClientSocket,(char *)pDataNew);

		m_hWriteLock.Lock();
        if (pDataNew != NULL)
        {
		    while (nDatalen > 0)
		    {
#if 0
		        if (m_bThreadExit || m_bNeedClose)
		        {
		            break;
		        }
#endif

		        // int nSend = send(m_nClientSocket,(char *)pDataNew,nDatalen,0);
		        int nSend = SSL_write(m_pSsl,(char *)pDataNew,nDatalen);
		        if (nSend > 0)
		        {
		            nDatalen -= nSend;
		            pDataNew += nSend;
		        }
		        else
		        {
		            errorNo = GetLastError();

		            if(errorNo == EWOULDBLOCK ||
		               errorNo == EINTR||
		               errorNo == EAGAIN||
		               errorNo == EINPROGRESS)
		            {
                        int bSleep = 1;
		                //RTSP_DEBUG("[%s.%d]errno = %d %s[%d] m_bNeedClose = %d m_bUsing = %d\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket, m_bNeedClose, m_bUsing);
						CheckRecvHandler();
                        if (IsSendUDP())
                        {
                            bSleep = m_pRtspServer->UDPData(m_pRtpSess, m_nSessionID);
                        }
                        if(bSleep)
                        {
		                    Ants_RTSPServer_WaitTime(0,10000);
                        }
		                continue;
		            }
		            RTSP_DEBUG("[%s.%d]errno = %d %s\n",__FUNCTION__,__LINE__,errorNo,strerror(errorNo),m_nClientSocket);
					delete [](pDel->pData);
			        pDel->pData = NULL;
					free(pDel);
					pDel = NULL;
		            m_hWriteLock.Unlock();
                    SetNeedClose(ANTS_RTSPSERVER_ERROR_SocketError);
		            return -1;
		        }
		    }
		    //RTSP_DEBUG("in buff %d err = %d[%s]\n",len,GetLastError(),strerror(GetLastError()));

			delete [](pDel->pData);
			pDel->pData = NULL;
        }

		free(pDel);
		pDel = NULL;
		m_hWriteLock.Unlock();
	}


	return 0;
}


int CClientSocket::SendResponse()
{
    int nRet = 0;
    int nSendBytes;
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
	if (m_pOutPutSocket != NULL)
	{
		bOutPut = 1;
		if (!m_pOutPutSocket->m_bNeedClose)
		{
			pOutPut = m_pOutPutSocket;
			pOutPut->SetUsed(1);
		}
	}
	m_pRtspServer->GUnlock();

	if (pOutPut != NULL)
	{
		nRet =  pOutPut->SendResponse();
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
		return nRet;
	}





    m_hWriteLock.Lock();

    if (m_nClientSocket < 0)
    {


        m_hWriteLock.Unlock();
        return 0;
    }


    if (m_nWriteLen <= 0)
    {

        m_hWriteLock.Unlock();
        return 0;
    }
    if (RTSP_BUFFER_SEND_SIZE == m_nSendPos)
    {
        m_nSendPos = 0;
    }
    if(m_nSendPos >= m_nWritePos)
    {

        // nSendBytes = send(m_nClientSocket,(char *)m_pResponseBuffer + m_nSendPos,RTSP_BUFFER_SEND_SIZE - m_nSendPos,0);
        nSendBytes = SSL_write(m_pSsl,(char *)m_pResponseBuffer + m_nSendPos,RTSP_BUFFER_SEND_SIZE - m_nSendPos);
        if (nSendBytes > 0)
        {
            if (nSendBytes == RTSP_BUFFER_SEND_SIZE - m_nSendPos)
            {

                m_nWriteLen -= nSendBytes;
                m_nSendPos = 0;

            }
            else
            {
                m_nSendPos += nSendBytes;
                m_nWriteLen -= nSendBytes;
            }
            m_bHasError = 0;
            m_nErrorCode = 0;
            m_pRtspServer->DecWriteLen(nSendBytes);
            nRet = nSendBytes;


        }
        else
        {

            m_bHasError = 1;
            m_nErrorCode = GetLastError();
            RTSP_DEBUG("[%s.%d]errno = %d %s[%d]\n",__FUNCTION__,__LINE__,m_nErrorCode,strerror(m_nErrorCode));
            //RTSP_DEBUG("S->:Send Failed <%d-%d> \n",nSendBytes,m_nErrorCode);

            nRet = -1;
        }


    }
    else
    {
        // nSendBytes = send(m_nClientSocket,(char *)m_pResponseBuffer + m_nSendPos,m_nWritePos - m_nSendPos,0);
        nSendBytes = SSL_write(m_pSsl,(char *)m_pResponseBuffer + m_nSendPos,m_nWritePos - m_nSendPos);
        if (nSendBytes > 0)
        {

            m_nSendPos += nSendBytes;
            m_nWriteLen -= nSendBytes;
            m_bHasError = 0;
            m_nErrorCode = 0;
            m_pRtspServer->DecWriteLen(nSendBytes);
            nRet = nSendBytes;


        }
        else
        {

            m_bHasError = 1;
            m_nErrorCode = GetLastError();
            RTSP_DEBUG("[%s.%d]errno = %d %s\n",__FUNCTION__,__LINE__,m_nErrorCode,strerror(m_nErrorCode));
            //RTSP_DEBUG("S->:Send Failed <%d-%d> \n",nSendBytes,m_nErrorCode);

            nRet = -1;
        }
    }



    m_hWriteLock.Unlock();


    return nRet;
}
int CClientSocket::MessageHandle()
{
    CMessage *pMsg;
    int nRet = 0;
    //开始处理消息

    pMsg = GetMessage();
    while(pMsg != NULL)
    {
        if(!pMsg->IsDollar())
        {
            // RTSP_DEBUG("[%d]C->:\n%s",m_nClientSocket,m_pRequestBuffer);
            RTSP_DEBUG("\n======MSG:[%d] %s======\n",pMsg->m_nLineCnt,pMsg->m_pMessage);
        }
        else
        {
            RTSP_DEBUG("\n======MSG:[%d] $======\n",pMsg->m_nMsgLen);
        }
        if (pMsg->IsCommand("OPTIONS"))
        {
            Options_handle(pMsg);

        }
        else if (pMsg->IsCommand("DESCRIBE"))
        {
            Describe_handle(pMsg);

        }
        else if (pMsg->IsCommand("SETUP"))
        {
            Setup_handle(pMsg);

        }
        else if (pMsg->IsCommand("PLAY"))
        {
            Play_handle(pMsg);

        }
        else if (pMsg->IsCommand("PAUSE"))
        {
            Pause_handle(pMsg);

        }
        else if (pMsg->IsCommand("SET_PARAMETER"))
        {
            set_parameter_handle(pMsg);
        }
        else if (pMsg->IsCommand("GET_PARAMETER"))
        {
            get_parameter_handle(pMsg);
        }
        else if (pMsg->IsCommand("ANTSCOMB_ADDCH"))
        {
            AntsComb_AddChan_handle(pMsg);
        }
        else if (pMsg->IsCommand("ANTSCOMB_DECCH"))
        {
            AntsComb_DecChan_handle(pMsg);
        }
        else if (pMsg->IsCommand("TEARDOWN"))
        {
            Teardown_handle(pMsg);
        }
        else if (pMsg->IsCommand("GET"))
        {
            Httpget_handle(pMsg);
        }
        else if (pMsg->IsCommand("POST"))
        {
            Httppost_handle(pMsg);
        }
        else if (pMsg->IsExtraData())
        {
            nRet = ExtraData_handle(pMsg);
        }
        else if (pMsg->IsDollar())
        {
            Dollar_handle(pMsg);
            nRet = 1;
        }
        else
        {
            int ii= 0;
            //SendData(NULL,0);
            //SendDataV2(NULL,0);

        }

        if (!pMsg->IsDollar())
        {
            //  RTSP_DEBUG("\n[%d]S->:\n%s",m_nClientSocket,(char *)m_pResponseBuffer);
        }
        //SendResponse();
        delete pMsg;
        pMsg = GetMessage();
    }



    if (m_bNeedClose)
    {
        return -1;
    }
    return nRet;

}

int CClientSocket::MessageHandleWithoutSend()
{
    CMessage *pMsg;
    int nRet = 0;
    //开始处理消息

	if (m_pMsg == NULL)
    {
        return 0;
    }

    pMsg = m_pMsg;
    while(pMsg != NULL)
    {
        if(!pMsg->IsDollar())
        {
            // RTSP_DEBUG("[%d]C->:\n%s",m_nClientSocket,m_pRequestBuffer);
            RTSP_DEBUG("\n======MSG:[%d] %s======\n",pMsg->m_nLineCnt,pMsg->m_pMessage);
        }
        else
        {
            RTSP_DEBUG("\n======MSG:[%d] $======\n",pMsg->m_nMsgLen);
        }
        if (pMsg->IsCommand("OPTIONS"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("DESCRIBE"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("SETUP"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("PLAY"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("PAUSE"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("SET_PARAMETER"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("GET_PARAMETER"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("ANTSCOMB_ADDCH"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("ANTSCOMB_DECCH"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("TEARDOWN"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("GET"))
        {
            nRet = 1;
        }
        else if (pMsg->IsCommand("POST"))
        {
            nRet = 1;
        }
        else if (pMsg->IsExtraData())
        {
            nRet = 1;
        }
        else if (pMsg->IsDollar())
        {
            nRet = 1;
        }
        else
        {
            int ii= 0;
            //SendData(NULL,0);
            //SendDataV2(NULL,0);

        }

        if (!pMsg->IsDollar())
        {
            //  RTSP_DEBUG("\n[%d]S->:\n%s",m_nClientSocket,(char *)m_pResponseBuffer);
        }
        //SendResponse();

		if (nRet == 1 && (!pMsg->IsCheck()))
		{
			CClientSocket *pOutPut = NULL;
            Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
			OutPutSocketRefreshTime(m_tLastRefreshTime);

			pMsg->m_bCheck = 1;
		}

        pMsg = pMsg->m_pNext;
    }

    if (m_bNeedClose)
    {
        return -1;
    }

    return nRet;
}

int CClientSocket::Httpget_handle(CMessage *pMsg)
{
    int i;
    char *pCookie = NULL;
    /*
    GET /ch01.264 HTTP/1.0
    CSeq: 1
    User-Agent: LIVE555 Streaming Media v2012.01.13
    x-sessioncookie: f4490eb919f5dee4f39cf33
    Accept: application/x-rtsp-tunnelled
    Pragma: no-cache
    Cache-Control: no-cache

    HTTP/1.0 200 OK\r\n
    Server: DSS/5.5.5 (Build/489.16; Platform/Linux; Release/Darwin; state/beta; )\r\n
    Content-Type: application/x-rtsp-tunnelled\r\n
    */

    int nBytes = 0;
    int nRet;
    char *buffer = NULL;

    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }

    buffer = new char[512];
    if (buffer == NULL)
    {
        return -1;
    }

    for (i = 0; i < pMsg->m_nLineCnt; i++)
    {
        if(!pMsg->StrCmp("x-sessioncookie:",i,0))
        {
            pCookie = pMsg->GetStr(i,1);
            break;
        }
    }
    if (pCookie == NULL)
    {
        delete[]buffer;
		buffer = NULL;
        return -1;
    }
    if (m_pOverHttpSessionCookie != NULL)
    {
        if (strcmp(m_pOverHttpSessionCookie,pCookie))
        {
            delete[]buffer;
		    buffer = NULL;
            return -1;
        }

    }
    else
    {
        m_pOverHttpSessionCookie = new char[strlen(pCookie) + 1];
    }

    if (m_pOverHttpSessionCookie == NULL)
    {
        delete[]buffer;
		buffer = NULL;
        return -1;
    }
	m_pRtspServer->GLock();
    strcpy(m_pOverHttpSessionCookie,pCookie);
    nRet = m_pRtspServer->AddHttpSessionCookie(this,m_pOverHttpSessionCookie);
	m_pRtspServer->GUnlock();
    if (nRet)
    {
        delete[]m_pOverHttpSessionCookie;
        m_pOverHttpSessionCookie = NULL;
        delete[]buffer;
		buffer = NULL;
        return -1;
    }
    nBytes += snprintf(buffer,512,
                       "HTTP/1.0 200 OK\r\n"
                       "Date: Thu, 19 Aug 1982 18:30:00 GMT\r\n"
                       "Cache-Control: no-cache\r\n"
                       "Pragma: no-cache\r\n"
                       "Content-Type: application/x-rtsp-tunnelled\r\n"
                       "\r\n");

    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);

    return 0;
}

int CClientSocket::Httppost_handle(CMessage *pMsg)
{
    uint32_t ResultSize = 0;
    int i;
    char *pCookie = NULL;
    CClientSocket *pOutput = NULL;
    for (i = 0; i < pMsg->m_nLineCnt; i++)
    {
        if(!pMsg->StrCmp("x-sessioncookie:",i,0))
        {
            pCookie = pMsg->GetStr(i,1);
            break;
        }
    }
    if (pCookie == NULL)
    {
        return -1;
    }
    if (m_pOverHttpSessionCookie != NULL)
    {
        if (strcmp(m_pOverHttpSessionCookie,pCookie))
        {
            return -1;
        }

    }
    pOutput = m_pRtspServer->GetOutPutClientByHttpSessionCookie(pCookie);
    if (pOutput == NULL)
    {
        return -1;
    }
    if (m_pOverHttpSessionCookie == NULL)
    {
        m_pOverHttpSessionCookie = new char[strlen(pCookie) + 1];
    }

    if (m_pOverHttpSessionCookie == NULL)
    {
        return -1;
    }
    m_pRtspServer->GLock();
	strcpy(m_pOverHttpSessionCookie,pCookie);
    m_pOutPutSocket = pOutput;
	m_pOutPutSocket->m_bOverHTTP = 1;
	pOutput->m_pInPutSocket = this;
	m_pRtspServer->AddPostHttpSessionCookie(this,pCookie);
	m_pRtspServer->GUnlock();

    ExtraData_handle(pMsg);

    return 0;
}

int CClientSocket::ExtraData_handle(CMessage *pMsg)
{
    unsigned char *pExtra;
    int nRet = 0;
    uint32_t ResultSize = 0;
    if (pMsg->m_bHasContext)
    {
        pExtra = (unsigned char *)Ants_Rtsp_Base64Decode((char *)pMsg->m_pContext,strlen((char *)pMsg->m_pContext),&ResultSize);
        //pExtra = base64Decode((char *)pMsg->m_pContext,ResultSize);

        if (pExtra != NULL)
        {
            m_bOverHTTP = 1;
            Parse((char *)pExtra,ResultSize);
            nRet = MessageHandle();
            //free(pExtra);
            delete []pExtra;
        }
    }

    return nRet;

}
int CClientSocket::Dollar_handle(CMessage *pMsg)
{
    int nCh,len;

    char *pstr = pMsg->GetStr(0,0);
    if (pstr == NULL || pstr[0] != '$')
    {
        return 0;
    }

    nCh = pstr[1];
    len = *((short *)(pstr + 2));
    len = htons(len);
    if (nCh == m_nPort_VA[0] ||
        nCh == m_nPort_VA[0] + 1)
    {


        //m.DealRecvPacket(pstr + 4,len);


    }
    else if (nCh == m_nPort_VA[1] ||
             nCh == m_nPort_VA[1] + 1)
    {

        // m_pAudioSessTCP.DealRecvPacket(pstr + 4,len);
    }
    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
    OutPutSocketRefreshTime(m_tLastRefreshTime);
    PushRTPorRTCP(nCh,pstr + 4,len);

    //time(&m_tLastRefreshTime);
    return 1;
}

void CClientSocket::OutPutSocketRefreshTime(uint32_t newTime)
{
	CClientSocket *pOutPut = NULL;
	int bOutPut = 0;
	m_pRtspServer->GLock();
	if (m_pOutPutSocket != NULL)
	{
		bOutPut = 1;
		if (!m_pOutPutSocket->m_bNeedClose)
		{
			pOutPut = m_pOutPutSocket;
			pOutPut->SetUsed(1);
		}
	}
	m_pRtspServer->GUnlock();

	if (pOutPut != NULL)
	{
		pOutPut->RefreshTime(newTime);
		m_pRtspServer->GLock();
		pOutPut->SetUsed(0);
		m_pRtspServer->GUnlock();
	}
}
int CClientSocket::Options_handle(CMessage *pMsg)
{
    int nBytes = 0;
    int nSeq = 0,nLine;
    char *pstr;
    char *buffer = NULL;
    RTSP_URL_T  *ptParseUrl = NULL;
    int nRet;
    time_t tim;
    int bDigest = 0;
    CRtspTmpObject tTmpObject;

    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    ptParseUrl = (RTSP_URL_T*)malloc(sizeof(RTSP_URL_T));
    tTmpObject.SetPt(ptParseUrl);
    if (ptParseUrl == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_MallocError);
        return -1;
    }
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }
        }
#if 0
        if (m_pUserName == NULL && (!pMsg->StrCmp("Authorization:",nLine,0)))
        {
            if (!pMsg->StrCmp("Basic",nLine,1))
            {
                char *pAuth;
                pAuth = pMsg->GetStr(nLine,2);
                if(pAuth != NULL)
                {
                    unsigned int nLen = strlen(pAuth) + 1;
                    char *pUserPwd;
                    pUserPwd = (char *)Ants_Rtsp_Base64Decode((char *)pMsg->m_pContext,nLen,NULL);

                    int nStrPos = 0;

                    if (pUserPwd != NULL)
                    {
                        char *pPwd = strchr(pUserPwd,':');
                        if (pPwd != NULL)
                        {
                            pPwd[0] = 0;
                            pPwd++;
                        }
                        m_pUserName = strdup(pUserPwd);
                        m_pPassword = strdup(pPwd);
                    }
                }
            }
        }
#endif

    }
#if 0
    if(!m_bStreamOpen)
    {
        Ants_RTSPV2_InParam_T tInParam;
        Ants_RTSPV2_OutParam_T tOutParam;
        char *pUrl;
        memset(&tOutParam,0,sizeof(tOutParam));
        memset(&tInParam,0,sizeof(tInParam));
        pUrl = pMsg->GetStr(0,1);
        if(pUrl != NULL)
        {
            if (m_pCurrUrl != NULL)
            {
                free(m_pCurrUrl);
                m_pCurrUrl = NULL;
            }
            m_pCurrUrl = strdup(pUrl);
        }
        if(m_pRequire != NULL)
        {
            free(m_pRequire);
            m_pRequire = NULL;
        }
        m_pRequire = pMsg->GetItem("Require:");
        tInParam.pUrl = m_pCurrUrl;
        tInParam.dwClientIP = m_dwIPAddress;
        tInParam.wRemotePort = m_uPort;
        tInParam.dwSessionID = m_nSessionID;
        tInParam.pRequire = m_pRequire;
        tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
        if(0 == m_pRtspServer->fStatusCallBack(0,ANTS_RTSPSERVER_CALLBACK_TYPE_OPEN_URL,&tInParam,NULL))
        {
            m_bStreamOpen = 1;
        }
    }
#endif

    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
       SetNeedClose(ANTS_RTSPSERVER_ERROR_MallocError);
        return -1;
    }

    time(&tim);

    buffer[0] = 0;
#if 0
    if(!m_bAuthOK)
    {
        // 获取密码
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
            "RTSP/1.0 401 Unauthorized\r\nCSeq: %d\r\n",
            nSeq);
        bDigest = 1;
        if(bDigest)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                "WWW-Authenticate: Digest realm=\"@\",nonce=\"%s\"\r\n",
                "xxx");

        }
        else
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                "WWW-Authenticate: Basic realm=\"@\"\r\n");
        }
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
        //delete []pBuffer;
        //m_bNeedClose = 1;
        return 0;
    }
#endif
    nRet = Ants_Rtsp_ParseUrl(pMsg->GetStr(0,1),ptParseUrl);
    if (nRet)
    {
        //错误
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 451 Parameter Not Understood\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
        //delete []buffer;
       SetNeedClose(ANTS_RTSPSERVER_ERROR_InvSdpParam);
        return 0;
    }
    if (ptParseUrl->nCustom == 1 && m_pRtspServer->IsOldRecording())
    {
        ptParseUrl->bRecording = 1;
    }

#if 0
    if (ptParseUrl->bRecording && m_pRtpSess == NULL)
    {
        // 需要创建流 token="recording" + clientID
        m_dwRecordCreateCnt++;
        sprintf(buffer,"Recording_%d_%u",m_dwRecordCreateCnt,m_nSessionID);
        m_hRtpSess = m_pRtspServer->CreateStream(buffer);
        m_pRtpSess = m_pRtspServer->GetSessionMgrByHandle(m_hRtpSess);
        if (m_pRtpSess == NULL)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes);
            delete []buffer;
            m_bNeedClose = 1;
            return 0;
        }
        m_pRtpSess->SetConfig(6,(void *)1,0);
        m_bCreateRecordStream = 1;
        m_bRecording = 1;
        m_tRecStart = ptParseUrl->tStart;
        m_tRecStop = ptParseUrl->tStop;
        m_stype = ptParseUrl->stype;
        m_ptype = ptParseUrl->ptype;
    }
#endif

    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
    OutPutSocketRefreshTime(m_tLastRefreshTime);
    // time(&m_tLastRefreshTime);

    nBytes = snprintf(buffer,ANTS_RTSP_TMP_BUFSIZE,"%s 200 OK\r\nCSeq: %d\r\n",
                      &pMsg->m_pMessage[pMsg->m_nPos[0][2]],
                      nSeq);
    if(m_bPlay)
    {
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d\r\n",m_nSessionID);
    }
    if (ptParseUrl->bRecording)
    {
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER, TEARDOWN\r\n\r\n");
    }
    else
    {
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Public: OPTIONS, DESCRIBE, SETUP, PLAY, GET_PARAMETER, SET_PARAMETER, TEARDOWN\r\n\r\n");
    }

    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);
    //delete []buffer;

    return 0;

}
/*
RTSP/1.0 200 OK
CSeq: 312
Date: 23 Jan 1997 15:35:06 GMT
	  Content-Type: application/sdp
	  Content-Length: 376
	  v=0
	  o=mhandley 2890844526 2890842807 IN IP4 126.16.64.4
	  s=SDP Seminar
	  i=A Seminar on the session description protocol
	  u=http://www.cs.ucl.ac.uk/staff/M.Handley/sdp.03.ps
e=mjh@isi.edu (Mark Handley)
c=IN IP4 224.2.17.12/127
t=2873397496 2873404696
a=recvonly
m=audio 3456 RTP/AVP 0
m=video 2232 RTP/AVP 31
m=whiteboard 32416 UDP WB
a=orient:portrait
*/
int CClientSocket::Describe_handle(CMessage *pMsg)
{

//[rtsp,urtsp]://[ip,dnsname][:port]/file?ctype=[video;audio]&stype=[unicast,multicast]&ptype=[tcp,http,udp]&ip=[224.x.x.x]&port=x
    int nBytes = 0,nContext = 0;
    time_t tim;
    char *buffer = NULL;
    char *Context=NULL;
    char *pBuffer = NULL;
    int handle = -1;
    int nLine ;
    char *pstr;
    int nSeq = 0;
    RTSP_URL_T *ptParseUrl = NULL;
    int nRet;
	unsigned int uiMulticastIP[4] = {0,0,0,0};
    int nMulticastPort,nMulticastTTL;
    char *pPPS = NULL ,*pSPS = NULL,*pVPS = NULL;
    int nProfileLevelID = -1;
    int nWaitVideoInPutCnt = 0;
    CRtpSessionMgr *pRtpSess = NULL;
    int bDigest = 0,bReqDigest = 0,bReqAuth = 0;
     int bAuth = 0;
    char *pUserName = NULL;
    char *pPassword = NULL;
    char *pNonce = NULL;
    char *pResponse = NULL;
    char *pUri = NULL;
    CRtspTmpObject tTmpObject;

#if 0
    nBytes = sprintf(buffer,"RTSP/1.0 401 Unauthorized\r\nCSeq: %s\r\n\r\n",
                     &pMsg->m_pMessage[pMsg->m_nPos[1][1]]);
    SendData(buffer,nBytes);
    return 0;
#endif
    if (pMsg->m_nLineCnt < 2)
    {
       SetNeedClose(ANTS_RTSPSERVER_ERROR_InvSdpParam);
        return -1;
    }
    ptParseUrl = (RTSP_URL_T*)malloc(sizeof(RTSP_URL_T));
    tTmpObject.SetPt(ptParseUrl);
    if (ptParseUrl == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_MallocError);
        return -1;
    }
    pBuffer = new char[(ANTS_RTSP_TMP_BUFSIZE + 1) * 2];
    if(pBuffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_MallocError);
        return -1;
    }
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }

        }
        if (m_pUserName == NULL && (!pMsg->StrCmp("Authorization:",nLine,0)))
        {
            bAuth = 1;
            if (!pMsg->StrCmp("Basic",nLine,1))
            {
                char *pAuth;

                pAuth = pMsg->GetStr(nLine,2);
                if(pAuth != NULL)
                {
                    unsigned int nLen = strlen(pAuth);
                    char *pUserPwd = NULL;
                    int nStrPos = 0;
                    pUserPwd = Ants_Rtsp_Base64Decode(pAuth,nLen,NULL);


                    if (pUserPwd != NULL)
                    {
                        char *pPwd = strchr(pUserPwd,':');
                        if (pPwd != NULL)
                        {
                            pPwd[0] = 0;
                            pPwd++;
                        }
                        pUserName = strdup(pUserPwd);
                        pPassword = strdup(pPwd);
                        delete [] pUserPwd;
                    }
                }
            }
            else if (!pMsg->StrCmp("Digest",nLine,1))
            {
                int nSeg;
                char *pSegStr,*pstr1,*pstr2;
                bDigest = 1;
                for (nSeg = 2;nSeg < pMsg->m_nSegCnt[nLine];nSeg++)
                {
                    if (pSegStr = pMsg->StrStr("username=\"",nLine,nSeg))
                    {
                        //pSegStr=pMsg->GetStr(nLine,nSeg);
                        if (pSegStr != NULL)
                        {
                            pstr1 = pSegStr+strlen("username=\"");
                            pstr2 = strchr(pstr1,'\"');
                            if (pstr2)
                            {
                                pstr2[0] = 0;
                            }
                            pUserName = strdup(pstr1);
                        }

                    }
                    else if (pSegStr = pMsg->StrStr("realm=\"",nLine,nSeg))
                    {
                       // pSegStr=pMsg->GetStr(nLine,nSeg);
                        if (pSegStr != NULL)
                        {
                            // = strdup(pSegStr);
                        }

                    }
                    else if (pSegStr = pMsg->StrStr("nonce=\"",nLine,nSeg))
                    {
                        //pSegStr=pMsg->GetStr(nLine,nSeg);
                        if (pSegStr != NULL)
                        {
                            pstr1 = pSegStr+strlen("nonce=\"");
                            pstr2 = strchr(pstr1,'\"');
                            if (pstr2)
                            {
                                pstr2[0] = 0;
                            }
                            pNonce = strdup(pstr1);
                        }

                    }
                    else if (pSegStr = pMsg->StrStr("uri=\"",nLine,nSeg))
                    {
                        //pSegStr=pMsg->GetStr(nLine,nSeg);
                        if (pSegStr != NULL)
                        {
                            pstr1 = pSegStr+strlen("uri=\"");
                            pstr2 = strchr(pstr1,'\"');
                            if (pstr2)
                            {
                                pstr2[0] = 0;
                            }
                            pUri = strdup(pstr1);
                        }

                    }
                    else if (pSegStr = pMsg->StrStr("response=\"",nLine,nSeg))
                    {
                        //pSegStr=pMsg->GetStr(nLine,nSeg);
                        if (pSegStr != NULL)
                        {
                            pstr1 = pSegStr+strlen("response=\"");
                            pstr2 = strchr(pstr1,'\"');
                            if (pstr2)
                            {
                                pstr2[0] = 0;
                            }
                             pResponse = strdup(pstr1);
                        }

                    }

                }
            }
        }

    }
    time(&tim);
    buffer = pBuffer;
    Context = pBuffer + ANTS_RTSP_TMP_BUFSIZE;

    buffer[0] = 0;
    if(!m_bAuthOK)
    {
        int bSendAuthReq = 0,bSendDigestReq = 0;
        Ants_RTSPV2_AuthInParam_T tIn;
        Ants_RTSPV2_AuthOutParam_T tOut;
        // 获取密码
        memset(&tIn,0,sizeof(tIn));
        memset(&tOut,0,sizeof(tOut));
		if (bDigest)
		{
			tIn.nAuthType = 2;
		}
		else if (bAuth)
		{
			tIn.nAuthType = 1;
		}


		tIn.pUrl = pMsg->GetStr(0,1);
		tIn.dwClientIP = m_dwIPAddress[0];
		tIn.wRemotePort = m_uPort;
		tIn.dwSessionID = m_nSessionID;
		tIn.bRecording = m_bRecording;
		tIn.wSize = sizeof(tIn);
		memcpy(tIn.dwClientIPv6,m_dwIPAddress,16);

        tIn.pUserName = pUserName;
		tIn.pPassword = pPassword;
		tIn.pRealm = "@";
		tIn.pNonce = m_pNonce;
		tIn.pUri = pUri;
		tIn.pCmd = "DESCRIBE";
		tIn.pResponse = pResponse;
        tOut.nAuthType = -1;
        if(0 == m_pRtspServer->fControl(m_hRtpSess,ANTS_RTSPSERVER_CALLBACK_TYPE_AUTH,&tIn,&tOut,pStreamUser))
        {
            bReqAuth = 0;
            bReqDigest = 0;


            char *pOrgUserName = tOut.pUserName;
            char *pOrgRealm="@";
            char *pOrgPassword=tOut.pPassword;


            if (tOut.nAuthType == 0)
            {
                bReqAuth = 1;
                bReqDigest = 0;
                bSendAuthReq = 1;
                bSendDigestReq = 1;
            }
            else if (tOut.nAuthType == 1)
            {// base 64
                bReqAuth = 1;
                bReqDigest = 0;
                bSendAuthReq = 1;
                bSendDigestReq = 0;
            }
            else if (tOut.nAuthType == 2)
            {
                bReqAuth = 1;
                bReqDigest = 1;
                bSendAuthReq = 1;
                bSendDigestReq = 1;
            }
			else if (tOut.nAuthType == 3)
			{
				if (!tOut.bAuthOk)
				{
					bReqAuth = 0;
					bReqDigest = 0;
					bSendAuthReq = 1;
					bSendDigestReq = 0;
				}
			}
			else if (tOut.nAuthType == 4)
			{
				if (!tOut.bAuthOk)
				{
					bReqAuth = 0;
					bReqDigest = 0;
					bSendAuthReq = 1;
					bSendDigestReq = 1;
				}
			}
            if(bReqAuth)
            {

                if(bAuth)
                {
                    if (bDigest)
                    {
                        if (bReqDigest || tOut.nAuthType == 0)
                        {

                            if(pResponse != NULL)
                            {
                                HASHHEX Response,HA1;
                                char *cmd = "DESCRIBE";

                                // The "response" field is computed as:
                                //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
                                // or, if "fPasswordIsMD5" is True:
                                //    md5(<password>:<nonce>:md5(<cmd>:<url>))
                                DigestCalcHA1("",pOrgUserName,pOrgRealm,pOrgPassword,"","",HA1);
                                DigestCalcResponse(HA1,m_pNonce,"","","",cmd,pUri,"",Response);
                                if (0 == strcmp(Response,pResponse))
                                {
                                    m_bAuthOK = 1;
                                    bSendAuthReq = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        if(!bReqDigest)
                        {
                            // base64
                            if (pOrgUserName != NULL)
                            {
                                if (pUserName != NULL && pUserName[0] != 0)
                                {
                                    if (0 == strcmp(pUserName,pOrgUserName))
                                    {
                                        // 密码
                                        if (pOrgPassword == NULL || pOrgPassword[0] == 0)
                                        {
                                            if (pPassword == NULL || pPassword[0] == 0)
                                            {
                                                m_bAuthOK = 1;
                                                bSendAuthReq = 0;
                                            }
                                        }
                                        else
                                        {
                                            if (pPassword != NULL && pPassword[0] != 0)
                                            {
                                                if (!strcmp(pOrgPassword,pPassword))
                                                {
                                                    m_bAuthOK = 1;
                                                    bSendAuthReq = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                }

             }
        }

        if(!bSendAuthReq)
        {
            if(tOut.pUserName != NULL)
            {
                m_pUserName = tOut.pUserName;
                tOut.pUserName = NULL;
                m_pPassword = tOut.pPassword;
                tOut.pPassword = NULL;
            }
            else
            {
                m_pUserName = pUserName;
                pUserName = NULL;
                m_pPassword = pPassword;
                pPassword = NULL;
            }

        }
        if(tOut.pUserName)
        {
            free(tOut.pUserName);
            tOut.pUserName = NULL;
        }
        if(tOut.pPassword)
        {
            free(tOut.pPassword);
            tOut.pPassword = NULL;
        }
        if(bSendAuthReq)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                "RTSP/1.0 401 Unauthorized\r\nCSeq: %d\r\n",
                nSeq);
            if(bSendDigestReq)
            {
                // 生成 nonce;
                if (m_pNonce != NULL)
                {
                    free(m_pNonce);
                    m_pNonce = NULL;
                }
                m_pNonce = (char *)malloc(128);
                if (m_pNonce != NULL)
                {
                    int nNonLen = 0;
                    m_pNonce[127] = 0;
                    nNonLen += snprintf(m_pNonce+nNonLen,64 - nNonLen,"%x",m_nSessionID);
                    nNonLen += snprintf(m_pNonce+nNonLen,64 - nNonLen,"%x",nSeq);
                    nNonLen += snprintf(m_pNonce+nNonLen,64 - nNonLen,"%x",tim);
                    nNonLen += snprintf(m_pNonce+nNonLen,64 - nNonLen,"%x",(uint32_t)(uint64_t)m_pNonce);
                }

                nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                    "WWW-Authenticate: Digest realm=\"@\", nonce=\"%s\"\r\n",
                    m_pNonce?m_pNonce:"");

            }
            else
            {
                nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                    "WWW-Authenticate: Basic realm=\"@\"\r\n");
            }
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes,0);
            //delete []pBuffer;
            //m_bNeedClose = 1;

            // free
            if (pUserName)
            {
                free(pUserName);
                pUserName = NULL;
            }
            if (pPassword)
            {
                free(pPassword);
                pPassword = NULL;
            }
            if (pNonce)
            {
                free(pNonce);
                pNonce = NULL;
            }
            if (pResponse)
            {
                free(pResponse);
                pResponse = NULL;
            }
            if (pUri)
            {
                free(pUri);
                pUri = NULL;
            }
            return 0;
        }

    }
    // free
    if (pUserName)
    {
        free(pUserName);
        pUserName = NULL;
    }
    if (pPassword)
    {
        free(pPassword);
        pPassword = NULL;
    }
    if (pNonce)
    {
        free(pNonce);
        pNonce = NULL;
    }
    if (pResponse)
    {
        free(pResponse);
        pResponse = NULL;
    }
    if (pUri)
    {
        free(pUri);
        pUri = NULL;
    }
    //
    nRet = Ants_Rtsp_ParseUrl(pMsg->GetStr(0,1),ptParseUrl);
    if (nRet)
    {
        //错误
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 451 Parameter Not Understood\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
        //delete []pBuffer;
        SetNeedClose(ANTS_RTSPSERVER_ERROR_InvSdpParam);
        return 0;
    }
    if (ptParseUrl->nCustom == 1 && m_pRtspServer->IsOldRecording())
    {
        ptParseUrl->bRecording = 1;
    }


#if 0
    if (ptParseUrl->bRecording && m_pRtpSess == NULL)
    {
        // 需要创建流 token="recording" + clientID
        m_dwRecordCreateCnt++;
        sprintf(buffer,"Recording_%d_%u",m_dwRecordCreateCnt,m_nSessionID);
        m_hRtpSess = m_pRtspServer->CreateStream(buffer);
        m_pRtpSess = m_pRtspServer->GetSessionMgrByHandle(m_hRtpSess);
        if (m_pRtpSess == NULL)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            SendData(buffer,nBytes);
            delete []pBuffer;
            m_bNeedClose = 1;
            return 0;
        }
        m_pRtpSess->SetConfig(6,(void *)1,0);
        m_tRecStart = ptParseUrl->tStart;
        m_tRecStop = ptParseUrl->tStop;
        m_bCreateRecordStream = 1;
        m_bRecording = 1;
        handle = m_hRtpSess;
        pRtpSess = m_pRtpSess;
        m_stype = ptParseUrl->stype;
        m_ptype = ptParseUrl->ptype;
    }
    else
    {
        handle = m_hRtpSess;
        pRtpSess = m_pRtpSess;
        m_stype = ptParseUrl->stype;
        m_ptype = ptParseUrl->ptype;
    }
#endif


    if(!m_bStreamOpen)
    {
        Ants_RTSPV2_InParam_T tInParam;
        Ants_RTSPV2_OutParam_T tOutParam;
        char *pUrl;
        memset(&tOutParam,0,sizeof(tOutParam));
        memset(&tInParam,0,sizeof(tInParam));
        pUrl = pMsg->GetStr(0,1);
        if(pUrl != NULL)
        {
            if (m_pCurrUrl != NULL)
            {
                free(m_pCurrUrl);
                m_pCurrUrl = NULL;
            }
            m_pCurrUrl = strdup(pUrl);
        }
        if(m_pRequire != NULL)
        {
            free(m_pRequire);
            m_pRequire = NULL;
        }
        m_pRequire = pMsg->GetItem("Require:");
        tInParam.pUrl = m_pCurrUrl;

        tInParam.wRemotePort = m_uPort;
        tInParam.dwSessionID = m_nSessionID;
        tInParam.pRequire = m_pRequire;
		tInParam.nAntsComb = ptParseUrl->bAntsComb;
        tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
		tInParam.bIPv6 = m_bIPv6;
		if(m_bIPv6)
		{
			memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
		}
		else
		{
			tInParam.dwClientIP = m_dwIPAddress[0];
		}

        if (ptParseUrl->bRecording)
        {
            tInParam.bRecording = 1;
            tInParam.nChan = ptParseUrl->nCh;
            tInParam.nStream = ptParseUrl->nStream;
            tInParam.bRangeValid = ptParseUrl->bTimeValid;
            tInParam.tRangeStart = ptParseUrl->tStart;
            tInParam.tRangeStop = ptParseUrl->tStop;

        }

        if(0 == m_pRtspServer->fStatusCallBack(m_nSessionID,ANTS_RTSPSERVER_CALLBACK_TYPE_SET_PARAM,&tInParam,&tOutParam))
        {
            m_bStreamOpen = 1;
            m_nRequireType = tOutParam.nRequireType;
            m_nRequireVideoType = tOutParam.nRequireVideoType;
            m_nRequireAudioType = tOutParam.nRequireAudioType;
            m_nCh = tOutParam.nCh;
            m_nStream = tOutParam.nStream;
        }
    }

    if (m_pRequire != NULL && (m_nRequireType <= 0 || m_nRequireType > 3))
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 551 Option not supported\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Unsupported: %s\r\n",m_pRequire);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
        //delete []pBuffer;
       SetNeedClose(ANTS_RTSPSERVER_ERROR_InvSdpParam);
        return 0;
    }

    if (ptParseUrl->bRecording == 1)
    {
        m_bRecording = 1;
    }

	if (ptParseUrl->bAntsComb == 1)
	{
        m_bAntsComb = 1;
		if (m_nClientSocket > 0)
		{
			int size;
			size = 256 * 1024;
	        setsockopt(m_nClientSocket,SOL_SOCKET,SO_SNDBUF,(const char *)&size,sizeof(int));
		}
	}

#if 0
    if(m_pRtpSess == NULL)
    {

        if (ptParseUrl->nCustom == 1)
        {
            char *pNewToken,*pfind;

            pfind = strstr(pMsg->GetStr(0,1),"recordstream");
            if (pfind == NULL)
            {
                delete []pBuffer;
                m_bNeedClose = 1;
                return 0;
            }
            pNewToken = strdup(pfind);
            if (pNewToken == NULL)
            {
                delete []pBuffer;
                m_bNeedClose = 1;
                return 0;
            }
            int i = 0;
            while(1)
            {
                if (pNewToken[i] == 0)
                {
                    break;
                }
                if(!((pNewToken[i] >= 'a' && pNewToken[i] <= 'z') ||
                     (pNewToken[i] >= 'A' && pNewToken[i] <= 'Z') ||
                     (pNewToken[i] >= '0' && pNewToken[i] <= '9') ||
                     pNewToken[i] == '.'
                    ))
                {

                    pNewToken[i] = '_';
                }
                i++;
            }
            handle = m_pRtspServer->LookupParam(pNewToken,&pRtpSess);
            free(pNewToken);
            pNewToken = NULL;
        }
        else
        {
            handle = m_pRtspServer->LookupParam(ptParseUrl->pStreamFileName,&pRtpSess);
        }

        if (handle < 0)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            SendData(buffer,nBytes);
            delete []pBuffer;
            m_bNeedClose = 1;
            return 0;

        }
#if 0
        if (m_pRtspServer->IsAutoPayloadType(handle,0))
        {
            while(1)
            {
                nWaitVideoInPutCnt++;
                if (m_pRtspServer->IsVideoReady(handle))
                {
                    break;
                }
                if (nWaitVideoInPutCnt > 5)
                {
                    break;
                }
                Ants_WaitTime(1,0);
            }
        }
#endif


        m_hRtpSess = handle;
        m_pRtpSess = pRtpSess;
        m_stype = ptParseUrl->stype;
        m_ptype = ptParseUrl->ptype;

    }
    else
    {
        handle = m_hRtpSess;
        pRtpSess = m_pRtpSess;
        m_stype = ptParseUrl->stype;
        m_ptype = ptParseUrl->ptype;
    }
#endif

    m_stype = ptParseUrl->stype;
    m_ptype = ptParseUrl->ptype;

    nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"v=0\r\n");

#if 1

#if 0
    {
        struct hostent *h;
        unsigned int dwLocalIP;
        h=gethostbyname(ptParseUrl->pAddress);
        if (h != NULL)
        {
            int i;
            char ipstr[17];
            for (i = 0; (h->h_addr_list)[i] != NULL; i++)
            {

                RTSP_DEBUG("%s\n", inet_ntoa(*(struct in_addr  *)(h->h_addr_list)[i]));
                RTSP_DEBUG("officail name : %s\n", h->h_name);
            }
            dwLocalIP = ((struct in_addr  *)h->h_addr)->s_addr;

            if (dwLocalIP != 0 && dwLocalIP != -1)
            {
                nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"o=- %d %d IN IP4 %d.%d.%d.%d\r\n",m_nSessionID,m_nSessionID,
                                     dwLocalIP&0xFF,(dwLocalIP >> 8)&0xFF,(dwLocalIP>>16)&0xFF,(dwLocalIP>>24)&0xFF);
            }
        }

    }
#endif
        {
			unsigned int dwLocalIP[4] = {0,0,0,0};
			int bIsIPv6 = 0;
            struct addrinfo *pResult = NULL,*pNext = NULL;
            if(0 == getaddrinfo(ptParseUrl->pAddress,NULL, NULL, &pResult ))
            {
                pNext = pResult;
                while (pNext != NULL)
                {
                    if (pNext->ai_family == AF_INET)
                    {
                        //RTSP_DEBUG("officail name : %s\n", inet_ntoa((struct in_addr)((struct sockaddr_in *)pNext->ai_addr)->sin_addr));
                        //RTSP_DEBUG("officail name : %s\n", pNext->ai_canonname );
                        dwLocalIP[0] = ((struct in_addr)((struct sockaddr_in *)pNext->ai_addr)->sin_addr).s_addr;
                         RTSP_DEBUG("officail name : %d.%d.%d.%d\n", dwLocalIP[0]&0xFF,(dwLocalIP[0]>>8)&0xFF,(dwLocalIP[0]>>16)&0xFF,(dwLocalIP[0]>>24)&0xFF);
                         break;

					}
					else if (pNext->ai_family == AF_INET6)
					{
						memcpy(dwLocalIP,&((struct sockaddr_in6 *)pNext->ai_addr)->sin6_addr,16);
						bIsIPv6 = 1;
					}
					pNext = pNext->ai_next;
				}

            freeaddrinfo(pResult);
			if (bIsIPv6)
			{
				char szStringIPv6[128];
				inet_ntop(AF_INET6,dwLocalIP,szStringIPv6,128);
				nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"o=- %d %d IN IP6 %s\r\n",m_nSessionID,m_nSessionID,
					szStringIPv6);
				nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:*\r\n");
				nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=source-filter: incl IN IP6 * %s\r\n",
					szStringIPv6);
			}
			else if (dwLocalIP[0] != 0 && dwLocalIP[0] != -1)
                {
                    nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"o=- %d %d IN IP4 %d.%d.%d.%d\r\n",m_nSessionID,m_nSessionID,
                        dwLocalIP[0]&0xFF,(dwLocalIP[0] >> 8)&0xFF,(dwLocalIP[0]>>16)&0xFF,(dwLocalIP[0]>>24)&0xFF);
                    nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:*\r\n");
                    nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=source-filter: incl IN IP4 * %d.%d.%d.%d\r\n",
                        dwLocalIP[0]&0xFF,(dwLocalIP[0] >> 8)&0xFF,(dwLocalIP[0]>>16)&0xFF,(dwLocalIP[0]>>24)&0xFF);


                }
            }



    }
#else
    nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"o=- %d %d IN IP4 1h\r\n",m_nSessionID,m_nSessionID);
#endif
    nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"s=RTSP Server\r\n");
    if (m_bRecording)
    {
#if 0
        if (ptParseUrl->bTimeValid & 1)
        {
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=range:clock=%04d%02d%02dT%02d%02d%02dZ-",
                                 ptParseUrl->tStart.wYear,ptParseUrl->tStart.byMon,ptParseUrl->tStart.byDay,ptParseUrl->tStart.byHour,ptParseUrl->tStart.byMin,ptParseUrl->tStart.bySec);
        }
        if (ptParseUrl->bTimeValid & 2)
        {
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"%04d%02d%02dT%02d%02d%02dZ",
                                 ptParseUrl->tStop.wYear,ptParseUrl->tStop.byMon,ptParseUrl->tStop.byDay,ptParseUrl->tStop.byHour,ptParseUrl->tStop.byMin,ptParseUrl->tStop.bySec);
        }
        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"\r\n");
#else
        if (ptParseUrl->bTimeValid & 3)
        {
            struct tm tSaveTm,tSaveTm1;
            time_t newTime,StopTime;
            tSaveTm.tm_year = ptParseUrl->tStart.wYear - 1900;
            tSaveTm.tm_mon = ptParseUrl->tStart.byMon - 1;
            tSaveTm.tm_mday = ptParseUrl->tStart.byDay;
            tSaveTm.tm_hour = ptParseUrl->tStart.byHour;
            tSaveTm.tm_min = ptParseUrl->tStart.byMin;
            tSaveTm.tm_sec = ptParseUrl->tStart.bySec;
            newTime = mktime(&tSaveTm);
            tSaveTm1.tm_year = ptParseUrl->tStop.wYear - 1900;
            tSaveTm1.tm_mon = ptParseUrl->tStop.byMon - 1;
            tSaveTm1.tm_mday = ptParseUrl->tStop.byDay;
            tSaveTm1.tm_hour = ptParseUrl->tStop.byHour;
            tSaveTm1.tm_min = ptParseUrl->tStop.byMin;
            tSaveTm1.tm_sec = ptParseUrl->tStop.bySec;
            StopTime = mktime(&tSaveTm1);
			 nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"t=%lld %lld\r\n",newTime,StopTime);
            if(StopTime > newTime)
            {
                nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=range:npt=0-%d\r\n",StopTime-newTime);
            }
            else
            {
                nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=range:npt=0-\r\n");
            }


        }
        else
        {
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=range:npt=0-\r\n");
        }




#endif



    }
    else
    {
		 nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"t=0 0\r\n");
        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=range:npt=0-\r\n");

    }




    if (m_StreamType == ANTS_RTSPSERVER_STREAMTYPE_ALL || m_StreamType == ANTS_RTSPSERVER_STREAMTYPE_VIDEO)
    {
        int payloadtype = m_bPlayloadType[0];
        int bMulticast = 0,bUseDefault = 0;
        // 默认多播地址
        if(GetMulticastIPv4(uiMulticastIP,&nMulticastPort,&nMulticastTTL,0) >= 0)
        {
            bMulticast = 1;
        }
#if 0
        if (htonl(ptParseUrl->destIPV) < 0xE0000000 || htonl(ptParseUrl->destIPV) > 0xEFFFFFFF)
        {
            bUseDefault = bMulticast;
        }
#else
        if (ptParseUrl->stype == 2)
        {
            bUseDefault = bMulticast;
        }

#endif

        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"m=video %d RTP/AVP %d\r\n",bUseDefault?nMulticastPort:ptParseUrl->destPortV,payloadtype);
        if (ptParseUrl->stype != 2)
        {
			if (m_bIPv6)
			{
				unsigned short wIPv6[8];
				char szIPv6String[128];
				GetLocalIPv6((unsigned char *)wIPv6,szIPv6String,128);

				//nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP6 %x:%x:%x:%x:%x:%x:%x:%x\r\n",wIPv6[0],wIPv6[1],wIPv6[2],wIPv6[3],wIPv6[4],wIPv6[5],wIPv6[6],wIPv6[7]);
				nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP6 %s\r\n",szIPv6String);
			}
			else
			{
				unsigned char byIPv4[16];

				GetLocalIPv6((unsigned char *)byIPv4);
				nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP4 %d.%d.%d.%d\r\n",byIPv4[0],byIPv4[1],byIPv4[2],byIPv4[3]);
			}
        }
#if 0
        if (ptParseUrl->stype == 2)
        {
            //多播


            nContext += sprintf(Context + nContext,"c=IN IP4 %d.%d.%d.%d/%d\r\n",GETBYTE(ptParseUrl->destIPV,0),GETBYTE(ptParseUrl->destIPV,1),GETBYTE(ptParseUrl->destIPV,2),GETBYTE(ptParseUrl->destIPV,3),ptParseUrl->nTTL == 0?255:ptParseUrl->nTTL);
        }
#else
        if (ptParseUrl->stype == 2)
        {
            //多播
            if(m_pRtspServer->GetSupportMulticast(-1) && GetSupportMulticast())
            {

				if (m_bIPv6)
				{
					if(bUseDefault)
					{
						char szIPv6String[128];
						inet_ntop(AF_INET6,uiMulticastIP,szIPv6String,128);
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP6 %s\r\n",szIPv6String);
					}
					else
					{
						char szIPv6String[128];
						inet_ntop(AF_INET6,ptParseUrl->destIPV,szIPv6String,128);
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP6 %s\r\n",szIPv6String);
					}
				}
				else
				{
					if(bUseDefault)
					{
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP4 %d.%d.%d.%d/%d\r\n",GETBYTE(uiMulticastIP[0],0),GETBYTE(uiMulticastIP[0],1),GETBYTE(uiMulticastIP[0],2),GETBYTE(uiMulticastIP[0],3),nMulticastTTL == 0?255:nMulticastTTL);
					}
					else
					{
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP4 %d.%d.%d.%d/%d\r\n",GETBYTE(ptParseUrl->destIPV[0],0),GETBYTE(ptParseUrl->destIPV[1],1),GETBYTE(ptParseUrl->destIPV[2],2),GETBYTE(ptParseUrl->destIPV[3],3),ptParseUrl->nTTL == 0?255:ptParseUrl->nTTL);
					}
				}

            }

        }

#endif

        if (payloadtype >= 96)
        {

            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=rtpmap:%d %s/%d\r\n",payloadtype,
                                 m_szPlayloadName[0],
                                 m_nPlayloadClockRate[0]);
        }
        pVPS = GetVPS_Base64();
        if (pVPS != NULL)
        {
            // 265
            pPPS = GetPPS_Base64();
            pSPS = GetSPS_Base64();
			 nProfileLevelID = H265_GetProfileId();
            if (payloadtype == ANTS_RTSPSERVER_PAYLOADTYPE_H265 &&
                pPPS != NULL &&
                pSPS != NULL &&
                pVPS != NULL &&
                nProfileLevelID != -1)
            {// profile-space=0;profile-id=%X;tier-flag=0;level-id=150;interop-constraints=000000000000; //nProfileLevelID,
                  nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=fmtp:%d packetization-mode=1;profile-space=%u;profile-id=%u;tier-flag=%u;level-id=%u;interop-constraints=%s;sprop-vps=%s;sprop-sps=%s;sprop-pps=%s\r\n",
					  payloadtype,H265_GetProfileSpace(),nProfileLevelID,H265_GetTierFlag(),H265_GetLevelId(),H265_GetInteropConstraints(),pVPS,pSPS,pPPS);
            }
        }
        else
        {
            // 264
            pPPS = GetPPS_Base64();
            pSPS = GetSPS_Base64();


            nProfileLevelID = GetProfileLevelID();
            //RTSP_DEBUG("H264.......payloadtype = %d..pPPS = %x pSPS = %x. id = %x\n",payloadtype,pPPS,pSPS,nProfileLevelID);
            if (payloadtype == ANTS_RTSPSERVER_PAYLOADTYPE_H264 && pPPS != NULL && pSPS != NULL && nProfileLevelID != -1)
            {
                nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=fmtp:%d profile-level-id=%X; packetization-mode=1; sprop-parameter-sets=%s,%s\r\n",payloadtype,nProfileLevelID,pSPS,pPPS);
            }

        }
	if(payloadtype == ANTS_RTSPSERVER_PAYLOADTYPE_H265)
		// nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=Media_header:MEDIAINFO=494D4B48010100000400050000000000000000000000000000000000000000000000000000000000;\r\na=appversion:1.0\r\n");
		 nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=Media_header:MEDIAINFO=494D4B48010100000400050010710110401F000000FA000000000000000000000000000000000000;\r\na=appversion:1.0\r\n");

	else if(payloadtype == ANTS_RTSPSERVER_PAYLOADTYPE_H264)
		nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=Media_header:MEDIAINFO=494D4B48010100000400010000000000000000000000000000000000000000000000000000000000;\r\na=appversion:1.0\r\n");


        if (pPPS != NULL)
        {
            delete []pPPS;
            pPPS = NULL;
        }
        if (pSPS != NULL)
        {
            delete []pSPS;
            pSPS = NULL;
        }





        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:%sctype=video\r\n",ptParseUrl->bHasParam?"":"?");
        if (ptParseUrl->bRecording)
        {
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=x-onvif-track:VIDEO001\r\n");
        }
        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=recvonly\r\n");

        //  nContext += sprintf(Context + nContext,"a=framerate:25\r\n");
    }
    if (m_StreamType == ANTS_RTSPSERVER_STREAMTYPE_ALL || m_StreamType == ANTS_RTSPSERVER_STREAMTYPE_AUDIO)
    {
        int payloadtype = m_bPlayloadType[1];
        int bMulticast = 0,bUseDefault =0;
        if(GetMulticastIPv4(uiMulticastIP,&nMulticastPort,&nMulticastTTL,1) >= 0)
        {
            bMulticast = 1;
        }
#if 0
        if (htonl(ptParseUrl->destIPA) < 0xE0000000 || htonl(ptParseUrl->destIPA) > 0xEFFFFFFF)
        {
            bUseDefault = bMulticast;
        }
#else
        if (ptParseUrl->stype == 2)
        {
            bUseDefault = bMulticast;
        }
#endif

        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"m=audio %d RTP/AVP %d\r\n",bUseDefault?nMulticastPort:ptParseUrl->destPortA,payloadtype);
        //if (payloadtype >= 96)
        {

             nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=rtpmap:%d %s/%d/1\r\n",payloadtype,
                 m_szPlayloadName[1],
                 m_nPlayloadClockRate[1]);
        }

        if(payloadtype == ANTS_RTSPSERVER_PAYLOADTYPE_AAC)
        {
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,
                "a=fmtp:%d streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=0d88;Profile=1\r\n",payloadtype);

        }
#if 0

        if (ptParseUrl->stype == 2)
        {
            //多播

            nContext += sprintf(Context + nContext,"c=IN IP4 %d.%d.%d.%d/%d\r\n",GETBYTE(ptParseUrl->destIPA,0),GETBYTE(ptParseUrl->destIPA,1),GETBYTE(ptParseUrl->destIPA,2),GETBYTE(ptParseUrl->destIPA,3),ptParseUrl->nTTL == 0?255:ptParseUrl->nTTL);
        }
#else
        if (ptParseUrl->stype == 2)
        {
            //多播
            if(m_pRtspServer->GetSupportMulticast(-1) && GetSupportMulticast())
            {
				if (m_bIPv6)
				{
					char szStringIPv6[128];
					if(bUseDefault)
					{
						inet_ntop(AF_INET6,uiMulticastIP,szStringIPv6,128);
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP6 %s\r\n",szStringIPv6);
					}
					else
					{
						inet_ntop(AF_INET6,ptParseUrl->destIPA,szStringIPv6,128);
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP6 %s\r\n",szStringIPv6);
					}
				}
				else
				{
					if(bUseDefault)
					{
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP4 %d.%d.%d.%d/%d\r\n",GETBYTE(uiMulticastIP[0],0),GETBYTE(uiMulticastIP[0],1),GETBYTE(uiMulticastIP[0],2),GETBYTE(uiMulticastIP[0],3),nMulticastTTL == 0?255:nMulticastTTL);
					}
					else
					{
						nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"c=IN IP4 %d.%d.%d.%d/%d\r\n",GETBYTE(ptParseUrl->destIPA[0],0),GETBYTE(ptParseUrl->destIPA[1],1),GETBYTE(ptParseUrl->destIPA[2],2),GETBYTE(ptParseUrl->destIPA[3],3),ptParseUrl->nTTL == 0?255:ptParseUrl->nTTL);
					}
				}



            }

        }
#endif
        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:%sctype=audio\r\n",ptParseUrl->bHasParam?"":"?");

        if (ptParseUrl->bRecording)
        {
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=x-onvif-track:AUDIO001\r\n");
        }
        nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=recvonly\r\n");
    }
    if (m_pRequire != NULL)
    {
        //
        if(m_nRequireType & 1)
        {

            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"m=video %d RTP/AVP %d\r\n",0,m_nRequireVideoType);
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=rtpmap:%d %s/%d/1\r\n",m_nRequireVideoType,
                                 m_pRtspServer->GetPayloadTypeName(m_nRequireVideoType),
                                 m_pRtspServer->GetPayloadClockRate(m_nRequireVideoType));
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:%sctype=videoback\r\n",ptParseUrl->bHasParam?"":"?");




            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=sendonly\r\n");
        }

        if(m_nRequireType & 2)
        {

            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"m=audio %d RTP/AVP %d\r\n",0,m_nRequireAudioType);
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=rtpmap:%d %s/%d/1\r\n",m_nRequireAudioType,
                m_pRtspServer->GetPayloadTypeName(m_nRequireAudioType),
                m_pRtspServer->GetPayloadClockRate(m_nRequireAudioType));
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:%sctype=audioback\r\n",ptParseUrl->bHasParam?"":"?");

            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=sendonly\r\n");
        }


    }
    // APP

    for (int nApp = 0; nApp < RTSP_APP_SUPPORT_MAX_NUM; nApp++)
    {
        if (m_pAppMgr[nApp] != NULL)
        {

            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"m=application %d RTP/AVP %d\r\n",0,m_pAppMgr[nApp]->byAppPayloadType);
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=rtpmap:%d %s/%d\r\n",m_pAppMgr[nApp]->byAppPayloadType,
                                 m_pAppMgr[nApp]->szAppName,m_pAppMgr[nApp]->dwClockRate);
            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=control:%sctype=app%d\r\n",ptParseUrl->bHasParam?"":"?",m_pAppMgr[nApp]->byAppPayloadType);

            nContext += snprintf(Context + nContext,ANTS_RTSP_TMP_BUFSIZE - nContext,"a=recvonly\r\n");
        }
    }



    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-Type: application/sdp\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-Base: %s/\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][1]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-Length: %d\r\n",nContext);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Cache-Control: no-cache\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s",Context);

    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);
#if 0
    if(pBuffer != NULL)
    {
        delete []pBuffer;
        pBuffer = NULL;
    }
#endif
    m_bPlayReady = 1;
    return 0;
}

int CClientSocket::Setup_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0,bExist = 0;
    int rand_num;
    int nLine,nSeg;
    time_t tim;
    char *buffer = NULL;
    int control = -1;
    char *pstr;
    int nSessionID = 0;
    int nTransPort[2]= {0,0}; //
    int nInterleaved[2] = {0,0};

    int bTransType = 0;
    int nSeq = 0;
    int handle = -1;
    int ServerBasePort;
    int nStreamType = 3;
    int bRealMedia = 0;
    int nRet;
    unsigned int uiSSRC = 0;
    int bTCPMode = 0,bUDPMode = 0,bMultiMode = 0;
    CRtpSessionMgr *pRtpSess = NULL;
    RTSP_URL_T *ptParseUrl = NULL;
    int nAppIdx = -1;
    CRtspTmpObject tTmpObject;
	unsigned int dwExtSSRC = 0;
	int bExtSSRC = 0;
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    ptParseUrl = (RTSP_URL_T*)malloc(sizeof(RTSP_URL_T));
    tTmpObject.SetPt(ptParseUrl);
    if (ptParseUrl == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_MallocError);
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_MallocError);
        return -1;
    }
    time(&tim);

    buffer[0] = 0;

    //time(&m_tLastRefreshTime);
    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
    OutPutSocketRefreshTime(m_tLastRefreshTime);






    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);
            }



        }
#if 0
        if (m_pUserName == NULL && (!pMsg->StrCmp("Authorization:",nLine,0)))
        {
            if (!pMsg->StrCmp("Basic",nLine,1))
            {
                char *pAuth;
                pAuth = pMsg->GetStr(nLine,2);
                if(pAuth != NULL)
                {
                    unsigned int nLen = strlen(pAuth) + 1;
                    char *pUserPwd = (char *)Ants_Rtsp_Base64Decode(pAuth,nLen,NULL);
                    int nStrPos = 0;

                    if (pUserPwd != NULL)
                    {
                        char *pPwd = strchr(pUserPwd,':');
                        if (pPwd != NULL)
                        {
                            pPwd[0] = 0;
                            pPwd++;
                        }
                        m_pUserName = strdup(pUserPwd);
                        m_pPassword = strdup(pPwd);
                    }
                }
            }
        }
#endif
        if (!pMsg->StrCmp("Transport:",nLine,0))
        {
            //

            for (nSeg = 0; nSeg < pMsg->m_nSegCnt[nLine]; nSeg++)
            {
                pstr = pMsg->GetStr(nLine,nSeg);
                pstr = pMsg->StrStr("interleaved=",nLine,nSeg);
                if (pstr != NULL)
                {
                    int n;
                    n = sscanf(pstr,"interleaved=%d-%d",&nInterleaved[0],&nInterleaved[1]);
                    if (n != 2)
                    {
                        n = sscanf(pstr,"interleaved=%d",&nInterleaved[0]);
                        if (n == 1)
                        {
                            nInterleaved[1] = nInterleaved[0] + 1;
                        }
                    }
                }
                pstr = pMsg->StrStr("client_port=",nLine,nSeg);
                if (pstr != NULL)
                {
                    int n;
                    n = sscanf(pstr,"client_port=%d-%d",&nTransPort[0],&nTransPort[1]);
                    if (n != 2)
                    {
                        n = sscanf(pstr,"client_port=%d",&nTransPort[0]);
                        if (n == 1)
                        {
                            nTransPort[1] = nTransPort[0] + 1;
                        }
                    }
                }
                else
                {
                    pstr = pMsg->StrStr("port=",nLine,nSeg);
                    if (pstr != NULL)
                    {
                        int n;
                        n = sscanf(pstr,"port=%d-%d",&nTransPort[0],&nTransPort[1]);
                        if (n != 2)
                        {
                            n = sscanf(pstr,"port=%d",&nTransPort[0]);
                            if (n == 1)
                            {
                                nTransPort[1] = nTransPort[0] + 1;
                            }
                        }
                    }
                }

                pstr = pMsg->StrStr("RTP/AVP/TCP",nLine,nSeg);
                if (pstr != NULL)
                {
                    bTransType = 3;//TCP
                    bTCPMode = 1;


                }
                else
                {
                    pstr = pMsg->StrStr("RTP/AVP/UDP",nLine,nSeg);
                    if (pstr != NULL)
                    {
                        bTransType = 0;//UDP
                        bUDPMode = 1;


                    }
                    else
                    {
                        pstr = pMsg->StrStr("RTP/AVP",nLine,nSeg);
                        if (pstr != NULL)
                        {
                            bTransType = 0;//UDP
                            bUDPMode = 1;


                        }
                    }
                }
				pstr = pMsg->StrStr("ssrc=",nLine,nSeg);
				if (pstr != NULL)
				{

					dwExtSSRC = 0;
					if(1 == sscanf(pstr,"ssrc=%x",&dwExtSSRC))
					{
						bExtSSRC = 1;
					}
				}





                pstr = pMsg->StrStr("multicast",nLine,nSeg);
                if (pstr != NULL)
                {
                    if (bTransType == 0)
                    {
                        bTransType = 2;//多播
                    }
                    bMultiMode = 1;
                }
            }


        }
        else if (!pMsg->StrCmp("Session:",nLine,0))
        {
            int n;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                n = sscanf(pstr,"%d",&nSessionID);
            }

        }
        else if (!pMsg->StrCmp("User-Agent:",nLine,0))
        {
            pstr = pMsg->StrStr("RealMedia",nLine,1);
            if (pstr != NULL)
            {
                bRealMedia = 1;
            }
        }
    }
    nRet = Ants_Rtsp_ParseUrl(pMsg->GetStr(0,1),ptParseUrl);
    if (nRet)
    {
        //错误
        RTSP_DEBUG("!!!!!!!!!!!!!!\n");
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 451 Parameter Not Understood\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);

       SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete [] buffer;
        return 0;
    }
    if (ptParseUrl->nCustom == 1 && m_pRtspServer->IsOldRecording())
    {
        ptParseUrl->bRecording = 1;
    }
#if 1

    if(bTCPMode)
    {
        if(m_ptype == 2)
        {
            // TCP
            bTransType = 3;
        }
        if (m_ptype == 3)
        {
            if (m_bOverHTTP)
            {
                bTransType = 4;
            }
        }
    }
    if(bUDPMode)
    {
        if(m_ptype < 2)
        {
            // TCP
            bTransType = 0;

        }

    }
    if (m_stype == 2)
    {
        if (bMultiMode)
        {
            bTransType = 2;
        }
    }
#endif
    if (nInterleaved[0] == 0 && nInterleaved[1] == 0)
    {
        nInterleaved[0] = m_nInterleaved;
		nInterleaved[1] = nInterleaved[0] + 1;
        m_nInterleaved = nInterleaved[1] + 1;
	}
	else
	{
        m_nInterleaved = nInterleaved[1] + 1;
	}

    RTSP_DEBUG("bTransType = %d, %d-%d ,%d-%d\n",bTransType,nTransPort[0] ,nTransPort[1],nInterleaved[0],nInterleaved[1] );
    if ((bTransType < 3 && (nTransPort[0] <= 0 || nTransPort[1] !=nTransPort[0] +1)) ||
        (bTransType >= 3 && (nInterleaved[1] != nInterleaved[0] + 1)))
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 451 Parameter Not Understood\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);

       SetNeedClose(ANTS_RTSPSERVER_ERROR_InvTransPort);
        //delete [] buffer;
        return 0;
    }
    if (bTransType == 3 && m_bOverHTTP)
    {
        bTransType = 4;
    }

    control = m_ptype == 2;
    RTSP_DEBUG("%d, %d ,%d\n",ptParseUrl->ptype,bTransType,m_bOverHTTP);
    if (m_ptype == 2)
    {
        //tcp
        if (bTransType != 3)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 461 Unsupported transport\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes,0);

            SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
            //delete [] buffer;
            return 0;
        }
    }
    else if (bTransType != 4 && m_ptype == 3)
    {
        //http
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 461 Unsupported transport\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);

        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete [] buffer;
        return 0;
    }
    else if (bTransType != 2 && m_stype == 2)
    {
        //多播
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 461 Unsupported transport\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);

        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete [] buffer;
        return 0;
    }

#if 0

    handle = m_pRtspServer->LookupParam(ptParseUrl->pStreamFileName,&pRtpSess);
    if (handle < 0)
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        SendData(buffer,nBytes);
        m_bNeedClose = 1;
        delete [] buffer;
        return 0;
    }

    pRtpSess = m_pRtpSess;
    handle = m_hRtpSess;
    if (pRtpSess == NULL)
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes);
        m_bNeedClose = 1;
        //delete [] buffer;
        return 0;
    }
#endif
    if (bTransType == 2)
    {
        // 多播
        if(!(m_pRtspServer->GetSupportMulticast(-1) && GetSupportMulticast()))
        {
            // 不支持多播
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 461 Unsupported transport\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes,0);

            SetNeedClose(ANTS_RTSPSERVER_ERROR_InvalidMulticast);
            //delete [] buffer;
            return 0;
        }
    }



    nStreamType = m_pRtspServer->GetStreamType(handle);
    if (nStreamType == 0)
    {
        nStreamType = 3;
    }

#if 0
    if(m_nSessionID == 0)
    {
        srand(tim + m_nSessionCnt);
        rand_num = (int)(255.0*rand()/(RAND_MAX+1.0));
        rand_num &= 0xFF;
        m_nSessionCnt++;
        m_nSessionCnt &= 0xFF;

        //产生ID
        m_nSessionID = ((tim & 0x7FFF) << 16) | m_nSessionCnt |(rand_num << 8) ;
    }
#endif

    if (m_bSetup && nSessionID != m_nSessionID)
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 454 Session Not Found\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);

        SetNeedClose(ANTS_RTSPSERVER_ERROR_InvSessionID);
        //delete [] buffer;
        return 0;
    }


    m_bTransType = bTransType;
    bExist = 0;

    if (m_bTransType == 3 || m_bTransType == 4)
    {
        m_bTCP = 1;
    }

    if (m_pRtpSess == NULL)
    {
	    m_pRtspServer->GLock();

        if (ptParseUrl->bRecording)
        {
            // 需要创建流 token="recording" + clientID
            m_dwRecordCreateCnt++;
            sprintf(buffer,"Recording_%d_%u_%d",m_dwRecordCreateCnt,m_nSessionID,m_bIPv6);
            m_hRtpSess = m_pRtspServer->CreateStream(buffer,&bExist,m_bTCP,m_StreamType,m_bPlayloadType[0],m_bPlayloadType[1],0,m_bIPv6);
            m_pRtpSess = m_pRtspServer->GetSessionMgrByHandle(m_hRtpSess);
            if (m_pRtpSess == NULL)
            {
                m_pRtspServer->GUnlock();
                nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                                   "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                                   nSeq);
                nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
                nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
                nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
                SendDataV2(buffer,nBytes,0);
               //delete []buffer;

                SetNeedClose(ANTS_RTSPSERVER_ERROR_ResError);
                return 0;
            }
#if 1
            if (m_bTransType == 0 || m_bTransType == 1 || m_bTransType == 2)
            {
                if (0 == m_pRtpSess->GetCurSendSessionID())
                {
                    SetSendUDPFlag(1);
					m_pRtpSess->SetCurSendSessionID(m_nSessionID);
                }
            }
#endif
            m_pRtpSess->SetConfig(6,(void *)1,0);
            m_tRecStart = ptParseUrl->tStart;
            m_tRecStop = ptParseUrl->tStop;
            m_bCreateRecordStream = 1;
            m_bRecording = 1;
            handle = m_hRtpSess;
            pRtpSess = m_pRtpSess;
            m_stype = ptParseUrl->stype;
            m_ptype = ptParseUrl->ptype;
        }
        else
        {
            if (m_nRequireType != 0)
            {
                if (m_nCh < 0)
                {
                    m_pRtspServer->GUnlock();
                    nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                                       "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                                       nSeq);
                    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
                    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
                    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
                    SendDataV2(buffer,nBytes,0);
                    //delete []buffer;

                    SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
                    return 0;
                }

                m_bTCP += 2;
                sprintf(buffer,"back%02d_%u_%d",m_nCh,m_nSessionID,m_bIPv6);
                RTSP_DEBUG("buffer = %s!!!!!!!!!!!!!!!!!!!!!!!!!\n", buffer);
            }
            else
            {
                if (m_nCh < 0 || m_nStream < 0)
                {
                    m_pRtspServer->GUnlock();
                    nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                                       "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                                       nSeq);
                    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
                    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
                    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
                    SendDataV2(buffer,nBytes,0);
                    //delete []buffer;

                    SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
                    return 0;
                }

                if (m_bTCP == 1 || m_bAntsComb == 1 || m_bTransType == 0)
                {
                    sprintf(buffer,"%s_%u_%d",ptParseUrl->pStreamFileName,m_nSessionID,m_bIPv6);
                }
                else
                {// 广播/多播
                    sprintf(buffer,"%s_%d",ptParseUrl->pStreamFileName,m_bIPv6);
                }
            }

            m_hRtpSess = m_pRtspServer->CreateStream(buffer,&bExist,m_bTCP,m_StreamType,m_bPlayloadType[0],m_bPlayloadType[1],0,m_bIPv6);
            m_pRtpSess = m_pRtspServer->GetSessionMgrByHandle(m_hRtpSess);

            if (m_pRtpSess == NULL)
            {
                m_pRtspServer->GUnlock();
                RTSP_DEBUG("buffer = %s!!!!!!!!!!!!!!!!!!!!!!!!!\n", buffer);
                nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                                   "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                                   nSeq);
                nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
                nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
                nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
                SendDataV2(buffer,nBytes,0);
               // delete []buffer;

                SetNeedClose(ANTS_RTSPSERVER_ERROR_ResError);
                return 0;
            }
#if 1
            if (m_bTransType == 0 || m_bTransType == 1 || m_bTransType == 2)
            {
                if (0 == m_pRtpSess->GetCurSendSessionID())
                {
                    SetSendUDPFlag(1);
					m_pRtpSess->SetCurSendSessionID(m_nSessionID);
                }
            }
#endif
            m_bCreateRecordStream = 1;
            handle = m_hRtpSess;
            pRtpSess = m_pRtpSess;
            m_stype = ptParseUrl->stype;
            m_ptype = ptParseUrl->ptype;
        }

		m_pRtspServer->GUnlock();

        if (bExist == 0)
        {
            Ants_RTSPV2_InParam_T tInParam;
            Ants_RTSPV2_OutParam_T tOutParam;
            char *pUrl,*pUrl_In;
            int nInPos = 0;
            memset(&tOutParam,0,sizeof(tOutParam));
            memset(&tInParam,0,sizeof(tInParam));
            pUrl = pMsg->GetStr(0,1);
            if(pUrl != NULL)
            {
                if (m_pCurrUrl != NULL)
                {
                    free(m_pCurrUrl);
                    m_pCurrUrl = NULL;
                }
                m_pCurrUrl = strdup(pUrl);
            }
            if(m_pRequire != NULL)
            {
                free(m_pRequire);
                m_pRequire = NULL;
            }
            pUrl_In = new char[256];
            m_pRequire = pMsg->GetItem("Require:");
            if (pUrl_In != NULL)
            {
                char strans[][10]={"udp","boardcast","multicast","tcp","http"};
                nInPos += sprintf(pUrl_In+nInPos,"%s",pUrl);
                if (!ptParseUrl->bHasParam)
                {

                    nInPos += sprintf(pUrl_In + nInPos,"?");
                }
                //
                if(m_bTransType >= 0 && m_bTransType <= 4)
                {
                    nInPos += sprintf(pUrl_In + nInPos,"&transtype=%s",strans[m_bTransType]);
                }

                if (m_pUserName != NULL)
                {
                    nInPos += sprintf(pUrl_In+nInPos,"&user=%s",m_pUserName);
                }
                if (m_pPassword != NULL)
                {
                    nInPos += sprintf(pUrl_In+nInPos,"&pwd=%s",m_pPassword);
                }
            }

            tInParam.pUrl = pUrl_In;
            tInParam.wRemotePort = m_uPort;
            tInParam.dwSessionID = m_nSessionID;
            tInParam.pRequire = m_pRequire;
			tInParam.nAntsComb = ptParseUrl->bAntsComb;
            tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
			tInParam.bIPv6 = m_bIPv6;
			if (m_bIPv6)
			{
				memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
			}
			else
			{
				tInParam.dwClientIP = m_dwIPAddress[0];

			}
            if (ptParseUrl->bRecording)
            {
                tInParam.bRecording = 1;
                tInParam.nChan = ptParseUrl->nCh;
                tInParam.nStream = ptParseUrl->nStream;
                tInParam.bRangeValid = ptParseUrl->bTimeValid;
                tInParam.tRangeStart = ptParseUrl->tStart;
                tInParam.tRangeStop = ptParseUrl->tStop;
            }

            if (0 == m_pRtspServer->fOpen(m_hRtpSess,&tInParam,&tOutParam,&pStreamUser))
            {
                m_bModeOpen = 1;
                m_pRtpSess->SetStreamUser(pStreamUser);
            }
			else
			{
                nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                                   "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                                   nSeq);
                nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
                nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
                nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
                SendDataV2(buffer,nBytes,0);
               // delete []buffer;

                SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
				if (pUrl_In)
				{
					free(pUrl_In);
					pUrl_In = NULL;
				}
                return 0;
			}

            if (pUrl_In)
            {
                free(pUrl_In);
                pUrl_In = NULL;
            }
        }
		else
		{
            m_bModeOpen = 1;
		}

        //if (m_pRtpSess != NULL)
        //{
         //   m_bUsed = 1;
        //}
    }

    pRtpSess = m_pRtpSess;
    handle = m_hRtpSess;

	if (m_bAntsComb)
	{
		pRtpSess->SetCombFlag(1);
	}

    if (pStreamUser == NULL)
    {
        pStreamUser = m_pRtpSess->GetStreamUser();
    }

    //先视频,后音频
    if(ptParseUrl->ctype <= 1)
    {
        m_bSetup |= 1;


        ServerBasePort = m_pRtspServer->GetServerBasePortByHandle(handle);
        uiSSRC = m_pRtspServer->GetSSRCByHandle(handle,0);
		if(bExtSSRC)
		{
			uiSSRC = dwExtSSRC;
			m_pRtspServer->SetSSRCByHandle(handle,0,uiSSRC);
		}

    }
    else if (ptParseUrl->ctype == 2)
    {
        //音频
        m_bSetup |= 2;

        ServerBasePort = m_pRtspServer->GetServerBasePortAByHandle(handle);
        uiSSRC = m_pRtspServer->GetSSRCByHandle(handle,1);
		if(bExtSSRC)
		{
			uiSSRC = dwExtSSRC;
			m_pRtspServer->SetSSRCByHandle(handle,1,uiSSRC);
		}
    }
    else if (ptParseUrl->ctype == 3)
    {
        //videoback
        m_bSetup |= 4;
        if(CreateVideo())
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 500 Internal Server Error\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes,0);
            //m_pRtpSess->SetSetup(0);
            SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
            //delete [] buffer;
            return 0;
        }

        ServerBasePort = m_nVideoBasePort;
        if (m_pVideoSession != NULL)
        {
            uiSSRC = m_pVideoSession->GetLocalSSRC();
        }
    }
    else if (ptParseUrl->ctype == 4)
    {
        //audioback
        m_bSetup |= 8;
        if (CreateAudio())
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 500 Internal Server Error\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes,0);
            //m_pRtpSess->SetSetup(0);
            SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
            //delete [] buffer;
            return 0;
        }

        ServerBasePort = m_nAudioBasePort;
        if (m_pAudioSession != NULL)
        {
            uiSSRC = m_pAudioSession->GetLocalSSRC();
        }
    }
    else if (ptParseUrl->ctype == 5)
    {
        int nIdx = -1;
        nAppIdx = GetAppIndexByPayloadType(ptParseUrl->byAppPayloadType);
        nIdx = m_pRtpSess->AddAppStream(m_pAppMgr[nAppIdx]->byAppPayloadType, m_pAppMgr[nAppIdx]->dwClockRate, m_pAppMgr[nAppIdx]->szAppName);
        if (nIdx == -1)
        {
            nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                               "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                               nSeq);
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
            nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
            nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
            //SendData(buffer,nBytes);
            SendDataV2(buffer,nBytes,0);
            //m_pRtpSess->SetSetup(0);
            SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
            //delete [] buffer;
            return 0;
        }
        m_bSetup |= (1 << (3 + 1 + nAppIdx));

        ServerBasePort = m_pRtspServer->GetServerBaseAppPortByHandle(handle,nIdx);
        uiSSRC = m_pRtspServer->GetAppSSRCByHandle(handle,nIdx);

        {
            Ants_RTSPV2_InParam_T tInParam;
            char *pUrl;
            memset(&tInParam,0,sizeof(tInParam));
            pUrl = pMsg->GetStr(0,1);
            if(pUrl != NULL)
            {
                if (m_pCurrUrl != NULL)
                {
                    free(m_pCurrUrl);
                    m_pCurrUrl = NULL;
                }
                m_pCurrUrl = strdup(pUrl);
            }
            if(m_pRequire != NULL)
            {
                free(m_pRequire);
                m_pRequire = NULL;
            }
            m_pRequire = pMsg->GetItem("Require:");
            tInParam.pUrl = m_pCurrUrl;
            tInParam.wRemotePort = m_uPort;
            tInParam.dwSessionID = m_nSessionID;
            tInParam.pRequire = m_pRequire;
			tInParam.nAntsComb = ptParseUrl->bAntsComb;
            tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
            tInParam.nAppPayloadType = m_pAppMgr[nAppIdx]->byAppPayloadType;
			if (m_bIPv6)
			{
				memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
			}
			else
			{
				tInParam.dwClientIP = m_dwIPAddress[0];
			}
            if (ptParseUrl->bRecording)
            {
                tInParam.bRecording = 1;
                tInParam.nChan = ptParseUrl->nCh;
                tInParam.nStream = ptParseUrl->nStream;
                tInParam.bRangeValid = ptParseUrl->bTimeValid;
                tInParam.tRangeStart = ptParseUrl->tStart;
                tInParam.tRangeStop = ptParseUrl->tStop;
            }

            if (m_bModeOpen)
            {
                m_pRtspServer->fControl(m_hRtpSess,ANTS_RTSPSERVER_CALLBACK_TYPE_SET_APP,&tInParam,NULL,pStreamUser);
            }
        }
    }








    //会话ID是否已经产生。在有多个码流时会多次SETUP。
    //m_hRtpSess = handle;
    //m_pRtpSess = pRtpSess;

    //获取客户连接类型，及端口.服务器端口动态生成。UDP查询一对可用端口对;TCP使用当前的RTSP连接,交错方式





    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Server: Ants Rtsp Server/1.0\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));

    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d",m_nSessionID);
    if (m_pRtspServer->GetStreamTimeOut(handle))
    {
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"; timeout=%d",m_pRtspServer->GetStreamTimeOut(handle));
    }

    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");

    if (m_bTransType == 3 ||
        m_bTransType == 4)
    {

        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d;ssrc=%X",
                           nInterleaved[0],nInterleaved[1],uiSSRC);
        if(ptParseUrl->ctype == 1 ||
           ptParseUrl->ctype == 0)
        {

            m_nPort_VA[0] = nInterleaved[0];
        }
        else if (ptParseUrl->ctype == 2)
        {
            m_nPort_VA[1] = nInterleaved[0];
        }
        else if (ptParseUrl->ctype == 3)
        {
            m_nPort_VA[2] = nInterleaved[0];
        }
        else if (ptParseUrl->ctype == 4)
        {
            m_nPort_VA[3] = nInterleaved[0];
        }
        else if (ptParseUrl->ctype == 5)
        {
            m_nPort_VA[4 + nAppIdx] = nInterleaved[0];
        }


    }
    else if(m_bTransType == 0)
    {
        //UDP

        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d;ssrc=%X",
                           nTransPort[0],nTransPort[1],ServerBasePort,ServerBasePort+1,uiSSRC);

        if(ptParseUrl->ctype == 1 ||
           ptParseUrl->ctype == 0)
        {
            //视频
			GetIPv6((unsigned char *)m_dwIPAdress_VA[0]);
            m_nPort_VA[0] = nTransPort[0];

        }
        else if (ptParseUrl->ctype == 2)
        {
            //音频
			GetIPv6((unsigned char *)m_dwIPAdress_VA[1]);
            m_nPort_VA[1] = nTransPort[0];
        }
        else if (ptParseUrl->ctype == 3)
        {
            //音频

			GetIPv6((unsigned char *)m_dwIPAdress_VA[2]);
            m_nPort_VA[2] = nTransPort[0];
        }
        else if (ptParseUrl->ctype == 4)
        {
            //音频

			GetIPv6((unsigned char *)m_dwIPAdress_VA[3]);
            m_nPort_VA[3] = nTransPort[0];
        }
        else if (ptParseUrl->ctype == 5)
        {

			GetIPv6((unsigned char *)m_dwIPAdress_VA[4 + nAppIdx]);
            m_nPort_VA[4 + nAppIdx] = nTransPort[0];
        }

    }
    else if (m_bTransType == 2)
    {
        //多播
#if 1
        int bMulticast = 0,bUseDefault =0;
		unsigned int uiMulticastIP[4] = {0,0,0,0};
        int nMulticastPort = 0,nMulticastTTL = 255;
		unsigned int dwLocalIP[4] = {0,0,0,0};
        if(ptParseUrl->ctype < 3)
        {
            if(GetMulticastIPv4(&uiMulticastIP[0],&nMulticastPort,&nMulticastTTL,ptParseUrl->ctype == 2) >= 0)
            {
                bMulticast = 1;
				if (ptParseUrl->ctype == 2)
				{
			        if (m_pRtpSess != NULL)
			        {
			            if (m_pRtpSess->GetAudio() != NULL)
			            {
			                m_pRtpSess->GetAudio()->SetMulticastTTL(nMulticastTTL);
			            }
			        }
				}
				else
				{
			        if (m_pRtpSess != NULL)
			        {
			            if (m_pRtpSess->GetVideo() != NULL)
			            {
			                m_pRtpSess->GetVideo()->SetMulticastTTL(nMulticastTTL);
			            }
			        }
				}
            }
        }
		int bLocalIPv6 = 0;
			if(0)
         {

             struct addrinfo *pResult = NULL,*pNext = NULL;
             if(0 == getaddrinfo(ptParseUrl->pAddress,NULL, NULL, &pResult ))
             {
                 pNext = pResult;
                 while (pNext != NULL)
                 {
                     if (pNext->ai_family == AF_INET)
                     {

							 //RTSP_DEBUG("officail name : %s\n", inet_ntoa((struct in_addr)((struct sockaddr_in *)pNext->ai_addr)->sin_addr));
							 //RTSP_DEBUG("officail name : %s\n", pNext->ai_canonname );
							 dwLocalIP[0] = ((struct in_addr)((struct sockaddr_in *)pNext->ai_addr)->sin_addr).s_addr;
							 RTSP_DEBUG("officail name : %d.%d.%d.%d\n", dwLocalIP[0]&0xFF,(dwLocalIP[0]>>8)&0xFF,(dwLocalIP[0]>>16)&0xFF,(dwLocalIP[0]>>24)&0xFF);

                         break;

                     }
					 else if(pNext->ai_family == AF_INET6)
					 {
						 char szStringIPv6[128];
						 memcpy(dwLocalIP,&((struct sockaddr_in6 *)pNext->ai_addr)->sin6_addr,16);
						 inet_ntop(AF_INET6,dwLocalIP,szStringIPv6,128);

						 RTSP_DEBUG("officail name : %s\n", szStringIPv6);
						 break;

					 }
                     pNext = pNext->ai_next;
                 }

                 freeaddrinfo(pResult);


             }



         }
			else
			{
				GetIPv6((unsigned char *)dwLocalIP);
			}
		 if(m_bIPv6)
		 {
			 if (dwLocalIP[0] != 0 || dwLocalIP[1] != 0 ||dwLocalIP[2] != 0 ||dwLocalIP[3] != 0||
				 dwLocalIP[0] != -1 ||dwLocalIP[1] != -1 ||dwLocalIP[2] != -1 ||dwLocalIP[3] != -1)
			 {
				  char szStringIPv6[128],szStringIPv6_1[128];
				   inet_ntop(AF_INET6,dwLocalIP,szStringIPv6,128);
				   inet_ntop(AF_INET6,uiMulticastIP,szStringIPv6_1,128);
				 nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;multicast;destination=%s;source==%s;port=%d-%d;ttl=%u;ssrc=%X",
					 szStringIPv6_1,
					 szStringIPv6,
					 nMulticastPort,nMulticastPort + 1,nMulticastTTL,uiSSRC);
			 }
			 else
			 {
				  char szStringIPv6[128];
				   inet_ntop(AF_INET6,uiMulticastIP,szStringIPv6,128);
				 nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;multicast;destination=%s;port=%d-%d;ttl=%u;ssrc=%X",
					szStringIPv6,
					 nMulticastPort,nMulticastPort + 1,nMulticastTTL,uiSSRC);
			 }
		 }
		 else
		 {
		   if (dwLocalIP[0] != 0 && dwLocalIP[0] != -1)
		   {
				nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;multicast;destination=%u.%u.%u.%u;source=%u.%u.%u.%u;port=%d-%d;ttl=%u;ssrc=%X",
					GETBYTE(uiMulticastIP[0],0),GETBYTE(uiMulticastIP[0],1),GETBYTE(uiMulticastIP[0],2),GETBYTE(uiMulticastIP[0],3),
					dwLocalIP[0]&0xFF,(dwLocalIP[0] >> 8)&0xFF,(dwLocalIP[0]>>16)&0xFF,(dwLocalIP[0]>>24)&0xFF,
					nMulticastPort,nMulticastPort + 1,nMulticastTTL,uiSSRC);

		   }
		   else
		   {
			   nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;multicast;destination=%u.%u.%u.%u;port=%d-%d;ttl=%u;ssrc=%X",
				   GETBYTE(uiMulticastIP[0],0),GETBYTE(uiMulticastIP[0],1),GETBYTE(uiMulticastIP[0],2),GETBYTE(uiMulticastIP[0],3),
				   nMulticastPort,nMulticastPort + 1,nMulticastTTL,uiSSRC);
		   }
		}

        if(ptParseUrl->ctype == 1 ||
           ptParseUrl->ctype == 0)
        {
            //视频
			memcpy(m_dwIPAdress_VA[0],uiMulticastIP,16);
            m_nPort_VA[0] = nMulticastPort;

        }
        else if (ptParseUrl->ctype == 2)
        {
            //音频
           memcpy(m_dwIPAdress_VA[1],uiMulticastIP,16);
            m_nPort_VA[1] = nMulticastPort;
        }
        else if (ptParseUrl->ctype == 3)
        {
            //videoback
            memcpy(m_dwIPAdress_VA[2],uiMulticastIP,16);
            m_nPort_VA[2] = nMulticastPort;
        }
        else if (ptParseUrl->ctype == 4)
        {
            //audioback
            memcpy(m_dwIPAdress_VA[3],uiMulticastIP,16);
            m_nPort_VA[3] = nMulticastPort;
        }
        else if (ptParseUrl->ctype == 5)
        {
			memcpy(m_dwIPAdress_VA[4 + nAppIdx],uiMulticastIP,16);
            m_nPort_VA[4 + nAppIdx] = nMulticastPort;
        }
#else
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;multicast;client_port=%d-%d;server_port=%d-%d;ttl=255",
                           nTransPort[0],nTransPort[1],ServerBasePort,ServerBasePort+1);

        if(ptParseUrl->ctype == 1 ||
           ptParseUrl->ctype == 0)
        {
            //视频
            m_dwIPAdress_VA[0] = ptParseUrl->destIPV;
            m_nPort_VA[0] = nTransPort[0];

        }
        else if (ptParseUrl->ctype == 2)
        {
            //音频
            m_dwIPAdress_VA[1] = ptParseUrl->destIPA;
            m_nPort_VA[1] = nTransPort[0];
        }
#endif
    }
    else
    {


        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Transport: RTP/AVP;client_port=%d;server_port=%d-%d;ssrc=%X",
                           nTransPort[0],ServerBasePort,ServerBasePort+1,uiSSRC);

    }

    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行

    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行

    //  RTSP_DEBUG("Setup Send size = %d\n",nBytes);
    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);
    //delete [] buffer;
    return 0;
}
int CClientSocket::Play_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0;
    time_t tim;
    int handle = -1;
    int nLine,nSeq = 0;
    char *pstr = NULL;
    int nSessionID = 0;
    char *buffer = NULL;
    CRtpSessionMgr *pRtpSess = NULL;
    Ants_RtspDayTime tStart,tStop;
    int bTimeValid = 0;
    int nFrame = 0,nFrameMsec = 0;
    int nScale = 0,bScale = 0;
    int bRateControl = 1;
    int bNPT = 0,nNptStart = 0,nNptStop = 0,bClock = 0;
    memset(&tStart,0,sizeof(tStart));
    memset(&tStop,0,sizeof(tStop));
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
       SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        return -1;
    }

    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }



        }
        else if (!pMsg->StrCmp("Session:",nLine,0))
        {
            int n;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                n = sscanf(pstr,"%d",&nSessionID);
            }

        }
        else if (!pMsg->StrCmp("Frames:",nLine,0))
        {
            int n;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                if (strnicmp(pstr,"intra",5) == 0)
                {
                    nFrame = 1;
                    nFrameMsec = atoi(pstr+6);
                }
                else if (strnicmp(pstr,"predicted",9) == 0)
                {
                    nFrame = 2;
                }
            }

        }
        else if (!pMsg->StrCmp("Scale:",nLine,0))
        {
            int n;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nScale = atoi(pstr);
            }
            bScale = 1;

        }
        else if (!pMsg->StrCmp("Rate-Control:",nLine,0))
        {
            int n;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                if (strstr(pstr,"no"))
                {
                    bRateControl = 0;
                }

            }

        }
        else if (!pMsg->StrCmp("Range:",nLine,0))
        {
            int n;
            char *pBar;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0,nMsec = 0;
                // 开始时间
                if (strnicmp(pstr,"clock=",6) == 0)
                {
                    bClock = 1;
                    n = sscanf(pstr,"clock=%04d%02d%02dT%02d%02d%02d.%dZ",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec,&nMsec);
                    if (n > 3)
                    {
                        bTimeValid |= 1;
                        tStart.wYear = nYear;
                        tStart.byMon = nMon;
                        tStart.byDay = nDay;
                        tStart.byHour = nHour;
                        tStart.byMin = nMin;
                        tStart.bySec = nSec;
                        tStart.wMsec = nMsec;
                    }
                    pBar = strchr(pstr,'-');

                    if (pBar != NULL)
                    {
                        n = sscanf(pBar,"-%04d%02d%02dT%02d%02d%02d.%dZ",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec,&nMsec);
                        if (n > 3)
                        {
                            bTimeValid |= 2;
                            tStop.wYear = nYear;
                            tStop.byMon = nMon;
                            tStop.byDay = nDay;
                            tStop.byHour = nHour;
                            tStop.byMin = nMin;
                            tStop.bySec = nSec;
                            tStop.wMsec = nMsec;
                        }
                    }
                }
                else if (strnicmp(pstr,"npt=",4) == 0)
                {
                    int nStart,nStop;
                    struct tm Localtime;
                    time_t tCurrTime;
                    bNPT = 1;
                    if(1 == sscanf(pstr,"npt=%d",&nStart))
                    {
                        bTimeValid |= 1;
                        Localtime.tm_year = m_tRecStart.wYear - 1900;
                        Localtime.tm_mon  = m_tRecStart.byMon  - 1;
                        Localtime.tm_mday = m_tRecStart.byDay ;
                        Localtime.tm_hour = m_tRecStart.byHour;
                        Localtime.tm_min  = m_tRecStart.byMin ;
                        Localtime.tm_sec  = m_tRecStart.bySec ;
                        tCurrTime = mktime(&Localtime);
                        tCurrTime += nStart;
                        Ants_rtsp_LocalTime_r(&tCurrTime,&Localtime);
                        tStart.wYear = Localtime.tm_year + 1900;
                        tStart.byMon = Localtime.tm_mon + 1;
                        tStart.byDay = Localtime.tm_mday;
                        tStart.byHour = Localtime.tm_hour;
                        tStart.byMin = Localtime.tm_min;
                        tStart.bySec = Localtime.tm_sec;
                        tStart.wMsec = 0;
                        nNptStart = nStart;
                    }
                    pBar = strchr(pstr,'-');
                    if (pBar != NULL)
                    {
                        n = sscanf(pBar,"-%d",&nStop);
                        if (n == 1)
                        {
                            bTimeValid |= 2;
                            Localtime.tm_year = m_tRecStop.wYear - 1900;
                            Localtime.tm_mon  = m_tRecStop.byMon  - 1;
                            Localtime.tm_mday = m_tRecStop.byDay ;
                            Localtime.tm_hour = m_tRecStop.byHour;
                            Localtime.tm_min  = m_tRecStop.byMin ;
                            Localtime.tm_sec  = m_tRecStop.bySec ;
                            tCurrTime = mktime(&Localtime);
                            tCurrTime += nStart;
                            Ants_rtsp_LocalTime_r(&tCurrTime,&Localtime);
                            tStop.wYear = Localtime.tm_year + 1900;
                            tStop.byMon = Localtime.tm_mon + 1;
                            tStop.byDay = Localtime.tm_mday;
                            tStop.byHour = Localtime.tm_hour;
                            tStop.byMin = Localtime.tm_min;
                            tStop.bySec = Localtime.tm_sec;
                            tStop.wMsec = 0;
                            nNptStop = nStop;
                        }
                    }
                }

            }

        }

    }
    time(&tim);
    handle = m_hRtpSess;
    pRtpSess = m_pRtpSess;
    if (handle < 0)
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,
                           "RTSP/1.0 404 Stream Not Found\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete[]buffer;
        return 0;
    }



    if (nSessionID != m_nSessionID)
    {
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,
                           "RTSP/1.0 454 Session Not Found\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
        //if (!m_bPlay)
        //{
        //    m_pRtpSess->SetSetup(0);
        //}
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete [] buffer;
        return 0;
    }


    if(!m_bPlay)
    {


        if (m_bSetup & 3)
        {

            //播放
            if (m_bTransType == 3 ||
                m_bTransType == 4)
            {
                //tcp
                if(m_bSetup &1)
                {
                    m_pRtspServer->AddRtpDataClient(m_hRtpSess,m_dwIPAddress,m_uPort,m_nPort_VA[0],0,1,this,m_bIPv6);
                }
                if(m_bSetup &2)
                {
                    m_pRtspServer->AddRtpDataClient(m_hRtpSess,m_dwIPAddress,m_uPort,m_nPort_VA[1],1,1,this,m_bIPv6);
                }
            }
            else
            {
                if(m_bSetup &1)
                {
                    m_pRtspServer->AddRtpDataClient(m_hRtpSess,m_dwIPAdress_VA[0],m_nPort_VA[0],0,0,0,this,m_bIPv6);
                }
                if(m_bSetup &2)
                {
                    m_pRtspServer->AddRtpDataClient(m_hRtpSess,m_dwIPAdress_VA[1],m_nPort_VA[1],0,1,0,this,m_bIPv6);
                }
            }
        }
        if ((m_bSetup & 4) && m_pVideoSession)
        {
            if (m_bTransType == 3 ||
                m_bTransType == 4)
            {
                //tcp

                m_pVideoSession->AddDestination(m_dwIPAddress,m_uPort,m_uPort + 1,1,m_nPort_VA[2],this,m_bIPv6);
            }
            else
            {
                m_pVideoSession->AddDestination(m_dwIPAdress_VA[2],m_nPort_VA[2],m_nPort_VA[2]+1,0,0,this,m_bIPv6);
            }
        }
        if ((m_bSetup & 8) && m_pAudioSession)
        {
            if (m_bTransType == 3 ||
                m_bTransType == 4)
            {
                //tcp

                m_pAudioSession->AddDestination(m_dwIPAddress,m_uPort,m_uPort+1,1,m_nPort_VA[3],this,m_bIPv6);
            }
            else
            {
                m_pAudioSession->AddDestination(m_dwIPAdress_VA[3],m_nPort_VA[3],m_nPort_VA[3]+1,0,0,this,m_bIPv6);
            }
        }
        for (int nAppIdx = 0; nAppIdx < RTSP_APP_SUPPORT_MAX_NUM; nAppIdx++)
        {
            if (m_bSetup & (1 << (4 + nAppIdx)))
            {
                CAppRtpSession *pAppSess;
                pAppSess = pRtpSess->GetAppSessionByType(m_pAppMgr[nAppIdx]->byAppPayloadType, m_pAppMgr[nAppIdx]->dwClockRate, m_pAppMgr[nAppIdx]->szAppName);
                if (pAppSess == NULL)
                {
                    continue;
                }
                if (m_bTransType == 3 ||
                    m_bTransType == 4)
                {
                    pAppSess->AddDestination(m_dwIPAddress,m_uPort,m_uPort+1,1,m_nPort_VA[4 + nAppIdx],this,m_bIPv6);
                }
                else
                {
                    pAppSess->AddDestination(m_dwIPAdress_VA[4 + nAppIdx],m_nPort_VA[4 + nAppIdx],m_nPort_VA[4 + nAppIdx]+1,0,0,this,m_bIPv6);
                }
            }
        }
        m_bPlay = 1;
        //m_pRtpSess->SetSetup(0);
    }









    //nContext += sprintf(Context + nContext,"v=0\r\n");



    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"CSeq: %d\r\n",nSeq);

    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"Session: %d\r\n",m_nSessionID);
#if 1
    if (m_bRecording)
    {
        if (bNPT)
        {
            if (bTimeValid)
            {
                if (bTimeValid&1)
                {
                    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Range: npt=%d-",nNptStart);
                    if (bTimeValid&2)
                    {
                        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d",nNptStop);
                    }
#if 0
                    else
                    {
                        struct tm tSaveTm;
                        time_t newTime,StopTime;
                        tSaveTm.tm_year = m_tRecStart.wYear - 1900;
                        tSaveTm.tm_mon = m_tRecStart.byMon - 1;
                        tSaveTm.tm_mday = m_tRecStart.byDay;
                        tSaveTm.tm_hour = m_tRecStart.byHour;
                        tSaveTm.tm_min = m_tRecStart.byMin;
                        tSaveTm.tm_sec = m_tRecStart.bySec;
                        newTime = mktime(&tSaveTm);
                        tSaveTm.tm_year = m_tRecStop.wYear - 1900;
                        tSaveTm.tm_mon = m_tRecStop.byMon - 1;
                        tSaveTm.tm_mday = m_tRecStop.byDay;
                        tSaveTm.tm_hour = m_tRecStop.byHour;
                        tSaveTm.tm_min = m_tRecStop.byMin;
                        tSaveTm.tm_sec = m_tRecStop.bySec;
                        StopTime = mktime(&tSaveTm);
                        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d",StopTime-newTime);
                    }
#endif
                    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");
                }
                //nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"RTP-Info: %d\r\n",m_nSessionID);
            }

        }
        //nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"a=range:clock=20140102T120000Z-20140102T130000Z\r\n");

    }

#endif




    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE-nBytes,"\r\n");//空行
    //nBytes += sprintf(buffer + nBytes,"%s\r\n",Context);
    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);
//强迫I帧
	{
		Ants_RTSPV2_InParam_T tInParam;
		Ants_RTSPV2_OutParam_T tOutParam;
		char *pUrl;
		memset(&tOutParam,0,sizeof(tOutParam));
		memset(&tInParam,0,sizeof(tInParam));

		tInParam.pUrl = &pMsg->m_pMessage[pMsg->m_nPos[0][2]];//m_pCurrUrl;
		tInParam.wRemotePort = m_uPort;
		tInParam.dwSessionID = m_nSessionID;
		m_nLastPlaySeq = nSeq;
		m_bRateControl = bRateControl;
		tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
		tInParam.bIPv6 = m_bIPv6;
		if (m_bIPv6)
		{
			memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
		}
		else
		{
			tInParam.dwClientIP = m_dwIPAddress[0];
		}
		if (m_bRecording)
		{
			tInParam.bRecording = 1;
			tInParam.bRangeValid = bTimeValid;
			tInParam.tRangeStart = tStart;
			tInParam.tRangeStop = tStop;
			tInParam.nFrame = nFrame;
			tInParam.nInterval = nFrameMsec;
			if (bScale)
			{
				m_nScale = nScale;
			}
			tInParam.nScale = m_nScale;
			tInParam.nPlaySeq = nSeq;
			tInParam.bRateControl = m_bRateControl;

			tInParam.nStreamType = m_bSetup & 3;


			if(nScale < 0)
			{
				m_pRtpSess->SetRecordExHeader_D(1);
			}
		}

		if (m_bModeOpen)
		{
			m_pRtspServer->fControl(m_hRtpSess,ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING,&tInParam,NULL,pStreamUser);
		}
	}

#if 1
	{
		Ants_RTSPV2_InParam_T tInParam;
		Ants_RTSPV2_OutParam_T tOutParam;
		memset(&tOutParam,0,sizeof(tOutParam));
		memset(&tInParam,0,sizeof(tInParam));
		tInParam.pUrl = m_pCurrUrl;
		tInParam.wRemotePort = m_uPort;
		tInParam.dwSessionID = m_nSessionID;
		tInParam.pRequire = m_pRequire;
		tInParam.bRecording = m_bRecording;
		tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
		tInParam.bIPv6 = m_bIPv6;
		if (m_bIPv6)
		{
			memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
		}
		else
		{
			tInParam.dwClientIP = m_dwIPAddress[0];
		}

		// 开始调用停止播放
		if(!m_bPlaying_status)
		{

			if (0 == m_pRtspServer->fStatusCallBack(m_nSessionID,ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING,&tInParam,NULL))
			{

			}
			m_bPlaying_status = 1;
		}
	}


#endif

    //time(&m_tLastRefreshTime);
    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
   OutPutSocketRefreshTime(m_tLastRefreshTime);
    //delete[]buffer;
    return 0;
}

int CClientSocket::OnSendFailAsk_callback()
{
    Ants_RTSPV2_InParam_T tInParam;
    char *pUrl;
    if (m_bNeedClose || (!m_bPlay))
    {
        return 0;
    }
    memset(&tInParam,0,sizeof(tInParam));

    tInParam.pUrl = m_pCurrUrl;//m_pCurrUrl;
    tInParam.wRemotePort = m_uPort;
    tInParam.dwSessionID = m_nSessionID;
    tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
	tInParam.bIPv6 = m_bIPv6;
	if (m_bIPv6)
	{
		memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
	}
	else
	{
		tInParam.dwClientIP = m_dwIPAddress[0];
	}
    if (m_bRecording)
    {
        tInParam.bRecording = 1;
    }
    return m_pRtspServer->fStatusCallBack(m_nSessionID,ANTS_RTSPSERVER_CALLBACK_TYPE_SENDFAIL_ASKING,NULL,NULL);
}

int CClientSocket::Pause_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0;
    int handle = -1;
    int nLine,nSeq = 0;
    char *pstr = NULL;
    char *buffer=NULL;
    time_t tim;
    Ants_RtspDayTime tStart,tStop;
    int bTimeValid = 0;
    memset(&tStart,0,sizeof(tStart));
    memset(&tStop,0,sizeof(tStop));
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        return -1;
    }

    //time(&m_tLastRefreshTime);
    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
    OutPutSocketRefreshTime(m_tLastRefreshTime);
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }
        }
        else if (!pMsg->StrCmp("Range:",nLine,0))
        {
            int n;
            char *pBar;
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                int nYear = 0,nMon = 0,nDay = 0,nHour = 0,nMin = 0,nSec = 0,nMsec = 0;
                // 开始时间
                if (strnicmp(pstr,"clock=",6) == 0)
                {
                    n = sscanf(pstr,"clock=%04d%02d%02dT%02d%02d%02d.%dZ",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec,&nMsec);
                    if (n > 3)
                    {
                        bTimeValid |= 1;
                        tStart.wYear = nYear;
                        tStart.byMon = nMon;
                        tStart.byDay = nDay;
                        tStart.byHour = nHour;
                        tStart.byMin = nMin;
                        tStart.bySec = nSec;
                        tStart.wMsec = nMsec;
                    }
                    pBar = strchr(pstr,'-');

                    if (pBar != NULL)
                    {
                        n = sscanf(pBar,"-%04d%02d%02dT%02d%02d%02d.%dZ",&nYear,&nMon,&nDay,&nHour,&nMin,&nSec,&nMsec);
                        if (n > 3)
                        {
                            bTimeValid |= 2;
                            tStop.wYear = nYear;
                            tStop.byMon = nMon;
                            tStop.byDay = nDay;
                            tStop.byHour = nHour;
                            tStop.byMin = nMin;
                            tStop.bySec = nSec;
                            tStop.wMsec = nMsec;
                        }
                    }
                }

            }

        }
    }
    Ants_RTSPV2_InParam_T tInParam;
    Ants_RTSPV2_OutParam_T tOutParam;
    char *pUrl;
    memset(&tOutParam,0,sizeof(tOutParam));
    memset(&tInParam,0,sizeof(tInParam));

    tInParam.pUrl = &pMsg->m_pMessage[pMsg->m_nPos[0][2]];//m_pCurrUrl;
    tInParam.wRemotePort = m_uPort;
    tInParam.dwSessionID = m_nSessionID;
    tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
	tInParam.bIPv6 = m_bIPv6;
	if (m_bIPv6)
	{
		memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
	}
	else
	{
		tInParam.dwClientIP = m_dwIPAddress[0];
	}
    if (m_bRecording)
    {
        tInParam.bPause = 1;
        tInParam.bRecording = 1;
        tInParam.bRangeValid = bTimeValid;
        tInParam.tRangeStart = tStart;
        tInParam.tRangeStop = tStop;
        tInParam.nPlaySeq = m_nLastPlaySeq;
        tInParam.bRateControl = m_bRateControl;
        tInParam.nStreamType = m_bSetup & 3;
        tInParam.nScale = m_nScale;
    }
    else
    {
        time(&tim);
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
                           "RTSP/1.0 455 Method Not Valid in This State\r\nCSeq: %d\r\n",
                           nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        //SendData(buffer,nBytes);
        SendDataV2(buffer,nBytes,0);
       SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete []buffer;
        return 0;
    }

    handle = m_hRtpSess;
    if (m_bModeOpen)
    {
        m_pRtspServer->fControl(m_hRtpSess,ANTS_RTSPSERVER_CALLBACK_TYPE_PLAYING,&tInParam,NULL,pStreamUser);
    }

    time(&tim);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d\r\n",m_nSessionID);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");
    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);
    //delete []buffer;
    return 0;
}

int CClientSocket::set_parameter_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0;
    int handle = -1;
    int nLine,nSeq = 0;
    char *pstr = NULL;
    char *buffer=NULL;
    time_t tim;
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        return -1;
    }
    //time(&m_tLastRefreshTime);
    if (!pMsg->IsCheck())
    {
        Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
       OutPutSocketRefreshTime(m_tLastRefreshTime);
    }
    time(&tim);
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }
        }
    }
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-Type: text/parameters\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-length: 0\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d\r\n",m_nSessionID);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");
    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,2);
    //delete []buffer;
    return 0;
}

int CClientSocket::get_parameter_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0;
    int handle = -1;
    int nLine,nSeq = 0;
    char *pstr = NULL;
    char *buffer=NULL;
    time_t tim;
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        return -1;
    }
    //time(&m_tLastRefreshTime);
    if (!pMsg->IsCheck())
    {
        Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
        OutPutSocketRefreshTime(m_tLastRefreshTime);
    }
    time(&tim);
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }
        }
    }
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-Type: text/parameters\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Content-length: 0\r\n");
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d\r\n",m_nSessionID);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");
    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,1);
    //delete []buffer;
    return 0;
}

int CClientSocket::AntsComb_AddChan_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0;
    int handle = -1;
    int nLine,nSeq = 0;
    char *pstr = NULL;
    char *buffer=NULL;
    char *pChans = NULL;
    time_t tim;
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        return -1;
    }
    //time(&m_tLastRefreshTime);
    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
    OutPutSocketRefreshTime(m_tLastRefreshTime);
    time(&tim);
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {//
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }
        }
        else if (!pMsg->StrCmp("Channel-Number:",nLine,0))
        {//
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                pChans = pstr;
            }
        }
    }
    Ants_RTSPV2_InParam_T tInParam;
    Ants_RTSPV2_OutParam_T tOutParam;
    char *pUrl,*pChansTmp = NULL;
    memset(&tOutParam,0,sizeof(tOutParam));
    memset(&tInParam,0,sizeof(tInParam));

    tInParam.pUrl = &pMsg->m_pMessage[pMsg->m_nPos[0][2]];//m_pCurrUrl;
    tInParam.wRemotePort = m_uPort;
    tInParam.dwSessionID = m_nSessionID;
    tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
    tInParam.nAntsComb = m_bAntsComb;
	tInParam.bIPv6 = m_bIPv6;
	if (m_bIPv6)
	{
		memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
	}
	else
	{
		tInParam.dwClientIP = m_dwIPAddress[0];
	}
    if (pChans != NULL)
    {
        pChansTmp = strdup(pChans);
    }
    tInParam.pChChange = pChansTmp;
    if (m_bRecording)
    {
        tInParam.bRecording = 1;
    }


    handle = m_hRtpSess;
    if(m_pRtspServer->fControl(m_hRtpSess,ANTS_RTSPSERVER_CALLBACK_TYPE_ADDCH,&tInParam,NULL,pStreamUser))
    {
        time(&tim);
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
            "RTSP/1.0 406 Not Acceptable\r\nCSeq: %d\r\n",
            nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        SendDataV2(buffer,nBytes,0);
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete []buffer;
		if(pChansTmp != NULL)
        {
            free(pChansTmp);
            pChansTmp = NULL;
        }
        return 0;
    }
	if(pChansTmp != NULL)
    {
        free(pChansTmp);
        pChansTmp = NULL;
    }
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d\r\n",m_nSessionID);
    if(pChans != NULL)
    {
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Channel-Number: %s\r\n",pChans);
    }
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");
    SendDataV2(buffer,nBytes,0);
    //delete []buffer;
    return 0;
}

int CClientSocket::AntsComb_DecChan_handle(CMessage *pMsg)
{
    int nBytes = 0,nContext = 0;
    int handle = -1;
    int nLine,nSeq = 0;
    char *pstr = NULL;
    char *buffer=NULL;
    time_t tim;
    char *pChans = NULL;
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }
    buffer = new char[ANTS_RTSP_TMP_BUFSIZE + 1];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        return -1;
    }
    //time(&m_tLastRefreshTime);
    Ants_rtsp_GetSysRunTime(&m_tLastRefreshTime,NULL);
   OutPutSocketRefreshTime(m_tLastRefreshTime);
    time(&tim);
    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {//
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }
        }
        else if (!pMsg->StrCmp("Channel-Number:",nLine,0))
        {//
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                pChans = pstr;
            }
        }
    }
    Ants_RTSPV2_InParam_T tInParam;
    Ants_RTSPV2_OutParam_T tOutParam;
    char *pUrl,*pChansTmp = NULL;
    memset(&tOutParam,0,sizeof(tOutParam));
    memset(&tInParam,0,sizeof(tInParam));

    tInParam.pUrl = &pMsg->m_pMessage[pMsg->m_nPos[0][2]];//m_pCurrUrl;
    tInParam.wRemotePort = m_uPort;
    tInParam.dwSessionID = m_nSessionID;
    tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);
    tInParam.nAntsComb = m_bAntsComb;
	tInParam.bIPv6 = m_bIPv6;
	if (m_bIPv6)
	{
		memcpy(tInParam.dwClientIPv6,m_dwIPAddress,16);
	}
	else
	{
		tInParam.dwClientIP = m_dwIPAddress[0];
	}
    if (pChans != NULL)
    {
        pChansTmp = strdup(pChans);
    }
    tInParam.pChChange = pChansTmp;
    if (m_bRecording)
    {
        tInParam.bRecording = 1;
    }

    handle = m_hRtpSess;
    if(m_pRtspServer->fControl(m_hRtpSess,ANTS_RTSPSERVER_CALLBACK_TYPE_DECCH,&tInParam,NULL,pStreamUser))
    {
        time(&tim);
        nBytes += snprintf((char*)buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,
            "RTSP/1.0 406 Not Acceptable\r\nCSeq: %d\r\n",
            nSeq);
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
        nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");//空行
        SendDataV2(buffer,nBytes,0);
        SetNeedClose(ANTS_RTSPSERVER_ERROR_ParseError);
        //delete []buffer;
        if(pChansTmp != NULL)
        {
            free(pChansTmp);
            pChansTmp = NULL;
        }
        return 0;
    }
    if(pChansTmp != NULL)
    {
        free(pChansTmp);
        pChansTmp = NULL;
    }
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%s 200 OK\r\n",&pMsg->m_pMessage[pMsg->m_nPos[0][2]]);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"CSeq: %d\r\n",nSeq);
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Session: %d\r\n",m_nSessionID);
    if(pChans != NULL)
    {
        nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Channel-Number: %s\r\n",pChans);
    }

    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"Date: ");//date
    nBytes += strftime(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"%d %b %Y %H:%M:%S GMT\r\n",gmtime(&tim));
    nBytes += snprintf(buffer + nBytes,ANTS_RTSP_TMP_BUFSIZE - nBytes,"\r\n");
    SendDataV2(buffer,nBytes,0);
    //delete []buffer;
    return 0;
}

int CClientSocket::Teardown_handle(CMessage *pMsg)
{
    int nBytes;
    int nLine,nSeq = 0;
    char *pstr;
    char *buffer = NULL;
    if (pMsg->m_nLineCnt < 2)
    {
        return -1;
    }

    buffer = new char[256];
    if (buffer == NULL)
    {
        SetNeedClose(ANTS_RTSPSERVER_ERROR_Close_Normal);
        return -1;
    }

    for (nLine = 1; nLine < pMsg->m_nLineCnt; nLine++)
    {
        if (!pMsg->StrCmp("CSeq:",nLine,0))
        {
            //
            pstr = pMsg->GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);//TCP
            }



        }

    }

#if 0
    if (m_dwIPAdress_VA[0])
    {
        m_pRtspServer->RemoveRtpDataClient(m_hRtpSess,m_dwIPAdress_VA[0],m_nPort_VA[0],0);
        m_dwIPAdress_VA[0] = 0;
    }
    if (m_dwIPAdress_VA[1])
    {
        m_pRtspServer->RemoveRtpDataClient(m_hRtpSess,m_dwIPAdress_VA[1],m_nPort_VA[1],1);
        m_dwIPAdress_VA[1] = 0;
    }


    Ants_RTSPV2_InParam_T tInParam;
    Ants_RTSPV2_OutParam_T tOutParam;
    memset(&tOutParam,0,sizeof(tOutParam));
    memset(&tInParam,0,sizeof(tInParam));
    tInParam.pUrl = m_pCurrUrl;
    tInParam.dwClientIP = m_dwIPAddress;
    tInParam.wRemotePort = m_uPort;
    tInParam.dwSessionID = m_nSessionID;
    tInParam.pRequire = m_pRequire;
    tInParam.bRecording = m_bRecording;
    tInParam.wSize = sizeof(Ants_RTSPV2_InParam_T);

    // 开始调用停止播放
    if(m_bPlay)
    {
        if (m_bModeOpen)
        {
            if (0 == m_pRtspServer->fControl(m_nSessionID,ANTS_RTSPSERVER_CALLBACK_TYPE_STOPPING,&tInParam,NULL,pStreamUser))
            {

            }
        }
        m_bPlay = 0;
    }
#endif

    nBytes = snprintf(buffer,256,"%s 200 OK\r\nCSeq: %d\r\n\r\n",
                      &pMsg->m_pMessage[pMsg->m_nPos[0][2]],
                      nSeq);
    //SendData(buffer,nBytes);
    SendDataV2(buffer,nBytes,0);
    SetNeedClose(ANTS_RTSPSERVER_ERROR_Close_Normal);

    return 0;
}

static int Ants_rtpsession_svr_client_callbakcFxn (void *hClient,int nCallbackType,int SubType,int nProp,int nDataType, void *pData,int nDataLen,void *pUser)
{
    CClientSocket *pClient = (CClientSocket* )hClient;
    if (pClient == NULL)
    {
        if(nProp & 1)
        {
            if (pData != NULL)
            {
                free(pData);
                pData = NULL;
            }
        }
        return 0;
    }

    return pClient->Client_callbakcFxn(nCallbackType,SubType,nProp,nDataType,pData,nDataLen,pUser);


}

int CClientSocket::Client_callbakcFxn(int nCallbackType,int nSubType,int nProp,int nDataType, void *pData,int nDataLen,void *pUser)
{
    unsigned long dwStreamDataProp = 0;
    ANTS_RTSPSERVER_STREAMDATA fStreamDataCallback = NULL;
    void *pStreamDataUser = NULL;
    if (nCallbackType != ANTS_RTSP_CALLBACKBYPE_STREAM)
    {
        if(nProp & 1)
        {
            if (pData != NULL)
            {
                free(pData);
                pData = NULL;
            }
        }
        return 0;
    }
    // if(nDataType == AntsPktIFrames)
    // RTSP_DEBUG("[%d]nCallbackType = %d,subtype = %d,type = %d,datalen = %dn",0,nCallbackType,nSubType,nDataType,nDataLen);

    dwStreamDataProp = m_dwStreamDataProp;
    pStreamDataUser = m_pStreamDataUser;
    fStreamDataCallback = m_fStreamDataCallback;

    if (fStreamDataCallback == NULL)
    {
        dwStreamDataProp = 0;
        fStreamDataCallback = NULL;
        pStreamDataUser = NULL;
        if (m_pRtspServer !=NULL)
        {
            m_pRtspServer->GetStreamCallBack(-1,&dwStreamDataProp,&fStreamDataCallback,&pStreamDataUser);
        }
    }
    if (fStreamDataCallback == NULL)
    {
        if(nProp & 1)
        {
            if (pData != NULL)
            {
                free(pData);
                pData = NULL;
            }
        }
        return 0;
    }
    if (nDataType == ANTS_RTSPSERVER_DATATYPE_AUDIOSTREAMDATA)
    {
        if (pUser == m_pAudioSession && m_pAudioSession != NULL)
        {
            nDataType = ANTS_RTSPSERVER_DATATYPE_TALK_AUDIODATA;
        }

    }
    fStreamDataCallback(m_hRtpSess,m_nSessionID,nDataType,(nProp & 1) & (dwStreamDataProp&1),(unsigned char *)pData,nDataLen,pStreamDataUser);
    if (!(dwStreamDataProp & 1))
    {
        if(nProp & 1)
        {
            if (pData != NULL)
            {
                free(pData);
                pData = NULL;
            }
        }
    }


    return 0;
}
void CClientSocket::Poll()
{
    if (m_pRtpSess != NULL)
    {
        m_pRtpSess->Poll();
    }

    if (m_pVideoSession != NULL)
    {
        m_pVideoSession->PollRTPPacket();
    }
    if (m_pAudioSession != NULL)
    {
        m_pAudioSession->PollRTPPacket();
    }
}
int CClientSocket::CreateVideo()
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CH264RtpSession *sess = NULL;

    if (m_pVideoSession != NULL)
    {
        m_pVideoSession->Destroy();
        delete m_pVideoSession;
        m_pVideoSession =NULL;
    }

    sess = new CH264RtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_svr_client_callbakcFxn,sess);
    if (m_bTransType == 3 ||
        m_bTransType == 4)
    {

        status = sess->CreateClient(0,1,NULL,this,NULL,m_bIPv6);

        m_nVideoBasePort = 0;
    }
    else
    {
        if (m_bTransType == 2)
        {
            // 多播
            portbase = m_nPort_VA[2];
        }
        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->CreateClient(portbase,0,NULL,this,NULL,m_bIPv6);
                if (!status)
                {
                    m_nVideoBasePort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
            status = sess->CreateClient(portbase,m_bTransType == 2?2:0,NULL,this,NULL,m_bIPv6);
            if (!status)
            {
                m_nVideoBasePort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }
    }

    if (status)
    {
        delete sess;
        return -1;
    }
    RTSP_DEBUG("video port %d\n",m_nAudioBasePort);
    sess->SetDefaultPayloadType(m_nRequireVideoType);
    sess->SetRTSPStreamType(Ants_rtsp_Payload2StreamType(m_nRequireVideoType));
    sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(m_nRequireVideoType)/1000);
    sess->SetPayloadClockRate(CRtspServer::GetPayloadClockRate(m_nRequireVideoType));
    sess->SetDefaultMark(false);
    sess->SetProp(1);
    m_pVideoSession = sess;
    return 0;
}

int CClientSocket::CreateAudio()
{
    uint16_t portbase = 0,destport;
    int status = -1,i,num;
    CAudioRtpSession *sess = NULL;
    if (m_pAudioSession != NULL)
    {
        m_pAudioSession->Destroy();
        delete m_pAudioSession;
        m_pAudioSession = NULL;
    }
    sess = new CAudioRtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    sess->SetClientCallback(this,Ants_rtpsession_svr_client_callbakcFxn,sess);
    if (m_bTransType == 3 ||
        m_bTransType == 4)
    {

        status = sess->CreateClient(0,1,NULL,this,NULL,m_bIPv6);
        m_nAudioBasePort = 3;
    }
    else
    {
        if (m_bTransType == 2)
        {
            portbase = m_nPort_VA[3];
        }
        if (portbase == 0)
        {
            for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
            {
                portbase -= portbase & 1;
                if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
                {
                    break;
                }
                if (portbase == 0)
                {
                    continue;
                }


                status = sess->CreateClient(portbase,0,NULL,this,NULL,m_bIPv6);
                if (!status)
                {
                    m_nAudioBasePort = portbase;
                    break;
                }
                else
                {
                    // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
                }

            }
        }
        else
        {
            status = sess->CreateClient(portbase,m_bTransType == 2?2:0,NULL,this,NULL,m_bIPv6);
            if (!status)
            {
                m_nAudioBasePort = portbase;
            }
            else
            {
                //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
            }
        }





    }

    if (status)
    {
        delete sess;
        return -1;
    }
    sess->SetDefaultPayloadType(m_nRequireAudioType);
    sess->SetRTSPStreamType(Ants_rtsp_Payload2StreamType(m_nRequireAudioType));
    sess->SetDefaultTimestampIncrement(40 * CRtspServer::GetPayloadClockRate(m_nRequireAudioType)/1000);
    sess->SetPayloadClockRate(CRtspServer::GetPayloadClockRate(m_nRequireAudioType));
    sess->SetDefaultMark(false);
    sess->SetProp(1);
    m_pAudioSession = sess;
    return 0;
}
int CClientSocket::SetStreamCallBack(unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser)
{
    m_dwStreamDataProp = dwProp;
    m_fStreamDataCallback=fxn;
    m_pStreamDataUser=pUser;
    return 0;
}

CRtspServer::CRtspServer()
{
    m_nSocket = -1;
	m_nSocketIPv6 = -1;
    m_pClientSocketList = NULL;
    m_pClientSocketDelList = NULL;
    m_pRtpSessionMgrList = NULL;
    m_pRtpSessionMgrDelList = NULL;
    m_nMaxNumSocket = 0;
    m_bThreadExit = 0;
    m_nTotalWriteSize = 0;
    m_bWriteWait = 0;
    m_pHttpSessionCookieList = NULL;
    m_nOverHttpSocket = -1;
	m_nOverHttpSocketIPv6 = -1;
    m_nOverHttpPort = 0;
    m_bInit = 0;
    m_fTCPDataCallBack = NULL;
    m_fStatusCallBack = NULL;
    m_bRelTimeStamp = 0;
    m_bSupportMulticast = 1;
    m_dwRunTimeMSecond = 0;
    m_dwRunTimeSecond = 0;
    m_dwRunTimeLastSecond = 0;
    m_lock.Init();
    m_dwStreamDataProp = 0;
    m_pStreamDataUser = NULL;
    m_fStreamDataCallback = NULL;
    m_bUsing = 0;
    m_bWaitDelete = 0;
    m_bThreadFlag = 0;
    m_bRelTimeStamp = -1;
    m_bOldRecording = 1;
    fxnOpen = NULL;
    fxnClose = NULL;
    fxnRead = NULL;
    fxnRelease = NULL;
    fxnControl = NULL;
    fxnUser = NULL;
    fxnMode = 0;

    m_sslCtx = NULL;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_server_method();  // 使用TLS方法
    m_sslCtx = SSL_CTX_new(method);
    if (!m_sslCtx) {
        RTSP_ERROR("ctx new error\n");
        printf("-----------ssl ctx new failed\n");
        return;
    }


    // 加载服务器证书和私钥（假设文件名为server.crt和server.key）
    if (SSL_CTX_use_certificate_file(m_sslCtx, "/root/nginx/ssl/self.crt", SSL_FILETYPE_PEM) <= 0) {
        RTSP_ERROR("ctx add file error\n");
        printf("-----------ssl self.crt failed\n");
        return;
    }
    if (SSL_CTX_use_PrivateKey_file(m_sslCtx, "/root/nginx/ssl/self.key", SSL_FILETYPE_PEM) <= 0) {
        RTSP_ERROR("ctx add file error\n");
        printf("-----------ssl self.key failed\n");
        return;
    }
    printf("-----------ssl server ok\n");

}

CRtspServer::~CRtspServer()
{

    Destroy();

}
int CRtspServer::Destroy()
{
	m_lock.Lock();
	if(!m_bInit)
	{
		m_lock.Unlock();
		return 0;
	}
	m_lock.Unlock();
	m_bThreadExit = 1;

	if (m_bThreadFlag > 0)
	{
		while (1)
		{
			if (m_bThreadFlag == 2)
			{
				break;
			}

			Ants_RTSPServer_WaitTime(0,10000);
		}
	}
	while(IsRunning())
	{
		Ants_RTSPServer_WaitTime(0,100000);
		//RTPTime::Wait(RTPTime(0,100000));
	}
	RTSP_DEBUG("removeall client \n");


    RemoveAllClientSocket();


RTSP_DEBUG("removeall client del \n");
	do
	{
		RemoveClientFromDelList();
		Ants_RTSPServer_WaitTime(0,10000);
	} while (m_pClientSocketDelList != NULL);
	RTSP_DEBUG("here \n");
    do
    {
        RemoveSessionMgrFromDelList();
        Ants_RTSPServer_WaitTime(0,10000);
    }
    while(m_pRtpSessionMgrDelList != NULL);
	RTSP_DEBUG("here \n");
    RemoveAllRtpSessionMgr();
	RTSP_DEBUG("here \n");


    if (m_nSocket >= 0)
    {
        RTSP_ERROR("Close %d err[%d,%s]\n",m_nSocket,GetLastError(),strerror(GetLastError()));
        closeSocket(m_nSocket);
        m_nSocket = -1;
    }
	if (m_nSocketIPv6 >= 0)
	{
		RTSP_ERROR("Close %d err[%d,%s]\n",m_nSocketIPv6,GetLastError(),strerror(GetLastError()));
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
	}
    if (m_nOverHttpSocket >= 0)
    {
        RTSP_ERROR("Close %d err[%d,%s]\n",m_nOverHttpSocket,GetLastError(),strerror(GetLastError()));
        closeSocket(m_nOverHttpSocket);
        m_nOverHttpSocket = -1;
    }
	if (m_nOverHttpSocketIPv6 >= 0)
	{
		RTSP_ERROR("Close %d err[%d,%s]\n",m_nOverHttpSocketIPv6,GetLastError(),strerror(GetLastError()));
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
	}

    //Kill();

    m_hWriteSem.Post();

    m_lock.Lock();
    m_bInit = 0;
    m_lock.Unlock();

    m_nTotalWriteSize = 0;
RTSP_DEBUG("all done \n");

    return 0;
}



int CRtspServer::SetTunnelingOverHTTPIPv4(int nHttpPort)
{
	int nRet = -1;
	if (!IsRunning())
	{
		return -1;
	}
	if (m_nOverHttpSocket != -1)
	{
		//已经启用
		return -1;
	}
	m_nOverHttpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_nOverHttpSocket == -1)
	{
		RTSP_ERROR("rtsp over http socket ipv4  failed\n");
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(nHttpPort);

	int val = 1;

	if(setsockopt(m_nOverHttpSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
	{
		RTSP_ERROR("reuse failed port = %d!\n",nHttpPort);
		closeSocket(m_nOverHttpSocket);
		m_nOverHttpSocket = -1;
		return(-1);
	}

	if (bind(m_nOverHttpSocket, (struct sockaddr*)&addr, sizeof addr) != 0)
	{
		RTSP_ERROR("rtsp over http bind <%d> failed\n",nHttpPort);
		closeSocket(m_nOverHttpSocket);
		m_nOverHttpSocket = -1;
		return -1;
	}

#if defined(__WIN32__) || defined(_WIN32)
	unsigned long arg = 1;
	nRet =  ioctlsocket(m_nOverHttpSocket, FIONBIO, &arg);
	if (nRet)
	{
		closeSocket(m_nOverHttpSocket);
		m_nOverHttpSocket = -1;
		return -1;
	}

#else
	int curFlags = fcntl(m_nOverHttpSocket, F_GETFL, 0);
	nRet =  fcntl(m_nOverHttpSocket, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(m_nOverHttpSocket);
		m_nOverHttpSocket = -1;
		return -1;
	}

#endif

	//开始监听
	if (listen(m_nOverHttpSocket, 10) < 0)
	{
		closeSocket(m_nOverHttpSocket);
		m_nOverHttpSocket = -1;
		return -1;
	}
	m_nOverHttpPort = nHttpPort;
	return 0;

}

int CRtspServer::SetTunnelingOverHTTPIPv6(int nHttpPort)
{
	int nRet = -1;
	if (!IsRunning())
	{
		return -1;
	}
	if (m_nOverHttpSocketIPv6 != -1)
	{
		//已经启用
		return -1;
	}
	m_nOverHttpSocketIPv6 = socket(AF_INET6, SOCK_STREAM, 0);
	if (m_nOverHttpSocketIPv6 == -1)
	{
		RTSP_ERROR("rtsp over http socket ipv6  failed\n");
		return -1;
	}
	struct sockaddr_in6 addr;
	memset(&addr,0,sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;
	addr.sin6_port = htons(nHttpPort);

	int val = 1;

	int on = 1;
	if (m_nOverHttpSocketIPv6 != -1 && setsockopt(m_nOverHttpSocketIPv6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on)) < 0)
	{
		printf("[%s.%d] http setsockopt IPV6_V6ONLY \n",__FUNCTION__,__LINE__);
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
		return -1;
	}

	if(setsockopt(m_nOverHttpSocketIPv6,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
	{
		RTSP_ERROR("reuse failed port = %d!\n",nHttpPort);
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
		return(-1);
	}

	if (bind(m_nOverHttpSocketIPv6, (struct sockaddr*)&addr, sizeof addr) != 0)
	{
		RTSP_ERROR("rtsp over http bind <%d> failed\n",nHttpPort);
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
		return -1;
	}

#if defined(__WIN32__) || defined(_WIN32)
	unsigned long arg = 1;
	nRet =  ioctlsocket(m_nOverHttpSocketIPv6, FIONBIO, &arg);
	if (nRet)
	{
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
		return -1;
	}

#else
	int curFlags = fcntl(m_nOverHttpSocketIPv6, F_GETFL, 0);
	nRet =  fcntl(m_nOverHttpSocketIPv6, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
		return -1;
	}

#endif

	//开始监听
	if (listen(m_nOverHttpSocketIPv6, 10) < 0)
	{
		closeSocket(m_nOverHttpSocketIPv6);
		m_nOverHttpSocketIPv6 = -1;
		return -1;
	}
	m_nOverHttpPort = nHttpPort;
	return 0;

}

int CRtspServer::SetTunnelingOverHTTP(int nHttpPort)
{
    int nRet = -1,nRetIPv6 = -1;
    if (!IsRunning())
    {
        return -1;
    }
	nRet = SetTunnelingOverHTTPIPv4(nHttpPort);
	nRetIPv6 = SetTunnelingOverHTTPIPv6(nHttpPort);
	if(nRet < 0 && nRetIPv6 < 0)
	{
		return nRet;
	}
    return 0;

}

int CRtspServer::CreateIPv4(int nPort )
{
	int nRet = -1;



	m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_nSocket == -1)
	{
		RTSP_ERROR("Socket failed\n");
		Destroy();
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(nPort);

	int val = 1;

	if(setsockopt(m_nSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
	{
		RTSP_ERROR("reuse failed port = %d! [%s]\n",nPort,strerror(GetLastError()));
		return(-1);
	}
	int alive = 1;
	if (0 != setsockopt(m_nSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&alive, sizeof(alive)))
	{

	}

	if (bind(m_nSocket, (struct sockaddr*)&addr, sizeof addr) != 0)
	{
		RTSP_ERROR("rtsp bind <%d> failed\n",nPort);
		closeSocket(m_nSocket);
		m_nSocket = -1;
		return -1;
	}

#if defined(__WIN32__) || defined(_WIN32)
	unsigned long arg = 1;
	nRet =  ioctlsocket(m_nSocket, FIONBIO, &arg);
	if (nRet)
	{
		RTSP_ERROR("Socket FIONBIO failed\n");
		closeSocket(m_nSocket);
		m_nSocket = -1;
		return -1;
	}

#else
	int curFlags = fcntl(m_nSocket, F_GETFL, 0);
	nRet =  fcntl(m_nSocket, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(m_nSocket);
		m_nSocket = -1;
		return -1;
	}

#endif

	//开始监听
	if (listen(m_nSocket, 10) < 0)
	{
		RTSP_ERROR("Socket listen failed\n");
		closeSocket(m_nSocket);
		m_nSocket = -1;
		return -1;
	}
#if 0
	CClientSocket *a =new CClientSocket(this,0);
	int ncnt = 0;
	while(1)
	{
		RTSP_DEBUG("cnt = %d\n",ncnt);
		if (ncnt == 47)
		{
			int iii = 0;
		}
		a->ReadAndParse();
		ncnt++;
		if (ncnt > 100)
		{
			break;
		}
	}
#endif


	return 0;
}
int CRtspServer::CreateIPv6(int nPort )
{
	int nRet = -1;



	m_nSocketIPv6 = socket(AF_INET6, SOCK_STREAM, 0);
	if (m_nSocketIPv6 == -1)
	{
		RTSP_ERROR("Socket failed\n");
		return -1;
	}
	struct sockaddr_in6 addr;
	memset(&addr,0,sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;
	addr.sin6_port = htons(nPort);

	int val = 1;

	int on = 1;
	if (m_nSocketIPv6 != -1 && setsockopt(m_nSocketIPv6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on)) < 0)
	{
		printf("[%s.%d] setsockopt IPV6_V6ONLY \n",__FUNCTION__,__LINE__);
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
		return -1;
	}

	if(setsockopt(m_nSocketIPv6,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val))!=0)//设置socket选项用来重绑定端口
	{
		RTSP_ERROR("reuse failed port = %d! [%s]\n",nPort,strerror(GetLastError()));
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
		return -1;
	}
	int alive = 1;
	if (0 != setsockopt(m_nSocketIPv6, SOL_SOCKET, SO_KEEPALIVE, (char *)&alive, sizeof(alive)))
	{

	}

	if (bind(m_nSocketIPv6, (struct sockaddr*)&addr, sizeof addr) != 0)
	{
		RTSP_ERROR("rtsp bind <%d> failed\n",nPort);
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
		return -1;
	}

#if defined(__WIN32__) || defined(_WIN32)
	unsigned long arg = 1;
	nRet =  ioctlsocket(m_nSocketIPv6, FIONBIO, &arg);
	if (nRet)
	{
		RTSP_ERROR("Socket FIONBIO failed\n");
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
		return -1;
	}

#else
	int curFlags = fcntl(m_nSocketIPv6, F_GETFL, 0);
	nRet =  fcntl(m_nSocketIPv6, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
		return -1;
	}

#endif

	//开始监听
	if (listen(m_nSocketIPv6, 10) < 0)
	{
		RTSP_ERROR("Socket listen failed\n");
		closeSocket(m_nSocketIPv6);
		m_nSocketIPv6 = -1;
		return -1;
	}
#if 0
	CClientSocket *a =new CClientSocket(this,0);
	int ncnt = 0;
	while(1)
	{
		RTSP_DEBUG("cnt = %d\n",ncnt);
		if (ncnt == 47)
		{
			int iii = 0;
		}
		a->ReadAndParse();
		ncnt++;
		if (ncnt > 100)
		{
			break;
		}
	}
#endif


	return 0;
}


int CRtspServer::Create(int nPort )
{
    int nRet = -1,nRetV6 = -1;
    m_lock.Lock();
    if (m_bInit)
    {
        m_lock.Unlock();
        RTSP_ERROR("server not inited\n");
        return -1;
    }
    if (IsRunning())
    {
        m_lock.Unlock();

        RTSP_ERROR("Create %d IsRunning\n",nPort);
        return -1;
    }
    m_bInit = 1;
    m_lock.Unlock();
#ifndef WIN32
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction( SIGPIPE, &sa, 0 );
#endif

	nRet = CreateIPv4(nPort);
	nRetV6 = CreateIPv6(nPort);
	if(nRet < 0 && nRetV6 < 0)
	{
		Destroy();
		return -1;
	}

#if 0
    CClientSocket *a =new CClientSocket(this,0);
    int ncnt = 0;
    while(1)
    {
        RTSP_DEBUG("cnt = %d\n",ncnt);
        if (ncnt == 47)
        {
            int iii = 0;
        }
        a->ReadAndParse();
        ncnt++;
        if (ncnt > 100)
        {
            break;
        }
    }
#endif

    m_bThreadExit = 0;
    nRet = Start(16 * 1024 * 64 * 4);//开始线程
    if (nRet)
    {
        RTSP_ERROR("start thread failed\n");
        Destroy();
        return -1;
    }

    //    nRet = m_RtspWriteThread.Start();
    //  if (nRet)
    {
        //      Destroy();
        //      return -1;
    }
    return 0;
}

void *CRtspServer::Thread()
{


    struct timeval tv_timeToDelay;
    unsigned int bTimeout = 0,dwTimeout = 0;
    m_dwRunTimeLastSecond = 0;
    m_dwRunTimeSecond = 0;
    m_dwRunTimeMSecond = 0;
    unsigned dwSec,dwUSec;
    int bSleep = 0;
    ThreadStarted();

    m_bThreadFlag = 1;
#if !(defined(_WIN32) || defined(ON_ANDROID))
	syslog(LOG_NOTICE, "[%s:%d] pid=%d enter\n", __FUNCTION__, __LINE__, (unsigned int)syscall(SYS_gettid));
#endif
	if(0 == Ants_rtsp_GetSysRunTime(&dwSec,&dwUSec))
    {
        m_dwRunTimeSecond = dwSec;
        m_dwRunTimeMSecond = dwUSec / 1000;
    }
    SetTime(m_dwRunTimeSecond,m_dwRunTimeMSecond);
    while(!m_bThreadExit)
    {
        if(0 == Ants_rtsp_GetSysRunTime(&dwSec,&dwUSec))
        {
            m_dwRunTimeSecond = dwSec;
            m_dwRunTimeMSecond = dwUSec / 1000;
        }
        if(m_dwRunTimeLastSecond != m_dwRunTimeSecond)
        {
            //  printf("SVR sec = %d msec = %d  ............................ dwRunTimeLastSecond = %d\n",m_dwRunTimeSecond,m_dwRunTimeMSecond,m_dwRunTimeLastSecond);
            m_dwRunTimeLastSecond = m_dwRunTimeSecond;
            SetTime(m_dwRunTimeSecond,m_dwRunTimeMSecond);
            RemoveClientFromDelList();
            RemoveSessionMgrFromDelList();

        }
//printf("CRtspServer[%s.%d]\n",__FUNCTION__,__LINE__);
        CheckAlive();
//printf("CRtspServer[%s.%d]\n",__FUNCTION__,__LINE__);
        bSleep = 1;//UDPData();
//printf("CRtspServer[%s.%d]\n",__FUNCTION__,__LINE__);
        Fd_SetAll();
//printf("CRtspServer[%s.%d]\n",__FUNCTION__,__LINE__);
        tv_timeToDelay.tv_sec = 0;
        tv_timeToDelay.tv_usec = bSleep?10000:0;
        //SendData();
//printf("CRtspServer[%s.%d] m_nMaxNumSocket = %d [%d/%d]\n",__FUNCTION__,__LINE__, m_nMaxNumSocket, tv_timeToDelay.tv_sec, tv_timeToDelay.tv_usec);
        int selectResult = select(m_nMaxNumSocket, &m_readSet,NULL/* &m_writeSet*/,NULL/* &m_exceptionSet*/, &tv_timeToDelay);
//printf("CRtspServer[%s.%d] selectResult = %d\n",__FUNCTION__,__LINE__, selectResult);
        if (selectResult < 0)
        {
            //错误
            RTSP_ERROR("[%s.%d]select  %d/%d err[%d,%s]\n",__FUNCTION__,__LINE__,m_nSocket,m_nSocketIPv6,GetLastError(),strerror(GetLastError()));
            //Poll();
#if 0

            closeSocket(m_nSocket);
            m_nSocket = -1;
            break;
#else
            continue;
#endif
        }
        if (selectResult == 0)
        {
            //超时
            //Poll();
            bTimeout = 1;
            dwTimeout = 0;
            continue;
        }
        dwTimeout = 10000 - tv_timeToDelay.tv_usec;
//printf("CRtspServer[%s.%d]\n",__FUNCTION__,__LINE__);
        selectHandler();
//printf("CRtspServer[%s.%d]\n",__FUNCTION__,__LINE__);

    }
    RemoveClientFromDelList();
    RemoveSessionMgrFromDelList();

    m_bThreadFlag = 2;

    return NULL;
}

int CRtspServer::CheckAlive()
{
    int nSocket = -1;
    uint32_t CurrTime = 0;
    CClientSocket *p,*pCurr;

    Ants_rtsp_GetSysRunTime(&CurrTime,NULL);

    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        pCurr = p;
        nSocket = pCurr->GetSocket();
        p = p->GetNext();
        if (nSocket == -1)
        {
            continue;
        }

        //printf("nSocket = %d!!!!!!!!!!!!!!!!!!\n", nSocket);
        //检查当前是超时
        if(pCurr->CheckAlive(CurrTime))
        {
            // RTSP_DEBUG("CheckAlive .... close %d\n",nSocket);
            RTSP_DEBUG("RemoveClientSocke = %d!!!!!!!!!!!!!!!!!!!!\n", nSocket);
            RemoveClientSocket(nSocket);
            continue;
        }
    }
    m_lock.Unlock();

    return 0;
}

void CRtspServer::Fd_SetAll()
{

    int nSock = -1;
    CClientSocket *p;
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    FD_ZERO(&m_exceptionSet);
    m_lock.Lock();
    if (m_nSocket >= 0)
    {
        FD_SET(m_nSocket,&m_readSet);
        FD_SET(m_nSocket,&m_writeSet);
        FD_SET(m_nSocket,&m_exceptionSet);
        m_nMaxNumSocket = m_nSocket + 1;
    }

	if (m_nSocketIPv6 >= 0)
	{
		FD_SET(m_nSocketIPv6,&m_readSet);
		FD_SET(m_nSocketIPv6,&m_writeSet);
		FD_SET(m_nSocketIPv6,&m_exceptionSet);
		if(m_nMaxNumSocket < m_nSocketIPv6 + 1 )
		{
			m_nMaxNumSocket = m_nSocketIPv6 + 1;
		}

	}



    if (m_nOverHttpSocket != -1)
    {
        FD_SET(m_nOverHttpSocket,&m_readSet);
        FD_SET(m_nOverHttpSocket,&m_exceptionSet);
        if (m_nMaxNumSocket < m_nOverHttpSocket + 1)
        {
            m_nMaxNumSocket = m_nOverHttpSocket + 1;
        }
    }

	if (m_nOverHttpSocketIPv6 != -1)
	{
		FD_SET(m_nOverHttpSocketIPv6,&m_readSet);
		FD_SET(m_nOverHttpSocketIPv6,&m_exceptionSet);
		if (m_nMaxNumSocket < m_nOverHttpSocketIPv6 + 1)
		{
			m_nMaxNumSocket = m_nOverHttpSocketIPv6 + 1;
		}
	}

#if 0
    p = m_pClientSocketList;
    while(p != NULL)
    {
        nSock = p->GetSocket();
        if (nSock >= 0)
        {
            FD_SET(nSock,&m_readSet);
            FD_SET(nSock,&m_writeSet);
            FD_SET(nSock,&m_exceptionSet);
            if (nSock + 1 > m_nMaxNumSocket)
            {
                m_nMaxNumSocket = nSock + 1;
            }


        }
        if (p->GetBackSession(1) != NULL)
        {
            nSock = p->GetBackSession(1)->GetRTPSocket();
            if (nSock >= 0)
            {
                FD_SET(nSock,&m_readSet);
                if (nSock + 1 > m_nMaxNumSocket)
                {
                    m_nMaxNumSocket = nSock + 1;
                }


            }
            nSock = p->GetBackSession(1)->GetRTCPSocket();
            if (nSock >= 0)
            {
                FD_SET(nSock,&m_readSet);
                if (nSock + 1 > m_nMaxNumSocket)
                {
                    m_nMaxNumSocket = nSock + 1;
                }


            }
        }
        if (p->GetBackSession(0) != NULL)
        {
            nSock = p->GetBackSession(0)->GetRTPSocket();
            if (nSock >= 0)
            {
                FD_SET(nSock,&m_readSet);
                if (nSock + 1 > m_nMaxNumSocket)
                {
                    m_nMaxNumSocket = nSock + 1;
                }


            }
            nSock = p->GetBackSession(0)->GetRTCPSocket();
            if (nSock >= 0)
            {
                FD_SET(nSock,&m_readSet);
                if (nSock + 1 > m_nMaxNumSocket)
                {
                    m_nMaxNumSocket = nSock + 1;
                }


            }
        }


        p = p->GetNext();
    }
#endif
    m_lock.Unlock();

}


void CRtspServer::SendData()
{
    int nSocket = -1;
    CClientSocket *p,*pCurr;
    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        pCurr = p;
        nSocket = pCurr->GetSocket();
        p = p->GetNext();
        if (nSocket == -1)
        {
            continue;
        }

        pCurr->SendResponse();


    }
    m_lock.Unlock();

}
void CRtspServer::selectHandler()
{
    int nRet;
    int nSocket = -1;
    //time_t CurrTime = 0;
    uint32_t CurrTime = 0;
    CClientSocket *p,*pCurr;
    if (m_nSocket != -1 && FD_ISSET(m_nSocket,&m_readSet))
    {
        incomingConnectionHandler();

    }
	if (m_nSocketIPv6 != -1 && FD_ISSET(m_nSocketIPv6,&m_readSet))
	{
		incomingConnectionHandler(0,-1,1);

	}
    if (m_nSocket != -1 && FD_ISSET(m_nSocket,&m_writeSet))
    {
        int ii = 0;
    }
    if (m_nSocket != -1 && FD_ISSET(m_nSocket,&m_exceptionSet))
    {
        int ii = 0;
    }

    if (m_nOverHttpSocket != -1 && FD_ISSET(m_nOverHttpSocket,&m_readSet))
    {
        incomingConnectionHandler(1);
    }
	if (m_nOverHttpSocketIPv6 != -1 && FD_ISSET(m_nOverHttpSocketIPv6,&m_readSet))
	{
		incomingConnectionHandler(1,-1,1);
	}
    if (m_nOverHttpSocket != -1 && FD_ISSET(m_nOverHttpSocket,&m_writeSet))
    {
        int ii = 0;
    }
    if (m_nOverHttpSocket != -1 && FD_ISSET(m_nOverHttpSocket,&m_exceptionSet))
    {
        int ii = 0;
    }

#if 0
    //time(&CurrTime);
    Ants_rtsp_GetSysRunTime(&CurrTime,NULL);
    p = m_pClientSocketList;
    while(p != NULL)
    {
        int nSockTmp;
        pCurr = p;
        nSocket = pCurr->GetSocket();
        p = p->GetNext();
        if (nSocket == -1)
        {
            continue;
        }


        if (pCurr->GetBackSession(0) != NULL)
        {
            nSockTmp = pCurr->GetBackSession(0)->GetRTPSocket();
            if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
            {

                pCurr->GetBackSession(0)->RecvRTPPacket();

            }
            nSockTmp = pCurr->GetBackSession(0)->GetRTCPSocket();
            if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
            {

                pCurr->GetBackSession(0)->RecvRTCPPacket();

            }
        }
        if (pCurr->GetBackSession(1) != NULL)
        {
            nSockTmp = pCurr->GetBackSession(1)->GetRTPSocket();
            if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
            {

                pCurr->GetBackSession(1)->RecvRTPPacket();

            }
            nSockTmp = pCurr->GetBackSession(1)->GetRTCPSocket();
            if (nSockTmp != -1 && FD_ISSET(nSockTmp,&m_readSet))
            {

                pCurr->GetBackSession(1)->RecvRTCPPacket();

            }
        }

        RTSP_DEBUG("nSocket = %d!!!!!!!!!!!!!!!!!!\n", nSocket);
        //检查当前是超时
        if(pCurr->CheckAlive(CurrTime))
        {
            // RTSP_DEBUG("CheckAlive .... close %d\n",nSocket);
            RTSP_DEBUG("nSocket1 = %d!!!!!!!!!!!!!!!!!!!!\n", nSocket);
            RemoveClientSocket(nSocket);
            continue;
        }

#if 0
        if (FD_ISSET(nSocket,&m_readSet))
        {
            nRet = incomingRequestHandler(pCurr);
            if (nRet < 0)
            {
                //RTSP_DEBUG("Requesthandle faild .... close %d\n",nSocket);
                RemoveClientSocket(nSocket);
                continue;
            }
        }

        if (FD_ISSET(nSocket,&m_writeSet))
        {
            pCurr->SendResponse();

        }
        if (FD_ISSET(nSocket,&m_exceptionSet))
        {
            RemoveClientSocket(nSocket);
            continue;
        }
#endif

    }
#endif

}
int CRtspServer::incomingRequestHandler(CClientSocket *pClient)
{

    int ReadBytes;
    int ret ;
    ReadBytes = pClient->ReadAndParse();
    if (ReadBytes < 0)
    {
        return -1;
    }
    ret = pClient->MessageHandle();
    if (ret < 0)
    {
        return -1;
    }
    if (ret == 1)
    {
        Poll();
    }
    // 查找客户端
    pClient->Poll();


    return 0;
}


int CRtspServer::incomingConnectionHandler(int bOverHttp,int nClientSocket,int bIPv6)
{
	int nRet = -1;
	struct sockaddr_in6 clientAddrIPv6;
	struct sockaddr_in clientAddrIPv4;
	socklen_t clientAddrLen = sizeof(clientAddrIPv4);
	struct timeval tv_out;
	struct sockaddr *clientAddr = (struct sockaddr *)&clientAddrIPv4;
	int nrtspSocket;
	int clientSocket = nClientSocket;
    SSL *ssl = NULL;

	if(clientSocket == -1)
	{
		if (bIPv6)
		{
			if (bOverHttp)
			{
				nrtspSocket = m_nOverHttpSocketIPv6;
			}
			else
			{
				nrtspSocket = m_nSocketIPv6;
			}
			clientAddr = (struct sockaddr *)&clientAddrIPv6;
			clientAddrLen = sizeof(clientAddrIPv6);
		}
		else
		{
			if (bOverHttp)
			{
				nrtspSocket = m_nOverHttpSocket;
			}
			else
			{
				nrtspSocket = m_nSocket;
			}
		}


		clientSocket = accept(nrtspSocket, (struct sockaddr*)clientAddr, &clientAddrLen);

		if (clientSocket < 0)
		{
			return -1;
		}

        // 创建SSL对象并进行握手
        ssl = SSL_new(m_sslCtx);
        printf("m_sslCtx:[%p] ssl:[%p] clientSocket[%d]\n",clientSocket);
        SSL_set_fd(ssl, clientSocket);
        if (SSL_accept(ssl) <= 0) {
            RTSP_ERROR("ssl accept failed\n");
            printf("-----------ssl failed\n");
            SSL_free(ssl);
            close(clientSocket);
            return -1;
        }
	}

	int alive = 1;
	if (0 != setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&alive, sizeof(alive)))
	{

	}
#if defined(__WIN32__) || defined(_WIN32)

	unsigned long arg = 1;
	nRet =  ioctlsocket(clientSocket, FIONBIO, &arg);
	if (nRet)
	{
		closeSocket(clientSocket);
		return -1;
	}

	tcp_keepalive tkeepalive;
	DWORD dwBytesReturned;

	tkeepalive.onoff = 1;
	tkeepalive.keepalivetime = 60000;
	tkeepalive.keepaliveinterval = 15;
	if(WSAIoctl(clientSocket,SIO_KEEPALIVE_VALS,&tkeepalive,sizeof(tkeepalive),NULL,0,&dwBytesReturned,NULL,NULL))
	{
		RTSP_DEBUG("WSAIoctl failed\n");
	}


#else
	int curFlags = fcntl(clientSocket, F_GETFL, 0);
	nRet =  fcntl(clientSocket, F_SETFL, curFlags|O_NONBLOCK);
	if (nRet < 0 )
	{
		closeSocket(clientSocket);
		return -1;
	}
	int keepalive = 1; // 开启keepalive属性
	int keepidle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepinterval = 15; // 探测时发包的时间间隔为15 秒
	int keepcount = 4; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(clientSocket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
	setsockopt(clientSocket, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	setsockopt(clientSocket, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));



#endif
	tv_out.tv_sec = 3;
	tv_out.tv_usec = 0;
	nRet = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv_out, sizeof(tv_out));
	if (nRet < 0 )
	{
		closeSocket(clientSocket);
		clientSocket = -1;
		return clientSocket;
	}

	tv_out.tv_sec = 3;
	tv_out.tv_usec = 0;
	nRet = setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv_out, sizeof(tv_out));
	if (nRet < 0 )
	{
		closeSocket(clientSocket);
		clientSocket = -1;
		return clientSocket;
	}

	int size;
	size = 65535;
	setsockopt(clientSocket,SOL_SOCKET,SO_RCVBUF,(const char *)&size,sizeof(int));
    size = 192 * 1024;//65535;
	setsockopt(clientSocket,SOL_SOCKET,SO_SNDBUF,(const char *)&size,sizeof(int));
	nRet = AddClientSocket(clientSocket,bIPv6,ssl);

	if (nRet)
	{
		closeSocket(clientSocket);
		return -1;
	}
	return nRet;

}
int CRtspServer::AddClientSocket(int clientSocket,int bIPv6,SSL *pSsl)
{
    CClientSocket *pNewClient;
    pNewClient = new CClientSocket(this,clientSocket,bIPv6, pSsl);
    if (pNewClient == NULL)
    {
        return -1;
    }

    m_lock.Lock();
    if (m_pClientSocketList != NULL)
    {
        m_pClientSocketList->SetPrev(pNewClient);
        pNewClient->SetNext(m_pClientSocketList);

    }

    m_pClientSocketList = pNewClient;
    pNewClient->Create();
    m_lock.Unlock();

    RTSP_ERROR("\n*** Open [%d] ***\n",pNewClient->GetSocket());

    return 0;

}

void CRtspServer::RemoveAllNoReadyClientSocket()
{
    CClientSocket *pClient,*pClientDel;
    int bDel = 0;
    //RTSP_DEBUG("close all.... \n");
    CRTSPOverHttpSessionCookie *p,*plast = NULL;


    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        bDel = 0;
        m_lock.Lock();
        pClient = p->m_pGet;
        if (pClient != NULL)
        {
            if (pClient->GetPlayReady())
            {
                // 删除
                RemoveClientSocket(pClient->GetSocket());
                p->m_pGet = NULL;
                bDel = 1;
            }

        }
        if (p->m_pPost != NULL)
        {
            if (p->m_pPost != pClient)
            {
                pClient = p->m_pPost;
                if (pClient->GetPlayReady())
                {
                    // 删除
                    RemoveClientSocket(pClient->GetSocket());
                    p->m_pPost = NULL;
                    bDel = 1;
                }
            }
        }
        m_lock.Unlock();
        if (bDel)
        {
            //删除掉
            if (plast == NULL)
            {
                //
                m_pHttpSessionCookieList = p->m_pNext;
            }
            else
            {
                plast->m_pNext = p->m_pNext;

            }

            delete p;
            if (plast == NULL)
            {
                p = m_pHttpSessionCookieList;
            }
            else
            {
                p = plast->m_pNext;
            }


            continue;
        }
        plast = p;
        p = p->m_pNext;
    }


    m_lock.Lock();
    pClient = m_pClientSocketList;
    while(pClient != NULL)
    {
        pClientDel = NULL;
        if (pClient->GetPlayReady())
        {
            pClientDel = pClient;
            pClient = pClient->GetNext();
            if (pClientDel->GetPrev() == NULL)
            {
                m_pClientSocketList = pClient;
                if (pClient != NULL)
                {
                    pClient->SetPrev(NULL);

                }


            }
            else
            {
                pClientDel->GetPrev()->SetNext(pClient);
                if (pClient != NULL)
                {
                    pClient->SetPrev(pClientDel->GetPrev());
                }

            }
        }
        else
        {
            pClient = pClient->GetNext();
        }
        if (pClientDel != NULL)
        {
            pClientDel->SetPrev(NULL);
            pClientDel->SetDeleteFlag();
            pClientDel->SetNext(m_pClientSocketDelList);
            m_pClientSocketDelList = pClientDel;

        }


    }
    m_lock.Unlock();
}
void CRtspServer::RemoveAllClientSocket()
{
    CClientSocket *p,*pDel;
    RTSP_DEBUG("close all.... \n");
    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        pDel = p;
		 RTSP_DEBUG("close %p.... \n",p);
        p = p->GetNext();

        pDel->SetDeleteFlag();

        RemoveClientSocket(pDel);
    }
    m_pClientSocketList = NULL;
    m_lock.Unlock();


}
void CRtspServer::RemoveClientFromDelList()
{
    CClientSocket *p,*pDel,*pTempDel = NULL,*pLast = NULL,*pOther = NULL;

    m_lock.Lock();
    p = m_pClientSocketDelList;


    while(p != NULL)
    {
        //if (p->IsUsed() && (p->GetThreadFlag() == 2 || p->GetThreadFlag() <= 0) && !(p->IsRunning()))
        if (p->IsUsed() || p->IsRunning())
        {
            pLast = p;
            p = p->GetNext();
            continue;
        }
        pOther = GetOtherClientInHttpSessionByClient(p);
        if (pOther != NULL)
        {
            if (pOther->IsUsed() || p->IsRunning())
            {
                pLast = p;
                p = p->GetNext();
                continue;
            }
        }

		pDel = p;
        p = p->GetNext();

        pDel->SetNext(NULL);
        pDel->SetPrev(NULL);
        pDel->SetNext(pTempDel);
        pTempDel = pDel;
        RemoveHttpSessionCookieByClient(pDel);
        RTSP_DEBUG("close m_pClientSocketDelList.... \n");

        if(pLast == NULL)
        {
            m_pClientSocketDelList = p;
        }
        else
        {
            pLast->SetNext(p);
			if(p != NULL)
			{
				p->SetPrev(pLast);
			}

        }
        // delete pDel;
    }

    m_lock.Unlock();
    p = pTempDel;
    while(p != NULL)
    {
        pDel = p;
        p = p->GetNext();
		RTSP_DEBUG("delete p %p\n",pDel);
        delete pDel;
		RTSP_DEBUG("delete p %p end\n",pDel);
    }
}

void CRtspServer::RemoveSessionMgrFromDelList()
{
    CRtpSessionMgr *pMgr,*pMgrDel = NULL,*pMgrLast = NULL,*pCurr = NULL;
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrDelList;
    while(pMgr != NULL)
    {
        if (pMgr->IsUsed())
        {
            pMgrLast = pMgr;
            pMgr = pMgr->m_pNext;
            continue;
        }
        pCurr = pMgr;
        pMgr = pMgr->m_pNext;

        pCurr->m_pNext = pMgrDel;
        pMgrDel = pCurr;



        if(pMgrLast == NULL)
        {
            m_pRtpSessionMgrDelList = pMgr;
        }
        else
        {
            pMgrLast->m_pNext = pMgr;
        }


    }
    m_lock.Unlock();
    pMgr = pMgrDel;
    while(pMgr != NULL)
    {
        pMgrDel = pMgr;
        pMgr = pMgr->m_pNext;
		RTSP_DEBUG("pMgrDel %p \n",pMgrDel);
        delete pMgrDel;
    }
}
void CRtspServer::RemoveClientSocket(int clientSocket)
{
    CClientSocket *p,*p1,*pOther = NULL;
    if (clientSocket == -1)
    {
        return;
    }
	RTSP_DEBUG("clientSocket = %d \n",clientSocket);
    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        if (p->GetSocket() == clientSocket)
        {
				RTSP_DEBUG("get it \n");
            //删除当前
            if (p->GetPrev() == NULL)
            {
                m_pClientSocketList = p->GetNext();

            }
            else
            {
                p->GetPrev()->SetNext(p->GetNext());
            }
            if (p->GetNext() != NULL)
            {
                p->GetNext()->SetPrev(p->GetPrev());
            }
            p->SetNext(NULL);
            p->SetPrev(NULL);
            break;
        }
        p = p->GetNext();
    }

    if (p != NULL)
    {
        // 检查当前是不是UDP发送会话。
        if(p->IsSendUDP())
        {
#if 1
            int nSessHandle;
            CRtpSessionMgr *pSessMgr;
            // 需要转移到新的会话
            p->SetSendUDPFlag(0);
            pSessMgr = p->GetRtpSessMgr();
			if (pSessMgr != NULL)
			{
                pSessMgr->SetCurSendSessionID(0);
			}
			nSessHandle = p->GetRtpSessHandle();

            p1 = m_pClientSocketList;
            while(p1 != NULL)
            {
                if ((p1->GetRtpSessHandle() == nSessHandle) && (p1 != p))
                {
                    if(!p1->IsWaitDelete())
                    {
                        p1->SetSendUDPFlag(1);
					    if (pSessMgr != NULL)
			            {
						    pSessMgr->SetCurSendSessionID(p1->GetSessionID());
					    }
                        break;
                    }

                }
                p1 = p1->GetNext();
            }
#endif
        }
        p->SetDeleteFlag();
		p->SetThreadExit(1);
        pOther = GetOtherClientInHttpSessionByClient(p);
        if (pOther != NULL)
        {
			pOther->SetNeedClose();
			if (pOther == p->m_pInPutSocket)
			{
				pOther->m_pOutPutSocket = NULL;
				p->m_pInPutSocket = NULL;
			}
			else if (pOther == p->m_pOutPutSocket)
			{
				pOther->m_pInPutSocket = NULL;
				p->m_pOutPutSocket = NULL;
			}


        }
        p->SetNext(m_pClientSocketDelList);
        m_pClientSocketDelList = p;

    }
    m_lock.Unlock();

}

void CRtspServer::RemoveClientSocket(CClientSocket *pClient)
{
    CClientSocket *p,*pOther = NULL;

    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        if (p == pClient)
        {
            //删除当前
            if (p->GetPrev() == NULL)
            {
                m_pClientSocketList = p->GetNext();

            }
            else
            {
                p->GetPrev()->SetNext(p->GetNext());
            }
            if (p->GetNext() != NULL)
            {
                p->GetNext()->SetPrev(p->GetPrev());
            }
            p->SetNext(NULL);
            p->SetPrev(NULL);
            break;
        }
        p = p->GetNext();
    }

    if (p != NULL)
    {
        p->SetDeleteFlag();
	    p->SetThreadExit(1);
        pOther = GetOtherClientInHttpSessionByClient(p);
        if (pOther != NULL)
        {
			if (pOther == p->m_pInPutSocket)
			{
				pOther->m_pOutPutSocket = NULL;
				p->m_pInPutSocket = NULL;
			}
			else if (pOther == p->m_pOutPutSocket)
			{
				pOther->m_pInPutSocket = NULL;
				p->m_pOutPutSocket = NULL;
			}
            pOther->SetNeedClose();

        }
        p->SetNext(m_pClientSocketDelList);
        m_pClientSocketDelList = p;

    }
    m_lock.Unlock();

}
int CRtspServer::LookupParam(char *pParam,CRtpSessionMgr **pRtpSess)
{
    int handle = -1;
    CRtpSessionMgr *pMgr;
    int bExist = 0;
    char str[256];
    RTSP_DEBUG("[%s.%d]pParam = %s\n",__FUNCTION__,__LINE__,pParam);
    if (pParam[0] == 0)
    {
        return handle;
    }
    strcpy(str,pParam);
    if (str[strlen(str) - 1] == '/')
    {
        str[strlen(str) - 1] = 0;
    }


    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (!strcmp(str,pMgr->m_pToken))
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    if (pMgr != NULL)
    {
        handle = pMgr->GetHandle();
        if (pRtpSess)
        {
            *pRtpSess = pMgr;
        }
    }



    return handle;

}

int CRtspServer::RemoveRtpSessionMgr(char *pParam)
{
    return 0;
}

int CRtspServer::RemoveAllRtpSessionMgr()
{
    CRtpSessionMgr *p,*pcurr;
    m_lock.Lock();
	RTSP_DEBUG("m_pRtpSessionMgrList %p \n",m_pRtpSessionMgrList);
    p = m_pRtpSessionMgrList;
    while(NULL != p)
    {
        pcurr = p;
        p = p->m_pNext;
        pcurr->m_pNext = NULL;
        pcurr->SetDeleteFlag();

        pcurr->m_pNext = m_pRtpSessionMgrDelList;
        m_pRtpSessionMgrDelList = pcurr;
        //delete pcurr;
    }
    m_pRtpSessionMgrList = NULL;
    m_lock.Unlock();
    return 0;

}

int CRtspServer::AddRtpSessionMgr(CRtpSessionMgr *pMgr)
{
    if (pMgr == NULL)
    {
        return -1;
    }
    m_lock.Lock();
    pMgr->m_nUseCnt++;
    pMgr->m_pNext = m_pRtpSessionMgrList;
    m_pRtpSessionMgrList = pMgr;
	RTSP_DEBUG("pMgr %p \n",pMgr);
    m_lock.Unlock();
    return 0;
}
int CRtspServer::RemoveRtpSessionMgr(CRtpSessionMgr *pMgr)
{
    CRtpSessionMgr *p,*pLast;
    m_lock.Lock();
	RTSP_DEBUG("pMgr %p \n",pMgr);
    p = m_pRtpSessionMgrList;
    pLast = NULL;
    while(NULL != p)
    {

        if (p == pMgr)
        {
            //彻底删除
            if (p->m_bFixed)
            {
                break;
            }
            if (pLast == NULL)
            {
                //
                m_pRtpSessionMgrList = p->m_pNext;
            }
            else
            {
                pLast->m_pNext = p->m_pNext;
            }
            p->m_pNext = NULL;
            //delete p;
            p->SetDeleteFlag();
            p->m_pNext = m_pRtpSessionMgrDelList;
            m_pRtpSessionMgrDelList = p;
            break;
        }
        pLast = p;
        p = p->m_pNext;
    }
    m_lock.Unlock();
    return 0;
}
void CRtspServer::SetTime(unsigned int uSec,unsigned int uMSec)
{
    CRtpSessionMgr *p,*pLast;
    m_lock.Lock();
    p = m_pRtpSessionMgrList;
    pLast = NULL;
    while(NULL != p)
    {
        p->SetTime(uSec,uMSec);
        p = p->m_pNext;
    }
    m_lock.Unlock();
}
void CRtspServer::Poll()
{
    CRtpSessionMgr *p,*pLast;
    m_lock.Lock();
    p = m_pRtpSessionMgrList;
    pLast = NULL;
    while(NULL != p)
    {
        p->Poll();
        p = p->m_pNext;
    }



    m_lock.Unlock();

}
int CRtspServer::CreateStream(char *pToken,int *bExist,int bTCP,int bStreamType,int nVideoPayloadType,int nAudioPayloadType,int bFixed,int bIPv6)
{
    CRtpSessionMgr *p;
    int btype,nRet;
    int handle = -1;
    char *pNewToken;
    int i;
    if (pToken == NULL)
    {
        return -1;
    }
    if (bStreamType < 0||bStreamType > 2)
    {
        return -1;
    }

    pNewToken = strdup(pToken);
    if (pNewToken == NULL)
    {
        return -1;
    }
    i = 0;
    while (pNewToken[i])
    {
        if(!((pNewToken[i] >= 'a' && pNewToken[i] <= 'z') ||
            (pNewToken[i] >= 'A' && pNewToken[i] <= 'Z') ||
            (pNewToken[i] >= '0' && pNewToken[i] <= '9') ||
            pNewToken[i] == '.'
            ))
        {

            pNewToken[i] = '_';
        }
        i++;
    }
    m_lock.Lock();
    p = m_pRtpSessionMgrList;
    while(p != NULL)
    {
        if (!strcmp(p->m_pToken,pNewToken))
        {
            break;
        }
        p = p->m_pNext;
    }
    if (p != NULL)
    {
        //存在
        handle = p->GetHandle();
        p->SetSetup(1);
        if (bExist != NULL)
        {
            *bExist = 1;
        }

	    btype = bStreamType;
	    if(btype == 0)btype =  3;
	    if (btype & 1)
	    {
	        //video
	        nRet = p->CreateVideo(nVideoPayloadType,bIPv6);
	        if (nRet)
	        {
	            m_lock.Unlock();
	            free(pNewToken);
	            return -1;
	        }
	    }
	    if (btype & 2)
	    {
	        //audio
	        nRet = p->CreateAudio(nAudioPayloadType,bIPv6);
	        if (nRet)
	        {
	            m_lock.Unlock();
	            free(pNewToken);
	            return -1;
	        }
	    }

        m_lock.Unlock();
        free(pNewToken);
        return handle;
    }

    p = new CRtpSessionMgr(pNewToken,bFixed,bTCP,m_fTCPDataCallBack,m_pRtpRtcpCallbackUser);
    if (p == NULL)
    {
        m_lock.Unlock();
        free(pNewToken);
        return -1;
    }
   // m_lock.Unlock();

   free(pNewToken);
    //创建
   // p = new CRtpSessionMgr(pToken,bFixed,bTCP,m_fTCPDataCallBack,m_pRtpRtcpCallbackUser);
    //if (p == NULL)
   // {
//
  //      return -1;
 //   }
    if (p->GetHandle() < 0)
    {
	    m_lock.Unlock();
        delete p;
        return -1;
    }
    p->SetSupportMulticast(m_bSupportMulticast);
    p->SetSetup(1);
    //
    btype = bStreamType;
    if(btype == 0)btype =  3;
    if (btype & 1)
    {
        //video
        nRet = p->CreateVideo(nVideoPayloadType,bIPv6);
        if (nRet)
        {
	        m_lock.Unlock();
            delete p;
            return -1;
        }
    }
    if (btype & 2)
    {
        //audio
        nRet = p->CreateAudio(nAudioPayloadType,bIPv6);
        if (nRet)
        {

            //
            m_lock.Unlock();
            delete p;
            return -1;
        }
    }
    //加入列表
    AddRtpSessionMgr(p);
	handle = p->GetHandle();
	m_lock.Unlock();

    return handle;
}
char *CRtspServer::GetStreamToken(int handle)
{
    CRtpSessionMgr *p;
    if (handle < 0)
    {
        return NULL;
    }

    m_lock.Lock();
    p = m_pRtpSessionMgrList;
    while(NULL != p)
    {
        if (handle == p->m_nStreamHandle)
        {

            break;
        }
        p = p->m_pNext;
    }
    if (p == NULL)
    {
        m_lock.Unlock();
        return NULL;
    }
    m_lock.Unlock();


    return p->GetToken();
}
CRtpSessionMgr *CRtspServer::GetSessionMgrByHandle(int handle)
{
    CRtpSessionMgr *p;
    if (handle < 0)
    {
        return NULL;
    }

    m_lock.Lock();
    p = m_pRtpSessionMgrList;
    while(NULL != p)
    {
        if (handle == p->m_nStreamHandle)
        {

            break;
        }
        p = p->m_pNext;
    }
    if (p == NULL)
    {
        m_lock.Unlock();
        return NULL;
    }
    m_lock.Unlock();


    return p;
}
int CRtspServer::DestroyStream(char *pToken)
{
    CRtpSessionMgr *p,*pLast;
    if (pToken == NULL)
    {
        return -1;
    }
    m_lock.Lock();
	RTSP_DEBUG("m_pRtpSessionMgrList %p \n",m_pRtpSessionMgrList);
    pLast = NULL;
    p = m_pRtpSessionMgrList;
    while(NULL != p)
    {
        if (!strcmp(pToken,p->m_pToken))
        {
		    if (p->m_bSetup > 0)
		    {
		        m_lock.Unlock();
		        return -1;
		    }

            if (pLast == NULL)
            {
                m_pRtpSessionMgrList = p->m_pNext;
            }
            else
            {
                pLast->m_pNext = p->m_pNext;
            }
            break;
        }
        pLast = p;
        p = p->m_pNext;
    }

    if (p == NULL)
    {
        m_lock.Unlock();
        return -1;
    }

    p->SetDeleteFlag();
    p->m_pNext = m_pRtpSessionMgrDelList;
    m_pRtpSessionMgrDelList = p;
    m_lock.Unlock();
    //delete p;

    return 0;

}
int CRtspServer::DestroyStream(int handle)
{

    CRtpSessionMgr *p,*pLast;
    if (handle < 0)
    {
        return -1;
    }
    pLast = NULL;
    m_lock.Lock();
    p = m_pRtpSessionMgrList;
    while(NULL != p)
    {
        if (handle == p->m_nStreamHandle)
        {
		    if (p->m_bSetup > 0)
		    {
		        m_lock.Unlock();
		        return -1;
		    }

            if (pLast == NULL)
            {
                m_pRtpSessionMgrList = p->m_pNext;
            }
            else
            {
                pLast->m_pNext = p->m_pNext;
            }
            break;
        }
        pLast = p;
        p = p->m_pNext;
    }

    if (p == NULL)
    {
        m_lock.Unlock();
        return -1;
    }

    p->SetDeleteFlag();
    p->m_pNext = m_pRtpSessionMgrDelList;
    m_pRtpSessionMgrDelList = p;
    m_lock.Unlock();
    // delete p;

    return 0;
}

int CRtspServer::CloseStream(int handle)
{
    CClientSocket  *pClient = NULL;
    int nRet = -1;
    m_lock.Lock();

    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (pClient->GetSessionID() == handle)
        {
            nRet = pClient->SetNeedClose();

            break;
        }
        pClient = pClient->m_pNext;
    }
    m_lock.Unlock();
    return nRet;
}

int CRtspServer::GetStreamCallBack(unsigned int dwSessionID,unsigned long *dwProp,ANTS_RTSPSERVER_STREAMDATA *fxn,void **pUser)
{
    CClientSocket *p;
    if (dwSessionID == -1)
    {
        if (dwProp)
        {
            *dwProp = m_dwStreamDataProp;
        }
        if (pUser)
        {
            *pUser = m_pStreamDataUser;
        }
        if (fxn)
        {
            *fxn = m_fStreamDataCallback;
        }
        return 0;
    }

    // 查找客户端

    //RTSP_DEBUG("close all.... \n");
    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        if (p->GetSessionID() == dwSessionID)
        {
            p->GetStreamCallBack(dwProp,fxn,pUser);
            break;
        }
        p = p->GetNext();
    }
    m_lock.Unlock();
    if (p == NULL)
    {
        return -1;
    }



    return 0;
}
int CRtspServer::SetStreamCallBack(unsigned int dwSessionID,unsigned long dwProp,ANTS_RTSPSERVER_STREAMDATA fxn,void *pUser)
{
    CClientSocket *p;
    if (dwSessionID == -1)
    {
        m_dwStreamDataProp = dwProp;
        m_fStreamDataCallback = fxn;
        m_pStreamDataUser = pUser;
        return 0;
    }

    // 查找客户端

    //RTSP_DEBUG("close all.... \n");
    m_lock.Lock();
    p = m_pClientSocketList;
    while(p != NULL)
    {
        if (p->GetSessionID() == dwSessionID)
        {
            p->SetStreamCallBack(dwProp,fxn,pUser);
            break;
        }
        p = p->GetNext();
    }
    m_lock.Unlock();
    if (p == NULL)
    {
        return -1;
    }


    return 0;
}

int CRtspServer::SetSupportMulticast(int hStreamHandle,int bSupportMulticast)
{
    int bMulticast = -1;
    CClientSocket *pClient = NULL;

    if (hStreamHandle < 0)
    {
        bMulticast = m_bSupportMulticast;
        m_bSupportMulticast = bSupportMulticast;
        return bMulticast;
    }

    m_lock.Lock();
    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (hStreamHandle == pClient->GetSessionID())
        {
            break;
        }
        pClient = pClient->m_pNext;
    }
    if (pClient != NULL)
    {
        bMulticast = pClient->SetSupportMulticast(bSupportMulticast);
        //RTSP_DEBUG("Check Multicast enable = %d\n",bSupportMulticast);
    }
    m_lock.Unlock();

    if(bMulticast == -1)
    {
        bMulticast = m_bSupportMulticast;
        m_bSupportMulticast = bSupportMulticast;
    }
    return bMulticast;
}


int CRtspServer::GetSupportMulticast(int hStreamHandle)
{
    int bMulticast = m_bSupportMulticast;
    CClientSocket *pClient = NULL;

    if (hStreamHandle < 0)
    {
        return m_bSupportMulticast;
    }
    m_lock.Lock();
    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (hStreamHandle == pClient->GetSessionID())
        {
            break;
        }
        pClient = pClient->m_pNext;
    }
    if (pClient != NULL)
    {
        bMulticast = pClient->GetSupportMulticast();
    }
    m_lock.Unlock();

    return bMulticast;
}


int CRtspServer::GetServerBasePortByHandle(int handle)
{
    int port = 0;
    CRtpSessionMgr *pMgr;
    if (handle < 0)
    {
        return 0;
    }
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (handle == pMgr->GetHandle())
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    if (pMgr != NULL)
    {
        port = pMgr->m_nVideoRtpPort;
    }
    m_lock.Unlock();

    return port;
}
int CRtspServer::GetServerBaseAppPortByHandle(int handle,int nIdx)
{
    int port = 0;
    CRtpSessionMgr *pMgr;
    if (handle < 0 || nIdx <0 || nIdx >= RTSP_APP_SUPPORT_MAX_NUM)
    {
        return 0;
    }
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (handle == pMgr->GetHandle())
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    if (pMgr != NULL)
    {
        if (pMgr->m_pAppMgr[nIdx] != NULL)
        {
            port = pMgr->m_pAppMgr[nIdx]->nAppRtpPort;
        }
    }
    m_lock.Unlock();

    return port;
}
int CRtspServer::GetServerBasePortAByHandle(int handle)
{
    int port = 0;
    CRtpSessionMgr *pMgr;
    if (handle < 0)
    {
        return 0;
    }
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (handle == pMgr->GetHandle())
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    if (pMgr != NULL)
    {
        port = pMgr->m_nAudioRtpPort;
    }
    m_lock.Unlock();
    return port;
}
unsigned int CRtspServer::GetAppSSRCByHandle(int handle,int nIdx)
{
    unsigned int uiSSRC = 0;
    CRtpSessionMgr *pMgr;
    if (handle < 0 || nIdx < 0 || nIdx >= RTSP_APP_SUPPORT_MAX_NUM)
    {
        return 0;
    }
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (handle == pMgr->GetHandle())
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    if (pMgr != NULL)
    {
        if (pMgr->m_pAppMgr[nIdx] != NULL)
        {
            uiSSRC = pMgr->m_pAppMgr[nIdx]->pSess->GetLocalSSRC();
        }

    }
    m_lock.Unlock();
    return uiSSRC;
}
unsigned int CRtspServer::GetSSRCByHandle(int handle,int bAudio)
{
    unsigned int uiSSRC = 0;
    CRtpSessionMgr *pMgr;
    if (handle < 0)
    {
        return 0;
    }
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (handle == pMgr->GetHandle())
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    if (pMgr != NULL)
    {
        if (bAudio)
        {
            if (pMgr->m_pAudioSess)
            {
                uiSSRC = pMgr->m_pAudioSess->GetLocalSSRC();
            }

        }
        else
        {
            if (pMgr->m_pVideoSess)
            {
                uiSSRC = pMgr->m_pVideoSess->GetLocalSSRC();
            }

        }
    }
    m_lock.Unlock();
    return uiSSRC;
}

void CRtspServer::SetSSRCByHandle(int handle,int bAudio,unsigned int uSSRC)
{
	unsigned int uiSSRC = 0;
	CRtpSessionMgr *pMgr;
	if (handle < 0)
	{
		return;
	}
	m_lock.Lock();
	pMgr = m_pRtpSessionMgrList;
	while(NULL != pMgr)
	{
		if (handle == pMgr->GetHandle())
		{
			break;
		}
		pMgr = pMgr->m_pNext;
	}
	if (pMgr != NULL)
	{
		if (bAudio)
		{
			if (pMgr->m_pAudioSess)
			{
				pMgr->m_pAudioSess->SetLocalSSRC(uSSRC);
			}

		}
		else
		{
			if (pMgr->m_pVideoSess)
			{
				pMgr->m_pVideoSess->SetLocalSSRC(uSSRC);
			}

		}
	}
	m_lock.Unlock();
	return ;
}

int CRtspServer::SetConfig(int handle,int nCmdType,void *pData,int nDataSize)
{
    CRtpSessionMgr *pMgr;
    int nNewValue = (uint64_t )pData;
    int nRet = -1;
    if (nCmdType == 9)
    {
        m_bOldRecording = !nNewValue;
        return 0;
    }

	if (nCmdType == 12 || nCmdType == 13 || nCmdType == 14)
	{
	    CClientSocket  *pClient = NULL;

	    m_lock.Lock();

	    pClient = m_pClientSocketList;
	    while(NULL != pClient)
	    {
	        if (pClient->GetSessionID() == handle)
	        {
	            nRet = pClient->SetConfig(nCmdType,pData,nDataSize);

	            break;
	        }
	        pClient = pClient->m_pNext;
	    }

	    m_lock.Unlock();

        return nRet;
	}
	else if (nCmdType == 17)
	{ // 端口复用
		int *pSocket = (int *)pData;
		if(pSocket == NULL || nDataSize  != sizeof(int))
		{
			return -1;
		}
		if (*pSocket == -1)
		{
			return -1;
		}
		return incomingConnectionHandler(0,*pSocket);
	}

    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->SetConfig(nCmdType,pData,nDataSize);

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();

    return nRet;
}
int CRtspServer::UDPData(CRtpSessionMgr *pSess, int nSessionID)
{
    CRtpSessionMgr *pMgr = NULL;
    void *pStreamUser = NULL;
    int nRet = -1, handle, bRecording;
    int nReadType = 0;
    int bAdpcm2G711u = 0;
    rtsp_stream_info nInfo;
    char *pBuffer = NULL;
    int dwBuffsize;
    int bSleep = 1,bDo = 0;
	int nVideoCnt = 0, nAudioCnt = 0;

    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    if (pSess != NULL)
    {
        pMgr = pSess;
    }
    else
    {
        pMgr = m_pRtpSessionMgrList;
    }

    while(NULL != pMgr)
    {
        if (pMgr->GetIsTCP() == 0 || pMgr->GetIsTCP() == 2)
        {
            pStreamUser = pMgr->GetStreamUser();

			if (pStreamUser != NULL)
		    {
                bDo = 1;
	            handle = pMgr->GetHandle();
	            bRecording = pMgr->IsRecording();

	            if (pMgr->GetIsTCP() == 2)
	            {
	                bRecording = 2;
	            }
                if (pMgr->IsWaitDelete())
                {
                    bDo = 0;
                }
				if (bDo)
				{
                    if (pMgr->GetCurSendSessionID() == 0)
                    {
                        //printf("[%s.%d] m_pRtpSess->SetCurSendSessionID()!!!!\n",__FUNCTION__,__LINE__);
                        pMgr->SetCurSendSessionID(nSessionID);
					}
					else if (pMgr->GetCurSendSessionID() == nSessionID)
					{

					}
					else
					{
                        bDo = 0;
					}
				}
                if(bDo)
                {
                    nVideoCnt = 0;
					nAudioCnt = 0;
			        if (pMgr->GetVideo()!= NULL)
			        {
			            nVideoCnt = pMgr->GetVideo()->GetDestinationCnt();
			        }

			        if (pMgr->GetAudio()!= NULL)
			        {
			            nAudioCnt = pMgr->GetAudio()->GetDestinationCnt();
			        }

                    pMgr->SetUsed(1);
                }

                m_lock.Unlock();
                if(bDo)
                {
                    if (nVideoCnt > 0 || nAudioCnt > 0)
                    {
		                if (0 == fRead(handle,pStreamUser,bRecording,&pBuffer, &dwBuffsize, &nInfo, &nReadType, &bAdpcm2G711u))
		                {
		                    if (nReadType == 1)
		                    {
#if 0
		                        if (NULL != pMgr->GetPoken())
		                        {
		                            printf("UDPData before[%s.%d]dwBuffsize = %d[%s]\n",__FUNCTION__,__LINE__,dwBuffsize,pMgr->GetPoken());
		                        }
								else
								{
                                    printf("UDPData before[%s.%d]dwBuffsize = %d\n",__FUNCTION__,__LINE__,dwBuffsize);
								}
#endif
								if (pMgr->IsAntsComb() == 1)
								{
									InputAntsCombData(handle, 0, nInfo.wCh + 1, nInfo.byStream, pBuffer, dwBuffsize);
								}
								else
								{
									InputAntsData(handle, pBuffer, dwBuffsize, bAdpcm2G711u);
								}
#if 0
								if (NULL != pMgr->GetPoken())
								{
		                            printf("UDPData after[%s.%d]dwBuffsize = %d[%s]\n",__FUNCTION__,__LINE__,dwBuffsize,pMgr->GetPoken());
								}
								else
								{
                                    printf("UDPData before[%s.%d]dwBuffsize = %d\n",__FUNCTION__,__LINE__,dwBuffsize);
								}
#endif
		                    }
		                    else if (nReadType == 2)
		                    {
		                        //m_pRtspServer->InputData();
		                    }

		                    fRelease(handle,pStreamUser,bRecording);
		                    bSleep = 0;
		                }
                    }
					else
					{
					    if (pMgr->GetIsTCP() == 0)
					    {
                          //  printf("[%s.%d] nVideoCnt = %d nAudioCnt = %d\n",__FUNCTION__,__LINE__,nVideoCnt,nAudioCnt);
					    }
					}
                }
                 m_lock.Lock();

                if(bDo)
                {
                    pMgr->SetUsed(0);
                }
			}
        }
        if (pSess != NULL)
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();

    return bSleep;
}




int CRtspServer::AddAppStream(int handle,int nAppPayloadType,unsigned int dwClockRate,char *szAppName)
{
    CClientSocket *pClient = NULL;
    int nRet = -1;

    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (pClient->GetSessionID() == handle)
        {
            nRet = pClient->AddAppStream(nAppPayloadType,dwClockRate,szAppName);
            break;
        }
        pClient = pClient->m_pNext;
    }
    m_lock.Unlock();

    return nRet;
}
int CRtspServer::DeleteAppStream(int handle,int nAppPayloadType)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;


    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->DeleteAppStream(nAppPayloadType);

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();

    return nRet;
}
int CRtspServer::InputDataEx(int handle,int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;

    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }

    // printf("line = %d handle =%d pdata=%x,size = %d,time = %lld,bau = %d,badpcm=%d\n",__LINE__, handle,pData, nDataSize, timestamp, bAudio, bAdpcm2G711u);

    if (pMgr == NULL)
    {
        m_lock.Unlock();
        return -1;
    }
    if (pMgr->IsWaitDelete())
    {
        m_lock.Unlock();
        return -1;
    }
    pMgr->SetUsed(1);
    m_lock.Unlock();

	nRet = pMgr->InputDataEx(nPayloadType,nFrameType,pData,nDataSize,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);

	m_lock.Lock();
    pMgr->SetUsed(0);
    m_lock.Unlock();

    return nRet;
}

int CRtspServer::InputAntsCombData(int handle,int nType,int nChan,int nStreamIdx,void *pData,int nDataSize)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;

    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }

    // printf("line = %d handle =%d pdata=%x,size = %d,time = %lld,bau = %d,badpcm=%d\n",__LINE__, handle,pData, nDataSize, timestamp, bAudio, bAdpcm2G711u);

    if (pMgr == NULL)
    {
        m_lock.Unlock();
        return -1;
    }
    if (pMgr->IsWaitDelete())
    {
        m_lock.Unlock();
        return -1;
    }
    pMgr->SetUsed(1);
    m_lock.Unlock();

	nRet = pMgr->InputAntsCombData(nType,nChan,nStreamIdx,pData,nDataSize);

	m_lock.Lock();
    pMgr->SetUsed(0);
    m_lock.Unlock();

	return nRet;
}

int CRtspServer::GetAppStreamInfo(int handle,int nIdx,int *nAppPayloadType,unsigned int *dwClockRate,char **szAppName)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;


    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetAppStreamInfo(nIdx,nAppPayloadType,dwClockRate,szAppName);

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();

    return nRet;
}

int CRtspServer::IsAutoPayloadType(int handle,int bAudio)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;


    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->IsAutoPayloadType(bAudio);

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();

    return nRet;
}

int CRtspServer::InputData(int handle,int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{

    CRtpSessionMgr *pMgr;
    int nRet = -1;


    //printf("line = %d\n",__LINE__);
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }

    // printf("line = %d handle =%d pdata=%x,size = %d,time = %lld,bau = %d,badpcm=%d\n",__LINE__, handle,pData, nDataSize, timestamp, bAudio, bAdpcm2G711u);



    if (pMgr == NULL)
    {
        m_lock.Unlock();
        return -1;
    }
    if (pMgr->IsWaitDelete())
    {
        m_lock.Unlock();
        return -1;
    }
    pMgr->SetUsed(1);
    m_lock.Unlock();

    pMgr->InputData(nPayloadType,nFrameType,pData,nDataSize,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);

    m_lock.Lock();
    pMgr->SetUsed(0);
    m_lock.Unlock();
    // printf("line = %d\n",__LINE__);

    return 0;

}

int CRtspServer::InputAntsData(int handle,void *pData,int nDataSize,int bAdpcm2G711u)
{
    int nRet = -1;
    int i = 0;
    AntsFrameHeader framehead;
    int64_t timeStamp = 0;
    int nPlayloadType = -1,bAudio = 0,nStreamType = -1, nClockRate = -1;
    int bRelTimeStamp = 0;
    char *pPlayloadName = NULL;

    if (pData == NULL)
    {
        return nRet;
    }
    //RTSP_DEBUG("[%s.%d]nDataSize = %d\n",__FUNCTION__,__LINE__,nDataSize);
    do
    {
        memcpy(&framehead,pData,sizeof(AntsFrameHeader));
        //framehead= (AntsFrameHeader *)pData;

        if (framehead.uiStartId != ANTS_FRAME_STARTCODE)
        {
            //RTSP_ERROR("[%s.%d]start id = 0x%x\n",__FUNCTION__,__LINE__,framehead.uiStartId);
            return nRet;
        }
        if (framehead.uiFrameLen > nDataSize - sizeof(AntsFrameHeader))
        {
            RTSP_ERROR("[%s.%d]uiFrameLen = %d nDataSize = %d ,%d\n",__FUNCTION__,__LINE__,framehead.uiFrameLen , nDataSize , sizeof(AntsFrameHeader));
            return nRet;
        }
        bRelTimeStamp |= 2;
        if (framehead.uiFrameType == AntsPktIFrames ||
            framehead.uiFrameType == AntsPktPFrames ||
            framehead.uiFrameType == AntsPktSubIFrames ||
            framehead.uiFrameType == AntsPktSubPFrames ||
			framehead.uiFrameType == AntsPktAudioFrames||
			framehead.uiFrameType == AntsPktThirdIFrames||
			framehead.uiFrameType == AntsPktThirdPFrames ||
			framehead.uiFrameType == AntsPktVIFrames ||
			framehead.uiFrameType == AntsPktSubVIFrames ||
			framehead.uiFrameType == AntsPktThirdVIFrames)
        {


            timeStamp = framehead.uiFrameTime * 1000 + framehead.uiFrameTickCount/1000;
            if (framehead.uiFrameType == AntsPktAudioFrames)
            {
                if (framehead.uMedia.struAudioHeader.cCodecId == 1)
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_G711A;
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_G711A;
                    nClockRate = 8000;
                    pPlayloadName="PCMA";
                }
                else if (framehead.uMedia.struAudioHeader.cCodecId == AntsADPCM)
                {

                }
                else if (framehead.uMedia.struAudioHeader.cCodecId == AntsAAC)
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_AAC;
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_AAC;
                    nClockRate = 8000;
                    pPlayloadName="mpeg4-generic";

                }
                else
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_G711U;
                    nClockRate = 8000;
                    pPlayloadName="PCMU";
                }
                bAudio = 1;
            }
            else
            {

                if (framehead.uMedia.struVideoHeader.cCodecId == AntsMJPEG_hisi)
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG;
                    bRelTimeStamp |= 1;
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG;
                    nClockRate = 90000;
                    pPlayloadName="JPEG";
                }
                else if (framehead.uMedia.struVideoHeader.cCodecId == AntsH265)
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_H265;
                    bRelTimeStamp |= 1;
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H265;
                    nClockRate = 90000;
                    pPlayloadName="H265";
                }
                else if (framehead.uMedia.struVideoHeader.cCodecId == AntsH265_RTP_PACK)
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_H265;
                    bRelTimeStamp |= 1;
                    bRelTimeStamp |= (1 << 31);
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H265;
                    nClockRate = 90000;
                    pPlayloadName="H265";
                }
                else
                {
                    nPlayloadType = ANTS_RTSPSERVER_PAYLOADTYPE_H264;
                    if (framehead.uMedia.struVideoHeader.cCodecId == AntsH264_hisi_RTP)
                    {
                        bRelTimeStamp |= 1;
                    }
                    if (framehead.uMedia.struVideoHeader.cCodecId == AntsH264_RTP_PACK)
                    {
                        bRelTimeStamp |= 1;
                        bRelTimeStamp |= (1 << 31);
                    }
                    nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H264;
                    nClockRate = 90000;
                    pPlayloadName="H264";

                }
                if (m_bRelTimeStamp != bRelTimeStamp)
                {

                    m_bRelTimeStamp = bRelTimeStamp;
                    //ChangeTimeStampType(handle);
                }


            }
            //m_bRelTimeStamp = 1;
            if (m_bRelTimeStamp)
            {
                timeStamp = framehead.uiTimeStamp;
            }
            if (nStreamType == -1)
            {
               break;
            }
            //printf("..............%d .........bAudio = %d cCodecId = %d\n",nPlayloadType,bAudio);
            SetPayloadStreamType(handle,nStreamType,nPlayloadType,nClockRate,pPlayloadName,bAudio);

            //  if (framehead->uiFrameType == AntsPktAudioFrames)
            //  {
            //      RTSP_DEBUG("[%d] %d\n",framehead->uiFrameNo,framehead->uiFrameTime * 1000+framehead->uiFrameTickCount/1000);
            //  }
            //RTSP_DEBUG("[%s.%d]type = %d %lld %u %u\n",__FUNCTION__,__LINE__,framehead->uiFrameType,timeStamp,timeStampL,timeStampH);
            if (framehead.uiFrameType == AntsPktAudioFrames)
            {
                //RTSP_DEBUG("[%s.%d]type = %d %lld %u %d %d\n",__FUNCTION__,__LINE__,framehead.uiFrameType,timeStamp,framehead.uiFrameLen, framehead.uiFrameNo,framehead.uMedia.struAudioHeader.cCodecId);
            }
            nRet = InputData(handle,-1,framehead.uiFrameType,
                             (void *)((char *)pData + sizeof(AntsFrameHeader)) ,
                             framehead.uiFrameLen,
                             framehead.uiTimeStamp,
                             framehead.uiFrameTime,
                             framehead.uiFrameTickCount,bRelTimeStamp);
            if (nRet)
            {
                break;
            }

            // checkerror(nRet);
        }
        pData = (char *)pData + framehead.uiFrameLen + sizeof(AntsFrameHeader);
        nDataSize -= framehead.uiFrameLen + sizeof(AntsFrameHeader);
        if (nDataSize <= 0)
        {
            break;
        }

    }
    while(1);

    return nRet;

}

uint32_t CRtspServer::GetStreamTimeOut(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = 60;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetTimeOut();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}

int CRtspServer::AddRtpDataClient(int handle,unsigned int uiIPv4[4],int nPort,int nCh,int bAudio,int bTCP,void *pClient,int bIPv6)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->AddDestination(uiIPv4,nPort,nCh,bAudio,bTCP,pClient,bIPv6);

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();
    return nRet;
}
int CRtspServer::RemoveRtpDataClient(int handle,unsigned int uiIPv4[4],int nPort,int nCh,int bAudio ,int bTCP ,void *pClient)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;

    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;

    while(NULL != pMgr)
    {

        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->DeleteDestination(uiIPv4,nPort,nCh,bAudio,bTCP,pClient);

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();
    return nRet;

}

int CRtspServer::SetMulticastAddressIPv4(int handle,unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio,int bIPv6)
{
    CClientSocket  *pClient = NULL;
    int nRet = -1;
    m_lock.Lock();

    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (pClient->GetSessionID() == handle)
        {
			if (pClient->IsIPv6())
			{
				if (bIPv6)
				{
					nRet = pClient->SetMulticastIPv4(uiIPv4,nPort,nTTL,bAudio);
				}
			}
			else if (!bIPv6)
			{
					nRet = pClient->SetMulticastIPv4(uiIPv4,nPort,nTTL,bAudio);
			}


            break;
        }
        pClient = pClient->m_pNext;
    }
    m_lock.Unlock();
    return nRet;
}


int CRtspServer::SetStreamInfo(int handle,int bStreamType,int nVideoPayloadType,int nAudioPayloadType)
{
    CClientSocket  *pClient = NULL;
    int nRet = -1;
    m_lock.Lock();

    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (pClient->GetSessionID() == handle)
        {
            nRet = pClient->SetStreamInfo(bStreamType,nVideoPayloadType,nAudioPayloadType);

            break;
        }
        pClient = pClient->m_pNext;
    }
    m_lock.Unlock();
    return nRet;
}


int CRtspServer::SetMulticastAddressIPv4NoApply(int handle,unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio,int bIPv6)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;
    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->StartMulticastIPv4NoApply(uiIPv4,nPort,nTTL,bAudio,bIPv6);
            if (!nRet)
            {
                // 断开所有此流的多播客户端
                CClientSocket *p,*pDel;
                //RTSP_DEBUG("close all.... \n");
                p = m_pClientSocketList;
                while(p != NULL)
                {
                    pDel = p;
                    p = p->GetNext();
                    if (pDel->GetRtpSessHandle() == pMgr->GetHandle())
                    {
                        if (pDel->IsMulticast())
                        {
                            RemoveClientSocket(pDel->GetSocket());
                        }

                    }

                }
            }

            break;
        }
        pMgr = pMgr->m_pNext;
    }
    m_lock.Unlock();
    return nRet;
}

int CRtspServer::GetMulticastAddressIPv4(int handle,unsigned int *uiIPv4,int *nPort,int *nTTL,int bAudio,int bIPv6)
{
    CClientSocket  *pClient = NULL;
    int nRet = -1;
    m_lock.Lock();
    pClient = m_pClientSocketList;
    while(NULL != pClient)
    {
        if (pClient->GetSessionID() == handle)
        {
			if (pClient->IsIPv6())
			{
				if (bIPv6)
				{
					nRet = pClient->GetMulticastIPv4(uiIPv4,nPort,nTTL,bAudio,NULL);
				}
			}
			else if(!bIPv6)
			{
				nRet = pClient->GetMulticastIPv4(uiIPv4,nPort,nTTL,bAudio,NULL);
			}


            break;
        }
        pClient = pClient->m_pNext;
    }
    m_lock.Unlock();
    return nRet;
}
int CRtspServer::AddHttpSessionCookie(CClientSocket *pGet,char *pCookie)
{
    CRTSPOverHttpSessionCookie *p;
    if (pCookie == NULL)
    {
        return -1;
    }

    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        if (!strcmp(pCookie,p->m_pXsessioncookie))
        {
            break;
        }
        p = p->m_pNext;
    }
    if (p != NULL)
    {
        //存在
        p->AddOutputClient(pGet);
        return 0;
    }
    p = new CRTSPOverHttpSessionCookie(pGet,NULL,pCookie);
    if (p == NULL)
    {

        return -1;
    }
    if (p->m_pXsessioncookie == NULL)
    {
        delete p;

        return -1;
    }
    p->m_pNext = m_pHttpSessionCookieList;
    m_pHttpSessionCookieList = p;


    return 0;
}

int CRtspServer::AddPostHttpSessionCookie(CClientSocket *pPost,char *pCookie)
{
    CRTSPOverHttpSessionCookie *p;
    if (pCookie == NULL)
    {
        return -1;
    }

    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        if (!strcmp(pCookie,p->m_pXsessioncookie))
        {
            break;
        }
        p = p->m_pNext;
    }
    if (p != NULL)
    {
        //存在
        p->AddInputClient(pPost);
        return 0;
    }
    p = new CRTSPOverHttpSessionCookie(NULL,pPost,pCookie);
    if (p == NULL)
    {

        return -1;
    }
    if (p->m_pXsessioncookie == NULL)
    {
        delete p;

        return -1;
    }
    p->m_pNext = m_pHttpSessionCookieList;
    m_pHttpSessionCookieList = p;


    return 0;
}
CClientSocket *CRtspServer::GetOutPutClientByHttpSessionCookie(char *pCookie)
{
    CClientSocket *pOutPut = NULL;
    CRTSPOverHttpSessionCookie *p;
    if (pCookie == NULL)
    {
        return NULL;
    }

    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        if (!strcmp(pCookie,p->m_pXsessioncookie))
        {
            break;
        }
        p = p->m_pNext;
    }
    if (p != NULL)
    {
        //存在
        pOutPut = p->m_pGet;

    }


    return pOutPut;
}
CClientSocket *CRtspServer::GetInPutClientByHttpSessionCookie(char *pCookie)
{
    CClientSocket *pInPut = NULL;
    CRTSPOverHttpSessionCookie *p;
    if (pCookie == NULL)
    {
        return NULL;
    }

    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        if (!strcmp(pCookie,p->m_pXsessioncookie))
        {
            break;
        }
        p = p->m_pNext;
    }
    if (p != NULL)
    {
        //存在
        pInPut = p->m_pPost;

    }


    return pInPut;
}
int CRtspServer::RemoveHttpSessionCookie(char *pCookie)
{
    CRTSPOverHttpSessionCookie *p,*plast = NULL;
    if (pCookie == NULL)
    {
        return 0;
    }

    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {


        if (!strcmp(pCookie,p->m_pXsessioncookie))
        {
            //删除掉
            if (plast == NULL)
            {
                //
                m_pHttpSessionCookieList = p->m_pNext;
            }
            else
            {
                plast->m_pNext = p->m_pNext;
            }

            delete p;
            break;
        }
        plast = p;
        p = p->m_pNext;
    }



    return 0;
}
CClientSocket *CRtspServer::GetOtherClientInHttpSessionByClient(CClientSocket *pClient)
{
    CRTSPOverHttpSessionCookie *p;
    CClientSocket *pOther = NULL;
    if (pClient == NULL)
    {
        return NULL;
    }
    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        if (pClient == p->m_pGet || pClient == p->m_pPost)
        {

            if (pClient == p->m_pGet)
            {
                pOther = p->m_pPost;
            }
            else
            {
                pOther = p->m_pGet;
            }

            break;
        }
        p = p->m_pNext;
    }


    return pOther;
}
int CRtspServer::RemoveHttpSessionCookieByClient(CClientSocket *pClient)
{
    CRTSPOverHttpSessionCookie *p,*plast = NULL;
    CClientSocket *pDelOther = NULL;
    if (pClient == NULL)
    {
        return 0;
    }
    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {


        if (pClient == p->m_pGet || pClient == p->m_pPost)
        {
            //删除掉
            if (plast == NULL)
            {
                //
                m_pHttpSessionCookieList = p->m_pNext;
            }
            else
            {
                plast->m_pNext = p->m_pNext;
            }

            delete p;
            break;
        }
        plast = p;
        p = p->m_pNext;
    }


    return 0;
}
int CRtspServer::RemoveAllHttpSessionCookie()
{
    CRTSPOverHttpSessionCookie *p,*pcurr = NULL;


    p = m_pHttpSessionCookieList;
    while(NULL != p)
    {
        pcurr = p;
        p = p->m_pNext;
        delete pcurr;
    }
    m_pHttpSessionCookieList = NULL;



    return 0;
}

int CRtspServer::GetSourceCnt(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetSourceCnt();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}
int CRtspServer::HasVideo(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->HasVideo();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}
int CRtspServer::HasAudio(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->HasAudio();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}

int CRtspServer::GetStreamType(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetStreamType();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}

int CRtspServer::GetSPS(int handle,char *pSPSData,int nBuffSize)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetSPS(pSPSData,nBuffSize);

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}


int CRtspServer::GetPPS(int handle,char *pPPSData,int nBuffSize)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetPPS(pPPSData,nBuffSize);

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}

char * CRtspServer::GetPPS_Base64(int handle)
{
    CRtpSessionMgr *pMgr;
    char * pRet = NULL;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            pRet = pMgr->GetPPS_Base64();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return pRet;
}
char * CRtspServer::GetSPS_Base64(int handle)
{
    CRtpSessionMgr *pMgr;
    char * pRet = NULL;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            pRet = pMgr->GetSPS_Base64();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return pRet;
}

char * CRtspServer::GetVPS_Base64(int handle)
{
    CRtpSessionMgr *pMgr;
    char * pRet = NULL;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            pRet = pMgr->GetVPS_Base64();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return pRet;
}
int CRtspServer::IsVideoReady(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = 0;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->IsVideoReady();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}
int CRtspServer::GetProfileLevelID(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetProfileLevelID();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}

int CRtspServer::GetPayloadStreamType(int handle,int *nStreamType,int *nPlayloadType,int *nClockRate,char *pPlayloadName,int bAudio)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName,bAudio);

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}

int CRtspServer::SetPayloadStreamType(int handle,int nStreamType,int nPlayloadType,int nClockRate,char *pPlayloadName,int bAudio)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;
    int nPlayloadTypeOld,nStreamTypeOld,nClockRateOld;
    char szPlayloadName[32];

    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }

    if (pMgr == NULL)
    {
        m_lock.Unlock();
        return -1;
    }
    if (pMgr->IsWaitDelete())
    {
        m_lock.Unlock();
        return -1;
    }
    pMgr->SetUsed(1);
    m_lock.Unlock();

    if (pMgr->IsAutoPayloadType(bAudio))
    {
        szPlayloadName[0] = 0;
        pMgr->GetPayloadStreamType(&nStreamTypeOld,&nPlayloadTypeOld,&nClockRateOld,szPlayloadName,bAudio);
        if (nPlayloadTypeOld != nPlayloadType ||
            nStreamTypeOld != nStreamType ||
            nClockRateOld != nClockRate ||
            strcmp(szPlayloadName,pPlayloadName))
        {
            if(nPlayloadTypeOld != -1)
            {
                //RemoveAllHttpSessionCookie();
                //RemoveAllClientSocket();
                //RemoveAllNoReadyClientSocket();
            }

            nRet = pMgr->SetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName,bAudio);
        }
    }

    m_lock.Lock();
    pMgr->SetUsed(0);
    m_lock.Unlock();

    return nRet;
}


int CRtspServer::GetPayloadType(int handle,int bAudio)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            nRet = pMgr->GetPayloadType(bAudio);

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}
int CRtspServer::SetPayloadType(int handle,int nPlayloadType,int bAudio)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;
    int nPlayloadTypeOld;

    m_lock.Lock();
    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            break;
        }
        pMgr = pMgr->m_pNext;
    }

    if (pMgr == NULL)
    {
        m_lock.Unlock();
        return -1;
    }
    if (pMgr->IsWaitDelete())
    {
        m_lock.Unlock();
        return -1;
    }
    pMgr->SetUsed(1);
    m_lock.Unlock();

    if (pMgr->IsAutoPayloadType(bAudio))
    {
	    nPlayloadTypeOld = pMgr->GetPayloadType(bAudio);
	    if (nPlayloadTypeOld != nPlayloadType)
	    {
	        if(nPlayloadTypeOld != -1)
	        {
	            //RemoveAllHttpSessionCookie();
	            //RemoveAllClientSocket();
	            //RemoveAllNoReadyClientSocket();
	        }

	        nRet = pMgr->SetPayloadType(nPlayloadType,bAudio);
	    }
    }

    m_lock.Lock();
    pMgr->SetUsed(0);
    m_lock.Unlock();

    return nRet;
}
int CRtspServer::ChangeTimeStampType(int handle)
{
    CRtpSessionMgr *pMgr;
    int nRet = -1;
    int nPlayloadTypeOld;

    pMgr = m_pRtpSessionMgrList;
    while(NULL != pMgr)
    {
        if (pMgr->GetHandle() == handle)
        {
            RemoveAllNoReadyClientSocket();

            break;
        }
        pMgr = pMgr->m_pNext;
    }

    return nRet;
}


char *CRtspServer::GetPayloadTypeName(int nPayloadType)
{
    int nCnt,i;
    nCnt = sizeof(m_tPayloadTypes)/sizeof(PAYLOADTYPEMAP_T);
    for (i = 0; i < nCnt; i++)
    {
        if (m_tPayloadTypes[i].nPayloadType == nPayloadType)
        {
            return m_tPayloadTypes[i].Name;
        }
    }
    return NULL;
}
int CRtspServer::GetPayloadClockRate(int nPayloadType)
{
    int nCnt,i;
    nCnt = sizeof(m_tPayloadTypes)/sizeof(PAYLOADTYPEMAP_T);
    for (i = 0; i < nCnt; i++)
    {
        if (m_tPayloadTypes[i].nPayloadType == nPayloadType)
        {
            return m_tPayloadTypes[i].clockRate;
        }
    }
    return 0;
}


void CRtspServer::SetStreamControlCallBack(RTSPServer_StreamControl *pStreamControl, void *pUser)
{
    fxnOpen = pStreamControl->fxnOpen;
    fxnClose = pStreamControl->fxnClose;
    fxnRead = pStreamControl->fxnRead;
    fxnRelease = pStreamControl->fxnRelease;
    fxnControl = pStreamControl->fxnControl;
    fxnUser = pUser;

    if (fxnOpen != NULL && fxnClose != NULL && fxnRead != NULL && fxnRelease != NULL)
    {
        fxnMode = 1;
    }
}

int CRtspServer::fOpen(int hStreamHandle,void *pInParam,void *pOutParam,void **pStreamUser)
{
    if (fxnOpen != NULL)
    {
        return (*fxnOpen)(m_hRtspHandle,hStreamHandle,pInParam,pOutParam,pStreamUser,fxnUser);
    }
    return -1;//未处理
}

int CRtspServer::fClose(int hStreamHandle,void *pStreamUser,int nStreamType,char *pUrl)
{
    if (fxnClose != NULL)
    {
        return (*fxnClose)(m_hRtspHandle,hStreamHandle,pStreamUser,nStreamType,pUrl);
    }
    return -1;//未处理
}

int CRtspServer::fRead(int hStreamHandle,void *pStreamUser,int nStreamType, char **pData,int *pDataSize,void *pStreamInfo, int *nReadType, int *bAdpcm2G711u)
{
    if (fxnRead != NULL)
    {
        return (*fxnRead)(m_hRtspHandle,hStreamHandle,pStreamUser,nStreamType,pData,pDataSize,pStreamInfo,nReadType,bAdpcm2G711u);
    }
    return -1;//未处理
}

int CRtspServer::fRelease(int hStreamHandle,void *pStreamUser,int nStreamType)
{
    if (fxnRelease != NULL)
    {
        return (*fxnRelease)(m_hRtspHandle,hStreamHandle,pStreamUser,nStreamType);
    }
    return -1;//未处理
}


int CRtspServer::fControl(int hStreamHandle,int nType,void *pInParam,void *pOutParam,void *pStreamUser)
{
    if (fxnControl != NULL)
    {
        return (*fxnControl)(m_hRtspHandle,hStreamHandle,nType,pInParam,pOutParam,pStreamUser);
    }
    return -1;//未处理
}

void CRtspServer::SetStatusCallBack(ANTS_RTSPSTATUSINTERCALLBACK fxn)
{
    m_fStatusCallBack = fxn;
}
void CRtspServer::SetSendTCPDataCallBack(ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn,void *pUser)
{
    m_fTCPDataCallBack = fxn;
    m_pRtpRtcpCallbackUser = pUser;
}
int CRtspServer::fStatusCallBack(int hStreamHandle,int nType, void *pInParam,void *pOutParam)
{
    if (m_fStatusCallBack != NULL)
    {
        return (*m_fStatusCallBack)(this,hStreamHandle,nType,pInParam,pOutParam);
    }
    return -1;//未处理
}
int CRtspServer::fStatusCallBack(int nType, void *pInParam,void *pOutParam)
{
    if (m_fStatusCallBack != NULL)
    {
        return (*m_fStatusCallBack)(this,0,nType,pInParam,pOutParam);
    }
    return -1;//未处理
}

void CRtspServer::IncWriteLen(int len)
{


    m_nTotalWriteSize += len;
    if (m_nTotalWriteSize > 0  && m_bWriteWait)
    {
        m_hWriteSem.Post();
    }

}



void CRtspServer::DecWriteLen(int len)
{


    m_nTotalWriteSize -= len;
    if (m_nTotalWriteSize < 0 )
    {
        m_nTotalWriteSize = 0;
    }
    if (m_nTotalWriteSize > 0 && m_bWriteWait)
    {
        m_hWriteSem.Post();
    }

}
void CRtspServer::WriteWait()
{
    int bWait = 0;

    if (m_nTotalWriteSize <= 0)
    {
        m_bWriteWait = 1;
    }


    if(m_bWriteWait)
    {
        m_hWriteSem.Pend();
    }


    m_bWriteWait = 0;

}



unsigned int CRtspServer::GetLocalIP()
{

    struct sockaddr_in addr;
    int nRet;
    socklen_t addrlen;
    addrlen = sizeof(addr);
    nRet = getsockname(m_nSocket,(struct sockaddr *)&addr,&addrlen);
    if (nRet)
    {
        return 0;
    }
    return addr.sin_addr.s_addr;
}



int CRtspServer::SendRTP_RTCPData(CClientSocket *pClient,int nCh,char *pRTPHeader,int nRTPHeadLen,char *pAdd,int nAddLen,const void *pData,size_t len)
{
    CClientSocket *pSock;
    int bOwe = 0;
    m_lock.Lock();
    pSock = m_pClientSocketList;
    while(NULL != pSock)
    {
        // printf("line = %d pSock 0x%x\n",__LINE__,(int)pSock);
        if (pClient == pSock)
        {
            bOwe = 1;
            break;
        }

        //printf("line = %d\n",__LINE__);
        pSock = pSock->GetNext();
    }
    if ((!bOwe) || pSock == NULL)
    {
        m_lock.Unlock();
        return -1;
    }

    pSock->SetUsed(1);
    m_lock.Unlock();

	bOwe = pSock->SendRTP_RTCPDataV2(nCh,pRTPHeader,nRTPHeadLen,pAdd,nAddLen,pData,len);

	m_lock.Lock();
    pSock->SetUsed(0);
    m_lock.Unlock();

    return bOwe;
}


unsigned char CRtpSessionMgr::m_byStreamHandleCnt = 0;
CRtpSessionMgr::CRtpSessionMgr(char *pToken,int bFixed,int bTCP,ANTS_RTSP_RTPRTCP_DATAPACKETCALLBACK fxn ,void *pUser)
{
    m_pVideoSess = NULL;
    m_pAudioSess = NULL;
    m_pNext = NULL;
    m_bTransType = 0;
    m_bTCP = bTCP;
    m_nAudioRtpPort = 0;
    m_nVideoRtpPort = 0;
    m_nUseCnt = 0;
    m_bFixed = bFixed;
    m_nStreamHandle = -1;


    m_pToken = strdup(pToken);

    m_byStreamHandleCnt++;
    m_nStreamHandle = (uint64_t)this;
    m_nStreamHandle &= 0x7FFFFF;
    m_nStreamHandle = (m_byStreamHandleCnt & 0xFF) | (m_nStreamHandle << 8);
    m_nStreamHandle &= 0x7FFFFFFF;
    m_bRecording = 0;


    m_nTTL[0] = m_nTTL[1] = 255;
	memset(m_dwIPv4Multicast,0,sizeof(m_dwIPv4Multicast));
	memset(m_dwIPv4MulticastIPv6,0,sizeof(m_dwIPv4MulticastIPv6));
    m_dwIPv4Multicast[0][0] = 0x010101E0;
    m_dwIPv4Multicast[1][0] = 0x020101E0;
    m_nMulticastPort[0] = 5678;
    m_nMulticastPort[1] = 5680;
    m_nMulticastCnt[0] =
    m_nMulticastCnt[1] = 0;
	m_nMulticastCntIPv6[0] =
	m_nMulticastCntIPv6[1] = 0;
    m_fTCPDataCallBack = fxn;
    m_pRtpRtcpCallbackUser = pUser;
    m_bSupportMulticast = 1;
    m_nTimeOut = 60;//0-无限,default = 60
    m_dwRunTimeMSecond = 0;
    m_dwRunTimeSecond = 0;
    m_dwStreamDataProp = 0;
    m_pStreamDataUser = NULL;
    m_fStreamDataCallback = NULL;
    m_nVideoPayloadType = -1; // 自动
    m_nAudioPayloadType = -1;
    m_bVideoType_Auto = 1;
    m_bAudioType_Auto = 1;
    memset(m_pAppMgr,0,sizeof(m_pAppMgr));
    m_bUsing = 0;
    m_bWaitDelete = 0;
    m_bSetup = 0;
	pStreamUser = NULL;
	m_nSessionID = 0;
	m_bAntsComb = 0;
    RTSP_DEBUG("Create New Stream[%x][%s]\n",m_nStreamHandle,pToken?pToken:"nul");
}
void CRtpSessionMgr::SetCombFlag(int bAntsComb)
{
	m_bAntsComb = bAntsComb;
}

int  CRtpSessionMgr::IsAntsComb()
{
	return m_bAntsComb;
}

int CRtpSessionMgr::SetAutoPayloadType(int bAuto,int bAudio)
{
    if (bAudio)
    {
        m_bAudioType_Auto = bAuto;
    }
    else
    {
        m_bVideoType_Auto = bAuto;
    }
    return 0;
}
int CRtpSessionMgr::IsAutoPayloadType(int bAudio)
{
    return bAudio?m_bAudioType_Auto:m_bVideoType_Auto;
}



CRtpSessionMgr::~CRtpSessionMgr()
{
    int i;
    unsigned int rtptime = 5000;
    char *pstr= "Bye";

    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->BYEDestroy(rtptime,pstr,strlen(pstr));
        delete m_pVideoSess;
        m_pVideoSess = NULL;
    }
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->BYEDestroy(rtptime,pstr,strlen(pstr));
        delete m_pAudioSess;
        m_pAudioSess = NULL;
    }
    if (m_pToken != NULL)
    {
        RTSP_DEBUG("Destroy  Stream[%x][%s]\n",m_nStreamHandle,m_pToken);
        //delete []m_pToken;
        free(m_pToken);
        m_pToken = NULL;
    }
    m_pNext = NULL;
    for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
			if(m_pAppMgr[i]->pSess != NULL)
			{
				m_pAppMgr[i]->pSess->BYEDestroy(rtptime,pstr,strlen(pstr));
				delete m_pAppMgr[i]->pSess;
				m_pAppMgr[i]->pSess = NULL;
			}
			if(m_pAppMgr[i]->szAppName != NULL)
			{
				free(m_pAppMgr[i]->szAppName);
			}
            delete m_pAppMgr[i];
            m_pAppMgr[i]  = NULL;
        }

    }


}
int CRtpSessionMgr::GetStreamType()
{
    int nRet = 0;
    if (m_pAudioSess != NULL)
    {
        nRet |= 2;
    }
    if (m_pVideoSess != NULL)
    {
        nRet |= 1;
    }
    if (nRet == 3)
    {
        nRet = 0;
    }
    return nRet;
};

int CRtpSessionMgr::GetSPS(char *pSPSData,int nBuffSize)
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->GetSPS(pSPSData,nBuffSize);
    }
    return 0;
}
int CRtpSessionMgr::GetPPS(char *pPPSData,int nBuffSize)
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->GetPPS(pPPSData,nBuffSize);
    }
    return 0;
}

char * CRtpSessionMgr::GetPPS_Base64()
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->GetPPS_Base64();
    }
    return NULL;
}
char * CRtpSessionMgr::GetSPS_Base64()
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->GetSPS_Base64();
    }
    return NULL;
}
char * CRtpSessionMgr::GetVPS_Base64()
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->GetVPS_Base64();
    }
    return NULL;
}


int CRtpSessionMgr::GetProfileLevelID()
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->GetProfileLevelID();
    }
    return -1;
}
int CRtpSessionMgr::IsVideoReady()
{
    if (m_pVideoSess != NULL)
    {
        return m_pVideoSess->IsReady();
    }
    return 0;
}

int CRtpSessionMgr::GetPayloadStreamType(int *nStreamType,int *nPlayloadType,int *nClockRate,char *pPlayloadName,int bAudio)
{
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            return m_pAudioSess->GetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName);
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            return m_pVideoSess->GetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName);
        }
    }
    return -1;
}
int CRtpSessionMgr::SetPayloadStreamType(int nStreamType,int nPlayloadType,int nClockRate,char *pPlayloadName,int bAudio)
{
    if(bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName);

            m_pAudioSess->SetDefaultTimestampIncrement(40 * nClockRate/1000);
            m_pAudioSess->SetDefaultMark(false);
        }

    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName);

            m_pVideoSess->SetDefaultTimestampIncrement(40 * nClockRate/1000);
            m_pVideoSess->SetDefaultMark(false);

        }

    }
    return 0;
}

int CRtpSessionMgr::GetPayloadType(int bAudio)
{
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            return m_pAudioSess->GetDefaultPayloadType();
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            return m_pVideoSess->GetDefaultPayloadType();
        }
    }
    return -1;
}
int CRtpSessionMgr::SetPayloadType(int nPlayloadType,int bAudio)
{
    int nClockRate;
    char *pPlayloadName = NULL;
    int nStreamType;
    if(bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            if (nPlayloadType == 0)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_G711U;
                pPlayloadName="PCMU";
                nClockRate = 8000;
            }
            else if (nPlayloadType == 8)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_G711A;
                pPlayloadName="PCMA";
                nClockRate = 8000;
            }
            m_pAudioSess->SetDefaultTimestampIncrement(40 * nClockRate/1000);
            m_pAudioSess->SetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName);
            m_pAudioSess->SetDefaultMark(false);
        }

    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            if (nPlayloadType == 96)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H264;
                pPlayloadName="H264";
                nClockRate = 90000;
            }
            else if (nPlayloadType == 98)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H265;
                pPlayloadName="H265";
                nClockRate = 90000;
            }
            else if (nPlayloadType == 26)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG;
                pPlayloadName="JPEG";
                nClockRate = 90000;
            }
            else if (nPlayloadType == 110)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB;
                pPlayloadName="AntsComb";
                nClockRate = 90000;
            }
            else if (nPlayloadType == 106)
            {
                nStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_APP;
                nClockRate = 90000;
            }
            m_pVideoSess->SetDefaultTimestampIncrement(40 * nClockRate/1000);
            m_pVideoSess->SetPayloadStreamType(nStreamType,nPlayloadType,nClockRate,pPlayloadName);
            m_pVideoSess->SetDefaultMark(false);
        }

    }
    return 0;

}
int CRtpSessionMgr::CreateAudio(int nPayloadType,int bIPv6)
{

    uint16_t portbase;
    int status = -1;
    CAudioRtpSession *sess;
    unsigned int rtptime = 5000;
    char *pstr="Bye";
	int bTCP = 0;
    int nPlayloadStreamType;
    char *pPlayloadName = NULL;
    if (m_pAudioSess != NULL)
    {
        if (m_nAudioPayloadType == nPayloadType)
        {
            return 0;
        }
        sess = m_pAudioSess;
        m_pAudioSess = NULL;
        sess->BYEDestroy(rtptime,pstr,strlen(pstr));
        delete sess;
    }
    if (nPayloadType == -1)
    {
        return -1;
    }

    sess = new CAudioRtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
    {
        portbase -= portbase & 1;
        if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
        {
            break;
        }
        if (portbase == 0)
        {
            continue;
        }

		if (m_bTCP == 0 || m_bTCP == 2)
        {
            bTCP = 0;
		}
		else if (m_bTCP == 1 || m_bTCP == 3)
		{
            bTCP = 1;
		}

        //status = sess->Create(sessparams,&transparams);
        status = sess->Create(portbase,bTCP,m_fTCPDataCallBack,m_pRtpRtcpCallbackUser,NULL,bIPv6);
        if (!status)
        {
            m_nAudioRtpPort = portbase;
            break;
        }
        else
        {
            // RTSP_DEBUG("[%s.%d] port = %d failed[status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
        }

    }

    if (status)
    {
        delete sess;
        return -1;
    }
    sess->SetRecording(m_bRecording);
    m_pAudioSess = sess;
    m_nAudioPayloadType = nPayloadType;
    //SetPayloadType(nPayloadType,1);
    if (nPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G711A)
    {
        nPlayloadStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_G711A;
        pPlayloadName = "PCMA";
    }
    else if (nPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G711U)
    {
        nPlayloadStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_G711U;
        pPlayloadName = "PCMU";
    }

    SetPayloadStreamType(nPlayloadStreamType,nPayloadType,8000,pPlayloadName,1);


    return 0;
}

int CRtpSessionMgr::CreateVideo(int nPayloadType,int bIPv6)
{
    uint16_t portbase;
    int status = -1;
    CH264RtpSession *sess;
    unsigned int rtptime = 5000;
    char *pstr="Bye";
	int bTCP = 0;
    int nPlayloadStreamType;
    char *pPlayloadName = NULL;
    if (m_pVideoSess != NULL)
    {
        if (m_nVideoPayloadType == nPayloadType)
        {
            return 0;
        }
        sess = m_pVideoSess;
        m_pVideoSess = NULL;
        sess->BYEDestroy(rtptime,pstr,strlen(pstr));
        delete sess;
    }
    if (nPayloadType == -1)
    {
        return -1;
    }
    sess = new CH264RtpSession;
    if (sess == NULL)
    {
        return -1;
    }
    for (portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
    {
        portbase -= portbase & 1;
        if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
        {
            break;
        }
        if (portbase == 0)
        {
            continue;
        }

        if (m_bTCP == 0 || m_bTCP == 2)
        {
            bTCP = 0;
		}
		else if (m_bTCP == 1 || m_bTCP == 3)
		{
            bTCP = 1;
		}

        // status = sess->Create(sessparams,&transparams);
        status = sess->Create(portbase,bTCP,m_fTCPDataCallBack,m_pRtpRtcpCallbackUser,NULL,bIPv6);
        if (!status)
        {
            m_nVideoRtpPort = portbase;
            break;
        }
        else
        {
            //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
        }

    }

    if (status)
    {
        delete sess;
        return -1;
    }

    sess->SetRecording(m_bRecording);
    m_pVideoSess = sess;
    m_nVideoPayloadType = nPayloadType;
    if (nPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_H264)
    {
        nPlayloadStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H264;
        pPlayloadName = "H264";
    }
    else if (nPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_H265)
    {
        nPlayloadStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_H265;
        pPlayloadName = "H265";
    }
    else if (nPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG)
    {
        nPlayloadStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_JPEG;
        pPlayloadName = "JPEG";
    }
    else if (nPayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB)
    {
        nPlayloadStreamType = ANTS_RTSP_CALLBACKBYPE_STREAM_ANTSCOMB;
        pPlayloadName = "AntsComb";
    }

    SetPayloadStreamType(nPlayloadStreamType,nPayloadType,90000,pPlayloadName,0);
    //SetPayloadType(nPayloadType,0);

    return 0;

}
//nCmdType:
//          1- 设置流类型 ,ANTS_RTSPSERVER_STREAMTYPE_ALL/
//          2- 设置视频负载类型自动 1-自动,0-非自动
//          3- 设置音频负载类型自动 1-自动,0-非自动
//          4- 设置视频负载类型
//          5- 设置音频负载类型
//          6- 设置流为录像源。 0-实时流,1-历史流
//          7- 设置操作影响的play Seq; // 从回调中得到

int CRtpSessionMgr::SetConfig(int nCmdType,void *pData,int nDataSize)
{
    int nNewValue = (uint64_t)pData;
    if (nCmdType == 1)
    {
        if (nNewValue == ANTS_RTSPSERVER_STREAMTYPE_ALL)
        {
            CreateVideo(m_nVideoPayloadType);
            CreateAudio(m_nAudioPayloadType);
        }
        else if (nNewValue == ANTS_RTSPSERVER_STREAMTYPE_VIDEO)
        {
            CreateVideo(m_nVideoPayloadType);
            if (m_pAudioSess != NULL)
            {
                char *pstr="Bye";
                CAudioRtpSession *pSess;
                unsigned int rtptime = 5000;
                pSess = m_pAudioSess;
                m_pAudioSess = NULL;
                pSess->BYEDestroy(rtptime,pstr,strlen(pstr));
                delete pSess;
            }
        }
        else if (nNewValue == ANTS_RTSPSERVER_STREAMTYPE_AUDIO)
        {
            CreateAudio(m_nAudioPayloadType);
            if (m_pVideoSess != NULL)
            {
                CH264RtpSession *pSess;
                char *pstr="Bye";
                unsigned int rtptime = 5000;
                pSess = m_pVideoSess;
                m_pVideoSess = NULL;
                pSess->BYEDestroy(rtptime,pstr,strlen(pstr));
                delete pSess;
            }
        }
    }
    else if (nCmdType == 2)
    {
        m_bVideoType_Auto = nNewValue;
    }
    else if (nCmdType == 3)
    {
        m_bAudioType_Auto = nNewValue;
    }
    else if (nCmdType == 4)
    {
        SetPayloadType(nNewValue,0);
    }
    else if (nCmdType == 5)
    {
        SetPayloadType(nNewValue,1);
    }
    else if (nCmdType == 6)
    {
        // recording
        m_bRecording = nNewValue;
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetRecording(m_bRecording);
        }
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetRecording(m_bRecording);
        }
    }
    else if (nCmdType == 7)
    {
        // recording
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetRecordPlaySeq(nNewValue);
        }
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetRecordPlaySeq(nNewValue);
        }
    }
    else if (nCmdType == 8)
    {
        // send fail,and ask
       return -1;
    }
    else if (nCmdType == 10)
    {
        // 帧率
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetFrameRate(nNewValue);
        }
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetFrameRate(nNewValue);
        }
    }
    else if (nCmdType == 12)
    {
        // SPS
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetSPS((char *)pData,nDataSize);
        }
    }
    else if (nCmdType == 13)
    {
        // PPS
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetPPS((char *)pData,nDataSize);
        }
    }
    else if (nCmdType == 14)
    {
        // VPS
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetVPS((char *)pData,nDataSize);
        }
    }
    else if (nCmdType == 15)
    {
        // video ssrc id
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetSSRC((uint32_t)(uint64_t)pData);
        }
    }
    else if (nCmdType == 16)
    {
        // audio ssrc id
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetSSRC((uint32_t)(uint64_t)pData);
        }
    }
    else if (nCmdType == 18)
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SendRTCPData(pData,nDataSize);
        }
    }
    else if (nCmdType == 19)
    {
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SendRTCPData(pData,nDataSize);
        }
    }

    return 0;
}

int CRtpSessionMgr::AddAppStream(int nAppPayloadType,unsigned int dwClockRate,char *szAppName)
{
    int i,nFreeIdx = -1;
    char *pNewAppName;
    RTSP_APP_MGR_T *pMgr;
    int status = 0;
    if (szAppName == NULL)
    {
        return -1;
    }
    for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            if (m_pAppMgr[i]->byAppPayloadType == nAppPayloadType ||
                0 == strcmp(m_pAppMgr[i]->szAppName,szAppName ))
            {
                return i;
            }
        }
        else if(nFreeIdx == -1)
        {
            nFreeIdx = i;

        }
    }
    if (nFreeIdx == -1)
    {
        return -1;
    }
    pMgr = new RTSP_APP_MGR_T;
    if (pMgr == NULL)
    {
        return -1;
    }
    memset(pMgr,0,sizeof(RTSP_APP_MGR_T));
    pNewAppName = strdup(szAppName);
    if (pNewAppName == NULL)
    {
        delete pMgr;
        return -1;
    }
    pMgr->pSess = new CAppRtpSession;
    if (pMgr->pSess == NULL)
    {
        delete pMgr;
        free(pNewAppName);
        return -1;
    }
    //
    for (int portbase = RTSP_BASE_PORT_NUM;; portbase+=2)
    {
        portbase -= portbase & 1;
        if (portbase < RTSP_BASE_PORT_NUM && portbase >= RTSP_BASE_PORT_NUM - 2)
        {
            break;
        }
        if (portbase == 0)
        {
            continue;
        }



        // status = sess->Create(sessparams,&transparams);
        status = pMgr->pSess->Create(portbase,0,m_fTCPDataCallBack,m_pRtpRtcpCallbackUser);
        if (!status)
        {
            pMgr->nAppRtpPort = portbase;
            break;
        }
        else
        {
            //  RTSP_DEBUG("[%s.%d] port = %d failed [status:%d-%s]\n",__FUNCTION__,__LINE__,portbase,status,RTPGetErrorString(status).c_str());
        }

    }
    if (status)
    {
        delete pMgr->pSess;
        delete pMgr;
        free(pNewAppName);
        return -1;
    }
    pMgr->pSess->SetPayloadStreamType(ANTS_RTSP_CALLBACKBYPE_STREAM_APP,nAppPayloadType,dwClockRate,pNewAppName);
    pMgr->byAppPayloadType = nAppPayloadType;
    pMgr->dwClockRate = dwClockRate;
    pMgr->szAppName = pNewAppName;
    m_pAppMgr[nFreeIdx] = pMgr;
    return nFreeIdx;


}
int CRtpSessionMgr::DeleteAppStream(int nAppPayloadType)
{
    int i;
    RTSP_APP_MGR_T *pMgr = NULL;

    for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            if (m_pAppMgr[i]->byAppPayloadType == nAppPayloadType)
            {
                pMgr = m_pAppMgr[i];
                m_pAppMgr[i] = NULL;
                break;
            }
        }

    }
    if (pMgr == NULL)
    {
        return -1;
    }
    pMgr->pSess->BYEDestroy(0,NULL,0);
    delete pMgr->pSess;
    pMgr->pSess = NULL;
    free(pMgr->szAppName);
    pMgr->szAppName = NULL;

    delete pMgr;
    return 0;

}
int CRtpSessionMgr::InputDataEx(int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{
    int i;
    RTSP_APP_MGR_T *pMgr = NULL;
    int nRet = -1;

    for (i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            if (m_pAppMgr[i]->byAppPayloadType == nPayloadType)
            {
                pMgr = m_pAppMgr[i];
                break;
            }
        }

    }
    if (pMgr == NULL)
    {
        return -1;
    }
    if (pMgr->pSess != NULL)
    {
        nRet = pMgr->pSess->SendAppPacket(pData,nDataSize,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
    }


    return nRet;
}

int CRtpSessionMgr::InputAntsCombData(int nType,int nChan,int nStreamIdx,void *pData,int nDataSize)
{
    int nRet = -1;

    if (m_pVideoSess != NULL)
    {

        nRet = m_pVideoSess->SendAntsCombPacket(nType,nChan, nStreamIdx,pData,nDataSize);
    }

    return nRet;
}

int CRtpSessionMgr::GetAppStreamInfo(int nIdx,int *nAppPayloadType,unsigned int *dwClockRate,char **szAppName)
{
    if (nIdx < 0 || nIdx >= RTSP_APP_SUPPORT_MAX_NUM)
    {
        return -1;
    }
    if(m_pAppMgr[nIdx] == NULL)
    {
        return -1;
    }
    if (nAppPayloadType != NULL)
    {
        *nAppPayloadType = m_pAppMgr[nIdx]->byAppPayloadType;
    }
    if (dwClockRate != NULL)
    {
        *dwClockRate = m_pAppMgr[nIdx]->dwClockRate;
    }
    if (szAppName != NULL)
    {
        *szAppName = strdup(m_pAppMgr[nIdx]->szAppName);
    }
    return 0;
}

int CRtpSessionMgr::GetAppIndexByPayloadType(int nAppPayloadType)
{
    for (int i = 0; i < RTSP_APP_SUPPORT_MAX_NUM; i++)
    {
        if(m_pAppMgr[i] != NULL)
        {
            if(m_pAppMgr[i]->byAppPayloadType == nAppPayloadType)
            {
                return i;
            }
        }
    }
    return -1;
}

void CRtpSessionMgr::SetRecordExHeader_C(uint8_t c,int bAudio)
{
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetRecordExHeader_C(c);
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetRecordExHeader_C(c);
        }
    }
}
void CRtpSessionMgr::SetRecordExHeader_E(uint8_t e,int bAudio)
{
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetRecordExHeader_E(e);
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetRecordExHeader_E(e);
        }
    }
}
void CRtpSessionMgr::SetRecordExHeader_D(uint8_t d,int bAudio)
{
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetRecordExHeader_D(d);
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetRecordExHeader_D(d);
        }
    }
}
void CRtpSessionMgr::SetRecordPlaySeq(int nSeq)
{
    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->SetRecordPlaySeq(nSeq);
    }
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->SetRecordPlaySeq(nSeq);
    }
}
void CRtpSessionMgr::SetRecordAbsTimeStamp(uint32_t Sec,uint32_t uSec,int bAudio)
{
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {

            m_pAudioSess->SetRecordAbsTimeStamp(Sec,uSec);
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetRecordAbsTimeStamp(Sec,uSec);
        }

    }
    return ;
}


int CRtpSessionMgr::InputData(int nPayloadType,int nFrameType,void *pData,int nDataSize,uint32_t Reltimestamp,uint32_t AbstimestampSec,uint32_t AbstimestampUSec,int bTimeStampValid)
{
    int nRet = -1;
    if (nFrameType == ANTS_RTSPSERVER_FRAMETYPE_AUDIO)
    {
        if (m_pAudioSess != NULL)
        {
            nRet = m_pAudioSess->SendAudioPacket(pData,nDataSize,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
        }
    }
    else
    {
        if (m_pVideoSess != NULL)
        {

#if 0
            static FILE *pf = NULL;
            if (pf == NULL)
            {
                pf = fopen("xx.264","wb+");
            }
            if (pf != NULL)
            {
                fwrite(pData ,1,nDataSize,pf);
                fflush(pf);
            }
#endif

            nRet = m_pVideoSess->SendVideoPacket(nFrameType,pData,nDataSize,Reltimestamp,AbstimestampSec,AbstimestampUSec,bTimeStampValid);
        }

    }
    return nRet;
}
int CRtpSessionMgr::AddDestination(unsigned int uiIPv4[4],int nPort,int nCh,int bAudio ,int bTCP,void *pClient,int bIPv6)
{
    int nRet = -1;

    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            RTSP_DEBUG("AA--Add %d :%d   %d[%x]\n",uiIPv4,nPort,nCh,pClient);
            nRet = m_pAudioSess->AddDestination(uiIPv4,nPort,nPort + 1,bTCP,nCh,pClient,bIPv6);
            RTSP_DEBUG("AA-RETURN : %d\n",nRet);

        }

    }
    else
    {
        if(m_pVideoSess != NULL)
        {
            RTSP_DEBUG("VV--Add %d :%d  %d[%x]\n",uiIPv4,nPort,nCh,pClient);
            nRet = m_pVideoSess->AddDestination(uiIPv4,nPort,nPort + 1,bTCP,nCh,pClient,bIPv6);
            RTSP_DEBUG("VV-RETURN : %d\n",nRet);

        }
    }
    return nRet;

}

int CRtpSessionMgr::DeleteDestination(unsigned int uiIPv4[4],int nPort,int nCh,int bAudio,int bTCP,void *pClient)
{

    int nRet = -1;
    //  RTPIPv4Address addrV4(addr,port);
	RTSP_DEBUG("baudio = %d au=%p vs=%p\n",bAudio,m_pAudioSess,m_pVideoSess);
    if (bAudio)
    {
        if (m_pAudioSess != NULL)
        {
            RTSP_DEBUG("AA--DEL btcp=%d %x :%d ch = %d [%p]\n",bTCP,uiIPv4,nPort,nCh,pClient);
            nRet = m_pAudioSess->DeleteDestination(uiIPv4,nPort,bTCP,nCh,pClient);

        }

    }
    else
    {
        if(m_pVideoSess != NULL)
        {
            RTSP_DEBUG("VV--DEL btcp=%d %x :%d ch=%d[%p]\n",bTCP,uiIPv4,nPort,nCh,pClient);
            nRet = m_pVideoSess->DeleteDestination(uiIPv4,nPort,bTCP,nCh,pClient);

        }
    }
    return nRet;
}

int CRtpSessionMgr::GetMulticastIPv4(unsigned int *uiIPv4,int *nPort,int *nTTL,int bAudio,int bIPv6)
{
    unsigned int CurrIPv4;
    int CurrPort;
    int CurrTTL;
	if (bIPv6)
	{
	}
	else
	{
		CurrIPv4 = m_dwIPv4Multicast[bAudio != 0][0];
		CurrPort = m_nMulticastPort[bAudio != 0];
		CurrTTL = m_nTTL[bAudio != 0];
		if(htonl(CurrIPv4) < 0xE0000000 || htonl(CurrIPv4) > 0xEFFFFFFF)
			return -1;
	}
    if(CurrTTL < 1)CurrTTL = 255;
    if(CurrTTL > 255)CurrTTL = 255;
    if (uiIPv4)
    {
		if (bIPv6)
		{
			memcpy(uiIPv4,m_dwIPv4Multicast[bAudio != 0],16);
		}
		else
		{
			*uiIPv4 = CurrIPv4;
		}
    }
    if (nPort)
    {
        *nPort = CurrPort;
    }
    if (nTTL)
    {
        *nTTL = CurrTTL;
    }
    return 0;
}
int CRtpSessionMgr::StartMulticastIPv4(unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio,int bIPv6)
{
    int nRet;
    int nIdx = bAudio != 0;
    //224.0.0.0到239.255.255.255
    if(nPort & 1)
        return -1;
	if (bIPv6)
	{
		//
		int iii = 0;
	}
	else if (htonl(uiIPv4[0]) < 0xE0000000 || htonl(uiIPv4[0]) > 0xEFFFFFFF)
    {
        return -1;
    }
    nPort = nPort &0xFFFFFFFE;
    if (((!bIPv6) && m_dwIPv4Multicast[nIdx][0] != uiIPv4[0]) || ((bIPv6 && 0 != memcmp(m_dwIPv4MulticastIPv6,uiIPv4,16))) || nPort != m_nMulticastPort[nIdx])
    {
        //更改地址
		if (bIPv6)
		{
			 DeleteDestination(m_dwIPv4MulticastIPv6[nIdx],m_nMulticastPort[nIdx],bAudio);
			 memcpy(m_dwIPv4MulticastIPv6[nIdx],uiIPv4,16) ;
			 m_nMulticastCntIPv6[nIdx] = 0;
		}
		else
		{
			  DeleteDestination(m_dwIPv4Multicast[nIdx],m_nMulticastPort[nIdx],bAudio);
			  memcpy(m_dwIPv4Multicast[nIdx],uiIPv4,16) ;
			  m_nMulticastCnt[nIdx] = 0;
		}


        m_nMulticastPort[nIdx] = nPort ;
    }
    //DeleteDestination(m_tIPv4Multicast[bAudio != 0],bAudio);
    //m_tIPv4Multicast[bAudio != 0].SetIP(ip);
    //m_tIPv4Multicast[bAudio != 0].SetPort(addr.GetPort());
    if (nTTL <= 0)
    {
        nTTL = 1;
    }
    if (nTTL > 255)
    {
        nTTL = 255;
    }
    m_nTTL[nIdx] = nTTL;
    if (nIdx == 0)
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetMulticastTTL(nTTL);
        }

    }
    else
    {
        if (m_pAudioSess != NULL)
        {
            m_pAudioSess->SetMulticastTTL(nTTL);
        }

    }
	if (bIPv6)
	{
		if (m_nMulticastCntIPv6[nIdx] <= 0)
		{
			nRet = AddDestination(uiIPv4,nPort,0,bAudio,bIPv6);
			if (nRet == 0 )
			{
				m_nMulticastCntIPv6[nIdx]++;

			}
		}
		else
		{
			m_nMulticastCntIPv6[nIdx]++;
		}
	}
	else
	{
		if (m_nMulticastCnt[nIdx] <= 0)
		{
			nRet = AddDestination(uiIPv4,nPort,0,bAudio,bIPv6);
			if (nRet == 0 )
			{
				m_nMulticastCnt[nIdx]++;

			}
		}
		else
		{
			m_nMulticastCnt[nIdx]++;
		}
	}

    return 0;
}

int CRtpSessionMgr::StartMulticastIPv4NoApply(unsigned int uiIPv4[4],int nPort,int nTTL,int bAudio,int bIPv6)
{
    int nRet;
    int nIdx = bAudio != 0;
    //224.0.0.0到239.255.255.255
    if(nPort & 1)
        return -1;
	if (bIPv6)
	{
	}
    else if (htonl(uiIPv4[0]) < 0xE0000000 || htonl(uiIPv4[0]) > 0xEFFFFFFF)
    {
        return -1;
    }
    nPort = nPort &0xFFFFFFFE;
	if (((!bIPv6) && m_dwIPv4Multicast[nIdx][0] != uiIPv4[0]) || ((bIPv6 && 0 != memcmp(m_dwIPv4MulticastIPv6,uiIPv4,16))) || nPort != m_nMulticastPort[nIdx])
	{
		//更改地址
		if (bIPv6)
		{
			DeleteDestination(m_dwIPv4MulticastIPv6[nIdx],m_nMulticastPort[nIdx],bAudio);
			memcpy(m_dwIPv4MulticastIPv6[nIdx],uiIPv4,16) ;
			m_nMulticastCntIPv6[nIdx] = 0;
		}
		else
		{
			DeleteDestination(m_dwIPv4Multicast[nIdx],m_nMulticastPort[nIdx],bAudio);
			m_dwIPv4Multicast[nIdx][0]=uiIPv4[0] ;
			m_nMulticastCnt[nIdx] = 0;
		}



		m_nMulticastPort[nIdx] = nPort ;
	}

    //DeleteDestination(m_tIPv4Multicast[bAudio != 0],bAudio);
    //m_tIPv4Multicast[bAudio != 0].SetIP(ip);
    //m_tIPv4Multicast[bAudio != 0].SetPort(addr.GetPort());
    if (nTTL <= 0)
    {
        nTTL = 1;
    }
    if (nTTL > 255)
    {
        nTTL = 255;
    }
    m_nTTL[nIdx] = nTTL;
    if (nIdx == 0)
    {
        if (m_pVideoSess != NULL)
        {
            m_pVideoSess->SetMulticastTTL(nTTL);
        }

    }
    else if(m_pAudioSess != NULL)
    {
        m_pAudioSess->SetMulticastTTL(nTTL);
    }


#if 0
    nRet = AddDestination(uiIPv4,nPort,0,bAudio);
    if (nRet == 0 )
    {
        m_nMulticastCnt[nIdx]++;

    }
#endif
    return 0;
}
int CRtpSessionMgr::StopMulticastIPv4(int bAll,int bAudio,int bIPv6)
{

    int nIdx = bAudio != 0;
    m_nMulticastCnt[nIdx]--;

    if (bAll || m_nMulticastCnt[nIdx] <= 0)
    {

		if (bIPv6)
		{
			DeleteDestination(m_dwIPv4MulticastIPv6[nIdx],m_nMulticastPort[nIdx],bAudio);
		}
		else
		{
			DeleteDestination(m_dwIPv4Multicast[nIdx],m_nMulticastPort[nIdx],bAudio);
		}

        m_nMulticastCnt[nIdx] = 0;
        return 0;
    }

    return 0;
}
void CRtpSessionMgr::Poll()
{
    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->PollRTPPacket();
    }
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->PollRTPPacket();
    }
}
void CRtpSessionMgr::SetTime(unsigned int uSec,unsigned int uMSec)
{
    if (m_pVideoSess != NULL)
    {
        m_pVideoSess->SetTime(uSec,uMSec);
    }
    if (m_pAudioSess != NULL)
    {
        m_pAudioSess->SetTime(uSec,uMSec);
    }
}
CRTSPOverHttpSessionCookie::CRTSPOverHttpSessionCookie(CClientSocket *pinput,CClientSocket *poutput,char *pCookie)
{
    int len;

    m_pXsessioncookie = NULL;
    m_pGet = NULL;
    m_pPost = NULL;
    m_pNext = NULL;
    if (pCookie == NULL)
    {
        return;
    }
    len = strlen(pCookie);
    m_pXsessioncookie = new char[len + 1];
    if (m_pXsessioncookie != NULL)
    {
        m_pGet = pinput;
        m_pPost = poutput;
        strcpy(m_pXsessioncookie,pCookie);
    }

}
CRTSPOverHttpSessionCookie::~CRTSPOverHttpSessionCookie()
{
    if (m_pXsessioncookie != NULL)
    {
        delete m_pXsessioncookie;
        m_pXsessioncookie = NULL;
    }
    m_pGet = NULL;
    m_pPost = NULL;
    m_pNext = NULL;
}

void CRTSPOverHttpSessionCookie::AddInputClient(CClientSocket *pPost)
{
    m_pPost = pPost;
}

void CRTSPOverHttpSessionCookie::AddOutputClient(CClientSocket *pGet)
{
    m_pGet = pGet;
}






