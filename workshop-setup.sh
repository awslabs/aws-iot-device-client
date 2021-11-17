#!/bin/sh
set -e

# Prompt color constants
PMPT='\033[95;1m%s\033[0m\n'
GREEN='\033[92m%s\033[0m\n'
RED='\033[91m%s\033[0m\n'

if [ $(id -u) = 0 ]; then
   printf ${RED} "WARNING: Only run this setup script as root if you plan to run the AWS IoT Device Client as root,\
  or if you plan to run the AWS IoT Device Client as a service. Otherwise, you should run this script as\
  the user that will execute the client."
fi

### Build Configuration File ###
printf ${PMPT} "Do you want to interactively generate a configuration file for the AWS IoT Device Client? y/n"
read -r BUILD_CONFIG

if [ $(id -u) = 0 ]; then
  OUTPUT_DIR=/root/.aws-iot-device-client/
else
  OUTPUT_DIR=/home/$(whoami)/.aws-iot-device-client/
fi

### Config Defaults ###
CONF_OUTPUT_PATH=${OUTPUT_DIR}aws-iot-device-client.conf

###log defaults###
LOG_TYPE="FILE"
LOG_LEVEL="DEBUG"
LOG_LOCATION="/var/log/aws-iot-device-client/aws-iot-device-client.log"

