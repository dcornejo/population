cmake_minimum_required(VERSION 3.28)
project(hostmon)

set(CMAKE_CXX_STANDARD 20)

add_executable(hostmon main.cpp)
target_link_libraries(hostmon pthread)
