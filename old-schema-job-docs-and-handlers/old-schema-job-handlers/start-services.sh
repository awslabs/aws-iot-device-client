#!/bin/sh
if command -v "systemctl" > /dev/null;
then
  for service in "$@"
  do
    sudo -n systemctl start "$service"
  done
elif command -v "service" > /dev/null;
then
  for service in "$@"
  do
    sudo -n service start "$service"
  done
fi