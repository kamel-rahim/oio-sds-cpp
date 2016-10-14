cmake_minimum_required(VERSION 3.2)
project(oio-kinetic)

include(ExternalProject)
find_package(PkgConfig)

set(LINKER_LANGUAGE CXX)

pkg_check_modules(CRYPTO libcrypto REQUIRED)

macro(dump_dependency_components _DEP)
message("${_DEP}"
        " system=${${_DEP}_SYSTEM}"
        " guess=${${_DEP}_GUESS}"
        " incdir=${${_DEP}_INCLUDE_DIRS}"
        " libdir=${${_DEP}_LIBRARY_DIRS}"
        " lib=${${_DEP}_LIBRARIES}"
        " exe=${${_DEP}_EXE}")
endmacro()

################################################################################
### liberasurecode
### Often poorly packaged, it lacks a .pc file for pkg-config. We accept
### explicit paths, but try to guess if not specified

option(EC_SYSTEM "Use system's liberasurecode" ON)
option(EC_GUESS "Try to find liberasurecode in standard places" OFF)
if (DEFINED EC_INCDIR AND DEFINED EC_LIBDIR)
    find_library(EC_LIBRARIES
            NAMES erasurecode
            PATHS ${EC_LIBDIR})
    find_path(EC_INCLUDE_DIRECTORIES
            NAMES liberasurecode/erasurecode.h
            PATHS ${EC_INCDIR})
elseif (EC_SYSTEM)
    pkg_check_modules(EC erasurecode REQUIRED)
elseif (EC_GUESS)
    find_library(EC_LIBRARIES
            NAMES erasurecode
            HINTS /usr/lib /usr/lib64)
    find_path(EC_INCLUDE_DIRECTORIES
            NAMES liberasurecode/erasurecode.h
            PATHS /usr/include)
endif ()
dump_dependency_components("EC")

################################################################################
### Google Flags
### Often poorly packaged. The .pc file lacks, e.g. on Ubuntu 15. We accept
### system configuration, explicit paths, or paths guessed at standard places.

option(GFLAGS_SYSTEM "Use system's gflags" ON)
option(GFLAGS_GUESS "Try to find gflags in standard places" OFF)
if (DEFINED GFLAGS_INCDIR AND DEFINED GFLAGS_LIBDIR)
    find_library(GFLAGS_LIBRARIES
            NAMES gflags
            PATHS ${GFLAGS_LIBDIR})
    find_path(GFLAGS_INCLUDE_DIRECTORIES
            NAMES gflags/gflags.h
            PATHS ${GFLAGS_INCDIR})
elseif (GFLAGS_SYSTEM)
    pkg_check_modules(GFLAGS gflags REQUIRED)
elseif (GFLAGS_GUESS)
    find_library(GFLAGS_LIBRARIES
            NAMES gflags
            HINTS /usr/lib /usr/lib64)
    find_path(GFLAGS_INCLUDE_DIRECTORIES
            NAMES gflags/gflags.h
            PATHS /usr/include)
