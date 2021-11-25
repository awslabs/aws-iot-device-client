#!/bin/sh
set -e

echo "Running install-packages.sh"
if command -v "apt-get" > /dev/null
then
    echo "Using apt-get for package install"
    # Squash any issues related to debconf frontend
    export DEBIAN_FRONTEND=noninteractive
    if command -v "sudo" > /dev/null; then
      sudo -n apt-get install -y "$@"
    else
      echo "WARNING: sudo command not found"
      apt-get install -y "$@"
    fi
elif command -v "yum" > /dev/null
then
  echo "Using yum for package install"
    if command -v "sudo" > /dev/null; then
      sudo -n yum install -y "$@"
    else
      echo "WARNING: sudo command not found"
      yum install -y "$@"
    fi
else
  >&2 echo "No suitable package manager found"
  exit 1
fi