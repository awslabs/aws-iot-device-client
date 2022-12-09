# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

#!/bin/bash

HOME_DIR=$1

FILES=("${HOME_DIR}/LINUX_PACKAGES_LICENSES" "${HOME_DIR}/BUILD_FROM_SOURCE_PACKAGES_LICENCES")

for FILE in ${FILES[@]}; do
    if [ -f "$FILE" ]; then
        echo "$FILE exists."
    else
        echo "$FILE doesn't exist which is needed for license attribution compliance."
        exit 1
    fi
done
