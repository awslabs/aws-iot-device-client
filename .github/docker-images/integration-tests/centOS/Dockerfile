ARG OS
ARG PRE_RELEASE_IMAGE
FROM  ${OS} AS build_integration_tests

RUN yum update -y && yum install -y libcurl-devel openssl-devel libuuid-devel cmake3 make gcc gcc-c++ git

COPY ./integration-tests /root/integration-tests
RUN mkdir -p /root/integration-tests/build \
    && cd /root/integration-tests/build \
    && cmake3 .. \
    && cmake3 --build . --target aws-iot-device-client-integration-tests

# Actual docker image
FROM ${PRE_RELEASE_IMAGE} AS runner

# Install openssl for libssl dependency.
RUN yum update -y && yum install -y libatomic libcurl-devel openssl-devel libuuid-devel wget && \
    rm -rf /var/cache/yum && \
    yum clean all


COPY --from=build_integration_tests /root/integration-tests/build/aws-iot-device-client-integration-tests /aws-iot-device-client-integration-tests
COPY --from=build_integration_tests /root/integration-tests/entry-point.sh /entry-point.sh

RUN chmod a+x /entry-point.sh
RUN chmod 700 /root/.aws-iot-device-client/jobs/*.sh

ENTRYPOINT ["/entry-point.sh"]
