FROM amazonlinux:2.0.20230307.0 as base

ARG OPENSSL_VERSION=3.0.8

###############################################################################
# Install prereqs
###############################################################################
RUN yum -y update \
    && yum -y install \
    tar \
    bzip2 \
    git \
    wget \
    perl \
    sudo \
    make \
    gcc \
    gcc-c++ \
    softhsm \
    zlib-devel \
    perl-IPC-Cmd \
    perl-Pod-Html \
    && yum clean all \
    && rm -rf /var/cache/yum

###############################################################################
# Install OpenSSL 3.0.8
###############################################################################
WORKDIR /tmp
RUN wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -zxvf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./config \
    && make \
    && sudo make install

# Install pre-built CMake
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.24.4/cmake-3.24.4.tar.gz -o cmake-3.24.4.tar.gz \
        && tar -zxvf cmake-3.24.4.tar.gz \
        && cd cmake-3.24.4 \
        && ./bootstrap \
        && make -j 4 \
        && make install

###############################################################################
# Clone and build Google Test
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/google/googletest/archive/release-1.12.0.tar.gz -o release-1.12.0.tar.gz \
    && tar xf release-1.12.0.tar.gz \
    && cd googletest-release-1.12.0 \
    && cmake -DBUILD_SHARED_LIBS=ON . \
    && make \
    && cp -a googletest/include/gtest /usr/include/ \
    && cp -a googlemock/include/gmock /usr/include/ \
    && cp -a lib/* /usr/lib64/ \
    && rm -f /tmp/release-1.11.0.tar.gz

###############################################################################
# Clone and build valgrind
###############################################################################
WORKDIR /tmp
RUN wget https://sourceware.org/pub/valgrind/valgrind-3.19.0.tar.bz2 \
    && tar jxvf valgrind-3.19.0.tar.bz2 \
    && cd valgrind-3.19.0 \
    && ./configure \
    && make \
    && make install

###############################################################################
# Install cppcheck
###############################################################################
WORKDIR /tmp
RUN wget https://github.com/danmar/cppcheck/archive/2.9.tar.gz \
    && tar zxvf 2.9.tar.gz \
    && cd cppcheck-2.9 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install

# ###############################################################################
# # Install Aws Iot Device Sdk Cpp v2
# ###############################################################################
WORKDIR /home/aws-iot-device-client
RUN mkdir sdk-cpp-workspace \
    && cd sdk-cpp-workspace \
    && git clone https://github.com/aws/aws-iot-device-sdk-cpp-v2.git \
    && cd aws-iot-device-sdk-cpp-v2 \
    && git checkout 74c8b683ebe5b1cbf484f6acaa281f56aaa63948 \
    && git submodule update --init --recursive \
    && cd .. \
    && mkdir aws-iot-device-sdk-cpp-v2-build \
    && cd aws-iot-device-sdk-cpp-v2-build \
    && cmake -DCMAKE_INSTALL_PREFIX="/usr" -DUSE_OPENSSL=ON -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2 \
    && cmake --build . --target install

COPY ./.github/docker-images/oss-compliance /root/oss-compliance
RUN HOME_DIR=/root \
    && chmod +x ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh \
    && chmod +x ${HOME_DIR}/oss-compliance/test/test-oss-compliance.sh \
    && bash ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh ${HOME_DIR} \
    && rm -rf ${HOME_DIR}/oss-compliance*

ADD .github/docker-images/entry-script.sh /home/entry-script
RUN chmod a+x /home/entry-script
ENTRYPOINT ["/home/entry-script"]

FROM base AS deploy

###############################################################################
# Copy and build device client
###############################################################################
COPY . /root/aws-iot-device-client
RUN mkdir -p /root/aws-iot-device-client/build \
    && cd /root/aws-iot-device-client/build \
    && cmake .. \
    && cmake --build . --target aws-iot-device-client

ENTRYPOINT ["/root/aws-iot-device-client/build/aws-iot-device-client"]
