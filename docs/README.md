# Doxygen Documentation
 **Notice:** Running the AWS IoT Device Client will incur usage of AWS IoT services, and is likely to incur charges on your AWS account. Please refer the pricing pages for [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/), [AWS IoT Device Management](https://aws.amazon.com/iot-device-management/pricing/), and [AWS IoT Device Defender](https://aws.amazon.com/iot-device-defender/pricing/) for more details.

[*Back To The Main Readme*](../README.md)

## Doxygen Documentation

The AWS IoT Device Client project supports the generation of [Doxygen](https://www.doxygen.nl/index.html) documentation.
To generate the documentation, you'll first need to install Doxygen on your machine. This can be done using Homebrew on Mac
as follows:

```
brew install doxygen # https://formulae.brew.sh/formula/doxygen
```

and on Linux (Ubuntu):

```
sudo apt-get install doxygen
```

Once Doxygen is installed on your machine, you can generate the documentation as follows:

```
cd docs # Navigate into the documentation folder
doxygen # Doxygen bin generates HTML and Latex output
```

To view the HTML documentation, you can then navigate into the HTML folder and open `index.html` in your preferred web browser.

[*Back To The Top*](#doxygen-documentation)