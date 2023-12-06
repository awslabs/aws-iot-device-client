FROM ubuntu:18.04

ARG OPENSSL_VERSION=1.1.1n

RUN apt-get update && apt-get install -y \
    wget \
    git \
    build-essential \
    autoconf \
    build-essential \
    cmake \
    libtool \
    zlib1g-dev

RUN mkdir /openssl && cd /openssl && wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -zxvf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./config \
    && make \
    && make install

RUN mkdir /curl && git clone https://github.com/curl/curl.git /curl --branch curl-8_0_1 \
        && cd /curl \
        && autoreconf -fi \
        && ./configure --disable-shared --with-openssl=/usr/local \
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

