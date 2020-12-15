#!/bin/sh
cd /root/aws-iot-device-client/.github/
if [ -n "${ST_COMPONENT_MODE}" ]; then
  chmod +x ./build_st_component.sh
  ./build_st_component.sh
else
  chmod +x ./build.sh
  ./build.sh $1
fi