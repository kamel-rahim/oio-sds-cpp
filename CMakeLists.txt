cmake_minimum_required(VERSION 3.2)
project(oio-kinetic)

find_package(PkgConfig)
find_program(PROTOBUF_EXE protoc)
find_program(RAGEL_EXE ragel)

set(LINKER_LANGUAGE CXX)

pkg_check_modules(GLOG libglog REQUIRED)
pkg_check_modules(GFLAGS libgflags)
pkg_check_modules(PROTOBUF protobuf REQUIRED)
pkg_check_modules(CRYPTO libcrypto REQUIRED)

################################################################################
### liberasurecode
### Often poorly packaged, it lacks a .pc file for pkg-config.
### We accept explicit paths, but try to guess if not specified
pkg_check_modules(EC liberasurecode)
if (NOT EC_FOUND)
    if (DEFINED EC_LIBDIR)
        find_library(EC_LIBRARIES
            NAMES erasurecode
            HINTS /usr/lib /usr/lib64)
    else ()
        find_library(EC_LIBRARIES
            NAMES erasurecode
            PATHS ${EC_LIBDIR})
    endif()
    if (DEFINED EC_INCDIR)
        find_path(EC_INCLUDE_DIRECTORIES
            NAMES erasurecode.h
            PATHS ${EC_INCDIR})
    else ()
        find_path(EC_INCLUDE_DIRECTORIES
            NAMES erasurecode.h
            HINTS /usr/include/liberasurecode)
    endif ()
endif()
MESSAGE("EC libraries=${EC_LIBRARIES} header=${EC_INCLUDE_DIRECTORIES}")

set(GFLAGS_INCLUDE_DIRECTORIES "/usr/include")
set(GFLAGS_LIBRARIES "-lgflags")
set(GTEST_LIBRARIES "-lgtest")

include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd
        ${CMAKE_CURRENT_BINARY_DIR}
        3rd/libmill
        ${PROTOBUF_INCLUDE_DIRECTORIES}
        ${CRYPTO_INCLUDE_DIRECTORIES}
        ${GLOG_INCLUDE_DIRECTORIES}
        ${GFLAGS_INCLUDE_DIRECTORIES}
        ${EC_INCLUDE_DIRECTORIES})

add_subdirectory(3rd/libmill)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pipe -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2")

add_library(oio-http-parser SHARED
        3rd/http-parser/http_parser.c
        3rd/http-parser/http_parser.h)

add_definitions(-DMILL_USE_PREFIX=1)

add_subdirectory(src)
add_subdirectory(bin)

ENABLE_TESTING()

include_directories(AFTER /usr/src/gtest)
add_library(gtest STATIC /usr/src/gtest/src/gtest-all.cc)
target_link_libraries(gtest pthread)

add_subdirectory(tests/unit)
add_subdirectory(tests/func)
