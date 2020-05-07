cmake_minimum_required(VERSION 2.8)

project(bluez-alsa)

option(ENABLE_DEBUG "Enable debug build" ON)
set(CMAKE_BUILD_TYPE Release)

if(ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory (src)
