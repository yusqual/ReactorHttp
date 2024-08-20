# CMakeLists.txt

# C11(GNU11)
set(CMAKE_C_STANDARD 11)


# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 添加头文件目录  
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 查找源文件  
set(SOURCES  
    src/buffer.c
    src/channel_map.c
    src/channel.c
    src/epollDispatcher.c
    src/eventLoop.c
    src/httpRequest.c
    src/httpResponse.c
    src/main.c
    src/poolDispatcher.c
    src/selectDispatcher.c
    src/tcpConnection.c
    src/tcpServer.c
    src/threadPool.c
    src/workerThread.c
)  

# 创建可执行文件  
add_executable(reactorHttp ${SOURCES})  

# 链接动态库  
# target_link_libraries(reactorHttp pthread)