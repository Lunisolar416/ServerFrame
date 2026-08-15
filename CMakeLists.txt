cmake_minimum_required(VERSION 3.16)
project(sylar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# =========================
# modules path
# =========================
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# =========================
# third party packages
# =========================
find_package(Boost REQUIRED)

# yaml-cpp（推荐 modern target）
find_package(yaml-cpp REQUIRED)

# =========================
# library source
# =========================
set(LIB_SRC
    sylar/address.cc
    sylar/config.cc
    sylar/fiber.cc
    sylar/hook.cc
    sylar/iomanager.cc
    sylar/log.cc
    sylar/socket.cc
    sylar/thread.cc
    sylar/util.cc
    sylar/timer.cc
    sylar/scheduler.cc
    sylar/fd_manager.cc
    sylar/mutex.cc
    sylar/bytearray.cc
    sylar/http/http.cc
    sylar/http/http_parser.cc
    sylar/http/http_session.cc
    sylar/http/http_connection.cc
    sylar/http/http_server.cc
    sylar/http/servlet.cc
    sylar/tcp_server.cc
    sylar/stream.cc
    sylar/socket_stream.cc
)
ragelmaker(
    sylar/http/http11_parser.rl
    LIB_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http
)

ragelmaker(
    sylar/http/httpclient_parser.rl
    LIB_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http
)

ragelmaker(
    sylar/uri.rl
    LIB_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar
)
# =========================
# generate sylar library
# =========================
add_library(sylar SHARED ${LIB_SRC})

target_include_directories(sylar
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(sylar
    PUBLIC
        yaml-cpp
        pthread
        dl
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${MYSQL_LIBRARIES}
)

# =========================
# safer include style (IMPORTANT)
# =========================
target_include_directories(sylar SYSTEM PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
)

# =========================
# output dirs
# =========================
set_target_properties(sylar PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

# =========================
# test helper macro
# =========================
function(add_sylar_test name file)
    add_executable(${name} ${file})
    target_link_libraries(${name} PRIVATE sylar)
    target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    )
endfunction()

# =========================
# tests (clean version)
# =========================
add_sylar_test(logtest test/logtest.cc)
add_sylar_test(config test/testconfig.cc)
add_sylar_test(thread test/testthread.cc)
add_sylar_test(util test/test_util.cc)
add_sylar_test(fiber test/test_fiber.cc)
add_sylar_test(scheduler test/test_scheduler.cc)
add_sylar_test(iomanager test/test_iomanager.cc)
add_sylar_test(hook test/test_hook.cc)
add_sylar_test(address test/test_address.cc)
add_sylar_test(socket test/test_socket.cc)
add_sylar_test(bytearray test/test_bytearray.cc)
add_sylar_test(http test/test_http.cc)
add_sylar_test(httpparser test/test_httpparser.cc)
add_sylar_test(tcpserver test/test_tcp_server.cc)
add_sylar_test(echoserver example/echo_server.cc)
add_sylar_test(httpserver test/test_httpserver.cc)
add_sylar_test(httpconnection test/test_httpconnection.cc)
add_sylar_test(uri test/test_uri.cc)