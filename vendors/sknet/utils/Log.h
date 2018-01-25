#ifndef __SKNET_UTILS_LOG_H__
#define __SKNET_UTILS_LOG_H__
#include<stdio.h>
#include<assert.h>
#include"SkLog.h"
#ifdef SKPLAT_DISABLE_LOG
#define ALOGV
#define ALOGD
#define ALOGI
#define ALOGW
#define ALOGE
#define ALOGF

#define ALOGV_IF
#define ALOGD_IF
#define ALOGI_IF
#define ALOGW_IF
#define ALOGE_IF
#define ALOGF_IF

#define ALOG_ASSERT
#define ASSERT
#define LOG_ALWAYS_FATAL_IF
#define LOG_FATAL_IF
#else
#define ALOGV skverbose
#define ALOGD skdebug
#define ALOGI skinfo
#define ALOGW skwarn
#define ALOGE skerror
#define ALOGF skfatal

#define ALOGV_IF skverbose_if
#define ALOGD_IF skdebug_if
#define ALOGI_IF skinfo_if
#define ALOGW_IF skwarn_if
#define ALOGE_IF skerror_if
#define ALOGF_IF skfatal_if

#define ALOG_ASSERT skassert
#define ASSERT  skassert
#define LOG_ALWAYS_FATAL_IF skassert_if
#define LOG_FATAL_IF LOG_ALWAYS_FATAL_IF
#define ALOGDUMP sk_debug_dump

#endif

/*
#define ALOGD(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOGI(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOGW(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOGE(...) {printf(__VA_ARGS__);printf("\n");};
#define ALOG_ASSERT(condition,...) if(!(condition)){ALOGE(__VA_ARGS__);assert(condition);}
#define ASSERT(condition,...) if(!(condition)){ALOGE(__VA_ARGS__);assert(condition);}
#define LOG_ALWAYS_FATAL_IF(condition,...) if((condition)){ALOGE(__VA_ARGS__);}
#define LOG_FATAL_IF LOG_ALWAYS_FATAL_IF
#define ALOG(level,tag,...) ALOGD(__VA_ARGS__)
*/
#endif
