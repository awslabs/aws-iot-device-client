#!/bin/sh
cd /root/aws-iot-device-client/.github/

compileModeArgument=$(echo "$1" | cut -c3-14)
if [ "$compileModeArgument" = "compile-mode" ]; then
  compileMode=$(echo "$1" | cut -d "=" -f2-)
  case $compileMode in
    st_component_mode)
    chmod +x ./build_st_component.sh
    ./build_st_component.sh
    ;;
    *)
    echo "No compile mode match found"
    ;;
  esac
else
  chmod +x ./build.sh
  ./build.sh "$1"
fi