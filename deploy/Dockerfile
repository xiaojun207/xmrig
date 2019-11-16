FROM  alpine:latest

RUN   adduser -S -D -H -h /xmrig miner

RUN   apk --no-cache add \
        git \
        cmake \
        libuv-dev \
        build-base \
        openssl-dev \
        libmicrohttpd-dev

RUN apk --no-cache add \
    --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing \
    hwloc-dev

RUN git clone https://github.com/xiaojun207/xmrig

RUN cd xmrig && \
    mkdir build && \
      cmake -DCMAKE_BUILD_TYPE=Release . && \
      make &&  rm -rf /xmrig/src

RUN apk del build-base cmake git

#COPY startup.sh /xmrig/startup.sh
COPY  config.json /xmrig/config.json
RUN  chown miner /xmrig /xmrig/config.json
USER miner
WORKDIR    /xmrig
ENTRYPOINT  ["./xmrig"]
