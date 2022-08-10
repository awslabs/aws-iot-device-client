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
  OUTPUT_DIR=/etc/.aws-iot-device-client/
else
  OUTPUT_DIR=/home/$(whoami)/.aws-iot-device-client/
fi

### Config Defaults ###
CONF_OUTPUT_PATH=${OUTPUT_DIR}aws-iot-device-client.conf
HANDLER_DIR=${OUTPUT_DIR}jobs
PUBSUB_DIR=${OUTPUT_DIR}pubsub/
PUB_FILE=${PUBSUB_DIR}publish-file.txt
SUB_FILE=${PUBSUB_DIR}subscribe-file.txt
PUB_FILE_PROVIDED="n"
SUB_FILE_PROVIDED="n"
DD_INTERVAL=300
LOG_TYPE="STDOUT"
LOG_LEVEL="DEBUG"
LOG_LOCATION="/var/log/aws-iot-device-client/aws-iot-device-client.log"
SDK_LOGS_ENABLED="false"
SDK_LOG_LEVEL="TRACE"
SDK_LOG_LOCATION="/var/log/aws-iot-device-client/sdk.log"

if [ "$BUILD_CONFIG" = "y" ]; then
  while [ "$CONFIGURED" != 1 ]; do
    printf ${PMPT} "Specify AWS IoT endpoint to use:"
    read -r ENDPOINT
    printf ${PMPT} "Specify path to public PEM certificate:"
    read -r CERT
    printf ${PMPT} "Specify path to private key:"
    read -r PRIVATE_KEY
    printf ${PMPT} "Specify path to ROOT CA certificate:"
    read -r ROOT_CA
    printf ${PMPT} "Specify thing name (Also used as Client ID):"
    read -r THING_NAME

    ### Logging Config ###
    printf ${PMPT} "Would you like to configure the logger? y/n"
    CONFIGURE_LOGS=""
    read -r CONFIGURE_LOGS
    if [ "$CONFIGURE_LOGS" = "y" ]; then
      printf ${PMPT} "Specify desired log level: DEBUG/INFO/WARN/ERROR"
      read -r LOG_LEVEL
      printf ${PMPT} "Specify log type: STDOUT for standard output, FILE for file"
      read -r LOG_TYPE
      if [ "$LOG_TYPE" = "FILE" ] || [ "$LOG_TYPE" = "file" ]; then
        printf ${PMPT} "Specify path to desired log file (if no path is provided, will default to ${LOG_LOCATION}:"
        read -r LOG_LOCATION_TMP
        if [ "${LOG_LOCATION_TMP}" ]; then
          LOG_LOCATION=${LOG_LOCATION_TMP}
        else
          printf ${GREEN} "Creating default log directory..."
          if command -v "sudo" > /dev/null; then
            sudo -n mkdir -p /var/log/aws-iot-device-client/ | true
            CURRENT_USER=$(whoami)
            sudo -n chown "$CURRENT_USER":"$CURRENT_USER" /var/log/aws-iot-device-client/
          else
            printf ${RED} "WARNING: sudo command not found"
            mkdir -p /var/log/aws-iot-device-client/ | true
          fi
          chmod 745 /var/log/aws-iot-device-client/
        fi
      fi
      ### SDK Logging Config ###
      printf ${PMPT} "Would you like to configure the SDK logging? y/n"
      read -r CONFIGURE_SDK_LOGS
      if [ "$CONFIGURE_SDK_LOGS" = "y" ]; then
        SDK_LOGS_ENABLED="true"
        printf ${PMPT} "Specify desired SDK log level: TRACE/DEBUG/INFO/WARN/ERROR/FATAL"
        read -r SDK_LOG_LEVEL
        printf ${PMPT} "Specify path to desired SDK log file (if no path is provided, will default to ${SDK_LOG_LOCATION}:"
        read -r SDK_LOG_LOCATION_TMP
        if [ "${SDK_LOG_LOCATION_TMP}" ]; then
          SDK_LOG_LOCATION=${SDK_LOG_LOCATION_TMP}
        else
          printf ${GREEN} "Creating default SDK log directory..."
          if command -v "sudo" > /dev/null; then
            sudo -n mkdir -p /var/log/aws-iot-device-client/ | true
            CURRENT_USER=$(whoami)
            sudo -n chown "$CURRENT_USER":"$CURRENT_USER" /var/log/aws-iot-device-client/
          else
            printf ${RED} "WARNING: sudo command not found"
            mkdir -p /var/log/aws-iot-device-client/ | true
          fi
          chmod 745 /var/log/aws-iot-device-client/
        fi
      fi
    fi


    ### Jobs Config ###
    printf ${PMPT} "Enable Jobs feature? y/n"
    JOBS_ENABLED=""
    read -r ENABLE_JOBS
    if [ "$ENABLE_JOBS" = "y" ]; then
      JOBS_ENABLED="true"
      printf ${PMPT} "Specify absolute path to Job handler directory (if no path is provided, will default to ${HANDLER_DIR}):"
      read -r HANDLER_DIR_TEMP
      if [ "$HANDLER_DIR_TEMP" ]; then
        HANDLER_DIR=$HANDLER_DIR_TEMP
      fi
    else
      JOBS_ENABLED="false"
    fi

    ### ST Config ###
    printf ${PMPT} "Enable Secure Tunneling feature? y/n"
    ST_ENABLED=""
    read -r ENABLE_ST
    if [ "$ENABLE_ST" = "y" ]; then
      ST_ENABLED="true"
    else
      ST_ENABLED="false"
    fi

    ### DD Config ###
    printf ${PMPT} "Enable Device Defender feature? y/n"
    DD_ENABLED=""
    read -r ENABLE_DD
    if [ "$ENABLE_DD" = "y" ]; then
      DD_ENABLED="true"
      printf ${PMPT} "Specify an interval for Device Defender in seconds (default is 300):"
      read -r INTERVAL_TEMP
      if [ "$INTERVAL_TEMP" ]; then
        DD_INTERVAL=$INTERVAL_TEMP
      fi
    else
      DD_ENABLED="false"
    fi

    ### FP Config ###
    printf ${PMPT} "Enable Fleet Provisioning feature? y/n"
    FP_ENABLED=""
    read -r ENABLE_FP
    if [ "$ENABLE_FP" = "y" ]; then
      FP_ENABLED="true"
      printf ${PMPT} "Specify Fleet Provisioning Template name you want to use for Provisioning your device:"
      read -r TEMPLATE_NAME_TEMP
      if [ "$TEMPLATE_NAME_TEMP" ]; then
        FP_TEMPLATE_NAME=$TEMPLATE_NAME_TEMP
      fi
      printf ${PMPT} "Specify Fleet Provisioning Template parameters you want to use for Provisioning your device:"
      read -r TEMPLATE_PARAMS_TEMP
      if [ "$TEMPLATE_PARAMS_TEMP" ]; then
        FP_TEMPLATE_PARAMS=$TEMPLATE_PARAMS_TEMP
      else
        FP_TEMPLATE_PARAMS="{}"
      fi
      printf ${PMPT} "Specify absolute path to Certificate Signing Request (CSR) file used for creating new certificate while provisioning device by keeping private key secure:"
      read -r CSR_FILE_TEMP
      if [ "$CSR_FILE_TEMP" ]; then
        FP_CSR_FILE=$CSR_FILE_TEMP
      fi
      printf ${PMPT} "Specify absolute path to Device Private Key file:"
      read -r DEVICE_KEY_TEMP
      if [ "$DEVICE_KEY_TEMP" ]; then
        FP_DEVICE_KEY=$DEVICE_KEY_TEMP
      fi
    else
      FP_ENABLED="false"
    fi

    ### PUBSUB Config ###
    printf ${PMPT} "Enable Pub Sub sample feature? y/n"
    PUBSUB_ENABLED=""
    read -r ENABLE_PUBSUB
    if [ "$ENABLE_PUBSUB" = "y" ]; then
      PUBSUB_ENABLED="true"
      printf ${PMPT} "Specify a topic for the feature to publish to:"
      read -r PUB_TOPIC
      printf ${PMPT} "Specify the path of a file for the feature to publish (if no path is provided, will default to ${PUB_FILE}):"
      read -r PUB_FILE_TMP
      if [ "$PUB_FILE_TMP" ]; then
        PUB_FILE=$PUB_FILE_TMP
        PUB_FILE_PROVIDED="y"
      fi
      printf ${PMPT} "Specify a topic for the feature to subscribe to:"
      read -r SUB_TOPIC
      printf ${PMPT} "Specify the path of a file for the feature to write to (if no path is provided, will default to ${SUB_FILE}):"
      read -r SUB_FILE_TMP
      if [ "$SUB_FILE_TMP" ]; then
        SUB_FILE=$SUB_FILE_TMP
        SUB_FILE_PROVIDED="y"
      fi
    else
      PUBSUB_ENABLED="false"
    fi

    ### ConfigShadow Config ###
    printf ${PMPT} "Enable Config Shadow feature? y/n"
    CONFIG_SHADOW_ENABLED=""
    read -r ENABLE_CONFIG_SHADOW
    if [ "$ENABLE_CONFIG_SHADOW" = "y" ]; then
      CONFIG_SHADOW_ENABLED="true"
    else
      CONFIG_SHADOW_ENABLED="false"
    fi

    ### SampleShadow Config ###
    printf ${PMPT} "Enable Sample Shadow feature? y/n"
    SAMPLE_SHADOW_ENABLED=""
    read -r ENABLE_SAMPLE_SHADOW
    if [ "$ENABLE_SAMPLE_SHADOW" = "y" ]; then
      SAMPLE_SHADOW_ENABLED="true"
      printf ${PMPT} "Specify a shadow name for the feature to create or update:"
      read -r SAMPLE_SHADOW_NAME
      printf ${PMPT} "Specify the path of a file for the feature to read from:"
      read -r SAMPLE_SHADOW_INPUT_FILE
      printf ${PMPT} "Specify a the path of a file for the feature to write shadow document to:"
      read -r SAMPLE_SHADOW_OUTPUT_FILE
    else
      SAMPLE_SHADOW_ENABLED="false"
    fi

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
        \"file\": \"$LOG_LOCATION\",
        \"enable-sdk-logging\":	$SDK_LOGS_ENABLED,
        \"sdk-log-level\":	\"$SDK_LOG_LEVEL\",
        \"sdk-log-file\": \"$SDK_LOG_LOCATION\"
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

    while [ "$CONFIRMED" != 1 ]; do
      printf ${GREEN} "${CONFIG_OUTPUT}"
      printf ${PMPT} "Does the following configuration appear correct? If yes, configuration will be written to ${CONF_OUTPUT_PATH}: y/n"
      read -r GOOD_TO_GO
      if [ "$GOOD_TO_GO" = "y" ] || [ "$GOOD_TO_GO" = "n" ]; then
        CONFIRMED=1
      fi
      if [ "$GOOD_TO_GO" = "y" ]; then
        CONFIGURED=1
        mkdir -p "$OUTPUT_DIR"
        echo "$CONFIG_OUTPUT" | tee "$CONF_OUTPUT_PATH" >/dev/null
        chmod 745 "$OUTPUT_DIR"
        chmod 640 "$CONF_OUTPUT_PATH"
        printf ${GREEN} "Configuration has been successfully written to ${CONF_OUTPUT_PATH}"
      fi
    done
    tput sgr0
  done
