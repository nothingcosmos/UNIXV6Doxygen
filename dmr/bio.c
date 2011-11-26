/*
 */

#include "../param.h"
#include "../user.h"
#include "../buf.h"
#include "../conf.h"
#include "../systm.h"
#include "../proc.h"
#include "../seg.h"

/**
 * This is the set of buffers proper, whose heads
 * were declared in buf.h.  There can exist buffer
 * headers not pointing here that are used purely
 * as arguments to the I/O routines to describe
 * I/O to be done-- e.g. swbuf, just below, for
 * swapping.
 */
char	buffers[NBUF][514]; // <-- なぜ514
struct	buf	swbuf;

/**
 * Declarations of the tables for the magtape devices;
 * see bdwrite.
 */
int	tmtab;
int	httab;

/**
 * The following several routines allocate and free
 * buffers with various side effects.  In general the
 * arguments to an allocate routine are a device and
 * a block number, and the value is a pointer to
 * to the buffer header; the buffer is marked "busy"
 * so that no on else can touch it.  If the block was
 * already in core, no I/O need be done; if it is
 * already busy, the process waits until it becomes free.
 * The following routines allocate a buffer:
 *	getblk
 *	bread
 *	breada
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	bawrite
 *	brelse
 */

/**
 * @brief
 * Read in (if necessary) the block and return a buffer pointer.
 * @note
 * - B_DONEだったらreturn
 * - B_READを設定 同期してIOから読み込む
 */
bread(dev, blkno)
{
	register struct buf *rbp;

	rbp = getblk(dev, blkno);
        /// IO完了したフラグ
	if (rbp->b_flags&B_DONE)
		return(rbp);

	rbp->b_flags =| B_READ;
	rbp->b_wcount = -256;
	(*bdevsw[dev.d_major].d_strategy)(rbp);
        /// 同期で読み終わるのを待つ
	iowait(rbp);
	return(rbp);
}

/**
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 *
 * rablknoは、次に読む奴なので、可能であれば呼んでおいて
 * 
 * breadaは、readiで読み込むが、次のinodeに並んでいる場合に限り、breadaが呼ばれる。
 * 普通の場合は、breadが呼ばれる。
 */
breada(adev, blkno, rablkno)
{
	register struct buf *rbp, *rabp;
	register int dev;

	dev = adev;
	rbp = 0;
	if (!incore(dev, blkno)) {
		rbp = getblk(dev, blkno);
		if ((rbp->b_flags&B_DONE) == 0) {
			rbp->b_flags =| B_READ;
			rbp->b_wcount = -256;
                        /// - 今欲しいblkをもらう
			(*bdevsw[adev.d_major].d_strategy)(rbp);
		}
	}
	if (rablkno && !incore(dev, rablkno)) {
		rabp = getblk(dev, rablkno);
		if (rabp->b_flags & B_DONE)
			brelse(rabp);
		else {
			rabp->b_flags =| B_READ|B_ASYNC;
			rabp->b_wcount = -256;
                        /// - 非同期で2つ目を発行
			(*bdevsw[adev.d_major].d_strategy)(rabp);
		}
	}
	if (rbp==0)
		return(bread(dev, blkno));
	iowait(rbp);
	return(rbp);
}

/**
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 *
 * バッファを書き込み、完了を待つ。
 * その後、バッファを開放する。
 *
 */
bwrite(bp)
struct buf *bp;
{
	register struct buf *rbp;
	register flag;

	rbp = bp;
	flag = rbp->b_flags;
	rbp->b_flags =& ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	rbp->b_wcount = -256;
	(*bdevsw[rbp->b_dev.d_major].d_strategy)(rbp);

        /// 非同期じゃなかったら、デバイスの書き込み動作を待つ
	if ((flag&B_ASYNC) == 0) {
		iowait(rbp);
		brelse(rbp);
	} else if ((flag&B_DELWRI)==0)
          /// 同期かつ遅延書き込みでない場合は
		geterror(rbp);
}

/**
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 *
 *
 * tarはテープ用のアーカイブコマンドです。
 *
 */
bdwrite(bp)
struct buf *bp;
{
	register struct buf *rbp;
	register struct devtab *dp;

