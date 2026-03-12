cmake_minimum_required(VERSION 3.10)
project(common)

add_custom_target(
    generate_logs ALL          # 目标名称，ALL 表示每次都运行
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/LogFilter.py  # 要执行的命令
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}  # 命令执行的工作目录
)

file (GLOB SOURSE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(common ${SOURSE})
add_dependencies(common generate_logs)
target_include_directories(common 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
 