#ifndef __SK_LOG_H__
#define __SK_LOG_H__
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <inttypes.h>
#include "AppLogBase.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_DISABLE
#define  skLogCheckLevel(_level)	(false)
#define  skLogAssert(...)			((void)0)
#define  skLogPrint(...)			((void)0)
#else

#define sklog(level,tag, file, func, line, ...) do{ \
    if (skLogCheckLevel(level)) {\
        LogEntry info= {level, tag, file, func, line, {0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL);\
        skLogPrint(&info,__VA_ARGS__); \
    } \
}while(0)

#define sklog_if(exp, level, tag, file, func, line, ...) do{ \
    if ((exp) && skLogCheckLevel(level)) {\
        LogEntry info= {level, tag, file, func, line,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL);\
        skLogPrint(&info,__VA_ARGS__); \
    } \
}while(0)

#define skassert(exp,...) do { \
    if (!(exp) && skLogCheckLevel(kLevelFatal)) { \
        LogEntry info= {kLevelFatal, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL); \
        skLogAssert(&info,#exp,__VA_ARGS__); \
    } \
}while(0)

#ifndef SKLOG_TAG
#define SKLOG_TAG "skplat" 
#endif

#define __sklog_impl(level, ...) 			sklog(level,SKLOG_TAG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define __sklog_impl_if(level, exp, ...) 	sklog_if(exp,level,SKLOG_TAG, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define skverbose(...)             __sklog_impl(kLevelVerbose, __VA_ARGS__)
#define skdebug(...)               __sklog_impl(kLevelDebug, __VA_ARGS__)
#define skinfo(...)                __sklog_impl(kLevelInfo, __VA_ARGS__)
#define skwarn(...)                __sklog_impl(kLevelWarn, __VA_ARGS__)
#define skerror(...)               __sklog_impl(kLevelError, __VA_ARGS__)
#define skfatal(...)               __sklog_impl(kLevelFatal, __VA_ARGS__)

#define skverbose_if(exp, ...)     __sklog_impl_if(kLevelVerbose, exp, __VA_ARGS__)
#define skdebug_if(exp, ...)       __sklog_impl_if(kLevelDebug, exp, __VA_ARGS__)
#define skinfo_if(exp, ...)        __sklog_impl_if(kLevelInfo, exp, __VA_ARGS__)
#define skwarn_if(exp, ...)        __sklog_impl_if(kLevelWarn, exp,  __VA_ARGS__)
#define skerror_if(exp, ...)       __sklog_impl_if(kLevelError, exp, __VA_ARGS__)
#define skfatal_if(exp, ...)       __sklog_impl_if(kLevelFatal, exp, __VA_ARGS__)

#define variable_name(name,func,line) name##func##line

#define __scope_impl(level,name)  ScopeProfile variable_name(name,__func__,__LINE__) (level, SKLOG_TAG, name, __FILE__, __func__, __LINE__) 

#define verbose_scope(name,msg) __scope_impl(kLevelVerbose,name,msg)
#define debug_scope(name, msg) __scope_impl(kLevelDebug, name,msg)
#define info_scope(name,msg) __scope_impl(kLevelInfo, name,msg)

#endif

#ifdef __cplusplus
}
#endif

#endif