	rbp = bp;
	dp = bdevsw[rbp->b_dev.d_major].d_tab;
	if (dp == &tmtab || dp == &httab)
		bawrite(rbp);
	else {
		rbp->b_flags =| B_DELWRI | B_DONE;
		brelse(rbp);
	}
}

/**
 * Release the buffer, start I/O on it, but don't wait for completion.
 * 非同期書き込み
 * callee 2箇所 bdwrite writei
 */
bawrite(bp)
struct buf *bp;
{
	register struct buf *rbp;

	rbp = bp;
	rbp->b_flags =| B_ASYNC;
	bwrite(rbp);
}

/**
 * @brief 引数でもらったbufをav-リストに戻す
 * I/Oが必要ない場合にバッファを開放する
 * release the buffer, with no I/O implied.
 * @note
 * - B_WANTED --> wakeup
 * - B_ERROR
 *
 * 実際はアンロック処理
 * フラグを初期化して浮かしておくだけで、使いまわす
 */
brelse(bp)
struct buf *bp;
{
	register struct buf *rbp, **backp;
	register int sps;

	rbp = bp;
	if (rbp->b_flags&B_WANTED)
		wakeup(rbp);
	if (bfreelist.b_flags&B_WANTED) {
		bfreelist.b_flags =& ~B_WANTED;
		wakeup(&bfreelist);
	}
	if (rbp->b_flags&B_ERROR)
		rbp->b_dev.d_minor = -1;  /* no assoc. on error */
	backp = &bfreelist.av_back;
        /// - integは、PSアドレスをinteger型として取り扱う
        /// - PSを一時退避してリエントエントに処理
        /// bpからフラグを消す。(B_WANTED | B_BUSY | B_ASYNC)
	sps = PS->integ;

        /// - ディスクからの割り込みの可能性があるため、ブロックする
        /// - ロックの代替処理として存在する。割り込みはブロックするべきではないので。
	spl6();
	rbp->b_flags =& ~(B_WANTED|B_BUSY|B_ASYNC);
	(*backp)->av_forw = rbp;
	rbp->av_back = *backp;
	*backp = rbp;
	rbp->av_forw = &bfreelist;

	PS->integ = sps;
}
/// V6は線形サーチだけど、大規模になってくると、ハッシュ化したりする。
/// ハードウェアの進歩に合わせて、ソフトウェアもどんどん工夫されていく

/**
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 * @note
 * - bdevsw[].d_tabの双方向リストの中から、該当するdevを探し当てる
 *   つまり、bdevsw[adev]の双方向リスト中から、blknoが一致するbuffを探す
 */
incore(adev, blkno)
{
	register int dev;
	register struct buf *bp;
	register struct devtab *dp;

	dev = adev;
	dp = bdevsw[adev.d_major].d_tab;
	for (bp=dp->b_forw; bp != dp; bp = bp->b_forw)
		if (bp->b_blkno==blkno && bp->b_dev==dev)
			return(bp);
	return(0);
}

/**
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 * When a 512-byte area is wanted for some random reason
 * (e.g. during exec, for the user arglist) getblk can be called
 * with device NODEV to avoid unwanted associativity.
 *
 * 指定のデバイスに対するバッファを割り当てる
 * 適切なブロックがすでに関連付けられているならば、それを返す。
 * そうでなければ、一番古くてビジーでないバッファを探して、それを再割り当てする
 *
 *
 * - version
 * メジャー 処理が決まる ドライバ
 * マイナー パーティション分割
 * tty メジャー　ドライバ
 * マイナーで、何個つながっているかどうか。
 */
