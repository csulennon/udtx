/*****************************************************************************
 Copyright (c) 2001 - 2015, The Board of Trustees of the University of Illinois.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the
 following disclaimer.

 * Redistributions in binary form must reproduce the
 above copyright notice, this list of conditions
 and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of the University of Illinois
 nor the names of its contributors may be used to
 endorse or promote products derived from this
 software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
/*****************************************************************************
 written by
 fan lielong, last updated 04/17/2015
 *****************************************************************************/

#include "core.h"
#include "ccc.h"
#include "linux_cc.h"
#include "tcpabstract.h"
#include <string.h>

LINUXCC::LINUXCC()
{
    registerLinuxprotocol(&tcp_vegasx);
    //registerLinuxprotocol(&tcp_cubic);
    //init();
}

LINUXCC::LINUXCC(char* name)
{
    if (strcmp(name, "cubic"))
    {
        registerLinuxprotocol(&tcp_cubic);
    }
    else if (strcmp(name, "vegas"))
    {
        registerLinuxprotocol(&tcp_vegasx);
    }
    else
    {
        printf("未知拥塞控制算法,使用默认vegas算法.\n");
        registerLinuxprotocol(&tcp_vegasx);
    }
    init();
}

/* 在此处向系统注册拥塞控制算法 */
void LINUXCC::registerLinuxprotocol(struct tcp_congestion_ops *ops)
{
    ca_ops              = ops;
    //ca_ops->set_state   = tcp_set_ca_state;
}

/* 初始化,默认会调用 */
void LINUXCC::init()
{
    //sk initialization
    sk.snd_cwnd                 = 2;
    sk.snd_cwnd_clamp           = 83333;
    sk.snd_cwnd_cnt             = 0;
    sk.srtt                     = 0;
    sk.snd_nxt                  = 0;
    sk.snd_una                  = 0;
    sk.snd_ssthresh             = 83333;
    sk.snd_wnd                  = 83333;

    sk.prior_ssthresh           = 0;
    sk.bytes_acked              = 0;
    sk.icsk_ca_ops              = new tcp_congestion_ops;
    sk.icsk_ca_ops->set_state   = tcp_set_ca_state;
    sk.icsk_ca_ops              = NULL;
    sk.retrans_out              = 0;
    sk.sacked_out               = 0; /* SACK'd packets */
    sk.lost_out                 = 0;
    sk.snd_wl1                  = 0;
    sk.packets_out              = 0;
    sk.fackets_out              = 0;
    sk.undo_retrans             = 0;
    sk.high_seq                 = 0;

    //class var initialization
    m_bSlowStart    = true;    //in slowstart phrase
    m_bLoss         = false;
    m_dCWndSize     = 2;        //cwnd
    m_issthresh     = 83333;    //sstreshold
    m_iDupACKCount  = 0;     //dup count
    m_iLastACK      = m_iSndCurrSeqNo;
    setACKInterval(2);
    setRTO(1000000);        //200ms

    sysctl_tcp_abc = 0; // 默认值应该是0，即对每个ACK都进行拥塞避免。

    //tcp_congestion_ops initialization
    ca_ops->init(&sk);      //callback LINUX TCP init()
    if(ca_ops->set_state)
    {
        //printf("调用\n");
        ca_ops->set_state(&sk, TCP_CA_Open);
    }
    
    //printf("set_state = %p\n", ca_ops->set_state);

}

void LINUXCC::updateudt()
{
    m_dCWndSize = sk.snd_cwnd;
    m_issthresh = sk.snd_ssthresh;
}

void LINUXCC::updatesock()
{
    const CPerfMon* perf      = getPerfInfo();
    sk.snd_cwnd         = (int) m_dCWndSize;
    //sock.snd_cwnd_cnt = (int)
    sk.snd_ssthresh     = (int) m_issthresh;
    sk.snd_nxt          = m_iSndCurrSeqNo + m_iMSS;
    sk.retrans_out      = perf->pktRetrans;
    sk.lost_out         = perf->pktSndLoss;
    sk.packets_out      = perf->pktFlightSize;
    //sk.fackets_out  
       
}

