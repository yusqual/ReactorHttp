#include "channel.h"
#include <unistd.h>

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        printf("usage: ./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);

    // 切换当前进程的动作目录
    int ret = chdir(argv[2]);
    errif_exit(ret == -1, "chdir error", true);
    // 输出当前工作路径
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Curr working dir: %s\n", cwd);
    } else {
        perror("getcwd() error\n");
    }


}