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

#########################################
# Test Files                            #
#########################################

file(GLOB CONFIG_TST "./config/*.cpp")
file(GLOB UTIL_TST "./util/*.cpp")
file(GLOB LOG_TST "./logging/*.cpp")
file(GLOB FP_TST "./fleetprovisioning/*.cpp")
file(GLOB DC_TST "./*.cpp" /
        ${CONFIG_TST}
        ${LOG_TST}
        ${UTIL_TST})

file(GLOB JOBS_TST "./jobs/*.cpp")

if (NOT EXCLUDE_JOBS)
    list(APPEND DC_TST ${JOBS_TST})
endif ()

if (NOT EXCLUDE_FP)
    list(APPEND DC_TST ${FP_TST})
endif ()

list(APPEND DC_SRC ${DC_TST})
list(FILTER DC_SRC EXCLUDE REGEX ".*main.cpp$")

set(HEADERS)
add_executable(${GTEST_PROJECT} ${DC_SRC})

if (NOT EXCLUDE_JOBS)
    target_link_libraries(${GTEST_PROJECT} IotJobs-cpp)
endif ()

target_link_libraries(${GTEST_PROJECT} gtest gtest_main)
target_link_libraries(${GTEST_PROJECT} ${DEP_DC_LIBS})
target_link_libraries(${GTEST_PROJECT} OpenSSL::SSL)
target_link_libraries(${GTEST_PROJECT} OpenSSL::Crypto)

if (LINK_DL)
    target_link_libraries(${GTEST_PROJECT} dl)
endif ()

add_test(${GTEST_PROJECT} ${GTEST_PROJECT})
