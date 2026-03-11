cmake_minimum_required(VERSION 3.10)
project(udp_module LANGUAGES CXX)

file(GLOB UDP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 将 UDP 目录下的文件添加到主 net 库中
target_sources(net PRIVATE ${UDP_SOURCES})
