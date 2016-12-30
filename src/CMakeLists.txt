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
		utils/utils.hpp utils/utils.cpp
		utils/net.hpp utils/net.cpp
		utils/http.hpp utils/http.cpp)
target_link_libraries(oio-utils
        oio-http-parser
        ${MILL_LIBRARIES} ${CRYPTO_LIBRARIES}
		${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})


add_library(oio-data SHARED
		oio/api/common.cpp
		oio/api/common.hpp
        oio/api/blob.cpp
		oio/api/blob.hpp
		oio/api/resolver.hpp)
target_link_libraries(oio-data oio-utils)


add_library(oio-data-mem SHARED
		oio/blob/mem/blob.cpp
		oio/blob/mem/blob.hpp)
target_link_libraries(oio-data-mem oio-data)

add_library(oio-data-local SHARED
		oio/blob/local/blob.cpp
		oio/blob/local/blob.hpp)
target_link_libraries(oio-data-local oio-data)

add_library(oio-data-ec SHARED
		oio/blob/ec/blob.cpp
		oio/blob/ec/blob.hpp)
target_link_libraries(oio-data-ec
        oio-data oio-data-local
        ${EC_LIBRARIES})

add_library(oio-data-router SHARED
		oio/blob/router/blob.cpp
		oio/blob/router/blob.hpp)
target_link_libraries(oio-data-router
        oio-data oio-data-local oio-data-ec oio-data-rawx
        ${EC_LIBRARIES})


add_library(oio-data-http SHARED
		oio/blob/http/blob.cpp
		oio/blob/http/blob.hpp)
target_link_libraries(oio-data-http
        oio-data oio-http-parser)


add_library(oio-data-rawx SHARED
		oio/blob/rawx/blob.cpp
		oio/blob/rawx/blob.hpp)
target_link_libraries(oio-data-rawx oio-data-http)


add_library(oio-data-kinetic SHARED
		oio/blob/kinetic/coro/blob.hpp
		oio/blob/kinetic/coro/blob.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic.pb.h
		oio/blob/kinetic/coro/ClientInterface.h
		oio/blob/kinetic/coro/CoroutineClient.cpp
		oio/blob/kinetic/coro/CoroutineClient.h
		oio/blob/kinetic/coro/CoroutineClientFactory.cpp
		oio/blob/kinetic/coro/CoroutineClientFactory.h
		oio/blob/kinetic/coro/PendingExchange.cpp
		oio/blob/kinetic/coro/PendingExchange.h
		oio/blob/kinetic/coro/RPC.cpp
		oio/blob/kinetic/coro/RPC.h)
target_link_libraries(oio-data-kinetic
        oio-utils oio-data oio-http-parser
        ${MILL_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CRYPTO_LIBRARIES}
        ${GLOG_LIBRARIES} ${ATTR_LIBRARIES})

add_library(oio-directory SHARED
        oio/directory/dir.cpp
		oio/directory/dir.hpp)
target_link_libraries(oio-directory)

add_library(oio-sds SHARED
        oio/sds/sds.cpp
		oio/sds/sds.hpp)
target_link_libraries(oio-sds)

add_library(oio-container SHARED
        oio/container/container.cpp
		oio/container/container.hpp)
target_link_libraries(oio-container)

add_library(oio-content SHARED
        oio/content/content.cpp
		oio/content/content.hpp)
target_link_libraries(oio-content)
