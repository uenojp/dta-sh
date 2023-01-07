#/bin/bash

# cat /etc/passwd | curl -X POST -d @- localhost:9999
seq 100 | curl -X POST -d @- localhost:9999
# cat /etc/passwd | nl
