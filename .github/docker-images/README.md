# Docker Images

The AWS IoT Device Client provides several Docker images to simplify testing and development.

## Base Images

The base images are built from the Docker files located in the [base-images](./base-images) directory. These images contain
the dependencies of the Device Client only and are intended to be used to build & run the Device Client.

## Integration Test Images

Before releasing new features on the main branch end-to-end tests are performed in the pre-release branch. As part of this
process the Docker files located in the [integration-tests](./integration-tests) directory are used to build and run the
end-to-end tests.
