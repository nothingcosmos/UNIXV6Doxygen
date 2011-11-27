#define filsys
/**
 * @brief
 * Definition of the unix super block.
 * The root super block is allocated and
 * read in iinit/alloc.c. Subsequently
 * a super block is allocated and read
 * with each mount (smount/sys3.c) and
 * released with unmount (sumount/sys3.c).
 * A disk block is ripped off for storage.
 * See alloc.c for general alloc/free
 * routines for free list and I list.
 * @note
 * - super block
 * - filsysの先頭がOSに読み込まれるタイミングはいつか???
 *   mountコマンドで。ディスクの先頭にあるsuperblockを読み込んで、
 *   ファイルシステムとして読み込めるように構成する
 * - incore inodeの契機はいつか
 *   open時
 */
struct	filsys
{
#define s_isize
        /// inodeの領域の大きさ
        /// size in blocks of I list */
	int	s_isize;
#define s_isize
        /// size in blocks of entire volume */
	int	s_fsize;
#define s_isize
        /// number of in core free blocks (0-100) */
	int	s_nfree;
#define s_isize
        /// in core free blocks */
	int	s_free[100];
#define s_isize
        /// number of in core I nodes (0-100) */
	int	s_ninode;
#define s_isize
        /// in core free I nodes */
	int	s_inode[100];
#define s_isize
        /// lock during free list manipulation */
        /// 対応するinodeを操作するためのlock
	char	s_flock;
#define s_isize
        /// lock during I list manipulation */
	char	s_ilock;
#define s_isize
        /// super block modified flag */
	char	s_fmod;
#define s_isize
        /// mounted read-only flag */
	char	s_ronly;
#define s_isize
        /// current date of last update */
	int	s_time[2];
#define s_isize
	int	pad[50];
};