else ()
    ExternalProject_Add(googleflags
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rd/gflags
            BINARY_DIR ${CMAKE_BINARY_DIR}/3rd/gflags-bin
            TMP_DIR    ${CMAKE_BINARY_DIR}/3rd/gflags-tmp
            STAMP_DIR  ${CMAKE_BINARY_DIR}/3rd/gflags-stamp
            CMAKE_ARGS
            -DBUILD_SHARED_LIBS=ON
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
            -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
        ExternalProject_Get_Property(googleflags source_dir)
        ExternalProject_Get_Property(googleflags binary_dir)
    set(GFLAGS_INCLUDE_DIRS ${source_dir}/googleflags/include)
    set(GFLAGS_LIBRARY_DIRS ${binary_dir}/)
    set(GFLAGS_LIBRARIES gflags)
endif ()
dump_dependency_components("GFLAGS")

################################################################################
### Google Test
### Worst of all, there is probably a good reason to explain gtest's packages
### only ship headers and sources...

option(GTEST_SYSTEM "Use system's googletest" ON)
option(GTEST_GUESS "Try to find googletest in standard places" OFF)
if (DEFINED GTEST_INCDIR AND DEFINED GTEST_LIBDIR)
    find_library(GTEST_LIBRARIES
            NAMES gtest
            PATHS ${GTEST_LIBDIR})
    find_path(GTEST_INCLUDE_DIRECTORIES
            NAMES gtest/gtest.h
            PATHS ${GTEST_INCDIR})
elseif (GTEST_SYSTEM)
    pkg_check_modules(GTEST libgtest REQUIRED)
elseif (GTEST_GUESS)
    find_library(GTEST_LIBRARIES
            NAMES gtest
            HINTS /usr/lib /usr/lib64)
    find_path(GTEST_INCLUDE_DIRECTORIES
            NAMES gtest/gtest.h
            PATHS /usr/include)
else ()
    ExternalProject_Add(googletest
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rd/gtest
            BINARY_DIR ${CMAKE_BINARY_DIR}/3rd/gtest-bin
            TMP_DIR    ${CMAKE_BINARY_DIR}/3rd/gtest-tmp
            STAMP_DIR  ${CMAKE_BINARY_DIR}/3rd/gtest-stamp
            CMAKE_ARGS
            -DBUILD_SHARED_LIBS=ON
            -DBUILD_GTEST=ON
            -DBUILD_GMOCK=OFF
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
            -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
        ExternalProject_Get_Property(googletest source_dir)
        ExternalProject_Get_Property(googletest binary_dir)
    set(GTEST_INCLUDE_DIRS ${source_dir}/googletest/include)
    set(GTEST_LIBRARY_DIRS ${binary_dir}/googletest)
    set(GTEST_LIBRARIES gtest)
endif ()
dump_dependency_components("GTEST")

################################################################################
### Google Log

option(GLOG_SYSTEM "Use system's googlelog" ON)
option(GLOG_GUESS "Try to find googlelog in standard places" OFF)
if (DEFINED GLOG_INCDIR AND DEFINED GLOG_LIBDIR)
    find_library(GLOG_LIBRARIES
            NAMES glog
            PATHS ${GLOG_LIBDIR})
    find_path(GLOG_INCLUDE_DIRECTORIES
            NAMES glog/logging.h
            PATHS ${GLOG_INCDIR})
elseif (GLOG_SYSTEM)
    pkg_check_modules(GLOG libglog REQUIRED)
elseif (GLOG_GUESS)
    find_library(GLOG_LIBRARIES
            NAMES glog
            HINTS /usr/lib /usr/lib64)
    find_path(GLOG_INCLUDE_DIRECTORIES
            NAMES glog/logging.h
            PATHS /usr/include)
else ()
    ExternalProject_Add(googlelog
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rd/glog
            BINARY_DIR ${CMAKE_BINARY_DIR}/3rd/glog-bin
            TMP_DIR    ${CMAKE_BINARY_DIR}/3rd/glog-tmp
            STAMP_DIR  ${CMAKE_BINARY_DIR}/3rd/glog-stamp
            CMAKE_ARGS
            -DBUILD_SHARED_LIBS=ON
            -DWITH_THREADS=ON
            -DWITH_GFLAGS=OFF
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
            -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
        ExternalProject_Get_Property(googlelog source_dir)
        ExternalProject_Get_Property(googlelog binary_dir)
    set(GLOG_INCLUDE_DIRS ${source_dir}/googlelog/include)
    set(GLOG_LIBRARY_DIRS ${binary_dir}/googlelog)
    set(GLOG_LIBRARIES glog)
endif ()
dump_dependency_components("GLOG")

################################################################################
### LibMill

option(MILL_SYSTEM "Use system's libmill" OFF)
option(MILL_GUESS "Use system's libmill" OFF)
if (DEFINED MILL_INCDIR AND DEFINED MILL_LIBDIR)
    find_library(MILL_LIBRARIES
            NAMES mill
            PATHS ${MILL_LIBDIR})
    find_path(MILL_INCLUDE_DIRECTORIES
            NAMES libmill.h
            PATHS ${MILL_INCDIR})
elseif (MILL_SYSTEM)
    pkg_check_modules(MILL mill REQUIRED)
elseif (MILL_GUESS)
    find_library(MILL_LIBRARIES
            NAMES mill
            HINTS /usr/lib /usr/lib64)
    find_path(MILL_INCLUDE_DIRECTORIES
            NAMES libmill.h
            PATHS /usr/include)
else ()
	include_directories(${CMAKE_SOURCE_DIR}/3rd/libmill)
    ExternalProject_Add(libmill
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rd/libmill
            BINARY_DIR ${CMAKE_BINARY_DIR}/3rd/libmill-bin
            TMP_DIR    ${CMAKE_BINARY_DIR}/3rd/libmill-tmp
            STAMP_DIR  ${CMAKE_BINARY_DIR}/3rd/libmill-stamp
            CMAKE_ARGS
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
			-DCMAKE_C_FLAGS=-I${CMAKE_SOURCE_DIR}/3rd/libmill
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
        ExternalProject_Get_Property(libmill source_dir)
        ExternalProject_Get_Property(libmill binary_dir)
    set(MILL_INCLUDE_DIRECTORIES ${source_dir})
	set(MILL_LIBRARY_DIRS ${binary_dir})
    set(MILL_LIBRARIES libmill.so)
endif ()
dump_dependency_components("MILL")

################################################################################
### NodeJS's http-parser
### coming as a git submodule, we directly use its sources (2 files) in our
### project


################################################################################
### Kinetic's Protocol
### No need to build anything here, we excpect protobuf to come from the system,
### at least for the moment.

find_program(PROTOBUF_EXE protoc)
pkg_check_modules(PROTOBUF protobuf REQUIRED)
dump_dependency_components("PROTOBUF")

################################################################################
### RAGEL
### Only use at compile time, we currently expect the ragel binary is know from
### the PATH environment variable.

find_program(RAGEL_EXE ragel)
dump_dependency_components("RAGEL")

################################################################################

link_directories(
    ${GFLAGS_LIBRARY_DIRS}
    ${GLOG_LIBRARY_DIRS})

include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd
        ${CMAKE_CURRENT_BINARY_DIR}
        ${MILL_INCLUDE_DIRECTORIES}
        ${EC_INCLUDE_DIRECTORIES}
        ${CRYPTO_INCLUDE_DIRECTORIES}
        ${PROTOBUF_INCLUDE_DIRECTORIES}
        ${GLOG_INCLUDE_DIRECTORIES}
        ${GFLAGS_INCLUDE_DIRECTORIES})

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

add_subdirectory(tests/unit)
add_subdirectory(tests/func)
