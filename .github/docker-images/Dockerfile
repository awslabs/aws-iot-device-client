# This is a multi-stage Dockerfile used for building minimum sized images of Device Client from your current directory.
ARG OS
ARG BASE_IMAGE
FROM ${BASE_IMAGE} AS deploy

###############################################################################
# Copy and build device client
###############################################################################
COPY . /root/aws-iot-device-client
RUN mkdir -p /root/aws-iot-device-client/build \
    && cd /root/aws-iot-device-client/build \
    && cmake .. \
    && cmake --build . --target aws-iot-device-client

ENTRYPOINT ["/root/aws-iot-device-client/build/aws-iot-device-client"]

FROM ${OS} AS minimum_size

COPY --from=deploy /root/aws-iot-device-client/build/aws-iot-device-client ./aws-iot-device-client
COPY --from=deploy /root/aws-iot-device-client/sample-job-handlers /root/.aws-iot-device-client/jobs

COPY ./.github/docker-images/oss-compliance /root/oss-compliance
RUN HOME_DIR=/root \
    && chmod +x ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh \
    && chmod +x ${HOME_DIR}/oss-compliance/test/test-oss-compliance.sh \
    && bash ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh ${HOME_DIR} \
    && rm -rf ${HOME_DIR}/oss-compliance*

RUN chmod 700 /root/.aws-iot-device-client/jobs/*.sh

ENTRYPOINT ["/aws-iot-device-client"]
