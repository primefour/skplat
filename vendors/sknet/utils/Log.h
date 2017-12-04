#ifndef __SKNET_UTILS_LOG_H__
#define __SKNET_UTILS_LOG_H__
#include<stdio.h>
#include<assert.h>

typedef int status_t;
#define NO_ERROR (0)
#define INVALID_OPERATION (-1)
#define WOULD_BLOCK (-2)
#define UNKNOWN_ERROR (-3)
#define LOG_WARN "warning"

#define ALOGD(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOGW(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOGE(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOG_ASSERT(condition,...) if(!condition){assert(condition);ALOGE(__VA_ARGS__);}
#define ASSERT(condition,...) if(!condition){assert(condition);ALOGE(__VA_ARGS__);}
#define LOG_ALWAYS_FATAL_IF(condition,...) if(!condition){ALOGE(__VA_ARGS__);}
#define ALOG(level,tag,...) ALOGD(__VA_ARGS__)
#endif
