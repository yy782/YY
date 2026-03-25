cmake_minimum_required(VERSION 3.10)


project(yy LANGUAGES CXX)

# @learn 
    # project必须在cmake_minimum_required之后,其他文件之前
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#cmake -DCMAKE_BUILD_TYPE=Release ..
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# @learn
    #CMAKE_ARCHIVE_OUTPUT_DIRECTORY是归档文件输出目录，(静态库文件)
    #CMAKE_LIBRARY_OUTPUT_DIRECTORY是动态库文件输出目录
    #CMAKE_RUNTIME_OUTPUT_DIRECTORY是可执行文件输出目录
#

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WARNING_OPTIONS
        -g -O0
        -std=c++17
        -DNEXCLUDE_BEFORE_COMPILATION
        -rdynamic
        -march=native
        -Wall
        -Wextra                 #额外警告
        -Wpedantic              #严格遵守C++标准
        -Werror                 #将警告视为错误
        -Wno-unknown-pragmas    #需要关闭的警告
        -Wshadow                #变量遮掩警告
        -Wconversion            #类型转换警告
        -Wunused-parameter     #未使用参数警告
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wwrite-strings

    )


else()
    set(WARNING_OPTIONS
        -O3                     # 生产优化
        -std=c++17
        -march=native
        -DNEXCLUDE_BEFORE_COMPILATION
        -DNDEBUG
        -Wall
        -Wextra
        -Wno-error              # 警告不升级为错误
        -Wno-shadow
        -Wno-conversion    
    )

endif()    

add_subdirectory(Common)
add_subdirectory(MemoryPool)
add_subdirectory(ThreadPool)
add_subdirectory(Net)
# @learn 
    #add_subdirectory多目录需要拆开调用
#

target_compile_options(common
    PRIVATE
    ${WARNING_OPTIONS}
)
target_compile_options(memorypool
    PRIVATE
    ${WARNING_OPTIONS}
)
target_compile_options(threadpool
    PRIVATE
    ${WARNING_OPTIONS}
)
target_compile_options(net
    PRIVATE
    ${WARNING_OPTIONS}
)

add_library(yy INTERFACE)


target_link_libraries(yy 
    INTERFACE 
        common 
        memorypool
        net
        threadpool
        )