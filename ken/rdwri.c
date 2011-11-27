/*
 */

#include "../param.h"
#include "../inode.h"
#include "../user.h"
#include "../buf.h"
#include "../conf.h"
#include "../systm.h"

/**
 * @brief
 *
 * Read the file corresponding to
 * the inode pointed at by the argument.
 * The actual read arguments are found
 * in the variables:
 *	u_base		core address for destination
 *	u_offset	byte offset in file
 *	u_count		number of bytes to read
 *	u_segflg	read to kernel/user
 */
readi(aip)
struct inode *aip;
{
	int *bp;
	int lbn, bn, on;
	register dn, n;
	register struct inode *ip;

	ip = aip;
	if(u.u_count == 0)
		return;
	ip->i_flag =| IACC;
	if((ip->i_mode&IFMT) == IFCHR) {
		(*cdevsw[ip->i_addr[0].d_major].d_read)(ip->i_addr[0]);
		return;
	}

	do {
                /// blknoを取得
		lbn = bn = lshift(u.u_offset, -9);
                /// maskする
		on = u.u_offset[1] & 0777;
                /// blockの境界を跨がらないものだけ取得する
                /// 最大で512
		n = min(512-on, u.u_count);
                /// formatがIFBLKでなければ
		if((ip->i_mode&IFMT) != IFBLK) {
                        /// - Asm
                        /// (arg1 - arg3) <= (arg2 - arg4)
			dn = dpcmp(ip->i_size0&0377, ip->i_size1,
				u.u_offset[0], u.u_offset[1]);
			if(dn <= 0)
				return;
			n = min(n, dn);
			if ((bn = bmap(ip, lbn)) == 0)
				return;
			dn = ip->i_dev;
		} else {
			dn = ip->i_addr[0];
			rablock = bn+1;
		}
                /// read a headの場合
		if (ip->i_lastr+1 == lbn)
			bp = breada(dn, bn, rablock);
		else
			bp = bread(dn, bn);
                /// read a headのフラグ補助
		ip->i_lastr = lbn;
		iomove(bp, on, n, B_READ);
		brelse(bp);
	} while(u.u_error==0 && u.u_count!=0);
}

/**
 * @brief
 *
 * Write the file corresponding to
 * the inode pointed at by the argument.
 * The actual write arguments are found
 * in the variables:
 *	u_base		core address for source
 *	u_offset	byte offset in file
 *	u_count		number of bytes to write
 *	u_segflg	write to kernel/user
 */
writei(aip)
struct inode *aip;
{
	int *bp;
	int n, on;
	register dn, bn;
	register struct inode *ip;

	ip = aip;
	ip->i_flag =| IACC|IUPD;
	if((ip->i_mode&IFMT) == IFCHR) {
		(*cdevsw[ip->i_addr[0].d_major].d_write)(ip->i_addr[0]);
		return;
	}
	if (u.u_count == 0)
		return;

	do {
		bn = lshift(u.u_offset, -9);
		on = u.u_offset[1] & 0777;
		n = min(512-on, u.u_count);
		if((ip->i_mode&IFMT) != IFBLK) {
			if ((bn = bmap(ip, bn)) == 0)
				return;
			dn = ip->i_dev;
		} else
			dn = ip->i_addr[0];
                /// - blockの全体を書く場合、そのまま
                /// - blockの途中から書きたい場合、既存のものを読んでから、書き込みしなおす
		if(n == 512) 
			bp = getblk(dn, bn); else
			bp = bread(dn, bn);

		iomove(bp, on, n, B_WRITE);
                /// 0777が0のときは、ブロック境界を跨いだ際、いままでの奴全部かいていいよという指示
		if(u.u_error != 0)
			brelse(bp); else
		if ((u.u_offset[1]&0777)==0)
                        /// 非同期
			bawrite(bp); else
                        /// 再割り当て時に立てる
			bdwrite(bp);

                /// ここはループの外に出していいんじゃない？
		if(dpcmp(ip->i_size0&0377, ip->i_size1,
		  u.u_offset[0], u.u_offset[1]) < 0 &&
		  (ip->i_mode&(IFBLK&IFCHR)) == 0) {
			ip->i_size0 = u.u_offset[0];
			ip->i_size1 = u.u_offset[1];
		}
                /// inode has been modified
		ip->i_flag =| IUPD;
	} while(u.u_error==0 && u.u_count!=0);
}

/**
 * Return the logical maximum
 * of the 2 arguments.
 */
max(a, b)
char *a, *b;
{

	if(a > b)
		return(a);
	return(b);
}

/**
 * Return the logical minimum
 * of the 2 arguments.
 */
min(a, b)
char *a, *b;
{

	if(a < b)
		return(a);
	return(b);
}

/**
 * @brief 
 *
 * Move 'an' bytes at byte location
 * &bp->b_addr[o] to/from (flag) the
 * user/kernel (u.segflg) area starting at u.base.
 * Update all the arguments by the number
 * of bytes moved.
 *
 * There are 2 algorithms,
 * if source address, dest address and count
 * are all even in a user copy,
 * then the machine language copyin/copyout
 * is called.
 * If not, its done byte-by-byte with
 * cpass and passc.
 */
iomove(bp, o, an, flag)
struct buf *bp;
{
	register char *cp;
	register int n, t;

	n = an;
	cp = bp->b_addr + o;
        /// - u_segflgが0の場合
	if(u.u_segflg==0 && ((n | cp | u.u_base)&01)==0) {
		if (flag==B_WRITE)
			cp = copyin(u.u_base, cp, n);
		else
			cp = copyout(cp, u.u_base, n);
		if (cp) {
			u.u_error = EFAULT;
			return;
		}
		u.u_base =+ n;
		dpadd(u.u_offset, n);
		u.u_count =- n;
		return;
	}
	if (flag==B_WRITE) {
                /// B_WRITEの場合
		while(n--) {
			if ((t = cpass()) < 0)
				return;
			*cp++ = t;
		}
	} else
               /// B_WRITEでない場合
		while (n--)
			if(passc(*cp++) < 0)
				return;
}
