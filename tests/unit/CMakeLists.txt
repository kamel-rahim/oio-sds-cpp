link_directories(
        ${GTEST_LIBRARY_DIRS}
		${MILL_LIBRARY_DIRS})

include_directories(BEFORE
        ${GTEST_INCLUDE_DIRS})

add_executable(test-utils TestUtils.cpp)
target_link_libraries(test-utils oio-utils oio-data
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME utils/hash COMMAND test-utils)

add_executable(test-blob-local TestBlobLocal.cpp)
target_link_libraries(test-blob-local oio-data-local
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME blob/local COMMAND test-blob-local)
