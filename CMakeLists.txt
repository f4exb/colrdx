cmake_minimum_required(VERSION 3.1.0)

project(colrdx)

# configure version
set(colrdx_VERSION_MAJOR "1")
set(colrdx_VERSION_MINOR "5")
set(colrdx_VERSION_PATCH "0")
set(colrdx_VERSION_SUFFIX "")

# Set additional project information
set(COMPANY "f4exb")
add_definitions("-DCOMPANY=\"${COMPANY}\"")
set(APPLICATION_NAME "Colrdx")
add_definitions("-DAPPLICATION_NAME=\"${APPLICATION_NAME}\"")
set(APPLICATION_MAINTAINER="Edouard Griffiths, F4EXB <f4exb06@gmail.com>")
set(COPYRIGHT "Copyright (c) 2020 Edouard Griffiths. All rights reserved.")
add_definitions("-DCOPYRIGHT=\"${COPYRIGHT}\"")
set(IDENTIFIER "com.${COMPANY}.${APPLICATION_NAME}")
add_definitions("-DIDENTIFIER=\"${IDENTIFIER}\"")

set(colrdx_VERSION "${colrdx_VERSION_MAJOR}.${colrdx_VERSION_MINOR}.${colrdx_VERSION_PATCH}${colrdx_VERSION_SUFFIX}")
message(STATUS "${APPLICATION_NAME} Version: ${colrdx_VERSION}")
add_definitions("-DCOLRDX_LIB_VERSION=\"${colrdx_VERSION_MAJOR}.${colrdx_VERSION_MINOR}.${colrdx_VERSION_PATCH}\"")
add_definitions("-DCOLRDX_VERSION=\"${colrdx_VERSION}\"")

# Requirements
find_package(PkgConfig REQUIRED)
find_package(Curses REQUIRED)

set(colrdx_SOURCES
    src/colrdx.c
    src/dsplnkst.c
)

set(colrdx_HEADERS
    src/display.h
)

include_directories(
    ${CURSES_INCLUDE_DIRS}
)

add_executable(colrdx
    ${colrdx_SOURCES}
)

target_link_libraries(colrdx
    ${CURSES_LIBRARIES}
)

install(TARGETS colrdx DESTINATION ${INSTALL_BIN_DIR})
