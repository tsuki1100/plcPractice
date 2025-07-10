cmake_minimum_required(VERSION 3.15)
project(ToyC-Compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Flex 和 Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 生成词法分析器和语法分析器
FLEX_TARGET(ToyC_Lexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
BISON_TARGET(ToyC_Parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp)

ADD_FLEX_BISON_DEPENDENCY(ToyC_Lexer ToyC_Parser)

# 包含目录
include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 源文件
set(SOURCES
    src/main.cpp
    src/ast/ast.cpp
    src/semantic/analyzer.cpp
    src/codegen/riscv.cpp
    src/utils/utils.cpp
    ${FLEX_ToyC_Lexer_OUTPUTS}
    ${BISON_ToyC_Parser_OUTPUTS}
)

# 创建可执行文件
add_executable(toyc ${SOURCES})

# 编译选项
target_compile_options(toyc PRIVATE -Wall -Wextra -g)

# 忽略flex/bison生成代码的警告
set_source_files_properties(
    ${FLEX_ToyC_Lexer_OUTPUTS} ${BISON_ToyC_Parser_OUTPUTS}
    PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -Wno-sign-compare"
)

# 添加测试目标
add_custom_target(test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/toyc
    DEPENDS toyc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running tests"
)