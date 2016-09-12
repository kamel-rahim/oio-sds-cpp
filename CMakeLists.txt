cmake_minimum_required(VERSION 3.2)
project(oio-kinetic)

find_package(PkgConfig)
find_program(PROTOBUF_EXE protoc)
find_program(RAGEL_EXE ragel)

set(LINKER_LANGUAGE CXX)

pkg_check_modules(GLOG libglog REQUIRED)
pkg_check_modules(PROTOBUF protobuf REQUIRED)
pkg_check_modules(CRYPTO libcrypto REQUIRED)

include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libmill
		${PROTOBUF_INCLUDE_DIRECTORIES}
		${CRYPTO_INCLUDE_DIRECTORIES}
		${GLOG_INCLUDE_DIRECTORIES})

add_subdirectory(libmill)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pipe -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2")

add_definitions(-DMILL_USE_PREFIX=1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
		COMMAND ${PROTOBUF_EXE}
			--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
			-I${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol
			${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol/kinetic.proto
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol/kinetic.proto)

add_library(oio-http-parser SHARED
		http-parser/http_parser.c
		http-parser/http_parser.h)

add_library(oio-kinetic-client SHARED
        utils/utils.h
        utils/utils.cpp
        utils/net.h
        utils/net.cpp
        utils/Http.h
        utils/Http.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
        oio/api/blob.h
        oio/kinetic/coro/rpc/Request.h
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
        oio/kinetic/coro/client/PendingExchange.h
        oio/kinetic/coro/blob.h
        oio/kinetic/coro/blob/Upload.cpp
        oio/kinetic/coro/blob/Download.cpp
        oio/kinetic/coro/blob/Removal.cpp
        oio/kinetic/coro/blob/Listing.cpp
        oio/http/imperative/blob.cpp
        oio/http/imperative/blob.h
		oio/local/blob/Upload.cpp
		oio/local/blob/Download.cpp
        oio/local/blob/Listing.cpp
        oio/local/blob/Removal.cpp
        oio/local/blob.h)

target_link_libraries(oio-kinetic-client
        oio-http-parser
		protobuf mill_s ${CRYPTO_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(test-kinetic-rpc tests/TestKineticExchange.cpp)
target_link_libraries(test-kinetic-rpc oio-kinetic-client)

add_executable(test-blob-kinetic tests/TestBlobKinetic.cpp)
target_link_libraries(test-blob-kinetic oio-kinetic-client)

add_executable(test-blob-http tests/TestBlobHttp.cpp)
target_link_libraries(test-blob-http oio-kinetic-client oio-http-parser)

add_executable(test-blob-local tests/TestBlobLocal.cpp)
target_link_libraries(test-blob-local oio-kinetic-client)

add_executable(test-http-client tests/TestHttpClient.cpp)
target_link_libraries(test-http-client oio-kinetic-client)

add_executable(oio-kinetic-listener bin/oio-kinetic-listener.cpp)
target_link_libraries(oio-kinetic-listener oio-kinetic-client
        ${GLOG_LIBRARIES} mill_s)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
		COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
		${CMAKE_CURRENT_SOURCE_DIR}/bin/headers.rl
        DEPENDS bin/headers.rl)

add_executable(oio-kinetic-proxy
        ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
		bin/headers.h
		bin/MillDaemon.h
		bin/MillDaemon.cpp
		bin/oio-kinetic-proxy.cpp)

target_link_libraries(oio-kinetic-proxy
		oio-http-parser oio-kinetic-client
		${GLOG_LIBRARIES})

add_executable(oio-rawx
        ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
        bin/headers.h
        bin/MillDaemon.h
		bin/MillDaemon.cpp
		bin/oio-rawx.cpp)

target_link_libraries(oio-rawx
        oio-http-parser oio-kinetic-client
        ${GLOG_LIBRARIES})