getblk(dev, blkno)
{
	register struct buf *bp;
	register struct devtab *dp;
	extern lbolt;

        /// - 未定義のデバイス
        ///  - panic
	if(dev.d_major >= nblkdev)
		panic("blkdev");

    loop:
	if (dev < 0)
		dp = &bfreelist;
	else {
		dp = bdevsw[dev.d_major].d_tab;
		if(dp == NULL)
			panic("devtab");
                /// - incoreと同様の検索
                ///
                /// - メイン処理
                ///   ブロック番号を探している。
		for (bp=dp->b_forw; bp != dp; bp = bp->b_forw) {
			if (bp->b_blkno!=blkno || bp->b_dev!=dev)
				continue;
			spl6();
                        /// - B_BUSYの場合は、B_WANTEDのフラグを追加して
                        ///   sleepする
			if (bp->b_flags&B_BUSY) {
				bp->b_flags =| B_WANTED;
				sleep(bp, PRIBIO);
				spl0();
				goto loop;
			}
			spl0();
			notavail(bp);
			return(bp);
		}
	}
        /// - 求めていたbufが見つからない場合

        /// - av-リストが空の場合、
        ///   B_WANTEDフラグを設定してsleepする
	spl6();
	if (bfreelist.av_forw == &bfreelist) {
		bfreelist.b_flags =| B_WANTED;
		sleep(&bfreelist, PRIBIO);
		spl0();
		goto loop;
	}
        /// - av-リストが空でない場合
	spl0();
	notavail(bp = bfreelist.av_forw);
        /// - B_DELWRIだった場合、B_ASYNCをセットしてbwriteをcallする
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags =| B_ASYNC;
		bwrite(bp);
		goto loop;
	}
        /// - 新しいdevの入れ直し
	bp->b_flags = B_BUSY | B_RELOC;
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_dev = dev;
	bp->b_blkno = blkno;
	return(bp);
}

/**
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
iowait(bp)
struct buf *bp;
{
	register struct buf *rbp;

	rbp = bp;
	spl6();
	while ((rbp->b_flags&B_DONE)==0)
		sleep(rbp, PRIBIO);
	spl0();
	geterror(rbp);
}

/**
 * @brief 引数で指定したbpを双方向リンクリストから取り除き、
 *        B_BUSYフラグをセットする
 * Unlink a buffer from the available list and mark it busy.
 * (internal interface)
 * @note
 * - getblkから呼ばれる
 * - 引数で与えたbpを、双方向リンクリストから取り除く
 */
notavail(bp)
struct buf *bp;
{
	register struct buf *rbp;
	register int sps;

	rbp = bp;
	sps = PS->integ;
	spl6();
        /// 双方向リストから自身rbpを取り除く処理
	rbp->av_back->av_forw = rbp->av_forw;
	rbp->av_forw->av_back = rbp->av_back;
	rbp->b_flags =| B_BUSY;
	PS->integ = sps;
}

/**
 * @brief ブロック入出力操作の完了処理 B_DONE && wakeup
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 * @note
 * - b_flags B_MAP | B_DONE | B_ASYNC | B_WANTED
 * - not B_ASYNCの場合、B_WANTEDフラグをリセットする
 */
iodone(bp)
struct buf *bp;
{
	register struct buf *rbp;

	rbp = bp;
	if(rbp->b_flags&B_MAP)
		mapfree(rbp);
        /// - B_DONEをセット
	rbp->b_flags =| B_DONE;
	if (rbp->b_flags&B_ASYNC)
		brelse(rbp);
	else {
		rbp->b_flags =& ~B_WANTED;
                /// - wakeup
		wakeup(rbp);
	}
}

/**
 * @brief バッファの最初の256word/512byteを開放する.というか0を敷き詰める
 * Zero the core associated with a buffer.
 */
clrbuf(bp)
int *bp;
{
	register *p;
	register c;

	p = bp->b_addr;
	c = 256;
	do
		*p++ = 0;
	while (--c);
}

/**
 *
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 * @note
 * - caller main
 */
binit()
{
	register struct buf *bp;
	register struct devtab *dp;
	register int i;
	struct bdevsw *bdp;

        /// - bfreelist initialize
	bfreelist.b_forw = bfreelist.b_back =
	    bfreelist.av_forw = bfreelist.av_back = &bfreelist;
        /// - とりあえずヘッダのみ初期化
	for (i=0; i<NBUF; i++) {
		bp = &buf[i];
		bp->b_dev = -1;
		bp->b_addr = buffers[i];
		bp->b_back = &bfreelist;
		bp->b_forw = bfreelist.b_forw;
		bfreelist.b_forw->b_back = bp;
		bfreelist.b_forw = bp;
                /// B_BUSYで初期化している
		bp->b_flags = B_BUSY;
		brelse(bp);
	}
        /// - bdevsw initialize
	i = 0;
	for (bdp = bdevsw; bdp->d_open; bdp++) {
		dp = bdp->d_tab;
		if(dp) {
			dp->b_forw = dp;
			dp->b_back = dp;
		}
		i++;
	}
	nblkdev = i;
}

