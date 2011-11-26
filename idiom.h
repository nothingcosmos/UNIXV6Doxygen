///v6 idiom

/// @deprecated
/// - PS->integ はプロセッサの優先度をさす
///  - 何らかの変数に退避しておき、復帰することで、一種のmutexを実現する
#define idiom001

/// @deprecated
/// - スワップ前後のコンテキストの退避方法
///  - savu(u.u_ssav)
///  - xswap()
///  - p_flag =| SSWAP
///
/// 復帰する際は
/// - p_flag =& ~SSWAP
/// - aretu(u.u_ssav)
#define idiom002

/// @deprecated
/// - sql6() --> sql0()
/// - 一時的に高い優先度を設定し、割り込みをブロックした状態で処理を行ったのち、sql0の状態に戻し、割り込みを許可する。
/// - 割り込み前の優先度に戻さないのが特徴である
#define idiom003



/// @deprecated
/// - 
#define idiom00
