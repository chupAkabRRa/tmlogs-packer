cmake_minimum_required(VERSION 4.0)
project(TimeMachineLogsPacker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    src
    src/packer
    src/packer/hasher
    src/third_party
)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(packer ${SOURCES})
