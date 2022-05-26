# Provisioning with Secure Elements (HSM/TPM)
**Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

* [Provisioning with Secure Elements (HSM/TPM)](#provisioning-with-secure-element-hsmtpm)
    + [Overview](#overview)
      - [Fleet Provisioning with Secured Elements](#fleet-provisioning-with-secured-elements)
      - [Store Private Key in Security Module ](#store-private-key-in-security-module)
      - [Provisioning with Secure Elements Feature Configuration Options ](#provisioning-with-secure-elements-feature-configuration-options)
    + [Building And Starting Device Client ](#building-and-starting-device-client)
      - [Building Device Client](#building-device-client)
      - [Configure SoftHSM2 and start Device Client](#configure-softhsm2-and-start-device-client)
    + [Security Best Practices](#security-best-practices) 
        
[*Back To The Main Readme*](../../README.md)

## Overview

The Device Client handles MQTT connectivity with AWS IoT core - it enables your IoT device to automatically connect and make subscriptions to feature-relevant MQTT topics. The client uses combination of certificate and private key registered with AWS IoT for MQTT connectivity with AWS IoT core. The Device Client provides support for private key stored in secure element. It will use PKCS#11 APIs (Cryptoki) to access the private key stored on a PKCS#11 compatible smart card or Hardware Security Module (HSM).

For more details regarding how AWS IoT Device SDK initializes and uses PKCS#11 (Cryptoki) library, refer these documents: 
1. [TlsContext Pkcs11 Options](https://aws.github.io/aws-iot-device-sdk-cpp-v2/class_aws_1_1_crt_1_1_io_1_1_tls_context_pkcs11_options.html) 
2. [Pkcs11Lib Class ](https://aws.github.io/aws-iot-device-sdk-cpp-v2/class_aws_1_1_crt_1_1_io_1_1_pkcs11_lib.html)  

***The minimum version of PKCS#11 API (Cryptoki) library supported by Device Client is v2.20.*** 

### Fleet Provisioning with Secured Elements

The AWS IoT Device Client offers the ability to automatically provision your IoT device when you first connect it to AWS IoT Core. The device client provides two different mechanisms for provisioning your device (1) using a claim certificate and private key OR (2) using a Certificate Signing Request (CSR) along with claim certificate and key to securely provision the device while keeping the user private key secure. 

When you use Fleet Provisioning with Secure Elements, the AWS IoT Device Client currently only supports the use of Certificate Signing Requests (CSRs). The AWS IoT Device Client does not support writing a new private key to your Secure Element since this is against security best practices.

More details about the Fleet Provisioning feature can be found here: [Fleet Provisioning Feature README](../fleetprovisioning/README.md)

**Note:** In the above-mentioned scenario, the runtime config file will store empty file path for **key** if no file path is provided.

### Store Private Key in Security Module

The user is responsible for creating and storing the private key on a PKCS#11 compatible smart card or Hardware Security Module (HSM). The AWS IoT Device Client **does not** support writing a new private key to your Secure Element since this is against security best practices.

### Provisioning with Secure Elements Feature Configuration Options

The Provisioning with Secure Elements feature is **disabled** by default. You can use the JSON config file and/or CLI options to enable/disable the feature. 

To get started with the feature you will need to set the right configuration. This consists of three required parameters and three optional parameters

**Required Parameters:**

`enabled`: Whether or not the Provisioning with Secure Elements feature is enabled (True/False).

`pkcs11-lib`: Path to PKCS#11 library.

`secure-element-pin`: User PIN for logging into PKCS#11 token.

**Optional Parameter:**

`secure-element-key-label`: Label of private key on the PKCS#11 token (optional).

`secure-element-slot-id`: Numeric Slot ID containing PKCS#11 token to use (optional).

`secure-element-token-label`: Label of the PKCS#11 token to use (optional).

#### Configuring the Provisioning with Secure Elements feature via the command line
```
$ ./aws-iot-device-client --enable-secure-element [true|false] --pkcs11-lib [your/path/to/pkcs#11/library] --secure-element-pin [User PIN of PKCS#11 token] --secure-element-key-label [key-label] --secure-element-slot-id [token-slot-id] --secure-element-token-label [token-label]
```
#### Configuring the Provisioning with Secure Elements feature via the JSON configuration file
```
{
    ...
    "secure-element": {
		"enabled": false,
		"pkcs11-lib": "<replace_with_pkcs11_lib_path>",
		"secure-element-pin": "<replace_with_secure_element_pin>",
		"secure-element-key-label": "<replace_with_secure_element_key_label>",
		"secure-element-slot-id": replace_with_secure_element_slot_id_integer,
		"secure-element-token-label": "<replace_with_secure_element_token_label>"
	}
    ...
}
```

**Note:** If Provisioning with Secure Elements feature is disabled, Device Client will look for private key on the file system.


## Building And Starting Device Client

The following example shows how you can build and start the Device Client with Provisioning using Secure Element feature enabled. In this example, we will use SoftHSM2 for storing private key. 

### Building Device Client

The following commands should work for most users when you plan to run the AWS IoT Device Client on the same machine that you're performing the compilation on:

```
# Building
git clone https://github.com/awslabs/aws-iot-device-client
cd aws-iot-device-client
mkdir build
cd build
cmake -DEXCLUDE_SECURE_ELEMENT="OFF" ../
cmake --build . --target aws-iot-device-client

# Setup
cd ../
./setup.sh # At this point you'll need to respond to prompts for information, including paths to your thing certs

```

**Note:** You can use `EXCLUDE_SECURE_ELEMENT` cmake option for including or excluding Secure Element feature from binary build. Default value of this flag is set to `ON`.

```
# Building
...
cmake -DEXCLUDE_SECURE_ELEMENT="OFF" ../
...
```

**Note:** If you decide not to use `setup.sh` script, they can manually update the JSON config file or use CLI commands to set the secure element config parameters as shown above.

### Configure SoftHSM2 and start Device Client 

In this example, we will use SoftHSM2 for storing private key. 

1. Create an IoT Thing with a certificate and key if you haven't already.
2. Convert the private key into PKCS#8 format:

   ```
   openssl pkcs8 -topk8 -in <private.pem.key> -out <private.p8.key> -nocrypt
   ```
3. Install SoftHSM2:

    ```
   apt install softhsm
   ```
4. Check that it's working:

   ```
   softhsm2-util --show-slots
   ```

   If this spits out an error message, create a config file:

   * Default location: ~/.config/softhsm2/softhsm2.conf
   * This file must specify a valid token directory:

   ```
    directories.tokendir = /path/for/my/softhsm/tokens/
    ```

   * This file must specify a valid token directory:
   * Export the location of the config file using the `SOFTHSM2_CONF`

   ```
   export SOFTHSM2_CONF=/path/to/config/softhsm2.conf
   ```
7. Create token and import private key.

    You can use any values for the labels, PINs, etc

    ```
   softhsm2-util --init-token --free --label <token-label> --pin <user-pin> --so-pin <so-pin>
   ```

    Note which slot the token ended up in

    ```
   softhsm2-util --import <private.p8.key> --slot <slot-with-token> --label <key-label> --id <hex-chars> --pin <user-pin>
   ```
8. Now you can run the sample:
    
    ```
    # Run the AWS IoT Device Client
    ./aws-iot-device-client # This command runs the executable
   ```

**Note:** If you decide not to use `setup.sh` script, they can manually update the JSON config file or use CLI commands to set the secure element config parameters as shown above.

## Security Best Practices

Mentioned bellow are the recommended security best practices for using the Device Client. We recommend you to follow these to avoid any security risks.

1. The AWS IoT Device Client requires specific permissions on files and directory storing these files. Please make sure the [required permissions](../../docs/PERMISSIONS.md) are applied on files and directories which are created manually by user before starting the Device Client.
2. The AWS IoT Device Client's Secure Element feature requires user to install and pass valid PKCS#11 (Cryptoki) library file to the client. The minimum version of PKCS#11 API (Cryptoki) library supported by Device Client is v2.20. Please make sure the library version is or above the minimum requirement of v2.20. 
3. To avoid unauthorized access to the private key file, please make sure the file stored in security module should not be user extractable.

[*Back To The Top*](#secure-element-access-using-pkcs11)