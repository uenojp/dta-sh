#!/bin/bash

cat /etc/passwd | curl -X POST -d @- localhost:9999

# curl -X POST -d "$(cat /etc/passwd)" localhost:9999

# data="$(cat /etc/passwd)"
# curl -X POST -d "$data" localhost:9999

# seq 100 | curl -X POST -d @- localhost:9999
