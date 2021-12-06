#!/usr/bin/env sh
set -e

echo "Running shutdown.sh"
user=$1
echo "Username: $user"

if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
  sudo -u "$user" -n shutdown -h +1 > /dev/null
else
  echo "username or sudo command not found"
  shutdown -h +1 > /dev/null
fi
echo "System scheduled for shut down in one minute..."