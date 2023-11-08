
# AWS IoT Device Client

 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

*__Jump To:__*
  * [Introduction](#introduction)
  * [Installation](#installation)
  * [Setup & Configuration](#setup-and-configuration)
  * [Jobs Feature](source/jobs/README.md)
  * [Fleet Provisioning Feature](source/fleetprovisioning/README.md)
  * [Device Defender Feature](source/devicedefender/README.md)
  * [Secure Tunneling Feature](source/tunneling/README.md)
  * [Named Shadow Feature](source/shadow/README.md)
    - [Sample Shadow Feature](source/shadow#sample-shadow)
    - [Config Shadow Feature](source/shadow#config-shadow)
  * [Sensor Publish Feature](source/sensor-publish/README.md)
  * [Provisioning with Secure Elements Feature](source/secure-element/README.md)
  * [Logging](source/logging/README.md)
  * [HTTP Proxy](docs/HTTP_PROXY.md)
  * [Samples](source/samples/):
    - [MQTT Pub/Sub Sample Feature](source/samples/pubsub/README.md)
  * [Doxygen Documentation](docs/README.md)
  * [Additional Resources](#additional-resources)
  * [Security](#security)
  * [License](#license)

## Introduction
*__Sections:__*
* [Current Capabilities](#current-capabilities)
* [List of Supported Platforms](#list-of-supported-platforms)

[*Back To The Top*](#aws-iot-device-client)


The AWS IoT Device Client is free, open-source, modular software written in C++ that you can compile and install on
 your Embedded Linux based IoT devices to access [AWS IoT Core](https://aws.amazon.com/iot-core/), [AWS IoT Device
  Management](https://aws.amazon.com/iot-device-management/), and [AWS IoT Device
  Defender](https://aws.amazon.com/iot-device-defender) features by default. It serves as a reference implementation for your IoT devices to work with AWS IoT
   services, with operational best practices baked in – using it is the easiest way to create a proof-of-concept (PoC) for your IoT project. What’s more, since it is open-source, you can modify it to fit your business needs, or optimize it when you wish to scale up from a PoC to production.

### Current Capabilities
The modular IoT Device Client consists of a “base client” and discrete “client-side features” that support the following:
* The base client handles MQTT connectivity with AWS IoT core - it enables your IoT device to automatically
 connect and make subscriptions to feature-relevant MQTT topics. It also provides a logging API for device side logs.
* The client-side Jobs feature enables you to execute remote actions on your device when you use the [Jobs feature](https://aws.amazon.com/iot-device-management/features/#Remotely_Manage_Connected_Devices) of
 the AWS IoT Device Management service. It provides support for a few remote actions by default, and extensibility for custom actions. You can use custom actions to remotely control the state of your IoT devices.
* The client-side Secure Tunneling feature enables secure, privileged access to your IoT device when you use the
 [Secure Tunneling feature](https://aws.amazon.com/iot-device-management/features/#Secure_Tunneling) in the AWS IoT Device Management service.
* The client-side Device Defender feature enables you to collect standard [Device Side Metrics](https://docs.aws.amazon.com/iot/latest/developerguide/detect-device-side-metrics.html) when you use the [Rules
 Detect feature](https://docs.aws.amazon.com/iot/latest/developerguide/detect-device-side-metrics.html) in the AWS IoT Device Defender service.
* The client-side Fleet Provisioning feature enables you to replace provisional credentials with device-specific ones
  when you onboard a fleet of devices using the [Fleet Provisioning capability](https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html) of AWS IoT Core. It creates a device specific certificate and private key, and registers the device on AWS IoT Core.
* The client-side Named Shadows feature enables you to control your IoT device using [AWS IoT Named Shadows](https://docs.aws.amazon.com/iot/latest/developerguide/iot-device-shadows.html). Shadows can store your device's state information and make it available to your device, AWS IoT services, your custom  apps and other AWS services whether the device is online and connected to AWS IoT or not.
### List of Supported Platforms
The AWS IoT Device Client is currently compatible with x86_64, aarch64, armv7l, mips32, ppc64, and ppc64le architectures and common Linux software environments (Debian, Ubuntu, and RHEL).

## Installation
*__Sections:__*
+ [Minimum Requirements](#minimum-requirements)
+ [Building from source](#building-from-source)
+ [Running the tests](#running-the-tests)
+ [Advanced Compilation](docs/COMPILATION.md)

[*Back To The Top*](#aws-iot-device-client)

### Minimum Requirements

* C++ 11 or higher
* [CMake](https://cmake.org/) 3.10+
* OpenSSL 3.0.0+
* [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) commit hash located in `CMakeLists.txt.awssdk`

*Note:* The TLS stack, and the version of the SDK mentioned above is what our CI uses.  You could potentially use a different TLS stack for example, we just don't actively test or support this.

### Docker

The AWS IoT Device Client currently provides several docker images on various platforms and Linux distributions.

To build a Docker image from the repository locally simply run the [docker-build.sh](docker-build.sh) script with your
preferred OS (ubuntu/amazonlinux/ubi8) (e.g. docker-build.sh ubuntu) if no OS is passed the build will default to ubuntu (18.04).

For a minimum sized fully built AWS IoT Device Client Docker image simply pull your preferred architecture/OS combination
from our repository [here](https://gallery.ecr.aws/aws-iot-device-client/aws-iot-device-client)

#### Docker Files

- [Base Images:](.github/docker-images/base-images) Multi-stage Dockerfiles which will build dependencies only (target=**base**) or build the Device Client from your current directory(target=**deploy**).
- [Dockerfile:](.github/docker-images/Dockerfile) Takes two BUILD_ARGS: OS(ubuntu/amazonlinux/ubi8) and BASE_IMAGE(Image URI). Build target **deploy** will build Device Client from your current directory using the provided BASE_IMAGE. Build target **minimum_size** will build a minimum sized Docker image with the Device Client binary using the BASE_IMAGE.

### Building from source

To use the AWS IoT Device Client, you'll need to compile an executable using the source code provided by this
repository. Below, you'll find instructions for how to build the AWS IoT Device Client for your target machine.

### Quick Start

The following commands should work for most users when you plan to run the AWS IoT Device Client on the same machine
that you're performing the compilation on:

```
# Building
git clone https://github.com/awslabs/aws-iot-device-client
cd aws-iot-device-client
mkdir build
cd build
cmake ../
cmake --build . --target aws-iot-device-client

# Setup
cd ../
./setup.sh # At this point you'll need to respond to prompts for information, including paths to your thing certs

# Run the AWS IoT Device Client
./aws-iot-device-client # This command runs the executable
```

### Running the tests
```
cmake ../
cmake --build . --target test-aws-iot-device-client
./build/test/test-aws-iot-device-client
```
### Advanced Compilation
[Advanced Compilation](docs/COMPILATION.md)

## Setup and Configuration
* [Setting Up The Device Client](docs/SETUP.md)
* [Configuring the AWS IoT Device Client](docs/CONFIG.md)
* [File and Directory Permission Requirements](docs/PERMISSIONS.md)
* [Environment Variables](docs/ENV.md)
* [Version](docs/VERSION.md)
* [HTTP Proxy](docs/HTTP_PROXY.md)

## AWS IoT Features
* [Jobs Feature Readme](source/jobs/README.md)
* [Fleet Provisioning Feature Readme](source/fleetprovisioning/README.md)
* [Device Defender Feature Readme](source/devicedefender/README.md)
* [Secure Tunneling Feature Readme](source/tunneling/README.md)
* [Named Shadow Feature Readme](source/shadow/README.md)
* [Sensor Publish Feature](source/sensor-publish/README.md)
* [Provisioning with Secure Elements Feature](source/secure-element/README.md)

## AWS IoT Device Client Samples
[MQTT Pub/Sub Sample Feature](source/samples/pubsub/README.md)

## Logging
[Logging Readme](source/logging/README.md)

## Doxygen Documentation
[Doxygen Documentation Readme](docs/README.md)

## Additional Resources

This section provides links to additional AWS IoT resources that may help you use and modify the AWS IoT Device Client:

- [API Documentation generated by Doxygen](https://awslabs.github.io/aws-iot-device-client/annotated.html)
- [Related SDKs, Samples, and Documentation](https://docs.aws.amazon.com/iot/latest/developerguide/iot-sdks.html#iot-device-sdks)
- [AWS IoT Device SDK for C++ v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2)

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

This project is licensed under the Apache-2.0 License.

[*Back To The Top*](#aws-iot-device-client)
