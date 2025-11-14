FROM fedora:42 AS builder

ARG OPENSSL_VERSION=3.0.8

###############################################################################
# Install prereqs
###############################################################################
RUN dnf -y update \
    && dnf -y install \
    tar \
    bzip2 \
    git \
    wget \
    perl \
    make \
    gcc \
    gcc-c++ \
    zlib-devel \
    perl-IPC-Cmd \
    perl-FindBin \
    && dnf clean all

###############################################################################
# Install OpenSSL 3.0.8 (matching the Amazon Linux image)
###############################################################################
WORKDIR /tmp
RUN wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -zxvf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./config \
    && make -j$(nproc) \
    && make install \
    && ldconfig

###############################################################################
# Install CMake 3.24.4 (matching the Amazon Linux image)
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.24.4/cmake-3.24.4.tar.gz -o cmake-3.24.4.tar.gz \
    && tar -zxvf cmake-3.24.4.tar.gz \
    && cd cmake-3.24.4 \
    && ./bootstrap -- -DCMAKE_USE_OPENSSL=ON \
    && make -j$(nproc) \
    && make install

###############################################################################
# Install Aws Iot Device Sdk Cpp v2 (v1.40.2 - latest)
###############################################################################
WORKDIR /home/aws-iot-device-client
RUN mkdir sdk-cpp-workspace \
    && cd sdk-cpp-workspace \
    && git clone --depth 1 --branch v1.40.2 https://github.com/aws/aws-iot-device-sdk-cpp-v2.git \
    && cd aws-iot-device-sdk-cpp-v2 \
    && git submodule update --init --recursive \
    && cd .. \
    && mkdir aws-iot-device-sdk-cpp-v2-build \
    && cd aws-iot-device-sdk-cpp-v2-build \
    && cmake -DCMAKE_INSTALL_PREFIX="/usr" -DUSE_OPENSSL=ON -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2 \
    && cmake --build . --target install

###############################################################################
# Copy and build device client
###############################################################################
WORKDIR /root/aws-iot-device-client
COPY . .

# Clean any existing build artifacts
RUN rm -rf build && mkdir build

WORKDIR /root/aws-iot-device-client/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. \
    && cmake --build . --target aws-iot-device-client -j$(nproc)

###############################################################################
# Runtime stage - minimal image with just the binary
###############################################################################
FROM fedora:42 AS runtime

# Install runtime dependencies only
RUN dnf -y install openssl && dnf clean all

# Copy the built binary
COPY --from=builder /root/aws-iot-device-client/build/aws-iot-device-client /usr/local/bin/

# Create directories for configs and logs
RUN mkdir -p /etc/.aws-iot-device-client /var/log/aws-iot-device-client

ENTRYPOINT ["/usr/local/bin/aws-iot-device-client"]
