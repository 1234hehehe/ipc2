
#include "ovfs_arp_rarp_service.h"
#include "ovfs_arp_tool_resource.h"
#include "ovfs_cfg_manage_api.h"
#include "ovfs_network_utility_api.h"

using namespace ovfs_arptool;
using namespace ovfs_soft;




static unsigned char g_byBroadcastMac[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static S8 g_eth_table[][OVFS_MAX_ETH_IFNAME_SIZE] = 
{
	{"eth0"},
	{"eth1"},
	{"bond0"},	
	{"wlan0"},
};


static U32 min_u32(U32 para1, U32 para2)
{
	return (para1 < para2) ? para1 : para2;
}


// 压缩
// i个连续相同的数a, 最大为127个
// j个连续不同的数b,最大为127个，最高为1
// i a j b
static char* arp_compress_data(char *pOrgData,int nOrgDataSize,int *pNewDataSize)
{
    char *pNewData=NULL;
    int nPos = 0,nPos1 = 0;
    int i,nSameNum,nDiffNum,bSame = -1,bFirst = 0;
    char cLastChar,cCurrChar;
    int *pTotalSize = NULL,*pOrgTotalSize =NULL;


    pNewData = (char *)Common_Malloc(16 + nOrgDataSize* 2,0,__FUNCTION__,__LINE__);
    if(pNewData == NULL)
    {
        return NULL;
    }
    pNewData[nPos] = 'Z';
    pNewData[nPos + 1] = 'I';
    pNewData[nPos + 2] = 'P';
    pNewData[nPos + 3] = '0';
    nPos += 4;
    pTotalSize = (int *)(void *)&pNewData[nPos];
    nPos += 4;
    pOrgTotalSize = (int *)(void *)&pNewData[nPos];
    nPos += 4;
    pOrgTotalSize[0] = nOrgDataSize;

    cLastChar = pOrgData[0];
    bSame = -1;
    nSameNum =0;
    nDiffNum = 0;
    nPos1 = nPos;
    nPos++;
   
    bFirst = 1;
    bSame = 0;
    for(i = 1; i < nOrgDataSize;i++)
    {
        cCurrChar =  pOrgData[i];
        if(cLastChar == cCurrChar)
        {
            if (bFirst)
            {
                bFirst = 0;
                pNewData[nPos] = cLastChar;
                nPos++;
                nSameNum = 2;
                bSame = 1;
                nDiffNum = 0;
            }
            else
            {
                if (bSame)
                {
                    if (nSameNum >= 127)
                    {// 换新计数
                        pNewData[nPos1] = nSameNum;
                        nPos1 = nPos;
                        nPos++;
                        nSameNum = 0;
                        bFirst = 1;
                    }
                    else
                    {
                     nSameNum++;
                    }
                    
                }
                else
                {// ab >> b
                    bSame = 1;
                    pNewData[nPos1] = nDiffNum | (1 << 7);
                    nPos1 = nPos;
                    nPos++;
                    pNewData[nPos] = cLastChar;
                    nPos++;
                    nSameNum = 2;
                }
            }
        }
        else
        {
            if (bFirst)
            {// ab
                bFirst = 0;
                nDiffNum = 1;
                pNewData[nPos] = cLastChar;
                nPos++;
                cLastChar = cCurrChar;
                bSame = 0;

            }
            else
            {
                if (bSame)
                {// aab
                    bSame = 0;
                    pNewData[nPos1] = nSameNum;
                    nPos1 = nPos;
                    nPos++;
                    cLastChar = cCurrChar;
                    
                    bFirst = 1;
                }
                else
                {// abc
                    bSame = 0;
                    nDiffNum++;
                    pNewData[nPos] = cLastChar;
                    nPos++;
                    cLastChar = cCurrChar;
                    if (nDiffNum >= 126)
                    {// 换新计数
                         pNewData[nPos1] = nDiffNum | (1 << 7);
                         nPos1 = nPos;
                         nPos++;
                         nDiffNum = 1;
                         bFirst = 1;
                    }
                   
                }
            }
        }
       
    }
    if(bSame)
    {
        pNewData[nPos1] = nSameNum;
    }
    else
    {
        nDiffNum++;
        pNewData[nPos1] = nDiffNum | (1 << 7);
        pNewData[nPos] = cCurrChar;
        nPos++;
    }
    *pTotalSize = nPos-12;
    pNewData[nPos] = 'Z';
    pNewData[nPos + 1] = 'I';
    pNewData[nPos + 2] = 'P';
    pNewData[nPos + 3] = 'E';
    nPos += 4;
    if(pNewDataSize)
    {
        *pNewDataSize = nPos;
    }
    return pNewData;
}

static char* arp_decompress_data(char *pOrgData,int nOrgDataSize,int *pNewDataSize)
{
      int nTotalSize = 0,nNewTotalSize = 0;
      int nNewPos = 0,bError = 0;
      char *pNewData = NULL;
	  char cStrLen;
      if (pOrgData == NULL || nOrgDataSize <= 8)
      {
          return NULL;
      }
      if (pOrgData[0] != 'Z' && 
          pOrgData[1] != 'I' &&
          pOrgData[2] != 'P' &&
          pOrgData[3] != '0')
      {
          return NULL;
      }
      nTotalSize = ((int *)(void *)(&pOrgData[4]))[0];
      if (nTotalSize > nOrgDataSize - 12)
      {
          return NULL;
      }
      nNewTotalSize = ((int *)(void *)(&pOrgData[8]))[0];
      
      if(pOrgData[12 + nTotalSize] != 'Z' &&
         pOrgData[12 + nTotalSize + 1] != 'I' &&
         pOrgData[12 + nTotalSize + 2] != 'P' &&
         pOrgData[12 + nTotalSize + 3] != 'E')
      {
          return NULL;
      }
	  
      pNewData = (char *)Common_Malloc(nNewTotalSize,0,__FUNCTION__,__LINE__);
      if(pNewData == NULL)
      {
          return NULL;
      }
      for (int i = 0,nPos = 12; i < nTotalSize;i++)
      {
          cStrLen = pOrgData[nPos];
          nPos++;
          if (cStrLen & (1 << 7))
          {// 不同
              cStrLen = cStrLen & 0x7F;
              if (nNewPos+cStrLen>nNewTotalSize)
              {
                  bError = 1;
                  break;
              }
              memcpy(&pNewData[nNewPos],&pOrgData[nPos],cStrLen);
              nPos += cStrLen;
              nNewPos += cStrLen;
              
          }
          else
          {
              if (nNewPos+cStrLen>nNewTotalSize)
              {
                  bError = 1;
                  break;
              }
              memset(&pNewData[nNewPos],pOrgData[nPos],cStrLen);
              nPos++;
              nNewPos += cStrLen;
          }
          if (nPos >= nTotalSize + 12)
          {
              break;
          }
         

      }
      if (nNewPos != nNewTotalSize || bError)
      {
          Common_Free(pNewData,__FUNCTION__,__LINE__);
		  pNewData = NULL;
          printf("Err\n");
          return NULL;
      }
      if (pNewDataSize)
      {
          *pNewDataSize = nNewTotalSize;
      }
      return pNewData;
}



static S32 arp_rarp_recv_msg_thread_fxn(Common_Thread_T hThreadHandle,void *pParam)
{
	ovfs_arptool_arp *pParent = (ovfs_arptool_arp *)pParam;
	if(pParent != NULL)
	{
		pParent->on_message();
	}
	
	return 0;
}


ovfs_arptool_arp::ovfs_arptool_arp( unsigned int max_channel_num):
	m_max_channel_num(max_channel_num)
{
	
	OVFS_CLR_ARG(m_arp_socket);
	OVFS_CLR_ARG(m_rarp_socket);
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
	{
		m_arp_socket[i].socket = -1;
	}

	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_rarp_socket); ++i)
	{
		m_rarp_socket[i].socket = -1;
	}
	
	OVFS_CLR_ARG(m_local_mac);
	OVFS_CLR_ARG(m_local_ipv4);
	
	m_local_eth_cnt = 0;

	OVFS_CLR_ARG(m_socket_lock);
	OVFS_CLR_ARG(m_serch_dev_lock);
	Common_Lock_Create(&m_socket_lock, NULL);
	Common_Lock_Create(&m_serch_dev_lock, NULL);
	
	m_local_chanInfo = (ovfs_arp_channel_device *)Common_Malloc(sizeof(ovfs_arp_channel_device) * m_max_channel_num,0,__FUNCTION__,__LINE__);
	OVFS_ASSERT(NULL != m_local_chanInfo);
	memset(m_local_chanInfo, 0, sizeof(ovfs_arp_channel_device) * m_max_channel_num);
	
	OVFS_CLR_ARG(m_send_pkt);
	m_send_pkt_num = 0;
	m_send_refresh_cnt = 0;
	m_send_refreshed_cnt = 0;
	m_send_cnt = 0; 
	
	m_exit_thread = 0;
	m_thread = NULL; 
	
	m_local_device_type = 2;
	m_use_mgr = NULL;
	m_dev_use_mgr_count = 0;
	m_interval_time_by_second = 0;
	m_last_search_time_by_second = 0;
	m_start_search = 0;

	OVFS_CLR_ARG(m_net_out);
	OVFS_CLR_ARG(m_ip_conflict);
	OVFS_CLR_ARG(m_ip_conflict_cnt);
	OVFS_CLR_ARG(m_local_ipv4_num);
	
	m_run_time_s = 0;
	m_run_time_ms = 0;
	m_run_time_tast_second = 0;
	OVFS_CLR_ARG(m_conflict_require);
	OVFS_CLR_ARG(m_conflict_mac);
	m_dev_mgr = NULL;
	m_dev_mgr_tail = NULL;
	m_dev_search_count = 0;
}

ovfs_arptool_arp::~ovfs_arptool_arp()
{
	stop_arp_rarp_server();
	stop_arp_search();
	Common_Free(m_local_chanInfo,__FUNCTION__,__LINE__);
	m_local_chanInfo = NULL;
	if(m_serch_dev_lock != NULL)
	{
		Common_Lock_Destroy(&m_serch_dev_lock);
		m_serch_dev_lock = NULL;
	}
	if(m_socket_lock!= NULL)
	{
		Common_Lock_Destroy(&m_socket_lock);
		m_socket_lock = NULL;
	}
}

int ovfs_arptool_arp::on_message()
{
	int selectResult,nMaxSocket = 0;
	
	char *buffer = NULL;
	
	
	struct timeval tv_timeToDelay;
	fd_set readSet,execptSet;
	int nTimeCnt = 0;
	int nSearchTimeCnt = 0;
	int	nSendHelloCnt = 0;
	struct timespec tsp;
	int need_open_socket = 1;
	struct timeval last_open_time;
	
	
	while(1)
	{
		if(m_exit_thread)
		{
			break;
		}
		Common_Lock(m_socket_lock);
		nMaxSocket = 0;


		struct timeval cur_time;
		gettimeofday(&cur_time, NULL);
		if(need_open_socket || Common_cnt_interval_ms(&cur_time, &last_open_time) > 10000)
		{
			int open_ret = open_all_support_socket();
			if(open_ret >= 0)
			{
				need_open_socket = 0;
			}
			last_open_time = cur_time;
				 // 更新
			if(m_local_eth_cnt == 0)
			{//一个网卡都还没起来，只能等一下再试一试
				Common_UnLock(m_socket_lock);
				Common_Sleep(1, 0);
				continue;
			}
			if(open_ret > 0)
			{
				Common_Lock(m_serch_dev_lock);
				update_local_usr_mac(m_local_mac[0], m_local_ipv4[0][0]);
				Common_UnLock(m_serch_dev_lock);
			}
		}
		fd_set_all(&readSet, &execptSet, &nMaxSocket);
		if(nMaxSocket == 0)
		{
			Common_UnLock(m_socket_lock);
			Common_Sleep(1, 0);
			continue;
		}
	 
		if(0 == clock_gettime(CLOCK_MONOTONIC,&tsp))
		{
			m_run_time_s = tsp.tv_sec;
		}

		if(m_run_time_s != m_run_time_tast_second)
		{

			m_run_time_tast_second = m_run_time_s;
			send_all_ip_conflict_pkt();

			nTimeCnt++;
			if(nTimeCnt > 3)
			{
				nTimeCnt = 0;
				//printf("check ....\n");
				// 检查网线断	
				for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); i++)
				{
					//printf("[%d] %d\n",i,m_arp_socket[i].socket);
					if(m_arp_socket[i].socket < 0)
					{
						continue ;
					}
					check_mac_and_ip_change(m_arp_socket[i].eth_name, m_arp_socket[i].socket, m_arp_socket[i].phy_idx, m_arp_socket[i].fr_ifindex);
					check_net_out_update(m_arp_socket[i].eth_name, m_arp_socket[i].socket, m_arp_socket[i].phy_idx, m_arp_socket[i].fr_ifindex);
					check_ip_conflict_update(m_arp_socket[i].eth_name, m_arp_socket[i].socket, m_arp_socket[i].phy_idx, m_arp_socket[i].fr_ifindex, buffer, OVFS_ARPTOOL_ARP_MAX_BUFFSIZE + 16);
					
				}
			}

			if((m_local_device_type == 2 || 
			   m_local_device_type == 4 ||
			   m_local_device_type == 5 ||
			   m_local_device_type == 6 )&& 
			   (unsigned int)(m_last_search_time_by_second + m_interval_time_by_second) <= m_run_time_s)
			{
			// 每隔一段时间搜索一次。每次发几个包。
				nSearchTimeCnt++;
				if(nSearchTimeCnt >= 3)
				{
					nSearchTimeCnt = 0;
					m_last_search_time_by_second = m_run_time_s;
				}
				for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
				{
					if(m_arp_socket[i].socket < 0)
					{
						continue;
					}
					//这里应该发的是搜索请求包
					Common_Lock(m_serch_dev_lock);
					send_dev_search_pkt(m_arp_socket[i].socket, m_arp_socket[i].phy_idx, m_arp_socket[i].fr_ifindex, buffer, OVFS_ARPTOOL_ARP_MAX_BUFFSIZE + 16);
					Common_UnLock(m_serch_dev_lock);
				}
					
			}
		// 如果通道参数更新了就需要广播自己的包

			for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
			{
				if(m_arp_socket[i].socket < 0)
				{
					continue;
				}
				Common_Lock(m_serch_dev_lock);
				if(m_send_pkt_num > 0 && m_send_refresh_cnt!= m_send_refreshed_cnt)
				{
					nSendHelloCnt++;
					if(nSendHelloCnt >= 3)
					{
						nSendHelloCnt = 0;
						m_send_refreshed_cnt = m_send_refresh_cnt;
					}
					broadcast_self_dev_pkt(m_arp_socket[i].socket, m_arp_socket[i].phy_idx, m_arp_socket[i].fr_ifindex);
				}
				update_dev_mgr();
				Common_UnLock(m_serch_dev_lock);
			}
		}

		//printf("socket [%d,%d,%d,%d]\n",m_nSocket[0],m_nSocket[1],m_nSocket[2],m_nSocket[3]);
		tv_timeToDelay.tv_sec = 1;
		tv_timeToDelay.tv_usec = 0;  
		
		Common_UnLock(m_socket_lock);
		selectResult = select(nMaxSocket + 1, &readSet, NULL, NULL/*&execptSet*/, &tv_timeToDelay);
		//printf("selectResult = %d!!!!!\n", selectResult);
		if (selectResult < 0)
		{
			// 出错
			// printf("[%s.%d]nPhyIdx = %d SIOCGIFINDEX error = %d [%s]\n",__FUNCTION__,__LINE__,0,errno,strerror(errno));

			usleep(1000*1000);
			continue;;
		}
		else if(selectResult == 0)
		{

			// 超时
			continue;
		}

		// printf("tv_usec = %d-%d\n",tv_timeToDelay.tv_sec,tv_timeToDelay.tv_usec);
		if(buffer == NULL)
		{
			buffer = (char* )Common_Malloc(OVFS_ARPTOOL_ARP_MAX_BUFFSIZE + 16,0,__FUNCTION__,__LINE__);
			if(buffer == NULL)
			{
				break;
			}
		}
		
		Common_Lock(m_socket_lock);
		Common_Lock(m_serch_dev_lock);
		for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
		{
			if(m_arp_socket[i].socket > 0)
			{
				if(!FD_ISSET(m_arp_socket[i].socket,&readSet))
				{
					//LOGW("FD_ISSET readSet continue\n ");
					continue;
				}
				//LOGD("m_arp_socket[%d].socket = %d!!!!!\n", i, m_arp_socket[i].socket);
				int ret = recv_data(m_arp_socket[i].socket, m_arp_socket[i].phy_idx,  m_arp_socket[i].fr_ifindex, buffer, OVFS_ARPTOOL_ARP_MAX_BUFFSIZE + 16);
				if(ret < 0)
				{
					close(m_arp_socket[i].socket);
					m_arp_socket[i].socket = -1;
					need_open_socket = 1;
					Common_Sleep(1, 0);
				}
			}
		}
		//usleep(10000);
		Common_UnLock(m_serch_dev_lock);
		Common_UnLock(m_socket_lock);
	}
	
	if(buffer != NULL)
	{
		Common_Free(buffer,__FUNCTION__,__LINE__);
		buffer = NULL;
	}
	return 0;
}


