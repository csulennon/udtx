/*
 * TCP CUBIC: Binary Increase Congestion control for TCP v2.3
 * Home page:
 *      http://netsrv.csc.ncsu.edu/twiki/bin/view/Main/BIC
 * This is from the implementation of CUBIC TCP in
 * Sangtae Ha, Injong Rhee and Lisong Xu,
 *  "CUBIC: A New TCP-Friendly High-Speed TCP Variant"
 *  in ACM SIGOPS Operating System Review, July 2008.
 * Available from:
 *  http://netsrv.csc.ncsu.edu/export/cubic_a_new_tcp_2008.pdf
 *
 * CUBIC integrates a new slow start algorithm, called HyStart.
 * The details of HyStart are presented in
 *  Sangtae Ha and Injong Rhee,
 *  "Taming the Elephants: New TCP Slow Start", NCSU TechReport 2008.
 * Available from:
 *  http://netsrv.csc.ncsu.edu/export/hystart_techreport_2008.pdf
 *
 * All testing results are available from:
 * http://netsrv.csc.ncsu.edu/wiki/index.php/TCP_Testing
 *
 * Unless CUBIC is enabled and congestion window is large
 * this behaves the same as the original Reno.
 */
/*
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <net/tcp.h>
*/
#include "tcp_cubic.h"
#include <stdio.h>


static int fast_convergence __read_mostly = 1;
static int beta __read_mostly = 717;	/* = 717/1024 (BICTCP_BETA_SCALE) */
static int initial_ssthresh __read_mostly;
static int bic_scale __read_mostly = 41;
static int tcp_friendliness __read_mostly = 1;/* 友好性 */

static int hystart __read_mostly = 1;
static int hystart_detect __read_mostly = HYSTART_ACK_TRAIN | HYSTART_DELAY;
static int hystart_low_window __read_mostly = 16;

static u32 cube_rtt_scale __read_mostly;
static u32 beta_scale __read_mostly;
static u64 cube_factor __read_mostly;

/* Note parameters that are used for precomputing scale factors are read-only */
module_param(fast_convergence, int, 0644);
MODULE_PARM_DESC(fast_convergence, "turn on/off fast convergence");
module_param(beta, int, 0644);
MODULE_PARM_DESC(beta, "beta for multiplicative increase");
module_param(initial_ssthresh, int, 0644);
MODULE_PARM_DESC(initial_ssthresh, "initial value of slow start threshold");
module_param(bic_scale, int, 0444);
MODULE_PARM_DESC(bic_scale, "scale (scaled by 1024) value for bic function (bic_scale/1024)");
module_param(tcp_friendliness, int, 0644);
MODULE_PARM_DESC(tcp_friendliness, "turn on/off tcp friendliness");
module_param(hystart, int, 0644);
MODULE_PARM_DESC(hystart, "turn on/off hybrid slow start algorithm");
module_param(hystart_detect, int, 0644);
MODULE_PARM_DESC(hystart_detect, "hyrbrid slow start detection mechanisms"
		 " 1: packet-train 2: delay 3: both packet-train and delay");
module_param(hystart_low_window, int, 0644);
MODULE_PARM_DESC(hystart_low_window, "lower bound cwnd for hybrid slow start");




static inline void bictcp_reset(struct bictcp *ca)
{
	ca->cnt = 0;
	ca->last_max_cwnd = 0;
	ca->loss_cwnd = 0;
	ca->last_cwnd = 0;
	ca->last_time = 0;
	ca->bic_origin_point = 0;
	ca->bic_K = 0;
	ca->delay_min = 0;
	ca->epoch_start = 0;
	ca->delayed_ack = 2 << ACK_RATIO_SHIFT;
	ca->ack_cnt = 0;
	ca->tcp_cwnd = 0;
	ca->found = 0;
}

extern unsigned long volatile __jiffy_data jiffies;

static inline void bictcp_hystart_reset(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca_cubic(sk);

	ca->round_start = ca->last_jiffies = jiffies;
	ca->end_seq = tp->snd_nxt;
	ca->curr_rtt = 0;
	ca->sample_cnt = 0;
}

