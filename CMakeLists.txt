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

# All source files except main.cpp and packet_test.cpp
file(GLOB_RECURSE SOURCES src/*.cpp)
list(REMOVE_ITEM SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test/packer_test.cpp
)

add_library(tmlp_lib STATIC ${SOURCES})

target_include_directories(
    tmlp_lib PRIVATE
    src
    src/packer
    src/packer/hasher
    src/third_party
    src/utils
)

target_link_libraries(tmlp_lib PRIVATE xxHash::xxhash)

# Main executable
add_executable(packer src/main.cpp)
target_link_libraries(packer PRIVATE tmlp_lib)

# Tests
enable_testing()

add_executable(packer_test src/test/packer_test.cpp)
target_link_libraries(packer_test PRIVATE tmlp_lib)

target_include_directories(
    packer_test PRIVATE
    src
    src/packer
    src/packer/hasher
    src/third_party
    src/utils
)
add_test(NAME PackerFunctionalTest COMMAND packer_test)

