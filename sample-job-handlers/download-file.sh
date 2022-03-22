#!/usr/bin/env sh
set -e

echo "Running download-file.sh"
user=$1
fileUrl=$2
outputFile=$3
echo "Username: $user"
echo "File URL: $fileUrl"
echo "Write documents to file: $outputFile"
if command -v "wget" > /dev/null
then
    echo "Using wget for downloading user specified file"
    if id "$user" 2>/dev/null && command -v "sudo" > /dev/null; then
      if [ -d "$outputFile" ]; then
        sudo -u "$user" -n wget --quiet -P "$outputFile" "$fileUrl"
      else
        sudo -u "$user" -n wget --quiet -O "$outputFile" "$fileUrl"
      fi
    else
      echo "username or sudo command not found"
      if [ -d "$outputFile" ]; then
        wget --quiet -P "$outputFile" "$fileUrl"
      else
        wget --quiet -O "$outputFile" "$fileUrl"
      fi
    fi
else
    >&2 echo "Wget software package not installed on the device. Use the \"install-packages.sh\" operation to install the wget software package on this device."
    exit 1
fi