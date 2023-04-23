FROM registry.access.redhat.com/ubi8/ubi

RUN yum -y update && yum -y install \
    autoconf \
    bzip2 \
    gcc \
    gcc-c++ \
    git \
    gzip \
    libtool \
    make \
    perl \
    tar \
    wget


RUN mkdir /openssl && cd /openssl && wget https://www.openssl.org/source/openssl-1.1.1n.tar.gz \
    && tar -zxvf openssl-1.1.1n.tar.gz \
    && cd openssl-1.1.1n \
    && ./config \
    && make \
    && make install


RUN mkdir /zlib && git clone https://github.com/madler/zlib.git /zlib --branch v1.2.13 \
    && cd /zlib \
    && ./configure --static --prefix=/usr/local \
    && make \
    && make install


WORKDIR /tmp
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.24.4/cmake-3.24.4.tar.gz -o cmake-3.24.4.tar.gz \
    && tar -zxvf cmake-3.24.4.tar.gz \
    && cd cmake-3.24.4 \
    && ./bootstrap \
    && make \
    && make install