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
 fan lielong, last updated 04/13/2015
 *****************************************************************************/
#include "core.h"
#include "ccc.h"
#include "cubic_cc.h"

CUBICCC::CUBICCC()
{
    registerLinuxprotocol(&tcp_cubic);
    init();
}

/* 初始化拥塞控制算法 */
void CUBICCC::init()
{
    // 初始化sock结构
    sk.snd_cwnd = 2;
    sk.snd_cwnd_clamp = 83333;
    sk.snd_cwnd_cnt = 0;
    sk.srtt = 0;
    sk.snd_nxt = 0;
    sk.snd_una = 0;
    sk.snd_ssthresh = 83333;
    sk.snd_wnd = 83333;

    // 初始化成员变量
    m_bSlowStart = true;    //in slowstart phrase
    m_dCWndSize = 2;        //cwnd
    m_issthresh = 83333;    //sstreshold
    m_iDupACKCount = 0;     //dup count
    m_iLastACK = m_iSndCurrSeqNo;
    setACKInterval(2);
    setRTO(1000000);        //200ms

    //调用Linux内部的tcp拥塞控制选项;tcp_congestion_ops initialization
    ca_ops->init(&sk);      //callback LINUX TCP init()
    ca_ops->set_state(&sk, TCP_CA_Open);
}

/* 在此处向系统注册使用cubic算法 */
void CUBICCC::registerLinuxprotocol(struct tcp_congestion_ops *ops)
{
    ca_ops = ops;
    init();
}

void CUBICCC::onACK(int32_t ack)
{
    int in_flight = 0;
        updatesock();
        if (ack == m_iLastACK)
        {
            if (3 == ++m_iDupACKCount)
            {
                //three DupACK action
                printf("连续收到3个重复的ack\n");
            }
            else if (m_iDupACKCount > 3)
            {
                //more than three DupACK action
                printf("连续收到大于3个重复的ack\n");
            }
            else
            {
                //less than three DupACK action
                printf("连续收到小于3个重复的ack\n");
            }
        }
        else
        {
            if (m_iDupACKCount >= 3)
            {
                printf("不连续收到3个重复的ack\n");
                ca_ops->set_state(&sk, TCP_CA_Open);
                sk.snd_cwnd = ca_ops->ssthresh(&sk);
            }

            m_iLastACK = ack;
            m_iDupACKCount = 0;
            //printf("m_iRTT %d\n",m_iRTT);
            ca_ops->pkts_acked(&sk, 0, m_iRTT);
            //printf("pkts_acked\n");
            ca_ops->cong_avoid(&sk, ack, getPerfInfo()->pktFlightSize);

        }

        updateudt();    //write back sock variables to UDT variables
    //  printf("Debugsock: snd_cwnd %d snd_sstresh %d snd_nxt %d\n",sk.snd_cwnd,sk.snd_ssthresh,sk.snd_nxt);
        //m_dCWndSize = 50;
}

void CUBICCC::updateudt()
{
    sk.snd_cwnd = (int) m_dCWndSize;
    //sock.snd_cwnd_cnt = (int)
    sk.snd_ssthresh = (int) m_issthresh;
    sk.snd_nxt = m_iSndCurrSeqNo + m_iMSS;
}

void CUBICCC::onTimeout()
{
    printf("onTimeout()\n");
    printf("onTimeout()\n");
    m_issthresh = getPerfInfo()->pktFlightSize / 2;
    if (m_issthresh < 2)
        m_issthresh = 2;

    m_bSlowStart = true;
    m_dCWndSize = 2.0;
}

void CUBICCC::updatesock()
{
}


void CUBICCC::onLoss(const int32_t *, int)
{

}

void CUBICCC::statemachine(int32_t)
{

}

