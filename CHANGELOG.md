# Changelog
All notable changes to the AWS IoT Device Client will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Known Issues]
* [Fix resource leak by cleaning up used Secure Tunneling Contexts](https://github.com/awslabs/aws-iot-device-client/issues/443)

## [1.9.X] - 2023-12-21
### Added
* [Added GitHub CI for Coverity code scan](https://github.com/awslabs/aws-iot-device-client/pull/366)
* [Added Integration tests for Device Defender feature](https://github.com/awslabs/aws-iot-device-client/pull/368)
* [Added support for publishing MQTT message on file change through Pub-Sub sample](https://github.com/awslabs/aws-iot-device-client/pull/374)
* 
### Fixed
* [Device Client aborts on Red Hat Enterprise Linux ARM](https://github.com/awslabs/aws-iot-device-client/issues/87)
* [Device Client stability issues when receiving signal interrupts](https://github.com/awslabs/aws-iot-device-client/issues/110)
* [Device Client build failure with gcc v11](https://github.com/awslabs/aws-iot-device-client/issues/146)
* [Fixed Secure Tunneling feature to gracefully shutdown](https://github.com/awslabs/aws-iot-device-client/pull/427)
* [Fixed memory leak in Device Client Pub-Sub sample and Shadow feature](https://github.com/awslabs/aws-iot-device-client/pull/363)
* [Fixed reboot job handler](https://github.com/awslabs/aws-iot-device-client/pull/375)
* [Device Client fails to start with Fleet Provisioning enabled because of multiple lock files were being created](https://github.com/awslabs/aws-iot-device-client/pull/419)
* [Device Client unable to setup secure tunneling while using HTTP proxy](https://github.com/awslabs/aws-iot-device-client/issues/440)

## [1.8.X] - 2022-12-15
### Added
* Support for new runCommand Job type (e.g. ["AWS-Run-Command" Managed Job Template](https://docs.aws.amazon.com/iot/latest/developerguide/job-templates-managed.html))  (#359)
* Added cross-compilation support for PowerPC64 and PowerPC64le architectures (#328) (#329)
* New GitHub workflows for building [Docker images](https://github.com/awslabs/aws-iot-device-client/tree/main/.github/docker-images) and publishing to [ECR](https://gallery.ecr.aws/aws-iot-device-client?page=1) (#328) (#329) (#355)

### Fixed
* Fixed permissions for files created by Device Client (#344) (#303) (#305)
* Refactoring for testability and added unit tests (#275) (#313) (#309) (#317)
* Bug fix caused by promise variable being called more than once during shutdown (#307)
* Bug fix for lock file handler causing Device Client to fail on Amazon Linux Docker images (#332)

## [1.7.X] - 2022-07-21
### Added
* Provisioning with Secure Element (PKCS#11) support. (#272)
* HTTP proxy support. (#290)
* Add `softhsm` in the docker build files. (#267)
* Add more unit tests for jobs feature. (#266 #278)
* Add `--version` CLI option. (#280)

### Fixed
* Bump ubuntu version from 16 to 18, fix build issues with gcc 11. (#249)
* Bug fix for segmentation fault caused by empty root CA. Will default to default trust store if empty or invalid root-ca path is provided. (#262)
* The systemd script now have `--config-file` flag with default directory in `/etc`. (#259)
* Disable device defender by default. (#271)
* Fix cross compilation issue. (#274)
* Fix segmentation fault when closing secure tunnel with null WebSocket. (#276)
* Add check for return code from `wordexp` in implementation of `FileUtils::ExtractExpandedPath`. (#288)
* Fixes CLI parsing bug when command is unrecognized. (#289)
* Replace third party clang format action with `clang-format` from CI image. (#291)
* Documentation and README updates. (#261)

## [1.6.X] - 2022-05-06
### Added
* Sensor Publish over MQTT feature (#260)
* Modifies download file handler to allow for passing in a directory to store file (#238)
* Prevent multiple processes (#236)

### Fixed
* Upgrade openssl version to 1.1.1n to address CVE-2022-0778 (#228)
* Updated DD code to subscribe & unsubscribe to `/rejected` topic (#224)
* Secure tunneling deadlock fix. (#219)
* Bug fix for pub sub creation (#214)
* Device Client bug fixes for (1) reboot action, (2) Docker images and (3) DC versioning (#210)

## [1.5.X] - 2021-12-16
### Added
- Added support for AWS Managed Job Templates aka Standard Job Actions.
- Added validation checks for config file, cli inputs and empty job handler directory.
- Added support for creating PubSub files. 

### Fixed
- Added support for enabling SDK logging using SDK and Config file
- Updated documentation for Named Shadow feature and use of thing-name across Device Client
- Updated docker image files to use latest code from SDK's submodule.  
- Updated Device Client fatal error messages. 


## [1.3.X] - 2021-08-19
### Added
- Added support for Telemetry data.
- Added support for Named Shadow feature in Device Client.

### Fixed
- Resolved high CPU utilization issue.
- Updated SDK version which resolves security vulnerability issue, and issues causing Device Defender and Secure Tunneling failures.

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