###Jobs defaults###
JOBS_ENABLED="true"
HANDLER_DIR=${OUTPUT_DIR}jobs
mkdir -p ${HANDLER_DIR}
chmod 700 ${HANDLER_DIR}
cp ./sample-job-handlers/* ${HANDLER_DIR}
chmod 700 ${HANDLER_DIR}/*

###other feature defaults###
ST_ENABLED="true"
DD_ENABLED="true"
DD_INTERVAL=300
FP_ENABLED="false"
PUBSUB_ENABLED="true"
CONFIG_SHADOW_ENABLED="false"
SAMPLE_SHADOW_ENABLED="false"

if [ "$BUILD_CONFIG" = "y" ]; then
  while [ "$CONFIGURED" != 1 ]; do
    printf ${PMPT} "Specify AWS IoT endpoint to use:"
    read -r ENDPOINT
    printf ${PMPT} "Specify path to you device PEM certificate (file name ending in *-certificate.pem.crt):"
    read -r CERT
    printf ${PMPT} "Specify path to private key (file name ending in *-private.pem.key):"
    read -r PRIVATE_KEY
    printf ${PMPT} "Specify path to ROOT CA certificate (file name ending in *.pem):"
    read -r ROOT_CA
    printf ${PMPT} "Specify thing name (Also used as Client ID):"
    read -r THING_NAME


    ### PUBSUB Config ###
    printf ${GREEN} "We will now configure the MQTT PUB-SUB feature - this feature lets your device communicate with AWS IoT Core over MQTT"
    printf ${PMPT} "Specify a topic for the feature to publish to:"
    read -r PUB_TOPIC
    printf ${PMPT} "Specify the path of a file for the feature to publish (Leaving this blank will publish 'Hello World!'):"
    read -r PUB_FILE
    printf ${PMPT} "Specify a topic for the feature to subscribe to:"
    read -r SUB_TOPIC
    printf ${PMPT} "Specify the path of a file for the feature to write to (Optional):"
    read -r SUB_FILE
    
    DISP_CONFIG_OUTPUT="
    {
      \"endpoint\":	\"$ENDPOINT\",
      \"cert\":	\"$CERT\",
      \"key\":	\"$PRIVATE_KEY\",
      \"root-ca\":	\"$ROOT_CA\",
      \"thing-name\":	\"$THING_NAME\",
     
     \"MQTT Settings\":	{ 
        \"publish-topic\": \"$PUB_TOPIC\",
        \"publish-file\": \"$PUB_FILE\",
        \"subscribe-topic\": \"$SUB_TOPIC\",
        \"subscribe-file\": \"$SUB_FILE\"
      }  
     
      \"Further Information\":	{
        \"Log file location\": \"$LOG_LOCATION\"
        \"Jobs enabled\":	$JOBS_ENABLED,
        \"Secure Tunneling enabled\":	$ST_ENABLED
        \"Device Defender enabled\":	$DD_ENABLED,
        \"MQTT Pub-sub enabled\": $PUBSUB_ENABLED,
      }
      
    }"
    
    CONFIG_OUTPUT="
    {
      \"endpoint\":	\"$ENDPOINT\",
      \"cert\":	\"$CERT\",
      \"key\":	\"$PRIVATE_KEY\",
      \"root-ca\":	\"$ROOT_CA\",
      \"thing-name\":	\"$THING_NAME\",
      \"logging\":	{
        \"level\":	\"$LOG_LEVEL\",
        \"type\":	\"$LOG_TYPE\",
        \"file\": \"$LOG_LOCATION\"
      },
      \"jobs\":	{
        \"enabled\":	$JOBS_ENABLED,
        \"handler-directory\": \"$HANDLER_DIR\"
      },
      \"tunneling\":	{
        \"enabled\":	$ST_ENABLED
      },
      \"device-defender\":	{
        \"enabled\":	$DD_ENABLED,
        \"interval\": $DD_INTERVAL
      },
      \"fleet-provisioning\":	{
        \"enabled\":	$FP_ENABLED,
        \"template-name\": \"$FP_TEMPLATE_NAME\",
        \"template-parameters\": \"$FP_TEMPLATE_PARAMS\",
        \"csr-file\": \"$FP_CSR_FILE\",
        \"device-key\": \"$FP_DEVICE_KEY\"
      },
      \"samples\": {
        \"pub-sub\": {
          \"enabled\": $PUBSUB_ENABLED,
          \"publish-topic\": \"$PUB_TOPIC\",
          \"publish-file\": \"$PUB_FILE\",
          \"subscribe-topic\": \"$SUB_TOPIC\",
          \"subscribe-file\": \"$SUB_FILE\"
        }
      },
      \"config-shadow\":	{
        \"enabled\":	$CONFIG_SHADOW_ENABLED
      },
      \"sample-shadow\": {
        \"enabled\": $SAMPLE_SHADOW_ENABLED,
        \"shadow-name\": \"$SAMPLE_SHADOW_NAME\",
        \"shadow-input-file\": \"$SAMPLE_SHADOW_INPUT_FILE\",
        \"shadow-output-file\": \"$SAMPLE_SHADOW_OUTPUT_FILE\"
      }
    }"

    printf ${PMPT} "Please read the configuration carefully. We have also provided additional information regarding defaults"
    printf ${GREEN} "${DISP_CONFIG_OUTPUT}"
    printf ${PMPT} "Does the above configuration appear correct? If yes, configuration will be written to ${CONF_OUTPUT_PATH}: y/n"
    read -r GOOD_TO_GO
    if [ "$GOOD_TO_GO" = "y" ]; then
      CONFIGURED=1
      mkdir -p "$OUTPUT_DIR" &>/dev/null | true
      echo "$CONFIG_OUTPUT" | tee "$CONF_OUTPUT_PATH" >/dev/null
      chmod 745 "$OUTPUT_DIR"
      chmod 644 "$CONF_OUTPUT_PATH"
      printf ${GREEN} "Configuration has been successfully written to ${CONF_OUTPUT_PATH}"
    fi
    tput sgr0
  done
fi

### printf ${PMPT} "Copying sample job handlers to the default handler directory (${HANDLER_DIR})"

printf ${PMPT} "Do you want to install AWS IoT Device Client as a service? y/n"
read -r INSTALL_SERVICE

if [ "$INSTALL_SERVICE" = "y" ]; then
  if ! [ $(id -u) = 0 ]; then
    printf ${RED} "WARNING: You may need to rerun this setup script as root ('sudo ./workshop-setup.sh') \
    to successfully install the AWS IoT Device Client as a service"
  fi
  ### Get DeviceClient Artifact Location ###
  FOUND_DEVICE_CLIENT=false
  DEVICE_CLIENT_ARTIFACT_DEFAULT="./build/aws-iot-device-client"
  DEVICE_CLIENT_ARTIFACT="$DEVICE_CLIENT_ARTIFACT_DEFAULT"  
  while [ "$FOUND_DEVICE_CLIENT" != true ]; do
    if [ ! -f "$DEVICE_CLIENT_ARTIFACT" ]; then
      printf ${RED} "File: $DEVICE_CLIENT_ARTIFACT does not exist."
      printf ${PMPT} "Enter the complete directory path for the aws-iot-device-client. (Empty for default: ${DEVICE_CLIENT_ARTIFACT_DEFAULT})"
      read -r DEVICE_CLIENT_ARTIFACT
      if [ -z "$DEVICE_CLIENT_ARTIFACT" ]; then
        DEVICE_CLIENT_ARTIFACT="$DEVICE_CLIENT_ARTIFACT_DEFAULT"
      fi
    else
      FOUND_DEVICE_CLIENT=true
    fi    
  done

  ### Get DeviceClient Service File Location ###
  FOUND_SERVICE_FILE=false
  SERVICE_FILE_DEFAULT="./setup/aws-iot-device-client.service"
  SERVICE_FILE="$SERVICE_FILE_DEFAULT"
  while [ "$FOUND_SERVICE_FILE" != true ]; do
    if [ ! -f "$SERVICE_FILE" ]; then
      printf ${RED} "File: $SERVICE_FILE does not exist."
      printf ${PMPT} "Enter the complete directory path for the aws-iot-device-client service file. (Empty for default: ${SERVICE_FILE_DEFAULT})"
      read -r SERVICE_FILE
      if [ -z "$SERVICE_FILE" ]; then
        SERVICE_FILE="$SERVICE_FILE_DEFAULT"
      fi
    else
      FOUND_SERVICE_FILE=true
    fi 
  done

  BINARY_DESTINATION="/sbin/aws-iot-device-client"
  
  printf ${PMPT} "Installing AWS IoT Device Client..."
  if command -v "systemctl" &>/dev/null; then
    systemctl stop aws-iot-device-client.service || true
    cp "$SERVICE_FILE" /etc/systemd/system/aws-iot-device-client.service
    if [ "$SERVICE_DEBUG" = "y" ]; then
      echo "$DEBUG_SCRIPT" | tee /sbin/aws-iot-device-client >/dev/null
    else
      # In case we previously ran in debug, make sure to delete the old binary
      rm -f /sbin/aws-iot-device-client-bin
    fi
    cp "$DEVICE_CLIENT_ARTIFACT" "$BINARY_DESTINATION"
    chmod 700 "$BINARY_DESTINATION"
    systemctl enable aws-iot-device-client.service
    systemctl start aws-iot-device-client.service
    systemctl status aws-iot-device-client.service
  elif command -v "service" &>/dev/null; then
    service stop aws-iot-device-client.service || true
    cp "$SERVICE_FILE" /etc/systemd/system/aws-iot-device-client.service
    if [ "$SERVICE_DEBUG" = "y" ]; then
      echo "$DEBUG_SCRIPT" | tee /sbin/aws-iot-device-client >/dev/null
    else
      # In case we previously ran in debug, make sure to delete the old binary
      rm -f /sbin/aws-iot-device-client-bin
    fi
    cp "$DEVICE_CLIENT_ARTIFACT" "$BINARY_DESTINATION"
    chmod 700 "$BINARY_DESTINATION"
    service enable aws-iot-device-client.service
    service start aws-iot-device-client.service
    service status aws-iot-device-client.service
  fi
  printf ${PMPT} "AWS IoT Device Client is now running! Check /var/log/aws-iot-device-client/aws-iot-device-client.log for log output."
fi
