# SPDX-License-Identifier: Apache-2.0

include_directories(${CMAKE_SOURCE_DIR}/src/ble)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/clib/inc)

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(console)

FILE(GLOB app_sources src/*.c)
FILE(GLOB ble_sources src/ble/*.c)
FILE(GLOB clib_sources clib/src/*.c)

set(SOURCES
	${app_sources}
	${ble_sources}
    ${clib_sources}
)

target_sources(app PRIVATE ${SOURCES})