int ovfs_arptool_arp::start_arp_rarp_server()
{
	Common_Lock(m_socket_lock);
	if(m_thread != NULL)
	{
		Common_UnLock(m_socket_lock);
		return 0;
	}
	
	open_all_support_socket();
	m_exit_thread = 0;
	
	if(Common_Thread_Create(&m_thread,"arp_rarp_recv_msg_thread_fxn",1024*128,
		                    COMMON_THREAD_CREATEFLAG_NORMAL,arp_rarp_recv_msg_thread_fxn,(void *)this))
	{
		m_thread = NULL;
		Common_UnLock(m_socket_lock);
		return -1;
	}	
	Common_UnLock(m_socket_lock);
	
	return 0;
}

int ovfs_arptool_arp::stop_arp_rarp_server()
{
	m_exit_thread = 1;
	Common_Thread_Destroy(&m_thread);
	
	Common_Lock(m_socket_lock);
	if(m_thread == NULL)
	{
		Common_UnLock(m_socket_lock);
		return 0;
	}
	m_thread = NULL;
	close_all_socket();
	
	Common_Lock(m_serch_dev_lock);
	delete_all_dev();
	for(int i = 0; i < m_send_pkt_num; ++i)
	{
		if(m_send_pkt[i] == NULL)
		{
			continue;
		}
		Common_Free(m_send_pkt[i],__FUNCTION__,__LINE__);
		m_send_pkt[i] = NULL;
	}
	Common_UnLock(m_serch_dev_lock);

	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_conflict_require); ++i)
	{
		if(m_conflict_require[i] == NULL)
		{
			continue;
		}
		m_conflict_require[i]->nStatus = 3;
		sem_post(&m_conflict_require[i]->hSem_Res);
	}
	
	Common_UnLock(m_socket_lock);
	return 0;
}

int ovfs_arptool_arp::set_callback(unsigned int dwModule,OVFS_ARPTOOL_ARP_CALLBACK fxn,void *pUser)
{
	return -1;
}

int ovfs_arptool_arp::send_packet(unsigned int dwOP,unsigned char *bySrcMac/*[6]*/,char *pSrcIP,unsigned char *byDstMac/*[6]*/,char *pDstIP)
{
	return -1;
}

int ovfs_arptool_arp::send_packet(unsigned int dwOP,unsigned char *bySrcMac/*[6]*/,unsigned int dwSrcIP,unsigned char *byDstMac/*[6]*/,unsigned int dwDstIP)
{
	return -1;
}

int ovfs_arptool_arp::require(int bRARP,unsigned char *bySrcMac/*[6]*/,char *pSrcIP,unsigned char *byDstMac/*[6]*/,char *pDstIP,unsigned int dwTimeoutMs)
{
	return -1;
}


int ovfs_arptool_arp::set_arp_search_local_data(int device_type, ovfs_arp_channel_device *chan_info, int chan_count)
{
	if(chan_count > (int)m_max_channel_num)
	{
		return -1;
	}
	
	Common_Lock(m_serch_dev_lock);
	m_local_device_type = device_type;
	
	if(m_local_chanInfo != NULL &&
	   m_local_chan_count == chan_count&&
	   chan_info != NULL)
	{//如果和现有的通道连接一样，就直接返回
		if(0 == memcmp(m_local_chanInfo, chan_info, chan_count * sizeof(ovfs_arp_channel_device)))
		{
			Common_UnLock(m_serch_dev_lock);
			return 0;
		}
	}
	
	// 删除自己上次有的IPC

	// 删除使用列表
	// 也需要从使用列表里删除
	ovfs_arp_channel_device *pDeviceInfo = NULL;
		
	pDeviceInfo = m_local_chanInfo;
	for(unsigned int nDelIdx = 0; nDelIdx < m_max_channel_num && pDeviceInfo != NULL;nDelIdx++)
	{
	    OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs;
		if(pDeviceInfo[nDelIdx].device.szDomain[0] == 0)
		{
			continue;
		}
		//先从ipc链表中找到对应的ipc
		pIPCs = proc_find_ipc_from_usr_mgr(&pDeviceInfo[nDelIdx].device);
		if(pIPCs == NULL)
		{
			continue;
		}
		//然后将ipc的本地使用者节点删除
		delete_ipc_self_usr_node(pIPCs);
		OVFS_ASSERT(pIPCs->nUseCount >= 0); 
		if(pIPCs->nUseCount == 0)
		{
			OVFS_ASSERT(pIPCs->pWhoUse == NULL);
			delete_use_ipc_node(pIPCs);
		}
	}
	
	// 自己有哪些通道IPC
	pDeviceInfo = chan_info;
	for(int nDelIdx = 0; nDelIdx < chan_count&& pDeviceInfo != NULL;nDelIdx++)
   	{
		if(pDeviceInfo[nDelIdx].device.szDomain[0] == 0)
		{
			continue;
		}
		OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs = NULL;
		OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUseS = NULL;
		//先从ipc链表中找到对应的ipc
		pIPCs = proc_find_ipc_from_usr_mgr(&pDeviceInfo[nDelIdx].device);
		if(pIPCs != NULL)
		{
			
			//然后找到ipc的本地使用者节点
			pUseS = proc_find_self_from_ipc_user(pIPCs->pWhoUse);
		}

		if(pIPCs == NULL)
		{
			//如果没找到就创建新的IPC，并且加入到ipc链表
			pIPCs = malloc_ipc_node(&pDeviceInfo[nDelIdx].device);
			if(pIPCs != NULL)
			{
				add_ipc_node_2_use(pIPCs);
			}
		}

		if(pUseS == NULL)
		{
			if(pIPCs == NULL)
			{
				continue;
			}
			pUseS = malloc_usr_node(m_local_mac[0], m_local_ipv4[0][0], 1);
			if(pUseS != NULL)
			{//往ipc使用者中添加本地用户
				add_use_node_2_ipc(pIPCs, pUseS);
			}
			
		}
	}
	update_local_chan_info(chan_info, chan_count);
	update_local_send_pkt(chan_info, chan_count);
	Common_UnLock(m_serch_dev_lock);
	return 0;
}

int ovfs_arptool_arp::start_arp_search(int nIntervalTimeBySecond)
{
	if(nIntervalTimeBySecond < 10)
	{
		nIntervalTimeBySecond = 10;
	}
	m_interval_time_by_second = nIntervalTimeBySecond;
	m_start_search = 1;
	return 0;
}

int ovfs_arptool_arp::stop_arp_search()
{
	m_start_search = 0;
	Common_Lock(m_serch_dev_lock);
	delete_all_ipc();
	Common_UnLock(m_serch_dev_lock);
	return 0;
}

int ovfs_arptool_arp::check_IP_conflict(unsigned char *byMac_in/*[6]*/,char *pIP,unsigned char *byMac_out/*[6]*/,unsigned int dwTimeoutMs)
{
	int nRet = -1, nIdx_Req = -1;
	char *buffer = NULL;
	unsigned int dwIP;
	OVFS_ARPTOOL_ARP_REQUIRE_T tRequire;
	int bConflict = 0, bSendFail = 0;

	
	if(pIP == NULL)
	{
		printf("[%s.%d]here\n",__FUNCTION__,__LINE__);
		return 0;
	}
	
	dwIP = inet_addr(pIP);
	if((int)dwIP == -1 ||dwIP == 0)
	{
		printf("%s error ip\n", pIP);
		return 0;
	}

	
	unsigned char byMacTest[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},byMacTest0[6]={0,0,0,0,0,0};
	if(byMac_in == NULL)
	{
		printf("byMac_in is null\n");
		return 0;
	}
	
	if(memcmp(byMac_in,byMacTest,6) == 0||
	    memcmp(byMac_in,byMacTest0,6) == 0)
	{
		printf("[%s.%d]here\n",__FUNCTION__,__LINE__);
		return 0;
	}

	if(is_conflict_with_self(byMac_in, pIP, byMac_out))
	{
		return 1;
	}
	
	Common_Lock(m_socket_lock);
	if(m_thread == NULL)
	{
		Common_UnLock(m_socket_lock);
		return 0;
	}
	
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_conflict_require); i++)
	{
		if(m_conflict_require[i] == NULL)
		{
			nIdx_Req = i;
			memset(&tRequire,0,sizeof(tRequire));
			m_conflict_require[i] = &tRequire;
			break;
		}
	}
	
	if(nIdx_Req == -1)
	{
		printf("[%s.%d]here\n",__FUNCTION__,__LINE__);
		Common_UnLock(m_socket_lock);
		return 0;
	}
	
	// 检查IP冲突
	// 发送一个ARP包
	buffer = (char *)Common_Malloc(OVFS_ARPTOOL_ARP_PING_PKT_SIZE,0,__FUNCTION__,__LINE__);
	if(buffer == NULL)
	{
		if(m_conflict_require[nIdx_Req] == &tRequire)
		{
			m_conflict_require[nIdx_Req]  = NULL;
		}
		printf("[%s.%d]here\n",__FUNCTION__,__LINE__);
		Common_UnLock(m_socket_lock);
		return 0;
	}
	
	if(byMac_in != NULL)
	{
		memcpy(tRequire.byReqMac, byMac_in, 6);
		tRequire.bReqMacSet = 1;
	}
			
	tRequire.dwReqIP = dwIP;
	tRequire.nTimeoutCnt = (dwTimeoutMs < 1000?1 : (dwTimeoutMs/1000));
	sem_init(&tRequire.hSem_Res, 0,0);
	tRequire.nStatus = 1;
  	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket);i++)
  	{
  		if(m_arp_socket[i].socket < 0
			|| buffer == NULL)
  		{
			continue;
  		}
		unsigned int phy_idx = m_arp_socket[i].phy_idx;
		struct sockaddr_ll tDstEtheraddr;
		int nSendSize = 60;
		
	  	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
		OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);

		memset(buffer + 28 + 14,0,60 - 28 - 14);
		memcpy(pEther->bySrcMac,m_local_mac[phy_idx],6);
		memset(pEther->byDstMac,0xFF,6);
		
		pEther->wEtherType = htons(OVFS_ARPTOOL_ARP_FTYPE);
		pARPPacket->wProtoType = htons(0x0800);
		pARPPacket->wHardType = htons(0x0001);
		pARPPacket->byHrdLen = 6;
		pARPPacket->byProtoLen = 4;
		pARPPacket->wOP = htons(1);
		memcpy(pARPPacket->bySrcMac,m_local_mac[phy_idx],6);
		pARPPacket->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][0]);
		memset(pARPPacket->byDstMac,0,6);
		pARPPacket->dwDstIP = dwIP;	
		memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
		tDstEtheraddr.sll_ifindex=m_arp_socket[i].fr_ifindex; 
		tRequire.pArpPacket = buffer;
		tRequire.nArpPacketSize = nSendSize;
		nRet = sendto(m_arp_socket[i].socket, buffer, nSendSize, 0, (struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
		if(nRet != nSendSize)
		{
			printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
			bSendFail = 1;
		}	
	}
		
	if(bSendFail)
	{
		if(m_conflict_require[nIdx_Req] == &tRequire)
		{
			m_conflict_require[nIdx_Req]  = NULL;
		}
		
		sem_destroy(&tRequire.hSem_Res);
		if(buffer != NULL)
		{
			Common_Free(buffer,__FUNCTION__,__LINE__);
			buffer = NULL;
		}
		printf("[%s.%d]here\n",__FUNCTION__,__LINE__);
		Common_UnLock(m_socket_lock);
		return 0;
	}
	Common_UnLock(m_socket_lock);
	//设置一个信号量等待发送接收消息的线程接到ip冲突的检测消息或者超时后发送信号量过来，函数就可以继续往下走
	sem_wait(&tRequire.hSem_Res);
	Common_Lock(m_socket_lock);
	if(m_conflict_require[nIdx_Req] == &tRequire)
	{
		m_conflict_require[nIdx_Req]  = NULL;
	}
	
	if(tRequire.nStatus == 2)
	{
		bConflict = 1;
		if(byMac_out != NULL)
		{
			memcpy(byMac_out,tRequire.byConflictMac,6);
		}
	}
	sem_destroy(&tRequire.hSem_Res);
	Common_UnLock(m_socket_lock);
	
	if(buffer != NULL)
	{
		Common_Free(buffer,__FUNCTION__,__LINE__);
		buffer = NULL;
	}

	//printf("status = %d nTimeoutCnt=%d\n",tRequire.nStatus,tRequire.nTimeoutCnt);
	return bConflict;
}


int ovfs_arptool_arp::check_net_out(const char *if_name)
{//这些只是查看一下，不需要加锁
	for(unsigned int i = 0; i < m_local_eth_cnt; ++i)
	{
		if(strcmp(m_local_if_name[i], if_name) == 0)
		{
			return m_net_out[i] == 0?0:1;
		}
	}
	return 0;
}

int ovfs_arptool_arp::check_local_IP_conflict(const char *if_name,const char *ip  )
{//这些只是查看一下，不需要加锁
	for(unsigned int i = 0; i < m_local_eth_cnt; ++i)
	{
		if(strcmp(m_local_if_name[i], if_name) == 0)
		{
			for(U32 j = 0; j < m_local_ipv4_num[i] && j < OVFS_ARRAY_DIM(m_local_ipv4[i]); ++j)
			{
				if(ip != NULL)
				{
					if(strcmp(ip, m_local_ipv4[i][j]) == 0)
					{
						if(m_ip_conflict[i][j])
						{
							return 1;
						}
						else
						{
							return 0;
						}
					}
				}
				else
				{
					if(m_ip_conflict[i][j])
						return 1;
				}
			}
		}
	}
	return 0;
}



