cmake_minimum_required(VERSION 3.10)
project(common)


find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_custom_target(
        generate_logs ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/LogFilter.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set(GENERATE_TARGET generate_logs)
else()
    set(GENERATE_TARGET "")
endif()

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(common ${SOURCES})

if(GENERATE_TARGET)
    add_dependencies(common ${GENERATE_TARGET})
endif()

target_include_directories(common 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
 