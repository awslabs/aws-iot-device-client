#!/bin/sh
if command -v "apt-get" > /dev/null
then
    if command -v "sudo" > /dev/null; then
      sudo -n apt-get remove -y --purge "$@"
    else
      echo "WARNING: sudo command not found"
      apt-get remove -y --purge "$@"
    fi
elif command -v "yum" > /dev/null; then
  if command -v "sudo" > /dev/null; then
      sudo -n yum remove -y "$@"
    else
      echo "WARNING: sudo command not found"
      yum remove -y "$@"
    fi
else
  >&2 echo "No suitable package manager found"
  exit 1
fi