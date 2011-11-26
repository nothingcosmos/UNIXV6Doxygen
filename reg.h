/**
 * @brief
 * - 仮想アドレス - 物理アドレス
 * - 0160000 - 017777 --> 0760000 - 0777777
 * - 0170000 clock.c UMODE <-- _start $170000,-(sp)
 * - 0170011 trap.c SETD
 * - 0170200 seg.h UBMAP
 * - 0170500 dhdm.c DMADDR
 * - 0172040 hs.c HSADDR
 * - 0172300 m40.s KISD0
 * - 0172340 m40.s KISA0
 * - 0172354 m40.s KISA6
 * - 0172522 m40.s MTC
 * - 0172440 ht.c HTADDR
 * - 0174000 dc.c DCADDR
 * - 0174779 dp.c DPADDR
 * - 0175200 dn.c DNADDR
 * - 0176500 kl.c KLBASE
 * - 0176700 hp.c HPADDR
 * - 0177460 rf.c RFADDR
 * - 0177514 lp.c LPADDR
 * - 0177546 main.c CLOCK1
 * - 0177550 pc.c PCADDR
 * - 0177560 param.h KL
 * - 0177570 param.h SW
 * - 0177572 conf/m40.s:SSR0
 * - 0177576 conf/m40.s:SSR2
 * - 0177600 seg.h UISD or UISD0
 * - 0177602 m40.s UISD1
 * - 0177640 seg.h UISA or UISA0
 * - 0177642 m40.s UISA1
 * - 0177660 seg.h UDSA
 * - 0177776 param.h PS
 *
 * Location of the users' stored
 * registers relative to R0.
 * Usage is u.u_ar0[XX].
 *
 */
#define dummy 0

/// accumulate
#define	R0	(0)
#define	R1	(-2)
/// arg
#define	R2	(-9)
#define	R3	(-8)
#define	R4	(-7)
/// dynamic chain (enviroment pointer)
#define	R5	(-6)
/// sp
#define	R6	(-3)
/// pc
#define	R7	(1)
#define	RPS	(2)

#define	TBIT	020		/* PS trace bit */
