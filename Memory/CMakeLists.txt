cmake_minimum_required(VERSION 3.10)
project(memory_pool LANGUAGES CXX)





file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(memory_pool ${SOURCES})

target_include_directories(memory_pool 
    PUBLIC
        ${PROJECT_SOURCE_DIR}

    )
# @learn
    #include_directories是全局生效
    #target_include_directories是仅对指定目标生效，支持不同的依赖传递
#    
# @learn
    #PUBLIC 仅用于当前模块编译，传递给依赖方,所有链接了当前模块的目标，都会继承当前头文件的搜索路径
    #INTERFACE 不用于当前模块编译，传递给依赖方(纯接口文件)
    #PRIVATE 既用于自身模块编译，不传递给依赖方
#

add_subdirectory(memory_test)

target_link_libraries(memory_pool
    PUBLIC
        common
)





