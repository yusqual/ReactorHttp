# CMakeLists.txt

# C++11
set(CMAKE_CXX_STANDARD 11)


file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 添加头文件目录  
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 查找源文件  
set(SOURCES  
    src/buffer.cpp
    src/channel.cpp
    src/dispatcher.cpp
    src/epollDispatcher.cpp
    src/eventLoop.cpp
    src/httpRequest.cpp
    src/httpResponse.cpp
    src/main.cpp
    src/pollDispatcher.cpp
    src/selectDispatcher.cpp
    src/tcpConnection.cpp
    src/tcpServer.cpp
    src/threadPool.cpp
    src/workerThread.cpp
)  

# 创建可执行文件  
add_executable(reactorHttp-CPP ${SOURCES})  

# 链接动态库  
# target_link_libraries(reactorHttp pthread)