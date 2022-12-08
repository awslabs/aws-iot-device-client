# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

HOME_DIR=/root
LICENSE_TEXT_FILE_NAME="LINUX_PACKAGES_LICENSES"
LICENSE_TEXT_FILE_PATH=${HOME_DIR}/${LICENSE_TEXT_FILE_NAME}
PACKAGE_LIST_TEXT_FILE_NAME="LINUX_PACKAGES_LIST"

OUTPUT=$(dpkg -l | grep '^.[iufhwt]')
echo "${OUTPUT}" > ${HOME_DIR}/${PACKAGE_LIST_TEXT_FILE_NAME}

IFS=$'\n' read -rd '' -a OUTPUT_LIST <<<${OUTPUT}

for (( i=0; i<${#OUTPUT_LIST[@]}; i++ ))
do
    IFS=$' ' read -rd '' -a PACKAGE_DETAILS <<<${OUTPUT_LIST[$i]}
    if [ ${#PACKAGE_DETAILS[@]} ]; then
        IFS=$':' read -a PACKAGE_NAME_AND_ARCH <<<${PACKAGE_DETAILS[1]}
        PACKAGE_NAME="${PACKAGE_NAME_AND_ARCH[0]}"
        LICENSE_TEXT=$(cat "/usr/share/doc/${PACKAGE_NAME}/copyright")
        if [ -z "${LICENSE_TEXT}" ]; then
            LICENSE_TEXT="License is not present for this package."
        fi
        echo "Package Name: "${PACKAGE_NAME} >> ${LICENSE_TEXT_FILE_PATH}
        echo "Package Version: "${PACKAGE_DETAILS[2]} >> ${LICENSE_TEXT_FILE_PATH}
        echo "Package License Location: "${PACKAGE_LICENSE_LOCATION} >> ${LICENSE_TEXT_FILE_PATH}
        echo -e "Package License Text: "${LICENSE_TEXT}"\n" >> ${LICENSE_TEXT_FILE_PATH}
    fi
done