static void bictcp_init(struct sock *sk)
{
    printf("调用cubic算法\n");
	bictcp_reset(inet_csk_ca_cubic(sk));

	if (hystart)
		bictcp_hystart_reset(sk);

	if (!hystart && initial_ssthresh)
		tcp_sk(sk)->snd_ssthresh = initial_ssthresh;
	printf("initial_ssthresh = %d\n", initial_ssthresh);
}

/* calculate the cubic root of x using a table lookup followed by one
 * Newton-Raphson iteration.
 * Avg err ~= 0.195%
 */
static u32 cubic_root(u64 a)
{
	u32 x, b, shift;
	/*
	 * cbrt(x) MSB values for x MSB values in [0..63].
	 * Precomputed then refined by hand - Willy Tarreau
	 *
	 * For x in [0..63],
	 *   v = cbrt(x << 18) - 1
	 *   cbrt(x) = (v[x] + 10) >> 6
	 */
	static const u8 v[] = {
		/* 0x00 */    0,   54,   54,   54,  118,  118,  118,  118,
		/* 0x08 */  123,  129,  134,  138,  143,  147,  151,  156,
		/* 0x10 */  157,  161,  164,  168,  170,  173,  176,  179,
		/* 0x18 */  181,  185,  187,  190,  192,  194,  197,  199,
		/* 0x20 */  200,  202,  204,  206,  209,  211,  213,  215,
		/* 0x28 */  217,  219,  221,  222,  224,  225,  227,  229,
		/* 0x30 */  231,  232,  234,  236,  237,  239,  240,  242,
		/* 0x38 */  244,  245,  246,  248,  250,  251,  252,  254,
	};

	b = fls64(a);
	if (b < 7) {
		/* a in [0..63] */
		return ((u32)v[(u32)a] + 35) >> 6;
	}

	b = ((b * 84) >> 8) - 1;
	shift = (a >> (b * 3));

	x = ((u32)(((u32)v[shift] + 10) << b)) >> 6;

	/*
	 * Newton-Raphson iteration
	 *                         2
	 * x    = ( 2 * x  +  a / x  ) / 3
	 *  k+1          k         k
	 */
	x = (2 * x + (u32)div64_u64(a, (u64)x * (u64)(x - 1)));
	x = ((x * 341) >> 10);
	return x;
}

/*
函数关键点

1.  我们最终要得到的是ca->cnt，用来控制snd_cwnd的增长。

2.  ca->cnt的值，是根据cwnd和w( t + after ) 的大小来判断的。
	w( t + after )即bic_target，它表示我们预期的

在经过after时间后的snd_cwnd。如果此时cwnd < w( t + after )，那么我们就快速增加窗口，达到预期目标。

如果cwnd > w( t + after )，那说明我们已经增加过快了，需要降速了，这样才能达到预期目标。

              cwnd / (bic_target - cwnd )   // bic_target > cwnd 

cnt =   100 * cwnd   // bic_target < cwnd 

3.  cwnd是传入的参数，已知。现在我们只需要计算bic_target。

而根据Cubic的窗口增长函数：W(t) = C(t - K)^3 + Wmax，

我们要计算时间( 当前 + after )，以及时间K。时间K即bic_K，表示函数值为Wmax所对应的时间。

通过代码可以发现，after为min RTT，即连接的传播时延。

4.  然后就是bic_K和t的计算了，详细可看代码。
 */
/*
 * 计算拥塞窗口,Compute congestion window to use.
 */
