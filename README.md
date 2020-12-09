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

Use the following `cmake` commands to build the AWS IoT Device Client with dependencies and tests. These should be run
with  `aws-iot-device-client` (the contents of this repository) in a folder called `aws-iot-device-client` in your 
current working directory:

```bash
mkdir aws-iot-device-client/build
cd aws-iot-device-client/build
cmake ../
cmake --build . --target aws-iot-device-client
cmake --build . --target test-aws-iot-device-client # This line builds the test executable
```

##### Build With Dependencies Already Installed

**Description**:  
Use this build method for building the device client with dependencies already installed. This build option may be
applicable if you already have the aws-iot-device-sdk-cpp-v2 installed on your device.

* The Device Client requires that the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) 
  is installed.
* The Device Client tests require that [googletest](https://github.com/google/googletest) is installed.

Use the following `cmake` commands to build the AWS IoT Device Client with already existing and installed dependencies.
These commands should be run with `aws-iot-device-client` (the contents of this repository) in a folder called
`aws-iot-device-client` in your current working directory

```bash
mkdir aws-iot-device-client/build
cd aws-iot-device-client/build
cmake ../ -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF
cmake --build . --target aws-iot-device-client
cmake --build . --target test-aws-iot-device-client # This line builds the test executable
```

#### Custom Compilation

The Device Client can be customized during build time to compile out certain IoT Features as desired. All features are
included by default. Use CMake variables in order to configure which features to exclude. See the project 
`CMakeLists.txt` file for the options provided and their descriptions.

Available CMake flags for feature exclusion:
* `EXCLUDE_JOBS`
* `EXCLUDE_DD`
* `EXCLUDE_ST`
* `EXCLUDE_FP`

Example CMake command to exclude only the Device Defender feature from the build:

```bash
mkdir aws-iot-device-client/build
cd aws-iot-device-client/build
cmake ../ -DEXCLUDE_DD=ON
```

### Running the tests
```
./build/test-aws-iot-device-client 
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

### Usage
Configuration information can be passed to the AWS IoT Device Client through either the command line, a JSON configuration
file, or both. For a complete list of available command line arguments, pass `--help` to the executable. 

#### Configuring the AWS IoT Device Client's Logger
The AWS IoT Device Client has the capability to log directly to standard output or write logs to a log file. For
either option, the logging level can be specified as either DEBUG, INFO, WARN, or ERROR. The logger implementation
can be specified as either "STDOUT" (for standard output) or "FILE" (for logging to a file). If file based logging
is specified, you can also specify a file to log to. If a file is not specified, the Device Client will log to 
the default log location of `/var/log/aws-iot-device-client.log`. Keep in mind that the Device Client will need
elevated permissions to log to this location, and will automatically fall back to STDOUT logging if the Device Client
is unable to log to either the specified or default location. 

###### Example
Configuring the logger via the command line:
```
./aws-iot-device-client --log-level WARN --log-type FILE --log-file ./aws-iot-device-client.log
```

Configuring the logger via JSON:
```
    {
        ...
        "logging": {
            "level": "WARN",
            "type": "FILE",
            "file": "./aws-iot-device-client.log"
        }
        ...
    }
```
If you've decided to add additional logs to the AWS IoT Device Client's source code, the high-level
logging API macros can be found in `source/logging/LoggerFactory.h` and typically follow the convention of 
`LOG_XXXX` for simple log messages and `LOGM_XXXX` for logs that should be formatted with variadic arguments. 

### Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

### License

This project is licensed under the Apache-2.0 License.