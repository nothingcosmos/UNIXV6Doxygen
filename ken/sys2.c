#include "../param.h"
#include "../systm.h"
#include "../user.h"
#include "../reg.h"
#include "../file.h"
#include "../inode.h"

/// @brief
/// ファイルIO関連のシステムコールを定義している
/// プロセッサ優先度によるロックアンロックを行う必要はないか
///
///

/**
 * @brief
 * read system call
 */
read()
{
	rdwr(FREAD);
}

/**
 * @brief
 * write system call
 */
write()
{
	rdwr(FWRITE);
}

/**
 * @brief
 * @param[in,out] mode FREAD or FWRITE
 *
 * common code for read and write calls:
 * check permissions, set base, count, and offset,
 * and switch out to readi, writei, or pipe code.
 */
rdwr(mode)
{
	register *fp, m;

        /// - ファイルディスクリプタの取得
	m = mode;
	fp = getf(u.u_ar0[R0]);
	if(fp == NULL)
		return;

        /// - modeとfpが一致しない場合、ERROR
	if((fp->f_flag&m) == 0) {
		u.u_error = EBADF;
		return;
	}
        /// - system callの引数取得
        ///   user構造体に設定する
	u.u_base = u.u_arg[0];
	u.u_count = u.u_arg[1];
	u.u_segflg = 0;

	if(fp->f_flag&FPIPE) {
                /// - PIPEだったら、
                ///   readp or writep
		if(m==FREAD)
			readp(fp); else
			writep(fp);
	} else {
                /// - 普通のファイルだったら
                ///   u_offsetにf_offsetを設定して、
                ///   readi or writei
		u.u_offset[1] = fp->f_offset[1];
		u.u_offset[0] = fp->f_offset[0];
		if(m==FREAD)
			readi(fp->f_inode); else
			writei(fp->f_inode);
		dpadd(fp->f_offset, u.u_arg[1]-u.u_count);
	}
        /// ファイルに書き込んだサイズを書き込む
	u.u_ar0[R0] = u.u_arg[1]-u.u_count;
}

/**
 * @brief 既存のファイルを参照する場合
 * open system call
 */
open()
{
	register *ip;
	extern uchar;

	ip = namei(&uchar, 0);
	if(ip == NULL)
		return;
        /// - ユーザプログラミング規約と内部データ表現の間にずれがあるので
	u.u_arg[1]++;
        /// - 0を指定して呼び出す
	open1(ip, u.u_arg[1], 0);
}

/**
 * @brief
 * creat system call
 *
 */
creat()
{
	register *ip;
	extern uchar;

        /// - ucharは、ユーザプログラムのデータ領域から1文字ずつパス名を取り出す手続きの名前
	ip = namei(&uchar, 1);
	if(ip == NULL) {
                /// - エラーが発生した
		if(u.u_error)
			return;
                /// - その名前のファイルが存在しない
                /// @attention
                /// - スティッキービットの明示的なリセットに注意すること
		ip = maknode(u.u_arg[1]&07777&(~ISVTX));
		if (ip==NULL)
			return;
		open1(ip, FWRITE, 2);
	} else
		open1(ip, FWRITE, 1);
}

/**
 * @brief
 * common code for open and creat.
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 *
 * @param[in,out] ip
 * @param[in,out] mode
 * @param[in,out] trf 2<--指定したファイルが存在しない場合
 *
 */
open1(ip, mode, trf)
int *ip;
{
	register struct file *fp;
	register *rip, m;
	int i;

	rip = ip;
	m = mode;
	if(trf != 2) {
                /// check permission if FREAD --> IREAD
		if(m&FREAD)
			access(rip, IREAD);
		if(m&FWRITE) {
			access(rip, IWRITE);
			if((rip->i_mode&IFMT) == IFDIR)
				u.u_error = EISDIR;
		}
	}
	if(u.u_error)
		goto out;
        /// - trfが0以外だったら、
        ///   ファイルを生成しようとしていることになるため、
        ///   前の内容を消去する
	if(trf)
		itrunc(rip);
        /// - inodeのアンロックを行う
	prele(rip);
	if ((fp = falloc()) == NULL)
		goto out;
	fp->f_flag = m&(FREAD|FWRITE);
	fp->f_inode = rip;
        /// 異常終了時のためのreserve
	i = u.u_ar0[R0];

        /// - inodeをげっと
        /// - 正常処理の場合、return
	openi(rip, m&FWRITE);
	if(u.u_error == 0)
		return;

        /// - 異常処理
	u.u_ofile[i] = NULL;
	fp->f_count--;

out:
        /// - 異常があった場合、inodeエントリを開放する
	iput(rip);
}

