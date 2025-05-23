ARG OS
ARG BASE_IMAGE
ARG PRE_RELEASE_IMAGE
FROM ${OS} AS build_localproxy

ARG OPENSSL_VERSION=3.0.8

###############################################################################
# Install prereqs for CMake
###############################################################################
RUN apt-get update -qq \
    && apt-get -y install \
    git \
    clang-format \
    curl \
    build-essential \
    wget \
    libc6-dbg \
    softhsm \
    zlib1g-dev \
    && apt-get clean


###############################################################################
# Install OpenSSL 3.0.8
###############################################################################
WORKDIR /tmp
RUN wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -zxvf openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./config \
    && make \
    && make install \
    && ldconfig

###############################################################################
# Install pre-built CMake
###############################################################################
WORKDIR /tmp
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0.tar.gz -o cmake-3.28.0.tar.gz \
    && tar -zxvf cmake-3.28.0.tar.gz \
    && cd cmake-3.28.0 \
    && ./bootstrap \
    && make -j 2 \
    && make install

###############################################################################
# Install Prerequisites
###############################################################################

RUN apt update && apt upgrade -y && \
	apt install -y git libboost-all-dev autoconf automake \
	wget libtool curl make g++ unzip cmake libssl-dev

###############################################################################
# Install Dependencies
###############################################################################

RUN mkdir /home/dependencies
WORKDIR /home/dependencies

RUN wget https://zlib.net/zlib-1.3.1.tar.gz -O /tmp/zlib-1.3.1.tar.gz && \
	tar xzvf /tmp/zlib-1.3.1.tar.gz && \
	cd zlib-1.3.1 && \
	./configure && \
	make && \
	make install && \
	cd /home/dependencies

RUN wget https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz -O /tmp/boost.tar.gz && \
	tar xzvf /tmp/boost.tar.gz && \
	cd boost_1_87_0 && \
	./bootstrap.sh && \
	./b2 install link=static && \
	cd /home/dependencies

RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.17.3/protobuf-all-3.17.3.tar.gz -O /tmp/protobuf-all-3.17.3.tar.gz && \
	tar xzvf /tmp/protobuf-all-3.17.3.tar.gz && \
	cd protobuf-3.17.3 && \
	mkdir build && \
	cd build && \
	cmake ../cmake && \
	make && \
	make install && \
	cd /home/dependencies

RUN git clone --branch v2.13.6 https://github.com/catchorg/Catch2.git && \
	cd Catch2 && \
	mkdir build && \
	cd build && \
	cmake ../ && \
	make && \
	make install && \
	cd /home/dependencies

RUN git clone https://github.com/aws-samples/aws-iot-securetunneling-localproxy && \
	cd aws-iot-securetunneling-localproxy && \
    git checkout 8557d5faf5fe21fbf8ddbc82e96840d8fe985cef && \
	mkdir build && \
	cd build && \
	cmake ../ && \
	make

RUN mkdir -p /home/aws-iot-securetunneling-localproxy && \
	cd /home/aws-iot-securetunneling-localproxy && \
	cp /home/dependencies/aws-iot-securetunneling-localproxy/build/bin/* /home/aws-iot-securetunneling-localproxy/

RUN rm -rf /home/dependencies

WORKDIR /home/aws-iot-securetunneling-localproxy/

FROM ${BASE_IMAGE} AS build_integration_tests

RUN apt-get update && apt-get install -y uuid-dev zlib1g-dev libpulse-dev

COPY ./integration-tests /root/integration-tests
RUN mkdir -p /root/integration-tests/build \
    && cd /root/integration-tests/build \
    && cmake .. -DCMAKE_BUILD_TYPE=DEBUG \
    && cmake --build . --target aws-iot-device-client-integration-tests

###############################################################################
# Actual docker image
###############################################################################

FROM ${PRE_RELEASE_IMAGE} AS runner
RUN apt-get update && apt-get install -y libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev libpulse-dev wget ssh ca-certificates wget apt-utils

RUN mkdir -p /home/aws-iot-securetunneling-localproxy/certs && \
    cd /home/aws-iot-securetunneling-localproxy/certs && \
    wget https://www.amazontrust.com/repository/AmazonRootCA1.pem && \
	openssl rehash ./
###############################################################################
# Copy the binaries from builder stages.
###############################################################################

COPY --from=build_localproxy /home/aws-iot-securetunneling-localproxy/localproxy /localproxy
COPY --from=build_integration_tests /root/integration-tests/build/aws-iot-device-client-integration-tests /aws-iot-device-client-integration-tests
COPY --from=build_integration_tests /root/integration-tests/source/tunneling/test-tunnel.sh /test-tunnel.sh
COPY --from=build_integration_tests /root/integration-tests/entry-point.sh /entry-point.sh

COPY ./.github/docker-images/oss-compliance /root/oss-compliance
RUN HOME_DIR=/root \
    && chmod +x ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh \
    && chmod +x ${HOME_DIR}/oss-compliance/test/test-oss-compliance.sh \
    && bash ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh ${HOME_DIR} \
    && rm -rf ${HOME_DIR}/oss-compliance*

RUN chmod a+x /entry-point.sh
RUN chmod a+x /test-tunnel.sh
RUN chmod 700 /root/.aws-iot-device-client/jobs/*.sh

RUN ssh-keygen -f ~/.ssh/id_rsa -N ""
RUN cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

ENTRYPOINT ["/entry-point.sh"]
