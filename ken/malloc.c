/*
 */

/**
 * @brief
 * Structure of the coremap and swapmap
 * arrays. Consists of non-zero count
 * and base address of that many
 * contiguous units.
 *
 * @note
 * (The coremap unit is 64 bytes,
 * the swapmap unit is 512 bytes)
 * The addresses are increasing and
 * the list is terminated with the
 * first zero count.
 * @note
 * - mapは、m_addrからm_size分の領域をリソースマップとして管理する
 * - sizeが0である場合、そのmapは尾であり、m_addrは不定である
 * - m_addr+sizeが、後のmapのaddrになることはない。
 *   なぜなら、ぴったり一致する場合、常に前後のmapは後mapへ融合するため
 */
struct map
{
	char *m_size;
	char *m_addr;
};

/**
 * @brief
 * Allocate size units from the given
 * map. Return the base of the allocated
 * space.
 * @retval a 走査して見つけたmapのm_addr
 * @retval 0 指定したサイズを確保できない
 * @note
 * - Algorithm is first fit.
 * - whileの後続条件は無条件一致する。
 *   リソースマップ尾のm_size=0を代入した際に終了。
 * - coremapとswapmap (@ref main) の初期設定に依存して正しく動作する
 * - 基本的な動作として、m_addr=100, m_size=50のmapに対して、
 *   size=20を要求した場合、a=100が返り、元のmapはm_addr=120, m_size=30になる。
 * @par 詳細:
 */
malloc(mp, size)
struct map *mp;
{
	register int a;
	register struct map *bp;

        /// - map->m_sizeが0になるまで、mapをリスト走査する
        ///  - 走査中のmapのm_sizeが引数size以上の場合、
        ///   - 見つけたmapのm_addrを覚えておき、返値とする
        ///   - 見つけたmapのm_addr を size加算
        ///   - 見つけたmapのm_sizeからsize減算、==0だったら、
        ///     (つまりmapの空きリソースにぴったり一致する場合)
        ///    - 見つけたmapから更に走査して、後続の全てのmapを1つ前へ移動する
        ///      (つまりmapのentryは1つ減ることになる)
        ///   - 見つけたm_addrを返す
	for (bp = mp; bp->m_size; bp++) {
		if (bp->m_size >= size) {
			a = bp->m_addr;
			bp->m_addr =+ size;
			if ((bp->m_size =- size) == 0)
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while ((bp-1)->m_size = bp->m_size);
			return(a);
		}
	}
	return(0);
}

/**
 * @brief
 * Free the previously allocated space aa
 * of size units into the specified map.
 * Sort aa into map and combine on
 * one or both ends if possible.
 * @note
 * - mpによって指定されたリソースマップに
 *   アドレスaaからsize分のサイズの領域を返す
 * - 変数aのスコープを狭めるため、else下でのみ宣言されるべきだろう
 * @par 詳細:
 */
mfree(mp, size, aa)
struct map *mp;
{
	register struct map *bp;
	register int t;
	register int a;

        /// - mpを走査して、aa以下のアドレス、かつm_size!=0な条件に合致する
        ///   ものを探す
	a = aa;
	for (bp = mp; bp->m_addr<=a && bp->m_size!=0; bp++);
        /// - 見つかったmapがmpより後続、かつ直前のm_addr+size == aの場合、
        ///   (aがぴったり収まる場合)
        ///  - 直前のmapのm_size += size
        ///  - aa+sizeと直前のmapのm_addrが一致する場合
        ///   - 後続のmpを全て1つ前へずらす
	if (bp>mp && (bp-1)->m_addr+(bp-1)->m_size == a) {
		(bp-1)->m_size =+ size;
		if (a+size == bp->m_addr) {
			(bp-1)->m_size =+ bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
		}
        /// - 条件に合致しない場合
        ///  - aa+sizeと見つかったmapのm_addrが一致し、
        ///    かつ見つかったmapが尾でない場合、
        ///   - 見つかったmapの前へ隣接した領域を処理する
        ///   - m_addrはsize分減算して、m_sizeはsize分加算する
        ///  - sizeが0でない場合
        ///   - 後続のmapを全て1つ後ろへずらす
	} else {
		if (a+size == bp->m_addr && bp->m_size) {
			bp->m_addr =- size;
			bp->m_size =+ size;
		} else if (size) do {
			t = bp->m_addr;
			bp->m_addr = a;
			a = t;
			t = bp->m_size;
			bp->m_size = size;
			bp++;
		} while (size = t);
	}
}
