FROM debian:stretch

# 更新阿里云的wheezy版本包源
COPY sources.list /etc/apt/sources.list

# Install related tools
RUN apt update && \
    apt install gcc g++ mariadb-client -y 
    
RUN apt-get install -y libqt5sql5-mysql
RUN apt-get install -y qt5-default
RUN apt-get install -y make
# RUN apt-get install -y qtcreator

WORKDIR /

COPY /cpps_product_client /

EXPOSE 4000

ENV TZ=Asia/Shanghai

RUN qmake cpps_product_client.pro
RUN make 

RUN chmod +x cpps_product_client 

RUN ln -sf /usr/share/zoneinfo/$TZ /etc/localtime && \ 
    echo $TZ > /etc/timezone

ENV IPAddress 192.168.137.41
ENV IPPort 4844

# Commands when creating a new container
ENTRYPOINT "/cpps_product_client" ${IPAddress} ${IPPort}