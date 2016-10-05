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

set(GFLAGS_INCLUDE_DIRECTORIES "/usr/include")
set(GFLAGS_LIBRARIES "-lgflags")
set(GTEST_LIBRARIES "-lgtest")

include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libmill
		${PROTOBUF_INCLUDE_DIRECTORIES}
		${CRYPTO_INCLUDE_DIRECTORIES}
		${GLOG_INCLUDE_DIRECTORIES}
        ${GFLAGS_INCLUDE_DIRECTORIES})

add_subdirectory(libmill)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pipe -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2")

add_definitions(-DMILL_USE_PREFIX=1)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
		COMMAND ${PROTOBUF_EXE}
			--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
			-I${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol
			${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol/kinetic.proto
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol/kinetic.proto)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic-proxy-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/kinetic-proxy-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/kinetic-proxy-headers.rl
        DEPENDS bin/kinetic-proxy-headers.rl)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/rawx-server-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/rawx-server-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/rawx-server-headers.rl
        DEPENDS bin/rawx-server-headers.rl)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/common-server-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/common-server-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/common-server-headers.rl
        DEPENDS bin/common-server-headers.rl)

add_library(oio-http-parser SHARED
		http-parser/http_parser.c
		http-parser/http_parser.h)

add_library(oio-utils SHARED
        utils/utils.h utils/utils.cpp
        utils/net.h utils/net.cpp
        utils/Http.h utils/Http.cpp)
target_link_libraries(oio-utils
        oio-http-parser
        mill_s ${CRYPTO_LIBRARIES} ${GLOG_LIBRARIES})

add_library(oio-server SHARED
		bin/MillDaemon.h
		bin/MillDaemon.cpp
        bin/common-server-headers.h
        ${CMAKE_CURRENT_BINARY_DIR}/common-server-headers.cpp
        bin/common-header-parser.h)

add_library(oio-data SHARED
        oio/api/blob.cpp
        oio/api/blob.h)
target_link_libraries(oio-data oio-utils)


add_library(oio-data-local SHARED
        oio/local/blob.cpp
        oio/local/blob.h)
target_link_libraries(oio-data-local oio-data)

add_library(oio-data-rawx SHARED
        oio/http/imperative/blob.cpp
        oio/http/imperative/blob.h)
target_link_libraries(oio-data-rawx oio-data oio-http-parser)

add_library(oio-data-kinetic SHARED
        oio/kinetic/coro/blob.h
        oio/kinetic/coro/blob.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
        oio/kinetic/coro/rpc/Exchange.h
        oio/kinetic/coro/rpc/Exchange.cpp
        oio/kinetic/coro/rpc/Put.cpp
        oio/kinetic/coro/rpc/Put.h
        oio/kinetic/coro/rpc/Get.cpp
        oio/kinetic/coro/rpc/Get.h
        oio/kinetic/coro/rpc/GetKeyRange.cpp
        oio/kinetic/coro/rpc/GetKeyRange.h
        oio/kinetic/coro/rpc/GetNext.cpp
        oio/kinetic/coro/rpc/GetNext.h
        oio/kinetic/coro/rpc/Delete.cpp
        oio/kinetic/coro/rpc/Delete.h
		oio/kinetic/coro/rpc/GetLog.cpp
		oio/kinetic/coro/rpc/GetLog.h
        oio/kinetic/coro/client/ClientInterface.h
        oio/kinetic/coro/client/CoroutineClient.cpp
        oio/kinetic/coro/client/CoroutineClient.h
        oio/kinetic/coro/client/CoroutineClientFactory.cpp
        oio/kinetic/coro/client/CoroutineClientFactory.h
        oio/kinetic/coro/client/PendingExchange.cpp
        oio/kinetic/coro/client/PendingExchange.h)
target_link_libraries(oio-data-kinetic
        oio-utils oio-data oio-http-parser
		protobuf mill_s ${CRYPTO_LIBRARIES} ${GLOG_LIBRARIES})


add_executable(oio-kinetic-listener bin/kinetic-listener.cpp)
target_link_libraries(oio-kinetic-listener oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} mill_s)


add_executable(oio-kinetic-proxy
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic-proxy-headers.cpp
        bin/kinetic-proxy-headers.h
        bin/kinetic-proxy.cpp)
target_link_libraries(oio-kinetic-proxy
		oio-http-parser oio-server oio-data-kinetic
		${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})

add_executable(oio-rawx
        ${CMAKE_CURRENT_BINARY_DIR}/rawx-server-headers.cpp
        bin/rawx-server-headers.h
        bin/rawx-server.cpp)
target_link_libraries(oio-rawx
        oio-http-parser oio-server oio-data-local
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})



add_executable(test-utils tests/TestUtils.cpp)
target_link_libraries(test-utils oio-utils
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

add_executable(test-blob-local tests/TestBlobLocal.cpp)
target_link_libraries(test-blob-local oio-data-local
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

add_executable(test-blob-http tests/TestBlobHttp.cpp)
target_link_libraries(test-blob-http oio-data-rawx oio-http-parser
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

add_executable(test-http-client tests/TestHttpClient.cpp)
target_link_libraries(test-http-client oio-data-rawx
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

add_executable(test-kinetic-rpc tests/TestKineticRpc.cpp)
target_link_libraries(test-kinetic-rpc oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

add_executable(test-blob-kinetic tests/TestBlobKinetic.cpp)
target_link_libraries(test-blob-kinetic oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

add_executable(test-random-put tests/TestRandomPut.cpp)
target_link_libraries(test-random-put oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)

include_directories(AFTER /usr/src/gtest)
add_library(gtest STATIC /usr/src/gtest/src/gtest-all.cc)
target_link_libraries(gtest pthread)
