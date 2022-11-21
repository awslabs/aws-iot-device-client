# This is a multi-stage Dockerfile used for building minimum sized images of Device Client from your current directory.
ARG OS
ARG BASE_IMAGE
FROM ${BASE_IMAGE} AS deploy

###############################################################################
# Copy and build device client
###############################################################################
COPY . /root/aws-iot-device-client
RUN mkdir -p /root/aws-iot-device-client/build
RUN cd /root/aws-iot-device-client/build
RUN pwd
RUN ls
RUN cmake ..
RUN cmake --build . --target aws-iot-device-client

ENTRYPOINT ["/root/aws-iot-device-client/build/aws-iot-device-client"]

FROM ${OS} AS minimum_size

COPY --from=deploy /root/aws-iot-device-client/build/aws-iot-device-client ./aws-iot-device-client
COPY --from=deploy /root/aws-iot-device-client/sample-job-handlers /root/.aws-iot-device-client/jobs

RUN chmod 700 /root/.aws-iot-device-client/jobs/*.sh

ENTRYPOINT ["/aws-iot-device-client"]
