#define file
/**
 * @brief
 * One file structure is allocated
 * for each open/creat/pipe call.
 * Main use is to hold the read/write
 * pointer associated with each open
 * file.
 * @note
 * - newprocで参照を二重化し、参照カウントをインクリメントする
 */
struct	file
{
#define f_flag
        /// - flagsのマクロ参照。read/write/pipe
	char	f_flag;
#define f_count
        /// - reference count
        /// - 0であれば割り当てられていないとみなす
        /// - インクリメントする newproc, dup, falloc
        /// - デクリメントする   closef, open1
	char	f_count;
#define f_inode
        /// - pointer to inode structure
        /// - inodeテーブルのエントリへのポインタ
	int	f_inode;
#define f_offset
        /// - read/write character pointer
        /// - ファイル内への文字への論理ポインタ
	char	*f_offset[2];
} file[NFILE];

/* flags */
#define	FREAD	01
#define	FWRITE	02
#define	FPIPE	04
