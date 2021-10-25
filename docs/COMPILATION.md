# Advanced Compilation
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

+ [Advanced Compilation](#advanced-compilation)
    - [Build and Install All Dependencies via CMake](#build-and-install-all-dependencies-via-cmake)
    - [Build With Dependencies Already Installed](#build-with-dependencies-already-installed)
    - [Building a Release Candidate](#building-a-release-candidate)
    - [Custom Compilation - Exclude Specific IoT Features to Reduce Executable Footprint](#custom-compilation---exclude-specific-iot-features-to-reduce-executable-footprint)
    - [Cross Compiliation - Building from one architecture to the other](../cmake-toolchain/README.md)

[*Back To The Main Readme*](../README.md)

## Advanced Compilation

### Build and Install All Dependencies via CMake

**Note:** It is important to know that we build the Device Client with a specific commit hash of the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2).  This commit can be located in `CMakeLists.txt.awssdk`.  While other versions of the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) ***may*** work, it is highly recommended that the specified commit is used. 

**Description**:  
Use this build method to compile an executable AWS IoT Device Client. CMake will pull the dependencies required to build
the AWS IoT Device Client, including the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2).

Use the following `cmake` commands to build the AWS IoT Device Client with dependencies and tests. These should be run
with  `aws-iot-device-client` (the contents of this repository) in a folder called `aws-iot-device-client` in your 
current working directory:

```
git clone https://github.com/awslabs/aws-iot-device-client
cd aws-iot-device-client
mkdir build
cd build
cmake ../
cmake --build . --target aws-iot-device-client
cmake --build . --target test-aws-iot-device-client # This line builds the test executable
```

### Build With Dependencies Already Installed

**Description**:  
Use this build method for building the device client with dependencies already installed. This build option may be
applicable if you already have the aws-iot-device-sdk-cpp-v2 installed on your device.

* The Device Client requires that the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) 
  is installed.
* The Device Client tests require that [googletest](https://github.com/google/googletest) is installed.

Options (These options can be passed to :
* BUILD_SDK: This CMake flag is set to `ON` by default, which will enable CMake to pull and build the 
  aws-iot-device-sdk-cpp-v2
* BUILD_TEST_DEPS: This CMake flag is set to `ON` by default, will enable CMake to pull and build googletest

Use the following `cmake` commands to build the AWS IoT Device Client with already existing and installed dependencies.
These commands should be run with `aws-iot-device-client` (the contents of this repository) in a folder called
`aws-iot-device-client` in your current working directory

```
git clone https://github.com/awslabs/aws-iot-device-client
cd aws-iot-device-client
mkdir build
cd build
cmake ../ -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF
cmake --build . --target aws-iot-device-client
cmake --build . --target test-aws-iot-device-client # This line builds the test executable
```

### Building a Release Candidate

By default, the CMake configuration for the AWS IoT Device Client generates a 'Debug' build, which includes
symbols in the output binary that can be used to debug the program via utilities such as GDB. However, it is possible
to reduce output binary size (helpful for memory constrained devices) and/or optimize the executable using the following
CMake Build Type flags:

* Debug: Includes debug info with no optimization
* Release: No debug info and full optimization
* RelWithDebInfo: Release optimizations with debug info
* MinSizeRel: Release build with additional output size optimizations

Use the build type flags as follows:
```
cmake -DCMAKE_BUILD_TYPE=Release ../
cmake --build . --target aws-iot-device-client
```

### Custom Compilation - Exclude Specific IoT Features to Reduce Executable Footprint

**Description**:
The Device Client can be customized during build time to exclude certain IoT features from compilation, which may be
helpful if you don't plan to use a particular feature and you'd like to decrease the overall size of the output
executable. All features are included by default. Use CMake variables in order to configure which features to
exclude. See the project `CMakeLists.txt` file for the options provided and their descriptions.

**Use Case**:
If there are certain IoT Features that you do not plan to use, then you can potentially minimize both on-disk and 
runtime memory for your device client executable by excluding those features from compilation.

Available CMake flags for feature exclusion:
* `EXCLUDE_JOBS`
* `EXCLUDE_DD`
* `EXCLUDE_ST`
* `EXCLUDE_FP`
* `EXCLUDE_SAMPLES`
* `EXCLUDE_PUBSUB`
* `EXCLUDE_SHADOW`
* `EXCLUDE_CONFIG_SHADOW`
* `EXCLUDE_SAMPLE_SHADOW`

Example CMake command to exclude only the Device Defender feature from the build:

```bash
git clone https://github.com/awslabs/aws-iot-device-client
cd aws-iot-device-client
mkdir build
cd build
cmake ../ -DEXCLUDE_DD=ON
```
### Cross Compiliation - Building from one architecture to the other
[Cross Compiliation READMD](../cmake-toolchain/README.md)

[*Back To The Top*](#advanced-compilation)