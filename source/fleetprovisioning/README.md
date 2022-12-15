# Fleet Provisioning
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

  * [Fleet Provisioning Feature](#fleet-provisioning-feature)
    + [Fleet Provisioning with Secured Element feature](#fleet-provisioning-with-secured-element-feature)
    + [Resources required for Fleet Provisioning feature](#resources-required-for-fleet-provisioning-feature)
    + [Sample Claim Certificate Policy](#sample-claim-certificate-policy)
        * [Sample Policy](#sample-policy)
    + [Sample Fleet Provisioning Template](#sample-fleet-provisioning-template)
        * [Sample Template](#sample-template)
    + [Sample Permanent Certificate Policy](#sample-permanent-certificate-policy)
        * [Sample (FPCertPolicy) Policy](#sample-fpcertpolicy-policy)
    + [Fleet Provisioning Runtime Configuration File](#fleet-provisioning-runtime-configuration-file)
        * [Example Fleet Provisioning Runtime Configuration File](#example-fleet-provisioning-runtime-configuration-file)
    + [Fleet Provisioning Feature Configuration Options](#fleet-provisioning-feature-configuration-options)
      - [Configuring the Fleet Provisioning feature via the command line](#configuring-the-fleet-provisioning-feature-via-the-command-line)
      - [Configuring the Fleet Provisioning feature via the JSON configuration file](#configuring-the-fleet-provisioning-feature-via-the-json-configuration-file)

[*Back To The Main Readme*](../../README.md)

## Fleet Provisioning Feature

The AWS IoT Device Client has the capability to provision your IoT device when you connect it to AWS IoT Core for the first time. The device client provides two different mechanisms for provisioning your device (1) using a claim certificate and private key and (2) using a CSR file (along with claim certificate and key) to securely provision the device while keeping the user private key secure. After all required information is provided, the Fleet Provisioning Feature will provision, register the device with AWS IoT Core, and then establish a connection to IoT Core that is ready for use by other Device Client features. 

When the AWS IoT Device Client's Fleet Provisioning feature is enabled and provisions the device for the first time, it will first create a permanent certificate, private key (if required), and will then attach a policy to the certificate in IoT Core which will provide the device with the permissions required to run other Device Client features such as Jobs, Secure Tunneling, and Device Defender. Once these resources are created correctly, Fleet Provisioning feature will then create and register the thing/device with AWS IoT Core which will complete the provision process for the device. 

More details about AWS IoT Fleet Provisioning by claim can be found [here](https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html).

*Note: If the fleet provisioning feature fails to provision the new key, certificate or thing/device, the device client will abort with fatal error.*

*Note: If the CSR file is specified without also specifying a device private key, the Device Client will use Claim Certificate and Private key to generate new Certificate and Private Key while provisioning the device*

*Note: Please make sure that the Claim certificate, private key, CSR file and/or device private key stored on device side have respective permissions applied to it as mentioned above in the "File and Directory Permissions" section of the readme.*

Refer to the [CreateKeysAndCertificate](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-keys-cert) and [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) APIs for more details.

### Fleet Provisioning with Secured Element feature 

The Device Client uses combination of certificate and private key registered with AWS IoT for MQTT connectivity with AWS IoT core. It provides support for private key stored in secure element. 

With Secure Element feature enabled, Fleet Provisioning can only be done using certificate signing request (CSR). This is because the client **does not** provide support for importing private key into security module since it is not considered a security best practice.

More details about the Secure Element feature can be found here: [Secure Element Feature README](../secure-element/README.md)

*Note: In the above-mentioned scenario, the runtime config file will store empty file path for **key** if no file path is provided.*

### Resources required for Fleet Provisioning feature

The AWS IoT Device Client's Fleet Provisioning feature will require the following resources for provisioning the device. The Claim Certificate and Private Key will be used to create a secure MQTT connection between Device Client and AWS IoT Core. A CSR file is only required if you want to use the [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API for creating a permanent certificate. 

* Claim Certificate
* Private Key
* CSR File (Required for creating certificate using [CreateCertificateFromCsr](https://docs.aws.amazon.com/iot/latest/developerguide/fleet-provision-api.html#create-cert-csr) API)
* Device Private Key (Required for provisioning using CSR file)
* Fleet Provisioning Template

### Sample Claim Certificate Policy

Claim Certificate policy will allow the Device Client to use the claim certificate to securely connect to AWS IoT Core and provision the device. Please note, the Claim certificate policy example provided below restricts the Device Client to only provision the device; it does not enable the Device Client to interact with any other cloud side features apart from Fleet Provisioning. 

You can navigate to the *AWS IoT console -> Secure -> Policies* to create and attach a policy to the claim certificate.

##### Sample Policy

```
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": ["iot:Connect"],
            "Resource": "*"
        },
        {
            "Effect": "Allow",
            "Action": ["iot:Publish","iot:Receive"],
            "Resource": [
                "arn:aws:iot:<aws-region>:<aws-account-id>:topic/$aws/certificates/create/*",
                "arn:aws:iot:<aws-region>:<aws-account-id>:topic/$aws/provisioning-templates/<templateName>/provision/*"
            ]
        },
        {
            "Effect": "Allow",
            "Action": "iot:Subscribe",
            "Resource": [
                "arn:aws:iot:<aws-region>:<aws-account-id>:topicfilter/$aws/certificates/create/*",
                "arn:aws:iot:<aws-region>:<aws-account-id>:topicfilter/$aws/provisioning-templates/<templateName>/provision/*"
            ]
        }
    ]
}

```

### Sample Fleet Provisioning Template

A Fleet Provisioning template is a JSON document that uses parameters to describe the resources your device must use to interact with AWS IoT. When you use the Fleet Provisioning feature of the Device Client along with a template for your device, it automates the creation of these resources as part of the provisioning process.

You can navigate to the *AWS IoT console -> Onboard -> Fleet Provisioning* Templates to create a Fleet Provisioning Template.
More details about AWS IoT Fleet Provisioning template can be found [here](https://docs.aws.amazon.com/iot/latest/developerguide/provision-template.html).

##### Sample Template

```
{
  "Parameters": {
    "AWS::IoT::Certificate::Id": {
      "Type": "String"
    }
  },
  "Resources": {
    "certificate": {
      "Properties": {
        "CertificateId": {
          "Ref": "AWS::IoT::Certificate::Id"
        },
        "Status": "Active"
      },
      "Type": "AWS::IoT::Certificate"
    },
    "policy": {
      "Properties": {
        "PolicyName": "FPCertPolicy"
      },
      "Type": "AWS::IoT::Policy"
    },
    "thing": {
      "OverrideSettings": {
        "AttributePayload": "MERGE",
        "ThingGroups": "DO_NOTHING",
        "ThingTypeName": "REPLACE"
      },
      "Properties": {
        "AttributePayload": {},
        "ThingGroups": [],
        "ThingName": {
          "Fn::Join": [
            "",
            [
              "FPTemplateName-",
              {
                "Ref": "AWS::IoT::Certificate::Id"
              }
            ]
          ]
        }
      },
      "Type": "AWS::IoT::Thing"
    }
  }
}
```

### Sample Permanent Certificate Policy

Create and attach the permanent certificate policy to the Fleet provisioning template. All new certificates created using your Fleet Provisioning template will have this certificate policy attached to it by default, which will provide the device with the permissions required to invoke other Device Client features such as Jobs, Secure Tunneling, and Device Defender. 

You can navigate to the *AWS IoT console -> Secure -> Policies* to create a permanent certificate policy.

##### Sample (FPCertPolicy) Policy

```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:1234567890:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Receive"
      ],
      "Resource": "arn:aws:iot:us-east-1:1234567890:topic/$aws/things/${iot:Connection.Thing.ThingName}/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:us-east-1:1234567890:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/*"
    }
  ]
}
```

### Fleet Provisioning Runtime Configuration File
Once the device is provisioned correctly, Fleet Provisioning feature will validate and store the information about newly provisioned resources in a runtime config file on your device. This file will be further used by the Device Client while connecting to AWS IoT Core. 

The information stored in the runtime config file **created by Fleet Provisioning feature**:
* Thing Name: Name of the newly provisioned thing
* Certificate: Path of the newly created certificate file
* Private Key: Path of the newly created or device private key file
* FP Status: A boolean value stating if the Fleet Provision process was completed earlier. 

The runtime-config file is stored on your device at ```~/.aws-iot-device-client/aws-iot-device-client-runtime.conf```.
If the AWS IoT Device Client is restarted in future, it reads the runtime config file, and will use the certificate, private key and thing name mentioned in the runtime config while connecting to AWS IoT core **only if the value of 'completed-fp' parameter is true**. 

##### Example Fleet Provisioning Runtime Configuration File

Example runtime config created by Fleet Provisioning feature:

```
"runtime-config": {
    "completed-fp": true,
    "cert": "/path/to/newly/created/certificate.pem.crt",
    "key": "/path/to/private.pem.key",
    "thing-name": "NewlyProvisionedThingName"
    }
}
```

*Note: If you wish to re-provision your device in the future, then you can either delete or update the runtime config file, setting the "completed-fp" parameter to "false" before starting the Device client with Fleet Provisioning feature enabled.*

### Fleet Provisioning Feature Configuration Options

The Fleet Provisioning feature is disabled by default. You can use the JSON config file and/or CLI options to enable/disable the feature.

To get started with the feature you will need to set the right configuration. This consists of two required parameters and three optional parameters

**Required Parameters:**

`enabled`: Whether or not the Fleet Provisioning feature is enabled (True/False).
 
`template-name`: The Fleet Provisioning Template name

**Optional Parameter:**

`csr-file`: Path to the CSR file.

`device-key`: Path to the device private key.

`template-parameters`: The Fleet Provisioning Template parameters. A JSON object specified as an escaped string. In this example we define 'SerialNumber' with value 'Device-SN' as a parameter to the template:
```
"{\"SerialNumber\": \"Device-SN\"}"
```

*Note: If the CSR file is specified without also specifying a device private key, the Device Client will use Claim Certificate and Private key to generate new Certificate and Private Key while provisioning the device*

*Note: Provisioning process will exit with an error in case template parameters are malformed as JSON escaped string.*

#### Configuring the Fleet Provisioning feature via the command line
```
$ ./aws-iot-device-client --enable-fleet-provisioning [true|false] --fleet-provisioning-template-name [Fleet-Provisioning-Template-Name] --fleet-provisioning-template-parameters [Fleet-Provisioning-Template-Parameters] --csr-file [your/path/to/csr/file] --device-key [your/path/to/device/private/key] 
```

#### Configuring the Fleet Provisioning feature via the JSON configuration file
```
{
    ...
    "fleet-provisioning": {
        "enabled": [true|false],
        "template-name": "Fleet-Provisioning-Template-Name",
        "csr-file": "your/path/to/csr/file",
        "device-key": "your/path/to/device/private/key",
        "template-parameters": "Fleet-Provisioning-Template-Parameters",
    }
    ...
}
```

**Note**: *If the CSR file is specified without also specifying a device private key, the Device Client will use Claim Certificate and Private key to generate new Certificate and Private Key while provisioning the device*

**Note**: *If Fleet Provisioning feature is enabled, the Device Client will use the `thing-name` value as `client-id` **only** for connecting to IoT Core the first time. Once your device connects to IoT Core, and is successfully provisioned, it will automatically be assigned a new `thing-name` according to your fleet provisioning template. This new `thing-name` will replace the old `client-id` and `thing-name` when the device subscribes to MQTT topics for other enabled features.*

[*Back To The Top*](#fleet-provisioning)