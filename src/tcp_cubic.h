/*
 * tcp_cubic.h
 *
 *  Created on: 2015年4月15日
 *      Author: lennon
 */

#ifndef SRC_TCP_CUBIC_H_
#define SRC_TCP_CUBIC_H_
#include "tcpabstract.h"


#define __jiffy_data  __attribute__((section(".data")))
#define __read_mostly __attribute__((__section__(".data..read_mostly")))
#define tcp_time_stamp      ((__u32)(jiffies))

#ifndef HZ
#define HZ 100
#endif

struct timeval
{
    long tv_sec;         /* seconds */
    long tv_usec;        /* microseconds */
};

static inline u32 raid6_jiffies(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}
# define jiffies    raid6_jiffies()

#define BICTCP_BETA_SCALE    1024   /* Scale factor beta calculation
                     * max_cwnd = snd_cwnd * beta
                     */
#define BICTCP_HZ       10  /* BIC HZ 2^10 = 1024 */

/* Two methods of hybrid slow start */
#define HYSTART_ACK_TRAIN   0x1
#define HYSTART_DELAY       0x2

/* Number of delay samples for detecting the increase of delay */
#define HYSTART_MIN_SAMPLES 8
#define HYSTART_DELAY_MIN   (2U<<3)
#define HYSTART_DELAY_MAX   (16U<<3)
#define HYSTART_DELAY_THRESH(x) clamp(x, HYSTART_DELAY_MIN, HYSTART_DELAY_MAX)


/* BIC TCP Parameters */
struct bictcp {
    u32 cnt;        /* increase cwnd by 1 after ACKs */
    u32     last_max_cwnd;  /* last maximum snd_cwnd */
    u32 loss_cwnd;  /* congestion window at last loss */
    u32 last_cwnd;  /* the last snd_cwnd */
    u32 last_time;  /* time when updated last_cwnd */
    u32 bic_origin_point;/* origin point of bic function */
    u32 bic_K;      /* time to origin point from the beginning of the current epoch */
    u32 delay_min;  /* min delay */
    u32 epoch_start;    /* beginning of an epoch */
    u32 ack_cnt;    /* number of acks */
    u32 tcp_cwnd;   /* estimated tcp cwnd */
#define ACK_RATIO_SHIFT 4
    u16 delayed_ack;    /* estimate the ratio of Packets/ACKs << 4 */
    u8  sample_cnt; /* number of samples to decide curr_rtt */
    u8  found;      /* the exit point is found? */
    u32 round_start;    /* beginning of each round */
    u32 end_seq;    /* end_seq of the round */
    u32 last_jiffies;   /* last time when the ACK spacing is close */
    u32 curr_rtt;   /* the minimum rtt of current round */
};

/*
 * Pointers to address related TCP functions
 * (i.e. things that depend on the address family)
 */
struct inet_connection_sock_af_ops;
struct inet_sock{} ;
struct request_sock_queue{};
struct timer_list{};

struct inet_connection_sock {
    /* inet_sock has to be the first member! */
    struct inet_sock      icsk_inet;
    struct request_sock_queue icsk_accept_queue;
    struct inet_bind_bucket   *icsk_bind_hash;
    unsigned long         icsk_timeout;
    struct timer_list     icsk_retransmit_timer;
    struct timer_list     icsk_delack_timer;
    __u32             icsk_rto;
    __u32             icsk_pmtu_cookie;
    const struct tcp_congestion_ops *icsk_ca_ops;
    const struct inet_connection_sock_af_ops *icsk_af_ops;
    unsigned int          (*icsk_sync_mss)(struct sock *sk, u32 pmtu);
    __u8              icsk_ca_state;
    __u8              icsk_retransmits;
    __u8              icsk_pending;
    __u8              icsk_backoff;
    __u8              icsk_syn_retries;
    __u8              icsk_probes_out;
    __u16             icsk_ext_hdr_len;
    struct {
        __u8          pending;   /* ACK is pending             */
        __u8          quick;     /* Scheduled number of quick acks     */
        __u8          pingpong;  /* The session is interactive         */
        __u8          blocked;   /* Delayed ACK was blocked by socket lock */
        __u32         ato;       /* Predicted tick of soft clock       */
        unsigned long     timeout;   /* Currently scheduled timeout        */
        __u32         lrcvtime;  /* timestamp of last received data packet */
        __u16         last_seg_size; /* Size of last incoming segment      */
        __u16         rcv_mss;   /* MSS used for delayed ACK decisions     */
    } icsk_ack;
    struct {
        int       enabled;

        /* Range of MTUs to search */
        int       search_high;
        int       search_low;

        /* Information on the current probe. */
        int       probe_size;
    } icsk_mtup;
    u32           icsk_ca_priv[16];
    u32           icsk_user_timeout;
#define ICSK_CA_PRIV_SIZE   (16 * sizeof(u32))
};





#endif /* SRC_TCP_CUBIC_H_ */
