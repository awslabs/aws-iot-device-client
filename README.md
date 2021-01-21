# AWS IoT Device Client

- [AWS IoT Device Client](#aws-iot-device-client)
  * [Introduction](#introduction)
    + [Current Capabilities](#current-capabilities)
    + [List of Supported Platforms](#list-of-supported-platforms)
  * [Installation](#installation)
    + [Minimum Requirements](#minimum-requirements)
    + [Building from source](#building-from-source)
      - [Quick Start](#quick-start)
      - [Build and Install All Dependencies via CMake](#build-and-install-all-dependencies-via-cmake)
      - [Build With Dependencies Already Installed](#build-with-dependencies-already-installed)
      - [Custom Compilation - Exclude Specific IoT Features to Reduce Executable Footprint](#custom-compilation---exclude-specific-iot-features-to-reduce-executable-footprint)
      - [Cross Compiliation - Building from one architecture to the other](#cross-compiliation---building-from-one-architecture-to-the-other)
    + [Running the tests](#running-the-tests)
  * [Setting Up The Device Client](#setting-up-the-device-client)
    + [Configuring the AWS IoT Device Client](#configuring-the-aws-iot-device-client)
  * [File and Directory Permissions](#file-and-directory-permissions)
      - [Recommended and Required permissions on files](#recommended-and-required-permissions-on-files)
      - [Recommended and Required permissions on directories storing respective files](#recommended-and-required-permissions-on-directories-storing-respective-files)
  * [Jobs Feature](#jobs-feature)
    + [Creating a Job](#creating-a-job)
    + [Creating your own Job Handler](#creating-your-own-job-handler)
      - [Job/Job Handler Security Considerations](#jobjob-handler-security-considerations)
    + [Debugging your Job](#debugging-your-job)
    + [Jobs Build Flags](#jobs-build-flags)
    + [Jobs Feature Configuration Options](#jobs-feature-configuration-options)
      - [Configuring the Jobs feature via the command line:](#configuring-the-jobs-feature-via-the-command-line)
      - [Configuring the Jobs feature via the JSON configuration file:](#configuring-the-jobs-feature-via-the-json-configuration-file)
  * [Fleet Provisioning Feature](#fleet-provisioning-feature)
    + [Resources required for Fleet Provisioning feature](#resources-required-for-fleet-provisioning-feature)
    + [Sample Claim Certificate Policy](#sample-claim-certificate-policy)
        * [Sample Policy:](#sample-policy)
    + [Sample Fleet Provisioning Template](#sample-fleet-provisioning-template)
        * [Sample Template:](#sample-template)
    + [Sample Permanent Certificate Policy](#sample-permanent-certificate-policy)
        * [Sample (FPCertPolicy) Policy:](#sample-fpcertpolicy-policy)
    + [Fleet Provisioning Runtime Configuration File](#fleet-provisioning-runtime-configuration-file)
        * [Example Fleet Provisioning Runtime Configuration File:](#example-fleet-provisioning-runtime-configuration-file)
    + [Fleet Provisioning Feature Configuration Options](#fleet-provisioning-feature-configuration-options)
        - [Configuring the Fleet Provisioning feature via the command line:](#configuring-the-fleet-provisioning-feature-via-the-command-line)
        - [Configuring the Fleet Provisioning feature via the JSON configuration file:](#configuring-the-fleet-provisioning-feature-via-the-json-configuration-file)
  * [Device Defender Feature](#device-defender-feature)
    + [Device Defender Feature Configuration Options](#device-defender-feature-configuration-options)
        - [Configuring the Device Defender feature via the command line:](#configuring-the-device-defender-feature-via-the-command-line)
        - [Configuring the Device Defender feature via the JSON configuration file:](#configuring-the-device-defender-feature-via-the-json-configuration-file) 
  * [Secure Tunneling Feature](#secure-tunneling-feature)
    + [Secure Tunneling Feature Configuration Options](#secure-tunneling-feature-configuration-options)
        - [Configuring the Secure Tunneling feature via the command line:](#configuring-the-secure-tunneling-feature-via-the-command-line)
        - [Configuring the Secure Tunneling feature via the JSON configuration file:](#configuring-the-secure-tunneling-feature-via-the-json-configuration-file)
    + [Example steps to use the Secure Tunneling feature](#example-steps-to-use-the-secure-tunneling-feature)
    + [Limitation](#limitation)
  * [Logging](#logging)
    + [Logging Configuration Options](#logging-configuration-options)
  * [Security](#security)
  * [License](#license)
    
## Introduction

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
The AWS IoT Device Client currently works by default on IoT devices with common microprocessors (x86_64 or ARM architectures), and common Linux software environments (Debian, Ubuntu, and RHEL).
Tested devices: Raspberry Pi 4, <Add others>

## Installation

### Minimum Requirements

* C++ 11 or higher
* [CMake](https://cmake.org/) 3.10+
* *Note* CI Build uses OpenSSL 1.1.1 

### Building from source

To use the AWS IoT Device Client, you'll need to compile an executable using the source code provided by this
repository. Below, you'll find instructions for how to build the AWS IoT Device Client for your target machine. 
 
#### Quick Start #### 

The following commands should work for most users when you plan to run the AWS IoT Device Client on the same machine
that you're performing the compilation on:

```
# Building
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

#### Build and Install All Dependencies via CMake

**Description**:  
Use this build method to compile an executable AWS IoT Device Client. CMake will pull the dependencies required to build
the AWS IoT Device Client, including the [aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2).

Use the following `cmake` commands to build the AWS IoT Device Client with dependencies and tests. These should be run
with  `aws-iot-device-client` (the contents of this repository) in a folder called `aws-iot-device-client` in your 
current working directory:

```bash
cd aws-iot-device-client
mkdir build
cd build
cmake ../
cmake --build . --target aws-iot-device-client
cmake --build . --target test-aws-iot-device-client # This line builds the test executable
```

#### Build With Dependencies Already Installed

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

```bash
cd aws-iot-device-client
mkdir build
cd build
cmake ../ -DBUILD_SDK=OFF -DBUILD_TEST_DEPS=OFF
cmake --build . --target aws-iot-device-client
cmake --build . --target test-aws-iot-device-client # This line builds the test executable
```

#### Custom Compilation - Exclude Specific IoT Features to Reduce Executable Footprint

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

Example CMake command to exclude only the Device Defender feature from the build:

```bash
cd aws-iot-device-client
mkdir build
cd build
cmake ../ -DEXCLUDE_DD=ON
```
#### Cross Compiliation - Building from one architecture to the other
**Description**:
Cross Compiling allows you to compile for a **specific architecture** while on a **different architecture**.  This is extremely useful, as some *(Especially really constrained)* devices won't have the resources to complete the build process.  Even if your devices can complete the build, it is often times much easier to build in a central location (like your developer laptop) and then distribute the artifacts to appropriate devices. 

The `cmake-toolchain` folder contains toolchains that will make cross compiling for other architectures easier. Currently we have toolchains to support cross compiling for the following architectures; **MIPS**, **ARMhf**, & **AArch64**.

You can specify one of the given toolchains when running cmake:
```
cmake ../ -DCMAKE_TOOLCHAIN_FILE=<Path/To/Toolchain>
```
This will allow the toolchain to overwrite variables *(various paths, compilers, & flags)* required to execute cross compilation without changing the original **cmake** file.

**Dependencies**:
For your build to be successful you'll also need a cross compiled version of our dependencies ([aws-iot-device-sdk-cpp-v2](https://github.com/aws/aws-iot-device-sdk-cpp-v2) & **openssl**), for the SDK this is automatically accomplished when running the **cmake** command above without the following flag `-DBUILD_SDK=OFF`.

The last dependency you'll need cross compiled is **openssl**.  This one is slightly more complicated but can be done as follows:  *(This example is from our build process, replace the information in carets.  While we happen to be linking against OpenSSL 1.1.1 in this example since our target device uses OpenSSL 1.1.1 for its TLS implementation, you'll want to replace this with whatever TLS implementation is present on your target device.)*
```
wget https://www.openssl.org/source/openssl-1.1.1.tar.gz
tar -xvzf openssl-1.1.1.tar.gz
export INSTALL_DIR=</Path/To/Install/Dir>
cd openssl-1.1.1
./Configure <Platform> shared \
    --prefix=$INSTALL_DIR --openssldir=$INSTALL_DIR/openssl \
    --cross-compile-prefix=</Compiler/Prefix/Path> 
make depend
make -j 4
make install 
```
For a real example look into our `.github/build.sh` script.

**Other Architecture**:
If you need to compile for an architecture that isn't currently supported, then you can create a new toolchain based on the sample toolchains included in the cmake-toolchain folder.

### Running the tests
```
./build/test/test-aws-iot-device-client 
```

## Setting Up The Device Client

This package comes with a interactive script for setting up the Device Client. This script can be used to generate
initial Device Client configuration, as well as setup the Device Client as a service.

To use the script, run the following command.

```
./setup.sh
```

When setting up the Device Client as a service, you will be asked to provide the location of the Device Client binary
as well as the service file. For an example of the service file, see the file included in this repository at 
`setup/device-client.service`. You can also accept the default location during this step by hitting return.

You should also make sure to run the script as root or via `sudo` if you are installing the AWS IoT Device Client as
 a service as follows:
 
 ```
sudo ./setup.sh
```

*Note: The Jobs, Secure Tunneling, and Device Defender features are **enabled** by default, while the Fleet Provisioning feature is **disabled** by default. You can use the JSON config file and/or CLI options to enable/disable any of these features.*

You can use the following command to see all of the CLI options available for the Device Client

 ```
./aws-iot-device-client --help
```
  
### Configuring the AWS IoT Device Client
Configuration information can be passed to the AWS IoT Device Client through either the command line, a JSON configuration
file, or both. For a complete list of available command line arguments, pass `--help` to the executable. 

Note: Parameters that are passed through the command line take precedence and overwrite any values that may have been written in the JSON configuration file. 

To configure the AWS IoT Device Client via the JSON configuration file, you can also modify our configuration template 
`config-template.json` located in the root of this package and store it at `~/.aws-iot-device-client/aws-iot-device-client.conf`
where `~/` refers to the home directory of the user running the Device Client. If you'd like to specify a different location
for your configuration file, you can pass the `--config-file <your-path>` flag to the AWS IoT Device Client. 

There are five (5) fields that MUST be passed to the AWS IoT Device Client through some combination of either CLI arguments, 
JSON configuration, or both:

`endpoint`: This is the IoT Core endpoint that your device should connect to. This can be found by navigating to the settings
page under AWS IoT within the AWS Console under the subcategory "Custom Endpoint". You can also find the correct value for this 
field by running `aws iot describe-endpoint` on the CLI. 

`cert`: If your devices certificates were provisioned manually, then this is the path to your device's public certificate. 

`key`: If your device's certificates were provisioned manually, then this is the path to your device's private key. 

`root-ca`: This is the path to your device's Root Certificate Authority. (https://www.amazontrust.com/repository/AmazonRootCA1.pem)

`thing-name`: This is the name for your thing. It should be unique across your regional AWS account. 

## File and Directory Permissions
The AWS IoT Device Client requires specific permissions on files and directory storing these files. Please make sure these permissions are applied on files and directories which are created manually by user before starting the Device Client.

*Note: The numbers mentioned bellow are Chmod Permissions for File or Directory*

#### Recommended and Required permissions on files
File          | Chmod Permissions | Required |
------------- | ------------- | -------------
Private Keys  | 600 | **Yes**
Public Certificates | 644 | **Yes**
Root Certificate Authority | 644 | **Yes**
CSR File  | 600 | **Yes**
Log File  | 600 | **Yes**
Job Handler | 700 | **Yes**
Config File | 644 | **Recommended**


#### Recommended and Required permissions on directories storing respective files
Directory     | Chmod Permissions | Required |
------------- | ------------- | ------------- 
Directory Storing Private Key | 700 | **Yes**
Directory Storing Public Certificates   | 700 | **Yes**
Directory Storing Root Certificate Authority | 700 | **Yes**
Directory Storing CSR File  | 700 | **Yes**
Directory Storing Log File  | 745 | **Yes**
Directory Storing Config Files | 745 | **Recommended**

*Note: It is worth noting here that files are directories storing these files created by AWS IoT Device Client will have the above mentioned permissions set by default*

## Jobs Feature

The Jobs feature within the AWS IoT Device Client provides device-side functionality for execution of jobs created via
https://docs.aws.amazon.com/iot/latest/developerguide/iot-jobs.html. When the Jobs feature starts within the AWS IoT Device Client, 
the feature will attempt to establish subscriptions to important notification topics, and will also publish
a request to receive the latest pending jobs. It will use the shared MQTT connection to subscribe/publish to these
topics. 

When a new Job is received via these topics, the Jobs feature will look for and extract the following required and optional elements:


### Creating a Job

[View a sample job document here](sample-job-docs/install-packages.json)

The following fields are extracted from the incoming Job document by the Jobs feature within the AWS IoT Device Client when a 
job is received.

`operation` *string* (Required): This is the executable or script that you'd like to run. Here we can specify a script located
in the Device Client's handler directory, such as `install-packages.sh`, or it could be a well known executable binary
in the Device Client's environment path, such as `echo` or `apt-get`.

`args` *string array* (Optional): This is a JSON array of string arguments which will be passed to the specified operation. For example, 
if we want to run our `install-packages.sh` to install the package `ifupdown`, our args field would look like:
```
...
"args": ["ifupdown"],
...
```
If we wanted our remote device to print out "Hello World", then our job document so far might look like:
```
{
    "operation": "echo",
    "args": ["Hello world!"]
}
```

`path` *string* (Optional): This field tells the Jobs feature where it should look to find an executable that matches the specified
operation. If this field is omitted, the Jobs feature will assume that the executable can be found in its path. If
you expect that the executable (operation) should be found in the Job feature's handler directory 
(~/.aws-iot-device-client/jobs by default or specified via command line/json configuration values), then the path
must be specified as part of the job document as follows:
```
...
"path": "default",
...
```
The path could also be specified as a particular directory that is expected on the device, such as:
```
...
"path": "/home/ubuntu/my-job-handler-folder"
...
```

`includeStdOut` *boolean* (Optional): This field tells the Jobs feature whether the standard output (STDOUT) of the child process
should be included in the job execution update that is published to the IoT Jobs service. For example:
```
...
"includeStdOut": true,
...
```

`allowStdErr` *integer* (Optional): This field tells the Jobs feature whether a specific number of lines of output on STDERR
from the job handler may be acceptable. If this field is omitted, the Jobs feature will use a default of 0 (zero), meaning
that any lines of output on STDERR will cause the job to be marked as failed. For example: 
```
...
"allowStdErr": 2,
...
```
This job document indicates that the Jobs feature will mark the job as a success if the return code of the process is 0 AND
there are fewer than or equal to 2 (two) lines of standard error output from the job handler. 

### Creating your own Job Handler

The Jobs feature within the AWS IoT Device Client handles establishing and maintaining a connection to AWS IoT Core and making
sure that jobs are delivered to your device, but Job handlers represent the actual business logic that you want to execute on your
device. 

For example, we might want to create a new application written in Python called "foo" that should be run whenever our
device receives a "foo" job. After writing the foo application, which for this particular example might only consist of a single
file called "Foo.py", here's the steps that we would need to go through to set our AWS IoT Device Client to automatically trigger
foo:

1. Add Foo.py to our Job Handler Directory: By default, the Jobs feature will look at ~/.aws-iot-device-client/jobs/ for a handler, 
so we should put Foo.py here (where `~/` represents the home directory of the user that will run the Device Client - /root if we're
running the AWS IoT Device Client as a service, or /home/ubuntu/ if we're running under the default Ubuntu user). The new path
then looks something like /home/ubuntu/.aws-iot-device-client/jobs/Foo.py. 

2. Set Foo's file permissions: All job handlers that we place in the AWS IoT Device Client's handler directory need to have
permissions of `700`, meaning the user that owns it has read, write, and execute permissions while all other users
do not have any permissions for this file. You can typically set these permissions correctly with `chmod 700 Foo.py` on most linux systems.

3. Make sure the AWS IoT Device Client is running: If the AWS IoT Device Client is already running, you can skip this step since
the AWS IoT Device Client will dynamically load your Job handle when it receives a corresponding job. If it's not running, 
you can start the client as the current user by running `./aws-iot-device-client`, or if you've already installed the client
as a service, you can probably run something similar to `sudo systemctl start aws-iot-device-client.service`

4. Create a Job targeting your thing: Now that the AWS IoT Device Client is running on our device, we can create a job either through
the AWS Console or through the AWS CLI. For example, here's how we might create a job targeting a device on the command line:
```
aws iot create-job --job-id run-foo --document "{\"operation\": \"Foo.py\", \"path\": \"default\"}" --targets arn:aws:iot:us-west-2:XXXXXXXXXX:thing/my-thing
```

After creating this job, a notification will be sent from the AWS IoT Core endpoint via MQTT to the AWS IoT Device Client running
on your target thing. The Device Client will automatically invoke Foo.py and will update the job execution status once Foo.py returns. 

For more examples of how a Job handler can be implemented, check out the sample job handlers within this package under the
`sample-job-handlers` directory. 

#### Job/Job Handler Security Considerations

When creating a job, it's critically important that you consider the risks associated with executing particular commands
on the device. For example, running a job that prints the contents of secure credentials to STDOUT or STDERR will cause
these contents to enter the AWS IoT Device Client logs, and may run the risk of exposing these credentials to an
attacker if the logs are not properly secured.

We recommend avoiding execution of any jobs that may reveal or leak sensitive information such as credentials
, private keys, or sensitive files as well as assignment of appropriately restrictive permissions for your credentials
/sensitive files. 

### Debugging your Job

Once you've set up your job handlers and started targeting your thing or thing group with jobs, you may need to perform
a diagnosis in the event that your job fails. The Jobs feature within the AWS IoT Device Client automatically publishes
the last `1024` characters (this is a maximum limit enforced by the Jobs service) of your job process's standard error output as part of the job execution update. To view
the standard error (STDERR) output from your device, use the AWS CLI as shown in the example below to describe
the job execution. The STDERR output is shown in the response body as part of the status details in a field
labeled 'stderr'. 

```
aws iot describe-job-execution --region us-west-2 --job-id 7ff399d0-87f7-rebootstage0-reboot-setup --thing-name my-test-thing
{
    "execution": {
        "jobId": "7ff399d0-87f7-rebootstage0-reboot-setup",
        "status": "FAILED",
        "statusDetails": {
            "detailsMap": {
                "reason": "Exited with status: 1",
                "stderr": "mv: cannot stat '/sbin/reboot-invalid': No such file or directory\n"
            }
        },
        "thingArn": "arn:aws:iot:us-west-2:XXXXXXXXXX:thing/my-test-thing",
        "queuedAt": "2020-12-02T14:48:38.705000-08:00",
        "startedAt": "2020-12-02T16:15:11.677000-08:00",
        "lastUpdatedAt": "2020-12-02T16:15:11.677000-08:00",
        "executionNumber": 1,
        "versionNumber": 2
    }
}

```

If the STDERR output alone is insufficient, we recommend setting the `includeStdOut` flag to true within your Job document. 
By setting this flag to true, the Jobs feature within the AWS IoT Device Client will publish the last `1024` characters
of standard output (STDOUT) as part of the job execution update to assist with debugging. Below is an example of how
to add this to your job document:

```
...
"includeStdOut": true,
...
```

### Jobs Build Flags
`-DEXCLUDE_JOBS`: If this flag is set to ON, symbols related to the Jobs feature will not be included in the output binary.
If this flag is not included, the Jobs feature will be included in the output binary. 

Example: 
```
cmake ../ -DEXCLUDE_JOBS=ON
```
### Jobs Feature Configuration Options
`enabled`: Whether or not the jobs feature is enabled (True/False).
 
`handler-directory`: A path to a directory containing scripts or executables that the Jobs feature should look in
when receiving an incoming job. If there is a script or executable in the directory that matches the name of the
operation passed as part of the incoming job document, the Job feature will automatically execute it and will
update the job based on the performance of the specified executable. This directory should have permissions
of `700`, and any script/executable in this directory should have permissions of `700`. If these permissions are not found, 
the Jobs feature will not execute the scripts or executables in this directory. 

#### Configuring the Jobs feature via the command line:
```
./aws-iot-device-client --enable-jobs [true|false] --jobs-handler-dir [your/path/to/job/handler/directory/]
```

Example:
```
./aws-iot-device-client --enable-jobs true --jobs-handler-dir ~/.aws-iot-device-client/jobs/
```

#### Configuring the Jobs feature via the JSON configuration file:
```
    {
        ...
        "jobs": {
            "enabled": [true|false],
            "handler-directory": "[your/path/to/job/handler/directory/]"
        }
        ...
    }
```

Example: 
```
    {
        ...
        "jobs": {
            "enabled": true,
            "handler-directory": "~/.aws-iot-device-client/jobs/"
        }
        ...
    }
```

## Fleet Provisioning Feature

The AWS IoT Device Client has the capability to provision your IoT device when you connect it to AWS IoT Core for the first time. The device client provides two different mechanisms for provisioning your device (1) using a claim certificate and private key and (2) using a CSR file (along with claim certificate and key) to securely provision the device while keeping the user private key secure. After all required information is provided, the Fleet Provisioning Feature will provision, register the device with AWS IoT Core, and then establish a connection to IoT Core that is ready for use by other Device Client features. 

When the AWS IoT Device Client's Fleet Provisioning feature is enabled and provisions the device for the first time, it will first create a permanent certificate, private key (if required), and will then attach a policy to the certificate in IoT Core which will provide the device with the permissions required to run other Device Client features such as Jobs, Secure Tunneling, and Device Defender. Once these resources are created correctly, Fleet Provisioning feature will then create and register the thing/device with AWS IoT Core which will complete the provision process for the device. 

More details about AWS IoT Fleet Provisioning by claim can be found here: https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html

*Note: If the fleet provisioning feature fails to provision the new key, certificate or thing/device, the device client will abort with fatal error.*

*Note: If a CSR file is not provided, the Device Client will use Claim Certificate and Private key for provisioning the device.*

*Note: Please make sure that the Claim certificate, private key and/or CSR file stored on device side have respective permissions applied to it as mentioned above in the "File and Directory Permissions" section of the readme.*

Refer to the [CreateKeysAndCertificate](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-keys-cert) and [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) APIs for more details.

### Resources required for Fleet Provisioning feature

The AWS IoT Device Client's Fleet Provisioning feature will require the following resources for provisioning the device. The Claim Certificate and Private Key will be used to create a secure MQTT connection between Device Client and AWS IoT Core. A CSR file is only required if you want to use the [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API for creating a permanent certificate. 

* Claim Certificate
* Private Key
* CSR File (Required for creating certificate using [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API)
* Fleet Provisioning Template

### Sample Claim Certificate Policy

Claim Certificate policy will allow the Device Client to use the claim certificate to securely connect to AWS IoT Core and provision the device. Please note, the Claim certificate policy example provided below restricts the Device Client to only provision the device; it does not enable the Device Client to interact with any other cloud side features apart from Fleet Provisioning. 

You can navigate to the *AWS IoT console -> Secure -> Policies* to create and attach a policy to the claim certificate.

##### Sample Policy:

```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:1234567890:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Receive"
      ],
      "Resource": "arn:aws:iot:us-east-1:1234567890:topic/$aws/certificates/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:us-east-1:1234567890:topicfilter/$aws/certificates/*"
    }
  ]
}

```

### Sample Fleet Provisioning Template

A Fleet Provisioning template is a JSON document that uses parameters to describe the resources your device must use to interact with AWS IoT. When you use the Fleet Provisioning feature of the Device Client along with a template for your device, it automates the creation of these resources as part of the provisioning process.

You can navigate to the *AWS IoT console -> Onboard -> Fleet Provisioning* Templates to create a Fleet Provisioning Template.

##### Sample Template:

```
{
  "Parameters": {
    "AWS::IoT::Certificate::Id": {
      "Type": "String"
    }
  },
  "Resources": {
    "certificate": {
      "Properties": {
        "CertificateId": {
          "Ref": "AWS::IoT::Certificate::Id"
        },
        "Status": "Active"
      },
      "Type": "AWS::IoT::Certificate"
    },
    "policy": {
      "Properties": {
        "PolicyName": "FPCertPolicy"
      },
      "Type": "AWS::IoT::Policy"
    },
    "thing": {
      "OverrideSettings": {
        "AttributePayload": "MERGE",
        "ThingGroups": "DO_NOTHING",
        "ThingTypeName": "REPLACE"
      },
      "Properties": {
        "AttributePayload": {},
        "ThingGroups": [],
        "ThingName": {
          "Fn::Join": [
            "",
            [
              "FPTemplateName-",
              {
                "Ref": "AWS::IoT::Certificate::Id"
              }
            ]
          ]
        }
      },
      "Type": "AWS::IoT::Thing"
    }
  }
}
```

### Sample Permanent Certificate Policy

Create and attach the permanent certificate policy to the Fleet provisioning template. All new certificates created using your Fleet Provisioning template will have this certificate policy attached to it by default, which will provide the device with the permissions required to invoke other Device Client features such as Jobs, Secure Tunneling, and Device Defender. 

You can navigate to the *AWS IoT console -> Secure -> Policies* to create a permanent certificate policy.

##### Sample (FPCertPolicy) Policy:

```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:1234567890:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Receive"
      ],
      "Resource": "arn:aws:iot:us-east-1:1234567890:topic/$aws/things/${iot:Connection.Thing.ThingName}/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:us-east-1:1234567890:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/*"
    }
  ]
}
```

### Fleet Provisioning Runtime Configuration File
Once the device is provisioned correctly, Fleet Provisioning feature will validate and store the information about newly provisioned resources in a runtime config file on your device. This file will be further used by the Device Client while connecting to AWS IoT Core. 

The information stored in the runtime config file **created by Fleet Provisioning feature**:
* Thing Name: Name of the newly provisioned thing
* Certificate: Path of the newly created certificate file
* Private Key: Path of the newly created private key file
* FP Status: A boolean value stating if the Fleet Provision process was completed earlier. 

The runtime-config file is stored on your device at ```~/.aws-iot-device-client/aws-iot-device-client-runtime.conf```.
If the AWS IoT Device Client is restarted in future, it reads the runtime config file, and will use the certificate, private key and thing name mentioned in the runtime config while connecting to AWS IoT core **only if the value of 'completed-fp' parameter is true**. 

##### Example Fleet Provisioning Runtime Configuration File:

Example runtime config created by Fleet Provisioning feature:

```
"runtime-config": {
    "completed-fp": true,
    "cert": "/path/to/newly/created/certificate.pem.crt",
    "key": "/path/to/newly/created/private.pem.key",
    "thing-name": "NewlyProvisionedThingName"
    }
}
```

*Note: If you wish to re-provision your device in the future, then you can either delete or update the runtime config file, setting the "completed-fp" parameter to "false" before starting the Device client with Fleet Provisioning feature enabled.*

### Fleet Provisioning Feature Configuration Options:

The Fleet Provisioning feature is disabled by default. You can use the JSON config file and/or CLI options to enable/disable the feature.

To get started with the feature you will need to set the right configuration. This consists of two required parameters and one optional parameter

**Required Parameters:**

`enabled`: Whether or not the Fleet Provisioning feature is enabled (True/False).
 
`template-name`: The Fleet Provisioning Template name

**Optional Parameter:**

`csr-file`: Path to the CSR file. If CSR file is not provided, the Device Client will use Private key and Claim certificate for provisioning the device

#### Configuring the Fleet Provisioning feature via the command line:
```
$ ./aws-iot-device-client --enable-fleet-provisioning [true|false] --fleet-provisioning-template-name [Fleet-Provisioning-Template-Name] --csr-file [your/path/to/csr/file] 
```

#### Configuring the Fleet Provisioning feature via the JSON configuration file:
```
{
    ...
    "fleet-provisioning": {
        "enabled": [true|false],
        "template-name": "Fleet-Provisioning-Template-Name",
        "csr-file": "your/path/to/csr/file"
    }
    ...
}
```

*Note: If CSR file is not provided, the Device Client will use Claim Certificate and Private key for provisioning the device.*


## Device Defender Feature
The Device Defender feature within the AWS IoT Device Client publishes [device-side metrics](https://docs.aws.amazon.com/iot/latest/developerguide/detect-device-side-metrics.html) about the device to the cloud.  You can then use the cloud-side service to identify unusual behavior that might indicate a compromised device by monitoring the behavior of your devices.

### Device Defender Feature Configuration Options
To get started with the feature you will need to set the right configuration. This consists of two parameters

`enabled`: Whether or not the Device Defender feature is enabled (True/False).
 
`device-defender-interval`: Defines the interval in seconds between each cycle of gathering and reporting Device Defender metrics. The client-side Device Defender feature gathers your device side metrics and posts them to the Device Defender cloud service.

#### Configuring the Device Defender feature via the command line:
```
$ ./aws-iot-device-client --enable-device-defender [true|false] --device-defender-interval 300
```

#### Configuring the Device Defender feature via the JSON configuration file:
```
{
  ...
 "device-defender":	{
		"enabled":	true,
		"interval": 300
  }
  ...
}
```
*It is important to note the interval's recommended minimum is 300 seconds, anything less than this is subject to being throttled.*
Starting the AWS IoT Device Client will now start the Device Defender feature.  The device will begin publishing reports with all of the available [device-side metrics](https://docs.aws.amazon.com/iot/latest/developerguide/detect-device-side-metrics.html) (*You can see an example report at the bottom of the link*).

The rest of the functionality and interaction with Device Defender will be on the cloud-side, where you can create security profiles and alarms to monitor the metrics your device publishes. In order to learn more about the cloud side features, please refer [How to use AWS IoT Device Defender detect](https://docs.aws.amazon.com/iot/latest/developerguide/detect-HowToHowTo.html).

## Secure Tunneling Feature
The Secure Tunneling feature allows you to gain access to a remote device even if the device is behind a firewall. You may want to remote access to a device to troubleshoot, or update its configuration. For complete reference of Secure Tunneling, please see [here](https://docs.aws.amazon.com/iot/latest/developerguide/secure-tunneling.html).

Without the Device Client, if you wanted secure privileged access to a device by secure tunneling, you would need to build and deploy a compatible binary of [local proxy](https://docs.aws.amazon.com/iot/latest/developerguide/local-proxy.html) onto the device. You'd also need to write, build and deploy [code](https://docs.aws.amazon.com/iot/latest/developerguide/agent-snippet.html) that subscribes to the MQTT new tunnel topic and launches the local proxy. When you use the Device Client, you can skip building the local proxy for your IoT device and writing code to subscribe to the relevant MQTT topics. You can simply build and deploy the Device Client to your IoT devices and enable the Secure Tunneling feature.

### Secure Tunneling Feature Configuration Options

You can enable or disable the Secure Tunneling feature by a CLI argument or in the configuration file.

`enabled`: Whether or not the Secure Tunneling feature is enabled (True/False).

#### Configuring the Secure Tunneling feature via the command line:
```
$ ./aws-iot-device-client --enable-tunneling [true|false]
```

#### Configuring the Secure Tunneling feature via the JSON configuration file:
```
{
  ...
  "tunneling": {
    "enabled": [true|false]
  }
  ...
}
```

### Example steps to use the Secure Tunneling feature
Here is a sample workflow to access a remote device using the Secure Tunneling feature.

On the remote device:
1. Deploy and run the Device Client with Secure Tunneling feature enabled

On your laptop:
1. Create a new secure tunnel on the AWS Console
2. Copy the source access token
3. Start the local proxy with the source access token. For example:  
```
$ ./localproxy -r us-east-1 -s 8080 -t <source token>
```
4. Start the SSH client but connect to the local proxy listening port. For example:
```
$ ssh -p 8080 <remote_user_name>@localhost
```

### Limitation
The Secure Tunneling feature within the AWS IoT Device Client currently does not support [multiplex data streams](https://docs.aws.amazon.com/iot/latest/developerguide/multiplexing.html). However the feature supports multiple tunnels each with a single data streams.

## Logging
The AWS IoT Device Client has the capability to log directly to standard output or write logs to a log file. For
either option, the logging level can be specified as either DEBUG, INFO, WARN, or ERROR. The logger implementation
can be specified as either "STDOUT" (for standard output) or "FILE" (for logging to a file). If file based logging
is specified, you can also specify a file to log to. If a file is not specified, the Device Client will log to 
the default log location of `/var/log/aws-iot-device-client/aws-iot-device-client.log`. Keep in mind that the Device Client will need
elevated permissions to log to this location, and will automatically fall back to STDOUT logging if the Device Client
is unable to log to either the specified or default location. 

### Logging Configuration Options

#### Configuring the logger via the command line:
```
./aws-iot-device-client --log-level WARN --log-type FILE --log-file ./aws-iot-device-client.log
```

#### Configuring the logger via the JSON configuration file:
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

## Documentation

The AWS IoT Device Client project supports the generation of [Doxygen](https://www.doxygen.nl/index.html) documentation.
To generate the documentation, you'll first need to install Doxygen on your machine. This can be done using Homebrew on Mac
as follows:

```
brew install doxygen # https://formulae.brew.sh/formula/doxygen
```

and on Linux (Ubuntu):

```
sudo apt-get install doxygen
```

Once Doxygen is installed on your machine, you can generate the documentation as follows:

```
cd doc # Navigate into the documentation folder
doxygen # Doxygen bin generates HTML and Latex output
```

To view the HTML documentation, you can then navigate into the HTML folder and open `index.html` in your preferred web browser. 

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

This project is licensed under the Apache-2.0 License.
