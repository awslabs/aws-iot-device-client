# Jobs
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

  * [Jobs Feature](#jobs-feature)
    + [Creating a Job](#creating-a-job)
    + [Creating your own Job Handler](#creating-your-own-job-handler)
      - [Job/Job Handler Security Considerations](#jobjob-handler-security-considerations)
    + [Debugging your Job](#debugging-your-job)
    + [Jobs Build Flags](#jobs-build-flags)
    + [Jobs Feature Configuration Options](#jobs-feature-configuration-options)
      - [Configuring the Jobs feature via the command line](#configuring-the-jobs-feature-via-the-command-line)
      - [Configuring the Jobs feature via the JSON configuration file](#configuring-the-jobs-feature-via-the-json-configuration-file)

[*Back To The Main Readme*](../../README.md)

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

#### Configuring the Jobs feature via the command line
```
./aws-iot-device-client --enable-jobs [true|false] --jobs-handler-dir [your/path/to/job/handler/directory/]
```

Example:
```
./aws-iot-device-client --enable-jobs true --jobs-handler-dir ~/.aws-iot-device-client/jobs/
```

#### Configuring the Jobs feature via the JSON configuration file
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

[*Back To The Top*](#jobs)