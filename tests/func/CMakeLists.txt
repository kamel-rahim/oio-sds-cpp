
if (NOT DEFINED MAVEN_EXE)
	find_program(MAVEN_EXE mvn)
endif()

ExternalProject_Add(kinetic-java
		DOWNLOAD_COMMAND ""
		SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rd/kinetic-java
		BINARY_DIR ${CMAKE_BINARY_DIR}/3rd/kinetic-java
		TMP_DIR    ${CMAKE_BINARY_DIR}/3rd/kinetic-java-tmp
		STAMP_DIR  ${CMAKE_BINARY_DIR}/3rd/kinetic-java-stamp
		CONFIGURE_COMMAND ""
		CMAKE_COMMAND ""
		PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
		BUILD_COMMAND ${MAVEN_EXE} -B -f ${CMAKE_SOURCE_DIR}/3rd/kinetic-java/pom.xml clean package
		INSTALL_COMMAND "")

include_directories(BEFORE
		${CMAKE_SOURCE_DIR}/
		${CMAKE_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/3rd
		${CMAKE_BINARY_DIR}
		${GTEST_INCLUDE_DIRS}
		${GFLAGS_INCLUDE_DIRS}
		${GLOG_INCLUDE_DIRS}
		${MILL_INCLUDE_DIRS}
        ${RAPIDJSON_INCLUDE_DIRS})

link_directories(
		${GTEST_LIBRARY_DIRS}
		${GFLAGS_LIBRARY_DIRS}
		${GLOG_LIBRARY_DIRS}
		${MILL_LIBRARY_DIRS})

add_executable(test-blob-http TestBlobHttp.cpp)
target_link_libraries(test-blob-http
        oio-data-rawx oio-http-parser
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
#add_test(NAME blob/http COMMAND test-blob-http)

add_executable(test-blob-kinetic TestBlobKinetic.cpp)
target_link_libraries(test-blob-kinetic
        oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME blob/kinetic COMMAND test-blob-kinetic)

add_executable(test-blob-rawx TestBlobRawx.cpp)
target_link_libraries(test-blob-rawx
		oio-data-rawx
		${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME blob/rawx COMMAND test-blob-rawx)


add_executable(test-http-client-proxy TestHttpClient_OioProxy.cpp)
target_link_libraries(test-http-client-proxy
        oio-data-http
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
add_test(NAME http/vanilla COMMAND test-http-client-proxy)


add_executable(test-client TestClient.cpp)
target_link_libraries(test-client
        oio-directory oio-container oio-http-parser oio-utils
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})

#add_executable(test-http-client-rawx TestHttpClient_OioRawx.cpp)
#target_link_libraries(test-http-client-rawx
#        oio-data-rawx
#        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
#	add_test(NAME http/rawx COMMAND test-http-client-rawx)

add_executable(test-kinetic-rpc TestKineticRpc.cpp)
target_link_libraries(test-kinetic-rpc
        oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES} ${GTEST_LIBRARIES})
#add_test(NAME kinetic/rpc COMMAND test-kinetic-rpc)

