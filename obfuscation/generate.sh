#!/bin/env bash

set -eu

mkdir out

N=30
i=1

while [[ $i -le $N ]]; do
    INFILE='../src/sendfile.sh'
    OUTFILE="out/$i.sh"

    timeout 10 \
    bashfuscator -f "$INFILE" -o "$OUTFILE" \
        -s1 \
        -t1 \
        --no-random-whitespace \
        --test |
        # When curl is executed, the progress of curl is displayed.
        # When progress is displayed, the Bashfuscator test is considered successful.
        # Only those scripts that have been successfully tested by Bashfuscator are left.
        grep '% Total    % Received % Xferd  Average Speed   Time    Time     Time  Current' \
        && i=$((i+1))
done