static inline void bictcp_update(struct bictcp *ca, u32 cwnd)
{
	u64 offs;/* 时间差，| t - K | */  

	/* delta是cwnd差，bic_target是预测值，t为预测时间 */ 
	u32 delta, t, bic_target, max_cnt;

	ca->ack_cnt++;	/* count the number of ACKs */

	 /* 31.25ms以内不更新ca！！！*/  
	if (ca->last_cwnd == cwnd &&
	    (s32)(tcp_time_stamp - ca->last_time) <= HZ / 32)
		return;

	ca->last_cwnd = cwnd;
	ca->last_time = tcp_time_stamp;

	/*丢包后 一个新的时段 */  
	if (ca->epoch_start == 0) {
		ca->epoch_start = tcp_time_stamp;	/* record the beginning of an epoch */
		ca->ack_cnt = 1;			/* start counting */
		ca->tcp_cwnd = cwnd;			/* syn with cubic */

		/* 取max(last_max_cwnd , cwnd)作为当前Wmax */  
		if (ca->last_max_cwnd <= cwnd) {
			ca->bic_K = 0;
			ca->bic_origin_point = cwnd;
		} else {
			/* Compute new K based on
			 * (wmax-cwnd) * (srtt>>3 / HZ) / c * 2^(3*bictcp_HZ)
			 * bic_K本来单位为秒，转成单位为 1 / 1024秒
			 */
			ca->bic_K = cubic_root(cube_factor
					       * (ca->last_max_cwnd - cwnd));
			ca->bic_origin_point = ca->last_max_cwnd;
		}
	}


	/* cubic function - calc*/
	/* calculate c * time^3 / rtt,
	 *  while considering overflow in calculation of time^3
	 * (so time^3 is done by using 64 bit)
	 * and without the support of division of 64bit numbers
	 * (so all divisions are done by using 32 bit)
	 *  also NOTE the unit of those veriables
	 *	  time  = (t - K) / 2^bictcp_HZ
	 *	  c = bic_scale >> 10
	 * Constant = c / srtt = 0.4, 实际参数为0.4 
	 * rtt  = (srtt >> 3) / HZ
	 * !!! The following code does not have overflow problems,
	 * if the cwnd < 1 million packets !!!
	 * 预测时间为：ca->delay_min >> 3后
	 */

	/* change the unit from HZ to bictcp_HZ */
	t = ((tcp_time_stamp + (ca->delay_min>>3) - ca->epoch_start)
	     << BICTCP_HZ) / HZ;

	 /* 求| t - bic_K | */  
	if (t < ca->bic_K)		/* t - K */
		offs = ca->bic_K - t;
	else
		offs = t - ca->bic_K;/* 此时已经超过Wmax */

	/* c/rtt * (t-K)^3 */
	/* 计算delta =| W(t) - W(bic_K) |  
     * cube_rtt_scale = (bic_scale * 10) = c / srtt * 2^10，c/srtt = 0.4 
     */ 
	delta = (cube_rtt_scale * offs * offs * offs) >> (10+3*BICTCP_HZ);
	if (t < ca->bic_K)                                	/* below origin*/
		bic_target = ca->bic_origin_point - delta;
	else                                                	/* above origin*/
		bic_target = ca->bic_origin_point + delta;

	/* cubic function - calc bictcp_cnt*/
	/* 计算bic_target，即预测cwnd */ 
	if (bic_target > cwnd) {
		/* 相差越多，增长越快，这就是函数形状由来 */  
		ca->cnt = cwnd / (bic_target - cwnd);
	} else {
		/* very small increment，目前cwnd已经超出预期了，应该降速 */
		ca->cnt = 100 * cwnd;              /* very small increment*/
	}
	/* TCP Friendly —如果bic比RENO慢，则提升cwnd增长速度，即减小cnt 
     * 以上次丢包以后的时间t算起，每次RTT增长 3B / ( 2 - B)，那么可以得到 
     * 采用RENO算法的cwnd。 
     * cwnd (RENO) = cwnd + 3B / (2 - B) * ack_cnt / cwnd 
     * B为乘性减少因子，在此算法中为0.3 
     */ 
	if (tcp_friendliness) {
		u32 scale = 15;//通过计算获得
		delta = (cwnd * scale) >> 3;/* delta代表多少ACK可使tcp_cwnd++ */ 
		//printf("cwnd = %d, scale = %d\n", cwnd, scale);
		//printf("入bictcp_update\n");
		while (ca->ack_cnt > delta) {		/* update tcp cwnd */
		//printf("ca->ack_cnt = %d, delta=%d\n", ca->ack_cnt, delta);
			ca->ack_cnt -= delta;
			ca->tcp_cwnd++;
		}
		//printf("出bictcp_update\n");

		if (ca->tcp_cwnd > cwnd){	/* if bic is slower than tcp */
			delta = ca->tcp_cwnd - cwnd;
			max_cnt = cwnd / delta;
			if (ca->cnt > max_cnt)
				ca->cnt = max_cnt;
		}
	}

	ca->cnt = (ca->cnt << ACK_RATIO_SHIFT) / ca->delayed_ack;
	if (ca->cnt == 0)			/* cannot be zero */
		ca->cnt = 1;/* 此时代表cwnd远小于bic_target，增长速度最大 */
}

