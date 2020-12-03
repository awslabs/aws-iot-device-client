#AWS IoT Device Client

## Installation

### Minimum Requirements

* C++ 11 or higher
* CMake 3.10+

### Build from source

#### Automatically Build and Install Dependencies via CMake

```bash
cmake -DCMAKE_PREFIX_PATH=<> -DCMAKE_INSTALL_PREFIX=<> -DBUILD_SDK=ON -DBUILD_TEST_DEPS=ON -S aws-iot-device-client -B aws-iot-device-client/build
cmake --build aws-iot-device-client/build --target aws-iot-device-client -- -j 8
```

#### Manually Build and Install Dependencies

The Device Client requires that the aws-iot-device-sdk-cpp-v2 is installed.
The Device Client tests require that googletest is installed.

* To install the AWS IoT v2 SDK and Google Test
```bash
./run-install.sh
```   

* To build the AWS IoT AWS IoT AWS IoT Device Client
```bash
./run-build.sh
```

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

This project is licensed under the Apache-2.0 License.