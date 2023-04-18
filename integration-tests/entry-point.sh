#!/bin/bash
# This entry script is intended for use within the AWS IoT Device Client GitHub Actions Workflows

OUTPUT_DIR=/root/.aws-iot-device-client/
CONFIG_PATH=${OUTPUT_DIR}aws-iot-device-client.conf
CERT_DIRECTORY=${OUTPUT_DIR}certs/
CERT_PATH=${CERT_DIRECTORY}cert.crt
KEY_PATH=${CERT_DIRECTORY}key.pem
ROOT_CA_PATH=${CERT_DIRECTORY}AmazonRootCA1.pem
SDK_LOG_FILE=/var/log/aws-iot-device-client/sdk.log

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
        \"level\":	\"DEBUG\",
        \"type\":	\"STDOUT\",
        \"enable-sdk-logging\": true,
        \"sdk-log-level\": \"INFO\",
        \"sdk-log-file\": \"${SDK_LOG_FILE}\"
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
      },
      \"device-defender\": {
      	\"enabled\": true,
      	\"interval\": 300
      }
    }"
# output the config & set permissions
echo "$CONFIG_OUTPUT" > ${CONFIG_PATH}
chmod 640 ${CONFIG_PATH}

# start ssh
service ssh start || true

# Ensure lockfile exists
mkdir -p /run/lock > dev/null

# Spoof lockfile
echo "${THING_NAME}" > /run/lock/devicecl.lock
bash -c 'sleep 1000' &
FAKE_PID=$!
echo ${FAKE_PID} >> /run/lock/devicecl.lock

# TEST: Start and stop Device Client
# 1. Rename aws-iot-device-client to sleep and exec binary
# 2. Kill dummy process
# 3. Kill Device Client. If Device Client has already exited as expected, then pkill will return 1, and we pass the test
cp aws-iot-device-client sleep
./sleep &
DC_PID=$!

# give some buffer time for the background instance of DC to run its logic.
sleep 5
kill $FAKE_PID
kill $DC_PID
retVal=$?
if [ $retVal -ne 1 ]; then
  echo 'TEST FAILURE: Device Client ran with a valid lockfile already in place.'
  exit 1
fi

# Cleanup
rm -rf /run/lock

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

# tail SDK log file
tail -f ${SDK_LOG_FILE} 2>&1 &

# Run integration tests
./aws-iot-device-client-integration-tests $@
