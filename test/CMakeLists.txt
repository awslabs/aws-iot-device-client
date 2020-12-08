cmake_minimum_required(VERSION 2.6)

set(GTEST_PROJECT test-aws-iot-device-client)

enable_testing()

if (NOT BUILD_TEST_DEPS)
    message("Gtest Cmake")
    find_package(GTest REQUIRED)
endif()

#file(GLOB DC_SRC "../source/*.cpp" /
#        "../source/*.c" /
#        "../source/**/*.cpp" /
#        "../source/**/*.c")

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

file(GLOB DD_SRC "../source/devicedefender/*.cpp")
file(GLOB JOBS_SRC "../source/jobs/*.cpp")
file(GLOB ST_SRC "../source/tunneling/*.cpp")

if (NOT SECURE_TUNNEL_COMPONENT)
    list(APPEND DC_SRC ${DD_SRC} ${JOBS_SRC})
endif()
list(APPEND DC_SRC ${ST_SRC})

#########################################
# Test Files                            #
#########################################

file(GLOB CONFIG_TST "./config/*.cpp")
file(GLOB UTIL_TST "./util/*.cpp")
file(GLOB DC_TST "./*.cpp" /
        ${CONFIG_TST}
        ${UTIL_TST})

file(GLOB JOBS_TST "./jobs/*.cpp")

if (NOT SECURE_TUNNEL_COMPONENT)
    list(APPEND DC_TST ${JOBS_TST})
endif()

list(APPEND DC_SRC ${DC_TST})
list(FILTER DC_SRC EXCLUDE REGEX ".*main.cpp$")

set(HEADERS)

add_executable(${GTEST_PROJECT} ${DC_SRC})
target_link_libraries(${GTEST_PROJECT} gtest gtest_main)
target_link_libraries(${GTEST_PROJECT} ${DEP_DC_LIBS})
target_link_libraries(${GTEST_PROJECT} OpenSSL::SSL)
target_link_libraries(${GTEST_PROJECT} OpenSSL::Crypto)

add_test(${GTEST_PROJECT} ${GTEST_PROJECT})
