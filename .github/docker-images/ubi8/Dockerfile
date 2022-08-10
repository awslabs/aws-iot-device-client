FROM registry.redhat.io/ubi8/ubi

ARG OPENSSL_VERSION=1.1.1n

###############################################################################
# Install prereqs
###############################################################################
RUN subscription-manager config --rhsm.manage_repos=0
RUN yum --disableplugin=subscription-manager -y update \
    && yum --disableplugin=subscription-manager -y install \
    tar \
    bzip2 \
    git \
    wget \
    curl \
    sudo \
    make \
    gcc \
    gcc-c++ \
    && yum clean all \
    && rm -rf /var/cache/yum

###############################################################################
# Install cppcheck
# cppcheck package is not available from Redhat Linux default repositories
# Install and enable epel repository.
# https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/add-repositories.html
###############################################################################
RUN yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
RUN yum --enablerepo=epel install -y cppcheck

###############################################################################
# Install pre-built CMake
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.10.0/cmake-3.10.0.tar.gz -o cmake-3.10.0.tar.gz \
    && tar -zxvf cmake-3.10.0.tar.gz \
    && cd cmake-3.10.0 \
    && ./bootstrap \
    && make \
    && make install

###############################################################################
# Install OpenSSL 1.1.1
###############################################################################
WORKDIR /tmp
RUN wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -zxvf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./config \
    && make \
    && make install

###############################################################################
# Install softhsm v2.3.0 from source
###############################################################################

WORKDIR /tmp
RUN wget https://dist.opendnssec.org/source/softhsm-2.3.0.tar.gz \
    && tar -xzf softhsm-2.3.0.tar.gz \
    && cd softhsm-2.3.0 \
    && ./configure --disable-gost \
    && make \
    && make install

###############################################################################
# Clone and build Google Test
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/google/googletest/archive/release-1.10.0.tar.gz -o release-1.10.0.tar.gz \
    && tar xf release-1.10.0.tar.gz \
    && cd googletest-release-1.10.0 \
    && cmake -DBUILD_SHARED_LIBS=ON . \
    && make \
    && cp -a googletest/include/gtest /usr/include/ \
    && cp -a googlemock/include/gmock /usr/include/ \
    && cp -a lib/* /usr/lib64/ \
    && rm -f /tmp/release-1.10.0.tar.gz

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

################################################################################
# Install Aws Iot Device Sdk Cpp v2
################################################################################
WORKDIR /home/aws-iot-device-client
RUN mkdir sdk-cpp-workspace \
    && cd sdk-cpp-workspace \
    && git clone https://github.com/aws/aws-iot-device-sdk-cpp-v2.git \
    && cd aws-iot-device-sdk-cpp-v2 \
    && git checkout ac3ba3774b031dde1b988e698880d6064d53b9d9 \
    && git submodule update --init --recursive \
    && cd .. \
    && mkdir aws-iot-device-sdk-cpp-v2-build \
    && cd aws-iot-device-sdk-cpp-v2-build \
    && cmake -DCMAKE_INSTALL_PREFIX="/usr" -DUSE_OPENSSL=ON -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2 \
    && cmake --build . --target install

ADD entry-script.sh /home/entry-script
RUN chmod a+x /home/entry-script
ENTRYPOINT ["/home/entry-script"]
