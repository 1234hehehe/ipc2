#ifndef _RTSP_MSG_H_
#define _RTSP_MSG_H_

#define MSG_MAX_LINENUMS   (30)
#define MSG_MAX_SEGNUMS    (20)
class CMessage
{
public:
    CMessage();
    ~CMessage();
    int CopyMessage(unsigned char *pbuff,int nSize);
    int CopyMessage(CMessage &msg);
    int IsCommand(char *pCmd);
    int StrCmp(char *pStr,int nLine,int nSeg,int nCmpLen = -1);
    char *StrStr(char *pStr,int nLine,int nSeg);
    char *GetStr(int nLine,int nSeg);
    char *GetItem(char *pItem);
    void Reset();
    int IsDollar();
    int IsCheck();    
    int IncSeg(unsigned char *pSeg,int Pos,int len);
    int IncLine();
    int IsExtraData();
    int GetSeqNum();
    int GetResNum();


    int m_nPos[MSG_MAX_LINENUMS][MSG_MAX_SEGNUMS];
    int m_nLen[MSG_MAX_LINENUMS][MSG_MAX_SEGNUMS];
    int m_nLineCnt;
    int m_nSegCnt[MSG_MAX_LINENUMS];
    char *m_pMessage;
    int m_nMsgLen;
    int m_bDollar;
    int m_bAllocFlag;
    int m_nDollar_ch;
    int m_bCheck;
    

    unsigned char *m_pContext;
    int m_bHasContext;
    int m_nContextLength;
    int m_nContextPos;

    CMessage *m_pNext;

};

#endif