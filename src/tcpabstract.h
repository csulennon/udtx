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


#ifdef XXXX
typedef struct
 {
 volatile int counter;
 }
 atomic_t;

 struct __wait_queue_head {
         spinlock_t lock;
         struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;

typedef struct spinlock {
    union {
        struct raw_spinlock rlock;

#ifdef CONFIG_DEBUG_LOCK_ALLOC
# define LOCK_PADSIZE (offsetof(struct raw_spinlock, dep_map))
        struct {
            u8 __padding[LOCK_PADSIZE];
            struct lockdep_map dep_map;
        };
#endif
    };
} spinlock_t;



 /* This is the per-socket lock.  The spinlock provides a synchronization
  * between user contexts and software interrupt processing, whereas the
  * mini-semaphore synchronizes multiple users amongst themselves.
  */
 typedef struct {
     spinlock_t      slock;
     int         owned;
     wait_queue_head_t   wq;
     /*
      * We express the mutex-alike socket_lock semantics
      * to the lock validator by explicitly managing
      * the slock as a lock variant (in addition to
      * the slock itself):
      */
 #ifdef CONFIG_DEBUG_LOCK_ALLOC
     struct lockdep_map dep_map;
 #endif
 } socket_lock_t;

/**
  * struct sock - network layer representation of sockets
  * @__sk_common: shared layout with inet_timewait_sock
  * @sk_shutdown: mask of %SEND_SHUTDOWN and/or %RCV_SHUTDOWN
  * @sk_userlocks: %SO_SNDBUF and %SO_RCVBUF settings
  * @sk_lock:   synchronizer
  * @sk_rcvbuf: size of receive buffer in bytes
  * @sk_wq: sock wait queue and async head
  * @sk_dst_cache: destination cache
  * @sk_dst_lock: destination cache lock
  * @sk_policy: flow policy
  * @sk_rmem_alloc: receive queue bytes committed
  * @sk_receive_queue: incoming packets
  * @sk_wmem_alloc: transmit queue bytes committed
  * @sk_write_queue: Packet sending queue
  * @sk_async_wait_queue: DMA copied packets
  * @sk_omem_alloc: "o" is "option" or "other"
  * @sk_wmem_queued: persistent queue size
  * @sk_forward_alloc: space allocated forward
  * @sk_allocation: allocation mode
  * @sk_sndbuf: size of send buffer in bytes
  * @sk_flags: %SO_LINGER (l_onoff), %SO_BROADCAST, %SO_KEEPALIVE,
  *        %SO_OOBINLINE settings, %SO_TIMESTAMPING settings
  * @sk_no_check: %SO_NO_CHECK setting, wether or not checkup packets
  * @sk_route_caps: route capabilities (e.g. %NETIF_F_TSO)
  * @sk_route_nocaps: forbidden route capabilities (e.g NETIF_F_GSO_MASK)
  * @sk_gso_type: GSO type (e.g. %SKB_GSO_TCPV4)
  * @sk_gso_max_size: Maximum GSO segment size to build
  * @sk_lingertime: %SO_LINGER l_linger setting
  * @sk_backlog: always used with the per-socket spinlock held
  * @sk_callback_lock: used with the callbacks in the end of this struct
  * @sk_error_queue: rarely used
  * @sk_prot_creator: sk_prot of original sock creator (see ipv6_setsockopt,
  *           IPV6_ADDRFORM for instance)
  * @sk_err: last error
  * @sk_err_soft: errors that don't cause failure but are the cause of a
  *           persistent failure not just 'timed out'
  * @sk_drops: raw/udp drops counter
  * @sk_ack_backlog: current listen backlog
  * @sk_max_ack_backlog: listen backlog set in listen()
  * @sk_priority: %SO_PRIORITY setting
  * @sk_type: socket type (%SOCK_STREAM, etc)
  * @sk_protocol: which protocol this socket belongs in this network family
  * @sk_peer_pid: &struct pid for this socket's peer
  * @sk_peer_cred: %SO_PEERCRED setting
  * @sk_rcvlowat: %SO_RCVLOWAT setting
  * @sk_rcvtimeo: %SO_RCVTIMEO setting
  * @sk_sndtimeo: %SO_SNDTIMEO setting
  * @sk_rxhash: flow hash received from netif layer
  * @sk_filter: socket filtering instructions
  * @sk_protinfo: private area, net family specific, when not using slab
  * @sk_timer: sock cleanup timer
  * @sk_stamp: time stamp of last packet received
  * @sk_socket: Identd and reporting IO signals
  * @sk_user_data: RPC layer private data
  * @sk_sndmsg_page: cached page for sendmsg
  * @sk_sndmsg_off: cached offset for sendmsg
  * @sk_send_head: front of stuff to transmit
  * @sk_security: used by security modules
  * @sk_mark: generic packet mark
  * @sk_classid: this socket's cgroup classid
  * @sk_write_pending: a write to stream socket waits to start
  * @sk_state_change: callback to indicate change in the state of the sock
  * @sk_data_ready: callback to indicate there is data to be processed
  * @sk_write_space: callback to indicate there is bf sending space available
  * @sk_error_report: callback to indicate errors (e.g. %MSG_ERRQUEUE)
  * @sk_backlog_rcv: callback to process the backlog
  * @sk_destruct: called at sock freeing time, i.e. when all refcnt == 0
 */
struct sock {
    /*
     * Now struct inet_timewait_sock also uses sock_common, so please just
     * don't add nothing before this first member (__sk_common) --acme
     */
    struct sock_common  __sk_common;
#define sk_node         __sk_common.skc_node
#define sk_nulls_node       __sk_common.skc_nulls_node
#define sk_refcnt       __sk_common.skc_refcnt
#define sk_tx_queue_mapping __sk_common.skc_tx_queue_mapping

#define sk_dontcopy_begin   __sk_common.skc_dontcopy_begin
#define sk_dontcopy_end     __sk_common.skc_dontcopy_end
#define sk_hash         __sk_common.skc_hash
#define sk_family       __sk_common.skc_family
#define sk_state        __sk_common.skc_state
#define sk_reuse        __sk_common.skc_reuse
#define sk_bound_dev_if     __sk_common.skc_bound_dev_if
#define sk_bind_node        __sk_common.skc_bind_node
#define sk_prot         __sk_common.skc_prot
#define sk_net          __sk_common.skc_net
    socket_lock_t       sk_lock;
    struct sk_buff_head sk_receive_queue;
    /*
     * The backlog queue is special, it is always used with
     * the per-socket spinlock held and requires low latency
     * access. Therefore we special case it's implementation.
     * Note : rmem_alloc is in this structure to fill a hole
     * on 64bit arches, not because its logically part of
     * backlog.
     */
    struct {
        atomic_t    rmem_alloc;
        int     len;
        struct sk_buff  *head;
        struct sk_buff  *tail;
    } sk_backlog;
#define sk_rmem_alloc sk_backlog.rmem_alloc
    int         sk_forward_alloc;
#ifdef CONFIG_RPS
    __u32           sk_rxhash;
#endif
    atomic_t        sk_drops;
    int         sk_rcvbuf;

    struct sk_filter __rcu  *sk_filter;
    struct socket_wq    *sk_wq;

#ifdef CONFIG_NET_DMA
    struct sk_buff_head sk_async_wait_queue;
#endif

#ifdef CONFIG_XFRM
    struct xfrm_policy  *sk_policy[2];
#endif
    unsigned long       sk_flags;
    struct dst_entry    *sk_dst_cache;
    spinlock_t      sk_dst_lock;
    atomic_t        sk_wmem_alloc;
    atomic_t        sk_omem_alloc;
    int         sk_sndbuf;
    struct sk_buff_head sk_write_queue;
    kmemcheck_bitfield_begin(flags);
    unsigned int        sk_shutdown  : 2,
                sk_no_check  : 2,
                sk_userlocks : 4,
                sk_protocol  : 8,
                sk_type      : 16;
    kmemcheck_bitfield_end(flags);
    int         sk_wmem_queued;
    gfp_t           sk_allocation;
    int         sk_route_caps;
    int         sk_route_nocaps;
    int         sk_gso_type;
    unsigned int        sk_gso_max_size;
    int         sk_rcvlowat;
    unsigned long           sk_lingertime;
    struct sk_buff_head sk_error_queue;
    struct proto        *sk_prot_creator;
    rwlock_t        sk_callback_lock;
    int         sk_err,
                sk_err_soft;
    unsigned short      sk_ack_backlog;
    unsigned short      sk_max_ack_backlog;
    __u32           sk_priority;
    struct pid      *sk_peer_pid;
    const struct cred   *sk_peer_cred;
    long            sk_rcvtimeo;
    long            sk_sndtimeo;
    void            *sk_protinfo;
    struct timer_list   sk_timer;
    ktime_t         sk_stamp;
    struct socket       *sk_socket;
    void            *sk_user_data;
    struct page     *sk_sndmsg_page;
    struct sk_buff      *sk_send_head;
    __u32           sk_sndmsg_off;
    int         sk_write_pending;
#ifdef CONFIG_SECURITY
    void            *sk_security;
#endif
    __u32           sk_mark;
    u32         sk_classid;
    void            (*sk_state_change)(struct sock *sk);
    void            (*sk_data_ready)(struct sock *sk, int bytes);
    void            (*sk_write_space)(struct sock *sk);
    void            (*sk_error_report)(struct sock *sk);
    int         (*sk_backlog_rcv)(struct sock *sk,
                          struct sk_buff *skb);
    void                    (*sk_destruct)(struct sock *sk);
};


#endif





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