void LINUXCC::onACK(int32_t ack)
{
    //printf("onack\n");
    updatesock();

    struct tcp_sock *tp = &sk;
    struct inet_connection_sock *icsk = &sk;
    u32 in_flight       = 0;      //正在传输的数据
    u32 prior_in_flight = 0;
    u32 prior_fackets   = 0;
    u32 ack_seq         = m_iSndCurrSeqNo;
    u32 prior_snd_una   = sk.snd_una;

     /*
      * 检验确认的序号是否落在SND.UNA和SND.NXT之间，否则
      * 是不合法的序号。
      * 如果确认的序号在SND.NXT的右边，则说明该序号的数据
      * 发送方还没有发送，直接返回。
      * 如果确认的序号在SND.UNA的左边，则说明已经接受过
      * 该序号的ACK了。因为每个有负载的TCP段都会顺便
      * 携带一个ACK序号，即使这个序号已经确认过。因此
      * 如果是一个重复的ACK就无需作处理直接返回即可。但
      * 如果段中带有SACK选项，则需对此进行处理
      */
    // 收到旧的ack,甚至可以忽略他
    if (before(ack, m_iLastACK))
    {
        if (sk.icsk_ca_state == TCP_CA_Open)
            tcp_try_keep_open(&sk);
    }

    /*if (after(ack, tp->snd_nxt))
    {
        printk(KERN_DEBUG "Ack %u after %u:%u\n", ack, sk.snd_una, sk.snd_nxt);
        return ;
    }*/


    // 默认不开启
    if (sysctl_tcp_abc) {
        if (icsk->icsk_ca_state < TCP_CA_CWR)
            tp->bytes_acked += ack - prior_snd_una;
        else if (icsk->icsk_ca_state == TCP_CA_Loss)
            // we assume just one segment left network 
            tp->bytes_acked += min(ack - prior_snd_una, tp->mss_cache);
    }

    const CPerfMon* perf= getPerfInfo();
    prior_fackets = tp->fackets_out;
    prior_in_flight = perf->pktFlightSize;
    in_flight  = prior_in_flight;
    int mTmpSSthresh = 0;

    if (ack == m_iLastACK)
    {
        if (3 == ++m_iDupACKCount)
        {
            //three DupACK action
            //printf("连续收到3个重复的ack\n");

            mTmpSSthresh = m_issthresh;
            m_issthresh =  m_dCWndSize / 2;
            m_dCWndSize = m_issthresh + 3;
            
            if(m_dCWndSize > m_issthresh) /*拥塞避免 */
                m_dCWndSize = m_issthresh ;
            sk.icsk_ca_ops->set_state(&sk,TCP_CA_Recovery);
            updatesock();
            ca_ops->cong_avoid(&sk, ack, perf->pktFlightSize);
            //TODO:进入Recovery状态,fast-retransmitting,直到所有的Recovery状态的数据都确认后回到Open状态
            // 重传或者超时有可能终端Recovery状态
        }
        else if (m_iDupACKCount > 3)
        {
            m_dCWndSize++;
            sk.snd_cwnd = m_dCWndSize;
            //printf("连续收到大于3个重复的ack\n");
        }
        else
        {
            //less than three DupACK action
            //printf("连续收到小于3个重复的ack\n");
        }
    }
    else
    {
        if (m_iDupACKCount >= 3)
        {
            //printf("不连续收到3个重复的ack\n");
            ca_ops->set_state(&sk, TCP_CA_Open);
            sk.snd_cwnd = ca_ops->ssthresh(&sk);
        }


        m_dCWndSize = max(mTmpSSthresh, m_dCWndSize);
        sk.snd_cwnd = m_dCWndSize;

        m_iLastACK = ack;
        m_iDupACKCount = 0;
        //printf("m_iRTT %d\n",m_iRTT);
        ca_ops->pkts_acked(&sk, 0, m_iRTT);
        //printf("pkts_acked\n");
        ca_ops->cong_avoid(&sk, ack, getPerfInfo()->pktFlightSize);

    }
    

    updateudt();
    //write back sock variables to UDT variables
    //  printf("Debugsock: snd_cwnd %d snd_sstresh %d snd_nxt %d\n",sk.snd_cwnd,sk.snd_ssthresh,sk.snd_nxt);
    //m_dCWndSize = 50;
}


void LINUXCC::onTimeout()
{
    // 进入LOSS状态
    //printf("onTimeout()\n");
    const CPerfMon* perf= getPerfInfo();
    m_issthresh = perf->pktFlightSize / 2;
    
    //printf("snd_cwnd=%d,  snd_sstresh=%d,m_issthresh=%d,  mbpsBandwidth=%f, m_iRTT=%d, msRTT=%f\n",
    //    sk.snd_cwnd,sk.snd_ssthresh,m_issthresh,perf->mbpsBandwidth,m_iRTT, perf->msRTT );

    if (m_issthresh < 2)
    {
        m_issthresh = 2;
    }

    m_bSlowStart = false;
    m_dCWndSize = 2.0;
    //updatesock();
}



void LINUXCC::onLoss(const int32_t *, int)
{
    const CPerfMon* perf= getPerfInfo();
    m_issthresh = perf->pktFlightSize / 2;
    if (m_issthresh < 2)
    {
        m_issthresh = 2;
    }
    m_dCWndSize = 2.0;
    sk.snd_cwnd = 2;
    sk.snd_ssthresh = m_issthresh;
}

void LINUXCC::statemachine(int32_t)
{
}

void LINUXCC::tcp_fastretrans_alert(int pkts_acked, int flag)
{


}

