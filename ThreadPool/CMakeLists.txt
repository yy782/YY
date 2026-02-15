cmake_minimum_required(VERSION 3.10)
project(threadpool LANGUAGES CXX)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(threadpool ${SOURCES})

target_include_directories(threadpool 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_subdirectory(threadpool_test)



target_link_libraries(threadpool
    PRIVATE
    common
)
add_dependencies(threadpool common)    