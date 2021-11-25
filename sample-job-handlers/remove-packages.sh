#!/usr/bin/env sh
set -e

echo "Running remove-packages.sh"
user=$1
shift 1
packages=$@
echo "Username: $user"
echo "Packages to remove: $packages"

if command -v "apt-get" > /dev/null
then
    echo "Using apt-get for removing packages"
    # Squash any issues related to debconf frontend
    export DEBIAN_FRONTEND=noninteractive
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
        sudo -u "$user" -n apt-get remove -y --purge $packages
    else
        echo "username or sudo command not found"
        apt-get remove -y --purge $packages
    fi
elif command -v "yum" > /dev/null
then
  echo "Using yum for removing packages"
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n yum remove -y $packages
    else
      echo "username or sudo command not found"
      yum remove -y $packages
    fi
else
  >&2 echo "No suitable package manager found"
  exit 1
fi