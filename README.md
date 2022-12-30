# 動的テイント解析を用いた悪性シェルスクリプトによる情報漏洩の検知

curlを用いたPOSTリクエストによる情報漏洩を動的テイント解析ライブラリである[libdft64](https://github.com/AngoraFuzzer/libdft64)を用いて検知・阻止する。

## クイックスタート

### 依存関係をインストール

```
./setup.sh
```

### PIN_ROOTを設定

```
source ./env.init
```

### dta-shの例を実行

```
python3 server.py &
cd src
make
make test
```

## FAQ
エラーに関するものは起きたエラーがそのままタイトル。

### Makefile:18: /Config/makefile.default.rules: No such file or directory

[`PIN_ROOT`](#PIN_ROOTとは)が設定されておらず、PinのMakefileが見つからない。
Pinのディレクトリを環境変数経由で教えてやる。

```
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
