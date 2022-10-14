# Integration Tests
**Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

* Integration Tests
  * Introduction
  * Setup & Configuration
  * Running Tests

## Introduction

The Device Client Integration Tests are a separate CMake project within the Device Client Repository. The tests run as part
of the release process for Device Client. However, they may also be built and run locally with a custom target. Currently,
not all features are supported in the integration test module.

### Dependencies
The only additional dependencies in the integration tests are the AWS C++ SDK iot and iotsecuretunneling components.
GoogleTest is the only other dependency of the integration tests and it is also used in the Device Client main project.

## Setup & Configuration

### Building from source

To use the AWS IoT Device Client Integration Tests, you'll need to compile an executable using the source code provided by this
repository. Below, you'll find instructions for how to build the AWS IoT Device Client Integration Tests for your source machine.

### Quick Start

The following commands should work for most users when you plan to run the AWS IoT Device Client Integration Tests on the 
same machine that you're performing the compilation on:

```
# Building
git clone https://github.com/awslabs/aws-iot-device-client
cd aws-iot-device-client/integration-tests
mkdir build
cd build
cmake ../
cmake --build . --target aws-iot-device-client-integration-tests
```

### AWS Credentials

The integration tests use the AWS Default Credentials Provider. In order to run the tests you will need environment variables
set for AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, and optionally AWS_SESSION_TOKEN. You can read more about using credentials
from environment variables [here](https://docs.aws.amazon.com/sdk-for-php/v3/developer-guide/guide_credentials_environment.html).

### Targeting

To run the integration tests you will need to have an AWS Thing registered and running Device Client connected to the AWS IoT Cloud.
If no target is provided to the integration tests, it will look locally for a Device Client Runtime Configuration file and load the
target from there. It is recommended to explicitly pass a target unless you are running the integration tests on one of the integration
test docker images for this specific purpose. Once you have an AWS Thing setup and running Device Client (see [here](../README.md)). 
You can run the integration tests like so:

```
# Run integration tests against <thing-name>
./aws-iot-device-client-integration-tests --thing-name <thing-name>

# Print help information
./aws-iot-device-client-integration-tests --help
```