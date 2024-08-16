#ifndef _SERVER_H_
#define _SERVER_H_

#include "base.h"

// 初始化监听的套接字
int initListenFd(unsigned short port);

// 启动epoll
int epollRun(int lfd);

// 和客户端建立连接
int acceptClient(int epfd, int lfd);

// 接收http请求
int recvHttpRequest(int epfd, int cfd);

// 解析请求行
int parseRequestLine(const char* line, int cfd);

// 发送文件
int sendFile(const char* file, int cfd);

// 发送响应头
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);

// 根据文件后缀获取对应的content-type
const char* getFileType(const char* name);

// 发送目录
int sendDir(const char* dir, int cfd);

// 解决无法访问中文路径
int hexToDec(char c);
void decodeMsg(char* to, const char* from);

#endif // _SERVER_H_