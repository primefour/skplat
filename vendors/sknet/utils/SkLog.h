#ifndef __SK_LOG_H__
#define __SK_LOG_H__
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <inttypes.h>
#include "AppLogBase.h"

#ifndef SKLOG_TAG
#define SKLOG_TAG "skplat" 
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define sklog(level,tag, file, func, line, ...) do{ \
    if (skLogCheckLevel(level)) {\
        LogEntry info= {level, tag, file, func, line, {0, 0}, -1, -1, -1};\
        gettimeofday(&info.tv, NULL);\
        skLogPrint(&info,__VA_ARGS__); \
    } \
}while(0)

#define sklog_if(exp, level, tag, file, func, line, ...) do{ \
    if ((exp) && skLogCheckLevel(level)) {\
        LogEntry info= {level, tag, file, func, line,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.tv, NULL);\
        skLogPrint(&info,__VA_ARGS__); \
    } \
}while(0)

#define skassert(exp,...) do { \
    if (!(exp) && skLogCheckLevel(LogLevelFatal)) { \
        LogEntry info= {LogLevelFatal, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.tv, NULL); \
        skLogAssert(&info,#exp,__VA_ARGS__); \
    } \
}while(0)

#define skassert_if(exp,...) do { \
    if ((exp) && skLogCheckLevel(LogLevelFatal)) { \
        LogEntry info= {LogLevelFatal, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.tv, NULL); \
        skLogAssert(&info,#exp,__VA_ARGS__); \
    } \
}while(0)

#define __sklog_impl(level, ...) 			sklog(level,SKLOG_TAG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define __sklog_impl_if(level, exp, ...) 	sklog_if(exp,level,SKLOG_TAG, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define skverbose(...)             __sklog_impl(LogLevelVerbose, __VA_ARGS__)
#define skdebug(...)               __sklog_impl(LogLevelDebug, __VA_ARGS__)
#define skinfo(...)                __sklog_impl(LogLevelInfo, __VA_ARGS__)
#define skwarn(...)                __sklog_impl(LogLevelWarn, __VA_ARGS__)
#define skerror(...)               __sklog_impl(LogLevelError, __VA_ARGS__)
#define skfatal(...)               __sklog_impl(LogLevelFatal, __VA_ARGS__)

#define skverbose_if(exp, ...)     __sklog_impl_if(LogLevelVerbose, exp, __VA_ARGS__)
#define skdebug_if(exp, ...)       __sklog_impl_if(LogLevelDebug, exp, __VA_ARGS__)
#define skinfo_if(exp, ...)        __sklog_impl_if(LogLevelInfo, exp, __VA_ARGS__)
#define skwarn_if(exp, ...)        __sklog_impl_if(LogLevelWarn, exp,  __VA_ARGS__)
#define skerror_if(exp, ...)       __sklog_impl_if(LogLevelError, exp, __VA_ARGS__)
#define skfatal_if(exp, ...)       __sklog_impl_if(LogLevelFatal, exp, __VA_ARGS__)

#define variable_name(name,func,line) name##func##line

#define __scope_impl(level,name)  ScopeProfile variable_name(name,__func__,__LINE__) (level, SKLOG_TAG, name, __FILE__, __func__, __LINE__) 

#define verbose_scope(name,msg) __scope_impl(LogLevelVerbose,name,msg)
#define debug_scope(name, msg) __scope_impl(LogLevelDebug, name,msg)
#define info_scope(name,msg) __scope_impl(LogLevelInfo, name,msg)

#ifdef __cplusplus
}
#endif

#endif
