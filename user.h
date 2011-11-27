#define user
/**
 * @brief
 * The user structure.
 * One allocated per process.
 * Contains all per process data
 * that doesn't need to be referenced
 * while the process is swapped.
 * The user block is USIZE*64 bytes
 * long; resides at virtual kernel
 * loc 140000; contains the system
 * stack per user; is cross referenced
 * with the proc structure for the
 * same process.
 */
struct user
{
#define u_rsav
        /// アクティブなプロセス
        /// r5とr6をsavuでこの領域に退避
        /// swtch() newproc() expand() xmalloc()
        /// において退避する
        /// save r5,r6 when exchanging stacks
	int	u_rsav[2];
#define u_fsav
        /// save fp registers
	/// rsav and fsav must be first in structure
	int	u_fsav[25];
#define u_segflg
        /// flag for IO; user or kernel space
        /// - rdwrで0に初期化する
	char	u_segflg;
#define u_error
        /// return error code
	char	u_error;
#define u_uid
        /// effective user id
	char	u_uid;
#define u_gid
        /// effective group id
	char	u_gid;
#define u_ruid
        /// real user id
	char	u_ruid;
#define u_rgid
        /// real group id
	char	u_rgid;
#define u_procp
        /// pointer to proc structure
        /// :Gtags proc
	int	u_procp;
#define u_base
        /// - rdwrで書き換える
        /// base address for IO
	char	*u_base;
#define u_count
        /// - rdwrで書き換える
        /// bytes remaining for IO
	char	*u_count;
#define u_offset
        /// - rdwrで書き換える
        /// offset in file for IO
	char	*u_offset[2];
#define u_cdir
        /// pointer to inode of current directory
	int	*u_cdir;
#define u_dbuf
        /// current pathname component
	char	u_dbuf[DIRSIZ];
#define u_dirp
        /// current pointer to inode
	char	*u_dirp;
#define u_dent
        /// current directory entry
	struct	{
#define u_ino
		int	u_ino;
#define u_name
		char	u_name[DIRSIZ];
	} u_dent;
#define u_pdir
        /// inode of parent directory of dirp
	int	*u_pdir;
#define u_uisa
        /// prototype of segmentation addresses */

        /// page address registerに対するprototypeを格納する
        /// 2word * 8 ???
        /// in sureg()
        /// in,out estabur()
	int	u_uisa[16];
#define u_uisd
        /// prototype of segmentation descriptors

        /// page discriptor registerに対するprototypeを格納する
        /// 2word * 8 ???
        /// in sureg()
        /// int,out estabur()
	int	u_uisd[16];
#define u_ofile
        /// pointers to file structures of open files 
        /// 0 stdin
        /// 1 stdout
        /// 2 stderr
        /// 3 新しいファイル
        /// 初期値では、15で最大
	int	u_ofile[NOFILE];
#define u_arg
        /// arguments to current system call */
	int	u_arg[5];
#define u_tsize
        /// text size (*64) 
        /// text segment size を規定するパラメータ
	int	u_tsize;
#define u_dsize
        /// data size (*64)
        /// data segment size を規定するパラメータ
	int	u_dsize;
#define u_ssize
        /// stack size (*64)
        /// stack segment size を規定するパラメータ
	int	u_ssize;
#define u_sep
        /// flag for I and D separation
	int	u_sep;
#define u_qsav
        /// @brief label variable for quits and interrupts
        /// savu()するのがtrap1()
        /// aretu()するのがsleep()
        ///
        /// trap1()でtrap用の関数ポインタを呼び出す際に退避しておく
        /// trap1()でsavu()されてたレジスタをsleep()で起こす
	int	u_qsav[2];
#define u_ssav
        /// label variable for swapping
        /// idiom002を参照
	int	u_ssav[2];
#define u_signal
        /// disposition of signals
	int	u_signal[NSIG];
#define u_utime
        /// this process user time 
	int	u_utime;
#define u_stime
        /// this process system time 
	int	u_stime;
#define u_cutime
        /// sum of childs' utimes 
	int	u_cutime[2];
#define u_cstime
        /// sum of childs' stimes 
	int	u_cstime[2];
#define u_ar0
        /// address of users saved R0 
	int	*u_ar0;
#define u_prof
        /// profile arguments 
	int	u_prof[4];
#define u_intflg
        /// catch intr from sys
	/// kernel stack per user
	/// extends from u + USIZE*64
	/// backward not to reach here
        ///
        /// trap()とtrap1()から参照
        /// set  trap1() 0
        ///      trap1() 1
        /// ref  trap()
	char	u_intflg;
} u;

/** u_error codes */
#define	EFAULT	106
#define	EPERM	1
#define	ENOENT	2
#define	ESRCH	3
#define	EINTR	4
#define	EIO	5
#define	ENXIO	6
#define	E2BIG	7
#define	ENOEXEC	8
#define	EBADF	9
#define	ECHILD	10
#define	EAGAIN	11
#define	ENOMEM	12
#define	EACCES	13
#define	ENOTBLK	15
#define	EBUSY	16
#define	EEXIST	17
#define	EXDEV	18
#define	ENODEV	19
#define	ENOTDIR	20
#define	EISDIR	21
#define	EINVAL	22
#define	ENFILE	23
#define	EMFILE	24
#define	ENOTTY	25
#define	ETXTBSY	26
#define	EFBIG	27
#define	ENOSPC	28
#define	ESPIPE	29
#define	EROFS	30
#define	EMLINK	31
#define	EPIPE	32


//dummy for gtags
/// @brief R5とR6の値を、パラメータとして受け取ったアドレスにある配列に格納する
///
#define savu

/// @brief 7番目のカーネルセグメンテーションアドレスレジスタを再設定し、
///        r6とR5を再設定する
///
/// procのp_addrをpcに設定する際に仕様する
#define retu

/// @brief パラメータとして渡されたアドレスからR6とR5を再ロードする
#define aretu
