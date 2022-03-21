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
      if sudo -u "$user" -n dpkg -s "$package"; then
        continue
      else
        exit 1
      fi
    fi
    echo "No suitable package manager found"
    exit 1
  done
else
  echo "username or sudo command not found"
  for package in $packages
  do
    if command -v "rpm" >/dev/null; then
      if command rpm -q "$package" >/dev/null; then
        continue
      fi
    fi
    if command -v "dpkg" >/dev/null; then
      if command dpkg -s "$package" >/dev/null; then
        continue
      else
        exit 1
      fi
    fi
    echo "No suitable package manager found"
    exit 1
  done
fi;
