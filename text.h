#define text
/**
 * @brief
 * Text structure.
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */
/// @note
/// - x_sizeに関して
///  - リソースマップのsizeと同じ考え方。addrはx_daddrかx_caddrが対応する
/// - x_iptrに関して
///  - x_iptrがNULLかどうかは重要か???
///  - x_iptrは、ファイルシステムを操作する構造体
/// - x_daddrとx_countに関して
///  - textがu.procp->p_textpから参照されている個数をカウントする
///  - xalloc()においてswapmapへ領域確保 x_daddrに値設定 x_countを1で初期化
///  - xfree()においてx_daddrを対象にswapmapの領域開放
/// - x_caddrとx_ccountに関して
///  - coreにloadされているtext領域の参照数をカウントする
///  - sched()においてcoremapへ領域確保(swapin)
///    found2:のブロックでx_caddrhに値設定
///  - xccdec()において、x_caddrを対象にcoremapの領域開放
struct text
{
#define x_daddr
	int	x_daddr;	/**< disk address of segment */
#define x_caddr
	int	x_caddr;	/**< core address, if loaded */
#define x_size
	int	x_size;		/**< size (*64) */
#define x_iptr
	int	*x_iptr;	/**< inode of prototype */
#define x_count
	char	x_count;	/**< reference count */
#define x_ccount
	char	x_ccount;	/**< number of loaded references */
} text[NTEXT];
