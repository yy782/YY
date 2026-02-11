cmake_minimum_required(VERSION 3.10)


project(yy LANGUAGES CXX)

# @learn 
    # project必须在cmake_minimum_required之后,其他文件之前
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-g -O0)

add_compile_options(-Wno-unknown-pragmas)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# @learn
    #CMAKE_ARCHIVE_OUTPUT_DIRECTORY是归档文件输出目录，(静态库文件)
    #CMAKE_LIBRARY_OUTPUT_DIRECTORY是动态库文件输出目录
    #CMAKE_RUNTIME_OUTPUT_DIRECTORY是可执行文件输出目录
#
add_subdirectory(Common)
add_subdirectory(Memory)
add_subdirectory(ThreadPool)
add_subdirectory(Net)
# @learn 
    #add_subdirectory多目录需要拆开调用
#
add_library(yy INTERFACE)


target_link_libraries(yy 
    INTERFACE 
        common 
        memory_pool
        net
        )