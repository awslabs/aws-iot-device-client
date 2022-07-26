# Permissions
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.).

  * [File and Directory Permissions](#file-and-directory-permissions)
      - [Recommended and Required permissions on files](#recommended-and-required-permissions-on-files)
      - [Recommended and Required permissions on directories storing respective files](#recommended-and-required-permissions-on-directories-storing-respective-files)

[*Back To The Main Readme*](../README.md)

## File and Directory Permissions
The AWS IoT Device Client requires specific permissions on files and directory storing these files. Please make sure these permissions are applied on files and directories which are created manually by user before starting the Device Client.

*Note: The numbers mentioned bellow are Chmod Permissions for File or Directory*

#### Recommended and Required permissions on files
File          | Chmod Permissions | Required |
------------- |-------------------| -------------
Private Keys  | 600               | **Yes**
Public Certificates | 644               | **Yes**
Root Certificate Authority | 644               | **Yes**
CSR File  | 600               | **Yes**
Log File  | 600               | **Yes**
Job Handler | 700               | **Yes**
Config File | 640               | **Recommended**
HTTP Proxy Config File | 600           | **Recommended**
Pub/Sub Files | 600               | **Yes**
Sensor Pubilsh Pathname Socket | 660               | **Yes**
PKCS11 Library File | 640               | **Yes**

#### Recommended and Required permissions on directories storing respective files
Directory     | Chmod Permissions | Required |
------------- |-------------------| -------------
Directory Storing Private Key | 700               | **Yes**
Directory Storing Public Certificates   | 700               | **Yes**
Directory Storing Root Certificate Authority | 700               | **Yes**
Directory Storing CSR File  | 700               | **Yes**
Directory Storing Log File  | 745               | **Yes**
Directory Storing Config Files | 745               | **Recommended**
Directory Storing PubSub File  | 745               | **Yes**
Directory Storing Sensor Publish Pathname Socket | 700               | **Yes**
Directory Storing PKCS11 Library File  | 700               | **Yes**

*Note: It is worth noting here that files are directories storing these files created by AWS IoT Device Client will have the above mentioned permissions set by default*

**Next**: [Environment Variables](ENV.md)

[*Back To The Top*](#permissions)
