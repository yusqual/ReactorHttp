#include "server.h"

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        printf("usage: ./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);

    // 切换当前进程的动作目录
    int ret = chdir(argv[2]);
    errif_exit(ret == -1, "chdir error");
    // 输出当前工作路径
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error\n");
    }
    
    // 初始化用于监听的套接字
    int lfd = initListenFd(port);
    // 启动服务器程序
    epollRun(lfd);

    return 0;
}