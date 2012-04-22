#
/*
 */

#include "../param.h"
#include "../systm.h"
#include "../user.h"
#include "../inode.h"
#include "../file.h"
#include "../reg.h"

/**
 * @brief
 * Max allowable buffering per pipe.
 * This is also the max size of the
 * file created to implement the pipe.
 * If this size is bigger than 4096,
 * pipes will be implemented in LARG
 * files, which is probably not good.
 */
#define	PIPSIZ	4096

/**
 * @brief pipeを起動する
 * The sys-pipe entry.
 * Allocate an inode on the root device.
 * Allocate 2 file structures.
 * Put it all together with flags.
 */
pipe()
{
	register *ip, *rf, *wf;
	int r;

        // rootdev inode 確保(system call ialloc)
        // iallocで、incoreかつdisk上はリザーブしている。なぜか
        //   deviceにリザーブするのは、存在をユニークにしたいため
        //   一意性の確認
	ip = ialloc(rootdev);
	if(ip == NULL)
		return;
        // (system call falloc)
	rf = falloc();
	if(rf == NULL) {
		iput(ip);
		return;
	}
	r = u.u_ar0[R0];
        // (system call falloc)
	wf = falloc();
	if(wf == NULL) {
		rf->f_count = 0;
		u.u_ofile[r] = NULL;
		iput(ip);
		return;
	}
        // swap R0 to R1
        //   r  <- R0
        //   R1 <- R0
        //   R0 <- r
	u.u_ar0[R1] = u.u_ar0[R0];
	u.u_ar0[R0] = r;
        // write
	wf->f_flag = FWRITE|FPIPE;
	wf->f_inode = ip;
        // read
	rf->f_flag = FREAD|FPIPE;
	rf->f_inode = ip;
        // ip
	ip->i_count = 2;
	ip->i_flag = IACC|IUPD;
	ip->i_mode = IALLOC;
}

/**
 * @brief pipeを読み込む
 * Read call directed to a pipe.
 */
readp(fp)
int *fp;
{
	register *rp, *ip;

	rp = fp;
	ip = rp->f_inode;

loop:
	/*
	 * Very conservative locking.
	 */

	plock(ip);

	/*
	 * If the head (read) has caught up with
	 * the tail (write), reset both to 0.
	 */

	if(rp->f_offset[1] == ip->i_size1) {
		if(rp->f_offset[1] != 0) {
			rp->f_offset[1] = 0;
			ip->i_size1 = 0;
			if(ip->i_mode&IWRITE) {
				ip->i_mode =& ~IWRITE;
				wakeup(ip+1);
			}
		}

		/*
		 * If there are not both reader and
		 * writer active, return without
		 * satisfying read.
		 */

		prele(ip);
		if(ip->i_count < 2)
			return;
		ip->i_mode =| IREAD;
		sleep(ip+2, PPIPE);
		goto loop;
	}

	/*
	 * Read and return
	 */

	u.u_offset[0] = 0;
	u.u_offset[1] = rp->f_offset[1];
	readi(ip);
	rp->f_offset[1] = u.u_offset[1];
	prele(ip);
}

/**
 * @brief pipeへ書き込む
 * Write call directed to a pipe.
 */
writep(fp)
{
	register *rp, *ip, c;

	rp = fp;
	ip = rp->f_inode;
	c = u.u_count;

loop:

	/*
	 * If all done, return.
	 */

	plock(ip);
	if(c == 0) {
		prele(ip);
		u.u_count = 0;
		return;
	}

	/*
	 * If there are not both read and
	 * write sides of the pipe active,
	 * return error and signal too.
	 */

	if(ip->i_count < 2) {
		prele(ip);
		u.u_error = EPIPE;
		psignal(u.u_procp, SIGPIPE);
		return;
	}

	/*
	 * If the pipe is full,
	 * wait for reads to deplete
	 * and truncate it.
	 */

	if(ip->i_size1 == PIPSIZ) {
		ip->i_mode =| IWRITE;
		prele(ip);
		sleep(ip+1, PPIPE);
		goto loop;
	}

	/*
	 * Write what is possible and
	 * loop back.
	 */

	u.u_offset[0] = 0;
	u.u_offset[1] = ip->i_size1;
	u.u_count = min(c, PIPSIZ-u.u_offset[1]);
	c =- u.u_count;
	writei(ip);
	prele(ip);
	if(ip->i_mode&IREAD) {
		ip->i_mode =& ~IREAD;
		wakeup(ip+2);
	}
	goto loop;
}

/**
 * @brief
 * Lock a pipe.
 * If its already locked,
 * set the WANT bit and sleep.
 */
plock(ip)
int *ip;
{
	register *rp;

	rp = ip;
	while(rp->i_flag&ILOCK) {
		rp->i_flag =| IWANT;
		sleep(rp, PPIPE);
	}
	rp->i_flag =| ILOCK;
}

/**
 * @brief
 * Unlock a pipe.
 * If WANT bit is on,
 * wakeup.
 * This routine is also used
 * to unlock inodes in general.
 */
prele(ip)
int *ip;
{
	register *rp;

	rp = ip;
        /// ILOCKを落とす
	rp->i_flag =& ~ILOCK;
        /// IWANTだった場合
        /// つまり欲しがっていたら、起こしてあげる
	if(rp->i_flag&IWANT) {
		rp->i_flag =& ~IWANT;
		wakeup(rp);
	}
}
