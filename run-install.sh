#!/bin/bash
set -e
# Determine the build platform and specify the correct directories based on that info
if [ "$(uname)" == "Darwin" ]; then
    # Do something under Mac OS X platform
    echo "Detected Mac platform"
    PLATFORM="Mac"
    INSTALL_PATH="/usr/local"
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    # Do something under GNU/Linux platform
    echo "Detected Linux platform"
    PLATFORM="Linux"
    INSTALL_PATH="/usr"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]; then
    # Do something under 32 bits Windows NT platform
    echo "Windows currently unsupported!"
    exit 1
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    # Do something under 64 bits Windows NT platform
    echo "Windows currently unsupported!"
    exit 1
fi

##########################################
# Clone and build the AWS IoT v2 C++ SDK #
#########################################
mkdir sdk-cpp-workspace
cd sdk-cpp-workspace

git clone --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git
mkdir aws-iot-device-sdk-cpp-v2-build
cd aws-iot-device-sdk-cpp-v2-build
cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_PATH}"  -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2
cmake --build . --target install

##########################################
# Clone and build Google Test
##########################################
cd /tmp/
wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz
tar xf release-1.10.0.tar.gz
cd googletest-release-1.10.0
cmake -DBUILD_SHARED_LIBS=ON .
make

cp -a googletest/include/gtest ${INSTALL_PATH}/include/
cp -a googlemock/include/gmock ${INSTALL_PATH}/include/
cp -a lib/* ${INSTALL_PATH}/lib/

if [ ${PLATFORM} == "L" ]; then
  ldconfig -v | grep gtest
fi

