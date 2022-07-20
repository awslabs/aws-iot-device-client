cmake_minimum_required(VERSION 3.10)

set(GTEST_PROJECT test-aws-iot-device-client)

enable_testing()

if (NOT BUILD_TEST_DEPS)
    message("Gtest Cmake")
    find_package(GTest REQUIRED)
endif ()

#########################################
# Source Files                          #
#########################################

file(GLOB CONFIG_SRC "../source/config/*.cpp")
file(GLOB LOG_SRC "../source/logging/*.cpp")
file(GLOB UTIL_SRC "../source/util/*.cpp")

file(GLOB DC_SRC "../source/*.cpp" /
        "../source/*.c" /
        ${CONFIG_SRC} /
        ${LOG_SRC} /
        ${UTIL_SRC})

if (NOT EXCLUDE_DD)
    file(GLOB DD_SRC "../source/devicedefender/*.cpp")
    list(APPEND DC_SRC ${DD_SRC})
endif ()

if (NOT EXCLUDE_JOBS)
    file(GLOB JOBS_SRC "../source/jobs/*.cpp")
    list(APPEND DC_SRC ${JOBS_SRC})
endif ()

if (NOT EXCLUDE_ST)
    file(GLOB ST_SRC "../source/tunneling/*.cpp")
    list(APPEND DC_SRC ${ST_SRC})
endif ()

if (NOT EXCLUDE_FP)
    file(GLOB FP_SRC "../source/fleetprovisioning/*.cpp")
    list(APPEND DC_SRC ${FP_SRC})
endif ()


if (NOT EXCLUDE_SHADOW)
    if (NOT EXCLUDE_CONFIG_SHADOW)
        file(GLOB CONFIG_SHADOW_SRC "../source/shadow/ConfigShadow.cpp")
        list(APPEND DC_SRC ${CONFIG_SHADOW_SRC})
    endif ()
    if (NOT EXCLUDE_SAMPLE_SHADOW)
        file(GLOB SAMPLE_SHADOW_SRC "../source/shadow/SampleShadowFeature.cpp")
        list(APPEND DC_SRC ${SAMPLE_SHADOW_SRC})
    endif ()
endif ()

if (NOT EXCLUDE_SENSOR_PUBLISH)
    file(GLOB SENSOR_PUBLISH_SRC "../source/sensor-publish/*.cpp")
    list(APPEND DC_SRC ${SENSOR_PUBLISH_SRC})
endif ()

#########################################
# Test Files                            #
#########################################

file(GLOB CONFIG_TST "./config/*.cpp")
file(GLOB UTIL_TST "./util/*.cpp")
file(GLOB LOG_TST "./logging/*.cpp")
file(GLOB FP_TST "./fleetprovisioning/*.cpp")
file(GLOB CONFIG_SHADOW_TST "./shadow/TestConfigShadowFeature.cpp")
file(GLOB SAMPLE_SHADOW_TST "./shadow/TestSampleShadowFeature.cpp")
file(GLOB DC_TST "./*.cpp" /
        ${CONFIG_TST}
        ${LOG_TST}
        ${UTIL_TST})

file(GLOB JOBS_TST "./jobs/*.cpp")

if (NOT EXCLUDE_JOBS)
    list(APPEND DC_TST ${JOBS_TST})
endif ()

if (NOT EXCLUDE_ST)
    file(GLOB ST_TST "./tunneling/*.cpp")
    list(APPEND DC_TST ${ST_TST})
endif ()

if (NOT EXCLUDE_FP)
    list(APPEND DC_TST ${FP_TST})
endif ()

if (NOT EXCLUDE_SHADOW)
    if (NOT EXCLUDE_CONFIG_SHADOW)
        list(APPEND DC_TST ${CONFIG_SHADOW_TST})
    endif()
    if (NOT EXCLUDE_SAMPLE_SHADOW)
        list(APPEND DC_TST ${SAMPLE_SHADOW_TST})
    endif()
endif ()

if (NOT EXCLUDE_SENSOR_PUBLISH)
    file(GLOB SENSOR_PUBLISH_TST "./sensor-publish/*.cpp")
    list(APPEND DC_TST ${SENSOR_PUBLISH_TST})
endif ()

list(APPEND DC_SRC ${DC_TST})
list(FILTER DC_SRC EXCLUDE REGEX ".*main.cpp$")

set(HEADERS)
add_executable(${GTEST_PROJECT} ${DC_SRC})

if (NOT EXCLUDE_JOBS)
    target_link_libraries(${GTEST_PROJECT} IotJobs-cpp)
endif ()

target_link_libraries(${GTEST_PROJECT} gtest gtest_main)
target_link_libraries(${GTEST_PROJECT} gmock gmock_main)
target_link_libraries(${GTEST_PROJECT} ${DEP_DC_LIBS})
target_link_libraries(${GTEST_PROJECT} OpenSSL::SSL)
target_link_libraries(${GTEST_PROJECT} OpenSSL::Crypto)

if (LINK_DL)
    target_link_libraries(${GTEST_PROJECT} dl)
endif ()

add_test(${GTEST_PROJECT} ${GTEST_PROJECT})
set_tests_properties(${GTEST_PROJECT} PROPERTIES ENVIRONMENT AWS_CRT_MEMORY_TRACING=1)

# Add custom target for detecting memory leaks while running unit tests.
find_program(VALGRIND_PATH valgrind)
if (VALGRIND_PATH)
    add_custom_target(
        ${GTEST_PROJECT}-leak-check
        COMMAND ${VALGRIND_PATH} --leak-check=yes --error-exitcode=1 ./${GTEST_PROJECT}
        DEPENDS ${GTEST_PROJECT})
endif()
