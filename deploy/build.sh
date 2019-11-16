#!/usr/bin/env bash

app="xmrig"
DOCKER_BUILD_TAG="latest"
DOCKER_BASE_REPO="xiaojun207"
docker build -t ${DOCKER_BASE_REPO}/$app:${DOCKER_BUILD_TAG} -f deploy/Dockerfile .

#docker push ${DOCKER_BASE_REPO}/$app:${DOCKER_BUILD_TAG}


# 启动命令
# docker run --rm -it --name test -e password="ice@post.com" xiaojun207/xmrig:latest

# docker run -d -it --name - xmrig xiaojun207/xmrig:latest

# sh ./deploy/build.sh
