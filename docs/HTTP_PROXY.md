# HTTP Proxy

**Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.
* [HTTP Proxy](#http-proxy)
    + [HTTP Proxy Support](#http-proxy-support)
        - [How It Works](#how-it-works)
        - [Configuring HTTP Proxy via the JSON configuration file](#configuring-http-proxy-via-the-json-configuration-file)
            - [Sample HTTP proxy configuration](#sample-http-proxy-configuration)
            - [Configuration options](#configuration-options)
            - [Permissions](#permissions)
        - [Overriding the default configuration file from CLI](#overriding-the-default-configuration-file-from-command-line-interface)

[*Back To The Main Readme*](../../README.md)

## HTTP Proxy Support
The AWS IoT Device Client is free, open-source, modular software written in C++ that customers compile and install on Embedded Linux based IoT devices to access AWS IoT Core, AWS IoT Device Management, and AWS IoT Device Defender features. A common setup in the IoT industry is to have devices in field behind an HTTP proxy where the devices could only connect to the internet via the proxy. The HTTP Proxy support feature allows customers to configure their HTTP proxy server with username and password as authentication method when executing device client sample features.

### How It Works
The AWS IoT Device Client initiates the MQTT connection based on the implementation of aws-iot-device-sdk-cpp-v2. The HTTP proxy support for AWS IoT Device Client enables the tunneling proxy mode provided by the C++ SDK. Upon the start of the IoT Device Client, the `HttpProxyOptions` will be attached to the MQTT connection context together with the proxy IP address, port number, authentication information to connect to AWS IoT via an HTTP proxy.

### Configuring HTTP Proxy via the JSON configuration file
An HTTP Proxy configuration file is a JSON document that uses parameters to describe the proxy options used to interact with AWS IoT.

The default HTTP proxy config file should be placed on your device at `~/.aws-iot-device-client/http-proxy.conf`. AWS IoT Device Client would establish MQTT connect with HTTP proxy **only if the `http-proxy-enabled` flag in the configuration is set to `true`**.
#### Sample HTTP proxy configuration
```
{
  "http-proxy-enabled": true,
  "http-proxy-host": "10.0.0.140",
  "http-proxy-port": "9999",
  "http-proxy-auth-method": "UserNameAndPassword",
  "http-proxy-username": "MyUserName",
  "http-proxy-password": "password12345"
}
```
#### Configuration options
**Required Parameters:**

`http-proxy-enabled`: Whether or not the HTTP proxy is enabled (true/false). We read this value in as boolean so no double quotes required for the value field.

`http-proxy-host`: The host IP address of the http proxy server.

*Note:* It is required that you configure the proxy server's IP address within the reserved private IP address range. Valid range of the private IP address are including as follow:
```
Class A: 10.0.0.0 to 10.255.255.255
Class B: 172.16.0.0 to 172.31.255.255
Class C: 192.168.0.0 to 192.168.255.255
```

`http-proxy-port`: The port which the proxy server will listen on. Port number will be validated to make sure it falls into the valid range of 1 - 65535. Note that we read this field in string format so quotes are required.

`http-proxy-auth-method`: Configure HTTP "Basic Authentication" username and password for accessing the proxy. Access is only granted for authenticated users. Field is case-sensitive. Valid values are `UserNameAndPassword` or `None`.

**Optional Parameter:**

`http-proxy-username`: Username for the HTTP proxy authentication.

`http-proxy-password`: Password for the HTTP proxy authentication.

####Permissions

The HTTP proxy support for AWS IoT Device Client enforces chmod permission code of 600 on the HTTP proxy config files. It is also recommended to set chmod permission to 745 on the directory storing config files.

For more information regarding permission recommandations, check out the [permission readme page]((docs/PERMISSIONS.md))

### Overriding the default configuration file from command line interface

User can switch between different HTTP proxy config files without modifying the default config. To override the default HTTP proxy config under the file path `~/.aws-iot-device-client/http-proxy.conf`, start the AWS IoT Device Client with the `--http-proxy-config` parameter and the file path of the overriding HTTP proxy config file.

```
$ ./aws-iot-device-client --http-proxy-config ~/.aws-iot-device-client/MyProxyConfig1.conf
```
*Note:* User still need to follow the example format of the HTTP proxy configuration file. HTTP proxy will be enabled only if the `http-proxy-enabled` is set to `true`.

*Note:* File and parent folder permissions will also be enforced on the overriding configuration file.

[*Back To The Top*](#http-proxy)
