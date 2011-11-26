/*
 */

#include "../param.h"
#include "../systm.h"
#include "../user.h"
#include "../proc.h"
#include "../text.h"
#include "../inode.h"

/**
 * Swap out process p.
 * @note
 * The ff flag causes its core to be freed--
 * it may be off when called to create an image for a
 * child process in newproc.
 * Os is the old size of the data area of the process,
 * and is supplied during core expansion swaps.
 * @param[in, out] p
 * @param[in] ff
 * @param[in] os oldsize processの古サイズ 0指定の場合はpのp_sizeを参照する
 *
 * @attention
 * - panic: out of swap space
 * - panic: swap error -- IO error
 * @note
 * - プロセスpをswapmapへswap outする
 * - swap outされたpは、swapmapの領域を保持し、SLOADがfalseになる
 * @par 詳細:
 */
xswap(p, ff, os)
int *p;
{
	register *rp, a;

	rp = p;
        /// - osが0だった場合、xp->p_sizeを使う
        /// - swapmapから、(p_p_size+7)/8 の領域を確保する
	if(os == 0)
		os = rp->p_size;
	a = malloc(swapmap, (rp->p_size+7)/8);
	if(a == NULL)
		panic("out of swap space");
        /// - p->p_textpのcoreカウント参照数を減らす(x_countは減らさない)
        /// - p->p_flagにSLOCKを設定する
        ///  - (スワップアウト中のプロセスを
        ///     再度スワップアウトしようとするのを防ぐ)
        /// - swapmapに確保した領域とpの領域をswapする
        /// - もしffが0以外の場合、pの領域をcoremapからmfreeする
	xccdec(rp->p_textp);
	rp->p_flag =| SLOCK;
	if(swap(a, rp->p_addr, os, 0))
		panic("swap error");
	if(ff)
		mfree(coremap, os, rp->p_addr);

        /// - pをswapした領域へ更新する
        ///  - p_flagから、SLOAD|SLOCKをoffにする
        ///  - SLOADは、coreにloadされているフラグ
        ///  - SLOCKは、プロセスをスワップできないフラグ
        /// - runoutが設定されていれば、
        ///   schedがスワップインするプロセスを待っているので、
        ///   runoutを目覚めさせる
	rp->p_addr = a;
	rp->p_flag =& ~(SLOAD|SLOCK);
	rp->p_time = 0;
	if(runout) {
		runout = 0;
		wakeup(&runout);
	}
}

/**
 * @brief x_daddr用の開放処理
 * relinquish use of the shared text segment
 * of a process.
 * @note
 * - プロセスが終了する際に、exitから呼び出される
 * - プロセスが姿を変える場合に、execから呼び出される
 * - swapしないのにxccdec()が呼ばれる理由は、
 *   x_count >= x_ccountが成立するため、
 *   coreの方も開放をチェックする必要があるためかな？
 * @par 詳細:
 */
xfree()
{
	register *xp, *ip;

        /// - p_textpがNULLの場合、何もせず終了
        /// - userのp_textpをxpに保存して、userのp_textpをNULLに設定
        /// - x_ccountを減らし、可能であれば開放する
        /// - x_countを1減じて、0だった場合、
        ///  - inode->i_modeがISVTXでない場合、
        ///    (これは再利用を期待して、開放しないようにするフラグである。)
        ///   - xpのx_iptrをNULLに設定
        ///   - swapmapからx_daddrを開放する
        ///   - i_flagのITEXTを無効にする
        ///   - @todo iput(ip)
	if((xp=u.u_procp->p_textp) != NULL) {
		u.u_procp->p_textp = NULL;
		xccdec(xp);
		if(--xp->x_count == 0) {
			ip = xp->x_iptr;
			if((ip->i_mode&ISVTX) == 0) {
				xp->x_iptr = NULL;
				mfree(swapmap, (xp->x_size+7)/8, xp->x_daddr);
				ip->i_flag =& ~ITEXT;
				iput(ip);
			}
		}
	}
}

/**
 * @brief
 * Attach to a shared text segment.
 * @note
 * If there is no shared text, just return.
 * If there is, hook up to it:
 * if it is not currently being used, it has to be read
 * in from the inode (ip) and established in the swap space.
 * If it is being used, but is not currently in core,
 * a swap has to be done to get it back.
 * The full coroutine glory has to be invoked--
 * see slp.c-- because if the calling process
 * is misplaced in core the text image might not fit.
 * Quite possibly the code after "out:" could check to
 * see if the text does fit and simply swap it in.
 *
 * @param[in] ip struct inode
 * @note
 * - テキストセグメントの割り当てやテキストセグメントへのリンクを扱うため、
 *   execから呼び出される
 * - execから呼び出されるので、必ず最初にswapへtextが配置されることになる
 * @attention
 * - panic: out of swap space
 * @pre
 * - inode != 0 // caller exec()
 * @par 詳細:
 */