/**
 * @brief
 *
 * close system call
 */
close()
{
	register *fp;

        /// inodeのfpを取得
	fp = getf(u.u_ar0[R0]);
	if(fp == NULL)
		return;
	u.u_ofile[u.u_ar0[R0]] = NULL;
	closef(fp);
}

/**
 * @brief
 * @param[in,out] tbuf
 * @param[in,out] txtsiz or datsiz
 * @param[in,out] 0 or 1 
 * seek system call
 *
 * @note
 * u_arg system call argument
 */
seek()
{
        /// 32bitの値を使いたい
        /// idiom005
	int n[2];
	register *fp, t;

	fp = getf(u.u_ar0[R0]);
	if(fp == NULL)
		return;
	if(fp->f_flag&FPIPE) {
		u.u_error = ESPIPE;
		return;
	}
	t = u.u_arg[1];
	if(t > 2) {
		n[1] = u.u_arg[0]<<9;
		n[0] = u.u_arg[0]>>7;
		if(t == 3)
			n[0] =& 0777;
	} else {
		n[1] = u.u_arg[0];
		n[0] = 0;
		if(t!=0 && n[1]<0)
			n[0] = -1;
	}
	switch(t) {

	case 1:
	case 4:
		n[0] =+ fp->f_offset[0];
		dpadd(n, fp->f_offset[1]);
		break;

	default:
		n[0] =+ fp->f_inode->i_size0&0377;
		dpadd(n, fp->f_inode->i_size1);

	case 0:
	case 3:
		;
	}
	fp->f_offset[1] = n[1];
	fp->f_offset[0] = n[0];
}

/*
 * link system call
 */
link()
{
	register *ip, *xp;
	extern uchar;

	ip = namei(&uchar, 0);
	if(ip == NULL)
		return;
	if(ip->i_nlink >= 127) {
		u.u_error = EMLINK;
		goto out;
	}
	if((ip->i_mode&IFMT)==IFDIR && !suser())
		goto out;
	/*
	 * unlock to avoid possibly hanging the namei
	 */
	ip->i_flag =& ~ILOCK;
	u.u_dirp = u.u_arg[1];
	xp = namei(&uchar, 1);
	if(xp != NULL) {
		u.u_error = EEXIST;
		iput(xp);
	}
	if(u.u_error)
		goto out;
	if(u.u_pdir->i_dev != ip->i_dev) {
		iput(u.u_pdir);
		u.u_error = EXDEV;
		goto out;
	}
	wdir(ip);
	ip->i_nlink++;
	ip->i_flag =| IUPD;

out:
	iput(ip);
}

/*
 * mknod system call
 */
mknod()
{
	register *ip;
	extern uchar;

	if(suser()) {
		ip = namei(&uchar, 1);
		if(ip != NULL) {
			u.u_error = EEXIST;
			goto out;
		}
	}
	if(u.u_error)
		return;
	ip = maknode(u.u_arg[1]);
	if (ip==NULL)
		return;
	ip->i_addr[0] = u.u_arg[2];

out:
	iput(ip);
}

/*
 * sleep system call
 * not to be confused with the sleep internal routine.
 */
sslep()
{
	char *d[2];

	spl7();
	d[0] = time[0];
	d[1] = time[1];
	dpadd(d, u.u_ar0[R0]);

	while(dpcmp(d[0], d[1], time[0], time[1]) > 0) {
		if(dpcmp(tout[0], tout[1], time[0], time[1]) <= 0 ||
		   dpcmp(tout[0], tout[1], d[0], d[1]) > 0) {
			tout[0] = d[0];
			tout[1] = d[1];
		}
		sleep(tout, PSLEP);
	}
	spl0();
}
