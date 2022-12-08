# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

#!/bin/bash

PRETTY_NAME=$(cat /etc/os-release | grep PRETTY_NAME)

HOME_DIR=$1

export HOME_DIR=${HOME_DIR}
LINUX_PACKAGES=${HOME_DIR}/oss-compliance/linux-packages
BUILD_FROM_SOURCE_PACKAGES_LICENCES=${HOME_DIR}/oss-compliance/build-from-source-packages/build-from-source-package-licenses.txt

set -e

chmod +x ${LINUX_PACKAGES}/yum-packages.sh
chmod +x ${LINUX_PACKAGES}/dkpg-packages.sh

if [[ $PRETTY_NAME == *"Ubuntu"* ]]; then
    ${LINUX_PACKAGES}/dkpg-packages.sh
fi

if [[ $PRETTY_NAME == *"Amazon Linux"* ]]; then
  ${LINUX_PACKAGES}/yum-packages.sh
fi

if [[ $PRETTY_NAME == *"Red Hat Enterprise Linux"* ]]; then
  ${LINUX_PACKAGES}/yum-packages.sh
  BUILD_FROM_SOURCE_PACKAGES_LICENCES=${HOME_DIR}/oss-compliance/build-from-source-packages/build-from-source-package-licenses-ubi8.txt
fi

cp ${BUILD_FROM_SOURCE_PACKAGES_LICENCES} ${HOME_DIR}/BUILD_FROM_SOURCE_PACKAGES_LICENCES
chmod +x ${HOME_DIR}/oss-compliance/test/test-oss-compliance.sh
bash ${HOME_DIR}/oss-compliance/test/test-oss-compliance.sh ${HOME_DIR}