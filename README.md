
# AWS IoT Device Client

 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

*__Jump To:__*
  * [Introduction](#introduction)
  * [Installation](#installation)
  * [Setup & Configuration](#setup-and-config)
  * [Jobs Feature](source/jobs/README.md)
  * [Fleet Provisioning Feature](source/fleetprovisioning/README.md)
  * [Device Defender Feature](source/devicedefender/README.md)
  * [Secure Tunneling Feature](source/tunneling/README.md)
  * [MQTT Pub/Sub Sample Feature](source/samples/pubsub/README.md)
  * [Logging](source/logging/README.md)
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
  when you onboard a fleet of devices to AWS IoT Core. It creates a device specific certificate and private key, and registers the device on AWS IoT Core.
### List of Supported Platforms
The AWS IoT Device Client currently works on IoT devices with common microprocessors (x86_64, ARM architectures), and common Linux software environments (Debian, Ubuntu, and RHEL).

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
* OpenSSL 1.1.1
* [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) commit hash located in `CMakeLists.txt.awssdk`

*Note:* The TLS stack, and the version of the SDK mentioned above is what our CI uses.  You could potentially use a different TLS stack for example, we just don't actively test or support this.

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
./build/test/test-aws-iot-device-client 
```
### Advanced Compilation
[Advanced Compilation](docs/COMPILATION.md)

## Setup and Configuration
[Setting Up The Device Client](docs/SETUP.md)  
[Configuring the AWS IoT Device Client](docs/CONFIG.md)  
[File and Directory Permission Requirements](docs/PERMISSIONS.md)

## AWS IoT Features
[Jobs Feature Readme](source/jobs/README.md)  
[Fleet Provisioning Feature Readme](source/fleetprovisioning/README.md)  
[Device Defender Feature Readme](source/devicedefender/README.md)  
[Secure Tunneling Feature Readme](source/tunneling/README.md)

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
