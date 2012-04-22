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
/// - inodeは、ls -i /でディレクトリごとにinodeを出力できる
#define idiom004

/// @deprecated
/// - n[] 32bitの値を使いたい場合に使用する
#define idiom005

/// @deprecated
/// -  system callに関しては、trapが呼ばれた後に限り、kernel空間でsystem callが実行されるため、
///    system call間での競合はない。
/// - 上のほうのレイヤーであるbufの場合、bufとinodeの競合の可能性があり、同時に触られる可能性があるため、
///   bufのほうでは、優先度をあげてlockをかけてフラグ操作を行っていた。 
#define idiom006

/// @deprecated
/// - inoとinodeが重複しているような気がする
/// - i_mode         i_nlink
/// - i_flag i_count i_dev
#define idiom007

/// @deprecated
/// - alloc()でErrorが発生する可能性があるが、nmapから呼び出している。
/// - nmapで発生したエラーのハンドリングが正常に終わっているか不明
#define idiom008

/// @deprecated
/// - 
#define idiom00



2012/04/22


ハードウェアは、ﾛｼﾞｱﾅで01をみて、でばっぐする。
波形が綺麗な四角か
ハードウェア屋のprintfに相当


Q1 リダイレクトは誰がどこでやっているのか。

リダイレクトは、パイプで 1.outと 2.inをつなげる。


rootdev <-- /の存在するブロックデバイスのこと

pipeは、int pipe(fd[2])
  fd[0]が、pipeの出力
  fd[1]が、pipeの入力

pipeつくって、forkするのがポイントなのか

pipe事態は、リング上に入出力先が作られたpipeが生成されて、
そのpipeのfiledescripterをfork execしてコピーして、
親子間でつなぎ替えを行って作る。

shellの|は、pipeを作ってから、それぞれのプロセスを並列に作って、
そのプロセス間の入出力をバッファリングされたpipeにつなぎ替えている。


stdioの使用として、
ttyが出力の場合、1文字ずつ出力
file立った場合、bufferをだす。

ioctrlで判断している。


dmaコントローラーで,cpuではなく、独自コピーする

pipeを使う限り、オンメモリで処理されるので速いはず
