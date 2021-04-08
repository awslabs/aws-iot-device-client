# Secure Tunneling 
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

  * [Secure Tunneling Feature](#secure-tunneling-feature)
    + [Secure Tunneling Feature Configuration Options](#secure-tunneling-feature-configuration-options)
      - [Configuring the Secure Tunneling feature via the command line](#configuring-the-secure-tunneling-feature-via-the-command-line)
      - [Configuring the Secure Tunneling feature via the JSON configuration file](#configuring-the-secure-tunneling-feature-via-the-json-configuration-file)
    + [Example steps to use the Secure Tunneling feature](#example-steps-to-use-the-secure-tunneling-feature)
    + [Limitation](#limitation)

[*Back To The Main Readme*](../../README.md)

## Secure Tunneling Feature
The Secure Tunneling feature allows you to gain access to a remote device even if the device is behind a firewall. You may want to remote access to a device to troubleshoot, or update its configuration. For complete reference of Secure Tunneling, please see [here](https://docs.aws.amazon.com/iot/latest/developerguide/secure-tunneling.html).

Without the Device Client, if you wanted secure privileged access to a device by secure tunneling, you would need to build and deploy a compatible binary of [local proxy](https://docs.aws.amazon.com/iot/latest/developerguide/local-proxy.html) onto the device. You'd also need to write, build and deploy [code](https://docs.aws.amazon.com/iot/latest/developerguide/agent-snippet.html) that subscribes to the MQTT new tunnel topic and launches the local proxy. When you use the Device Client, you can skip building the local proxy for your IoT device and writing code to subscribe to the relevant MQTT topics. You can simply build and deploy the Device Client to your IoT devices and enable the Secure Tunneling feature.

### Secure Tunneling Feature Configuration Options

You can enable or disable the Secure Tunneling feature by a CLI argument or in the configuration file.

`enabled`: Whether or not the Secure Tunneling feature is enabled (True/False).

#### Configuring the Secure Tunneling feature via the command line
```
$ ./aws-iot-device-client --enable-tunneling [true|false]
```

#### Configuring the Secure Tunneling feature via the JSON configuration file
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
3. Start the [local proxy](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/blob/master/README.md#options-set-via-command-line-arguments) with the source access token. For example:  
```
$ ./localproxy -r us-east-1 -s 8080 -t <source token>
```
4. Start the SSH client but connect to the local proxy listening port. For example:
```
$ ssh -p 8080 <remote_user_name>@localhost
```

### Limitation
The Secure Tunneling feature within the AWS IoT Device Client currently does not support [multiplex data streams](https://docs.aws.amazon.com/iot/latest/developerguide/multiplexing.html). However the feature supports multiple tunnels each with a single data streams.

[*Back To The Top*](#)