/*
 * 拥塞避免
 */
static void bictcp_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca_cubic(sk);

	 // 如果传输中的报文数量 <= 窗口数量，那么没有必要执行拥塞避免
	// 如果发送拥塞窗口不被限制，不能再增加，则返回 
	if (!tcp_is_cwnd_limited(sk, in_flight))
		return;

	/*  希望收到的窗口大小比门限小,则进入慢启动.否则计算拥塞窗口,并进入拥塞避免阶段. */
	/*  after(ack, ca->end_seq): 表示end_seq-ack < 0 返回1, end_seq-ack >= 0 返回0*/
	if (tp->snd_cwnd <= tp->snd_ssthresh) {
		/*如果ack 包的序号大于,前面ACK 过数据*/
		if (hystart && after(ack, ca->end_seq))
			bictcp_hystart_reset(sk);
		
		//printf("进入慢启动,snd_cwnd=%d, snd_ssthresh=%d\n",tp->snd_cwnd,tp->snd_ssthresh);
		tcp_slow_start(tp);
	} else {
		;//printf("进入拥塞避免,snd_cwnd=%d, snd_ssthresh=%d\n",tp->snd_cwnd,tp->snd_ssthresh);
		bictcp_update(ca, tp->snd_cwnd);
		tcp_cong_avoid_ai(tp, ca->cnt);
	}

}

/**
 * 重新计算门限
 */
static u32 bictcp_recalc_ssthresh(struct sock *sk)
{

	const struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca_cubic(sk);
	printf("重新计算门限,snd_cwnd=%d, snd_ssthresh=%d\n",tp->snd_cwnd,tp->snd_ssthresh);

	ca->epoch_start = 0;	/* end of epoch */

	/* 当发送窗口小于上一次的最大窗口的时候,快速收敛;Wmax and fast convergence */
	/*如果第二拥塞, 并且cwnd 值小于前面一次的cwnd, 进入该流程*/
	if (tp->snd_cwnd < ca->last_max_cwnd && fast_convergence)
	{
		//last_max_cwnd = (snd_cwnd * (1024+717))/(2*1024)
		ca->last_max_cwnd = (tp->snd_cwnd * (BICTCP_BETA_SCALE + beta))
			/ (2 * BICTCP_BETA_SCALE);
	}
	else
	{
		ca->last_max_cwnd = tp->snd_cwnd;
	}

	/*丢包时候的窗口值*/
	ca->loss_cwnd = tp->snd_cwnd;

	return max((tp->snd_cwnd * beta) / BICTCP_BETA_SCALE, 2U);
}

static u32 bictcp_undo_cwnd(struct sock *sk)
{
	struct bictcp *ca = inet_csk_ca_cubic(sk);

	return max(tcp_sk(sk)->snd_cwnd, ca->last_max_cwnd);
}

static void bictcp_state(struct sock *sk, u8 new_state)
{
	if (new_state == TCP_CA_Loss) {
		bictcp_reset(inet_csk_ca_cubic(sk));
		bictcp_hystart_reset(sk);
	}
}

/*Hybrid Start算法*/
static void hystart_update(struct sock *sk, u32 delay)
{
	//printf("--->进入Hybrid Start\n");
	struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca_cubic(sk);

	//如果找到了, 只能在下次出现丢包的时候才能在次进入该流程
	if (!(ca->found & hystart_detect)) {
		u32 curr_jiffies = jiffies;

		/* first detection parameter - ack-train detection */
		/*如果ack  过来的速度小于等于2 ms */
		if (curr_jiffies - ca->last_jiffies <= msecs_to_jiffies(2)) {
			ca->last_jiffies = curr_jiffies;
			if (curr_jiffies - ca->round_start >= ca->delay_min>>4)
				ca->found |= HYSTART_ACK_TRAIN;
		}

		/* obtain the minimum delay of more than sampling packets */
		if (ca->sample_cnt < HYSTART_MIN_SAMPLES) {
			if (ca->curr_rtt == 0 || ca->curr_rtt > delay)
				ca->curr_rtt = delay;

			ca->sample_cnt++;
		} else {
			if (ca->curr_rtt > ca->delay_min + HYSTART_DELAY_THRESH(ca->delay_min>>4))
				ca->found |= HYSTART_DELAY;
		}
		/*
		 * Either one of two conditions are met,
		 * we exit from slow start immediately.
		 */
		if (ca->found & hystart_detect);
			tp->snd_ssthresh = tp->snd_cwnd;
	}
}

