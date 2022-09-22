#!/usr/bin/env bash

rm -rf build
mkdir build
cd build
cmake ..
cov-build --dir cov-int cmake --build . --target aws-iot-device-client
chmod +x aws-iot-device-client
tar czvf aws-iot-device-client.tgz cov-int
