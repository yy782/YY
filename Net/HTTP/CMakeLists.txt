cmake_minimum_required(VERSION 3.10)
project(http_module LANGUAGES CXX)

file(GLOB HTTP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 将 HTTP 目录下的文件添加到主 net 库中
target_sources(net PRIVATE ${HTTP_SOURCES})
