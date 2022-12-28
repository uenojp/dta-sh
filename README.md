# 動的テイント解析を用いた悪性シェルスクリプトによる情報漏洩の検知

## クイックスタート
### 依存関係をインストール
`./setup.sh`

### PIN_ROOTを設定
`source ./env.init`

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
`PIN_ROOT`が設定されておらず、PinのMakefileが見つからない。
Pinのディレクトリを環境変数経由で教えてやる。

`source env.init`

### dta-sh.cpp:58:15: error: unused variable 'sockfd' [-Werror=unused-variable]
Pintoolのコンパイルに`-Werror`が有効なため、WarningがすべてErrorとして扱われる。
Pintoolのコンパイルオプションは`pin-3.20-98437-gf02b61307-gcc-linux/source/tools/Config/makefile.unix.config`で定義されているため、`-Werror`を削除することでコンパイルが通る（WarningはWarningとして扱われる）。

```diff
- TOOL_CXXFLAGS_NOOPT := -Wall -Werror -Wno-unknown-pragmas -DPIN_CRT=1
+ TOOL_CXXFLAGS_NOOPT := -Wall -Wno-unknown-pragmas -DPIN_CRT=1
```

### likely()/unlikely()
条件分岐を最適化するためにコンパイラに与えるヒント。
`libdft64/src/branch_pred.h`で定義されている。

ref. https://stackoverflow.com/questions/109710/how-do-the-likely-unlikely-macros-in-the-linux-kernel-work-and-what-is-their-ben

### curl: (7) Failed to connect to localhost port 9999 after 468 ms: Connection refused
ローカルサーバが立っていない。
サーバを立ててやる。`&`はバックグラウンド実行の意味（Bashの機能）。

`python3 server.py &`

