/**
 * KT-11 addresses and bits.
 * @note
 * - 仮想アドレス空間の最高位のページは、
 *   物理アドレス空間の最高位のページにマップ
 * - 0160000-017777 --> 0760000 - 0777777
 * @note
 * - UISD
 *  - [in]
 *  - [out] main()
 *  - [in,out]
 * - UISA
 *  - [in]
 *  - [out] main()
 *  - [in,out] sureg()
 * - UDSA
 *  - [in] bio.c:physio()
 * - RO
 *  - [in] estabur()
 * - WO
 *  - [in] sureg()
 * - RW
 *  - [in] estabur()
 * - ED
 *  - [in] estabur()
 */

#define	UISD	0177600		/**< first user I-space descriptor register */
#define	UISA	0177640		/**< first user I-space address register */
#define	UDSA	0177660		/**< first user D-space address register */
#define	RO	02		/**< access abilities */
#define	WO	04
#define	RW	06
#define	ED	010		/**< extend direction */

/**
 * structure used to address
 * a sequence of integers.
 */
struct
{
	int	r[];
};
int	*ka6;		/**< 11/40 KISA6; 11/45 KDSA6 */

/**
 * address to access 11/70 UNIBUS map
 */
#define	UBMAP	0170200
