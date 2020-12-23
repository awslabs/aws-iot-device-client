# Changelog
All notable changes to the AWS IoT Device Client will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Resolution for Secure Tunneling SDK 16KB buffering issue. Current Secure Tunneling implementation will
encounter issues if data written to the buffer is larger than 16KB
- Resolution for Device Defender reboot segfault. There is a known issue where rebooting the device
while Device Defender is running as a service will trigger a segfault when the Device Client starts again after the reboot

## [0.2.X] - 2020-12-23
### Added
- CMake build system
- CMake conditional compilation flags for reducing output executable footprint
- CMake support for automatically building the [AWS IoT v2 SDK for C++](https://github.com/aws/aws-iot-device-sdk-cpp-v2)
- Setup script for configuration automation and help installing the AWS IoT Device Client
- Valgrind debugging support added to setup.sh
- Configuration via CLI and JSON file
- Logger with both standard output and file-based logging implementations
- AWS IoT Jobs feature for execution of jobs on the device
- AWS IoT Secure Tunneling feature for establishing secure tunnels to the device
- AWS IoT Device Defender feature for publishing Device Defender metrics to AWS
- AWS IoT Fleet Provisioning feature for device provisioning. Includes support for provisioning via CSR and 
- Sample job documents and job handlers for examples of how to use the Jobs feature functionality
- Semantic versioning logic built into the CMake system which pulls information from Git
- Utility classes for operations on files and strings
- Google Test framework added for unit testing
- Doxygen support for building HTML and Latex documentation from source code
- Clang-format support via format-check.sh for verifying code will pass our CI Lint checks

### Changed

### Removed

### Fixed

## Release Deltas

[Unreleased](https://github.com/awslabs/aws-iot-device-client/compare/v0.2...HEAD)
[0.2.X](https://github.com/awslabs/aws-iot-device-client/compare/v0.0...v0.2)
[0.0.X](https://github.com/awslabs/aws-iot-device-client/releases/tag/v0.0)