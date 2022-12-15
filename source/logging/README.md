# Logging
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

 * [Logging](#logging)
    + [Logging Configuration Options](#logging-configuration-options)
      - [Configuring the logger via the command line](#configuring-the-logger-via-the-command-line)
      - [Configuring SDK logging via the command line](#configuring-sdk-logging-via-the-command-line)
      - [Configuring the logger via the JSON configuration file](#configuring-the-logger-via-the-json-configuration-file)
      - [Configuring SDK logging via the JSON configuration file](#configuring-sdk-logging-via-the-json-configuration-file)

[*Back To The Main Readme*](../../README.md)

## Logging
The AWS IoT Device Client has the capability to log directly to standard output or write logs to a log file. For
either option, the logging level can be specified as either DEBUG, INFO, WARN, or ERROR. The logger implementation
can be specified as either "STDOUT" (for standard output) or "FILE" (for logging to a file). If file based logging
is specified, you can also specify a file to log to. If a file is not specified, the Device Client will log to 
the default log location of `/var/log/aws-iot-device-client/aws-iot-device-client.log`. Keep in mind that the Device Client will need
elevated permissions to log to this location, and will automatically fall back to STDOUT logging if the Device Client
is unable to log to either the specified or default location. 

The AWS IoT Device Client also provides the ability to enable native SDK logging from the AWS Common Runtime (CRT). By
default, enabling this functionality will log TRACE level logs and above to `/var/log/aws-iot-device-client/sdk.log`
but these defaults can be overridden as show below. If no configuration options are passed, SDK logging is disabled
by default.

### Logging Configuration Options

#### Configuring the logger via the command line
```
./aws-iot-device-client --log-level WARN --log-type FILE --log-file ./aws-iot-device-client.log
```

#### Configuring SDK logging via the command line
```
./aws-iot-device-client --enable-sdk-logging --sdk-log-level Warn --sdk-log-file /home/user/my-log-file.log
```

#### Configuring the logger via the JSON configuration file
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

#### Configuring SDK logging via the JSON configuration file
```
{
        ...
        "logging": {
            ...
            "enable-sdk-logging": true,
            "sdk-log-level": "Warn",
            "sdk-log-file": "/home/user/my-log-file.log"
        }
        ...
    }
```

[*Back To The Top*](#logging)