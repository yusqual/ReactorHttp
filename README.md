# C/C++/Reactor
## 参考链接

[【基于多反应堆的高并发服务器【C/C++/Reactor】】](https://www.bilibili.com/video/BV1XB4y1B7P9/?share_source=copy_web&vd_source=d82b4d9c97f08207c1489029425f087f)

## 环境和工具
    # WSL2 (Ubuntu22.04)
    # CMake
    # VS Code

## 用到的头文件

### myheads/base.h
```c++
    #ifndef _MYHEADS_BASE_
    #define _MYHEADS_BASE_

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <math.h>
    #include <errno.h>
    #include <unistd.h>

    #ifndef _C_PROJECT_
    // C++项目
    static inline void err_exit(const char* msg, bool quit = true) {
        perror(msg);
        if (quit) exit(EXIT_FAILURE);
    }

    static inline void errif_exit(bool condition, const char* msg, bool quit = true) {
        if (condition) {
        perror(msg);
            if (quit) exit(EXIT_FAILURE);
        }
    }

    #else
    // C项目
    static inline void err_exit(const char* msg, int quit) {
        perror(msg);
        if (quit) exit(EXIT_FAILURE);
    }

    static inline void errif_exit(bool condition, const char* msg, int quit) {
        if (condition) {
            perror(msg);
            if (quit) exit(EXIT_FAILURE);
        }
    }
    #endif  // _C_PROJECT_

    #endif  // _MYHEADS_BASE_

```