cmake_minimum_required(VERSION 3.10)
project(aws-iot-device-client)

option(BUILD_SDK "Builds the aws-iot-device-sdk-cpp-v2 as part of build. Only do this if the SDK is not installed." ON)
option(BUILD_TEST_DEPS "Builds the device client test dependencies as part of build. Only do this if you do not have gtest installed and you wish to run the tests." ON)
option(EXCLUDE_JOBS "Builds the device client without the IoT Jobs Feature." OFF)
option(EXCLUDE_DD "Builds the device client without the IoT Device Defender Feature." OFF)
option(EXCLUDE_ST "Builds the device client without the IoT Secure Tunneling Feature." OFF)
option(EXCLUDE_FP "Builds the device client without the IoT Fleet Provisioning Feature." OFF)
option(DISABLE_MQTT "WARNING: This flag is meant for internal use cases and will prevent Device Client features from operating. Builds the device client without MQTT Connection Management." OFF)

if (EXCLUDE_JOBS)
    add_definitions(-DEXCLUDE_JOBS)
endif()

if (EXCLUDE_DD)
    add_definitions(-DEXCLUDE_DD)
endif()

if (EXCLUDE_ST)
    add_definitions(-DEXCLUDE_ST)
endif()

if (EXCLUDE_FP)
    add_definitions(-DEXCLUDE_FP)
endif()

if (DISABLE_MQTT)
    add_definitions(-DDISABLE_MQTT)
endif()

list(APPEND CMAKE_MODULE_PATH "./sdk-cpp-workspace/lib/cmake")

file(GLOB CONFIG_SRC "source/config/*.cpp")
file(GLOB LOG_SRC "source/logging/*.cpp")
file(GLOB UTIL_SRC "source/util/*.cpp")

file(GLOB DC_SRC "source/*.cpp" /
        "source/*.c" /
        ${CONFIG_SRC}
        ${LOG_SRC}
        ${UTIL_SRC})

if (NOT EXCLUDE_DD)
    file(GLOB DD_SRC "source/devicedefender/*.cpp")
    list(APPEND DC_SRC ${DD_SRC})
endif()

if (NOT EXCLUDE_JOBS)
    file(GLOB JOBS_SRC "source/jobs/*.cpp")
    list(APPEND DC_SRC ${JOBS_SRC})
endif()

if (NOT EXCLUDE_ST)
    file(GLOB ST_SRC "source/tunneling/*.cpp")
    list(APPEND DC_SRC ${ST_SRC})
endif()

if (NOT EXCLUDE_FP)
    #TODO: Append FP source files
endif()

set(DC_PROJECT_NAME aws-iot-device-client)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUGOPT "")
set(CMAKE_BUILD_TYPE Debug) # Switch to Release for the "Release" build
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

option(GIT_VERSION "Updates the version number using the Git commit history" ON)
option(BUILD_SDK "Builds the aws-iot-device-sdk-cpp-v2 as part of build. Only do this if the SDK is not installed." ON)
option(BUILD_TEST_DEPS "Builds the device client test dependencies as part of build. Only do this if you do not have gtest installed and you wish to run the tests." ON)

#########################################
# Generate Version Information from Git #
#########################################
find_package(Git)
include(CMakeLists.txt.versioning)
# Now we inject the version information into a header that is accessible from the device client executable
configure_file("source/Version.h.in" "${PROJECT_BINARY_DIR}/Version.h")

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
endif()

##############################################
# Build the AWS IoT Device Client Executable #
##############################################
file(GLOB DC_SRC "source/*.cpp" /
        "source/*.c" /
        "source/**/*.cpp" /
        "source/**/*.c")
add_executable(${DC_PROJECT_NAME} ${DC_SRC})
set_target_properties(${DC_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(${DC_PROJECT_NAME} PRIVATE "-DDEBUG_BUILD")
# We need to add the project binary directory to the list of include directories
# for passing the version information down to the executable
include_directories("${PROJECT_BINARY_DIR}")

if (MSVC)
    target_compile_options(${DC_PROJECT_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${DC_PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic -Werror)
endif ()

set(DEP_DC_LIBS ${DEP_DC_LIBS} aws-crt-cpp aws-c-mqtt aws-c-auth aws-c-io s2n aws-c-cal aws-c-http aws-c-compression aws-c-common)

# Set SDK dependency libraries as needed

if (NOT EXCLUDE_DD)
    set(DEP_DC_LIBS ${DEP_DC_LIBS} IotDeviceCommon-cpp IotDeviceDefender-cpp aws-c-iot)
endif()

if (NOT EXCLUDE_JOBS)
    set(DEP_DC_LIBS ${DEP_DC_LIBS} IotJobs-cpp)
    target_link_libraries(${DC_PROJECT_NAME} IotJobs-cpp)
endif()

if (NOT EXCLUDE_ST)
    set(DEP_DC_LIBS ${DEP_DC_LIBS} IotDeviceCommon-cpp IotSecureTunneling-cpp aws-c-iot)
endif()

if (NOT EXCLUDE_FP)
    #TODO: Set FP required SDK libs
endif()

target_link_libraries(${DC_PROJECT_NAME} ${DEP_DC_LIBS})
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
