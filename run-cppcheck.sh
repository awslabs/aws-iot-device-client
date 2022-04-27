#!/usr/bin/env bash
#
# Run cppcheck on source files, excluding test subdirectory and build dependencies.
#
# Options
# --enable=all
#       Enable all checks, exclude explicit suppressions below, use --doc to obtain list of all checks.
#
# --suppress=missingIncludeSystem
#       Suppress errors about missing includes, cppcheck is not good at locating system headers.
#
# --suppress=preprocessorErrorDirective
#       Suppress errors about #error preprocessor directives from AWS CRT, example: aws/common/math.h
#
# --suppress=unusedFunction
#       Suppress errors about unused functions.
#
# --error-exitcode=1
#       Return 1 when any error is detected. This will cause build to fail.
#       NOTE(marcoaz): Not set due to failure of wildcard suppressions to suppress all errors.
#
cppcheck --enable=all --suppress=missingIncludeSystem:* --suppress=preprocessorErrorDirective:* --suppress=unusedFunction:* --output-file=/src/build-debug/cppcheck-results.txt -i /src/test /src/source
