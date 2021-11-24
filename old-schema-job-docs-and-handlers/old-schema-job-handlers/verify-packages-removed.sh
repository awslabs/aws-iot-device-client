#!/bin/bash

if command -v "rpm" > /dev/null; then
  for package in "$@"
  do
    rpm -q $1
    RETVAL=$?
    if [ $RETVAL -eq 0 ]; then
        exit 1
    fi
  done
elif command -v "dpkg" > /dev/null; then
  for package in "$@"
  do
    # Using -s flag instead of -l for dpkg based on https://github.com/bitrise-io/bitrise/issues/433
    dpkg -s "$package"
    RETVAL=$?
    if [ $RETVAL -eq 0 ]; then
        exit 1
    fi
  done
else
  >&2 echo "No suitable method found to determine installed packages"
  exit 1
fi

