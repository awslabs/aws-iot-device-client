cmake_minimum_required(VERSION 2.6)

set(GTEST_PROJECT test-aws-iot-device-client)

enable_testing()

message("Gtest Cmake")

find_package(GTest REQUIRED)

file(GLOB DC_SRC "../source/*.cpp" /
        "../source/*.c" /
        "../source/**/*.cpp" /
        "../source/**/*.c")

file(GLOB DC_TST "./*.cpp" /
        "./**/*.cpp")
list(APPEND DC_SRC ${DC_TST})
list(FILTER DC_SRC EXCLUDE REGEX ".*main.cpp$")

set(HEADERS)

add_executable(${GTEST_PROJECT} ${DC_SRC})
target_link_libraries(${GTEST_PROJECT} PUBLIC
        gtest
        gtest_main
        )
target_link_libraries(${GTEST_PROJECT} IotJobs-cpp IotDeviceCommon-cpp IotDeviceDefender-cpp IotSecureTunneling-cpp aws-crt-cpp aws-c-mqtt aws-c-auth aws-c-io s2n aws-c-cal aws-c-http aws-c-compression aws-c-common aws-c-iot)
target_link_libraries(${GTEST_PROJECT} OpenSSL::SSL)
target_link_libraries(${GTEST_PROJECT} OpenSSL::Crypto)

add_test(${GTEST_PROJECT} ${GTEST_PROJECT})
