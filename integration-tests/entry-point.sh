#!/bin/bash
# This entry script is intended for use within the AWS IoT Device Client GitHub Actions Workflows

OUTPUT_DIR=/root/.aws-iot-device-client/
CONFIG_PATH=${OUTPUT_DIR}aws-iot-device-client.conf
CERT_DIRECTORY=${OUTPUT_DIR}certs/
CERT_PATH=${CERT_DIRECTORY}cert.crt
KEY_PATH=${CERT_DIRECTORY}key.pem
ROOT_CA_PATH=${CERT_DIRECTORY}AmazonRootCA1.pem

# create Certificate Directory
mkdir -p ${CERT_DIRECTORY}

# create output directory
mkdir -p ${OUTPUT_DIR}

# output credentials to files
echo ${CERTIFICATE} > ${CERT_PATH}
echo ${DEVICE_KEY_SECRET} > ${KEY_PATH}
echo ${AMAZON_ROOT_CA} > ${ROOT_CA_PATH}

# set file permissions
chmod 600 ${KEY_PATH}
chmod 644 ${CERT_PATH}
chmod 644 ${ROOT_CA_PATH}
chmod 700 ${CERT_DIRECTORY}
chmod 745 ${OUTPUT_DIR}

    CONFIG_OUTPUT="
    {
      \"endpoint\":	\"${IOT_ENDPOINT}\",
      \"cert\":	\"${CERT_PATH}\",
      \"key\":	\"${KEY_PATH}\",
      \"thing-name\":	\"${THING_NAME}\",
      \"root-ca\":	\"${ROOT_CA_PATH}\",
      \"logging\":	{
        \"level\":	\"INFO\",
        \"type\":	\"STDOUT\"
      },
      \"jobs\":	{
        \"enabled\": true,
        \"handler-directory\": \"${OUTPUT_DIR}jobs\"
      },
      \"tunneling\":	{
        \"enabled\":	true
      },
      \"fleet-provisioning\":	{
        \"enabled\":	true,
        \"template-name\": \"aws-iot-device-client\",
        \"device-key\": \"${KEY_PATH}\"
      }
    }"
# output the config & set permissions
echo "$CONFIG_OUTPUT" > ${CONFIG_PATH}
chmod 640 ${CONFIG_PATH}

# start ssh
service ssh start || true

./aws-iot-device-client 2>&1 &

sleep 2
pkill -f aws-iot-device-client

# Ensure /run/lock exists
mkdir -p /run/lock > dev/null

# start Device Client
./aws-iot-device-client 2>&1 &

# Wait for Fleet Provisioning
RUNTIME_CONFIG=~/.aws-iot-device-client/aws-iot-device-client-runtime.conf
TIMEOUT=60
until [ -f "$RUNTIME_CONFIG" ] ; do
  sleep 1
  ((TIMEOUT=TIMEOUT-1))
  if [ $TIMEOUT -le 0 ]; then
    break
  fi
done

# Run integration tests
./aws-iot-device-client-integration-tests $@
