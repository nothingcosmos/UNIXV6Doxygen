/*
 */

/**
 * RK disk driver
 */

#include "../param.h"
#include "../buf.h"
#include "../conf.h"
#include "../user.h"

#define	RKADDR	0177400
#define	NRK	4
#define	NRKBLK	4872

#define	RESET	0
#define	GO	01
#define	DRESET	014
#define	IENABLE	0100
#define	DRY	0200
#define	ARDY	0100
#define	WLO	020000
#define	CTLRDY	0200

struct {
	int rkds;
	int rker;
	int rkcs;
	int rkwc;
	int rkba;
	int rkda;
};

struct	devtab	rktab;
struct	buf	rrkbuf;

/// @brief swap等から呼ばれる
/// @param[in,out] abp swbufのアドレス address buf pages???
/// @note
/// - 読み出しと書き込みの両方のリクエストのサポート
rkstrategy(abp)
struct buf *abp;
{
	register struct buf *bp;
	register *qc, *ql;
	int d;

        /// - この処理は、PDP11/70の場合にのみ機能する
        ///   - abpのb_flagsがB_PHYSだった場合、
        ///     mapallocをcall
	bp = abp;
	if(bp->b_flags&B_PHYS)
		mapalloc(bp);
        /// - 
	d = bp->b_dev.d_minor-7;
	if(d <= 0)
		d = 1;
        /// - ブロック番号がでかすぎる場合
        ///   B_ERRORを設定してreturn
	if (bp->b_blkno >= NRKBLK*d) {
		bp->b_flags =| B_ERROR;
		iodone(bp);
		return;
	}
	bp->av_forw = 0;
        /// @note
        /// - 優先度は5だが、kernelレベルのtrapは6が多いらしいぞ
        /// - 一段階の優先度ロック 再帰的ではない。
	spl5();
        /// - バッファのコントローラーをFIFOにリンク
	if (rktab.d_actf==0)
		rktab.d_actf = bp;
	else
		rktab.d_actl->av_forw = bp;
	rktab.d_actl = bp;
	if (rktab.d_active==0)
		rkstart();
	spl0();
}

/// ディスクアドレスレジスタ(RKDA)の形に変換する
rkaddr(bp)
struct buf *bp;
{
	register struct buf *p;
	register int b;
	int d, m;

	p = bp;
	b = p->b_blkno;
	m = p->b_dev.d_minor - 7;
	if(m <= 0)
		d = p->b_dev.d_minor;
	else {
		d = lrem(b, m);
		b = ldiv(b, m);
	}
        /// - 15-13 ドライブ番号
        /// - 12-5  シリンダ番号
        /// - 4     面番号
        /// - 3-0   セクタアドレス
	return(d<<13 | (b/12)<<4 | b%12);
}

/// - d_activeをインクリメント
/// - devstartをcallする
rkstart()
{
	register struct buf *bp;

	if ((bp = rktab.d_actf) == 0)
		return;
	rktab.d_active++;
	devstart(bp, &RKADDR->rkda, rkaddr(bp), 0);
}
/// - ディスク操作が終了する際に発生する割り込みを処理する
rkintr()
{
	register struct buf *bp;

        /// - d_activeが0だったら処理終了
	if (rktab.d_active == 0)
		return;
	bp = rktab.d_actf;
	rktab.d_active = 0;
        /// - エラービットが設定されていれば
	if (RKADDR->rkcs < 0) {		/* error bit */
		deverror(bp, RKADDR->rker, RKADDR->rkds);
		RKADDR->rkcs = RESET|GO;
                /// - ステータスが変わるのを待ってる。  
                /// - volatileがないと、コンパイラの最適化で消されちゃうかも
		while((RKADDR->rkcs&CTLRDY) == 0) ;
                /// - 操作のリトライが10回未満であれば、再度挑戦
		if (++rktab.d_errcnt <= 10) {
			rkstart();
			return;
		}
                /// - そうでないならば、エラーを報告する
		bp->b_flags =| B_ERROR;
	}
	rktab.d_errcnt = 0;
	rktab.d_actf = bp->av_forw;
        /// この中でwakeupしてくれる
	iodone(bp);
	rkstart();
}

/// conf/mkconf.cにシンボルの記述あり
rkread(dev)
{

	physio(rkstrategy, &rrkbuf, dev, B_READ);
}

/// conf/mkconfにシンボルの記述あり
rkwrite(dev)
{

	physio(rkstrategy, &rrkbuf, dev, B_WRITE);
}
