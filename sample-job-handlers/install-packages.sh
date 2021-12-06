#!/usr/bin/env sh
set -e

echo "Running install-packages.sh"
user=$1
shift 1
packages=$@
echo "Username: $user"
echo "Packages to install: $packages"
if command -v "apt-get" > /dev/null
then
    echo "Using apt-get for package install"
    # Squash any issues related to debconf frontend
    export DEBIAN_FRONTEND=noninteractive
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
        sudo -u "$user" -n apt-get install -y $packages
    else
        echo "username or sudo command not found"
        apt-get install -y $packages
    fi
elif command -v "yum" > /dev/null
then
  echo "Using yum for package install"
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n yum install -y $packages
    else
      echo "username or sudo command not found"
      yum install -y $packages
    fi
else
  >&2 echo "No suitable package manager found"
  exit 1
fi