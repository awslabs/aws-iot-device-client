#!/bin/sh
set -e

echo "Running download-file.sh"
if command -v "wget" > /dev/null
then
    echo "Using wget for downloading user specified file"
    if command -v "sudo" > /dev/null; then
      sudo -n wget --quiet -O "$2" "$1"
    else
      echo "WARNING: sudo command not found"
      wget --quiet -O --quiet "$2" "$1"
    fi
else
    >&2 echo "wget software not installed on the device. Use install-packages operation for installing wget software on the target device."
    exit 1
fi