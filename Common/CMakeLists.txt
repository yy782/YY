cmake_minimum_required(VERSION 3.10)
project(common)

file(GLOB SOURSE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(common ${SOURSE})

target_include_directories(common 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
 