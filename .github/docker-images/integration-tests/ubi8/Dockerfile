ARG BASE_IMAGE
ARG PRE_RELEASE_IMAGE
FROM  ${BASE_IMAGE} AS build_integration_tests

RUN #yum update -y && yum install -y libcurl-devel openssl-devel libuuid-devel make gcc gcc-c++ git zlib-devel

COPY ./integration-tests /root/integration-tests
RUN mkdir -p /root/integration-tests/build \
    && cd /root/integration-tests/build \
    && cmake .. -DCMAKE_BUILD_TYPE=DEBUG \
    && cmake --build . --target aws-iot-device-client-integration-tests

# Actual docker image
FROM ${PRE_RELEASE_IMAGE} AS runner

# Install openssl for libssl dependency.
RUN yum update -y && yum install -y libatomic libcurl-devel openssl-devel libuuid-devel wget && \
    rm -rf /var/cache/yum && \
    yum clean all


COPY --from=build_integration_tests /root/integration-tests/build/aws-iot-device-client-integration-tests /aws-iot-device-client-integration-tests
COPY --from=build_integration_tests /root/integration-tests/entry-point.sh /entry-point.sh

COPY ./.github/docker-images/oss-compliance /root/oss-compliance
RUN HOME_DIR=/root \
    && chmod +x ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh \
    && chmod +x ${HOME_DIR}/oss-compliance/test/test-oss-compliance.sh \
    && bash ${HOME_DIR}/oss-compliance/generate-oss-compliance.sh ${HOME_DIR} \
    && rm -rf ${HOME_DIR}/oss-compliance*

RUN chmod a+x /entry-point.sh
RUN chmod 700 /root/.aws-iot-device-client/jobs/*.sh

ENTRYPOINT ["/entry-point.sh"]
