#!/bin/sh
set -e
sudo -n shutdown -h +1 > /dev/null
echo "System scheduled for shut down in one minute..."