/**
 * Device start routine for disks
 * and other devices that have the register
 * layout of the older DEC controllers (RF, RK, RP, TM)
 * @note
 * - RKCSの仕様に合わせてレジスタをフラグで埋める
 */
#define	IENABLE	0100
#define	WCOM	02
#define	RCOM	04
#define	GO	01
devstart(bp, devloc, devblk, hbcom)
struct buf *bp;
int *devloc;
{
	register int *dp;
	register struct buf *rbp;
	register int com;

	dp = devloc;
	rbp = bp;
	*dp = devblk;			/* block address */
	*--dp = rbp->b_addr;		/* buffer address */
	*--dp = rbp->b_wcount;		/* word count */
	com = (hbcom<<8) | IENABLE | GO |
		((rbp->b_xmem & 03) << 4);
	if (rbp->b_flags&B_READ)	/* command + x-mem */
		com =| RCOM;
	else
		com =| WCOM;
	*--dp = com;
}

/**
 * startup routine for RH controllers.
 */
#define	RHWCOM	060
#define	RHRCOM	070

rhstart(bp, devloc, devblk, abae)
struct buf *bp;
int *devloc, *abae;
{
	register int *dp;
	register struct buf *rbp;
	register int com;

	dp = devloc;
	rbp = bp;
	if(cputype == 70)
		*abae = rbp->b_xmem;
	*dp = devblk;			/* block address */
	*--dp = rbp->b_addr;		/* buffer address */
	*--dp = rbp->b_wcount;		/* word count */
	com = IENABLE | GO |
		((rbp->b_xmem & 03) << 8);
	if (rbp->b_flags&B_READ)	/* command + x-mem */
		com =| RHRCOM; else
		com =| RHWCOM;
	*--dp = com;
}

/**
 * 11/70 routine to allocate the
 * UNIBUS map and initialize for
 * a unibus device.
 * The code here and in
 * rhstart assumes that an rh on an 11/70
 * is an rh70 and contains 22 bit addressing.
 */
int	maplock;
mapalloc(abp)
struct buf *abp;
{
	register i, a;
	register struct buf *bp;

	if(cputype != 70)
		return;
	spl6();
	while(maplock&B_BUSY) {
		maplock =| B_WANTED;
		sleep(&maplock, PSWP);
	}
	maplock =| B_BUSY;
	spl0();
	bp = abp;
	bp->b_flags =| B_MAP;
	a = bp->b_xmem;
	for(i=16; i<32; i=+2)
		UBMAP->r[i+1] = a;
	for(a++; i<48; i=+2)
		UBMAP->r[i+1] = a;
	bp->b_xmem = 1;
}

mapfree(bp)
struct buf *bp;
{

	bp->b_flags =& ~B_MAP;
	if(maplock&B_WANTED)
		wakeup(&maplock);
	maplock = 0;
}

/**
 * @brief  swap I/O coreaddrをblknoに書き出す処理
 * @param[in,out] blkno
 * @param[in,out] coreaddr
 * @param[in,out] count size???
 * @param[in,out] rdflg 0 or B_READ @todo 0はB_WRITE ???
 * @retval 1 B_ERRORがセットされていたら1を返して、呼び出し元でpanicする
 * @retval 0 正常終了
 * @todo
 *   rdflgが0は、B_WRITEフラグのこと？
 *   p_addrがswap対象の引数になる場合はどういうケース？？？
 *   B_BUSYフラグがセットされるのはどんな状況下だろうか？
 *   spl6()はプロセッサ優先度を6に設定する
 *   swapdevって何よ
 *   swapはディスクIOが終わるまでまつが、その際にsleepするので、
 *   他プロセスに処理を移譲する
 */
