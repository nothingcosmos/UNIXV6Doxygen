#define proc
/**
 * @brief
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 * @note
 * - proc[] == NULL && p_state == 0
 * - p_uid == u_uid
 * - proc構造体は、core常駐のproc配列に含まれ、いつでもアクセス可能である
 * - per process data area / kernel mode stack area /
 * - data segment は下記から構成される
 *  - per process data area ... size = USIZE
 *   - (user structure, kernel mode stack area)
 *  - user process data area ... size = u.u_dsize
 *   - (program text, initialize data, noninitialize data)
 *  - user program stack ... size = u.u_ssize
 * @todo
 */
struct	proc
{
#define p_stat
	/// 7つの排他的な状態を定義し,内1つを取る
        /// 0が設定されていることもある。
        /// pstateがzonmbiの場合、初期化してp_state=0にする
	char	p_stat;
#define p_flag
        /// 6個の1bit flags
	char	p_flag;
#define p_pri
        /// priority, 小さいほど高い 
        /// p_priを一度
	char	p_pri;
#define p_sig
        /// このプロセスに送られるシグナル番号
	char	p_sig;
#define p_uid
        /// user id, used to direct tty signals */
	char	p_uid;
#define p_time
        /// resident time for scheduling */
	char	p_time;
#define p_cpu
        /// cpu usage for scheduling */
	char	p_cpu;
#define p_nice
        /// nice for scheduling */
	char	p_nice;
#define p_ttyp
        /// controlling tty */
	int	p_ttyp;
#define p_pid
        /// unique process id */
	int	p_pid;
#define p_ppid
        /// process id of parent */
	int	p_ppid;
#define p_addr
        /// スワップ可能イメージのアドレス
        /// data segment へのaddr
        /// data segment がメインメモリにあるならば、block 番号
        /// そうでなく、data segment がswapout されているならば、
        /// これはdisk record 番号である
	int	p_addr;
#define p_size
        /// スワップ可能イメージのサイズ(64byte単位)
        /// block単位で測られるdata segment の size
	int	p_size;
#define p_wchan
        /// プロセスが待っているイベント
        /// sleepしているprocess (p_state が SSLEAP or SWAIT)に対して、
        /// sleepの理由を識別する
        /// @note sched->sleepでは、runout++が設定された
	int	p_wchan;
#define p_textp
        /// null or textへのpointer
	int	*p_textp;
} proc[NPROC];


/** stat codes */
/// このstatは排他的である
/// 0は空を定義する

/// set sleep()
#define	SSLEEP	1		/**< sleeping on high priority */
/// set sleep()
#define	SWAIT	2		/**< sleeping on low priority */
/// set main()
/// set setrun()
/// set newproc()
#define	SRUN	3		/**< running */
/// set newproc()
#define	SIDL	4		/**< intermediate state in process creation */
/// set exit()
#define	SZOMB	5		/**< intermediate state in process termination*/
/// set stop()
#define	SSTOP	6		/**< process being traced */

/** flag codes */
/// フラグなので、組み合わせ自由

/// @brief in core
/// set   main()
///       sched()
///       newproc()
/// unset sched()   xswap()を呼び出す前に解除
///       xmalloc() swapする際に解除
#define	SLOAD	01

/// @brief scheduling process
/// set   main()    system processに対して設定
#define	SSYS	02

/// @brief process cannot be swapped
/// swap()と(*strat)()を呼び出す前後を囲む
/// set   physio()   (*strat)()の呼び出しの前
///       xswap()    swap()の呼び出しの前
///       swap()     swap()の呼び出しの前
/// unset phisio()   ...の後
///       xswap()    ...の後
///       xalloc()   ...の後
///
/// swap()は、B_DONEまでsleep()で待つ
/// (*strat)()の呼び出し後、B_DONEまでsleep()で待つ
///
/// SLOCKの参照先は、sched()
#define	SLOCK	04

/// @brief process is being swapped out
/// 
/// set   newproc()
///       expand()
///       xalloc()
/// unset swtch()
/// 
/// 参照先はswtch()のみ
/// idiom002を参照
#define	SSWAP	010

/// @brief process is being traced
///
#define	STRC	020

/// @brief another tracing flag
///
#define	SWTED	040





// dummy for gtags
#define spl0
#define spl1
#define spl2 //<-- not defined
#define spl3 //<-- not defined
#define spl4
#define spl5
#define spl6
#define spl7