int ovfs_arptool_arp::get_ipc_connected_times(unsigned char *byMac, char* ip)
{
	unsigned char byzeroMac[6]={0,0,0,0,0,0};
	int dwIP = 0 ;
		
	if(byMac == NULL && ip == NULL)
	{
		return 0;
	}
	if(ip != NULL)
	{
		dwIP = inet_addr(ip);
	}
	
	Common_Lock(m_serch_dev_lock);
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIpc = m_use_mgr;
	int cnt = 0;
	while(pIpc != NULL)
	{
		if(byMac != NULL&&
			memcmp(byzeroMac,pIpc->byMac,6) != 0 &&
			memcmp(byzeroMac,byMac,6) != 0 )
		{
			if(memcmp(pIpc->byMac,byMac,6) == 0)
			{
				cnt += pIpc->nUseCount;
				
			}
		}
		else if(dwIP != 0 && dwIP != -1 && ip!=NULL && 0 == strcmp(pIpc->szIP, ip))
		{
			if(memcmp(byzeroMac,pIpc->byMac,6) == 0)
			{
				cnt += pIpc->nUseCount;
			}
			else if(byMac == NULL || memcmp(byzeroMac,byMac,6) == 0)
			{
				cnt += pIpc->nUseCount;
			}
			
		}
		pIpc = pIpc->pNext;
	}
	Common_UnLock(m_serch_dev_lock);
	return cnt;
}

void ovfs_arptool_arp::diagnose_dump(unsigned int para, ovfs_soft::ovfs_diag_dump dump_fuc)
{
	switch(para)
	{
		case 1:
			{
				dump_fuc("网卡信息:\n");
				for(unsigned int i = 0; i < m_local_eth_cnt; ++i)
				{
					dump_fuc("网卡:%s \nmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n"
						, m_local_if_name[i]
						, m_local_mac[i][0]
						, m_local_mac[i][1]
						, m_local_mac[i][2]
						, m_local_mac[i][3]
						, m_local_mac[i][4]
						, m_local_mac[i][5]
						);
					for(unsigned int j = 0; j < m_local_ipv4_num[i]; ++j)
					{
						dump_fuc("\tip%d:[%s]\n"
							, j
							, m_local_ipv4[i][j]
							);
					}
				}
			}
			break;
			
		case 2:
			{
				dump_fuc("ARP服务套接字状态:\n");
				for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
				{
					if(m_arp_socket[i].socket < 0)
					{
						continue;
					}
					
					dump_fuc("网卡:%s 是否为虚拟网卡:%d 套接字_%d 网卡内部索引_%d 网口号_%d\n"
						, m_arp_socket[i].eth_name
						, m_arp_socket[i].is_bond_if
						, m_arp_socket[i].socket
						, m_arp_socket[i].phy_idx
						, m_arp_socket[i].fr_ifindex);
				}
				dump_fuc("\nRARP服务套接字状态:\n");
				for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_rarp_socket); ++i)
				{
					if(m_rarp_socket[i].socket < 0)
					{
						continue;
					}
					dump_fuc("网卡:%s 是否为虚拟网卡:%d 套接字_%d 网卡内部索引_%d 网口号_%d\n"
						, m_rarp_socket[i].eth_name
						, m_rarp_socket[i].is_bond_if
						, m_rarp_socket[i].socket
						, m_rarp_socket[i].phy_idx
						, m_rarp_socket[i].fr_ifindex);
				}
			}
			break;
			
		case 3:
			{
				dump_fuc("本地通道:\n");
				Common_Lock(m_serch_dev_lock);
				for(unsigned int i = 0; i < m_max_channel_num; ++i)
				{
					dump_fuc("通道 <%d>: 是否开启_%d\n", i, m_local_chanInfo[i].enable);
					if(m_local_chanInfo[i].device.szDomain[0] == '\0')
					{
						continue;
					}
					dump_fuc("链接地址_%s, 协议:[%s], MAC:%02x:%02x:%02x:%02x:%02x:%02x], IP:[%s]\n"
						, m_local_chanInfo[i].device.szDomain
						, m_local_chanInfo[i].device.szProtocolName
						, m_local_chanInfo[i].device.byMac[0]
						, m_local_chanInfo[i].device.byMac[1]
						, m_local_chanInfo[i].device.byMac[2]
						, m_local_chanInfo[i].device.byMac[3]
						, m_local_chanInfo[i].device.byMac[4]
						, m_local_chanInfo[i].device.byMac[5]
						, m_local_chanInfo[i].device.szIP);
					
				}
				Common_UnLock(m_serch_dev_lock);
			}
			break;
			
		case 4:
			{
				dump_fuc("ipc 使用状况:\n");
				Common_Lock(m_serch_dev_lock);
				dump_fuc("ipc 个数: %d\n", m_dev_use_mgr_count);
				unsigned int ipc_index = 0;
				OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs = m_use_mgr;
				while(pIPCs != NULL)
				{
					dump_fuc("ipc<%d>: mac:[%02x:%02x:%02x:%02x:%02x:%02x] ip:[%s] 被连接次数_%d\n"
						, ipc_index
						, pIPCs->byMac[0]
						, pIPCs->byMac[1]
						, pIPCs->byMac[2]
						, pIPCs->byMac[3]
						, pIPCs->byMac[4]
						, pIPCs->byMac[5]
						, pIPCs->szIP
						, pIPCs->nUseCount
						);
					OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUses = pIPCs->pWhoUse;
					unsigned int usr_index = 0;
					while(pUses != NULL)
					{
						dump_fuc("\t使用者:<%d> mac:[%02x:%02x:%02x:%02x:%02x:%02x] ip:[%s] 是否是本机在使用_%d\n"
							, usr_index
							, pUses->byMac[0]
							, pUses->byMac[1]
							, pUses->byMac[2]
							, pUses->byMac[3]
							, pUses->byMac[4]
							, pUses->byMac[5]
							, pUses->szIP
							, pUses->bLocal);
						usr_index++;
						pUses = pUses->pNext;
					}
					dump_fuc("\n");
					ipc_index++;
					pIPCs = pIPCs->pNext;
				}
				Common_UnLock(m_serch_dev_lock);
			}
			break;
			
		case 5:
			{
				dump_fuc("nvr 设备状况:\n");
				Common_Lock(m_serch_dev_lock);
				dump_fuc("nvr 设备个数:%d\n", m_dev_search_count);
				OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *dev_node = m_dev_mgr;
				unsigned int dev_index = 0;
				while(dev_node != NULL)
				{
					OVFS_ARPTOOL_ARP_SEARCH_DEV_T *dev_info = &dev_node->tDev;
					
					dump_fuc("设备信息: 索引_%d 设备类型:%d 网卡个数:%d 通道总数_%d\n"
						,dev_index
						,dev_info->tDevInfo.byDevType
						,dev_info->tDevInfo.byPhyNum
						,dev_info->tDevInfo.wTotalChanNum);
					dump_fuc("网卡信息:\n");
					for(unsigned int i = 0; i < dev_info->tDevInfo.byPhyNum; ++i)
					{
						dump_fuc("\t网卡_%d mac:[%02x:%02x:%02x:%02x:%02x:%02x] ip:[%d.%d.%d.%d]\n"
							, i
							, dev_info->tDevInfo.byMac[i][0]
							, dev_info->tDevInfo.byMac[i][1]
							, dev_info->tDevInfo.byMac[i][2]
							, dev_info->tDevInfo.byMac[i][3]
							, dev_info->tDevInfo.byMac[i][4]
							, dev_info->tDevInfo.byMac[i][5]
							, dev_info->tDevInfo.dwIP[i]&0xff
							, (dev_info->tDevInfo.dwIP[i]>>8)&0xff
							, (dev_info->tDevInfo.dwIP[i]>>16)&0xff
							, (dev_info->tDevInfo.dwIP[i]>>24)&0xff );
					}
					AntsARP_IPCSearchDeviceInfo_T *pChan = (AntsARP_IPCSearchDeviceInfo_T *)(void *)dev_info->byChanInfo;
					unsigned char *pEnable = dev_info->byEnable;
					dump_fuc("通道连接参数:\n");
					for(unsigned int j = 0; j < dev_info->tDevInfo.wTotalChanNum; ++j)
					{
						dump_fuc("\t\t通道_%d, 是否开启_%d  mac:[%02x:%02x:%02x:%02x:%02x:%02x] ip:[%s]  链接地址:%s\n"
							, j
							, pEnable[j]
							, pChan[j].byMac[0]
							, pChan[j].byMac[1]
							, pChan[j].byMac[2]
							, pChan[j].byMac[3]
							, pChan[j].byMac[4]
							, pChan[j].byMac[5]
							, pChan[j].szIP
							, pChan[j].szDomain);
					}
					dev_index++;
					dev_node = dev_node->pNext;
				}
				Common_UnLock(m_serch_dev_lock);
			}
				break;
		case 6:
			{
				dump_fuc("网线断开的网卡:\n");
				for(unsigned int i = 0; i < m_local_eth_cnt; ++i)
				{
					if(!m_net_out[i])
						continue;
					dump_fuc("网卡:%s \nmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n"
						, m_local_if_name[i]
						, m_local_mac[i][0]
						, m_local_mac[i][1]
						, m_local_mac[i][2]
						, m_local_mac[i][3]
						, m_local_mac[i][4]
						, m_local_mac[i][5]
						);
					
					for(unsigned int j = 0; j < m_local_ipv4_num[i]; ++j)
					{
						dump_fuc("\tip%d:[%s]\n"
							, j
							, m_local_ipv4[i][j]
							);
					}
				}
			}
			break;
		case 7:
			{
				dump_fuc("IP冲突的网卡:\n");
				for(unsigned int i = 0; i < m_local_eth_cnt; ++i)
				{
					for(unsigned int j = 0; j < m_local_ipv4_num[i]; ++j)
					{
						if(!m_ip_conflict[i][j])
							continue;
						dump_fuc("网卡:%s \nmac:[%02x:%02x:%02x:%02x:%02x:%02x]\nip:[%s]\n"
							, m_local_if_name[i]
							, m_local_mac[i][0]
							, m_local_mac[i][1]
							, m_local_mac[i][2]
							, m_local_mac[i][3]
							, m_local_mac[i][4]
							, m_local_mac[i][5]
							, m_local_ipv4[i][j]
							);
					}
				}
			}
			break;
		default:
			break;
			
	}
}


int ovfs_arptool_arp::open_all_support_socket()
{//需要打开每一个普通网卡和bound网卡的arp和rarp的socket
	int ret = 0;

	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(g_eth_table); ++i)
	{
		if(!ovfs_utility_is_net_dev_exist(g_eth_table[i])|| 
		   !ovfs_utility_is_net_dev_up(g_eth_table[i])|| 
			ovfs_utility_is_slave_net_dev(g_eth_table[i]))
		{
			continue;
		}
		const ovfs_inet_ipv4_ip_info_list *ipv4_config = ovfs_utility_get_ipv4_ipconfig(g_eth_table[i]);
		if(ipv4_config == NULL)
		{
			continue;
		}
		//目前只处理IPV4的地址		
		ovfs_inet_ipv4_ip_info_node *ipv4_node = ipv4_config->head;
		if(ipv4_node == NULL)
		{
			ovfs_utility_relase_ipv4_ipconfig(ipv4_config);
			continue;
		}

		int is_new = 1;
		int phy_idx = 0;
		for(unsigned int j = 0; j < OVFS_ARRAY_DIM(m_local_if_name); ++j)
		{
			if(strcmp(m_local_if_name[j], g_eth_table[i]) == 0)
			{
				phy_idx = j;
				is_new = 0;
				break;
			}
		}
		
		if(is_new&&m_local_eth_cnt < OVFS_ARRAY_DIM(m_local_if_name))		
		{
			snprintf(m_local_if_name[m_local_eth_cnt], sizeof(m_local_if_name[m_local_eth_cnt]), "%s", g_eth_table[i]);
			ovfs_utility_get_local_mac(m_local_if_name[m_local_eth_cnt], m_local_mac[m_local_eth_cnt], sizeof(m_local_mac[m_local_eth_cnt]));
			m_local_ipv4_num[m_local_eth_cnt] = 0;
			while(ipv4_node != NULL && m_local_ipv4_num[m_local_eth_cnt] < OVFS_ARRAY_DIM(m_local_ipv4[m_local_eth_cnt]))
			{
				snprintf(m_local_ipv4[m_local_eth_cnt][m_local_ipv4_num[m_local_eth_cnt]], sizeof(m_local_ipv4[m_local_eth_cnt][m_local_ipv4_num[m_local_eth_cnt]]), "%s", ipv4_node->info.ip);
				m_local_ipv4_num[m_local_eth_cnt]++;
				ipv4_node = ipv4_node->next;
			}
			phy_idx = m_local_eth_cnt;
			m_local_eth_cnt ++;
		}
		//LOGD("phy_idx = %d, m_local_eth_cnt = %d\n", phy_idx, m_local_eth_cnt);
		OVFS_ASSERT(phy_idx < (int)m_local_eth_cnt && phy_idx < (int)OVFS_ARRAY_DIM(m_local_if_name));
		ovfs_utility_relase_ipv4_ipconfig(ipv4_config);
		unsigned int if_index;
		if(is_need_open_socket(g_eth_table[i], 0, &if_index) == 1)
		{
			m_arp_socket[if_index].socket = open_socket(g_eth_table[i], 0, &m_arp_socket[if_index].fr_ifindex);
			if(m_arp_socket[if_index].socket < 0)
			{
				ret = -1;
				LOGE("open_socket fail return %d,if_index=%d,ret=%d\n",m_arp_socket[if_index].socket,if_index,ret);
			}
			else
			{
				snprintf(m_arp_socket[if_index].eth_name, sizeof(m_arp_socket[if_index].eth_name), "%s", g_eth_table[i]);
				m_arp_socket[if_index].is_bond_if = 0;
				m_arp_socket[if_index].phy_idx = phy_idx;
				ret = 1;
			}
		}
#if 0
		if(is_need_open_socket(g_eth_table[i],  1, &if_index) == 1)
		{
			m_rarp_socket[if_index].socket = open_socket(g_eth_table[i], 1, &m_rarp_socket[if_index].fr_ifindex);
			if(m_rarp_socket[if_index].socket < 0)
			{
				ret = -2;
				LOGE("open_socket fail return %d,if_index=%d,ret=%d\n",m_arp_socket[if_index].socket,if_index,ret);
			}
			else
			{
				snprintf(m_rarp_socket[if_index].eth_name, sizeof(m_rarp_socket[if_index].eth_name), "%s", g_eth_table[i]);
				m_rarp_socket[if_index].is_bond_if = 0;
				m_rarp_socket[if_index].phy_idx = phy_idx;
				ret = 1;
			}
		}
#endif		
	}
	return ret;
}


int ovfs_arptool_arp::close_all_socket()
{
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
	{
		if(m_arp_socket[i].socket<0)
			continue;
		close(m_arp_socket[i].socket);
		m_arp_socket[i].socket = -1;
	}

	
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_rarp_socket); ++i)
	{
		if(m_rarp_socket[i].socket<0)
			continue;
		close(m_rarp_socket[i].socket);
		m_rarp_socket[i].socket = -1;
	}
	return 0;
}
	


int ovfs_arptool_arp::is_need_open_socket(const char *if_name,int is_rarp, unsigned int *socket_index)
{
	if(if_name == NULL)
		return 0;

	ovfs_arptool_arp_socket_t *socket = NULL;
	unsigned int dim = 0;
	
	if(is_rarp)
	{
		socket = m_rarp_socket;
		dim = OVFS_ARRAY_DIM(m_rarp_socket);
	}
	else
	{
		socket = m_arp_socket;
		dim = OVFS_ARRAY_DIM(m_arp_socket);
	}

	*socket_index = -1;
	
	for(unsigned int i = 0; i < dim; i++)
	{
		if(strcmp(socket[i].eth_name, if_name) == 0
			&& socket[i].socket >= 0)
		{
			return 0;
		}
		
		if(socket[i].socket == -1)
		{
			*socket_index = i;
		}
	}
	if(*socket_index == (unsigned int)-1)
	{
		return 0;
	}
	return 1;
}

