# Changelog
All notable changes to the AWS IoT Device Client will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Known Issues]
- [Device Client aborts on Red Hat Enterprise Linux ARM](https://github.com/awslabs/aws-iot-device-client/issues/87)
- [Device Client crashes due to SIGSEGV on Ubuntu x86](https://github.com/awslabs/aws-iot-device-client/issues/86)

## [1.2.X] - 2021-05-06
### Added
- Added support for storing FP device config from FP Template in runtime-config file
- Support to pass parameters to the AWS IoT Fleet Provisioning RegisterThing MQTT request

### Fixed
- Resolved issue where missing pub sub sample files (publish file and subscribe file) would cause a seg fault
- Resolved issue where pub sub sample lost reference on the Default Publish Payload allocator, publishing a blank string
- Fixed a bug with the pub sub sample CLI config

## [1.1.X] - 2021-04-14
### Added
- SDK logging now configurable via CLI or JSON
- Support for configuring Device Private Key via CLI and JSON for Fleet provisioning using CreateCertificateFromCsr API 
- Added [Pub/Sub Sample Feature](source/samples/pubsub/README.md)
- Update job to "In Progress" after receive
- Shared Lib builds in Github Actions
- Add CodeDeploy config for integration test setup
- Doxygen Github Actions

### Changed
- Readme Makeover
- Address cross-platform compatibility for ANSI color codes

### Removed

### Fixed
- Properly stripping symbols on RELEASE builds
- Updated SDK version to fix shared lib builds
- Broken Doxygen URL in the Readme
- Fix Ubi8 and Al2 Dockerfile Openssl install

## [1.0.X] - 2021-01-21
### Added
- Support for opening multiple tunnels at the same time
- Handling for secure tunnel OnConnectionShutdown
- Cross compilation toolchains for a number of platforms
- Log sanitization to minimize attack surface and protect against printf vulnerabilities
- Handling for empty json values, local keys and certificates
- Log messaging to indicate permission information to the user
- Ownership validation against required files
- Limitations to log output for Job child processes
- Exponential backoff retry for initial MQTT connection
- Recovery mode for Jobs feature to handle duplicate notifications received after connectivity loss
- Improved logging API startup behavior
- Support for transferring log queues between logger implementations when configuration changes

### Changed
- README documentation to provide a better getting started experience
- String handling in utility functions to reduce risk of string related security vulnerabilities
- Improved CLI handling for feature enable/disable to maintain parity with .conf JSON file
- Device Client no longer requires presence of a configuration file as long as required values are provided via CLI

### Removed

### Fixed
- Secure Tunneling SDK 16KB buffering issue. Previous Secure Tunneling implementation would
encounter issues if data written to the buffer is larger than 16KB
- Device Defender reboot segfault. There was a known issue where rebooting the device
  while Device Defender is running as a service would trigger a segfault when the Device Client starts again after the reboot

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
- AWS IoT Fleet Provisioning feature for device provisioning
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

[Unreleased](https://github.com/awslabs/aws-iot-device-client/compare/v1.0...HEAD)
[1.0.X](https://github.com/awslabs/aws-iot-device-client/compare/v0.2...v1.0)
[0.2.X](https://github.com/awslabs/aws-iot-device-client/compare/v0.0...v0.2)
[0.0.X](https://github.com/awslabs/aws-iot-device-client/releases/tag/v0.0)
