#!/bin/env bash

set -eu

cd ..
source env.init

N=30
i=1

while [[ $i -le $N ]]; do
    printf "=========== $i/$N ===========\n"
    timeout 10 \
    pin-3.20-98437-gf02b61307-gcc-linux/pin \
        -follow-execv \
        -t src/obj-intel64/dta-sh.so \
        -- bash "obfuscation/out/$i.sh" | true
    printf "\n\n\n"

    i=$((i+1))
done

