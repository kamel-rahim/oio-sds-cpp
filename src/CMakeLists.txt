
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
        mill_s ${CRYPTO_LIBRARIES} ${GLOG_LIBRARIES})


add_library(oio-data SHARED
        oio/api/blob.cpp
        oio/api/blob.h)
target_link_libraries(oio-data oio-utils)


add_library(oio-data-local SHARED
        oio/local/blob.cpp
        oio/local/blob.h)
target_link_libraries(oio-data-local oio-data)


add_library(oio-data-http SHARED
        oio/http/blob.cpp
        oio/http/blob.h)
target_link_libraries(oio-data-http oio-data oio-http-parser)


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
        protobuf mill_s ${CRYPTO_LIBRARIES} ${GLOG_LIBRARIES})