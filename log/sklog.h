#ifndef __LOG_H__
#define __LOG_H__

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <inttypes.h>
#include "logbase.h"
#include "sktrace.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_DISABLE
#define  log_for(_level)	(false)
#define  log_assertp(...) 		 	((void)0)
#define  log_assert(...)			((void)0)
#define  log_vprint(...)			((void)0)
#define  log_print(...)				((void)0)
#define  log_write(...)				((void)0)
#else

#define sklog(level,tag, file, func, line, ...) do{ \
    if (log_for(level)) {\
        LogInfo info= {level, tag, file, func, line, {0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL);\
        log_print(&info,__VA_ARGS__); \
    } \
}while(0)

#define sklog_if(exp, level, tag, file, func, line, ...) do{ \
    if ((exp) &&  log_for(level)) {\
        LogInfo info= {level, tag, file, func, line,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL);\
        log_print(&info,__VA_ARGS__); \
    } \
}while(0)

#ifndef SKLOG_TAG
#define SKLOG_TAG "solar" 
#endif

#define __sklog_impl(level, ...) 			sklog(level,SKLOG_TAG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define __sklog_impl_if(level, exp, ...) 	sklog_if(exp, level,SKLOG_TAG, __FILE__, __func__, __LINE__, __VA_ARGS__)

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


#define skassert(exp) do { \
    if (!(exp) && log_for(kLevelFatal)) { \
        LogInfo info= {kLevelFatal, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL); \
        log_assertp(&info,#exp,"%s","FATAL"); \
    } \
}while(0)


#define skassert2(exp,...) do { \
    if (!(exp) && log_for(kLevelFatal)) { \
        LogInfo info= {kLevelFatal, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1};\
        gettimeofday(&info.timeval, NULL); \
        log_assertp(&info,#exp,__VA_ARGS__); \
    } \
}while(0)


#define verbose_loginfo(name) LogInfo name = {kLevelVerbose, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1}
#define debug_loginfo(name) LogInfo name = {kLevelDebug, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1}
#define info_loginfo(name) LogInfo name = {kLevelInfo, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1}
#define warn_loginfo(name) LogInfo name = {kLevelWarn, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1}
#define error_logInfo(name) LogInfo name = {kLevelError, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1}
#define fatal_loginfo(name) LogInfo name = {kLevelFatal, SKLOG_TAG,__FILE__, __func__, __LINE__,{0, 0}, -1, -1, -1}


#define variable_name(name,func,line) name##func##line

#define __scope_impl(level,name,msg)  SKTracer variable_name(name,__func__,__LINE__) (level, SKLOG_TAG, name, __FILE__, __func__, __LINE__,msg) 

#define verbose_scope(name,msg) __scope_impl(kLevelVerbose,name,msg)
#define debug_scope(name, msg) __scope_impl(kLevelDebug, name,msg)
#define info_scope(name,msg) __scope_impl(kLevelInfo, name,msg)

#define verbose_scope_func() __scope_impl(kLevelVerbose,__func__,NULL)
#define debug_scope_func() __scope_impl(kLevelDebug, __func__,NULL)
#define info_scope_func() __scope_impl(kLevelInfo, __func__,NULL)
#endif

#ifdef __cplusplus
}
#endif

#endif
