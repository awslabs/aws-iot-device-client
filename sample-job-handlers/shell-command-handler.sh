#!/usr/bin/env sh
set -e

echo "Running shell-command-handler.sh"
user=$1
shift 1
echo "Username: $user"
if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
    sudo -u "$user" -n $@
else
    echo "username or sudo command not found"
    $@
fi