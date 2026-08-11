cmake_minimum_required(VERSION 3.16)
project(toyc_compiler LANGUAGES CXX)
# 题目要求 C++20；关闭编译器扩展以减少平台差异。
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
# Bison/Flex 的输出放入构建目录，不污染源码目录。
BISON_TARGET(
    Parser
    src/parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)
FLEX_TARGET(Lexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)
# 手写源码与两个生成文件共同组成最终可执行文件。
add_executable(
    toyc_compiler
    src/main.cpp
    src/compiler.cpp
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
)
target_include_directories(toyc_compiler PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
# GCC/Clang 开启常用警告，MSVC 使用对应的警告级别。
if(MSVC)
    target_compile_options(toyc_compiler PRIVATE /W4)
else()
    target_compile_options(toyc_compiler PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
