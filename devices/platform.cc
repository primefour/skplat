/*******************************************************************************
 **      Filename: platform.cc
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-08-14 16:21:11
 ** Last Modified: 2017-08-14 16:21:11
 ******************************************************************************/
#ifdef ANDROID
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"

#define CONSOLE_ENABLE true

intmax_t GetPid(){
    intmax_t pid = getpid();
    return pid;
}

intmax_t GetTid(){
    return gettid();
}

intmax_t GetMainTid(){
    intmax_t pid = getpid();
    return pid;
}

void ConsolePrint(int level,const char *log){
    printf("hello world");
}

void ConsolePrintf(const char* _tips_format, ...) {
    if (NULL == _tips_format || !CONSOLE_ENABLE) {
        return;
    }

    char tips_info[4096] = {0};
    va_list ap;
    va_start(ap, _tips_format);
    vsnprintf(tips_info, sizeof(tips_info), _tips_format, ap);
    va_end(ap);
    ConsolePrint(0, tips_info);
}

#endif

