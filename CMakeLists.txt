cmake_minimum_required(VERSION 3.10)
project(sylar)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(yaml-cpp REQUIRED)
include_directories(include)
include_directories(include/http)
set(SOURCES 
    src/config.cpp src/log.cpp src/thread.cpp src/fiber.cpp src/scheduler.cpp 
    src/iomanager.cpp src/timer.cpp src/hook.cpp src/fd_manager.cpp src/address.cpp
    src/util.cpp src/socket.cpp src/bytearray.cpp 
    src/http/http.cpp src/http/http11_parser.cpp src/http/httpclient_parser.cpp  src/http/http_parser.cpp
    src/tcpserver.cpp  src/stream.cpp src/socket_stream.cpp 
    src/http/http_server.cpp src/http/http_session.cpp src/http/servlet.cpp src/http/uri.cpp
    src/http/http_connection.cpp 
    )  
# add_executable(test ${SOURCES} test/test_http_connection.cpp)
# add_executable(test1 ${SOURCES} test/test_http_server.cpp)
add_executable(server src/main.cpp ${SOURCES})
target_link_libraries(server PRIVATE yaml-cpp::yaml-cpp)
add_executable(test_main test/test_main.cpp ${SOURCES})
target_link_libraries(test_main PRIVATE yaml-cpp::yaml-cpp)
# add_executable(test_config ${SOURCES} test/test_config.cpp)
# add_executable(test_util ${SOURCES} test/test_util.cpp)
# target_link_libraries(test PRIVATE yaml-cpp::yaml-cpp)
# target_link_libraries(test1 PRIVATE yaml-cpp::yaml-cpp)
# target_link_libraries(test_config PRIVATE yaml-cpp::yaml-cpp )
# target_link_libraries(test_util PRIVATE yaml-cpp::yaml-cpp )