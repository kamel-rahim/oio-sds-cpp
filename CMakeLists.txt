cmake_minimum_required(VERSION 3.2)
project(oio-kinetic)

include(ExternalProject)
find_package(PkgConfig)

set(LINKER_LANGUAGE CXX)

pkg_check_modules(CRYPTO libcrypto REQUIRED)

macro(dump_dependency_components _DEP)
message("${_DEP}")
message(" system:${${_DEP}_SYSTEM} guess:${${_DEP}_GUESS} exe:${${_DEP}_EXE}")
message(" incdir: ${${_DEP}_INCLUDE_DIRS}")
message(" libdir: ${${_DEP}_LIBRARY_DIRS}")
message(" lib: ${${_DEP}_LIBRARIES}")
endmacro()

if (NOT DEFINED CPPLINT_EXE)
	find_program(CPPLINT_EXE cpplint)
endif()

option(SYS "General trigger superseding all *_SYSTEM options" ON)
option(GUESS "General trigger superseding all *_GUESS options" ON)
option(TESTING "Enables the tests and their dependencies" ON)

################################################################################
### liberasurecode
### Often poorly packaged, it lacks a .pc file for pkg-config. We accept
### explicit paths, but try to guess if not specified

option(EC_SYSTEM "Use system's liberasurecode" ON)
option(EC_GUESS "Try to find liberasurecode in standard places" ON)
if (DEFINED EC_INCDIR AND DEFINED EC_LIBDIR)
    find_library(EC_LIBRARIES
            NAMES erasurecode
            PATHS ${EC_LIBDIR})
    find_path(EC_INCLUDE_DIRS
            NAMES liberasurecode/erasurecode.h
            PATHS ${EC_INCDIR})
elseif (SYS AND EC_SYSTEM)
    pkg_check_modules(EC erasurecode-1 REQUIRED)
elseif (GUESS AND EC_GUESS)
    find_library(EC_LIBRARIES
            NAMES erasurecode
            HINTS /usr/lib /usr/lib64)
    find_path(EC_INCLUDE_DIRS
            NAMES liberasurecode/erasurecode.h
            PATHS /usr/include)
else ()
    set(EC_LIBRARIES EC_LIBRARIES-NOTFOUND)
    set(EC_INCLUDE_DIRS EC_INCLUDE_DIRS-NOTFOUND)
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
    find_path(GFLAGS_INCLUDE_DIRS
            NAMES gflags/gflags.h
            PATHS ${GFLAGS_INCDIR})
elseif (SYS AND GFLAGS_SYSTEM)
    pkg_check_modules(GFLAGS gflags REQUIRED)
