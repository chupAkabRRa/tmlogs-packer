cmake_minimum_required(VERSION 3.20)
project(TimeMachineLogsPacker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add xxHash
option(BUILD_SHARED_LIBS "Build shared libs" OFF)
set(XXHASH_BUILD_ENABLE_INLINE_API OFF)
set(XXHASH_BUILD_XXHSUM OFF)

add_subdirectory(
    src/third_party/xxhash/cmake_unofficial
    src/third_party/xxhash/build
    EXCLUDE_FROM_ALL
)

# All source files
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(packer ${SOURCES})

target_include_directories(
    packer PRIVATE
    src
    src/packer
    src/packer/hasher
    src/third_party
    src/utils
)

target_link_libraries(packer PRIVATE xxHash::xxhash)

