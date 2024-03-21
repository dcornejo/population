cmake_minimum_required(VERSION 3.26)
project(hostmon)

set(CMAKE_CXX_STANDARD 20)

include_directories("/usr/local/include")

add_executable(hostmon main.cpp
        monitor.cpp
        monitor.h
        utilities.cpp
        utilities.h
        config.cpp
        config.h)
target_link_libraries(hostmon pthread)