void ovfs_arptool_arp::str_mac_2_ucmac(char *src_mac, unsigned char *dst, unsigned int dst_size)
{
	if(dst == NULL
		||dst_size < 6
		|| src_mac == NULL)
	{
		return;
	}
	
	unsigned int mac[6];
	sscanf(src_mac, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	dst[0] = mac[0];
	dst[1] = mac[1];
	dst[2] = mac[2];
	dst[3] = mac[3];
	dst[4] = mac[4];
	dst[5] = mac[5];
	return ;
			
}

int ovfs_arptool_arp::open_socket(const char *if_name, int is_rarp, int *fr_ifindex)
{
	struct sockaddr_ll my_etheraddr;
	struct ifreq  ifr;						/*网络接口结构*/  
	int nSocket = -1;
	int nSocket1 = -1;
	int ret;
	if(if_name == NULL || fr_ifindex == NULL)
	{
		return -1;
	}
	
	nSocket1 = socket(AF_INET, SOCK_DGRAM, 0);
	if(nSocket1 < 0)
	{
		printf("[%s.%d] error = %d [%s]\n",__FUNCTION__,__LINE__,errno,strerror(errno));
		return -2;
	}
	memset(&ifr,0,sizeof(struct ifreq));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name),"%s", if_name);			/*"eth0"写入ifr结构的一个字段中*/  
	ret = ioctl(nSocket1, SIOCGIFINDEX, &ifr,sizeof(ifr)); 	/*获得eth0的标志位值*/	
	if (ret < 0)								/*判断是否取出出错*/  
	{  
		printf("[%s.%d]phy = %s SIOCGIFINDEX error = %d [%s]\n",__FUNCTION__,__LINE__, if_name, errno, strerror(errno));
		close(nSocket1);	
		nSocket1 = -1;
		perror("can't get flags \n");  
		return -3;

	}  
	*fr_ifindex = ifr.ifr_ifindex;
	ret = ioctl(nSocket1,SIOCGIFHWADDR,&ifr,sizeof(ifr));
	if (ret < 0)								/*判断是否取出出错*/  
	{  
		printf("[%s.%d]phy = %s SIOCGIFINDEX error = %d [%s]\n",__FUNCTION__,__LINE__, if_name, errno, strerror(errno));
		close(nSocket1);	
		nSocket1 = -1;
		//perror("can't get flags \n");  
		return -4;
	}  
	if(memcmp(ifr.ifr_hwaddr.sa_data,g_byBroadcastMac,6) == 0)
	{
		close(nSocket1);	
		nSocket1 = -1;
		return -5;
	}
	close(nSocket1);
	

	memset(&my_etheraddr,0,sizeof(my_etheraddr));
	my_etheraddr.sll_family = AF_PACKET;
	my_etheraddr.sll_protocol =  htons(is_rarp ? OVFS_ARPTOOL_RARP_FTYPE:OVFS_ARPTOOL_ARP_FTYPE);
	my_etheraddr.sll_ifindex = *fr_ifindex; /*接口号2表示是eth0*/
	//printf("[%s.%d] error = %d [%s] idx = %d nEthIdx = %d\n",__FUNCTION__,__LINE__,errno,strerror(errno),m_nfr_ifindex[nPhyIdx],nPhyIdx);
	nSocket = socket(PF_PACKET,SOCK_RAW,htons(is_rarp ? OVFS_ARPTOOL_RARP_FTYPE:OVFS_ARPTOOL_ARP_FTYPE));
	if(nSocket < 0)
	{
		printf("[%s.%d]Phy = %s error = %d [%s]\n",__FUNCTION__,__LINE__,if_name,errno,strerror(errno));
		return -6;
	}

	int curFlags = fcntl(nSocket, F_GETFL, 0);
    if (fcntl(nSocket, F_SETFL, curFlags|O_NONBLOCK) < 0)
    {
		close(nSocket);
		nSocket = -1;
		return -7;
    }	
	
	int SocketBuffsize = 65535;    

	setsockopt(nSocket,SOL_SOCKET,SO_RCVBUF,(const char *)&SocketBuffsize,sizeof(int));  

	if(bind(nSocket,(struct sockaddr *)&my_etheraddr,sizeof(my_etheraddr))!=0)
	{
		//printf("[%s.%d]nPhyIdx = %d error = %d [%s]\n",__FUNCTION__,__LINE__,nPhyIdx,errno,strerror(errno));
		close(nSocket);
		nSocket = -1;
		return -8;
	}
	return nSocket;
}

OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *ovfs_arptool_arp::proc_find_ipc_from_usr_mgr(AntsARP_IPCSearchDeviceInfo_T *chan_dev)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs = m_use_mgr;
	while(pIPCs != NULL)
	{
		if(memcmp(pIPCs->byMac,chan_dev->byMac,6) == 0 
			&& 0 == strcmp(chan_dev->szIP, pIPCs->szIP) )
		{
			return pIPCs;
		}
		pIPCs = pIPCs->pNext;
	}

	return NULL;
}


void ovfs_arptool_arp::delete_ipc_self_usr_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUseS = NULL;
	do
	{
		pUseS = proc_find_self_from_ipc_user(pIPCs->pWhoUse);
		if(pUseS == NULL)
			break;

		OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pDeleteIPCs = pUseS;
		if(pUseS == pIPCs->pWhoUse)
		{
			pIPCs->pWhoUse = pUseS->pNext;
			if(pIPCs->pWhoUse != NULL)
			{
				pIPCs->pWhoUse->pPrev = NULL;
			}
		}
		else
		{
			pUseS->pPrev->pNext = pUseS->pNext;
			if(pUseS->pNext != NULL)
			{
				pUseS->pNext->pPrev = pUseS->pPrev;
			}
		}
		pIPCs->nUseCount--;
		Common_Free(pDeleteIPCs,__FUNCTION__,__LINE__);
		pDeleteIPCs = NULL;
	}while(pUseS != NULL);
	return;
}

void ovfs_arptool_arp::delete_use_ipc_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs)
{
	if(pIPCs == NULL)
		return;
	if(m_use_mgr == pIPCs)
	{
		m_use_mgr = pIPCs->pNext;
		if(m_use_mgr != NULL)
		{
			m_use_mgr->pPrev = NULL;
		}
	}
	else
	{
		pIPCs->pPrev->pNext = pIPCs->pNext;
		if(pIPCs->pNext != NULL)
		{
			pIPCs->pNext->pPrev = pIPCs->pPrev;
		}
	}

	Common_Free(pIPCs,__FUNCTION__,__LINE__);

	m_dev_use_mgr_count--;
	OVFS_ASSERT(m_dev_use_mgr_count >= 0);
	return;
}

OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *ovfs_arptool_arp::proc_find_self_from_ipc_user(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *who_use)
{
//每次都要从network模块获取自己的mac地址,因为有网卡的负载均衡模式开启导致MAC地址改变， 所以要想找到ipc的使用者需要比对所有的网卡mac

	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *p_nvr = who_use;
	while(p_nvr != NULL)
	{
		if(p_nvr->bLocal)
		{
			return p_nvr;
		}
		
		for(unsigned int i = 0; i < m_local_eth_cnt; ++i)
		{
			if(memcmp(m_local_mac[i], p_nvr->byMac, 6) == 0)
			{
				return p_nvr;
			}
		}

		p_nvr = p_nvr->pNext;
	}
	return NULL;
}



OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *ovfs_arptool_arp::malloc_ipc_node(AntsARP_IPCSearchDeviceInfo_T *dev)
{
	if(dev == NULL)
		return NULL;

	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *node = (OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *)Common_Malloc(sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T),0,__FUNCTION__,__LINE__);
	if(node == NULL)
		return NULL;
	
	memset(node, 0, sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T) );
	memcpy(node->byMac, dev->byMac,6);
	snprintf(node->szIP, sizeof(node->szIP), "%s", dev->szIP);
	return node;
}

OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *ovfs_arptool_arp::malloc_usr_node(unsigned char *mac, char *ip, char is_local)
{
	if(m_local_eth_cnt == 0)
		return NULL;

	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *node = (OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *)Common_Malloc(sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T),0,__FUNCTION__,__LINE__);
	if(node == NULL)
		return NULL;
	
	memset(node, 0, sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T) );
	node->bLocal = is_local;
	memcpy(node->byMac, mac, 6);
	snprintf(node->szIP, sizeof(node->szIP), "%s", ip);
	return node;
}



void ovfs_arptool_arp::add_ipc_node_2_use(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs)
{
	if(pIPCs == NULL )
	{
		return;
	}
	pIPCs->pNext = m_use_mgr;
	if(m_use_mgr != NULL)
	{
		m_use_mgr->pPrev = pIPCs;
	}
	m_use_mgr = pIPCs;
	m_dev_use_mgr_count++;
	return;
}

void ovfs_arptool_arp::add_use_node_2_ipc(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs, OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUser)
{
	if(pIPCs == NULL|| pUser == NULL)
		return;

	
	pUser->pNext = pIPCs->pWhoUse;
	if(pIPCs->pWhoUse != NULL)
	{
		pIPCs->pWhoUse->pPrev = pUser;
	}
	pIPCs->pWhoUse = pUser;
	pIPCs->nUseCount++;
	return;
}

void ovfs_arptool_arp::update_local_chan_info(ovfs_arp_channel_device *chan_info, int chan_count)
{
	unsigned int cp_dim = min_u32(m_max_channel_num, (unsigned int)chan_count);
	memset(m_local_chanInfo, 0, sizeof(ovfs_arp_channel_device) * m_max_channel_num);
	if(chan_info == NULL || chan_count == 0)
	{
		m_local_chan_count = 0;
		return;
	}
	memcpy(m_local_chanInfo, chan_info, sizeof(ovfs_arp_channel_device) * cp_dim);
	m_local_chan_count = cp_dim;
	return;
}

void ovfs_arptool_arp::update_local_send_pkt(ovfs_arp_channel_device *chan_info, int chan_count)
{
	unsigned int nPktIdx = 0;
	OVFS_ARPTOOL_ARPPACKET_T *pArp = NULL;
	char *pBuffer = NULL,*pNewBuff = NULL,*pSaveBuff = NULL;
	int nNewBuffsize,nLeftSize = 0;
	int bCompressed;
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T *pPktChan = NULL;
	
	for(nPktIdx = 0; nPktIdx < (unsigned int)m_send_pkt_num; nPktIdx++)
	{
		if(m_send_pkt[nPktIdx] != NULL)
		{
			Common_Free(m_send_pkt[nPktIdx],__FUNCTION__,__LINE__);
			m_send_pkt[nPktIdx] = NULL;
		}
	}
	
	m_send_pkt_num = 0;
	nPktIdx = 0;
	
	if(chan_info == NULL || chan_count == 0)
	{
		return;
	}
	
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearch = NULL;
	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = NULL;
	unsigned int nOffset = 0;
	for( int nCh = 0; nCh < chan_count; nCh++)
	{

	// printf("[%s.%d]nCh = %d\n",__FUNCTION__,__LINE__,nCh);
		pBuffer = (char*)(void *)&chan_info[nCh];

		nNewBuffsize = 0;
		pNewBuff = arp_compress_data(pBuffer, sizeof(AntsARP_IPCSearchDeviceInfo_T), &nNewBuffsize);
		if(pNewBuff == NULL || (unsigned int )nNewBuffsize >= sizeof(AntsARP_IPCSearchDeviceInfo_T))
		{
			if(pNewBuff != NULL)
			{
				Common_Free(pNewBuff,__FUNCTION__,__LINE__);
				pNewBuff = NULL;
			}
			pNewBuff = pBuffer;
			nNewBuffsize = sizeof(AntsARP_IPCSearchDeviceInfo_T);
			bCompressed = 0;

		}
		else
		{
			bCompressed = 1;
		}
		// printf("[%d]bCompressed = %d nNewBuffsize = %d [%d] nLeftSize=%d nOffset = %d nPktIdx = %d\n",nCh,bCompressed,nNewBuffsize,nInfoSizePerChan,nLeftSize,nOffset,nPktIdx);
		if(pSaveBuff != NULL)
		{
			if(nNewBuffsize + (int)(sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T)) > nLeftSize)
			{
				// 开新的
				nPktIdx++;
			}
		}
		if(m_send_pkt[nPktIdx] == NULL)
		{
			m_send_pkt[nPktIdx] = (OVFS_ARPTOOL_ARP_SEARCH_SNED_PKT_T*)Common_Malloc(sizeof(OVFS_ARPTOOL_ARP_SEARCH_SNED_PKT_T),0,__FUNCTION__,__LINE__);
			if(m_send_pkt[nPktIdx] == NULL)
			{
				if(bCompressed)
				{
					Common_Free(pNewBuff,__FUNCTION__,__LINE__);
					pNewBuff = NULL;
				}
				break;
			}
			
			memset(m_send_pkt[nPktIdx], 0, sizeof(OVFS_ARPTOOL_ARP_SEARCH_SNED_PKT_T));
			pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)(m_send_pkt[nPktIdx]->pBuffer);
			pArp = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
			pSearch = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *)(pArp + 1);
			pSearch->dwMark = OVFS_ARPTOOL_ARP_SEARCH_MARK;
			pSearch->byDevType = m_local_device_type;
			pSearch->wTotalChanNum = chan_count;
			pSearch->wChanInfoSize = sizeof(AntsARP_IPCSearchDeviceInfo_T);
			pSearch->wChanStartIdx = nCh;
			pSearch->wChanNum = 0;
			pSearch->dwRefreshCnt = m_send_refresh_cnt;
			m_send_pkt[nPktIdx]->nDataSize = sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T) + sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T);
			pSaveBuff = (char *)(pSearch + 1);
			nLeftSize = 1460 - m_send_pkt[nPktIdx]->nDataSize;
			nOffset = 0;
			
		}
		pPktChan = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T *)(pSaveBuff + nOffset);
	
			
		memcpy((char *)(pPktChan + 1), pNewBuff, nNewBuffsize);
		nLeftSize -= nNewBuffsize + sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T);
		pPktChan->bCompressed = bCompressed;
		pPktChan->wSize = nNewBuffsize;
		pPktChan->bEnable = chan_info[nCh].enable;
		nOffset += sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T) + nNewBuffsize;
	    pSearch->wChanNum++;
		m_send_pkt[nPktIdx]->nDataSize += sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T) + nNewBuffsize;
		if(bCompressed)
		{
			Common_Free(pNewBuff,__FUNCTION__,__LINE__);
			pNewBuff = NULL;
		}
		//printf("wChanNum = %d nOffset = %d nLeftSize = %d\n",pSearch->wChanNum,nOffset,nLeftSize);
	}
	m_send_pkt_num= nPktIdx + 1;
	m_send_refresh_cnt++;
	return;
}
	