elseif (GUESS AND GFLAGS_GUESS)
    find_library(GFLAGS_LIBRARIES
            NAMES gflags
            HINTS /usr/lib /usr/lib64)
    find_path(GFLAGS_INCLUDE_DIRS
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
            -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
    ExternalProject_Get_Property(googleflags source_dir)
    ExternalProject_Get_Property(googleflags binary_dir)
    set(GFLAGS_INCLUDE_DIRS ${binary_dir}/include)
    set(GFLAGS_LIBRARY_DIRS ${binary_dir}/lib)
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
    find_path(GTEST_INCLUDE_DIRS
            NAMES gtest/gtest.h
            PATHS ${GTEST_INCDIR})
elseif (SYS AND GTEST_SYSTEM)
    pkg_check_modules(GTEST libgtest REQUIRED)
elseif (GUESS AND GTEST_GUESS)
    find_library(GTEST_LIBRARIES
            NAMES gtest
            HINTS /usr/lib /usr/lib64)
    find_path(GTEST_INCLUDE_DIRS
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
            -DBUILD_GMOCK=ON
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
### Google Mock

option(GMOCK_SYSTEM "Use system's googlemock" ON)
option(GMOCK_GUESS "Try to find googlemock in standard places" OFF)
if (DEFINED GMOCK_INCDIR AND DEFINED GMOCK_LIBDIR)
    find_library(GMOCK_LIBRARIES
            NAMES gmock
            PATHS ${GMOCK_LIBDIR})
    find_path(GMOCK_INCLUDE_DIRS
            NAMES gmock/gmock.h
            PATHS ${GMOCK_INCDIR})
elseif (SYS AND GMOCK_SYSTEM)
    pkg_check_modules(GMOCK google-mock REQUIRED)
elseif (GUESS AND GMOCK_GUESS)
    find_library(GMOCK_LIBRARIES
            NAMES gmock
            HINTS /usr/lib /usr/lib64 /usr/local/lib)
    find_path(GMOCK_INCLUDE_DIRS
            NAMES gmock/gmock.h
            PATHS /usr/include /usr/local/include)
else ()
    ExternalProject_Add(googlemock
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rd/gtest
            BINARY_DIR ${CMAKE_BINARY_DIR}/3rd/gtest-bin
            TMP_DIR    ${CMAKE_BINARY_DIR}/3rd/gtest-tmp
            STAMP_DIR  ${CMAKE_BINARY_DIR}/3rd/gtest-stamp
            CMAKE_ARGS
            -DBUILD_SHARED_LIBS=ON
            -DBUILD_GTEST=OFF
            -DBUILD_GMOCK=ON
            -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
    ExternalProject_Get_Property(googlemock source_dir)
    ExternalProject_Get_Property(googlemock binary_dir)
    set(GMOCK_INCLUDE_DIRS ${source_dir}/googlemock/include)
    set(GMOCK_LIBRARY_DIRS ${binary_dir}/googlemock)
    set(GMOCK_LIBRARIES gmock)
endif ()
dump_dependency_components("GMOCK")

################################################################################
### Google Log

option(GLOG_SYSTEM "Use system's googlelog" ON)
option(GLOG_GUESS "Try to find googlelog in standard places" OFF)
if (DEFINED GLOG_INCDIR AND DEFINED GLOG_LIBDIR)
    find_library(GLOG_LIBRARIES
            NAMES glog
            PATHS ${GLOG_LIBDIR})
	find_path(GLOG_INCLUDE_DIRS
            NAMES glog/logging.h
            PATHS ${GLOG_INCDIR})
elseif (SYS AND GLOG_SYSTEM)
    pkg_check_modules(GLOG libglog REQUIRED)
elseif (GUESS AND GLOG_GUESS)
    find_library(GLOG_LIBRARIES
            NAMES glog
            HINTS /usr/lib /usr/lib64)
	find_path(GLOG_INCLUDE_DIRS
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
            -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
    ExternalProject_Get_Property(googlelog source_dir)
    ExternalProject_Get_Property(googlelog binary_dir)
    set(GLOG_INCLUDE_DIRS ${source_dir}/src ${binary_dir})
    set(GLOG_LIBRARY_DIRS ${binary_dir})
    set(GLOG_LIBRARIES glog)
endif ()
dump_dependency_components("GLOG")

################################################################################
### LibMill

option(MILL_SYSTEM "Use system's libmill" OFF)
option(MILL_GUESS "Guess the libmill places at system's standard" OFF)
if (DEFINED MILL_INCDIR AND DEFINED MILL_LIBDIR)
    find_library(MILL_LIBRARIES
            NAMES mill
            PATHS ${MILL_LIBDIR})
	find_path(MILL_INCLUDE_DIRS
            NAMES libmill.h
            PATHS ${MILL_INCDIR})
elseif (SYS AND MILL_SYSTEM)
    pkg_check_modules(MILL mill REQUIRED)
elseif (GUESS AND MILL_GUESS)
    find_library(MILL_LIBRARIES
            NAMES mill
            HINTS /usr/lib /usr/lib64)
	find_path(MILL_INCLUDE_DIRS
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
			-DCMAKE_C_FLAGS=-I${CMAKE_SOURCE_DIR}/3rd/libmill
            PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
            INSTALL_COMMAND "")
    ExternalProject_Get_Property(libmill source_dir)
    ExternalProject_Get_Property(libmill binary_dir)
	set(MILL_INCLUDE_DIRS ${source_dir})
	set(MILL_LIBRARY_DIRS ${binary_dir})
    set(MILL_LIBRARIES libmill.so)
endif ()
dump_dependency_components("MILL")

################################################################################
### NodeJS's http-parser
### coming as a git submodule, we directly use its sources (2 files) in our
### project

set(HTTPPARSER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rd)

################################################################################
### Kinetic's Protocol
### No need to build anything here, we expect protobuf to come from the system,
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
### ATTR

option(ATTR_SYSTEM "Use system's libattr" ON)
option(ATTR_GUESS "Guess the libattr places at system's standards" ON)
if (DEFINED ATTR_INCDIR AND DEFINED ATTR_LIBDIR)
    find_library(ATTR_LIBRARIES
            NAMES attr
            PATHS ${ATTR_LIBDIR})
    find_path(ATTR_INCLUDE_DIRS
            NAMES attr/xattr.h
            PATHS ${ATTR_INCDIR})
elseif (SYS AND ATTR_SYSTEM)
    pkg_check_modules(ATTR libattr REQUIRED)
elseif (GUESS AND ATTR_GUESS)
    find_library(ATTR_LIBRARIES
            NAMES attr
            HINTS /usr/lib /usr/lib64)
    find_path(ATTR_INCLUDE_DIRS
            NAMES attr/xattr.h
            PATHS /usr/include)
else ()
    set(ATTR_INCLUDE_DIRS ATTR_INCLUDE_DIRS-NOTFOUND)
    set(ATTR_LIBRARIES ATTR_LIBRARIES-NOTFOUND)
endif ()
dump_dependency_components("ATTR")

################################################################################
### RadipJSON

option(RAPIDJSON_SYSTEM "Use system's rapidjson" ON)
option(RAPIDJSON_GUESS "Guess the libattr places at standard system places" OFF)
if (DEFINED RAPIDJSON_INCDIR AND DEFINED RAPIDJSON_LIBDIR)
    find_path(RAPIDJSON_INCLUDE_DIRS
            NAMES rapidjson/document.h rapidjson/filereadstream.h rapidjson/writer.h
            PATHS ${RAPIDJSON_INCDIR})
elseif (SYS AND RAPIDJSON_SYSTEM)
    pkg_check_modules(RAPIDJSON rapidjson REQUIRED)
elseif (GUESS AND RAPIDJSON_GUESS)
    find_path(RAPIDJSON_INCLUDE_DIRS
            NAMES rapidjson/document.h rapidjson/filereadstream.h rapidjson/writer.h
            PATHS /usr/include)
else ()
    # Compiling rapidjson is only useful to execute test suites
    find_path(RAPIDJSON_INCLUDE_DIRS
            NAMES rapidjson/document.h rapidjson/filereadstream.h rapidjson/writer.h
            PATHS ${CMAKE_CURRENT_SOURCE_DIR}/3rd/rapidjson/include)
endif ()
dump_dependency_components("RAPIDJSON")


################################################################################
### Proxygen 
### No need to build anything here, we expect proxygen to come from the system,
### at least for the moment.

option(PROXYGEN_SYSTEM "Use system's Proxygen" OFF)
option(PROXYGEN_GUESS "Guess the Proxygen places at system's standard" OFF)
if (DEFINED PROXYGEN_INCDIR AND DEFINED PROXYGEN_LIBDIR)
    find_library(PROXYGENCURL_LIBRARIES
            NAMES proxygencurl
            PATHS ${GLOG_LIBDIR})
    find_library(PROXYGENHTTPSERVER_LIBRARIES
            NAMES proxygenhttpserver
            PATHS ${GLOG_LIBDIR})	  
    find_path(GLOG_INCLUDE_DIRS
            NAMES proxygen
            PATHS ${GLOG_INCDIR})
elseif (SYS AND PROXYGEN_SYSTEM)
      pkg_check_modules(PROXYGENCURL libproxygencurl REQUIRED)
      pkg_check_modules(PROXYGENHTTPSERVER libproxygenhttpserver REQUIRED)
elseif (GUESS AND PROXYGEN_GUESS)
find_library(PROXYGENCURL_LIBRARIES
        NAMES proxygencurl
        HINTS /usr/lib /usr/lib64 /usr/local/lib)
find_library(PROXYGENHTTPSERVER_LIBRARIES
        NAMES proxygenhttpserver
        HINTS /usr/lib /usr/lib64 /usr/local/lib)
find_path(PROXYGEN_INCLUDE_DIRS
	NAMES proxygen
	HINTS /usr/lib /usr/lib64 /usr/local/lib)
else()
  execute_process(COMMAND "./deps.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rd/proxygen/proxygen)
endif ()  
dump_dependency_components("PROXYGENCURL")
dump_dependency_components("PROXYGENHTTPSERVER")
################################################################################
### Folly 
option(FOLLY_SYSTEM "Use system's Folly" OFF)
option(FOLLY_GUESS "Guess the Folly places at system's standard" OFF)
if (DEFINED FOLLY_INCDIR AND DEFINED FOLLY_LIBDIR)
    find_library(FOLLY_LIBRARIES
            NAMES folly
            PATHS ${GLOG_LIBDIR})
    find_path(FOLLY_INCLUDE_DIRS
            NAMES folly
            PATHS ${GLOG_INCDIR})
elseif (SYS AND FOLLY_SYSTEM)
      pkg_check_modules(FOLLY libfolly REQUIRED)
elseif (GUESS AND FOLLY_GUESS)
find_library(FOLLY_LIBRARIES
        NAMES folly
        HINTS /usr/lib /usr/lib64 /usr/local/lib)      
find_path(FOLLY_INCLUDE_DIRS
	NAMES folly
	HINTS /usr/lib /usr/lib64 /usr/local/lib)
endif ()      
dump_dependency_components("FOLLY")
################################################################################
### Wrangle
option(WANGLE_SYSTEM "Use system's Wangle" OFF)
option(WANGLE_GUESS "Guess the Wangle places at system's standard" OFF)
if (DEFINED WANGLE_INCDIR AND DEFINED WANGLE_LIBDIR)
    find_library(WANGLE_LIBRARIES
            NAMES wangle
            PATHS ${GLOG_LIBDIR})
    find_path(WANGLE_INCLUDE_DIRS
            NAMES wangle
            PATHS ${GLOG_INCDIR})
elseif (SYS AND WANGLE_SYSTEM)
    pkg_check_modules(WANGLE libwangle REQUIRED)
elseif (GUESS AND FOLLY_GUESS)
find_library(WANGLE_LIBRARIES
        NAMES wangle
        HINTS /usr/lib /usr/lib64 /usr/local/lib)
find_path(WANGLE_INCLUDE_DIRS
	NAMES wangle
	HINTS /usr/lib /usr/lib64 /usr/local/lib)
endif ()      
dump_dependency_components("WANGLE")
################################################################################

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pipe -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wclobbered")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++14")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2")

set(CMAKE_C_FLAGS_DEBUG "-O0 -fno-inline")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2")

add_library(oio-http-parser SHARED
        3rd/http-parser/http_parser.c
        3rd/http-parser/http_parser.h)

add_definitions(-DMILL_USE_PREFIX=1)

add_subdirectory(src)
add_subdirectory(bin)

if (TESTING)
    ENABLE_TESTING()
    add_subdirectory(tests/unit)
    add_subdirectory(tests/func)
endif()
