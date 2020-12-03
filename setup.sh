#!/bin/sh
set -e

# Prompt color constants
PMPT='\033[95;1m'
GREEN='\033[92m'
RED='\033[91m'
NC='\033[0m'

### Build Configuration File ###

echo -e "${PMPT}Do you want to interactively generate a configuration file for the AWS IoT Device Client? y/n${NC}"
read -r BUILD_CONFIG

if [ "$BUILD_CONFIG" = "y" ]; then
  while [ "$CONFIGURED" != 1 ]
    do
    echo -e "${PMPT}Specify AWS IoT endpoint to use:${NC}"
    read -r ENDPOINT
    echo -e "${PMPT}Specify absolute path to public PEM certificate:${NC}"
    read -e -r CERT
    echo -e "${PMPT}Specify absolute path to private key:${NC}"
    read -e -r PRIVATE_KEY
    echo -e "${PMPT}Specify absolute path to ROOT CA certificate:${NC}"
    read -e -r ROOT_CA

    ### Jobs Config ###
    echo -e "${PMPT}Enable Jobs feature? y/n${NC}"
    JOBS_ENABLED=""
    read -r ENABLE_JOBS
    if [ "$ENABLE_JOBS" = "y" ]; then
      JOBS_ENABLED="true"
      echo -e "${PMPT}Specify thing name:${NC}"
      read -r THING_NAME
      echo -e "${PMPT}Specify absolute path to Job handler directory:${NC}"
      read -e -r HANDLER_DIR
    else
      JOBS_ENABLED="false"
    fi

    ### ST Config ###
    echo -e "${PMPT}Enable Secure Tunneling feature? y/n${NC}"
    ST_ENABLED=""
    read -r ENABLE_ST
    if [ "$ENABLE_ST" = "y" ]; then
      ST_ENABLED="true"
    else
      ST_ENABLED="false"
    fi

    CONFIG_OUTPUT="
    {
      \"endpoint\":	\"$ENDPOINT\",
      \"cert\":	\"$CERT\",
      \"key\":	\"$PRIVATE_KEY\",
      \"root-ca\":	\"$ROOT_CA\",
      \"jobs\":	{
        \"enabled\":	\"$JOBS_ENABLED\",
        \"thing_name\":	\"$THING_NAME\",
        \"handler_directory\": \"$HANDLER_DIR\"
      },
      \"tunneling\":	{
        \"enabled\":	\"$ST_ENABLED\"
      }
    }"

    echo -e "${PMPT}Does the following configuration appear correct? y/n${NC}"
    echo -e "${GREEN}${CONFIG_OUTPUT}${NC}"
    read -r GOOD_TO_GO
    if [ "$GOOD_TO_GO" = "y" ]; then
      CONFIGURED=1
      echo "$CONFIG_OUTPUT" | sudo tee /etc/aws-iot-device-client.conf > /dev/null
    fi
    tput sgr0
  done
fi

echo -e "${PMPT}Do you want to install AWS IoT Device Client as a service? y/n${NC}"
read -r INSTALL_SERVICE

if [ "$INSTALL_SERVICE" = "y" ]; then

  ### Get DeviceClient Artifact Location ###
  FOUND_DEVICE_CLIENT=false
  DEVICE_CLIENT_ARTIFACT=""
  while [ "$FOUND_DEVICE_CLIENT" != true ]; do
    echo -e "${PMPT}Enter the complete directory path for the aws-iot-device-client.${NC}"
    read -r DEVICE_CLIENT_ARTIFACT
    if [ ! -f "$DEVICE_CLIENT_ARTIFACT" ]; then
      echo "${RED}File: $DEVICE_CLIENT_ARTIFACT does not exist.${NC}"
    else
      FOUND_DEVICE_CLIENT=true
    fi
  done

  ### Get DeviceClient Service File Location ###
  FOUND_SERVICE_FILE=false
  SERVICE_FILE=""
  while [ "$FOUND_SERVICE_FILE" != true ]; do
    echo -e "${PMPT}Enter the complete directory path for the aws-iot-device-client service file.${NC}"
    read -r SERVICE_FILE
    if [ ! -f "$SERVICE_FILE" ]; then
      echo "${RED}File: $SERVICE_FILE does not exist.${NC}"
    else
      FOUND_SERVICE_FILE=true
    fi
  done

  echo -e "${PMPT}Do you want to run the AWS IoT Device Client service via Valgrind for debugging? y/n${NC}"
  read -r SERVICE_DEBUG
  if [ "$SERVICE_DEBUG" = "y" ]; then
    LOG_FILE="/var/log/aws-iot-device-client-debug"
    echo -e "${GREEN}Valgrind output can be found at $LOG_FILE-{PID}.log. {PID} corresponds
    to the current process ID of the service, and will change if the system is rebooted${NC}"
    DEBUG_SCRIPT="#!/bin/sh
                  valgrind --log-file=\"${LOG_FILE}-\$\$.log\" /sbin/aws-iot-device-client-bin"
    BINARY_DESTINATION="/sbin/aws-iot-device-client-bin"
  else
    BINARY_DESTINATION="/sbin/aws-iot-device-client"
  fi


  echo -e "${PMPT}Installing AWS IoT Device Client...${NC}"
  if command -v "systemctl" &> /dev/null;
  then
      sudo -n systemctl stop aws-iot-device-client.service
      sudo -n cp "$SERVICE_FILE" /etc/systemd/system/aws-iot-device-client.service
      if [ "$SERVICE_DEBUG" = "y" ]; then
        echo "$DEBUG_SCRIPT" | sudo tee /sbin/aws-iot-device-client > /dev/null
      else
        # In case we previously ran in debug, make sure to delete the old binary
        sudo -n rm -f /sbin/aws-iot-device-client-bin
      fi
      sudo -n cp "$DEVICE_CLIENT_ARTIFACT" "$BINARY_DESTINATION"
      sudo -n systemctl enable aws-iot-device-client.service
      sudo -n systemctl start aws-iot-device-client.service
      sudo -n systemctl status aws-iot-device-client.service
  elif command -v "service" &> /dev/null;
  then
      sudo -n service stop aws-iot-device-client.service
      sudo -n cp "$SERVICE_FILE" /etc/systemd/system/aws-iot-device-client.service
      if [ "$SERVICE_DEBUG" = "y" ]; then
        echo "$DEBUG_SCRIPT" | sudo tee /sbin/aws-iot-device-client > /dev/null
      else
        # In case we previously ran in debug, make sure to delete the old binary
        sudo -n rm -f /sbin/aws-iot-device-client-bin
      fi
      sudo -n cp "$DEVICE_CLIENT_ARTIFACT" "$BINARY_DESTINATION"
      sudo -n service enable aws-iot-device-client.service
      sudo -n service start aws-iot-device-client.service
      sudo -n service status aws-iot-device-client.service
  fi
  echo -e "${PMPT}AWS IoT Device Client is now running! Check /var/log/aws-iot-device-client.log for log output.${NC}"
fi