int ovfs_arptool_arp::is_conflict_with_self(unsigned char *byMac_in/*[6]*/, char *pIP, unsigned char *byMac_out)
{
	int is_conflict = 0;

	// 检查与本地IP是否冲突
	if(byMac_in != NULL)
	{
        printf("mac_in :[%02x:%02x:%02x:%02x:%02x:%02x] ip_%s \n"
		, byMac_in[0],byMac_in[1],byMac_in[2],byMac_in[3],byMac_in[4],byMac_in[5] ,pIP);
	}


	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(g_eth_table); ++i)
	{
		
		if(!ovfs_utility_is_net_dev_exist(g_eth_table[i])|| 
		   !ovfs_utility_is_net_dev_up(g_eth_table[i])|| 
			ovfs_utility_is_slave_net_dev(g_eth_table[i]))
		{
			continue;
		}
		
		const ovfs_inet_ipv4_ip_info_list *ipv4_config = ovfs_utility_get_ipv4_ipconfig(g_eth_table[i]);
		if(ipv4_config == NULL)
		{
			continue;
		}
		U8 local_mac[6];
		ovfs_utility_get_local_mac(g_eth_table[i], local_mac, sizeof(local_mac));
		
		ovfs_inet_ipv4_ip_info_node *ipv4_node = ipv4_config->head;
		while(ipv4_node != NULL)
		{
			if(strcmp(ipv4_node->info.ip, pIP) == 0)
			{
				if(byMac_in == NULL)
				{
					is_conflict = 1;
					break;
				}
				if(memcmp(byMac_in, local_mac,6) != 0)
				{
					is_conflict = 1;
					break;
				}
			}
			
			if(is_conflict)
			{
				if(byMac_out != NULL)
				{
					memcpy(byMac_out, local_mac, 6);
				}
				break;
			}
			ipv4_node = ipv4_node->next;
		}
		ovfs_utility_relase_ipv4_ipconfig(ipv4_config);
		if(is_conflict)
		{
			break;
		}
	}
	return is_conflict;
}


void ovfs_arptool_arp::update_local_usr_mac(const unsigned char *mac, const char *ip)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs,*pUseS;
	pIPCs = m_use_mgr;
	while(pIPCs != NULL)
	{
		pUseS = pIPCs->pWhoUse;
		while(pUseS != NULL)
		{
			if(pUseS->bLocal)
			{// 找到
				memcpy(pUseS->byMac, mac, 6);
				snprintf(pUseS->szIP, sizeof(pUseS->szIP), "%s", ip);
			}
			pUseS = pUseS->pNext;
		}
		pIPCs = pIPCs->pNext;
	}
	return;
}


void ovfs_arptool_arp::fd_set_all(fd_set *readSet, fd_set *execptSet, int *max_socket)
{
	int max = 0;
	FD_ZERO(readSet);
	FD_ZERO(execptSet);

	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
	{
		if(m_arp_socket[i].socket < 0)
		{
			continue;
		}
		FD_SET(m_arp_socket[i].socket,readSet);
		FD_SET(m_arp_socket[i].socket,execptSet);
		if(max < m_arp_socket[i].socket)
		{
			max = m_arp_socket[i].socket;
		}
	}
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_rarp_socket); ++i)
	{
		if(m_rarp_socket[i].socket < 0)
		{
			continue;
		}
		FD_SET(m_rarp_socket[i].socket,readSet);
		FD_SET(m_rarp_socket[i].socket,execptSet);
		if(max < m_rarp_socket[i].socket)
		{
			max = m_rarp_socket[i].socket;
		}
	}
	*max_socket = max;
	return ;
}

void ovfs_arptool_arp::send_all_ip_conflict_pkt()
{
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_conflict_require); i++)
	{
		if(m_conflict_require[i] == NULL)
		{
			continue;
		}
		if(m_conflict_require[i]->nStatus != 1)
		{
			continue;
		}
		m_conflict_require[i]->nTimeoutCnt--;
		//printf("nTimeoutCnt = %d\n",m_conflict_require[i]->nTimeoutCnt);
		if(m_conflict_require[i]->nTimeoutCnt < 0)
		{
			m_conflict_require[i]->nStatus = 3;
			sem_post(&m_conflict_require[i]->hSem_Res);//发送信号量让ip冲突检测的主函数收到，返回
			m_conflict_require[i] = NULL;
		}
		else
		{
			for(unsigned int ncheck = 0; ncheck < OVFS_ARRAY_DIM(m_arp_socket); ncheck++)
			{
				if(m_arp_socket[ncheck].socket<0
					|| m_conflict_require[i]->pArpPacket == NULL)
				{
					continue;
				}
				
				unsigned int phy_idx = m_arp_socket[ncheck].phy_idx;
				struct sockaddr_ll tDstEtheraddr;
 				int nSendSize = 60;
	  			OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)m_conflict_require[i]->pArpPacket;
				OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
				
				memcpy(pEther->bySrcMac,m_local_mac[phy_idx],6);
				memcpy(pARPPacket->bySrcMac,m_local_mac[phy_idx],6);
				pARPPacket->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][0]);
				
				memset(&tDstEtheraddr, 0, sizeof(tDstEtheraddr));
				tDstEtheraddr.sll_ifindex = m_arp_socket[ncheck].fr_ifindex; /*接口号2表示是eth0*/
				int nRet = sendto(m_arp_socket[ncheck].socket,m_conflict_require[i]->pArpPacket,m_conflict_require[i]->nArpPacketSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
				if(nRet != nSendSize)
				{
					printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
					
				}
				/*
				printf("MAC:%02x:%02x:%02x:%02x:%02x:%02x IP:%#x\n"
					, m_conflict_require[i]->byReqMac[0]
					, m_conflict_require[i]->byReqMac[1]
					, m_conflict_require[i]->byReqMac[2]
					, m_conflict_require[i]->byReqMac[3]
					, m_conflict_require[i]->byReqMac[4]
					, m_conflict_require[i]->byReqMac[5]
					, m_conflict_require[i]->dwReqIP);
				printf("############################################################################################\n");
				for(int x = 0; x < m_conflict_require[i]->nArpPacketSize; ++x)
				{
					printf("%#x, ", m_conflict_require[i]->pArpPacket[x]);
					if(x%16 == 0)
					{
						printf("\n");
					}
				}
				printf("\n############################################################################################\n");
				*/
			}
		}
	}
	return ;
}

void ovfs_arptool_arp::check_mac_and_ip_change(const char *if_name, int socket, unsigned int phy_idx, int fr_ifindex)
{
	unsigned char u8Mac[6];
	if(ovfs_utility_get_local_mac(if_name, u8Mac, sizeof(u8Mac)) == OVFS_SUCCESS)
	{
		if(memcmp(m_local_mac[phy_idx], u8Mac, sizeof(m_local_mac[phy_idx])))
		{
			memcpy(m_local_mac[phy_idx], u8Mac, sizeof(m_local_mac[phy_idx]));
		}
	}

	if(!ovfs_utility_is_net_dev_exist(if_name)|| 
	   !ovfs_utility_is_net_dev_up(if_name)|| 
		ovfs_utility_is_slave_net_dev(if_name))
	{
		return;
	}
	const ovfs_inet_ipv4_ip_info_list *ipv4_config = ovfs_utility_get_ipv4_ipconfig(if_name);
	if(ipv4_config == NULL)
	{
		return ;
	}

	ovfs_inet_ipv4_ip_info_node *ipv4_node = ipv4_config->head;
	if(ipv4_node == NULL)
	{
		ovfs_utility_relase_ipv4_ipconfig(ipv4_config);
		return;
	}
	unsigned int ip_index = 0;
	while(ipv4_node != NULL && ip_index < OVFS_ARRAY_DIM(m_local_ipv4[phy_idx]))
	{
		if(strcmp(m_local_ipv4[phy_idx][ip_index], ipv4_node->info.ip) != 0)
		{
			snprintf(m_local_ipv4[phy_idx][ip_index], sizeof(m_local_ipv4[phy_idx][ip_index]), "%s", ipv4_node->info.ip);
		}
		ip_index++;
		ipv4_node = ipv4_node->next;
	}
	m_local_ipv4_num[phy_idx] = ip_index;
	//printf("ip_index = %d\n",ip_index);
	ovfs_utility_relase_ipv4_ipconfig(ipv4_config);
	return;
}

void ovfs_arptool_arp::check_net_out_update(const char *if_name, int socket, unsigned int phy_idx, int fr_ifindex)
{
	struct ifreq ifr;
	OVFS_CLR_ARG(ifr);
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
	//printf("[%d]check ....SIOCGIFFLAGS\n",phy_idx);
	// SIOCGIFFLAGS
	if (ioctl(socket,SIOCGIFFLAGS,&ifr) == 0)
	{
		if (ifr.ifr_ifru.ifru_flags & IFF_RUNNING)
		{
			m_net_out[phy_idx] = 0;
		}
		else
		{
			m_net_out[phy_idx] = 1;
		}
	}
	else 
	{// SIOCETHTOOL
		struct ethtool_value edata;
		memset(&ifr, 0, sizeof(ifr));
		memset(&edata,0,sizeof(edata));
		edata.cmd = ETHTOOL_GLINK;
		//printf("[%d]check ....SIOCETHTOOL\n",i);

		snprintf(ifr.ifr_name,  sizeof(ifr.ifr_name) ,"%s", if_name);
		ifr.ifr_data = (char *)&edata;

		 if (ioctl(socket, SIOCETHTOOL, &ifr) == 0)
		 {
			m_net_out[phy_idx] = !edata.data;
		 }
		 else
		 { // SIOCGMIIREG
			unsigned short *data, mii_val;
			//printf("[%d]check ....SIOCGMIIREG\n",i);

			snprintf(ifr.ifr_name,  sizeof(ifr.ifr_name), "%s", if_name);
			if (ioctl(socket, SIOCGMIIPHY, &ifr) == 0)
			{
			 	data = (unsigned short *)(&ifr.ifr_data);
				data[1] = 1;
				if (ioctl(socket, SIOCGMIIREG, &ifr) == 0)
				{
					 // mii_val |= data[3];
					 mii_val = data[3];
					 m_net_out[phy_idx] = !(((mii_val & 0x0016) == 0x0004) ? 0 : 1);
				}
			}
		 }
	}
	if(m_net_out[phy_idx])
	{
//		printf("[%d] Net out\n",phy_idx);
	}
	return;
}

void ovfs_arptool_arp::check_ip_conflict_update(const char *if_name, int socket, unsigned int phy_idx, int fr_ifindex, char *buffer, int size)
{
	if(buffer != NULL)
	{
		for(U32 i = 0; i < m_local_ipv4_num[phy_idx]; ++i)
		{
			if(m_ip_conflict_cnt[phy_idx][i] > 0)
			{// 有冲突
				m_ip_conflict[phy_idx][i] = 1;
				printf("[%d] IPConflict %s\n",phy_idx, m_local_ipv4[phy_idx][i]);
			}
			else
			{
			 // 无冲突
			    m_ip_conflict[phy_idx][i] = 0;
			}
			m_ip_conflict_cnt[phy_idx][i] = 0;
		}
		
		send_ip_conflict_pkt(socket, phy_idx, fr_ifindex, buffer, size);
	}
	else
	{
		OVFS_CLR_ARG(m_ip_conflict);
	}
	return;
}


int ovfs_arptool_arp::send_ip_conflict_pkt(int socket,  unsigned int phy_idx, int fr_ifindex, char *buffer, int buffer_size)
{
	if(buffer_size < 60 || buffer == NULL)
	{
		return -1;
	}
	int ret = 0;
	for(unsigned int i = 0; i < m_local_ipv4_num[phy_idx] && i < OVFS_ARRAY_DIM(m_local_ipv4[phy_idx]); ++i)
	{
		struct sockaddr_ll tDstEtheraddr;
		int nSendSize = 60;
		OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
		OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
		memset(buffer + 28 + 14,0,60 - 28 - 14);
		memcpy(pEther->bySrcMac,m_local_mac[phy_idx],6);
		memset(pEther->byDstMac,0xFF,6);
		pEther->wEtherType = htons(OVFS_ARPTOOL_ARP_FTYPE);
		pARPPacket->wProtoType = htons(0x0800);
		pARPPacket->wHardType = htons(0x0001);
		pARPPacket->byHrdLen = 6;
		pARPPacket->byProtoLen = 4;
		pARPPacket->wOP = htons(1);
		memcpy(pARPPacket->bySrcMac,m_local_mac[phy_idx],6);
		pARPPacket->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][i]);
		memset(pARPPacket->byDstMac,0,6);
		pARPPacket->dwDstIP = inet_addr(m_local_ipv4[phy_idx][i]);


		memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
		tDstEtheraddr.sll_ifindex = fr_ifindex; /*接口号2表示是eth0*/
		int nRet = sendto(socket,buffer,nSendSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
		if(nRet <= 0 || nRet != nSendSize)
		{
			printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
			ret = -1;
			break;
		}
	}
		
		
	/*
	printf("MAC:%02x:%02x:%02x:%02x:%02x:%02x IP:%s\n"
					, m_local_mac[phy_idx][0]
					, m_local_mac[phy_idx][1]
					, m_local_mac[phy_idx][2]
					, m_local_mac[phy_idx][3]
					, m_local_mac[phy_idx][4]
					, m_local_mac[phy_idx][5]
					, m_local_ipv4[phy_idx]);
	
	printf("############################################################################################\n");
	for(int x = 0; x < nSendSize; ++x)
	{
		printf("%#x, ", buffer[x]);
		if(x%16 == 0 && x != 0)
		{
			printf("\n");
		}
	}
	printf("\n############################################################################################\n");
	*/			
	return ret;
}

int ovfs_arptool_arp::send_dev_search_pkt(int socket,  unsigned int phy_idx, int fr_ifindex, char *buffer, int buffer_size)
{
	if(buffer_size < 60 || buffer == NULL)
	{
		return -1;
	}
	
	struct sockaddr_ll tDstEtheraddr;
	int nSendSize = 60;
	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pArpSearch = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *)(pARPPacket + 1);
	memset(pArpSearch,0,sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T));
	memcpy(pEther->bySrcMac,m_local_mac[phy_idx],6);
	memset(pEther->byDstMac,0xFF,6);
	pEther->wEtherType = htons(OVFS_ARPTOOL_ARP_FTYPE);
	pARPPacket->wProtoType = htons(0x0800);
	pARPPacket->wHardType = htons(0x0001);
	pARPPacket->byHrdLen = 6;
	pARPPacket->byProtoLen = 4;
	pARPPacket->wOP = htons(5);
	memcpy(pARPPacket->bySrcMac,m_local_mac[phy_idx],6);
	pARPPacket->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][0]);
	memset(pARPPacket->byDstMac,0,6);
	pARPPacket->dwDstIP = inet_addr(m_local_ipv4[phy_idx][0]);
	nSendSize = sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T) + sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T);
	pArpSearch->dwMark = OVFS_ARPTOOL_ARP_SEARCH_MARK;

	memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
	tDstEtheraddr.sll_ifindex=fr_ifindex;
	int nRet = sendto(socket,buffer,nSendSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
	if(nRet <= 0 || nRet != nSendSize)
	{
		printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
	}
	return 0;
				  		
}


