cmake_minimum_required(VERSION 3.10)
project(protobuf_module LANGUAGES CXX)

file(GLOB PROTOBUF_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 将 protobuf 目录下的文件添加到主 net 库中
target_sources(net PRIVATE ${PROTOBUF_SOURCES})
