/*****************************************************************************
Copyright (c) 2001 - 2011, Central South University.
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
   Yi Huang, last updated 24/11/2014
*****************************************************************************/
#ifdef __cplusplus
extern "C"{
#endif
#include "tcpabstract.h"

/* some useful functions here*/

/*
static inline struct inet_connection_sock *inet_csk(const struct sock *sk)
{
    return (struct inet_connection_sock *)sk;
}

static inline void *inet_csk_ca(const struct sock *sk)
{
    return (void *)inet_csk(sk)->icsk_ca_priv;
}
*/


u32 tcp_reno_min_cwnd(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	return tp->snd_ssthresh/2;
}

u32 tcp_reno_ssthresh(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	return max(tp->snd_cwnd >> 1U, 2U);
}
u32 tcp_current_ssthresh(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	return max(tp->snd_ssthresh, 2U);
}

 
//tcp_is_cwnd_limited from kernel source ignoring GSO part
/* 返回0，不需要增加cwnd ; 返回1，cwnd被限制，需要增加 */  
int tcp_is_cwnd_limited(const struct sock *sk, u32 in_flight)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	u32 left;
	if (in_flight >= tp->snd_cwnd)
		return 1;
	return 0;
}

//tcp_slow_start from kernel source ignoring sysctl_tcp_abc
void tcp_slow_start(struct tcp_sock *tp)
{
	//printf("\t进入慢启动,snd_cwnd=%d, snd_ssthresh=%d\n",tp->snd_cwnd,tp->snd_ssthresh);
	int cnt; 					
	cnt = tp->snd_cwnd;			// 指数增长,exponential increase 
	tp->snd_cwnd_cnt += cnt;
	while (tp->snd_cwnd_cnt >= tp->snd_cwnd) {
		tp->snd_cwnd_cnt -= tp->snd_cwnd;
		if (tp->snd_cwnd < tp->snd_cwnd_clamp)
			tp->snd_cwnd++;
	}
}

//tcp_cong_avoid_ai from kernel source without changes
void tcp_cong_avoid_ai(struct tcp_sock *tp, u32 w)
{
	if (tp->snd_cwnd_cnt >= w) {
		if (tp->snd_cwnd < tp->snd_cwnd_clamp)
			tp->snd_cwnd++;
		tp->snd_cwnd_cnt = 0;
	} else {
		tp->snd_cwnd_cnt++;
	}
}

//tcp_reno_cong_avoid from kernel source
void tcp_reno_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (!tcp_is_cwnd_limited(sk, in_flight))
		return;

	/* In "safe" area, increase. */
	if (tp->snd_cwnd <= tp->snd_ssthresh)
		tcp_slow_start(tp);
	else
		tcp_cong_avoid_ai(tp, tp->snd_cwnd);	
}

unsigned int tcp_left_out(const struct tcp_sock *tp)
{
    return tp->sacked_out + tp->lost_out;
}

/* This determines how many packets are "in the network" to the best
 * of our knowledge.  In many cases it is conservative, but where
 * detailed information is available from the receiver (via SACK
 * blocks etc.) we can make more aggressive calculations.
 *
 * Use this for decisions involving congestion control, use just
 * tp->packets_out to determine if the send queue is empty or not.
 *
 * Read this equation as:
 *
 *  "Packets sent once on transmission queue" MINUS
 *  "Packets left network, but not honestly ACKed yet" PLUS
 *  "Packets fast retransmitted"
 */
unsigned int tcp_packets_in_flight(const struct tcp_sock *tp)
{
    return tp->packets_out - tcp_left_out(tp) + tp->retrans_out;
}

void tcp_update_wl(struct tcp_sock *tp, u32 seq)
{
    tp->snd_wl1 = seq;
}



static pthread_mutex_t mutex_set_state = PTHREAD_MUTEX_INITIALIZER;
static inline void set_state(struct sock *sk, const u8 ca_state)
{
    pthread_mutex_lock(&mutex_set_state);
    sk->icsk_ca_state = ca_state;
    pthread_mutex_unlock(&mutex_set_state);
}

/**
 * 设置拥塞控制状态
 */
void tcp_set_ca_state(struct sock *sk, const u8 ca_state)
{
    //printf("进入tcp_set_ca_state\n");
    struct inet_connection_sock *icsk = inet_csk(sk);

    if (sk->icsk_ca_ops->set_state)
       sk->icsk_ca_ops->set_state(sk, ca_state);
    sk->icsk_ca_state = ca_state;
    //printf("出tcp_set_ca_state\n");
}

static void tcp_clear_retrans_partial(struct tcp_sock *tp)
{
    tp->retrans_out = 0;
    tp->lost_out = 0;

    //tp->undo_marker = 0;
    tp->undo_retrans = 0;
}

void tcp_clear_retrans(struct tcp_sock *tp)
{
    tcp_clear_retrans_partial(tp);

    tp->fackets_out = 0;
    tp->sacked_out = 0;
}

/*忽略了skb和sack*/
static int tcp_any_retrans_done(struct sock *sk)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct sk_buff *skb;

    if (tp->retrans_out)
        return 1;

    //skb = tcp_write_queue_head(sk);
    //if (unlikely(skb && TCP_SKB_CB(skb)->sacked & TCPCB_EVER_RETRANS))
        return 1;

    return 0;
}

void tcp_try_keep_open(struct sock *sk)
{
    struct tcp_sock *tp = tcp_sk(sk);
    int state = TCP_CA_Open;

    // 如果发生了左边发送成功/重传完成或者undo_marker被标记则进入TCP_CA_Disorder状态
    if (tcp_left_out(tp) || tcp_any_retrans_done(sk) /*|| tp->undo_marker*/)
        state = TCP_CA_Disorder;

    if (inet_csk(sk)->icsk_ca_state != state) {
        tcp_set_ca_state(sk, state);
        tp->high_seq = tp->snd_nxt;
    }
}

#ifdef __cplusplus
}
#endif
/* some useful functions here*/
