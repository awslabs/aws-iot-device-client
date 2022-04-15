# Env
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

[*Back To The Main Readme*](../README.md)

## Environment Variables

In addition to the application configuration settings described in [Configuring the AWS IoT Device Client](CONFIG.md), the device client will also read from the environment variables listed below.

* `AWS_CRT_MEMORY_TRACING`
    * The AWS C-runtime aka CRT supports capturing statistics about memory allocations at runtime.
    * This diagnostic information is made configurable by the device client through the `AWS_CRT_MEMORY_TRACING` environment variable.
    * When `AWS_CRT_MEMORY_TRACING` is unset or has the value `0`, then no diagnostic information is captured by the CRT.
    * When `AWS_CRT_MEMORY_TRACING=1` aka `AWS_MEMTRACE_BYTES`, then the CRT will collect information about the size and number of allocations.
    * When `AWS_CRT_MEMORY_TRACING=2` aka `AWS_MEMTRACE_STACKS`, then the CRT will also collect the callstack for each allocation.
    * Sending the hangup signal `SIGHUP` to a running device client process when memory tracing is enabled will print the contents of the trace to the SDK log file.
    * The device client will also print the contents of the trace during shutdown when memory tracing is enabled.
    * When there are no pending allocations or memory trace is not enabled, then nothing is printed to the SDK log file.
    * Enabling memory allocation tracing has a nontrivial cost and we do not recommend that customers enable this by default for production deployments.
    
* `LOCK_FILE_PATH`
  * To enforce single instance creation, device client writes a file to a specific directory. By default, the device client will write the lockfile to `/run/lock/` and name it "devicecl.lock". 
  * To override the default directory, set `LOCK_FILE_PATH` to a writable directory e.g. `LOCK_FILE_PATH=/my/dir/`. Permissions still apply when writing to restricted directories.
  * While this should in theory enforce a single instance of device client, double check with `ps` if device client is not starting properly.
  
* `AWSIOT_TUNNEL_ACCESS_TOKEN`
    * The [Secure Tunneling Feature](../source/tunneling/README.md) allows customers to gain remote access to the device client.
    * When the secure tunneling feature is enabled, the device client will obtain the destination access token used to communicate with the secure tunnel proxy by listening for a notification on an MQTT topic.  More details are available at [Secure Tunneling Feature](../source/tunneling/README.md).
    * The `AWSIOT_TUNNEL_ACCESS_TOKEN` environment variable can be used to inject the destination access token in situations where listening for the notification on an MQTT topic is disabled.
    * Since the destination access token has a limited lifetime and will change each time a new secure tunnel is provisioned, using `AWSIOT_TUNNEL_ACCESS_TOKEN` to pass the destination access token is strictly for debugging purposes and not recommended for production deployments.

**Next**: [Version](VERSION.md)

[*Back To The Top*](#env)
