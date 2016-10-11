add_executable(test-blob-http TestBlobHttp.cpp)
target_link_libraries(test-blob-http oio-data-rawx oio-http-parser
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)
add_test(NAME blob/http COMMAND test-blob-http)

add_executable(test-blob-kinetic TestBlobKinetic.cpp)
target_link_libraries(test-blob-kinetic oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)


add_executable(test-http-client-proxy TestHttpClient_OioProxy.cpp)
target_link_libraries(test-http-client-proxy oio-data-http
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)
add_test(NAME http/vanilla COMMAND test-http-client-proxy)

add_executable(test-http-client-proxy-rawx TestHttpClient_OioRawx.cpp)
target_link_libraries(test-http-client-proxy-rawx oio-data-rawx
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)
add_test(NAME http/rawx COMMAND test-http-client-proxy-rawx)


add_executable(test-kinetic-rpc TestKineticRpc.cpp)
target_link_libraries(test-kinetic-rpc oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)
add_test(NAME kinetic/rpc COMMAND test-kinetic-rpc)

