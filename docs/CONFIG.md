# Config
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

[*Back To The Main Readme*](../README.md)

## Configuring the AWS IoT Device Client

Configuration information can be passed to the AWS IoT Device Client through either the command line, a JSON configuration
file, or both. For a complete list of available command line arguments, pass `--help` to the executable. 

Note: Parameters that are passed through the command line take precedence and overwrite any values that may have been written in the JSON configuration file. 

To configure the AWS IoT Device Client via the JSON configuration file, you can also modify our configuration template 
`config-template.json` located in the root of this package and store it at `~/.aws-iot-device-client/aws-iot-device-client.conf`
where `~/` refers to the home directory of the user running the Device Client. If you'd like to specify a different location
for your configuration file, you can pass the `--config-file <your-path>` flag to the AWS IoT Device Client. 

There are four (4) fields that MUST be passed to the AWS IoT Device Client through some combination of either CLI arguments, 
JSON configuration, or both:

`endpoint` *or* `--endpoint`: This is the IoT Core endpoint that your device should connect to. This can be found by navigating to the settings
page under AWS IoT within the AWS Console under the subcategory "Custom Endpoint". You can also find the correct value for this 
field by running `aws iot describe-endpoint` on the CLI. 

`cert` *or* `--cert`: If your devices certificates were provisioned manually, then this is the path to your device's public certificate. 

`key` *or* `--key`: If your device's certificates were provisioned manually, then this is the path to your device's private key.

`thing-name` *or* `--thing-name`: This is the name for your thing. It should be unique across your regional AWS account. Thing Name is also used as client id while connecting to the IoT core.

**Note:** If the `root-ca` *or* `--root-ca` file path is missing, Device Client will look for the ROOT-CA file stored on the IoT device. `root-ca` is no longer a mandatory field but we would recommend users to pass in the valid path to `root-ca` file via JSON or CLI config.

**Note:** With a minimum configuration the Device Client by default will run with Jobs and SecureTunneling features enabled only.

**Next**: [File and Directory Permission Requirements](PERMISSIONS.md)

[*Back To The Top*](#config)