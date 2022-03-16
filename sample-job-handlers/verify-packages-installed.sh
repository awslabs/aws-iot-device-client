#!/usr/bin/env sh
set -e

echo "Running verify-packages-installed.sh"
user=$1
shift 1
packages=$@
echo "Username: $user"
echo "Packages to verify: $packages"

if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
  for package in $packages
  do
    if command -v "rpm" >/dev/null; then
      if command sudo -u "$user" -n rpm -q "$package" > /dev/null; then
        continue
      fi
    fi
    if command -v "dpkg" >/dev/null; then
      sudo -u "$user" -n dpkg -s "$package"
    fi
  done
else
  echo "username or sudo command not found"
  for package in $packages
  do
    if command -v "rpm" >/dev/null; then
      if command rpm -q "$package" > /dev/null; then
        continue
      fi
    fi
    if command -v "dpkg" >/dev/null; then
      dpkg -s "$package"
    fi
  done
fi;
