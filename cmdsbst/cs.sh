data="$(cat /etc/services)"
curl -X POST -d"$data" localhost:9999

# curl -X POST -d"$(cat /etc/passwd)" localhost:9999