void ovfs_arptool_arp::broadcast_self_dev_pkt(int socket, int phy_idx, int fr_ifindex)
{
	
	char *pCurrBuffer = NULL;
	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pSendEthHeader;
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSendSearchPacket;
	OVFS_ARPTOOL_ARPPACKET_T *pSendARPPkt;
	int nPktNum, nSednPhyIdx;
	int nSendSize;
	struct sockaddr_ll tDstEtheraddr;
	
	memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
	
	tDstEtheraddr.sll_ifindex = fr_ifindex;

	for(nPktNum = 0; nPktNum < m_send_pkt_num; nPktNum++)
	{
		pCurrBuffer = m_send_pkt[nPktNum]->pBuffer;
		nSendSize = m_send_pkt[nPktNum]->nDataSize;

		pSendEthHeader = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)(pCurrBuffer);
		pSendARPPkt = (OVFS_ARPTOOL_ARPPACKET_T *)(pSendEthHeader + 1);
		pSendSearchPacket = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *)(pSendARPPkt + 1);
		memset(pSendEthHeader->byDstMac,-1,6);
		memcpy(pSendEthHeader->bySrcMac,m_local_mac[phy_idx],6);
		pSendEthHeader->wEtherType = htons(OVFS_ARPTOOL_ARP_FTYPE);
		
		pSendARPPkt->wHardType = htons(1);
		pSendARPPkt->wProtoType = htons(0x0800);
		pSendARPPkt->byHrdLen = 6;
		pSendARPPkt->byProtoLen = 4;
		memset(pSendARPPkt->byDstMac,-1,6);
		pSendARPPkt->dwDstIP = inet_addr(m_local_ipv4[phy_idx][0]);
		memcpy(pSendARPPkt->bySrcMac,m_local_mac[phy_idx],6);
		pSendARPPkt->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][0]);
		pSendARPPkt->wOP = htons(7);
		nSednPhyIdx = 0;

		//这个地方有点问题，为了做到和1.0兼容，暂时先将就着发出去的包的mac地址就填两个
		for(unsigned int j = 0; j < m_local_eth_cnt && j < OVFS_ARRAY_DIM(pSendSearchPacket->byPhyIdx); ++j)
		{
			memcpy(pSendSearchPacket->byMac[j],m_local_mac[j],6);
			pSendSearchPacket->byPhyIdx[j] = nSednPhyIdx;
			pSendSearchPacket->dwIP[j] = inet_addr(m_local_ipv4[j][0]);
			nSednPhyIdx++;
		}
		pSendSearchPacket->byPhyNum = nSednPhyIdx;
		int nRet = sendto(socket,pCurrBuffer,nSendSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
		if(nRet <= 0 || nRet != nSendSize)
		{
			printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
			
		}
	}

	
	return ;
}


void ovfs_arptool_arp::update_dev_mgr()
{
	if(m_dev_search_count > 0)
	{
		OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pDelete = NULL;
		
		while(1)
		{
			pDelete = proc_pop_dev_node();
			if(pDelete == NULL)
			{
				break;
			}
			OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs;
			AntsARP_IPCSearchDeviceInfo_T *pDeviceInfo;
			
			pDeviceInfo = (AntsARP_IPCSearchDeviceInfo_T *)(void *)pDelete->tDev.byChanInfo;
			for(int nDelIdx = 0; nDelIdx < pDelete->tDev.tDevInfo.wTotalChanNum;nDelIdx++)
			{
				if(pDeviceInfo[nDelIdx].szDomain[0] == 0)
				{
					continue;
				}
				pIPCs = proc_find_ipc_from_usr_mgr(&pDeviceInfo[nDelIdx]);
				if(pIPCs == NULL)
				{
					continue;
				}	
				proc_delete_ipc_usr(pIPCs, &pDelete->tDev.tDevInfo);
				if(pIPCs->nUseCount <= 0)
				{
					OVFS_ASSERT(pIPCs->pWhoUse == NULL);
					delete_use_ipc_node(pIPCs);
					
				}
			}
			proc_del_dev_node(pDelete);
		}
		
	}
}


OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *ovfs_arptool_arp::proc_pop_dev_node()
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pTmp,*pDelete = NULL;
	pTmp = m_dev_mgr_tail;
	while(pTmp != NULL)
	{
		if(pTmp->tDev.dwRunTimeSecond + 60 > m_run_time_s)
			return NULL;

		OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *prev = pTmp->pPrev;
		OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *next = pTmp->pNext;
		pDelete = pTmp;
		if(prev == NULL)
		{
			m_dev_mgr = next;
		}
		else
		{
			prev->pNext = next;
		}
		
		if(next == NULL)
		{
			m_dev_mgr_tail = prev;
		}
		else
		{
			next->pPrev = prev;
		}
		m_dev_search_count--;
		return pDelete;
		
		pTmp = pTmp->pNext;
	}
	return NULL;
}

void ovfs_arptool_arp::proc_del_dev_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pDelete)
{
	if(pDelete->tDev.byChanInfo != NULL)
	{
		Common_Free(pDelete->tDev.byChanInfo,__FUNCTION__,__LINE__);
		pDelete->tDev.byChanInfo = NULL;
	}
	if(pDelete->tDev.byEnable != NULL)
	{
		Common_Free(pDelete->tDev.byEnable,__FUNCTION__,__LINE__);
		pDelete->tDev.byEnable = NULL;
	}
	
	Common_Free(pDelete,__FUNCTION__,__LINE__);
	pDelete = NULL;
	return;
}



OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T * ovfs_arptool_arp::proc_find_ipc_usr_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T* who_use, OVFS_ARPTOOL_ARP_SEARCH_DEVINFO_T *dev_info)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *p_nvr = who_use;
	while(p_nvr != NULL)
	{
		for(unsigned int i = 0; i < dev_info->byPhyNum && i < OVFS_ARRAY_DIM(dev_info->byMac); ++i)
		{
			if(memcmp(dev_info->byMac[i], p_nvr->byMac, 6) == 0)
			{
				return p_nvr;
			}
		}
		p_nvr = p_nvr->pNext;
	}
	return NULL;
}

void ovfs_arptool_arp::proc_delete_ipc_usr(OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T* pIPCs, OVFS_ARPTOOL_ARP_SEARCH_DEVINFO_T *dev_info)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUseS = NULL;
	do
	{
		pUseS = proc_find_ipc_usr_node(pIPCs->pWhoUse, dev_info);
		if(pUseS == NULL)
			break;

		OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pDeleteIPCs = pUseS;
		if(pUseS == pIPCs->pWhoUse)
		{
			pIPCs->pWhoUse = pUseS->pNext;
			if(pIPCs->pWhoUse != NULL)
			{
				pIPCs->pWhoUse->pPrev = NULL;
			}
		}
		else
		{
			pUseS->pPrev->pNext = pUseS->pNext;
			if(pUseS->pNext != NULL)
			{
				pUseS->pNext->pPrev = pUseS->pPrev;
			}
		}
		pIPCs->nUseCount--;
		Common_Free(pDeleteIPCs,__FUNCTION__,__LINE__);
		pDeleteIPCs = NULL;
	}while(pUseS != NULL);
	return;
}








/*void ovfs_arptool_arp::recv_all_socket_data(int &need_open_socket,  char *buffer, int buffer_size)
{
	for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_arp_socket); ++i)
	{
		if(m_arp_socket[i].socket < 0)
		{
			continue;
		}
		int ret = recv_data(m_arp_socket[i].socket, m_arp_socket[i].phy_idx,  m_arp_socket[i].fr_ifindex, buffer, buffer_size);
		if(ret < 0)
		{
			close(m_arp_socket[i].socket);
			m_arp_socket[i].socket = -1;
			need_open_socket = 1;
		}
	}

	return ;
}*/

void ovfs_arptool_arp::respon_rarp_require(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len)
{
	return;
	if((unsigned int)data_len < sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T))
		return ;

	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
	struct sockaddr_ll tDstEtheraddr;
	if(pEther->wEtherType != htons(OVFS_ARPTOOL_RARP_FTYPE))
	{
		return ;
	}
	int nSendSize = data_len;
	if(pARPPacket->wOP == htons(3))
	{
		// 检查是自己的包
		if(0 == memcmp(pARPPacket->byDstMac, m_local_mac[phy_idx], 6))
		{
			
			/*printf("[%d][%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]wEtherType = %x,op=%d,ip=[%x ,%x,%x],mac=[%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]\n",phy_idx,\
			pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],\
			pEther->byDstMac[0],pEther->byDstMac[1],pEther->byDstMac[2],pEther->byDstMac[3],pEther->byDstMac[4],pEther->byDstMac[5],\
			pEther->wEtherType,pARPPacket->wOP,pARPPacket->dwSrcIP,pARPPacket->dwDstIP, inet_addr(m_local_ipv4[phy_idx]),\
			pARPPacket->bySrcMac[0],pARPPacket->bySrcMac[1],pARPPacket->bySrcMac[2],pARPPacket->bySrcMac[3],pARPPacket->bySrcMac[4],pARPPacket->bySrcMac[5],\
			pARPPacket->byDstMac[0],pARPPacket->byDstMac[1],pARPPacket->byDstMac[2],pARPPacket->byDstMac[3],pARPPacket->byDstMac[4],pARPPacket->byDstMac[5]);*/
				// 填地址
			memcpy(pEther->byDstMac,pEther->bySrcMac,6);
			memcpy(pEther->bySrcMac,m_local_mac[phy_idx],6);

			memcpy(pARPPacket->byDstMac,pEther->byDstMac,6);
			pARPPacket->dwDstIP = pARPPacket->dwSrcIP;
			memcpy(pARPPacket->bySrcMac,m_local_mac[phy_idx],6);
			pARPPacket->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][0]);

			pARPPacket->wOP = htons(4);
			memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
			tDstEtheraddr.sll_ifindex=fr_ifindex; /*接口号2表示是eth0*/
			int nRet = sendto(socket,buffer,nSendSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
			if(nRet <= 0 || nRet != nSendSize)
			{
				printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);	
			}
		}
	}
	else if(pARPPacket->wOP == htons(4))
	{
		// 应答
	}
}


void ovfs_arptool_arp::respon_arp_op_1(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len)
{
	if((unsigned int)data_len < sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T))
	{
		printf("data_len = %d\n",data_len);
		return ;
	}

	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
	struct sockaddr_ll tDstEtheraddr;
	if(pEther->wEtherType != htons(OVFS_ARPTOOL_ARP_FTYPE))
	{
		printf("wEtherType = %d\n",pEther->wEtherType);
		return ;
	}
	int nSendSize = data_len;
	if(pARPPacket->wOP != htons(1))
	{
		printf("wOP = %d\n",htons(pARPPacket->wOP));
		return ;
	}
	

	// 检查是自己的包
	for(unsigned int i = 0; i < m_local_ipv4_num[phy_idx] && i < OVFS_ARRAY_DIM(m_local_ipv4[phy_idx]); ++i)
	{/*
		if(memcmp(pEther->bySrcMac, m_local_mac[phy_idx], 6) != 0)
		{
			if(memcmp(pEther->bySrcMac, pARPPacket->bySrcMac, 6) != 0)
			{
				OVFS_ASSERT(0);
			}
			
			if(pARPPacket->bySrcMac[5] == 0x02)
				printf("phy_idx = %#x wOP = %#x, MAC:[%02x:%02x:%02x:%02x:%02x:%02x],MAC:[%02x:%02x:%02x:%02x:%02x:%02x],	SRC:%d.%d.%d.%d DST:%d.%d.%d.%d %s MAC:[%02x:%02x:%02x:%02x:%02x:%02x],\n", 
				phy_idx,
				pARPPacket->wOP, 
				pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],
			 	pARPPacket->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],
				pARPPacket->dwSrcIP & 0xff, 
				(pARPPacket->dwSrcIP>>8) & 0xff, 
				(pARPPacket->dwSrcIP>>16) & 0xff, 
				(pARPPacket->dwSrcIP>>24) & 0xff, 
				pARPPacket->dwDstIP & 0xff, 
				(pARPPacket->dwDstIP>>8) & 0xff, 
				(pARPPacket->dwDstIP>>16) & 0xff, 
				(pARPPacket->dwDstIP>>24) & 0xff,
				m_local_ipv4[phy_idx][i],
				m_local_mac[phy_idx][0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5]
				);
		}*/
		if(pARPPacket->dwSrcIP == inet_addr(m_local_ipv4[phy_idx][i]))
		{
			if(memcmp(pARPPacket->bySrcMac,m_local_mac[phy_idx],6) != 0)
			{
				// 有冲突
				m_ip_conflict_cnt[phy_idx][i]++;
				memcpy(m_conflict_mac[phy_idx][i], pARPPacket->bySrcMac,6);
				
			}
		}
	}
	
	for(unsigned int i = 0; i < m_local_ipv4_num[phy_idx] && i < OVFS_ARRAY_DIM(m_local_ipv4[phy_idx]); ++i)
	{
		if(pARPPacket->dwDstIP == inet_addr(m_local_ipv4[phy_idx][i]))
		{							
		/*	printf("[%d][%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]wEtherType = %x,op=%d,ip=[%x ,%x,%x],mac=[%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]\n",phy_idx,\
			pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],\
			pEther->byDstMac[0],pEther->byDstMac[1],pEther->byDstMac[2],pEther->byDstMac[3],pEther->byDstMac[4],pEther->byDstMac[5],\
			pEther->wEtherType,pARPPacket->wOP,pARPPacket->dwSrcIP,pARPPacket->dwDstIP,inet_addr(m_local_ipv4[phy_idx]),\
			pARPPacket->bySrcMac[0],pARPPacket->bySrcMac[1],pARPPacket->bySrcMac[2],pARPPacket->bySrcMac[3],pARPPacket->bySrcMac[4],pARPPacket->bySrcMac[5],\
			pARPPacket->byDstMac[0],pARPPacket->byDstMac[1],pARPPacket->byDstMac[2],pARPPacket->byDstMac[3],pARPPacket->byDstMac[4],pARPPacket->byDstMac[5]);
				*/					// 填地址
			memcpy(pEther->byDstMac,pEther->bySrcMac,6);
			memcpy(pEther->bySrcMac,m_local_mac[phy_idx],6);

			memcpy(pARPPacket->byDstMac, pEther->byDstMac,6);
			pARPPacket->dwDstIP = pARPPacket->dwSrcIP;
			memcpy(pARPPacket->bySrcMac,m_local_mac[phy_idx],6);
			pARPPacket->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][i]);
			
			pARPPacket->wOP = htons(2);
			
			memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
			tDstEtheraddr.sll_ifindex= fr_ifindex; 
			
			int nRet = sendto(socket, buffer,nSendSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
			if(nRet <= 0 || nRet != nSendSize)
			{
				printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
				
			}
		}
	}
	return;
}

void ovfs_arptool_arp::respon_arp_op_2(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len)
{
	if((unsigned int)data_len < sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T))
		return ;

	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
	if(pEther->wEtherType != htons(OVFS_ARPTOOL_ARP_FTYPE))
	{
		return ;
	}
	if(pARPPacket->wOP != htons(2))
	{
		return ;
	}
	// 应答
	// 检查是自己的包
