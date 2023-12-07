FROM public.ecr.aws/amazonlinux/amazonlinux:2.0.20230307.0

ARG OPENSSL_VERSION=3.0.8

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
    softhsm \
    tar \
    perl-IPC-Cmd \
    perl-Pod-Html \
    wget


RUN mkdir /openssl && cd /openssl && wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -zxvf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
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