FROM ubuntu:16.04

ENV DEBIAN_FRONTEND=noninteractive

###############################################################################
# Install prereqs
###############################################################################
RUN apt-get update -qq \
    && apt-get -y install \
    git \
    curl \
    build-essential \
    wget \
    && apt-get clean

###############################################################################
# Install OpenSSL 1.1.1
###############################################################################
WORKDIR /tmp
RUN wget https://www.openssl.org/source/openssl-1.1.1i.tar.gz \
    && tar -zxvf openssl-1.1.1i.tar.gz \
    && cd openssl-1.1.1i.tar.gz \
    && ./config \
    && make \
    && sudo make install \
    && sudo ln -s /usr/local/bin/openssl /usr/bin/openssl

###############################################################################
# Install pre-built CMake
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.10.0/cmake-3.10.0.tar.gz -o cmake-3.10.0.tar.gz \
    && tar -zxvf cmake-3.10.0.tar.gz \
    && cd cmake-3.10.0 \ 
    && ./bootstrap \
    && make -j 4 \
    && make install

###############################################################################
# Clone and build Google Test
###############################################################################
WORKDIR /tmp
RUN wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz \
    && tar xf release-1.10.0.tar.gz \
    && cd googletest-release-1.10.0 \
    && cmake -DBUILD_SHARED_LIBS=ON . \
    && make \
    && cp -a googletest/include/gtest /usr/include/ \
    && cp -a googlemock/include/gmock /usr/include/ \
    && cp -a lib/* /usr/lib/

###############################################################################
# Install Aws Iot Device Sdk Cpp v2
###############################################################################
WORKDIR /home/aws-iot-device-client
RUN mkdir sdk-cpp-workspace \
    && cd sdk-cpp-workspace \
    && mkdir aws-iot-device-sdk-cpp-v2-build \
    && git clone --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git \
    && cd aws-iot-device-sdk-cpp-v2 \
    && git checkout 62516d506a4ae8b97a50eb240f8da57be43cb4f1 \
    && cd ../aws-iot-device-sdk-cpp-v2-build \
    && cmake -DCMAKE_INSTALL_PREFIX="/usr" -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2 \
    && cmake --build . --target install

ADD entry-script.sh /home/entry-script
RUN chmod a+x /home/entry-script
ENTRYPOINT ["/home/entry-script"]
