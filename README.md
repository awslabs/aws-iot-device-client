## AWS IoT Device Client

### Installation

#### Minimum Requirements

* C++ 11 or higher
* [CMake](https://cmake.org/) 3.10+

### Building from source

##### Build and Install All Dependencies via CMake

**Description**:  
Use this build method for the simplest getting started experience. CMake will pull the dependencies required to build
the AWS IoT Device Client, including the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2).

Options:
* BUILD_SDK: This CMake flag is set to `ON` by default, which will enable CMake to pull and build the 
  aws-iot-device-sdk-cpp-v2
* BUILD_TEST_DEPS: This CMake flag is set to `ON` by default, will enable CMake to pull and build googletest

Use the following `cmake` commands to build the AWS IoT Device Client with dependencies and tests.

```bash
cmake -S aws-iot-device-client -B aws-iot-device-client/build
cmake --build aws-iot-device-client/build --target aws-iot-device-client
```

##### Build With Dependencies Already Installed

**Description**:  
Use this build method for building the device client with dependencies already installed. This build option may be
applicable if you already have the aws-iot-device-sdk-cpp-v2 installed on your device.

* The Device Client requires that the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) 
  is installed.
* The Device Client tests require that [googletest](https://github.com/google/googletest) is installed.

Use the following `cmake` commands to build the AWS IoT Device Client with already existing and installed dependencies.

```bash
cmake -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF -S aws-iot-device-client -B aws-iot-device-client/build
cmake --build aws-iot-device-client/build --target aws-iot-device-client
```

### Setting Up The Device Client

This package comes with a interactive script for setting up the Device Client. This script can be used to generate
initial Device Client configuration, as well as setup the Device Client as a service.

When setting up the Device Client as a service, you will be asked to provide the location of the Device Client binary
as well as the service file. For an example of the service file, see the file included in this repository at 
`setup/device-client.service`.

To use the script, run the following command.

```bash
./setup.sh
```

### Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

### License

This project is licensed under the Apache-2.0 License.