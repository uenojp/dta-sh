## コマンド置換の実現方法を調べる

```shell
strace -f sh cs.sh &>! log
```

## NOTE

プロセスが一つしかないとき、straceはpidを省略する。
プロセスを作ったときpid付いているプロセスと今までpid付いていなかったプロセスが同じことがある。
```
# cloneを実行シているプロセスはpid1044648
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7fa8186b3a10) = 1044649
strace: Process 1044649 attached
# cloneされて追跡するプロセスが２つ以上になったので、今まで付けていなかったプロセスのログにpid付ける
[pid 1044648] close(4)                  = 0
[pid 1044648] read(3,  <unfinished ...>
```

これを確認するためには、stdoutに出力されたシェルのpidとログのpidを見比べる。

```check.sh
echo "shell pid: $$"
/bin/echo "current directory $(/bin/pwd)"
```

```
strace -f check.sh > stdout
```

### 動作を確認するために必要そうなsyscallのみ
```
strace -oout -e execve,pipe2,openat,read,readv,write,writev,close,wait4,dup2,clone,fork,vfork,socket,connect,sendto,recvfrom,fcntl -ff sh -c 'seq 10 | factor | awk "NF==2"'
```