/*	printf("[%d][%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]wEtherType = %x,op=%d,ip=[%x ,%x,%x],mac=[%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]\n",phy_idx,\
			pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],\
			pEther->byDstMac[0],pEther->byDstMac[1],pEther->byDstMac[2],pEther->byDstMac[3],pEther->byDstMac[4],pEther->byDstMac[5],\
			pEther->wEtherType,pARPPacket->wOP,pARPPacket->dwSrcIP,pARPPacket->dwDstIP,inet_addr(m_local_ipv4[phy_idx]),\
			pARPPacket->bySrcMac[0],pARPPacket->bySrcMac[1],pARPPacket->bySrcMac[2],pARPPacket->bySrcMac[3],pARPPacket->bySrcMac[4],pARPPacket->bySrcMac[5],\
			pARPPacket->byDstMac[0],pARPPacket->byDstMac[1],pARPPacket->byDstMac[2],pARPPacket->byDstMac[3],pARPPacket->byDstMac[4],pARPPacket->byDstMac[5]);
	*/	

	// 检查是自己的包

	int is_my = 0;
	U32 ip_index = 0;
	for(unsigned int i = 0; i < m_local_ipv4_num[phy_idx] && i < OVFS_ARRAY_DIM(m_local_ipv4[phy_idx]); ++i)
	{
		if(pARPPacket->dwSrcIP == inet_addr(m_local_ipv4[phy_idx][i]))
		{
			is_my = 1;
			ip_index = i;
			break;
		}
	}
	
	if(is_my)
	{
		if(memcmp(pARPPacket->bySrcMac,m_local_mac[phy_idx],6) != 0)
		{
			// 有冲突
			m_ip_conflict_cnt[phy_idx][ip_index]++;
			memcpy(m_conflict_mac[phy_idx][ip_index], pARPPacket->bySrcMac,6);
			
			/*printf("[%d][%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]wEtherType = %x,op=%d,ip=[%x ,%x,%x],mac=[%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]\n",phy_idx,\
			pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],\
			pEther->byDstMac[0],pEther->byDstMac[1],pEther->byDstMac[2],pEther->byDstMac[3],pEther->byDstMac[4],pEther->byDstMac[5],\
			pEther->wEtherType,pARPPacket->wOP,pARPPacket->dwSrcIP,pARPPacket->dwDstIP,inet_addr(m_local_ipv4[phy_idx]),\
			pARPPacket->bySrcMac[0],pARPPacket->bySrcMac[1],pARPPacket->bySrcMac[2],pARPPacket->bySrcMac[3],pARPPacket->bySrcMac[4],pARPPacket->bySrcMac[5],\
			pARPPacket->byDstMac[0],pARPPacket->byDstMac[1],pARPPacket->byDstMac[2],pARPPacket->byDstMac[3],pARPPacket->byDstMac[4],pARPPacket->byDstMac[5]);
		*/
		}
	}
	else
	{
		for(unsigned int i = 0; i < OVFS_ARRAY_DIM(m_conflict_require);i++)
		{
			if(m_conflict_require[i] == NULL)
			{
				continue;
			}

			if(m_conflict_require[i]->nStatus != 1)
			{
				continue;
			}

			if(pARPPacket->dwSrcIP != m_conflict_require[i]->dwReqIP)
			{
				continue;
			}
			if((!m_conflict_require[i]->bReqMacSet) || (m_conflict_require[i]->bReqMacSet && memcmp(pARPPacket->bySrcMac,m_conflict_require[i]->byReqMac,6) != 0))
			{
				OVFS_ARPTOOL_ARP_REQUIRE_T *pReq = m_conflict_require[i];
				m_conflict_require[i] = NULL;
				memcpy(pReq->byConflictMac,pARPPacket->bySrcMac,6);
				pReq->nStatus = 2;
				sem_post(&pReq->hSem_Res);
			}
		}
	}
	return;
}

void ovfs_arptool_arp::respon_arp_op_5(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len)
{
	if((unsigned int)data_len < sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T))
		return ;

	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
	struct sockaddr_ll tDstEtheraddr;
	if(pEther->wEtherType != htons(OVFS_ARPTOOL_ARP_FTYPE))
	{
		return ;
	}
	int nSendSize = data_len;
	if(pARPPacket->wOP != htons(5))
	{
		return ;
	}

	if(
		!(m_local_device_type == 2 ||
	     m_local_device_type == 4 ||
	     m_local_device_type == 5 ||
	     m_local_device_type == 6)
	  )
	{
		return;
	}
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *)(pARPPacket + 1);
/*	printf("[%d][%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]wEtherType = %x,op=%d,ip=[%x ,%x,%x],mac=[%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]\n",phy_idx,\
		pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],\
		pEther->byDstMac[0],pEther->byDstMac[1],pEther->byDstMac[2],pEther->byDstMac[3],pEther->byDstMac[4],pEther->byDstMac[5],\
		pEther->wEtherType,pARPPacket->wOP,pARPPacket->dwSrcIP,pARPPacket->dwDstIP,inet_addr(m_local_ipv4[phy_idx]),\
		pARPPacket->bySrcMac[0],pARPPacket->bySrcMac[1],pARPPacket->bySrcMac[2],pARPPacket->bySrcMac[3],pARPPacket->bySrcMac[4],pARPPacket->bySrcMac[5],\
		pARPPacket->byDstMac[0],pARPPacket->byDstMac[1],pARPPacket->byDstMac[2],pARPPacket->byDstMac[3],pARPPacket->byDstMac[4],pARPPacket->byDstMac[5]);
	printf("dwMark= %x total = %d num = %d,chsize=%d,start = %d pktsize = %d\n",pSearchPkt->dwMark,pSearchPkt->wTotalChanNum,pSearchPkt->wChanNum,pSearchPkt->wChanInfoSize,pSearchPkt->wChanStartIdx,data_len);						
*/
	if(m_send_pkt_num == 0 || pSearchPkt->dwMark != OVFS_ARPTOOL_ARP_SEARCH_MARK)
	{
		return ;
	}

	char *pCurrBuffer = NULL;
	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pSendEthHeader;
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSendSearchPacket;
	OVFS_ARPTOOL_ARPPACKET_T *pSendARPPkt;
	int nPktNum, nSednPhyIdx;
	memset(&tDstEtheraddr,0,sizeof(tDstEtheraddr));
	tDstEtheraddr.sll_ifindex=fr_ifindex; /*接口号2表示是eth0*/
	for(nPktNum = 0; nPktNum < m_send_pkt_num;nPktNum++)
	{
		pCurrBuffer = m_send_pkt[nPktNum]->pBuffer;
		nSendSize = m_send_pkt[nPktNum]->nDataSize;

		pSendEthHeader = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)(pCurrBuffer);
		pSendARPPkt = (OVFS_ARPTOOL_ARPPACKET_T *)(pSendEthHeader + 1);
		pSendSearchPacket = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *)(pSendARPPkt + 1);
		memcpy(pSendEthHeader->byDstMac,pEther->bySrcMac,6);
		memcpy(pSendEthHeader->bySrcMac,m_local_mac[phy_idx],6);
		pSendEthHeader->wEtherType = pEther->wEtherType;
		
		pSendARPPkt->wHardType = htons(1);
		pSendARPPkt->wProtoType = htons(0x0800);
		pSendARPPkt->byHrdLen = 6;
		pSendARPPkt->byProtoLen = 4;
		memcpy(pSendARPPkt->byDstMac,pEther->byDstMac,6);
		pSendARPPkt->dwDstIP = pARPPacket->dwSrcIP;
		memcpy(pSendARPPkt->bySrcMac,m_local_mac[phy_idx],6);
		pSendARPPkt->dwSrcIP = inet_addr(m_local_ipv4[phy_idx][0]);
		pSendARPPkt->wOP = htons(6);
		nSednPhyIdx = 0;

		//这个地方有点问题，为了做到和1.0兼容，暂时先将就着发出去的包的mac地址就填两个
		for(unsigned int i = 0; i < m_local_eth_cnt && i < OVFS_ARRAY_DIM(pSendSearchPacket->byPhyIdx); ++i)
		{
			memcpy(pSendSearchPacket->byMac[i],m_local_mac[i],6);
			pSendSearchPacket->byPhyIdx[i] = nSednPhyIdx;
			pSendSearchPacket->dwIP[i] = inet_addr(m_local_ipv4[i][0]);
			nSednPhyIdx++;
		}
		pSendSearchPacket->byPhyNum = nSednPhyIdx;
		
		int nRet = sendto(socket,pCurrBuffer,nSendSize,0,(struct sockaddr *)&tDstEtheraddr,sizeof(tDstEtheraddr));
		if(nRet <= 0 || nRet != nSendSize)
		{
			printf("send to errno = %d[%s]  nRet = %d - %d\n",errno,strerror(errno),nRet, nSendSize);
			
		}
	}
}

OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *ovfs_arptool_arp::proc_find_dev_node(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pTmpMgr = m_dev_mgr;
	while(pTmpMgr != NULL)
	{
		for(unsigned int i = 0; i < OVFS_ARRAY_DIM(pTmpMgr->tDev.tDevInfo.byMac) && i < pTmpMgr->tDev.tDevInfo.byPhyNum; ++i)
		{
			for(unsigned int j = 0; j < OVFS_ARRAY_DIM(pSearchPkt->byMac) && j < pSearchPkt->byPhyNum; ++j)
			{
				if(memcmp(pTmpMgr->tDev.tDevInfo.byMac[i],pSearchPkt->byMac[j],6) == 0)
				{
					return pTmpMgr;
				}
			}
		}
		pTmpMgr = pTmpMgr->pNext;
	}
	return NULL;
}

OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *ovfs_arptool_arp::malloc_search_dev_node(OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt)
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev= (OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *)Common_Malloc(sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T),0,__FUNCTION__,__LINE__);
	if(pNewDev == NULL)
		return NULL;

	memset(pNewDev,0,sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T));
	pNewDev->tDev.tDevInfo.byDevType = pSearchPkt->byDevType;
	pNewDev->tDev.tDevInfo.byPhyIdx[0]= pSearchPkt->byPhyIdx[0];
	pNewDev->tDev.tDevInfo.byPhyIdx[1]= pSearchPkt->byPhyIdx[1];
				 
	pNewDev->tDev.tDevInfo.byPhyNum= pSearchPkt->byPhyNum;
	pNewDev->tDev.tDevInfo.dwIP[0]= pSearchPkt->dwIP[0];
	pNewDev->tDev.tDevInfo.dwIP[1]= pSearchPkt->dwIP[1];
	pNewDev->tDev.tDevInfo.wTotalChanNum= pSearchPkt->wTotalChanNum;
	pNewDev->tDev.wChanInfoSize = pSearchPkt->wChanInfoSize;
	pNewDev->tDev.dwRefreshCnt = -1;
	pNewDev->tDev.dwBuffSize = pNewDev->tDev.tDevInfo.wTotalChanNum * pNewDev->tDev.wChanInfoSize;
	memcpy(pNewDev->tDev.tDevInfo.byMac, pSearchPkt->byMac,sizeof(pSearchPkt->byMac));
	pNewDev->tDev.byEnable = (unsigned char *)Common_Malloc(pNewDev->tDev.tDevInfo.wTotalChanNum,0,__FUNCTION__,__LINE__);
	if(pNewDev->tDev.byEnable != NULL)
	{
		memset(pNewDev->tDev.byEnable,-1,pNewDev->tDev.tDevInfo.wTotalChanNum);
	}
	else
	{
		Common_Free(pNewDev,__FUNCTION__,__LINE__);
		pNewDev = NULL;
		return NULL;
	}
	
	pNewDev->tDev.byChanInfo = (unsigned char *)Common_Malloc(pNewDev->tDev.dwBuffSize,0,__FUNCTION__,__LINE__);
	if(pNewDev->tDev.byChanInfo != NULL)
	{
		 memset(pNewDev->tDev.byChanInfo,0,pNewDev->tDev.dwBuffSize);
	}
	else
	{
		Common_Free(pNewDev->tDev.byEnable,__FUNCTION__,__LINE__);
		pNewDev->tDev.byEnable = NULL;
		Common_Free(pNewDev,__FUNCTION__,__LINE__);
		pNewDev = NULL;
		return NULL;
	}
	
	pNewDev->tDev.dwRunTimeSecond = m_run_time_s;
	return pNewDev;
	 
}

void ovfs_arptool_arp::push_search_dev_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev)
{
	m_dev_search_count++;
	pNewDev->pNext = m_dev_mgr;
	if(m_dev_mgr != NULL)
	{
		m_dev_mgr->pPrev = pNewDev;
	}
	else
	{
		m_dev_mgr_tail = pNewDev;
	}
	m_dev_mgr = pNewDev;

	return ;
}

void ovfs_arptool_arp::pop_search_dev_node(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev)
{
	m_dev_search_count--;
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T * prev = pNewDev->pPrev;
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T * next = pNewDev->pNext;
	if(prev == NULL)
	{
		m_dev_mgr = next;
	}
	else
	{
		prev->pNext = next;
	}
	
	if(next == NULL)
	{
		m_dev_mgr_tail = prev;
	}
	else
	{
		next->pPrev = prev;
	}

	pNewDev->pNext = NULL;
	pNewDev->pPrev = NULL;
	return;
}

int ovfs_arptool_arp::update_dev_node_by_chan_change(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev, OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt)
{
	if(pNewDev->tDev.tDevInfo.wTotalChanNum != pSearchPkt->wTotalChanNum)
	{
	 	// 通道数有变化
	 	// 同时删除相应使用
	 	
	 	// 释放内存
	 	if(pNewDev->tDev.byEnable != NULL)
	 	{
	 		Common_Free(pNewDev->tDev.byEnable,__FUNCTION__,__LINE__);
			pNewDev->tDev.byEnable = NULL;
	 	}
		if(pNewDev->tDev.byChanInfo != NULL)
	 	{
	 		Common_Free(pNewDev->tDev.byChanInfo,__FUNCTION__,__LINE__);
			pNewDev->tDev.byChanInfo = NULL;
	 	}
	 	memset(pNewDev,0,sizeof(OVFS_ARPTOOL_ARP_SEARCH_DEV_T));
		pNewDev->tDev.tDevInfo.byDevType = pSearchPkt->byDevType;
		pNewDev->tDev.tDevInfo.byPhyIdx[0]= pSearchPkt->byPhyIdx[0];
		pNewDev->tDev.tDevInfo.byPhyIdx[1]= pSearchPkt->byPhyIdx[1];

		pNewDev->tDev.tDevInfo.byPhyNum = pSearchPkt->byPhyNum;
		pNewDev->tDev.tDevInfo.dwIP[0] = pSearchPkt->dwIP[0];
		pNewDev->tDev.tDevInfo.dwIP[1] = pSearchPkt->dwIP[1];
		pNewDev->tDev.tDevInfo.wTotalChanNum = pSearchPkt->wTotalChanNum;
		pNewDev->tDev.wChanInfoSize = pSearchPkt->wChanInfoSize;
		pNewDev->tDev.dwRefreshCnt = pSearchPkt->dwRefreshCnt;
		pNewDev->tDev.dwBuffSize = pNewDev->tDev.tDevInfo.wTotalChanNum * pNewDev->tDev.wChanInfoSize;
		memcpy(pNewDev->tDev.tDevInfo.byMac, pSearchPkt->byMac,sizeof(pSearchPkt->byMac));

		pNewDev->tDev.byEnable = (unsigned char *)Common_Malloc(pNewDev->tDev.tDevInfo.wTotalChanNum,0,__FUNCTION__,__LINE__);
		if(pNewDev->tDev.byEnable != NULL)
		{
			memset(pNewDev->tDev.byEnable,-1,pNewDev->tDev.tDevInfo.wTotalChanNum);
		}
		else
		{
			return -1;
		}
		
		pNewDev->tDev.byChanInfo = (unsigned char *)Common_Malloc(pNewDev->tDev.dwBuffSize,0,__FUNCTION__,__LINE__);
		if(pNewDev->tDev.byChanInfo != NULL)
		{
			 memset(pNewDev->tDev.byChanInfo,0,pNewDev->tDev.dwBuffSize);
		}
		else
		{
			return -1;
		}
		
	}
	return 0;
}

void ovfs_arptool_arp::update_dev_node_by_chan_freshcnt(OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev, OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt)
{
	pNewDev->tDev.dwRefreshCnt = pSearchPkt->dwRefreshCnt;
	if(pNewDev->tDev.byEnable != NULL)
	{
		memset(pNewDev->tDev.byEnable,-1,pNewDev->tDev.tDevInfo.wTotalChanNum);

	}
	if(pNewDev->tDev.byChanInfo != NULL)
	{
		memset(pNewDev->tDev.byChanInfo,0,pNewDev->tDev.dwBuffSize);
	}
	return ;
}



