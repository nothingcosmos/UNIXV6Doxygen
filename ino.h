#define inode
/**
 * @brief
 * Inode structure as it appears on
 * the disk. Not used by the system,
 * but by things like check, df, dump.
 * @note
 * - disk中のinodeに対応する構造体
 */
struct	inode
{
#define i_mode
	int	i_mode;
#define i_nlink
        /// - 名前空間からの参照の個数
	char	i_nlink;
#define i_uid
	char	i_uid;
#define i_gid
	char	i_gid;
#define i_size0
	char	i_size0;
#define i_size1
	char	*i_size1;
#define i_addr
	int	i_addr[8];
#define i_atime
	int	i_atime[2];
#define i_mtime
	int	i_mtime[2];
};

/* modes */
#define	IALLOC	0100000
#define	IFMT	060000
#define		IFDIR	040000
#define		IFCHR	020000
#define		IFBLK	060000
#define	ILARG	010000
#define	ISUID	04000
#define	ISGID	02000
#define ISVTX	01000
#define	IREAD	0400
#define	IWRITE	0200
#define	IEXEC	0100
