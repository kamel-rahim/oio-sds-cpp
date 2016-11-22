include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
        ${HTTPPARSER_INCLUDE_DIRS}
		${MILL_INCLUDE_DIRS}
		${GFLAGS_INCLUDE_DIRS}
		${GTEST_INCLUDE_DIRS}
		${GLOG_INCLUDE_DIRS}
		${EC_INCLUDE_DIRS}
		${ATTR_INCLUDE_DIRS}
		${PROTOBUF_INCLUDE_DIRS}
        ${RAPIDJSON_INCLUDE_DIRS})

link_directories(
        ${MILL_LIBRARY_DIRS}
        ${GFLAGS_LIBRARY_DIRS}
        ${GLOG_LIBRARY_DIRS}
        ${EC_LIBRARY_DIRS}
        ${ATTR_LIBRARY_DIRS}
        ${PROTOBUF_LIBRARY_DIRS})

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
        COMMAND ${PROTOBUF_EXE}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        -I${CMAKE_SOURCE_DIR}/3rd/kinetic-protocol
        ${CMAKE_SOURCE_DIR}/3rd/kinetic-protocol/kinetic.proto
        DEPENDS ${CMAKE_SOURCE_DIR}/3rd/kinetic-protocol/kinetic.proto)


add_library(oio-utils SHARED
        utils/utils.h utils/utils.cpp
        utils/net.h utils/net.cpp
        utils/Http.h utils/Http.cpp)
target_link_libraries(oio-utils
        oio-http-parser
        ${MILL_LIBRARIES} ${CRYPTO_LIBRARIES}
		${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})


add_library(oio-data SHARED
        oio/api/blob.cpp
        oio/api/blob.h)
target_link_libraries(oio-data oio-utils)


add_library(oio-data-mem SHARED
        oio/mem/blob.cpp
        oio/mem/blob.h)
target_link_libraries(oio-data-mem oio-data)

add_library(oio-data-local SHARED
        oio/local/blob.cpp
        oio/local/blob.h)
target_link_libraries(oio-data-local oio-data)

add_library(oio-data-ec SHARED
        oio/ec/blob.cpp
        oio/ec/blob.h)
target_link_libraries(oio-data-ec
        oio-data oio-data-local
        ${EC_LIBRARIES})

add_library(oio-data-router SHARED
        oio/router/blob.cpp
        oio/router/blob.h)
target_link_libraries(oio-data-router
        oio-data oio-data-local oio-data-ec oio-data-rawx
        ${EC_LIBRARIES})


add_library(oio-data-http SHARED
        oio/http/blob.cpp
        oio/http/blob.h)
target_link_libraries(oio-data-http
        oio-data oio-http-parser)


add_library(oio-data-rawx SHARED
        oio/rawx/blob.cpp
        oio/rawx/blob.h)
target_link_libraries(oio-data-rawx oio-data-http)


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
        ${MILL_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CRYPTO_LIBRARIES}
        ${GLOG_LIBRARIES} ${ATTR_LIBRARIES})

