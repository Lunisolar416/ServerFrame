cmake_minimum_required(VERSION 3.16)
project(mysylar)

# 打印详细编译信息
set(CMAKE_VERBOSE_MAKEFILE ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -g -O3 -Wall -Wextra -Werror -pthread -Wno-unused-function -Wno-deprecated -Wno-unused-parameter")

# 查找系统安装的 yaml-cpp
find_package(yaml-cpp REQUIRED)

# 源文件
set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
    sylar/thread.cc
    sylar/mutex.cc
    sylar/fiber.cc
    sylar/scheduler.cc
)

# 生成共享库
add_library(sylar SHARED ${LIB_SRC})

# 添加头文件目录（让其他目标可以包含 sylar/xxx.h）
target_include_directories(sylar PUBLIC
    ${PROJECT_SOURCE_DIR}/sylar
)

# 链接 yaml-cpp 到 sylar 库
target_link_libraries(sylar PUBLIC yaml-cpp::yaml-cpp)

# ======================
# 测试 log 程序
# ======================
add_executable(logtest test/logtest.cc)
add_dependencies(logtest sylar)
target_link_libraries(logtest PRIVATE sylar)

target_include_directories(logtest PUBLIC
    ${PROJECT_SOURCE_DIR}/sylar
)

# ======================
# 测试 config 程序
# ======================
add_executable(config test/testconfig.cc)
add_dependencies(config sylar)
target_link_libraries(config PRIVATE sylar)

target_include_directories(config PUBLIC
    ${PROJECT_SOURCE_DIR}/sylar
)
# ======================
# 测试 config 程序
# ======================
add_executable(thread test/testthread.cc)
add_dependencies(thread sylar)
target_link_libraries(thread PRIVATE sylar)

target_include_directories(thread PUBLIC
    ${PROJECT_SOURCE_DIR}/sylar
)

# ======================
# 测试 util 程序
# ======================
add_executable(util test/test_util.cc)
add_dependencies(util sylar)
target_link_libraries(util PRIVATE sylar)

target_include_directories(util PUBLIC
    ${PROJECT_SOURCE_DIR}/sylar
)
# ======================
# 测试 fiber 程序
# ======================
add_executable(fiber test/test_fiber.cc)
add_dependencies(fiber sylar)
target_link_libraries(fiber PRIVATE sylar)

target_include_directories(fiber PUBLIC
    ${PROJECT_SOURCE_DIR}/sylar
)
# ======================
# 设置输出目录
# ======================
set_target_properties(sylar PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

set_target_properties(logtest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

set_target_properties(config PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)
