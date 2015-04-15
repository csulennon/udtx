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
int tcp_is_cwnd_limited(const struct sock *sk, u32 in_flight)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	u32 left;

	if (in_flight >= tp->snd_cwnd)
		return 1;
}

//tcp_slow_start from kernel source ignoring sysctl_tcp_abc
void tcp_slow_start(struct tcp_sock *tp)
{
	//printf("tcp_slow_start");
	int cnt; 					/* increase in packets */
	cnt = tp->snd_cwnd;			/* exponential increase */
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

/* some useful functions here*/
