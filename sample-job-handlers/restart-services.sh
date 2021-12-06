#!/usr/bin/env sh
set -e

echo "Running restart-services.sh"
user=$1
shift 1
services=$@
echo "Username: $user"
echo "Services to restart: $services"

if command -v "systemctl" > /dev/null;
then
  for service in $services
  do
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n systemctl restart "$service"
    else
      echo "username or sudo command not found"
      systemctl restart "$service"
    fi
  done
elif command -v "service" > /dev/null;
then
  for service in $services
  do
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n service "$service" restart
    else
      echo "username or sudo command not found"
      service "$service" restart
    fi
  done
else
  >&2 echo "No suitable package manager found"
  exit 1
fi
