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
	char	f_flag;
	char	f_count;	/**< reference count */
	int	f_inode;	/**< pointer to inode structure */
	char	*f_offset[2];	/**< read/write character pointer */
} file[NFILE];

/* flags */
#define	FREAD	01
#define	FWRITE	02
#define	FPIPE	04
