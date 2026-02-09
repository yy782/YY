cmake_minimum_required(VERSION 3.10)
project(common LANGUAGES CXX)


add_library(common INTERFACE)#INTERFACE类型的库是CMAKE专为纯头文件库设计的接口库
target_include_directories(common
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)