swap(blkno, coreaddr, count, rdflg)
{
	register int *fp;
        /// - 優先度レベル6にする(恐らくsleep()中に変わる可能性がある)
        /// - swapbufのb_flagsがB_BUSYの間、B_WANTEDを設定してsleepし、
        ///   待機する
        ///   
	fp = &swbuf.b_flags;
	spl6();
	while (*fp&B_BUSY) {
		*fp =| B_WANTED;
		sleep(fp, PSWP);
	}
        /// - b_flagsにB_BUSY | B_PHYS | rdflg(参照点はここだけ) を設定する
	*fp = B_BUSY | B_PHYS | rdflg;
	swbuf.b_dev = swapdev;
	swbuf.b_wcount = - (count<<5);	/* 32 w/block */
	swbuf.b_blkno = blkno;
	swbuf.b_addr = coreaddr<<6;	/* 64 b/block */
	swbuf.b_xmem = (coreaddr>>10) & 077;
	(*bdevsw[swapdev>>8].d_strategy)(&swbuf);

        /// - 優先度レベル6にセットする
        /// - B_DONEでない間、sleep()で待機する
        /// - B_DONEでなくなり、B_WANTEDフラグがセットされていたら、
        ///   wakeup()する
	spl6();
	while((*fp&B_DONE)==0)
		sleep(fp, PSWP);
	if (*fp&B_WANTED)
		wakeup(fp);

        /// - 優先度レベルを0に戻す
        /// - B_BUSYフラグとB_WANTEDフラグをリセットする
	spl0();
	*fp =& ~(B_BUSY|B_WANTED);
	return(*fp&B_ERROR);
}

/**
 * make sure all write-behind blocks
 * on dev (or NODEV for all)
 * are flushed out.
 * (from umount and update)
 *
 *
 * 遅延書き込みフラグがたっている場合の、実書き込み
 */
bflush(dev)
{
	register struct buf *bp;

loop:
	spl6();
	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
          /// 遅延書き込みフラグだったら
		if (bp->b_flags&B_DELWRI && (dev == NODEV||dev==bp->b_dev)) {
			bp->b_flags =| B_ASYNC;
			notavail(bp);
			bwrite(bp);
			goto loop;
		}
	}
	spl0();
}

/**
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 *
 * バッファを経由せずに直接書き込み
 * 色々なドライバから呼ばれる。
 *
 *
 */
physio(strat, abp, dev, rw)
struct buf *abp;
int (*strat)();
{
	register struct buf *bp;
	register char *base;
	register int nb;
	int ts;

	bp = abp;
	base = u.u_base;
	/*
	 * Check odd base, odd count, and address wraparound
	 */
	if (base&01 || u.u_count&01 || base>=base+u.u_count)
		goto bad;
	ts = (u.u_tsize+127) & ~0177;
	if (u.u_sep)
		ts = 0;
	nb = (base>>6) & 01777;
	/*
	 * Check overlap with text. (ts and nb now
	 * in 64-byte clicks)
	 */
	if (nb < ts)
		goto bad;
	/*
	 * Check that transfer is either entirely in the
	 * data or in the stack: that is, either
	 * the end is in the data or the start is in the stack
	 * (remember wraparound was already checked).
	 */
	if ((((base+u.u_count)>>6)&01777) >= ts+u.u_dsize
	    && nb < 1024-u.u_ssize)
		goto bad;
	spl6();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags =| B_WANTED;
		sleep(bp, PRIBIO);
	}
	bp->b_flags = B_BUSY | B_PHYS | rw;
	bp->b_dev = dev;
	/*
	 * Compute physical address by simulating
	 * the segmentation hardware.
	 */
	bp->b_addr = base&077;
	base = (u.u_sep? UDSA: UISA)->r[nb>>7] + (nb&0177);
	bp->b_addr =+ base<<6;
	bp->b_xmem = (base>>10) & 077;
	bp->b_blkno = lshift(u.u_offset, -9);
	bp->b_wcount = -((u.u_count>>1) & 077777);
	bp->b_error = 0;
	u.u_procp->p_flag =| SLOCK;
	(*strat)(bp);
	spl6();
	while ((bp->b_flags&B_DONE) == 0)
		sleep(bp, PRIBIO);
	u.u_procp->p_flag =& ~SLOCK;
	if (bp->b_flags&B_WANTED)
		wakeup(bp);
	spl0();
	bp->b_flags =& ~(B_BUSY|B_WANTED);
	u.u_count = (-bp->b_resid)<<1;
	geterror(bp);
	return;
    bad:
	u.u_error = EFAULT;
}

/**
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 *
 *
 *
 */
geterror(abp)
struct buf *abp;
{
	register struct buf *bp;

	bp = abp;
	if (bp->b_flags&B_ERROR)
		if ((u.u_error = bp->b_error)==0)
			u.u_error = EIO;
}
