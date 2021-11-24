#!/bin/sh


if command -v "rpm" > /dev/null; then
  rpm -q $1
elif command -v "dpkg" > /dev/null; then
  # Using -s flag instead of -l for dpkg based on https://github.com/bitrise-io/bitrise/issues/433
  dpkg -s $1
else
  >&2 echo "No suitable package manager found"
  exit 1
fi;