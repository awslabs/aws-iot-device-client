#!/usr/bin/env sh
set -e

echo "Running reboot.sh"
user=$1
echo "Username: $user"

GLOBAL_LAST_BOOT=""
REBOOT_LOCK_FILE=/var/tmp/dc-reboot.lock

get_last_boot_time() {
  # Calculate exact time of last boot
  UPTIME=$(cat /proc/uptime | head -n1 | awk '{print $1;}')
  CURRENT=$(date "+%s")
  GLOBAL_LAST_BOOT=$(awk -v currentTime=$CURRENT -v uptime=$UPTIME 'BEGIN {printf "%.0f",currentTime-uptime}')
}

if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
  if sudo -u "$user" -n test -f "$REBOOT_LOCK_FILE"; then
    echo "Found lock file"
    LOCK_FILE_BOOT=$(cat $REBOOT_LOCK_FILE)
    rm $REBOOT_LOCK_FILE
    get_last_boot_time
    if [ "$LOCK_FILE_BOOT" -lt "$GLOBAL_LAST_BOOT" ]; then
      echo "Reboot was successful"
      exit 0
    else
      echo "Reboot failed"
      exit 1
    fi
  else
    echo "Did not find $REBOOT_LOCK_FILE"
    date "+%s" > $REBOOT_LOCK_FILE
    sudo -u "$user" -n reboot
  fi
else
  echo "username or sudo command not found"
  if test -f "$REBOOT_LOCK_FILE"; then
    echo "Found lock file"
    LOCK_FILE_BOOT=$(cat $REBOOT_LOCK_FILE)
    rm $REBOOT_LOCK_FILE
    get_last_boot_time
    if [ "$LOCK_FILE_BOOT" -lt "$GLOBAL_LAST_BOOT" ]; then
      echo "Reboot was successful"
      exit 0
    else
      echo "Reboot failed"
      exit 1
    fi
  else
    echo "Did not find $REBOOT_LOCK_FILE"
    date "+%s" > $REBOOT_LOCK_FILE
    reboot || true
    rm $REBOOT_LOCK_FILE
    exit 1
  fi
fi