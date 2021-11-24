#!/usr/bin/env sh

echo "Running verify-packages-removed.sh"
user=$1
shift 1
packages=$@
echo "Username: $user"
echo "Packages to verify: $packages"

if command -v "rpm" > /dev/null; then
  for pkg in $packages
  do
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n rpm -q "$pkg"
    else
      echo "username or sudo command not found"
      rpm -q "$pkg"
    fi
    RETVAL=$?
    if [ $RETVAL -eq 0 ]; then
        exit 1
    fi
  done
elif command -v "dpkg" > /dev/null; then
  for pkg in $packages
  do
    # Using -s flag instead of -l for dpkg based on https://github.com/bitrise-io/bitrise/issues/433
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      sudo -u "$user" -n dpkg -s "$pkg"
    else
      echo "username or sudo command not found"
      dpkg -s "$pkg"
    fi
    RETVAL=$?
    if [ $RETVAL -eq 0 ]; then
        exit 1
    fi
  done
else
  >&2 echo "No suitable method found to determine installed packages"
  exit 1
fi