/* Track delayed acknowledgment ratio using sliding window
 * ratio = (15*ratio + sample) / 16
 */
static void bictcp_acked(struct sock *sk, u32 cnt, s32 rtt_us)
{

	const struct inet_connection_sock *icsk = inet_csk(sk);
	const struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca_cubic(sk);
	u32 delay;

	//printf("入bictcp_acked,snd_cwnd=%d, snd_ssthresh=%d\n",tp->snd_cwnd,tp->snd_ssthresh);

	if (icsk->icsk_ca_state == TCP_CA_Open) {
		cnt -= ca->delayed_ack >> ACK_RATIO_SHIFT;
		ca->delayed_ack += cnt;
	}

	/* Some calls are for duplicates without timetamps */
	if (rtt_us < 0)
		return;

	/* Discard delay samples right after fast recovery */
	if ((s32)(tcp_time_stamp - ca->epoch_start) < HZ)
		return;

	delay = usecs_to_jiffies(rtt_us) << 3;
	if (delay == 0)
		delay = 1;

	/* first time call or link delay decreases */
	if (ca->delay_min == 0 || ca->delay_min > delay)
		ca->delay_min = delay;

	/* hystart triggers when cwnd is larger than some threshold */
	if (hystart && tp->snd_cwnd <= tp->snd_ssthresh &&
	    tp->snd_cwnd >= hystart_low_window)
		hystart_update(sk, delay);
	
}

struct tcp_congestion_ops tcp_cubic = {
	.init		= bictcp_init,
	.ssthresh	= bictcp_recalc_ssthresh,
	.cong_avoid	= bictcp_cong_avoid,
	.set_state	= bictcp_state,
	.undo_cwnd	= bictcp_undo_cwnd,
	.pkts_acked = bictcp_acked,
	.owner		= THIS_MODULE,
	.name		= "cubic",
};

/*注册tcp_cubic算法*/
static int __init cubictcp_register(void)
{
	/* 始终会成立,因为bictcp结构的大小< 16*4字节*/
	BUILD_BUG_ON(sizeof(struct bictcp) > ICSK_CA_PRIV_SIZE);

	/* Precompute a bunch of the scaling factors that are used per-packet
	 * based on SRTT of 100ms
	 * beta_scale = 8*(1024 + 717) / 3 / (1024 -717 )，大约为15 
	 */

	beta_scale = 8 * (BICTCP_BETA_SCALE + beta) / 3 / (BICTCP_BETA_SCALE - beta);

	/* 1024 * c / rtt ，值为410 
	 * c = bic_scale >> 10 = 41 / 2^10 = 0.04 
     *  rtt = 100ms = 0.1s 
     * 如此算来，cube_rtt_scale = 1024 * c / rtt 
     * c / rtt = 0.4 
     */ 
	cube_rtt_scale = (bic_scale * 10);	/* 1024*c/rtt */


	/* calculate the "K" for (wmax-cwnd) = c/rtt * K^3
	 *  so K = cubic_root( (wmax-cwnd)*rtt/c )
	 * the unit of K is bictcp_HZ=2^10, not HZ
	 *
	 *  c = bic_scale >> 10
	 *  rtt = 100ms
	 *
	 * the following code has been designed and tested for
	 * cwnd < 1 million packets
	 * RTT < 100 seconds
	 * HZ < 1,000,00  (corresponding to 10 nano-second)
	 */

	/* 1/c * 2^2*bictcp_HZ * srtt */
	cube_factor = 1ull << (10+3*BICTCP_HZ); /* 2^40 */

	/* divide by bic_scale and by constant Srtt (100ms) */
	do_div(cube_factor, bic_scale * 10);

	// 注册拥塞控制算法
	tcp_register_congestion_control(&cubictcp);
	return 0;
}

static void __exit cubictcp_unregister(void)
{
	tcp_unregister_congestion_control(&cubictcp);
}

module_init(cubictcp_register);
module_exit(cubictcp_unregister);

MODULE_AUTHOR("Sangtae Ha, Stephen Hemminger");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CUBIC TCP");
MODULE_VERSION("2.3");
