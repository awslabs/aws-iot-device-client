#!/bin/sh

### Compile ###
cd ..
ln -s /home/sdk-cpp-workspace ./sdk-cpp-workspace
if [ ! -d "./build" ]; then
  mkdir ./build
fi
cd ./build/ || exit 1

echo "Building ST component mode."
cmake ../ -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON

cmake --build . --target aws-iot-device-client
if [ $? != 0 ]
then
  exit 1
fi
cmake --build . --target test-aws-iot-device-client

### Run Tests ###
./test/test-aws-iot-device-client
