# Named Shadows
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

[*Back To The Main Readme*](../../../README.md)


## Shadow Feature
The [AWS IoT Device Shadow service](https://docs.aws.amazon.com/iot/latest/developerguide/iot-device-shadows.html) adds shadows to AWS IoT thing objects, which provides a reliable data store for devices, apps, and other cloud services to share data. The client-side Named Shadows feature enables you to control your IoT device using AWS IoT Named Shadows. Shadows can store your device's state information and make it available to your device, AWS IoT services, your custom  apps and other AWS services whether the device is online and connected to AWS IoT or not.

The following two shadow examples created in device client illustrate how you might take advantage of backend shadow sdk to store and interact with data in device shadow.

### Sample Shadow
Sample shadow is a shadow in which you can store custom data. You can set custom name of shadow you want to create or update by editing `shadow-name` in the configuration. This feature takes a data file as input, updating the target shadow with all data in this source file and writing the latest shadow document to a shadow output file every time if there is a shadow update event occurring. You can use this file to track the latest shadow data stored in the cloud. The maximum size of this file is 8 Kilobytes as defined [here](https://docs.aws.amazon.com/general/latest/gr/iot-core.html#device-shadow-limits).
There is also a file monitor added for the feature to detect any change on the input data source file when device client is running, if any change is detected device client will update the shadow automatically to ensure that we have target shadow kept syncing with the input data source file.

*Note: Device Client uses `inotify` to monitor the file change event happening under parent directory of input file. At most it could monitor 100 events by default, and the feature would stop monitoring file if this limit is reached.*

### Config Shadow
Config shadow stores the device client feature’s [configuration](https://github.com/awslabs/aws-iot-device-client/blob/main/config-template.json), and is served as reference implementation, enabling you to remotely configure the various features of the AWS IoT Device Client on their devices.
When the config shadow is enabled, the device client will create or update a shadow named as `DeviceClientConfigShadow` with the latest feature and sample's configuration.
You can also update desired value in this config shadow from cloud side to remotely configure the device client. Once the value is updated, if the device client is running you need to restart the device client to make the new change take effect. Device client also provides jobs functionality to automate this restart process remotely, so you can also choose to create a new `restart job` when jobs feature is enabled.
Device Client would also perform configuration validation check when detecting there is an update request from cloud, if a feature's configuration in shadow is not valid, device client will abort this change and still keep sync with the local configuration. You can refer to the device client logging to identify which specific configuration is invalid. 

*Note: Config Shadow only supports to store Jobs/Tunneling/DeviceDefender/SampleShadow/PubSub's configureation and all secure parts of configuration (endpoints, thing-name, keys, certs, CA and FleetProvision feature) will not be exposed in this shadow.*

*Note: Device Client will check if there is update request from cloud by checking the [delta](https://docs.aws.amazon.com/iot/latest/developerguide/device-shadow-mqtt.html#update-delta-pub-sub-topic) filed in the config shadow. If there is no update request from cloud, device client would be configured based on local setting and config shadow will be updated correspondingly.*


```
{
    ...
   "config-shadow": {
        "enabled": false
   },
   "sample-shadow": {
        "enabled": false,
        "shadow-name": "<replace_with_shadow_name>",
        "shadow-input-file": "<replace_with_shadow_input_file_path>",
        "shadow-output-file": "<replace_with_shadow_output_file_path>"
	
    }
	
    ...
}
```
*It is important to note that providing input and output files is optional.  If they are not configured or left blank, "{welcome","aws-iot}" will be published to this shadow and the `default-sample-shadow-document`file will be created under this path `~/.aws-iot-device-client/sample-shadow/` by default*

### Policy Permissions
In order to use the Shadow feature the device must first have permission to connect to IoT Core.
The device must also be able to publish, subscribe, and receive messages on the Shadow topics. You can read more [here](https://docs.aws.amazon.com/iot/latest/developerguide/device-shadow-mqtt.html).
The example policy below demonstrates the minimum permissions required for the Named Shadow feature. 
Replace the `<region>`, `<accountId`> and `<shadowName>` with the correct values.
```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:<region>:<accountId>:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/shadow/name/<shadowName>/get",
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/shadow/name/<shadowName>/update"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": [
        "arn:aws:iot:<region>:<accountId>:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/shadow/name/<shadowName>*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": [
        "arn:aws:iot:<region>:<accountId>:topic/$aws/things/${iot:Connection.Thing.ThingName}/shadow/name/<shadowName>/*"
      ]
    }
  ]
}
```
[*Back To The Top*](#Shadow_Feature)