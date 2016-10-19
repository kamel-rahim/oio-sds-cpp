include_directories(BEFORE
		${CMAKE_SOURCE_DIR}/
		${CMAKE_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/3rd
		${CMAKE_BINARY_DIR}
        ${GFLAGS_INCLUDE_DIRS}
        ${GLOG_INCLUDE_DIRS}
        ${MILL_INCLUDE_DIRS}
        ${RAPIDJSON_INCLUDE_DIRS})

link_directories(
        ${MILL_LIBRARY_DIRS}
        ${GFLAGS_LIBRARY_DIRS}
        ${GLOG_LIBRARY_DIRS})

add_library(oio-server SHARED
        MillDaemon.h
        MillDaemon.cpp
        common-server-headers.h
        ${CMAKE_CURRENT_BINARY_DIR}/common-server-headers.cpp
        common-header-parser.h)


add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic-proxy-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/kinetic-proxy-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/kinetic-proxy-headers.rl
        DEPENDS kinetic-proxy-headers.rl)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/ec-proxy-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/ec-proxy-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ec-proxy-headers.rl
        DEPENDS ec-proxy-headers.rl)


add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/rawx-server-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/rawx-server-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rawx-server-headers.rl
        DEPENDS rawx-server-headers.rl)

add_custom_command(OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/common-server-headers.cpp
        COMMAND ${RAGEL_EXE} -o ${CMAKE_CURRENT_BINARY_DIR}/common-server-headers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/common-server-headers.rl
        DEPENDS common-server-headers.rl)


add_executable(oio-kinetic-listener kinetic-listener.cpp)
target_link_libraries(oio-kinetic-listener oio-data-kinetic
		${MILL_LIBRARIES} ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})


add_executable(oio-kinetic-proxy
        ${CMAKE_CURRENT_BINARY_DIR}/kinetic-proxy-headers.cpp
        kinetic-proxy-headers.h
        kinetic-proxy.cpp)
target_link_libraries(oio-kinetic-proxy
        oio-http-parser oio-server oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})

add_executable(oio-ec-proxy
        ${CMAKE_CURRENT_BINARY_DIR}/ec-proxy-headers.cpp
        ec-proxy-headers.h
        ec-proxy.cpp)
target_link_libraries(oio-ec-proxy
        oio-http-parser oio-server oio-data-ec
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})

add_executable(kinetic-stress-put kinetic-stress-put.cpp)
target_link_libraries(kinetic-stress-put oio-data-kinetic
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})


add_executable(oio-rawx
        ${CMAKE_CURRENT_BINARY_DIR}/rawx-server-headers.cpp
        rawx-server-headers.h
        rawx-server.cpp)
target_link_libraries(oio-rawx
        oio-http-parser oio-server oio-data-local
        ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})
