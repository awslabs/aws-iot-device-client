# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

LICENSE_FILE_NAMES=("LICENSE" "LICENSE.txt" "LICENSE.md" "license.txt" "license" "COPYRIGHT" "LICENSE.rst" "COPYING" "COPYING.md" "COPYING.txt")

LICENSE_FILE_LOCATIONS=("/usr/share/licenses" "/usr/share/doc")

HOME_DIR=/root
LICENSE_TEXT_FILE_NAME="LINUX_PACKAGES_LICENSES"
LICENSE_TEXT_FILE_PATH=${HOME_DIR}/${LICENSE_TEXT_FILE_NAME}
PACKAGE_LIST_TEXT_FILE_NAME="LINUX_PACKAGES_LIST"

OUTPUT="$(yum list installed | grep -v @amzn2-core | sort)"
echo "${OUTPUT}" > ${HOME_DIR}/${PACKAGE_LIST_TEXT_FILE_NAME}

IFS=$'\n' read -rd '' -a OUTPUT_LIST <<<"${OUTPUT}"

for (( i=0; i<${#OUTPUT_LIST[@]}; i++ ))
do
    IFS=$' ' read -rd '' -a PACKAGE_DETAILS <<<${OUTPUT_LIST[$i]}
    if [ ${#PACKAGE_DETAILS[@]} -eq "3" ]; then
        IFS=$'.' read -rd '' -a PACKAGE_NAME_AND_ARCH <<<${PACKAGE_DETAILS[0]}
        PACKAGE_NAME=${PACKAGE_NAME_AND_ARCH[0]}
        IFS=$'-:' read -rd '' -a PACKAGE_VERSION_ARR <<<${PACKAGE_DETAILS[1]}
        if [ ${#PACKAGE_VERSION[@]} -ge "2" ]; then
            PACKAGE_VERSION="${PACKAGE_VERSION_ARR[1]}"
        else
            PACKAGE_VERSION="${PACKAGE_VERSION_ARR[0]}"
        fi
        PACKAGE_LOCATION=""
        for (( license_file_dir=0; license_file_dir<"${#LICENSE_FILE_LOCATIONS[@]}"; license_file_dir++ ))
        do
            for (( license_file=0; license_file<"${#LICENSE_FILE_NAMES[@]}"; license_file++ ))
            do
                if [[ -f "${LICENSE_FILE_LOCATIONS[$license_file_dir]}/${PACKAGE_NAME}-${PACKAGE_VERSION}/${LICENSE_FILE_NAMES[$license_file]}" ]]; then
                    PACKAGE_LICENSE_LOCATION=${LICENSE_FILE_LOCATIONS[$license_file_dir]}/${PACKAGE_NAME}-${PACKAGE_VERSION}/${LICENSE_FILE_NAMES[$license_file]}
                    break
                elif [[ -f "${LICENSE_FILE_LOCATIONS[$license_file_dir]}/${PACKAGE_NAME}/${LICENSE_FILE_NAMES[$license_file]}" ]]; then
                    PACKAGE_LICENSE_LOCATION=${LICENSE_FILE_LOCATIONS[$license_file_dir]}/${PACKAGE_NAME}/${LICENSE_FILE_NAMES[$license_file]}
                    break
                fi
            done
        done
        if [ ${PACKAGE_LICENSE_LOCATION} ] && [ -f ${PACKAGE_LICENSE_LOCATION} ]; then
            LICENSE_TEXT=$(cat "${PACKAGE_LICENSE_LOCATION}") || true
        else
            LICENSE_TEXT="License is not present for this package."
        fi
        echo "Package Name: "${PACKAGE_NAME} >> ${LICENSE_TEXT_FILE_PATH}
        echo "Package Version: "${PACKAGE_VERSION} >> ${LICENSE_TEXT_FILE_PATH}
        echo "Package License Location: "${PACKAGE_LICENSE_LOCATION} >> ${LICENSE_TEXT_FILE_PATH}
        echo -e "Package License Text: "${LICENSE_TEXT}"\n" >> ${LICENSE_TEXT_FILE_PATH}
    fi
done