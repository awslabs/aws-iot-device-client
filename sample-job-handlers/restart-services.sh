#!/usr/bin/env sh
set -e

echo "Running restart-services.sh"
user=$1
shift 1
services=$@
echo "Username: $user"
echo "Services to restart: $services"

RESTART_LOCK_FILE=/var/tmp/dc-restart.lock
LOCK_FILE_PID="0"
PID=$(pidof aws-iot-device-client)
  # If this script is used to restart Device Client then the Job will not be updated to completed. On restart, the Device
  # Client will begin to execute the Job again. Therefore, to avoid an infinite loop here we need to use a lock file to
  # check if the Device Client had already been restarted.
for service in $services
do
  if [ "$service" = "aws-iot-device-client" ]; then
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      if sudo -u "$user" -n test -f "$RESTART_LOCK_FILE"; then
        LOCK_FILE_PID=$(cat $RESTART_LOCK_FILE)
      fi
    else
      if test -f "$RESTART_LOCK_FILE"; then
        LOCK_FILE_PID=$(cat $RESTART_LOCK_FILE)
      fi
    fi
    if [ "$LOCK_FILE_PID" != "0" ]; then
      rm $RESTART_LOCK_FILE
      if [ "$LOCK_FILE_PID" = "$PID" ]; then
        echo "Failed to restart aws-iot-device-client"
        exit 1;
      else
        echo "Successfully restarted aws-iot-device-client"
        continue;
      fi
    else
      echo $PID > $RESTART_LOCK_FILE
    fi
  fi

  if command -v "systemctl" > /dev/null; then
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n systemctl restart "$service"
    else
      echo "username or sudo command not found"
      systemctl restart "$service"
    fi
  elif command -v "service" > /dev/null; then
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n service "$service" restart
    else
      echo "username or sudo command not found"
      service "$service" restart
    fi
  else
    >&2 echo "No suitable package manager found"
    exit 1
  fi
done
