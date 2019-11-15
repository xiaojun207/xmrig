#!/usr/bin/env bash

sudo yum install -y epel-release
sudo yum install -y git make cmake gcc gcc-c++ libstdc++-static libuv-static hwloc-devel openssl-devel

mkdir build && cd build
cmake .. -DUV_LIBRARY=/usr/lib64/libuv.a
make

cd ..

app="xmrig"
DOCKER_BUILD_TAG="latest"
DOCKER_BASE_REPO="xiaojun207"
docker build -t ${DOCKER_BASE_REPO}/$app:${DOCKER_BUILD_TAG} -f deploy/Dockerfile .

#docker push ${DOCKER_BASE_REPO}/$app:${DOCKER_BUILD_TAG}


# 启动命令
# docker run --rm -it --name xmrig -e password="ice@post.com" xiaojun207/xmrig:latest

# docker run -d -it --name - xmrig xiaojun207/xmrig:latest

# sh ./deploy/build.sh
