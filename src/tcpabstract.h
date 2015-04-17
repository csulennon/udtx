/*****************************************************************************
 Copyright (c) 2001 - 2009, Central South University.
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
 Yi Huang, last updated 11/23/2014
 *****************************************************************************/
#ifndef __UDT_LINUXWRAP_H__
#define __UDT_LINUXWRAP_H__ 1

/************************ DEFINE WRAP *****************************/
//keyword re-define
#define __u64 unsigned long long
#define __u32 unsigned long
#define __u16 unsigned int
#define __u8 unsigned char

#define u64 __u64
#define u32 __u32
#define u16 __u16
#define u8 __u8

#define s32 long
#define s64 long long
#define module_param(a,b,c)
#define MODULE_PARM_DESC(a,b)
#define module_put(x)
#define try_module_get(x) 0
#define MODULE_AUTHOR(x)
#define MODULE_LICENSE(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_VERSION(a)

#define KERN_ERR "<2> "
#define KERN_NOTICE "<1> "
#define KERN_INFO "<0> "

#define printk(fmt, args...) { \
	if (strlen(fmt)<4 || fmt[0]!='<' || fmt[2]!='>' || fmt[3]!=' ') \
		fprintf(stderr, "<Unspecified>" fmt, args);\
	else if (fmt[1]>=debug_level+'0')\
		printf(fmt, args);\
    }\

#define tcp_register_congestion_control(A)
#define tcp_unregister_congestion_control(B)

#define __init
#define __exit
#define BUG_ON(x) 
#define BUILD_BUG_ON(x)
#define WARN_ON(x)

#define EXPORT_SYMBOL_GPL(hello)
#define ktime_t s64
#define nla_put(A,B,C,D)
#define tcp_sock sock
#define TCP_CA_NAME_MAX	16
#define TCP_CA_MAX	128
#define TCP_CA_BUF_MAX	(TCP_CA_NAME_MAX*TCP_CA_MAX)

#define TCP_CONG_NON_RESTRICTED 0x1
#define TCP_CONG_RTT_STAMP	0x2

#define THIS_MODULE 0
#define THIS_LIST 0

//shortcut re-define
#define min(x,y) (((x)<(y))?(x):(y))
#define max(x,y) (((x)>(y))?(x):(y))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define JIFFY_RATIO 1000
#define US_RATIO 1000000
#define MS_RATIO 1000

#define jiffies_to_usecs(x) ((US_RATIO/JIFFY_RATIO)*(x))
#define msecs_to_jiffies(x) ((JIFFY_RATIO/MS_RATIO)*(x))
#define usecs_to_jiffies(x) ((JIFFY_RATIO/US_RATIO)*(x))


#define min_t(type,x,y) \
	(((type)(x)) < ((type)(y)) ? ((type)(x)): ((type)(y)))

#define max_t(type,x,y) \
	(((type)(x)) > ((type)(y)) ? ((type)(x)): ((type)(y)))

#define after(seq1,seq2) ((seq2)<(seq1))
#define before(seq1,seq2) after(seq2,seq1)	
#define inet_csk(sk) (sk)
#define tcp_sk(sk) (sk)
#define inet_csk_ca(sk) ((struct vegas*)(sk)->icsk_ca_priv)
#define inet_csk_ca_cubic(sk) ((struct bictcp*)(sk)->icsk_ca_priv)



//TCP state machine
enum tcp_ca_state
{
    TCP_CA_Open = 0,
#define TCPF_CA_Open	(1<<TCP_CA_Open)
    TCP_CA_Disorder = 1,
#define TCPF_CA_Disorder (1<<TCP_CA_Disorder)
    TCP_CA_CWR = 2,
#define TCPF_CA_CWR	(1<<TCP_CA_CWR)
    TCP_CA_Recovery = 3,
#define TCPF_CA_Recovery (1<<TCP_CA_Recovery)
    TCP_CA_Loss = 4
#define TCPF_CA_Loss	(1<<TCP_CA_Loss)
};

#define FLAG_ECE 1
#define FLAG_DATA_SACKED 2
#define FLAG_DATA_ACKED 4
#define FLAG_DATA_LOST 8
#define FLAG_CA_ALERT           (FLAG_DATA_SACKED | FLAG_ECE)
#define FLAG_NOT_DUP		(FLAG_DATA_ACKED)

#define FLAG_UNSURE_TSTAMP 16

