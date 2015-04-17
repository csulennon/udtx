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
#include <string.h>

LINUXCC::LINUXCC()
{
    printf("tcp_vegas\n");
    registerLinuxprotocol(&tcp_vegasx);
    //registerLinuxprotocol(&tcp_cubic);
    init();
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
    ca_ops = ops;
}

/* 初始化 */
void LINUXCC::init()
{
    //sk initialization
    sk.snd_cwnd = 2;
    sk.snd_cwnd_clamp = 83333;
    sk.snd_cwnd_cnt = 0;
    sk.srtt = 0;
    sk.snd_nxt = 0;
    sk.snd_una = 0;
    sk.snd_ssthresh = 83333;
    sk.snd_wnd = 83333;

    //class var initialization
    m_bSlowStart = true;    //in slowstart phrase
    m_dCWndSize = 2;        //cwnd
    m_issthresh = 83333;    //sstreshold
    m_iDupACKCount = 0;     //dup count
    m_iLastACK = m_iSndCurrSeqNo;
    setACKInterval(2);
    setRTO(1000000);        //200ms

    //tcp_congestion_ops initialization
    ca_ops->init(&sk);      //callback LINUX TCP init()
    ca_ops->set_state(&sk, TCP_CA_Open);
}
/*
 为了防止网络的拥塞现象，TCP提出了一系列的拥塞控制机制。
 最初由V. Jacobson在1988年的论文中提出的TCP的拥塞控制由
 “慢启动(Slow start)”和“拥塞避免(Congestion avoidance)”组成，
 后来TCP Reno版本中又针对性的加入了“快速重传(Fast retransmit)”、
 “快速恢复(Fast Recovery)”算法，再后来在TCP NewReno中又对“快速恢复”
 算法进行了改进，近些年又出现了选择性应答( selective acknowledgement,SACK)算法，
 还有其他方面的大大小小的改进，成为网络研究的一个热点。

 TCP的拥塞控制主要原理依赖于一个拥塞窗口(cwnd)来控制，在之前我们还讨论过TCP还有
 一个对端通告的接收窗口(rwnd)用于流量控制。窗口值的大小就代表能够发送出去的但还
 没有收到ACK的最大数据报文段，显然窗口越大那么数据发送的速度也就越快，但是也有
 越可能使得网络出现拥塞，如果窗口值为1，那么就简化为一个停等协议，每发送一个数据，
 都要等到对方的确认才能发送第二个数据包，显然数据传输效率低下。TCP的拥塞控制算法
 就是要在这两者之间权衡，选取最好的cwnd值，从而使得网络吞吐量最大化且不产生拥塞。

 由于需要考虑拥塞控制和流量控制两个方面的内容，因此TCP的真正的发送窗口=min(rwnd, cwnd)。
 但是rwnd是由对端确定的，网络环境对其没有影响，所以在考虑拥塞的时候我们一般不考虑rwnd的值，
 我们暂时只讨论如何确定cwnd值的大小。关于cwnd的单位，在TCP中是以字节来做单位的，
 我们假设TCP每次传输都是按照MSS大小来发送数据的，因此你可以认为cwnd按照数据包个数来做
 单位也可以理解，所以有时我们说cwnd增加1也就是相当于字节数增加1个MSS大小。

 慢启动：最初的TCP在连接建立成功后会向网络中发送大量的数据包，这样很容易导致网络中路由器
 缓存空间耗尽，从而发生拥塞。因此新建立的连接不能够一开始就大量发送数据包，而只能根据网络
 情况逐步增加每次发送的数据量，以避免上述现象的发生。

 具体来说，当新建连接时，cwnd初始化为1个最大报文段(MSS)大小，发送端开始按照拥塞窗口大小发送数据，
 每当有一个报文段被确认，cwnd就增加1个MSS大小。这样cwnd的值就随着网络往返时间(Round Trip Time,RTT)
 呈指数级增长，事实上，慢启动的速度一点也不慢，只是它的起点比较低一点而已。我们可以简单计算下：

 开始           --->     cwnd = 1

 经过1个RTT后   --->     cwnd = 2*1 = 2

 经过2个RTT后   --->     cwnd = 2*2= 4

 经过3个RTT后   --->     cwnd = 4*2 = 8

 如果带宽为W，那么经过RTT*log2W时间就可以占满带宽。

 拥塞避免：从慢启动可以看到，cwnd可以很快的增长上来，从而最大程度利用网络带宽资源，
 但是cwnd不能一直这样无限增长下去，一定需要某个限制。TCP使用了一个叫慢启动门限(ssthresh)的变量，
 当cwnd超过该值后，慢启动过程结束，进入拥塞避免阶段。对于大多数TCP实现来说，
 ssthresh的值是65536(同样以字节计算)。拥塞避免的主要思想是加法增大，也就是cwnd的值不再
 指数级往上升，开始加法增加。此时当窗口中所有的报文段都被确认时，cwnd的大小加1，cwnd的
 值就随着RTT开始线性增加，这样就可以避免增长过快导致网络拥塞，慢慢的增加调整到网络的最佳值。

 上面讨论的两个机制都是没有检测到拥塞的情况下的行为，那么当发现拥塞了cwnd又该怎样去调整呢？

 首先来看TCP是如何确定网络进入了拥塞状态的，TCP认为网络拥塞的主要依据是它重传了一个报文段。
 上面提到过，TCP对每一个报文段都有一个定时器，称为重传定时器(RTO)，当RTO超时且还没有得到数据确认，
 那么TCP就会对该报文段进行重传，当发生超时时，那么出现拥塞的可能性就很大，某个报文段可能在网络中
 某处丢失，并且后续的报文段也没有了消息，在这种情况下，TCP反应比较“强烈”：

 1.把ssthresh降低为cwnd值的一半

 2.把cwnd重新设置为1

 3.重新进入慢启动过程。

 从整体上来讲，TCP拥塞控制窗口变化的原则是AIMD原则，即加法增大、乘法减小。可以看出TCP的该原则
 可以较好地保证流之间的公平性，因为一旦出现丢包，那么立即减半退避，可以给其他新建的流留有足够
 的空间，从而保证整个的公平性。

 其实TCP还有一种情况会进行重传：那就是收到3个相同的ACK。TCP在收到乱序到达包时就会立即发送ACK，
 TCP利用3个相同的ACK来判定数据包的丢失，此时进行快速重传，快速重传做的事情有：

 1.把ssthresh设置为cwnd的一半

 2.把cwnd再设置为ssthresh的值(具体实现有些为ssthresh+3)

 3.重新进入拥塞避免阶段。

 后来的“快速恢复”算法是在上述的“快速重传”算法后添加的，当收到3个重复ACK时，TCP最后进入的不是
 拥塞避免阶段，而是快速恢复阶段。快速重传和快速恢复算法一般同时使用。快速恢复的思想是“数据包
 守恒”原则，即同一个时刻在网络中的数据包数量是恒定的，只有当“老”数据包离开了网络后，才能向网
 络中发送一个“新”的数据包，如果发送方收到一个重复的ACK，那么根据TCP的ACK机制就表明有一个数据
 包离开了网络，于是cwnd加1。如果能够严格按照该原则那么网络中很少会发生拥塞，事实上拥塞控制的
 目的也就在修正违反该原则的地方。

 具体来说快速恢复的主要步骤是：

 1.当收到3个重复ACK时，把ssthresh设置为cwnd的一半，把cwnd设置为ssthresh的值加3，然后重传丢
 失的报文段，加3的原因是因为收到3个重复的ACK，表明有3个“老”的数据包离开了网络。

 2.再收到重复的ACK时，拥塞窗口增加1。

 3.当收到新的数据包的ACK时，把cwnd设置为第一步中的ssthresh的值。原因是因为该ACK确认了新的
 数据，说明从重复ACK时的数据都已收到，该恢复过程已经结束，可以回到恢复之前的状态了，也即
 再次进入拥塞避免状态。

 快速重传算法首次出现在4.3BSD的Tahoe版本，快速恢复首次出现在4.3BSD的Reno版本，也称之为
 Reno版的TCP拥塞控制算法。

 可以看出Reno的快速重传算法是针对一个包的重传情况的，然而在实际中，一个重传超时可能导
 致许多的数据包的重传，因此当多个数据包从一个数据窗口中丢失时并且触发快速重传和快速恢
 复算法时，问题就产生了。因此NewReno出现了，它在Reno快速恢复的基础上稍加了修改，可以恢
 复一个窗口内多个包丢失的情况。具体来讲就是：Reno在收到一个新的数据的ACK时就退出了快速
 恢复状态了，而NewReno需要收到该窗口内所有数据包的确认后才会退出快速恢复状态，从而更一
 步提高吞吐量。

 SACK就是改变TCP的确认机制，最初的TCP只确认当前已连续收到的数据，SACK则把乱序等信息会
 全部告诉对方，从而减少数据发送方重传的盲目性。比如说序号1，2，3，5，7的数据收到了，
 那么普通的ACK只会确认序列号4，而SACK会把当前的5，7已经收到的信息在SACK选项里面告知对端，
 从而提高性能，当使用SACK的时候，NewReno算法可以不使用，因为SACK本身携带的信息就可以使得
 发送方有足够的信息来知道需要重传哪些包，而不需要重传哪些包。*/
