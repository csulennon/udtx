/*
 * tcp_cubic.h
 *
 *  Created on: 2015年4月15日
 *      Author: lennon
 */

#ifndef SRC_TCP_CUBIC_H_
#define SRC_TCP_CUBIC_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tcpabstract.h"



#define cpu_has_mips32r1    0
#define cpu_has_mips32r2    0
#define cpu_has_mips64r1    0
#define cpu_has_mips64r2    0
#define LONG_MAX 0xffffffff
#define MAX_JIFFY_OFFSET ((LONG_MAX >> 1)-1)
#define cpu_has_mips_r  (cpu_has_mips32r1 | cpu_has_mips32r2 | \
             cpu_has_mips64r1 | cpu_has_mips64r2)

# ifndef cpu_has_clo_clz
# define cpu_has_clo_clz    cpu_has_mips_r
# endif

/**
 * div64_u64 - unsigned 64bit divide with 64bit divisor
 */
static inline u64 div64_u64(u64 dividend, u64 divisor)
{
    return dividend / divisor;
}


inline int fls(int x)
{
    int r;

    if (__builtin_constant_p(cpu_has_clo_clz) && cpu_has_clo_clz) {
        __asm__("clz %0, %1" : "=r" (x) : "r" (x));

        return 32 - x;
    }

    r = 32;
    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

/**
 * fls64 - find last bit set in a 64-bit value
 * @n: the value to search
 *
 * This is defined the same way as ffs:
 * - return 64..1 to indicate bit 63..0 most significant bit set
 * - return 0 to indicate no bits set
 */
inline int fls64(__u64 x)
{
    __u32 h = x >> 32;
    if (h)
        return fls(h) + 32;
    return fls(x);
}

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

/* 缩放因子,max_cwnd = snd_cwnd * beta;
 * Scale factor beta calculation
 * max_cwnd = snd_cwnd * beta
 */
#define BICTCP_BETA_SCALE   1024
#define BICTCP_HZ           10  /* BIC HZ 2^10 = 1024 */

/* Two methods of hybrid slow start */
#define HYSTART_ACK_TRAIN   0x1
#define HYSTART_DELAY       0x2

#define clamp(val, min, max) ({         \
    typeof(val) __val = (val);      \
    typeof(min) __min = (min);      \
    typeof(max) __max = (max);      \
    (void) (&__val == &__min);      \
    (void) (&__val == &__max);      \
    __val = __val < __min ? __min: __val;   \
    __val > __max ? __max: __val; })



/* Number of delay samples for detecting the increase of delay */
#define HYSTART_MIN_SAMPLES 8
#define HYSTART_DELAY_MIN   (2U<<3)
#define HYSTART_DELAY_MAX   (16U<<3)
#define HYSTART_DELAY_THRESH(x) (clamp(x, HYSTART_DELAY_MIN, HYSTART_DELAY_MAX))


/* BIC TCP Parameters */
struct bictcp {
    u32 cnt;        /* 窗口增加计数器,收到一个ack增加1,用于控制snd_cwnd增长速度.increase cwnd by 1 after ACKs */
    u32     last_max_cwnd;  /* last maximum snd_cwnd */
    u32 loss_cwnd;  /* congestion window at last loss */
    u32 last_cwnd;  /* the last snd_cwnd */
    u32 last_time;  /* time when updated last_cwnd */
    u32 bic_origin_point;/* 即新的Wmax，取Wlast_max和snd_cwnd大者,origin point of bic function */
    u32 bic_K;      /* 即新Wmax所对应的时间点t，W(bic_K) = Wmax ;time to origin point from the beginning of the current epoch */
    u32 delay_min;  /* 应该是最小RTT;min delay */
    u32 epoch_start;    /* beginning of an epoch */
    u32 ack_cnt;    /* number of acks */
    u32 tcp_cwnd;   /* 按照Reno算法计算得的cwnd;estimated tcp cwnd */
#define ACK_RATIO_SHIFT 4
    u16 delayed_ack;    /* estimate the ratio of Packets/ACKs << 4 */
    u8  sample_cnt; /* 第几个sample;number of samples to decide curr_rtt */
    u8  found;      /* the exit point is found? */
    u32 round_start;    /* beginning of each round,针对每个RTT*/
    u32 end_seq;    /* end_seq of the round,用来标识每个RTT */
    u32 last_jiffies;   /* 超过2ms则不认为是连续的;last time when the ACK spacing is close */
    u32 curr_rtt;   /* 由sample中最小的决定;the minimum rtt of current round */
};


#ifdef __cplusplus
}
#endif

#endif /* SRC_TCP_CUBIC_H_ */
