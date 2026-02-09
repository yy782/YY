cmake_minimum_required(VERSION 3.10)
project(threadpool LANGUAGES CXX)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(threadpool ${SOURCES})

target_include_directories(threadpool 
    PUBLIC
        ${PROJECT_SOURCE_DIR}
    )

add_subdirectory(threadpool_test)



target_link_libraries(threadpool
    PUBLIC
    common
)
add_dependencies(threadpool common)    