FROM alpine AS build-env
RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories
RUN apk update\
    && apk add --no-cache mariadb-dev cmake g++ make

WORKDIR /

COPY /cpps_product_client /

EXPOSE 4000

ENV TZ=Asia/Shanghai

RUN cmake .
RUN make 

RUN ln -sf /usr/share/zoneinfo/$TZ /etc/localtime && \ 
    echo $TZ > /etc/timezone

ENV IPAddress 192.168.137.41
ENV IPPort 4844

# Commands when creating a new container
ENTRYPOINT "/cpps_product_client" ${IPAddress} ${IPPort}