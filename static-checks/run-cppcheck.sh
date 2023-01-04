#!/usr/bin/env bash
#
# Run cppcheck on source files, excluding test subdirectory and build dependencies.
#
# Options
# --enable=all
#       Enable all checks, exclude explicit suppressions below, use --doc to obtain list of all checks.
#
# --suppress=toomanyconfigs:source/main.cpp
#       Cppcheck emits an error if a source file contains more than 12 #ifdefs.
#
# --suppress=missingInclude:*
#       Suppress errors about missing includes, cppcheck is not good at locating system headers.
#       Since include paths depend on the target system, we choose to suppress rather than providing explicit include paths to cppcheck.
#
# --inline-suppr
#       Enable inline suppressions of specific lines in source code.
#
# --error-exitcode=1
#       Return 1 when any error is detected. This will cause build to fail.
#
cppcheck \
    --enable=all \
    --suppress=toomanyconfigs:source/main.cpp \
    --suppress=missingInclude:* \
    --inline-suppr \
    --error-exitcode=1 \
    -i test \
    source integration-tests/source
