#!/bin/bash

ID=$(printf '%05d' $RANDOM)
strace -e execve,pipe2,openat,read,readv,write,writev,close,wait4,dup2,clone,fork,vfork,socket,connect,sendto,recvfrom,fcntl -o "/tmp/$ID" -ttt -ff sh cs.sh

(
    for output in "/tmp/$ID"*; do
        pid="${output:11}"
        < "$output" sed "s/^/[pid $pid] /"
        rm "$output"
    done
) | sort -k3,3n
