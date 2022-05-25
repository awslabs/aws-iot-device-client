#!/bin/sh

compileMode="default"
stMode=false
sharedLibs=false
OPENSSL_VERSION=1.1.1n

# Check if first argument is compile mode
compileModeArgument=$(echo "$1" | cut -c3-14)
if [ "$compileModeArgument" = "compile-mode" ]; then
  compileModeTmp=$(echo "$1" | cut -d "=" -f2-)
  case $compileModeTmp in
    st_component_mode)
    compileMode="st_component_mode"
    ;;
    shared_lib_mode)
    sharedLibs=true
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
    armhf_cross_mode_shared_libs)
    compileMode="armhf_cross_mode"
    sharedLibs=true
    ;;
    mips_cross_mode_shared_libs)
    compileMode="mips_cross_mode"
    sharedLibs=true
    ;;
    aarch64_cross_mode_shared_libs)
    compileMode="aarch64_cross_mode"
    sharedLibs=true
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
          wget --ca-certificate=/etc/ssl/certs/ca-certificates.crt -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add
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
    cmake ../ -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SDK=ON -DBUILD_TEST_DEPS=OFF -DLINK_DL=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON
    ;;
    armhf_cross_mode)
    apt-get update
    apt-get install --assume-yes software-properties-common
    apt-get install --assume-yes build-essential
    apt-get install --assume-yes g++-arm-linux-gnueabihf
    apt-get install --assume-yes gcc-arm-linux-gnueabihf
    apt-get install --assume-yes gdb-multiarch
    wget --ca-certificate=/etc/ssl/certs/ca-certificates.crt https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
    tar -xvzf openssl-${OPENSSL_VERSION}.tar.gz
    export INSTALL_DIR=/usr/lib/arm-linux-gnueabihf
    cd openssl-${OPENSSL_VERSION}
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
      cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../ # || true
#      cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../
    elif [ "$sharedLibs" = true ]; then
      cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake -DBUILD_SDK=ON ../ # || true
#      cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake -DBUILD_SDK=ON ../
      make install DESTDIR=./shared_install_dir
      chmod 0777 ./shared_install_dir
    else
      # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake ../ # || true
#      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-armhf.cmake ../
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
    wget --ca-certificate=/etc/ssl/certs/ca-certificates.crt https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
    tar -xvzf openssl-${OPENSSL_VERSION}.tar.gz
    export INSTALL_DIR=/usr/lib/mips-linux-gnu
    cd openssl-${OPENSSL_VERSION}
    ./Configure linux-mips32 shared \
      --prefix=$INSTALL_DIR --openssldir=$INSTALL_DIR/openssl \
      --cross-compile-prefix=/usr/bin/mips-linux-gnu-
    make depend
    make -j 4
    make install
    cd ..
    # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
    export S2N_NO_PQ=1
    export S2N_NO_PQ_ASM=1
    if [ "$sharedLibs" = true ]; then
      cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-mips.cmake ../ || true
#      cat CMakeCache.txt
#      rm -rf CMakeCache.txt
#      cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-mips.cmake ../
      make install DESTDIR=./shared_install_dir
      chmod 0777 ./shared_install_dir
    else
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-mips.cmake ../ || true
#      cat CMakeCache.txt
#      rm -rf CMakeCache.txt
#      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-mips.cmake ../
    fi
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
    wget --ca-certificate=/etc/ssl/certs/ca-certificates.crt https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
    tar -xvzf openssl-${OPENSSL_VERSION}.tar.gz
    export INSTALL_DIR=/usr/lib/aarch64-linux-gnu
    cd openssl-${OPENSSL_VERSION}
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
      cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../ # || true
#      cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake -DBUILD_SDK=ON -DEXCLUDE_JOBS=ON -DEXCLUDE_DD=ON -DEXCLUDE_FP=ON -DDISABLE_MQTT=ON ../
    elif [ "$sharedLibs" = true ]; then
      cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake -DBUILD_SDK=ON ../ # || true
#      cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake -DBUILD_SDK=ON ../
      make install DESTDIR=./shared_install_dir
      chmod 0777 ./shared_install_dir
    else
      # Fix for the Cmake executing build of the sdk which errors out linking incorrectly to openssl
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake ../ # || true
#      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/Toolchain-aarch64.cmake ../
    fi
    cmake --build . --target aws-iot-device-client
    cmake --build . --target test-aws-iot-device-client
    exit $?
    ;;
    *)
    if [ "$sharedLibs" = false ]; then
      cmake ../ -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF -DLINK_DL=ON
    else
      cmake ../ -DBUILD_SHARED_LIBS=ON -DBUILD_SDK=ON -DLINK_DL=ON
      make install DESTDIR=./shared_install_dir
      chmod 0777 ./shared_install_dir
    fi
    ;;
esac

cmake --build . --target aws-iot-device-client
if [ $? != 0 ]
then
  exit 1
fi
cmake --build . --target test-aws-iot-device-client

### Run Tests ###
env AWS_CRT_MEMORY_TRACING=1 valgrind --suppressions=./../cmake-toolchain/valgrind-suppressions.txt --leak-check=yes --error-exitcode=1 ./test/test-aws-iot-device-client
