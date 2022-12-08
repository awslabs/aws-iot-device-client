# Docker Images

The AWS IoT Device Client provides several Docker images to simplify testing and development.

## Base Images

The base images are built from the Docker files located in the [base-images](./base-images) directory. These images contain
the dependencies of the Device Client only and are intended to be used to build & run the Device Client. Pre-built base
images are publicly available through AWS ECR in the [aws-iot-device-client-base-images repository](https://gallery.ecr.aws/aws-iot-device-client/aws-iot-device-client-base-images).

## AWS IoT Device Client Images

Docker images containing Device Client binaries are also available through ECR. These images are built using this [Dockerfile](./Dockerfile)
and are multi-stage builds that are minimum sized. Images are built from both the pre-release and main branches. The pre-release
images are available for testing and experimenting with features which are still in development from the [pre-release repository](https://gallery.ecr.aws/aws-iot-device-client/pre-release).
Images built from the main branch are available from the [aws-iot-device-client repository](https://gallery.ecr.aws/aws-iot-device-client/aws-iot-device-client).

## Integration Test Images

Before releasing new features on the main branch end-to-end tests are performed in the pre-release branch. As part of this
process the Docker files located in the [integration-tests](./integration-tests) directory are used to build and run the
end-to-end tests. In order to make it easy to recreate any test failures locally, these images are pushed to the [integration-tests repository](https://gallery.ecr.aws/aws-iot-device-client/integration-tests).
These images contain both the pre-release version of the Device Client and the integration test binary.