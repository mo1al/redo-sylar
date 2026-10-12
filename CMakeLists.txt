cmake_minimum_required(VERSION 3.10)
project(sylar)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(yaml-cpp REQUIRED)
include_directories(include)
include_directories(include/http)
include_directories(include/service_level)
include_directories(third_party/cmark/include)
set(SOURCES
    src/config.cpp src/log.cpp src/thread.cpp src/fiber.cpp src/scheduler.cpp 
    src/iomanager.cpp src/timer.cpp src/hook.cpp src/fd_manager.cpp src/address.cpp
    src/util.cpp src/socket.cpp src/bytearray.cpp 
    src/http/http.cpp src/http/http11_parser.cpp src/http/httpclient_parser.cpp  src/http/http_parser.cpp
    src/tcpserver.cpp  src/stream.cpp src/socket_stream.cpp 
    src/http/http_server.cpp src/http/http_session.cpp src/http/servlet.cpp src/http/uri.cpp
    src/http/http_connection.cpp 
    src/service_level/service.cpp
    )  

add_executable(server main.cpp ${SOURCES})
target_link_libraries(server PRIVATE yaml-cpp::yaml-cpp ${CMAKE_SOURCE_DIR}/third_party/cmark/lib/libcmark.a)