void ovfs_arptool_arp::respon_arp_op_6_and_7(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len)
{
	if((unsigned int)data_len < sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T))
		return ;

	

	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);
	if(pEther->wEtherType != htons(OVFS_ARPTOOL_ARP_FTYPE))
	{
		return ;
	}
	if(!(pARPPacket->wOP == htons(6) ||
		pARPPacket->wOP == htons(7))
	  )
	{
		return ;
	}

	if(
		!(m_local_device_type == 2 ||
	     m_local_device_type == 4 ||
	     m_local_device_type == 5 ||
	     m_local_device_type == 6)
	  )
	{
		return;
	}
	
	OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *pSearchPkt = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_T *)(pARPPacket + 1);
	char *pChanInfoBuff = (char *)(pSearchPkt + 1);
							 
	/*printf("[%d][%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]wEtherType = %x,op=%d,ip=[%d.%d.%d.%d ,%d.%d.%d.%d,%s],mac=[%02x:%02x:%02x:%02x:%02x:%02x - >%02x:%02x:%02x:%02x:%02x:%02x]\n",phy_idx,\
		pEther->bySrcMac[0],pEther->bySrcMac[1],pEther->bySrcMac[2],pEther->bySrcMac[3],pEther->bySrcMac[4],pEther->bySrcMac[5],\
		pEther->byDstMac[0],pEther->byDstMac[1],pEther->byDstMac[2],pEther->byDstMac[3],pEther->byDstMac[4],pEther->byDstMac[5],\
		pEther->wEtherType
		,pARPPacket->wOP
		,pARPPacket->dwSrcIP&0xff, (pARPPacket->dwSrcIP>>8)&0xff, (pARPPacket->dwSrcIP>>16)&0xff, (pARPPacket->dwSrcIP>>24)&0xff
		,pARPPacket->dwDstIP&0xff, (pARPPacket->dwDstIP>>8)&0xff, (pARPPacket->dwDstIP>>16)&0xff, (pARPPacket->dwDstIP>>24)&0xff
		,m_local_ipv4[phy_idx],\
		pARPPacket->bySrcMac[0],pARPPacket->bySrcMac[1],pARPPacket->bySrcMac[2],pARPPacket->bySrcMac[3],pARPPacket->bySrcMac[4],pARPPacket->bySrcMac[5],\
		pARPPacket->byDstMac[0],pARPPacket->byDstMac[1],pARPPacket->byDstMac[2],pARPPacket->byDstMac[3],pARPPacket->byDstMac[4],pARPPacket->byDstMac[5]);
	
	printf("dwMark= %x total = %d num = %d,chsize=%d,start = %d pktsize = %d m_dev_search_count = %d\n",pSearchPkt->dwMark,pSearchPkt->wTotalChanNum,pSearchPkt->wChanNum,pSearchPkt->wChanInfoSize,pSearchPkt->wChanStartIdx,data_len,m_dev_search_count);						
*/
	if(pSearchPkt->dwMark != OVFS_ARPTOOL_ARP_SEARCH_MARK)
 	{
		return;
	}
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *pNewDev = NULL;
	
	// 搜索结果
	pNewDev = proc_find_dev_node(pSearchPkt);
	if(pNewDev == NULL)
	{
		 pNewDev= malloc_search_dev_node(pSearchPkt);
		 if(pNewDev != NULL)
		 {
		 	push_search_dev_node(pNewDev);
		 }
	}
	else
	{
		// 移动到头部
		if(pNewDev != m_dev_mgr)
		{
			pop_search_dev_node(pNewDev);
			push_search_dev_node(pNewDev);
		}
	}
	
	if(pNewDev != NULL)
	{
		int nOffSet = 0;
		OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T *pChanInfo = NULL;
		int nChIdx = pSearchPkt->wChanStartIdx;
		 
		pNewDev->tDev.dwRunTimeSecond = m_run_time_s;
		
		if(pNewDev->tDev.dwRefreshCnt == pSearchPkt->dwRefreshCnt)
		{
			if(pNewDev->tDev.tDevInfo.wTotalChanNum == pSearchPkt->wTotalChanNum)
			{
				if(nChIdx < pNewDev->tDev.tDevInfo.wTotalChanNum)
				{
					if(pNewDev->tDev.byEnable[nChIdx] == (unsigned char)-1 )
					{
						return ;
					}
				}
			}
			return;
		}

		 // 有更新
		if(pNewDev->tDev.byChanInfo != NULL 
				&&(pNewDev->tDev.tDevInfo.wTotalChanNum != pSearchPkt->wTotalChanNum
					|| pNewDev->tDev.dwRefreshCnt != pSearchPkt->dwRefreshCnt))
		{
			int nDelIdx;
			// 删除使用列表
			// 也需要从使用列表里删除
				
			AntsARP_IPCSearchDeviceInfo_T *pDeviceInfo;
			pDeviceInfo = (AntsARP_IPCSearchDeviceInfo_T *)(void *)pNewDev->tDev.byChanInfo;
			for(nDelIdx = 0; nDelIdx < pNewDev->tDev.tDevInfo.wTotalChanNum;nDelIdx++)
			{
			    OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs;
				if(pNewDev->tDev.byEnable[nDelIdx] == (unsigned char)-1 ||
				  pDeviceInfo[nDelIdx].szDomain[0] == 0)
				{
					continue;
				}
			
				pIPCs = proc_find_ipc_from_usr_mgr(&pDeviceInfo[nDelIdx]);
				if(NULL == pIPCs)
				{
					continue;
				}
				proc_delete_ipc_usr(pIPCs, &pNewDev->tDev.tDevInfo);
				if(pIPCs->nUseCount <= 0)
				{
					OVFS_ASSERT(pIPCs->pWhoUse == NULL);
					delete_use_ipc_node(pIPCs);
				}
			}
		}
		
		if(pNewDev->tDev.tDevInfo.wTotalChanNum != pSearchPkt->wTotalChanNum)
		{
			if(0 != update_dev_node_by_chan_change(pNewDev, pSearchPkt));
			{
				pop_search_dev_node(pNewDev);
				proc_del_dev_node(pNewDev);
				return;
			}
			pNewDev->tDev.dwRefreshCnt = pSearchPkt->dwRefreshCnt;
		}
		//代码逻辑冗余by turkey
		//if(pNewDev->tDev.dwRefreshCnt != pSearchPkt->dwRefreshCnt)
		//{
		//	update_dev_node_by_chan_freshcnt(pNewDev, pSearchPkt);
		//	pNewDev->tDev.dwRefreshCnt = pSearchPkt->dwRefreshCnt;
		//}
		
		for(; nChIdx < pSearchPkt->wChanNum + pSearchPkt->wChanStartIdx;nChIdx++)
		{
			pChanInfo = (OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T *)(nOffSet + pChanInfoBuff);
			nOffSet += pChanInfo->wSize + sizeof(OVFS_ARPTOOL_ARP_SEARCH_PACKET_CHANINFO_T);
			if(pChanInfo->bCompressed)
			{
				int nNewSize = 0;
				char *pNewBuf;
				pNewBuf = arp_decompress_data((char *)(pChanInfo+1),pChanInfo->wSize,&nNewSize);
				//printf("nNewSize_%d ,pNewBuf_%p", nNewSize, pNewBuf);
				if(pNewBuf != NULL)
				{
					if(nNewSize == pNewDev->tDev.wChanInfoSize)
					{
						memcpy(pNewDev->tDev.byChanInfo+nNewSize * nChIdx,pNewBuf,nNewSize);
						pNewDev->tDev.byEnable[nChIdx] = pChanInfo->bEnable;
					}
					else
					{
						printf("Size error [nNewSize = %d -%d] \n",nNewSize,pNewDev->tDev.wChanInfoSize);
					}
					Common_Free(pNewBuf,__FUNCTION__,__LINE__);
					pNewBuf = NULL;
				}
			}
			else if(pChanInfo->wSize == pNewDev->tDev.wChanInfoSize)
			{
				memcpy(pNewDev->tDev.byChanInfo+pChanInfo->wSize * nChIdx,(char *)(pChanInfo+1),pChanInfo->wSize);
				pNewDev->tDev.byEnable[nChIdx] = pChanInfo->bEnable;
			}
			else
			{
				printf("!Size error [nNewSize = %d -%d] \n",pChanInfo->wSize,pNewDev->tDev.wChanInfoSize);
			}

			AntsARP_IPCSearchDeviceInfo_T *pDeviceInfo = (AntsARP_IPCSearchDeviceInfo_T *)(void  *)(pNewDev->tDev.byChanInfo+pNewDev->tDev.wChanInfoSize * nChIdx);
			if(pDeviceInfo->szDomain[0] == 0)
			{
				continue;
			}
			OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIPCs,*pUseMgr = NULL;
			pIPCs = proc_find_ipc_from_usr_mgr(pDeviceInfo);
			if(NULL != pIPCs)
			{
				
				pUseMgr = proc_find_ipc_usr_node(pIPCs->pWhoUse, &pNewDev->tDev.tDevInfo);
			}
			if(pIPCs == NULL)
			{
				// 新的
				pIPCs = malloc_ipc_node(pDeviceInfo);
				if(pIPCs != NULL)
				{
					add_ipc_node_2_use(pIPCs);
				}
			}
			if(pUseMgr == NULL && pIPCs != NULL)
			{
				char sz_ip[64];
				snprintf(sz_ip, sizeof(sz_ip), "%d.%d.%d.%d",((pNewDev->tDev.tDevInfo.dwIP[0] >> 0) & 0xFF),((pNewDev->tDev.tDevInfo.dwIP[0] >> 8) & 0xFF),
						                       ((pNewDev->tDev.tDevInfo.dwIP[0] >> 16) & 0xFF),((pNewDev->tDev.tDevInfo.dwIP[0] >> 24) & 0xFF));
				pUseMgr = malloc_usr_node(pNewDev->tDev.tDevInfo.byMac[0], sz_ip, 0);
				if(pUseMgr != NULL)
				{
					add_use_node_2_ipc(pIPCs, pUseMgr);
				}
				
			}
		}
	}
}

void ovfs_arptool_arp::respon_arp_require(int socket, int phy_idx, int fr_ifindex, char* buffer, int data_len)
{
	if((unsigned int )data_len < sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T))
		return ;

	OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
	OVFS_ARPTOOL_ARPPACKET_T *pARPPacket = (OVFS_ARPTOOL_ARPPACKET_T *)(pEther + 1);

	if(pEther->wEtherType != htons(OVFS_ARPTOOL_ARP_FTYPE))
	{
		return ;
	}			
	if(pARPPacket->wOP == htons(1))
	{
		respon_arp_op_1(socket, phy_idx, fr_ifindex, buffer, data_len);
	}
	else if(pARPPacket->wOP == htons(2))
	{
		respon_arp_op_2(socket, phy_idx, fr_ifindex, buffer, data_len);
	}
	else if((m_local_device_type == 2 ||
		     m_local_device_type == 4 ||
		     m_local_device_type == 5 ||
		     m_local_device_type == 6)
		     && pARPPacket->wOP == htons(5))
	{
		respon_arp_op_5(socket, phy_idx, fr_ifindex, buffer, data_len);
	}
	else if((m_local_device_type == 2 ||
		     m_local_device_type == 4 ||
		     m_local_device_type == 5 ||
		     m_local_device_type == 6)&& 
		     (pARPPacket->wOP == htons(6) ||
		      pARPPacket->wOP == htons(7)))
	{
		respon_arp_op_6_and_7(socket, phy_idx, fr_ifindex, buffer, data_len);
	}
	return;
}

int ovfs_arptool_arp::recv_data(int socket, int phy_idx, int fr_ifindex, char* buffer, int buffer_size)
{
	int nErrorNo;
	int nTotalLen = 0;
	int nRet;
	int nMacIdx = phy_idx;
	
	int nMacSocket = socket;
	if(nMacSocket < 0 )
	{
		return -1;
	}
	// printf("[%d] RecvData\n",phy_idx);
	//len = 0;    
	//ioctl(nMacSocket,FIONREAD,&len);
	//if(len <= 0)
	{
	//	return -1;
	}

	if(buffer_size < (OVFS_ARPTOOL_ARP_MAX_BUFFSIZE + 16))
	{
		return -1;
	}
	
	//while(len > 0)
	while(1)
	{
		nRet = recvfrom(nMacSocket,buffer,OVFS_ARPTOOL_ARP_MAX_BUFFSIZE,0,0,0);
		nErrorNo = errno;

		if(nRet > 0)
		{
			nTotalLen += nRet;
			if(nRet >= (int)(sizeof(OVFS_ARPTOOL_ARP_ETHER_HEADER_T) + sizeof(OVFS_ARPTOOL_ARPPACKET_T)))
			{
			    OVFS_ARPTOOL_ARP_ETHER_HEADER_T *pEther = (OVFS_ARPTOOL_ARP_ETHER_HEADER_T *)buffer;
				 if(0 != memcmp(pEther->bySrcMac,m_local_mac[nMacIdx],6))
				 {// 过滤掉自己的包
					if(pEther->wEtherType == htons(OVFS_ARPTOOL_RARP_FTYPE))
					{
						respon_rarp_require(socket, phy_idx, fr_ifindex, buffer, nRet);
					}
					else if(pEther->wEtherType == htons(OVFS_ARPTOOL_ARP_FTYPE))
					{
						respon_arp_require(socket, phy_idx, fr_ifindex, buffer, nRet);
					}
				 }
			}

		}
		else if(nRet == 0)
		{
		  usleep(10000);
		  break;
		}
		else if(nErrorNo == EWOULDBLOCK ||
		 nErrorNo == EINTR||
		 nErrorNo == EAGAIN||
		 nErrorNo == EINPROGRESS)
		{
			//printf("[arp]recvfroem nErrorNo = %d \n",nErrorNo);
			usleep(10000);
			break;
		}
		else
		{
		  break;
		}
		if(m_exit_thread)
		{
			break;
		}
		//len = 0;       
		//ioctl(nMacSocket,FIONREAD,&len);
	}
	return nTotalLen;
 }


void ovfs_arptool_arp::delete_all_dev()
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *dev_node = m_dev_mgr;
	while(dev_node != NULL)
	{
		OVFS_ARPTOOL_ARP_SEARCH_DEV_MGR_T *delet_node = dev_node;
		dev_node = dev_node->pNext;
		pop_search_dev_node(delet_node);
		proc_del_dev_node(delet_node);
	}
	m_dev_mgr = NULL;
	m_dev_mgr_tail = NULL;
	return;
}

void ovfs_arptool_arp::delete_all_ipc()
{
	OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pIpc = m_use_mgr; 
	while(pIpc != NULL)
	{
		OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pDeleteIpc = pIpc;
		pIpc = pIpc->pNext;
		
		OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pUses = pDeleteIpc->pWhoUse;
		while(pUses != NULL)
		{
			OVFS_ARPTOOL_ARP_SEARCH_DEV_COUNT_MGR_T *pDeleteUsr = pUses;
			pUses = pUses->pNext;
			Common_Free(pDeleteUsr,__FUNCTION__,__LINE__);
			pDeleteUsr = NULL;
		}
		Common_Free(pDeleteIpc,__FUNCTION__,__LINE__);
		pDeleteIpc = NULL;
	}
	m_dev_use_mgr_count = 0;
	m_use_mgr= NULL;
	return;
}
