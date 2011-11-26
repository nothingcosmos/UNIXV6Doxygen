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
  /// inodeの領域の大きさ
	int	s_isize;	/**< size in blocks of I list */
	int	s_fsize;	/**< size in blocks of entire volume */
	int	s_nfree;	/**< number of in core free blocks (0-100) */
	int	s_free[100];	/**< in core free blocks */
  ///
	int	s_ninode;	/**< number of in core I nodes (0-100) */
	int	s_inode[100];	/**< in core free I nodes */
  /// 対応するinodeを操作するためのlock
	char	s_flock;	/**< lock during free list manipulation */
	char	s_ilock;	/**< lock during I list manipulation */
	char	s_fmod;		/**< super block modified flag */
	char	s_ronly;	/**< mounted read-only flag */
	int	s_time[2];	/**< current date of last update */
	int	pad[50];
};
