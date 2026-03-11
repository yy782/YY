cmake_minimum_required(VERSION 3.10)
project(poller LANGUAGES CXX)

file(GLOB POLLER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 将 Poller 目录下的文件添加到主 net 库中
target_sources(net PRIVATE ${POLLER_SOURCES})
