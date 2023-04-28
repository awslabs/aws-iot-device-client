# Setup
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

[*Back To The Main Readme*](../README.md)

## Setting Up The Device Client

This package comes with an interactive script for setting up the Device Client. This script can be used to generate
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

After completing the prompts, check if the config file exists in the proper directory. 
By default, this will be `~/.aws-iot-device-client/aws-iot-device-client.conf` or `/etc/.aws-iot-device-client/aws-iot-device-client.conf` if you set up Device Client as a service.

*Note: The Jobs and Secure Tunneling features are **enabled** by default, while all other Device Client features are **disabled** by default. You can use the JSON config file and/or CLI options to enable/disable any of these features.*

You can use the following command to see all of the CLI options available for the Device Client

 ```
./aws-iot-device-client --help
```

Next [Configuring the AWS IoT Device Client](CONFIG.md)  
[*Back To The Top*](#setup)
