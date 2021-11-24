#!/bin/sh
if command -v "systemctl" > /dev/null;
then
  for service in "$@"
  do
    sudo -n systemctl restart "$service"
  done
elif command -v "service" > /dev/null;
then
  for service in "$@"
  do
    sudo -n service restart "$service"
  done
else
  >&2 echo "No suitable package manager found"
  exit 1
fi