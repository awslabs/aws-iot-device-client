#!/usr/bin/env bash

wget https://scan.coverity.com/download/linux64 --post-data "token=$COVERITY_TOKEN&project=aws-iot-device-client" -O coverity_tool.tgz \
&& tar zxvf coverity_tool.tgz --one-top-level=cov --strip-components=1

export PATH=$PATH:/tmp/cov/bin/

rm -rf build
mkdir build
cd build
cmake ..
cov-build --dir cov-int cmake --build . --target aws-iot-device-client
chmod +x aws-iot-device-client
tar czvf aws-iot-device-client.tgz cov-int
ls

curl --form token="$COVERITY_TOKEN" \
  --form email="$COVERITY_EMAIL" \
  --form file=@aws-iot-device-client.tgz \
  --form version="Version" \
  --form description="Description" \
  https://scan.coverity.com/builds?project=aws-iot-device-client
