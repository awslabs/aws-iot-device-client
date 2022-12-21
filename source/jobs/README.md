# Jobs
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

  * [Jobs Feature](#jobs-feature)
    + [Sample Jobs](#sample-jobs)
    + [Creating a Job using (New) Standard Job Action Document Schema ](#creating-a-job-using-(new)-standard-job-action-document-schema)
    + [Creating a Job using Old Job Document Schema ](#creating-a-job-using-old-job-document-schema)
    + [Creating your own Job Handler](#creating-your-own-job-handler)
      - [Job/Job Handler Security Considerations](#jobjob-handler-security-considerations)
    + [Debugging your Job](#debugging-your-job)
    + [Jobs Build Flags](#jobs-build-flags)
    + [Jobs Feature Configuration Options](#jobs-feature-configuration-options)
      - [Configuring the Jobs feature via the command line](#configuring-the-jobs-feature-via-the-command-line)
      - [Configuring the Jobs feature via the JSON configuration file](#configuring-the-jobs-feature-via-the-json-configuration-file)
    + [Policy Permissions](#policy-permissions)

[*Back To The Main Readme*](../../README.md)

## Jobs Feature

The Jobs feature within the AWS IoT Device Client provides device-side functionality for execution of [jobs created](https://docs.aws.amazon.com/iot/latest/developerguide/iot-jobs.html). When the Jobs feature starts within the AWS IoT Device Client, the feature will attempt to establish subscriptions to important notification topics, and will also publish a request to receive the latest pending jobs. It will use the shared MQTT connection to subscribe/publish to these topics. 

When a new Job is received via these topics, the Jobs feature will look for and extract the following required and optional elements:

### Sample Jobs
Existing [Sample Job Docs](../../sample-job-docs)
Existing [Sample Job Handlers](../../sample-job-handlers)

### Creating a Job using (New) Standard Job Action Document Schema 
 
 The following fields are extracted from the incoming Job document by the Jobs feature within the AWS IoT Device Client when a 
 job is received.
 
 `version` *string* (Required): This field specifies the version of the ***job document schema*** used to structure this job document. ***Please note, this is not the version of the Job document or Job handler.*** To version the job document or job handler, please use the filename of your job document json or handler script. 
 Currently, the Device Client will only process your job document if it carries the string value of `"1.0"` in the `version` field 
 ```
 ...
 "version": "1.0",
 ...
 ```

  `includeStdOut` *boolean* (Optional): This field determines whether the standard output (STDOUT) of the child process is included in the statusDetails field while (updating the job execution)[https://docs.aws.amazon.com/iot/latest/apireference/API_iot-jobs-data_UpdateJobExecution.html#iot-iot-jobs-data_UpdateJobExecution-request-statusDetails].
  For example:
 ```
 ...
 "includeStdOut": true,
 ...
 ```
 
 `steps` *list of Actions* (Required): This field defines the list of steps or actions you want to carry out remotely on your IoT device as part of a single Job execution.
 Each action in the list of actions will be executed in a sequential manner and will stop executing if any of the step fails to execute.
 
 `finalStep` *Action* (Optional): This field defines the final step to be executed on your IoT device as part of a single Job execution. The Device client 
 will execute `finalStep` only when all of the actions in the `steps` field are executed successfully. User can use this final step 
 for device cleanup purpose.
 
 `action` *JSON* (Required): This field defines the action to be executed on your IoT device by the Device Client. 
 Each action you want to execute on the device needs to be specified either in the `steps` field or in the `finalStep` field as described above. The properties of each step or action are further described by 5 attributes: `name`, `type`, `runAsUser`, `ignoreStepFailure` and `input`. These attributes are explained in detail below 
  
  `name` *string* (Required): This attribute defines the `name` of the step to be executed. We recommend you use an easily identifiable name for each step, since it will be reflected in the logs of the Device Client, and will help you debug any unexpected behavior.
  
  ```
  ...
  "name":"Install wget package on the device"
  ...
  ```
  `type` *string* (Required): This attribute defines the type of step to be executed. We currently support actions of the type `runHandler` or `runCommand` in `version` `"1.0"` of the Job Document Schema. `runCommand` is only supported using NEW job document schema.
    
  ```
  ...
  "type":"runHandler"
  ...
  ```  
  `runAsUser` *string* (Optional): This attribute defines the name of the local user on the IoT device that executes this single step. 
  You are responsible for creating local users on your IoT device OS. ***This directly impacts device security.*** We recommend you carefully create local users and assign permissions of least privilege to execute specific actions as a security best practice. If this field is omitted in the Job Document, 
  the Device Client will execute the step in your Job document under the same set of permissions that govern the Device Client's execution on your IoT device. ***For instance, if you start the Device Client as root, and do not provide a `runAsUser` parameter for your step in the job document, your step will be run as root.***    
  ```
  ...
  "runAsUser":"root"
  ...
  ```

  `ignoreStepFailure` *string* (Optional): This attribute defines if the Device Client should ignore the failure of this specific step in the series of steps defined in your Job document. If you set this attribute to `"true"`, the Device Client will ignore the failure of your step, and continue executing any subsequent steps that are part of your Job document.
   By default, this attribute is set to `"false"`. Note that `"true"` and `"false"` need to be passed as strings (not booleans)
    
  ```
  ...
  "ignoreStepFailure":"true"
  ...
  ```

  `input` *JSON* (Required): This attribute defines the supporting parameters / arguments required to execute your step as part of the Job execution.

  The `input` attribute consists of different fields between types. For `runHandler` type, it further consists of three fields: `handler`, `args`, and `path`. 
  
  `handler` *string* (Required): This field declares the name of the handler script to be executed for your step as part of the Job execution.
    
  ```
  ...
  "handler":"install-package.sh"
  ...
  ```
  
  `args` *string array* (Optional): This is a JSON array of string arguments which will be passed to the specified handler script while executing the aforesaid step as part of your Job execution.
   For example, to install `ifupdown` package, use `install-packages.sh` as your handler with `args` as follows:
    
  ```
  ...
  "args": ["ifupdown"]
  ...
  ```

 `path` *string* (Optional): This field stores the local path to the directory containing your handlers. If you want to run a handler that is
  ***not*** stored in the default Job handler directory for your Device Client installation, you can optionally use this field. If this field is omitted, the Device Client will assume that the right handler is already available in the default location, demarcated by the PATH environment variable.
 ***default: ~/.aws-iot-device-client/jobs***
 If you would like to explicitly use the default Job handler directory, you can declare that as follows (optional):
 ```
 ...
 "path": "default",
 ...
 ```
Else you can also specify a different handler directory for your step in the following manner:
 ```
 ...
 "path": "/home/ubuntu/my-job-handler-folder"
 ...
 ```
For `runCommand` type, `input` field consists of only one field: `command`.

`command` *string* (Required): This field stores one command provided by the customer to be executed on devices. The format of `command` needs to be comma separated. For example, `aws iot describe-endpoint --endpoint-type XXX --region XXX --endpoint https://xxxxx` needs to be comma separated into `"aws,iot,describe-endpoint,--endpoint-type,XXX,--region,XXX,--endpoint,https://xxxxx"`. If command itself contains comma, the comma needs to be escaped. For example, `echo Hello, I am Device Client.` needs to be transformed into `echo,Hello\\, I am Device Client.`. The first string of the command cannot contain any space characters. 

Lastly, the permission used to execute `command` will be the value of `runAsUser`. if no user is provided, it will use the permission of the user which runs device client. Therefore, if a command needs to use `root` permission, simply provide `root` for `runAsUser` instead of adding `sudo` in front of `command`, because device client will execute the command in the form of `sudo -u $user -n command` if `sudo` is installed and `user` is found.
```
...
"command": "echo,Hello World!"
...
```    

**Note**: Once a job document is received by Device Client and parsed successfully, `input` will be stored into `handlerInput` or `commandInput` according to the `type` of the job document. `handlerInput` consists of `handler`, `args` and `path` and `commandInput` only consists of `command`.

  **Example of Step Field:**
   
 ```
  ...
  "steps": [
      {
          "action": {
              "name": "Install-Packages",
              "type": "runHandler",
              "runAsUser": "root",
              "ignoreStepFailure":"true",
              "input": {
                  "handler": "install-packages.sh",
                  "args": [
                      "lftp",
                      "dos2unix"
                  ],
                  "path": "default"
              }
          }
      }
  ]
  ...
  ```

 ```
  ...
  "steps": [
      {
          "action": {
              "name": "print-greeting",
              "type": "runCommand",
              "runAsUser": "root",
              "ignoreStepFailure":"true",
              "input": {
                  "command": "echo,Hello World!"
              }
          }
      }
  ]
  ...
  ```

  **Example of Final Step Field:**
 
 ```
   ...
   "finalStep": 
       {
           "action": {
               "name": "Remove-Packages",
               "type": "runHandler",
               "runAsUser": "root",
               "ignoreStepFailure":"true",
               "input": {
                   "handler": "remove-packages.sh",
                   "args": [
                       "lftp",
                       "dos2unix"
                   ],
                   "path": "default"
               }
           }
       }
   ...
   ```
 
 #### Sample Job Document using New Schema
 
 **Job Document to (1)Install `wget` package, (2)download file and (3)remove `wget` package from device as a final cleanup step**
 ```
 {
   "_comment": "This sample JSON file can be used for installing wget packages on the device, then downloading specified file and cleanup by removing the installed package in the end.",
   "version": "1.0",
   "steps": [
       {
         "action": {
           "name": "Install-Packages",
           "type": "runHandler",
           "input": {
             "handler": "install-packages.sh",
             "args": [
                 "wget"
             ],
             "path": "default"
           },
           "runAsUser": "root"
       }
       },
       {
         "action": {
           "name": "Download-File",
           "type": "runHandler",
           "input": {
             "handler": "download-file.sh",
             "args": [
               "https://github.com/awslabs/aws-iot-device-client/archive/refs/tags/v1.3.tar.gz",
               "/tmp/Downloaded_File.tar.gz"
             ],
             "path": "default"
           },
           "runAsUser": "root"
         }
       }
   ],
   "finalStep": {
     "action": {
       "name": "Remove-Packages",
       "type": "runHandler",
       "input": {
         "handler": "remove-packages.sh",
         "args": [
           "wget"
         ],
         "path": "default"
       },
       "runAsUser": "root"
     }
   } 
 }
 ``` 


### Creating a Job using Old Job Document Schema 

***Note: This is the old Job document schema preserved here for backward compatibility. We will not continue active development using the old Job document schema. Therefore, we highly recommend using the new Job document schema described above.***
 
 [Sample Job Docs using Old Schema](../../old-schema-job-docs-and-handlers/old-schema-job-docs)

 [Sample Job Handlers for Old Schema Docs](../../old-schema-job-docs-and-handlers/old-schema-job-handlers)


The following fields are extracted from the incoming Job document by the Jobs feature within the AWS IoT Device Client when a 
job is received.

`operation` *string* (Required): This is the executable or script that you'd like to run. Here we can specify a script located
in the Device Client's handler directory, such as `install-packages.sh`, or it could be a well known executable binary
in the Device Client's environmentx path, such as `echo` or `apt-get`.

`args` *string array* (Optional): This is a JSON array of string arguments which will be passed to the specified handler script while executing the aforesaid step as part of your Job execution.
 For example, to install `ifupdown` package, use `install-packages.sh` as your handler with `args` as follows:
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

`path` *string* (Optional): This field stores the local path to the directory containing your handlers. If you want to run a handler that is
***not*** stored in the default Job handler directory for your Device Client installation, you can optionally use this field. If this field is omitted, the Device Client will assume that the right handler is already available in the default location, demarcated by the PATH environment variable.
***default: ~/.aws-iot-device-client/jobs***
If you would like to explicitly use the default Job handler directory, you can declare that as follows (optional):
```
...
"path": "default",
...
```
Else you can also specify a different handler directory for your step in the following manner:
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

#### Sample Job Document using Old Schema

**Install Packages Job Document**
```
{
    "operation": "install-packages.sh",
    "args": ["lftp", "dos2unix"],
    "path": "default",
    "allowStdErr": 2
}
``` 

### Creating your own Job Handler

The Jobs feature within the AWS IoT Device Client handles establishing and maintaining a connection to AWS IoT Core and making
sure that jobs are delivered to your device, but Job handlers represent the actual business logic that you want to execute on your
device. 

For example, we might want to create a new application written in Python called "foo" that should be run whenever our
device receives a "foo" job. After writing the foo application, which for this particular example might only consist of a single
file called `Foo.py`, here's the steps that we would need to go through to set our AWS IoT Device Client to automatically trigger
foo:

1. Add `Foo.py` to our Job Handler Directory: By default, the Jobs feature will look at `~/.aws-iot-device-client/jobs/` for a handler, 
so we should put `Foo.py` here (where `~/` represents the home directory of the user that will run the Device Client - `/root` if we're
running the AWS IoT Device Client as a service, or `/home/ubuntu/` if we're running under the default Ubuntu user). The new path
then looks something like `/home/ubuntu/.aws-iot-device-client/jobs/Foo.py`. 

**Note: It is important to note that the Device Client passes `runAsUser` job field value as first argument for starting the handler script and then attaches remaining job `args` list values.**

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
`enabled`: Whether or not the Jobs feature is enabled (True/False). If not specified, Jobs feature is enabled by default.
 
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

### Policy Permissions
In order to use the Jobs feature the device must first have permission to connect to IoT Core.
The device must also be able to publish, subscribe, and receive messages on the Jobs MQTT topics.
The example policy below demonstrates the minimum permissions required for the Jobs feature. 
Here we have left the JobId as a wildcard so any Job targeting the device can be run. 
Replace the `<region>` and `<accountId`> with the correct values.
```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:<region>:<accountId>:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/start-next",
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/*/update"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": [
        "arn:aws:iot:<region>:<accountId>:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/jobs/start-next/accepted",
        "arn:aws:iot:<region>:<accountId>:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/jobs/start-next/rejected",
        "arn:aws:iot:<region>:<accountId>:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/jobs/*/update/accepted",
        "arn:aws:iot:<region>:<accountId>:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/jobs/*/update/rejected",
        "arn:aws:iot:<region>:<accountId>:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/jobs/notify-next"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": [
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/start-next/accepted",
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/start-next/rejected",
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/*/update/accepted",
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/*/update/rejected",
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/jobs/notify-next"
      ]
    }
  ]
}
```

[*Back To The Top*](#jobs)