void LINUXCC::onACK(int32_t ack)
{
    //printf("onACK\n");
    int in_flight = 0;      //正在传输的数据
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

    updateudt();
    //write back sock variables to UDT variables
    //  printf("Debugsock: snd_cwnd %d snd_sstresh %d snd_nxt %d\n",sk.snd_cwnd,sk.snd_ssthresh,sk.snd_nxt);
    //m_dCWndSize = 50;
}

void LINUXCC::updateudt()
{
    m_dCWndSize = sk.snd_cwnd;
    m_issthresh = sk.snd_ssthresh;
}

void LINUXCC::onTimeout()
{
    printf("onTimeout()\n");
    m_issthresh = getPerfInfo()->pktFlightSize / 2;
    if (m_issthresh < 2)
        m_issthresh = 2;

    m_bSlowStart = true;
    m_dCWndSize = 2.0;
}

void LINUXCC::updatesock()
{
    sk.snd_cwnd = (int) m_dCWndSize;
    //sock.snd_cwnd_cnt = (int)
    sk.snd_ssthresh = (int) m_issthresh;
    sk.snd_nxt = m_iSndCurrSeqNo + m_iMSS;
}

void LINUXCC::onLoss(const int32_t *, int)
{
}

void LINUXCC::statemachine(int32_t)
{
}
