cmake_minimum_required(VERSION 3.13)
project(aws-iot-device-client-integration-tests)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)

#########################################
# AWS C++ SDK dependency                #
#########################################

## Download and unpack aws cpp sdk at configure time
configure_file(CMakeLists.txt.libawscpp
        aws-sdk-cpp/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/aws-sdk-cpp)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/aws-sdk-cpp)
#
## Add aws c++ sdk directly to the build
add_subdirectory(${CMAKE_BINARY_DIR}/aws-sdk-cpp)

set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/aws-sdk-cpp")
find_package(AWSSDK REQUIRED COMPONENTS iot iotsecuretunneling)

#########################################
# GoogleTest dependency                #
#########################################

configure_file(CMakeLists.txt.gtest
        googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This adds
# the following targets: gtest, gtest_main, gmock
# and gmock_main
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build)
include_directories(${CMAKE_BINARY_DIR}/googletest-src/googletest/include/)
add_executable(aws-iot-device-client-integration-tests source/GTestMain.cpp source/fleetprovisioning/FleetProvisioningIntegrationTests.cpp source/IntegrationTestResourceHandler.cpp source/jobs/JobsIntegrationTests.cpp source/devicedefender/DeviceDefenderIntegrationTests.cpp source/tunneling/SecureTunnelingIntegrationTests.cpp)
target_link_libraries(aws-iot-device-client-integration-tests gtest ${AWSSDK_LINK_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)