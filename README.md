# AWS IoT Device Client

## Installation

### Minimum Requirements

* C++ 11 or higher
* [CMake](https://cmake.org/) 3.10+

### Note
* CI Build uses OpenSSL 1.1.1 

### Building from source

#### Build and Install All Dependencies via CMake

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

#### Build With Dependencies Already Installed

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

#### Custom Compilation - Exclude Specific IoT Features to Reduce Executable Footprint

**Description**:
The Device Client can be customized during build time to exclude certain IoT features from compilation. All features are
included by default. Use CMake variables in order to configure which features to exclude. See the project 
`CMakeLists.txt` file for the options provided and their descriptions.

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
mkdir aws-iot-device-client/build
cd aws-iot-device-client/build
cmake ../ -DEXCLUDE_DD=ON
```

### Running the tests
```
./build/test/test-aws-iot-device-client 
```

## Setting Up The Device Client

This package comes with a interactive script for setting up the Device Client. This script can be used to generate
initial Device Client configuration, as well as setup the Device Client as a service.

When setting up the Device Client as a service, you will be asked to provide the location of the Device Client binary
as well as the service file. For an example of the service file, see the file included in this repository at 
`setup/device-client.service`. You can also accept the default location during this step by hitting return. 

To use the script, run the following command.

```bash
./setup.sh
```

### Runtime Configuration
Configuration information can be passed to the AWS IoT Device Client through either the command line, a JSON configuration
file, or both. For a complete list of available command line arguments, pass `--help` to the executable. 

To configure the AWS IoT Device Client via JSON configuration file, you can also modify our configuration template 
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
### Jobs Feature Runtime Configuration Options
`enabled`: Whether or not the jobs feature should be enabled or not.
 
`handler-directory`: A path to a directory containing scripts or executables that the Jobs feature should look in
when receiving an incoming job. If there is a script or executable in the directory that matches the name of the
operation passed as part of the incoming job document, the Job feature will automatically execute it and will
update the job based on the performance of the specified executable. This directory should have permissions
of `700`, and any script/executable in this directory should have permissions of `700`. If these permissions are not found, 
the Jobs feature will not execute the scripts or executables in this directory. 

#### Configuring the Jobs feature via the command line:
```
./aws-iot-device-client --jobs-enabled [<true>|false>] --handler-directory [your/path/to/job/handler/directory/]
```

Example:
```
./aws-iot-device-client --jobs-enabled true --handler-directory ~/.aws-iot-device-client/jobs/
```

#### Configuring the Jobs feature via JSON:
```
    {
        ...
        "jobs": {
            "enabled": [<true>|<false>],
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

The AWS IoT Device Client has the capability to provision the device when logged in for the first time. The device client provides two different mechanisms for provisioning the user device (1) using claim certificate and private key and (2) using CSR file (along with claim certificate and key) to securely provision the device while keeping the user private key secure. After all required information is provided, the Fleet Provisioning Feature will provision, register the device with AWS IoT Core, and then establish a connection to IoT Core that is ready for use by other Device Client features. 

When the AWS IoT Device Client's Fleet Provisioning feature is enabled and is provisioning the device for the first time, it will first create a permanent certificate, private key (if required), and will then attach a policy to the certificate in IoT Core which will provide the device with the permissions required to run other Device Client features such as Jobs, Secure Tunneling, and Device Defender. Once these resources are created correctly, Fleet Provisioning feature will then create and register the thing/device with AWS IoT Core which will complete the provision process for the device. 

More details about AWS IoT Fleet Provisioning by claim can be found here: https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html

*Note: It is worth noting here that if fleet provisioning fails to provision the new key, certificate or thing/device, the device client will abort with fatal error.*

*Note: If CSR file is not provided, the Device Client will use Claim Certificate and Private key for provisioning the device.*

Check [CreateKeysAndCertificate](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-keys-cert) and [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API for more details.

### Resources required for Fleet Provisioning feature

The AWS IoT Device Client's Fleet Provisioning feature will require the following resources for provisioning the device. Claim Certificate and Private key will be used to create a Secure MQTT connection between Device Client and AWS IoT Core. CSR file is only required if you want to use [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API for creating permanent certificate. 

* Claim Certificate
* Private Key
* CSR File (Required for creating certificate using [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API)
* Fleet Provisioning Template

### Sample Claim Certificate Policy

Claim Certificate policy will allow the Device Client to use the claim certificate to securely connect to AWS IoT Core and provision the device. It is worth noting here that the Claim certificate policy restricts Device Client to only provision the device; meaning, it will not allow the Device Client to start/run any other feature apart from Fleet Provisioning. 

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

Fleet Provisioning template is where you will define all of the resources and their properties which the Device Client will create while provisioning the device.

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

Create and attach the permanent certificate policy to the Fleet provisioning template. All of the new certificates created using your Fleet Provisioning template will have this certificate policy attached to it by default which which will provide the device with the permissions required to run other Device Client features such as Jobs, Secure Tunneling, and Device Defender. 

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

## Secure Tunneling Feature
The Secure Tunneling feature allows you to gain access to a remote device even if the device is behind a firewall. You may want to remote access to a device to troubleshoot, or update its configuration. For complete reference of Secure Tunneling, please see [here](https://docs.aws.amazon.com/iot/latest/developerguide/secure-tunneling.html).

Without the Device Client, if you wanted secure privileged access to a device by secure tunneling, you would need to build and deploy a compatible binary of [local proxy](https://docs.aws.amazon.com/iot/latest/developerguide/local-proxy.html) onto the device. You'd also need to write, build and deploy [code](https://docs.aws.amazon.com/iot/latest/developerguide/agent-snippet.html) that subscribes to the MQTT new tunnel topic and launches the local proxy. When you use the Device Client, you can skip building the local proxy for your IoT device and writing code to subscribe to the relevant MQTT topics. You can simply build and deploy the Device Client to your IoT devices and enable the Secure Tunneling feature.

### Configuration
The Secure Tunneling feature is disabled by default. You can enable it by a CLI argument or in the configuration file.

To enable the feature by CLI:
```
$ ./aws-iot-device-client --enable-tunneling
```

To enable the feature in the configuration file:
```
{
  ...
  "tunneling": {
    "enabled": true
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
