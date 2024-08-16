# # CMakeLists.txt

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 添加头文件目录  
include_directories(inc)  

# 查找源文件  
set(SOURCES  
    src/main.cpp
    src/server.cpp
    src/threadpool.cpp
)  

# 创建可执行文件  
add_executable(simpleHttp ${SOURCES})  

# 链接动态库  
target_link_libraries(simpleHttp pthread)