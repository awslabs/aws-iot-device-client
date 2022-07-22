# Device Defender
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

  * [Device Defender Feature](#device-defender-feature)
    + [Device Defender Feature Configuration Options](#device-defender-feature-configuration-options)
      - [Configuring the Device Defender feature via the command line](#configuring-the-device-defender-feature-via-the-command-line)
      - [Configuring the Device Defender feature via the JSON configuration file](#configuring-the-device-defender-feature-via-the-json-configuration-file)

[*Back To The Main Readme*](../../README.md)

## Device Defender Feature
The Device Defender feature within the AWS IoT Device Client publishes [device-side metrics](https://docs.aws.amazon.com/iot/latest/developerguide/detect-device-side-metrics.html) about the device to the cloud.  You can then use the cloud-side service to identify unusual behavior that might indicate a compromised device by monitoring the behavior of your devices.

### Device Defender Feature Configuration Options
To get started with the feature you will need to set the right configuration. This consists of two parameters

`enabled`: Whether or not the Device Defender feature is enabled (True/False).
 
`device-defender-interval`: Defines the interval in seconds between each cycle of gathering and reporting Device Defender metrics. The client-side Device Defender feature gathers your device side metrics and posts them to the Device Defender cloud service.

#### Configuring the Device Defender feature via the command line
```
$ ./aws-iot-device-client --enable-device-defender [true|false] --device-defender-interval 300
```

#### Configuring the Device Defender feature via the JSON configuration file
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

[*Back To The Top*](#device-defender)