#define CONFIG_DEFAULT_TCP_CONG "reno"
#define TCP_CLOSE 0

enum tcp_ca_event
{
    CA_EVENT_TX_START, /* first transmit when no packets in flight */
    CA_EVENT_CWND_RESTART, /* congestion window restart */
    CA_EVENT_COMPLETE_CWR, /* end of congestion recovery */
    CA_EVENT_FRTO, /* fast recovery timeout */
    CA_EVENT_LOSS, /* loss timeout */
    CA_EVENT_FAST_ACK, /* in sequence ack */
    CA_EVENT_SLOW_ACK, /* other ack */
};

/************************ DEFINE WRAP *****************************/

/************************ struct WRAP *****************************/

//INET_DIAG
struct tcpvegas_info
{
    __u32 tcpv_enabled;
    __u32 tcpv_rttcnt;
    __u32 tcpv_rtt;
    __u32 tcpv_minrtt;
};

//Empty ``list_head" struct to cheat kernel source
struct list_head
{
    char empty;
};

//Empty ``modul" struct to cheat kernel source
struct module
{
    char empty;
};

extern struct list_head empty_list_head;
extern struct module empty_module;

enum
{
    INET_DIAG_NONE, INET_DIAG_MEMINFO, INET_DIAG_INFO, INET_DIAG_VEGASINFO, INET_DIAG_CONG,
};

//Re-contruct ``sock" struct in user-space
//now only some of variables in sock and tcpsock structs are implemented in our design
struct sock
{
    u32 snd_nxt;        /* 发送的下一个序列;Next sequence we send*/
    u32 snd_una;        /* 希望ack的第一个字节,即左边缘;First byte we want an ack for*/
    u32 snd_wnd;        /* 希望收到的窗口大小;The window we expect to receive*/
    u32 srtt;           /* 平滑的rtt值 << 3;smoothed round trip time << 3	*/
    u32 snd_ssthresh;   /* 慢启动门限;Slow start size threshold*/
    u32 snd_cwnd;       /* 发送的拥塞窗口;Sending congestion window*/
    u32 snd_cwnd_cnt;   /* 线性增加计数器;Linear increase counter*/
    u32 snd_cwnd_clamp; /* 最大窗口大小;Max cwnd size*/
    u32 icsk_ca_priv[16]; /* 协议特殊结构;Protocol specific struct*/
    u8  icsk_ca_state;
};





struct sk_buff
{
};
struct tcp_congestion_ops
{
    struct list_head list;
    unsigned long flags;

    /* initialize private data (optional) */
    void (*init)(struct sock *sk);

    /* cleanup private data  (optional) */
    void (*release)(struct sock *sk);

    /* return slow start threshold (required) */
    u32 (*ssthresh)(struct sock *sk);

    /* lower bound for congestion window (optional) */
    u32 (*min_cwnd)(const struct sock *sk);

    /* do new cwnd calculation (required) */
    void (*cong_avoid)(struct sock *sk, u32 ack, u32 in_flight);

    /* call before changing ca_state (optional) */
    void (*set_state)(struct sock *sk, u8 new_state);

    /* call when cwnd event occurs (optional) */
    void (*cwnd_event)(struct sock *sk, enum tcp_ca_event ev);

    /* new value of cwnd after loss (optional) */
    u32 (*undo_cwnd)(struct sock *sk);

    /* hook for packet ack accounting (optional) */
    void (*pkts_acked)(struct sock *sk, u32 num_acked, s32 rtt_us);

    /* get info for inet_diag (optional) */
    void (*get_info)(struct sock *sk, u32 ext, struct sk_buff *skb);

    char name[TCP_CA_NAME_MAX];
    struct module *owner;
};
extern struct tcp_congestion_ops tcp_vegasx;
extern struct tcp_congestion_ops tcp_cubic;











/************************ struct WRAP *****************************/

/************************ Function WRAP ***************************/
u32 tcp_reno_min_cwnd(const struct sock *sk);
u32 tcp_reno_ssthresh(struct sock *sk);
u32 tcp_current_ssthresh(struct sock *sk);
int tcp_is_cwnd_limited(const struct sock *sk, u32 in_flight);
void tcp_slow_start(struct tcp_sock *tp);
void tcp_cong_avoid_ai(struct tcp_sock *tp, u32 w);
void tcp_reno_cong_avoid(struct sock *sk, u32 ack, u32 in_flight);
/************************ function WRAP ***************************/

#endif

