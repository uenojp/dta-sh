#!/bin/env bash

bin=~/git/Bashfuscator/bashfuscator/bin/bashfuscator

source ./env.init

cd ./src || exit 1

for i in {1..10}; do
    echo "$i/10"
    "$bin" -t1 -s1 -f ./sendfile.sh -o "./obf-sendfile-$i.sh"

    timeout $((3*60)) /home/ueno/dev/dta-sh/pin-3.20-98437-gf02b61307-gcc-linux/pin   -follow-execv -t ./obj-intel64/dta-sh.so -- bash "obf-sendfile-$i.sh" &> "log-$i"

    [ $? -ne 0 ] && echo timout >> "obf-sendfile-$i.sh"

    
    echo -e "$i/10 done\n\n\n"
    sleep 3
done

