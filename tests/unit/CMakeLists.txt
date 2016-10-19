include_directories(BEFORE
		${CMAKE_SOURCE_DIR}/
		${CMAKE_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/3rd
		${CMAKE_BINARY_DIR}
		${GTEST_INCLUDE_DIRS}
		${GFLAGS_INCLUDE_DIRS}
		${GLOG_INCLUDE_DIRS}
		${MILL_INCLUDE_DIRS})

link_directories(
        ${GTEST_LIBRARY_DIRS}
		${MILL_LIBRARY_DIRS}
		${GFLAGS_LIBRARY_DIRS}
		${GLOG_LIBRARY_DIRS})

add_executable(test-utils TestUtils.cpp)
target_link_libraries(test-utils oio-utils oio-data
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME utils/hash COMMAND test-utils)

add_executable(test-blob-local TestBlobLocal.cpp)
target_link_libraries(test-blob-local oio-data-local
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME blob/local COMMAND test-blob-local)
