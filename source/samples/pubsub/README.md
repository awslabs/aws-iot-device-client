# Pub Sub Sample
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

[*Back To The Main Readme*](../../../README.md)

## Pub Sub Sample Feature

The Pub Sub sample feature was designed to illustrate how you might take advantage of the shared mqtt connection that exists in the `SharedCrtResourceManager`.  The Pub Sub sample feature will publish data from a configured `publish-file` to a `publish-topic`, and will recieve messages on a configured `subscribe-topic` and write them to a `subscribe-file`.

The feature also allows you to retrigger the publish action by sending a message with the payload `DC-Publish` to the configured `subscribe-topic`.

The feature also allows you to retrigger the publish action by watching file changes to `publish-file` with the `publish-on-change` flag. Note: when `publish-on-change` is enabled, the feature takes advantage of the system inode notify events.

#### Configuring the Pub Sub Sample feature via the JSON configuration file
```
{
    ...
    "samples": {
		"pub-sub": {
			"enabled": false,
			"publish-topic": "replace_with_publish_topic",
			"publish-file": "replace_with_publish_file",
			"subscribe-topic": "replace_with_subscribe_topic",
			"subscribe-file": "replace_with_subscribe_file",
			"publish-on-change": true
			}
		}
	}
    ...
}
```
*It is important to note that providing files is optional.  If they are not configured or left blank, "Hello World!" will be published over MQTT by default.*

[*Back To The Top*](#pub-sub-sample)