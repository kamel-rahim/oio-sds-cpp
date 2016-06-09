cmake_minimum_required(VERSION 3.2)
project(oio-kinetic)

find_package(PkgConfig)
find_program(PROTOBUF_EXE protoc)
find_program(RAGEL_EXE ragel)

set(LINKER_LANGUAGE CXX)

pkg_check_modules(GLOG libglog REQUIRED)
pkg_check_modules(PROTOBUF libprotobuf REQUIRED)
pkg_check_modules(CRYPTO libcrypto REQUIRED)

include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libmill
		${PROTOBUF_INCLUDE_DIRECTORIES}
		${CRYPTO_INCLUDE_DIRECTORIES}
		${GLOG_INCLUDE_DIRECTORIES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pipe -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2")

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
		COMMAND ${PROTOBUF_EXE}
			--cpp_out=${CMAKE_CURRENT_BINARY_DIR}
			-I${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol
			${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol/kinetic.proto
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kinetic-protocol/kinetic.proto)

add_subdirectory(libmill)

add_library(oio-kinetic-client SHARED
        utils/MillSocket.cpp
        utils/MillSocket.h
        utils/Socket.cpp
        utils/Socket.h
        utils/Addr.cpp
        utils/Addr.h
        utils/utils.h
        utils/utils.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
        oio/api/Upload.h
        oio/api/Download.h
        oio/api/Removal.h
        oio/api/Listing.h
        oio/kinetic/rpc/Request.h
        oio/kinetic/rpc/Exchange.h
        oio/kinetic/rpc/Put.cpp
        oio/kinetic/rpc/Put.h
        oio/kinetic/rpc/Get.cpp
        oio/kinetic/rpc/Get.h
        oio/kinetic/rpc/GetKeyRange.cpp
        oio/kinetic/rpc/GetKeyRange.h
        oio/kinetic/rpc/GetNext.cpp
        oio/kinetic/rpc/GetNext.h
        oio/kinetic/rpc/Delete.cpp
        oio/kinetic/rpc/Delete.h
        oio/kinetic/client/ClientInterface.h
        oio/kinetic/client/CoroutineClient.cpp
        oio/kinetic/client/CoroutineClient.h
        oio/kinetic/client/CoroutineClientFactory.cpp
        oio/kinetic/client/CoroutineClientFactory.h
        oio/kinetic/client/PendingExchange.cpp
        oio/kinetic/client/PendingExchange.h
        oio/kinetic/blob/Upload.cpp
        oio/kinetic/blob/Upload.h
        oio/kinetic/blob/Download.cpp
        oio/kinetic/blob/Download.h
        oio/kinetic/blob/Removal.cpp
        oio/kinetic/blob/Removal.h
        oio/kinetic/blob/Listing.cpp
        oio/kinetic/blob/Listing.h)

target_link_libraries(oio-kinetic-client
    protobuf mill_s ${CRYPTO_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(test-rpc
        oio/kinetic/client/TestExchange.cpp)
target_link_libraries(test-rpc oio-kinetic-client)

add_executable(test-client
        oio/kinetic/blob/TestClient.cpp)
target_link_libraries(test-client oio-kinetic-client)


add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
		COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/oio/kinetic/proxy/headers.rl
        DEPENDS oio/kinetic/proxy/headers.rl)

add_executable(oio-kinetic-proxy
        http-parser/http_parser.c
        http-parser/http_parser.h
        ${CMAKE_CURRENT_BINARY_DIR}/headers.cc
        oio/kinetic/proxy/headers.h
        oio/kinetic/proxy/main.cpp)
    target_link_libraries(oio-kinetic-proxy oio-kinetic-client ${GLOG_LIBRARIES})
