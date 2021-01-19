#!/bin/sh

compileMode="default"
stMode=false

# Check if first argument is compile mode
compileModeArgument=$(echo "$1" | cut -c3-14)
if [ "$compileModeArgument" = "compile-mode" ]; then
  compileModeTmp=$(echo "$1" | cut -d "=" -f2-)
  case $compileModeTmp in
    st_component_mode)
    compileMode="st_component_mode"
    ;;
    armhf_cross_mode)
    compileMode="armhf_cross_mode"
    ;;
    mips_cross_mode)
    compileMode="mips_cross_mode"
    ;;
    aarch64_cross_mode)
    compileMode="aarch64_cross_mode"
    ;;
    st_armhf_cross_mode)
    compileMode="armhf_cross_mode"
    stMode=true
    ;;
    st_aarch64_cross_mode)
    compileMode="aarch64_cross_mode"
    stMode=true
    ;;
    *)
    echo "No compile mode match found"
    ;;
  esac
else
  # Compiler name, g++ or clang, just need the first character
  compilerName=$(echo $1 | cut -c12-12)
  # Everything that follows the first character
  compilerExec=$(echo $1 | cut -c12-20)
  echo $compilerName
  echo $compilerExec
  if [ $# -eq 1 ]
    then
      apt-get update
      apt-get install --assume-yes software-properties-common
      # Install correct compiler from CI matrix from (APT)
      if [ $compilerName = "g" ]
        then
          # Cuts to just the version ie, g++-{7}
          ver=$(echo $compilerExec | cut -c5-20)
          echo "ver"
          echo $ver
          add-apt-repository ppa:ubuntu-toolchain-r/test -y
          apt-get update
          apt-get --assume-yes install g++-$ver

          export CXX=/usr/bin/g++-$ver
      fi
      if [ $compilerName = "c" ]
        then
          # Cuts to just the version ie, clang-{7}
          ver=$(echo $compilerExec | cut -c7-20)
          if [ $ver = "5" ] || [ $ver = "6" ]
            then
              ver=$(echo "$ver.0")
          fi
          echo "ver"
          echo $ver
          apt-get --assume-yes install apt-transport-https
          wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add
          apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-$ver main" -y
          apt-get update
          apt-get --assume-yes install clang-$ver
          export CXX=/usr/bin/clang++-$ver
      fi
  fi
fi
### Compile ###
cd ..
ln -s /home/sdk-cpp-workspace ./sdk-cpp-workspace
if [ ! -d "./build" ]; then
  mkdir ./build
fi
cd ./build/

case $compileMode in
    st_component_mode)
    echo "Building in ST component mode"
    cmake ../ -DBUILD_SDK=ON -DBUILD_TEST_DEPS=OFF -DLINK_DL=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON
    ;;
    armhf_cross_mode)
    apt-get update
    apt-get install --assume-yes software-properties-common
    apt-get install --assume-yes build-essential
    apt-get install --assume-yes g++-arm-linux-gnueabihf
    apt-get install --assume-yes gcc-arm-linux-gnueabihf
    apt-get install --assume-yes gdb-multiarch
    wget https://www.openssl.org/source/openssl-1.1.1.tar.gz
    tar -xvzf openssl-1.1.1.tar.gz
    export INSTALL_DIR=/usr/lib/arm-linux-gnueabihf
    cd openssl-1.1.1
    ./Configure linux-generic32 shared \
      --prefix=$INSTALL_DIR --openssldir=$INSTALL_DIR/openssl \
      --cross-compile-prefix=/usr/bin/arm-linux-gnueabihf-
    make depend
    make -j 4
    make install
    cd ..
    if [ "$stMode" = true ]; then
      # Set CMake flags for ST mode
      # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../ || true
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../
    else
      # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake ../ || true
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake ../
    fi
    cmake --build . --target aws-iot-device-client
    cmake --build . --target test-aws-iot-device-client
    exit $?
    ;;
    #################################
    mips_cross_mode)
    apt-get update
    apt-get install --assume-yes software-properties-common
    apt-get install --assume-yes build-essential
    apt-get install --assume-yes g++-mips-linux-gnu
    apt-get install --assume-yes gcc-mips-linux-gnu
    apt-get install --assume-yes gdb-multiarch
    wget https://www.openssl.org/source/openssl-1.1.1.tar.gz
    tar -xvzf openssl-1.1.1.tar.gz
    export INSTALL_DIR=/usr/lib/mips-linux-gnu
    cd openssl-1.1.1
    ./Configure linux-mips32 shared \
      --prefix=$INSTALL_DIR --openssldir=$INSTALL_DIR/openssl \
      --cross-compile-prefix=/usr/bin/mips-linux-gnu-
    make depend
    make -j 4
    make install
    cd ..
    # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-mips.cmake ../ || true
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-mips.cmake ../
    cmake --build . --target aws-iot-device-client
    cmake --build . --target test-aws-iot-device-client
    exit $?
    ;;
    #################################
    aarch64_cross_mode)
    apt-get update
    apt-get install --assume-yes software-properties-common
    apt-get install --assume-yes build-essential
    apt-get install --assume-yes g++-aarch64-linux-gnu
    apt-get install --assume-yes gcc-aarch64-linux-gnu
    apt-get install --assume-yes gdb-multiarch
    wget https://www.openssl.org/source/openssl-1.1.1.tar.gz
    tar -xvzf openssl-1.1.1.tar.gz
    export INSTALL_DIR=/usr/lib/aarch64-linux-gnu
    cd openssl-1.1.1
    ./Configure linux-aarch64 shared \
      --prefix=$INSTALL_DIR --openssldir=$INSTALL_DIR/openssl \
      --cross-compile-prefix=/usr/bin/aarch64-linux-gnu-
    make depend
    make -j 4
    make install
    cd ..
    if [ "$stMode" = true ]; then
      # Set CMake flags for ST mode
      # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../ || true
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../
    else
      # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake ../ || true
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake ../
    fi
    cmake --build . --target aws-iot-device-client
    cmake --build . --target test-aws-iot-device-client
    exit $?
    ;;
    *)
    cmake ../ -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF -DLINK_DL=ON
    ;;
esac

cmake --build . --target aws-iot-device-client
if [ $? != 0 ]
then
  exit 1
fi
cmake --build . --target test-aws-iot-device-client

### Run Tests ###
./test/test-aws-iot-device-client
