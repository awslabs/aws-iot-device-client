FROM registry.redhat.io/ubi8/ubi

###############################################################################
# Install prereqs
###############################################################################
RUN subscription-manager config --rhsm.manage_repos=0
RUN yum --disableplugin=subscription-manager -y update \
    && yum --disableplugin=subscription-manager -y install \
    tar \
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
RUN curl -sSL https://github.com/google/googletest/archive/release-1.10.0.tar.gz -o release-1.10.0.tar.gz \
    && tar xf release-1.10.0.tar.gz \
    && cd googletest-release-1.10.0 \
    && cmake -DBUILD_SHARED_LIBS=ON . \
    && make \
    && cp -a googletest/include/gtest /usr/include/ \
    && cp -a googlemock/include/gmock /usr/include/ \
    && cp -a lib/* /usr/lib64/ \
    && rm -f /tmp/release-1.10.0.tar.gz

# ###############################################################################
# # Install Aws Iot Device Sdk Cpp v2
# ###############################################################################
WORKDIR /home/aws-iot-device-client
RUN mkdir sdk-cpp-workspace \
    && cd sdk-cpp-workspace \
    && mkdir aws-iot-device-sdk-cpp-v2-build \
    && git clone --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git \
    && cd aws-iot-device-sdk-cpp-v2 \
    && git checkout 163fdf3bff9cc2a51f5d0aeca3d9356f665583a3 \
    && cd ../aws-iot-device-sdk-cpp-v2-build \
    && cmake -DCMAKE_INSTALL_PREFIX="/usr" -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2 \
    && cmake --build . --target install

ADD entry-script.sh /home/entry-script
RUN chmod a+x /home/entry-script
ENTRYPOINT ["/home/entry-script"]