fi

if [ "$PUB_FILE_PROVIDED" = "n" ] || [ "$SUB_FILE_PROVIDED" = "n" ]; then
  printf ${GREEN} "Creating default pubsub directory..."
  mkdir -p ${PUBSUB_DIR}
  chmod 745 ${PUBSUB_DIR}
fi

printf ${PMPT} "Do you want to copy the sample job handlers to the specified handler directory (${HANDLER_DIR})? y/n"
read -r COPY_HANDLERS

if [ "$COPY_HANDLERS" = "y" ]; then
  mkdir -p ${HANDLER_DIR}
  chmod 700 ${HANDLER_DIR}
  cp ./sample-job-handlers/* ${HANDLER_DIR}
  chmod 700 ${HANDLER_DIR}/*
fi

printf ${PMPT} "Do you want to install AWS IoT Device Client as a service? y/n"
read -r INSTALL_SERVICE

if [ "$INSTALL_SERVICE" = "y" ]; then
  if ! [ $(id -u) = 0 ]; then
    printf ${RED} "WARNING: You may need to rerun this setup script as root ('sudo ./setup.sh') \
    to successfully install the AWS IoT Device Client as a service"
  fi
  ### Get DeviceClient Artifact Location ###
  FOUND_DEVICE_CLIENT=false
  DEVICE_CLIENT_ARTIFACT_DEFAULT="./build/aws-iot-device-client"
  while [ "$FOUND_DEVICE_CLIENT" != true ]; do
    printf ${PMPT} "Enter the complete directory path for the aws-iot-device-client. (Empty for default: ${DEVICE_CLIENT_ARTIFACT_DEFAULT})"
    read -r DEVICE_CLIENT_ARTIFACT
    if [ -z "$DEVICE_CLIENT_ARTIFACT" ]; then
      DEVICE_CLIENT_ARTIFACT="$DEVICE_CLIENT_ARTIFACT_DEFAULT"
    fi
    if [ ! -f "$DEVICE_CLIENT_ARTIFACT" ]; then
      printf ${RED} "File: $DEVICE_CLIENT_ARTIFACT does not exist."
    else
      FOUND_DEVICE_CLIENT=true
    fi
  done

  ### Get DeviceClient Service File Location ###
  FOUND_SERVICE_FILE=false
  SERVICE_FILE_DEFAULT="./setup/aws-iot-device-client.service"
  while [ "$FOUND_SERVICE_FILE" != true ]; do
    printf ${PMPT} "Enter the complete directory path for the aws-iot-device-client service file. (Empty for default: ${SERVICE_FILE_DEFAULT})"
    read -r SERVICE_FILE
    if [ -z "$SERVICE_FILE" ]; then
      SERVICE_FILE="$SERVICE_FILE_DEFAULT"
    fi
    if [ ! -f "$SERVICE_FILE" ]; then
      printf ${RED} "File: $SERVICE_FILE does not exist."
    else
      FOUND_SERVICE_FILE=true
    fi
  done

  printf ${PMPT} "Do you want to run the AWS IoT Device Client service via Valgrind for debugging? y/n"
  read -r SERVICE_DEBUG
  if [ "$SERVICE_DEBUG" = "y" ]; then
    LOG_FILE="/var/log/aws-iot-device-client/aws-iot-device-client-debug"
    printf ${GREEN} "Valgrind output can be found at $LOG_FILE-{PID}.log. {PID} corresponds
    to the current process ID of the service, and will change if the system is rebooted"
    DEBUG_SCRIPT="#!/bin/sh
                  valgrind --log-file=\"${LOG_FILE}-\$\$.log\" /sbin/aws-iot-device-client-bin"
    BINARY_DESTINATION="/sbin/aws-iot-device-client-bin"
  else
    BINARY_DESTINATION="/sbin/aws-iot-device-client"
  fi

  printf ${PMPT} "Installing AWS IoT Device Client..."
  if command -v "systemctl" &>/dev/null; then
    systemctl stop aws-iot-device-client.service || true
    sed -i "s#/etc/.aws-iot-device-client/aws-iot-device-client.conf#$CONF_OUTPUT_PATH#g" $SERVICE_FILE
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
