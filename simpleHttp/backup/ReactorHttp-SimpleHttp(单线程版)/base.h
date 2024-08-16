#ifndef _BASE_H_
#define _BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>

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

#endif // _BASE_H_