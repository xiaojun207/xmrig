FROM centos

ADD . /app
WORKDIR /app

COPY ./build/xmrig /app/
COPY ./deploy/config.json /app/

ENV PASSWORD="ic"

CMD ["./xmrig", " -p $PASSWORD "]