xalloc(ip)
int *ip;
{
	register struct text *xp;
	register *rp, ts;

        /// - current system callの引数[1]が0だった場合、return
        ///   (execを参照すると、dataとtextがくっついて、text=0のケース)
	if(u.u_arg[1] == 0)
		return;

        /// - text配列を走査し、
        ///  - x_iptrがNULLのtextを探す(複数あったら、最初に見つけた領域が対象)
        ///  - x_iptrがipと等しかった場合、
        ///   - textのx_countを+1する(x_countを加算する場所はここだけである)
        ///   - user procのp_textpに探したtextを設定する
        ///   - 終了処理(out)へgo
        /// - 見つけられなかった、panicする
	rp = NULL;
	for(xp = &text[0]; xp < &text[NTEXT]; xp++)
		if(xp->x_iptr == NULL) {
			if(rp == NULL)
				rp = xp;
		} else
			if(xp->x_iptr == ip) {
				xp->x_count++;
				u.u_procp->p_textp = xp;
				goto out;
			}
	if((xp=rp) == NULL)
		panic("out of text");

        /// - text領域の初期化
        ///  - iptrがNULLなtext領域を初期化する
        ///   - x_count = 1   //参照カウント
        ///   - x_ccount = 0; //loaded references
        ///   - x_iptr = ip(引数)
	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_iptr = ip;

        /// - u_arg[1]は、textセグメントのサイズ
        /// - x_sizeに値を設定する(唯一の値設定)
        /// - swapmapに領域確保。x_daddrに設定
	ts = ((u.u_arg[1]+63)>>6) & 01777;
	xp->x_size = ts;
	if((xp->x_daddr = malloc(swapmap, (ts+7)/8)) == NULL)
		panic("out of swap space");

        /// - pagingのアドレスやらレジスタやら設定
	expand(USIZE+ts);
	estabur(0, ts, 0, 0);

        /// - user構造体に値設定 readi()で使われるはず
	u.u_count = u.u_arg[1];
	u.u_offset[1] = 020;
	u.u_base = 0;
	readi(ip);
	rp = u.u_procp;

        /// - SLOCKを設定し、swapをロックしてから
        /// - swapmapに確保したばかりの領域をswapする
        /// - SLOCKをfalseにする
	rp->p_flag =| SLOCK;
	swap(xp->x_daddr, rp->p_addr+USIZE, ts, 0);
	rp->p_flag =& ~SLOCK;

        /// - p_textpにxpを設定する(x_countは1で初期化ずみ)
	rp->p_textp = xp;

        /// - inode関連の初期化
	rp = ip;
	rp->i_flag =| ITEXT;
	rp->i_count++;
	expand(USIZE);

        /// - out
        ///  - x_ccountが0の場合(上記の初期化パスを通ってきた場合に限る)
        ///  - savu()でu_rsavとu_ssavにr5/r6を一時保存し、復帰先を保存する
        ///   - swtch()への擬似コルーチンジャンプの前処理(exec()へ戻る) 
        ///     よってx_ccount++は行わない。
        ///  - xswap()
        ///  - p_flagにSSWAPを設定してswtch()へコルーチンジャンプする
        /// - x_ccount++して処理終了
out:
	if(xp->x_ccount == 0) {
		savu(u.u_rsav);
		savu(u.u_ssav);
		xswap(u.u_procp, 1, 0);
		u.u_procp->p_flag =| SSWAP;
		swtch();
		/* no return */
	}
	xp->x_ccount++;
}

/**
 * @brief text領域のx_ccountをデクリメントし、
 *        0になったらcoremapからtextを開放する
 * @note
 * Decrement the in-core usage count of a shared text segment.
 * When it drops to zero, free the core space.
 * @param[in,out] xp struct text
 * @note
 * - x_ccountの参照カウントのみ参照/操作する
 * - coreからswapされる際にセットで呼ばれると考えてよいはず
 * @par 詳細:
 */
xccdec(xp)
int *xp;
{
	register *rp;

        /// - xpが0 もしくは x_ccountが0だった場合何もしない
        /// - x_ccountを-1し、0になったらmfreeでcoremapから開放する
	if((rp=xp)!=NULL && rp->x_ccount!=0)
		if(--rp->x_ccount == 0)
			mfree(coremap, rp->x_size, rp->x_caddr);
}
