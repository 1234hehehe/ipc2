#include "rtsp_common.h"
#include "rtsp_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
CMessage::CMessage()
{
    m_nLineCnt = 0;
    memset(m_nSegCnt,0,sizeof(int) * MSG_MAX_LINENUMS);
    memset(m_nPos,0,sizeof(int) * MSG_MAX_LINENUMS*MSG_MAX_SEGNUMS);
    memset(m_nLen,0,sizeof(int) * MSG_MAX_LINENUMS*MSG_MAX_SEGNUMS);

    m_pNext = NULL;
    m_pMessage = NULL;
    m_nMsgLen = 0;
    m_bDollar = 0;
    m_bAllocFlag = 0;
    m_bHasContext = 0;
    m_nContextLength = -1;
    m_pContext = NULL;
    m_nDollar_ch = 0;
	m_bCheck = 0;
}
CMessage::~CMessage()
{
    m_nLineCnt = 0;
    memset(m_nSegCnt,0,sizeof(int) * MSG_MAX_LINENUMS);
    if (m_pMessage != NULL && m_bAllocFlag)
    {
        delete []m_pMessage;
        m_pMessage = NULL;
    }

    m_pMessage = NULL;
    m_pNext = NULL;
    m_nMsgLen = 0;
    m_bAllocFlag = 0;
    m_bHasContext = 0;
    m_nContextLength = -1;

    m_pContext = NULL;
}
int CMessage::CopyMessage(unsigned char *pbuff,int nSize)
{
    int i;
    char c;
    if (m_pMessage != NULL && m_bAllocFlag)
    {
        delete []m_pMessage;
    }
    m_pMessage = NULL;
    m_bAllocFlag = 0;
    m_pMessage = new char[nSize];
    if (m_pMessage == NULL)
    {
        return -1;
    }
    m_bAllocFlag = 1;
    for (i = 0; i < nSize; i ++)
    {
        c = pbuff[i];
        if (c == ' ' || c == CR || c == LF)
        {
            c = 0;
        }
        m_pMessage[i] = c;
    }
    return 0;

}
int CMessage::CopyMessage(CMessage &msg)
{
    char c;
    int i;
    int bq = 0;

    m_pMessage = msg.m_pMessage;
    m_nMsgLen = msg.m_nMsgLen;
    for (i = 0; i < m_nMsgLen; i ++)
    {
        c = m_pMessage[i];
        if (c == '\"')
        {
            bq = !bq;
        }
        if (bq)
        {
        }
        else if (c == ' ' || c == CR || c == LF/* || c == ';'*/)
        {
            c = 0;
            m_pMessage[i] = c;
        }

    }

    memcpy(m_nSegCnt,msg.m_nSegCnt,sizeof(int) * MSG_MAX_LINENUMS);
    m_nLineCnt = msg.m_nLineCnt;
    memcpy(m_nPos,msg.m_nPos,sizeof(int) * MSG_MAX_LINENUMS*MSG_MAX_SEGNUMS);
    memcpy(m_nLen,msg.m_nLen,sizeof(int) * MSG_MAX_LINENUMS*MSG_MAX_SEGNUMS);

    m_pContext = msg.m_pContext;
    m_nContextLength = msg.m_nContextLength;
    m_bHasContext = msg.m_bHasContext;
    m_nContextPos = msg.m_nContextPos;
    return 0;

}
int CMessage::IsCommand(char *pCmd)
{
    int nLen;
    if (pCmd == NULL)
    {
        return 0;
    }
    if (m_pMessage == NULL)
    {
        return 0;
    }
     if(stricmp(pCmd,"TEARDOWN") == 0){
        //printf("pcmd:%s  common:%s\n",pCmd,&m_pMessage[m_nPos[0][0]]);

         if((!strnicmp(pCmd,&m_pMessage[m_nPos[0][0]],m_nLen[0][0])) || strstr(&m_pMessage[m_nPos[0][0]],"DOWN") != NULL){
            return 1;
         }
    }
    if (m_bDollar)
    {
        return 0;
    }
    nLen = strlen(pCmd);
    if(nLen != m_nLen[0][0])
    {
        return 0;
    }
    return (!strnicmp(pCmd,&m_pMessage[m_nPos[0][0]],m_nLen[0][0]));
}
int CMessage::StrCmp(char *pStr,int nLine,int nSeg,int nCmpLen)
{
    int nLen;
    if (m_pMessage == NULL || pStr==NULL ||nLine >= m_nLineCnt || nSeg >= m_nSegCnt[nLine])
    {
        return -1;
    }

    nLen = strlen(pStr);
    if (nCmpLen > 0)
    {
        if (nLen > nCmpLen)
        {
            nLen = nCmpLen;
        }
    }
    else if(nLen != m_nLen[nLine][nSeg])
    {
        return -1;
    }

    return strnicmp(pStr,m_pMessage+m_nPos[nLine][nSeg],nLen);
}
char *CMessage::StrStr(char *pStr,int nLine,int nSeg)
{
    char *p;
    int testlen;
    if (m_pMessage == NULL || pStr == NULL || nLine >= m_nLineCnt || nSeg >= m_nSegCnt[nLine])
    {
        return NULL;
    }
    testlen = strlen(pStr);
    p = (char *)strstri(m_pMessage+m_nPos[nLine][nSeg],pStr);
    if (p != NULL)
    {
        if((p + testlen) > (m_pMessage+m_nPos[nLine][nSeg]+m_nLen[nLine][nSeg]) )
        {
            return NULL;
        }
    }
    return p;
}
char *CMessage::GetItem(char *pItem)
{
     char *sValue = NULL;
    int nLine;
    if (pItem == NULL)
    {
        return sValue;
    }
    for (nLine = 0; nLine < m_nLineCnt;nLine++)
    {
        if(0 == StrCmp(pItem,nLine,0))
        {
            sValue = strdup(GetStr(nLine,1));
            return sValue;
        }
    }
    return sValue;
}
char *CMessage::GetStr(int nLine,int nSeg)
{
    if (m_pMessage == NULL || nLine >= m_nLineCnt || nSeg >= m_nSegCnt[nLine])
    {
        return NULL;
    }
    return m_pMessage + m_nPos[nLine][nSeg];

}
void CMessage::Reset()
{
    m_nLineCnt = 0;
    memset(m_nPos,0,sizeof(int) * MSG_MAX_LINENUMS*MSG_MAX_SEGNUMS);
    memset(m_nLen,0,sizeof(int) * MSG_MAX_LINENUMS*MSG_MAX_SEGNUMS);
    memset(m_nSegCnt,0,sizeof(int) * MSG_MAX_LINENUMS);
    m_pNext = NULL;
    if (m_pMessage != NULL && m_bAllocFlag)
    {
        delete m_pMessage;
    }
    m_bAllocFlag = 0;
    m_pMessage = NULL;
    m_nMsgLen = 0;
    m_bDollar = 0;
    m_bHasContext = 0;
    m_nContextLength = -1;
    m_nDollar_ch = 0;

    m_pContext = NULL;
}
int CMessage::IsDollar(){return m_bDollar;}
int CMessage::IsCheck(){return m_bCheck;}
int CMessage::IncSeg(unsigned char *pSeg,int Pos,int len)
{
    if (m_nLineCnt >= MSG_MAX_LINENUMS)
    {
        return -1;
    }
    if (m_nSegCnt[m_nLineCnt] >= MSG_MAX_SEGNUMS)
    {
        return -1;
    }
    m_nPos[m_nLineCnt][m_nSegCnt[m_nLineCnt]] = Pos;
    m_nLen[m_nLineCnt][m_nSegCnt[m_nLineCnt]] = len;
    if (len > 0)
    {

        //ÅÐ¶ÏÊÇ·ñÓÐÄÚÈÝ
        if (!strnicmp((char *)pSeg,"Content-Length:",len))
        {//
            m_bHasContext = 1;
        }

        m_nSegCnt[m_nLineCnt]++;
    }
    return 0;
}
int CMessage::IncLine()
{
    if (m_nLineCnt >= MSG_MAX_LINENUMS)
    {
        return -1;
    }
    if (m_nSegCnt[m_nLineCnt] > 0)
    {
        m_nLineCnt++;
    }
    return 0;

}
int CMessage::IsExtraData(){return m_bHasContext;}

int CMessage::GetSeqNum()
{
    int nLine,nSeq = -1;
    char *pstr;
    for (nLine = 1; nLine < m_nLineCnt; nLine++)
    {
        if (!StrCmp("CSeq:",nLine,0))
        {//
            pstr = GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);
            }



        }
    }
  return nSeq;

}
int CMessage::GetResNum()
{
    int nLine = 0,nSeq = -1;
    char *pstr;
    if (IsDollar())
    {
        return -1;
    }

        if (!StrCmp("RTSP/1.0",nLine,0))
        {//
            pstr = GetStr(nLine,1);
            if (pstr != NULL)
            {
                nSeq = atoi(pstr);
            }



        }

    return nSeq;

}

