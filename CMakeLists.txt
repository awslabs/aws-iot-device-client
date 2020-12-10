cmake_minimum_required(VERSION 3.10)
project(aws-iot-device-client)

option(BUILD_SDK "Builds the aws-iot-device-sdk-cpp-v2 as part of build. Only do this if the SDK is not installed." ON)
option(BUILD_TEST_DEPS "Builds the device client test dependencies as part of build. Only do this if you do not have gtest installed and you wish to run the tests." ON)

list(APPEND CMAKE_MODULE_PATH "./sdk-cpp-workspace/lib/cmake")

file(GLOB DC_SRC "source/*.cpp" /
        "source/*.c" /
        "source/**/*.cpp" /
        "source/**/*.c")


set(DC_PROJECT_NAME aws-iot-device-client)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUGOPT "")
set(CMAKE_BUILD_TYPE Debug) # Switch to Release for the "Release" build
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

#########################################
# OpenSSL dependency                    #
#########################################
find_package(OpenSSL REQUIRED)

#########################################
# AWS IoT v2 SDK C++ dependency         #
#########################################

if (BUILD_SDK)
    # Download and unpack aws iot device sdk cpp v2 at configure time
    set(BUILD_DEPS ON CACHE BOOL "Build dependencies for the AWS SDK" FORCE)
    configure_file(CMakeLists.txt.awssdk
            aws-iot-device-sdk-cpp-v2-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/aws-iot-device-sdk-cpp-v2-download )
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/aws-iot-device-sdk-cpp-v2-download )

    # Add aws-iot-device-sdk-cpp-v2 directly to the build
    add_subdirectory(${CMAKE_BINARY_DIR}/aws-iot-device-sdk-cpp-v2-src
            ${CMAKE_BINARY_DIR}/aws-iot-device-sdk-cpp-v2-build)
else()
    include_directories(/include)
    link_directories(/lib)

    ### These next two lines are just a test to verify the Jobs SDK lib is available for sanity check
    find_library(jobs_lib IotJobs-cpp)
    message("-- *STATUS* Found jobs_lib: ${jobs_lib}")
endif()

### Now build the DC executable
add_executable(${DC_PROJECT_NAME} ${DC_SRC})
set_target_properties(${DC_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(${DC_PROJECT_NAME} PRIVATE "-DDEBUG_BUILD")

if (MSVC)
    target_compile_options(${DC_PROJECT_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${DC_PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic -Werror)
endif ()

target_link_libraries(${DC_PROJECT_NAME} IotIdentity-cpp IotJobs-cpp IotDeviceCommon-cpp IotDeviceDefender-cpp IotSecureTunneling-cpp aws-crt-cpp aws-c-mqtt aws-c-auth aws-c-io s2n aws-c-cal aws-c-http aws-c-compression aws-c-common aws-c-iot)
target_link_libraries(${DC_PROJECT_NAME} OpenSSL::SSL)
target_link_libraries(${DC_PROJECT_NAME} OpenSSL::Crypto)

if (BUILD_TEST_DEPS)
    # Download and unpack googletest at configure time
    configure_file(CMakeLists.txt.gtest
            googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )

    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This adds
    # the following targets: gtest, gtest_main, gmock
    # and gmock_main
    add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
            ${CMAKE_BINARY_DIR}/googletest-build)
endif()

add_subdirectory(test)
