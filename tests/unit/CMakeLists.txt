add_executable(test-utils TestUtils.cpp)
target_link_libraries(test-utils oio-utils oio-data
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)
add_test(NAME utils/hash COMMAND test-utils)

add_executable(test-blob-local TestBlobLocal.cpp)
target_link_libraries(test-blob-local oio-data-local
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} gtest)
add_test(NAME blob/local COMMAND test-blob-local)